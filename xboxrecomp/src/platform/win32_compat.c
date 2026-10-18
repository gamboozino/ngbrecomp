/*
 * win32_compat.c - POSIX implementation of generic Win32 host primitives.
 *
 * See win32_compat.h. This is deliberately a *generic* OS-primitive layer
 * (threads/events/mutexes/atomics/heap/timers) -- it carries no Xbox
 * semantics. The Xbox kernel HLE in src/kernel builds on top of it.
 *
 * POSIX (Linux/MacOS) only.
 */

#if !defined(_WIN32)

/* Enable memfd_create, MAP_FIXED_NOREPLACE, timegm. Must precede all #includes. */
#define _GNU_SOURCE
/* Darwin: exposes memset_s, its explicit_bzero equivalent. */
#define __STDC_WANT_LIB_EXT1__ 1

#include "win32_compat.h"
#include "vcpu.h"

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sched.h>
#include <fenv.h>
#include <sys/mman.h>
#if defined(__APPLE__)
#include <sys/sysctl.h>
#include <sys/stat.h>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#else
#include <sys/sysinfo.h>
#endif

/* ===================================================================== */
/* Last-error (thread-local)                                             */
/* ===================================================================== */

static __thread DWORD t_last_error = 0;

DWORD GetLastError(void)            { return t_last_error; }
VOID  SetLastError(DWORD code)      { t_last_error = code; }

/* ===================================================================== */
/* Interlocked atomics                                                   */
/* ===================================================================== */

LONG InterlockedIncrement(volatile LONG *p)        { return __atomic_add_fetch(p, 1, __ATOMIC_SEQ_CST); }
LONG InterlockedDecrement(volatile LONG *p)        { return __atomic_sub_fetch(p, 1, __ATOMIC_SEQ_CST); }
LONG InterlockedExchange(volatile LONG *p, LONG v) { return __atomic_exchange_n(p, v, __ATOMIC_SEQ_CST); }
LONG InterlockedExchangeAdd(volatile LONG *p, LONG v) { return __atomic_fetch_add(p, v, __ATOMIC_SEQ_CST); }

LONG InterlockedCompareExchange(volatile LONG *p, LONG xchg, LONG cmp)
{
    __atomic_compare_exchange_n(p, &cmp, xchg, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return cmp;
}

LONGLONG InterlockedCompareExchange64(volatile LONGLONG *p, LONGLONG xchg, LONGLONG cmp)
{
    __atomic_compare_exchange_n(p, &cmp, xchg, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return cmp;
}

PVOID InterlockedCompareExchangePointer(PVOID volatile *p, PVOID xchg, PVOID cmp)
{
    __atomic_compare_exchange_n(p, &cmp, xchg, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return cmp;
}

/* ===================================================================== */
/* Critical sections (recursive pthread mutex)                           */
/* ===================================================================== */

VOID InitializeCriticalSection(LPCRITICAL_SECTION cs)
{
    memset(cs, 0, sizeof(*cs));
    pthread_mutex_t *m = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(m, &attr);
    pthread_mutexattr_destroy(&attr);
    cs->LockSemaphore = m;
}

VOID InitializeCriticalSectionAndSpinCount(LPCRITICAL_SECTION cs, DWORD spin)
{
    InitializeCriticalSection(cs);
    cs->SpinCount = spin;
}

VOID EnterCriticalSection(LPCRITICAL_SECTION cs)
{
    if (!cs->LockSemaphore) InitializeCriticalSection(cs);
    /* Contended: give the virtual CPU up while waiting, or a holder that
     * needs it to get as far as LeaveCriticalSection never would. */
    if (pthread_mutex_trylock((pthread_mutex_t *)cs->LockSemaphore) != 0) {
        vcpu_block_begin();
        pthread_mutex_lock((pthread_mutex_t *)cs->LockSemaphore);
        vcpu_block_end();
    }
    cs->RecursionCount++;
}

VOID LeaveCriticalSection(LPCRITICAL_SECTION cs)
{
    if (!cs->LockSemaphore) return;
    cs->RecursionCount--;
    pthread_mutex_unlock((pthread_mutex_t *)cs->LockSemaphore);
}

BOOL TryEnterCriticalSection(LPCRITICAL_SECTION cs)
{
    if (!cs->LockSemaphore) InitializeCriticalSection(cs);
    if (pthread_mutex_trylock((pthread_mutex_t *)cs->LockSemaphore) == 0) {
        cs->RecursionCount++;
        return TRUE;
    }
    return FALSE;
}

VOID DeleteCriticalSection(LPCRITICAL_SECTION cs)
{
    if (cs->LockSemaphore) {
        pthread_mutex_destroy((pthread_mutex_t *)cs->LockSemaphore);
        free(cs->LockSemaphore);
        cs->LockSemaphore = NULL;
    }
}

/* ===================================================================== */
/* Slim reader/writer locks                                              */
/* ===================================================================== */

/* An SRWLOCK is usable straight from SRWLOCK_INIT, so the pthread_rwlock_t
 * behind it has to appear on first use. Unlike the condition variables below
 * -- whose lazy init is covered by the caller holding the paired CRITICAL
 * SECTION -- an SRWLOCK is by definition taken from several threads at once
 * with nothing else held, so first use genuinely races. Serialise just that:
 * once Ptr is published, every acquire is a plain atomic load. */
static pthread_rwlock_t *srw_lazy_init(PSRWLOCK lock)
{
    pthread_rwlock_t *rw = __atomic_load_n((pthread_rwlock_t **)&lock->Ptr,
                                           __ATOMIC_ACQUIRE);
    if (!rw) {
        static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;
        pthread_mutex_lock(&init_lock);
        rw = (pthread_rwlock_t *)lock->Ptr;
        if (!rw) {
            rw = (pthread_rwlock_t *)malloc(sizeof(*rw));
            pthread_rwlock_init(rw, NULL);
            __atomic_store_n((pthread_rwlock_t **)&lock->Ptr, rw, __ATOMIC_RELEASE);
        }
        pthread_mutex_unlock(&init_lock);
    }
    return rw;
}

VOID InitializeSRWLock(PSRWLOCK lock)
{
    lock->Ptr = NULL;
    srw_lazy_init(lock);
}

VOID AcquireSRWLockShared(PSRWLOCK lock)     { pthread_rwlock_rdlock(srw_lazy_init(lock)); }
VOID ReleaseSRWLockShared(PSRWLOCK lock)     { pthread_rwlock_unlock(srw_lazy_init(lock)); }
VOID AcquireSRWLockExclusive(PSRWLOCK lock)  { pthread_rwlock_wrlock(srw_lazy_init(lock)); }
VOID ReleaseSRWLockExclusive(PSRWLOCK lock)  { pthread_rwlock_unlock(srw_lazy_init(lock)); }

/* ===================================================================== */
/* One-time initialisation                                               */
/* ===================================================================== */

/* Win32 semantics: the callback runs at most once for a given INIT_ONCE, and
 * a callback returning FALSE leaves it un-run so a later call retries. Ptr
 * doubles as the "done" flag. One global mutex covers every INIT_ONCE --
 * initialisation is rare, and the fast path never touches it. */
BOOL InitOnceExecuteOnce(PINIT_ONCE once, PINIT_ONCE_FN fn, PVOID param, PVOID *context)
{
    static pthread_mutex_t once_lock = PTHREAD_MUTEX_INITIALIZER;

    if (__atomic_load_n(&once->Ptr, __ATOMIC_ACQUIRE))
        return TRUE;

    pthread_mutex_lock(&once_lock);
    BOOL ok = TRUE;
    if (!once->Ptr) {
        ok = fn(once, param, context);
        if (ok)
            __atomic_store_n(&once->Ptr, (PVOID)1, __ATOMIC_RELEASE);
    }
    pthread_mutex_unlock(&once_lock);
    return ok;
}

/* ===================================================================== */
/* Condition variables (paired with a CRITICAL_SECTION)                  */
/* ===================================================================== */

/* Forward decl; the definition lives further down with the wait helpers. */
static void deadline_from_ms(DWORD ms, struct timespec *ts);

static void cv_lazy_init(PCONDITION_VARIABLE cv)
{
    if (!cv->Ptr) {
        pthread_cond_t *c = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));
        pthread_cond_init(c, NULL);
        /* Race window OK for typical Win32 usage (the CS is held). */
        cv->Ptr = c;
    }
}

VOID InitializeConditionVariable(PCONDITION_VARIABLE cv)
{
    cv->Ptr = NULL;
    cv_lazy_init(cv);
}

BOOL SleepConditionVariableCS(PCONDITION_VARIABLE cv, PCRITICAL_SECTION cs, DWORD ms)
{
    cv_lazy_init(cv);
    if (!cs->LockSemaphore) InitializeCriticalSection(cs);
    pthread_cond_t  *c = (pthread_cond_t  *)cv->Ptr;
    pthread_mutex_t *m = (pthread_mutex_t *)cs->LockSemaphore;
    if (ms == INFINITE) {
        vcpu_block_begin();
        pthread_cond_wait(c, m);
        vcpu_block_end();
        return TRUE;
    }
    struct timespec ts;
    deadline_from_ms(ms, &ts);
    vcpu_block_begin();
    int rc = pthread_cond_timedwait(c, m, &ts);
    vcpu_block_end();
    if (rc == ETIMEDOUT) { SetLastError(WAIT_TIMEOUT); return FALSE; }
    return TRUE;
}

VOID WakeConditionVariable(PCONDITION_VARIABLE cv)
{
    cv_lazy_init(cv);
    pthread_cond_signal((pthread_cond_t *)cv->Ptr);
}

VOID WakeAllConditionVariable(PCONDITION_VARIABLE cv)
{
    cv_lazy_init(cv);
    pthread_cond_broadcast((pthread_cond_t *)cv->Ptr);
}

/* ===================================================================== */
/* Waitable kernel objects                                               */
/* ===================================================================== */

typedef enum { K_EVENT, K_SEM, K_MUTEX, K_THREAD, K_TIMER, K_HEAP,
               K_FILEMAP, K_FILE, K_WAITABLE_TIMER } w32_kind;

#define W32_MAX_APC 16

/* Darwin has no unnamed semaphores; unlink named ones once opened. */
#ifdef __APPLE__
typedef sem_t *host_sem_t;
static int host_sem_init(host_sem_t *s)
{
    static unsigned seq;
    char name[32];
    snprintf(name, sizeof name, "/xbox-%x-%x", (unsigned)getpid(),
             __atomic_add_fetch(&seq, 1, __ATOMIC_RELAXED));
    *s = sem_open(name, O_CREAT | O_EXCL, 0600, 0);
    if (*s == SEM_FAILED)
        return -1;
    sem_unlink(name);
    return 0;
}
static inline int host_sem_wait(host_sem_t *s) { return sem_wait(*s); }
static inline int host_sem_post(host_sem_t *s) { return sem_post(*s); }
static inline int host_sem_destroy(host_sem_t *s) { return sem_close(*s); }
#else
typedef sem_t host_sem_t;
#define host_sem_init(s) sem_init(s, 0, 0)
#define host_sem_wait sem_wait
#define host_sem_post sem_post
#define host_sem_destroy sem_destroy
#endif

typedef struct w32_object {
    w32_kind        kind;
    LONG            refcount;
    pthread_mutex_t lock;
    pthread_cond_t  cond;

    /* event */
    int             signaled;
    int             manual_reset;

    /* semaphore */
    long            sem_count;
    long            sem_max;

    /* mutex */
    DWORD           mtx_owner;
    int             mtx_recursion;

    /* thread */
    pthread_t       thread;
    int             thread_joinable;
    DWORD           tid;
    int             exited;
    DWORD           exit_code;
    int             suspend_count;
    pthread_cond_t  gate;
    int             started;      /* past the CREATE_SUSPENDED gate */
    volatile int    parked;       /* stopped in park_now() */
    volatile int    park_pending; /* suspended while in host code */
    host_sem_t      park_sem;     /* resume wakes the parked thread */
    host_sem_t      park_ack;     /* the thread saw the suspend request */
    void           *user;         /* the thread's vcpu record (kernel) */
    LPTHREAD_START_ROUTINE start;
    LPVOID          start_param;
    int             priority;
    PAPCFUNC        apc_func[W32_MAX_APC];
    ULONG_PTR       apc_data[W32_MAX_APC];
    int             apc_count;

    /* timer-queue timer */
    int             timer_cancel;
    DWORD           timer_due;
    DWORD           timer_period;
    WAITORTIMERCALLBACK timer_cb;
    PVOID           timer_param;

    /* waitable timer */
    int             waitable_manual_reset;
    struct timespec waitable_due_time;
    int             waitable_triggered;
    int             waitable_armed;

    /* file mapping / fd-backed file handle */
    int             fd;
    SIZE_T          map_size;
    char           *file_path;
} w32_object;

/* pseudo handles for "current thread"/"current process" */
#define PSEUDO_CURRENT_PROCESS ((HANDLE)(LONG_PTR)-1)
#define PSEUDO_CURRENT_THREAD  ((HANDLE)(LONG_PTR)-2)
#define STILL_ACTIVE 259u

static __thread w32_object *t_self_obj = NULL;
static __thread DWORD       t_tid      = 0;
static __thread int         t_host_depth = 0;
static void suspend_signal_install(void);   /* with SuspendThread */

/* Live threads made by CreateThread, for compat_thread_list (diagnostics). */
#define W32_MAX_THREADS 256
static pthread_t s_threads[W32_MAX_THREADS];
static int s_thread_live[W32_MAX_THREADS];
static pthread_mutex_t s_threads_lock = PTHREAD_MUTEX_INITIALIZER;

int compat_thread_list(pthread_t *out, int max)
{
    int n = 0;
    pthread_mutex_lock(&s_threads_lock);
    for (int i = 0; i < W32_MAX_THREADS && n < max; i++)
        if (s_thread_live[i])
            out[n++] = s_threads[i];
    pthread_mutex_unlock(&s_threads_lock);
    return n;
}

static int thread_list_add(void)
{
    int slot = -1;
    pthread_mutex_lock(&s_threads_lock);
    for (int i = 0; i < W32_MAX_THREADS; i++)
        if (!s_thread_live[i]) {
            s_threads[i] = pthread_self();
            s_thread_live[i] = 1;
            slot = i;
            break;
        }
    pthread_mutex_unlock(&s_threads_lock);
    return slot;
}

static void thread_list_remove(int slot)
{
    if (slot < 0)
        return;
    pthread_mutex_lock(&s_threads_lock);
    s_thread_live[slot] = 0;
    pthread_mutex_unlock(&s_threads_lock);
}
static volatile LONG        s_next_tid = 1000;

DWORD GetCurrentThreadId(void)
{
    if (t_tid == 0)
        t_tid = (DWORD)InterlockedIncrement(&s_next_tid);
    return t_tid;
}

DWORD GetCurrentProcessId(void) { return (DWORD)getpid(); }
HANDLE GetCurrentThread(void)   { return t_self_obj ? (HANDLE)t_self_obj : PSEUDO_CURRENT_THREAD; }
HANDLE GetCurrentProcess(void)  { return PSEUDO_CURRENT_PROCESS; }

static w32_object *obj_alloc(w32_kind kind)
{
    w32_object *o = (w32_object *)calloc(1, sizeof(w32_object));
    o->kind     = kind;
    o->refcount = 1;
    pthread_mutex_init(&o->lock, NULL);
    pthread_cond_init(&o->cond, NULL);
    pthread_cond_init(&o->gate, NULL);
    return o;
}

static void obj_release(w32_object *o)
{
    if (InterlockedDecrement(&o->refcount) > 0)
        return;
    if (o->kind == K_FILE) {
        if (o->fd >= 0) close(o->fd);
        free(o->file_path);
    } else if (o->kind == K_FILEMAP) {
        if (o->fd >= 0) close(o->fd);
    } else if (o->kind == K_THREAD) {
        host_sem_destroy(&o->park_sem);
        host_sem_destroy(&o->park_ack);
    } else if (o->kind == K_WAITABLE_TIMER) {
        /* Waitable timers: no special cleanup needed */
    }
    pthread_mutex_destroy(&o->lock);
    pthread_cond_destroy(&o->cond);
    pthread_cond_destroy(&o->gate);
    free(o);
}

/* ---- fd-backed file handle (for the file-I/O HLE) -------------------- */
HANDLE w32_open_handle(int fd, const char *host_path)
{
    w32_object *o = obj_alloc(K_FILE);
    o->fd        = fd;
    o->file_path = host_path ? strdup(host_path) : NULL;
    return (HANDLE)o;
}

int w32_handle_fd(HANDLE h)
{
    w32_object *o = (w32_object *)h;
    return (o && o->kind == K_FILE) ? o->fd : -1;
}

const char *w32_handle_path(HANDLE h)
{
    w32_object *o = (w32_object *)h;
    return (o && o->kind == K_FILE) ? o->file_path : NULL;
}

BOOL CloseHandle(HANDLE h)
{
    if (!h || h == PSEUDO_CURRENT_THREAD || h == PSEUDO_CURRENT_PROCESS ||
        h == INVALID_HANDLE_VALUE)
        return TRUE;
    obj_release((w32_object *)h);
    return TRUE;
}

BOOL DuplicateHandle(HANDLE srcProc, HANDLE src, HANDLE dstProc, PHANDLE dst,
                     DWORD access, BOOL inherit, DWORD options)
{
    (void)srcProc; (void)dstProc; (void)access; (void)inherit;
    if (!dst) return FALSE;
    if (src == PSEUDO_CURRENT_THREAD)  src = GetCurrentThread();
    if (src == PSEUDO_CURRENT_PROCESS) { *dst = src; return TRUE; }
    if (src == PSEUDO_CURRENT_THREAD || !src) { *dst = src; return TRUE; }
    w32_object *o = (w32_object *)src;
    InterlockedIncrement(&o->refcount);
    *dst = src;
    if (options & DUPLICATE_CLOSE_SOURCE)
        obj_release(o);
    return TRUE;
}

/* ---- deadline helper -------------------------------------------------- */
static void deadline_from_ms(DWORD ms, struct timespec *ts)
{
    clock_gettime(CLOCK_REALTIME, ts);
    ts->tv_sec  += ms / 1000;
    ts->tv_nsec += (long)(ms % 1000) * 1000000L;
    if (ts->tv_nsec >= 1000000000L) { ts->tv_sec++; ts->tv_nsec -= 1000000000L; }
}

/* Run any pending user-APCs for the calling thread. Returns count run. */
static int drain_apcs(void)
{
    w32_object *o = t_self_obj;
    int run = 0;
    if (!o) return 0;
    pthread_mutex_lock(&o->lock);
    while (o->apc_count > 0) {
        PAPCFUNC  f = o->apc_func[0];
        ULONG_PTR d = o->apc_data[0];
        memmove(o->apc_func, o->apc_func + 1, sizeof(PAPCFUNC) * (o->apc_count - 1));
        memmove(o->apc_data, o->apc_data + 1, sizeof(ULONG_PTR) * (o->apc_count - 1));
        o->apc_count--;
        pthread_mutex_unlock(&o->lock);
        f(d);
        run++;
        pthread_mutex_lock(&o->lock);
    }
    pthread_mutex_unlock(&o->lock);
    return run;
}

static int timespec_before(const struct timespec *a, const struct timespec *b)
{
    return a->tv_sec != b->tv_sec ? a->tv_sec < b->tv_sec : a->tv_nsec < b->tv_nsec;
}

/* Signalled once the due time passes; latched, so a manual-reset timer stays
 * signalled until it is set or cancelled again. Caller holds o->lock. */
static int waitable_due(w32_object *o)
{
    struct timespec now;
    if (o->waitable_triggered) return 1;
    if (!o->waitable_armed) return 0;
    clock_gettime(CLOCK_REALTIME, &now);
    if (timespec_before(&now, &o->waitable_due_time)) return 0;
    o->waitable_triggered = 1;
    return 1;
}

/*
 * Wait on a single object. The object lock must NOT be held.
 * Returns WAIT_OBJECT_0 / WAIT_TIMEOUT.
 */
static DWORD wait_single(w32_object *o, DWORD ms)
{
    struct timespec ts;
    int timed = (ms != INFINITE);
    if (timed) deadline_from_ms(ms, &ts);

    pthread_mutex_lock(&o->lock);
    DWORD result = WAIT_OBJECT_0;

    for (;;) {
        int ready = 0;
        switch (o->kind) {
        case K_EVENT:  ready = o->signaled; break;
        case K_THREAD: ready = o->exited;   break;
        case K_SEM:    ready = (o->sem_count > 0); break;
        case K_MUTEX:
            ready = (o->mtx_owner == 0 || o->mtx_owner == GetCurrentThreadId());
            break;
        case K_WAITABLE_TIMER: ready = waitable_due(o); break;
        default:       ready = 1; break;
        }
        if (ready) break;

        /* An armed timer has its own deadline. Waiting on the caller's alone
         * would sleep straight past the due time, so take whichever comes
         * first and re-test. */
        struct timespec until = ts;
        int bounded = timed;
        if (o->kind == K_WAITABLE_TIMER && o->waitable_armed &&
            (!timed || timespec_before(&o->waitable_due_time, &ts))) {
            until = o->waitable_due_time;
            bounded = 1;
        }
        int rc = bounded ? pthread_cond_timedwait(&o->cond, &o->lock, &until)
                         : pthread_cond_wait(&o->cond, &o->lock);
        if (rc == ETIMEDOUT && timed && !timespec_before(&until, &ts)) {
            result = WAIT_TIMEOUT; break;
        }
    }

    if (result == WAIT_OBJECT_0) {
        switch (o->kind) {
        case K_EVENT: if (!o->manual_reset) o->signaled = 0; break;
        case K_WAITABLE_TIMER:
            if (!o->waitable_manual_reset) { o->waitable_triggered = 0; o->waitable_armed = 0; }
            break;
        case K_SEM:   o->sem_count--; break;
        case K_MUTEX: o->mtx_owner = GetCurrentThreadId(); o->mtx_recursion++; break;
        default: break;
        }
    }
    pthread_mutex_unlock(&o->lock);
    return result;
}

DWORD WaitForSingleObject(HANDLE h, DWORD ms)
{
    DWORD r;
    if (!h || h == PSEUDO_CURRENT_THREAD || h == PSEUDO_CURRENT_PROCESS)
        return WAIT_OBJECT_0;
    /* Already satisfied: no reason to give the CPU up. */
    r = wait_single((w32_object *)h, 0);
    if (r != WAIT_TIMEOUT || ms == 0)
        return r;
    vcpu_block_begin();
    r = wait_single((w32_object *)h, ms);
    vcpu_block_end();
    return r;
}

DWORD WaitForSingleObjectEx(HANDLE h, DWORD ms, BOOL alertable)
{
    if (alertable && drain_apcs() > 0)
        return WAIT_IO_COMPLETION;
    return WaitForSingleObject(h, ms);
}

/*
 * WaitForMultipleObjects: polling implementation. Adequate for the light
 * multi-object waits the Xbox kernel HLE issues; not a high-throughput path.
 */
DWORD WaitForMultipleObjects(DWORD count, const HANDLE *handles, BOOL waitAll, DWORD ms)
{
    return WaitForMultipleObjectsEx(count, handles, waitAll, ms, FALSE);
}

static DWORD wfmo_inner(DWORD count, const HANDLE *handles, BOOL waitAll,
                        DWORD ms, BOOL alertable);

DWORD WaitForMultipleObjectsEx(DWORD count, const HANDLE *handles, BOOL waitAll,
                               DWORD ms, BOOL alertable)
{
    DWORD r = wfmo_inner(count, handles, waitAll, 0, alertable);
    if (r != WAIT_TIMEOUT || ms == 0)
        return r;
    vcpu_block_begin();
    r = wfmo_inner(count, handles, waitAll, ms, alertable);
    vcpu_block_end();
    return r;
}

static DWORD wfmo_inner(DWORD count, const HANDLE *handles, BOOL waitAll,
                        DWORD ms, BOOL alertable)
{
    struct timespec ts;
    int timed = (ms != INFINITE);
    if (timed) deadline_from_ms(ms, &ts);

    for (;;) {
        if (alertable && drain_apcs() > 0)
            return WAIT_IO_COMPLETION;

        if (waitAll) {
            DWORD got = 0;
            for (DWORD i = 0; i < count; i++)
                if (WaitForSingleObject(handles[i], 0) == WAIT_OBJECT_0) got++;
            if (got == count) return WAIT_OBJECT_0;
        } else {
            for (DWORD i = 0; i < count; i++)
                if (WaitForSingleObject(handles[i], 0) == WAIT_OBJECT_0)
                    return WAIT_OBJECT_0 + i;
        }

        if (timed) {
            struct timespec now;
            clock_gettime(CLOCK_REALTIME, &now);
            if (now.tv_sec > ts.tv_sec ||
                (now.tv_sec == ts.tv_sec && now.tv_nsec >= ts.tv_nsec))
                return WAIT_TIMEOUT;
        }
        usleep(1000);
    }
}

/* ===================================================================== */
/* Events                                                                */
/* ===================================================================== */

HANDLE CreateEventA(LPSECURITY_ATTRIBUTES sa, BOOL manualReset, BOOL initialState, LPCSTR name)
{
    (void)sa; (void)name;
    w32_object *o = obj_alloc(K_EVENT);
    o->manual_reset = manualReset ? 1 : 0;
    o->signaled     = initialState ? 1 : 0;
    return (HANDLE)o;
}
HANDLE CreateEventW(LPSECURITY_ATTRIBUTES sa, BOOL manualReset, BOOL initialState, LPCWSTR name)
{
    (void)name;
    return CreateEventA(sa, manualReset, initialState, NULL);
}

BOOL SetEvent(HANDLE h)
{
    w32_object *o = (w32_object *)h;
    if (!o || o->kind != K_EVENT) return FALSE;
    pthread_mutex_lock(&o->lock);
    o->signaled = 1;
    pthread_cond_broadcast(&o->cond);
    pthread_mutex_unlock(&o->lock);
    return TRUE;
}

BOOL ResetEvent(HANDLE h)
{
    w32_object *o = (w32_object *)h;
    if (!o || o->kind != K_EVENT) return FALSE;
    pthread_mutex_lock(&o->lock);
    o->signaled = 0;
    pthread_mutex_unlock(&o->lock);
    return TRUE;
}

BOOL PulseEvent(HANDLE h)
{
    w32_object *o = (w32_object *)h;
    if (!o || o->kind != K_EVENT) return FALSE;
    pthread_mutex_lock(&o->lock);
    o->signaled = 1;
    pthread_cond_broadcast(&o->cond);
    o->signaled = 0;
    pthread_mutex_unlock(&o->lock);
    return TRUE;
}

/* ===================================================================== */
/* Semaphores                                                            */
/* ===================================================================== */

HANDLE CreateSemaphoreA(LPSECURITY_ATTRIBUTES sa, LONG initial, LONG maximum, LPCSTR name)
{
    (void)sa; (void)name;
    w32_object *o = obj_alloc(K_SEM);
    o->sem_count = initial;
    o->sem_max   = maximum;
    return (HANDLE)o;
}
HANDLE CreateSemaphoreW(LPSECURITY_ATTRIBUTES sa, LONG initial, LONG maximum, LPCWSTR name)
{
    (void)name;
    return CreateSemaphoreA(sa, initial, maximum, NULL);
}

BOOL ReleaseSemaphore(HANDLE h, LONG releaseCount, PLONG previousCount)
{
    w32_object *o = (w32_object *)h;
    if (!o || o->kind != K_SEM) return FALSE;
    pthread_mutex_lock(&o->lock);
    if (previousCount) *previousCount = (LONG)o->sem_count;
    o->sem_count += releaseCount;
    if (o->sem_max && o->sem_count > o->sem_max) o->sem_count = o->sem_max;
    pthread_cond_broadcast(&o->cond);
    pthread_mutex_unlock(&o->lock);
    return TRUE;
}

/* ===================================================================== */
/* Mutexes                                                               */
/* ===================================================================== */

HANDLE CreateMutexA(LPSECURITY_ATTRIBUTES sa, BOOL initialOwner, LPCSTR name)
{
    (void)sa; (void)name;
    w32_object *o = obj_alloc(K_MUTEX);
    if (initialOwner) { o->mtx_owner = GetCurrentThreadId(); o->mtx_recursion = 1; }
    return (HANDLE)o;
}
HANDLE CreateMutexW(LPSECURITY_ATTRIBUTES sa, BOOL initialOwner, LPCWSTR name)
{
    (void)name;
    return CreateMutexA(sa, initialOwner, NULL);
}

BOOL ReleaseMutex(HANDLE h)
{
    w32_object *o = (w32_object *)h;
    if (!o || o->kind != K_MUTEX) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    pthread_mutex_lock(&o->lock);
    if (o->mtx_owner != GetCurrentThreadId()) {
        pthread_mutex_unlock(&o->lock);
        SetLastError(ERROR_NOT_OWNER);
        return FALSE;
    }
    if (--o->mtx_recursion <= 0) {
        o->mtx_owner = 0;
        o->mtx_recursion = 0;
        pthread_cond_broadcast(&o->cond);
    }
    pthread_mutex_unlock(&o->lock);
    return TRUE;
}

/* ===================================================================== */
/* Threads                                                               */
/* ===================================================================== */

static void *thread_trampoline(void *arg)
{
    w32_object *o = (w32_object *)arg;
    t_self_obj = o;
    t_tid      = o->tid;

    /* CREATE_SUSPENDED gate */
    pthread_mutex_lock(&o->lock);
    while (o->suspend_count > 0)
        pthread_cond_wait(&o->gate, &o->lock);
    o->started = 1;
    pthread_mutex_unlock(&o->lock);

    int list_slot = thread_list_add();
    DWORD rc = o->start ? o->start(o->start_param) : 0;
    thread_list_remove(list_slot);
    vcpu_detach();

    pthread_mutex_lock(&o->lock);
    o->exit_code = rc;
    o->exited    = 1;
    o->signaled  = 1;
    pthread_cond_broadcast(&o->cond);
    pthread_mutex_unlock(&o->lock);

    obj_release(o);   /* drop the trampoline's reference */
    return NULL;
}

HANDLE CreateThread(LPSECURITY_ATTRIBUTES sa, SIZE_T stackSize,
                    LPTHREAD_START_ROUTINE start, LPVOID param,
                    DWORD flags, LPDWORD threadId)
{
    (void)sa;
    w32_object *o = obj_alloc(K_THREAD);
    if (host_sem_init(&o->park_sem) || host_sem_init(&o->park_ack)) {
        fprintf(stderr, "Cannot create thread suspension semaphores\n");
        abort();
    }
    suspend_signal_install();
    o->start         = start;
    o->start_param   = param;
    o->tid           = (DWORD)InterlockedIncrement(&s_next_tid);
    o->suspend_count = (flags & CREATE_SUSPENDED) ? 1 : 0;
    o->refcount      = 2;   /* one for caller, one for the trampoline */

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (stackSize)
        pthread_attr_setstacksize(&attr, stackSize < 65536 ? 65536 : stackSize);

    if (pthread_create(&o->thread, &attr, thread_trampoline, o) != 0) {
        pthread_attr_destroy(&attr);
        o->refcount = 1;
        obj_release(o);
        SetLastError(8 /* ERROR_NOT_ENOUGH_MEMORY */);
        return NULL;
    }
    pthread_attr_destroy(&attr);
    o->thread_joinable = 1;

    if (threadId) *threadId = o->tid;
    return (HANDLE)o;
}

VOID ExitThread(DWORD exitCode)
{
    vcpu_detach();
    w32_object *o = t_self_obj;
    if (o) {
        pthread_mutex_lock(&o->lock);
        o->exit_code = exitCode;
        o->exited    = 1;
        o->signaled  = 1;
        pthread_cond_broadcast(&o->cond);
        pthread_mutex_unlock(&o->lock);
        obj_release(o);
    }
    pthread_exit(NULL);
}

BOOL GetExitCodeThread(HANDLE h, LPDWORD exitCode)
{
    w32_object *o = (w32_object *)h;
    if (!o || o->kind != K_THREAD || !exitCode) return FALSE;
    pthread_mutex_lock(&o->lock);
    *exitCode = o->exited ? o->exit_code : STILL_ACTIVE;
    pthread_mutex_unlock(&o->lock);
    return TRUE;
}

/* ---- Suspension (see compat_host_enter in win32_compat.h) ----
 *
 * A suspend request is a signal to the target. Its handler either parks the
 * thread on the spot -- it was running guest code, which holds no runtime
 * locks -- or, inside host code, leaves a note that compat_host_leave acts
 * on. Either way it acknowledges, so SuspendThread returns knowing the
 * target will run no more guest code. Parking waits on a semaphore: both it
 * and the acknowledgement are async-signal-safe, which a mutex is not. */
#ifdef __APPLE__
#define SIG_SUSPEND SIGURG
#else
#define SIG_SUSPEND (SIGRTMIN + 3)
#endif

static void park_now(w32_object *o)
{
    o->park_pending = 0;
    __atomic_store_n(&o->parked, 1, __ATOMIC_SEQ_CST);
    vcpu_block_begin();
    while (__atomic_load_n(&o->suspend_count, __ATOMIC_SEQ_CST) > 0)
        while (host_sem_wait(&o->park_sem) != 0 && errno == EINTR)
            ;
    __atomic_store_n(&o->parked, 0, __ATOMIC_SEQ_CST);
    vcpu_block_end();
}

static void suspend_signal(int sig)
{
    w32_object *o = t_self_obj;
    int saved_errno = errno;

    (void)sig;
    if (o) {
        /* Inside host code or the scheduler: park at the next safe point. */
        if (t_host_depth > 0 || vcpu_busy()) {
            o->park_pending = 1;
            host_sem_post(&o->park_ack);
        } else {
            host_sem_post(&o->park_ack);
            park_now(o);
        }
    }
    errno = saved_errno;
}

static void suspend_signal_install_once(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = suspend_signal;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    sigaction(SIG_SUSPEND, &sa, NULL);
}

static void suspend_signal_install(void)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    pthread_once(&once, suspend_signal_install_once);
}

void compat_host_enter(void)
{
    t_host_depth++;
}

void compat_host_leave(void)
{
    if (--t_host_depth == 0) {
        vcpu_check_preempt();
        compat_park_if_pending();
    }
}

void compat_park_if_pending(void)
{
    if (t_host_depth == 0 && t_self_obj && t_self_obj->park_pending)
        park_now(t_self_obj);
}

int compat_host_depth(void)
{
    return t_host_depth;
}

void compat_thread_set_user(void *user)
{
    if (t_self_obj)
        t_self_obj->user = user;
}

void *compat_thread_user(HANDLE h)
{
    w32_object *o = (w32_object *)h;
    if (h == PSEUDO_CURRENT_THREAD)
        o = t_self_obj;
    return (o && o->kind == K_THREAD) ? o->user : NULL;
}

int compat_is_thread(HANDLE h)
{
    w32_object *o = (w32_object *)h;
    return h == PSEUDO_CURRENT_THREAD || (o && o->kind == K_THREAD);
}

int compat_guest_enter(void)
{
    int depth = t_host_depth;
    t_host_depth = 0;
    if (t_self_obj && t_self_obj->park_pending)
        park_now(t_self_obj);
    return depth;
}

void compat_guest_leave(int saved_depth)
{
    t_host_depth = saved_depth;
}

DWORD ResumeThread(HANDLE h)
{
    w32_object *o = (w32_object *)h;
    if (!o || o->kind != K_THREAD) return (DWORD)-1;
    pthread_mutex_lock(&o->lock);
    DWORD prev = (DWORD)o->suspend_count;
    if (o->suspend_count > 0 &&
        __atomic_sub_fetch(&o->suspend_count, 1, __ATOMIC_SEQ_CST) == 0) {
        pthread_cond_broadcast(&o->gate);
        o->park_pending = 0;
        if (__atomic_load_n(&o->parked, __ATOMIC_SEQ_CST))
            host_sem_post(&o->park_sem);
    }
    pthread_mutex_unlock(&o->lock);
    return prev;
}

DWORD SuspendThread(HANDLE h)
{
    w32_object *o = (w32_object *)h;
    if (h == PSEUDO_CURRENT_THREAD)
        o = t_self_obj;
    if (!o || o->kind != K_THREAD) return (DWORD)-1;
    pthread_mutex_lock(&o->lock);
    DWORD prev = (DWORD)__atomic_fetch_add(&o->suspend_count, 1, __ATOMIC_SEQ_CST);
    int running = o->started && !o->exited;
    pthread_mutex_unlock(&o->lock);

    if (prev == 0 && running) {
        if (o == t_self_obj) {
            /* Suspending oneself takes effect on the way back to guest code. */
            if (t_host_depth > 0)
                o->park_pending = 1;
            else
                park_now(o);
        } else {
            suspend_signal_install();
            if (pthread_kill(o->thread, SIG_SUSPEND) == 0)
                while (host_sem_wait(&o->park_ack) != 0 && errno == EINTR)
                    ;
        }
    }
    return prev;
}

BOOL TerminateThread(HANDLE h, DWORD exitCode)
{
    w32_object *o = (w32_object *)h;
    if (!o || o->kind != K_THREAD) return FALSE;
    pthread_cancel(o->thread);
    pthread_mutex_lock(&o->lock);
    o->exit_code = exitCode;
    o->exited    = 1;
    o->signaled  = 1;
    pthread_cond_broadcast(&o->cond);
    pthread_mutex_unlock(&o->lock);
    return TRUE;
}

BOOL SetThreadPriority(HANDLE h, int priority)
{
    w32_object *o = (h == PSEUDO_CURRENT_THREAD) ? t_self_obj : (w32_object *)h;
    if (o && o->kind == K_THREAD) o->priority = priority;
    return TRUE;   /* real RT priorities need privileges; tracked only */
}

int GetThreadPriority(HANDLE h)
{
    w32_object *o = (h == PSEUDO_CURRENT_THREAD) ? t_self_obj : (w32_object *)h;
    return (o && o->kind == K_THREAD) ? o->priority : THREAD_PRIORITY_NORMAL;
}

VOID SwitchToThread(void) { if (vcpu_self()) vcpu_yield_any(); else sched_yield(); }

DWORD QueueUserAPC(PAPCFUNC func, HANDLE thread, ULONG_PTR data)
{
    w32_object *o = (thread == PSEUDO_CURRENT_THREAD) ? t_self_obj : (w32_object *)thread;
    if (!o || o->kind != K_THREAD) return 0;
    pthread_mutex_lock(&o->lock);
    DWORD ok = 0;
    if (o->apc_count < W32_MAX_APC) {
        o->apc_func[o->apc_count] = func;
        o->apc_data[o->apc_count] = data;
        o->apc_count++;
        ok = 1;
    }
    pthread_mutex_unlock(&o->lock);
    return ok;
}

/* ===================================================================== */
/* Sleep                                                                 */
/* ===================================================================== */

VOID Sleep(DWORD ms)
{
    if (ms == 0) {
        if (vcpu_self()) vcpu_yield(); else sched_yield();
        return;
    }
    struct timespec ts = { ms / 1000, (long)(ms % 1000) * 1000000L };
    vcpu_block_begin();
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR) { }
    vcpu_block_end();
}

DWORD SleepEx(DWORD ms, BOOL alertable)
{
    if (alertable && drain_apcs() > 0)
        return WAIT_IO_COMPLETION;
    Sleep(ms);
    if (alertable && drain_apcs() > 0)
        return WAIT_IO_COMPLETION;
    return 0;
}

/* ===================================================================== */
/* Timer queues (one helper thread per timer)                            */
/* ===================================================================== */

static void *timer_thread(void *arg)
{
    w32_object *o = (w32_object *)arg;
    int once = (o->timer_period == 0);

    /* initial due time */
    if (o->timer_due) Sleep(o->timer_due);
    if (!o->timer_cancel && o->timer_cb)
        o->timer_cb(o->timer_param, TRUE);

    while (!once && !o->timer_cancel) {
        Sleep(o->timer_period);
        if (o->timer_cancel) break;
        if (o->timer_cb) o->timer_cb(o->timer_param, TRUE);
    }
    obj_release(o);
    return NULL;
}

HANDLE CreateTimerQueue(void)
{
    /* A timer queue is just a grouping token here. */
    return (HANDLE)obj_alloc(K_TIMER);
}

BOOL DeleteTimerQueue(HANDLE timerQueue)
{
    return CloseHandle(timerQueue);
}

BOOL CreateTimerQueueTimer(PHANDLE newTimer, HANDLE timerQueue,
                           WAITORTIMERCALLBACK callback, PVOID param,
                           DWORD dueTime, DWORD period, ULONG flags)
{
    (void)timerQueue;
    if (flags & WT_EXECUTEONLYONCE) period = 0;
    w32_object *o = obj_alloc(K_TIMER);
    o->timer_cb     = callback;
    o->timer_param  = param;
    o->timer_due    = dueTime;
    o->timer_period = period;
    o->refcount     = 2;   /* caller + timer thread */

    if (pthread_create(&o->thread, NULL, timer_thread, o) != 0) {
        o->refcount = 1;
        obj_release(o);
        return FALSE;
    }
    pthread_detach(o->thread);
    if (newTimer) *newTimer = (HANDLE)o;
    return TRUE;
}

BOOL ChangeTimerQueueTimer(HANDLE timerQueue, HANDLE timer, ULONG dueTime, ULONG period)
{
    (void)timerQueue;
    w32_object *o = (w32_object *)timer;
    if (!o || o->kind != K_TIMER) return FALSE;
    o->timer_due    = dueTime;
    o->timer_period = period;
    return TRUE;
}

BOOL DeleteTimerQueueTimer(HANDLE timerQueue, HANDLE timer, HANDLE completionEvent)
{
    (void)timerQueue;
    w32_object *o = (w32_object *)timer;
    if (!o || o->kind != K_TIMER) return FALSE;
    o->timer_cancel = 1;
    if (completionEvent) SetEvent(completionEvent);
    obj_release(o);
    return TRUE;
}

struct w32_tp_args { PTP_SIMPLE_CALLBACK cb; PVOID ctx; };

static void *w32_tp_trampoline(void *arg)
{
    struct w32_tp_args *a = (struct w32_tp_args *)arg;
    a->cb(NULL, a->ctx);
    free(a);
    return NULL;
}

BOOL TrySubmitThreadpoolCallback(PTP_SIMPLE_CALLBACK callback,
                                 PVOID context, PVOID env)
{
    (void)env;
    /* Run on a throwaway detached thread. */
    struct w32_tp_args *a = (struct w32_tp_args *)malloc(sizeof(*a));
    a->cb = callback; a->ctx = context;
    pthread_t th;
    if (pthread_create(&th, NULL, w32_tp_trampoline, a) != 0) { free(a); return FALSE; }
    pthread_detach(th);
    return TRUE;
}

/* ===================================================================== */
/* Waitable timers                                                       */
/* ===================================================================== */

HANDLE CreateWaitableTimerW(LPSECURITY_ATTRIBUTES sa, BOOL manualReset, LPCWSTR name)
{
    (void)sa;
    (void)name;
    w32_object *o = obj_alloc(K_WAITABLE_TIMER);
    o->waitable_manual_reset = manualReset;
    return (HANDLE)o;
}

BOOL SetWaitableTimer(HANDLE h, const LARGE_INTEGER *dueTime, LONG period,
                      PTIMERAPCROUTINE completion, PVOID arg, BOOL resume)
{
    w32_object *o = (w32_object *)h;
    (void)completion; (void)arg; (void)resume;
    if (!o || o->kind != K_WAITABLE_TIMER || !dueTime) return FALSE;
    pthread_mutex_lock(&o->lock);
    /* Win32 100ns units: negative is relative to now, positive is an absolute
     * FILETIME. ponytail: period is ignored -- one-shot only, revisit if a
     * title actually arms a repeating timer. */
    if (dueTime->QuadPart <= 0) {
        clock_gettime(CLOCK_REALTIME, &o->waitable_due_time);
        LONGLONG ns = -dueTime->QuadPart * 100LL;
        o->waitable_due_time.tv_sec  += (time_t)(ns / 1000000000LL);
        o->waitable_due_time.tv_nsec += (long)(ns % 1000000000LL);
        if (o->waitable_due_time.tv_nsec >= 1000000000L) {
            o->waitable_due_time.tv_sec++;
            o->waitable_due_time.tv_nsec -= 1000000000L;
        }
    } else {
        /* FILETIME epoch is 1601-01-01; Unix is 1970-01-01. */
        LONGLONG unix100ns = dueTime->QuadPart - 116444736000000000LL;
        o->waitable_due_time.tv_sec  = (time_t)(unix100ns / 10000000LL);
        o->waitable_due_time.tv_nsec = (long)((unix100ns % 10000000LL) * 100LL);
    }
    (void)period;
    o->waitable_armed = 1;
    o->waitable_triggered = 0;
    pthread_mutex_unlock(&o->lock);
    pthread_cond_broadcast(&o->cond);
    return TRUE;
}

BOOL CancelWaitableTimer(HANDLE h)
{
    w32_object *o = (w32_object *)h;
    if (!o || o->kind != K_WAITABLE_TIMER) return FALSE;
    pthread_mutex_lock(&o->lock);
    o->waitable_triggered = 0;
    o->waitable_armed = 0;
    pthread_mutex_unlock(&o->lock);
    return TRUE;
}

/* ===================================================================== */
/* Heap (thin wrapper over malloc; the single process heap)              */
/* ===================================================================== */

static w32_object s_process_heap = { .kind = K_HEAP };

HANDLE GetProcessHeap(void)                       { return (HANDLE)&s_process_heap; }
HANDLE HeapCreate(DWORD o, SIZE_T i, SIZE_T m)    { (void)o;(void)i;(void)m; return (HANDLE)&s_process_heap; }
BOOL   HeapDestroy(HANDLE h)                      { (void)h; return TRUE; }

LPVOID HeapAlloc(HANDLE heap, DWORD flags, SIZE_T bytes)
{
    (void)heap;
    return (flags & HEAP_ZERO_MEMORY) ? calloc(1, bytes ? bytes : 1)
                                      : malloc(bytes ? bytes : 1);
}
LPVOID HeapReAlloc(HANDLE heap, DWORD flags, LPVOID mem, SIZE_T bytes)
{
    (void)heap; (void)flags;
    return realloc(mem, bytes ? bytes : 1);
}
BOOL HeapFree(HANDLE heap, DWORD flags, LPVOID mem)
{
    (void)heap; (void)flags;
    free(mem);
    return TRUE;
}
SIZE_T HeapSize(HANDLE heap, DWORD flags, LPCVOID mem)
{
    (void)heap; (void)flags; (void)mem;
    return 0;   /* glibc malloc_usable_size could be used; not needed yet */
}

/* ===================================================================== */
/* Virtual memory                                                        */
/* ===================================================================== */

static int prot_from_page(DWORD protect)
{
    switch (protect & 0xFF) {
    case PAGE_NOACCESS:          return PROT_NONE;
    case PAGE_READONLY:          return PROT_READ;
    case PAGE_READWRITE:         return PROT_READ | PROT_WRITE;
    case PAGE_EXECUTE:           return PROT_EXEC;
    case PAGE_EXECUTE_READ:      return PROT_READ | PROT_EXEC;
    case PAGE_EXECUTE_READWRITE: return PROT_READ | PROT_WRITE | PROT_EXEC;
    default:                     return PROT_READ | PROT_WRITE;
    }
}

#if defined(__APPLE__)
/* Darwin has no MAP_FIXED_NOREPLACE, and the two mmap options are both wrong
 * for VirtualAlloc: MAP_FIXED silently unmaps whatever already occupies the
 * range, and a bare address hint can be relocated by the kernel for reasons
 * other than the range being taken -- so "we got a different address" is only
 * an approximation of "it was occupied", and a racy one.
 *
 * mach_vm_map with VM_FLAGS_FIXED is the exact primitive: it maps at the
 * address given, and returns KERN_NO_SPACE rather than displacing an existing
 * mapping. That is what Win32 promises, and this layer exists to keep the Xbox
 * HLE above it honest -- a VirtualAlloc that quietly replaced a live mapping
 * would corrupt whatever held it, far from the call that did it.
 *
 * Memory from mach_vm_map is released by munmap like any other, because the
 * BSD and Mach halves of Darwin share one VM map, so VirtualFree is unchanged.
 */
/* Length registry, defined with the view helpers below. Win32 frees by address
 * alone -- UnmapViewOfFile takes no length and VirtualFree(MEM_RELEASE) is
 * documented to take size 0 -- so the length has to be recoverable here or
 * munmap cannot be called at all. */
void view_register(void *addr, size_t len);
size_t view_take(const void *addr);

static void *mach_map_fixed(void *address, size_t size, int prot)
{
    mach_vm_address_t addr = (mach_vm_address_t)(uintptr_t)address;
    mach_vm_size_t len = (size + vm_page_size - 1) & ~((mach_vm_size_t)vm_page_size - 1);
    vm_prot_t vmprot = VM_PROT_NONE;

    if (prot & PROT_READ)  vmprot |= VM_PROT_READ;
    if (prot & PROT_WRITE) vmprot |= VM_PROT_WRITE;
    if (prot & PROT_EXEC)  vmprot |= VM_PROT_EXECUTE;

    kern_return_t kr = mach_vm_map(
        mach_task_self(),
        &addr,
        len,
        0,
        VM_FLAGS_FIXED,
        MEMORY_OBJECT_NULL,
        0,
        FALSE,
        vmprot,
        VM_PROT_ALL,
        VM_INHERIT_DEFAULT
    );
    if (kr != KERN_SUCCESS) {
        return MAP_FAILED;
    }
    return (void *)(uintptr_t)addr;
}
#endif

/* Mapping-length registry, defined with the file-mapping views below. */
void   view_register(void *addr, size_t len);
size_t view_take(const void *addr);
static int view_overlaps(const void *addr, size_t len);

/* The placeholder window (compat_reserve_window). */
static uintptr_t s_window_lo, s_window_hi;

static int in_window(const void *addr, size_t len)
{
    uintptr_t a = (uintptr_t)addr;
    return s_window_hi && a >= s_window_lo && a <= s_window_hi && len <= s_window_hi - a;
}

/* Put the placeholder back over [addr, addr+len): inaccessible, uncommitted,
 * and still ours, so no host allocation can be handed the range. */
static int window_restore(void *addr, size_t len)
{
    return mmap(addr, len, PROT_NONE,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_FIXED,
                -1, 0) == addr;
}

LPVOID compat_reserve_window(SIZE_T size, SIZE_T align)
{
    size_t span = size + align;
    uintptr_t raw, base;
    void *p;

    if (s_window_hi || !size || (align & (align - 1)))
        return NULL;
    p = mmap(NULL, span, PROT_NONE,
             MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (p == MAP_FAILED)
        return NULL;
    raw = (uintptr_t)p;
    base = align ? (raw + align - 1) & ~(uintptr_t)(align - 1) : raw;
    /* Trim the alignment slack so only the window itself stays reserved. */
    if (base > raw)
        munmap(p, base - raw);
    if (raw + span > base + size)
        munmap((void *)(base + size), raw + span - (base + size));
    s_window_lo = base;
    s_window_hi = base + size;
    return (LPVOID)base;
}

/* Told when a protection change makes memory writable, so a write-protect
 * scheme laid over that memory (the NV2A dirty tracker) can learn its pages
 * no longer fault. */
void (*win32_ProtectHook)(void *address, size_t size);

LPVOID VirtualAlloc(LPVOID address, SIZE_T size, DWORD allocationType, DWORD protect)
{
    int prot  = prot_from_page(protect);
    int flags = MAP_PRIVATE | MAP_ANONYMOUS;

    /* MEM_COMMIT on a region already reserved by a prior VirtualAlloc:
     * just adjust protection. */
    if ((allocationType & MEM_COMMIT) && !(allocationType & MEM_RESERVE) && address) {
        if (mprotect(address, size, prot) == 0) {
            if (win32_ProtectHook && (prot & PROT_WRITE))
                win32_ProtectHook(address, size);
            return address;
        }
        /* fall through to a fresh mapping */
    }

    /* Inside the placeholder window a fixed address replaces the placeholder
     * -- unless a live allocation is already there, which is the refusal
     * MAP_FIXED_NOREPLACE would have given outside it. */
    if (address && in_window(address, size)) {
        void *p;
        if (view_overlaps(address, size)) {
            SetLastError(ERROR_INVALID_ADDRESS);
            return NULL;
        }
        p = mmap(address, size, prot ? prot : PROT_READ | PROT_WRITE,
                 flags | MAP_FIXED, -1, 0);
        if (p == MAP_FAILED) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return NULL;
        }
        view_register(p, size);
        return p;
    }

#if defined(MAP_FIXED_NOREPLACE)
    if (address) flags |= MAP_FIXED_NOREPLACE;
#endif

    void *p;
#if defined(__APPLE__)
    if (address) {
        p = mach_map_fixed(address, size, prot ? prot : PROT_READ | PROT_WRITE);
    } else
#endif
    p = mmap(address, size, prot ? prot : PROT_READ | PROT_WRITE, flags, -1, 0);
    if (p == MAP_FAILED) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    /* Remember the length: VirtualFree(MEM_RELEASE) is passed size 0 by every
     * Win32 caller, and munmap cannot be called without one. */
    view_register(p, size);
#if !defined(MAP_FIXED_NOREPLACE) && !defined(__APPLE__)
    /* Older kernels without MAP_FIXED_NOREPLACE: plain MAP_FIXED would silently
     * unmap whatever already lives there, so we pass the address as a hint and
     * treat a different result as "taken". Apple goes through mach_map_fixed
     * above, which reports that properly instead of inferring it. */
    if (address && p != address) {
        munmap(p, size);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
#endif
    return p;
}

BOOL VirtualFree(LPVOID address, SIZE_T size, DWORD freeType)
{
    if (freeType & MEM_RELEASE) {
        /* Win32 MEM_RELEASE passes size 0 and frees the whole allocation, so
         * the length comes from the registry VirtualAlloc filled in. Returning
         * TRUE without unmapping -- as this used to -- made every release a
         * silent no-op: the caller believed the address was free, the next
         * allocation there failed, and nothing connected the two. */
        size_t len = view_take(address);
        if (size == 0) size = len;
        if (size == 0) return FALSE;
        if (s_window_hi && (uintptr_t)address == s_window_lo
                && size == s_window_hi - s_window_lo) {
            if (munmap(address, size) != 0)
                return FALSE;
            s_window_lo = s_window_hi = 0;
            return TRUE;
        }
        if (in_window(address, size))
            return window_restore(address, size);
        return munmap(address, size) == 0;
    }
    if (freeType & MEM_DECOMMIT)
        return mprotect(address, size, PROT_NONE) == 0;
    return TRUE;
}

BOOL VirtualProtect(LPVOID address, SIZE_T size, DWORD newProtect, PDWORD oldProtect)
{
    if (oldProtect) *oldProtect = PAGE_READWRITE;
#ifdef __APPLE__
    uintptr_t start = (uintptr_t)address & ~(uintptr_t)(vm_page_size - 1);
    size = (((uintptr_t)address + size + vm_page_size - 1) & ~(uintptr_t)(vm_page_size - 1)) - start;
    address = (void *)start;
#endif
    if (mprotect(address, size, prot_from_page(newProtect)) != 0)
        return FALSE;
    if (win32_ProtectHook && (prot_from_page(newProtect) & PROT_WRITE))
        win32_ProtectHook(address, size);
    return TRUE;
}

/* ===================================================================== */
/* Time                                                                  */
/* ===================================================================== */

/* 100-ns intervals between 1601-01-01 and 1970-01-01 */
#define FILETIME_EPOCH_DIFF 116444736000000000ULL

VOID GetSystemTimeAsFileTime(LPFILETIME ft)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ULONGLONG t = FILETIME_EPOCH_DIFF
                + (ULONGLONG)ts.tv_sec * 10000000ULL
                + (ULONGLONG)ts.tv_nsec / 100ULL;
    ft->dwLowDateTime  = (DWORD)(t & 0xFFFFFFFFULL);
    ft->dwHighDateTime = (DWORD)(t >> 32);
}

static void fill_systemtime(LPSYSTEMTIME st, const struct tm *tm, long nsec)
{
    st->wYear         = (WORD)(tm->tm_year + 1900);
    st->wMonth        = (WORD)(tm->tm_mon + 1);
    st->wDayOfWeek    = (WORD)tm->tm_wday;
    st->wDay          = (WORD)tm->tm_mday;
    st->wHour         = (WORD)tm->tm_hour;
    st->wMinute       = (WORD)tm->tm_min;
    st->wSecond       = (WORD)tm->tm_sec;
    st->wMilliseconds = (WORD)(nsec / 1000000L);
}

VOID GetSystemTime(LPSYSTEMTIME st)
{
    struct timespec ts; struct tm tm;
    clock_gettime(CLOCK_REALTIME, &ts);
    gmtime_r(&ts.tv_sec, &tm);
    fill_systemtime(st, &tm, ts.tv_nsec);
}

VOID GetLocalTime(LPSYSTEMTIME st)
{
    struct timespec ts; struct tm tm;
    clock_gettime(CLOCK_REALTIME, &ts);
    localtime_r(&ts.tv_sec, &tm);
    fill_systemtime(st, &tm, ts.tv_nsec);
}

ULONGLONG GetTickCount64(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ULONGLONG)ts.tv_sec * 1000ULL + (ULONGLONG)ts.tv_nsec / 1000000ULL;
}

DWORD GetTickCount(void) { return (DWORD)GetTickCount64(); }

BOOL QueryPerformanceCounter(PLARGE_INTEGER count)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    count->QuadPart = (LONGLONG)ts.tv_sec * 1000000000LL + ts.tv_nsec;
    return TRUE;
}

BOOL QueryPerformanceFrequency(PLARGE_INTEGER freq)
{
    freq->QuadPart = 1000000000LL;   /* QPC is in nanoseconds */
    return TRUE;
}

/* ===================================================================== */
/* Misc                                                                  */
/* ===================================================================== */

VOID OutputDebugStringA(LPCSTR str)
{
    if (str) fputs(str, stderr);
}

VOID ExitProcess(UINT exitCode) { exit((int)exitCode); }

BOOL IsDebuggerPresent(void)
{
#if defined(__APPLE__)
    /* Darwin: KERN_PROC_PID reports P_TRACED when a debugger is attached. */
    struct kinfo_proc info;
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid() };
    size_t size = sizeof(info);
    memset(&info, 0, size);
    if (sysctl(mib, sizeof(mib), &info, &size, NULL, 0) != 0) return FALSE;
    return (info.kp_proc.p_flag & P_TRACED) != 0;
#else
    /* Linux: a non-zero TracerPid in /proc/self/status means ptrace is attached. */
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) return FALSE;
    char line[256];
    BOOL traced = FALSE;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "TracerPid:", 10) == 0) {
            traced = strtol(line + 10, NULL, 10) != 0;
            break;
        }
    }
    fclose(f);
    return traced;
#endif
}

VOID DebugBreak(void) {
    /* Not __debugbreak(): that is an MSVC intrinsic, and this file is the
     * half that MSVC never compiles. SIGTRAP is the POSIX equivalent --
     * continuable under a debugger, fatal without one, as on Windows. */
    raise(SIGTRAP);
}

VOID SecureZeroMemory(PVOID ptr, SIZE_T cnt)
{
#if defined(__APPLE__)
    memset_s(ptr, cnt, 0, cnt);
#else
    explicit_bzero(ptr, cnt);
#endif
}

unsigned int _clearfp(void)
{
    feclearexcept(FE_ALL_EXCEPT);
    return 0;
}

/* ===================================================================== */
/* Win32 file API on POSIX (open/read/write/fstat-backed)                */
/* ===================================================================== */

#include <fcntl.h>
#include <sys/stat.h>

HANDLE CreateFileA(LPCSTR name, DWORD access, DWORD share,
                   LPSECURITY_ATTRIBUTES sa, DWORD disp,
                   DWORD flags, HANDLE templ)
{
    (void)share; (void)sa; (void)flags; (void)templ;
    if (!name) { SetLastError(ERROR_INVALID_PARAMETER); return INVALID_HANDLE_VALUE; }

    int rw = O_RDONLY;
    int wantW = (access & (GENERIC_WRITE | GENERIC_ALL)) != 0;
    int wantR = (access & (GENERIC_READ  | GENERIC_ALL)) != 0;
    if (wantW && wantR) rw = O_RDWR;
    else if (wantW)     rw = O_WRONLY;

    int extra = 0;
    switch (disp) {
    case CREATE_NEW:        extra = O_CREAT | O_EXCL;  break;
    case CREATE_ALWAYS:     extra = O_CREAT | O_TRUNC; break;
    case OPEN_EXISTING:     extra = 0;                 break;
    case OPEN_ALWAYS:       extra = O_CREAT;           break;
    case TRUNCATE_EXISTING: extra = O_TRUNC;           break;
    default:                extra = 0;                 break;
    }
    if ((extra & (O_CREAT | O_TRUNC)) && rw == O_RDONLY) rw = O_RDWR;

    /* Normalise embedded Windows-style backslashes before open(). */
    char norm[1024];
    snprintf(norm, sizeof(norm), "%s", name);
    xbox_path_normalize(norm);
    int fd = open(norm, rw | extra, 0644);
    if (fd < 0) { SetLastError(ERROR_FILE_NOT_FOUND); return INVALID_HANDLE_VALUE; }
    return w32_open_handle(fd, name);
}

HANDLE CreateFileW(LPCWSTR name, DWORD access, DWORD share,
                   LPSECURITY_ATTRIBUTES sa, DWORD disp,
                   DWORD flags, HANDLE templ)
{
    /* Basic UTF-16 -> UTF-8 (ASCII path of WideCharToMultiByte). */
    char buf[1024];
    int len = WideCharToMultiByte(CP_UTF8, 0, name, -1, buf, sizeof(buf), NULL, NULL);
    if (len <= 0) buf[0] = '\0';
    return CreateFileA(buf, access, share, sa, disp, flags, templ);
}

BOOL ReadFile(HANDLE h, LPVOID buf, DWORD len, LPDWORD nread, void *overlapped)
{
    (void)overlapped;
    int fd = w32_handle_fd(h);
    if (fd < 0) { if (nread) *nread = 0; SetLastError(ERROR_INVALID_HANDLE); return FALSE; }
    ssize_t n = read(fd, buf, len);
    if (n < 0)  { if (nread) *nread = 0; SetLastError(ERROR_GEN_FAILURE);    return FALSE; }
    if (nread)  *nread = (DWORD)n;
    return TRUE;
}

BOOL WriteFile(HANDLE h, LPCVOID buf, DWORD len, LPDWORD nwritten, void *overlapped)
{
    (void)overlapped;
    int fd = w32_handle_fd(h);
    if (fd < 0) { if (nwritten) *nwritten = 0; SetLastError(ERROR_INVALID_HANDLE); return FALSE; }
    ssize_t n = write(fd, buf, len);
    if (n < 0)  { if (nwritten) *nwritten = 0; SetLastError(ERROR_GEN_FAILURE);    return FALSE; }
    if (nwritten) *nwritten = (DWORD)n;
    return TRUE;
}

DWORD GetFileSize(HANDLE h, LPDWORD high)
{
    int fd = w32_handle_fd(h);
    if (fd < 0) return INVALID_FILE_SIZE;
    struct stat st;
    if (fstat(fd, &st) != 0) return INVALID_FILE_SIZE;
    if (high) *high = (DWORD)(((uint64_t)st.st_size >> 32) & 0xFFFFFFFFu);
    return (DWORD)(st.st_size & 0xFFFFFFFFu);
}

BOOL GetFileSizeEx(HANDLE h, PLARGE_INTEGER size)
{
    int fd = w32_handle_fd(h);
    if (fd < 0 || !size) { SetLastError(ERROR_INVALID_HANDLE); return FALSE; }
    struct stat st;
    if (fstat(fd, &st) != 0) { SetLastError(ERROR_GEN_FAILURE); return FALSE; }
    size->QuadPart = (LONGLONG)st.st_size;
    return TRUE;
}

BOOL FlushFileBuffers(HANDLE h)
{
    int fd = w32_handle_fd(h);
    if (fd < 0) return FALSE;
    return fsync(fd) == 0;
}

/* ===================================================================== */
/* Keyboard + window helpers -- stubs. Real keyboard polling will come   */
/* via SDL_GetKeyboardState when main.c gets its SDL2 port.              */
/* ===================================================================== */

SHORT GetAsyncKeyState(int vKey)          { (void)vKey; return 0; }
HWND  FindWindowA(LPCSTR c, LPCSTR w)     { (void)c; (void)w; return NULL; }
HWND  GetActiveWindow(void)               { return NULL; }
BOOL  SetWindowTextA(HWND h, LPCSTR t)    { (void)h; (void)t; return TRUE; }
int   GetWindowTextA(HWND h, LPSTR t, int n) { (void)h; (void)t; (void)n; return 0; }
BOOL  EnumWindows(WNDENUMPROC p, LPARAM l) { (void)p; (void)l; return FALSE; }

int MessageBoxA(HWND h, LPCSTR text, LPCSTR caption, UINT type)
{
    (void)h; (void)type;
    fprintf(stderr, "[%s] %s\n", caption ? caption : "MessageBox",
                                   text    ? text    : "");
    return 1;   /* IDOK */
}

/* Message-loop stubs: no Win32 messages on POSIX (SDL events drive the
 * d3d8_gl backend; this layer is just for the game's Win32 message pump). */
BOOL    PeekMessageA(LPMSG m, HWND w, UINT a, UINT b, UINT f)
{ (void)m; (void)w; (void)a; (void)b; (void)f; return FALSE; }
BOOL    TranslateMessage(const MSG *m) { (void)m; return TRUE; }
LRESULT DispatchMessageA(const MSG *m) { (void)m; return 0; }

/* XInput stub: real gamepad is wired through input_compat (SDL2). */
DWORD XInputGetState(DWORD idx, XINPUT_STATE *state)
{ (void)idx; if (state) memset(state, 0, sizeof(*state)); return ERROR_DEVICE_NOT_CONNECTED; }

BOOL TerminateProcess(HANDLE process, UINT exitCode)
{
    (void)process;
    exit((int)exitCode);
}

VOID OutputDebugStringW(LPCWSTR str)
{
    if (!str) return;
    for (const WCHAR *p = str; *p; p++)
        fputc((*p < 128) ? (int)*p : '?', stderr);
}

/*
 * Minimal MultiByteToWideChar / WideCharToMultiByte. Handles UTF-8 and a
 * latin-1 interpretation of CP_ACP -- enough for path/name strings.
 */
int MultiByteToWideChar(UINT cp, DWORD flags, LPCSTR mb, int mbCount,
                        LPWSTR wide, int wideCount)
{
    (void)flags;
    if (!mb) return 0;
    int srcLen = (mbCount < 0) ? (int)strlen(mb) + 1 : mbCount;
    int out = 0;

    for (int i = 0; i < srcLen; ) {
        unsigned int cpval;
        unsigned char c = (unsigned char)mb[i];

        if (cp == CP_UTF8 && c >= 0x80) {
            if ((c & 0xE0) == 0xC0 && i + 1 < srcLen) {
                cpval = ((c & 0x1F) << 6) | (mb[i+1] & 0x3F); i += 2;
            } else if ((c & 0xF0) == 0xE0 && i + 2 < srcLen) {
                cpval = ((c & 0x0F) << 12) | ((mb[i+1] & 0x3F) << 6) |
                        (mb[i+2] & 0x3F); i += 3;
            } else if ((c & 0xF8) == 0xF0 && i + 3 < srcLen) {
                cpval = ((c & 0x07) << 18) | ((mb[i+1] & 0x3F) << 12) |
                        ((mb[i+2] & 0x3F) << 6) | (mb[i+3] & 0x3F); i += 4;
            } else { cpval = c; i += 1; }
        } else {
            cpval = c; i += 1;   /* ASCII / latin-1 */
        }

        if (cpval > 0xFFFF) cpval = '?';   /* no surrogate pairs */
        if (wideCount > 0) {
            if (out >= wideCount) return 0;
            wide[out] = (WCHAR)cpval;
        }
        out++;
    }
    return out;
}

int WideCharToMultiByte(UINT cp, DWORD flags, LPCWSTR wide, int wideCount,
                        LPSTR mb, int mbCount, LPCSTR defChar, PBOOL usedDef)
{
    (void)flags; (void)defChar; (void)usedDef;
    if (!wide) return 0;
    int srcLen = wideCount;
    if (srcLen < 0) { srcLen = 0; while (wide[srcLen]) srcLen++; srcLen++; }
    int out = 0;

    for (int i = 0; i < srcLen; i++) {
        unsigned int cpval = wide[i];
        char buf[4]; int n;
        if (cp == CP_UTF8 && cpval >= 0x80) {
            if (cpval < 0x800) {
                buf[0] = (char)(0xC0 | (cpval >> 6));
                buf[1] = (char)(0x80 | (cpval & 0x3F)); n = 2;
            } else {
                buf[0] = (char)(0xE0 | (cpval >> 12));
                buf[1] = (char)(0x80 | ((cpval >> 6) & 0x3F));
                buf[2] = (char)(0x80 | (cpval & 0x3F)); n = 3;
            }
        } else {
            buf[0] = (char)(cpval > 0xFF ? '?' : cpval); n = 1;
        }
        if (mbCount > 0) {
            if (out + n > mbCount) return 0;
            for (int k = 0; k < n; k++) mb[out + k] = buf[k];
        }
        out += n;
    }
    return out;
}

/* ===================================================================== */
/* File mapping (memfd-backed) -- true aliased mirror views              */
/* ===================================================================== */

/* Registry of active views: UnmapViewOfFile takes no length, so we must
 * recover the mapping length here for munmap. */
typedef struct { void *addr; size_t len; } w32_view;
static w32_view        s_views[512];
static pthread_mutex_t s_views_lock = PTHREAD_MUTEX_INITIALIZER;

void view_register(void *addr, size_t len)
{
    pthread_mutex_lock(&s_views_lock);
    for (int i = 0; i < 512; i++)
        if (!s_views[i].addr) { s_views[i].addr = addr; s_views[i].len = len; break; }
    pthread_mutex_unlock(&s_views_lock);
}

/* Non-destructive counterpart to view_take, and interior-aware: VirtualQuery
 * is asked about addresses *within* a region at least as often as about its
 * base -- a translated guest VA lands in the middle of the 64 MB window.
 * Picks the containing region and reports where it starts. */
int view_lookup(const void *addr, void **base_out, size_t *len_out)
{
    int found = 0;
    pthread_mutex_lock(&s_views_lock);
    for (int i = 0; i < 512; i++) {
        if (!s_views[i].addr)
            continue;
        uintptr_t lo = (uintptr_t)s_views[i].addr;
        uintptr_t hi = lo + s_views[i].len;
        if ((uintptr_t)addr >= lo && (uintptr_t)addr < hi) {
            if (base_out) *base_out = s_views[i].addr;
            if (len_out)  *len_out  = s_views[i].len;
            found = 1;
            break;
        }
    }
    pthread_mutex_unlock(&s_views_lock);
    return found;
}

/* Does [addr, addr+len) intersect any registered allocation or view? */
static int view_overlaps(const void *addr, size_t len)
{
    uintptr_t lo = (uintptr_t)addr, hi = lo + len;
    int hit = 0;
    pthread_mutex_lock(&s_views_lock);
    for (int i = 0; i < 512 && !hit; i++) {
        uintptr_t vlo = (uintptr_t)s_views[i].addr;
        if (vlo && vlo < hi && lo < vlo + s_views[i].len)
            hit = 1;
    }
    pthread_mutex_unlock(&s_views_lock);
    return hit;
}

size_t view_take(const void *addr)
{
    size_t len = 0;
    pthread_mutex_lock(&s_views_lock);
    for (int i = 0; i < 512; i++)
        if (s_views[i].addr == addr) { len = s_views[i].len; s_views[i].addr = NULL; break; }
    pthread_mutex_unlock(&s_views_lock);
    return len;
}

/* An unnamed file descriptor that ftruncate and mmap both accept. Linux has
 * memfd_create for this; elsewhere an immediately-unlinked temp file does. */
static int anon_map_fd(const char *name)
{
#if defined(__APPLE__)
    static volatile LONG map_counter = 0;
    char shm_name[32];
    LONG seq = InterlockedIncrement(&map_counter);
    const char *base = name ? name : "xbox_map";
    snprintf(shm_name, sizeof(shm_name), "/%.12s_%d_%ld", base, (int)getpid(), (long)seq);
    int fd = shm_open(shm_name, O_CREAT | O_RDWR | O_EXCL, 0600);
    if (fd >= 0) shm_unlink(shm_name);
    return fd;
#else
    return memfd_create(name ? name : "xbox_map", 0);
#endif
}

HANDLE CreateFileMappingA(HANDLE file, LPSECURITY_ATTRIBUTES sa, DWORD protect,
                          DWORD maxSizeHigh, DWORD maxSizeLow, LPCSTR name)
{
    (void)file; (void)sa; (void)protect;
    SIZE_T size = ((SIZE_T)maxSizeHigh << 32) | maxSizeLow;
    if (size == 0) { SetLastError(ERROR_INVALID_PARAMETER); return NULL; }

    int fd = anon_map_fd(name);
    if (fd < 0) { SetLastError(ERROR_NOT_ENOUGH_MEMORY); return NULL; }
    if (ftruncate(fd, (off_t)size) != 0) {
        close(fd);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    w32_object *o = obj_alloc(K_FILEMAP);
    o->fd       = fd;
    o->map_size = size;
    return (HANDLE)o;
}

HANDLE CreateFileMappingW(HANDLE file, LPSECURITY_ATTRIBUTES sa, DWORD protect,
                          DWORD maxSizeHigh, DWORD maxSizeLow, LPCWSTR name)
{
    (void)name;
    return CreateFileMappingA(file, sa, protect, maxSizeHigh, maxSizeLow, NULL);
}

LPVOID MapViewOfFileEx(HANDLE mapping, DWORD access, DWORD offHigh, DWORD offLow,
                       SIZE_T count, LPVOID baseAddr)
{
    w32_object *o = (w32_object *)mapping;
    if (!o || o->kind != K_FILEMAP) { SetLastError(ERROR_INVALID_HANDLE); return NULL; }

    off_t  off = ((off_t)offHigh << 32) | offLow;
    SIZE_T len = count ? count : (o->map_size - (SIZE_T)off);
    int prot   = PROT_READ | ((access != FILE_MAP_READ) ? PROT_WRITE : 0);
    int flags  = MAP_SHARED;

    /* Win32 MapViewOfFileEx *fails* when the requested address is unavailable.
     * Plain MAP_FIXED does the opposite: it silently unmaps whatever is there
     * and succeeds. The Xbox memory model asks for 28 mirror views at computed
     * addresses, so with a base the OS chose rather than one we picked, that
     * difference is the process quietly destroying its own libraries and heap
     * and dying somewhere unrelated a moment later. */
    if (baseAddr && in_window(baseAddr, len)) {
        /* Replacing our own placeholder: MAP_FIXED, but not over a live view. */
        if (view_overlaps(baseAddr, len)) {
            SetLastError(ERROR_INVALID_ADDRESS);
            return NULL;
        }
        flags |= MAP_FIXED;
    } else if (baseAddr) {
#if defined(MAP_FIXED_NOREPLACE)
        flags |= MAP_FIXED_NOREPLACE;
#elif defined(__APPLE__)
        /* Darwin has no MAP_FIXED_NOREPLACE. Claim the range first with
         * mach_vm_map(VM_FLAGS_FIXED), which refuses rather than displaces;
         * MAP_FIXED below can then only replace the placeholder we now own. */
        if (mach_map_fixed(baseAddr, len, PROT_READ | PROT_WRITE) == MAP_FAILED) {
            SetLastError(ERROR_INVALID_ADDRESS);
            return NULL;
        }
        flags |= MAP_FIXED;
#else
        flags |= MAP_FIXED;
#endif
    }

    void *p = mmap(baseAddr, len, prot, flags, o->fd, off);
    if (p == MAP_FAILED) { SetLastError(ERROR_NOT_ENOUGH_MEMORY); return NULL; }
    if (baseAddr && p != baseAddr) {
        /* MAP_FIXED_NOREPLACE hands back a different address instead of
         * failing on some kernels; treat that as the refusal it means. */
        munmap(p, len);
        SetLastError(ERROR_INVALID_ADDRESS);
        return NULL;
    }
    view_register(p, len);
    return p;
}

LPVOID MapViewOfFile(HANDLE mapping, DWORD access, DWORD offHigh, DWORD offLow, SIZE_T count)
{
    return MapViewOfFileEx(mapping, access, offHigh, offLow, count, NULL);
}

BOOL UnmapViewOfFile(LPCVOID baseAddr)
{
    size_t len = view_take(baseAddr);
    if (len == 0) return FALSE;
    if (in_window(baseAddr, len))
        return window_restore((void *)baseAddr, len);
    return munmap((void *)baseAddr, len) == 0;
}

/* ===================================================================== */
/* VirtualQuery                                                           */
/* ===================================================================== */

/*
 * Answers from the view registry rather than from a constant.
 *
 * This used to report RegionSize 0x1000, MEM_COMMIT, PAGE_READWRITE and
 * AllocationBase NULL for every address it was handed, mapped or not. Four
 * kernel entry points are built on it, and one of them chooses a deallocator
 * with it: MmFreeContiguousMemory frees via VirtualFree only when
 * AllocationBase equals the pointer, so a hardcoded NULL sent every
 * contiguous buffer -- all of which come from VirtualAlloc, i.e. mmap -- to
 * _aligned_free, which is free(). The allocator aborts on the foreign
 * pointer. MmQueryAllocationSize answered 0x1000 for everything and
 * NtQueryVirtualMemory called unmapped addresses committed and readable.
 *
 * Everything the shim maps -- VirtualAlloc and MapViewOfFileEx alike -- is in
 * the registry, so it can answer for exactly the memory it owns and say
 * MEM_FREE for the rest. Saying MEM_FREE for an address it did not map is the
 * honest answer: a host heap pointer is not a Win32 reservation, and the
 * callers that branch on this want to know which allocator owns the pointer.
 */
SIZE_T VirtualQuery(LPCVOID address, PMEMORY_BASIC_INFORMATION buffer, SIZE_T length)
{
    if (!buffer || length < sizeof(*buffer)) return 0;
    memset(buffer, 0, sizeof(*buffer));

    void  *base = NULL;
    size_t len  = 0;

    if (!view_lookup(address, &base, &len)) {
        /* Not ours. Report it free rather than inventing a committed page. */
        buffer->BaseAddress    = (PVOID)address;
        buffer->AllocationBase = NULL;
        buffer->RegionSize     = 0;
        buffer->State          = MEM_FREE;
        buffer->Protect        = PAGE_NOACCESS;
        buffer->Type           = 0;
        return sizeof(*buffer);
    }

    buffer->BaseAddress      = (PVOID)address;
    buffer->AllocationBase   = base;
    buffer->AllocationProtect = PAGE_READWRITE;
    /* From the queried address to the end of the region, which is what Win32
     * reports and what callers sizing a copy out of it depend on. */
    buffer->RegionSize       = len - (size_t)((uintptr_t)address - (uintptr_t)base);
    buffer->State            = MEM_COMMIT;
    buffer->Protect          = PAGE_READWRITE;
    buffer->Type             = MEM_PRIVATE;
    return sizeof(*buffer);
}

BOOL GlobalMemoryStatusEx(LPMEMORYSTATUSEX b)
{
    if (!b) return FALSE;
#if defined(__APPLE__)
    /* Darwin has no sysinfo(2): physical memory comes from sysctl hw.memsize,
     * swap from vm.swapusage, the free page count from the Mach VM statistics. */
    uint64_t memsize = 0;
    size_t   len     = sizeof(memsize);
    int oid_memsize[] = { CTL_HW, HW_MEMSIZE };
    if (sysctl(oid_memsize, 2, &memsize, &len, NULL, 0) != 0) return FALSE;

    vm_size_t page = 0;
    if (host_page_size(mach_host_self(), &page) != KERN_SUCCESS) page = 4096;

    vm_statistics64_data_t vm;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    ULONGLONG avail = 0;
    if (host_statistics64(mach_host_self(), HOST_VM_INFO64,
                          (host_info64_t)&vm, &count) == KERN_SUCCESS)
        avail = ((ULONGLONG)vm.free_count + vm.inactive_count) * (ULONGLONG)page;

    struct xsw_usage swap;
    len = sizeof(swap);
    int oid_swapusage[] = { CTL_VM, VM_SWAPUSAGE };
    if (sysctl(oid_swapusage, 2, &swap, &len, NULL, 0) != 0)
        memset(&swap, 0, sizeof(swap));

    b->ullTotalPhys     = (ULONGLONG)memsize;
    b->ullAvailPhys     = avail;
    b->ullTotalPageFile = b->ullTotalPhys + (ULONGLONG)swap.xsu_total;
    b->ullAvailPageFile = b->ullAvailPhys + (ULONGLONG)swap.xsu_avail;
#else
    struct sysinfo si;
    if (sysinfo(&si) != 0) return FALSE;

    ULONGLONG unit = si.mem_unit ? si.mem_unit : 1;
    b->ullTotalPhys     = (ULONGLONG)si.totalram  * unit;
    b->ullAvailPhys     = (ULONGLONG)si.freeram   * unit;
    b->ullTotalPageFile = b->ullTotalPhys + (ULONGLONG)si.totalswap * unit;
    b->ullAvailPageFile = b->ullAvailPhys + (ULONGLONG)si.freeswap  * unit;
#endif
    b->ullTotalVirtual  = b->ullTotalPhys;
    b->ullAvailVirtual  = b->ullAvailPhys;
    b->ullAvailExtendedVirtual = 0;
    b->dwMemoryLoad = b->ullTotalPhys
        ? (DWORD)(100 - (b->ullAvailPhys * 100 / b->ullTotalPhys)) : 0;
    return TRUE;
}

/* ===================================================================== */
/* Aligned allocation                                                     */
/* ===================================================================== */

void *_aligned_malloc(SIZE_T size, SIZE_T alignment)
{
    if (alignment < sizeof(void *)) alignment = sizeof(void *);
    /* round alignment up to a power of two */
    SIZE_T a = sizeof(void *);
    while (a < alignment) a <<= 1;
    void *p = NULL;
    if (posix_memalign(&p, a, size ? size : 1) != 0) return NULL;
    return p;
}

void _aligned_free(void *ptr) { free(ptr); }

/* ===================================================================== */
/* Case-insensitive string compare                                        */
/* ===================================================================== */

int _stricmp(const char *a, const char *b)            { return strcasecmp(a, b); }
int _strnicmp(const char *a, const char *b, SIZE_T n) { return strncasecmp(a, b, n); }

/* ===================================================================== */
/* Wide-string helpers (16-bit Xbox WCHAR)                                */
/* ===================================================================== */

SIZE_T xbox_wcslen(const WCHAR *s)
{
    SIZE_T n = 0;
    if (s) while (s[n]) n++;
    return n;
}

int xbox_wcsncmp(const WCHAR *a, const WCHAR *b, SIZE_T n)
{
    for (SIZE_T i = 0; i < n; i++) {
        if (a[i] != b[i]) return (int)a[i] - (int)b[i];
        if (a[i] == 0)    return 0;
    }
    return 0;
}

WCHAR *xbox_wcscat(WCHAR *dst, const WCHAR *src)
{
    SIZE_T d = xbox_wcslen(dst), i = 0;
    while (src[i]) { dst[d + i] = src[i]; i++; }
    dst[d + i] = 0;
    return dst;
}

WCHAR *xbox_wcscpy(WCHAR *dst, const WCHAR *src)
{
    SIZE_T i = 0;
    while (src[i]) { dst[i] = src[i]; i++; }
    dst[i] = 0;
    return dst;
}

/* ===================================================================== */
/* Time conversion                                                        */
/* ===================================================================== */

BOOL SystemTimeToFileTime(const SYSTEMTIME *st, LPFILETIME ft)
{
    struct tm tm;
    memset(&tm, 0, sizeof(tm));
    tm.tm_year = st->wYear - 1900;
    tm.tm_mon  = st->wMonth - 1;
    tm.tm_mday = st->wDay;
    tm.tm_hour = st->wHour;
    tm.tm_min  = st->wMinute;
    tm.tm_sec  = st->wSecond;
    time_t t = timegm(&tm);
    ULONGLONG ticks = FILETIME_EPOCH_DIFF
                    + (ULONGLONG)t * 10000000ULL
                    + (ULONGLONG)st->wMilliseconds * 10000ULL;
    ft->dwLowDateTime  = (DWORD)(ticks & 0xFFFFFFFFULL);
    ft->dwHighDateTime = (DWORD)(ticks >> 32);
    return TRUE;
}

BOOL FileTimeToSystemTime(const FILETIME *ft, LPSYSTEMTIME st)
{
    ULONGLONG ticks = ((ULONGLONG)ft->dwHighDateTime << 32) | ft->dwLowDateTime;
    if (ticks < FILETIME_EPOCH_DIFF) { memset(st, 0, sizeof(*st)); return FALSE; }
    ULONGLONG since = ticks - FILETIME_EPOCH_DIFF;
    time_t t = (time_t)(since / 10000000ULL);
    struct tm tm;
    gmtime_r(&t, &tm);
    fill_systemtime(st, &tm, (long)((since % 10000000ULL) * 100ULL));
    return TRUE;
}

/* ===================================================================== */
/* Exception handling (compile-shim -- SEH not yet emulated on Linux)     */
/* ===================================================================== */

VOID RtlUnwind(PVOID TargetFrame, PVOID TargetIp,
               PEXCEPTION_RECORD ExceptionRecord, PVOID ReturnValue)
{
    (void)TargetFrame; (void)TargetIp; (void)ExceptionRecord; (void)ReturnValue;
    /* TODO: Windows SEH unwinding is not yet emulated on Linux. */
}

VOID RaiseException(DWORD code, DWORD flags, DWORD nargs, const ULONG_PTR *args)
{
    (void)flags; (void)nargs; (void)args;
    fprintf(stderr, "[win32_compat] RaiseException(0x%08X): SEH not emulated\n", code);
    /* TODO: on Windows this does not return; SEH dispatch unimplemented. */
}

#if RECOMP_HAS_VEH
#ifdef __APPLE__
#include <sys/ucontext.h>
#else
#include <ucontext.h>
#endif

/* Vectored exception handlers over sigaction.
 *
 * The list is a fixed array published with release stores and read without a
 * lock, because the reader is a signal handler: it can interrupt a thread that
 * is itself registering a handler, and taking a mutex there would deadlock.
 * Registration happens during bring-up, so the array never needs to grow. */
#define VEH_MAX 32
static PVECTORED_EXCEPTION_HANDLER s_veh[VEH_MAX];
static int                         s_veh_count;
static pthread_mutex_t             s_veh_lock = PTHREAD_MUTEX_INITIALIZER;

static const int s_veh_signals[] = { SIGSEGV, SIGBUS, SIGTRAP, SIGILL, SIGFPE };
#define VEH_NSIG ((int)(sizeof(s_veh_signals) / sizeof(s_veh_signals[0])))
static struct sigaction s_veh_prev[VEH_NSIG];

static void veh_ctx_from(CONTEXT *c, const ucontext_t *uc)
{
#if defined(__APPLE__) && defined(__aarch64__)
    memset(c, 0, sizeof(*c));
    c->Rip = uc->uc_mcontext->__ss.__pc;
    c->Rsp = uc->uc_mcontext->__ss.__sp;
    c->Rbp = uc->uc_mcontext->__ss.__fp;
    c->native_context = (void *)uc;
#else
    const greg_t *g = uc->uc_mcontext.gregs;
    c->Rax = (uint64_t)g[REG_RAX]; c->Rcx = (uint64_t)g[REG_RCX];
    c->Rdx = (uint64_t)g[REG_RDX]; c->Rbx = (uint64_t)g[REG_RBX];
    c->Rsp = (uint64_t)g[REG_RSP]; c->Rbp = (uint64_t)g[REG_RBP];
    c->Rsi = (uint64_t)g[REG_RSI]; c->Rdi = (uint64_t)g[REG_RDI];
    c->R8  = (uint64_t)g[REG_R8];  c->R9  = (uint64_t)g[REG_R9];
    c->R10 = (uint64_t)g[REG_R10]; c->R11 = (uint64_t)g[REG_R11];
    c->R12 = (uint64_t)g[REG_R12]; c->R13 = (uint64_t)g[REG_R13];
    c->R14 = (uint64_t)g[REG_R14]; c->R15 = (uint64_t)g[REG_R15];
    c->Rip = (uint64_t)g[REG_RIP];
    c->EFlags = (uint32_t)g[REG_EFL];
#endif
}

static void veh_ctx_to(ucontext_t *uc, const CONTEXT *c)
{
#if defined(__APPLE__) && defined(__aarch64__)
    /* The ARM64 decoder updates the native context directly. */
    (void)uc;
    (void)c;
#else
    greg_t *g = uc->uc_mcontext.gregs;
    g[REG_RAX] = (greg_t)c->Rax; g[REG_RCX] = (greg_t)c->Rcx;
    g[REG_RDX] = (greg_t)c->Rdx; g[REG_RBX] = (greg_t)c->Rbx;
    g[REG_RSP] = (greg_t)c->Rsp; g[REG_RBP] = (greg_t)c->Rbp;
    g[REG_RSI] = (greg_t)c->Rsi; g[REG_RDI] = (greg_t)c->Rdi;
    g[REG_R8]  = (greg_t)c->R8;  g[REG_R9]  = (greg_t)c->R9;
    g[REG_R10] = (greg_t)c->R10; g[REG_R11] = (greg_t)c->R11;
    g[REG_R12] = (greg_t)c->R12; g[REG_R13] = (greg_t)c->R13;
    g[REG_R14] = (greg_t)c->R14; g[REG_R15] = (greg_t)c->R15;
    g[REG_RIP] = (greg_t)c->Rip;
    g[REG_EFL] = (greg_t)c->EFlags;
#endif
}

static DWORD veh_code(int sig, const siginfo_t *si)
{
    switch (sig) {
    case SIGSEGV: case SIGBUS: return EXCEPTION_ACCESS_VIOLATION;
    /* A TF step reports TRAP_TRACE; int3 reports SI_KERNEL or TRAP_BRKPT. */
    case SIGTRAP: return si->si_code == TRAP_TRACE ? EXCEPTION_SINGLE_STEP
                                                   : EXCEPTION_BREAKPOINT;
    case SIGILL:  return si->si_code == ILL_PRVOPC ? EXCEPTION_PRIV_INSTRUCTION
                                                   : EXCEPTION_ILLEGAL_INSTRUCTION;
    case SIGFPE:  return si->si_code == FPE_INTDIV ? EXCEPTION_INT_DIVIDE_BY_ZERO
                                                   : EXCEPTION_FLT_DIVIDE_BY_ZERO;
    }
    return 0;
}

static void veh_signal(int sig, siginfo_t *si, void *uc_)
{
    ucontext_t *uc = (ucontext_t *)uc_;
    EXCEPTION_RECORD rec;
    CONTEXT ctx;
    EXCEPTION_POINTERS ep = { &rec, &ctx };
    int saved_errno = errno;
    int i, n;

    veh_ctx_from(&ctx, uc);
    memset(&rec, 0, sizeof(rec));
    rec.ExceptionCode = veh_code(sig, si);
    /* A step can also arrive without TRAP_TRACE -- gdb re-injects a passed
     * SIGTRAP as a plain one -- but the saved flags still have TF set, which
     * an int3 never does. */
    if (sig == SIGTRAP && (ctx.EFlags & 0x100))
        rec.ExceptionCode = EXCEPTION_SINGLE_STEP;
    rec.ExceptionAddress = (PVOID)(uintptr_t)ctx.Rip;
    if (sig == SIGSEGV || sig == SIGBUS) {
        rec.NumberParameters = 2;
#if defined(__APPLE__) && defined(__aarch64__)
        rec.ExceptionInformation[0] = (uc->uc_mcontext->__es.__esr >> 6) & 1;
#else
        /* Page-fault error code bit 1 is set for a write. */
        rec.ExceptionInformation[0] = (uc->uc_mcontext.gregs[REG_ERR] & 2) ? 1 : 0;
#endif
        rec.ExceptionInformation[1] = (ULONG_PTR)si->si_addr;
    }

    /* Handlers take runtime locks (the GPU model's, for one): a suspension
     * arriving now waits until the handler is done. */
    t_host_depth++;
    n = __atomic_load_n(&s_veh_count, __ATOMIC_ACQUIRE);
    for (i = 0; i < n; i++) {
        PVECTORED_EXCEPTION_HANDLER h = __atomic_load_n(&s_veh[i], __ATOMIC_ACQUIRE);
        if (h && h(&ep) == EXCEPTION_CONTINUE_EXECUTION) {
            veh_ctx_to(uc, &ctx);
            compat_host_leave();
            errno = saved_errno;
            return;
        }
    }
    t_host_depth--;

    /* Nobody took it: hand it to whoever had the signal before us, or restore
     * the default and return so the instruction faults again and the process
     * dies with the right signal (and a core) instead of looping here. */
    for (i = 0; i < VEH_NSIG; i++) {
        if (s_veh_signals[i] != sig)
            continue;
        if (s_veh_prev[i].sa_flags & SA_SIGINFO) {
            if (s_veh_prev[i].sa_sigaction) {
                s_veh_prev[i].sa_sigaction(sig, si, uc_);
                return;
            }
        } else if (s_veh_prev[i].sa_handler != SIG_DFL
                   && s_veh_prev[i].sa_handler != SIG_IGN) {
            s_veh_prev[i].sa_handler(sig);
            return;
        }
        break;
    }
    signal(sig, SIG_DFL);
    /* SIGTRAP from a step does not re-fault on return; raise it instead. */
    if (sig == SIGTRAP)
        raise(sig);
}

static void veh_install(void)
{
    struct sigaction sa;
    int i;

    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = veh_signal;
    sigemptyset(&sa.sa_mask);
    /* SA_NODEFER: a handler that touches a trapped page itself (a nested MMIO
     * access) must be able to take the fault rather than be killed by it. */
    sa.sa_flags = SA_SIGINFO | SA_NODEFER | SA_ONSTACK;
    for (i = 0; i < VEH_NSIG; i++)
        sigaction(s_veh_signals[i], &sa, &s_veh_prev[i]);
}

PVOID AddVectoredExceptionHandler(ULONG First, PVECTORED_EXCEPTION_HANDLER Handler)
{
    PVOID ret = NULL;

    if (!Handler)
        return NULL;
    pthread_mutex_lock(&s_veh_lock);
    if (s_veh_count < VEH_MAX) {
        if (s_veh_count == 0)
            veh_install();
        if (First) {
            /* Shift right from the top so each slot is copied before it is
             * overwritten. A fault racing this on another thread can see a
             * handler twice, which is harmless; handlers are registered during
             * bring-up, before guest threads exist. */
            int i;
            for (i = s_veh_count; i > 0; i--)
                __atomic_store_n(&s_veh[i], s_veh[i - 1], __ATOMIC_RELEASE);
            __atomic_store_n(&s_veh[0], Handler, __ATOMIC_RELEASE);
        } else {
            __atomic_store_n(&s_veh[s_veh_count], Handler, __ATOMIC_RELEASE);
        }
        __atomic_store_n(&s_veh_count, s_veh_count + 1, __ATOMIC_RELEASE);
        ret = (PVOID)Handler;
    }
    pthread_mutex_unlock(&s_veh_lock);
    return ret;
}

ULONG RemoveVectoredExceptionHandler(PVOID h)
{
    ULONG found = 0;
    int i;

    pthread_mutex_lock(&s_veh_lock);
    for (i = 0; i < s_veh_count; i++) {
        if ((PVOID)s_veh[i] != h)
            continue;
        for (; i + 1 < s_veh_count; i++)
            __atomic_store_n(&s_veh[i], s_veh[i + 1], __ATOMIC_RELEASE);
        __atomic_store_n(&s_veh[s_veh_count - 1], NULL, __ATOMIC_RELEASE);
        __atomic_store_n(&s_veh_count, s_veh_count - 1, __ATOMIC_RELEASE);
        found = 1;
        break;
    }
    pthread_mutex_unlock(&s_veh_lock);
    return found;
}
#else
PVOID AddVectoredExceptionHandler(ULONG First, PVECTORED_EXCEPTION_HANDLER Handler)
{ (void)First; (void)Handler; return NULL; }   /* not wired off Linux x86-64 */
ULONG RemoveVectoredExceptionHandler(PVOID h) { (void)h; return 1; }
#endif

#endif /* !_WIN32 */
