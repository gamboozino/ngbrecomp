/*
 * responsive.c -- Responsive Menus (Gameplay, #116): presses the game would ignore for a while taken at once.
 *
 * The title: the PRESS START BUTTON screen counts its frames in 0x45A7F4 while 0x45A7F8 is 1, and
 * sub_00165930 reads no button until the count passes 0x43B688 + 0x43B694 (40 frames of fade-in and 60
 * more, a second with the prompt up and Start doing nothing). The screen's drawing reads the count too
 * (sub_0022CA00 through sub_0022C920): the prompt from 40 on, a sound at 30. Once the prompt is drawn
 * (src/pause_tab.c, sprite 0xC5) the count is set to the mark; both have happened by then.
 *
 * Cutscenes: Start skips one only once its script has said it may (0x8D3A66 = 1, command 3 0 in sub_001DD300;
 * src/cutscene_skip.c), and scripts often open with a wait first (sub_001DD1B0: 0x8D3A54 = 0 a wait of frames,
 * counted down in 0x8D3A52 by sub_001DCD60; the chapter 1 waterfall's is 180). The skip itself jumps the script
 * to its end mark without running the commands between (sub_001DD690), so it is not let through early: a new
 * press of Start while the script has yet to reach that command, and does reach it a few commands on, ends the
 * waits before it; the script runs its own commands up to it, and the press is kept for it. An event that never
 * says so, or is past the part it said so for, is left as it is.
 *
 * The pause menu: its message box (sub_00212500: "equipped" and the like, 0x8DB848 the message, 0x8DB84C its 60
 * frames) returns 1 while it is up, and its callers (sub_001828B0, sub_00185AD0) then set a busy bit (0x1000) in
 * 0xDA41E8 and 0xDA4218, which keeps the menu from closing: Start after equipping a weapon did nothing for a
 * second. The function is the lifted one, kept here (src/manual_functions.json), returning 0 with the option on;
 * the message is drawn and times out as before. Equipping also leaves the weapon list in a state of its own
 * (bit 1 of 0xDA41E8 and of its +0x18 at 0xDA4218, 0xDA420A and 0xDA4211 of the list at 0xDA4200; set by
 * sub_00180DC0 and the like) that takes the next Start or B to leave the list rather than the menu, or ends by
 * itself a moment later; while the message is up it is ended at once, as the game ends it, so one Start closes
 * the menu.
 */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"

int ngb_pc_setting(int which);   /* src/ingame_menu.c: 34 Skip Any Cutscene, 36 Responsive Menus */

/* The title's PRESS START BUTTON drawn (src/pause_tab.c) */
void ngb_responsive_title(void)
{
    uint32_t mark = MEM32(0x43B694) + MEM32(0x43B688);
    if (ngb_pc_setting(36) && MEM32(0x45A7F8) == 1 && MEM32(0x45A7F4) < mark)
        MEM32(0x45A7F4) = mark;
}

static int s_on, s_pending, s_allowed, s_start, s_age;   /* this event: the option at its start, a press kept,
                                                            made skippable once, Start down last frame, frames
                                                            since the press was kept */

static int start_down(void) { return MEM8(MEM8(0xDB932E) * 0x78 + 0xD219A8) & 0x10; }

/* The script, from its next command on, makes the event skippable within a few commands: 3 0 before any other
 * 3 (the end of the skippable part, 3 1, and the jumps, conditional or not: the 3 0 found past one might be on a
 * path not taken) or the end (6). Commands are their length in bytes at 0x42D49A, as sub_001DD690 steps over
 * them. */
static int skippable_ahead(void)
{
    uint32_t p = MEM32(0x8D3A70);
    for (int i = 0; i < 32; i++) {
        uint16_t op = MEM16(p), len = op < 0x6E ? MEM16(0x42D49A + op * 4) : 0;
        if (op == 3)
            return MEM16(p + 2) == 0;
        if (op == 6 || !len)
            return 0;
        p += len;
    }
    return 0;
}

/* The event runner (src/cutscene_skip.c) starting an event */
void ngb_responsive_event_start(void)
{
    s_on = ngb_pc_setting(36);
    s_pending = s_allowed = 0;
    s_start = start_down();   /* (a Start held from the event before is not a new press) */
}

/* The event runner waiting while the game is paused: Start, which closes the pause menu, counts as held
 * until let go */
void ngb_responsive_paused(void)
{
    s_start = 1;
    if (s_pending && s_age <= 2)
        s_pending = 0;   /* the press that opened it was not one for the cutscene */
}

/* The event runner, once a frame before its Start test: 1 when a press kept from before the event was
 * skippable is to skip it now. */
int ngb_responsive_cutscene(void)
{
    int start = start_down(), press = start && !s_start;
    s_start = start;
    s_age++;
    if (!s_on || !ngb_pc_setting(36))
        return 0;
    if (MEM16(0x8D3A66) != 0)
        s_allowed = 1;
    if (s_allowed) {
        s_pending &= MEM16(0x8D3A66) == 1;   /* skipped, or past its end mark: done */
        /* (in a wait of kind 2 or 6 the game takes Start and does not skip: held for after it) */
        return s_pending && MEM16(0x8D3A54) != 2 && MEM16(0x8D3A54) != 6;
    }
    int may = MEM8(0xDB9477 + MEM16(0xDB9C6E)) || ngb_pc_setting(34);   /* watched before, or Skip Any Cutscene */
    if (press && may && skippable_ahead())
        s_pending = 1, s_age = 0;
    if (s_pending && MEM16(0x8D3A50) == 2 && MEM16(0x8D3A54) == 0 && MEM16(0x8D3A52) > 1)
        MEM16(0x8D3A52) = 1;   /* the wait ends next frame */
    return 0;
}

/* Once a frame (src/cheats.c) */
void ngb_responsive_frame(void)
{
    if (ngb_pc_setting(36) && MEM8(0xDB9AD5) && MEM16(0x8DB848) && (MEM32(0xDA41E8) & 1) && MEM8(0xDA420A) == 2) {
        MEM8(0xDA420A) = 0;       /* (the pause menu up, its message showing: a weapon just equipped) -- the list */
        MEM8(0xDA4211) = 0;       /* as the game leaves it once that state ends by itself, a moment later */
        MEM32(0xDA41E8) &= ~1u;
        MEM32(0xDA4218) &= ~1u;
    }
}

/**
 * sub_00212500
 * Original: 0x00212500 - 0x00212BF9 (1785 bytes, 404 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: standard_frame
 */
void sub_00212500(void)
{
    RECOMP_ENTRY(0x00212500u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* prologue saves caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    double _fca = 0.0, _fcb = 0.0;
    (void)_fca; (void)_fcb;
    #define fp_push(v) do { double _fp_value = (v); \
        g_fp_top = (g_fp_top + 7u) & 7u; \
        g_fp_stack[g_fp_top] = _fp_value; } while (0)
    #define fp_pop() (g_fp_top = (g_fp_top + 1u) & 7u)
    #define fp_top() g_fp_stack[g_fp_top]
    #define fp_st(i) g_fp_stack[(g_fp_top + (i)) & 7u]
    #define fp_st1() fp_st(1)

loc_00212500: ;
    PUSH32(esp, ebp);
    ebp = esp;
    g_ebp = ebp; /* publish frame for frameless callees */
    g_seh_ebp = ebp;
    esp = esp & 0xFFFFFFF0u;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fb = (uint32_t)(0x84) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x84;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    eax = MEM32(ebp + 8);
    PUSH32(esp, ebx);
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    PUSH32(esp, eax);
    esi = 0x8DB9C4;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021251Du); RECOMP_ABI_CALL(0x002122B0u, sub_002122B0); /* call 0x002122B0 */

loc_0021251D: ;
    xmm0 = XMM_SCALAR(MEMF(0x414C6C)); /* movss */
    SET_LO16(edx, MEM16(0x977680));
    MEM32(esp + 0x18) = eax;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO16(eax, MEM16(0x8DB9DE));
    _fb = (uint32_t)(MEM16(0x977684)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(eax, LO16(eax) + MEM16(0x977684));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    MEMF(esp + 0x24) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x415278)); /* movss */
    MEMF(esp + 0x28) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x414C68)); /* movss */
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    edi = 0xFF000000u;
    MEMF(esp + 0x2C) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x415350)); /* movss */
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esi = esp + 0x38;
    ebx = 0x8F7DF0;
    MEMF(esp + 0x2C) = xmm0.f[0]; /* movss */
    MEM32(esp + 0x18) = eax;
    MEM32(esp + 0x44) = edi;
    MEM32(esp + 0x40) = edi;
    MEM32(esp + 0x3C) = edi;
    MEM16(esp + 0x48) = LO16(ecx);
    MEM16(esp + 0x4A) = LO16(eax);
    MEM16(esp + 0x4C) = LO16(edx);
    MEM16(esp + 0x4E) = 0x40;
    MEM32(esp + 0x58) = ecx;
    MEM32(esp + 0x5C) = ecx;
    MEM16(esp + 0x50) = LO16(ecx);
    MEM32(esp + 0x38) = 0x7F000000;
    MEM16(esp + 0x52) = 0x1D6;
    MEM8(esp + 0x54) = 0x20;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x002125CEu); RECOMP_ABI_CALL(0x00222250u, sub_00222250); /* call 0x00222250 */

loc_002125CE: ;
    eax = MEM32(esp + 0x18);
    _fb = (uint32_t)(0xFFFFFFC0u) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0xFFFFFFC0u;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esp + 0x1C) = eax;
    MEM16(esp + 0x4A) = LO16(eax);
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    MEM32(esp + 0x44) = edi;
    MEM32(esp + 0x40) = edi;
    MEM32(esp + 0x3C) = edi;
    MEM16(esp + 0x48) = 0xA4;
    MEM16(esp + 0x4C) = 0x22D;
    MEM16(esp + 0x4E) = 2;
    MEM32(esp + 0x58) = eax;
    MEM32(esp + 0x5C) = eax;
    MEM16(esp + 0x50) = LO16(eax);
    MEM32(esp + 0x38) = 0x7FBFA640;
    MEM16(esp + 0x52) = 0x1D6;
    MEM8(esp + 0x54) = 0x20;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00212627u); RECOMP_ABI_CALL(0x00222250u, sub_00222250); /* call 0x00222250 */

loc_00212627: ;
    SET_LO16(ecx, MEM16(esp + 0x1C));
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    MEM32(esp + 0x44) = edi;
    MEM32(esp + 0x40) = edi;
    MEM32(esp + 0x3C) = edi;
    MEM16(esp + 0x48) = LO16(eax);
    MEM16(esp + 0x4A) = LO16(ecx);
    MEM16(esp + 0x4C) = 0x96;
    MEM16(esp + 0x4E) = 0x14;
    MEM32(esp + 0x58) = eax;
    MEM32(esp + 0x5C) = eax;
    MEM16(esp + 0x50) = LO16(eax);
    MEM32(esp + 0x38) = 0x7F000000;
    MEM16(esp + 0x52) = 0x1D6;
    MEM8(esp + 0x54) = 0x20;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00212678u); RECOMP_ABI_CALL(0x00222250u, sub_00222250); /* call 0x00222250 */

loc_00212678: ;
    edx = MEM32(esp + 0x18);
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    MEM32(esp + 0x44) = edi;
    MEM32(esp + 0x40) = edi;
    MEM32(esp + 0x3C) = edi;
    MEM16(esp + 0x48) = LO16(eax);
    _fb = (uint32_t)(0xFFFFFFA9u) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edx = edx + 0xFFFFFFA9u;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM16(esp + 0x4A) = LO16(edx);
    MEM16(esp + 0x4C) = 0x96;
    MEM16(esp + 0x4E) = 2;
    MEM32(esp + 0x58) = eax;
    MEM32(esp + 0x5C) = eax;
    MEM16(esp + 0x50) = LO16(eax);
    MEM32(esp + 0x38) = 0x7F000000;
    MEM16(esp + 0x52) = 0x1D6;
    MEM8(esp + 0x54) = 0x20;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x002126CBu); RECOMP_ABI_CALL(0x00222250u, sub_00222250); /* call 0x00222250 */

loc_002126CB: ;
    SET_LO16(ecx, MEM16(0x8DB9DE));
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    MEM32(esp + 0x44) = edi;
    MEM32(esp + 0x40) = edi;
    MEM32(esp + 0x3C) = edi;
    _fb = (uint32_t)(0x189) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(ecx, LO16(ecx) + 0x189);
    _fa = (uint32_t)(LO16(ecx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    edi = 0x1D6;
    MEM16(esp + 0x48) = LO16(eax);
    MEM16(esp + 0x4A) = LO16(ecx);
    MEM16(esp + 0x4C) = 0x96;
    MEM16(esp + 0x4E) = 2;
    MEM32(esp + 0x58) = eax;
    MEM32(esp + 0x5C) = eax;
    MEM16(esp + 0x50) = LO16(eax);
    MEM32(esp + 0x38) = 0x7FBFA640;
    MEM16(esp + 0x52) = LO16(edi);
    MEM8(esp + 0x54) = LO8(eax);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00212725u); RECOMP_ABI_CALL(0x00222250u, sub_00222250); /* call 0x00222250 */

loc_00212725: ;
    edx = MEM32(ebp + 8);
    SET_LO16(eax, MEM16(edx + 6));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x18) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x18 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212796; /* je: equal / zero */

loc_00212732: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x19) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x19 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212796; /* je: equal / zero */

loc_00212738: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x1A) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x1A (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212796; /* je: equal / zero */

loc_0021273E: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x1B) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x1B (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212796; /* je: equal / zero */

loc_00212744: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x26) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x26 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212796; /* je: equal / zero */

loc_0021274A: ;
    xmm0 = XMM_SCALAR(MEMF(0x414920)); /* movss */
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO16(eax, MEM16(0x8DB9DE));
    _fb = (uint32_t)(0x189) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(eax, LO16(eax) + 0x189);
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    PUSH32(esp, edi);
    ecx = 0x189;
    MEMF(esp + 0x30) = xmm0.f[0]; /* movss */
    PUSH32(esp, eax);
    PUSH32(esp, 0x96);
    eax = esp + 0x44;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00212779u); RECOMP_ABI_CALL(0x001744F0u, sub_001744F0); /* call 0x001744F0 */

loc_00212779: ;
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esi = esp + 0x20;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00212785u); RECOMP_ABI_CALL(0x00053770u, sub_00053770); /* call 0x00053770 */

loc_00212785: ;
    edi = esp + 0x38;
    MEM32(esp + 0x60) = eax;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00212792u); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_00212792: ;
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    goto loc_00212806;

loc_00212796: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    MEM16(esp + 0x40) = LO16(edi);
    MEMF(esp + 0x4C) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x50) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x54) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x58) = xmm0.f[0]; /* movss */
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    edi = esp + 0x38;
    MEM16(0x8DB9DE) = LO16(ebx);
    MEM32(esp + 0x38) = 0x29D;
    MEM16(esp + 0x3C) = 0x96;
    MEM16(esp + 0x3E) = 0x189;
    MEM8(esp + 0x42) = LO8(ebx);
    MEM8(esp + 0x43) = 1;
    MEM8(esp + 0x44) = 1;
    MEM32(esp + 0x48) = ebx;
    MEMF(esp + 0x5C) = xmm0.f[0]; /* movss */
    MEM32(esp + 0x60) = 0xFFBFA640u;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00212806u); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_00212806: ;
    esi = MEM32(ebp + 8);
    SET_LO16(eax, MEM16(esi));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0xD) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0xD (16-bit) */
    SET_LO16(ecx, MEM16(0x43A0A0));
    if (CMP_NE(_fa, _fb)) goto loc_00212827; /* jne: not equal / not zero */

loc_00212819: ;
    _fa = (uint32_t)(LO16(ecx)) & 0xFFFFu; _fb = (uint32_t)(0x171) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(ecx), 0x171 (16-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00212827; /* jne: not equal / not zero */

loc_00212820: ;
    MEM16(esi) = 0x10;
    goto loc_00212859;

loc_00212827: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0xE) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0xE (16-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0021283B; /* jne: not equal / not zero */

loc_0021282D: ;
    _fa = (uint32_t)(LO16(ecx)) & 0xFFFFu; _fb = (uint32_t)(0x171) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(ecx), 0x171 (16-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0021283B; /* jne: not equal / not zero */

loc_00212834: ;
    MEM16(esi) = 0x11;
    goto loc_00212859;

loc_0021283B: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(2) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 2 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212865; /* je: equal / zero */

loc_00212841: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(4) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 4 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212865; /* je: equal / zero */

loc_00212847: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x13) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x13 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212865; /* je: equal / zero */

loc_0021284D: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0xC) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0xC (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212865; /* je: equal / zero */

loc_00212853: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x1A) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x1A (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212865; /* je: equal / zero */

loc_00212859: ;
    eax = esi;
    ecx = 0x8DB9C4;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00212865u); RECOMP_ABI_CALL(0x00212C20u, sub_00212C20); /* call 0x00212C20 */

loc_00212865: ;
    SET_LO16(eax, MEM16(esi + 6));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x18) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x18 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_002128B0; /* je: equal / zero */

loc_0021286F: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x19) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x19 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_002128B0; /* je: equal / zero */

loc_00212875: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x1A) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x1A (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_002128B0; /* je: equal / zero */

loc_0021287B: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x1B) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x1B (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_002128B0; /* je: equal / zero */

loc_00212881: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x26) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x26 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_002128B0; /* je: equal / zero */

loc_00212887: ;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO16(ecx, MEM16(0x8DB9DE));
    _fb = (uint32_t)(0x191) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(ecx, LO16(ecx) + 0x191);
    _fa = (uint32_t)(LO16(ecx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    PUSH32(esp, 0x1D7);
    eax = esp + 0x3C;
    PUSH32(esp, ecx);
    PUSH32(esp, 0x2C);
    ecx = 0x18A;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x002128ABu); RECOMP_ABI_CALL(0x001744F0u, sub_001744F0); /* call 0x001744F0 */

loc_002128AB: ;
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_00212914;

loc_002128B0: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    SET_LO16(edx, MEM16(0x8DB9DE));
    _fb = (uint32_t)(0x191) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(edx, LO16(edx) + 0x191);
    _fa = (uint32_t)(LO16(edx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    MEMF(esp + 0x4C) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x50) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x54) = xmm0.f[0]; /* movss */
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    MEM32(esp + 0x38) = 0x29E;
    MEM16(esp + 0x3C) = 0x2C;
    MEM16(esp + 0x3E) = LO16(edx);
    MEM16(esp + 0x40) = 0x1D7;
    MEM8(esp + 0x42) = LO8(ebx);
    MEM8(esp + 0x43) = 1;
    MEM8(esp + 0x44) = 1;
    MEM32(esp + 0x48) = ebx;
    MEMF(esp + 0x5C) = xmm0.f[0]; /* movss */
    MEM32(esp + 0x60) = 0xFFFFFFFFu;

loc_00212914: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C0)); /* movss */
    edi = esp + 0x38;
    MEMF(esp + 0x58) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021292Bu); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_0021292B: ;
    SET_LO16(ecx, MEM16(esi));
    _fa = (uint32_t)(LO16(ecx)) & 0xFFFFu; _fb = (uint32_t)(LO16(ebx)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(ecx), LO16(ebx) (16-bit) */
    SET_LO16(eax, MEM16(0x8DB848));
    if (CMP_NE(_fa, _fb)) goto loc_00212942; /* jne: not equal / not zero */

loc_00212939: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(LO16(ebx)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), LO16(ebx) (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212BEE; /* je: equal / zero */

loc_00212942: ;
    edx = ZX16(LO16(ecx));
    _fb = (uint32_t)(0xFFFFFFFEu) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edx = edx + 0xFFFFFFFEu;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x18) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0x18 (32-bit) */
    if (CMP_A(_fa, _fb)) goto loc_002129F6; /* ja: above (unsigned >) */

loc_00212951: ;
    edx = ZX8(MEM8(edx + 0x212C04));
    { uint32_t _jt = MEM32(edx * 4 + 0x212BFC); /* switch: 2 entries, 2 targets */
    if (_jt == 0x0021295Fu) goto loc_0021295F;
    if (_jt == 0x002129F6u) goto loc_002129F6;
    g_seh_ebp = ebp; RECOMP_ITAIL(_jt); return; }

loc_0021295F: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(LO16(ecx)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), LO16(ecx) (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0021298D; /* je: equal / zero */

loc_00212964: ;
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    SET_LO16(eax, LO16(ecx));
    MEM16(0x8DB848) = LO16(eax);
    MEM32(0x8DB84C) = 7;
    MEMF(0x8DB850) = xmm0.f[0]; /* movss */
    MEM32(0x8DB934) = ebx;
    goto loc_00212B4E;

loc_0021298D: ;
    ecx = MEM32(0x8DB84C);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, ebx (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_002129D5; /* je: equal / zero */

loc_00212997: ;
    ecx--;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    MEM32(esp + 0x1C) = ecx;
    fp_push((double)SMEM32(esp + 0x1C)); /* fild */
    MEM32(0x8DB84C) = ecx;
    if (CMP_GE(_fas, _fbs)) goto loc_002129B0; /* jge: greater or equal (signed >=) */

loc_002129AA: ;
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148E4)); /* fadd dword ptr [0x4148e4] */

loc_002129B0: ;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x4153E8)); /* fmul dword ptr [0x4153e8] */
    MEM32(esp + 0x14) = 1;
    MEM32(0x8DB934) = ebx;
    fp_top() = RECOMP_FP_PC(MEMF(0x4148C8) - fp_top()); /* fsubr dword ptr [0x4148c8] */
    MEMF(0x8DB850) = (float)fp_top(); fp_pop(); /* fstp */
    goto loc_00212B4E;

loc_002129D5: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    MEM32(0x8DB84C) = ebx;
    MEMF(0x8DB850) = xmm0.f[0]; /* movss */
    MEM32(0x8DB934) = ebx;
    goto loc_00212B4E;

loc_002129F6: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(2) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 2 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212B06; /* je: equal / zero */

loc_00212A00: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(4) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 4 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212B06; /* je: equal / zero */

loc_00212A0A: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x13) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x13 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212B06; /* je: equal / zero */

loc_00212A14: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0xC) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0xC (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212B06; /* je: equal / zero */

loc_00212A1E: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0xD) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0xD (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212B06; /* je: equal / zero */

loc_00212A28: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0xE) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0xE (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212B06; /* je: equal / zero */

loc_00212A32: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0xF) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0xF (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212B06; /* je: equal / zero */

loc_00212A3C: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x10) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x10 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212B06; /* je: equal / zero */

loc_00212A46: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x11) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x11 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212B06; /* je: equal / zero */

loc_00212A50: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x1A) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x1A (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212B06; /* je: equal / zero */

loc_00212A5A: ;
    ecx = MEM32(0x8DB84C);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, ebx (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212AC5; /* je: equal / zero */

loc_00212A64: ;
    ecx--;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(7) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 7 (32-bit) */
    MEM32(0x8DB84C) = ecx;
    if (CMP_AE(_fa, _fb)) goto loc_00212A98; /* jae: above or equal (unsigned >=) */

loc_00212A70: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    MEM32(esp + 0x1C) = ecx;
    fp_push((double)SMEM32(esp + 0x1C)); /* fild */
    if (CMP_GE(_fas, _fbs)) goto loc_00212A82; /* jge: greater or equal (signed >=) */

loc_00212A7C: ;
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148E4)); /* fadd dword ptr [0x4148e4] */

loc_00212A82: ;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x4153E8)); /* fmul dword ptr [0x4153e8] */
    MEM32(esp + 0x14) = 1;
    MEMF(0x8DB850) = (float)fp_top(); fp_pop(); /* fstp */
    goto loc_00212AD9;

loc_00212A98: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x35) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x35 (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_00212ABB; /* jb: below (unsigned <) */

loc_00212A9D: ;
    edx = 0x3C;
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    edx = edx - ecx;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    ecx = SX16(LO16(edx));
    xmm0.f[0] = (float)(int32_t)ecx; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x4153E8); /* mulss */
    MEMF(0x8DB850) = xmm0.f[0]; /* movss */

loc_00212ABB: ;
    MEM32(esp + 0x14) = 1;
    goto loc_00212AD9;

loc_00212AC5: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(LO16(ebx)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), LO16(ebx) (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212AD9; /* je: equal / zero */

loc_00212ACA: ;
    SET_LO16(eax, 0); /* xor self */
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* xor result */
    MEM32(0x8DB84C) = ebx;
    MEM16(0x8DB848) = LO16(eax);

loc_00212AD9: ;
    SET_LO16(ecx, MEM16(esi));
    _fa = (uint32_t)(LO16(ecx)) & 0xFFFFu; _fb = (uint32_t)(LO16(ebx)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(ecx), LO16(ebx) (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212B4E; /* je: equal / zero */

loc_00212AE1: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(LO16(ecx)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), LO16(ecx) (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212B4E; /* je: equal / zero */

loc_00212AE6: ;
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    SET_LO16(eax, LO16(ecx));
    MEM16(0x8DB848) = LO16(eax);
    MEM32(0x8DB84C) = 0x3C;
    MEMF(0x8DB850) = xmm0.f[0]; /* movss */
    goto loc_00212B4E;

loc_00212B06: ;
    ecx = MEM32(0x8DB84C);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, ebx (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212B44; /* je: equal / zero */

loc_00212B10: ;
    ecx--;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    MEM32(esp + 0x1C) = ecx;
    fp_push((double)SMEM32(esp + 0x1C)); /* fild */
    MEM32(0x8DB84C) = ecx;
    if (CMP_GE(_fas, _fbs)) goto loc_00212B29; /* jge: greater or equal (signed >=) */

loc_00212B23: ;
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148E4)); /* fadd dword ptr [0x4148e4] */

loc_00212B29: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, ebx (32-bit) */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x4153E8)); /* fmul dword ptr [0x4153e8] */
    MEMF(0x8DB850) = (float)fp_top(); fp_pop(); /* fstp */
    if (CMP_NE(_fa, _fb)) goto loc_00212B4E; /* jne: not equal / not zero */

loc_00212B39: ;
    SET_LO16(eax, 0); /* xor self */
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* xor result */
    MEM16(0x8DB848) = LO16(eax);
    goto loc_00212B4E;

loc_00212B44: ;
    MEM32(0x8DB84C) = 7;

loc_00212B4E: ;
    SET_LO16(ecx, MEM16(0x8DB9DE));
    _fb = (uint32_t)(MEM16(0x43A048)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(ecx, LO16(ecx) + MEM16(0x43A048));
    _fa = (uint32_t)(LO16(ecx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    edx = MEM32(0x43B4A0);
    PUSH32(esp, edx);
    edx = SX16(LO16(ecx));
    ecx = MEM32(0x43A044);
    PUSH32(esp, 0x1E0);
    PUSH32(esp, edx);
    edx = MEM32(0x8DB9DC);
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edx = edx + ecx;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx = SX16(LO16(edx));
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(edx, MEM8(0xBCE030));
    PUSH32(esp, ecx);
    eax = ZX16(LO16(eax));
    PUSH32(esp, edx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00212B8Fu); RECOMP_ABI_CALL(0x001E6AE0u, sub_001E6AE0); /* call 0x001E6AE0 */

loc_00212B8F: ;
    ecx = eax;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = esp + 0x48;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00212B9Du); RECOMP_ABI_CALL(0x00168730u, sub_00168730); /* call 0x00168730 */

loc_00212B9D: ;
    xmm0 = XMM_SCALAR(MEMF(0x8DB850)); /* movss */
    eax = esp + 0x48;
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, eax);
    ecx = esp + 0x34;
    PUSH32(esp, ecx);
    MEM8(esp + 0x9A) = LO8(ebx);
    MEMF(esp + 0x64) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00212BC1u); RECOMP_ICALL_SAFE(0x001E86A0u, _icall_esp); /* manual call 0x001E86A0 */
    }

loc_00212BC1: ;
    xmm1 = XMM_SCALAR(MEMF(esp + 0x40)); /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x415354)); /* movss */
    _fb = (uint32_t)(0x18) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x18;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fca = xmm1.f[0]; _fcb = xmm0.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_00212BE1; /* jbe: below or equal (unsigned <=) (xmm1.f[0] vs xmm0.f[0]) */

loc_00212BD7: ;
    xmm0.f[0] = xmm0.f[0] / xmm1.f[0]; /* divss */
    MEMF(esp + 0x54) = xmm0.f[0]; /* movss */

loc_00212BE1: ;
    edx = esp + 0x38;
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, edx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00212BEBu); RECOMP_ICALL_SAFE(0x001E6D30u, _icall_esp); /* manual call 0x001E6D30 */
    }

loc_00212BEB: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_00212BEE: ;
    eax = MEM32(esp + 0x14);
    if (ngb_pc_setting(36))
        eax = 0;   /* the one change: the message never busy, so its callers leave the menu's input alone */
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebx);
    esp = ebp;
    POP32(esp, ebp);
    esp += 4; return; /* ret */

    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}

