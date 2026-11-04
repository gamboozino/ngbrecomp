/*
 * sound_boot.c -- the settings a cold start applies (sub_001DB1C0, called once by the start-up sub_00166D10), the
 * lifted one kept here (src/manual_functions.json) with one change: the sound volumes come from system.dat as the
 * rest do, not the defaults. The game takes the saved volumes only when relaunched (sub_001DAFF0, the same
 * function but for those three loads); after a cold start it compared them with the defaults at the title and
 * asked "Apply changes to the sound settings?" every time, once any volume had been changed. Its "Yes" is
 * sub_001DAFF0 too, so this is the answer given at start.
 */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"

/**
 * sub_001DB1C0
 * Original: 0x001DB1C0 - 0x001DB2A0 (224 bytes, 37 insns)
 * Category: game_render
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_001DB1C0(void)
{
    RECOMP_ENTRY(0x001DB1C0u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_001DB1C0: ;
    eax = ZX8(MEM8(esi + 0x87));
    MEM32(0x44D454) = eax;
    PUSH32(esp, 0x001DB1D1u); RECOMP_ABI_CALL(0x0031E630u, sub_0031E630); /* call 0x0031E630 */

loc_001DB1D1: ;
    eax = ZX8(MEM8(esi + 0x88));
    xmm0.f[0] = (float)(int32_t)MEM32(0x44D454); /* cvtsi2ss */
    MEM32(0x44D45C) = eax;
    MEM32(0x44D458) = eax;
    eax = ZX8(MEM8(esi + 0x89));
    MEMF(0xBB4C00) = xmm0.f[0]; /* movss */
    MEM32(0x44D460) = eax;
    PUSH32(esp, 0x001DB203u); RECOMP_ABI_CALL(0x0031E790u, sub_0031E790); /* call 0x0031E790 */

loc_001DB203: ;
    eax = ZX8(MEM8(esi + 0x8C));
    ecx = ZX8(MEM8(esi + 0x8E));
    edx = ZX8(MEM8(esi + 0x8D));
    xmm0.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x41492C); /* mulss */
    xmm0.f[0] = xmm0.f[0] + MEMF(0x4148C8); /* addss */
    MEMF(0x9ABD98) = xmm0.f[0]; /* movss */
    xmm0.f[0] = (float)(int32_t)ecx; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x4148C0); /* mulss */
    xmm0.f[0] = xmm0.f[0] - MEMF(0x3EC1C8); /* subss */
    MEMF(0x9ABD94) = xmm0.f[0]; /* movss */
    xmm0.f[0] = (float)(int32_t)edx; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x4148D0); /* mulss */
    xmm0.f[0] = xmm0.f[0] - MEMF(0x4148F8); /* subss */
    MEMF(0x9ABD9C) = xmm0.f[0]; /* movss */
    PUSH32(esp, 0x001DB271u); RECOMP_ABI_CALL(0x00317C50u, sub_00317C50); /* call 0x00317C50 */

loc_001DB271: ;
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, 0x9ABDA0);
    PUSH32(esp, 1);
    PUSH32(esp, 0x001DB27Du); RECOMP_ICALL_SAFE(0x003C6D90u, _icall_esp); /* manual call 0x003C6D90 */
    }

loc_001DB27D: ;
    SET_LO8(eax, MEM8(esi + 0x8F));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    xmm0 = XMM_SCALAR(MEMF(0x414BD0)); /* movss */
    if (CMP_EQ(_fa, _fb)) goto loc_001DB297; /* je: equal / zero */

loc_001DB28F: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */

loc_001DB297: ;
    MEMF(0x44D444) = xmm0.f[0]; /* movss */
    esp += 4; return; /* ret */

}

