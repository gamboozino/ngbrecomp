/*
 * pc_menu.c -- the PC settings in the game's own Options menu.
 *
 * Ninja Gaiden Black's Options menu is data driven: a table of pages
 * (0x429CD0, 0x18 bytes each: id, first and last row, wrap, ..., row table)
 * whose rows (0x1C bytes: index, action -- a page to open, 0xFF a value,
 * 0xFC defaults, 0xFD exit -- type, range, page handler) the menu engine
 * moves the cursor over and reports value changes for. The text comes from
 * a table per category (0x436240, counts at 0x43626C; entries of id,
 * English, Japanese). What is page specific is code: each page draws its
 * rows from its own list of text ids, handles its own values, and the
 * information line picks a description by page and row.
 *
 * The main menu's RECOMP SETTINGS (src/main_menu.c) opens this menu as
 * OPTIONS does, but on pages of ours: page 0's categories become
 * Graphics, Display and Startup Settings, Gameplay, Cheats and Controls,
 * opening pages 10-15 (no Exit row: the column has six, and B leaves) --
 * the recomp's settings as rows with values, like Video Settings'; Cheats'
 * row Unlocks puts the unlocks' rows in its page's place (unl_show) -- and
 * the header and category words are pictures of ours (src/pause_tab.c)
 * while it is open. The page and row tables and the text table are
 * extended in guest memory at first use, and the page table pointer is
 * swapped for ours while RECOMP SETTINGS is open; the four functions below
 * are the lifted ones (listed in src/manual_functions.json so the
 * pipeline leaves them out) with the changes marked "PC:".
 */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>


/* ---------- the PC rows ---------- */

#include "xbox_memory_layout.h"   /* xbox_HeapAlloc: guest memory */
#include "port.h"
#include "launcher.h"                 /* ngb_vsync_name, ngb_vsync_step */

/* Settings, from src/ingame_menu.c: 0 render scale (1-8), 1 smooth
 * texture filtering, 2 shadow map scale (1, 2, 4, 8), 3 fullscreen,
 * 4 V-Sync, 5 the FPS counter, 6 the window size, 7 the aspect ratio. */
int ngb_pc_setting(int which);
void ngb_pc_set_setting(int which, int value);
int ngb_settings_size_count(int aspect);   /* src/launcher.c */
void ngb_settings_size_at(int aspect, int i, int *w, int *h);

#define MENU_STATE 0xDA41D0       /* the Options menu: page, row, ... */
#define PAGE_GFX   10             /* ours: Graphics, Display, Sound, Gameplay, Cheats, Controls */
#define PAGE_DISP  11
#define PAGE_SOUND 12
#define PAGE_GAME  13             /* Gameplay: the optional changes to play (the Quick Menu) */
#define PAGE_CHEAT 14
#define PAGE_CTRL  15             /* Controls: the bindings' panel (src/pause_tab.c) over it */
#define N_OURS     6
#define UNL_ACTION 16             /* Cheats' row Unlocks: its rows in the page's place (unl_show), not a page */
#define PC_CLOSER  8              /* a page of more than seven rows: closer, clear of the information line */

enum { ROW_SCREEN, ROW_MOVIE, ROW_PC, ROW_DEFAULT, ROW_EXIT,
       ROW_RENDER, ROW_TEXTURE, ROW_SHADOW, ROW_DISPLAY, ROW_VSYNC, ROW_FPS, ROW_WINDOW, ROW_ASPECT, ROW_BACK,
       ROW_SKIP, ROW_LAUNCHER, ROW_MSAA, ROW_HEALTH, ROW_NINPO, ROW_ESSENCE, ROW_ALLDIFF, ROW_SCARABS, ROW_ALLCOST, ROW_EXTRACOST,
       ROW_GLOW, ROW_QUICKFREEZE, ROW_QUICKSCALE, ROW_INFITEMS, ROW_INFAMMO, ROW_POSAUDIO, ROW_HUD, ROW_AUDIOOUT,
       ROW_MISSIONS, ROW_ARCADE, ROW_MOVIES, ROW_UNLOCKS, ROW_LOADDETAIL, ROW_RTHREAD, ROW_SPARKTHICKNESS,
};

static int is_ours(int page) { return page >= PAGE_GFX && page < PAGE_GFX + N_OURS; }

/* Video Settings with the console's widescreen option (page 4) and without
 * (page 6) -- their rows were Screen Adjustment, Movie Display Ratio,
 * Default Settings, Exit and Screen Adjustment, Exit -- and PC Settings. */
/* (without the PC Settings row: the PC settings are in RECOMP SETTINGS only) */
static const uint8_t s_rows_ws[] = { ROW_SCREEN, ROW_MOVIE, ROW_DEFAULT, ROW_EXIT };
static const uint8_t s_rows_std[] = { ROW_SCREEN, ROW_EXIT };
static const uint8_t s_rows_gfx[] = { ROW_RENDER, ROW_TEXTURE, ROW_MSAA, ROW_SHADOW, ROW_GLOW, ROW_LOADDETAIL,
                                      ROW_RTHREAD, ROW_SPARKTHICKNESS, ROW_BACK };
static const uint8_t s_rows_disp[] = { ROW_ASPECT, ROW_WINDOW, ROW_DISPLAY, ROW_VSYNC, ROW_FPS, ROW_HUD, ROW_SKIP,
                                       ROW_BACK };
static const uint8_t s_rows_sound[] = { ROW_POSAUDIO, ROW_AUDIOOUT, ROW_BACK };
static const uint8_t s_rows_game[] = { ROW_QUICKSCALE, ROW_QUICKFREEZE, ROW_BACK };
static const uint8_t s_rows_cheat[] = { ROW_HEALTH, ROW_NINPO, ROW_ESSENCE, ROW_SCARABS, ROW_INFITEMS, ROW_INFAMMO,
                                        ROW_UNLOCKS, ROW_BACK };
static const uint8_t s_rows_ctrl[] = { ROW_BACK };
static const uint8_t s_rows_unl[] = { ROW_ALLDIFF, ROW_ALLCOST, ROW_MISSIONS, ROW_ARCADE, ROW_MOVIES, ROW_BACK };
static const struct { const uint8_t *kinds; int n; } s_ours[N_OURS] = {
    { s_rows_gfx, (int)sizeof s_rows_gfx },
    { s_rows_disp, (int)sizeof s_rows_disp },
    { s_rows_sound, (int)sizeof s_rows_sound },
    { s_rows_game, (int)sizeof s_rows_game },
    { s_rows_cheat, (int)sizeof s_rows_cheat },
    { s_rows_ctrl, (int)sizeof s_rows_ctrl },
};
#define N_UNL (int)sizeof s_rows_unl
#define N_WS  (int)sizeof s_rows_ws
#define N_STD (int)sizeof s_rows_std

/* New text, category 2 (the Options menu's). */
#define T_QUICKFREEZE 0x201B7
#define D_QUICKFREEZE 0x201B8
#define D_GAMEPLAY 0x201B9            /* the category */
#define T_INFITEMS 0x201BA            /* the cheats: infinite items, infinite projectiles */
#define T_INFAMMO  0x201BB
#define D_INFITEMS 0x201BC
#define D_INFAMMO  0x201BD
#define T_QUICKSCALE  0x201C0
#define D_QUICKSCALE  0x201C1
#define T_QUICKSCALE0 0x201C2        /* + (150 - size) / 25: 150%, 125%, 100% ... 25%, Off (+6) */
#define T_POSAUDIO 0x201C9            /* Positional Audio (the APU's HRTF filters, #30) */
#define D_POSAUDIO 0x201CA
#define T_POSCONSOLE 0x201CD          /* Positional Audio's values: Console, Panning */
#define T_POSPAN   0x201CE
#define T_HUD      0x201CB            /* the HUD on or off (src/hud.c, #80) */
#define D_HUD      0x201CC
#define T_AUDIOOUT 0x201CF            /* Audio Output: Auto, Stereo, 5.1 (src/audio) */
#define D_AUDIOOUT 0x201D0
#define T_AUTO     0x201D1
#define T_STEREO   0x201D2
#define T_SURROUND 0x201D3
#define T_PC       0x20100
#define T_RENDER   0x20101
#define T_TEXTURE  0x20102
#define T_SHADOW   0x20103
#define T_DISPLAY  0x20104
#define T_VSYNC    0x20105
#define T_FPS      0x20106
#define T_SCALE0   0x20180            /* + 0..9: Native (1x) .. 10x */
#define T_ANISO0   0x20190            /* + 0..4: Off, 2x, 4x, 8x, 16x */
#define T_MSAA0    0x20198            /* + 0..3: Off, 2x, 4x, 8x */
#define T_MSAA     0x2019C
#define D_MSAA     0x2019D
#define T_SMOOTH   0x20118
#define T_ORIGINAL 0x20119
#define T_SHADOW0  0x2011A            /* + 0..3: Original, 2x, 4x, 8x */
#define T_SHADOW10 0x2019E            /* 10x */
#define T_FULL     0x2011E
#define T_WINDOW   0x2011F
#define T_ON       0x20016            /* the game's own On and Off */
#define T_OFF      0x20017
#define D_PC       0x20120
#define D_RENDER   0x20121
#define D_TEXTURE  0x20122
#define D_SHADOW   0x20123
#define D_DISPLAY  0x20124
#define D_VSYNC    0x20125
#define D_FPS      0x20126
#define T_ADAPTIVE 0x20127
#define T_MAILBOX  0x20128
#define T_FIFO     0x20129
#define T_BORDERLESS 0x2012C
#define T_ASPECT   0x2012D
#define D_ASPECT   0x2012E
#define T_ASPECT0  0x20170            /* + 0..3: 4:3, 16:9, 16:10, 21:9 (not in T_WSIZE0's 32) */
#define T_SKIP     0x20160
#define T_LAUNCHER 0x20161
#define T_YES      0x20162
#define T_NO       0x20163
#define D_SKIP     0x20164
#define D_LAUNCHER 0x20165
#define D_CAT0     0x20166            /* + 0..2: the categories */
#define D_BACK     0x20169
#define T_HEALTH   0x201A0            /* the cheats */
#define T_NINPO    0x201A1
#define T_ESSENCE  0x201A2
#define D_HEALTH   0x201A3
#define D_NINPO    0x201A4
#define D_ESSENCE  0x201A5
#define D_CHEATS   0x201A6            /* the category */
#define T_ALLDIFF  0x201A7            /* every difficulty from the start */
#define D_ALLDIFF  0x201A8
#define T_SCARABS  0x201A9
#define D_SCARABS  0x201AA
#define T_ALLCOST  0x201AB
#define D_ALLCOST  0x201AC
#define T_EXTRACOST 0x201AD
#define D_EXTRACOST 0x201AE
#define T_EXTRA0   0x201B0            /* + 0..3: Game's Choice, Extra 1-3 */
#define D_CONTROLS 0x201B4            /* the category */
#define T_GLOW     0x201B5            /* Smoother Glow (src/d3d/vk/glow_options.h) */
#define D_GLOW     0x201B6
#define T_MISSIONS 0x201D4            /* the unlocks (src/cheats.c): the main menu's extras */
#define T_ARCADE   0x201D5
#define T_MOVIES   0x201D6
#define T_UNLOCKS  0x201D7            /* Cheats' row opening the Unlocks page */
#define D_MISSIONS 0x201D8
#define D_ARCADE   0x201D9
#define D_MOVIES   0x201DA
#define D_UNLOCKS  0x201DB
#define T_LOADDETAIL 0x201DC          /* Lower Detail Under Load (src/d3d/d3d.c) */
#define D_LOADDETAIL 0x201DD
#define T_RTHREAD  0x201DE            /* Render Thread, experimental (src/d3d/gpu/threaded.cpp) */
#define D_RTHREAD  0x201DF
#define T_SPARKTHICKNESS 0x201E0          /* the impact sparks' width (src/d3d/vk/spark_aspect.h) */
#define D_SPARKTHICKNESS 0x201E1
#define T_SPARKTHICKNESS0  0x201E2             /* + 0..3: Very Thin, Thin, Slightly Thin, Original */
#define T_WINDOWSZ 0x2012A
#define D_WINDOWSZ 0x2012B
#define T_WSIZE0   0x20140            /* + aspect * 8 + index: "1280 x 720" */

static const struct { uint32_t id; const char *text; } s_text[] = {
    { T_QUICKFREEZE, "Quick Menu Pause" },
    { T_POSAUDIO, "Positional Audio" },
    { T_POSCONSOLE, "Console" },
    { T_POSPAN, "Panning" },
    { T_HUD, "HUD" },
    { D_HUD, "The health and Ninpo bar, weapons and the rest drawn over play. Off: a clear screen." },
    { T_AUDIOOUT, "Audio Output" },
    { D_AUDIOOUT, "Speakers. Auto: 5.1 when the output device has 5.1 or 7.1 channels, else stereo. From the next start." },
    { T_AUTO, "Auto" },
    { T_STEREO, "Stereo" },
    { T_SURROUND, "5.1" },
    { D_POSAUDIO, "Sounds placed left and right. Console: the sound chip's filters. Panning: their placement only, clearer." },
    { T_QUICKSCALE, "Quick Menu" },
    { D_QUICKSCALE, "A d-pad menu of weapons, Ninpo and items (Up and Down open it), at this size." },
    { D_GAMEPLAY, "Optional changes to how the game plays, each off unless chosen: the Quick Menu." },
    { T_MISSIONS, "Missions" },
    { T_ARCADE,   "Arcade Ninja Gaiden" },
    { T_MOVIES,   "Movie Theater" },
    { T_UNLOCKS,  "Unlocks" },
    { D_MISSIONS, "MISSIONS in the main menu without earning it." },
    { D_ARCADE,   "ARCADE NINJA GAIDEN in the main menu without earning it." },
    { D_MOVIES,   "MOVIE THEATER in the main menu without earning it." },
    { D_UNLOCKS,  "Difficulties, costumes and the main menu's extras, without earning them. Saved." },
    { T_INFITEMS, "Infinite Items" },
    { T_INFAMMO,  "Infinite Projectiles" },
    { D_INFITEMS, "Healing items, the Talisman of Rebirth and the Jewel of the Demon Seal are not used up.\nFor this session only." },
    { D_INFAMMO,  "Shuriken, smoke bombs, arrows and spears are not used up. For this session only." },
    { T_QUICKSCALE0 + 0, "150%" }, { T_QUICKSCALE0 + 1, "125%" },
    { T_QUICKSCALE0 + 2, "100%" }, { T_QUICKSCALE0 + 3, "75%" },
    { T_QUICKSCALE0 + 4, "50%" }, { T_QUICKSCALE0 + 5, "25%" },
    { T_QUICKSCALE0 + 6, "Off" },
    { D_QUICKFREEZE, "The game pauses while the quick menu is open (once Ryu's action ends)." },
    { T_PC,      "PC Settings" },
    { T_RENDER,  "Render Resolution" },
    { T_TEXTURE, "Anisotropic Filtering" },
    { T_MSAA,    "Anti-Aliasing (MSAA)" },
    { D_MSAA,    "Smooths the edges of the 3D; 2x to 8x samples a pixel." },
    { T_GLOW,    "Smoother Glow" },
    { D_GLOW,    "A softer, fainter glow on bright lights. Off: the console's." },
    { T_LOADDETAIL, "Lower Detail Under Load" },
    { D_LOADDETAIL, "Eases off detail when busy, as on the Xbox; Ryu's shadow may flicker." },
    { T_RTHREAD, "Experimental Render Thread" },
    { D_RTHREAD, "Drawing on a thread of its own. May glitch. Needs restart." },
    { T_SPARKTHICKNESS, "Spark Thickness" },
    { D_SPARKTHICKNESS, "The thickness of the sparks drawn on a weapon hit. Default: the console's." },
    { T_SPARKTHICKNESS0 + 0, "Very Thin" }, { T_SPARKTHICKNESS0 + 1, "Thin" }, { T_SPARKTHICKNESS0 + 2, "Slightly Thin" }, { T_SPARKTHICKNESS0 + 3, "Original" },
    { T_ANISO0 + 0, "Off" }, { T_ANISO0 + 1, "2x" }, { T_ANISO0 + 2, "4x" },
    { T_ANISO0 + 3, "8x" }, { T_ANISO0 + 4, "16x" },
    { T_MSAA0 + 0, "Off" }, { T_MSAA0 + 1, "2x" }, { T_MSAA0 + 2, "4x" }, { T_MSAA0 + 3, "8x" },
    { T_SHADOW,  "Shadow Resolution" },
    { T_DISPLAY, "Display" },
    { T_VSYNC,   "V-Sync" },
    { T_FPS,     "Show FPS" },
    { T_SCALE0 + 0, "1x 720x480" }, { T_SCALE0 + 1, "2x 1440x960" }, { T_SCALE0 + 2, "3x 2160x1440" },
    { T_SCALE0 + 3, "4x 2880x1920" }, { T_SCALE0 + 4, "5x 3600x2400" }, { T_SCALE0 + 5, "6x 4320x2880" },
    { T_SCALE0 + 6, "7x 5040x3360" }, { T_SCALE0 + 7, "8x 5760x3840" }, { T_SCALE0 + 8, "9x 6480x4320" },
    { T_SCALE0 + 9, "10x 7200x4800" },
    { T_SMOOTH,   "Smooth" },
    { T_ORIGINAL, "Original" },
    { T_SHADOW0 + 0, "Original" }, { T_SHADOW0 + 1, "2x" }, { T_SHADOW0 + 2, "4x" },
    { T_SHADOW0 + 3, "8x" }, { T_SHADOW10, "10x" },
    { T_FULL,    "Fullscreen" },
    { T_WINDOW,  "Windowed" },
    { T_ADAPTIVE, "Adaptive" },
    { T_MAILBOX, "Mailbox" },
    { T_FIFO,    "FIFO" },
    { T_BORDERLESS, "Borderless" },
    { T_ASPECT,  "Aspect Ratio" },
    { D_ASPECT,  "The shape of the picture and the window sizes offered." },
    { T_ASPECT0 + 0, "4:3" }, { T_ASPECT0 + 1, "16:9" }, { T_ASPECT0 + 2, "16:10" }, { T_ASPECT0 + 3, "21:9" },
    { T_WINDOWSZ, "Window Size" },
    { D_WINDOWSZ, "The size of the window when not full screen." },
    { D_PC,      "Graphics and display settings of the PC version." },
    { D_RENDER,  "The resolution the game is drawn at: the console's 720x480 times this." },
    { D_TEXTURE, "Keeps textures sharp at an angle; Off is the console's filtering." },
    { D_SHADOW,  "Sets the resolution of the character shadows." },
    { D_DISPLAY, "A window, the whole screen, or a window without a frame." },
    { D_VSYNC,   "No tearing. Mailbox: least delay. FIFO: steadiest. Adaptive: a late frame goes at once." },
    { D_FPS,     "Shows the frame rate in a corner of the screen." },
    { T_SKIP,    "Skip Intros" },
    { T_LAUNCHER, "Settings at Startup" },
    { T_YES,     "Yes" },
    { T_NO,      "No" },
    { D_SKIP,    "Starts at the title screen, from the next start." },
    { D_LAUNCHER, "Shows the settings screen before the game, from the next start." },
    { D_CAT0 + 0, "Resolution, texture filtering, anti-aliasing, shadows, the glow, and two options for slower PCs." },
    { D_CAT0 + 1, "Aspect ratio, window size, display mode, V-Sync, the FPS counter, the HUD and the intros." },
    { D_CAT0 + 2, "Positional audio and the speakers." },
    { D_BACK,    "Returns to the Recomp Settings screen." },
    { T_HEALTH,  "Infinite Health" },
    { T_NINPO,   "Infinite Ninpo" },
    { T_ESSENCE, "Infinite Essence" },
    { D_HEALTH,  "Ryu's health stays full. For this session only." },
    { D_NINPO,   "Ninpo is always ready. For this session only." },
    { D_ESSENCE, "As much essence as can be held. For this session only." },
    { D_CHEATS,  "Help for a hard spot: health, ninpo, essence, items and projectiles. Not saved. And the unlocks." },
    { T_ALLDIFF, "All Difficulties" },
    { D_ALLDIFF, "Every difficulty at New Game, Ninja Dog to Master Ninja." },
    { T_SCARABS, "Golden Scarabs" },
    { D_SCARABS, "All 50 Golden Scarabs: Muramasa gives every reward. For this session only." },
    { T_ALLCOST, "All Costumes" },
    { D_ALLCOST, "Every costume at the Costume Selection, and three it never offers." },
    { T_EXTRACOST, "Extra Costume" },
    { D_EXTRACOST, "Costumes in the game's files it never offers. From the next area." },
    { T_EXTRA0 + 0, "Game's Choice" }, { T_EXTRA0 + 1, "Extra 1" }, { T_EXTRA0 + 2, "Extra 2" },
    { T_EXTRA0 + 3, "Extra 3" },
    { D_CONTROLS, "The keys, mouse buttons and controller buttons used in play." },
};

static int s_ready;
static uint32_t s_labels_ws, s_labels_std;               /* guest: text ids */
static uint32_t s_labels_ours[N_OURS];
static uint32_t s_labels_unl, s_rowtab_unl, s_rowtab_cheat;   /* guest: Unlocks' rows and labels, Cheats' rows */
static int s_unl;                                         /* Cheats' page shows Unlocks' rows */
static void unl_show(int on, int row);
static uint32_t s_highlight;                              /* guest: a byte per row */
static uint32_t s_pages_recomp;                           /* guest: RECOMP SETTINGS' page table */

static uint32_t guest_string(const char *t)
{
    uint32_t n = (uint32_t)strlen(t) + 1, va = xbox_HeapAlloc(n, 4);
    if (va)
        memcpy((void *)XBOX_PTR(va), t, n);
    return va;
}

/* A row table, from the original rows (page 4's at 0x429968: Screen
 * Adjustment, Movie Display Ratio, Default Settings, Exit), ending as the
 * engine expects with an 0xFF row. */
static uint32_t make_rows(const uint8_t *kinds, int n)
{
    uint32_t va = xbox_HeapAlloc(0x1C * (n + 1), 4);
    for (int i = 0; i < n; i++) {
        uint32_t e = va + 0x1C * i, src;
        int max = 1;
        switch (kinds[i]) {
        case ROW_SCREEN:  src = 0x429968; break;
        case ROW_PC:      src = 0x429968; break;
        case ROW_UNLOCKS: src = 0x429968; break;          /* a row with an action, as Screen Adjustment's */
        case ROW_DEFAULT: src = 0x4299A0; break;
        case ROW_EXIT:
        case ROW_BACK:    src = 0x4299BC; break;
        case ROW_RENDER:  src = 0x429984; max = 9; break;
        case ROW_TEXTURE: src = 0x429984; max = 4; break;
        case ROW_MSAA:    src = 0x429984; max = 3; break;
        case ROW_SHADOW:  src = 0x429984; max = 4; break;
        case ROW_VSYNC:   src = 0x429984; max = 3; break;
        case ROW_WINDOW:  src = 0x429984; max = 6; break;
        case ROW_ASPECT:  src = 0x429984; max = 3; break;
        case ROW_EXTRACOST: src = 0x429984; max = 3; break;
        case ROW_QUICKSCALE: src = 0x429984; max = 10; break;
        default:          src = 0x429984; break;          /* a value, 0-1 */
        }
        memcpy((void *)XBOX_PTR(e), (void *)XBOX_PTR(src), 0x1C);
        MEM8(e) = (uint8_t)i;                              /* row */
        if (src == 0x429984)
            MEM32(e + 0xC) = (uint32_t)max;                /* value range */
        if (kinds[i] == ROW_UNLOCKS)
            MEM8(e + 1) = UNL_ACTION;                      /* (never a page: unl_show) */
    }
    memset((void *)XBOX_PTR(va + 0x1C * n), 0, 0x1C);
    MEM8(va + 0x1C * n) = 0xFF;
    return va;
}

static uint32_t make_labels(const uint8_t *kinds, int n)
{
    static const uint32_t id[] = { 0x20011, 0x20015, T_PC, 0x20004, 0x20005,
                                   T_RENDER, T_TEXTURE, T_SHADOW, T_DISPLAY, T_VSYNC,
                                   T_FPS, T_WINDOWSZ, T_ASPECT, 0x20005, T_SKIP, T_LAUNCHER, T_MSAA,
                                   T_HEALTH, T_NINPO, T_ESSENCE, T_ALLDIFF, T_SCARABS, T_ALLCOST, T_EXTRACOST,
                                   T_GLOW, T_QUICKFREEZE, T_QUICKSCALE, T_INFITEMS, T_INFAMMO, T_POSAUDIO, T_HUD,
                                   T_AUDIOOUT, T_MISSIONS, T_ARCADE, T_MOVIES, T_UNLOCKS, T_LOADDETAIL, T_RTHREAD, T_SPARKTHICKNESS };
    uint32_t va = xbox_HeapAlloc(4 * n, 4);
    for (int i = 0; i < n; i++)
        MEM32(va + 4 * i) = id[kinds[i]];
    return va;
}

/* Once, before the Options menu is drawn: the text, the rows, our page. */
static void pc_menu_init(void)
{
    if (s_ready)
        return;
    s_ready = 1;

    /* Category 2's text table with ours after it. */
    uint32_t old = MEM32(0x436240 + 2 * 4);
    uint32_t count = MEM16(0x43626C + 2 * 2);
    uint32_t add = sizeof s_text / sizeof s_text[0], extra = 4 * 8;
    uint32_t tab = xbox_HeapAlloc(12 * (count + add + extra), 4);
    memcpy((void *)XBOX_PTR(tab), (void *)XBOX_PTR(old), 12 * count);
    for (uint32_t i = 0; i < add + extra; i++) {
        char sz[24];
        const char *text = i < add ? s_text[i].text : "";
        uint32_t id = i < add ? s_text[i].id : T_WSIZE0 + (i - add);
        if (i >= add) {                                /* a window size, by aspect and place */
            int w, h, a = (int)(i - add) / 8, k = (int)(i - add) % 8;
            if (k < ngb_settings_size_count(a)) {
                ngb_settings_size_at(a, k, &w, &h);
                snprintf(sz, sizeof sz, "%d x %d", w, h);
                text = sz;
            } else {
                text = "-";
            }
        }
        uint32_t str = guest_string(text), e = tab + 12 * (count + i);
        MEM32(e) = id;
        MEM32(e + 4) = str;           /* English */
        MEM32(e + 8) = str;           /* and in the Japanese text too */
    }
    MEM32(0x436240 + 2 * 4) = tab;
    MEM16(0x43626C + 2 * 2) = (uint16_t)(count + add + extra);

    /* Pages 4 (0x429D18) and 6 (0x429D48): last row, row table. */
    MEM8(0x429D18 + 2) = N_WS - 1;
    MEM32(0x429D18 + 0xC) = make_rows(s_rows_ws, N_WS);
    MEM8(0x429D48 + 2) = N_STD - 1;
    MEM32(0x429D48 + 0xC) = make_rows(s_rows_std, N_STD);

    /* RECOMP SETTINGS' page table: the game's (0x429CB8: pages 0-9 of 0x18
     * bytes, then 0xFF) with page 0's categories ours -- rows opening pages
     * 10-15, no Exit -- and our pages 10-15, copies of Screen Adjustment's
     * (page 9: a page of Video Settings) going back to their category. */
    uint32_t pages = xbox_HeapAlloc(0x18 * (10 + N_OURS + 1), 4);
    memcpy((void *)XBOX_PTR(pages), (void *)XBOX_PTR(0x429CB8), 0x18 * 10);
    {
        uint32_t src = MEM32(0x429CB8 + 0xC), rows = xbox_HeapAlloc(0x1C * (N_OURS + 1), 4);
        for (int i = 0; i < N_OURS; i++) {                /* page 0's rows: each a category's (the game's */
            uint32_t e = rows + 0x1C * i;                  /* first five; the sixth as the fifth, not Exit) */
            memcpy((void *)XBOX_PTR(e), (void *)XBOX_PTR(src + 0x1C * (i < 5 ? i : 4)), 0x1C);
            MEM8(e) = (uint8_t)i;
            MEM8(e + 1) = (uint8_t)(PAGE_GFX + i);
        }
        memset((void *)XBOX_PTR(rows + 0x1C * N_OURS), 0, 0x1C);
        MEM8(rows + 0x1C * N_OURS) = 0xFF;
        MEM32(pages + 0xC) = rows;
        MEM8(pages + 2) = N_OURS - 1;                      /* the last row */
    }
    for (int i = 0; i < N_OURS; i++) {
        uint32_t pg = pages + 0x18 * (10 + i);
        memcpy((void *)XBOX_PTR(pg), (void *)XBOX_PTR(0x429D90), 0x18);
        MEM8(pg) = (uint8_t)(PAGE_GFX + i);
        MEM8(pg + 2) = (uint8_t)(s_ours[i].n - 1);
        MEM8(pg + 6) = 0;                                  /* back to page 0, */
        MEM8(pg + 7) = (uint8_t)i;                         /* its category's row */
        MEM32(pg + 0xC) = make_rows(s_ours[i].kinds, s_ours[i].n);
        s_labels_ours[i] = make_labels(s_ours[i].kinds, s_ours[i].n);
    }
    memset((void *)XBOX_PTR(pages + 0x18 * (10 + N_OURS)), 0, 0x18);
    MEM8(pages + 0x18 * (10 + N_OURS)) = 0xFF;
    s_pages_recomp = pages;
    s_rowtab_cheat = MEM32(pages + 0x18 * PAGE_CHEAT + 0xC);
    s_rowtab_unl = make_rows(s_rows_unl, N_UNL);
    s_labels_unl = make_labels(s_rows_unl, N_UNL);

    s_labels_ws = make_labels(s_rows_ws, N_WS);
    s_labels_std = make_labels(s_rows_std, N_STD);
    s_highlight = xbox_HeapAlloc(16, 4);
}

/* At startup, before the game runs: the Options menu reads the page table
 * once when it opens. */
void ngb_pause_tab_init(void);   /* src/pause_tab.c */
void ngb_general_page_init(void);   /* src/main_menu.c */
int ngb_general_no_flag(void);

/* The game's picture shape, while it runs or before it starts. The game has a widescreen mode
 * (the D3D present flags' bit 4, 0x9776A8, and the video flags' bit 0, 0xD21748): in it, it takes
 * its 3D's shape from 0x414AF8 (16:9) instead of 0x414AF4 (4:3) -- kept in 0x977EF8 as it set it
 * up -- and draws some of its 2D narrower for a widescreen TV to stretch (the title's logo, the
 * items' pictures, some bars), others not (the letters, the menus' plates). The renderer draws the
 * 2D in its own 4:3 shape on a wider screen (ngb_hud_shape, src/d3d/vk/backend.cpp), alike for
 * every piece, which those already narrower would be twice: so on a wider screen the game is kept
 * in its 4:3 mode -- its 2D as for 4:3 -- with both constants the screen's shape, its 3D drawn for
 * it. NGB_GAME_WIDESCREEN=1 keeps the game's own widescreen mode instead. */
extern uint32_t xbox_XcVideoFlags;

/* The constants are in .rdata (read-only): written through /proc/self/mem, and tried again from the
 * per-frame hook until the image is mapped. */
static double s_aspect_a;      /* the screen's shape to give the game, 0 none pending */
static float s_orig_43;        /* 0x414AF4 as the game has it */

static int patch_float(uint32_t va, float f)
{
    return ngb_mem_write((void *)XBOX_PTR(va), &f, sizeof f);
}

/* (fails, rather than faulting, while the image is not mapped) */
static int read_float(uint32_t va, float *f)
{
    return ngb_mem_read((const void *)XBOX_PTR(va), f, sizeof *f);
}

static int game_widescreen_mode(void)
{
    static int on = -1;
    if (on < 0)
        on = getenv("NGB_GAME_WIDESCREEN") != NULL;
    return on;
}

static int patch_constant(double a)
{
    if (!s_orig_43) {
        float f;
        if (!read_float(0x414AF4, &f) || !(f > 1.2f && f < 1.5f))
            return 0;                                  /* not mapped yet */
        s_orig_43 = f;
    }
    int wide = a > 1.34;
    return patch_float(0x414AF8, (float)a) &&
           patch_float(0x414AF4, wide && !game_widescreen_mode() ? (float)a : s_orig_43);
}

void ngb_game_set_aspect_constant(double a)
{
    s_aspect_a = patch_constant(a) ? 0 : a;
}

/* The flags as wanted on a wide screen: in play the game's 4:3 mode (its HUD and words, drawn for 4:3,
 * are narrowed to their own shape by the renderer) -- but its widescreen mode, the console's 16:9
 * look, for its 2D screens: while a menu runs (marked as it runs, so from its first frame), when
 * the frame before drew next to no 3D (the title, chapter cards, loading screens), while a movie
 * plays and until the title (the boot movie: a wide one is chosen). There it draws its pictures
 * narrower for the screen to stretch, and the renderer draws them as they are, stretched
 * (ngb_2d_stretched) -- no wider than 16:9. NGB_GAME_WIDESCREEN=1: the widescreen mode throughout;
 * NGB_MENUS_43=1: the 4:3 mode on the 2D screens too, narrowed as the HUD. */
static int s_flags_wide = -1, s_screen_wide, s_boot = 1, s_menu_now, s_menu_last, s_screen_2d;
static uint64_t s_movie_ms;
static uint64_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}
/* The movie player's state, polled each frame while a movie is up: playing (the first ends the boot,
 * whose wide mode is only for choosing the wide boot movie), or ended (4:3 again at once, so the title
 * after it is never drawn a frame in the wide mode). */
void ngb_movie_playing(int playing)
{
    s_movie_ms = playing ? now_ms() : 0;
    if (playing)
        s_boot = 0;
}
int ngb_movie_active(void) { return s_movie_ms && now_ms() - s_movie_ms < 200; }
void ngb_boot_over(void) { s_boot = 0; }   /* the title is up */

/* NGB_MENUS_43=1: on a wide screen the 2D screens in the game's 4:3 mode too, the console's 4:3 picture
 * in the middle of the screen (debugging; they follow the aspect ratio otherwise). */
static int menus_43(void)
{
    static int env = -1;
    if (env < 0)
        env = getenv("NGB_MENUS_43") != NULL;
    return env;
}

void ngb_aspect_tick(void);

/* A menu runs (src/menu_mouse.c, each frame it does): the widescreen mode from now. */
void ngb_aspect_menu_running(void)
{
    if (!s_menu_now) {
        s_menu_now = 1;
        ngb_aspect_tick();
    }
}

/* Each frame's end (src/cheats.c, at Swap): the next frame's mode -- a menu marks itself again; a
 * frame of next to no 3D (the renderer's count, src/d3d/vk/backend.cpp) is a 2D screen. */
extern int ngb_frame_draws, ngb_frame_3d_draws;
void ngb_aspect_frame_end(void)
{

    s_menu_last = s_menu_now;   /* a menu last frame: the mode kept, so it never flips within a frame */
    s_menu_now = 0;
    s_screen_2d = ngb_frame_draws > 0 && ngb_frame_3d_draws < 30;
}

/* For the renderer and the mouse: the game is in its widescreen mode on a wide screen, its 2D drawn
 * as it is, stretched. */
int ngb_2d_stretched(void)
{
    /* The renderer asks from its first present, which can come before the guest's memory is mapped:
     * the flag read at 0 + 0xD21748 crashed a slow first start (Windows, just after a reboot). */
    return s_screen_wide && g_xbox_mem_offset && (MEM32(0xD21748) & 1) != 0;
}

/* For the renderer: a 2D screen (a menu, the title, a chapter card) on a wide screen, in whichever mode. */
int ngb_2d_screen(void)
{
    return s_screen_wide && (s_menu_now || s_menu_last || s_screen_2d);
}

/* The same, knowing the 3D this frame has drawn so far (the renderer's count): a frame that has drawn
 * the game's 3D, with no menu running in it, is play -- the first frame after a menu closes too, which
 * the frame before (a 2D screen) would otherwise have drawn in the menus' layout, the HUD jumping for
 * that one frame. */
int ngb_2d_screen_frame(int threed)
{
    return s_screen_wide && (s_menu_now || ((s_menu_last || s_screen_2d) && threed < 30));
}

/* Its parts as of a call, for a renderer that draws later (gpu::Peek): 1 a menu now, 2 the last frame's menu or a
 * 2D screen -- ngb_2d_screen_frame is (1) or (2 and fewer than 30 3D draws so far). */
int ngb_2d_screen_flags(void)
{
    return s_screen_wide ? (s_menu_now ? 1 : 0) | (s_menu_last || s_screen_2d ? 2 : 0) : 0;
}
/* Before the game runs: the screen's shape (the flags are the game's to set up, then kept by the tick). */
void ngb_game_aspect_init(int widescreen)
{
    s_screen_wide = widescreen;
    s_flags_wide = widescreen && game_widescreen_mode();
}
static void set_flags(int wide)
{
    MEM32(0x9776A8) = wide ? MEM32(0x9776A8) | 0x10 : MEM32(0x9776A8) & ~0x10u;
    MEM32(0xD21748) = wide ? MEM32(0xD21748) | 1 : MEM32(0xD21748) & ~1u;
    if (wide)
        xbox_XcVideoFlags |= 1;
    else
        xbox_XcVideoFlags &= ~1u;
}

int ngb_menu_up(void);   /* src/menu_mouse.c */
static int page_now(void);
static int s_recomp;

void ngb_aspect_tick(void)
{
    static int aspect_log = -1;
    if (aspect_log < 0)
        aspect_log = getenv("NGB_ASPECT_LOG") != NULL;
    if (aspect_log) {
        static unsigned n;
        if (n++ % 20000 == 0)
            fprintf(stderr, "[ASPECT] tick: constants %.4f %.4f 977EF8 %.4f flags %X\n", MEMF(0x414AF4), MEMF(0x414AF8),
                    MEMF(0x977EF8), MEM32(0x9776A8));
    }
    if (s_aspect_a > 0 && patch_constant(s_aspect_a))
        s_aspect_a = 0;
    if (s_flags_wide >= 0) {
        /* a movie in the game's widescreen mode: it follows its own Movie Display Ratio there, 16:9
         * across the screen or 4:3 with bars at the sides (its 4:3 mode as well narrowed that to 9/16, #28) */
        int movie = s_movie_ms && now_ms() - s_movie_ms < 200;
        /* On a wide screen the game's widescreen mode throughout -- its own 16:9 look, every screen as it
         * draws it (src/d3d/vk/backend.cpp stretches its 2D, no wider than 16:9). NGB_HUD_FIXED=1: the earlier way, play in the 4:3 mode with the HUD
         * drawn in its own shape and the 16:9 mode for the menus it can tell. */
        static int hud_fixed = -1;
        if (hud_fixed < 0)
            hud_fixed = getenv("NGB_HUD_FIXED") != NULL;
        int two_d = hud_fixed ? !menus_43() && (s_menu_now || s_menu_last || s_screen_2d) : !menus_43();
        int want = s_flags_wide || (s_screen_wide && (s_boot || movie || two_d));
        if ((MEM32(0x9776A8) >> 4 & 1) != (uint32_t)want || (MEM32(0xD21748) & 1) != (uint32_t)want)
        {
            set_flags(want);   /* the game sets them again from its video settings */
            if (want && getenv("NGB_MENU_MODE_LOG"))
                fprintf(stderr, "[MENUMODE] wide: menu %d 2d %d movie %d boot %d\n", s_menu_now, s_screen_2d, movie, s_boot);
            static unsigned n;
            if (getenv("NGB_ASPECT_LOG") && n++ % 60 == 0)
                fprintf(stderr, "[ASPECT] flags set to %d (%u times)\n", want, n);
        }
    }
}

/* While it runs. */
void ngb_game_set_aspect(int widescreen, double a)
{
    if (!widescreen)
        a = 4.0 / 3;
    ngb_game_set_aspect_constant(a);
    s_flags_wide = widescreen && game_widescreen_mode();
    s_screen_wide = widescreen;
    set_flags(s_flags_wide || (widescreen && s_boot));
    MEMF(0x977EF8) = (float)a * MEMF(0x4148D0);
    if (getenv("NGB_ASPECT_LOG"))
        fprintf(stderr, "[ASPECT] widescreen %d a %.4f: constants %.4f %.4f (pending %.4f), 977EF8 %.4f\n", widescreen,
                a, MEMF(0x414AF4), MEMF(0x414AF8), s_aspect_a, MEMF(0x977EF8));
}

void ngb_pc_menu_init(void)
{
    pc_menu_init();
    ngb_pause_tab_init();
    ngb_general_page_init();
}

static int page_now(void)
{
    return MEM8(MENU_STATE);
}

/* ---------- RECOMP SETTINGS ---------- */

/* From the main menu's choice until the main menu is back: our page table
 * in the Options menu's place (both of the menus that use the game's). */
static int s_recomp_seen;
static uint32_t s_saved_tables[2];
void ngb_recomp_pictures_off(void);   /* src/pause_tab.c */

void ngb_recomp_menu_enter(void)
{
    pc_menu_init();
    s_saved_tables[0] = MEM32(0x42A28C);
    s_saved_tables[1] = MEM32(0x42A2AC);
    MEM32(0x42A28C) = s_pages_recomp;
    MEM32(0x42A2AC) = s_pages_recomp;
    s_recomp = 1;
    s_recomp_seen = 0;
}

int ngb_recomp_menu_active(void) { return s_recomp; }
void ngb_recomp_menu_seen(void) { s_recomp_seen = s_recomp; }

/* The main menu again (its drawer, each frame): the game's tables back. */
void ngb_recomp_menu_tick(void)
{
    if (!s_recomp || !s_recomp_seen)
        return;
    MEM32(0x42A28C) = s_saved_tables[0];
    MEM32(0x42A2AC) = s_saved_tables[1];
    s_recomp = s_recomp_seen = 0;
    if (s_unl)
        unl_show(0, -1);
    ngb_recomp_pictures_off();
}

/* The row kinds of the page on screen: page 4/6 by the widescreen flag,
 * or ours. */
static const uint8_t *page_rows(int *n)
{
    if (page_now() == PAGE_CHEAT && s_unl) {
        *n = N_UNL;
        return s_rows_unl;
    }
    if (is_ours(page_now())) {
        *n = s_ours[page_now() - PAGE_GFX].n;
        return s_ours[page_now() - PAGE_GFX].kinds;
    }
    if (MEM8(0xD21748) & 1)   { *n = N_WS; return s_rows_ws; }
    *n = N_STD;
    return s_rows_std;
}

static int kind_unlock(int kind) { return kind - ROW_MISSIONS; }   /* settings 26-28 */

static int row_kind(int row)
{
    int n;
    const uint8_t *k = page_rows(&n);
    return row >= 0 && row < n ? k[row] : -1;
}


/* The value text of a row, or 0: Movie Display Ratio's is the page's own
 * (movie_id). */
static uint32_t pc_value_id(int row, uint32_t movie_id)
{
    switch (row_kind(row)) {
    case ROW_QUICKFREEZE: return ngb_pc_setting(19) ? T_ON : T_OFF;
    case ROW_POSAUDIO: return ngb_pc_setting(23) == 2 ? T_POSPAN : ngb_pc_setting(23) ? T_POSCONSOLE : T_OFF;
    case ROW_HUD:     return ngb_pc_setting(24) ? T_ON : T_OFF;
    case ROW_AUDIOOUT: return ngb_pc_setting(25) == 2 ? T_SURROUND : ngb_pc_setting(25) ? T_STEREO : T_AUTO;
    case ROW_QUICKSCALE: {
        int v = ngb_pc_setting(20);
        return T_QUICKSCALE0 + (uint32_t)(v ? (150 - v) / 25 : 6);
    }
    case ROW_MOVIE:   return movie_id;
    case ROW_RENDER:  return T_SCALE0 + (uint32_t)(ngb_pc_setting(0) - 1);
    case ROW_TEXTURE: {
        int v = ngb_pc_setting(1);
        return T_ANISO0 + (v >= 16 ? 4 : v >= 8 ? 3 : v >= 4 ? 2 : v >= 2 ? 1 : 0);
    }
    case ROW_MSAA: {
        int v = ngb_pc_setting(10);
        return T_MSAA0 + (v >= 8 ? 3 : v >= 4 ? 2 : v >= 2 ? 1 : 0);
    }
    case ROW_SHADOW: {
        int v = ngb_pc_setting(2);
        return v >= 10 ? T_SHADOW10 : T_SHADOW0 + (v >= 8 ? 3 : v >= 4 ? 2 : v >= 2 ? 1 : 0);
    }
    case ROW_DISPLAY: return ngb_pc_setting(3) == 2 ? T_BORDERLESS : ngb_pc_setting(3) ? T_FULL : T_WINDOW;
    case ROW_VSYNC: {
        int v = ngb_pc_setting(4);
        return v == 1 ? T_MAILBOX : v == 2 ? T_ADAPTIVE : v == 3 ? T_FIFO : T_OFF;
    }
    case ROW_FPS:     return ngb_pc_setting(5) ? T_ON : T_OFF;
    case ROW_WINDOW:  return T_WSIZE0 + (uint32_t)ngb_pc_setting(6);
    case ROW_ASPECT:  return T_ASPECT0 + (uint32_t)ngb_pc_setting(7);
    case ROW_SKIP:    return ngb_pc_setting(8) ? T_YES : T_NO;
    case ROW_LAUNCHER: return ngb_pc_setting(9) ? T_YES : T_NO;
    case ROW_HEALTH:  return ngb_pc_setting(11) ? T_ON : T_OFF;
    case ROW_NINPO:   return ngb_pc_setting(12) ? T_ON : T_OFF;
    case ROW_ESSENCE: return ngb_pc_setting(13) ? T_ON : T_OFF;
    case ROW_ALLDIFF: return ngb_pc_setting(14) ? T_YES : T_NO;
    case ROW_SCARABS: return ngb_pc_setting(15) ? T_ON : T_OFF;
    case ROW_ALLCOST: return ngb_pc_setting(16) ? T_YES : T_NO;
    case ROW_GLOW:    return ngb_pc_setting(18) ? T_ON : T_OFF;
    case ROW_LOADDETAIL: return ngb_pc_setting(29) ? T_ON : T_OFF;
    case ROW_RTHREAD: return ngb_pc_setting(30) ? T_ON : T_OFF;
    case ROW_SPARKTHICKNESS: return T_SPARKTHICKNESS0 + (uint32_t)ngb_pc_setting(31);
    case ROW_INFITEMS: return ngb_pc_setting(21) ? T_ON : T_OFF;
    case ROW_INFAMMO: return ngb_pc_setting(22) ? T_ON : T_OFF;
    case ROW_MISSIONS:
    case ROW_ARCADE:
    case ROW_MOVIES:  return ngb_pc_setting(26 + kind_unlock(row_kind(row))) ? T_YES : T_NO;
    default:          return 0;
    }
}

/* Left/right on a PC row (delta from the menu engine); false for the
 * page's own rows. */
static int pc_value_change(int row, int delta)
{
    int kind = row_kind(row);
    int step = delta > 0 ? 1 : -1;
    if ((kind < ROW_RENDER || kind > ROW_ASPECT) && (kind < ROW_SKIP || kind > ROW_MOVIES) && kind != ROW_SPARKTHICKNESS)
        return 0;                                      /* (Unlocks opens its page: the engine's) */
    if (!delta)
        return 1;
    switch (kind) {
    case ROW_QUICKFREEZE: ngb_pc_set_setting(19, !ngb_pc_setting(19)); break;
    case ROW_POSAUDIO: ngb_pc_set_setting(23, (ngb_pc_setting(23) + (step > 0 ? 1 : 2)) % 3); break;
    case ROW_HUD: ngb_pc_set_setting(24, !ngb_pc_setting(24)); break;
    case ROW_AUDIOOUT: ngb_pc_set_setting(25, (ngb_pc_setting(25) + (step > 0 ? 1 : 2)) % 3); break;
    case ROW_QUICKSCALE: ngb_pc_set_setting(20, ngb_quick_scale_step(ngb_pc_setting(20), step)); break;
    case ROW_RENDER: {
        int v = ngb_pc_setting(0) + step;
        ngb_pc_set_setting(0, v < 1 ? 10 : v > 10 ? 1 : v);
        break;
    }
    case ROW_SHADOW: {
        static const int steps[] = { 1, 2, 4, 8, 10 };
        int v = ngb_pc_setting(2), i = 0;
        while (i < 4 && steps[i] != v) i++;
        ngb_pc_set_setting(2, steps[(i + step + 5) % 5]);
        break;
    }
    case ROW_TEXTURE: {
        static const int steps[] = { 0, 2, 4, 8, 16 };
        int v = ngb_pc_setting(1), i = 0;
        while (i < 4 && steps[i] != v) i++;
        ngb_pc_set_setting(1, steps[(i + step + 5) % 5]);
        break;
    }
    case ROW_MSAA: {
        static const int steps[] = { 1, 2, 4, 8 };
        int v = ngb_pc_setting(10), i = 0;
        while (i < 3 && steps[i] != v) i++;
        ngb_pc_set_setting(10, steps[(i + step + 4) % 4]);
        break;
    }
    case ROW_DISPLAY: ngb_pc_set_setting(3, (ngb_pc_setting(3) + (step > 0 ? 1 : 2)) % 3); break;
    case ROW_VSYNC:   ngb_pc_set_setting(4, ngb_vsync_step(ngb_pc_setting(4), step)); break;
    case ROW_FPS:     ngb_pc_set_setting(5, !ngb_pc_setting(5)); break;
    case ROW_SKIP:    ngb_pc_set_setting(8, !ngb_pc_setting(8)); break;
    case ROW_LAUNCHER: ngb_pc_set_setting(9, !ngb_pc_setting(9)); break;
    case ROW_ALLDIFF: ngb_pc_set_setting(14, !ngb_pc_setting(14)); break;
    case ROW_SCARABS: ngb_pc_set_setting(15, !ngb_pc_setting(15)); break;
    case ROW_ALLCOST: ngb_pc_set_setting(16, !ngb_pc_setting(16)); break;
    case ROW_GLOW:    ngb_pc_set_setting(18, !ngb_pc_setting(18)); break;
    case ROW_LOADDETAIL: ngb_pc_set_setting(29, !ngb_pc_setting(29)); break;
    case ROW_RTHREAD: ngb_pc_set_setting(30, !ngb_pc_setting(30)); break;
    case ROW_SPARKTHICKNESS: ngb_pc_set_setting(31, (ngb_pc_setting(31) + (step > 0 ? 1 : 3)) % 4); break;
    case ROW_INFITEMS: ngb_pc_set_setting(21, !ngb_pc_setting(21)); break;
    case ROW_INFAMMO: ngb_pc_set_setting(22, !ngb_pc_setting(22)); break;
    case ROW_MISSIONS:
    case ROW_ARCADE:
    case ROW_MOVIES: {
        int which = 26 + kind_unlock(kind);
        ngb_pc_set_setting(which, !ngb_pc_setting(which));
        break;
    }
    case ROW_HEALTH:
    case ROW_NINPO:
    case ROW_ESSENCE: {
        int which = 11 + kind - ROW_HEALTH;
        ngb_pc_set_setting(which, !ngb_pc_setting(which));
        break;
    }
    case ROW_ASPECT:  ngb_pc_set_setting(7, (ngb_pc_setting(7) + (step > 0 ? 1 : 3)) % 4); break;
    case ROW_WINDOW: {
        int v = ngb_pc_setting(6), a = v / 8, n = ngb_settings_size_count(a);
        ngb_pc_set_setting(6, a * 8 + (v % 8 + step + n) % n);
        break;
    }
    }
    return 1;
}

/* Cheats' page with Unlocks' rows (on) or its own, the cursor on row: the page's row table and last row
 * swapped in place. Not a page of its own: the engine slides the whole screen out and in between two
 * pages, and a page with fewer rows sat lower. */
static void unl_show(int on, int row)
{
    uint32_t pg = s_pages_recomp + 0x18 * PAGE_CHEAT;
    s_unl = on;
    MEM32(pg + 0xC) = on ? s_rowtab_unl : s_rowtab_cheat;
    MEM8(pg + 2) = (uint8_t)((on ? N_UNL : (int)sizeof s_rows_cheat) - 1);
    if (row >= 0) {                                       /* (not as the menu closes) */
        MEM8(MENU_STATE + 1) = MEM8(MENU_STATE + 2) = (uint8_t)row;
        MEM16(0x8DE468) = (uint16_t)row;                  /* the drawer's copy: else a frame on the old row */
    }
}

static void menu_sound(uint32_t which)
{
    PUSH32(esp, 0x40);
    PUSH32(esp, which);
    PUSH32(esp, 0x0017F54Eu); RECOMP_ABI_CALL(0x001E3400u, sub_001E3400);
    esp = esp + 8;
}

/**
 * sub_0017F4E0
 * Original: 0x0017F4E0 - 0x0017F6F0 (528 bytes, 161 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_0017F4E0(void)
{
    RECOMP_ENTRY(0x0017F4E0u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_0017F4E0: ;
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    PUSH32(esp, edi);
    edi = MEM32(esp + 0x1C);
    eax = MEM32(edi + 0x28);
    SET_LO8(edx, MEM8(edi));
    ebp = MEM32(edi + 0x2C);
    MEM32(esp + 0x10) = eax;
    SET_LO8(eax, MEM8(edi + 1));
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(LO8(edx)) & 0xFFu; _fb = (uint32_t)(6) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(edx), 6 (8-bit) */
    SET_LO8(ecx, 1);
    MEM8(esp + 0x14) = LO8(eax);
    MEM8(esp + 0x1C) = LO8(ebx);
    MEM8(esp + 0xC) = LO8(ecx);
    if (CMP_NE(_fa, _fb)) goto loc_0017F514; /* jne: not equal / not zero */

loc_0017F50E: ;
    SET_LO8(ecx, 0); /* xor self */
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* xor result */
    MEM8(esp + 0xC) = LO8(ecx);

loc_0017F514: ;
    PUSH32(esp, esi);
    esi = MEM32(edi + 0x18);
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0x7FFFFFFF) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test esi, 0x7FFFFFFF (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_0017F540; /* je: equal / zero */

loc_0017F520: ;
    SET_LO8(eax, MEM8(edi + 0x11));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), LO8(ebx) (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0017F52C; /* je: equal / zero */

loc_0017F527: ;
    SET_LO8(eax, LO8(eax) - 1);
    _fa = (uint32_t)(LO8(eax)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fb = (_fa == 0x7Fu); /* dec result/SF/OF; CF unchanged */
    MEM8(edi + 0x11) = LO8(eax);

loc_0017F52C: ;
    _fa = (uint32_t)(MEM8(edi + 0x11)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(edi + 0x11), LO8(ebx) (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0017F681; /* jne: not equal / not zero */

loc_0017F535: ;
    esi = esi & 0xFFFFFFFCu;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(edi + 0x18) = esi;
    goto loc_0017F681;

loc_0017F540: ;
    _fa = (uint32_t)(MEM8(edi + 0xD)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(edi + 0xD), LO8(ebx) (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0017F567; /* je: equal / zero */

loc_0017F545: ;
    if (s_recomp && s_unl && MEM8(edi) == PAGE_CHEAT) {   /* PC: B on Unlocks' rows: Cheats' again, on Unlocks */
        menu_sound(3);
        unl_show(0, (int)sizeof s_rows_cheat - 2);
        goto loc_0017F681;
    }
    PUSH32(esp, 0x40);
    PUSH32(esp, 3);
    PUSH32(esp, 0x0017F54Eu); RECOMP_ABI_CALL(0x001E3400u, sub_001E3400); /* call 0x001E3400 */

loc_0017F54E: ;
    ecx = ZX8(MEM8(edi + 0xC));
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(edi + 0x1C) = ecx;
    MEM32(0xDA4808) = 0x81;
    goto loc_0017F681;

loc_0017F567: ;
    SET_LO8(edx, MEM8(edi + 0xC));
    _fa = (uint32_t)(LO8(edx)) & 0xFFu; _fb = (uint32_t)(0xFF) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(edx), 0xFF (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0017F5F9; /* jne: not equal / not zero */

loc_0017F573: ;
    /* PC: the PC rows' values (the menu engine's step at +0x10). */
    pc_menu_init();
    if ((MEM8(edi) == 4 || MEM8(edi) == 6 || is_ours(MEM8(edi))) &&
        pc_value_change(LO8(eax), SMEM8(edi + 0x10))) {
        if (SMEM8(edi + 0x10)) {
            PUSH32(esp, 0x40);
            PUSH32(esp, 1);
            PUSH32(esp, 0x0017F5C1u); RECOMP_ABI_CALL(0x001E3400u, sub_001E3400); /* the change sound */
            esp = esp + 8;
        }
        ebx = 0;
        goto loc_0017F681;
    }
    if (MEM8(edi) != 4 || row_kind(LO8(eax)) != ROW_MOVIE)
        goto loc_0017F681;         /* Movie Display Ratio's is page 4's row 1 */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 1 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0017F681; /* jne: not equal / not zero */

loc_0017F57B: ;
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), LO8(ebx) (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0017F681; /* je: equal / zero */

loc_0017F583: ;
    ecx = ZX8(MEM8(0xDA42EF));
    esi = ZX8(MEM8(0x42D3DF));
    eax = ZX8(MEM8(0x42D3E0));
    edx = (uint32_t)(int32_t)SMEM8(edi + 0x10);
    ebx = MEM32(ebp + 0x10);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x0017F5A5u); RECOMP_ABI_CALL(0x0017D910u, sub_0017D910); /* call 0x0017D910 */

loc_0017F5A5: ;
    MEM8(0xDA42EF) = LO8(eax);
    SET_LO8(eax, MEM8(edi + 0x10));
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0017F67F; /* je: equal / zero */

loc_0017F5B8: ;
    PUSH32(esp, 0x40);
    PUSH32(esp, 1);
    PUSH32(esp, 0x0017F5C1u); RECOMP_ABI_CALL(0x001E3400u, sub_001E3400); /* call 0x001E3400 */

loc_0017F5C1: ;
    SET_LO8(eax, MEM8(0xDA42EF));
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0017F5E4; /* jne: not equal / not zero */

loc_0017F5CF: ;
    xmm0 = XMM_SCALAR(MEMF(0x414BD0)); /* movss */
    MEMF(0x44D444) = xmm0.f[0]; /* movss */
    goto loc_0017F681;

loc_0017F5E4: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    MEMF(0x44D444) = xmm0.f[0]; /* movss */
    goto loc_0017F681;

loc_0017F5F9: ;
    if (s_recomp && MEM8(edi) == PAGE_CHEAT &&
        (LO8(edx) == UNL_ACTION || (LO8(edx) == 0xFD && s_unl))) {   /* PC: Unlocks' rows, or Exit on them */
        int open = LO8(edx) == UNL_ACTION;
        menu_sound(open ? 5 : 3);
        unl_show(open, open ? 0 : (int)sizeof s_rows_cheat - 2);
        goto loc_0017F681;
    }
    eax = ZX8(LO8(edx));
    _fb = (uint32_t)(0xFC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - 0xFC;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    if ((_fa == 0)) goto loc_0017F661; /* je: equal / zero */

loc_0017F603: ;
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa == 0)) goto loc_0017F63E; /* je: equal / zero */

loc_0017F606: ;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, ebx (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0017F681; /* jne: not equal / not zero */

loc_0017F60A: ;
    PUSH32(esp, 0x40);
    PUSH32(esp, 5);
    PUSH32(esp, 0x0017F613u); RECOMP_ABI_CALL(0x001E3400u, sub_001E3400); /* call 0x001E3400 */

loc_0017F613: ;
    ecx = MEM32(edi + 0x18);
    SET_LO8(edx, MEM8(edi + 0xC));
    eax = 2;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx = ecx | eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM8(edi) = LO8(edx);
    MEM8(edi + 1) = LO8(ebx);
    MEM8(edi + 2) = LO8(ebx);
    MEM8(edi + 3) = LO8(ebx);
    MEM8(edi + 4) = LO8(ebx);
    MEM32(edi + 0x18) = ecx;
    MEM8(edi + 0x11) = 1;
    MEM8(esp + 0x20) = LO8(eax);
    goto loc_0017F681;

loc_0017F63E: ;
    PUSH32(esp, 0x40);
    PUSH32(esp, 3);
    PUSH32(esp, 0x0017F647u); RECOMP_ABI_CALL(0x001E3400u, sub_001E3400); /* call 0x001E3400 */

loc_0017F647: ;
    eax = MEM32(esp + 0x1C);
    ecx = ZX8(MEM8(eax + 6));
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(edi + 0x1C) = ecx;
    MEM32(0xDA4808) = 0x81;
    goto loc_0017F681;

loc_0017F661: ;
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), LO8(ebx) (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0017F681; /* je: equal / zero */

loc_0017F665: ;
    PUSH32(esp, 0x40);
    PUSH32(esp, 5);
    PUSH32(esp, 0x0017F66Eu); RECOMP_ABI_CALL(0x001E3400u, sub_001E3400); /* call 0x001E3400 */

loc_0017F66E: ;
    SET_LO8(edx, MEM8(0x42D3E1));
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM8(0xDA42EF) = LO8(edx);
    goto loc_0017F681;

loc_0017F67F: ;
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */

loc_0017F681: ;
    PUSH32(esp, ebx);
    PUSH32(esp, 0x0017F687u); RECOMP_ABI_CALL(0x0017FC00u, sub_0017FC00); /* call 0x0017FC00 */

loc_0017F687: ;
    /* PC: the category lit in the column: Video's (3), or ours. */
    PUSH32(esp, is_ours(page_now()) ? (uint32_t)(page_now() - PAGE_GFX) : 3u);
    SET_LO8(edx, 1);
    PUSH32(esp, 0x0017F690u); RECOMP_ABI_CALL(0x0017FD30u, sub_0017FD30); /* call 0x0017FD30 */

loc_0017F690: ;
    eax = MEM32(esp + 0x18);
    ecx = MEM32(esp + 0x20);
    PUSH32(esp, eax);
    PUSH32(esp, ecx);
    esi = edi;
    PUSH32(esp, 0x0017F6A1u); RECOMP_ABI_CALL(0x00180140u, sub_00180140); /* call 0x00180140 */

loc_0017F6A1: ;
    ecx = MEM32(edi + 0x18);
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x10;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = eax | 0xFFFFFFFFu;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, ebx (32-bit) */
    POP32(esp, esi);
    if (CMP_NE(_fa, _fb)) goto loc_0017F6D0; /* jne: not equal / not zero */

loc_0017F6AF: ;
    ecx = MEM32(edi + 0x1C);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, eax (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0017F6D0; /* je: equal / zero */

loc_0017F6B6: ;
    SET_LO8(edx, LO8(ecx));
    ecx = MEM32(esp + 0x10);
    MEM8(edi) = LO8(edx);
    MEM32(edi + 0x1C) = eax;
    SET_LO8(edx, MEM8(ecx + 7));
    MEM8(edi + 1) = LO8(edx);
    MEM8(edi + 2) = LO8(ebx);
    MEM8(edi + 3) = LO8(ebx);
    MEM8(edi + 4) = LO8(ebx);

loc_0017F6D0: ;
    SET_LO8(ecx, MEM8(esp + 0x1C));
    POP32(esp, edi);
    POP32(esp, ebp);
    MEM32(0xDA4808) = ebx;
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), LO8(ebx) (8-bit) */
    POP32(esp, ebx);
    if (CMP_EQ(_fa, _fb)) goto loc_0017F6EA; /* je: equal / zero */

loc_0017F6E1: ;
    ecx = ZX8(LO8(ecx));
    MEM32(0xDA4808) = ecx;

loc_0017F6EA: ;
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 4; return; /* ret */

}

/**
 * sub_00180140
 * Original: 0x00180140 - 0x00180245 (261 bytes, 69 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_00180140(void)
{
    RECOMP_ENTRY(0x00180140u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_00180140: ;
    _fb = (uint32_t)(0x40) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x40;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    eax = MEM32(0x41CCD8);
    ecx = 2;
    MEM32(esp + 0x3C) = eax;
    eax = ZX8(MEM8(esp + 0x44));
    /* PC: a byte per row for up to 16 rows, not five on the stack. */
    pc_menu_init();
    for (int _i = 0; _i < 16; _i++)
        MEM8(s_highlight + _i) = LO8(ecx);
    MEM8(s_highlight + (eax & 15)) = 3;
    MEM32(esp + 0x10) = eax;
    SET_LO8(eax, MEM8(esp + 0x48));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    edx = s_highlight;
    MEM16(esp + 0x26) = LO16(ecx);
    MEM32(esp) = edx;
    MEM16(esp + 0x24) = 3;
    if (CMP_EQ(_fa, _fb)) goto loc_00180195; /* je: equal / zero */

loc_0018018E: ;
    MEM16(esp + 0x24) = 4;

loc_00180195: ;
    eax = MEM32(0xDA41C8);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 1 (32-bit) */
    MEM32(esp + 0x1C) = 0;
    MEM16(esp + 0x2E) = 0;
    if (CMP_NE(_fa, _fb)) goto loc_001801B5; /* jne: not equal / not zero */

loc_001801AE: ;
    MEM16(esp + 0x2E) = LO16(ecx);
    goto loc_001801FF;

loc_001801B5: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(3) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 3 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001801C1; /* jne: not equal / not zero */

loc_001801BA: ;
    MEM16(esp + 0x2E) = LO16(eax);
    goto loc_001801FF;

loc_001801C1: ;
    eax = MEM32(0xDA4808);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 1 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001801D4; /* jne: not equal / not zero */

loc_001801CB: ;
    MEM16(esp + 0x2E) = 0x16;
    goto loc_001801FF;

loc_001801D4: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x81) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x81 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001801E4; /* jne: not equal / not zero */

loc_001801DB: ;
    MEM16(esp + 0x2E) = 0x17;
    goto loc_001801FF;

loc_001801E4: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ecx (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001801F1; /* jne: not equal / not zero */

loc_001801E8: ;
    MEM16(esp + 0x2E) = 0x14;
    goto loc_001801FF;

loc_001801F1: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x82) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x82 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001801FF; /* jne: not equal / not zero */

loc_001801F8: ;
    MEM16(esp + 0x2E) = 0x15;

loc_001801FF: ;
    eax = esp;
    PUSH32(esp, eax);
    PUSH32(esp, 0x00180208u); RECOMP_ABI_CALL(0x0021F080u, sub_0021F080); /* call 0x0021F080 */

loc_00180208: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 1 (32-bit) */
    ecx = esi;
    eax = 3;
    if (CMP_NE(_fa, _fb)) goto loc_0018022E; /* jne: not equal / not zero */

loc_00180217: ;
    PUSH32(esp, 1);
    PUSH32(esp, 0x0018021Eu); RECOMP_ABI_CALL(0x00180510u, sub_00180510); /* call 0x00180510 */

loc_0018021E: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx = MEM32(esp + 0x3C);
    PUSH32(esp, 0x0018022Au); RECOMP_ABI_CALL(0x00379C29u, sub_00379C29); /* call 0x00379C29 */

loc_0018022A: ;
    _fb = (uint32_t)(0x40) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x40;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 4; return; /* ret */

loc_0018022E: ;
    PUSH32(esp, 0);
    PUSH32(esp, 0x00180235u); RECOMP_ABI_CALL(0x00180510u, sub_00180510); /* call 0x00180510 */

loc_00180235: ;
    ecx = MEM32(esp + 0x40);
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, 0x00180241u); RECOMP_ABI_CALL(0x00379C29u, sub_00379C29); /* call 0x00379C29 */

loc_00180241: ;
    _fb = (uint32_t)(0x40) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x40;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 4; return; /* ret */

}

/**
 * sub_0021F080
 * Original: 0x0021F080 - 0x0021F5F0 (1392 bytes, 319 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: standard_frame
 */
void sub_0021F080(void)
{
    RECOMP_ENTRY(0x0021F080u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* prologue saves caller's frame */
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

loc_0021F080: ;
    PUSH32(esp, ebp);
    ebp = esp;
    g_ebp = ebp; /* publish frame for frameless callees */
    g_seh_ebp = ebp;
    esp = esp & 0xFFFFFFF0u;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fb = (uint32_t)(0xF4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0xF4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, ebx);
    eax = 0x20005;
    PUSH32(esp, esi);
    MEM32(esp + 0x70) = eax;
    MEM32(esp + 0x88) = eax;
    eax = MEM32(ebp + 8);
    ecx = 0x20011;
    PUSH32(esp, edi);
    PUSH32(esp, eax);
    edx = 0x8DE4FC;
    MEM32(esp + 0x74) = ecx;
    MEM32(esp + 0x84) = ecx;
    MEM32(esp + 0x88) = 0x20015;
    MEM32(esp + 0x8C) = 0x20004;
    MEM32(esp + 0x7C) = 0x20023;
    MEM32(esp + 0x80) = 0x20024;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021F0E6u); RECOMP_ABI_CALL(0x0021C0F0u, sub_0021C0F0); /* call 0x0021C0F0 */

loc_0021F0E6: ;
    esi = eax;
    SET_LO8(eax, MEM8(0xD21748));
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(eax), 1 (8-bit) */
    MEM32(esp + 0x68) = esi;
    if (TEST_Z(_fa, _fb)) goto loc_0021F10D; /* je: equal / zero */

loc_0021F0F8: ;
    /* PC: the page's rows and their text. */
    pc_menu_init();
    MEM32(esp + 0x1C) = N_WS;
    MEM32(esp + 0x4C) = s_labels_ws;
    goto loc_0021F11C;

loc_0021F10D: ;
    pc_menu_init();
    MEM32(esp + 0x1C) = N_STD;
    MEM32(esp + 0x4C) = s_labels_std;

loc_0021F11C: ;
    if (is_ours(page_now())) {
        MEM32(esp + 0x1C) = (uint32_t)s_ours[page_now() - PAGE_GFX].n;
        MEM32(esp + 0x4C) = s_labels_ours[page_now() - PAGE_GFX];
    }
    if (page_now() == PAGE_CHEAT && s_unl) {
        MEM32(esp + 0x1C) = (uint32_t)N_UNL;
        MEM32(esp + 0x4C) = s_labels_unl;
    }

loc_0021F11D: ;
    ecx = ZX8(MEM8(0xDB9ADA));
    eax = MEM32(0xBCE018);
    edx = ((int32_t)eax < 0) ? 0xFFFFFFFF : 0; /* cdq */
    edi = ecx + ecx;
    { int64_t _dividend = ((int64_t)(int32_t)edx << 32) | eax;
      eax = (uint32_t)((int32_t)(_dividend / (int32_t)edi));
      edx = (uint32_t)((int32_t)(_dividend % (int32_t)edi)); }
    xmm1 = XMM_SCALAR(MEMF(0x415420)); /* movss */
    xmm0.f[0] = (float)(int32_t)ecx; /* cvtsi2ss */
    xmm1.f[0] = xmm1.f[0] / xmm0.f[0]; /* divss */
    eax = edx;
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_LE(_fas, _fbs)) goto loc_0021F14B; /* jle: less or equal (signed <=) */

loc_0021F147: ;
    edx = eax;
    goto loc_0021F14F;

loc_0021F14B: ;
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    ecx = ecx - edx;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    edx = ecx;

loc_0021F14F: ;
    eax = esp + 0x50;
    MEM32(esp + 0x18) = eax;
    eax = 0x43A0B0;
    ecx = MEM32(esp + 0x18);
    xmm0 = XMM_MEM(eax); /* movaps */
    XMM_STORE(ecx, xmm0); /* movaps */
    xmm0.f[0] = (float)(int32_t)edx; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * xmm1.f[0]; /* mulss */
    xmm0.f[0] = xmm0.f[0] + MEMF(0x414B74); /* addss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x3EC1C8); /* mulss */
    xmm0.f[0] = xmm0.f[0] + MEMF(0x4148D0); /* addss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x41492C); /* mulss */
    ecx = (int32_t)recomp_fist((double)xmm0.f[0], 0x0E7Fu, 32); /* cvttss2si */
    xmm0.f[0] = (float)(int32_t)ecx; /* cvtsi2ss */
    ecx = MEM32(esp + 0x1C);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x4148E0); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x414920); /* mulss */
    eax = 1;
    MEMF(esp + 0x5C) = xmm0.f[0]; /* movss */
    MEM32(0x43B47C) = eax;
    MEM32(0x43B480) = eax;
    ebx = 0x70;                                  /* PC: every page level with the categories' top */
    MEM32(esp + 0x10) = 0;
    if (CMP_LE(_fas, _fbs)) goto loc_0021F5E7; /* jle: less or equal (signed <=) */

loc_0021F1D4: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    MEMF(esp + 0x3C) = xmm0.f[0]; /* movss */
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    MEM8(esp + 0x2B) = LO8(eax);
    MEM8(esp + 0x2C) = LO8(eax);
    MEMF(esp + 0x44) = xmm0.f[0]; /* movss */
    goto loc_0021F200;

loc_0021F1F5: ;
    eax = 1;
    /* nop */

loc_0021F200: ;
    _fa = (uint32_t)(MEM8(0xD21748)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test MEM8(0xD21748), 1 (8-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_0021F21E; /* je: equal / zero */

loc_0021F209: ;
    ecx = MEM32(ebp + 8);
    esi = MEM32(ecx + 0x10);
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, eax (32-bit) */
    SET_LO8(edx, (CMP_NE(_fa, _fb)) ? 1 : 0); /* setne */
    MEM32(0x43B47C) = edx;
    goto loc_0021F223;

loc_0021F21E: ;
    MEM32(0x43B47C) = eax;

loc_0021F223: ;
    edx = MEM32(esp + 0x10);
    edi = esp + 0x14;
    esi = esp + 0x18;
    eax = 0x8DE4FC;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021F239u); RECOMP_ABI_CALL(0x0021C380u, sub_0021C380); /* call 0x0021C380 */

loc_0021F239: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    edi = MEM32(esp + 0x18);
    eax = MEM32(0x43B490);
    MEMF(esp + 0x38) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x415424)); /* movss */
    _fb = (uint32_t)(0x116) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edi = edi + 0x116;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 1 (32-bit) */
    MEMF(esp + 0x34) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x14)); /* movss */
    MEM32(esp + 0x20) = 0x246;
    MEM16(esp + 0x24) = LO16(edi);
    MEM16(esp + 0x26) = LO16(ebx);
    MEM16(esp + 0x28) = 0x1C2;
    MEM8(esp + 0x2A) = 0;
    MEM32(esp + 0x30) = 0;
    MEM32(esp + 0x48) = 0xFFFFFFFFu;
    MEMF(esp + 0x40) = xmm0.f[0]; /* movss */
    if (CMP_NE(_fa, _fb)) goto loc_0021F2FC; /* jne: not equal / not zero */

loc_0021F2A3: ;
    ecx = MEM32(0x8F7DFC);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(0x8F7DF8)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, MEM32(0x8F7DF8) (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_0021F302; /* jae: above or equal (unsigned >=) */

loc_0021F2B1: ;
    eax = MEM32(0x8F7DF4);
    esi = ecx;
    esi = (uint32_t)((int32_t)esi * (int32_t)0x70);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + eax;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = MEM32(0x8F7DF0);
    ecx = ecx << 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, 0);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, 0);
    eax = esp + 0x28;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021F2D4u); RECOMP_ABI_CALL(0x002216B0u, sub_002216B0); /* call 0x002216B0 */

loc_0021F2D4: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0021F2FC; /* je: equal / zero */

loc_0021F2DB: ;
    ecx = MEM32(0x8F7DFC);
    eax = 0x1C2;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021F2EDu); RECOMP_ABI_CALL(0x00221170u, sub_00221170); /* call 0x00221170 */

loc_0021F2ED: ;
    ecx = MEM32(0x8F7DFC);
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x8F7DFC) = ecx;
    goto loc_0021F302;

loc_0021F2FC: ;
    ecx = MEM32(0x8F7DFC);

loc_0021F302: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    _fa = (uint32_t)(MEM32(0x43B490)) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0x43B490), 1 (32-bit) */
    edx = edi + 0x185;
    MEMF(esp + 0x34) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x38) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x14)); /* movss */
    MEM32(esp + 0x20) = 0x247;
    MEM16(esp + 0x24) = LO16(edx);
    MEM16(esp + 0x26) = LO16(ebx);
    MEM16(esp + 0x28) = 0x1C2;
    MEM8(esp + 0x2A) = 0;
    MEM32(esp + 0x30) = 0;
    MEM32(esp + 0x48) = 0xFFFFFFFFu;
    MEMF(esp + 0x40) = xmm0.f[0]; /* movss */
    if (CMP_NE(_fa, _fb)) goto loc_0021F3A9; /* jne: not equal / not zero */

loc_0021F35F: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(0x8F7DF8)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, MEM32(0x8F7DF8) (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_0021F3A9; /* jae: above or equal (unsigned >=) */

loc_0021F367: ;
    eax = MEM32(0x8F7DF4);
    esi = ecx;
    esi = (uint32_t)((int32_t)esi * (int32_t)0x70);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + eax;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = MEM32(0x8F7DF0);
    ecx = ecx << 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, 0);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, 0);
    eax = esp + 0x28;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021F38Au); RECOMP_ABI_CALL(0x002216B0u, sub_002216B0); /* call 0x002216B0 */

loc_0021F38A: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0021F3A9; /* je: equal / zero */

loc_0021F391: ;
    ecx = MEM32(0x8F7DFC);
    eax = 0x1C2;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021F3A3u); RECOMP_ABI_CALL(0x00221170u, sub_00221170); /* call 0x00221170 */

loc_0021F3A3: ;
    MEM32(0x8F7DFC) = MEM32(0x8F7DFC) + 1;
    _fa = (uint32_t)(MEM32(0x8F7DFC)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_0021F3A9: ;
    edx = MEM32(esp + 0x4C);
    eax = SX16(LO16(ebx));
    esi = eax + 0x13;
    PUSH32(esp, 2);
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(ecx, MEM8(0x8DE524));
    eax = SX16(LO16(edi));
    edi = MEM32(esp + 0x14);
    PUSH32(esp, 0x1C3);
    _fb = (uint32_t)(0x23) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x23;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, esi);
    PUSH32(esp, eax);
    eax = MEM32(edx + edi * 4);
    PUSH32(esp, ecx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021F3D7u); RECOMP_ABI_CALL(0x001E6AE0u, sub_001E6AE0); /* call 0x001E6AE0 */

loc_0021F3D7: ;
    ecx = eax;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = esp + 0xB8;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021F3E8u); RECOMP_ABI_CALL(0x00168730u, sub_00168730); /* call 0x00168730 */

loc_0021F3E8: ;
    eax = esp + 0xB8;
    PUSH32(esp, eax);
    ecx = esp + 0xA4;
    PUSH32(esp, ecx);
    MEM8(esp + 0x10A) = 0x10;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021F405u); RECOMP_ABI_CALL(0x001E86A0u, sub_001E86A0); /* call 0x001E86A0 */

loc_0021F405: ;
    edx = ZX16(MEM16(0x43B488));
    xmm1 = XMM_SCALAR(MEMF(esp + 0xB0)); /* movss */
    _fb = (uint32_t)(0x18) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x18;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    xmm0.f[0] = (float)(int32_t)edx; /* cvtsi2ss */
    _fca = xmm1.f[0]; _fcb = xmm0.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_0021F437; /* jbe: below or equal (unsigned <=) (xmm1.f[0] vs xmm0.f[0]) */

loc_0021F421: ;
    xmm0.f[0] = xmm0.f[0] / xmm1.f[0]; /* divss */
    xmm0.f[0] = xmm0.f[0] * MEMF(esp + 0xC4); /* mulss */
    MEMF(esp + 0xC4) = xmm0.f[0]; /* movss */

loc_0021F437: ;
    xmm0 = XMM_SCALAR(MEMF(esp + 0x14)); /* movss */
    eax = esp + 0xA8;
    PUSH32(esp, eax);
    MEMF(esp + 0xC0) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021F453u); RECOMP_ABI_CALL(0x001E6D30u, sub_001E6D30); /* call 0x001E6D30 */

loc_0021F453: ;
    ecx = MEM32(ebp + 8);
    eax = MEM32(ecx + 0x10);
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edi (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0021F542; /* jne: not equal / not zero */

loc_0021F464: ;
    xmm0 = XMM_SCALAR(MEMF(esp + 0xC4)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(esp + 0x98); /* mulss */
    xmm1 = xmm0; /* movaps */
    xmm1.f[0] = xmm1.f[0] * MEMF(0x4148C0); /* mulss */
    xmm1.f[0] = xmm1.f[0] + MEMF(esp + 0x90); /* addss */
    edx = ebx + 0x11;
    MEM32(esp + 0x20) = 0xBA;
    MEM16(esp + 0x26) = LO16(edx);
    MEM16(esp + 0x28) = 0x1C4;
    MEMF(esp + 0x6C) = xmm1.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x6C), 0x0E7Fu, 32); /* cvttss2si */
    fp_push(MEMF(esp + 0x50)); /* fld float */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x414974); /* mulss */
    MEMF(esp + 0x34) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x9C)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x415240); /* mulss */
    MEM16(esp + 0x24) = LO16(eax);
    MEM8(esp + 0x2A) = 0x11;
    MEMF(esp + 0x38) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021F4E5u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_0021F4E5: ;
    fp_push(MEMF(esp + 0x5C)); /* fld float */
    edi = eax;
    edi = edi & 0xFF;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021F4F6u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_0021F4F6: ;
    fp_push(MEMF(esp + 0x54)); /* fld float */
    eax = eax << 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edi = edi | eax;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    edi = edi << 8;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021F507u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_0021F507: ;
    fp_push(MEMF(esp + 0x58)); /* fld float */
    eax = eax & 0xFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    edi = edi | eax;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    edi = edi << 8;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021F51Au); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_0021F51A: ;
    xmm0 = XMM_SCALAR(MEMF(esp + 0x14)); /* movss */
    eax = eax & 0xFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    edi = edi | eax;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(esp + 0x48) = edi;
    edi = esp + 0x20;
    MEMF(esp + 0x40) = xmm0.f[0]; /* movss */
    MEM32(esp + 0x30) = 5;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021F542u); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_0021F542: ;
    _fa = (uint32_t)(MEM8(0xD21748)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test MEM8(0xD21748), 1 (8-bit) */
    /* PC: values on both pages, not only Movie Display Ratio's. */

loc_0021F54B: ;
    eax = MEM32(0x43B474);
    ecx = MEM32(esp + 0x18);
    edi = eax + ecx;
    eax = pc_value_id((int)MEM32(esp + 0x10),
                      MEM32(esp + ZX8(MEM8(0xDA42EF)) * 4 + 0x78));
    if (eax == 0) goto loc_0021F5BF;

loc_0021F55E: ;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(edx, MEM8(0x8DE524));
    PUSH32(esp, edx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021F577u); RECOMP_ABI_CALL(0x001E6AE0u, sub_001E6AE0); /* call 0x001E6AE0 */

loc_0021F577: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0021F5BF; /* je: equal / zero */

loc_0021F57E: ;
    PUSH32(esp, 2);
    PUSH32(esp, 0x1C2);
    ecx = SX16(LO16(edi));
    PUSH32(esp, esi);
    PUSH32(esp, ecx);
    ecx = eax;
    eax = esp + 0xB8;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021F598u); RECOMP_ABI_CALL(0x00168730u, sub_00168730); /* call 0x00168730 */

loc_0021F598: ;
    xmm0 = XMM_SCALAR(MEMF(esp + 0x24)); /* movss */
    edx = esp + 0xB8;
    PUSH32(esp, edx);
    MEM8(esp + 0x106) = 0x10;
    MEMF(esp + 0xD0) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021F5BCu); RECOMP_ABI_CALL(0x001E6D30u, sub_001E6D30); /* call 0x001E6D30 */

loc_0021F5BC: ;
    _fb = (uint32_t)(0x14) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x14;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_0021F5BF: ;
    esi = MEM32(0x43B478);
    if ((int)MEM32(esp + 0x1C) > 7 || (s_recomp && page_now() == PAGE_CHEAT))
        esi -= PC_CLOSER;                        /* PC: more than seven rows closer (Cheats' always: its Unlocks rows as its own) */
    eax = MEM32(esp + 0x10);
    ecx = MEM32(esp + 0x1C);
    _fb = (uint32_t)(esi) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ebx = ebx + esi;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ecx (32-bit) */
    MEM32(esp + 0x10) = eax;
    if (CMP_L(_fas, _fbs)) goto loc_0021F1F5; /* jl: less (signed <) */

loc_0021F5DC: ;
    eax = MEM32(esp + 0x68);
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebx);
    esp = ebp;
    POP32(esp, ebp);
    esp += 4; return; /* ret */

loc_0021F5E7: ;
    POP32(esp, edi);
    eax = esi;
    POP32(esp, esi);
    POP32(esp, ebx);
    esp = ebp;
    POP32(esp, ebp);
    esp += 4; return; /* ret */

    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}

/**
 * sub_00220870
 * Original: 0x00220870 - 0x00220A5A (490 bytes, 106 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_00220870(void)
{
    /* PC: the original, in C, with the PC rows' descriptions. The page
     * level (0 the tabs, 1 a tab's rows, 2 a page of its own), the tab and
     * the row pick a description text id (category 2) into [ecx]. */
    static const uint16_t tabs[] = { 0x48, 0x32, 0x33, 0x34, 0x35, 0x36 };
    static const uint16_t general_all[] = { 0x49, 0x37, 0x38, 0x39, 0x4A, 0x3A, 0x3B };
    static const uint16_t general_noflag[] = { 0x49, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3B };
    const uint16_t *general = ngb_general_no_flag() ? general_noflag : general_all;
    static const uint16_t audio[] = { 0x3C, 0x3D, 0x3E, 0x3F, 0x3A, 0x3B };
    static const uint16_t control[] = { 0x40, 0x43, 0x44, 0x45, 0x3A, 0x3B };
    static const uint16_t video_kind[] = { 0x46, 0x47, D_PC & 0xFFFF, 0x3A, 0x3B,
                                           D_RENDER & 0xFFFF, D_TEXTURE & 0xFFFF,
                                           D_SHADOW & 0xFFFF, D_DISPLAY & 0xFFFF,
                                           D_VSYNC & 0xFFFF, D_FPS & 0xFFFF,
                                           D_WINDOWSZ & 0xFFFF, D_ASPECT & 0xFFFF, D_BACK & 0xFFFF,
                                           D_SKIP & 0xFFFF, D_LAUNCHER & 0xFFFF, D_MSAA & 0xFFFF,
                                           D_HEALTH & 0xFFFF, D_NINPO & 0xFFFF, D_ESSENCE & 0xFFFF,
                                           D_ALLDIFF & 0xFFFF, D_SCARABS & 0xFFFF, D_ALLCOST & 0xFFFF,
                                           D_EXTRACOST & 0xFFFF, D_GLOW & 0xFFFF,
                                           D_QUICKFREEZE & 0xFFFF, D_QUICKSCALE & 0xFFFF,
                                           D_INFITEMS & 0xFFFF, D_INFAMMO & 0xFFFF, D_POSAUDIO & 0xFFFF,
                                           D_HUD & 0xFFFF, D_AUDIOOUT & 0xFFFF, D_MISSIONS & 0xFFFF,
                                           D_ARCADE & 0xFFFF, D_MOVIES & 0xFFFF, D_UNLOCKS & 0xFFFF,
                                           D_LOADDETAIL & 0xFFFF, D_RTHREAD & 0xFFFF, D_SPARKTHICKNESS & 0xFFFF };
    RECOMP_ENTRY(0x00220870u);
    uint32_t out = ecx;
    uint32_t level = MEM16(0x8DE430), tab = MEM16(0x8DE4CC), row = MEM16(0x8DE468);
    uint16_t id = 0;

    pc_menu_init();
    {   /* PC: the Controls panel (src/pause_tab.c) over RECOMP SETTINGS' Controls page (the game's own
         * Button Settings is left as it is: over it, leaving it showed its page and moved its cursor;
         * NGB_CONTROLS_IN_OPTIONS=1 puts it there too) */
        extern void ngb_controls_panel_tick(int want);
        int ours = s_recomp && page_now() == PAGE_CTRL;
        int game = !s_recomp && level == 2 && tab == 2 && getenv("NGB_CONTROLS_IN_OPTIONS");
        ngb_controls_panel_tick(ours || game);
        if (ours || game) {
            MEM16(out) = (uint16_t)(D_CONTROLS & 0xFFFF);   /* the panel's line */
            esp += 4; return;
        }
    }
    if (s_recomp && is_ours(page_now())) {
        int kind = row_kind((int)row);
        if (kind >= 0)
            id = video_kind[kind];
    } else if (s_recomp && level == 0) {                /* RECOMP SETTINGS' categories */
        id = tab == PAGE_CHEAT - PAGE_GFX ? (uint16_t)(D_CHEATS & 0xFFFF)
           : tab == PAGE_GAME - PAGE_GFX ? (uint16_t)(D_GAMEPLAY & 0xFFFF)
           : tab == PAGE_CTRL - PAGE_GFX ? (uint16_t)(D_CONTROLS & 0xFFFF)
           : tab < N_OURS ? (uint16_t)((D_CAT0 + tab) & 0xFFFF) : 0x36;
    } else if (level == 0) {
        if (tab < 6)
            id = tabs[tab];
    } else if (level == 1) {
        if (tab == 0 && row < 7) id = general[row];
        else if (tab == 1 && row < 6) id = audio[row];
        else if (tab == 2 && row < 6) id = control[row];
        else if (tab == 3) {
            int kind = row_kind((int)row);
            if (kind >= 0)
                id = video_kind[kind];
        }
    } else if (level == 2) {
        if (MEM16(0x8DE4CC) == 2)
            id = row < 0xA ? 0x41 : 0x3A;
    }
    MEM16(out) = id;
    esp += 4; return; /* ret */
}

/* The menus' and HUD's narrowing: the game lays them out for 4:3 and draws
 * them over the 3D whatever the screen's shape, stretched at 16:9. The
 * Direct3D renderer draws its screen-space draws into the back buffer
 * (the menus, the HUD, the words) narrowed by this about the middle, in a
 * 4:3 area there (src/d3d/vk/backend.cpp); what spans the screen spans it
 * still. 4:3 over the screen's shape; 1 at 4:3, or with NGB_HUD_STRETCH=1
 * (the console's way). */
float ngb_hud_shape(void)
{
    static int stretch = -1;
    if (stretch < 0)
        stretch = getenv("NGB_HUD_STRETCH") != NULL;
    if (stretch)
        return 1.0f;
    int a = ngb_pc_setting(7);
    return a == 1 ? 0.75f : a == 2 ? 0.8333333f : a == 3 ? 0.5625f : 1.0f;
}
