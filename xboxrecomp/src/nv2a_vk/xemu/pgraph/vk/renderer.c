/*
 * Geforce NV2A PGRAPH Vulkan Renderer
 *
 * Copyright (c) 2024-2025 Matt Borgerson
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

#include "hw/xbox/nv2a/nv2a_int.h"
#include "renderer.h"

#if HAVE_EXTERNAL_MEMORY
#include "gloffscreen.h"
#endif
#include "nv2a_host_vk.h"

#if HAVE_EXTERNAL_MEMORY
static GloContext *g_gl_context;
#endif

static void early_context_init(void)
{
#if HAVE_EXTERNAL_MEMORY
    g_gl_context = glo_context_create();
#endif
}

static void pgraph_vk_init(NV2AState *d, Error **errp)
{
    PGRAPHState *pg = &d->pgraph;

    pg->vk_renderer_state = (PGRAPHVkState *)g_malloc0(sizeof(PGRAPHVkState));

#if HAVE_EXTERNAL_MEMORY
    glo_set_current(g_gl_context);
#endif

    pgraph_vk_debug_init();

    pgraph_vk_init_instance(pg, errp);
    if (*errp) {
        return;
    }

    pgraph_vk_init_command_buffers(pg);
    pgraph_vk_init_buffers(d);
    pgraph_vk_init_surfaces(pg);
    pgraph_vk_init_shaders(pg);
    pgraph_vk_init_pipelines(pg);
    pgraph_vk_init_textures(pg);
    pgraph_vk_init_reports(pg);
    pgraph_vk_init_compute(pg);
    pgraph_vk_init_display(pg);

    pgraph_vk_update_vertex_ram_buffer(&d->pgraph, 0, d->vram_ptr,
                                   memory_region_size(d->vram));

    /* The winding probe draws with a geometry shader; drivers without them (Mali, #180) crash making its
     * pipeline. Only xemu's own drawing uses the result, and that needs geometry shaders anyway. */
    if (pg->vk_renderer_state->enabled_physical_device_features.geometryShader)
        pgraph_vk_determine_gpu_properties(d);
}

static void pgraph_vk_finalize(NV2AState *d)
{
    PGRAPHState *pg = &d->pgraph;

    pgraph_vk_finalize_display(pg);
    pgraph_vk_finalize_compute(pg);
    pgraph_vk_finalize_reports(pg);
    pgraph_vk_finalize_textures(pg);
    pgraph_vk_finalize_pipelines(pg);
    pgraph_vk_finalize_shaders(pg);
    pgraph_vk_finalize_surfaces(pg);
    pgraph_vk_finalize_buffers(d);
    pgraph_vk_finalize_command_buffers(pg);
    pgraph_vk_finalize_instance(pg);

    g_free(pg->vk_renderer_state);
    pg->vk_renderer_state = NULL;
}

static bool s_drop_textures;

/* xboxrecomp: the overlay's quality settings, on the renderer's thread. The
 * surfaces are written back and dropped (made again at the new scale), as
 * pgraph_vk_set_surface_scale_factor does from another thread; the textures
 * too when the filter mode changes. */
void pgraph_vk_apply_quality(NV2AState *d, int scale, int aniso, int shadow)
{
    PGRAPHVkState *r = d->pgraph.vk_renderer_state;
    extern int pgraph_vk_tex_smooth, pgraph_vk_shadow_scale;

    scale = scale < 1 ? 1 : scale > 10 ? 10 : scale;
    shadow = shadow < 1 ? 1 : shadow > 10 ? 10 : shadow;
    if ((int)g_config.display.quality.surface_scale == scale &&
        pgraph_vk_tex_smooth == aniso && pgraph_vk_shadow_scale == shadow)
        return;
    if (pgraph_vk_tex_smooth != aniso)
        s_drop_textures = true;
    g_config.display.quality.surface_scale = scale;
    pgraph_vk_tex_smooth = aniso;
    pgraph_vk_shadow_scale = shadow;
    qemu_event_reset(&r->dirty_surfaces_download_complete);
    qatomic_set(&r->download_dirty_surfaces_pending, true);
    qemu_event_reset(&d->pgraph.flush_complete);
    qatomic_set(&d->pgraph.flush_pending, true);
}

static void pgraph_vk_flush(NV2AState *d)
{
    PGRAPHState *pg = &d->pgraph;

    pgraph_vk_finish(pg, VK_FINISH_REASON_FLUSH);
    if (s_drop_textures) {
        s_drop_textures = false;
        pgraph_vk_drop_textures(pg);
    }
    pgraph_vk_surface_flush(d);
    pgraph_vk_mark_textures_possibly_dirty(d, 0, memory_region_size(d->vram));
    pgraph_vk_update_vertex_ram_buffer(&d->pgraph, 0, d->vram_ptr,
                                       memory_region_size(d->vram));
    for (int i = 0; i < 4; i++) {
        pg->texture_dirty[i] = true;
    }

    /* FIXME: Flush more? */

    qatomic_set(&d->pgraph.flush_pending, false);
    qemu_event_set(&d->pgraph.flush_complete);
}

/* xboxrecomp: early present.
 *
 * The GPU reaches a frame's FLIP_STALL once the frame is drawn, a few ms
 * after the vblank that let it start; the title then flips to it at the
 * next vblank, and only then did it reach the screen -- most of a refresh
 * spent showing the previous frame. Here the finished frame is presented
 * at FLIP_STALL, and the vblank whose flip lands on it presents nothing
 * new. The guest sees no difference: its vblanks, flips and timing are as
 * before; only the picture is on screen sooner. Done only for a render
 * target the title has scanned out before (its front/back buffers), so a
 * wrong guess never shows something that was not going to be shown.
 * Off unless NV2A_EARLY_PRESENT=1: on a busy frame (the first chapter's
 * cutscene, some 600 draws) the present holds the PFIFO thread until the
 * next vblank while the title's next frame waits, and the frame rate fell
 * to 40-50 at any render scale; without it, a steady 60. */
#define MAX_SCANOUTS 4
static hwaddr s_scanouts[MAX_SCANOUTS];
static int s_num_scanouts;
static bool s_early_pending;
static hwaddr s_early_start;
static int s_early_waits;

static void note_scanout(hwaddr start)
{
    for (int i = 0; i < s_num_scanouts; i++)
        if (s_scanouts[i] == start)
            return;
    if (s_num_scanouts < MAX_SCANOUTS)
        s_scanouts[s_num_scanouts++] = start;
    else {
        memmove(s_scanouts, s_scanouts + 1,
                (MAX_SCANOUTS - 1) * sizeof(s_scanouts[0]));
        s_scanouts[MAX_SCANOUTS - 1] = start;
    }
}

static bool pace_enabled(void)
{
    static int on = -1;
    if (on < 0) {
        const char *e = getenv("NV2A_PACE_PRESENT");
        on = !(e && *e == '0');
    }
    return on;
}

static bool early_present_enabled(void)
{
    static int on = -1;
    if (on < 0) {
        const char *e = getenv("NV2A_EARLY_PRESENT");
        on = e && *e == '1';
    }
    return on;
}

/* Frames finished (FLIP_STALL) and frames flipped to (scanout writes),
 * to tell whether an older frame is still waiting for its flip. */
static unsigned s_stalls;
unsigned long nv2a_early_presents;   /* for NV2A_FPS_LOG */

/* Paced presents. A frame reaches FLIP_STALL a few ms after the vblank
 * that let it start, a little sooner or later each time, and a VRR display
 * shows each one the moment it is presented: presented as soon as they were
 * done, frames came out unevenly spaced. So the present is scheduled for a
 * steady point after the vblank -- just past the second-latest finish of
 * the last second, well short of the next vblank -- and the PFIFO thread
 * waits for it where it waits for work (pfifo.c), holding no lock the
 * guest might need. */
#define PACE_WINDOW 60
#define PACE_MAX_US 10000
volatile int64_t nv2a_present_due_us;   /* 0: none scheduled */

static int64_t pace_target(int64_t vblank, int64_t now)
{
    static int64_t offsets[PACE_WINDOW];
    static int next;
    int64_t off = now - vblank, first = 0, second = 0;

    if (!vblank || off < 0 || off > 16700)
        return now;
    offsets[next] = off;
    next = (next + 1) % PACE_WINDOW;
    for (int i = 0; i < PACE_WINDOW; i++) {
        if (offsets[i] > first) {
            second = first;
            first = offsets[i];
        } else if (offsets[i] > second) {
            second = offsets[i];
        }
    }
    return vblank + MIN(second + 500, PACE_MAX_US);
}

static void present_scheduled(PGRAPHState *pg)
{
    if (qatomic_read(&nv2a_present_due_us)) {
        qatomic_set(&nv2a_present_due_us, 0);
        nv2a_host_vk_present(pg);
    }
}

static bool scheduled_present_due(void)
{
    int64_t due = qatomic_read(&nv2a_present_due_us);
    return due && g_get_monotonic_time() >= due;
}

static void early_present(NV2AState *d)
{
    extern volatile unsigned nv2a_pcrtc_start_writes;
    PGRAPHState *pg = &d->pgraph;
    PGRAPHVkState *r = pg->vk_renderer_state;
    SurfaceBinding *s = r->color_binding;
    unsigned flips = __atomic_load_n(&nv2a_pcrtc_start_writes, __ATOMIC_ACQUIRE);

    /* Frames finished before this one but not flipped yet. Showing this
     * one now would skip them on screen; let them flip as usual. Counted
     * as the gap between frames finished and flips, less the smallest gap
     * of the last two seconds: a stall or flip that is not part of a
     * frame (boot, a mode change) leaves a constant offset, not a queue. */
    #define GAP_WINDOW 120
    static int gaps[GAP_WINDOW];
    static int gap_next, gap_count;
    int gap = (int)(s_stalls - flips);
    s_stalls++;
    gaps[gap_next] = gap;
    gap_next = (gap_next + 1) % GAP_WINDOW;
    if (gap_count < GAP_WINDOW)
        gap_count++;
    int floor_gap = gap;
    for (int i = 0; i < gap_count; i++)
        floor_gap = MIN(floor_gap, gaps[i]);
    int older_pending = gap - floor_gap;

    if (!early_present_enabled() || s_early_pending || older_pending ||
        !s || !s->color)
        return;
    for (int i = 0; i < s_num_scanouts; i++) {
        hwaddr start = s_scanouts[i];
        if (start == d->pcrtc.start)
            continue;   /* on screen already: not the next frame */
        if (start < s->vram_addr || start >= s->vram_addr + s->size)
            continue;
        if (pgraph_vk_render_display_at(pg, start)) {
            extern volatile int64_t nv2a_vblank_us;
            int64_t now = g_get_monotonic_time();
            int64_t due = pace_target(qatomic_read(&nv2a_vblank_us), now);
            if (due <= now || !pace_enabled()) {
                nv2a_host_vk_present(pg);
            } else {
                qatomic_set(&nv2a_present_due_us, due);
            }
            nv2a_early_presents++;
            s_early_pending = true;
            s_early_start = start;
            s_early_waits = 0;
        }
        return;
    }
}

static void pgraph_vk_sync(NV2AState *d)
{
    PGRAPHState *pg = &d->pgraph;
    bool show = true;

    present_scheduled(pg);   /* not shown yet: now, before anything newer */
    note_scanout(d->pcrtc.start);
    if (s_early_pending) {
        if (d->pcrtc.start == s_early_start) {
            show = false;            /* the flip to what is already shown */
            s_early_pending = false;
        } else if (++s_early_waits <= 3) {
            show = false;            /* an older frame still to flip: do
                                      * not step back to it */
        } else {
            s_early_pending = false; /* the guess was wrong after all */
        }
    }
    if (show) {
        pgraph_vk_render_display(pg);
        /* xboxrecomp: show it. Queue work stays on this (the PFIFO) thread. */
        nv2a_host_vk_present(pg);
    }

    qatomic_set(&d->pgraph.sync_pending, false);
    qemu_event_set(&d->pgraph.sync_complete);
}

static void pgraph_vk_process_pending(NV2AState *d)
{
    PGRAPHVkState *r = d->pgraph.vk_renderer_state;

    if (qatomic_read(&r->downloads_pending) ||
        qatomic_read(&r->download_dirty_surfaces_pending) ||
        qatomic_read(&d->pgraph.sync_pending) ||
        qatomic_read(&d->pgraph.flush_pending) ||
        scheduled_present_due()
    ) {
        qemu_mutex_unlock(&d->pfifo.lock);
        qemu_mutex_lock(&d->pgraph.lock);
        if (scheduled_present_due()) {
            present_scheduled(&d->pgraph);
        }
        if (qatomic_read(&r->downloads_pending)) {
            pgraph_vk_process_pending_downloads(d);
        }
        if (qatomic_read(&r->download_dirty_surfaces_pending)) {
            pgraph_vk_download_dirty_surfaces(d);
        }
        if (qatomic_read(&d->pgraph.sync_pending)) {
            pgraph_vk_sync(d);
        }
        if (qatomic_read(&d->pgraph.flush_pending)) {
            pgraph_vk_flush(d);
        }
        qemu_mutex_unlock(&d->pgraph.lock);
        qemu_mutex_lock(&d->pfifo.lock);
    }
}

static void pgraph_vk_flip_stall(NV2AState *d)
{
    pgraph_vk_finish(&d->pgraph, VK_FINISH_REASON_FLIP_STALL);
    early_present(d);
    pgraph_vk_debug_frame_terminator();
}

static void pgraph_vk_pre_savevm_trigger(NV2AState *d)
{
    qatomic_set(&d->pgraph.vk_renderer_state->download_dirty_surfaces_pending, true);
    qemu_event_reset(&d->pgraph.vk_renderer_state->dirty_surfaces_download_complete);
}

static void pgraph_vk_pre_savevm_wait(NV2AState *d)
{
    qemu_event_wait(&d->pgraph.vk_renderer_state->dirty_surfaces_download_complete);
}

static void pgraph_vk_pre_shutdown_trigger(NV2AState *d)
{
    // qatomic_set(&d->pgraph.vk_renderer_state->shader_cache_writeback_pending, true);
    // qemu_event_reset(&d->pgraph.vk_renderer_state->shader_cache_writeback_complete);
}

static void pgraph_vk_pre_shutdown_wait(NV2AState *d)
{
    // qemu_event_wait(&d->pgraph.vk_renderer_state->shader_cache_writeback_complete);   
}

static int pgraph_vk_get_framebuffer_surface(NV2AState *d)
{
    PGRAPHState *pg = &d->pgraph;
    PGRAPHVkState *r = pg->vk_renderer_state;

    qemu_mutex_lock(&d->pfifo.lock);

    VGADisplayParams vga_display_params;
    d->vga.get_params(&d->vga, &vga_display_params);

    SurfaceBinding *surface = pgraph_vk_surface_get_within(
        d, d->pcrtc.start + vga_display_params.line_offset);
    if (surface == NULL || !surface->color) {
        qemu_mutex_unlock(&d->pfifo.lock);
        return 0;
    }

    assert(surface->color);

    surface->frame_time = pg->frame_time;

#if HAVE_EXTERNAL_MEMORY
    qemu_event_reset(&d->pgraph.sync_complete);
    qatomic_set(&pg->sync_pending, true);
    pfifo_kick(d);
    qemu_mutex_unlock(&d->pfifo.lock);
    qemu_event_wait(&d->pgraph.sync_complete);
    return r->display.gl_texture_id;
#else
    qemu_mutex_unlock(&d->pfifo.lock);
    pgraph_vk_wait_for_surface_download(surface);
    return 0;
#endif
}

static PGRAPHRenderer pgraph_vk_renderer = {
    .type = CONFIG_DISPLAY_RENDERER_VULKAN,
    .name = "Vulkan",
    .ops = {
        .init = pgraph_vk_init,
        .early_context_init = early_context_init,
        .finalize = pgraph_vk_finalize,
        .clear_report_value = pgraph_vk_clear_report_value,
        .clear_surface = pgraph_vk_clear_surface,
        .draw_begin = pgraph_vk_draw_begin,
        .draw_end = pgraph_vk_draw_end,
        .flip_stall = pgraph_vk_flip_stall,
        .flush_draw = pgraph_vk_flush_draw,
        .get_report = pgraph_vk_get_report,
        .image_blit = pgraph_vk_image_blit,
        .pre_savevm_trigger = pgraph_vk_pre_savevm_trigger,
        .pre_savevm_wait = pgraph_vk_pre_savevm_wait,
        .pre_shutdown_trigger = pgraph_vk_pre_shutdown_trigger,
        .pre_shutdown_wait = pgraph_vk_pre_shutdown_wait,
        .process_pending = pgraph_vk_process_pending,
        .process_pending_reports = pgraph_vk_process_pending_reports,
        .surface_update = pgraph_vk_surface_update,
        .set_surface_scale_factor = pgraph_vk_set_surface_scale_factor,
        .get_surface_scale_factor = pgraph_vk_get_surface_scale_factor,
        .get_framebuffer_surface = pgraph_vk_get_framebuffer_surface,
        .get_gpu_properties = pgraph_vk_get_gpu_properties,
    }
};

static void __attribute__((constructor)) register_renderer(void)
{
    pgraph_renderer_register(&pgraph_vk_renderer);
}

void pgraph_vk_check_memory_budget(PGRAPHState *pg)
{
#if 0 // FIXME
    PGRAPHVkState *r = pg->vk_renderer_state;

    VkPhysicalDeviceMemoryProperties const *props;
    vmaGetMemoryProperties(r->allocator, &props);

    g_autofree VmaBudget *budgets = g_malloc_n(props->memoryHeapCount, sizeof(VmaBudget));
    vmaGetHeapBudgets(r->allocator, budgets);

    const float budget_threshold = 0.8;
    bool near_budget = false;

    for (int i = 0; i < props->memoryHeapCount; i++) {
        VmaBudget *b = &budgets[i];
        float use_to_budget_ratio =
            (double)b->statistics.allocationBytes / (double)b->budget;
        NV2A_VK_DPRINTF("Heap %d: used %lu/%lu MiB (%.2f%%)", i,
                        b->statistics.allocationBytes / (1024 * 1024),
                        b->budget / (1024 * 1024), use_to_budget_ratio * 100);
        near_budget |= use_to_budget_ratio > budget_threshold;
    }

    // If any heaps are near budget, free up some resources
    if (near_budget) {
        pgraph_vk_trim_texture_cache(pg);
    }
#endif

#if 0
    char *s;
    vmaBuildStatsString(r->allocator, &s, VK_TRUE);
    puts(s);
    vmaFreeStatsString(r->allocator, s);
#endif
}
