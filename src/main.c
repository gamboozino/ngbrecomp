/**
 * Ninja Gaiden Black - static recompilation host entry point.
 *
 * Builds natively for Linux (x86-64) and Windows. Boots the recompiled XBE:
 * load default.xbe, map the Xbox address space, bring up the kernel HLE and
 * thunk table, then call the lifted entry point.
 *
 * XBE details (tools.xbe_parser):
 *   Title:       NINJA GAIDEN Black (Kunai.exe, release_ltcg)
 *   Title ID:    0x5443000D, region NA, XDK 5849
 *   Base addr:   0x00010000, image size 0x00DB6BC0
 *   Entry point: 0x0032B385
 *   Code:        .text 0x369480 bytes + D3D/DSOUND/XPP/XONLINE/XNET/DOLBY...
 *   Libraries:   D3D8LTCG, DSOUND, XAPILIB, XONLINES, LIBCMT/LIBCPMT, XGRAPHCL
 *   Kernel:      145 imports
 *
 * Game data: the extracted disc goes in a directory named "game" next to the
 * executable, or wherever NGB_GAME_DIR (or the first argument) points.
 */

#if defined(_WIN32)
#include <windows.h>
#include <dbghelp.h>
#include <mmsystem.h>
#include <unistd.h>   /* (mingw's: usleep) */
#include <pthread.h>
#include "platform/vcpu.h"
#else
#define _GNU_SOURCE
#include <dlfcn.h>
#include <execinfo.h>
#include <limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>
#include "platform/win32_compat.h"
#include "platform/vcpu.h"
#endif
#include <stdio.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <xbox/xboxrecomp.h>
#include "recomp_icall_feedback.h"
#include "usb/ohci.h"
#include "usb/usb_gamepad.h"
extern void xbox_InputInit(void);
#if defined(NGB_NV2A_VK)
#include "launcher.h"

/* Skip intro movie: see src/movie_skip.c. The kernel's open hook tells it
 * which movie the title is about to play. */
extern int ngb_skip_intro;
extern void ngb_movie_read(const char *host_path);
extern void (*xbox_FileReadHook)(const char *host_path, uint64_t offset, size_t len);

/* Skipping the boot "Warnings" screen (always; NGB_KEEP_WARNINGS=1 shows it). The screen reads
 * its assets from demo.afs (the last of them at offset 0x151000) and then
 * nothing until it ends; the title it leads to starts with more reads. So
 * after that read, press Start -- what a player does to skip it -- and
 * again only while nothing more has been read, so no press can reach the
 * title screen or a menu. It keeps trying for 15 s: on a slow first start
 * (pipelines being built) the screen takes Start only seconds later. The
 * music and sound streams, and demo.afs (read on behind the screen), do
 * not count as the title's reads. */
#if defined(NGB_NV2A_VK)
void nv2a_present_set_blank(_Bool blank);   /* nv2a_window.h */
#endif
static volatile int s_warning_reads;    /* reads seen since the trigger */
static volatile int s_warning_armed;

static void *warning_skip_thread(void *arg)
{
    (void)arg;
    for (int tries = 0; tries < 37; tries++) {
        usleep(tries ? 400000 : 500000);
        if (__atomic_load_n(&s_warning_reads, __ATOMIC_ACQUIRE))
            break;                      /* moved on: done */
        usb_gamepad_inject |= 0x10;     /* Start */
        usleep(150000);
        usb_gamepad_inject &= ~0x10;
    }
#if defined(NGB_NV2A_VK)
    usleep(300000);                     /* the screen's fade out */
    nv2a_present_set_blank(false);
#endif
    return NULL;
}

#if defined(NGB_NV2A_VK)
/* The screen is drawn before it reads anything of its own, and before it
 * there is only black: with Skip intro the window stays black from the
 * start until the skip is done -- or, should it never start, 15 s. */
static void *warning_unblank_thread(void *arg)
{
    (void)arg;
    usleep(15000000);
    if (!__atomic_load_n(&s_warning_armed, __ATOMIC_ACQUIRE))
        nv2a_present_set_blank(false);
    return NULL;
}
#endif

static void warning_read(const char *host_path, uint64_t off)
{
    const char *base = host_path;   /* the file's name: after the last / or (Windows) \ */
    for (const char *c = host_path; *c; c++)
        if (*c == '/' || *c == '\\')
            base = c + 1;
    if (__atomic_load_n(&s_warning_armed, __ATOMIC_ACQUIRE)) {
        /* (demo.afs goes on being read behind the screen, the attract
         * demo's data streamed in 64 KB pieces: not the title) */
        if (strcmp(base, "bgm.afs") && strcmp(base, "sse.afs") && strcmp(base, "demo.afs"))
            __atomic_store_n(&s_warning_reads, 1, __ATOMIC_RELEASE);
        return;
    }
    if (!strcmp(base, "demo.afs") && off == 0x151000) {
        static int once;

        pthread_t th;
        if (once++)
            return;
        __atomic_store_n(&s_warning_armed, 1, __ATOMIC_RELEASE);
#if defined(NGB_NV2A_VK)
        nv2a_present_set_blank(true);   /* skipped unseen */
#endif
        if (pthread_create(&th, NULL, warning_skip_thread, NULL) == 0)
            pthread_detach(th);
    }
}

static void movie_read_hook(const char *host_path, uint64_t off, size_t len)
{
    (void)len;
    if (host_path) {
        ngb_movie_read(host_path);
        warning_read(host_path, off);
    }
}
#endif
#include "install.h"
#include "port.h"
#if defined(NGB_NV2A_VK)
#include "nv2a_vk_runtime.h"
#include "nv2a_vk.h"
#include "nv2a_window.h"
#endif
#include <signal.h>

/* ── Global register state (defined in xbox_memory_layout.c) ── */

/* RECOMP_TLS is not optional: the runtime defines these thread-local, and a
 * plain extern would resolve to the TLS template rather than this thread's
 * copy, so the guest would start with every register at zero. */
extern RECOMP_TLS uint32_t g_eax, g_ecx, g_edx, g_esp;
extern RECOMP_TLS uint32_t g_ebx, g_esi, g_edi;
extern ptrdiff_t g_xbox_mem_offset;

#define NGB_ENTRY_POINT 0x0032B385u

/* Renderer choice (choose_renderer): the GPU model, and whether it has a window. */
static bool s_use_gpu, s_headless;
static bool s_widescreen;
static char s_game_dir[4096];
static int s_show_settings;
#define NGB_D3D_DEVICE_PTR 0x003D83C8u   /* D3D8LTCG global device pointer */

extern void xbe_entry_point(void);
extern int recomp_dispatch_init(void);   /* recomp_dispatch.c */

/* ── Crash reporting ───────────────────────────────────────── */

/* Name the host function a fault happened in. Every lifted function is a real
 * symbol (sub_XXXXXXXX), so this is the guest function as well. On Linux the
 * executable is linked with -rdynamic so dladdr can see them; the module
 * offset is printed too, for addr2line when it cannot. */
static void print_host_symbol(void *rip)
{
#if defined(_WIN32)
    char buf[sizeof(SYMBOL_INFO) + 256];
    SYMBOL_INFO *sym = (SYMBOL_INFO *)buf;
    DWORD64 disp = 0;

    memset(buf, 0, sizeof(buf));
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym->MaxNameLen = 255;
    if (SymFromAddr(GetCurrentProcess(), (DWORD64)(uintptr_t)rip, &disp, sym))
        fprintf(stderr, "  in %s+0x%llX\n", sym->Name, (unsigned long long)disp);
    {   /* and which module: a fault in a driver or an overlay's DLL names it here (#39) */
        HMODULE mod = NULL;
        char path[MAX_PATH];
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               (LPCSTR)rip, &mod) && GetModuleFileNameA(mod, path, sizeof path))
            fprintf(stderr, "  in module %s+0x%llX\n", path,
                    (unsigned long long)((uintptr_t)rip - (uintptr_t)mod));
    }
#else
    Dl_info di;

    if (dladdr(rip, &di) && di.dli_fname) {
        fprintf(stderr, "  in %s+0x%lX (%s+0x%lX)\n",
                di.dli_sname ? di.dli_sname : "?",
                di.dli_saddr ? (unsigned long)((char *)rip - (char *)di.dli_saddr) : 0ul,
                di.dli_fname,
                (unsigned long)((char *)rip - (char *)di.dli_fbase));
    }
#endif
}

/* Recover the guest call chain: every lifted call pushes its guest return
 * address before jumping, so scanning the guest stack for values inside the
 * code range finds it. Over-reports (stale returns linger below esp), but
 * naming the caller is what a fault report needs. */
static void print_guest_stack(void)
{
    const uint32_t *sp;
    int i, shown = 0;

    if (!g_xbox_mem_offset || !g_esp)
        return;
    sp = (const uint32_t *)((uintptr_t)g_xbox_mem_offset + g_esp);
    fprintf(stderr, "  guest stack (return addresses, innermost first):\n");
    for (i = 0; i < 256 && shown < 24; i++) {
        uint32_t v = sp[i];
        if (v > g_xbox_code_lo && v < g_xbox_code_hi) {
            fprintf(stderr, "    [esp+%-4d] 0x%08X\n", i * 4, v);
            shown++;
        }
    }
    if (!shown) {
        extern volatile uint32_t g_icall_trace[16];
        extern volatile uint32_t g_icall_trace_idx;
        uint32_t k;

        fprintf(stderr, "    (no return addresses in range)\n  recent ICALL targets:");
        for (k = 0; k < 16; k++)
            fprintf(stderr, " %08X", g_icall_trace[(g_icall_trace_idx + k) & 15]);
        fprintf(stderr, "\n");
        for (i = 0; i < 20; i++)
            fprintf(stderr, "    raw[esp+%-4d] 0x%08X\n", i * 4, sp[i]);
    }
}

/* The emulated APU: its registers are unmapped when RECOMP_AC97_READY is set,
 * and every access arrives here to be decoded and serviced. */
typedef struct MCPXAPUState MCPXAPUState;
extern MCPXAPUState *mcpx_apu_init_standalone(uint8_t *ram_ptr);
extern MCPXAPUState *g_apu_state;
extern bool apu_hook_handle_mmio(PCONTEXT ctx, uintptr_t fault_addr,
                                 uint32_t fault_xbox_va, int is_write);
#define APU_TRAP_BASE 0xFE800000u
#define APU_TRAP_END  0xFE880000u

#define MEM32_HOST(va) (*(uint32_t *)(uintptr_t)(g_xbox_mem_offset + (uint32_t)(va)))
/* NGB_WATCH_WRITE=va[:len]: report every write to guest [va, va+len) -- the
 * host function (a lifted sub_XXXXXXXX) and the guest return addresses --
 * by write-protecting the page and stepping over each write. */
static uint32_t s_watch_va, s_watch_len, s_watch_page;
static __thread int t_watch_stepping;
static unsigned long s_watch_hits;

static void watch_protect(int on)
{
    DWORD old;
    VirtualProtect((void *)(uintptr_t)(g_xbox_mem_offset + s_watch_page), 4096,
                   on ? PAGE_READONLY : PAGE_READWRITE, &old);
}

static void watch_arm(void)
{
    const char *e = getenv("NGB_WATCH_WRITE");
    char *end;
    if (!e || !*e)
        return;
#if defined(__APPLE__) && defined(__aarch64__)
    fprintf(stderr, "NGB_WATCH_WRITE requires x86 single-step support\n");
    return;
#endif
    s_watch_va = (uint32_t)strtoul(e, &end, 0);
    s_watch_len = *end == ':' ? (uint32_t)strtoul(end + 1, NULL, 0) : 4;
    s_watch_page = s_watch_va & ~0xFFFu;
    watch_protect(1);
    fprintf(stderr, "[WATCH] writes to 0x%08X+%u\n", s_watch_va, s_watch_len);
}

static LONG watch_exception(PEXCEPTION_POINTERS ep, uint32_t xbox_va, int is_write)
{
    if (ep->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP) {
        if (!t_watch_stepping)
            return EXCEPTION_CONTINUE_SEARCH;
        t_watch_stepping = 0;
        ep->ContextRecord->EFlags &= ~0x100u;
        watch_protect(1);
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (!s_watch_len || !is_write || (xbox_va & ~0xFFFu) != s_watch_page)
        return EXCEPTION_CONTINUE_SEARCH;
    static long long after = -1, t0;
    if (after < 0) {
        const char *e = getenv("NGB_WATCH_AFTER");   /* ms before logging */
        after = e ? atoll(e) : 0;
        t0 = (long long)GetTickCount64();
    }
    if (xbox_va + 8 > s_watch_va && xbox_va < s_watch_va + s_watch_len &&
        (long long)GetTickCount64() - t0 >= after && s_watch_hits++ < 400) {
        void *rip = (void *)(uintptr_t)ep->ContextRecord->Rip;
#if defined(_WIN32)
        const char *who = "?";
#else
        Dl_info di;
        const char *who = dladdr(rip, &di) && di.dli_sname ? di.dli_sname : "?";
#endif
        fprintf(stderr, "[WATCH] write 0x%08X by %s at %p (esp=%08X ret %08X %08X %08X %08X)\n",
                xbox_va, who, rip, g_esp, MEM32_HOST(g_esp), MEM32_HOST(g_esp + 4),
                MEM32_HOST(g_esp + 8), MEM32_HOST(g_esp + 12));
#if !defined(_WIN32)
        if (getenv("NGB_WATCH_BT") && s_watch_hits % 50 == 1) {
            void *frames[64];
            int n = backtrace(frames, 64);
            backtrace_symbols_fd(frames, n, 2);
        }
#endif
    }
    watch_protect(0);
    t_watch_stepping = 1;
    ep->ContextRecord->EFlags |= 0x100u;
    return EXCEPTION_CONTINUE_EXECUTION;
}

static LONG CALLBACK veh_handler(PEXCEPTION_POINTERS ep)
{
    uintptr_t fault_addr;
    uint32_t xbox_va;
    int is_write;

#if defined(_WIN32) && XBOX_HAS_VCPU
    vcpu_win_unredirect(ep);   /* the faulting instruction's context, not the preemption trampoline's */
#endif
    if (s_watch_len && (ep->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP ||
                        ep->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)) {
        uintptr_t fa = ep->ExceptionRecord->ExceptionInformation[1];
        LONG r = watch_exception(ep, (uint32_t)(fa - (uintptr_t)g_xbox_mem_offset),
                                 ep->ExceptionRecord->ExceptionInformation[0] ? 1 : 0);
        if (r == EXCEPTION_CONTINUE_EXECUTION)
            return r;
    }
    if (ep->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_CONTINUE_SEARCH;

    fault_addr = ep->ExceptionRecord->ExceptionInformation[1];
    is_write = ep->ExceptionRecord->ExceptionInformation[0] ? 1 : 0;
    xbox_va = (uint32_t)(fault_addr - (uintptr_t)g_xbox_mem_offset);

    if (g_apu_state && xbox_va >= APU_TRAP_BASE && xbox_va < APU_TRAP_END
            && apu_hook_handle_mmio(ep->ContextRecord, fault_addr, xbox_va, is_write))
        return EXCEPTION_CONTINUE_EXECUTION;
    /* The USB host controllers (the controller ports). */
    if (xbox_OhciOwnsAddress(xbox_va) &&
        xbox_OhciHandleMmio(ep->ContextRecord, xbox_va))
        return EXCEPTION_CONTINUE_EXECUTION;

    fprintf(stderr, "[CRASH] Access violation at RIP=0x%llX, fault addr=0x%llX (%s)\n",
            (unsigned long long)ep->ContextRecord->Rip,
            (unsigned long long)fault_addr, is_write ? "write" : "read");
    fprintf(stderr, "  Xbox VA of fault: 0x%08X\n", xbox_va);
    fprintf(stderr, "  Xbox regs: eax=%08X ecx=%08X edx=%08X ebx=%08X\n",
            g_eax, g_ecx, g_edx, g_ebx);
    fprintf(stderr, "             esi=%08X edi=%08X esp=%08X\n",
            g_esi, g_edi, g_esp);
    print_host_symbol((void *)(uintptr_t)ep->ContextRecord->Rip);
    print_guest_stack();
    fflush(stderr);
    return EXCEPTION_CONTINUE_SEARCH;
}

/* ── Hang sampling (Linux) ─────────────────────────────────── */

#if !defined(_WIN32)
/* `kill -USR1 <pid>` prints where the guest thread is: a host backtrace
 * (every lifted function is a named symbol) and its guest registers. No
 * ptrace, so it works where gdb cannot attach and where gdb would swallow the
 * TF single-steps the register traps depend on. Whichever thread the kernel
 * hands the signal to forwards it to the guest thread. */
static pthread_t s_guest_thread;
static sem_t *s_sample_done;

/* One external `kill -USR1` fans out to every guest thread (the boot thread
 * and everything CreateThread started); each prints its own host backtrace
 * and its own guest registers, which are thread-local. */
static void sample_on_signal(int sig, siginfo_t *si, void *uc)
{
    void *frames[48];
    int n;

    (void)uc;
#ifdef __APPLE__
    if (!pthread_equal(pthread_self(), s_guest_thread)) {
        pthread_kill(s_guest_thread, sig);
        return;
    }
#else
    if (si->si_code == SI_USER) {
        /* One at a time, so the backtraces do not interleave. */
        static sem_t done;
        static int inited;
        pthread_t th[256];
        int k = compat_thread_list(th, 256);
        if (!inited) { sem_init(&done, 0, 0); inited = 1; }
        s_sample_done = &done;
        th[k++] = s_guest_thread;
        for (int i = 0; i < k; i++) {
            if (pthread_equal(th[i], pthread_self()))
                continue;   /* cannot wait on itself; it is sampled last */
            if (pthread_kill(th[i], sig) == 0) {
                struct timespec ts;
                clock_gettime(CLOCK_REALTIME, &ts);
                ts.tv_sec += 2;
                sem_timedwait(&done, &ts);
            }
        }
#if defined(NGB_NV2A_VK)
        if (s_use_gpu)
            nv2a_vk_sample_threads();
#endif
        xbox_PeekSample("sample");
        {   /* NGB_DUMP_RAM=path: the guest's low 64 MB, then the 128 MB
             * contiguous window, to path.N */
            const char *path = getenv("NGB_DUMP_RAM");
            static int seq;
            if (path && *path) {
                char name[600];
                FILE *f;
                snprintf(name, sizeof name, "%s.%d", path, seq++);
                f = fopen(name, "wb");
                if (f) {
                    fwrite((const void *)(uintptr_t)xbox_GetMemoryOffset(), 1,
                           64u << 20, f);
                    /* then the contiguous window (0x80000000), where the
                     * title's heaps live */
                    fwrite((const void *)((uintptr_t)xbox_GetMemoryOffset()
                                          + 0x80000000u), 1, 128u << 20, f);
                    fclose(f);
                    fprintf(stderr, "[DUMP] RAM -> %s\n", name);
                }
            }
        }
        {   /* NGB_DUMP=va:len[,va:len...]: hex+ascii dump on a sample */
            const char *d = getenv("NGB_DUMP");
            while (d && *d) {
                char *e;
                uint32_t va = (uint32_t)strtoul(d, &e, 0), len = 0;
                if (*e == ':') len = (uint32_t)strtoul(e + 1, &e, 0);
                for (uint32_t o = 0; o < len; o += 16) {
                    fprintf(stderr, "[DUMP] %08X:", va + o);
                    for (int j = 0; j < 16; j++)
                        fprintf(stderr, " %02X", ((const uint8_t *)(uintptr_t)xbox_GetMemoryOffset())[va + o + j]);
                    fprintf(stderr, "  ");
                    for (int j = 0; j < 16; j++) {
                        uint8_t c = ((const uint8_t *)(uintptr_t)xbox_GetMemoryOffset())[va + o + j];
                        fputc(c >= 32 && c < 127 ? c : '.', stderr);
                    }
                    fputc('\n', stderr);
                }
                d = *e == ',' ? e + 1 : e;
                if (e == d && *e) break;
            }
        }
        return;
    }
#endif
    n = backtrace(frames, 48);
    fprintf(stderr, "[SAMPLE] %s thread %lx prio %d%s: esp=%08X eax=%08X ecx=%08X "
                    "edx=%08X ebx=%08X esi=%08X edi=%08X\n",
            pthread_equal(pthread_self(), s_guest_thread) ? "boot" : "guest",
            (unsigned long)pthread_self(),
            vcpu_self() ? vcpu_get_priority(vcpu_self()) : -99,
            vcpu_holds_cpu() ? " (running)" : "",
            g_esp, g_eax, g_ecx, g_edx, g_ebx, g_esi, g_edi);
    backtrace_symbols_fd(frames, n, 2);
    if (s_sample_done)
        sem_post(s_sample_done);
}
#endif

/* ── Profiling (Linux) ─────────────────────────────────────── */

#if !defined(_WIN32)
#include <sys/time.h>
#ifdef __APPLE__
#include <sys/ucontext.h>
#else
#include <ucontext.h>
#endif
/* NGB_PROFILE=1: sample the instruction pointer of whichever thread is using
 * CPU every millisecond of process CPU time, and print the hottest functions
 * (every lifted guest function is a named symbol) when the process ends. */
#define PROF_SLOTS (1u << 20)
static uintptr_t *s_prof_pc;
static volatile unsigned long s_prof_n;

static void prof_signal(int sig, siginfo_t *si, void *uc_)
{
    ucontext_t *uc = uc_;
    unsigned long i = __atomic_fetch_add(&s_prof_n, 1, __ATOMIC_RELAXED);
    (void)sig; (void)si;
    if (i < PROF_SLOTS)
#if defined(__APPLE__) && defined(__aarch64__)
        s_prof_pc[i] = uc->uc_mcontext->__ss.__pc;
#else
        s_prof_pc[i] = (uintptr_t)uc->uc_mcontext.gregs[REG_RIP];
#endif
}

typedef struct { const char *name; unsigned long n; } ProfEntry;

static int prof_cmp(const void *a, const void *b)
{
    const ProfEntry *x = a, *y = b;
    return (y->n > x->n) - (y->n < x->n);
}

static void prof_report(void)
{
    unsigned long n = s_prof_n < PROF_SLOTS ? s_prof_n : PROF_SLOTS, k = 0;
    ProfEntry *e;

    if (!n)
        return;
    e = calloc(n, sizeof(*e));
    for (unsigned long i = 0; i < n; i++) {
        Dl_info di;
        const char *name = "?";
        if (dladdr((void *)s_prof_pc[i], &di))
            name = di.dli_sname ? di.dli_sname : di.dli_fname;
        unsigned long j;
        for (j = 0; j < k; j++)
            if (e[j].name == name || !strcmp(e[j].name, name))
                break;
        if (j == k)
            e[k++].name = name;
        e[j].n++;
    }
    qsort(e, k, sizeof(*e), prof_cmp);
    fprintf(stderr, "[PROFILE] %lu samples (1 ms of CPU each)\n", n);
    for (unsigned long j = 0; j < k && j < 40; j++)
        fprintf(stderr, "  %5.1f%%  %s\n", 100.0 * e[j].n / n, e[j].name);
    free(e);
}

static void prof_exit_on_signal(int sig)
{
    struct itimerval off = { { 0, 0 }, { 0, 0 } };
    (void)sig;
    setitimer(ITIMER_PROF, &off, NULL);
    prof_report();
    _exit(0);   /* see feedback_on_signal */
}

static void prof_start(void)
{
    struct sigaction sa;
    struct itimerval it = { { 0, 1000 }, { 0, 1000 } };

    s_prof_pc = calloc(PROF_SLOTS, sizeof(*s_prof_pc));
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = prof_signal;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGPROF, &sa, NULL);
    setitimer(ITIMER_PROF, &it, NULL);
    atexit(prof_report);
}
#endif

/* ── Paths ─────────────────────────────────────────────────── */

/* Directory holding the executable, with a trailing separator. */
static void exe_dir(char *out, size_t n)
{
    char *slash;
    if (ngb_exe_path(out, n) < 0) { out[0] = 0; return; }
    slash = strrchr(out, '/');
    if (slash) slash[1] = 0; else out[0] = 0;
}

/* Per-title save location (hard disk partition images, saves, cache):
 * NGB_SAVE_DIR, else $XDG_DATA_HOME or ~/.local/share on Linux and
 * %LOCALAPPDATA% on Windows, under NinjaGaidenBlack. */
static void save_dir_path(char *out, size_t n)
{
    const char *env = getenv("NGB_SAVE_DIR");
#if defined(_WIN32)
    const char *base = getenv("LOCALAPPDATA");
    if (env && *env) { snprintf(out, n, "%s", env); return; }
    snprintf(out, n, "%s/NinjaGaidenBlack", base && *base ? base : ".");
#else
    const char *xdg = getenv("XDG_DATA_HOME"), *home = getenv("HOME");
    if (env && *env) { snprintf(out, n, "%s", env); return; }
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

static bool load_file(const char *path, void **out_data, size_t *out_size)
{
    FILE *f = fopen(path, "rb");
    long size;
    void *data;

    if (!f)
        return false;
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0 || !(data = malloc((size_t)size))) {
        fclose(f);
        return false;
    }
    if (fread(data, 1, (size_t)size, f) != (size_t)size) {
        free(data);
        fclose(f);
        return false;
    }
    fclose(f);
    *out_data = data;
    *out_size = (size_t)size;
    return true;
}

/* ── Entry ─────────────────────────────────────────────────── */

/* Runtime switches this title needs to get past hardware it waits on, until
 * the hardware is modelled. Set only when absent, so any of them can still be
 * overridden (or turned off with an empty value) from the environment.
 *
 *   RECOMP_VBLANK      D3D blocks on its vblank event; raise the NV2A vblank
 *                      interrupt at 60 Hz.
 *   RECOMP_AC97_READY  DirectSound programs the APU directly; trap its
 *                      registers into the emulated APU.
 *   RECOMP_DSP_ACK     DirectSound's effects-image download posts command 3 to
 *                      the GP DSP mailbox and spins until the DSP clears it
 *                      (sub_0037DAAE). The DSP is not emulated, so complete it
 *                      at once. The mailbox is +0x810 into a contiguous block
 *                      whose address is deterministic for this build (it moves if the
 *                      contiguous allocator does: XBOX_CONTIG_ALLOC_BASE). */
static void set_default(const char *name, const char *value)
{
    if (getenv(name))
        return;
#if defined(_WIN32)
    _putenv_s(name, value);
#else
    setenv(name, value, 0);
#endif
}

/* NGB_RENDERER: "d3d" (default when built with the GPU model: the Direct3D
 * renderer, src/d3d, draws on the GPU model's device and window), "xemu"
 * (the GPU model's own NV2A renderer draws), "mirror" (xemu draws and shows,
 * the Direct3D renderer draws unseen, for comparing), "null" (the GPU model
 * without a window -- for headless runs), or "none" (no GPU model: the
 * plain-RAM aperture and the fence mirror, as before it existed). */

static void choose_renderer(void)
{
#if defined(NGB_NV2A_VK)
    set_default("NGB_RENDERER", "d3d");   /* read again by src/d3d (d3d.c, hle.cpp, vk/backend.cpp) */
    if (!strcmp(getenv("NGB_RENDERER"), "d3d") && !getenv("NGB_D3D_KEEP_NV2A"))
        set_default("NV2A_NO_PREWARM", "1");   /* xemu's renderer draws nothing: none of its pipelines */
#endif
    const char *r = getenv("NGB_RENDERER");
#if defined(NGB_NV2A_VK)
    s_use_gpu = !(r && !strcmp(r, "none"));
    s_headless = r && !strcmp(r, "null");
#else
    (void)r;
#endif
}

static void bringup_defaults(void)
{
    /* With the GPU model, vblank comes from its CRTC. */
    if (!s_use_gpu)
        set_default("RECOMP_VBLANK", "1");
    set_default("RECOMP_AC97_READY", "1");
    set_default("RECOMP_USB", "1");
    set_default("RECOMP_DSP_ACK", "0x86BF8810");   /* contiguous memory starts at 64 MB */
}

#if defined(RECOMP_ICALL_FEEDBACK)
/* A bring-up run usually ends at a timeout or ^C, not an orderly exit, and
 * atexit does not run on a signal. Dump on the way out of those too. */
static void feedback_on_signal(int sig)
{
    RECOMP_ICALL_FEEDBACK_DUMP();
    /* Straight out, as closing the window does: exit() would run library
     * destructors (the Vulkan driver's, SDL's audio) while the GPU and
     * audio threads still use them, and either hangs or crashes there. */
    _exit(128 + sig);
}
#endif

/* ── Boot (guest thread) ───────────────────────────────────── */


/* Everything from loading the XBE to the entry point runs on the thread that
 * will execute guest code: the guest register file and the TIB the kernel
 * sets up are per-thread. */
static void *guest_main(void *unused)
{
    char xbe_path[4200];
    void *xbe_data = NULL;
    size_t xbe_size = 0;

    (void)unused;
#if !defined(_WIN32)
    s_guest_thread = pthread_self();
#endif
#if XBOX_HAS_VCPU
    /* The boot thread runs the title's main thread inline: it is a guest
     * thread like the ones the title creates (platform/vcpu.h). */
    vcpu_attach(VCPU_PRIO_NORMAL);
#endif
    snprintf(xbe_path, sizeof(xbe_path), "%s/default.xbe", s_game_dir);
    if (!load_file(xbe_path, &xbe_data, &xbe_size)) {
        fprintf(stderr, "Cannot read %s\n"
                        "Put the extracted disc in a 'game' directory next to the "
                        "executable, or pass its path.\n", xbe_path);
        exit(1);
    }
    printf("XBE loaded: %zu bytes\n", xbe_size);

    if (!xbox_MemoryLayoutInit(xbe_data, xbe_size)) {
        fprintf(stderr, "Failed to initialize the Xbox memory layout.\n");
        exit(1);
    }
    g_xbox_mem_offset = xbox_GetMemoryOffset();
    printf("Xbox memory mapped. Offset: 0x%llX\n",
           (unsigned long long)g_xbox_mem_offset);

#if defined(NGB_NV2A_VK)
    if (s_use_gpu) {
        Nv2aVkOptions o = { .headless = s_headless };
        if (xbox_Nv2aVkStart(&o) != 0) {
            fprintf(stderr, "Cannot start the GPU.\n");
            exit(1);
        }
    } else
#endif
    {
        /* No GPU model: D3D8LTCG's pushbuffer fence never advances on its
         * own. The device pointer lives at 0x3D83C8; +0x2C is the write
         * sequence and +0x30 points at where the GPU reports how far it has
         * read. Mirror one into the other so space and fence waits
         * (sub_003CAA10) complete instead of spinning forever. */
        xbox_Nv2aMirrorFence(NGB_D3D_DEVICE_PTR, 0x2C, 0x30);
    }

    /* The APU and its register trap are only useful together: a trap with no
     * model declines every access, a model nothing traps into sees nothing. */
    if (getenv("RECOMP_AC97_READY")) {
        g_apu_state = mcpx_apu_init_standalone((uint8_t *)xbox_GetMemoryBase());
        fprintf(stderr, "[BOOT] emulated APU %s\n",
                g_apu_state ? "up" : "FAILED to initialise");
    }

    xbox_kernel_init();
    {
        extern void xbox_path_init(const char *game_dir, const char *save_dir);
        char save_dir[4096];
        save_dir_path(save_dir, sizeof(save_dir));
        printf("Save directory: %s\n", save_dir);
        xbox_path_init(s_game_dir, save_dir);
    }
    xbox_kernel_bridge_init();
    /* Controller ports: OHCI with an Xbox gamepad fed by SDL's game
     * controllers (src/usb). Inert unless RECOMP_USB is set. */
    xbox_OhciInit();
    g_esp = XBOX_STACK_TOP;

    /* Flat dispatch table: without it every indirect call binary-searches the
     * function table. Optional -- the search still works if this fails. */
    if (!recomp_dispatch_init())
        fprintf(stderr, "[BOOT] flat dispatch unavailable; "
                        "indirect calls will use the binary search\n");

    /* Inert unless RECOMP_WATCHDOG_SECS is set; must run on the guest thread. */
    xbox_WatchdogStart();

    watch_arm();
    printf("Entry point: 0x%08X, ESP: 0x%08X\nStarting game...\n",
           NGB_ENTRY_POINT, g_esp);
    {
        /* The PC settings in the game's Options menu (src/pc_menu.c). */
        extern void ngb_pc_menu_init(void);
        ngb_pc_menu_init();
    }
    xbe_entry_point();

    printf("Game returned.\n");
    exit(0);
    return NULL;
}

/* ── Entry (host main thread: window and events) ───────────── */

#if defined(_WIN32)
#include <tlhelp32.h>
#include <psapi.h>
/* NGB_HANG_DUMP=<seconds> (debugging, Windows): after that long, each thread's call stack, as
 * module+offset, into the log -- where a hang is, without a debugger. A thread is suspended only
 * while its frames are read into a buffer: nothing that takes a lock (stdio, the loader) runs
 * meanwhile, since the suspended thread may hold it. */
static DWORD WINAPI hang_dump_thread(LPVOID arg)
{
    static HMODULE mods[256];
    static char names[256][64];
    static char out[1 << 16];
    MODULEINFO mi[256];
    DWORD need = 0, pid = GetCurrentProcessId(), me = GetCurrentThreadId();
    int nmods, len = 0;
    HANDLE snap;
    THREADENTRY32 te;

    Sleep((DWORD)(uintptr_t)arg * 1000);
    for (int round = 0; round < 3; round++, Sleep(3000)) {
    len = 0;
    K32EnumProcessModules(GetCurrentProcess(), mods, sizeof mods, &need);
    nmods = (int)(need / sizeof(HMODULE));
    if (nmods > 256) nmods = 256;
    for (int i = 0; i < nmods; i++) {
        char path[MAX_PATH], *b;
        K32GetModuleInformation(GetCurrentProcess(), mods[i], &mi[i], sizeof mi[i]);
        K32GetModuleFileNameExA(GetCurrentProcess(), mods[i], path, sizeof path);
        b = strrchr(path, '\\');
        snprintf(names[i], sizeof names[i], "%s", b ? b + 1 : path);
    }
    snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    te.dwSize = sizeof te;
    for (BOOL ok = Thread32First(snap, &te); ok && len < (int)sizeof out - 4096; ok = Thread32Next(snap, &te)) {
        HANDLE h;
        CONTEXT c;
        if (te.th32OwnerProcessID != pid || te.th32ThreadID == me)
            continue;
        h = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE,
                       te.th32ThreadID);
        if (!h)
            continue;
        (SuspendThread)(h);
        memset(&c, 0, sizeof c);
        c.ContextFlags = CONTEXT_FULL;
        if ((GetThreadContext)(h, &c)) {
            len += snprintf(out + len, sizeof out - len, "[HANG] thread %lu:", te.th32ThreadID);
            for (int f = 0; f < 20 && c.Rip; f++) {
                DWORD64 base = 0;
                PRUNTIME_FUNCTION fn = RtlLookupFunctionEntry(c.Rip, &base, NULL);
                int m = -1;
                for (int i = 0; i < nmods; i++)
                    if (c.Rip >= (DWORD64)mi[i].lpBaseOfDll &&
                        c.Rip < (DWORD64)mi[i].lpBaseOfDll + mi[i].SizeOfImage)
                        m = i;
                if (m >= 0)
                    len += snprintf(out + len, sizeof out - len, " %s+%llx", names[m],
                                    (unsigned long long)(c.Rip - (DWORD64)mi[m].lpBaseOfDll));
                else
                    len += snprintf(out + len, sizeof out - len, " %llx", (unsigned long long)c.Rip);
                if (fn) {
                    PVOID hd;
                    DWORD64 ef;
                    RtlVirtualUnwind(UNW_FLAG_NHANDLER, base, c.Rip, fn, &c, &hd, &ef, NULL);
                } else {
                    c.Rip = *(DWORD64 *)c.Rsp;
                    c.Rsp += 8;
                }
            }
            len += snprintf(out + len, sizeof out - len, "\n");
        }
        (ResumeThread)(h);
        CloseHandle(h);
    }
    CloseHandle(snap);
    {
        extern void nv2a_debug_state(char *buf, int n);
        extern void xbox_kernel_debug_counts(char *buf, int n, uint32_t watch_va);
        char gpu[512];
        nv2a_debug_state(gpu, sizeof gpu);
        len += snprintf(out + len, sizeof out - len, "%s\n", gpu);
        xbox_kernel_debug_counts(gpu, sizeof gpu, 0x3D83C8);   /* D3D's device pointer */
        len += snprintf(out + len, sizeof out - len, "%s\n", gpu);
        vcpu_debug_state(out + len, (int)sizeof out - len);   /* who holds the CPU, who waits */
        len += (int)strlen(out + len);
        /* NGB_HANG_MEM=va:bytes[,va:bytes...] (hex): guest memory, as words */
        if (getenv("NGB_HANG_MEM")) {
            char spec[512], *tok, *save = NULL;
            snprintf(spec, sizeof spec, "%s", getenv("NGB_HANG_MEM"));
            for (tok = strtok_r(spec, ",", &save); tok; tok = strtok_r(NULL, ",", &save)) {
                uint32_t va = (uint32_t)strtoul(tok, NULL, 16), n = 64;
                char *c = strchr(tok, ':');
                if (c) n = (uint32_t)strtoul(c + 1, NULL, 16);
                for (uint32_t i = 0; i < n && len < (int)sizeof out - 256; i += 32) {
                    len += snprintf(out + len, sizeof out - len, "[MEM] %08X:", va + i);
                    for (uint32_t j = i; j < i + 32 && j < n; j += 4)
                        len += snprintf(out + len, sizeof out - len, " %08X",
                                        *(uint32_t *)((uint8_t *)g_xbox_mem_offset + va + j));
                    len += snprintf(out + len, sizeof out - len, "\n");
                }
            }
        }
    }
    fprintf(stderr, "[HANG] sample %d\n", round + 1);
    fputs(out, stderr);
    fflush(stderr);
    }
    return 0;
}

/* NGB_MEM_TRACE=va:bytes[,...] (hex, debugging, Windows): those guest words every 50 ms for 20 s,
 * logged when they change -- what a game's state machine went through, against another run. */
static DWORD WINAPI mem_trace_thread(LPVOID arg)
{
    static uint32_t last[1024];
    char spec[512], *tok, *save = NULL;
    ULONGLONG t0 = GetTickCount64();
    (void)arg;
    while (GetTickCount64() - t0 < 20000) {
        int k = 0;
        Sleep(50);
        if (!g_xbox_mem_offset)
            continue;
        snprintf(spec, sizeof spec, "%s", getenv("NGB_MEM_TRACE"));
        for (tok = strtok_r(spec, ",", &save); tok; tok = strtok_r(NULL, ",", &save)) {
            uint32_t va = (uint32_t)strtoul(tok, NULL, 16), n = 64;
            char *c = strchr(tok, ':');
            if (c) n = (uint32_t)strtoul(c + 1, NULL, 16);
            for (uint32_t i = 0; i < n && k < 1024; i += 4, k++) {
                uint32_t v = *(uint32_t *)((uint8_t *)g_xbox_mem_offset + va + i);
                if (v != last[k]) {
                    fprintf(stderr, "[MEMT] %llu %08X: %08X -> %08X\n",
                            (unsigned long long)(GetTickCount64() - t0), va + i, last[k], v);
                    last[k] = v;
                }
            }
        }
        fflush(stderr);
    }
    return 0;
}
#endif

/* Portable mode (issue #4): a file named portable.txt beside the program -- beside the AppImage on
 * Linux -- keeps everything in a "userdata" folder there instead of the user's profile: the installed
 * game and the console's drive (about 11 GB), saves, settings, the shader cache and the log. Each
 * location set explicitly (NGB_SAVE_DIR and the rest) still wins. */
static void portable_init(void)
{
    char dir[4096], flag[4200], data[4200];
    FILE *f;

    if (!ngb_portable_base(dir, sizeof dir))
        return;
    snprintf(flag, sizeof flag, "%s/portable.txt", dir);
    if (!(f = fopen(flag, "r")))
        return;
    fclose(f);
    snprintf(data, sizeof data, "%s/userdata", dir);
    ngb_mkdir(data);
    setenv("NGB_PORTABLE", "1", 1);   /* the setup does not offer it again (src/install.c) */
    if (!getenv("NGB_SAVE_DIR"))
        setenv("NGB_SAVE_DIR", data, 1);
    if (!getenv("NGB_CONFIG_DIR"))
        setenv("NGB_CONFIG_DIR", data, 1);
    if (!getenv("NV2A_SHADER_CACHE")) {
        snprintf(flag, sizeof flag, "%s/shaders", data);
        setenv("NV2A_SHADER_CACHE", flag, 1);
    }
}

int main(int argc, char **argv)
{
    portable_init();   /* before anything picks a folder (the log's, first) */
#ifdef __APPLE__
    if (!getenv("NGB_TEST_HIDDEN") && !isatty(STDOUT_FILENO)) {
        char log[4200];
        save_dir_path(log, sizeof log - 16);
        ngb_mkdir(log);
        strcat(log, "/log.txt");
        if (freopen(log, "w", stdout)) {
            dup2(fileno(stdout), STDERR_FILENO);
            setvbuf(stdout, NULL, _IOLBF, 0);
        }
    }
#endif
#if defined(_WIN32)
    /* Sleeps and waits to the millisecond, not the 15.6 ms default tick: the
     * virtual CPU's time slices, the kernel's timers and the frame pacing
     * all sleep briefly. */
    timeBeginPeriod(1);
    /* A GUI program, so no console window opens; started from one, its
     * output goes there, else to log.txt in the save folder. */
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    } else {
        char log[4096];
        save_dir_path(log, sizeof log - 8);
        ngb_mkdir(log);
        strcat(log, "/log.txt");
        FILE *f = fopen(log, "w");      /* emptied, then both append */
        if (f) {
            fclose(f);
            freopen(log, "a", stdout);
            freopen(log, "a", stderr);
        }
    }
    if (getenv("NGB_MEM_TRACE"))
        CloseHandle(CreateThread(NULL, 0, mem_trace_thread, NULL, 0, NULL));
    if (getenv("NGB_HANG_DUMP"))
        CloseHandle(CreateThread(NULL, 0, hang_dump_thread,
                                 (LPVOID)(uintptr_t)atoi(getenv("NGB_HANG_DUMP")), 0, NULL));
#else
    /* NGB_PTRACE_ANY=1: let any process of this user attach a debugger
     * (Yama's ptrace_scope 1 allows only ancestors), for stack dumps. */
#ifdef __linux__
    if (getenv("NGB_PTRACE_ANY"))
        prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY, 0, 0, 0);
#endif
#endif
    /* --settings: show the settings screen before the game (the settings
     * are in the game's own menus; there is no screen otherwise). */
    for (int i = 1; i < argc; i++)
        if (!strcmp(argv[i], "--settings")) {
            s_show_settings = 1;
            memmove(&argv[i], &argv[i + 1], (size_t)(argc - i) * sizeof *argv);
            argc--;
            i--;
        }
#if defined(NGB_NV2A_VK)
    {
        extern void ngb_set_argv(char **argv);   /* src/ingame_menu.c: Return to Main Menu */
        ngb_set_argv(argv);
    }
#endif

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    choose_renderer();
    bringup_defaults();

    {
        /* A game folder, or a disc image/archive installed on first use. */
        char base[4096], save[4096];
        exe_dir(base, sizeof(base));
        save_dir_path(save, sizeof(save));
        if (ngb_find_game(argc > 1 ? argv[1] : NULL, save, base,
                          s_game_dir, sizeof(s_game_dir)) != 0)
            return 1;
    }

    printf("=== Ninja Gaiden Black - Static Recompilation ===\n");
    printf("Game directory: %s\n", s_game_dir);

#if defined(_WIN32)
    SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME);
    SymInitialize(GetCurrentProcess(), NULL, TRUE);
#endif
    AddVectoredExceptionHandler(1, veh_handler);
#if !defined(_WIN32)
    {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_sigaction = sample_on_signal;
        sa.sa_flags = SA_SIGINFO | SA_RESTART;
        sigaction(SIGUSR1, &sa, NULL);
    }
#endif
#if !defined(_WIN32)
    if (getenv("NGB_PROFILE")) {
        prof_start();
        signal(SIGINT, prof_exit_on_signal);
        signal(SIGTERM, prof_exit_on_signal);
    }
#endif
#if defined(RECOMP_ICALL_FEEDBACK)
    RECOMP_ICALL_FEEDBACK_INIT();     /* atexit dump to icall_targets.dump */
    signal(SIGINT, feedback_on_signal);
    signal(SIGTERM, feedback_on_signal);
#endif

#if defined(NGB_NV2A_VK)
    /* The settings screen, then the settings. Environment variables of the
     * same meaning still win, for testing. */
    NgbSettings cfg;
    ngb_settings_load(&cfg);
    if (s_use_gpu && !s_headless && s_show_settings &&
        !getenv("NGB_NO_SETTINGS")) {
        if (!ngb_launcher_run(&cfg))
            return 0;
    }
    {
        const char *e;
        if ((e = getenv("NGB_WIDESCREEN")))
            cfg.aspect = *e == '0' ? ASPECT_4_3 : ASPECT_16_9;
        if ((e = getenv("NGB_FULLSCREEN")))
            cfg.fullscreen = *e == '0' ? 0 : *e == '2' ? 2 : 1;
        if ((e = getenv("NGB_VSYNC")))
            cfg.vsync = *e == '2' ? 2 : *e != '0';
        if (!getenv("NV2A_SCALE") && cfg.scale > 1) {
            char v[8];
            snprintf(v, sizeof v, "%d", cfg.scale);
            setenv("NV2A_SCALE", v, 1);
        }
        if (!getenv("NV2A_TEX_REPLACE")) {
            /* Texture packs: <data>/textures/<id>.png replace the title's
             * textures (NV2A_TEX_DUMP=dir writes the ids it uses). */
            char dir[4096];
            save_dir_path(dir, sizeof dir - 16);
            strcat(dir, "/textures");
            setenv("NV2A_TEX_REPLACE", dir, 1);
        }
        if (!getenv("NV2A_SHADER_CACHE")) {
            /* Compiled shaders, so a new effect only stutters once. */
            const char *xdg = getenv("XDG_CACHE_HOME"), *home = ngb_home();
            char dir[4096];
#if defined(_WIN32)
            if ((!xdg || !*xdg) && getenv("LOCALAPPDATA"))   /* %LOCALAPPDATA%\NinjaGaidenBlack\shaders */
                xdg = getenv("LOCALAPPDATA");
#endif
            if (xdg && *xdg)
                snprintf(dir, sizeof dir, "%s/NinjaGaidenBlack/shaders", xdg);
            else if (home && *home) {
#ifdef __APPLE__
                ngb_macos_dir(dir, sizeof dir - 32, "cache", 1);
                strcat(dir, "/NinjaGaidenBlack/shaders");
#else
                snprintf(dir, sizeof dir, "%s/.cache/NinjaGaidenBlack/shaders", home);
#endif
            } else
                dir[0] = 0;
            if (dir[0])
                setenv("NV2A_SHADER_CACHE", dir, 1);
        }
        if (!getenv("NV2A_SHADER_SEED")) {
            /* Shipped beside the binary: <prefix>/bin/NinjaGaidenBlack and
             * <prefix>/share/NinjaGaidenBlack/shader-seed (on Windows the folder's
             * share/NinjaGaidenBlack, beside the .exe). */
            char exe[4096], seed[4200];
            if (ngb_exe_path(exe, sizeof exe) > 0) {
                char *slash = strrchr(exe, '/');
                if (slash) {
                    *slash = 0;
                    snprintf(seed, sizeof seed,
                             "%s/../share/NinjaGaidenBlack/shader-seed", exe);
                    if (access(seed, 4) != 0)
                        snprintf(seed, sizeof seed, "%s/share/NinjaGaidenBlack/shader-seed", exe);
                    if (access(seed, 4) == 0)
                        setenv("NV2A_SHADER_SEED", seed, 1);
                }
            }
        }
        if (!getenv("NV2A_SHADER_SEED_XBE")) {
            /* The seed's vertex programs come from the user's own copy. */
            char xbe[4200];
            snprintf(xbe, sizeof xbe, "%s/default.xbe", s_game_dir);
            setenv("NV2A_SHADER_SEED_XBE", xbe, 1);
        }
        ngb_menu_set_running(&cfg);
        if (!getenv("NV2A_SHADOW_SCALE")) {
            char v[8];
            snprintf(v, sizeof v, "%d", cfg.shadow_scale);
            setenv("NV2A_SHADOW_SCALE", v, 1);
        }
        if (!getenv("NV2A_TEX_FILTER")) {
            char v[8];
            snprintf(v, sizeof v, "%d", cfg.anisotropy);
            setenv("NV2A_TEX_FILTER", cfg.anisotropy ? v : "original", 1);
        }
        nv2a_set_quality(cfg.scale, cfg.anisotropy, cfg.shadow_scale, cfg.msaa);
        {
            extern void ngb_set_all_difficulties(int on);   /* src/cheats.c */
            extern void ngb_set_all_costumes(int on);
            ngb_set_all_difficulties(cfg.all_difficulties);
            ngb_set_all_costumes(cfg.all_costumes);
        }
        if (cfg.show_fps && !getenv("NV2A_SHOW_FPS"))
            setenv("NV2A_SHOW_FPS", "1", 1);
        /* The boot "Warnings" screen is always skipped (its scrolling text drew badly, and it says only
         * what the box does); the boot movie with Skip intro. */
        if (cfg.skip_intro)
            ngb_skip_intro = 1;
        if (!getenv("NGB_KEEP_WARNINGS")) {
            if (!s_headless) {
                pthread_t th;
                nv2a_present_set_blank(true);
                if (pthread_create(&th, NULL, warning_unblank_thread, NULL) == 0)
                    pthread_detach(th);
            }
            xbox_FileReadHook = movie_read_hook;
        } else if (cfg.skip_intro) {
            xbox_FileReadHook = movie_read_hook;
        }
    }
    /* Widescreen (16:9 and 16:10 alike: the game's anamorphic mode, shown
     * at the chosen shape) unless 4:3. */
    s_widescreen = cfg.aspect != ASPECT_4_3;
#else
    {
        const char *ws = getenv("NGB_WIDESCREEN");
        s_widescreen = !(ws && *ws == '0');
    }
#endif
    if (s_widescreen)
        xbox_XcVideoFlags |= XC_VIDEO_FLAGS_WIDESCREEN;
#if defined(NGB_NV2A_VK)
    if (s_widescreen) {   /* the 3D's shape: the game is kept in its 4:3 mode on a wide screen (src/pc_menu.c) */
        extern void ngb_game_set_aspect_constant(double a);
        extern void ngb_game_aspect_init(int widescreen);
        ngb_game_set_aspect_constant(ngb_settings_aspect_ratio(&cfg));
        ngb_game_aspect_init(1);   /* widescreen for the boot movie, 4:3 from the title (src/pc_menu.c) */
    }
#endif
#if defined(NGB_NV2A_VK)
    if (s_use_gpu && !s_headless &&
        nv2a_window_create("Ninja Gaiden Black", cfg.width, cfg.height,
                           cfg.fullscreen, ngb_settings_aspect_ratio(&cfg),
                           cfg.vsync) != 0) {
        fprintf(stderr, "Cannot open the game window.\n");
        return 1;
    }
    /* Controllers and keyboard, on the thread that pumps the window's
     * events (hotplug arrives there too). */
    if (s_use_gpu && !s_headless)
        xbox_InputInit();
#endif

#if defined(_WIN32)
    CloseHandle(CreateThread(NULL, 64u << 20,
                             (LPTHREAD_START_ROUTINE)guest_main, NULL, 0, NULL));
#else
    {
        /* Lifted code nests a host call per guest call; give it room. */
        pthread_attr_t attr;
        pthread_t th;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, 64u << 20);
        if (pthread_create(&th, &attr, guest_main, NULL) != 0) {
            fprintf(stderr, "Cannot start the guest thread.\n");
            return 1;
        }
        pthread_attr_destroy(&attr);
    }
#endif

#if defined(NGB_NV2A_VK)
    if (s_use_gpu && !s_headless) {
        while (nv2a_window_pump())
            usleep(2000);
        printf("Window closed.\n");
        _exit(0);
    }
#endif
    for (;;)
#if defined(_WIN32)
        Sleep(INFINITE);
#else
        pause();
#endif
}
