/*
 * hud.c -- the HUD on or off (Display & Audio > HUD, #80).
 *
 * sub_001FA210 is the game's in-play HUD, each frame: its first calls (sub_001FA070 to sub_00203A50) run
 * whatever the screen, then -- in play, the HUD not hidden by the game (0xDB9C6E 0xFFFF) -- the health and
 * Ninpo bar (sub_001FA3F0 with the health and Ninpo values) and the rest of the HUD (sub_001FC050 to
 * sub_00201F80), and it returns 1. With the HUD off, that second part is not drawn; the return is the same,
 * so what its caller does with it does not change.
 *
 * A copy of the generated function (src/manual_functions.json lists it) with that one test added.
 */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"

int ngb_pc_setting(int which);   /* src/ingame_menu.c: 24 the HUD shown */
/* This frame's run of it found play (src/pc_menu.c: such a frame is never a 2D screen; cleared at the frame's end) */
volatile int ngb_hud_in_play;

void sub_001FA210(void)
{
    RECOMP_ENTRY(0x001FA210u);

    PUSH32(esp, 0x001FA215u); RECOMP_ABI_CALL(0x001FA070u, sub_001FA070);
    SET_LO8(eax, MEM8(0x4B75E5));
    if (LO8(eax)) {
        PUSH32(esp, 0x001FA223u); RECOMP_ABI_CALL(0x001FA320u, sub_001FA320);
        PUSH32(esp, 0x001FA228u); RECOMP_ABI_CALL(0x001FBE60u, sub_001FBE60);
        PUSH32(esp, 0x001FA22Du); RECOMP_ABI_CALL(0x001FC710u, sub_001FC710);
    }
    PUSH32(esp, 0x001FA232u); RECOMP_ABI_CALL(0x002072B0u, sub_002072B0);
    PUSH32(esp, 0x001FA237u); RECOMP_ABI_CALL(0x00203A50u, sub_00203A50);
    if (MEM32(0xD6AF00) != 1 || MEM8(0xDB5A63) < 0x28) {
        eax = 0;
        esp += 4;
        return;
    }
    ngb_hud_in_play = 1;
    eax = 0;
    if (MEM16(0xDB9C6E) != 0xFFFF) {
        esp += 4;
        return;
    }
    /* The karma window turned off as the HUD goes off and on again as it comes back, by its button, as the player
     * would (#214): skipped while the HUD is off, the window's tally after a fight froze, and its Combo, Kill and
     * Time stayed on screen once the HUD was back; turned on again, it starts over. Pressed only as needed, by the
     * window's own state (0x8DB6DC, its state machine sub_00207760: 0 hidden, 1 sliding in, 7 sliding out -- its
     * place, 0x8DB6D8, reads 0xFF06 both hidden and as it starts in): off only if it shows, on again
     * only if it showed and is hidden -- a press the game missed (two at once, the game took the first only; one
     * as the pause menu closes, a button still held) leaves it as it is rather than the other way round. In play
     * only, not under the pause menu (which would take the press): a change made there is pressed when play goes
     * on. The setting is read once: the Toggle HUD key changes it from another thread. */
    int on = ngb_pc_setting(24);
    static int hidden_for = -1;   /* the HUD off: whether the window showed then (-1: the HUD is on) */
    if (!MEM8(0xDB9AD5)) {
        extern void ngb_karma_window_press(void);   /* src/ingame_menu.c */
        int shows = MEM16(0x8DB6DC) != 0 && MEM16(0x8DB6DC) != 7;
        if (!on && hidden_for < 0) {
            hidden_for = shows;
            if (shows)
                ngb_karma_window_press();
        } else if (on && hidden_for >= 0) {
            if (hidden_for && !shows)
                ngb_karma_window_press();
            hidden_for = -1;
        }
    }
    if (on) {
        eax = MEM16(0x58A56C);
        ecx = MEM16(0x58A56E);
        edx = MEM16(0x58A564);
        PUSH32(esp, eax);
        eax = MEM16(0x58A566);
        PUSH32(esp, ecx);
        PUSH32(esp, edx);
        PUSH32(esp, eax);
        eax = MEM32(0x58A574);
        PUSH32(esp, 0x001FA287u); RECOMP_ABI_CALL(0x001FA3F0u, sub_001FA3F0);
        esp += 0x10;
        PUSH32(esp, 0x001FA28Fu); RECOMP_ABI_CALL(0x001FC050u, sub_001FC050);
        PUSH32(esp, 0x001FA294u); RECOMP_ABI_CALL(0x001FC940u, sub_001FC940);
        PUSH32(esp, 0x001FA299u); RECOMP_ABI_CALL(0x001FCC20u, sub_001FCC20);
        PUSH32(esp, 0x001FA29Eu); RECOMP_ABI_CALL(0x001FE350u, sub_001FE350);
        PUSH32(esp, 0x001FA2A3u); RECOMP_ABI_CALL(0x001FEA10u, sub_001FEA10);
        PUSH32(esp, 0x001FA2A8u); RECOMP_ABI_CALL(0x00208120u, sub_00208120);
        PUSH32(esp, 0x001FA2ADu); RECOMP_ABI_CALL(0x00201F80u, sub_00201F80);
    }
    eax = 1;
    esp += 4;
}

/* The hit counter's "HITS" (#238): sub_001FE350 draws the count's digits and the word after them, the word 0xA8 to the
 * right of the counter's place, where the "H" overlaps a wide digit (the reporter's screenshots in #238: Ninja
 * Gaiden's shorter "HIT" did not). The European release moves the word by a language's own amount (a table of six, by the game's language
 * index: 6 for English and for Japanese); here the same 6, at the three places that draw the word and its glow. The
 * lifted function, kept here (src/manual_functions.json), with that one change. */
#define HITS_SHIFT 6
/**
 * sub_001FE350
 * Original: 0x001FE350 - 0x001FEA01 (1713 bytes, 407 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: standard_frame
 */
void sub_001FE350(void)
{
    RECOMP_ENTRY(0x001FE350u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* prologue saves caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_001FE350: ;
    PUSH32(esp, ebp);
    ebp = esp;
    g_ebp = ebp; /* publish frame for frameless callees */
    g_seh_ebp = ebp;
    esp = esp & 0xFFFFFFF0u;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fb = (uint32_t)(0x64) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x64;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    eax = MEM32(0x41CCD8);
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */
    ecx = MEM32(0x8F7DFC);
    PUSH32(esp, ebx);
    MEM32(esp + 0x64) = eax;
    eax = 0x19;
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    MEM32(esp + 0x3C) = eax;
    MEM32(esp + 0x40) = 0x15;
    MEM32(esp + 0x44) = eax;
    MEM32(esp + 0x48) = eax;
    MEM32(esp + 0x4C) = eax;
    MEM32(esp + 0x50) = eax;
    MEM32(esp + 0x54) = eax;
    MEM32(esp + 0x58) = eax;
    MEM32(esp + 0x5C) = eax;
    MEM32(esp + 0x60) = eax;
    MEM16(esp + 0x64) = 5;
    MEM16(esp + 0x66) = 0xFFF6;
    MEM16(esp + 0x68) = 0xFFE2;
    ebx = 0xD6AF74;
    MEM32(esp + 0x24) = 2;
    goto loc_001FE3D0;

    /* nop */
    /* nop */

loc_001FE3D0: ;
    _fa = (uint32_t)(MEM16(ebx + 0x164)) & 0xFFFFu; _fb = (uint32_t)(0) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(ebx + 0x164), 0 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001FE9DC; /* je: equal / zero */

loc_001FE3DE: ;
    edi = ZX16(MEM16(ebx + 0x168));
    eax = 0x51EB851F;
    { int64_t _r = (int64_t)(int32_t)eax * (int64_t)(int32_t)edi;
      eax = (uint32_t)_r; edx = (uint32_t)(_r >> 32); _fa = eax; _fas = (int32_t)eax; _fb = (_r != (int64_t)(int32_t)_r); }
    edx = (uint32_t)(((int32_t)(int32_t)(edx)) >> ((5) & 31u));
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sar result */
    esi = edx;
    esi = esi >> 0x1F;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + edx;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = esi;
    { int64_t _r = (int64_t)(int32_t)(eax) * (int64_t)(int32_t)(0x64);
      eax = (uint32_t)_r;
      _fa = (uint32_t)_r & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); _fb = (_r != (int64_t)(int32_t)_r); } /* imul */
    edx = edi;
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    edx = edx - eax;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    eax = 0x66666667;
    { int64_t _r = (int64_t)(int32_t)eax * (int64_t)(int32_t)edx;
      eax = (uint32_t)_r; edx = (uint32_t)(_r >> 32); _fa = eax; _fas = (int32_t)eax; _fb = (_r != (int64_t)(int32_t)_r); }
    edx = (uint32_t)(((int32_t)(int32_t)(edx)) >> ((2) & 31u));
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sar result */
    eax = edx;
    eax = eax >> 0x1F;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + edx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edx = esi + esi * 4;
    edx = eax + edx * 2;
    edx = edx + edx * 4;
    edx = edx << 1;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    edi = edi - edx;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0 (32-bit) */
    MEM32(esp + 0x38) = esi;
    MEM32(esp + 0x34) = eax;
    MEM32(esp + 0x30) = edi;
    MEM32(esp + 0x18) = 0;
    if (CMP_LE(_fas, _fbs)) goto loc_001FE43F; /* jle: less or equal (signed <=) */

loc_001FE435: ;
    MEM32(esp + 0x1C) = 3;
    goto loc_001FE44B;

loc_001FE43F: ;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    SET_LO8(edx, (CMP_G(_fas, _fbs)) ? 1 : 0); /* setg */
    edx++;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(esp + 0x1C) = edx;

loc_001FE44B: ;
    eax = MEM32(esp + 0x1C);
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    MEM32(esp + 0x20) = edx;
    if (CMP_LE(_fas, _fbs)) goto loc_001FE6EC; /* jle: less or equal (signed <=) */

loc_001FE45D: ;
    /* nop */

loc_001FE460: ;
    edi = MEM32(esp + edx * 4 + 0x30);
    edx = edi + 0xF1;
    MEM32(ebx + -52) = edx;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO16(edx, MEM16(ebx + 0x16E));
    _fb = (uint32_t)(MEM16(esp + 0x18)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(edx, LO16(edx) + MEM16(esp + 0x18));
    _fa = (uint32_t)(LO16(edx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    eax = ebx + -52;
    xmm1 = xmm0; /* movaps */
    xmm1.f[0] = xmm1.f[0] * MEMF(0xD6AF08); /* mulss */
    xmm1.f[0] = xmm1.f[0] * MEMF(ebx + 0x158); /* mulss */
    MEM32(esp + 0x14) = edi;
    MEM32(ebx + -12) = 0xFFFFFFFFu;
    MEM16(ebx + -44) = 0x1E0;
    _fb = (uint32_t)(0x86) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edx = edx + 0x86;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM16(ebx + -48) = LO16(edx);
    SET_LO16(edx, MEM16(ebx + 0x170));
    _fb = (uint32_t)(0x78) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(edx, LO16(edx) + 0x78);
    _fa = (uint32_t)(LO16(edx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    MEM16(ebx + -46) = LO16(edx);
    _fa = (uint32_t)(MEM32(0x43B490)) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0x43B490), 1 (32-bit) */
    MEMF(ebx + -20) = xmm1.f[0]; /* movss */
    if (CMP_NE(_fa, _fb)) goto loc_001FE565; /* jne: not equal / not zero */

loc_001FE4CD: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(0x8F7DF8)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, MEM32(0x8F7DF8) (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_001FE565; /* jae: above or equal (unsigned >=) */

loc_001FE4D9: ;
    edx = MEM32(0x8F7DF4);
    esi = ecx;
    { int64_t _r = (int64_t)(int32_t)(esi) * (int64_t)(int32_t)(0x70);
      esi = (uint32_t)_r;
      _fa = (uint32_t)_r & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); _fb = (_r != (int64_t)(int32_t)_r); } /* imul */
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + edx;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edx = MEM32(0x8F7DF0);
    ecx = ecx << 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, 0);
    PUSH32(esp, 0);
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + edx;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001FE4FAu); RECOMP_ICALL_SAFE(0x002216B0u, _icall_esp); /* manual call 0x002216B0 */
    }

loc_001FE4FA: ;
    ecx = MEM32(0x8F7DFC);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001FE55D; /* je: equal / zero */

loc_001FE507: ;
    esi = MEM32(0xC0CF14);
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO16(eax, MEM16(ebx + -44));
    ecx = ecx << 0xF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edx = esp + 0x28;
    PUSH32(esp, edx);
    eax = eax & 0x7FFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax = eax | ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    eax = eax << 2;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    MEM32(esp + 0x2C) = eax;
    eax = MEM32(esi + 4);
    PUSH32(esp, eax);
    PUSH32(esp, esi);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001FE533u); RECOMP_ABI_CALL(0x00222DB0u, sub_00222DB0); /* call 0x00222DB0 */

loc_001FE533: ;
    ecx = 1;
    edx = 0xC0CF10;
    edi = eax;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001FE544u); RECOMP_ABI_CALL(0x00222DE0u, sub_00222DE0); /* call 0x00222DE0 */

loc_001FE544: ;
    MEM32(esi + 4) = edi;
    ecx = MEM32(edi + 4);
    MEM32(ecx) = edi;
    ecx = MEM32(0x8F7DFC);
    edi = MEM32(esp + 0x14);
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x8F7DFC) = ecx;

loc_001FE55D: ;
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */

loc_001FE565: ;
    SET_LO8(eax, MEM8(ebx + 0x176));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001FE63A; /* je: equal / zero */

loc_001FE573: ;
    xmm0.f[0] = xmm0.f[0] * MEMF(0xD6AF08); /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(ebx + 0x158); /* mulss */
    edx = edi + 0xFC;
    eax = ebx + -52;
    MEM32(eax) = edx;
    edx = MEM32(ebx + 0x160);
    MEM32(ebx + -12) = edx;
    _fa = (uint32_t)(MEM32(0x43B490)) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0x43B490), 1 (32-bit) */
    MEM16(ebx + -44) = 0x1DF;
    MEMF(ebx + -20) = xmm0.f[0]; /* movss */
    if (CMP_NE(_fa, _fb)) goto loc_001FE63A; /* jne: not equal / not zero */

loc_001FE5AF: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(0x8F7DF8)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, MEM32(0x8F7DF8) (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_001FE63A; /* jae: above or equal (unsigned >=) */

loc_001FE5BB: ;
    edx = MEM32(0x8F7DF4);
    esi = ecx;
    { int64_t _r = (int64_t)(int32_t)(esi) * (int64_t)(int32_t)(0x70);
      esi = (uint32_t)_r;
      _fa = (uint32_t)_r & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); _fb = (_r != (int64_t)(int32_t)_r); } /* imul */
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + edx;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edx = MEM32(0x8F7DF0);
    ecx = ecx << 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, 0);
    PUSH32(esp, 0);
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + edx;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001FE5DCu); RECOMP_ICALL_SAFE(0x002216B0u, _icall_esp); /* manual call 0x002216B0 */
    }

loc_001FE5DC: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001FE63A; /* je: equal / zero */

loc_001FE5E3: ;
    ecx = MEM32(0x8F7DFC);
    esi = MEM32(0xC0CF14);
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO16(eax, MEM16(ebx + -44));
    ecx = ecx << 0xF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edx = esp + 0x14;
    PUSH32(esp, edx);
    eax = eax & 0x7FFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax = eax | ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    eax = eax << 2;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    MEM32(esp + 0x18) = eax;
    eax = MEM32(esi + 4);
    PUSH32(esp, eax);
    PUSH32(esp, esi);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001FE615u); RECOMP_ABI_CALL(0x00222DB0u, sub_00222DB0); /* call 0x00222DB0 */

loc_001FE615: ;
    ecx = 1;
    edx = 0xC0CF10;
    MEM32(esp + 0x14) = eax;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001FE628u); RECOMP_ABI_CALL(0x00222DE0u, sub_00222DE0); /* call 0x00222DE0 */

loc_001FE628: ;
    eax = MEM32(esp + 0x14);
    MEM32(esi + 4) = eax;
    ecx = MEM32(eax + 4);
    MEM32(ecx) = eax;
    MEM32(0x8F7DFC) = MEM32(0x8F7DFC) + 1;
    _fa = (uint32_t)(MEM32(0x8F7DFC)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_001FE63A: ;
    eax = ZX16(MEM16(ebx + 0x166));
    _fb = (uint32_t)(2) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - 2;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    if ((_fa == 0)) goto loc_001FE653; /* je: equal / zero */

loc_001FE646: ;
    _fb = (uint32_t)(2) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - 2;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    if ((_fa != 0)) goto loc_001FE6A4; /* jne: not equal / not zero */

loc_001FE64B: ;
    MEM16(ebx + 0x2C) = 0x1DD;
    goto loc_001FE659;

loc_001FE653: ;
    MEM16(ebx + 0x2C) = 0x1E1;

loc_001FE659: ;
    SET_LO16(ecx, MEM16(ebx + 0x170));
    xmm0 = XMM_SCALAR(MEMF(ebx + 0x44)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0xD6AF08); /* mulss */
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO16(edx, MEM16(ebx + 0x16E));
    _fb = (uint32_t)(MEM16(esp + 0x18)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(edx, LO16(edx) + MEM16(esp + 0x18));
    _fa = (uint32_t)(LO16(edx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    _fb = (uint32_t)(0xFC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edi = edi + 0xFC;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = ebx + 0x24;
    MEM32(eax) = edi;
    _fb = (uint32_t)(0x78) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(ecx, LO16(ecx) + 0x78);
    _fa = (uint32_t)(LO16(ecx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    edi = eax;
    MEMF(ebx + 0x44) = xmm0.f[0]; /* movss */
    MEM16(ebx + 0x2A) = LO16(ecx);
    _fb = (uint32_t)(0x86) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edx = edx + 0x86;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM16(ebx + 0x28) = LO16(edx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001FE6A4u); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_001FE6A4: ;
    edx = MEM32(esp + 0x20);
    eax = MEM32(esp + edx * 4 + 0x30);
    xmm0.f[0] = (float)(int32_t)MEM32(esp + eax * 4 + 0x3C); /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * MEMF(ebx + -32); /* mulss */
    MEMF(esp + 0x2C) = xmm0.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x2C), 0x0E7Fu, 32); /* cvttss2si */
    esi = MEM32(esp + 0x18);
    ecx = MEM32(0x8F7DFC);
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esi = esi - eax;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    eax = MEM32(esp + 0x1C);
    edx++;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, eax (32-bit) */
    MEM32(esp + 0x18) = esi;
    MEM32(esp + 0x20) = edx;
    if (CMP_L(_fas, _fbs)) goto loc_001FE460; /* jl: less (signed <) */

loc_001FE6EC: ;
    SET_LO16(edx, MEM16(ebx + 0x172));
    xmm1 = XMM_SCALAR(MEMF(ebx + 0x15C)); /* movss */
    _fb = (uint32_t)(0xA8) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(edx, LO16(edx) + 0xA8 + HITS_SHIFT);   /* #238: the European release's place */
    _fa = (uint32_t)(LO16(edx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    MEM16(ebx + -4) = LO16(edx);
    SET_LO16(edx, MEM16(ebx + 0x174));
    _fb = (uint32_t)(0x7B) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(edx, LO16(edx) + 0x7B);
    _fa = (uint32_t)(LO16(edx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    MEM16(ebx + -2) = LO16(edx);
    _fa = (uint32_t)(MEM32(0x43B490)) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0x43B490), 1 (32-bit) */
    eax = ebx + -8;
    xmm1.f[0] = xmm1.f[0] * xmm0.f[0]; /* mulss */
    xmm1.f[0] = xmm1.f[0] * MEMF(0xD6AF08); /* mulss */
    MEM32(eax) = 0xFB;
    MEM32(ebx + 0x20) = 0xFFFFFFFFu;
    MEM16(ebx) = 0x1E0;
    MEMF(ebx + 0x18) = xmm1.f[0]; /* movss */
    if (CMP_NE(_fa, _fb)) goto loc_001FE7D9; /* jne: not equal / not zero */

loc_001FE746: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(0x8F7DF8)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, MEM32(0x8F7DF8) (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_001FE7D9; /* jae: above or equal (unsigned >=) */

loc_001FE752: ;
    edi = MEM32(0x8F7DF4);
    esi = ecx;
    { int64_t _r = (int64_t)(int32_t)(esi) * (int64_t)(int32_t)(0x70);
      esi = (uint32_t)_r;
      _fa = (uint32_t)_r & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); _fb = (_r != (int64_t)(int32_t)_r); } /* imul */
    _fb = (uint32_t)(edi) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + edi;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edi = MEM32(0x8F7DF0);
    ecx = ecx << 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, 0);
    PUSH32(esp, 0);
    _fb = (uint32_t)(edi) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + edi;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001FE773u); RECOMP_ICALL_SAFE(0x002216B0u, _icall_esp); /* manual call 0x002216B0 */
    }

loc_001FE773: ;
    ecx = MEM32(0x8F7DFC);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001FE7D1; /* je: equal / zero */

loc_001FE780: ;
    esi = MEM32(0xC0CF14);
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO16(eax, MEM16(ebx));
    ecx = ecx << 0xF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edx = esp + 0x14;
    PUSH32(esp, edx);
    eax = eax & 0x7FFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax = eax | ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    eax = eax << 2;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    MEM32(esp + 0x18) = eax;
    eax = MEM32(esi + 4);
    PUSH32(esp, eax);
    PUSH32(esp, esi);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001FE7ABu); RECOMP_ABI_CALL(0x00222DB0u, sub_00222DB0); /* call 0x00222DB0 */

loc_001FE7AB: ;
    ecx = 1;
    edx = 0xC0CF10;
    edi = eax;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001FE7BCu); RECOMP_ABI_CALL(0x00222DE0u, sub_00222DE0); /* call 0x00222DE0 */

loc_001FE7BC: ;
    MEM32(esi + 4) = edi;
    ecx = MEM32(edi + 4);
    MEM32(ecx) = edi;
    ecx = MEM32(0x8F7DFC);
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x8F7DFC) = ecx;

loc_001FE7D1: ;
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */

loc_001FE7D9: ;
    SET_LO8(eax, MEM8(ebx + 0x176));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001FE8B7; /* je: equal / zero */

loc_001FE7E7: ;
    edx = MEM32(ebx + 0x160);
    xmm1 = XMM_SCALAR(MEMF(ebx + 0x15C)); /* movss */
    MEM32(ebx + 0x20) = edx;
    _fa = (uint32_t)(MEM32(0x43B490)) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0x43B490), 1 (32-bit) */
    eax = ebx + -8;
    xmm1.f[0] = xmm1.f[0] * xmm0.f[0]; /* mulss */
    xmm1.f[0] = xmm1.f[0] * MEMF(0xD6AF08); /* mulss */
    MEM32(eax) = 0x106;
    MEM16(ebx) = 0x1DF;
    MEMF(ebx + 0x18) = xmm1.f[0]; /* movss */
    if (CMP_NE(_fa, _fb)) goto loc_001FE8B7; /* jne: not equal / not zero */

loc_001FE824: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(0x8F7DF8)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, MEM32(0x8F7DF8) (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_001FE8B7; /* jae: above or equal (unsigned >=) */

loc_001FE830: ;
    edi = MEM32(0x8F7DF4);
    esi = ecx;
    { int64_t _r = (int64_t)(int32_t)(esi) * (int64_t)(int32_t)(0x70);
      esi = (uint32_t)_r;
      _fa = (uint32_t)_r & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); _fb = (_r != (int64_t)(int32_t)_r); } /* imul */
    _fb = (uint32_t)(edi) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + edi;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edi = MEM32(0x8F7DF0);
    ecx = ecx << 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, 0);
    PUSH32(esp, 0);
    _fb = (uint32_t)(edi) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + edi;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001FE851u); RECOMP_ICALL_SAFE(0x002216B0u, _icall_esp); /* manual call 0x002216B0 */
    }

loc_001FE851: ;
    ecx = MEM32(0x8F7DFC);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001FE8AF; /* je: equal / zero */

loc_001FE85E: ;
    esi = MEM32(0xC0CF14);
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO16(eax, MEM16(ebx));
    ecx = ecx << 0xF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edx = esp + 0x14;
    PUSH32(esp, edx);
    eax = eax & 0x7FFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax = eax | ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    eax = eax << 2;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    MEM32(esp + 0x18) = eax;
    eax = MEM32(esi + 4);
    PUSH32(esp, eax);
    PUSH32(esp, esi);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001FE889u); RECOMP_ABI_CALL(0x00222DB0u, sub_00222DB0); /* call 0x00222DB0 */

loc_001FE889: ;
    ecx = 1;
    edx = 0xC0CF10;
    edi = eax;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001FE89Au); RECOMP_ABI_CALL(0x00222DE0u, sub_00222DE0); /* call 0x00222DE0 */

loc_001FE89A: ;
    MEM32(esi + 4) = edi;
    ecx = MEM32(edi + 4);
    MEM32(ecx) = edi;
    ecx = MEM32(0x8F7DFC);
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x8F7DFC) = ecx;

loc_001FE8AF: ;
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */

loc_001FE8B7: ;
    eax = ZX16(MEM16(ebx + 0x166));
    _fb = (uint32_t)(2) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - 2;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    if ((_fa == 0)) goto loc_001FE91A; /* je: equal / zero */

loc_001FE8C3: ;
    _fb = (uint32_t)(2) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - 2;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    if ((_fa != 0)) goto loc_001FE9DC; /* jne: not equal / not zero */

loc_001FE8CC: ;
    SET_LO16(edx, MEM16(ebx + 0x172));
    SET_LO16(eax, MEM16(ebx + 0x174));
    xmm0 = XMM_SCALAR(MEMF(0xD6AF08)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(ebx + 0x70); /* mulss */
    edi = ebx + 0x50;
    _fb = (uint32_t)(0xA8) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(edx, LO16(edx) + 0xA8 + HITS_SHIFT);   /* #238: the European release's place */
    _fa = (uint32_t)(LO16(edx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    _fb = (uint32_t)(0x7B) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(eax, LO16(eax) + 0x7B);
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    MEM32(edi) = 0x106;
    MEM8(ebx + 0x5A) = 0x11;
    MEM16(ebx + 0x54) = LO16(edx);
    MEM16(ebx + 0x56) = LO16(eax);
    MEM16(ebx + 0x58) = 0x1DD;
    MEMF(ebx + 0x70) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001FE915u); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_001FE915: ;
    goto loc_001FE9CE;

loc_001FE91A: ;
    SET_LO16(ecx, MEM16(ebx + 0x172));
    SET_LO16(edx, MEM16(ebx + 0x174));
    xmm0 = XMM_SCALAR(MEMF(0xD6AF08)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(ebx + 0x70); /* mulss */
    edi = ebx + 0x50;
    _fb = (uint32_t)(0xA8) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(ecx, LO16(ecx) + 0xA8 + HITS_SHIFT);   /* #238: the European release's place */
    _fa = (uint32_t)(LO16(ecx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    _fb = (uint32_t)(0x7B) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(edx, LO16(edx) + 0x7B);
    _fa = (uint32_t)(LO16(edx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    MEM32(edi) = 0x106;
    MEM8(ebx + 0x5A) = 0x11;
    MEM16(ebx + 0x54) = LO16(ecx);
    MEM16(ebx + 0x56) = LO16(edx);
    MEM16(ebx + 0x58) = 0x1E1;
    MEMF(ebx + 0x70) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001FE963u); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_001FE963: ;
    eax = MEM32(esp + 0x1C);
    SET_LO16(edx, MEM16(ebx + 0x174));
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO16(ecx, MEM16(esp + eax * 2 + 0x62));
    _fb = (uint32_t)(MEM16(ebx + 0x172)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(ecx, LO16(ecx) + MEM16(ebx + 0x172));
    _fa = (uint32_t)(LO16(ecx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    xmm0.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x414AE8); /* mulss */
    xmm0.f[0] = xmm0.f[0] + MEMF(0x414B2C); /* addss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x415240); /* mulss */
    MEMF(ebx + 0x90) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x4148C4)); /* movss */
    MEMF(ebx + 0x94) = xmm0.f[0]; /* movss */
    _fb = (uint32_t)(0x49) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 0x49;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(0x78) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(edx, LO16(edx) + 0x78);
    _fa = (uint32_t)(LO16(edx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    MEM16(ebx + 0x80) = LO16(ecx);
    MEM16(ebx + 0x82) = LO16(edx);
    MEM16(ebx + 0x84) = 0x1E2;

loc_001FE9CE: ;
    xmm0 = XMM_SCALAR(MEMF(0xD6AF04)); /* movss */
    ecx = MEM32(0x8F7DFC);

loc_001FE9DC: ;
    eax = MEM32(esp + 0x24);
    _fb = (uint32_t)(0x1AC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ebx = ebx + 0x1AC;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    MEM32(esp + 0x24) = eax;
    if ((_fa != 0)) goto loc_001FE3D0; /* jne: not equal / not zero */

loc_001FE9F1: ;
    ecx = MEM32(esp + 0x6C);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001FE9FAu); RECOMP_ABI_CALL(0x00379C29u, sub_00379C29); /* call 0x00379C29 */

loc_001FE9FA: ;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebx);
    esp = ebp;
    POP32(esp, ebp);
    esp += 4; return; /* ret */

}

/**
 * sub_001FB230
 * Original: 0x001FB230 - 0x001FB2B4 (132 bytes, 35 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_001FB230(void)
{
    RECOMP_ENTRY(0x001FB230u);
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_001FB230: ;
    /* The half-black square behind the Ninpo orb (#272), sub_001FA3F0's second call (returning to 0x1FA70F;
     * the first is the bar's backing): the game puts it 10 right and 19 down of the bar's corner, 29 square
     * (the first stack word across, ecx down, edx wide, the second word high), in the 720 x 480 HUD -- (60,50) to
     * (90,80) as drawn, lower and further left than the frame's hole (65..89 x 52..76), and its corner below the
     * frame's left edge showed as black pixels on a bright background (the original game's too). Cut to (64,50)
     * to (90,78): the hole still covered, all of it under the frame's opaque part. */
    if (MEM32(esp) == 0x001FA70Fu) {
        MEM32(esp + 4) += 4;
        edx -= 4;
        MEM32(esp + 8) -= 2;
    }
    _fb = (uint32_t)(0x28) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x28;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x4148C0); /* mulss */
    eax = 0xFF000000u;
    MEM32(esp + 0xC) = eax;
    MEM32(esp + 8) = eax;
    MEM32(esp + 4) = eax;
    SET_LO16(eax, MEM16(esp + 0x2C));
    PUSH32(esp, ebx);
    MEM16(esp + 0x14) = LO16(eax);
    SET_LO16(eax, MEM16(esp + 0x34));
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    MEM16(esp + 0x22) = LO16(eax);
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    PUSH32(esp, ecx);
    edi = esp + 0x10;
    MEM16(esp + 0x22) = LO16(ecx);
    MEM16(esp + 0x24) = LO16(edx);
    MEM8(esp + 0x2C) = LO8(eax);
    MEM32(esp + 0x30) = eax;
    MEM32(esp + 0x34) = eax;
    MEM16(esp + 0x28) = LO16(eax);
    MEM32(esp + 0x10) = 0xBF000000u;
    MEM16(esp + 0x2A) = 0x1CC;
    MEMF(esp) = xmm0.f[0]; /* movss */
    PUSH32(esp, 0x001FB29Eu); RECOMP_ABI_CALL(0x00175860u, sub_00175860); /* call 0x00175860 */

loc_001FB29E: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esi = edi;
    ebx = 0x8F7DF0;
    PUSH32(esp, 0x001FB2ADu); RECOMP_ABI_CALL(0x00222250u, sub_00222250); /* call 0x00222250 */

loc_001FB2AD: ;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebx);
    _fb = (uint32_t)(0x28) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x28;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 4; return; /* ret */

}

/**
 * sub_001FB8D0
 * Original: 0x001FB8D0 - 0x001FB981 (177 bytes, 46 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: standard_frame
 */
void sub_001FB8D0(void)
{
    RECOMP_ENTRY(0x001FB8D0u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* prologue saves caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_001FB8D0: ;
    PUSH32(esp, ebp);
    ebp = esp;
    g_ebp = ebp; /* publish frame for frameless callees */
    g_seh_ebp = ebp;
    esp = esp & 0xFFFFFFF0u;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fb = (uint32_t)(0x38) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x38;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(MEM16(ebp + 8)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), MEM16(ebp + 8) (16-bit) */
    xmm1 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    PUSH32(esp, esi);
    esi = MEM32(ebp + 0xC);
    PUSH32(esp, edi);
    if (CMP_AE(_fa, _fb)) goto loc_001FB8F4; /* jae: above or equal (unsigned >=) */

loc_001FB8EC: ;
    xmm1 = XMM_SCALAR(MEMF(0x414BB4)); /* movss */

loc_001FB8F4: ;
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    SET_LO8(eax, 1);
    MEM8(esp + 0x1F) = LO8(eax);
    MEM8(esp + 0x20) = LO8(eax);
    SET_LO8(eax, MEM8(0xBCDF78));
    MEM32(esp + 0x14) = ecx;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(2) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 2 (8-bit) */
    MEM16(esp + 0x1C) = 0x1CD;
    MEM8(esp + 0x1E) = LO8(ecx);
    MEM32(esp + 0x24) = ecx;
    MEMF(esp + 0x38) = xmm0.f[0]; /* movss */
    MEM32(esp + 0x3C) = 0xFFFFFFFFu;
    if (CMP_NE(_fa, _fb)) goto loc_001FB93D; /* jne: not equal / not zero */

loc_001FB92D: ;
    xmm0 = XMM_SCALAR(MEMF(0x4149D8)); /* movss */
    _fb = (uint32_t)(0xFFFFFFFCu) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + 0xFFFFFFFCu;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(0xFFFFFFFDu) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edx = edx + 0xFFFFFFFDu;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_001FB94B;

loc_001FB93D: ;
    xmm0 = XMM_SCALAR(MEMF(0x414BF4)); /* movss */
    _fb = (uint32_t)(9) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + 9;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(6) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edx = edx + 6;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    /* HUD Aspect Corrected (setting 42): the Ninpo icon a pixel left (#272). The game draws it 1.35 pixels of
     * the 720 x 480 HUD right of the middle of the frame's hole (the icon's disc at 78.85 across, the hole's
     * at 77.5); in their own shape that shows, the icon against the hole's right edge. Stretched as the game
     * draws it. */
    if (ngb_pc_setting(42))
        esi -= 1;

loc_001FB94B: ;
    MEMF(esp + 0x28) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x2C) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(ebp + 0x10)); /* movss */
    edi = esp + 0x14;
    MEM16(esp + 0x18) = LO16(esi);
    MEM16(esp + 0x1A) = LO16(edx);
    MEMF(esp + 0x34) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x30) = xmm1.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001FB97Bu); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_001FB97B: ;
    POP32(esp, edi);
    POP32(esp, esi);
    esp = ebp;
    POP32(esp, ebp);
    esp += 4; return; /* ret */

}

