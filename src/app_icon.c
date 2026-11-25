/*
 * app_icon.c -- the program's icon, made from the player's own copy of the game.
 *
 * The picture is the title logo's first letter with its red glow on a black rounded tile. The logo is game art,
 * so it is not in this repository nor in a release: it is read from <game dir>/demo.afs -> demo.spr (an XPR0
 * package), texture 2 (512 x 256, P8 swizzled with a BGRA palette: "NINJA GAIDEN", the glow in its alpha). The
 * letter is found there, not placed by numbers: the leftmost opaque shape of the top row.
 *
 * Used for the window's icon (Windows' taskbar, X11's) and, on Linux, written as the desktop's icon
 * (hicolor/<size>/apps/ninja-gaiden-black.png under XDG_DATA_HOME, again when ninja-gaiden-black.version beside
 * the largest names another ICON_VERSION): a menu entry naming that icon (the AppImage's own entry does) shows
 * it, and Wayland's compositors match the window to that entry by its app id (ngb_app_id_setup). No entry is
 * written: the player's, or an AppImage tool's, is theirs. The settings screen's window gets this icon too
 * (src/launcher.c); only the program file's own icon (and the package's, e.g. the AppImage's) stays the
 * repository's.
 */
#include <SDL.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "port.h"

#define TW 512
#define TH 256
#define M 512                  /* the master's size: drawn there, reduced to the size asked for */
#define APP_ID "ninja-gaiden-black"
#define ICON_VERSION 1         /* raised whenever the icon is drawn differently: the desktop's files written again */

int stbi_write_png(const char *filename, int w, int h, int comp, const void *data, int stride_in_bytes);

static uint32_t rd32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }

/* One file of an .afs archive: malloc'd, its size in *size; NULL when not there */
static uint8_t *afs_file(const char *path, const char *name, size_t *size)
{
    FILE *f = fopen(path, "rb");
    uint8_t head[8], *table = NULL, *names = NULL, *out = NULL;
    if (!f)
        return NULL;
    if (fread(head, 1, 8, f) != 8 || memcmp(head, "AFS\0", 4))
        goto done;
    uint32_t n = rd32(head + 4);
    if (!n || n > 65536)
        goto done;
    table = malloc((size_t)(n + 1) * 8);
    names = malloc((size_t)n * 48);
    if (!table || !names || fread(table, 8, n + 1, f) != n + 1)
        goto done;
    if (fseek(f, (long)rd32(table + n * 8), SEEK_SET) != 0 || fread(names, 48, n, f) != n)
        goto done;
    for (uint32_t i = 0; i < n; i++) {
        if (strncmp((const char *)names + i * 48, name, 32))
            continue;
        uint32_t off = rd32(table + i * 8), sz = rd32(table + i * 8 + 4);
        if (!sz || sz > (64u << 20) || fseek(f, (long)off, SEEK_SET) != 0 || !(out = malloc(sz)))
            break;
        if (fread(out, 1, sz, f) != sz) {
            free(out);
            out = NULL;
        }
        *size = sz;
        break;
    }
done:
    free(table);
    free(names);
    fclose(f);
    return out;
}

/* Texture 2 of the package as RGBA floats (TW x TH); 0 when it is not what is expected */
static int logo_texture(const uint8_t *b, size_t size, float *px)
{
    if (size < 0x40 || memcmp(b, "XPR0", 4))
        return 0;
    uint32_t data = rd32(b + 8), dsize = rd32(b + 0x10), r = 0x14 + dsize, pal = 0;
    if (data > size || dsize > size || r > data)
        return 0;
    for (int i = 0;; i++) {            /* resources: a texture (type 4), then its palette (type 3) when it has one */
        if (r + 20 > data || (rd32(b + r) >> 16 & 7) != 4)
            return 0;
        pal = r + 32 <= data && (rd32(b + r + 20) >> 16 & 7) == 3 ? r + 20 : 0;
        if (i == 2)
            break;
        r += pal ? 32 : 20;
    }
    uint32_t fmt = rd32(b + r + 12), at = data + rd32(b + r + 4);
    if (!pal || (fmt >> 8 & 255) != 0x0B || (1u << (fmt >> 20 & 15)) != TW || (1u << (fmt >> 24 & 15)) != TH)
        return 0;
    uint32_t pat = data + rd32(b + pal + 4);
    if ((size_t)at + TW * TH > size || (size_t)pat + 1024 > size)
        return 0;
    for (int y = 0; y < TH; y++)
        for (int x = 0; x < TW; x++) {
            uint32_t idx = 0, bit = 0;         /* the swizzled place: x's and y's bits in turn, while each has more */
            for (int i = 0; (1 << i) < TW || (1 << i) < TH; i++) {
                if ((1 << i) < TW)
                    idx |= (uint32_t)(x >> i & 1) << bit++;
                if ((1 << i) < TH)
                    idx |= (uint32_t)(y >> i & 1) << bit++;
            }
            const uint8_t *c = b + pat + 4 * b[at + idx];
            float *p = px + 4 * (y * TW + x);
            p[0] = c[2] / 255.0f, p[1] = c[1] / 255.0f, p[2] = c[0] / 255.0f, p[3] = c[3] / 255.0f;
        }
    return 1;
}

/* Steps from the marked pixels (4 neighbours), 99 beyond 39 */
static void spread(const uint8_t *m, float *d)
{
    uint8_t *cur = malloc(TW * TH), *nxt = malloc(TW * TH);
    for (int i = 0; i < TW * TH; i++)
        d[i] = m[i] ? 0 : 99;
    if (!cur || !nxt) {
        free(cur);
        free(nxt);
        return;
    }
    memcpy(cur, m, TW * TH);
    for (int s = 1; s < 40; s++) {
        memcpy(nxt, cur, TW * TH);
        for (int y = 0; y < TH; y++)
            for (int x = 0; x < TW; x++) {
                int i = y * TW + x;
                if (cur[i])
                    continue;
                if ((y && cur[i - TW]) || (y < TH - 1 && cur[i + TW]) || (x && cur[i - 1]) || (x < TW - 1 && cur[i + 1])) {
                    nxt[i] = 1;
                    d[i] = (float)s;
                }
            }
        uint8_t *t = cur;
        cur = nxt;
        nxt = t;
    }
    free(cur);
    free(nxt);
}

static void blur(float *a, int w, int h, double sigma)
{
    int r = (int)ceil(sigma * 3);
    float k[32], sum = 0, *t;
    if (r > 15 || !(t = malloc(sizeof(float) * w * h)))
        return;
    for (int i = -r; i <= r; i++)
        sum += k[i + r] = (float)exp(-i * i / (2 * sigma * sigma));
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            float v = 0;
            for (int i = -r; i <= r; i++) {
                int xx = x + i < 0 ? 0 : x + i >= w ? w - 1 : x + i;
                v += a[y * w + xx] * k[i + r];
            }
            t[y * w + x] = v / sum;
        }
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            float v = 0;
            for (int i = -r; i <= r; i++) {
                int yy = y + i < 0 ? 0 : y + i >= h ? h - 1 : y + i;
                v += t[yy * w + x] * k[i + r];
            }
            a[y * w + x] = v / sum;
        }
    free(t);
}

static float clip01(float v) { return v < 0 ? 0 : v > 1 ? 1 : v; }

/* The texture's colour with alpha al (TW x TH), at (u, v) texels: bicubic, on premultiplied colour */
static void sample(const float *px, const float *al, double u, double v, float out[4])
{
    int x0 = (int)floor(u - 0.5), y0 = (int)floor(v - 0.5);
    double fx = u - 0.5 - x0, fy = v - 0.5 - y0, wx[4], wy[4];
    for (int i = 0; i < 4; i++) {          /* Catmull-Rom */
        double t = fabs(i - 1 - fx), s = fabs(i - 1 - fy);
        wx[i] = t < 1 ? 1.5 * t * t * t - 2.5 * t * t + 1 : t < 2 ? -0.5 * t * t * t + 2.5 * t * t - 4 * t + 2 : 0;
        wy[i] = s < 1 ? 1.5 * s * s * s - 2.5 * s * s + 1 : s < 2 ? -0.5 * s * s * s + 2.5 * s * s - 4 * s + 2 : 0;
    }
    double acc[4] = { 0, 0, 0, 0 };
    for (int j = 0; j < 4; j++)
        for (int i = 0; i < 4; i++) {
            int x = x0 - 1 + i, y = y0 - 1 + j;
            if (x < 0 || x >= TW || y < 0 || y >= TH)
                continue;
            double w = wx[i] * wy[j], a = al[y * TW + x];
            const float *p = px + 4 * (y * TW + x);
            acc[0] += w * a * p[0], acc[1] += w * a * p[1], acc[2] += w * a * p[2], acc[3] += w * a;
        }
    out[3] = clip01((float)acc[3]);
    for (int c = 0; c < 3; c++)
        out[c] = acc[3] > 1e-5 ? clip01((float)(acc[c] / acc[3])) : 0;
}

static void over(float *d, const float s[4], float lift)
{
    float sa = s[3], da = d[3], oa = sa + da * (1 - sa);
    if (oa <= 0)
        return;
    for (int c = 0; c < 3; c++)
        d[c] = (clip01(s[c] * lift) * sa + d[c] * da * (1 - sa)) / oa;
    d[3] = oa;
}

/* The icon, size x size, RGBA bytes (straight alpha) into out; 0 when the game's files do not give it */
int ngb_app_icon_make(const char *game_dir, int size, uint8_t *out)
{
    char path[4200];
    size_t n = 0;
    int ok = 0;
    if (!game_dir || !*game_dir || size < 8 || size > M)
        return 0;
    snprintf(path, sizeof path, "%s/demo.afs", game_dir);
    uint8_t *spr = afs_file(path, "demo.spr", &n);
    float *px = malloc(sizeof(float) * 4 * TW * TH), *dn = malloc(sizeof(float) * TW * TH),
          *dother = malloc(sizeof(float) * TW * TH), *glow = malloc(sizeof(float) * TW * TH),
          *letter = malloc(sizeof(float) * TW * TH), *big = calloc((size_t)M * M * 4, sizeof(float));
    uint8_t *solid = malloc(TW * TH), *nm = calloc(TW * TH, 1), *rest = malloc(TW * TH);
    int *stack = malloc(sizeof(int) * TW * TH);
    if (!spr || !px || !dn || !dother || !glow || !letter || !big || !solid || !nm || !rest || !stack ||
        !logo_texture(spr, n, px))
        goto done;

    /* the letter: the leftmost opaque shape of the top row ("NINJA" ends above row 140), filled out from there */
    int seed = -1, sx = TW;
    for (int i = 0; i < TW * TH; i++)
        solid[i] = px[4 * i + 3] >= 252 / 255.0f;
    for (int y = 0; y < 140; y++)
        for (int x = 0; x < sx; x++)
            if (solid[y * TW + x]) {
                sx = x;
                seed = y * TW + x;
            }
    if (seed < 0)
        goto done;
    int top = 0, x0 = TW, y0 = TH, x1 = -1, y1 = -1;
    stack[top++] = seed;
    nm[seed] = 1;
    while (top) {
        int i = stack[--top], x = i % TW, y = i / TW;
        if (x < x0) x0 = x;
        if (x > x1) x1 = x;
        if (y < y0) y0 = y;
        if (y > y1) y1 = y;
        const int nb[4] = { y ? i - TW : -1, y < TH - 1 ? i + TW : -1, x ? i - 1 : -1, x < TW - 1 ? i + 1 : -1 };
        for (int k = 0; k < 4; k++)
            if (nb[k] >= 0 && solid[nb[k]] && !nm[nb[k]]) {
                nm[nb[k]] = 1;
                stack[top++] = nb[k];
            }
    }
    if (x1 - x0 < 24 || x1 - x0 > 200 || y1 - y0 < 24)      /* not a letter of that logo */
        goto done;
    for (int i = 0; i < TW * TH; i++)
        rest[i] = solid[i] && !nm[i];
    spread(nm, dn);
    spread(rest, dother);

    /* its share of the glow: what is nearer it than another letter, dying out 26 texels away; and the letter alone */
    for (int i = 0; i < TW * TH; i++) {
        glow[i] = dn[i] <= dother[i];
        letter[i] = nm[i];
    }
    blur(glow, TW, TH, 2.5);
    blur(letter, TW, TH, 0.6);
    for (int i = 0; i < TW * TH; i++) {
        float a = px[4 * i + 3], fade = clip01((26 - dn[i]) / 8), g = glow[i] * fade;
        glow[i] = a * (g > nm[i] ? g : nm[i]);
        float e = clip01(letter[i] * 1.6f);
        letter[i] = dn[i] <= 1 ? (a < e ? a : e) : 0;
    }

    /* on the tile: the glow twice (the title draws it over black too), the letter over it; at the small sizes the
     * letter wider on the tile and its chrome rim, under a pixel there, brighter */
    const int pad = 26;
    double cw = x1 + 1 - x0 + 2 * pad, ch = y1 + 1 - y0 + 2 * pad, k = size > 32 ? 1.16 : 1.30;
    double w = M * k, scale = cw / w, h = ch / scale, ox = (M - w) / 2, oy = (M - h) / 2;
    float lift = size > 48 ? 1.0f : size > 32 ? 1.35f : 1.7f;
    const double rad = 0.19 * M;
    for (int y = 0; y < M; y++)
        for (int x = 0; x < M; x++) {
            float *d = big + 4 * ((size_t)y * M + x), s[4];
            double qx = fabs(x + 0.5 - M / 2.0) - (M / 2.0 - rad), qy = fabs(y + 0.5 - M / 2.0) - (M / 2.0 - rad);
            double sd = hypot(qx > 0 ? qx : 0, qy > 0 ? qy : 0) + fmin(fmax(qx, qy), 0) - rad;
            float tile = clip01((float)(0.5 - sd));
            if (tile <= 0)
                continue;
            d[3] = 1;                                  /* black */
            double u = x0 - pad + (x + 0.5 - ox) * scale, v = y0 - pad + (y + 0.5 - oy) * scale;
            if (u > -2 && u < TW + 2 && v > -2 && v < TH + 2) {
                sample(px, glow, u, v, s);
                over(d, s, 1);
                over(d, s, 1);
                sample(px, letter, u, v, s);
                over(d, s, lift);
            }
            d[3] = tile;
        }

    /* reduced by area; the small sizes sharpened a little, so the rim stays crisp */
    float *sm = malloc(sizeof(float) * 4 * size * size), *bl = malloc(sizeof(float) * size * size);
    if (sm && bl) {
        double f = (double)M / size;
        for (int y = 0; y < size; y++)
            for (int x = 0; x < size; x++) {
                double acc[4] = { 0, 0, 0, 0 }, wsum = 0;
                double ya = y * f, yb = (y + 1) * f, xa = x * f, xb = (x + 1) * f;
                for (int yy = (int)ya; yy < yb && yy < M; yy++)
                    for (int xx = (int)xa; xx < xb && xx < M; xx++) {
                        double wy = fmin(yb, yy + 1) - fmax(ya, yy), wxx = fmin(xb, xx + 1) - fmax(xa, xx), ww = wy * wxx;
                        const float *p = big + 4 * ((size_t)yy * M + xx);
                        acc[0] += ww * p[3] * p[0], acc[1] += ww * p[3] * p[1], acc[2] += ww * p[3] * p[2];
                        acc[3] += ww * p[3];
                        wsum += ww;
                    }
                float *o = sm + 4 * (y * size + x);
                for (int c = 0; c < 3; c++)
                    o[c] = acc[3] > 1e-6 ? (float)(acc[c] / acc[3]) : 0;
                o[3] = wsum > 0 ? (float)(acc[3] / wsum) : 0;
            }
        if (size < 128) {
            float amount = size <= 32 ? 0.9f : 0.5f;
            for (int c = 0; c < 3; c++) {
                for (int i = 0; i < size * size; i++)
                    bl[i] = sm[4 * i + c];
                blur(bl, size, size, 0.6);
                for (int i = 0; i < size * size; i++)
                    sm[4 * i + c] = clip01(sm[4 * i + c] + amount * (sm[4 * i + c] - bl[i]));
            }
        }
        for (int i = 0; i < size * size * 4; i++)
            out[i] = (uint8_t)(clip01(sm[i]) * 255 + 0.5f);
        ok = 1;
    }
    free(sm);
    free(bl);
done:
    free(spr); free(px); free(dn); free(dother); free(glow); free(letter); free(big);
    free(solid); free(nm); free(rest); free(stack);
    return ok;
}

static char s_dir[4096];
static uint8_t *s_icon;            /* 256 x 256, made once (NULL: not yet, or not to be had) */
static int s_tried;

static const uint8_t *icon256(void)
{
    if (!s_tried) {
        s_tried = 1;
        s_icon = malloc(256 * 256 * 4);
        if (s_icon && !ngb_app_icon_make(s_dir, 256, s_icon)) {
            free(s_icon);
            s_icon = NULL;
        }
    }
    return s_icon;
}

/* The window's icon (a window of ours just made): Windows' taskbar and title bar, X11's. Nothing without the game's
 * files -- the program's own stays. */
void ngb_app_icon_window(void *sdl_window)
{
#ifndef __ANDROID__
    const uint8_t *px = sdl_window ? icon256() : NULL;
    if (!px)
        return;
    SDL_Surface *s = SDL_CreateRGBSurfaceWithFormatFrom((void *)px, 256, 256, 32, 256 * 4, SDL_PIXELFORMAT_RGBA32);
    if (s) {
        SDL_SetWindowIcon((SDL_Window *)sdl_window, s);
        SDL_FreeSurface(s);
    }
#else
    (void)sdl_window;
#endif
}

#if defined(__linux__) && !defined(__ANDROID__)
static void mkdirs(char *path)     /* every folder of the path (not its last part, a file) */
{
    for (char *p = path + 1; *p; p++)
        if (*p == '/') {
            *p = 0;
            mkdir(path, 0755);
            *p = '/';
        }
}

#endif

/* Once the game's folder is known (src/main.c). On Linux the desktop's icon files, written when they are not there
 * or are of another ICON_VERSION (NGB_ICON_REFRESH=1: again). */
void ngb_app_icon_setup(const char *game_dir)
{
    snprintf(s_dir, sizeof s_dir, "%s", game_dir ? game_dir : "");
    const char *dump = getenv("NGB_ICON_DUMP");       /* testing: <prefix>_<size>.png */
    if (dump && *dump) {
        static const int sizes[] = { 256, 128, 64, 48, 32, 16 };
        for (int i = 0; i < 6; i++) {
            char path[4200];
            uint8_t *px = malloc((size_t)sizes[i] * sizes[i] * 4);
            snprintf(path, sizeof path, "%s_%d.png", dump, sizes[i]);
            if (px && ngb_app_icon_make(s_dir, sizes[i], px))
                stbi_write_png(path, sizes[i], sizes[i], 4, px, sizes[i] * 4);
            free(px);
        }
    }
#if defined(__linux__) && !defined(__ANDROID__)
    if (getenv("NGB_NO_ICON") || getenv("NGB_TEST_HIDDEN"))
        return;
    const char *xdg = getenv("XDG_DATA_HOME"), *home = getenv("HOME");
    char base[4200], path[4400];
    if (xdg && *xdg)
        snprintf(base, sizeof base, "%s", xdg);
    else if (home && *home)
        snprintf(base, sizeof base, "%s/.local/share", home);
    else
        return;
    char stamp[4400], want[32], have[32] = "";
    snprintf(stamp, sizeof stamp, "%s/icons/hicolor/256x256/apps/" APP_ID ".version", base);
    snprintf(want, sizeof want, "%d\n", ICON_VERSION);
    FILE *f = fopen(stamp, "r");
    if (f) {
        if (!fgets(have, sizeof have, f))
            have[0] = 0;
        fclose(f);
    }
    snprintf(path, sizeof path, "%s/icons/hicolor/256x256/apps/" APP_ID ".png", base);
    struct stat st;
    if (stat(path, &st) == 0 && !strcmp(have, want) && !getenv("NGB_ICON_REFRESH"))
        return;
    static const int sizes[] = { 256, 64, 48, 32 };
    for (int i = 0; i < 4; i++) {
        uint8_t *px = malloc((size_t)sizes[i] * sizes[i] * 4);
        snprintf(path, sizeof path, "%s/icons/hicolor/%dx%d/apps/" APP_ID ".png", base, sizes[i], sizes[i]);
        int made = px && ngb_app_icon_make(s_dir, sizes[i], px);
        if (made) {
            mkdirs(path);
            stbi_write_png(path, sizes[i], sizes[i], 4, px, sizes[i] * 4);
        }
        free(px);
        if (!made)
            return;                    /* (the game's files do not give it: nothing written) */
    }
    if ((f = fopen(stamp, "w"))) {
        fputs(want, f);
        fclose(f);
    }
    fprintf(stderr, "icon: written under %s/icons/hicolor\n", base);
#endif
}

/* Before SDL's video starts (src/main.c): the windows' app id, the desktop entry's name (its StartupWMClass,
 * packaging/appimage/build.sh), so Wayland's compositors and X11's task bars give them that entry's icon. SDL2
 * reads it from these variables when it makes a window (SDL3, under sdl2-compat, from SDL_APP_ID); one the
 * player set stays. */
void ngb_app_id_setup(void)
{
#if defined(__linux__) && !defined(__ANDROID__)
    setenv("SDL_VIDEO_WAYLAND_WMCLASS", APP_ID, 0);
    setenv("SDL_VIDEO_X11_WMCLASS", APP_ID, 0);
    setenv("SDL_APP_ID", APP_ID, 0);
#endif
}

/* (xboxrecomp's present_sdl.c: the game window, just made) */
void nv2a_window_made(void *sdl_window)
{
    ngb_app_icon_window(sdl_window);
}
