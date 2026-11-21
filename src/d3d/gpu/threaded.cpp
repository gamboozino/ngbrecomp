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
#include <array>
#include <atomic>
#include <bitset>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <mutex>
#include <thread>
#include <tuple>

#include "platform/vcpu.h"   // the title's wait for a frame gives the virtual CPU away meanwhile
#include <unordered_map>
#include <vector>

#include "gpu/backend.h"
#include "gpu/headers.h"
#include "kernel/kernel.h"

extern "C" {
void nv2a_pfifo_gate(int on, uint32_t put);   // xboxrecomp xemu/pfifo.c
uint32_t nv2a_pfifo_put(void);
int64_t nv2a_present_now_us(void);   // xboxrecomp present_sdl.c
int nv2a_present_refresh_hz(void);
void nv2a_present_flush_held(void);
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
        if (interp_) inner_->SetHold(true);
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
            Drawn(cur_);
            g_header_view = was;
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
            Drawn(b);
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

    // Frame interpolation (NGB_INTERP): the frame's batches kept as drawn (the tape), with the renderer's state and
    // headers as the frame began, so that its commands can be drawn again -- the renderer in its replay (SetReplay):
    // nothing written back to the title. Step 1: each frame drawn a second time just after it, as it was.
    void Drawn(Batch* b) {
        if (!interp_) {
            Recycle(b);
            if (presented_) {   // a frame's end: switched on (g_interp, the setting) from the next frame
                presented_ = false;
                if (g_interp.load(std::memory_order_relaxed) != 0) {
                    interp_ = true;
                    inner_->SetHold(true);
                    inner_->ReplaySkipped();
                    prev_draws_.clear();
                    cur_draws_.clear();
        frame_targets_.clear();
        cur_vb_.clear();
                    tape_state_ = state_;
                    tape_headers_ = headers_;
                    tape_peek_ = last_peek_;
                    std::fprintf(stderr, "[INTERP] on\n");
                }
            }
            return;
        }
        tape_.push_back(b);
        if (!presented_ && tape_.size() > kTapeMost) {   // long without a present (loading): no replay of it
            for (Batch* t : tape_) Recycle(t);
            tape_.clear();
            cur_draws_.clear();
        frame_targets_.clear();
        cur_vb_.clear();
            tape_broken_ = true;
        }
        if (!presented_) return;
        // shown in turn: the replay's picture (between the last frame and this one) now, this frame's half the
        // time between the game's frames later
        int64_t now = nv2a_present_now_us();
        if (last_frame_us_) {
            double dt = double(now - last_frame_us_);
            if (dt > 4000 && dt < 100000) frame_us_ += (dt - frame_us_) * 0.1;
        }
        last_frame_us_ = now;
        // pictures a frame, by the setting and the display (as NG2's): Full Rate one each refresh, Half Rate each
        // second one (each one below 120 Hz), 2x two; four at most. NGB_INTERP_ANY_HZ=1: 2x's two on any display.
        int hz = nv2a_present_refresh_hz();
        int k = Pictures(g_interp.load(std::memory_order_relaxed), hz);
        if (k != pictures_) {
            pictures_ = k;
            inner_->Pictures(k);
            std::fprintf(stderr, "[INTERP] the display at %d Hz: %d pictures a frame\n", hz, k);
        }
        bool whole = !tape_broken_;
        tape_broken_ = false;
        if (Match() && k > 1 && whole) {
            // the first between now, the others and the frame's own each 1/k of the frame later
            int64_t base = 0, step = int64_t(frame_us_ / k);
            for (int i = 1; i < k; i++) {
                interp_t_ = fixed_t_ >= 0 ? fixed_t_ : float(i) / float(k);
                inner_->ReplayShowAt(i == 1 ? 0 : base + step * (i - 1));
                inner_->ReplayIndex(i);
                Replay();
                if (i == 1) base = nv2a_present_now_us();
            }
            inner_->ReplayShowAt(0);
            inner_->ShowHeld(base + step * (k - 1));
        } else {
            inner_->ShowHeld(0);
            inner_->ReplaySkipped();
        }
        presented_ = false;   // (the replay's own present)
        for (Batch* t : tape_) Recycle(t);
        tape_.clear();
        tape_state_ = state_;      // the next frame begins from here
        tape_headers_ = headers_;
        tape_peek_ = last_peek_;
        prev_draws_.swap(cur_draws_);
        prev_vb_.swap(cur_vb_);
        cur_vb_.clear();
        cur_draws_.clear();
        frame_targets_.clear();
        cur_vb_.clear();
        if (g_interp.load(std::memory_order_relaxed) == 0) {   // switched off: the next frame shown as drawn
            interp_ = false;
            inner_->SetHold(false);
            inner_->ReplaySkipped();
            prev_draws_.clear();
            std::fprintf(stderr, "[INTERP] off\n");
        }
    }
    void Replay() {
        State now = state_;
        std::unordered_map<uint32_t, HeaderCopy> heads = headers_;
        state_ = tape_state_;
        headers_ = tape_headers_;
        replaying_ = true;
        replay_draw_ = 0;
        lerped_vb_.clear();   // (made for this replay's t)
        inner_->SetReplay(true);
        if (have_peek_) inner_->UsePeek(tape_peek_);   // (sent on a change only: the one the frame began with)
        for (Batch* b : tape_) ExecuteBatch(*b);
        if (have_peek_) inner_->UsePeek(last_peek_);
        inner_->SetReplay(false);
        replaying_ = false;
        state_ = now;
        headers_ = std::move(heads);
    }

    // Step 2: each draw paired with the last frame's draw of the same things (shaders, textures, vertices, range),
    // the n-th of its kind with the n-th; the replay draws it with its matrices and shader constants part of the way
    // from the last frame's (interp_t_).
    struct InterpDraw {
        uint64_t key;
        bool vs;
        std::array<std::array<float, 16>, 6> m;   // view, projection, world 0-3
        std::array<std::array<float, 4>, 192> c;
        std::array<std::array<float, 16>, 4> tm;   // texture matrices 16-19 (a scrolling stream)
        uint32_t tfactor;                          // the texture factor (a puff of mist fading)
        uint16_t slot;                             // its target among the frame's (frame_targets_)
        uint8_t vblend;                            // fixed function: vertices blended between worlds (D3DRS_VERTEXBLEND)
        uint64_t key2;                             // the key without the vertex buffer (a mesh moved on the CPU)
        uint64_t key3;                             // the key without the target's slot
        uint64_t key4;                             // its look alone: shaders, textures, target (another mesh of it)
        uint64_t key5;                             // the key without the textures (a texture animated by turns)
        uint64_t key6;                             // ... nor the vertex buffer (one look drawn from several)
        const std::bitset<192>* reads;             // a vertex shader's: the constants its program reads (Reads)
        uint32_t vb, fvf;                          // stream 0's buffer, the vertex format
        bool copied;                               // its vertices kept (cur_vb_)
        bool divides;                              // a vertex shader's program divides by a vertex's value (Divides)
        uint32_t dbg[18];                          // (NGB_INTERP_LOG) what its key was made of
        uint64_t vhash;   // samples of its vertices, as drawn
    };
    // samples of the vertices a draw reads (from guest memory, but for DrawVerticesUP's copy): the replay comes
    // after the title may have written them again
    uint64_t VertexHash(uint32_t start, uint32_t count, const uint8_t* idx) const {
        uint32_t lo = start, hi = start + count;
        if (idx) {
            lo = 0xFFFFFFFFu;
            hi = 0;
            for (uint32_t i = 0; i < count; i++) {
                uint16_t v;
                std::memcpy(&v, idx + i * 2, 2);
                lo = std::min<uint32_t>(lo, v);
                hi = std::max<uint32_t>(hi, v + 1u);
            }
            if (hi <= lo) return 0;
            lo += state_.base_vertex;
            hi += state_.base_vertex;
        }
        uint64_t h = 0x9E3779B97F4A7C15ull;
        for (const auto& st : state_.streams) {
            if (!st.vb || !st.stride) continue;
            const uint8_t* p = ResourceHost(st.vb);
            if (!p) continue;
            size_t a = size_t(lo) * st.stride, n = size_t(hi - lo) * st.stride;
            if (n < 8) continue;
            for (int k = 0; k < 32; k++) {
                uint64_t w;
                std::memcpy(&w, p + a + (n - 8) * size_t(k) / 31, 8);
                h = (h ^ w) * 0x100000001b3ull;
            }
        }
        return h;
    }
    static constexpr uint16_t kLerpMatrices[6] = {2, 3, 256, 257, 258, 259};
    // A matrix part of the way (t) from a to b. A world or view matrix (rows: its axes, scaled, then its place)
    // turned along the arc, its scale and place lerped: lerped number by number, one turned far in a frame
    // (Ryu's limbs in a flip) shrank toward nothing, half-way through a half turn to a point (and its lighting,
    // by the inverse, blew up white). kind: 0 number by number (a projection), 1 a world (row 3 its place), 2 a
    // view (row 3 = -place x turn: the camera's place lerped and the row made again from it). A mirror is turned
    // with a row flipped and back; a sheared matrix, or one with no size, number by number.
    static void LerpMatrix(const std::array<float, 16>& a, const std::array<float, 16>& b, float t, int kind,
                           std::array<float, 16>& out) {
        auto plain = [&] {
            for (size_t e = 0; e < 16; e++) out[e] = a[e] == b[e] ? b[e] : a[e] + (b[e] - a[e]) * t;
        };
        if (!kind || a == b) return plain();
        bool same3 = true;   // (only the place moved: number by number is exact for a world)
        for (size_t r = 0; r < 3; r++)
            for (size_t c = 0; c < 3; c++) same3 = same3 && a[r * 4 + c] == b[r * 4 + c];
        if (same3 && kind == 1) return plain();
        float ra[9], rb[9], sa[3], sb[3];
        for (int r = 0; r < 3; r++) {
            sa[r] = std::sqrt(a[size_t(r * 4)] * a[size_t(r * 4)] + a[size_t(r * 4 + 1)] * a[size_t(r * 4 + 1)] +
                              a[size_t(r * 4 + 2)] * a[size_t(r * 4 + 2)]);
            sb[r] = std::sqrt(b[size_t(r * 4)] * b[size_t(r * 4)] + b[size_t(r * 4 + 1)] * b[size_t(r * 4 + 1)] +
                              b[size_t(r * 4 + 2)] * b[size_t(r * 4 + 2)]);
            if (sa[r] < 1e-6f || sb[r] < 1e-6f) return plain();
            for (int c = 0; c < 3; c++) {
                ra[r * 3 + c] = a[size_t(r * 4 + c)] / sa[r];
                rb[r * 3 + c] = b[size_t(r * 4 + c)] / sb[r];
            }
        }
        auto det = [](const float* m) {
            return m[0] * (m[4] * m[8] - m[5] * m[7]) - m[1] * (m[3] * m[8] - m[5] * m[6]) + m[2] * (m[3] * m[7] - m[4] * m[6]);
        };
        for (const float* m : {ra, rb})   // rows at right angles, else sheared: no turn to take apart
            for (int i = 0; i < 3; i++)
                for (int j = i + 1; j < 3; j++)
                    if (std::fabs(m[i * 3] * m[j * 3] + m[i * 3 + 1] * m[j * 3 + 1] + m[i * 3 + 2] * m[j * 3 + 2]) > 1e-2f)
                        return plain();
        float da = det(ra), db = det(rb);
        if ((da < 0) != (db < 0)) return plain();   // (mirrored on one side only)
        if (da < 0) {   // the third row flipped into a turn, the scale's sign carrying it back
            for (int c = 0; c < 3; c++) {
                ra[6 + c] = -ra[6 + c];
                rb[6 + c] = -rb[6 + c];
            }
            sa[2] = -sa[2];
            sb[2] = -sb[2];
        }
        auto quat = [](const float* m, float q[4]) {   // w x y z
            float tr = m[0] + m[4] + m[8];
            if (tr > 0) {
                float k = std::sqrt(tr + 1.0f) * 2;
                q[0] = 0.25f * k; q[1] = (m[7] - m[5]) / k; q[2] = (m[2] - m[6]) / k; q[3] = (m[3] - m[1]) / k;
            } else if (m[0] > m[4] && m[0] > m[8]) {
                float k = std::sqrt(1.0f + m[0] - m[4] - m[8]) * 2;
                q[0] = (m[7] - m[5]) / k; q[1] = 0.25f * k; q[2] = (m[1] + m[3]) / k; q[3] = (m[2] + m[6]) / k;
            } else if (m[4] > m[8]) {
                float k = std::sqrt(1.0f + m[4] - m[0] - m[8]) * 2;
                q[0] = (m[2] - m[6]) / k; q[1] = (m[1] + m[3]) / k; q[2] = 0.25f * k; q[3] = (m[5] + m[7]) / k;
            } else {
                float k = std::sqrt(1.0f + m[8] - m[0] - m[4]) * 2;
                q[0] = (m[3] - m[1]) / k; q[1] = (m[2] + m[6]) / k; q[2] = (m[5] + m[7]) / k; q[3] = 0.25f * k;
            }
        };
        float qa[4], qb[4], q[4];
        quat(ra, qa);
        quat(rb, qb);
        float d = qa[0] * qb[0] + qa[1] * qb[1] + qa[2] * qb[2] + qa[3] * qb[3];
        if (d < 0) {   // the short way round
            d = -d;
            for (float& v : qb) v = -v;
        }
        // A world turned less than 60 degrees: number by number after all. Lerped, a point two pieces share in both
        // frames (a shoulder's seam) stays shared; turned along their arcs, the pieces parted by a sliver and the
        // seam showed through (#226). Past 60 degrees the lerp shrinks the piece (to nothing at a half turn, and
        // its lighting, by the inverse, white): the turn then. (The view, one for every draw, always turned.)
        if (kind == 1 && d > 0.8660254f) return plain();
        if (kind == 3) kind = 1;   // (a test: a world always turned)
        float wa = 1 - t, wb = t;
        if (d < 0.9995f) {
            float th = std::acos(std::min(d, 1.0f)), sn = std::sin(th);
            wa = std::sin((1 - t) * th) / sn;
            wb = std::sin(t * th) / sn;
        }
        float n = 0;
        for (int k = 0; k < 4; k++) {
            q[k] = qa[k] * wa + qb[k] * wb;
            n += q[k] * q[k];
        }
        n = 1.0f / std::sqrt(n);
        float w = q[0] * n, x = q[1] * n, y = q[2] * n, z = q[3] * n;
        const float m[9] = {1 - 2 * (y * y + z * z), 2 * (x * y - w * z),     2 * (x * z + w * y),
                            2 * (x * y + w * z),     1 - 2 * (x * x + z * z), 2 * (y * z - w * x),
                            2 * (x * z - w * y),     2 * (y * z + w * x),     1 - 2 * (x * x + y * y)};
        for (int r = 0; r < 3; r++) {
            float sc = sa[r] + (sb[r] - sa[r]) * t;
            for (int c = 0; c < 3; c++) out[size_t(r * 4 + c)] = m[r * 3 + c] * sc;
            out[size_t(r * 4 + 3)] = a[size_t(r * 4 + 3)] + (b[size_t(r * 4 + 3)] - a[size_t(r * 4 + 3)]) * t;
        }
        for (size_t e = 12; e < 16; e++) out[e] = a[e] + (b[e] - a[e]) * t;
        if (kind == 2) {   // row 3 = -p M (M its 3x3): p for each frame, lerped, the row made from it
            auto place = [](const std::array<float, 16>& v, float p[3]) {
                const float m[9] = {v[0], v[1], v[2], v[4], v[5], v[6], v[8], v[9], v[10]};
                float d = m[0] * (m[4] * m[8] - m[5] * m[7]) - m[1] * (m[3] * m[8] - m[5] * m[6]) +
                          m[2] * (m[3] * m[7] - m[4] * m[6]);
                if (std::fabs(d) < 1e-12f) return false;
                const float inv[9] = {(m[4] * m[8] - m[5] * m[7]) / d, (m[2] * m[7] - m[1] * m[8]) / d,
                                      (m[1] * m[5] - m[2] * m[4]) / d, (m[5] * m[6] - m[3] * m[8]) / d,
                                      (m[0] * m[8] - m[2] * m[6]) / d, (m[2] * m[3] - m[0] * m[5]) / d,
                                      (m[3] * m[7] - m[4] * m[6]) / d, (m[1] * m[6] - m[0] * m[7]) / d,
                                      (m[0] * m[4] - m[1] * m[3]) / d};
                for (int c = 0; c < 3; c++)   // p = -row3 M^-1
                    p[c] = -(v[12] * inv[c] + v[13] * inv[3 + c] + v[14] * inv[6 + c]);
                return true;
            };
            float pa[3], pb[3];
            if (place(a, pa) && place(b, pb))
                for (int c = 0; c < 3; c++) {
                    float p0 = pa[0] + (pb[0] - pa[0]) * t, p1 = pa[1] + (pb[1] - pa[1]) * t,
                          p2 = pa[2] + (pb[2] - pa[2]) * t;
                    out[size_t(12 + c)] = -(p0 * out[size_t(c)] + p1 * out[size_t(4 + c)] + p2 * out[size_t(8 + c)]);
                }
        }
    }
    // The constants a vertex shader's program reads (its instructions' constant operands; one read by index, the
    // bones of a skinned mesh, counts from its base to the last register), kept by the program's words.
    std::unordered_map<uint64_t, std::bitset<192>> reads_;
    const std::bitset<192>* Reads(const VertexShader& vs, uint64_t h) {
        auto it = reads_.find(h);
        if (it != reads_.end()) return &it->second;
        std::bitset<192> m, direct, bases;
        for (size_t i = 0; i + 4 <= vs.program.size(); i += 4) {
            const uint32_t* t = &vs.program[i];
            const uint32_t mux_a = (t[2] >> 26) & 3, mux_b = (t[2] >> 11) & 3, mux_c = (t[3] >> 28) & 3;
            if (mux_a != 3 && mux_b != 3 && mux_c != 3) continue;
            const uint32_t c = (t[1] >> 13) & 0xFF;
            if (c >= 192) continue;
            ((t[3] >> 1) & 1 ? bases : direct).set(c);
        }
        // one read by index: from its base up to the next register the program reads by number (what lies past a
        // skinned mesh's bones is another draw's leavings), or the last
        m = direct;
        for (uint32_t c = 0; c < 192; c++) {
            if (!bases.test(c)) continue;
            for (uint32_t r = c; r < 192 && (r == c || !direct.test(r)); r++) m.set(r);
        }
        if (m.none()) m.set();   // (a program not understood: all of them, as before)
        if (reads_.size() >= 4096) return nullptr;   // (never in play, a few dozen programs: then all, as before)
        return &reads_.emplace(h, m).first->second;
    }
    // Whether a vertex shader's program takes a reciprocal (ILU RCP) of its own: it divides by a value of each vertex.
    // Zarkhan Falls' water does, for the screen texture its refraction samples: each vertex's place on the screen,
    // divided by its w. Its constants part of the way (two cameras' matrices number by number) can take the w of a
    // vertex beside the camera, behind it in one frame and in front in the next, through 0 -- its coordinates
    // thousands of screens wide, and the picture between showed the water as streaks and a flat patch (#259). Of the
    // game's programs only such effects use RCP (the position's own divide is the hardware's, RCC).
    std::unordered_map<uint64_t, bool> divides_;
    bool Divides(const VertexShader& vs, uint64_t h) {
        auto it = divides_.find(h);
        if (it != divides_.end()) return it->second;
        bool rcp = false;
        for (size_t i = 0; i + 4 <= vs.program.size() && !rcp; i += 4)
            rcp = ((vs.program[i + 1] >> 25) & 7) == 2;
        if (divides_.size() < 4096) divides_.emplace(h, rcp);
        if (rcp && interp_log_)
            std::fprintf(stderr, "[INTERP] program %016llx divides by a vertex's value: its constants as now\n",
                         (unsigned long long)h);
        return rcp;
    }
    // How far apart two draws of one kind are: what tells several of a kind apart (six pots of one mesh, three
    // enemies of one model) when each is paired with the last frame's nearest. Only by what the draw itself uses:
    // a vertex shader's constants its program reads, a fixed-function draw's worlds, as many as it blends.
    // Compared whole (192 registers, four worlds), the registers and worlds left by whatever was drawn before
    // counted too, and more than the things' own places: when the title drew the pots in another order (it sorts
    // them by distance, so whenever the camera moves in a room full of them), a pot was paired with another pot's
    // last place and drawn part of the way between the two -- pots and enemies jumping back and forth in the
    // pictures between.
    static float Distance(const InterpDraw& a, const InterpDraw& b) {
        float d = 0;
        if (a.vs) {
            const std::bitset<192>* reads = a.reads;
            for (size_t r = 0; r < 192; r++) {
                if (reads && !reads->test(r)) continue;
                for (size_t e = 0; e < 4; e++) d += std::fabs(a.c[r][e] - b.c[r][e]);
            }
        } else {
            // (D3DVERTEXBLENDFLAGS, 1..6: odd, n weights for n + 1 worlds; even, a weight a world)
            const uint32_t v = a.vblend;
            const size_t worlds = v >= 1 && v <= 6 ? size_t(v & 1 ? (v + 3) / 2 : v / 2 + 1) : 1;
            for (size_t w = 2; w < 2 + worlds; w++)
                for (size_t e = 0; e < 16; e++) d += std::fabs(a.m[w][e] - b.m[w][e]);
        }
        return d < 3e38f ? d : 3e38f;   // (NaN or inf in a register: the farthest, never unordered)
    }
    // What a draw looks like, not which vertices: a pot drawn from its near mesh in one frame and its far one in the
    // next (the title's level of detail, as the camera moves) is the same thing
    // The shaders by what they are, once a draw (its keys and Reads all take it): vs the vertex program alone
    struct ShaderHash { uint64_t vs, both; };
    static ShaderHash HashShaders(const State& s) {
        uint64_t h = 0xcbf29ce484222325ull;
        auto mix = [&](uint64_t v) { h = (h ^ v) * 0x100000001b3ull; };
        if (s.vs) {
            mix(s.vs->program_type);
            for (uint32_t w : s.vs->program) mix(w);
        }
        ShaderHash out{h, 0};
        if (s.ps)
            for (uint32_t w : s.ps->def) mix(w);
        out.both = h;
        return out;
    }
    static uint64_t LookKey(const State& s, uint32_t prim, uint64_t shaders) {
        uint64_t h = 0x84222325cbf29ce4ull;
        auto mix = [&](uint64_t v) { h = (h ^ v) * 0x100000001b3ull; };
        mix(s.vs_handle);
        mix(shaders);
        for (uint32_t t : s.textures) mix(t);
        mix(prim);
        mix(s.rt.format);
        return h;
    }
    static uint64_t DrawKey(const State& s, uint32_t prim, uint32_t start, uint32_t count, uint64_t shaders,
                            bool with_vb = true, bool with_textures = true) {
        uint64_t h = 0xcbf29ce484222325ull;
        auto mix = [&](uint64_t v) { h = (h ^ v) * 0x100000001b3ull; };
        mix(s.vs_handle);
        // the shaders by what they are (HashShaders), not where: the title selects Ryu's vertex shader anew for
        // each piece, a new object each time, and by address a piece found its pair only when the allocator gave
        // the same place again: some of his pieces lerped, their neighbours not, and the seams between them opened
        // (#226)
        mix(shaders);
        if (with_textures)
            for (uint32_t t : s.textures) mix(t);
        mix(with_vb ? s.streams[0].vb : 0x56420000u);
        mix(s.streams[0].stride);
        mix(s.index_buffer);
        mix(s.base_vertex);
        mix(prim);
        mix(start);
        mix(count);
        mix(s.rt.format);
        return h;
    }
    // A mesh the title moves on the CPU (Ryu's cloth and the like: four objects, 20 draws) writes its vertices each
    // frame, into two buffers in turn. By the buffer's address its draws never found a pair, and their shape is in
    // the vertices, not the matrices: drawn as they are now beside pieces half a frame behind, one layer showed
    // through the other at the seams (#226). So: such a draw's vertices are kept each frame (a fixed-function draw
    // from a buffer made at run time, not one loaded with a file), draws left without a pair are paired again by
    // key2 (Match), and the replay draws them from vertices part of the way (LerpedVertices).
    struct VbCopy {
        uint32_t lo = 0, hi = 0, stride = 0;   // vertices [lo, hi)
        std::vector<uint8_t> bytes;
    };
    bool KeepVertices(uint32_t start, uint32_t count, const uint8_t* idx) {
        const auto& st = state_.streams[0];
        if ((state_.vs_handle & 1) || !st.vb || (st.vb & 0x80000000u) || st.stride < 12 || st.stride > 96) return false;
        {   // (not a DrawVerticesUP's vertices: every one of those comes through one header, its data with the call)
            auto h = headers_.find(st.vb);
            if (h != headers_.end() && h->second.host) return false;
        }
        uint32_t lo = start, hi = start + count;
        if (idx) {
            lo = 0xFFFFFFFFu;
            hi = 0;
            for (uint32_t i = 0; i < count; i++) {
                uint16_t v;
                std::memcpy(&v, idx + i * 2, 2);
                lo = std::min<uint32_t>(lo, v);
                hi = std::max<uint32_t>(hi, v + 1u);
            }
            if (hi <= lo) return false;
            lo += state_.base_vertex;
            hi += state_.base_vertex;
        }
        if (hi - lo > 65536) return false;
        VbCopy& c = cur_vb_[st.vb];
        if (!c.bytes.empty() && c.stride != st.stride) return false;   // (read two ways in a frame: left alone)
        if (!c.bytes.empty() && c.lo <= lo && c.hi >= hi) return true;
        const uint8_t* p = ResourceHost(st.vb);
        if (!p) return false;
        if (!c.bytes.empty() && c.stride == st.stride) {   // (another of its draws read other vertices: both)
            lo = std::min(lo, c.lo);
            hi = std::max(hi, c.hi);
        }
        c.lo = lo;
        c.hi = hi;
        c.stride = st.stride;
        c.bytes.assign(p + size_t(lo) * st.stride, p + size_t(hi) * st.stride);
        return true;
    }
    // The buffer for a replayed draw of such a mesh: this frame's vertices, their places and normals part of the way
    // from the last frame's (as if the buffer began at its vertex 0); null when it can't be made.
    const uint8_t* LerpedVertices(const InterpDraw& was, const InterpDraw& now, float t) {
        auto done = lerped_vb_.find(now.vb);
        if (done != lerped_vb_.end()) return done->second.empty() ? nullptr : done->second.data();
        std::vector<uint8_t>& out = lerped_vb_[now.vb];
        auto c = cur_vb_.find(now.vb);
        auto p = prev_vb_.find(was.vb);
        if (c == cur_vb_.end() || p == prev_vb_.end() || c->second.stride != p->second.stride) return nullptr;
        const VbCopy& cc = c->second;
        const VbCopy& pc = p->second;
        // D3DFVF: a plain place (XYZ) then, with NORMAL, the normal; else only what the matrices do
        size_t floats = (now.fvf & 0x00E) == 0x002 ? ((now.fvf & 0x010) ? 6 : 3) : 0;
        if (floats * 4 > cc.stride) floats = 0;
        out.assign(size_t(cc.hi) * cc.stride, 0);
        std::memcpy(out.data() + size_t(cc.lo) * cc.stride, cc.bytes.data(), cc.bytes.size());
        uint32_t lo = std::max(cc.lo, pc.lo), hi = std::min(cc.hi, pc.hi);
        for (uint32_t v = lo; v < hi && floats; v++) {
            const uint8_t* a = pc.bytes.data() + size_t(v - pc.lo) * pc.stride;
            uint8_t* b = out.data() + size_t(v) * cc.stride;
            float fa[6], fb[6];
            std::memcpy(fa, a, floats * 4);
            std::memcpy(fb, b, floats * 4);
            for (size_t f = 0; f < floats; f++) fb[f] = fa[f] + (fb[f] - fa[f]) * t;
            std::memcpy(b, fb, floats * 4);
        }
        return out.data();
    }
    void NoteDraw(uint32_t prim, uint32_t start, uint32_t count, const uint8_t* idx) {
        InterpDraw& d = cur_draws_.emplace_back();
        d.vhash = 0;
        if (interp_log_) {   // (a check, measured: none changed in play, combat included)
            d.vhash = VertexHash(start, count, idx);
            if (d.vhash != 0x9E3779B97F4A7C15ull && d.vhash) vhashed_++;
        }
        // Which of the frame's targets it draws into, by their order of first use (the back buffers' addresses turn
        // each frame): Ryu is drawn twice with all else the same, into a second buffer as he was the frame before
        // and into the picture; paired by nearness alone, that first pass took the last frame's second (the same
        // pose exactly), and the picture's own was left the one before: his body half a frame behind his sword.
        const uint64_t target = uint64_t(state_.rt.data) << 32 | state_.zs.data;
        size_t slot = 0;
        while (slot < frame_targets_.size() && frame_targets_[slot] != target) slot++;
        if (slot == frame_targets_.size()) frame_targets_.push_back(target);
        d.slot = uint16_t(slot);
        d.vblend = uint8_t(state_.rs[kRsVertexBlend]);
        const ShaderHash sh = HashShaders(state_);
        d.key2 = (DrawKey(state_, prim, start, count, sh.both, false) ^ (slot + 1)) * 0x100000001b3ull;
        d.key3 = DrawKey(state_, prim, start, count, sh.both);
        d.key4 = (LookKey(state_, prim, sh.both) ^ (slot + 1)) * 0x100000001b3ull;
        d.key5 = (DrawKey(state_, prim, start, count, sh.both, true, false) ^ (slot + 1)) * 0x100000001b3ull;
        d.key6 = (DrawKey(state_, prim, start, count, sh.both, false, false) ^ (slot + 1)) * 0x100000001b3ull;
        d.vb = state_.streams[0].vb;
        d.fvf = state_.vs_handle;
        d.copied = KeepVertices(start, count, idx);
        if (interp_log_) {
            uint32_t vh = 0, ph = 0;
            if (state_.vs) for (uint32_t w : state_.vs->program) vh = vh * 31 + w;
            if (state_.ps) for (uint32_t w : state_.ps->def) ph = ph * 31 + w;
            const uint32_t v[18] = {prim, start, count, state_.textures[0], state_.textures[1], state_.textures[2],
                                    state_.textures[3], state_.streams[0].vb, state_.index_buffer, state_.base_vertex,
                                    state_.rs[kRsVertexBlend], state_.vs_handle, state_.streams[0].stride, vh, ph,
                                    state_.rt.format, uint32_t(slot), state_.vs ? state_.vs->program_type : 0};
            std::memcpy(d.dbg, v, sizeof v);
        }
        d.key = (dbg_ & 64) ? d.key3   // (a test: as before, the targets not told apart)
                            : (d.key3 ^ (slot + 1)) * 0x100000001b3ull;
        d.vs = (state_.vs_handle & 1) != 0;
        d.reads = d.vs && state_.vs ? Reads(*state_.vs, sh.vs) : nullptr;
        d.divides = d.vs && state_.vs && Divides(*state_.vs, sh.vs);
        for (int i = 0; i < 6; i++) d.m[size_t(i)] = state_.transforms[kLerpMatrices[i]];
        d.c = state_.vsc;
        for (size_t t = 0; t < 4; t++) d.tm[t] = state_.transforms[16 + t];
        d.tfactor = state_.rs[kRsTextureFactor];
    }
    // The camera from a 3D draw's view matrix (D3D's, rows): where it is and the way it looks.
    static void Camera(const InterpDraw& d, float pos[3], float fwd[3]) {
        const auto& v = d.m[0];
        for (int i = 0; i < 3; i++) {
            pos[i] = -(v[12] * v[size_t(i * 4)] + v[13] * v[size_t(i * 4 + 1)] + v[14] * v[size_t(i * 4 + 2)]);
            fwd[i] = v[size_t(i * 4 + 2)];
        }
    }
    static bool Perspective(const InterpDraw& d) { return d.m[1][11] == 1.0f && d.m[1][15] == 0.0f; }
    // Where a fixed-function draw's object (its world's origin) is on the screen, -1..1 across; false for one under
    // half a unit in front of the camera (its place swings with the least move) or far off the screen
    static bool OnScreen(const InterpDraw& d, float out[2]) {
        const auto& w = d.m[2];
        const auto& v = d.m[0];
        const auto& p = d.m[1];
        float e[4], c[4];
        for (int j = 0; j < 4; j++)
            e[j] = w[12] * v[size_t(j)] + w[13] * v[size_t(4 + j)] + w[14] * v[size_t(8 + j)] + v[size_t(12 + j)];
        for (int j = 0; j < 4; j++)
            c[j] = e[0] * p[size_t(j)] + e[1] * p[size_t(4 + j)] + e[2] * p[size_t(8 + j)] + e[3] * p[size_t(12 + j)];
        if (!(e[2] > 0.5f) || !(c[3] > 0.01f)) return false;
        out[0] = c[0] / c[3];
        out[1] = c[1] / c[3];
        return std::fabs(out[0]) < 1.5f && std::fabs(out[1]) < 1.5f;
    }
    // Two fixed-function draws stand the same way (each world they blend turned under 25 degrees or so from the
    // other's). A pair made by less than the whole key -- another mesh of the same look, another texture -- is one
    // thing only then: the Great Bridge portal's wall is segments of one look, each its own mesh turned about the
    // middle, and as the camera moved one segment came into view in the frame another left; paired, the new one was
    // drawn part of the way between the two turns, across the arena, and the wall flickered (#259).
    static bool StandAlike(const InterpDraw& a, const InterpDraw& b) {
        if (a.vs || b.vs) return true;
        const uint32_t v = a.vblend;
        const size_t worlds = v >= 1 && v <= 6 ? size_t(v & 1 ? (v + 3) / 2 : v / 2 + 1) : 1;
        for (size_t w = 2; w < 2 + worlds; w++) {
            float dot = 0, la = 0, lb = 0;
            for (size_t e = 0; e < 11; e++) {
                if (e % 4 == 3) continue;
                dot += a.m[w][e] * b.m[w][e];
                la += a.m[w][e] * a.m[w][e];
                lb += b.m[w][e] * b.m[w][e];
            }
            if (la > 0 && lb > 0 && dot < 0.9f * std::sqrt(la * lb)) return false;
            // ... or turned under that but over 18 degrees on the very spot: the portal's segments stand 30 degrees
            // apart about one middle (0.91 by this measure), and one was still paired with its neighbour now and
            // then, a ghost of the barrier for a picture. (A limb or a cloth swung that far in a frame moved too.)
            float mv = 0;
            for (size_t e = 12; e < 15; e++) mv += (a.m[w][e] - b.m[w][e]) * (a.m[w][e] - b.m[w][e]);
            if (la > 0 && lb > 0 && mv < 1e-6f && dot < 0.95f * std::sqrt(la * lb)) return false;
        }
        return true;
    }
    // One of the last frame's draws left free (a list of one kind, the next in order at its back) for this draw: the
    // nearest (Distance), the next in order when they tie. In order alone, two of a kind that the title drew the
    // other way round this frame -- the two halves of a piece of Ryu's costume, each blended to its own leg's bone
    // -- were each paired with the other's bone, and drawn part of the way between the two legs the piece was
    // gone from his hips for that picture (#259).
    int32_t TakeNearest(const InterpDraw& d, std::vector<int32_t>& free, bool other_vb = false) {
        // (other_vb: only one from another buffer -- with the same buffer it was left without a pair on purpose)
        auto ok = [&](size_t k) { return !other_vb || prev_draws_[size_t(free[k])].vb != d.vb; };
        size_t best = free.size();
        for (size_t k = free.size(); k-- > 0;)
            if (ok(k)) { best = k; break; }
        if (best == free.size()) return -1;
        if (free.size() > 1 && free.size() <= 64) {
            float least = Distance(d, prev_draws_[size_t(free[best])]);
            for (size_t k = best; k-- > 0;) {
                if (prev_draws_[size_t(free[k])].vs != d.vs || !ok(k)) continue;
                const float dist = Distance(d, prev_draws_[size_t(free[k])]);
                if (dist < least) least = dist, best = k;
            }
        }
        const int32_t j = free[best];
        if (!StandAlike(d, prev_draws_[size_t(j)])) return -1;   // (another of its kind, not this one a frame ago)
        free.erase(free.begin() + std::ptrdiff_t(best));
        return j;
    }
    // false: no picture between this frame and the last (too little the same, or the camera cut)
    bool Match() {
        match_.assign(cur_draws_.size(), -1);
        how_.assign(cur_draws_.size(), 0);
        // Several of one kind (the shards of a broken pot, one mesh): each with the nearest of the last frame's, by
        // its place (fixed function) or its constants (a vertex shader), not the n-th with the n-th: drawn in
        // another order, a shard was lerped toward another's place.
        std::unordered_map<uint64_t, std::vector<int32_t>> prev_by, cur_by;
        for (int32_t i = 0; i < int32_t(prev_draws_.size()); i++) prev_by[prev_draws_[size_t(i)].key].push_back(i);
        uint32_t matched = 0, ff = 0, vs = 0;
        for (int32_t i = 0; i < int32_t(cur_draws_.size()); i++) {
            cur_by[cur_draws_[size_t(i)].key].push_back(i);
            if (cur_draws_[size_t(i)].vs) vs++; else ff++;
        }
        auto distance = [&](int32_t c, int32_t p) { return Distance(cur_draws_[size_t(c)], prev_draws_[size_t(p)]); };
        // (a frame's budget of floats compared: past it, a group in order; a vertex shader pair is 768, a
        // fixed-function one 64)
        int64_t budget = 1500000;
        std::vector<std::tuple<float, int32_t, int32_t>> pairs;
        std::vector<uint8_t> used;
        for (auto& [key, cs] : cur_by) {
            auto pit = prev_by.find(key);
            if (pit == prev_by.end()) continue;
            const std::vector<int32_t>& ps = pit->second;
            if (cs.size() == 1 && ps.size() == 1) {
                match_[size_t(cs[0])] = ps[0];
                matched++;
                continue;
            }
            const bool vs_group = cur_draws_[size_t(cs[0])].vs;
            int64_t cost = int64_t(cs.size() * ps.size()) * (vs_group ? 768 : 64);
            if (cs.size() * ps.size() > 4096 || cost > budget) {
                // Many (the puffs of a waterfall's mist, born and gone each frame): fixed-function ones each with the
                // last frame's nearest by place where that one's nearest is it too, the others drawn as they are
                // (paired in order, unrelated puffs were lerped toward each other). Vertex shader ones as they are.
                if (vs_group || cs.size() * ps.size() > 262144) {   // (in order, as before)
                    for (size_t n = 0; n < cs.size() && n < ps.size(); n++) {
                        match_[size_t(cs[n])] = ps[n], how_[size_t(cs[n])] = 1;
                        matched++;
                    }
                    continue;
                }
                auto place = [](const InterpDraw& d, float p[3]) {
                    for (size_t e = 0; e < 3; e++) p[e] = d.m[2][12 + e];
                };
                std::vector<int32_t> near_c(cs.size(), -1), near_p(ps.size(), -1);
                std::vector<float> best_c(cs.size(), 3e38f), best_p(ps.size(), 3e38f);
                std::vector<std::array<float, 3>> pp(ps.size());
                for (size_t y = 0; y < ps.size(); y++) place(prev_draws_[size_t(ps[y])], pp[y].data());
                for (size_t x = 0; x < cs.size(); x++) {
                    float c3[3];
                    place(cur_draws_[size_t(cs[x])], c3);
                    for (size_t y = 0; y < ps.size(); y++) {
                        float d = (c3[0] - pp[y][0]) * (c3[0] - pp[y][0]) + (c3[1] - pp[y][1]) * (c3[1] - pp[y][1]) +
                                  (c3[2] - pp[y][2]) * (c3[2] - pp[y][2]);
                        if (!(d < 3e38f)) d = 3e38f;
                        if (d < best_c[x]) best_c[x] = d, near_c[x] = int32_t(y);
                        if (d < best_p[y]) best_p[y] = d, near_p[y] = int32_t(x);
                    }
                }
                // ... and only for a move under half the way to its nearest neighbour this frame (else it may be
                // another puff where this one was)
                std::vector<std::array<float, 3>> cp(cs.size());
                for (size_t x = 0; x < cs.size(); x++) place(cur_draws_[size_t(cs[x])], cp[x].data());
                for (size_t x = 0; x < cs.size(); x++) {
                    if (near_c[x] < 0 || near_p[size_t(near_c[x])] != int32_t(x)) continue;
                    float gap = 3e38f;
                    for (size_t z = 0; z < cs.size(); z++) {
                        if (z == x) continue;
                        float d = (cp[x][0] - cp[z][0]) * (cp[x][0] - cp[z][0]) + (cp[x][1] - cp[z][1]) * (cp[x][1] - cp[z][1]) +
                                  (cp[x][2] - cp[z][2]) * (cp[x][2] - cp[z][2]);
                        if (d < gap) gap = d;
                    }
                    if (gap < 3e38f && best_c[x] > gap * 0.25f) near_c[x] = -1;   // (squared: half the way)
                }
                for (size_t x = 0; x < cs.size(); x++)
                    if (near_c[x] >= 0 && near_p[size_t(near_c[x])] == int32_t(x)) {
                        match_[size_t(cs[x])] = ps[size_t(near_c[x])], how_[size_t(cs[x])] = 2;
                        matched++;
                    }
                continue;
            }
            budget -= cost;
            pairs.clear();
            for (size_t x = 0; x < cs.size(); x++)
                for (size_t y = 0; y < ps.size(); y++)   // (a tie: the one in the same order)
                    pairs.emplace_back(distance(cs[x], ps[y]) + (x == y ? 0.0f : 1e-6f), int32_t(x), int32_t(y));
            std::sort(pairs.begin(), pairs.end());
            used.assign(cs.size() + ps.size(), 0);
            for (auto& [d, x, y] : pairs) {
                if (used[size_t(x)] || used[cs.size() + size_t(y)]) continue;
                used[size_t(x)] = used[cs.size() + size_t(y)] = 1;
                match_[size_t(cs[size_t(x)])] = ps[size_t(y)];
                matched++;
            }
        }
        // Several of one look whose textures are flip-books at their own pages (the flames on the imps' heads in
        // Chapter 15: layers of one quad, each from its own buffer, any of them on any imp from frame to frame):
        // by the whole key one imp's flame now was paired with another's a frame ago, at the page and layer this
        // one has now, and drawn half the way between the two, a flame in the air between them (#259). Where draws
        // differ only by texture or buffer (more than one key to one key6), the pairs are made again among them all
        // by the nearest.
        if (!(dbg_ & 2048) && !(dbg_ & 64)) {   // (64: whole keys without the slot, the groups not closed)
            std::unordered_map<uint64_t, std::vector<int32_t>> prev6, cur6;
            for (int32_t j = 0; j < int32_t(prev_draws_.size()); j++) prev6[prev_draws_[size_t(j)].key6].push_back(j);
            for (int32_t i = 0; i < int32_t(cur_draws_.size()); i++) cur6[cur_draws_[size_t(i)].key6].push_back(i);
            for (auto& [k6, cs] : cur6) {
                auto pit = prev6.find(k6);
                if (pit == prev6.end()) continue;
                const std::vector<int32_t>& ps = pit->second;
                if (cs.size() + ps.size() < 3 || cs.size() * ps.size() > 1024) continue;
                bool pages = false;   // (more than one key among them)
                const uint64_t k0 = cur_draws_[size_t(cs[0])].key;
                for (int32_t i : cs) pages |= cur_draws_[size_t(i)].key != k0;
                for (int32_t j : ps) pages |= prev_draws_[size_t(j)].key != k0;
                if (!pages) continue;
                {   // (out of the frame's budget, as the whole key's groups are: only for a group worked on, or one of
                    // many pots, its key6 group its key's, was paid for twice and the passes after went short)
                    const int64_t cost = int64_t(cs.size() * ps.size()) * (cur_draws_[size_t(cs[0])].vs ? 768 : 64);
                    if (cost > budget) continue;
                    budget -= cost;
                }
                // (a pair a pass before made by the whole key is in here, both ends of it: the group is closed)
                std::vector<int32_t> was(cs.size());
                for (size_t x = 0; x < cs.size(); x++) {
                    was[x] = match_[size_t(cs[x])];
                    if (was[x] >= 0) match_[size_t(cs[x])] = -1, matched--;
                }
                pairs.clear();
                for (size_t x = 0; x < cs.size(); x++)
                    for (size_t y = 0; y < ps.size(); y++)
                        pairs.emplace_back(distance(cs[x], ps[y]) + (x == y ? 0.0f : 1e-6f), int32_t(x), int32_t(y));
                std::sort(pairs.begin(), pairs.end());
                used.assign(cs.size() + ps.size(), 0);
                for (auto& [d, x, y] : pairs) {
                    if (used[size_t(x)] || used[cs.size() + size_t(y)]) continue;
                    const InterpDraw& c = cur_draws_[size_t(cs[size_t(x)])];
                    const InterpDraw& p = prev_draws_[size_t(ps[size_t(y)])];
                    if (c.key != p.key && !StandAlike(c, p)) continue;
                    if (c.copied && p.copied && c.vb != p.vb) continue;   // (a CPU-moved mesh's: the key2 pass's)
                    used[size_t(x)] = used[cs.size() + size_t(y)] = 1;
                    match_[size_t(cs[size_t(x)])] = ps[size_t(y)];
                    how_[size_t(cs[size_t(x)])] = c.key == p.key ? 0 : 5;
                    matched++;
                }
                // ... only where a pair by the whole key was such a mix-up: one of them a good deal farther than
                // the one it has now. Else (all of them still where they were: a draw of another texture new in
                // this frame as one of a third went) as they were, the new one not lerped from the one gone.
                bool mixed = false;
                for (size_t x = 0; x < cs.size() && !mixed; x++) {
                    const int32_t now = match_[size_t(cs[x])];
                    if (was[x] < 0 || now < 0 || now == was[x]) continue;
                    const float before = distance(cs[x], was[x]), after = distance(cs[x], now);
                    mixed = before > 1e-3f && after < before * 0.25f;
                }
                // ... and a draw new in this frame (no pair before) keeps its new one only when it is plainly its own:
                // nearer than a quarter of the way to any other of the group now not on the same spot, and drawn
                // by a fixed-function draw (a vertex shader one's constants say nothing of which mesh it is)
                if (mixed)
                    for (size_t x = 0; x < cs.size(); x++) {
                        const int32_t now = match_[size_t(cs[x])];
                        if (was[x] >= 0 || now < 0) continue;
                        const InterpDraw& c = cur_draws_[size_t(cs[x])];
                        bool own = !c.vs;
                        const float d = distance(cs[x], now);
                        for (size_t z = 0; z < cs.size() && own; z++) {
                            if (z == x) continue;
                            const float n = Distance(c, cur_draws_[size_t(cs[z])]);
                            if (n > 1e-3f && d * 4 >= n) own = false;
                        }
                        if (!own) match_[size_t(cs[x])] = -1, matched--;
                    }
                if (!mixed)
                    for (size_t x = 0; x < cs.size(); x++) {
                        if (match_[size_t(cs[x])] >= 0) matched--;
                        match_[size_t(cs[x])] = was[x], how_[size_t(cs[x])] = 0;
                        if (was[x] >= 0) matched++;
                    }
            }
        }
        // A target drawn into in this frame and not the last (or the other way) moved every later one's slot: the
        // draws left without a pair then by the key without it, in order
        if (frame_targets_.size() != prev_targets_) {
            std::vector<uint8_t> taken(prev_draws_.size(), 0);
            for (int32_t m : match_)
                if (m >= 0) taken[size_t(m)] = 1;
            std::unordered_map<uint64_t, std::vector<int32_t>> free_prev;
            for (int32_t j = int32_t(prev_draws_.size()) - 1; j >= 0; j--)
                if (!taken[size_t(j)]) free_prev[prev_draws_[size_t(j)].key3].push_back(j);
            for (size_t i = 0; i < cur_draws_.size(); i++) {
                if (match_[i] >= 0) continue;
                auto it = free_prev.find(cur_draws_[i].key3);
                if (it == free_prev.end() || it->second.empty()) continue;
                match_[i] = TakeNearest(cur_draws_[i], it->second), how_[i] = 3;
                if (match_[i] >= 0) matched++;
            }
        }
        prev_targets_ = frame_targets_.size();
        // the draws of CPU-moved meshes (KeepVertices) left without a pair: by key6, nearest first
        dyn_.assign(cur_draws_.size(), 0);
        {
            std::vector<uint8_t> taken(prev_draws_.size(), 0);
            for (int32_t m : match_)
                if (m >= 0) taken[size_t(m)] = 1;
            std::unordered_map<uint64_t, std::vector<int32_t>> free_prev;
            for (int32_t j = int32_t(prev_draws_.size()) - 1; j >= 0; j--)
                if (!taken[size_t(j)] && prev_draws_[size_t(j)].copied) free_prev[prev_draws_[size_t(j)].key6].push_back(j);
            // By group -- the key without the buffer nor the textures (key6): the barrier's ripples in the Underworld-like
            // room (#259) take one of two ripple maps in turn, each the other's the frame after -- and the nearest pairs
            // first (a pair's distance as TakeNearest's), not each in its turn with the nearest left. A ripple new this
            // frame took the place of the one that was there, which was left without a pair; each showed the scene
            // through it from the other's place, a copy of Ryu and the chains beside them. (Large groups, a puff of
            // many: in turn, as before.)
            std::unordered_map<uint64_t, std::vector<int32_t>> cur_dyn;
            for (size_t i = 0; i < cur_draws_.size(); i++)
                if (match_[i] < 0 && cur_draws_[i].copied && free_prev.count(cur_draws_[i].key6))
                    cur_dyn[cur_draws_[i].key6].push_back(int32_t(i));
            struct Cand { float d; int32_t i, j; };
            std::vector<Cand> cand;
            std::vector<uint8_t> used(prev_draws_.size(), 0), done(cur_draws_.size(), 0);   // (groups share nothing)
            for (auto& [k6, cs] : cur_dyn) {
                std::vector<int32_t>& ps = free_prev[k6];
                if (cs.size() * ps.size() > 1024) {
                    for (int32_t i : cs) {
                        // (only the buffers' turn: with the same buffer it was left without a pair on purpose, a puff of many)
                        match_[size_t(i)] = TakeNearest(cur_draws_[size_t(i)], ps, true), how_[size_t(i)] = 4;
                        if (match_[size_t(i)] < 0) continue;
                        dyn_[size_t(i)] = 1;
                        matched++;
                    }
                    continue;
                }
                cand.clear();
                for (int32_t i : cs)
                    for (int32_t j : ps) {
                        const InterpDraw &c = cur_draws_[size_t(i)], &p = prev_draws_[size_t(j)];
                        if (p.vb == c.vb || p.vs != c.vs) continue;   // (as TakeNearest's)
                        cand.push_back({Distance(c, p), i, j});
                    }
                // (ties as TakeNearest broke them: the earlier draws first, each with the earliest of the last frame's --
                // pieces built in world space all stand at the world's middle, every distance 0)
                std::sort(cand.begin(), cand.end(), [](const Cand& x, const Cand& y) {
                    return x.d != y.d ? x.d < y.d : x.i != y.i ? x.i < y.i : x.j < y.j;
                });
                for (const Cand& c : cand) {
                    if (match_[size_t(c.i)] >= 0 || done[size_t(c.i)] || used[size_t(c.j)]) continue;
                    done[size_t(c.i)] = 1;   // (its nearest left: that one or none, as TakeNearest's StandAlike)
                    if (!StandAlike(cur_draws_[size_t(c.i)], prev_draws_[size_t(c.j)])) continue;
                    match_[size_t(c.i)] = c.j, how_[size_t(c.i)] = 4;
                    used[size_t(c.j)] = 1;
                    dyn_[size_t(c.i)] = 1;
                    matched++;
                }
            }
        }
        // A mesh whose texture is a flip-book (Chapter 16's lava, a frame of its animation every third frame): by its
        // key it found no pair in the frames it turned a page, and drawn as it is now beside the rest part of the
        // way, the lava's surface covered the rock in it in those pictures between and let it through in the others
        // (#259). Paired by its key without the textures, when it is the only one so on both sides.
        if (!(dbg_ & 1024)) {
            std::vector<uint8_t> taken(prev_draws_.size(), 0);
            for (int32_t m : match_)
                if (m >= 0) taken[size_t(m)] = 1;
            std::unordered_map<uint64_t, std::pair<int32_t, int32_t>> free_prev, free_cur;   // (one, how many)
            for (int32_t j = 0; j < int32_t(prev_draws_.size()); j++)
                if (!taken[size_t(j)] && !cur_by.count(prev_draws_[size_t(j)].key)) {   // (its key gone: a page turned)
                    auto& e = free_prev.try_emplace(prev_draws_[size_t(j)].key5, j, 0).first->second;
                    e.second++;
                }
            for (int32_t i = 0; i < int32_t(cur_draws_.size()); i++)
                if (match_[size_t(i)] < 0 && !prev_by.count(cur_draws_[size_t(i)].key)) {   // (not one Match left out)
                    auto& e = free_cur.try_emplace(cur_draws_[size_t(i)].key5, i, 0).first->second;
                    e.second++;
                }
            for (auto& [key, c] : free_cur) {
                auto it = free_prev.find(key);
                if (c.second != 1 || it == free_prev.end() || it->second.second != 1) continue;
                if (!StandAlike(cur_draws_[size_t(c.first)], prev_draws_[size_t(it->second.first)])) continue;
                match_[size_t(c.first)] = it->second.first, how_[size_t(c.first)] = 5;
                matched++;
            }
        }
        // Last, a draw whose mesh changed between the frames (the title's level of detail: a pot's near mesh, then its
        // far one, as the camera moves away): left without a pair it was drawn where it is now while all around it
        // was half a frame behind, and jumped against the room in every picture between. Paired by its look with
        // the last frame's draw of the same look left without one, each the other's nearest (by its constants or
        // matrices, as Match does), so its place is part of the way too; the replay draws the new mesh.
        lod_pairs_ = 0;
        {
            std::vector<uint8_t> taken(prev_draws_.size(), 0);
            for (int32_t m : match_)
                if (m >= 0) taken[size_t(m)] = 1;
            // only draws whose whole key is on one side alone: a mesh that changed (not one of many alike, which
            // Match leaves without a pair on purpose)
            std::unordered_map<uint64_t, std::vector<int32_t>> free_prev, free_cur, all_cur;
            for (int32_t j = 0; j < int32_t(prev_draws_.size()); j++)
                if (!taken[size_t(j)] && !cur_by.count(prev_draws_[size_t(j)].key))
                    free_prev[prev_draws_[size_t(j)].key4].push_back(j);
            bool any = false;
            for (int32_t i = 0; i < int32_t(cur_draws_.size()); i++)
                if (match_[size_t(i)] < 0 && !prev_by.count(cur_draws_[size_t(i)].key)) {
                    const uint64_t k4 = cur_draws_[size_t(i)].key4;
                    if (!free_prev.count(k4)) continue;
                    free_cur[k4].push_back(i);
                    any = true;
                }
            // How far a thing moves between these two frames: among the draws paired by their whole key, vertex
            // shader and fixed function apart, the distance nine in ten stay within (the camera's turn is in it; not
            // the most, which an unrelated pair Match made in order would set). A pair by look further apart than
            // three times that is two things (one gone, another of its kind come), not one with a new mesh. Measured
            // only in a frame with such a draw.
            float reach[2] = {0, 0};
            if (any) {
                std::vector<float> seen[2];
                int64_t cost = 0;
                for (size_t i = 0; i < cur_draws_.size() && cost < budget; i++) {
                    const int32_t m = match_[i];
                    if (m < 0 || cur_draws_[i].key != prev_draws_[size_t(m)].key) continue;
                    const InterpDraw& a = cur_draws_[i];
                    seen[a.vs].push_back(Distance(a, prev_draws_[size_t(m)]));
                    cost += a.vs ? 768 : 64;
                }
                budget -= cost;
                for (int v = 0; v < 2; v++)
                    if (!seen[v].empty()) {
                        const size_t at = seen[v].size() * 9 / 10;
                        std::nth_element(seen[v].begin(), seen[v].begin() + std::ptrdiff_t(at), seen[v].end());
                        reach[v] = seen[v][at];
                    }
                for (int32_t i = 0; i < int32_t(cur_draws_.size()); i++)
                    if (free_cur.count(cur_draws_[size_t(i)].key4)) all_cur[cur_draws_[size_t(i)].key4].push_back(i);
            }
            for (auto& [key, cs] : free_cur) {
                auto it = free_prev.find(key);
                if (it == free_prev.end()) continue;
                const std::vector<int32_t>& ps = it->second;
                const bool vs = cur_draws_[size_t(cs[0])].vs;
                const int64_t group = int64_t(cs.size() * ps.size() + cs.size() * all_cur[key].size()) * (vs ? 768 : 64);
                if (reach[vs] <= 0 || cs.size() * ps.size() > 1024 || group > budget) continue;
                budget -= group;
                std::vector<int32_t> near_c(cs.size(), -1), near_p(ps.size(), -1);
                std::vector<float> best_c(cs.size(), 3e38f), best_p(ps.size(), 3e38f);
                for (size_t x = 0; x < cs.size(); x++)
                    for (size_t y = 0; y < ps.size(); y++) {
                        const InterpDraw& a = cur_draws_[size_t(cs[x])];
                        const InterpDraw& b = prev_draws_[size_t(ps[y])];
                        if (a.vs != b.vs) continue;
                        float d = Distance(a, b);
                        if (d < best_c[x]) best_c[x] = d, near_c[x] = int32_t(y);
                        if (d < best_p[y]) best_p[y] = d, near_p[y] = int32_t(x);
                    }
                for (size_t x = 0; x < cs.size(); x++) {
                    if (near_c[x] < 0 || near_p[size_t(near_c[x])] != int32_t(x)) continue;
                    if (best_c[x] > reach[vs] * 3.0f) continue;   // (two things, not one)
                    // ... and under half the way to the nearest other draw of its look this frame (Match's rule)
                    float gap = 3e38f;
                    for (int32_t z : all_cur[key])
                        if (z != cs[x]) gap = std::min(gap, Distance(cur_draws_[size_t(cs[x])], cur_draws_[size_t(z)]));
                    if (best_c[x] > gap * 0.5f) continue;
                    if (!StandAlike(cur_draws_[size_t(cs[x])], prev_draws_[size_t(ps[size_t(near_c[x])])])) continue;
                    match_[size_t(cs[x])] = ps[size_t(near_c[x])], how_[size_t(cs[x])] = 6;
                    lod_pairs_++;
                }
            }
        }
        if (interp_log_ && interp_frames_ % 60 == 59) {   // (NGB_INTERP_LOG) the draws without a pair
            uint32_t un_vs = 0, un_ff = 0;
            for (size_t i = 0; i < cur_draws_.size(); i++)
                if (match_[i] < 0) (cur_draws_[i].vs ? un_vs : un_ff)++;
            for (size_t i = 0; i < cur_draws_.size(); i++)
                if (match_[i] < 0) {
                    const uint32_t* v = cur_draws_[i].dbg;
                    std::fprintf(stderr, "[INTERP]   f%u %zu %s n %u tex %X %X %X %X vb %X ib %X base %u fvf %X stride %u vsh %X psh %X rtf %X slot %u type %X start %u\n",
                                 interp_frames_, i, cur_draws_[i].vs ? "VS" : "FF", v[2], v[3], v[4], v[5], v[6],
                                 v[7], v[8], v[9], v[11], v[12], v[13], v[14], v[15], v[16], v[17], v[1]);
                }
            std::fprintf(stderr, "[INTERP] unpaired: %u VS, %u FF (%u paired by look); VS draws:", un_vs, un_ff, lod_pairs_);
            for (size_t i = 0; i < cur_draws_.size(); i++)
                if (cur_draws_[i].vs) std::fprintf(stderr, " %zu%s", i, match_[i] < 0 ? "!" : "");
            std::fprintf(stderr, "\n");
        }
        if (interp_log_)
            for (size_t i = 0; i < cur_draws_.size(); i++)
                if (match_[i] >= 0 && prev_draws_[size_t(match_[i])].slot != cur_draws_[i].slot)
                    (cur_draws_[i].vs ? crossed_vs_ : crossed_ff_)++;
        // a matched fixed-function draw whose world jumped (another of the same mesh, a thing put elsewhere): its
        // world as it is now, the camera still part of the way. (Vertex shader draws aren't checked: their
        // registers hold what other shaders left too, and their matrices move with the camera.)
        uint32_t jumped = 0;
        snap_.assign(cur_draws_.size(), 0);
        for (size_t i = 0; i < cur_draws_.size(); i++) {
            if (match_[i] < 0 || cur_draws_[i].vs) continue;
            const auto& a = prev_draws_[size_t(match_[i])].m[2];
            const auto& b = cur_draws_[i].m[2];
            float d2 = 0;
            for (size_t e = 12; e < 15; e++) d2 += (b[e] - a[e]) * (b[e] - a[e]);
            if (d2 > cut_move_ * cut_move_) {
                snap_[i] = 1;
                jumped++;
            }
        }
        // (NGB_INTERP_LOG=2) each frame: the draws paired by less than the whole key, and the 3D ones left without
        // a pair (a sprite's four vertices aside), with how far their worlds stand turned and moved
        static const bool pairlog = [] { const char* e = std::getenv("NGB_INTERP_LOG"); return e && *e == '2'; }();
        if (pairlog)
            for (size_t i = 0; i < cur_draws_.size(); i++) {
                const InterpDraw& a = cur_draws_[i];
                if (a.dbg[2] <= 6 || (match_[i] >= 0 && !how_[i])) continue;
                float cosw = 1, mv = 0;
                if (match_[i] >= 0 && !a.vs) {
                    const auto& p = prev_draws_[size_t(match_[i])].m[2];
                    const auto& q = a.m[2];
                    float dot = 0, la = 0, lb = 0;
                    for (size_t e = 0; e < 11; e++) {
                        if (e % 4 == 3) continue;
                        dot += p[e] * q[e];
                        la += p[e] * p[e];
                        lb += q[e] * q[e];
                    }
                    cosw = la > 0 && lb > 0 ? dot / std::sqrt(la * lb) : 1;
                    for (size_t e = 12; e < 15; e++) mv += (q[e] - p[e]) * (q[e] - p[e]);
                }
                std::fprintf(stderr, "[PAIR] f%u d%zu %s n %u tex %X vsh %X psh %X %s how %u cos %.3f moved %.3f snap %d\n",
                             interp_frames_, i, a.vs ? "VS" : "FF", a.dbg[2], a.dbg[3], a.dbg[13], a.dbg[14],
                             match_[i] < 0 ? "UNPAIRED" : "paired", unsigned(how_[i]), cosw, std::sqrt(mv), int(snap_.size() > i && snap_[i]));
            }
        // (NGB_INTERP_LOG) matched draws whose world matrices shrink when lerped halfway: a big turn in a frame
        uint32_t shrunk = 0, ffw = 0;
        if (interp_log_)
            for (size_t i = 0; i < cur_draws_.size(); i++) {
                if (match_[i] < 0 || cur_draws_[i].vs) continue;
                ffw++;
                const InterpDraw& a = prev_draws_[size_t(match_[i])];
                const InterpDraw& b = cur_draws_[i];
                for (int w = 2; w < 6; w++) {
                    float worst = 1;
                    std::array<float, 16> h;
                    LerpMatrix(a.m[size_t(w)], b.m[size_t(w)], 0.5f, 1, h);
                    for (int r = 0; r < 3; r++) {
                        float n0 = 0, nh = 0;
                        for (int c = 0; c < 3; c++) {
                            float x = b.m[size_t(w)][size_t(r * 4 + c)], y = h[size_t(r * 4 + c)];
                            n0 += x * x;
                            nh += y * y;
                        }
                        if (n0 > 1e-8f) worst = std::min(worst, std::sqrt(nh / n0));
                    }
                    if (worst < 0.8f) {
                        shrunk++;
                        break;
                    }
                }
            }
        shrunk_total_ += shrunk;
        ffw_total_ += ffw;
        // The camera: the one most of the frame's matched 3D fixed-function draws are seen from, against the same
        // draw's last frame. (Not the first such draw's: the frame begins with a pass of its own -- the shadows',
        // from where the light is -- whose view stays as the camera cuts. Read from that one a cut was never seen,
        // the picture between came out part of the way between two cameras, and a screen that builds on its last
        // picture, "Now Loading" over the frozen game at a shop's door, kept that picture between for as long as
        // it stayed: the screen flickered between two views at 60 Hz, #259.) cams_: each camera's first paired
        // draw, also for the draws left without a pair (Replay).
        float move = 0, turn = 1;
        cam_was_ = cam_now_ = nullptr;
        cams_.clear();
        {
            std::unordered_map<uint64_t, uint32_t> count;
            uint32_t most = 0;
            int32_t at = -1;
            for (size_t i = 0; i < cur_draws_.size(); i++) {
                if (match_[i] < 0 || cur_draws_[i].vs) continue;
                const uint64_t h = CamHash(cur_draws_[i].m[0], cur_draws_[i].m[1]);
                const int32_t first = cams_.try_emplace(h, int32_t(i)).first->second;
                if (!Perspective(cur_draws_[i])) continue;
                const uint32_t n = ++count[h];
                if (n > most) most = n, at = first;
            }
            if (at >= 0) {
                float p0[3], f0[3], p1[3], f1[3];
                Camera(prev_draws_[size_t(match_[size_t(at)])], p0, f0);
                Camera(cur_draws_[size_t(at)], p1, f1);
                float n0 = 0, n1 = 0;
                for (int k = 0; k < 3; k++) {
                    move += (p1[k] - p0[k]) * (p1[k] - p0[k]);
                    n0 += f0[k] * f0[k];
                    n1 += f1[k] * f1[k];
                }
                move = std::sqrt(move);
                float dot = f0[0] * f1[0] + f0[1] * f1[1] + f0[2] * f1[2];
                turn = n0 > 0 && n1 > 0 ? dot / std::sqrt(n0 * n1) : 1;
                cam_was_ = &prev_draws_[size_t(match_[size_t(at)])];
                cam_now_ = &cur_draws_[size_t(at)];
            }
        }
        // ... and where the things on the screen went: a camera that snaps back from a wall (a small room full of
        // pots, the camera pushed in against Ryu and then free again) moves little and turns under 30 degrees, but
        // everything jumps across the screen; drawn part of the way, the picture between was a view the game never
        // had, and pots and enemies flickered in and out of it. A cut when the middle of the matched objects moved
        // over a quarter of the screen's width in the frame (a fast turn moves them a tenth or so).
        float jump = 0;
        {
            // each place once: a room's many pieces drawn with no world of their own are all its origin, one point
            // that would be the middle alone
            std::vector<float> moved;
            std::vector<std::array<float, 3>> seen;
            for (size_t i = 0; i < cur_draws_.size() && seen.size() < 512; i++) {
                if (match_[i] < 0 || cur_draws_[i].vs || !Perspective(cur_draws_[i])) continue;
                // (not one whose world jumped, nor one paired by less than its whole key: those pairs may be two
                // things, and a room of them would read as a cut every frame)
                if (snap_[i] || cur_draws_[i].key != prev_draws_[size_t(match_[i])].key) continue;
                const auto& w = cur_draws_[i].m[2];
                const std::array<float, 3> at = {w[12], w[13], w[14]};
                if (std::find(seen.begin(), seen.end(), at) != seen.end()) continue;
                seen.push_back(at);
                float a[2], b[2];
                if (!OnScreen(prev_draws_[size_t(match_[i])], a) || !OnScreen(cur_draws_[i], b)) continue;
                moved.push_back(std::sqrt((b[0] - a[0]) * (b[0] - a[0]) + (b[1] - a[1]) * (b[1] - a[1])));
            }
            if (moved.size() >= 8) {
                std::nth_element(moved.begin(), moved.begin() + std::ptrdiff_t(moved.size() / 2), moved.end());
                jump = moved[moved.size() / 2];
            }
        }
        bool cut = turn < kCutCos || move > cut_move_ || jump > cut_jump_;
        bool ok = !cut && matched * 2 >= cur_draws_.size() && !cur_draws_.empty();
        if (!ok) interp_skipped_++;
        if (move > move_max_) move_max_ = move;
        interp_frames_++;
        if (interp_log_ && (cut || interp_frames_ % 60 == 0)) {
            std::fprintf(stderr, "[INTERP] frame %u: %zu draws (%u FF, %u VS), %u matched to the last frame's %zu; "
                         "camera moved %.3f (most %.3f), turned %.1f deg, things moved %.2f%s; %u skipped, %.2f ms apart; %u vertices changed (of %u hashed), %u jumped; shrunk %llu of %llu; crossed %llu VS %llu FF\n",
                         interp_frames_, cur_draws_.size(), ff, vs, matched, prev_draws_.size(), move, move_max_,
                         std::acos(std::clamp(turn, -1.0f, 1.0f)) * 57.29578f, jump, cut ? ": CUT" : "", interp_skipped_,
                         frame_us_ / 1000.0, vchanged_, vhashed_, jumped, (unsigned long long)shrunk_total_,
                         (unsigned long long)ffw_total_, (unsigned long long)crossed_vs_, (unsigned long long)crossed_ff_);
            if (!cut) move_max_ = 0;
        }
        return ok;
    }
    // the replay's draw: its values part of the way from the matched draw's (put back after)
    void ReplayDraw(const Cmd& c, const uint8_t* data) {
        size_t i = replay_draw_++;
        if (interp_log_ && i < cur_draws_.size() && VertexHash(c.b, c.c, c.d ? data : nullptr) != cur_draws_[i].vhash)
            vchanged_++;
        int32_t j = i < match_.size() ? match_[i] : -1;
        if (interp_t_ >= 1.0f) {
            inner_->Draw(state_, c.a, c.b, c.c, c.d ? data : nullptr);
            return;
        }
        if (j < 0) {
            // no pair (new this frame, or one of many): as it is now, but seen from the camera part of the way when a
            // paired draw has its camera (as that one is drawn), else it slid against the world
            auto& view = state_.transforms[2];
            auto& proj = state_.transforms[3];
            auto cam = i < cur_draws_.size() && !cur_draws_[i].vs ? cams_.find(CamHash(view, proj)) : cams_.end();
            if (cam != cams_.end() && cur_draws_[size_t(cam->second)].m[0] == view &&
                cur_draws_[size_t(cam->second)].m[1] == proj) {
                const InterpDraw& now = cur_draws_[size_t(cam->second)];
                const InterpDraw& was = prev_draws_[size_t(match_[size_t(cam->second)])];
                const auto keep_v = view, keep_p = proj;
                LerpMatrix(was.m[0], now.m[0], interp_t_, (dbg_ & 2) ? 0 : 2, view);
                LerpMatrix(was.m[1], now.m[1], interp_t_, 0, proj);
                inner_->Draw(state_, c.a, c.b, c.c, c.d ? data : nullptr);
                view = keep_v;
                proj = keep_p;
                return;
            }
            inner_->Draw(state_, c.a, c.b, c.c, c.d ? data : nullptr);
            return;
        }
        const InterpDraw& was = prev_draws_[size_t(j)];
        const InterpDraw& now = cur_draws_[i];
        const float t = interp_t_;
        auto lerp = [t](float a, float b) { return a == b ? b : a + (b - a) * t; };
        // NGB_INTERP_DBG (tests): 1 vertex shader constants as now, 2 the view number by number, 4 worlds number
        // by number, 8 fixed-function draws' worlds as now, 16 texture matrices as now, 32 worlds always turned
        // (128 blended draws' worlds as now, 256 the others'; 512 CPU-moved meshes as now, 1024 no pairs by
        // the key without the textures, 2048 no flip-book regrouping, 4096 the texture factor as now,
        // 8192 a dividing program's constants part of the way too)
        const int matrices = (snap_[i] || (dbg_ & 8) || ((dbg_ & 128) && now.vblend) || ((dbg_ & 256) && !now.vblend)) ? 2 : 6;
        for (int k = 0; k < matrices; k++)
            LerpMatrix(was.m[size_t(k)], now.m[size_t(k)], t,
                       k == 0 ? ((dbg_ & 2) ? 0 : 2) : k == 1 ? 0 : (dbg_ & 4) ? 0 : (dbg_ & 32) ? 3 : 1,
                       state_.transforms[kLerpMatrices[k]]);
        // (a program dividing by a vertex's value: its constants as now, Divides)
        if (now.vs && !(dbg_ & 1) && (!now.divides || (dbg_ & 8192)))
            for (size_t r = 0; r < 192; r++)
                for (size_t e = 0; e < 4; e++) state_.vsc[r][e] = lerp(was.c[r][e], now.c[r][e]);
        // texture matrices. Only its place moved (a scroll, or a sprite sheet's next cell): the short way across a
        // wrap when the step is small, else as now (the next cell). Anything else (a shadow's projection, which
        // follows the camera: left as now, the shadow slid over the ground lerped under it) number by number.
        // Not for a snapped draw (its pair another thing).
        for (size_t m = 0; m < 4 && !snap_[i]; m++) {
            if (was.tm[m] == now.tm[m]) continue;
            bool place_only = true;
            const size_t at[4] = {8, 9, 12, 13};
            for (size_t e = 0; e < 16 && place_only; e++)
                if (e != 8 && e != 9 && e != 12 && e != 13 && was.tm[m][e] != now.tm[m][e]) place_only = false;
            auto& o = state_.transforms[16 + m];
            if (!place_only) {
                if (!(dbg_ & 16))
                    for (size_t e = 0; e < 16; e++) o[e] = lerp(was.tm[m][e], now.tm[m][e]);
                continue;
            }
            float d[4];
            bool scroll = true;
            for (int k = 0; k < 4; k++) {
                d[k] = now.tm[m][at[k]] - was.tm[m][at[k]];
                d[k] -= std::round(d[k]);   // (across a wrap)
                if (std::fabs(d[k]) > 0.2f) scroll = false;
                // (a step of the matrix's own scale: the next cell of a sheet of eight, not a scroll)
                float cell = std::fabs(now.tm[m][(k & 1) ? 5 : 0]);
                if (d[k] != 0 && cell < 0.999f && std::fabs(std::fabs(d[k]) - cell) < 1e-3f) scroll = false;
            }
            if (!scroll) continue;
            for (int k = 0; k < 4; k++) o[at[k]] = now.tm[m][at[k]] - d[k] * (1.0f - t);
        }
        // The texture factor part of the way, for a fade (a puff of mist); a jump of over a quarter in a frame as now:
        // a save statue's words brighten step by step to full, then the factor is 0x0F for one frame (drawn as
        // bright: the game's flash), and halfway between the two they were dim in that picture alone (#259)
        bool tf_step = was.tfactor != now.tfactor && !snap_[i] && !(dbg_ & 4096);
        for (int sh = 0; sh < 32 && tf_step; sh += 8)
            if (std::abs(int((was.tfactor >> sh) & 0xFF) - int((now.tfactor >> sh) & 0xFF)) > 64) tf_step = false;
        if (tf_step) {
            uint32_t v = 0;
            for (int sh = 0; sh < 32; sh += 8) {
                float a0 = float((was.tfactor >> sh) & 0xFF), b0 = float((now.tfactor >> sh) & 0xFF);
                v |= uint32_t(std::clamp(lerp(a0, b0) + 0.5f, 0.0f, 255.0f)) << sh;
            }
            state_.rs[kRsTextureFactor] = v;
        }
        // a CPU-moved mesh: its vertices part of the way too, shown to the renderer as the buffer's data (the
        // header's copy, as a DrawVerticesUP's) for this draw
        HeaderCopy* head = nullptr;
        const uint8_t* head_was = nullptr;
        if (dyn_[i] && !snap_[i] && !(dbg_ & 512)) {
            auto hit = headers_.find(now.vb);
            const uint8_t* v = hit != headers_.end() ? LerpedVertices(was, now, t) : nullptr;
            if (v) {
                head = &hit->second;
                head_was = head->host;
                head->host = v;
            }
        }
        inner_->Draw(state_, c.a, c.b, c.c, c.d ? data : nullptr);
        if (head) head->host = head_was;
        for (int k = 0; k < 6; k++) state_.transforms[kLerpMatrices[k]] = now.m[size_t(k)];
        if (now.vs) state_.vsc = now.c;
        for (size_t m = 0; m < 4; m++) state_.transforms[16 + m] = now.tm[m];
        state_.rs[kRsTextureFactor] = now.tfactor;
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
                if (!replaying_) {
                    last_peek_ = p;
                    have_peek_ = true;
                }
                break;
            }
            case Op::Draw:
                if (replaying_) {
                    ReplayDraw(c, data);
                    break;
                }
                if (interp_ && pictures_ != 1) NoteDraw(c.a, c.b, c.c, c.d ? data : nullptr);   // (none between: not noted)
                inner_->Draw(state_, c.a, c.b, c.c, c.d ? data : nullptr);
                break;
            case Op::Clear: inner_->Clear(state_, c.a, c.size ? data : nullptr, c.b, c.c, c.f, c.d); break;
            case Op::Copy:
                inner_->Copy(c.s1, c.s2, c.a, c.b ? b.bytes.data() + c.b - 1 : nullptr,
                             c.c ? b.bytes.data() + c.c - 1 : nullptr);
                break;
            case Op::Present:
                inner_->UseQuick(b.quick[size_t(c.quick)]);
                inner_->Present(c.s1);
                presented_ = true;
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
    bool interp_ = g_interp.load() != 0;   // (the render thread's: on or off at a frame's end only)
    std::vector<Batch*> tape_;
    State tape_state_;
    std::unordered_map<uint32_t, HeaderCopy> tape_headers_;
    bool presented_ = false;
    bool replaying_ = false;
    static constexpr size_t kTapeMost = 4096;   // batches a frame kept for its replay
    bool tape_broken_ = false;                  // some of this frame's dropped (kTapeMost)
    Peek last_peek_{}, tape_peek_{};
    bool have_peek_ = false;
    std::vector<InterpDraw> prev_draws_, cur_draws_;
    std::vector<int32_t> match_;   // cur_draws_'s draw in prev_draws_, or -1
    std::vector<uint8_t> how_;     // (NGB_INTERP_LOG) the pass that paired it: 0 the whole key, 1 in order, 2 by place, 3 key3, 4 key2, 5 key5, 6 look
    uint32_t lod_pairs_ = 0;       // (NGB_INTERP_LOG) draws paired by their look alone (Match's last pass)
    std::unordered_map<uint64_t, int32_t> cams_;   // a camera (view, projection) -> a paired draw of it
    static uint64_t CamHash(const std::array<float, 16>& v, const std::array<float, 16>& p) {
        uint64_t h = 0xcbf29ce484222325ull;
        for (const auto* m : {&v, &p})
            for (float f : *m) {
                uint32_t u;
                std::memcpy(&u, &f, 4);
                h = (h ^ u) * 0x100000001b3ull;
            }
        return h;
    }
    std::vector<uint8_t> snap_;    // its world jumped: not lerped
    std::vector<uint8_t> dyn_;     // a CPU-moved mesh's draw, paired by key2: its vertices lerped
    std::unordered_map<uint32_t, VbCopy> cur_vb_, prev_vb_;          // by vertex buffer: this frame's vertices, the last's
    std::unordered_map<uint32_t, std::vector<uint8_t>> lerped_vb_;   // by this frame's buffer: the replay's vertices
    std::vector<uint64_t> frame_targets_;   // this frame's colour and depth targets drawn into, in order of first use
    size_t prev_targets_ = 0;               // how many the last frame had
    const InterpDraw* cam_was_ = nullptr;   // the frame's camera (a matched 3D draw's), last frame and now
    const InterpDraw* cam_now_ = nullptr;
    size_t replay_draw_ = 0;
    float interp_t_ = 0.5f;   // the replay's: part of the way from the last frame
    const float fixed_t_ = [] {   // NGB_INTERP_T=t: every replay at t (tests: 1.0 draws the frame as it was)
        const char* v = std::getenv("NGB_INTERP_T");
        return v ? float(std::atof(v)) : -1.0f;
    }();
    const bool interp_log_ = std::getenv("NGB_INTERP_LOG") != nullptr;
    const int dbg_ = std::getenv("NGB_INTERP_DBG") ? std::atoi(std::getenv("NGB_INTERP_DBG")) : 0;
    static constexpr float kCutCos = 0.8660254f;   // a turn of over 30 degrees in a frame
    float cut_move_ = [] {   // the camera moving further in a frame (NGB_INTERP_CUT)
        const char* v = std::getenv("NGB_INTERP_CUT");
        return v ? float(std::atof(v)) : 3.0f;
    }();
    float cut_jump_ = [] {   // the middle of the things on the screen moving further in a frame, -1..1 across (NGB_INTERP_JUMP)
        const char* v = std::getenv("NGB_INTERP_JUMP");
        return v ? float(std::atof(v)) : 0.5f;
    }();
    float move_max_ = 0;
    uint32_t interp_skipped_ = 0;
    uint32_t vchanged_ = 0, vhashed_ = 0;
    uint64_t crossed_vs_ = 0, crossed_ff_ = 0;   // (NGB_INTERP_LOG) pairs across two of the frame's targets
    uint64_t shrunk_total_ = 0, ffw_total_ = 0;   // (NGB_INTERP_LOG) FF draws whose world shrinks lerped, of all   // replayed draws whose vertices had changed since (NGB_INTERP_LOG)
    int64_t last_frame_us_ = 0;
    const bool any_hz_ = std::getenv("NGB_INTERP_ANY_HZ") != nullptr;
    int pictures_ = -1;   // the last count logged
    int Pictures(int mode, int hz) const {
        if (any_hz_) return mode ? 2 : 1;
        // (whole refreshes a frame, rounded down: more pictures than refreshes held the presents up)
        if (mode <= 0 || hz < 118) return 1;   // (not known yet, or below 120 Hz: none between)
        int most = std::clamp(int(hz / 60.0 + 0.05), 1, 4);
        if (mode == 3) return std::min(2, most);
        int div = mode == 2 ? std::min(most, 2) : 1;
        return std::clamp(most / div, 1, 4);
    }
    double frame_us_ = 1e6 / 60;
    uint32_t interp_frames_ = 0;

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

std::atomic<int> g_interp{0};

std::unique_ptr<Backend> CreateThreadedBackend(std::unique_ptr<Backend> inner) {
    const char* e = std::getenv("NGB_D3D_THREAD");
    bool thread = !(e && !std::strcmp(e, "capture"));
    return std::make_unique<Recorded>(std::move(inner), thread);
}

}  // namespace gpu
