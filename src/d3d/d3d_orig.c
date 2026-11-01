/* The recompiled Direct3D functions, moved here by tools/take_d3d.py:
 * src/d3d/d3d_hooks.c calls them when the hook does not take the call. */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"
#include "gpu_regs.h"

/**
 * sub_003C5260
 * Original: 0x003C5260 - 0x003C529C (60 bytes, 19 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C5260(void)
{
    RECOMP_ENTRY(0x003C5260u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C5260: ;
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C527D; /* jb: below (unsigned <) */

loc_003C526E: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C527Du); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C527D: ;
    ecx = MEM32(esp + 8);
    MEM32(eax) = 0x80320;
    MEM32(eax + 4) = ecx;
    MEM32(eax + 8) = ecx;
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0xC;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi) = eax;
    MEM32(0x3D8634) = ecx;
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C52A0
 * Original: 0x003C52A0 - 0x003C52DF (63 bytes, 19 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C52A0(void)
{
    RECOMP_ENTRY(0x003C52A0u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C52A0: ;
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C52BD; /* jb: below (unsigned <) */

loc_003C52AE: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C52BDu); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C52BD: ;
    ecx = MEM32(esp + 8);
    MEM32(eax) = 0x41E6C;
    edx = ecx + -512;
    MEM32(eax + 4) = edx;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi) = eax;
    MEM32(0x3D8648) = ecx;
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C52E0
 * Original: 0x003C52E0 - 0x003C5337 (87 bytes, 28 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C52E0(void)
{
    RECOMP_ENTRY(0x003C52E0u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C52E0: ;
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    PUSH32(esp, edi);
    if (CMP_B(_fa, _fb)) goto loc_003C52FE; /* jb: below (unsigned <) */

loc_003C52EF: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C52FEu); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C52FE: ;
    ecx = MEM32(esp + 0xC);
    edx = ZX8(MEM8(esp + 0xE));
    edi = ecx;
    edi = edi & 0xFF;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    edi = edi << 0x10;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edx = edx | edi;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    edi = ecx;
    edi = edi & 0xFF00FF00u;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(eax) = 0x402A8;
    edx = edx | edi;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(eax + 4) = edx;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi) = eax;
    POP32(esp, edi);
    MEM32(0x3D8600) = ecx;
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C5340
 * Original: 0x003C5340 - 0x003C53B2 (114 bytes, 35 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C5340(void)
{
    RECOMP_ENTRY(0x003C5340u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C5340: ;
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C535D; /* jb: below (unsigned <) */

loc_003C534E: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C535Du); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C535D: ;
    ecx = MEM32(esp + 8);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    MEM32(eax) = 0x40308;
    if (CMP_NE(_fa, _fb)) goto loc_003C537D; /* jne: not equal / not zero */

loc_003C536B: ;
    MEM32(eax + 4) = ecx;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi) = eax;
    MEM32(0x3D8624) = ecx;
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

loc_003C537D: ;
    MEM32(eax + 4) = 1;
    PUSH32(esp, edi);
    edi = MEM32(0x3D8620);
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, edi (32-bit) */
    SET_LO8(edx, (CMP_NE(_fa, _fb)) ? 1 : 0); /* setne */
    MEM32(eax + 8) = 0x4039C;
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x10;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    POP32(esp, edi);
    _fb = (uint32_t)(0x404) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edx = edx + 0x404;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(eax + -4) = edx;
    MEM32(esi) = eax;
    MEM32(0x3D8624) = ecx;
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C53C0
 * Original: 0x003C53C0 - 0x003C5405 (69 bytes, 20 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C53C0(void)
{
    RECOMP_ENTRY(0x003C53C0u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003C53C0: ;
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C53DD; /* jb: below (unsigned <) */

loc_003C53CE: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C53DDu); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C53DD: ;
    ecx = MEM32(esp + 8);
    MEM32(eax) = 0x403A0;
    MEM32(eax + 4) = ecx;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi) = eax;
    edx = MEM32(0x3D8624);
    MEM32(0x3D8620) = ecx;
    POP32(esp, esi);
    MEM32(esp + 4) = edx;
    g_seh_ebp = ebp; sub_003C5340(); return; /* tail jmp 0x003C5340 */

}

/**
 * sub_003C5410
 * Original: 0x003C5410 - 0x003C5453 (67 bytes, 19 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C5410(void)
{
    RECOMP_ENTRY(0x003C5410u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C5410: ;
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C542D; /* jb: below (unsigned <) */

loc_003C541E: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C542Du); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C542D: ;
    ecx = MEM32(esp + 8);
    MEM32(eax) = 0x403A4;
    MEM32(eax + 4) = ecx;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi) = eax;
    MEM32(0x3D83D0) = MEM32(0x3D83D0) | 0x200;
    _fa = (uint32_t)(MEM32(0x3D83D0)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(0x3D8610) = ecx;
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C5460
 * Original: 0x003C5460 - 0x003C54FE (158 bytes, 49 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C5460(void)
{
    RECOMP_ENTRY(0x003C5460u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C5460: ;
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C547D; /* jb: below (unsigned <) */

loc_003C546E: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C547Du); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C547D: ;
    edx = MEM32(esp + 8);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C5496; /* je: equal / zero */

loc_003C5485: ;
    ecx = MEM32(esi + 0x1A08);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C5496; /* je: equal / zero */

loc_003C548F: ;
    ecx = 1;
    goto loc_003C5498;

loc_003C5496: ;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */

loc_003C5498: ;
    MEM32(eax) = 0x4030C;
    MEM32(eax + 4) = ecx;
    ecx = MEM32(0x3D8520);
    MEM32(eax + 8) = 0x41D78;
    MEM32(eax + 0xC) = ecx;
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x10;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi) = eax;
    eax = MEM32(0x3D8614);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(2) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 2 (32-bit) */
    MEM32(0x3D8614) = edx;
    if (CMP_EQ(_fa, _fb)) goto loc_003C54CB; /* je: equal / zero */

loc_003C54C6: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(2) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 2 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C54FA; /* jne: not equal / not zero */

loc_003C54CB: ;
    PUSH32(esp, 0x003C54D0u); RECOMP_ABI_CALL(0x003C67B0u, sub_003C67B0); /* call 0x003C67B0 */

loc_003C54D0: ;
    PUSH32(esp, 0x003C54D5u); RECOMP_ABI_CALL(0x003C8F10u, sub_003C8F10); /* call 0x003C8F10 */

loc_003C54D5: ;
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C54EB; /* jb: below (unsigned <) */

loc_003C54DC: ;
    eax = MEM32(0x3D9588);
    edx = eax;
    edx = edx >> 1;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, edx);
    PUSH32(esp, 0x003C54EBu); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C54EB: ;
    PUSH32(esp, esi);
    edx = eax;
    PUSH32(esp, 0x003C54F3u); RECOMP_ABI_CALL(0x003C6300u, sub_003C6300); /* call 0x003C6300 */

loc_003C54F3: ;
    PUSH32(esp, 0x003C54F8u); RECOMP_ABI_CALL(0x003C6100u, sub_003C6100); /* call 0x003C6100 */

loc_003C54F8: ;
    MEM32(esi) = eax;

loc_003C54FA: ;
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C5500
 * Original: 0x003C5500 - 0x003C55A2 (162 bytes, 50 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C5500(void)
{
    RECOMP_ENTRY(0x003C5500u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C5500: ;
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    PUSH32(esp, edi);
    edi = MEM32(esp + 0xC);
    MEM32(0x3D8618) = edi;
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C5528; /* jb: below (unsigned <) */

loc_003C5519: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C5528u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C5528: ;
    edx = MEM32(0x3D8660);
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C5539; /* je: equal / zero */

loc_003C5534: ;
    ecx = 2;

loc_003C5539: ;
    edx = MEM32(0x3D865C);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C555C; /* je: equal / zero */

loc_003C5543: ;
    edx = MEM32(0x3D8618);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C5559; /* je: equal / zero */

loc_003C554D: ;
    _fa = (uint32_t)(MEM32(0x3D861C)) & 0xFFFFFFFFu; _fb = (uint32_t)(0x1E00) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0x3D861C), 0x1E00 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C555C; /* jne: not equal / not zero */

loc_003C5559: ;
    ecx = ecx | 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */

loc_003C555C: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0 (32-bit) */
    MEM32(eax) = 0x41D84;
    MEM32(eax + 4) = ecx;
    if (CMP_EQ(_fa, _fb)) goto loc_003C558C; /* je: equal / zero */

loc_003C5569: ;
    ecx = MEM32(esi + 0x1A08);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C558C; /* je: equal / zero */

loc_003C5573: ;
    MEM32(eax + 8) = 0x4032C;
    ecx = 1;
    MEM32(eax + 0xC) = ecx;
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x10;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    POP32(esp, edi);
    MEM32(esi) = eax;
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

loc_003C558C: ;
    MEM32(eax + 8) = 0x4032C;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    MEM32(eax + 0xC) = ecx;
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x10;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    POP32(esp, edi);
    MEM32(esi) = eax;
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C55B0
 * Original: 0x003C55B0 - 0x003C5629 (121 bytes, 36 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C55B0(void)
{
    RECOMP_ENTRY(0x003C55B0u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C55B0: ;
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    PUSH32(esp, edi);
    edi = MEM32(esp + 0xC);
    MEM32(0x3D861C) = edi;
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C55D8; /* jb: below (unsigned <) */

loc_003C55C9: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C55D8u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C55D8: ;
    edx = MEM32(0x3D8660);
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C55E9; /* je: equal / zero */

loc_003C55E4: ;
    ecx = 2;

loc_003C55E9: ;
    edx = MEM32(0x3D865C);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C560C; /* je: equal / zero */

loc_003C55F3: ;
    edx = MEM32(0x3D8618);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C5609; /* je: equal / zero */

loc_003C55FD: ;
    _fa = (uint32_t)(MEM32(0x3D861C)) & 0xFFFFFFFFu; _fb = (uint32_t)(0x1E00) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0x3D861C), 0x1E00 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C560C; /* jne: not equal / not zero */

loc_003C5609: ;
    ecx = ecx | 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */

loc_003C560C: ;
    MEM32(eax) = 0x41D84;
    MEM32(eax + 4) = ecx;
    MEM32(eax + 8) = 0x40370;
    MEM32(eax + 0xC) = edi;
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x10;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    POP32(esp, edi);
    MEM32(esi) = eax;
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C5630
 * Original: 0x003C5630 - 0x003C5692 (98 bytes, 30 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C5630(void)
{
    RECOMP_ENTRY(0x003C5630u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C5630: ;
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    eax = MEM32(esi + 0x784);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C5684; /* jne: not equal / not zero */

loc_003C5641: ;
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C5657; /* jb: below (unsigned <) */

loc_003C5648: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C5657u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C5657: ;
    ecx = MEM32(esp + 8);
    MEM32(eax) = 0x400A60;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 4;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edx = 0x10;
    /* nop */

loc_003C5670: ;
    MEM32(eax) = ecx;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 4;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edx--;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa != 0)) goto loc_003C5670; /* jne: not equal / not zero */

loc_003C5678: ;
    MEM32(esi) = eax;
    MEM32(0x3D8628) = ecx;
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

loc_003C5684: ;
    edx = MEM32(esp + 8);
    MEM32(0x3D8628) = edx;
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C56A0
 * Original: 0x003C56A0 - 0x003C570F (111 bytes, 34 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C56A0(void)
{
    RECOMP_ENTRY(0x003C56A0u);
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

loc_003C56A0: ;
    PUSH32(esp, ebx);
    ebx = MEM32(esp + 8);
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    MEM32(esp + 0xC) = ebx;
    fp_push(MEMF(esp + 0xC)); /* fld float */
    PUSH32(esp, edi);
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esi + 0x964)); /* fmul dword ptr [esi + 0x964] */
    PUSH32(esp, ecx);
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x3E9E08)); /* fmul dword ptr [0x3e9e08] */
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C0)); /* fadd dword ptr [0x4148c0] */
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003C56D0u); RECOMP_ABI_CALL(0x003CC3C0u, sub_003CC3C0); /* call 0x003CC3C0 */

loc_003C56D0: ;
    edi = eax;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0x1FF) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0x1FF (32-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_003C56DF; /* jbe: below or equal (unsigned <=) */

loc_003C56DA: ;
    edi = 0x1FF;

loc_003C56DF: ;
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C56F5; /* jb: below (unsigned <) */

loc_003C56E6: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C56F5u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C56F5: ;
    MEM32(eax) = 0x40380;
    MEM32(eax + 4) = edi;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    POP32(esp, edi);
    MEM32(esi) = eax;
    POP32(esp, esi);
    MEM32(0x3D864C) = ebx;
    POP32(esp, ebx);
    esp += 8; return; /* ret 4 */

    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}

/**
 * sub_003C5710
 * Original: 0x003C5710 - 0x003C5791 (129 bytes, 42 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C5710(void)
{
    RECOMP_ENTRY(0x003C5710u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C5710: ;
    PUSH32(esp, ebx);
    ebx = MEM32(esp + 8);
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    edi = MEM32(0x3D83C8);
    eax = MEM32(edi + 0x1A04);
    ecx = ZX8(MEM8(eax + 0xD));
    SET_LO8(edx, MEM8(ecx + 0x3D4B68));
    SET_LO8(edx, LO8(edx) & 0x3C);
    _fa = (uint32_t)(LO8(edx)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* and result */
    esi = 0; /* xor self */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(LO8(edx)) & 0xFFu; _fb = (uint32_t)(0x20) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(edx), 0x20 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C5739; /* jne: not equal / not zero */

loc_003C5737: ;
    esi = ebx;

loc_003C5739: ;
    eax = MEM32(edi + 8);
    ecx = eax;
    ecx = ecx & 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, ecx (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C5785; /* je: equal / zero */

loc_003C5745: ;
    ecx = MEM32(edi + 4);
    eax = eax ^ 1;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    MEM32(edi + 8) = eax;
    eax = MEM32(edi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ecx (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C5763; /* jb: below (unsigned <) */

loc_003C5754: ;
    eax = MEM32(0x3D9588);
    edx = eax;
    edx = edx >> 1;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, edx);
    PUSH32(esp, 0x003C5763u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C5763: ;
    MEM32(eax) = 0x40110;
    MEM32(eax + 4) = 0;
    esi = esi << 5;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    MEM32(eax + 8) = 0x40100;
    esi = esi | 8;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(eax + 0xC) = esi;
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x10;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(edi) = eax;

loc_003C5785: ;
    POP32(esp, edi);
    POP32(esp, esi);
    MEM32(0x3D8654) = ebx;
    POP32(esp, ebx);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C57A0
 * Original: 0x003C57A0 - 0x003C57D4 (52 bytes, 18 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C57A0(void)
{
    RECOMP_ENTRY(0x003C57A0u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C57A0: ;
    eax = MEM32(esp + 4);
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    MEM32(0x3D8658) = eax;
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C57C6; /* jb: below (unsigned <) */

loc_003C57B7: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C57C6u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C57C6: ;
    PUSH32(esp, esi);
    edx = eax;
    PUSH32(esp, 0x003C57CEu); RECOMP_ABI_CALL(0x003C6300u, sub_003C6300); /* call 0x003C6300 */

loc_003C57CE: ;
    MEM32(esi) = eax;
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C57E0
 * Original: 0x003C57E0 - 0x003C584C (108 bytes, 32 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C57E0(void)
{
    RECOMP_ENTRY(0x003C57E0u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C57E0: ;
    eax = MEM32(esp + 4);
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    MEM32(0x3D865C) = eax;
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C5806; /* jb: below (unsigned <) */

loc_003C57F7: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C5806u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C5806: ;
    edx = MEM32(0x3D8660);
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C5817; /* je: equal / zero */

loc_003C5812: ;
    ecx = 2;

loc_003C5817: ;
    edx = MEM32(0x3D865C);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C583A; /* je: equal / zero */

loc_003C5821: ;
    edx = MEM32(0x3D8618);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C5837; /* je: equal / zero */

loc_003C582B: ;
    _fa = (uint32_t)(MEM32(0x3D861C)) & 0xFFFFFFFFu; _fb = (uint32_t)(0x1E00) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0x3D861C), 0x1E00 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C583A; /* jne: not equal / not zero */

loc_003C5837: ;
    ecx = ecx | 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */

loc_003C583A: ;
    MEM32(eax) = 0x41D84;
    MEM32(eax + 4) = ecx;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi) = eax;
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C5850
 * Original: 0x003C5850 - 0x003C58BC (108 bytes, 32 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C5850(void)
{
    RECOMP_ENTRY(0x003C5850u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C5850: ;
    eax = MEM32(esp + 4);
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    MEM32(0x3D8660) = eax;
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C5876; /* jb: below (unsigned <) */

loc_003C5867: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C5876u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C5876: ;
    edx = MEM32(0x3D8660);
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C5887; /* je: equal / zero */

loc_003C5882: ;
    ecx = 2;

loc_003C5887: ;
    edx = MEM32(0x3D865C);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C58AA; /* je: equal / zero */

loc_003C5891: ;
    edx = MEM32(0x3D8618);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C58A7; /* je: equal / zero */

loc_003C589B: ;
    _fa = (uint32_t)(MEM32(0x3D861C)) & 0xFFFFFFFFu; _fb = (uint32_t)(0x1E00) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0x3D861C), 0x1E00 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C58AA; /* jne: not equal / not zero */

loc_003C58A7: ;
    ecx = ecx | 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */

loc_003C58AA: ;
    MEM32(eax) = 0x41D84;
    MEM32(eax + 4) = ecx;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi) = eax;
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C58C0
 * Original: 0x003C58C0 - 0x003C58D1 (17 bytes, 4 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C58C0(void)
{
    RECOMP_ENTRY(0x003C58C0u);

loc_003C58C0: ;
    eax = MEM32(esp + 4);
    MEM32(0x3D8664) = eax;
    PUSH32(esp, 0x003C58CEu); RECOMP_ABI_CALL(0x003C63B0u, sub_003C63B0); /* call 0x003C63B0 */

loc_003C58CE: ;
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C58E0
 * Original: 0x003C58E0 - 0x003C58F1 (17 bytes, 4 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C58E0(void)
{
    RECOMP_ENTRY(0x003C58E0u);

loc_003C58E0: ;
    eax = MEM32(esp + 4);
    MEM32(0x3D8668) = eax;
    PUSH32(esp, 0x003C58EEu); RECOMP_ABI_CALL(0x003C63B0u, sub_003C63B0); /* call 0x003C63B0 */

loc_003C58EE: ;
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C5900
 * Original: 0x003C5900 - 0x003C5911 (17 bytes, 4 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C5900(void)
{
    RECOMP_ENTRY(0x003C5900u);

loc_003C5900: ;
    eax = MEM32(esp + 4);
    MEM32(0x3D866C) = eax;
    PUSH32(esp, 0x003C590Eu); RECOMP_ABI_CALL(0x003C63B0u, sub_003C63B0); /* call 0x003C63B0 */

loc_003C590E: ;
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C5920
 * Original: 0x003C5920 - 0x003C59C8 (168 bytes, 48 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C5920(void)
{
    RECOMP_ENTRY(0x003C5920u);
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

loc_003C5920: ;
    PUSH32(esp, ecx);
    PUSH32(esp, ebx);
    PUSH32(esp, esi);
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    PUSH32(esp, edi);
    edi = MEM32(esp + 0x14);
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0 (32-bit) */
    SET_LO8(eax, (CMP_NE(_fa, _fb)) ? 1 : 0); /* setne */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0 (32-bit) */
    MEM32(esp + 0x14) = edi;
    fp_push((double)SMEM32(esp + 0x14)); /* fild */
    esi = eax;
    if (CMP_GE(_fas, _fbs)) goto loc_003C5943; /* jge: greater or equal (signed >=) */

loc_003C593D: ;
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148E4)); /* fadd dword ptr [0x4148e4] */

loc_003C5943: ;
    fp_top() = -fp_top(); /* fchs */
    ecx = 0x40384;
    MEMF(esp + 0x14) = (float)fp_top(); /* fst */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x4149C0)); /* fmul dword ptr [0x4149c0] */
    MEMF(esp + 0xC) = (float)fp_top(); fp_pop(); /* fstp */
    ebx = MEM32(esp + 0xC);
    edx = ebx;
    PUSH32(esp, 0x003C5963u); RECOMP_ABI_CALL(0x003C5C60u, sub_003C5C60); /* call 0x003C5C60 */

loc_003C5963: ;
    ecx = MEM32(esp + 0x14);
    MEM32(0x3D850C) = ebx;
    ebx = ecx;
    MEM32(esp + 0x14) = ecx;
    edx = ebx;
    ecx = 0x40388;
    PUSH32(esp, 0x003C597Fu); RECOMP_ABI_CALL(0x003C5C60u, sub_003C5C60); /* call 0x003C5C60 */

loc_003C597F: ;
    edx = esi;
    ecx = 0x40330;
    MEM32(0x3D8510) = ebx;
    PUSH32(esp, 0x003C5991u); RECOMP_ABI_CALL(0x003C5C60u, sub_003C5C60); /* call 0x003C5C60 */

loc_003C5991: ;
    edx = esi;
    ecx = 0x40334;
    MEM32(0x3D8514) = esi;
    PUSH32(esp, 0x003C59A3u); RECOMP_ABI_CALL(0x003C5C60u, sub_003C5C60); /* call 0x003C5C60 */

loc_003C59A3: ;
    edx = esi;
    ecx = 0x40338;
    MEM32(0x3D8518) = esi;
    PUSH32(esp, 0x003C59B5u); RECOMP_ABI_CALL(0x003C5C60u, sub_003C5C60); /* call 0x003C5C60 */

loc_003C59B5: ;
    MEM32(0x3D862C) = edi;
    POP32(esp, edi);
    MEM32(0x3D851C) = esi;
    POP32(esp, esi);
    POP32(esp, ebx);
    POP32(esp, ecx);
    esp += 8; return; /* ret 4 */

    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}

/**
 * sub_003C59D0
 * Original: 0x003C59D0 - 0x003C59FD (45 bytes, 9 insns)
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C59D0(void)
{
    RECOMP_ENTRY(0x003C59D0u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003C59D0: ;
    ecx = MEM32(esp + 4);
    eax = MEM32(0x3D83C8);
    MEM32(0x3D8640) = ecx;
    edx = MEM32(eax + 0x1A04);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(eax + 0x1A14)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, MEM32(eax + 0x1A14) (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C59FA; /* jne: not equal / not zero */

loc_003C59ED: ;
    MEM32(esp + 4) = 0;
    g_seh_ebp = ebp; sub_003C6490(); return; /* tail jmp 0x003C6490 */

loc_003C59FA: ;
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C5A00
 * Original: 0x003C5A00 - 0x003C5A2D (45 bytes, 9 insns)
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C5A00(void)
{
    RECOMP_ENTRY(0x003C5A00u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003C5A00: ;
    ecx = MEM32(esp + 4);
    eax = MEM32(0x3D83C8);
    MEM32(0x3D8644) = ecx;
    edx = MEM32(eax + 0x1A04);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(eax + 0x1A14)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, MEM32(eax + 0x1A14) (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C5A2A; /* je: equal / zero */

loc_003C5A1D: ;
    MEM32(esp + 4) = 0;
    g_seh_ebp = ebp; sub_003C6490(); return; /* tail jmp 0x003C6490 */

loc_003C5A2A: ;
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C5A30
 * Original: 0x003C5A30 - 0x003C5A6F (63 bytes, 21 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C5A30(void)
{
    RECOMP_ENTRY(0x003C5A30u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C5A30: ;
    eax = MEM32(esp + 4);
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    PUSH32(esp, edi);
    MEM32(0x3D8638) = eax;
    PUSH32(esp, 0x003C5A46u); RECOMP_ABI_CALL(0x003C8F10u, sub_003C8F10); /* call 0x003C8F10 */

loc_003C5A46: ;
    edi = esi;
    PUSH32(esp, 0x003C5A4Du); RECOMP_ABI_CALL(0x003C6360u, sub_003C6360); /* call 0x003C6360 */

loc_003C5A4D: ;
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C5A63; /* jb: below (unsigned <) */

loc_003C5A54: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C5A63u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C5A63: ;
    PUSH32(esp, 0x003C5A68u); RECOMP_ABI_CALL(0x003C6100u, sub_003C6100); /* call 0x003C6100 */

loc_003C5A68: ;
    POP32(esp, edi);
    MEM32(esi) = eax;
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C5A70
 * Original: 0x003C5A70 - 0x003C5A89 (25 bytes, 7 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C5A70(void)
{
    RECOMP_ENTRY(0x003C5A70u);

loc_003C5A70: ;
    eax = MEM32(esp + 4);
    PUSH32(esp, edi);
    edi = MEM32(0x3D83C8);
    MEM32(0x3D863C) = eax;
    PUSH32(esp, 0x003C5A85u); RECOMP_ABI_CALL(0x003C6360u, sub_003C6360); /* call 0x003C6360 */

loc_003C5A85: ;
    POP32(esp, edi);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C5A90
 * Original: 0x003C5A90 - 0x003C5AA9 (25 bytes, 7 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C5A90(void)
{
    RECOMP_ENTRY(0x003C5A90u);

loc_003C5A90: ;
    eax = MEM32(esp + 4);
    PUSH32(esp, edi);
    edi = MEM32(0x3D83C8);
    MEM32(0x3D8650) = eax;
    PUSH32(esp, 0x003C5AA5u); RECOMP_ABI_CALL(0x003C6360u, sub_003C6360); /* call 0x003C6360 */

loc_003C5AA5: ;
    POP32(esp, edi);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C5AB0
 * Original: 0x003C5AB0 - 0x003C5B0C (92 bytes, 28 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C5AB0(void)
{
    RECOMP_ENTRY(0x003C5AB0u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C5AB0: ;
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C5ACD; /* jb: below (unsigned <) */

loc_003C5ABE: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C5ACDu); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C5ACD: ;
    ecx = MEM32(esp + 8);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C5AED; /* jne: not equal / not zero */

loc_003C5AD5: ;
    MEM32(eax) = 0x417BC;
    MEM32(eax + 4) = ecx;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi) = eax;
    MEM32(0x3D8630) = ecx;
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

loc_003C5AED: ;
    MEM32(eax) = 0x817BC;
    MEM32(eax + 4) = 1;
    MEM32(eax + 8) = ecx;
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0xC;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi) = eax;
    MEM32(0x3D8630) = ecx;
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C5B10
 * Original: 0x003C5B10 - 0x003C5B5E (78 bytes, 24 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C5B10(void)
{
    RECOMP_ENTRY(0x003C5B10u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C5B10: ;
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C5B2D; /* jb: below (unsigned <) */

loc_003C5B1E: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C5B2Du); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C5B2D: ;
    ecx = MEM32(0x3D860C);
    edx = MEM32(0x3D8608);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    ecx = MEM32(esp + 8);
    if (CMP_NE(_fa, _fb)) goto loc_003C5B43; /* jne: not equal / not zero */

loc_003C5B41: ;
    edx = ecx;

loc_003C5B43: ;
    MEM32(eax) = 0x8038C;
    MEM32(eax + 4) = ecx;
    MEM32(eax + 8) = edx;
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0xC;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi) = eax;
    MEM32(0x3D8604) = ecx;
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C5B60
 * Original: 0x003C5B60 - 0x003C5B78 (24 bytes, 5 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C5B60(void)
{
    RECOMP_ENTRY(0x003C5B60u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003C5B60: ;
    eax = MEM32(esp + 4);
    ecx = MEM32(0x3D8604);
    MEM32(0x3D8608) = eax;
    MEM32(esp + 4) = ecx;
    g_seh_ebp = ebp; sub_003C5B10(); return; /* tail jmp 0x003C5B10 */

}

/**
 * sub_003C5B80
 * Original: 0x003C5B80 - 0x003C5BD3 (83 bytes, 23 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C5B80(void)
{
    RECOMP_ENTRY(0x003C5B80u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003C5B80: ;
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C5B9D; /* jb: below (unsigned <) */

loc_003C5B8E: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C5B9Du); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C5B9D: ;
    ecx = MEM32(esp + 8);
    MEM32(eax) = 0x417C4;
    MEM32(eax + 4) = ecx;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    MEM32(esi) = eax;
    POP32(esp, esi);
    if (CMP_EQ(_fa, _fb)) goto loc_003C5BBE; /* je: equal / zero */

loc_003C5BB4: ;
    MEM32(0x3D83D0) = MEM32(0x3D83D0) | 0x9000;
    _fa = (uint32_t)(MEM32(0x3D83D0)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */

loc_003C5BBE: ;
    edx = MEM32(0x3D8604);
    MEM32(0x3D860C) = ecx;
    MEM32(esp + 4) = edx;
    g_seh_ebp = ebp; sub_003C5B10(); return; /* tail jmp 0x003C5B10 */

}

/**
 * sub_003C5BE0
 * Original: 0x003C5BE0 - 0x003C5C2D (77 bytes, 22 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C5BE0(void)
{
    RECOMP_ENTRY(0x003C5BE0u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C5BE0: ;
    edx = MEM32(0x3D83D0);
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    eax = MEM32(esi);
    ecx = MEM32(esi + 4);
    edx = edx | 0x200;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ecx (32-bit) */
    MEM32(0x3D83D0) = edx;
    if (CMP_B(_fa, _fb)) goto loc_003C5C11; /* jb: below (unsigned <) */

loc_003C5C02: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C5C11u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C5C11: ;
    ecx = MEM32(esp + 8);
    MEM32(eax) = 0x40328;
    MEM32(eax + 4) = ecx;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi) = eax;
    MEM32(0x3D85FC) = ecx;
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C5C30
 * Original: 0x003C5C30 - 0x003C5C52 (34 bytes, 6 insns)
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C5C30(void)
{
    RECOMP_ENTRY(0x003C5C30u);
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C5C30: ;
    eax = MEM32(esp + 4);
    ecx = MEM32(0x3D83C8);
    MEM32(ecx + 0x790) = eax;
    MEM32(0x3D83D0) = MEM32(0x3D83D0) | 0x4000;
    _fa = (uint32_t)(MEM32(0x3D83D0)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(0x3D85F8) = eax;
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C5C60
 * Original: 0x003C5C60 - 0x003C5C87 (39 bytes, 14 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C5C60(void)
{
    RECOMP_ENTRY(0x003C5C60u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C5C60: ;
    eax = MEM32(0x3D5F20);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(0x3D5F24)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(0x3D5F24) (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_003C5C7C; /* jae: above or equal (unsigned >=) */

loc_003C5C70: ;
    MEM32(0x3D5F20) = eax;
    MEM32(eax + -8) = ecx;
    MEM32(eax + -4) = edx;
    esp += 4; return; /* ret */

loc_003C5C7C: ;
    PUSH32(esp, edx);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C5C83u); RECOMP_ABI_CALL(0x003CACC0u, sub_003CACC0); /* call 0x003CACC0 */

loc_003C5C83: ;
    POP32(esp, ecx);
    POP32(esp, edx);
    goto loc_003C5C60;

}

/**
 * sub_003C5C90
 * Original: 0x003C5C90 - 0x003C5D95 (261 bytes, 80 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C5C90(void)
{
    RECOMP_ENTRY(0x003C5C90u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003C5C90: ;
    PUSH32(esp, ecx);
    PUSH32(esp, ebx);
    ebx = MEM32(0x3D83C8);
    PUSH32(esp, ebp);
    ebp = MEM32(esp + 0x10);
    eax = esi;
    eax = eax << 7;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    MEM32(eax + 0x3D5D80) = ebp;
    eax = MEM32(ebx);
    ecx = MEM32(ebx + 4);
    PUSH32(esp, edi);
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    edi = 0; /* xor self */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ecx (32-bit) */
    MEM32(esp + 0x14) = edx;
    MEM32(esp + 0xC) = ebp;
    if (CMP_B(_fa, _fb)) goto loc_003C5CD1; /* jb: below (unsigned <) */

loc_003C5CBE: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C5CCDu); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C5CCD: ;
    edx = MEM32(esp + 0x14);

loc_003C5CD1: ;
    ebp = ebp & 0xFFFF0000u;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ecx = ebp;
    ebp = 1;
    if ((_fa == 0)) goto loc_003C5D2F; /* je: equal / zero */

loc_003C5CE0: ;
    edi = esi * 4 + 0x41964;
    MEM32(eax) = edi;
    MEM32(eax + 4) = 0xFF000000u;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x30000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x30000 (32-bit) */
    MEM32(esp + 0xC) = esi;
    if (CMP_A(_fa, _fb)) goto loc_003C5D2A; /* ja: above (unsigned >) */

loc_003C5CFF: ;
    if (CMP_EQ(_fa, _fb)) goto loc_003C5D1D; /* je: equal / zero */

loc_003C5D01: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x10000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x10000 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C5D10; /* je: equal / zero */

loc_003C5D09: ;
    edi = 0x2400;
    goto loc_003C5D2F;

loc_003C5D10: ;
    MEM32(esp + 0x14) = ebp;
    edi = 0x8511;
    edx = ebp;
    goto loc_003C5D2F;

loc_003C5D1D: ;
    MEM32(esp + 0x14) = ebp;
    edi = 0x8512;
    edx = ebp;
    goto loc_003C5D2F;

loc_003C5D2A: ;
    edi = 0x2401;

loc_003C5D2F: ;
    SET_LO8(ecx, MEM8(esp + 0xC));
    _fb = (uint32_t)(9) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    SET_LO8(ecx, LO8(ecx) + 9);
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    MEM8(esi + 0x4527E1) = LO8(ecx);
    ecx = esi + 0xC03C;
    ecx = ecx << 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    MEM32(eax) = ecx;
    MEM32(eax + 4) = edi;
    MEM32(eax + 8) = edi;
    MEM32(eax + 0xC) = edi;
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x10;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(ebx) = eax;
    eax = MEM32(ebx + 0x950);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C5D6D; /* jne: not equal / not zero */

loc_003C5D5F: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C5D6D; /* je: equal / zero */

loc_003C5D63: ;
    MEM32(0x3D83D0) = MEM32(0x3D83D0) | 0x200;
    _fa = (uint32_t)(MEM32(0x3D83D0)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */

loc_003C5D6D: ;
    ecx = esi;
    ebp = ebp << LO8(ecx);
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edx = edx << LO8(ecx);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    POP32(esp, edi);
    ebp = ~ebp;
    ebp = ebp & eax;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax = MEM32(0x3D83D0);
    ebp = ebp | edx;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(ebx + 0x950) = ebp;
    eax = eax | 0x47F;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    POP32(esp, ebp);
    MEM32(0x3D83D0) = eax;
    POP32(esp, ebx);
    POP32(esp, ecx);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C5DA0
 * Original: 0x003C5DA0 - 0x003C5E0B (107 bytes, 39 insns)
 * Category: game_render
 * CC: cdecl, 2 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C5DA0(void)
{
    RECOMP_ENTRY(0x003C5DA0u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003C5DA0: ;
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    ebp = MEM32(esp + 0x10);
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    PUSH32(esp, edi);
    edi = eax;
    eax = MEM32(esi + 0x784);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    ebx = edi;
    if (CMP_NE(_fa, _fb)) goto loc_003C5DBF; /* jne: not equal / not zero */

loc_003C5DBC: ;
    ebx = edi + 1;

loc_003C5DBF: ;
    _fa = (uint32_t)(LO8(ebx)) & 0xFFu; _fb = (uint32_t)(3) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ebx), 3 (8-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003C5DF4; /* je: equal / zero */

loc_003C5DC4: ;
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C5DDA; /* jb: below (unsigned <) */

loc_003C5DCB: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C5DDAu); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C5DDA: ;
    ecx = MEM32(esp + 0x14);
    ebx = ebx << 4;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ebx = ebx + ecx;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edx = ebx * 4 + 0x41AD0;
    MEM32(eax) = edx;
    MEM32(eax + 4) = ebp;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi) = eax;

loc_003C5DF4: ;
    eax = MEM32(esp + 0x14);
    edi = edi << 5;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edi = edi + eax;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(edi * 4 + 0x3D5D10) = ebp;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebp);
    POP32(esp, ebx);
    esp += 12; return; /* ret 8 */

}

/**
 * sub_003C5E10
 * Original: 0x003C5E10 - 0x003C5E53 (67 bytes, 25 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C5E10(void)
{
    RECOMP_ENTRY(0x003C5E10u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C5E10: ;
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    edi = MEM32(0x3D83C8);
    ecx = MEM32(edi + 4);
    esi = eax;
    eax = MEM32(edi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ecx (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C5E32; /* jb: below (unsigned <) */

loc_003C5E23: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C5E32u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C5E32: ;
    edx = esi;
    edx = edx << 6;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(0x41B24) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edx = edx + 0x41B24;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(eax) = edx;
    MEM32(eax + 4) = ebx;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(edi) = eax;
    esi = esi << 7;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    POP32(esp, edi);
    MEM32(esi + 0x3D5D84) = ebx;
    POP32(esp, esi);
    esp += 4; return; /* ret */

}

/**
 * sub_003C5E60
 * Original: 0x003C5E60 - 0x003C5E9F (63 bytes, 23 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C5E60(void)
{
    RECOMP_ENTRY(0x003C5E60u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C5E60: ;
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    edi = MEM32(0x3D83C8);
    ecx = MEM32(edi + 4);
    esi = eax;
    eax = MEM32(edi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ecx (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C5E82; /* jb: below (unsigned <) */

loc_003C5E73: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C5E82u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C5E82: ;
    edx = esi * 4 + 0x40AE0;
    MEM32(eax) = edx;
    MEM32(eax + 4) = ebx;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(edi) = eax;
    esi = esi << 7;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    POP32(esp, edi);
    MEM32(esi + 0x3D5D88) = ebx;
    POP32(esp, esi);
    esp += 4; return; /* ret */

}

/**
 * sub_003C5EA0
 * Original: 0x003C5EA0 - 0x003C5F5E (190 bytes, 70 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C5EA0(void)
{
    RECOMP_ENTRY(0x003C5EA0u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    int _cf = 0; /* carry flag */

loc_003C5EA0: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xC) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0xC (32-bit) */
    _cf = (int)(_fa < _fb);
    PUSH32(esp, ebx);
    PUSH32(esp, esi);
    ebx = ecx;
    if (CMP_GE(_fas, _fbs)) goto loc_003C5ECF; /* jge: greater or equal (signed >=) */

loc_003C5EA9: ;
    ecx = eax;
    esi = 1;
    if (LO8(ecx)) _cf = (int)(((esi) >> (32 - (LO8(ecx)))) & 1);
    esi = esi << LO8(ecx);
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ecx = MEM32(0x3D83D0);
    if (5) _cf = (int)(((eax) >> (32 - (5))) & 1);
    eax = eax << 5;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _cf = 0; /* logical op clears CF */
    ecx = ecx | esi;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(edx)) >> 32) & 1);
    eax = eax + edx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    POP32(esp, esi);
    MEM32(eax * 4 + 0x3D5D10) = ebx;
    MEM32(0x3D83D0) = ecx;
    POP32(esp, ebx);
    esp += 4; return; /* ret */

loc_003C5ECF: ;
    if (CMP_NE(_fa, _fb)) goto loc_003C5EFC; /* jne: not equal / not zero */

loc_003C5ED1: ;
    edx = MEM32(0x3D83D0);
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x19) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, 0x19 (32-bit) */
    _cf = (int)(_fa < _fb);
    ecx = _cf ? 0xFFFFFFFF : 0; /* sbb self (CF extend) */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sbb result */
    _cf = 0; /* logical op clears CF */
    ecx = ecx & 0xFFFFBFF1u;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fb = (uint32_t)(0x480F) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(ecx) + (uint64_t)(0x480F)) >> 32) & 1);
    ecx = ecx + 0x480F;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _cf = 0; /* logical op clears CF */
    edx = edx | ecx;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    if (7) _cf = (int)(((eax) >> (32 - (7))) & 1);
    eax = eax << 7;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    POP32(esp, esi);
    MEM32(eax + 0x3D5D40) = ebx;
    MEM32(0x3D83D0) = edx;
    POP32(esp, ebx);
    esp += 4; return; /* ret */

loc_003C5EFC: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x16) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0x16 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_GE(_fas, _fbs)) goto loc_003C5F25; /* jge: greater or equal (signed >=) */

loc_003C5F01: ;
    esi = MEM32(0x3D83D0);
    ecx = MEM32(edx * 4 + 0x3E9170);
    _cf = 0; /* logical op clears CF */
    esi = esi | ecx;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    if (5) _cf = (int)(((eax) >> (32 - (5))) & 1);
    eax = eax << 5;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    MEM32(0x3D83D0) = esi;
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(edx)) >> 32) & 1);
    eax = eax + edx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    POP32(esp, esi);
    MEM32(eax * 4 + 0x3D5D10) = ebx;
    POP32(esp, ebx);
    esp += 4; return; /* ret */

loc_003C5F25: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x1C) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0x1C (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_NE(_fa, _fb)) goto loc_003C5F35; /* jne: not equal / not zero */

loc_003C5F2A: ;
    PUSH32(esp, ebx);
    esi = eax;
    PUSH32(esp, 0x003C5F32u); RECOMP_ABI_CALL(0x003C5C90u, sub_003C5C90); /* call 0x003C5C90 */

loc_003C5F32: ;
    POP32(esp, esi);
    POP32(esp, ebx);
    esp += 4; return; /* ret */

loc_003C5F35: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x1D) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0x1D (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_NE(_fa, _fb)) goto loc_003C5F42; /* jne: not equal / not zero */

loc_003C5F3A: ;
    PUSH32(esp, 0x003C5F3Fu); RECOMP_ABI_CALL(0x003C5E10u, sub_003C5E10); /* call 0x003C5E10 */

loc_003C5F3F: ;
    POP32(esp, esi);
    POP32(esp, ebx);
    esp += 4; return; /* ret */

loc_003C5F42: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x1E) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0x1E (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_NE(_fa, _fb)) goto loc_003C5F4F; /* jne: not equal / not zero */

loc_003C5F47: ;
    PUSH32(esp, 0x003C5F4Cu); RECOMP_ABI_CALL(0x003C5E60u, sub_003C5E60); /* call 0x003C5E60 */

loc_003C5F4C: ;
    POP32(esp, esi);
    POP32(esp, ebx);
    esp += 4; return; /* ret */

loc_003C5F4F: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x1B) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0x1B (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_G(_fas, _fbs)) goto loc_003C5F5B; /* jg: greater (signed >) */

loc_003C5F54: ;
    PUSH32(esp, ebx);
    PUSH32(esp, edx);
    PUSH32(esp, 0x003C5F5Bu); RECOMP_ABI_CALL(0x003C5DA0u, sub_003C5DA0); /* call 0x003C5DA0 */

loc_003C5F5B: ;
    POP32(esp, esi);
    POP32(esp, ebx);
    esp += 4; return; /* ret */

}

/**
 * sub_003C5FD0
 * Original: 0x003C5FD0 - 0x003C60F3 (291 bytes, 74 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C5FD0(void)
{
    RECOMP_ENTRY(0x003C5FD0u);
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

loc_003C5FD0: ;
    eax = MEM32(0x3D83C8);
    fp_push((double)SMEM32(eax + 0xEE0)); /* fild */
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x10;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, esi);
    esi = MEM32(eax + 0xEE0);
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0 (32-bit) */
    PUSH32(esp, edi);
    if (CMP_GE(_fas, _fbs)) goto loc_003C5FF0; /* jge: greater or equal (signed >=) */

loc_003C5FEA: ;
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148E4)); /* fadd dword ptr [0x4148e4] */

loc_003C5FF0: ;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(eax + 0x95C)); /* fmul dword ptr [eax + 0x95c] */
    esi = MEM32(eax + 0xEE4);
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0 (32-bit) */
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(eax + 0xEF8)); /* fadd dword ptr [eax + 0xef8] */
    fp_push((double)SMEM32(eax + 0xEE4)); /* fild */
    if (CMP_GE(_fas, _fbs)) goto loc_003C6012; /* jge: greater or equal (signed >=) */

loc_003C600C: ;
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148E4)); /* fadd dword ptr [0x4148e4] */

loc_003C6012: ;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(eax + 0x960)); /* fmul dword ptr [eax + 0x960] */
    edi = MEM32(eax + 8);
    esi = 0; /* xor self */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0x8000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test edi, 0x8000 (32-bit) */
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(eax + 0xEFC)); /* fadd dword ptr [eax + 0xefc] */
    if (TEST_Z(_fa, _fb)) goto loc_003C6043; /* je: equal / zero */

loc_003C602B: ;
    _fa = (uint32_t)(MEM32(0x3D8638)) & 0xFFFFFFFFu; _fb = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0x3D8638), esi (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C6043; /* je: equal / zero */

loc_003C6033: ;
    { double _t = fp_top(); fp_top() = fp_st1(); fp_st1() = _t; } /* fxch st(1) */
    fp_top() = RECOMP_FP_PC(fp_top() - MEMF(0x4148C0)); /* fsub dword ptr [0x4148c0] */
    { double _t = fp_top(); fp_top() = fp_st1(); fp_st1() = _t; } /* fxch st(1) */
    fp_top() = RECOMP_FP_PC(fp_top() - MEMF(0x4148C0)); /* fsub dword ptr [0x4148c0] */

loc_003C6043: ;
    edi = MEM32(eax + 0xEE8);
    fp_push((double)SMEM32(eax + 0xEE8)); /* fild */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0 (32-bit) */
    if (CMP_GE(_fas, _fbs)) goto loc_003C6059; /* jge: greater or equal (signed >=) */

loc_003C6053: ;
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148E4)); /* fadd dword ptr [0x4148e4] */

loc_003C6059: ;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(eax + 0x95C)); /* fmul dword ptr [eax + 0x95c] */
    edi = MEM32(eax + 0xEEC);
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0 (32-bit) */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x4148C0)); /* fmul dword ptr [0x4148c0] */
    MEMF(esp + 8) = (float)fp_top(); fp_pop(); /* fstp */
    fp_push((double)SMEM32(eax + 0xEEC)); /* fild */
    if (CMP_GE(_fas, _fbs)) goto loc_003C607F; /* jge: greater or equal (signed >=) */

loc_003C6079: ;
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148E4)); /* fadd dword ptr [0x4148e4] */

loc_003C607F: ;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(eax + 0x960)); /* fmul dword ptr [eax + 0x960] */
    POP32(esp, edi);
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x4148E8)); /* fmul dword ptr [0x4148e8] */
    MEMF(esp + 8) = (float)fp_top(); fp_pop(); /* fstp */
    fp_push(MEMF(eax + 0xEF4)); /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() - MEMF(eax + 0xEF0)); /* fsub dword ptr [eax + 0xef0] */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(eax + 0x948)); /* fmul dword ptr [eax + 0x948] */
    MEMF(esp + 0x10) = (float)fp_top(); fp_pop(); /* fstp */
    fp_push(MEMF(eax + 0xEF0)); /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(eax + 0x948)); /* fmul dword ptr [eax + 0x948] */
    MEM32(edx + 0xC) = esi;
    MEMF(esp + 0xC) = (float)fp_top(); fp_pop(); /* fstp */
    fp_push(MEMF(esp + 4)); /* fld float */
    eax = MEM32(esp + 0xC);
    fp_top() = RECOMP_FP_PC(fp_top() + g_fp_stack[(g_fp_top + 2) & 7]); /* fadd st(2) */
    MEM32(edx + 8) = eax;
    eax = MEM32(esp + 8);
    MEMF(edx) = (float)fp_top(); fp_pop(); /* fstp */
    fp_push(MEMF(esp + 8)); /* fld float */
    fp_top() = -fp_top(); /* fchs */
    fp_top() = RECOMP_FP_PC(fp_top() + fp_st1()); /* fadd st(1) */
    MEMF(edx + 4) = (float)fp_top(); fp_pop(); /* fstp */
    edx = MEM32(esp + 4);
    MEM32(ecx) = edx;
    edx = MEM32(esp + 0x10);
    fp_pop(); /* fstp st(0) */
    MEM32(ecx + 0xC) = esi;
    fp_pop(); /* fstp st(0) */
    MEM32(ecx + 4) = eax;
    MEM32(ecx + 8) = edx;
    POP32(esp, esi);
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x10;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 4; return; /* ret */

    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}

/**
 * sub_003C63B0
 * Original: 0x003C63B0 - 0x003C6488 (216 bytes, 51 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C63B0(void)
{
    RECOMP_ENTRY(0x003C63B0u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C63B0: ;
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    edx = MEM32(esi + 0x2428);
    edx = edx & 0xFFFFFFF7u;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(esi + 0x2428) = edx;
    ecx = MEM32(0x3D866C);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    eax = edx;
    if (CMP_EQ(_fa, _fb)) goto loc_003C63DB; /* je: equal / zero */

loc_003C63D2: ;
    eax = eax | 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(esi + 0x2428) = eax;

loc_003C63DB: ;
    edx = MEM32(esi + 0x242C);
    edx = edx & 0xE7EFFFFFu;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(esi + 0x242C) = edx;
    ecx = MEM32(0x3D8664);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    eax = edx;
    if (CMP_EQ(_fa, _fb)) goto loc_003C6404; /* je: equal / zero */

loc_003C63F9: ;
    eax = eax | 0x100000;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(esi + 0x242C) = eax;

loc_003C6404: ;
    eax = MEM32(0x3D8668);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C6417; /* je: equal / zero */

loc_003C640D: ;
    MEM32(esi + 0x242C) = MEM32(esi + 0x242C) | 0x8000000;
    _fa = (uint32_t)(MEM32(esi + 0x242C)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */

loc_003C6417: ;
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C642D; /* jb: below (unsigned <) */

loc_003C641E: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C642Du); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C642D: ;
    MEM32(eax) = 0x40110;
    MEM32(eax + 4) = 0;
    ecx = MEM32(esi + 0x2428);
    MEM32(eax + 8) = 0x81D8C;
    MEM32(eax + 0xC) = 0x400094;
    MEM32(eax + 0x10) = ecx;
    MEM32(eax + 0x14) = 0x40100;
    edx = 9;
    MEM32(eax + 0x18) = edx;
    ecx = MEM32(esi + 0x242C);
    MEM32(eax + 0x1C) = 0x81D8C;
    MEM32(eax + 0x20) = 0x400B80;
    MEM32(eax + 0x24) = ecx;
    MEM32(eax + 0x28) = 0x40100;
    MEM32(eax + 0x2C) = edx;
    _fb = (uint32_t)(0x30) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x30;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi) = eax;
    POP32(esp, esi);
    esp += 4; return; /* ret */

}

/**
 * sub_003C6490
 * Original: 0x003C6490 - 0x003C67AB (795 bytes, 236 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C6490(void)
{
    RECOMP_ENTRY(0x003C6490u);
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

loc_003C6490: ;
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    PUSH32(esp, esi);
    esi = eax;
    ebx = MEM32(esi + 0x1A04);
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 0x1A14)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, MEM32(esi + 0x1A14) (32-bit) */
    PUSH32(esp, edi);
    if (CMP_NE(_fa, _fb)) goto loc_003C65F4; /* jne: not equal / not zero */

loc_003C64AB: ;
    ecx = MEM32(esi + 0x970);
    eax = MEM32(esi + 0x96C);
    ebp = MEM32(0x3D8640);
    MEM32(esp + 0x14) = ecx;
    ecx = MEM32(esi + 0x1A18);
    MEM32(esp + 0x10) = eax;
    eax = MEM32(ecx + 0x10);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C64E4; /* jne: not equal / not zero */

loc_003C64D2: ;
    ecx = MEM32(ecx + 0xC);
    ecx = ecx >> 0x14;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ecx = ecx & 0xF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax = 1;
    eax = eax << LO8(ecx);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    goto loc_003C64EA;

loc_003C64E4: ;
    eax = eax & 0xFFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_003C64EA: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    MEM32(esp + 0x18) = eax;
    fp_push((double)SMEM32(esp + 0x18)); /* fild */
    if (CMP_GE(_fas, _fbs)) goto loc_003C64FC; /* jge: greater or equal (signed >=) */

loc_003C64F6: ;
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148E4)); /* fadd dword ptr [0x4148e4] */

loc_003C64FC: ;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x10)); /* fmul dword ptr [esp + 0x10] */
    PUSH32(esp, ecx);
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C0)); /* fadd dword ptr [0x4148c0] */
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003C650Fu); RECOMP_ABI_CALL(0x003CC3C0u, sub_003CC3C0); /* call 0x003CC3C0 */

loc_003C650F: ;
    ecx = MEM32(esi + 0x1A18);
    edi = eax;
    eax = MEM32(ecx + 0x10);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C652E; /* jne: not equal / not zero */

loc_003C651E: ;
    ecx = ZX8(MEM8(ecx + 0xF));
    ecx = ecx & 0xF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax = 1;
    eax = eax << LO8(ecx);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    goto loc_003C6537;

loc_003C652E: ;
    eax = eax >> 0xC;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    eax = eax & 0xFFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_003C6537: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    MEM32(esp + 0x18) = eax;
    fp_push((double)SMEM32(esp + 0x18)); /* fild */
    if (CMP_GE(_fas, _fbs)) goto loc_003C6549; /* jge: greater or equal (signed >=) */

loc_003C6543: ;
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148E4)); /* fadd dword ptr [0x4148e4] */

loc_003C6549: ;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x14)); /* fmul dword ptr [esp + 0x14] */
    PUSH32(esp, ecx);
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C0)); /* fadd dword ptr [0x4148c0] */
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003C655Cu); RECOMP_ABI_CALL(0x003CC3C0u, sub_003CC3C0); /* call 0x003CC3C0 */

loc_003C655C: ;
    edx = MEM32(esp + 0x10);
    fp_push(MEMF(esp + 0x14)); /* fld float */
    MEM32(esp + 0x10) = edx;
    edx = MEM32(esi + 0x1A14);
    ecx = MEM32(edx + 0x10);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C6587; /* jne: not equal / not zero */

loc_003C6575: ;
    ecx = MEM32(edx + 0xC);
    ecx = ecx >> 0x14;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ecx = ecx & 0xF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ebx = 1;
    ebx = ebx << LO8(ecx);
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    goto loc_003C6590;

loc_003C6587: ;
    ecx = ecx & 0xFFF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    ebx = ecx;

loc_003C6590: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, ebx (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C65B4; /* jb: below (unsigned <) */

loc_003C6594: ;
    edi = MEM32(edx + 0x10);
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C65AD; /* jne: not equal / not zero */

loc_003C659B: ;
    ecx = MEM32(edx + 0xC);
    ecx = ecx >> 0x14;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ecx = ecx & 0xF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    edi = 1;
    edi = edi << LO8(ecx);
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    goto loc_003C65B4;

loc_003C65AD: ;
    edi = edi & 0xFFF;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    edi++;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_003C65B4: ;
    ecx = MEM32(edx + 0x10);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C65CC; /* jne: not equal / not zero */

loc_003C65BB: ;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(ecx, MEM8(edx + 0xF));
    ebx = 1;
    ecx = ecx & 0xF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ebx = ebx << LO8(ecx);
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    goto loc_003C65D8;

loc_003C65CC: ;
    ecx = ecx >> 0xC;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ecx = ecx & 0xFFF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    ebx = ecx;

loc_003C65D8: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ebx (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C664A; /* jb: below (unsigned <) */

loc_003C65DC: ;
    eax = MEM32(edx + 0x10);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C6641; /* jne: not equal / not zero */

loc_003C65E3: ;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(ecx, MEM8(edx + 0xF));
    eax = 1;
    ecx = ecx & 0xF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax = eax << LO8(ecx);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    goto loc_003C664A;

loc_003C65F4: ;
    eax = MEM32(ebx + 0x10);
    fp_push(MEMF(0x4148C8)); /* fld float */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    ebp = MEM32(0x3D8644);
    MEM32(esp + 0x10) = 0x3F800000;
    if (CMP_NE(_fa, _fb)) goto loc_003C6621; /* jne: not equal / not zero */

loc_003C660F: ;
    ecx = MEM32(ebx + 0xC);
    ecx = ecx >> 0x14;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ecx = ecx & 0xF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    edx = 1;
    edx = edx << LO8(ecx);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    goto loc_003C662A;

loc_003C6621: ;
    edx = eax;
    edx = edx & 0xFFF;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    edx++;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_003C662A: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    edi = edx;
    if (CMP_NE(_fa, _fb)) goto loc_003C6641; /* jne: not equal / not zero */

loc_003C6630: ;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(ecx, MEM8(ebx + 0xF));
    eax = 1;
    ecx = ecx & 0xF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax = eax << LO8(ecx);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    goto loc_003C664A;

loc_003C6641: ;
    eax = eax >> 0xC;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    eax = eax & 0xFFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_003C664A: ;
    ebx = MEM32(esi + 0x1A0C);
    ecx = eax;
    eax = MEM32(esi + 8);
    eax = eax & 0xFFFF7FFFu;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, 0 (32-bit) */
    MEM32(esi + 8) = eax;
    if (CMP_EQ(_fa, _fb)) goto loc_003C669B; /* je: equal / zero */

loc_003C6661: ;
    _fa = (uint32_t)(HI8(ebx)) & 0xFFu; _fb = (uint32_t)(2) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(ebx), 2 (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C669B; /* jne: not equal / not zero */

loc_003C6666: ;
    fp_push(MEMF(esp + 0x10)); /* fld float */
    eax = eax | 0x8000;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x4148C0)); /* fmul dword ptr [0x4148c0] */
    edi++;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    edi = edi >> 1;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(2) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, 2 (32-bit) */
    MEMF(esp + 0x10) = (float)fp_top(); fp_pop(); /* fstp */
    MEM32(esi + 8) = eax;
    if (CMP_NE(_fa, _fb)) goto loc_003C6695; /* jne: not equal / not zero */

loc_003C6684: ;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x4148C0)); /* fmul dword ptr [0x4148c0] */
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ebx = ebx | 0x2000;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    goto loc_003C669B;

loc_003C6695: ;
    ebx = ebx | 0x1000;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */

loc_003C669B: ;
    eax = MEM32(esp + 0x10);
    MEMF(esi + 0x960) = (float)fp_top(); /* fst */
    fp_push(MEMF(esp + 0x10)); /* fld float */
    MEM32(esi + 0x95C) = eax;
    g_fp_cmp = RECOMP_FCMP(fp_top(), fp_st1()); g_fp_cc = RECOMP_FCMP_CC(g_fp_cmp); fp_pop(); /* fcomp st(1) */
    MEM32(esi + 0x954) = edi;
    MEM32(esi + 0x958) = ecx;
    eax = (eax & 0xFFFF0000u) | (uint32_t)(uint16_t)(((g_fp_top & 7u) << 11) | g_fp_cc); /* fnstsw ax <- fpu status */
    _fa = (uint32_t)(HI8(eax)) & 0xFFu; _fb = (uint32_t)(5) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(eax), 5 (8-bit) */
    if (RECOMP_PARITY8((_fa) & (_fb))) goto loc_003C66D0; /* jp: parity */

loc_003C66C4: ;
    ecx = MEM32(esp + 0x10);
    fp_pop(); /* fstp st(0) */
    MEM32(esp + 0x10) = ecx;
    goto loc_003C66D4;

loc_003C66D0: ;
    MEMF(esp + 0x10) = (float)fp_top(); fp_pop(); /* fstp */

loc_003C66D4: ;
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C66EA; /* jb: below (unsigned <) */

loc_003C66DB: ;
    eax = MEM32(0x3D9588);
    edx = eax;
    edx = edx >> 1;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, edx);
    PUSH32(esp, 0x003C66EAu); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C66EA: ;
    fp_push(MEMF(esp + 0x10)); /* fld float */
    MEM32(eax) = 0x40208;
    MEM32(eax + 4) = ebx;
    fp_push(MEMF(esi + 0x964)); /* fld float */
    g_fp_cmp = RECOMP_FCMP(fp_top(), fp_st1()); g_fp_cc = RECOMP_FCMP_CC(g_fp_cmp); fp_pop(); fp_pop(); /* fucompp  */
    ecx = eax + 8;
    eax = (eax & 0xFFFF0000u) | (uint32_t)(uint16_t)(((g_fp_top & 7u) << 11) | g_fp_cc); /* fnstsw ax <- fpu status */
    _fa = (uint32_t)(HI8(eax)) & 0xFFu; _fb = (uint32_t)(0x44) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(eax), 0x44 (8-bit) */
    if ((!RECOMP_PARITY8((_fa) & (_fb)))) goto loc_003C6785; /* jnp: not parity */

loc_003C6709: ;
    fp_push(MEMF(esp + 0x10)); /* fld float */
    eax = MEM32(esp + 0x10);
    fp_top() = RECOMP_FP_PC(fp_top() + fp_top()); /* fadd st(0), st(0) */
    PUSH32(esp, ecx);
    MEM32(esi + 0x964) = eax;
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C0)); /* fadd dword ptr [0x4148c0] */
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003C6728u); RECOMP_ABI_CALL(0x003CC3C0u, sub_003CC3C0); /* call 0x003CC3C0 */

loc_003C6728: ;
    edx = MEM32(eax * 4 + 0x3D4910);
    ebx = MEM32(0x3D83D0);
    MEM32(esi + 0x968) = edx;
    eax = MEM32(0x3D864C);
    MEM32(esp + 0x14) = eax;
    fp_push(MEMF(esp + 0x14)); /* fld float */
    PUSH32(esp, ecx);
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x14)); /* fmul dword ptr [esp + 0x14] */
    ebx = ebx | 0x10F;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(0x3D83D0) = ebx;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x3E9E08)); /* fmul dword ptr [0x3e9e08] */
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C0)); /* fadd dword ptr [0x4148c0] */
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003C676Du); RECOMP_ABI_CALL(0x003CC3C0u, sub_003CC3C0); /* call 0x003CC3C0 */

loc_003C676D: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x1FF) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x1FF (32-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_003C6779; /* jbe: below or equal (unsigned <=) */

loc_003C6774: ;
    eax = 0x1FF;

loc_003C6779: ;
    MEM32(ecx) = 0x40380;
    MEM32(ecx + 4) = eax;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 8;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C6785: ;
    edi = esi;
    MEM32(esi) = ecx;
    PUSH32(esp, 0x003C678Eu); RECOMP_ABI_CALL(0x003C6360u, sub_003C6360); /* call 0x003C6360 */

loc_003C678E: ;
    ecx = MEM32(esi + 0x794);
    _fa = (uint32_t)(MEM8(ecx + 4)) & 0xFFu; _fb = (uint32_t)(2) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test MEM8(ecx + 4), 2 (8-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003C679F; /* je: equal / zero */

loc_003C679A: ;
    PUSH32(esp, 0x003C679Fu); RECOMP_ABI_CALL(0x003C8F10u, sub_003C8F10); /* call 0x003C8F10 */

loc_003C679F: ;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebp);
    POP32(esp, ebx);
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    g_seh_ebp = ebp; sub_003C7270(); return; /* tail jmp 0x003C7270 */

    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}

/**
 * sub_003C67B0
 * Original: 0x003C67B0 - 0x003C68F0 (320 bytes, 84 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: standard_frame
 */
void d3d_orig_003C67B0(void)
{
    RECOMP_ENTRY(0x003C67B0u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* prologue saves caller's frame */
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

loc_003C67B0: ;
    PUSH32(esp, ebp);
    ebp = esp;
    g_ebp = ebp; /* publish frame for frameless callees */
    g_seh_ebp = ebp;
    esp = esp & 0xFFFFFFF0u;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fb = (uint32_t)(0x5C) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x5C;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    edx = MEM32(0x3D83C8);
    eax = MEM32(edx + 0xEE8);
    fp_push((double)SMEM32(edx + 0xEE8)); /* fild */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    PUSH32(esp, edi);
    if (CMP_GE(_fas, _fbs)) goto loc_003C67D6; /* jge: greater or equal (signed >=) */

loc_003C67D0: ;
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148E4)); /* fadd dword ptr [0x4148e4] */

loc_003C67D6: ;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(edx + 0x95C)); /* fmul dword ptr [edx + 0x95c] */
    ecx = MEM32(edx + 0xEEC);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x4148C0)); /* fmul dword ptr [0x4148c0] */
    MEMF(esp + 0x14) = (float)fp_top(); /* fst */
    fp_push((double)SMEM32(edx + 0xEEC)); /* fild */
    if (CMP_GE(_fas, _fbs)) goto loc_003C67FC; /* jge: greater or equal (signed >=) */

loc_003C67F6: ;
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148E4)); /* fadd dword ptr [0x4148e4] */

loc_003C67FC: ;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(edx + 0x960)); /* fmul dword ptr [edx + 0x960] */
    eax = MEM32(0x3D8614);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(2) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 2 (32-bit) */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x4148E8)); /* fmul dword ptr [0x4148e8] */
    MEMF(esp + 0xC) = (float)fp_top(); /* fst */
    fp_top() = -fp_top(); /* fchs */
    MEMF(esp + 0x18) = (float)fp_top(); fp_pop(); /* fstp */
    fp_push(MEMF(edx + 0xEF4)); /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() - MEMF(edx + 0xEF0)); /* fsub dword ptr [edx + 0xef0] */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(edx + 0x948)); /* fmul dword ptr [edx + 0x948] */
    MEMF(esp + 0x10) = (float)fp_top(); fp_pop(); /* fstp */
    fp_push(MEMF(edx + 0x948)); /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(edx + 0xEF0)); /* fmul dword ptr [edx + 0xef0] */
    MEMF(esp + 0x1C) = (float)fp_top(); fp_pop(); /* fstp */
    if (CMP_NE(_fa, _fb)) goto loc_003C6882; /* jne: not equal / not zero */

loc_003C6842: ;
    fp_push(MEMF(edx + 0x944)); /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(edx + 0x948)); /* fmul dword ptr [edx + 0x948] */
    { double _t = fp_top(); fp_push(_t); } /* fld st(0) */
    g_fp_stack[(g_fp_top + 2) & 7] = RECOMP_FP_PC(g_fp_stack[(g_fp_top + 2) & 7] * fp_top()); fp_pop(); /* fmulp st(2) */
    fp_push(MEMF(esp + 0xC)); /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() * fp_st1()); /* fmul st(1) */
    { double _t = fp_st1(); fp_push(_t); } /* fld st(1) */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x10)); /* fmul dword ptr [esp + 0x10] */
    MEMF(esp + 0x10) = (float)fp_top(); fp_pop(); /* fstp */
    { double _t = fp_st1(); fp_push(_t); } /* fld st(1) */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x14)); /* fmul dword ptr [esp + 0x14] */
    MEMF(esp + 0x14) = (float)fp_top(); fp_pop(); /* fstp */
    { double _t = fp_st1(); fp_push(_t); } /* fld st(1) */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x18)); /* fmul dword ptr [esp + 0x18] */
    MEMF(esp + 0x18) = (float)fp_top(); fp_pop(); /* fstp */
    { double _t = fp_st1(); fp_push(_t); } /* fld st(1) */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x1C)); /* fmul dword ptr [esp + 0x1c] */
    MEMF(esp + 0x1C) = (float)fp_top(); fp_pop(); /* fstp */
    goto loc_003C688C;

loc_003C6882: ;
    fp_push(MEMF(0x4148C8)); /* fld float */
    fp_push(MEMF(esp + 0xC)); /* fld float */

loc_003C688C: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    { double _t = fp_top(); fp_top() = g_fp_stack[(g_fp_top + 2) & 7]; g_fp_stack[(g_fp_top + 2) & 7] = _t; } /* fxch st(2) */
    ecx = 0x10;
    edi = esp + 0x20;
    { uint32_t _i; int32_t _st = RECOMP_DF_STEP(4); for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*_st) = eax; edi += ecx * _st; }
    ecx = 0; /* rep stosd */
    MEMF(esp + 0x20) = (float)fp_top(); fp_pop(); /* fstp */
    { double _t = fp_top(); fp_top() = fp_st1(); fp_st1() = _t; } /* fxch st(1) */
    MEMF(esp + 0x34) = (float)fp_top(); fp_pop(); /* fstp */
    eax = MEM32(esp + 0x10);
    MEMF(esp + 0x5C) = (float)fp_top(); fp_pop(); /* fstp */
    ecx = MEM32(esp + 0x14);
    MEM32(esp + 0x48) = eax;
    eax = MEM32(esp + 0x18);
    MEM32(esp + 0x50) = ecx;
    ecx = MEM32(esp + 0x1C);
    MEM32(esp + 0x54) = eax;
    eax = esp + 0x20;
    MEM32(esp + 0x58) = ecx;
    PUSH32(esp, eax);
    ecx = edx + 0xCA0;
    PUSH32(esp, ecx);
    _fb = (uint32_t)(0x980) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edx = edx + 0x980;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, edx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x003C68E1u); RECOMP_ABI_CALL(0x003CC550u, sub_003CC550); /* call 0x003CC550 */

loc_003C68E1: ;
    MEM32(0x3D83D0) = MEM32(0x3D83D0) | 0x200;
    _fa = (uint32_t)(MEM32(0x3D83D0)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    POP32(esp, edi);
    esp = ebp;
    POP32(esp, ebp);
    esp += 4; return; /* ret */

    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}

/**
 * sub_003C69D0
 * Original: 0x003C69D0 - 0x003C6A7A (170 bytes, 65 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: standard_frame
 */
void d3d_orig_003C69D0(void)
{
    RECOMP_ENTRY(0x003C69D0u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C69D0: ;
    PUSH32(esp, ebp);
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    edi = MEM32(0x3D83C8);
    eax = MEM32(edi + 0x2C);
    PUSH32(esp, 2);
    PUSH32(esp, eax);
    PUSH32(esp, 0x003C69E4u); RECOMP_ABI_CALL(0x003CAA10u, sub_003CAA10); /* call 0x003CAA10 */

loc_003C69E4: ;
    eax = MEM32(edi + 0x1DE4);
    ebp = edi + 0x1C28;
    eax = eax & 1;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ecx = eax + eax * 2 + 0x5D;
    eax = ebp + ecx * 4;
    goto loc_003C6A00;

    /* nop */

loc_003C6A00: ;
    _fa = (uint32_t)(MEM32(eax)) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(eax), 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) { RECOMP_SPIN(0x003C6A00u); goto loc_003C6A00; } /* jne: not equal / not zero */

loc_003C6A05: ;
    PUSH32(esp, edi);
    PUSH32(esp, 0x003C6A0Bu); RECOMP_ABI_CALL(0x003CDE80u, sub_003CDE80); /* call 0x003CDE80 */

loc_003C6A0B: ;
    PUSH32(esp, ebx);
    PUSH32(esp, 0x003C6A11u); RECOMP_ABI_CALL(0x003CE090u, sub_003CE090); /* call 0x003CE090 */

loc_003C6A11: ;
    esi = eax;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0 (32-bit) */
    if (CMP_GE(_fas, _fbs)) goto loc_003C6A23; /* jge: greater or equal (signed >=) */

loc_003C6A17: ;
    PUSH32(esp, edi);
    PUSH32(esp, 0x003C6A1Du); RECOMP_ABI_CALL(0x003CDE80u, sub_003CDE80); /* call 0x003CDE80 */

loc_003C6A1D: ;
    POP32(esp, edi);
    eax = esi;
    POP32(esp, esi);
    POP32(esp, ebp);
    esp += 4; return; /* ret */

loc_003C6A23: ;
    esi = MEM32(edi + 0x1A18);
    PUSH32(esp, 0x003C6A2Eu); RECOMP_ABI_CALL(0x003C68F0u, sub_003C68F0); /* call 0x003C68F0 */

loc_003C6A2E: ;
    edx = MEM32(ebx + 8);
    ecx = MEM32(ebx + 0x2C);
    PUSH32(esp, eax);
    eax = MEM32(ebx + 0x28);
    PUSH32(esp, edx);
    edx = MEM32(ebx + 4);
    PUSH32(esp, eax);
    eax = MEM32(ebx);
    PUSH32(esp, ecx);
    PUSH32(esp, edx);
    PUSH32(esp, eax);
    esi = ebp;
    PUSH32(esp, 0x003C6A49u); RECOMP_ABI_CALL(0x003D0571u, sub_003D0571); /* call 0x003D0571 */

loc_003C6A49: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_L(_fas, _fbs)) goto loc_003C6A76; /* jl: less (signed <) */

loc_003C6A4D: ;
    ecx = MEM32(edi + 0x1A20);
    edx = MEM32(edi + 0x1A14);
    PUSH32(esp, ecx);
    PUSH32(esp, edx);
    PUSH32(esp, 0x003C6A60u); RECOMP_ABI_CALL(0x003C6A80u, sub_003C6A80); /* call 0x003C6A80 */

loc_003C6A60: ;
    PUSH32(esp, 0);
    PUSH32(esp, 0x3F800000);
    PUSH32(esp, 0);
    PUSH32(esp, 3);
    PUSH32(esp, 0);
    PUSH32(esp, 0);
    PUSH32(esp, 0x003C6A74u); RECOMP_ABI_CALL(0x003CA270u, sub_003CA270); /* call 0x003CA270 */

loc_003C6A74: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */

loc_003C6A76: ;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebp);
    esp += 4; return; /* ret */

}

/**
 * sub_003C6A80
 * Original: 0x003C6A80 - 0x003C6D83 (771 bytes, 225 insns)
 * Category: game_render
 * CC: cdecl, 2 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C6A80(void)
{
    RECOMP_ENTRY(0x003C6A80u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003C6A80: ;
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, ebx);
    ebx = MEM32(esp + 0x14);
    PUSH32(esp, ebp);
    ebp = MEM32(0x3D83C8);
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    edi = 0; /* xor self */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, edi (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C6AA3; /* jne: not equal / not zero */

loc_003C6A97: ;
    eax = MEM32(ebp + 0x1A04);
    MEM32(esp + 0x20) = eax;
    ebx = eax;

loc_003C6AA3: ;
    eax = MEM32(ebx);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x780000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test eax, 0x780000 (32-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C6AC5; /* jne: not equal / not zero */

loc_003C6AAC: ;
    eax = eax & 0x70000;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x50000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x50000 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C6AC5; /* jne: not equal / not zero */

loc_003C6AB8: ;
    eax = MEM32(ebx + 0x14);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edi (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C6AC5; /* je: equal / zero */

loc_003C6ABF: ;
    _fb = (uint32_t)(0x80000) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    MEM32(eax) = MEM32(eax) + 0x80000;
    _fa = (uint32_t)(MEM32(eax)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C6AC5: ;
    _fb = (uint32_t)(0x80000) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    MEM32(ebx) = MEM32(ebx) + 0x80000;
    _fa = (uint32_t)(MEM32(ebx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = MEM32(ebp + 0x1A04);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edi (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C6AEF; /* je: equal / zero */

loc_003C6AD5: ;
    ecx = MEM32(eax + 0x14);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, edi (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C6ADE; /* je: equal / zero */

loc_003C6ADC: ;
    eax = ecx;

loc_003C6ADE: ;
    ecx = MEM32(ebp + 0x2C);
    MEM32(eax + 8) = ecx;
    esi = MEM32(ebp + 0x1A04);
    PUSH32(esp, 0x003C6AEFu); RECOMP_ABI_CALL(0x003C9F70u, sub_003C9F70); /* call 0x003C9F70 */

loc_003C6AEF: ;
    eax = MEM32(ebp + 0x1A08);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edi (32-bit) */
    MEM32(ebp + 0x1A04) = ebx;
    if (CMP_EQ(_fa, _fb)) goto loc_003C6B19; /* je: equal / zero */

loc_003C6AFF: ;
    ecx = MEM32(eax + 0x14);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, edi (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C6B08; /* je: equal / zero */

loc_003C6B06: ;
    eax = ecx;

loc_003C6B08: ;
    edx = MEM32(ebp + 0x2C);
    MEM32(eax + 8) = edx;
    esi = MEM32(ebp + 0x1A08);
    PUSH32(esp, 0x003C6B19u); RECOMP_ABI_CALL(0x003C9F70u, sub_003C9F70); /* call 0x003C9F70 */

loc_003C6B19: ;
    esi = MEM32(esp + 0x24);
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, edi (32-bit) */
    MEM32(ebp + 0x1A08) = esi;
    if (CMP_EQ(_fa, _fb)) goto loc_003C6B67; /* je: equal / zero */

loc_003C6B27: ;
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x780000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test eax, 0x780000 (32-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C6B49; /* jne: not equal / not zero */

loc_003C6B30: ;
    eax = eax & 0x70000;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x50000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x50000 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C6B49; /* jne: not equal / not zero */

loc_003C6B3C: ;
    eax = MEM32(esi + 0x14);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edi (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C6B49; /* je: equal / zero */

loc_003C6B43: ;
    _fb = (uint32_t)(0x80000) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    MEM32(eax) = MEM32(eax) + 0x80000;
    _fa = (uint32_t)(MEM32(eax)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C6B49: ;
    _fb = (uint32_t)(0x80000) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    MEM32(esi) = MEM32(esi) + 0x80000;
    _fa = (uint32_t)(MEM32(esi)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = ebp + 0x948;
    ecx = esi;
    PUSH32(esp, 0x003C6B5Cu); RECOMP_ABI_CALL(0x003CE860u, sub_003CE860); /* call 0x003CE860 */

loc_003C6B5C: ;
    PUSH32(esp, 0x003C6B61u); RECOMP_ABI_CALL(0x003C68F0u, sub_003C68F0); /* call 0x003C68F0 */

loc_003C6B61: ;
    MEM32(ebp + 0x94C) = eax;

loc_003C6B67: ;
    eax = MEM32(ebp);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(ebp + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(ebp + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C6B7E; /* jb: below (unsigned <) */

loc_003C6B6F: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C6B7Eu); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C6B7E: ;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, edi (32-bit) */
    edx = MEM32(ebx + 4);
    MEM32(esp + 0x18) = edx;
    if (CMP_EQ(_fa, _fb)) goto loc_003C6B92; /* je: equal / zero */

loc_003C6B89: ;
    ecx = MEM32(esi + 4);
    MEM32(esp + 0x10) = ecx;
    goto loc_003C6B96;

loc_003C6B92: ;
    MEM32(esp + 0x10) = edi;

loc_003C6B96: ;
    MEM32(esp + 0x14) = 2;
    ebx = 0x40100;

loc_003C6BA3: ;
    esi = MEM32(esp + 0x20);
    MEM32(eax) = ebx;
    MEM32(eax + 4) = edi;
    _fa = (uint32_t)(MEM32(esi + 0x10)) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(esi + 0x10), edi (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C6BFE; /* jne: not equal / not zero */

loc_003C6BB1: ;
    ecx = MEM32(esi + 0xC);
    esi = ZX8(MEM8(esi + 0xD));
    ecx = ecx >> 0x14;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ecx = ecx & 0xF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    edx = 1;
    edx = edx << LO8(ecx);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0xC) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0xC (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C6BF6; /* je: equal / zero */

loc_003C6BCA: ;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0xD) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0xD (32-bit) */
    if (CMP_LE(_fas, _fbs)) goto loc_003C6BDD; /* jle: less or equal (signed <=) */

loc_003C6BCF: ;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0xF) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0xF (32-bit) */
    if (CMP_G(_fas, _fbs)) goto loc_003C6BDD; /* jg: greater (signed >) */

loc_003C6BD4: ;
    esi = MEM32(esp + 0x20);
    edx = edx << 2;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    goto loc_003C6C06;

loc_003C6BDD: ;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(ecx, MEM8(esi + 0x3D4B68));
    esi = MEM32(esp + 0x20);
    ecx = ecx & 0x3C;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ecx = (uint32_t)((int32_t)ecx * (int32_t)edx);
    ecx = ecx >> 3;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edx = ecx;
    goto loc_003C6C06;

loc_003C6BF6: ;
    esi = MEM32(esp + 0x20);
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edx = edx + edx;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_003C6C06;

loc_003C6BFE: ;
    edx = ZX8(MEM8(esi + 0x13));
    edx++;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    edx = edx << 6;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */

loc_003C6C06: ;
    ecx = MEM32(ebp + 0x94C);
    MEM32(eax + 8) = 0x4020C;
    ecx = ecx << 0x10;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edx = edx & 0xFFFF;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ecx = ecx | edx;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(eax + 0xC) = ecx;
    edx = MEM32(esp + 0x18);
    MEM32(eax + 0x10) = ebx;
    MEM32(eax + 0x14) = edi;
    _fb = (uint32_t)(0x20) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x20;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx = 0x40110;
    MEM32(eax + -8) = ecx;
    MEM32(eax + -4) = edi;
    MEM32(eax) = ebx;
    MEM32(eax + 4) = edi;
    MEM32(eax + 8) = 0x40210;
    MEM32(eax + 0xC) = edx;
    edx = MEM32(esp + 0x10);
    MEM32(eax + 0x10) = ebx;
    MEM32(eax + 0x14) = edi;
    MEM32(eax + 0x18) = ecx;
    MEM32(eax + 0x1C) = edi;
    _fb = (uint32_t)(0x20) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x20;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(eax) = ebx;
    MEM32(eax + 4) = edi;
    MEM32(eax + 8) = 0x40214;
    MEM32(eax + 0xC) = edx;
    MEM32(eax + 0x10) = ebx;
    MEM32(eax + 0x14) = edi;
    MEM32(eax + 0x18) = ecx;
    ecx = MEM32(esp + 0x14);
    MEM32(eax + 0x1C) = edi;
    _fb = (uint32_t)(0x20) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x20;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx--;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    MEM32(esp + 0x14) = ecx;
    if ((_fa != 0)) goto loc_003C6BA3; /* jne: not equal / not zero */

loc_003C6C88: ;
    _fa = (uint32_t)(MEM32(0x3D8658)) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0x3D8658), edi (32-bit) */
    edx = 0x100001;
    if (CMP_EQ(_fa, _fb)) goto loc_003C6C9A; /* je: equal / zero */

loc_003C6C95: ;
    edx = 0x10100001;

loc_003C6C9A: ;
    _fa = (uint32_t)(MEM32(0x3D8614)) & 0xFFFFFFFFu; _fb = (uint32_t)(2) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0x3D8614), 2 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C6CA9; /* jne: not equal / not zero */

loc_003C6CA3: ;
    edx = edx | 0x10000;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */

loc_003C6CA9: ;
    ecx = MEM32(ebp + 0x1A08);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, edi (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C6CD1; /* je: equal / zero */

loc_003C6CB3: ;
    ecx = ZX8(MEM8(ecx + 0xD));
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x2D) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x2D (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C6CCB; /* je: equal / zero */

loc_003C6CBC: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x2B) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x2B (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C6CCB; /* je: equal / zero */

loc_003C6CC1: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x31) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x31 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C6CCB; /* je: equal / zero */

loc_003C6CC6: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x2F) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x2F (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C6CD1; /* jne: not equal / not zero */

loc_003C6CCB: ;
    edx = edx | 0x1000;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */

loc_003C6CD1: ;
    MEM32(eax) = 0x40290;
    MEM32(eax + 4) = edx;
    ecx = MEM32(0x3D8614);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, edi (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C6CF6; /* je: equal / zero */

loc_003C6CE7: ;
    _fa = (uint32_t)(MEM32(ebp + 0x1A08)) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(ebp + 0x1A08), edi (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C6CF6; /* je: equal / zero */

loc_003C6CEF: ;
    ecx = 1;
    goto loc_003C6CF8;

loc_003C6CF6: ;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */

loc_003C6CF8: ;
    MEM32(eax) = 0x4030C;
    MEM32(eax + 4) = ecx;
    _fa = (uint32_t)(MEM32(0x3D8618)) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0x3D8618), edi (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C6D18; /* je: equal / zero */

loc_003C6D09: ;
    _fa = (uint32_t)(MEM32(ebp + 0x1A08)) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(ebp + 0x1A08), edi (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C6D18; /* je: equal / zero */

loc_003C6D11: ;
    ecx = 1;
    goto loc_003C6D1A;

loc_003C6D18: ;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */

loc_003C6D1A: ;
    MEM32(eax + 8) = 0x4032C;
    MEM32(eax + 0xC) = ecx;
    ecx = MEM32(ebp + 8);
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x10;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(HI8(ecx)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(ecx), 1 (8-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003C6D4C; /* je: equal / zero */

loc_003C6D2F: ;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(ebp + 0x1A14)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, MEM32(ebp + 0x1A14) (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C6D4C; /* jne: not equal / not zero */

loc_003C6D37: ;
    ecx = ecx & 0xFFFFFEFFu;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(ebp + 8) = ecx;
    MEM32(eax) = 0x40130;
    MEM32(eax + 4) = edi;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C6D4C: ;
    esi = MEM32(esp + 0x24);
    ecx = MEM32(esp + 0x20);
    MEM32(ebp) = eax;
    PUSH32(esp, 0x003C6D5Cu); RECOMP_ABI_CALL(0x003CE750u, sub_003CE750); /* call 0x003CE750 */

loc_003C6D5C: ;
    MEM32(ebp + 0x1A0C) = eax;
    PUSH32(esp, 0x3D49C0);
    eax = ebp;
    PUSH32(esp, 0x003C6D6Eu); RECOMP_ABI_CALL(0x003C6490u, sub_003C6490); /* call 0x003C6490 */

loc_003C6D6E: ;
    eax = MEM32(0x3D8654);
    PUSH32(esp, eax);
    PUSH32(esp, 0x003C6D79u); RECOMP_ABI_CALL(0x003C5710u, sub_003C5710); /* call 0x003C5710 */

loc_003C6D79: ;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebp);
    POP32(esp, ebx);
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 12; return; /* ret 8 */

}

/**
 * sub_003C6D90
 * Original: 0x003C6D90 - 0x003C6E10 (128 bytes, 40 insns)
 * Category: game_render
 * CC: cdecl, 2 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C6D90(void)
{
    RECOMP_ENTRY(0x003C6D90u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003C6D90: ;
    edx = MEM32(0x3D83C8);
    PUSH32(esp, ebx);
    ebx = MEM32(edx + 0x2478);
    eax = edx + 0x1C28;
    edx = MEM32(edx + 0x240C);
    PUSH32(esp, ebp);
    ebp = MEM32(esp + 0x10);
    edx = edx + edx * 2;
    PUSH32(esp, esi);
    edx = edx << 8;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, edi);
    ebx = ebx & 1;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    esi = edx + eax + 0x1DC;
    ecx = 0xC0;
    edi = ebp;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _flags = ((_fa == 0)) ? 1 : 0; /* ZF in: a zero count keeps it */
    { int32_t _st = RECOMP_DF_STEP(4);
    while (ecx != 0) {
        _flags = (MEM32(esi) == MEM32(edi));
        esi += _st; edi += _st; ecx--;
        if (!_flags) break;
    } } /* repe cmpsd */
    if ((_flags != 0)) goto loc_003C6E09; /* je: equal / zero */

loc_003C6DCD: ;
    ecx = ebx + ebx * 2;
    ecx = ecx << 8;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edx = ecx + eax + 0x1DC;
    ecx = 0xC0;
    esi = ebp;
    edi = edx;
    if (!g_df) { uint8_t *_d = (uint8_t*)XBOX_PTR(edi), *_s = (uint8_t*)XBOX_PTR(esi); uint32_t _n = ecx * 4;
      if (_d + _n <= _s || _s + _n <= _d) memcpy(_d, _s, _n);
      else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*4) = MEM32(esi + _i*4); }
      esi += ecx * 4; edi += ecx * 4; }
    else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi - _i*4) = MEM32(esi - _i*4); esi -= ecx * 4; edi -= ecx * 4; }
    ecx = 0; /* rep movsd */
    _fa = (uint32_t)(MEM8(esp + 0x14)) & 0xFFu; _fb = (uint32_t)(2) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test MEM8(esp + 0x14), 2 (8-bit) */
    MEM32(eax + 0x7E4) = ebx;
    if (TEST_Z(_fa, _fb)) goto loc_003C6DFE; /* je: equal / zero */

loc_003C6DF2: ;
    PUSH32(esp, 0x003C6DF7u); RECOMP_ABI_CALL(0x003D0486u, sub_003D0486); /* call 0x003D0486 */

loc_003C6DF7: ;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebp);
    POP32(esp, ebx);
    esp += 12; return; /* ret 8 */

loc_003C6DFE: ;
    MEM32(eax + ebx * 4 + 0x7DC) = 1;

loc_003C6E09: ;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebp);
    POP32(esp, ebx);
    esp += 12; return; /* ret 8 */

}

/**
 * sub_003C6E10
 * Original: 0x003C6E10 - 0x003C7151 (833 bytes, 269 insns)
 * Category: game_render
 * CC: cdecl, 5 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C6E10(void)
{
    RECOMP_ENTRY(0x003C6E10u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003C6E10: ;
    _fb = (uint32_t)(0x34) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x34;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    PUSH32(esp, esi);
    esi = MEM32(esp + 0x44);
    ebp = ZX8(MEM8(esi + 0xD));
    SET_LO8(ebx, MEM8(ebp + 0x3D4B68));
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(eax, LO8(ebx));
    PUSH32(esp, edi);
    eax = eax >> 3;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    eax = eax & 7;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(esp + 0x18) = eax;
    PUSH32(esp, 0x003C6E38u); RECOMP_ABI_CALL(0x003C68F0u, sub_003C68F0); /* call 0x003C68F0 */

loc_003C6E38: ;
    esi = MEM32(esp + 0x54);
    edi = eax;
    PUSH32(esp, 0x003C6E43u); RECOMP_ABI_CALL(0x003C68F0u, sub_003C68F0); /* call 0x003C68F0 */

loc_003C6E43: ;
    _fa = (uint32_t)(LO8(ebx)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ebx), 1 (8-bit) */
    esi = eax;
    if (TEST_NZ(_fa, _fb)) goto loc_003C6E63; /* jne: not equal / not zero */

loc_003C6E4A: ;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(0xC) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, 0xC (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C6E63; /* je: equal / zero */

loc_003C6E4F: ;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(0xD) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, 0xD (32-bit) */
    if (CMP_LE(_fas, _fbs)) goto loc_003C6E59; /* jle: less or equal (signed <=) */

loc_003C6E54: ;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(0xF) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, 0xF (32-bit) */
    if (CMP_LE(_fas, _fbs)) goto loc_003C6E63; /* jle: less or equal (signed <=) */

loc_003C6E59: ;
    MEM32(esp + 0x14) = 0;
    goto loc_003C6E6B;

loc_003C6E63: ;
    MEM32(esp + 0x14) = 1;

loc_003C6E6B: ;
    ebp = MEM32(0x3D83C8);
    eax = MEM32(esp + 0x50);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    ebx = MEM32(ebp + 0x192C);
    MEM32(esp + 0x28) = ebp;
    if (CMP_NE(_fa, _fb)) goto loc_003C6E8B; /* jne: not equal / not zero */

loc_003C6E83: ;
    MEM32(esp + 0x50) = 1;

loc_003C6E8B: ;
    eax = MEM32(esp + 0x14);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C6EE4; /* je: equal / zero */

loc_003C6E93: ;
    edx = MEM32(esp + 0x48);
    PUSH32(esp, 0x003C6E9Cu); RECOMP_ABI_CALL(0x003C6950u, sub_003C6950); /* call 0x003C6950 */

loc_003C6E9C: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x1FC0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x1FC0 (32-bit) */
    if (CMP_A(_fa, _fb)) goto loc_003C6EB9; /* ja: above (unsigned >) */

loc_003C6EA3: ;
    edi = eax + 0x3F;
    edi = edi & 0xFFFFFFC0u;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    esi = edi;
    MEM32(esp + 0x10) = eax;
    MEM32(esp + 0x1C) = 1;
    goto loc_003C6ECB;

loc_003C6EB9: ;
    esi = 0x1000;
    eax = eax >> 0xC;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edi = esi;
    MEM32(esp + 0x10) = esi;
    MEM32(esp + 0x1C) = eax;

loc_003C6ECB: ;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C6ED6; /* jne: not equal / not zero */

loc_003C6ECF: ;
    eax = 1;
    goto loc_003C6EEC;

loc_003C6ED6: ;
    eax = MEM32(esp + 0x10);
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    { uint64_t _dividend = ((uint64_t)edx << 32) | eax;
      eax = (uint32_t)(_dividend / (uint32_t)MEM32(esp + 0x18));
      edx = (uint32_t)(_dividend % (uint32_t)MEM32(esp + 0x18)); }
    MEM32(esp + 0x10) = eax;

loc_003C6EE4: ;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C6F05; /* jne: not equal / not zero */

loc_003C6EE8: ;
    eax = MEM32(esp + 0x18);

loc_003C6EEC: ;
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa == 0)) goto loc_003C6F00; /* je: equal / zero */

loc_003C6EEF: ;
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa == 0)) goto loc_003C6EF9; /* je: equal / zero */

loc_003C6EF2: ;
    ebx = 0xA;
    goto loc_003C6F05;

loc_003C6EF9: ;
    ebx = 4;
    goto loc_003C6F05;

loc_003C6F00: ;
    ebx = 1;

loc_003C6F05: ;
    eax = MEM32(ebp);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(ebp + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(ebp + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C6F1C; /* jb: below (unsigned <) */

loc_003C6F0D: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C6F1Cu); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C6F1C: ;
    edx = MEM32(esp + 0x54);
    ecx = MEM32(edx + 4);
    edx = MEM32(esp + 0x48);
    edx = MEM32(edx + 4);
    MEM32(eax) = 0x86308;
    MEM32(eax + 4) = edx;
    MEM32(eax + 8) = ecx;
    esi = esi << 0x10;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edi = edi & 0xFFFF;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(eax + 0xC) = 0x86300;
    MEM32(eax + 0x10) = ebx;
    esi = esi | edi;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(eax + 0x14) = esi;
    _fb = (uint32_t)(0x18) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x18;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(ebp) = eax;

loc_003C6F54: ;
    eax = MEM32(esp + 0x50);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 1 (32-bit) */
    ebx = 1;
    if (CMP_A(_fa, _fb)) goto loc_003C6F64; /* ja: above (unsigned >) */

loc_003C6F62: ;
    ebx = eax;

loc_003C6F64: ;
    _fb = (uint32_t)(ebx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - ebx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    MEM32(esp + 0x50) = eax;
    eax = MEM32(esp + 0x4C);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C6F7D; /* je: equal / zero */

loc_003C6F72: ;
    esi = eax;
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x10;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esp + 0x4C) = eax;
    goto loc_003C6FFB;

loc_003C6F7D: ;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, 0 (32-bit) */
    eax = esp + 0x34;
    MEM32(esp + 0x18) = eax;
    if (CMP_BE(_fa, _fb)) goto loc_003C6FF7; /* jbe: below or equal (unsigned <=) */

loc_003C6F89: ;
    ebp = MEM32(esp + 0x48);
    edi = MEM32(ebp + 0x10);
    edx = esp + 0x40;
    esi = ebx;
    goto loc_003C6FA0;

    /* nop */
    /* nop */

loc_003C6FA0: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0 (32-bit) */
    MEM32(edx + -8) = 0;
    if (CMP_NE(_fa, _fb)) goto loc_003C6FBC; /* jne: not equal / not zero */

loc_003C6FAB: ;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(ecx, MEM8(ebp + 0xF));
    eax = 1;
    ecx = ecx & 0xF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax = eax << LO8(ecx);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    goto loc_003C6FC7;

loc_003C6FBC: ;
    eax = edi;
    eax = eax >> 0xC;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    eax = eax & 0xFFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_003C6FC7: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0 (32-bit) */
    MEM32(edx) = eax;
    MEM32(edx + -12) = 0;
    if (CMP_NE(_fa, _fb)) goto loc_003C6FE6; /* jne: not equal / not zero */

loc_003C6FD4: ;
    ecx = MEM32(ebp + 0xC);
    ecx = ecx >> 0x14;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ecx = ecx & 0xF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax = 1;
    eax = eax << LO8(ecx);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    goto loc_003C6FEE;

loc_003C6FE6: ;
    eax = edi;
    eax = eax & 0xFFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_003C6FEE: ;
    MEM32(edx + -4) = eax;
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edx = edx + 0x10;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esi--;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa != 0)) goto loc_003C6FA0; /* jne: not equal / not zero */

loc_003C6FF7: ;
    esi = MEM32(esp + 0x18);

loc_003C6FFB: ;
    eax = MEM32(esp + 0x58);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C700E; /* je: equal / zero */

loc_003C7003: ;
    ebp = eax;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esp + 0x58) = eax;
    goto loc_003C7035;

loc_003C700E: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, 0 (32-bit) */
    ebp = esp + 0x2C;
    if (CMP_BE(_fa, _fb)) goto loc_003C7035; /* jbe: below or equal (unsigned <=) */

loc_003C7018: ;
    ecx = esi;
    /* nop */

loc_003C7020: ;
    edx = MEM32(ecx);
    MEM32(esp + eax * 8 + 0x2C) = edx;
    edx = MEM32(ecx + 4);
    MEM32(esp + eax * 8 + 0x30) = edx;
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 0x10;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ebx (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C7020; /* jb: below (unsigned <) */

loc_003C7035: ;
    eax = MEM32(esp + 0x14);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C7061; /* je: equal / zero */

loc_003C703D: ;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, 0 (32-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_003C7061; /* jbe: below or equal (unsigned <=) */

loc_003C7041: ;
    eax = esi + 8;
    ecx = ebx;

loc_003C7046: ;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    MEM32(eax + -8) = edx;
    MEM32(eax + -4) = edx;
    edx = MEM32(esp + 0x10);
    MEM32(eax) = edx;
    edx = MEM32(esp + 0x1C);
    MEM32(eax + 4) = edx;
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x10;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx--;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa != 0)) goto loc_003C7046; /* jne: not equal / not zero */

loc_003C7061: ;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, 0 (32-bit) */
    ecx = MEM32(0x3D83C8);
    MEM32(esp + 0x18) = ebp;
    MEM32(esp + 0x24) = ecx;
    if (CMP_EQ(_fa, _fb)) goto loc_003C7107; /* je: equal / zero */

loc_003C7077: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + 4;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esp + 0x20) = ebx;
    edi = edi;

loc_003C7080: ;
    eax = MEM32(ecx);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(ecx + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(ecx + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C7096; /* jb: below (unsigned <) */

loc_003C7087: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C7096u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C7096: ;
    edi = MEM32(esi);
    ebx = MEM32(esi + -4);
    ecx = edi;
    ecx = ecx << 0x10;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edx = ebx;
    edx = edx & 0xFFFF;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ecx = ecx | edx;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    edx = MEM32(ebp + 4);
    ebp = MEM32(ebp);
    ebp = ebp & 0xFFFF;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    edx = edx << 0x10;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edx = edx | ebp;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    ebp = MEM32(esi + 8);
    _fb = (uint32_t)(edi) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    ebp = ebp - edi;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    ebp = ebp << 0x10;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edi = ebp;
    ebp = MEM32(esi + 4);
    _fb = (uint32_t)(ebx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    ebp = ebp - ebx;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    ebp = ebp & 0xFFFF;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(eax) = 0xC4300;
    MEM32(eax + 4) = ecx;
    ecx = MEM32(esp + 0x24);
    edi = edi ^ ebp;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    ebp = MEM32(esp + 0x18);
    MEM32(eax + 8) = edx;
    MEM32(eax + 0xC) = edi;
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x10;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(ecx) = eax;
    eax = MEM32(esp + 0x20);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ebp = ebp + 8;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + 0x10;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    MEM32(esp + 0x18) = ebp;
    MEM32(esp + 0x20) = eax;
    if ((_fa != 0)) goto loc_003C7080; /* jne: not equal / not zero */

loc_003C7107: ;
    eax = MEM32(esp + 0x50);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C6F54; /* jne: not equal / not zero */

loc_003C7113: ;
    eax = MEM32(esp + 0x48);
    ecx = MEM32(eax + 0x14);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebp);
    POP32(esp, ebx);
    if (CMP_EQ(_fa, _fb)) goto loc_003C7124; /* je: equal / zero */

loc_003C7122: ;
    eax = ecx;

loc_003C7124: ;
    ecx = MEM32(esp + 0x18);
    edx = MEM32(ecx + 0x2C);
    MEM32(eax + 8) = edx;
    edx = MEM32(esp + 0x44);
    eax = MEM32(edx + 0x14);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C7145; /* je: equal / zero */

loc_003C7139: ;
    ecx = MEM32(ecx + 0x2C);
    MEM32(eax + 8) = ecx;
    _fb = (uint32_t)(0x34) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x34;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 24; return; /* ret 20 */

loc_003C7145: ;
    eax = MEM32(ecx + 0x2C);
    MEM32(edx + 8) = eax;
    _fb = (uint32_t)(0x34) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x34;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 24; return; /* ret 20 */

}

/**
 * sub_003C7160
 * Original: 0x003C7160 - 0x003C726E (270 bytes, 81 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C7160(void)
{
    RECOMP_ENTRY(0x003C7160u);
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

loc_003C7160: ;
    PUSH32(esp, ebx);
    ebx = MEM32(0x3D83C8);
    PUSH32(esp, esi);
    ecx = eax;
    ecx = ecx << 6;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, edi);
    edi = ecx + ebx + 0xC60;
    ecx = 0x10;
    esi = edx;
    if (!g_df) { uint8_t *_d = (uint8_t*)XBOX_PTR(edi), *_s = (uint8_t*)XBOX_PTR(esi); uint32_t _n = ecx * 4;
      if (_d + _n <= _s || _s + _n <= _d) memcpy(_d, _s, _n);
      else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*4) = MEM32(esi + _i*4); }
      esi += ecx * 4; edi += ecx * 4; }
    else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi - _i*4) = MEM32(esi - _i*4); esi -= ecx * 4; edi -= ecx * 4; }
    ecx = 0; /* rep movsd */
    esi = MEM32(0x3D83D0);
    ecx = MEM32(eax * 4 + 0x3D4BF8);
    esi = esi | ecx;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 1 (32-bit) */
    MEM32(0x3D83D0) = esi;
    if (CMP_NE(_fa, _fb)) goto loc_003C726A; /* jne: not equal / not zero */

loc_003C719C: ;
    fp_push(MEMF(edx + 0x2C)); /* fld float */
    esi = 0x3F800000;
    fp_push(MEMF(edx + 0x28)); /* fld float */
    g_fp_cmp = RECOMP_FCMP(fp_top(), fp_st1()); g_fp_cc = RECOMP_FCMP_CC(g_fp_cmp); fp_pop(); fp_pop(); /* fucompp  */
    eax = (eax & 0xFFFF0000u) | (uint32_t)(uint16_t)(((g_fp_top & 7u) << 11) | g_fp_cc); /* fnstsw ax <- fpu status */
    _fa = (uint32_t)(HI8(eax)) & 0xFFu; _fb = (uint32_t)(0x44) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(eax), 0x44 (8-bit) */
    if ((!RECOMP_PARITY8((_fa) & (_fb)))) goto loc_003C71F0; /* jnp: not parity */

loc_003C71B0: ;
    fp_push(MEMF(0x4148F0)); /* fld float */
    fp_push(MEMF(edx + 0x28)); /* fld float */
    g_fp_cmp = RECOMP_FCMP(fp_top(), fp_st1()); g_fp_cc = RECOMP_FCMP_CC(g_fp_cmp); fp_pop(); fp_pop(); /* fucompp  */
    eax = (eax & 0xFFFF0000u) | (uint32_t)(uint16_t)(((g_fp_top & 7u) << 11) | g_fp_cc); /* fnstsw ax <- fpu status */
    _fa = (uint32_t)(HI8(eax)) & 0xFFu; _fb = (uint32_t)(0x44) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(eax), 0x44 (8-bit) */
    if ((!RECOMP_PARITY8((_fa) & (_fb)))) goto loc_003C71F0; /* jnp: not parity */

loc_003C71C2: ;
    fp_push(MEMF(edx + 0x38)); /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() / MEMF(edx + 0x28)); /* fdiv dword ptr [edx + 0x28] */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(edx + 0x2C)); /* fmul dword ptr [edx + 0x2c] */
    fp_top() = RECOMP_FP_PC(MEMF(edx + 0x3C) - fp_top()); /* fsubr dword ptr [edx + 0x3c] */
    MEMF(ebx + 0x93C) = (float)fp_top(); fp_pop(); /* fstp */
    fp_push(MEMF(edx + 0x3C)); /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() - MEMF(edx + 0x38)); /* fsub dword ptr [edx + 0x38] */
    fp_push(MEMF(edx + 0x28)); /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() - MEMF(edx + 0x2C)); /* fsub dword ptr [edx + 0x2c] */
    fp_st1() = RECOMP_FP_PC(fp_st1() / fp_top()); fp_pop(); /* fdivp st(1) */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(edx + 0x2C)); /* fmul dword ptr [edx + 0x2c] */
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(edx + 0x3C)); /* fadd dword ptr [edx + 0x3c] */
    MEMF(ebx + 0x940) = (float)fp_top(); fp_pop(); /* fstp */
    goto loc_003C7200;

loc_003C71F0: ;
    MEM32(ebx + 0x93C) = 0;
    MEM32(ebx + 0x940) = esi;

loc_003C7200: ;
    fp_push(MEMF(0x4148C8)); /* fld float */
    eax = MEM32(ebx + 8);
    fp_top() = RECOMP_FP_PC(fp_top() / MEMF(ebx + 0x940)); /* fdiv dword ptr [ebx + 0x940] */
    eax = eax & 0xFFFFFFFDu;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(ebx + 8) = eax;
    ecx = eax;
    MEMF(ebx + 0x944) = (float)fp_top(); fp_pop(); /* fstp */
    fp_push(MEMF(0x4148F0)); /* fld float */
    fp_push(MEMF(edx + 0xC)); /* fld float */
    g_fp_cmp = RECOMP_FCMP(fp_top(), fp_st1()); g_fp_cc = RECOMP_FCMP_CC(g_fp_cmp); fp_pop(); fp_pop(); /* fucompp  */
    eax = (eax & 0xFFFF0000u) | (uint32_t)(uint16_t)(((g_fp_top & 7u) << 11) | g_fp_cc); /* fnstsw ax <- fpu status */
    _fa = (uint32_t)(HI8(eax)) & 0xFFu; _fb = (uint32_t)(0x44) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(eax), 0x44 (8-bit) */
    if (RECOMP_PARITY8((_fa) & (_fb))) goto loc_003C725E; /* jp: parity */

loc_003C722F: ;
    fp_push(MEMF(0x4148F0)); /* fld float */
    fp_push(MEMF(edx + 0x1C)); /* fld float */
    g_fp_cmp = RECOMP_FCMP(fp_top(), fp_st1()); g_fp_cc = RECOMP_FCMP_CC(g_fp_cmp); fp_pop(); fp_pop(); /* fucompp  */
    eax = (eax & 0xFFFF0000u) | (uint32_t)(uint16_t)(((g_fp_top & 7u) << 11) | g_fp_cc); /* fnstsw ax <- fpu status */
    _fa = (uint32_t)(HI8(eax)) & 0xFFu; _fb = (uint32_t)(0x44) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(eax), 0x44 (8-bit) */
    if (RECOMP_PARITY8((_fa) & (_fb))) goto loc_003C725E; /* jp: parity */

loc_003C7241: ;
    fp_push(MEMF(0x4148F0)); /* fld float */
    fp_push(MEMF(edx + 0x2C)); /* fld float */
    g_fp_cmp = RECOMP_FCMP(fp_top(), fp_st1()); g_fp_cc = RECOMP_FCMP_CC(g_fp_cmp); fp_pop(); fp_pop(); /* fucompp  */
    eax = (eax & 0xFFFF0000u) | (uint32_t)(uint16_t)(((g_fp_top & 7u) << 11) | g_fp_cc); /* fnstsw ax <- fpu status */
    _fa = (uint32_t)(HI8(eax)) & 0xFFu; _fb = (uint32_t)(0x44) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(eax), 0x44 (8-bit) */
    if (RECOMP_PARITY8((_fa) & (_fb))) goto loc_003C725E; /* jp: parity */

loc_003C7253: ;
    _fa = (uint32_t)(MEM32(edx + 0x3C)) & 0xFFFFFFFFu; _fb = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(edx + 0x3C), esi (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C725E; /* jne: not equal / not zero */

loc_003C7258: ;
    ecx = ecx | 2;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(ebx + 8) = ecx;

loc_003C725E: ;
    PUSH32(esp, 0x003C7263u); RECOMP_ABI_CALL(0x003C67B0u, sub_003C67B0); /* call 0x003C67B0 */

loc_003C7263: ;
    esi = ebx;
    PUSH32(esp, 0x003C726Au); RECOMP_ABI_CALL(0x003C8F10u, sub_003C8F10); /* call 0x003C8F10 */

loc_003C726A: ;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebx);
    esp += 4; return; /* ret */

    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}

/**
 * sub_003C7270
 * Original: 0x003C7270 - 0x003C7486 (534 bytes, 169 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C7270(void)
{
    RECOMP_ENTRY(0x003C7270u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    int _cf = 0; /* carry flag */
    #define fp_push(v) do { double _fp_value = (v); \
        g_fp_top = (g_fp_top + 7u) & 7u; \
        g_fp_stack[g_fp_top] = _fp_value; } while (0)
    #define fp_pop() (g_fp_top = (g_fp_top + 1u) & 7u)
    #define fp_top() g_fp_stack[g_fp_top]
    #define fp_st(i) g_fp_stack[(g_fp_top + (i)) & 7u]
    #define fp_st1() fp_st(1)
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003C7270: ;
    ecx = MEM32(esp + 4);
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(esp) < (uint32_t)(0xC));
    esp = esp - 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    if (CMP_EQ(_fa, _fb)) goto loc_003C7452; /* je: equal / zero */

loc_003C7286: ;
    eax = MEM32(esi + 0x1A14);
    PUSH32(esp, ebp);
    ebp = MEM32(esi + 0x1A04);
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, eax (32-bit) */
    _cf = (int)(_fa < _fb);
    PUSH32(esp, edi);
    if (CMP_NE(_fa, _fb)) goto loc_003C729E; /* jne: not equal / not zero */

loc_003C7298: ;
    ebp = MEM32(esi + 0x1A18);

loc_003C729E: ;
    edx = MEM32(ecx);
    _cf = 0; /* xor clears CF */
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edx (32-bit) */
    _cf = (int)(_fa < _fb);
    eax = _cf ? 0xFFFFFFFF : 0; /* sbb self (CF extend) */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sbb result */
    _cf = 0; /* logical op clears CF */
    eax = eax & edx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(esp + 0xC) = eax;
    _cf = 0; /* xor clears CF */
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    PUSH32(esp, ebx);
    ebx = MEM32(ecx + 4);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ebx (32-bit) */
    _cf = (int)(_fa < _fb);
    eax = _cf ? 0xFFFFFFFF : 0; /* sbb self (CF extend) */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sbb result */
    _cf = 0; /* logical op clears CF */
    eax = eax & ebx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(esp + 0x14) = eax;
    eax = MEM32(ebp + 0x10);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_NE(_fa, _fb)) goto loc_003C72D9; /* jne: not equal / not zero */

loc_003C72C3: ;
    ecx = MEM32(ebp + 0xC);
    if (0x14) _cf = (int)(((ecx) >> ((0x14) - 1)) & 1);
    ecx = ecx >> 0x14;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _cf = 0; /* logical op clears CF */
    ecx = ecx & 0xF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    edi = 1;
    if (LO8(ecx)) _cf = (int)(((edi) >> (32 - (LO8(ecx)))) & 1);
    edi = edi << LO8(ecx);
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ecx = MEM32(esp + 0x20);
    goto loc_003C72E2;

loc_003C72D9: ;
    edi = eax;
    _cf = 0; /* logical op clears CF */
    edi = edi & 0xFFF;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    edi++;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_003C72E2: ;
    ecx = MEM32(ecx + 8);
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(edx) + (uint64_t)(ecx)) >> 32) & 1);
    edx = edx + ecx;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, edi (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_B(_fa, _fb)) goto loc_003C730A; /* jb: below (unsigned <) */

loc_003C72EB: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_NE(_fa, _fb)) goto loc_003C7301; /* jne: not equal / not zero */

loc_003C72EF: ;
    ecx = MEM32(ebp + 0xC);
    if (0x14) _cf = (int)(((ecx) >> ((0x14) - 1)) & 1);
    ecx = ecx >> 0x14;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _cf = 0; /* logical op clears CF */
    ecx = ecx & 0xF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    edx = 1;
    if (LO8(ecx)) _cf = (int)(((edx) >> (32 - (LO8(ecx)))) & 1);
    edx = edx << LO8(ecx);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    goto loc_003C730A;

loc_003C7301: ;
    edx = eax;
    _cf = 0; /* logical op clears CF */
    edx = edx & 0xFFF;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    edx++;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_003C730A: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    edi = edx;
    if (CMP_NE(_fa, _fb)) goto loc_003C7321; /* jne: not equal / not zero */

loc_003C7310: ;
    _cf = 0; /* xor clears CF */
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(ecx, MEM8(ebp + 0xF));
    edx = 1;
    _cf = 0; /* logical op clears CF */
    ecx = ecx & 0xF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    if (LO8(ecx)) _cf = (int)(((edx) >> (32 - (LO8(ecx)))) & 1);
    edx = edx << LO8(ecx);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    goto loc_003C732D;

loc_003C7321: ;
    edx = eax;
    if (0xC) _cf = (int)(((edx) >> ((0xC) - 1)) & 1);
    edx = edx >> 0xC;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _cf = 0; /* logical op clears CF */
    edx = edx & 0xFFF;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    edx++;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_003C732D: ;
    ecx = MEM32(esp + 0x20);
    ecx = MEM32(ecx + 0xC);
    _fb = (uint32_t)(ebx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(ecx) + (uint64_t)(ebx)) >> 32) & 1);
    ecx = ecx + ebx;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, edx (32-bit) */
    _cf = (int)(_fa < _fb);
    POP32(esp, ebx);
    if (CMP_AE(_fa, _fb)) goto loc_003C733F; /* jae: above or equal (unsigned >=) */

loc_003C733B: ;
    eax = ecx;
    goto loc_003C735D;

loc_003C733F: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_NE(_fa, _fb)) goto loc_003C7354; /* jne: not equal / not zero */

loc_003C7343: ;
    _cf = 0; /* xor clears CF */
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(ecx, MEM8(ebp + 0xF));
    eax = 1;
    _cf = 0; /* logical op clears CF */
    ecx = ecx & 0xF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    if (LO8(ecx)) _cf = (int)(((eax) >> (32 - (LO8(ecx)))) & 1);
    eax = eax << LO8(ecx);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    goto loc_003C735D;

loc_003C7354: ;
    if (0xC) _cf = (int)(((eax) >> ((0xC) - 1)) & 1);
    eax = eax >> 0xC;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _cf = 0; /* logical op clears CF */
    eax = eax & 0xFFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_003C735D: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    MEM32(esp + 0x14) = edi;
    fp_push((double)SMEM32(esp + 0x14)); /* fild */
    ecx = eax;
    if (CMP_GE(_fas, _fbs)) goto loc_003C7371; /* jge: greater or equal (signed >=) */

loc_003C736B: ;
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148E4)); /* fadd dword ptr [0x4148e4] */

loc_003C7371: ;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esi + 0x95C)); /* fmul dword ptr [esi + 0x95c] */
    edx = MEM32(esi + 0x954);
    PUSH32(esp, ecx);
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C0)); /* fadd dword ptr [0x4148c0] */
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003C738Cu); RECOMP_ABI_CALL(0x003CC3C0u, sub_003CC3C0); /* call 0x003CC3C0 */

loc_003C738C: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edx (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_BE(_fa, _fb)) goto loc_003C73B3; /* jbe: below or equal (unsigned <=) */

loc_003C7390: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    MEM32(esp + 0x14) = edx;
    fp_push((double)SMEM32(esp + 0x14)); /* fild */
    if (CMP_GE(_fas, _fbs)) goto loc_003C73A2; /* jge: greater or equal (signed >=) */

loc_003C739C: ;
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148E4)); /* fadd dword ptr [0x4148e4] */

loc_003C73A2: ;
    fp_top() = RECOMP_FP_PC(fp_top() / MEMF(esi + 0x95C)); /* fdiv dword ptr [esi + 0x95c] */
    PUSH32(esp, ecx);
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003C73B1u); RECOMP_ABI_CALL(0x003CC3C0u, sub_003CC3C0); /* call 0x003CC3C0 */

loc_003C73B1: ;
    edi = eax;

loc_003C73B3: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    MEM32(esp + 0x14) = ecx;
    fp_push((double)SMEM32(esp + 0x14)); /* fild */
    if (CMP_GE(_fas, _fbs)) goto loc_003C73C5; /* jge: greater or equal (signed >=) */

loc_003C73BF: ;
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148E4)); /* fadd dword ptr [0x4148e4] */

loc_003C73C5: ;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esi + 0x960)); /* fmul dword ptr [esi + 0x960] */
    edx = MEM32(esi + 0x958);
    PUSH32(esp, ecx);
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C0)); /* fadd dword ptr [0x4148c0] */
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003C73E0u); RECOMP_ABI_CALL(0x003CC3C0u, sub_003CC3C0); /* call 0x003CC3C0 */

loc_003C73E0: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edx (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_BE(_fa, _fb)) goto loc_003C7407; /* jbe: below or equal (unsigned <=) */

loc_003C73E4: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    MEM32(esp + 0x14) = edx;
    fp_push((double)SMEM32(esp + 0x14)); /* fild */
    if (CMP_GE(_fas, _fbs)) goto loc_003C73F6; /* jge: greater or equal (signed >=) */

loc_003C73F0: ;
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148E4)); /* fadd dword ptr [0x4148e4] */

loc_003C73F6: ;
    fp_top() = RECOMP_FP_PC(fp_top() / MEMF(esi + 0x960)); /* fdiv dword ptr [esi + 0x960] */
    PUSH32(esp, ecx);
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003C7405u); RECOMP_ABI_CALL(0x003CC3C0u, sub_003CC3C0); /* call 0x003CC3C0 */

loc_003C7405: ;
    ecx = eax;

loc_003C7407: ;
    eax = MEM32(esp + 0xC);
    edx = MEM32(esp + 0x10);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(edi) < (uint32_t)(eax));
    edi = edi - eax;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    MEM32(esi + 0xEE0) = eax;
    eax = MEM32(esp + 0x1C);
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(ecx) < (uint32_t)(edx));
    ecx = ecx - edx;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    MEM32(esi + 0xEE8) = edi;
    MEM32(esi + 0xEE4) = edx;
    MEM32(esi + 0xEEC) = ecx;
    edx = MEM32(eax + 0x10);
    MEM32(esi + 0xEF0) = edx;
    eax = MEM32(eax + 0x14);
    MEM32(esi + 0xEF4) = eax;
    eax = MEM32(0x3D83D0);
    _cf = 0; /* logical op clears CF */
    eax = eax | 0x100;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    POP32(esp, edi);
    MEM32(0x3D83D0) = eax;
    POP32(esp, ebp);

loc_003C7452: ;
    PUSH32(esp, 0);
    PUSH32(esp, 0);
    PUSH32(esp, 0);
    PUSH32(esp, 0x003C745Du); RECOMP_ABI_CALL(0x003C7DB0u, sub_003C7DB0); /* call 0x003C7DB0 */

loc_003C745D: ;
    PUSH32(esp, 0x003C7462u); RECOMP_ABI_CALL(0x003C67B0u, sub_003C67B0); /* call 0x003C67B0 */

loc_003C7462: ;
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_B(_fa, _fb)) goto loc_003C7478; /* jb: below (unsigned <) */

loc_003C7469: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    if (1) _cf = (int)(((ecx) >> ((1) - 1)) & 1);
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C7478u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C7478: ;
    PUSH32(esp, 0x003C747Du); RECOMP_ABI_CALL(0x003C6100u, sub_003C6100); /* call 0x003C6100 */

loc_003C747D: ;
    MEM32(esi) = eax;
    POP32(esp, esi);
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(0xC)) >> 32) & 1);
    esp = esp + 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 8; return; /* ret 4 */

    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}

/**
 * sub_003C7490
 * Original: 0x003C7490 - 0x003C784C (956 bytes, 287 insns)
 * Category: game_render
 * CC: cdecl, 2 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C7490(void)
{
    RECOMP_ENTRY(0x003C7490u);
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

loc_003C7490: ;
    eax = MEM32(esp + 4);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    ebp = MEM32(0x3D83C8);
    ebx = MEM32(ebp + 0x7A8);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ebx (32-bit) */
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    if (CMP_B(_fa, _fb)) goto loc_003C760C; /* jb: below (unsigned <) */

loc_003C74AF: ;
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x10;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = eax & 0xFFFFFFF0u;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(esp + 0x10) = eax;
    eax = eax + eax * 8;
    PUSH32(esp, 0x24800000);
    eax = eax << 4;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, eax);
    PUSH32(esp, 0x003C74CAu); RECOMP_ABI_CALL(0x0032BD3Eu, sub_0032BD3E); /* call 0x0032BD3E */

loc_003C74CA: ;
    edi = eax;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0 (32-bit) */
    MEM32(esp + 0x14) = edi;
    if (CMP_NE(_fa, _fb)) goto loc_003C74E3; /* jne: not equal / not zero */

loc_003C74D4: ;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebp);
    eax = 0x8007000Eu;
    POP32(esp, ebx);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 12; return; /* ret 8 */

loc_003C74E3: ;
    esi = MEM32(ebp + 0x7A4);
    ecx = MEM32(ebp + 0x7AC);
    eax = edi;
    _fb = (uint32_t)(esi) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - esi;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C74FF; /* je: equal / zero */

loc_003C74F7: ;
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(ebp + 0x7AC) = ecx;

loc_003C74FF: ;
    ecx = MEM32(ebp + 0x7B0);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C7511; /* je: equal / zero */

loc_003C7509: ;
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(ebp + 0x7B0) = ecx;

loc_003C7511: ;
    ecx = MEM32(ebp + 0x7B4);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C7523; /* je: equal / zero */

loc_003C751B: ;
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(ebp + 0x7B4) = ecx;

loc_003C7523: ;
    ecx = MEM32(ebp + 0x7B8);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C7535; /* je: equal / zero */

loc_003C752D: ;
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(ebp + 0x7B8) = ecx;

loc_003C7535: ;
    ecx = MEM32(ebp + 0x7BC);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C7547; /* je: equal / zero */

loc_003C753F: ;
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(ebp + 0x7BC) = ecx;

loc_003C7547: ;
    ecx = MEM32(ebp + 0x7C0);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C7559; /* je: equal / zero */

loc_003C7551: ;
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(ebp + 0x7C0) = ecx;

loc_003C7559: ;
    ecx = MEM32(ebp + 0x7C4);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C756B; /* je: equal / zero */

loc_003C7563: ;
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(ebp + 0x7C4) = ecx;

loc_003C756B: ;
    ecx = MEM32(ebp + 0x7C8);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C757D; /* je: equal / zero */

loc_003C7575: ;
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(ebp + 0x7C8) = ecx;

loc_003C757D: ;
    esi = MEM32(ebp + 0x7A4);
    eax = ebx + ebx * 8;
    eax = eax << 4;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ecx = eax;
    edx = ecx;
    ecx = ecx >> 2;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    if (!g_df) { uint8_t *_d = (uint8_t*)XBOX_PTR(edi), *_s = (uint8_t*)XBOX_PTR(esi); uint32_t _n = ecx * 4;
      if (_d + _n <= _s || _s + _n <= _d) memcpy(_d, _s, _n);
      else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*4) = MEM32(esi + _i*4); }
      esi += ecx * 4; edi += ecx * 4; }
    else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi - _i*4) = MEM32(esi - _i*4); esi -= ecx * 4; edi -= ecx * 4; }
    ecx = 0; /* rep movsd */
    ecx = edx;
    edx = MEM32(esp + 0x10);
    ecx = ecx & 3;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    if (!g_df) { uint8_t *_d = (uint8_t*)XBOX_PTR(edi), *_s = (uint8_t*)XBOX_PTR(esi); uint32_t _n = ecx;
      if (_d + _n <= _s || _s + _n <= _d) memcpy(_d, _s, _n);
      else { uint32_t _i; for (_i = 0; _i < _n; _i++) _d[_i] = _s[_i]; }
      esi += ecx; edi += ecx; }
    else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM8(edi - _i) = MEM8(esi - _i); esi -= ecx; edi -= ecx; }
    ecx = 0; /* rep movsb */
    ecx = MEM32(esp + 0x14);
    esi = eax + ecx;
    _fb = (uint32_t)(ebx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    edx = edx - ebx;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    ecx = edx + edx * 8;
    ecx = ecx << 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ebx = ecx;
    ecx = ecx >> 2;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    edi = esi;
    { uint32_t _i; int32_t _st = RECOMP_DF_STEP(4); for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*_st) = eax; edi += ecx * _st; }
    ecx = 0; /* rep stosd */
    ecx = ebx;
    ecx = ecx & 3;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    if (!g_df) { memset((void*)XBOX_PTR(edi), (uint8_t)eax, ecx); edi += ecx; }
    else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM8(edi - _i) = LO8(eax); edi -= ecx; }
    ecx = 0; /* rep stosb */
    if (CMP_BE(_fa, _fb)) goto loc_003C75DF; /* jbe: below or equal (unsigned <=) */

loc_003C75C2: ;
    _fb = (uint32_t)(0x8C) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + 0x8C;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_003C75D0;

    /* nop */

loc_003C75D0: ;
    MEM32(esi) = 0xFFFFFFFFu;
    _fb = (uint32_t)(0x90) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + 0x90;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edx--;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa != 0)) goto loc_003C75D0; /* jne: not equal / not zero */

loc_003C75DF: ;
    eax = MEM32(ebp + 0x7A4);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C75F4; /* je: equal / zero */

loc_003C75E9: ;
    PUSH32(esp, 0x24800000);
    PUSH32(esp, eax);
    PUSH32(esp, 0x003C75F4u); RECOMP_ABI_CALL(0x0032BDDEu, sub_0032BDDE); /* call 0x0032BDDE */

loc_003C75F4: ;
    eax = MEM32(esp + 0x14);
    edx = MEM32(esp + 0x10);
    MEM32(ebp + 0x7A4) = eax;
    eax = MEM32(esp + 0x1C);
    MEM32(ebp + 0x7A8) = edx;

loc_003C760C: ;
    edx = MEM32(ebp + 0x7A4);
    esi = MEM32(esp + 0x20);
    ebx = eax + eax * 8;
    ebx = ebx << 4;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ebx = ebx + edx;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx = 0x1A;
    edi = ebx;
    if (!g_df) { uint8_t *_d = (uint8_t*)XBOX_PTR(edi), *_s = (uint8_t*)XBOX_PTR(esi); uint32_t _n = ecx * 4;
      if (_d + _n <= _s || _s + _n <= _d) memcpy(_d, _s, _n);
      else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*4) = MEM32(esi + _i*4); }
      esi += ecx * 4; edi += ecx * 4; }
    else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi - _i*4) = MEM32(esi - _i*4); esi -= ecx * 4; edi -= ecx * 4; }
    ecx = 0; /* rep movsd */
    ecx = MEM32(ebx + 0x68);
    eax = MEM32(ebx);
    ecx = ecx | 3;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 1 (32-bit) */
    MEM32(ebx + 0x68) = ecx;
    if (CMP_EQ(_fa, _fb)) goto loc_003C77F9; /* je: equal / zero */

loc_003C763B: ;
    fp_push(MEMF(ebx + 0x40)); /* fld float */
    fp_top() = -fp_top(); /* fchs */
    MEMF(ebx + 0x6C) = (float)fp_top(); fp_pop(); /* fstp */
    fp_push(MEMF(ebx + 0x44)); /* fld float */
    fp_top() = -fp_top(); /* fchs */
    MEMF(ebx + 0x70) = (float)fp_top(); fp_pop(); /* fstp */
    fp_push(MEMF(ebx + 0x48)); /* fld float */
    fp_top() = -fp_top(); /* fchs */
    MEMF(ebx + 0x74) = (float)fp_top(); fp_pop(); /* fstp */
    fp_push(MEMF(ebx + 0x74)); /* fld float */
    fp_push(MEMF(ebx + 0x70)); /* fld float */
    fp_push(MEMF(ebx + 0x6C)); /* fld float */
    { double _t = fp_top(); fp_push(_t); } /* fld st(0) */
    fp_top() = RECOMP_FP_PC(fp_top() * fp_st1()); /* fmul st(1) */
    { double _t = g_fp_stack[(g_fp_top + 2) & 7]; fp_push(_t); } /* fld st(2) */
    fp_top() = RECOMP_FP_PC(fp_top() * g_fp_stack[(g_fp_top + 3) & 7]); /* fmul st(3) */
    fp_st1() = RECOMP_FP_PC(fp_st1() + fp_top()); fp_pop(); /* faddp st(1) */
    { double _t = g_fp_stack[(g_fp_top + 3) & 7]; fp_push(_t); } /* fld st(3) */
    fp_top() = RECOMP_FP_PC(fp_top() * g_fp_stack[(g_fp_top + 4) & 7]); /* fmul st(4) */
    fp_st1() = RECOMP_FP_PC(fp_st1() + fp_top()); fp_pop(); /* faddp st(1) */
    MEMF(esp + 0x20) = (float)fp_top(); fp_pop(); /* fstp */
    fp_pop(); /* fstp st(0) */
    fp_pop(); /* fstp st(0) */
    fp_pop(); /* fstp st(0) */
    eax = 0xBE800000u;
    _fb = (uint32_t)(MEM32(esp + 0x20)) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - MEM32(esp + 0x20);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    eax = (uint32_t)(((int32_t)(int32_t)(eax)) >> ((1) & 31u));
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sar result */
    MEM32(esp + 0x1C) = eax;
    fp_push(MEMF(0x452860)); /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x20)); /* fmul dword ptr [esp + 0x20] */
    fp_push(MEMF(esp + 0x1C)); /* fld float */
    { double _t = fp_top(); fp_push(_t); } /* fld st(0) */
    fp_top() = RECOMP_FP_PC(fp_top() * fp_st1()); /* fmul st(1) */
    fp_push(MEMF(0x452864)); /* fld float */
    { double _t = fp_top(); fp_top() = g_fp_stack[(g_fp_top + 3) & 7]; g_fp_stack[(g_fp_top + 3) & 7] = _t; } /* fxch st(3) */
    fp_st1() = RECOMP_FP_PC(fp_st1() * fp_top()); fp_pop(); /* fmulp st(1) */
    g_fp_stack[(g_fp_top + 2) & 7] = RECOMP_FP_PC(g_fp_stack[(g_fp_top + 2) & 7] - fp_top()); fp_pop(); /* fsubp st(2) */
    fp_st1() = RECOMP_FP_PC(fp_st1() * fp_top()); fp_pop(); /* fmulp st(1) */
    MEMF(esp + 0x1C) = (float)fp_top(); fp_pop(); /* fstp */
    ecx = MEM32(esp + 0x1C);
    ecx = ecx & 0x7FFFFFFF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(esp + 0x1C) = ecx;
    fp_push(MEMF(esp + 0x1C)); /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x1C)); /* fmul dword ptr [esp + 0x1c] */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x20)); /* fmul dword ptr [esp + 0x20] */
    fp_top() = RECOMP_FP_PC(MEMF(0x4148D4) - fp_top()); /* fsubr dword ptr [0x4148d4] */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x1C)); /* fmul dword ptr [esp + 0x1c] */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x4148C0)); /* fmul dword ptr [0x4148c0] */
    { double _t = fp_top(); fp_push(_t); } /* fld st(0) */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(ebx + 0x6C)); /* fmul dword ptr [ebx + 0x6c] */
    MEMF(ebx + 0x6C) = (float)fp_top(); fp_pop(); /* fstp */
    { double _t = fp_top(); fp_push(_t); } /* fld st(0) */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(ebx + 0x70)); /* fmul dword ptr [ebx + 0x70] */
    MEMF(ebx + 0x70) = (float)fp_top(); fp_pop(); /* fstp */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(ebx + 0x74)); /* fmul dword ptr [ebx + 0x74] */
    MEMF(ebx + 0x74) = (float)fp_top(); fp_pop(); /* fstp */
    eax = MEM32(ebx);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(2) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 2 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C77F9; /* jne: not equal / not zero */

loc_003C76F4: ;
    eax = MEM32(ebx + 0x50);
    edx = ebx + 0x7C;
    esi = ebx + 0x78;
    PUSH32(esp, eax);
    PUSH32(esp, 0x003C7703u); RECOMP_ABI_CALL(0x003CCB80u, sub_003CCB80); /* call 0x003CCB80 */

loc_003C7703: ;
    fp_push(MEMF(esi)); /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C8)); /* fadd dword ptr [0x4148c8] */
    PUSH32(esp, ecx);
    fp_top() = RECOMP_FP_PC(fp_top() - MEMF(edx)); /* fsub dword ptr [edx] */
    MEMF(ebx + 0x80) = (float)fp_top(); fp_pop(); /* fstp */
    fp_push(MEMF(ebx + 0x60)); /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x4148C0)); /* fmul dword ptr [0x4148c0] */
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003C7725u); RECOMP_ABI_CALL(0x003CC3D0u, sub_003CC3D0); /* call 0x003CC3D0 */

loc_003C7725: ;
    MEMF(esp + 0x20) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, ecx);
    fp_push(MEMF(ebx + 0x64)); /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x4148C0)); /* fmul dword ptr [0x4148c0] */
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003C773Bu); RECOMP_ABI_CALL(0x003CC3D0u, sub_003CC3D0); /* call 0x003CC3D0 */

loc_003C773B: ;
    MEMF(esp + 0x1C) = (float)fp_top(); /* fst */
    g_fp_cmp = RECOMP_FCMP(fp_top(), MEMF(esp + 0x20)); g_fp_cc = RECOMP_FCMP_CC(g_fp_cmp); fp_pop(); /* fcomp dword ptr [esp + 0x20] */
    eax = (eax & 0xFFFF0000u) | (uint32_t)(uint16_t)(((g_fp_top & 7u) << 11) | g_fp_cc); /* fnstsw ax <- fpu status */
    _fa = (uint32_t)(HI8(eax)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(eax), 1 (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C7758; /* jne: not equal / not zero */

loc_003C774A: ;
    fp_push(MEMF(esp + 0x20)); /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x414D20)); /* fmul dword ptr [0x414d20] */
    MEMF(esp + 0x1C) = (float)fp_top(); fp_pop(); /* fstp */

loc_003C7758: ;
    fp_push(MEMF(esp + 0x20)); /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() - MEMF(esp + 0x1C)); /* fsub dword ptr [esp + 0x1c] */
    MEMF(esp + 0x10) = (float)fp_top(); /* fst */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x10)); /* fmul dword ptr [esp + 0x10] */
    MEMF(esp + 0x14) = (float)fp_top(); fp_pop(); /* fstp */
    eax = 0xBE800000u;
    _fb = (uint32_t)(MEM32(esp + 0x14)) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - MEM32(esp + 0x14);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    eax = (uint32_t)(((int32_t)(int32_t)(eax)) >> ((1) & 31u));
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sar result */
    MEM32(esp + 0x20) = eax;
    fp_push(MEMF(0x452860)); /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x14)); /* fmul dword ptr [esp + 0x14] */
    fp_push(MEMF(esp + 0x20)); /* fld float */
    { double _t = fp_top(); fp_push(_t); } /* fld st(0) */
    fp_top() = RECOMP_FP_PC(fp_top() * fp_st1()); /* fmul st(1) */
    fp_push(MEMF(0x452864)); /* fld float */
    { double _t = fp_top(); fp_top() = g_fp_stack[(g_fp_top + 3) & 7]; g_fp_stack[(g_fp_top + 3) & 7] = _t; } /* fxch st(3) */
    fp_st1() = RECOMP_FP_PC(fp_st1() * fp_top()); fp_pop(); /* fmulp st(1) */
    g_fp_stack[(g_fp_top + 2) & 7] = RECOMP_FP_PC(g_fp_stack[(g_fp_top + 2) & 7] - fp_top()); fp_pop(); /* fsubp st(2) */
    fp_st1() = RECOMP_FP_PC(fp_st1() * fp_top()); fp_pop(); /* fmulp st(1) */
    MEMF(esp + 0x20) = (float)fp_top(); fp_pop(); /* fstp */
    ecx = MEM32(esp + 0x20);
    edx = MEM32(esp + 0x10);
    ecx = ecx & 0x7FFFFFFF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(esp + 0x20) = ecx;
    fp_push(MEMF(esp + 0x20)); /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x20)); /* fmul dword ptr [esp + 0x20] */
    edx = edx & 0x80000000u;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x14)); /* fmul dword ptr [esp + 0x14] */
    fp_top() = RECOMP_FP_PC(MEMF(0x4148D4) - fp_top()); /* fsubr dword ptr [0x4148d4] */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x20)); /* fmul dword ptr [esp + 0x20] */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x4148C0)); /* fmul dword ptr [0x4148c0] */
    MEMF(esp + 0x20) = (float)fp_top(); fp_pop(); /* fstp */
    eax = MEM32(esp + 0x20);
    edx = edx | eax;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(esp + 0x20) = edx;
    fp_push(MEMF(esp + 0x20)); /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x1C)); /* fmul dword ptr [esp + 0x1c] */
    eax = edx;
    MEM32(ebx + 0x84) = eax;
    fp_top() = -fp_top(); /* fchs */
    MEMF(ebx + 0x88) = (float)fp_top(); fp_pop(); /* fstp */

loc_003C77F9: ;
    ecx = MEM32(ebx + 0x8C);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFFFFFFu) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0xFFFFFFFFu (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C7840; /* je: equal / zero */

loc_003C7804: ;
    esi = MEM32(0x3D83D0);
    ebx = MEM32(ebx);
    edx = 0x10000;
    edx = edx << LO8(ecx);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + ecx;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = 3;
    eax = eax << LO8(ecx);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    esi = esi | edx;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(0x3D83D0) = esi;
    esi = MEM32(ebp + 0x7CC);
    eax = ~eax;
    eax = eax & esi;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(3) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, 3 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C7835; /* jne: not equal / not zero */

loc_003C7833: ;
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */

loc_003C7835: ;
    ebx++;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    ebx = ebx << LO8(ecx);
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ebx = ebx | eax;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(ebp + 0x7CC) = ebx;

loc_003C7840: ;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebp);
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    POP32(esp, ebx);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 12; return; /* ret 8 */

    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}

/**
 * sub_003C7850
 * Original: 0x003C7850 - 0x003C799F (335 bytes, 94 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C7850(void)
{
    RECOMP_ENTRY(0x003C7850u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C7850: ;
    _fb = (uint32_t)(0x68) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x68;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, ebx);
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    ebx = eax;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 0x7A8)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, MEM32(esi + 0x7A8) (32-bit) */
    PUSH32(esp, edi);
    if (CMP_AE(_fa, _fb)) goto loc_003C7879; /* jae: above or equal (unsigned >=) */

loc_003C7866: ;
    ecx = MEM32(esi + 0x7A4);
    eax = ebx + ebx * 8;
    eax = eax << 4;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fa = (uint32_t)(MEM8(ecx + eax + 0x68)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test MEM8(ecx + eax + 0x68), 1 (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C78C1; /* jne: not equal / not zero */

loc_003C7879: ;
    edx = esp + 0xC;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    PUSH32(esp, edx);
    ecx = 0x1A;
    edi = esp + 0x10;
    { uint32_t _i; int32_t _st = RECOMP_DF_STEP(4); for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*_st) = eax; edi += ecx * _st; }
    ecx = 0; /* rep stosd */
    PUSH32(esp, ebx);
    MEM32(esp + 0x14) = 3;
    MEM32(esp + 0x5C) = 0x3F800000;
    MEM32(esp + 0x18) = 0x3F800000;
    MEM32(esp + 0x1C) = 0x3F800000;
    MEM32(esp + 0x20) = 0x3F800000;
    PUSH32(esp, 0x003C78B9u); RECOMP_ABI_CALL(0x003C7490u, sub_003C7490); /* call 0x003C7490 */

loc_003C78B9: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C7996; /* jne: not equal / not zero */

loc_003C78C1: ;
    edx = MEM32(esp + 0x78);
    eax = ebx + ebx * 8;
    ebx = MEM32(esi + 0x7A4);
    eax = eax << 4;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edi = MEM32(eax + ebx + 0x68);
    ecx = MEM32(eax + ebx + 0x8C);
    _fb = (uint32_t)(ebx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + ebx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edi = edi | 4;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    MEM32(eax + 0x68) = edi;
    if (CMP_EQ(_fa, _fb)) goto loc_003C794C; /* je: equal / zero */

loc_003C78E8: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFFFFFFu) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0xFFFFFFFFu (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C7994; /* jne: not equal / not zero */

loc_003C78F1: ;
    ebx = MEM32(esi + 0x7CC);
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(LO8(ebx)) & 0xFFu; _fb = (uint32_t)(3) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ebx), 3 (8-bit) */
    edx = ebx;
    if (TEST_Z(_fa, _fb)) goto loc_003C7909; /* je: equal / zero */

loc_003C7900: ;
    edx = edx >> 2;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(LO8(edx)) & 0xFFu; _fb = (uint32_t)(3) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(edx), 3 (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C7900; /* jne: not equal / not zero */

loc_003C7909: ;
    edi = MEM32(0x3D83D0);
    edx = 0x10000;
    edx = edx << LO8(ecx);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    MEM32(eax + 0x8C) = ecx;
    MEM32(esi + ecx * 4 + 0x7AC) = eax;
    eax = MEM32(eax);
    edi = edi | edx;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(3) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 3 (32-bit) */
    MEM32(0x3D83D0) = edi;
    if (CMP_NE(_fa, _fb)) goto loc_003C7934; /* jne: not equal / not zero */

loc_003C7932: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */

loc_003C7934: ;
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + ecx;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = eax << LO8(ecx);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    POP32(esp, edi);
    eax = eax | ebx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(esi + 0x7CC) = eax;
    POP32(esp, esi);
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    POP32(esp, ebx);
    _fb = (uint32_t)(0x68) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x68;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 8; return; /* ret 4 */

loc_003C794C: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFFFFFFu) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0xFFFFFFFFu (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C7994; /* je: equal / zero */

loc_003C7951: ;
    ebx = MEM32(0x3D83D0);
    MEM32(eax + 0x8C) = 0xFFFFFFFFu;
    eax = 0x10000;
    eax = eax << LO8(ecx);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    MEM32(esi + ecx * 4 + 0x7AC) = 0;
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + ecx;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edx = 3;
    edx = edx << LO8(ecx);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ebx = ebx | eax;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    eax = MEM32(esi + 0x7CC);
    MEM32(0x3D83D0) = ebx;
    edx = ~edx;
    eax = eax & edx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(esi + 0x7CC) = eax;

loc_003C7994: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */

loc_003C7996: ;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebx);
    _fb = (uint32_t)(0x68) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x68;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C79A0
 * Original: 0x003C79A0 - 0x003C7B4F (431 bytes, 132 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C79A0(void)
{
    RECOMP_ENTRY(0x003C79A0u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003C79A0: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, ebx);
    ebx = MEM32(0x3D83C8);
    PUSH32(esp, ebp);
    ebp = MEM32(esp + 0x14);
    PUSH32(esp, esi);
    esi = eax;
    PUSH32(esp, edi);
    edi = MEM32(ebx + esi * 4 + 0xF88);
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0 (32-bit) */
    MEM32(esp + 0x14) = ebx;
    MEM32(esp + 0x10) = edi;
    if (CMP_EQ(_fa, _fb)) goto loc_003C79E4; /* je: equal / zero */

loc_003C79C6: ;
    eax = MEM32(ebx + 0x2C);
    ecx = MEM32(edi);
    _fb = (uint32_t)(0xFFF80000u) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 0xFFF80000u;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(edi + 8) = eax;
    eax = ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x78FFFF) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test eax, 0x78FFFF (32-bit) */
    MEM32(edi) = ecx;
    if (TEST_NZ(_fa, _fb)) goto loc_003C79E4; /* jne: not equal / not zero */

loc_003C79DF: ;
    PUSH32(esp, 0x003C79E4u); RECOMP_ABI_CALL(0x003C9EC0u, sub_003C9EC0); /* call 0x003C9EC0 */

loc_003C79E4: ;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, 0 (32-bit) */
    MEM32(ebx + esi * 4 + 0xF88) = ebp;
    if (CMP_NE(_fa, _fb)) goto loc_003C7A3F; /* jne: not equal / not zero */

loc_003C79EF: ;
    eax = MEM32(ebx);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(ebx + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(ebx + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C7A05; /* jb: below (unsigned <) */

loc_003C79F6: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C7A05u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C7A05: ;
    edx = esi;
    edx = edx << 6;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(0x41B0C) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edx = edx + 0x41B0C;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(eax) = edx;
    MEM32(eax + 4) = 0;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(ebx) = eax;
    eax = MEM32(0x3D83D0);
    POP32(esp, edi);
    MEM32(ebx + esi * 4 + 0xC) = 0x80000000u;
    POP32(esp, esi);
    eax = eax | 0x4800;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    POP32(esp, ebp);
    MEM32(0x3D83D0) = eax;
    POP32(esp, ebx);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 8; return; /* ret 4 */

loc_003C7A3F: ;
    _fb = (uint32_t)(0x80000) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    MEM32(ebp) = MEM32(ebp) + 0x80000;
    _fa = (uint32_t)(MEM32(ebp)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = MEM32(ebx);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(ebx + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(ebx + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C7A5C; /* jb: below (unsigned <) */

loc_003C7A4D: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C7A5Cu); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C7A5C: ;
    ecx = MEM32(ebp + 0xC);
    edx = MEM32(ebp + 4);
    ebx = esi + 0x206C;
    ebx = ebx << 6;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    MEM32(eax) = ebx;
    MEM32(eax + 4) = edx;
    MEM32(eax + 8) = ecx;
    ecx = MEM32(ebp + 0x10);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    ebx = eax + 0xC;
    if (CMP_EQ(_fa, _fb)) goto loc_003C7ACA; /* je: equal / zero */

loc_003C7A7D: ;
    edx = ZX8(MEM8(ebp + 0x13));
    eax = ecx;
    ecx = ecx >> 0xC;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    eax = eax & 0xFFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ecx = ecx & 0xFFF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(esp + 0x1C) = ecx;
    edx++;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    ecx = esi;
    ecx = ecx << 6;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edi = ecx + 0x41B10;
    edx = edx << 6;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    MEM32(ebx) = edi;
    edi = MEM32(esp + 0x10);
    edx = edx << 0x10;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(0x41B1C) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 0x41B1C;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(ebx + 4) = edx;
    MEM32(ebx + 8) = ecx;
    ecx = MEM32(esp + 0x1C);
    eax = eax << 0x10;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    eax = eax | ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(ebx + 0xC) = eax;
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ebx = ebx + 0x10;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C7ACA: ;
    ebp = MEM32(ebp + 0xC);
    ecx = MEM32(esp + 0x14);
    edx = MEM32(ecx + esi * 4 + 0xC);
    eax = ebp;
    eax = eax & 0x20F4;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, eax (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C7B43; /* je: equal / zero */

loc_003C7AE0: ;
    _fa = (uint32_t)(HI8(eax)) & 0xFFu; _fb = (uint32_t)(0x20) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(eax), 0x20 (8-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003C7B05; /* je: equal / zero */

loc_003C7AE5: ;
    ebp = ebp & 0xFF00;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax = eax & 0xFFFFDFFFu;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(0x2A00) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, 0x2A00 (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C7B05; /* jb: below (unsigned <) */

loc_003C7AF8: ;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(0x3100) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, 0x3100 (32-bit) */
    if (CMP_A(_fa, _fb)) goto loc_003C7B05; /* ja: above (unsigned >) */

loc_003C7B00: ;
    eax = eax | 0x40000000;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */

loc_003C7B05: ;
    edx = MEM32(0x3D83D0);
    edx = edx | 0x4000;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0 (32-bit) */
    MEM32(ecx + esi * 4 + 0xC) = eax;
    MEM32(0x3D83D0) = edx;
    if (CMP_NE(_fa, _fb)) goto loc_003C7B43; /* jne: not equal / not zero */

loc_003C7B1F: ;
    eax = MEM32(ecx + esi * 4 + 0x774);
    esi = esi << 6;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(0x41B0C) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + 0x41B0C;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(ebx) = esi;
    MEM32(ebx + 4) = eax;
    eax = edx;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ebx = ebx + 8;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = eax | 0x800;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(0x3D83D0) = eax;

loc_003C7B43: ;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebp);
    MEM32(ecx) = ebx;
    POP32(esp, ebx);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C7B50
 * Original: 0x003C7B50 - 0x003C7BE5 (149 bytes, 51 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C7B50(void)
{
    RECOMP_ENTRY(0x003C7B50u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003C7B50: ;
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    ebp = MEM32(esp + 0xC);
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    ebx = eax;
    eax = MEM32(esi + ebx * 4 + 0xF98);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    PUSH32(esp, edi);
    if (CMP_EQ(_fa, _fb)) goto loc_003C7B90; /* je: equal / zero */

loc_003C7B6B: ;
    ecx = MEM32(esi + 0x2C);
    MEM32(eax + 8) = ecx;
    edi = MEM32(esi + ebx * 4 + 0xF98);
    ecx = MEM32(edi);
    _fb = (uint32_t)(0xFFF80000u) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 0xFFF80000u;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x78FFFF) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test eax, 0x78FFFF (32-bit) */
    MEM32(edi) = ecx;
    if (TEST_NZ(_fa, _fb)) goto loc_003C7B90; /* jne: not equal / not zero */

loc_003C7B8B: ;
    PUSH32(esp, 0x003C7B90u); RECOMP_ABI_CALL(0x003C9EC0u, sub_003C9EC0); /* call 0x003C9EC0 */

loc_003C7B90: ;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, 0 (32-bit) */
    MEM32(esi + ebx * 4 + 0xF98) = ebp;
    if (CMP_EQ(_fa, _fb)) goto loc_003C7BDE; /* je: equal / zero */

loc_003C7B9B: ;
    edi = MEM32(ebp);
    _fb = (uint32_t)(0x80000) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edi = edi + 0x80000;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = edi;
    eax = eax >> 0x1C;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    MEM32(ebp) = edi;
    ecx = MEM32(esi + 4);
    edi = eax;
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ecx (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C7BC6; /* jb: below (unsigned <) */

loc_003C7BB7: ;
    eax = MEM32(0x3D9588);
    edx = eax;
    edx = edx >> 1;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, edx);
    PUSH32(esp, 0x003C7BC6u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C7BC6: ;
    ebp = MEM32(ebp + 4);
    ebx = ebx << 6;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(0x41B20) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ebx = ebx + 0x41B20;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ebp = ebp | edi;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(eax) = ebx;
    MEM32(eax + 4) = ebp;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi) = eax;

loc_003C7BDE: ;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebp);
    POP32(esp, ebx);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C7C50
 * Original: 0x003C7C50 - 0x003C7CAB (91 bytes, 31 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C7C50(void)
{
    RECOMP_ENTRY(0x003C7C50u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C7C50: ;
    PUSH32(esp, esi);
    PUSH32(esp, 0x003C7C56u); RECOMP_ABI_CALL(0x003C7BF0u, sub_003C7BF0); /* call 0x003C7BF0 */

loc_003C7C56: ;
    esi = eax;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C7C63; /* jne: not equal / not zero */

loc_003C7C5C: ;
    eax = 0x8007000Eu;
    POP32(esp, esi);
    esp += 4; return; /* ret */

loc_003C7C63: ;
    PUSH32(esp, edi);
    edi = MEM32(0x3D83C8);
    eax = MEM32(edi + 0x2C);
    MEM32(esi) = eax;
    eax = MEM32(edi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(edi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(edi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C7C85; /* jb: below (unsigned <) */

loc_003C7C76: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C7C85u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C7C85: ;
    MEM32(eax) = 0x817CC;
    esi = esi & 0xEFFFFFF;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(eax + 4) = 0;
    esi = esi | 0x1000000;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(eax + 8) = esi;
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0xC;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(edi) = eax;
    POP32(esp, edi);
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    POP32(esp, esi);
    esp += 4; return; /* ret */

}

/**
 * sub_003C7CB0
 * Original: 0x003C7CB0 - 0x003C7CD4 (36 bytes, 10 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C7CB0(void)
{
    RECOMP_ENTRY(0x003C7CB0u);
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C7CB0: ;
    eax = MEM32(0x3D83C8);
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, 0);
    PUSH32(esp, 0);
    PUSH32(esp, 1);
    MEM32(eax + 0x1DC0) = 0;
    PUSH32(esp, 6);
    _fb = (uint32_t)(0x1DBC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x1DBC;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, eax);
    { uint32_t _icall_target = MEM32(0x3E14D0); PUSH32(esp, 0x003C7CD3u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x003C7CCDu); } /* indirect call */
    }

loc_003C7CD3: ;
    esp += 4; return; /* ret */

}

/**
 * sub_003C7CE0
 * Original: 0x003C7CE0 - 0x003C7DA1 (193 bytes, 71 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C7CE0(void)
{
    RECOMP_ENTRY(0x003C7CE0u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003C7CE0: ;
    _fb = (uint32_t)(0x18) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x18;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    PUSH32(esp, ebx);
    ebx = MEM32(0x3D83C8);
    PUSH32(esp, esi);
    if (CMP_EQ(_fa, _fb)) goto loc_003C7D66; /* je: equal / zero */

loc_003C7CEF: ;
    edx = MEM32(ecx + 4);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C7D66; /* je: equal / zero */

loc_003C7CF6: ;
    esi = MEM32(ecx);
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0 (32-bit) */
    PUSH32(esp, ebp);
    PUSH32(esp, edi);
    edi = MEM32(ecx + 8);
    MEM32(esp + 0x18) = edi;
    edi = MEM32(ecx + 0xC);
    MEM32(esp + 0x1C) = edi;
    edi = MEM32(ecx + 0x10);
    ecx = MEM32(ecx + 0x14);
    if ((((uint32_t)((uint32_t)(_fas) - (uint32_t)(_fbs)) >> 31) != 0)) goto loc_003C7D16; /* js: sign (negative) */

loc_003C7D12: ;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    edi = 0; /* xor self */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */

loc_003C7D16: ;
    ebp = eax + eax * 2;
    ebx = ebx + ebp * 8 + 0x1AC0;
    ebp = ebx;
    ebx = MEM32(esp + 0x18);
    MEM32(ebp) = esi;
    MEM32(ebp + 4) = edx;
    PUSH32(esp, ecx);
    MEM32(ebp + 8) = ebx;
    ebx = MEM32(esp + 0x20);
    PUSH32(esp, edi);
    MEM32(ebp + 0xC) = ebx;
    PUSH32(esp, esi);
    MEM32(ebp + 0x10) = edi;
    MEM32(ebp + 0x14) = ecx;
    ecx = MEM32(esp + 0x24);
    PUSH32(esp, ebx);
    PUSH32(esp, ecx);
    ecx = MEM32(0x3D83C8);
    edx = edx & 0xFFFFFFF;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    PUSH32(esp, edx);
    _fb = (uint32_t)(0x1C28) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 0x1C28;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C7D5Eu); RECOMP_ABI_CALL(0x003CF53Fu, sub_003CF53F); /* call 0x003CF53F */

loc_003C7D5E: ;
    POP32(esp, edi);
    POP32(esp, ebp);
    POP32(esp, esi);
    POP32(esp, ebx);
    _fb = (uint32_t)(0x18) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x18;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 4; return; /* ret */

loc_003C7D66: ;
    edx = eax + eax * 2;
    ecx = MEM32(ebx + edx * 8 + 0x1AD4);
    esi = ebx + edx * 8;
    PUSH32(esp, ecx);
    _fb = (uint32_t)(0x1C28) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ebx = ebx + 0x1C28;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, ebx);
    ebx = eax;
    PUSH32(esp, 0x003C7D82u); RECOMP_ABI_CALL(0x003CF72Au, sub_003CF72A); /* call 0x003CF72A */

loc_003C7D82: ;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fb = (uint32_t)(0x1AC0) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + 0x1AC0;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi) = edx;
    MEM32(esi + 4) = edx;
    MEM32(esi + 8) = edx;
    MEM32(esi + 0xC) = edx;
    MEM32(esi + 0x10) = edx;
    MEM32(esi + 0x14) = edx;
    POP32(esp, esi);
    POP32(esp, ebx);
    _fb = (uint32_t)(0x18) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x18;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 4; return; /* ret */

}

/**
 * sub_003C7DB0
 * Original: 0x003C7DB0 - 0x003C809A (746 bytes, 211 insns)
 * Category: game_render
 * CC: cdecl, 3 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C7DB0(void)
{
    RECOMP_ENTRY(0x003C7DB0u);
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

loc_003C7DB0: ;
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, ebx);
    ebx = MEM32(0x3D83C8);
    eax = MEM32(ebx + 0x95C);
    ecx = MEM32(ebx + 0x960);
    PUSH32(esp, ebp);
    MEM32(esp + 8) = eax;
    eax = MEM32(ebx + 4);
    MEM32(esp + 0xC) = ecx;
    ecx = MEM32(ebx);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, eax (32-bit) */
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    if (CMP_B(_fa, _fb)) goto loc_003C7DEB; /* jb: below (unsigned <) */

loc_003C7DDA: ;
    eax = MEM32(0x3D9588);
    edx = eax;
    edx = edx >> 1;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, edx);
    PUSH32(esp, 0x003C7DE9u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C7DE9: ;
    ecx = eax;

loc_003C7DEB: ;
    esi = MEM32(esp + 0x20);
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C7EA7; /* jne: not equal / not zero */

loc_003C7DF7: ;
    eax = MEM32(ebx + 0xEE0);
    fp_push((double)SMEM32(ebx + 0xEE0)); /* fild */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_GE(_fas, _fbs)) goto loc_003C7E0D; /* jge: greater or equal (signed >=) */

loc_003C7E07: ;
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148E4)); /* fadd dword ptr [0x4148e4] */

loc_003C7E0D: ;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x10)); /* fmul dword ptr [esp + 0x10] */
    PUSH32(esp, ecx);
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C0)); /* fadd dword ptr [0x4148c0] */
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003C7E20u); RECOMP_ABI_CALL(0x003CC3C0u, sub_003CC3C0); /* call 0x003CC3C0 */

loc_003C7E20: ;
    edx = MEM32(ebx + 0xEE4);
    fp_push((double)SMEM32(ebx + 0xEE4)); /* fild */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    edi = eax;
    if (CMP_GE(_fas, _fbs)) goto loc_003C7E38; /* jge: greater or equal (signed >=) */

loc_003C7E32: ;
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148E4)); /* fadd dword ptr [0x4148e4] */

loc_003C7E38: ;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x14)); /* fmul dword ptr [esp + 0x14] */
    PUSH32(esp, ecx);
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C0)); /* fadd dword ptr [0x4148c0] */
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003C7E4Bu); RECOMP_ABI_CALL(0x003CC3C0u, sub_003CC3C0); /* call 0x003CC3C0 */

loc_003C7E4B: ;
    fp_push((double)SMEM32(ebx + 0xEE8)); /* fild */
    esi = eax;
    eax = MEM32(ebx + 0xEE8);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_GE(_fas, _fbs)) goto loc_003C7E63; /* jge: greater or equal (signed >=) */

loc_003C7E5D: ;
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148E4)); /* fadd dword ptr [0x4148e4] */

loc_003C7E63: ;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x10)); /* fmul dword ptr [esp + 0x10] */
    PUSH32(esp, ecx);
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C0)); /* fadd dword ptr [0x4148c0] */
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003C7E76u); RECOMP_ABI_CALL(0x003CC3C0u, sub_003CC3C0); /* call 0x003CC3C0 */

loc_003C7E76: ;
    fp_push((double)SMEM32(ebx + 0xEEC)); /* fild */
    edx = eax;
    eax = MEM32(ebx + 0xEEC);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_GE(_fas, _fbs)) goto loc_003C7E8E; /* jge: greater or equal (signed >=) */

loc_003C7E88: ;
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148E4)); /* fadd dword ptr [0x4148e4] */

loc_003C7E8E: ;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x14)); /* fmul dword ptr [esp + 0x14] */
    PUSH32(esp, ecx);
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C0)); /* fadd dword ptr [0x4148c0] */
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003C7EA1u); RECOMP_ABI_CALL(0x003CC3C0u, sub_003CC3C0); /* call 0x003CC3C0 */

loc_003C7EA1: ;
    ebp = MEM32(esp + 0x28);
    goto loc_003C7F22;

loc_003C7EA7: ;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 1 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C7F78; /* jne: not equal / not zero */

loc_003C7EB0: ;
    eax = MEM32(esp + 0x24);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C7F78; /* jne: not equal / not zero */

loc_003C7EBC: ;
    ebp = MEM32(esp + 0x28);
    fp_push((double)SMEM32(ebp)); /* fild */
    PUSH32(esp, ecx);
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x14)); /* fmul dword ptr [esp + 0x14] */
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C0)); /* fadd dword ptr [0x4148c0] */
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003C7ED6u); RECOMP_ABI_CALL(0x003CC3C0u, sub_003CC3C0); /* call 0x003CC3C0 */

loc_003C7ED6: ;
    fp_push((double)SMEM32(ebp + 4)); /* fild */
    PUSH32(esp, ecx);
    edi = eax;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x18)); /* fmul dword ptr [esp + 0x18] */
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C0)); /* fadd dword ptr [0x4148c0] */
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003C7EEEu); RECOMP_ABI_CALL(0x003CC3C0u, sub_003CC3C0); /* call 0x003CC3C0 */

loc_003C7EEE: ;
    fp_push((double)SMEM32(ebp + 8)); /* fild */
    PUSH32(esp, ecx);
    esi = eax;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x14)); /* fmul dword ptr [esp + 0x14] */
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C0)); /* fadd dword ptr [0x4148c0] */
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003C7F06u); RECOMP_ABI_CALL(0x003CC3C0u, sub_003CC3C0); /* call 0x003CC3C0 */

loc_003C7F06: ;
    fp_push((double)SMEM32(ebp + 0xC)); /* fild */
    edx = eax;
    PUSH32(esp, ecx);
    _fb = (uint32_t)(edi) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    edx = edx - edi;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x18)); /* fmul dword ptr [esp + 0x18] */
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C0)); /* fadd dword ptr [0x4148c0] */
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003C7F20u); RECOMP_ABI_CALL(0x003CC3C0u, sub_003CC3C0); /* call 0x003CC3C0 */

loc_003C7F20: ;
    _fb = (uint32_t)(esi) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - esi;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */

loc_003C7F22: ;
    MEM32(ecx) = 0x80200;
    eax = eax << 0x10;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edx = edx << 0x10;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    eax = eax | esi;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    esi = MEM32(esp + 0x20);
    edx = edx | edi;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(ecx + 4) = edx;
    MEM32(ecx + 8) = eax;
    MEM32(ecx + 0xC) = 0x402B4;
    MEM32(ecx + 0x10) = 0;
    eax = MEM32(ebx + 0x954);
    eax = eax << 0x10;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    MEM32(ecx + 0x14) = 0x402C0;
    MEM32(ecx + 0x18) = eax;
    eax = MEM32(ebx + 0x958);
    eax = eax << 0x10;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    MEM32(ecx + 0x1C) = 0x402E0;
    MEM32(ecx + 0x20) = eax;
    edx = ecx + 0x24;
    goto loc_003C805F;

loc_003C7F78: ;
    edx = MEM32(ebx + 0x954);
    eax = MEM32(ebx + 0x958);
    edx = edx << 0x10;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    MEM32(ecx) = 0x80200;
    MEM32(ecx + 4) = edx;
    edx = MEM32(esp + 0x24);
    eax = eax << 0x10;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0 (32-bit) */
    MEM32(ecx + 8) = eax;
    MEM32(ecx + 0xC) = 0x402B4;
    MEM32(ecx + 0x10) = edx;
    edx = ecx + 0x14;
    if (CMP_BE(_fa, _fb)) goto loc_003C805B; /* jbe: below or equal (unsigned <=) */

loc_003C7FAF: ;
    eax = edx;
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    ecx = MEM32(esp + 0x28);
    edi = 0x402E0;
    ecx = eax + ecx + -16;
    ebp = esi;

loc_003C7FC2: ;
    fp_push((double)SMEM32(ecx + -4)); /* fild */
    PUSH32(esp, ecx);
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x14)); /* fmul dword ptr [esp + 0x14] */
    MEMF(esp + 0x1C) = (float)fp_top(); fp_pop(); /* fstp */
    fp_push((double)SMEM32(ecx + 4)); /* fild */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x14)); /* fmul dword ptr [esp + 0x14] */
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C0)); /* fadd dword ptr [0x4148c0] */
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003C7FE3u); RECOMP_ABI_CALL(0x003CC3C0u, sub_003CC3C0); /* call 0x003CC3C0 */

loc_003C7FE3: ;
    fp_push(MEMF(esp + 0x18)); /* fld float */
    PUSH32(esp, ecx);
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C0)); /* fadd dword ptr [0x4148c0] */
    esi = eax;
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003C7FF8u); RECOMP_ABI_CALL(0x003CC3C0u, sub_003CC3C0); /* call 0x003CC3C0 */

loc_003C7FF8: ;
    esi--;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    esi = esi << 0x10;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    eax = eax | esi;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    esi = edi + -32;
    MEM32(edx) = esi;
    MEM32(edx + 4) = eax;
    fp_push((double)SMEM32(ecx)); /* fild */
    PUSH32(esp, ecx);
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x18)); /* fmul dword ptr [esp + 0x18] */
    MEMF(esp + 0x1C) = (float)fp_top(); fp_pop(); /* fstp */
    fp_push((double)SMEM32(ecx + 8)); /* fild */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x18)); /* fmul dword ptr [esp + 0x18] */
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C0)); /* fadd dword ptr [0x4148c0] */
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003C8026u); RECOMP_ABI_CALL(0x003CC3C0u, sub_003CC3C0); /* call 0x003CC3C0 */

loc_003C8026: ;
    fp_push(MEMF(esp + 0x18)); /* fld float */
    PUSH32(esp, ecx);
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C0)); /* fadd dword ptr [0x4148c0] */
    esi = eax;
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003C803Bu); RECOMP_ABI_CALL(0x003CC3C0u, sub_003CC3C0); /* call 0x003CC3C0 */

loc_003C803B: ;
    esi--;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    esi = esi << 0x10;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    eax = eax | esi;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(edx + 8) = edi;
    MEM32(edx + 0xC) = eax;
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edx = edx + 0x10;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 0x10;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edi = edi + 4;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ebp--;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa != 0)) goto loc_003C7FC2; /* jne: not equal / not zero */

loc_003C8057: ;
    esi = MEM32(esp + 0x20);

loc_003C805B: ;
    ebp = MEM32(esp + 0x28);

loc_003C805F: ;
    eax = MEM32(esp + 0x20);
    ecx = esi;
    ecx = ecx << 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    MEM32(ebx) = edx;
    edx = ecx;
    ecx = ecx >> 2;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    esi = ebp;
    edi = ebx + 0x1B80;
    if (!g_df) { uint8_t *_d = (uint8_t*)XBOX_PTR(edi), *_s = (uint8_t*)XBOX_PTR(esi); uint32_t _n = ecx * 4;
      if (_d + _n <= _s || _s + _n <= _d) memcpy(_d, _s, _n);
      else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*4) = MEM32(esi + _i*4); }
      esi += ecx * 4; edi += ecx * 4; }
    else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi - _i*4) = MEM32(esi - _i*4); esi -= ecx * 4; edi -= ecx * 4; }
    ecx = 0; /* rep movsd */
    ecx = edx;
    ecx = ecx & 3;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    if (!g_df) { uint8_t *_d = (uint8_t*)XBOX_PTR(edi), *_s = (uint8_t*)XBOX_PTR(esi); uint32_t _n = ecx;
      if (_d + _n <= _s || _s + _n <= _d) memcpy(_d, _s, _n);
      else { uint32_t _i; for (_i = 0; _i < _n; _i++) _d[_i] = _s[_i]; }
      esi += ecx; edi += ecx; }
    else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM8(edi - _i) = MEM8(esi - _i); esi -= ecx; edi -= ecx; }
    ecx = 0; /* rep movsb */
    ecx = MEM32(esp + 0x24);
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebp);
    MEM32(ebx + 0x1C00) = eax;
    MEM32(ebx + 0x1C04) = ecx;
    POP32(esp, ebx);
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 16; return; /* ret 12 */

    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}

/**
 * sub_003C8120
 * Original: 0x003C8120 - 0x003C82AC (396 bytes, 145 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C8120(void)
{
    RECOMP_ENTRY(0x003C8120u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003C8120: ;
    PUSH32(esp, ecx);
    PUSH32(esp, ebp);
    ebp = MEM32(0x3D83C8);
    { uint32_t _icall_esp = g_esp;
    { uint32_t _icall_target = MEM32(0x3E15C4); PUSH32(esp, 0x003C812Eu); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x003C8128u); } /* indirect call */
    }

loc_003C812E: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C8141; /* je: equal / zero */

loc_003C8132: ;
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, eax);
    { uint32_t _icall_target = MEM32(0x3E1430); PUSH32(esp, 0x003C8139u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x003C8133u); } /* indirect call */
    }

loc_003C8139: ;
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, 0);
    { uint32_t _icall_target = MEM32(0x3E15CC); PUSH32(esp, 0x003C8141u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x003C813Bu); } /* indirect call */
    }

loc_003C8141: ;
    eax = MEM32(ebp + 0x938);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C8153; /* jne: not equal / not zero */

loc_003C814B: ;
    eax = 0x80004005u;
    POP32(esp, ebp);
    POP32(esp, ecx);
    esp += 4; return; /* ret */

loc_003C8153: ;
    eax = MEM32(ebp + 0x1DE4);
    ecx = ebp + 0x1C28;
    eax = eax & 1;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax = eax + eax * 2 + 0x5D;
    MEM32(esp + 4) = ecx;
    eax = ecx + eax * 4;
    /* nop */

loc_003C8170: ;
    _fa = (uint32_t)(MEM32(eax)) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(eax), 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) { RECOMP_SPIN(0x003C8170u); goto loc_003C8170; } /* jne: not equal / not zero */

loc_003C8175: ;
    PUSH32(esp, ebx);
    PUSH32(esp, edi);
    edi = MEM32(ebp + 0x1A18);
    eax = MEM32(edi + 0x10);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    ebx = ZX8(MEM8(edi + 0xD));
    if (CMP_NE(_fa, _fb)) goto loc_003C819A; /* jne: not equal / not zero */

loc_003C8188: ;
    ecx = MEM32(edi + 0xC);
    ecx = ecx >> 0x14;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ecx = ecx & 0xF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    edx = 1;
    edx = edx << LO8(ecx);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    goto loc_003C81A3;

loc_003C819A: ;
    edx = eax;
    edx = edx & 0xFFF;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    edx++;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_003C81A3: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C81B8; /* jne: not equal / not zero */

loc_003C81A7: ;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(ecx, MEM8(edi + 0xF));
    eax = 1;
    ecx = ecx & 0xF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax = eax << LO8(ecx);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    goto loc_003C81C1;

loc_003C81B8: ;
    eax = eax >> 0xC;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    eax = eax & 0xFFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_003C81C1: ;
    PUSH32(esp, esi);
    PUSH32(esp, eax);
    PUSH32(esp, edx);
    edx = ebx;
    PUSH32(esp, 0x003C81CBu); RECOMP_ABI_CALL(0x003C80A0u, sub_003C80A0); /* call 0x003C80A0 */

loc_003C81CB: ;
    esi = eax;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C81DC; /* jne: not equal / not zero */

loc_003C81D1: ;
    POP32(esp, esi);
    POP32(esp, edi);
    POP32(esp, ebx);
    eax = 0x80004005u;
    POP32(esp, ebp);
    POP32(esp, ecx);
    esp += 4; return; /* ret */

loc_003C81DC: ;
    PUSH32(esp, 0);
    PUSH32(esp, esi);
    PUSH32(esp, 0);
    PUSH32(esp, 0);
    PUSH32(esp, edi);
    PUSH32(esp, 0x003C81E9u); RECOMP_ABI_CALL(0x003C6E10u, sub_003C6E10); /* call 0x003C6E10 */

loc_003C81E9: ;
    ecx = MEM32(0x3D83C8);
    edx = MEM32(ecx + 0x2C);
    PUSH32(esp, 2);
    PUSH32(esp, edx);
    PUSH32(esp, 0x003C81FAu); RECOMP_ABI_CALL(0x003CAA10u, sub_003CAA10); /* call 0x003CAA10 */

loc_003C81FA: ;
    eax = MEM32(esp + 0x10);
    edi = MEM32(eax);
    ebp = MEM32(ebp + 0x1C30);
    eax = ebx;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    PUSH32(esp, 0x003C820Fu); RECOMP_ABI_CALL(0x003CE670u, sub_003CE670); /* call 0x003CE670 */

loc_003C820F: ;
    ebx = eax;

loc_003C8211: ;
    eax = MEM32(esi + 0x10);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C825C; /* jne: not equal / not zero */

loc_003C8218: ;
    ecx = MEM32(esi + 0xC);
    ecx = ecx >> 0x14;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ecx = ecx & 0xF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax = 1;
    eax = eax << LO8(ecx);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ecx = ZX8(MEM8(esi + 0xD));
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xC) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0xC (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C8258; /* je: equal / zero */

loc_003C8231: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xD) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0xD (32-bit) */
    if (CMP_LE(_fas, _fbs)) goto loc_003C8240; /* jle: less or equal (signed <=) */

loc_003C8236: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xF) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0xF (32-bit) */
    if (CMP_G(_fas, _fbs)) goto loc_003C8240; /* jg: greater (signed >) */

loc_003C823B: ;
    eax = eax << 2;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    goto loc_003C8264;

loc_003C8240: ;
    ecx = ZX8(MEM8(esi + 0xD));
    ecx = ZX8(MEM8(ecx + 0x3D4B68));
    ecx = ecx & 0x3C;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ecx = (uint32_t)((int32_t)ecx * (int32_t)eax);
    ecx = ecx >> 3;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    eax = ecx;
    goto loc_003C8264;

loc_003C8258: ;
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + eax;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_003C8264;

loc_003C825C: ;
    eax = ZX8(MEM8(esi + 0x13));
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    eax = eax << 6;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */

loc_003C8264: ;
    ecx = MEM32(esi + 4);
    PUSH32(esp, ecx);
    PUSH32(esp, eax);
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, edx);
    PUSH32(esp, edi);
    { uint32_t _icall_target = MEM32(0x3E15D0); PUSH32(esp, 0x003C8273u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x003C826Du); } /* indirect call */
    }

loc_003C8273: ;
    edx = eax;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C8211; /* jne: not equal / not zero */

loc_003C8279: ;
    ecx = MEM32(esp + 0x10);
    MEM32(0x3D928C) = eax;
    MEM32(0x3D9288) = eax;
    PUSH32(esp, 0x003C828Cu); RECOMP_ABI_CALL(0x003CF91Bu, sub_003CF91B); /* call 0x003CF91B */

loc_003C828C: ;
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, esi);
    { uint32_t _icall_target = MEM32(0x3E14F0); PUSH32(esp, 0x003C8293u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x003C828Du); } /* indirect call */
    }

loc_003C8293: ;
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, 1);
    PUSH32(esp, eax);
    PUSH32(esp, esi);
    { uint32_t _icall_target = MEM32(0x3E1424); PUSH32(esp, 0x003C829Du); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x003C8297u); } /* indirect call */
    }

loc_003C829D: ;
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, esi);
    { uint32_t _icall_target = MEM32(0x3E15CC); PUSH32(esp, 0x003C82A4u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x003C829Eu); } /* indirect call */
    }

loc_003C82A4: ;
    POP32(esp, esi);
    POP32(esp, edi);
    POP32(esp, ebx);
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    POP32(esp, ebp);
    POP32(esp, ecx);
    esp += 4; return; /* ret */

}

/**
 * sub_003C82B0
 * Original: 0x003C82B0 - 0x003C82EA (58 bytes, 15 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C82B0(void)
{
    RECOMP_ENTRY(0x003C82B0u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C82B0: ;
    ecx = MEM32(0x3D928C);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    eax = MEM32(0x3D83C8);
    if (CMP_EQ(_fa, _fb)) goto loc_003C82C7; /* je: equal / zero */

loc_003C82BF: ;
    _fa = (uint32_t)(MEM32(0x3D9284)) & 0xFFFFFFFFu; _fb = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0x3D9284), esi (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C82E9; /* je: equal / zero */

loc_003C82C7: ;
    eax = MEM32(eax + 0x1C28);
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, 0);
    PUSH32(esp, esi);
    PUSH32(esp, 0xB);
    PUSH32(esp, eax);
    { uint32_t _icall_target = MEM32(0x3E15C8); PUSH32(esp, 0x003C82D9u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x003C82D3u); } /* indirect call */
    }

loc_003C82D9: ;
    MEM32(0x3D928C) = 1;
    MEM32(0x3D9284) = esi;

loc_003C82E9: ;
    esp += 4; return; /* ret */

}

/**
 * sub_003C82F0
 * Original: 0x003C82F0 - 0x003C833D (77 bytes, 21 insns)
 * Category: game_render
 * CC: cdecl, 2 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C82F0(void)
{
    RECOMP_ENTRY(0x003C82F0u);
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

loc_003C82F0: ;
    fp_push(MEMF(esp + 4)); /* fld float */
    PUSH32(esp, esi);
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x3D49AC)); /* fadd dword ptr [0x3d49ac] */
    esi = MEM32(0x3D83C8);
    MEMF(esi + 0xEF8) = (float)fp_top(); fp_pop(); /* fstp */
    fp_push(MEMF(esp + 0xC)); /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x3D49AC)); /* fadd dword ptr [0x3d49ac] */
    MEMF(esi + 0xEFC) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003C831Cu); RECOMP_ABI_CALL(0x003C8F10u, sub_003C8F10); /* call 0x003C8F10 */

loc_003C831C: ;
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C8332; /* jb: below (unsigned <) */

loc_003C8323: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C8332u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C8332: ;
    PUSH32(esp, 0x003C8337u); RECOMP_ABI_CALL(0x003C6100u, sub_003C6100); /* call 0x003C6100 */

loc_003C8337: ;
    MEM32(esi) = eax;
    POP32(esp, esi);
    esp += 12; return; /* ret 8 */

    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}

/**
 * sub_003C8710
 * Original: 0x003C8710 - 0x003C878F (127 bytes, 39 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C8710(void)
{
    RECOMP_ENTRY(0x003C8710u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003C8710: ;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, 0 (32-bit) */
    ecx = MEM32(0x3D83C8);
    PUSH32(esp, ebp);
    ebp = MEM32(esp + 8);
    PUSH32(esp, esi);
    if (CMP_EQ(_fa, _fb)) goto loc_003C8726; /* je: equal / zero */

loc_003C8720: ;
    _fb = (uint32_t)(0x80000) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    MEM32(ebx) = MEM32(ebx) + 0x80000;
    _fa = (uint32_t)(MEM32(ebx)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C8726: ;
    esi = eax + eax * 2;
    esi = esi << 2;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    eax = MEM32(esi + 0x3D93B0);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C875C; /* je: equal / zero */

loc_003C8736: ;
    ecx = MEM32(ecx + 0x2C);
    MEM32(eax + 8) = ecx;
    PUSH32(esp, edi);
    edi = MEM32(esi + 0x3D93B0);
    ecx = MEM32(edi);
    _fb = (uint32_t)(0xFFF80000u) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 0xFFF80000u;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x78FFFF) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test eax, 0x78FFFF (32-bit) */
    MEM32(edi) = ecx;
    if (TEST_NZ(_fa, _fb)) goto loc_003C875B; /* jne: not equal / not zero */

loc_003C8756: ;
    PUSH32(esp, 0x003C875Bu); RECOMP_ABI_CALL(0x003C9EC0u, sub_003C9EC0); /* call 0x003C9EC0 */

loc_003C875B: ;
    POP32(esp, edi);

loc_003C875C: ;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 0x3D93A8)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, MEM32(esi + 0x3D93A8) (32-bit) */
    eax = MEM32(0x3D83D0);
    MEM32(esi + 0x3D93A8) = ebp;
    MEM32(esi + 0x3D93B0) = ebx;
    if (CMP_NE(_fa, _fb)) goto loc_003C8782; /* jne: not equal / not zero */

loc_003C8775: ;
    eax = eax | 0x40;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    POP32(esp, esi);
    MEM32(0x3D83D0) = eax;
    POP32(esp, ebp);
    esp += 8; return; /* ret 4 */

loc_003C8782: ;
    eax = eax | 0x70;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    POP32(esp, esi);
    MEM32(0x3D83D0) = eax;
    POP32(esp, ebp);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C88A0
 * Original: 0x003C88A0 - 0x003C8922 (130 bytes, 47 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C88A0(void)
{
    RECOMP_ENTRY(0x003C88A0u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003C88A0: ;
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    PUSH32(esp, edi);
    ebp = esi;
    edi = eax;
    SET_LO8(eax, MEM8(ebp + 8));
    edi--;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x10) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(eax), 0x10 (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C88C1; /* jne: not equal / not zero */

loc_003C88B6: ;
    ecx = MEM32(esp + 0x14);
    eax = edi;
    PUSH32(esp, 0x003C88C1u); RECOMP_ABI_CALL(0x003C8790u, sub_003C8790); /* call 0x003C8790 */

loc_003C88C1: ;
    ebx = MEM32(edi + 0xC);
    eax = MEM32(esi);
    edx = MEM32(esi + 4);
    ecx = eax + ebx * 4 + 8;
    _fb = (uint32_t)(0x200) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edx = edx + 0x200;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, edx (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C88FA; /* jb: below (unsigned <) */

loc_003C88D7: ;
    ecx = MEM32(0x3D9588);
    eax = ecx;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edx = ebx * 4 + 0x20C;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, ecx (32-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_003C88EE; /* jbe: below or equal (unsigned <=) */

loc_003C88EC: ;
    ecx = edx;

loc_003C88EE: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, eax (32-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_003C88F4; /* jbe: below or equal (unsigned <=) */

loc_003C88F2: ;
    eax = edx;

loc_003C88F4: ;
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C88FAu); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C88FA: ;
    ecx = MEM32(esp + 0x14);
    MEM32(eax) = 0x41E9C;
    MEM32(eax + 4) = ecx;
    esi = edi + 0x114;
    ecx = ebx;
    edi = eax + 8;
    if (!g_df) { uint8_t *_d = (uint8_t*)XBOX_PTR(edi), *_s = (uint8_t*)XBOX_PTR(esi); uint32_t _n = ecx * 4;
      if (_d + _n <= _s || _s + _n <= _d) memcpy(_d, _s, _n);
      else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*4) = MEM32(esi + _i*4); }
      esi += ecx * 4; edi += ecx * 4; }
    else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi - _i*4) = MEM32(esi - _i*4); esi -= ecx * 4; edi -= ecx * 4; }
    ecx = 0; /* rep movsd */
    POP32(esp, edi);
    edx = eax + ebx * 4 + 8;
    POP32(esp, esi);
    MEM32(ebp) = edx;
    POP32(esp, ebp);
    POP32(esp, ebx);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C8930
 * Original: 0x003C8930 - 0x003C89E4 (180 bytes, 53 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C8930(void)
{
    RECOMP_ENTRY(0x003C8930u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C8930: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    if (CMP_EQ(_fa, _fb)) goto loc_003C89B8; /* je: equal / zero */

loc_003C893B: ;
    edx = MEM32(0x3D83D0);
    ecx = MEM32(esi + 0x794);
    ecx = MEM32(ecx + 4);
    edx = edx | 0x70;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    PUSH32(esp, edi);
    edi = eax + -1;
    MEM32(0x3D83D0) = edx;
    MEM32(esi + 0x794) = edi;
    MEM32(esi + 0x798) = eax;
    edx = MEM32(edi + 4);
    edx = edx ^ ecx;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(LO8(edx)) & 0xFFu; _fb = (uint32_t)(0x13) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(edx), 0x13 (8-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003C89B7; /* je: equal / zero */

loc_003C896D: ;
    edx = MEM32(0x3D83D0);
    eax = MEM32(esi);
    ecx = MEM32(esi + 4);
    edx = edx | 0x1000;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ecx (32-bit) */
    MEM32(0x3D83D0) = edx;
    if (CMP_B(_fa, _fb)) goto loc_003C8997; /* jb: below (unsigned <) */

loc_003C8988: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C8997u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C8997: ;
    PUSH32(esp, 0x003C899Cu); RECOMP_ABI_CALL(0x003C6100u, sub_003C6100); /* call 0x003C6100 */

loc_003C899C: ;
    ecx = MEM32(edi + 4);
    MEM32(eax) = 0x81E94;
    ecx = ecx & 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(eax + 4) = 6;
    MEM32(eax + 8) = ecx;
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0xC;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi) = eax;

loc_003C89B7: ;
    POP32(esp, edi);

loc_003C89B8: ;
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C89CE; /* jb: below (unsigned <) */

loc_003C89BF: ;
    eax = MEM32(0x3D9588);
    edx = eax;
    edx = edx >> 1;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, edx);
    PUSH32(esp, 0x003C89CEu); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C89CE: ;
    MEM32(eax) = 0x41EA0;
    MEM32(eax + 4) = ebx;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi) = eax;
    MEM32(esi + 0x79C) = ebx;
    POP32(esp, esi);
    esp += 4; return; /* ret */

}

/**
 * sub_003C89F0
 * Original: 0x003C89F0 - 0x003C8B14 (292 bytes, 68 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C89F0(void)
{
    RECOMP_ENTRY(0x003C89F0u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C89F0: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x10) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(eax), 0x10 (8-bit) */
    PUSH32(esp, ebx);
    ebx = MEM32(0x3D83C8);
    ecx = MEM32(ebx + 8);
    if (TEST_Z(_fa, _fb)) goto loc_003C8A06; /* je: equal / zero */

loc_003C89FE: ;
    ecx = ecx | 0x200;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    goto loc_003C8A0C;

loc_003C8A06: ;
    ecx = ecx & 0xFFFFFDFFu;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */

loc_003C8A0C: ;
    eax = eax & 0xFFFFFFEFu;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(ebx + 8) = ecx;
    MEM32(ebx + 0x1928) = eax;
    if ((_fa != 0)) goto loc_003C8B12; /* jne: not equal / not zero */

loc_003C8A1E: ;
    edx = MEM32(0x3D83D0);
    eax = MEM32(ebx);
    ecx = MEM32(ebx + 4);
    edx = edx | 0x1600;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ecx (32-bit) */
    MEM32(0x3D83D0) = edx;
    if (CMP_B(_fa, _fb)) goto loc_003C8A48; /* jb: below (unsigned <) */

loc_003C8A39: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C8A48u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C8A48: ;
    PUSH32(esp, esi);
    MEM32(eax) = 0x41EA4;
    PUSH32(esp, edi);
    MEM32(eax + 4) = 0x3C;
    MEM32(eax + 8) = 0x300B80;
    edi = eax + 0xC;
    ecx = 0xC;
    esi = 0x3D4C20;
    if (!g_df) { uint8_t *_d = (uint8_t*)XBOX_PTR(edi), *_s = (uint8_t*)XBOX_PTR(esi); uint32_t _n = ecx * 4;
      if (_d + _n <= _s || _s + _n <= _d) memcpy(_d, _s, _n);
      else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*4) = MEM32(esi + _i*4); }
      esi += ecx * 4; edi += ecx * 4; }
    else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi - _i*4) = MEM32(esi - _i*4); esi -= ecx * 4; edi -= ecx * 4; }
    ecx = 0; /* rep movsd */
    esi = eax + 0x3C;
    ecx = 0x3D4BB0;
    edx = 0x400840;
    eax = esi;
    PUSH32(esp, 0x003C8A81u); RECOMP_ABI_CALL(0x003CAEB0u, sub_003CAEB0); /* call 0x003CAEB0 */

loc_003C8A81: ;
    eax = esi + 0x44;
    ecx = 0x3D4BB0;
    edx = 0x400880;
    PUSH32(esp, 0x003C8A93u); RECOMP_ABI_CALL(0x003CAEB0u, sub_003CAEB0); /* call 0x003CAEB0 */

loc_003C8A93: ;
    eax = esi + 0x88;
    ecx = 0x3D4BB0;
    edx = 0x4008C0;
    PUSH32(esp, 0x003C8AA8u); RECOMP_ABI_CALL(0x003CAEB0u, sub_003CAEB0); /* call 0x003CAEB0 */

loc_003C8AA8: ;
    eax = esi + 0xCC;
    ecx = 0x3D4BB0;
    edx = 0x400900;
    PUSH32(esp, 0x003C8ABDu); RECOMP_ABI_CALL(0x003CAEB0u, sub_003CAEB0); /* call 0x003CAEB0 */

loc_003C8ABD: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    MEM32(esi + 0x110) = 0x1009D0;
    MEM32(esi + 0x114) = eax;
    MEM32(esi + 0x118) = eax;
    MEM32(esi + 0x120) = eax;
    ecx = 0x3F800000;
    MEM32(esi + 0x11C) = ecx;
    MEM32(esi + 0x124) = 0x100A50;
    MEM32(esi + 0x128) = eax;
    MEM32(esi + 0x12C) = eax;
    MEM32(esi + 0x130) = eax;
    MEM32(esi + 0x134) = ecx;
    _fb = (uint32_t)(0x138) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + 0x138;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    POP32(esp, edi);
    MEM32(ebx) = esi;
    POP32(esp, esi);

loc_003C8B12: ;
    POP32(esp, ebx);
    esp += 4; return; /* ret */

}

/**
 * sub_003C8DC0
 * Original: 0x003C8DC0 - 0x003C8F04 (324 bytes, 114 insns)
 * Category: game_render
 * CC: cdecl, 4 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C8DC0(void)
{
    RECOMP_ENTRY(0x003C8DC0u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003C8DC0: ;
    PUSH32(esp, ecx);
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    ebp = MEM32(esp + 0x14);
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, 0 (32-bit) */
    PUSH32(esp, esi);
    if (CMP_EQ(_fa, _fb)) goto loc_003C8DDC; /* je: equal / zero */

loc_003C8DCC: ;
    eax = ZX16(MEM16(ebp));
    ecx = ZX16(MEM16(ebp + 2));
    MEM32(esp + 0xC) = eax;
    MEM32(esp + 0x18) = ecx;

loc_003C8DDC: ;
    esi = 0; /* xor self */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C8DF2; /* je: equal / zero */

loc_003C8DE2: ;
    eax = MEM32(esp + 0x18);
    eax = eax << 4;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edx = eax + 0x7F;
    edx = edx >> 7;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    esi = eax + edx * 4;

loc_003C8DF2: ;
    ecx = MEM32(esp + 0x14);
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C8E06; /* je: equal / zero */

loc_003C8DFC: ;
    PUSH32(esp, eax);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C8E03u); RECOMP_ABI_CALL(0x003C8CC0u, sub_003C8CC0); /* call 0x003C8CC0 */

loc_003C8E03: ;
    eax = eax << 2;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */

loc_003C8E06: ;
    ebx = eax + esi;
    PUSH32(esp, 0x24800000);
    eax = ebx + 0x118;
    PUSH32(esp, eax);
    PUSH32(esp, 0x003C8E1Au); RECOMP_ABI_CALL(0x0032BD3Eu, sub_0032BD3E); /* call 0x0032BD3E */

loc_003C8E1A: ;
    esi = eax;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C8E2C; /* jne: not equal / not zero */

loc_003C8E20: ;
    POP32(esp, esi);
    POP32(esp, ebp);
    eax = 0x8007000Eu;
    POP32(esp, ebx);
    POP32(esp, ecx);
    esp += 20; return; /* ret 16 */

loc_003C8E2C: ;
    PUSH32(esp, edi);
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    ecx = 0x46;
    edi = esi;
    { uint32_t _i; int32_t _st = RECOMP_DF_STEP(4); for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*_st) = eax; edi += ecx * _st; }
    ecx = 0; /* rep stosd */
    ecx = MEM32(esp + 0x24);
    MEM32(esi + 4) = ecx;
    MEM32(esi) = 1;
    eax = esi + 0x1C;
    ecx = 0x10;
    /* nop */

loc_003C8E50: ;
    MEM32(eax) = 2;
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x10;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx--;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa != 0)) goto loc_003C8E50; /* jne: not equal / not zero */

loc_003C8E5C: ;
    ecx = MEM32(esp + 0x18);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C8E78; /* je: equal / zero */

loc_003C8E64: ;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, 0 (32-bit) */
    SET_LO8(edx, (CMP_EQ(_fa, _fb)) ? 1 : 0); /* sete */
    edi = esi;
    PUSH32(esp, edx);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C8E74u); RECOMP_ABI_CALL(0x003C8B20u, sub_003C8B20); /* call 0x003C8B20 */

loc_003C8E74: ;
    ecx = MEM32(esp + 0x18);

loc_003C8E78: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C8ECC; /* je: equal / zero */

loc_003C8E7E: ;
    edi = MEM32(esi + 4);
    ecx = MEM32(esp + 0x10);
    edi = edi | 0x10;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x2078) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x2078 (32-bit) */
    MEM32(esi + 4) = edi;
    eax = edi;
    if (CMP_EQ(_fa, _fb)) goto loc_003C8E9B; /* je: equal / zero */

loc_003C8E95: ;
    eax = eax | 1;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(esi + 4) = eax;

loc_003C8E9B: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x7378) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x7378 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C8EA7; /* jne: not equal / not zero */

loc_003C8EA3: ;
    MEM32(esi + 4) = MEM32(esi + 4) | 8;
    _fa = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */

loc_003C8EA7: ;
    eax = MEM32(esp + 0x1C);
    MEM32(esi + 8) = eax;
    eax = eax << 2;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, eax);
    edi = esi + 0x114;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ebp = ebp + 4;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, ebp);
    eax = edi;
    PUSH32(esp, 0x003C8EC3u); RECOMP_ABI_CALL(0x003C8840u, sub_003C8840); /* call 0x003C8840 */

loc_003C8EC3: ;
    ecx = MEM32(esp + 0x18);
    _fb = (uint32_t)(edi) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - edi;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    eax = (uint32_t)(((int32_t)(int32_t)(eax)) >> ((2) & 31u));
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sar result */

loc_003C8ECC: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    POP32(esp, edi);
    if (CMP_EQ(_fa, _fb)) goto loc_003C8EDF; /* je: equal / zero */

loc_003C8ED1: ;
    edx = esi + eax * 4 + 0x114;
    PUSH32(esp, edx);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C8EDFu); RECOMP_ABI_CALL(0x003C8CC0u, sub_003C8CC0); /* call 0x003C8CC0 */

loc_003C8EDF: ;
    ebx = ebx >> 2;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    eax = ebx;
    MEM32(esi + 0xC) = eax;
    MEM32(esi + eax * 4 + 0x114) = 0;
    eax = MEM32(esp + 0x1C);
    esi = esi | 1;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(eax) = esi;
    POP32(esp, esi);
    POP32(esp, ebp);
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    POP32(esp, ebx);
    POP32(esp, ecx);
    esp += 20; return; /* ret 16 */

}

/**
 * sub_003C9030
 * Original: 0x003C9030 - 0x003C919D (365 bytes, 116 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C9030(void)
{
    RECOMP_ENTRY(0x003C9030u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003C9030: ;
    PUSH32(esp, ecx);
    PUSH32(esp, ebx);
    ebx = MEM32(esp + 0xC);
    _fa = (uint32_t)(LO8(ebx)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ebx), 1 (8-bit) */
    PUSH32(esp, ebp);
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    eax = MEM32(esi + 0x794);
    ecx = MEM32(eax + 4);
    ebp = MEM32(eax + 0x10);
    PUSH32(esp, edi);
    MEM32(esp + 0x10) = ecx;
    if (TEST_Z(_fa, _fb)) goto loc_003C907A; /* je: equal / zero */

loc_003C9054: ;
    SET_LO8(eax, MEM8(ebx + 3));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x10) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(eax), 0x10 (8-bit) */
    edi = ebx + -1;
    if (TEST_Z(_fa, _fb)) goto loc_003C9088; /* je: equal / zero */

loc_003C905E: ;
    PUSH32(esp, 0);
    eax = ebx;
    PUSH32(esp, 0x003C9067u); RECOMP_ABI_CALL(0x003C88A0u, sub_003C88A0); /* call 0x003C88A0 */

loc_003C9067: ;
    eax = MEM32(esp + 0x18);
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    PUSH32(esp, 0x003C9072u); RECOMP_ABI_CALL(0x003C8930u, sub_003C8930); /* call 0x003C8930 */

loc_003C9072: ;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebp);
    POP32(esp, ebx);
    POP32(esp, ecx);
    esp += 8; return; /* ret 4 */

loc_003C907A: ;
    edi = 0x3D9290;
    ecx = ebx;
    eax = edi;
    PUSH32(esp, 0x003C9088u); RECOMP_ABI_CALL(0x003C85D0u, sub_003C85D0); /* call 0x003C85D0 */

loc_003C9088: ;
    ecx = MEM32(0x3D83D0);
    eax = MEM32(edi + 0x10);
    ecx = ecx | 0x70;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, eax (32-bit) */
    MEM32(0x3D83D0) = ecx;
    if (CMP_EQ(_fa, _fb)) goto loc_003C90AA; /* je: equal / zero */

loc_003C909E: ;
    eax = ecx;
    eax = eax | 0x400;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(0x3D83D0) = eax;

loc_003C90AA: ;
    edx = MEM32(esp + 0x10);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(edi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, MEM32(edi + 4) (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C90BD; /* je: equal / zero */

loc_003C90B3: ;
    MEM32(0x3D83D0) = MEM32(0x3D83D0) | 0x1600;
    _fa = (uint32_t)(MEM32(0x3D83D0)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */

loc_003C90BD: ;
    eax = MEM32(esi);
    ecx = MEM32(esi + 4);
    MEM32(esi + 0x798) = ebx;
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ecx (32-bit) */
    MEM32(esi + 0x794) = edi;
    MEM32(esi + 0x79C) = ebx;
    if (CMP_B(_fa, _fb)) goto loc_003C90E9; /* jb: below (unsigned <) */

loc_003C90DA: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C90E9u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C90E9: ;
    edx = MEM32(edi + 4);
    ecx = ecx | 0xFFFFFFFFu;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    _fa = (uint32_t)(HI8(edx)) & 0xFFu; _fb = (uint32_t)(4) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(edx), 4 (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C9100; /* jne: not equal / not zero */

loc_003C90F4: ;
    MEM32(eax) = 0x4194C;
    MEM32(eax + 4) = ecx;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C9100: ;
    edx = MEM32(edi + 4);
    _fa = (uint32_t)(HI8(edx)) & 0xFFu; _fb = (uint32_t)(8) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(edx), 8 (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C9114; /* jne: not equal / not zero */

loc_003C9108: ;
    MEM32(eax) = 0x41950;
    MEM32(eax + 4) = ebx;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C9114: ;
    edx = MEM32(edi + 4);
    _fa = (uint32_t)(HI8(edx)) & 0xFFu; _fb = (uint32_t)(0x10) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(edx), 0x10 (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C9128; /* jne: not equal / not zero */

loc_003C911C: ;
    MEM32(eax) = 0x4195C;
    MEM32(eax + 4) = ecx;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C9128: ;
    ecx = MEM32(edi + 4);
    _fa = (uint32_t)(HI8(ecx)) & 0xFFu; _fb = (uint32_t)(0x20) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(ecx), 0x20 (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C913C; /* jne: not equal / not zero */

loc_003C9130: ;
    MEM32(eax) = 0x41960;
    MEM32(eax + 4) = ebx;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C913C: ;
    ecx = MEM32(edi + 4);
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(2) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ecx), 2 (8-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003C9175; /* je: equal / zero */

loc_003C9144: ;
    PUSH32(esp, 0x003C9149u); RECOMP_ABI_CALL(0x003C6100u, sub_003C6100); /* call 0x003C6100 */

loc_003C9149: ;
    MEM32(eax) = 0x41EA0;
    MEM32(eax + 4) = ebx;
    MEM32(eax + 8) = 0x81E94;
    MEM32(eax + 0xC) = 6;
    MEM32(eax + 0x10) = ebx;
    _fb = (uint32_t)(0x14) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x14;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi) = eax;
    PUSH32(esp, 0x003C916Du); RECOMP_ABI_CALL(0x003C8F10u, sub_003C8F10); /* call 0x003C8F10 */

loc_003C916D: ;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebp);
    POP32(esp, ebx);
    POP32(esp, ecx);
    esp += 8; return; /* ret 4 */

loc_003C9175: ;
    ecx = ecx ^ MEM32(esp + 0x10);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0x12) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ecx), 0x12 (8-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003C9193; /* je: equal / zero */

loc_003C917E: ;
    MEM32(eax) = 0x41E94;
    MEM32(eax + 4) = 4;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, 0x003C9193u); RECOMP_ABI_CALL(0x003C6100u, sub_003C6100); /* call 0x003C6100 */

loc_003C9193: ;
    POP32(esp, edi);
    MEM32(esi) = eax;
    POP32(esp, esi);
    POP32(esp, ebp);
    POP32(esp, ebx);
    POP32(esp, ecx);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C91A0
 * Original: 0x003C91A0 - 0x003C91FF (95 bytes, 31 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C91A0(void)
{
    RECOMP_ENTRY(0x003C91A0u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C91A0: ;
    PUSH32(esp, ebx);

loc_003C91A1: ;
    eax = MEM32(0x3D5F20);
    _fb = (uint32_t)(0x1C) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x1C;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(0x3D5F24)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(0x3D5F24) (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_003C91F4; /* jae: above or equal (unsigned >=) */

loc_003C91B1: ;
    MEM32(0x3D5F20) = eax;
    MEM32(eax + -28) = 0x41EA4;
    MEM32(eax + -24) = ecx;
    MEM32(eax + -20) = 0x100B80;
    ecx = ecx << 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ecx = ecx + 0x3D8678;
    ebx = MEM32(edx);
    MEM32(eax + -16) = ebx;
    MEM32(ecx) = ebx;
    ebx = MEM32(edx + 4);
    MEM32(eax + -12) = ebx;
    MEM32(ecx + 4) = ebx;
    ebx = MEM32(edx + 8);
    MEM32(eax + -8) = ebx;
    MEM32(ecx + 8) = ebx;
    ebx = MEM32(edx + 0xC);
    MEM32(eax + -4) = ebx;
    MEM32(ecx + 0xC) = ebx;
    POP32(esp, ebx);
    esp += 4; return; /* ret */

loc_003C91F4: ;
    PUSH32(esp, edx);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C91FBu); RECOMP_ABI_CALL(0x003CACC0u, sub_003CACC0); /* call 0x003CACC0 */

loc_003C91FB: ;
    POP32(esp, ecx);
    POP32(esp, edx);
    goto loc_003C91A1;

}

/**
 * sub_003C9200
 * Original: 0x003C9200 - 0x003C92A2 (162 bytes, 42 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C9200(void)
{
    RECOMP_ENTRY(0x003C9200u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C9200: ;
    eax = MEM32(0x3D5F20);
    _fb = (uint32_t)(0x4C) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x4C;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(0x3D5F24)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(0x3D5F24) (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_003C9294; /* jae: above or equal (unsigned >=) */

loc_003C9214: ;
    MEM32(0x3D5F20) = eax;
    mm0 = MMX_MEM(edx); /* movq */
    mm1 = MMX_MEM(edx + 8); /* movq */
    mm2 = MMX_MEM(edx + 0x10); /* movq */
    mm3 = MMX_MEM(edx + 0x18); /* movq */
    mm4 = MMX_MEM(edx + 0x20); /* movq */
    mm5 = MMX_MEM(edx + 0x28); /* movq */
    mm6 = MMX_MEM(edx + 0x30); /* movq */
    mm7 = MMX_MEM(edx + 0x38); /* movq */
    MEM32(eax + -76) = 0x41EA4;
    MEM32(eax + -72) = ecx;
    ecx = ecx << 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ecx = ecx + 0x3D8678;
    MEM32(eax + -68) = 0x400B80;
    MMX_STORE(eax + -64, mm0); /* movq */
    MMX_STORE(ecx, mm0); /* movq */
    MMX_STORE(eax + -56, mm1); /* movq */
    MMX_STORE(ecx + 8, mm1); /* movq */
    MMX_STORE(eax + -48, mm2); /* movq */
    MMX_STORE(ecx + 0x10, mm2); /* movq */
    MMX_STORE(eax + -40, mm3); /* movq */
    MMX_STORE(ecx + 0x18, mm3); /* movq */
    MMX_STORE(eax + -32, mm4); /* movq */
    MMX_STORE(ecx + 0x20, mm4); /* movq */
    MMX_STORE(eax + -24, mm5); /* movq */
    MMX_STORE(ecx + 0x28, mm5); /* movq */
    MMX_STORE(eax + -16, mm6); /* movq */
    MMX_STORE(ecx + 0x30, mm6); /* movq */
    MMX_STORE(eax + -8, mm7); /* movq */
    MMX_STORE(ecx + 0x38, mm7); /* movq */
    (void)0; /* emms - empty MMX state */
    esp += 4; return; /* ret */

loc_003C9294: ;
    PUSH32(esp, edx);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C929Bu); RECOMP_ABI_CALL(0x003CACC0u, sub_003CACC0); /* call 0x003CACC0 */

loc_003C929B: ;
    POP32(esp, ecx);
    POP32(esp, edx);
    goto loc_003C9200;

}

/**
 * sub_003C92B0
 * Original: 0x003C92B0 - 0x003C92F8 (72 bytes, 22 insns)
 * CC: cdecl, 2 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C92B0(void)
{
    RECOMP_ENTRY(0x003C92B0u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C92B0: ;
    PUSH32(esp, 0x24800000);
    PUSH32(esp, 0xFC);
    PUSH32(esp, 0x003C92BFu); RECOMP_ABI_CALL(0x0032BD3Eu, sub_0032BD3E); /* call 0x0032BD3E */

loc_003C92BF: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C92CB; /* jne: not equal / not zero */

loc_003C92C3: ;
    eax = 0x8007000Eu;
    esp += 12; return; /* ret 8 */

loc_003C92CB: ;
    PUSH32(esp, esi);
    esi = MEM32(esp + 8);
    PUSH32(esp, edi);
    edi = eax + 0xC;
    MEM32(eax + 8) = edi;
    MEM32(eax) = 1;
    MEM32(eax + 4) = 1;
    ecx = 0x3C;
    if (!g_df) { uint8_t *_d = (uint8_t*)XBOX_PTR(edi), *_s = (uint8_t*)XBOX_PTR(esi); uint32_t _n = ecx * 4;
      if (_d + _n <= _s || _s + _n <= _d) memcpy(_d, _s, _n);
      else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*4) = MEM32(esi + _i*4); }
      esi += ecx * 4; edi += ecx * 4; }
    else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi - _i*4) = MEM32(esi - _i*4); esi -= ecx * 4; edi -= ecx * 4; }
    ecx = 0; /* rep movsd */
    ecx = MEM32(esp + 0x10);
    POP32(esp, edi);
    MEM32(ecx) = eax;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    POP32(esp, esi);
    esp += 12; return; /* ret 8 */

}

/**
 * sub_003C9300
 * Original: 0x003C9300 - 0x003C9503 (515 bytes, 132 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C9300(void)
{
    RECOMP_ENTRY(0x003C9300u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003C9300: ;
    PUSH32(esp, ecx);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    PUSH32(esp, ebx);
    ebx = MEM32(0x3D83C8);
    ecx = MEM32(ebx + 0x784);
    MEM32(esp + 4) = ecx;
    MEM32(ebx + 0x784) = eax;
    if (CMP_NE(_fa, _fb)) goto loc_003C9382; /* jne: not equal / not zero */

loc_003C931C: ;
    ecx = MEM32(0x3D83D0);
    eax = MEM32(ebx + 0x788);
    ecx = ecx | 0x4800;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    MEM32(0x3D83D0) = ecx;
    if (CMP_EQ(_fa, _fb)) goto loc_003C9344; /* je: equal / zero */

loc_003C9338: ;
    eax = ecx;
    eax = eax | 0x2000;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(0x3D83D0) = eax;

loc_003C9344: ;
    edx = MEM32(0x3D8628);
    PUSH32(esp, edx);
    PUSH32(esp, 0x003C9350u); RECOMP_ABI_CALL(0x003C5630u, sub_003C5630); /* call 0x003C5630 */

loc_003C9350: ;
    eax = MEM32(ebx);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(ebx + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(ebx + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C9366; /* jb: below (unsigned <) */

loc_003C9357: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C9366u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C9366: ;
    edx = ebx;
    PUSH32(esp, 0x003C936Du); RECOMP_ABI_CALL(0x003C5F60u, sub_003C5F60); /* call 0x003C5F60 */

loc_003C936D: ;
    MEM32(eax) = 0x41E78;
    MEM32(eax + 4) = 0x210000;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(ebx) = eax;
    POP32(esp, ebx);
    POP32(esp, ecx);
    esp += 4; return; /* ret */

loc_003C9382: ;
    edx = MEM32(eax + 8);
    ecx = MEM32(edx + 0x24);
    PUSH32(esp, ebp);
    PUSH32(esp, esi);
    ecx = ecx | MEM32(edx + 0x20);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    if ((_fa != 0)) goto loc_003C93A3; /* jne: not equal / not zero */

loc_003C938F: ;
    edx = MEM32(ebx + 0x788);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C93A3; /* je: equal / zero */

loc_003C9399: ;
    MEM32(0x3D83D0) = MEM32(0x3D83D0) | 0x2000;
    _fa = (uint32_t)(MEM32(0x3D83D0)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */

loc_003C93A3: ;
    MEM32(ebx + 0x788) = ecx;
    edx = MEM32(eax + 8);
    ecx = MEM32(edx + 0xEC);
    ecx = ecx & 0x100;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(ebx + 0x78C) = ecx;
    ebp = MEM32(eax + 8);
    edx = MEM32(ebp + 0xD8);
    eax = MEM32(ebx);
    ecx = MEM32(ebx + 4);
    MEM32(ebx + 0x790) = edx;
    edx = MEM32(0x3D83D0);
    edx = edx | 0x4000;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ecx (32-bit) */
    MEM32(0x3D83D0) = edx;
    if (CMP_B(_fa, _fb)) goto loc_003C93F7; /* jb: below (unsigned <) */

loc_003C93E8: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C93F7u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C93F7: ;
    _fa = (uint32_t)(MEM8(ebx + 8)) & 0xFFu; _fb = (uint32_t)(0x10) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test MEM8(ebx + 8), 0x10 (8-bit) */
    PUSH32(esp, edi);
    if (TEST_NZ(_fa, _fb)) goto loc_003C9418; /* jne: not equal / not zero */

loc_003C93FE: ;
    ecx = 0x39;
    esi = ebp;
    edi = 0x3D83D8;
    if (!g_df) { uint8_t *_d = (uint8_t*)XBOX_PTR(edi), *_s = (uint8_t*)XBOX_PTR(esi); uint32_t _n = ecx * 4;
      if (_d + _n <= _s || _s + _n <= _d) memcpy(_d, _s, _n);
      else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*4) = MEM32(esi + _i*4); }
      esi += ecx * 4; edi += ecx * 4; }
    else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi - _i*4) = MEM32(esi - _i*4); esi -= ecx * 4; edi -= ecx * 4; }
    ecx = 0; /* rep movsd */
    edx = MEM32(ebp + 0xD8);
    MEM32(0x3D85F8) = edx;

loc_003C9418: ;
    ecx = MEM32(esp + 0x10);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C9427; /* jne: not equal / not zero */

loc_003C9420: ;
    edx = ebx;
    PUSH32(esp, 0x003C9427u); RECOMP_ABI_CALL(0x003C5F60u, sub_003C5F60); /* call 0x003C5F60 */

loc_003C9427: ;
    MEM32(eax) = 0x200260;
    edi = eax + 4;
    ecx = 8;
    esi = ebp;
    if (!g_df) { uint8_t *_d = (uint8_t*)XBOX_PTR(edi), *_s = (uint8_t*)XBOX_PTR(esi); uint32_t _n = ecx * 4;
      if (_d + _n <= _s || _s + _n <= _d) memcpy(_d, _s, _n);
      else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*4) = MEM32(esi + _i*4); }
      esi += ecx * 4; edi += ecx * 4; }
    else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi - _i*4) = MEM32(esi - _i*4); esi -= ecx * 4; edi -= ecx * 4; }
    ecx = 0; /* rep movsd */
    MEM32(eax + 0x24) = 0x800A60;
    esi = ebp + 0x28;
    edi = eax + 0x28;
    ecx = 0x20;
    if (!g_df) { uint8_t *_d = (uint8_t*)XBOX_PTR(edi), *_s = (uint8_t*)XBOX_PTR(esi); uint32_t _n = ecx * 4;
      if (_d + _n <= _s || _s + _n <= _d) memcpy(_d, _s, _n);
      else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*4) = MEM32(esi + _i*4); }
      esi += ecx * 4; edi += ecx * 4; }
    else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi - _i*4) = MEM32(esi - _i*4); esi -= ecx * 4; edi -= ecx * 4; }
    ecx = 0; /* rep movsd */
    ecx = MEM32(ebp + 0xA8);
    MEM32(eax + 0xA8) = 0x417F8;
    MEM32(eax + 0xAC) = ecx;
    MEM32(eax + 0xB0) = 0x81E20;
    ecx = MEM32(ebp + 0xAC);
    MEM32(eax + 0xB4) = ecx;
    edx = MEM32(ebp + 0xB0);
    MEM32(eax + 0xB8) = edx;
    MEM32(eax + 0xBC) = 0x241E40;
    esi = ebp + 0xB4;
    edi = eax + 0xC0;
    ecx = 9;
    if (!g_df) { uint8_t *_d = (uint8_t*)XBOX_PTR(edi), *_s = (uint8_t*)XBOX_PTR(esi); uint32_t _n = ecx * 4;
      if (_d + _n <= _s || _s + _n <= _d) memcpy(_d, _s, _n);
      else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*4) = MEM32(esi + _i*4); }
      esi += ecx * 4; edi += ecx * 4; }
    else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi - _i*4) = MEM32(esi - _i*4); esi -= ecx * 4; edi -= ecx * 4; }
    ecx = 0; /* rep movsd */
    MEM32(eax + 0xE4) = 0x81E74;
    ecx = MEM32(ebp + 0xDC);
    MEM32(eax + 0xE8) = ecx;
    edx = MEM32(ebp + 0xE0);
    MEM32(eax + 0xEC) = edx;
    ecx = MEM32(ebx + 0x788);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    POP32(esp, edi);
    if (CMP_EQ(_fa, _fb)) goto loc_003C94F7; /* je: equal / zero */

loc_003C94CF: ;
    MEM32(eax + 0xF0) = 0x80288;
    ecx = MEM32(ebp + 0x20);
    MEM32(eax + 0xF4) = ecx;
    edx = MEM32(ebp + 0x24);
    MEM32(eax + 0xF8) = edx;
    POP32(esp, esi);
    _fb = (uint32_t)(0xFC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0xFC;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    POP32(esp, ebp);
    MEM32(ebx) = eax;
    POP32(esp, ebx);
    POP32(esp, ecx);
    esp += 4; return; /* ret */

loc_003C94F7: ;
    POP32(esp, esi);
    _fb = (uint32_t)(0xF0) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0xF0;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    POP32(esp, ebp);
    MEM32(ebx) = eax;
    POP32(esp, ebx);
    POP32(esp, ecx);
    esp += 4; return; /* ret */

}

/**
 * sub_003C9510
 * Original: 0x003C9510 - 0x003C97D5 (709 bytes, 190 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: standard_frame
 */
void d3d_orig_003C9510(void)
{
    RECOMP_ENTRY(0x003C9510u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* prologue saves caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C9510: ;
    PUSH32(esp, ebp);
    ebp = esp;
    g_ebp = ebp; /* publish frame for frameless callees */
    g_seh_ebp = ebp;
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x10;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    edx = MEM32(0x3D83C8);
    PUSH32(esp, edi);
    edi = ecx;
    ecx = MEM32(edx + 0x784);
    ecx = MEM32(ecx + 8);
    MEM32(ebp + -4) = ecx;
    ecx = MEM32(ebp + 8);
    MEM32(ebp + -16) = edx;
    MEM32(ebp + 8) = ecx;
    if (CMP_EQ(_fa, _fb)) goto loc_003C97CE; /* je: equal / zero */

loc_003C953C: ;
    PUSH32(esp, ebx);
    MEM32(ebp + -8) = eax;
    PUSH32(esp, esi);

loc_003C9541: ;
    eax = MEM32(ebp + 8);
    xmm0 = XMM_MEM(eax); /* movups */
    xmm0 = XMM_MIN(xmm0, XMM_MEM(0x452800)); /* minps */
    xmm0 = XMM_MAX(xmm0, XMM_MEM(0xC06150)); /* maxps */
    xmm0 = XMM_MUL(xmm0, XMM_MEM(0x452810)); /* mulps */
    eax = (int32_t)recomp_fist((double)xmm0.f[0], 0x027Fu, 32); /* cvtss2si */
    eax = eax << 0x10;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    xmm0 = XMM_SHUFFLE(xmm0, xmm0, 0x39); /* shufps */
    ebx = (int32_t)recomp_fist((double)xmm0.f[0], 0x027Fu, 32); /* cvtss2si */
    ebx = ebx << 8;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    eax = eax | ebx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    xmm0 = XMM_SHUFFLE(xmm0, xmm0, 0x39); /* shufps */
    ebx = (int32_t)recomp_fist((double)xmm0.f[0], 0x027Fu, 32); /* cvtss2si */
    eax = eax | ebx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    xmm0 = XMM_SHUFFLE(xmm0, xmm0, 0x39); /* shufps */
    ebx = (int32_t)recomp_fist((double)xmm0.f[0], 0x027Fu, 32); /* cvtss2si */
    ebx = ebx << 0x18;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    eax = eax | ebx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(ebp + -12) = eax;
    esi = MEM32(ebp + -12);
    MEM32(edx + edi * 4 + 0x8E4) = esi;
    eax = MEM32(edx);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(edx + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(edx + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C95AD; /* jb: below (unsigned <) */

loc_003C959B: ;
    eax = MEM32(0x3D9588);
    edx = eax;
    edx = edx >> 1;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, edx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x003C95AAu); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C95AA: ;
    edx = MEM32(ebp + -16);

loc_003C95AD: ;
    ecx = MEM32(ebp + -4);
    ecx = MEM32(ecx + 0xE4);
    ecx = ecx ^ MEM32(edi * 4 + 0x452820);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0xF) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ecx), 0xF (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C95D4; /* jne: not equal / not zero */

loc_003C95C2: ;
    MEM32(0x3D8400) = esi;
    MEM32(eax) = 0x40A60;
    MEM32(eax + 4) = esi;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C95D4: ;
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0xF0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ecx), 0xF0 (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C95EB; /* jne: not equal / not zero */

loc_003C95D9: ;
    MEM32(0x3D8404) = esi;
    MEM32(eax) = 0x40A64;
    MEM32(eax + 4) = esi;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C95EB: ;
    _fa = (uint32_t)(HI8(ecx)) & 0xFFu; _fb = (uint32_t)(0xF) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(ecx), 0xF (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C9602; /* jne: not equal / not zero */

loc_003C95F0: ;
    MEM32(0x3D8408) = esi;
    MEM32(eax) = 0x40A68;
    MEM32(eax + 4) = esi;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C9602: ;
    _fa = (uint32_t)(HI8(ecx)) & 0xFFu; _fb = (uint32_t)(0xF0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(ecx), 0xF0 (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C9619; /* jne: not equal / not zero */

loc_003C9607: ;
    MEM32(0x3D840C) = esi;
    MEM32(eax) = 0x40A6C;
    MEM32(eax + 4) = esi;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C9619: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xF0000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test ecx, 0xF0000 (32-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C9633; /* jne: not equal / not zero */

loc_003C9621: ;
    MEM32(0x3D8410) = esi;
    MEM32(eax) = 0x40A70;
    MEM32(eax + 4) = esi;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C9633: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xF00000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test ecx, 0xF00000 (32-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C964D; /* jne: not equal / not zero */

loc_003C963B: ;
    MEM32(0x3D8414) = esi;
    MEM32(eax) = 0x40A74;
    MEM32(eax + 4) = esi;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C964D: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xF000000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test ecx, 0xF000000 (32-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C9667; /* jne: not equal / not zero */

loc_003C9655: ;
    MEM32(0x3D8418) = esi;
    MEM32(eax) = 0x40A78;
    MEM32(eax + 4) = esi;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C9667: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xF0000000u) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test ecx, 0xF0000000u (32-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C9681; /* jne: not equal / not zero */

loc_003C966F: ;
    MEM32(0x3D841C) = esi;
    MEM32(eax) = 0x40A7C;
    MEM32(eax + 4) = esi;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C9681: ;
    ecx = MEM32(edi * 4 + 0x452820);
    ebx = MEM32(ebp + -4);
    ecx = ecx ^ MEM32(ebx + 0xE8);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0xF) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ecx), 0xF (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C96A8; /* jne: not equal / not zero */

loc_003C9696: ;
    MEM32(0x3D8420) = esi;
    MEM32(eax) = 0x40A80;
    MEM32(eax + 4) = esi;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C96A8: ;
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0xF0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ecx), 0xF0 (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C96BF; /* jne: not equal / not zero */

loc_003C96AD: ;
    MEM32(0x3D8424) = esi;
    MEM32(eax) = 0x40A84;
    MEM32(eax + 4) = esi;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C96BF: ;
    _fa = (uint32_t)(HI8(ecx)) & 0xFFu; _fb = (uint32_t)(0xF) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(ecx), 0xF (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C96D6; /* jne: not equal / not zero */

loc_003C96C4: ;
    MEM32(0x3D8428) = esi;
    MEM32(eax) = 0x40A88;
    MEM32(eax + 4) = esi;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C96D6: ;
    _fa = (uint32_t)(HI8(ecx)) & 0xFFu; _fb = (uint32_t)(0xF0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(ecx), 0xF0 (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C96ED; /* jne: not equal / not zero */

loc_003C96DB: ;
    MEM32(0x3D842C) = esi;
    MEM32(eax) = 0x40A8C;
    MEM32(eax + 4) = esi;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C96ED: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xF0000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test ecx, 0xF0000 (32-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C9707; /* jne: not equal / not zero */

loc_003C96F5: ;
    MEM32(0x3D8430) = esi;
    MEM32(eax) = 0x40A90;
    MEM32(eax + 4) = esi;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C9707: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xF00000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test ecx, 0xF00000 (32-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C9721; /* jne: not equal / not zero */

loc_003C970F: ;
    MEM32(0x3D8434) = esi;
    MEM32(eax) = 0x40A94;
    MEM32(eax + 4) = esi;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C9721: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xF000000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test ecx, 0xF000000 (32-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C973B; /* jne: not equal / not zero */

loc_003C9729: ;
    MEM32(0x3D8438) = esi;
    MEM32(eax) = 0x40A98;
    MEM32(eax + 4) = esi;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C973B: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xF0000000u) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test ecx, 0xF0000000u (32-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C9755; /* jne: not equal / not zero */

loc_003C9743: ;
    MEM32(0x3D843C) = esi;
    MEM32(eax) = 0x40A9C;
    MEM32(eax + 4) = esi;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C9755: ;
    ecx = MEM32(edi * 4 + 0x452820);
    ecx = ecx ^ MEM32(ebx + 0xEC);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0xF) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ecx), 0xF (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C9779; /* jne: not equal / not zero */

loc_003C9767: ;
    MEM32(0x3D8484) = esi;
    MEM32(eax) = 0x41E20;
    MEM32(eax + 4) = esi;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C9779: ;
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0xF0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ecx), 0xF0 (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C9790; /* jne: not equal / not zero */

loc_003C977E: ;
    MEM32(0x3D8488) = esi;
    MEM32(eax) = 0x41E24;
    MEM32(eax + 4) = esi;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C9790: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C97B3; /* jne: not equal / not zero */

loc_003C9794: ;
    esi = MEM32(ebp + 8);
    MEM32(eax) = 0xC181C;
    ebx = MEM32(esi);
    ecx = eax + 4;
    MEM32(ecx) = ebx;
    ebx = MEM32(esi + 4);
    MEM32(ecx + 4) = ebx;
    esi = MEM32(esi + 8);
    MEM32(ecx + 8) = esi;
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x10;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C97B3: ;
    ecx = MEM32(ebp + 8);
    MEM32(edx) = eax;
    eax = MEM32(ebp + -8);
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 0x10;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edi++;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    MEM32(ebp + 8) = ecx;
    MEM32(ebp + -8) = eax;
    if ((_fa != 0)) goto loc_003C9541; /* jne: not equal / not zero */

loc_003C97CC: ;
    POP32(esp, esi);
    POP32(esp, ebx);

loc_003C97CE: ;
    POP32(esp, edi);
    esp = ebp;
    POP32(esp, ebp);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C97E0
 * Original: 0x003C97E0 - 0x003C982B (75 bytes, 25 insns)
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C97E0(void)
{
    RECOMP_ENTRY(0x003C97E0u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C97E0: ;
    PUSH32(esp, esi);
    PUSH32(esp, 0x64800000);
    PUSH32(esp, 0xC);
    PUSH32(esp, 0x003C97EDu); RECOMP_ABI_CALL(0x0032BD3Eu, sub_0032BD3E); /* call 0x0032BD3E */

loc_003C97ED: ;
    esi = eax;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C9811; /* je: equal / zero */

loc_003C97F3: ;
    eax = MEM32(esp + 8);
    PUSH32(esp, 0xB2800000u);
    PUSH32(esp, eax);
    PUSH32(esp, 0x003C9802u); RECOMP_ABI_CALL(0x0032BD3Eu, sub_0032BD3E); /* call 0x0032BD3E */

loc_003C9802: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C9817; /* jne: not equal / not zero */

loc_003C9806: ;
    PUSH32(esp, 0x24800000);
    PUSH32(esp, esi);
    PUSH32(esp, 0x003C9811u); RECOMP_ABI_CALL(0x0032BDDEu, sub_0032BDDE); /* call 0x0032BDDE */

loc_003C9811: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

loc_003C9817: ;
    eax = eax & 0xFFFFFFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(esi + 4) = eax;
    MEM32(esi) = 0x1000001;
    eax = esi;
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C98C0
 * Original: 0x003C98C0 - 0x003C9A65 (421 bytes, 140 insns)
 * Category: game_render
 * CC: cdecl, 4 params, returns int_or_void
 * Frame: standard_frame
 */
void d3d_orig_003C98C0(void)
{
    RECOMP_ENTRY(0x003C98C0u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* prologue saves caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C98C0: ;
    PUSH32(esp, ebp);
    ebp = esp;
    g_ebp = ebp; /* publish frame for frameless callees */
    g_seh_ebp = ebp;
    _fb = (uint32_t)(0x14) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x14;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, ebx);
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    PUSH32(esp, edi);
    MEM32(ebp + -20) = esi;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x003C98D7u); RECOMP_ABI_CALL(0x003D20A0u, sub_003D20A0); /* call 0x003D20A0 */

loc_003C98D7: ;
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C98ED; /* jb: below (unsigned <) */

loc_003C98DE: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x003C98EDu); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C98ED: ;
    edx = MEM32(ebp + 8);
    ecx = MEM32(ebp + 0x10);
    edi = MEM32(ebp + 0x14);
    MEM32(eax) = 0x417FC;
    MEM32(eax + 4) = edx;
    edx = MEM32(esi + 8);
    ebx = MEM32(esi + 0xBCC);
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ebx = ebx + ecx;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx = MEM32(esi + 0xBD0);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edx = edx | 0x800;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    _fb = (uint32_t)(edi) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + edi;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi) = eax;
    MEM32(esi + 8) = edx;
    edx = MEM32(esi + 0xC54);
    eax = esi + 0xBD4;
    MEM32(ebp + -12) = ecx;
    MEM32(eax + edx * 8 + -4) = ecx;
    ecx = MEM32(ebp + 0xC);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x10) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x10 (32-bit) */
    MEM32(ebp + -16) = eax;
    MEM32(ebp + 8) = ebx;
    MEM32(ebp + -4) = edx;
    if (CMP_A(_fa, _fb)) goto loc_003C994E; /* ja: above (unsigned >) */

loc_003C9944: ;
    eax = 0x10;
    MEM32(ebp + 0x14) = eax;
    goto loc_003C9961;

loc_003C994E: ;
    eax = 0x7FF;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    { uint64_t _dividend = ((uint64_t)edx << 32) | eax;
      eax = (uint32_t)(_dividend / (uint32_t)MEM32(esi + 0xBC8));
      edx = (uint32_t)(_dividend % (uint32_t)MEM32(esi + 0xBC8)); }
    MEM32(ebp + 0x14) = eax;

loc_003C995E: ;
    eax = MEM32(ebp + 0x14);

loc_003C9961: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ecx (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C9967; /* jb: below (unsigned <) */

loc_003C9965: ;
    eax = ecx;

loc_003C9967: ;
    edi = MEM32(esi + 0xBC8);
    edi = (uint32_t)((int32_t)edi * (int32_t)eax);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    ecx = ecx - eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    MEM32(ebp + -8) = eax;
    eax = MEM32(0x3D83C8);
    MEM32(ebp + 0xC) = ecx;
    ecx = MEM32(eax);
    eax = MEM32(eax + 4);
    edx = ecx + edi * 4 + 0xC;
    _fb = (uint32_t)(0x200) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x200;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, eax (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C99B4; /* jb: below (unsigned <) */

loc_003C998F: ;
    ecx = MEM32(0x3D9588);
    eax = ecx;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edx = edi * 4 + 0x210;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, ecx (32-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_003C99A6; /* jbe: below or equal (unsigned <=) */

loc_003C99A4: ;
    ecx = edx;

loc_003C99A6: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, eax (32-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_003C99AC; /* jbe: below or equal (unsigned <=) */

loc_003C99AA: ;
    eax = edx;

loc_003C99AC: ;
    PUSH32(esp, ecx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x003C99B2u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C99B2: ;
    ecx = eax;

loc_003C99B4: ;
    edx = edi;
    edx = edx << 0x12;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(0x40001818) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edx = edx + 0x40001818;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0x80) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0x80 (32-bit) */
    eax = ecx + 4;
    MEM32(ebp + 0x10) = eax;
    MEM32(ecx) = edx;
    if (CMP_B(_fa, _fb)) goto loc_003C99F4; /* jb: below (unsigned <) */

loc_003C99CF: ;
    _fa = (uint32_t)(MEM32(ebp + -4)) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(ebp + -4), 1 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C99F4; /* jne: not equal / not zero */

loc_003C99D5: ;
    ecx = MEM32(ebp + -12);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C99F4; /* jne: not equal / not zero */

loc_003C99DC: ;
    PUSH32(esp, edi);
    PUSH32(esp, ebx);
    PUSH32(esp, eax);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x003C99E4u); RECOMP_ABI_CALL(0x003C9830u, sub_003C9830); /* call 0x003C9830 */

loc_003C99E4: ;
    ecx = edi * 4;
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ebx = ebx + ecx;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(ebp + 8) = ebx;
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_003C9A22;

loc_003C99F4: ;
    edx = MEM32(ebp + -8);
    esi = MEM32(ebp + 8);
    edi = MEM32(ebp + 0x10);

loc_003C99FD: ;
    ebx = MEM32(ebp + -16);
    eax = MEM32(ebp + -4);

loc_003C9A03: ;
    ecx = MEM32(ebx);
    if (!g_df) { uint8_t *_d = (uint8_t*)XBOX_PTR(edi), *_s = (uint8_t*)XBOX_PTR(esi); uint32_t _n = ecx * 4;
      if (_d + _n <= _s || _s + _n <= _d) memcpy(_d, _s, _n);
      else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*4) = MEM32(esi + _i*4); }
      esi += ecx * 4; edi += ecx * 4; }
    else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi - _i*4) = MEM32(esi - _i*4); esi -= ecx * 4; edi -= ecx * 4; }
    ecx = 0; /* rep movsd */
    _fb = (uint32_t)(MEM32(ebx + 4)) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + MEM32(ebx + 4);
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ebx = ebx + 8;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa != 0)) goto loc_003C9A03; /* jne: not equal / not zero */

loc_003C9A10: ;
    edx--;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa != 0)) goto loc_003C99FD; /* jne: not equal / not zero */

loc_003C9A13: ;
    MEM32(ebp + 0x10) = edi;
    MEM32(ebp + 8) = esi;
    ebx = MEM32(ebp + 8);
    eax = MEM32(ebp + 0x10);
    esi = MEM32(ebp + -20);

loc_003C9A22: ;
    ecx = MEM32(ebp + 0xC);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C9A30; /* je: equal / zero */

loc_003C9A29: ;
    MEM32(esi) = eax;
    goto loc_003C995E;

loc_003C9A30: ;
    MEM32(eax) = 0x417FC;
    MEM32(eax + 4) = 0;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi) = eax;
    eax = MEM32(esi + 8);
    ecx = eax;
    ecx = ecx & 0xFFFFE7FFu;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(HI8(eax)) & 0xFFu; _fb = (uint32_t)(0x10) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(eax), 0x10 (8-bit) */
    POP32(esp, edi);
    MEM32(esi + 8) = ecx;
    POP32(esp, esi);
    POP32(esp, ebx);
    if (TEST_Z(_fa, _fb)) goto loc_003C9A5F; /* je: equal / zero */

loc_003C9A58: ;
    PUSH32(esp, 1);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x003C9A5Fu); RECOMP_ABI_CALL(0x003CA720u, sub_003CA720); /* call 0x003CA720 */

loc_003C9A5F: ;
    esp = ebp;
    POP32(esp, ebp);
    esp += 20; return; /* ret 16 */

}

/**
 * sub_003C9A70
 * Original: 0x003C9A70 - 0x003C9B35 (197 bytes, 64 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C9A70(void)
{
    RECOMP_ENTRY(0x003C9A70u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003C9A70: ;
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    ebp = MEM32(0x3D83C8);
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    PUSH32(esp, 0);
    PUSH32(esp, ebp);
    edi = eax;
    ebx = ecx;
    PUSH32(esp, 0x003C9A86u); RECOMP_ABI_CALL(0x003D1EF0u, sub_003D1EF0); /* call 0x003D1EF0 */

loc_003C9A86: ;
    ecx = MEM32(0x3D83C8);
    eax = MEM32(ecx);
    ecx = MEM32(ecx + 4);
    esi = edi + -1;
    esi = esi >> 8;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    esi++;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    edx = eax + esi * 4 + 0x14;
    _fb = (uint32_t)(0x200) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 0x200;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, ecx (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C9AC9; /* jb: below (unsigned <) */

loc_003C9AA6: ;
    ecx = MEM32(0x3D9588);
    eax = ecx;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edx = esi * 4 + 0x218;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, ecx (32-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_003C9ABD; /* jbe: below or equal (unsigned <=) */

loc_003C9ABB: ;
    ecx = edx;

loc_003C9ABD: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, eax (32-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_003C9AC3; /* jbe: below or equal (unsigned <=) */

loc_003C9AC1: ;
    eax = edx;

loc_003C9AC3: ;
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C9AC9u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C9AC9: ;
    edx = MEM32(esp + 0x14);
    esi = esi << 0x12;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    MEM32(eax) = 0x417FC;
    _fb = (uint32_t)(0x40001810) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + 0x40001810;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0x100) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0x100 (32-bit) */
    MEM32(eax + 4) = edx;
    MEM32(eax + 8) = esi;
    if (CMP_BE(_fa, _fb)) goto loc_003C9B11; /* jbe: below or equal (unsigned <=) */

loc_003C9AEA: ;
    ecx = edi + -257;
    ecx = ecx >> 8;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_003C9AF4: ;
    edx = ebx;
    edx = edx | 0xFF000000u;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(eax + 0xC) = edx;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 4;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(0x100) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ebx = ebx + 0x100;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(0x100) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    edi = edi - 0x100;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    ecx--;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa != 0)) goto loc_003C9AF4; /* jne: not equal / not zero */

loc_003C9B11: ;
    edi--;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    edi = edi << 0x18;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edi = edi | ebx;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(eax + 0xC) = edi;
    MEM32(eax + 0x10) = 0x417FC;
    POP32(esp, edi);
    MEM32(eax + 0x14) = 0;
    _fb = (uint32_t)(0x18) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x18;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    POP32(esp, esi);
    MEM32(ebp) = eax;
    POP32(esp, ebp);
    POP32(esp, ebx);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003C9B40
 * Original: 0x003C9B40 - 0x003C9E11 (721 bytes, 225 insns)
 * Category: game_render
 * CC: cdecl, 3 params, returns int_or_void
 * Frame: standard_frame
 */
void d3d_orig_003C9B40(void)
{
    RECOMP_ENTRY(0x003C9B40u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* prologue saves caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C9B40: ;
    PUSH32(esp, ebp);
    ebp = esp;
    g_ebp = ebp; /* publish frame for frameless callees */
    g_seh_ebp = ebp;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, ebx);
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    eax = MEM32(esi + 0x1C);
    PUSH32(esp, eax);
    PUSH32(esp, esi);
    MEM32(ebp + -8) = esi;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x003C9B5Bu); RECOMP_ABI_CALL(0x003D1EF0u, sub_003D1EF0); /* call 0x003D1EF0 */

loc_003C9B5B: ;
    ebx = MEM32(esi + 8);
    ecx = MEM32(0x3D83C8);
    ebx = ebx | 0x800;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(esi + 8) = ebx;
    eax = MEM32(ecx);
    ecx = MEM32(ecx + 4);
    _fb = (uint32_t)(0x200) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 0x200;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edx = eax + 0x824;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, ecx (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C9BAB; /* jb: below (unsigned <) */

loc_003C9B82: ;
    ecx = MEM32(0x3D9588);
    eax = ecx;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xA28) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0xA28 (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_003C9B99; /* jae: above or equal (unsigned >=) */

loc_003C9B94: ;
    ecx = 0xA28;

loc_003C9B99: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xA28) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0xA28 (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_003C9BA5; /* jae: above or equal (unsigned >=) */

loc_003C9BA0: ;
    eax = 0xA28;

loc_003C9BA5: ;
    PUSH32(esp, ecx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x003C9BABu); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C9BAB: ;
    ecx = MEM32(ebp + 8);
    edx = MEM32(ebp + 0xC);
    MEM32(eax) = 0x417FC;
    ebx = eax + 8;
    MEM32(eax + 4) = ecx;
    eax = ebx;
    eax = (uint32_t)(-(int32_t)eax);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* neg result */
    eax = eax >> 2;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    eax = eax & 7;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ecx = eax + eax + 0x3E;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, ecx (32-bit) */
    PUSH32(esp, edi);
    MEM32(ebp + 8) = ebx;
    if (CMP_B(_fa, _fb)) goto loc_003C9D97; /* jb: below (unsigned <) */

loc_003C9BD7: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C9C18; /* je: equal / zero */

loc_003C9BDB: ;
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    ecx = eax;
    ecx = ecx << 0x12;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(0x40001800) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 0x40001800;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(ebp + 0xC) = eax;
    MEM32(ebx) = ecx;
    esi = MEM32(ebp + 0x10);
    edi = MEM32(ebp + 8);
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edi = edi + 4;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx = MEM32(ebp + 0xC);
    if (!g_df) { uint8_t *_d = (uint8_t*)XBOX_PTR(edi), *_s = (uint8_t*)XBOX_PTR(esi); uint32_t _n = ecx * 4;
      if (_d + _n <= _s || _s + _n <= _d) memcpy(_d, _s, _n);
      else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*4) = MEM32(esi + _i*4); }
      esi += ecx * 4; edi += ecx * 4; }
    else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi - _i*4) = MEM32(esi - _i*4); esi -= ecx * 4; edi -= ecx * 4; }
    ecx = 0; /* rep movsd */
    esi = MEM32(ebp + 0x10);
    ecx = eax * 4;
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + ecx;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = (uint32_t)(-(int32_t)eax);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* neg result */
    ebx = ebx + ecx + 4;
    edx = edx + eax * 2;
    MEM32(ebp + 8) = ebx;
    MEM32(ebp + 0x10) = esi;
    MEM32(ebp + 0xC) = edx;

loc_003C9C18: ;
    eax = 0x804021;
    { uint64_t _r = (uint64_t)eax * (uint64_t)edx;
      eax = (uint32_t)_r; edx = (uint32_t)(_r >> 32); }
    eax = MEM32(ebp + 0xC);
    ecx = eax;
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    ecx = ecx - edx;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + edx;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx = ecx >> 9;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ecx = (uint32_t)((int32_t)ecx * (int32_t)0x3FE);
    edx = eax;
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    edx = edx - ecx;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x3E) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0x3E (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C9C45; /* jb: below (unsigned <) */

loc_003C9C3C: ;
    edx = edx >> 1;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edx++;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    edx = edx & 0xFFFFFFF0u;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    edx--;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    goto loc_003C9C4A;

loc_003C9C45: ;
    edx = 0x1FF;

loc_003C9C4A: ;
    MEM32(ebp + -4) = edx;
    /* nop */

loc_003C9C50: ;
    eax = edx;
    eax = eax << 0x12;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(0x40001800) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x40001800;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(ebx) = eax;
    esi = MEM32(ebp + 0x10);
    edi = MEM32(ebp + 8);
    ecx = MEM32(ebp + -4);
    _fb = (uint32_t)(0xF) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    ecx = ecx - 0xF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    ecx = ecx >> 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    (void)0; /* prefetchnta: cache/ordering hint, nothing to model */
    (void)0; /* prefetchnta: cache/ordering hint, nothing to model */
    eax = MEM32(esi);
    mm1 = MMX_MEM(esi + 4); /* movq */
    mm2 = MMX_MEM(esi + 0xC); /* movq */
    mm3 = MMX_MEM(esi + 0x14); /* movq */
    mm4 = MMX_MEM(esi + 0x1C); /* movq */
    mm5 = MMX_MEM(esi + 0x24); /* movq */
    mm6 = MMX_MEM(esi + 0x2C); /* movq */
    mm7 = MMX_MEM(esi + 0x34); /* movq */
    /* nop */
    /* nop */
    MEM32(edi + 4) = eax;
    MMX_STORE(edi + 8, mm1); /* movntq */
    MMX_STORE(edi + 0x10, mm2); /* movntq */
    MMX_STORE(edi + 0x18, mm3); /* movntq */
    MMX_STORE(edi + 0x20, mm4); /* movntq */
    MMX_STORE(edi + 0x28, mm5); /* movntq */
    MMX_STORE(edi + 0x30, mm6); /* movntq */
    MMX_STORE(edi + 0x38, mm7); /* movntq */
    _fb = (uint32_t)(0x3C) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + 0x3C;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(0x40) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edi = edi + 0x40;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_003C9CC0;

    /* nop */

loc_003C9CC0: ;
    (void)0; /* prefetchnta: cache/ordering hint, nothing to model */
    (void)0; /* prefetchnta: cache/ordering hint, nothing to model */
    mm0 = MMX_MEM(esi); /* movq */
    mm1 = MMX_MEM(esi + 8); /* movq */
    mm2 = MMX_MEM(esi + 0x10); /* movq */
    mm3 = MMX_MEM(esi + 0x18); /* movq */
    mm4 = MMX_MEM(esi + 0x20); /* movq */
    mm5 = MMX_MEM(esi + 0x28); /* movq */
    mm6 = MMX_MEM(esi + 0x30); /* movq */
    mm7 = MMX_MEM(esi + 0x38); /* movq */
    /* nop */
    /* nop */
    MMX_STORE(edi, mm0); /* movntq */
    MMX_STORE(edi + 8, mm1); /* movntq */
    MMX_STORE(edi + 0x10, mm2); /* movntq */
    MMX_STORE(edi + 0x18, mm3); /* movntq */
    MMX_STORE(edi + 0x20, mm4); /* movntq */
    MMX_STORE(edi + 0x28, mm5); /* movntq */
    MMX_STORE(edi + 0x30, mm6); /* movntq */
    MMX_STORE(edi + 0x38, mm7); /* movntq */
    _fb = (uint32_t)(0x40) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + 0x40;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(0x40) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edi = edi + 0x40;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx--;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa != 0)) goto loc_003C9CC0; /* jne: not equal / not zero */

loc_003C9D11: ;
    ecx = MEM32(ebp + 0xC);
    esi = MEM32(ebp + 0x10);
    eax = edx * 4;
    edx = (uint32_t)(-(int32_t)edx);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* neg result */
    edx = ecx + edx * 2;
    ecx = MEM32(ebp + -8);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + eax;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = eax + ebx + 4;
    MEM32(ecx) = eax;
    ecx = MEM32(0x3D83C8);
    eax = MEM32(ecx);
    MEM32(ebp + 0xC) = edx;
    edx = MEM32(ecx + 4);
    _fb = (uint32_t)(0x200) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edx = edx + 0x200;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx = eax + 0x810;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, edx (32-bit) */
    MEM32(ebp + 0x10) = esi;
    if (CMP_B(_fa, _fb)) goto loc_003C9D78; /* jb: below (unsigned <) */

loc_003C9D4F: ;
    ecx = MEM32(0x3D9588);
    eax = ecx;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xA14) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0xA14 (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_003C9D66; /* jae: above or equal (unsigned >=) */

loc_003C9D61: ;
    ecx = 0xA14;

loc_003C9D66: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xA14) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0xA14 (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_003C9D72; /* jae: above or equal (unsigned >=) */

loc_003C9D6D: ;
    eax = 0xA14;

loc_003C9D72: ;
    PUSH32(esp, ecx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x003C9D78u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C9D78: ;
    ebx = eax;
    eax = MEM32(ebp + 0xC);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x3FE) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x3FE (32-bit) */
    edx = 0x1FF;
    MEM32(ebp + 8) = ebx;
    MEM32(ebp + -4) = edx;
    if (CMP_AE(_fa, _fb)) goto loc_003C9C50; /* jae: above or equal (unsigned >=) */

loc_003C9D93: ;
    (void)0; /* emms - empty MMX state */
    edx = eax;

loc_003C9D97: ;
    eax = edx;
    eax = eax >> 1;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ecx = eax;
    ecx = ecx << 0x12;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(0x40001800) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 0x40001800;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(ebp + 0xC) = eax;
    MEM32(ebx) = ecx;
    esi = MEM32(ebp + 0x10);
    edi = MEM32(ebp + 8);
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edi = edi + 4;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx = MEM32(ebp + 0xC);
    if (!g_df) { uint8_t *_d = (uint8_t*)XBOX_PTR(edi), *_s = (uint8_t*)XBOX_PTR(esi); uint32_t _n = ecx * 4;
      if (_d + _n <= _s || _s + _n <= _d) memcpy(_d, _s, _n);
      else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*4) = MEM32(esi + _i*4); }
      esi += ecx * 4; edi += ecx * 4; }
    else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi - _i*4) = MEM32(esi - _i*4); esi -= ecx * 4; edi -= ecx * 4; }
    ecx = 0; /* rep movsd */
    ecx = MEM32(ebp + 0x10);
    eax = eax << 2;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(LO8(edx)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(edx), 1 (8-bit) */
    ebx = ebx + eax + 4;
    POP32(esp, edi);
    if (TEST_Z(_fa, _fb)) goto loc_003C9DDA; /* je: equal / zero */

loc_003C9DCB: ;
    eax = ZX16(MEM16(ecx));
    MEM32(ebx) = 0x41808;
    MEM32(ebx + 4) = eax;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ebx = ebx + 8;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003C9DDA: ;
    ecx = MEM32(ebp + -8);
    MEM32(ebx) = 0x417FC;
    MEM32(ebx + 4) = 0;
    eax = MEM32(ecx + 8);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ebx = ebx + 8;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edx = eax;
    edx = edx & 0xFFFFE7FFu;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(HI8(eax)) & 0xFFu; _fb = (uint32_t)(0x10) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(eax), 0x10 (8-bit) */
    POP32(esp, esi);
    MEM32(ecx) = ebx;
    MEM32(ecx + 8) = edx;
    POP32(esp, ebx);
    if (TEST_Z(_fa, _fb)) goto loc_003C9E0B; /* je: equal / zero */

loc_003C9E04: ;
    PUSH32(esp, 1);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x003C9E0Bu); RECOMP_ABI_CALL(0x003CA720u, sub_003CA720); /* call 0x003CA720 */

loc_003C9E0B: ;
    esp = ebp;
    POP32(esp, ebp);
    esp += 16; return; /* ret 12 */

}

/**
 * sub_003C9E20
 * Original: 0x003C9E20 - 0x003C9E6B (75 bytes, 24 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C9E20(void)
{
    RECOMP_ENTRY(0x003C9E20u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C9E20: ;
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003C9E3D; /* jb: below (unsigned <) */

loc_003C9E2E: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C9E3Du); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003C9E3D: ;
    MEM32(eax) = 0x417FC;
    MEM32(eax + 4) = 0;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi) = eax;
    eax = MEM32(esi + 8);
    edx = eax;
    edx = edx & 0xFFFFE7FFu;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(HI8(eax)) & 0xFFu; _fb = (uint32_t)(0x10) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(eax), 0x10 (8-bit) */
    MEM32(esi + 8) = edx;
    POP32(esp, esi);
    if (TEST_Z(_fa, _fb)) goto loc_003C9E6A; /* je: equal / zero */

loc_003C9E63: ;
    PUSH32(esp, 1);
    PUSH32(esp, 0x003C9E6Au); RECOMP_ABI_CALL(0x003CA720u, sub_003CA720); /* call 0x003CA720 */

loc_003C9E6A: ;
    esp += 4; return; /* ret */

}

/**
 * sub_003C9EC0
 * Original: 0x003C9EC0 - 0x003C9F6A (170 bytes, 53 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C9EC0(void)
{
    RECOMP_ENTRY(0x003C9EC0u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C9EC0: ;
    eax = MEM32(edi);
    PUSH32(esp, esi);
    esi = eax;
    esi = esi & 0x70000;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0x50000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0x50000 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C9ED7; /* jne: not equal / not zero */

loc_003C9ED3: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if ((((uint32_t)((uint32_t)(_fa) - (uint32_t)(_fb)) >> 31) == 0)) goto loc_003C9EDE; /* jns: not sign (positive) */

loc_003C9ED7: ;
    eax = edi;
    PUSH32(esp, 0x003C9EDEu); RECOMP_ABI_CALL(0x003CADF0u, sub_003CADF0); /* call 0x003CADF0 */

loc_003C9EDE: ;
    PUSH32(esp, edi);
    PUSH32(esp, 0x003C9EE4u); RECOMP_ABI_CALL(0x003C9E70u, sub_003C9E70); /* call 0x003C9E70 */

loc_003C9EE4: ;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0x50000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0x50000 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C9F11; /* jne: not equal / not zero */

loc_003C9EEC: ;
    _fa = (uint32_t)(MEM32(edi)) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(edi), 0 (32-bit) */
    if ((((uint32_t)((uint32_t)(_fa) - (uint32_t)(_fb)) >> 31) == 0)) goto loc_003C9F5D; /* jns: not sign (positive) */

loc_003C9EF1: ;
    eax = MEM32(edi + 4);
    PUSH32(esp, 0xA1800000u);
    eax = eax | 0x80000000u;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    PUSH32(esp, eax);
    PUSH32(esp, 0x003C9F04u); RECOMP_ABI_CALL(0x0032BDDEu, sub_0032BDDE); /* call 0x0032BDDE */

loc_003C9F04: ;
    PUSH32(esp, 0x24800000);
    PUSH32(esp, edi);
    PUSH32(esp, 0x003C9F0Fu); RECOMP_ABI_CALL(0x0032BDDEu, sub_0032BDDE); /* call 0x0032BDDE */

loc_003C9F0F: ;
    POP32(esp, esi);
    esp += 4; return; /* ret */

loc_003C9F11: ;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0x20000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0x20000 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C9F39; /* jne: not equal / not zero */

loc_003C9F19: ;
    _fa = (uint32_t)(MEM32(edi)) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(edi), 0 (32-bit) */
    if ((((uint32_t)((uint32_t)(_fas) - (uint32_t)(_fbs)) >> 31) != 0)) goto loc_003C9F5D; /* js: sign (negative) */

loc_003C9F1E: ;
    ecx = MEM32(edi + 4);
    PUSH32(esp, 0xA1800000u);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003C9F2Cu); RECOMP_ABI_CALL(0x0032BDDEu, sub_0032BDDE); /* call 0x0032BDDE */

loc_003C9F2C: ;
    PUSH32(esp, 0x24800000);
    PUSH32(esp, edi);
    PUSH32(esp, 0x003C9F37u); RECOMP_ABI_CALL(0x0032BDDEu, sub_0032BDDE); /* call 0x0032BDDE */

loc_003C9F37: ;
    POP32(esp, esi);
    esp += 4; return; /* ret */

loc_003C9F39: ;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0x10000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0x10000 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C9F5D; /* je: equal / zero */

loc_003C9F41: ;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0x60000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0x60000 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C9F5D; /* je: equal / zero */

loc_003C9F49: ;
    edx = MEM32(edi + 4);
    PUSH32(esp, 0xA1800000u);
    edx = edx | 0x80000000u;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    PUSH32(esp, edx);
    PUSH32(esp, 0x003C9F5Du); RECOMP_ABI_CALL(0x0032BDDEu, sub_0032BDDE); /* call 0x0032BDDE */

loc_003C9F5D: ;
    PUSH32(esp, 0x24800000);
    PUSH32(esp, edi);
    PUSH32(esp, 0x003C9F68u); RECOMP_ABI_CALL(0x0032BDDEu, sub_0032BDDE); /* call 0x0032BDDE */

loc_003C9F68: ;
    POP32(esp, esi);
    esp += 4; return; /* ret */

}

/**
 * sub_003C9FD0
 * Original: 0x003C9FD0 - 0x003CA007 (55 bytes, 20 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003C9FD0(void)
{
    RECOMP_ENTRY(0x003C9FD0u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003C9FD0: ;
    PUSH32(esp, esi);
    esi = MEM32(esp + 8);
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFF) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test eax, 0xFFFF (32-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003C9FF7; /* jne: not equal / not zero */

loc_003C9FDE: ;
    eax = eax & 0x70000;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x50000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x50000 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003C9FF7; /* jne: not equal / not zero */

loc_003C9FEA: ;
    eax = MEM32(esi + 0x14);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003C9FF7; /* je: equal / zero */

loc_003C9FF1: ;
    PUSH32(esp, eax);
    PUSH32(esp, 0x003C9FF7u); RECOMP_ABI_CALL(0x003C9FD0u, sub_003C9FD0); /* call 0x003C9FD0 */

loc_003C9FF7: ;
    edx = MEM32(esi);
    edx++;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    eax = edx;
    MEM32(esi) = edx;
    eax = eax & 0xFFFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003CA010
 * Original: 0x003CA010 - 0x003CA05D (77 bytes, 27 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003CA010(void)
{
    RECOMP_ENTRY(0x003CA010u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003CA010: ;
    PUSH32(esp, edi);
    edi = MEM32(esp + 8);
    eax = MEM32(edi);
    ecx = eax;
    ecx = ecx & 0xFFFF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 1 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CA051; /* jne: not equal / not zero */

loc_003CA024: ;
    eax = eax & 0x70000;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x50000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x50000 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CA03D; /* jne: not equal / not zero */

loc_003CA030: ;
    eax = MEM32(edi + 0x14);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CA03D; /* je: equal / zero */

loc_003CA037: ;
    PUSH32(esp, eax);
    PUSH32(esp, 0x003CA03Du); RECOMP_ABI_CALL(0x003CA010u, sub_003CA010); /* call 0x003CA010 */

loc_003CA03D: ;
    eax = MEM32(edi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x780000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test eax, 0x780000 (32-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003CA051; /* jne: not equal / not zero */

loc_003CA046: ;
    PUSH32(esp, 0x003CA04Bu); RECOMP_ABI_CALL(0x003C9EC0u, sub_003C9EC0); /* call 0x003C9EC0 */

loc_003CA04B: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    POP32(esp, edi);
    esp += 8; return; /* ret 4 */

loc_003CA051: ;
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    MEM32(edi) = eax;
    eax = eax & 0xFFFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    POP32(esp, edi);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003CA060
 * Original: 0x003CA060 - 0x003CA0D8 (120 bytes, 42 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003CA060(void)
{
    RECOMP_ENTRY(0x003CA060u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    int _cf = 0; /* carry flag */

loc_003CA060: ;
    eax = MEM32(ecx);
    _cf = 0; /* logical op clears CF */
    eax = eax & 0x70000;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x30000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x30000 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_A(_fa, _fb)) goto loc_003CA090; /* ja: above (unsigned >) */

loc_003CA06E: ;
    if (CMP_EQ(_fa, _fb)) goto loc_003CA08A; /* je: equal / zero */

loc_003CA070: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_003CA084; /* je: equal / zero */

loc_003CA074: ;
    _cf = 0; /* xor clears CF */
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x10000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x10000 (32-bit) */
    _cf = (int)(_fa < _fb);
    SET_LO8(ecx, (CMP_NE(_fa, _fb)) ? 1 : 0); /* setne */
    _fb = (uint32_t)(7) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(ecx) + (uint64_t)(7)) >> 32) & 1);
    ecx = ecx + 7;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = ecx;
    esp += 4; return; /* ret */

loc_003CA084: ;
    eax = 6;
    esp += 4; return; /* ret */

loc_003CA08A: ;
    eax = 9;
    esp += 4; return; /* ret */

loc_003CA090: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x40000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x40000 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_EQ(_fa, _fb)) goto loc_003CA0B7; /* je: equal / zero */

loc_003CA097: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x50000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x50000 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_EQ(_fa, _fb)) goto loc_003CA0A4; /* je: equal / zero */

loc_003CA09E: ;
    eax = 0xA;
    esp += 4; return; /* ret */

loc_003CA0A4: ;
    edx = MEM32(ecx + 0xC);
    _cf = 0; /* logical op clears CF */
    edx = edx & 0xF0;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _cf = 0; /* xor clears CF */
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x20) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0x20 (32-bit) */
    _cf = (int)(_fa < _fb);
    SET_LO8(eax, (CMP_A(_fa, _fb)) ? 1 : 0); /* seta */
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    esp += 4; return; /* ret */

loc_003CA0B7: ;
    eax = MEM32(ecx + 0xC);
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(4) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(eax), 4 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (TEST_Z(_fa, _fb)) goto loc_003CA0C4; /* je: equal / zero */

loc_003CA0BE: ;
    eax = 5;
    esp += 4; return; /* ret */

loc_003CA0C4: ;
    _cf = 0; /* logical op clears CF */
    eax = eax & 0xF0;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ecx = 0x20;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, eax (32-bit) */
    _cf = (int)(_fa < _fb);
    eax = _cf ? 0xFFFFFFFF : 0; /* sbb self (CF extend) */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sbb result */
    _cf = (int)((eax) != 0);
    eax = (uint32_t)(-(int32_t)eax);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* neg result */
    _fb = (uint32_t)(3) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(3)) >> 32) & 1);
    eax = eax + 3;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 4; return; /* ret */

}

/**
 * sub_003CA270
 * Original: 0x003CA270 - 0x003CA634 (964 bytes, 277 insns)
 * Category: game_render
 * CC: cdecl, 6 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003CA270(void)
{
    RECOMP_ENTRY(0x003CA270u);
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

loc_003CA270: ;
    _fb = (uint32_t)(0x38) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x38;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    ebp = MEM32(0x3D83C8);
    ebx = MEM32(ebp + 0x1A08);
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    edi = MEM32(ebp + 0x1A04);
    eax = ZX8(MEM8(edi + 0xD));
    SET_LO8(ecx, MEM8(eax + 0x3D4B68));
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ecx), 1 (8-bit) */
    MEM32(esp + 0x10) = edx;
    MEM32(esp + 0x18) = edx;
    if (TEST_Z(_fa, _fb)) goto loc_003CA2E7; /* je: equal / zero */

loc_003CA2A2: ;
    esi = ebx;
    ecx = edi;
    PUSH32(esp, 0x003CA2ABu); RECOMP_ABI_CALL(0x003CE750u, sub_003CE750); /* call 0x003CE750 */

loc_003CA2AB: ;
    ecx = MEM32(ebp + 4);
    esi = eax;
    esi = esi & 0xFFFFFDFFu;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(esp + 0x18) = eax;
    eax = MEM32(ebp);
    esi = esi | 0x100;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ecx (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003CA2D6; /* jb: below (unsigned <) */

loc_003CA2C7: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003CA2D6u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003CA2D6: ;
    MEM32(eax) = 0x40208;
    MEM32(eax + 4) = esi;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(ebp) = eax;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */

loc_003CA2E7: ;
    ecx = MEM32(esp + 0x54);
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0xF0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ecx), 0xF0 (8-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003CA352; /* je: equal / zero */

loc_003CA2F0: ;
    eax = ZX8(MEM8(edi + 0xD));
    _fb = (uint32_t)(0xFFFFFFFDu) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0xFFFFFFFDu;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x19) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x19 (32-bit) */
    if (CMP_A(_fa, _fb)) goto loc_003CA352; /* ja: above (unsigned >) */

loc_003CA2FC: ;
    eax = ZX8(MEM8(eax + 0x3CA640));
    { uint32_t _jt = MEM32(eax * 4 + 0x3CA634); /* switch: 3 entries, 3 targets */
    if (_jt == 0x003CA30Au) goto loc_003CA30A;
    if (_jt == 0x003CA328u) goto loc_003CA328;
    if (_jt == 0x003CA352u) goto loc_003CA352;
    g_seh_ebp = ebp; RECOMP_ITAIL(_jt); return; }

loc_003CA30A: ;
    eax = MEM32(esp + 0x58);
    esi = eax;
    esi = esi >> 3;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edi = eax;
    esi = esi & 0x1F0000;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    edi = edi & 0xFC00;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    esi = esi | edi;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    esi = esi >> 2;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    goto loc_003CA344;

loc_003CA328: ;
    eax = MEM32(esp + 0x58);
    esi = eax;
    esi = esi >> 3;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edi = eax;
    esi = esi & 0x1F0000;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    edi = edi & 0xF800;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    esi = esi | edi;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    esi = esi >> 3;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */

loc_003CA344: ;
    eax = eax & 0xF8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    esi = esi | eax;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    esi = esi >> 3;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    MEM32(esp + 0x58) = esi;

loc_003CA352: ;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, edx (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CA363; /* jne: not equal / not zero */

loc_003CA356: ;
    ecx = ecx & 0xFFFFFFFCu;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(esp + 0x54) = ecx;
    if ((_fa == 0)) goto loc_003CA62A; /* je: equal / zero */

loc_003CA363: ;
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ecx), 1 (8-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003CA450; /* je: equal / zero */

loc_003CA36C: ;
    ecx = ZX8(MEM8(ebx + 0xD));
    _fb = (uint32_t)(0x2A) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    ecx = ecx - 0x2A;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    { uint32_t _jt = MEM32(ecx * 4 + 0x3CA65C); /* switch: 8 entries, 4 targets */
    if (_jt == 0x003CA37Au) goto loc_003CA37A;
    if (_jt == 0x003CA3ACu) goto loc_003CA3AC;
    if (_jt == 0x003CA3DEu) goto loc_003CA3DE;
    if (_jt == 0x003CA416u) goto loc_003CA416;
    g_seh_ebp = ebp; RECOMP_ITAIL(_jt); return; }

loc_003CA37A: ;
    fp_push(MEMF(esp + 0x5C)); /* fld float */
    PUSH32(esp, ecx);
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x41491C)); /* fmul dword ptr [0x41491c] */
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003CA38Du); RECOMP_ABI_CALL(0x003CC3C0u, sub_003CC3C0); /* call 0x003CC3C0 */

loc_003CA38D: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFF) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0xFFFF (32-bit) */
    if (CMP_LE(_fas, _fbs)) goto loc_003CA399; /* jle: less or equal (signed <=) */

loc_003CA394: ;
    eax = 0xFFFF;

loc_003CA399: ;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edx (32-bit) */
    SET_LO8(ecx, (CMP_L(_fas, _fbs)) ? 1 : 0); /* setl */
    ecx--;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    ecx = ecx & eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(esp + 0x10) = ecx;
    goto loc_003CA450;

loc_003CA3AC: ;
    fp_push(MEMF(esp + 0x5C)); /* fld float */
    PUSH32(esp, ecx);
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x415074)); /* fmul dword ptr [0x415074] */
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003CA3BFu); RECOMP_ABI_CALL(0x003CC3C0u, sub_003CC3C0); /* call 0x003CC3C0 */

loc_003CA3BF: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFFFF) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0xFFFFFF (32-bit) */
    if (CMP_LE(_fas, _fbs)) goto loc_003CA3CB; /* jle: less or equal (signed <=) */

loc_003CA3C6: ;
    eax = 0xFFFFFF;

loc_003CA3CB: ;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edx (32-bit) */
    SET_LO8(ecx, (CMP_L(_fas, _fbs)) ? 1 : 0); /* setl */
    ecx--;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    ecx = ecx & eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ecx = ecx << 8;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    MEM32(esp + 0x10) = ecx;
    goto loc_003CA450;

loc_003CA3DE: ;
    fp_push(MEMF(0x4148F0)); /* fld float */
    fp_push(MEMF(esp + 0x5C)); /* fld float */
    g_fp_cmp = RECOMP_FCMP(fp_top(), fp_st1()); g_fp_cc = RECOMP_FCMP_CC(g_fp_cmp); fp_pop(); fp_pop(); /* fucompp  */
    eax = (eax & 0xFFFF0000u) | (uint32_t)(uint16_t)(((g_fp_top & 7u) << 11) | g_fp_cc); /* fnstsw ax <- fpu status */
    _fa = (uint32_t)(HI8(eax)) & 0xFFu; _fb = (uint32_t)(0x44) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(eax), 0x44 (8-bit) */
    if ((!RECOMP_PARITY8((_fa) & (_fb)))) goto loc_003CA44C; /* jnp: not parity */

loc_003CA3F1: ;
    fp_push(MEMF(esp + 0x5C)); /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMD(0x415368)); /* fmul qword ptr [0x415368] */
    MEMD(esp + 0x30) = fp_top(); fp_pop(); /* fstp */
    eax = MEM32(esp + 0x34);
    eax = eax >> 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(0x8000) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - 0x8000;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    eax = eax & 0xFFFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(esp + 0x10) = eax;
    goto loc_003CA450;

loc_003CA416: ;
    fp_push(MEMF(0x4148F0)); /* fld float */
    fp_push(MEMF(esp + 0x5C)); /* fld float */
    g_fp_cmp = RECOMP_FCMP(fp_top(), fp_st1()); g_fp_cc = RECOMP_FCMP_CC(g_fp_cmp); fp_pop(); fp_pop(); /* fucompp  */
    eax = (eax & 0xFFFF0000u) | (uint32_t)(uint16_t)(((g_fp_top & 7u) << 11) | g_fp_cc); /* fnstsw ax <- fpu status */
    _fa = (uint32_t)(HI8(eax)) & 0xFFu; _fb = (uint32_t)(0x44) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(eax), 0x44 (8-bit) */
    if ((!RECOMP_PARITY8((_fa) & (_fb)))) goto loc_003CA44C; /* jnp: not parity */

loc_003CA429: ;
    fp_push(MEMF(esp + 0x5C)); /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMD(0x415360)); /* fmul qword ptr [0x415360] */
    MEMD(esp + 0x30) = fp_top(); fp_pop(); /* fstp */
    eax = MEM32(esp + 0x34);
    _fb = (uint32_t)(0xF8000000u) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0xF8000000u;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = eax & 0xFFFFFFF0u;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax = eax << 4;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    MEM32(esp + 0x10) = eax;
    goto loc_003CA450;

loc_003CA44C: ;
    MEM32(esp + 0x10) = edx;

loc_003CA450: ;
    ebx = MEM32(ebp + 0xEE0);
    ecx = MEM32(ebp + 0xEE4);
    esi = MEM32(ebp + 0xEE8);
    edi = MEM32(ebp + 0xEEC);
    eax = MEM32(esp + 0x4C);
    _fb = (uint32_t)(ebx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + ebx;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edi = edi + ecx;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edx (32-bit) */
    MEM32(esp + 0x2C) = ebx;
    MEM32(esp + 0x24) = ecx;
    MEM32(esp + 0x28) = esi;
    MEM32(esp + 0x30) = edi;
    if (CMP_NE(_fa, _fb)) goto loc_003CA4A6; /* jne: not equal / not zero */

loc_003CA484: ;
    edx = esp + 0x38;
    MEM32(esp + 0x38) = ebx;
    MEM32(esp + 0x3C) = ecx;
    MEM32(esp + 0x40) = esi;
    MEM32(esp + 0x44) = edi;
    MEM32(esp + 0x50) = edx;
    MEM32(esp + 0x4C) = 1;
    goto loc_003CA4B0;

loc_003CA4A6: ;
    edx = MEM32(esp + 0x50);
    goto loc_003CA4B0;

loc_003CA4AC: ;
    ecx = MEM32(esp + 0x24);

loc_003CA4B0: ;
    eax = MEM32(edx);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ebx (32-bit) */
    MEM32(esp + 0x14) = eax;
    if (CMP_G(_fas, _fbs)) goto loc_003CA4BE; /* jg: greater (signed >) */

loc_003CA4BA: ;
    MEM32(esp + 0x14) = ebx;

loc_003CA4BE: ;
    eax = MEM32(edx + 4);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ecx (32-bit) */
    MEM32(esp + 0x5C) = eax;
    if (CMP_G(_fas, _fbs)) goto loc_003CA4CD; /* jg: greater (signed >) */

loc_003CA4C9: ;
    MEM32(esp + 0x5C) = ecx;

loc_003CA4CD: ;
    eax = MEM32(edx + 8);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, esi (32-bit) */
    ecx = eax;
    if (CMP_L(_fas, _fbs)) goto loc_003CA4D8; /* jl: less (signed <) */

loc_003CA4D6: ;
    ecx = esi;

loc_003CA4D8: ;
    eax = MEM32(edx + 0xC);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edi (32-bit) */
    MEM32(esp + 0x1C) = ecx;
    if (CMP_L(_fas, _fbs)) goto loc_003CA4E5; /* jl: less (signed <) */

loc_003CA4E3: ;
    eax = edi;

loc_003CA4E5: ;
    _fa = (uint32_t)(MEM32(esp + 0x14)) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(esp + 0x14), ecx (32-bit) */
    MEM32(esp + 0x20) = eax;
    if (CMP_GE(_fas, _fbs)) goto loc_003CA5E2; /* jge: greater or equal (signed >=) */

loc_003CA4F3: ;
    _fa = (uint32_t)(MEM32(esp + 0x5C)) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(esp + 0x5C), eax (32-bit) */
    if (CMP_GE(_fas, _fbs)) goto loc_003CA5E2; /* jge: greater or equal (signed >=) */

loc_003CA4FD: ;
    fp_push((double)SMEM32(esp + 0x14)); /* fild */
    PUSH32(esp, ecx);
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(ebp + 0x95C)); /* fmul dword ptr [ebp + 0x95c] */
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C0)); /* fadd dword ptr [0x4148c0] */
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003CA516u); RECOMP_ABI_CALL(0x003CC3C0u, sub_003CC3C0); /* call 0x003CC3C0 */

loc_003CA516: ;
    fp_push((double)SMEM32(esp + 0x1C)); /* fild */
    PUSH32(esp, ecx);
    ebx = eax;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(ebp + 0x95C)); /* fmul dword ptr [ebp + 0x95c] */
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C0)); /* fadd dword ptr [0x4148c0] */
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003CA531u); RECOMP_ABI_CALL(0x003CC3C0u, sub_003CC3C0); /* call 0x003CC3C0 */

loc_003CA531: ;
    fp_push((double)SMEM32(esp + 0x5C)); /* fild */
    PUSH32(esp, ecx);
    esi = eax;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(ebp + 0x960)); /* fmul dword ptr [ebp + 0x960] */
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C0)); /* fadd dword ptr [0x4148c0] */
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003CA54Cu); RECOMP_ABI_CALL(0x003CC3C0u, sub_003CC3C0); /* call 0x003CC3C0 */

loc_003CA54C: ;
    fp_push((double)SMEM32(esp + 0x20)); /* fild */
    PUSH32(esp, ecx);
    MEM32(esp + 0x60) = eax;
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(ebp + 0x960)); /* fmul dword ptr [ebp + 0x960] */
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C0)); /* fadd dword ptr [0x4148c0] */
    MEMF(esp) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x003CA569u); RECOMP_ABI_CALL(0x003CC3C0u, sub_003CC3C0); /* call 0x003CC3C0 */

loc_003CA569: ;
    ecx = MEM32(ebp + 4);
    edi = eax;
    eax = MEM32(ebp);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ecx (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003CA588; /* jb: below (unsigned <) */

loc_003CA575: ;
    eax = MEM32(0x3D9588);
    edx = eax;
    edx = edx >> 1;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, edx);
    PUSH32(esp, 0x003CA584u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003CA584: ;
    edx = MEM32(esp + 0x50);

loc_003CA588: ;
    ecx = MEM32(esp + 0x10);
    esi = esi << 0x10;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(0x10000) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esi = esi - 0x10000;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    esi = esi | ebx;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    ebx = MEM32(esp + 0x5C);
    edi = edi << 0x10;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    MEM32(eax) = 0x81D98;
    MEM32(eax + 4) = esi;
    esi = MEM32(esp + 0x60);
    _fb = (uint32_t)(0x10000) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    edi = edi - 0x10000;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    edi = edi | ebx;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    ebx = MEM32(esp + 0x2C);
    ecx = ecx | esi;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    esi = MEM32(esp + 0x28);
    MEM32(eax + 8) = edi;
    edi = MEM32(esp + 0x30);
    MEM32(eax + 0xC) = 0xC1D8C;
    MEM32(eax + 0x10) = ecx;
    ecx = MEM32(esp + 0x58);
    MEM32(eax + 0x14) = ecx;
    ecx = MEM32(esp + 0x54);
    MEM32(eax + 0x18) = ecx;
    _fb = (uint32_t)(0x1C) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x1C;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(ebp) = eax;

loc_003CA5E2: ;
    eax = MEM32(esp + 0x4C);
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edx = edx + 0x10;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    MEM32(esp + 0x50) = edx;
    MEM32(esp + 0x4C) = eax;
    if ((_fa != 0)) goto loc_003CA4AC; /* jne: not equal / not zero */

loc_003CA5F8: ;
    eax = MEM32(esp + 0x18);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CA62A; /* je: equal / zero */

loc_003CA600: ;
    eax = MEM32(ebp);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(ebp + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(ebp + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003CA617; /* jb: below (unsigned <) */

loc_003CA608: ;
    eax = MEM32(0x3D9588);
    edx = eax;
    edx = edx >> 1;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, edx);
    PUSH32(esp, 0x003CA617u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003CA617: ;
    ecx = MEM32(esp + 0x18);
    MEM32(eax) = 0x40208;
    MEM32(eax + 4) = ecx;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(ebp) = eax;

loc_003CA62A: ;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebp);
    POP32(esp, ebx);
    _fb = (uint32_t)(0x38) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x38;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 28; return; /* ret 24 */

    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}

/**
 * sub_003CA720
 * Original: 0x003CA720 - 0x003CA7E2 (194 bytes, 62 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003CA720(void)
{
    RECOMP_ENTRY(0x003CA720u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003CA720: ;
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    PUSH32(esp, edi);
    if (CMP_B(_fa, _fb)) goto loc_003CA73E; /* jb: below (unsigned <) */

loc_003CA72F: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003CA73Eu); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003CA73E: ;
    edi = MEM32(esi + 0x2C);
    MEM32(eax) = 0x4A310;
    ecx = edi;
    ecx = ecx & 0x1F;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    edx = eax * 8;
    edx = edx | ecx;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    ecx = MEM32(esi + 0x40);
    ecx = ecx & 3;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    edx = edx << 2;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edx = edx | ecx;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(eax + 4) = edx;
    MEM32(eax + 8) = 0x41D70;
    MEM32(eax + 0xC) = edi;
    ecx = 0x41D90;
    MEM32(eax + 0x10) = ecx;
    MEM32(eax + 0x14) = 0;
    MEM32(eax + 0x18) = ecx;
    MEM32(eax + 0x1C) = 0;
    edx = eax + 0x20;
    MEM32(esi) = edx;
    SET_LO8(edx, MEM8(esp + 0xC));
    ecx = edi;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ecx = ecx & 0x3F;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(LO8(edx)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(edx), 1 (8-bit) */
    MEM32(esi + ecx * 8 + 0x64) = edi;
    MEM32(esi + ecx * 8 + 0x68) = eax;
    if (TEST_Z(_fa, _fb)) goto loc_003CA7BF; /* je: equal / zero */

loc_003CA7A3: ;
    ecx = MEM32(esi + 0x34);
    PUSH32(esp, ebp);
    ebp = MEM32(esi + 0x38);
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    ecx = ecx & ebp;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ebp = MEM32(esi + 0x48);
    MEM32(esi + 0x34) = ecx;
    MEM32(ebp + ecx * 8) = edi;
    ebp = MEM32(esi + 0x48);
    MEM32(ebp + ecx * 8 + 4) = eax;
    POP32(esp, ebp);

loc_003CA7BF: ;
    ecx = MEM32(esi + 0x2C);
    _fb = (uint32_t)(2) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 2;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(LO8(edx)) & 0xFFu; _fb = (uint32_t)(2) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(edx), 2 (8-bit) */
    MEM32(esi + 0x3C) = 0;
    MEM32(esi + 0x2C) = ecx;
    if (TEST_NZ(_fa, _fb)) goto loc_003CA7DB; /* jne: not equal / not zero */

loc_003CA7D4: ;
    edx = esi;
    PUSH32(esp, 0x003CA7DBu); RECOMP_ABI_CALL(0x003CACD0u, sub_003CACD0); /* call 0x003CACD0 */

loc_003CA7DB: ;
    eax = edi;
    POP32(esp, edi);
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003CAE80
 * Original: 0x003CAE80 - 0x003CAEAD (45 bytes, 9 insns)
 * D3D's write-combine flush before each kick-off: set the flush bit of NV_PFB_WC_CACHE (0x100410) and
 * read it until the GPU clears it. Three trapped register accesses, about 1300 times a second -- most
 * of the GPU's register traffic. Here the CPU's writes are in memory the GPU model reads directly, with
 * nothing in between to flush, so it only leaves eax as the original does (the register base). Not on
 * Windows by default: its players saw texture and white flashes in 0.9.20 (#16), the first version
 * without the flush. NGB_WC_FLUSH=1 or 0: the flush, or none, on either.
 */
void sub_003CAE80(void)
{
    RECOMP_ENTRY(0x003CAE80u);
    static int flush = -1;
    if (flush < 0) {
        extern char *getenv(const char *);
        const char *e = getenv("NGB_WC_FLUSH");
#if defined(_WIN32)
        flush = !(e && *e == '0');
#else
        flush = e && *e == '1';
#endif
    }
    eax = MEM32(0x3D83C8);
    eax = MEM32(eax + 0x1C28);
    if (flush) {
        ecx = MEM32(eax + 0x100410) | 0x10000;
        MEM32(eax + 0x100410) = ecx;
        while (MEM32(eax + 0x100410) & 0x10000)
            RECOMP_SPIN(0x003CAEA0u);
    }
    esp += 4; return; /* ret */
}

/**
 * sub_003CAA10
 * Original: 0x003CAA10 - 0x003CAB76 (358 bytes, 117 insns)
 * Category: game_render
 * CC: cdecl, 2 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003CAA10(void)
{
    RECOMP_ENTRY(0x003CAA10u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003CAA10: ;
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    eax = MEM32(esi + 0x30);
    ecx = MEM32(eax);
    eax = MEM32(esi + 0x2C);
    edx = eax;
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    edx = edx - ecx;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, edi);
    edi = MEM32(esp + 0xC);
    ecx = eax;
    _fb = (uint32_t)(edi) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    ecx = ecx - edi;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, edx (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_003CAB4C; /* jae: above or equal (unsigned >=) */

loc_003CAA34: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, eax (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CAA3F; /* jne: not equal / not zero */

loc_003CAA38: ;
    PUSH32(esp, 0);
    PUSH32(esp, 0x003CAA3Fu); RECOMP_ABI_CALL(0x003CA720u, sub_003CA720); /* call 0x003CA720 */

loc_003CAA3F: ;
    PUSH32(esp, ebp);
    eax = edi;
    PUSH32(esp, 0x003CAA47u); RECOMP_ABI_CALL(0x003CA820u, sub_003CA820); /* call 0x003CA820 */

loc_003CAA47: ;
    ebp = eax;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CAB4B; /* je: equal / zero */

loc_003CAA51: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    PUSH32(esp, 0x003CAA58u); RECOMP_ABI_CALL(0x003CA9A0u, sub_003CA9A0); /* call 0x003CA9A0 */

loc_003CAA58: ;
    edx = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edx (32-bit) */
    if (CMP_A(_fa, _fb)) goto loc_003CAA61; /* ja: above (unsigned >) */

loc_003CAA5E: ;
    _fb = (uint32_t)(MEM32(esi + 0x44)) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + MEM32(esi + 0x44);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003CAA61: ;
    ecx = MEM32(ebp + 4);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, edx (32-bit) */
    if (CMP_A(_fa, _fb)) goto loc_003CAA6B; /* ja: above (unsigned >) */

loc_003CAA68: ;
    _fb = (uint32_t)(MEM32(esi + 0x44)) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + MEM32(esi + 0x44);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003CAA6B: ;
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    ecx = ecx - eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, ebx);
    ebx = MEM32(esp + 0x18);
    if ((_fas < 0)) goto loc_003CAB13; /* js: sign (negative) */

loc_003CAA78: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x2400) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x2400 (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003CAB13; /* jb: below (unsigned <) */

loc_003CAA84: ;
    eax = MEM32(esi + 0x19FC);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CAAB4; /* je: equal / zero */

loc_003CAA8E: ;
    (void)0; /* emms - empty MMX state */
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, ebx);
    MEM32(esi + 0x1A00) = edi;
    { uint32_t _icall_target = MEM32(esi + 0x19FC); PUSH32(esp, 0x003CAA9Du); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x003CAA97u); } /* indirect call */
    }

loc_003CAA9D: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    PUSH32(esp, 0x003CAAA7u); RECOMP_ABI_CALL(0x003CA9A0u, sub_003CA9A0); /* call 0x003CA9A0 */

loc_003CAAA7: ;
    PUSH32(esp, ebp);
    PUSH32(esp, 0x003CAAADu); RECOMP_ABI_CALL(0x003CA7F0u, sub_003CA7F0); /* call 0x003CA7F0 */

loc_003CAAAD: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x2000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x2000 (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003CAB13; /* jb: below (unsigned <) */

loc_003CAAB4: ;
    edi = MEM32(ebp + 4);
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(LO8(ebx)) & 0xFFu; _fb = (uint32_t)(8) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ebx), 8 (8-bit) */
    MEM32(esi + 0x1DD0) = eax;
    if (TEST_NZ(_fa, _fb)) goto loc_003CAACE; /* jne: not equal / not zero */

loc_003CAAC4: ;
    MEM32(edi + 0x10) = 0x40110;
    MEM32(edi + 0x14) = eax;

loc_003CAACE: ;
    MEM32(edi + 0x18) = 0x40100;
    MEM32(edi + 0x1C) = 5;
    PUSH32(esp, 0x003CAAE1u); RECOMP_ABI_CALL(0x003CAE80u, sub_003CAE80); /* call 0x003CAE80 */

loc_003CAAE1: ;
    eax = 1;
    PUSH32(esp, 0x003CAAEBu); RECOMP_ABI_CALL(0x003CA9A0u, sub_003CA9A0); /* call 0x003CA9A0 */

loc_003CAAEB: ;
    PUSH32(esp, ebp);
    PUSH32(esp, 0x003CAAF1u); RECOMP_ABI_CALL(0x003CA7F0u, sub_003CA7F0); /* call 0x003CA7F0 */

loc_003CAAF1: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x2000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x2000 (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_003CAB51; /* jae: above or equal (unsigned >=) */

loc_003CAAF8: ;
    eax = 0x40100;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    MEM32(edi + 0x10) = eax;
    MEM32(edi + 0x14) = ecx;
    MEM32(edi + 0x18) = eax;
    MEM32(edi + 0x1C) = ecx;
    PUSH32(esp, 0x003CAB10u); RECOMP_ABI_CALL(0x003CAE80u, sub_003CAE80); /* call 0x003CAE80 */

loc_003CAB10: ;
    edi = MEM32(ebp);

loc_003CAB13: ;
    eax = MEM32(esi + 0x19FC);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CAB35; /* je: equal / zero */

loc_003CAB1D: ;
    (void)0; /* emms - empty MMX state */
    ebx = ebx | 0x80000000u;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, ebx);
    MEM32(esi + 0x1A00) = edi;
    { uint32_t _icall_target = MEM32(esi + 0x19FC); PUSH32(esp, 0x003CAB32u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x003CAB2Cu); } /* indirect call */
    }

loc_003CAB32: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003CAB35: ;
    edx = MEM32(esi + 0x30);
    esi = MEM32(esi + 0x2C);
    eax = esi;
    _fb = (uint32_t)(edi) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - edi;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    /* nop */
    /* the loop below polls the GPU's progress (a semaphore it writes to memory) until it has reached
     * the point: asleep in the GPU model instead, which wakes it when the GPU writes it; the loop then
     * reads once and goes on with its registers as the console would have them. Not on Windows by
     * default: 0.9.20's sleeper let the virtual CPU go there and getting it back took up to a quantum
     * (the Game Over fade fell from 30 to 17 fps, play turned choppy: #43, #17); 0.9.32's kept it and
     * yielded each millisecond, and the thread that took the CPU kept it for its quantum, so frames ran
     * 30 ms now and then (#107, #69). Windows polls as the title does. NGB_GPU_WAIT=1 or 0: the sleep,
     * or the polling loop, on either. */
    {
        extern void nv2a_vk_wait_progress(const uint32_t *word, uint32_t put, uint32_t ahead);
        extern void nv2a_vk_wait_dma_get(uint32_t put, uint32_t ahead);
        extern int g_ngb_gpu_model;
        static int sleep_wait = -1;
        if (sleep_wait < 0) {
            extern char *getenv(const char *);
            const char *e = getenv("NGB_GPU_WAIT");
#if defined(_WIN32)
            sleep_wait = e && *e == '1';
#else
            sleep_wait = !(e && *e == '0');
#endif
        }
        if (sleep_wait && g_ngb_gpu_model && edx == 0xFD800044u) nv2a_vk_wait_dma_get(esi, eax);
        else if (sleep_wait && g_ngb_gpu_model) nv2a_vk_wait_progress((const uint32_t *)XBOX_PTR(edx), esi, eax);
    }

loc_003CAB40: ;
    ecx = MEM32(edx);
    edi = esi;
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    edi = edi - ecx;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edi (32-bit) */
    if (CMP_B(_fa, _fb)) { RECOMP_SPIN(0x003CAB40u); goto loc_003CAB40; } /* jb: below (unsigned <) */

loc_003CAB4A: ;
    POP32(esp, ebx);

loc_003CAB4B: ;
    POP32(esp, ebp);

loc_003CAB4C: ;
    POP32(esp, edi);
    POP32(esp, esi);
    esp += 12; return; /* ret 8 */

loc_003CAB51: ;
    edi = MEM32(0x3E14D0);
    _fb = (uint32_t)(0x1DCC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + 0x1DCC;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    /* nop */

loc_003CAB60: ;
    PUSH32(esp, 0);
    PUSH32(esp, 0);
    PUSH32(esp, 1);
    PUSH32(esp, 6);
    PUSH32(esp, esi);
    { uint32_t _icall_esp = g_esp;
    { uint32_t _icall_target = edi; PUSH32(esp, 0x003CAB6Bu); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x003CAB69u); } /* indirect call */
    }

loc_003CAB6B: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CAB60; /* jne: not equal / not zero */

loc_003CAB6F: ;
    POP32(esp, ebx);
    POP32(esp, ebp);
    POP32(esp, edi);
    POP32(esp, esi);
    esp += 12; return; /* ret 8 */

}

/**
 * sub_003CAB80
 * Original: 0x003CAB80 - 0x003CACBF (319 bytes, 117 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003CAB80(void)
{
    RECOMP_ENTRY(0x003CAB80u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003CAB80: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, ebx);
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    ebx = eax;
    _fa = (uint32_t)(MEM8(esi + 8)) & 0xFFu; _fb = (uint32_t)(4) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test MEM8(esi + 8), 4 (8-bit) */
    PUSH32(esp, edi);
    if (TEST_Z(_fa, _fb)) goto loc_003CABBD; /* je: equal / zero */

loc_003CAB94: ;
    eax = MEM32(esi + 0x764);
    edi = MEM32(eax + 4);
    ecx = MEM32(esi);
    edx = MEM32(esi + 0x768);
    _fb = (uint32_t)(edi) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    ecx = ecx - edi;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edx = edx + ecx;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi + 0x768) = edx;
    eax = MEM32(eax + 4);
    POP32(esp, edi);
    MEM32(esi) = eax;
    POP32(esp, esi);
    POP32(esp, ebx);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 8; return; /* ret 4 */

loc_003CABBD: ;
    edi = MEM32(esi);
    edx = MEM32(esi + 0x30);
    eax = MEM32(edx);
    PUSH32(esp, ebp);
    ebp = edi + ebx;
    ecx = esi;
    MEM32(esp + 0x10) = ebp;
    MEM32(esp + 0x14) = eax;
    PUSH32(esp, 0x003CABD7u); RECOMP_ABI_CALL(0x003CA920u, sub_003CA920); /* call 0x003CA920 */

loc_003CABD7: ;
    ecx = MEM32(esi + 0x28);
    _fb = (uint32_t)(0x4000) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ebp = ebp + 0x4000;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, ecx (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003CAC27; /* jb: below (unsigned <) */

loc_003CABE4: ;
    edx = MEM32(esp + 0x1C);
    _fb = (uint32_t)(edi) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edx = edx + edi;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, ecx (32-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_003CAC23; /* jbe: below or equal (unsigned <=) */

loc_003CABEE: ;
    ecx = MEM32(esi + 0x40);
    edx = MEM32(esi);
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(esi + 0x40) = ecx;
    ecx = MEM32(esi + 0x24);
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    edx = edx - ecx;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    ecx = ecx & 0xFFFFFFF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(esi + 0x44) = edx;
    edx = MEM32(esi);
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(edx) = ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi) (32-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_003CAC14; /* jbe: below or equal (unsigned <=) */

loc_003CAC0E: ;
    eax = MEM32(esi + 0x24);
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 4;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003CAC14: ;
    edi = MEM32(esi + 0x24);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edi (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CAC1E; /* jne: not equal / not zero */

loc_003CAC1B: ;
    eax = edi + 4;

loc_003CAC1E: ;
    ecx = edi + ebx;
    MEM32(esi) = edi;

loc_003CAC23: ;
    MEM32(esp + 0x10) = ecx;

loc_003CAC27: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, eax (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_003CAC77; /* jae: above or equal (unsigned >=) */

loc_003CAC2B: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esp + 0x10)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esp + 0x10) (32-bit) */
    if (CMP_A(_fa, _fb)) goto loc_003CAC77; /* ja: above (unsigned >) */

loc_003CAC31: ;
    ebx = MEM32(0x3D958C);
    eax = MEM32(esi + 0x34);
    edx = MEM32(esi + 0x48);
    ebp = MEM32(esi + 0x38);
    ebx = ebx >> 1;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(edi) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ebx = ebx + edi;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003CAC44: ;
    ecx = MEM32(edx + eax * 8);
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    eax = eax & ebp;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(esp + 0x1C) = ecx;
    ecx = MEM32(edx + eax * 8 + 4);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, edi (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_003CAC59; /* jae: above or equal (unsigned >=) */

loc_003CAC56: ;
    _fb = (uint32_t)(MEM32(esi + 0x44)) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + MEM32(esi + 0x44);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003CAC59: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, ebx (32-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_003CAC6B; /* jbe: below or equal (unsigned <=) */

loc_003CAC5D: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 0x34)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 0x34) (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CAC6B; /* je: equal / zero */

loc_003CAC62: ;
    ecx = MEM32(esp + 0x14);
    _fa = (uint32_t)(MEM32(edx + eax * 8)) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(edx + eax * 8), ecx (32-bit) */
    if (CMP_A(_fa, _fb)) goto loc_003CAC44; /* ja: above (unsigned >) */

loc_003CAC6B: ;
    edx = MEM32(esp + 0x1C);
    PUSH32(esp, 8);
    PUSH32(esp, edx);
    PUSH32(esp, 0x003CAC77u); RECOMP_ABI_CALL(0x003CAA10u, sub_003CAA10); /* call 0x003CAA10 */

loc_003CAC77: ;
    eax = MEM32(esp + 0x10);
    _fb = (uint32_t)(0xFFFFFDFCu) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0xFFFFFDFCu;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi + 4) = eax;
    eax = MEM32(esi + 8);
    _fa = (uint32_t)(HI8(eax)) & 0xFFu; _fb = (uint32_t)(8) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(eax), 8 (8-bit) */
    POP32(esp, ebp);
    if (TEST_Z(_fa, _fb)) goto loc_003CACA6; /* je: equal / zero */

loc_003CAC8C: ;
    eax = eax | 0x1000;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    edx = esi;
    MEM32(esi + 8) = eax;
    PUSH32(esp, 0x003CAC9Bu); RECOMP_ABI_CALL(0x003CACD0u, sub_003CACD0); /* call 0x003CACD0 */

loc_003CAC9B: ;
    eax = MEM32(esi);
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebx);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 8; return; /* ret 4 */

loc_003CACA6: ;
    PUSH32(esp, 3);
    PUSH32(esp, 0x003CACADu); RECOMP_ABI_CALL(0x003CA720u, sub_003CA720); /* call 0x003CA720 */

loc_003CACAD: ;
    edx = esi;
    PUSH32(esp, 0x003CACB4u); RECOMP_ABI_CALL(0x003CACD0u, sub_003CACD0); /* call 0x003CACD0 */

loc_003CACB4: ;
    eax = MEM32(esi);
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebx);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003CACC0
 * Original: 0x003CACC0 - 0x003CACD0 (16 bytes, 6 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003CACC0(void)
{
    RECOMP_ENTRY(0x003CACC0u);
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003CACC0: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003CACCFu); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003CACCF: ;
    esp += 4; return; /* ret */

}

/**
 * sub_003CACD0
 * Original: 0x003CACD0 - 0x003CAD72 (162 bytes, 46 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003CACD0(void)
{
    RECOMP_ENTRY(0x003CACD0u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003CACD0: ;
    eax = MEM32(edx + 8);
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(4) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(eax), 4 (8-bit) */
    PUSH32(esp, esi);
    if (TEST_Z(_fa, _fb)) goto loc_003CACE0; /* je: equal / zero */

loc_003CACD8: ;
    esi = MEM32(edx + 0x770);
    goto loc_003CACE2;

loc_003CACE0: ;
    esi = MEM32(edx);

loc_003CACE2: ;
    _fa = (uint32_t)(HI8(eax)) & 0xFFu; _fb = (uint32_t)(0x20) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(eax), 0x20 (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003CAD38; /* jne: not equal / not zero */

loc_003CACE7: ;
    PUSH32(esp, 0x003CACECu); RECOMP_ABI_CALL(0x003CAE80u, sub_003CAE80); /* call 0x003CAE80 */

loc_003CACEC: ;
    eax = MEM32(edx + 0x1C20);
    esi = esi & 0xFFFFFFF;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    GW32(eax + 0x40, esi);   /* the GPU's PUT, without a fault (src/d3d/gpu_regs.h) */
    eax = MEM32(0x3D5F10);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CAD70; /* je: equal / zero */

loc_003CAD04: ;
    ecx = MEM32(0x3D83C8);
    eax = MEM32(ecx + 0x934);

loc_003CAD10: ;
    ecx = MEM32(eax + 0x3240);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(eax + 0x3244)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, MEM32(eax + 0x3244) (32-bit) */
    if (CMP_NE(_fa, _fb)) { RECOMP_SPIN(0x003CAD10u); goto loc_003CAD10; } /* jne: not equal / not zero */

loc_003CAD1E: ;
    esi = MEM32(eax + 0x400700);
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0 (32-bit) */
    SET_LO8(ecx, (CMP_NE(_fa, _fb)) ? 1 : 0); /* setne */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CAD10; /* jne: not equal / not zero */

loc_003CAD2F: ;
    MEM32(edx + 8) = MEM32(edx + 8) | 0x2000;
    _fa = (uint32_t)(MEM32(edx + 8)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    POP32(esp, esi);
    esp += 4; return; /* ret */

loc_003CAD38: ;
    eax = MEM32(edx);
    eax = eax & 0xFFFFFFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ecx = 0x3D83C0;
    _fb = (uint32_t)(0x40) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    ecx = ecx - 0x40;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    MEM32(edx + 0x1C20) = ecx;
    MEM32(ecx + 0x40) = eax;
    ecx = MEM32(edx + 0x1C20);
    MEM32(ecx + 0x44) = eax;
    eax = MEM32(edx + 0x2C);
    ecx = MEM32(edx + 0x30);
    _fb = (uint32_t)(2) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - 2;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    MEM32(ecx) = eax;
    eax = MEM32(edx + 0x2478);
    MEM32(edx + 0x1DE4) = eax;

loc_003CAD70: ;
    POP32(esp, esi);
    esp += 4; return; /* ret */

}

/**
 * sub_003CADF0
 * Original: 0x003CADF0 - 0x003CAE7C (140 bytes, 50 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003CADF0(void)
{
    RECOMP_ENTRY(0x003CADF0u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003CADF0: ;
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CAE7A; /* je: equal / zero */

loc_003CADFB: ;
    edx = MEM32(eax);
    ecx = edx;
    ecx = ecx & 0x70000;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x50000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x50000 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CAE37; /* jne: not equal / not zero */

loc_003CAE0D: ;
    ecx = MEM32(eax + 0x14);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CAE37; /* je: equal / zero */

loc_003CAE14: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x780000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test edx, 0x780000 (32-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003CAE28; /* jne: not equal / not zero */

loc_003CAE1C: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CAE35; /* je: equal / zero */

loc_003CAE20: ;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu; _fb = (uint32_t)(0x780000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test MEM32(ecx), 0x780000 (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003CAE35; /* je: equal / zero */

loc_003CAE28: ;
    edx = MEM32(esi + 0x2C);
    PUSH32(esp, 2);
    PUSH32(esp, edx);
    PUSH32(esp, 0x003CAE33u); RECOMP_ABI_CALL(0x003CAA10u, sub_003CAA10); /* call 0x003CAA10 */

loc_003CAE33: ;
    POP32(esp, esi);
    esp += 4; return; /* ret */

loc_003CAE35: ;
    eax = ecx;

loc_003CAE37: ;
    ecx = MEM32(eax);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x780000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test ecx, 0x780000 (32-bit) */
    edx = MEM32(eax + 8);
    if (TEST_NZ(_fa, _fb)) goto loc_003CAE61; /* jne: not equal / not zero */

loc_003CAE44: ;
    ecx = ecx & 0x70000;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x50000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x50000 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CAE6E; /* jne: not equal / not zero */

loc_003CAE52: ;
    eax = MEM32(eax + 0x14);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CAE6E; /* je: equal / zero */

loc_003CAE59: ;
    _fa = (uint32_t)(MEM32(eax)) & 0xFFFFFFFFu; _fb = (uint32_t)(0x780000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test MEM32(eax), 0x780000 (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003CAE6E; /* je: equal / zero */

loc_003CAE61: ;
    eax = MEM32(esi + 0x2C);
    PUSH32(esp, 2);
    PUSH32(esp, eax);
    PUSH32(esp, 0x003CAE6Cu); RECOMP_ABI_CALL(0x003CAA10u, sub_003CAA10); /* call 0x003CAA10 */

loc_003CAE6C: ;
    POP32(esp, esi);
    esp += 4; return; /* ret */

loc_003CAE6E: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CAE7A; /* je: equal / zero */

loc_003CAE72: ;
    PUSH32(esp, 0x10);
    PUSH32(esp, edx);
    PUSH32(esp, 0x003CAE7Au); RECOMP_ABI_CALL(0x003CAA10u, sub_003CAA10); /* call 0x003CAA10 */

loc_003CAE7A: ;
    POP32(esp, esi);
    esp += 4; return; /* ret */

}

/**
 * sub_003CAFC0
 * Original: 0x003CAFC0 - 0x003CB2CD (781 bytes, 243 insns)
 * Category: game_render
 * CC: cdecl, 2 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003CAFC0(void)
{
    RECOMP_ENTRY(0x003CAFC0u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003CAFC0: ;
    _fb = (uint32_t)(0x30) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x30;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    PUSH32(esp, edi);
    ebp = 0; /* xor self */
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    PUSH32(esp, ebp);
    PUSH32(esp, esi);
    PUSH32(esp, 0x003CAFD6u); RECOMP_ABI_CALL(0x003D1EF0u, sub_003D1EF0); /* call 0x003D1EF0 */

loc_003CAFD6: ;
    edx = MEM32(esp + 0x44);
    _fa = (uint32_t)(MEM32(edx)) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(edx), 0 (32-bit) */
    if ((((uint32_t)((uint32_t)(_fas) - (uint32_t)(_fbs)) >> 31) != 0)) goto loc_003CB26B; /* js: sign (negative) */

loc_003CAFE3: ;
    ebx = MEM32(edx + 0xC);
    edi = MEM32(edx + 4);
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    ebx = ebx - 4;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    eax = edi + ebx;
    MEM32(esp + 0x20) = eax;
    eax = MEM32(esp + 0x48);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    MEM32(esp + 0x24) = ebx;
    MEM32(esp + 0x14) = ebp;
    if (CMP_EQ(_fa, _fb)) goto loc_003CB01B; /* je: equal / zero */

loc_003CB003: ;
    ebp = MEM32(eax + 4);
    ecx = MEM32(eax + 0xC);
    _fb = (uint32_t)(ebp) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + ebp;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esp + 0x10) = 0x20000000;
    ebp = MEM32(esp + 0x10);
    MEM32(esp + 0x14) = ecx;

loc_003CB01B: ;
    SET_LO8(eax, MEM8(esi + 8));
    ecx = MEM32(esi + 4);
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(4) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(eax), 4 (8-bit) */
    eax = MEM32(esi);
    if (TEST_NZ(_fa, _fb)) goto loc_003CB206; /* jne: not equal / not zero */

loc_003CB02B: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ecx (32-bit) */
    MEM32(esp + 0x18) = eax;
    if (CMP_B(_fa, _fb)) goto loc_003CB04A; /* jb: below (unsigned <) */

loc_003CB033: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003CB042u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003CB042: ;
    edx = MEM32(esp + 0x44);
    MEM32(esp + 0x18) = eax;

loc_003CB04A: ;
    ecx = MEM32(esi + 0x30);
    ecx = MEM32(ecx);
    MEM32(esp + 0x10) = ecx;
    ecx = MEM32(esi + 0x2C);
    MEM32(esp + 0x1C) = ecx;
    _fb = (uint32_t)(MEM32(edx + 8)) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    ecx = ecx - MEM32(edx + 8);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    edx = MEM32(esp + 0x10);
    MEM32(esp + 0x28) = ecx;
    ecx = MEM32(esp + 0x1C);
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    ecx = ecx - edx;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    edx = MEM32(esp + 0x28);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, ecx (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003CB0C5; /* jb: below (unsigned <) */

loc_003CB073: ;
    ecx = MEM32(esp + 0x14);
    edx = edi;
    edx = edx & 0xFFFFFFF;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 4;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edx++;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(eax + -4) = edx;
    MEM32(esp + 0x1C) = eax;
    eax = eax & 0xFFFFFFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax = eax | ebp;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(esp + 0x2C) = eax;
    eax = MEM32(esp + 0x20);
    MEM32(esp + 0x34) = eax;
    eax = esp + 0x2C;
    MEM32(esp + 0x30) = 0;
    MEM32(esp + 0x38) = ecx;
    MEM32(esp + 0x3C) = edi;
    PUSH32(esp, 0x003CB0B6u); RECOMP_ABI_CALL(0x003CAF20u, sub_003CAF20); /* call 0x003CAF20 */

loc_003CB0B6: ;
    edx = MEM32(esi + 0x54);
    eax = MEM32(esp + 0x44);
    MEM32(eax + 0x14) = edx;
    goto loc_003CB1BF;

loc_003CB0C5: ;
    ecx = edi;
    ecx = ecx & 0xFFFFFFF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(eax) = 0x40100;
    MEM32(eax + 4) = 0;
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(eax + 8) = ecx;
    ecx = eax + 0xC;
    eax = MEM32(esi + 0x4C);
    eax = eax & 0x3F;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    edx = eax + eax * 4;
    ebx = esi + edx * 4 + 0x264;
    _fa = (uint32_t)(MEM32(ebx)) & 0xFFFFFFFFu; _fb = (uint32_t)(0x40000000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test MEM32(ebx), 0x40000000 (32-bit) */
    MEM32(esp + 0x1C) = ecx;
    if (TEST_NZ(_fa, _fb)) goto loc_003CB11D; /* jne: not equal / not zero */

loc_003CB0FD: ;
    ecx = ecx & 0xFFFFFFF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ecx = ecx | 0xC0000000u;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    ebp = ebp | ecx;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(esp + 0x10) = ebp;
    ebp = eax;
    ebp = ebp << 5;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ebp = ebp | 0xD;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(esi + 0x4C) = eax;
    goto loc_003CB13A;

loc_003CB11D: ;
    ebx = ecx;
    _fb = (uint32_t)(0x14) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 0x14;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esp + 0x1C) = ecx;
    ecx = ecx & 0xFFFFFFF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ebp = ebp | ecx;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(esp + 0x10) = ebp;
    ebp = ebx;
    ebp = ebp << 5;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ebp = ebp | 0xC;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */

loc_003CB13A: ;
    eax = MEM32(esp + 0x10);
    ecx = MEM32(esp + 0x24);
    edx = MEM32(esp + 0x14);
    MEM32(ebx) = eax;
    eax = edi + ecx;
    MEM32(ebx + 4) = 0;
    MEM32(ebx + 8) = eax;
    MEM32(ebx + 0xC) = edx;
    MEM32(ebx + 0x10) = edi;
    eax = MEM32(esi + 0x50);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    edi = esi + 0x50;
    MEM32(esp + 0x20) = ebx;
    if (CMP_BE(_fa, _fb)) goto loc_003CB193; /* jbe: below or equal (unsigned <=) */

loc_003CB169: ;
    eax = MEM32(esp + 0x44);
    ecx = MEM32(eax + 0x14);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 0x54)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, MEM32(esi + 0x54) (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CB193; /* je: equal / zero */

loc_003CB175: ;
    edx = MEM32(esi + 0x58);
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edx = edx + 4;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esp + 0x28) = edx;
    eax = 0;
    ecx = MEM32(esp + 0x28);
    edx = MEM32(esp + 0x20);
    { uint32_t _cmp = eax;
      uint32_t _old = RECOMP_ATOMIC_CAS32(XBOX_PTR(ecx), _cmp, edx);
      _fa = _old; _fb = _cmp;
      _fas = (int32_t)_fa; _fbs = (int32_t)_fb;
      if (_old != _cmp) eax = _old; }  /* cmpxchg */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CB1AE; /* je: equal / zero */

loc_003CB193: ;
    MEM32(esp + 0x28) = edi;
    eax = 1;
    ecx = MEM32(esp + 0x28);
    { uint32_t _old = RECOMP_ATOMIC_ADD32(XBOX_PTR(ecx), eax);
      eax = _old; }  /* xadd */
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(esi + 0x54) = MEM32(esi + 0x54) + 1;
    _fa = (uint32_t)(MEM32(esi + 0x54)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    eax = MEM32(esp + 0x18);
    MEM32(eax + 4) = ebp;

loc_003CB1AE: ;
    ecx = MEM32(esi + 0x54);
    edx = MEM32(esp + 0x44);
    MEM32(edx + 0x14) = ecx;
    MEM32(esi + 0x58) = ebx;
    ebx = MEM32(esp + 0x24);

loc_003CB1BF: ;
    edx = MEM32(esi + 0x3C);
    eax = MEM32(esp + 0x1C);
    ecx = MEM32(0x3D9588);
    _fb = (uint32_t)(ebx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edx = edx + ebx;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi) = eax;
    eax = edx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ecx (32-bit) */
    MEM32(esi + 0x3C) = edx;
    if (CMP_BE(_fa, _fb)) goto loc_003CB1E0; /* jbe: below or equal (unsigned <=) */

loc_003CB1D9: ;
    PUSH32(esp, 0);
    PUSH32(esp, 0x003CB1E0u); RECOMP_ABI_CALL(0x003CA720u, sub_003CA720); /* call 0x003CA720 */

loc_003CB1E0: ;
    edx = MEM32(esp + 0x44);

loc_003CB1E4: ;
    eax = MEM32(esi + 0x2C);
    MEM32(edx + 8) = eax;
    eax = MEM32(esp + 0x48);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CB2C3; /* je: equal / zero */

loc_003CB1F6: ;
    ecx = MEM32(esi + 0x2C);
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebp);
    MEM32(eax + 8) = ecx;
    POP32(esp, ebx);
    _fb = (uint32_t)(0x30) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x30;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 12; return; /* ret 8 */

loc_003CB206: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ecx (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003CB21D; /* jb: below (unsigned <) */

loc_003CB20A: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003CB219u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003CB219: ;
    edx = MEM32(esp + 0x44);

loc_003CB21D: ;
    ecx = eax + 0xC;
    MEM32(eax) = 0x40100;
    ebx = ecx;
    ebx = ebx << 5;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ebx = ebx | 0xE;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(eax + 4) = ebx;
    ebx = edi;
    ebx = ebx & 0xFFFFFFF;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ebx++;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(eax + 8) = ebx;
    eax = ecx + 0x14;
    ebx = eax;
    ebx = ebx & 0xFFFFFFF;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ebx = ebx | ebp;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(ecx) = ebx;
    ebx = MEM32(esp + 0x20);
    MEM32(ecx + 4) = 0;
    MEM32(ecx + 8) = ebx;
    ebx = MEM32(esp + 0x14);
    MEM32(ecx + 0xC) = ebx;
    MEM32(ecx + 0x10) = edi;
    MEM32(esi) = eax;
    goto loc_003CB1E4;

loc_003CB26B: ;
    edi = MEM32(edx + 0xC);
    ecx = MEM32(0x3D83C8);
    eax = MEM32(ecx);
    ecx = MEM32(ecx + 4);
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    edi = edi - 4;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    edi = edi >> 2;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ebx = eax + edi * 4;
    _fb = (uint32_t)(0x200) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 0x200;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, ecx (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003CB2B3; /* jb: below (unsigned <) */

loc_003CB28C: ;
    ecx = MEM32(0x3D9588);
    eax = ecx;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edx = edi * 4 + 0x204;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, ecx (32-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_003CB2A3; /* jbe: below or equal (unsigned <=) */

loc_003CB2A1: ;
    ecx = edx;

loc_003CB2A3: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, eax (32-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_003CB2A9; /* jbe: below or equal (unsigned <=) */

loc_003CB2A7: ;
    eax = edx;

loc_003CB2A9: ;
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003CB2AFu); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003CB2AF: ;
    edx = MEM32(esp + 0x44);

loc_003CB2B3: ;
    edx = MEM32(edx + 4);
    PUSH32(esp, edi);
    PUSH32(esp, edx);
    PUSH32(esp, eax);
    PUSH32(esp, 0x003CB2BEu); RECOMP_ABI_CALL(0x003C9830u, sub_003C9830); /* call 0x003C9830 */

loc_003CB2BE: ;
    eax = eax + edi * 4;
    MEM32(esi) = eax;

loc_003CB2C3: ;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebp);
    POP32(esp, ebx);
    _fb = (uint32_t)(0x30) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x30;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 12; return; /* ret 8 */

}

/**
 * sub_003CC160
 * Original: 0x003CC160 - 0x003CC249 (233 bytes, 73 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003CC160(void)
{
    RECOMP_ENTRY(0x003CC160u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003CC160: ;
    PUSH32(esp, ebx);
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    ebx = eax;
    eax = MEM32(esi + 0x8D4);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    PUSH32(esp, edi);
    if (CMP_NE(_fa, _fb)) goto loc_003CC17F; /* jne: not equal / not zero */

loc_003CC175: ;
    eax = MEM32(esi + 0x8D8);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CC184; /* je: equal / zero */

loc_003CC17F: ;
    PUSH32(esp, 0x003CC184u); RECOMP_ABI_CALL(0x003D25F0u, sub_003D25F0); /* call 0x003D25F0 */

loc_003CC184: ;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CC18D; /* jne: not equal / not zero */

loc_003CC188: ;
    ebx = 5;

loc_003CC18D: ;
    eax = MEM32(esi + 8);
    _fa = (uint32_t)(HI8(eax)) & 0xFFu; _fb = (uint32_t)(0x40) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(eax), 0x40 (8-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003CC1D5; /* je: equal / zero */

loc_003CC195: ;
    _fa = (uint32_t)(LO8(ebx)) & 0xFFu; _fb = (uint32_t)(3) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ebx), 3 (8-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003CC1C4; /* je: equal / zero */

loc_003CC19A: ;
    PUSH32(esp, 0);
    PUSH32(esp, 0x003CC1A1u); RECOMP_ABI_CALL(0x003CA720u, sub_003CA720); /* call 0x003CA720 */

loc_003CC1A1: ;
    eax = MEM32(esi + 0x2478);
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    eax = eax & 1;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax = MEM32(esi + eax * 4 + 0x1974);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CC1BE; /* je: equal / zero */

loc_003CC1B6: ;
    PUSH32(esp, 1);
    PUSH32(esp, eax);
    PUSH32(esp, 0x003CC1BEu); RECOMP_ABI_CALL(0x003CAA10u, sub_003CAA10); /* call 0x003CAA10 */

loc_003CC1BE: ;
    MEM32(esi + 0x2478) = MEM32(esi + 0x2478) + 1;
    _fa = (uint32_t)(MEM32(esi + 0x2478)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_003CC1C4: ;
    PUSH32(esp, ebx);
    eax = esi;
    PUSH32(esp, 0x003CC1CCu); RECOMP_ABI_CALL(0x003CBC60u, sub_003CBC60); /* call 0x003CBC60 */

loc_003CC1CC: ;
    edi = ebx;
    edi = edi & 4;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    if ((_fa == 0)) goto loc_003CC210; /* je: equal / zero */

loc_003CC1D3: ;
    goto loc_003CC20B;

loc_003CC1D5: ;
    edi = ebx;
    edi = edi & 4;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    if ((_fa == 0)) goto loc_003CC210; /* je: equal / zero */

loc_003CC1DC: ;
    PUSH32(esp, 0);
    PUSH32(esp, 0x003CC1E3u); RECOMP_ABI_CALL(0x003CA720u, sub_003CA720); /* call 0x003CA720 */

loc_003CC1E3: ;
    ecx = MEM32(esi + 0x2478);
    ecx--;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    ecx = ecx & 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax = MEM32(esi + ecx * 4 + 0x1974);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CC200; /* je: equal / zero */

loc_003CC1F8: ;
    PUSH32(esp, 1);
    PUSH32(esp, eax);
    PUSH32(esp, 0x003CC200u); RECOMP_ABI_CALL(0x003CAA10u, sub_003CAA10); /* call 0x003CAA10 */

loc_003CC200: ;
    MEM32(esi + 0x2478) = MEM32(esi + 0x2478) + 1;
    _fa = (uint32_t)(MEM32(esi + 0x2478)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    PUSH32(esp, 0x003CC20Bu); RECOMP_ABI_CALL(0x003CBDE0u, sub_003CBDE0); /* call 0x003CBDE0 */

loc_003CC20B: ;
    PUSH32(esp, 0x003CC210u); RECOMP_ABI_CALL(0x003CC120u, sub_003CC120); /* call 0x003CC120 */

loc_003CC210: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CC23F; /* je: equal / zero */

loc_003CC214: ;
    ecx = MEM32(0x3D83D0);
    eax = MEM32(esi + 0x8D4);
    ecx = ecx | 0xFF1000;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    MEM32(0x3D83D0) = ecx;
    if (CMP_NE(_fa, _fb)) goto loc_003CC23A; /* jne: not equal / not zero */

loc_003CC230: ;
    eax = MEM32(esi + 0x8D8);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CC23F; /* je: equal / zero */

loc_003CC23A: ;
    PUSH32(esp, 0x003CC23Fu); RECOMP_ABI_CALL(0x003D26A0u, sub_003D26A0); /* call 0x003D26A0 */

loc_003CC23F: ;
    eax = MEM32(esi + 0x2478);
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebx);
    esp += 4; return; /* ret */

}

/**
 * sub_003CC310
 * Original: 0x003CC310 - 0x003CC3BA (170 bytes, 45 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003CC310(void)
{
    RECOMP_ENTRY(0x003CC310u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003CC310: ;
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    edi = ecx;
    ecx = MEM32(0x3D958C);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CC328; /* jne: not equal / not zero */

loc_003CC31E: ;
    MEM32(0x3D958C) = 0x80000;

loc_003CC328: ;
    ecx = MEM32(0x3D9588);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CC33C; /* jne: not equal / not zero */

loc_003CC332: ;
    MEM32(0x3D9588) = 0x8000;

loc_003CC33C: ;
    esi = MEM32(0x3D5F28);
    eax = eax & 0x10;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    esi = esi | eax;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    eax = MEM32(esp + 0xC);
    PUSH32(esp, eax);
    PUSH32(esp, 0x3D5F20);
    MEM32(0x3D83C8) = 0x3D5F20;
    MEM32(0x3D6858) = 1;
    MEM32(0x3D5F28) = esi;
    PUSH32(esp, 0x003CC370u); RECOMP_ABI_CALL(0x003CD040u, sub_003CD040); /* call 0x003CD040 */

loc_003CC370: ;
    esi = eax;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0 (32-bit) */
    if (CMP_GE(_fas, _fbs)) goto loc_003CC3A9; /* jge: greater or equal (signed >=) */

loc_003CC376: ;
    eax = 0x3D5F20;
    PUSH32(esp, 0x003CC380u); RECOMP_ABI_CALL(0x003CDC90u, sub_003CDC90); /* call 0x003CDC90 */

loc_003CC380: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CC38A; /* je: equal / zero */

loc_003CC384: ;
    MEM32(edi) = 0;

loc_003CC38A: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    ecx = 0x928;
    edi = 0x3D5F20;
    { uint32_t _i; int32_t _st = RECOMP_DF_STEP(4); for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*_st) = eax; edi += ecx * _st; }
    ecx = 0; /* rep stosd */
    POP32(esp, edi);
    eax = esi;
    MEM32(0x3D83C8) = 0;
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

loc_003CC3A9: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CC3B3; /* je: equal / zero */

loc_003CC3AD: ;
    MEM32(edi) = 0x3D5F20;

loc_003CC3B3: ;
    POP32(esp, edi);
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003CDE80
 * Original: 0x003CDE80 - 0x003CE087 (519 bytes, 159 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: standard_frame
 */
void d3d_orig_003CDE80(void)
{
    RECOMP_ENTRY(0x003CDE80u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003CDE80: ;
    PUSH32(esp, ebp);
    ebp = MEM32(esp + 8);
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, edi);
    { uint32_t _icall_target = MEM32(0x3E15C4); PUSH32(esp, 0x003CDE8Cu); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x003CDE86u); } /* indirect call */
    }

loc_003CDE8C: ;
    edi = 0; /* xor self */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CDEA4; /* jne: not equal / not zero */

loc_003CDE92: ;
    eax = MEM32(ebp + 0x1C28);
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, edi);
    PUSH32(esp, 1);
    PUSH32(esp, 9);
    PUSH32(esp, eax);
    { uint32_t _icall_target = MEM32(0x3E15C8); PUSH32(esp, 0x003CDEA4u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x003CDE9Eu); } /* indirect call */
    }

loc_003CDEA4: ;
    eax = MEM32(ebp + 0x1A04);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edi (32-bit) */
    PUSH32(esp, ebx);
    PUSH32(esp, esi);
    if (CMP_EQ(_fa, _fb)) goto loc_003CDED0; /* je: equal / zero */

loc_003CDEB0: ;
    ecx = MEM32(eax + 0x14);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, edi (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CDEB9; /* je: equal / zero */

loc_003CDEB7: ;
    eax = ecx;

loc_003CDEB9: ;
    ecx = MEM32(ebp + 0x2C);
    MEM32(eax + 8) = ecx;
    esi = MEM32(ebp + 0x1A04);
    PUSH32(esp, 0x003CDECAu); RECOMP_ABI_CALL(0x003C9F70u, sub_003C9F70); /* call 0x003C9F70 */

loc_003CDECA: ;
    MEM32(ebp + 0x1A04) = edi;

loc_003CDED0: ;
    eax = MEM32(ebp + 0x1A08);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edi (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CDEFA; /* je: equal / zero */

loc_003CDEDA: ;
    ecx = MEM32(eax + 0x14);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, edi (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CDEE3; /* je: equal / zero */

loc_003CDEE1: ;
    eax = ecx;

loc_003CDEE3: ;
    edx = MEM32(ebp + 0x2C);
    MEM32(eax + 8) = edx;
    esi = MEM32(ebp + 0x1A08);
    PUSH32(esp, 0x003CDEF4u); RECOMP_ABI_CALL(0x003C9F70u, sub_003C9F70); /* call 0x003C9F70 */

loc_003CDEF4: ;
    MEM32(ebp + 0x1A08) = edi;

loc_003CDEFA: ;
    _fa = (uint32_t)(MEM32(ebp + 0x1A10)) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(ebp + 0x1A10), edi (32-bit) */
    MEM32(esp + 0x14) = edi;
    if (CMP_BE(_fa, _fb)) goto loc_003CDF6D; /* jbe: below or equal (unsigned <=) */

loc_003CDF06: ;
    ebx = ebp + 0x1A14;
    /* nop */

loc_003CDF10: ;
    esi = MEM32(ebx);
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, edi (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CDF57; /* je: equal / zero */

loc_003CDF16: ;
    eax = MEM32(esi);
    ecx = eax;
    ecx = ecx & 0xFFFF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 1 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CDF52; /* jne: not equal / not zero */

loc_003CDF25: ;
    eax = eax & 0x70000;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x50000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x50000 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CDF3E; /* jne: not equal / not zero */

loc_003CDF31: ;
    eax = MEM32(esi + 0x14);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edi (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CDF3E; /* je: equal / zero */

loc_003CDF38: ;
    PUSH32(esp, eax);
    PUSH32(esp, 0x003CDF3Eu); RECOMP_ABI_CALL(0x003CA010u, sub_003CA010); /* call 0x003CA010 */

loc_003CDF3E: ;
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x780000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test eax, 0x780000 (32-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003CDF52; /* jne: not equal / not zero */

loc_003CDF47: ;
    edi = esi;
    PUSH32(esp, 0x003CDF4Eu); RECOMP_ABI_CALL(0x003C9EC0u, sub_003C9EC0); /* call 0x003C9EC0 */

loc_003CDF4E: ;
    edi = 0; /* xor self */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    goto loc_003CDF55;

loc_003CDF52: ;
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    MEM32(esi) = eax;

loc_003CDF55: ;
    MEM32(ebx) = edi;

loc_003CDF57: ;
    eax = MEM32(esp + 0x14);
    ecx = MEM32(ebp + 0x1A10);
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ebx = ebx + 4;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ecx (32-bit) */
    MEM32(esp + 0x14) = eax;
    if (CMP_B(_fa, _fb)) goto loc_003CDF10; /* jb: below (unsigned <) */

loc_003CDF6D: ;
    esi = MEM32(ebp + 0x1A20);
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, edi (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CDFBC; /* je: equal / zero */

loc_003CDF77: ;
    eax = MEM32(esi);
    edx = eax;
    edx = edx & 0xFFFF;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 1 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CDFB3; /* jne: not equal / not zero */

loc_003CDF86: ;
    eax = eax & 0x70000;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x50000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x50000 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CDF9F; /* jne: not equal / not zero */

loc_003CDF92: ;
    eax = MEM32(esi + 0x14);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edi (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CDF9F; /* je: equal / zero */

loc_003CDF99: ;
    PUSH32(esp, eax);
    PUSH32(esp, 0x003CDF9Fu); RECOMP_ABI_CALL(0x003CA010u, sub_003CA010); /* call 0x003CA010 */

loc_003CDF9F: ;
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x780000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test eax, 0x780000 (32-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003CDFB3; /* jne: not equal / not zero */

loc_003CDFA8: ;
    edi = esi;
    PUSH32(esp, 0x003CDFAFu); RECOMP_ABI_CALL(0x003C9EC0u, sub_003C9EC0); /* call 0x003C9EC0 */

loc_003CDFAF: ;
    edi = 0; /* xor self */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    goto loc_003CDFB6;

loc_003CDFB3: ;
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    MEM32(esi) = eax;

loc_003CDFB6: ;
    MEM32(ebp + 0x1A20) = edi;

loc_003CDFBC: ;
    _fa = (uint32_t)(MEM32(ebp + 0x1A8C)) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(ebp + 0x1A8C), edi (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CE012; /* je: equal / zero */

loc_003CDFC4: ;
    esi = MEM32(0x3D83C8);
    eax = MEM32(esi + 0x1AEC);
    PUSH32(esp, eax);
    ecx = esi + 0x1C28;
    PUSH32(esp, ecx);
    ebx = 1;
    PUSH32(esp, 0x003CDFE2u); RECOMP_ABI_CALL(0x003CF72Au, sub_003CF72A); /* call 0x003CF72A */

loc_003CDFE2: ;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fb = (uint32_t)(0x1AD8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + 0x1AD8;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi) = edx;
    MEM32(esi + 4) = edx;
    MEM32(esi + 8) = edx;
    MEM32(esi + 0xC) = edx;
    MEM32(esi + 0x10) = edx;
    MEM32(esi + 0x14) = edx;
    eax = MEM32(ebp + 0x1A8C);
    PUSH32(esp, 0xA1800000u);
    PUSH32(esp, eax);
    PUSH32(esp, 0x003CE00Cu); RECOMP_ABI_CALL(0x0032BDDEu, sub_0032BDDE); /* call 0x0032BDDE */

loc_003CE00C: ;
    MEM32(ebp + 0x1A8C) = edi;

loc_003CE012: ;
    _fa = (uint32_t)(MEM32(ebp + 0x1A84)) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(ebp + 0x1A84), edi (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CE065; /* je: equal / zero */

loc_003CE01A: ;
    esi = MEM32(0x3D83C8);
    ecx = MEM32(esi + 0x1AD4);
    PUSH32(esp, ecx);
    edx = esi + 0x1C28;
    PUSH32(esp, edx);
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    PUSH32(esp, 0x003CE035u); RECOMP_ABI_CALL(0x003CF72Au, sub_003CF72A); /* call 0x003CF72A */

loc_003CE035: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fb = (uint32_t)(0x1AC0) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + 0x1AC0;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi) = eax;
    MEM32(esi + 4) = eax;
    MEM32(esi + 8) = eax;
    MEM32(esi + 0xC) = eax;
    MEM32(esi + 0x10) = eax;
    MEM32(esi + 0x14) = eax;
    ecx = MEM32(ebp + 0x1A84);
    PUSH32(esp, 0xA1800000u);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003CE05Fu); RECOMP_ABI_CALL(0x0032BDDEu, sub_0032BDDE); /* call 0x0032BDDE */

loc_003CE05F: ;
    MEM32(ebp + 0x1A84) = edi;

loc_003CE065: ;
    eax = MEM32(ebp + 0x1A88);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edi (32-bit) */
    POP32(esp, esi);
    POP32(esp, ebx);
    if (CMP_EQ(_fa, _fb)) goto loc_003CE082; /* je: equal / zero */

loc_003CE071: ;
    PUSH32(esp, 0xA1800000u);
    PUSH32(esp, eax);
    PUSH32(esp, 0x003CE07Cu); RECOMP_ABI_CALL(0x0032BDDEu, sub_0032BDDE); /* call 0x0032BDDE */

loc_003CE07C: ;
    MEM32(ebp + 0x1A88) = edi;

loc_003CE082: ;
    POP32(esp, edi);
    POP32(esp, ebp);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_003CE090
 * Original: 0x003CE090 - 0x003CE655 (1477 bytes, 437 insns)
 * Category: game_render
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003CE090(void)
{
    RECOMP_ENTRY(0x003CE090u);
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

loc_003CE090: ;
    _fb = (uint32_t)(0x4C) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x4C;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    ebp = MEM32(esp + 0x58);
    eax = MEM32(ebp + 0xC);
    PUSH32(esp, esi);
    esi = 1;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, esi (32-bit) */
    MEM32(esp + 0x14) = esi;
    if (CMP_B(_fa, _fb)) goto loc_003CE0AE; /* jb: below (unsigned <) */

loc_003CE0AA: ;
    MEM32(esp + 0x14) = eax;

loc_003CE0AE: ;
    eax = MEM32(ebp + 8);
    PUSH32(esp, 0x003CE0B6u); RECOMP_ABI_CALL(0x003CE670u, sub_003CE670); /* call 0x003CE670 */

loc_003CE0B6: ;
    MEM32(esp + 0x2C) = eax;
    eax = MEM32(ebp + 0x24);
    PUSH32(esp, 0x003CE0C2u); RECOMP_ABI_CALL(0x003CE670u, sub_003CE670); /* call 0x003CE670 */

loc_003CE0C2: ;
    ecx = MEM32(ebp + 4);
    edx = MEM32(ebp + 0x30);
    MEM32(esp + 0x24) = eax;
    eax = MEM32(ebp);
    MEM32(0x3D85D4) = edx;
    edx = MEM32(edi + 8);
    MEM32(esp + 0x34) = eax;
    eax = MEM32(ebp + 0x14);
    MEM32(edi + 0x1970) = eax;
    eax = MEM32(esp + 0x14);
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    edx = edx & 0xFFFFBFFFu;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(edi + 0x1A10) = eax;
    MEM32(esp + 0x30) = ecx;
    MEM32(edi + 8) = edx;
    ecx = MEM32(ebp + 0x10);
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(HI8(ecx)) & 0xFFu; _fb = (uint32_t)(0x30) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(ecx), 0x30 (8-bit) */
    MEM32(esp + 0x18) = eax;
    eax = edx;
    MEM32(esp + 0x1C) = ebx;
    if (TEST_NZ(_fa, _fb)) goto loc_003CE117; /* jne: not equal / not zero */

loc_003CE111: ;
    _fa = (uint32_t)(MEM32(ebp + 0x14)) & 0xFFFFFFFFu; _fb = (uint32_t)(3) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(ebp + 0x14), 3 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CE123; /* jne: not equal / not zero */

loc_003CE117: ;
    eax = eax | 0x4000;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(esp + 0x1C) = esi;
    MEM32(edi + 8) = eax;

loc_003CE123: ;
    edx = MEM32(ebp + 0x10);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, ebx (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CE12F; /* jne: not equal / not zero */

loc_003CE12A: ;
    edx = 0x11;

loc_003CE12F: ;
    eax = edx;
    eax = eax >> 4;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    eax = eax & 0xF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ecx = edx;
    ecx = ecx & 0xF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    MEM32(esp + 0x3C) = eax;
    fp_push((double)SMEM32(esp + 0x3C)); /* fild */
    MEM32(edi + 0x196C) = edx;
    if (CMP_GE(_fas, _fbs)) goto loc_003CE154; /* jge: greater or equal (signed >=) */

loc_003CE14E: ;
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148E4)); /* fadd dword ptr [0x4148e4] */

loc_003CE154: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    MEMF(edi + 0x96C) = (float)fp_top(); fp_pop(); /* fstp */
    MEM32(esp + 0x3C) = ecx;
    fp_push((double)SMEM32(esp + 0x3C)); /* fild */
    if (CMP_GE(_fas, _fbs)) goto loc_003CE16C; /* jge: greater or equal (signed >=) */

loc_003CE166: ;
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148E4)); /* fadd dword ptr [0x4148e4] */

loc_003CE16C: ;
    _fa = (uint32_t)(HI8(edx)) & 0xFFu; _fb = (uint32_t)(0x10) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(edx), 0x10 (8-bit) */
    MEMF(edi + 0x970) = (float)fp_top(); fp_pop(); /* fstp */
    if (TEST_Z(_fa, _fb)) goto loc_003CE183; /* je: equal / zero */

loc_003CE177: ;
    _fa = (uint32_t)(LO8(edx)) & 0xFFu; _fb = (uint32_t)(0x21) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(edx), 0x21 (8-bit) */
    ebx = 2;
    if (CMP_NE(_fa, _fb)) goto loc_003CE183; /* jne: not equal / not zero */

loc_003CE181: ;
    ebx = esi;

loc_003CE183: ;
    _fa = (uint32_t)(HI8(edx)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(edx), 1 (8-bit) */
    esi = 2;
    if (TEST_Z(_fa, _fb)) goto loc_003CE194; /* je: equal / zero */

loc_003CE18D: ;
    esi = 4;
    goto loc_003CE19E;

loc_003CE194: ;
    _fa = (uint32_t)(HI8(edx)) & 0xFFu; _fb = (uint32_t)(2) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(edx), 2 (8-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003CE19E; /* je: equal / zero */

loc_003CE199: ;
    esi = 5;

loc_003CE19E: ;
    MEM32(0x3D8640) = ebx;
    ebx = ebp + 0x34;
    MEM32(0x3D85D0) = esi;
    _fa = (uint32_t)(MEM32(ebx)) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(ebx), 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CE251; /* je: equal / zero */

loc_003CE1B6: ;
    esi = MEM32(esp + 0x18);
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 0 (32-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_003CE210; /* jbe: below or equal (unsigned <=) */

loc_003CE1BE: ;
    edx = edi + 0x1A14;
    eax = edi + 0x1A24;
    ecx = ebx;
    MEM32(esp + 0x24) = esi;

loc_003CE1D0: ;
    esi = MEM32(ecx);
    ebp = MEM32(esi);
    ebx = eax;
    MEM32(ebx) = ebp;
    ebp = MEM32(esi + 4);
    MEM32(ebx + 4) = ebp;
    ebp = MEM32(esi + 8);
    MEM32(ebx + 8) = ebp;
    ebp = MEM32(esi + 0xC);
    MEM32(ebx + 0xC) = ebp;
    ebp = MEM32(esi + 0x10);
    MEM32(ebx + 0x10) = ebp;
    esi = MEM32(esi + 0x14);
    MEM32(ebx + 0x14) = esi;
    esi = MEM32(esp + 0x24);
    MEM32(edx) = eax;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edx = edx + 4;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(0x18) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x18;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esi--;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    MEM32(esp + 0x24) = esi;
    if ((_fa != 0)) goto loc_003CE1D0; /* jne: not equal / not zero */

loc_003CE20C: ;
    ebp = MEM32(esp + 0x5C);

loc_003CE210: ;
    ebp = MEM32(ebp + 0x40);
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CE59B; /* je: equal / zero */

loc_003CE21B: ;
    edx = MEM32(ebp);
    eax = edi + 0x1A6C;
    ecx = eax;
    MEM32(ecx) = edx;
    edx = MEM32(ebp + 4);
    MEM32(ecx + 4) = edx;
    edx = MEM32(ebp + 8);
    MEM32(ecx + 8) = edx;
    edx = MEM32(ebp + 0xC);
    MEM32(ecx + 0xC) = edx;
    edx = MEM32(ebp + 0x10);
    MEM32(ecx + 0x10) = edx;
    edx = MEM32(ebp + 0x14);
    MEM32(ecx + 0x14) = edx;
    MEM32(edi + 0x1A20) = eax;
    goto loc_003CE59B;

loc_003CE251: ;
    ebx = MEM32(ebp + 0x10);
    edx = MEM32(esp + 0x2C);
    ebx = ebx & 0xF0000;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x30000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, 0x30000 (32-bit) */
    if (CMP_A(_fa, _fb)) goto loc_003CE28D; /* ja: above (unsigned >) */

loc_003CE266: ;
    if (CMP_EQ(_fa, _fb)) goto loc_003CE286; /* je: equal / zero */

loc_003CE268: ;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x10000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, 0x10000 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CE27F; /* je: equal / zero */

loc_003CE270: ;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x20000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, 0x20000 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CE29A; /* jne: not equal / not zero */

loc_003CE278: ;
    edx = 0x11;
    goto loc_003CE29A;

loc_003CE27F: ;
    edx = 0x1C;
    goto loc_003CE29A;

loc_003CE286: ;
    edx = 0x1E;
    goto loc_003CE29A;

loc_003CE28D: ;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x40000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, 0x40000 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CE29A; /* jne: not equal / not zero */

loc_003CE295: ;
    edx = 0x12;

loc_003CE29A: ;
    eax = (uint32_t)((int32_t)eax * (int32_t)MEM32(esp + 0x34));
    ecx = (uint32_t)((int32_t)ecx * (int32_t)MEM32(esp + 0x30));
    ebx = eax;
    PUSH32(esp, ebx);
    eax = edx;
    MEM32(esp + 0x24) = ebx;
    MEM32(esp + 0x3C) = ecx;
    PUSH32(esp, 0x003CE2B6u); RECOMP_ABI_CALL(0x003CE710u, sub_003CE710); /* call 0x003CE710 */

loc_003CE2B6: ;
    esi = eax;
    eax = esp + 0x10;
    PUSH32(esp, eax);
    ecx = esp + 0x10;
    PUSH32(esp, ecx);
    PUSH32(esp, 0);
    PUSH32(esp, 0);
    PUSH32(esp, esi);
    PUSH32(esp, edx);
    edx = MEM32(esp + 0x50);
    PUSH32(esp, 1);
    PUSH32(esp, 1);
    eax = ebx;
    PUSH32(esp, 0x003CE2D7u); RECOMP_ABI_CALL(0x003CECF0u, sub_003CECF0); /* call 0x003CECF0 */

loc_003CE2D7: ;
    ecx = MEM32(esp + 0x1C);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    MEM32(esp + 0x3C) = eax;
    if (CMP_EQ(_fa, _fb)) goto loc_003CE2ED; /* je: equal / zero */

loc_003CE2E3: ;
    MEM32(esp + 0x18) = 1;
    goto loc_003CE2F5;

loc_003CE2ED: ;
    edx = MEM32(esp + 0x18);
    MEM32(esp + 0x18) = edx;

loc_003CE2F5: ;
    ecx = MEM32(esp + 0x18);
    ecx = (uint32_t)((int32_t)ecx * (int32_t)eax);
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    eax = ecx + esi + -1;
    { uint64_t _dividend = ((uint64_t)edx << 32) | eax;
      eax = (uint32_t)(_dividend / (uint32_t)esi);
      edx = (uint32_t)(_dividend % (uint32_t)esi); }
    ebx = eax;
    ebx = (uint32_t)((int32_t)ebx * (int32_t)esi);
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x3FFF) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test ebx, 0x3FFF (32-bit) */
    MEM32(esp + 0x28) = ebx;
    if (TEST_Z(_fa, _fb)) goto loc_003CE323; /* je: equal / zero */

loc_003CE315: ;
    _fb = (uint32_t)(esi) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ebx = ebx + esi;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x3FFF) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test ebx, 0x3FFF (32-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003CE315; /* jne: not equal / not zero */

loc_003CE31F: ;
    MEM32(esp + 0x28) = ebx;

loc_003CE323: ;
    PUSH32(esp, 0xBE800000u);
    PUSH32(esp, ebx);
    PUSH32(esp, 0x003CE32Eu); RECOMP_ABI_CALL(0x0032BD3Eu, sub_0032BD3E); /* call 0x0032BD3E */

loc_003CE32E: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CE4EF; /* je: equal / zero */

loc_003CE336: ;
    edx = MEM32(esp + 0x18);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0 (32-bit) */
    MEM32(edi + 0x1A84) = eax;
    if (CMP_BE(_fa, _fb)) goto loc_003CE38E; /* jbe: below or equal (unsigned <=) */

loc_003CE344: ;
    esi = edi + 0x1A14;
    ecx = edi + 0x1A30;
    ebx = edx;

loc_003CE352: ;
    edx = ecx + -12;
    MEM32(esi) = edx;
    MEM32(edx) = 0x1050001;
    edx = eax;
    edx = edx & 0xFFFFFFF;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(ecx + -8) = edx;
    edx = MEM32(esp + 0xC);
    MEM32(ecx) = edx;
    edx = MEM32(esp + 0x10);
    MEM32(ecx + 4) = edx;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    MEM32(ecx + 8) = edx;
    MEM32(ecx + -4) = edx;
    _fb = (uint32_t)(MEM32(esp + 0x3C)) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + MEM32(esp + 0x3C);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + 4;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(0x18) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 0x18;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ebx--;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa != 0)) goto loc_003CE352; /* jne: not equal / not zero */

loc_003CE38A: ;
    ebx = MEM32(esp + 0x28);

loc_003CE38E: ;
    _fa = (uint32_t)(MEM32(esp + 0x20)) & 0xFFFFFFFFu; _fb = (uint32_t)(0x780) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(esp + 0x20), 0x780 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CE3DA; /* je: equal / zero */

loc_003CE398: ;
    ecx = MEM32(edi + 0x1A84);
    esi = MEM32(edi + 0x1A14);
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    MEM32(esp + 0x40) = eax;
    MEM32(esp + 0x44) = eax;
    MEM32(esp + 0x48) = eax;
    MEM32(esp + 0x4C) = eax;
    MEM32(esp + 0x50) = eax;
    MEM32(esp + 0x54) = eax;
    MEM32(esp + 0x44) = ecx;
    MEM32(esp + 0x48) = ebx;
    PUSH32(esp, 0x003CE3CBu); RECOMP_ABI_CALL(0x003C68F0u, sub_003C68F0); /* call 0x003C68F0 */

loc_003CE3CB: ;
    MEM32(esp + 0x4C) = eax;
    ecx = esp + 0x40;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    PUSH32(esp, 0x003CE3DAu); RECOMP_ABI_CALL(0x003C7CE0u, sub_003C7CE0); /* call 0x003C7CE0 */

loc_003CE3DA: ;
    eax = MEM32(esp + 0x1C);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CE47D; /* je: equal / zero */

loc_003CE3E6: ;
    ecx = MEM32(esp + 0x2C);
    edx = esp + 0x10;
    PUSH32(esp, edx);
    edx = MEM32(esp + 0x34);
    eax = esp + 0x10;
    PUSH32(esp, eax);
    eax = MEM32(esp + 0x3C);
    PUSH32(esp, 0);
    PUSH32(esp, 0);
    PUSH32(esp, 0);
    PUSH32(esp, ecx);
    PUSH32(esp, 1);
    PUSH32(esp, 1);
    PUSH32(esp, 0x003CE40Cu); RECOMP_ABI_CALL(0x003CECF0u, sub_003CECF0); /* call 0x003CECF0 */

loc_003CE40C: ;
    esi = MEM32(esp + 0x14);
    ebx = eax;
    eax = (uint32_t)((int32_t)eax * (int32_t)esi);
    PUSH32(esp, 0xBE800000u);
    PUSH32(esp, eax);
    PUSH32(esp, 0x003CE420u); RECOMP_ABI_CALL(0x0032BD3Eu, sub_0032BD3E); /* call 0x0032BD3E */

loc_003CE420: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CE4EF; /* je: equal / zero */

loc_003CE428: ;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 1 (32-bit) */
    MEM32(edi + 0x1A88) = eax;
    if (CMP_B(_fa, _fb)) goto loc_003CE47D; /* jb: below (unsigned <) */

loc_003CE433: ;
    ebp = MEM32(esp + 0x14);
    esi = edi + 0x1A18;
    ecx = edi + 0x1A48;

loc_003CE443: ;
    edx = ecx + -12;
    MEM32(esi) = edx;
    MEM32(edx) = 0x1050001;
    edx = eax;
    edx = edx & 0xFFFFFFF;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(ecx + -8) = edx;
    edx = MEM32(esp + 0xC);
    MEM32(ecx) = edx;
    edx = MEM32(esp + 0x10);
    MEM32(ecx + 4) = edx;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    MEM32(ecx + 8) = edx;
    MEM32(ecx + -4) = edx;
    _fb = (uint32_t)(ebx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + ebx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + 4;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(0x18) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 0x18;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ebp--;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa != 0)) goto loc_003CE443; /* jne: not equal / not zero */

loc_003CE479: ;
    ebp = MEM32(esp + 0x5C);

loc_003CE47D: ;
    eax = MEM32(ebp + 0x20);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CE59B; /* je: equal / zero */

loc_003CE488: ;
    eax = MEM32(esp + 0x20);
    PUSH32(esp, eax);
    eax = MEM32(esp + 0x28);
    PUSH32(esp, 0x003CE496u); RECOMP_ABI_CALL(0x003CE710u, sub_003CE710); /* call 0x003CE710 */

loc_003CE496: ;
    ebp = MEM32(esp + 0x20);
    ecx = esp + 0x10;
    PUSH32(esp, ecx);
    edx = esp + 0x10;
    PUSH32(esp, edx);
    edx = MEM32(esp + 0x40);
    PUSH32(esp, 0);
    PUSH32(esp, 0);
    ebx = eax;
    eax = MEM32(esp + 0x34);
    PUSH32(esp, ebx);
    PUSH32(esp, eax);
    PUSH32(esp, 1);
    PUSH32(esp, 1);
    eax = ebp;
    PUSH32(esp, 0x003CE4BFu); RECOMP_ABI_CALL(0x003CECF0u, sub_003CECF0); /* call 0x003CECF0 */

loc_003CE4BF: ;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    eax = eax + ebx + -1;
    { uint64_t _dividend = ((uint64_t)edx << 32) | eax;
      eax = (uint32_t)(_dividend / (uint32_t)ebx);
      edx = (uint32_t)(_dividend % (uint32_t)ebx); }
    esi = eax;
    esi = (uint32_t)((int32_t)esi * (int32_t)ebx);
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0x3FFF) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test esi, 0x3FFF (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003CE4DE; /* je: equal / zero */

loc_003CE4D4: ;
    _fb = (uint32_t)(ebx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + ebx;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(0x3FFF) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test esi, 0x3FFF (32-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003CE4D4; /* jne: not equal / not zero */

loc_003CE4DE: ;
    PUSH32(esp, 0xBE800000u);
    PUSH32(esp, esi);
    PUSH32(esp, 0x003CE4E9u); RECOMP_ABI_CALL(0x0032BD3Eu, sub_0032BD3E); /* call 0x0032BD3E */

loc_003CE4E9: ;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edx (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CE4FD; /* jne: not equal / not zero */

loc_003CE4EF: ;
    POP32(esp, esi);
    POP32(esp, ebp);
    eax = 0x8007000Eu;
    POP32(esp, ebx);
    _fb = (uint32_t)(0x4C) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x4C;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 8; return; /* ret 4 */

loc_003CE4FD: ;
    ecx = edi + 0x1A6C;
    MEM32(edi + 0x1A8C) = eax;
    MEM32(edi + 0x1A20) = ecx;
    ebx = eax;
    ebx = ebx & 0xFFFFFFF;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(0x780) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, 0x780 (32-bit) */
    MEM32(ecx + 4) = ebx;
    ebx = MEM32(esp + 0xC);
    MEM32(ecx + 0xC) = ebx;
    ebx = MEM32(esp + 0x10);
    MEM32(ecx) = 0x1050001;
    MEM32(ecx + 0x10) = ebx;
    MEM32(ecx + 0x14) = edx;
    MEM32(ecx + 8) = edx;
    if (CMP_EQ(_fa, _fb)) goto loc_003CE59B; /* je: equal / zero */

loc_003CE53C: ;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    MEM32(esp + 0x40) = ecx;
    MEM32(esp + 0x44) = ecx;
    MEM32(esp + 0x48) = ecx;
    MEM32(esp + 0x4C) = ecx;
    MEM32(esp + 0x50) = ecx;
    MEM32(esp + 0x48) = esi;
    esi = MEM32(edi + 0x1A20);
    MEM32(esp + 0x54) = ecx;
    MEM32(esp + 0x40) = 0x80000001u;
    MEM32(esp + 0x44) = eax;
    PUSH32(esp, 0x003CE571u); RECOMP_ABI_CALL(0x003C68F0u, sub_003C68F0); /* call 0x003C68F0 */

loc_003CE571: ;
    edx = MEM32(esp + 0x24);
    MEM32(esp + 0x4C) = eax;
    SET_LO8(eax, MEM8(edx + 0x3D4B68));
    SET_LO8(eax, LO8(eax) & 0x3C);
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* and result */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x20) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0x20 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CE58D; /* jne: not equal / not zero */

loc_003CE585: ;
    MEM32(esp + 0x40) = 0x84000001u;

loc_003CE58D: ;
    ecx = esp + 0x40;
    eax = 1;
    PUSH32(esp, 0x003CE59Bu); RECOMP_ABI_CALL(0x003C7CE0u, sub_003C7CE0); /* call 0x003C7CE0 */

loc_003CE59B: ;
    eax = MEM32(edi + 0x1A10);
    esi = 0; /* xor self */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_003CE5E4; /* jbe: below or equal (unsigned <=) */

loc_003CE5A7: ;
    ebx = edi + 0x1A14;
    /* nop */

loc_003CE5B0: ;
    ecx = MEM32(ebx);
    eax = MEM32(ecx);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFF) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test eax, 0xFFFF (32-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003CE5D4; /* jne: not equal / not zero */

loc_003CE5BB: ;
    eax = eax & 0x70000;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x50000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x50000 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CE5D4; /* jne: not equal / not zero */

loc_003CE5C7: ;
    eax = MEM32(ecx + 0x14);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CE5D4; /* je: equal / zero */

loc_003CE5CE: ;
    PUSH32(esp, eax);
    PUSH32(esp, 0x003CE5D4u); RECOMP_ABI_CALL(0x003C9FD0u, sub_003C9FD0); /* call 0x003C9FD0 */

loc_003CE5D4: ;
    MEM32(ecx) = MEM32(ecx) + 1;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    eax = MEM32(edi + 0x1A10);
    esi++;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ebx = ebx + 4;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, eax (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003CE5B0; /* jb: below (unsigned <) */

loc_003CE5E4: ;
    ecx = MEM32(edi + 0x1A20);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CE612; /* je: equal / zero */

loc_003CE5EE: ;
    eax = MEM32(ecx);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFF) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test eax, 0xFFFF (32-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003CE610; /* jne: not equal / not zero */

loc_003CE5F7: ;
    eax = eax & 0x70000;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x50000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x50000 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CE610; /* jne: not equal / not zero */

loc_003CE603: ;
    eax = MEM32(ecx + 0x14);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003CE610; /* je: equal / zero */

loc_003CE60A: ;
    PUSH32(esp, eax);
    PUSH32(esp, 0x003CE610u); RECOMP_ABI_CALL(0x003C9FD0u, sub_003C9FD0); /* call 0x003C9FD0 */

loc_003CE610: ;
    MEM32(ecx) = MEM32(ecx) + 1;
    _fa = (uint32_t)(MEM32(ecx)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_003CE612: ;
    eax = MEM32(edi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(edi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(edi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003CE628; /* jb: below (unsigned <) */

loc_003CE619: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003CE628u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003CE628: ;
    ecx = MEM32(edi + 0x1A10);
    MEM32(eax) = 0xC0120;
    MEM32(eax + 4) = 0;
    MEM32(eax + 8) = 1;
    MEM32(eax + 0xC) = ecx;
    POP32(esp, esi);
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x10;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    POP32(esp, ebp);
    MEM32(edi) = eax;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    POP32(esp, ebx);
    _fb = (uint32_t)(0x4C) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x4C;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 8; return; /* ret 4 */

    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}

/**
 * sub_003CEA10
 * Original: 0x003CEA10 - 0x003CEAAF (159 bytes, 48 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003CEA10(void)
{
    RECOMP_ENTRY(0x003CEA10u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003CEA10: ;
    eax = ZX8(MEM8(edi + 0xD));
    _fb = (uint32_t)(0x14) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x14;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    ecx = edi;
    MEM32(esi) = eax;
    PUSH32(esp, 0x003CEA20u); RECOMP_ABI_CALL(0x003CA060u, sub_003CA060); /* call 0x003CA060 */

loc_003CEA20: ;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, 0 (32-bit) */
    MEM32(esi + 4) = eax;
    MEM32(esi + 8) = 0;
    if (CMP_NE(_fa, _fb)) goto loc_003CEA50; /* jne: not equal / not zero */

loc_003CEA2E: ;
    ecx = ZX8(MEM8(edi + 0xD));
    SET_LO8(eax, MEM8(ecx + 0x3D4B68));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if ((((uint8_t)((uint8_t)(_fa) - (uint8_t)(_fb)) >> 7) == 0)) goto loc_003CEA45; /* jns: not sign (positive) */

loc_003CEA3C: ;
    MEM32(esi + 8) = 1;
    goto loc_003CEA50;

loc_003CEA45: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x40) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(eax), 0x40 (8-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003CEA50; /* je: equal / zero */

loc_003CEA49: ;
    MEM32(esi + 8) = 2;

loc_003CEA50: ;
    eax = MEM32(0x3D83C8);
    edx = MEM32(eax + 0x1A14);
    ecx = MEM32(edi + 4);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(edx + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, MEM32(edx + 4) (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003CEA6E; /* jne: not equal / not zero */

loc_003CEA63: ;
    edx = MEM32(eax + 0x196C);
    MEM32(esi + 0x10) = edx;
    goto loc_003CEA75;

loc_003CEA6E: ;
    MEM32(esi + 0x10) = 0x11;

loc_003CEA75: ;
    eax = esp + 8;
    PUSH32(esp, eax);
    ecx = esp + 0x10;
    PUSH32(esp, ecx);
    edx = esp + 0x18;
    PUSH32(esp, edx);
    eax = esp + 0x10;
    PUSH32(esp, eax);
    ecx = esp + 0x10;
    PUSH32(esp, ecx);
    eax = ebx;
    ecx = edi;
    PUSH32(esp, 0x003CEA97u); RECOMP_ABI_CALL(0x003CE8B0u, sub_003CE8B0); /* call 0x003CE8B0 */

loc_003CEA97: ;
    edx = MEM32(esp);
    eax = MEM32(esp + 4);
    ecx = MEM32(esp + 8);
    MEM32(esi + 0x14) = edx;
    MEM32(esi + 0x18) = eax;
    MEM32(esi + 0xC) = ecx;
    _fb = (uint32_t)(0x14) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x14;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 4; return; /* ret */

}

/**
 * sub_003CEEF5
 * Original: 0x003CEEF5 - 0x003CF055 (352 bytes, 114 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: standard_frame
 */
void d3d_orig_003CEEF5(void)
{
    RECOMP_ENTRY(0x003CEEF5u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* prologue saves caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    int _cf = 0; /* carry flag */

loc_003CEEF5: ;
    PUSH32(esp, ebp);
    ebp = esp;
    g_ebp = ebp; /* publish frame for frameless callees */
    g_seh_ebp = ebp;
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(esp) < (uint32_t)(0x10));
    esp = esp - 0x10;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, ebx);
    PUSH32(esp, edi);
    edi = eax;
    PUSH32(esp, edi);
    PUSH32(esp, 0x3CFAA0);
    eax = edi + 0x84;
    PUSH32(esp, eax);
    { uint32_t _icall_target = MEM32(0x3E14D8); PUSH32(esp, 0x003CEF12u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x003CEF0Cu); } /* indirect call */
    }

loc_003CEF12: ;
    eax = edi + 0x1AC;
    MEM32(edi + 0x1B0) = eax;
    MEM32(eax) = eax;
    eax = edi + 0x19C;
    _cf = 0; /* xor clears CF */
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    ebx++;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(edi + 0x1A0) = eax;
    MEM32(eax) = eax;
    eax = edi;
    MEM8(edi + 0x1A4) = 0;
    MEM8(edi + 0x1A6) = 4;
    MEM32(edi + 0x1A8) = ebx;
    MEM8(edi + 0x194) = 0;
    MEM8(edi + 0x196) = 4;
    MEM32(edi + 0x198) = ebx;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x003CEF60u); RECOMP_ABI_CALL(0x003CF055u, sub_003CF055); /* call 0x003CF055 */

loc_003CEF60: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_NE(_fa, _fb)) goto loc_003CEF6B; /* jne: not equal / not zero */

loc_003CEF64: ;
    _cf = 0; /* xor clears CF */
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    goto loc_003CF051;

loc_003CEF6B: ;
    eax = edi;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x003CEF72u); RECOMP_ABI_CALL(0x003CF080u, sub_003CF080); /* call 0x003CF080 */

loc_003CEF72: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_003CEF64; /* je: equal / zero */

loc_003CEF76: ;
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, esi);
    eax = ebp + -16;
    PUSH32(esp, eax);
    PUSH32(esp, 3);
    { uint32_t _icall_target = MEM32(0x3E1570); PUSH32(esp, 0x003CEF83u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x003CEF7Du); } /* indirect call */
    }

loc_003CEF83: ;
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, ebx);
    PUSH32(esp, 0);
    PUSH32(esp, MEM32(ebp + -16));
    esi = edi + 0x14;
    PUSH32(esp, eax);
    PUSH32(esp, edi);
    PUSH32(esp, 0x3CF980);
    PUSH32(esp, esi);
    { uint32_t _icall_target = MEM32(0x3E157C); PUSH32(esp, 0x003CEF9Au); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x003CEF94u); } /* indirect call */
    }

loc_003CEF9A: ;
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, esi);
    { uint32_t _icall_target = MEM32(0x3E1578); PUSH32(esp, 0x003CEFA1u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x003CEF9Bu); } /* indirect call */
    }

loc_003CEFA1: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_NE(_fa, _fb)) goto loc_003CEFAC; /* jne: not equal / not zero */

loc_003CEFA5: ;
    _cf = 0; /* xor clears CF */
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    goto loc_003CF050;

loc_003CEFAC: ;
    _cf = 0; /* logical op clears CF */
    MEM32(edi + 0x168) = MEM32(edi + 0x168) & 0;
    _fa = (uint32_t)(MEM32(edi + 0x168)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax = edi + 0x164;
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, ebx);
    PUSH32(esp, eax);
    MEM32(eax) = 0x3D0350;
    { uint32_t _icall_target = MEM32(0x3E1550); PUSH32(esp, 0x003CEFC7u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x003CEFC1u); } /* indirect call */
    }

loc_003CEFC7: ;
    eax = edi;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x003CEFCEu); RECOMP_ABI_CALL(0x003CF0C3u, sub_003CF0C3); /* call 0x003CF0C3 */

loc_003CEFCE: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_003CEFA5; /* je: equal / zero */

loc_003CEFD2: ;
    eax = edi + 0x2DC;
    MEM32(ebp + -4) = eax;
    MEM32(ebp + -12) = 2;
    edx = 0x100;

loc_003CEFE7: ;
    _cf = 0; /* xor clears CF */
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */

loc_003CEFE9: ;
    ecx = MEM32(ebp + -4);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(ecx) + (uint64_t)(eax)) >> 32) & 1);
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esi = 0xFFFFFF00u;
    MEM8(esi + ecx) = LO8(eax);
    esi = 0x100;
    MEM8(ecx) = LO8(eax);
    MEM8(esi + ecx) = LO8(eax);
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edx (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_B(_fa, _fb)) goto loc_003CEFE9; /* jb: below (unsigned <) */

loc_003CF005: ;
    _fb = (uint32_t)(0x300) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(ebp + -4)) + (uint64_t)(0x300)) >> 32) & 1);
    MEM32(ebp + -4) = MEM32(ebp + -4) + 0x300;
    _fa = (uint32_t)(MEM32(ebp + -4)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(ebp + -12) = MEM32(ebp + -12) - 1;
    _fa = (uint32_t)(MEM32(ebp + -12)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa != 0)) goto loc_003CEFE7; /* jne: not equal / not zero */

loc_003CF011: ;
    esi = MEM32(0x3E15D8);
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, 0);
    SET_LO16(edx, 0x80C0);
    SET_LO8(eax, LO8(ebx));
    PUSH32(esp, 4);
    xbox_PortOut((edx & 0xFFFFu), LO8(eax), 1); /* out */
    eax = ebp + -8;
    PUSH32(esp, eax);
    PUSH32(esp, 0x4C);
    PUSH32(esp, 0);
    PUSH32(esp, ebx);
    { uint32_t _icall_target = esi; PUSH32(esp, 0x003CF02Du); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x003CF02Bu); } /* indirect call */
    }

loc_003CF02D: ;
    _cf = 0; /* logical op clears CF */
    MEM8(ebp + -5) = MEM8(ebp + -5) | 0x1F;
    _fa = (uint32_t)(MEM8(ebp + -5)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* or result */
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, ebx);
    PUSH32(esp, 4);
    eax = ebp + -8;
    PUSH32(esp, eax);
    PUSH32(esp, 0x4C);
    PUSH32(esp, 0);
    PUSH32(esp, ebx);
    { uint32_t _icall_target = esi; PUSH32(esp, 0x003CF03Fu); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x003CF03Du); } /* indirect call */
    }

loc_003CF03F: ;
    MEM32(edi + 0xA0) = ebx;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x003CF04Au); RECOMP_ABI_CALL(0x003CF144u, sub_003CF144); /* call 0x003CF144 */

loc_003CF04A: ;
    _cf = (int)((eax) != 0);
    eax = (uint32_t)(-(int32_t)eax);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* neg result */
    eax = _cf ? 0xFFFFFFFF : 0; /* sbb self (CF extend) */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sbb result */
    _cf = (int)((eax) != 0);
    eax = (uint32_t)(-(int32_t)eax);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* neg result */

loc_003CF050: ;
    POP32(esp, esi);

loc_003CF051: ;
    POP32(esp, edi);
    POP32(esp, ebx);
    esp = ebp;
    POP32(esp, ebp); /* leave */
    esp += 4; return; /* ret */

}

/**
 * sub_003D04D9
 * Original: 0x003D04D9 - 0x003D04F8 (31 bytes, 10 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003D04D9(void)
{
    RECOMP_ENTRY(0x003D04D9u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_003D04D9: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(MEM32(0x3D83CC)) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0x3D83CC), eax (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003D04F2; /* jne: not equal / not zero */

loc_003D04E3: ;
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, 0x3D83CC);
    PUSH32(esp, eax);
    PUSH32(esp, 6);
    PUSH32(esp, eax);
    { uint32_t _icall_target = MEM32(0x3E15C8); PUSH32(esp, 0x003D04F2u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x003D04ECu); } /* indirect call */
    }

loc_003D04F2: ;
    eax = MEM32(0x3D83CC);
    esp += 4; return; /* ret */

}

/**
 * sub_003D1EF0
 * Original: 0x003D1EF0 - 0x003D2091 (417 bytes, 132 insns)
 * Category: game_render
 * CC: cdecl, 2 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003D1EF0(void)
{
    RECOMP_ENTRY(0x003D1EF0u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003D1EF0: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, ebx);
    ebx = MEM32(0x3D83D0);
    eax = ebx;
    eax = eax & 0xFFFFFFAFu;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x3FFFFF8F) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test ebx, 0x3FFFFF8F (32-bit) */
    PUSH32(esp, edi);
    edi = MEM32(esp + 0x18);
    MEM32(0x3D83D0) = eax;
    if (TEST_Z(_fa, _fb)) goto loc_003D1F16; /* je: equal / zero */

loc_003D1F11: ;
    PUSH32(esp, 0x003D1F16u); RECOMP_ABI_CALL(0x003D1E60u, sub_003D1E60); /* call 0x003D1E60 */

loc_003D1F16: ;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x40000000) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test ebx, 0x40000000 (32-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003D2089; /* jne: not equal / not zero */

loc_003D1F22: ;
    _fa = (uint32_t)(LO8(ebx)) & 0xFFu; _fb = (uint32_t)(0x40) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ebx), 0x40 (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_003D1F34; /* jne: not equal / not zero */

loc_003D1F27: ;
    ecx = MEM32(esp + 0x14);
    _fa = (uint32_t)(MEM32(ecx + 0x20)) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(ecx + 0x20), edi (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003D2089; /* je: equal / zero */

loc_003D1F34: ;
    ecx = MEM32(esp + 0x14);
    eax = MEM32(ecx + 0x794);
    edx = MEM32(ecx + 4);
    PUSH32(esp, ebp);
    MEM32(ecx + 0x20) = edi;
    ebp = MEM32(eax + 4);
    ebp = ebp & 0x10;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(esp + 0xC) = eax;
    eax = MEM32(ecx);
    _fb = (uint32_t)(0x4527D8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ebp = ebp + 0x4527D8;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edx (32-bit) */
    PUSH32(esp, esi);
    if (CMP_B(_fa, _fb)) goto loc_003D1F6B; /* jb: below (unsigned <) */

loc_003D1F5C: ;
    eax = MEM32(0x3D9588);
    edx = eax;
    edx = edx >> 1;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, edx);
    PUSH32(esp, 0x003D1F6Bu); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003D1F6B: ;
    _fa = (uint32_t)(LO8(ebx)) & 0xFFu; _fb = (uint32_t)(0x10) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ebx), 0x10 (8-bit) */
    ecx = eax;
    if (TEST_Z(_fa, _fb)) goto loc_003D1FAE; /* je: equal / zero */

loc_003D1F72: ;
    MEM32(eax) = 0x401760;
    ecx = eax + 4;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    /* nop */

loc_003D1F80: ;
    eax = ZX8(MEM8(edx + ebp));
    esi = MEM32(esp + 0x10);
    eax = eax << 4;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    ebx = MEM32(eax + esi + 0x1C);
    _fb = (uint32_t)(esi) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + esi;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esi = MEM32(eax + 0x14);
    esi = esi + esi * 2;
    esi = MEM32(esi * 4 + 0x3D93A8);
    esi = esi << 8;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(ebx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + ebx;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(ecx) = esi;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edx++;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x10) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0x10 (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003D1F80; /* jb: below (unsigned <) */

loc_003D1FAE: ;
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, ebx (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003D2018; /* jne: not equal / not zero */

loc_003D1FB4: ;
    edi = 0x41720;
    /* nop */

loc_003D1FC0: ;
    eax = ZX8(MEM8(ebx + ebp));
    edx = MEM32(esp + 0x10);
    eax = eax << 4;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edx = edx + eax;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(MEM32(edx + 0x1C)) & 0xFFFFFFFFu; _fb = (uint32_t)(2) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(edx + 0x1C), 2 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003D1FFC; /* je: equal / zero */

loc_003D1FD3: ;
    eax = MEM32(edx + 0x14);
    esi = eax + eax * 2;
    eax = MEM32(esi * 4 + 0x3D93B0);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    esi = esi * 4 + 0x3D93A8;
    if (CMP_EQ(_fa, _fb)) goto loc_003D1FFC; /* je: equal / zero */

loc_003D1FEB: ;
    eax = MEM32(eax + 4);
    _fb = (uint32_t)(MEM32(edx + 0x18)) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + MEM32(edx + 0x18);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + MEM32(esi + 4);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(ecx) = edi;
    MEM32(ecx + 4) = eax;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 8;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003D1FFC: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edi = edi + 4;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ebx++;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0x41760) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0x41760 (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_003D1FC0; /* jb: below (unsigned <) */

loc_003D2008: ;
    eax = MEM32(esp + 0x1C);
    POP32(esp, esi);
    POP32(esp, ebp);
    POP32(esp, edi);
    MEM32(eax) = ecx;
    POP32(esp, ebx);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 12; return; /* ret 8 */

loc_003D2018: ;
    MEM32(esp + 0x14) = ebx;
    ebx = 0x41720;

loc_003D2021: ;
    eax = MEM32(esp + 0x14);
    eax = ZX8(MEM8(eax + ebp));
    edx = MEM32(esp + 0x10);
    eax = eax << 4;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    esi = eax + edx;
    _fa = (uint32_t)(MEM32(esi + 0x1C)) & 0xFFFFFFFFu; _fb = (uint32_t)(2) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(esi + 0x1C), 2 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003D206D; /* je: equal / zero */

loc_003D2039: ;
    eax = MEM32(esi + 0x14);
    edx = eax + eax * 2;
    edi = MEM32(edx * 4 + 0x3D93B0);
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0 (32-bit) */
    edx = edx * 4 + 0x3D93A8;
    if (CMP_EQ(_fa, _fb)) goto loc_003D206D; /* je: equal / zero */

loc_003D2051: ;
    eax = MEM32(edx);
    eax = (uint32_t)((int32_t)eax * (int32_t)MEM32(esp + 0x20));
    _fb = (uint32_t)(MEM32(edi + 4)) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + MEM32(edi + 4);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edi = MEM32(esi + 0x18);
    esi = MEM32(edx + 4);
    _fb = (uint32_t)(edi) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + edi;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(esi) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + esi;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(ecx) = ebx;
    MEM32(ecx + 4) = eax;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 8;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_003D206D: ;
    esi = MEM32(esp + 0x14);
    esi++;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ebx = ebx + 4;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x41760) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, 0x41760 (32-bit) */
    MEM32(esp + 0x14) = esi;
    if (CMP_B(_fa, _fb)) goto loc_003D2021; /* jb: below (unsigned <) */

loc_003D2081: ;
    eax = MEM32(esp + 0x1C);
    POP32(esp, esi);
    MEM32(eax) = ecx;
    POP32(esp, ebp);

loc_003D2089: ;
    POP32(esp, edi);
    POP32(esp, ebx);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 12; return; /* ret 8 */

}

/**
 * sub_003D20A0
 * Original: 0x003D20A0 - 0x003D21EC (332 bytes, 94 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_003D20A0(void)
{
    RECOMP_ENTRY(0x003D20A0u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_003D20A0: ;
    eax = MEM32(0x3D83D0);
    _fb = (uint32_t)(0x14) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x14;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x3FFFFF8F) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test eax, 0x3FFFFF8F (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003D20B9; /* je: equal / zero */

loc_003D20AF: ;
    PUSH32(esp, 0x003D20B4u); RECOMP_ABI_CALL(0x003D1E60u, sub_003D1E60); /* call 0x003D1E60 */

loc_003D20B4: ;
    eax = MEM32(0x3D83D0);

loc_003D20B9: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x20) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(eax), 0x20 (8-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_003D21E8; /* je: equal / zero */

loc_003D20C1: ;
    ecx = MEM32(esi + 4);
    eax = eax & 0xFFFFFFDFu;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax = eax | 0x50;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    PUSH32(esp, ebx);
    MEM32(0x3D83D0) = eax;
    eax = MEM32(esi + 0x794);
    ebx = MEM32(eax + 4);
    PUSH32(esp, ebp);
    ebx = ebx & 0x10;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(esp + 0x14) = eax;
    eax = MEM32(esi);
    PUSH32(esp, edi);
    edi = 0; /* xor self */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fb = (uint32_t)(0x4527D8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ebx = ebx + 0x4527D8;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ecx (32-bit) */
    MEM32(esp + 0x10) = edi;
    MEM32(esp + 0x14) = edi;
    if (CMP_B(_fa, _fb)) goto loc_003D2107; /* jb: below (unsigned <) */

loc_003D20F8: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, ecx);
    PUSH32(esp, 0x003D2107u); RECOMP_ABI_CALL(0x003CAB80u, sub_003CAB80); /* call 0x003CAB80 */

loc_003D2107: ;
    MEM32(eax) = 0x401760;
    ebp = eax + 4;
    MEM32(esp + 0xC) = 0;
    goto loc_003D2120;

    /* nop */

loc_003D2120: ;
    edx = MEM32(esp + 0xC);
    eax = ZX8(MEM8(edx + ebx));
    ecx = MEM32(esp + 0x18);
    eax = eax << 4;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edx = eax + ecx;
    ecx = MEM32(edx + 0x1C);
    MEM32(ebp) = ecx;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ebp = ebp + 4;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(2) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 2 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_003D21AB; /* je: equal / zero */

loc_003D2140: ;
    edx = MEM32(edx + 0x18);
    eax = ecx;
    eax = eax >> 4;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    eax = eax & 0xF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    eax = ZX8(MEM8(eax + 0x3D4AF8));
    ecx = ecx & 0xF;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    ecx = ZX8(MEM8(ecx + 0x3D4AF0));
    eax = (uint32_t)((int32_t)eax * (int32_t)ecx);
    _fb = (uint32_t)(3) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 3;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = eax >> 2;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_003D217A; /* jne: not equal / not zero */

loc_003D2169: ;
    MEM32(esp + 0x1C) = edx;
    MEM32(esi + 0xBD4) = eax;
    edi = 1;
    goto loc_003D219A;

loc_003D217A: ;
    ecx = edx;
    _fb = (uint32_t)(MEM32(esp + 0x14)) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    ecx = ecx - MEM32(esp + 0x14);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    if ((_fa != 0)) goto loc_003D218B; /* jne: not equal / not zero */

loc_003D2182: ;
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    MEM32(esi + edi * 8 + 0xBCC) = MEM32(esi + edi * 8 + 0xBCC) + eax;
    _fa = (uint32_t)(MEM32(esi + edi * 8 + 0xBCC)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_003D219A;

loc_003D218B: ;
    MEM32(esi + edi * 8 + 0xBD0) = ecx;
    MEM32(esi + edi * 8 + 0xBD4) = eax;
    edi++;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_003D219A: ;
    ecx = MEM32(esp + 0x10);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edx = edx + eax * 4;
    MEM32(esp + 0x10) = ecx;
    MEM32(esp + 0x14) = edx;

loc_003D21AB: ;
    eax = MEM32(esp + 0xC);
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x10) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x10 (32-bit) */
    MEM32(esp + 0xC) = eax;
    if (CMP_B(_fa, _fb)) goto loc_003D2120; /* jb: below (unsigned <) */

loc_003D21BD: ;
    eax = MEM32(esp + 0x10);
    ecx = MEM32(esp + 0x14);
    MEM32(esi + 0xBC8) = eax;
    eax = MEM32(esp + 0x1C);
    MEM32(esi + 0xC54) = edi;
    POP32(esp, edi);
    MEM32(esi) = ebp;
    MEM32(esi + 0xBCC) = eax;
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    POP32(esp, ebp);
    MEM32(esi + 0xBD0) = eax;
    POP32(esp, ebx);

loc_003D21E8: ;
    _fb = (uint32_t)(0x14) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x14;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 4; return; /* ret */

}


/**
 * sub_001A0BB0 -- D3DDevice_BeginVisibilityTest, inlined into the title (the
 * count cleared and turned on: NV097_CLEAR_REPORT_VALUE, SET_ZPASS_PIXEL_COUNT_ENABLE);
 * src/d3d/d3d_hooks.c tells the renderer first.
 * Original: 0x001A0BB0 - 0x001A0BF1 (65 bytes, 21 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void d3d_orig_001A0BB0(void)
{
    RECOMP_ENTRY(0x001A0BB0u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_001A0BB0: ;
    PUSH32(esp, esi);
    esi = MEM32(0x3D83C8);
    eax = MEM32(esi);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 4)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 4) (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_001A0BCD; /* jb: below (unsigned <) */

loc_001A0BBE: ;
    eax = MEM32(0x3D9588);
    ecx = eax;
    ecx = ecx >> 1;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, ecx);
    PUSH32(esp, 0x001A0BCDu); RECOMP_ICALL_SAFE(0x003CAB80u, _icall_esp); /* manual call 0x003CAB80 */
    }

loc_001A0BCD: ;
    edx = MEM32(esp + 8);
    MEM32(eax) = 0x817C8;
    ecx = 1;
    MEM32(eax + 4) = ecx;
    MEM32(eax + 8) = ecx;
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0xC;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(esi) = eax;
    eax = MEM32(0x9777DC);
    MEM32(edx + 0x54) = eax;
    POP32(esp, esi);
    esp += 4; return; /* ret */

}

