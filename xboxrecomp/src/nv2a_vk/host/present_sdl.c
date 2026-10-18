/*
 * Window and swapchain for the NV2A Vulkan renderer (SDL2).
 *
 * The window is created and its events pumped by the host's main thread. The
 * swapchain lives on the PFIFO thread with the rest of the queue work: each
 * vblank, pgraph_vk_sync() renders the scanout surface into the display image
 * and calls nv2a_host_vk_present(), which scales it into a swapchain image,
 * letterboxed to the configured aspect ratio, and presents.
 *
 * Mailbox presentation when the driver offers it (then immediate, then FIFO),
 * so presenting never blocks the thread that executes the pushbuffer: the
 * game's frame rate is paced by vblank interrupts, not by the swapchain.
 */
#include "hw/xbox/nv2a/nv2a_int.h"
#include "hw/xbox/nv2a/pgraph/vk/renderer.h"
#include "nv2a_host_vk.h"
#include "nv2a_window.h"
#include "nv2a_vk_share.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <time.h>
#include <SDL.h>
#include <pthread.h>
#include <SDL_vulkan.h>

#define FRAMES_IN_FLIGHT 2
#define MAX_SWAP_IMAGES 8

static SDL_Window *s_window;
static double s_aspect = 4.0 / 3.0;
static int s_vsync;                /* 0 off, 1 mailbox, 2 adaptive, 3 FIFO */
static volatile bool s_resized;

static VkInstance s_instance;
#ifdef __APPLE__
static int s_surface_pending;
#endif
static VkSurfaceKHR s_surface = VK_NULL_HANDLE;
static VkSwapchainKHR s_swapchain = VK_NULL_HANDLE;
static VkImage s_images[MAX_SWAP_IMAGES];
static VkSemaphore s_render_done[MAX_SWAP_IMAGES];
static uint32_t s_image_count;
static VkFormat s_format;
static VkExtent2D s_extent;

static VkCommandPool s_pool = VK_NULL_HANDLE;
static VkCommandBuffer s_cmd[FRAMES_IN_FLIGHT];
static VkFence s_fence[FRAMES_IN_FLIGHT];
static VkSemaphore s_acquired[FRAMES_IN_FLIGHT];
static unsigned s_frame;
unsigned long nv2a_host_presents, nv2a_host_present_blank;

/* Another renderer's picture, shown in place of the display image
 * (nv2a_vk_share.h): set from its thread, read here. */
static VkImage s_ext_image;
static uint32_t s_ext_w, s_ext_h;
volatile int64_t nv2a_ext_set_us;   /* when the newest one came */
#define s_ext_set_us nv2a_ext_set_us
/* How many pictures the other renderer has handed over, and the count when
 * the one presented last came (nv2a_host.c's vblank thread paces them). */
volatile unsigned nv2a_ext_pictures, nv2a_ext_presented;
static bool present_external_now(void);
static volatile int s_quality_set;

void nv2a_present_set_external(VkImage image, uint32_t width, uint32_t height)
{
    __atomic_store_n(&s_ext_set_us, g_get_monotonic_time(), __ATOMIC_RELAXED);
    __atomic_store_n(&s_ext_w, width, __ATOMIC_RELAXED);
    __atomic_store_n(&s_ext_h, height, __ATOMIC_RELAXED);
    __atomic_store_n(&s_ext_image, image, __ATOMIC_RELEASE);
    if (image) {
        __atomic_add_fetch(&nv2a_ext_pictures, 1, __ATOMIC_RELEASE);
        present_external_now();
    }
}

/* The picture to show or read: the external one when set (top down, BGRA,
 * TRANSFER_SRC already), else the renderer's display image (bottom up,
 * RGBA, SHADER_READ_ONLY). */
static VkImage picture_source(PGRAPHVkState *r, int *w, int *h, bool *ext)
{
    VkImage e = __atomic_load_n(&s_ext_image, __ATOMIC_ACQUIRE);
    *ext = e != VK_NULL_HANDLE;
    if (*ext) {
        *w = (int)__atomic_load_n(&s_ext_w, __ATOMIC_RELAXED);
        *h = (int)__atomic_load_n(&s_ext_h, __ATOMIC_RELAXED);
        return e;
    }
    *w = r->display.width;
    *h = r->display.height;
    return r->display.image;
}

/* ---- Frame rate overlay (NV2A_SHOW_FPS=1, F10 in game) ----
 *
 * The game's frame rate, not ours: a frame is a flip, a write of the
 * scanout address, and each is presented right after it lands, so the time
 * between presents that carry a new flip is the game's frame time. Drawn on
 * the CPU with a 5x7 font into a small buffer and copied over the corner of
 * the swapchain image -- grey and white only, so it reads the same in BGRA
 * and RGBA swapchains. */

/* Downscaling a high render resolution to the window. One bilinear blit
 * reads only 2x2 source pixels per window pixel, so past 2:1 it skips
 * pixels outright and a supersampled picture shimmers instead of getting
 * smoother. Each step here halves the picture exactly (a 2:1 linear blit
 * averages 2x2 pixels: a box filter), until the last, ordinary blit has
 * less than 2:1 left to do. */
#define MAX_HALVINGS 4
static VkImage s_half[MAX_HALVINGS];
static VmaAllocation s_half_mem[MAX_HALVINGS];
static uint32_t s_half_w[MAX_HALVINGS], s_half_h[MAX_HALVINGS];

static void halving_images(PGRAPHVkState *r, int n, uint32_t w, uint32_t h)
{
    for (int i = 0; i < MAX_HALVINGS; i++) {
        uint32_t hw = w >> (i + 1), hh = h >> (i + 1);
        bool want = i < n;
        if (s_half[i] && (!want || s_half_w[i] != hw || s_half_h[i] != hh)) {
            nv2a_vk_queue_lock();
            vkDeviceWaitIdle(r->device);
            nv2a_vk_queue_unlock();
            vmaDestroyImage(r->allocator, s_half[i], s_half_mem[i]);
            s_half[i] = VK_NULL_HANDLE;
        }
        if (want && !s_half[i]) {
            VkImageCreateInfo ci = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .imageType = VK_IMAGE_TYPE_2D,
                .format = VK_FORMAT_R8G8B8A8_UNORM,
                .extent = { hw, hh, 1 },
                .mipLevels = 1,
                .arrayLayers = 1,
                .samples = VK_SAMPLE_COUNT_1_BIT,
                .tiling = VK_IMAGE_TILING_OPTIMAL,
                .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                         VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            };
            VmaAllocationCreateInfo ai = { .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE };
            if (vmaCreateImage(r->allocator, &ci, &ai, &s_half[i], &s_half_mem[i],
                               NULL) != VK_SUCCESS) {
                s_half[i] = VK_NULL_HANDLE;
                return;
            }
            s_half_w[i] = hw;
            s_half_h[i] = hh;
        }
    }
}

static volatile bool s_show_fps;
/* F9: write the next 120 presented frames to ~/ngb-frames/ (PPM). */
static volatile int s_f9_frames;
static VkBuffer s_ovl_buf[FRAMES_IN_FLIGHT];
static VmaAllocation s_ovl_mem[FRAMES_IN_FLIGHT];
static uint8_t *s_ovl_ptr[FRAMES_IN_FLIGHT];
#define OVL_MAX_W 1600
#define OVL_MAX_H 160

static const struct { char c; uint8_t rows[7]; } s_glyphs[] = {
    { '0', { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E } },
    { '1', { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E } },
    { '2', { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F } },
    { '3', { 0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E } },
    { '4', { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 } },
    { '5', { 0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E } },
    { '6', { 0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E } },
    { '7', { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 } },
    { '8', { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E } },
    { '9', { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C } },
    { '.', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C } },
    { 'F', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 } },
    { 'P', { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 } },
    { 'S', { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E } },
    { 'm', { 0x00, 0x00, 0x1A, 0x15, 0x15, 0x11, 0x11 } },
    { 's', { 0x00, 0x00, 0x0E, 0x10, 0x0E, 0x01, 0x1E } },
    { 'a', { 0x00, 0x00, 0x0E, 0x01, 0x0F, 0x11, 0x0F } },
    { 'x', { 0x00, 0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11 } },
};

static const uint8_t *glyph(char c)
{
    for (size_t i = 0; i < sizeof s_glyphs / sizeof s_glyphs[0]; i++)
        if (s_glyphs[i].c == c)
            return s_glyphs[i].rows;
    return NULL;
}

/* Frame statistics over half a second, refreshed as they complete. */
static char s_fps_text[48] = "-- FPS";

static void fps_track(void)
{
    extern volatile unsigned nv2a_pcrtc_start_writes;
    static unsigned last_flips;
    static uint64_t last_flip_ns, window_ns;
    static unsigned frames;
    static double max_ms;
    static double jitter_sum;
    static unsigned jitter_n;
    unsigned flips = __atomic_load_n(&nv2a_pcrtc_start_writes, __ATOMIC_ACQUIRE);
    struct timespec ts;
    uint64_t now;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    now = (uint64_t)ts.tv_sec * 1000000000u + (uint64_t)ts.tv_nsec;
    if (!window_ns)
        window_ns = now;
    if (flips != last_flips) {
        if (last_flip_ns) {
            double ms = (now - last_flip_ns) / 1e6;
            if (ms > max_ms)
                max_ms = ms;
            jitter_sum += fabs(ms - 1000.0 / 60.0);
            jitter_n++;
        }
        frames++;
        last_flips = flips;
        last_flip_ns = now;
    }
    if (now - window_ns >= 500000000u) {
        double secs = (now - window_ns) / 1e9;
        unsigned frames_logged = frames;
        double fps = frames / secs;
        snprintf(s_fps_text, sizeof s_fps_text, "%.0f FPS  %.1f ms  max %.1f",
                 fps, frames ? secs * 1000.0 / frames : 0.0, max_ms);
        /* NV2A_FPS_LOG=1: to stderr; NV2A_FPS_LOG=<path>: appended to that
         * file (where stderr is hard to reach, e.g. a Steam launch). */
        const char *fl = getenv("NV2A_FPS_LOG");
        if (fl && *fl) {
            extern unsigned long nv2a_early_presents;
            static unsigned long last_early;
            static FILE *out;
            if (!out) {
                out = strcmp(fl, "1") ? fopen(fl, "a") : NULL;
                if (!out)
                    out = stderr;
            }
            fprintf(out, "[FPS] %s  (early %lu of %u, jitter %.2f ms)\n",
                    s_fps_text, nv2a_early_presents - last_early,
                    frames_logged, jitter_n ? jitter_sum / jitter_n : 0.0);
            fflush(out);
            last_early = nv2a_early_presents;
        }
        window_ns = now;
        frames = 0;
        max_ms = 0;
        jitter_sum = 0;
        jitter_n = 0;
    }
}

/* The title can letter the overlay itself, in its own font: text into px (at most maxw x maxh, opaque
 * grey and white pixels), about line_px tall; false to leave it to the 5x7 font here. */
__attribute__((weak)) bool nv2a_fps_render(const char *text, uint32_t line_px, uint32_t *px,
                                           uint32_t maxw, uint32_t maxh, uint32_t *ow, uint32_t *oh)
{
    (void)text; (void)line_px; (void)px; (void)maxw; (void)maxh; (void)ow; (void)oh;
    return false;
}

/* Draw the text into frame f's buffer; returns its size in pixels. */
static void fps_draw(unsigned f, uint32_t *ow, uint32_t *oh)
{
    {
        uint32_t line = s_extent.height / 36, mw = OVL_MAX_W, mh = OVL_MAX_H;
        if (line < 14) line = 14;
        if (mw > s_extent.width) mw = s_extent.width;
        if (mh > s_extent.height) mh = s_extent.height;
        if (nv2a_fps_render(s_fps_text, line, (uint32_t *)s_ovl_ptr[f], mw, mh, ow, oh))
            return;
    }
    unsigned k = s_extent.height / 360;
    const char *t = s_fps_text;
    uint32_t w, h, *px = (uint32_t *)s_ovl_ptr[f];

    if (k < 2) k = 2;
    if (k > 8) k = 8;
    w = (uint32_t)(strlen(t) * 6 + 5) * k;
    h = 13 * k;
    if (w > OVL_MAX_W) w = OVL_MAX_W;
    if (h > OVL_MAX_H) h = OVL_MAX_H;
    if (w > s_extent.width) w = s_extent.width;
    if (h > s_extent.height) h = s_extent.height;
    for (uint32_t i = 0; i < w * h; i++)
        px[i] = 0xFF181818u;
    for (int ci = 0; t[ci]; ci++) {
        const uint8_t *g = glyph(t[ci]);
        if (!g)
            continue;
        for (int gy = 0; gy < 7; gy++)
            for (int gx = 0; gx < 5; gx++) {
                if (!(g[gy] & (0x10 >> gx)))
                    continue;
                for (unsigned yy = 0; yy < k; yy++)
                    for (unsigned xx = 0; xx < k; xx++) {
                        uint32_t x = (3 + ci * 6 + gx) * k + xx;
                        uint32_t y = (3 + gy) * k + yy;
                        if (x < w && y < h)
                            px[y * w + x] = 0xFFFFFFFFu;
                    }
            }
    }
    *ow = w;
    *oh = h;
}

static void fps_init(PGRAPHVkState *r)
{
    const char *e = getenv("NV2A_SHOW_FPS");
    s_show_fps = e && *e && *e != '0';
    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        VkBufferCreateInfo bi = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = (VkDeviceSize)OVL_MAX_W * OVL_MAX_H * 4,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };
        VmaAllocationCreateInfo ai = {
            .usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                     VMA_ALLOCATION_CREATE_MAPPED_BIT,
        };
        VmaAllocationInfo info;
        if (vmaCreateBuffer(r->allocator, &bi, &ai, &s_ovl_buf[i],
                            &s_ovl_mem[i], &info) != VK_SUCCESS) {
            s_ovl_buf[i] = VK_NULL_HANDLE;
            continue;
        }
        s_ovl_ptr[i] = info.pMappedData;
    }
}

/* ---- Window (host main thread) ---- */

/* Display modes: 0 a window, 1 fullscreen (the desktop's), 2 borderless
 * (no frame, over the whole display). */
static void borderless_apply(SDL_Window *w)
{
    SDL_Rect r;
    int d = SDL_GetWindowDisplayIndex(w);
    if (SDL_GetDisplayBounds(d < 0 ? 0 : d, &r) != 0)
        return;
    SDL_SetWindowBordered(w, SDL_FALSE);
    SDL_SetWindowPosition(w, r.x, r.y);
    SDL_SetWindowSize(w, r.w, r.h);
}

int nv2a_window_create(const char *title, int width, int height,
                       int fullscreen, double aspect, int vsync)
{
    /* Native Wayland when the session has it, X11 otherwise. SDL2 picks
     * X11 first, so on a Wayland desktop the game went through XWayland.
     * SDL_VIDEODRIVER still overrides. (Windows has its one driver.) */
#if defined(__linux__)
    if (!getenv("SDL_VIDEODRIVER")) {
        /* Under gamescope (the Steam Deck's Game Mode) X11: its frame
         * counting -- the performance overlay's frame rate and frame times
         * -- follows the XWayland window, not a Wayland surface. */
        const char *desk = getenv("XDG_CURRENT_DESKTOP");
        int gamescope = getenv("GAMESCOPE_WAYLAND_DISPLAY") != NULL ||
                        (desk && strcasecmp(desk, "gamescope") == 0);
        SDL_SetHint(SDL_HINT_VIDEODRIVER, gamescope ? "x11,wayland" : "wayland,x11");
    }
#endif
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "nv2a: SDL video: %s\n", SDL_GetError());
        return -1;
    }
    s_window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED,
                                SDL_WINDOWPOS_CENTERED, width, height,
                                (getenv("NGB_TEST_HIDDEN") ? SDL_WINDOW_HIDDEN : 0) |
                                SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE |
                                SDL_WINDOW_ALLOW_HIGHDPI |
                                (fullscreen == 1 ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) |
                                (fullscreen == 2 ? SDL_WINDOW_BORDERLESS : 0));
    if (!s_window) {
        fprintf(stderr, "nv2a: window: %s\n", SDL_GetError());
        return -1;
    }
    fprintf(stderr, "nv2a: window on %s\n", SDL_GetCurrentVideoDriver());
    if (fullscreen == 2)
        borderless_apply(s_window);
    if (aspect > 0)
        s_aspect = aspect;
    else if (aspect < 0)
        s_aspect = 0;            /* stretch: fill the window */
    s_vsync = vsync;
    return 0;
}

/* Window changes asked for by the overlay (renderer thread), made here on
 * the main thread, which owns the window. */
static int s_win_req, s_win_w, s_win_h, s_win_fs;

void nv2a_window_request(int width, int height, int fullscreen)   /* mode: 0, 1, 2 */
{
    s_win_w = width;
    s_win_h = height;
    s_win_fs = fullscreen;
    __atomic_store_n(&s_win_req, 1, __ATOMIC_RELEASE);
}

void nv2a_present_set_aspect(double aspect)
{
    s_aspect = aspect > 0 ? aspect : 0;
}

/* For the vblank clock (nv2a_host.c): 0 off, 1 mailbox, 2 adaptive, 3 FIFO. */
int nv2a_present_vsync(void)
{
    return s_vsync;
}

void nv2a_present_set_vsync(int vsync)
{
    if (vsync != s_vsync) {
        s_vsync = vsync;
        s_resized = true;            /* a new swapchain, in the new mode */
    }
}

void nv2a_present_set_show_fps(bool show)
{
    s_show_fps = show;
}

static volatile bool s_blank;

void nv2a_present_set_blank(bool blank)
{
    s_blank = blank;
}

/* Each pump of the window's events, on its thread (weak; a host defines it
 * for work that thread must do: the mouse's capture). */
__attribute__((weak)) void nv2a_window_tick(void) {}

__attribute__((weak)) void xbox_InputHotplugPoll(void) {}

bool nv2a_window_pump(void)
{
#ifdef __APPLE__
    /* SDL's Vulkan surface creates and attaches a Cocoa Metal view. AppKit
     * changes must run on this thread, even though Vulkan runs on PFIFO. */
    if (__atomic_load_n(&s_surface_pending, __ATOMIC_ACQUIRE)) {
        if (!SDL_Vulkan_CreateSurface(s_window, s_instance, &s_surface)) {
            fprintf(stderr, "nv2a: Vulkan surface: %s\n", SDL_GetError());
            s_surface = VK_NULL_HANDLE;
        }
        __atomic_store_n(&s_surface_pending, 0, __ATOMIC_RELEASE);
    }
#endif
    xbox_InputHotplugPoll();   /* pads connected or gone in the last pump (xboxrecomp input) */
    nv2a_window_tick();
    SDL_Event ev;

    if (__atomic_exchange_n(&s_win_req, 0, __ATOMIC_ACQ_REL)) {
        bool fs = SDL_GetWindowFlags(s_window) & SDL_WINDOW_FULLSCREEN;
        if (s_win_fs != 1 && fs)
            SDL_SetWindowFullscreen(s_window, 0);
        if (s_win_fs != 2)
            SDL_SetWindowBordered(s_window, SDL_TRUE);
        if (s_win_fs == 0 && s_win_w > 0 && s_win_h > 0) {
            SDL_SetWindowSize(s_window, s_win_w, s_win_h);
            SDL_SetWindowPosition(s_window, SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED);
        }
        if (s_win_fs == 2)
            borderless_apply(s_window);
        if (s_win_fs == 1 && !fs)
            SDL_SetWindowFullscreen(s_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        s_resized = true;
    }

    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
        case SDL_QUIT:
            return false;
        case SDL_WINDOWEVENT:
            if (ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                s_resized = true;
            break;
        case SDL_KEYDOWN:
            if (ev.key.keysym.sym == SDLK_F10 && !ev.key.repeat)
                s_show_fps = !s_show_fps;
            if (ev.key.keysym.sym == SDLK_F9 && !ev.key.repeat)
                __atomic_store_n(&s_f9_frames, 120, __ATOMIC_RELEASE);
            if (ev.key.keysym.sym == SDLK_F11 ||
                (ev.key.keysym.sym == SDLK_RETURN &&
                 (ev.key.keysym.mod & KMOD_ALT))) {
                bool fs = SDL_GetWindowFlags(s_window) & SDL_WINDOW_FULLSCREEN;
                SDL_SetWindowFullscreen(s_window,
                                        fs ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
                if (fs)
                    SDL_SetWindowBordered(s_window, SDL_TRUE);
            }
            break;
        default:
            break;
        }
        nv2a_window_event_hook(&ev);
    }
    return true;
}

__attribute__((weak)) void nv2a_window_event_hook(const void *sdl_event)
{
    (void)sdl_event;
}

__attribute__((weak)) bool nv2a_overlay_wanted(void)
{
    return false;
}

__attribute__((weak)) bool nv2a_overlay_frame(uint32_t *px, int w, int h,
                                              const uint32_t *backdrop)
{
    (void)px; (void)w; (void)h; (void)backdrop;
    return false;
}

static volatile int s_q_scale, s_q_aniso, s_q_shadow, s_q_msaa = 1;

static volatile int s_q_known;

void nv2a_set_quality(int render_scale, int anisotropy, int shadow_scale, int msaa)
{
    s_q_scale = render_scale;
    s_q_aniso = anisotropy;
    s_q_shadow = shadow_scale;
    s_q_msaa = msaa;
    __atomic_store_n(&s_q_known, 1, __ATOMIC_RELEASE);
    __atomic_store_n(&s_quality_set, 1, __ATOMIC_RELEASE);
}

void nv2a_quality_get(int *render_scale, int *anisotropy, int *shadow_scale, int *msaa)
{
    extern int pgraph_vk_tex_smooth, pgraph_vk_shadow_scale;
    int scale, aniso, shadow, samples;
    if (__atomic_load_n(&s_q_known, __ATOMIC_ACQUIRE)) {
        scale = s_q_scale;
        aniso = s_q_aniso;
        shadow = s_q_shadow;
        samples = s_q_msaa;
    } else {
        /* The renderer's own defaults (texture.c, surface.c). */
        const char *e;
        scale = (int)g_config.display.quality.surface_scale;
        aniso = pgraph_vk_tex_smooth >= 0 ? pgraph_vk_tex_smooth
                : ((e = getenv("NV2A_TEX_FILTER")) && !strcmp(e, "original")) ? 0 : 16;
        shadow = pgraph_vk_shadow_scale >= 1 ? pgraph_vk_shadow_scale
                 : (e = getenv("NV2A_SHADOW_SCALE")) ? atoi(e) : 1;
        samples = (e = getenv("NV2A_MSAA")) ? atoi(e) : 1;
    }
    if (render_scale)
        *render_scale = scale < 1 ? 1 : scale > 10 ? 10 : scale;
    if (anisotropy)
        *anisotropy = aniso <= 0 ? 0 : aniso >= 16 ? 16 : aniso >= 8 ? 8 : aniso >= 4 ? 4 : 2;
    if (shadow_scale)
        *shadow_scale = shadow < 1 ? 1 : shadow > 10 ? 10 : shadow;
    if (msaa)
        *msaa = samples >= 8 ? 8 : samples >= 4 ? 4 : samples >= 2 ? 2 : 1;
}

/* ---- Hooks called by the renderer (PFIFO thread) ---- */

const char *const *nv2a_host_vk_instance_extensions(uint32_t *count)
{
    static const char *names[16];
    unsigned n = 16;

    *count = 0;
    if (!s_window || !SDL_Vulkan_GetInstanceExtensions(s_window, &n, names))
        return NULL;
    *count = n;
    return names;
}

void nv2a_host_vk_instance_created(VkInstance instance)
{
    s_instance = instance;
#ifdef __APPLE__
    if (s_window) {
        __atomic_store_n(&s_surface_pending, 1, __ATOMIC_RELEASE);
        while (__atomic_load_n(&s_surface_pending, __ATOMIC_ACQUIRE))
            SDL_Delay(1);
    }
#else
    if (s_window && !SDL_Vulkan_CreateSurface(s_window, instance, &s_surface)) {
        fprintf(stderr, "nv2a: Vulkan surface: %s\n", SDL_GetError());
        s_surface = VK_NULL_HANDLE;
    }
#endif
}

bool nv2a_host_vk_can_present(VkPhysicalDevice dev, uint32_t queue_family)
{
    VkBool32 ok = VK_FALSE;

    if (s_surface == VK_NULL_HANDLE)
        return true;
    vkGetPhysicalDeviceSurfaceSupportKHR(dev, queue_family, s_surface, &ok);
    return ok;
}

static void destroy_swapchain(PGRAPHVkState *r)
{
    nv2a_vk_queue_lock();
    vkDeviceWaitIdle(r->device);
    nv2a_vk_queue_unlock();
    for (uint32_t i = 0; i < s_image_count; i++) {
        vkDestroySemaphore(r->device, s_render_done[i], NULL);
        s_render_done[i] = VK_NULL_HANDLE;
    }
    if (s_swapchain)
        vkDestroySwapchainKHR(r->device, s_swapchain, NULL);
    s_swapchain = VK_NULL_HANDLE;
    s_image_count = 0;
}

/* The game's frames come from its own 60 Hz clock, not the display's, so a
 * FIFO queue fills up and stays full: every frame then waits behind two or
 * three others, 30-50 ms of input lag that the frame counter does not show.
 * So V-Sync is mailbox where the system has it -- no tearing, and the newest
 * frame at each refresh -- and FIFO (relaxed, for Adaptive) only where it has
 * not, on as few images as it allows. Off is immediate (tearing, the least
 * lag), else mailbox.
 *
 * But mailbox is what Nvidia calls Fast sync, and with the game's frames at
 * the refresh rate it can stutter: each frame lands at a different point of
 * the refresh, and now and then one is shown twice and the next not at all
 * (#17). So V-Sync also offers FIFO (3): every frame in turn, classic
 * V-Sync, the steadiest; with NGB_VBLANK_SYNC=1 on Windows the game's frames
 * then follow the display's own refresh. */
static VkPresentModeKHR choose_present_mode(PGRAPHVkState *r)
{
    VkPresentModeKHR modes[16];
    uint32_t n = 16;
    bool mailbox = false, immediate = false, relaxed = false;

    vkGetPhysicalDeviceSurfacePresentModesKHR(r->physical_device, s_surface,
                                              &n, modes);
    for (uint32_t i = 0; i < n; i++) {
        mailbox |= modes[i] == VK_PRESENT_MODE_MAILBOX_KHR;
        immediate |= modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR;
        relaxed |= modes[i] == VK_PRESENT_MODE_FIFO_RELAXED_KHR;
    }
    if (s_vsync == 0 && immediate)
        return VK_PRESENT_MODE_IMMEDIATE_KHR;
    if (s_vsync == 3)   /* the menus' FIFO */
        return VK_PRESENT_MODE_FIFO_KHR;
    if (mailbox)
        return VK_PRESENT_MODE_MAILBOX_KHR;
    if (s_vsync == 2 && relaxed)
        return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
    return VK_PRESENT_MODE_FIFO_KHR;
}

static bool create_swapchain(PGRAPHVkState *r)
{
    VkSurfaceCapabilitiesKHR caps;
    VkSurfaceFormatKHR formats[32];
    uint32_t nf = 32;
    VkSwapchainKHR old = s_swapchain;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(r->physical_device, s_surface, &caps);
    vkGetPhysicalDeviceSurfaceFormatsKHR(r->physical_device, s_surface, &nf, formats);
    if (!nf)
        return false;

    /* UNORM to match the display image: the NV2A's output is already in
     * display space, and an sRGB swapchain would encode it a second time. */
    s_format = formats[0].format;
    for (uint32_t i = 0; i < nf; i++)
        if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM ||
            formats[i].format == VK_FORMAT_R8G8B8A8_UNORM) {
            s_format = formats[i].format;
            break;
        }

    if (caps.currentExtent.width != 0xFFFFFFFFu) {
        s_extent = caps.currentExtent;
    } else {
        int w, h;
        SDL_Vulkan_GetDrawableSize(s_window, &w, &h);
        s_extent.width = w;
        s_extent.height = h;
    }
    if (!s_extent.width || !s_extent.height)
        return false;   /* minimised */

    {
        VkPresentModeKHR mode = choose_present_mode(r);
        /* Mailbox needs a spare image to replace; a FIFO queue is kept short. */
        uint32_t want = caps.minImageCount + (mode == VK_PRESENT_MODE_MAILBOX_KHR ? 1 : 0);
        if (want < 2)
            want = 2;
        if (caps.maxImageCount && want > caps.maxImageCount)
            want = caps.maxImageCount;
        if (want > MAX_SWAP_IMAGES)
            want = MAX_SWAP_IMAGES;

        VkSwapchainCreateInfoKHR ci = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = s_surface,
            .minImageCount = want,
            .imageFormat = s_format,
            .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
            .imageExtent = s_extent,
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .preTransform = caps.currentTransform,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = mode,
            .clipped = VK_TRUE,
            .oldSwapchain = old,
        };
        if (vkCreateSwapchainKHR(r->device, &ci, NULL, &s_swapchain) != VK_SUCCESS) {
            s_swapchain = VK_NULL_HANDLE;
            return false;
        }
        {
            static const char *const names[] = { "immediate", "mailbox", "fifo", "fifo relaxed" };
            fprintf(stderr, "nv2a: swapchain %ux%u, %s, %u images (driver minimum %u)\n",
                    s_extent.width, s_extent.height,
                    ci.presentMode <= 3 ? names[ci.presentMode] : "?", want, caps.minImageCount);
        }
    }
    if (old) {
        nv2a_vk_queue_lock();
        vkDeviceWaitIdle(r->device);
        nv2a_vk_queue_unlock();
        for (uint32_t i = 0; i < s_image_count; i++)
            vkDestroySemaphore(r->device, s_render_done[i], NULL);
        vkDestroySwapchainKHR(r->device, old, NULL);
    }

    s_image_count = MAX_SWAP_IMAGES;
    vkGetSwapchainImagesKHR(r->device, s_swapchain, &s_image_count, s_images);
    for (uint32_t i = 0; i < s_image_count; i++) {
        VkSemaphoreCreateInfo si = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        vkCreateSemaphore(r->device, &si, NULL, &s_render_done[i]);
    }
    return true;
}

static void create_frame_resources(PGRAPHVkState *r)
{
    VkCommandPoolCreateInfo pi = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex =
            pgraph_vk_find_queue_families(r->physical_device).queue_family,
    };
    VkCommandBufferAllocateInfo ai = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = FRAMES_IN_FLIGHT,
    };
    VK_CHECK(vkCreateCommandPool(r->device, &pi, NULL, &s_pool));
    ai.commandPool = s_pool;
    VK_CHECK(vkAllocateCommandBuffers(r->device, &ai, s_cmd));
    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        VkFenceCreateInfo fi = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                                 .flags = VK_FENCE_CREATE_SIGNALED_BIT };
        VkSemaphoreCreateInfo si = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VK_CHECK(vkCreateFence(r->device, &fi, NULL, &s_fence[i]));
        VK_CHECK(vkCreateSemaphore(r->device, &si, NULL, &s_acquired[i]));
    }
}

static void barrier(VkCommandBuffer cmd, VkImage img, VkImageLayout from,
                    VkImageLayout to, VkAccessFlags src, VkAccessFlags dst,
                    VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage)
{
    VkImageMemoryBarrier b = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = src,
        .dstAccessMask = dst,
        .oldLayout = from,
        .newLayout = to,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = img,
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
    };
    vkCmdPipelineBarrier(cmd, src_stage, dst_stage, 0, 0, NULL, 0, NULL, 1, &b);
}

/* NV2A_CAPTURE=prefix: every 300 presents, the display image as a PPM.
 * Reads the picture back from the GPU itself, so it shows exactly what is
 * being presented, with no compositor or screenshot tool in the way. */
static void capture_display(PGRAPHState *pg)
{
    PGRAPHVkState *r = pg->vk_renderer_state;
    const char *prefix = getenv("NV2A_CAPTURE");
    static char f9_prefix[4096];
    if (__atomic_load_n(&s_f9_frames, __ATOMIC_ACQUIRE) > 0) {
        if (!f9_prefix[0]) {
            const char *home = getenv("HOME");
            snprintf(f9_prefix, sizeof f9_prefix, "%s/ngb-frames", home ? home : ".");
            g_mkdir_with_parents(f9_prefix, 0755);
            strncat(f9_prefix, "/f", sizeof f9_prefix - strlen(f9_prefix) - 1);
            fprintf(stderr, "[NV2A] F9: writing 120 frames to %s*.ppm\n", f9_prefix);
        }
        prefix = f9_prefix;
    }
    static int n;
    int w, h;
    bool ext;
    VkImage src = picture_source(r, &w, &h, &ext);
    VkBuffer buf;
    VmaAllocation alloc;
    VmaAllocationInfo info;
    char path[512];
    FILE *f;

    if (!prefix || src == VK_NULL_HANDLE || !w || !h)
        return;
    {
        VkBufferCreateInfo bi = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = (VkDeviceSize)w * h * 4,
            .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        };
        VmaAllocationCreateInfo ai = {
            .usage = VMA_MEMORY_USAGE_AUTO,
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
                     VMA_ALLOCATION_CREATE_MAPPED_BIT,
        };
        if (vmaCreateBuffer(r->allocator, &bi, &ai, &buf, &alloc, &info) != VK_SUCCESS)
            return;
    }
    {
        VkCommandBuffer cmd = pgraph_vk_begin_single_time_commands(pg);
        VkBufferImageCopy region = {
            .imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
            .imageExtent = { w, h, 1 },
        };
        if (!ext)
            barrier(cmd, src, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT);
        else   /* the other renderer's frame, submitted before: wait for its writes */
            barrier(cmd, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
        vkCmdCopyImageToBuffer(cmd, src,
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buf, 1, &region);
        if (!ext)
            barrier(cmd, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        pgraph_vk_end_single_time_commands(pg, cmd);
    }
    vmaInvalidateAllocation(r->allocator, alloc, 0, VK_WHOLE_SIZE);
    snprintf(path, sizeof(path), "%s%03d.ppm", prefix, n++);
    if ((f = fopen(path, "wb"))) {
        const uint8_t *px = info.pMappedData;
        fprintf(f, "P6\n%d %d\n255\n", w, h);
        uint8_t *row = g_malloc((size_t)w * 3);
        for (int i = 0; i < h; i++) {
            int y = ext ? i : h - 1 - i;        /* bottom-up, see blit */
            for (int x = 0; x < w; x++) {
                const uint8_t *q = px + ((size_t)y * w + x) * 4;
                if (ext) {                       /* BGRA -> RGB */
                    row[x * 3] = q[2];
                    row[x * 3 + 1] = q[1];
                    row[x * 3 + 2] = q[0];
                } else {
                    memcpy(row + x * 3, q, 3);   /* RGBA -> RGB */
                }
            }
            fwrite(row, 1, (size_t)w * 3, f);
        }
        g_free(row);
        fclose(f);
    }
    vmaDestroyBuffer(r->allocator, buf, alloc);
}

/* ---- Host overlay ---- */

/* Where the picture goes in the window: as the present blit fits it. */
static void picture_rect(int ww, int wh, int *x0, int *y0, int *dw, int *dh)
{
    *dw = ww;
    *dh = s_aspect > 0 ? (int)(ww / s_aspect) : wh;
    if (*dh > wh) {
        *dh = wh;
        *dw = (int)(wh * s_aspect);
    }
    *x0 = (ww - *dw) / 2;
    *y0 = (wh - *dh) / 2;
}

void nv2a_present_picture_rect(int ww, int wh, int *x0, int *y0, int *dw, int *dh)
{
    picture_rect(ww, wh, x0, y0, dw, dh);
}

/* The display image, read back: RGBA rows, bottom-up (see the blit). */
static uint8_t *read_display(PGRAPHState *pg, int *ow, int *oh)
{
    PGRAPHVkState *r = pg->vk_renderer_state;
    int w, h;
    bool ext;
    VkImage src = picture_source(r, &w, &h, &ext);
    VkBuffer buf;
    VmaAllocation alloc;
    VmaAllocationInfo info;
    uint8_t *out;

    if (src == VK_NULL_HANDLE || !w || !h)
        return NULL;
    VkBufferCreateInfo bi = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = (VkDeviceSize)w * h * 4,
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    };
    VmaAllocationCreateInfo ai = {
        .usage = VMA_MEMORY_USAGE_AUTO,
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
                 VMA_ALLOCATION_CREATE_MAPPED_BIT,
    };
    if (vmaCreateBuffer(r->allocator, &bi, &ai, &buf, &alloc, &info) != VK_SUCCESS)
        return NULL;
    VkCommandBuffer cmd = pgraph_vk_begin_single_time_commands(pg);
    VkBufferImageCopy region = {
        .imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        .imageExtent = { w, h, 1 },
    };
    if (!ext)
        barrier(cmd, src, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT);
    else
        barrier(cmd, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    vkCmdCopyImageToBuffer(cmd, src,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buf, 1, &region);
    if (!ext)
        barrier(cmd, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    pgraph_vk_end_single_time_commands(pg, cmd);
    vmaInvalidateAllocation(r->allocator, alloc, 0, VK_WHOLE_SIZE);
    out = g_malloc((size_t)w * h * 4);
    if (ext) {
        /* To the display image's order: bottom up, RGBA. */
        const uint8_t *px = info.pMappedData;
        for (int y = 0; y < h; y++) {
            uint8_t *d = out + (size_t)(h - 1 - y) * w * 4;
            const uint8_t *q = px + (size_t)y * w * 4;
            for (int x = 0; x < w; x++, d += 4, q += 4) {
                d[0] = q[2];
                d[1] = q[1];
                d[2] = q[0];
                d[3] = q[3];
            }
        }
    } else {
        memcpy(out, info.pMappedData, (size_t)w * h * 4);
    }
    vmaDestroyBuffer(r->allocator, buf, alloc);
    *ow = w;
    *oh = h;
    return out;
}

/* The frame fitted to a w x h window as 0xAARRGGBB, black around it; each
 * window pixel the average of the frame pixels under it. */
static void fit_backdrop(const uint8_t *src, int sw, int sh, uint32_t *dst,
                         int w, int h)
{
    int x0, y0, dw, dh;
    picture_rect(w, h, &x0, &y0, &dw, &dh);
    for (int i = 0; i < w * h; i++)
        dst[i] = 0xFF000000u;
    if (!src || dw <= 0 || dh <= 0)
        return;
    for (int y = 0; y < dh; y++) {
        int sy0 = (int)((int64_t)y * sh / dh), sy1 = (int)((int64_t)(y + 1) * sh / dh);
        if (sy1 <= sy0) sy1 = sy0 + 1;
        for (int x = 0; x < dw; x++) {
            int sx0 = (int)((int64_t)x * sw / dw), sx1 = (int)((int64_t)(x + 1) * sw / dw);
            if (sx1 <= sx0) sx1 = sx0 + 1;
            uint32_t r = 0, g = 0, b = 0, n = 0;
            for (int yy = sy0; yy < sy1 && yy < sh; yy++) {
                const uint8_t *p = src + ((size_t)(sh - 1 - yy) * sw + sx0) * 4;
                for (int xx = sx0; xx < sx1 && xx < sw; xx++, p += 4) {
                    r += p[0];
                    g += p[1];
                    b += p[2];
                    n++;
                }
            }
            if (n)
                dst[(size_t)(y0 + y) * w + x0 + x] =
                    0xFF000000u | (r / n) << 16 | (g / n) << 8 | (b / n);
        }
    }
}

/* Show a w x h host picture (0xAARRGGBB) in the window. */
static void present_pixels(PGRAPHVkState *r, VkBuffer buf, int w, int h)
{
    unsigned f = s_frame++ % FRAMES_IN_FLIGHT;
    uint32_t idx;
    VkResult res;

    vkWaitForFences(r->device, 1, &s_fence[f], VK_TRUE, UINT64_MAX);
    res = vkAcquireNextImageKHR(r->device, s_swapchain, 100000000ull,
                                s_acquired[f], VK_NULL_HANDLE, &idx);
    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        s_resized = true;
        return;
    }
    if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
        return;
    vkResetFences(r->device, 1, &s_fence[f]);
    VkCommandBuffer cmd = s_cmd[f];
    vkResetCommandBuffer(cmd, 0);
    VkCommandBufferBeginInfo bi = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(cmd, &bi);
    barrier(cmd, s_images[idx], VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    VkBufferImageCopy region = {
        .bufferRowLength = w,
        .imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
        .imageExtent = { w, h, 1 },
    };
    vkCmdCopyBufferToImage(cmd, buf, s_images[idx],
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    barrier(cmd, s_images[idx], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_TRANSFER_WRITE_BIT, 0,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    vkEndCommandBuffer(cmd);
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkSubmitInfo si = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &s_acquired[f],
        .pWaitDstStageMask = &wait_stage,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &s_render_done[idx],
    };
    nv2a_vk_queue_lock();
    VK_CHECK(vkQueueSubmit(r->queue, 1, &si, s_fence[f]));
    VkPresentInfoKHR pi = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &s_render_done[idx],
        .swapchainCount = 1,
        .pSwapchains = &s_swapchain,
        .pImageIndices = &idx,
    };
    res = vkQueuePresentKHR(r->queue, &pi);
    nv2a_vk_queue_unlock();
    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
        s_resized = true;
}

/* The overlay, over the frame the game is held at: the renderer's thread
 * stays here, so the title's next flip waits and the game stands still. */
static void overlay_run(PGRAPHState *pg)
{
    PGRAPHVkState *r = pg->vk_renderer_state;
    int fw = 0, fh = 0, bw = 0, bh = 0;
    uint8_t *frame = read_display(pg, &fw, &fh);
    uint32_t *backdrop = NULL, *px = NULL;
    VkBuffer buf = VK_NULL_HANDLE;
    VmaAllocation mem = NULL;
    uint32_t *mapped = NULL;
    bool open = true;

    while (open) {
        int64_t t0 = g_get_monotonic_time();
        if (s_resized || !s_swapchain) {
            s_resized = false;
            if (!create_swapchain(r)) {
                g_usleep(16000);
                continue;
            }
        }
        int w = (int)s_extent.width, h = (int)s_extent.height;
        if (w != bw || h != bh) {
            if (buf) {
                nv2a_vk_queue_lock();
                vkDeviceWaitIdle(r->device);
                nv2a_vk_queue_unlock();
                vmaDestroyBuffer(r->allocator, buf, mem);
                buf = VK_NULL_HANDLE;
            }
            g_free(backdrop);
            g_free(px);
            backdrop = g_malloc((size_t)w * h * 4);
            px = g_malloc((size_t)w * h * 4);
            fit_backdrop(frame, fw, fh, backdrop, w, h);
            VkBufferCreateInfo bi = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                .size = (VkDeviceSize)w * h * 4,
                .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            };
            VmaAllocationCreateInfo ai = {
                .usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
                .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                         VMA_ALLOCATION_CREATE_MAPPED_BIT,
            };
            VmaAllocationInfo info;
            if (vmaCreateBuffer(r->allocator, &bi, &ai, &buf, &mem, &info) != VK_SUCCESS) {
                buf = VK_NULL_HANDLE;
                break;
            }
            mapped = info.pMappedData;
            bw = w;
            bh = h;
        }
        open = nv2a_overlay_frame(px, w, h, backdrop);
        /* One buffer for every frame in flight: wait until it is free. */
        nv2a_vk_queue_lock();
        vkDeviceWaitIdle(r->device);
        nv2a_vk_queue_unlock();
        if (s_format == VK_FORMAT_R8G8B8A8_UNORM) {
            for (int i = 0; i < w * h; i++) {
                uint32_t c = px[i];
                mapped[i] = (c & 0xFF00FF00u) | (c >> 16 & 0xFF) | (c & 0xFF) << 16;
            }
        } else {
            memcpy(mapped, px, (size_t)w * h * 4);
        }
        vmaFlushAllocation(r->allocator, mem, 0, VK_WHOLE_SIZE);
        present_pixels(r, buf, w, h);
        int64_t spent = g_get_monotonic_time() - t0;
        if (open && spent < 16000)
            g_usleep(16000 - spent);
    }
    nv2a_vk_queue_lock();
    vkDeviceWaitIdle(r->device);
    nv2a_vk_queue_unlock();
    if (buf)
        vmaDestroyBuffer(r->allocator, buf, mem);
    g_free(backdrop);
    g_free(px);
    g_free(frame);
}

/* NV2A_FPS_LOG: how old the other renderer's picture is when presented
 * (from its hand-over to the queue), and frames dropped with every
 * swapchain image queued. */
static unsigned s_lat_dropped;

static void lat_note(bool ext)
{
    static int on = -1;
    static unsigned n;
    static int64_t sum, max;
    if (on < 0)
        on = getenv("NV2A_FPS_LOG") != NULL;
    if (!on || !ext)
        return;
    static int64_t last, gmin = INT64_MAX, gmax;
    int64_t now = g_get_monotonic_time();
    int64_t age = now - __atomic_load_n(&s_ext_set_us, __ATOMIC_RELAXED);
    if (last) {   /* the time between presents: even, or the picture stutters */
        if (now - last < gmin) gmin = now - last;
        if (now - last > gmax) gmax = now - last;
    }
    last = now;
    if (getenv("NV2A_PACE_LOG")) {
        extern volatile int64_t nv2a_vblank_us;
        static int pn;
        if (pn++ < 400)
            fprintf(stderr, "[PACE] present at %.1f after the tick, age %.1f\n",
                    (now - nv2a_vblank_us) / 1000.0, age / 1000.0);
    }
    sum += age;
    if (age > max)
        max = age;
    if (++n == 120) {
        fprintf(stderr, "[LAT] picture age at present: avg %.1f ms, max %.1f ms; presents %.1f-%.1f ms apart;"
                " %u dropped (images all queued)\n",
                sum / 120 / 1000.0, max / 1000.0, gmin / 1000.0, gmax / 1000.0, s_lat_dropped);
        n = 0;
        sum = max = 0;
        gmin = INT64_MAX;
        gmax = 0;
        s_lat_dropped = 0;
    }
}

static void present_locked(PGRAPHState *pg, bool on_pfifo);

/* Presents come from the PFIFO thread (the vblank tick's, the renderer's
 * own picture) and from the other renderer's thread (its picture, the moment
 * it hands it over: nv2a_present_set_external) -- one at a time. */
static pthread_mutex_t s_present_mu = PTHREAD_MUTEX_INITIALIZER;
static PGRAPHState *volatile s_pg;

void nv2a_host_vk_present(PGRAPHState *pg)
{
    s_pg = pg;
    pthread_mutex_lock(&s_present_mu);
    present_locked(pg, true);
    pthread_mutex_unlock(&s_present_mu);
}

/* The other renderer's picture, presented from its own thread as it is
 * handed over: a fixed time after each vblank, so evenly spaced, and with
 * no wait for the PFIFO thread, which is busy with the next frame's
 * commands by then (presented through it, they came out unevenly, and at
 * the tick a whole refresh late). False to leave it to the tick. */
static bool present_external_now(void)
{
    static int on = -1;
    PGRAPHState *pg = s_pg;
    if (on < 0)
        on = !(getenv("NV2A_PACE") && !strcmp(getenv("NV2A_PACE"), "tick"));
    if (!on || !pg || __atomic_load_n(&s_quality_set, __ATOMIC_ACQUIRE))
        return false;   /* (quality changes are the renderer's, applied on its thread) */
    /* claimed first: the tick must not present it again meanwhile */
    __atomic_store_n(&nv2a_ext_presented, __atomic_load_n(&nv2a_ext_pictures, __ATOMIC_ACQUIRE),
                     __ATOMIC_RELEASE);
    pthread_mutex_lock(&s_present_mu);
    present_locked(pg, false);
    pthread_mutex_unlock(&s_present_mu);
    return true;
}

static void present_locked(PGRAPHState *pg, bool on_pfifo)
{
    PGRAPHVkState *r = pg->vk_renderer_state;
    VkCommandBuffer cmd;
    uint32_t idx;
    VkResult res;
    unsigned f;

    {
        /* NV2A_CAPTURE=prefix: the display image as PPM every 300 frames;
         * with NV2A_CAPTURE_FROM=<ms>,<n>, every frame for n frames from
         * <ms> after the first present instead (debugging flicker). */
        static int64_t t0;
        static long from = -2, count;
        if (from == -2) {
            const char *e = getenv("NV2A_CAPTURE_FROM");
            from = -1;
            if (e && sscanf(e, "%ld,%ld", &from, &count) != 2)
                from = -1;
        }
        if (!t0)
            t0 = g_get_monotonic_time();
        if (__atomic_load_n(&s_f9_frames, __ATOMIC_ACQUIRE) > 0) {
            capture_display(pg);
            __atomic_sub_fetch(&s_f9_frames, 1, __ATOMIC_RELEASE);
        } else if (from >= 0) {
            if ((g_get_monotonic_time() - t0) / 1000 >= from && count > 0) {
                count--;
                capture_display(pg);
            }
        } else {
            static int every;   /* NV2A_CAPTURE_EVERY: frames between captures (300) */
            if (!every) {
                const char *e = getenv("NV2A_CAPTURE_EVERY");
                every = e && atoi(e) > 0 ? atoi(e) : 300;
            }
            if (s_frame % (unsigned)every == (unsigned)every - 1)
                capture_display(pg);
        }
    }
    if (s_surface == VK_NULL_HANDLE)
        return;
    if (!s_pool) {
        create_frame_resources(r);
        fps_init(r);
    }
    fps_track();
    if (s_resized || !s_swapchain) {
        s_resized = false;
        if (!create_swapchain(r))
            return;
    }
    /* Asked every frame (the host's per-frame work runs there: the mouse's
     * presses, input for its menus), whichever renderer has the picture. */
    bool overlay = nv2a_overlay_wanted();
    if (overlay && (r->display.image != VK_NULL_HANDLE ||
                    __atomic_load_n(&s_ext_image, __ATOMIC_ACQUIRE) != VK_NULL_HANDLE)) {
        extern volatile int nv2a_vblank_hold;
        nv2a_vblank_hold = 1;
        overlay_run(pg);
        nv2a_vblank_hold = 0;
        if (s_resized || !s_swapchain) {
            s_resized = false;
            if (!create_swapchain(r))
                return;
        }
    }
    /* Quality settings, from the overlay or the game's own menu. */
    if (on_pfifo && __atomic_exchange_n(&s_quality_set, 0, __ATOMIC_ACQ_REL))
        pgraph_vk_apply_quality(container_of(pg, NV2AState, pgraph),
                                s_q_scale, s_q_aniso, s_q_shadow);

    f = s_frame++ % FRAMES_IN_FLIGHT;
    vkWaitForFences(r->device, 1, &s_fence[f], VK_TRUE, UINT64_MAX);

    res = vkAcquireNextImageKHR(r->device, s_swapchain, 0, s_acquired[f],
                                VK_NULL_HANDLE, &idx);
    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        s_resized = true;
        return;
    }
    if (res == VK_NOT_READY || res == VK_TIMEOUT) {
        s_lat_dropped++;
        return;   /* every image queued: drop this frame rather than wait */
    }
    if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
        return;
    vkResetFences(r->device, 1, &s_fence[f]);

    cmd = s_cmd[f];
    vkResetCommandBuffer(cmd, 0);
    {
        VkCommandBufferBeginInfo bi = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };
        vkBeginCommandBuffer(cmd, &bi);
    }

    barrier(cmd, s_images[idx], VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    {
        VkClearColorValue black = { .float32 = { 0, 0, 0, 1 } };
        if (getenv("NV2A_TEST_COLOR"))      /* letterbox in blue: is this the window? */
            black.float32[2] = 0.6f;
        VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        vkCmdClearColorImage(cmd, s_images[idx], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             &black, 1, &range);
    }

    nv2a_host_presents++;
    int pw, ph;
    bool ext;
    unsigned pics_now = __atomic_load_n(&nv2a_ext_pictures, __ATOMIC_ACQUIRE);
    VkImage picture = picture_source(r, &pw, &ph, &ext);
    if (ext)
        __atomic_store_n(&nv2a_ext_presented, pics_now, __ATOMIC_RELEASE);
    if (picture == VK_NULL_HANDLE)
        nv2a_host_present_blank++;
    if (picture != VK_NULL_HANDLE && pw && ph && !s_blank) {
        /* Fit the picture to the window at the display's aspect ratio. */
        int32_t ww = s_extent.width, wh = s_extent.height;
        int32_t dw = ww, dh = s_aspect > 0 ? (int32_t)(ww / s_aspect) : wh;
        if (dh > wh) {
            dh = wh;
            dw = (int32_t)(wh * s_aspect);
        }
        int32_t x0 = (ww - dw) / 2, y0 = (wh - dh) / 2;
        VkImageBlit blit = {
            .srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
            /* Bottom-up: xemu's display shader writes GL row order (its GL
             * frontend flips again on the way out); the blit undoes it. */
            .srcOffsets = { { 0, ph, 0 }, { pw, 0, 1 } },
            .dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
            .dstOffsets = { { x0, y0, 0 }, { x0 + dw, y0 + dh, 1 } },
        };

        if (!ext)
            barrier(cmd, picture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT);
        else   /* the other renderer's frame, submitted before: wait for its writes */
            barrier(cmd, picture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_ACCESS_MEMORY_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

        /* Halve while more than 2:1 would be left for the final blit. */
        int n = 0;
        uint32_t sw = pw, sh = ph;
        while (n < MAX_HALVINGS && sw / 2 >= (uint32_t)dw && sh / 2 >= (uint32_t)dh &&
               sw / 2 > 0 && sh / 2 > 0) {
            sw /= 2;
            sh /= 2;
            n++;
        }
        halving_images(r, n, pw, ph);
        VkImage src = picture;
        uint32_t cw = pw, ch = ph;
        for (int i = 0; i < n && s_half[i]; i++) {
            barrier(cmd, s_half[i], VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
                    VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT);
            VkImageBlit half = {
                .srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
                .srcOffsets = { { 0, 0, 0 }, { (int32_t)(s_half_w[i] * 2), (int32_t)(s_half_h[i] * 2), 1 } },
                .dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
                .dstOffsets = { { 0, 0, 0 }, { (int32_t)s_half_w[i], (int32_t)s_half_h[i], 1 } },
            };
            vkCmdBlitImage(cmd, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           s_half[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                           &half, VK_FILTER_LINEAR);
            barrier(cmd, s_half[i], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT,
                    VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT);
            src = s_half[i];
            cw = s_half_w[i];
            ch = s_half_h[i];
        }
        /* The display image is bottom up; an external picture top down. */
        blit.srcOffsets[0] = (VkOffset3D){ 0, ext ? 0 : (int32_t)ch, 0 };
        blit.srcOffsets[1] = (VkOffset3D){ (int32_t)cw, ext ? (int32_t)ch : 0, 1 };
        vkCmdBlitImage(cmd, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       s_images[idx], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                       &blit, VK_FILTER_LINEAR);
        if (!ext)
            barrier(cmd, picture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }

    if (s_show_fps && s_ovl_ptr[f]) {
        uint32_t w, h;
        fps_draw(f, &w, &h);
        vmaFlushAllocation(r->allocator, s_ovl_mem[f], 0, VK_WHOLE_SIZE);
        barrier(cmd, s_images[idx], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT);
        VkBufferImageCopy region = {
            .bufferRowLength = w,
            .imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
            .imageOffset = { 0, 0, 0 },
            .imageExtent = { w, h, 1 },
        };
        vkCmdCopyBufferToImage(cmd, s_ovl_buf[f], s_images[idx],
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }

    barrier(cmd, s_images[idx], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ACCESS_TRANSFER_WRITE_BIT, 0,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    vkEndCommandBuffer(cmd);

    {
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        VkSubmitInfo si = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &s_acquired[f],
            .pWaitDstStageMask = &wait_stage,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &s_render_done[idx],
        };
        nv2a_vk_queue_lock();
        VK_CHECK(vkQueueSubmit(r->queue, 1, &si, s_fence[f]));
    }
    {
        VkPresentInfoKHR pi = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &s_render_done[idx],
            .swapchainCount = 1,
            .pSwapchains = &s_swapchain,
            .pImageIndices = &idx,
        };
        res = vkQueuePresentKHR(r->queue, &pi);
        nv2a_vk_queue_unlock();
        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
            s_resized = true;
    }
    lat_note(ext);
}

void nv2a_host_vk_finalize(PGRAPHState *pg)
{
    PGRAPHVkState *r = pg->vk_renderer_state;

    nv2a_vk_queue_lock();

    vkDeviceWaitIdle(r->device);

    nv2a_vk_queue_unlock();
    destroy_swapchain(r);
    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        if (s_fence[i]) vkDestroyFence(r->device, s_fence[i], NULL);
        if (s_acquired[i]) vkDestroySemaphore(r->device, s_acquired[i], NULL);
    }
    for (int i = 0; i < MAX_HALVINGS; i++)
        if (s_half[i])
            vmaDestroyImage(r->allocator, s_half[i], s_half_mem[i]);
    for (int i = 0; i < FRAMES_IN_FLIGHT; i++)
        if (s_ovl_buf[i])
            vmaDestroyBuffer(r->allocator, s_ovl_buf[i], s_ovl_mem[i]);
    if (s_pool)
        vkDestroyCommandPool(r->device, s_pool, NULL);
    s_pool = VK_NULL_HANDLE;
    if (s_surface)
        vkDestroySurfaceKHR(s_instance, s_surface, NULL);
    s_surface = VK_NULL_HANDLE;
}
