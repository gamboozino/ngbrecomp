/*
 * NV2A device host: xemu's NV2A model run inside a recompiled title.
 *
 * Replaces xemu's nv2a.c, whose job was the QEMU device around the model --
 * PCI BAR, VGA console, QOM, migration. Here the "bus" is a register trap in
 * the runtime and the "console" is the presenter, so this file keeps what the
 * model itself needs from nv2a.c (block table, interrupt routing, DMA object
 * decoding -- copied below with their copyright) and supplies the QEMU runtime
 * pieces the model leans on: the big lock, timers, threads, a VGA register
 * file, dirty-memory answers and the settings struct.
 *
 * Portions copied from xemu hw/xbox/nv2a/nv2a.c:
 *   Copyright (c) 2012 espes
 *   Copyright (c) 2015 Jannik Vogel
 *   Copyright (c) 2018-2025 Matt Borgerson
 * LGPL-2.1-or-later, like the rest of this directory.
 */
#include "hw/xbox/nv2a/nv2a_int.h"
#include "nv2a_vk.h"

#include <time.h>
#include <unistd.h>
#include "platform/vcpu.h"   /* vcpu_nap_us */

/* ======================================================================
 * QEMU runtime pieces
 * ====================================================================== */

struct config g_config = {
    .display = {
        .renderer = CONFIG_DISPLAY_RENDERER_VULKAN,
        .quality = { .surface_scale = 1 },
    },
};

Error *error_fatal;
Error *error_abort;

void error_setg_internal(Error **errp, const char *fmt, ...)
{
    va_list ap;
    Error *e = g_new0(Error, 1);

    va_start(ap, fmt);
    e->msg = g_strdup_vprintf(fmt, ap);
    va_end(ap);
    if (errp == &error_fatal || errp == &error_abort || !errp) {
        fprintf(stderr, "nv2a: fatal: %s\n", e->msg);
        abort();
    }
    *errp = e;
}

void error_prepend(Error **errp, const char *fmt, ...)
{
    va_list ap;
    char *pre;

    if (!errp || !*errp)
        return;
    va_start(ap, fmt);
    pre = g_strdup_vprintf(fmt, ap);
    va_end(ap);
    {
        char *msg = g_strconcat(pre, (*errp)->msg, NULL);
        g_free((*errp)->msg);
        (*errp)->msg = msg;
    }
    g_free(pre);
}

typedef struct {
    void *(*fn)(void *);
    void *arg;
    char name[16];
} ThreadStart;

static void *thread_trampoline(void *p)
{
    ThreadStart s = *(ThreadStart *)p;
    g_free(p);
#if defined(__linux__)
    pthread_setname_np(pthread_self(), s.name);
#endif
    return s.fn(s.arg);
}

void qemu_thread_create(QemuThread *thread, const char *name,
                        void *(*start_routine)(void *), void *arg, int mode)
{
    ThreadStart *s = g_new0(ThreadStart, 1);
    pthread_attr_t attr;

    s->fn = start_routine;
    s->arg = arg;
    /* "nv2a.pfifo_thread" is longer than the 15 characters Linux keeps. */
    g_strlcpy(s->name, g_str_has_prefix(name, "nv2a.") ? name + 5 : name,
              sizeof(s->name));
    pthread_attr_init(&attr);
    /* Shader generation and the renderer recurse deeper than the 8 MB default
     * comfortably allows on every distro's ulimit. */
    pthread_attr_setstacksize(&attr, 32u << 20);
    if (mode == QEMU_THREAD_DETACHED)
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&thread->t, &attr, thread_trampoline, s) != 0) {
        fprintf(stderr, "nv2a: cannot start thread %s\n", name);
        abort();
    }
    pthread_attr_destroy(&attr);
}

/* ---- The big lock ---- */

static pthread_mutex_t s_bql = PTHREAD_MUTEX_INITIALIZER;
static __thread bool s_bql_held;

void bql_lock(void)   { pthread_mutex_lock(&s_bql); s_bql_held = true; }
void bql_unlock(void) { s_bql_held = false; pthread_mutex_unlock(&s_bql); }
bool bql_locked(void) { return s_bql_held; }

/* ---- Clocks and timers ----
 *
 * One thread runs every armed timer's callback with the BQL held, as QEMU's
 * main loop does. Only PTIMER's alarm uses this, so a sorted list is plenty. */

int64_t qemu_clock_get_ns(QEMUClockType type)
{
    struct timespec ts;
    (void)type;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

static pthread_mutex_t s_timer_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_timer_cond = PTHREAD_COND_INITIALIZER;
static QEMUTimer *s_timers;
static bool s_timer_thread_started;

static void *timer_thread(void *unused)
{
    (void)unused;
    pthread_mutex_lock(&s_timer_lock);
    for (;;) {
        int64_t now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
        QEMUTimer *due = NULL;

        for (QEMUTimer *t = s_timers; t; t = t->next)
            if (t->expire_ns >= 0 && t->expire_ns <= now &&
                (!due || t->expire_ns < due->expire_ns))
                due = t;
        if (due) {
            due->expire_ns = -1;
            pthread_mutex_unlock(&s_timer_lock);
            bql_lock();
            due->cb(due->opaque);
            bql_unlock();
            pthread_mutex_lock(&s_timer_lock);
            continue;
        }

        {
            int64_t next = -1;
            for (QEMUTimer *t = s_timers; t; t = t->next)
                if (t->expire_ns >= 0 && (next < 0 || t->expire_ns < next))
                    next = t->expire_ns;
            if (next < 0) {
                pthread_cond_wait(&s_timer_cond, &s_timer_lock);
            } else {
                struct timespec ts = { next / 1000000000LL, next % 1000000000LL };
#ifdef __APPLE__
                int64_t delta = next - qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
                if (delta < 0)
                    delta = 0;
                ts = (struct timespec){delta / 1000000000LL, delta % 1000000000LL};
                pthread_cond_timedwait_relative_np(&s_timer_cond, &s_timer_lock, &ts);
#else
                pthread_cond_timedwait(&s_timer_cond, &s_timer_lock, &ts);
#endif
            }
        }
    }
    return NULL;
}

void timer_init_ns(QEMUTimer *t, QEMUClockType type, QEMUTimerCB *cb, void *opaque)
{
    (void)type;
    pthread_mutex_lock(&s_timer_lock);
    t->cb = cb;
    t->opaque = opaque;
    t->expire_ns = -1;
    if (!t->linked) {
        t->next = s_timers;
        s_timers = t;
        t->linked = true;
    }
    if (!s_timer_thread_started) {
        /* The waits are against CLOCK_MONOTONIC, like qemu_clock_get_ns. */
        pthread_condattr_t ca;
        pthread_t th;
        pthread_condattr_init(&ca);
#ifndef __APPLE__
        pthread_condattr_setclock(&ca, CLOCK_MONOTONIC);
#endif
        pthread_cond_init(&s_timer_cond, &ca);
        pthread_condattr_destroy(&ca);
        pthread_create(&th, NULL, timer_thread, NULL);
        pthread_detach(th);
        s_timer_thread_started = true;
    }
    pthread_mutex_unlock(&s_timer_lock);
}

void timer_mod(QEMUTimer *t, int64_t expire_ns)
{
    pthread_mutex_lock(&s_timer_lock);
    t->expire_ns = expire_ns < 0 ? 0 : expire_ns;
    pthread_cond_signal(&s_timer_cond);
    pthread_mutex_unlock(&s_timer_lock);
}

void timer_del(QEMUTimer *t)
{
    pthread_mutex_lock(&s_timer_lock);
    t->expire_ns = -1;
    pthread_mutex_unlock(&s_timer_lock);
}

bool timer_pending(QEMUTimer *t)
{
    bool armed;
    pthread_mutex_lock(&s_timer_lock);
    armed = t->expire_ns >= 0;
    pthread_mutex_unlock(&s_timer_lock);
    return armed;
}

/* ---- Dirty memory ----
 *
 * QEMU logs guest writes per page; nothing here sees guest writes, so each
 * client gets the answer that keeps it correct:
 *   NV2A_TEX  always "possibly dirty": the texture cache then compares a hash
 *             of the contents and uploads only what changed.
 *   NV2A      (vertex data) always dirty: the ranges a draw reads are copied.
 *   NV2A_SURF never dirty: a render target is GPU-written, and re-uploading
 *             it from RAM on a guess would overwrite what was drawn. */

extern int  nv2a_dirty_enabled(void);
extern bool nv2a_dirty_test_and_clear(hwaddr addr, hwaddr size, unsigned client);
extern void nv2a_dirty_set(hwaddr addr, hwaddr size, unsigned client);

bool memory_region_test_and_clear_dirty(MemoryRegion *mr, hwaddr addr,
                                        hwaddr size, unsigned client)
{
    static int surf = -1;
    (void)mr;
    /* Real answers from page protection (host/nv2a_dirty.c). */
    if (nv2a_dirty_enabled())
        return nv2a_dirty_test_and_clear(addr, size, client);
    if (surf < 0)
        surf = getenv("NV2A_SURF_DIRTY") != NULL;
    /* Untracked: no CPU write into a cached surface can be seen, so do not
     * claim one (every surface would be thrown away on every use). */
    if (client == DIRTY_MEMORY_NV2A_SURF_CPU)
        return false;
    return client != DIRTY_MEMORY_NV2A_SURF || surf;
}

void memory_region_set_client_dirty(MemoryRegion *mr, hwaddr addr,
                                    hwaddr size, unsigned client)
{
    (void)mr;
    if (nv2a_dirty_enabled())
        nv2a_dirty_set(addr, size, client);
}

/* ---- VGA register file ---- */

uint32_t vga_ioport_read(void *opaque, uint32_t addr)
{
    VGACommonState *s = opaque;

    switch (addr) {
    case VGA_ATT_W:  return s->ar_flip_flop == 0 ? s->ar_index : 0;
    case VGA_ATT_R:  return (s->ar_index & 0x1f) < 21 ? s->ar[s->ar_index & 0x1f] : 0;
    case VGA_MIS_W:  return s->st00;
    case VGA_SEQ_I:  return s->sr_index;
    case VGA_SEQ_D:  return s->sr[s->sr_index];
    case VGA_MIS_R:  return s->msr;
    case VGA_GFX_I:  return s->gr_index;
    case VGA_GFX_D:  return s->gr[s->gr_index];
    case VGA_CRT_IC: case VGA_CRT_IM: return s->cr_index;
    case VGA_CRT_DC: case VGA_CRT_DM: return s->cr[s->cr_index];
    case VGA_IS1_RC: case VGA_IS1_RM:
        /* Reading input status resets the attribute flip-flop; the retrace
         * bits toggle so a poll for either edge finishes. */
        s->ar_flip_flop = 0;
        s->st01 ^= 0x09;
        return s->st01;
    default:
        return 0xff;
    }
}

void vga_ioport_write(void *opaque, uint32_t addr, uint32_t val)
{
    VGACommonState *s = opaque;

    val &= 0xff;
    switch (addr) {
    case VGA_ATT_W:
        if (s->ar_flip_flop == 0)
            s->ar_index = val & 0x3f;
        else if ((s->ar_index & 0x1f) < 21)
            s->ar[s->ar_index & 0x1f] = val;
        s->ar_flip_flop ^= 1;
        break;
    case VGA_MIS_W:  s->msr = val; break;
    case VGA_SEQ_I:  s->sr_index = val; break;
    case VGA_SEQ_D:  s->sr[s->sr_index] = val; break;
    case VGA_GFX_I:  s->gr_index = val; break;
    case VGA_GFX_D:  s->gr[s->gr_index] = val; break;
    case VGA_CRT_IC: case VGA_CRT_IM: s->cr_index = val; break;
    case VGA_CRT_DC: case VGA_CRT_DM: s->cr[s->cr_index] = val; break;
    default: break;
    }
}

static void vga_get_params(VGACommonState *s, VGADisplayParams *params)
{
    NV2AState *d = container_of(s, NV2AState, vga);

    params->line_offset = s->line_offset;
    params->start_addr = d->pcrtc.start / 4;
    params->line_compare = 0;
}

static void vga_get_resolution(VGACommonState *s, int *w, int *h)
{
    *w = s->width;
    *h = s->height;
}

/* ======================================================================
 * The device (from xemu nv2a.c)
 * ====================================================================== */

static NV2AState s_nv2a;
static NV2AHostConfig s_cfg;
static MemoryRegion s_vram;

void nv2a_update_irq(NV2AState *d)
{
    /* PFIFO */
    if (d->pfifo.pending_interrupts & d->pfifo.enabled_interrupts) {
        d->pmc.pending_interrupts |= NV_PMC_INTR_0_PFIFO;
    } else {
        d->pmc.pending_interrupts &= ~NV_PMC_INTR_0_PFIFO;
    }

    /* PCRTC */
    if (d->pcrtc.pending_interrupts & d->pcrtc.enabled_interrupts) {
        d->pmc.pending_interrupts |= NV_PMC_INTR_0_PCRTC;
    } else {
        d->pmc.pending_interrupts &= ~NV_PMC_INTR_0_PCRTC;
    }

    /* PGRAPH */
    if (d->pgraph.pending_interrupts & d->pgraph.enabled_interrupts) {
        d->pmc.pending_interrupts |= NV_PMC_INTR_0_PGRAPH;
    } else {
        d->pmc.pending_interrupts &= ~NV_PMC_INTR_0_PGRAPH;
    }

    /* PTIMER */
    if (d->ptimer.pending_interrupts & d->ptimer.enabled_interrupts) {
        d->pmc.pending_interrupts |= NV_PMC_INTR_0_PTIMER;
    } else {
        d->pmc.pending_interrupts &= ~NV_PMC_INTR_0_PTIMER;
    }

    if (s_cfg.set_irq)
        s_cfg.set_irq(d->pmc.pending_interrupts && d->pmc.enabled_interrupts);
}

DMAObject nv_dma_load(NV2AState *d, hwaddr dma_obj_address)
{
    assert(dma_obj_address < memory_region_size(&d->ramin));

    uint32_t *dma_obj = (uint32_t *)(d->ramin_ptr + dma_obj_address);
    uint32_t flags = ldl_le_p(dma_obj);
    uint32_t limit = ldl_le_p(dma_obj + 1);
    uint32_t frame = ldl_le_p(dma_obj + 2);

    return (DMAObject){
        .dma_class  = GET_MASK(flags, NV_DMA_CLASS),
        .dma_target = GET_MASK(flags, NV_DMA_TARGET),
        .address    = (frame & NV_DMA_ADDRESS) | GET_MASK(flags, NV_DMA_ADJUST),
        .limit      = limit,
    };
}

void *nv_dma_map(NV2AState *d, hwaddr dma_obj_address, hwaddr *len)
{
    DMAObject dma = nv_dma_load(d, dma_obj_address);

    /* TODO: Handle targets and classes properly */
    dma.address &= 0x07FFFFFF;
    assert(dma.address < memory_region_size(d->vram));
    *len = dma.limit;
    return d->vram_ptr + dma.address;
}

hwaddr nv_clip_gpu_tile_blit(NV2AState *d, hwaddr blit_base_address, hwaddr len)
{
    const uint32_t *regs = d->pfb.regs;
    hwaddr blit_end = blit_base_address + len;

    for (int i = 0; i < NV_NUM_GPU_TILES; ++i) {
        uint32_t base_and_flags = regs[NV_PFB_TILE_BASE_ADDRESS_AND_FLAGS(i)];
        if (!(base_and_flags & NV_PFB_TILE_FLAGS_VALID)) {
            continue;
        }
        uint32_t limit = regs[NV_PFB_TILE_LIMIT(i)];
        if (blit_base_address < limit && blit_end > limit) {
            return limit + 1 - blit_base_address;
        }
    }
    return len;
}

const NV2ABlockInfo blocktable[NV_NUM_BLOCKS] = {
#define ENTRY(NAME, LNAME, OFFSET, SIZE) [NV_##NAME] = {            \
        .name   = #NAME,                                                \
        .offset = OFFSET,                                               \
        .size   = SIZE,                                                 \
        .ops    = { .read = LNAME ## _read, .write = LNAME ## _write }, \
    }
    ENTRY(PMC,      pmc,      0x000000, 0x001000),
    ENTRY(PBUS,     pbus,     0x001000, 0x001000),
    ENTRY(PFIFO,    pfifo,    0x002000, 0x002000),
    ENTRY(PRMA,     prma,     0x007000, 0x001000),
    ENTRY(PVIDEO,   pvideo,   0x008000, 0x001000),
    ENTRY(PTIMER,   ptimer,   0x009000, 0x001000),
    ENTRY(PCOUNTER, pcounter, 0x00a000, 0x001000),
    ENTRY(PVPE,     pvpe,     0x00b000, 0x001000),
    ENTRY(PTV,      ptv,      0x00d000, 0x001000),
    ENTRY(PRMFB,    prmfb,    0x0a0000, 0x020000),
    ENTRY(PRMVIO,   prmvio,   0x0c0000, 0x001000),
    ENTRY(PFB,      pfb,      0x100000, 0x001000),
    ENTRY(PSTRAPS,  pstraps,  0x101000, 0x001000),
    ENTRY(PGRAPH,   pgraph,   0x400000, 0x002000),
    ENTRY(PCRTC,    pcrtc,    0x600000, 0x001000),
    ENTRY(PRMCIO,   prmcio,   0x601000, 0x001000),
    ENTRY(PRAMDAC,  pramdac,  0x680000, 0x001000),
    ENTRY(PRMDIO,   prmdio,   0x681000, 0x001000),
    ENTRY(USER,     user,     0x800000, 0x800000),
#undef ENTRY
};

/* Find the register block an aperture offset belongs to. Sorted by offset
 * and short, so a scan is as fast as anything cleverer. */
static const NV2ABlockInfo *block_for(uint32_t off, uint32_t *rel)
{
    for (int i = 0; i < NV_NUM_BLOCKS; i++) {
        const NV2ABlockInfo *b = &blocktable[i];
        if (b->name && off >= b->offset && off - b->offset < b->size) {
            *rel = off - (uint32_t)b->offset;
            return b;
        }
    }
    return NULL;
}

uint64_t nv2a_vk_mmio_read(uint32_t off, unsigned size)
{
    uint32_t rel;
    const NV2ABlockInfo *b;
    uint64_t v;

    if (off >= 0x700000 && off < 0x800000)          /* PRAMIN: plain memory */
        return size == 1 ? s_nv2a.ramin_ptr[off - 0x700000]
             : size == 2 ? lduw_le_p(s_nv2a.ramin_ptr + off - 0x700000)
             : ldl_le_p(s_nv2a.ramin_ptr + off - 0x700000);
    b = block_for(off, &rel);
    if (!b)
        return 0;
    bql_lock();
    v = b->ops.read(&s_nv2a, rel, size);
    bql_unlock();
    return v;
}

void nv2a_vk_mmio_write(uint32_t off, uint64_t val, unsigned size)
{
    uint32_t rel;
    const NV2ABlockInfo *b;

    if (off >= 0x700000 && off < 0x800000) {
        uint8_t *p = s_nv2a.ramin_ptr + off - 0x700000;
        if (size == 1) *p = (uint8_t)val;
        else if (size == 2) stw_le_p(p, (uint16_t)val);
        else stl_le_p(p, (uint32_t)val);
        return;
    }
    b = block_for(off, &rel);
    if (!b)
        return;
    bql_lock();
    b->ops.write(&s_nv2a, rel, val, size);
    bql_unlock();
}

/* The xbox kernel's AvSetDisplayMode programs the CRTC for a mode; no kernel
 * runs here, so the mode is decoded into what the display path asks for.
 *
 * AV mode words are 0xSSWWHHOO-ish: byte 2 selects the width class (1 = 640,
 * 2 = 720, 3 = 1280, 4 = 1920), the low byte the output standard. Heights
 * follow from the width class and whether the standard is PAL. */
void nv2a_vk_set_display_mode(uint32_t mode, uint32_t format, uint32_t pitch,
                              uint32_t fb)
{
    NV2AState *d = &s_nv2a;
    static const int widths[] = { 0, 640, 720, 1280, 1920 };
    unsigned wc = (mode >> 16) & 0xff;
    unsigned out = mode & 0xff;
    int bpp = (format == 0x12 || format == 0x1E || format == 0x06) ? 4 : 2;
    int w = wc < 5 ? widths[wc] : 0;
    int h;

    if (!w)
        w = pitch / bpp;
    if (w >= 1920)       h = 1080;
    else if (w >= 1280)  h = 720;
    else                 h = (out == 0x02 || out == 0x03) ? 576 : 480;

    bql_lock();
    d->vga.width = w;
    d->vga.height = h;
    d->vga.line_offset = pitch;
    d->vga.cr[0x28] = bpp == 4 ? 3 : 2;
    d->pcrtc.start = fb & 0x07FFFFFF;
    bql_unlock();
    fprintf(stderr, "nv2a: display mode 0x%08X -> %dx%d, pitch %u, scanout 0x%08X\n",
            mode, w, h, pitch, fb);
}

/* ---- vblank ----
 *
 * The CRTC's frame clock. Each tick raises the vblank interrupt, whose
 * handler is where D3D flips, and then asks the renderer to present the
 * scanout surface (without waiting for it: the PFIFO thread does it between
 * pushbuffer runs). Absolute deadlines, so the rate does not drift with
 * scheduling delays. */

static double s_refresh_hz = 60.0;
volatile unsigned nv2a_pcrtc_start_writes;
/* When the last vblank began (g_get_monotonic_time); the renderer paces
 * its early presents from it. */
volatile int64_t nv2a_vblank_us;
/* Nonzero while the host holds the game (its overlay): no vblanks, which
 * the title would otherwise count up and then catch up on, running at
 * several times the speed until it had. */
volatile int nv2a_vblank_hold;

/* Until the absolute CLOCK_MONOTONIC time t. Windows sleeps in whole
 * milliseconds (MinGW's clock_nanosleep among them), which made the 60 Hz
 * clock run unevenly: there in high-resolution naps (platform/vcpu.h). */
static void sleep_until(const struct timespec *t)
{
#ifdef _WIN32
    for (;;) {
        struct timespec now;
        int64_t ns;
        clock_gettime(CLOCK_MONOTONIC, &now);
        ns = (int64_t)(t->tv_sec - now.tv_sec) * 1000000000LL + (t->tv_nsec - now.tv_nsec);
        if (ns <= 0)
            return;
        vcpu_nap_us((unsigned)((ns + 999) / 1000));
    }
#elif defined(__APPLE__)
    for (;;) {
        struct timespec now, delay;
        clock_gettime(CLOCK_MONOTONIC, &now);
        int64_t ns = (int64_t)(t->tv_sec - now.tv_sec) * 1000000000LL + t->tv_nsec - now.tv_nsec;
        if (ns <= 0)
            return;
        delay = (struct timespec){ns / 1000000000LL, ns % 1000000000LL};
        nanosleep(&delay, NULL);
    }
#else
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, t, NULL);
#endif
}

#ifdef _WIN32
#define COBJMACROS
#include <dxgi.h>
/* The display's own vertical blank as the game's frame clock: V-Sync on and a screen at about 60 Hz.
 * On a fixed 60.0 Hz timer the game drifted against the monitor's refresh -- a "60 Hz" screen runs at
 * 59.95, or 60.0003 -- and whenever the two came round to the same moment, frames came out doubled and
 * dropped for a while, the frame counter still saying 60: smooth for minutes, then stuttering (issue
 * #17). Any other refresh (120, 144 Hz; anything outside 59-61 Hz) or V-Sync off keeps the timer.
 * Only with NGB_VBLANK_SYNC=1 for now: on the PC it was meant for, it made the stutter start at once. */
extern int nv2a_present_vsync(void);

static IDXGIOutput *display_output(void)
{
    static int tried;
    static IDXGIOutput *found;
    IDXGIFactory1 *f;
    POINT origin = { 0, 0 };
    HMONITOR primary = MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY);

    if (tried)
        return found;
    tried = 1;
    if (FAILED(CreateDXGIFactory1(&IID_IDXGIFactory1, (void **)&f)))
        return NULL;
    for (UINT a = 0; !found; a++) {
        IDXGIAdapter1 *ad;
        if (IDXGIFactory1_EnumAdapters1(f, a, &ad) == DXGI_ERROR_NOT_FOUND)
            break;
        for (UINT o = 0; !found; o++) {
            IDXGIOutput *out;
            DXGI_OUTPUT_DESC desc;
            if (IDXGIAdapter1_EnumOutputs(ad, o, &out) == DXGI_ERROR_NOT_FOUND)
                break;
            if (SUCCEEDED(IDXGIOutput_GetDesc(out, &desc)) && desc.Monitor == primary)
                found = out;
            else
                IDXGIOutput_Release(out);
        }
        IDXGIAdapter1_Release(ad);
    }
    IDXGIFactory1_Release(f);
    return found;
}

/* Waits for the display's next vertical blank and returns 1, or returns 0 at once when the timer is to
 * pace the frames instead. */
static int display_vblank(void)
{
    static int state = -1;   /* -1 not measured, 0 the timer, 1 the display */
    static int last_vsync = -1;
    IDXGIOutput *out;
    int vsync = nv2a_present_vsync();

    if (!getenv("NGB_VBLANK_SYNC") || vsync == 0) {
        last_vsync = vsync;
        return 0;
    }
    if (vsync != last_vsync) {   /* switched on: measured (again) */
        last_vsync = vsync;
        state = -1;
    }
    if (state == 0 || !(out = display_output()))
        return 0;
    if (state < 0) {   /* the refresh, over a few blanks: about 60 Hz, or the timer */
        LARGE_INTEGER fq, t0, t1;
        double ms;
        QueryPerformanceFrequency(&fq);
        if (FAILED(IDXGIOutput_WaitForVBlank(out))) {
            state = 0;
            return 0;
        }
        QueryPerformanceCounter(&t0);
        for (int i = 0; i < 6; i++)
            IDXGIOutput_WaitForVBlank(out);
        QueryPerformanceCounter(&t1);
        ms = (double)(t1.QuadPart - t0.QuadPart) * 1000.0 / (double)fq.QuadPart / 6.0;
        state = ms > 16.39 && ms < 16.95;   /* 59-61 Hz: the game's speed follows these ticks */
        fprintf(stderr, "[NV2A] display refresh %.3f Hz: the game's frames %s\n", 1000.0 / ms,
                state ? "follow the display's vertical blank" : "keep the 60 Hz timer");
        return state;   /* (just waited for one) */
    }
    if (FAILED(IDXGIOutput_WaitForVBlank(out))) {
        state = 0;
        return 0;
    }
    return 1;
}
#endif

static void *vblank_thread(void *unused)
{
    NV2AState *d = &s_nv2a;
    struct timespec next;
    long period_ns = (long)(1e9 / s_refresh_hz);
    unsigned ticks = 0;

    (void)unused;
    clock_gettime(CLOCK_MONOTONIC, &next);
    for (;;) {
#ifdef _WIN32
        if (display_vblank()) {
            clock_gettime(CLOCK_MONOTONIC, &next);   /* the timer resumes from here if it takes over */
        } else
#endif
        {
            next.tv_nsec += period_ns;
            while (next.tv_nsec >= 1000000000L) {
                next.tv_nsec -= 1000000000L;
                next.tv_sec++;
            }
            sleep_until(&next);
        }
        if (nv2a_vblank_hold) {
            clock_gettime(CLOCK_MONOTONIC, &next);   /* resume from now */
            continue;
        }
        __atomic_store_n(&nv2a_vblank_us, g_get_monotonic_time(),
                         __ATOMIC_RELEASE);

        /* NV2A_DUMP_SCANOUT=prefix: the scanout surface as it is in guest
         * RAM, every 5 s, as PPM (X8R8G8B8, linear). */
        ticks++;
        if (ticks % (unsigned)(s_refresh_hz * 5) == 0 &&
            getenv("NV2A_DUMP_SCANOUT")) {
            static int n;
            char path[512];
            FILE *f;
            int w = d->vga.width, h = d->vga.height;
            uint32_t pitch = d->vga.line_offset;
            const uint8_t *src = d->vram_ptr + (d->pcrtc.start & 0x07FFFFFF);
            snprintf(path, sizeof(path), "%s%03d.ppm",
                     getenv("NV2A_DUMP_SCANOUT"), n++);
            if ((f = fopen(path, "wb"))) {
                fprintf(f, "P6\n%d %d\n255\n", w, h);
                for (int y = 0; y < h; y++)
                    for (int x = 0; x < w; x++) {
                        const uint8_t *px = src + y * pitch + x * 4;
                        uint8_t rgb[3] = { px[2], px[1], px[0] };
                        fwrite(rgb, 1, 3, f);
                    }
                fclose(f);
            }
        }
        if (s_cfg.stats && ticks % (unsigned)(s_refresh_hz * 5) == 0) {
            extern unsigned long nv2a_host_presents, nv2a_host_present_blank;
            fprintf(stderr, "[NV2A] %lu flips, scanout 0x%08llX, presents %lu "
                            "(%lu without a picture), get/put %08X/%08X\n",
                    (unsigned long)g_nv2a_stats.frame_count,
                    (unsigned long long)d->pcrtc.start, nv2a_host_presents,
                    nv2a_host_present_blank,
                    d->pfifo.regs[NV_PFIFO_CACHE1_DMA_GET],
                    d->pfifo.regs[NV_PFIFO_CACHE1_DMA_PUT]);
            fprintf(stderr, "       wait nop %d flip %d ctx %d; intr pmc %08X/%08X "
                            "pgraph %08X/%08X pfifo %08X/%08X pcrtc %08X/%08X\n",
                    d->pgraph.waiting_for_nop, d->pgraph.waiting_for_flip,
                    d->pgraph.waiting_for_context_switch,
                    d->pmc.pending_interrupts, d->pmc.enabled_interrupts,
                    d->pgraph.pending_interrupts, d->pgraph.enabled_interrupts,
                    d->pfifo.pending_interrupts, d->pfifo.enabled_interrupts,
                    d->pcrtc.pending_interrupts, d->pcrtc.enabled_interrupts);
            {
                /* The last whole frame's renderer counters. */
                unsigned fp = (g_nv2a_stats.frame_ptr + NV2A_PROF_NUM_FRAMES - 1)
                              % NV2A_PROF_NUM_FRAMES;
                fprintf(stderr, "       last frame %d ms:",
                        g_nv2a_stats.frame_history[fp].mspf);
                for (unsigned c = 0; c < NV2A_PROF__COUNT; c++)
                    if (g_nv2a_stats.frame_history[fp].counters[c])
                        fprintf(stderr, " %s=%d",
                                nv2a_profile_get_counter_name(c),
                                g_nv2a_stats.frame_history[fp].counters[c]);
                fprintf(stderr, "\n");
            }
            fprintf(stderr, "       pgraph fifo %08X; push0 %08X dma_push %08X "
                            "dma_state %08X pull0 %08X\n",
                    d->pgraph.regs_[NV_PGRAPH_FIFO],
                    d->pfifo.regs[NV_PFIFO_CACHE1_PUSH0],
                    d->pfifo.regs[NV_PFIFO_CACHE1_DMA_PUSH],
                    d->pfifo.regs[NV_PFIFO_CACHE1_DMA_STATE],
                    d->pfifo.regs[NV_PFIFO_CACHE1_PULL0]);
        }

        unsigned flips = __atomic_load_n(&nv2a_pcrtc_start_writes,
                                         __ATOMIC_ACQUIRE);
        bql_lock();
        d->pcrtc.pending_interrupts |= NV_PCRTC_INTR_0_VBLANK;
        d->pcrtc.raster = 0;
        nv2a_update_irq(d);
        bql_unlock();

        /* Present after the flip, not before it. D3D flips in its vblank
         * handler, by writing the new scanout address; presenting at the
         * tick, ahead of the interrupt, showed the previous frame and left
         * every new one waiting a whole refresh -- 16.7 ms of input lag on
         * top of everything else. Give the handler a few milliseconds to
         * flip (it takes well under one); a tick with no flip still
         * presents, for pictures that change without one. */
        if (!s_cfg.headless) {
            /* With another renderer drawing (ngb-recomp's Direct3D one), its
             * thread presents each picture as it hands it over
             * (present_sdl.c); the tick presents only one it did not, or the
             * screen while none come (a loading pause). */
            extern volatile unsigned nv2a_ext_pictures, nv2a_ext_presented;
            static unsigned last_pics;
            static int64_t ext_seen_us;
            unsigned pics = __atomic_load_n(&nv2a_ext_pictures, __ATOMIC_ACQUIRE);
            int64_t tick_us = __atomic_load_n(&nv2a_vblank_us, __ATOMIC_ACQUIRE);
            if (pics != last_pics)
                ext_seen_us = tick_us;
            last_pics = pics;
            static int direct = -1;   /* NV2A_PACE=tick: all presents at the tick, as before */
            if (direct < 0)
                direct = !(getenv("NV2A_PACE") && !strcmp(getenv("NV2A_PACE"), "tick"));
            bool ext = direct && ext_seen_us && tick_us - ext_seen_us < 100000;
            for (int i = 0; i < 40 && __atomic_load_n(&nv2a_pcrtc_start_writes,
                                                      __ATOMIC_ACQUIRE) == flips; i++)
                vcpu_nap_us(100);
            if (ext) {
                vcpu_nap_us(4000);   /* (its picture comes about 2 ms after the tick) */
                if (__atomic_load_n(&nv2a_ext_presented, __ATOMIC_ACQUIRE) ==
                    __atomic_load_n(&nv2a_ext_pictures, __ATOMIC_ACQUIRE))
                    continue;
            }
            qemu_mutex_lock(&d->pfifo.lock);
            qatomic_set(&d->pgraph.sync_pending, true);
            pfifo_kick(d);
            qemu_mutex_unlock(&d->pfifo.lock);
        }
    }
    return NULL;
}

static void nv2a_reset(NV2AState *d)
{
    memset(d->pfifo.regs, 0, sizeof(d->pfifo.regs));
    memset(d->pgraph.regs_, 0, sizeof(d->pgraph.regs_));
    memset(d->pvideo.regs, 0, sizeof(d->pvideo.regs));

    d->pcrtc.start = 0;
    d->pramdac.core_clock_coeff = 0x00011C01; /* 189MHz...? */
    d->pramdac.core_clock_freq = 233333324;
    d->pramdac.memory_clock_coeff = 0;
    d->pramdac.video_clock_coeff = 0x0003C20D; /* 25182Khz...? */

    d->pfifo.regs[NV_PFIFO_CACHE1_STATUS] |= NV_PFIFO_CACHE1_STATUS_LOW_MARK;

    d->vga.msr = VGA_MIS_COLOR;
    d->vga.ar[0x10] = VGA_AR_ENABLE_DISPLAY;

    d->pgraph.waiting_for_nop = false;
    d->pgraph.waiting_for_flip = false;
    d->pgraph.waiting_for_context_switch = false;

    d->pmc.pending_interrupts = 0;
    d->pfifo.pending_interrupts = 0;
    d->ptimer.pending_interrupts = 0;
    ptimer_reset(d);
    d->pcrtc.pending_interrupts = 0;

    for (int i = 0; i < 256; i++) {
        d->puserdac.palette[i*3]   = i;
        d->puserdac.palette[i*3+1] = i;
        d->puserdac.palette[i*3+2] = i;
    }
}

int nv2a_vk_init(const NV2AHostConfig *cfg)
{
    NV2AState *d = &s_nv2a;
    QemuThread th;
    PCIDevice *pci = &d->parent_obj;

    s_cfg = *cfg;
    g_config.display.vulkan.validation_layers = cfg->validation;
    g_config.display.quality.surface_scale = cfg->surface_scale > 0
                                             ? cfg->surface_scale : 1;
    g_config.display.renderer = cfg->headless ? CONFIG_DISPLAY_RENDERER_NULL
                                              : CONFIG_DISPLAY_RENDERER_VULKAN;
    if (cfg->refresh_hz > 0)
        s_refresh_hz = cfg->refresh_hz;
    {   /* NV2A_REFRESH_HZ: the vertical blank's rate (testing) */
        const char *e = getenv("NV2A_REFRESH_HZ");
        if (e && atof(e) >= 10)
            s_refresh_hz = atof(e);
    }

    /* PCI identity: GeForce3-class NV2A A1, as PBUS reports it. */
    pci_set_long(pci->config + PCI_VENDOR_ID, 0x02A010DE);
    pci_set_long(pci->config + PCI_CLASS_REVISION, 0x030000A1);

    /* The Xbox is UMA: VRAM is guest RAM, physical address 0 upward. */
    s_vram.ram_ptr = cfg->ram;
    s_vram.size = cfg->ram_size;
    d->vram = &s_vram;
    d->vram_ptr = cfg->ram;
    d->ramin.ram_ptr = cfg->ramin;
    d->ramin.size = 0x100000;
    d->ramin_ptr = cfg->ramin;

    d->vga.get_params = vga_get_params;
    d->vga.get_resolution = vga_get_resolution;
    d->vga.width = 640;
    d->vga.height = 480;
    d->vga.line_offset = 640 * 4;
    /* The CRTC's interlace register reads 0xFF when progressive; zero would
     * have xemu's display double the height (its 1080i path). */
    d->vga.cr[NV_PRMCIO_INTERLACE_MODE] = NV_PRMCIO_INTERLACE_MODE_DISABLED;

    qemu_mutex_init(&d->pfifo.lock);
    qemu_cond_init(&d->pfifo.fifo_cond);
    qemu_cond_init(&d->pfifo.fifo_idle_cond);
    ptimer_init(d);
    nv2a_reset(d);

    pgraph_init(d);
    qemu_thread_create(&d->pfifo.thread, "nv2a.pfifo_thread", pfifo_thread, d,
                       QEMU_THREAD_DETACHED);
    qemu_thread_create(&th, "nv2a.vblank", vblank_thread, NULL,
                       QEMU_THREAD_DETACHED);
    return 0;
}

NV2AState *nv2a_vk_state(void)
{
    return &s_nv2a;
}

/* ---- vCPU work (see exec/memory.h) ---- */

typedef struct {
    run_on_cpu_func func;
    run_on_cpu_data data;
} CpuWork;

static void *cpu_work_thread(void *p)
{
    CpuWork w = *(CpuWork *)p;
    g_free(p);
    w.func(NULL, w.data);
    return NULL;
}

void async_safe_run_on_cpu(CPUState *cpu, run_on_cpu_func func,
                           run_on_cpu_data data)
{
    CpuWork *w = g_new(CpuWork, 1);
    QemuThread th;

    (void)cpu;
    w->func = func;
    w->data = data;
    qemu_thread_create(&th, "nv2a.cpu_work", cpu_work_thread, w,
                       QEMU_THREAD_DETACHED);
}

/* ---- Sampling (Linux) ----
 *
 * nv2a_vk_sample_threads() makes the PFIFO thread print its backtrace: the
 * host's own SIGUSR1 sampler calls it after sampling the guest thread, so one
 * `kill -USR1` shows both sides of a stall. */
#ifndef _WIN32
#include <execinfo.h>
#include <signal.h>

static void sample_pfifo(int sig)
{
    void *frames[48];
    int n = backtrace(frames, 48);
    (void)sig;
    fprintf(stderr, "[SAMPLE] nv2a pfifo thread:\n");
    backtrace_symbols_fd(frames, n, 2);
}
#endif

void nv2a_vk_sample_threads(void)
{
#ifndef _WIN32
    static bool installed;
    if (!installed) {
        signal(SIGUSR2, sample_pfifo);
        installed = true;
    }
    pthread_kill(s_nv2a.pfifo.thread.t, SIGUSR2);
#endif
    if (getenv("NV2A_DUMP_CONSTANTS")) {
        /* The vertex shader constants as last uploaded: where a title keeps
         * its view/projection matrices and camera position. */
        PGRAPHState *pg = &s_nv2a.pgraph;
        for (int i = 0; i < NV2A_VERTEXSHADER_CONSTANTS; i++) {
            float *f = (float *)pg->vsh_constants[i];
            if (f[0] || f[1] || f[2] || f[3])
                fprintf(stderr, "  [VSHC] c%-3d %12g %12g %12g %12g\n",
                        i - 96, f[0], f[1], f[2], f[3]);
        }
    }
}

/* Debugging (NGB_HANG_DUMP, src/main.c): the GPU's command queue and interrupts in one line --
 * whether a stalled title waits on commands the GPU never saw, or on a GPU that stopped. Read
 * without the locks: a snapshot, possibly torn. */
void nv2a_debug_state(char *buf, int n)
{
    NV2AState *d = &s_nv2a;
    PGRAPHState *pg = &d->pgraph;
    snprintf(buf, (size_t)n,
             "[NV2A] put %08X get %08X dma_push %08X dma_state %08X push0 %X pull0 %X halt %d kick %d"
             " | waiting flip %d nop %d ctx %d surface %08X"
             " | intr pending/enabled pmc %X/%X pfifo %X/%X pgraph %X/%X pcrtc %X/%X",
             d->pfifo.regs[NV_PFIFO_CACHE1_DMA_PUT], d->pfifo.regs[NV_PFIFO_CACHE1_DMA_GET],
             d->pfifo.regs[NV_PFIFO_CACHE1_DMA_PUSH], d->pfifo.regs[NV_PFIFO_CACHE1_DMA_STATE],
             d->pfifo.regs[NV_PFIFO_CACHE1_PUSH0], d->pfifo.regs[NV_PFIFO_CACHE1_PULL0],
             d->pfifo.halt, d->pfifo.fifo_kick, pg->waiting_for_flip, pg->waiting_for_nop,
             pg->waiting_for_context_switch, pgraph_reg_r(pg, NV_PGRAPH_SURFACE),
             d->pmc.pending_interrupts, d->pmc.enabled_interrupts, d->pfifo.pending_interrupts,
             d->pfifo.enabled_interrupts, pg->pending_interrupts, pg->enabled_interrupts,
             d->pcrtc.pending_interrupts, d->pcrtc.enabled_interrupts);
}
