/*
 * costume_menu.c -- the Costume Selection with the costumes the game never offers.
 *
 * The game has eight costumes (chr_ryu.afs, chr_ryua-g.afs: 0-7) but its Costume Selection (New Game's,
 * and Load Game's Costume) offers five: the game's choice and 4-7, from a table of 12-byte entries at
 * 0x3E9D0C -- the picture's sprite id (the locked one id + 5), the flag that unlocks it
 * (sub_001DB520) and the costume. With All Costumes on (src/cheats.c) the list here is that one and
 * 1, 2 and 3 after it, each with a picture of ours (src/pause_tab.c: the game's own default picture as
 * a dark silhouette with the costume's name, as the game has none) and always unlocked; otherwise it
 * is the game's. The functions below are the lifted ones (src/manual_functions.json) with each change
 * marked "Costumes:"; the picture carousel's round list of eleven entry pointers (five, then six more
 * to wrap) is worked out from the row instead.
 */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"
#include <math.h>
#include <string.h>

#include "xbox_memory_layout.h"   /* xbox_HeapAlloc */

#define GAME_TABLE 0x3E9D0C       /* the game's five */
#define EXTRAS 3

int ngb_pc_setting(int which);                 /* src/ingame_menu.c: 16 All Costumes */
uint32_t ngb_extra_costume_picture(int n);     /* src/pause_tab.c: costume n's picture, sprite id; 0 none */

static uint32_t s_table;          /* ours: the game's five, then 1-3 */
static int s_n = 5;               /* how many are offered (set as the menu opens) */

static uint32_t table(void)
{
    if (!s_table) {
        s_table = xbox_HeapAlloc(12 * (5 + EXTRAS), 4);
        memcpy((void *)XBOX_PTR(s_table), (void *)XBOX_PTR(GAME_TABLE), 12 * 5);
        for (int i = 0; i < EXTRAS; i++) {
            uint32_t e = s_table + 12 * (5 + i);
            MEM32(e) = 0;             /* the picture: when made */
            MEM32(e + 4) = 0;         /* always unlocked (flag 0 is the game's choice's) */
            MEM32(e + 8) = (uint32_t)(1 + i);
        }
    }
    return s_table;
}

/* As the menu opens, the list: the game's five, the locked ones as the game shows them (white shapes,
 * not chosen: its own checks of the flag, #246), and with All Costumes on the extra costumes after
 * them, while their pictures can be made. */
static void costume_count_now(void)
{
    uint32_t t = table();
    s_n = 0;
    for (int i = 0; i < 5; i++) {
        memcpy((void *)XBOX_PTR(t + 12 * s_n), (void *)XBOX_PTR(GAME_TABLE + 12 * i), 12);
        s_n++;
    }
    if (!ngb_pc_setting(16))
        return;
    for (int i = 0; i < EXTRAS; i++) {
        uint32_t id = ngb_extra_costume_picture(1 + i);
        if (!id)
            break;
        uint32_t e = t + 12 * s_n;
        MEM32(e) = id;
        MEM32(e + 4) = 0;         /* always unlocked (flag 0 is the game's choice's) */
        MEM32(e + 8) = (uint32_t)(1 + i);
        s_n++;
    }
}

/**
 * sub_001773F0
 * Original: 0x001773F0 - 0x00177555 (357 bytes, 105 insns)
 * Category: game_vtable
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_001773F0(void)
{
    RECOMP_ENTRY(0x001773F0u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_001773F0: ;
    eax = MEM32(esi);
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    PUSH32(esp, edi);
    ecx = esi;
    { uint32_t _icall_esp = g_esp;
    { uint32_t _icall_target = MEM32(eax); PUSH32(esp, 0x001773F9u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x001773F7u); } /* indirect call */
    }

loc_001773F9: ;
    edx = MEM32(esi);
    { uint32_t _icall_esp = g_esp;
    {
        extern void ngb_sprite_range_dump(void);   /* src/pause_tab.c: debugging */
        ngb_sprite_range_dump();
    }
    costume_count_now();                      /* Costumes: how many this time */
    PUSH32(esp, (uint32_t)(s_n - 1));         /* Costumes: the last row */
    ecx = esi;
    { uint32_t _icall_target = MEM32(edx + 0x18); PUSH32(esp, 0x00177402u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x001773FFu); } /* indirect call */
    }

loc_00177402: ;
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    ebp = 0; /* xor self */
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    edi = 0; /* xor self */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    goto loc_00177410;

    /* nop */

loc_00177410: ;
    eax = ZX8(MEM8(esi + 0x3D));
    _fa = (uint32_t)(MEM32(edi + table() + 8)) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;   /* Costumes: ours */
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(edi + 0x3E9D14), eax (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00177424; /* jne: not equal / not zero */

loc_0017741C: ;
    edx = MEM32(esi);
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, ebp);
    ecx = esi;
    { uint32_t _icall_target = MEM32(edx + 0xC); PUSH32(esp, 0x00177424u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x00177421u); } /* indirect call */
    }

loc_00177424: ;
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edi = edi + 0xC;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ebp++;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(12 * s_n) & 0xFFFFFFFFu;   /* Costumes: all of ours */
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0x3C (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_00177410; /* jb: below (unsigned <) */

loc_0017742D: ;
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    eax = 0x74F;
    MEM32(0x45A7DC) = eax;
    edx = 0x6D5;
    MEM32(0x45A7E4) = edx;
    ecx = 0x6D6;
    MEM32(0x45A7E0) = ecx;
    MEM16(0x45A7F0) = 0xF5;
    eax = 1;
    MEM32(0x45A7D0) = eax;
    MEM32(0x45A7D4) = ebx;
    MEMF(0x45A7D8) = xmm0.f[0]; /* movss */
    edi = 0x6D8;
    MEM32(0x45A7E8) = edi;
    ebp = 0x6D7;
    MEM32(0x45A7EC) = ebp;
    edx = ebx;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, eax (32-bit) */
    MEM32(esi + 0x40) = ebx;
    MEM32(esi + 0x44) = ebx;
    MEM32(esi + 0x48) = ebx;
    MEM32(esi + 0x4C) = ebx;
    ecx = 0x14;
    if (CMP_EQ(_fa, _fb)) goto loc_001774A7; /* je: equal / zero */

loc_0017749E: ;
    MEM32(esi + 0x44) = eax;
    MEM32(esi + 0x48) = ebx;
    MEM32(esi + 0x4C) = ecx;

loc_001774A7: ;
    xmm2 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    eax = ebx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ebx (32-bit) */
    MEMF(esi + 0x50) = xmm0.f[0]; /* movss */
    MEMF(esi + 0x54) = xmm0.f[0]; /* movss */
    MEM32(esi + 0x58) = ebx;
    MEM32(esi + 0x5C) = ebx;
    MEM32(esi + 0x60) = ebx;
    if (CMP_NE(_fa, _fb)) goto loc_001774CD; /* jne: not equal / not zero */

loc_001774C8: ;
    xmm1 = xmm0; /* movaps */
    goto loc_0017753E;

loc_001774CD: ;
    xmm0.f[0] = (float)(int32_t)MEM32(esi + 0x58); /* cvtsi2ss */
    xmm3 = XMM_SCALAR(MEMF(esi + 0x50)); /* movss */
    xmm1.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    eax = MEM32(esi + 0x60);
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(3) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 3 (32-bit) */
    xmm0.f[0] = xmm0.f[0] / xmm1.f[0]; /* divss */
    if (CMP_A(_fa, _fb)) goto loc_0017752D; /* ja: above (unsigned >) */

loc_001774E8: ;
    { uint32_t _jt = MEM32(eax * 4 + 0x177558); /* switch: 4 entries, 4 targets */
    if (_jt == 0x001774EFu) goto loc_001774EF;
    if (_jt == 0x001774FCu) goto loc_001774FC;
    if (_jt == 0x00177505u) goto loc_00177505;
    if (_jt == 0x00177515u) goto loc_00177515;
    g_seh_ebp = ebp; RECOMP_ITAIL(_jt); return; }

loc_001774EF: ;
    xmm1 = xmm0; /* movaps */
    xmm1.f[0] = xmm1.f[0] * xmm0.f[0]; /* mulss */
    xmm1.f[0] = xmm1.f[0] * xmm0.f[0]; /* mulss */
    goto loc_0017752A;

loc_001774FC: ;
    xmm1 = xmm0; /* movaps */
    xmm1.f[0] = xmm1.f[0] * xmm0.f[0]; /* mulss */
    goto loc_0017752A;

loc_00177505: ;
    xmm1 = xmm2; /* movaps */
    xmm1.f[0] = xmm1.f[0] - xmm0.f[0]; /* subss */
    xmm0 = xmm1; /* movaps */
    xmm0.f[0] = xmm0.f[0] * xmm1.f[0]; /* mulss */
    goto loc_0017751F;

loc_00177515: ;
    xmm1 = xmm2; /* movaps */
    xmm1.f[0] = xmm1.f[0] - xmm0.f[0]; /* subss */
    xmm0 = xmm1; /* movaps */

loc_0017751F: ;
    xmm0.f[0] = xmm0.f[0] * xmm1.f[0]; /* mulss */
    xmm1 = xmm2; /* movaps */
    xmm1.f[0] = xmm1.f[0] - xmm0.f[0]; /* subss */

loc_0017752A: ;
    xmm0 = xmm1; /* movaps */

loc_0017752D: ;
    xmm1 = XMM_SCALAR(MEMF(esi + 0x54)); /* movss */
    xmm1.f[0] = xmm1.f[0] - xmm3.f[0]; /* subss */
    xmm1.f[0] = xmm1.f[0] * xmm0.f[0]; /* mulss */
    xmm1.f[0] = xmm1.f[0] + xmm3.f[0]; /* addss */

loc_0017753E: ;
    POP32(esp, edi);
    POP32(esp, ebp);
    MEM32(esi + 0x58) = ebx;
    MEM32(esi + 0x60) = ebx;
    MEMF(esi + 0x50) = xmm1.f[0]; /* movss */
    MEMF(esi + 0x54) = xmm2.f[0]; /* movss */
    MEM32(esi + 0x5C) = ecx;
    POP32(esp, ebx);
    esp += 4; return; /* ret */

}

/**
 * sub_00177570
 * Original: 0x00177570 - 0x00177780 (528 bytes, 137 insns)
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_00177570(void)
{
    RECOMP_ENTRY(0x00177570u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
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
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_00177570: ;
    _fb = (uint32_t)(0x58) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x58;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    ebp = MEM32(esp + 0x64);
    eax = MEM32(ebp + 0x5C);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    if (CMP_NE(_fa, _fb)) goto loc_00177589; /* jne: not equal / not zero */

loc_00177582: ;
    xmm0 = XMM_SCALAR(MEMF(ebp + 0x54)); /* movss */
    goto loc_001775AF;

loc_00177589: ;
    ecx = MEM32(ebp + 0x54);
    edx = MEM32(ebp + 0x50);
    xmm0.f[0] = (float)(int32_t)MEM32(ebp + 0x58); /* cvtsi2ss */
    PUSH32(esp, ecx);
    PUSH32(esp, edx);
    xmm1.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    eax = MEM32(ebp + 0x60);
    PUSH32(esp, ecx);
    xmm0.f[0] = xmm0.f[0] / xmm1.f[0]; /* divss */
    MEMF(esp) = xmm0.f[0]; /* movss */
    PUSH32(esp, 0x001775ACu); RECOMP_ABI_CALL(0x00303C50u, sub_00303C50); /* call 0x00303C50 */

loc_001775AC: ;
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_001775AF: ;
    eax = 0x3E9D24;
    edi = 0x3E9D30;
    esi = 0x3E9D3C;
    edx = 0x3E9D0C;
    ecx = 0x3E9D18;
    MEMF(esp + 0x6C) = xmm0.f[0]; /* movss */
    MEM32(esp + 0x3C) = eax;
    MEM32(esp + 0x40) = edi;
    MEM32(esp + 0x44) = esi;
    MEM32(esp + 0x48) = edx;
    MEM32(esp + 0x4C) = ecx;
    MEM32(esp + 0x50) = eax;
    MEM32(esp + 0x54) = edi;
    MEM32(esp + 0x58) = esi;
    MEM32(esp + 0x5C) = edx;
    MEM32(esp + 0x60) = ecx;
    MEM32(esp + 0x64) = eax;
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    /* nop */

loc_00177600: ;
    eax = MEM32(ebp);
    ecx = ebp;
    { uint32_t _icall_esp = g_esp;
    { uint32_t _icall_target = MEM32(eax + 8); PUSH32(esp, 0x00177608u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x00177605u); } /* indirect call */
    }

loc_00177608: ;
    edx = MEM32(ebp);
    esi = ebx;
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esi = esi - eax;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    ecx = ebp;
    _fb = (uint32_t)(3) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esi = esi - 3;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    { uint32_t _icall_esp = g_esp;
    { uint32_t _icall_target = MEM32(edx + 0x14); PUSH32(esp, 0x00177617u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x00177614u); } /* indirect call */
    }

loc_00177617: ;
    fp_top() = RECOMP_FP_PC(fp_top() * (double)SMEM32(0x45A9B8)); /* fimul dword ptr [0x45a9b8] */
    PUSH32(esp, 0x00177622u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_00177622: ;
    edx = MEM32(0x45A9B8);
    xmm2 = XMM_ZERO(); /* xorps self = zero */
    ecx = edx;
    ecx = (uint32_t)((int32_t)ecx * (int32_t)esi);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    ecx = ecx - eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    eax = edx + edx;
    xmm1.f[0] = (float)(int32_t)ecx; /* cvtsi2ss */
    xmm0.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    xmm1.f[0] = xmm1.f[0] / xmm0.f[0]; /* divss */
    _fca = xmm1.f[0]; _fcb = xmm2.f[0]; /* comiss */
    if ((_fca < _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_0017764B; /* jb: below (unsigned <) (xmm1.f[0] vs xmm2.f[0]) */

loc_00177646: ;
    xmm0 = xmm1; /* movaps */
    goto loc_00177652;

loc_0017764B: ;
    xmm0 = xmm2; /* movaps */
    xmm0.f[0] = xmm0.f[0] - xmm1.f[0]; /* subss */

loc_00177652: ;
    xmm3 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    xmm1 = xmm3; /* movaps */
    xmm1.f[0] = xmm1.f[0] - xmm0.f[0]; /* subss */
    xmm0 = xmm1; /* movaps */
    xmm0.f[0] = xmm0.f[0] - xmm3.f[0]; /* subss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x45A9B4); /* mulss */
    xmm0.f[0] = xmm0.f[0] + xmm1.f[0]; /* addss */
    _fca = xmm2.f[0]; _fcb = xmm0.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_0017767C; /* jbe: below or equal (unsigned <=) (xmm2.f[0] vs xmm0.f[0]) */

loc_00177679: ;
    xmm0 = xmm2; /* movaps */

loc_0017767C: ;
    xmm0.f[0] = xmm0.f[0] * MEMF(esp + 0x6C); /* mulss */
    _fca = xmm0.f[0]; _fcb = xmm2.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_0017773F; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs xmm2.f[0]) */

loc_0017768B: ;
    /* Costumes: each entry once, at its nearest place round the list (with fewer than five the game's
     * list of eleven would show one beside itself) */
    if ((int32_t)esi < -(s_n - 1) / 2 || (int32_t)esi > s_n / 2)
        goto loc_0017773F;
    esi = table() + 12 * ((ebx + s_n - 3) % s_n);   /* Costumes: the entry (the game's list of five, round) */
    edx = MEM32(esi);
    eax = MEM32(0x977680);
    edi = MEM32(0x45A9A8);
    MEM32(esp + 0x10) = edx;
    edx = ((int32_t)eax < 0) ? 0xFFFFFFFF : 0; /* cdq */
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - edx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    eax = (uint32_t)(((int32_t)(int32_t)(eax)) >> ((1) & 31u));
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sar result */
    _fb = (uint32_t)(edi) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + edi;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx = MEM32(esi + 4);
    MEM16(esp + 0x14) = LO16(eax);
    SET_LO16(eax, MEM16(0x45A9AC));
    MEM16(esp + 0x16) = LO16(eax);
    MEM16(esp + 0x18) = 0x1CC;
    MEM8(esp + 0x1B) = 1;
    MEM8(esp + 0x1C) = 1;
    MEM32(esp + 0x20) = 0;
    MEMF(esp + 0x2C) = xmm3.f[0]; /* movss */
    MEMF(esp + 0x34) = xmm2.f[0]; /* movss */
    MEM32(esp + 0x38) = 0xFFFFFFFFu;
    PUSH32(esp, 0x001776EEu); RECOMP_ABI_CALL(0x001DB520u, sub_001DB520); /* call 0x001DB520 */

loc_001776EE: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001776F7; /* jne: not equal / not zero */

loc_001776F2: ;
    _fb = (uint32_t)(5) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    MEM32(esp + 0x10) = MEM32(esp + 0x10) + 5;
    _fa = (uint32_t)(MEM32(esp + 0x10)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_001776F7: ;
    _fa = (uint32_t)(MEM8(0x9776A8)) & 0xFFu; _fb = (uint32_t)(0x10) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test MEM8(0x9776A8), 0x10 (8-bit) */
    xmm1 = XMM_SCALAR(MEMF(0x45A9B0)); /* movss */
    MEMF(esp + 0x30) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x414BD0)); /* movss */
    MEM8(esp + 0x1A) = 0x11;
    MEMF(esp + 0x28) = xmm1.f[0]; /* movss */
    if (TEST_NZ(_fa, _fb)) goto loc_00177724; /* jne: not equal / not zero */

loc_00177721: ;
    xmm0 = xmm3; /* movaps */

loc_00177724: ;
    xmm0.f[0] = xmm0.f[0] * xmm1.f[0]; /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x414C90); /* mulss */
    edi = esp + 0x10;
    MEMF(esp + 0x24) = xmm0.f[0]; /* movss */
    PUSH32(esp, 0x0017773Fu); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_0017773F: ;
    ebx++;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(s_n + 6) & 0xFFFFFFFFu;   /* Costumes: ours */
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, 0xB (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_00177600; /* jb: below (unsigned <) */

loc_00177749: ;
    esi = MEM32(esp + 0x6C);
    PUSH32(esp, esi);
    PUSH32(esp, 0x00177753u); RECOMP_ABI_CALL(0x00177780u, sub_00177780); /* call 0x00177780 */

loc_00177753: ;
    PUSH32(esp, esi);
    ecx = ebp;
    PUSH32(esp, 0x0017775Bu); RECOMP_ABI_CALL(0x001778C0u, sub_001778C0); /* call 0x001778C0 */

loc_0017775B: ;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebp);
    MEM32(0xC0CBE4) = 0x1E0;
    MEM32(0xC0CBE0) = 4;
    MEM8(0xC0CBE8) = 1;
    POP32(esp, ebx);
    _fb = (uint32_t)(0x58) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x58;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 8; return; /* ret 4 */

    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}

/**
 * sub_001778C0
 * Original: 0x001778C0 - 0x0017796D (173 bytes, 44 insns)
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_001778C0(void)
{
    RECOMP_ENTRY(0x001778C0u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_001778C0: ;
    eax = MEM32(ecx);
    _fb = (uint32_t)(0x54) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x54;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    { uint32_t _icall_esp = g_esp;
    { uint32_t _icall_target = MEM32(eax + 8); PUSH32(esp, 0x001778C8u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x001778C5u); } /* indirect call */
    }

loc_001778C8: ;
    ecx = eax + eax * 2;
    ecx = MEM32(ecx * 4 + table() + 4);       /* Costumes: ours */
    PUSH32(esp, 0x001778D7u); RECOMP_ABI_CALL(0x001DB520u, sub_001DB520); /* call 0x001DB520 */

loc_001778D7: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 1 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001778EC; /* jne: not equal / not zero */

loc_001778DC: ;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(edx, MEM8(0xBCE030));
    eax = 0x39;
    PUSH32(esp, edx);
    goto loc_001778F9;

loc_001778EC: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(eax, MEM8(0xBCE030));
    PUSH32(esp, eax);
    eax = 0x38;

loc_001778F9: ;
    PUSH32(esp, 0x001778FEu); RECOMP_ABI_CALL(0x001E6AE0u, sub_001E6AE0); /* call 0x001E6AE0 */

loc_001778FE: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00177967; /* je: equal / zero */

loc_00177905: ;
    ecx = (uint32_t)(int32_t)SMEM16(0x45A99C);
    edx = (uint32_t)(int32_t)SMEM16(0x45A9A0);
    PUSH32(esp, 2);
    PUSH32(esp, 0x1CE);
    PUSH32(esp, ecx);
    _fb = (uint32_t)(MEM32(0x977680)) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edx = edx + MEM32(0x977680);
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx = eax;
    PUSH32(esp, edx);
    eax = esp + 0x10;
    PUSH32(esp, 0x0017792Du); RECOMP_ABI_CALL(0x00168730u, sub_00168730); /* call 0x00168730 */

loc_0017792D: ;
    xmm0 = XMM_SCALAR(MEMF(0x45A998)); /* movss */
    MEMF(esp + 0x30) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x2C) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x68)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x414A70); /* mulss */
    eax = esp + 0x10;
    PUSH32(esp, eax);
    MEM8(esp + 0x5E) = 0x22;
    MEMF(esp + 0x28) = xmm0.f[0]; /* movss */
    PUSH32(esp, 0x00177964u); RECOMP_ABI_CALL(0x001E6D30u, sub_001E6D30); /* call 0x001E6D30 */

loc_00177964: ;
    _fb = (uint32_t)(0x14) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x14;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_00177967: ;
    _fb = (uint32_t)(0x54) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x54;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 8; return; /* ret 4 */

}

/**
 * sub_00177970
 * Original: 0x00177970 - 0x001779E9 (121 bytes, 40 insns)
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_00177970(void)
{
    RECOMP_ENTRY(0x00177970u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_00177970: ;
    ecx = ZX8(MEM8(0xDB932E));
    ecx = (uint32_t)((int32_t)ecx * (int32_t)0x78);
    PUSH32(esp, esi);
    _fb = (uint32_t)(0xD219A0) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 0xD219A0;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esi = eax;
    eax = MEM32(esi);
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, ecx);
    ecx = esi;
    { uint32_t _icall_target = MEM32(eax + 0x20); PUSH32(esp, 0x0017798Bu); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x00177988u); } /* indirect call */
    }

loc_0017798B: ;
    eax = MEM32(esi + 0x18);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001779E5; /* jne: not equal / not zero */

loc_00177992: ;
    PUSH32(esp, edi);
    edi = ZX8(MEM8(esp + 0xC));
    edi = (uint32_t)((int32_t)edi * (int32_t)0x78);
    eax = MEM32(edi + 0xD219A8);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x510) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test eax, 0x510 (32-bit) */
    edi = edi + 0xD219A8;
    if (TEST_Z(_fa, _fb)) goto loc_001779D7; /* je: equal / zero */

loc_001779AE: ;
    edx = MEM32(esi);
    ecx = esi;
    { uint32_t _icall_esp = g_esp;
    { uint32_t _icall_target = MEM32(edx + 8); PUSH32(esp, 0x001779B5u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x001779B2u); } /* indirect call */
    }

loc_001779B5: ;
    eax = eax + eax * 2;
    ecx = MEM32(eax * 4 + table() + 4);       /* Costumes: ours */
    PUSH32(esp, 0x001779C4u); RECOMP_ABI_CALL(0x001DB520u, sub_001DB520); /* call 0x001DB520 */

loc_001779C4: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 1 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001779D7; /* jne: not equal / not zero */

loc_001779C9: ;
    PUSH32(esp, ebx);
    ebx = esi;
    PUSH32(esp, 0x001779D1u); RECOMP_ABI_CALL(0x001779F0u, sub_001779F0); /* call 0x001779F0 */

loc_001779D1: ;
    POP32(esp, ebx);
    POP32(esp, edi);
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

loc_001779D7: ;
    _fa = (uint32_t)(MEM32(edi)) & 0xFFFFFFFFu; _fb = (uint32_t)(0x220) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test MEM32(edi), 0x220 (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_001779E4; /* je: equal / zero */

loc_001779DF: ;
    PUSH32(esp, 0x001779E4u); RECOMP_ABI_CALL(0x00177AB0u, sub_00177AB0); /* call 0x00177AB0 */

loc_001779E4: ;
    POP32(esp, edi);

loc_001779E5: ;
    POP32(esp, esi);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_001779F0
 * Original: 0x001779F0 - 0x00177AB0 (192 bytes, 53 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: standard_frame
 */
void sub_001779F0(void)
{
    RECOMP_ENTRY(0x001779F0u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* prologue saves caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_001779F0: ;
    PUSH32(esp, ebp);
    ebp = esp;
    g_ebp = ebp; /* publish frame for frameless callees */
    g_seh_ebp = ebp;
    esp = esp & 0xFFFFFFF0u;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fb = (uint32_t)(0x68) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x68;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    eax = MEM32(ebx);
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    ecx = ebx;
    MEM8(ebx + 0x3C) = 1;
    { uint32_t _icall_esp = g_esp;
    { uint32_t _icall_target = MEM32(eax + 8); PUSH32(esp, 0x00177A06u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x00177A03u); } /* indirect call */
    }

loc_00177A06: ;
    ecx = eax + eax * 2;
    SET_LO8(edx, MEM8(ecx * 4 + table() + 8));   /* Costumes: ours */
    PUSH32(esp, 0x40);
    PUSH32(esp, 5);
    MEM8(ebx + 0x3D) = LO8(edx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00177A1Cu); RECOMP_ABI_CALL(0x001E3400u, sub_001E3400); /* call 0x001E3400 */

loc_00177A1C: ;
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    eax = MEM32(0xC0CEF0);
    edx = 2;
    MEMF(esp + 0x18) = xmm0.f[0]; /* movss */
    MEM32(esp + 0x1C) = 0;
    MEM32(esp + 0x20) = edx;
    ecx = 8;
    esi = 0x431160;
    edi = esp + 0x38;
    if (!g_df) { uint8_t *_d = (uint8_t*)XBOX_PTR(edi), *_s = (uint8_t*)XBOX_PTR(esi); uint32_t _n = ecx * 4;
      if (_d + _n <= _s || _s + _n <= _d) memcpy(_d, _s, _n);
      else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*4) = MEM32(esi + _i*4); }
      esi += ecx * 4; edi += ecx * 4; }
    else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi - _i*4) = MEM32(esi - _i*4); esi -= ecx * 4; edi -= ecx * 4; }
    ecx = 0; /* rep movsd */
    ecx = 8;
    esi = 0xC0CAD0;
    edi = esp + 0x58;
    if (!g_df) { uint8_t *_d = (uint8_t*)XBOX_PTR(edi), *_s = (uint8_t*)XBOX_PTR(esi); uint32_t _n = ecx * 4;
      if (_d + _n <= _s || _s + _n <= _d) memcpy(_d, _s, _n);
      else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*4) = MEM32(esi + _i*4); }
      esi += ecx * 4; edi += ecx * 4; }
    else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi - _i*4) = MEM32(esi - _i*4); esi -= ecx * 4; edi -= ecx * 4; }
    ecx = 0; /* rep movsd */
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(8) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 8 (32-bit) */
    MEM32(esp + 0x1C) = edx;
    MEM32(esp + 0x20) = 1;
    MEM32(esp + 0x24) = 0x10;
    if (CMP_AE(_fa, _fb)) goto loc_00177A94; /* jae: above or equal (unsigned >=) */

loc_00177A77: ;
    edi = eax + eax * 2;
    edi = edi << 5;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(0xC0CBF0) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edi = edi + 0xC0CBF0;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx = 0x18;
    esi = esp + 0x10;
    if (!g_df) { uint8_t *_d = (uint8_t*)XBOX_PTR(edi), *_s = (uint8_t*)XBOX_PTR(esi); uint32_t _n = ecx * 4;
      if (_d + _n <= _s || _s + _n <= _d) memcpy(_d, _s, _n);
      else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*4) = MEM32(esi + _i*4); }
      esi += ecx * 4; edi += ecx * 4; }
    else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi - _i*4) = MEM32(esi - _i*4); esi -= ecx * 4; edi -= ecx * 4; }
    ecx = 0; /* rep movsd */
    MEM32(0xC0CEF0) = MEM32(0xC0CEF0) + 1;
    _fa = (uint32_t)(MEM32(0xC0CEF0)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_00177A94: ;
    _fa = (uint32_t)(MEM32(ebx + 0x40)) & 0xFFFFFFFFu; _fb = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(ebx + 0x40), edx (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00177AAA; /* je: equal / zero */

loc_00177A99: ;
    MEM32(ebx + 0x44) = edx;
    MEM32(ebx + 0x48) = 0;
    MEM32(ebx + 0x4C) = 0x14;

loc_00177AAA: ;
    POP32(esp, edi);
    POP32(esp, esi);
    esp = ebp;
    POP32(esp, ebp);
    esp += 4; return; /* ret */

}

