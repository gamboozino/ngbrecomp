/*
 * Dirty-page tracking for the GPU's memory (the contiguous window).
 *
 * xemu asks "has this range changed since I last looked?" before rehashing a
 * texture, recopying vertex data or re-uploading a render target; QEMU answers
 * from the pages the guest wrote. Answering "maybe" for everything (what this
 * port did first) made the GPU thread hash every bound texture on every draw:
 * half its time in gameplay, and Ninja Gaiden Black ran at 24 fps.
 *
 * Here the answer comes from page protection. A page a client has looked at is
 * write-protected; the first write to it -- guest code, a kernel bridge, the
 * GPU thread itself -- faults, marks the page dirty for every client and
 * unprotects it until the next look. A check protects the page *before*
 * reading and clearing its bit, so no write can land between the two.
 *
 * The contiguous window is also visible through the tiled aperture at
 * 0xF0000000 (same pages, second mapping); both mappings are protected.
 * Writes by the GPU thread do not mark render targets dirty: those are its own
 * downloads of what it drew, and re-uploading them would be wasted work at
 * best. NV2A_DIRTY_TRACK=0 restores the old always-dirty answers.
 */
#include "platform/vcpu.h"   /* vcpu_win_unredirect */
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#ifndef CLOCK_MONOTONIC_COARSE
#define CLOCK_MONOTONIC_COARSE CLOCK_MONOTONIC
#endif
#ifdef _WIN32
#include <windows.h>
/* The page protections as Windows has them (no write-only, as POSIX has none either). */
#define PROT_NONE  0
#define PROT_READ  1
#define PROT_WRITE 2
static void mprotect(void *addr, size_t len, int prot)
{
    DWORD old;
    VirtualProtect(addr, len, prot == PROT_NONE ? PAGE_NOACCESS : (prot & PROT_WRITE) ? PAGE_READWRITE : PAGE_READONLY,
                   &old);
}
#else
#include <sys/mman.h>
#endif

#include "platform/xbox_winnt.h"
#include "platform/win32_compat.h"
#include "qemu/osdep.h"
#include "exec/memory.h"

#define PAGE      4096u
#define MAX_PAGES (128u << 20) / PAGE

static uint8_t *s_base, *s_alias;
static size_t s_size, s_alias_size;
static unsigned s_pages;
static int s_enabled;
static volatile uint8_t s_dirty[DIRTY_MEMORY_NUM][MAX_PAGES];
static volatile uint8_t s_prot[MAX_PAGES];
/* Pages written over and over (dynamic vertex buffers, the CPU's ring of
 * per-frame data) cost a fault and two mprotects per draw if tracked. A page
 * that faults more than HOT_FAULTS times in one HOT_WINDOW_MS window is left
 * unprotected and always reads as dirty, until the next window. */
#define HOT_FAULTS     6
#define HOT_WINDOW_MS  1000
static volatile uint8_t s_faults[MAX_PAGES];
static volatile uint8_t s_hot[MAX_PAGES];
static volatile uint64_t s_window_start;
static pthread_t s_gpu_thread;
static int s_gpu_thread_set;

/* Pages under a render target the GPU has drawn and not yet written back.
 *
 * Without TCG, xemu cannot see the CPU touch a render target, so it copied
 * every one back to guest memory as soon as the GPU moved on from it -- a
 * full GPU stall each, some forty a frame in Ninja Gaiden Black, nearly
 * all of it never read, and at 4x scale each one also a downscale. Instead
 * those pages are made inaccessible (a count of the targets that cover
 * each), and the first CPU access asks the renderer to write the target
 * back -- what xemu's TCG access callback does. s_gpu_fault is that
 * request: it returns once the targets over [addr, addr+len) are written
 * back, which releases their pages. */
static volatile uint8_t s_own[MAX_PAGES];
static int (*s_gpu_fault)(hwaddr addr, hwaddr len, int write);
static unsigned long s_own_stolen;

/* Serialises a page's protection change against a check of it. The fault
 * handler marks a page dirty and only then makes it writable; the write it
 * stands for lands after the handler returns. Unserialised, a check could
 * fall between the mark and the unprotect: it consumed the mark, saw the
 * page still protected, and uploaded the old bytes -- then the page went
 * writable and the write landed unrecorded, so the GPU kept the stale copy
 * for good (Ninja Gaiden Black: exploded enemies, a branch that vanished,
 * the scarf). Under this lock a check either precedes the mark (and the
 * write is caught next time) or follows the unprotect, re-protects, and the
 * write faults again and is recorded. A spinlock: the handler runs in
 * signal context, and the holder never touches guest memory meanwhile. */
static volatile int s_lock;
static inline void dirty_lock(void)
{
    while (__atomic_exchange_n(&s_lock, 1, __ATOMIC_ACQUIRE))
        while (__atomic_load_n(&s_lock, __ATOMIC_RELAXED))
#if defined(__aarch64__)
            __asm__ volatile("yield");
#else
            __builtin_ia32_pause();
#endif
}
static inline void dirty_unlock(void)
{
    __atomic_store_n(&s_lock, 0, __ATOMIC_RELEASE);
}

void nv2a_dirty_set_gpu_thread(void)
{
    s_gpu_thread = pthread_self();
    s_gpu_thread_set = 1;
}

static void protect_run(unsigned p0, unsigned n, int prot)
{
    mprotect(s_base + (size_t)p0 * PAGE, (size_t)n * PAGE, prot);
    if (s_alias && (size_t)(p0 + n) * PAGE <= s_alias_size)
        mprotect(s_alias + (size_t)p0 * PAGE, (size_t)n * PAGE, prot);
}

/* The protection a page should have now. */
static int page_prot(unsigned p)
{
    return s_own[p] ? PROT_NONE : s_prot[p] ? PROT_READ : PROT_READ | PROT_WRITE;
}

static void reprotect(unsigned p)
{
    protect_run(p, 1, page_prot(p));
}

static int is_gpu_thread(void)
{
    return s_gpu_thread_set && pthread_equal(pthread_self(), s_gpu_thread);
}

static void mark_all(unsigned p, int surf)
{
    for (int c = 0; c < DIRTY_MEMORY_NUM; c++)
        if (surf || (c != DIRTY_MEMORY_NV2A_SURF && c != DIRTY_MEMORY_NV2A_SURF_CPU))
            s_dirty[c][p] = 1;
}

static int page_of(const uint8_t *a, unsigned *page)
{
    if (a >= s_base && a < s_base + s_size) {
        *page = (unsigned)((a - s_base) / PAGE);
        return 1;
    }
    if (s_alias && a >= s_alias && a < s_alias + s_alias_size) {
        *page = (unsigned)((a - s_alias) / PAGE);
        return 1;
    }
    return 0;
}

static LONG CALLBACK dirty_veh(PEXCEPTION_POINTERS ep)
{
    unsigned p;
    const uint8_t *a;

#if defined(_WIN32) && XBOX_HAS_VCPU
    vcpu_win_unredirect(ep);   /* the faulting instruction's context, not the preemption trampoline's */
#endif
    if (ep->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_CONTINUE_SEARCH;
    a = (const uint8_t *)ep->ExceptionRecord->ExceptionInformation[1];
    if (!page_of(a, &p))
        return EXCEPTION_CONTINUE_SEARCH;
    if (s_own[p]) {
        /* A render target not yet written back. Any thread but the GPU's
         * waits for the write-back (which releases the page) and retries;
         * the GPU thread cannot wait on itself, and its paths that read
         * guest memory write overlapping targets back first, so a fault
         * there is memory the GPU is about to overwrite: let it through. */
        int write = ep->ExceptionRecord->ExceptionInformation[0] != 0;
        if (!is_gpu_thread() && s_gpu_fault)
            s_gpu_fault((hwaddr)p * PAGE, PAGE, write);
        dirty_lock();
        if (s_own[p]) {
            s_own[p] = 0;
            s_own_stolen++;
            reprotect(p);
        }
        dirty_unlock();
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (!ep->ExceptionRecord->ExceptionInformation[0])      /* reads pass */
        return EXCEPTION_CONTINUE_SEARCH;
    dirty_lock();
    if (!s_prot[p]) {
        /* Unprotected meanwhile by someone else (or not ours): retry. */
        dirty_unlock();
        return page_of(a, &p) ? EXCEPTION_CONTINUE_EXECUTION
                              : EXCEPTION_CONTINUE_SEARCH;
    }
    {
        int gpu = is_gpu_thread();
        mark_all(p, !gpu);
        /* Any writer can make a page hot -- the GPU thread writing back a
         * render target every frame is the commonest case. Hot pages never
         * claim a render target changed (that client stays exact). */
        if (s_faults[p] < 255 && ++s_faults[p] > HOT_FAULTS)
            s_hot[p] = 1;
    }
    s_prot[p] = 0;
    reprotect(p);
    dirty_unlock();
    return EXCEPTION_CONTINUE_EXECUTION;
}

/* Host code about to write guest memory behind the guest's back (a file
 * read into a contiguous buffer): a syscall does not fault on a protected
 * page, it fails, so unprotect first and count it as a write. */
void nv2a_dirty_dma_write(void *host, size_t len)
{
    unsigned p, p1;
    if (!s_enabled || !len)
        return;
    if (!page_of(host, &p) || !page_of((uint8_t *)host + len - 1, &p1))
        return;
    for (unsigned q = p; q <= p1; q++)
        if (s_own[q] && s_gpu_fault && !is_gpu_thread()) {
            s_gpu_fault((hwaddr)p * PAGE, (hwaddr)(p1 - p + 1) * PAGE, 1);
            break;
        }
    dirty_lock();
    for (unsigned q = p; q <= p1; q++) {
        mark_all(q, 1);
        if (s_prot[q] || s_own[q]) {
            s_prot[q] = 0;
            s_own[q] = 0;
            protect_run(q, 1, PROT_READ | PROT_WRITE);
        }
    }
    dirty_unlock();
}

/* The title (or the kernel for it) made memory writable -- D3D sets
 * PAGE_READWRITE on its vertex buffers. On a tracked page that silently
 * drops our write protection: the title's writes stop faulting, the page
 * still reads as protected and clean, and the GPU keeps the old copy
 * (Ninja Gaiden Black's planked scarf and exploded enemies). Count the
 * change as a write to the pages it covers in the mapping it was made on,
 * and make the other mapping match so neither view faults on a page
 * recorded as unprotected. */
static void protect_changed(void *host, size_t len)
{
    const uint8_t *a = host, *b = a + len;
    const uint8_t *lo, *hi;

    if (!s_enabled || !len)
        return;
    if (a < s_base + s_size && b > s_base) {
        lo = a > s_base ? a : s_base;
        hi = b < s_base + s_size ? b : s_base + s_size;
        lo = s_base + (size_t)(lo - s_base) / PAGE * PAGE;
    } else if (s_alias && a < s_alias + s_alias_size && b > s_alias) {
        lo = a > s_alias ? a : s_alias;
        hi = b < s_alias + s_alias_size ? b : s_alias + s_alias_size;
        lo = s_alias + (size_t)(lo - s_alias) / PAGE * PAGE;
    } else {
        return;
    }
    nv2a_dirty_dma_write((void *)lo, (size_t)(hi - lo));
}

void nv2a_dirty_init(void *base, size_t size, void *alias, size_t alias_size)
{
#ifdef __APPLE__
    /* Guest 4 KB pages share 16 KB host pages. Keep conservative dirty answers
     * until the protection tracker groups pages at the host granularity. */
    s_enabled = 0;
#else
    const char *e = getenv("NV2A_DIRTY_TRACK");
    s_enabled = !(e && *e == '0');
#endif
    if (!s_enabled)
        return;
    s_base = base;
    s_size = size > (size_t)MAX_PAGES * PAGE ? (size_t)MAX_PAGES * PAGE : size;
    s_pages = (unsigned)(s_size / PAGE);
    s_alias = alias;
    s_alias_size = alias_size < s_size ? alias_size : s_size;
    memset((void *)s_dirty, 1, sizeof s_dirty);
    AddVectoredExceptionHandler(1, dirty_veh);
    {
        extern void (*win32_ProtectHook)(void *, size_t);
        win32_ProtectHook = protect_changed;
    }
    fprintf(stderr, "[NV2A] dirty tracking: %u pages%s\n", s_pages,
            alias ? " (+ tiled alias)" : "");
}

int nv2a_dirty_enabled(void)
{
    return s_enabled;
}

bool nv2a_dirty_test_and_clear(hwaddr addr, hwaddr size, unsigned client)
{
    unsigned p0, p1, run = 0, run0 = 0;
    bool dirty = false;

    if (!size)
        return false;
    static unsigned calls;
    if ((++calls & 1023) == 0) {   /* the clock, not on every call */
        struct timespec ts;
        uint64_t now;
        clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
        now = (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
        if (now - s_window_start >= HOT_WINDOW_MS) {
            /* New window: hot pages get another chance to go quiet. A hot
             * page was left writable and its writes were not recorded, so
             * a client that did not look at it while it was hot (a texture
             * or vertex buffer not drawn that second) would never hear it
             * changed: the title reuses that memory for a new model and the
             * GPU keeps the old one. Leaving the hot state therefore counts
             * as a write for every client but the render-target ones, which
             * only ever trust recorded non-GPU writes. */
            s_window_start = now;
            for (unsigned p = 0; p < s_pages; p++)
                if (s_hot[p])
                    mark_all(p, 0);
            memset((void *)s_faults, 0, sizeof s_faults);
            memset((void *)s_hot, 0, sizeof s_hot);
        }
    }
    p0 = (unsigned)(addr / PAGE);
    p1 = (unsigned)((addr + size - 1) / PAGE);
    if (p1 >= s_pages)
        p1 = s_pages - 1;
    dirty_lock();
    /* Protect first, then read-and-clear: a write after the protection
     * faults and re-marks the page for the next check. */
    for (unsigned p = p0; p <= p1; p++) {
        if (s_hot[p]) {
            /* Render targets: only a recorded non-GPU write counts. */
            if (client != DIRTY_MEMORY_NV2A_SURF && client != DIRTY_MEMORY_NV2A_SURF_CPU)
                dirty = true;
            if (run) { protect_run(run0, run, PROT_READ); run = 0; }
            continue;
        }
        if (!s_prot[p] && !s_own[p]) {
            s_prot[p] = 1;
            if (!run) run0 = p;
            run++;
        } else if (!s_prot[p]) {
            s_prot[p] = 1;          /* inaccessible already */
            if (run) { protect_run(run0, run, PROT_READ); run = 0; }
        } else if (run) {
            protect_run(run0, run, PROT_READ);
            run = 0;
        }
    }
    if (run)
        protect_run(run0, run, PROT_READ);
    for (unsigned p = p0; p <= p1; p++) {
        if (s_hot[p] && client != DIRTY_MEMORY_NV2A_SURF &&
            client != DIRTY_MEMORY_NV2A_SURF_CPU)
            continue;
        /* Vertex data is re-uploaded by exact byte range, not by page: a
         * draw that checks part of a page uploads only that part. Clearing
         * the page there told the next draw using the rest of the page that
         * nothing had changed, and it drew stale vertices -- Ninja Gaiden
         * Black's scarf frozen in its rest pose, its enemies exploded. A
         * page the range only partly covers keeps its mark. */
        if (client == DIRTY_MEMORY_NV2A &&
            ((p == p0 && addr % PAGE) ||
             (p == p1 && (addr + size) % PAGE))) {
            if (s_dirty[client][p])
                dirty = true;
            continue;
        }
        if (__atomic_exchange_n(&s_dirty[client][p], 0, __ATOMIC_ACQ_REL))
            dirty = true;
    }
    dirty_unlock();
    return dirty;
}

void nv2a_dirty_set(hwaddr addr, hwaddr size, unsigned client)
{
    if (!size)
        return;
    for (unsigned p = (unsigned)(addr / PAGE);
         p <= (unsigned)((addr + size - 1) / PAGE) && p < s_pages; p++)
        s_dirty[client][p] = 1;
}

void nv2a_dirty_set_gpu_fault_handler(int (*fn)(hwaddr, hwaddr, int))
{
    s_gpu_fault = fn;
}

static void own_range(hwaddr addr, hwaddr size, int delta)
{
    unsigned p0, p1;
    if (!s_enabled || !size)
        return;
    p0 = (unsigned)(addr / PAGE);
    p1 = (unsigned)((addr + size - 1) / PAGE);
    if (p1 >= s_pages)
        p1 = s_pages - 1;
    dirty_lock();
    for (unsigned p = p0; p <= p1; p++) {
        unsigned before = s_own[p];
        if (delta > 0 && s_own[p] < 255)
            s_own[p]++;
        else if (delta < 0 && s_own[p])
            s_own[p]--;
        if (!before != !s_own[p])
            reprotect(p);
    }
    dirty_unlock();
}

/* The GPU's copy of [addr, addr+size) is newer than guest memory. */
void nv2a_dirty_gpu_own(hwaddr addr, hwaddr size)
{
    own_range(addr, size, 1);
}

/* ... no longer: written back, discarded or overwritten. */
void nv2a_dirty_gpu_release(hwaddr addr, hwaddr size)
{
    own_range(addr, size, -1);
}

unsigned long nv2a_dirty_gpu_stolen(void)
{
    return s_own_stolen;
}
