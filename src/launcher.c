/*
 * launcher.c -- the settings screen shown before the game starts.
 *
 * Its own small SDL window with an SDL_Renderer (the game's window is a
 * Vulkan one, created afterwards), driven by keyboard, mouse or controller
 * so it works the same at a desk and on a Steam Deck. Text comes from a
 * bitmap font baked into launcher_font.h (tools/font/gen_font.py).
 *
 * The choices live in $XDG_CONFIG_HOME/NinjaGaidenBlack/settings.ini; the
 * environment variables of the same meaning (NGB_WIDESCREEN, NGB_FULLSCREEN,
 * NGB_VSYNC, NV2A_SCALE) still override them, for testing.
 */
#include <SDL.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "launcher.h"
#include "launcher_font.h"
#include "port.h"

/* ---------- settings file ---------- */

static void config_path(char *out, size_t n, int make_dir)
{
    char dir[1024];
    ngb_config_dir(dir, sizeof dir, make_dir);
    snprintf(out, n, "%s/settings.ini", dir);
}

static const char *const ASPECT_NAMES[ASPECT_COUNT] = { "4:3", "16:9", "16:10", "21:9" };

/* Window sizes offered for each aspect ratio, smallest first. */
static const int SIZES_4_3[][2] = {
    { 960, 720 }, { 1024, 768 }, { 1280, 960 }, { 1440, 1080 },
    { 1600, 1200 }, { 1920, 1440 }, { 2880, 2160 }, { 0, 0 } };
static const int SIZES_16_9[][2] = {
    { 1280, 720 }, { 1600, 900 }, { 1920, 1080 }, { 2560, 1440 },
    { 3200, 1800 }, { 3840, 2160 }, { 0, 0 } };
static const int SIZES_16_10[][2] = {
    { 1280, 800 }, { 1440, 900 }, { 1680, 1050 }, { 1920, 1200 },
    { 2560, 1600 }, { 3840, 2400 }, { 0, 0 } };
static const int SIZES_21_9[][2] = {     /* the "21:9" of the displays: 64:27 (2560 x 1080), 43:18 (3440 x 1440) */
    { 1720, 720 }, { 2560, 1080 }, { 3440, 1440 }, { 5120, 2160 }, { 0, 0 } };

static const int (*sizes_for(int aspect))[2]
{
    return aspect == ASPECT_4_3 ? SIZES_4_3
         : aspect == ASPECT_16_10 ? SIZES_16_10 : aspect == ASPECT_21_9 ? SIZES_21_9 : SIZES_16_9;
}

static int nsizes(int aspect)
{
    int n = 0;
    while (sizes_for(aspect)[n][0])
        n++;
    return n;
}

static int size_index(const NgbSettings *s)
{
    const int (*z)[2] = sizes_for(s->aspect);
    for (int i = 0; z[i][0]; i++)
        if (z[i][0] == s->width && z[i][1] == s->height)
            return i;
    return -1;
}

int ngb_settings_size_count(int aspect) { return nsizes(aspect); }

void ngb_settings_size_at(int aspect, int i, int *w, int *h)
{
    *w = sizes_for(aspect)[i][0];
    *h = sizes_for(aspect)[i][1];
}

int ngb_settings_size_index(const NgbSettings *s) { return size_index(s); }

/* A window size of the current aspect: the listed one nearest in height. */
static void snap_size(NgbSettings *s)
{
    const int (*z)[2] = sizes_for(s->aspect);
    int best = 0;
    if (size_index(s) >= 0)
        return;
    for (int i = 1; z[i][0]; i++)
        if (abs(z[i][1] - s->height) < abs(z[best][1] - s->height))
            best = i;
    s->width = z[best][0];
    s->height = z[best][1];
}

void ngb_settings_snap_size(NgbSettings *s) { snap_size(s); }

void ngb_settings_load(NgbSettings *s)
{
    char path[1100], line[256];
    FILE *f;

    memset(s, 0, sizeof *s);   /* off unless set: All Costumes, All Difficulties, 4:3 movies... */
    s->aspect = ASPECT_16_9;
    s->width = 1280;
    s->height = 720;
    s->fullscreen = 0;
    s->scale = 1;
    s->vsync = 0;
    s->show_fps = 0;
    s->anisotropy = 16;
    s->msaa = 1;
    s->shadow_scale = 4;
    s->skip_intro = 0;
    s->show_launcher = 1;

    config_path(path, sizeof path, 0);
    f = fopen(path, "r");
    printf("Settings: %s%s\n", path, f ? "" : " (not there yet: the defaults)");
    if (!f)
        return;
    s->width = s->height = 0;
    while (fgets(line, sizeof line, f)) {
        char key[64], val[128];
        if (sscanf(line, " %63[^= ] = %127[^\r\n]", key, val) != 2)
            continue;
        if (!strcmp(key, "aspect")) {
            for (int i = 0; i < ASPECT_COUNT; i++)
                if (!strcmp(val, ASPECT_NAMES[i]))
                    s->aspect = i;
        } else if (!strcmp(key, "window")) {
            int w, h;
            if (sscanf(val, "%dx%d", &w, &h) == 2 && w >= 320 && h >= 200) {
                s->width = w;
                s->height = h;
            }
        } else if (!strcmp(key, "fullscreen")) {
            s->fullscreen = atoi(val) < 0 ? 0 : atoi(val) > 2 ? 2 : atoi(val);
        } else if (!strcmp(key, "render_scale")) {
            int v = atoi(val);
            s->scale = v < 1 ? 1 : v > 10 ? 10 : v;
        } else if (!strcmp(key, "vsync")) {
            s->vsync = atoi(val) < 0 ? 0 : atoi(val) > 3 ? 3 : atoi(val);
        } else if (!strcmp(key, "shadow_resolution")) {
            int v = atoi(val);
            s->shadow_scale = v >= 10 ? 10 : v >= 8 ? 8 : v >= 4 ? 4 : v >= 2 ? 2 : 1;
        } else if (!strcmp(key, "texture_filter")) {          /* before anisotropic_filtering */
            s->anisotropy = strcmp(val, "original") ? 16 : 0;
        } else if (!strcmp(key, "anisotropic_filtering")) {
            int v = atoi(val);
            s->anisotropy = v >= 16 ? 16 : v >= 8 ? 8 : v >= 4 ? 4 : v >= 2 ? 2 : 0;
        } else if (!strcmp(key, "msaa")) {
            int v = atoi(val);
            s->msaa = v >= 8 ? 8 : v >= 4 ? 4 : v >= 2 ? 2 : 1;
        } else if (!strcmp(key, "unlock_all_difficulties")) {
            s->all_difficulties = atoi(val) != 0;
        } else if (!strcmp(key, "unlock_all_costumes")) {
            s->all_costumes = atoi(val) != 0;
        } else if (!strcmp(key, "movie_ratio")) {
            s->movie_43 = !strcmp(val, "4:3");
        } else if (!strcmp(key, "show_fps")) {
            s->show_fps = atoi(val) != 0;
        } else if (!strcmp(key, "skip_intro")) {
            s->skip_intro = atoi(val) != 0;
        } else if (!strcmp(key, "show_settings_at_startup")) {
            s->show_launcher = atoi(val) != 0;
        }
    }
    fclose(f);
    snap_size(s);
}

void ngb_settings_save(const NgbSettings *s)
{
    char path[1100];
    FILE *f;

    config_path(path, sizeof path, 1);
    f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "settings: cannot write %s: %s\n", path, strerror(errno));
        return;
    }
    fprintf(f, "# Ninja Gaiden Black settings (the settings screen writes this file)\n");
    fprintf(f, "aspect = %s\n", ASPECT_NAMES[s->aspect]);
    fprintf(f, "window = %dx%d\n", s->width, s->height);
    fprintf(f, "fullscreen = %d\n", s->fullscreen);
    fprintf(f, "render_scale = %d\n", s->scale);
    fprintf(f, "vsync = %d\n", s->vsync);
    fprintf(f, "show_fps = %d\n", s->show_fps);
    fprintf(f, "anisotropic_filtering = %d\n", s->anisotropy);
    fprintf(f, "msaa = %d\n", s->msaa);
    fprintf(f, "unlock_all_difficulties = %d\n", s->all_difficulties);
    fprintf(f, "unlock_all_costumes = %d\n", s->all_costumes);
    fprintf(f, "movie_ratio = %s\n", s->movie_43 ? "4:3" : "16:9");
    fprintf(f, "shadow_resolution = %d\n", s->shadow_scale);
    fprintf(f, "skip_intro = %d\n", s->skip_intro);
    fprintf(f, "show_settings_at_startup = %d\n", s->show_launcher);
    fclose(f);
}

double ngb_settings_aspect_ratio(const NgbSettings *s)
{
    switch (s->aspect) {
    case ASPECT_4_3:   return 4.0 / 3.0;
    case ASPECT_16_10: return 16.0 / 10.0;
    case ASPECT_21_9:  return 64.0 / 27.0;
    default:           return 16.0 / 9.0;
    }
}

/* ---------- drawing ---------- */

#define LW 960            /* logical size of the screen */
#define LH 748

static SDL_Texture *s_font;
static int s_glyph_x[95];

const char *ngb_vsync_name(int v)
{
    return v == 1 ? "Mailbox" : v == 2 ? "Adaptive" : v == 3 ? "FIFO" : "Off";
}

int ngb_vsync_step(int v, int dir)
{
    static const int order[4] = { 0, 1, 3, 2 };
    int i = 0;
    while (i < 3 && order[i] != v)
        i++;
    return order[(i + (dir < 0 ? 3 : 1)) % 4];
}

static int font_init(SDL_Renderer *r)
{
    int total = 0;
    for (int i = 0; i < 95; i++) {
        s_glyph_x[i] = total;
        total += launcher_font_w[i];
    }
    uint32_t *px = calloc((size_t)total * LAUNCHER_FONT_H_PX, 4);
    if (!px)
        return -1;
    for (int i = 0; i < 95; i++) {
        int w = launcher_font_w[i];
        const uint8_t *g = launcher_font_px + launcher_font_off[i];
        for (int y = 0; y < LAUNCHER_FONT_H_PX; y++)
            for (int x = 0; x < w; x++)
                px[y * total + s_glyph_x[i] + x] =
                    ((uint32_t)g[y * w + x] << 24) | 0x00FFFFFFu;
    }
    s_font = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STATIC, total, LAUNCHER_FONT_H_PX);
    if (!s_font) {
        free(px);
        return -1;
    }
    SDL_UpdateTexture(s_font, NULL, px, total * 4);
    SDL_SetTextureBlendMode(s_font, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(s_font, SDL_ScaleModeLinear);
    free(px);
    return 0;
}

static int text_width(const char *t, float size)
{
    float w = 0;
    for (; *t; t++) {
        int c = (unsigned char)*t;
        if (c < 32 || c > 126) c = '?';
        w += launcher_font_w[c - 32] * size / LAUNCHER_FONT_H_PX;
    }
    return (int)w;
}

/* align: 0 left, 1 centre, 2 right of x */
static void draw_text(SDL_Renderer *r, const char *t, int x, int y, float size,
                      SDL_Color col, int align)
{
    float fx = (float)x;
    if (align == 1) fx -= text_width(t, size) / 2.0f;
    if (align == 2) fx -= (float)text_width(t, size);
    SDL_SetTextureColorMod(s_font, col.r, col.g, col.b);
    SDL_SetTextureAlphaMod(s_font, col.a);
    for (; *t; t++) {
        int c = (unsigned char)*t;
        if (c < 32 || c > 126) c = '?';
        int w = launcher_font_w[c - 32];
        SDL_Rect src = { s_glyph_x[c - 32], 0, w, LAUNCHER_FONT_H_PX };
        SDL_FRect dst = { fx, (float)y, w * size / LAUNCHER_FONT_H_PX, size };
        SDL_RenderCopyF(r, s_font, &src, &dst);
        fx += dst.w;
    }
}

static void fill(SDL_Renderer *r, int x, int y, int w, int h, SDL_Color c)
{
    SDL_Rect rc = { x, y, w, h };
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_RenderFillRect(r, &rc);
}

/* ---------- the screen ---------- */

enum { ROW_ASPECT, ROW_SIZE, ROW_DISPLAY, ROW_SCALE, ROW_TEXFILTER, ROW_MSAA, ROW_SHADOW,
       ROW_VSYNC, ROW_FPS, ROW_SKIP, ROW_START, ROW_QUIT, NROWS };
_Static_assert(ROW_START == NGB_SETTINGS_ITEMS, "settings rows");

void ngb_settings_change(NgbSettings *s, int row, int dir)
{
    switch (row) {
    case ROW_ASPECT:
        s->aspect = (s->aspect + dir + ASPECT_COUNT) % ASPECT_COUNT;
        snap_size(s);
        break;
    case ROW_SIZE: {
        int n = nsizes(s->aspect), i = size_index(s);
        if (i < 0) i = dir > 0 ? -1 : n;
        i = (i + dir + n) % n;
        s->width = sizes_for(s->aspect)[i][0];
        s->height = sizes_for(s->aspect)[i][1];
        break;
    }
    case ROW_DISPLAY: s->fullscreen = (s->fullscreen + (dir < 0 ? 2 : 1)) % 3; break;
    case ROW_SCALE:   s->scale = (s->scale - 1 + dir + 10) % 10 + 1; break;
    case ROW_VSYNC:   s->vsync = ngb_vsync_step(s->vsync, dir); break;
    case ROW_FPS:     s->show_fps = !s->show_fps; break;
    case ROW_TEXFILTER: {
        static const int STEPS[] = { 0, 2, 4, 8, 16 };
        int i = 0;
        while (i < 4 && STEPS[i] != s->anisotropy) i++;
        s->anisotropy = STEPS[(i + dir + 5) % 5];
        break;
    }
    case ROW_MSAA: {
        static const int STEPS[] = { 1, 2, 4, 8 };
        int i = 0;
        while (i < 3 && STEPS[i] != s->msaa) i++;
        s->msaa = STEPS[(i + dir + 4) % 4];
        break;
    }
    case ROW_SHADOW: {
        static const int STEPS[] = { 1, 2, 4, 8, 10 };
        int i = 0;
        while (i < 4 && STEPS[i] != s->shadow_scale) i++;
        s->shadow_scale = STEPS[(i + dir + 5) % 5];
        break;
    }
    case ROW_SKIP:    s->skip_intro = !s->skip_intro; break;
    default: break;
    }
}

void ngb_settings_row_text(const NgbSettings *s, int row, const char **label, char *val, size_t n)
{
    static const char *const ASPECT_LABEL[ASPECT_COUNT] = { "4:3", "16:9", "16:10", "21:9" };
    static const char *const SCALE_LABEL[10] = {
        "Native (1x)", "2x", "3x", "4x", "5x", "6x", "7x", "8x", "9x", "10x" };
    val[0] = 0;
    switch (row) {
    case ROW_ASPECT:  *label = "Aspect ratio";  snprintf(val, n, "%s", ASPECT_LABEL[s->aspect]); break;
    case ROW_SIZE:    *label = "Window size";   snprintf(val, n, "%d x %d", s->width, s->height); break;
    case ROW_DISPLAY: *label = "Display";       snprintf(val, n, "%s", s->fullscreen == 2 ? "Borderless" : s->fullscreen ? "Fullscreen" : "Windowed"); break;
    case ROW_SCALE:   *label = "Render resolution"; snprintf(val, n, "%s", SCALE_LABEL[s->scale - 1]); break;
    case ROW_VSYNC:   *label = "V-Sync";        snprintf(val, n, "%s", ngb_vsync_name(s->vsync)); break;
    case ROW_FPS:     *label = "Show FPS";      snprintf(val, n, "%s", s->show_fps ? "On" : "Off"); break;
    case ROW_TEXFILTER: *label = "Anisotropic filtering"; if (s->anisotropy) snprintf(val, n, "%dx", s->anisotropy); else snprintf(val, n, "Off"); break;
    case ROW_MSAA:    *label = "Anti-aliasing (MSAA)"; if (s->msaa > 1) snprintf(val, n, "%dx", s->msaa); else snprintf(val, n, "Off"); break;
    case ROW_SHADOW:  *label = "Shadow resolution"; if (s->shadow_scale == 1) snprintf(val, n, "Original"); else snprintf(val, n, "%dx", s->shadow_scale); break;
    case ROW_SKIP:    *label = "Skip intros and warnings"; snprintf(val, n, "%s", s->skip_intro ? "Yes" : "No"); break;
    case ROW_START:   *label = "Start game"; break;
    case ROW_QUIT:    *label = "Quit"; break;
    }
}

#define ROW_Y0 128
#define ROW_H  44

static int row_at(int y)
{
    if (y < ROW_Y0)
        return -1;
    int r = (y - ROW_Y0) / ROW_H;
    if (r >= ROW_START)            /* a gap before the buttons */
        r = (y - ROW_Y0 - 16) / ROW_H;
    return r >= 0 && r < NROWS ? r : -1;
}

static int row_y(int row)
{
    return ROW_Y0 + row * ROW_H + (row >= ROW_START ? 16 : 0);
}

static void draw(SDL_Renderer *r, const NgbSettings *s, int sel)
{
    const SDL_Color white = { 235, 235, 235, 255 }, grey = { 150, 150, 155, 255 },
                    red = { 200, 32, 40, 255 }, bar = { 120, 16, 22, 200 },
                    dim = { 255, 255, 255, 18 };

    SDL_SetRenderDrawColor(r, 14, 14, 18, 255);
    SDL_RenderClear(r);
    fill(r, 0, 0, LW, 96, (SDL_Color){ 26, 8, 10, 255 });
    fill(r, 0, 94, LW, 2, red);
    draw_text(r, "NINJA GAIDEN BLACK", LW / 2, 18, 44, white, 1);
    draw_text(r, "Settings", LW / 2, 64, 22, grey, 1);

    for (int row = 0; row < NROWS; row++) {
        const char *label;
        char val[96];
        int y = row_y(row);
        ngb_settings_row_text(s, row, &label, val, sizeof val);
        if (row >= ROW_START) {
            int w = 260, x = LW / 2 - w / 2;
            fill(r, x, y + 2, w, ROW_H - 6, row == sel ? bar : dim);
            draw_text(r, label, LW / 2, y + 8, 26, row == sel ? white : grey, 1);
            continue;
        }
        if (row == sel)
            fill(r, 60, y + 2, LW - 120, ROW_H - 6, bar);
        else if (row % 2)
            fill(r, 60, y + 2, LW - 120, ROW_H - 6, dim);
        draw_text(r, label, 84, y + 9, 24, row == sel ? white : grey, 0);
        draw_text(r, "<", 560, y + 9, 24, row == sel ? white : grey, 0);
        draw_text(r, val, 736, y + 9, 24, white, 1);
        draw_text(r, ">", LW - 92, y + 9, 24, row == sel ? white : grey, 0);
    }
    draw_text(r, "Up/Down: choose    Left/Right: change    Enter / A: select    Start: play    Esc / B: quit",
              LW / 2, LH - 58, 17, grey, 1);
    draw_text(r, "In game: the pause menu's Settings tab has these options, F11 toggles fullscreen, F10 the FPS counter.",
              LW / 2, LH - 34, 15, grey, 1);
    SDL_RenderPresent(r);
}

int ngb_launcher_run(NgbSettings *s)
{
    SDL_Window *win;
    SDL_Renderer *r;
    SDL_GameController *pad = NULL;
    int sel = ROW_START, result = -1;
    Uint32 repeat_at = 0;
    int held_dir = 0;            /* stick/d-pad auto-repeat: -2 up, 2 down, -1/1 */

#if !defined(_WIN32)
    if (!getenv("SDL_VIDEODRIVER"))
        SDL_SetHint(SDL_HINT_VIDEODRIVER, "wayland,x11");
#endif
    if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        fprintf(stderr, "settings screen: %s\n", SDL_GetError());
        return 1;                /* no screen: just start */
    }
    win = SDL_CreateWindow("Ninja Gaiden Black", SDL_WINDOWPOS_CENTERED,
                           SDL_WINDOWPOS_CENTERED, LW, LH,
                           SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    r = win ? SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED |
                                          SDL_RENDERER_PRESENTVSYNC) : NULL;
    if (r == NULL && win)
        r = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!win || !r || font_init(r) != 0) {
        fprintf(stderr, "settings screen unavailable: %s\n", SDL_GetError());
        if (r) SDL_DestroyRenderer(r);
        if (win) SDL_DestroyWindow(win);
        return 1;
    }
    SDL_RenderSetLogicalSize(r, LW, LH);
    Uint32 t_open = SDL_GetTicks();
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (int i = 0; i < SDL_NumJoysticks() && !pad; i++)
        if (SDL_IsGameController(i))
            pad = SDL_GameControllerOpen(i);

    while (result < 0) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            int act = 0, dir = 0, move = 0;
            switch (ev.type) {
            case SDL_QUIT:
                result = 0;
                break;
            case SDL_KEYDOWN:
                switch (ev.key.keysym.sym) {
                case SDLK_UP:    case SDLK_w: move = -1; break;
                case SDLK_DOWN:  case SDLK_s: move = 1; break;
                case SDLK_LEFT:  case SDLK_a: dir = -1; break;
                case SDLK_RIGHT: case SDLK_d: dir = 1; break;
                case SDLK_RETURN: case SDLK_KP_ENTER: case SDLK_SPACE: act = 1; break;
                case SDLK_ESCAPE: result = 0; break;
                default: break;
                }
                break;
            case SDL_CONTROLLERDEVICEADDED:
                if (!pad)
                    pad = SDL_GameControllerOpen(ev.cdevice.which);
                break;
            case SDL_CONTROLLERBUTTONDOWN:
                switch (ev.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_DPAD_UP:    move = -1; held_dir = -2; break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  move = 1;  held_dir = 2;  break;
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  dir = -1;  held_dir = -1; break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: dir = 1;   held_dir = 1;  break;
                case SDL_CONTROLLER_BUTTON_A: act = 1; break;
                case SDL_CONTROLLER_BUTTON_START: sel = ROW_START; act = 1; break;
                case SDL_CONTROLLER_BUTTON_B: result = 0; break;
                default: break;
                }
                repeat_at = SDL_GetTicks() + 400;
                break;
            case SDL_CONTROLLERBUTTONUP:
                held_dir = 0;
                break;
            case SDL_CONTROLLERAXISMOTION:
                if (ev.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY ||
                    ev.caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
                    int v = ev.caxis.value, y = ev.caxis.axis == SDL_CONTROLLER_AXIS_LEFTY;
                    int nd = v > 20000 ? (y ? 2 : 1) : v < -20000 ? (y ? -2 : -1) : 0;
                    if (nd && nd != held_dir) {
                        if (y) move = nd / 2; else dir = nd;
                        repeat_at = SDL_GetTicks() + 400;
                    }
                    if (nd || (y ? (held_dir == 2 || held_dir == -2)
                                 : (held_dir == 1 || held_dir == -1)))
                        held_dir = nd;
                }
                break;
            case SDL_MOUSEMOTION: {
                /* A pointer already resting over the window reports a
                 * motion at once; only a real move should select. */
                if (SDL_GetTicks() - t_open < 500)
                    break;
                int row = row_at(ev.motion.y);
                if (row >= 0) sel = row;
                break;
            }
            case SDL_MOUSEBUTTONDOWN: {
                int row = row_at(ev.button.y);
                if (row >= 0) {
                    sel = row;
                    if (row >= ROW_START) act = 1;
                    else dir = (ev.button.button == SDL_BUTTON_RIGHT ||
                                ev.button.x < 560 + 40) ? -1 : 1;
                }
                break;
            }
            default:
                break;
            }
            if (move)
                sel = (sel + move + NROWS) % NROWS;
            if (dir && sel < ROW_START)
                ngb_settings_change(s, sel, dir);
            if (act) {
                if (sel == ROW_START) result = 1;
                else if (sel == ROW_QUIT) result = 0;
                else ngb_settings_change(s, sel, 1);
            }
        }
        /* Held d-pad or stick: repeat after a pause. */
        if (held_dir && SDL_GetTicks() >= repeat_at) {
            if (held_dir == 2 || held_dir == -2)
                sel = (sel + held_dir / 2 + NROWS) % NROWS;
            else if (sel < ROW_START)
                ngb_settings_change(s, sel, held_dir);
            repeat_at = SDL_GetTicks() + 140;
        }
        draw(r, s, sel);
    }

    if (pad)
        SDL_GameControllerClose(pad);
    SDL_DestroyTexture(s_font);
    s_font = NULL;
    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(win);
    if (result == 1)
        ngb_settings_save(s);
    return result;
}
