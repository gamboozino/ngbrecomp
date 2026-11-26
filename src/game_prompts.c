/*
 * game_prompts.c -- the button prompts (PlayStation, Nintendo, keyboard and mouse, Xbox) and the touch controls'
 * pieces, made at run time from the player's own copy of the game: its button icons (the X Y B A L R texture,
 * sprites 5-10; the Black and White buttons, 0xB and 0xC; the d-pad, 0xD) and its menus' small font, read from
 * guest memory once they are loaded. Nothing cut from the game is kept in the repository or shipped; the only
 * things written are the touch pieces, on Android (NGB_TOUCH_ART=1 elsewhere, for testing), in the save folder
 * (<save folder>/touch-art, so the next start has them from its first frame; on Android that is the app's
 * external files folder, which Android's own backup may include), and NGB_PROMPT_DUMP=<dir>'s pictures for
 * comparing. Until the textures are read the prompts are typed (src/pause_tab.c).
 *
 *   PlayStation, Nintendo: the game's face disc with its colour and letter taken out (the median of the four),
 *       dark, its rim and glint kept, the symbols drawn or the menu font's letters; the triggers and bumpers the
 *       game's L / R plate with its letter wiped and "L2", "ZL"... drawn in the plate letters' cut.
 *   Xbox: the game's own pictures enlarged smoothly; Start / Back the Black button's ball with a triangle; the
 *       sticks the bare disc with an L / R; the d-pad the game's icon, the arm meant lit. With Button Resolution
 *       High the face buttons, the balls and the d-pad drawn again as the touch pieces are (below), at 4x.
 *   Every prompt is brought down to the game's own resolution (game_res), as its buttons beside them look,
 *       unless Button Resolution is High.
 *   Keys: the plate wiped, the legend in the game's font enlarged; arrows, the Windows key and the mouse drawn.
 *   Tabs: Q and E for the pause menu's L / R plates, the plate's shape and glow drawn at 4x.
 *   Touch: every piece drawn at its own size (16 pixels a game pixel, 384 x 384), none an enlarged icon: the gems,
 *       the Black and White balls, the plates, the d-pad and the sticks' wells are the icons' shapes drawn again,
 *       their colours and light read from the game's textures; each held piece a picture of its own.
 *
 * The shapes are coverage drawn supersampled 8 times and box-filtered, scan-converted the way the mock-ups'
 * drawing library (Pillow) does, and enlarged with its bicubic filter, so these match the approved mock-ups
 * (build/promptdump) closely. The resampling, polygon, ellipse and wide-line code is ported from Pillow:
 *   Copyright (c) 1997-2011 by Secret Labs AB; Copyright (c) 1995-2011 by Fredrik Lundh and contributors;
 *   Copyright (c) 2010 by Jeffrey A. Clark and contributors. MIT-CMU (HPND) licence: see src/LICENSE-Pillow.txt
 *   (its full copyright and permission notice, shipped with every package).
 *
 * Threads: the prompts are made on the game thread (src/pause_tab.c); the touch pieces on a thread of their own,
 * published once all are made. The game's textures are read on the game thread and never change after.
 */
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif

#ifndef GP_STANDALONE
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"
#endif
#include "game_prompts.h"

/* ---------- scratch memory: everything a picture is made from, freed once it is made ---------- */

static _Thread_local struct { void **p; int n, cap; } t_arena;

static void *tmp(size_t n)
{
    void *q = calloc(1, n ? n : 1);
    if (!q)
        abort();
    if (t_arena.n == t_arena.cap) {
        t_arena.cap = t_arena.cap ? t_arena.cap * 2 : 256;
        t_arena.p = realloc(t_arena.p, sizeof(void *) * t_arena.cap);
        if (!t_arena.p)
            abort();
    }
    t_arena.p[t_arena.n++] = q;
    return q;
}

static void tmp_release(void)
{
    for (int i = 0; i < t_arena.n; i++)
        free(t_arena.p[i]);
    t_arena.n = 0;
}

typedef struct { int w, h; float *p; } Im;      /* RGBA, straight alpha, 0..1 */
typedef struct { int w, h; float *m; } Mk;      /* coverage */

#define PX(a, x, y) (&(a).p[4 * ((size_t)(y) * (a).w + (x))])
#define MV(a, x, y) ((a).m[(size_t)(y) * (a).w + (x)])

static Im im_new(int w, int h) { Im a = { w, h, tmp(sizeof(float) * 4 * (size_t)w * h) }; return a; }
static Mk mk_new(int w, int h) { Mk a = { w, h, tmp(sizeof(float) * (size_t)w * h) }; return a; }

static float clip01(double v) { return v < 0 ? 0.0f : v > 1 ? 1.0f : (float)v; }
static double clampd(double v, double lo, double hi) { return v < lo ? lo : v > hi ? hi : v; }
static int pyround(double v) { return (int)rint(v); }            /* Python's round(): halves to even */
static double lum3(const float *c) { return 0.299f * c[0] + 0.587f * c[1] + 0.114f * c[2]; }

static Im im_copy(Im a)
{
    Im o = im_new(a.w, a.h);
    memcpy(o.p, a.p, sizeof(float) * 4 * (size_t)a.w * a.h);
    return o;
}

static Im crop(Im a, int x0, int y0, int w, int h)
{
    Im o = im_new(w, h);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            if (x0 + x < a.w && y0 + y < a.h)
                memcpy(PX(o, x, y), PX(a, x0 + x, y0 + y), sizeof(float) * 4);
    return o;
}

/* src over dst at (x, y), straight alpha */
static void over(Im d, Im s, int x, int y)
{
    int x0 = x > 0 ? x : 0, y0 = y > 0 ? y : 0, x1 = x + s.w < d.w ? x + s.w : d.w, y1 = y + s.h < d.h ? y + s.h : d.h;
    for (int Y = y0; Y < y1; Y++)
        for (int X = x0; X < x1; X++) {
            float *q = PX(s, X - x, Y - y), *p = PX(d, X, Y);
            float sa = q[3], da = p[3], oa = sa + da * (1 - sa);
            for (int c = 0; c < 3; c++)
                p[c] = oa > 0 ? (q[c] * sa + p[c] * da * (1 - sa)) / (oa > 1e-6f ? oa : 1e-6f) : 0;
            p[3] = oa;
        }
}

/* col painted through a coverage mask onto dst's colour, dst's alpha kept */
static void ink(Im d, Mk m, const float col[3], int x, int y, float a)
{
    int x0 = x > 0 ? x : 0, y0 = y > 0 ? y : 0, x1 = x + m.w < d.w ? x + m.w : d.w, y1 = y + m.h < d.h ? y + m.h : d.h;
    for (int Y = y0; Y < y1; Y++)
        for (int X = x0; X < x1; X++) {
            float v = MV(m, X - x, Y - y) * a, *p = PX(d, X, Y);
            for (int c = 0; c < 3; c++)
                p[c] = p[c] * (1 - v) + col[c] * v;
        }
}

static Im nearest(Im a, int k)
{
    Im o = im_new(a.w * k, a.h * k);
    for (int y = 0; y < o.h; y++)
        for (int x = 0; x < o.w; x++)
            memcpy(PX(o, x, y), PX(a, x / k, y / k), sizeof(float) * 4);
    return o;
}

static Mk mk_min(Mk a, Mk b)
{
    Mk o = mk_new(a.w, a.h);
    for (int i = 0; i < a.w * a.h; i++)
        o.m[i] = a.m[i] < b.m[i] ? a.m[i] : b.m[i];
    return o;
}

/* ---------- Pillow's resampling (float images): bicubic, two passes, horizontal first ---------- */

static double bicubic(double x)
{
    const double a = -0.5;
    if (x < 0)
        x = -x;
    if (x < 1)
        return ((a + 2) * x - (a + 3)) * x * x + 1;
    if (x < 2)
        return (((x - 5) * x + 8) * x - 4) * a;
    return 0;
}

static int coeffs(int in, int out, int **bounds, double **kk)
{
    double scale = (double)in / out, fs = scale < 1 ? 1 : scale, support = 2.0 * fs;
    int ksize = (int)ceil(support) * 2 + 1;
    *kk = tmp(sizeof(double) * (size_t)out * ksize);
    *bounds = tmp(sizeof(int) * 2 * (size_t)out);
    for (int xx = 0; xx < out; xx++) {
        double center = (xx + 0.5) * scale, ww = 0;
        int xmin = (int)(center - support + 0.5), xmax;
        if (xmin < 0)
            xmin = 0;
        xmax = (int)(center + support + 0.5);
        if (xmax > in)
            xmax = in;
        xmax -= xmin;
        double *k = &(*kk)[xx * ksize];
        for (int x = 0; x < xmax; x++) {
            k[x] = bicubic((x + xmin - center + 0.5) / fs);
            ww += k[x];
        }
        if (ww != 0)
            for (int x = 0; x < xmax; x++)
                k[x] /= ww;
        (*bounds)[2 * xx] = xmin;
        (*bounds)[2 * xx + 1] = xmax;
    }
    return ksize;
}

static float *resize_f(const float *in, int iw, int ih, int ow, int oh)
{
    const float *cur = in;
    int cw = iw;
    int horizontal_first = !((ih - oh) > 0 && (ih - oh) > (iw - ow) * 2);
    float *out = NULL;
    for (int pass = 0; pass < 2; pass++) {
        int horiz = horizontal_first ? pass == 0 : pass == 1;
        if (horiz) {
            if (ow == iw)
                continue;
            int *b;
            double *kk;
            int ks = coeffs(iw, ow, &b, &kk);
            int rows = cur == in ? ih : oh;
            float *o = tmp(sizeof(float) * (size_t)ow * rows);
            for (int y = 0; y < rows; y++)
                for (int x = 0; x < ow; x++) {
                    double ss = 0;
                    for (int i = 0; i < b[2 * x + 1]; i++)
                        ss += cur[(size_t)y * cw + b[2 * x] + i] * kk[x * ks + i];
                    o[(size_t)y * ow + x] = (float)ss;
                }
            cur = o;
            cw = ow;
        } else {
            if (oh == ih)
                continue;
            int *b;
            double *kk;
            int ks = coeffs(ih, oh, &b, &kk);
            float *o = tmp(sizeof(float) * (size_t)cw * oh);
            for (int y = 0; y < oh; y++)
                for (int x = 0; x < cw; x++) {
                    double ss = 0;
                    for (int i = 0; i < b[2 * y + 1]; i++)
                        ss += cur[(size_t)(b[2 * y] + i) * cw + x] * kk[y * ks + i];
                    o[(size_t)y * cw + x] = (float)ss;
                }
            cur = o;
        }
    }
    out = tmp(sizeof(float) * (size_t)ow * oh);
    memcpy(out, cur, sizeof(float) * (size_t)ow * oh);
    return out;
}

static Mk resize_mk(Mk m, int ow, int oh)
{
    Mk o = { ow, oh, resize_f(m.m, m.w, m.h, ow, oh) };
    return o;
}

/* A k-times enlargement without new detail: bicubic on premultiplied colour */
static Im smooth(Im a, int k)
{
    if (k == 1)
        return im_copy(a);
    int n = a.w * a.h, W = a.w * k, H = a.h * k;
    float *ch[4];
    for (int c = 0; c < 4; c++) {
        float *pl = tmp(sizeof(float) * n);
        for (int i = 0; i < n; i++)
            pl[i] = c < 3 ? a.p[4 * i + c] * a.p[4 * i + 3] : a.p[4 * i + 3];
        ch[c] = resize_f(pl, a.w, a.h, W, H);
    }
    Im o = im_new(W, H);
    for (int i = 0; i < W * H; i++) {
        float al = clip01(ch[3][i]);
        for (int c = 0; c < 3; c++) {
            float v = clip01(ch[c][i]);
            o.p[4 * i + c] = clip01(al > 1e-4f ? v / (al > 1e-4f ? al : 1e-4f) : 0);
        }
        o.p[4 * i + 3] = al;
    }
    return o;
}

static Mk smooth_mask(Mk m, int k)
{
    Mk o = resize_mk(m, m.w * k, m.h * k);
    for (int i = 0; i < o.w * o.h; i++)
        o.m[i] = clip01(o.m[i]);
    return o;
}

/* Gaussian blur, the edge carried on (numpy's pad 'edge', convolved down the columns then along the rows) */
static Mk blur(Mk m, double sigma)
{
    int r = (int)(sigma * 3 + 0.5);
    if (r < 1)
        r = 1;
    double *k = tmp(sizeof(double) * (2 * r + 1)), sum = 0;
    for (int i = -r; i <= r; i++)
        sum += k[i + r] = exp(-0.5 * (i / sigma) * (i / sigma));
    for (int i = 0; i <= 2 * r; i++)
        k[i] /= sum;
    int w = m.w, h = m.h;
    double *t = tmp(sizeof(double) * (size_t)w * h);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            double s = 0;
            for (int i = -r; i <= r; i++) {
                int yy = y + i < 0 ? 0 : y + i >= h ? h - 1 : y + i;
                s += m.m[(size_t)yy * w + x] * k[i + r];
            }
            t[(size_t)y * w + x] = s;
        }
    Mk o = mk_new(w, h);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            double s = 0;
            for (int i = -r; i <= r; i++) {
                int xx = x + i < 0 ? 0 : x + i >= w ? w - 1 : x + i;
                s += t[(size_t)y * w + xx] * k[i + r];
            }
            o.m[(size_t)y * w + x] = (float)s;
        }
    return o;
}

static Mk trim(Mk m, float thr)
{
    int x0 = m.w, y0 = m.h, x1 = -1, y1 = -1;
    for (int y = 0; y < m.h; y++)
        for (int x = 0; x < m.w; x++)
            if (MV(m, x, y) > thr) {
                if (x < x0) x0 = x;
                if (x > x1) x1 = x;
                if (y < y0) y0 = y;
                if (y > y1) y1 = y;
            }
    if (x1 < 0) {
        Mk e = mk_new(1, 1);
        return e;
    }
    Mk o = mk_new(x1 - x0 + 1, y1 - y0 + 1);
    for (int y = 0; y < o.h; y++)
        for (int x = 0; x < o.w; x++)
            MV(o, x, y) = MV(m, x0 + x, y0 + y);
    return o;
}

static Mk mk_rows(Mk m, int y0, int y1)
{
    Mk o = mk_new(m.w, y1 - y0);
    memcpy(o.m, &m.m[(size_t)y0 * m.w], sizeof(float) * (size_t)m.w * (y1 - y0));
    return o;
}

/* ---------- coverage drawing: Pillow's scan conversion on an 8-times canvas, box-filtered down ---------- */

#define SS 8

typedef struct { int w, h, s, k, W, H; uint8_t *px; } Cv;

static Cv cv_new(int w, int h, int s)
{
    Cv c = { w, h, s, s * SS, w * s * SS, h * s * SS, NULL };
    c.px = tmp((size_t)c.W * c.H);
    return c;
}

static void hline8(Cv *c, int x0, int y0, int x1, int ink)
{
    if (y0 < 0 || y0 >= c->H)
        return;
    if (x0 < 0)
        x0 = 0;
    else if (x0 >= c->W)
        return;
    if (x1 < 0)
        return;
    else if (x1 >= c->W)
        x1 = c->W - 1;
    if (x0 <= x1)
        memset(c->px + (size_t)y0 * c->W + x0, ink, (size_t)(x1 - x0 + 1));
}

#define ROUND_UP(f) ((int)((f) >= 0.0 ? floor((f) + 0.5F) : -floor(fabs(f) + 0.5F)))
#define ROUND_DOWN(f) ((int)((f) >= 0.0 ? ceil((f) - 0.5F) : -ceil(fabs(f) - 0.5F)))

typedef struct { int d, x0, y0, xmin, ymin, xmax, ymax; float dx; } Edge;

static void add_edge(Edge *e, int x0, int y0, int x1, int y1)
{
    if (x0 <= x1) { e->xmin = x0; e->xmax = x1; } else { e->xmin = x1; e->xmax = x0; }
    if (y0 <= y1) { e->ymin = y0; e->ymax = y1; } else { e->ymin = y1; e->ymax = y0; }
    if (y0 == y1) {
        e->d = 0;
        e->dx = 0;
    } else {
        e->dx = ((float)(x1 - x0)) / (y1 - y0);
        e->d = y0 == e->ymin ? 1 : -1;
    }
    e->x0 = x0;
    e->y0 = y0;
}

static int x_cmp(const void *a, const void *b)
{
    float x = *(const float *)a, y = *(const float *)b;
    return x < y ? -1 : x > y ? 1 : 0;
}

static void polygon_generic(Cv *c, int n, Edge *e, int ink)
{
    if (n <= 0)
        return;
    Edge **tab = tmp(sizeof(Edge *) * n);
    int count = 0, ymin = c->H - 1, ymax = 0;
    for (int i = 0; i < n; i++) {
        if (ymin > e[i].ymin) ymin = e[i].ymin;
        if (ymax < e[i].ymax) ymax = e[i].ymax;
        if (e[i].ymin == e[i].ymax) {
            hline8(c, e[i].xmin, e[i].ymin, e[i].xmax, ink);
            continue;
        }
        tab[count++] = e + i;
    }
    if (ymin < 0) ymin = 0;
    if (ymax > c->H) ymax = c->H;
    float *xx = tmp(sizeof(float) * (count * 2 + 1));
    for (; ymin <= ymax; ymin++) {
        int j = 0;
        for (int i = 0; i < count; i++) {
            Edge *cur = tab[i];
            if (ymin < cur->ymin || ymin > cur->ymax)
                continue;
            xx[j++] = (ymin - cur->y0) * cur->dx + cur->x0;
            if (ymin == cur->ymax && ymin < ymax) {
                xx[j] = xx[j - 1];
                j++;
            } else if ((ymin == cur->ymin || ymin == cur->ymax) && cur->dx != 0) {
                for (int k = 0; k < i; k++) {           /* (corners joined as Pillow joins them) */
                    Edge *o = tab[k];
                    if ((ymin != o->ymin && ymin != o->ymax) || o->dx == 0)
                        continue;
                    if (roundf(xx[j - 1]) == roundf((ymin - o->y0) * o->dx + o->x0)) {
                        int off = ymin == cur->ymax ? -1 : 1;
                        float adj = (ymin + off - cur->y0) * cur->dx + cur->x0;
                        if (ymin + off >= o->ymin && ymin + off <= o->ymax) {
                            float adj2 = (ymin + off - o->y0) * o->dx + o->x0;
                            if (xx[j - 1] > adj + 1 && xx[j - 1] > adj2 + 1)
                                xx[j - 1] = roundf(fmax(adj, adj2)) + 1;
                            else if (xx[j - 1] < adj - 1 && xx[j - 1] < adj2 - 1)
                                xx[j - 1] = roundf(fmin(adj, adj2)) - 1;
                            break;
                        }
                    }
                }
            }
        }
        qsort(xx, j, sizeof(float), x_cmp);
        for (int i = 1; i < j; i += 2)
            hline8(c, ROUND_UP(xx[i - 1]), ymin, ROUND_DOWN(xx[i]), ink);
    }
}

static void draw_polygon(Cv *c, const int *xy, int count, int ink)
{
    if (count <= 0)
        return;
    Edge *e = tmp(sizeof(Edge) * (count + 1));
    int i, n;
    for (i = n = 0; i < count - 1; i++) {
        int x0 = xy[i * 2], y0 = xy[i * 2 + 1], x1 = xy[i * 2 + 2], y1 = xy[i * 2 + 3];
        if (y0 == y1 && i != 0 && y0 == xy[i * 2 - 1]) {
            Edge *last = &e[n - 1];
            if (x1 > x0 && x0 > xy[i * 2 - 2]) {
                last->xmax = x1;
                continue;
            } else if (x1 < x0 && x0 < xy[i * 2 - 2]) {
                last->xmin = x1;
                continue;
            }
        }
        add_edge(&e[n++], x0, y0, x1, y1);
    }
    if (xy[i * 2] != xy[0] || xy[i * 2 + 1] != xy[1])
        add_edge(&e[n++], xy[i * 2], xy[i * 2 + 1], xy[0], xy[1]);
    polygon_generic(c, n, e, ink);
}

static void wide_line(Cv *c, int x0, int y0, int x1, int y1, int width, int ink)
{
    int dx = x1 - x0, dy = y1 - y0;
    if (!dx && !dy) {
        hline8(c, x0, y0, x0, ink);
        return;
    }
    double big = hypot(dx, dy), small = (width - 1) / 2.0;
    double rmax = ROUND_UP(small) / big, rmin = ROUND_DOWN(small) / big;
    int dxmin = ROUND_DOWN(rmin * dy), dxmax = ROUND_DOWN(rmax * dy), dymin = ROUND_DOWN(rmin * dx),
        dymax = ROUND_DOWN(rmax * dx);
    int v[4][2] = { { x0 - dxmin, y0 + dymax }, { x1 - dxmin, y1 + dymax }, { x1 + dxmax, y1 - dymin },
                    { x0 + dxmax, y0 - dymin } };
    Edge e[4];
    for (int i = 0; i < 4; i++)
        add_edge(&e[i], v[i][0], v[i][1], v[(i + 1) % 4][0], v[(i + 1) % 4][1]);
    polygon_generic(c, 4, e, ink);
}

/* Pillow's ellipse: its quarter tracer on a grid of step 2, joined into rows */
typedef struct { int32_t a, b, cx, cy, ex, ey; int64_t a2, b2, a2b2; int8_t finished; } QuarterState;

static void quarter_init(QuarterState *s, int32_t a, int32_t b)
{
    if (a < 0 || b < 0) {
        s->finished = 1;
        return;
    }
    s->a = a; s->b = b; s->cx = a; s->cy = b % 2; s->ex = a % 2; s->ey = b;
    s->a2 = (int64_t)a * a; s->b2 = (int64_t)b * b; s->a2b2 = s->a2 * s->b2;
    s->finished = 0;
}

static int64_t quarter_delta(QuarterState *s, int64_t x, int64_t y)
{
    int64_t v = s->a2 * y * y + s->b2 * x * x - s->a2b2;
    return v < 0 ? -v : v;
}

static int quarter_next(QuarterState *s, int32_t *rx, int32_t *ry)
{
    if (s->finished)
        return -1;
    *rx = s->cx;
    *ry = s->cy;
    if (s->cx == s->ex && s->cy == s->ey) {
        s->finished = 1;
    } else {
        int32_t nx = s->cx, ny = s->cy + 2;
        int64_t nd = quarter_delta(s, nx, ny);
        if (nx > 1) {
            int64_t d2 = quarter_delta(s, s->cx - 2, s->cy + 2);
            if (nd > d2) { nx = s->cx - 2; ny = s->cy + 2; nd = d2; }
            d2 = quarter_delta(s, s->cx - 2, s->cy);
            if (nd > d2) { nx = s->cx - 2; ny = s->cy; }
        }
        s->cx = nx;
        s->cy = ny;
    }
    return 0;
}

typedef struct {
    QuarterState o, i;
    int32_t py, pl, pr, cy[4], cl[4], cr[4];
    int8_t bufcnt, finished, leftmost;
} EllState;

static void ellipse_init(EllState *s, int32_t a, int32_t b, int32_t w)
{
    s->bufcnt = 0;
    s->leftmost = a % 2;
    quarter_init(&s->o, a, b);
    if (w < 1 || quarter_next(&s->o, &s->pr, &s->py) == -1) {
        s->finished = 1;
    } else {
        s->finished = 0;
        quarter_init(&s->i, a - 2 * (w - 1), b - 2 * (w - 1));
        s->pl = s->leftmost;
    }
}

static int ellipse_next(EllState *s, int32_t *x0, int32_t *ry, int32_t *x1)
{
    if (s->bufcnt == 0) {
        if (s->finished)
            return -1;
        int32_t y = s->py, l = s->pl, r = s->pr, cx = 0, cy = 0;
        int nr;
        while ((nr = quarter_next(&s->o, &cx, &cy)) != -1 && cy <= y) {
        }
        if (nr == -1) {
            s->finished = 1;
        } else {
            s->pr = cx;
            s->py = cy;
        }
        while ((nr = quarter_next(&s->i, &cx, &cy)) != -1 && cy <= y)
            l = cx;
        s->pl = nr == -1 ? s->leftmost : cx;
        if ((l > 0 || l < r) && y > 0) {
            s->cl[s->bufcnt] = l == 0 ? 2 : l; s->cy[s->bufcnt] = y; s->cr[s->bufcnt] = r; ++s->bufcnt;
        }
        if (y > 0) {
            s->cl[s->bufcnt] = -r; s->cy[s->bufcnt] = y; s->cr[s->bufcnt] = -l; ++s->bufcnt;
        }
        if (l > 0 || l < r) {
            s->cl[s->bufcnt] = l == 0 ? 2 : l; s->cy[s->bufcnt] = -y; s->cr[s->bufcnt] = r; ++s->bufcnt;
        }
        s->cl[s->bufcnt] = -r; s->cy[s->bufcnt] = -y; s->cr[s->bufcnt] = -l; ++s->bufcnt;
    }
    --s->bufcnt;
    *x0 = s->cl[s->bufcnt];
    *ry = s->cy[s->bufcnt];
    *x1 = s->cr[s->bufcnt];
    return 0;
}

/* A filled ellipse in the box (x0, y0)-(x1, y1), only what lies in the clip box drawn */
static void ellipse_fill(Cv *c, int x0, int y0, int x1, int y1, int ink, int cx0, int cy0, int cx1, int cy1)
{
    int a = x1 - x0, b = y1 - y0;
    if (a < 0 || b < 0)
        return;
    EllState st;
    ellipse_init(&st, a, b, a + b);
    int32_t X0, Y, X1;
    while (ellipse_next(&st, &X0, &Y, &X1) != -1) {
        int y = y0 + (Y + b) / 2, l = x0 + (X0 + a) / 2, r = x0 + (X1 + a) / 2;
        if (y < cy0 || y > cy1)
            continue;
        if (l < cx0) l = cx0;
        if (r > cx1) r = cx1;
        if (l <= r)
            hline8(c, l, y, r, ink);
    }
}

typedef struct { double xy[2 * 192]; int n; } Pts;

static void pt(Pts *p, double x, double y)
{
    if (p->n < 192) {
        p->xy[2 * p->n] = x;
        p->xy[2 * p->n + 1] = y;
        p->n++;
    }
}

static void cv_poly(Cv *c, const Pts *p, int v)
{
    int *xy = tmp(sizeof(int) * 2 * (p->n + 1));
    for (int i = 0; i < 2 * p->n; i++)
        xy[i] = (int)(p->xy[i] * c->k);
    draw_polygon(c, xy, p->n, v);
}

static void cv_rect(Cv *c, double x0, double y0, double x1, double y1, int v)
{
    int X0 = (int)(x0 * c->k), Y0 = (int)(y0 * c->k), X1 = (int)(x1 * c->k - 1), Y1 = (int)(y1 * c->k - 1);
    if (Y0 > Y1) { int t = Y0; Y0 = Y1; Y1 = t; }
    if (Y0 < 0) Y0 = 0;
    else if (Y0 >= c->H) return;
    if (Y1 < 0) return;
    else if (Y1 > c->H) Y1 = c->H;
    for (int y = Y0; y <= Y1; y++)
        hline8(c, X0, y, X1, v);
}

static void cv_ellipse(Cv *c, double cx, double cy, double rx, double ry, int v)
{
    double k = c->k;
    ellipse_fill(c, (int)((cx - rx) * k), (int)((cy - ry) * k), (int)((cx + rx) * k - 1), (int)((cy + ry) * k - 1), v,
                 INT32_MIN, INT32_MIN, INT32_MAX, INT32_MAX);
}

static void cv_disc(Cv *c, double cx, double cy, double r, int v) { cv_ellipse(c, cx, cy, r, r, v); }

/* A line of width w (Pillow's wide line per segment); round_ends: a disc on each point, else on the inner ones */
static void cv_stroke(Cv *c, const Pts *p, double w, int v, int round_ends)
{
    int width = pyround(w * c->k);
    if (width < 1)
        width = 1;
    for (int i = 0; i + 1 < p->n; i++)
        wide_line(c, (int)(p->xy[2 * i] * c->k), (int)(p->xy[2 * i + 1] * c->k), (int)(p->xy[2 * i + 2] * c->k),
                  (int)(p->xy[2 * i + 3] * c->k), width, v);
    for (int i = round_ends ? 0 : 1; i < (round_ends ? p->n : p->n - 1); i++)
        cv_disc(c, p->xy[2 * i], p->xy[2 * i + 1], w / 2, v);
}

/* Pillow's rounded rectangle: its corners' pie slices as the quarters of their ellipses */
static void cv_rrect(Cv *c, double x0, double y0, double x1, double y1, double r, int v)
{
    double k = c->k, fx0 = x0 * k, fy0 = y0 * k, fx1 = x1 * k - 1, fy1 = y1 * k - 1, rad = r * k;
    double d = fmin(fmin(fx1 - fx0, fy1 - fy0), rad * 2);
    int X0 = pyround(fx0), Y0 = pyround(fy0), X1 = pyround(fx1), Y1 = pyround(fy1);
    int full_x = d >= X1 - X0 - 1, full_y;
    if (full_x)
        d = X1 - X0;
    full_y = d >= Y1 - Y0 - 1;
    if (full_y)
        d = Y1 - Y0;
    if (full_x && full_y) {
        ellipse_fill(c, (int)fx0, (int)fy0, (int)fx1, (int)fy1, v, INT32_MIN, INT32_MIN, INT32_MAX, INT32_MAX);
        return;
    }
    if (d == 0) {
        for (int y = (int)fy0; y <= (int)fy1; y++)
            hline8(c, (int)fx0, y, (int)fx1, v);
        return;
    }
    int R = (int)floor(d / 2);
    int di = (int)d;
    if (full_x) {
        ellipse_fill(c, X0, Y0, X0 + di, Y0 + di, v, X0, Y0, X1, Y0 + R);
        ellipse_fill(c, X0, Y1 - di, X0 + di, Y1, v, X0, Y1 - R, X1, Y1);
    } else if (full_y) {
        ellipse_fill(c, X0, Y0, X0 + di, Y0 + di, v, X0, Y0, X0 + R, Y1);
        ellipse_fill(c, X1 - di, Y0, X1, Y0 + di, v, X1 - R, Y0, X1, Y1);
    } else {
        ellipse_fill(c, X0, Y0, X0 + di, Y0 + di, v, X0, Y0, X0 + R, Y0 + R);
        ellipse_fill(c, X1 - di, Y0, X1, Y0 + di, v, X1 - R, Y0, X1, Y0 + R);
        ellipse_fill(c, X1 - di, Y1 - di, X1, Y1, v, X1 - R, Y1 - R, X1, Y1);
        ellipse_fill(c, X0, Y1 - di, X0 + di, Y1, v, X0, Y1 - R, X0 + R, Y1);
    }
    if (full_x) {
        for (int y = Y0 + R + 1; y <= Y1 - R - 1; y++)
            hline8(c, X0, y, X1, v);
    } else if (X1 - R - 1 >= X0 + R + 1) {
        for (int y = Y0; y <= Y1; y++)
            hline8(c, X0 + R + 1, y, X1 - R - 1, v);
    }
    if (!full_x && !full_y) {
        for (int y = Y0 + R + 1; y <= Y1 - R - 1; y++) {
            hline8(c, X0, y, X0 + R, v);
            hline8(c, X1 - R, y, X1, v);
        }
    }
}

/* The coverage at the canvas's scale: Pillow's box filter on 8-bit samples (two passes, each rounded) */
static Mk cv_mask(Cv *c)
{
    int ow = c->w * c->s, oh = c->h * c->s;
    const int64_t wt = (int64_t)(0.5 + (1.0 / SS) * (1 << 22));
    uint8_t *t = tmp((size_t)ow * c->H);
    for (int y = 0; y < c->H; y++)
        for (int x = 0; x < ow; x++) {
            int64_t ss = 1 << 21;
            const uint8_t *q = c->px + (size_t)y * c->W + x * SS;
            for (int i = 0; i < SS; i++)
                ss += q[i] * wt;
            ss >>= 22;
            t[(size_t)y * ow + x] = (uint8_t)(ss < 0 ? 0 : ss > 255 ? 255 : ss);
        }
    Mk o = mk_new(ow, oh);
    for (int y = 0; y < oh; y++)
        for (int x = 0; x < ow; x++) {
            int64_t ss = 1 << 21;
            for (int i = 0; i < SS; i++)
                ss += t[(size_t)(y * SS + i) * ow + x] * wt;
            ss >>= 22;
            MV(o, x, y) = (float)(ss < 0 ? 0 : ss > 255 ? 255 : ss) / 255.0f;
        }
    return o;
}

/* ---------- the game's art ---------- */

static Im s_t0, s_black, s_white, s_dpad, s_font[2];   /* (malloc'd once, kept) */
static int s_have;                                     /* 1: all read */

#define CX 10.5
#define CY 10.5
#define R_DISC 9.7
#define PLATE_FACE (0x3F / 255.0f)
#define PLATE_INK (0xB2 / 255.0f)
#define SHEAR 0.17
#define STEM 3.3
#define CAP 10.5
static const float k_ink[3] = { PLATE_INK, PLATE_INK, PLATE_INK };

static Im face(int i)                 /* X Y B A: 0..3 */
{
    return crop(s_t0, 21 * i, 0, 21, 23);
}

static Im plate(char k)
{
    return crop(s_t0, k == 'L' ? 85 : 107, 0, 21, 21);
}

/* Button Resolution High (src/bindings.c, the Controls page): the pictures as drawn at 4x, not brought down to the
 * game's own resolution (game_res), and nothing in them an enlarged icon or an enlarged letter of the font, whose
 * few pixels show as steps and uneven lines once nothing softens them: the Xbox's are the touch controls' drawn
 * shapes (a gem, the Black and White balls, the d-pad) at the prompt's size, the body and the letter fitted to the
 * game's own picture (gem_hi: the letter pressed in, upright); the dark discs' ring the gem's; the plates their bevels drawn;
 * the Nintendo letters strokes. Set while one is made (by_id and ngb_gp_key_label, the game thread). */
static _Thread_local int s_hi;
static Im plate_box(int w, int h, int s);
static void gem_letter(Cv *cv, int k, double cx, double cy, double w, double grow);

/* ---------- discs: the face disc without its colour or letter ---------- */

static struct {
    float l_body[23 * 21], alpha[23 * 21], grey[23 * 21], rim[72];
    int made;
} D;

static float median4(float a, float b, float c, float d)
{
    float v[4] = { a, b, c, d };
    for (int i = 0; i < 4; i++)
        for (int j = i + 1; j < 4; j++)
            if (v[j] < v[i]) { float t = v[i]; v[i] = v[j]; v[j] = t; }
    return (v[1] + v[2]) / 2;
}

static int fcmp(const void *a, const void *b)
{
    float x = *(const float *)a, y = *(const float *)b;
    return x < y ? -1 : x > y ? 1 : 0;
}

static double pymod360(double a)
{
    double m = fmod(a, 360);
    if (m < 0)
        m += 360;
    if (m >= 360)
        m -= 360;
    return m;
}

static void discs_init(void)
{
    if (D.made)
        return;
    const int order[4] = { 0, 1, 2, 3 };                /* X Y B A */
    Im f[4];
    float ls[4][23 * 21], rr[23 * 21];
    for (int y = 0; y < 23; y++)
        for (int x = 0; x < 21; x++)
            rr[y * 21 + x] = (float)hypot(x + 0.5 - CX, y + 0.5 - CY);
    for (int i = 0; i < 4; i++) {
        f[i] = face(order[i]);
        float vals[23 * 21];
        int n = 0;
        for (int p = 0; p < 23 * 21; p++)
            if (rr[p] < 7.5f)
                vals[n++] = (float)lum3(&f[i].p[4 * p]);
        qsort(vals, n, sizeof(float), fcmp);
        float med = n % 2 ? vals[n / 2] : (vals[n / 2 - 1] + vals[n / 2]) / 2;
        for (int p = 0; p < 23 * 21; p++)
            ls[i][p] = (float)lum3(&f[i].p[4 * p]) / med;
    }
    Mk lmed = mk_new(21, 23), in = mk_new(21, 23), lm = mk_new(21, 23);
    for (int p = 0; p < 23 * 21; p++) {
        lmed.m[p] = median4(ls[0][p], ls[1][p], ls[2][p], ls[3][p]);
        D.alpha[p] = median4(f[0].p[4 * p + 3], f[1].p[4 * p + 3], f[2].p[4 * p + 3], f[3].p[4 * p + 3]);
        D.grey[p] = (float)((lum3(&f[0].p[4 * p]) + lum3(&f[1].p[4 * p]) + lum3(&f[2].p[4 * p]) +
                             lum3(&f[3].p[4 * p])) / 4);
        in.m[p] = rr[p] < 8.0f;
        lm.m[p] = lmed.m[p] * in.m[p];
    }
    Mk a = blur(lm, 2.0), b = blur(in, 2.0);
    for (int p = 0; p < 23 * 21; p++)
        D.l_body[p] = a.m[p] / (b.m[p] > 1e-3f ? b.m[p] : 1e-3f);
    for (int i = 0; i < 72; i++) {
        double sw = 0, sg = 0;
        for (int y = 0; y < 23; y++)
            for (int x = 0; x < 21; x++) {
                int p = y * 21 + x;
                if (!(rr[p] > 8.6f && rr[p] < 9.6f && D.alpha[p] > 0.9f && D.grey[p] < 0.6f))
                    continue;
                double ang = pymod360(atan2(y + 0.5 - CY, x + 0.5 - CX) * 180 / M_PI);
                double d = fabs(pymod360(ang - i * 5 + 180) - 180);
                double w = exp(-0.5 * (d / 16) * (d / 16));
                sw += w;
                sg += D.grey[p] * w;
            }
        D.rim[i] = (float)(sg / (sw > 1e-6 ? sw : 1e-6));
    }
    D.made = 1;
}

static float *up_plane(const float *v, int s)
{
    return resize_f(v, 21, 23, 21 * s, 23 * s);
}

/* The disc as a near-black glossy button under the game's own light, with its rim and glint, at scale s > 1 */
static Im dark_disc(const double base[3], double gain, int s)
{
    discs_init();
    Im o = im_new(21 * s, 23 * s);
    float *l = up_plane(D.l_body, s), *al = up_plane(D.alpha, s);
    for (int y = 0; y < o.h; y++)
        for (int x = 0; x < o.w; x++) {
            double px = (x + 0.5) / s, py = (y + 0.5) / s, r = hypot(px - CX, py - CY);
            double ang = pymod360(atan2(py - CY, px - CX) * 180 / M_PI);
            float lv = l[y * o.w + x];
            double t = ang / 5, fl = floor(t), f = t - fl;
            int i0 = ((int)fl) % 72;
            double rimc = D.rim[i0] * (1 - f) + D.rim[(i0 + 1) % 72] * f;
            double rim = clip01((r - 8.55) * s / fmax(s * 0.5, 1) + 0.5), seam = 0;
            if (s_hi) {
                /* the ring as the drawn gem's (jewel): a firm inner edge, rounded, a fine dark line where the
                 * disc meets it -- the flat band, soft across half its width, is made for game_res */
                rim = clip01((r - 8.75) * s + 0.5);
                rimc *= 0.8 + 0.42 * sin(M_PI * clip01((r - 8.75) / (R_DISC - 8.75)));
                seam = clip01(0.5 - (fabs(r - 8.75) - 0.1) * s) * 0.4;
            }
            double g = clip01(1.3 - hypot(px - 6.2, py - 2.2) / 1.1);
            g = pow(g, 1.5);
            double circ = clip01((R_DISC - r) * s + 0.5);
            float *p = PX(o, x, y);
            for (int c = 0; c < 3; c++) {
                double body = clip01(lv * base[c] * 1.5 * gain) + clip01((lv - 0.95) / 1.3) * 0.14;
                double v = (body * (1 - rim) + rimc * rim) * (1 - seam);
                v = v * (1 - g * 0.9) + 0.93 * g * 0.9;
                p[c] = (float)(v * circ + 0.02 * (1 - circ));
            }
            double sh = clip01(al[y * o.w + x]) * (r > R_DISC - 0.5);
            p[3] = (float)fmax(circ, fmin(sh, 0.5));
            if (s_hi && s > 4)       /* (a touch piece: the icon's shadow enlarged that far is squares; the gem's) */
                p[3] = (float)fmax(circ, clip01((R_DISC + 0.55 - hypot(px - CX - 0.45, py - CY - 0.75)) / 1.3) * 0.5);
        }
    return o;
}

/* A coloured symbol on the disc: a darker edge down-right under it, lit from above */
static Im put_symbol(Im d, Mk m, const float col[3], int s, float shade)
{
    int off = pyround(0.5 * s);
    if (off < 1)
        off = 1;
    Mk sh = mk_new(m.w, m.h);
    for (int y = off; y < m.h; y++)
        for (int x = off; x < m.w; x++)
            MV(sh, x, y) = MV(m, x - off, y - off) * (s == 1 ? 0.5f : 1.0f);
    const float dark[3] = { 0.02f, 0.02f, 0.03f };
    ink(d, sh, dark, 0, 0, shade);
    for (int y = 0; y < m.h; y++) {
        double grad = 1.12 - 0.28 * ((double)y / m.h);
        for (int x = 0; x < m.w; x++) {
            float v = MV(m, x, y), *p = PX(d, x, y);
            for (int c = 0; c < 3; c++)
                p[c] = p[c] * (1 - v) + clip01(col[c] * grad) * v;
        }
    }
    return d;
}

/* ---------- the plates ---------- */

static Im plate_sized(int w, int h, int s)
{
    if (s_hi)                        /* (the game's 21 pixels enlarged pixel by pixel: steps along every bevel) */
        return plate_box(w, h, s);
    Im p = plate('L');
    for (int y = 2; y < 19; y++)
        for (int x = 2; x < 19; x++)
            for (int c = 0; c < 3; c++)
                PX(p, x, y)[c] = PLATE_FACE;
    int xs[64], ys[64];
    for (int i = 0; i < w; i++)
        xs[i] = i < 4 ? i : i >= w - 4 ? 17 + (i - (w - 4)) : 10;
    for (int i = 0; i < h; i++)
        ys[i] = i < 4 ? i : i >= h - 4 ? 17 + (i - (h - 4)) : 10;
    Im o = im_new(w, h);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            memcpy(PX(o, x, y), PX(p, xs[x], ys[y]), sizeof(float) * 4);
    return nearest(o, s);
}

/* The plate letters' cut: ch drawn on c (NULL: only measured), its left edge x at the baseline; its width */
static double plate_glyph(Cv *c, char ch, double x, double base, double u)
{
    Pts p;
#define T(px_, py_) pt(&p, x + (px_) * u + SHEAR * (CAP - (py_)) * u, base - (CAP - (py_)) * u)
#define TA(cx_, cy_, rx_, ry_, a0_, a1_, n_)                                                                      \
    do {                                                                                                         \
        for (int i_ = 0; i_ <= (n_); i_++) {                                                                     \
            double a_ = ((a0_) + ((a1_) - (a0_)) * i_ / (double)(n_)) * M_PI / 180;                              \
            T((cx_) + (rx_) * cos(a_), (cy_) + (ry_) * sin(a_));                                                 \
        }                                                                                                        \
    } while (0)
#define RR(x0_, y0_, x1_, y1_, r_)                                                                               \
    do {                                                                                                         \
        T(x0_, y0_);                                                                                             \
        TA((x1_) - (r_), (y0_) + (r_), r_, r_, 270, 360, 8);                                                     \
        TA((x1_) - (r_), (y1_) - (r_), r_, r_, 0, 90, 8);                                                        \
        T(x0_, y1_);                                                                                             \
    } while (0)
#define BEGIN p.n = 0
#define FILL(v) do { if (c) cv_poly(c, &p, v); } while (0)
#define STROKE(w) do { if (c) cv_stroke(c, &p, (w) * u, 255, 0); } while (0)
    switch (ch) {
    case '1':
        BEGIN; T(1.9, 0); T(5.0, 0); T(5.0, CAP); T(1.9, CAP); T(1.9, 3.6); T(0.1, 4.2); T(0.1, 2.0); T(1.4, 1.5);
        FILL(255);
        return 5.2 * u;
    case '2':
        BEGIN; TA(3.55, 3.4, 2.2, 2.05, 185, 392, 20); T(1.6, CAP - 1.6);
        STROKE(2.7);
        BEGIN; T(0.0, CAP - 2.5); T(7.1, CAP - 2.5); T(7.1, CAP); T(0.0, CAP); FILL(255);
        BEGIN; T(0.0, CAP - 2.5); T(2.6, CAP - 4.6); T(3.8, CAP - 2.5); FILL(255);
        BEGIN; T(0.0, 2.6); T(2.7, 2.6); T(2.7, 4.0); T(0.0, 4.0); FILL(255);
        return 7.3 * u;
    case '3':
        BEGIN; TA(3.3, 3.05, 2.2, 1.75, 205, 450, 20); STROKE(2.6);
        BEGIN; TA(3.4, 7.3, 2.4, 1.9, 270, 515, 20); STROKE(2.6);
        return 7.3 * u;
    case 'Z':
        BEGIN; T(0.2, 0); T(7.2, 0); T(7.2, 2.4); T(0.2, 2.4); FILL(255);
        BEGIN; T(0, CAP - 2.5); T(7.2, CAP - 2.5); T(7.2, CAP); T(0, CAP); FILL(255);
        BEGIN; T(3.6, 2.3); T(7.2, 2.3); T(3.6, CAP - 2.4); T(0, CAP - 2.4); FILL(255);
        return 7.5 * u;
    case 'A':
        BEGIN; T(0, CAP); T(3.4, 0); T(7.3, 0); T(10.7, CAP); T(7.2, CAP); T(6.55, 8.3); T(4.15, 8.3); T(3.5, CAP);
        FILL(255);
        BEGIN; T(5.35, 2.7); T(4.55, 6.0); T(6.15, 6.0); FILL(0);
        return 11.0 * u;
    case 'B':
        BEGIN; RR(0, 0, 8.7, 5.7, 2.6); FILL(255);
        BEGIN; RR(0, 4.8, 9.3, CAP, 2.8); FILL(255);
        BEGIN; T(3.2, 2.1); TA(5.0, 2.95, 0.8, 0.85, 270, 450, 8); T(3.2, 3.8); FILL(0);
        BEGIN; T(3.2, 6.5); TA(5.3, 7.5, 0.95, 1.0, 270, 450, 8); T(3.2, 8.5); FILL(0);
        return 9.6 * u;
    case 'X':
        BEGIN; T(0, 0); T(3.9, 0); T(10.2, CAP); T(6.3, CAP); FILL(255);
        BEGIN; T(6.3, 0); T(10.2, 0); T(3.9, CAP); T(0, CAP); FILL(255);
        return 10.5 * u;
    case 'Y':
        BEGIN; T(0, 0); T(3.8, 0); T(5.1, 4.0); T(6.4, 0); T(10.2, 0); T(6.75, 6.4); T(6.75, CAP); T(3.45, CAP);
        T(3.45, 6.4); FILL(255);
        return 10.5 * u;
    case 'S':
        BEGIN; TA(4.4, 2.95, 2.9, 1.65, 340, 90, 20); TA(4.4, 7.55, 2.9, 1.6, 270, 520, 20); STROKE(2.7);
        return 9.0 * u;
    case 'L':
        BEGIN; T(0, 0); T(STEM, 0); T(STEM, CAP - 2.5); T(8.3, CAP - 2.5); T(8.3, CAP); T(0, CAP); FILL(255);
        return 8.6 * u;
    case 'R':
        BEGIN; T(0, 0); T(STEM, 0); T(STEM, CAP); T(0, CAP); FILL(255);
        BEGIN; RR(0, 0, 9.5, 6.3, 2.6); FILL(255);
        BEGIN; T(3.2, 2.0); TA(5.3, 3.0, 0.9, 1.0, 270, 450, 8); T(3.2, 4.0); FILL(0);
        BEGIN; T(4.05, 5.5); T(7.75, 5.5); T(10.35, CAP); T(6.55, CAP); FILL(255);
        return 10.6 * u;
    }
#undef T
#undef TA
#undef RR
#undef BEGIN
#undef FILL
#undef STROKE
    return 0;
}

/* A plate w x h with text in the plate letters' cut, centred, at scale s > 1 */
/* A legend in the plate letters' cut, centred on a plate w x h at scale s */
static void plate_text(Im p, const char *text, int w, int h, int s, double size)
{
    Cv cv = cv_new(w, h, s);
    int n = (int)strlen(text);
    double ws[8], tw = 0, gap = 0.15 * size;
    for (int i = 0; i < n; i++)
        tw += ws[i] = plate_glyph(NULL, text[i], 2, 16, size);
    tw = tw - 0.3 * size + gap * (n - 1) + SHEAR * CAP * size * 0.55;
    double x = (w - 1 - tw) / 2 + 0.1, base = (h - 0.6) / 2 + CAP * size / 2 - 0.05;
    for (int i = 0; i < n; i++) {
        plate_glyph(&cv, text[i], x, base, size);
        x += ws[i] + gap;
    }
    ink(p, cv_mask(&cv), k_ink, 0, 0, 1);
}

static Im plate_label(const char *text, int w, int h, int s, double size)
{
    Im p = plate_sized(w, h, s);
    plate_text(p, text, w, h, s, size);
    return p;
}

/* ---------- the game's small font (16 x 16 cells, white with a 1 px shadow) ---------- */

static int glyph_at(char ch, int *pg, int *row, int *col)
{
    static const char *rows32[3] = { "#&*@?0123456789A", "BCDEFGHIJKLMNOPQ", "RSTUVWXYZ" };
    static const char row0[] = { ',', 2, '.', 3, ':', 5, ';', 6, '?', 7, '!', 8, '/', 14, 0 };
    static const char row1[] = { '\\', 0, '~', 1, '(', 5, ')', 6, '+', 9, '-', 10, '=', 14, '%', 15, 0 };
    if (ch >= 'a' && ch <= 'p') { *pg = 1; *row = 5; *col = ch - 'a'; return 1; }
    if (ch >= 'q' && ch <= 'z') { *pg = 1; *row = 6; *col = ch - 'q'; return 1; }
    for (int r = 0; r < 3; r++) {
        const char *q = strchr(rows32[r], ch);
        if (q && ch != '?' && ch != 0) { *pg = 0; *row = r + 2; *col = (int)(q - rows32[r]); return 1; }
    }
    for (int i = 0; row0[i]; i += 2)
        if (row0[i] == ch) { *pg = 0; *row = 0; *col = row0[i + 1]; return 1; }
    for (int i = 0; row1[i]; i += 2)
        if (row1[i] == ch) { *pg = 0; *row = 1; *col = row1[i + 1]; return 1; }
    return 0;
}

static Mk font_cell(char ch)
{
    Mk m = mk_new(16, 16);
    int pg, r, i;
    if (!glyph_at(ch, &pg, &r, &i))
        return m;
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            float *q = PX(s_font[pg], i * 16 + x, r * 16 + y);
            MV(m, x, y) = (float)(q[3] * lum3(q));
        }
    return m;
}

static Mk font_glyph(char ch)
{
    Mk m = font_cell(ch);
    int x0 = 16, x1 = -1;
    for (int x = 0; x < 16; x++)
        for (int y = 0; y < 16; y++)
            if (MV(m, x, y) > 0.08f) {
                if (x < x0) x0 = x;
                if (x > x1) x1 = x;
            }
    if (x1 < 0)
        x0 = 0, x1 = 3;
    Mk o = mk_new(x1 - x0 + 1, 16);
    for (int y = 0; y < 16; y++)
        for (int x = x0; x <= x1; x++)
            MV(o, x - x0, y) = MV(m, x, y);
    return o;
}

/* the brackets and quotes the game's font lacks, in its cut (rows of its 16-high cell) */
static const char *const k_extra[4][2] = {
    { "[", "....|....|####|#...|#...|#...|#...|#...|#...|#...|#...|#...|#...|####" },
    { "]", "....|....|####|...#|...#|...#|...#|...#|...#|...#|...#|...#|...#|####" },
    { "'", "..|..|##|##|.#|#." },
    { "`", "...|...|#..|.#.|..#" },
};

/* A line of the game's font (and k_extra), 16 high, 1 px apart */
static Mk font_mask(const char *text)
{
    Mk parts[64];
    int n = 0, W = 0;
    for (const char *s = text; *s && n < 62; s++) {
        Mk g;
        const char *ex = NULL;
        for (int i = 0; i < 4; i++)
            if (k_extra[i][0][0] == *s)
                ex = k_extra[i][1];
        if (ex) {
            int w = (int)strcspn(ex, "|");
            g = mk_new(w, 16);
            int y = 0, x = 0;
            for (const char *q = ex; *q; q++) {
                if (*q == '|') { y++; x = 0; continue; }
                if (y < 16 && x < w)
                    MV(g, x, y) = *q == '#';
                x++;
            }
        } else if (*s == ' ') {
            g = mk_new(3, 16);
        } else {
            g = font_glyph(*s);
        }
        parts[n++] = g;
        W += g.w;
        if (s[1]) {
            parts[n++] = mk_new(1, 16);
            W += 1;
        }
    }
    Mk o = mk_new(W, 16);
    int x = 0;
    for (int i = 0; i < n; i++) {
        for (int y = 0; y < 16; y++)
            for (int xx = 0; xx < parts[i].w; xx++)
                MV(o, x + xx, y) = MV(parts[i], xx, y);
        x += parts[i].w;
    }
    return o;
}

/* ---------- PlayStation ---------- */

static Im ps_face(const char *kind, int s)
{
    static const double base[3] = { 0.115, 0.115, 0.135 };
    Im d = dark_disc(base, 1.0, s);
    Cv cv = cv_new(21, 23, s);
    Pts p;
    double x = CX, y = CY;
    float col[3];
    if (!strcmp(kind, "Cross")) {
        double q = 4.3, t = 2.0;
        p.n = 0; pt(&p, x - q, y - q); pt(&p, x + q, y + q); cv_stroke(&cv, &p, t, 255, 0);
        p.n = 0; pt(&p, x - q, y + q); pt(&p, x + q, y - q); cv_stroke(&cv, &p, t, 255, 0);
        col[0] = 0.50f; col[1] = 0.70f; col[2] = 0.95f;
    } else if (!strcmp(kind, "Circle")) {
        cv_disc(&cv, x, y, 5.9, 255);
        cv_disc(&cv, x, y, 3.9, 0);
        col[0] = 0.96f; col[1] = 0.36f; col[2] = 0.36f;
    } else if (!strcmp(kind, "Square")) {
        cv_rect(&cv, x - 5.2, y - 5.2, x + 5.2, y + 5.2, 255);
        cv_rect(&cv, x - 3.3, y - 3.3, x + 3.3, y + 3.3, 0);
        col[0] = 0.90f; col[1] = 0.56f; col[2] = 0.80f;
    } else {
        const double a[3] = { -90, 30, 150 };
        p.n = 0;
        for (int i = 0; i < 3; i++)
            pt(&p, x + 6.6 * cos(a[i] * M_PI / 180), y + 1.0 + 6.6 * sin(a[i] * M_PI / 180));   /* (7.0 at 1.2 sat high, larger than the others) */
        cv_poly(&cv, &p, 255);
        p.n = 0;
        for (int i = 0; i < 3; i++)
            pt(&p, x + 3.3 * cos(a[i] * M_PI / 180), y + 1.0 + 3.3 * sin(a[i] * M_PI / 180));
        cv_poly(&cv, &p, 0);
        col[0] = 0.30f; col[1] = 0.86f; col[2] = 0.64f;
    }
    return put_symbol(d, cv_mask(&cv), col, s, 0.45f);
}

/* ---------- Nintendo ---------- */

static void center_ink(Mk m, double cx, double cy);

static Im nx_face(char ch, int s)
{
    static const double base[3] = { 0.14, 0.14, 0.15 };
    static const float letter[3] = { 0.88f, 0.88f, 0.90f };
    Im d = dark_disc(base, 1.0, s);
    if (s_hi) {                      /* (the font's 9-pixel letter enlarged: its strokes uneven, its slopes wavy) */
        Cv cv = cv_new(21, 23, s);
        gem_letter(&cv, (int)(strchr("XYBA", ch) - "XYBA"), CX, CY, 1.9, 1);
        return put_symbol(d, cv_mask(&cv), letter, s, 0.5f);
    }
    Mk g = trim(font_cell(ch), 0.04f), m = mk_new(21, 23);
    int x = pyround(CX - g.w / 2.0), y = pyround(CY - g.h / 2.0);
    for (int yy = 0; yy < g.h; yy++)
        for (int xx = 0; xx < g.w; xx++)
            if (y + yy >= 0 && y + yy < 23 && x + xx >= 0 && x + xx < 21)
                MV(m, x + xx, y + yy) = MV(g, xx, yy);
    m = smooth_mask(m, s);
    Mk b = blur(m, 0.42 * s);
    for (int i = 0; i < m.w * m.h; i++)
        m.m[i] = clip01((b.m[i] - 0.42) * 3.2 + 0.5);
    center_ink(m, CX * s, CY * s);   /* (placed on whole pixels at 1x it sat up to half a pixel off: 2 at 4x) */
    return put_symbol(d, m, letter, s, 0.5f);
}

/* Moves what is drawn in m so its box's centre is at (cx, cy), to the nearest pixel; cy NAN: across only. */
static void center_ink(Mk m, double cx, double cy)
{
    int x0 = m.w, y0 = m.h, x1 = -1, y1 = -1;
    for (int y = 0; y < m.h; y++)
        for (int x = 0; x < m.w; x++)
            if (MV(m, x, y) > 0.5f) {
                if (x < x0) x0 = x;
                if (x > x1) x1 = x;
                if (y < y0) y0 = y;
                if (y > y1) y1 = y;
            }
    if (x1 < 0)
        return;
    int dx = (int)lround(cx - (x0 + x1 + 1) / 2.0), dy = isnan(cy) ? 0 : (int)lround(cy - (y0 + y1 + 1) / 2.0);
    if (!dx && !dy)
        return;
    float *t = malloc(sizeof(float) * m.w * m.h);
    if (!t)
        return;
    for (int y = 0; y < m.h; y++)
        for (int x = 0; x < m.w; x++) {
            int sx = x - dx, sy = y - dy;
            t[y * m.w + x] = sx >= 0 && sx < m.w && sy >= 0 && sy < m.h ? MV(m, sx, sy) : 0;
        }
    memcpy(m.m, t, sizeof(float) * m.w * m.h);
    free(t);
}

static Im ball_black_n(int s, int marked, int n);
static Im ball_white_n(int s, int n);
static Im dpad_piece_at(int lit, int n, double s);
static Im gem_at(int k, int s);

/* The Black button's ball the Start, Back, Options... pictures are drawn on (25 x 25 at s): the game's icon
 * enlarged, or with High the ball drawn, its glint up where Start's and Back's is */
static Im black_base(int s)
{
    return s_hi ? ball_black_n(s, 1, 25 * s) : smooth(crop(s_black, 0, 0, 25, 25), s);
}

/* PlayStation Options / Create and Nintendo + / -: the Xbox Start / Back ball (the game's Black button) with
 * the button's own mark. kind: 'O' Options (Start's triangle), 'C' Create (Select's bar), '+', '-'. */
static Im ball_mark(char kind, int s)
{
    Im d = black_base(s);
    Cv cv = cv_new(25, 25, s);
    double cx = 12.5, cy = 12.5, t = 1.5;
    Pts p;
    if (kind == 'O') {                 /* the PlayStation's Start: its triangle, pointing right */
        p.n = 0;
        pt(&p, cx - 2.4, cy - 3.2); pt(&p, cx + 3.2, cy + 0.0); pt(&p, cx - 2.4, cy + 3.2);
        cv_poly(&cv, &p, 255);
    } else if (kind == 'C') {          /* the classic Select: a flat rounded bar, outlined */
        cv_rrect(&cv, cx - 4.0, cy - 1.9 + 0.3, cx + 4.0, cy + 1.9 + 0.3, 1.9, 255);
        cv_rrect(&cv, cx - 2.7, cy - 0.7 + 0.3, cx + 2.7, cy + 0.7 + 0.3, 0.7, 0);
    } else {
        p.n = 0; pt(&p, cx - 3.5, cy); pt(&p, cx + 3.5, cy); cv_stroke(&cv, &p, t + 0.2, 255, 0);
        if (kind == '+') {
            p.n = 0; pt(&p, cx, cy - 3.5); pt(&p, cx, cy + 3.5); cv_stroke(&cv, &p, t + 0.2, 255, 0);
        }
    }
    ink(d, cv_mask(&cv), k_ink, 0, 0, 1);
    return d;
}

/* ---------- Xbox ---------- */

static Im ball(int start, int s)
{
    Im d = black_base(s);
    Cv cv = cv_new(25, 25, s);
    double cx = 12.5, cy = 12.5, q = 3.2, sg = start ? 1 : -1;
    Pts p = { .n = 0 };
    pt(&p, cx - sg * q * 0.8, cy - q); pt(&p, cx + sg * q, cy); pt(&p, cx - sg * q * 0.8, cy + q);
    cv_poly(&cv, &p, 255);
    ink(d, cv_mask(&cv), k_ink, 0, 0, 1);
    return d;
}

static Im stick_on(Im d, char side, int click, int s)   /* the L / R (and the click's chevron) on a disc */
{
    Cv cv = cv_new(21, 23, s);
    double size = click ? 0.74 : 0.9, w = (side == 'L' ? 8.3 : 10.3) * size, bl = click ? 13.0 : 15.4;
    plate_glyph(&cv, side, CX - w / 2 - SHEAR * CAP * size / 2, bl, size);
    Mk g = cv_mask(&cv);
    center_ink(g, CX * s, NAN);       /* (the slanted letter sat 0.6 of a pixel left of the disc's middle) */
    ink(d, g, k_ink, 0, 0, 1);
    cv = cv_new(21, 23, s);
    if (click) {
        Pts p = { .n = 0 };
        pt(&p, CX - 3.0, 14.6); pt(&p, CX, 17.0); pt(&p, CX + 3.0, 14.6);
        cv_stroke(&cv, &p, 1.5, 255, 1);
    }
    ink(d, cv_mask(&cv), k_ink, 0, 0, 1);
    return d;
}

static Im stick(char side, int click, int s)
{
    static const double base[3] = { 0.17, 0.17, 0.18 };
    return stick_on(dark_disc(base, 1.0, s), side, click, s);
}

static Im dpad_icon(int dir, int s)          /* 0 Up 1 Down 2 Left 3 Right */
{
    static const int ax[4] = { 0, 0, -1, 1 }, ay[4] = { -1, 1, 0, 0 };
    static const float light[3] = { 0.93f, 0.93f, 0.93f };
    Im d = s_hi ? dpad_piece_at(0, 25 * s, s) : smooth(crop(s_dpad, 0, 0, 23, 22), s);
    const double cx = s_hi ? 12.5 : 11.5, cy = s_hi ? 12.5 : 11.0;   /* (the drawn one square, room for its shadow) */
    Mk m = mk_new(d.w, d.h);
    for (int y = 0; y < d.h; y++)
        for (int x = 0; x < d.w; x++) {
            double px = (x + 0.5) / s - cx, py = (y + 0.5) / s - cy;
            double along = px * ax[dir] + py * ay[dir], across = fabs(px * ay[dir] - py * ax[dir]);
            MV(m, x, y) = (float)(clip01((along - 2.6) * s + 0.5) * clip01((9.6 - along) * s + 0.5) *
                                  clip01((2.9 - across) * s + 0.5));
        }
    ink(d, m, light, 0, 0, 0.85f);
    return d;
}

/* ---------- keys ---------- */

#define FACE_C 10.5

static Im key_cap(int s)
{
    Im p = im_new(21 * s, 23 * s);
    over(p, plate_sized(21, 21, s), 0, 0);
    return p;
}

/* A game-font mask enlarged smoothly by k * s, its strokes evened and its edge made firm */
static Mk fit(Mk m, int s, double k)
{
    m = trim(m, 0.04f);
    int w = pyround(m.w * k * s), h = pyround(m.h * k * s);
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    Mk o = resize_mk(m, w, h);
    for (int i = 0; i < w * h; i++)
        o.m[i] = clip01(o.m[i]);
    if (k * s >= 3) {
        Mk pd = mk_new(w + 8, h + 8);
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                MV(pd, x + 4, y + 4) = MV(o, x, y);
        Mk b = blur(pd, 0.42 * k * s);
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                MV(o, x, y) = clip01((MV(b, x + 4, y + 4) - 0.42) * 3.2 + 0.5);
        return o;
    }
    for (int i = 0; i < w * h; i++)
        o.m[i] = clip01((o.m[i] - 0.5) * 2.0 + 0.5);
    return o;
}

static Mk fit_line(Mk cell, int s, double k)
{
    int x0 = cell.w, x1 = -1;
    for (int x = 0; x < cell.w; x++)
        for (int y = 0; y < cell.h; y++)
            if (MV(cell, x, y) > 0.04f) {
                if (x < x0) x0 = x;
                if (x > x1) x1 = x;
            }
    if (x1 < 0)
        x0 = 0, x1 = cell.w - 1;
    Mk c = mk_new(x1 - x0 + 1, cell.h);
    for (int y = 0; y < c.h; y++)
        for (int x = 0; x < c.w; x++)
            MV(c, x, y) = MV(cell, x0 + x, y);
    int w = pyround(c.w * k * s), h = pyround(c.h * k * s);
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    Mk o = resize_mk(c, w, h);
    for (int i = 0; i < w * h; i++)
        o.m[i] = clip01((clip01(o.m[i]) - 0.5) * 2.0 + 0.5);
    return o;
}

static void place4(Im p, Mk m, int s, double cy)
{
    ink(p, m, k_ink, pyround(FACE_C * s - m.w / 2.0), pyround(cy * s - m.h / 2.0), 1);
}

static Im key_arrow(int dir, int s)            /* 0 Up 1 Right 2 Down 3 Left */
{
    Im p = key_cap(s);
    Cv cv = cv_new(21, 23, s);
    static const double pts[7][2] = { { 0, -8 }, { 8, 0 }, { 3, 0 }, { 3, 8 }, { -3, 8 }, { -3, 0 }, { -8, 0 } };
    double u = 0.7, a = dir * 90 * M_PI / 180;
    Pts q = { .n = 0 };
    for (int i = 0; i < 7; i++)
        pt(&q, FACE_C + u * (pts[i][0] * cos(a) - pts[i][1] * sin(a)), FACE_C + u * (pts[i][0] * sin(a) + pts[i][1] * cos(a)));
    cv_poly(&cv, &q, 255);
    ink(p, cv_mask(&cv), k_ink, 0, 0, 1);
    return p;
}

static Im key_win(int s)
{
    Im p = key_cap(s);
    Cv cv = cv_new(21, 23, s);
    static const double o[4][2] = { { 6, 6 }, { 11, 6 }, { 6, 11 }, { 11, 11 } };
    for (int i = 0; i < 4; i++)
        cv_rect(&cv, o[i][0], o[i][1], o[i][0] + 4, o[i][1] + 4, 255);
    ink(p, cv_mask(&cv), k_ink, 0, 0, 1);
    return p;
}

static Mk mouse_shape(int s, double dx, double dy, double inset)
{
    Cv cv = cv_new(21, 23, s);
    cv_rrect(&cv, 4.0 + inset + dx, 0.0 + inset + dy, 17.0 - inset + dx, 21.0 - inset + dy, 6.3 - inset, 255);
    return cv_mask(&cv);
}

static void mouse_fill(Im p, Mk m, int v)
{
    Im c = im_new(p.w, p.h);
    for (int i = 0; i < p.w * p.h; i++) {
        c.p[4 * i] = c.p[4 * i + 1] = c.p[4 * i + 2] = v / 255.0f;
        c.p[4 * i + 3] = m.m[i];
    }
    over(p, c, 0, 0);
}

static Im key_mouse(char which, int s)
{
    Im p = im_new(21 * s, 23 * s);
    Mk body = mouse_shape(s, 0, 0, 0), lit = mouse_shape(s, -1, -1, 0), dn = mouse_shape(s, 1, 1, 0),
       face_ = mouse_shape(s, 0, 0, 1.0);
    mouse_fill(p, body, 0x10);
    mouse_fill(p, mk_min(body, lit), 0x6E);
    Mk f = mk_min(lit, dn);
    for (int i = 0; i < f.w * f.h; i++)
        f.m[i] *= face_.m[i];
    mouse_fill(p, f, 0x3F);
    Cv cv = cv_new(21, 23, s);
    if (which == 'L')
        cv_rect(&cv, 4, 0, 10.0, 9.0, 255);
    else if (which == 'R')
        cv_rect(&cv, 11.0, 0, 17, 9.0, 255);
    mouse_fill(p, mk_min(cv_mask(&cv), face_), 0xE0);   /* (0xB2 was lost at the Controls page's size) */
    cv = cv_new(21, 23, s);
    cv_rect(&cv, 10.0, 1.0, 11.0, 9.0, 255);
    cv_rect(&cv, 5.0, 9.0, 16.0, 10.0, 255);
    mouse_fill(p, mk_min(cv_mask(&cv), face_), 0x1D);
    cv = cv_new(21, 23, s);
    cv_rrect(&cv, 9.0, 2.5, 12.0, 7.5, 1.5, 255);
    mouse_fill(p, cv_mask(&cv), 0x1D);
    cv = cv_new(21, 23, s);
    cv_rrect(&cv, 9.6, 3.1, 11.4, 6.9, 0.9, 255);
    mouse_fill(p, cv_mask(&cv), which == 'M' ? 0xE0 : 0x58);
    return p;
}

/* A key with a legend in the game's font at 4x (mock_keys.py's key(), its 4x branch) */
static Im key_g(const char *text, int s)
{
    Im p = key_cap(s);
    int n = (int)strlen(text);
    if (n == 1) {
        if (strchr(",.;'", text[0])) {   /* small marks: 1.7 times the font's, 0.55 of the way to their own height */
            Mk m = font_mask(text);
            int r0 = m.h, r1 = -1;
            for (int y = 0; y < m.h; y++)
                for (int x = 0; x < m.w; x++)
                    if (MV(m, x, y) > 0.04f) {
                        if (y < r0) r0 = y;
                        if (y > r1) r1 = y;
                    }
            double yc = r1 < 0 ? 8.0 : (r0 + r1 + 1) / 2.0;
            place4(p, fit(m, s, 1.7), s, FACE_C + (yc - 8.0) * 0.55);
            return p;
        }
        if (strchr("~-", text[0])) {
            Mk m = font_mask(text), big = resize_mk(m, m.w * s, 16 * s);
            for (int i = 0; i < big.w * big.h; i++)
                big.m[i] = clip01((clip01(big.m[i]) - 0.5) * 2.0 + 0.5);
            ink(p, big, k_ink, pyround(FACE_C * s - big.w / 2.0), pyround((FACE_C - 8.5) * s), 1);
            return p;
        }
        Mk m = font_mask(text);
        double k = fmin(1.0, 14.0 / trim(m, 0.04f).w);
        place4(p, fit(m, s, k), s, FACE_C);
        return p;
    }
    if (!strcmp(text, "Space")) {         /* the word was unreadable at the Controls page's size: the bar's mark */
        Cv cv = cv_new(21, 23, s);
        Pts q = { .n = 0 };
        pt(&q, FACE_C - 5.0, 8.6); pt(&q, FACE_C - 5.0, 12.0); pt(&q, FACE_C + 5.0, 12.0); pt(&q, FACE_C + 5.0, 8.6);
        cv_stroke(&cv, &q, 1.5, 255, 1);
        ink(p, cv_mask(&cv), k_ink, 0, 0, 1);
        return p;
    }
    if (!strcmp(text, "Enter")) {         /* as Space: the key's arrow, down and to the left */
        Cv cv = cv_new(21, 23, s);
        Pts q = { .n = 0 };
        pt(&q, FACE_C + 4.6, 6.6); pt(&q, FACE_C + 4.6, 11.2); pt(&q, FACE_C - 2.6, 11.2);
        cv_stroke(&cv, &q, 1.5, 255, 1);
        q.n = 0;
        pt(&q, FACE_C - 5.6, 11.2); pt(&q, FACE_C - 2.2, 8.6); pt(&q, FACE_C - 2.2, 13.8);
        cv_poly(&cv, &q, 255);
        ink(p, cv_mask(&cv), k_ink, 0, 0, 1);
        return p;
    }
    Mk cell = mk_rows(font_mask(text), 2, 16);
    int fkey = text[0] == 'F' && text[1] >= '0' && text[1] <= '9';
    double k = fmin(14.2 / trim(cell, 0.04f).w, fkey ? 0.7 : 0.42);   /* (15.5 touched the bevel: Space, Enter) */
    place4(p, fit_line(cell, s, k), s, FACE_C + 0.3);
    return p;
}

static Im key_g2(const char *a, const char *b, int s)
{
    Im p = key_cap(s);
    place4(p, fit_line(mk_rows(font_mask(a), 2, 16), s, 0.42), s, FACE_C - 3.0);
    place4(p, fit_line(mk_rows(font_mask(b), 2, 16), s, 0.42), s, FACE_C + 3.6);
    return p;
}

/* ---------- the pause menu's tab plates: Q and E ---------- */

static Im tab_key(int e, int s)
{
    static const double poly[5][2] = { { 9.0, 5.0 }, { 28.0, 5.0 }, { 28.0, 28.0 }, { 9.0, 28.0 }, { 4.0, 16.5 } };
    Cv cv = cv_new(32, 32, s);
    Pts q = { .n = 0 };
    for (int i = 0; i < 5; i++)
        pt(&q, e ? 32 - poly[i][0] : poly[i][0], poly[i][1]);
    cv_poly(&cv, &q, 255);
    Mk m = cv_mask(&cv), g = blur(m, 1.8 * s);
    Im p = im_new(32 * s, 32 * s);
    for (int i = 0; i < p.w * p.h; i++) {
        p.p[4 * i] = p.p[4 * i + 1] = p.p[4 * i + 2] = 1 - m.m[i];
        p.p[4 * i + 3] = fmaxf(m.m[i], clip01(1.6 * g.m[i]));
    }
    cv = cv_new(32, 32, s);
    if (e) {
        cv_rect(&cv, 7.53, 7.73, 12.27, 25.0, 255);
        cv_rect(&cv, 7.53, 7.73, 20.8, 11.35, 255);
        cv_rect(&cv, 7.53, 14.55, 19.9, 18.1, 255);
        cv_rect(&cv, 7.53, 21.4, 21.1, 25.0, 255);
    } else {
        cv_ellipse(&cv, 18.2, 16.4, 7.5, 8.9, 255);
        cv_ellipse(&cv, 18.2, 16.4, 2.95, 5.3, 0);
        q.n = 0;
        pt(&q, 18.6, 19.6); pt(&q, 21.2, 17.6); pt(&q, 26.6, 24.4); pt(&q, 24.0, 26.4);
        cv_poly(&cv, &q, 255);
    }
    static const float white[3] = { 1, 1, 1 };
    ink(p, cv_mask(&cv), white, 0, 0, 1);
    return p;
}

/* ---------- the touch pieces ---------- */

#define TN 384               /* a touch piece's picture (a face button is some 200 pixels across on a 1080p phone,
                              * 270 on a 1440p one: 192 was drawn enlarged, #284) */
#define TS 16                /* touch: 16 pixels a game pixel */
#define BALL_S 22
#define SMALL_S 22

static Im centred(Im a, int n, double ax, double ay)
{
    Im o = im_new(n, n);
    over(o, a, pyround(n / 2.0 - ax), pyround(n / 2.0 - ay));
    return o;
}

static Im held(Im a, int s)
{
    const double grow = 1.3, gain = 1.14, lift = 0.03, strength = 0.8, thr = 0.6;
    Mk solid = mk_new(a.w, a.h);
    for (int i = 0; i < a.w * a.h; i++)
        solid.m[i] = a.p[4 * i + 3] > thr;
    Mk g = blur(solid, grow * s * 0.55);
    Im o = im_new(a.w, a.h), top = im_copy(a);
    for (int i = 0; i < a.w * a.h; i++) {
        double gv = pow(clip01(g.m[i] * 1.9), 1.5) * strength;
        /* (faded out toward the picture's edge: a plate nearly as wide as its box, the L and R triggers', had
         * its glow cut off square there) */
        int x = i % a.w, y = i / a.w;
        double edge = fmin(fmin(x + 0.5, a.w - x - 0.5), fmin(y + 0.5, a.h - y - 0.5));
        o.p[4 * i] = o.p[4 * i + 1] = o.p[4 * i + 2] = 1;
        o.p[4 * i + 3] = (float)(gv * (1 - solid.m[i]) * clip01(edge / (1.5 * s)));
        for (int c = 0; c < 3; c++)
            top.p[4 * i + c] = clip01(top.p[4 * i + c] * gain + lift);
        if (!(solid.m[i] > 0))
            top.p[4 * i + 3] = fminf(top.p[4 * i + 3], 0.25f);
    }
    over(o, top, 0, 0);
    return o;
}

static void pt_grown(Pts *p, double x, double y, double cx, double cy, double grow)   /* about (cx, cy) */
{
    if (grow != 1)
        x = cx + (x - cx) * grow, y = cy + (y - cy) * grow;
    pt(p, x, y);
}

/* A face button's letter (X Y B A: 0..3) as strokes in the face's 21 x 23 cell, about the disc's centre: the game's
 * own letter is 7 pixels tall, and enlarged to a phone's button it was a blur (#284) */
static void gem_letter(Cv *cv, int k, double cx, double cy, double w, double grow)
{
    const double h = 4.2;                  /* half the letter's height */
    Pts p;
#define pt(p_, x_, y_) pt_grown(p_, x_, y_, cx, cy, grow)
    switch (k) {
    case 0:                                /* X */
        p.n = 0; pt(&p, cx - 3.1, cy - h); pt(&p, cx + 3.1, cy + h); cv_stroke(cv, &p, w, 255, 1);
        p.n = 0; pt(&p, cx + 3.1, cy - h); pt(&p, cx - 3.1, cy + h); cv_stroke(cv, &p, w, 255, 1);
        break;
    case 1:                                /* Y */
        p.n = 0; pt(&p, cx - 3.3, cy - h); pt(&p, cx, cy + 0.3); pt(&p, cx + 3.3, cy - h); cv_stroke(cv, &p, w, 255, 1);
        p.n = 0; pt(&p, cx, cy + 0.3); pt(&p, cx, cy + h); cv_stroke(cv, &p, w, 255, 1);
        break;
    case 2: {                              /* B: the stem, two bowls (the lower one wider) */
        p.n = 0;
        pt(&p, cx + 0.4, cy); pt(&p, cx - 2.9, cy); pt(&p, cx - 2.9, cy - h); pt(&p, cx + 0.4, cy - h);
        for (int i = 1; i < 12; i++) {
            double t = -M_PI / 2 + M_PI * i / 12;
            pt(&p, cx + 0.4 + 2.1 * cos(t), cy - h / 2 + 2.1 * sin(t));
        }
        pt(&p, cx + 0.4, cy); pt(&p, cx + 0.8, cy);
        for (int i = 1; i < 12; i++) {
            double t = -M_PI / 2 + M_PI * i / 12;
            pt(&p, cx + 0.8 + 2.1 * cos(t), cy + h / 2 + 2.1 * sin(t));
        }
        pt(&p, cx + 0.8, cy + h); pt(&p, cx - 2.9, cy + h); pt(&p, cx - 2.9, cy);
        cv_stroke(cv, &p, w, 255, 1);
        break;
    }
    default:                               /* A */
        p.n = 0; pt(&p, cx - 3.5, cy + h); pt(&p, cx, cy - h); pt(&p, cx + 3.5, cy + h); cv_stroke(cv, &p, w, 255, 1);
        p.n = 0; pt(&p, cx - 1.9, cy + 1.2); pt(&p, cx + 1.9, cy + 1.2); cv_stroke(cv, &p, w, 255, 0);
        break;
    }
#undef pt
}

static float aa(double sdf, double scale) { return clip01(0.5 - sdf * scale); }

static void put_px(float *p, double r, double g, double b, double a)
{
    float s[4] = { (float)r, (float)g, (float)b, clip01(a) };
    float sa = s[3], da = p[3], oa = sa + da * (1 - sa);
    for (int c = 0; c < 3; c++)
        p[c] = oa > 0 ? (s[c] * sa + p[c] * da * (1 - sa)) / (oa > 1e-6f ? oa : 1e-6f) : 0;
    p[3] = oa;
}

static double interp(double v, const double (*pts)[2], int n)
{
    if (v <= pts[0][0])
        return pts[0][1];
    for (int i = 1; i < n; i++)
        if (v <= pts[i][0])
            return pts[i - 1][1] + (pts[i][1] - pts[i - 1][1]) * (v - pts[i - 1][0]) / (pts[i][0] - pts[i - 1][0]);
    return pts[n - 1][1];
}

static double gridv(int i, int n, double s) { return (i + 0.5) / s - n / s / 2; }

/* ---- The buttons drawn at the touch pieces' own size (#284: the game's 21-pixel icons enlarged were a blur on a
 * phone). Nothing here is an enlarged icon: each is the icon's shapes, measured, drawn again as coverage -- a
 * gem's ring, the dark under the ring's top, its glint, its shadow; a ball's rim light and reflections; a plate's
 * bevels -- with the colours and the light read from the game's textures: a face's colour its body's median, the
 * light across it the four faces' (D.l_body, smooth), the ring's greys theirs (D.rim), the Black button's rim and
 * glow and the White button's light from their own. ---- */

static double angle_off(double a, double b)       /* degrees apart, 0..180 */
{
    return fabs(pymod360(a - b + 180) - 180);
}

/* A face's colour: each channel's upper-middle value over its body (two thirds up: the letter, its shadow and the
 * dark under the ring are the lower ones) */
static void face_tone(int k, double out[3])
{
    Im f = face(k);
    for (int c = 0; c < 3; c++) {
        float v[23 * 21];
        int n = 0;
        for (int y = 0; y < 23; y++)
            for (int x = 0; x < 21; x++)
                if (hypot(x + 0.5 - CX, y + 0.5 - CY) < 7.5)
                    v[n++] = PX(f, x, y)[c];
        qsort(v, n, sizeof(float), fcmp);
        out[c] = v[n * 2 / 3];
    }
}

/* The game's face button without its letter, in the face's 21 x 23 cell at scale s: a glass gem of this colour in
 * a thin metal ring, lit from the left as the game's are */
static Im jewel(const double base[3], int s)
{
    discs_init();
    const double RI = 8.75;                /* the ring's inner edge */
    Im o = im_new(21 * s, 23 * s);
    float *l = up_plane(D.l_body, s);
    for (int y = 0; y < o.h; y++)
        for (int x = 0; x < o.w; x++) {
            double px = (x + 0.5) / s, py = (y + 0.5) / s, dx = px - CX, dy = py - CY, r = hypot(dx, dy);
            double ang = pymod360(atan2(dy, dx) * 180 / M_PI);
            double lv = l[y * o.w + x];
            double t = ang / 5, fl = floor(t), f = t - fl;
            int i0 = ((int)fl) % 72;
            double metal = D.rim[i0] * (1 - f) + D.rim[(i0 + 1) % 72] * f;
            double circ = clip01((R_DISC - r) * s + 0.5), ring = clip01((r - RI) * s + 0.5);
            /* the body: dark under the ring's top (thickest up and to the right, away from the light), deeper
             * toward its edge, a soft sheen along the lit side */
            double under = clip01((hypot(dx + 0.4, dy - 1.7) - 8.3) / 0.45) * 0.5;
            double deep = pow(clip01((r - 6.4) / (RI - 6.4)), 2) * 0.2;
            double sheen = exp(-pow((r - 6.6) / 0.9, 2)) * pow(clip01(1 - angle_off(ang, 200) / 62), 1.5) * 0.2;
            /* the ring: rounded, a fine dark line where the gem meets it */
            metal *= 0.8 + 0.42 * sin(M_PI * clip01((r - RI) / (R_DISC - RI)));
            double seam = aa(fabs(r - RI) - 0.1, s) * 0.4;
            /* the glint, on the ring's inner edge at the upper left: a sharp core, a little bloom */
            double gu = (px - 6.3) * -0.447 + (py - 2.6) * -0.894, gt = (px - 6.3) * 0.894 + (py - 2.6) * -0.447;
            double e = hypot(gt / 1.25, gu / 0.85);
            double glint = fmax(clip01((0.7 - e) * 0.85 * s / 2 + 0.5), 0.55 * exp(-e * e / 0.9));
            float *p = PX(o, x, y);
            for (int c = 0; c < 3; c++) {
                double body = base[c] * lv + clip01(lv - 1.0) * 0.1;
                body *= (1 - under) * (1 - deep);
                body += (1 - body) * sheen;
                double v = body * (1 - ring) + metal * ring;
                v *= 1 - seam;
                v += (0.97 - v) * glint;
                p[c] = (float)(clip01(v) * circ + 0.02 * (1 - circ));
            }
            double shadow = clip01((R_DISC + 0.55 - hypot(dx - 0.45, dy - 0.75)) / 1.3) * 0.5;
            p[3] = (float)fmax(circ, shadow);
        }
    return o;
}

/* ---- A face button for Button Resolution High: beside the game's own buttons, so made to be the game's picture
 * when brought down to its 21 x 23 (the HI_ values: fitted to the least difference from the four, pixel by pixel,
 * the letter kept upright). The body's colours are the face's own, smoothed until its letter is gone; the ring the
 * faces' greys toward the face's colour; the letter pressed in. ---- */
#define HI_BLUR 2.0          /* the body's colours smoothed this far: the letter gone */
#define HI_FINE 0.8          /* the light's finer shapes smoothed this far (0.5 fits closer; blotches at 4x) */
#define HI_UNDER 0.09        /* jewel()'s dark under the ring's top, its deepening edge and its sheen: little, the */
#define HI_DEEP 0.12         /* faces' own light has them */
#define HI_SHEEN 0.12
#define HI_RING_TINT 0.45    /* the ring toward the face's colour */
#define HI_RING_GAIN 1.025
#define HI_STROKE 2.925      /* the letter's stroke, game pixels */
#define HI_GROW 1.008
#define HI_STROKE_A 2.5
#define HI_GROW_A 1.18
#define HI_WALL_X 0.6        /* the groove's walls: this far in from the stroke's upper-left and lower-right */
#define HI_WALL_Y 0.5
#define HI_DARK 0.42         /* the upper-left wall's shade */
#define HI_LIT 0.28          /* the lower-right wall's light (0.21 fitted; the A's was lost at the game's size) */
#define HI_FLOOR 0.03
#define HI_COUNTER 0.375     /* what the strokes close in, darker */
static const double k_hi_at[4][2] = { { -0.3, 0.0 }, { -0.4, 0.35 }, { -0.45, 0.0 }, { -0.6, -0.2 } };   /* X Y B A */

static float mk_at(Mk m, int x, int y)
{
    return x >= 0 && y >= 0 && x < m.w && y < m.h ? MV(m, x, y) : 0;
}

/* What a letter's strokes close in (the A's, the B's two): 1 inside them */
static Mk counters(Mk m)
{
    Mk o = mk_new(m.w, m.h);
    int *stack = tmp(sizeof(int) * m.w * m.h), n = 0;
    uint8_t *seen = tmp((size_t)m.w * m.h);
    stack[n++] = 0;
    seen[0] = 1;
    while (n) {
        int q = stack[--n], x = q % m.w, y = q / m.w;
        static const int dx[4] = { 1, -1, 0, 0 }, dy[4] = { 0, 0, 1, -1 };
        for (int d = 0; d < 4; d++) {
            int X = x + dx[d], Y = y + dy[d];
            if (X < 0 || Y < 0 || X >= m.w || Y >= m.h || seen[Y * m.w + X] || MV(m, X, Y) >= 0.5f)
                continue;
            seen[Y * m.w + X] = 1;
            stack[n++] = Y * m.w + X;
        }
    }
    for (int i = 0; i < m.w * m.h; i++)
        o.m[i] = seen[i] ? 0 : 1 - m.m[i];
    return o;
}

static Im gem_hi(int k, int s)
{
    discs_init();
    const double RI = 8.75;
    Im f = face(k), o = im_new(21 * s, 23 * s);
    /* the face's colours without its letter: smoothed over the body only (the ring and the dark under its top kept
     * out of it) */
    float *body[3];
    Mk in = mk_new(21, 23), ch = mk_new(21, 23), fine = mk_new(21, 23);
    for (int y = 0; y < 23; y++)
        for (int x = 0; x < 21; x++)
            MV(in, x, y) = hypot(x + 0.5 - CX, y + 0.5 - CY) < 8.0;
    Mk bin = blur(in, HI_BLUR);
    /* the light's finer shapes (the dark under the ring's top, the lit side's crescent): what the four faces share,
     * pixel by pixel, which their letters are not -- each pixel's median of the four, against its own smoothing */
    {
        Im g[4];
        float med[4];
        for (int i = 0; i < 4; i++) {
            g[i] = face(i);
            float v[23 * 21];
            int n = 0;
            for (int q = 0; q < 21 * 23; q++)
                if (in.m[q] > 0)
                    v[n++] = (float)lum3(&g[i].p[4 * q]);
            qsort(v, n, sizeof(float), fcmp);
            med[i] = v[n / 2];
        }
        for (int q = 0; q < 21 * 23; q++)
            ch.m[q] = median4((float)lum3(&g[0].p[4 * q]) / med[0], (float)lum3(&g[1].p[4 * q]) / med[1],
                              (float)lum3(&g[2].p[4 * q]) / med[2], (float)lum3(&g[3].p[4 * q]) / med[3]) * in.m[q];
        Mk sm = blur(ch, HI_BLUR), fb = blur(ch, HI_FINE), fn = blur(in, HI_FINE);
        for (int q = 0; q < 21 * 23; q++) {
            double lo = sm.m[q] / (bin.m[q] > 1e-3f ? bin.m[q] : 1e-3f), hi = fb.m[q] / (fn.m[q] > 1e-3f ? fn.m[q] : 1e-3f);
            fine.m[q] = in.m[q] > 0 && lo > 1e-3 ? (float)clampd(hi / lo, 0.3, 2.0) : 1;
        }
    }
    for (int c = 0; c < 3; c++) {
        for (int i = 0; i < 21 * 23; i++)
            ch.m[i] = f.p[4 * i + c] * in.m[i];
        Mk b = blur(ch, HI_BLUR);
        for (int i = 0; i < 21 * 23; i++)
            b.m[i] = b.m[i] / (bin.m[i] > 1e-3f ? bin.m[i] : 1e-3f) * fine.m[i];
        body[c] = up_plane(b.m, s);
    }
    double tone[3];
    face_tone(k, tone);
    const double tl = 0.299 * tone[0] + 0.587 * tone[1] + 0.114 * tone[2] + 1e-6;
    for (int y = 0; y < o.h; y++)
        for (int x = 0; x < o.w; x++) {
            double px = (x + 0.5) / s, py = (y + 0.5) / s, dx = px - CX, dy = py - CY, r = hypot(dx, dy);
            double ang = pymod360(atan2(dy, dx) * 180 / M_PI);
            double t = ang / 5, fl = floor(t), fr = t - fl;
            int i0 = ((int)fl) % 72;
            double metal = D.rim[i0] * (1 - fr) + D.rim[(i0 + 1) % 72] * fr;
            double circ = clip01((R_DISC - r) * s + 0.5), ring = clip01((r - RI) * s + 0.5);
            double under = clip01((hypot(dx + 0.4, dy - 1.7) - 8.3) / 0.45) * HI_UNDER;
            double deep = pow(clip01((r - 6.4) / (RI - 6.4)), 2) * HI_DEEP;
            double sheen = exp(-pow((r - 6.6) / 0.9, 2)) * pow(clip01(1 - angle_off(ang, 200) / 62), 1.5) * HI_SHEEN;
            metal *= (0.8 + 0.42 * sin(M_PI * clip01((r - RI) / (R_DISC - RI)))) * HI_RING_GAIN;
            double seam = aa(fabs(r - RI) - 0.1, s) * 0.4;
            double gu = (px - 6.3) * -0.447 + (py - 2.6) * -0.894, gt = (px - 6.3) * 0.894 + (py - 2.6) * -0.447;
            double e = hypot(gt / 1.25, gu / 0.85);
            double glint = fmax(clip01((0.7 - e) * 0.85 * s / 2 + 0.5), 0.55 * exp(-e * e / 0.9));
            float *p = PX(o, x, y);
            for (int c = 0; c < 3; c++) {
                double b = clip01(body[c][y * o.w + x]);
                b *= (1 - under) * (1 - deep);
                b += (1 - b) * sheen;
                double v = b * (1 - ring) + metal * (1 + HI_RING_TINT * (tone[c] / tl - 1)) * ring;
                v *= 1 - seam;
                v += (0.97 - v) * glint;
                p[c] = (float)(clip01(v) * circ + 0.02 * (1 - circ));
            }
            double shadow = clip01((R_DISC + 0.55 - hypot(dx - 0.45, dy - 0.75)) / 1.3) * 0.5;
            p[3] = (float)fmax(circ, shadow);
        }
    /* the letter: a wide stroke pressed in -- its upper-left wall in shadow, its lower-right one lit, its floor the
     * body's colour, what it closes in darker */
    Cv cv = cv_new(21, 23, s);
    /* (the A a little larger and its stroke finer: at the others' its bar filled the space above it, an inverted V) */
    gem_letter(&cv, k, CX + k_hi_at[k][0], CY + k_hi_at[k][1], k == 3 ? HI_STROKE_A : HI_STROKE, k == 3 ? HI_GROW_A : HI_GROW);
    Mk m = cv_mask(&cv), hole = counters(m);
    int ox = pyround(HI_WALL_X * s), oy = pyround(HI_WALL_Y * s);
    /* At a touch piece's size the walls are the stroke's edge shaded by the way it faces: cut out by the stroke moved
     * across, as at a prompt's, each wall showed the shape of what lay across from it (a counter, twice). */
    Mk soft = s > 4 ? blur(m, 0.3 * hypot(HI_WALL_X, HI_WALL_Y) * s) : m;
    const double lx = HI_WALL_X / hypot(HI_WALL_X, HI_WALL_Y), ly = HI_WALL_Y / hypot(HI_WALL_X, HI_WALL_Y);
    for (int y = 0; y < m.h; y++)
        for (int x = 0; x < m.w; x++) {
            float v = MV(m, x, y), hl = MV(hole, x, y);
            if (!(v > 0) && !(hl > 0))
                continue;
            double dark, lit;
            if (s > 4) {
                double gx = mk_at(soft, x + 1, y) - mk_at(soft, x - 1, y), gy = mk_at(soft, x, y + 1) - mk_at(soft, x, y - 1);
                double g = hypot(gx, gy), face = g > 1e-6 ? (gx * lx + gy * ly) / g : 0;
                double wall = v * clip01((1 - MV(soft, x, y)) * 3.0);
                /* (stronger than a prompt's: over the game's picture, half see-through, the letter has to read) */
                dark = wall * clip01(face * 1.4) * 1.35, lit = wall * clip01(-face * 1.4) * 1.5;
            } else {
                dark = v * (1 - mk_at(m, x - ox, y - oy)), lit = v * (1 - mk_at(m, x + ox, y + oy));
            }
            float *p = PX(o, x, y);
            for (int c = 0; c < 3; c++)
                p[c] = clip01(p[c] * (1 - HI_COUNTER * hl) * (1 + HI_LIT * lit) * (1 + (HI_FLOOR - HI_DARK * dark) * v));
        }
    return o;
}

/* A face button (X Y B A: 0..3) drawn in its 21 x 23 cell at scale s */
static Im gem_at(int k, int s)
{
    if (s_hi)   /* (Button Resolution High: its own body and engraved letter) */
        return gem_hi(k, s);
    double tone[3];
    face_tone(k, tone);
    Im a = jewel(tone, s);
    /* the letter: a soft dark edge under it, as the game's letters have, then the letter in the game's near-white */
    Cv cv = cv_new(21, 23, s);
    gem_letter(&cv, k, CX, CY, 2.0, 1);
    Mk m = cv_mask(&cv);
    Cv cvs = cv_new(21, 23, s);
    gem_letter(&cvs, k, CX + 0.3, CY + 0.4, 2.7, 1);
    Mk sh = blur(cv_mask(&cvs), 0.3 * s);
    static const float dark[3] = { 0.05f, 0.05f, 0.06f }, light[3] = { 0.96f, 0.96f, 0.95f };
    ink(a, sh, dark, 0, 0, 0.55f);
    ink(a, m, light, 0, 0, 1);
    return a;
}

static Im gem(int k)
{
    return centred(gem_at(k, TS), TN, CX * TS, CY * TS);
}

/* What the Black and White buttons' icons give: the Black's rim light round its edge, its glow by distance, its
 * glint's white; the White's light across it, smooth */
static struct {
    float rim[72], glow[14], spec, light[13 * 13];
    int made;
} K;

#define BALL_R 6.4

static void balls_init(void)
{
    if (K.made)
        return;
    double gw[14] = { 0 }, gs[14] = { 0 };
    K.spec = 0;
    for (int y = 0; y < 25; y++)
        for (int x = 0; x < 25; x++) {
            double r = hypot(x + 0.5 - 12.5, y + 0.5 - 12.5);
            const float *p = PX(s_black, x, y);
            if (r < 5 && lum3(p) > K.spec)
                K.spec = (float)lum3(p);
            for (int i = 0; i < 14; i++) {            /* the glow at 7.5, 8, 8.5... from the middle */
                double w = 1 - fabs(r - (7.5 + 0.5 * i)) / 0.5;
                if (w > 0 && r > BALL_R + 0.75) {
                    gw[i] += w;
                    gs[i] += p[3] * w;
                }
            }
        }
    for (int i = 0; i < 14; i++)
        K.glow[i] = (float)(gw[i] > 0 ? gs[i] / gw[i] : 0);
    for (int i = 0; i < 72; i++) {
        double sw = 0, sg = 0;
        for (int y = 0; y < 25; y++)
            for (int x = 0; x < 25; x++) {
                double r = hypot(x + 0.5 - 12.5, y + 0.5 - 12.5);
                if (!(r > 5.2 && r < 6.2))
                    continue;
                double d = angle_off(pymod360(atan2(y + 0.5 - 12.5, x + 0.5 - 12.5) * 180 / M_PI), i * 5);
                double w = exp(-0.5 * (d / 20) * (d / 20));
                sw += w;
                sg += lum3(PX(s_black, x, y)) * w;
            }
        K.rim[i] = (float)(sg / (sw > 1e-6 ? sw : 1e-6));
    }
    Mk lm = mk_new(13, 13), in = mk_new(13, 13);
    for (int i = 0; i < 13 * 13; i++) {
        in.m[i] = s_white.p[4 * ((i / 13) * s_white.w + i % 13) + 3] > 0.98f;
        lm.m[i] = (float)lum3(&s_white.p[4 * ((i / 13) * s_white.w + i % 13)]) * in.m[i];
    }
    Mk a = blur(lm, 1.5), b = blur(in, 1.5);
    float top = 0;
    for (int i = 0; i < 13 * 13; i++) {
        K.light[i] = a.m[i] / (b.m[i] > 1e-3f ? b.m[i] : 1e-3f);
        if (in.m[i] > 0 && K.light[i] > top)
            top = K.light[i];
    }
    for (int i = 0; i < 13 * 13; i++)      /* (smoothed, its white was a grey: the brightest white again) */
        K.light[i] = clip01(K.light[i] / (top > 0.1f ? top : 1));
    K.made = 1;
}

static double ball_glow(double r)
{
    double t = (r - 7.5) / 0.5;
    if (t <= 0)                                /* (under the ball's own edge in the icon: carried on inward) */
        return fmin(K.glow[0] + (K.glow[0] - K.glow[1]) * -t, K.glow[0] * 1.6);
    if (t >= 13)
        return 0;
    int i = (int)t;
    return K.glow[i] + (K.glow[i + 1] - K.glow[i]) * (t - i);
}

/* A spot of light: 1 inside the ellipse (rx, ry) about (cx, cy), its edge `soft` pixels of the piece wide */
static double spot(double x, double y, double cx, double cy, double rx, double ry, double soft, int s)
{
    double e = hypot((x - cx) / rx, (y - cy) / ry);
    return clip01((1 - e) * fmin(rx, ry) * s / soft + 0.5);
}

/* The Black button (Start and Back's ball too) on TN x TN at scale s: a black glossy ball in the game's white
 * glow, its rim light, its glint and the dimmer reflections where the icon has them */
static Im ball_black_n(int s, int marked, int n)   /* marked: Start's and Back's, the glint above the triangle; n
                                                    * across, the ball in its middle */
{
    const double gx = marked ? 0.4 : 1.0, gy = marked ? -4.3 : -2.75, gk = marked ? 0.7 : 1;
    balls_init();
    Im o = im_new(n, n);
    for (int Y = 0; Y < n; Y++)
        for (int X = 0; X < n; X++) {
            double x = (X + 0.5 - n / 2.0) / s, y = (Y + 0.5 - n / 2.0) / s, r = hypot(x, y);
            double ang = pymod360(atan2(y, x) * 180 / M_PI), t = ang / 5, fl = floor(t), f = t - fl;
            int i0 = ((int)fl) % 72;
            double circ = clip01((BALL_R - r) * s + 0.5);
            double v = 0.02;
            v += (K.rim[i0] * (1 - f) + K.rim[(i0 + 1) % 72] * f) * 1.25 * pow(clip01((r - 4.9) / 1.5), 2.2);
            v += 0.3 * exp(-pow((r - 4.7) / 0.42, 2)) * pow(clip01(1 - angle_off(ang, 212) / 34), 1.3);
            v += 0.26 * exp(-1.3 * (pow((x + 2.6) / 2.3, 2) + pow((y - 3.9) / 1.2, 2)));
            if (!marked)
                v += 0.4 * exp(-(pow(x + 1.9, 2) + pow(y + 0.9, 2)) / 0.3);
            double e = hypot((x - gx) / (1.4 * gk), (y - gy) / (1.05 * gk));
            v += 0.22 * exp(-e * e / 1.6);
            v += (K.spec - v) * spot(x, y, gx, gy, 1.4 * gk, 1.05 * gk, 2, s);
            double edge = clip01((n / 2.0 / s - r) / 2.0);
            float *p = PX(o, X, Y);
            for (int c = 0; c < 3; c++)
                p[c] = (float)(clip01(v) * circ + 1.0 * (1 - circ));
            p[3] = (float)fmax(circ, ball_glow(r) * (1 - circ) * edge);
        }
    return o;
}

static Im ball_black(int s, int marked)
{
    return ball_black_n(s, marked, TN);
}

/* The White button: the Black's ball in white, the light across it the icon's own, in the same glow (without it,
 * it looked the smaller one) */
static Im ball_white_n(int s, int n)
{
    balls_init();
    Im o = im_new(n, n);
    float *l = resize_f(K.light, 13, 13, 13 * s, 13 * s);
    for (int Y = 0; Y < n; Y++)
        for (int X = 0; X < n; X++) {
            double x = (X + 0.5 - n / 2.0) / s, y = (Y + 0.5 - n / 2.0) / s, r = hypot(x, y);
            double circ = clip01((BALL_R - r) * s + 0.5);
            /* (the icon's ball is 6.35 across 13 pixels: its light read at that size) */
            int lx = (int)((x * 6.35 / BALL_R + 6.5) * s), ly = (int)((y * 6.35 / BALL_R + 6.5) * s);
            lx = lx < 0 ? 0 : lx >= 13 * s ? 13 * s - 1 : lx;
            ly = ly < 0 ? 0 : ly >= 13 * s ? 13 * s - 1 : ly;
            double v = 1 - (1 - l[ly * 13 * s + lx]) * 0.8;   /* (a little whiter: half see-through, it was a grey) */
            v *= 1 - 0.2 * pow(clip01((r - 5.2) / 1.2), 2);
            v += (0.3 - v) * 0.5 * aa(fabs(r - BALL_R + 0.1) - 0.1, s);
            v += (1 - v) * 0.55 * exp(-(pow(x + 3.5, 2) + pow(y + 2.9, 2)) / 0.9);
            v += (1 - v) * spot(x, y, 1.0, -3.6, 1.3, 0.9, 2, s);
            double edge = clip01((n / 2.0 / s - r) / 2.0);
            float *p = PX(o, X, Y);
            for (int c = 0; c < 3; c++)
                p[c] = (float)(clip01(v) * circ + 1.0 * (1 - circ));
            p[3] = (float)fmax(circ, ball_glow(r) * (1 - circ) * edge);
        }
    return o;
}

static Im ball_white(int s)
{
    return ball_white_n(s, TN);
}

static Im start_back(int start)
{
    Im a = ball_black(SMALL_S, 1);
    Cv cv = cv_new(TN / 2, TN / 2, 2);         /* (a unit two of the piece's pixels) */
    double q = 3.0 * SMALL_S / 2, sg = start ? 1 : -1, m = TN / 4.0;
    Pts p = { .n = 0 };
    pt(&p, m - sg * q * 0.75, m - q); pt(&p, m + sg * q * 0.95, m); pt(&p, m - sg * q * 0.75, m + q);
    cv_poly(&cv, &p, 255);
    Mk mk = cv_mask(&cv), sh = mk_new(mk.w, mk.h);
    int off = SMALL_S / 8;
    for (int y = off; y < mk.h; y++)
        for (int x = off; x < mk.w; x++)
            MV(sh, x, y) = MV(mk, x - off, y - off);
    static const float black[3] = { 0, 0, 0 };
    ink(a, sh, black, 0, 0, 0.6f);
    ink(a, mk, k_ink, 0, 0, 1);
    return a;
}

/* The touch layout's switch (src/touch_pad.c): Back's small black ball with two arrows, one over the other, the
 * top one right and the lower one left, in Start and Back's ink and shadow */
static Im swap_piece(void)
{
    Im a = ball_black(SMALL_S, 1);
    Cv cv = cv_new(TN / 2, TN / 2, 2);
    double q = 3.0 * SMALL_S / 2, m = TN / 4.0;
    for (int k = 0; k < 2; k++) {
        double sg = k ? -1 : 1, yc = m + (k ? 0.5 : -0.5) * q, t = 0.15 * q;
        Pts p = { .n = 0 };
        pt(&p, m - sg * 0.95 * q, yc - t); pt(&p, m + sg * 0.15 * q, yc - t);
        pt(&p, m + sg * 0.15 * q, yc - 0.42 * q); pt(&p, m + sg * 0.95 * q, yc);
        pt(&p, m + sg * 0.15 * q, yc + 0.42 * q); pt(&p, m + sg * 0.15 * q, yc + t);
        pt(&p, m - sg * 0.95 * q, yc + t);
        cv_poly(&cv, &p, 255);
    }
    Mk mk = cv_mask(&cv), sh = mk_new(mk.w, mk.h);
    int off = SMALL_S / 8;
    for (int y = off; y < mk.h; y++)
        for (int x = off; x < mk.w; x++)
            MV(sh, x, y) = MV(mk, x - off, y - off);
    static const float black[3] = { 0, 0, 0 };
    ink(a, sh, black, 0, 0, 0.6f);
    ink(a, mk, k_ink, 0, 0, 1);
    return a;
}

/* The touch controls' opacity button (src/touch_pad.c): the switch's ball with a circle half filled, in the same
 * ink and shadow */
static Im opacity_piece(void)
{
    Im a = ball_black(SMALL_S, 1);
    double q = 3.0 * SMALL_S / 2, m = TN / 4.0, r = 0.85 * q, w = 0.17 * q;
    Cv ring = cv_new(TN / 2, TN / 2, 2), half = cv_new(TN / 2, TN / 2, 2);
    cv_disc(&ring, m, m, r, 255);
    cv_disc(&ring, m, m, r - w, 0);
    cv_disc(&half, m, m, r - 1.6 * w, 255);
    cv_rect(&half, m, m - r, m + r, m + r, 0);
    Mk mk = cv_mask(&ring), mh = cv_mask(&half), sh = mk_new(mk.w, mk.h);
    for (int i = 0; i < mk.w * mk.h; i++)
        mk.m[i] = fmaxf(mk.m[i], mh.m[i]);
    int off = SMALL_S / 8;
    for (int y = off; y < mk.h; y++)
        for (int x = off; x < mk.w; x++)
            MV(sh, x, y) = MV(mk, x - off, y - off);
    static const float black[3] = { 0, 0, 0 };
    ink(a, sh, black, 0, 0, 0.6f);
    ink(a, mk, k_ink, 0, 0, 1);
    return a;
}

/* The L / R plate without its letter, w x h at scale s: the game's face and its two-step bevels (their greys
 * the plate's own, read mid-edge), the corners mitred */
static Im plate_box(int w, int h, int s)
{
    Im g = plate('L'), o = im_new(w * s, h * s);
    const float *tone[4][2] = {            /* left, top, right, bottom: the outer step, the inner */
        { PX(g, 0, 3), PX(g, 1, 3) }, { PX(g, 10, 0), PX(g, 10, 1) },
        { PX(g, 20, 10), PX(g, 19, 10) }, { PX(g, 10, 20), PX(g, 10, 19) },
    };
    for (int y = 0; y < o.h; y++)
        for (int x = 0; x < o.w; x++) {
            double acc[3] = { 0, 0, 0 };
            for (int q = 0; q < 16; q++) {
                double u = (x + ((q & 3) + 0.5) / 4) / s, v = (y + ((q >> 2) + 0.5) / 4) / s;
                double d[4] = { u, v, w - u, h - v };
                int k = 0;
                for (int i = 1; i < 4; i++)
                    if (d[i] < d[k])
                        k = i;
                for (int c = 0; c < 3; c++)
                    acc[c] += d[k] >= 2 ? PLATE_FACE : tone[k][d[k] >= 1][c];
            }
            float *p = PX(o, x, y);
            for (int c = 0; c < 3; c++)
                p[c] = (float)(acc[c] / 16);
            p[3] = 1;
        }
    return o;
}

static Im plate_drawn(int s)
{
    return plate_box(21, 21, s);
}

static Im trigger(const char *text)
{
    Im p = plate_drawn(TS);
    plate_text(p, text, 21, 21, TS, 1.0);
    return centred(p, TN, p.w / 2.0, p.h / 2.0);
}

/* The sticks' knob and their clicks: the gem dark, as the prompts' disc is */
static Im knob(void)
{
    static const double base[3] = { 0.17, 0.17, 0.18 };   /* (the stick prompts': stick()) */
    return dark_disc(base, 1.0, TS);
}

/* The d-pad drawn at this size in the look of the game's icon; lit: 0 none, 1 n 2 s 3 w 4 e */
static Im dpad_piece_at(int lit, int n, double s)   /* n across, s pixels a game pixel */
{
    const double R = 10.9, hw = 3.0, lw = 0.75;
    static const double bv[6][2] = { { -11, 0.17 }, { -3, 0.27 }, { -1, 0.33 }, { 3, 0.50 }, { 7, 0.60 }, { 11, 0.74 } };
    static const double kl[4][2] = { { 3, 0.10 }, { 6, 0.32 }, { 9, 0.50 }, { 11, 0.62 } };
    static const double kr[3][2] = { { 3, 0.20 }, { 6, 0.38 }, { 11, 0.52 } };
    Im d = im_new(n, n);
    Mk disc = mk_new(n, n), cr = mk_new(n, n);
    for (int Y = 0; Y < n; Y++)
        for (int X = 0; X < n; X++) {
            double x = gridv(X, n, s), y = gridv(Y, n, s), r = hypot(x, y);
            float *p = PX(d, X, Y);
            float dk = aa(r - R, s);
            MV(disc, X, Y) = dk;
            double ground = 0.165 + 0.085 * clampd((y + 11) / 22, 0, 1);
            put_px(p, ground, ground, ground, dk);
            float inv = aa(fmax(fabs(x) - hw, fabs(y) - 11.5), s), inh = aa(fmax(fabs(y) - hw, fabs(x) - 11.5), s);
            double body_v = interp(y, bv, 6);
            double body_h = (0.22 + 0.19 * clampd((y + hw) / (2 * hw), 0, 1)) * (0.86 + 0.26 * (1 - clampd(fabs(x) / 11, 0, 1)));
            int in_side = fabs(x) > hw;
            double body = in_side ? body_h : body_v;
            float cross = fmaxf(inv, inh);
            MV(cr, X, Y) = cross;
            put_px(p, body, body, body, cross * dk);
            for (int q = 0; q < 2; q++) {
                double dx = q ? 0.0 : 0.28, dy = q ? 0.28 : 0.0;
                float sh = fmaxf(aa(fmax(fabs(x - dx) - hw, fabs(y - dy) - 11.5), s),
                                 aa(fmax(fabs(y - dy) - hw, fabs(x - dx) - 11.5), s));
                put_px(p, 0, 0, 0, clip01(sh - cross) * dk * 0.45);
            }
            double top_edge = aa(fabs(y + hw - lw / 2) - lw / 2, s) * in_side * (fabs(x) < 11.5);
            double k = x < 0 ? interp(-x, kl, 4) : interp(x, kr, 3);
            put_px(p, 1, 1, 1, top_edge * k * dk);
            double left_edge = aa(fabs(x + hw - lw / 2) - lw / 2, s) * (y < -hw) * (y > -11.5);
            put_px(p, 1, 1, 1, left_edge * 0.30 * dk);
            double cap_t = clampd(1 - (y + 11.0) / 1.5, 0, 1) * (fabs(x) < hw);
            put_px(p, 1, 1, 1, cap_t * 0.75 * dk);
            double cap_l = clampd(1 - (x + 11.2) / 1.3, 0, 1) * (fabs(y) < hw);
            put_px(p, 1, 1, 1, cap_l * 0.36 * dk);
            put_px(p, 1, 1, 1, aa(fabs(x - 9.9) - 0.3, s) * (fabs(y) < hw) * 0.20 * dk);
            put_px(p, 0.78, 0.78, 0.78, aa(fabs(r - R + 0.35) - 0.33, s) * 0.55);
            put_px(p, 0, 0, 0, clampd(1 - (hypot(x - 0.35, y - 0.5) - R) / 1.2, 0, 1) * 0.35 * (1 - dk));
        }
    if (lit) {
        static const int lx[5] = { 0, 0, 0, -1, 1 }, ly[5] = { 0, -1, 1, 0, 0 };
        Mk m = mk_new(n, n);
        for (int Y = 0; Y < n; Y++)
            for (int X = 0; X < n; X++) {
                double x = gridv(X, n, s), y = gridv(Y, n, s);
                double along = x * lx[lit] + y * ly[lit], across = fabs(x * ly[lit] - y * lx[lit]);
                MV(m, X, Y) = (float)(clip01((along - hw) * s + 0.5) * aa(across - hw, s) * MV(disc, X, Y));
            }
        Mk b = blur(m, 1.1 * s);
        for (int i = 0; i < n * n; i++) {
            double glow = pow(clip01(b.m[i] * 1.7), 1.4) * (1 - m.m[i]) * disc.m[i];
            put_px(&d.p[4 * i], 1, 1, 1, glow * 0.55);
            for (int c = 0; c < 3; c++)
                d.p[4 * i + c] = (float)(d.p[4 * i + c] + (0.86 - d.p[4 * i + c]) * 0.55 * m.m[i]);   /* (every arm lit alike: the
                                                                                         * bottom one went white, the top barely) */
        }
    }
    return d;
}

static Im dpad_piece(int lit)
{
    return dpad_piece_at(lit, 384, 16);
}

static Im dpad_arm(int lit)              /* 1 n 2 s 3 w 4 e: the arm alone, on 192 x 192 */
{
    static const int lx[5] = { 0, 0, 0, -1, 1 }, ly[5] = { 0, -1, 1, 0, 0 };
    Im full = dpad_piece(lit), out = im_new(192, 192);
    int ax = lx[lit], ay = ly[lit], cx = 192 + ax * 128, cy = 192 + ay * 128, h = 96;
    int y0 = cy - h > 0 ? cy - h : 0, x0 = cx - h > 0 ? cx - h : 0, y1 = cy + h < 384 ? cy + h : 384,
        x1 = cx + h < 384 ? cx + h : 384;
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++) {
            float *q = PX(full, x, y), *p = PX(out, x - (cx - h), y - (cy - h));
            double along = (x - 192) * ax + (y - 192) * ay, across = fabs((double)(x - 192) * ay - (double)(y - 192) * ax);
            memcpy(p, q, sizeof(float) * 4);
            p[3] = (float)(q[3] * clampd((along - 30) / 16, 0, 1) * clampd((58 - across) / 10, 0, 1));
        }
    return out;
}

static Im well(int lit)
{
    Im d = im_new(384, 384);
    double ring = lit ? 0.9 : 0.38, fill = lit ? 0.28 : 0.22;
    for (int Y = 0; Y < 384; Y++)
        for (int X = 0; X < 384; X++) {
            double x = gridv(X, 384, 16), y = gridv(Y, 384, 16), r = hypot(x, y);
            float *p = PX(d, X, Y);
            put_px(p, 0.03, 0.03, 0.035, aa(r - 11.6, 16) * fill);
            put_px(p, 1, 1, 1, aa(fabs(r - 11.5) - 0.1, 16) * ring);
            if (lit)
                put_px(p, 1, 1, 1, pow(clampd(1 - fabs(r - 11.5) / 0.6, 0, 1), 1.5) * 0.45);
        }
    return d;
}

/* ---------- the pictures by name ---------- */

static const struct { const char *name, *id; } k_names[] = {
    { "PS3_Cross", "ps/Cross" }, { "PS3_Circle", "ps/Circle" }, { "PS3_Square", "ps/Square" },
    { "PS3_Triangle", "ps/Triangle" }, { "PS3_L1", "ps/L1" }, { "PS3_R1", "ps/R1" }, { "PS3_L2", "ps/L2" },
    { "PS3_R2", "ps/R2" }, { "PS3_Start", "ps/Options" }, { "PS3_Select", "ps/Create" },
    { "PS3_Left_Stick_Click", "xbox/LS_Click" }, { "PS3_Right_Stick_Click", "xbox/RS_Click" },
    { "PS3_Dpad_Up", "xbox/Dpad_Up" }, { "PS3_Dpad_Down", "xbox/Dpad_Down" }, { "PS3_Dpad_Left", "xbox/Dpad_Left" },
    { "PS3_Dpad_Right", "xbox/Dpad_Right" }, { "PS3_Left_Stick", "xbox/LS" }, { "PS3_Right_Stick", "xbox/RS" },
    { "Switch_A", "nx/A" }, { "Switch_B", "nx/B" }, { "Switch_X", "nx/X" }, { "Switch_Y", "nx/Y" },
    { "Switch_LB", "nx/L" }, { "Switch_RB", "nx/R" }, { "Switch_LT", "nx/ZL" }, { "Switch_RT", "nx/ZR" },
    { "Switch_Plus", "nx/Plus" }, { "Switch_Minus", "nx/Minus" }, { "Switch_Left_Stick_Click", "xbox/LS_Click" },
    { "Switch_Right_Stick_Click", "xbox/RS_Click" }, { "Switch_Dpad_Up", "xbox/Dpad_Up" },
    { "Switch_Dpad_Down", "xbox/Dpad_Down" }, { "Switch_Dpad_Left", "xbox/Dpad_Left" },
    { "Switch_Dpad_Right", "xbox/Dpad_Right" }, { "Switch_Left_Stick", "xbox/LS" }, { "Switch_Right_Stick", "xbox/RS" },
    { "360_A", "xbox/A" }, { "360_B", "xbox/B" }, { "360_X", "xbox/X" }, { "360_Y", "xbox/Y" }, { "360_LB", "xbox/LB" },
    { "360_RB", "xbox/RB" }, { "360_LT", "xbox/LT" }, { "360_RT", "xbox/RT" }, { "360_Start", "xbox/Start" },
    { "360_Back", "xbox/Back" }, { "360_Left_Stick_Click", "xbox/LS_Click" },
    { "360_Right_Stick_Click", "xbox/RS_Click" }, { "360_Dpad_Up", "xbox/Dpad_Up" },
    { "360_Dpad_Down", "xbox/Dpad_Down" }, { "360_Dpad_Left", "xbox/Dpad_Left" },
    { "360_Dpad_Right", "xbox/Dpad_Right" }, { "360_Left_Stick", "xbox/LS" }, { "360_Right_Stick", "xbox/RS" },
    { "Space_Key_Dark", "keys/Space" }, { "Shift_Key_Dark", "keys/Shift" }, { "Ctrl_Key_Dark", "keys/Ctrl" },
    { "Alt_Key_Dark", "keys/Alt" }, { "Tab_Key_Dark", "keys/Tab" }, { "Esc_Key_Dark", "keys/Esc" },
    { "Enter_Key_Dark", "keys/Enter" }, { "Backspace_Key_Dark", "keys/Backspace" },
    { "Caps_Lock_Key_Dark", "keys/CapsLock" }, { "Del_Key_Dark", "keys/Delete" }, { "Insert_Key_Dark", "keys/Insert" },
    { "Home_Key_Dark", "keys/Home" }, { "End_Key_Dark", "keys/End" }, { "Page_Up_Key_Dark", "keys/PageUp" },
    { "Page_Down_Key_Dark", "keys/PageDown" }, { "Arrow_Up_Key_Dark", "keys/Up" }, { "Arrow_Down_Key_Dark", "keys/Down" },
    { "Arrow_Left_Key_Dark", "keys/Left" }, { "Arrow_Right_Key_Dark", "keys/Right" }, { "Minus_Key_Dark", "keys/Minus" },
    { "Plus_Key_Dark", "keys/KpPlus" }, { "Bracket_Left_Key_Dark", "keys/LBracket" },
    { "Bracket_Right_Key_Dark", "keys/RBracket" }, { "Semicolon_Key_Dark", "keys/Semicolon" },
    { "Quote_Key_Dark", "keys/Quote" }, { "Slash_Key_Dark", "keys/Slash" }, { "Tilda_Key_Dark", "keys/Grave" },
    { "Mark_Left_Key_Dark", "keys/Comma" }, { "Mark_Right_Key_Dark", "keys/Period" },
    { "Num_Lock_Key_Dark", "keys/NumLock" }, { "Print_Screen_Key_Dark", "keys/PrintScreen" },
    { "Win_Key_Dark", "keys/Win" }, { "Mouse_Left_Key_Dark", "keys/LMB" }, { "Mouse_Middle_Key_Dark", "keys/MMB" },
    { "Mouse_Right_Key_Dark", "keys/RMB" }, { "Blank_Black_Normal", "keys/Blank" },
};

/* The keys whose legend is a word (mock_keys.py's KEYS, the 4x legends) */
static const struct { const char *key, *a, *b; } k_words[] = {
    { "Space", "Space", NULL }, { "Shift", "Shift", NULL }, { "Ctrl", "Ctrl", NULL }, { "Alt", "Alt", NULL },
    { "Tab", "Tab", NULL }, { "Esc", "Esc", NULL }, { "Enter", "Enter", NULL }, { "Backspace", "Bksp", NULL },
    { "CapsLock", "Caps", NULL }, { "Delete", "Del", NULL }, { "Insert", "Ins", NULL }, { "Home", "Home", NULL },
    { "End", "End", NULL }, { "PageUp", "Page", "Up" }, { "PageDown", "Page", "Down" }, { "Minus", "-", NULL },
    { "KpPlus", "+", NULL }, { "LBracket", "[", NULL }, { "RBracket", "]", NULL }, { "Semicolon", ";", NULL },
    { "Quote", "'", NULL }, { "Slash", "/", NULL }, { "Grave", "~", NULL }, { "Comma", ",", NULL },
    { "Period", ".", NULL }, { "NumLock", "Num", "Lock" }, { "PrintScreen", "Prt", "Scr" },
};

/* One picture by its id ("ps/Cross"...), at scale s; 0: no such */
static int build_id(const char *id, int s, Im *out)
{
    const char *n = strchr(id, '/');
    if (!n)
        return 0;
    n++;
    if (!strncmp(id, "ps/", 3)) {
        if (!strcmp(n, "Cross") || !strcmp(n, "Circle") || !strcmp(n, "Square") || !strcmp(n, "Triangle"))
            *out = ps_face(n, s);
        else if (!strcmp(n, "L2") || !strcmp(n, "R2"))
            *out = plate_label(n, 21, 21, s, 0.8);
        else if (!strcmp(n, "L1") || !strcmp(n, "R1"))
            *out = plate_label(n, 21, 17, s, 0.8);
        else if (!strcmp(n, "Options") || !strcmp(n, "Create"))
            *out = ball_mark(n[0], s);
        else
            return 0;
        return 1;
    }
    if (!strncmp(id, "nx/", 3)) {
        if (strlen(n) == 1 && strchr("ABXY", n[0]))
            *out = nx_face(n[0], s);
        else if (!strcmp(n, "L") || !strcmp(n, "R"))
            *out = plate_label(n, 21, 17, s, 1.0);
        else if (!strcmp(n, "ZL"))
            *out = plate_label(n, 21, 21, s, 0.8);
        else if (!strcmp(n, "ZR"))
            *out = plate_label(n, 21, 21, s, 0.76);
        else if (!strcmp(n, "Plus") || !strcmp(n, "Minus"))
            *out = ball_mark(n[0] == 'P' ? '+' : '-', s);
        else
            return 0;
        return 1;
    }
    if (!strncmp(id, "xbox/", 5)) {
        static const char *const dirs[4] = { "Dpad_Up", "Dpad_Down", "Dpad_Left", "Dpad_Right" };
        int k = strlen(n) == 1 && strchr("XYBA", n[0]) ? (int)(strchr("XYBA", n[0]) - "XYBA") : -1;
        if (k >= 0)
            *out = s_hi ? gem_at(k, s) : smooth(face(k), s);
        else if (!strcmp(n, "LB"))
            *out = s_hi ? ball_black_n(s, 0, 25 * s) : smooth(crop(s_black, 0, 0, 25, 25), s);
        else if (!strcmp(n, "RB"))
            *out = s_hi ? ball_white_n(s, 13 * s) : smooth(crop(s_white, 0, 0, 13, 13), s);
        else if (!strcmp(n, "LT") || !strcmp(n, "RT")) {
            char l[2] = { n[0], 0 };
            *out = plate_label(l, 21, 21, s, 1.0);
        } else if (!strcmp(n, "Start") || !strcmp(n, "Back"))
            *out = ball(n[0] == 'S', s);
        else if (!strcmp(n, "LS") || !strcmp(n, "RS"))
            *out = stick(n[0], 0, s);
        else if (!strcmp(n, "LS_Click") || !strcmp(n, "RS_Click"))
            *out = stick(n[0], 1, s);
        else {
            int i;
            for (i = 0; i < 4 && strcmp(n, dirs[i]); i++)
                ;
            if (i == 4)
                return 0;
            *out = dpad_icon(i, s);
        }
        return 1;
    }
    if (!strncmp(id, "keys/", 5)) {
        if (!strcmp(n, "Tab_Q") || !strcmp(n, "Tab_E")) {
            *out = tab_key(n[4] == 'E', s);
            return 1;
        }
        if (strlen(n) == 1 && ((n[0] >= 'A' && n[0] <= 'Z') || (n[0] >= '0' && n[0] <= '9'))) {
            *out = key_g(n, s);
            return 1;
        }
        if (n[0] == 'F' && n[1] >= '1' && n[1] <= '9' && atoi(n + 1) <= 12) {
            *out = key_g(n, s);
            return 1;
        }
        if (!strcmp(n, "Up") || !strcmp(n, "Right") || !strcmp(n, "Down") || !strcmp(n, "Left")) {
            *out = key_arrow(!strcmp(n, "Up") ? 0 : !strcmp(n, "Right") ? 1 : !strcmp(n, "Down") ? 2 : 3, s);
            return 1;
        }
        if (!strcmp(n, "Win")) { *out = key_win(s); return 1; }
        if (!strcmp(n, "LMB") || !strcmp(n, "MMB") || !strcmp(n, "RMB")) { *out = key_mouse(n[0], s); return 1; }
        if (!strcmp(n, "Blank")) { *out = key_cap(s); return 1; }
        for (size_t i = 0; i < sizeof k_words / sizeof k_words[0]; i++)
            if (!strcmp(n, k_words[i].key)) {
                *out = k_words[i].b ? key_g2(k_words[i].a, k_words[i].b, s) : key_g(k_words[i].a, s);
                return 1;
            }
        return 0;
    }
    return 0;
}

static NgbPic to_pic(Im a)
{
    NgbPic p = { a.w, a.h, malloc((size_t)a.w * a.h * 4) };
    if (!p.px)
        abort();
    for (int i = 0; i < a.w * a.h * 4; i++)
        p.px[i] = (uint8_t)(clip01(a.p[i]) * 255 + 0.5f);
    return p;
}

/* ---------- the cache (game thread) ---------- */

/* A prompt as the game shows its own: its 4x drawing reduced to the game's size (by area), then enlarged again
 * bilinearly -- the renderer's filtering of the game's 21 x 23 buttons. Drawn sharp at 4x they looked another
 * resolution than the game's own beside them. On premultiplied colour. */
static Im game_res(Im a)
{
    const int k = 4;
    if (a.w % k || a.h % k)
        return a;
    int w = a.w / k, h = a.h / k;
    float *lo = tmp(sizeof(float) * 4 * w * h);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            double acc[4] = { 0, 0, 0, 0 };
            for (int j = 0; j < k; j++)
                for (int i = 0; i < k; i++) {
                    const float *p = PX(a, x * k + i, y * k + j);
                    acc[0] += p[0] * p[3], acc[1] += p[1] * p[3], acc[2] += p[2] * p[3], acc[3] += p[3];
                }
            for (int c = 0; c < 4; c++)
                lo[4 * (y * w + x) + c] = (float)(acc[c] / (k * k));
        }
    Im o = im_new(a.w, a.h);
    for (int y = 0; y < a.h; y++)
        for (int x = 0; x < a.w; x++) {
            double u = (x + 0.5) / k - 0.5, v = (y + 0.5) / k - 0.5;
            int x0 = (int)floor(u), y0 = (int)floor(v);
            double fx = u - x0, fy = v - y0, acc[4] = { 0, 0, 0, 0 };
            for (int j = 0; j < 2; j++)
                for (int i = 0; i < 2; i++) {
                    int xx = x0 + i < 0 ? 0 : x0 + i >= w ? w - 1 : x0 + i;
                    int yy = y0 + j < 0 ? 0 : y0 + j >= h ? h - 1 : y0 + j;
                    double wgt = (i ? fx : 1 - fx) * (j ? fy : 1 - fy);
                    for (int c = 0; c < 4; c++)
                        acc[c] += wgt * lo[4 * (yy * w + xx) + c];
                }
            float *d = PX(o, x, y);
            d[3] = clip01((float)acc[3]);
            for (int c = 0; c < 3; c++)
                d[c] = acc[3] > 1e-6 ? clip01((float)(acc[c] / acc[3])) : 0;
        }
    return o;
}

#define CACHE_N 640
static struct { char id[56]; NgbPic pic; int ok; } s_cache[CACHE_N];
static int s_ncache;

#ifndef GP_STANDALONE
int ngb_bind_prompt_high(void);       /* src/bindings.c: Button Resolution, 1 High */
#else
static int ngb_bind_prompt_high(void) { return getenv("GP_HIGH") != NULL; }
#endif

static const NgbPic *by_id(const char *id)
{
    if (!s_have)
        return NULL;
    int hi = ngb_bind_prompt_high();
    char key[56];
    snprintf(key, sizeof key, "%s%s", hi ? "high/" : "", id);   /* (both kept: the setting goes back and forth) */
    for (int i = 0; i < s_ncache; i++)
        if (!strcmp(s_cache[i].id, key))
            return s_cache[i].ok ? &s_cache[i].pic : NULL;
    if (s_ncache == CACHE_N)
        return NULL;
    Im a;
    s_hi = hi;
    int ok = build_id(id, 4, &a);
    s_hi = 0;
    if (ok && !hi)
        a = game_res(a);
    int i = s_ncache++;
    snprintf(s_cache[i].id, sizeof s_cache[i].id, "%s", key);
    s_cache[i].ok = ok;
    if (ok)
        s_cache[i].pic = to_pic(a);
    tmp_release();
    return ok ? &s_cache[i].pic : NULL;
}

const NgbPic *ngb_gp_prompt(const char *name)
{
    if (!name || !s_have)
        return NULL;
    for (size_t i = 0; i < sizeof k_names / sizeof k_names[0]; i++)
        if (!strcmp(k_names[i].name, name))
            return by_id(k_names[i].id);
    /* "A_Key_Dark", "7_Key_Dark", "F5_Key_Dark" */
    const char *u = strstr(name, "_Key_Dark");
    if (u && u - name <= 3 && !u[9]) {
        char id[16];
        snprintf(id, sizeof id, "keys/%.*s", (int)(u - name), name);
        return by_id(id);
    }
    return NULL;
}

const NgbPic *ngb_gp_tab_key(int e)
{
    return by_id(e ? "keys/Tab_E" : "keys/Tab_Q");
}

const NgbPic *ngb_gp_key_label(const char *label)
{
    if (!s_have || !label || !*label)
        return NULL;
    char text[24];
    int n = 0;
    for (const char *s = label; *s; s++) {               /* only what the game's font has, short */
        int pg, r, c;
        if (n == 12 || (*s != ' ' && !glyph_at(*s, &pg, &r, &c)))
            return NULL;
        text[n++] = *s;
    }
    text[n] = 0;
    int hi = ngb_bind_prompt_high();
    char id[56];
    snprintf(id, sizeof id, "%slabel/%s", hi ? "high/" : "", text);
    for (int i = 0; i < s_ncache; i++)
        if (!strcmp(s_cache[i].id, id))
            return s_cache[i].ok ? &s_cache[i].pic : NULL;
    if (s_ncache == CACHE_N)
        return NULL;
    s_hi = hi;
    Im a = key_g(text, 4);
    s_hi = 0;
    if (!hi)
        a = game_res(a);
    int i = s_ncache++;
    snprintf(s_cache[i].id, sizeof s_cache[i].id, "%s", id);
    s_cache[i].ok = 1;
    s_cache[i].pic = to_pic(a);
    tmp_release();
    return &s_cache[i].pic;
}

/* ---------- the touch pieces (a thread of their own) ---------- */

static const char *const k_touch[] = {
    "button_a", "button_b", "button_x", "button_y", "button_black", "button_white", "trigger_l", "trigger_r",
    "back", "start", "click_l", "click_r", "nub",
    "button_a_lit", "button_b_lit", "button_x_lit", "button_y_lit", "button_black_lit", "button_white_lit",
    "trigger_l_lit", "trigger_r_lit", "back_lit", "start_lit", "click_l_lit", "click_r_lit", "nub_lit",
    "pad", "pad_lit", "dpad", "dpad_north_lit", "dpad_south_lit", "dpad_west_lit", "dpad_east_lit",
    "swap", "swap_lit", "opacity", "opacity_lit",
};
#define TOUCH_N ((int)(sizeof k_touch / sizeof k_touch[0]))
static NgbPic s_touch[TOUCH_N];
static int s_touch_ready, s_touch_started;

static Im touch_base(int k)
{
    switch (k) {
    case 0: case 1: case 2: case 3: return gem("\3\2\0\1"[k]);
    case 4: return ball_black(BALL_S, 0);
    case 5: return ball_white(BALL_S);
    case 6: return trigger("L");
    case 7: return trigger("R");
    case 8: return start_back(0);
    case 9: return start_back(1);
    case 10: return centred(stick_on(knob(), 'L', 1, TS), TN, CX * TS, CY * TS);
    case 11: return centred(stick_on(knob(), 'R', 1, TS), TN, CX * TS, CY * TS);
    default: return centred(knob(), TN, CX * TS, CY * TS);
    }
}

static void build_touch(void)
{
    /* the touch pieces are the Button Resolution High prompts at their own size: the faces fitted to the game's
     * (gem_hi), the sticks' disc with its drawn ring (s_hi is this thread's own) */
    s_hi = 1;
    for (int k = 0; k < 13; k++) {     /* a piece at a time, its scratch memory let go after each (a phone's) */
        Im base = touch_base(k);
        s_touch[k] = to_pic(base);
        Im a;
        if (k == 4 || k == 8 || k == 9) {        /* they carry the game's halo already: held, it is stronger */
            a = im_copy(base);
            double lim = BALL_R * (k == 4 ? BALL_S : SMALL_S);
            for (int y = 0; y < TN; y++)
                for (int x = 0; x < TN; x++) {
                    float *p = PX(a, x, y);
                    if (hypot(x + 0.5 - TN / 2.0, y + 0.5 - TN / 2.0) < lim)
                        for (int c = 0; c < 3; c++)
                            p[c] = clip01(p[c] * 1.25 + 0.05);
                    else
                        p[3] = clip01(p[3] * 1.9);
                }
        } else {
            a = held(base, k == 5 ? BALL_S : TS);
        }
        s_touch[13 + k] = to_pic(a);
        tmp_release();
    }
    s_touch[26] = to_pic(well(0));
    s_touch[27] = to_pic(well(1));
    tmp_release();
    s_touch[28] = to_pic(dpad_piece(0));
    tmp_release();
    for (int l = 1; l <= 4; l++) {
        s_touch[28 + l] = to_pic(dpad_arm(l));
        tmp_release();
    }
    for (int k = 0; k < 2; k++) {   /* the layout switch and the opacity button, held as Back is */
        Im base = k ? opacity_piece() : swap_piece(), lit;
        s_touch[33 + 2 * k] = to_pic(base);
        lit = im_copy(base);
        double lim = BALL_R * SMALL_S;
        for (int y = 0; y < TN; y++)
            for (int x = 0; x < TN; x++) {
                float *p = PX(lit, x, y);
                if (hypot(x + 0.5 - TN / 2.0, y + 0.5 - TN / 2.0) < lim)
                    for (int c = 0; c < 3; c++)
                        p[c] = clip01(p[c] * 1.25 + 0.05);
                else
                    p[3] = clip01(p[3] * 1.9);
            }
        s_touch[34 + 2 * k] = to_pic(lit);
        tmp_release();
    }
    s_hi = 0;
}

/* The pieces as saved by an earlier start (ngb_gp_touch_saved), shown until this start's are made -- or instead
 * of making them again */
static NgbPic s_touch_saved[TOUCH_N];
static int s_touch_saved_ok;

const NgbPic *ngb_gp_touch(const char *piece)
{
    const NgbPic *set = __atomic_load_n(&s_touch_ready, __ATOMIC_ACQUIRE)      ? s_touch
                      : __atomic_load_n(&s_touch_saved_ok, __ATOMIC_ACQUIRE) ? s_touch_saved
                                                                              : NULL;
    if (!set || !piece)
        return NULL;
    for (int i = 0; i < TOUCH_N; i++)
        if (!strcmp(k_touch[i], piece))
            return &set[i];
    return NULL;
}

/* ---------- the comparison dump: NGB_PROMPT_DUMP=<dir> ---------- */

int stbi_write_png(const char *filename, int w, int h, int comp, const void *data, int stride_in_bytes);

static void mkdirs(const char *dir)
{
    char p[1024];
    snprintf(p, sizeof p, "%s", dir);
    for (char *q = p + 1; *q; q++)
        if (*q == '/') {
            *q = 0;
#ifdef _WIN32
            _mkdir(p);
#else
            mkdir(p, 0755);
#endif
            *q = '/';
        }
#ifdef _WIN32
    _mkdir(p);
#else
    mkdir(p, 0755);
#endif
}

static void write_pic(const char *dir, const char *rel, const NgbPic *p)
{
    char path[1200];
    snprintf(path, sizeof path, "%s/%s", dir, rel);
    char *slash = strrchr(path, '/');
    *slash = 0;
    mkdirs(path);
    *slash = '/';
    if (p && p->px)
        stbi_write_png(path, p->w, p->h, 4, p->px, p->w * 4);
}

static void dump_prompts(const char *dir)
{
    static const char *const sets[] = {
        "ps/Cross", "ps/Circle", "ps/Square", "ps/Triangle", "ps/L1", "ps/R1", "ps/L2", "ps/R2",
        "nx/A", "nx/B", "nx/X", "nx/Y", "nx/L", "nx/R", "nx/ZL", "nx/ZR", "nx/Plus", "nx/Minus", "ps/Options", "ps/Create",
        "xbox/A", "xbox/B", "xbox/X", "xbox/Y", "xbox/LB", "xbox/RB", "xbox/LT", "xbox/RT", "xbox/Start", "xbox/Back",
        "xbox/LS", "xbox/RS", "xbox/LS_Click", "xbox/RS_Click", "xbox/Dpad_Up", "xbox/Dpad_Down", "xbox/Dpad_Left",
        "xbox/Dpad_Right", "keys/Tab_Q", "keys/Tab_E",
        "keys/Up", "keys/Down", "keys/Left", "keys/Right", "keys/Win", "keys/LMB", "keys/MMB", "keys/RMB", "keys/Blank",
    };
    char id[48], rel[64];
    for (size_t i = 0; i < sizeof sets / sizeof sets[0]; i++) {
        snprintf(rel, sizeof rel, "%s_4x.png", sets[i]);
        write_pic(dir, rel, by_id(sets[i]));
    }
    for (int c = 0; c < 36; c++) {
        snprintf(id, sizeof id, "keys/%c", c < 26 ? 'A' + c : '0' + c - 26);
        snprintf(rel, sizeof rel, "%s_4x.png", id);
        write_pic(dir, rel, by_id(id));
    }
    for (int f = 1; f <= 12; f++) {
        snprintf(id, sizeof id, "keys/F%d", f);
        snprintf(rel, sizeof rel, "%s_4x.png", id);
        write_pic(dir, rel, by_id(id));
    }
    for (size_t i = 0; i < sizeof k_words / sizeof k_words[0]; i++) {
        snprintf(id, sizeof id, "keys/%s", k_words[i].key);
        snprintf(rel, sizeof rel, "%s_4x.png", id);
        write_pic(dir, rel, by_id(id));
    }
    write_pic(dir, "label/Mouse 4_4x.png", ngb_gp_key_label("Mouse 4"));
}

static void dump_touch(const char *dir)
{
    char rel[96];
    for (int i = 0; i < TOUCH_N; i++) {
        snprintf(rel, sizeof rel, "touch/touch_%s.png", k_touch[i]);
        write_pic(dir, rel, &s_touch[i]);
    }
}

/* ---------- the touch pieces saved: <save folder>/touch-art (Android) ----------
 * Made from the player's own copy and kept only in the save folder (Android's backup may copy it): the next start shows
 * them from its first frame, before the game's textures are read, and does not make them again. version.txt,
 * written last, says which drawing of them they are; another (or none) and they are made and saved again. */

#define TOUCH_ART_VERSION 7   /* raised whenever the pieces are drawn differently (and each release redraws them) */

unsigned char *stbi_load_from_memory(const unsigned char *buf, int len, int *x, int *y, int *comp, int req);
void stbi_image_free(void *p);

static char s_touch_dir[4096];

static void touch_stamp(char *out, size_t n)
{
#ifndef NGB_VERSION
#define NGB_VERSION "dev"
#endif
    snprintf(out, n, "ngb touch art %d, %d pieces, %s\n", TOUCH_ART_VERSION, TOUCH_N, NGB_VERSION);
}

static uint8_t *file_bytes(const char *path, size_t *n)
{
    FILE *f = fopen(path, "rb");
    uint8_t *b = NULL;
    long size;
    if (!f)
        return NULL;
    if (fseek(f, 0, SEEK_END) == 0 && (size = ftell(f)) > 0 && size < (8 << 20) && fseek(f, 0, SEEK_SET) == 0 &&
        (b = malloc((size_t)size)) && fread(b, 1, (size_t)size, f) != (size_t)size) {
        free(b);
        b = NULL;
    }
    fclose(f);
    *n = b ? (size_t)size : 0;
    return b;
}

static int load_saved_touch(void)
{
    char path[4200], want[64], have[64] = "";
    snprintf(path, sizeof path, "%s/version.txt", s_touch_dir);
    FILE *f = fopen(path, "r");
    if (!f)
        return 0;
    if (!fgets(have, sizeof have, f))
        have[0] = 0;
    fclose(f);
    touch_stamp(want, sizeof want);
    if (strcmp(have, want)) {
        fprintf(stderr, "[PROMPTS] touch pieces in %s of another version: made again\n", s_touch_dir);
        return 0;
    }
    for (int i = 0; i < TOUCH_N; i++) {
        size_t n;
        int w, h, c;
        snprintf(path, sizeof path, "%s/%s.png", s_touch_dir, k_touch[i]);
        uint8_t *b = file_bytes(path, &n), *px = b ? stbi_load_from_memory(b, (int)n, &w, &h, &c, 4) : NULL;
        free(b);
        if (!px || w < 8 || h < 8 || w > 1024 || h > 1024) {
            fprintf(stderr, "[PROMPTS] touch piece %s not read: made again\n", path);
            if (px)
                stbi_image_free(px);
            for (int k = 0; k < i; k++) {
                stbi_image_free(s_touch_saved[k].px);
                s_touch_saved[k].px = NULL;
            }
            return 0;
        }
        s_touch_saved[i].w = w;
        s_touch_saved[i].h = h;
        s_touch_saved[i].px = px;
    }
    return 1;
}

unsigned char *stbi_write_png_to_mem(const unsigned char *pixels, int stride_bytes, int x, int y, int n, int *out_len);

/* One file written whole or not at all: to <path>.tmp, every write and the close checked (a full disk makes
 * stbi_write_png's own file writes fail unnoticed), then renamed over <path>. */
static int write_file_safe(const char *path, const void *data, size_t n)
{
    char tmp_path[4300];
    snprintf(tmp_path, sizeof tmp_path, "%s.tmp", path);
    FILE *f = fopen(tmp_path, "wb");
    if (!f)
        return 0;
    int ok = fwrite(data, 1, n, f) == n;
    ok = (fclose(f) == 0) && ok;
#ifdef _WIN32
    if (ok)
        remove(path);                 /* (rename does not replace a file there) */
#endif
    if (!ok || rename(tmp_path, path) != 0) {
        remove(tmp_path);
        return 0;
    }
    return 1;
}

static void save_touch(void)
{
    char path[4200], stamp[64];
    mkdirs(s_touch_dir);
    snprintf(path, sizeof path, "%s/version.txt", s_touch_dir);
    remove(path);                     /* (until all are written: a start meanwhile makes them again) */
    for (int i = 0; i < TOUCH_N; i++) {
        int len = 0;
        unsigned char *png = s_touch[i].px ? stbi_write_png_to_mem(s_touch[i].px, s_touch[i].w * 4, s_touch[i].w,
                                                                    s_touch[i].h, 4, &len) : NULL;
        snprintf(path, sizeof path, "%s/%s.png", s_touch_dir, k_touch[i]);
        int ok = png && len > 0 && write_file_safe(path, png, (size_t)len);
        free(png);
        if (!ok) {
            fprintf(stderr, "[PROMPTS] touch piece %s not saved (disk full?): made again next start\n", path);
            return;
        }
    }
    snprintf(path, sizeof path, "%s/version.txt", s_touch_dir);
    touch_stamp(stamp, sizeof stamp);
    if (write_file_safe(path, stamp, strlen(stamp)))
        fprintf(stderr, "[PROMPTS] touch pieces saved in %s\n", s_touch_dir);
}

/* At start, the save folder known (src/main.c): the pieces an earlier start saved, if they are this drawing's.
 * Android's only (NGB_TOUCH_ART=1 elsewhere, for testing: the pieces then made and saved there too). */
void ngb_gp_touch_saved(const char *save_dir)
{
#ifndef __ANDROID__
    if (!getenv("NGB_TOUCH_ART"))
        return;
#endif
    if (!save_dir || !*save_dir || s_touch_dir[0])
        return;
    snprintf(s_touch_dir, sizeof s_touch_dir, "%s/touch-art", save_dir);
    if (load_saved_touch()) {
        __atomic_store_n(&s_touch_saved_ok, 1, __ATOMIC_RELEASE);
        fprintf(stderr, "[PROMPTS] touch pieces read from %s\n", s_touch_dir);
    }
}

static void *touch_thread(void *arg)
{
    (void)arg;
    build_touch();
    __atomic_store_n(&s_touch_ready, 1, __ATOMIC_RELEASE);
    if (s_touch_dir[0] && !__atomic_load_n(&s_touch_saved_ok, __ATOMIC_ACQUIRE))
        save_touch();
    const char *dir = getenv("NGB_PROMPT_DUMP");
    if (dir && *dir)
        dump_touch(dir);
    return NULL;
}

void ngb_gp_touch_start(void)
{
    const char *dump = getenv("NGB_PROMPT_DUMP");
    if (__atomic_load_n(&s_touch_saved_ok, __ATOMIC_ACQUIRE) && !(dump && *dump))
        return;                       /* (saved by an earlier start: not made again) */
    if (!s_have || __atomic_exchange_n(&s_touch_started, 1, __ATOMIC_ACQ_REL))
        return;
    pthread_t t;
    if (pthread_create(&t, NULL, touch_thread, NULL) == 0)
        pthread_detach(t);
}

int ngb_gp_ready(void)
{
    return s_have;
}

static void sources_done(void)
{
    discs_init();                     /* (shared by both threads' pictures: made before the touch thread starts) */
    balls_init();
    tmp_release();
    s_have = 1;
    const char *dir = getenv("NGB_PROMPT_DUMP");
    if (dir && *dir) {
        dump_prompts(dir);
        fprintf(stderr, "[PROMPTS] made from the game's textures, written to %s\n", dir);
    }
#ifdef __ANDROID__
    ngb_gp_touch_start();
#else
    if ((dir && *dir) || s_touch_dir[0])
        ngb_gp_touch_start();         /* (for the dump, or NGB_TOUCH_ART's test: the touch controls are Android's) */
#endif
}

/* ---------- reading the game's textures ---------- */

static Im from_argb(const uint32_t *px, int w, int h)
{
    Im a = { w, h, malloc(sizeof(float) * 4 * (size_t)w * h) };
    if (!a.p)
        abort();
    for (int i = 0; i < w * h; i++) {
        uint32_t c = px[i];
        a.p[4 * i] = (c >> 16 & 255) / 255.0f;
        a.p[4 * i + 1] = (c >> 8 & 255) / 255.0f;
        a.p[4 * i + 2] = (c & 255) / 255.0f;
        a.p[4 * i + 3] = (c >> 24) / 255.0f;
    }
    return a;
}

#ifndef GP_STANDALONE

#include <SDL_timer.h>

uint32_t ngb_sprite_def(uint32_t id);                                    /* src/pause_tab.c */
uint32_t *ngb_guest_texture(uint32_t hdr, uint32_t pal_hdr, int *w, int *h);
uint32_t ngb_game_tex_header(uint32_t hdr);   /* the game's own for one of ours put in its place */

#define TEX_TABLE 0x8DBB28

static int s_bad;                     /* textures found not as expected */

/* The texture sprite id is cut from, if it is w x h and the sprite's cell starts at (u, v) texels */
static int read_sprite_tex(uint32_t id, int w, int h, int u, int v, Im *out)
{
    uint32_t def = ngb_sprite_def(id);
    if (!def)
        return 0;
    uint32_t idx = MEM32(def + 0x10);
    if (idx >= 0x400)
        return 0;
    uint32_t hdr = ngb_game_tex_header(MEM32(TEX_TABLE + idx * 8)), pal = MEM32(TEX_TABLE + 4 + idx * 8);
    if (!hdr)
        return 0;
    if (u >= 0 && (fabsf(MEMF(def) * w - u) > 0.01f || fabsf(MEMF(def + 4) * h - v) > 0.01f)) {
        if (getenv("NGB_PROMPT_LOG"))
            fprintf(stderr, "[PROMPTS] sprite %X: cell at %.2f, %.2f, not %d, %d\n", id, MEMF(def) * w,
                    MEMF(def + 4) * h, u, v);
        return 0;
    }
    int tw, th;
    uint32_t *px = ngb_guest_texture(hdr, pal, &tw, &th);
    if (getenv("NGB_PROMPT_LOG"))
        fprintf(stderr, "[PROMPTS] sprite %X: texture %u, %dx%d, cell at %.2f, %.2f\n", id, idx, px ? tw : 0,
                px ? th : 0, MEMF(def) * w, MEMF(def + 4) * h);
    if (!px)
        return 0;
    if (tw != w || th != h) {
        free(px);
        s_bad++;
        return 0;
    }
    *out = from_argb(px, w, h);
    free(px);
    return 1;
}

/* The button icons as expected: the X disc (cell 0) opaque in its middle, clear in the cell's corner */
static int t0_ok(void)
{
    if (PX(s_t0, 10, 10)[3] > 0.9f && PX(s_t0, 0, 22)[3] < 0.1f && PX(s_t0, 20, 0)[3] < 0.1f)
        return 1;
    free(s_t0.p);
    s_t0.p = NULL;
    s_bad++;
    return 0;
}

/* A font page as expected: white ink in the cell of 'A' (page 0) or 'a' (page 1); else let go */
static int font_page_ok(Im *pg, int row, int col)
{
    int ink = 0;
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            float *q = PX(*pg, col * 16 + x, row * 16 + y);
            ink += q[3] * lum3(q) > 0.5;
        }
    if (ink >= 6)
        return 1;
    free(pg->p);
    pg->p = NULL;
    s_bad++;
    return 0;
}

/* Called for each sprite the game queues: tried every quarter of a second (by the clock, not per call -- a
 * frame queues dozens), and while what is found is not as expected every 5 seconds, never given up for good:
 * meanwhile the prompts are typed (src/pause_tab.c) and the touch controls wait (src/touch_pad.c). */
int ngb_gp_poll(void)
{
    if (s_have)
        return 1;
    static int off;
    static uint32_t next;
    if (!off)
        off = getenv("NGB_GAME_PROMPTS_OFF") ? -1 : 1;   /* (testing: the typed prompts only) */
    if (off < 0)
        return 0;
    uint32_t now = SDL_GetTicks();
    if (next && (int32_t)(now - next) < 0)
        return 0;
    static int got;                     /* each source kept once read */
    int bad = s_bad;
    if (!(got & 1) && read_sprite_tex(5, 128, 32, 0, 0, &s_t0) && t0_ok()) got |= 1;
    if (!(got & 2) && read_sprite_tex(0xB, 32, 32, 0, 0, &s_black)) got |= 2;
    if (!(got & 4) && read_sprite_tex(0xC, 16, 16, 0, 0, &s_white)) got |= 4;
    if (!(got & 8) && read_sprite_tex(0xD, 32, 32, 0, 0, &s_dpad)) got |= 8;
    /* the menus' small font, its two pages of 16 x 16 cells (sprites 0xBB and 0xBC are each a whole page) */
    if (!(got & 16) && read_sprite_tex(0xBB, 256, 256, -1, -1, &s_font[0]) && font_page_ok(&s_font[0], 2, 15)) got |= 16;
    if (!(got & 32) && read_sprite_tex(0xBC, 256, 256, -1, -1, &s_font[1]) && font_page_ok(&s_font[1], 5, 0)) got |= 32;
    if (got != 63) {
        next = now + (s_bad > bad ? 5000 : 250);
        next += !next;
        if (s_bad > bad && getenv("NGB_PROMPT_LOG"))
            fprintf(stderr, "[PROMPTS] the game's art not as expected (%#x): tried again in 5 s\n", got);
        return 0;
    }
    sources_done();
    return 1;
}

#else  /* GP_STANDALONE: the mock-ups' decoded textures from files, everything written to argv[2] */

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static Im load_png(const char *dir, const char *name)
{
    char p[1024];
    snprintf(p, sizeof p, "%s/%s", dir, name);
    int w, h, n;
    uint8_t *d = stbi_load(p, &w, &h, &n, 4);
    if (!d) {
        fprintf(stderr, "no %s\n", p);
        exit(1);
    }
    Im a = { w, h, malloc(sizeof(float) * 4 * (size_t)w * h) };
    for (int i = 0; i < w * h * 4; i++)
        a.p[i] = d[i] / 255.0f;
    stbi_image_free(d);
    return a;
}

int ngb_gp_poll(void) { return s_have; }

int main(int argc, char **argv)
{
    if (argc < 3)
        return 2;
    s_t0 = load_png(argv[1], "tex_0.png");
    s_black = load_png(argv[1], "tex_1.png");
    s_white = load_png(argv[1], "tex_2.png");
    s_dpad = load_png(argv[1], "tex_3.png");
    s_font[0] = load_png(argv[1], "tex_32.png");
    s_font[1] = load_png(argv[1], "tex_33.png");
    s_have = 1;
    dump_prompts(argv[2]);
    build_touch();
    s_touch_ready = 1;
    dump_touch(argv[2]);
    return 0;
}
#endif
