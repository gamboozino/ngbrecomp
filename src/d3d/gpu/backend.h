#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

#include "gpu/state.h"

namespace gpu {

// D3DCLEAR flags as the XDK defines them
enum : uint32_t {
    kClearTarget = 0xF0,  // ARGB channels
    kClearZ = 0x01,
    kClearStencil = 0x02,
};

// What the renderer takes from the game besides the Direct3D state, as of the call it draws for: a render thread
// draws later, when the game has moved on. Taken on the title's thread (Backend::CapturePeek), handed back before
// the call is drawn (Backend::UsePeek); a backend that draws at once takes it itself.
struct Peek {
    uint32_t draw_node = 0;    // the map piece being drawn (g_draw_node), 0 if none
    int32_t chunk_slot = -1;   // its chunk slot, -1 if none (debugging views)
    uint8_t towers = 0;        // stream 0's vertex buffer in the drawbridge towers' chunk (the section pull)
    uint8_t stretched = 0;     // the game's 2D in its widescreen mode (ngb_2d_stretched)
    uint8_t screen = 0;        // ngb_2d_screen_flags: 1 a menu now, 2 the last frame's menu or a 2D screen
    float hud_shape = 1;       // ngb_hud_shape
};
// The quick menu as the frame is presented (src/quick_ui.c draws it on the CPU)
struct QuickPeek {
    int percent = 0;           // its size, 0 off
    int frozen = 0;            // the game paused behind it: its background effect
    std::vector<uint32_t> layer;   // NGB_QUICK_WIDTH x NGB_QUICK_HEIGHT, empty when it is closed
    bool fps = false;              // the frame counter shown (src/fps_text.c)
    std::vector<uint32_t> fps_px;  // its picture when it changed this frame (kFpsWidth x kFpsHeight), else empty
    bool fps_lettered = false;     // that picture has the text (not yet: the font not loaded)
    bool version = false;          // the port's version shown (the main menu up, src/fps_text.c)
    std::vector<uint32_t> version_px;   // its picture when it changed (kFpsWidth x kFpsHeight), else empty
    int version_w = 0, version_h = 0;   // the text's size in it
    uint8_t crosshair = 0;              // Bow Crosshair shown (src/camera.c ngb_bow_aiming): 1 a bow, 2 the Strongbow
    std::vector<uint32_t> crosshair_px; // its picture once made (kCrosshairSize square), else empty
};

class Backend {
public:
    virtual ~Backend() = default;
    // the game's side of a call (Peek) and of a frame (QuickPeek), on the title's thread; and given back before
    // the call is drawn -- from then on the backend takes nothing itself
    virtual void CapturePeek(const State&, Peek&) {}
    virtual void CaptureQuick(QuickPeek&) {}
    virtual void UsePeek(const Peek&) {}
    virtual void UseQuick(const QuickPeek&) {}
    // the title's back buffer size, the output window may differ
    virtual bool Init(uint32_t width, uint32_t height) = 0;
    // rects: count D3DRECTs (4 x int32), null for the whole viewport
    virtual void Clear(const State& s, uint32_t count, const uint8_t* rects, uint32_t flags, uint32_t color, float z,
                       uint32_t stencil) = 0;
    // indices: the title's 16 bit indices, null for non indexed draws, then start is the first vertex
    virtual void Draw(const State& s, uint32_t prim, uint32_t start, uint32_t count, const uint8_t* indices) = 0;
    // src_rects: count D3DRECTs, dst_points: count POINTs (2 x int32), either null
    virtual void Copy(const Surface& src, const Surface& dst, uint32_t count, const uint8_t* src_rects,
                      const uint8_t* dst_points) = 0;
    virtual void Present(const Surface& front) = 0;
    // the CPU is about to touch this resource's memory: write GPU results back first
    virtual void Sync(uint32_t data_va, uint32_t size) = 0;
    // a lock that does not wait for the GPU (NOOVERWRITE): a result a frame or two old is what the console gives too
    virtual void SyncLate(uint32_t data_va) = 0;
    // SyncLate in two for the render thread: DeliverLate (any thread) writes the newest finished copy to guest memory,
    // false when there is none recent; RecordLate (in order with the draws) starts a copy for a later lock
    virtual bool DeliverLate(uint32_t) { return false; }
    virtual void RecordLate(uint32_t) {}
    virtual void Flush() = 0;
    virtual void SetGamma(const uint8_t* ramp /*3 x 256*/) = 0;

    // the XDK's fences and busy checks against the render thread; a backend that draws at once is never behind
    virtual uint32_t Fence() { return ++fence_; }
    virtual bool FencePending(uint32_t) { return false; }
    virtual void WaitFence(uint32_t) {}
    virtual bool Busy(uint32_t /*resource*/) { return false; }
    virtual void WaitResource(uint32_t /*resource*/) {}
    // after Present, out of every lock (d3d.c, the Swap hook's host section over): a renderer drawing on a thread of
    // its own waits here for the frame before, giving the virtual CPU to the title's other threads meanwhile
    virtual void FrameWait() {}
    // the title's visibility tests: the draws between counted, the count written to the report slot
    // (guest address) once known, as the GPU would
    virtual void VisibilityBegin() {}
    virtual void VisibilityEnd(uint32_t /*report_va*/) {}
    // frame interpolation (threaded.cpp): the commands that follow draw a frame again, a picture between the last
    // frame and this one; nothing goes back to the title (visibility counts) and its picture is shown at once
    virtual void SetReplay(bool) {}
    virtual void ReplaySkipped() {}   // a frame drawn without its replay (its targets fall behind)
    virtual void ReplayShowAt(int64_t /*at_us*/) {}   // the next replay's picture shown then (0 at once)
    virtual void ReplayIndex(int) {}   // which of the frame's replays (1-3) the next is: each its own render targets
    virtual void Pictures(int) {}      // pictures a frame now (the game's and its replays')
    // while set, each frame's own picture is held after its present until ShowHeld (when to show it, the
    // presenter's microsecond clock; 0 at once)
    virtual void SetHold(bool) {}
    virtual void ShowHeld(int64_t /*at_us*/) {}

private:
    uint32_t fence_ = 0;
};

std::unique_ptr<Backend> CreateBackend();
// Init failed only because the device it shares is not up yet: call again later
inline bool g_init_retry = false;
// the same backend driven from a thread of its own: calls return once recorded
std::unique_ptr<Backend> CreateThreadedBackend(std::unique_ptr<Backend> inner);
// Frame interpolation (Graphics > Frame Interpolation, NGB_INTERP): wanted or not, read by the recording at each
// frame's end (src/d3d/hle.cpp sets it at each Swap)
extern std::atomic<int> g_interp;   // 0 off, 1 Full Rate, 2 Half Rate, 3 2x

// perf diagnostic: the title's thread recording for the render thread, and waiting on it
inline std::atomic<uint64_t> g_perf_capture_ns{0}, g_perf_wait_ns{0}, g_perf_waits{0};
inline std::atomic<uint64_t> g_perf_wait_kind_ns[6];  // read-back, resource, fence, flush, copy, next frame
inline std::atomic<uint64_t> g_perf_sync_late{0}, g_perf_sync_block{0};  // read-backs the title waited for, by cause

// F3: save the next frame, every draw and target, under diag::CaptureDir
inline std::atomic<bool> g_capture{false};

}  // namespace gpu
