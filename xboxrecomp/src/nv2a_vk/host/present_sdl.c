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
#ifdef __ANDROID__
#include <SDL_syswm.h>   /* (the window's ANativeWindow: its Vulkan surface) */
#include <SDL_system.h>
#include <android/native_window_jni.h>
#endif

#define FRAMES_IN_FLIGHT 2
#define MAX_SWAP_IMAGES 8

static SDL_Window *s_window;
void nv2a_window_made(void *sdl_window);
static double s_aspect = 4.0 / 3.0;
static int s_vsync;                /* 0 off, 1 mailbox, 2 adaptive, 3 FIFO */
static volatile bool s_resized;

/* A display-mode change (windowed, fullscreen, borderless) is made on the window's thread while frames are
 * presented from others. With V-Sync off, presenting immediately right through the change hard-froze a whole
 * system (a GTX 1060 laptop on driver 581.80 with an external screen, ngb-recomp #227; with V-Sync on it never did).
 * So the window's thread raises s_switching and waits (up to 200 ms) for a present or swapchain under way to finish
 * (s_in_present), presents and swapchains wait until it is done, and the swapchains made in the second after it (the new size comes
 * in a few events) are FIFO, which waits for the display; then the chosen mode is back. */
static int s_switching, s_in_present;
static int64_t s_fifo_until;   /* monotonic us: FIFO until then, V-Sync off (0: no hold) */
static bool s_fifo_held;       /* the last swapchain asked for was FIFO for the hold, not by the setting */

static VkInstance s_instance;
#ifdef __APPLE__
static int s_surface_pending;
#endif
static VkSurfaceKHR s_surface = VK_NULL_HANDLE;
static volatile bool s_surface_lost;   /* acquire or present said so: the window went away */
#ifdef __ANDROID__
static ANativeWindow *s_surface_window;   /* the window s_surface is for, held (android_surface_current) */
#endif
static VkSwapchainKHR s_swapchain = VK_NULL_HANDLE;
static VkImage s_images[MAX_SWAP_IMAGES];
static VkSemaphore s_render_done[MAX_SWAP_IMAGES];
static uint32_t s_image_count;
static VkFormat s_format;
static VkExtent2D s_extent;
static bool s_system_turns;   /* the system turns the pictures for the display (swapchain_create) */
static bool s_swapchain_color;   /* its images can be drawn into (the overlay picture, layer_draw) */

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

static void external_show(VkImage image, uint32_t width, uint32_t height);

/* Frame interpolation: the pictures shown (external_show), the longest gap between two in the window (us), and
 * whether any was made between frames (set_external_at): the counter then counts what is shown. */
static volatile unsigned s_shown;
static volatile int64_t s_shown_gap_us;
static volatile int s_interp_seen;

/* Frame interpolation (the D3D renderer's NGB_INTERP): pictures held to be
 * shown at set times (nv2a_present_set_external_at, in the order given) by
 * the pacer thread. Every picture is shown under s_show_mu, the held ones
 * taken under it too: a picture shown now, or a flush, shows the held ones
 * first, so the order never changes; a NULL (the renderer going) drops them.
 * Lock order: s_show_mu, then s_hold_mu. */
#define HOLD_MAX 8
static pthread_mutex_t s_show_mu = PTHREAD_MUTEX_INITIALIZER;
static SDL_mutex *s_hold_mu;
static SDL_cond *s_hold_cv;
static struct { VkImage img; uint32_t w, h; int64_t at; } s_hold[HOLD_MAX];
static int s_hold_n;
static pthread_once_t s_hold_once = PTHREAD_ONCE_INIT;

static int hold_pacer(void *unused);
static void hold_init(void)
{
    s_hold_mu = SDL_CreateMutex();
    s_hold_cv = SDL_CreateCond();
}

/* (s_show_mu held) the first held picture, if due by `by` (or drop: none shown) */
static bool show_first_locked(int64_t by, bool drop)
{
    SDL_LockMutex(s_hold_mu);
    if (!s_hold_n || s_hold[0].at > by) {
        SDL_UnlockMutex(s_hold_mu);
        return false;
    }
    VkImage img = s_hold[0].img;
    uint32_t w = s_hold[0].w, h = s_hold[0].h;
    memmove(&s_hold[0], &s_hold[1], sizeof s_hold[0] * (size_t)--s_hold_n);
    SDL_UnlockMutex(s_hold_mu);
    if (!drop)
        external_show(img, w, h);
    return true;
}

/* (s_show_mu held) all held pictures, now */
static void show_held_locked(bool drop)
{
    while (show_first_locked(INT64_MAX, drop)) {
    }
}

void nv2a_present_flush_held(void)
{
    pthread_once(&s_hold_once, hold_init);
    pthread_mutex_lock(&s_show_mu);
    show_held_locked(false);
    pthread_mutex_unlock(&s_show_mu);
}

static int hold_pacer(void *unused)
{
    (void)unused;
    SDL_LockMutex(s_hold_mu);
    for (;;) {
        if (!s_hold_n) {
            SDL_CondWait(s_hold_cv, s_hold_mu);
            continue;
        }
        int64_t left = s_hold[0].at - g_get_monotonic_time();
        if (left > 1500) {   /* the last of it in short sleeps: the wait's own timer is in milliseconds */
            SDL_CondWaitTimeout(s_hold_cv, s_hold_mu, (Uint32)(left / 1000 - 1));
            continue;
        }
        SDL_UnlockMutex(s_hold_mu);
        if (left > 0) {
            SDL_Delay(0);
        } else {
            pthread_mutex_lock(&s_show_mu);
            show_first_locked(g_get_monotonic_time(), false);   /* (none if a newer picture took it meanwhile) */
            pthread_mutex_unlock(&s_show_mu);
        }
        SDL_LockMutex(s_hold_mu);
    }
    return 0;
}

static int s_refresh_hz;
int nv2a_present_refresh_hz(void)
{
    return __atomic_load_n(&s_refresh_hz, __ATOMIC_RELAXED);
}

int64_t nv2a_present_now_us(void)
{
    return g_get_monotonic_time();
}

void nv2a_present_set_external_at(VkImage image, uint32_t width, uint32_t height, int64_t at_us)
{
    static bool pacer;   /* (under s_show_mu) started with the first held picture */
    pthread_once(&s_hold_once, hold_init);
    pthread_mutex_lock(&s_show_mu);
    if (!pacer) {
        pacer = true;
        SDL_DetachThread(SDL_CreateThread(hold_pacer, "present-pacer", NULL));
    }
    for (;;) {   /* full: the first shown now */
        SDL_LockMutex(s_hold_mu);
        bool full = s_hold_n == HOLD_MAX;
        SDL_UnlockMutex(s_hold_mu);
        if (!full)
            break;
        show_first_locked(INT64_MAX, false);
    }
    __atomic_store_n(&s_interp_seen, 1, __ATOMIC_RELAXED);
    SDL_LockMutex(s_hold_mu);
    if (s_hold_n && at_us < s_hold[s_hold_n - 1].at)
        at_us = s_hold[s_hold_n - 1].at;   /* (in order) */
    s_hold[s_hold_n].img = image;
    s_hold[s_hold_n].w = width;
    s_hold[s_hold_n].h = height;
    s_hold[s_hold_n].at = at_us;
    s_hold_n++;
    SDL_CondSignal(s_hold_cv);
    SDL_UnlockMutex(s_hold_mu);
    pthread_mutex_unlock(&s_show_mu);
}

void nv2a_present_set_external(VkImage image, uint32_t width, uint32_t height)
{
    pthread_once(&s_hold_once, hold_init);
    pthread_mutex_lock(&s_show_mu);
    show_held_locked(image == VK_NULL_HANDLE);
    external_show(image, width, height);
    pthread_mutex_unlock(&s_show_mu);
}

static void external_show(VkImage image, uint32_t width, uint32_t height)
{
    static int show_log = -1;   /* NV2A_SHOW_LOG=1: the time since the last picture shown, for each */
    if (show_log < 0)
        show_log = getenv("NV2A_SHOW_LOG") != NULL;
    if (image) {
        static int64_t last;   /* (under s_show_mu) */
        int64_t t = g_get_monotonic_time();
        if (show_log)
            fprintf(stderr, "[SHOW] %+.2f ms\n", last ? (t - last) / 1000.0 : 0.0);
        if (last && t - last > __atomic_load_n(&s_shown_gap_us, __ATOMIC_RELAXED))
            __atomic_store_n(&s_shown_gap_us, t - last, __ATOMIC_RELAXED);
        __atomic_add_fetch(&s_shown, 1, __ATOMIC_RELAXED);
        last = t;
    }
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
/* (one more: the picture copied at its own size, for the area filter below when no halving is needed) */
#define FULL_COPY MAX_HALVINGS
static VkImage s_half[MAX_HALVINGS + 1];
static VmaAllocation s_half_mem[MAX_HALVINGS + 1];
static VkImageView s_half_view[MAX_HALVINGS + 1];
static uint32_t s_half_w[MAX_HALVINGS + 1], s_half_h[MAX_HALVINGS + 1];

static void level_image(PGRAPHVkState *r, int i, bool want, uint32_t hw, uint32_t hh);
static void area_forget(int i);

/* n halvings of a w x h picture, and the full-size copy if full */
static void halving_images(PGRAPHVkState *r, int n, uint32_t w, uint32_t h, bool full)
{
    for (int i = 0; i < MAX_HALVINGS; i++)
        level_image(r, i, i < n, w >> (i + 1), h >> (i + 1));
    level_image(r, FULL_COPY, full, w, h);
}

static void level_image(PGRAPHVkState *r, int i, bool want, uint32_t hw, uint32_t hh)
{
    if (s_half[i] && (!want || s_half_w[i] != hw || s_half_h[i] != hh)) {
        nv2a_vk_queue_lock();
        vkDeviceWaitIdle(r->device);
        nv2a_vk_queue_unlock();
        if (s_half_view[i])
            vkDestroyImageView(r->device, s_half_view[i], NULL);
        s_half_view[i] = VK_NULL_HANDLE;
        area_forget(i);   /* (a new view may get the old one's handle: its set written again regardless) */
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
                     VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };
        VmaAllocationCreateInfo ai = { .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE };
        if (vmaCreateImage(r->allocator, &ci, &ai, &s_half[i], &s_half_mem[i],
                           NULL) != VK_SUCCESS) {
            s_half[i] = VK_NULL_HANDLE;
            return;
        }
        VkImageViewCreateInfo vi = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, .image = s_half[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D, .format = VK_FORMAT_R8G8B8A8_UNORM,
            .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
        };
        if (vkCreateImageView(r->device, &vi, NULL, &s_half_view[i]) != VK_SUCCESS)
            s_half_view[i] = VK_NULL_HANDLE;
        s_half_w[i] = hw;
        s_half_h[i] = hh;
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
static pthread_mutex_t s_fps_text_mu = PTHREAD_MUTEX_INITIALIZER;   /* (read on the title's thread too) */

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
        static unsigned last_shown;
        unsigned shown = __atomic_load_n(&s_shown, __ATOMIC_RELAXED), pics = shown - last_shown;
        last_shown = shown;
        int64_t gap = __atomic_exchange_n(&s_shown_gap_us, 0, __ATOMIC_RELAXED);
        unsigned count = frames;
        double worst = max_ms;
        if (__atomic_exchange_n(&s_interp_seen, 0, __ATOMIC_RELAXED)) {   /* interpolating: the pictures shown */
            count = pics;
            worst = gap / 1000.0;
        }
        double fps = count / secs;
        pthread_mutex_lock(&s_fps_text_mu);
        snprintf(s_fps_text, sizeof s_fps_text, "%.0f FPS  %.1f ms  max %.1f",
                 fps, count ? secs * 1000.0 / count : 0.0, worst);
        pthread_mutex_unlock(&s_fps_text_mu);
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

/* ---- A picture over the whole window (a title's touch controls) ----
 * A title may give one (nv2a_overlay_picture; by default none): premultiplied BGRA of any size, blended over
 * each frame across the whole window -- the bars beside the game's picture too -- by one draw of its own
 * after the frame's copy. Made again only when the title's serial says it changed; nothing is made, nor
 * anything kept for it, while the title wants none (nv2a_layer_wanted). */
__attribute__((weak)) bool nv2a_layer_wanted(void)
{
    return false;
}

/* Where the picture just given differs from the one given before, as boxes (x0, y0, x1, y1); -1: anywhere */
__attribute__((weak)) int nv2a_overlay_changed(int (*box)[4], int max)
{
    (void)box;
    (void)max;
    return -1;
}

__attribute__((weak)) bool nv2a_overlay_picture(uint32_t win_w, uint32_t win_h, uint32_t *px, size_t max,
                                                uint32_t *ow, uint32_t *oh, unsigned *serial)
{
    (void)win_w; (void)win_h; (void)px; (void)max; (void)ow; (void)oh; (void)serial;
    return false;
}

/* (a title that draws it at the window's size: a 2400 x 1080 phone's whole, a 3200 x 1440 one's at three quarters) */
#define LAYER_MAX_PX (2560u * 1440u)
static struct {
    int state;                          /* 0 not tried, 1 ready, -1 not possible */
    VkBuffer buf[FRAMES_IN_FLIGHT];
    VmaAllocation buf_mem[FRAMES_IN_FLIGHT];
    void *ptr[FRAMES_IN_FLIGHT];
    VkImage img;
    VmaAllocation img_mem;
    VkImageView view;
    uint32_t w, h;
    bool valid;                         /* it holds a picture (and is in SHADER_READ_ONLY) */
    unsigned serial;
    VkSampler sampler;
    VkDescriptorSetLayout dsl;
    VkPipelineLayout layout;
    VkDescriptorPool pool;
    VkDescriptorSet set;
    VkPipeline pipe;
    VkFormat pipe_format;
    VkImageView views[MAX_SWAP_IMAGES];
    VkSwapchainKHR views_of;
} s_layer;

static void layer_drop_views(PGRAPHVkState *r)
{
    for (int i = 0; i < MAX_SWAP_IMAGES; i++)
        if (s_layer.views[i]) {
            vkDestroyImageView(r->device, s_layer.views[i], NULL);
            s_layer.views[i] = VK_NULL_HANDLE;
        }
    s_layer.views_of = VK_NULL_HANDLE;
}

static bool layer_init(PGRAPHVkState *r)
{
    if (s_layer.state)
        return s_layer.state > 0;
    s_layer.state = -1;
    nv2a_vk_shared sh;   /* (dynamic rendering on, core or extension: not a loader's stub for it) */
    if (nv2a_vk_share(&sh) != 1 || !sh.features13 || !vkCmdBeginRendering)
        return false;
    for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
        VkBufferCreateInfo bi = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = (VkDeviceSize)LAYER_MAX_PX * 4,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        };
        VmaAllocationCreateInfo ai = {
            .usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        };
        VmaAllocationInfo info;
        if (vmaCreateBuffer(r->allocator, &bi, &ai, &s_layer.buf[i], &s_layer.buf_mem[i], &info) != VK_SUCCESS)
            return false;
        s_layer.ptr[i] = info.pMappedData;
    }
    VkSamplerCreateInfo sci = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR, .minFilter = VK_FILTER_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    };
    VkDescriptorSetLayoutBinding b = {
        .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    };
    VkDescriptorSetLayoutCreateInfo dli = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .bindingCount = 1, .pBindings = &b,
    };
    VkDescriptorPoolSize ps = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 };
    VkDescriptorPoolCreateInfo pci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, .maxSets = 1, .poolSizeCount = 1, .pPoolSizes = &ps,
    };
    if (vkCreateSampler(r->device, &sci, NULL, &s_layer.sampler) != VK_SUCCESS ||
        vkCreateDescriptorSetLayout(r->device, &dli, NULL, &s_layer.dsl) != VK_SUCCESS ||
        vkCreateDescriptorPool(r->device, &pci, NULL, &s_layer.pool) != VK_SUCCESS)
        return false;
    VkDescriptorSetAllocateInfo dai = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .descriptorPool = s_layer.pool,
        .descriptorSetCount = 1, .pSetLayouts = &s_layer.dsl,
    };
    VkPipelineLayoutCreateInfo pli = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, .setLayoutCount = 1, .pSetLayouts = &s_layer.dsl,
    };
    if (vkAllocateDescriptorSets(r->device, &dai, &s_layer.set) != VK_SUCCESS ||
        vkCreatePipelineLayout(r->device, &pli, NULL, &s_layer.layout) != VK_SUCCESS)
        return false;
    s_layer.state = 1;
    return true;
}

/* The pipeline for the swapchain's format: one triangle over the window, the picture blended premultiplied */
static bool layer_pipeline(PGRAPHVkState *r)
{
    if (s_layer.pipe && s_layer.pipe_format == s_format)
        return true;
    if (s_layer.pipe)
        vkDestroyPipeline(r->device, s_layer.pipe, NULL);
    s_layer.pipe = VK_NULL_HANDLE;
    ShaderModuleInfo *vs = pgraph_vk_create_shader_module_from_glsl(r, VK_SHADER_STAGE_VERTEX_BIT,
        "#version 450\n"
        "layout(location = 0) out vec2 uv;\n"
        "void main() { vec2 p = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);\n"
        "  uv = p; gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0); }\n");
    ShaderModuleInfo *fs = pgraph_vk_create_shader_module_from_glsl(r, VK_SHADER_STAGE_FRAGMENT_BIT,
        "#version 450\n"
        "layout(set = 0, binding = 0) uniform sampler2D tex;\n"
        "layout(location = 0) in vec2 uv;\n"
        "layout(location = 0) out vec4 color;\n"
        "void main() { color = texture(tex, uv); }\n");
    if (!vs || !fs)
        return false;
    VkPipelineShaderStageCreateInfo st[2] = {
        { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_VERTEX_BIT,
          .module = vs->module, .pName = "main" },
        { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
          .module = fs->module, .pName = "main" },
    };
    VkPipelineVertexInputStateCreateInfo vi = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    VkPipelineInputAssemblyStateCreateInfo ia = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };
    VkPipelineViewportStateCreateInfo vp = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, .viewportCount = 1, .scissorCount = 1,
    };
    VkPipelineRasterizationStateCreateInfo rs = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL, .cullMode = VK_CULL_MODE_NONE, .lineWidth = 1.0f,
    };
    VkPipelineMultisampleStateCreateInfo ms = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };
    VkPipelineColorBlendAttachmentState ba = {
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE, .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = 0xF,
    };
    VkPipelineColorBlendStateCreateInfo cb = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, .attachmentCount = 1, .pAttachments = &ba,
    };
    VkDynamicState dyn[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo ds = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, .dynamicStateCount = 2, .pDynamicStates = dyn,
    };
    VkPipelineRenderingCreateInfo rinfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1, .pColorAttachmentFormats = &s_format,
    };
    VkGraphicsPipelineCreateInfo gi = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, .pNext = &rinfo,
        .stageCount = 2, .pStages = st, .pVertexInputState = &vi, .pInputAssemblyState = &ia,
        .pViewportState = &vp, .pRasterizationState = &rs, .pMultisampleState = &ms, .pColorBlendState = &cb,
        .pDynamicState = &ds, .layout = s_layer.layout,
    };
    VkResult res = vkCreateGraphicsPipelines(r->device, VK_NULL_HANDLE, 1, &gi, NULL, &s_layer.pipe);
    pgraph_vk_destroy_shader_module(r, vs);
    pgraph_vk_destroy_shader_module(r, fs);
    if (res != VK_SUCCESS) {
        s_layer.pipe = VK_NULL_HANDLE;
        return false;
    }
    s_layer.pipe_format = s_format;
    return true;
}

/* The picture's image, made again when its size changes (nothing in flight uses the old one then) */
static bool layer_image(PGRAPHVkState *r, uint32_t w, uint32_t h)
{
    if (s_layer.img && s_layer.w == w && s_layer.h == h)
        return true;
    if (s_layer.img) {
        nv2a_vk_queue_lock();
        vkDeviceWaitIdle(r->device);
        nv2a_vk_queue_unlock();
        vkDestroyImageView(r->device, s_layer.view, NULL);
        vmaDestroyImage(r->allocator, s_layer.img, s_layer.img_mem);
        s_layer.img = VK_NULL_HANDLE;
    }
    VkImageCreateInfo ii = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO, .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_B8G8R8A8_UNORM, .extent = { w, h, 1 }, .mipLevels = 1, .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT, .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    };
    VmaAllocationCreateInfo ai = { .usage = VMA_MEMORY_USAGE_AUTO };
    if (vmaCreateImage(r->allocator, &ii, &ai, &s_layer.img, &s_layer.img_mem, NULL) != VK_SUCCESS) {
        s_layer.img = VK_NULL_HANDLE;
        return false;
    }
    VkImageViewCreateInfo vi = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, .image = s_layer.img,
        .viewType = VK_IMAGE_VIEW_TYPE_2D, .format = VK_FORMAT_B8G8R8A8_UNORM,
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
    };
    if (vkCreateImageView(r->device, &vi, NULL, &s_layer.view) != VK_SUCCESS) {
        vmaDestroyImage(r->allocator, s_layer.img, s_layer.img_mem);
        s_layer.img = VK_NULL_HANDLE;
        return false;
    }
    VkDescriptorImageInfo di = { s_layer.sampler, s_layer.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    VkWriteDescriptorSet wr = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = s_layer.set, .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &di,
    };
    vkUpdateDescriptorSets(r->device, 1, &wr, 0, NULL);
    s_layer.w = w;
    s_layer.h = h;
    s_layer.valid = false;   /* (its contents still to come) */
    return true;
}

static void barrier(VkCommandBuffer cmd, VkImage img, VkImageLayout from,
                    VkImageLayout to, VkAccessFlags src, VkAccessFlags dst,
                    VkPipelineStageFlags src_stage, VkPipelineStageFlags dst_stage);

/* The swapchain image idx's view, to draw into (made again for a new swapchain) */
static VkImageView swap_view(PGRAPHVkState *r, uint32_t idx)
{
    if (s_layer.views_of != s_swapchain) {
        layer_drop_views(r);
        for (uint32_t i = 0; i < s_image_count; i++) {
            VkImageViewCreateInfo vi = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, .image = s_images[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D, .format = s_format,
                .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
            };
            if (vkCreateImageView(r->device, &vi, NULL, &s_layer.views[i]) != VK_SUCCESS)
                s_layer.views[i] = VK_NULL_HANDLE;
        }
        s_layer.views_of = s_swapchain;
    }
    return s_layer.views[idx];
}

/* Over the swapchain image idx (in TRANSFER_DST, and left there) */
static void layer_draw(PGRAPHVkState *r, VkCommandBuffer cmd, unsigned f, uint32_t idx)
{
    uint32_t w = 0, h = 0;
    if (!s_swapchain_color || !nv2a_layer_wanted() || !layer_init(r))
        return;
    /* (no picture in the image yet, or one of another size: drawn whatever the serial) */
    unsigned serial = s_layer.valid ? s_layer.serial : 0;
    if (!nv2a_overlay_picture(s_extent.width, s_extent.height, s_layer.ptr[f], LAYER_MAX_PX, &w, &h, &serial) ||
        !w || !h)
        return;
    if (s_layer.img && (s_layer.w != w || s_layer.h != h)) {
        s_layer.valid = false;   /* (this frame's buffer holds a picture of the old size: again next frame) */
        layer_image(r, w, h);
        return;
    }
    if (!layer_image(r, w, h) || !layer_pipeline(r)) {
        if (serial != s_layer.serial)
            s_layer.valid = false;   /* (a picture drawn and not sent: the next one whole) */
        return;
    }
    if (!s_layer.valid || serial != s_layer.serial) {   /* new contents: in this frame's buffer now */
        /* (only where it differs from the picture in the image, when the title says where) */
        enum { BOXES = 64 };
        int box[BOXES][4], n = s_layer.valid ? nv2a_overlay_changed(box, BOXES) : -1;
        VkBufferImageCopy c[BOXES];
        if (n < 0 || n > BOXES) {
            n = 1;
            box[0][0] = box[0][1] = 0;
            box[0][2] = (int)w;
            box[0][3] = (int)h;
        }
        uint32_t nc = 0;
        for (int i = 0; i < n; i++) {
            int x0 = box[i][0] < 0 ? 0 : box[i][0], y0 = box[i][1] < 0 ? 0 : box[i][1];
            int x1 = box[i][2] > (int)w ? (int)w : box[i][2], y1 = box[i][3] > (int)h ? (int)h : box[i][3];
            if (x0 >= x1 || y0 >= y1)
                continue;
            c[nc++] = (VkBufferImageCopy){
                .bufferOffset = 4 * ((VkDeviceSize)y0 * w + (VkDeviceSize)x0),
                .bufferRowLength = w,
                .imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
                .imageOffset = { x0, y0, 0 },
                .imageExtent = { (uint32_t)(x1 - x0), (uint32_t)(y1 - y0), 1 },
            };
        }
        if (nc) {
            vmaFlushAllocation(r->allocator, s_layer.buf_mem[f], 0, VK_WHOLE_SIZE);
            barrier(cmd, s_layer.img, s_layer.valid ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
            vkCmdCopyBufferToImage(cmd, s_layer.buf[f], s_layer.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, nc, c);
            barrier(cmd, s_layer.img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
        }
        s_layer.serial = serial;
        s_layer.valid = true;
    }
    if (!swap_view(r, idx))
        return;
    barrier(cmd, s_images[idx], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    VkRenderingAttachmentInfo ca = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO, .imageView = s_layer.views[idx],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD, .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    };
    VkRenderingInfo ri = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO, .renderArea = { { 0, 0 }, s_extent }, .layerCount = 1,
        .colorAttachmentCount = 1, .pColorAttachments = &ca,
    };
    vkCmdBeginRendering(cmd, &ri);
    VkViewport v = { 0, 0, (float)s_extent.width, (float)s_extent.height, 0, 1 };
    VkRect2D sc = { { 0, 0 }, s_extent };
    vkCmdSetViewport(cmd, 0, 1, &v);
    vkCmdSetScissor(cmd, 0, 1, &sc);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, s_layer.pipe);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, s_layer.layout, 0, 1, &s_layer.set, 0, NULL);
    vkCmdDraw(cmd, 3, 1, 0, 0);
    vkCmdEndRendering(cmd);
    barrier(cmd, s_images[idx], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
}

/* ---- The last step down to the window, by area ----
 *
 * After the halvings less than 2:1 is left, and a bilinear blit at, say,
 * 1.8:1 reads 2x2 pixels around each window pixel's centre: of the 1.8 picture
 * pixels a window pixel covers, some count fully, some partly, some not at all,
 * and which depends on where the pixel falls, so fine detail shimmers as the
 * camera moves. Here each window pixel is the average of the picture pixels
 * under it, each weighted by how much of it is covered. Upscaling keeps the
 * blit. NV2A_AREA_DOWNSCALE=0 turns it off. */
static struct {
    int state;                 /* 0 not tried, 1 ready, -1 unavailable */
    VkSampler sampler;
    VkDescriptorSetLayout dsl;
    VkPipelineLayout layout;
    VkDescriptorPool pool;
    VkDescriptorSet set[MAX_HALVINGS + 1];   /* one per level image */
    VkImageView bound[MAX_HALVINGS + 1];     /* the view each set holds */
    VkPipeline pipe;
    VkFormat pipe_format, failed_format;   /* (failed_format: VK_FORMAT_UNDEFINED, 0, when none failed) */
} s_area;

/* Level image i is going: its set is written again for whatever view comes next */
static void area_forget(int i)
{
    s_area.bound[i] = VK_NULL_HANDLE;
}

static bool area_init(PGRAPHVkState *r)
{
    if (s_area.state)
        return s_area.state > 0;
    s_area.state = -1;
    const char *e = getenv("NV2A_AREA_DOWNSCALE");
    if (e && *e == '0')
        return false;
    nv2a_vk_shared sh;
    if (nv2a_vk_share(&sh) != 1 || !sh.features13 || !vkCmdBeginRendering)
        return false;
    VkSamplerCreateInfo sci = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_NEAREST, .minFilter = VK_FILTER_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    };
    VkDescriptorSetLayoutBinding b = {
        .binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    };
    VkDescriptorSetLayoutCreateInfo dli = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .bindingCount = 1, .pBindings = &b,
    };
    VkDescriptorPoolSize ps = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_HALVINGS + 1 };
    VkDescriptorPoolCreateInfo pci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, .maxSets = MAX_HALVINGS + 1,
        .poolSizeCount = 1, .pPoolSizes = &ps,
    };
    VkPushConstantRange pc = { VK_SHADER_STAGE_FRAGMENT_BIT, 0, 8 * sizeof(float) };
    if (vkCreateSampler(r->device, &sci, NULL, &s_area.sampler) != VK_SUCCESS ||
        vkCreateDescriptorSetLayout(r->device, &dli, NULL, &s_area.dsl) != VK_SUCCESS ||
        vkCreateDescriptorPool(r->device, &pci, NULL, &s_area.pool) != VK_SUCCESS)
        return false;
    VkPipelineLayoutCreateInfo pli = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, .setLayoutCount = 1, .pSetLayouts = &s_area.dsl,
        .pushConstantRangeCount = 1, .pPushConstantRanges = &pc,
    };
    if (vkCreatePipelineLayout(r->device, &pli, NULL, &s_area.layout) != VK_SUCCESS)
        return false;
    for (int i = 0; i <= MAX_HALVINGS; i++) {
        VkDescriptorSetAllocateInfo dai = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .descriptorPool = s_area.pool,
            .descriptorSetCount = 1, .pSetLayouts = &s_area.dsl,
        };
        if (vkAllocateDescriptorSets(r->device, &dai, &s_area.set[i]) != VK_SUCCESS)
            return false;
    }
    s_area.state = 1;
    return true;
}

static bool area_pipeline(PGRAPHVkState *r)
{
    if (s_area.pipe && s_area.pipe_format == s_format)
        return true;
    if (s_area.failed_format == s_format)   /* (failed for this format: the blit, not a compile every frame) */
        return false;
    s_area.failed_format = s_format;
    if (s_area.pipe)
        vkDestroyPipeline(r->device, s_area.pipe, NULL);
    s_area.pipe = VK_NULL_HANDLE;
    ShaderModuleInfo *vs = pgraph_vk_create_shader_module_from_glsl(r, VK_SHADER_STAGE_VERTEX_BIT,
        "#version 450\n"
        "void main() { vec2 p = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);\n"
        "  gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0); }\n");
    /* a: the picture's size, the rectangle's corner in the window; b: the rectangle's size, bottom up (1) */
    ShaderModuleInfo *fs = pgraph_vk_create_shader_module_from_glsl(r, VK_SHADER_STAGE_FRAGMENT_BIT,
        "#version 450\n"
        "layout(set = 0, binding = 0) uniform sampler2D tex;\n"
        "layout(push_constant) uniform P { vec4 a; vec4 b; } p;\n"
        "layout(location = 0) out vec4 color;\n"
        "void main() {\n"
        "  vec2 size = p.a.xy, k = size / p.b.xy, o = gl_FragCoord.xy - p.a.zw;\n"
        "  vec2 lo = (o - 0.5) * k, hi = (o + 0.5) * k;\n"   /* the window pixel's span in picture pixels */
        "  ivec2 first = ivec2(floor(lo)), n = ivec2(ceil(hi)) - first, last = ivec2(size) - 1;\n"
        "  vec4 sum = vec4(0.0); float total = 0.0;\n"
        "  for (int y = 0; y < 8 && y < n.y; y++) {\n"
        "    float py = float(first.y + y), wy = min(hi.y, py + 1.0) - max(lo.y, py);\n"
        "    int ty = clamp(p.b.z > 0.5 ? last.y - (first.y + y) : first.y + y, 0, last.y);\n"
        "    for (int x = 0; x < 8 && x < n.x; x++) {\n"
        "      float px = float(first.x + x), w = wy * (min(hi.x, px + 1.0) - max(lo.x, px));\n"
        "      sum += w * texelFetch(tex, ivec2(clamp(first.x + x, 0, last.x), ty), 0);\n"
        "      total += w;\n"
        "    }\n"
        "  }\n"
        "  color = sum / max(total, 1e-6);\n"
        "}\n");
    if (!vs || !fs) {
        if (vs)
            pgraph_vk_destroy_shader_module(r, vs);
        if (fs)
            pgraph_vk_destroy_shader_module(r, fs);
        return false;
    }
    VkPipelineShaderStageCreateInfo st[2] = {
        { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_VERTEX_BIT,
          .module = vs->module, .pName = "main" },
        { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
          .module = fs->module, .pName = "main" },
    };
    VkPipelineVertexInputStateCreateInfo vi = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    VkPipelineInputAssemblyStateCreateInfo ia = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };
    VkPipelineViewportStateCreateInfo vp = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO, .viewportCount = 1, .scissorCount = 1,
    };
    VkPipelineRasterizationStateCreateInfo rs = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL, .cullMode = VK_CULL_MODE_NONE, .lineWidth = 1.0f,
    };
    VkPipelineMultisampleStateCreateInfo ms = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };
    VkPipelineColorBlendAttachmentState ba = { .colorWriteMask = 0xF };
    VkPipelineColorBlendStateCreateInfo cb = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, .attachmentCount = 1, .pAttachments = &ba,
    };
    VkDynamicState dyn[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo ds = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO, .dynamicStateCount = 2, .pDynamicStates = dyn,
    };
    VkPipelineRenderingCreateInfo rinfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1, .pColorAttachmentFormats = &s_format,
    };
    VkGraphicsPipelineCreateInfo gi = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO, .pNext = &rinfo,
        .stageCount = 2, .pStages = st, .pVertexInputState = &vi, .pInputAssemblyState = &ia,
        .pViewportState = &vp, .pRasterizationState = &rs, .pMultisampleState = &ms, .pColorBlendState = &cb,
        .pDynamicState = &ds, .layout = s_area.layout,
    };
    VkResult res = vkCreateGraphicsPipelines(r->device, VK_NULL_HANDLE, 1, &gi, NULL, &s_area.pipe);
    pgraph_vk_destroy_shader_module(r, vs);
    pgraph_vk_destroy_shader_module(r, fs);
    if (res != VK_SUCCESS) {
        s_area.pipe = VK_NULL_HANDLE;
        return false;
    }
    s_area.pipe_format = s_format;
    s_area.failed_format = VK_FORMAT_UNDEFINED;
    return true;
}

static void area_destroy(PGRAPHVkState *r)
{
    if (s_area.pipe)
        vkDestroyPipeline(r->device, s_area.pipe, NULL);
    if (s_area.layout)
        vkDestroyPipelineLayout(r->device, s_area.layout, NULL);
    if (s_area.pool)
        vkDestroyDescriptorPool(r->device, s_area.pool, NULL);   /* (its sets with it) */
    if (s_area.dsl)
        vkDestroyDescriptorSetLayout(r->device, s_area.dsl, NULL);
    if (s_area.sampler)
        vkDestroySampler(r->device, s_area.sampler, NULL);
    memset(&s_area, 0, sizeof s_area);
}

/* Whether the area filter can draw this frame (before any level image is made for it) */
static bool area_ready(PGRAPHVkState *r)
{
    return s_swapchain_color && area_init(r) && area_pipeline(r);
}

/* Level image lv (in TRANSFER_SRC, cw x ch, bottom up if flip) into the rectangle x0, y0, dw x dh of the
 * swapchain image idx (in TRANSFER_DST, and left there). False: nothing drawn, blit instead. */
static bool area_draw(PGRAPHVkState *r, VkCommandBuffer cmd, uint32_t idx, int lv, uint32_t cw, uint32_t ch,
                      bool flip, int32_t x0, int32_t y0, int32_t dw, int32_t dh)
{
    VkImageView dst = swap_view(r, idx);
    if (!dst || !s_half_view[lv])
        return false;
    if (s_area.bound[lv] != s_half_view[lv]) {   /* (a new image: made after the device went idle) */
        VkDescriptorImageInfo ii = { s_area.sampler, s_half_view[lv], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
        VkWriteDescriptorSet w = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = s_area.set[lv], .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .pImageInfo = &ii,
        };
        vkUpdateDescriptorSets(r->device, 1, &w, 0, NULL);
        s_area.bound[lv] = s_half_view[lv];
    }
    barrier(cmd, s_half[lv], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    barrier(cmd, s_images[idx], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    VkRenderingAttachmentInfo ca = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO, .imageView = dst,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD, .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    };
    VkRenderingInfo ri = {   /* (only the picture's rectangle: a tiled GPU loads and stores no more) */
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO, .renderArea = { { x0, y0 }, { (uint32_t)dw, (uint32_t)dh } },
        .layerCount = 1, .colorAttachmentCount = 1, .pColorAttachments = &ca,
    };
    vkCmdBeginRendering(cmd, &ri);
    VkViewport v = { (float)x0, (float)y0, (float)dw, (float)dh, 0, 1 };
    VkRect2D sc = { { x0, y0 }, { (uint32_t)dw, (uint32_t)dh } };
    vkCmdSetViewport(cmd, 0, 1, &v);
    vkCmdSetScissor(cmd, 0, 1, &sc);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, s_area.pipe);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, s_area.layout, 0, 1, &s_area.set[lv], 0, NULL);
    float pc[8] = { (float)cw, (float)ch, (float)x0, (float)y0, (float)dw, (float)dh, flip ? 1.0f : 0.0f, 0 };
    vkCmdPushConstants(cmd, s_area.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), pc);
    vkCmdDraw(cmd, 3, 1, 0, 0);
    vkCmdEndRendering(cmd);
    {
        static uint32_t said_w, said_h;
        if (said_w != cw || said_h != ch) {
            fprintf(stderr, "[PRESENT] %ux%u to %dx%d by area\n", cw, ch, dw, dh);
            said_w = cw, said_h = ch;
        }
    }
    barrier(cmd, s_images[idx], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    /* back for the next frame's write into it (a transfer, which then waits for this read) */
    barrier(cmd, s_half[lv], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_ACCESS_SHADER_READ_BIT, 0, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    return true;
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

/* Window, F11 out of fullscreen: the menus' mode to go back to (0 a window, 2 borderless). */
static int s_win_base;
/* F11 out of fullscreen to a window: its size (the menus' last, or the window's as it went fullscreen). A window
 * manager may not give the old size back: KWin, for one as wide as the display. */
static int s_keep_w, s_keep_h;

/* A framed window w x h (its inside; 0: as it is now), fitted into the display's usable area -- the
 * taskbar and panels left out -- with its frame (#282): a size bigger than that is made smaller, its shape
 * kept. Centered there when asked, or when made smaller. */
static void window_fit(SDL_Window *win, int w, int h, bool center)
{
    SDL_Rect r;
    int d = SDL_GetWindowDisplayIndex(win), top = 0, left = 0, bottom = 0, right = 0;
    bool as_is = w <= 0 || h <= 0;   /* (just out of fullscreen on Wayland, the size can still be the old one) */
    if (as_is)
        SDL_GetWindowSize(win, &w, &h);
    if (w <= 0 || h <= 0)
        return;
    if (SDL_GetDisplayUsableBounds(d < 0 ? 0 : d, &r) != 0) {
        if (!as_is)
            SDL_SetWindowSize(win, w, h);
        if (center)
            SDL_SetWindowPosition(win, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        return;
    }
    if (SDL_GetWindowBordersSize(win, &top, &left, &bottom, &right) != 0)
        top = left = bottom = right = 0;
    int mw = r.w - left - right, mh = r.h - top - bottom;
    if (mw > 0 && mh > 0 && (w > mw || h > mh)) {
        double k = (double)mw / w < (double)mh / h ? (double)mw / w : (double)mh / h;
        int fw = (int)(w * k), fh = (int)(h * k);
        fprintf(stderr, "nv2a: a %dx%d window is bigger than the display has room for (%dx%d): %dx%d\n",
                w, h, mw, mh, fw, fh);
        w = fw, h = fh, center = true;
    } else if (as_is) {
        return;              /* it fits: left as it is */
    }
    SDL_SetWindowSize(win, w, h);
    if (center)   /* (SDL places the window's inside; the frame goes around it) */
        SDL_SetWindowPosition(win, r.x + left + (mw - w) / 2, r.y + top + (mh - h) / 2);
}

/* ---- The player's mouse motions (#281) ----
 * SDL reports a motion whenever the pointer's place in the window changes, so also when the window
 * moves, changes size or mode under a pointer at rest, comes to the front or gets the pointer back, or
 * when the relative mode ends. Taken for the player's, such a motion moved a menu's cursor to the row
 * under the pointer (the next Left/Right changed that row) and made the mouse the device in use. They
 * are dropped here, ahead of every event watcher and the queue: a motion is the player's when the
 * window is as it was at the last one, nothing brought it or the pointer in since, and it moved.
 * (A real move that comes first after such a change is dropped too; the next one is taken.) */
typedef struct {
    int x, y, w, h, dw, dh, hz;
    Uint32 flags;
} mouse_geom;
static mouse_geom s_mouse_geom;
static int s_mouse_seen, s_mouse_fresh;

static void mouse_geom_of(Uint32 id, mouse_geom *g)
{
    SDL_Window *w = SDL_GetWindowFromID(id);
    SDL_DisplayMode m;
    memset(g, 0, sizeof *g);
    if (!w)
        return;
    SDL_GetWindowPosition(w, &g->x, &g->y);
    SDL_GetWindowSize(w, &g->w, &g->h);
    g->flags = SDL_GetWindowFlags(w) & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP |
                                        SDL_WINDOW_BORDERLESS | SDL_WINDOW_MAXIMIZED |
                                        SDL_WINDOW_MINIMIZED);
    int d = SDL_GetWindowDisplayIndex(w);
    if (d >= 0 && SDL_GetCurrentDisplayMode(d, &m) == 0)
        g->dw = m.w, g->dh = m.h, g->hz = m.refresh_rate;
}

static int SDLCALL mouse_filter(void *unused, SDL_Event *ev)
{
    (void)unused;
    if (ev->type == SDL_WINDOWEVENT) {
        switch (ev->window.event) {
        case SDL_WINDOWEVENT_ENTER: case SDL_WINDOWEVENT_FOCUS_GAINED:
        case SDL_WINDOWEVENT_SHOWN: case SDL_WINDOWEVENT_RESTORED:
        case SDL_WINDOWEVENT_MAXIMIZED:
            s_mouse_fresh = 1;
            break;
        default:
            break;
        }
        return 1;
    }
    if (ev->type != SDL_MOUSEMOTION || ev->motion.which == SDL_TOUCH_MOUSEID ||
        ev->motion.which == NV2A_MOUSEID_SCRIPTED)
        return 1;
    if (SDL_GetRelativeMouseMode()) {   /* the camera's: all of them; the first one after is not */
        s_mouse_fresh = 1;
        return 1;
    }
    mouse_geom g;
    mouse_geom_of(ev->motion.windowID, &g);
    int player = s_mouse_seen && !s_mouse_fresh && !memcmp(&g, &s_mouse_geom, sizeof g) &&
                 (ev->motion.xrel || ev->motion.yrel);
    s_mouse_geom = g;
    s_mouse_seen = 1;
    s_mouse_fresh = 0;
    if (!player) {
        static int log = -1;
        if (log < 0)
            log = getenv("NGB_MOUSE_LOG") != NULL;
        if (log)
            fprintf(stderr, "[MOUSE] motion %d,%d rel %d,%d not the player's (window %d,%d %dx%d)\n",
                    ev->motion.x, ev->motion.y, ev->motion.xrel, ev->motion.yrel, g.x, g.y, g.w, g.h);
    }
    return player;
}

void nv2a_mouse_filter_install(void)
{
    s_mouse_seen = 0;
    SDL_SetEventFilter(mouse_filter, NULL);
}

int nv2a_window_create(const char *title, int width, int height,
                       int fullscreen, double aspect, int vsync)
{
    /* Native Wayland when the session has it, X11 otherwise. SDL2 picks
     * X11 first, so on a Wayland desktop the game went through XWayland.
     * SDL_VIDEODRIVER still overrides. (Windows and Android have their one driver.) */
#if defined(__linux__) && !defined(__ANDROID__)
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
    nv2a_mouse_filter_install();   /* (before the window: setting it empties the event queue) */
    if (fullscreen == 0) {   /* made at a size that fits already: a window window managers find too big gets maximized */
        SDL_Rect r;
        if (SDL_GetDisplayUsableBounds(0, &r) == 0 && r.w > 0 && r.h > 0 && (width > r.w || height > r.h)) {
            double k = (double)r.w / width < (double)r.h / height ? (double)r.w / width : (double)r.h / height;
            width = (int)(width * k), height = (int)(height * k);
        }
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
    /* No typing in the game: SDL starts with text input on, which on Windows
     * gives the window an input method. A keyboard layout with one (Japanese,
     * Chinese, Korean) could open its candidate box over the game, or hang
     * the window as "Not responding" when the input method stalled. */
    SDL_StopTextInput();
    nv2a_window_made(s_window);   /* (the host's icon on it) */
    s_win_base = fullscreen == 2 ? 2 : 0;
    s_keep_w = width, s_keep_h = height;   /* (fitted above) */
    if (fullscreen == 2)
        borderless_apply(s_window);
    else if (fullscreen == 0) {
        if (SDL_GetWindowFlags(s_window) & SDL_WINDOW_MAXIMIZED)
            SDL_RestoreWindow(s_window);
        window_fit(s_window, width, height, true);   /* now with its frame */
    }
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

int nv2a_window_mode(int *width, int *height)
{
    if (!s_window)
        return -1;
    Uint32 flags = SDL_GetWindowFlags(s_window);
    if (flags & SDL_WINDOW_FULLSCREEN)
        return 1;
    if (flags & SDL_WINDOW_BORDERLESS)
        return 2;
    SDL_GetWindowSize(s_window, width, height);
    return 0;
}

int nv2a_window_extent(int *width, int *height)
{
    int w = (int)s_extent.width, h = (int)s_extent.height;
    if (w <= 0 || h <= 0)
        return -1;
    *width = w;
    *height = h;
    return 0;
}

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

/* For a renderer that letters the counter into its own picture (ngb-recomp's Direct3D one, src/fps_text.c): whether
 * it is on and its text; once it has, nv2a_fps_drawn_outside is set and the corner copy here stays off. */
bool nv2a_present_show_fps(void)
{
    return s_show_fps;
}

void nv2a_present_fps_text(char *buf, size_t n)
{
    pthread_mutex_lock(&s_fps_text_mu);
    snprintf(buf, n, "%s", s_fps_text);
    pthread_mutex_unlock(&s_fps_text_mu);
}

volatile int nv2a_fps_drawn_outside;

static volatile bool s_blank;

void nv2a_present_set_blank(bool blank)
{
    s_blank = blank;
}

/* Each pump of the window's events, on its thread (weak; a host defines it
 * for work that thread must do: the mouse's capture). */
__attribute__((weak)) void nv2a_window_tick(void) {}

__attribute__((weak)) void xbox_InputHotplugPoll(void) {}

/* Around a display-mode change, on the window's thread (s_switching above). */
static void mode_switch_begin(void)
{
    __atomic_store_n(&s_switching, 1, __ATOMIC_SEQ_CST);
    int i = 0;
    for (; i < 200 && __atomic_load_n(&s_in_present, __ATOMIC_SEQ_CST); i++)
        SDL_Delay(1);
    if (i == 200)
        fprintf(stderr, "nv2a: display mode change with a present still under way after 200 ms\n");
    SDL_Delay(20);   /* (what is queued reaches the display) */
}

static void mode_switch_end(void)
{
    __atomic_store_n(&s_fifo_until, g_get_monotonic_time() + 1000000, __ATOMIC_RELEASE);
    s_resized = true;
    __atomic_store_n(&s_switching, 0, __ATOMIC_SEQ_CST);
}

/* Every display mode change, on the window's thread: mode 0 a window w x h (0: its size now), 1
 * fullscreen, 2 borderless. A window the player maximized (a double click on its title, Win+Up) is
 * made normal first: left maximized, a window asked for stayed the display's size (#282). */
static void window_apply(int mode, int w, int h)
{
    Uint32 was = SDL_GetWindowFlags(s_window);
    bool fs = was & SDL_WINDOW_FULLSCREEN;
    mode_switch_begin();
    if (mode == 1 && !fs && !(was & (SDL_WINDOW_BORDERLESS | SDL_WINDOW_MAXIMIZED)))
        SDL_GetWindowSize(s_window, &s_keep_w, &s_keep_h);
    if (mode != 1 && fs)
        SDL_SetWindowFullscreen(s_window, 0);
    if (mode == 0 && fs && (w <= 0 || h <= 0) && s_keep_w > 0 && s_keep_h > 0)
        w = s_keep_w, h = s_keep_h;
    if (mode != 1 && (SDL_GetWindowFlags(s_window) & SDL_WINDOW_MAXIMIZED))
        SDL_RestoreWindow(s_window);
    if (mode != 2)
        SDL_SetWindowBordered(s_window, SDL_TRUE);
    if (mode == 0)
    {
        window_fit(s_window, w, h, w > 0 && h > 0 && !fs);
        if (w > 0 && h > 0)   /* (just set: SDL has it; as it was, it can be the fullscreen size still on Wayland) */
            SDL_GetWindowSize(s_window, &s_keep_w, &s_keep_h);
    }
    else if (mode == 2)
        borderless_apply(s_window);
    else if (!fs)
        SDL_SetWindowFullscreen(s_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (mode != 1)
        s_win_base = mode;
    mode_switch_end();
}

__attribute__((weak)) void nv2a_window_mode_changed(int mode)
{
    (void)mode;
}

__attribute__((weak)) void nv2a_window_made(void *sdl_window)
{
    (void)sdl_window;
}

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
    {   /* the display's refresh rate, now and then (nv2a_present_refresh_hz: other threads) */
        static int64_t next;
        int64_t now = g_get_monotonic_time();
        if (s_window && now >= next) {
            SDL_DisplayMode m;
            int d = SDL_GetWindowDisplayIndex(s_window);
            if (SDL_GetCurrentDisplayMode(d < 0 ? 0 : d, &m) == 0)
                __atomic_store_n(&s_refresh_hz, m.refresh_rate, __ATOMIC_RELAXED);
            next = now + 1000000;
        }
    }
    SDL_Event ev;

    if (__atomic_exchange_n(&s_win_req, 0, __ATOMIC_ACQ_REL))
        window_apply(s_win_fs, s_win_w, s_win_h);

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
                /* Out of fullscreen to the menus' mode, at the window's size: what
                 * the menus show follows (#282). */
                int to = (SDL_GetWindowFlags(s_window) & SDL_WINDOW_FULLSCREEN) ? s_win_base : 1;
                window_apply(to, 0, 0);
                nv2a_window_mode_changed(to);
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

#ifdef __ANDROID__
/* SDL's window as Java has it now, held by us: SDL's own pointer is swapped and released on Java's thread
 * (onNativeSurfaceDestroyed), unlocked for us. NULL while the app is in the background. */
static ANativeWindow *android_window(void)
{
    JNIEnv *env = SDL_AndroidGetJNIEnv();
    jobject act = SDL_AndroidGetActivity();
    ANativeWindow *w = NULL;
    if (!env || !act)
        return NULL;
    jclass cls = (*env)->GetObjectClass(env, act);
    jmethodID m = (*env)->GetStaticMethodID(env, cls, "getNativeSurface", "()Landroid/view/Surface;");
    jobject surface = m ? (*env)->CallStaticObjectMethod(env, cls, m) : NULL;
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);
        surface = NULL;
    }
    if (surface) {
        w = ANativeWindow_fromSurface(env, surface);
        (*env)->DeleteLocalRef(env, surface);
    }
    (*env)->DeleteLocalRef(env, cls);
    (*env)->DeleteLocalRef(env, act);
    return w;
}

/* A surface for the window as it is now, in place of any old one. Through the loader the instance came from
 * (volk's): SDL's own copy of the system's would not know an instance made through another (Turnip's,
 * instance.c's nv2a_vk_custom_loader). */
static bool android_surface_create(VkInstance instance)
{
    ANativeWindow *w = android_window();
    if (!w || !vkCreateAndroidSurfaceKHR) {
        if (w)
            ANativeWindow_release(w);
        return false;
    }
    if (s_surface)
        vkDestroySurfaceKHR(instance, s_surface, NULL);
    s_surface = VK_NULL_HANDLE;
    if (s_surface_window)
        ANativeWindow_release(s_surface_window);
    s_surface_window = w;
    s_surface_lost = false;
    VkAndroidSurfaceCreateInfoKHR ci = {
        .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
        .window = w,
    };
    if (vkCreateAndroidSurfaceKHR(instance, &ci, NULL, &s_surface) != VK_SUCCESS) {
        s_surface = VK_NULL_HANDLE;
        return false;
    }
    return true;
}

static void destroy_swapchain(PGRAPHVkState *r);

/* Android takes the window away while the app is in the background and hands a new one back on return
 * (#162): the old surface then only fails, so the picture stays black. Checked before each present:
 * false while there is no window to draw to. */
static bool android_surface_current(PGRAPHVkState *r)
{
    SDL_SysWMinfo wm;
    SDL_VERSION(&wm.version);
    void *now = s_window && SDL_GetWindowWMInfo(s_window, &wm) ? (void *)wm.info.android.window : NULL;
    if (!now)
        return false;
    if (now == (void *)s_surface_window && s_surface && !s_surface_lost)
        return true;
    if (s_swapchain)
        destroy_swapchain(r);
    if (!android_surface_create(s_instance))
        return false;
    fprintf(stderr, "nv2a: new window surface (the app is back in front)\n");
    return true;
}
#endif

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
#elif defined(__ANDROID__)
    /* Through the loader the instance came from (volk's): SDL's own copy of the system's would not know an
     * instance made through another (Turnip's, instance.c's nv2a_vk_custom_loader) */
    if (!android_surface_create(instance))
        fprintf(stderr, "nv2a: Vulkan surface: none for the window\n");
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
    layer_drop_views(r);
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
    s_fifo_held = false;
    if (s_vsync == 0 && g_get_monotonic_time() < __atomic_load_n(&s_fifo_until, __ATOMIC_ACQUIRE)) {
        s_fifo_held = true;
        return VK_PRESENT_MODE_FIFO_KHR;   /* just after a display-mode change */
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
    /* Pictures as they are shown, the system turning them: a phone held sideways reports its surface turned
     * (Android), and handing that turn on would leave the turning to us. The window's size is the picture's
     * then, and presenting reports "suboptimal" every frame for it, which is not a reason to rebuild. */
    VkSurfaceTransformFlagBitsKHR pre = caps.currentTransform;
    s_system_turns = false;
    if (pre != VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR &&
        (caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)) {
        int w, h;
        SDL_Vulkan_GetDrawableSize(s_window, &w, &h);
        if (w > 0 && h > 0) {
            s_extent.width = (uint32_t)w < caps.minImageExtent.width ? caps.minImageExtent.width
                           : (uint32_t)w > caps.maxImageExtent.width ? caps.maxImageExtent.width : (uint32_t)w;
            s_extent.height = (uint32_t)h < caps.minImageExtent.height ? caps.minImageExtent.height
                            : (uint32_t)h > caps.maxImageExtent.height ? caps.maxImageExtent.height : (uint32_t)h;
        }
        pre = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        s_system_turns = true;
    }

    /* drawn into as well: a picture over it (layer_draw: the touch controls, Android) and the last step down
     * from a high render resolution (area_draw) */
    VkImageUsageFlags color_usage = caps.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

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
            .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | color_usage,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .preTransform = pre,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = mode,
            .clipped = VK_TRUE,
            .oldSwapchain = old,
        };
        if (vkCreateSwapchainKHR(r->device, &ci, NULL, &s_swapchain) != VK_SUCCESS) {
            /* the old one is retired all the same: let it go, and on Android try a new surface next
             * (a window gone in the background can say only "out of date") */
            s_swapchain = old;
            if (old)
                destroy_swapchain(r);
#ifdef __ANDROID__
            s_surface_lost = true;
#endif
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
        layer_drop_views(r);
        for (uint32_t i = 0; i < s_image_count; i++)
            vkDestroySemaphore(r->device, s_render_done[i], NULL);
        vkDestroySwapchainKHR(r->device, old, NULL);
    }

    s_swapchain_color = color_usage != 0;
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
        /* NV2A_CAPTURE_SCALE=n: each n x n block averaged, for captures at a high render scale */
        static int sc = -1;
        if (sc < 0) {
            const char *e = getenv("NV2A_CAPTURE_SCALE");
            sc = e && atoi(e) > 1 ? atoi(e) : 1;
        }
        if (sc > 1) {
            int ow = w / sc, oh = h / sc;
            fprintf(f, "P6\n%d %d\n255\n", ow, oh);
            uint8_t *row = g_malloc((size_t)ow * 3);
            for (int oy = 0; oy < oh; oy++) {
                for (int ox = 0; ox < ow; ox++) {
                    unsigned sum[3] = { 0, 0, 0 };
                    for (int dy = 0; dy < sc; dy++) {
                        int i = oy * sc + dy, y = ext ? i : h - 1 - i;
                        for (int dx = 0; dx < sc; dx++) {
                            const uint8_t *q = px + ((size_t)y * w + ox * sc + dx) * 4;
                            sum[0] += ext ? q[2] : q[0];
                            sum[1] += q[1];
                            sum[2] += ext ? q[0] : q[2];
                        }
                    }
                    for (int c = 0; c < 3; c++)
                        row[ox * 3 + c] = (uint8_t)(sum[c] / (unsigned)(sc * sc));
                }
                fwrite(row, 1, (size_t)ow * 3, f);
            }
            g_free(row);
            fclose(f);
            vmaDestroyBuffer(r->allocator, buf, alloc);
            return;
        }
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

/* A present's side of s_switching: false (the frame skipped) while the window changes mode. */
static bool present_enter(void)
{
    __atomic_store_n(&s_in_present, 1, __ATOMIC_SEQ_CST);
    if (!__atomic_load_n(&s_switching, __ATOMIC_SEQ_CST))
        return true;
    __atomic_store_n(&s_in_present, 0, __ATOMIC_SEQ_CST);
    return false;
}

static void present_leave(void)
{
    __atomic_store_n(&s_in_present, 0, __ATOMIC_SEQ_CST);
}

/* The FIFO hold over: a swapchain in the chosen mode again. */
static void fifo_hold_check(void)
{
    if (s_fifo_held && g_get_monotonic_time() >= __atomic_load_n(&s_fifo_until, __ATOMIC_ACQUIRE))
        s_resized = true;
}

static void present_pixels_now(PGRAPHVkState *r, VkBuffer buf, int w, int h);

/* Show a w x h host picture (0xAARRGGBB) in the window. */
static void present_pixels(PGRAPHVkState *r, VkBuffer buf, int w, int h)
{
    if (!present_enter())
        return;
    present_pixels_now(r, buf, w, h);
    present_leave();
}

static void present_pixels_now(PGRAPHVkState *r, VkBuffer buf, int w, int h)
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
    if (res == VK_ERROR_SURFACE_LOST_KHR)
        s_surface_lost = true;
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
    if (res == VK_ERROR_OUT_OF_DATE_KHR || (res == VK_SUBOPTIMAL_KHR && !s_system_turns))
        s_resized = true;
    if (res == VK_ERROR_SURFACE_LOST_KHR)
        s_surface_lost = true;
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
#ifdef __ANDROID__
        if (!android_surface_current(r)) {
            g_usleep(16000);
            continue;
        }
#endif
        fifo_hold_check();
        if (s_resized || !s_swapchain) {
            if (!present_enter()) {
                g_usleep(16000);   /* (made once the window has changed) */
                continue;
            }
            s_resized = false;
            bool made = create_swapchain(r);
            present_leave();
            if (!made) {
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
    static int pace_log = -1;
    if (pace_log < 0)
        pace_log = getenv("NV2A_PACE_LOG") != NULL;
    if (pace_log) {
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

static void present_frame(PGRAPHState *pg, bool on_pfifo);

static void present_locked(PGRAPHState *pg, bool on_pfifo)
{
    if (!present_enter())
        return;
    present_frame(pg, on_pfifo);
    present_leave();
}

static void present_frame(PGRAPHState *pg, bool on_pfifo)
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
#ifdef __ANDROID__
    if (!android_surface_current(r))
        return;
#endif
    if (s_surface == VK_NULL_HANDLE)
        return;
    if (!s_pool) {
        create_frame_resources(r);
        fps_init(r);
    }
    fps_track();
    fifo_hold_check();
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
        present_leave();   /* (overlay_run's presents and swapchains enter and leave on their own) */
        overlay_run(pg);
        while (!present_enter())
            g_usleep(1000);
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
    if (res == VK_ERROR_SURFACE_LOST_KHR)
        s_surface_lost = true;
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
        static int test_color = -1;
        if (test_color < 0)
            test_color = getenv("NV2A_TEST_COLOR") != NULL;
        if (test_color)      /* letterbox in blue: is this the window? */
            black.float32[2] = 0.6f;
        VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        vkCmdClearColorImage(cmd, s_images[idx], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             &black, 1, &range);
        /* The picture is blitted over the cleared image: two transfer writes to it, ordered only by a
         * barrier (the validation layer's WRITE_AFTER_WRITE; a driver may overlap them, the clear landing
         * over part of the picture). */
        barrier(cmd, s_images[idx], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
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
        /* what is left is a reduction both ways: the area filter, from the last halving or a copy of the
         * picture (an image it can read) */
        bool area = sw >= (uint32_t)dw && sh >= (uint32_t)dh && (sw > (uint32_t)dw || sh > (uint32_t)dh) &&
                    area_ready(r);
        halving_images(r, n, pw, ph, area && n == 0);
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
        int lv = n > 0 && s_half[n - 1] && src == s_half[n - 1] ? n - 1 : -1;
        if (area && n == 0 && s_half[FULL_COPY]) {   /* the picture as it is, into an image the filter reads */
            barrier(cmd, s_half[FULL_COPY], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
                    VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
            VkImageBlit same = {
                .srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
                .srcOffsets = { { 0, 0, 0 }, { (int32_t)cw, (int32_t)ch, 1 } },
                .dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
                .dstOffsets = { { 0, 0, 0 }, { (int32_t)cw, (int32_t)ch, 1 } },
            };
            vkCmdBlitImage(cmd, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, s_half[FULL_COPY],
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &same, VK_FILTER_NEAREST);
            barrier(cmd, s_half[FULL_COPY], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT);
            lv = FULL_COPY;
        }
        /* The display image is bottom up; an external picture top down. */
        if (!(area && lv >= 0 && area_draw(r, cmd, idx, lv, cw, ch, !ext, x0, y0, dw, dh))) {
            blit.srcOffsets[0] = (VkOffset3D){ 0, ext ? 0 : (int32_t)ch, 0 };
            blit.srcOffsets[1] = (VkOffset3D){ (int32_t)cw, ext ? (int32_t)ch : 0, 1 };
            vkCmdBlitImage(cmd, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           s_images[idx], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                           &blit, VK_FILTER_LINEAR);
        }
        if (!ext)
            barrier(cmd, picture, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    }

    if (s_show_fps && s_ovl_ptr[f] && !nv2a_fps_drawn_outside) {
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
    layer_draw(r, cmd, f, idx);

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
        if (res == VK_ERROR_OUT_OF_DATE_KHR || (res == VK_SUBOPTIMAL_KHR && !s_system_turns))
            s_resized = true;
        if (res == VK_ERROR_SURFACE_LOST_KHR)
            s_surface_lost = true;
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
    for (int i = 0; i <= MAX_HALVINGS; i++) {
        if (s_half_view[i])
            vkDestroyImageView(r->device, s_half_view[i], NULL);
        if (s_half[i])
            vmaDestroyImage(r->allocator, s_half[i], s_half_mem[i]);
        s_half_view[i] = VK_NULL_HANDLE;
        s_half[i] = VK_NULL_HANDLE;
    }
    area_destroy(r);
    for (int i = 0; i < FRAMES_IN_FLIGHT; i++)
        if (s_ovl_buf[i])
            vmaDestroyBuffer(r->allocator, s_ovl_buf[i], s_ovl_mem[i]);
    if (s_pool)
        vkDestroyCommandPool(r->device, s_pool, NULL);
    s_pool = VK_NULL_HANDLE;
    if (s_surface)
        vkDestroySurfaceKHR(s_instance, s_surface, NULL);
    s_surface = VK_NULL_HANDLE;
#ifdef __ANDROID__
    if (s_surface_window)
        ANativeWindow_release(s_surface_window);
    s_surface_window = NULL;
#endif
}
