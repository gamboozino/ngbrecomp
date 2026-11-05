/*
 * nv2a_vk in a recompiled title: the GPU's registers, memory and interrupt.
 *
 * The NV2A aperture (guest 0xFD000000, 16 MiB) is mapped by the memory layout
 * as plain RAM. Here it becomes the device: every page except PRAMIN is made
 * inaccessible, so a register access faults into the vectored handler below,
 * which decodes the instruction and services it against xemu's register
 * blocks. PRAMIN stays plain memory and *is* the model's instance memory --
 * no register semantics, and D3D writes a lot of it while building its DMA
 * objects and hash table, so trapping it would cost faults for nothing.
 *
 * VRAM is guest RAM (UMA): physical 0 is the base of the mapped window.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "platform/xbox_winnt.h"
#if !defined(_WIN32)
#include "platform/win32_compat.h"
#endif
#include "platform/mmio_decode.h"
#include "kernel.h"
#include "xbox_memory_layout.h"
#include "nv2a_vk.h"
#include "nv2a_vk_runtime.h"
#include "platform/vcpu.h"

#define NV2A_BASE      0xFD000000u
#define NV2A_SIZE      0x01000000u
#define PRAMIN_OFF     0x00700000u
#define PRAMIN_SIZE    0x00100000u
#define NV2A_VECTOR    3u

static uintptr_t s_aperture;     /* host address of guest 0xFD000000 */
static unsigned long s_decode_fail;

/* NV2A_MMIO_TRACE=N prints the first N register accesses. */
static long s_trace_left = -1;

static void trace(const char *dir, uint32_t off, int size, uint64_t val)
{
    if (s_trace_left < 0) {
        const char *e = getenv("NV2A_MMIO_TRACE");
        s_trace_left = e ? atol(e) : 0;
    }
    if (s_trace_left > 0) {
        s_trace_left--;
        fprintf(stderr, "  [NV2A] %s +0x%06X/%d = 0x%08llX\n", dir, off, size,
                (unsigned long long)val);
    }
}

static uint64_t rd(void *dev, uint32_t off, int size)
{
    uint64_t v = nv2a_vk_mmio_read(off, (unsigned)size);
    (void)dev;
#if XBOX_HAS_VCPU
    /* D3D waits for the GPU by polling DMA_GET. On the console the CPU is
     * slow enough that this rarely spins long; on a modern one the game's
     * frame is done early and the poll would hold the virtual CPU until the
     * flip, starving lower-priority threads (CRI's file and movie server).
     * A guest polling the GPU is waiting on hardware: let others run. */
    if (vcpu_self()) {
        /* A higher-priority thread is waiting for the CPU (the GPU's interrupt, typically): give it
         * now. On Windows this read runs in an exception handler, where the preemption redirect is
         * held off -- and under Wine the mark that holds it off is never cleared, so a game polling
         * GPU registers in a tight loop (the arcade Ninja Gaiden waiting for PGRAPH idle, its reads
         * alternating so never "the same register again") kept the CPU from the interrupt thread
         * for good: no frame ever (#18, Proton). The flag is all a pending preemption is. */
        vcpu_check_preempt();
        /* Not only DMA_GET: D3D's Present also spins on a PGRAPH register
         * (0x400B10) until the GPU catches up. Any register read twice in a
         * row by the same thread is a poll. */
        static __thread uint32_t last_off = 0xFFFFFFFFu;
        static __thread unsigned repeats;
        if (off == last_off)
            repeats++;
        else
            repeats = 0;
        /* A poll that has gone on for a while is D3D waiting for the flip
         * at the next vblank: sleep a little between reads instead of
         * spinning a core for most of every frame (battery and heat on a
         * handheld). 100 us is nothing against a 16.7 ms frame. */
        if (repeats > 32) {
            extern int nv2a_pace_on;
            extern void nv2a_pace_nap(int64_t us);
            struct timespec a, b;
            if (nv2a_pace_on > 0)
                clock_gettime(CLOCK_MONOTONIC, &a);
            vcpu_block_begin();
            vcpu_nap_us(100);
            vcpu_block_end();
            if (nv2a_pace_on > 0) {
                clock_gettime(CLOCK_MONOTONIC, &b);
                nv2a_pace_nap((b.tv_sec - a.tv_sec) * 1000000LL + (b.tv_nsec - a.tv_nsec) / 1000);
            }
        } else if (off == 0x800044 || repeats) {
            vcpu_yield_any();
        }
        last_off = off;
    }
#endif
    if (s_trace_left)
        trace("read ", off, size, v);
    return v;
}

static void wr(void *dev, uint32_t off, uint64_t val, int size)
{
    (void)dev;
    if (s_trace_left)
        trace("write", off, size, val);
    nv2a_vk_mmio_write(off, val, (unsigned)size);
}

/* The registers read and written without a fault: what a guest access to the aperture comes to (rd's polling
 * included), for code that knows it may be touching them (src/d3d/gpu_regs.h). Each fault is a trip through the
 * kernel -- some 20 us on a phone, thousands a second from D3D's kickoff and the GPU's interrupt handler. */
int nv2a_reg_live(void) { return s_aperture != 0; }   /* (the model started: NGB_RENDERER=none never starts it) */
uint32_t nv2a_reg_read32(uint32_t off) { return (uint32_t)rd(NULL, off, 4); }
void nv2a_reg_write32(uint32_t off, uint32_t val) { wr(NULL, off, val, 4); }

static LONG CALLBACK nv2a_veh(PEXCEPTION_POINTERS ep)
{
    uintptr_t a;
    uint32_t off;

#if defined(_WIN32) && XBOX_HAS_VCPU
    vcpu_win_unredirect(ep);   /* the faulting instruction's context, not the preemption trampoline's */
#endif
    if (ep->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_CONTINUE_SEARCH;
    a = ep->ExceptionRecord->ExceptionInformation[1];
    if (a < s_aperture || a - s_aperture >= NV2A_SIZE)
        return EXCEPTION_CONTINUE_SEARCH;
    off = (uint32_t)(a - s_aperture);
    if (mmio_emulate(ep->ContextRecord, off, NULL, rd, wr))
        return EXCEPTION_CONTINUE_EXECUTION;
    if (s_decode_fail++ < 20) {
        const uint8_t *ip = (const uint8_t *)ep->ContextRecord->Rip;
        fprintf(stderr, "[NV2A] undecoded register access at +0x%06X: "
                        "%02X %02X %02X %02X %02X %02X\n", off,
                ip[0], ip[1], ip[2], ip[3], ip[4], ip[5]);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

static void set_irq(int level)
{
    xbox_InterruptLine(NV2A_VECTOR, level);
}

static void display_mode(uint32_t mode, uint32_t format, uint32_t pitch,
                         uint32_t fb)
{
    nv2a_vk_set_display_mode(mode, format, pitch, fb);
}

static int env_int(const char *name, int dflt)
{
    const char *v = getenv(name);
    return v && *v ? atoi(v) : dflt;
}

int xbox_Nv2aVkStart(const Nv2aVkOptions *opt)
{
    NV2AHostConfig cfg;
    DWORD old;
    /* The GPU's physical address P is P bytes into the contiguous window
     * (guest 0x80000000), not into the low RAM view: the runtime backs that
     * window with its own storage, MmGetPhysicalAddress maps contiguous
     * addresses into it, and that is where D3D allocates its pushbuffer,
     * vertex buffers, textures and surfaces. The tiled aperture at
     * 0xF0000000 aliases the same storage. */
    uint8_t *ram = (uint8_t *)(uintptr_t)(xbox_GetMemoryOffset() + XBOX_CONTIG_BASE);

    s_aperture = (uintptr_t)xbox_Nv2aAperture();
    if (!xbox_GetMemoryOffset() || !s_aperture) {
        fprintf(stderr, "[NV2A] memory layout not ready\n");
        return -1;
    }

    memset(&cfg, 0, sizeof(cfg));
    cfg.ram = ram;
    cfg.ram_size = XBOX_CONTIG_SIZE;
    cfg.ramin = (uint8_t *)(s_aperture + PRAMIN_OFF);
    cfg.set_irq = set_irq;
    cfg.surface_scale = opt && opt->surface_scale ? opt->surface_scale
                                                  : env_int("NV2A_SCALE", 1);
    cfg.refresh_hz = opt ? opt->refresh_hz : 0;
    cfg.validation = env_int("NV2A_VK_VALIDATION", 0) != 0;
    cfg.headless = opt && opt->headless;
    cfg.stats = env_int("NV2A_STATS", 0) != 0;

    /* Registers start zeroed, the instance memory too. */
    memset((void *)s_aperture, 0, NV2A_SIZE);

    AddVectoredExceptionHandler(1, nv2a_veh);
    if (!VirtualProtect((void *)s_aperture, PRAMIN_OFF, PAGE_NOACCESS, &old) ||
        !VirtualProtect((void *)(s_aperture + PRAMIN_OFF + PRAMIN_SIZE),
                        NV2A_SIZE - PRAMIN_OFF - PRAMIN_SIZE, PAGE_NOACCESS,
                        &old)) {
        fprintf(stderr, "[NV2A] cannot trap the register aperture\n");
        return -1;
    }

    {
        /* Dirty tracking over the window and its tiled alias. */
        extern void nv2a_dirty_init(void *, size_t, void *, size_t);
        extern void nv2a_dirty_dma_write(void *, size_t);
        extern void (*xbox_GuestDmaWriteHook)(void *, size_t);
        nv2a_dirty_init(ram, XBOX_CONTIG_SIZE,
                        (void *)(uintptr_t)(xbox_GetMemoryOffset() + 0xF0000000u),
                        XBOX_CONTIG_SIZE);
        xbox_GuestDmaWriteHook = nv2a_dirty_dma_write;
    }
    xbox_DisplayModeHook = display_mode;
    if (nv2a_vk_init(&cfg) != 0)
        return -1;
    fprintf(stderr, "[NV2A] xemu NV2A model up: Vulkan renderer, scale %dx\n",
            cfg.surface_scale);
    return 0;
}
