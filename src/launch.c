/*
 * launch.c -- one recompiled program handing over to another, as the Xbox reboots into the image a
 * title launches (#18).
 *
 * Ninja Gaiden Black runs the NES Ninja Gaiden (the cabinet in Han's Bar, the main menu's ARCADE NINJA
 * GAIDEN) by XLaunchNewImage("D:\NinjaTrilogy.xbe"), and that program returns the same way to
 * "D:\default.xbe". XAPI fills in the launch data page (the path, the launching title's id and 3072
 * bytes for the next image) and calls HalReturnToFirmware; the runtime hands the page to the hook
 * installed here (xbox_LaunchImageHook), which saves it, points the next program at it
 * (RECOMP_LAUNCH_PAGE: the runtime puts it back where XGetLaunchInfo looks) and starts that program in
 * place of this one: exec on Linux and macOS, a new process and this one ended on Windows, and on Android
 * the app's process started again into the image's library (src/android_main.c, NgbActivity.relaunch). Each
 * image is its own recompiled program, next to this one: NinjaGaidenBlack for default.xbe, NinjaTrilogy for
 * NinjaTrilogy.xbe, NinjaTomoe for NinjaTomoe.xbe (the Japanese arcade, launched instead when the game's
 * text is Japanese); on Android libmain.so, libtrilogy.so and libtomoe.so.
 */
#include "ngb_launch.h"
#include "port.h"
#include "nv2a_window.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#if defined(_WIN32)
#include <windows.h>
#else
#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#endif

static char s_game[4096], s_save[4096];
static int s_w, s_h, s_fs;
static unsigned char s_page[0x1000];
static int s_page_set = -1;

void ngb_default_save_dir(char *out, size_t n)
{
    const char *env = getenv("NGB_SAVE_DIR");
    if (env && *env) {
        snprintf(out, n, "%s", env);
        return;
    }
#if defined(_WIN32)
    const char *base = getenv("LOCALAPPDATA");
    snprintf(out, n, "%s/NinjaGaidenBlack", base && *base ? base : ".");
#else
    const char *xdg = getenv("XDG_DATA_HOME"), *home = getenv("HOME");
    if (xdg && *xdg)
        snprintf(out, n, "%s/NinjaGaidenBlack", xdg);
    else
#ifdef __APPLE__
        ngb_macos_dir(out, n, "saves", 1);
#else
        snprintf(out, n, "%s/.local/share/NinjaGaidenBlack", home && *home ? home : ".");
#endif
#endif
}

void ngb_launch_log_to_file(const char *save_dir, const char *name)
{
    char log[4200];
    snprintf(log, sizeof log, "%s/%s", save_dir, name);
#if defined(_WIN32)
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        return;
    }
    ngb_mkdir(save_dir);
    FILE *f = fopen(log, "w");
    if (f) {
        fclose(f);
        freopen(log, "a", stdout);
        freopen(log, "a", stderr);
    }
#elif defined(__ANDROID__)
    (void)log;   /* (src/android_main.c has it in its log file and logcat already) */
#else
    if (getenv("NGB_TEST_HIDDEN") || isatty(STDOUT_FILENO))
        return;
    struct stat st;
    if (fstat(STDOUT_FILENO, &st) == 0 && S_ISREG(st.st_mode))   /* already going to a file */
        return;
    ngb_mkdir(save_dir);
    if (freopen(log, "w", stdout)) {
        dup2(fileno(stdout), STDERR_FILENO);
        setvbuf(stdout, NULL, _IOLBF, 0);
    }
#endif
}

static void window_from_env(void)
{
    const char *e = getenv("NGB_LAUNCH_WINDOW");
    if (!(e && sscanf(e, "%d %d %d", &s_w, &s_h, &s_fs) == 3 && s_w > 0 && s_h > 0)) {
        s_w = 1280;
        s_h = 960;
        s_fs = 0;
    }
}
int ngb_launch_window_w(void) { if (!s_w) window_from_env(); return s_w; }
int ngb_launch_window_h(void) { if (!s_w) window_from_env(); return s_h; }
int ngb_launch_fullscreen(void) { if (!s_w) window_from_env(); return s_fs; }

const unsigned char *ngb_launch_page(void)
{
    if (s_page_set < 0) {
        const char *p = getenv("RECOMP_LAUNCH_PAGE");
        FILE *f = p && *p ? fopen(p, "rb") : NULL;
        s_page_set = f && fread(s_page, 1, sizeof s_page, f) == sizeof s_page;
        if (f)
            fclose(f);
    }
    return s_page_set ? s_page : NULL;
}

static void set_env(const char *name, const char *value)
{
#if defined(_WIN32)
    _putenv_s(name, value);
#else
    setenv(name, value, 1);
#endif
}

#if !defined(_WIN32)
static unsigned char s_inherited[1024];   /* the descriptors this program was started with */

static void note_inherited(void)
{
    DIR *d = opendir("/proc/self/fd");
    struct dirent *e;
    if (!d)
        d = opendir("/dev/fd");
    if (!d)
        return;
    while ((e = readdir(d))) {
        int fd = atoi(e->d_name);
        if (fd > 2 && fd < 1024 && fd != dirfd(d))
            s_inherited[fd] = 1;
    }
    closedir(d);
}

/* What this program opened (the window, the GPU, the sound device) is not the next one's. */
static void close_on_exec_all(void)
{
    DIR *d = opendir("/proc/self/fd");
    struct dirent *e;
    if (!d)
        d = opendir("/dev/fd");
    if (!d)
        return;
    while ((e = readdir(d))) {
        int fd = atoi(e->d_name);
        if (fd > 2 && fd != dirfd(d) && !(fd < 1024 && s_inherited[fd]))
            fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
    }
    closedir(d);
}

/* The virtual CPU's preemption signal, pending at the exec, would end the new image (as for Return to
 * Main Menu, src/ingame_menu.c). */
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

/* The program prog (NinjaGaidenBlack, NinjaTrilogy, NinjaTomoe) beside this one into out: on Android its library's
 * short name; 0 if it is not there. */
static int program_path(const char *prog, char *out, size_t n)
{
#ifdef __ANDROID__
    {   /* the library, in the app beside this one */
        const char *lib = !strcmp(prog, "NinjaGaidenBlack") ? "main" : !strcmp(prog, "NinjaTrilogy") ? "trilogy" : "tomoe";
        Dl_info info;
        char path[4200];
        if (!dladdr((void *)program_path, &info) || !info.dli_fname)
            return 0;
        snprintf(path, sizeof path, "%s", info.dli_fname);
        char *slash = strrchr(path, '/');
        if (!slash)
            return 0;
        snprintf(slash + 1, sizeof path - (size_t)(slash + 1 - path), "lib%s.so", lib);
        if (access(path, R_OK) != 0)
            return 0;
        snprintf(out, n, "%s", lib);
        return 1;
    }
#endif
    long len = ngb_exe_path(out, n);
    if (len <= 0)
        return 0;
    out[len] = 0;
    char *slash = strrchr(out, '/');
#if defined(_WIN32)
    char *bs = strrchr(out, '\\');
    if (!slash || (bs && bs > slash))
        slash = bs;
#endif
    if (!slash)
        return 0;
#if defined(_WIN32)
    snprintf(slash + 1, n - (size_t)(slash + 1 - out), "%s.exe", prog);
#else
    snprintf(slash + 1, n - (size_t)(slash + 1 - out), "%s", prog);
    if (access(out, X_OK) != 0)
        return 0;
#endif
    return 1;
}

/* The program for a guest image path ("D:\NinjaTrilogy.xbe") into out; 0 if there is none. *arcade: one of the
 * arcade's. */
static int s_is_trilogy;

static int program_for(const char *path, char *out, size_t n, int *arcade)
{
    /* XAPI writes the device and the file joined by ';': \Device\CdRom0;NinjaTrilogy.xbe */
    const char *name = path;
    for (const char *c = path; *c; c++)
        if (*c == '\\' || *c == ';')
            name = c + 1;
    /* NinjaTrilogy names no image when it hands back (XLaunchNewImage(NULL)): the disc's title. In
     * the game itself an empty path is a reboot to the dashboard, an exit. */
    if (!*name && s_is_trilogy)
        name = "default.xbe";
    *arcade = 1;
    if (!strcasecmp(name, "default.xbe")) {
        *arcade = 0;
        return program_path("NinjaGaidenBlack", out, n);
    }
    if (!strcasecmp(name, "NinjaTomoe.xbe") &&   /* the Japanese build: the game's text set to Japanese */
        program_path("NinjaTomoe", out, n))
        return 1;
    if (!strcasecmp(name, "NinjaTrilogy.xbe") || !strcasecmp(name, "NinjaTomoe.xbe"))   /* (else the English one) */
        return program_path("NinjaTrilogy", out, n);
    return 0;
}

static void launch_image(const char *path, const void *page, size_t len);

/* Back to Ninja Gaiden Black as if the arcade had been played and left: the page, its path the game's, handed to it
 * with a notice it shows (NGB_NOTICE, src/fps_text.c) -- the game started again where it launched the arcade from,
 * which an arcade that cannot run on this machine, or is not there, leaves it no other way to be (by now its threads
 * are ended). Returns only when that could not be started either. */
static void hand_back(const void *page, size_t len, const char *notice)
{
    static unsigned char back[0x1000];
    static const char game[] = "\\Device\\CdRom0;default.xbe";
    if (len > sizeof back)
        len = sizeof back;
    memset(back, 0, sizeof back);
    if (page)
        memcpy(back, page, len);
    else
        len = sizeof back;
    memset(back + 8, 0, 520);   /* (the path: after the type and the launching title's id) */
    memcpy(back + 8, game, sizeof game);
    fprintf(stderr, "[LAUNCH] back to the game: %s\n", notice);
    set_env("NGB_NOTICE", notice);
    launch_image(game, back, len);
}

#ifdef NGB_NV2A_VK
const char *nv2a_vk_missing_draw_features(void);   /* xboxrecomp nv2a_vk instance.c */
#endif

static void launch_image(const char *path, const void *page, size_t len)
{
    char prog[4200], file[4200], win[64];
    int arcade = 0;
    if (!program_for(path, prog, sizeof prog, &arcade)) {
        fprintf(stderr, "[LAUNCH] no program for %s\n", path);
        if (arcade && !s_is_trilogy)
            hand_back(page, len, "The arcade Ninja Gaiden is not installed");
        return;
    }
#ifdef NGB_NV2A_VK
    if (arcade && !s_is_trilogy) {   /* the arcade draws with xemu's renderer, which needs what this GPU may lack */
        const char *missing = nv2a_vk_missing_draw_features(), *test = getenv("NGB_TEST_ARCADE_NO_GPU");
        if (test && !strcmp(test, "1"))   /* (testing: =2 lets the arcade find it out, src/trilogy/main.c) */
            missing = "geometryShader (NGB_TEST_ARCADE_NO_GPU)";
        if (missing && *missing) {
            fprintf(stderr, "[LAUNCH] the arcade's renderer needs what this GPU lacks: %s\n", missing);
            hand_back(page, len, "The arcade cannot run on this GPU");
            return;
        }
    }
#endif
    snprintf(file, sizeof file, "%s/launch-page.bin", s_save);
    FILE *f = fopen(file, "wb");
    if (!f || fwrite(page, 1, len, f) != len) {
        fprintf(stderr, "[LAUNCH] cannot write %s\n", file);
        if (f)
            fclose(f);
        return;
    }
    fclose(f);
    /* The window as the player left it (F11, Alt+Enter, the settings), not as it started. */
    {
        int w = s_w, h = s_h, fs = nv2a_window_mode(&w, &h);
        if (fs >= 0) {
            s_fs = fs;
            if (w > 0 && h > 0) {
                s_w = w;
                s_h = h;
            }
        }
    }
    snprintf(win, sizeof win, "%d %d %d", s_w, s_h, s_fs);
    set_env("RECOMP_LAUNCH_PAGE", file);
    set_env("NGB_GAME_DIR", s_game);
    set_env("NGB_SAVE_DIR", s_save);
    set_env("NGB_LAUNCH_WINDOW", win);
    fprintf(stderr, "[LAUNCH] %s: starting %s (window %s)\n", path, prog, win);
    fflush(stderr);
#if defined(__ANDROID__)
    {
        /* What the next program is handed (the rest it reads again, env.txt among it): these, set above or by the
         * game for its arcade (its Audio Output, the frame counter, the render scale, the shaders' folder) -- as an
         * exec passes them on */
        static const char *const pass[] = { "RECOMP_LAUNCH_PAGE", "NGB_GAME_DIR", "NGB_SAVE_DIR", "NGB_LAUNCH_WINDOW",
                                            "NGB_NOTICE", "NGB_AUDIO_OUTPUT", "NV2A_SHOW_FPS", "NV2A_SCALE",
                                            "NV2A_SHADER_CACHE", NULL };
        extern int ngb_android_relaunch(const char *lib, const char *const *names);   /* src/android_main.c */
        if (ngb_android_relaunch(prog, pass))
            for (int i = 0; i < 100; i++)   /* (ended from outside, at once) */
                usleep(100000);
        fprintf(stderr, "[LAUNCH] could not start lib%s.so\n", prog);
    }
#elif !defined(_WIN32)
    close_on_exec_all();
    quiet_signals();
    char *args[2] = { prog, NULL };
    execv(prog, args);
    fprintf(stderr, "[LAUNCH] could not start %s\n", prog);
#else
    {
        wchar_t wprog[MAX_PATH];
        static wchar_t line[MAX_PATH + 3];
        STARTUPINFOW si;
        PROCESS_INFORMATION pi;
        memset(&si, 0, sizeof si);
        si.cb = sizeof si;
        if (MultiByteToWideChar(CP_UTF8, 0, prog, -1, wprog, MAX_PATH)) {
            swprintf(line, MAX_PATH + 3, L"\"%ls\"", wprog);
            if (CreateProcessW(wprog, line, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
                TerminateProcess(GetCurrentProcess(), 0);
            }
        }
        fprintf(stderr, "[LAUNCH] could not start %s (error %lu)\n", prog, GetLastError());
    }
#endif
}

/* The arcade's renderer could not start (xboxrecomp's nv2a_renderer_fatal, src/trilogy/main.c): back to the game,
 * with the page this program was launched with. Returns only when that could not be started. */
void ngb_launch_hand_back(const char *notice)
{
    hand_back(ngb_launch_page(), 0x1000, notice);
}

void ngb_launch_init(const char *game_dir, const char *save_dir, int width, int height, int fullscreen)
{
    extern void (*xbox_LaunchImageHook)(const char *path, const void *page, size_t len);
    snprintf(s_game, sizeof s_game, "%s", game_dir);
    snprintf(s_save, sizeof s_save, "%s", save_dir);
    s_w = width;
    s_h = height;
    s_fs = fullscreen;
#if !defined(_WIN32)
    note_inherited();
#endif
    xbox_LaunchImageHook = launch_image;
    ngb_launch_page();   /* (kept now: the runtime takes the file away once it has read it) */
#ifdef TRILOGY_XBE   /* (the arcade's programs: CMakeLists.txt) */
    s_is_trilogy = 1;
#endif
}
