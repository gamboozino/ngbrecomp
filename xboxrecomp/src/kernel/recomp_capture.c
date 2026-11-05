/*
 * Differential capture of one lifted function (RECOMP_CAPTURE).
 *
 *   RECOMP_CAPTURE=0xVA[@ms]   capture the first call to VA made after ms
 *                              milliseconds of run time (default 0)
 *   RECOMP_CAPTURE=0xVA,0xVB,...[@ms]
 *                              the first call to each, one after another,
 *                              each into DIR/<VA in hex>
 *   RECOMP_CAPTURE_SKIP=n      skip the first n qualifying calls (a function
 *                              run per object or per bone: test another one)
 *   RECOMP_CAPTURE_DIR=dir     where the snapshot goes (default ./capture)
 *
 * At that call every lifted function's entry check (RECOMP_ENTRY) hands
 * control here: the guest state is written out (registers, the 64 MB low
 * view and the contiguous window), the function runs once, and the state is
 * written again. tools/recomp/difftest.py replays the original machine code
 * from the "pre" snapshot in an emulator and compares it with "post": a
 * mismatch is a lifting bug, found without knowing what the code does.
 */
#ifdef _WIN32
#include <direct.h>
#define MKDIR(d) _mkdir(d)
#else
#define MKDIR(d) mkdir((d), 0755)
#endif
#include <stdint.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#include "kernel.h"
#include "xbox_memory_layout.h"
#if defined(_MSC_VER)
#  define CAP_TLS __declspec(thread)
#else
#  define CAP_TLS __thread
#endif
#include "recomp_regs.h"   /* the registers: g_eax... */

typedef void (*recomp_func_t)(void);
extern recomp_func_t recomp_lookup(uint32_t va);

uint32_t g_recomp_capture_va;
volatile int g_recomp_capture_busy;   /* interrupts held while set */
static uint64_t s_after_ms, s_start_ms;
#define CAP_MAX 256
static uint32_t s_list[CAP_MAX];
static int s_count, s_left;      /* s_count > 1: a list, each VA once */
static CAP_TLS int t_inside;
static FILE *s_trace;            /* functions entered during the captured call */

static uint64_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/* RECOMP_COVER=from_ms-to_ms[:file]: count every lifted function entered in
 * that window of run time and write "VA count" lines to file (default
 * cover.txt) when it closes -- the candidates for a differential test of
 * whatever was on screen then. */
static uint64_t s_cover_from, s_cover_to;
static const char *s_cover_file;
static uint32_t *s_cover_va, *s_cover_n;
#define COVER_SLOTS (1u << 16)

/* RECOMP_COVER_SNAP=prefix: count all the time instead, and on each SIGUSR2
 * write the counts since the last one to prefix.N and start again -- one
 * snapshot in each state to compare (a menu open and closed, say). */
static const char *s_snap_prefix;
static volatile int s_snap_req;

static void snap_on_signal(int sig) { (void)sig; s_snap_req = 1; }

static void snap_write(void)
{
    static int seq;
    char path[1024];
    FILE *f;
    snprintf(path, sizeof path, "%s.%d", s_snap_prefix, seq++);
    f = fopen(path, "w");
    for (uint32_t i = 0; f && i < COVER_SLOTS; i++)
        if (s_cover_va[i])
            fprintf(f, "%08X %u\n", s_cover_va[i], s_cover_n[i]);
    if (f) fclose(f);
    memset(s_cover_va, 0, COVER_SLOTS * sizeof *s_cover_va);
    memset(s_cover_n, 0, COVER_SLOTS * sizeof *s_cover_n);
    fprintf(stderr, "[COVER] snapshot %s\n", path);
}

static void cover_entry(uint32_t va)
{
    uint64_t t = now_ms() - s_start_ms;
    if (s_snap_prefix && __atomic_exchange_n(&s_snap_req, 0, __ATOMIC_ACQ_REL))
        snap_write();
    if (t < s_cover_from)
        return;
    if (t >= s_cover_to) {
        FILE *f;
        if (!s_cover_va)
            return;
        f = fopen(s_cover_file, "w");
        for (uint32_t i = 0; f && i < COVER_SLOTS; i++)
            if (s_cover_va[i])
                fprintf(f, "%08X %u\n", s_cover_va[i], s_cover_n[i]);
        if (f) fclose(f);
        fprintf(stderr, "[COVER] written to %s\n", s_cover_file);
        s_cover_va = NULL;
        return;
    }
    for (uint32_t h = (va * 2654435761u) >> 16, k = 0; k < COVER_SLOTS; k++, h = (h + 1) & (COVER_SLOTS - 1)) {
        if (s_cover_va[h] == va) { __atomic_add_fetch(&s_cover_n[h], 1, __ATOMIC_RELAXED); return; }
        if (!s_cover_va[h]) { s_cover_va[h] = va; s_cover_n[h] = 1; return; }
    }
}

__attribute__((constructor)) static void capture_init(void)
{
    const char *e = getenv("RECOMP_CAPTURE");
    const char *c = getenv("RECOMP_COVER");
    char *end;
    const char *snap = getenv("RECOMP_COVER_SNAP");
    if (snap && *snap) {
        s_snap_prefix = snap;
        s_cover_file = snap;
        s_cover_to = ~0ull;
        s_cover_va = calloc(COVER_SLOTS, sizeof *s_cover_va);
        s_cover_n = calloc(COVER_SLOTS, sizeof *s_cover_n);
#ifdef SIGUSR2
        signal(SIGUSR2, snap_on_signal);
#endif
        g_recomp_capture_va = 1;
        return;
    }
    if (c && *c) {
        s_cover_from = strtoull(c, &end, 0);
        s_cover_to = *end == '-' ? strtoull(end + 1, &end, 0) : ~0ull;
        s_cover_file = *end == ':' ? end + 1 : "cover.txt";
        s_cover_va = calloc(COVER_SLOTS, sizeof *s_cover_va);
        s_cover_n = calloc(COVER_SLOTS, sizeof *s_cover_n);
        s_start_ms = now_ms();
        g_recomp_capture_va = 1;     /* route entries here */
        return;
    }
    if (!e || !*e)
        return;
    g_recomp_capture_va = (uint32_t)strtoul(e, &end, 0);
    s_list[s_count++] = g_recomp_capture_va;
    while (*end == ',' && s_count < CAP_MAX)
        s_list[s_count++] = (uint32_t)strtoul(end + 1, &end, 0);
    s_left = s_count;
    s_after_ms = *end == '@' ? strtoull(end + 1, NULL, 0) : 0;
    s_start_ms = now_ms();
}

static void write_file(const char *dir, const char *name, const void *p, size_t n)
{
    char path[1024];
    FILE *f;
    snprintf(path, sizeof path, "%s/%s", dir, name);
    f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "[CAPTURE] cannot write %s\n", path); return; }
    fwrite(p, 1, n, f);
    fclose(f);
}

static void write_state(const char *dir, const char *tag)
{
    const uint8_t *mem = (const uint8_t *)(uintptr_t)xbox_GetMemoryOffset();
    char name[64], path[1024];
    FILE *f;

    snprintf(name, sizeof name, "%s_low.bin", tag);
    write_file(dir, name, mem, 64u << 20);
    snprintf(name, sizeof name, "%s_contig.bin", tag);
    write_file(dir, name, mem + XBOX_CONTIG_BASE, XBOX_CONTIG_SIZE);

    snprintf(path, sizeof path, "%s/%s_regs.txt", dir, tag);
    f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "eax %u\necx %u\nedx %u\nebx %u\nesp %u\nesi %u\nedi %u\n",
            g_eax, g_ecx, g_edx, g_ebx, g_esp, g_esi, g_edi);
    fprintf(f, "ebp %u\nseh_ebp %u\nfs_base %u\ndf %d\n", g_ebp, g_seh_ebp,
            g_fs_base, g_df);
    fprintf(f, "fp_top %d\nfp_cw %u\n", g_fp_top, (unsigned)g_fp_control_word);
    for (int i = 0; i < 8; i++)
        fprintf(f, "fp%d %.17g\n", i, g_fp_stack[i]);
    {
        RecompXmm *x[8] = { &g_xmm0, &g_xmm1, &g_xmm2, &g_xmm3,
                            &g_xmm4, &g_xmm5, &g_xmm6, &g_xmm7 };
        for (int i = 0; i < 8; i++)
            fprintf(f, "xmm%d %u %u %u %u\n", i, x[i]->u[0], x[i]->u[1],
                    x[i]->u[2], x[i]->u[3]);
    }
    fclose(f);
}

int recomp_capture_entry(uint32_t va)
{
    const char *dir = getenv("RECOMP_CAPTURE_DIR");
    char sub[1024];
    recomp_func_t fn;
    int idx = -1;

    if (s_cover_file) {
        cover_entry(va);
        return 0;
    }
    if (t_inside) {
        if (s_trace)
            fprintf(s_trace, "%08X\n", va);
        return 0;
    }
    if (s_count > 1) {
        for (int i = 0; i < s_count; i++)
            if (s_list[i] == va) { idx = i; break; }
        if (idx < 0)
            return 0;
    } else if (va != g_recomp_capture_va)
        return 0;
    if (s_after_ms && now_ms() - s_start_ms < s_after_ms)
        return 0;
    {
        static long skip = -1;
        if (skip < 0) {
            const char *e = getenv("RECOMP_CAPTURE_SKIP");
            skip = e ? atol(e) : 0;
        }
        if (s_count <= 1 && skip > 0) {
            skip--;
            return 0;
        }
        if (s_count > 1 && skip > 0) {
            static long left[CAP_MAX];
            static int init;
            if (!init) {
                init = 1;
                for (int i = 0; i < CAP_MAX; i++) left[i] = skip;
            }
            if (left[idx] > 0) {
                left[idx]--;
                return 0;
            }
        }
    }
    fn = recomp_lookup(va);
    if (!fn)
        return 0;
    /* One shot: keep the hook armed only while the call runs (tracing). */
    if (!dir || !*dir)
        dir = "capture";
    MKDIR(dir);
    if (s_count > 1) {
        snprintf(sub, sizeof sub, "%s/%08X", dir, va);
        MKDIR(sub);
        dir = sub;
        s_list[idx] = 0;            /* each once */
    }
    fprintf(stderr, "[CAPTURE] 0x%08X: pre-state\n", va);
    write_state(dir, "pre");
    {
        char path[1024];
        snprintf(path, sizeof path, "%s/calls.txt", dir);
        s_trace = fopen(path, "w");
    }
    t_inside = 1;
    g_recomp_capture_busy = 1;
    fn();
    g_recomp_capture_busy = 0;
    t_inside = 0;
    if (s_trace) { fclose(s_trace); s_trace = NULL; }
    if (s_count <= 1 || --s_left == 0)
        g_recomp_capture_va = 0;
    write_state(dir, "post");
    fprintf(stderr, "[CAPTURE] 0x%08X: post-state written to %s\n", va, dir);
    return 1;                        /* the caller's call is done */
}
