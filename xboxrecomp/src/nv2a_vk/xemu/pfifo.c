/*
 * QEMU Geforce NV2A implementation
 *
 * Copyright (c) 2012 espes
 * Copyright (c) 2015 Jannik Vogel
 * Copyright (c) 2018-2025 Matt Borgerson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "nv2a_int.h"

void nv2a_vk_progress(void);
#include "../host/threadload.h"

typedef struct RAMHTEntry {
    uint32_t handle;
    hwaddr instance;
    enum FIFOEngine engine;
    unsigned int channel_id : 5;
    bool valid;
} RAMHTEntry;

static void pfifo_run_pusher(NV2AState *d);
static uint32_t ramht_hash(NV2AState *d, uint32_t handle);
static RAMHTEntry ramht_lookup(NV2AState *d, uint32_t handle);

/* PFIFO - MMIO and DMA FIFO submission to PGRAPH and VPE */
uint64_t pfifo_read(void *opaque, hwaddr addr, unsigned int size)
{
    NV2AState *d = (NV2AState *)opaque;

    qemu_mutex_lock(&d->pfifo.lock);

    uint64_t r = 0;
    switch (addr) {
    case NV_PFIFO_INTR_0:
        r = d->pfifo.pending_interrupts;
        break;
    case NV_PFIFO_INTR_EN_0:
        r = d->pfifo.enabled_interrupts;
        break;
    case NV_PFIFO_RUNOUT_STATUS:
        r = NV_PFIFO_RUNOUT_STATUS_LOW_MARK; /* low mark empty */
        break;
    default:
        r = d->pfifo.regs[addr];
        break;
    }

    qemu_mutex_unlock(&d->pfifo.lock);

    nv2a_reg_log_read(NV_PFIFO, addr, size, r);
    return r;
}

void pfifo_write(void *opaque, hwaddr addr, uint64_t val, unsigned int size)
{
    NV2AState *d = (NV2AState *)opaque;

    nv2a_reg_log_write(NV_PFIFO, addr, size, val);

    qemu_mutex_lock(&d->pfifo.lock);

    switch (addr) {
    case NV_PFIFO_INTR_0:
        d->pfifo.pending_interrupts &= ~val;
        nv2a_update_irq(d);
        break;
    case NV_PFIFO_INTR_EN_0:
        d->pfifo.enabled_interrupts = val;
        nv2a_update_irq(d);
        break;
    default:
        /* xboxrecomp: the pusher works through a batch of methods holding only pgraph.lock (pfifo_run_pusher) */
        qemu_mutex_lock(&d->pgraph.lock);
        d->pfifo.regs[addr] = val;
        qemu_mutex_unlock(&d->pgraph.lock);
        break;
    }

    pfifo_kick(d);

    qemu_mutex_unlock(&d->pfifo.lock);
}

void pfifo_kick(NV2AState *d)
{
    d->pfifo.fifo_kick = true;
    qemu_cond_broadcast(&d->pfifo.fifo_cond);
}

/* xboxrecomp: the push buffer read only as far as a renderer drawing after the title's calls has drawn (the port's
 * src/d3d/gpu/threaded.cpp): every reference, fence and semaphore the title waits on then follows that drawing, as
 * the console's GPU's would -- else the title reused vertex and texture memory still to be drawn. on: read up to
 * put, a DMA_PUT the title set (the pusher stops there exactly: PUT is at a method's end); off: as far as the
 * title has put. */
/* one word, on in the high half: the pusher reads it without the lock, and two words could be seen one old and
 * one new (the gate closing at exit: on with put 0, the pusher running on to 0 through stale words) */
static uint64_t s_gate;
void nv2a_pfifo_gate(int on, uint32_t put)
{
    NV2AState *d = g_nv2a;
    if (!d)
        return;
    qemu_mutex_lock(&d->pfifo.lock);
    __atomic_store_n(&s_gate, ((uint64_t)(on != 0) << 32) | put, __ATOMIC_RELEASE);
    pfifo_kick(d);
    qemu_mutex_unlock(&d->pfifo.lock);
}

/* The title's DMA_PUT now, for a recording to be read up to once drawn. */
uint32_t nv2a_pfifo_put(void)
{
    return g_nv2a ? qatomic_read(&g_nv2a->pfifo.regs[NV_PFIFO_CACHE1_DMA_PUT]) : 0;
}

static bool can_fifo_access(NV2AState *d) {
    return qatomic_read(&d->pgraph.regs_[NV_PGRAPH_FIFO]) &
           NV_PGRAPH_FIFO_ACCESS;
}

/* If NV097_FLIP_STALL was executed, check if the flip has completed.
 * This will usually happen in the VSYNC interrupt handler.
 */
static bool is_flip_stall_complete(NV2AState *d)
{
    PGRAPHState *pg = &d->pgraph;

    uint32_t s = pgraph_reg_r(pg, NV_PGRAPH_SURFACE);

    NV2A_DPRINTF("flip stall read: %d, write: %d, modulo: %d\n",
        GET_MASK(s, NV_PGRAPH_SURFACE_READ_3D),
        GET_MASK(s, NV_PGRAPH_SURFACE_WRITE_3D),
        GET_MASK(s, NV_PGRAPH_SURFACE_MODULO_3D));

    if (GET_MASK(s, NV_PGRAPH_SURFACE_READ_3D)
        != GET_MASK(s, NV_PGRAPH_SURFACE_WRITE_3D)) {
        return true;
    }

    return false;
}

extern int nv2a_pace_on;
extern void nv2a_pace_stall(int64_t us);
static int64_t s_pace_stall_since;   /* (NV2A_PACE_LOG: when this flip stall began) */

static bool pfifo_stall_for_flip(NV2AState *d, bool graph_held)
{
    bool should_stall = false;

    if (qatomic_read(&d->pgraph.waiting_for_flip)) {
        if (!graph_held)
            qemu_mutex_lock(&d->pgraph.lock);
        if (!is_flip_stall_complete(d)) {
            should_stall = true;
            if (nv2a_pace_on > 0 && !s_pace_stall_since)
                s_pace_stall_since = g_get_monotonic_time();
        } else {
            d->pgraph.waiting_for_flip = false;
            if (s_pace_stall_since) {
                nv2a_pace_stall(g_get_monotonic_time() - s_pace_stall_since);
                s_pace_stall_since = 0;
            }
        }
        if (!graph_held)
            qemu_mutex_unlock(&d->pgraph.lock);
    }

    return should_stall;
}

static bool pfifo_puller_should_stall(NV2AState *d, bool graph_held)
{
    return pfifo_stall_for_flip(d, graph_held) || qatomic_read(&d->pgraph.waiting_for_nop) ||
           qatomic_read(&d->pgraph.waiting_for_context_switch) ||
           !can_fifo_access(d);
}

/* xboxrecomp: pfifo.lock is let go and pgraph.lock taken for a batch of methods (pfifo_run_pusher, *graph_held),
 * not around each one: the swap per method was some 3 million lock operations a second. The order is xemu's,
 * pfifo.lock before pgraph.lock, and never both: a batch ends by letting pgraph.lock go before pfifo.lock is
 * taken again. */
static void pfifo_graph_begin(NV2AState *d, bool *graph_held)
{
    if (!*graph_held) {
        qemu_mutex_unlock(&d->pfifo.lock);
        qemu_mutex_lock(&d->pgraph.lock);
        *graph_held = true;
    }
}

static void pfifo_graph_end(NV2AState *d, bool *graph_held)
{
    if (*graph_held) {
        qemu_mutex_unlock(&d->pgraph.lock);
        qemu_mutex_lock(&d->pfifo.lock);
        *graph_held = false;
    }
}

static ssize_t pfifo_run_puller(NV2AState *d, uint32_t method_entry,
                                uint32_t parameter, uint32_t *parameters,
                                size_t num_words_available,
                                size_t max_lookahead_words, bool *graph_held)
{
    if (pfifo_puller_should_stall(d, *graph_held)) {
        return -1;
    }

    uint32_t *pull0 = &d->pfifo.regs[NV_PFIFO_CACHE1_PULL0];
    uint32_t *pull1 = &d->pfifo.regs[NV_PFIFO_CACHE1_PULL1];
    uint32_t *engine_reg = &d->pfifo.regs[NV_PFIFO_CACHE1_ENGINE];
    uint32_t *status = &d->pfifo.regs[NV_PFIFO_CACHE1_STATUS];
    ssize_t num_proc = -1;

    // TODO think more about locking

    if (!GET_MASK(*pull0, NV_PFIFO_CACHE1_PULL0_ACCESS) ||
        (*status & NV_PFIFO_CACHE1_STATUS_LOW_MARK)) {
        return -1;
    }

    uint32_t method = method_entry & 0x1FFC;
    uint32_t subchannel =
        GET_MASK(method_entry, NV_PFIFO_CACHE1_METHOD_SUBCHANNEL);
    bool inc = !GET_MASK(method_entry, NV_PFIFO_CACHE1_METHOD_TYPE);

    if (method == 0) {
        RAMHTEntry entry = ramht_lookup(d, parameter);
        assert(entry.valid);
        // assert(entry.channel_id == state->channel_id);
        assert(entry.engine == ENGINE_GRAPHICS);

        /* the engine is bound to the subchannel */
        assert(subchannel < 8);
        SET_MASK(*engine_reg, 3 << (4*subchannel), entry.engine);
        SET_MASK(*pull1, NV_PFIFO_CACHE1_PULL1_ENGINE, entry.engine);

        pfifo_graph_begin(d, graph_held);

        // Switch contexts if necessary
        if (can_fifo_access(d)) {
            pgraph_context_switch(d, entry.channel_id);
            if (!d->pgraph.waiting_for_context_switch) {
                num_proc =
                    pgraph_method(d, subchannel, 0, entry.instance, parameters,
                                  num_words_available, max_lookahead_words, inc);
            }
        }

    } else if (method >= 0x100) {
        // method passed to engine

        /* methods that take objects.
         * TODO: Check this range is correct for the nv2a */
        if (method >= 0x180 && method < 0x200) {
            //bql_lock();
            RAMHTEntry entry = ramht_lookup(d, parameter);
            assert(entry.valid);
            // assert(entry.channel_id == state->channel_id);
            parameter = entry.instance;
            //bql_unlock();
        }

        enum FIFOEngine engine = GET_MASK(*engine_reg, 3 << (4*subchannel));
        assert(engine == ENGINE_GRAPHICS);
        SET_MASK(*pull1, NV_PFIFO_CACHE1_PULL1_ENGINE, engine);

        pfifo_graph_begin(d, graph_held);

        if (can_fifo_access(d)) {
            num_proc =
                pgraph_method(d, subchannel, method, parameter, parameters,
                              num_words_available, max_lookahead_words, inc);
        }
    } else {
        assert(!"Unrecognized pfifo puller method");
    }

    if (num_proc > 0) {
        *status |= NV_PFIFO_CACHE1_STATUS_LOW_MARK;
    }

    return num_proc;
}

/* xboxrecomp: methods the pusher passes over without handing them to PGRAPH. This port draws with its own
 * renderer from the title's Direct3D calls; the NV2A model only keeps the time -- fences, flips, notifies,
 * reports -- and its own drawing is off (nv2a_vk_set_draws_off). These are the 3D object's (subchannel 0)
 * bulky state for that drawing alone: combiners, texgen, matrices, fog/specular/viewport/eye parameters,
 * the vertex program and its constants, lights, vertex arrays, inline vertex data and the texture stages.
 * Every method else -- surfaces, BEGIN_END, clears, semaphores, flips, NOPs, reports, objects and context
 * DMAs -- still goes through PGRAPH. NV2A_PFIFO_SKIP=0: every method to PGRAPH as before. */
static bool pfifo_skippable(uint32_t subchannel, uint32_t m)
{
    if (subchannel != 0)
        return false;
    return (m >= 0x0260 && m <= 0x028C) || (m >= 0x03C0 && m <= 0x03CC) ||
           (m >= 0x0440 && m <= 0x07BC) || (m >= 0x09D0 && m <= 0x0BFC) ||
           (m >= 0x0C00 && m <= 0x13FC) || (m >= 0x1720 && m <= 0x17BC) ||
           m == 0x17E0 || m == 0x17E4 || m == 0x17E8 ||
           (m >= 0x1800 && m <= 0x1AFC) || (m >= 0x1B00 && m <= 0x1BFC) ||
           (m >= 0x1E40 && m <= 0x1E60);
}

bool nv2a_vk_draws_off(void);   /* pgraph/vk/instance.c: the port's renderer draws */

/* Only while the NV2A model draws nothing: with NGB_RENDERER=xemu, or a program on the model's drawing, every
 * method still counts. */
static int pfifo_skip_enabled(void)
{
    static int on = -1;
    if (on < 0) {
        const char *e = getenv("NV2A_PFIFO_SKIP");
        on = !(e && *e == '0');
    }
    return on && nv2a_vk_draws_off();
}

static bool pfifo_pusher_should_stall(NV2AState *d)
{
    return !can_fifo_access(d) ||
           qatomic_read(&d->pgraph.waiting_for_nop);
}

static void pfifo_run_pusher(NV2AState *d)
{
    uint32_t *push0 = &d->pfifo.regs[NV_PFIFO_CACHE1_PUSH0];
    uint32_t *push1 = &d->pfifo.regs[NV_PFIFO_CACHE1_PUSH1];
    uint32_t *dma_subroutine = &d->pfifo.regs[NV_PFIFO_CACHE1_DMA_SUBROUTINE];
    uint32_t *dma_state = &d->pfifo.regs[NV_PFIFO_CACHE1_DMA_STATE];
    uint32_t *dma_push = &d->pfifo.regs[NV_PFIFO_CACHE1_DMA_PUSH];
    uint32_t *dma_get = &d->pfifo.regs[NV_PFIFO_CACHE1_DMA_GET];
    uint32_t *dma_put = &d->pfifo.regs[NV_PFIFO_CACHE1_DMA_PUT];
    uint32_t *dma_dcount = &d->pfifo.regs[NV_PFIFO_CACHE1_DMA_DCOUNT];
    uint32_t *status = &d->pfifo.regs[NV_PFIFO_CACHE1_STATUS];

    if (!GET_MASK(*push0, NV_PFIFO_CACHE1_PUSH0_ACCESS) ||
        !GET_MASK(*dma_push, NV_PFIFO_CACHE1_DMA_PUSH_ACCESS) ||
        GET_MASK(*dma_push, NV_PFIFO_CACHE1_DMA_PUSH_STATUS)) {
        return;
    }

    // TODO: should we become busy here??
    // NV_PFIFO_CACHE1_DMA_PUSH_STATE _BUSY

    unsigned int channel_id = GET_MASK(*push1,
                                       NV_PFIFO_CACHE1_PUSH1_CHID);


    /* Channel running DMA mode */
    uint32_t channel_modes = d->pfifo.regs[NV_PFIFO_MODE];
    assert(channel_modes & (1 << channel_id));

    assert(GET_MASK(*push1, NV_PFIFO_CACHE1_PUSH1_MODE)
            == NV_PFIFO_CACHE1_PUSH1_MODE_DMA);

    /* We're running so there should be no pending errors... */
    assert(GET_MASK(*dma_state, NV_PFIFO_CACHE1_DMA_STATE_ERROR)
            == NV_PFIFO_CACHE1_DMA_STATE_ERROR_NONE);

    hwaddr dma_instance =
        GET_MASK(d->pfifo.regs[NV_PFIFO_CACHE1_DMA_INSTANCE],
                 NV_PFIFO_CACHE1_DMA_INSTANCE_ADDRESS) << 4;

    hwaddr dma_len;
    uint8_t *dma = nv_dma_map(d, dma_instance, &dma_len);

    /* xboxrecomp: up to kBatch methods to a pgraph.lock (pfifo_graph_begin): long enough to make the locking
     * cheap, short enough that a thread waiting for pgraph.lock (a register write, a surface download) waits
     * little */
    enum { kBatch = 64 };
    bool graph_held = false;
    unsigned batch = 0, since_progress = 0;

    while (!pfifo_pusher_should_stall(d)) {
        if (graph_held && ++batch >= kBatch) {
            pfifo_graph_end(d, &graph_held);
            batch = 0;
        }
        uint32_t dma_get_v = *dma_get;
        uint64_t gate = __atomic_load_n(&s_gate, __ATOMIC_ACQUIRE);
        uint32_t dma_put_v = (gate >> 32) ? (uint32_t)gate : qatomic_read(dma_put);
        if (dma_get_v == dma_put_v) break;
        if (dma_get_v >= dma_len) {
            assert(!"Dma value is out of range in PFIFO pusher");
            SET_MASK(*dma_state, NV_PFIFO_CACHE1_DMA_STATE_ERROR,
                     NV_PFIFO_CACHE1_DMA_STATE_ERROR_PROTECTION);
            break;
        }

        size_t num_words_available = dma_put_v - dma_get_v;
        assert(num_words_available % 4 == 0);
        num_words_available /= 4;

        uint32_t *word_ptr = (uint32_t*)(dma + dma_get_v);
        uint32_t word = ldl_le_p(word_ptr);
        dma_get_v += 4;

        uint32_t method_type =
            GET_MASK(*dma_state, NV_PFIFO_CACHE1_DMA_STATE_METHOD_TYPE);
        uint32_t method_subchannel =
            GET_MASK(*dma_state, NV_PFIFO_CACHE1_DMA_STATE_SUBCHANNEL);
        uint32_t method =
            GET_MASK(*dma_state, NV_PFIFO_CACHE1_DMA_STATE_METHOD) << 2;
        uint32_t method_count =
            GET_MASK(*dma_state, NV_PFIFO_CACHE1_DMA_STATE_METHOD_COUNT);

        uint32_t subroutine_state =
            GET_MASK(*dma_subroutine, NV_PFIFO_CACHE1_DMA_SUBROUTINE_STATE);

        if (method_count && pfifo_skip_enabled() && pfifo_skippable(method_subchannel, method)) {
            /* a run of skippable methods (pfifo_skippable): taken as read, all at once */
            if (pfifo_puller_should_stall(d, graph_held)) {
                break;
            }
            uint32_t n = MIN(method_count, (uint32_t)num_words_available), k = 1;
            if (method_type == NV_PFIFO_CACHE1_DMA_STATE_METHOD_TYPE_INC) {
                while (k < n && pfifo_skippable(method_subchannel, method + 4 * k)) {
                    k++;
                }
            } else {
                k = n;
            }
            d->pfifo.regs[NV_PFIFO_CACHE1_DMA_DATA_SHADOW] = ldl_le_p(word_ptr + (k - 1));
            dma_get_v += (k - 1) * 4;
            if (method_type == NV_PFIFO_CACHE1_DMA_STATE_METHOD_TYPE_INC) {
                SET_MASK(*dma_state, NV_PFIFO_CACHE1_DMA_STATE_METHOD, (method + 4 * k) >> 2);
            }
            SET_MASK(*dma_state, NV_PFIFO_CACHE1_DMA_STATE_METHOD_COUNT, method_count - k);
            (*dma_dcount) += k;
        } else if (method_count) {
            /* data word of methods command */
            d->pfifo.regs[NV_PFIFO_CACHE1_DMA_DATA_SHADOW] = word;

            assert((method & 3) == 0);
            uint32_t method_entry = 0;
            SET_MASK(method_entry, NV_PFIFO_CACHE1_METHOD_ADDRESS, method >> 2);
            SET_MASK(method_entry, NV_PFIFO_CACHE1_METHOD_TYPE, method_type);
            SET_MASK(method_entry, NV_PFIFO_CACHE1_METHOD_SUBCHANNEL,
                     method_subchannel);

            *status &= ~NV_PFIFO_CACHE1_STATUS_LOW_MARK;

            ssize_t num_words_processed =
                pfifo_run_puller(d, method_entry, word, word_ptr,
                                 MIN(method_count, num_words_available),
                                 num_words_available, &graph_held);
            if (num_words_processed < 0) {
                break;
            }

            dma_get_v += (num_words_processed-1)*4;

            if (method_type == NV_PFIFO_CACHE1_DMA_STATE_METHOD_TYPE_INC) {
                SET_MASK(*dma_state, NV_PFIFO_CACHE1_DMA_STATE_METHOD,
                         (method + 4*num_words_processed) >> 2);
            }
            SET_MASK(*dma_state, NV_PFIFO_CACHE1_DMA_STATE_METHOD_COUNT,
                     method_count - MIN(method_count, num_words_processed));

            (*dma_dcount) += num_words_processed;
        } else {
            /* no command active - this is the first word of a new one */
            d->pfifo.regs[NV_PFIFO_CACHE1_DMA_RSVD_SHADOW] = word;

            /* match all forms */
            if ((word & 0xe0000003) == 0x20000000) {
                /* old jump */
                d->pfifo.regs[NV_PFIFO_CACHE1_DMA_GET_JMP_SHADOW] =
                    dma_get_v;
                dma_get_v = word & 0x1fffffff;
                NV2A_DPRINTF("pb OLD_JMP 0x%x\n", dma_get_v);
            } else if ((word & 3) == 1) {
                /* jump */
                d->pfifo.regs[NV_PFIFO_CACHE1_DMA_GET_JMP_SHADOW] =
                    dma_get_v;
                dma_get_v = word & 0xfffffffc;
                NV2A_DPRINTF("pb JMP 0x%x\n", dma_get_v);
            } else if ((word & 3) == 2) {
                /* call */
                if (subroutine_state) {
                    SET_MASK(*dma_state, NV_PFIFO_CACHE1_DMA_STATE_ERROR,
                             NV_PFIFO_CACHE1_DMA_STATE_ERROR_CALL);
                    break;
                } else {
                    *dma_subroutine = dma_get_v;
                    SET_MASK(*dma_subroutine,
                             NV_PFIFO_CACHE1_DMA_SUBROUTINE_STATE, 1);
                    dma_get_v = word & 0xfffffffc;
                    NV2A_DPRINTF("pb CALL 0x%x\n", dma_get_v);
                }
            } else if (word == 0x00020000) {
                /* return */
                if (!subroutine_state) {
                    SET_MASK(*dma_state, NV_PFIFO_CACHE1_DMA_STATE_ERROR,
                             NV_PFIFO_CACHE1_DMA_STATE_ERROR_RETURN);
                    // break;
                } else {
                    dma_get_v = *dma_subroutine & 0xfffffffc;
                    SET_MASK(*dma_subroutine,
                             NV_PFIFO_CACHE1_DMA_SUBROUTINE_STATE, 0);
                    NV2A_DPRINTF("pb RET 0x%x\n", dma_get_v);
                }
            } else if ((word & 0xe0030003) == 0) {
                /* increasing methods */
                SET_MASK(*dma_state, NV_PFIFO_CACHE1_DMA_STATE_METHOD,
                         (word & 0x1fff) >> 2 );
                SET_MASK(*dma_state, NV_PFIFO_CACHE1_DMA_STATE_SUBCHANNEL,
                         (word >> 13) & 7);
                SET_MASK(*dma_state, NV_PFIFO_CACHE1_DMA_STATE_METHOD_COUNT,
                         (word >> 18) & 0x7ff);
                SET_MASK(*dma_state, NV_PFIFO_CACHE1_DMA_STATE_METHOD_TYPE,
                         NV_PFIFO_CACHE1_DMA_STATE_METHOD_TYPE_INC);
                *dma_dcount = 0;
            } else if ((word & 0xe0030003) == 0x40000000) {
                /* non-increasing methods */
                SET_MASK(*dma_state, NV_PFIFO_CACHE1_DMA_STATE_METHOD,
                         (word & 0x1fff) >> 2 );
                SET_MASK(*dma_state, NV_PFIFO_CACHE1_DMA_STATE_SUBCHANNEL,
                         (word >> 13) & 7);
                SET_MASK(*dma_state, NV_PFIFO_CACHE1_DMA_STATE_METHOD_COUNT,
                         (word >> 18) & 0x7ff);
                SET_MASK(*dma_state, NV_PFIFO_CACHE1_DMA_STATE_METHOD_TYPE,
                         NV_PFIFO_CACHE1_DMA_STATE_METHOD_TYPE_NON_INC);
                *dma_dcount = 0;
            } else {
                NV2A_DPRINTF("pb reserved cmd 0x%x - 0x%x\n",
                             dma_get_v, word);
                SET_MASK(*dma_state, NV_PFIFO_CACHE1_DMA_STATE_ERROR,
                         NV_PFIFO_CACHE1_DMA_STATE_ERROR_RESERVED_CMD);
                // break;
                assert(!"Reserved pb command - invalid GPU command. Check logs for more info");
            }
        }

        /* xboxrecomp: the read position as each word is taken, and a title waiting for the GPU
         * (host/nv2a_host.c) woken every 64 words and at the end -- the full fence and the check it
         * pairs with, after every word, were most of this thread's time */
        __atomic_store_n(dma_get, dma_get_v, __ATOMIC_RELEASE);
        if (++since_progress >= 64) {
            since_progress = 0;
            __atomic_thread_fence(__ATOMIC_SEQ_CST);
            nv2a_vk_progress();
        }

        if (GET_MASK(*dma_state, NV_PFIFO_CACHE1_DMA_STATE_ERROR)) {
            break;
        }
    }
    pfifo_graph_end(d, &graph_held);
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
    nv2a_vk_progress();

    // NV2A_DPRINTF("DMA pusher done: max 0x%" HWADDR_PRIx ", 0x%" HWADDR_PRIx " - 0x%" HWADDR_PRIx "\n",
    //      dma_len, control->dma_get, control->dma_put);

    uint32_t error = GET_MASK(*dma_state, NV_PFIFO_CACHE1_DMA_STATE_ERROR);
    if (error) {
        NV2A_DPRINTF("pb error: %d\n", error);
        assert(!"Error reported in PFIFO dma state - check logs for more info");

        SET_MASK(*dma_push, NV_PFIFO_CACHE1_DMA_PUSH_STATUS, 1); /* suspended */

        // d->pfifo.pending_interrupts |= NV_PFIFO_INTR_0_DMA_PUSHER;
        // nv2a_update_irq(d);
    }
}

void *pfifo_thread(void *arg)
{
    NV2AState *d = (NV2AState *)arg;
    extern void nv2a_dirty_set_gpu_thread(void);   /* xboxrecomp */

    nv2a_dirty_set_gpu_thread();
    pgraph_init_thread(d);

    rcu_register_thread();

    qemu_mutex_lock(&d->pfifo.lock);
    unsigned rounds = 0;
    while (true) {
        d->pfifo.fifo_kick = false;
        if ((++rounds & 15) == 0)
            recomp_load_sample(RECOMP_LOAD_GPU);   /* this thread's CPU time, for log.txt (host/threadload.h) */

        pgraph_process_pending(d);

        if (!d->pfifo.halt) {
            pfifo_run_pusher(d);
        }

        pgraph_process_pending_reports(d);

        if (!d->pfifo.fifo_kick) {
            qemu_cond_broadcast(&d->pfifo.fifo_idle_cond);

            // Both the pusher and puller are waiting for some action
            /* xboxrecomp: or for a present the renderer has scheduled
             * (pgraph/vk/renderer.c, early present). */
            extern volatile int64_t nv2a_present_due_us;
            extern void nv2a_pace_idle(int why, int64_t us);
            /* (NV2A_PACE_LOG: why this wait: 0 no commands, 1 no FIFO access, 2 a software method, 3 a flip,
             * 4 other) */
            int pace_why = nv2a_pace_on <= 0 ? -1
                         : d->pfifo.regs[NV_PFIFO_CACHE1_DMA_GET] == d->pfifo.regs[NV_PFIFO_CACHE1_DMA_PUT] ? 0
                         : !can_fifo_access(d) ? 1 : qatomic_read(&d->pgraph.waiting_for_nop) ? 2
                         : qatomic_read(&d->pgraph.waiting_for_flip) ? 3 : 4;
            int64_t pace_t0 = pace_why >= 0 ? g_get_monotonic_time() : 0;
            int64_t due = qatomic_read(&nv2a_present_due_us);
            if (pace_why == 4) {
                uint64_t g = __atomic_load_n(&s_gate, __ATOMIC_ACQUIRE);
                if ((g >> 32) && d->pfifo.regs[NV_PFIFO_CACHE1_DMA_GET] == (uint32_t)g)
                    pace_why = 7;   /* (held at the gate: the renderer has not drawn further) */
            }
            if (pace_why == 4 && due)
                pace_why = 5;   /* (waiting for a present the renderer scheduled) */
            else if (pace_why == 4 && (d->pfifo.halt ||
                     GET_MASK(d->pfifo.regs[NV_PFIFO_CACHE1_DMA_PUSH], NV_PFIFO_CACHE1_DMA_PUSH_STATUS)))
                pace_why = 6;   /* (halted, or the pusher suspended) */
            int64_t wait = due ? due - g_get_monotonic_time() : 0;
            if (due && wait <= 0) {
                /* due already: go round and present */
            } else if (due) {
                struct timespec ts;
                clock_gettime(CLOCK_REALTIME, &ts);
                ts.tv_nsec += (long)(wait % 1000000) * 1000;
                ts.tv_sec += wait / 1000000 + ts.tv_nsec / 1000000000;
                ts.tv_nsec %= 1000000000;
                pthread_cond_timedwait(&d->pfifo.fifo_cond.c,
                                       &d->pfifo.lock.m, &ts);
            } else {
                qemu_cond_wait(&d->pfifo.fifo_cond, &d->pfifo.lock);
            }
            if (pace_why >= 0)
                nv2a_pace_idle(pace_why, g_get_monotonic_time() - pace_t0);
        }

        if (d->exiting) {
            break;
        }
    }
    qemu_mutex_unlock(&d->pfifo.lock);

    rcu_unregister_thread();

    return NULL;
}

static uint32_t ramht_hash(NV2AState *d, uint32_t handle)
{
    unsigned int ramht_size =
        1 << (GET_MASK(d->pfifo.regs[NV_PFIFO_RAMHT], NV_PFIFO_RAMHT_SIZE)+12);

    /* XXX: Think this is different to what nouveau calculates... */
    unsigned int bits = ctz32(ramht_size)-1;

    uint32_t hash = 0;
    while (handle) {
        hash ^= (handle & ((1 << bits) - 1));
        handle >>= bits;
    }

    unsigned int channel_id = GET_MASK(d->pfifo.regs[NV_PFIFO_CACHE1_PUSH1],
                                       NV_PFIFO_CACHE1_PUSH1_CHID);
    hash ^= channel_id << (bits - 4);

    return hash;
}


static RAMHTEntry ramht_lookup(NV2AState *d, uint32_t handle)
{
    hwaddr ramht_size =
        1 << (GET_MASK(d->pfifo.regs[NV_PFIFO_RAMHT], NV_PFIFO_RAMHT_SIZE)+12);

    uint32_t hash = ramht_hash(d, handle);
    assert(hash * 8 < ramht_size);

    hwaddr ramht_address =
        GET_MASK(d->pfifo.regs[NV_PFIFO_RAMHT],
                 NV_PFIFO_RAMHT_BASE_ADDRESS) << 12;

    assert(ramht_address + hash * 8 < memory_region_size(&d->ramin));

    uint8_t *entry_ptr = d->ramin_ptr + ramht_address + hash * 8;

    uint32_t entry_handle = ldl_le_p((uint32_t*)entry_ptr);
    uint32_t entry_context = ldl_le_p((uint32_t*)(entry_ptr + 4));

    return (RAMHTEntry){
        .handle = entry_handle,
        .instance = (entry_context & NV_RAMHT_INSTANCE) << 4,
        .engine = (entry_context & NV_RAMHT_ENGINE) >> 16,
        .channel_id = (entry_context & NV_RAMHT_CHID) >> 24,
        .valid = entry_context & NV_RAMHT_STATUS,
    };
}
