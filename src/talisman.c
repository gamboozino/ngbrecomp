/*
 * talisman.c -- the Talisman of Rebirth equipped from the pause menu's item page (#148, #223).
 *
 * The game counts the Talisman of Rebirth among the items it lists but gives it no Select: it only acts by
 * itself as Ryu falls. Here the item page's button hint (sub_00212C20, the lifted function with the change
 * marked "PC:", listed in src/manual_functions.json) gives it Select / Back, and src/pause_tab.c turns A on it
 * into equipping or unequipping it -- the setting src/cheats.c's sub_002FD3E0 reads (43, settings.ini
 * talisman_of_rebirth, on until unequipped) -- with "Equipped" or "Not Equipped" by its count.
 */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"

void ngb_talisman_item_page(void);   /* src/pause_tab.c */

/**
 * sub_00212C20
 * Original: 0x00212C20 - 0x00212FBB (923 bytes, 260 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_00212C20(void)
{
    RECOMP_ENTRY(0x00212C20u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_00212C20: ;
    _fb = (uint32_t)(0x5C) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x5C;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO16(edx, MEM16(ecx + 0x18));
    ecx = (uint32_t)(int32_t)SMEM16(ecx + 0x1A);
    _fb = (uint32_t)(0x297) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(edx, LO16(edx) + 0x297);
    _fa = (uint32_t)(LO16(edx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    _fb = (uint32_t)(0x19F) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    SET_LO16(ecx, LO16(ecx) + 0x19F);
    _fa = (uint32_t)(LO16(ecx)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */
    PUSH32(esp, esi);
    esi = eax;
    _fa = (uint32_t)(MEM16(esi + 4)) & 0xFFFFu; _fb = (uint32_t)(1) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(esi + 4), 1 (16-bit) */
    SET_LO16(eax, MEM16(esi + 6));
    MEM32(esp + 8) = edx;
    MEM32(esp + 4) = ecx;
    if (CMP_NE(_fa, _fb)) goto loc_00212CF6; /* jne: not equal / not zero */

loc_00212C51: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x18) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x18 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212CAB; /* je: equal / zero */

loc_00212C57: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x19) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x19 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212CAB; /* je: equal / zero */

loc_00212C5D: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x1A) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x1A (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212CAB; /* je: equal / zero */

loc_00212C63: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x1B) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x1B (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212CAB; /* je: equal / zero */

loc_00212C69: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x26) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x26 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212CAB; /* je: equal / zero */

loc_00212C6F: ;
    _fa = (uint32_t)(MEM16(0x8DB8B8)) & 0xFFFFu; _fb = (uint32_t)(0x27) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(0x8DB8B8), 0x27 (16-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00212C8B; /* jne: not equal / not zero */

loc_00212C79: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(eax, MEM8(0xBCE030));
    PUSH32(esp, eax);
    eax = 0x38;
    goto loc_00212F59;

loc_00212C8B: ;
    eax = MEM32(0x8DB8B0);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212F10; /* je: equal / zero */

loc_00212C98: ;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(ecx, MEM8(0xBCE030));
    eax = 0x3B;
    PUSH32(esp, ecx);
    goto loc_00212F59;

loc_00212CAB: ;
    eax = ZX16(LO16(eax));
    _fb = (uint32_t)(0x1A) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - 0x1A;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    if ((_fa == 0)) goto loc_00212CE4; /* je: equal / zero */

loc_00212CB3: ;
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa == 0)) goto loc_00212CD1; /* je: equal / zero */

loc_00212CB6: ;
    _fb = (uint32_t)(0xB) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - 0xB;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    if ((_fa == 0)) goto loc_00212EB6; /* je: equal / zero */

loc_00212CBF: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(eax, MEM8(0xBCE030));
    PUSH32(esp, eax);
    eax = 0x3B;
    goto loc_00212F59;

loc_00212CD1: ;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(edx, MEM8(0xBCE030));
    eax = 0x45;
    PUSH32(esp, edx);
    goto loc_00212F59;

loc_00212CE4: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(eax, MEM8(0xBCE030));
    PUSH32(esp, eax);
    eax = 0x43;
    goto loc_00212F59;

loc_00212CF6: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x18) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x18 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212F20; /* je: equal / zero */

loc_00212D00: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x19) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x19 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212F20; /* je: equal / zero */

loc_00212D0A: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x1A) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x1A (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212F20; /* je: equal / zero */

loc_00212D14: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x1B) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x1B (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212F26; /* je: equal / zero */

loc_00212D1E: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x26) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x26 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212F20; /* je: equal / zero */

loc_00212D28: ;
    SET_LO16(eax, MEM16(0x8DB8B8));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(4) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 4 (16-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00212D5D; /* jne: not equal / not zero */

loc_00212D34: ;
    _fa = (uint32_t)(MEM16(0xD6AAA8)) & 0xFFFFu; _fb = (uint32_t)(0) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(0xD6AAA8), 0 (16-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00212D85; /* jne: not equal / not zero */

loc_00212D3E: ;
    eax = MEM32(0x8DB93C);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212EC9; /* je: equal / zero */

loc_00212D4B: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(eax, MEM8(0xBCE030));
    PUSH32(esp, eax);
    eax = 0x38;
    goto loc_00212F59;

loc_00212D5D: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(8) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 8 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212D3E; /* je: equal / zero */

loc_00212D63: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(6) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 6 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212D3E; /* je: equal / zero */

loc_00212D69: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0xC) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0xC (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212D3E; /* je: equal / zero */

loc_00212D6F: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0xD) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0xD (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212D3E; /* je: equal / zero */

loc_00212D75: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0xE) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0xE (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212D3E; /* je: equal / zero */

loc_00212D7B: ;
    _fa = (uint32_t)(MEM16(0xD6AAA8)) & 0xFFFFu; _fb = (uint32_t)(0) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(0xD6AAA8), 0 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212DD7; /* je: equal / zero */

loc_00212D85: ;
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    PUSH32(esp, edi);
    edi = MEM32(0x8DBA14);
    PUSH32(esp, 0x00212D93u); RECOMP_ABI_CALL(0x00155390u, sub_00155390); /* call 0x00155390 */

loc_00212D93: ;
    ecx = edi;
    SET_LO8(ebx, LO8(eax));
    PUSH32(esp, 0x00212D9Cu); RECOMP_ABI_CALL(0x00155760u, sub_00155760); /* call 0x00155760 */

loc_00212D9C: ;
    ecx = edi;
    ebp = eax;
    esi = 1;
    PUSH32(esp, 0x00212DAAu); RECOMP_ABI_CALL(0x00155720u, sub_00155720); /* call 0x00155720 */

loc_00212DAA: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00212DB0; /* jne: not equal / not zero */

loc_00212DAE: ;
    esi = 0; /* xor self */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */

loc_00212DB0: ;
    _fa = (uint32_t)(LO8(ebx)) & 0xFFu; _fb = (uint32_t)(0xFF) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ebx), 0xFF (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212DB9; /* je: equal / zero */

loc_00212DB5: ;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, 0 (32-bit) */
    if (CMP_GE(_fas, _fbs)) goto loc_00212DBB; /* jge: greater or equal (signed >=) */

loc_00212DB9: ;
    esi = 0; /* xor self */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */

loc_00212DBB: ;
    ecx = ZX8(LO8(ebx));
    POP32(esp, edi);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(ebp) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, ebp (32-bit) */
    POP32(esp, ebp);
    POP32(esp, ebx);
    if (CMP_EQ(_fa, _fb)) goto loc_00212D4B; /* je: equal / zero */

loc_00212DC5: ;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212D4B; /* je: equal / zero */

loc_00212DC9: ;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(edx, MEM8(0xBCE030));
    PUSH32(esp, edx);
    goto loc_00212F54;

loc_00212DD7: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(7) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 7 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212F10; /* je: equal / zero */

loc_00212DE1: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(9) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 9 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212F10; /* je: equal / zero */

loc_00212DEB: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0xA) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0xA (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212F10; /* je: equal / zero */

loc_00212DF5: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x27) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x27 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212F10; /* je: equal / zero */

loc_00212DFF: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0xB) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0xB (16-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00212E23; /* jne: not equal / not zero */

loc_00212E05: ;
    _fa = (uint32_t)(MEM16(esi + 8)) & 0xFFFFu; _fb = (uint32_t)(5) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(esi + 8), 5 (16-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00212F10; /* jne: not equal / not zero */

loc_00212E10: ;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(ecx, MEM8(0xBCE030));
    eax = 0x3E;
    PUSH32(esp, ecx);
    goto loc_00212F59;

loc_00212E23: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(3) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 3 (16-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00212E5D; /* jne: not equal / not zero */

loc_00212E29: ;
    SET_LO16(eax, MEM16(esi + 8));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(3) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 3 (16-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00212E45; /* jne: not equal / not zero */

loc_00212E33: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(eax, MEM8(0xBCE030));
    PUSH32(esp, eax);
    eax = 0x3A;
    goto loc_00212F59;

loc_00212E45: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(2) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 2 (16-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00212F10; /* jne: not equal / not zero */

loc_00212E4F: ;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(ecx, MEM8(0xBCE030));
    PUSH32(esp, ecx);
    goto loc_00212F54;

loc_00212E5D: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(1) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 1 (16-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00212EAB; /* jne: not equal / not zero */

loc_00212E63: ;
    SET_LO16(eax, MEM16(esi + 8));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(3) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 3 (16-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00212E7F; /* jne: not equal / not zero */

loc_00212E6D: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(eax, MEM8(0xBCE030));
    PUSH32(esp, eax);
    eax = 0x3C;
    goto loc_00212F59;

loc_00212E7F: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(4) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 4 (16-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00212E98; /* jne: not equal / not zero */

loc_00212E85: ;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(ecx, MEM8(0xBCE030));
    eax = 0x38;
    PUSH32(esp, ecx);
    goto loc_00212F59;

loc_00212E98: ;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(edx, MEM8(0xBCE030));
    eax = 0x3D;
    PUSH32(esp, edx);
    goto loc_00212F59;

loc_00212EAB: ;
    _fa = (uint32_t)(MEM16(esi + 8)) & 0xFFFFu; _fb = (uint32_t)(2) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(esi + 8), 2 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212F4C; /* je: equal / zero */

loc_00212EB6: ;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(ecx, MEM8(0xBCE030));
    eax = 0x38;
    PUSH32(esp, ecx);
    goto loc_00212F59;

loc_00212EC9: ;
    eax = MEM32(0x8DB938);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212D4B; /* je: equal / zero */

loc_00212ED6: ;
    eax = MEM32(0x8DBA14);
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x6F) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x6F (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212F10; /* je: equal / zero */

loc_00212EE1: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x70) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x70 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212F10; /* je: equal / zero */

loc_00212EE7: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0xC4) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0xC4 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212F10; /* je: equal / zero */

loc_00212EED: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0xC5) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0xC5 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212F10; /* je: equal / zero */

loc_00212EF3: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0xC6) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0xC6 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212F10; /* je: equal / zero */

loc_00212EF9: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x17F) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x17F (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00212F10; /* je: equal / zero */

loc_00212EFF: ;
    /* PC: the Talisman of Rebirth, left out here (only Back), gets Select: it is equipped from this page
     * (src/pause_tab.c ngb_talisman_item_page, its word and A) */
    if (LO16(eax) == 0xFB && MEM16(0x8DB8B8) == 4) {   /* (4: the item page; elsewhere Back, as the game has it) */
        ngb_talisman_item_page();
        goto loc_00212F05;
    }

loc_00212F05: ;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(ecx, MEM8(0xBCE030));
    PUSH32(esp, ecx);
    goto loc_00212F54;

loc_00212F10: ;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(edx, MEM8(0xBCE030));
    eax = 0x38;
    PUSH32(esp, edx);
    goto loc_00212F59;

loc_00212F20: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x1B) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x1B (16-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00212F36; /* jne: not equal / not zero */

loc_00212F26: ;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(ecx, MEM8(0xBCE030));
    eax = 0x45;
    PUSH32(esp, ecx);
    goto loc_00212F59;

loc_00212F36: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0x1A) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0x1A (16-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00212F4C; /* jne: not equal / not zero */

loc_00212F3C: ;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(edx, MEM8(0xBCE030));
    eax = 0x43;
    PUSH32(esp, edx);
    goto loc_00212F59;

loc_00212F4C: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(eax, MEM8(0xBCE030));
    PUSH32(esp, eax);

loc_00212F54: ;
    eax = 0x39;

loc_00212F59: ;
    PUSH32(esp, 0x00212F5Eu); RECOMP_ABI_CALL(0x001E6AE0u, sub_001E6AE0); /* call 0x001E6AE0 */

loc_00212F5E: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    POP32(esp, esi);
    if (CMP_EQ(_fa, _fb)) goto loc_00212FB7; /* je: equal / zero */

loc_00212F66: ;
    ecx = (uint32_t)(int32_t)SMEM16(esp);
    edx = (uint32_t)(int32_t)SMEM16(esp + 4);
    PUSH32(esp, 2);
    PUSH32(esp, 0x1E0);
    PUSH32(esp, ecx);
    ecx = eax;
    PUSH32(esp, edx);
    eax = esp + 0x18;
    PUSH32(esp, 0x00212F83u); RECOMP_ABI_CALL(0x00168730u, sub_00168730); /* call 0x00168730 */

loc_00212F83: ;
    xmm0 = XMM_SCALAR(MEMF(0x414BD0)); /* movss */
    eax = esp + 0x18;
    MEMF(esp + 0x38) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x34) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x414A70)); /* movss */
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, eax);
    MEM8(esp + 0x66) = 0x22;
    MEMF(esp + 0x30) = xmm0.f[0]; /* movss */
    PUSH32(esp, 0x00212FB4u); RECOMP_ICALL_SAFE(0x001E6D30u, _icall_esp); /* manual call 0x001E6D30 */
    }

loc_00212FB4: ;
    _fb = (uint32_t)(0x14) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x14;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_00212FB7: ;
    _fb = (uint32_t)(0x5C) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x5C;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 4; return; /* ret */

}

