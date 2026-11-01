/*
 * The D3D library's GPU interrupt handler (sub_003CF980, sub_003CFAA0, sub_003CFEB0), its read of how far the GPU
 * has got (sub_003CA920) and the USB controller's interrupt handling for the pads (sub_003DFE3D, sub_003E0511),
 * lifted (src/manual_functions.json) with their memory accesses through gpu_regs.h: they reach the devices'
 * registers through pointers, and through MEM32 each was a fault -- 4,000 a second on a phone, some 20 us each. The guest stack's are left as they were; anything else not on the registers is plain memory still.
 */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"
#include "gpu_regs.h"

/**
 * sub_003CA920
 * Original: 0x003CA920 - 0x003CA99A (122 bytes, 50 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_003CA920(void)
{
    RECOMP_ENTRY(0x003CA920u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    int _cf = 0; /* carry flag */

loc_003CA920: ;
    eax = GR32(ecx + 0x1C20);
    eax = GR32(eax + 0x44);
    edx = GR32(ecx + 0x24);
    _cf = 0; /* logical op clears CF */
    eax = eax | 0x80000000u;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edx (32-bit) */
    _cf = (int)(_fa < _fb);
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    if (CMP_B(_fa, _fb)) goto loc_003CA952; /* jb: below (unsigned <) */

loc_003CA937: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(GR32(ecx + 0x28)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, GR32(ecx + 0x28) (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_AE(_fa, _fb)) goto loc_003CA952; /* jae: above or equal (unsigned >=) */

loc_003CA93C: ;
    edi = GR32(ecx);
    esi = GR32(ecx + 0x40);
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, eax (32-bit) */
    _cf = (int)(_fa < _fb);
    edx = _cf ? 0xFFFFFFFF : 0; _fb = 0; /* sbb self (CF extend), OF clear */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sbb result */
    _cf = (int)((edx) != 0);
    edx = (uint32_t)(-(int32_t)edx);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* neg result */
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(esi) < (uint32_t)(edx));
    esi = esi - edx;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    POP32(esp, edi);
    GW32(ecx + 0x60, esi);
    GW32(ecx + 0x5C, eax);
    POP32(esp, esi);
    esp += 4; return; /* ret */

loc_003CA952: ;
    esi = GR32(ecx + 0x40);
    eax = GR32(ecx + 0x934);
    eax = GR32(eax + 0x400B10);
    edx = esi;
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(edx) < (uint32_t)(eax));
    edx = edx - eax;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    if (5) _cf = (int)(((eax) >> ((5) - 1)) & 1);
    eax = eax >> 5;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ebx);
    ebx = GR32(ecx + 0x60);
    edi = esi;
    _cf = 0; /* logical op clears CF */
    eax = eax & 0x7FFFFFC;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fb = (uint32_t)(ebx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(edi) < (uint32_t)(ebx));
    edi = edi - ebx;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _cf = 0; /* logical op clears CF */
    edx = edx & 3;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _cf = 0; /* logical op clears CF */
    eax = eax | 0x80000000u;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, edx (32-bit) */
    _cf = (int)(_fa < _fb);
    POP32(esp, ebx);
    if (CMP_B(_fa, _fb)) goto loc_003CA994; /* jb: below (unsigned <) */

loc_003CA982: ;
    if (CMP_NE(_fa, _fb)) goto loc_003CA989; /* jne: not equal / not zero */

loc_003CA984: ;
    _fa = (uint32_t)(GR32(ecx + 0x5C)) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp GR32(ecx + 0x5C), eax (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_A(_fa, _fb)) goto loc_003CA994; /* ja: above (unsigned >) */

loc_003CA989: ;
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(esi) < (uint32_t)(edx));
    esi = esi - edx;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    POP32(esp, edi);
    GW32(ecx + 0x60, esi);
    GW32(ecx + 0x5C, eax);
    POP32(esp, esi);
    esp += 4; return; /* ret */

loc_003CA994: ;
    eax = GR32(ecx + 0x5C);
    POP32(esp, edi);
    POP32(esp, esi);
    esp += 4; return; /* ret */

}

/**
 * sub_003CF980
 * Original: 0x003CF980 - 0x003CFA97 (279 bytes, 72 insns)
 * Category: game_render
 * CC: cdecl, 2 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_003CF980(void)
{
    RECOMP_ENTRY(0x003CF980u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003CF980: ;
    PUSH32(esp, esi);
    esi = MEM32(esp + 0xC);
    eax = GR32(esi + 0xA0);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    PUSH32(esp, edi);
    edi = GR32(esi);
    if (CMP_EQ(_fa, _fb)) goto loc_003CFA90; /* je: equal / zero */

loc_003CF996: ;
    eax = GR32(edi + 0x140);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CFA90; /* je: equal / zero */

loc_003CF9A4: ;
    ecx = GR32(esi + 8);
    ecx = ecx & 0xC0000000u;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x80000000u) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x80000000u (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CF9C8; /* jne: not equal / not zero */

loc_003CF9B5: ;
    _fa = (uint32_t)(GR32(esi + 0x1B4)) & 0xFFFFFFFFu; _fb = (uint32_t)(0x200000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test GR32(esi + 0x1B4), 0x200000 (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003CF9C8; /* je: equal / zero */

loc_003CF9C1: ;
    eax = 1;
    goto loc_003CF9CA;

loc_003CF9C8: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */

loc_003CF9CA: ;
    edx = GR32(esi + 0xA4);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CFA6E; /* jne: not equal / not zero */

loc_003CF9D8: ;
    _fa = (uint32_t)(GR32(esi + 0x1B4)) & 0xFFFFFFFFu; _fb = (uint32_t)(0x1000000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test GR32(esi + 0x1B4), 0x1000000 (32-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003CF9EC; /* jne: not equal / not zero */

loc_003CF9E4: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CFA6E; /* je: equal / zero */

loc_003CF9EC: ;
    _fa = (uint32_t)(GR32(edi + 0x100)) & 0xFFFFFFFFu; _fb = (uint32_t)(0x1000000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test GR32(edi + 0x100), 0x1000000 (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003CFA6E; /* je: equal / zero */

loc_003CF9F8: ;
    SET_LO16(edx, 0x80C0);
    SET_LO8(eax, xbox_PortIn((edx & 0xFFFFu), 1)); /* in */
    eax = eax >> 5;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    eax = ~eax;
    eax = eax & 1;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x40000000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x40000000 (32-bit) */
    GW32(esi + 0x1D0, eax);
    if (CMP_NE(_fa, _fb)) goto loc_003CFA20; /* jne: not equal / not zero */

loc_003CFA13: ;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    SET_LO8(ecx, (CMP_EQ(_fa, _fb)) ? 1 : 0); /* sete */
    GW32(esi + 0x1D0, ecx);

loc_003CFA20: ;
    eax = GR32(esi + 0x1BC);
    eax = eax & 1;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    edx = eax + eax * 2 + 0x5D;
    GW32(esi + 0xA4, 1);
    _fa = (uint32_t)(GR32(esi + edx * 4)) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp GR32(esi + edx * 4), 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CFA61; /* je: equal / zero */

loc_003CFA3D: ;
    ecx = GR32(esi + 0x1C0);
    eax = eax + eax * 2;
    edx = GR32(esi + eax * 4 + 0x178);
    eax = esi + eax * 4;
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, ecx (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CFA61; /* jne: not equal / not zero */

loc_003CFA55: ;
    edx = GR32(eax + 0x17C);
    GW32(0x3D9590, edx);

loc_003CFA61: ;
    ecx = GR32(0x3D9590);
    eax = esi;
    PUSH32(esp, 0x003CFA6Eu); RECOMP_ABI_CALL(0x003D0408u, sub_003D0408); /* call 0x003D0408 */

loc_003CFA6E: ;
    PUSH32(esp, 0);
    PUSH32(esp, 0);
    _fb = (uint32_t)(0x84) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + 0x84;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, esi);
    GW32(edi + 0x140, 0);
    { uint32_t _icall_esp = g_esp;
    { uint32_t _icall_target = GR32(0x3E15BC); PUSH32(esp, 0x003CFA89u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x003CFA83u); } /* indirect call */
    }

loc_003CFA89: ;
    POP32(esp, edi);
    SET_LO8(eax, 1);
    POP32(esp, esi);
    esp += 12; return; /* ret 8 */

loc_003CFA90: ;
    POP32(esp, edi);
    SET_LO8(eax, 0); /* xor self */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* xor result */
    POP32(esp, esi);
    esp += 12; return; /* ret 8 */

}

/**
 * sub_003CFAA0
 * Original: 0x003CFAA0 - 0x003CFB76 (214 bytes, 57 insns)
 * Category: game_render
 * CC: cdecl, 4 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_003CFAA0(void)
{
    RECOMP_ENTRY(0x003CFAA0u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003CFAA0: ;
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    PUSH32(esp, esi);
    esi = MEM32(esp + 0x14);
    ebp = GR32(esi);
    PUSH32(esp, edi);
    /* nop */

loc_003CFAB0: ;
    edi = GR32(ebp + 0x100);
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0x1000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test edi, 0x1000 (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003CFAC9; /* je: equal / zero */

loc_003CFAC0: ;
    ecx = esi;
    PUSH32(esp, 0x003CFAC7u); RECOMP_ABI_CALL(0x003CFEB0u, sub_003CFEB0); /* call 0x003CFEB0 */

loc_003CFAC7: ;
    ebx = eax;

loc_003CFAC9: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFFEFFFu) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test edi, 0xFFFFEFFFu (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003CFB55; /* je: equal / zero */

loc_003CFAD5: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0x100000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test edi, 0x100000 (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003CFAE4; /* je: equal / zero */

loc_003CFADD: ;
    PUSH32(esp, 0x003CFAE2u); RECOMP_ABI_CALL(0x003CFD60u, sub_003CFD60); /* call 0x003CFD60 */

loc_003CFAE2: ;
    ebx = ebx | eax;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */

loc_003CFAE4: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0x1000000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test edi, 0x1000000 (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003CFB28; /* je: equal / zero */

loc_003CFAEC: ;
    eax = GR32(esi + 0x820);
    eax = eax | GR32(esi + 0x824);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    if ((_fa != 0)) goto loc_003CFB10; /* jne: not equal / not zero */

loc_003CFAFA: ;
    eax = esi;
    PUSH32(esp, 0x003CFB01u); RECOMP_ABI_CALL(0x003CFC50u, sub_003CFC50); /* call 0x003CFC50 */

loc_003CFB01: ;
    ebx = ebx | eax;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    goto loc_003CFB28;

    goto loc_003CFB10;

    /* nop */
    edi = edi;

loc_003CFB10: ;
    GW32(ebp + 0x600100, 1);
    edi = GR32(ebp + 0x100);
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0x1000000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test edi, 0x1000000 (32-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003CFB10; /* jne: not equal / not zero */

loc_003CFB28: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0x100) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test edi, 0x100 (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003CFB38; /* je: equal / zero */

loc_003CFB30: ;
    PUSH32(esp, esi);
    PUSH32(esp, 0x003CFB36u); RECOMP_ABI_CALL(0x003CFF70u, sub_003CFF70); /* call 0x003CFF70 */

loc_003CFB36: ;
    ebx = ebx | eax;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */

loc_003CFB38: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0x10000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test edi, 0x10000 (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003CFB55; /* je: equal / zero */

loc_003CFB40: ;
    eax = GR32(esi);
    _fa = (uint32_t)(MEM8(eax + 0x8100)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test MEM8(eax + 0x8100), 1 (8-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003CFB55; /* je: equal / zero */

loc_003CFB4B: ;
    GW32(eax + 0x8100, 1);

loc_003CFB55: ;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CFAB0; /* jne: not equal / not zero */

loc_003CFB5D: ;
    ecx = GR32(esi + 0xB4);
    POP32(esp, edi);
    GW32(esi + 0xA4, ebx);
    POP32(esp, esi);
    GW32(ebp + 0x140, ecx);
    POP32(esp, ebp);
    POP32(esp, ebx);
    esp += 20; return; /* ret 16 */

}

/**
 * sub_003CFEB0
 * Original: 0x003CFEB0 - 0x003CFF6A (186 bytes, 55 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_003CFEB0(void)
{
    RECOMP_ENTRY(0x003CFEB0u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003CFEB0: ;
    PUSH32(esp, ecx);
    PUSH32(esp, ebx);
    ebx = ecx;
    PUSH32(esp, esi);
    esi = GR32(ebx);
    GW32(esi + 0x400720, 0);
    eax = GR32(esi + 0x400100);
    PUSH32(esp, edi);
    edi = GR32(esi + 0x400704);
    ecx = GR32(esi + 0x400108);
    edi = edi & 0x1FFC;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(HI8(eax)) & 0xFFu; _fb = (uint32_t)(0x10) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(eax), 0x10 (8-bit) */
    MEM32(esp + 0xC) = ecx;
    if (TEST_Z(_fa, _fb)) goto loc_003CFF1E; /* je: equal / zero */

loc_003CFEE3: ;
    eax = edi;
    eax = eax >> 0x14;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    GW32(esi + 0x400100, 0x1000);
    ecx = GR32(ebx);
    eax = eax & 0x1F;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    goto loc_003CFF00;

    /* nop */

loc_003CFF00: ;
    edx = GR32(ecx + 0x400700);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) { RECOMP_SPIN(0x003CFF00u); goto loc_003CFF00; } /* jne: not equal / not zero */

loc_003CFF0A: ;
    PUSH32(esp, eax);
    PUSH32(esp, 0x003CFF10u); RECOMP_ABI_CALL(0x003D3307u, sub_003D3307); /* call 0x003D3307 */

loc_003CFF10: ;
    eax = GR32(esi + 0x400100);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CFF55; /* je: equal / zero */

loc_003CFF1A: ;
    ecx = MEM32(esp + 0xC);

loc_003CFF1E: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    GW32(esi + 0x400100, eax);
    if (CMP_EQ(_fa, _fb)) goto loc_003CFF55; /* je: equal / zero */

loc_003CFF28: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x100001) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test eax, 0x100001 (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003CFF55; /* je: equal / zero */

loc_003CFF2F: ;
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0x40) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ecx), 0x40 (8-bit) */
    eax = GR32(esi + 0x400708);
    if (TEST_NZ(_fa, _fb)) goto loc_003CFF55; /* jne: not equal / not zero */

loc_003CFF3A: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0x100) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0x100 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CFF4B; /* jne: not equal / not zero */

loc_003CFF42: ;
    ecx = ebx;
    PUSH32(esp, 0x003CFF49u); RECOMP_ABI_CALL(0x003D0100u, sub_003D0100); /* call 0x003D0100 */

loc_003CFF49: ;
    goto loc_003CFF55;

loc_003CFF4B: ;
    PUSH32(esp, eax);
    PUSH32(esp, edi);
    PUSH32(esp, ecx);
    PUSH32(esp, 0);
    PUSH32(esp, 0x003CFF55u); RECOMP_ABI_CALL(0x003D03C0u, sub_003D03C0); /* call 0x003D03C0 */

loc_003CFF55: ;
    GW32(esi + 0x400720, 1);
    eax = GR32(esi + 0x400100);
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebx);
    POP32(esp, ecx);
    esp += 4; return; /* ret */

}

/**
 * sub_003DFE3D
 * Original: 0x003DFE3D - 0x003DFED3 (150 bytes, 51 insns)
 * CC: cdecl, 2 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_003DFE3D(void)
{
    RECOMP_ENTRY(0x003DFE3Du);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003DFE3D: ;
    PUSH32(esp, esi);
    esi = MEM32(esp + 0xC);
    ecx = GR32(esi);
    GW32(0x45E9C0, MEM32(0x45E9C0) + 1);
    _fa = (uint32_t)(GR32(0x45E9C0)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    eax = GR32(ecx + 0x10);
    edx = GR32(ecx + 0xC);
    edx = edx & eax;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    PUSH32(esp, edi);
    if ((_fa == 0)) goto loc_003DFECC; /* je: equal / zero */

loc_003DFE55: ;
    edi = 0x80000000u;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test edi, eax (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003DFECC; /* je: equal / zero */

loc_003DFE5E: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(LO8(edx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(eax), LO8(edx) (8-bit) */
    GW32(ecx + 0x14, edi);
    if (TEST_Z(_fa, _fb)) goto loc_003DFE6E; /* je: equal / zero */

loc_003DFE68: ;
    GW32(ecx + 0xC, eax);
    edx = edx & 0xFFFFFFFEu;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */

loc_003DFE6E: ;
    _fa = (uint32_t)(LO8(edx)) & 0xFFu; _fb = (uint32_t)(0x20) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(edx), 0x20 (8-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003DFEA2; /* je: equal / zero */

loc_003DFE73: ;
    PUSH32(esp, ebx);
    ebx = GR32(esi + 8);
    ebx = ZX16(MEM16(ebx + 0x80));
    edi = esi + 0x418;
    eax = GR32(edi);
    ebx = ebx ^ eax;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    ebx = ebx & 0x8000;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fb = (uint32_t)(ebx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - ebx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fb = (uint32_t)(0x10000) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x10000;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    GW32(edi, eax);
    GW32(ecx + 0xC, 0x20);
    edx = edx & 0xFFFFFFDFu;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    POP32(esp, ebx);

loc_003DFEA2: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003DFEBF; /* je: equal / zero */

loc_003DFEA6: ;
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, 0);
    GW32(esi + 0x438, edx);
    PUSH32(esp, 0);
    _fb = (uint32_t)(0x440) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + 0x440;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, esi);
    { uint32_t _icall_target = GR32(0x3E15BC); PUSH32(esp, 0x003DFEBDu); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x003DFEB7u); } /* indirect call */
    }

loc_003DFEBD: ;
    goto loc_003DFEC8;

loc_003DFEBF: ;
    eax = GR32(esi);
    GW32(eax + 0x10, 0x80000000u);

loc_003DFEC8: ;
    SET_LO8(eax, 1);
    goto loc_003DFECE;

loc_003DFECC: ;
    SET_LO8(eax, 0); /* xor self */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* xor result */

loc_003DFECE: ;
    POP32(esp, edi);
    POP32(esp, esi);
    esp += 12; return; /* ret 8 */

}

/**
 * sub_003E0511
 * Original: 0x003E0511 - 0x003E0627 (278 bytes, 96 insns)
 * Category: game_input
 * CC: cdecl, 4 params, returns int_or_void
 * Frame: standard_frame
 */
void sub_003E0511(void)
{
    RECOMP_ENTRY(0x003E0511u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* prologue saves caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003E0511: ;
    PUSH32(esp, ebp);
    ebp = esp;
    g_ebp = ebp; /* publish frame for frameless callees */
    g_seh_ebp = ebp;
    PUSH32(esp, ecx);
    PUSH32(esp, ecx);
    PUSH32(esp, ebx);
    PUSH32(esp, esi);
    esi = GR32(ebp + 0xC);
    ebx = GR32(esi);
    eax = esi + 0x438;
    ecx = GR32(eax);
    PUSH32(esp, edi);
    GW32(ebp + -4, ecx);
    edi = 0; /* xor self */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    ecx = esi;
    MEM8(ebp + 0xF) = 0;
    GW32(eax, edi);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x003E0538u); RECOMP_ABI_CALL(0x003DFEF7u, sub_003DFEF7); /* call 0x003DFEF7 */

loc_003E0538: ;
    _fa = (uint32_t)(MEM8(ebp + -4)) & 0xFFu; _fb = (uint32_t)(2) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test MEM8(ebp + -4), 2 (8-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003E0595; /* je: equal / zero */

loc_003E053E: ;
    ecx = GR32(esi + 8);
    _fb = (uint32_t)(0x84) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 0x84;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = GR32(ecx);
    eax = eax & 0xFFFFFFF0u;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    GW32(ecx, edi);
    if ((_fa == 0)) goto loc_003E0581; /* je: equal / zero */

loc_003E0550: ;
    ecx = GR32(0xC0EC40);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = GR32(ecx + 8);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    GW32(ecx + 8, edi);
    edi = ecx;
    if (CMP_NE(_fa, _fb)) goto loc_003E0550; /* jne: not equal / not zero */

loc_003E0564: ;
    edx = edi;
    _fa = (uint32_t)(MEM8(edx + 2)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test MEM8(edx + 2), 1 (8-bit) */
    edi = GR32(edi + 8);
    ecx = esi;
    if (TEST_Z(_fa, _fb)) goto loc_003E0578; /* je: equal / zero */

loc_003E0571: ;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x003E0576u); RECOMP_ABI_CALL(0x003E0B93u, sub_003E0B93); /* call 0x003E0B93 */

loc_003E0576: ;
    goto loc_003E057D;

loc_003E0578: ;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x003E057Du); RECOMP_ABI_CALL(0x003E01ACu, sub_003E01AC); /* call 0x003E01AC */

loc_003E057D: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003E0564; /* jne: not equal / not zero */

loc_003E0581: ;
    GW32(ebp + -4, MEM32(ebp + -4) & 0xFFFFFFFDu);
    _fa = (uint32_t)(GR32(ebp + -4)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    GW32(ebx + 0xC, 2);
    eax = GR32(esi);
    GW32(eax + 8, 6);

loc_003E0595: ;
    _fa = (uint32_t)(MEM8(ebp + -4)) & 0xFFu; _fb = (uint32_t)(4) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test MEM8(ebp + -4), 4 (8-bit) */
    PUSH32(esp, 4);
    POP32(esp, edi);
    if (TEST_Z(_fa, _fb)) goto loc_003E05A8; /* je: equal / zero */

loc_003E059E: ;
    GW32(ebp + -4, MEM32(ebp + -4) & 0xFFFFFFFBu);
    _fa = (uint32_t)(GR32(ebp + -4)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    GW32(ebx + 0xC, edi);
    GW32(ebx + 0x14, edi);

loc_003E05A8: ;
    ecx = esi;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x003E05AFu); RECOMP_ABI_CALL(0x003DFED3u, sub_003DFED3); /* call 0x003DFED3 */

loc_003E05AF: ;
    edx = eax;
    ecx = esi;
    GW32(ebp + -8, edx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x003E05BBu); RECOMP_ABI_CALL(0x003E0258u, sub_003E0258); /* call 0x003E0258 */

loc_003E05BB: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003E05C3; /* je: equal / zero */

loc_003E05BF: ;
    MEM8(ebp + 0xF) = 1;

loc_003E05C3: ;
    edx = GR32(ebp + -8);
    ecx = esi;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x003E05CDu); RECOMP_ABI_CALL(0x003E03B0u, sub_003E03B0); /* call 0x003E03B0 */

loc_003E05CD: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003E05D5; /* je: equal / zero */

loc_003E05D1: ;
    MEM8(ebp + 0xF) = 1;

loc_003E05D5: ;
    edx = GR32(ebp + -8);
    ecx = esi;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x003E05DFu); RECOMP_ABI_CALL(0x003E0437u, sub_003E0437); /* call 0x003E0437 */

loc_003E05DF: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003E05E7; /* je: equal / zero */

loc_003E05E3: ;
    MEM8(ebp + 0xF) = 1;

loc_003E05E7: ;
    _fa = (uint32_t)(MEM8(ebp + 0xF)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(ebp + 0xF), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003E05F7; /* je: equal / zero */

loc_003E05ED: ;
    eax = GR32(esi);
    GW32(eax + 0xC, edi);
    eax = GR32(esi);
    GW32(eax + 0x10, edi);

loc_003E05F7: ;
    _fa = (uint32_t)(MEM8(ebp + -4)) & 0xFFu; _fb = (uint32_t)(0x40) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test MEM8(ebp + -4), 0x40 (8-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003E060F; /* je: equal / zero */

loc_003E05FD: ;
    ecx = esi;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x003E0604u); RECOMP_ABI_CALL(0x003DE3F0u, sub_003DE3F0); /* call 0x003DE3F0 */

loc_003E0604: ;
    GW32(ebp + -4, MEM32(ebp + -4) & 0xFFFFFFBFu);
    _fa = (uint32_t)(GR32(ebp + -4)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    GW32(ebx + 0xC, 0x40);

loc_003E060F: ;
    eax = GR32(ebp + -4);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003E0619; /* je: equal / zero */

loc_003E0616: ;
    GW32(ebx + 0xC, eax);

loc_003E0619: ;
    POP32(esp, edi);
    POP32(esp, esi);
    GW32(ebx + 0x10, 0x80000000u);
    POP32(esp, ebx);
    esp = ebp;
    POP32(esp, ebp); /* leave */
    esp += 20; return; /* ret 16 */

}

