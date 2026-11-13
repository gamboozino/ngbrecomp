/*
 * menu_mouse.c -- whether one of the game's menus is up.
 *
 * The menu engine's runner (sub_0017BB90: Options and the like), and the
 * text the menus write theirs with (sub_001E86A0: the main menu, the pause
 * menu, the game's message boxes; in play only a prompt's first frame), each
 * mark the frame when they run (the text when drawn frame after frame). Both are the lifted functions (listed in
 * src/manual_functions.json) renamed, behind a wrapper. The host knows from
 * it that the game is in a menu: there mouse clicks and the wheel are the
 * pad's buttons (src/ingame_menu.c).
 */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"
#include <time.h>
#include <math.h>
#include <string.h>
#include "xbox_memory_layout.h"
#include <stdio.h>
#include <stdlib.h>

/* When a menu last ran, in ms of CLOCK_MONOTONIC; and the game's frame it ran in (ngb_menu_frame) */
volatile uint64_t ngb_menu_ran_ms;
static volatile uint32_t s_frames, s_menu_frame, s_menu_seen;

static void menu_runner(void);
static void menu_text(void);
static void text_draw(void);

/* A menu ran this frame (also from the pause menu's handler, which is not
 * always reached through the runner: src/pause_tab.c). */
void ngb_aspect_menu_running(void);   /* src/pc_menu.c: the game's widescreen mode for it */
void ngb_menu_mark(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ngb_menu_ran_ms = (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
    s_menu_frame = s_frames;
    s_menu_seen = 1;
}

/* Each frame the game draws (Swap, src/cheats.c's ngb_frame_tick) */
void ngb_menu_frame(void)
{
    s_frames++;
}

static uint64_t now_ms(void);
static volatile uint64_t s_engine_ms;

void sub_0017BB90(void)
{
    /* a menu of the engine runs (not text alone: sub_001E86A0 also draws play's prompts -- "Interact",
     * the karma -- which marked a menu for a frame and drew the HUD in the menus' 16:9 mode) */
    if (getenv("NGB_MENU_MODE_LOG"))
        fprintf(stderr, "[MENUMODE] engine\n");
    ngb_aspect_menu_running();
    ngb_menu_mark();
    s_engine_ms = now_ms();
    menu_runner();
}

/* The open page's rows: the first and how many (0 when no menu engine). */
int ngb_engine_rows(int *first)
{
    if (now_ms() - s_engine_ms > 150)
        return 0;
    int32_t depth = (int32_t)MEM32(0xDA41C0);
    if (depth < 0 || depth > 3)
        return 0;
    uint32_t st = 0xDA41D0 + (uint32_t)depth * 0x30, page = MEM32(st + 0x28);
    if (page < 0x10000 || page >= 0x4000000)
        return 0;
    int lo = MEM8(page + 1), hi = MEM8(page + 2);
    if (hi == 0xFF)
        hi = MEM8(0xDA41CC);
    *first = lo;
    return hi >= lo ? hi - lo + 1 : 0;
}

int ngb_engine_row(void)
{
    if (now_ms() - s_engine_ms > 150)
        return -1;
    int32_t depth = (int32_t)MEM32(0xDA41C0);
    if (depth < 0 || depth > 3)
        return -1;
    uint32_t st = 0xDA41D0 + (uint32_t)depth * 0x30;
    return MEM8(st + 1);
}

void ngb_menu_font_init(void);   /* src/menu_font.c */

/* Text drawn on and on: the menus redraw theirs every frame. Play's prompts ("Interact", the karma) draw
 * once when they appear; marked as a menu, the keys and the controller left their bindings for 150 ms --
 * F, bound to move, was Ninpo's Y then, a Special Attack walking up to a save statue (#71). */
static uint64_t s_text_ms, s_text_since;

void sub_001E86A0(void)
{
    uint64_t now = now_ms();
    if (now - s_text_ms > 100)
        s_text_since = now;
    s_text_ms = now;
    if (now - s_text_since >= 50)
        ngb_menu_mark();
    ngb_menu_font_init();
    menu_text();
}


/* ---------- what the menus write, for the mouse ---------- */

/* Each piece of text drawn: where (its box in the 720x480, measured as
 * the game measures it), how bright (the chosen item is drawn brighter),
 * and which code drew it (the items of one list share it). Kept for the
 * last few frames; src/ingame_menu.c finds the item under the mouse. */
#include "menu_mouse.h"

static NgbTextItem s_items[MOUSE_ITEMS];
static volatile unsigned s_item_n;
static uint32_t s_scratch;        /* guest: a struct and a rect to measure with */

static uint64_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

static int text_log(void)
{
    static int on = -1;
    if (on < 0)
        on = getenv("NGB_TEXT_LOG") != NULL;
    return on;
}

static void record_text(uint32_t st, uint32_t caller)
{
    uint32_t str = MEM32(st + 0x10);
    if (!str || MEM32(st + 0x38) >= 3) {
        if (text_log())
            fprintf(stderr, "[TEXTLOG] %08X skipped: str %08X f%u\n", caller, str, MEM32(st + 0x38));
        return;
    }
    if (!s_scratch && !(s_scratch = xbox_HeapAlloc(0x80, 16)))
        return;
    /* Measure on a copy: the measurer writes to its struct. */
    memcpy((void *)XBOX_PTR(s_scratch), (void *)XBOX_PTR(st), 0x60);
    uint32_t sv_eax = eax, sv_ecx = ecx, sv_edx = edx, sv_ebx = ebx, sv_esi = esi,
             sv_edi = edi, sv_esp = esp, sv_ebp = g_ebp, sv_seh = g_seh_ebp;
    PUSH32(esp, s_scratch);
    PUSH32(esp, s_scratch + 0x60);
    PUSH32(esp, 0);
    menu_text();
    float rx = MEMF(s_scratch + 0x60), ry = MEMF(s_scratch + 0x64);
    float rw = MEMF(s_scratch + 0x68), rh = MEMF(s_scratch + 0x6C);
    eax = sv_eax; ecx = sv_ecx; edx = sv_edx; ebx = sv_ebx; esi = sv_esi; edi = sv_edi;
    esp = sv_esp; g_ebp = sv_ebp; g_seh_ebp = sv_seh;
    if (!(rw > 0 && rh > 0) || rw > 720 || rh > 200) {
        if (text_log())
        {
            fprintf(stderr, "[TEXTLOG] %08X not measured: %.0f %.0f %.0f %.0f f%u s3c %04X %04X %04X %04X '",
                    caller, rx, ry, rw, rh, MEM32(st + 0x38), MEM16(st + 0x3C), MEM16(st + 0x3E),
                    MEM16(st + 0x40), MEM16(st + 0x42));
            for (int k = 0; k < 120 && MEM8(str + k); k++) {
                uint8_t c = MEM8(str + k);
                if (c >= 32 && c < 127) fputc(c, stderr); else fprintf(stderr, "<%02X>", c);
            }
            fprintf(stderr, "'\n");
        }
        return;
    }
    uint8_t al = MEM8(st + 0x4A);
    float x0 = rx - ((al & 0xF) == 1 ? rw / 2 : (al & 0xF) == 2 ? rw : 0);
    float y0 = ry - ((al >> 4) == 1 ? rh / 2 : (al >> 4) == 2 ? rh : 0);
    unsigned i = __atomic_fetch_add(&s_item_n, 1, __ATOMIC_ACQ_REL);
    NgbTextItem *it = &s_items[i % MOUSE_ITEMS];
    it->ms = now_ms();
    it->x0 = x0; it->y0 = y0; it->x1 = x0 + rw; it->y1 = y0 + rh;
    it->bright = MEMF(st + 0x14);                    /* the chosen one: 1, the rest dim */
    it->caller = caller;
    it->str = str;
    it->kind = 0;
    it->seq = i;
    if (text_log() && i % 600 < 24) {
        fprintf(stderr, "[TEXTLOG] %08X %.0f,%.0f-%.0f,%.0f b %.2f col %.2f %.2f %.2f %.2f '",
                caller, x0, y0, x0 + rw, y0 + rh, it->bright,
                MEMF(st + 0x14), MEMF(st + 0x18), MEMF(st + 0x1C), MEMF(st + 0x20));
        for (int k = 0; k < 30 && MEM8(str + k); k++) {
            uint8_t c = MEM8(str + k);
            fputc(c >= 32 && c < 127 ? c : '.', stderr);
        }
        fprintf(stderr, "'\n");
    }
}

/* Something else to point at: the pause menu's tabs (src/pause_tab.c). */
void ngb_menu_add_item(float x0, float y0, float x1, float y1, float bright,
                       uint32_t group, uint32_t id, int kind)
{
    unsigned i = __atomic_fetch_add(&s_item_n, 1, __ATOMIC_ACQ_REL);
    NgbTextItem *it = &s_items[i % MOUSE_ITEMS];
    it->ms = now_ms();
    it->x0 = x0; it->y0 = y0; it->x1 = x1; it->y1 = y1;
    it->bright = bright;
    it->caller = group;
    it->str = id;
    it->kind = kind;
    it->seq = i;
}

/* The letters of the menus, as the sprites they are drawn with (from the
 * sprite queue, src/pause_tab.c): joined into runs -- the letters of a line,
 * one after another at the same height -- each run an item, those at the
 * same depth and size one list. Whatever code wrote them. Game thread. */
static struct {
    float x0, y0, x1, y1, bright;
    uint32_t z;
    int n;
} s_run;

static void run_flush(void)
{
    if (s_run.n >= 2) {
        uint32_t h = (uint32_t)lroundf(s_run.y1 - s_run.y0);
        ngb_menu_add_item(s_run.x0, s_run.y0, s_run.x1, s_run.y1, s_run.bright,
                          0x6C000000u | (s_run.z & 0xFFFF) << 8 | (h & 0xFF),
                          (uint32_t)lroundf(s_run.y0) << 16 | (uint32_t)lroundf(s_run.x0 / 8) , 0);
    }
    s_run.n = 0;
}

void ngb_menu_glyph(float x0, float y0, float x1, float y1, float bright, uint32_t z)
{
    float h = y1 - y0;
    if (!(h > 0) || h > 120 || x1 <= x0)
        return;
    if (s_run.n && z == s_run.z && fabsf(y0 - s_run.y0) < 2 && fabsf(h - (s_run.y1 - s_run.y0)) < 2 &&
        x0 >= s_run.x0 && x0 <= s_run.x1 + h * 0.9f) {
        if (x1 > s_run.x1) s_run.x1 = x1;
        if (bright > s_run.bright) s_run.bright = bright;
        s_run.n++;
        return;
    }
    run_flush();
    s_run.x0 = x0; s_run.y0 = y0; s_run.x1 = x1; s_run.y1 = y1;
    s_run.bright = bright;
    s_run.z = z;
    s_run.n = 1;
}

/* The items drawn in the newest frame (those within 20 ms of the newest);
 * returns how many were copied. Any thread. */
int ngb_menu_items(NgbTextItem *out, int max)
{
    unsigned n = __atomic_load_n(&s_item_n, __ATOMIC_ACQUIRE);
    if (!n)
        return 0;
    uint64_t newest = s_items[(n - 1) % MOUSE_ITEMS].ms;
    if (now_ms() - newest > 150)
        return 0;
    int k = 0;
    for (unsigned j = 0; j < MOUSE_ITEMS && j < n && k < max; j++) {
        NgbTextItem it = s_items[(n - 1 - j) % MOUSE_ITEMS];
        if (newest - it.ms > 20)                     /* a frame's worth */
            break;
        out[k++] = it;
    }
    return k;
}

void sub_001E6D30(void)
{
    record_text(MEM32(esp + 4), MEM32(esp));
    text_draw();
}

/* A game menu up: one ran in the last 150 ms. */
int ngb_menu_up(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t now = (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
    return now - ngb_menu_ran_ms < 150;
}

/* For the controls (the bindings, the keyboard, Esc): one ran in this frame or the two before, by the game's
 * frames -- not 150 ms, after a message box (an item from a chest) the pad went to the game as it is that long
 * and a rebound button did its default (L1 bound to Block opened the map, #218); and not by time, a slow frame
 * inside a menu (a shader built, a save written) does not give the menu play's controls */
int ngb_menu_up_controls(void)
{
    uint32_t at = s_menu_frame;   /* (first: Esc asks from the window's thread, as Swap runs) */
    return s_menu_seen && (int32_t)(s_frames - at) <= 2;
}

/**
 * sub_0017BB90
 * Original: 0x0017BB90 - 0x0017BD07 (375 bytes, 120 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
static void menu_runner(void)
{
    RECOMP_ENTRY(0x0017BB90u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_0017BB90: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    edi = MEM32(esp + 0x1C);
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0xA) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0xA (32-bit) */
    MEM32(0xDA3E40) = edi;
    MEM32(0xDA4810) = 0;
    if (CMP_EQ(_fa, _fb)) goto loc_0017BBB5; /* je: equal / zero */

loc_0017BBB0: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(9) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 9 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0017BBBF; /* jne: not equal / not zero */

loc_0017BBB5: ;
    MEM32(0xDA4810) = 1;

loc_0017BBBF: ;
    eax = edi;
    PUSH32(esp, 0x0017BBC6u); RECOMP_ABI_CALL(0x0017BD10u, sub_0017BD10); /* call 0x0017BD10 */

loc_0017BBC6: ;
    ebp = MEM32(edi * 4 + 0x42A280);
    MEM32(esp + 0x10) = 1;
    goto loc_0017BBE0;

    /* nop */
    edi = edi;

loc_0017BBE0: ;
    s_engine_ms = now_ms();                          /* a frame of the menu */
    ngb_menu_mark();
    eax = MEM32(0xD21E4C);
    eax = eax << 5;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(0xD21C00) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0xD21C00;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(eax) = MEM32(eax) & 0xFFFFFFEFu;
    _fa = (uint32_t)(MEM32(eax)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(eax + 4) = 1;
    eax = MEM32(0x9570F0);
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, eax);
    PUSH32(esp, 0x0017BC02u); RECOMP_ICALL_SAFE(0x0032A293u, _icall_esp); /* manual call 0x0032A293 */
    }

loc_0017BC02: ;
    eax = MEM32(0xDA41C0);
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_L(_fas, _fbs)) goto loc_0017BCAA; /* jl: less (signed <) */

loc_0017BC11: ;
    esi = 0xDA41D0;
    goto loc_0017BC20;

    /* nop */
    /* nop */

loc_0017BC20: ;
    MEM32(0xDA41C4) = ebx;
    SET_LO8(eax, MEM8(ebp));
    _fa = (uint32_t)(MEM8(esi)) & 0xFFu; _fb = (uint32_t)(LO8(eax)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(esi), LO8(eax) (8-bit) */
    edi = ebp;
    if (CMP_EQ(_fa, _fb)) goto loc_0017BC40; /* je: equal / zero */

loc_0017BC2F: ;
    /* nop */

loc_0017BC30: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0xFF) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0xFF (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0017BC9E; /* je: equal / zero */

loc_0017BC34: ;
    SET_LO8(eax, MEM8(edi + 0x18));
    SET_LO8(ecx, MEM8(esi));
    _fb = (uint32_t)(0x18) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edi = edi + 0x18;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(LO8(eax)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), LO8(eax) (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0017BC30; /* jne: not equal / not zero */

loc_0017BC40: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0017BC9E; /* je: equal / zero */

loc_0017BC44: ;
    MEM32(esi + 0x28) = edi;
    PUSH32(esp, 0x0017BC4Cu); RECOMP_ABI_CALL(0x0017D7D0u, sub_0017D7D0); /* call 0x0017D7D0 */

loc_0017BC4C: ;
    edi = MEM32(edi + 0xC);
    SET_LO8(eax, MEM8(edi));
    _fa = (uint32_t)(MEM8(esi + 1)) & 0xFFu; _fb = (uint32_t)(LO8(eax)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(esi + 1), LO8(eax) (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0017BC6B; /* je: equal / zero */

loc_0017BC56: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0xFE) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0xFE (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0017BC6B; /* je: equal / zero */

loc_0017BC5A: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0xFF) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0xFF (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0017BC9E; /* je: equal / zero */

loc_0017BC5E: ;
    SET_LO8(eax, MEM8(edi + 0x1C));
    SET_LO8(ecx, MEM8(esi + 1));
    _fb = (uint32_t)(0x1C) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edi = edi + 0x1C;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(LO8(eax)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), LO8(eax) (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0017BC56; /* jne: not equal / not zero */

loc_0017BC6B: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0017BC9E; /* je: equal / zero */

loc_0017BC6F: ;
    eax = esi;
    MEM32(esi + 0x2C) = edi;
    PUSH32(esp, 0x0017BC79u); RECOMP_ABI_CALL(0x0017D950u, sub_0017D950); /* call 0x0017D950 */

loc_0017BC79: ;
    edi = MEM32(edi + 0x14);
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0017BC8F; /* je: equal / zero */

loc_0017BC80: ;
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, esi);
    { uint32_t _icall_target = edi; PUSH32(esp, 0x0017BC83u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x0017BC81u); } /* indirect call */
    }

loc_0017BC83: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFFFFFFu) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0xFFFFFFFFu (32-bit) */
    MEM32(esp + 0x14) = eax;
    if (CMP_NE(_fa, _fb)) goto loc_0017BC9E; /* jne: not equal / not zero */

loc_0017BC8F: ;
    eax = MEM32(0xDA41C0);
    ebx++;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fb = (uint32_t)(0x30) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + 0x30;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ebx (32-bit) */
    if (CMP_GE(_fas, _fbs)) goto loc_0017BC20; /* jge: greater or equal (signed >=) */

loc_0017BC9C: ;
    goto loc_0017BCA6;

loc_0017BC9E: ;
    MEM32(esp + 0x10) = 0;

loc_0017BCA6: ;
    edi = MEM32(esp + 0x1C);

loc_0017BCAA: ;
    eax = MEM32(0xDA41C8);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 1 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0017BCB9; /* je: equal / zero */

loc_0017BCB4: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(3) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 3 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0017BCE8; /* jne: not equal / not zero */

loc_0017BCB9: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 1 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0017BCCD; /* je: equal / zero */

loc_0017BCBE: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(4) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 4 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0017BCCD; /* je: equal / zero */

loc_0017BCC3: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(9) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 9 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0017BCCD; /* je: equal / zero */

loc_0017BCC8: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0xA) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0xA (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0017BCDE; /* jne: not equal / not zero */

loc_0017BCCD: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(3) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 3 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0017BCDE; /* jne: not equal / not zero */

loc_0017BCD2: ;
    PUSH32(esp, 0x40);
    PUSH32(esp, 4);
    PUSH32(esp, 0x0017BCDBu); RECOMP_ABI_CALL(0x001E3400u, sub_001E3400); /* call 0x001E3400 */

loc_0017BCDB: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_0017BCDE: ;
    MEM32(0xDA41C8) = 2;

loc_0017BCE8: ;
    eax = MEM32(esp + 0x10);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0017BBE0; /* jne: not equal / not zero */

loc_0017BCF4: ;
    esi = MEM32(esp + 0x14);
    eax = edi;
    PUSH32(esp, 0x0017BCFFu); RECOMP_ABI_CALL(0x0017CAF0u, sub_0017CAF0); /* call 0x0017CAF0 */

loc_0017BCFF: ;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebp);
    POP32(esp, ebx);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 4; return; /* ret */

}


/**
 * sub_001E86A0
 * Original: 0x001E86A0 - 0x001E8D0A (1642 bytes, 419 insns)
 * Category: game_vtable
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
static void menu_text(void)
{
    RECOMP_ENTRY(0x001E86A0u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    #define fp_push(v) do { double _fp_value = (v); \
        g_fp_top = (g_fp_top + 7u) & 7u; \
        g_fp_stack[g_fp_top] = _fp_value; } while (0)
    #define fp_pop() (g_fp_top = (g_fp_top + 1u) & 7u)
    #define fp_top() g_fp_stack[g_fp_top]
    #define fp_st(i) g_fp_stack[(g_fp_top + (i)) & 7u]
    #define fp_st1() fp_st(1)
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_001E86A0: ;
    eax = 0x1248;
    PUSH32(esp, 0x001E86AAu); RECOMP_ABI_CALL(0x0036FA70u, sub_0036FA70); /* call 0x0036FA70 */

loc_001E86AA: ;
    eax = MEM32(0x41CCD8);
    PUSH32(esp, ebx);
    MEM32(esp + 0x1248) = eax;
    eax = MEM32(esp + 0x1254);
    PUSH32(esp, ebp);
    ebp = MEM32(eax + 0x10);
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, ebx (32-bit) */
    PUSH32(esp, esi);
    if (CMP_EQ(_fa, _fb)) goto loc_001E86D1; /* je: equal / zero */

loc_001E86C9: ;
    esi = MEM32(eax + 0x38);
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(3) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 3 (32-bit) */
    if (CMP_L(_fas, _fbs)) goto loc_001E86EA; /* jl: less (signed <) */

loc_001E86D1: ;
    POP32(esp, esi);
    POP32(esp, ebp);
    eax = eax | 0xFFFFFFFFu;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    POP32(esp, ebx);
    ecx = MEM32(esp + 0x1244);
    PUSH32(esp, 0x001E86E3u); RECOMP_ABI_CALL(0x00379C29u, sub_00379C29); /* call 0x00379C29 */

loc_001E86E3: ;
    _fb = (uint32_t)(0x1248) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x1248;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 4; return; /* ret */

loc_001E86EA: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    MEMF(esp + 0x38) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(eax + 0x1C)); /* movss */
    eax = MEM32(eax + 0x20);
    PUSH32(esp, edi);
    MEM32(esp + 0x10) = eax;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    ecx = 0x7F;
    edi = esp + 0x54;
    { uint32_t _i; int32_t _st = RECOMP_DF_STEP(4); for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*_st) = eax; edi += ecx * _st; }
    ecx = 0; /* rep stosd */
    ecx = esi + esi * 2;
    edx = ZX16(MEM16(ecx * 8 + 0x452A00));
    MEM32(esp + 0x34) = edx;
    MEM32(esp + 0x1C) = ebx;
    MEM32(esp + 0x24) = ebx;
    fp_push((double)SMEM32(esp + 0x34)); /* fild */
    MEMF(esp + 0x14) = xmm0.f[0]; /* movss */
    MEM16(edi) = LO16(eax); edi += RECOMP_DF_STEP(2); /* stosw */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x43B494)); /* fmul dword ptr [0x43b494] */
    MEM32(esp + 0x28) = ebx;
    PUSH32(esp, 0x001E8744u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E8744: ;
    ecx = esi + esi * 2;
    SET_LO16(edx, MEM16(ecx * 8 + 0x452A04));
    ecx = esi + esi * 2;
    MEM16(esp + 0x34) = LO16(edx);
    edx = MEM32(ecx * 8 + 0x4529FC);
    MEM32(esp + 0x40) = edx;
    MEM32(esp + 0x2C) = eax;
    eax = esi + esi * 2;
    SET_LO16(eax, MEM16(eax * 8 + 0x452A02));
    eax = ZX16(LO16(eax));
    MEM32(esp + 0x18) = eax;
    fp_push((double)SMEM32(esp + 0x18)); /* fild */
    MEMF(esp + 0x30) = (float)fp_top(); /* fst */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x10)); /* fmul dword ptr [esp + 0x10] */
    PUSH32(esp, 0x001E8789u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E8789: ;
    edi = MEM32(esp + 0x1260);
    ecx = (uint32_t)(int32_t)SMEM8(edi + 0x4B);
    MEM32(esp + 0x20) = ecx;
    MEM32(esp + 0x18) = eax;
    MEM32(esp + 0x38) = eax;
    fp_push((double)SMEM32(esp + 0x20)); /* fild */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x43B494)); /* fmul dword ptr [0x43b494] */
    PUSH32(esp, 0x001E87AFu); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E87AF: ;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 1 (32-bit) */
    MEM32(edi + 0x28) = 0;
    MEM32(esp + 0x20) = eax;
    if (CMP_NE(_fa, _fb)) goto loc_001E8835; /* jne: not equal / not zero */

loc_001E87BF: ;
    ebx = 0x1000;
    ecx = ebp;
    eax = esp + 0x254;
    PUSH32(esp, 0x001E87D2u); RECOMP_ABI_CALL(0x001E5B10u, sub_001E5B10); /* call 0x001E5B10 */

loc_001E87D2: ;
    edx = ZX16(MEM16(esp + 0x2C));
    xmm0 = XMM_SCALAR(MEMF(0x414EF4)); /* movss */
    xmm1.f[0] = (float)(int32_t)edx; /* cvtsi2ss */
    xmm1.f[0] = xmm1.f[0] * xmm0.f[0]; /* mulss */
    ebp = esp + 0x254;
    MEMF(esp + 0x18) = xmm1.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x18), 0x0E7Fu, 32); /* cvttss2si */
    xmm1 = XMM_SCALAR(MEMF(esp + 0x30)); /* movss */
    xmm1.f[0] = xmm1.f[0] * xmm0.f[0]; /* mulss */
    MEM32(esp + 0x2C) = eax;
    MEMF(esp + 0x18) = xmm1.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x18), 0x0E7Fu, 32); /* cvttss2si */
    eax = ZX16(LO16(eax));
    xmm0.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * MEMF(esp + 0x10); /* mulss */
    MEMF(esp + 0x18) = xmm0.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x18), 0x0E7Fu, 32); /* cvttss2si */
    ebx = MEM32(esp + 0x1C);
    MEM32(esp + 0x18) = eax;

loc_001E8835: ;
    SET_LO16(ecx, MEM16(edi + 0x3C));
    _fa = (uint32_t)(LO16(ecx)) & 0xFFFFu; _fb = (uint32_t)(0) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(ecx), 0 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001E8874; /* je: equal / zero */

loc_001E883E: ;
    SET_LO8(eax, MEM8(ebp));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001E8868; /* je: equal / zero */

loc_001E8845: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0xA) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0xA (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001E8854; /* jne: not equal / not zero */

loc_001E8849: ;
    ebx++;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    edx = SX16(LO16(ecx));
    eax = ZX16(LO16(ebx));
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edx (32-bit) */
    if (CMP_GE(_fas, _fbs)) goto loc_001E8867; /* jge: greater or equal (signed >=) */

loc_001E8854: ;
    esi = MEM32(esp + 0x24);
    SET_LO8(eax, MEM8(ebp + 1));
    ebp++;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    esi++;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    MEM32(esp + 0x24) = esi;
    if (CMP_NE(_fa, _fb)) goto loc_001E8845; /* jne: not equal / not zero */

loc_001E8865: ;
    goto loc_001E8868;

loc_001E8867: ;
    ebp++;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_001E8868: ;
    MEM32(esp + 0x1C) = 0;
    ebx = MEM32(esp + 0x1C);

loc_001E8874: ;
    _fa = (uint32_t)(MEM8(ebp)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(ebp), 0 (8-bit) */
    esi = ebp;
    if (CMP_EQ(_fa, _fb)) goto loc_001E89F2; /* je: equal / zero */

loc_001E8880: ;
    SET_LO8(eax, MEM8(esi));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x40) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x40 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001E8B55; /* jne: not equal / not zero */

loc_001E888A: ;
    eax = (uint32_t)(int32_t)SMEM8(esi + 1);
    esi++;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fb = (uint32_t)(0xFFFFFFD5u) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0xFFFFFFD5u;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x4C) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x4C (32-bit) */
    if (CMP_A(_fa, _fb)) goto loc_001E89DD; /* ja: above (unsigned >) */

loc_001E889B: ;
    ecx = ZX8(MEM8(eax + 0x1E8D30));
    { uint32_t _jt = MEM32(ecx * 4 + 0x1E8D0C); /* switch: 9 entries, 9 targets */
    if (_jt == 0x001E88A9u) goto loc_001E88A9;
    if (_jt == 0x001E88D4u) goto loc_001E88D4;
    if (_jt == 0x001E88FFu) goto loc_001E88FF;
    if (_jt == 0x001E897Fu) goto loc_001E897F;
    if (_jt == 0x001E89DDu) goto loc_001E89DD;
    if (_jt == 0x001E8A84u) goto loc_001E8A84;
    if (_jt == 0x001E8A8Au) goto loc_001E8A8A;
    if (_jt == 0x001E8ADFu) goto loc_001E8ADF;
    if (_jt == 0x001E8B4Du) goto loc_001E8B4D;
    g_seh_ebp = ebp; RECOMP_ITAIL(_jt); return; }

loc_001E88A9: ;
    edx = (uint32_t)(int32_t)SMEM8(esi + 1);
    esi++;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fb = (uint32_t)(0x30) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    edx = edx - 0x30;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    MEM32(esp + 0x10) = edx;
    ebp = ZX16(LO16(ebx));
    fp_push((double)SMEM32(esp + 0x10)); /* fild */
    ebp = esp + ebp * 2 + 0x54;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x43B494)); /* fmul dword ptr [0x43b494] */
    PUSH32(esp, 0x001E88CBu); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E88CB: ;
    _fb = (uint32_t)(LO16(eax)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    MEM16(ebp) = MEM16(ebp) + LO16(eax);
    _fa = (uint32_t)(MEM16(ebp)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    goto loc_001E89DD;

loc_001E88D4: ;
    eax = (uint32_t)(int32_t)SMEM8(esi + 1);
    esi++;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fb = (uint32_t)(0x30) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - 0x30;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    MEM32(esp + 0x10) = eax;
    ebp = ZX16(LO16(ebx));
    fp_push((double)SMEM32(esp + 0x10)); /* fild */
    ebp = esp + ebp * 2 + 0x54;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x43B494)); /* fmul dword ptr [0x43b494] */
    PUSH32(esp, 0x001E88F6u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E88F6: ;
    _fb = (uint32_t)(LO16(eax)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* sub source, before the write */
    MEM16(ebp) = MEM16(ebp) - LO16(eax);
    _fa = (uint32_t)(MEM16(ebp)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* sub result */
    goto loc_001E89DD;

loc_001E88FF: ;
    SET_LO8(eax, MEM8(esi + 1));
    esi++;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    ecx = SX8(LO8(eax));
    _fb = (uint32_t)(0xFFFFFFD5u) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 0xFFFFFFD5u;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x4E) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x4E (32-bit) */
    if (CMP_A(_fa, _fb)) goto loc_001E8947; /* ja: above (unsigned >) */

loc_001E890E: ;
    ecx = ZX8(MEM8(ecx + 0x1E8D88));
    { uint32_t _jt = MEM32(ecx * 4 + 0x1E8D80); /* switch: 2 entries, 2 targets */
    if (_jt == 0x001E891Cu) goto loc_001E891C;
    if (_jt == 0x001E8947u) goto loc_001E8947;
    g_seh_ebp = ebp; RECOMP_ITAIL(_jt); return; }

loc_001E891C: ;
    xmm0 = XMM_SCALAR(MEMF(esp + 0x14)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x414EF0); /* mulss */
    MEMF(esp + 0x30) = xmm0.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x30), 0x0E7Fu, 32); /* cvttss2si */
    ecx = ZX16(LO16(ebx));
    _fb = (uint32_t)(LO16(eax)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    MEM16(esp + ecx * 2 + 0x54) = MEM16(esp + ecx * 2 + 0x54) + LO16(eax);
    _fa = (uint32_t)(MEM16(esp + ecx * 2 + 0x54)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    ecx = esp + ecx * 2 + 0x54;
    goto loc_001E89DD;

loc_001E8947: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x31) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x31 (8-bit) */
    if (CMP_L(_fas, _fbs)) goto loc_001E89DD; /* jl: less (signed <) */

loc_001E894F: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x39) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x39 (8-bit) */
    if (CMP_G(_fas, _fbs)) goto loc_001E89DD; /* jg: greater (signed >) */

loc_001E8957: ;
    xmm0 = XMM_SCALAR(MEMF(esp + 0x14)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x414EF0); /* mulss */
    MEMF(esp + 0x44) = xmm0.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x44), 0x0E7Fu, 32); /* cvttss2si */
    ecx = ZX16(LO16(ebx));
    _fb = (uint32_t)(LO16(eax)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    MEM16(esp + ecx * 2 + 0x54) = MEM16(esp + ecx * 2 + 0x54) + LO16(eax);
    _fa = (uint32_t)(MEM16(esp + ecx * 2 + 0x54)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    ecx = esp + ecx * 2 + 0x54;
    goto loc_001E89DD;

loc_001E897F: ;
    SET_LO8(ebx, MEM8(esi + 1));
    esi++;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    PUSH32(esp, 0x001E8988u); RECOMP_ABI_CALL(0x001EB830u, sub_001EB830); /* call 0x001EB830 */

loc_001E8988: ;
    SET_LO8(ecx, LO8(ebx));
    PUSH32(esp, 0x001E898Fu); RECOMP_ABI_CALL(0x001EBA00u, sub_001EBA00); /* call 0x001EBA00 */

loc_001E898F: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(7) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 7 (8-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_001E89D2; /* jae: above or equal (unsigned >=) */

loc_001E8993: ;
    edx = ZX8(LO8(eax));
    eax = MEM32(edx * 4 + 0x43882C);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFFFFFFu) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0xFFFFFFFFu (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001E89D2; /* je: equal / zero */

loc_001E89A2: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x14)); /* movss */
    if (CMP_L(_fas, _fbs)) goto loc_001E8A6B; /* jl: less (signed <) */

loc_001E89B0: ;
    xmm0.f[0] = xmm0.f[0] * MEMF(0x414EF0); /* mulss */
    MEMF(esp + 0x50) = xmm0.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x50), 0x0E7Fu, 32); /* cvttss2si */

loc_001E89C4: ;
    ecx = ZX16(MEM16(esp + 0x1C));
    _fb = (uint32_t)(LO16(eax)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    MEM16(esp + ecx * 2 + 0x54) = MEM16(esp + ecx * 2 + 0x54) + LO16(eax);
    _fa = (uint32_t)(MEM16(esp + ecx * 2 + 0x54)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    ecx = esp + ecx * 2 + 0x54;

loc_001E89D2: ;
    ebx = MEM32(esp + 0x1C);
    edi = MEM32(esp + 0x1260);

loc_001E89DD: ;
    ecx = MEM32(esp + 0x24);
    SET_LO8(eax, MEM8(esi + 1));
    esi++;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    MEM32(esp + 0x24) = ecx;
    if (CMP_NE(_fa, _fb)) goto loc_001E8880; /* jne: not equal / not zero */

loc_001E89F2: ;
    ecx = ZX16(LO16(ebx));
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO16(eax, MEM16(esp + ecx * 2 + 0x54));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0 (16-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_001E8A15; /* jbe: below or equal (unsigned <=) */

loc_001E8A01: ;
    _fb = (uint32_t)(MEM32(esp + 0x20)) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - MEM32(esp + 0x20);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fa = (uint32_t)(MEM16(esp + 0x28)) & 0xFFFFu; _fb = (uint32_t)(LO16(eax)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(esp + 0x28), LO16(eax) (16-bit) */
    MEM16(esp + ecx * 2 + 0x54) = LO16(eax);
    if (CMP_AE(_fa, _fb)) goto loc_001E8A15; /* jae: above or equal (unsigned >=) */

loc_001E8A11: ;
    MEM32(esp + 0x28) = eax;

loc_001E8A15: ;
    edx = (uint32_t)(int32_t)SMEM16(edi + 0x44);
    eax = MEM32(esp + 0x125C);
    xmm0.f[0] = (float)(int32_t)edx; /* cvtsi2ss */
    MEMF(eax) = xmm0.f[0]; /* movss */
    edx = (uint32_t)(int32_t)SMEM16(edi + 0x46);
    xmm0.f[0] = (float)(int32_t)edx; /* cvtsi2ss */
    edx = ZX16(MEM16(esp + 0x28));
    MEMF(eax + 4) = xmm0.f[0]; /* movss */
    xmm0.f[0] = (float)(int32_t)edx; /* cvtsi2ss */
    edx = ZX16(MEM16(esp + 0x38));
    POP32(esp, edi);
    POP32(esp, esi);
    MEMF(eax + 8) = xmm0.f[0]; /* movss */
    xmm0.f[0] = (float)(int32_t)edx; /* cvtsi2ss */
    POP32(esp, ebp);
    MEMF(eax + 0xC) = xmm0.f[0]; /* movss */
    eax = ecx + 1;
    POP32(esp, ebx);
    ecx = MEM32(esp + 0x1244);
    PUSH32(esp, 0x001E8A64u); RECOMP_ABI_CALL(0x00379C29u, sub_00379C29); /* call 0x00379C29 */

loc_001E8A64: ;
    _fb = (uint32_t)(0x1248) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x1248;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 4; return; /* ret */

loc_001E8A6B: ;
    xmm0.f[0] = xmm0.f[0] * MEMF(0x414EEC); /* mulss */
    MEMF(esp + 0x4C) = xmm0.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x4C), 0x0E7Fu, 32); /* cvttss2si */
    goto loc_001E89C4;

loc_001E8A84: ;
    esi++;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    goto loc_001E89DD;

loc_001E8A8A: ;
    SET_LO8(eax, MEM8(esi + 1));
    esi++;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x31) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x31 (8-bit) */
    if (CMP_L(_fas, _fbs)) goto loc_001E89DD; /* jl: less (signed <) */

loc_001E8A96: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x34) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x34 (8-bit) */
    if (CMP_G(_fas, _fbs)) goto loc_001E89DD; /* jg: greater (signed >) */

loc_001E8A9E: ;
    eax = SX8(LO8(eax));
    _fb = (uint32_t)(0x5E) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x5E;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, 0x001E8AA9u); RECOMP_ABI_CALL(0x0021A960u, sub_0021A960); /* call 0x0021A960 */

loc_001E8AA9: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001E89DD; /* je: equal / zero */

loc_001E8AB1: ;
    xmm0 = XMM_SCALAR(MEMF(eax + 8)); /* movss */
    xmm0.f[0] = xmm0.f[0] - MEMF(eax); /* subss */
    eax = ZX16(MEM16(eax + 0x14));
    xmm1.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * xmm1.f[0]; /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x43B494); /* mulss */
    ecx = (int32_t)recomp_fist((double)xmm0.f[0], 0x0E7Fu, 32); /* cvttss2si */
    MEM32(esp + 0x10) = ecx;
    fp_push((double)SMEM32(esp + 0x10)); /* fild */
    goto loc_001E8CD3;

loc_001E8ADF: ;
    SET_LO8(eax, MEM8(esi + 1));
    esi++;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x30) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x30 (8-bit) */
    if (CMP_L(_fas, _fbs)) goto loc_001E8B33; /* jl: less (signed <) */

loc_001E8AE7: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x39) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x39 (8-bit) */
    if (CMP_G(_fas, _fbs)) goto loc_001E8B33; /* jg: greater (signed >) */

loc_001E8AEB: ;
    edx = SX8(LO8(eax));
    SET_LO8(eax, MEM8(esi + 2));
    _fb = (uint32_t)(2) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + 2;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(0x30) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    edx = edx - 0x30;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x30) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x30 (8-bit) */
    xmm1.f[0] = (float)(int32_t)edx; /* cvtsi2ss */
    if (CMP_L(_fas, _fbs)) goto loc_001E8B33; /* jl: less (signed <) */

loc_001E8AFF: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x39) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x39 (8-bit) */
    if (CMP_G(_fas, _fbs)) goto loc_001E8B33; /* jg: greater (signed >) */

loc_001E8B03: ;
    eax = SX8(LO8(eax));
    _fb = (uint32_t)(0x30) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - 0x30;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    xmm0.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x41492C); /* mulss */
    xmm0.f[0] = xmm0.f[0] + xmm1.f[0]; /* addss */
    xmm1 = XMM_SCALAR(MEMF(edi + 0x1C)); /* movss */
    xmm1.f[0] = xmm1.f[0] * xmm0.f[0]; /* mulss */
    MEMF(esp + 0x3C) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x14) = xmm1.f[0]; /* movss */
    goto loc_001E89DD;

loc_001E8B33: ;
    xmm0 = XMM_SCALAR(MEMF(esp + 0x3C)); /* movss */
    xmm1 = XMM_SCALAR(MEMF(edi + 0x1C)); /* movss */
    xmm1.f[0] = xmm1.f[0] * xmm0.f[0]; /* mulss */
    MEMF(esp + 0x14) = xmm1.f[0]; /* movss */
    goto loc_001E89DD;

loc_001E8B4D: ;
    _fb = (uint32_t)(3) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + 3;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_001E89DD;

loc_001E8B55: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0xA) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0xA (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001E8BB0; /* jne: not equal / not zero */

loc_001E8B59: ;
    SET_LO16(ecx, MEM16(esp + 0x20));
    eax = ZX16(LO16(ebx));
    _fb = (uint32_t)(LO16(ecx)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* sub source, before the write */
    MEM16(esp + eax * 2 + 0x54) = MEM16(esp + eax * 2 + 0x54) - LO16(ecx);
    _fa = (uint32_t)(MEM16(esp + eax * 2 + 0x54)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* sub result */
    eax = esp + eax * 2 + 0x54;
    eax = ZX16(MEM16(eax));
    _fa = (uint32_t)(MEM16(esp + 0x28)) & 0xFFFFu; _fb = (uint32_t)(LO16(eax)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(esp + 0x28), LO16(eax) (16-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_001E8B78; /* jae: above or equal (unsigned >=) */

loc_001E8B74: ;
    MEM32(esp + 0x28) = eax;

loc_001E8B78: ;
    ebx++;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(LO16(ebx)) & 0xFFFFu; _fb = (uint32_t)(0xFF) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(ebx), 0xFF (16-bit) */
    MEM32(esp + 0x1C) = ebx;
    if (CMP_AE(_fa, _fb)) goto loc_001E8CF0; /* jae: above or equal (unsigned >=) */

loc_001E8B88: ;
    SET_LO16(edx, ZX8(MEM8(edi + 0x4D)));
    _fa = (uint32_t)(LO16(ebx)) & 0xFFFFu; _fb = (uint32_t)(LO16(edx)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(ebx), LO16(edx) (16-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_001E89F2; /* jae: above or equal (unsigned >=) */

loc_001E8B96: ;
    SET_LO16(eax, (uint32_t)(int32_t)SMEM8(edi + 0x4C));
    edx = MEM32(esp + 0x18);
    ecx = MEM32(esp + 0x38);
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + edx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esp + 0x38) = ecx;
    goto loc_001E89DD;

loc_001E8BB0: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x20) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x20 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001E8CC6; /* je: equal / zero */

loc_001E8BB8: ;
    eax = MEM32(esp + 0x40);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001E8BCF; /* je: equal / zero */

loc_001E8BC0: ;
    ecx = ZX16(MEM16(esp + 0x2C));
    MEM32(esp + 0x10) = ecx;
    fp_push((double)SMEM32(esp + 0x10)); /* fild */
    goto loc_001E8C31;

loc_001E8BCF: ;
    eax = MEM32(edi + 0x38);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(3) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 3 (32-bit) */
    if (CMP_L(_fas, _fbs)) goto loc_001E8BDB; /* jl: less (signed <) */

loc_001E8BD7: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    goto loc_001E8BFB;

loc_001E8BDB: ;
    edx = eax + eax * 2;
    eax = esp + 0x48;
    PUSH32(esp, esi);
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, eax);
    { uint32_t _icall_target = MEM32(edx * 8 + 0x452A08); PUSH32(esp, 0x001E8BEBu); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x001E8BE4u); } /* indirect call */
    }

loc_001E8BEB: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001E8BF8; /* je: equal / zero */

loc_001E8BF2: ;
    eax = MEM32(esp + 0x48);
    goto loc_001E8BFB;

loc_001E8BF8: ;
    eax = eax | 0xFFFFFFFFu;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */

loc_001E8BFB: ;
    PUSH32(esp, 0x001E8C00u); RECOMP_ABI_CALL(0x0021A960u, sub_0021A960); /* call 0x0021A960 */

loc_001E8C00: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001E89DD; /* je: equal / zero */

loc_001E8C08: ;
    ecx = ZX16(MEM16(eax + 0x14));
    xmm0 = XMM_SCALAR(MEMF(eax + 8)); /* movss */
    xmm0.f[0] = xmm0.f[0] - MEMF(eax); /* subss */
    xmm1.f[0] = (float)(int32_t)ecx; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * xmm1.f[0]; /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x43B494); /* mulss */
    edx = (int32_t)recomp_fist((double)xmm0.f[0], 0x0E7Fu, 32); /* cvttss2si */
    MEM32(esp + 0x10) = edx;
    fp_push((double)SMEM32(esp + 0x10)); /* fild */

loc_001E8C31: ;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x14)); /* fmul dword ptr [esp + 0x14] */
    edi = ZX16(LO16(ebx));
    edi = esp + edi * 2 + 0x54;
    PUSH32(esp, 0x001E8C41u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E8C41: ;
    _fb = (uint32_t)(LO16(eax)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    MEM16(edi) = MEM16(edi) + LO16(eax);
    _fa = (uint32_t)(MEM16(edi)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    SET_LO16(eax, MEM16(esp + 0x20));
    _fb = (uint32_t)(LO16(eax)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    MEM16(edi) = MEM16(edi) + LO16(eax);
    _fa = (uint32_t)(MEM16(edi)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    _fa = (uint32_t)(MEM16(esp + 0x24)) & 0xFFFFu; _fb = (uint32_t)(0) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(esp + 0x24), 0 (16-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_001E8CA5; /* jbe: below or equal (unsigned <=) */

loc_001E8C54: ;
    ecx = MEM32(esp + 0x1260);
    eax = MEM32(ecx + 0x38);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(3) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 3 (32-bit) */
    if (CMP_GE(_fas, _fbs)) goto loc_001E8C8C; /* jge: greater or equal (signed >=) */

loc_001E8C63: ;
    edx = eax + eax * 2;
    eax = MEM32(edx * 8 + 0x452A0C);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001E8C8C; /* je: equal / zero */

loc_001E8C71: ;
    PUSH32(esp, esi);
    { uint32_t _icall_esp = g_esp;
    { uint32_t _icall_target = eax; PUSH32(esp, 0x001E8C74u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x001E8C72u); } /* indirect call */
    }

loc_001E8C74: ;
    eax = SX16(LO16(eax));
    xmm0.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x43B494); /* mulss */
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = (int32_t)recomp_fist((double)xmm0.f[0], 0x0E7Fu, 32); /* cvttss2si */
    goto loc_001E8C8E;

loc_001E8C8C: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */

loc_001E8C8E: ;
    ecx = SX16(LO16(eax));
    MEM32(esp + 0x10) = ecx;
    fp_push((double)SMEM32(esp + 0x10)); /* fild */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x14)); /* fmul dword ptr [esp + 0x14] */
    PUSH32(esp, 0x001E8CA2u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E8CA2: ;
    _fb = (uint32_t)(LO16(eax)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    MEM16(edi) = MEM16(edi) + LO16(eax);
    _fa = (uint32_t)(MEM16(edi)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */

loc_001E8CA5: ;
    _fa = (uint32_t)(MEM16(esp + 0x34)) & 0xFFFFu; _fb = (uint32_t)(1) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(esp + 0x34), 1 (16-bit) */
    edi = MEM32(esp + 0x1260);
    if (CMP_BE(_fa, _fb)) goto loc_001E89DD; /* jbe: below or equal (unsigned <=) */

loc_001E8CB8: ;
    edx = ZX16(MEM16(esp + 0x34));
    esi = esi + edx + -1;
    goto loc_001E89DD;

loc_001E8CC6: ;
    eax = ZX16(MEM16(esp + 0x2C));
    MEM32(esp + 0x10) = eax;
    fp_push((double)SMEM32(esp + 0x10)); /* fild */

loc_001E8CD3: ;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x14)); /* fmul dword ptr [esp + 0x14] */
    ebp = ZX16(LO16(ebx));
    ebp = esp + ebp * 2 + 0x54;
    PUSH32(esp, 0x001E8CE3u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E8CE3: ;
    _fb = (uint32_t)(MEM32(esp + 0x20)) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + MEM32(esp + 0x20);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(LO16(eax)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    MEM16(ebp) = MEM16(ebp) + LO16(eax);
    _fa = (uint32_t)(MEM16(ebp)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    goto loc_001E89DD;

loc_001E8CF0: ;
    ecx = MEM32(esp + 0x1254);
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebp);
    eax = eax | 0xFFFFFFFFu;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    POP32(esp, ebx);
    PUSH32(esp, 0x001E8D03u); RECOMP_ABI_CALL(0x00379C29u, sub_00379C29); /* call 0x00379C29 */

loc_001E8D03: ;
    _fb = (uint32_t)(0x1248) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x1248;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 4; return; /* ret */

    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}


/**
 * sub_001E6D30
 * Original: 0x001E6D30 - 0x001E86A2 (6514 bytes, 1711 insns)
 * Category: game_vtable
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: standard_frame
 */
static void text_draw(void)
{
    RECOMP_ENTRY(0x001E6D30u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* prologue saves caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    double _fca = 0.0, _fcb = 0.0;
    (void)_fca; (void)_fcb;
    int _cf = 0; /* carry flag */
    #define fp_push(v) do { double _fp_value = (v); \
        g_fp_top = (g_fp_top + 7u) & 7u; \
        g_fp_stack[g_fp_top] = _fp_value; } while (0)
    #define fp_pop() (g_fp_top = (g_fp_top + 1u) & 7u)
    #define fp_top() g_fp_stack[g_fp_top]
    #define fp_st(i) g_fp_stack[(g_fp_top + (i)) & 7u]
    #define fp_st1() fp_st(1)

loc_001E6D30: ;
    PUSH32(esp, ebp);
    ebp = esp;
    g_ebp = ebp; /* publish frame for frameless callees */
    g_seh_ebp = ebp;
    _cf = 0; /* logical op clears CF */
    esp = esp & 0xFFFFFFF0u;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax = 0x14C4;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E6D40u); RECOMP_ABI_CALL(0x0036FA70u, sub_0036FA70); /* call 0x0036FA70 */

loc_001E6D40: ;
    eax = MEM32(0x41CCD8);
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    PUSH32(esp, ebx);
    PUSH32(esp, esi);
    esi = MEM32(ebp + 8);
    MEM32(esp + 0x14C8) = eax;
    eax = MEM32(esi);
    PUSH32(esp, edi);
    MEM32(esp + 0x60) = eax;
    _cf = 0; /* xor clears CF */
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    ecx = 0x7F;
    edi = esp + 0x2CA;
    MEM16(esp + 0x2C8) = 0;
    { uint32_t _i; int32_t _st = RECOMP_DF_STEP(4); for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*_st) = eax; edi += ecx * _st; }
    ecx = 0; /* rep stosd */
    eax = MEM32(esi + 0x10);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    MEMF(esp + 0xAC) = xmm0.f[0]; /* movss */
    if (CMP_EQ(_fa, _fb)) goto loc_001E7503; /* je: equal / zero */

loc_001E6D8E: ;
    ebx = MEM32(esi + 0x38);
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(3) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, 3 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_GE(_fas, _fbs)) goto loc_001E7503; /* jge: greater or equal (signed >=) */

loc_001E6D9A: ;
    ecx = MEM32(esi + 8);
    MEM32(esp + 0x28) = eax;
    eax = MEM32(esi + 0x20);
    MEM32(esp + 0x90) = ecx;
    _cf = 0; /* xor clears CF */
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    MEM32(esp + 0xC) = eax;
    _cf = 0; /* xor clears CF */
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    ecx = 0x7F;
    edi = esp + 0xC8;
    { uint32_t _i; int32_t _st = RECOMP_DF_STEP(4); for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*_st) = eax; edi += ecx * _st; }
    ecx = 0; /* rep stosd */
    ecx = ebx + ebx * 2;
    MEM32(esp + 0x20) = edx;
    MEM32(esp + 0x6C) = edx;
    MEM32(esp + 0x14) = edx;
    MEM32(esp + 0x10) = edx;
    edx = ZX16(MEM16(ecx * 8 + 0x452A00));
    MEM32(esp + 0x8C) = edx;
    MEMF(esp + 0x84) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esi + 0x1C)); /* movss */
    fp_push((double)SMEM32(esp + 0x8C)); /* fild */
    MEMF(esp + 0x1C) = xmm0.f[0]; /* movss */
    MEM16(edi) = LO16(eax); edi += RECOMP_DF_STEP(2); /* stosw */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x43B494)); /* fmul dword ptr [0x43b494] */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E6E0Bu); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E6E0B: ;
    ecx = ebx + ebx * 2;
    SET_LO16(edx, MEM16(ecx * 8 + 0x452A04));
    ecx = ebx + ebx * 2;
    MEM16(esp + 0x8C) = LO16(edx);
    edx = MEM32(ecx * 8 + 0x4529FC);
    MEM32(esp + 0x88) = eax;
    eax = ebx + ebx * 2;
    SET_LO16(eax, MEM16(eax * 8 + 0x452A02));
    eax = ZX16(LO16(eax));
    MEM32(esp + 0x64) = eax;
    MEM32(esp + 0xA0) = edx;
    fp_push((double)SMEM32(esp + 0x64)); /* fild */
    MEMF(esp + 0x24) = (float)fp_top(); /* fst */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0xC)); /* fmul dword ptr [esp + 0xc] */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E6E59u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E6E59: ;
    ecx = (uint32_t)(int32_t)SMEM8(esi + 0x4B);
    MEM32(esp + 0x68) = ecx;
    fp_push((double)SMEM32(esp + 0x68)); /* fild */
    MEM32(esp + 0x64) = eax;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x43B494)); /* fmul dword ptr [0x43b494] */
    MEM32(esp + 0x18) = eax;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E6E78u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E6E78: ;
    edx = esp + 0x70;
    MEM32(esp + 0x68) = edx;
    MEM32(esp + 0x2C) = eax;
    eax = 0x436330;
    ecx = MEM32(esp + 0x68);
    xmm0 = XMM_MEM(eax); /* movaps */
    XMM_STORE(ecx, xmm0); /* movaps */
    fp_push(MEMF(esp + 0x7C)); /* fld float */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E6E9Cu); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E6E9C: ;
    fp_push(MEMF(esp + 0x70)); /* fld float */
    edi = eax;
    if (8) _cf = (int)(((edi) >> (32 - (8))) & 1);
    edi = edi << 8;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E6EAAu); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E6EAA: ;
    fp_push(MEMF(esp + 0x74)); /* fld float */
    _cf = 0; /* logical op clears CF */
    eax = eax & 0xFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _cf = 0; /* logical op clears CF */
    edi = edi | eax;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    if (8) _cf = (int)(((edi) >> (32 - (8))) & 1);
    edi = edi << 8;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E6EBDu); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E6EBD: ;
    fp_push(MEMF(esp + 0x78)); /* fld float */
    _cf = 0; /* logical op clears CF */
    eax = eax & 0xFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _cf = 0; /* logical op clears CF */
    edi = edi | eax;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    if (8) _cf = (int)(((edi) >> (32 - (8))) & 1);
    edi = edi << 8;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E6ED0u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E6ED0: ;
    _cf = 0; /* logical op clears CF */
    eax = eax & 0xFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _cf = 0; /* logical op clears CF */
    edi = edi | eax;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    _fa = (uint32_t)(MEM32(esi + 0x38)) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(esi + 0x38), 1 (32-bit) */
    _cf = (int)(_fa < _fb);
    MEM32(esp + 0x68) = edi;
    MEM32(esi + 0x28) = 0;
    if (CMP_NE(_fa, _fb)) goto loc_001E6F77; /* jne: not equal / not zero */

loc_001E6EEC: ;
    ecx = MEM32(esp + 0x28);
    ebx = 0x1000;
    eax = esp + 0x4C8;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E6F01u); RECOMP_ABI_CALL(0x001E5B10u, sub_001E5B10); /* call 0x001E5B10 */

loc_001E6F01: ;
    ecx = ZX16(MEM16(esp + 0x88));
    xmm0 = XMM_SCALAR(MEMF(0x414EF4)); /* movss */
    eax = esp + 0x4C8;
    xmm1.f[0] = (float)(int32_t)ecx; /* cvtsi2ss */
    xmm1.f[0] = xmm1.f[0] * xmm0.f[0]; /* mulss */
    MEM32(esp + 0x28) = eax;
    MEMF(esp + 0xAC) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x64) = xmm1.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x64), 0x0E7Fu, 32); /* cvttss2si */
    xmm1 = XMM_SCALAR(MEMF(esp + 0x24)); /* movss */
    xmm1.f[0] = xmm1.f[0] * xmm0.f[0]; /* mulss */
    MEM32(esp + 0x88) = eax;
    MEMF(esp + 0x64) = xmm1.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x64), 0x0E7Fu, 32); /* cvttss2si */
    edx = ZX16(LO16(eax));
    xmm0.f[0] = (float)(int32_t)edx; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * MEMF(esp + 0xC); /* mulss */
    MEMF(esp + 0x64) = xmm0.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x64), 0x0E7Fu, 32); /* cvttss2si */
    MEM32(esp + 0x64) = eax;
    MEM32(esp + 0x18) = eax;

loc_001E6F77: ;
    SET_LO16(eax, MEM16(esi + 0x3C));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0 (16-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E6FD4; /* je: equal / zero */

loc_001E6F80: ;
    ecx = MEM32(esp + 0x28);
    SET_LO8(eax, MEM8(ecx));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E6FC4; /* je: equal / zero */

loc_001E6F8A: ;
    edx = MEM32(esp + 0x6C);
    edi = MEM32(esp + 0x20);

loc_001E6F92: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0xA) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0xA (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_NE(_fa, _fb)) goto loc_001E6FA2; /* jne: not equal / not zero */

loc_001E6F96: ;
    eax = (uint32_t)(int32_t)SMEM16(esi + 0x3C);
    edi++;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    ebx = ZX16(LO16(edi));
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, eax (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_GE(_fas, _fbs)) goto loc_001E6FBF; /* jge: greater or equal (signed >=) */

loc_001E6FA2: ;
    SET_LO8(eax, MEM8(ecx + 1));
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    edx++;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_NE(_fa, _fb)) goto loc_001E6F92; /* jne: not equal / not zero */

loc_001E6FAB: ;
    ebx = ecx;
    MEM32(esp + 0x6C) = edx;
    MEM32(esp + 0x20) = 0;
    MEM32(esp + 0x28) = ebx;
    goto loc_001E6FD8;

loc_001E6FBF: ;
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(esp + 0x6C) = edx;

loc_001E6FC4: ;
    ebx = ecx;
    MEM32(esp + 0x20) = 0;
    MEM32(esp + 0x28) = ebx;
    goto loc_001E6FD8;

loc_001E6FD4: ;
    ebx = MEM32(esp + 0x28);

loc_001E6FD8: ;
    SET_LO8(eax, MEM8(ebx));
    _cf = 0; /* xor clears CF */
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    edi = ebx;
    MEM32(esp + 0x24) = edx;
    if (CMP_EQ(_fa, _fb)) goto loc_001E7014; /* je: equal / zero */

loc_001E6FE6: ;
    SET_LO8(ecx, LO8(eax));

loc_001E6FE8: ;
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0xA) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), 0xA (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_NE(_fa, _fb)) goto loc_001E7004; /* jne: not equal / not zero */

loc_001E6FED: ;
    edx++;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFF) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0xFF (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_GE(_fas, _fbs)) goto loc_001E7014; /* jge: greater or equal (signed >=) */

loc_001E6FF6: ;
    ecx = ZX8(MEM8(esi + 0x4D));
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, ecx (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_GE(_fas, _fbs)) goto loc_001E7014; /* jge: greater or equal (signed >=) */

loc_001E6FFE: ;
    MEM32(esp + 0x24) = MEM32(esp + 0x24) + 1;
    _fa = (uint32_t)(MEM32(esp + 0x24)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    goto loc_001E700C;

loc_001E7004: ;
    MEM32(esp + 0x24) = 0;

loc_001E700C: ;
    SET_LO8(ecx, MEM8(edi + 1));
    edi++;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), 0 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_NE(_fa, _fb)) goto loc_001E6FE8; /* jne: not equal / not zero */

loc_001E7014: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    edi = ebx;
    if (CMP_EQ(_fa, _fb)) goto loc_001E7519; /* je: equal / zero */

loc_001E701E: ;
    edi = edi;

loc_001E7020: ;
    SET_LO8(eax, MEM8(edi));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x40) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x40 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_NE(_fa, _fb)) goto loc_001E72B8; /* jne: not equal / not zero */

loc_001E702A: ;
    eax = (uint32_t)(int32_t)SMEM8(edi + 1);
    edi++;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fb = (uint32_t)(0xFFFFFFD5u) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(0xFFFFFFD5u)) >> 32) & 1);
    eax = eax + 0xFFFFFFD5u;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x4C) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x4C (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_A(_fa, _fb)) goto loc_001E748E; /* ja: above (unsigned >) */

loc_001E703B: ;
    edx = ZX8(MEM8(eax + 0x1E8514));
    { uint32_t _jt = MEM32(edx * 4 + 0x1E84F0); /* switch: 9 entries, 9 targets */
    if (_jt == 0x001E7049u) goto loc_001E7049;
    if (_jt == 0x001E7075u) goto loc_001E7075;
    if (_jt == 0x001E70A4u) goto loc_001E70A4;
    if (_jt == 0x001E7143u) goto loc_001E7143;
    if (_jt == 0x001E71DEu) goto loc_001E71DE;
    if (_jt == 0x001E71E4u) goto loc_001E71E4;
    if (_jt == 0x001E723Du) goto loc_001E723D;
    if (_jt == 0x001E72B0u) goto loc_001E72B0;
    if (_jt == 0x001E748Eu) goto loc_001E748E;
    g_seh_ebp = ebp; RECOMP_ITAIL(_jt); return; }

loc_001E7049: ;
    eax = (uint32_t)(int32_t)SMEM8(edi + 1);
    ebx = ZX16(MEM16(esp + 0x20));
    edi++;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fb = (uint32_t)(0x30) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(eax) < (uint32_t)(0x30));
    eax = eax - 0x30;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    MEM32(esp + 0xC) = eax;
    ebx = esp + ebx * 2 + 0xC8;
    fp_push((double)SMEM32(esp + 0xC)); /* fild */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x43B494)); /* fmul dword ptr [0x43b494] */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7070u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E7070: ;
    goto loc_001E748B;

loc_001E7075: ;
    ecx = (uint32_t)(int32_t)SMEM8(edi + 1);
    ebx = ZX16(MEM16(esp + 0x20));
    edi++;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fb = (uint32_t)(0x30) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(ecx) < (uint32_t)(0x30));
    ecx = ecx - 0x30;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    MEM32(esp + 0xC) = ecx;
    ebx = esp + ebx * 2 + 0xC8;
    fp_push((double)SMEM32(esp + 0xC)); /* fild */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x43B494)); /* fmul dword ptr [0x43b494] */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E709Cu); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E709C: ;
    _fb = (uint32_t)(LO16(eax)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(MEM16(ebx)) < (uint32_t)(LO16(eax)));
    MEM16(ebx) = MEM16(ebx) - LO16(eax);
    _fa = (uint32_t)(MEM16(ebx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* sub result */
    goto loc_001E748E;

loc_001E70A4: ;
    SET_LO8(eax, MEM8(edi + 1));
    edi++;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    ecx = SX8(LO8(eax));
    _fb = (uint32_t)(0xFFFFFFD5u) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(ecx) + (uint64_t)(0xFFFFFFD5u)) >> 32) & 1);
    ecx = ecx + 0xFFFFFFD5u;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x4E) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x4E (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_A(_fa, _fb)) goto loc_001E70FA; /* ja: above (unsigned >) */

loc_001E70B3: ;
    edx = ZX8(MEM8(ecx + 0x1E856C));
    { uint32_t _jt = MEM32(edx * 4 + 0x1E8564); /* switch: 2 entries, 2 targets */
    if (_jt == 0x001E70C1u) goto loc_001E70C1;
    if (_jt == 0x001E70FAu) goto loc_001E70FA;
    g_seh_ebp = ebp; RECOMP_ITAIL(_jt); return; }

loc_001E70C1: ;
    xmm0 = XMM_SCALAR(MEMF(esp + 0x1C)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x414EF0); /* mulss */
    MEMF(esp + 0x9C) = xmm0.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x9C), 0x0E7Fu, 32); /* cvttss2si */
    ecx = ZX16(MEM16(esp + 0x20));
    _fb = (uint32_t)(LO16(eax)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM16(esp + ecx * 2 + 0xC8)) + (uint64_t)(LO16(eax))) >> 16) & 1);
    MEM16(esp + ecx * 2 + 0xC8) = MEM16(esp + ecx * 2 + 0xC8) + LO16(eax);
    _fa = (uint32_t)(MEM16(esp + ecx * 2 + 0xC8)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    ecx = esp + ecx * 2 + 0xC8;
    goto loc_001E748E;

loc_001E70FA: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x31) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x31 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_L(_fas, _fbs)) goto loc_001E748E; /* jl: less (signed <) */

loc_001E7102: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x39) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x39 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_001E748E; /* jg: greater (signed >) */

loc_001E710A: ;
    xmm0 = XMM_SCALAR(MEMF(esp + 0x1C)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x414EF0); /* mulss */
    MEMF(esp + 0xA8) = xmm0.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0xA8), 0x0E7Fu, 32); /* cvttss2si */
    ecx = ZX16(MEM16(esp + 0x20));
    _fb = (uint32_t)(LO16(eax)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM16(esp + ecx * 2 + 0xC8)) + (uint64_t)(LO16(eax))) >> 16) & 1);
    MEM16(esp + ecx * 2 + 0xC8) = MEM16(esp + ecx * 2 + 0xC8) + LO16(eax);
    _fa = (uint32_t)(MEM16(esp + ecx * 2 + 0xC8)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    ecx = esp + ecx * 2 + 0xC8;
    goto loc_001E748E;

loc_001E7143: ;
    SET_LO8(ebx, MEM8(edi + 1));
    edi++;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E714Cu); RECOMP_ABI_CALL(0x001EB830u, sub_001EB830); /* call 0x001EB830 */

loc_001E714C: ;
    SET_LO8(ecx, LO8(ebx));
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7153u); RECOMP_ABI_CALL(0x001EBA00u, sub_001EBA00); /* call 0x001EBA00 */

loc_001E7153: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(7) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 7 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_AE(_fa, _fb)) goto loc_001E748E; /* jae: above or equal (unsigned >=) */

loc_001E715B: ;
    eax = ZX8(LO8(eax));
    eax = MEM32(eax * 4 + 0x43882C);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFFFFFFu) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0xFFFFFFFFu (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_EQ(_fa, _fb)) goto loc_001E748E; /* je: equal / zero */

loc_001E716E: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x1C)); /* movss */
    if (CMP_L(_fas, _fbs)) goto loc_001E71AB; /* jl: less (signed <) */

loc_001E7178: ;
    xmm0.f[0] = xmm0.f[0] * MEMF(0x414EF0); /* mulss */
    MEMF(esp + 0xA4) = xmm0.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0xA4), 0x0E7Fu, 32); /* cvttss2si */
    ecx = ZX16(MEM16(esp + 0x20));
    _fb = (uint32_t)(LO16(eax)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM16(esp + ecx * 2 + 0xC8)) + (uint64_t)(LO16(eax))) >> 16) & 1);
    MEM16(esp + ecx * 2 + 0xC8) = MEM16(esp + ecx * 2 + 0xC8) + LO16(eax);
    _fa = (uint32_t)(MEM16(esp + ecx * 2 + 0xC8)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    ecx = esp + ecx * 2 + 0xC8;
    goto loc_001E748E;

loc_001E71AB: ;
    xmm0.f[0] = xmm0.f[0] * MEMF(0x414EEC); /* mulss */
    MEMF(esp + 0x98) = xmm0.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x98), 0x0E7Fu, 32); /* cvttss2si */
    ecx = ZX16(MEM16(esp + 0x20));
    _fb = (uint32_t)(LO16(eax)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM16(esp + ecx * 2 + 0xC8)) + (uint64_t)(LO16(eax))) >> 16) & 1);
    MEM16(esp + ecx * 2 + 0xC8) = MEM16(esp + ecx * 2 + 0xC8) + LO16(eax);
    _fa = (uint32_t)(MEM16(esp + ecx * 2 + 0xC8)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    ecx = esp + ecx * 2 + 0xC8;
    goto loc_001E748E;

loc_001E71DE: ;
    edi++;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    goto loc_001E748E;

loc_001E71E4: ;
    SET_LO8(eax, MEM8(edi + 1));
    edi++;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x31) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x31 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_L(_fas, _fbs)) goto loc_001E748E; /* jl: less (signed <) */

loc_001E71F0: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x34) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x34 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_001E748E; /* jg: greater (signed >) */

loc_001E71F8: ;
    eax = SX8(LO8(eax));
    _fb = (uint32_t)(0x5E) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(0x5E)) >> 32) & 1);
    eax = eax + 0x5E;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esp + 0x34) = eax;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7207u); RECOMP_ABI_CALL(0x0021A960u, sub_0021A960); /* call 0x0021A960 */

loc_001E7207: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E748E; /* je: equal / zero */

loc_001E720F: ;
    ecx = ZX16(MEM16(eax + 0x14));
    xmm0 = XMM_SCALAR(MEMF(eax + 8)); /* movss */
    xmm0.f[0] = xmm0.f[0] - MEMF(eax); /* subss */
    xmm1.f[0] = (float)(int32_t)ecx; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * xmm1.f[0]; /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x43B494); /* mulss */
    edx = (int32_t)recomp_fist((double)xmm0.f[0], 0x0E7Fu, 32); /* cvttss2si */
    MEM32(esp + 0xC) = edx;
    fp_push((double)SMEM32(esp + 0xC)); /* fild */
    goto loc_001E7472;

loc_001E723D: ;
    SET_LO8(eax, MEM8(edi + 1));
    edi++;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x30) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x30 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_L(_fas, _fbs)) goto loc_001E7293; /* jl: less (signed <) */

loc_001E7245: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x39) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x39 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_001E7293; /* jg: greater (signed >) */

loc_001E7249: ;
    eax = SX8(LO8(eax));
    _fb = (uint32_t)(0x30) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(eax) < (uint32_t)(0x30));
    eax = eax - 0x30;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fb = (uint32_t)(2) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(edi) + (uint64_t)(2)) >> 32) & 1);
    edi = edi + 2;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    xmm1.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    SET_LO8(eax, MEM8(edi));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x30) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x30 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_L(_fas, _fbs)) goto loc_001E7293; /* jl: less (signed <) */

loc_001E725C: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x39) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x39 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_001E7293; /* jg: greater (signed >) */

loc_001E7260: ;
    ecx = SX8(LO8(eax));
    _fb = (uint32_t)(0x30) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(ecx) < (uint32_t)(0x30));
    ecx = ecx - 0x30;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    xmm0.f[0] = (float)(int32_t)ecx; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x41492C); /* mulss */
    xmm0.f[0] = xmm0.f[0] + xmm1.f[0]; /* addss */
    xmm1 = XMM_SCALAR(MEMF(esi + 0x1C)); /* movss */
    xmm1.f[0] = xmm1.f[0] * xmm0.f[0]; /* mulss */
    MEMF(esp + 0x84) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x1C) = xmm1.f[0]; /* movss */
    goto loc_001E748E;

loc_001E7293: ;
    xmm0 = XMM_SCALAR(MEMF(esp + 0x84)); /* movss */
    xmm1 = XMM_SCALAR(MEMF(esi + 0x1C)); /* movss */
    xmm1.f[0] = xmm1.f[0] * xmm0.f[0]; /* mulss */
    MEMF(esp + 0x1C) = xmm1.f[0]; /* movss */
    goto loc_001E748E;

loc_001E72B0: ;
    _fb = (uint32_t)(3) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(edi) + (uint64_t)(3)) >> 32) & 1);
    edi = edi + 3;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_001E748E;

loc_001E72B8: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0xA) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0xA (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_NE(_fa, _fb)) goto loc_001E734F; /* jne: not equal / not zero */

loc_001E72C0: ;
    eax = ZX16(MEM16(esp + 0x20));
    SET_LO16(edx, MEM16(esp + 0x2C));
    SET_LO16(ecx, MEM16(esi + 0x44));
    if (1) _cf = (int)(((eax) >> (32 - (1))) & 1);
    eax = eax << 1;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(LO16(edx)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(MEM16(esp + eax + 0xC8)) < (uint32_t)(LO16(edx)));
    MEM16(esp + eax + 0xC8) = MEM16(esp + eax + 0xC8) - LO16(edx);
    _fa = (uint32_t)(MEM16(esp + eax + 0xC8)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* sub result */
    SET_LO16(ebx, MEM16(esp + eax + 0xC8));
    _cf = 0; /* xor clears CF */
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(edx, MEM8(esi + 0x4A));
    eax = esp + eax + 0x2C8;
    MEM16(eax) = LO16(ecx);
    _cf = 0; /* logical op clears CF */
    edx = edx & 0xF;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    edx--;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa == 0)) goto loc_001E72FC; /* je: equal / zero */

loc_001E72F5: ;
    edx--;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa != 0)) goto loc_001E7309; /* jne: not equal / not zero */

loc_001E72F8: ;
    _fb = (uint32_t)(ebx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(ecx) < (uint32_t)(ebx));
    ecx = ecx - ebx;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    goto loc_001E7306;

loc_001E72FC: ;
    _cf = 0; /* xor clears CF */
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO16(edx, LO16(ebx));
    if (1) _cf = (int)(((LO16(edx)) >> ((1) - 1)) & 1);
    SET_LO16(edx, LO16(edx) >> 1);
    _fa = (uint32_t)(LO16(edx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* shift result */
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(ecx) < (uint32_t)(edx));
    ecx = ecx - edx;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */

loc_001E7306: ;
    MEM16(eax) = LO16(ecx);

loc_001E7309: ;
    _fa = (uint32_t)(MEM16(esp + 0x10)) & 0xFFFFu; _fb = (uint32_t)(LO16(ebx)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(esp + 0x10), LO16(ebx) (16-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_AE(_fa, _fb)) goto loc_001E7314; /* jae: above or equal (unsigned >=) */

loc_001E7310: ;
    MEM32(esp + 0x10) = ebx;

loc_001E7314: ;
    eax = MEM32(esp + 0x20);
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0xFF) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0xFF (16-bit) */
    _cf = (int)(_fa < _fb);
    MEM32(esp + 0x20) = eax;
    if (CMP_AE(_fa, _fb)) goto loc_001E7503; /* jae: above or equal (unsigned >=) */

loc_001E7327: ;
    SET_LO16(ecx, ZX8(MEM8(esi + 0x4D)));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(LO16(ecx)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), LO16(ecx) (16-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_AE(_fa, _fb)) goto loc_001E74A7; /* jae: above or equal (unsigned >=) */

loc_001E7335: ;
    SET_LO16(edx, (uint32_t)(int32_t)SMEM8(esi + 0x4C));
    ecx = MEM32(esp + 0x64);
    eax = MEM32(esp + 0x18);
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(edx) + (uint64_t)(ecx)) >> 32) & 1);
    edx = edx + ecx;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(edx)) >> 32) & 1);
    eax = eax + edx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esp + 0x18) = eax;
    goto loc_001E748E;

loc_001E734F: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x20) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x20 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_EQ(_fa, _fb)) goto loc_001E7462; /* je: equal / zero */

loc_001E7357: ;
    eax = MEM32(esp + 0xA0);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E7374; /* je: equal / zero */

loc_001E7362: ;
    eax = ZX16(MEM16(esp + 0x88));
    MEM32(esp + 0xC) = eax;
    fp_push((double)SMEM32(esp + 0xC)); /* fild */
    goto loc_001E73DA;

loc_001E7374: ;
    eax = MEM32(esi + 0x38);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(3) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 3 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_L(_fas, _fbs)) goto loc_001E7380; /* jl: less (signed <) */

loc_001E737C: ;
    _cf = 0; /* xor clears CF */
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    goto loc_001E73A0;

loc_001E7380: ;
    edx = esp + 0x30;
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, edi);
    ecx = eax + eax * 2;
    PUSH32(esp, edx);
    { uint32_t _icall_target = MEM32(ecx * 8 + 0x452A08); PUSH32(esp, 0x001E7390u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x001E7389u); } /* indirect call */
    }

loc_001E7390: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E739D; /* je: equal / zero */

loc_001E7397: ;
    eax = MEM32(esp + 0x30);
    goto loc_001E73A0;

loc_001E739D: ;
    _cf = 0; /* logical op clears CF */
    eax = eax | 0xFFFFFFFFu;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */

loc_001E73A0: ;
    MEM32(esp + 0x34) = eax;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E73A9u); RECOMP_ABI_CALL(0x0021A960u, sub_0021A960); /* call 0x0021A960 */

loc_001E73A9: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E748E; /* je: equal / zero */

loc_001E73B1: ;
    xmm0 = XMM_SCALAR(MEMF(eax + 8)); /* movss */
    xmm0.f[0] = xmm0.f[0] - MEMF(eax); /* subss */
    eax = ZX16(MEM16(eax + 0x14));
    xmm1.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * xmm1.f[0]; /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x43B494); /* mulss */
    ecx = (int32_t)recomp_fist((double)xmm0.f[0], 0x0E7Fu, 32); /* cvttss2si */
    MEM32(esp + 0xC) = ecx;
    fp_push((double)SMEM32(esp + 0xC)); /* fild */

loc_001E73DA: ;
    ebx = ZX16(MEM16(esp + 0x20));
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x1C)); /* fmul dword ptr [esp + 0x1c] */
    ebx = esp + ebx * 2 + 0xC8;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E73EFu); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E73EF: ;
    _fb = (uint32_t)(LO16(eax)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM16(ebx)) + (uint64_t)(LO16(eax))) >> 16) & 1);
    MEM16(ebx) = MEM16(ebx) + LO16(eax);
    _fa = (uint32_t)(MEM16(ebx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    SET_LO16(edx, MEM16(esp + 0x2C));
    _fb = (uint32_t)(LO16(edx)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM16(ebx)) + (uint64_t)(LO16(edx))) >> 16) & 1);
    MEM16(ebx) = MEM16(ebx) + LO16(edx);
    _fa = (uint32_t)(MEM16(ebx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    _fa = (uint32_t)(MEM16(esp + 0x6C)) & 0xFFFFu; _fb = (uint32_t)(0) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(esp + 0x6C), 0 (16-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_BE(_fa, _fb)) goto loc_001E744C; /* jbe: below or equal (unsigned <=) */

loc_001E7402: ;
    eax = MEM32(esi + 0x38);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(3) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 3 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_GE(_fas, _fbs)) goto loc_001E7433; /* jge: greater or equal (signed >=) */

loc_001E740A: ;
    eax = eax + eax * 2;
    eax = MEM32(eax * 8 + 0x452A0C);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E7433; /* je: equal / zero */

loc_001E7418: ;
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, edi);
    { uint32_t _icall_target = eax; PUSH32(esp, 0x001E741Bu); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x001E7419u); } /* indirect call */
    }

loc_001E741B: ;
    ecx = SX16(LO16(eax));
    xmm0.f[0] = (float)(int32_t)ecx; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x43B494); /* mulss */
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = (int32_t)recomp_fist((double)xmm0.f[0], 0x0E7Fu, 32); /* cvttss2si */
    goto loc_001E7435;

loc_001E7433: ;
    _cf = 0; /* xor clears CF */
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */

loc_001E7435: ;
    edx = SX16(LO16(eax));
    MEM32(esp + 0xC) = edx;
    fp_push((double)SMEM32(esp + 0xC)); /* fild */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x1C)); /* fmul dword ptr [esp + 0x1c] */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7449u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E7449: ;
    _fb = (uint32_t)(LO16(eax)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM16(ebx)) + (uint64_t)(LO16(eax))) >> 16) & 1);
    MEM16(ebx) = MEM16(ebx) + LO16(eax);
    _fa = (uint32_t)(MEM16(ebx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */

loc_001E744C: ;
    eax = MEM32(esp + 0x8C);
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(1) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 1 (16-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_BE(_fa, _fb)) goto loc_001E748E; /* jbe: below or equal (unsigned <=) */

loc_001E7459: ;
    eax = ZX16(LO16(eax));
    edi = edi + eax + -1;
    goto loc_001E748E;

loc_001E7462: ;
    ecx = ZX16(MEM16(esp + 0x88));
    MEM32(esp + 0xC) = ecx;
    fp_push((double)SMEM32(esp + 0xC)); /* fild */

loc_001E7472: ;
    ebx = ZX16(MEM16(esp + 0x20));
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x1C)); /* fmul dword ptr [esp + 0x1c] */
    ebx = esp + ebx * 2 + 0xC8;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7487u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E7487: ;
    _fb = (uint32_t)(MEM32(esp + 0x2C)) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(MEM32(esp + 0x2C))) >> 32) & 1);
    eax = eax + MEM32(esp + 0x2C);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_001E748B: ;
    _fb = (uint32_t)(LO16(eax)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM16(ebx)) + (uint64_t)(LO16(eax))) >> 16) & 1);
    MEM16(ebx) = MEM16(ebx) + LO16(eax);
    _fa = (uint32_t)(MEM16(ebx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */

loc_001E748E: ;
    ecx = MEM32(esp + 0x6C);
    SET_LO8(eax, MEM8(edi + 1));
    edi++;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    MEM32(esp + 0x6C) = ecx;
    if (CMP_NE(_fa, _fb)) goto loc_001E7020; /* jne: not equal / not zero */

loc_001E74A3: ;
    eax = MEM32(esp + 0x20);

loc_001E74A7: ;
    ebx = MEM32(esp + 0x28);

loc_001E74AB: ;
    SET_LO16(edx, (uint32_t)(int32_t)SMEM8(esi + 0x4C));
    edi = MEM32(esp + 0x64);
    ecx = MEM32(esp + 0x18);
    eax = ZX16(LO16(eax));
    if (1) _cf = (int)(((eax) >> (32 - (1))) & 1);
    eax = eax << 1;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(edi) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(edx) + (uint64_t)(edi)) >> 32) & 1);
    edx = edx + edi;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edx = (uint32_t)((int32_t)edx * (int32_t)MEM32(esp + 0x24));
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(ecx) < (uint32_t)(edx));
    ecx = ecx - edx;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    MEM32(esp + 0x18) = ecx;
    _cf = 0; /* xor clears CF */
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO16(ecx, MEM16(esp + eax + 0xC8));
    _fa = (uint32_t)(LO16(ecx)) & 0xFFFFu; _fb = (uint32_t)(0) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(ecx), 0 (16-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_BE(_fa, _fb)) goto loc_001E7527; /* jbe: below or equal (unsigned <=) */

loc_001E74D9: ;
    _fb = (uint32_t)(MEM32(esp + 0x2C)) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(ecx) < (uint32_t)(MEM32(esp + 0x2C)));
    ecx = ecx - MEM32(esp + 0x2C);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _cf = 0; /* xor clears CF */
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(edx, MEM8(esi + 0x4A));
    edi = esp + eax + 0x2C8;
    MEM16(esp + eax + 0xC8) = LO16(ecx);
    SET_LO16(eax, MEM16(esi + 0x44));
    MEM16(edi) = LO16(eax);
    _cf = 0; /* logical op clears CF */
    edx = edx & 0xF;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    edx--;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa == 0)) goto loc_001E751F; /* je: equal / zero */

loc_001E74FE: ;
    edx--;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa != 0)) goto loc_001E7527; /* jne: not equal / not zero */

loc_001E7501: ;
    goto loc_001E7522;

loc_001E7503: ;
    _cf = 0; /* logical op clears CF */
    eax = eax | 0xFFFFFFFFu;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    ecx = MEM32(esp + 0x14CC);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7512u); RECOMP_ABI_CALL(0x00379C29u, sub_00379C29); /* call 0x00379C29 */

loc_001E7512: ;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebx);
    esp = ebp;
    POP32(esp, ebp);
    esp += 4; return; /* ret */

loc_001E7519: ;
    eax = MEM32(esp + 0x20);
    goto loc_001E74AB;

loc_001E751F: ;
    if (1) _cf = (int)(((LO16(ecx)) >> ((1) - 1)) & 1);
    SET_LO16(ecx, LO16(ecx) >> 1);
    _fa = (uint32_t)(LO16(ecx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* shift result */

loc_001E7522: ;
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(eax) < (uint32_t)(ecx));
    eax = eax - ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    MEM16(edi) = LO16(eax);

loc_001E7527: ;
    eax = MEM32(esp + 0x2C8);
    SET_LO16(ecx, MEM16(esi + 0x46));
    MEM32(esp + 0x10) = eax;
    eax = ZX8(MEM8(esi + 0x4A));
    if (4) _cf = (int)(((eax) >> ((4) - 1)) & 1);
    eax = eax >> 4;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    MEM16(esp + 0x24) = LO16(ecx);
    if ((_fa == 0)) goto loc_001E7558; /* je: equal / zero */

loc_001E7545: ;
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa != 0)) goto loc_001E7569; /* jne: not equal / not zero */

loc_001E7548: ;
    eax = MEM32(esp + 0x24);
    edx = MEM32(esp + 0x18);
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(eax) < (uint32_t)(edx));
    eax = eax - edx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    MEM32(esp + 0x24) = eax;
    goto loc_001E7569;

loc_001E7558: ;
    eax = MEM32(esp + 0x18);
    ecx = MEM32(esp + 0x24);
    if (1) _cf = (int)(((LO16(eax)) >> ((1) - 1)) & 1);
    SET_LO16(eax, LO16(eax) >> 1);
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* shift result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(ecx) < (uint32_t)(eax));
    ecx = ecx - eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    MEM32(esp + 0x24) = ecx;

loc_001E7569: ;
    SET_LO8(eax, MEM8(ebx));
    xmm0 = XMM_SCALAR(MEMF(esi + 0x1C)); /* movss */
    _cf = 0; /* xor clears CF */
    edi = 0; /* xor self */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    MEM32(esp + 0x20) = edi;
    MEM32(esp + 0x6C) = edi;
    MEMF(esp + 0x1C) = xmm0.f[0]; /* movss */
    MEM32(esp + 0x18) = ebx;
    if (CMP_EQ(_fa, _fb)) goto loc_001E843A; /* je: equal / zero */

loc_001E758C: ;
    /* nop */

loc_001E7590: ;
    edi = MEM32(esp + 0x18);
    SET_LO8(eax, MEM8(edi));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x40) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x40 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_NE(_fa, _fb)) goto loc_001E8104; /* jne: not equal / not zero */

loc_001E759E: ;
    eax = (uint32_t)(int32_t)SMEM8(edi + 1);
    edi++;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fb = (uint32_t)(0xFFFFFFD5u) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(0xFFFFFFD5u)) >> 32) & 1);
    eax = eax + 0xFFFFFFD5u;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x4C) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x4C (32-bit) */
    _cf = (int)(_fa < _fb);
    MEM32(esp + 0x18) = edi;
    if (CMP_A(_fa, _fb)) goto loc_001E84AE; /* ja: above (unsigned >) */

loc_001E75B3: ;
    eax = ZX8(MEM8(eax + 0x1E85E0));
    { uint32_t _jt = MEM32(eax * 4 + 0x1E85BC); /* switch: 9 entries, 9 targets */
    if (_jt == 0x001E75C1u) goto loc_001E75C1;
    if (_jt == 0x001E75E9u) goto loc_001E75E9;
    if (_jt == 0x001E7617u) goto loc_001E7617;
    if (_jt == 0x001E7821u) goto loc_001E7821;
    if (_jt == 0x001E7E43u) goto loc_001E7E43;
    if (_jt == 0x001E7EB5u) goto loc_001E7EB5;
    if (_jt == 0x001E7F1Bu) goto loc_001E7F1B;
    if (_jt == 0x001E80A5u) goto loc_001E80A5;
    if (_jt == 0x001E84AEu) goto loc_001E84AE;
    g_seh_ebp = ebp; RECOMP_ITAIL(_jt); return; }

loc_001E75C1: ;
    eax = MEM32(esp + 0x18);
    ecx = (uint32_t)(int32_t)SMEM8(eax + 1);
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fb = (uint32_t)(0x30) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(ecx) < (uint32_t)(0x30));
    ecx = ecx - 0x30;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    MEM32(esp + 0x30) = ecx;
    MEM32(esp + 0x18) = eax;
    fp_push((double)SMEM32(esp + 0x30)); /* fild */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x43B494)); /* fmul dword ptr [0x43b494] */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E75E4u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E75E4: ;
    goto loc_001E84A4;

loc_001E75E9: ;
    eax = MEM32(esp + 0x18);
    edx = (uint32_t)(int32_t)SMEM8(eax + 1);
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fb = (uint32_t)(0x30) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(edx) < (uint32_t)(0x30));
    edx = edx - 0x30;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    MEM32(esp + 0x30) = edx;
    MEM32(esp + 0x18) = eax;
    fp_push((double)SMEM32(esp + 0x30)); /* fild */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x43B494)); /* fmul dword ptr [0x43b494] */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E760Cu); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E760C: ;
    ecx = MEM32(esp + 0x10);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(ecx) < (uint32_t)(eax));
    ecx = ecx - eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    goto loc_001E84AA;

loc_001E7617: ;
    eax = MEM32(esp + 0x18);
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(esp + 0x18) = eax;
    SET_LO8(eax, MEM8(eax));
    edx = SX8(LO8(eax));
    ecx = edx + -43;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x4E) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x4E (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_A(_fa, _fb)) goto loc_001E76C3; /* ja: above (unsigned >) */

loc_001E7631: ;
    ecx = ZX8(MEM8(ecx + 0x1E8650));
    { uint32_t _jt = MEM32(ecx * 4 + 0x1E8630); /* switch: 8 entries, 8 targets */
    if (_jt == 0x001E763Fu) goto loc_001E763F;
    if (_jt == 0x001E7654u) goto loc_001E7654;
    if (_jt == 0x001E7669u) goto loc_001E7669;
    if (_jt == 0x001E767Bu) goto loc_001E767B;
    if (_jt == 0x001E768Du) goto loc_001E768D;
    if (_jt == 0x001E769Fu) goto loc_001E769F;
    if (_jt == 0x001E76B1u) goto loc_001E76B1;
    if (_jt == 0x001E76C3u) goto loc_001E76C3;
    g_seh_ebp = ebp; RECOMP_ITAIL(_jt); return; }

loc_001E763F: ;
    MEM32(esp + 0x34) = 8;
    MEM32(esp + 0xC) = 1;
    goto loc_001E76EC;

loc_001E7654: ;
    MEM32(esp + 0x34) = 7;
    MEM32(esp + 0xC) = 1;
    goto loc_001E76EC;

loc_001E7669: ;
    MEM32(esp + 0x34) = 5;
    MEM32(esp + 0xC) = 1;
    goto loc_001E76EC;

loc_001E767B: ;
    MEM32(esp + 0x34) = 6;
    MEM32(esp + 0xC) = 1;
    goto loc_001E76EC;

loc_001E768D: ;
    MEM32(esp + 0x34) = 9;
    MEM32(esp + 0xC) = 1;
    goto loc_001E76EC;

loc_001E769F: ;
    MEM32(esp + 0x34) = 0xA;
    MEM32(esp + 0xC) = 1;
    goto loc_001E76EC;

loc_001E76B1: ;
    MEM32(esp + 0x34) = 0xD;
    MEM32(esp + 0xC) = 1;
    goto loc_001E76EC;

loc_001E76C3: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x31) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x31 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_L(_fas, _fbs)) goto loc_001E76E0; /* jl: less (signed <) */

loc_001E76C7: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x39) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x39 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_001E76E0; /* jg: greater (signed >) */

loc_001E76CB: ;
    edx = MEM32(edx * 4 + 0x436280);
    MEM32(esp + 0x34) = edx;
    MEM32(esp + 0xC) = 1;
    goto loc_001E76EC;

loc_001E76E0: ;
    eax = MEM32(esp + 0xC);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E84AE; /* je: equal / zero */

loc_001E76EC: ;
    eax = MEM32(esp + 0x90);
    SET_LO16(ecx, MEM16(esp + 0x10));
    xmm1 = XMM_ZERO(); /* xorps self = zero */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x84)); /* movss */
    xmm3 = XMM_SCALAR(MEMF(esi + 0x1C)); /* movss */
    SET_LO16(edx, MEM16(esp + 0x24));
    xmm4 = XMM_SCALAR(MEMF(esi + 0x14)); /* movss */
    MEM16(esp + 0x38) = LO16(ecx);
    ecx = MEM32(esp + 0x68);
    MEM32(esi + 8) = eax;
    SET_LO16(eax, MEM16(esi + 0x48));
    MEMF(esp + 0x58) = xmm1.f[0]; /* movss */
    xmm1 = XMM_SCALAR(MEMF(esi + 0x18)); /* movss */
    MEM32(esp + 0x5C) = ecx;
    ecx = MEM32(esp + 0x14);
    MEM16(esp + 0x3C) = LO16(eax);
    eax = MEM32(esp + 0x60);
    MEMF(esp + 0x50) = xmm1.f[0]; /* movss */
    xmm1 = XMM_SCALAR(MEMF(esi + 0x20)); /* movss */
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    xmm3.f[0] = xmm3.f[0] * xmm0.f[0]; /* mulss */
    xmm1.f[0] = xmm1.f[0] * xmm0.f[0]; /* mulss */
    MEMF(esp + 0x1C) = xmm3.f[0]; /* movss */
    MEM16(esp + 0x3A) = LO16(edx);
    MEM8(esp + 0x3F) = 1;
    MEM8(esp + 0x40) = 1;
    MEM32(esp + 0x44) = 0;
    MEMF(esp + 0x54) = xmm4.f[0]; /* movss */
    MEMF(esp + 0x48) = xmm3.f[0]; /* movss */
    MEMF(esp + 0x4C) = xmm1.f[0]; /* movss */
    MEM8(esp + 0x3E) = 0;
    MEM32(esp + 0x14) = ecx;
    if (CMP_EQ(_fa, _fb)) goto loc_001E77AD; /* je: equal / zero */

loc_001E778F: ;
    edx = ZX16(LO16(ecx));
    edi = esp + 0x34;
    eax = esi;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E779Du); RECOMP_ABI_CALL(0x001E6C30u, sub_001E6C30); /* call 0x001E6C30 */

loc_001E779D: ;
    xmm4 = XMM_SCALAR(MEMF(esp + 0x54)); /* movss */
    xmm3 = XMM_SCALAR(MEMF(esp + 0x1C)); /* movss */
    MEM32(esp + 0xC) = eax;

loc_001E77AD: ;
    eax = MEM32(esp + 0xC);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E77FC; /* je: equal / zero */

loc_001E77B5: ;
    eax = MEM32(esi + 0x30);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E77E4; /* je: equal / zero */

loc_001E77BC: ;
    eax = MEM32(ebp + 8);
    SET_LO16(edx, MEM16(esi + 0x50));
    SET_LO16(ecx, MEM16(eax + 0x40));
    SET_LO16(esi, MEM16(esi + 0x42));
    eax = MEM32(esp + 0x3A);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E77D4u); RECOMP_ABI_CALL(0x001EA030u, sub_001EA030); /* call 0x001EA030 */

loc_001E77D4: ;
    esi = MEM32(ebp + 8);
    xmm0.f[0] = xmm0.f[0] * xmm4.f[0]; /* mulss */
    xmm4 = xmm0; /* movaps */
    MEMF(esp + 0x54) = xmm4.f[0]; /* movss */

loc_001E77E4: ;
    _fca = xmm4.f[0]; _fcb = MEMF(0x4149C4); /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_001E77FC; /* jbe: below or equal (unsigned <=) (xmm4.f[0] vs MEMF(0x4149C4)) */

loc_001E77ED: ;
    edi = esp + 0x34;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E77F6u); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_001E77F6: ;
    xmm3 = XMM_SCALAR(MEMF(esp + 0x1C)); /* movss */

loc_001E77FC: ;
    xmm3.f[0] = xmm3.f[0] * MEMF(0x414EF0); /* mulss */
    MEMF(esp + 0x98) = xmm3.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x98), 0x0E7Fu, 32); /* cvttss2si */
    ecx = MEM32(esp + 0x2C);
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(ecx)) >> 32) & 1);
    eax = eax + ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_001E84A4;

loc_001E7821: ;
    eax = MEM32(esp + 0x18);
    SET_LO8(ebx, MEM8(eax + 1));
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(esp + 0x18) = eax;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7832u); RECOMP_ABI_CALL(0x001EB830u, sub_001EB830); /* call 0x001EB830 */

loc_001E7832: ;
    SET_LO8(ecx, LO8(ebx));
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7839u); RECOMP_ABI_CALL(0x001EBA00u, sub_001EBA00); /* call 0x001EBA00 */

loc_001E7839: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(7) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 7 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_B(_fa, _fb)) goto loc_001E7B21; /* jb: below (unsigned <) */

loc_001E7841: ;
    _cf = 0; /* logical op clears CF */
    eax = eax | 0xFFFFFFFFu;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */

loc_001E7844: ;
    ecx = MEM32(esp + 0xC);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E84AE; /* je: equal / zero */

loc_001E7850: ;
    ecx = MEM32(esp + 0x90);
    xmm1 = XMM_SCALAR(MEMF(esp + 0x84)); /* movss */
    SET_LO16(edx, MEM16(esp + 0x10));
    xmm3 = XMM_SCALAR(MEMF(esi + 0x1C)); /* movss */
    xmm0 = XMM_SCALAR(MEMF(esi + 0x20)); /* movss */
    MEM32(esi + 8) = ecx;
    SET_LO16(ecx, MEM16(esi + 0x48));
    xmm3.f[0] = xmm3.f[0] * xmm1.f[0]; /* mulss */
    xmm0.f[0] = xmm0.f[0] * xmm1.f[0]; /* mulss */
    xmm1 = XMM_ZERO(); /* xorps self = zero */
    MEM16(esp + 0x3C) = LO16(ecx);
    ecx = MEM32(esp + 0x14);
    MEM16(esp + 0x38) = LO16(edx);
    edx = MEM32(esp + 0x68);
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFFFFFFu) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0xFFFFFFFFu (32-bit) */
    _cf = (int)(_fa < _fb);
    MEMF(esp + 0x58) = xmm1.f[0]; /* movss */
    xmm1 = XMM_SCALAR(MEMF(esi + 0x18)); /* movss */
    MEMF(esp + 0x1C) = xmm3.f[0]; /* movss */
    MEM8(esp + 0x3E) = 0;
    MEMF(esp + 0x4C) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x48) = xmm3.f[0]; /* movss */
    MEM32(esp + 0x44) = 0;
    MEM8(esp + 0x40) = 1;
    MEM8(esp + 0x3F) = 1;
    MEMF(esp + 0x50) = xmm1.f[0]; /* movss */
    MEM32(esp + 0x5C) = edx;
    MEM32(esp + 0x14) = ecx;
    if (CMP_NE(_fa, _fb)) goto loc_001E7B41; /* jne: not equal / not zero */

loc_001E78DF: ;
    SET_LO16(eax, MEM16(esp + 0x24));
    xmm1 = XMM_SCALAR(MEMF(esi + 0x14)); /* movss */
    MEM16(esp + 0x3A) = LO16(eax);
    eax = MEM32(esp + 0x60);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    MEM32(esp + 0x34) = 5;
    MEMF(esp + 0x54) = xmm1.f[0]; /* movss */
    if (CMP_EQ(_fa, _fb)) goto loc_001E791C; /* je: equal / zero */

loc_001E7904: ;
    edx = ZX16(LO16(ecx));
    edi = esp + 0x34;
    eax = esi;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7912u); RECOMP_ABI_CALL(0x001E6C30u, sub_001E6C30); /* call 0x001E6C30 */

loc_001E7912: ;
    xmm3 = XMM_SCALAR(MEMF(esp + 0x1C)); /* movss */
    MEM32(esp + 0xC) = eax;

loc_001E791C: ;
    eax = MEM32(esp + 0xC);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E7970; /* je: equal / zero */

loc_001E7924: ;
    eax = MEM32(esi + 0x30);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E7952; /* je: equal / zero */

loc_001E792B: ;
    eax = MEM32(ebp + 8);
    SET_LO16(edx, MEM16(esi + 0x50));
    SET_LO16(ecx, MEM16(eax + 0x40));
    SET_LO16(esi, MEM16(esi + 0x42));
    eax = MEM32(esp + 0x3A);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7943u); RECOMP_ABI_CALL(0x001EA030u, sub_001EA030); /* call 0x001EA030 */

loc_001E7943: ;
    xmm0.f[0] = xmm0.f[0] * MEMF(esp + 0x54); /* mulss */
    esi = MEM32(ebp + 8);
    MEMF(esp + 0x54) = xmm0.f[0]; /* movss */

loc_001E7952: ;
    xmm0 = XMM_SCALAR(MEMF(esp + 0x54)); /* movss */
    _fca = xmm0.f[0]; _fcb = MEMF(0x4149C4); /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_001E7970; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs MEMF(0x4149C4)) */

loc_001E7961: ;
    edi = esp + 0x34;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E796Au); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_001E796A: ;
    xmm3 = XMM_SCALAR(MEMF(esp + 0x1C)); /* movss */

loc_001E7970: ;
    xmm3.f[0] = xmm3.f[0] * MEMF(0x414EF0); /* mulss */
    MEMF(esp + 0x28) = xmm3.f[0]; /* movss */
    MEMF(esp + 0xA4) = xmm3.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0xA4), 0x0E7Fu, 32); /* cvttss2si */
    ecx = MEM32(esp + 0x2C);
    edx = MEM32(esp + 0x10);
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(ecx)) >> 32) & 1);
    eax = eax + ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(edx) + (uint64_t)(eax)) >> 32) & 1);
    edx = edx + eax;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = MEM32(esp + 0x60);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    MEM32(esp + 0x10) = edx;
    if (CMP_EQ(_fa, _fb)) goto loc_001E79B6; /* je: equal / zero */

loc_001E79A8: ;
    edx = ZX16(MEM16(esp + 0x14));
    _fa = (uint32_t)(MEM32(esi + 0xC)) & 0xFFFFFFFFu; _fb = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(esi + 0xC), edx (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_B(_fa, _fb)) goto loc_001E84AE; /* jb: below (unsigned <) */

loc_001E79B6: ;
    edx = MEM32(esp + 0x14);
    SET_LO16(ecx, MEM16(esp + 0x10));
    edx++;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    MEM16(esp + 0x38) = LO16(ecx);
    MEM32(esp + 0x34) = 0x1B;
    MEM32(esp + 0x14) = edx;
    if (CMP_EQ(_fa, _fb)) goto loc_001E79ED; /* je: equal / zero */

loc_001E79D5: ;
    edx = ZX16(LO16(edx));
    edi = esp + 0x34;
    eax = esi;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E79E3u); RECOMP_ABI_CALL(0x001E6C30u, sub_001E6C30); /* call 0x001E6C30 */

loc_001E79E3: ;
    xmm3 = XMM_SCALAR(MEMF(esp + 0x28)); /* movss */
    MEM32(esp + 0xC) = eax;

loc_001E79ED: ;
    eax = MEM32(esp + 0xC);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E7A41; /* je: equal / zero */

loc_001E79F5: ;
    eax = MEM32(esi + 0x30);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E7A23; /* je: equal / zero */

loc_001E79FC: ;
    eax = MEM32(ebp + 8);
    SET_LO16(edx, MEM16(esi + 0x50));
    SET_LO16(ecx, MEM16(eax + 0x40));
    SET_LO16(esi, MEM16(esi + 0x42));
    eax = MEM32(esp + 0x3A);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7A14u); RECOMP_ABI_CALL(0x001EA030u, sub_001EA030); /* call 0x001EA030 */

loc_001E7A14: ;
    xmm0.f[0] = xmm0.f[0] * MEMF(esp + 0x54); /* mulss */
    esi = MEM32(ebp + 8);
    MEMF(esp + 0x54) = xmm0.f[0]; /* movss */

loc_001E7A23: ;
    xmm0 = XMM_SCALAR(MEMF(esp + 0x54)); /* movss */
    _fca = xmm0.f[0]; _fcb = MEMF(0x4149C4); /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_001E7A41; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs MEMF(0x4149C4)) */

loc_001E7A32: ;
    edi = esp + 0x34;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7A3Bu); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_001E7A3B: ;
    xmm3 = XMM_SCALAR(MEMF(esp + 0x28)); /* movss */

loc_001E7A41: ;
    MEMF(esp + 0xA8) = xmm3.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0xA8), 0x0E7Fu, 32); /* cvttss2si */
    ecx = MEM32(esp + 0x2C);
    edx = MEM32(esp + 0x10);
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(ecx)) >> 32) & 1);
    eax = eax + ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(edx) + (uint64_t)(eax)) >> 32) & 1);
    edx = edx + eax;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = MEM32(esp + 0x60);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    MEM32(esp + 0x10) = edx;
    if (CMP_EQ(_fa, _fb)) goto loc_001E7A79; /* je: equal / zero */

loc_001E7A6B: ;
    edx = ZX16(MEM16(esp + 0x14));
    _fa = (uint32_t)(MEM32(esi + 0xC)) & 0xFFFFFFFFu; _fb = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(esi + 0xC), edx (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_B(_fa, _fb)) goto loc_001E84AE; /* jb: below (unsigned <) */

loc_001E7A79: ;
    edx = MEM32(esp + 0x14);
    SET_LO16(ecx, MEM16(esp + 0x10));
    edx++;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    MEM16(esp + 0x38) = LO16(ecx);
    MEM32(esp + 0x34) = 8;
    MEM32(esp + 0x14) = edx;
    if (CMP_EQ(_fa, _fb)) goto loc_001E7AB0; /* je: equal / zero */

loc_001E7A98: ;
    edx = ZX16(LO16(edx));
    edi = esp + 0x34;
    eax = esi;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7AA6u); RECOMP_ABI_CALL(0x001E6C30u, sub_001E6C30); /* call 0x001E6C30 */

loc_001E7AA6: ;
    xmm3 = XMM_SCALAR(MEMF(esp + 0x28)); /* movss */
    MEM32(esp + 0xC) = eax;

loc_001E7AB0: ;
    eax = MEM32(esp + 0xC);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E7B04; /* je: equal / zero */

loc_001E7AB8: ;
    eax = MEM32(esi + 0x30);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E7AE6; /* je: equal / zero */

loc_001E7ABF: ;
    eax = MEM32(ebp + 8);
    SET_LO16(edx, MEM16(esi + 0x50));
    SET_LO16(ecx, MEM16(eax + 0x40));
    SET_LO16(esi, MEM16(esi + 0x42));
    eax = MEM32(esp + 0x3A);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7AD7u); RECOMP_ABI_CALL(0x001EA030u, sub_001EA030); /* call 0x001EA030 */

loc_001E7AD7: ;
    xmm0.f[0] = xmm0.f[0] * MEMF(esp + 0x54); /* mulss */
    esi = MEM32(ebp + 8);
    MEMF(esp + 0x54) = xmm0.f[0]; /* movss */

loc_001E7AE6: ;
    xmm0 = XMM_SCALAR(MEMF(esp + 0x54)); /* movss */
    _fca = xmm0.f[0]; _fcb = MEMF(0x4149C4); /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_001E7B04; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs MEMF(0x4149C4)) */

loc_001E7AF5: ;
    edi = esp + 0x34;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7AFEu); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_001E7AFE: ;
    xmm3 = XMM_SCALAR(MEMF(esp + 0x28)); /* movss */

loc_001E7B04: ;
    MEMF(esp + 0x9C) = xmm3.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x9C), 0x0E7Fu, 32); /* cvttss2si */
    ecx = MEM32(esp + 0x2C);
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(ecx)) >> 32) & 1);
    eax = eax + ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_001E84A4;

loc_001E7B21: ;
    edx = ZX8(LO8(eax));
    eax = MEM32(edx * 4 + 0x43882C);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFFFFFFu) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0xFFFFFFFFu (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_EQ(_fa, _fb)) goto loc_001E7844; /* je: equal / zero */

loc_001E7B34: ;
    MEM32(esp + 0xC) = 1;
    goto loc_001E7850;

loc_001E7B41: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFFFFFEu) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0xFFFFFFFEu (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_NE(_fa, _fb)) goto loc_001E7D8E; /* jne: not equal / not zero */

loc_001E7B4A: ;
    SET_LO16(eax, MEM16(esp + 0x24));
    xmm1 = XMM_SCALAR(MEMF(esi + 0x14)); /* movss */
    MEM16(esp + 0x3A) = LO16(eax);
    eax = MEM32(esp + 0x60);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    MEM32(esp + 0x34) = 6;
    MEMF(esp + 0x54) = xmm1.f[0]; /* movss */
    if (CMP_EQ(_fa, _fb)) goto loc_001E7B89; /* je: equal / zero */

loc_001E7B6F: ;
    edx = ZX16(MEM16(esp + 0x14));
    edi = esp + 0x34;
    eax = esi;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7B7Fu); RECOMP_ABI_CALL(0x001E6C30u, sub_001E6C30); /* call 0x001E6C30 */

loc_001E7B7F: ;
    xmm3 = XMM_SCALAR(MEMF(esp + 0x1C)); /* movss */
    MEM32(esp + 0xC) = eax;

loc_001E7B89: ;
    eax = MEM32(esp + 0xC);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E7BDD; /* je: equal / zero */

loc_001E7B91: ;
    eax = MEM32(esi + 0x30);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E7BBF; /* je: equal / zero */

loc_001E7B98: ;
    eax = MEM32(ebp + 8);
    SET_LO16(edx, MEM16(esi + 0x50));
    SET_LO16(ecx, MEM16(eax + 0x40));
    SET_LO16(esi, MEM16(esi + 0x42));
    eax = MEM32(esp + 0x3A);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7BB0u); RECOMP_ABI_CALL(0x001EA030u, sub_001EA030); /* call 0x001EA030 */

loc_001E7BB0: ;
    xmm0.f[0] = xmm0.f[0] * MEMF(esp + 0x54); /* mulss */
    esi = MEM32(ebp + 8);
    MEMF(esp + 0x54) = xmm0.f[0]; /* movss */

loc_001E7BBF: ;
    xmm0 = XMM_SCALAR(MEMF(esp + 0x54)); /* movss */
    _fca = xmm0.f[0]; _fcb = MEMF(0x4149C4); /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_001E7BDD; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs MEMF(0x4149C4)) */

loc_001E7BCE: ;
    edi = esp + 0x34;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7BD7u); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_001E7BD7: ;
    xmm3 = XMM_SCALAR(MEMF(esp + 0x1C)); /* movss */

loc_001E7BDD: ;
    xmm3.f[0] = xmm3.f[0] * MEMF(0x414EF0); /* mulss */
    MEMF(esp + 0x28) = xmm3.f[0]; /* movss */
    MEMF(esp + 0xB8) = xmm3.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0xB8), 0x0E7Fu, 32); /* cvttss2si */
    ecx = MEM32(esp + 0x2C);
    edx = MEM32(esp + 0x10);
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(ecx)) >> 32) & 1);
    eax = eax + ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(edx) + (uint64_t)(eax)) >> 32) & 1);
    edx = edx + eax;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = MEM32(esp + 0x60);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    MEM32(esp + 0x10) = edx;
    if (CMP_EQ(_fa, _fb)) goto loc_001E7C23; /* je: equal / zero */

loc_001E7C15: ;
    edx = ZX16(MEM16(esp + 0x14));
    _fa = (uint32_t)(MEM32(esi + 0xC)) & 0xFFFFFFFFu; _fb = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(esi + 0xC), edx (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_B(_fa, _fb)) goto loc_001E84AE; /* jb: below (unsigned <) */

loc_001E7C23: ;
    edx = MEM32(esp + 0x14);
    SET_LO16(ecx, MEM16(esp + 0x10));
    edx++;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    MEM16(esp + 0x38) = LO16(ecx);
    MEM32(esp + 0x34) = 0x1B;
    MEM32(esp + 0x14) = edx;
    if (CMP_EQ(_fa, _fb)) goto loc_001E7C5A; /* je: equal / zero */

loc_001E7C42: ;
    edx = ZX16(LO16(edx));
    edi = esp + 0x34;
    eax = esi;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7C50u); RECOMP_ABI_CALL(0x001E6C30u, sub_001E6C30); /* call 0x001E6C30 */

loc_001E7C50: ;
    xmm3 = XMM_SCALAR(MEMF(esp + 0x28)); /* movss */
    MEM32(esp + 0xC) = eax;

loc_001E7C5A: ;
    eax = MEM32(esp + 0xC);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E7CAE; /* je: equal / zero */

loc_001E7C62: ;
    eax = MEM32(esi + 0x30);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E7C90; /* je: equal / zero */

loc_001E7C69: ;
    eax = MEM32(ebp + 8);
    SET_LO16(edx, MEM16(esi + 0x50));
    SET_LO16(ecx, MEM16(eax + 0x40));
    SET_LO16(esi, MEM16(esi + 0x42));
    eax = MEM32(esp + 0x3A);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7C81u); RECOMP_ABI_CALL(0x001EA030u, sub_001EA030); /* call 0x001EA030 */

loc_001E7C81: ;
    xmm0.f[0] = xmm0.f[0] * MEMF(esp + 0x54); /* mulss */
    esi = MEM32(ebp + 8);
    MEMF(esp + 0x54) = xmm0.f[0]; /* movss */

loc_001E7C90: ;
    xmm0 = XMM_SCALAR(MEMF(esp + 0x54)); /* movss */
    _fca = xmm0.f[0]; _fcb = MEMF(0x4149C4); /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_001E7CAE; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs MEMF(0x4149C4)) */

loc_001E7C9F: ;
    edi = esp + 0x34;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7CA8u); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_001E7CA8: ;
    xmm3 = XMM_SCALAR(MEMF(esp + 0x28)); /* movss */

loc_001E7CAE: ;
    MEMF(esp + 0xC4) = xmm3.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0xC4), 0x0E7Fu, 32); /* cvttss2si */
    ecx = MEM32(esp + 0x2C);
    edx = MEM32(esp + 0x10);
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(ecx)) >> 32) & 1);
    eax = eax + ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(edx) + (uint64_t)(eax)) >> 32) & 1);
    edx = edx + eax;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = MEM32(esp + 0x60);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    MEM32(esp + 0x10) = edx;
    if (CMP_EQ(_fa, _fb)) goto loc_001E7CE6; /* je: equal / zero */

loc_001E7CD8: ;
    edx = ZX16(MEM16(esp + 0x14));
    _fa = (uint32_t)(MEM32(esi + 0xC)) & 0xFFFFFFFFu; _fb = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(esi + 0xC), edx (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_B(_fa, _fb)) goto loc_001E84AE; /* jb: below (unsigned <) */

loc_001E7CE6: ;
    edx = MEM32(esp + 0x14);
    SET_LO16(ecx, MEM16(esp + 0x10));
    edx++;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    MEM16(esp + 0x38) = LO16(ecx);
    MEM32(esp + 0x34) = 7;
    MEM32(esp + 0x14) = edx;
    if (CMP_EQ(_fa, _fb)) goto loc_001E7D1D; /* je: equal / zero */

loc_001E7D05: ;
    edx = ZX16(LO16(edx));
    edi = esp + 0x34;
    eax = esi;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7D13u); RECOMP_ABI_CALL(0x001E6C30u, sub_001E6C30); /* call 0x001E6C30 */

loc_001E7D13: ;
    xmm3 = XMM_SCALAR(MEMF(esp + 0x28)); /* movss */
    MEM32(esp + 0xC) = eax;

loc_001E7D1D: ;
    eax = MEM32(esp + 0xC);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E7D71; /* je: equal / zero */

loc_001E7D25: ;
    eax = MEM32(esi + 0x30);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E7D53; /* je: equal / zero */

loc_001E7D2C: ;
    eax = MEM32(ebp + 8);
    SET_LO16(edx, MEM16(esi + 0x50));
    SET_LO16(ecx, MEM16(eax + 0x40));
    SET_LO16(esi, MEM16(esi + 0x42));
    eax = MEM32(esp + 0x3A);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7D44u); RECOMP_ABI_CALL(0x001EA030u, sub_001EA030); /* call 0x001EA030 */

loc_001E7D44: ;
    xmm0.f[0] = xmm0.f[0] * MEMF(esp + 0x54); /* mulss */
    esi = MEM32(ebp + 8);
    MEMF(esp + 0x54) = xmm0.f[0]; /* movss */

loc_001E7D53: ;
    xmm0 = XMM_SCALAR(MEMF(esp + 0x54)); /* movss */
    _fca = xmm0.f[0]; _fcb = MEMF(0x4149C4); /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_001E7D71; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs MEMF(0x4149C4)) */

loc_001E7D62: ;
    edi = esp + 0x34;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7D6Bu); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_001E7D6B: ;
    xmm3 = XMM_SCALAR(MEMF(esp + 0x28)); /* movss */

loc_001E7D71: ;
    MEMF(esp + 0xB4) = xmm3.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0xB4), 0x0E7Fu, 32); /* cvttss2si */
    ecx = MEM32(esp + 0x2C);
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(ecx)) >> 32) & 1);
    eax = eax + ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_001E84A4;

loc_001E7D8E: ;
    xmm4 = XMM_SCALAR(MEMF(esi + 0x14)); /* movss */
    MEM32(esp + 0x34) = eax;
    SET_LO16(eax, MEM16(esp + 0x24));
    MEM16(esp + 0x3A) = LO16(eax);
    eax = MEM32(esp + 0x60);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    MEMF(esp + 0x54) = xmm4.f[0]; /* movss */
    if (CMP_EQ(_fa, _fb)) goto loc_001E7DCF; /* je: equal / zero */

loc_001E7DAF: ;
    edx = ZX16(MEM16(esp + 0x14));
    edi = esp + 0x34;
    eax = esi;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7DBFu); RECOMP_ABI_CALL(0x001E6C30u, sub_001E6C30); /* call 0x001E6C30 */

loc_001E7DBF: ;
    xmm4 = XMM_SCALAR(MEMF(esp + 0x54)); /* movss */
    xmm3 = XMM_SCALAR(MEMF(esp + 0x1C)); /* movss */
    MEM32(esp + 0xC) = eax;

loc_001E7DCF: ;
    eax = MEM32(esp + 0xC);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E7E1E; /* je: equal / zero */

loc_001E7DD7: ;
    eax = MEM32(esi + 0x30);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E7E06; /* je: equal / zero */

loc_001E7DDE: ;
    eax = MEM32(ebp + 8);
    SET_LO16(edx, MEM16(esi + 0x50));
    SET_LO16(ecx, MEM16(eax + 0x40));
    SET_LO16(esi, MEM16(esi + 0x42));
    eax = MEM32(esp + 0x3A);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7DF6u); RECOMP_ABI_CALL(0x001EA030u, sub_001EA030); /* call 0x001EA030 */

loc_001E7DF6: ;
    esi = MEM32(ebp + 8);
    xmm0.f[0] = xmm0.f[0] * xmm4.f[0]; /* mulss */
    xmm4 = xmm0; /* movaps */
    MEMF(esp + 0x54) = xmm4.f[0]; /* movss */

loc_001E7E06: ;
    _fca = xmm4.f[0]; _fcb = MEMF(0x4149C4); /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_001E7E1E; /* jbe: below or equal (unsigned <=) (xmm4.f[0] vs MEMF(0x4149C4)) */

loc_001E7E0F: ;
    edi = esp + 0x34;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7E18u); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_001E7E18: ;
    xmm3 = XMM_SCALAR(MEMF(esp + 0x1C)); /* movss */

loc_001E7E1E: ;
    xmm3.f[0] = xmm3.f[0] * MEMF(0x414EF0); /* mulss */
    MEMF(esp + 0xB0) = xmm3.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0xB0), 0x0E7Fu, 32); /* cvttss2si */
    ecx = MEM32(esp + 0x2C);
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(ecx)) >> 32) & 1);
    eax = eax + ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_001E84A4;

loc_001E7E43: ;
    eax = MEM32(esp + 0x18);
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(esp + 0x18) = eax;
    SET_LO8(eax, MEM8(eax));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x30) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x30 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_L(_fas, _fbs)) goto loc_001E84AE; /* jl: less (signed <) */

loc_001E7E56: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x39) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x39 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_001E84AE; /* jg: greater (signed >) */

loc_001E7E5E: ;
    edi = SX8(LO8(eax));
    _fb = (uint32_t)(0x30) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(edi) < (uint32_t)(0x30));
    edi = edi - 0x30;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    if (4) _cf = (int)(((edi) >> (32 - (4))) & 1);
    edi = edi << 4;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(0x436290) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(edi) + (uint64_t)(0x436290)) >> 32) & 1);
    edi = edi + 0x436290;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    fp_push(MEMF(edi + 0xC)); /* fld float */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7E75u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E7E75: ;
    fp_push(MEMF(edi)); /* fld float */
    ebx = eax;
    if (8) _cf = (int)(((ebx) >> (32 - (8))) & 1);
    ebx = ebx << 8;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7E81u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E7E81: ;
    fp_push(MEMF(edi + 4)); /* fld float */
    _cf = 0; /* logical op clears CF */
    eax = eax & 0xFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _cf = 0; /* logical op clears CF */
    ebx = ebx | eax;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    if (8) _cf = (int)(((ebx) >> (32 - (8))) & 1);
    ebx = ebx << 8;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7E93u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E7E93: ;
    fp_push(MEMF(edi + 8)); /* fld float */
    _cf = 0; /* logical op clears CF */
    eax = eax & 0xFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _cf = 0; /* logical op clears CF */
    ebx = ebx | eax;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    if (8) _cf = (int)(((ebx) >> (32 - (8))) & 1);
    ebx = ebx << 8;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7EA5u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E7EA5: ;
    _cf = 0; /* logical op clears CF */
    eax = eax & 0xFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _cf = 0; /* logical op clears CF */
    ebx = ebx | eax;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(esp + 0x68) = ebx;
    goto loc_001E84AE;

loc_001E7EB5: ;
    eax = MEM32(esp + 0x18);
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(esp + 0x18) = eax;
    SET_LO8(eax, MEM8(eax));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x30) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x30 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_L(_fas, _fbs)) goto loc_001E84AE; /* jl: less (signed <) */

loc_001E7EC8: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x39) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x39 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_001E84AE; /* jg: greater (signed >) */

loc_001E7ED0: ;
    edx = SX8(LO8(eax));
    eax = MEM32(esp + 0x18);
    _fb = (uint32_t)(2) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(2)) >> 32) & 1);
    eax = eax + 2;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esp + 0x18) = eax;
    SET_LO8(eax, MEM8(eax));
    _fb = (uint32_t)(0x30) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(edx) < (uint32_t)(0x30));
    edx = edx - 0x30;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x30) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x30 (8-bit) */
    _cf = (int)(_fa < _fb);
    xmm1.f[0] = (float)(int32_t)edx; /* cvtsi2ss */
    if (CMP_L(_fas, _fbs)) goto loc_001E84AE; /* jl: less (signed <) */

loc_001E7EEF: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x39) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x39 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_001E84AE; /* jg: greater (signed >) */

loc_001E7EF7: ;
    eax = SX8(LO8(eax));
    _fb = (uint32_t)(0x30) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(eax) < (uint32_t)(0x30));
    eax = eax - 0x30;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    xmm0.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x41492C); /* mulss */
    xmm0.f[0] = xmm0.f[0] + xmm1.f[0]; /* addss */
    MEMF(esp + 0x84) = xmm0.f[0]; /* movss */
    goto loc_001E84AE;

loc_001E7F1B: ;
    eax = MEM32(esp + 0x18);
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(esp + 0x18) = eax;
    SET_LO8(eax, MEM8(eax));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x31) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x31 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_L(_fas, _fbs)) goto loc_001E84AE; /* jl: less (signed <) */

loc_001E7F2E: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x34) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x34 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_001E84AE; /* jg: greater (signed >) */

loc_001E7F36: ;
    edi = SX8(LO8(eax));
    _fb = (uint32_t)(0x5E) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(edi) + (uint64_t)(0x5E)) >> 32) & 1);
    edi = edi + 0x5E;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = edi;
    MEM32(esp + 0x34) = edi;
    MEM32(esp + 0xC) = 1;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7F4Fu); RECOMP_ABI_CALL(0x0021A960u, sub_0021A960); /* call 0x0021A960 */

loc_001E7F4F: ;
    ebx = eax;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E84AE; /* je: equal / zero */

loc_001E7F59: ;
    ecx = MEM32(esp + 0x90);
    eax = MEM32(esp + 0x24);
    xmm0 = XMM_SCALAR(MEMF(esi + 0x1C)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(esp + 0x84); /* mulss */
    _cf = 0; /* xor clears CF */
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO16(edx, MEM16(esi + 0x48));
    MEM32(esi + 8) = ecx;
    ecx = MEM32(esp + 0x10);
    MEMF(esp + 0x1C) = xmm0.f[0]; /* movss */
    PUSH32(esp, edx);
    PUSH32(esp, eax);
    PUSH32(esp, ecx);
    ecx = edi;
    eax = esp + 0x40;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7F93u); RECOMP_ABI_CALL(0x001744F0u, sub_001744F0); /* call 0x001744F0 */

loc_001E7F93: ;
    xmm0 = XMM_SCALAR(MEMF(esi + 0x18)); /* movss */
    ecx = MEM32(esp + 0x20);
    eax = MEM32(esp + 0x6C);
    edx = MEM32(esp + 0x74);
    xmm3 = XMM_SCALAR(MEMF(esi + 0x14)); /* movss */
    MEMF(esp + 0x5C) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x28)); /* movss */
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(0xC)) >> 32) & 1);
    esp = esp + 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEMF(esp + 0x48) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esi + 0x20)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(esp + 0x84); /* mulss */
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    MEMF(esp + 0x54) = xmm3.f[0]; /* movss */
    MEMF(esp + 0x4C) = xmm0.f[0]; /* movss */
    MEM32(esp + 0x5C) = edx;
    MEM8(esp + 0x3E) = 0;
    MEM32(esp + 0x14) = ecx;
    if (CMP_EQ(_fa, _fb)) goto loc_001E8006; /* je: equal / zero */

loc_001E7FEA: ;
    edx = ZX16(LO16(ecx));
    edi = esp + 0x34;
    eax = esi;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E7FF8u); RECOMP_ABI_CALL(0x001E6C30u, sub_001E6C30); /* call 0x001E6C30 */

loc_001E7FF8: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    MEM32(esp + 0xC) = eax;
    if (CMP_EQ(_fa, _fb)) goto loc_001E8047; /* je: equal / zero */

loc_001E8000: ;
    xmm3 = XMM_SCALAR(MEMF(esp + 0x54)); /* movss */

loc_001E8006: ;
    eax = MEM32(esi + 0x30);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E8035; /* je: equal / zero */

loc_001E800D: ;
    eax = MEM32(ebp + 8);
    SET_LO16(edx, MEM16(esi + 0x50));
    SET_LO16(ecx, MEM16(eax + 0x40));
    SET_LO16(esi, MEM16(esi + 0x42));
    eax = MEM32(esp + 0x3A);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E8025u); RECOMP_ABI_CALL(0x001EA030u, sub_001EA030); /* call 0x001EA030 */

loc_001E8025: ;
    esi = MEM32(ebp + 8);
    xmm0.f[0] = xmm0.f[0] * xmm3.f[0]; /* mulss */
    xmm3 = xmm0; /* movaps */
    MEMF(esp + 0x54) = xmm3.f[0]; /* movss */

loc_001E8035: ;
    _fca = xmm3.f[0]; _fcb = MEMF(0x4149C4); /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_001E8047; /* jbe: below or equal (unsigned <=) (xmm3.f[0] vs MEMF(0x4149C4)) */

loc_001E803E: ;
    edi = esp + 0x34;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E8047u); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_001E8047: ;
    eax = MEM32(esp + 0xA0);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E8064; /* je: equal / zero */

loc_001E8052: ;
    ecx = ZX16(MEM16(esp + 0x88));
    MEM32(esp + 0x30) = ecx;
    fp_push((double)SMEM32(esp + 0x30)); /* fild */
    goto loc_001E808D;

loc_001E8064: ;
    edx = ZX16(MEM16(ebx + 0x14));
    xmm0 = XMM_SCALAR(MEMF(ebx + 8)); /* movss */
    xmm0.f[0] = xmm0.f[0] - MEMF(ebx); /* subss */
    xmm1.f[0] = (float)(int32_t)edx; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * xmm1.f[0]; /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x43B494); /* mulss */
    eax = (int32_t)recomp_fist((double)xmm0.f[0], 0x0E7Fu, 32); /* cvttss2si */
    MEM32(esp + 0x30) = eax;
    fp_push((double)SMEM32(esp + 0x30)); /* fild */

loc_001E808D: ;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x1C)); /* fmul dword ptr [esp + 0x1c] */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E8096u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E8096: ;
    ecx = MEM32(esp + 0x10);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(ecx) + (uint64_t)(eax)) >> 32) & 1);
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = MEM32(esp + 0x2C);
    goto loc_001E84A8;

loc_001E80A5: ;
    eax = MEM32(esp + 0x18);
    SET_LO8(ebx, MEM8(eax + 1));
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _cf = 0; /* xor clears CF */
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(LO8(ebx)) & 0xFFu; _fb = (uint32_t)(0x2D) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ebx), 0x2D (8-bit) */
    _cf = (int)(_fa < _fb);
    SET_LO8(edx, (CMP_NE(_fa, _fb)) ? 1 : 0); /* setne */
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(esp + 0x18) = eax;
    SET_LO8(eax, MEM8(eax));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x30) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x30 (8-bit) */
    _cf = (int)(_fa < _fb);
    edx = edx + edx + -1;
    if (CMP_L(_fas, _fbs)) goto loc_001E84AE; /* jl: less (signed <) */

loc_001E80C8: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x39) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x39 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_001E84AE; /* jg: greater (signed >) */

loc_001E80D0: ;
    ecx = MEM32(esp + 0x18);
    eax = SX8(LO8(eax));
    _fb = (uint32_t)(0x30) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(eax) < (uint32_t)(0x30));
    eax = eax - 0x30;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(esp + 0x18) = ecx;
    SET_LO8(ecx, MEM8(ecx));
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0x30) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), 0x30 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_L(_fas, _fbs)) goto loc_001E80F5; /* jl: less (signed <) */

loc_001E80E6: ;
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0x39) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), 0x39 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_001E80F5; /* jg: greater (signed >) */

loc_001E80EB: ;
    ecx = SX8(LO8(ecx));
    eax = eax + eax * 4;
    eax = ecx + eax * 2 + -48;

loc_001E80F5: ;
    edx = (uint32_t)((int32_t)edx * (int32_t)eax);
    MEM32(esp + 0x90) = edx;
    goto loc_001E84AE;

loc_001E8104: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0xA) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0xA (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_NE(_fa, _fb)) goto loc_001E8147; /* jne: not equal / not zero */

loc_001E8108: ;
    edi = MEM32(esp + 0x20);
    SET_LO16(edx, ZX8(MEM8(esi + 0x4D)));
    edi++;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(LO16(edi)) & 0xFFFFu; _fb = (uint32_t)(LO16(edx)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(edi), LO16(edx) (16-bit) */
    _cf = (int)(_fa < _fb);
    MEM32(esp + 0x20) = edi;
    if (CMP_AE(_fa, _fb)) goto loc_001E84D0; /* jae: above or equal (unsigned >=) */

loc_001E811F: ;
    SET_LO16(edx, (uint32_t)(int32_t)SMEM8(esi + 0x4C));
    eax = ZX16(LO16(edi));
    SET_LO16(ecx, MEM16(esp + eax * 2 + 0x2C8));
    eax = MEM32(esp + 0x24);
    MEM16(esp + 0x10) = LO16(ecx);
    _fb = (uint32_t)(MEM32(esp + 0x64)) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(edx) + (uint64_t)(MEM32(esp + 0x64))) >> 32) & 1);
    edx = edx + MEM32(esp + 0x64);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(edx)) >> 32) & 1);
    eax = eax + edx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esp + 0x24) = eax;
    goto loc_001E84AE;

loc_001E8147: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x20) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x20 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_EQ(_fa, _fb)) goto loc_001E8481; /* je: equal / zero */

loc_001E814F: ;
    eax = MEM32(esi + 0x38);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(3) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 3 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_L(_fas, _fbs)) goto loc_001E815B; /* jl: less (signed <) */

loc_001E8157: ;
    _cf = 0; /* xor clears CF */
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    goto loc_001E8181;

loc_001E815B: ;
    ecx = esp + 0xC0;
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, edi);
    eax = eax + eax * 2;
    PUSH32(esp, ecx);
    { uint32_t _icall_target = MEM32(eax * 8 + 0x452A08); PUSH32(esp, 0x001E816Eu); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x001E8167u); } /* indirect call */
    }

loc_001E816E: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E817E; /* je: equal / zero */

loc_001E8175: ;
    eax = MEM32(esp + 0xC0);
    goto loc_001E8181;

loc_001E817E: ;
    _cf = 0; /* logical op clears CF */
    eax = eax | 0xFFFFFFFFu;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */

loc_001E8181: ;
    MEM32(esp + 0x34) = eax;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E818Au); RECOMP_ABI_CALL(0x0021A960u, sub_0021A960); /* call 0x0021A960 */

loc_001E818A: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    MEM32(esp + 0x94) = eax;
    if (CMP_NE(_fa, _fb)) goto loc_001E81D6; /* jne: not equal / not zero */

loc_001E8195: ;
    eax = MEM32(esp + 0x8C);
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(1) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 1 (16-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_BE(_fa, _fb)) goto loc_001E81AD; /* jbe: below or equal (unsigned <=) */

loc_001E81A2: ;
    edx = ZX16(LO16(eax));
    eax = edi + edx + -1;
    MEM32(esp + 0x18) = eax;

loc_001E81AD: ;
    ecx = ZX16(MEM16(esp + 0x88));
    xmm0.f[0] = (float)(int32_t)ecx; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * MEMF(esp + 0x1C); /* mulss */
    MEMF(esp + 0xBC) = xmm0.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0xBC), 0x0E7Fu, 32); /* cvttss2si */
    goto loc_001E84A4;

loc_001E81D6: ;
    _fa = (uint32_t)(MEM16(esp + 0x6C)) & 0xFFFFu; _fb = (uint32_t)(0) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(esp + 0x6C), 0 (16-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_BE(_fa, _fb)) goto loc_001E822D; /* jbe: below or equal (unsigned <=) */

loc_001E81DE: ;
    eax = MEM32(esi + 0x38);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(3) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 3 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_GE(_fas, _fbs)) goto loc_001E820F; /* jge: greater or equal (signed >=) */

loc_001E81E6: ;
    edx = eax + eax * 2;
    eax = MEM32(edx * 8 + 0x452A0C);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E820F; /* je: equal / zero */

loc_001E81F4: ;
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, edi);
    { uint32_t _icall_target = eax; PUSH32(esp, 0x001E81F7u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x001E81F5u); } /* indirect call */
    }

loc_001E81F7: ;
    eax = SX16(LO16(eax));
    xmm0.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x43B494); /* mulss */
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = (int32_t)recomp_fist((double)xmm0.f[0], 0x0E7Fu, 32); /* cvttss2si */
    goto loc_001E8211;

loc_001E820F: ;
    _cf = 0; /* xor clears CF */
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */

loc_001E8211: ;
    ecx = SX16(LO16(eax));
    MEM32(esp + 0x30) = ecx;
    fp_push((double)SMEM32(esp + 0x30)); /* fild */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x1C)); /* fmul dword ptr [esp + 0x1c] */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E8225u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E8225: ;
    ebx = MEM32(esp + 0x10);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(ebx) + (uint64_t)(eax)) >> 32) & 1);
    ebx = ebx + eax;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_001E8231;

loc_001E822D: ;
    ebx = MEM32(esp + 0x10);

loc_001E8231: ;
    edx = MEM32(esp + 0x90);
    xmm1 = XMM_SCALAR(MEMF(esp + 0x84)); /* movss */
    xmm0 = XMM_SCALAR(MEMF(esi + 0x1C)); /* movss */
    SET_LO16(ecx, MEM16(esi + 0x48));
    xmm3 = XMM_SCALAR(MEMF(esi + 0x18)); /* movss */
    SET_LO16(eax, MEM16(esp + 0x24));
    xmm2 = XMM_ZERO(); /* xorps self = zero */
    MEM32(esi + 8) = edx;
    edx = MEM32(esp + 0x68);
    xmm0.f[0] = xmm0.f[0] * xmm1.f[0]; /* mulss */
    xmm4 = xmm0; /* movaps */
    MEMF(esp + 0x1C) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0xAC)); /* movss */
    xmm4.f[0] = xmm4.f[0] * xmm0.f[0]; /* mulss */
    MEMF(esp + 0x48) = xmm4.f[0]; /* movss */
    xmm4 = XMM_SCALAR(MEMF(esi + 0x20)); /* movss */
    MEM32(esp + 0x5C) = edx;
    edx = MEM32(esp + 0x14);
    MEM16(esp + 0x3C) = LO16(ecx);
    ecx = MEM32(esp + 0x60);
    edx++;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    MEMF(esp + 0x50) = xmm3.f[0]; /* movss */
    xmm3 = XMM_SCALAR(MEMF(esi + 0x14)); /* movss */
    xmm4.f[0] = xmm4.f[0] * xmm1.f[0]; /* mulss */
    xmm4.f[0] = xmm4.f[0] * xmm0.f[0]; /* mulss */
    MEM16(esp + 0x38) = LO16(ebx);
    MEM16(esp + 0x3A) = LO16(eax);
    MEM8(esp + 0x3F) = 1;
    MEM8(esp + 0x40) = 1;
    MEM32(esp + 0x44) = 0;
    MEMF(esp + 0x58) = xmm2.f[0]; /* movss */
    MEMF(esp + 0x54) = xmm3.f[0]; /* movss */
    MEMF(esp + 0x4C) = xmm4.f[0]; /* movss */
    MEM8(esp + 0x3E) = 0;
    MEM32(esp + 0x14) = edx;
    MEM32(esp + 0xC) = 1;
    if (CMP_EQ(_fa, _fb)) goto loc_001E8313; /* je: equal / zero */

loc_001E82EB: ;
    edx = ZX16(LO16(edx));
    edi = esp + 0x34;
    eax = esi;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E82F9u); RECOMP_ABI_CALL(0x001E6C30u, sub_001E6C30); /* call 0x001E6C30 */

loc_001E82F9: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    MEM32(esp + 0xC) = eax;
    if (CMP_EQ(_fa, _fb)) goto loc_001E839C; /* je: equal / zero */

loc_001E8305: ;
    xmm3 = XMM_SCALAR(MEMF(esp + 0x54)); /* movss */
    SET_LO16(eax, MEM16(esp + 0x3A));
    xmm2 = XMM_ZERO(); /* xorps self = zero */

loc_001E8313: ;
    ecx = MEM32(esi + 0x30);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E838A; /* je: equal / zero */

loc_001E831A: ;
    SET_LO16(ecx, MEM16(esi + 0x40));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(LO16(ecx)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), LO16(ecx) (16-bit) */
    _cf = (int)(_fa < _fb);
    SET_LO16(edx, MEM16(esi + 0x50));
    SET_LO16(edi, MEM16(esi + 0x42));
    if (CMP_LE(_fas, _fbs)) goto loc_001E833A; /* jle: less or equal (signed <=) */

loc_001E832B: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(LO16(edi)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), LO16(edi) (16-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_GE(_fas, _fbs)) goto loc_001E8348; /* jge: greater or equal (signed >=) */

loc_001E8330: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    goto loc_001E837D;

loc_001E833A: ;
    ecx = SX16(LO16(ecx));
    eax = SX16(LO16(eax));
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(ecx) < (uint32_t)(eax));
    ecx = ecx - eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    xmm1.f[0] = (float)(int32_t)ecx; /* cvtsi2ss */
    goto loc_001E8354;

loc_001E8348: ;
    ecx = SX16(LO16(edi));
    eax = SX16(LO16(eax));
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(eax) < (uint32_t)(ecx));
    eax = eax - ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    xmm1.f[0] = (float)(int32_t)eax; /* cvtsi2ss */

loc_001E8354: ;
    ecx = SX16(LO16(edx));
    xmm0.f[0] = (float)(int32_t)ecx; /* cvtsi2ss */
    xmm1.f[0] = xmm1.f[0] / xmm0.f[0]; /* divss */
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    _fca = xmm1.f[0]; _fcb = xmm0.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_001E8371; /* jbe: below or equal (unsigned <=) (xmm1.f[0] vs xmm0.f[0]) */

loc_001E836C: ;
    xmm1 = xmm0; /* movaps */
    goto loc_001E8379;

loc_001E8371: ;
    _fca = xmm2.f[0]; _fcb = xmm1.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_001E8379; /* jbe: below or equal (unsigned <=) (xmm2.f[0] vs xmm1.f[0]) */

loc_001E8376: ;
    xmm1 = xmm2; /* movaps */

loc_001E8379: ;
    xmm0.f[0] = xmm0.f[0] - xmm1.f[0]; /* subss */

loc_001E837D: ;
    xmm0.f[0] = xmm0.f[0] * xmm3.f[0]; /* mulss */
    xmm3 = xmm0; /* movaps */
    MEMF(esp + 0x54) = xmm3.f[0]; /* movss */

loc_001E838A: ;
    _fca = xmm3.f[0]; _fcb = MEMF(0x4149C4); /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_001E839C; /* jbe: below or equal (unsigned <=) (xmm3.f[0] vs MEMF(0x4149C4)) */

loc_001E8393: ;
    edi = esp + 0x34;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E839Cu); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_001E839C: ;
    eax = MEM32(esp + 0xA0);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E83B9; /* je: equal / zero */

loc_001E83A7: ;
    edx = ZX16(MEM16(esp + 0x88));
    MEM32(esp + 0x30) = edx;
    fp_push((double)SMEM32(esp + 0x30)); /* fild */
    goto loc_001E83EF;

loc_001E83B9: ;
    eax = MEM32(esp + 0x94);
    xmm0 = XMM_SCALAR(MEMF(eax + 8)); /* movss */
    xmm0.f[0] = xmm0.f[0] - MEMF(eax); /* subss */
    eax = ZX16(MEM16(eax + 0x14));
    xmm1.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * xmm1.f[0]; /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x43B494); /* mulss */
    ecx = (int32_t)recomp_fist((double)xmm0.f[0], 0x0E7Fu, 32); /* cvttss2si */
    MEM32(esp + 0x94) = ecx;
    fp_push((double)SMEM32(esp + 0x94)); /* fild */

loc_001E83EF: ;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x1C)); /* fmul dword ptr [esp + 0x1c] */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E83F8u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E83F8: ;
    edx = MEM32(esp + 0x2C);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(ebx) + (uint64_t)(eax)) >> 32) & 1);
    ebx = ebx + eax;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = MEM32(esp + 0x8C);
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(ebx) + (uint64_t)(edx)) >> 32) & 1);
    ebx = ebx + edx;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(1) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 1 (16-bit) */
    _cf = (int)(_fa < _fb);
    MEM32(esp + 0x10) = ebx;
    if (CMP_BE(_fa, _fb)) goto loc_001E8420; /* jbe: below or equal (unsigned <=) */

loc_001E8411: ;
    edx = ZX16(LO16(eax));
    eax = MEM32(esp + 0x18);
    ecx = eax + edx + -1;
    MEM32(esp + 0x18) = ecx;

loc_001E8420: ;
    eax = MEM32(esp + 0x60);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_001E84AE; /* je: equal / zero */

loc_001E842C: ;
    edx = ZX16(MEM16(esp + 0x14));
    _fa = (uint32_t)(MEM32(esi + 0xC)) & 0xFFFFFFFFu; _fb = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(esi + 0xC), edx (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_AE(_fa, _fb)) goto loc_001E84AE; /* jae: above or equal (unsigned >=) */

loc_001E8436: ;
    edi = MEM32(esp + 0x20);

loc_001E843A: ;
    eax = MEM32(esp + 0x60);
    _cf = 0; /* xor clears CF */
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ecx (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_EQ(_fa, _fb)) goto loc_001E846B; /* je: equal / zero */

loc_001E8444: ;
    eax = MEM32(eax + 0x1C);
    edx = MEM32(esi + 8);
    ebx = MEM32(esi + 4);
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(edx)) >> 32) & 1);
    eax = eax + edx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ebx++;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    edx = ebx;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, eax (32-bit) */
    _cf = (int)(_fa < _fb);
    MEM32(esi + 4) = ebx;
    if (CMP_LE(_fas, _fbs)) goto loc_001E846B; /* jle: less or equal (signed <=) */

loc_001E8459: ;
    _fa = (uint32_t)(MEM32(esi + 0x2C)) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(esi + 0x2C), ecx (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_EQ(_fa, _fb)) goto loc_001E846B; /* je: equal / zero */

loc_001E845E: ;
    MEM32(esi + 4) = ecx;
    MEM32(esi + 0xC) = 0xFFFF;
    MEM32(esi + 0x2C) = ecx;

loc_001E846B: ;
    ecx = MEM32(esp + 0x14CC);
    eax = ZX16(LO16(edi));
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E847Au); RECOMP_ABI_CALL(0x00379C29u, sub_00379C29); /* call 0x00379C29 */

loc_001E847A: ;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebx);
    esp = ebp;
    POP32(esp, ebp);
    esp += 4; return; /* ret */

loc_001E8481: ;
    eax = ZX16(MEM16(esp + 0x88));
    MEM32(esp + 0x94) = eax;
    fp_push((double)SMEM32(esp + 0x94)); /* fild */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x1C)); /* fmul dword ptr [esp + 0x1c] */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E84A0u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E84A0: ;
    _fb = (uint32_t)(MEM32(esp + 0x2C)) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(MEM32(esp + 0x2C))) >> 32) & 1);
    eax = eax + MEM32(esp + 0x2C);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_001E84A4: ;
    ecx = MEM32(esp + 0x10);

loc_001E84A8: ;
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(ecx) + (uint64_t)(eax)) >> 32) & 1);
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_001E84AA: ;
    MEM32(esp + 0x10) = ecx;

loc_001E84AE: ;
    eax = MEM32(esp + 0x18);
    edx = MEM32(esp + 0x6C);
    SET_LO8(ecx, MEM8(eax + 1));
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    edx++;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), 0 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    MEM32(esp + 0x18) = eax;
    MEM32(esp + 0x6C) = edx;
    if (CMP_NE(_fa, _fb)) goto loc_001E7590; /* jne: not equal / not zero */

loc_001E84CB: ;
    goto loc_001E8436;

loc_001E84D0: ;
    eax = MEM32(esi + 0x34);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    MEM32(esi + 0x28) = 1;
    if (CMP_EQ(_fa, _fb)) goto loc_001E843A; /* je: equal / zero */

loc_001E84E2: ;
    PUSH32(esp, esi);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001E84E8u); RECOMP_ABI_CALL(0x001E8DE0u, sub_001E8DE0); /* call 0x001E8DE0 */

loc_001E84E8: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_001E843A;

    ecx--;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fb != 0)) (void)0; /* goto loc_001E8511 - dead code, label not in function */ /* jo: overflow */

loc_001E84F3: ;
    _fb = (uint32_t)(HI8(edx)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM8(ebp + 0x70)) + (uint64_t)(HI8(edx))) >> 8) & 1);
    MEM8(ebp + 0x70) = MEM8(ebp + 0x70) + HI8(edx);
    _fa = (uint32_t)(MEM8(ebp + 0x70)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    PUSH32(esp, 0 /* seg:ds */);
    _fb = (uint32_t)(LO8(eax)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM8(ebx + 0x71)) + (uint64_t)(LO8(eax))) >> 8) & 1);
    MEM8(ebx + 0x71) = MEM8(ebx + 0x71) + LO8(eax);
    _fa = (uint32_t)(MEM8(ebx + 0x71)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    PUSH32(esp, 0 /* seg:ds */);
    _fb = (uint32_t)(HI8(eax)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM8(eax + esi * 2 + 0x71DE001E)) + (uint64_t)(HI8(eax))) >> 8) & 1);
    MEM8(eax + esi * 2 + 0x71DE001E) = MEM8(eax + esi * 2 + 0x71DE001E) + HI8(eax);
    _fa = (uint32_t)(MEM8(eax + esi * 2 + 0x71DE001E)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    PUSH32(esp, 0 /* seg:ds */);
    _fb = (uint32_t)(HI8(eax)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(HI8(eax)) + (uint64_t)(HI8(eax))) >> 8) & 1);
    SET_HI8(eax, HI8(eax) + HI8(eax));
    _fa = (uint32_t)(HI8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    if (_flags /* jno: not overflow */) goto loc_001E8525;

loc_001E8507: ;
    _fb = (uint32_t)(HI8(ebx)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM8(-1342169486)) + (uint64_t)(HI8(ebx))) >> 8) & 1);
    MEM8(-1342169486) = MEM8(-1342169486) + HI8(ebx);
    _fa = (uint32_t)(MEM8(-1342169486)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    if (_cf) (void)0; /* goto loc_001E852D - dead code, label not in function */ /* jb: below (unsigned <) */

loc_001E850F: ;
    _fb = (uint32_t)(LO8(ecx)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM8(esi + 0x1E74)) + (uint64_t)(LO8(ecx))) >> 8) & 1);
    MEM8(esi + 0x1E74) = MEM8(esi + 0x1E74) + LO8(ecx);
    _fa = (uint32_t)(MEM8(esi + 0x1E74)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    _cf = 0; /* logical op clears CF */
    MEM8(ecx) = MEM8(ecx) | LO8(eax);
    _fa = (uint32_t)(MEM8(ecx)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */

loc_001E8525: ;
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(edx) = MEM8(edx) | LO8(eax);
    _fa = (uint32_t)(MEM8(edx)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _fb = (uint32_t)(MEM32(eax + ecx)) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(MEM32(eax + ecx))) >> 32) & 1);
    eax = eax + MEM32(eax + ecx);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(0x8080806) = MEM8(0x8080806) | LO8(eax);
    _fa = (uint32_t)(MEM8(0x8080806)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(edx) = MEM8(edx) | LO8(eax);
    _fa = (uint32_t)(MEM8(edx)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _fb = (uint32_t)(MEM32(eax + ecx)) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(MEM32(eax + ecx))) >> 32) & 1);
    eax = eax + MEM32(eax + ecx);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(0x8080806) = MEM8(0x8080806) | LO8(eax);
    _fa = (uint32_t)(MEM8(0x8080806)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    /* nop */
    if (0) _cf = (int)(((MEM32(eax + 0x1E)) >> (32 - (0))) & 1);
    MEM32(eax + 0x1E) = MEM32(eax + 0x1E) << 0;
    _fa = (uint32_t)(MEM32(eax + 0x1E)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    RECOMP_UNIMPL("cli", 0x001E8568u); /* TODO: cli */
    if (_flags /* jo: overflow */) goto loc_001E8589;

loc_001E856B: ;
    _fb = (uint32_t)(LO8(eax)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM8(eax)) + (uint64_t)(LO8(eax))) >> 8) & 1);
    MEM8(eax) = MEM8(eax) + LO8(eax);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(eax)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(eax) = MEM32(eax) + eax;
    _fa = (uint32_t)(MEM32(eax)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(LO8(eax)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM8(ecx)) + (uint64_t)(LO8(eax))) >> 8) & 1);
    MEM8(ecx) = MEM8(ecx) + LO8(eax);
    _fa = (uint32_t)(MEM8(ecx)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_001E8589: ;
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(LO8(eax)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM8(ecx)) + (uint64_t)(LO8(eax))) >> 8) & 1);
    MEM8(ecx) = MEM8(ecx) + LO8(eax);
    _fa = (uint32_t)(MEM8(ecx)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(LO8(eax)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM8(ecx)) + (uint64_t)(LO8(eax))) >> 8) & 1);
    MEM8(ecx) = MEM8(ecx) + LO8(eax);
    _fa = (uint32_t)(MEM8(ecx)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(LO8(eax)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM8(eax)) + (uint64_t)(LO8(eax))) >> 8) & 1);
    MEM8(eax) = MEM8(eax) + LO8(eax);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(eax)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(eax) = MEM32(eax) + eax;
    _fa = (uint32_t)(MEM32(eax)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(LO8(eax)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM8(ecx)) + (uint64_t)(LO8(eax))) >> 8) & 1);
    MEM8(ecx) = MEM8(ecx) + LO8(eax);
    _fa = (uint32_t)(MEM8(ecx)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(LO8(eax)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM8(ecx)) + (uint64_t)(LO8(eax))) >> 8) & 1);
    MEM8(ecx) = MEM8(ecx) + LO8(eax);
    _fa = (uint32_t)(MEM8(ecx)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(LO8(eax)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM8(ecx)) + (uint64_t)(LO8(eax))) >> 8) & 1);
    MEM8(ecx) = MEM8(ecx) + LO8(eax);
    _fa = (uint32_t)(MEM8(ecx)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ecx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(ecx) = MEM32(ecx) + eax;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(LO8(eax)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM8(eax)) + (uint64_t)(LO8(eax))) >> 8) & 1);
    MEM8(eax) = MEM8(eax) + LO8(eax);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    /* nop */
    if (0) _cf = (int)(((MEM32(ebp + 0x1E)) >> (32 - (0))) & 1);
    MEM32(ebp + 0x1E) = MEM32(ebp + 0x1E) << 0;
    _fa = (uint32_t)(MEM32(ebp + 0x1E)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    g_seh_ebp = ebp; sub_211EA43A(); return; /* tail jmp 0x211EA43A */

    if (_flags /* js: sign (negative) */) goto loc_001E85E5;

loc_001E85C7: ;
    _fb = (uint32_t)(LO8(edx)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM8(edi)) + (uint64_t)(LO8(edx))) >> 8) & 1);
    MEM8(edi) = MEM8(edi) + LO8(edx);
    _fa = (uint32_t)(MEM8(edi)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    if ((_cf || _fa == 0)) goto loc_001E85E9; /* jbe: below or equal (unsigned <=) */

loc_001E85CB: ;
    _fb = (uint32_t)(LO8(eax)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM8(ebx + 0x7E)) + (uint64_t)(LO8(eax))) >> 8) & 1);
    MEM8(ebx + 0x7E) = MEM8(ebx + 0x7E) + LO8(eax);
    _fa = (uint32_t)(MEM8(ebx + 0x7E)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    PUSH32(esp, 0 /* seg:ds */);
    _fb = (uint32_t)(LO8(ebx)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM8(ebx)) + (uint64_t)(LO8(ebx))) >> 8) & 1);
    MEM8(ebx) = MEM8(ebx) + LO8(ebx);
    _fa = (uint32_t)(MEM8(ebx)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    if ((_fas > 0)) goto loc_001E85F1; /* jg: greater (signed >) */

loc_001E85D3: ;
    _fb = (uint32_t)(HI8(edx)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM8(ebp + -1526718850)) + (uint64_t)(HI8(edx))) >> 8) & 1);
    MEM8(ebp + -1526718850) = MEM8(ebp + -1526718850) + HI8(edx);
    _fa = (uint32_t)(MEM8(ebp + -1526718850)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    { uint64_t _t = (uint64_t)(MEM8(esi)) - (uint64_t)(0) - (uint64_t)_cf; _cf = (int)((_t >> 8) & 1); MEM8(esi) = (uint32_t)_t; }  /* sbb */
    _fa = (uint32_t)(MEM8(esi)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* sbb result */
    { uint32_t _rc = ecx; ecx = 1;
    { int32_t _st = RECOMP_DF_STEP(1);
    while (ecx != 0) {
        _flags = (LO8(eax) == MEM8(edi));
        _cf = (LO8(eax) < MEM8(edi));
        edi += _st; ecx--;
        if (!_flags) break;
    } } /* repe scasb */
    ecx = _rc; }
    _fa = (uint32_t)(MEM8(esi)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test MEM8(esi), LO8(ebx) (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    _fb = (uint32_t)(LO8(eax)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM8(eax)) + (uint64_t)(LO8(eax))) >> 8) & 1);
    MEM8(eax) = MEM8(eax) + LO8(eax);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    _cf = 0; /* logical op clears CF */
    MEM8(ecx) = MEM8(ecx) | LO8(eax);
    _fa = (uint32_t)(MEM8(ecx)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */

loc_001E85E5: ;
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */

loc_001E85E9: ;
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */

loc_001E85F1: ;
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(edx) = MEM8(edx) | LO8(eax);
    _fa = (uint32_t)(MEM8(edx)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _fb = (uint32_t)(MEM32(eax + ecx)) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(MEM32(eax + ecx))) >> 32) & 1);
    eax = eax + MEM32(eax + ecx);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(0x8080806) = MEM8(0x8080806) | LO8(eax);
    _fa = (uint32_t)(MEM8(0x8080806)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(edx) = MEM8(edx) | LO8(eax);
    _fa = (uint32_t)(MEM8(edx)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _fb = (uint32_t)(MEM32(eax + ecx)) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(MEM32(eax + ecx))) >> 32) & 1);
    eax = eax + MEM32(eax + ecx);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(eax) = MEM8(eax) | LO8(ecx);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    _cf = 0; /* logical op clears CF */
    MEM8(0x8080806) = MEM8(0x8080806) | LO8(eax);
    _fa = (uint32_t)(MEM8(0x8080806)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    /* nop */
    SET_LO8(ecx, 0x76);
    PUSH32(esp, 0 /* seg:ds */);
    _fb = (uint32_t)(HI8(ebx)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM8(edi)) + (uint64_t)(HI8(ebx))) >> 8) & 1);
    MEM8(edi) = MEM8(edi) + HI8(ebx);
    _fa = (uint32_t)(MEM8(edi)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    if ((_cf || _fa == 0)) goto loc_001E8655; /* jbe: below or equal (unsigned <=) */

loc_001E8637: ;
    _fb = (uint32_t)(LO8(edx)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM8(esi + esi * 2 + 0x1E)) + (uint64_t)(LO8(edx))) >> 8) & 1);
    MEM8(esi + esi * 2 + 0x1E) = MEM8(esi + esi * 2 + 0x1E) + LO8(edx);
    _fa = (uint32_t)(MEM8(esi + esi * 2 + 0x1E)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    _fb = (uint32_t)(LO8(ecx)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM8(ebp + -1627382154)) + (uint64_t)(LO8(ecx))) >> 8) & 1);
    MEM8(ebp + -1627382154) = MEM8(ebp + -1627382154) + LO8(ecx);
    _fa = (uint32_t)(MEM8(ebp + -1627382154)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    if ((_cf || _fa == 0)) goto loc_001E8661; /* jbe: below or equal (unsigned <=) */

loc_001E8643: ;
    _fb = (uint32_t)(HI8(ecx)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM8(ecx + 0x76)) + (uint64_t)(HI8(ecx))) >> 8) & 1);
    MEM8(ecx + 0x76) = MEM8(ecx + 0x76) + HI8(ecx);
    _fa = (uint32_t)(MEM8(ecx + 0x76)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    PUSH32(esp, 0 /* seg:ds */);
    _fb = (uint32_t)(HI8(ebx)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM8(ebx + 0x76)) + (uint64_t)(HI8(ebx))) >> 8) & 1);
    MEM8(ebx + 0x76) = MEM8(ebx + 0x76) + HI8(ebx);
    _fa = (uint32_t)(MEM8(ebx + 0x76)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    PUSH32(esp, 0 /* seg:ds */);
    _fb = (uint32_t)(LO8(eax)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(LO8(ebx)) + (uint64_t)(LO8(eax))) >> 8) & 1);
    SET_LO8(ebx, LO8(ebx) + LO8(eax));
    _fa = (uint32_t)(LO8(ebx)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    if ((_cf || _fa == 0)) goto loc_001E866D; /* jbe: below or equal (unsigned <=) */

loc_001E864F: ;
    _fb = (uint32_t)(LO8(eax)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM8(eax)) + (uint64_t)(LO8(eax))) >> 8) & 1);
    MEM8(eax) = MEM8(eax) + LO8(eax);
    _fa = (uint32_t)(MEM8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */

loc_001E8655: ;
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */

loc_001E8661: ;
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(edx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(edx) = MEM32(edx) + eax;
    _fa = (uint32_t)(MEM32(edx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */

loc_001E866D: ;
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    _fb = (uint32_t)(MEM32(edi)) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(MEM32(edi))) >> 32) & 1);
    eax = eax + MEM32(edi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    _fb = (uint32_t)(7) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(LO8(eax)) + (uint64_t)(7)) >> 8) & 1);
    SET_LO8(eax, LO8(eax) + 7);
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    _fb = (uint32_t)(0x7070706) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(0x7070706)) >> 32) & 1);
    eax = eax + 0x7070706;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(edx)) + (uint64_t)(eax)) >> 32) & 1);
    MEM32(edx) = MEM32(edx) + eax;
    _fa = (uint32_t)(MEM32(edx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    _fb = (uint32_t)(MEM32(edi)) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(MEM32(edi))) >> 32) & 1);
    eax = eax + MEM32(edi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    _fb = (uint32_t)(7) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(LO8(eax)) + (uint64_t)(7)) >> 8) & 1);
    SET_LO8(eax, LO8(eax) + 7);
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    { uint32_t _tmp; POP32(esp, _tmp); } /* pop es - segment register */
    _fb = (uint32_t)(0x48B8CC06) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(0x48B8CC06)) >> 32) & 1);
    eax = eax + 0x48B8CC06;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}

