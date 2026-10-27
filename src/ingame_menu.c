/*
 * ingame_menu.c -- the settings menu over the running game.
 *
 * F1, or Back+Start on the pad, opens it. The presenter holds the game at
 * the frame on screen (its next flip waits), the sound stops and the game
 * sees no pad input; the menu is drawn here, in software, over that frame,
 * in the settings screen's style and with its font. Closing it applies what
 * changed (window, aspect, V-Sync, FPS counter, render resolution, texture
 * filtering, shadow resolution) and saves the settings.
 */
#include <SDL.h>
#include "port.h"
#include <SDL_vulkan.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#if !defined(_WIN32)
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#endif

#include "launcher.h"
#include "launcher_font.h"
#include "nv2a_window.h"
#include "input/xinput_xbox.h"
#include "usb/usb_gamepad.h"
#include "apu/apu.h"

enum { KEY_UP = 1, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_OK, KEY_BACK, KEY_CLEAR };

/* The settings the game runs with; what the menu starts from. */
static NgbSettings s_running;
static bool s_have_running;

static volatile int s_want;              /* open at the next frame */
static bool s_open;
static NgbSettings s_cfg;
static int s_sel;
static bool s_quit_armed;
static bool s_wait_release;              /* pad blocked until let go */

/* Keys from the main thread (window events) to the renderer's thread. */
static volatile int s_keys[32];
static volatile unsigned s_khead, s_ktail;

#define ROW_RESUME NGB_SETTINGS_ITEMS
#define ROW_QUIT   (NGB_SETTINGS_ITEMS + 1)
#define NROWS      (NGB_SETTINGS_ITEMS + 2)

/* The picture's shape, now: the game's own mode (src/pc_menu.c), the
 * presenter's and the window's -- the window takes the nearest size of the
 * new shape's. */
void ngb_game_set_aspect(int widescreen, double a);

static void apply_aspect(NgbSettings *s, bool window)
{
    ngb_game_set_aspect(s->aspect != ASPECT_4_3, ngb_settings_aspect_ratio(s));
    nv2a_present_set_aspect(ngb_settings_aspect_ratio(s));
    if (window)
        nv2a_window_request(s->width, s->height, s->fullscreen);
}

void ngb_menu_set_running(const NgbSettings *s)
{
    s_running = *s;
    s_have_running = true;
}

/* The settings in the game's own menus (src/pc_menu.c, src/pause_tab.c),
 * on the game's thread: 0 render scale (1-8), 1 smooth texture filtering,
 * 2 the shadow map scale (1, 2, 4, 8), 3 fullscreen, 4 V-Sync, 5 the FPS
 * counter, 6 the window size (aspect * 8 + its place in the aspect's
 * list), 7 the aspect ratio, 8 skip the intros, 9 the settings screen at
 * startup, 10 MSAA samples (1, 2, 4, 8); 1 is the anisotropy (0, 2-16);
 * 11-13 the cheats (src/cheats.c: infinite health, ninpo, essence), 14 every
 * difficulty from the start (src/cheats.c, saved), 15 the golden scarabs
 * cheat, 16 every costume (saved), 17 an extra costume (0-3, saved), 18 Smoother
 * Glow (src/d3d/vk/glow_options.h), 19 quick-menu pause, 20 quick-menu scale (100..0
 * percent, in tens), 21 infinite items, 22 infinite projectiles (cheats, src/cheats.c), 23 positional
 * audio (0 off, 1 the HRTF filters, 2 panning only; #30, #86), 24 the HUD (src/hud.c, #80), 25 the audio
 * output (0 auto, 1 stereo, 2 5.1; src/audio), 26-28 the main menu's MISSIONS, ARCADE NINJA GAIDEN and
 * MOVIE THEATER unlocked (src/cheats.c, saved), 29 Lower Detail Under Load (src/d3d/d3d.c), 30 the render thread
 * (src/d3d/hle.cpp, experimental). A change applies at once (8, 9 and 25 at the next start, 30 after a restart) and is saved, the
 * cheats' not. */
int ngb_cheat(int which);
void ngb_cheat_set(int which, int on);
void ngb_audio_set_positional(int mode);  /* src/audio/dsound.c: positional audio (23) */
void ngb_audio_set_output(int choice);    /* src/audio/dsound.c: the audio output (25) */
void ngb_set_all_difficulties(int on);   /* src/cheats.c */
void ngb_set_all_costumes(int on);
void ngb_set_unlock(int which, int on);

int ngb_pc_setting(int which)
{
    if (which >= 11 && which <= 13)
        return ngb_cheat(which - 11);
    if (which == 15)
        return ngb_cheat(3);
    if (which == 21 || which == 22)                    /* infinite items, infinite projectiles */
        return ngb_cheat(which - 17);
    if (which == 14 || which == 16 || which == 17) {
        ngb_pc_setting(0);
        return which == 14 ? s_running.all_difficulties : which == 16 ? s_running.all_costumes : s_running.movie_43;
    }
    if (which >= 26 && which <= 28) {
        ngb_pc_setting(0);
        return s_running.unlock[which - 26];
    }
    if (!s_have_running) {
        ngb_settings_load(&s_running);
        s_have_running = true;
    }
    switch (which) {
    case 0:  return s_running.scale;
    case 1:  return s_running.anisotropy;
    case 2:  return s_running.shadow_scale;
    case 3:  return s_running.fullscreen;
    case 4:  return s_running.vsync;
    case 6: {                                  /* the window size: aspect * 8 + its index */
        int i = ngb_settings_size_index(&s_running);
        return s_running.aspect * 8 + (i < 0 ? 0 : i);
    }
    case 7:  return s_running.aspect;
    case 8:  return s_running.skip_intro;
    case 9:  return s_running.show_launcher;
    case 10: return s_running.msaa;
    case 18: return s_running.smooth_glow;
    case 19: return s_running.quick_menu_freeze;
    case 20: return s_running.quick_menu_scale;
    case 23: return s_running.positional_audio;
    case 24: return s_running.show_hud;
    case 29: return s_running.detail_under_load;
    case 30: return s_running.render_thread;
    case 31: return s_running.spark_thickness;
    case 25: return s_running.audio_output;
    default: return s_running.show_fps;
    }
}

void ngb_pc_set_setting(int which, int value)
{
    if ((which >= 11 && which <= 13) || which == 15 || which == 21 || which == 22) {
        ngb_cheat_set(which == 15 ? 3 : which >= 21 ? which - 17 : which - 11, value);
        return;
    }
    ngb_pc_setting(0);
    switch (which) {
    case 14: s_running.all_difficulties = value; ngb_set_all_difficulties(value); break;
    case 16: s_running.all_costumes = value; ngb_set_all_costumes(value); break;
    case 26: case 27: case 28: s_running.unlock[which - 26] = !!value; ngb_set_unlock(which - 26, !!value); break;
    case 17: s_running.movie_43 = !!value; break;   /* the next movie (src/pc_menu.c) */
    case 0: s_running.scale = value; break;
    case 1: s_running.anisotropy = value; break;
    case 10: s_running.msaa = value; break;
    case 19: s_running.quick_menu_freeze = !!value; break;
    case 20: s_running.quick_menu_scale = ngb_quick_scale_value(value); break;
    case 2: s_running.shadow_scale = value; break;
    case 3:
        s_running.fullscreen = value;
        nv2a_window_request(s_running.width, s_running.height, value);
        break;
    case 4: s_running.vsync = value; nv2a_present_set_vsync(value); break;
    case 5: s_running.show_fps = value; nv2a_present_set_show_fps(value); break;
    case 8: s_running.skip_intro = value; break;        /* both at the next start */
    case 9: s_running.show_launcher = value; break;
    case 18: s_running.smooth_glow = !!value; break;
    case 23: s_running.positional_audio = value < 0 ? 0 : value > 2 ? 2 : value; ngb_audio_set_positional(s_running.positional_audio); break;
    case 24: s_running.show_hud = !!value; break;
    case 29: s_running.detail_under_load = !!value; break;
    case 30: s_running.render_thread = !!value; break;   /* at the next start */
    case 31: s_running.spark_thickness = value < 0 ? 0 : value > 3 ? 3 : value; break;
    case 25:   /* the mixer's at the next start; the arcade's at its next */
        s_running.audio_output = value < 0 ? 0 : value > 2 ? 2 : value;
        ngb_audio_set_output(s_running.audio_output);
        break;
    case 6:
        if (value / 8 == s_running.aspect && value % 8 < ngb_settings_size_count(value / 8)) {
            ngb_settings_size_at(value / 8, value % 8, &s_running.width, &s_running.height);
            nv2a_window_request(s_running.width, s_running.height, s_running.fullscreen);
        }
        break;
    case 7:
        s_running.aspect = value < 0 ? 0 : value >= ASPECT_COUNT ? ASPECT_COUNT - 1 : value;
        ngb_settings_snap_size(&s_running);            /* the new shape's nearest size */
        apply_aspect(&s_running, true);
        break;
    }
    if (which <= 2 || which == 10)
        nv2a_set_quality(s_running.scale, s_running.anisotropy, s_running.shadow_scale, s_running.msaa);
    if (!getenv("NGB_NO_SETTINGS"))
        ngb_settings_save(&s_running);
}

/* Opens the recomp settings menu at the next frame (the main menu's "RECOMP SETTINGS"). */
void ngb_menu_open(void)
{
    s_want = 1;
}

static void push_key(int k)
{
    unsigned t = __atomic_load_n(&s_ktail, __ATOMIC_ACQUIRE);
    if (t - __atomic_load_n(&s_khead, __ATOMIC_ACQUIRE) >= 32)
        return;
    s_keys[t % 32] = k;
    __atomic_store_n(&s_ktail, t + 1, __ATOMIC_RELEASE);
}

static int pop_key(void)
{
    unsigned h = __atomic_load_n(&s_khead, __ATOMIC_ACQUIRE);
    if (h == __atomic_load_n(&s_ktail, __ATOMIC_ACQUIRE))
        return 0;
    int k = s_keys[h % 32];
    __atomic_store_n(&s_khead, h + 1, __ATOMIC_RELEASE);
    return k;
}

/* Mouse events in the queue: the position in drawable pixels, packed. */
#define MOUSE_MOVE  0x10000000
#define MOUSE_CLICK 0x20000000
#define MOUSE_WHEEL 0x40000000   /* low bit: 1 up, 0 down */

static void push_mouse(int kind, const SDL_Event *ev, int x, int y)
{
    SDL_Window *w = SDL_GetWindowFromID(kind == MOUSE_MOVE ? ev->motion.windowID
                                                           : ev->button.windowID);
    int ww = 1, wh = 1, dw = 1, dh = 1;
    if (w) {
        SDL_GetWindowSize(w, &ww, &wh);
        SDL_Vulkan_GetDrawableSize(w, &dw, &dh);
        if (dw <= 0 || dh <= 0) { dw = ww; dh = wh; }
    }
    x = ww > 0 ? x * dw / ww : x;
    y = wh > 0 ? y * dh / wh : y;
    if (x < 0 || y < 0 || x >= 16384 || y >= 16384)
        return;
    push_key(kind | x << 14 | y);
}

/* ---------- the pause menu's Options tab (src/pause_tab.c) ---------- */

/* Milliseconds, the same on every thread (SDL's ticks are not here). */
static uint64_t mono_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

static volatile uint64_t s_tab_seen;      /* last frame the tab was shown */
static volatile int s_tab_focus;          /* its page has the pad */
static volatile int s_tab_keys[16];
static volatile unsigned s_tab_head, s_tab_tail;

static bool tab_shown(void)
{
    return mono_ms() - s_tab_seen < 250;
}

static void tab_push(int k)
{
    unsigned t = s_tab_tail;
    if (t - s_tab_head < 16) {
        s_tab_keys[t % 16] = k;
        __atomic_store_n(&s_tab_tail, t + 1, __ATOMIC_RELEASE);
    }
}

static int tab_pop(void)
{
    unsigned h = s_tab_head;
    if (h == __atomic_load_n(&s_tab_tail, __ATOMIC_ACQUIRE))
        return 0;
    int k = s_tab_keys[h % 16];
    s_tab_head = h + 1;
    return k;
}

static uint32_t pad_bits(void);
static uint32_t s_tab_prev, s_tab_rbit;

/* F1 (and Back+Start) no longer open the settings menu: it is reached from the
 * game's own menus (RECOMP SETTINGS, the pause menu's Settings tab).
 * NGB_F1_MENU=1 brings the keys back. */
static int f1_menu(void)
{
    static int on = -1;
    if (on < 0) {
        on = getenv("NGB_F1_MENU") != NULL;
        usb_gamepad_back_start_host = on;   /* else Start+Back is the game's soft reset (#38) */
    }
    return on;
}
static uint64_t s_tab_rat;

/* The game's thread, each frame the Options tab is the chosen one: A (or
 * Enter) gives its page the pad, B (Esc) gives it back; while the page has
 * it the game sees no pad, and the key for the page (KEY_UP .. KEY_BACK, the
 * back already acted on) is returned. */
/* The page keeps the pad after a back it acted on itself (src/pause_tab.c: B on Cheats' Unlocks page
 * goes back to Cheats, not out of the page). */
void ngb_pause_tab_hold(void)
{
    s_tab_focus = 1;
    s_wait_release = false;
}

int ngb_pause_tab_key(int *focus)
{
    static const struct { uint32_t bit; int key; } map[] = {
        { XBOX_GAMEPAD_DPAD_UP, KEY_UP }, { XBOX_GAMEPAD_DPAD_DOWN, KEY_DOWN },
        { XBOX_GAMEPAD_DPAD_LEFT, KEY_LEFT }, { XBOX_GAMEPAD_DPAD_RIGHT, KEY_RIGHT },
        { 1u << 16, KEY_OK }, { 1u << 17, KEY_BACK }, { XBOX_GAMEPAD_START, KEY_BACK }, { 1u << 20, KEY_CLEAR },
    };
    uint64_t now = mono_ms();
    {
        extern int ngb_bind_capturing(void);   /* src/bindings.c: the page waits for a key to bind */
        if (ngb_bind_capturing() && *focus) {
            s_tab_seen = now;
            s_tab_prev = pad_bits();           /* what is pressed meanwhile is the binding's */
            while (tab_pop())
                ;
            return 0;
        }
    }
    if (*focus && !s_tab_focus)            /* given back meanwhile */
        *focus = 0;
    if (now - s_tab_seen > 250) {          /* just shown: what is held does nothing */
        s_tab_prev = pad_bits();
        while (tab_pop())
            ;
    }
    s_tab_seen = now;
    uint32_t b = pad_bits(), pressed = b & ~s_tab_prev;
    if (xbox_InputKeyboardActive())
        pressed &= ~XBOX_GAMEPAD_START;    /* Enter's: the keys come as keys (nv2a_window_event_hook) */
    int key = tab_pop();
    {   /* NGB_OPTIONS_TEST=<keys>: for headless tests, one key every 400 ms
         * from a second after the tab is first shown -- u d l r o b. */
        static const char *t;
        static uint64_t at;
        if (!at) {
            t = getenv("NGB_OPTIONS_TEST");
            at = now + 1000;
        }
        const char *go = getenv("NGB_OPTIONS_TEST_GO");   /* and only once this file exists */
        if (go && t && *t && access(go, F_OK) != 0)
            at = now + 1000;
        if (!key && t && *t && now >= at) {
            char c = *t++;
            at = now + 400;
            key = c == 'u' ? KEY_UP : c == 'd' ? KEY_DOWN : c == 'l' ? KEY_LEFT
                : c == 'r' ? KEY_RIGHT : c == 'o' ? KEY_OK : c == 'b' ? KEY_BACK : 0;
            fprintf(stderr, "[OPTIONS] test key %c\n", c);
        }
    }
    s_tab_prev = b;
    for (unsigned i = 0; !key && i < sizeof map / sizeof map[0]; i++)
        if (pressed & map[i].bit) {
            key = map[i].key;
            s_tab_rbit = map[i].bit;
            s_tab_rat = now + 380;
        }
    if (!key && s_tab_rbit && (b & s_tab_rbit) && s_tab_rbit <= XBOX_GAMEPAD_DPAD_RIGHT &&
        now >= s_tab_rat) {
        s_tab_rat = now + 90;
        for (unsigned i = 0; i < 4; i++)
            if (map[i].bit == s_tab_rbit)
                key = map[i].key;
    }
    if (!(b & s_tab_rbit))
        s_tab_rbit = 0;

    if (!*focus) {
        if (key == KEY_OK && !s_open) {
            *focus = s_tab_focus = 1;
            usb_gamepad_block = 1;
        }
        return 0;                          /* the game's pad meanwhile */
    }
    if (key == KEY_BACK) {
        *focus = s_tab_focus = 0;
        s_wait_release = true;             /* the game gets the pad once let go */
    }
    if (pressed & (3u << 18)) {            /* L or R: to the next tab, as the game's pages do */
        *focus = s_tab_focus = 0;
        s_wait_release = false;
        usb_gamepad_block = 0;             /* now: the game sees the press */
        return 0;
    }
    return key;
}

/* The Are you sure? box (src/pause_tab.c) has the pad as a tab's page
 * would: the game sees none until it is given back and let go. */
void ngb_tab_take_pad(void)
{
    s_tab_focus = 1;
    usb_gamepad_block = 1;
    s_wait_release = false;
    s_tab_seen = mono_ms();
    s_tab_prev = pad_bits();               /* what is held now does nothing */
}

void ngb_tab_give_pad(void)
{
    s_tab_focus = 0;
    s_wait_release = true;
}

/* The Controls panel over the Options menu (src/pause_tab.c) closed: the pad
 * back once let go, then B for the menu, which goes back from the page that
 * opened it. */
static bool s_back_pending;
void ngb_tab_give_pad_back(void)
{
    s_tab_focus = 0;
    s_wait_release = true;
    s_back_pending = true;
}

/* Exit's page (src/pause_tab.c): its three ways out. */
static void pulse_push(uint32_t p);

/* Return to Game: the page lets go of the pad and Start closes the menu. */
void ngb_pause_resume(void)
{
    s_tab_focus = 0;
    s_wait_release = false;
    usb_gamepad_block = 0;
    pulse_push(XBOX_GAMEPAD_START);
}

/* Exit to Desktop: as closing the window does. */
void ngb_quit_to_desktop(void)
{
    SDL_Event e = { .type = SDL_QUIT };
    SDL_PushEvent(&e);
}

/* Return to Main Menu: the program again, in this process (Steam and the
 * like still see the same game running), with the intros skipped and Start
 * pressed at the title (ngb_to_menu_tick). The settings are saved as they
 * change. */
static char **s_argv;
#if !defined(_WIN32)
static void note_inherited_fds(void);
#endif
void ngb_set_argv(char **argv)
{
    s_argv = argv;
#if !defined(_WIN32)
    note_inherited_fds();   /* at the start, before the program opens its own */
#endif
}

#if !defined(_WIN32)
/* The new program image keeps every file still open, unless it is marked close-on-exec: the game's
 * files (about 255 each run) piled up until a desktop's 1024-file limit made the next open fail and the
 * game wait for it on a black window -- the third Return to Main Menu. */
/* Only the program's own: what it was started with stays open across the exec, as the starter expects.
 * The AppImage runtime hands the program a descriptor that keeps its FUSE mount alive; closed, the mount
 * went with it and the new image died reading its own code (SIGBUS) on Return to Main Menu. */
static unsigned char s_inherited[1024];
#ifdef __APPLE__
static const char s_fd_dir[] = "/dev/fd";
#else
static const char s_fd_dir[] = "/proc/self/fd";
#endif

static void note_inherited_fds(void)
{
    DIR *d = opendir(s_fd_dir);
    struct dirent *e;

    if (!d)
        return;
    while ((e = readdir(d))) {
        int fd = atoi(e->d_name);
        if (fd >= 0 && fd < 1024 && fd != dirfd(d))
            s_inherited[fd] = 1;
    }
    closedir(d);
}

static void close_on_exec_all(void)
{
    DIR *d = opendir(s_fd_dir);
    struct dirent *e;

    if (!d)
        return;
    while ((e = readdir(d))) {
        int fd = atoi(e->d_name);
        if (fd > 2 && fd != dirfd(d) && !(fd < 1024 && s_inherited[fd]))
            fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
    }
    closedir(d);
}

/* A real-time signal sent just before the exec -- the virtual CPU's preemption, every few milliseconds --
 * stays pending into the new image, where it still has the default action, which ends the process
 * (about one Return to Main Menu in twenty). Ignored, it is dropped, and stays so until the new image
 * puts its handlers back. */
static void quiet_signals(void)
{
    sigset_t all, none;
    struct sigaction ign;

    sigfillset(&all);
    sigemptyset(&none);
    memset(&ign, 0, sizeof ign);
    ign.sa_handler = SIG_IGN;
    pthread_sigmask(SIG_BLOCK, &all, NULL);
#ifdef __APPLE__
    sigaction(SIGVTALRM, &ign, NULL);
    sigaction(SIGURG, &ign, NULL);
#else
    for (int sig = SIGRTMIN; sig <= SIGRTMAX; sig++)
        sigaction(sig, &ign, NULL);
#endif
    pthread_sigmask(SIG_SETMASK, &none, NULL);
}
#endif

/* Return to Main Menu: the game's own soft reset, Start+Back held (a second does it, 300 ms does
 * not), then Start at the title as after a restart. The same process, so on Windows the window
 * keeps the focus, which a new process could not take (#38). Starting the program again is left
 * for when no title comes. */
static volatile uint64_t s_soft_reset_at;   /* asked for at; 0 none */
static void to_menu_arm(void);
static void restart_program(void);

void ngb_restart_to_menu(void)
{
    s_tab_focus = 0;                        /* the pad back to the game, as Return to Game does */
    s_wait_release = false;
    usb_gamepad_block = 0;
    to_menu_arm();
    fprintf(stderr, "[MENU] Return to Main Menu: Start+Back\n");
    __atomic_store_n(&s_soft_reset_at, mono_ms(), __ATOMIC_RELEASE);
}

void ngb_keep_game_for_restart(void);   /* src/main.c */

/* Started again without the arguments (the game found as at any start: the folder kept, #59) */
static void restart_program(void)
{
    ngb_keep_game_for_restart();
#if !defined(_WIN32)
    char self[4096];
    long n = ngb_exe_path(self, sizeof self);
    if (n > 0 && s_argv) {
        self[n] = 0;
        setenv("NGB_TO_MAIN_MENU", "1", 1);
        fprintf(stderr, "[MENU] Return to Main Menu: starting %s again\n", self);
        fflush(stderr);
        close_on_exec_all();
        quiet_signals();
        char *args[2] = { s_argv[0], NULL };
        execv(self, args);
        fprintf(stderr, "[MENU] could not start again; exiting\n");
    }
#else
    /* No exec on Windows: a new process with this one's command line, then this one ends at once
     * (as an exec would replace it). */
    {
        wchar_t self[MAX_PATH];
        STARTUPINFOW si;
        PROCESS_INFORMATION pi;
        memset(&si, 0, sizeof si);
        si.cb = sizeof si;
        if (GetModuleFileNameW(NULL, self, MAX_PATH) && SetEnvironmentVariableA("NGB_TO_MAIN_MENU", "1")) {
            fprintf(stderr, "[MENU] Return to Main Menu: starting the program again\n");
            fflush(stderr);
            static wchar_t line[MAX_PATH + 3];
            swprintf(line, MAX_PATH + 3, L"\"%ls\"", self);   /* the program alone */
            if (CreateProcessW(self, line, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
                TerminateProcess(GetCurrentProcess(), 0);
            }
            fprintf(stderr, "[MENU] could not start again (error %lu); exiting\n", GetLastError());
        }
    }
#endif
    ngb_quit_to_desktop();
}

/* After such a start: Start at the title -- while its PRESS START BUTTON
 * is drawn (src/pause_tab.c says so), at most every 3 s -- until the main
 * menu is up (src/main_menu.c says so), for 60 s at most. */
static volatile int s_main_menu_seen;
static volatile uint64_t s_title_at;
void ngb_main_menu_seen(void) { s_main_menu_seen = 1; }
void ngb_title_seen(void)
{
    extern void ngb_boot_over(void);   /* src/pc_menu.c */
    ngb_boot_over();
    if (!s_title_at && getenv("NGB_MENU_LOG"))
        fprintf(stderr, "[MENU] title up\n");
    s_title_at = mono_ms();
}

/* Holds Start+Back for the reset; the title not up 8 s on, the program starts again. */
static void soft_reset_tick(void)
{
    const uint8_t combo = XBOX_GAMEPAD_START | XBOX_GAMEPAD_BACK;
    static int held;
    uint64_t at = __atomic_load_n(&s_soft_reset_at, __ATOMIC_ACQUIRE), now = mono_ms();
    if (!at)
        return;
    if (now - at < 1500) {
        __atomic_or_fetch(&usb_gamepad_inject, combo, __ATOMIC_RELEASE);
        held = 1;
        return;
    }
    if (held) {
        __atomic_and_fetch(&usb_gamepad_inject, (uint8_t)~combo, __ATOMIC_RELEASE);
        held = 0;
    }
    if (s_title_at > at) {
        s_soft_reset_at = 0;
    } else if (now - at > 8000) {
        s_soft_reset_at = 0;
        fprintf(stderr, "[MENU] Return to Main Menu: no title after Start+Back\n");
        restart_program();
    }
}

static int s_to_menu = -1;
static uint64_t s_to_menu_start, s_to_menu_next;

static void to_menu_arm(void)
{
    s_main_menu_seen = 0;
    s_to_menu_start = s_to_menu_next = mono_ms();
    s_to_menu = 1;
}

static void to_menu_tick(void)
{
    if (s_to_menu < 0) {
        s_to_menu = 0;
        if (getenv("NGB_TO_MAIN_MENU"))
            to_menu_arm();
        unsetenv("NGB_TO_MAIN_MENU");
    }
    soft_reset_tick();
    if (!s_to_menu || s_main_menu_seen)
        return;
    uint64_t now = mono_ms();
    if (now - s_to_menu_start > 60000) {
        s_to_menu = 0;
        return;
    }
    if (now >= s_to_menu_next && now - s_title_at < 200) {
        s_to_menu_next = now + 3000;
        pulse_push(XBOX_GAMEPAD_START);
        fprintf(stderr, "[MENU] Return to Main Menu: Start at the title\n");
    }
}

/* ---------- the mouse in the game's own menus ---------- */

int ngb_menu_up(void);                              /* src/menu_mouse.c */
int ngb_pause_tab_hit(float x, float y, int *zone); /* src/pause_tab.c */
int ngb_pause_exit_hit(float x, float y);
int ngb_pause_tab_page(void);
int ngb_confirm_hit(float x, float y);
static int tab_hit(float x, float y, int *zone)
{
    *zone = 0;
    switch (ngb_pause_tab_page()) {
    case 3:  { extern int ngb_controls_hit(float x, float y); return ngb_controls_hit(x, y); }
    case 2:  return ngb_confirm_hit(x, y);
    case 1:  return ngb_pause_exit_hit(x, y);
    default: return ngb_pause_tab_hit(x, y, zone);
    }
}
#define TAB_ROW 0x10000   /* | the row (src/pause_tab.c's K_ROW): past a byte */

/* Presses for the game's pad, played out a frame at a time (render
 * thread) so that each is seen: held 90 ms, then 60 ms let go. Low byte
 * the button bit; 0x100 an analog one (A, B, triggers), else digital. */
static volatile uint32_t s_pulse[16];
static volatile unsigned s_pulse_head, s_pulse_tail;

static SDL_SpinLock s_pulse_lock;

static void pulse_push(uint32_t p)
{
    SDL_AtomicLock(&s_pulse_lock);
    unsigned t = s_pulse_tail;
    if (t - s_pulse_head < 16) {
        s_pulse[t % 16] = p;
        __atomic_store_n(&s_pulse_tail, t + 1, __ATOMIC_RELEASE);
    }
    SDL_AtomicUnlock(&s_pulse_lock);
}

static bool pulse_idle(void);

static uint32_t s_pulse_cur;

static bool pulse_idle(void)
{
    return !s_pulse_cur && s_pulse_head == __atomic_load_n(&s_pulse_tail, __ATOMIC_ACQUIRE);
}

static void pulse_tick(void)
{
#define cur s_pulse_cur
    static uint64_t until, gap;
    uint64_t now = mono_ms();
    if (cur && now >= until) {
        if (cur & 0x100)
            __atomic_and_fetch(&usb_gamepad_inject_analog, (uint8_t)~cur, __ATOMIC_RELEASE);
        else
            __atomic_and_fetch(&usb_gamepad_inject, (uint8_t)~cur, __ATOMIC_RELEASE);
        cur = 0;
        gap = now + 40;
    }
    if (!cur && now >= gap && s_pulse_head != __atomic_load_n(&s_pulse_tail, __ATOMIC_ACQUIRE)) {
        cur = s_pulse[s_pulse_head % 16];
        s_pulse_head = s_pulse_head + 1;
        if (cur & 0x100)
            __atomic_or_fetch(&usb_gamepad_inject_analog, (uint8_t)cur, __ATOMIC_RELEASE);
        else
            __atomic_or_fetch(&usb_gamepad_inject, (uint8_t)cur, __ATOMIC_RELEASE);
        until = now + 45;
    }
#undef cur
}

/* A wheel event's direction: 1 up, -1 down, 0 none (the whole notches,
 * else the precise amount: touchpads). */
static int wheel_dir(const SDL_Event *ev)
{
    if (ev->wheel.y)
        return ev->wheel.y > 0 ? 1 : -1;
    return ev->wheel.preciseY > 0.5f ? 1 : ev->wheel.preciseY < -0.5f ? -1 : 0;
}

/* ---------- pointing at the game's menu items ---------- */

static int mouse_log(void)
{
    static int on = -1;
    if (on < 0)
        on = getenv("NGB_MOUSE_LOG") != NULL;
    return on;
}

#include "menu_mouse.h"

/* From the main thread: where the mouse is (in the 720x480), and a click
 * to make there. */
static volatile int s_mx = -1, s_my = -1;
static volatile unsigned s_move_seq, s_click_seq;
static volatile uint64_t s_move_ms;   /* when the mouse last moved */

/* Render thread, every frame: bring the game's cursor to the item under the
 * mouse -- the item, among those one routine drew (a list), the game draws
 * brightest being its cursor -- a press of the pad at a time, when the
 * mouse has moved onto another item; a click moves there, then presses A
 * (on no item: A, for screens that wait for any button). */
static void hover_tick(void)
{
    static unsigned seen_move, seen_click;
    static uint32_t target_group, target_id;
    static int target_kind, tries, confirm;
    static uint64_t next, since;
    static uint32_t done_group, done_id;         /* reached or given up on */
    static NgbTextItem it[MOUSE_ITEMS];
    unsigned move = s_move_seq, click = s_click_seq;
    bool moved = move != seen_move, clicked = click != seen_click;
    seen_move = move;
    seen_click = click;
    /* (the controller in use: the pointer is hidden, and where it rests chooses nothing) */
    if (s_open || !ngb_menu_up() || (tab_shown() && s_tab_focus) || !xbox_InputKeyboardActive()) {
        target_group = 0;
        return;
    }
    int n = ngb_menu_items(it, MOUSE_ITEMS);
    float mx = s_mx, my = s_my;

    /* The item under the mouse: of those in a list (others drawn the same
     * way at other places), the smallest box holding it. */
    int hov = -1;
    float best = 1e9f;
    for (int i = 0; i < n; i++) {
        const float pad = 3;
        if (mx < it[i].x0 - pad || mx >= it[i].x1 + pad || my < it[i].y0 - pad || my >= it[i].y1 + pad)
            continue;
        if (it[i].kind == 3)
            continue;
        int others = 0;
        for (int j = 0; j < n && !others; j++)
            others = it[j].caller == it[i].caller && it[j].str != it[i].str;
        if (!others)
            continue;
        float area = (it[i].x1 - it[i].x0) * (it[i].y1 - it[i].y0);
        if (it[i].kind == 2)
            area += 1e6f;                              /* text first, then pictures */
        if (area < best) { best = area; hov = i; }
    }
    if (moved && hov < 0 && mouse_log()) {
        fprintf(stderr, "[HOVER] nothing at %.0f,%.0f; %d items:", mx, my, n);
        for (int i = 0; i < n && i < 12; i++)
            fprintf(stderr, " %08X %.0f,%.0f-%.0f,%.0f", it[i].caller, it[i].x0, it[i].y0, it[i].x1, it[i].y1);
        fprintf(stderr, "\n");
    }
    if (clicked && mouse_log()) {
        fprintf(stderr, "[HOVER] click at %.0f,%.0f; prompts:", mx, my);
        for (int i = 0; i < n; i++)
            if (it[i].kind == 3)
                fprintf(stderr, " %02X@%.0f,%.0f-%.0f,%.0f", it[i].str, it[i].x0, it[i].y0, it[i].x1, it[i].y1);
        fprintf(stderr, "\n");
    }
    if (clicked) {                                     /* a button prompt: its button */
        for (int i = 0; i < n; i++) {
            if (it[i].kind != 3 || my < it[i].y0 - 3 || my >= it[i].y1 + 3)
                continue;
            float x1 = it[i].x1;                       /* and the word after it */
            for (int j = 0; j < n; j++)
                if (it[j].kind == 0 && it[j].x0 >= it[i].x1 - 3 && it[j].x0 <= it[i].x1 + 14 &&
                    it[j].y0 < it[i].y1 && it[j].y1 > it[i].y0 && it[j].x1 > x1)
                    x1 = it[j].x1;
            if (mx >= it[i].x0 - 3 && mx < x1 + 3) {
                pulse_push(0x100 | it[i].str);
                target_group = 0;
                confirm = 0;
                return;
            }
        }
    }
    if (hov >= 0 && !clicked && !target_group && mono_ms() - s_move_ms < 1500 &&
        (it[hov].caller != done_group || it[hov].str != done_id))
        moved = true;                                  /* just come to rest on a new item */
    if (hov >= 0 && it[hov].kind == 1 && !clicked)
        moved = false;                                 /* a tab: changes the page when clicked only */
    if ((moved || clicked) && hov >= 0) {
        if (it[hov].caller != target_group || it[hov].str != target_id || clicked) {
            target_group = it[hov].caller;
            target_id = it[hov].str;
            target_kind = it[hov].kind;
            tries = 0;
            since = mono_ms();
        }
        if (clicked && it[hov].kind != 1)            /* (a tab is gone to, nothing chosen on its page) */
            confirm = 1;
    } else if (clicked) {
        pulse_push(0x100 | 0x01);                     /* A */
        return;
    }
    if (!target_group || !pulse_idle() || mono_ms() < next)
        return;

    /* The list: the target's group, its cursor the brightest -- clearly. */
    int sel = -1, tgt = -1;
    float b1 = -1, b2 = -1;
    for (int i = 0; i < n; i++) {
        if (it[i].caller != target_group)
            continue;
        if (it[i].str == target_id && (tgt < 0 || it[i].bright > it[tgt].bright))
            tgt = i;
        if (it[i].bright > b1) {
            if (sel >= 0 && it[sel].str != it[i].str)
                b2 = b1;
            b1 = it[i].bright;
            sel = i;
        } else if (it[i].bright > b2 && (sel < 0 || it[i].str != it[sel].str)) {
            b2 = it[i].bright;
        }
    }
    if (tgt >= 0 && (sel < 0 || b1 - b2 < 0.25f || target_kind == 2)) {
        /* No brighter item: a menu of the game's menu engine marks its row
         * otherwise (a lit plate). Its row is the engine's; the items, top
         * to bottom, its rows. */
        int row = ngb_engine_row(), idx = 0, cnt = 0;
        float th = it[tgt].y1 - it[tgt].y0;
        /* The rows: things of the target's kind and about its height,
         * across the same columns, each height once (by its centre)... */
        #define IN_LIST(k) (it[k].kind == it[tgt].kind && fabsf(it[k].y1 - it[k].y0 - th) < 4 && \
                            it[k].x0 < it[tgt].x1 && it[k].x1 > it[tgt].x0 && \
                            (it[k].kind == 2 || it[k].caller == target_group))
        float cy[64];
        int nc = 0;
        for (int i = 0; i < n && nc < 64; i++)
            if (IN_LIST(i)) {
                float c = (it[i].y0 + it[i].y1) / 2;
                bool dup = false;
                for (int j = 0; j < nc; j++)
                    if (fabsf(cy[j] - c) < th / 2)
                        dup = true;
                if (!dup)
                    cy[nc++] = c;
            }
        #undef IN_LIST
        for (int i = 1; i < nc; i++)                  /* top to bottom */
            for (int j = i; j > 0 && cy[j - 1] > cy[j]; j--) {
                float t = cy[j]; cy[j] = cy[j - 1]; cy[j - 1] = t;
            }
        /* ...and of those, the evenly spaced run the target is in. */
        float tc = (it[tgt].y0 + it[tgt].y1) / 2;
        int t = 0;
        while (t < nc - 1 && fabsf(cy[t] - tc) >= th / 2)
            t++;
        if (nc >= 2) {
            float pitch = t + 1 < nc ? cy[t + 1] - cy[t] : cy[t] - cy[t - 1];
            int a = t, b = t;
            while (a > 0 && fabsf(cy[a] - cy[a - 1] - pitch) < pitch * 0.2f)
                a--;
            while (b + 1 < nc && fabsf(cy[b + 1] - cy[b] - pitch) < pitch * 0.2f)
                b++;
            cnt = b - a + 1;
            idx = t - a;
        }
        static int lrow = -2, lidx = -2;
        if (mouse_log() && (row != lrow || idx != lidx))
            fprintf(stderr, "[HOVER] engine: group %08X kind %d row %d target index %d of %d\n",
                    target_group, target_kind, row, idx, cnt);
        lrow = row;
        lidx = idx;
        int first = 0, rows = ngb_engine_rows(&first);
        row -= first;
        if (row >= 0 && cnt >= 2 && cnt == rows && row < cnt) {
            if (row == idx) {
                if (confirm)
                    pulse_push(0x100 | 0x01);
                done_group = target_group;
                done_id = target_id;
                target_group = 0;
                confirm = 0;
                return;
            }
            if (++tries > cnt + 1) {
                done_group = target_group;
                done_id = target_id;
                target_group = 0;
                confirm = 0;
                return;
            }
            pulse_push(idx < row ? XBOX_GAMEPAD_DPAD_UP : XBOX_GAMEPAD_DPAD_DOWN);
            next = mono_ms() + 60;                     /* the engine's row: read at once */
            since = mono_ms();
            return;
        }
    }
    if (tgt < 0 || sel < 0 || b1 - b2 < 0.25f || target_kind == 2) {
        /* No clear cursor: fading from one item to the next (wait), or not
         * a list with a cursor at all (give up after a while). */
        if (mono_ms() - since < 1200)
            return;
        if (confirm && tgt >= 0 && target_kind != 2)   /* never A on a row not reached */
            pulse_push(0x100 | 0x01);
        done_group = target_group;
        done_id = target_id;
        target_group = 0;
        confirm = 0;
        return;
    }
    if (it[sel].str == target_id) {                    /* there */
        if (confirm)
            pulse_push(0x100 | 0x01);
        done_group = target_group;
        done_id = target_id;
        target_group = 0;
        confirm = 0;
        return;
    }
    if (++tries > 10) {                                /* cannot get there */
        done_group = target_group;
        done_id = target_id;
        target_group = 0;
        confirm = 0;
        return;
    }
    if (mouse_log())
        fprintf(stderr, "[HOVER] group %08X sel %08X (%.2f) target %08X try %d, %d items, second %.2f\n",
                target_group, it[sel].str, b1, target_id, tries, n, b2);
    float dx = (it[tgt].x0 + it[tgt].x1 - it[sel].x0 - it[sel].x1) / 2;
    float dy = (it[tgt].y0 + it[tgt].y1 - it[sel].y0 - it[sel].y1) / 2;
    if (target_kind == 1)
        pulse_push(0x100 | (dx < 0 ? 0x40 : 0x80));   /* LT / RT */
    else if (fabsf(dy) >= fabsf(dx))
        pulse_push(dy < 0 ? XBOX_GAMEPAD_DPAD_UP : XBOX_GAMEPAD_DPAD_DOWN);
    else
        pulse_push(dx < 0 ? XBOX_GAMEPAD_DPAD_LEFT : XBOX_GAMEPAD_DPAD_RIGHT);
    next = mono_ms() + 130;                            /* the cursor moves, starts to fade */
    since = mono_ms();
}

/* A window position as the menus' 720x480. */
static void game_point(Uint32 window_id, int x, int y, float *gx, float *gy)
{
    SDL_Window *w = SDL_GetWindowFromID(window_id);
    int ww = 1, wh = 1, dw = 1, dh = 1, px0, py0, pw, ph;
    if (w) {
        SDL_GetWindowSize(w, &ww, &wh);
        SDL_Vulkan_GetDrawableSize(w, &dw, &dh);
        if (dw <= 0 || dh <= 0) { dw = ww; dh = wh; }
    }
    nv2a_present_picture_rect(dw, dh, &px0, &py0, &pw, &ph);
    float dx = ww > 0 ? (float)x * dw / ww : x, dy = wh > 0 ? (float)y * dh / wh : y;
    *gx = pw > 0 ? (dx - px0) * 720.0f / pw : -1;
    {   /* the menus are drawn stretched, no wider than 16:9 (in the game's widescreen mode,
         * ngb_2d_stretched), else narrowed about the middle on a wider screen (ngb_hud_shape) */
        extern float ngb_hud_shape(void);
        extern int ngb_2d_stretched(void);
        float k = ngb_hud_shape(), ky = 1.0f;
        if (ngb_2d_stretched()) {   /* kept 16:9: narrowed on a wider screen, shortened on a narrower */
            ky = 0.75f / k < 1.0f ? 0.75f / k : 1.0f;
            k = k * 4.0f / 3.0f < 1.0f ? k * 4.0f / 3.0f : 1.0f;
        }
        if (*gx >= 0 && k > 0)
            *gx = 360.0f + (*gx - 360.0f) / k;
        *gy = ph > 0 ? (dy - py0) * 480.0f / ph : -1;
        if (*gy >= 0 && ky > 0)
            *gy = 240.0f + (*gy - 240.0f) / ky;
    }
}

/* NGB_MOUSE_TEST=<steps>: for headless tests, mouse events from the moment
 * a game menu is up (NGB_MOUSE_TEST_ON=tab: the Options tab is shown), one
 * step every 500 ms, in the menus' 720x480: Mx,y move, Lx,y / Rx,y click,
 * U / D wheel, . wait; steps space-separated. Render thread. */
static void mouse_test_tick(void)
{
    static const char *t;
    static int state = -1;
    static uint64_t next;
    if (state < 0) {
        t = getenv("NGB_MOUSE_TEST");
        state = t && *t ? 0 : 2;
    }
    if (state == 2)
        return;
    const char *on = getenv("NGB_MOUSE_TEST_ON");
    bool ready = on && !strcmp(on, "tab") ? tab_shown() : ngb_menu_up();
    uint64_t now = mono_ms();
    if (state == 0) {
        if (!ready)
            return;
        state = 1;
        next = now + 1000;
    }
    if (now < next || !*t)
        return;
    next = now + 500;
    while (*t == ' ')
        t++;
    char c = *t++;
    float gx = 0, gy = 0;
    if (c == 'M' || c == 'L' || c == 'R') {
        gx = strtof(t, (char **)&t);
        if (*t == ',') t++;
        gy = strtof(t, (char **)&t);
    }
    SDL_Window *w = SDL_GetMouseFocus() ? SDL_GetMouseFocus() : SDL_GetKeyboardFocus();
    for (Uint32 id = 1; !w && id < 16; id++)
        w = SDL_GetWindowFromID(id);
    int ww = 1, wh = 1, dw = 1, dh = 1, px0, py0, pw, ph;
    if (w) {
        SDL_GetWindowSize(w, &ww, &wh);
        SDL_Vulkan_GetDrawableSize(w, &dw, &dh);
    }
    nv2a_present_picture_rect(dw, dh, &px0, &py0, &pw, &ph);
    int x = (int)((px0 + gx * pw / 720.0f) * ww / dw), y = (int)((py0 + gy * ph / 480.0f) * wh / dh);
    SDL_Event e;
    memset(&e, 0, sizeof e);
    Uint32 id = w ? SDL_GetWindowID(w) : 0;
    switch (c) {
    case 'M':
        e.type = SDL_MOUSEMOTION; e.motion.windowID = id; e.motion.x = x; e.motion.y = y;
        break;
    case 'L': case 'R':
        e.type = SDL_MOUSEBUTTONDOWN; e.button.windowID = id; e.button.x = x; e.button.y = y;
        e.button.button = c == 'L' ? SDL_BUTTON_LEFT : SDL_BUTTON_RIGHT;
        break;
    case 'U': case 'D':
        e.type = SDL_MOUSEWHEEL; e.wheel.windowID = id; e.wheel.y = c == 'U' ? 1 : -1;
        e.wheel.preciseY = (float)e.wheel.y;
        break;
    default:
        return;
    }
    fprintf(stderr, "[MOUSE] test %c %.0f,%.0f\n", c, gx, gy);
    SDL_PushEvent(&e);
}

/* A pause menu tab (src/pause_tab.c registers them) at x, y. */
static bool tab_item_at(float x, float y)
{
    static NgbTextItem it[MOUSE_ITEMS];
    int n = ngb_menu_items(it, MOUSE_ITEMS);
    for (int i = 0; i < n; i++)
        if (it[i].kind == 1 && x >= it[i].x0 - 3 && x < it[i].x1 + 3 && y >= it[i].y0 - 3 && y < it[i].y1 + 3)
            return true;
    return false;
}

/* Main thread: the mouse while the game is in one of its menus. True when
 * the event was used. */
static bool game_mouse(const SDL_Event *ev)
{
    static int hover = -1;
    if (mouse_log() && (ev->type == SDL_MOUSEBUTTONDOWN || ev->type == SDL_MOUSEWHEEL))
        fprintf(stderr, "[MOUSE] %s: menu %d, Options tab %d (page has the pad %d)\n",
                ev->type == SDL_MOUSEWHEEL ? "wheel" : "button", ngb_menu_up(), tab_shown(),
                s_tab_focus);
    if (ev->type == SDL_MOUSEMOTION && !s_open) {   /* always: where it rests */
        float gx, gy;
        game_point(ev->motion.windowID, ev->motion.x, ev->motion.y, &gx, &gy);
        s_mx = (int)gx;
        s_my = (int)gy;
        s_move_seq++;
        s_move_ms = mono_ms();
    }
    if (s_open || !ngb_menu_up())
        return false;
    if (tab_shown()) {                    /* the pause menu's Options page */
        float gx, gy;
        int zone, row = -1;
        if (ev->type == SDL_MOUSEMOTION) {
            game_point(ev->motion.windowID, ev->motion.x, ev->motion.y, &gx, &gy);
            row = tab_hit(gx, gy, &zone);
            if (row >= 0 && s_tab_focus && row != hover)
                tab_push(TAB_ROW | row);
            hover = row;
            s_mx = (int)gx;                       /* the tabs, for hover_tick */
            s_my = (int)gy;
            s_move_seq++;
            s_move_ms = mono_ms();
            return row >= 0 || (s_tab_focus && ngb_pause_tab_page() == 3);
        }
        if (ev->type == SDL_MOUSEBUTTONDOWN && ev->button.button == SDL_BUTTON_LEFT &&
            ngb_pause_tab_page() != 2) {
            game_point(ev->button.windowID, ev->button.x, ev->button.y, &gx, &gy);
            if (tab_item_at(gx, gy)) {            /* a tab along the top: the page lets go, hover_tick goes there */
                if (s_tab_focus) {
                    s_tab_focus = 0;
                    s_wait_release = false;
                    usb_gamepad_block = 0;
                }
                s_mx = (int)gx;
                s_my = (int)gy;
                s_click_seq++;
                return true;
            }
        }
        if (ev->type == SDL_MOUSEBUTTONDOWN) {
            game_point(ev->button.windowID, ev->button.x, ev->button.y, &gx, &gy);
            row = tab_hit(gx, gy, &zone);
            if (ev->button.button == SDL_BUTTON_LEFT && row >= 0 && ngb_pause_tab_page() == 2) {
                tab_push(TAB_ROW | row);  /* the box's Yes or No: chosen */
                tab_push(KEY_OK);
                return true;
            }
            if (ev->button.button == SDL_BUTTON_LEFT && row >= 0) {
                if (!s_tab_focus)
                    tab_push(KEY_OK);     /* takes the pad, then the row */
                tab_push(TAB_ROW | row);
                /* and does it: Exit's rows chosen; a setting's value back on its "<" half,
                 * else on (a category in the strip is only gone to) */
                if (ngb_pause_tab_page() == 1)
                    tab_push(KEY_OK);
                else if (row >= 128 && (row - 128) % 4 < 3)
                    tab_push(KEY_OK);             /* Controls' cell: waits for its new key */
                else if (row < 64)
                    tab_push(zone < 0 ? KEY_LEFT : KEY_RIGHT);
                return true;
            }
            if (ev->button.button == SDL_BUTTON_RIGHT && s_tab_focus) {
                tab_push(KEY_BACK);
                return true;
            }
        }
        if (ev->type == SDL_MOUSEWHEEL && s_tab_focus) {
            if (wheel_dir(ev))
                tab_push(wheel_dir(ev) > 0 ? KEY_UP : KEY_DOWN);
            return true;
        }
        if (s_tab_focus && ngb_pause_tab_page() == 3 &&   /* the Controls panel: the menu under it sees no mouse */
            (ev->type == SDL_MOUSEBUTTONDOWN || ev->type == SDL_MOUSEBUTTONUP || ev->type == SDL_MOUSEWHEEL))
            return true;
        if (s_tab_focus)                  /* the page has the pad */
            return ev->type == SDL_MOUSEBUTTONDOWN;
    }
    if (ev->type == SDL_MOUSEMOTION) {
        float gx, gy;
        game_point(ev->motion.windowID, ev->motion.x, ev->motion.y, &gx, &gy);
        s_mx = (int)gx;
        s_my = (int)gy;
        s_move_seq++;
        s_move_ms = mono_ms();
        return false;
    }
    if (ev->type == SDL_MOUSEBUTTONDOWN) {
        if (ev->button.button == SDL_BUTTON_LEFT) {
            float gx, gy;
            game_point(ev->button.windowID, ev->button.x, ev->button.y, &gx, &gy);
            s_mx = (int)gx;
            s_my = (int)gy;
            s_click_seq++;                         /* hover_tick: go there, A */
            return true;
        }
        switch (ev->button.button) {
        case SDL_BUTTON_RIGHT: pulse_push(0x100 | 0x02); return true;   /* B */
        case SDL_BUTTON_X1:    pulse_push(0x100 | 0x40); return true;   /* left trigger */
        case SDL_BUTTON_X2:    pulse_push(0x100 | 0x80); return true;   /* right trigger */
        default: return false;
        }
    }
    if (ev->type == SDL_MOUSEWHEEL && wheel_dir(ev)) {
        pulse_push(wheel_dir(ev) > 0 ? XBOX_GAMEPAD_DPAD_UP : XBOX_GAMEPAD_DPAD_DOWN);
        return true;
    }
    return false;
}

/* Main thread: every window event. */
void nv2a_window_event_hook(const void *sdl_event)
{
    const SDL_Event *ev = sdl_event;
    extern int ngb_quick_event(const void *event);
    if (ngb_quick_event(ev)) return;
    if (ev->type == SDL_KEYDOWN && ev->key.keysym.sym == SDLK_F7 && !ev->key.repeat) {
        extern volatile int ngb_trace_request;   /* src/d3d/vk/backend.cpp: record the next frames */
        ngb_trace_request = 1;
    }
    if (ev->type == SDL_KEYDOWN && ev->key.keysym.sym == SDLK_F8 && !ev->key.repeat) {
        extern volatile int ngb_pos_request;   /* src/cheats.c: Ryu's position to a file (debugging) */
        ngb_pos_request = 1;
    }
    {
        extern int ngb_bind_event(const SDL_Event *ev);   /* src/bindings.c: waiting for a key to bind */
        if (ngb_bind_event(ev))
            return;
    }
    /* With the controller in use (touched last) the mouse leaves the menus
     * alone; moving it makes it the device in use again. */
    if ((ev->type == SDL_MOUSEMOTION || ev->type == SDL_MOUSEBUTTONDOWN ||
         ev->type == SDL_MOUSEWHEEL) && !xbox_InputKeyboardActive())
        return;
    {
        extern int ngb_mouse_settling(void);   /* src/keyboard.c: the capture just changed */
        if (ev->type == SDL_MOUSEMOTION && (ngb_mouse_settling() || SDL_GetRelativeMouseMode()))
            return;
    }
    if (s_open && ev->type == SDL_MOUSEMOTION) {
        push_mouse(MOUSE_MOVE, ev, ev->motion.x, ev->motion.y);
        return;
    }
    if (s_open && ev->type == SDL_MOUSEBUTTONDOWN && ev->button.button == SDL_BUTTON_LEFT) {
        push_mouse(MOUSE_CLICK, ev, ev->button.x, ev->button.y);
        return;
    }
    if (s_open && ev->type == SDL_MOUSEBUTTONDOWN && ev->button.button == SDL_BUTTON_RIGHT) {
        push_key(KEY_BACK);
        return;
    }
    if (s_open && ev->type == SDL_MOUSEWHEEL) {
        if (wheel_dir(ev))
            push_key(wheel_dir(ev) > 0 ? KEY_UP : KEY_DOWN);
        return;
    }
    if (game_mouse(ev))
        return;
    if (ev->type == SDL_KEYDOWN && !ev->key.repeat && ev->key.keysym.sym == SDLK_ESCAPE &&
        !s_open && ngb_menu_up() && !(tab_shown() && s_tab_focus)) {
        pulse_push(0x100 | 0x02);                     /* Esc: B, back */
        return;
    }
    if (ev->type != SDL_KEYDOWN)
        return;
    SDL_Keycode k = ev->key.keysym.sym;
    if (!s_open && tab_shown()) {
        int key = 0;
        switch (k) {
        case SDLK_UP:     key = KEY_UP; break;
        case SDLK_DOWN:   key = KEY_DOWN; break;
        case SDLK_LEFT:   key = KEY_LEFT; break;
        case SDLK_RIGHT:  key = KEY_RIGHT; break;
        case SDLK_RETURN: case SDLK_KP_ENTER:
            if (!(ev->key.keysym.mod & KMOD_ALT)) key = KEY_OK;
            break;
        case SDLK_ESCAPE: case SDLK_BACKSPACE: key = KEY_BACK; break;
        case SDLK_DELETE: key = KEY_CLEAR; break;
        default: break;
        }
        if (key)
            tab_push(key);
    }
    if (k == SDLK_F1 && !ev->key.repeat && f1_menu()) {
        if (!s_open)
            s_want = 1;
        else
            push_key(KEY_BACK);
        return;
    }
    if (!s_open)
        return;
    switch (k) {
    case SDLK_UP:     case SDLK_w: push_key(KEY_UP); break;
    case SDLK_DOWN:   case SDLK_s: push_key(KEY_DOWN); break;
    case SDLK_LEFT:   case SDLK_a: push_key(KEY_LEFT); break;
    case SDLK_RIGHT:  case SDLK_d: push_key(KEY_RIGHT); break;
    case SDLK_RETURN: case SDLK_KP_ENTER: case SDLK_SPACE:
        if (!(ev->key.keysym.mod & KMOD_ALT))
            push_key(KEY_OK);
        break;
    case SDLK_ESCAPE: case SDLK_BACKSPACE: push_key(KEY_BACK); break;
    default: break;
    }
}

/* ---------- the pad ---------- */

static uint32_t pad_bits(void)
{
    XBOX_INPUT_STATE st;
    if (xbox_InputGetState(0, &st) != 0)
        return 0;
    const XBOX_GAMEPAD *g = &st.Gamepad;
    uint32_t b = g->wButtons;
    if (g->bAnalogButtons[0] > 40) b |= 1u << 16;     /* A */
    if (g->bAnalogButtons[1] > 40) b |= 1u << 17;     /* B */
    if (g->bAnalogButtons[6] > 40) b |= 1u << 18;     /* L (left trigger) */
    if (g->bAnalogButtons[7] > 40) b |= 1u << 19;     /* R */
    if (g->bAnalogButtons[2] > 40) b |= 1u << 20;     /* X */
    if (g->sThumbLY > 16000)  b |= XBOX_GAMEPAD_DPAD_UP;
    if (g->sThumbLY < -16000) b |= XBOX_GAMEPAD_DPAD_DOWN;
    if (g->sThumbLX < -16000) b |= XBOX_GAMEPAD_DPAD_LEFT;
    if (g->sThumbLX > 16000)  b |= XBOX_GAMEPAD_DPAD_RIGHT;
    return b;
}

static uint32_t s_prev_pad;
static uint32_t s_repeat_bit;
static uint64_t s_repeat_at;

/* Renderer's thread, every frame: open on F1 or Back+Start; give the pad
 * back to the game once it is let go after the menu. */
/* NGB_MENU_TEST=<ms>,<keys>: open the menu <ms> after the first frame and
 * press keys, one a frame: u d l r o b (up down left right ok back). For
 * headless tests. */
static const char *s_test_keys;
#define LW 960
static int row_y(int row);
static float s_k, s_ox, s_oy;
static int s_w, s_h;

static void menu_test_poll(void)
{
    static int state = -1;
    static uint64_t at;
    if (state < 0) {
        const char *e = getenv("NGB_MENU_TEST");
        state = 0;
        if (e) {
            at = mono_ms() + strtoull(e, NULL, 10);
            s_test_keys = strchr(e, ',');
            state = 1;
        }
    }
    if (state == 1 && mono_ms() >= at) {
        state = 2;
        s_want = 1;
        fprintf(stderr, "[MENU] test: open\n");
    }
}

static int test_key(void)
{
    static uint64_t next;
    if (!s_test_keys || !*s_test_keys)
        return 0;
    uint64_t now = mono_ms();
    if (now < next)
        return 0;
    next = now + 120;
    char c = *++s_test_keys;
    if (!c)
        return 0;
    if (c == 'C') {   /* a mouse click on the ">" of the shadow resolution row */
        SDL_Event e = { .type = SDL_MOUSEBUTTONDOWN };
        SDL_Window *w = SDL_GetMouseFocus() ? SDL_GetMouseFocus() : SDL_GetKeyboardFocus();
        int ww = s_w, wh = s_h, dw = s_w, dh = s_h;
        if (w) { SDL_GetWindowSize(w, &ww, &wh); SDL_Vulkan_GetDrawableSize(w, &dw, &dh); e.button.windowID = SDL_GetWindowID(w); }
        e.button.button = SDL_BUTTON_LEFT;
        e.button.x = (int)((s_ox + (LW - 100) * s_k) * ww / dw);
        e.button.y = (int)((s_oy + (row_y(5) + 20) * s_k) * wh / dh);
        SDL_PushEvent(&e);
        return 0;
    }
    return c == 'u' ? KEY_UP : c == 'd' ? KEY_DOWN : c == 'l' ? KEY_LEFT
         : c == 'r' ? KEY_RIGHT : c == 'o' ? KEY_OK : c == 'b' ? KEY_BACK : 0;
}

bool nv2a_overlay_wanted(void)
{
    menu_test_poll();
    pulse_tick();
    hover_tick();
    mouse_test_tick();
    uint32_t b = pad_bits();
    if (s_tab_focus && !tab_shown()) {     /* the menu went: give the pad back */
        s_tab_focus = 0;
        s_wait_release = true;
    }
    if (s_wait_release && !(b & 0x1FFFFF)) {   /* every bit of pad_bits: X (bit 20) too, or its press reached the game (#111) */
        s_wait_release = false;
        usb_gamepad_block = 0;
        if (s_back_pending) {
            s_back_pending = false;
            pulse_push(0x100 | 0x02);
        }
    }
    to_menu_tick();
    if (f1_menu() && !s_wait_release && (b & XBOX_GAMEPAD_BACK) && (b & XBOX_GAMEPAD_START) &&
        !((s_prev_pad & XBOX_GAMEPAD_BACK) && (s_prev_pad & XBOX_GAMEPAD_START)))
        s_want = 1;
    s_prev_pad = b;
    return s_want && !s_open;
}

static int pad_key(void)
{
    static const struct { uint32_t bit; int key; } map[] = {
        { XBOX_GAMEPAD_DPAD_UP, KEY_UP }, { XBOX_GAMEPAD_DPAD_DOWN, KEY_DOWN },
        { XBOX_GAMEPAD_DPAD_LEFT, KEY_LEFT }, { XBOX_GAMEPAD_DPAD_RIGHT, KEY_RIGHT },
        { 1u << 16, KEY_OK }, { 1u << 17, KEY_BACK }, { XBOX_GAMEPAD_START, KEY_BACK },
    };
    uint32_t b = pad_bits(), pressed = b & ~s_prev_pad;
    uint64_t now = mono_ms();
    int key = 0;
    s_prev_pad = b;
    for (unsigned i = 0; i < sizeof map / sizeof map[0]; i++) {
        if (pressed & map[i].bit) {
            key = map[i].key;
            s_repeat_bit = map[i].bit;
            s_repeat_at = now + 380;
            return key;
        }
    }
    /* Held directions repeat. */
    if (s_repeat_bit && (b & s_repeat_bit) && s_repeat_bit <= XBOX_GAMEPAD_DPAD_RIGHT &&
        now >= s_repeat_at) {
        s_repeat_at = now + 90;
        for (unsigned i = 0; i < 4; i++)
            if (map[i].bit == s_repeat_bit)
                return map[i].key;
    }
    if (!(b & s_repeat_bit))
        s_repeat_bit = 0;
    return 0;
}

/* ---------- drawing (0xAARRGGBB, software) ---------- */

#define LH 740
#define ROW_Y0 124
#define ROW_H  42

static uint32_t *s_px;

static int s_glyph_off_ready;

static inline void blend(uint32_t *d, uint32_t rgb, int a)
{
    if (a <= 0) return;
    if (a >= 255) { *d = 0xFF000000u | rgb; return; }
    uint32_t c = *d;
    int r = (int)(c >> 16 & 255), g = (int)(c >> 8 & 255), b = (int)(c & 255);
    r += (((int)(rgb >> 16 & 255) - r) * a) / 255;
    g += (((int)(rgb >> 8 & 255) - g) * a) / 255;
    b += (((int)(rgb & 255) - b) * a) / 255;
    *d = 0xFF000000u | (uint32_t)r << 16 | (uint32_t)g << 8 | (uint32_t)b;
}

/* A rectangle in logical units. */
static void fill(float x, float y, float w, float h, uint32_t rgb, int a)
{
    int x0 = (int)lroundf(s_ox + x * s_k), y0 = (int)lroundf(s_oy + y * s_k);
    int x1 = (int)lroundf(s_ox + (x + w) * s_k), y1 = (int)lroundf(s_oy + (y + h) * s_k);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > s_w) x1 = s_w;
    if (y1 > s_h) y1 = s_h;
    for (int yy = y0; yy < y1; yy++)
        for (int xx = x0; xx < x1; xx++)
            blend(&s_px[(size_t)yy * s_w + xx], rgb, a);
}

static float text_width(const char *t, float size)
{
    float w = 0;
    for (; *t; t++) {
        int c = (unsigned char)*t;
        if (c < 32 || c > 126) c = '?';
        w += launcher_font_w[c - 32] * size / LAUNCHER_FONT_H_PX;
    }
    return w;
}

/* Text in logical units; align 0 left, 1 centre, 2 right of x. Each window
 * pixel samples the 32-pixel glyph bilinearly. */
static void text(const char *t, float x, float y, float size, uint32_t rgb,
                 int alpha, int align)
{
    if (align == 1) x -= text_width(t, size) / 2;
    if (align == 2) x -= text_width(t, size);
    float scale = size * s_k / LAUNCHER_FONT_H_PX;   /* window px per glyph px */
    float px0 = s_ox + x * s_k, py0 = s_oy + y * s_k;
    for (; *t; t++) {
        int c = (unsigned char)*t;
        if (c < 32 || c > 126) c = '?';
        int gw = launcher_font_w[c - 32];
        const uint8_t *g = launcher_font_px + launcher_font_off[c - 32];
        int dx0 = (int)floorf(px0), dy0 = (int)floorf(py0);
        int dx1 = (int)ceilf(px0 + gw * scale), dy1 = (int)ceilf(py0 + LAUNCHER_FONT_H_PX * scale);
        for (int yy = dy0; yy < dy1; yy++) {
            if (yy < 0 || yy >= s_h) continue;
            float sy = (yy + 0.5f - py0) / scale - 0.5f;
            int iy = (int)floorf(sy);
            float fy = sy - iy;
            for (int xx = dx0; xx < dx1; xx++) {
                if (xx < 0 || xx >= s_w) continue;
                float sx = (xx + 0.5f - px0) / scale - 0.5f;
                int ix = (int)floorf(sx);
                float fx = sx - ix;
                float v = 0;
                for (int j = 0; j < 2; j++)
                    for (int i = 0; i < 2; i++) {
                        int gx = ix + i, gy = iy + j;
                        if (gx < 0 || gy < 0 || gx >= gw || gy >= LAUNCHER_FONT_H_PX)
                            continue;
                        v += g[gy * gw + gx] * (i ? fx : 1 - fx) * (j ? fy : 1 - fy);
                    }
                blend(&s_px[(size_t)yy * s_w + xx], rgb, (int)(v * alpha / 255));
            }
        }
        px0 += gw * scale;
    }
}

static int row_y(int row)
{
    return ROW_Y0 + row * ROW_H + (row >= ROW_RESUME ? 16 : 0);
}

/* The row under a point in logical units, or -1; *zone: -1 the "<",
 * +1 the ">", 0 elsewhere on the row. */
static int row_hit(float lx, float ly, int *zone)
{
    *zone = 0;
    for (int row = 0; row < NROWS; row++) {
        int y = row_y(row);
        if (ly < y + 2 || ly >= y + ROW_H - 4)
            continue;
        if (row >= ROW_RESUME)
            return lx >= LW / 2 - 150 && lx < LW / 2 + 150 ? row : -1;
        if (lx < 60 || lx >= LW - 60)
            return -1;
        if (lx >= 530 && lx < 610) *zone = -1;
        if (lx >= LW - 130) *zone = 1;
        return row;
    }
    return -1;
}

static void draw(const uint32_t *backdrop)
{
    const uint32_t white = 0xEBEBEB, grey = 0x96969B, red = 0xC82028, bar = 0x781016;

    /* The game, dimmed. */
    for (int i = 0; i < s_w * s_h; i++) {
        uint32_t c = backdrop[i];
        s_px[i] = 0xFF000000u | ((c >> 16 & 255) * 3 / 10) << 16 |
                  ((c >> 8 & 255) * 3 / 10) << 8 | ((c & 255) * 3 / 10);
    }
    fill(0, 0, LW, LH, 0x0E0E12, 200);
    fill(0, 0, LW, 92, 0x1A080A, 235);
    fill(0, 90, LW, 2, red, 255);
    text("NINJA GAIDEN BLACK", LW / 2, 14, 42, white, 255, 1);
    text("Options", LW / 2, 58, 22, grey, 255, 1);

    for (int row = 0; row < NROWS; row++) {
        int y = row_y(row);
        bool sel = row == s_sel;
        if (row >= ROW_RESUME) {
            const char *label = row == ROW_RESUME ? "Resume"
                              : s_quit_armed ? "Press again to quit" : "Quit game";
            float w = 300, x = LW / 2 - w / 2;
            fill(x, y + 2, w, ROW_H - 6, sel ? bar : 0xFFFFFF, sel ? 220 : 18);
            text(label, LW / 2, y + 7, 25, sel ? white : grey, 255, 1);
            continue;
        }
        const char *label;
        char val[96];
        ngb_settings_row_text(&s_cfg, row, &label, val, sizeof val);
        if (sel)
            fill(60, y + 2, LW - 120, ROW_H - 6, bar, 220);
        else if (row % 2)
            fill(60, y + 2, LW - 120, ROW_H - 6, 0xFFFFFF, 18);
        text(label, 84, y + 8, 23, sel ? white : grey, 255, 0);
        text("<", 560, y + 8, 23, sel ? white : grey, 255, 0);
        text(val, 736, y + 8, 23, white, 255, 1);
        text(">", LW - 92, y + 8, 23, sel ? white : grey, 255, 0);
    }
    text("Up/Down or mouse: choose    Left/Right or click: change    A / Enter: select    B / Esc / F1: resume",
         LW / 2, LH - 56, 17, grey, 255, 1);
    text("Changes apply when you resume. Skip intros and the startup screen take effect next start.",
         LW / 2, LH - 32, 15, grey, 255, 1);
}

/* ---------- open, run, close ---------- */

static void apply_and_close(void)
{
    const NgbSettings *o = &s_running;
    const NgbSettings *n = &s_cfg;

    if (n->aspect != o->aspect)
        apply_aspect(&s_cfg, false);
    if (n->width != o->width || n->height != o->height || n->fullscreen != o->fullscreen)
        nv2a_window_request(n->width, n->height, n->fullscreen);
    if (n->vsync != o->vsync)
        nv2a_present_set_vsync(n->vsync);
    if (n->show_fps != o->show_fps)
        nv2a_present_set_show_fps(n->show_fps);
    if (n->scale != o->scale || n->anisotropy != o->anisotropy || n->msaa != o->msaa ||
        n->shadow_scale != o->shadow_scale)
        nv2a_set_quality(n->scale, n->anisotropy, n->shadow_scale, n->msaa);
    if (memcmp(n, o, sizeof *n) && !getenv("NGB_NO_SETTINGS"))
        ngb_settings_save(n);
    s_running = *n;
    s_open = false;
    s_wait_release = true;               /* the pad stays ours until let go */
    apu_host_pause(0);
}

static void activate(int key)
{
    if (key != KEY_OK || s_sel != ROW_QUIT)
        s_quit_armed = false;
    switch (key) {
    case KEY_UP:   s_sel = (s_sel + NROWS - 1) % NROWS; break;
    case KEY_DOWN: s_sel = (s_sel + 1) % NROWS; break;
    case KEY_LEFT:
    case KEY_RIGHT:
        if (s_sel < NGB_SETTINGS_ITEMS)
            ngb_settings_change(&s_cfg, s_sel, key == KEY_LEFT ? -1 : 1);
        break;
    case KEY_OK:
        if (s_sel < NGB_SETTINGS_ITEMS) {
            ngb_settings_change(&s_cfg, s_sel, 1);
        } else if (s_sel == ROW_RESUME) {
            apply_and_close();
        } else if (s_quit_armed) {
            if (memcmp(&s_cfg, &s_running, sizeof s_cfg) && !getenv("NGB_NO_SETTINGS"))
                ngb_settings_save(&s_cfg);
            SDL_Event q = { .type = SDL_QUIT };
            SDL_PushEvent(&q);
        } else {
            s_quit_armed = true;
        }
        break;
    case KEY_BACK:
        apply_and_close();
        break;
    }
}

/* Renderer's thread, every frame while the game is held. */
bool nv2a_overlay_frame(uint32_t *px, int w, int h, const uint32_t *backdrop)
{
    if (!s_open) {
        s_open = true;
        fprintf(stderr, "[MENU] opened\n");
        s_want = 0;
        if (!s_have_running) {
            ngb_settings_load(&s_running);
            s_have_running = true;
        }
        s_cfg = s_running;
        s_sel = ROW_RESUME;
        s_quit_armed = false;
        s_repeat_bit = 0;
        s_prev_pad = pad_bits();         /* what is held already does nothing */
        while (pop_key())
            ;
        usb_gamepad_block = 1;
        apu_host_pause(1);
    }
    s_px = px;
    s_w = w;
    s_h = h;
    s_k = fminf((float)w / LW, (float)h / LH);
    s_ox = (w - LW * s_k) / 2;
    s_oy = (h - LH * s_k) / 2;

    int key;
    while (s_open && (key = pop_key())) {
        if (key & (MOUSE_MOVE | MOUSE_CLICK)) {
            float lx = ((key >> 14 & 0x3FFF) - s_ox) / s_k;
            float ly = ((key & 0x3FFF) - s_oy) / s_k;
            int zone, row = row_hit(lx, ly, &zone);
            if (row < 0)
                continue;
            if (row != s_sel)
                s_quit_armed = false;
            s_sel = row;
            if (key & MOUSE_CLICK)
                activate(zone < 0 ? KEY_LEFT : zone > 0 ? KEY_RIGHT : KEY_OK);
            continue;
        }
        activate(key);
    }
    if (s_open && (key = pad_key()))
        activate(key);
    if (s_open && (key = test_key())) {
        fprintf(stderr, "[MENU] test key %d\n", key);
        activate(key);
        if (!s_open)
            fprintf(stderr, "[MENU] closed\n");
    }

    (void)s_glyph_off_ready;
    draw(backdrop);
    return s_open;
}

/* The renderer cannot run on this system (src/d3d): said in a message box, then the program ends. xemu's
 * GPU model drawing instead (NGB_RENDERER=xemu) is for debugging only -- as a silent fallback it ran the
 * game far slower than the player had any way to know why (#47). */
_Noreturn void ngb_renderer_fatal(const char *detail)
{
    char msg[1024];
    snprintf(msg, sizeof msg,
             "Ninja Gaiden Black needs a GPU and driver with Vulkan 1.3 (dynamic rendering and push "
             "descriptors), and this system does not offer it:\n\n%s\n\n"
             "Please update your graphics driver. GPUs from before about 2016 (for example Nvidia's "
             "GTX 600 and 700 series) are not supported.", detail);
    fprintf(stderr, "[D3D] %s\n", msg);
    fflush(stderr);
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Ninja Gaiden Black", msg, NULL);
    exit(1);
}
