/*
 * The Android entry: SDL's activity calls SDL_main in libmain.so. The game's own main() takes its folders from
 * the environment as on Linux, so they are set here first, all under the app's folder on shared storage
 * (Android/data/io.github.gamboozino.ngbrecomp/files, which adb can reach): the saves there, with game/ the
 * installed game files in it as on the desktop (the first start installs them: src/android_install.c),
 * config/ the settings, cache/ the shaders. The log goes to log.txt there and to logcat. An env.txt
 * there (KEY=value lines) sets more, as the environment does on the desktop (NV2A_FPS_LOG=1, NGB_CHEATS=...),
 * and a settings.ini put there replaces the game's settings at the next start (then it is taken away).
 */
#include <android/log.h>
#include <dirent.h>
#include <dlfcn.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <SDL.h>
#undef main   /* (SDL_main.h renames main to SDL_main: this one is the game's) */

int main(int argc, char **argv);   /* src/main.c */

static int s_log_pipe[2];

/* The clocks. The game's threads take turns on one virtual CPU, each waiting on the others, so no core looks
 * busy and the CPU governor keeps them slow: a 60 Hz frame took 50 ms on a Pixel 9a, and 18 ms with the clocks
 * held up. Android's performance hints (13 and later) let the app say how long each frame took against the
 * 16.7 ms it should take, and the system raises the clocks of the threads named. Each frame the game ends
 * (ngb_frame_tick) reports the time since the last; the threads are the process's, renamed every few seconds
 * as they come and go. NGB_PERF_HINT=0 leaves it out (comparing). */
typedef struct APerformanceHintManager APerformanceHintManager;
typedef struct APerformanceHintSession APerformanceHintSession;
static APerformanceHintSession *s_hint;
static int (*s_hint_report)(APerformanceHintSession *, int64_t);
static int (*s_hint_threads)(APerformanceHintSession *, const pid_t *, size_t);

static size_t own_threads(pid_t *tids, size_t max)
{
    size_t n = 0;
    DIR *d = opendir("/proc/self/task");
    struct dirent *e;
    while (d && n < max && (e = readdir(d)))
        if (e->d_name[0] != '.')
            tids[n++] = (pid_t)atoi(e->d_name);
    if (d)
        closedir(d);
    return n;
}

static int64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
}

void ngb_android_frame(void)
{
    static int tried;
    static int64_t last, renamed;
    int64_t t = now_ns();
    if (!tried) {
        tried = 1;
        const char *off = getenv("NGB_PERF_HINT");
        void *lib = dlopen("libandroid.so", RTLD_NOW);
        APerformanceHintManager *(*get)(void) = lib ? dlsym(lib, "APerformanceHint_getManager") : NULL;
        APerformanceHintSession *(*create)(APerformanceHintManager *, const int32_t *, size_t, int64_t) =
            lib ? dlsym(lib, "APerformanceHint_createSession") : NULL;
        s_hint_report = lib ? dlsym(lib, "APerformanceHint_reportActualWorkDuration") : NULL;
        s_hint_threads = lib ? dlsym(lib, "APerformanceHint_setThreads") : NULL;
        APerformanceHintManager *m = get && !(off && *off == '0') ? get() : NULL;
        pid_t tids[256];
        size_t n = own_threads(tids, 256);
        s_hint = m && create && s_hint_report ? create(m, (const int32_t *)tids, n, 16666667) : NULL;
        fprintf(stderr, "[ANDROID] performance hints: %s (%zu threads)\n", s_hint ? "on" : "not available", n);
        renamed = t;
    }
    if (s_hint && last)
        s_hint_report(s_hint, t - last);
    if (s_hint && s_hint_threads && t - renamed > 5000000000LL) {   /* (setThreads: Android 14) */
        pid_t tids[256];
        size_t n = own_threads(tids, 256);
        s_hint_threads(s_hint, tids, n);
        renamed = t;
    }
    last = t;
}

/* Copies what the game writes to stderr into logcat, line by line. */
static void *log_pump(void *arg)
{
    FILE *in = fdopen(s_log_pipe[0], "r");
    FILE *file = arg;
    char line[1024];
    while (in && fgets(line, sizeof line, in)) {
        __android_log_write(ANDROID_LOG_INFO, "NGB", line);
        if (file) {
            fputs(line, file);
            fflush(file);
        }
    }
    return NULL;
}

static void read_env(const char *base)
{
    char path[1024], line[1024];
    snprintf(path, sizeof path, "%s/env.txt", base);
    FILE *f = fopen(path, "r");
    if (!f)
        return;
    while (fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = 0;
        char *eq = strchr(line, '=');
        if (line[0] == '#' || !eq || eq == line)
            continue;
        *eq = 0;
        setenv(line, eq + 1, 1);
    }
    fclose(f);
}

static void take_settings(const char *base)
{
    char from[1024], dir[1024], to[1100];
    snprintf(from, sizeof from, "%s/settings.ini", base);
    snprintf(dir, sizeof dir, "%s/config/NinjaGaidenBlack", base);
    snprintf(to, sizeof to, "%s/settings.ini", dir);
    FILE *in = fopen(from, "rb");
    if (!in)
        return;
    mkdir(dir, 0700);
    FILE *out = fopen(to, "wb");
    char buf[4096];
    size_t n;
    while (out && (n = fread(buf, 1, sizeof buf, in)) > 0)
        fwrite(buf, 1, n, out);
    fclose(in);
    if (out) {
        fclose(out);
        remove(from);
    }
}

static void set_dir(const char *var, const char *base, const char *sub)
{
    char path[1024];
    snprintf(path, sizeof path, "%s/%s", base, sub);
    mkdir(path, 0700);
    setenv(var, path, 0);
}

/* The renderer's shader seed (share/NinjaGaidenBlack/d3d-seed.bin on the desktop, src/d3d/vk/backend.cpp): in the
 * APK as libd3dseed.so, a name Android unpacks to the native libraries' folder with the rest (the APK had none
 * before, and a phone made every shader as it was first drawn: thousands of draws left out in the attract demo) */
static void seed_path(void)
{
    Dl_info info;
    char path[1024];
    if (getenv("NGB_D3D_SEED") || !dladdr((void *)seed_path, &info) || !info.dli_fname)
        return;
    snprintf(path, sizeof path, "%s", info.dli_fname);
    char *slash = strrchr(path, '/');
    if (!slash)
        return;
    snprintf(slash + 1, sizeof path - (size_t)(slash + 1 - path), "libd3dseed.so");
    if (access(path, R_OK) == 0)
        setenv("NGB_D3D_SEED", path, 0);
}

int SDL_main(int argc, char **argv)
{
    (void)argc;
    const char *base = SDL_AndroidGetExternalStoragePath();
    if (!base)
        base = SDL_AndroidGetInternalStoragePath();
    if (base) {
        read_env(base);
        setenv("HOME", base, 0);
        setenv("NGB_SAVE_DIR", base, 0);
        set_dir("XDG_CONFIG_HOME", base, "config");
        set_dir("XDG_CACHE_HOME", base, "cache");
        set_dir("XDG_DATA_HOME", base, "data");
        take_settings(base);
    }
    seed_path();
    char log_path[1024];
    snprintf(log_path, sizeof log_path, "%s/log.txt", base ? base : ".");
    FILE *log = fopen(log_path, "w");
    if (pipe(s_log_pipe) == 0) {
        setvbuf(stdout, NULL, _IOLBF, 0);
        setvbuf(stderr, NULL, _IONBF, 0);
        dup2(s_log_pipe[1], 1);
        dup2(s_log_pipe[1], 2);
        pthread_t t;
        pthread_create(&t, NULL, log_pump, log);
        pthread_detach(t);
    }
    fprintf(stderr, "[ANDROID] folders under %s%s\n", base ? base : "(none)", getenv("NV2A_FPS_LOG") ? " (env.txt read)" : "");
    /* Landscape either way up: SDL sets the activity's orientation from this when it makes the window (the
     * manifest's alone is overridden) */
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");   /* touches are the touch controls' (src/touch_pad.c), not a mouse */
    char *args[] = { argv && argv[0] ? argv[0] : "NinjaGaidenBlack", NULL };
    /* Ended here, not returned: SDL keeps the process after SDL_main, and a second start would run the game
     * again over the first one's state */
    int rc = main(1, args);
    fflush(NULL);
    _exit(rc);
}
