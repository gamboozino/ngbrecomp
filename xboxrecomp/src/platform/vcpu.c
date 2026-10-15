/*
 * vcpu: one virtual CPU for guest threads (see vcpu.h).
 *
 * State lives under one mutex: who holds the CPU and who is ready for it.
 * A thread waiting for the CPU sleeps on its own condition variable and is
 * handed the CPU directly (holder = it) by whoever releases, so there is no
 * thundering herd and no window where a lower-priority thread can grab it.
 *
 * Preemption is a signal to the holder. In guest code (host depth 0) the
 * handler yields on the spot -- guest code holds no host locks, so blocking
 * there is safe. In host code it leaves a note that compat_host_leave reads.
 */
#if !defined(_WIN32)
#define _GNU_SOURCE
#endif
#include "vcpu.h"
#if XBOX_HAS_VCPU
#if defined(_WIN32)
/* Windows: the host's own threads, the waits wrapped by win32_vcpu.h (not in
 * this file), and in place of a signal the holder is suspended and sent
 * through vcpu_win_trampoline (below) to yield. */
#include <windows.h>
int compat_host_depth(void);
volatile int *compat_host_depth_ptr(void);
void compat_park_if_pending(void);
#else
#include "win32_compat.h"
#include <signal.h>
#endif

#include <errno.h>
#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define VCPU_MAX_THREADS 256
#define VCPU_QUANTUM_MS  10
/* A thread ready this long without the CPU gets it for one quantum, whatever
 * its priority: NT-family schedulers boost starved threads the same way, and
 * titles that spin at raised priority on work a lower thread has to finish
 * depend on it. */
#define VCPU_STARVE_MS   (vcpu_starve_ms())
#if !defined(_WIN32)
#define SIG_PREEMPT      (SIGRTMIN + 4)
#endif

struct vcpu_thread {
    pthread_t thread;
    pthread_cond_t cv;
    int priority;
    int ready;              /* waiting for the CPU */
    unsigned long seq;      /* FIFO order among equal priorities */
    int64_t ready_ms;       /* when it became ready */
    volatile int preempt;   /* asked to yield at the next safe point */
    int in_vcpu;            /* inside this file: a preempt signal must wait */
    int held_blocked;       /* released the CPU in vcpu_block_begin */
    int detached;
#if defined(_WIN32)
    HANDLE host;            /* this thread, for suspending it */
    DWORD tid;
    volatile int *depth;    /* its compat_host_depth */
    int suspended;          /* SuspendThread count: no CPU for it meanwhile */
    int stolen;             /* lost the CPU suspended: get it back before guest code */
    volatile int redirected;   /* on its way through vcpu_win_trampoline */
#endif
};

static pthread_mutex_t s_lock = PTHREAD_MUTEX_INITIALIZER;
static vcpu_thread *s_holder;
static vcpu_thread *s_threads[VCPU_MAX_THREADS];
static unsigned long s_seq;
static int64_t s_holder_since_ms;
static int s_enabled = -1;
static __thread vcpu_thread *t_self;


int vcpu_enabled(void)
{
    if (s_enabled < 0) {
        const char *e = getenv("RECOMP_VCPU");
        s_enabled = !(e && !strcmp(e, "0"));
    }
    return s_enabled;
}

vcpu_thread *vcpu_self(void)
{
    return t_self;
}

int vcpu_busy(void)
{
    return t_self && t_self->in_vcpu;
}

static int64_t now_ms(void)
{
#if defined(_WIN32)
    return (int64_t)GetTickCount64();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
#endif
}

static int64_t now_ms(void);

static int vcpu_starve_ms(void)
{
    static int v = -1;
    if (v < 0) {
        const char *e = getenv("RECOMP_VCPU_STARVE_MS");
        v = e ? atoi(e) : 50;
    }
    return v;
}

#if defined(_WIN32)
static int win_suspended(vcpu_thread *t) { return t->suspended > 0; }
static void win_preempt(vcpu_thread *h);
#else
static int win_suspended(vcpu_thread *t) { (void)t; return 0; }
#endif

/* The longest-starved ready thread, if any has waited past VCPU_STARVE_MS. */
static vcpu_thread *starved_ready(void)
{
    vcpu_thread *st = NULL;
    int64_t now = now_ms();
    for (int i = 0; i < VCPU_MAX_THREADS; i++) {
        vcpu_thread *t = s_threads[i];
        if (t && t->ready && !win_suspended(t) && now - t->ready_ms >= VCPU_STARVE_MS &&
            (!st || t->ready_ms < st->ready_ms))
            st = t;
    }
    return st;
}

/* The best ready thread, or NULL. Caller holds s_lock. */
static vcpu_thread *best_ready(void)
{
    vcpu_thread *best = starved_ready();
    if (best)
        return best;
    for (int i = 0; i < VCPU_MAX_THREADS; i++) {
        vcpu_thread *t = s_threads[i];
        if (!t || !t->ready || win_suspended(t))
            continue;
        if (!best || t->priority > best->priority ||
            (t->priority == best->priority && t->seq < best->seq))
            best = t;
    }
    return best;
}

/* Hand the CPU to the best ready thread. Caller holds s_lock, and the CPU is
 * free. */
static void dispatch(void)
{
    vcpu_thread *next = best_ready();
    if (next) {
        next->ready = 0;
        s_holder = next;
        s_holder_since_ms = now_ms();
        pthread_cond_signal(&next->cv);
    }
}

static void request_preempt(vcpu_thread *h)
{
    if (h && !h->preempt) {
        h->preempt = 1;
        /* Not to oneself: the handler would take s_lock, which is held. The
         * flag is read at the next safe point instead. */
#if defined(_WIN32)
        if (h != t_self)
            win_preempt(h);
#else
        if (!pthread_equal(h->thread, pthread_self()))
            pthread_kill(h->thread, SIG_PREEMPT);
#endif
    }
}

/* Wait for the CPU. Caller holds s_lock and does not hold the CPU.
 * polite: do not preempt the holder (it was just handed the CPU by this
 * thread's yield); the quantum timer brings the CPU back. */
static void acquire_locked_ex(vcpu_thread *t, int polite)
{
    t->ready = 1;
    t->seq = ++s_seq;
    t->ready_ms = now_ms();
    if (!s_holder) {
        dispatch();
    } else if (!polite && t->priority > s_holder->priority) {
        request_preempt(s_holder);
    }
    while (s_holder != t)
        pthread_cond_wait(&t->cv, &s_lock);
    t->preempt = 0;
}

static void acquire_locked(vcpu_thread *t)
{
    acquire_locked_ex(t, 0);
}

static void release_locked(vcpu_thread *t)
{
    if (s_holder == t) {
        s_holder = NULL;
        dispatch();
    }
}

static void yield_now(vcpu_thread *t)
{
    t->in_vcpu = 1;
    pthread_mutex_lock(&s_lock);
    t->preempt = 0;
    if (s_holder == t) {
        vcpu_thread *b = best_ready();
        if (b && (b->priority >= t->priority || starved_ready())) {
            release_locked(t);
            acquire_locked_ex(t, 1);
        }
    }
    pthread_mutex_unlock(&s_lock);
    t->in_vcpu = 0;
}

#if !defined(_WIN32)
static void preempt_signal(int sig)
{
    vcpu_thread *t = t_self;
    int saved = errno;

    (void)sig;
    if (t && !t->in_vcpu && compat_host_depth() == 0) {
        yield_now(t);
        /* A suspension that arrived while this thread waited for the CPU
         * has to take effect before it runs more guest code. */
        compat_park_if_pending();
    }
    /* else: compat_host_leave sees t->preempt */
    errno = saved;
}
#endif

/* Time-slices equal priorities: a holder that has run a whole quantum while a
 * peer of the same (or higher) priority waits is asked to yield. */
static void *quantum_thread(void *unused)
{
    (void)unused;
    for (;;) {
        usleep(VCPU_QUANTUM_MS * 1000 / 2);
        pthread_mutex_lock(&s_lock);
        if (s_holder && now_ms() - s_holder_since_ms >= VCPU_QUANTUM_MS) {
            vcpu_thread *b = best_ready();
            if (b && (b->priority >= s_holder->priority || starved_ready()))
                request_preempt(s_holder);
        }
        pthread_mutex_unlock(&s_lock);
    }
    return NULL;
}

static void vcpu_init_once(void)
{
    pthread_t th;
#if !defined(_WIN32)
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = preempt_signal;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIG_PREEMPT, &sa, NULL);
#endif
    pthread_create(&th, NULL, quantum_thread, NULL);
    pthread_detach(th);
}

vcpu_thread *vcpu_attach(int priority)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    vcpu_thread *t;

    if (!vcpu_enabled() || t_self)
        return t_self;
    pthread_once(&once, vcpu_init_once);

    t = calloc(1, sizeof(*t));
    t->thread = pthread_self();
#if defined(_WIN32)
    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &t->host,
                    THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT |
                    THREAD_QUERY_INFORMATION, FALSE, 0);
    t->tid = GetCurrentThreadId();
    t->depth = compat_host_depth_ptr();
#endif
    t->priority = priority;
    pthread_cond_init(&t->cv, NULL);
    t_self = t;

    t->in_vcpu = 1;
    pthread_mutex_lock(&s_lock);
    for (int i = 0; i < VCPU_MAX_THREADS; i++)
        if (!s_threads[i]) {
            s_threads[i] = t;
            break;
        }
    acquire_locked(t);
    pthread_mutex_unlock(&s_lock);
    t->in_vcpu = 0;
    return t;
}

void vcpu_detach(void)
{
    vcpu_thread *t = t_self;
    if (!t)
        return;
    t->in_vcpu = 1;
    pthread_mutex_lock(&s_lock);
    for (int i = 0; i < VCPU_MAX_THREADS; i++)
        if (s_threads[i] == t)
            s_threads[i] = NULL;
    release_locked(t);
    t->detached = 1;
    pthread_mutex_unlock(&s_lock);
    t_self = NULL;
    /* The record is leaked on purpose: a racing preempt signal may still
     * name it, and threads are few. */
}

void vcpu_block_begin(void)
{
    vcpu_thread *t = t_self;
    if (!t)
        return;
    t->in_vcpu = 1;
    pthread_mutex_lock(&s_lock);
    if (s_holder == t) {
        release_locked(t);
        t->held_blocked++;
    }
    pthread_mutex_unlock(&s_lock);
    t->in_vcpu = 0;
}

/* Windows: the CPU taken while suspended is to be had back (and the note
 * cleared). Caller holds s_lock. */
static int win_stolen(vcpu_thread *t)
{
#if defined(_WIN32)
    if (t->stolen) {
        t->stolen = 0;
        return 1;
    }
#else
    (void)t;
#endif
    return 0;
}

void vcpu_block_end(void)
{
    vcpu_thread *t = t_self;
    if (!t)
        return;
    t->in_vcpu = 1;
    pthread_mutex_lock(&s_lock);
    if (t->held_blocked > 0 || win_stolen(t)) {
        if (t->held_blocked > 0)
            t->held_blocked--;
        if (s_holder != t)
            acquire_locked(t);
    }
    pthread_mutex_unlock(&s_lock);
    t->in_vcpu = 0;
}

void vcpu_yield(void)
{
    if (t_self)
        yield_now(t_self);
}

/* NtYieldExecution / SwitchToThread: hand the CPU to any ready thread, lower
 * priority included -- unlike Sleep(0), which only lets equal or higher
 * priorities in. A thread spinning on a lock held by a lower-priority one
 * depends on that. The lower thread keeps the CPU until it blocks or a
 * quantum passes. */
void vcpu_yield_any(void)
{
    vcpu_thread *t = t_self;
    if (!t)
        return;
    t->in_vcpu = 1;
    pthread_mutex_lock(&s_lock);
    t->preempt = 0;
    if (s_holder == t && best_ready()) {
        release_locked(t);
        acquire_locked_ex(t, 1);
    }
    pthread_mutex_unlock(&s_lock);
    t->in_vcpu = 0;
}

void vcpu_check_preempt(void)
{
    vcpu_thread *t = t_self;
#if defined(_WIN32)
    if (t && t->stolen && !t->in_vcpu) {   /* resumed in host code: the CPU first */
        t->in_vcpu = 1;
        pthread_mutex_lock(&s_lock);
        if (win_stolen(t) && s_holder != t)
            acquire_locked(t);
        pthread_mutex_unlock(&s_lock);
        t->in_vcpu = 0;
    }
#endif
    if (t && t->preempt && !t->in_vcpu)
        yield_now(t);
}

void vcpu_set_priority(vcpu_thread *t, int priority)
{
    if (!t)
        return;
    if (t_self) t_self->in_vcpu = 1;
    pthread_mutex_lock(&s_lock);
    t->priority = priority;
    /* A ready thread raised above the holder runs now; a holder lowered
     * below a ready thread gives way. */
    if (s_holder) {
        vcpu_thread *b = best_ready();
        if (b && b->priority > s_holder->priority)
            request_preempt(s_holder);
    }
    pthread_mutex_unlock(&s_lock);
    if (t_self) {
        t_self->in_vcpu = 0;
        vcpu_check_preempt();   /* lowered itself below a ready thread */
    }
}

void vcpu_nap_us(unsigned us)
{
#if defined(_WIN32)
#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
#define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif
    static __thread HANDLE timer;
    static __thread int tried;
    LARGE_INTEGER due;
    if (!tried) {
        tried = 1;
        timer = CreateWaitableTimerExW(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,
                                       TIMER_ALL_ACCESS);
    }
    if (!timer) {   /* (before Windows 10 1803) */
        if (us >= 1000) Sleep(us / 1000); else SwitchToThread();
        return;
    }
    due.QuadPart = -(LONGLONG)us * 10;   /* 100 ns units, relative */
    if (SetWaitableTimer(timer, &due, 0, NULL, NULL, FALSE))
        WaitForSingleObject(timer, INFINITE);
#else
    struct timespec ts = { us / 1000000, (long)(us % 1000000) * 1000 };
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR) { }
#endif
}

int vcpu_holds_cpu(void)
{
    return t_self && s_holder == t_self;
}

int vcpu_get_priority(vcpu_thread *t)
{
    return t ? t->priority : VCPU_PRIO_NORMAL;
}

#if defined(_WIN32)
/* ---- Windows: preemption by redirection --------------------------------
 *
 * No signals: the thread to preempt is suspended, and if it is in guest code
 * -- host depth 0, executing this program's own image (not a system DLL,
 * which may hold a loader or heap lock) -- its context is changed to call
 * vcpu_win_trampoline on the way, which saves every register the C ABI lets
 * a call clobber (and the x87/SSE state), yields, and returns where it was.
 * Anywhere else the flag stays set for compat_host_leave, as on POSIX. */
void vcpu_win_trampoline(void);
void vcpu_win_preempt_point(void);
__asm__(
    ".text\n"
    ".globl vcpu_win_trampoline\n"
    "vcpu_win_trampoline:\n"
    "    pushfq\n"
    "    pushq %rax\n"
    "    pushq %rcx\n"
    "    pushq %rdx\n"
    "    pushq %r8\n"
    "    pushq %r9\n"
    "    pushq %r10\n"
    "    pushq %r11\n"
    "    pushq %rbp\n"
    "    movq %rsp, %rbp\n"
    "    andq $-64, %rsp\n"
    "    subq $576, %rsp\n"            /* shadow space, then 512 for fxsave */
    "    fxsave64 64(%rsp)\n"
    "    cld\n"
    "    call vcpu_win_preempt_point\n"
    "    fxrstor64 64(%rsp)\n"
    "    movq %rbp, %rsp\n"
    "    popq %rbp\n"
    "    popq %r11\n"
    "    popq %r10\n"
    "    popq %r9\n"
    "    popq %r8\n"
    "    popq %rdx\n"
    "    popq %rcx\n"
    "    popq %rax\n"
    "    popfq\n"
    "    ret\n"
    ".globl vcpu_win_trampoline_end\n"
    "vcpu_win_trampoline_end:\n");
extern char vcpu_win_trampoline_end[];

void vcpu_win_preempt_point(void)
{
    vcpu_thread *t = t_self;
    if (!t)
        return;
    t->redirected = 0;
    vcpu_check_preempt();
    compat_park_if_pending();
}

static int in_image(DWORD64 ip)
{
    static DWORD64 lo, hi;
    if (!lo) {
        const unsigned char *base = (const unsigned char *)GetModuleHandleA(NULL);
        const IMAGE_NT_HEADERS *nt =
            (const IMAGE_NT_HEADERS *)(base + ((const IMAGE_DOS_HEADER *)base)->e_lfanew);
        hi = (DWORD64)base + nt->OptionalHeader.SizeOfImage;
        lo = (DWORD64)base;
    }
    return ip >= lo && ip < hi;
}

/* Send a suspended thread through the trampoline, if it is somewhere that
 * may yield. The return address goes on its stack, which must be committed
 * and not the guard page (touched from here, the guard would fault here). */
static int redirect(vcpu_thread *t)
{
    CONTEXT c;
    MEMORY_BASIC_INFORMATION mbi;

    if (t->in_vcpu || *t->depth != 0)
        return 0;
    memset(&c, 0, sizeof c);
    c.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
    if (!GetThreadContext(t->host, &c) || !in_image(c.Rip))
        return 0;
    if (t->redirected) {
        /* A redirect that is still pending has the thread inside the trampoline,
         * before it calls vcpu_win_preempt_point (which clears the flag). If the
         * thread is anywhere else the redirect was lost (a SetThreadContext made
         * while the thread is dispatching an exception, as every NV2A register
         * access is, is the likely way) and the flag would otherwise stay set for
         * good: the thread could then never be preempted, and holds the virtual
         * CPU against every other thread. The title's file server never runs and
         * the title reports a damaged disc after its 10 s read timeout. */
        if (c.Rip >= (DWORD64)vcpu_win_trampoline && c.Rip < (DWORD64)vcpu_win_trampoline_end)
            return 0;
        t->redirected = 0;
    }
    if (!VirtualQuery((LPCVOID)(c.Rsp - 8), &mbi, sizeof mbi) || mbi.State != MEM_COMMIT ||
        (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)))
        return 0;
    c.Rsp -= 8;
    *(DWORD64 *)c.Rsp = c.Rip;
    c.Rip = (DWORD64)vcpu_win_trampoline;
    c.ContextFlags = CONTEXT_CONTROL;
    if (!SetThreadContext(t->host, &c))
        return 0;
    t->redirected = 1;
    return 1;
}

int vcpu_win_unredirect(void *exception_pointers)
{
    EXCEPTION_POINTERS *ep = exception_pointers;
    CONTEXT *c = ep->ContextRecord;
    DWORD64 at = (DWORD64)ep->ExceptionRecord->ExceptionAddress;
    DWORD64 ret;

    if (c->Rip != (DWORD64)vcpu_win_trampoline)
        return 0;
    /* redirect() left the interrupted Rip on the stack. The fault is the guest
     * instruction's, but the redirect can land while it is being dispatched, so
     * the record may name the trampoline as both Rip and ExceptionAddress. */
    ret = *(DWORD64 *)c->Rsp;
    if (!in_image(ret) || (at != ret && at != c->Rip))
        return 0;
    c->Rip = ret;
    c->Rsp += 8;
    if (t_self)
        t_self->redirected = 0;
    return 1;
}

/* Caller holds s_lock. */
static void win_preempt(vcpu_thread *h)
{
    if (!h->host || h->in_vcpu || h->suspended)
        return;
    if (SuspendThread(h->host) == (DWORD)-1)
        return;
    redirect(h);   /* (GetThreadContext also waits for the suspension) */
    ResumeThread(h->host);
}

vcpu_thread *vcpu_win_find(unsigned long tid)
{
    vcpu_thread *found = NULL;
    for (int i = 0; i < VCPU_MAX_THREADS && !found; i++)
        if (s_threads[i] && s_threads[i]->tid == tid)
            found = s_threads[i];
    return found;
}

/* SuspendThread for any thread: a guest thread's CPU goes to the next. */
unsigned long vcpu_win_suspend_thread(void *h)
{
    DWORD tid = GetThreadId(h), prev;
    vcpu_thread *t;

    if (tid == GetCurrentThreadId()) {   /* itself: give the CPU up meanwhile */
        vcpu_block_begin();
        prev = SuspendThread(h);
        vcpu_block_end();
        return prev;
    }
    if (t_self) t_self->in_vcpu = 1;
    pthread_mutex_lock(&s_lock);
    prev = SuspendThread(h);
    t = prev == (DWORD)-1 ? NULL : vcpu_win_find(tid);
    if (t) {
        CONTEXT c;
        c.ContextFlags = CONTEXT_CONTROL;
        GetThreadContext(h, &c);   /* the suspension done */
        t->suspended++;
        if (s_holder == t) {
            t->stolen = 1;
            s_holder = NULL;
            dispatch();
        }
    }
    pthread_mutex_unlock(&s_lock);
    if (t_self) t_self->in_vcpu = 0;
    return prev;
}

/* ResumeThread: the last resume of a thread that lost the CPU sends it to
 * wait for it before it runs more guest code. */
unsigned long vcpu_win_resume_thread(void *h)
{
    DWORD tid = GetThreadId(h), prev;
    vcpu_thread *t;

    if (t_self) t_self->in_vcpu = 1;
    pthread_mutex_lock(&s_lock);
    t = vcpu_win_find(tid);
    if (t && t->suspended == 1 && t->stolen)
        redirect(t);   /* else compat_host_leave or a wait takes it back */
    prev = ResumeThread(h);
    if (t && prev != (DWORD)-1 && t->suspended > 0 && --t->suspended == 0 && !s_holder)
        dispatch();
    pthread_mutex_unlock(&s_lock);
    if (t_self) t_self->in_vcpu = 0;
    if (t_self) vcpu_check_preempt();   /* a resumed thread above this one */
    return prev;
}

#endif

#endif /* XBOX_HAS_VCPU */
