/*
 * The frame rate counter (F10, Show FPS) in the game's own menu letters: the frame
 * rate and frame time in white, a soft dark shadow under them, nothing behind (src/pause_tab.c,
 * ngb_game_text_picture). Lettered on the title's thread once a frame when the text changes (twice a second); the
 * Direct3D renderer blends the picture into the top left corner of each frame (src/d3d/vk/backend.cpp, FpsCounter),
 * and the GPU emulation's presenter copies it over a dark strip when another renderer draws (xboxrecomp's
 * present_sdl.c, nv2a_fps_render).
 */
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BACK 0xFF121212u
#define MAX_W 1024
#define MAX_H 128

bool nv2a_present_show_fps(void);                         /* xboxrecomp present_sdl.c */
void nv2a_present_fps_text(char *buf, size_t n);
/* src/pause_tab.c; the arcade programs have not got it (nor ngb_fps_tick's caller): the presenter's own letters */
__attribute__((weak)) int ngb_game_text_picture(const char *s, float cap, uint32_t *out, int maxw, int maxh, int *ow,
                                                int *oh)
{
    (void)s; (void)cap; (void)out; (void)maxw; (void)maxh; (void)ow; (void)oh;
    return 0;
}

static pthread_mutex_t s_lock = PTHREAD_MUTEX_INITIALIZER;
static uint32_t s_px[MAX_W * MAX_H];                      /* the picture, premultiplied BGRA */
static int s_w, s_h;
static unsigned s_serial;                                 /* +1 each time it changes */
volatile int ngb_fps_target_h;                            /* the picture it goes into (the renderer's, else the window's) */

/* The capital's height for a picture h tall, lettered at most 40 pixels (re-lettering a larger one twice a second
 * cost the title's thread milliseconds at high render scales); *scale how much larger it is drawn. */
int ngb_fps_cap(int h, float *scale)
{
    float want = h * 0.021f;
    int cap = (int)lroundf(want < 9 ? 9 : want > 40 ? 40 : want);
    if (scale)
        *scale = want / cap;
    return cap;
}

void ngb_fps_tick(void)
{
    static char last[64];
    static int last_cap;
    static uint32_t px[MAX_W * MAX_H];
    char text[64];
    if (!nv2a_present_show_fps()) {
        if (last[0]) {
            pthread_mutex_lock(&s_lock);
            s_w = s_h = 0;
            s_serial++;
            pthread_mutex_unlock(&s_lock);
            last[0] = 0;
        }
        return;
    }
    nv2a_present_fps_text(text, sizeof text);
    char *max = strstr(text, "  max");                    /* (the log keeps the longest frame; the screen not) */
    if (max)
        *max = 0;
    int cap = ngb_fps_cap(ngb_fps_target_h > 0 ? ngb_fps_target_h : 720, NULL);
    if (!strcmp(text, last) && cap == last_cap)
        return;
    int w, ph;
    if (!ngb_game_text_picture(text, (float)cap, px, MAX_W, MAX_H, &w, &ph))
        return;                                           /* the font not loaded yet: next frame */
    snprintf(last, sizeof last, "%s", text);
    last_cap = cap;
    pthread_mutex_lock(&s_lock);
    memcpy(s_px, px, sizeof *px * (size_t)w * ph);
    s_w = w;
    s_h = ph;
    s_serial++;
    pthread_mutex_unlock(&s_lock);
}

/* The picture into out (at most max pixels), if it changed since *serial; its size, 0 when there is none. */
int ngb_fps_picture(uint32_t *out, size_t max, int *w, int *h, unsigned *serial)
{
    int changed = 0;
    pthread_mutex_lock(&s_lock);
    if (*serial != s_serial && (size_t)s_w * s_h <= max) {
        memcpy(out, s_px, sizeof *out * (size_t)s_w * s_h);
        *w = s_w;
        *h = s_h;
        *serial = s_serial;
        changed = 1;
    }
    pthread_mutex_unlock(&s_lock);
    return changed;
}

/* ---- The port's version in the main menu's bottom right corner (#207) ----
 * Lettered the same way, once (and again when the picture's height changes), while the main menu's list is drawn
 * (src/main_menu.c, sub_0017A010, stamps ngb_version_seen each frame it draws it); the renderer shows it while the
 * stamp is fresh. */
#ifndef NGB_VERSION
#define NGB_VERSION "dev"
#endif
static pthread_mutex_t s_ver_lock = PTHREAD_MUTEX_INITIALIZER;
static uint32_t s_ver_px[MAX_W * MAX_H];
static int s_ver_w, s_ver_h;
static unsigned s_ver_serial;
static volatile unsigned s_ver_seen;                      /* frames since the main menu was last drawn, counting down */

void ngb_version_seen(void) { s_ver_seen = 3; }

void ngb_version_tick(void)
{
    static int last_cap;
    static uint32_t px[MAX_W * MAX_H];
    if (s_ver_seen)
        s_ver_seen--;
    int cap = ngb_fps_cap(ngb_fps_target_h > 0 ? ngb_fps_target_h : 720, NULL);
    if (!s_ver_seen || cap == last_cap)
        return;
    int w, ph;
    if (!ngb_game_text_picture("v" NGB_VERSION, (float)cap, px, MAX_W, MAX_H, &w, &ph))
        return;                                           /* the font not loaded yet: next frame */
    last_cap = cap;
    pthread_mutex_lock(&s_ver_lock);
    memcpy(s_ver_px, px, sizeof *px * (size_t)w * ph);
    s_ver_w = w;
    s_ver_h = ph;
    s_ver_serial++;
    pthread_mutex_unlock(&s_ver_lock);
}

/* Whether the main menu is up (the version to be shown). */
int ngb_version_shown(void) { return s_ver_seen != 0 && s_ver_w > 0; }

/* As ngb_fps_picture, for the version's picture. */
int ngb_version_picture(uint32_t *out, size_t max, int *w, int *h, unsigned *serial)
{
    int changed = 0;
    pthread_mutex_lock(&s_ver_lock);
    if (*serial != s_ver_serial && (size_t)s_ver_w * s_ver_h <= max) {
        memcpy(out, s_ver_px, sizeof *out * (size_t)s_ver_w * s_ver_h);
        *w = s_ver_w;
        *h = s_ver_h;
        *serial = s_ver_serial;
        changed = 1;
    }
    pthread_mutex_unlock(&s_ver_lock);
    return changed;
}

/* The presenter's corner copy (another renderer drawing): the picture over a dark strip, opaque. */
bool nv2a_fps_render(const char *text, uint32_t line_px, uint32_t *px, uint32_t maxw, uint32_t maxh,
                     uint32_t *ow, uint32_t *oh)
{
    (void)text;
    ngb_fps_target_h = (int)line_px * 36;                 /* (present_sdl's line: the window's height / 36) */
    pthread_mutex_lock(&s_lock);
    int w = s_w, h = s_h;
    if (!w || (uint32_t)w > maxw || (uint32_t)h > maxh) {
        pthread_mutex_unlock(&s_lock);
        return false;                                     /* its own 5x7 letters meanwhile */
    }
    for (int i = 0; i < w * h; i++) {
        uint32_t c = s_px[i], a = c >> 24, out = 0xFF000000u;
        for (int sh = 0; sh < 24; sh += 8)
            out |= (((c >> sh) & 0xFF) + ((BACK >> sh) & 0xFF) * (255 - a) / 255) << sh;
        px[i] = out;
    }
    pthread_mutex_unlock(&s_lock);
    *ow = (uint32_t)w;
    *oh = (uint32_t)h;
    return true;
}
