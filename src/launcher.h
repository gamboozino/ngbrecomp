/*
 * launcher.h -- the settings screen shown before the game, and the settings
 * file behind it ($XDG_CONFIG_HOME/NinjaGaidenBlack/settings.ini).
 */
#ifndef NGB_LAUNCHER_H
#define NGB_LAUNCHER_H
#include <stddef.h>

enum { ASPECT_4_3, ASPECT_16_9, ASPECT_16_10, ASPECT_21_9, ASPECT_32_9, ASPECT_COUNT };

typedef struct NgbSettings {
    int aspect;          /* ASPECT_* */
    int width, height;   /* window size */
    int fullscreen;
    int scale;           /* render scale 1..10 */
    int vsync;
    int show_fps;        /* frame rate and frame time overlay */
    int anisotropy;      /* 0 the console's texture filtering; 2, 4, 8, 16 trilinear + anisotropic */
    int msaa;            /* MSAA samples: 1 (off), 2, 4, 8 */
    int shadow_scale;    /* shadow map resolution multiple: 1, 2, 4 or 8 */
    int quick_menu_freeze; /* pause gameplay while the D-pad carousel is open */
    int quick_menu_scale;  /* whole menu size: 100..10 in tens; 0 disables it */
    int skip_intro;
    int show_launcher;   /* show the settings screen at startup */
    int all_difficulties;
    int all_costumes;       /* every costume at New Game's Costume Selection */
    int unlock[3];          /* the main menu's MISSIONS, ARCADE NINJA GAIDEN and MOVIE THEATER (src/cheats.c) */
    int movie_43;           /* the game's Movie Display Ratio 4:3 (kept here: src/pc_menu.c) */
    int positional_audio;   /* 3D sounds placed left and right (#30): 0 off, 1 the HRTF filters, 2 panning only (#86) */
    int show_hud;           /* the in-play HUD (src/hud.c, #80) */
    int audio_output;       /* 0 auto (5.1 on a 5.1 or 7.1 device), 1 stereo, 2 5.1 (src/audio/dsound_hle.cpp) */
    int smooth_glow;        /* the glow's continuous filtering (src/d3d/vk/glow_filter.h, #64) */
    int detail_under_load;  /* the game's own detail lowering when the GPU looks busy (src/d3d/d3d.c, #50, #114) */
    int render_thread;      /* Multithreaded Renderer: drawing on a thread of its own (src/d3d/gpu/threaded.cpp) */
    int intercept;          /* Gameplay > Intercept (src/intercept.c, #109): 0 off, 1 Black's, 2 the Hurricane Pack's */
    int essence_charging;   /* Gameplay > Essence-Only Charging (src/ut_charge.c, #120) */
    int skip_cutscenes;     /* Gameplay > Skip Any Cutscene (src/cutscene_skip.c, #139) */
    int responsive_menus;   /* Gameplay > Responsive Menus (src/responsive.c, #116) */
    /* Hard, Very Hard and Master Ninja offered without clearing the one before */
} NgbSettings;

void ngb_settings_load(NgbSettings *s);
void ngb_settings_save(const NgbSettings *s);

/* The settings shown on the settings screens, in order: aspect, window
 * size, display, render resolution, anisotropic filtering, MSAA, shadow resolution,
 * V-Sync, FPS counter, skip intros. */
#define NGB_SETTINGS_ITEMS 10
/* Step setting item one way (dir -1/+1); its label and current value. */
void ngb_settings_change(NgbSettings *s, int item, int dir);
void ngb_settings_row_text(const NgbSettings *s, int item, const char **label,
                           char *val, size_t n);

/* V-Sync's values: 0 Off, 1 Mailbox (the newest frame at each refresh: least delay; what Nvidia calls Fast),
 * 2 Adaptive (FIFO relaxed), 3 FIFO (every frame in turn: classic V-Sync). Its name, and the next one the
 * menus step to, in the order Off, Mailbox, FIFO, Adaptive. */
const char *ngb_vsync_name(int v);
int ngb_vsync_step(int v, int dir);

/* The quick menu's size (Gameplay): 25, 50, 75 or 100 percent, or 0 (Off). */
int ngb_quick_scale_value(int v);
int ngb_quick_scale_step(int v, int dir);

/* The window sizes offered for an aspect ratio, smallest first; the index of
 * the settings' own (-1: none of them). */
int ngb_settings_size_count(int aspect);
void ngb_settings_size_at(int aspect, int i, int *w, int *h);
int ngb_settings_size_index(const NgbSettings *s);
void ngb_settings_snap_size(NgbSettings *s);   /* the nearest size of s's aspect */

/* Width/height of the aspect, for the presenter. */
double ngb_settings_aspect_ratio(const NgbSettings *s);

/* Runs the settings screen in its own window. Returns 1 to start the game,
 * 0 to quit. Saves the settings when the game is started. */
int ngb_launcher_run(NgbSettings *s);

/* The settings the game was started with, for the in-game menu (F1 or
 * Back+Start), which starts from them and changes them. */
void ngb_menu_set_running(const NgbSettings *s);

#endif
