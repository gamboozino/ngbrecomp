/*
 * pause_tab.c -- Settings, a seventh tab in the pause menu.
 *
 * The pause menu (Start in game) is menu 1 of the game's menu engine (see
 * src/pc_menu.c for the engine): page 0 is its row of tabs -- Weapon,
 * Ninpo, Item, Info, Map, Exit, rows 0-5 -- and choosing one opens that
 * tab's own page, whose rows the engine moves over and whose handler draws
 * it. The tab row itself is drawn by sub_0020CDA0 from two lists of sprite
 * ids (the plates and the glowing label of the chosen one) at x positions
 * from a table, sub_00182240 saying how many there are.
 *
 * Here the tab row gets a seventh tab, Settings, before Exit: a row like
 * Exit's but doing nothing when chosen (so the engine opens no page of its
 * own for it). On it, A gives its page the pad -- the recomp's settings
 * (src/ingame_menu.c's ngb_pc_setting) in three categories, Graphics,
 * Display and Startup, on a strip at the top as the game's own tabs have
 * theirs: left and right on the strip choose one, up and down move between
 * it and the category's rows, left, right or A change a row -- and B gives
 * it back; meanwhile the game sees no pad (src/ingame_menu.c,
 * ngb_pause_tab_key). NGB_NO_SETTINGS_TAB=1 leaves the six tabs. Its
 * sprites are ours: the plate is the game's own Exit plate, read from the
 * player's copy at run time, with the lettering taken off and "Settings"
 * put on in a similar typeface (EB Garamond, SIL Open Font License, built
 * in); the page is
 * drawn into a texture of ours in the menu's colours, in the game's own
 * menu letters (Liberation Serif, the same licence, until they are loaded). The game draws them all as it draws its own: through its
 * sprite list, sized and faded with the rest of the menu.
 *
 * The five functions after this are the lifted ones (listed in
 * src/manual_functions.json, so the pipeline leaves them out) with each
 * change marked "Settings:". When the textures cannot be made -- the Exit
 * plate not where it was, or not in a format read here -- the menu is the
 * game's own, six tabs.
 */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xbox_memory_layout.h"   /* xbox_HeapAlloc, xbox_ContiguousAlloc */
#include "launcher.h"                 /* ngb_vsync_name, ngb_vsync_step */

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

extern const unsigned char ngb_font_tab[], ngb_font_body[], ngb_font_label[];
extern const unsigned int ngb_font_tab_len, ngb_font_body_len;

int ngb_pc_setting(int which);
int ngb_settings_size_count(int aspect);              /* src/launcher.c */
void ngb_settings_size_at(int aspect, int i, int *w, int *h);   /* src/launcher.c */
void ngb_pc_set_setting(int which, int value);

#define PT_TAB      5             /* the tab's row: before Exit, which is 6 then */
#define PT_SPRITE   0x7F00        /* our sprite ids: plate, glow, page */
#define PT_TEX      0x516         /* our texture numbers (the game's end at 0x515) */
#define PT_N        3
#define DLG_SPRITE  (PT_SPRITE + 8)   /* the Are you sure? box: anywhere, not only the pause menu */
#define DLG_TEX     (PT_TEX + 8)

#define MENU_TABLES 0x42A280      /* the menu engine's pages, by menu */
#define PAUSE_PAGES 0x428E78      /* the pause menu's */
#define EXIT_PLATE  0x177         /* the Exit tab's plate sprite */

#define PAGE_W 450                /* the page, in the menu's 720x480: right */
#define PAGE_H 450                /* of the blade, under the tab row (only */
#define PAGE_X 250                /* its top 280 used) */
#define PAGE_Y 118
#define PAGE_TW 1024              /* its texture */
#define PAGE_TH 1024

/* ---------- the settings ---------- */

/* The settings, numbered as ngb_pc_setting has them. */
enum { S_RENDER, S_TEXTURE, S_SHADOW, S_DISPLAY, S_VSYNC, S_FPS, S_WINDOW, S_ASPECT,
       S_SKIP, S_LAUNCHER, S_MSAA, S_HEALTH, S_NINPO, S_ESSENCE, S_ALLDIFF, S_SCARABS, S_ALLCOST, S_EXTRACOST,
       S_GLOW, S_QUICKFREEZE, S_QUICKSCALE, S_INFITEMS, S_INFAMMO, S_POSAUDIO, S_HUD, S_AUDIOOUT,
       S_MISSIONS, S_ARCADE, S_MOVIES, S_LOADDETAIL, S_RTHREAD, S_SPARKTHICKNESS,
       S_UNLOCKS,                 /* not a setting: Cheats' row opening the Unlocks page */
       S_COUNT };

static const char *const s_label[S_COUNT] = {
    "Render Resolution", "Anisotropic Filtering", "Shadow Resolution",
    "Display", "V-Sync", "FPS Counter", "Window Size", "Aspect Ratio",
    "Skip Intros", "Settings at Startup", "Anti-Aliasing (MSAA)",
    "Infinite Health", "Infinite Ninpo", "Infinite Essence", "All Difficulties",
    "Golden Scarabs", "All Costumes", "Extra Costume", "Smoother Glow", "Quick Menu Pause",
    "Quick Menu", "Infinite Items", "Infinite Projectiles", "Positional Audio", "HUD",
    "Audio Output", "Missions", "Arcade Ninja Gaiden", "Movie Theater", "Lower Detail Under Load",
    "Experimental Render Thread", "Spark Thickness", "Unlocks",
};
static const char *const s_help[S_COUNT] = {
    "Draws the game at a multiple of the console's resolution.",
    "Keeps textures sharp at an angle. Off: the console's filtering.",
    "The detail of the characters' shadows; 1x is the console's.",
    "A window, the whole screen, or a window without a frame.",
    "No tearing. Mailbox: least delay. FIFO: steadiest. Adaptive: a late frame goes at once.",
    "Frames per second, in the corner of the screen.",
    "The size of the window when not full screen.",
    "The shape of the picture and the window sizes offered.",
    "Starts at the title screen. From the next start.",
    "The settings screen before the game. From the next start.",
    "Smooths the edges of the 3D: 2x to 8x samples a pixel.",
    "Ryu's health stays full. For this session only.",
    "Ninpo is always ready. For this session only.",
    "As much essence as can be held. For this session only.",
    "Every difficulty at New Game, Ninja Dog to Master Ninja.",
    "All 50: Muramasa gives every reward. For this session only.",
    "Every costume at the Costume Selection, and three it never offers.",
    "Costumes in the game it never offers. From the next area.",
    "A softer, fainter glow on bright lights. Off: the console's.",
    "The game pauses while the quick menu is open (once Ryu's action ends).",
    "A d-pad menu of weapons, Ninpo and items (Up and Down open it), at this size.",
    "Healing items, the Talisman of Rebirth and the Jewel of the Demon Seal are not used up.\nFor this session only.",
    "Shuriken, smoke bombs, arrows and spears are not used up. For this session only.",
    "Sounds placed left and right. Console: the sound chip's filters. Panning: their placement only, clearer.",
    "The health and Ninpo bar, weapons and the rest drawn over play. Off: a clear screen.",
    "Speakers. Auto: 5.1 when the output device has 5.1 or 7.1 channels, else stereo. From the next start.",
    "MISSIONS in the main menu without earning it.",
    "ARCADE NINJA GAIDEN in the main menu without earning it.",
    "MOVIE THEATER in the main menu without earning it.",
    "Eases off detail when busy, as on the Xbox; Ryu's shadow may flicker.",
    "Drawing on a thread of its own. May glitch. Needs restart.",
    "The thickness of the sparks drawn on a weapon hit. Default: the console's.",
    "Difficulties, costumes and the main menu's extras, without earning them. Saved.",
};

/* The categories: the strip's words, their rows. */
#define CATS 6
#define CAT_ROWS 9
#define CAT_CTL 5                 /* Controls: the bindings' list (src/bindings.c), not settings */
#define CAT_CHEAT 4
#define CAT_UNL CATS              /* Unlocks: a page of Cheats' (its row S_UNLOCKS), not on the strip */
/* Gameplay: optional changes to how the game plays (off unless chosen; docs/quick-menu.md) */
static const char *const s_cat_name[CATS] = { "Graphics", "Display", "Sound", "Gameplay", "Cheats", "Controls" };
static const signed char s_cat_rows[CATS + 1][CAT_ROWS] = {
    { S_RENDER, S_TEXTURE, S_MSAA, S_SHADOW, S_GLOW, S_LOADDETAIL, S_RTHREAD, S_SPARKTHICKNESS, -1 },   /* -1 ends a page shorter than CAT_ROWS */
    { S_ASPECT, S_WINDOW, S_DISPLAY, S_VSYNC, S_FPS, S_HUD, S_SKIP, -1 },
    { S_POSAUDIO, S_AUDIOOUT, -1 },
    { S_QUICKSCALE, S_QUICKFREEZE, -1 },
    { S_HEALTH, S_NINPO, S_ESSENCE, S_SCARABS, S_INFITEMS, S_INFAMMO, S_UNLOCKS, -1 },
    { -1 },
    { S_ALLDIFF, S_ALLCOST, S_MISSIONS, S_ARCADE, S_MOVIES, -1 },
};

#include "bindings.h"
/* the actions; then On/Off rows: Invert Camera X and Y (the third-person camera, #26), Invert First-Person X
 * (#33, #63; its up/down is the game's own 1st Person Camera Control), Start in Third Person (#23); then
 * Default Controls */
#define CTL_INVX A_COUNT
#define CTL_START (A_COUNT + 3)
#define CTL_MOUSE (A_COUNT + 4)   /* Camera Sensitivity: a value, Left / Right (#67) */
#define CTL_DEF  (A_COUNT + 5)
#define CTL_ROWS (A_COUNT + 6)
#define CTL_TOGGLE(r) ((r) >= CTL_INVX && (r) <= CTL_START)
static const char *const s_ctl_toggle_name[4] = {
    "Invert Camera X", "Invert Camera Y", "Invert First-Person X", "Start in Third Person",
};
static const char *const s_ctl_toggle_help[4] = {
    "Turns the third-person camera the other way left and right (play only).",
    "Turns the third-person camera the other way up and down (play only).",
    "Turns the first-person view the other way left and right (play only).",
    "After loading, the right stick turns the camera instead of looking in first person.",
};
static int ctl_toggle_get(int r)
{
    int k = r - CTL_INVX;
    return k == 3 ? ngb_bind_third_person_start() : k == 2 ? ngb_bind_invert(1, 0) : ngb_bind_invert(0, k);
}
static void ctl_toggle_flip(int r)
{
    int k = r - CTL_INVX;
    if (k == 3)
        ngb_bind_set_third_person_start(!ngb_bind_third_person_start());
    else if (k == 2)
        ngb_bind_set_invert(1, 0, !ngb_bind_invert(1, 0));
    else
        ngb_bind_set_invert(0, k, !ngb_bind_invert(0, k));
}

static int cat_count(int c)
{
    if (c == CAT_CTL)
        return CTL_ROWS;
    int n = 0;
    while (n < CAT_ROWS && s_cat_rows[c][n] >= 0)
        n++;
    return n;
}

static void value_text(int row, char *out, size_t n)
{
    int v = ngb_pc_setting(row);
    switch (row) {
    case S_RENDER:  snprintf(out, n, "%dx %dx%d", v, 720 * v, 480 * v); break;   /* the game's 720x480 times it (#105) */
    case S_TEXTURE:
    case S_MSAA:    if (v > 1) snprintf(out, n, "%dx", v); else snprintf(out, n, "Off"); break;
    case S_SHADOW:  snprintf(out, n, v == 1 ? "1x (console)" : "%dx", v); break;
    case S_DISPLAY: snprintf(out, n, "%s", v == 2 ? "Borderless" : v ? "Fullscreen" : "Windowed"); break;
    case S_VSYNC:   snprintf(out, n, "%s", ngb_vsync_name(v)); break;
    case S_WINDOW: {
        int w = 0, h = 0;
        ngb_settings_size_at(v / 8, v % 8, &w, &h);
        snprintf(out, n, "%d x %d", w, h);
        break;
    }
    case S_ASPECT:  snprintf(out, n, "%s", v == 3 ? "21:9" : v == 2 ? "16:10" : v ? "16:9" : "4:3"); break;
    case S_QUICKSCALE: if (v) snprintf(out, n, "%d%%", v); else snprintf(out, n, "Off"); break;
    case S_POSAUDIO: snprintf(out, n, "%s", v == 2 ? "Panning" : v ? "Console" : "Off"); break;
    case S_AUDIOOUT: snprintf(out, n, "%s", v == 2 ? "5.1" : v ? "Stereo" : "Auto"); break;
    case S_SPARKTHICKNESS: snprintf(out, n, "%s", v == 3 ? "Original" : v == 2 ? "Slightly Thin" : v == 1 ? "Thin" : "Very Thin"); break;
    case S_UNLOCKS: snprintf(out, n, "%s", ""); break;
    case S_SKIP:
    case S_ALLDIFF:
    case S_ALLCOST:
    case S_MISSIONS:
    case S_ARCADE:
    case S_MOVIES:
    case S_LAUNCHER: snprintf(out, n, "%s", v ? "Yes" : "No"); break;
    default:        snprintf(out, n, "%s", v ? "On" : "Off"); break;
    }
}

static void change(int row, int step)
{
    int v = ngb_pc_setting(row);
    switch (row) {
    case S_RENDER:
        v += step;
        ngb_pc_set_setting(0, v < 1 ? 10 : v > 10 ? 1 : v);
        break;
    case S_TEXTURE: {
        static const int steps[] = { 0, 2, 4, 8, 16 };
        int i = 0;
        while (i < 4 && steps[i] != v)
            i++;
        ngb_pc_set_setting(1, steps[(i + step + 5) % 5]);
        break;
    }
    case S_MSAA: {
        static const int steps[] = { 1, 2, 4, 8 };
        int i = 0;
        while (i < 3 && steps[i] != v)
            i++;
        ngb_pc_set_setting(S_MSAA, steps[(i + step + 4) % 4]);
        break;
    }
    case S_SHADOW: {
        static const int steps[] = { 1, 2, 4, 8, 10 };
        int i = 0;
        while (i < 4 && steps[i] != v)
            i++;
        ngb_pc_set_setting(2, steps[(i + step + 5) % 5]);
        break;
    }
    case S_DISPLAY:
        ngb_pc_set_setting(row, (v + (step > 0 ? 1 : 2)) % 3);
        break;
    case S_VSYNC:
        ngb_pc_set_setting(row, ngb_vsync_step(v, step));
        break;
    case S_QUICKSCALE:
        ngb_pc_set_setting(row, ngb_quick_scale_step(v, step));
        break;
    case S_POSAUDIO:
    case S_AUDIOOUT:
        ngb_pc_set_setting(row, (v + (step > 0 ? 1 : 2)) % 3);
        break;
    case S_ASPECT:
        ngb_pc_set_setting(row, (v + (step > 0 ? 1 : 3)) % 4);
        break;
    case S_SPARKTHICKNESS:
        ngb_pc_set_setting(row, (v + (step > 0 ? 1 : 3)) % 4);
        break;
    case S_WINDOW: {
        int n = ngb_settings_size_count(v / 8);
        ngb_pc_set_setting(row, v / 8 * 8 + (v % 8 + step + n) % n);
        break;
    }
    default:
        ngb_pc_set_setting(row, !v);
        break;
    }
}

/* ---------- pictures: RGBA (0xAARRGGBB) to and from the guest ---------- */

/* The console's texture layout: x and y bits interleaved, x first, the
 * longer side's left over at the top. */
static void swizzle_masks(unsigned w, unsigned h, uint32_t *mx, uint32_t *my)
{
    uint32_t bit = 1, m = 1;
    *mx = *my = 0;
    for (;;) {
        int more = 0;
        if (bit < w) { *mx |= m; m <<= 1; more = 1; }
        if (bit < h) { *my |= m; m <<= 1; more = 1; }
        if (!more)
            break;
        bit <<= 1;
    }
}

static uint32_t deposit(uint32_t v, uint32_t mask)
{
    uint32_t r = 0;
    for (uint32_t b = 1; mask; b <<= 1) {
        uint32_t low = mask & -mask;
        if (v & b)
            r |= low;
        mask &= mask - 1;
    }
    return r;
}

/* A texture of the game's (P8 or A8R8G8B8, swizzled) as RGBA; NULL if it
 * is neither. */
static uint32_t *read_texture(uint32_t hdr, uint32_t pal_hdr, int *w, int *h)
{
    uint32_t fmt = MEM32(hdr + 0xC), data = MEM32(hdr + 4) | 0x80000000u;
    unsigned kind = fmt >> 8 & 0xFF, lw = fmt >> 20 & 0xF, lh = fmt >> 24 & 0xF;
    uint32_t mx, my, *px;
    if ((kind != 0x0B && kind != 0x06) || lw > 10 || lh > 10)
        return NULL;
    if (kind == 0x0B && !pal_hdr)
        return NULL;
    *w = 1 << lw;
    *h = 1 << lh;
    px = malloc(sizeof *px * *w * *h);
    swizzle_masks(*w, *h, &mx, &my);
    for (int y = 0; y < *h; y++)
        for (int x = 0; x < *w; x++) {
            uint32_t o = deposit(x, mx) | deposit(y, my);
            if (kind == 0x0B) {
                uint32_t pal = MEM32(pal_hdr + 4) | 0x80000000u;
                px[y * *w + x] = MEM32(pal + 4 * MEM8(data + o));
            } else {
                px[y * *w + x] = MEM32(data + 4 * o);
            }
        }
    return px;
}

typedef struct {
    uint32_t hdr, data;           /* guest: the texture's header and texels */
    int w, h;
} GuestTex;

static GuestTex s_tex[PT_N];
static uint32_t s_dlg_def, s_dlg_entry;   /* guest: the box's sprite definition, texture entry */
static uint32_t s_entries;        /* guest: per texture {header, palette} */
static uint32_t s_defs;           /* guest: per sprite the game's definition */
static int s_ready = -1;          /* -1 not tried, 0 failed, 1 made */
static int s_enabled;             /* the engine's tables have the tab */

static int make_texture(GuestTex *t, int w, int h, uint32_t fmt_low)
{
    unsigned lw = 0, lh = 0;
    while ((1 << lw) < w) lw++;
    while ((1 << lh) < h) lh++;
    t->w = w;
    t->h = h;
    t->data = xbox_ContiguousAlloc((uint32_t)(4 * w * h), 4096);
    t->hdr = xbox_HeapAlloc(0x14, 16);
    if (!t->data || !t->hdr)
        return 0;
    MEM32(t->hdr) = 0x40001;                         /* a texture, one reference */
    MEM32(t->hdr + 4) = t->data & 0x0FFFFFFF;        /* physical */
    MEM32(t->hdr + 8) = 0;
    MEM32(t->hdr + 0xC) = (lh << 24) | (lw << 20) | (1 << 16) | (0x06 << 8) | fmt_low;
    MEM32(t->hdr + 0x10) = 0;
    return 1;
}

static void upload(const GuestTex *t, const uint32_t *px)
{
    uint32_t mx, my;
    swizzle_masks(t->w, t->h, &mx, &my);
    for (int y = 0; y < t->h; y++) {
        uint32_t oy = deposit(y, my);
        for (int x = 0; x < t->w; x++)
            MEM32(t->data + 4 * (deposit(x, mx) | oy)) = px[y * t->w + x];
    }
}

/* ---------- lettering ---------- */

static stbtt_fontinfo s_font_tab, s_font_body;

/* Or the game's own menu letters (once loaded): each byte a
 * sprite (the table at 0x430C48) cut from the font's textures, white with a
 * dark shadow; here only their white -- how much each texel is letter --
 * drawn larger, the capital px * 0.66 tall as the typeface's would be, each
 * letter as wide as its cell (the game spaces its text by them). */
#define GLYPH_MAP 0x430C48
static int s_game_font;
static struct { uint32_t tex; int w, h; float *ink; } s_gf[4];
static int s_ngf;
static float s_cap_top, s_cap_h;   /* the capital H, in texels from its cell's top */

uint32_t ngb_sprite_def(uint32_t id);

static int glyph_tex(uint32_t tex)
{
    for (int i = 0; i < s_ngf; i++)
        if (s_gf[i].tex == tex)
            return i;
    if (s_ngf == 4)
        return -1;
    uint32_t hdr = MEM32(0x8DBB28 + tex * 8), pal = MEM32(0x8DBB2C + tex * 8);
    int w, h;
    uint32_t *px = hdr ? read_texture(hdr, pal, &w, &h) : NULL;
    if (!px)
        return -1;
    float *ink = malloc(sizeof *ink * w * h);
    for (int i = 0; i < w * h; i++) {
        uint32_t c = px[i];
        float l = ((c >> 16 & 255) + (c >> 8 & 255) + (c & 255)) / 3.0f;
        float t = (l - 70) / 130;
        ink[i] = (c >> 24) / 255.0f * (t < 0 ? 0 : t > 1 ? 1 : t);
    }
    free(px);
    s_gf[s_ngf].tex = tex; s_gf[s_ngf].w = w; s_gf[s_ngf].h = h; s_gf[s_ngf].ink = ink;
    return s_ngf++;
}

/* Byte c's glyph: its texture (index into s_gf) and cell in texels. */
static int glyph(int c, float cell[4])
{
    uint32_t id = MEM32(GLYPH_MAP + 4 * (c & 255));
    uint32_t def = id ? ngb_sprite_def(id) : 0;
    if (!def)
        return -1;
    int t = glyph_tex(MEM32(def + 0x10));
    if (t < 0)
        return -1;
    cell[0] = MEMF(def) * s_gf[t].w;
    cell[1] = MEMF(def + 4) * s_gf[t].h;
    cell[2] = MEMF(def + 8) * s_gf[t].w;
    cell[3] = MEMF(def + 12) * s_gf[t].h;
    return t;
}

/* Once the font is there: the capital's place in its cell. */
static int game_font_ready(void)
{
    float c[4];
    int t = glyph('H', c);
    if (t < 0)
        return 0;
    int x0 = (int)c[0], x1 = (int)c[2], y0 = (int)c[1], y1 = (int)c[3], top = -1, bot = -1;
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++)
            if (s_gf[t].ink[y * s_gf[t].w + x] > 0.5f) {
                if (top < 0) top = y;
                bot = y + 1;
            }
    if (top < 0)
        return 0;
    s_cap_top = (float)(top - y0);
    s_cap_h = (float)(bot - top);
    return 1;
}

static float sample_ink(int t, float x, float y)
{
    int w = s_gf[t].w, h = s_gf[t].h;
    x -= 0.5f; y -= 0.5f;
    int X = (int)floorf(x), Y = (int)floorf(y);
    float fx = x - X, fy = y - Y, v = 0;
    for (int j = 0; j < 2; j++)
        for (int i = 0; i < 2; i++) {
            int xx = X + i, yy = Y + j;
            if (xx < 0 || yy < 0 || xx >= w || yy >= h)
                continue;
            v += s_gf[t].ink[yy * w + xx] * (i ? fx : 1 - fx) * (j ? fy : 1 - fy);
        }
    return v;
}

/* One character of the typeface (coverage into m when m is given); its advance. */
static float ttf_char(uint8_t *m, int w, int h, const stbtt_fontinfo *f, int cp, float px, float sx,
                      float x, float base)
{
    float scale = stbtt_ScaleForPixelHeight(f, px);
    int adv, lsb, x1, y1, x2, y2;
    stbtt_GetCodepointHMetrics(f, cp, &adv, &lsb);
    stbtt_GetCodepointBitmapBox(f, cp, scale * sx, scale, &x1, &y1, &x2, &y2);
    int gw = x2 - x1, gh = y2 - y1;
    static uint8_t g[160 * 160];
    if (m && gw > 0 && gh > 0 && gw <= 160 && gh <= 160) {
        stbtt_MakeCodepointBitmap(f, g, gw, gh, gw, scale * sx, scale, cp);
        for (int yy = 0; yy < gh; yy++)
            for (int xx = 0; xx < gw; xx++) {
                int X = (int)floorf(x) + x1 + xx, Y = (int)lroundf(base) + y1 + yy;
                if (X >= 0 && Y >= 0 && X < w && Y < h && g[yy * gw + xx] > m[Y * w + X])
                    m[Y * w + X] = g[yy * gw + xx];
            }
    }
    return adv * scale * sx;
}

static float game_text(uint8_t *m, int w, int h, const char *s, float px, float sx,
                       float x, float base)
{
    float k = px * 0.66f / s_cap_h, x0 = x;     /* output pixels a texel */
    for (; *s; s++) {
        float c[4];
        if (*s == '&') {                         /* the game's letters have none (its cell is a quote mark) */
            x += ttf_char(m, w, h, &s_font_body, '&', px * 0.92f, sx, x, base);
            continue;
        }
        int t = glyph((unsigned char)*s, c);
        if (t < 0)
            continue;
        float cw = c[2] - c[0], ch = c[3] - c[1];
        float top = base - (s_cap_top + s_cap_h) * k;
        if (m && *s != ' ')                      /* (its cell is not blank) */
            for (int Y = (int)floorf(top); Y < (int)ceilf(top + ch * k); Y++)
                for (int X = (int)floorf(x); X < (int)ceilf(x + cw * k * sx); X++) {
                    if (X < 0 || Y < 0 || X >= w || Y >= h)
                        continue;
                    float u = c[0] + (X + 0.5f - x) / (k * sx), v = c[1] + (Y + 0.5f - top) / k;
                    if (u < c[0] || u > c[2] || v < c[1] || v > c[3])
                        continue;
                    float a = sample_ink(t, u, v);
                    uint8_t b = (uint8_t)lroundf(fminf(a, 1.0f) * 255);
                    if (b > m[Y * w + X])
                        m[Y * w + X] = b;
                }
        x += cw * k * sx * (*s == ' ' ? 0.5f : 1.0f);
    }
    return x - x0;
}

/* Coverage of text into m (w x h, 0-255, max), the baseline at (x, base),
 * px tall, squeezed sideways by sx. Returns the advance. */
static float draw_text(uint8_t *m, int w, int h, const stbtt_fontinfo *f,
                       const char *s, float px, float sx, float x, float base)
{
    if (s_game_font)
        return game_text(m, w, h, s, px, sx, x, base);
    float scale = stbtt_ScaleForPixelHeight(f, px), x0 = x;
    uint8_t g[160 * 160];
    for (; *s; s++) {
        int adv, lsb, x1, y1, x2, y2, cp = (unsigned char)*s;
        float fx = x - floorf(x);
        stbtt_GetCodepointHMetrics(f, cp, &adv, &lsb);
        stbtt_GetCodepointBitmapBoxSubpixel(f, cp, scale * sx, scale, fx, 0,
                                            &x1, &y1, &x2, &y2);
        int gw = x2 - x1, gh = y2 - y1;
        if (gw > 0 && gh > 0 && gw <= 160 && gh <= 160) {
            stbtt_MakeCodepointBitmapSubpixel(f, g, gw, gh, gw, scale * sx, scale,
                                              fx, 0, cp);
            for (int yy = 0; yy < gh; yy++)
                for (int xx = 0; xx < gw; xx++) {
                    int X = (int)floorf(x) + x1 + xx, Y = (int)lroundf(base) + y1 + yy;
                    if (X >= 0 && Y >= 0 && X < w && Y < h && g[yy * gw + xx] > m[Y * w + X])
                        m[Y * w + X] = g[yy * gw + xx];
                }
        }
        x += adv * scale * sx;
        if (s[1])
            x += stbtt_GetCodepointKernAdvance(f, cp, (unsigned char)s[1]) * scale * sx;
    }
    return x - x0;
}

static float text_width(const stbtt_fontinfo *f, const char *s, float px, float sx)
{
    if (s_game_font)
        return game_text(NULL, 0, 0, s, px, sx, 0, 0);
    float scale = stbtt_ScaleForPixelHeight(f, px), x = 0;
    for (; *s; s++) {
        int adv, lsb;
        stbtt_GetCodepointHMetrics(f, (unsigned char)*s, &adv, &lsb);
        x += adv * scale * sx;
        if (s[1])
            x += stbtt_GetCodepointKernAdvance(f, (unsigned char)*s, (unsigned char)s[1]) * scale * sx;
    }
    return x;
}

/* The tab's word in the plates' style: a tall capital, the rest smaller, a
 * little condensed, fitted into box (w x h) centred on (cx, cy). */
static void tab_word(uint8_t *m, int mw, int mh, float cx, float cy, float bw, float bh)
{
    static uint8_t tmp[400 * 160];
    const float cap = 58, rest = 40, sx = 0.82f, base = 110;
    memset(tmp, 0, sizeof tmp);
    {   /* always the typeface: it looks like the other plates' lettering */
        int game = s_game_font;
        s_game_font = 0;
        float a = draw_text(tmp, 400, 160, &s_font_tab, "S", cap, sx, 6, base);
        draw_text(tmp, 400, 160, &s_font_tab, "ettings", rest, sx, 6 + a - 2, base);
        s_game_font = game;
    }
    int x0 = 400, y0 = 160, x1 = -1, y1 = -1;
    for (int y = 0; y < 160; y++)
        for (int x = 0; x < 400; x++)
            if (tmp[y * 400 + x]) {
                if (x < x0) x0 = x;
                if (y < y0) y0 = y;
                if (x > x1) x1 = x;
                if (y > y1) y1 = y;
            }
    if (x1 < 0)
        return;
    float tw = (float)(x1 - x0 + 1), th = (float)(y1 - y0 + 1);
    float k = fminf(fminf(bw / tw, bh / th), 1.0f);
    int ow = (int)(tw * k), oh = (int)(th * k);
    int ox = (int)lroundf(cx - ow / 2.0f), oy = (int)lroundf(cy - oh / 2.0f);
    /* Area-average resample into place. */
    for (int y = 0; y < oh; y++)
        for (int x = 0; x < ow; x++) {
            float sx0 = x0 + x / k, sy0 = y0 + y / k, sum = 0, n = 0;
            for (float yy = sy0; yy < sy0 + 1 / k; yy += 0.5f)
                for (float xx = sx0; xx < sx0 + 1 / k; xx += 0.5f) {
                    int X = (int)xx, Y = (int)yy;
                    if (X <= x1 && Y <= y1) { sum += tmp[Y * 400 + X]; n++; }
                }
            int X = ox + x, Y = oy + y;
            if (n && X >= 0 && Y >= 0 && X < mw && Y < mh)
                m[Y * mw + X] = (uint8_t)(sum / n);
        }
}

static inline uint32_t mix(uint32_t c, uint32_t rgb, float a)
{
    int r = c >> 16 & 255, g = c >> 8 & 255, b = c & 255;
    r += (int)lroundf(((int)(rgb >> 16 & 255) - r) * a);
    g += (int)lroundf(((int)(rgb >> 8 & 255) - g) * a);
    b += (int)lroundf(((int)(rgb & 255) - b) * a);
    return (c & 0xFF000000u) | (uint32_t)r << 16 | (uint32_t)g << 8 | (uint32_t)b;
}

static inline float luma(uint32_t c)
{
    return ((c >> 16 & 255) + (c >> 8 & 255) + (c & 255)) / 3.0f;
}

/* The Exit plate with its word taken off -- each row of the metal face
 * filled from that row's bare metal -- and "Settings" put on, dark with a
 * light edge below, as the game's are. */
static void make_plate(uint32_t *px, int w, int h)
{
    const int fx0 = w * 20 / 128, fx1 = w * 109 / 128;
    for (int y = 3 * h / 64; y < 50 * h / 64; y++) {
        float l[256], sorted[256];
        int n = 0;
        for (int x = fx0; x < fx1; x++)
            sorted[n++] = l[x] = luma(px[y * w + x]);
        for (int i = 1; i < n; i++)            /* insertion sort, n < 256 */
            for (int j = i; j > 0 && sorted[j - 1] > sorted[j]; j--) {
                float t = sorted[j]; sorted[j] = sorted[j - 1]; sorted[j - 1] = t;
            }
        float bg = sorted[n * 8 / 10];
        uint8_t txt[256] = { 0 };
        for (int x = fx0; x < fx1; x++)
            if (l[x] < bg - 35)
                txt[x] = txt[x - 1 > fx0 ? x - 1 : x] = txt[x + 1 < fx1 ? x + 1 : x] = 1;
        int r = 0, g = 0, b = 0, k = 0;
        for (int x = fx0; x < fx1; x++)
            if (!txt[x] && l[x] >= bg - 20) {
                uint32_t c = px[y * w + x];
                r += c >> 16 & 255; g += c >> 8 & 255; b += c & 255; k++;
            }
        if (!k)
            continue;
        uint32_t fill = (uint32_t)(r / k) << 16 | (uint32_t)(g / k) << 8 | (uint32_t)(b / k);
        for (int x = fx0; x < fx1; x++)
            if (txt[x])
                px[y * w + x] = (px[y * w + x] & 0xFF000000u) | fill;
    }
    static uint8_t m[256 * 128];
    memset(m, 0, sizeof m);
    tab_word(m, w, h, w / 2.0f + 1, h * 25.0f / 64, w * 88.0f / 128, h * 38.0f / 64);
    for (int y = h - 1; y > 0; y--)            /* the light edge, then the word */
        for (int x = w - 1; x > 0; x--) {
            float a = m[(y - 1) * w + x - 1] / 255.0f * 0.5f;
            if (a > 0)
                px[y * w + x] = mix(px[y * w + x], 0xF5F5FA, a);
        }
    for (int i = 0; i < w * h; i++)
        if (m[i])
            px[i] = mix(px[i], 0x14141A, m[i] / 255.0f);
}

static void box_blur(float *v, int w, int h, int r)
{
    float *t = malloc(sizeof *t * w * h);
    for (int pass = 0; pass < 3; pass++) {
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++) {
                float s = 0; int n = 0;
                for (int k = -r; k <= r; k++)
                    if (x + k >= 0 && x + k < w) { s += v[y * w + x + k]; n++; }
                t[y * w + x] = s / n;
            }
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++) {
                float s = 0; int n = 0;
                for (int k = -r; k <= r; k++)
                    if (y + k >= 0 && y + k < h) { s += t[(y + k) * w + x]; n++; }
                v[y * w + x] = s / n;
            }
    }
    free(t);
}

/* The chosen tab's glowing word: near white, in a violet haze. */
static void make_glow(uint32_t *px, int w, int h)
{
    static uint8_t m[256 * 128];
    float *halo = malloc(sizeof *halo * w * h);
    memset(m, 0, sizeof m);
    /* where the plate has its dark word (make_plate), so none of it shows round the glow */
    tab_word(m, w, h, w / 2.0f + 1, h * 25.0f / 64, w * 88.0f / 128, h * 38.0f / 64);
    for (int i = 0; i < w * h; i++)
        halo[i] = m[i] / 255.0f;
    box_blur(halo, w, h, 2);
    /* as the game's: white letters in a strong violet glow (measured from its Exit) */
    for (int i = 0; i < w * h; i++) {
        float core = m[i] / 255.0f, a = fminf(halo[i] * 3.5f, 1.0f) * 0.95f;
        if (core > a) a = core;
        uint32_t c = mix(0x8986F4, 0xFFFFFF, fminf(core * 1.4f, 1.0f));
        px[i] = (uint32_t)lroundf(a * 255) << 24 | (c & 0xFFFFFF);
    }
    free(halo);
}

/* ---------- the page ---------- */

static struct {
    int page, focused, row, vals[S_COUNT];   /* page: 0 Settings, 1 Exit */
} s_drawn = { -1, -1, -1, { 0 } };

static void page_fill(uint32_t *px, float x, float y, float w, float h,
                      uint32_t rgb, float a0, float a1)
{
    const float k = (float)PAGE_TW / PAGE_W;
    int X0 = (int)(x * k), Y0 = (int)(y * k), X1 = (int)((x + w) * k), Y1 = (int)((y + h) * k);
    for (int Y = Y0; Y < Y1 && Y < PAGE_TH; Y++)
        for (int X = X0; X < X1 && X < PAGE_TW; X++) {
            float t = (float)(X - X0) / (X1 - X0), a = a0 + (a1 - a0) * t;
            uint32_t c = px[Y * PAGE_TW + X];
            float ca = (c >> 24) / 255.0f, oa = a + ca * (1 - a);
            uint32_t rgbmix = oa > 0 ? mix(c, rgb, a / oa) : rgb;
            px[Y * PAGE_TW + X] = (uint32_t)lroundf(oa * 255) << 24 | (rgbmix & 0xFFFFFF);
        }
}

static void page_text(uint32_t *px, uint8_t *m, const char *s, float x, float base,
                      float size, uint32_t rgb, int align)
{
    const float k = (float)PAGE_TW / PAGE_W;
    float w = text_width(&s_font_body, s, size * k, 1.0f);
    float room = (align == 2 ? x : align == 1 ? 2 * fminf(x, PAGE_W - x) : PAGE_W - x) - 4;
    if (w > room * k) {                         /* narrowed to fit the page */
        size *= room * k / w;
        w = room * k;
    }
    float X = x * k - (align == 1 ? w / 2 : align == 2 ? w : 0);
    memset(m, 0, PAGE_TW * PAGE_TH);
    draw_text(m, PAGE_TW, PAGE_TH, &s_font_body, s, size * k, 1.0f, X, base * k);
    /* A soft dark edge under the letters, as the game's text has. */
    for (int Y = 1; Y < PAGE_TH; Y++)
        for (int Xp = 1; Xp < PAGE_TW; Xp++) {
            uint8_t v = m[(Y - 1) * PAGE_TW + Xp - 1];
            if (v) {
                uint32_t c = px[Y * PAGE_TW + Xp];
                float ca = (c >> 24) / 255.0f, a = v / 255.0f * 0.6f;
                float oa = a + ca * (1 - a);
                uint32_t mm = oa > 0 ? mix(c, 0x000000, a / oa) : 0;
                px[Y * PAGE_TW + Xp] = (uint32_t)lroundf(oa * 255) << 24 | (mm & 0xFFFFFF);
            }
        }
    for (int i = 0; i < PAGE_TW * PAGE_TH; i++)
        if (m[i]) {
            uint32_t c = px[i];
            float ca = (c >> 24) / 255.0f, a = m[i] / 255.0f, oa = a + ca * (1 - a);
            uint32_t mm = mix(c, rgb, a / oa);
            px[i] = (uint32_t)lroundf(oa * 255) << 24 | (mm & 0xFFFFFF);
        }
}

#define ROW_Y0 50
#define ROW_H  27
#define SET_ROW_H 24                    /* the settings' rows: eight in the room of seven */
#define SET_HELP_Y (ROW_Y0 + 217)       /* their help, clear of the prompts below */
#define STRIP_Y 6                 /* the categories' strip */
#define STRIP_H 26
#define STRIP_X0 14               /* where its first word starts, and the gap */
#define STRIP_GAP 26

/* The strip's words: where each starts and ends, in page units. */
static float s_cat_x[CATS][2];

/* row -1 is the strip. */
/* ---------- Controls ---------- */

/* The list: a heading, then CTL_SHOWN rows of it scrolled to keep the chosen
 * one in sight; each an action, its two keys or mouse buttons and its
 * controller button. A (a click) waits for the new one, X (Delete) clears. */
#define CTL_Y0 74
#define CTL_SHOWN 6
#define CTL_NAME_X 14
static const float s_ctl_cx[3] = { 252, 343, 420 }, s_ctl_w[3] = { 88, 88, 56 };
static int s_ctl_col, s_ctl_top;

static void ctl_scroll(int row)
{
    if (row < 0)
        return;
    if (row < s_ctl_top)
        s_ctl_top = row;
    if (row >= s_ctl_top + CTL_SHOWN)
        s_ctl_top = row - CTL_SHOWN + 1;
}

/* Text no wider than maxw (page units), smaller if it would be. */
static void page_text_fit(uint32_t *px, uint8_t *m, const char *t, float x, float base, float size,
                          uint32_t rgb, int align, float maxw)
{
    const float k = (float)PAGE_TW / PAGE_W;
    float w = text_width(&s_font_body, t, size * k, 1.0f) / k;
    if (w > maxw)
        size *= maxw / w;
    page_text(px, m, t, x, base, size, rgb, align);
}

static void draw_controls(uint32_t *px, uint8_t *m, int focused, int row)
{
    page_text(px, m, "Keyboard / Mouse", (s_ctl_cx[0] + s_ctl_cx[1]) / 2, CTL_Y0 - 8, 14, 0x8C8C96, 1);
    page_text(px, m, "Controller", s_ctl_cx[2], CTL_Y0 - 8, 14, 0x8C8C96, 1);
    page_fill(px, 4, CTL_Y0 - 3, 440, 1, 0xB4B9D2, 0.35f, 0.0f);
    int capturing = ngb_bind_capturing();
    for (int i = 0; i < CTL_SHOWN && s_ctl_top + i < CTL_ROWS; i++) {
        int r = s_ctl_top + i, sel = focused && r == row;
        float y = CTL_Y0 + i * ROW_H;
        if (sel)
            page_fill(px, 0, y + 2, 10, ROW_H - 4, 0xE6E4FF, 0.95f, 0.85f);
        if (CTL_TOGGLE(r)) {                           /* the camera's On/Off rows */
            if (sel)
                page_fill(px, 10, y + 2, 440, ROW_H - 4, 0x6E64DC, 0.85f, 0.05f);
            page_text_fit(px, m, s_ctl_toggle_name[r - CTL_INVX], CTL_NAME_X + 6, y + 19, 18,
                          sel ? 0xF5EB5A : focused ? 0xE6E6EB : 0xA0A0A8, 0, 260);
            page_text(px, m, ctl_toggle_get(r) ? "On" : "Off", s_ctl_cx[2], y + 18, 15,
                      sel ? 0xFFFFFF : 0xC8C8D0, 1);
            continue;
        }
        if (r == CTL_MOUSE) {
            if (sel)
                page_fill(px, 10, y + 2, 440, ROW_H - 4, 0x6E64DC, 0.85f, 0.05f);
            page_text_fit(px, m, "Camera Sensitivity", CTL_NAME_X + 6, y + 19, 18,
                          sel ? 0xF5EB5A : focused ? 0xE6E6EB : 0xA0A0A8, 0, 260);
            char pct[16];
            snprintf(pct, sizeof pct, "%s%d%%%s", sel ? "< " : "", ngb_bind_mouse_sensitivity(), sel ? " >" : "");
            page_text(px, m, pct, s_ctl_cx[2], y + 18, 15, sel ? 0xFFFFFF : 0xC8C8D0, 1);
            continue;
        }
        if (r == CTL_DEF) {                            /* Default Controls */
            if (sel)
                page_fill(px, 10, y + 2, 440, ROW_H - 4, 0x6E64DC, 0.85f, 0.05f);
            page_text(px, m, "Default Controls", CTL_NAME_X + 6, y + 19, 19,
                      sel ? 0xF5EB5A : focused ? 0xE6E6EB : 0xA0A0A8, 0);
            continue;
        }
        page_text_fit(px, m, ngb_bind_name(r), CTL_NAME_X + 6, y + 19, 18,
                      sel ? 0xF5EB5A : focused ? 0xE6E6EB : 0xA0A0A8, 0, 186);
        for (int c = 0; c < 3; c++) {
            char name[64];
            int fixed = c == 2 && ngb_bind_pad_fixed(r), on = sel && c == s_ctl_col;
            const char *combo = c == 2 && !ngb_bind_get(r, 2) ? ngb_bind_pad_combo(r) : NULL;   /* the game's own two buttons */
            float cx = s_ctl_cx[c], w = s_ctl_w[c];
            page_fill(px, cx - w / 2, y + 3, w, ROW_H - 6, on ? 0x6E64DC : 0x14141C, on ? 0.9f : 0.55f,
                      on ? 0.9f : 0.55f);
            if (on && capturing)
                snprintf(name, sizeof name, c == 2 ? "Press a button" : "Press a key");
            else if (fixed)
                snprintf(name, sizeof name, r >= A_CAM_UP ? "R Stick" : "L Stick");
            else if (combo)
                snprintf(name, sizeof name, "%s", combo);
            else
                ngb_bind_input_name(ngb_bind_get(r, c), name, sizeof name);
            page_text_fit(px, m, name, cx, y + 18, 15,
                          on && capturing ? 0xF5EB5A : on ? 0xFFFFFF : fixed || combo ? 0x6E6E78 : 0xC8C8D0, 1, w - 6);
        }
    }
    /* how far down the list is: a thin bar at the right */
    float bar_h = CTL_SHOWN * ROW_H, th = bar_h * CTL_SHOWN / CTL_ROWS, ty = bar_h * s_ctl_top / CTL_ROWS;
    page_fill(px, 446, CTL_Y0 + 2, 2, bar_h - 4, 0x50505A, 0.6f, 0.6f);
    page_fill(px, 446, CTL_Y0 + 2 + ty, 2, th - 4, 0xC8C4FF, 0.9f, 0.9f);
    const char *help = NULL;
    if (focused && row >= 0 && capturing)
        help = s_ctl_col == 2 ? "Press the controller's button (wait to cancel)." : "Press a key or mouse button (Esc cancels).";
    else if (focused && row == CTL_DEF)
        help = "Every action as it was.";
    else if (focused && CTL_TOGGLE(row))
        help = s_ctl_toggle_help[row - CTL_INVX];
    else if (focused && row == CTL_MOUSE)
        help = "How fast the right stick and the mouse turn the camera (play only).";
    else if (focused && row >= 0)
        help = s_ctl_col == 2 && ngb_bind_pad_fixed(row) ? ngb_bind_help(row)
             : "A: change   X / Delete: clear   (play only; the menus keep theirs)";
    if (focused && help)
        page_text_fit(px, m, help, 445, CTL_Y0 + CTL_SHOWN * ROW_H + 16, 14, 0xFFFFFF, 2, 440);
}

static void draw_page(int focused, int cat, int row)
{
    static uint32_t px[PAGE_TW * PAGE_TH];
    static uint8_t m[PAGE_TW * PAGE_TH];
    const float k = (float)PAGE_TW / PAGE_W;
    memset(px, 0, sizeof px);

    /* The strip, as the game's tabs have theirs: a dark bar fading out to
     * the right, the chosen word light violet, the others grey. */
    page_fill(px, 4, STRIP_Y, 442, STRIP_H, 0x14141C, 0.8f, 0.8f);   /* one colour behind all the words */
    page_fill(px, 4, STRIP_Y + STRIP_H - 1.5f, 442, 1.5f, 0xB4B9D2, 0.6f, 0.6f);
    /* The words at 17, or smaller until they fit with a fair gap between them (six do not at 17). */
    float x = STRIP_X0, words = 0, gap = STRIP_GAP, fs = 17;
    for (;;) {
        words = 0;
        for (int c = 0; c < CATS; c++)
            words += text_width(&s_font_body, s_cat_name[c], fs * k, 1.0f) / k;
        gap = (440 - 2 * STRIP_X0 - words) / (CATS - 1);
        if (gap >= 16 || fs <= 13)
            break;
        fs -= 1;
    }
    if (gap > STRIP_GAP)
        gap = STRIP_GAP;
    for (int c = 0; c < CATS; c++) {
        float w = text_width(&s_font_body, s_cat_name[c], fs * k, 1.0f) / k;
        int on = c == (cat == CAT_UNL ? CAT_CHEAT : cat), sel = focused && row < 0 && on;
        if (sel)
            page_fill(px, x - 6, STRIP_Y + 3, w + 12, STRIP_H - 6, 0x6E64DC, 0.75f, 0.75f);
        page_text(px, m, s_cat_name[c], x, STRIP_Y + 19, fs,
                  sel ? 0xFFFFFF : on ? 0xC8C4FF : 0x8C8C96, 0);
        s_cat_x[c][0] = x - 6;
        s_cat_x[c][1] = x + w + 6;
        x += w + gap;
    }

    if (cat == CAT_CTL) {
        draw_controls(px, m, focused, row);
        upload(&s_tex[2], px);
        return;
    }
    int n = cat_count(cat);
    for (int i = 0; i < n; i++) {
        int set = s_cat_rows[cat][i];
        float y = ROW_Y0 + i * SET_ROW_H;
        int sel = focused && i == row;
        char val[48];
        if (sel) {
            page_fill(px, 0, y + 2, 10, SET_ROW_H - 4, 0xE6E4FF, 0.95f, 0.85f);
            page_fill(px, 10, y + 2, 440, SET_ROW_H - 4, 0x6E64DC, 0.85f, 0.05f);
        }
        value_text(set, val, sizeof val);
        page_text(px, m, s_label[set], 20, y + 18, 19,
                  sel ? 0xF5EB5A : focused ? 0xE6E6EB : 0xA0A0A8, 0);
        page_text(px, m, val, 350, y + 18, 19, sel ? 0xFFFFFF : 0xB4B4BC, 1);
        if (sel && set == S_UNLOCKS)               /* a page, not a value */
            page_text(px, m, ">", 428, y + 17, 17, 0xFFFFFF, 1);
        else if (sel) {
            page_text(px, m, "<", 272, y + 17, 17, 0xFFFFFF, 1);
            page_text(px, m, ">", 428, y + 17, 17, 0xFFFFFF, 1);
        }
    }
    if (focused && row >= 0 && row < n)
        {   /* a help of two lines: split at its \n */
            const char *h = s_help[s_cat_rows[cat][row]], *nl = strchr(h, '\n');
            char first[160];
            if (nl && (size_t)(nl - h) < sizeof first) {
                memcpy(first, h, (size_t)(nl - h));
                first[nl - h] = 0;
                page_text_fit(px, m, first, 445, SET_HELP_Y, 15, 0xFFFFFF, 2, 440);
                page_text_fit(px, m, nl + 1, 445, SET_HELP_Y + 18, 15, 0xFFFFFF, 2, 440);
            } else {
                page_text_fit(px, m, h, 445, SET_HELP_Y, 15, 0xFFFFFF, 2, 440);
            }
        }

    upload(&s_tex[2], px);
}


/* ---------- RECOMP SETTINGS' pictures ---------- */

/* While the main menu's RECOMP SETTINGS has the Options menu (src/pc_menu.c)
 * its pictures say so: the header ("OPTIONS", sprite 0x23D) reads SETTINGS
 * and the category words (sprites 0x248-0x24D, slices of one texture) are
 * Graphics Settings, Display Settings, Sound Settings, Gameplay, Cheats and Controls. Each is a texture of
 * ours put in the game's texture's place, the words drawn in the game's
 * pictures' style -- the size, the condensing and the colour of each row of
 * the letters measured from them, the shadow under them -- Liberation Serif
 * (SIL Open Font License). Put back once the main menu is. */
int ngb_recomp_menu_active(void);   /* src/pc_menu.c */
void ngb_recomp_menu_seen(void);
static uint32_t sprite_def(uint32_t id);

#define RP_HEADER 0x23D
#define RP_LABEL0 0x248
#define RP_LABELS 6
#define RP_WORDS 6
static const char *const s_rp_word[RP_WORDS] = { "GRAPHICS SETTINGS", "DISPLAY SETTINGS", "SOUND SETTINGS",
                                                 "GAMEPLAY", "CHEATS", "CONTROLS" };

static struct {
    GuestTex t;
    int made;
    uint32_t orig;                /* the game's header pointer in the texture table */
} s_rp[2];                        /* 0 the header, 1 the words */
static float s_rp_uv[RP_LABELS][2], s_rp_orig_uv[RP_LABELS][2];
static stbtt_fontinfo s_font_bold;

/* The letters of the picture px (w x h) between columns x0 and x1: the
 * rows of their capitals (bright texels), the colour of each row, and their
 * width. */
typedef struct { int top, bot, left, right; uint32_t row[128]; } Style;

static void measure(const uint32_t *px, int w, int h, int x0, int x1, Style *st)
{
    st->top = h; st->bot = -1; st->left = w; st->right = -1;
    for (int y = 0; y < h && y < 128; y++) {
        int r = 0, g = 0, b = 0, n = 0;
        for (int x = x0; x < x1; x++) {
            uint32_t c = px[y * w + x];
            if ((c >> 24) > 200 && luma(c) > 110) {
                r += c >> 16 & 255; g += c >> 8 & 255; b += c & 255; n++;
                if (y < st->top) st->top = y;
                if (y > st->bot) st->bot = y;
                if (x < st->left) st->left = x;
                if (x > st->right) st->right = x;
            }
        }
        st->row[y] = n ? (uint32_t)(r / n) << 16 | (uint32_t)(g / n) << 8 | (uint32_t)(b / n) : 0;
    }
    /* Rows with few letter texels take their neighbours' colour. */
    for (int y = st->top; y <= st->bot && y < 128; y++)
        if (!st->row[y] && y > st->top)
            st->row[y] = st->row[y - 1];
}

/* The font size whose capital H is cap texels tall. */
static float cap_px(const stbtt_fontinfo *f, float cap)
{
    int x0, y0, x1, y1;
    stbtt_GetCodepointBox(f, 'H', &x0, &y0, &x1, &y1);
    return cap / ((y1 - y0) * stbtt_ScaleForPixelHeight(f, 1.0f));
}

/* word into px (w x h, cleared where it goes) at x, in st's rows and colours:
 * a soft dark shadow down and right, then the letters. Returns its width. */
static int styled_word(uint32_t *px, int w, int h, const stbtt_fontinfo *f, const char *word,
                       float sx, int x, const Style *st, int shadow)
{
    float cap = (float)(st->bot - st->top + 1), size = cap_px(f, cap);
    int game = s_game_font;
    s_game_font = 0;
    int ww = (int)ceilf(text_width(f, word, size, sx)) + 2;
    uint8_t *m = calloc((size_t)w * h, 1);
    draw_text(m, w, h, f, word, size, sx, (float)x, (float)st->bot + 1);
    s_game_font = game;
    float *sh = malloc(sizeof *sh * w * h);
    for (int y = 0; y < h; y++)
        for (int X = 0; X < w; X++) {
            int sx0 = X - shadow, sy0 = y - shadow;
            sh[y * w + X] = sx0 >= 0 && sy0 >= 0 ? m[sy0 * w + sx0] / 255.0f : 0;
        }
    box_blur(sh, w, h, shadow > 1 ? 2 : 1);
    for (int y = 0; y < h; y++)
        for (int X = x - 2; X < x + ww + shadow + 3 && X < w; X++) {
            if (X < 0)
                continue;
            float a = m[y * w + X] / 255.0f, s2 = fminf(sh[y * w + X] * 1.6f, 1.0f) * 0.9f;
            int ry = y < st->top ? st->top : y > st->bot ? st->bot : y;
            uint32_t col = st->row[ry] ? st->row[ry] : 0xE0E0E0;
            float oa = a + s2 * (1 - a);
            uint32_t c = oa > 0 ? mix(0x000000, col, a / oa) : 0;
            px[y * w + X] = (uint32_t)lroundf(oa * 255) << 24 | (c & 0xFFFFFF);
        }
    free(sh);
    free(m);
    return ww;
}

/* The squeeze that makes the font's word as wide as the picture's. */
static float squeeze(const stbtt_fontinfo *f, const char *word, const Style *st)
{
    float size = cap_px(f, (float)(st->bot - st->top + 1));
    int game = s_game_font;
    s_game_font = 0;
    float natural = text_width(f, word, size, 1.0f);
    s_game_font = game;
    float k = natural > 0 ? (st->right - st->left + 1) / natural : 1.0f;
    return k < 0.6f ? 0.6f : k > 1.2f ? 1.2f : k;
}

static int rp_make_header(uint32_t hdr, uint32_t pal)
{
    int w, h;
    uint32_t *ref = read_texture(hdr, pal, &w, &h);
    if (!ref || h > 128)
        return free(ref), 0;
    Style st;
    measure(ref, w, h, 0, w, &st);
    if (st.bot <= st.top)
        return free(ref), 0;
    float sx = squeeze(&s_font_bold, "OPTIONS", &st);
    float size = cap_px(&s_font_bold, (float)(st.bot - st.top + 1));
    int game = s_game_font;
    s_game_font = 0;
    float tw = text_width(&s_font_bold, "SETTINGS", size, sx);
    s_game_font = game;
    float room = (st.right - st.left + 1) * 1.04f;    /* OPTIONS' width: clear of the ornaments */
    if (tw > room)
        sx *= room / tw, tw = room;
    int x = (int)lroundf((st.left + st.right) / 2.0f - tw / 2);
    uint32_t *px = calloc((size_t)w * h, sizeof *px);
    styled_word(px, w, h, &s_font_bold, "SETTINGS", sx, x, &st, 2);
    int ok = make_texture(&s_rp[0].t, w, h, MEM32(hdr + 0xC) & 0xFF);
    if (ok)
        upload(&s_rp[0].t, px);
    free(px);
    free(ref);
    return ok;
}

static int rp_make_words(uint32_t hdr, uint32_t pal)
{
    int w, h;
    uint32_t *ref = read_texture(hdr, pal, &w, &h);
    uint32_t dexit = sprite_def(RP_LABEL0 + 5);
    if (!ref || h > 128 || !dexit)
        return free(ref), 0;
    Style st;                                          /* from the game's EXIT */
    measure(ref, w, h, (int)(MEMF(dexit) * w), (int)(MEMF(dexit + 8) * w), &st);
    if (st.bot <= st.top)
        return free(ref), 0;
    float sx = squeeze(&s_font_body, "EXIT", &st);
    uint32_t *px = calloc((size_t)w * h, sizeof *px);
    int x = 4;
    for (int i = 0; i < RP_WORDS; i++) {
        int ww = styled_word(px, w, h, &s_font_body, s_rp_word[i], sx, x, &st, 1);
        s_rp_uv[i][0] = (float)x / w;
        s_rp_uv[i][1] = (float)(x + ww + 2) / w;
        x += ww + 12;
    }
    for (int i = RP_WORDS; i < RP_LABELS; i++) {       /* the game's other: nothing */
        s_rp_uv[i][0] = (float)(w - 6) / w;
        s_rp_uv[i][1] = (float)(w - 2) / w;
    }
    int ok = x < w - 8 && make_texture(&s_rp[1].t, w, h, MEM32(hdr + 0xC) & 0xFF);
    if (ok)
        upload(&s_rp[1].t, px);
    free(px);
    free(ref);
    return ok;
}

/* The category column (sub_0021B380) draws six rows, each its plates then
 * its word: ours are five, so what comes after our last word (0x24C) until
 * the game's sixth (0x24D) is not drawn. */
static int rp_skip(uint32_t id)
{
    static int after;
    if (RP_WORDS >= RP_LABELS)                        /* six: the game's six rows, all drawn */
        return 0;
    if (id == RP_LABEL0 + RP_WORDS - 1) {
        after = 1;
        return 0;
    }
    if (!after || id < 0x242 || id > RP_LABEL0 + 5)
        return 0;
    if (id == RP_LABEL0 + 5)
        after = 0;
    return 1;
}

/* Each sprite the game queues while RECOMP SETTINGS is open: ours in place. */
static void rp_check(uint32_t id)
{
    if (id != RP_HEADER && (id < RP_LABEL0 || id >= RP_LABEL0 + RP_LABELS))
        return;
    ngb_recomp_menu_seen();
    if (!s_font_bold.data && !stbtt_InitFont(&s_font_bold, ngb_font_label, 0))
        return;
    if (!s_font_body.data && !stbtt_InitFont(&s_font_body, ngb_font_body, 0))
        return;
    int k = id == RP_HEADER ? 0 : 1;
    uint32_t def = sprite_def(id);
    if (!def)
        return;
    uint32_t entry = 0x8DBB28 + MEM32(def + 0x10) * 8;
    if (s_rp[k].made < 0)
        return;                                        /* could not: the game's */
    if (!s_rp[k].made) {
        int ok = k == 0 ? rp_make_header(MEM32(entry), MEM32(entry + 4))
                        : rp_make_words(MEM32(entry), MEM32(entry + 4));
        s_rp[k].made = ok ? 1 : -1;
        if (!ok) {
            fprintf(stderr, "[RECOMP] the Options %s picture is not one read here: the game's\n",
                    k ? "category" : "header");
            return;
        }
    }
    if (MEM32(entry) != s_rp[k].t.hdr) {
        s_rp[k].orig = MEM32(entry);
        MEM32(entry) = s_rp[k].t.hdr;
    }
    if (k == 1)
        for (int i = 0; i < RP_LABELS; i++) {
            uint32_t d = sprite_def(RP_LABEL0 + i);
            if (d && MEMF(d) != s_rp_uv[i][0]) {
                s_rp_orig_uv[i][0] = MEMF(d);
                s_rp_orig_uv[i][1] = MEMF(d + 8);
                MEMF(d) = s_rp_uv[i][0];
                MEMF(d + 8) = s_rp_uv[i][1];
            }
        }
}

void ngb_recomp_pictures_off(void)
{
    for (int k = 0; k < 2; k++) {
        uint32_t def = sprite_def(k ? RP_LABEL0 : RP_HEADER);
        if (s_rp[k].made <= 0 || !def)
            continue;
        uint32_t entry = 0x8DBB28 + MEM32(def + 0x10) * 8;
        if (MEM32(entry) == s_rp[k].t.hdr)
            MEM32(entry) = s_rp[k].orig;
    }
    for (int i = 0; i < RP_LABELS; i++) {
        uint32_t d = sprite_def(RP_LABEL0 + i);
        if (d && MEMF(d) == s_rp_uv[i][0] && s_rp[1].made > 0) {
            MEMF(d) = s_rp_orig_uv[i][0];
            MEMF(d + 8) = s_rp_orig_uv[i][1];
        }
    }
}

/* ---------- keyboard prompts ---------- */

/* While the keyboard and mouse are in use (xboxrecomp's
 * xbox_InputKeyboardActive), the game's button icons -- one 128x32 texture,
 * X Y B A L R in 21x23 cells (sprites 5-10) -- are keys and mouse buttons
 * instead: Xelu's dark keyboard and mouse prompts (CC0, assets/prompts/keys,
 * embedded), drawn at KS times the texture's size. In play the keys and
 * buttons bound to the actions (src/bindings.c); in the menus the mouse
 * buttons (choose, back), K, Space, Shift and Q. A key the set has no
 * picture of: its blank key with the key's name. */
int xbox_InputKeyboardActive(void);
int ngb_menu_up(void);

#include <SDL_scancode.h>
#include "stb_image.h"            /* the decoder is the renderer's (texpack.c) */
extern const struct { const char *name; const unsigned char *data; const unsigned int *len; } ngb_key_pngs[];
extern const int ngb_key_png_count;

#define KC_W 128
#define KC_H 32
#define KC_CELL_W 21
#define KC_CELL_H 23
#define KS 4                      /* the icons drawn at 4x the game's texture */

/* The picture's name for an input (src/bindings.h), or NULL: none in the set. */
static const char *key_picture(int in)
{
    static char name[40];
    if (in & BIND_MOUSE) {
        int b = in & 0xFF;
        return b == 1 ? "Mouse_Left_Key_Dark" : b == 2 ? "Mouse_Middle_Key_Dark" : b == 3 ? "Mouse_Right_Key_Dark"
                                                                                       : "Mouse_Simple_Key_Dark";
    }
    if (in >= SDL_SCANCODE_A && in <= SDL_SCANCODE_Z) {
        snprintf(name, sizeof name, "%c_Key_Dark", 'A' + (in - SDL_SCANCODE_A));
        return name;
    }
    if (in >= SDL_SCANCODE_1 && in <= SDL_SCANCODE_0) {
        snprintf(name, sizeof name, "%d_Key_Dark", in == SDL_SCANCODE_0 ? 0 : 1 + (in - SDL_SCANCODE_1));
        return name;
    }
    if (in >= SDL_SCANCODE_KP_1 && in <= SDL_SCANCODE_KP_0) {   /* (no keypad set: the digit) */
        snprintf(name, sizeof name, "%d_Key_Dark", in == SDL_SCANCODE_KP_0 ? 0 : 1 + (in - SDL_SCANCODE_KP_1));
        return name;
    }
    if (in >= SDL_SCANCODE_F1 && in <= SDL_SCANCODE_F12) {
        snprintf(name, sizeof name, "F%d_Key_Dark", 1 + (in - SDL_SCANCODE_F1));
        return name;
    }
    switch (in) {
    case SDL_SCANCODE_SPACE:        return "Space_Key_Dark";
    case SDL_SCANCODE_LSHIFT: case SDL_SCANCODE_RSHIFT: return "Shift_Key_Dark";
    case SDL_SCANCODE_LCTRL: case SDL_SCANCODE_RCTRL:   return "Ctrl_Key_Dark";
    case SDL_SCANCODE_LALT: case SDL_SCANCODE_RALT:     return "Alt_Key_Dark";
    case SDL_SCANCODE_TAB:          return "Tab_Key_Dark";
    case SDL_SCANCODE_ESCAPE:       return "Esc_Key_Dark";
    case SDL_SCANCODE_RETURN: case SDL_SCANCODE_KP_ENTER: return "Enter_Key_Dark";
    case SDL_SCANCODE_BACKSPACE:    return "Backspace_Key_Dark";
    case SDL_SCANCODE_CAPSLOCK:     return "Caps_Lock_Key_Dark";
    case SDL_SCANCODE_DELETE:       return "Del_Key_Dark";
    case SDL_SCANCODE_INSERT:       return "Insert_Key_Dark";
    case SDL_SCANCODE_HOME:         return "Home_Key_Dark";
    case SDL_SCANCODE_END:          return "End_Key_Dark";
    case SDL_SCANCODE_PAGEUP:       return "Page_Up_Key_Dark";
    case SDL_SCANCODE_PAGEDOWN:     return "Page_Down_Key_Dark";
    case SDL_SCANCODE_UP:           return "Arrow_Up_Key_Dark";
    case SDL_SCANCODE_DOWN:         return "Arrow_Down_Key_Dark";
    case SDL_SCANCODE_LEFT:         return "Arrow_Left_Key_Dark";
    case SDL_SCANCODE_RIGHT:        return "Arrow_Right_Key_Dark";
    case SDL_SCANCODE_MINUS: case SDL_SCANCODE_KP_MINUS: return "Minus_Key_Dark";
    case SDL_SCANCODE_EQUALS: case SDL_SCANCODE_KP_PLUS: return "Plus_Key_Dark";
    case SDL_SCANCODE_KP_MULTIPLY:  return "Asterisk_Key_Dark";
    case SDL_SCANCODE_LEFTBRACKET:  return "Bracket_Left_Key_Dark";
    case SDL_SCANCODE_RIGHTBRACKET: return "Bracket_Right_Key_Dark";
    case SDL_SCANCODE_SEMICOLON:    return "Semicolon_Key_Dark";
    case SDL_SCANCODE_APOSTROPHE:   return "Quote_Key_Dark";
    case SDL_SCANCODE_SLASH: case SDL_SCANCODE_KP_DIVIDE: return "Slash_Key_Dark";
    case SDL_SCANCODE_GRAVE:        return "Tilda_Key_Dark";
    case SDL_SCANCODE_COMMA:        return "Mark_Left_Key_Dark";
    case SDL_SCANCODE_PERIOD:       return "Mark_Right_Key_Dark";
    case SDL_SCANCODE_NUMLOCKCLEAR: return "Num_Lock_Key_Dark";
    case SDL_SCANCODE_PRINTSCREEN:  return "Print_Screen_Key_Dark";
    case SDL_SCANCODE_LGUI: case SDL_SCANCODE_RGUI: return "Win_Key_Dark";
    default:                        return NULL;
    }
}

static uint8_t *key_png(const char *name, int *w, int *h)
{
    for (int i = 0; name && i < ngb_key_png_count; i++)
        if (!strcmp(ngb_key_pngs[i].name, name)) {
            int n;
            return stbi_load_from_memory(ngb_key_pngs[i].data, (int)*ngb_key_pngs[i].len, w, h, &n, 4);
        }
    return NULL;
}

/* The input's picture into px (pw wide) over the box (x0, y0)-(x1, y1), square, centred, box filtered
 * (straight alpha, over what is there); a key the set lacks, its blank key with its name. */
static void key_icon(uint32_t *px, int pw, int ph, float x0, float y0, float x1, float y1, int in)
{
    int iw, ih;
    const char *name = key_picture(in);
    uint8_t *img = key_png(name, &iw, &ih);
    int blank = !img;
    if (blank)
        img = key_png("Blank_Black_Normal", &iw, &ih);
    if (!img)
        return;
    /* the picture without its clear margin, as large as fits, its shape kept */
    int bx0 = iw, by0 = ih, bx1 = -1, by1 = -1;
    for (int v = 0; v < ih; v++)
        for (int u = 0; u < iw; u++)
            if (img[4 * (v * iw + u) + 3] > 8) {
                if (u < bx0) bx0 = u;
                if (u > bx1) bx1 = u;
                if (v < by0) by0 = v;
                if (v > by1) by1 = v;
            }
    if (bx1 < bx0) {
        stbi_image_free(img);
        return;
    }
    float bw = (float)(bx1 - bx0 + 1), bh = (float)(by1 - by0 + 1);
    float k = fminf((x1 - x0) / bw, (y1 - y0) / bh), dw = bw * k, dh = bh * k;
    float ox = (x0 + x1 - dw) / 2, oy = (y0 + y1 - dh) / 2, side = fminf(dw, dh);
    for (int Y = (int)oy; Y < (int)ceilf(oy + dh) && Y < ph; Y++)
        for (int X = (int)ox; X < (int)ceilf(ox + dw) && X < pw; X++) {
            if (X < 0 || Y < 0)
                continue;
            float u0 = bx0 + (X - ox) / k, u1 = bx0 + (X + 1 - ox) / k;
            float v0 = by0 + (Y - oy) / k, v1 = by0 + (Y + 1 - oy) / k;
            float acc[4] = { 0 }, nn = 0;
            for (int v = (int)floorf(v0); v < (int)ceilf(v1); v++)
                for (int u = (int)floorf(u0); u < (int)ceilf(u1); u++) {
                    nn++;
                    if (u < 0 || v < 0 || u >= iw || v >= ih)
                        continue;
                    const uint8_t *q = &img[4 * (v * iw + u)];
                    float a = q[3] / 255.0f;
                    acc[0] += q[0] * a; acc[1] += q[1] * a; acc[2] += q[2] * a; acc[3] += a;
                }
            if (!nn || acc[3] <= 0)
                continue;
            float a = acc[3] / nn;
            uint32_t c = (uint32_t)lroundf(acc[0] / acc[3]) << 16 | (uint32_t)lroundf(acc[1] / acc[3]) << 8 |
                         (uint32_t)lroundf(acc[2] / acc[3]);
            uint32_t d = px[Y * pw + X];
            float da = (d >> 24) / 255.0f, oa = a + da * (1 - a);
            uint32_t m = oa > 0 ? mix(d, c, a / oa) : c;
            px[Y * pw + X] = (uint32_t)lroundf(oa * 255) << 24 | (m & 0xFFFFFF);
        }
    stbi_image_free(img);
    if (blank) {                                       /* the key's name, in the set's light grey */
        char label[64];
        ngb_bind_input_name(in, label, sizeof label);
        if (!s_font_bold.data && !stbtt_InitFont(&s_font_bold, ngb_font_label, 0))
            return;
        int game = s_game_font;
        s_game_font = 0;
        float size = side * 0.42f, tw = text_width(&s_font_bold, label, size, 1.0f);
        if (tw > dw * 0.72f) {
            size *= dw * 0.72f / tw;
            tw = dw * 0.72f;
        }
        uint8_t *m = calloc((size_t)pw * ph, 1);
        draw_text(m, pw, ph, &s_font_bold, label, size, 1.0f, ox + (dw - tw) / 2, oy + dh * 0.45f + size * 0.33f);
        s_game_font = game;
        for (int i = 0; i < pw * ph; i++)
            if (m[i])
                px[i] = (px[i] & 0xFF000000u) | (mix(px[i], 0xD8D8D8, m[i] / 255.0f) & 0xFFFFFF);
        free(m);
    }
}

static GuestTex s_kc[2];                  /* 0 in play, 1 in the menus */
static int s_kc_made;                     /* 0 not yet, 1 made, -1 could not */
static uint32_t s_kc_orig;                /* the game's texture header in the table */
static unsigned s_kc_gen;                 /* the bindings play's icons were made from */

static int kc_make(uint32_t fmt_low)
{
    static const int play_b[6] = { 2, 3, 1, 0, 6, 7 };   /* X Y B A L R */
    int play[6];
    for (int i = 0; i < 6; i++)
        play[i] = ngb_bind_prompt(play_b[i]);
    s_kc_gen = ngb_bind_generation();
    /* in the menus the mouse: a left click chooses (where it points), a right click goes back */
    static const int menu[6] = { BIND_MOUSE | 1, SDL_SCANCODE_K, BIND_MOUSE | 3, SDL_SCANCODE_SPACE,
                                 SDL_SCANCODE_LSHIFT, SDL_SCANCODE_Q };
    for (int t = 0; t < 2; t++) {
        uint32_t *px = calloc(KC_W * KS * KC_H * KS, sizeof *px);
        for (int i = 0; i < 6; i++) {
            int in = t ? menu[i] : play[i];
            float x = (float)(i * KC_CELL_W * KS);
            if (in)
                key_icon(px, KC_W * KS, KC_H * KS, x, 0, x + KC_CELL_W * KS, KC_CELL_H * KS, in);
        }
        int ok = s_kc[t].hdr || make_texture(&s_kc[t], KC_W * KS, KC_H * KS, fmt_low);
        if (ok)
            upload(&s_kc[t], px);
        free(px);
        if (!ok)
            return 0;
    }
    return 1;
}

/* The tab row's first and last plates as last drawn (x0 y0 x1 y1, the
 * menu's 720x480), for the L and R plates' keys (kc_tab_keys). */
static float s_tab_first[4], s_tab_last[4];
static void kc_tab_seen(uint32_t id, float x0, float y0, float x1, float y1)
{
    float *r = id == 0x17B ? s_tab_first : s_tab_last;
    r[0] = x0; r[1] = y0; r[2] = x1; r[3] = y1;
    if (getenv("NGB_TAB_LOG")) {
        static int n;
        if (n++ % 120 == 0)
            fprintf(stderr, "[TAB] %X at %.1f,%.1f-%.1f,%.1f\n", id, x0, y0, x1, y1);
    }
}

/* The pause menu's L and R plates (in the tab row's own picture), on the
 * keyboard: Q and E keycaps over them, which change tabs in the menus
 * (src/keyboard.c). Placed over those plates. */
#define TK_SPRITE (PT_SPRITE + 9)     /* + 0 Q, + 1 E */
#define TK_TEX    (PT_TEX + 9)
static GuestTex s_tk;
static uint32_t s_tk_entry, s_tk_defs;

static int tk_ready(void)
{
    if (s_tk_defs)
        return 1;
    static int failed;
    if (failed)
        return 0;
    uint32_t *px = calloc(64 * KS * 32 * KS, sizeof *px);
    /* the prompts' keys (21 x 23 cells), larger */
    key_icon(px, 64 * KS, 32 * KS, 0, 0, 21 * KS, 23 * KS, SDL_SCANCODE_Q);
    key_icon(px, 64 * KS, 32 * KS, 32 * KS, 0, 53 * KS, 23 * KS, SDL_SCANCODE_E);
    if (!make_texture(&s_tk, 64 * KS, 32 * KS, 0x2A)) {
        free(px);
        failed = 1;
        return 0;
    }
    upload(&s_tk, px);
    free(px);
    s_tk_entry = xbox_HeapAlloc(8, 8);
    MEM32(s_tk_entry) = s_tk.hdr;
    MEM32(s_tk_entry + 4) = s_tk.hdr;
    uint32_t d = xbox_HeapAlloc(0x18 * 2, 8);
    for (int i = 0; i < 2; i++, d += 0x18) {
        MEMF(d) = i * 0.5f; MEMF(d + 4) = 0; MEMF(d + 8) = i * 0.5f + 21.0f / 64; MEMF(d + 12) = 23.0f / 32;
        MEM32(d + 0x10) = TK_TEX;
        MEM16(d + 0x14) = 64;                          /* the texture's size: a slice of it is drawn */
        MEM16(d + 0x16) = 32;
    }
    s_tk_defs = d - 0x18 * 2;
    return 1;
}

static void sprite(uint32_t id, int x, int y, int z, uint8_t anchor, float sx, float sy, float bright, float alpha);

static void kc_tab_keys(void)
{
    if (getenv("NGB_TAB_LOG")) {
        static int n;
        if (n++ % 120 == 0)
            fprintf(stderr, "[TAB] tick: kbm %d first %.1f..%.1f\n", xbox_InputKeyboardActive(), s_tab_first[0], s_tab_first[2]);
    }
    if (!xbox_InputKeyboardActive() || getenv("NGB_PAD_PROMPTS") || s_tab_first[2] <= s_tab_first[0] ||
        !tk_ready())
        return;
    /* Over the row's L and R plates, at their size: they stay at x 52..84 and 636..668 whichever tab is
     * chosen (the tabs' plates between them grow and move), 44..76 high with the row at rest -- 18.5
     * above the Weapon plate's centre, which follows the row as it slides in */
    float cy = (s_tab_first[1] + s_tab_first[3]) / 2;
    int y = (int)lroundf(cy - 18.5f);
    if (getenv("NGB_TAB_LOG")) {
        static int n;
        if (n++ % 120 == 0)
            fprintf(stderr, "[TAB] keys at y %d\n", y);
    }
    sprite(TK_SPRITE, 52, y - 1, 0x1F0, 0, 34.0f / 21, 34.0f / 23, 1.0f, 1.0f);   /* on the 32 x 32 plate */
    sprite(TK_SPRITE + 1, 636, y - 1, 0x1F0, 0, 34.0f / 21, 34.0f / 23, 1.0f, 1.0f);
}

/* Each button icon the game queues: the texture for the device in use. */
static void kc_check(uint32_t id)
{
    if (id < 5 || id > 10 || s_kc_made < 0 || getenv("NGB_PAD_PROMPTS"))
        return;
    uint32_t def = sprite_def(id);
    if (!def)
        return;
    uint32_t entry = 0x8DBB28 + MEM32(def + 0x10) * 8, now = MEM32(entry);
    int ours = s_kc_made > 0 && (now == s_kc[0].hdr || now == s_kc[1].hdr);
    if (!xbox_InputKeyboardActive()) {
        if (ours)
            MEM32(entry) = s_kc_orig;                   /* the controller's icons back */
        return;
    }
    if (!s_kc_made) {
        s_kc_made = now && kc_make(MEM32(now + 0xC) & 0xFF) ? 1 : -1;
        if (s_kc_made < 0)
            return;
    } else if (s_kc_gen != ngb_bind_generation()) {
        kc_make(0);                                   /* the bindings changed: play's icons again */
    }
    if (!ours)
        s_kc_orig = now;
    MEM32(entry) = s_kc[ngb_menu_up() ? 1 : 0].hdr;
}

/* ---------- making it ---------- */

uint32_t *ngb_guest_texture(uint32_t hdr, uint32_t pal_hdr, int *w, int *h);
uint32_t ngb_sprite_def(uint32_t id);

/* The game's sprite definition for an id, as sub_0021A960 finds it. */
static uint32_t sprite_def(uint32_t id)
{
    for (uint32_t g = 0x452A6C; g < 0x45A7F4; g += 0x34)
        if (MEM32(g - 32) && MEM32(g) <= id && id < MEM32(g) + MEM32(g + 4))
            return MEM32(g - 16) + (id - MEM32(g)) * 0x18;
    return 0;
}

/* For src/menu_font.c. */
uint32_t *ngb_guest_texture(uint32_t hdr, uint32_t pal_hdr, int *w, int *h)
{
    return read_texture(hdr, pal_hdr, w, h);
}

uint32_t ngb_sprite_def(uint32_t id)
{
    return sprite_def(id);
}

/* Once the pause menu's sprites are loaded: our plate, glow and page. */
static int pt_ready(void)
{
    if (s_ready >= 0 || !s_enabled)
        return s_ready > 0;
    uint32_t def = sprite_def(EXIT_PLATE);
    if (!def)
        return 0;                                   /* not loaded yet: later */
    s_game_font = 1;
    if (s_game_font && !game_font_ready())
        return 0;                                   /* the letters: later */
    s_ready = 0;
    uint32_t idx = MEM32(def + 0x10);
    if (idx >= PT_TEX)
        return 0;
    uint32_t hdr = MEM32(0x8DBB28 + idx * 8), pal = MEM32(0x8DBB2C + idx * 8);
    int w, h;
    uint32_t *plate = hdr ? read_texture(hdr, pal, &w, &h) : NULL;
    if (!plate || w > 256 || h > 128) {
        free(plate);
        fprintf(stderr, "[OPTIONS] the Exit plate is not one read here: six tabs\n");
        return 0;
    }
    if (!stbtt_InitFont(&s_font_tab, ngb_font_tab, 0) ||
        !stbtt_InitFont(&s_font_body, ngb_font_body, 0)) {
        free(plate);
        return 0;
    }
    uint32_t fmt_low = MEM32(hdr + 0xC) & 0xFF;
    if (!make_texture(&s_tex[0], w, h, fmt_low) || !make_texture(&s_tex[1], w, h, fmt_low) ||
        !make_texture(&s_tex[2], PAGE_TW, PAGE_TH, fmt_low)) {
        free(plate);
        return 0;
    }
    make_plate(plate, w, h);
    upload(&s_tex[0], plate);
    memset(plate, 0, sizeof *plate * w * h);
    make_glow(plate, w, h);
    upload(&s_tex[1], plate);
    free(plate);

    s_entries = xbox_HeapAlloc(8 * PT_N, 8);
    s_defs = xbox_HeapAlloc(0x18 * PT_N, 8);
    for (int i = 0; i < PT_N; i++) {
        uint32_t e = s_entries + 8 * i, d = s_defs + 0x18 * i;
        MEM32(e) = s_tex[i].hdr;
        MEM32(e + 4) = pal;           /* unused by A8R8G8B8, but never null */
        MEMF(d) = 0; MEMF(d + 4) = 0; MEMF(d + 8) = 1; MEMF(d + 12) = 1;
        MEM32(d + 0x10) = PT_TEX + i;
        MEM16(d + 0x14) = (uint16_t)(i < 2 ? w : PAGE_W);
        MEM16(d + 0x16) = (uint16_t)(i < 2 ? h : PAGE_H);
    }
    s_ready = 1;
    fprintf(stderr, "[SETTINGS] pause menu tab ready (textures at %08X %08X %08X)\n",
            s_tex[0].data, s_tex[1].data, s_tex[2].data);
    return 1;
}

static int pt_tabs(void) { return pt_ready() ? 7 : 6; }
/* Exit's row: the last, so 5 without the seventh tab, 6 with. */
#define EXIT_TAB (pt_tabs() == 7 ? 6u : 5u)

/* x of tab i (0-6) when there are seven: the six's, closed up. */
static int pt_tab_x(int i, int x6)
{
    if (pt_tabs() != 7)
        return x6;
    if (i == 6)
        x6 = 610;                                    /* after Settings (525) */
    return 74 + (int)lroundf((x6 - 80) * 0.88f);
}
#define PT_SCALE 0.88f

/* ---------- calling the game ---------- */

typedef struct { uint32_t ebx, esi, edi, ebp, esp, seh; } Saved;
static void save(Saved *s) { s->ebx = ebx; s->esi = esi; s->edi = edi; s->ebp = g_ebp; s->esp = esp; s->seh = g_seh_ebp; }
static void restore(const Saved *s) { ebx = s->ebx; esi = s->esi; edi = s->edi; g_ebp = s->ebp; esp = s->esp; g_seh_ebp = s->seh; }

void sub_002216B0(void);
void sub_00221170(void);
void sub_001E3400(void);

static void play_sound(int which)
{
    Saved sv;
    save(&sv);
    PUSH32(esp, 0x40);
    PUSH32(esp, (uint32_t)which);
    PUSH32(esp, 0);
    sub_001E3400();
    restore(&sv);
}

/* One sprite into the game's list, as its own menu code does it. */
static void sprite(uint32_t id, int x, int y, int z, uint8_t anchor, float sx, float sy,
                   float bright, float alpha)
{
    Saved sv;
    save(&sv);
    esp = (esp - 0x80) & ~0xFu;
    uint32_t p = esp + 0x10;
    MEM32(p) = id;
    MEM16(p + 4) = (uint16_t)x;
    MEM16(p + 6) = (uint16_t)y;
    MEM16(p + 8) = (uint16_t)z;
    MEM8(p + 0xA) = anchor;
    MEM8(p + 0xB) = 1;
    MEM8(p + 0xC) = 1;
    MEM32(p + 0x10) = 0;
    MEMF(p + 0x14) = sx;
    MEMF(p + 0x18) = sy;
    MEMF(p + 0x1C) = bright;
    MEMF(p + 0x20) = alpha;
    MEMF(p + 0x24) = 0;
    MEM32(p + 0x28) = 0xFFFFFFFFu;
    if (MEM32(0x43B490) == 1 && MEM32(0x8F7DFC) < MEM32(0x8F7DF8)) {
        ecx = MEM32(0x8F7DFC);
        esi = ecx * 0x70 + MEM32(0x8F7DF4);
        ecx = (ecx << 4) + MEM32(0x8F7DF0);
        PUSH32(esp, 0);
        PUSH32(esp, 0);
        eax = p;
        PUSH32(esp, 0);
        sub_002216B0();
        esp += 8;
        if (eax) {
            ecx = MEM32(0x8F7DFC);
            eax = (uint32_t)z;
            edx = 0;
            PUSH32(esp, 0);
            sub_00221170();
            MEM32(0x8F7DFC) = MEM32(0x8F7DFC) + 1;
        }
    }
    restore(&sv);
}

/* ---------- the page, each frame ---------- */

int ngb_pause_tab_key(int *focus);   /* src/ingame_menu.c */
void ngb_pause_tab_hold(void);
void ngb_menu_mark(void);            /* src/menu_mouse.c */
#include "menu_mouse.h"

/* A debugging option from the environment, read once (these are asked per sprite or per frame). */
#define ENV_ONCE(name) (__extension__({ static const char *v_ = (const char *)1; \
    if (v_ == (const char *)1) v_ = getenv(name); v_; }))


/* K_ROW | the row the mouse is on (HIT_*: up to HIT_CTL + CTL_ROWS * 4, past a byte since the quick menu's
 * actions -- read back as a byte, the lower rows of Controls were other rows and categories) */
enum { K_UP = 1, K_DOWN, K_LEFT, K_RIGHT, K_OK, K_BACK, K_CLEAR, K_ROW = 0x10000 };

/* For the mouse (any thread): the row of the page at (x, y) in the menu's
 * 720x480, or -1; *zone -1 on the "<" half of its value, 1 on the ">"
 * half, 0 on its name. */
static int s_focus, s_row = -1, s_cat;

/* The page's rows for the mouse: 0-4 a row of the category, 64 + c the
 * strip's word c. */
#define HIT_CAT 64
#define HIT_CTL 128

/* Controls' list at (x, y) in page units: HIT_CTL + row * 4 + the column (3 its name), or -1. */
static int ctl_hit(float x, float y)
{
    if (x < 0 || x >= PAGE_W || y < CTL_Y0 || y >= CTL_Y0 + CTL_SHOWN * ROW_H)
        return -1;
    int r = s_ctl_top + (int)((y - CTL_Y0) / ROW_H), c = 3;
    if (r >= CTL_ROWS)
        return -1;
    if (r >= A_COUNT)
        c = 0;                                        /* the camera rows, Default Controls: anywhere on it */
    for (int i = 0; i < 3 && r < A_COUNT; i++)
        if (fabsf(x - s_ctl_cx[i]) <= s_ctl_w[i] / 2)
            c = i;
    return HIT_CTL + r * 4 + c;
}

int ngb_pause_tab_hit(float x, float y, int *zone)
{
    x -= PAGE_X;
    y -= PAGE_Y;
    *zone = 0;
    if (y >= STRIP_Y && y < STRIP_Y + STRIP_H) {
        for (int c = 0; c < CATS; c++)
            if (x >= s_cat_x[c][0] && x < s_cat_x[c][1])
                return HIT_CAT + c;
        return -1;
    }
    if (s_cat == CAT_CTL)
        return ctl_hit(x, y);
    if (x < 0 || x >= PAGE_W || y < ROW_Y0 || y >= ROW_Y0 + cat_count(s_cat) * SET_ROW_H)
        return -1;
    if (x >= 262)
        *zone = x < 350 ? -1 : 1;
    return (int)((y - ROW_Y0) / SET_ROW_H);
}

/* A key on Controls' list (row its row); strip: up from the first row and down
 * from the last go to the categories' strip (row -1). Returns 0. */
static int ctl_keys(int key, int *row, int strip)
{
    switch (key) {
    case K_UP:
        if (*row > 0 || strip) {
            (*row)--;
            play_sound(1);
        }
        break;
    case K_DOWN:
        if (*row < CTL_ROWS - 1 || strip) {
            *row = *row >= CTL_ROWS - 1 ? -1 : *row + 1;
            play_sound(1);
        }
        break;
    case K_LEFT:
    case K_RIGHT: {
        if (*row == CTL_MOUSE) {
            ngb_bind_step_mouse_sensitivity(key == K_LEFT ? -1 : 1);
            play_sound(1);
            break;
        }
        int c = s_ctl_col + (key == K_LEFT ? -1 : 1);
        if (c >= 0 && c <= 2 && !(c == 2 && *row < A_COUNT && ngb_bind_pad_fixed(*row))) {
            s_ctl_col = c;
            play_sound(1);
        }
        break;
    }
    case K_OK:
        if (CTL_TOGGLE(*row)) {
            ctl_toggle_flip(*row);
            play_sound(6);
        } else if (*row == CTL_MOUSE) {
            ngb_bind_step_mouse_sensitivity(1);
            play_sound(6);
        } else if (*row == CTL_DEF) {
            ngb_bind_defaults();
            ngb_bind_set(0, 0, ngb_bind_get(0, 0));   /* saved */
            play_sound(6);
        } else if (!(s_ctl_col == 2 && ngb_bind_pad_fixed(*row))) {
            ngb_bind_capture_begin(*row, s_ctl_col);
            play_sound(1);
        }
        break;
    case K_CLEAR:
        if (*row < A_COUNT && !(s_ctl_col == 2 && ngb_bind_pad_fixed(*row))) {
            ngb_bind_set(*row, s_ctl_col, 0);
            play_sound(3);
        }
        break;
    }
    return 0;
}

/* The mouse on a cell of the list (HIT_CTL + row * 4 + column, 3 its name). */
static void ctl_mouse(int r, int *row)
{
    int rr = (r - HIT_CTL) / 4, c = (r - HIT_CTL) % 4;
    if (rr != *row || (c < 3 && c != s_ctl_col))
        play_sound(1);
    *row = rr;
    if (c < 3 && !(c == 2 && ngb_bind_pad_fixed(rr)))
        s_ctl_col = c;
}

/* Each frame the tab is the chosen one (from the tab row's pass): the pad,
 * then the page. */
static void pt_draw(void)
{
    int was = s_focus, vals[S_COUNT], n = cat_count(s_cat);
    int key = ngb_pause_tab_key(&s_focus);
    if (key == K_CLEAR && !(s_cat == CAT_CTL && s_row >= 0))
        key = K_OK;                                   /* X chooses, as in the game's menus (#111); in Controls' list it clears */
    if (ngb_bind_capturing()) {                       /* waiting for the key to bind: nothing else */
        key = 0;
        if (!ngb_bind_capture_tick())
            play_sound(6);
    }
    if (key & K_ROW) {                                /* the mouse on a row or a category */
        int r = key & 0xFFFF;
        if (r >= HIT_CTL && s_cat == CAT_CTL) {      /* a cell of Controls' list */
            ctl_mouse(r, &s_row);
        } else if (r >= HIT_CAT && r < HIT_CAT + CATS) {
            if (s_row >= 0 || r - HIT_CAT != s_cat) {
                s_cat = r - HIT_CAT;
                s_row = -1;
                play_sound(1);
            }
        } else if (r != s_row && r < n) {
            s_row = r;
            play_sound(1);
        }
        key = 0;
    }
    if (key == K_BACK && s_cat == CAT_UNL) {         /* Unlocks: back to Cheats, on its row */
        ngb_pause_tab_hold();
        s_focus = 1;
        s_cat = CAT_CHEAT;
        for (s_row = 0; s_cat_rows[CAT_CHEAT][s_row] != S_UNLOCKS; s_row++)
            ;
        play_sound(3);
        key = 0;
    }
    if ((key == K_OK || key == K_RIGHT || key == K_LEFT) && s_row >= 0 && s_row < n &&
        s_cat_rows[s_cat][s_row] == S_UNLOCKS) {     /* Cheats' Unlocks row: its page */
        if (key != K_LEFT) {
            s_cat = CAT_UNL;
            s_row = 0;
            play_sound(6);
        }
        key = 0;
    }
    if (s_focus && !was)
        s_row = -1;                                   /* in: on the strip */
    if (s_cat == CAT_CTL && s_row >= 0 && key)
        key = ctl_keys(key, &s_row, 1);
    if (s_cat == CAT_CTL) {
        if (s_row >= 0 && s_row < A_COUNT && s_ctl_col == 2 && ngb_bind_pad_fixed(s_row))
            s_ctl_col = 1;                            /* a stick's: not the controller's to change */
        ctl_scroll(s_row);
        if (s_row < 0)
            s_ctl_top = 0;
    }
    switch (key) {
    case K_UP:
        s_row = s_row < 0 ? n - 1 : s_row - 1;
        play_sound(1);
        break;
    case K_DOWN:
        s_row = s_row >= n - 1 ? -1 : s_row + 1;
        play_sound(1);
        break;
    case K_LEFT:
    case K_RIGHT:
    case K_OK:
        if (s_row < 0 && key == K_OK) {
            s_row = 0;                                /* into the category */
        } else if (s_row < 0) {
            s_cat = (s_cat + (key == K_LEFT ? CATS - 1 : 1)) % CATS;
        } else {
            change(s_cat_rows[s_cat][s_row], key == K_LEFT ? -1 : 1);
        }
        play_sound(1);
        break;
    }
    if (s_row < 0 && s_cat == CAT_UNL)
        s_cat = CAT_CHEAT;                            /* up out of Unlocks: the strip, on Cheats */
    if (s_focus != was)
        play_sound(s_focus ? 6 : 3);                  /* in, as a tab's page; back */
    for (int i = 0; i < S_COUNT; i++)
        vals[i] = ngb_pc_setting(i);
    /* what is drawn, each part in bits of its own (the list's top, past 15 since the quick menu's actions,
     * ran into the column's: a scroll could leave the page as it was) */
    int shown = (s_cat & 7) << 28 | ((s_row + 1) & 0xFF) << 20 | (s_ctl_col & 3) << 18 |
                (s_cat == CAT_CTL ? s_ctl_top & 0x3F : 0) << 12;
    if (s_cat == CAT_CTL)                             /* and the bindings, and waiting for one */
        shown ^= (int)(ngb_bind_generation() & 0x7FF) << 1 ^ ngb_bind_capturing();
    if (s_drawn.page != 0 || s_focus != s_drawn.focused || shown != s_drawn.row ||
        memcmp(vals, s_drawn.vals, sizeof vals)) {
        draw_page(s_focus, s_cat, s_row);
        if (s_cat == CAT_CTL && getenv("NGB_CTL_LOG"))   /* NGB_CTL_LOG=1: the Controls list as drawn */
            fprintf(stderr, "[CTL] row %d col %d top %d\n", s_row, s_ctl_col, s_ctl_top);
        s_drawn.page = 0;
        s_drawn.focused = s_focus;
        s_drawn.row = shown;
        memcpy(s_drawn.vals, vals, sizeof vals);
    }
    sprite(PT_SPRITE + 2, PAGE_X, PAGE_Y, 0x1CC, 0, 1.0f, 1.0f, 1.0f, 1.0f);
}

/* ---------- Are you sure? ---------- */

/* A question and Yes / No in a box in the menus' colours, over the pause
 * menu (Exit's page) or the main menu (EXIT TO DESKTOP): the pad its, A or
 * left/right to choose, B for No (src/ingame_menu.c's pad for the tab). */
void ngb_tab_take_pad(void);          /* src/ingame_menu.c */
void ngb_tab_give_pad(void);
void ngb_quit_to_desktop(void);
void ngb_restart_to_menu(void);
void ngb_pause_resume(void);

#define DLG_W 420                     /* in the menu's 720x480 */
#define DLG_H 105                     /* the texture's shape: 1024 x 256 */
#define DLG_TW 1024                   /* its texture */
#define DLG_TH 256
static GuestTex s_dlg_tex;
static int s_dlg_open, s_dlg_sel, s_dlg_focus, s_dlg_drawn = -1, s_dlg_back;
static const char *s_dlg_q;

static void dlg_fill(uint32_t *px, float x, float y, float w, float h, uint32_t rgb, float a)
{
    const float k = (float)DLG_TW / DLG_W;
    for (int Y = (int)(y * k); Y < (int)((y + h) * k) && Y < DLG_TH; Y++)
        for (int X = (int)(x * k); X < (int)((x + w) * k) && X < DLG_TW; X++) {
            uint32_t c = px[Y * DLG_TW + X];
            float ca = (c >> 24) / 255.0f, oa = a + ca * (1 - a);
            uint32_t m = oa > 0 ? mix(c, rgb, a / oa) : rgb;
            px[Y * DLG_TW + X] = (uint32_t)lroundf(oa * 255) << 24 | (m & 0xFFFFFF);
        }
}

/* Text into the box: its letters (the game's menu font, as the pages) in rgb
 * over a dark edge; glow: in the violet haze of the game's chosen menu item. */
static void dlg_text(uint32_t *px, const char *t, float cx, float base, float size, uint32_t rgb, int glow)
{
    static uint8_t m[DLG_TW * DLG_TH];
    static float halo[DLG_TW * DLG_TH];
    const float k = (float)DLG_TW / DLG_W;
    int game = s_game_font;
    s_game_font = game_font_ready();
    float w = text_width(&s_font_body, t, size * k, 1.0f);
    memset(m, 0, sizeof m);
    draw_text(m, DLG_TW, DLG_TH, &s_font_body, t, size * k, 1.0f, cx * k - w / 2, base * k);
    s_game_font = game;
    if (glow) {   /* as make_glow: white letters in a strong violet haze */
        for (int i = 0; i < DLG_TW * DLG_TH; i++)
            halo[i] = m[i] / 255.0f;
        box_blur(halo, DLG_TW, DLG_TH, 5);
        for (int i = 0; i < DLG_TW * DLG_TH; i++) {
            float a = fminf(halo[i] * 3.5f, 1.0f) * 0.9f;
            if (a > 0.01f)
                px[i] = mix(px[i], 0x8986F4, a);
        }
    }
    for (int Y = 3; Y < DLG_TH; Y++)                   /* the dark edge under, then the letters */
        for (int X = 3; X < DLG_TW; X++)
            if (m[(Y - 3) * DLG_TW + X - 3])
                px[Y * DLG_TW + X] = mix(px[Y * DLG_TW + X], 0x000000, m[(Y - 3) * DLG_TW + X - 3] / 255.0f * 0.8f);
    for (int i = 0; i < DLG_TW * DLG_TH; i++)
        if (m[i])
            px[i] = mix(px[i], rgb, m[i] / 255.0f);
}

/* As the game's own menus: a near-black panel between gold rules (its Information bar's), the
 * question in white, Yes and No as its menu items -- grey, the chosen one white in the violet haze. */
static void dlg_draw(void)
{
    static uint32_t px[DLG_TW * DLG_TH];
    memset(px, 0, sizeof px);
    for (int y = 0; y < DLG_H; y++) {                  /* the panel, darker at its middle */
        float t = fabsf(y - DLG_H / 2.0f) / (DLG_H / 2.0f);
        dlg_fill(px, 0, (float)y, DLG_W, 1, 0x040404, 0.93f - 0.1f * t * t);
    }
    for (int x = 0; x < DLG_W; x++) {                  /* the rules, fading toward the ends */
        float t = fabsf(x - DLG_W / 2.0f) / (DLG_W / 2.0f), a = 0.95f * (1 - t * t * t * t);
        dlg_fill(px, (float)x, 2, 1, 1.5f, 0xA08A48, a);
        dlg_fill(px, (float)x, DLG_H - 3.5f, 1, 1.5f, 0xA08A48, a);
        dlg_fill(px, (float)x, 5, 1, 0.75f, 0x6A5A2A, a * 0.6f);
        dlg_fill(px, (float)x, DLG_H - 6, 1, 0.75f, 0x6A5A2A, a * 0.6f);
    }
    dlg_text(px, s_dlg_q, DLG_W / 2.0f, 45, 28, 0xF0F0F0, 0);
    for (int i = 0; i < 2; i++) {
        float cx = DLG_W * (i ? 0.66f : 0.34f);
        if (i == s_dlg_sel) {   /* the game's chosen item: a soft violet bar behind it */
            const float k = (float)DLG_TW / DLG_W;
            for (int Y = (int)(62 * k); Y < (int)(96 * k) && Y < DLG_TH; Y++)
                for (int X = (int)((cx - 70) * k); X < (int)((cx + 70) * k); X++) {
                    float dx = (X / k - cx) / 70, dy = (Y / k - 79) / 17, d = dx * dx + dy * dy;
                    if (d < 1)
                        px[Y * DLG_TW + X] = mix(px[Y * DLG_TW + X], 0x6E64D2, 0.75f * (1 - d) * (1 - d));
                }
        }
        dlg_text(px, i ? "No" : "Yes", cx, 88, 27, i == s_dlg_sel ? 0xFFFFFF : 0x9A9A9A, i == s_dlg_sel);
    }
    upload(&s_dlg_tex, px);
}

static int dlg_ready(void)
{
    if (s_dlg_def)
        return 1;
    if (!s_font_body.data && !stbtt_InitFont(&s_font_body, ngb_font_body, 0))
        return 0;
    if (!make_texture(&s_dlg_tex, DLG_TW, DLG_TH, 0x2A))
        return 0;
    s_dlg_entry = xbox_HeapAlloc(8, 8);
    uint32_t d = xbox_HeapAlloc(0x18, 8);
    MEM32(s_dlg_entry) = s_dlg_tex.hdr;
    MEM32(s_dlg_entry + 4) = s_dlg_tex.hdr;            /* (no palette: never read) */
    MEMF(d) = 0; MEMF(d + 4) = 0; MEMF(d + 8) = 1; MEMF(d + 12) = 1;
    MEM32(d + 0x10) = DLG_TEX;
    MEM16(d + 0x14) = DLG_W;
    MEM16(d + 0x16) = DLG_H;
    s_dlg_def = d;
    return 1;
}

/* Opens the box with question q (No chosen); 0 if it cannot be made. */
static int ngb_confirm_open(const char *q, int pad_taken)
{
    if (!dlg_ready())
        return 0;
    s_dlg_q = q;
    s_dlg_sel = 1;
    s_dlg_open = 1;
    s_dlg_drawn = -1;
    s_dlg_focus = 1;
    if (!pad_taken)
        ngb_tab_take_pad();
    return 1;
}

/* Each frame while open: -1 still open, 1 Yes, 0 No (B, or L/R). */
static int ngb_confirm_tick(void)
{
    if (!s_dlg_open)
        return 0;
    int key = ngb_pause_tab_key(&s_dlg_focus), r = -1;
    if (key == K_CLEAR)
        key = K_OK;                                    /* X chooses, as in the game's menus (#111) */
    s_dlg_back = key == K_BACK;
    if (key & K_ROW) {                                 /* the mouse */
        if ((key & 0xFFFF) < 2 && (key & 0xFFFF) != s_dlg_sel) {
            s_dlg_sel = key & 0xFFFF;
            play_sound(1);
        }
        key = 0;
    }
    switch (key) {
    case K_LEFT:
    case K_RIGHT:
    case K_UP:
    case K_DOWN:
        s_dlg_sel ^= 1;
        play_sound(1);
        break;
    case K_OK:
        r = s_dlg_sel == 0;
        play_sound(r ? 6 : 3);
        break;
    case K_BACK:
        r = 0;
        play_sound(3);
        break;
    }
    if (!s_dlg_focus && r < 0)                         /* L/R: gone to another tab */
        r = 0;
    if (r >= 0) {
        s_dlg_open = 0;
        return r;
    }
    if (s_dlg_drawn != s_dlg_sel) {
        dlg_draw();
        s_dlg_drawn = s_dlg_sel;
    }
    sprite(DLG_SPRITE, 360 - DLG_W / 2, 240 - DLG_H / 2, 0x1CC, 0, 1.0f, 1.0f, 1.0f, 1.0f);
    return -1;
}

int ngb_confirm_is_open(void) { return s_dlg_open; }

/* ---------- the Controls panel ---------- */

/* Controls' list on a panel of its own over the Options menu: from the main
 * menu's RECOMP SETTINGS (src/pc_menu.c's Controls category) and in place of
 * the game's own Button Settings, which knows only the controller and shows
 * the menus' keys. It has the pad while open (as a tab's page); B (Esc, a
 * right click) closes it and the menu goes back from the page under it. */
#define OV_SPRITE (PT_SPRITE + 11)
#define OV_TEX    (PT_TEX + 11)
#define OV_X 62                       /* in the menu's 720x480: over the page */
#define OV_Y 86
#define OV_H 270                      /* of PAGE_H drawn */
#define OV_S 1.16f                    /* drawn larger */
static GuestTex s_ov_tex;
static uint32_t s_ov_entry, s_ov_def;
static int s_ov_open, s_ov_focus, s_ov_row, s_ov_drawn = -1;
void ngb_tab_give_pad_back(void);     /* src/ingame_menu.c */

static int ov_ready(void)
{
    if (s_ov_def)
        return 1;
    if (!s_font_body.data && !stbtt_InitFont(&s_font_body, ngb_font_body, 0))
        return 0;
    if (!make_texture(&s_ov_tex, PAGE_TW, PAGE_TH, 0x2A))
        return 0;
    s_ov_entry = xbox_HeapAlloc(8, 8);
    uint32_t d = xbox_HeapAlloc(0x18, 8);
    MEM32(s_ov_entry) = s_ov_tex.hdr;
    MEM32(s_ov_entry + 4) = s_ov_tex.hdr;
    MEMF(d) = 0; MEMF(d + 4) = 0; MEMF(d + 8) = 1; MEMF(d + 12) = 1;
    MEM32(d + 0x10) = OV_TEX;
    MEM16(d + 0x14) = PAGE_W;
    MEM16(d + 0x16) = PAGE_H;
    s_ov_def = d;
    return 1;
}

static void ov_draw(void)
{
    static uint32_t px[PAGE_TW * PAGE_TH];
    static uint8_t m[PAGE_TW * PAGE_TH];
    memset(px, 0, sizeof px);
    int game = s_game_font;
    s_game_font = game_font_ready();
    for (int y = 0; y < OV_H; y++) {                   /* the panel, as the Are you sure? box */
        float t = fabsf(y - OV_H / 2.0f) / (OV_H / 2.0f);
        page_fill(px, 0, (float)y, PAGE_W, 1, 0x040404, 0.95f - 0.08f * t * t, 0.95f - 0.08f * t * t);
    }
    for (int x = 0; x < PAGE_W; x++) {
        float t = fabsf(x - PAGE_W / 2.0f) / (PAGE_W / 2.0f), a = 0.95f * (1 - t * t * t * t);
        page_fill(px, (float)x, 2, 1, 1.5f, 0xA08A48, a, a);
        page_fill(px, (float)x, OV_H - 3.5f, 1, 1.5f, 0xA08A48, a, a);
    }
    page_text(px, m, "Controls", 14, 28, 22, 0xC8C4FF, 0);
    draw_controls(px, m, 1, s_ov_row);
    s_game_font = game;
    upload(&s_ov_tex, px);
}

int ngb_controls_hit(float x, float y) { return ctl_hit((x - OV_X) / OV_S, (y - OV_Y) / OV_S); }

/* Each frame of the Options menu (src/pc_menu.c's description picker): want
 * while the page that opens the panel is the one up. */
void ngb_controls_panel_tick(int want)
{
    if (!want) {
        if (s_ov_open > 0) {                           /* the menu went on without it */
            ngb_bind_capture_cancel();
            ngb_tab_give_pad();
        }
        s_ov_open = 0;
        return;
    }
    if (s_ov_open < 0)                                 /* closed: until the menu leaves the page */
        return;
    if (!s_ov_open) {
        if (!ov_ready())
            return;
        s_ov_open = 1;
        s_ov_focus = 1;
        s_ov_row = 0;
        s_ov_drawn = -1;
        s_ctl_col = 0;
        s_ctl_top = 0;
        ngb_tab_take_pad();
        play_sound(6);
    }
    int key = ngb_pause_tab_key(&s_ov_focus);
    if (ngb_bind_capturing()) {
        key = 0;
        if (!ngb_bind_capture_tick())
            play_sound(6);
    }
    if (key & K_ROW) {
        int r = key & 0xFFFF;
        if (r >= HIT_CTL)
            ctl_mouse(r, &s_ov_row);
        key = 0;
    }
    if (key)
        ctl_keys(key, &s_ov_row, 0);
    if (s_ov_row < A_COUNT && s_ctl_col == 2 && ngb_bind_pad_fixed(s_ov_row))
        s_ctl_col = 1;
    ctl_scroll(s_ov_row);
    if (!s_ov_focus) {                                 /* B: closed, and the menu goes back */
        s_ov_open = -1;
        ngb_bind_capture_cancel();
        ngb_tab_give_pad_back();
        play_sound(3);
        return;
    }
    int shown = ((s_ov_row + 1) & 0xFF) << 20 | (s_ctl_col & 3) << 18 | (s_ctl_top & 0x3F) << 12;   /* as the tab's */
    shown ^= (int)(ngb_bind_generation() & 0x7FF) << 1 ^ ngb_bind_capturing();
    if (shown != s_ov_drawn) {
        ov_draw();
        s_ov_drawn = shown;
    }
    sprite(OV_SPRITE, OV_X, OV_Y, 0x1F8, 0, OV_S, OV_S, 1.0f, 1.0f);   /* over the menu's plates */
}

int ngb_controls_panel_open(void) { return s_ov_open > 0; }

/* Debugging (NGB_SPRITE_RANGE_DUMP=lo-hi, hex ids): each sprite of the range's definition, and each
 * texture they use whole, as PPM in the working directory -- once. */
void ngb_sprite_range_dump(void)
{
    const char *e = getenv("NGB_SPRITE_RANGE_DUMP");
    static int done;
    unsigned lo, hi;
    if (!e || done || sscanf(e, "%x-%x", &lo, &hi) != 2)
        return;
    done = 1;
    uint32_t seen[64];
    int ns = 0;
    for (unsigned id = lo; id <= hi; id++) {
        uint32_t d = sprite_def(id);
        if (!d) {
            fprintf(stderr, "[SPRITES] %04X none\n", id);
            continue;
        }
        uint32_t ti = MEM32(d + 0x10);
        fprintf(stderr, "[SPRITES] %04X tex %X uv %.3f %.3f %.3f %.3f size %u %u\n", id, ti, MEMF(d), MEMF(d + 4),
                MEMF(d + 8), MEMF(d + 12), MEM16(d + 0x14), MEM16(d + 0x16));
        int k = 0;
        while (k < ns && seen[k] != ti) k++;
        if (k < ns || ns == 64)
            continue;
        seen[ns++] = ti;
        int w, h;
        uint32_t *px = read_texture(MEM32(0x8DBB28 + ti * 8), MEM32(0x8DBB2C + ti * 8), &w, &h);
        char path[64];
        snprintf(path, sizeof path, "tex_%03X.ppm", ti);
        FILE *f = px ? fopen(path, "wb") : NULL;
        if (f) {
            fprintf(f, "P6\n%d %d\n255\n", w, h);
            for (int i = 0; i < w * h; i++) {
                float a = (px[i] >> 24) / 255.0f;
                for (int c = 2; c >= 0; c--)
                    fputc((int)lroundf((px[i] >> (8 * c) & 255) * a + 40 * (1 - a)), f);
            }
            fclose(f);
        }
        free(px);
    }
}

/* ---------- the extra costumes' pictures ---------- */

/* The Costume Selection's pictures (src/costume_menu.c) for the costumes the game never offers, which
 * it has none of: its own default costume's picture (sprite 0x750) as a dark silhouette, the costume's
 * name across it in the game's letters. */
#define CP_SPRITE (PT_SPRITE + 12)    /* + 0..2: costumes 1-3 */
#define CP_TEX    (PT_TEX + 12)
static GuestTex s_cp_tex[3];
static uint32_t s_cp_entry[3], s_cp_def[3];
static int s_cp_failed;

uint32_t ngb_extra_costume_picture(int n)
{
    int i = n - 1;
    if (i < 0 || i > 2 || s_cp_failed)
        return 0;
    if (s_cp_def[i])
        return CP_SPRITE + i;
    uint32_t def = sprite_def(0x750);
    if (!def || (!s_font_body.data && !stbtt_InitFont(&s_font_body, ngb_font_body, 0)))
        return 0;                                      /* not loaded yet: later */
    uint32_t ti = MEM32(def + 0x10), hdr = MEM32(0x8DBB28 + ti * 8), pal = MEM32(0x8DBB2C + ti * 8);
    int w, h;
    uint32_t *src = hdr ? read_texture(hdr, pal, &w, &h) : NULL;
    if (!src) {
        s_cp_failed = 1;
        return 0;
    }
    int x0 = (int)lroundf(MEMF(def) * w), y0 = (int)lroundf(MEMF(def + 4) * h);
    int cw = (int)lroundf((MEMF(def + 8) - MEMF(def)) * w), ch = (int)lroundf((MEMF(def + 12) - MEMF(def + 4)) * h);
    int tw = 1, th = 1;
    while (tw < cw) tw <<= 1;
    while (th < ch) th <<= 1;
    if (cw <= 8 || ch <= 8 || tw > 1024 || th > 1024 || x0 < 0 || y0 < 0 || x0 + cw > w || y0 + ch > h) {
        free(src);
        s_cp_failed = 1;
        return 0;
    }
    uint32_t *px = calloc((size_t)tw * th, sizeof *px);
    uint8_t *m = calloc((size_t)tw * th, 1);
    for (int y = 0; y < ch; y++)                       /* the shape, dark, a little of its detail left */
        for (int x = 0; x < cw; x++) {
            uint32_t c = src[(y0 + y) * w + x0 + x];
            float l = luma(c) / 255.0f;
            px[y * tw + x] = (c & 0xFF000000u) | (mix(0x1C1C2A, 0x7A7896, l) & 0xFFFFFF);
        }
    free(src);
    int game = s_game_font;
    s_game_font = game_font_ready();
    char name[16];
    snprintf(name, sizeof name, "EXTRA %d", n);
    float size = ch * 0.085f, tx = text_width(&s_font_body, name, size, 1.0f);
    draw_text(m, tw, th, &s_font_body, name, size, 1.0f, (cw - tx) / 2, ch * 0.56f);
    s_game_font = game;
    for (int y = 2; y < th; y++)                       /* the dark edge under, then the letters */
        for (int x = 2; x < tw; x++) {
            float a = m[(y - 2) * tw + x - 2] / 255.0f;
            if (a > 0)
                px[y * tw + x] = 0xFF000000u | (mix(px[y * tw + x], 0x000000, a * 0.8f) & 0xFFFFFF);
        }
    for (int i2 = 0; i2 < tw * th; i2++)
        if (m[i2])
            px[i2] = 0xFF000000u | (mix(px[i2], 0xF0F0F0, m[i2] / 255.0f) & 0xFFFFFF);
    free(m);
    int ok = make_texture(&s_cp_tex[i], tw, th, 0x2A);
    if (ok)
        upload(&s_cp_tex[i], px);
    free(px);
    if (!ok) {
        s_cp_failed = 1;
        return 0;
    }
    s_cp_entry[i] = xbox_HeapAlloc(8, 8);
    MEM32(s_cp_entry[i]) = s_cp_tex[i].hdr;
    MEM32(s_cp_entry[i] + 4) = s_cp_tex[i].hdr;
    uint32_t d = xbox_HeapAlloc(0x18, 8);
    MEMF(d) = 0; MEMF(d + 4) = 0; MEMF(d + 8) = (float)cw / tw; MEMF(d + 12) = (float)ch / th;
    MEM32(d + 0x10) = CP_TEX + (uint32_t)i;
    MEM16(d + 0x14) = MEM16(def + 0x14);               /* drawn at the game's pictures' size */
    MEM16(d + 0x16) = MEM16(def + 0x16);
    s_cp_def[i] = d;
    return CP_SPRITE + i;
}

/* For the mouse: Yes (0) or No (1) at (x, y), or -1. */
int ngb_confirm_hit(float x, float y)
{
    x -= 360 - DLG_W / 2;
    y -= 240 - DLG_H / 2;
    if (y < 62 || y >= 98)
        return -1;
    if (x >= DLG_W * 0.34f - 56 && x < DLG_W * 0.34f + 56)
        return 0;
    if (x >= DLG_W * 0.66f - 56 && x < DLG_W * 0.66f + 56)
        return 1;
    return -1;
}

/* The main menu's EXIT TO DESKTOP (src/main_menu.c): the box, each frame
 * of the main menu's drawer; Yes closes the game. */
static int s_mm_exit;
void ngb_main_menu_exit(void)
{
    if (!s_mm_exit && ngb_confirm_open("Exit to Desktop?", 0))
        s_mm_exit = 1;
}

int ngb_main_menu_exit_tick(void)
{
    if (!s_mm_exit)
        return 0;
    int r = ngb_confirm_tick();
    if (r < 0)
        return 1;                                       /* the box has the pad */
    s_mm_exit = 0;
    if (r == 1)
        ngb_quit_to_desktop();
    else
        ngb_tab_give_pad();
    return 0;
}

/* ---------- Exit's page ---------- */

/* Return to Game (the game's own Exit: the menu closed, as Start does),
 * Return to Main Menu (the game has no way back from play but the console's
 * reset: the program starts again, at the main menu -- src/ingame_menu.c)
 * and Exit to Desktop; the last two after a second A. */
void ngb_pause_resume(void);          /* src/ingame_menu.c */
void ngb_restart_to_menu(void);
void ngb_quit_to_desktop(void);

#define EXIT_ROWS 3
static const char *const s_exit_label[EXIT_ROWS] = { "Return to Game", "Return to Main Menu", "Exit to Desktop" };
static const char *const s_exit_help[EXIT_ROWS] = {
    "Closes this menu.",
    "Back to the main menu. Unsaved progress is lost.",
    "Closes the game. Unsaved progress is lost.",
};
static const char *const s_exit_q[EXIT_ROWS] = { "", "Return to the Main Menu?", "Exit to Desktop?" };
static int s_exit_row, s_exit_armed = -1;   /* (no longer armed: the box asks) */

static void draw_exit_page(int focused, int row, int armed)
{
    static uint32_t px[PAGE_TW * PAGE_TH];
    static uint8_t m[PAGE_TW * PAGE_TH];
    memset(px, 0, sizeof px);
    for (int i = 0; i < EXIT_ROWS; i++) {
        float y = 12 + i * (ROW_H + 4);
        int sel = focused && i == row;
        if (sel) {
            page_fill(px, 0, y + 2, 10, ROW_H - 4, 0xE6E4FF, 0.95f, 0.85f);
            page_fill(px, 10, y + 2, 440, ROW_H - 4, 0x6E64DC, 0.85f, 0.05f);
        }
        page_text(px, m, s_exit_label[i], 20, y + 19, 20,
                  sel ? 0xF5EB5A : focused ? 0xE6E6EB : 0xA0A0A8, 0);
    }
    if (focused && row >= 0 && row < EXIT_ROWS && !armed)   /* (not under the box: it showed through it) */
        page_text(px, m, s_exit_help[row], 445, 12 + EXIT_ROWS * (ROW_H + 4) + 18, 15, 0xFFFFFF, 2);
    upload(&s_tex[2], px);
}

static void pt_exit_draw(void)
{
    if (ngb_confirm_is_open()) {                       /* Are you sure? (over the page, without its help line) */
        if (s_drawn.page != 1 || s_drawn.row != -2) {
            draw_exit_page(1, s_exit_row, 1);
            s_drawn.page = 1;
            s_drawn.row = -2;
        }
        sprite(PT_SPRITE + 2, PAGE_X, PAGE_Y, 0x1CC, 0, 1.0f, 1.0f, 1.0f, 1.0f);
        int r = ngb_confirm_tick();
        if (r == 1) {
            if (s_exit_row == 1)
                ngb_restart_to_menu();
            else
                ngb_quit_to_desktop();
        } else if (r == 0 && s_dlg_back) {
            ngb_tab_take_pad();                        /* B: the page again, still with the pad */
        } else if (r == 0 && !s_dlg_focus) {
            s_focus = 0;                               /* L/R: to the next tab */
        }
        return;
    }
    int was = s_focus;
    int key = ngb_pause_tab_key(&s_focus);
    if (key == K_CLEAR)
        key = K_OK;                                   /* X chooses, as in the game's menus (#111) */
    if (key & K_ROW) {                                /* the mouse on a row */
        int r = key & 0xFFFF;
        if (r < EXIT_ROWS && r != s_exit_row) {
            s_exit_row = r;
            s_exit_armed = -1;
            play_sound(1);
        }
        key = 0;
    }
    if (s_focus && !was) {
        s_exit_row = 0;                               /* in: on Return to Game */
        s_exit_armed = -1;
    }
    if (!s_focus)
        s_exit_armed = -1;
    switch (key) {
    case K_UP:
    case K_DOWN:
        s_exit_row = (s_exit_row + (key == K_UP ? EXIT_ROWS - 1 : 1)) % EXIT_ROWS;
        s_exit_armed = -1;
        play_sound(1);
        break;
    case K_OK:
        play_sound(6);
        if (s_exit_row == 0) {
            s_focus = 0;
            ngb_pause_resume();
        } else {
            ngb_confirm_open(s_exit_q[s_exit_row], 1);
        }
        break;
    }
    if (s_focus != was && !(key == K_OK && s_exit_row == 0))
        play_sound(s_focus ? 6 : 3);
    int shown = s_exit_row * 4 + s_exit_armed + 1;
    if (s_drawn.page != 1 || s_focus != s_drawn.focused || shown != s_drawn.row) {
        draw_exit_page(s_focus, s_exit_row, 0);
        s_drawn.page = 1;
        s_drawn.focused = s_focus;
        s_drawn.row = shown;
    }
    sprite(PT_SPRITE + 2, PAGE_X, PAGE_Y, 0x1CC, 0, 1.0f, 1.0f, 1.0f, 1.0f);
}

/* What the mouse points at: 0 the Settings page, 1 Exit's, 2 the box. */
int ngb_pause_tab_page(void) { return s_dlg_open ? 2 : s_ov_open > 0 ? 3 : s_drawn.page == 1 ? 1 : 0; }

/* For the mouse: Exit's page's row at (x, y), or -1. */
int ngb_pause_exit_hit(float x, float y)
{
    x -= PAGE_X;
    y -= PAGE_Y;
    if (x < 0 || x >= PAGE_W || y < 12 || y >= 12 + EXIT_ROWS * (ROW_H + 4))
        return -1;
    return (int)((y - 12) / (ROW_H + 4));
}

/* ---------- the engine's tables ---------- */

/* A row table with one more row: Exit's, the last; the row before it,
 * where Exit was, is Options, doing nothing when chosen (so the engine
 * opens no page under the tabs for it). */
static uint8_t s_exit_action;

static uint32_t copy_rows(uint32_t src)
{
    int n = 0;
    while (MEM8(src + 0x1C * n) != 0xFF)
        n++;
    uint32_t va = xbox_HeapAlloc(0x1C * (n + 2), 4);
    memcpy((void *)XBOX_PTR(va), (void *)XBOX_PTR(src), 0x1C * (n + 1));
    uint32_t e = va + 0x1C * n;
    memcpy((void *)XBOX_PTR(e), (void *)XBOX_PTR(src + 0x1C * (n - 1)), 0x1C);
    MEM8(e) = (uint8_t)n;                             /* Exit, now the last */
    s_exit_action = MEM8(e + 1);
    memset((void *)XBOX_PTR(e + 0x1C), 0, 0x1C);
    MEM8(e + 0x1C) = 0xFF;
    return va;
}

/* Before the game starts: the pause menu's pages, the tab rows with the
 * seventh. Their last row stays 5 until the textures are made. */
static uint32_t s_pages;

void ngb_pause_tab_init(void)
{
    if (getenv("NGB_NO_SETTINGS_TAB"))  /* the game's six tabs */
        return;
    int n = 0;
    while (MEM8(PAUSE_PAGES + 0x18 * n) != 0xFF)
        n++;
    s_pages = xbox_HeapAlloc(0x18 * (n + 1), 4);
    memcpy((void *)XBOX_PTR(s_pages), (void *)XBOX_PTR(PAUSE_PAGES), 0x18 * (n + 1));
    uint32_t src[4] = { 0 }, dst[4] = { 0 };
    for (int i = 0; i < n; i++) {
        uint32_t p = s_pages + 0x18 * i;
        if (MEM8(p) != 0 && MEM8(p) != 1 && MEM8(p) != 0x0E)
            continue;
        int k = 0;
        while (k < 4 && src[k] && src[k] != MEM32(p + 0xC))
            k++;
        if (k < 4 && !src[k]) {
            src[k] = MEM32(p + 0xC);
            dst[k] = copy_rows(src[k]);
        }
        if (k < 4)
            MEM32(p + 0xC) = dst[k];
    }
    for (int m = 0; m < 16; m++)
        if (MEM32(MENU_TABLES + 4 * m) == PAUSE_PAGES)
            MEM32(MENU_TABLES + 4 * m) = s_pages;
    s_enabled = 1;
}

/* The tab row's last row: 6 once there is a seventh tab to show; the row
 * before it Options (nothing: 0xFF, the engine opens no page) then, Exit's
 * own (close) while there is none. */
static void pt_rows(void)
{
    if (!s_pages)
        return;
    int seven = pt_tabs() == 7, last = pt_tabs() - 1;
    for (uint32_t p = s_pages; MEM8(p) != 0xFF; p += 0x18)
        if (MEM8(p) == 0 || MEM8(p) == 1 || MEM8(p) == 0x0E) {
            MEM8(p + 2) = (uint8_t)last;
            MEM8(MEM32(p + 0xC) + 0x1C * 5 + 1) = seven ? 0xFF : s_exit_action;
            if (seven)                                /* Exit: our page's, A does nothing */
                MEM8(MEM32(p + 0xC) + 0x1C * 6 + 1) = 0xFF;
        }
}


/* ---------- the lifted functions ---------- */

/**
 * sub_00182240
 * Original: 0x00182240 - 0x001823D3 (403 bytes, 119 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_00182240(void)
{
    RECOMP_ENTRY(0x00182240u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_00182240: ;
    _fb = (uint32_t)(0x48) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x48;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0 (32-bit) */
    eax = MEM32(0x41CCD8);
    PUSH32(esp, ebx);
    ebx = 2;
    MEM32(esp + 0x48) = eax;
    PUSH32(esp, esi);
    MEM8(esp + 0x44) = LO8(ebx);
    MEM8(esp + 0x45) = LO8(ebx);
    MEM8(esp + 0x46) = LO8(ebx);
    MEM8(esp + 0x47) = LO8(ebx);
    MEM8(esp + 0x48) = LO8(ebx);
    MEM8(esp + 0x49) = LO8(ebx);
    pt_rows();                                       /* Settings: */
    if (pt_tabs() == 7) MEM8(esp + 0x4A) = LO8(ebx); /* Settings: a seventh */
    if (CMP_EQ(_fa, _fb)) goto loc_00182278; /* je: equal / zero */

loc_0018226F: ;
    eax = ZX8(MEM8(edx + 1));
    MEM8(esp + eax + 0x44) = 3;

loc_00182278: ;
    SET_LO8(eax, MEM8(edx + 1));
    ecx = esp + 0x44;
    MEM32(esp + 0x10) = ecx;
    ecx = ZX8(LO8(eax));
    MEM32(esp + 0x20) = ecx;
    SET_LO8(ecx, MEM8(edx + 2));
    esi = ZX8(LO8(ecx));
    MEM32(esp + 0x24) = esi;
    esi = MEM32(0xDA41C8);
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 1 (32-bit) */
    MEM16(esp + 0x36) = LO16(ebx);
    MEM16(esp + 0x38) = 0;
    MEM32(esp + 0x28) = (uint32_t)pt_tabs();         /* Settings: was 6 */
    if (CMP_NE(_fa, _fb)) goto loc_001822CE; /* jne: not equal / not zero */

loc_001822B3: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(4) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 4 (8-bit) */
    SET_LO16(ecx, LO16(ebx));
    MEM16(esp + 0x3E) = LO16(ecx);
    if (CMP_NE(_fa, _fb)) goto loc_0018232A; /* jne: not equal / not zero */

loc_001822BF: ;
    SET_LO8(eax, MEM8(0xDA420A));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0018232A; /* je: equal / zero */

loc_001822C8: ;
    SET_LO16(ecx, 0x11);
    goto loc_00182325;

loc_001822CE: ;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(3) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 3 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001822D8; /* jne: not equal / not zero */

loc_001822D3: ;
    SET_LO16(ecx, LO16(esi));
    goto loc_00182325;

loc_001822D8: ;
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(LO8(eax)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), LO8(eax) (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001822E2; /* je: equal / zero */

loc_001822DC: ;
    SET_LO16(ecx, 9);
    goto loc_00182325;

loc_001822E2: ;
    _fa = (uint32_t)(MEM8(edx + 0xC)) & 0xFFu; _fb = (uint32_t)(0xFF) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(edx + 0xC), 0xFF (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001822EE; /* je: equal / zero */

loc_001822E8: ;
    SET_LO16(ecx, 5);
    goto loc_00182325;

loc_001822EE: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00182322; /* je: equal / zero */

loc_001822F2: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(4) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 4 (8-bit) */
    SET_LO8(ecx, MEM8(0xDA420B));
    SET_LO8(edx, MEM8(0xDA420A));
    if (CMP_NE(_fa, _fb)) goto loc_00182322; /* jne: not equal / not zero */

loc_00182302: ;
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(LO8(edx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), LO8(edx) (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00182310; /* je: equal / zero */

loc_00182306: ;
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), 0 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00182310; /* jne: not equal / not zero */

loc_0018230A: ;
    SET_LO16(ecx, 0xB);
    goto loc_00182325;

loc_00182310: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(4) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 4 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00182322; /* jne: not equal / not zero */

loc_00182314: ;
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(LO8(edx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), LO8(edx) (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00182322; /* je: equal / zero */

loc_00182318: ;
    _fa = (uint32_t)(LO8(edx)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(edx), 0 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00182322; /* jne: not equal / not zero */

loc_0018231C: ;
    SET_LO16(ecx, 0xC);
    goto loc_00182325;

loc_00182322: ;
    SET_LO16(ecx, 0); /* xor self */
    _fa = (uint32_t)(LO16(ecx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* xor result */

loc_00182325: ;
    MEM16(esp + 0x3E) = LO16(ecx);

loc_0018232A: ;
    edx = esp + 8;
    PUSH32(esp, edx);
    MEM16(esp + 0x38) = (uint16_t)pt_tabs();         /* Settings: was 6 */
    MEM32(esp + 0x30) = 0;
    MEM16(esp + 0xC) = LO16(ecx);
    MEM16(esp + 0xE) = LO16(ebx);
    PUSH32(esp, 0x0018234Du); RECOMP_ABI_CALL(0x0020B1D0u, sub_0020B1D0); /* call 0x0020B1D0 */

loc_0018234D: ;
    eax = esp + 0x14;
    PUSH32(esp, eax);
    PUSH32(esp, 0x00182357u); RECOMP_ABI_CALL(0x0020CDA0u, sub_0020CDA0); /* call 0x0020CDA0 */

loc_00182357: ;
    ecx = MEM32(0xDA41E8);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    POP32(esp, esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 1 (32-bit) */
    eax = MEM32(0x42A8E4);
    POP32(esp, ebx);
    if (CMP_NE(_fa, _fb)) goto loc_001823A3; /* jne: not equal / not zero */

loc_0018236C: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test eax, ecx (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_0018237A; /* je: equal / zero */

loc_00182370: ;
    ecx = MEM32(0xDA4218);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test ecx, eax (32-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_001823C6; /* jne: not equal / not zero */

loc_0018237A: ;
    edx = MEM32(0xDA41E8);
    ecx = MEM32(0xDA4218);
    edx = edx | eax;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    ecx = ecx | eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(0xDA41E8) = edx;
    MEM32(0xDA4218) = ecx;
    ecx = MEM32(esp + 0x44);
    PUSH32(esp, 0x0018239Fu); RECOMP_ABI_CALL(0x00379C29u, sub_00379C29); /* call 0x00379C29 */

loc_0018239F: ;
    _fb = (uint32_t)(0x48) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x48;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 4; return; /* ret */

loc_001823A3: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test eax, ecx (32-bit) */
    ecx = MEM32(0x42A904);
    if (TEST_Z(_fa, _fb)) goto loc_001823B3; /* je: equal / zero */

loc_001823AD: ;
    MEM32(0xDA41E8) = MEM32(0xDA41E8) & ecx;
    _fa = (uint32_t)(MEM32(0xDA41E8)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */

loc_001823B3: ;
    edx = MEM32(0xDA4218);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test eax, edx (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_001823C6; /* je: equal / zero */

loc_001823BD: ;
    eax = edx;
    eax = eax & ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(0xDA4218) = eax;

loc_001823C6: ;
    ecx = MEM32(esp + 0x44);
    PUSH32(esp, 0x001823CFu); RECOMP_ABI_CALL(0x00379C29u, sub_00379C29); /* call 0x00379C29 */

loc_001823CF: ;
    _fb = (uint32_t)(0x48) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x48;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 4; return; /* ret */

}

/**
 * sub_0020CDA0
 * Original: 0x0020CDA0 - 0x0020D618 (2168 bytes, 487 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: standard_frame
 */
void sub_0020CDA0(void)
{
    RECOMP_ENTRY(0x0020CDA0u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* prologue saves caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    #define fp_push(v) do { double _fp_value = (v); \
        g_fp_top = (g_fp_top + 7u) & 7u; \
        g_fp_stack[g_fp_top] = _fp_value; } while (0)
    #define fp_pop() (g_fp_top = (g_fp_top + 1u) & 7u)
    #define fp_top() g_fp_stack[g_fp_top]
    #define fp_st(i) g_fp_stack[(g_fp_top + (i)) & 7u]
    #define fp_st1() fp_st(1)

loc_0020CDA0: ;
    PUSH32(esp, ebp);
    ebp = esp;
    g_ebp = ebp; /* publish frame for frameless callees */
    g_seh_ebp = ebp;
    esp = esp & 0xFFFFFFF0u;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fb = (uint32_t)(0xA4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0xA4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, ebx);
    ebx = MEM32(ebp + 8);
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    eax = ebx;
    esi = 0x8DB948;
    MEM32(esp + 0x98) = 0x17B;
    MEM32(esp + 0x9C) = 0x17A;
    MEM32(esp + 0xA0) = 0x178;
    MEM32(esp + 0xA4) = 0x17C;
    MEM32(esp + 0xA8) = 0x179;
    MEM32(esp + 0xAC) = 0x177;
    MEM32(esp + 0x80) = 0x181;
    MEM32(esp + 0x84) = 0x180;
    MEM32(esp + 0x88) = 0x17E;
    MEM32(esp + 0x8C) = 0x182;
    MEM32(esp + 0x90) = 0x17F;
    MEM32(esp + 0x94) = 0x17D;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020CE42u); RECOMP_ABI_CALL(0x0020C6F0u, sub_0020C6F0); /* call 0x0020C6F0 */

loc_0020CE42: ;

    esi = MEM32(0x8DB970);
    esi++;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(esp + 0x54) = eax;
    eax = MEM32(0x8DB844);
    MEM32(0x8DB970) = esi;
    ecx = ZX16(MEM16(ebx + 0x24));
    edi = 0; /* xor self */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    MEM32(0x8DB8AC) = eax;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, edi (32-bit) */
    MEM32(0x8DB844) = edi;
    if (CMP_BE(_fa, _fb)) goto loc_0020CE86; /* jbe: below or equal (unsigned <=) */

loc_0020CE6F: ;
    edx = MEM32(ebx);

loc_0020CE71: ;
    _fa = (uint32_t)(MEM8(edx + eax)) & 0xFFu; _fb = (uint32_t)(3) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(edx + eax), 3 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0020CE81; /* jne: not equal / not zero */

loc_0020CE77: ;
    MEM32(0x8DB844) = 1;

loc_0020CE81: ;
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ecx (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_0020CE71; /* jb: below (unsigned <) */

loc_0020CE86: ;
    _fa = (uint32_t)(MEM32(ebx + 0x10)) & 0xFFFFFFFFu; _fb = EXIT_TAB; /* Settings: Exit's row */
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(ebx + 0x10), 5 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0020CE92; /* jne: not equal / not zero */

loc_0020CE8C: ;
    MEM32(0x8DB8B0) = edi;

loc_0020CE92: ;
    SET_LO16(ebx, MEM16(0x8DB962));
    xmm1 = XMM_ZERO(); /* xorps self = zero */
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    eax = MEM32(0x8DB934);
    _fb = (uint32_t)(0x2C) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(ebx, LO16(ebx) + 0x2C);
    _fa = (uint32_t)(LO16(ebx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edi (32-bit) */
    MEMF(esp + 0x34) = xmm1.f[0]; /* movss */
    xmm1 = XMM_SCALAR(MEMF(0x414BF4)); /* movss */
    MEM32(esp + 0x10) = 0x16F;
    MEM16(esp + 0x14) = 0x34;
    MEM16(esp + 0x16) = LO16(ebx);
    MEM16(esp + 0x18) = 0x1D6;
    MEM8(esp + 0x1A) = 0;
    MEM8(esp + 0x1B) = 1;
    MEM8(esp + 0x1C) = 1;
    MEM32(esp + 0x20) = edi;
    MEMF(esp + 0x24) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x28) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x2C) = xmm0.f[0]; /* movss */
    MEM32(esp + 0x38) = 0xFFFFFFFFu;
    MEMF(esp + 0x30) = xmm1.f[0]; /* movss */
    if (CMP_NE(_fa, _fb)) goto loc_0020CF1B; /* jne: not equal / not zero */

loc_0020CF0D: ;
    xmm1 = XMM_SCALAR(MEMF(0x414AB4)); /* movss */
    MEMF(esp + 0x2C) = xmm1.f[0]; /* movss */

loc_0020CF1B: ;
    _fa = (uint32_t)(MEM32(0x43B490)) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0x43B490), 1 (32-bit) */
    ecx = MEM32(0x8F7DFC);
    if (CMP_NE(_fa, _fb)) goto loc_0020CF81; /* jne: not equal / not zero */

loc_0020CF2A: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(0x8F7DF8)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, MEM32(0x8F7DF8) (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_0020CF81; /* jae: above or equal (unsigned >=) */

loc_0020CF32: ;
    eax = MEM32(0x8F7DF4);
    esi = ecx;
    esi = (uint32_t)((int32_t)esi * (int32_t)0x70);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + eax;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = MEM32(0x8F7DF0);
    ecx = ecx << 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, edi);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, edi);
    eax = esp + 0x18;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020CF53u); RECOMP_ABI_CALL(0x002216B0u, sub_002216B0); /* call 0x002216B0 */

loc_0020CF53: ;
    ecx = MEM32(0x8F7DFC);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0020CF79; /* je: equal / zero */

loc_0020CF60: ;
    eax = 0x1D6;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020CF6Cu); RECOMP_ABI_CALL(0x00221170u, sub_00221170); /* call 0x00221170 */

loc_0020CF6C: ;
    ecx = MEM32(0x8F7DFC);
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x8F7DFC) = ecx;

loc_0020CF79: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */

loc_0020CF81: ;
    xmm1 = XMM_ZERO(); /* xorps self = zero */
    _fa = (uint32_t)(MEM32(0x8DB934)) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0x8DB934), edi (32-bit) */
    MEMF(esp + 0x34) = xmm1.f[0]; /* movss */
    xmm1 = XMM_SCALAR(MEMF(0x414BF4)); /* movss */
    MEM32(esp + 0x10) = 0x170;
    MEM16(esp + 0x14) = 0x27C;
    MEM16(esp + 0x16) = LO16(ebx);
    MEM16(esp + 0x18) = 0x1D6;
    MEM8(esp + 0x1A) = 0;
    MEM8(esp + 0x1B) = 1;
    MEM8(esp + 0x1C) = 1;
    MEM32(esp + 0x20) = edi;
    MEMF(esp + 0x24) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x28) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x2C) = xmm0.f[0]; /* movss */
    MEM32(esp + 0x38) = 0xFFFFFFFFu;
    MEMF(esp + 0x30) = xmm1.f[0]; /* movss */
    if (CMP_NE(_fa, _fb)) goto loc_0020CFF6; /* jne: not equal / not zero */

loc_0020CFE8: ;
    xmm1 = XMM_SCALAR(MEMF(0x414AB4)); /* movss */
    MEMF(esp + 0x2C) = xmm1.f[0]; /* movss */

loc_0020CFF6: ;
    _fa = (uint32_t)(MEM32(0x43B490)) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0x43B490), 1 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0020D058; /* jne: not equal / not zero */

loc_0020CFFF: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(0x8F7DF8)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, MEM32(0x8F7DF8) (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_0020D058; /* jae: above or equal (unsigned >=) */

loc_0020D007: ;
    edx = MEM32(0x8F7DF4);
    ebx = MEM32(0x8F7DF0);
    esi = ecx;
    esi = (uint32_t)((int32_t)esi * (int32_t)0x70);
    ecx = ecx << 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, edi);
    PUSH32(esp, edi);
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + edx;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(ebx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + ebx;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = esp + 0x18;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020D02Au); RECOMP_ABI_CALL(0x002216B0u, sub_002216B0); /* call 0x002216B0 */

loc_0020D02A: ;
    ecx = MEM32(0x8F7DFC);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0020D050; /* je: equal / zero */

loc_0020D037: ;
    eax = 0x1D6;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020D043u); RECOMP_ABI_CALL(0x00221170u, sub_00221170); /* call 0x00221170 */

loc_0020D043: ;
    ecx = MEM32(0x8F7DFC);
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x8F7DFC) = ecx;

loc_0020D050: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */

loc_0020D058: ;
    xmm1 = XMM_ZERO(); /* xorps self = zero */
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO16(ebx, MEM16(0x8DB962));
    _fb = (uint32_t)(0x16) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(ebx, LO16(ebx) + 0x16);
    _fa = (uint32_t)(LO16(ebx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    eax = 1;
    MEM32(0x8DB934) = eax;
    MEM32(esp + 0x10) = 0x171;
    MEM16(esp + 0x14) = LO16(edi);
    MEM16(esp + 0x18) = 0x1CC;
    MEM8(esp + 0x1A) = 0;
    edx = ebx + -4;
    MEM16(esp + 0x16) = LO16(edx);
    _fa = (uint32_t)(MEM32(0x43B490)) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0x43B490), eax (32-bit) */
    MEM8(esp + 0x1B) = LO8(eax);
    MEM8(esp + 0x1C) = LO8(eax);
    MEM32(esp + 0x20) = edi;
    MEMF(esp + 0x24) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x28) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x2C) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x30) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x34) = xmm1.f[0]; /* movss */
    MEM32(esp + 0x38) = 0xFFFFFFFFu;
    if (CMP_NE(_fa, _fb)) goto loc_0020D124; /* jne: not equal / not zero */

loc_0020D0CD: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(0x8F7DF8)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, MEM32(0x8F7DF8) (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_0020D124; /* jae: above or equal (unsigned >=) */

loc_0020D0D5: ;
    eax = MEM32(0x8F7DF4);
    esi = ecx;
    esi = (uint32_t)((int32_t)esi * (int32_t)0x70);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + eax;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = MEM32(0x8F7DF0);
    ecx = ecx << 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, edi);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, edi);
    eax = esp + 0x18;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020D0F6u); RECOMP_ABI_CALL(0x002216B0u, sub_002216B0); /* call 0x002216B0 */

loc_0020D0F6: ;
    ecx = MEM32(0x8F7DFC);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0020D11C; /* je: equal / zero */

loc_0020D103: ;
    eax = 0x1CC;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020D10Fu); RECOMP_ABI_CALL(0x00221170u, sub_00221170); /* call 0x00221170 */

loc_0020D10F: ;
    ecx = MEM32(0x8F7DFC);
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x8F7DFC) = ecx;

loc_0020D11C: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */

loc_0020D124: ;
    xmm1 = XMM_ZERO(); /* xorps self = zero */
    eax = ebx + 0xA;
    MEM16(esp + 0x16) = LO16(eax);
    _fa = (uint32_t)(MEM32(0x43B490)) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0x43B490), 1 (32-bit) */
    MEMF(esp + 0x34) = xmm1.f[0]; /* movss */
    xmm1 = XMM_SCALAR(MEMF(0x415E78)); /* movss */
    MEM32(esp + 0x10) = 0x172;
    MEM16(esp + 0x14) = 0xFF;
    MEM16(esp + 0x18) = 0x1CC;
    MEM8(esp + 0x1A) = 0;
    MEM8(esp + 0x1B) = 1;
    MEM8(esp + 0x1C) = 1;
    MEM32(esp + 0x20) = edi;
    MEMF(esp + 0x28) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x2C) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x30) = xmm0.f[0]; /* movss */
    MEM32(esp + 0x38) = 0xFFFFFFFFu;
    MEMF(esp + 0x24) = xmm1.f[0]; /* movss */
    if (CMP_NE(_fa, _fb)) goto loc_0020D1E6; /* jne: not equal / not zero */

loc_0020D18F: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(0x8F7DF8)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, MEM32(0x8F7DF8) (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_0020D1E6; /* jae: above or equal (unsigned >=) */

loc_0020D197: ;
    eax = MEM32(0x8F7DF4);
    esi = ecx;
    esi = (uint32_t)((int32_t)esi * (int32_t)0x70);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + eax;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = MEM32(0x8F7DF0);
    ecx = ecx << 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, edi);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, edi);
    eax = esp + 0x18;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020D1B8u); RECOMP_ABI_CALL(0x002216B0u, sub_002216B0); /* call 0x002216B0 */

loc_0020D1B8: ;
    ecx = MEM32(0x8F7DFC);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0020D1DE; /* je: equal / zero */

loc_0020D1C5: ;
    eax = 0x1CC;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020D1D1u); RECOMP_ABI_CALL(0x00221170u, sub_00221170); /* call 0x00221170 */

loc_0020D1D1: ;
    ecx = MEM32(0x8F7DFC);
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x8F7DFC) = ecx;

loc_0020D1DE: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */

loc_0020D1E6: ;
    eax = MEM32(0x43B490);
    MEMF(esp + 0x28) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x2C) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x30) = xmm0.f[0]; /* movss */
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    _fb = (uint32_t)(0x4A) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ebx = ebx + 0x4A;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 1 (32-bit) */
    MEMF(esp + 0x34) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x415E78)); /* movss */
    MEM32(esp + 0x10) = 0x173;
    MEM16(esp + 0x14) = 0xFF;
    MEM16(esp + 0x16) = LO16(ebx);
    MEM16(esp + 0x18) = 0x1CC;
    MEM8(esp + 0x1A) = 0;
    MEM8(esp + 0x1B) = 1;
    MEM8(esp + 0x1C) = 1;
    MEM32(esp + 0x20) = edi;
    MEM32(esp + 0x38) = 0xFFFFFFFFu;
    MEMF(esp + 0x24) = xmm0.f[0]; /* movss */
    if (CMP_NE(_fa, _fb)) goto loc_0020D29C; /* jne: not equal / not zero */

loc_0020D252: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(0x8F7DF8)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, MEM32(0x8F7DF8) (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_0020D29C; /* jae: above or equal (unsigned >=) */

loc_0020D25A: ;
    edx = MEM32(0x8F7DF4);
    ebx = MEM32(0x8F7DF0);
    esi = ecx;
    esi = (uint32_t)((int32_t)esi * (int32_t)0x70);
    ecx = ecx << 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, edi);
    PUSH32(esp, edi);
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + edx;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(ebx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + ebx;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = esp + 0x18;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020D27Du); RECOMP_ABI_CALL(0x002216B0u, sub_002216B0); /* call 0x002216B0 */

loc_0020D27D: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0020D29C; /* je: equal / zero */

loc_0020D284: ;
    ecx = MEM32(0x8F7DFC);
    eax = 0x1CC;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020D296u); RECOMP_ABI_CALL(0x00221170u, sub_00221170); /* call 0x00221170 */

loc_0020D296: ;
    MEM32(0x8F7DFC) = MEM32(0x8F7DFC) + 1;
    _fa = (uint32_t)(MEM32(0x8F7DFC)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_0020D29C: ;
    SET_LO16(eax, MEM16(0x8DB962));
    SET_LO16(ecx, MEM16(0x977680));
    _fb = (uint32_t)(0xA) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(eax, LO16(eax) + 0xA);
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    MEM16(esp + 0x6A) = LO16(eax);
    eax = 0x80FFFFFFu;
    esi = esp + 0x58;
    ebx = 0x8F7DF0;
    MEM16(esp + 0x68) = LO16(edi);
    MEM16(esp + 0x6C) = LO16(ecx);
    MEM16(esp + 0x6E) = 0x17;
    MEM8(esp + 0x74) = 0;
    MEM32(esp + 0x78) = edi;
    MEM16(esp + 0x70) = LO16(edi);
    MEM32(esp + 0x5C) = 0xFFFFFF;
    MEM32(esp + 0x60) = eax;
    MEM32(esp + 0x64) = eax;
    MEM32(esp + 0x7C) = 1;
    MEM32(esp + 0x58) = eax;
    MEM16(esp + 0x72) = 0x1CB;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020D307u); RECOMP_ABI_CALL(0x00222250u, sub_00222250); /* call 0x00222250 */

loc_0020D307: ;
    SET_LO16(eax, MEM16(0x8DB962));
    ecx = MEM32(ebp + 8);
    _fb = (uint32_t)(MEM16(0x43A038)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(eax, LO16(eax) + MEM16(0x43A038));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    edi = 0; /* xor self */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(MEM16(ecx + 0x24)) & 0xFFFFu; _fb = (uint32_t)(LO16(edi)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(ecx + 0x24), LO16(edi) (16-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_0020D5EA; /* jbe: below or equal (unsigned <=) */

loc_0020D323: ;
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    ebx = MEM32(0x8F7DFC);
    MEMF(esp + 0x34) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    MEM16(esp + 0x16) = LO16(eax);
    MEM8(esp + 0x1B) = 1;
    MEM8(esp + 0x1C) = 1;
    MEM32(esp + 0x20) = edi;
    MEM32(esp + 0x38) = 0xFFFFFFFFu;
    MEMF(esp + 0x30) = xmm0.f[0]; /* movss */
    MEM8(esp + 0x1A) = 0x10;
    MEM32(esp + 0x40) = 0x8DB980;
    MEM32(esp + 0x4C) = 0x38;
    goto loc_0020D380;

loc_0020D372: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    /* nop */

loc_0020D380: ;
    eax = MEM32(ecx + 0x10);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edi (32-bit) */
    MEMF(esp + 0x48) = xmm0.f[0]; /* movss */
    MEM32(esp + 0x3C) = 0;
    if (CMP_NE(_fa, _fb)) goto loc_0020D39D; /* jne: not equal / not zero */

loc_0020D395: ;
    MEMF(esp + 0x44) = xmm0.f[0]; /* movss */
    goto loc_0020D3BD;

loc_0020D39D: ;
    ecx = MEM32(0x8DB844);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    xmm0 = XMM_SCALAR(MEMF(0x414BF4)); /* movss */
    MEMF(esp + 0x44) = xmm0.f[0]; /* movss */
    if (CMP_EQ(_fa, _fb)) goto loc_0020D3C3; /* je: equal / zero */

loc_0020D3B5: ;
    xmm0 = XMM_SCALAR(MEMF(0x414BB4)); /* movss */

loc_0020D3BD: ;
    MEMF(esp + 0x48) = xmm0.f[0]; /* movss */

loc_0020D3C3: ;
    _fa = (uint32_t)(MEM32(esp + 0x4C)) & 0xFFFFFFFFu; _fb = (uint32_t)(0x38) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(esp + 0x4C), 0x38 (32-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_0020D3EC; /* jbe: below or equal (unsigned <=) */

loc_0020D3CA: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edi (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_0020D3EC; /* jb: below (unsigned <) */

loc_0020D3CE: ;
    edx = (uint32_t)(int32_t)SMEM16(0x43A03C);
    MEM32(esp + 0x3C) = edx;
    fp_push((double)SMEM32(esp + 0x3C)); /* fild */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMD(0x415E70)); /* fmul qword ptr [0x415e70] */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020D3E8u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_0020D3E8: ;
    MEM32(esp + 0x3C) = eax;

loc_0020D3EC: ;
    eax = pt_tabs() == 7 && edi == PT_TAB ? PT_SPRITE : MEM32(esp + (pt_tabs() == 7 && edi == 6 ? 5 : edi) * 4 + 0x98); /* Settings: */
    ecx = MEM32(esp + 0x40);
    SET_LO16(edx, (uint16_t)pt_tab_x((int)edi, (int16_t)MEM16(ecx))); /* Settings: */
    _fb = (uint32_t)(MEM16(esp + 0x3C)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(edx, LO16(edx) + MEM16(esp + 0x3C));
    _fa = (uint32_t)(LO16(edx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x48)); /* movss */
    MEM32(esp + 0x10) = eax;
    _fa = (uint32_t)(MEM32(0x43B490)) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0x43B490), 1 (32-bit) */
    MEMF(esp + 0x2C) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x44)); /* movss */
    MEM16(esp + 0x14) = LO16(edx);
    MEM16(esp + 0x18) = 0x1CD;
    MEMF(esp + 0x28) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x24) = xmm0.f[0]; /* movss */
    if (pt_tabs() == 7) { MEMF(esp + 0x24) *= PT_SCALE; MEMF(esp + 0x28) *= PT_SCALE; } /* Settings: */
    if (CMP_NE(_fa, _fb)) goto loc_0020D4C8; /* jne: not equal / not zero */

loc_0020D43A: ;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(0x8F7DF8)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, MEM32(0x8F7DF8) (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_0020D4C8; /* jae: above or equal (unsigned >=) */

loc_0020D446: ;
    ecx = MEM32(0x8F7DF4);
    esi = ebx;
    esi = (uint32_t)((int32_t)esi * (int32_t)0x70);
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + ecx;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx = ebx;
    ebx = MEM32(0x8F7DF0);
    ecx = ecx << 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, 0);
    PUSH32(esp, 0);
    _fb = (uint32_t)(ebx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + ebx;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = esp + 0x18;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020D46Du); RECOMP_ABI_CALL(0x002216B0u, sub_002216B0); /* call 0x002216B0 */

loc_0020D46D: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0020D4C2; /* je: equal / zero */

loc_0020D474: ;
    esi = MEM32(0xC0CF14);
    eax = MEM32(0x8F7DFC);
    edx = MEM32(esi + 4);
    ecx = esp + 0x50;
    PUSH32(esp, ecx);
    eax = eax << 0x11;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, edx);
    eax = eax | 0x734;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    PUSH32(esp, esi);
    MEM32(esp + 0x5C) = eax;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020D49Au); RECOMP_ABI_CALL(0x00222DB0u, sub_00222DB0); /* call 0x00222DB0 */

loc_0020D49A: ;
    ecx = 1;
    edx = 0xC0CF10;
    ebx = eax;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020D4ABu); RECOMP_ABI_CALL(0x00222DE0u, sub_00222DE0); /* call 0x00222DE0 */

loc_0020D4AB: ;
    MEM32(esi + 4) = ebx;
    eax = MEM32(ebx + 4);
    MEM32(eax) = ebx;
    ebx = MEM32(0x8F7DFC);
    ebx++;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x8F7DFC) = ebx;
    goto loc_0020D4C8;

loc_0020D4C2: ;
    ebx = MEM32(0x8F7DFC);

loc_0020D4C8: ;
    eax = MEM32(0x8DB844);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0020D5C1; /* jne: not equal / not zero */

loc_0020D4D5: ;
    ecx = MEM32(ebp + 8);
    _fa = (uint32_t)(MEM32(ecx + 0x10)) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(ecx + 0x10), edi (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0020D5C1; /* jne: not equal / not zero */

loc_0020D4E1: ;
    eax = MEM32(esp + 0x40);
    SET_LO16(ecx, (uint16_t)pt_tab_x((int)edi, (int16_t)MEM16(eax))); /* Settings: */
    _fb = (uint32_t)(MEM16(esp + 0x3C)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(ecx, LO16(ecx) + MEM16(esp + 0x3C));
    _fa = (uint32_t)(LO16(ecx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    _fa = (uint32_t)(MEM32(0x43B490)) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0x43B490), 1 (32-bit) */
    edx = pt_tabs() == 7 && edi == PT_TAB ? PT_SPRITE + 1 : MEM32(esp + (pt_tabs() == 7 && edi == 6 ? 5 : edi) * 4 + 0x80); /* Settings: */
    MEMF(esp + 0x2C) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x44)); /* movss */
    MEM32(esp + 0x10) = edx;
    MEM16(esp + 0x14) = LO16(ecx);
    MEM16(esp + 0x18) = 0x1CE;
    MEMF(esp + 0x28) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x24) = xmm0.f[0]; /* movss */
    if (pt_tabs() == 7) { MEMF(esp + 0x24) *= PT_SCALE; MEMF(esp + 0x28) *= PT_SCALE; } /* Settings: */
    if (CMP_NE(_fa, _fb)) goto loc_0020D5C1; /* jne: not equal / not zero */

loc_0020D531: ;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(0x8F7DF8)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, MEM32(0x8F7DF8) (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_0020D5C1; /* jae: above or equal (unsigned >=) */

loc_0020D53D: ;
    ecx = MEM32(0x8F7DF4);
    esi = ebx;
    esi = (uint32_t)((int32_t)esi * (int32_t)0x70);
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + ecx;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx = ebx;
    ebx = MEM32(0x8F7DF0);
    ecx = ecx << 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, 0);
    PUSH32(esp, 0);
    _fb = (uint32_t)(ebx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + ebx;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = esp + 0x18;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020D564u); RECOMP_ABI_CALL(0x002216B0u, sub_002216B0); /* call 0x002216B0 */

loc_0020D564: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0020D5BB; /* je: equal / zero */

loc_0020D56B: ;
    esi = MEM32(0xC0CF14);
    edx = MEM32(0x8F7DFC);
    ecx = MEM32(esi + 4);
    eax = esp + 0x50;
    PUSH32(esp, eax);
    edx = edx << 0x11;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    edx = edx | 0x738;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    PUSH32(esp, esi);
    MEM32(esp + 0x5C) = edx;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020D593u); RECOMP_ABI_CALL(0x00222DB0u, sub_00222DB0); /* call 0x00222DB0 */

loc_0020D593: ;
    ecx = 1;
    edx = 0xC0CF10;
    ebx = eax;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020D5A4u); RECOMP_ABI_CALL(0x00222DE0u, sub_00222DE0); /* call 0x00222DE0 */

loc_0020D5A4: ;
    MEM32(esi + 4) = ebx;
    edx = MEM32(ebx + 4);
    MEM32(edx) = ebx;
    ebx = MEM32(0x8F7DFC);
    ebx++;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x8F7DFC) = ebx;
    goto loc_0020D5C1;

loc_0020D5BB: ;
    ebx = MEM32(0x8F7DFC);

loc_0020D5C1: ;
    ecx = MEM32(esp + 0x4C);
    esi = MEM32(esp + 0x40);
    eax = 2;
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esp + 0x4C) = ecx;
    ecx = MEM32(ebp + 8);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + eax;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = ZX16(MEM16(ecx + 0x24));
    edi++;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, eax (32-bit) */
    MEM32(esp + 0x40) = esi;
    if (CMP_B(_fa, _fb)) goto loc_0020D372; /* jb: below (unsigned <) */

loc_0020D5EA: ;
    _fa = (uint32_t)(MEM32(ecx + 0x10)) & 0xFFFFFFFFu; _fb = (uint32_t)(2) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(ecx + 0x10), 2 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0020D60D; /* jne: not equal / not zero */

loc_0020D5F0: ;
    PUSH32(esp, ecx);
    PUSH32(esp, 0x8DB948);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0020D5FBu); RECOMP_ABI_CALL(0x00214750u, sub_00214750); /* call 0x00214750 */

loc_0020D5FB: ;
    ecx = MEM32(esp + 0x5C);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx = ecx | eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    eax = ecx;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebx);
    esp = ebp;
    POP32(esp, ebp);
    { uint32_t r_ = eax; kc_tab_keys(); eax = r_; }   /* Settings: Q and E on the keyboard */
    esp += 4; return; /* ret */

loc_0020D60D: ;
    eax = MEM32(esp + 0x54);
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebx);
    esp = ebp;
    POP32(esp, ebp);
    { uint32_t r_ = eax; kc_tab_keys(); eax = r_; }   /* Settings: Q and E on the keyboard */
    esp += 4; return; /* ret */

    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}

/**
 * sub_0021A960
 * Original: 0x0021A960 - 0x0021A9A9 (73 bytes, 35 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_0021A960(void)
{
    RECOMP_ENTRY(0x0021A960u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_0021A960: ;
    /* Settings: our sprites. */
    if ((eax & 0xFFFF) >= PT_SPRITE && (eax & 0xFFFF) < PT_SPRITE + PT_N && s_ready == 1) {
        eax = s_defs + ((eax & 0xFFFF) - PT_SPRITE) * 0x18;
        esp += 4; return;
    }
    if ((eax & 0xFFFF) == DLG_SPRITE && s_dlg_def) {   /* Settings: the Are you sure? box */
        eax = s_dlg_def;
        esp += 4; return;
    }
    if ((eax & 0xFFFF) == OV_SPRITE && s_ov_def) {     /* Settings: the Controls panel */
        eax = s_ov_def;
        esp += 4; return;
    }
    if ((eax & 0xFFFF) >= CP_SPRITE && (eax & 0xFFFF) < CP_SPRITE + 3 && s_cp_def[(eax & 0xFFFF) - CP_SPRITE]) {
        eax = s_cp_def[(eax & 0xFFFF) - CP_SPRITE];      /* Settings: an extra costume's picture */
        esp += 4; return;
    }
    if ((eax & 0xFFFF) >= TK_SPRITE && (eax & 0xFFFF) < TK_SPRITE + 2 && s_tk_defs) {   /* Settings: Q, E */
        eax = s_tk_defs + ((eax & 0xFFFF) - TK_SPRITE) * 0x18;
        esp += 4; return;
    }
    PUSH32(esp, esi);
    esi = eax;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0 (32-bit) */
    if (CMP_GE(_fas, _fbs)) goto loc_0021A96B; /* jge: greater or equal (signed >=) */

loc_0021A967: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    POP32(esp, esi);
    esp += 4; return; /* ret */

loc_0021A96B: ;
    eax = 0x452A6C;
    PUSH32(esp, edi);

loc_0021A971: ;
    ecx = MEM32(eax + -32);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0021A98A; /* je: equal / zero */

loc_0021A978: ;
    edx = MEM32(eax);
    ecx = ZX16(LO16(esi));
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, ecx (32-bit) */
    if (CMP_A(_fa, _fb)) goto loc_0021A98A; /* ja: above (unsigned >) */

loc_0021A981: ;
    edi = MEM32(eax + 4);
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edi = edi + edx;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, ecx (32-bit) */
    if (CMP_A(_fa, _fb)) goto loc_0021A999; /* ja: above (unsigned >) */

loc_0021A98A: ;
    _fb = (uint32_t)(0x34) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x34;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x45A7F4) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x45A7F4 (32-bit) */
    if (CMP_L(_fas, _fbs)) goto loc_0021A971; /* jl: less (signed <) */

loc_0021A994: ;
    POP32(esp, edi);
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    POP32(esp, esi);
    esp += 4; return; /* ret */

loc_0021A999: ;
    ecx = MEM32(eax);
    edx = MEM32(eax + -16);
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esi = esi - ecx;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    ecx = esi + esi * 2;
    POP32(esp, edi);
    eax = edx + ecx * 8;
    POP32(esp, esi);
    esp += 4; return; /* ret */

}

/**
 * sub_001805C0
 * Original: 0x001805C0 - 0x00180DA5 (2021 bytes, 512 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_001805C0(void)
{
    RECOMP_ENTRY(0x001805C0u);
    ngb_menu_mark();                                 /* Settings: a menu is up */
    {
        void ngb_aspect_menu_running(void);         /* src/pc_menu.c: the menus' 16:9 mode */
        ngb_aspect_menu_running();
    }
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_001805C0: ;
    PUSH32(esp, ecx);
    PUSH32(esp, ebx);
    PUSH32(esp, esi);
    esi = MEM32(esp + 0x10);
    SET_LO8(eax, MEM8(esi + 0x13));
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), LO8(ebx) (8-bit) */
    MEM32(esp + 8) = ebx;
    if (CMP_EQ(_fa, _fb)) goto loc_001805F1; /* je: equal / zero */

loc_001805D4: ;
    SET_LO8(eax, MEM8(esi + 0x11));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), LO8(ebx) (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001805E0; /* je: equal / zero */

loc_001805DB: ;
    SET_LO8(eax, LO8(eax) - 1);
    _fa = (uint32_t)(LO8(eax)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fb = (_fa == 0x7Fu); /* dec result/SF/OF; CF unchanged */
    MEM8(esi + 0x11) = LO8(eax);

loc_001805E0: ;
    _fa = (uint32_t)(MEM8(esi + 0x11)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(esi + 0x11), LO8(ebx) (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001805F1; /* jne: not equal / not zero */

loc_001805E5: ;
    eax = MEM32(esi + 0x18);
    eax = eax & 0xFFFFFFFCu;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM8(esi + 0x13) = LO8(ebx);
    MEM32(esi + 0x18) = eax;

loc_001805F1: ;
    _fa = (uint32_t)(MEM32(esi + 0x18)) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(esi + 0x18), ebx (32-bit) */
    PUSH32(esp, ebp);
    PUSH32(esp, edi);
    if (CMP_EQ(_fa, _fb)) goto loc_001806E2; /* je: equal / zero */

loc_001805FC: ;
    ebp = MEM32(0xDA4218);
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFFFF00u) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test ebp, 0xFFFFFF00u (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_00180632; /* je: equal / zero */

loc_0018060A: ;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(0x80000000u) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, 0x80000000u (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180CBC; /* jne: not equal / not zero */

loc_00180616: ;
    SET_LO8(eax, MEM8(esi + 1));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(3) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 3 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00180632; /* je: equal / zero */

loc_0018061D: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), LO8(ebx) (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180CBC; /* jne: not equal / not zero */

loc_00180625: ;
    _fa = (uint32_t)(MEM8(0xDA420A)) & 0xFFu; _fb = (uint32_t)(0x10) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(0xDA420A), 0x10 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180CBC; /* jne: not equal / not zero */

loc_00180632: ;
    eax = MEM32(0xDA3E40);
    edi = 0; /* xor self */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xA) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0xA (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0018066E; /* jne: not equal / not zero */

loc_0018063E: ;
    _fa = (uint32_t)(MEM32(0xDA4810)) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0xDA4810), ebx (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0018066E; /* je: equal / zero */

loc_00180646: ;
    ecx = 9;
    PUSH32(esp, 0x00180650u); RECOMP_ABI_CALL(0x000324C0u, sub_000324C0); /* call 0x000324C0 */

loc_00180650: ;
    ecx = ZX8(MEM8(0xDB932E));
    ecx = (uint32_t)((int32_t)ecx * (int32_t)0x78);
    edx = MEM32(ecx + 0xD219A8);
    eax = eax | 0x220;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test edx, eax (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_0018066E; /* je: equal / zero */

loc_00180669: ;
    edi = 1;

loc_0018066E: ;
    eax = MEM32(0x42A8CC);
    PUSH32(esp, 0x00180678u); RECOMP_ABI_CALL(0x0017CDE0u, sub_0017CDE0); /* call 0x0017CDE0 */

loc_00180678: ;
    _fa = (uint32_t)(MEM32(0xDA41C0)) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0xDA41C0), 1 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180CBC; /* jne: not equal / not zero */

loc_00180685: ;
    SET_LO8(ecx, MEM8(esi + 1));
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(3) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), 3 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0018069A; /* je: equal / zero */

loc_0018068D: ;
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(4) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), 4 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0018069A; /* je: equal / zero */

loc_00180692: ;
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), LO8(ebx) (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180CBC; /* jne: not equal / not zero */

loc_0018069A: ;
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), LO8(ebx) (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001806B2; /* jne: not equal / not zero */

loc_0018069E: ;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(0x7FFFFFFF) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test ebp, 0x7FFFFFFF (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_001806B2; /* je: equal / zero */

loc_001806A6: ;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test ebp, 1 (32-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_00180CBC; /* jne: not equal / not zero */

loc_001806B2: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(8) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 8 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001806BF; /* je: equal / zero */

loc_001806B7: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, ebx (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00180CBC; /* je: equal / zero */

loc_001806BF: ;
    MEM8(esi + 0xD) = 1;
    MEM32(0xDA421C) = 0xFFFFFFFFu;
    _fa = (uint32_t)(MEM8(esi + 1)) & 0xFFu; _fb = (uint32_t)(4) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(esi + 1), 4 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001806E2; /* jne: not equal / not zero */

loc_001806D3: ;
    _fa = (uint32_t)(MEM8(0xDA420A)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(0xDA420A), LO8(ebx) (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001806E2; /* je: equal / zero */

loc_001806DB: ;
    MEM8(0xDA420A) = 0x80;

loc_001806E2: ;
    _fa = (uint32_t)(MEM32(0xDA41C0)) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0xDA41C0), ebx (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0018097B; /* jne: not equal / not zero */

loc_001806EE: ;
    SET_LO8(edx, MEM8(esi + 2));
    _fa = (uint32_t)(LO8(edx)) & 0xFFu; _fb = (uint32_t)(MEM8(esi + 1)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(edx), MEM8(esi + 1) (8-bit) */
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    if (CMP_EQ(_fa, _fb)) goto loc_0018078B; /* je: equal / zero */

loc_001806FD: ;
    MEM8(0xDA4201) = LO8(ebx);
    MEM8(0xDA4202) = LO8(ebx);
    MEM8(0xDA4203) = LO8(ebx);
    MEM8(0xDA4204) = LO8(ebx);
    MEM16(0xDA4206) = LO16(ebx);
    MEM16(0xDA4208) = LO16(ebx);
    MEM8(0xDA420A) = LO8(ebx);
    MEM8(0xDA420B) = LO8(ebx);
    MEM8(0xDA4214) = LO8(ebx);
    MEMF(0xDA4220) = xmm0.f[0]; /* movss */
    MEMF(0xDA4224) = xmm0.f[0]; /* movss */
    SET_LO8(eax, MEM8(esi + 1));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), LO8(ebx) (8-bit) */
    MEM8(0xDA2A61) = LO8(ebx);
    if (CMP_NE(_fa, _fb)) goto loc_00180761; /* jne: not equal / not zero */

loc_00180752: ;
    SET_LO8(ebx, MEM8(0xBBBA40));
    PUSH32(esp, 0x0018075Du); RECOMP_ABI_CALL(0x00184360u, sub_00184360); /* call 0x00184360 */

loc_0018075D: ;
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    goto loc_0018078B;

loc_00180761: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 1 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0018076C; /* jne: not equal / not zero */

loc_00180765: ;
    PUSH32(esp, 0x0018076Au); RECOMP_ABI_CALL(0x00184400u, sub_00184400); /* call 0x00184400 */

loc_0018076A: ;
    goto loc_0018078B;

loc_0018076C: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(2) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 2 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180777; /* jne: not equal / not zero */

loc_00180770: ;
    PUSH32(esp, 0x00180775u); RECOMP_ABI_CALL(0x001844C0u, sub_001844C0); /* call 0x001844C0 */

loc_00180775: ;
    goto loc_0018078B;

loc_00180777: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(3) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 3 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0018078B; /* jne: not equal / not zero */

loc_0018077B: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(eax, MEM8(0xDA3E44));
    PUSH32(esp, eax);
    PUSH32(esp, 0x00180788u); RECOMP_ABI_CALL(0x00184550u, sub_00184550); /* call 0x00184550 */

loc_00180788: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_0018078B: ;
    _fa = (uint32_t)(MEM8(esi + 0xD)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(esi + 0xD), LO8(ebx) (8-bit) */
    edi = MEM32(esi + 0x28);
    if (CMP_EQ(_fa, _fb)) goto loc_001807A2; /* je: equal / zero */

loc_00180793: ;
    MEM8(esi + 0xD) = LO8(ebx);
    ecx = ZX8(MEM8(edi + 8));
    MEM32(esi + 0x1C) = ecx;
    goto loc_0018094B;

loc_001807A2: ;
    SET_LO8(eax, MEM8(esi + 0xC));
    ebp = 0; /* xor self */
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0xFF) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0xFF (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180816; /* jne: not equal / not zero */

loc_001807AB: ;
    _fa = (uint32_t)(MEM8(esi + 1)) & 0xFFu; _fb = EXIT_TAB & 0xFFu; /* Settings: Exit's row */
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(esi + 1), Exit's */
    if (CMP_EQ(_fa, _fb)) goto loc_00180816; /* je: equal / zero */

loc_001807B1: ;
    eax = MEM32(0x42A5E0);
    PUSH32(esp, 0x001807BBu); RECOMP_ABI_CALL(0x0017CDE0u, sub_0017CDE0); /* call 0x0017CDE0 */

loc_001807BB: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(7) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 7 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180816; /* jne: not equal / not zero */

loc_001807C0: ;
    edx = MEM32(esi + 0x2C);
    SET_LO8(eax, MEM8(edx + 1));
    MEM8(esi + 0xC) = LO8(eax);
    MEM8(0xDA4201) = LO8(ebx);
    MEM8(0xDA4202) = LO8(ebx);
    MEM8(0xDA4203) = LO8(ebx);
    MEM8(0xDA4204) = LO8(ebx);
    MEM16(0xDA4206) = LO16(ebx);
    MEM16(0xDA4208) = LO16(ebx);
    MEM8(0xDA420A) = LO8(ebx);
    MEM8(0xDA420B) = LO8(ebx);
    MEM8(0xDA4214) = LO8(ebx);
    MEMF(0xDA4220) = xmm0.f[0]; /* movss */
    MEMF(0xDA4224) = xmm0.f[0]; /* movss */
    ebp = 1;

loc_00180816: ;
    SET_LO8(eax, MEM8(esi + 0xC));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0xFF) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0xFF (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00180CBC; /* je: equal / zero */

loc_00180821: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 1 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0018093C; /* jne: not equal / not zero */

loc_00180829: ;
    eax = MEM32(esi + 0x2C);
    SET_LO8(eax, MEM8(eax + 3));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(3) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 3 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0018083C; /* jne: not equal / not zero */

loc_00180833: ;
    _fa = (uint32_t)(MEM16(0xDA2C60)) & 0xFFFFu; _fb = (uint32_t)(LO16(ebx)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(0xDA2C60), LO16(ebx) (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00180856; /* je: equal / zero */

loc_0018083C: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(4) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 4 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180849; /* jne: not equal / not zero */

loc_00180840: ;
    _fa = (uint32_t)(MEM16(0xDA3D86)) & 0xFFFFu; _fb = (uint32_t)(LO16(ebx)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(0xDA3D86), LO16(ebx) (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00180856; /* je: equal / zero */

loc_00180849: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(6) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 6 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180861; /* jne: not equal / not zero */

loc_0018084D: ;
    _fa = (uint32_t)(MEM16(0xDA2B02)) & 0xFFFFu; _fb = (uint32_t)(LO16(ebx)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(0xDA2B02), LO16(ebx) (16-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180861; /* jne: not equal / not zero */

loc_00180856: ;
    MEM32(0xDA41C0) = ebx;
    goto loc_00180CBC;

loc_00180861: ;
    SET_LO8(ecx, MEM8(esi + 2));
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(MEM8(esi + 1)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), MEM8(esi + 1) (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180878; /* jne: not equal / not zero */

loc_00180869: ;
    PUSH32(esp, 0x40);
    PUSH32(esp, 6);
    PUSH32(esp, 0x00180872u); RECOMP_ABI_CALL(0x001E3400u, sub_001E3400); /* call 0x001E3400 */

loc_00180872: ;
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_00180878: ;
    SET_LO8(edx, MEM8(esi + 0xC));
    eax = MEM32(esi + 0x2C);
    MEM8(esi) = LO8(edx);
    ecx = MEM32(0xDA41C0);
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, ebx (32-bit) */
    MEM32(0xDA41C0) = ecx;
    SET_LO8(eax, MEM8(eax + 3));
    MEM8(0xDA4200) = LO8(eax);
    MEM8(0xDA4201) = LO8(ebx);
    MEM8(0xDA4202) = LO8(ebx);
    MEM8(0xDA4203) = LO8(ebx);
    MEM8(0xDA4204) = LO8(ebx);
    MEM16(0xDA4206) = LO16(ebx);
    MEM16(0xDA4208) = LO16(ebx);
    MEM8(0xDA420A) = LO8(ebx);
    MEM8(0xDA420B) = LO8(ebx);
    MEM8(0xDA4214) = LO8(ebx);
    MEM8(0xDA4211) = 1;
    MEM32(0xDA4218) = 2;
    MEM8(0xDA4213) = 1;
    MEM8(0xDA420D) = LO8(ebx);
    MEMF(0xDA4220) = xmm0.f[0]; /* movss */
    MEMF(0xDA4224) = xmm0.f[0]; /* movss */
    if (CMP_NE(_fa, _fb)) goto loc_00180CBC; /* jne: not equal / not zero */

loc_00180903: ;
    SET_LO8(eax, MEM8(esi + 1));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), LO8(ebx) (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0018091C; /* jne: not equal / not zero */

loc_0018090A: ;
    SET_LO8(ebx, MEM8(0xBBBA40));
    PUSH32(esp, 0x00180915u); RECOMP_ABI_CALL(0x00184360u, sub_00184360); /* call 0x00184360 */

loc_00180915: ;
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    goto loc_00180CBC;

loc_0018091C: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 1 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0018092A; /* jne: not equal / not zero */

loc_00180920: ;
    PUSH32(esp, 0x00180925u); RECOMP_ABI_CALL(0x00184400u, sub_00184400); /* call 0x00184400 */

loc_00180925: ;
    goto loc_00180CBC;

loc_0018092A: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(2) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 2 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180C7A; /* jne: not equal / not zero */

loc_00180932: ;
    PUSH32(esp, 0x00180937u); RECOMP_ABI_CALL(0x001844C0u, sub_001844C0); /* call 0x001844C0 */

loc_00180937: ;
    goto loc_00180CBC;

loc_0018093C: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(7) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 7 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180CBC; /* jne: not equal / not zero */

loc_00180944: ;
    edx = ZX8(MEM8(edi + 8));
    MEM32(esi + 0x1C) = edx;

loc_0018094B: ;
    edx = MEM32(0xDA41E8);
    ecx = MEM32(0xDA4218);
    eax = 0x7FFFFFFF;
    edx = edx & eax;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ecx = ecx & eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(0xDA41C8) = 3;
    MEM32(0xDA41E8) = edx;
    MEM32(0xDA4218) = ecx;
    goto loc_00180CBC;

loc_0018097B: ;
    eax = MEM32(0xDA421C);
    ecx = MEM32(0xDA3E40);
    ebp = 0; /* xor self */
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFF) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0xFF (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001809B4; /* je: equal / zero */

loc_0018098F: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(4) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 4 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00180999; /* je: equal / zero */

loc_00180994: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(9) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 9 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001809C2; /* jne: not equal / not zero */

loc_00180999: ;
    eax = ZX8(MEM8(0xDB932E));
    eax = (uint32_t)((int32_t)eax * (int32_t)0x78);
    _fa = (uint32_t)(MEM32(eax + 0xD219A8)) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFF30) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test MEM32(eax + 0xD219A8), 0xFF30 (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_001809C2; /* je: equal / zero */

loc_001809AF: ;
    ebp = 1;

loc_001809B4: ;
    MEM8(esi + 0xD) = 1;
    MEM32(0xDA421C) = 0xFFFFFFFFu;

loc_001809C2: ;
    _fa = (uint32_t)(MEM8(esi + 0xD)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(esi + 0xD), LO8(ebx) (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00180A8B; /* je: equal / zero */

loc_001809CB: ;
    edi = 0; /* xor self */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xA) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0xA (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001809FD; /* jne: not equal / not zero */

loc_001809D2: ;
    _fa = (uint32_t)(MEM32(0xDA4810)) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0xDA4810), ebx (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00180A25; /* je: equal / zero */

loc_001809DA: ;
    ecx = 9;
    PUSH32(esp, 0x001809E4u); RECOMP_ABI_CALL(0x000324C0u, sub_000324C0); /* call 0x000324C0 */

loc_001809E4: ;
    ecx = ZX8(MEM8(0xDB932E));
    ecx = (uint32_t)((int32_t)ecx * (int32_t)0x78);
    edx = MEM32(ecx + 0xD219A8);
    eax = eax | 0x220;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test edx, eax (32-bit) */
    goto loc_00180A1E;

loc_001809FD: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(9) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 9 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180A25; /* jne: not equal / not zero */

loc_00180A02: ;
    _fa = (uint32_t)(MEM32(0xDA4810)) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0xDA4810), ebx (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00180A25; /* je: equal / zero */

loc_00180A0A: ;
    edx = ZX8(MEM8(0xDB932E));
    edx = (uint32_t)((int32_t)edx * (int32_t)0x78);
    _fa = (uint32_t)(MEM32(edx + 0xD219A8)) & 0xFFFFFFFFu; _fb = (uint32_t)(0xF20) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test MEM32(edx + 0xD219A8), 0xF20 (32-bit) */

loc_00180A1E: ;
    if (TEST_Z(_fa, _fb)) goto loc_00180A25; /* je: equal / zero */

loc_00180A20: ;
    edi = 1;

loc_00180A25: ;
    eax = MEM32(0x42A8CC);
    PUSH32(esp, 0x00180A2Fu); RECOMP_ABI_CALL(0x0017CDE0u, sub_0017CDE0); /* call 0x0017CDE0 */

loc_00180A2F: ;
    _fa = (uint32_t)(MEM8(0xDA420A)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(0xDA420A), LO8(ebx) (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00180A48; /* je: equal / zero */

loc_00180A37: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(8) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 8 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00180A48; /* je: equal / zero */

loc_00180A3C: ;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, ebx (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180A48; /* jne: not equal / not zero */

loc_00180A40: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, ebx (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00180CBC; /* je: equal / zero */

loc_00180A48: ;
    eax = MEM32(esi + 0x28);
    MEM8(esi + 0xD) = LO8(ebx);
    ecx = ZX8(MEM8(eax + 8));
    MEM32(esi + 0x1C) = ecx;
    edx = MEM32(0xDA41E8);
    ecx = MEM32(0xDA4218);
    eax = 0x7FFFFFFF;
    edx = edx & eax;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ecx = ecx & eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(0xDA41C8) = 3;
    MEM32(0xDA41E8) = edx;
    MEM32(0xDA4218) = ecx;
    MEM32(0xDA41C0) = ebx;
    goto loc_00180CBC;

loc_00180A8B: ;
    SET_LO8(eax, MEM8(esi + 1));
    _fa = (uint32_t)(MEM8(esi + 2)) & 0xFFu; _fb = (uint32_t)(LO8(eax)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(esi + 2), LO8(eax) (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180B8B; /* jne: not equal / not zero */

loc_00180A97: ;
    _fa = (uint32_t)(MEM8(0xDA420A)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(0xDA420A), LO8(ebx) (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180B8B; /* jne: not equal / not zero */

loc_00180AA3: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), LO8(ebx) (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00180AC7; /* je: equal / zero */

loc_00180AA7: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(3) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 3 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00180AB2; /* je: equal / zero */

loc_00180AAB: ;
    ebp = 3;
    goto loc_00180AE4;

loc_00180AB2: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), LO8(ebx) (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00180AC7; /* je: equal / zero */

loc_00180AB6: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(3) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 3 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180B8B; /* jne: not equal / not zero */

loc_00180ABE: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(4) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 4 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00180B8B; /* je: equal / zero */

loc_00180AC7: ;
    SET_LO8(eax, MEM8(0xDA4201));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(2) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 2 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180AD7; /* jne: not equal / not zero */

loc_00180AD0: ;
    ebp = 1;
    goto loc_00180AE4;

loc_00180AD7: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), LO8(ebx) (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180B8B; /* jne: not equal / not zero */

loc_00180ADF: ;
    ebp = 2;

loc_00180AE4: ;
    eax = MEM32(0x42A898);
    edi = MEM32(esi + 0x28);
    PUSH32(esp, 0x00180AF1u); RECOMP_ABI_CALL(0x0017CDE0u, sub_0017CDE0); /* call 0x0017CDE0 */

loc_00180AF1: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 1 (32-bit) */
    edx = MEM32(esi + 0x2C);
    MEM32(esp + 0x18) = edx;
    if (CMP_NE(_fa, _fb)) goto loc_00180B1E; /* jne: not equal / not zero */

loc_00180AFD: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ebp) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test eax, ebp (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_00180B8B; /* je: equal / zero */

loc_00180B05: ;
    eax = ZX8(MEM8(esi + 1));
    ecx = ZX8(MEM8(edi + 2));
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ecx (32-bit) */
    if (CMP_LE(_fas, _fbs)) goto loc_00180B3E; /* jle: less or equal (signed <=) */

loc_00180B12: ;
    edx = ZX8(MEM8(edi + 1));
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    edx = edx - ecx;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    eax = edx + eax + -1;
    goto loc_00180B3E;

loc_00180B1E: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(2) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 2 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180B8B; /* jne: not equal / not zero */

loc_00180B23: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ebp) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test eax, ebp (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_00180B8B; /* je: equal / zero */

loc_00180B27: ;
    eax = ZX8(MEM8(esi + 1));
    ecx = ZX8(MEM8(edi + 1));
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ecx (32-bit) */
    if (CMP_GE(_fas, _fbs)) goto loc_00180B3E; /* jge: greater or equal (signed >=) */

loc_00180B34: ;
    edx = ZX8(MEM8(edi + 2));
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    edx = edx - ecx;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    eax = edx + eax + 1;

loc_00180B3E: ;
    _fa = (uint32_t)(MEM8(esi + 2)) & 0xFFu; _fb = (uint32_t)(LO8(eax)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(esi + 2), LO8(eax) (8-bit) */
    MEM8(esi + 1) = LO8(eax);
    if (CMP_EQ(_fa, _fb)) goto loc_00180B8B; /* je: equal / zero */

loc_00180B46: ;
    PUSH32(esp, 0x40);
    PUSH32(esp, 1);
    PUSH32(esp, 0x00180B4Fu); RECOMP_ABI_CALL(0x001E3400u, sub_001E3400); /* call 0x001E3400 */

loc_00180B4F: ;
    eax = MEM32(0xDA3E40);
    eax = MEM32(eax * 4 + 0x42A280);
    SET_LO8(edx, MEM8(esi));
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esp + 0x10) = 1;
    PUSH32(esp, 0x00180B6Du); RECOMP_ABI_CALL(0x0017D8D0u, sub_0017D8D0); /* call 0x0017D8D0 */

loc_00180B6D: ;
    eax = MEM32(eax + 0xC);
    SET_LO8(edx, MEM8(esi + 1));
    PUSH32(esp, 0x00180B78u); RECOMP_ABI_CALL(0x0017D8F0u, sub_0017D8F0); /* call 0x0017D8F0 */

loc_00180B78: ;
    MEM32(esi + 0x2C) = eax;
    SET_LO8(eax, LO8(edx));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = EXIT_TAB & 0xFFu; /* Settings: Exit's row */
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), Exit's */
    if (CMP_EQ(_fa, _fb)) goto loc_00180B8B; /* je: equal / zero */

loc_00180B81: ;
    ecx = MEM32(esp + 0x18);
    SET_LO8(edx, MEM8(ecx + 1));
    MEM8(esi + 0xC) = LO8(edx);

loc_00180B8B: ;
    SET_LO8(eax, MEM8(esi + 2));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(MEM8(esi + 1)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), MEM8(esi + 1) (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00180CBC; /* je: equal / zero */

loc_00180B97: ;
    _fa = (uint32_t)(MEM8(esi + 0xC)) & 0xFFu; _fb = (uint32_t)(0xFF) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(esi + 0xC), 0xFF (8-bit) */
    MEM8(0xDA2A61) = LO8(ebx);
    if (CMP_EQ(_fa, _fb)) goto loc_00180C91; /* je: equal / zero */

loc_00180BA7: ;
    eax = MEM32(esi + 0x2C);
    SET_LO8(eax, MEM8(eax + 3));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(3) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 3 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180BBA; /* jne: not equal / not zero */

loc_00180BB1: ;
    _fa = (uint32_t)(MEM16(0xDA2C60)) & 0xFFFFu; _fb = (uint32_t)(LO16(ebx)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(0xDA2C60), LO16(ebx) (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00180BDA; /* je: equal / zero */

loc_00180BBA: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(4) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 4 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180BC7; /* jne: not equal / not zero */

loc_00180BBE: ;
    _fa = (uint32_t)(MEM16(0xDA3D86)) & 0xFFFFu; _fb = (uint32_t)(LO16(ebx)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(0xDA3D86), LO16(ebx) (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00180BDA; /* je: equal / zero */

loc_00180BC7: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(6) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 6 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180BD4; /* jne: not equal / not zero */

loc_00180BCB: ;
    _fa = (uint32_t)(MEM16(0xDA2B02)) & 0xFFFFu; _fb = (uint32_t)(LO16(ebx)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(0xDA2B02), LO16(ebx) (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00180BDA; /* je: equal / zero */

loc_00180BD4: ;
    _fa = (uint32_t)(MEM32(esp + 0x10)) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(esp + 0x10), ebx (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00180BFB; /* je: equal / zero */

loc_00180BDA: ;
    MEM32(0xDA41C0) = MEM32(0xDA41C0) - 1;
    _fa = (uint32_t)(MEM32(0xDA41C0)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    MEM8(esi) = LO8(ebx);
    MEM8(esi + 3) = LO8(ebx);
    MEM8(esi + 4) = LO8(ebx);
    MEM16(esi + 6) = LO16(ebx);
    MEM16(esi + 8) = LO16(ebx);
    MEM8(esi + 0x14) = LO8(ebx);
    MEM8(esi + 0x11) = LO8(ebx);
    MEM32(esi + 0x18) = ebx;
    goto loc_00180C0C;

loc_00180BFB: ;
    _fa = (uint32_t)(MEM8(0xDA4200)) & 0xFFu; _fb = (uint32_t)(LO8(eax)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(0xDA4200), LO8(eax) (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00180903; /* je: equal / zero */

loc_00180C07: ;
    MEM8(0xDA4200) = LO8(eax);

loc_00180C0C: ;
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    MEM8(0xDA4201) = LO8(ebx);
    MEM8(0xDA4202) = LO8(ebx);
    MEM8(0xDA4203) = LO8(ebx);
    MEM8(0xDA4204) = LO8(ebx);
    MEM16(0xDA4206) = LO16(ebx);
    MEM16(0xDA4208) = LO16(ebx);
    MEM8(0xDA4214) = LO8(ebx);
    MEM8(0xDA4211) = 1;
    MEM32(0xDA4218) = 2;
    MEM8(0xDA4213) = 1;
    MEM8(0xDA420D) = LO8(ebx);
    MEM8(0xDA420A) = LO8(ebx);
    MEM8(0xDA420B) = LO8(ebx);
    MEMF(0xDA4220) = xmm0.f[0]; /* movss */
    MEMF(0xDA4224) = xmm0.f[0]; /* movss */
    goto loc_00180903;

loc_00180C7A: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(3) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 3 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180CBC; /* jne: not equal / not zero */

loc_00180C7E: ;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(ecx, MEM8(0xDA3E44));
    PUSH32(esp, ecx);
    PUSH32(esp, 0x00180C8Cu); RECOMP_ABI_CALL(0x00184550u, sub_00184550); /* call 0x00184550 */

loc_00180C8C: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_00180CBC;

loc_00180C91: ;
    MEM32(0xDA41C0) = MEM32(0xDA41C0) - 1;
    _fa = (uint32_t)(MEM32(0xDA41C0)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    MEM8(esi) = LO8(ebx);
    MEM8(esi + 3) = LO8(ebx);
    MEM8(esi + 4) = LO8(ebx);
    MEM16(esi + 6) = LO16(ebx);
    MEM16(esi + 8) = LO16(ebx);
    MEM8(esi + 0x14) = LO8(ebx);
    MEM8(esi + 0x11) = LO8(ebx);
    MEM32(esi + 0x18) = ebx;
    SET_LO8(edx, MEM8(0xDA4201));
    MEM8(0xDA4202) = LO8(edx);

loc_00180CBC: ;
    edi = MEM32(0xDA41C0);
    edx = esi;
    PUSH32(esp, 0x00180CC9u); RECOMP_ABI_CALL(0x00182240u, sub_00182240); /* call 0x00182240 */

loc_00180CC9: ;
    SET_LO8(eax, MEM8(0xDA41D1));
    MEM8(esp + 0x18) = LO8(eax);
    eax = ZX8(LO8(eax));
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(5) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 5 (32-bit) */
    POP32(esp, edi);
    POP32(esp, ebp);
    if (eax == PT_TAB && pt_tabs() == 7) { pt_draw(); goto loc_00180D2E; } /* Settings: then as Exit */
    if (eax == 6 && pt_tabs() == 7) { pt_exit_draw(); goto loc_00180D2E; } /* Settings: Exit's page, ours */
    if (CMP_A(_fa, _fb)) goto loc_00180D3B; /* ja: above (unsigned >) */

loc_00180CDC: ;
    { uint32_t _jt = MEM32(eax * 4 + 0x180DA8); /* switch: 6 entries, 6 targets */
    if (_jt == 0x00180CE3u) goto loc_00180CE3;
    if (_jt == 0x00180CF2u) goto loc_00180CF2;
    if (_jt == 0x00180D01u) goto loc_00180D01;
    if (_jt == 0x00180D10u) goto loc_00180D10;
    if (_jt == 0x00180D1Fu) goto loc_00180D1F;
    if (_jt == 0x00180D2Eu) goto loc_00180D2E;
    g_seh_ebp = ebp; RECOMP_ITAIL(_jt); return; }

loc_00180CE3: ;
    PUSH32(esp, 0xDA4200);
    PUSH32(esp, 0x00180CEDu); RECOMP_ABI_CALL(0x00180DC0u, sub_00180DC0); /* call 0x00180DC0 */

loc_00180CED: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_00180D3B;

loc_00180CF2: ;
    PUSH32(esp, 0xDA4200);
    PUSH32(esp, 0x00180CFCu); RECOMP_ABI_CALL(0x001818E0u, sub_001818E0); /* call 0x001818E0 */

loc_00180CFC: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_00180D3B;

loc_00180D01: ;
    PUSH32(esp, 0xDA4200);
    PUSH32(esp, 0x00180D0Bu); RECOMP_ABI_CALL(0x00181380u, sub_00181380); /* call 0x00181380 */

loc_00180D0B: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_00180D3B;

loc_00180D10: ;
    PUSH32(esp, 0xDA4200);
    PUSH32(esp, 0x00180D1Au); RECOMP_ABI_CALL(0x00181C00u, sub_00181C00); /* call 0x00181C00 */

loc_00180D1A: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_00180D3B;

loc_00180D1F: ;
    PUSH32(esp, 0xDA4200);
    PUSH32(esp, 0x00180D29u); RECOMP_ABI_CALL(0x00181F40u, sub_00181F40); /* call 0x00181F40 */

loc_00180D29: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_00180D3B;

loc_00180D2E: ;
    SET_LO8(eax, 0); /* xor self */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* xor result */
    MEM8(0xDA4213) = LO8(ebx);
    PUSH32(esp, 0x00180D3Bu); RECOMP_ABI_CALL(0x001828B0u, sub_001828B0); /* call 0x001828B0 */

loc_00180D3B: ;
    _fa = (uint32_t)(MEM8(esp + 0x10)) & 0xFFu; _fb = (uint32_t)(3) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(esp + 0x10), 3 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180D4B; /* jne: not equal / not zero */

loc_00180D42: ;
    _fa = (uint32_t)(MEM32(0xDA41C8)) & 0xFFFFFFFFu; _fb = (uint32_t)(3) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0xDA41C8), 3 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180D91; /* jne: not equal / not zero */

loc_00180D4B: ;
    ecx = ZX8(MEM8(0x589764));
    SET_LO8(edx, MEM8(0xDB9413));
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x31) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x31 (32-bit) */
    if (CMP_G(_fas, _fbs)) ecx = ebx; /* cmovg */
    _fa = (uint32_t)(LO8(edx)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(edx), LO8(ebx) (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180D69; /* jne: not equal / not zero */

loc_00180D64: ;
    eax = 1;

loc_00180D69: ;
    eax = (uint32_t)((int32_t)eax * (int32_t)0x32);
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = MEM32(eax * 4 + 0x428F98);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x80000000u) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x80000000u (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_00180D81; /* jb: below (unsigned <) */

loc_00180D7C: ;
    _fb = (uint32_t)(0x80000000u) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - 0x80000000u;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */

loc_00180D81: ;
    ecx = eax;
    PUSH32(esp, 0x00180D88u); RECOMP_ABI_CALL(0x0031F6E0u, sub_0031F6E0); /* call 0x0031F6E0 */

loc_00180D88: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00180D91; /* je: equal / zero */

loc_00180D8C: ;
    PUSH32(esp, 0x00180D91u); RECOMP_ABI_CALL(0x00184820u, sub_00184820); /* call 0x00184820 */

loc_00180D91: ;
    _fa = (uint32_t)(MEM32(esi + 0x18)) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(esi + 0x18), ebx (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180D9E; /* jne: not equal / not zero */

loc_00180D96: ;
    eax = MEM32(esi + 0x1C);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFFFFFFu) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0xFFFFFFFFu (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00180DA1; /* jne: not equal / not zero */

loc_00180D9E: ;
    eax = eax | 0xFFFFFFFFu;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */

loc_00180DA1: ;
    POP32(esp, esi);
    POP32(esp, ebx);
    POP32(esp, ecx);
    esp += 4; return; /* ret */

}

/**
 * sub_002216B0
 * Original: 0x002216B0 - 0x00221C6A (1466 bytes, 381 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
static void sprite_submit(void);

static int sprite_log(void)
{
    static int on = -1;
    if (on < 0)
        on = getenv("NGB_SPRITE_LOG") != NULL;
    return on;
}

/* Every sprite the game queues passes here: the letters of its menus are
 * noted for the mouse (src/menu_mouse.c), with where they were put. */
void ngb_menu_glyph(float x0, float y0, float x1, float y1, float bright, uint32_t z);
extern uint32_t ngb_font_tex[];
extern int ngb_font_tex_n;

/* The sprite's texture coordinates drawn in from its edges. The game's
 * sprites map texels to the console's pixels one to one, so a pixel never
 * samples past a sprite's edge; drawn larger, the pixels along the edge
 * sample there, and bilinear filtering blends in what lies beyond -- the
 * next picture in the texture, or (wrapping) the far edge: thin lines along
 * the seams of a menu's pieces and around its plates. Taking each edge in
 * by half a texel less half a pixel keeps every sample within the sprite,
 * and leaves the console's 1x as it was. A quad is four vertices of seven
 * dwords: x y z rhw colour u v. NGB_NO_EDGE_INSET=1 leaves them. */

static void edge_inset(uint32_t p, uint32_t verts)
{
    static int off = -1;
    if (off < 0)
        off = getenv("NGB_NO_EDGE_INSET") != NULL;
    int scale = ngb_pc_setting(0);
    if (scale <= 1 || off)
        return;
    uint32_t def = sprite_def(MEM32(p) & 0xFFFF);
    if (!def)
        return;
    uint32_t hdr = MEM32(0x8DBB28 + MEM32(def + 0x10) * 8);
    if (!hdr)
        return;
    uint32_t fmt = MEM32(hdr + 0xC);
    float tw = (float)(1 << (fmt >> 20 & 0xF)), th = (float)(1 << (fmt >> 24 & 0xF));
    for (int axis = 0; axis < 2; axis++) {
        uint32_t off = 20 + 4 * axis;
        float lo = MEMF(verts + off), hi = lo;
        for (int v = 1; v < 4; v++) {
            float t = MEMF(verts + v * 28 + off);
            if (t < lo) lo = t;
            if (t > hi) hi = t;
        }
        float size = axis ? th : tw, d = 0.5f / size * (1.0f - 1.0f / scale);
        if (hi - lo < 4 * d || hi > 4.0f || lo < -4.0f)  /* tiny, or not 0-1 coordinates */
            continue;
        for (int v = 0; v < 4; v++) {
            float t = MEMF(verts + v * 28 + off);
            MEMF(verts + v * 28 + off) = t == lo ? t + d : t == hi ? t - d : t;
        }
    }
}

void ngb_aspect_tick(void);   /* src/pc_menu.c */

void sub_002216B0(void)
{
    ngb_aspect_tick();
    uint32_t p = eax, verts = esi, def = 0;
    if (ngb_font_tex_n && (MEM32(p) & 0xFFFF) < 0x200) {
        def = sprite_def(MEM32(p) & 0xFFFF);
        int glyph = 0;
        for (int i = 0; def && i < ngb_font_tex_n; i++)
            glyph |= ngb_font_tex[i] == MEM32(def + 0x10);
        if (!glyph)
            def = 0;
    }
    if (!def && (MEM32(p) & 0xFFFF) == 0xC5) {        /* the title's PRESS START BUTTON */
        extern void ngb_title_seen(void);              /* (src/ingame_menu.c: Return to Main Menu) */
        ngb_title_seen();
    }
    kc_check(MEM32(p) & 0xFFFF);                       /* keys for the button icons (font glyphs too) */
    {   /* Our pages (Settings, Exit): the Weapon page's category strip (sprites 0x186-0x187, a dark band
         * with a light underline) is still drawn under them; above 1x its underline showed as a line
         * across the blade and through our page. Not drawn there. */
        uint32_t id = MEM32(p) & 0xFFFF;
        if ((id == 0x186 || id == 0x187) && pt_tabs() == 7 && (MEM8(0xDA41D1) == PT_TAB || MEM8(0xDA41D1) == 6) &&
            ngb_menu_up()) {
            eax = 0;
            esp += 4;
            return;
        }
    }
    if ((MEM32(p) & 0xFFFF) == 0x17B || (MEM32(p) & 0xFFFF) == 0x177)   /* Weapon's and Exit's plates: where */
        kc_tab_seen(MEM32(p) & 0xFFFF, MEMF(verts), MEMF(verts + 4), MEMF(verts + 7 * 4), MEMF(verts + 15 * 4));
    if (!def && ngb_recomp_menu_active()) {
        rp_check(MEM32(p) & 0xFFFF);                   /* RECOMP SETTINGS' header and words */
        if (rp_skip(MEM32(p) & 0xFFFF)) {              /* and four categories, not six */
            eax = 0;
            esp += 4;
            return;
        }
    }
    sprite_submit();
    if (eax && s_ov_open > 0 && (MEM32(p) & 0xFFFF) != OV_SPRITE) {   /* the Controls panel: what is under it */
        float cx = (MEMF(verts) + MEMF(verts + 7 * 4)) / 2, cy = (MEMF(verts + 4) + MEMF(verts + 15 * 4)) / 2;
        if (cx >= OV_X - 20 && cx < 690 && cy >= OV_Y - 8 && cy < OV_Y + OV_H * OV_S + 4)
            for (int v = 0; v < 4; v++) {              /* (the page's frame too) not drawn: off the screen */
                MEMF(verts + v * 28) = -4096.0f;
                MEMF(verts + v * 28 + 4) = -4096.0f;
            }
    }
    if (eax && !def && (s_tab_first[2] > 0 || ENV_ONCE("NGB_SPRITE_AT_ANY")) && ENV_ONCE("NGB_SPRITE_AT")) {   /* debugging: sprites at console row y (pause menu) */
        static int n;
        float y = (float)atof(ENV_ONCE("NGB_SPRITE_AT")), y0 = MEMF(verts + 4), y1 = MEMF(verts + 15 * 4);
        uint32_t d = sprite_def(MEM32(p) & 0xFFFF);
        static uint32_t seen[256];
        static int ns;
        uint32_t key = (MEM32(p) & 0xFFFF) << 16 ^ (uint32_t)lroundf(y0 * 7 + y1 * 13);
        int k = 0;
        while (k < ns && seen[k] != key) k++;
        if (k == ns && ns < 256 && n < 200 && ((fabsf(y0 - y) < 1.5f || fabsf(y1 - y) < 1.5f) ||
                       (y0 < y && y1 > y && MEMF(verts + 7 * 4) - MEMF(verts) > 150))) {
            seen[ns++] = key;
            n++;
            fprintf(stderr, "[AT] %04X tex %X x %.1f..%.1f y %.2f..%.2f v %.4f..%.4f def v %.4f..%.4f size %u\n",
                    MEM32(p) & 0xFFFF, d ? MEM32(d + 0x10) : 0, MEMF(verts), MEMF(verts + 7 * 4), y0, y1,
                    MEMF(verts + 24), MEMF(verts + 15 * 4 + 20), d ? MEMF(d + 4) : 0, d ? MEMF(d + 12) : 0,
                    d ? MEM16(d + 0x16) : 0);
        }
    }
    if (eax)
        edge_inset(p, verts);
    if (!def && eax && ENV_ONCE("NGB_SPRITE_DUMP")) {    /* debugging: a sprite's texture as PPM, once */
        static char done[0x800];
        uint32_t id = MEM32(p) & 0xFFFF, d = id < 0x800 && !done[id] ? sprite_def(id) : 0;
        char want[16];
        snprintf(want, sizeof want, ",%X,", id);
        char list[256];
        snprintf(list, sizeof list, ",%s,", ENV_ONCE("NGB_SPRITE_DUMP"));
        if (d && strstr(list, want)) {
            done[id] = 1;
            uint32_t ti = MEM32(d + 0x10);
            int w, h;
            uint32_t *px = read_texture(MEM32(0x8DBB28 + ti * 8), MEM32(0x8DBB2C + ti * 8), &w, &h);
            char path[96];
            snprintf(path, sizeof path, "sprite_%04X.ppm", id);   /* in the working directory */
            FILE *f = px ? fopen(path, "wb") : NULL;
            if (f) {
                fprintf(f, "P6\n%d %d\n255\n", w, h * 2);
                for (int i = 0; i < w * h; i++) {          /* colour over grey, then the alpha */
                    float a = (px[i] >> 24) / 255.0f;
                    for (int c = 2; c >= 0; c--)
                        fputc((int)lroundf((px[i] >> (8 * c) & 255) * a + 96 * (1 - a)), f);
                }
                for (int i = 0; i < w * h; i++)
                    for (int c = 0; c < 3; c++)
                        fputc(px[i] >> 24, f);
                fclose(f);
                fprintf(stderr, "[SPRITE] %04X texture %X %dx%d dumped; def uv %.3f %.3f %.3f %.3f size %u %u\n", id, ti, w, h,
                        MEMF(d), MEMF(d + 4), MEMF(d + 8), MEMF(d + 12), MEM16(d + 0x14), MEM16(d + 0x16));
            }
            free(px);
        }
    }
    if (!def && eax && sprite_log()) {
        static uint32_t seen[2048];
        static int ns;
        uint32_t id = MEM32(p) & 0xFFFF, d = sprite_def(id);
        int k = 0;
        while (k < ns && seen[k] != id) k++;
        if (k == ns && ns < 2048 && d) {
            seen[ns++] = id;
            uint32_t ti = MEM32(d + 0x10), hdr = MEM32(0x8DBB28 + ti * 8);
            fprintf(stderr, "[SPRITE] %04X tex %X data %08X fmt %08X uv %.3f %.3f %.3f %.3f at %.0f,%.0f-%.0f,%.0f\n",
                    id, ti, hdr ? MEM32(hdr + 4) : 0, hdr ? MEM32(hdr + 0xC) : 0,
                    MEMF(d), MEMF(d + 4), MEMF(d + 8), MEMF(d + 12),
                    MEMF(verts), MEMF(verts + 4), MEMF(verts + 7 * 4), MEMF(verts + 15 * 4));
        }
    }
    if (!def && eax && ngb_menu_up()) {                /* a button prompt's icon */
        uint32_t id = MEM32(p) & 0xFFFF;
        static const uint8_t button[6] = { 0x04, 0x08, 0x02, 0x01, 0x40, 0x80 };  /* X Y B A LT RT */
        uint32_t d = id >= 5 && id <= 10 ? sprite_def(id) : 0;
        if (d && MEM32(d + 0x10) == 0)
            ngb_menu_add_item(MEMF(verts), MEMF(verts + 4), MEMF(verts + 7 * 4),
                              MEMF(verts + 15 * 4), 1.0f, 0x8B000000u, button[id - 5], 3);
    }
    if (!def && eax && ngb_menu_up()) {                /* a pause menu tab: where it is */
        static const uint16_t tab_id[7] = { 0x17B, 0x17A, 0x178, 0x17C, 0x179, PT_SPRITE, 0x177 };
        uint32_t id = MEM32(p) & 0xFFFF;
        for (int t = 0; t < 7; t++)
            if (id == tab_id[t]) {
                ngb_menu_add_item(MEMF(verts), MEMF(verts + 4), MEMF(verts + 7 * 4),
                                  MEMF(verts + 15 * 4), t == MEM8(0xDA41D1) ? 1.0f : 0.4f,
                                  0x7AB00000u, (uint32_t)t, 1);
                break;
            }
    }
    if (!def && eax && ngb_menu_up() && ngb_engine_row() >= 0) {   /* plates and the like */
        float x0 = MEMF(verts), y0 = MEMF(verts + 4), x1 = MEMF(verts + 7 * 4), y1 = MEMF(verts + 15 * 4);
        float w = x1 - x0, h = y1 - y0;
        if (w > 8 && h > 8 && w < 600 && h < 200) {
            uint32_t g = 0x5B000000u | (MEM16(p + 8) & 0xFFF) << 12 |
                         ((uint32_t)lroundf(w / 4) & 0x3F) << 6 | ((uint32_t)lroundf(h / 4) & 0x3F);
            ngb_menu_add_item(x0, y0, x1, y1, MEMF(p + 0x1C) * MEMF(p + 0x20), g,
                              (uint32_t)lroundf(y0) << 16 | ((uint32_t)lroundf(x0) & 0xFFFF), 2);
        }
    }
    if (def && eax) {
        uint32_t tex = MEM32(def + 0x10);
        for (int i = 0; i < ngb_font_tex_n; i++)
            if (ngb_font_tex[i] == tex) {
                ngb_menu_glyph(MEMF(verts), MEMF(verts + 4), MEMF(verts + 7 * 4),
                               MEMF(verts + 15 * 4), MEMF(p + 0x1C) * MEMF(p + 0x20),
                               MEM16(p + 8));
                break;
            }
    }
}

static void sprite_submit(void)
{
    RECOMP_ENTRY(0x002216B0u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    double _fca = 0.0, _fcb = 0.0;
    (void)_fca; (void)_fcb;
    #define fp_push(v) do { double _fp_value = (v); \
        g_fp_top = (g_fp_top + 7u) & 7u; \
        g_fp_stack[g_fp_top] = _fp_value; } while (0)
    #define fp_pop() (g_fp_top = (g_fp_top + 1u) & 7u)
    #define fp_top() g_fp_stack[g_fp_top]
    #define fp_st(i) g_fp_stack[(g_fp_top + (i)) & 7u]
    #define fp_st1() fp_st(1)
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_002216B0: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    ebp = MEM32(esp + 0x18);
    PUSH32(esp, edi);
    edi = eax;
    eax = MEM32(edi);
    ebx = ecx;
    PUSH32(esp, 0x002216C5u); RECOMP_ABI_CALL(0x0021A960u, sub_0021A960); /* call 0x0021A960 */

loc_002216C5: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00221C61; /* je: equal / zero */

loc_002216CD: ;
    ecx = MEM32(eax + 0x10);
    if (ecx >= PT_TEX && ecx < PT_TEX + PT_N && s_ready == 1) { /* Settings: ours */
        ecx = s_entries + (ecx - PT_TEX) * 8;
        goto pt_entry;
    }
    if (ecx == DLG_TEX && s_dlg_entry) {               /* Settings: the box's */
        ecx = s_dlg_entry;
        goto pt_entry;
    }
    if (ecx == OV_TEX && s_ov_entry) {                 /* Settings: the Controls panel's */
        ecx = s_ov_entry;
        goto pt_entry;
    }
    if (ecx >= CP_TEX && ecx < CP_TEX + 3 && s_cp_entry[ecx - CP_TEX]) {   /* Settings: an extra costume's */
        ecx = s_cp_entry[ecx - CP_TEX];
        goto pt_entry;
    }
    if (ecx == TK_TEX && s_tk_entry) {                 /* Settings: the Q and E keys' */
        ecx = s_tk_entry;
        goto pt_entry;
    }
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x516) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x516 (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_00221C61; /* jae: above or equal (unsigned >=) */

loc_002216DC: ;
    ecx = ecx * 8 + 0x8DBB28;
pt_entry: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00221C61; /* je: equal / zero */

loc_002216EB: ;
    edx = MEM32(ecx);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    MEM32(ebx) = edx;
    if (CMP_EQ(_fa, _fb)) goto loc_00221C61; /* je: equal / zero */

loc_002216F7: ;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, 0 (32-bit) */
    ecx = MEM32(ecx + 4);
    MEM32(ebx + 4) = ecx;
    edx = MEM32(edi + 0x10);
    MEM32(ebx + 8) = edx;
    ecx = (uint32_t)(int32_t)SMEM16(edi + 4);
    edx = (uint32_t)(int32_t)SMEM16(edi + 6);
    SET_LO16(ebx, MEM16(eax + 0x14));
    xmm4.f[0] = (float)(int32_t)ecx; /* cvtsi2ss */
    SET_LO16(ecx, MEM16(eax + 0x16));
    xmm5.f[0] = (float)(int32_t)edx; /* cvtsi2ss */
    edx = (uint32_t)(int32_t)SMEM8(edi + 0xB);
    MEM16(esp + 0xC) = LO16(ecx);
    ecx = (uint32_t)(int32_t)SMEM8(edi + 0xC);
    if (CMP_NE(_fa, _fb)) goto loc_002217BD; /* jne: not equal / not zero */

loc_00221730: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFFFFFFu) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0xFFFFFFFFu (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00221740; /* jne: not equal / not zero */

loc_00221735: ;
    xmm3 = XMM_SCALAR(MEMF(eax + 8)); /* movss */
    xmm1 = XMM_SCALAR(MEMF(eax)); /* movss */
    goto loc_00221749;

loc_00221740: ;
    xmm3 = XMM_SCALAR(MEMF(eax)); /* movss */
    xmm1 = XMM_SCALAR(MEMF(eax + 8)); /* movss */

loc_00221749: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFFFFFFu) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0xFFFFFFFFu (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0022175A; /* jne: not equal / not zero */

loc_0022174E: ;
    xmm0 = XMM_SCALAR(MEMF(eax + 4)); /* movss */
    xmm2 = XMM_SCALAR(MEMF(eax + 0xC)); /* movss */
    goto loc_00221764;

loc_0022175A: ;
    xmm0 = XMM_SCALAR(MEMF(eax + 0xC)); /* movss */
    xmm2 = XMM_SCALAR(MEMF(eax + 4)); /* movss */

loc_00221764: ;
    MEMF(esi + 0x18) = xmm2.f[0]; /* movss */
    MEMF(esi + 0x30) = xmm1.f[0]; /* movss */
    MEMF(esi + 0x34) = xmm2.f[0]; /* movss */
    MEMF(esi + 0x50) = xmm0.f[0]; /* movss */
    MEMF(esi + 0x68) = xmm1.f[0]; /* movss */
    MEMF(esi + 0x6C) = xmm0.f[0]; /* movss */
    MEMF(esi + 0x14) = xmm3.f[0]; /* movss */
    MEMF(esi + 0x4C) = xmm3.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(eax + 8)); /* movss */
    xmm0.f[0] = xmm0.f[0] - MEMF(eax); /* subss */
    xmm0.f[0] = xmm0.f[0] * MEMF(edi + 0x14); /* mulss */
    edx = ZX16(LO16(ebx));
    xmm1.f[0] = (float)(int32_t)edx; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * xmm1.f[0]; /* mulss */
    xmm1 = XMM_SCALAR(MEMF(eax + 0xC)); /* movss */
    xmm1.f[0] = xmm1.f[0] - MEMF(eax + 4); /* subss */
    eax = ZX16(MEM16(esp + 0xC));
    xmm2.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    goto loc_00221850;

loc_002217BD: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFFFFFFu) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0xFFFFFFFFu (32-bit) */
    xmm2 = XMM_SCALAR(MEMF(ebp)); /* movss */
    xmm3 = XMM_SCALAR(MEMF(ebp + 4)); /* movss */
    xmm0 = XMM_SCALAR(MEMF(ebp + 8)); /* movss */
    xmm1 = XMM_SCALAR(MEMF(ebp + 0xC)); /* movss */
    if (CMP_NE(_fa, _fb)) goto loc_00221805; /* jne: not equal / not zero */

loc_002217D6: ;
    MEMF(esi + 0x68) = xmm2.f[0]; /* movss */
    MEMF(esi + 0x4C) = xmm0.f[0]; /* movss */
    MEMF(esi + 0x30) = xmm2.f[0]; /* movss */
    MEMF(esi + 0x14) = xmm0.f[0]; /* movss */

loc_002217EA: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFFFFFFu) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0xFFFFFFFFu (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0022181B; /* jne: not equal / not zero */

loc_002217EF: ;
    MEMF(esi + 0x18) = xmm1.f[0]; /* movss */
    MEMF(esi + 0x34) = xmm1.f[0]; /* movss */
    MEMF(esi + 0x50) = xmm3.f[0]; /* movss */
    MEMF(esi + 0x6C) = xmm3.f[0]; /* movss */
    goto loc_0022182F;

loc_00221805: ;
    MEMF(esi + 0x68) = xmm0.f[0]; /* movss */
    MEMF(esi + 0x4C) = xmm2.f[0]; /* movss */
    MEMF(esi + 0x30) = xmm0.f[0]; /* movss */
    MEMF(esi + 0x14) = xmm2.f[0]; /* movss */
    goto loc_002217EA;

loc_0022181B: ;
    MEMF(esi + 0x18) = xmm3.f[0]; /* movss */
    MEMF(esi + 0x34) = xmm3.f[0]; /* movss */
    MEMF(esi + 0x50) = xmm1.f[0]; /* movss */
    MEMF(esi + 0x6C) = xmm1.f[0]; /* movss */

loc_0022182F: ;
    edx = ZX16(MEM16(esp + 0xC));
    xmm0.f[0] = xmm0.f[0] - xmm2.f[0]; /* subss */
    xmm0.f[0] = xmm0.f[0] * MEMF(edi + 0x14); /* mulss */
    ecx = ZX16(LO16(ebx));
    xmm2.f[0] = (float)(int32_t)ecx; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * xmm2.f[0]; /* mulss */
    xmm1.f[0] = xmm1.f[0] - xmm3.f[0]; /* subss */
    xmm2.f[0] = (float)(int32_t)edx; /* cvtsi2ss */

loc_00221850: ;
    eax = MEM32(esp + 0x18);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    xmm1.f[0] = xmm1.f[0] * MEMF(edi + 0x18); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x43B494); /* mulss */
    xmm1.f[0] = xmm1.f[0] * xmm2.f[0]; /* mulss */
    if (CMP_EQ(_fa, _fb)) goto loc_0022189B; /* je: equal / zero */

loc_00221869: ;
    ecx = MEM32(eax);
    fp_push((double)SMEM32(eax)); /* fild */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_GE(_fas, _fbs)) goto loc_00221877; /* jge: greater or equal (signed >=) */

loc_00221871: ;
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148E4)); /* fadd dword ptr [0x4148e4] */

loc_00221877: ;
    edx = MEM32(eax + 4);
    MEMF(esp + 0xC) = (float)fp_top(); fp_pop(); /* fstp */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    fp_push((double)SMEM32(eax + 4)); /* fild */
    if (CMP_GE(_fas, _fbs)) goto loc_0022188B; /* jge: greater or equal (signed >=) */

loc_00221885: ;
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148E4)); /* fadd dword ptr [0x4148e4] */

loc_0022188B: ;
    xmm0 = XMM_SCALAR(MEMF(esp + 0xC)); /* movss */
    MEMF(esp + 0x10) = (float)fp_top(); fp_pop(); /* fstp */
    xmm1 = XMM_SCALAR(MEMF(esp + 0x10)); /* movss */

loc_0022189B: ;
    xmm6 = XMM_ZERO(); /* xorps self = zero */
    _fca = xmm6.f[0]; _fcb = xmm0.f[0]; /* comiss */
    if ((_fca >= _fcb)) goto loc_00221C61; /* jae: above or equal (unsigned >=) (xmm6.f[0] vs xmm0.f[0]) */

loc_002218A7: ;
    _fca = xmm6.f[0]; _fcb = xmm1.f[0]; /* comiss */
    if ((_fca >= _fcb)) goto loc_00221C61; /* jae: above or equal (unsigned >=) (xmm6.f[0] vs xmm1.f[0]) */

loc_002218B0: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(eax, MEM8(edi + 0xA));
    eax = eax & 0xF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fb = (uint32_t)(0) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - 0;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    if ((_fa == 0)) goto loc_002218EE; /* je: equal / zero */

loc_002218BD: ;
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa == 0)) goto loc_002218D5; /* je: equal / zero */

loc_002218C0: ;
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    xmm3 = XMM_SCALAR(MEMF(0x4148C0)); /* movss */
    if ((_fa != 0)) goto loc_00221918; /* jne: not equal / not zero */

loc_002218CB: ;
    xmm4.f[0] = xmm4.f[0] - xmm0.f[0]; /* subss */
    xmm4.f[0] = xmm4.f[0] - xmm3.f[0]; /* subss */
    goto loc_002218FE;

loc_002218D5: ;
    xmm2 = XMM_SCALAR(MEMF(0x4148C0)); /* movss */
    xmm3 = xmm0; /* movaps */
    xmm3.f[0] = xmm3.f[0] * xmm2.f[0]; /* mulss */
    xmm4.f[0] = xmm4.f[0] - xmm3.f[0]; /* subss */
    xmm4.f[0] = xmm4.f[0] - xmm2.f[0]; /* subss */
    goto loc_002218F6;

loc_002218EE: ;
    xmm4.f[0] = xmm4.f[0] - MEMF(0x4148C0); /* subss */

loc_002218F6: ;
    xmm3 = XMM_SCALAR(MEMF(0x4148C0)); /* movss */

loc_002218FE: ;
    xmm2 = xmm4; /* movaps */
    xmm2.f[0] = xmm2.f[0] + xmm0.f[0]; /* addss */
    MEMF(esi) = xmm4.f[0]; /* movss */
    MEMF(esi + 0x1C) = xmm2.f[0]; /* movss */
    MEMF(esi + 0x38) = xmm4.f[0]; /* movss */
    MEMF(esi + 0x54) = xmm2.f[0]; /* movss */

loc_00221918: ;
    eax = ZX8(MEM8(edi + 0xA));
    eax = eax >> 4;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(0) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - 0;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    if ((_fa == 0)) goto loc_0022193B; /* je: equal / zero */

loc_00221924: ;
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa == 0)) goto loc_00221930; /* je: equal / zero */

loc_00221927: ;
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa != 0)) goto loc_0022195A; /* jne: not equal / not zero */

loc_0022192A: ;
    xmm5.f[0] = xmm5.f[0] - xmm1.f[0]; /* subss */
    goto loc_0022193B;

loc_00221930: ;
    xmm0 = xmm1; /* movaps */
    xmm0.f[0] = xmm0.f[0] * xmm3.f[0]; /* mulss */
    xmm5.f[0] = xmm5.f[0] - xmm0.f[0]; /* subss */

loc_0022193B: ;
    xmm5.f[0] = xmm5.f[0] - xmm3.f[0]; /* subss */
    xmm0 = xmm5; /* movaps */
    MEMF(esi + 4) = xmm0.f[0]; /* movss */
    MEMF(esi + 0x20) = xmm0.f[0]; /* movss */
    xmm0.f[0] = xmm0.f[0] + xmm1.f[0]; /* addss */
    MEMF(esi + 0x3C) = xmm0.f[0]; /* movss */
    MEMF(esi + 0x58) = xmm0.f[0]; /* movss */

loc_0022195A: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    MEMF(esi + 8) = xmm6.f[0]; /* movss */
    MEMF(esi + 0x24) = xmm6.f[0]; /* movss */
    MEMF(esi + 0x40) = xmm6.f[0]; /* movss */
    MEMF(esi + 0x5C) = xmm6.f[0]; /* movss */
    MEMF(esi + 0xC) = xmm0.f[0]; /* movss */
    MEMF(esi + 0x28) = xmm0.f[0]; /* movss */
    MEMF(esi + 0x44) = xmm0.f[0]; /* movss */
    MEMF(esi + 0x60) = xmm0.f[0]; /* movss */
    eax = ZX8(MEM8(edi + 0x2B));
    MEM32(esp + 0x10) = eax;
    fp_push((double)SMEM32(esp + 0x10)); /* fild */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(edi + 0x20)); /* fmul dword ptr [edi + 0x20] */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x43B498)); /* fmul dword ptr [0x43b498] */
    PUSH32(esp, 0x002219A4u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_002219A4: ;
    ecx = ZX8(MEM8(edi + 0x2A));
    MEM32(esp + 0x10) = ecx;
    ebx = eax;
    ebx = ebx << 8;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    fp_push((double)SMEM32(esp + 0x10)); /* fild */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(edi + 0x1C)); /* fmul dword ptr [edi + 0x1c] */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x43B49C)); /* fmul dword ptr [0x43b49c] */
    PUSH32(esp, 0x002219C3u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_002219C3: ;
    edx = ZX8(MEM8(edi + 0x29));
    MEM32(esp + 0x10) = edx;
    eax = eax & 0xFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ebx = ebx | eax;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    fp_push((double)SMEM32(esp + 0x10)); /* fild */
    ebx = ebx << 8;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(edi + 0x1C)); /* fmul dword ptr [edi + 0x1c] */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x43B49C)); /* fmul dword ptr [0x43b49c] */
    PUSH32(esp, 0x002219E7u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_002219E7: ;
    eax = eax & 0xFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ebx = ebx | eax;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    eax = MEM32(edi + 0x28);
    eax = eax & 0xFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ebx = ebx << 8;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    MEM32(esp + 0x10) = eax;
    fp_push((double)SMEM32(esp + 0x10)); /* fild */
    if (CMP_GE(_fas, _fbs)) goto loc_00221A0B; /* jge: greater or equal (signed >=) */

loc_00221A05: ;
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148E4)); /* fadd dword ptr [0x4148e4] */

loc_00221A0B: ;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(edi + 0x1C)); /* fmul dword ptr [edi + 0x1c] */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x43B49C)); /* fmul dword ptr [0x43b49c] */
    PUSH32(esp, 0x00221A19u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_00221A19: ;
    eax = eax & 0xFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ebx = ebx | eax;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    ecx = ebx;
    edx = ebx;
    MEM32(esi + 0x10) = ebx;
    MEM32(esi + 0x2C) = ebx;
    MEM32(esi + 0x48) = ecx;
    MEM32(esi + 0x64) = edx;
    xmm0 = XMM_SCALAR(MEMF(edi + 0x24)); /* movss */
    _fca = xmm0.f[0]; _fcb = MEMF(0x414904); /* comiss */
    if ((_fca > _fcb)) goto loc_00221A50; /* ja: above (unsigned >) (xmm0.f[0] vs MEMF(0x414904)) */

loc_00221A3E: ;
    xmm0 = XMM_SCALAR(MEMF(0x414980)); /* movss */
    _fca = xmm0.f[0]; _fcb = MEMF(edi + 0x24); /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_00221C23; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs MEMF(edi + 0x24)) */

loc_00221A50: ;
    fp_push(MEMF(edi + 0x24)); /* fld float */
    ecx = ZX8(MEM8(edi + 0xA));
    { double _t = fp_top(); fp_push(_t); } /* fld st(0) */
    eax = ecx;
    fp_top() = sin(fp_top()); /* fsin */
    eax = eax & 0xF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fb = (uint32_t)(0) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - 0;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    MEMF(esp + 0x10) = (float)fp_top(); fp_pop(); /* fstp */
    fp_top() = cos(fp_top()); /* fcos */
    MEMF(esp + 0xC) = (float)fp_top(); fp_pop(); /* fstp */
    if ((_fa == 0)) goto loc_00221A93; /* je: equal / zero */

loc_00221A6F: ;
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa == 0)) goto loc_00221A7C; /* je: equal / zero */

loc_00221A72: ;
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa != 0)) goto loc_00221A99; /* jne: not equal / not zero */

loc_00221A75: ;
    xmm1 = XMM_SCALAR(MEMF(esi + 0x1C)); /* movss */
    goto loc_00221A9F;

loc_00221A7C: ;
    xmm1 = XMM_SCALAR(MEMF(esi + 0x1C)); /* movss */
    xmm1.f[0] = xmm1.f[0] - MEMF(esi); /* subss */
    xmm1.f[0] = xmm1.f[0] * MEMF(0x4148C0); /* mulss */
    xmm1.f[0] = xmm1.f[0] + MEMF(esi); /* addss */
    goto loc_00221A9F;

loc_00221A93: ;
    xmm1 = XMM_SCALAR(MEMF(esi)); /* movss */
    goto loc_00221A9F;

loc_00221A99: ;
    xmm1 = XMM_SCALAR(MEMF(esp + 0x10)); /* movss */

loc_00221A9F: ;
    ecx = ecx >> 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(0) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    ecx = ecx - 0;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    if ((_fa == 0)) goto loc_00221ACD; /* je: equal / zero */

loc_00221AA7: ;
    ecx--;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa == 0)) goto loc_00221AB4; /* je: equal / zero */

loc_00221AAA: ;
    ecx--;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa != 0)) goto loc_00221AD4; /* jne: not equal / not zero */

loc_00221AAD: ;
    xmm0 = XMM_SCALAR(MEMF(esi + 0x3C)); /* movss */
    goto loc_00221ADA;

loc_00221AB4: ;
    xmm0 = XMM_SCALAR(MEMF(esi + 0x3C)); /* movss */
    xmm0.f[0] = xmm0.f[0] - MEMF(esi + 4); /* subss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x4148C0); /* mulss */
    xmm0.f[0] = xmm0.f[0] + MEMF(esi + 4); /* addss */
    goto loc_00221ADA;

loc_00221ACD: ;
    xmm0 = XMM_SCALAR(MEMF(esi + 4)); /* movss */
    goto loc_00221ADA;

loc_00221AD4: ;
    xmm0 = XMM_SCALAR(MEMF(esp + 0x10)); /* movss */

loc_00221ADA: ;
    xmm3 = XMM_SCALAR(MEMF(esp + 0xC)); /* movss */
    xmm2 = XMM_SCALAR(MEMF(esp + 0x10)); /* movss */
    xmm5 = XMM_SCALAR(MEMF(esi)); /* movss */
    xmm6 = XMM_SCALAR(MEMF(esi + 4)); /* movss */
    xmm6.f[0] = xmm6.f[0] - xmm0.f[0]; /* subss */
    xmm4 = xmm6; /* movaps */
    xmm5.f[0] = xmm5.f[0] - xmm1.f[0]; /* subss */
    xmm6.f[0] = xmm6.f[0] * xmm3.f[0]; /* mulss */
    xmm7 = xmm5; /* movaps */
    xmm7.f[0] = xmm7.f[0] * xmm3.f[0]; /* mulss */
    xmm5.f[0] = xmm5.f[0] * xmm2.f[0]; /* mulss */
    xmm4.f[0] = xmm4.f[0] * xmm2.f[0]; /* mulss */
    xmm4.f[0] = xmm4.f[0] + xmm7.f[0]; /* addss */
    xmm4.f[0] = xmm4.f[0] + xmm1.f[0]; /* addss */
    MEMF(esi) = xmm4.f[0]; /* movss */
    xmm4 = XMM_ZERO(); /* xorps self = zero */
    xmm7 = xmm4; /* movaps */
    xmm7.f[0] = xmm7.f[0] - xmm5.f[0]; /* subss */
    xmm5 = XMM_SCALAR(MEMF(esi + 0x1C)); /* movss */
    xmm7.f[0] = xmm7.f[0] + xmm6.f[0]; /* addss */
    xmm6 = XMM_SCALAR(MEMF(esi + 0x20)); /* movss */
    xmm6.f[0] = xmm6.f[0] - xmm0.f[0]; /* subss */
    MEMF(esp + 0xC) = xmm6.f[0]; /* movss */
    xmm6.f[0] = xmm6.f[0] * xmm2.f[0]; /* mulss */
    xmm5.f[0] = xmm5.f[0] - xmm1.f[0]; /* subss */
    xmm7.f[0] = xmm7.f[0] + xmm0.f[0]; /* addss */
    MEMF(esi + 4) = xmm7.f[0]; /* movss */
    xmm7 = xmm5; /* movaps */
    xmm5.f[0] = xmm5.f[0] * xmm2.f[0]; /* mulss */
    xmm7.f[0] = xmm7.f[0] * xmm3.f[0]; /* mulss */
    xmm6.f[0] = xmm6.f[0] + xmm7.f[0]; /* addss */
    xmm6.f[0] = xmm6.f[0] + xmm1.f[0]; /* addss */
    MEMF(esi + 0x1C) = xmm6.f[0]; /* movss */
    xmm6 = xmm4; /* movaps */
    xmm6.f[0] = xmm6.f[0] - xmm5.f[0]; /* subss */
    xmm5 = XMM_SCALAR(MEMF(esp + 0xC)); /* movss */
    xmm5.f[0] = xmm5.f[0] * xmm3.f[0]; /* mulss */
    xmm6.f[0] = xmm6.f[0] + xmm5.f[0]; /* addss */
    xmm5 = XMM_SCALAR(MEMF(esi + 0x38)); /* movss */
    xmm6.f[0] = xmm6.f[0] + xmm0.f[0]; /* addss */
    MEMF(esi + 0x20) = xmm6.f[0]; /* movss */
    xmm6 = XMM_SCALAR(MEMF(esi + 0x3C)); /* movss */
    xmm6.f[0] = xmm6.f[0] - xmm0.f[0]; /* subss */
    MEMF(esp + 0xC) = xmm6.f[0]; /* movss */
    xmm5.f[0] = xmm5.f[0] - xmm1.f[0]; /* subss */
    xmm6.f[0] = xmm6.f[0] * xmm2.f[0]; /* mulss */
    xmm7 = xmm5; /* movaps */
    xmm7.f[0] = xmm7.f[0] * xmm3.f[0]; /* mulss */
    xmm6.f[0] = xmm6.f[0] + xmm7.f[0]; /* addss */
    xmm6.f[0] = xmm6.f[0] + xmm1.f[0]; /* addss */
    MEMF(esi + 0x38) = xmm6.f[0]; /* movss */
    xmm5.f[0] = xmm5.f[0] * xmm2.f[0]; /* mulss */
    xmm6 = xmm4; /* movaps */
    xmm6.f[0] = xmm6.f[0] - xmm5.f[0]; /* subss */
    xmm5 = XMM_SCALAR(MEMF(esp + 0xC)); /* movss */
    xmm5.f[0] = xmm5.f[0] * xmm3.f[0]; /* mulss */
    xmm6.f[0] = xmm6.f[0] + xmm5.f[0]; /* addss */
    xmm5 = XMM_SCALAR(MEMF(esi + 0x54)); /* movss */
    xmm6.f[0] = xmm6.f[0] + xmm0.f[0]; /* addss */
    MEMF(esi + 0x3C) = xmm6.f[0]; /* movss */
    xmm6 = XMM_SCALAR(MEMF(esi + 0x58)); /* movss */
    xmm5.f[0] = xmm5.f[0] - xmm1.f[0]; /* subss */
    xmm6.f[0] = xmm6.f[0] - xmm0.f[0]; /* subss */
    MEMF(esp + 0xC) = xmm6.f[0]; /* movss */
    xmm7 = xmm5; /* movaps */
    xmm6.f[0] = xmm6.f[0] * xmm2.f[0]; /* mulss */
    xmm7.f[0] = xmm7.f[0] * xmm3.f[0]; /* mulss */
    xmm6.f[0] = xmm6.f[0] + xmm7.f[0]; /* addss */
    xmm6.f[0] = xmm6.f[0] + xmm1.f[0]; /* addss */
    xmm1 = XMM_SCALAR(MEMF(esp + 0xC)); /* movss */
    xmm5.f[0] = xmm5.f[0] * xmm2.f[0]; /* mulss */
    xmm4.f[0] = xmm4.f[0] - xmm5.f[0]; /* subss */
    xmm1.f[0] = xmm1.f[0] * xmm3.f[0]; /* mulss */
    xmm4.f[0] = xmm4.f[0] + xmm1.f[0]; /* addss */
    xmm4.f[0] = xmm4.f[0] + xmm0.f[0]; /* addss */
    MEMF(esi + 0x54) = xmm6.f[0]; /* movss */
    MEMF(esi + 0x58) = xmm4.f[0]; /* movss */

loc_00221C23: ;
    edi = MEM32(edi);
    eax = edi + -263;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(7) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 7 (32-bit) */
    if (CMP_A(_fa, _fb)) goto loc_00221C55; /* ja: above (unsigned >) */

loc_00221C30: ;
    eax = ZX8(MEM8(eax + 0x221C74));
    { uint32_t _jt = MEM32(eax * 4 + 0x221C6C); /* switch: 2 entries, 2 targets */
    if (_jt == 0x00221C3Eu) goto loc_00221C3E;
    if (_jt == 0x00221C50u) goto loc_00221C50;
    g_seh_ebp = ebp; RECOMP_ITAIL(_jt); return; }

loc_00221C3E: ;
    ecx = MEM32(esi + 0x38);
    POP32(esp, edi);
    POP32(esp, ebp);
    MEM32(esi + 0x54) = ecx;
    eax = 1;
    POP32(esp, ebx);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 4; return; /* ret */

loc_00221C50: ;
    edx = MEM32(esi + 0x1C);
    MEM32(esi) = edx;

loc_00221C55: ;
    POP32(esp, edi);
    POP32(esp, ebp);
    eax = 1;
    POP32(esp, ebx);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 4; return; /* ret */

loc_00221C61: ;
    POP32(esp, edi);
    POP32(esp, ebp);
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    POP32(esp, ebx);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 4; return; /* ret */

    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}

