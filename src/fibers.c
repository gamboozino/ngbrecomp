/*
 * XAPI fibers on the host.
 *
 * XAPI's SwitchToFiber (sub_0032A293) saves ebp/esi/edi/ebx and the SEH head
 * on the current stack, stores esp in the current fiber, loads esp from the
 * target and pops the target's registers. In recompiled code the guest esp is
 * a variable, but the call chain is the host's C stack: swapping esp alone
 * "returns" into the caller of the *current* fiber with the other fiber's
 * registers -- that is how the main loop's edi came back holding an address
 * and D3D's Clear ran thirteen million rectangles.
 *
 * So every guest fiber also gets a host stack and context, and the switch
 * swaps both: the guest registers that are thread-local globals (ebx, esi,
 * edi, the SEH frame pointer, esp), the guest SEH head at fs:[0], and the
 * host stack with its C frames -- which is where each fiber's lifted
 * functions keep their locals, ebp among them. A new fiber starts on its own
 * host stack exactly as XAPI's first switch would have left it: guest esp at
 * the fiber record minus 8, registers zero, empty SEH chain, entering the
 * start routine sub_0032A1AF (which calls lpStartAddress(lpParameter)).
 *
 * XAPI layout (XDK 5849): fiber record = 16 bytes at the top of the fiber's
 * kernel stack: +0 lpParameter, +4 stack base, +8 stack limit, +C saved esp.
 * The current fiber is at +8 of XAPI's per-thread data, found through the TLS
 * index at 0x45E84C and fs:[4].
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#ifdef _WIN32
#include <windows.h>   /* the host contexts are Windows fibers */
#else
#include <sys/mman.h>
#ifndef MAP_STACK
#define MAP_STACK 0
#endif
#endif

#include "recomp/gen/recomp_types.h"

#define XAPI_TLS_INDEX_VA   0x0045E84Cu
#define FIBER_START_VA      0x0032A1AFu
#define HOST_FIBER_STACK    (2u << 20)
#define MAX_FIBERS          512

extern void sub_0032A1AF(void);   /* XAPI fiber start routine (lifted) */

typedef struct HostFiber {
    uint32_t guest;               /* fiber record VA; 0 = free slot */
    void *host_sp;                /* saved host stack pointer (Windows: the fiber) */
    void *stack;                  /* host stack, NULL for a thread's own */
    int started;
    uint32_t ebx, esi, edi, seh_ebp, esp, fs0;
} HostFiber;

static HostFiber s_fibers[MAX_FIBERS];
static pthread_mutex_t s_lock = PTHREAD_MUTEX_INITIALIZER;

#if defined(__APPLE__) && defined(__aarch64__)
/* AAPCS64: x19..x30, d8..d15, floating-point control and status. */
__asm__(
    ".text\n.p2align 2\n.globl _ngb_ctx_switch\n_ngb_ctx_switch:\n"
    "sub sp, sp, #176\n"
    "stp x19, x20, [sp, #0]\nstp x21, x22, [sp, #16]\n"
    "stp x23, x24, [sp, #32]\nstp x25, x26, [sp, #48]\n"
    "stp x27, x28, [sp, #64]\nstp x29, x30, [sp, #80]\n"
    "stp d8, d9, [sp, #96]\nstp d10, d11, [sp, #112]\n"
    "stp d12, d13, [sp, #128]\nstp d14, d15, [sp, #144]\n"
    "mrs x9, fpcr\nmrs x10, fpsr\nstp x9, x10, [sp, #160]\n"
    "mov x9, sp\nstr x9, [x0]\nmov sp, x1\n"
    "ldp x19, x20, [sp, #0]\nldp x21, x22, [sp, #16]\n"
    "ldp x23, x24, [sp, #32]\nldp x25, x26, [sp, #48]\n"
    "ldp x27, x28, [sp, #64]\nldp x29, x30, [sp, #80]\n"
    "ldp d8, d9, [sp, #96]\nldp d10, d11, [sp, #112]\n"
    "ldp d12, d13, [sp, #128]\nldp d14, d15, [sp, #144]\n"
    "ldp x9, x10, [sp, #160]\nmsr fpcr, x9\nmsr fpsr, x10\n"
    "add sp, sp, #176\nret\n");
void ngb_ctx_switch(void **save, void *load);
#elif !defined(_WIN32)
/* x86-64 SysV: save the callee-saved registers and the stack pointer, load
 * the other context's. A context that has never run starts in
 * fiber_host_entry via the return address planted below its stack top. */
__asm__(
    ".text\n"
    ".globl ngb_ctx_switch\n"
    ".type ngb_ctx_switch,@function\n"
    "ngb_ctx_switch:\n"            /* (void **save_sp, void *load_sp) */
    "  pushq %rbp\n"
    "  pushq %rbx\n"
    "  pushq %r12\n"
    "  pushq %r13\n"
    "  pushq %r14\n"
    "  pushq %r15\n"
    "  subq $8, %rsp\n"
    "  stmxcsr (%rsp)\n"
    "  fnstcw 4(%rsp)\n"
    "  movq %rsp, (%rdi)\n"
    "  movq %rsi, %rsp\n"
    "  ldmxcsr (%rsp)\n"
    "  fldcw 4(%rsp)\n"
    "  addq $8, %rsp\n"
    "  popq %r15\n"
    "  popq %r14\n"
    "  popq %r13\n"
    "  popq %r12\n"
    "  popq %rbx\n"
    "  popq %rbp\n"
    "  ret\n"
    ".size ngb_ctx_switch, .-ngb_ctx_switch\n");
void ngb_ctx_switch(void **save_sp, void *load_sp);
#endif

static __thread HostFiber *t_running;   /* the fiber this host thread runs */

static HostFiber *fiber_lookup(uint32_t guest, int create)
{
    HostFiber *f = NULL, *free_slot = NULL;

    pthread_mutex_lock(&s_lock);
    for (int i = 0; i < MAX_FIBERS; i++) {
        if (s_fibers[i].guest == guest) { f = &s_fibers[i]; break; }
        if (!s_fibers[i].guest && !free_slot) free_slot = &s_fibers[i];
    }
    if (!f && create && free_slot) {
        memset(free_slot, 0, sizeof(*free_slot));
        free_slot->guest = guest;
        f = free_slot;
    }
    pthread_mutex_unlock(&s_lock);
    return f;
}

/* First code a new fiber runs, on its own host stack. */
static void fiber_host_entry(void)
{
    HostFiber *f = t_running;

    g_esp = f->guest - 8;               /* [esp] fake return, [esp+4] start */
    g_ebx = g_esi = g_edi = 0;
    g_seh_ebp = 0;
    MEM32(XBOX_FS_BASE) = 0xFFFFFFFFu;  /* empty SEH chain */
    sub_0032A1AF();
    /* XAPI bugchecks if a fiber's routine returns; so do we. */
    fprintf(stderr, "[FIBER] fiber 0x%08X returned from its start routine\n",
            f->guest);
    abort();
}

#ifdef _WIN32
/* Windows: each fiber's host context is a Windows fiber (they keep everything the x64 ABI wants
 * kept, the TEB's stack bounds among it); the thread's own context, the fiber it was made into. */
static void WINAPI fiber_win_entry(void *arg)
{
    (void)arg;
    fiber_host_entry();
}

static void fiber_prepare(HostFiber *f)
{
    f->host_sp = CreateFiber(HOST_FIBER_STACK, fiber_win_entry, f);
    f->stack = f->host_sp;
    if (!f->host_sp) {
        fprintf(stderr, "[FIBER] no host fiber for fiber 0x%08X\n", f->guest);
        abort();
    }
}

static void fiber_reset(HostFiber *f)
{
    DeleteFiber(f->host_sp);
    f->stack = f->host_sp = NULL;
    fiber_prepare(f);
}

static void ngb_ctx_switch(void **save_sp, void *load_sp)
{
    if (!*save_sp)   /* the running thread's own context, made a fiber the first time */
        *save_sp = IsThreadAFiber() ? GetCurrentFiber() : ConvertThreadToFiber(NULL);
    SwitchToFiber(load_sp);
}
#else
/* Give a never-run fiber a host stack whose first switch lands in
 * fiber_host_entry. */
static void fiber_prepare(HostFiber *f)
{
    uint64_t *sp;

    f->stack = mmap(NULL, HOST_FIBER_STACK, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    if (f->stack == MAP_FAILED) {
        fprintf(stderr, "[FIBER] no host stack for fiber 0x%08X\n", f->guest);
        abort();
    }
#ifdef __APPLE__
    sp = (uint64_t *)((char *)f->stack + HOST_FIBER_STACK - 176);
    memset(sp, 0, 176);
    sp[11] = (uint64_t)(uintptr_t)fiber_host_entry;
#else
    sp = (uint64_t *)((char *)f->stack + HOST_FIBER_STACK);
    *--sp = 0;                                  /* alignment / fake return */
    *--sp = (uint64_t)(uintptr_t)fiber_host_entry;  /* ngb_ctx_switch's ret */
    for (int i = 0; i < 6; i++)
        *--sp = 0;                              /* rbp rbx r12..r15 */
    *--sp = 0;                                  /* mxcsr + x87 cw slot */
    ((uint32_t *)sp)[0] = 0x1F80;               /* default MXCSR */
    ((uint16_t *)sp)[2] = 0x037F;               /* default x87 control */
#endif
    f->host_sp = sp;
}


/* Point an existing host stack back at fiber_host_entry. */
static void fiber_reset(HostFiber *f)
{
    void *stack = f->stack;
    f->stack = NULL;
    munmap(stack, HOST_FIBER_STACK);
    fiber_prepare(f);
}
#endif

/* What a switched-out fiber's record holds in its saved-esp field. XAPI's
 * own switch kept the guest esp there; ours keeps it in HostFiber, so the
 * field only has to tell "ours, still alive" from "CreateFiber wrote this". */
static uint32_t fiber_stamp(const HostFiber *f)
{
    return 0xF1BE0000u | (uint32_t)(f - s_fibers);
}

/* SwitchToFiber(LPVOID lpFiber), stdcall. */
void sub_0032A293(void)
{
    uint32_t target = MEM32(g_esp + 4);
    uint32_t tdata = MEM32(MEM32(XBOX_FS_BASE + 4) + MEM32(XAPI_TLS_INDEX_VA) * 4);
    uint32_t current = MEM32(tdata + 8);
    HostFiber *cur, *next;

    {   /* NGB_FIBER_TRACE=N: log N switches after the first 2000 */
        static long left = -2, skip = 2000;
        if (left == -2) {
            const char *e = getenv("NGB_FIBER_TRACE");
            left = e ? atol(e) : 0;
        }
        if (left > 0 && --skip < 0) {
            left--;
            fprintf(stderr, "[FIBER] 0x%08X -> 0x%08X from 0x%08X (ret2 0x%08X)\n",
                    current, target, MEM32(g_esp), MEM32(g_esp + 8));
        }
    }
    g_esp += 8;                         /* ret 4: as the caller expects */
    if (target == current)
        return;

    cur = t_running && t_running->guest == current ? t_running
                                                   : fiber_lookup(current, 1);
    next = fiber_lookup(target, 1);
    if (!cur || !next) {
        fprintf(stderr, "[FIBER] fiber table full\n");
        abort();
    }
    /* A fiber deleted and created again at the same record (XAPI reuses the
     * stack) is a new fiber: CreateFiber rewrote the record's saved esp, so
     * it no longer carries the stamp left when the old one switched out.
     * Resuming the old host context there would run the dead fiber's tail --
     * the task wrapper then marks the task finished before its new routine
     * ever ran, which is how the loader task left NGB's boot waiting on a
     * request nobody would serve. */
    if (next->started && MEM32(target + 0xC) != fiber_stamp(next)) {
        next->started = 0;
        if (next->stack)
            fiber_reset(next);
    }
    if (!next->started && !next->stack)
        fiber_prepare(next);

    /* Save this fiber's guest state, publish the switch, run the other. */
    cur->ebx = g_ebx; cur->esi = g_esi; cur->edi = g_edi;
    cur->seh_ebp = g_seh_ebp; cur->esp = g_esp;
    cur->fs0 = MEM32(XBOX_FS_BASE);
    cur->started = 1;
    MEM32(current + 0xC) = fiber_stamp(cur);
    MEM32(tdata + 8) = target;

    if (next->started) {
        g_ebx = next->ebx; g_esi = next->esi; g_edi = next->edi;
        g_seh_ebp = next->seh_ebp; g_esp = next->esp;
        MEM32(XBOX_FS_BASE) = next->fs0;
    } else {
        next->started = 1;
    }
    t_running = next;
    ngb_ctx_switch(&cur->host_sp, next->host_sp);

    /* Back on this fiber: whoever switched here already restored our guest
     * registers (the branch above, run by them). */
    t_running = cur;
}
