// The renderer behind a recording of the title's calls (docs/renderer.md). The title's thread records each call with
// what the console takes at call time -- the state that changed, the resource headers it names, its indices and
// rects, the game's side of it (gpu::Peek) -- and the recording is drawn from: vertex, texture and surface memory are
// read when the draw runs, as the console's GPU reads them after the call returns. Adapted from TGP482's Xbox XBE
// Recomp SDK (runtime/gpu/render_thread.cpp; see NOTICE): the resource headers are kept as copies the renderer reads
// through (gpu/headers.h) instead of copies at other addresses, the game's side and the quick menu travel with the
// calls, and the GPU emulation is what the title waits on: its push buffer is read only as far as the recording has
// been drawn (nv2a_pfifo_gate), so every reference, fence and semaphore the title waits on follows the drawing.
//
// NGB_D3D_THREAD=1: drawn on a thread of its own, the title recording one frame while the one before it draws.
// NGB_D3D_THREAD=capture: each call drawn from its recording at once, on the title's thread (a test of the recording).
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <mutex>
#include <thread>

#include "platform/vcpu.h"   // the title's wait for a frame gives the virtual CPU away meanwhile
#include <unordered_map>
#include <vector>

#include "gpu/backend.h"
#include "gpu/headers.h"
#include "kernel/kernel.h"

extern "C" {
void nv2a_pfifo_gate(int on, uint32_t put);   // xboxrecomp xemu/pfifo.c
uint32_t nv2a_pfifo_put(void);
extern void (*nv2a_user_read_hook)(void);     // xboxrecomp xemu/user.c
}

// NGB_D3D_GATE=0 (a test): the GPU emulation not held back
static void Gate(int on, uint32_t put) {
    static const bool off = std::getenv("NGB_D3D_GATE") && *std::getenv("NGB_D3D_GATE") == '0';
    nv2a_pfifo_gate(off ? 0 : on, put);
}

namespace gpu {

namespace {

// commands handed over at once, so the render thread keeps close behind
constexpr uint32_t kBatchCommands = 64;
constexpr size_t kFreeBatches = 64;   // spare batches kept for reuse
// the render thread, waiting with nothing queued, takes what the title has recorded so far after this long: the
// title may be waiting on memory the GPU emulation writes (a semaphore) behind calls it has not handed over
constexpr auto kIdleTake = std::chrono::microseconds(1500);

// NV2A_PACE_LOG (testing): the hand-overs, every 2 s
struct PaceCounts {
    std::atomic<uint32_t> hook{0}, hook_busy{0}, hook_submit{0}, hook_gate{0}, idle{0}, idle_busy{0}, idle_submit{0},
        batches{0};
    std::atomic<uint64_t> lat_us{0}, lat_max{0}, exec_us{0};
};
static PaceCounts g_pace;
static bool PaceOn() {   // (read late: on Android env.txt is applied after the library is loaded)
    static const bool on = std::getenv("NV2A_PACE_LOG") != nullptr;
    return on;
}
#define g_pace_on PaceOn()
static uint64_t PaceNowUs() {
    return uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

enum class Op : uint8_t { State, Headers, Peek, Draw, Clear, Copy, Present, Flush, Gamma, VisBegin, VisEnd };

struct Cmd {
    Op op;
    uint32_t a = 0, b = 0, c = 0, d = 0;
    float f = 0;
    uint32_t data = 0, size = 0;   // bytes in Batch::bytes
    int32_t vs = -1, ps = -1;      // index in Batch::vs / ps: the shader changed
    int32_t quick = -1;            // index in Batch::quick: the frame's quick menu
    Surface s1, s2;
};

// a header copy as sent: data, when the call's own (a DrawVerticesUP's vertices: the title may reuse its memory once
// the call returns), at that offset in the batch's bytes
struct HeaderSend {
    uint32_t addr;
    uint32_t w[5];
    int32_t data = -1;
};

struct Batch {
    uint64_t id = 0;
    uint64_t handed_us = 0;   // (NV2A_PACE_LOG)
    uint32_t put = 0;   // the title's DMA_PUT as it was handed over: the push buffer may be read this far once drawn
    std::vector<Cmd> cmds;
    std::vector<uint8_t> bytes;
    std::vector<std::shared_ptr<VertexShader>> vs;
    std::vector<std::shared_ptr<PixelShader>> ps;
    std::vector<QuickPeek> quick;
};

// The plain data in State around its shader pointers, sent as it changes in 64 byte pieces. Of the simple render
// states only those the renderer reads (gpu/state.h's kM...: the pixel shader's combiners, alpha test to depth offset,
// the combiner constants, depth clip control), of the transforms only those drawn with: view and projection (2, 3),
// the textures' (16-19) and the world matrices (256-259).
struct Range {
    size_t off, len;
};
size_t At(const State& s, const void* p) {
    return size_t(static_cast<const uint8_t*>(p) - reinterpret_cast<const uint8_t*>(&s));
}
std::vector<Range> StateRanges() {
    static const State s;
    auto simple = [&](uint32_t first, uint32_t last) {   // methods first to last
        return Range{At(s, &s.simple[first >> 2]), size_t(last - first) + 4};
    };
    return {
        simple(kMPsAlphaInputs, kMPsFinalEfgh),
        simple(kMAlphaTestEnable, kMDepthOffset),
        simple(kMPsConstant0, kMPsRgbInputs + 0x1C),
        simple(kMPsCompareMode, kMPsCompareMode),
        simple(kMDepthClipControl, kMDepthClipControl),
        simple(kMPsFinalConstant0, kMPsInputTexture),
        {At(s, &s.rs), At(s, &s.index_buffer) - At(s, &s.rs)},          // render states, textures, the streams
        {At(s, &s.index_buffer), At(s, &s.vs) - At(s, &s.index_buffer)},
        {At(s, &s.vsc), At(s, &s.transforms) - At(s, &s.vsc)},            // constants, viewport, surfaces, scissors
        {At(s, &s.transforms[2]), 2 * sizeof(s.transforms[0])},
        {At(s, &s.transforms[16]), 4 * sizeof(s.transforms[0])},
        {At(s, &s.transforms[256]), 4 * sizeof(s.transforms[0])},
        {At(s, &s.material), sizeof(State) - At(s, &s.material)},
    };
}

constexpr size_t kPiece = 64;

class Recorded;
std::atomic<Recorded*> g_recorded{nullptr};   // for the title's progress reads (nv2a_user_read_hook)
std::atomic<int> g_hook_busy{0};              // guest threads inside the hook, which the destructor lets out first
#if XBOX_HAS_VCPU
extern "C" void compat_host_enter(void);   // a host section: not parked while holding cap_ (xboxrecomp win32_compat.h)
extern "C" void compat_host_leave(void);
#else
static inline void compat_host_enter(void) {}
static inline void compat_host_leave(void) {}
#endif
// A long wait is logged, never ended: the F1 menu holds the render thread on purpose (present_sdl.c overlay_run),
// and a minimized window or a first shader build can hold it a while too
constexpr int kStallLogSeconds = 10;

class Recorded final : public Backend {
public:
    Recorded(std::unique_ptr<Backend> inner, bool thread) : inner_(std::move(inner)), ranges_(StateRanges()), thread_(thread) {
        cur_ = NewBatch();
        if (thread_) {
            g_recorded.store(this);
            Gate(1, nv2a_pfifo_put());   // what the title put before the first recording: read
            // from guest code (the title polling the GPU's progress): a host section around it, or a guest thread
            // parked here holding cap_ froze the next one to make a Direct3D call (it holds the CPU and g_lock)
            nv2a_user_read_hook = [] {
                g_hook_busy.fetch_add(1);
                compat_host_enter();
                if (Recorded* r = g_recorded.load()) r->TitleWaits();
                compat_host_leave();
                g_hook_busy.fetch_sub(1);
            };
            std::thread t([this] { Run(); });
            render_ = t.get_id();
            t.detach();
        }
    }
    // Only exit() comes here (the game leaves by _exit): the thread is stopped and waited for, so nothing of this is
    // freed under it. A guest thread in the hook is let out first.
    ~Recorded() override {
        if (!thread_) return;
        nv2a_user_read_hook = nullptr;
        g_recorded.store(nullptr);
        for (int i = 0; g_hook_busy.load(); i++) {   // (a thread held in the hook past 5 s: leaving, not freeing)
            if (i >= 50000) {
                std::fprintf(stderr, "[D3D] a thread still in the progress hook after 5 s: leaving\n");
                std::fflush(nullptr);
                std::_Exit(1);
            }
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        {
            std::lock_guard lock(m_);
            stop_ = true;
        }
        cv_.notify_all();
        nv2a_pfifo_gate(0, 0);
        if (std::this_thread::get_id() == render_) {
            // exit() on the render thread itself (a fatal renderer error, its message already shown): Run never
            // resumes, and the title's thread may be waiting on what this would free -- leaving now
            std::fflush(nullptr);
            std::_Exit(1);
        }
        std::unique_lock lock(m_);
        if (!done_cv_.wait_for(lock, std::chrono::seconds(5), [&] { return exited_; })) {
            // held in a GPU wait that is not ending: the process goes now, as the game's own exit does, rather than
            // free the queue and the renderer under the thread
            std::fprintf(stderr, "[D3D] the render thread did not stop in 5 s: leaving\n");
            std::fflush(nullptr);
            std::_Exit(1);
        }
    }

    bool Init(uint32_t width, uint32_t height) override { return inner_->Init(width, height); }

    void Draw(const State& s, uint32_t prim, uint32_t start, uint32_t count, const uint8_t* indices) override {
        std::lock_guard lock(cap_);
        SendState(s, start + count);
        Cmd c{Op::Draw};
        c.a = prim;
        c.b = start;
        c.c = count;
        if (indices) {
            c.d = 1;
            c.data = Put(indices, size_t(count) * 2);
        }
        cur_->cmds.push_back(c);
        if (!thread_ || cur_->cmds.size() >= kBatchCommands) Submit();
    }

    void Clear(const State& s, uint32_t count, const uint8_t* rects, uint32_t flags, uint32_t color, float z,
               uint32_t stencil) override {
        std::lock_guard lock(cap_);
        SendState(s, 0);
        Cmd c{Op::Clear};
        c.a = count;
        c.b = flags;
        c.c = color;
        c.d = stencil;
        c.f = z;
        if (count && rects) {
            c.data = Put(rects, size_t(count) * 16);
            c.size = count * 16;
        }
        cur_->cmds.push_back(c);
        if (!thread_) Submit();
    }

    // rects and points are taken now; the copy itself runs in order
    void Copy(const Surface& src, const Surface& dst, uint32_t count, const uint8_t* src_rects,
              const uint8_t* dst_points) override {
        std::lock_guard lock(cap_);
        SendHeader(src.header);
        SendHeader(dst.header);
        FlushHeaders();
        Cmd c{Op::Copy};
        c.a = count;
        c.b = count && src_rects ? Put(src_rects, count * 16) + 1 : 0;     // (offsets + 1: 0 none)
        c.c = count && dst_points ? Put(dst_points, count * 8) + 1 : 0;
        c.s1 = src;
        c.s2 = dst;
        cur_->cmds.push_back(c);
        if (!thread_) Submit();
    }

    void Present(const Surface& front) override {
        uint64_t id;
        {
            std::lock_guard lock(cap_);
            SendHeader(front.header);
            FlushHeaders();
            SendPeek(sent_);
            Cmd c{Op::Present};
            c.s1 = front;
            c.quick = int32_t(cur_->quick.size());
            cur_->quick.emplace_back();
            inner_->CaptureQuick(cur_->quick.back());
            cur_->cmds.push_back(c);
            id = Submit();
        }
        // the title records one frame while the one before it draws: the wait for that one in FrameWait, which
        // the hook calls once g_lock and the host section are let go -- the wait gives the virtual CPU away, and
        // another guest thread taking it and then g_lock (a Direct3D call: the in-game menu's) froze the game
        if (thread_) {
            frame_wait_ = last_present_;
            last_present_ = id;
        }
    }
    void FrameWait() override {
        uint64_t id = frame_wait_;
        frame_wait_ = 0;
        if (thread_ && id) Wait(id);
    }

    void Sync(uint32_t data, uint32_t size) override {
        if (!thread_) inner_->Sync(data, size);   // (the renderer writes nothing back to the title's memory)
    }
    void SyncLate(uint32_t data) override {
        if (!thread_) inner_->SyncLate(data);
    }
    void Flush() override {
        std::lock_guard lock(cap_);
        cur_->cmds.push_back(Cmd{Op::Flush});
        Submit();
    }
    void SetGamma(const uint8_t* ramp) override {
        std::lock_guard lock(cap_);
        Cmd c{Op::Gamma};
        c.data = Put(ramp, 768);
        cur_->cmds.push_back(c);
        if (!thread_) Submit();
    }
    void VisibilityBegin() override {
        std::lock_guard lock(cap_);
        cur_->cmds.push_back(Cmd{Op::VisBegin});
        if (!thread_) Submit();
    }
    void VisibilityEnd(uint32_t report) override {
        std::lock_guard lock(cap_);
        Cmd c{Op::VisEnd};
        c.a = report;
        cur_->cmds.push_back(c);
        if (!thread_) Submit();
    }

    // The title reads the push buffer's progress (it may be waiting on it): what it has recorded since the last
    // batch goes now (Release). Skipped while a call is being recorded.
    void TitleWaits() {
        if (g_pace_on) g_pace.hook++;
        if (!cap_.try_lock()) {
            if (g_pace_on) g_pace.hook_busy++;
            return;
        }
        if (g_pace_on) (cur_->cmds.empty() ? g_pace.hook_gate : g_pace.hook_submit)++;
        Release();
        cap_.unlock();
    }

private:
    Batch* NewBatch() {
        Batch* b;
        {
            std::lock_guard lock(m_);
            if (free_.empty()) {
                b = new Batch;
            } else {
                b = free_.back();
                free_.pop_back();
            }
        }
        b->id = ++next_id_;
        return b;
    }

    uint32_t Put(const void* p, size_t n) {
        size_t at = (cur_->bytes.size() + 3) & ~size_t(3);
        cur_->bytes.resize(at + n);
        std::memcpy(cur_->bytes.data() + at, p, n);
        return uint32_t(at);
    }

    // a header as it is now, to the renderer's copies when it changed since it was last sent
    void SendHeader(uint32_t header) {
        if (!header) return;
        HeaderCopy now{};
        // vertex buffer, index buffer and palette headers are 12 bytes: reading 20 can run off the end of the heap
        uint32_t type = kernel::R32(header) & 0x70000;   // 0 vertex, 0x10000 index, 0x30000 palette
        std::memcpy(now.w, kernel::HostPtr(header), type < 0x20000 || type == 0x30000 ? 12 : 20);
        Recent& r = recent_[(header >> 4) & 63];   // the last few, in front of the map (its elements stay put)
        if (r.key != header) r = {header, &sent_headers_[header]};
        if (!std::memcmp(r.copy->w, now.w, sizeof now.w)) return;
        *r.copy = now;
        HeaderSend h{header, {}};
        std::memcpy(h.w, now.w, sizeof h.w);
        pending_headers_.push_back(h);
    }
    struct Recent {
        uint32_t key = 0;
        HeaderCopy* copy = nullptr;
    };
    Recent recent_[64];
    // a vertex buffer of the renderer's own, its data in the title's memory (DrawVerticesUP: Common bit 31): the
    // vertices copied, the copy's header pointing at them
    void SendUpHeader(uint32_t header, uint32_t bytes) {
        HeaderSend h{header, {}};
        std::memcpy(h.w, kernel::HostPtr(header), 12);
        h.data = int32_t(Put(kernel::HostPtr(h.w[1]), bytes));
        std::memset(sent_headers_[header].w, 0, sizeof h.w);   // (always sent: its vertices are new)
        recent_[(header >> 4) & 63] = {};
        pending_headers_.push_back(h);
    }
    void FlushHeaders() {
        if (pending_headers_.empty()) return;
        Cmd c{Op::Headers};
        c.data = Put(pending_headers_.data(), pending_headers_.size() * sizeof(pending_headers_[0]));
        c.size = uint32_t(pending_headers_.size());
        cur_->cmds.push_back(c);
        pending_headers_.clear();
    }

    // the game's side of the call, when it changed
    void SendPeek(const State& s) {
        Peek p;
        std::memset(&p, 0, sizeof p);
        inner_->CapturePeek(s, p);
        if (peek_sent_ && !std::memcmp(&p, &sent_peek_, sizeof p)) return;
        peek_sent_ = true;
        sent_peek_ = p;
        Cmd c{Op::Peek};
        c.data = Put(&p, sizeof p);
        cur_->cmds.push_back(c);
    }

    // a field of the state as the renderer has it, when it differs from what was sent
    void Delta(const void* field, const void* value, size_t n) {
        uint8_t* sent = reinterpret_cast<uint8_t*>(&sent_) + At(sent_, field);
        if (!std::memcmp(sent, value, n)) return;
        std::memcpy(sent, value, n);
        uint32_t head[2] = {uint32_t(At(sent_, field)), uint32_t(n)};
        size_t at = cur_->bytes.size();
        cur_->bytes.resize(at + 8 + n);
        std::memcpy(cur_->bytes.data() + at, head, 8);
        std::memcpy(cur_->bytes.data() + at + 8, value, n);
    }

    // what changed since the last command, in 64 byte pieces; the headers the call names; the game's side.
    // vertices: how many a non-indexed draw reads (a DrawVerticesUP's to copy)
    void SendState(const State& s, uint32_t vertices) {
        title_ = std::this_thread::get_id();
        const uint8_t* now = reinterpret_cast<const uint8_t*>(&s);
        for (int i = 0; i < 4; i++) {
            SendHeader(s.textures[size_t(i)]);
            SendHeader(s.palettes[size_t(i)]);
        }
        for (const auto& st : s.streams) {
            if (!st.vb) continue;
            if (kernel::R32(st.vb) & 0x80000000u)
                SendUpHeader(st.vb, vertices * st.stride);
            else
                SendHeader(st.vb);
        }
        SendHeader(s.rt.header);
        SendHeader(s.zs.header);
        FlushHeaders();
        SendPeek(s);
        Cmd c{Op::State};
        size_t first = cur_->bytes.size();
        const uint8_t* sent = reinterpret_cast<const uint8_t*>(&sent_);
        for (const Range& r : ranges_) {
            size_t off = r.off, end = r.off + r.len;
            for (; off + kPiece <= end; off += kPiece)
                if (std::memcmp(sent + off, now + off, kPiece)) Delta(sent + off, now + off, kPiece);
            if (off < end) Delta(sent + off, now + off, end - off);
        }
        if (s.vs.get() != sent_.vs.get()) {
            sent_.vs = s.vs;
            c.vs = int32_t(cur_->vs.size());
            cur_->vs.push_back(s.vs);
        }
        if (s.ps.get() != sent_.ps.get()) {
            sent_.ps = s.ps;
            c.ps = int32_t(cur_->ps.size());
            cur_->ps.push_back(s.ps);
        }
        if (cur_->bytes.size() == first && c.vs < 0 && c.ps < 0) return;
        c.data = uint32_t(first);
        c.size = uint32_t(cur_->bytes.size() - first);
        cur_->cmds.push_back(c);
    }

    // the batch handed over (cap_ held): drawn at once, or queued for the render thread with the title's DMA_PUT
    // as it is now -- every call whose push buffer commands are before it is in this batch or an earlier one (a call
    // is recorded before its Direct3D code writes them)
    uint64_t Submit() {
        uint64_t id = cur_->id;
        if (cur_->cmds.empty() && thread_) return id - 1;
        if (!thread_) {
            const auto* was = g_header_view;
            g_header_view = &headers_;
            ExecuteBatch(*cur_);
            g_header_view = was;
            Recycle(cur_);
            cur_ = NewBatch();
            return id;
        }
        cur_->put = nv2a_pfifo_put();
        if (g_pace_on) cur_->handed_us = PaceNowUs();
        submitted_ = id;
        {
            std::lock_guard lock(m_);
            queue_.push_back(cur_);
        }
        cv_.notify_one();
        cur_ = NewBatch();
        return id;
    }

    // Nothing the title waits on held back (cap_ held): what it has recorded handed over; with nothing recorded or
    // still to draw, the push buffer read as far as the title has put -- every call is recorded before its Direct3D
    // code writes its commands, so whatever is there now belongs to calls already drawn, or to none.
    void Release() {
        if (!cur_->cmds.empty()) {
            Submit();
        } else if (done_.load() >= submitted_) {
            Gate(1, nv2a_pfifo_put());
        }
    }

    // the title waits for a batch drawn (recording), holding no lock (FrameWait). It keeps the virtual CPU
    // (platform/vcpu.h) meanwhile, as before 0.9.40: giving it to the title's other threads (NGB_D3D_WAIT_RELEASE=1,
    // tried for the sound skipping with the drawing behind, #126) made play stutter and answer late, the title
    // waiting for the CPU back every frame. A wait that goes on for kStallLogSeconds is logged, then every so
    // often: a frozen game's log says which frame the render thread is held on.
    void Wait(uint64_t id, bool hand_over = true) {
        if (!id || done_.load() >= id) return;
        if (hand_over && id > submitted_.load()) {   // the batch being recorded: handed over
            std::lock_guard lock(cap_);
            id = Submit();
        }
#if XBOX_HAS_VCPU
        static const bool release = std::getenv("NGB_D3D_WAIT_RELEASE") && *std::getenv("NGB_D3D_WAIT_RELEASE") == '1';
        if (release) vcpu_block_begin();
#endif
        {
            std::unique_lock lock(m_);
            auto since = std::chrono::steady_clock::now();
            int logged = 0;
            while (!done_cv_.wait_for(lock, std::chrono::seconds(1), [&] { return done_.load() >= id; })) {
                int s = int(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - since).count());
                if (s >= kStallLogSeconds * (logged + 1)) {
                    logged++;
                    std::fprintf(stderr, "[D3D] frame %llu not drawn after %d s: the render thread is held up (drawn %llu)\n",
                                 (unsigned long long)id, s, (unsigned long long)done_.load());
                }
            }
        }
#if XBOX_HAS_VCPU
        if (release) vcpu_block_end();
#endif
    }

    void Recycle(Batch* b) {
        b->cmds.clear();
        b->bytes.clear();
        b->vs.clear();
        b->ps.clear();
        b->quick.clear();
        std::lock_guard lock(m_);
        if (free_.size() >= kFreeBatches) {   // a backlog's worth kept, not the peak's for good
            delete b;
            return;
        }
        free_.push_back(b);
    }

    void Run() {
        g_header_view = &headers_;
        for (;;) {
            Batch* b = nullptr;
            {
                std::unique_lock lock(m_);
                if (!cv_.wait_for(lock, kIdleTake, [&] { return stop_ || !queue_.empty(); })) {
                    // nothing handed over for a while: what the title has recorded may be what it waits on
                    lock.unlock();
                    if (g_pace_on) g_pace.idle++;
                    if (cap_.try_lock()) {
                        if (g_pace_on && !cur_->cmds.empty()) g_pace.idle_submit++;
                        Release();
                        cap_.unlock();
                    } else if (g_pace_on) {
                        g_pace.idle_busy++;
                    }
                    continue;
                }
                if (queue_.empty()) break;
                b = queue_.front();
                queue_.pop_front();
            }
            uint64_t t0 = g_pace_on ? PaceNowUs() : 0;
            ExecuteBatch(*b);
            if (g_pace_on) {
                uint64_t t1 = PaceNowUs(), lat = b->handed_us ? t1 - b->handed_us : 0;
                g_pace.batches++;
                g_pace.exec_us += t1 - t0;
                g_pace.lat_us += lat;
                if (lat > g_pace.lat_max) g_pace.lat_max = lat;
                static uint64_t last = t1;
                if (t1 - last >= 2000000) {
                    last = t1;
                    uint32_t n = g_pace.batches.exchange(0);
                    std::fprintf(stderr, "[PACE] render: %u batches, drawing %.0f ms, handed over to drawn avg %.2f max "
                                 "%.2f ms; title waits %u (lock busy %u, handed %u, gate %u); idle takes %u (lock busy "
                                 "%u, handed %u)\n", n, g_pace.exec_us.exchange(0) / 1000.0,
                                 n ? g_pace.lat_us.exchange(0) / 1000.0 / n : 0, g_pace.lat_max.exchange(0) / 1000.0,
                                 g_pace.hook.exchange(0), g_pace.hook_busy.exchange(0), g_pace.hook_submit.exchange(0),
                                 g_pace.hook_gate.exchange(0), g_pace.idle.exchange(0), g_pace.idle_busy.exchange(0),
                                 g_pace.idle_submit.exchange(0));
                }
            }
            uint64_t id = b->id;
            uint32_t put = b->put;
            Recycle(b);
            Gate(1, put);   // the GPU emulation reads as far as has been drawn
            {
                std::lock_guard lock(m_);
                done_.store(id);
            }
            done_cv_.notify_all();
        }
        std::lock_guard lock(m_);
        exited_ = true;
        done_cv_.notify_all();
    }

    void ExecuteBatch(Batch& b) {
        for (const Cmd& c : b.cmds) Execute(b, c);
        for (uint32_t a : hosted_) headers_[a].host = nullptr;   // (its bytes go with the batch)
        hosted_.clear();
    }

    void Execute(Batch& b, const Cmd& c) {
        const uint8_t* data = b.bytes.data() + c.data;
        switch (c.op) {
            case Op::State: {
                uint8_t* s = reinterpret_cast<uint8_t*>(&state_);
                for (const uint8_t* p = data; p < data + c.size;) {
                    uint32_t head[2];
                    std::memcpy(head, p, 8);
                    std::memcpy(s + head[0], p + 8, head[1]);
                    p += 8 + head[1];
                }
                if (c.vs >= 0) state_.vs = b.vs[size_t(c.vs)];
                if (c.ps >= 0) state_.ps = b.ps[size_t(c.ps)];
                break;
            }
            case Op::Headers: {
                for (uint32_t i = 0; i < c.size; i++) {
                    HeaderSend h;
                    std::memcpy(&h, data + i * sizeof h, sizeof h);
                    HeaderCopy& to = headers_[h.addr];
                    std::memcpy(to.w, h.w, sizeof to.w);
                    to.host = h.data >= 0 ? b.bytes.data() + h.data : nullptr;
                    if (to.host) hosted_.push_back(h.addr);
                }
                break;
            }
            case Op::Peek: {
                Peek p;
                std::memcpy(&p, data, sizeof p);
                inner_->UsePeek(p);
                break;
            }
            case Op::Draw: inner_->Draw(state_, c.a, c.b, c.c, c.d ? data : nullptr); break;
            case Op::Clear: inner_->Clear(state_, c.a, c.size ? data : nullptr, c.b, c.c, c.f, c.d); break;
            case Op::Copy:
                inner_->Copy(c.s1, c.s2, c.a, c.b ? b.bytes.data() + c.b - 1 : nullptr,
                             c.c ? b.bytes.data() + c.c - 1 : nullptr);
                break;
            case Op::Present:
                inner_->UseQuick(b.quick[size_t(c.quick)]);
                inner_->Present(c.s1);
                break;
            case Op::Flush: inner_->Flush(); break;
            case Op::Gamma: inner_->SetGamma(data); break;
            case Op::VisBegin: inner_->VisibilityBegin(); break;
            case Op::VisEnd: inner_->VisibilityEnd(c.a); break;
        }
    }

    std::unique_ptr<Backend> inner_;
    std::vector<Range> ranges_;
    const bool thread_;

    // the title's side (cap_: a call being recorded; the render thread takes the batch under it too)
    std::recursive_mutex cap_;
    Batch* cur_ = nullptr;
    std::thread::id title_, render_;
    State sent_;   // the state as the renderer will have it
    std::unordered_map<uint32_t, HeaderCopy> sent_headers_;
    std::vector<HeaderSend> pending_headers_;
    Peek sent_peek_;
    bool peek_sent_ = false;
    uint64_t next_id_ = 0, last_present_ = 0, frame_wait_ = 0;
    std::atomic<uint64_t> submitted_{0};   // the last batch handed over

    // the renderer's side
    State state_;
    std::unordered_map<uint32_t, HeaderCopy> headers_;
    std::vector<uint32_t> hosted_;   // headers whose data is in the batch being drawn: let go with it

    std::mutex m_;
    std::condition_variable cv_, done_cv_;
    std::deque<Batch*> queue_;
    std::vector<Batch*> free_;
    std::atomic<uint64_t> done_{0};
    bool stop_ = false, exited_ = false;
};

}  // namespace

std::unique_ptr<Backend> CreateThreadedBackend(std::unique_ptr<Backend> inner) {
    const char* e = std::getenv("NGB_D3D_THREAD");
    bool thread = !(e && !std::strcmp(e, "capture"));
    return std::make_unique<Recorded>(std::move(inner), thread);
}

}  // namespace gpu
