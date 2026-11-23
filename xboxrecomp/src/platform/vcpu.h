/*
 * vcpu: one virtual CPU for guest threads.
 *
 * The Xbox has a single CPU and a priority scheduler: when a thread makes a
 * higher-priority thread ready (sets its event, resumes it), that thread runs
 * at once and the first one does not continue until it blocks. Titles depend
 * on it -- CRI's middleware resumes its server thread and reads the result
 * straight back -- and running guest threads truly in parallel turns those
 * assumptions into races that hang at random.
 *
 * So guest threads share one token. A thread holds it while it runs guest
 * code and gives it up when it blocks (compat waits, sleeps, contended locks,
 * suspension); the token goes to the highest-priority ready thread, first come
 * first served among equals. A thread that becomes ready above the holder's
 * priority preempts it: at once if the holder is in guest code, else when its
 * current kernel call returns. Equal priorities are time-sliced (10 ms), so a
 * spin-wait on a peer still lets the peer run.
 *
 * Host threads (GPU, audio, timers) never attach and are unaffected; every
 * function here is a no-op on a thread that has not attached.
 */
#ifndef VCPU_H
#define VCPU_H

/* Built on POSIX, and on Windows when the build asks for it (XBOX_WIN_VCPU,
 * the CMake option XBOXRECOMP_WIN_VCPU): there the waits of the Win32 API go
 * through win32_vcpu.h's wrappers and preemption redirects the running thread
 * (vcpu.c), where POSIX has the compat layer and a signal. */
#if !defined(_WIN32) || defined(XBOX_WIN_VCPU)
#define XBOX_HAS_VCPU 1
#else
#define XBOX_HAS_VCPU 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vcpu_thread vcpu_thread;

/* Priorities: Xbox base priority increments are -15..15 (XAPI's
 * THREAD_PRIORITY_*); interrupt and DPC delivery sits above all of them. */
#define VCPU_PRIO_NORMAL     0
#define VCPU_PRIO_INTERRUPT  64

/* Make the calling thread a guest thread and wait for the CPU. */
vcpu_thread *vcpu_attach(int priority);
/* Give the CPU up for good (thread exit). */
void vcpu_detach(void);
vcpu_thread *vcpu_self(void);

/* Around anything that can block: the CPU goes to someone else meanwhile. */
void vcpu_block_begin(void);
void vcpu_block_end(void);
/* Let a ready thread of equal or higher priority run first. */
void vcpu_yield(void);
/* Let any ready thread run, lower priorities included (NtYieldExecution). */
void vcpu_yield_any(void);
/* Around a host wait that keeps the CPU: no preemption is forced on the thread meanwhile (on Windows a
 * redirect into a wait that is in a DLL fails and is retried every quantum); it yields by itself
 * (vcpu_yield_any) between its sleeps. */
void vcpu_quiet_begin(void);
void vcpu_quiet_end(void);
/* Around a host wait at host depth 0 that holds no lock of a library's while it sleeps (a condition variable's
 * wait): a preemption may yield in it, as it does in guest code. Elsewhere on Linux and macOS a preemption that
 * lands outside the executable waits for the next quantum (#275). No-op on Windows. */
void vcpu_yield_anywhere_begin(void);
void vcpu_yield_anywhere_end(void);

void vcpu_set_priority(vcpu_thread *t, int priority);
int  vcpu_get_priority(vcpu_thread *t);
int  vcpu_holds_cpu(void);   /* diagnostics */

/* Called by compat_host_leave: honour a preemption deferred while the thread
 * was in host code. */
void vcpu_check_preempt(void);
/* The calling thread is inside the scheduler (signals must defer). */
int vcpu_busy(void);

/* On by default; RECOMP_VCPU=0 runs guest threads in parallel as before,
 * for comparison. */
int vcpu_enabled(void);

/* Sleep about us microseconds (a spin-wait's pause; the caller gives the CPU
 * up around it). Windows sleeps in whole milliseconds, coarser still before
 * timeBeginPeriod: there a high-resolution waitable timer. */
void vcpu_nap_us(unsigned us);

#if defined(_WIN32) && XBOX_HAS_VCPU
/* Windows: SuspendThread/ResumeThread for guest threads (win32_vcpu.h routes
 * them here). A suspended thread gets no CPU, and one that held it gives it
 * up; resumed, it waits for it before running more guest code. */
unsigned long vcpu_win_suspend_thread(void *thread);
unsigned long vcpu_win_resume_thread(void *thread);
/* The record of the guest thread with this thread id, or NULL. */
vcpu_thread *vcpu_win_find(unsigned long tid);
/* An exception handler's first step (its EXCEPTION_POINTERS): a thread suspended while Windows was
 * delivering it an exception may have been sent through the preemption trampoline, so the context
 * the handler sees is the trampoline's, not the faulting instruction's. Undone here (the preemption
 * is tried again later); 1 when it was. Until the exception is done the thread is not preempted
 * (a handler may hold a lock). */
int vcpu_win_unredirect(void *exception_pointers);
/* Debugging: the CPU's holder and the threads waiting for it, as text into buf. */
void vcpu_debug_state(char *buf, int n);
#endif

#ifdef __cplusplus
}
#endif
#endif
