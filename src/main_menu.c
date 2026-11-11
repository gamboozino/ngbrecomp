/*
 * main_menu.c -- the main menu without Ranking.
 *
 * Ranking's scoreboards were Xbox Live's, long gone, and choosing it
 * leaves the game for the dashboard's account setup (dashupdate.xbe), which
 * is not here: the program ends. Its entry in the main menu's list is
 * hidden and not selectable (the list's entries: 8 bytes, byte 4 enabled,
 * byte 5 shown), as the game hides the entries it does not offer. The menu's drawer (sub_0017A010) is the
 * lifted one renamed, behind a wrapper; its list is at ebx.
 *
 * Options > General Settings without Flag Select (the country Ranking
 * wanted): the menu engine's General page (page 1 of the Options menu)
 * loses that row, the rows after it move up, and the page's drawer
 * (sub_0021C660, lifted, changes marked "no flag") draws six rows, Default
 * Settings and Exit in the flag's place and after. NGB_KEEP_RANKING=1
 * keeps both.
 *
 * Ranking's place in the list is "RECOMP SETTINGS" instead: its text in
 * the menu's text table (0x436184: id, English, Japanese) is ours, and
 * choosing it (the main menu's A handler, sub_0017B010, with the cursor at
 * 0x94 in the menu object) opens the Options menu as OPTIONS does (its id,
 * 7, at 0x90) but on the recomp's settings (src/pc_menu.c, RECOMP
 * SETTINGS), instead of Ranking's flag question and the dashboard's
 * account setup.
 *
 * The last entry is EXIT TO DESKTOP, behind an Are you sure? box
 * (src/pause_tab.c): OPTIONS moves up into COMING ATTRACTIONS' place (Xbox
 * Live's trailers, gone; it is never shown) -- its text there, and its
 * choice made as OPTIONS' (id 7) -- and OPTIONS' place has the new words.
 */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xbox_memory_layout.h"

/* A debugging option from the environment, read once (these are asked per sprite or per frame). */
#define ENV_ONCE(name) (__extension__({ static const char *v_ = (const char *)1; \
    if (v_ == (const char *)1) v_ = getenv(name); v_; }))


static void items_draw(void);
static int s_back_to = -1;   /* the entry to return to from the Options menu (OPTIONS, RECOMP SETTINGS) */
static int s_no_flag;

/* Before the game starts (after src/pc_menu.c has its page table). */
void ngb_recomp_menu_enter(void);   /* src/pc_menu.c */
void ngb_recomp_menu_tick(void);

void ngb_general_page_init(void)
{
    if (getenv("NGB_KEEP_RANKING"))
        return;
    {   /* COMING ATTRACTIONS' text (0x4361AC: id 0x9000C) OPTIONS', OPTIONS' ours */
        static const char t[] = "EXIT TO DESKTOP";
        uint32_t va = xbox_HeapAlloc(sizeof t, 4);
        MEM32(0x4361B0) = MEM32(0x4361BC);
        MEM32(0x4361B4) = MEM32(0x4361C0);
        if (va) {
            memcpy((void *)XBOX_PTR(va), t, sizeof t);
            MEM32(0x4361BC) = va;
            MEM32(0x4361C0) = va;
        }
    }
    {   /* Ranking's text: "RECOMP SETTINGS" (English and Japanese) */
        static const char t[] = "RECOMP SETTINGS";
        uint32_t va = xbox_HeapAlloc(sizeof t, 4);
        if (va) {
            memcpy((void *)XBOX_PTR(va), t, sizeof t);
            MEM32(0x43618C) = va;
            MEM32(0x436190) = va;
        }
    }
    uint32_t pages = MEM32(0x42A28C);
    for (uint32_t p = pages; MEM8(p) != 0xFF; p += 0x18) {
        if (MEM8(p) != 1 || MEM8(p + 2) != 6)
            continue;
        uint32_t src = MEM32(p + 0xC), dst = xbox_HeapAlloc(0x1C * 8, 4);
        int n = 0;
        for (int i = 0; MEM8(src + 0x1C * i) != 0xFF; i++) {
            if (i == 4)
                continue;                              /* Flag Select */
            memcpy((void *)XBOX_PTR(dst + 0x1C * n), (void *)XBOX_PTR(src + 0x1C * i), 0x1C);
            MEM8(dst + 0x1C * n) = (uint8_t)n;
            n++;
        }
        memset((void *)XBOX_PTR(dst + 0x1C * n), 0, 0x1C);
        MEM8(dst + 0x1C * n) = 0xFF;
        MEM32(p + 0xC) = dst;
        MEM8(p + 2) = 5;
        s_no_flag = 1;
    }
}

int ngb_general_no_flag(void) { return s_no_flag; }

/* The game's flag check (sub_001DB520, src/cheats.c), the registers kept. */
static int game_flag_set(uint32_t flag)
{
    uint32_t sa = eax, sb = ebx, sc = ecx, sd = edx, ss = esi, sdi = edi;
    ecx = flag;
    PUSH32(esp, 0);
    sub_001DB520();
    int r = (eax & 0xFF) != 0;
    eax = sa; ebx = sb; ecx = sc; edx = sd; esi = ss; edi = sdi;
    return r;
}

/* The unlocks (RECOMP SETTINGS > Cheats > Unlocks): MISSIONS, ARCADE NINJA GAIDEN and MOVIE THEATER, the
 * list's entries 2, 4 and 5. The game sets each entry from its flag as the main menu comes up; a setting
 * changed since (in RECOMP SETTINGS, over this menu) shows or hides the entry at once, as the flag now
 * reads -- set with the setting on, as earned with it off. */
static void unlocks_tick(uint32_t list)
{
    extern int ngb_unlock(int which);                  /* src/cheats.c */
    static const int entry[3] = { 2, 4, 5 };
    static int last[3] = { -1, -1, -1 };
    for (int i = 0; i < 3; i++) {
        int on = ngb_unlock(i);
        if (on == last[i])
            continue;
        uint32_t e = list + 8 * (uint32_t)entry[i];
        if (on || last[i] >= 0)                        /* off from the start: the game's own */
            MEM8(e + 4) = MEM8(e + 5) = (uint8_t)game_flag_set(MEM32(e));
        last[i] = on;
    }
}

void sub_0017A010(void)
{
    uint32_t list = MEM32(ebx), n = MEM32(ebx + 4);
    if (n == 8) {
        extern void ngb_main_menu_seen(void);          /* src/ingame_menu.c */
        ngb_recomp_menu_tick();                        /* back from RECOMP SETTINGS */
        ngb_main_menu_seen();                          /* (after Return to Main Menu) */
        extern void ngb_version_seen(void);            /* src/fps_text.c: the version in the corner (#207) */
        ngb_version_seen();
    }
    if (ENV_ONCE("NGB_MENU_LOG") && n < 32) {
        static unsigned k;
        if (k++ % 300 == 0) {
            fprintf(stderr, "[MAINMENU] list %08X n %u:", list, n);
            for (uint32_t i = 0; i < n; i++)
                fprintf(stderr, " [%08X %02X %02X %02X %02X]", MEM32(list + 8 * i), MEM8(list + 8 * i + 4),
                        MEM8(list + 8 * i + 5), MEM8(list + 8 * i + 6), MEM8(list + 8 * i + 7));
            fprintf(stderr, "\n");
        }
    }
    /* The main menu's list (NEW GAME, LOAD GAME, MISSIONS, RANKING, ...,
     * OPTIONS: eight): Ranking not shown. */
    if (n == 8 && ENV_ONCE("NGB_MENU_LOG")) {             /* the cursor (menu + 0x94), as it moves */
        static uint32_t last = ~0u;
        if (MEM32(ebx + 0x54) != last)
            fprintf(stderr, "[MAINMENU] cursor %u\n", last = MEM32(ebx + 0x54));
    }
    /* Back from the Options menu the game puts the cursor where OPTIONS was,
     * the last entry -- now EXIT TO DESKTOP: back on the entry chosen. */
    if (n == 8 && s_back_to >= 0 && MEM32(ebx + 0x54) == 7 && !ENV_ONCE("NGB_KEEP_RANKING")) {
        MEM32(ebx + 0x50) = (uint32_t)s_back_to;    /* where the highlight is drawn */
        MEM32(ebx + 0x54) = (uint32_t)s_back_to;
        if (MEM32(esp + 4) == 7)                    /* this frame's too: its caller passed the cursor */
            MEM32(esp + 4) = (uint32_t)s_back_to;
        s_back_to = -1;
    }
    if (n == 8 && ENV_ONCE("NGB_MENU_FORCE"))            /* a test: all unlocked */
        for (uint32_t i = 0; i < 8; i++)
            MEM8(list + 8 * i + 4) = MEM8(list + 8 * i + 5) = 1;
    else if (n == 8)
        unlocks_tick(list);
    if (n == 8 && !ENV_ONCE("NGB_KEEP_RANKING")) {
        MEM8(list + 8 * 3 + 4) = MEM8(list + 8 * 3 + 5) = 1;   /* "RECOMP SETTINGS": shown and chosen */
        MEM8(list + 8 * 6 + 4) = MEM8(list + 8 * 6 + 5) = 1;   /* OPTIONS, in COMING ATTRACTIONS' place */
        extern int ngb_main_menu_exit_tick(void);              /* src/pause_tab.c */
        if (ngb_main_menu_exit_tick()) {                       /* the Are you sure? box: */
            esp += 4;                                          /* alone, the entries not drawn */
            return;
        }
    }
    /* The difficulty selection (list 0xC074B8, sub_00178B50's) with Ninja Dog
     * too (src/cheats.c's All Difficulties): five rows, a case the game never
     * shows there. Its rows are centred on one line, spaced by the count
     * (0x45A9BA: 48 for four, 44 for five), and five at 44 reach up under
     * the header: spaced 38, they take the four's height. */
    uint32_t shown = 0;
    for (uint32_t i = 0; i < n && i < 32; i++)
        shown += MEM8(list + 8 * i + 5) != 0;
    if (ENV_ONCE("NGB_LIST_LOG")) {
        static uint32_t lb, ln, ls;
        if (ebx != lb || n != ln || shown != ls)
            fprintf(stderr, "[LIST] %08X n %u shown %u\n", lb = ebx, ln = n, ls = shown);
    }
    /* Kept 38 while that list is up, not only around its drawing: the highlight bar (sub_0017A620) places
     * itself by the same table, read in sub_0017A400, and slipped a row low by Very Hard (#42, found by
     * akirahayabusa). The game's value comes back once another list is drawn. */
    static int s_five;
    static uint16_t s_five_was;
    if (ebx == 0xC074E0 && shown == 5) {
        if (!s_five) {
            s_five_was = MEM16(0x45A9BA + 2 * 5);
            s_five = 1;
        }
        MEM16(0x45A9BA + 2 * 5) = 38;
        items_draw();
        return;
    }
    if (s_five) {
        MEM16(0x45A9BA + 2 * 5) = s_five_was;
        s_five = 0;
    }
    items_draw();
}

/**
 * sub_0017A010
 * Original: 0x0017A010 - 0x0017A246 (566 bytes, 149 insns)
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
static void items_draw(void)
{
    RECOMP_ENTRY(0x0017A010u);
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

loc_0017A010: ;
    eax = MEM32(ebx + 4);
    _fb = (uint32_t)(0x54) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x54;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, ebp);
    ebp = MEM32(esp + 0x5C);
    PUSH32(esp, edi);
    edi = 0; /* xor self */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_0017A223; /* jbe: below or equal (unsigned <=) */

loc_0017A026: ;
    eax = MEM32(ebx);
    SET_LO8(ecx, MEM8(eax + edi * 8 + 5));
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0017A217; /* je: equal / zero */

loc_0017A034: ;
    ecx = esp + 8;
    PUSH32(esp, ecx);
    PUSH32(esp, edi);
    PUSH32(esp, ebx);
    PUSH32(esp, esi);
    PUSH32(esp, 0x0017A041u); RECOMP_ABI_CALL(0x0017A400u, sub_0017A400); /* call 0x0017A400 */

loc_0017A041: ;
    edx = MEM32(ebx);
    SET_LO8(eax, MEM8(edx + edi * 8 + 6));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0017A0E7; /* je: equal / zero */

loc_0017A04F: ;
    eax = MEM32(esi + 0x20);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0017A0E7; /* je: equal / zero */

loc_0017A05A: ;
    eax = MEM32(esi + 0x24);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x24) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x24 (32-bit) */
    if (CMP_GE(_fas, _fbs)) goto loc_0017A090; /* jge: greater or equal (signed >=) */

loc_0017A062: ;
    xmm1 = XMM_ZERO(); /* xorps self = zero */
    xmm0.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x41534C); /* mulss */
    _fca = xmm1.f[0]; _fcb = xmm0.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_0017A07B; /* jbe: below or equal (unsigned <=) (xmm1.f[0] vs xmm0.f[0]) */

loc_0017A076: ;
    xmm0 = xmm1; /* movaps */
    goto loc_0017A0C9;

loc_0017A07B: ;
    xmm1 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    _fca = xmm0.f[0]; _fcb = xmm1.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_0017A0C9; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs xmm1.f[0]) */

loc_0017A088: ;
    MEMF(esp + 0x1C) = xmm1.f[0]; /* movss */
    goto loc_0017A0CF;

loc_0017A090: ;
    xmm2 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    ecx = eax + -36;
    xmm1.f[0] = (float)(int32_t)ecx; /* cvtsi2ss */
    xmm1.f[0] = xmm1.f[0] * MEMF(0x414C24); /* mulss */
    xmm0 = xmm2; /* movaps */
    xmm0.f[0] = xmm0.f[0] - xmm1.f[0]; /* subss */
    xmm1 = XMM_SCALAR(MEMF(0x42895C)); /* movss */
    _fca = xmm1.f[0]; _fcb = xmm0.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_0017A0BE; /* jbe: below or equal (unsigned <=) (xmm1.f[0] vs xmm0.f[0]) */

loc_0017A0BB: ;
    xmm0 = xmm1; /* movaps */

loc_0017A0BE: ;
    _fca = xmm0.f[0]; _fcb = xmm2.f[0]; /* comiss */
    MEMF(esp + 0x1C) = xmm2.f[0]; /* movss */
    if ((_fca > _fcb)) goto loc_0017A0CF; /* ja: above (unsigned >) (xmm0.f[0] vs xmm2.f[0]) */

loc_0017A0C9: ;
    MEMF(esp + 0x1C) = xmm0.f[0]; /* movss */

loc_0017A0CF: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x14) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x14 (32-bit) */
    if (CMP_G(_fas, _fbs)) goto loc_0017A20A; /* jg: greater (signed >) */

loc_0017A0D8: ;
    PUSH32(esp, esi);
    eax = esp + 0xC;
    PUSH32(esp, 0x0017A0E2u); RECOMP_ABI_CALL(0x0017A310u, sub_0017A310); /* call 0x0017A310 */

loc_0017A0E2: ;
    goto loc_0017A20A;

loc_0017A0E7: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(ebp) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, ebp (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0017A1D6; /* jne: not equal / not zero */

loc_0017A0EF: ;
    eax = MEM32(esi + 0x20);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    eax = 0x42;
    if (CMP_EQ(_fa, _fb)) goto loc_0017A0FE; /* je: equal / zero */

loc_0017A0FB: ;
    eax = MEM32(esi + 0x24);

loc_0017A0FE: ;
    xmm1 = XMM_ZERO(); /* xorps self = zero */
    _fb = (uint32_t)(0xFFFFFFDCu) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0xFFFFFFDCu;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    xmm0.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x414C24); /* mulss */
    _fca = xmm1.f[0]; _fcb = xmm0.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_0017A11A; /* jbe: below or equal (unsigned <=) (xmm1.f[0] vs xmm0.f[0]) */

loc_0017A115: ;
    xmm0 = xmm1; /* movaps */
    goto loc_0017A12D;

loc_0017A11A: ;
    xmm1 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    _fca = xmm0.f[0]; _fcb = xmm1.f[0]; /* comiss */
    MEMF(esp + 0x60) = xmm1.f[0]; /* movss */
    if ((_fca > _fcb)) goto loc_0017A133; /* ja: above (unsigned >) (xmm0.f[0] vs xmm1.f[0]) */

loc_0017A12D: ;
    MEMF(esp + 0x60) = xmm0.f[0]; /* movss */

loc_0017A133: ;
    fp_push(MEMF(0x428960)); /* fld float */
    eax = MEM32(esi + 0x14);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(3) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 3 (32-bit) */
    fp_top() = RECOMP_FP_PC(fp_top() - MEMF(0x42895C)); /* fsub dword ptr [0x42895c] */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x60)); /* fmul dword ptr [esp + 0x60] */
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x42895C)); /* fadd dword ptr [0x42895c] */
    fp_push((double)SMEM32(0xD21734)); /* fild */
    fp_top() = RECOMP_FP_PC(fp_top() / (double)SMEM32(0x428968)); /* fidiv dword ptr [0x428968] */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x414944)); /* fmul dword ptr [0x414944] */
    fp_top() = sin(fp_top()); /* fsin */
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C8)); /* fadd dword ptr [0x4148c8] */
    fp_push(MEMF(0x428964)); /* fld float */
    fp_top() = RECOMP_FP_PC(fp_top() - g_fp_stack[(g_fp_top + 2) & 7]); /* fsub st(2) */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x60)); /* fmul dword ptr [esp + 0x60] */
    fp_top() = RECOMP_FP_PC(fp_top() + g_fp_stack[(g_fp_top + 2) & 7]); /* fadd st(2) */
    fp_top() = RECOMP_FP_PC(fp_top() - g_fp_stack[(g_fp_top + 2) & 7]); /* fsub st(2) */
    fp_st1() = RECOMP_FP_PC(fp_st1() * fp_top()); fp_pop(); /* fmulp st(1) */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x4148C0)); /* fmul dword ptr [0x4148c0] */
    fp_top() = RECOMP_FP_PC(fp_top() + fp_st1()); /* fadd st(1) */
    MEMF(esp + 0x1C) = (float)fp_top(); fp_pop(); /* fstp */
    fp_pop(); /* fstp st(0) */
    if (CMP_NE(_fa, _fb)) goto loc_0017A1B1; /* jne: not equal / not zero */

loc_0017A18B: ;
    eax = esi;
    PUSH32(esp, 0x0017A192u); RECOMP_ABI_CALL(0x00175680u, sub_00175680); /* call 0x00175680 */

loc_0017A192: ;
    xmm2 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    xmm1 = xmm2; /* movaps */
    xmm1.f[0] = xmm1.f[0] - xmm0.f[0]; /* subss */
    xmm3 = xmm1; /* movaps */
    xmm3.f[0] = xmm3.f[0] * xmm1.f[0]; /* mulss */
    xmm0 = xmm2; /* movaps */
    xmm0.f[0] = xmm0.f[0] - xmm3.f[0]; /* subss */
    goto loc_0017A1FE;

loc_0017A1B1: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(2) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 2 (32-bit) */
    eax = esi;
    if (CMP_NE(_fa, _fb)) goto loc_0017A1F9; /* jne: not equal / not zero */

loc_0017A1B8: ;
    PUSH32(esp, 0x0017A1BDu); RECOMP_ABI_CALL(0x00175680u, sub_00175680); /* call 0x00175680 */

loc_0017A1BD: ;
    xmm1 = xmm0; /* movaps */
    xmm1.f[0] = xmm1.f[0] * xmm0.f[0]; /* mulss */
    xmm1.f[0] = xmm1.f[0] * xmm0.f[0]; /* mulss */
    xmm1.f[0] = xmm1.f[0] * MEMF(esp + 0x1C); /* mulss */
    MEMF(esp + 0x1C) = xmm1.f[0]; /* movss */
    goto loc_0017A20A;

loc_0017A1D6: ;
    eax = MEM32(esi + 0x14);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(3) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 3 (32-bit) */
    xmm0 = XMM_SCALAR(MEMF(0x42895C)); /* movss */
    MEMF(esp + 0x1C) = xmm0.f[0]; /* movss */
    if (CMP_NE(_fa, _fb)) goto loc_0017A1B1; /* jne: not equal / not zero */

loc_0017A1EC: ;
    eax = esi;
    PUSH32(esp, 0x0017A1F3u); RECOMP_ABI_CALL(0x00175680u, sub_00175680); /* call 0x00175680 */

loc_0017A1F3: ;
    xmm0.f[0] = xmm0.f[0] * xmm0.f[0]; /* mulss */
    goto loc_0017A1FE;

loc_0017A1F9: ;
    PUSH32(esp, 0x0017A1FEu); RECOMP_ABI_CALL(0x00175680u, sub_00175680); /* call 0x00175680 */

loc_0017A1FE: ;
    xmm0.f[0] = xmm0.f[0] * MEMF(esp + 0x1C); /* mulss */
    MEMF(esp + 0x1C) = xmm0.f[0]; /* movss */

loc_0017A20A: ;
    edx = esp + 8;
    PUSH32(esp, edx);
    PUSH32(esp, 0x0017A214u); RECOMP_ABI_CALL(0x001E6D30u, sub_001E6D30); /* call 0x001E6D30 */

loc_0017A214: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_0017A217: ;
    eax = MEM32(ebx + 4);
    edi++;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, eax (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_0017A026; /* jb: below (unsigned <) */

loc_0017A223: ;
    POP32(esp, edi);
    MEM32(0xC0CBE4) = 0x1E0;
    MEM32(0xC0CBE0) = 4;
    MEM8(0xC0CBE8) = 1;
    POP32(esp, ebp);
    _fb = (uint32_t)(0x54) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x54;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 8; return; /* ret 4 */

    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}


/**
 * sub_0021C660
 * Original: 0x0021C660 - 0x0021CD87 (1831 bytes, 397 insns)
 * Category: game_vtable
 * CC: thiscall, 0 params, returns int_or_void
 * Frame: standard_frame
 */
void sub_0021C660(void)
{
    RECOMP_ENTRY(0x0021C660u);
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

loc_0021C660: ;
    PUSH32(esp, ebp);
    ebp = esp;
    g_ebp = ebp; /* publish frame for frameless callees */
    g_seh_ebp = ebp;
    _cf = 0; /* logical op clears CF */
    esp = esp & 0xFFFFFFF0u;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fb = (uint32_t)(0x214) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(esp) < (uint32_t)(0x214));
    esp = esp - 0x214;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    eax = MEM32(0x41CCD8);
    MEM32(esp + 0x210) = eax;
    PUSH32(esp, ebx);
    eax = 0x20017;
    PUSH32(esp, esi);
    MEM32(esp + 0x94) = eax;
    MEM32(esp + 0x9C) = eax;
    eax = MEM32(ebp + 8);
    PUSH32(esp, edi);
    PUSH32(esp, eax);
    edx = 0x8DE4FC;
    MEM32(esp + 0xA8) = 0x20006;
    MEM32(esp + 0xAC) = 0x20001;
    MEM32(esp + 0xB0) = 0x20002;
    MEM32(esp + 0xB4) = 0x20003;
    MEM32(esp + 0xB8) = s_no_flag ? 0x20004 : 0x20007;   /* no flag: Default, Exit up */
    MEM32(esp + 0xBC) = s_no_flag ? 0x20005 : 0x20004;
    MEM32(esp + 0xC0) = 0x20005;
    MEM32(esp + 0x84) = 0x2001C;
    MEM32(esp + 0x88) = 0x2001B;
    MEM32(esp + 0x8C) = 0x2001D;
    MEM32(esp + 0x90) = 0x2001E;
    MEM32(esp + 0x94) = 0x2001F;
    MEM32(esp + 0x98) = 0x20020;
    MEM32(esp + 0xA0) = 0x20016;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021C736u); RECOMP_ABI_CALL(0x0021C0F0u, sub_0021C0F0); /* call 0x0021C0F0 */

loc_0021C736: ;
    ecx = ZX8(MEM8(0xDB9ADA));
    MEM32(esp + 0x68) = eax;
    eax = MEM32(0xBCE018);
    edx = ((int32_t)eax < 0) ? 0xFFFFFFFF : 0; /* cdq */
    esi = ecx + ecx;
    { int64_t _dividend = ((int64_t)(int32_t)edx << 32) | eax;
      eax = (uint32_t)((int32_t)(_dividend / (int32_t)esi));
      edx = (uint32_t)((int32_t)(_dividend % (int32_t)esi)); }
    xmm2 = XMM_SCALAR(MEMF(0x414920)); /* movss */
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    xmm0.f[0] = (float)(int32_t)ecx; /* cvtsi2ss */
    xmm1 = xmm2; /* movaps */
    xmm1.f[0] = xmm1.f[0] / xmm0.f[0]; /* divss */
    eax = edx;
    _fb = (uint32_t)(ecx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(eax) < (uint32_t)(ecx));
    eax = eax - ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_LE(_fas, _fbs)) goto loc_0021C76E; /* jle: less or equal (signed <=) */

loc_0021C76A: ;
    edx = eax;
    goto loc_0021C772;

loc_0021C76E: ;
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(ecx) < (uint32_t)(edx));
    ecx = ecx - edx;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    edx = ecx;

loc_0021C772: ;
    ecx = esp + 0x50;
    MEM32(esp + 0x1C) = ecx;
    eax = 0x43A0B0;
    ecx = MEM32(esp + 0x1C);
    xmm0 = XMM_MEM(eax); /* movaps */
    XMM_STORE(ecx, xmm0); /* movaps */
    xmm0.f[0] = (float)(int32_t)edx; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * xmm1.f[0]; /* mulss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x3EC1C8); /* mulss */
    xmm0.f[0] = xmm0.f[0] + MEMF(0x4148D0); /* addss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x41492C); /* mulss */
    edx = (int32_t)recomp_fist((double)xmm0.f[0], 0x0E7Fu, 32); /* cvttss2si */
    xmm0.f[0] = (float)(int32_t)edx; /* cvtsi2ss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x4148E0); /* mulss */
    xmm0.f[0] = xmm0.f[0] * xmm2.f[0]; /* mulss */
    MEMF(esp + 0x5C) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    ebx = 0x70;
    MEMF(esp + 0x40) = xmm0.f[0]; /* movss */
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    MEM32(0x43B480) = 1;
    MEM32(esp + 0x18) = ebx;
    MEM32(esp + 0x14) = 0;
    MEM8(esp + 0x2F) = 1;
    MEM8(esp + 0x30) = 1;
    MEMF(esp + 0x48) = xmm0.f[0]; /* movss */
    /* nop */

loc_0021C800: ;
    eax = MEM32(ebp + 8);
    esi = MEM32(eax + 0x10);
    edx = MEM32(esp + 0x14);
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(5) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 5 (32-bit) */
    _cf = (int)(_fa < _fb);
    ecx = _cf ? 0xFFFFFFFF : 0; /* sbb self (CF extend) */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sbb result */
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    edi = esp + 0x10;
    esi = esp + 0x1C;
    eax = 0x8DE4FC;
    MEM32(0x43B47C) = ecx;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021C828u); RECOMP_ABI_CALL(0x0021C380u, sub_0021C380); /* call 0x0021C380 */

loc_0021C828: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    edi = MEM32(esp + 0x1C);
    eax = MEM32(0x43B490);
    MEMF(esp + 0x3C) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x415424)); /* movss */
    _fb = (uint32_t)(0x116) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(edi) + (uint64_t)(0x116)) >> 32) & 1);
    edi = edi + 0x116;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 1 (32-bit) */
    _cf = (int)(_fa < _fb);
    MEMF(esp + 0x38) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x10)); /* movss */
    MEM32(esp + 0x24) = 0x246;
    MEM16(esp + 0x28) = LO16(edi);
    MEM16(esp + 0x2A) = LO16(ebx);
    MEM16(esp + 0x2C) = 0x1C2;
    MEM8(esp + 0x2E) = 0;
    MEM32(esp + 0x34) = 0;
    MEM32(esp + 0x4C) = 0xFFFFFFFFu;
    MEMF(esp + 0x44) = xmm0.f[0]; /* movss */
    if (CMP_NE(_fa, _fb)) goto loc_0021C8EB; /* jne: not equal / not zero */

loc_0021C892: ;
    ecx = MEM32(0x8F7DFC);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(0x8F7DF8)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, MEM32(0x8F7DF8) (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_AE(_fa, _fb)) goto loc_0021C8F1; /* jae: above or equal (unsigned >=) */

loc_0021C8A0: ;
    eax = MEM32(0x8F7DF4);
    esi = ecx;
    esi = (uint32_t)((int32_t)esi * (int32_t)0x70);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esi) + (uint64_t)(eax)) >> 32) & 1);
    esi = esi + eax;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = MEM32(0x8F7DF0);
    if (4) _cf = (int)(((ecx) >> (32 - (4))) & 1);
    ecx = ecx << 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, 0);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(ecx) + (uint64_t)(eax)) >> 32) & 1);
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, 0);
    eax = esp + 0x2C;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021C8C3u); RECOMP_ABI_CALL(0x002216B0u, sub_002216B0); /* call 0x002216B0 */

loc_0021C8C3: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_0021C8EB; /* je: equal / zero */

loc_0021C8CA: ;
    ecx = MEM32(0x8F7DFC);
    eax = 0x1C2;
    _cf = 0; /* xor clears CF */
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021C8DCu); RECOMP_ABI_CALL(0x00221170u, sub_00221170); /* call 0x00221170 */

loc_0021C8DC: ;
    ecx = MEM32(0x8F7DFC);
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    MEM32(0x8F7DFC) = ecx;
    goto loc_0021C8F1;

loc_0021C8EB: ;
    ecx = MEM32(0x8F7DFC);

loc_0021C8F1: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    _fa = (uint32_t)(MEM32(0x43B490)) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0x43B490), 1 (32-bit) */
    _cf = (int)(_fa < _fb);
    edx = edi + 0x185;
    MEMF(esp + 0x38) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x3C) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x10)); /* movss */
    MEM32(esp + 0x24) = 0x247;
    MEM16(esp + 0x28) = LO16(edx);
    MEM16(esp + 0x2A) = LO16(ebx);
    MEM16(esp + 0x2C) = 0x1C2;
    MEM8(esp + 0x2E) = 0;
    MEM32(esp + 0x34) = 0;
    MEM32(esp + 0x4C) = 0xFFFFFFFFu;
    MEMF(esp + 0x44) = xmm0.f[0]; /* movss */
    if (CMP_NE(_fa, _fb)) goto loc_0021C998; /* jne: not equal / not zero */

loc_0021C94E: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(0x8F7DF8)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, MEM32(0x8F7DF8) (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_AE(_fa, _fb)) goto loc_0021C998; /* jae: above or equal (unsigned >=) */

loc_0021C956: ;
    eax = MEM32(0x8F7DF4);
    esi = ecx;
    esi = (uint32_t)((int32_t)esi * (int32_t)0x70);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esi) + (uint64_t)(eax)) >> 32) & 1);
    esi = esi + eax;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = MEM32(0x8F7DF0);
    if (4) _cf = (int)(((ecx) >> (32 - (4))) & 1);
    ecx = ecx << 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    PUSH32(esp, 0);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(ecx) + (uint64_t)(eax)) >> 32) & 1);
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, 0);
    eax = esp + 0x2C;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021C979u); RECOMP_ABI_CALL(0x002216B0u, sub_002216B0); /* call 0x002216B0 */

loc_0021C979: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(8)) >> 32) & 1);
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_0021C998; /* je: equal / zero */

loc_0021C980: ;
    ecx = MEM32(0x8F7DFC);
    eax = 0x1C2;
    _cf = 0; /* xor clears CF */
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021C992u); RECOMP_ABI_CALL(0x00221170u, sub_00221170); /* call 0x00221170 */

loc_0021C992: ;
    MEM32(0x8F7DFC) = MEM32(0x8F7DFC) + 1;
    _fa = (uint32_t)(MEM32(0x8F7DFC)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_0021C998: ;
    esi = MEM32(esp + 0x14);
    PUSH32(esp, 2);
    eax = SX16(LO16(ebx));
    _fb = (uint32_t)(0x13) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(0x13)) >> 32) & 1);
    eax = eax + 0x13;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, 0x1C3);
    PUSH32(esp, eax);
    MEM32(esp + 0x74) = eax;
    _cf = 0; /* xor clears CF */
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(ecx, MEM8(0x8DE524));
    eax = SX16(LO16(edi));
    _fb = (uint32_t)(0x23) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(0x23)) >> 32) & 1);
    eax = eax + 0x23;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, eax);
    eax = MEM32(esp + esi * 4 + 0xB4);
    PUSH32(esp, ecx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021C9CAu); RECOMP_ABI_CALL(0x001E6AE0u, sub_001E6AE0); /* call 0x001E6AE0 */

loc_0021C9CA: ;
    ecx = eax;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = esp + 0xD0;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021C9DBu); RECOMP_ABI_CALL(0x00168730u, sub_00168730); /* call 0x00168730 */

loc_0021C9DB: ;
    edx = esp + 0xD0;
    PUSH32(esp, edx);
    eax = esp + 0x84;
    PUSH32(esp, eax);
    MEM8(esp + 0x122) = 0x10;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021C9F8u); RECOMP_ABI_CALL(0x001E86A0u, sub_001E86A0); /* call 0x001E86A0 */

loc_0021C9F8: ;
    ecx = ZX16(MEM16(0x43B488));
    xmm1 = XMM_SCALAR(MEMF(esp + 0x90)); /* movss */
    _fb = (uint32_t)(0x18) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(0x18)) >> 32) & 1);
    esp = esp + 0x18;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    xmm0.f[0] = (float)(int32_t)ecx; /* cvtsi2ss */
    _fca = xmm1.f[0]; _fcb = xmm0.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_0021CA2A; /* jbe: below or equal (unsigned <=) (xmm1.f[0] vs xmm0.f[0]) */

loc_0021CA14: ;
    xmm0.f[0] = xmm0.f[0] / xmm1.f[0]; /* divss */
    xmm0.f[0] = xmm0.f[0] * MEMF(esp + 0xDC); /* mulss */
    MEMF(esp + 0xDC) = xmm0.f[0]; /* movss */

loc_0021CA2A: ;
    xmm0 = XMM_SCALAR(MEMF(esp + 0x10)); /* movss */
    edx = esp + 0xC0;
    PUSH32(esp, edx);
    MEMF(esp + 0xD8) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021CA46u); RECOMP_ABI_CALL(0x001E6D30u, sub_001E6D30); /* call 0x001E6D30 */

loc_0021CA46: ;
    eax = MEM32(ebp + 8);
    ecx = MEM32(eax + 0x10);
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, esi (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_NE(_fa, _fb)) goto loc_0021CB2C; /* jne: not equal / not zero */

loc_0021CA57: ;
    xmm0 = XMM_SCALAR(MEMF(esp + 0xDC)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(esp + 0x78); /* mulss */
    xmm1 = xmm0; /* movaps */
    xmm1.f[0] = xmm1.f[0] * MEMF(0x4148C0); /* mulss */
    xmm1.f[0] = xmm1.f[0] + MEMF(esp + 0x70); /* addss */
    ecx = ebx + 0x11;
    MEM32(esp + 0x24) = 0xBA;
    MEM16(esp + 0x2A) = LO16(ecx);
    MEM16(esp + 0x2C) = 0x1C4;
    MEMF(esp + 0x6C) = xmm1.f[0]; /* movss */
    eax = (int32_t)recomp_fist((double)MEMF(esp + 0x6C), 0x0E7Fu, 32); /* cvttss2si */
    fp_push(MEMF(esp + 0x50)); /* fld float */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x414974); /* mulss */
    MEMF(esp + 0x38) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x7C)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x415240); /* mulss */
    MEM16(esp + 0x28) = LO16(eax);
    MEM8(esp + 0x2E) = 0x11;
    MEMF(esp + 0x3C) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021CACFu); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_0021CACF: ;
    fp_push(MEMF(esp + 0x5C)); /* fld float */
    edi = eax;
    _cf = 0; /* logical op clears CF */
    edi = edi & 0xFF;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021CAE0u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_0021CAE0: ;
    fp_push(MEMF(esp + 0x54)); /* fld float */
    if (8) _cf = (int)(((eax) >> (32 - (8))) & 1);
    eax = eax << 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _cf = 0; /* logical op clears CF */
    edi = edi | eax;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    if (8) _cf = (int)(((edi) >> (32 - (8))) & 1);
    edi = edi << 8;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021CAF1u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_0021CAF1: ;
    fp_push(MEMF(esp + 0x58)); /* fld float */
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
    PUSH32(esp, 0x0021CB04u); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_0021CB04: ;
    xmm0 = XMM_SCALAR(MEMF(esp + 0x10)); /* movss */
    _cf = 0; /* logical op clears CF */
    eax = eax & 0xFF;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _cf = 0; /* logical op clears CF */
    edi = edi | eax;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(esp + 0x4C) = edi;
    edi = esp + 0x24;
    MEMF(esp + 0x44) = xmm0.f[0]; /* movss */
    MEM32(esp + 0x34) = 5;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021CB2Cu); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_0021CB2C: ;
    edx = MEM32(0x43B474);
    eax = MEM32(esp + 0x1C);
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(edx)) >> 32) & 1);
    eax = eax + edx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(4) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 4 (32-bit) */
    _cf = (int)(_fa < _fb);
    MEM32(esp + 0x20) = eax;
    if (CMP_A(_fa, _fb)) goto loc_0021CD58; /* ja: above (unsigned >) */

loc_0021CB45: ;
    if (s_no_flag && esi >= 4) goto loc_0021CD58;    /* no flag: no value */
    { uint32_t _jt = MEM32(esi * 4 + 0x21CD88); /* switch: 5 entries, 5 targets */
    if (_jt == 0x0021CB4Cu) goto loc_0021CB4C;
    if (_jt == 0x0021CBB6u) goto loc_0021CBB6;
    if (_jt == 0x0021CBDAu) goto loc_0021CBDA;
    if (_jt == 0x0021CC00u) goto loc_0021CC00;
    if (_jt == 0x0021CC26u) goto loc_0021CC26;
    g_seh_ebp = ebp; RECOMP_ITAIL(_jt); return; }

loc_0021CB4C: ;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021CB51u); RECOMP_ABI_CALL(0x001DB330u, sub_001DB330); /* call 0x001DB330 */

loc_0021CB51: ;
    esi = eax;
    eax = 0x95217CB1u;
    { uint64_t _r = (uint64_t)eax * (uint64_t)esi;
      eax = (uint32_t)_r; edx = (uint32_t)(_r >> 32); }
    ecx = edx;
    if (0x15) _cf = (int)(((ecx) >> ((0x15) - 1)) & 1);
    ecx = ecx >> 0x15;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edx = ZX16(LO16(ecx));
    edx = (uint32_t)((int32_t)edx * (int32_t)0x36EE80);
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(esi) < (uint32_t)(edx));
    esi = esi - edx;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    eax = 0x45E7B273;
    { uint64_t _r = (uint64_t)eax * (uint64_t)esi;
      eax = (uint32_t)_r; edx = (uint32_t)(_r >> 32); }
    if (0xE) _cf = (int)(((edx) >> ((0xE) - 1)) & 1);
    edx = edx >> 0xE;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fa = (uint32_t)(LO16(ecx)) & 0xFFFFu; _fb = (uint32_t)(0x3E7) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(ecx), 0x3E7 (16-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_BE(_fa, _fb)) goto loc_0021CB83; /* jbe: below or equal (unsigned <=) */

loc_0021CB7B: ;
    SET_LO16(ecx, 0x3E7);
    SET_LO16(edx, 0x3B);

loc_0021CB83: ;
    _fa = (uint32_t)(LO16(edx)) & 0xFFFFu; _fb = (uint32_t)(0x3B) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(edx), 0x3B (16-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_BE(_fa, _fb)) goto loc_0021CB8D; /* jbe: below or equal (unsigned <=) */

loc_0021CB89: ;
    SET_LO16(edx, 0x3B);

loc_0021CB8D: ;
    eax = ZX16(LO16(edx));
    PUSH32(esp, eax);
    ecx = ZX16(LO16(ecx));
    PUSH32(esp, ecx);
    edx = esp + 0x120;
    PUSH32(esp, 0x40FC24);
    PUSH32(esp, edx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021CBA7u); RECOMP_ABI_CALL(0x0036E249u, sub_0036E249); /* call 0x0036E249 */

loc_0021CBA7: ;
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(0x10)) >> 32) & 1);
    esp = esp + 0x10;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ebx = esp + 0x118;
    goto loc_0021CCC2;

loc_0021CBB6: ;
    ecx = MEM32(0xDA4978);
    _cf = 0; /* xor clears CF */
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(eax, MEM8(0x8DE524));
    PUSH32(esp, eax);
    eax = MEM32(esp + ecx * 4 + 0x84);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021CBD0u); RECOMP_ABI_CALL(0x001E6AE0u, sub_001E6AE0); /* call 0x001E6AE0 */

loc_0021CBD0: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ebx = eax;
    goto loc_0021CCBA;

loc_0021CBDA: ;
    eax = ZX8(MEM8(0xDA42F3));
    eax = MEM32(esp + eax * 4 + 0x80);
    _cf = 0; /* xor clears CF */
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(edx, MEM8(0x8DE524));
    PUSH32(esp, edx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021CBF6u); RECOMP_ABI_CALL(0x001E6AE0u, sub_001E6AE0); /* call 0x001E6AE0 */

loc_0021CBF6: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ebx = eax;
    goto loc_0021CCBA;

loc_0021CC00: ;
    edx = ZX8(MEM8(0xDA42E4));
    eax = MEM32(esp + edx * 4 + 0x98);
    _cf = 0; /* xor clears CF */
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(ecx, MEM8(0x8DE524));
    PUSH32(esp, ecx);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021CC1Cu); RECOMP_ABI_CALL(0x001E6AE0u, sub_001E6AE0); /* call 0x001E6AE0 */

loc_0021CC1C: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ebx = eax;
    goto loc_0021CCBA;

loc_0021CC26: ;
    SET_LO16(edi, ZX8(MEM8(0xDA42F1)));
    _cf = 0; /* xor clears CF */
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO8(eax, MEM8(0x8DE524));
    esi = ZX16(LO16(edi));
    PUSH32(esp, eax);
    eax = esi;
    _cf = 0; /* logical op clears CF */
    eax = eax | 0x140000;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021CC45u); RECOMP_ABI_CALL(0x001E6AE0u, sub_001E6AE0); /* call 0x001E6AE0 */

loc_0021CC45: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(LO16(edi)) & 0xFFFFu; _fb = (uint32_t)(0x1B) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(edi), 0x1B (16-bit) */
    _cf = (int)(_fa < _fb);
    ebx = eax;
    if (CMP_AE(_fa, _fb)) goto loc_0021CCBA; /* jae: above or equal (unsigned >=) */

loc_0021CC50: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    edx = MEM32(esp + 0x18);
    SET_LO16(ecx, MEM16(esp + 0x20));
    _fb = (uint32_t)(0x12) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(edx) + (uint64_t)(0x12)) >> 32) & 1);
    edx = edx + 0x12;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEMF(esp + 0x38) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x3C) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x10)); /* movss */
    _fb = (uint32_t)(0x25E) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esi) + (uint64_t)(0x25E)) >> 32) & 1);
    esi = esi + 0x25E;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edi = esp + 0x24;
    MEM32(esp + 0x24) = esi;
    MEM16(esp + 0x28) = LO16(ecx);
    MEM16(esp + 0x2A) = LO16(edx);
    MEM16(esp + 0x2C) = 0x1C2;
    MEM32(esp + 0x34) = 0;
    MEM32(esp + 0x4C) = 0xFFFFFFFFu;
    MEM8(esp + 0x2E) = 0x10;
    MEMF(esp + 0x44) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021CCB5u); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_0021CCB5: ;
    _fb = (uint32_t)(0x28) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(MEM32(esp + 0x20)) + (uint64_t)(0x28)) >> 32) & 1);
    MEM32(esp + 0x20) = MEM32(esp + 0x20) + 0x28;
    _fa = (uint32_t)(MEM32(esp + 0x20)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_0021CCBA: ;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, 0 (32-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (CMP_EQ(_fa, _fb)) goto loc_0021CD50; /* je: equal / zero */

loc_0021CCC2: ;
    ecx = (uint32_t)(int32_t)SMEM16(esp + 0x20);
    eax = MEM32(esp + 0x68);
    PUSH32(esp, 2);
    PUSH32(esp, 0x1C2);
    PUSH32(esp, eax);
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(ecx) < (uint32_t)(4));
    ecx = ecx - 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, ecx);
    ecx = ebx;
    eax = esp + 0xD0;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021CCE5u); RECOMP_ABI_CALL(0x00168730u, sub_00168730); /* call 0x00168730 */

loc_0021CCE5: ;
    edx = esp + 0xD0;
    PUSH32(esp, edx);
    eax = esp + 0x84;
    PUSH32(esp, eax);
    MEM8(esp + 0x122) = 0x10;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021CD02u); RECOMP_ABI_CALL(0x001E86A0u, sub_001E86A0); /* call 0x001E86A0 */

loc_0021CD02: ;
    xmm0 = XMM_SCALAR(MEMF(esp + 0x90)); /* movss */
    xmm1 = XMM_SCALAR(MEMF(0x415418)); /* movss */
    _fb = (uint32_t)(0x18) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(0x18)) >> 32) & 1);
    esp = esp + 0x18;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fca = xmm0.f[0]; _fcb = xmm1.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_0021CD31; /* jbe: below or equal (unsigned <=) (xmm0.f[0] vs xmm1.f[0]) */

loc_0021CD1B: ;
    xmm1.f[0] = xmm1.f[0] / xmm0.f[0]; /* divss */
    xmm1.f[0] = xmm1.f[0] * MEMF(esp + 0xDC); /* mulss */
    MEMF(esp + 0xDC) = xmm1.f[0]; /* movss */

loc_0021CD31: ;
    xmm0 = XMM_SCALAR(MEMF(esp + 0x10)); /* movss */
    ecx = esp + 0xC0;
    PUSH32(esp, ecx);
    MEMF(esp + 0xD8) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021CD4Du); RECOMP_ABI_CALL(0x001E6D30u, sub_001E6D30); /* call 0x001E6D30 */

loc_0021CD4D: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(4)) >> 32) & 1);
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_0021CD50: ;
    ebx = MEM32(esp + 0x18);
    esi = MEM32(esp + 0x14);

loc_0021CD58: ;
    _fb = (uint32_t)(MEM32(0x43B478)) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(ebx) + (uint64_t)(MEM32(0x43B478))) >> 32) & 1);
    ebx = ebx + MEM32(0x43B478);
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esi++;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(s_no_flag ? 6 : 7) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 7 (32-bit) */
    _cf = (int)(_fa < _fb);
    MEM32(esp + 0x18) = ebx;
    MEM32(esp + 0x14) = esi;
    if (CMP_B(_fa, _fb)) goto loc_0021C800; /* jb: below (unsigned <) */

loc_0021CD70: ;
    ecx = MEM32(esp + 0x21C);
    eax = MEM32(esp + 0x64);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0021CD80u); RECOMP_ABI_CALL(0x00379C29u, sub_00379C29); /* call 0x00379C29 */

loc_0021CD80: ;
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

/**
 * sub_0017CC80
 * Original: 0x0017CC80 - 0x0017CD31 (177 bytes, 43 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_0017CC80(void)
{
    RECOMP_ENTRY(0x0017CC80u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_0017CC80: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_L(_fas, _fbs)) goto loc_0017CD21; /* jl: less (signed <) */

loc_0017CC88: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(2) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 2 (32-bit) */
    if (CMP_G(_fas, _fbs)) goto loc_0017CD21; /* jg: greater (signed >) */

loc_0017CC91: ;
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    PUSH32(esp, 0x0017CC98u); RECOMP_ABI_CALL(0x0031F930u, sub_0031F930); /* call 0x0031F930 */

loc_0017CC98: ;
    edx = MEM32(0x403FEC);
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    ecx = 0x300;
    edi = 0xDA2DC0;
    { uint32_t _i; int32_t _st = RECOMP_DF_STEP(4); for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*_st) = eax; edi += ecx * _st; }
    ecx = 0; /* rep stosd */
    ecx = MEM32(0x403FE8);
    eax = MEM32(0x403FE4);
    MEM32(0xDA3AC4) = ecx;
    SET_LO8(ecx, MEM8(0x403FF2));
    MEM32(0xDA3AC0) = eax;
    SET_LO16(eax, MEM16(0x403FF0));
    MEM8(0xDA3ACE) = LO8(ecx);
    MEM16(0xDA3ACC) = LO16(eax);
    eax = MEM32(0xDB9890);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    ecx = 0x87;
    esi = 0xDA3AC0;
    edi = 0xDA2FE4;
    MEM32(0xDA2FE0) = 2;
    MEM32(0xDA3AC8) = edx;
    if (!g_df) { uint8_t *_d = (uint8_t*)XBOX_PTR(edi), *_s = (uint8_t*)XBOX_PTR(esi); uint32_t _n = ecx * 4;
      if (_d + _n <= _s || _s + _n <= _d) memcpy(_d, _s, _n);
      else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*4) = MEM32(esi + _i*4); }
      esi += ecx * 4; edi += ecx * 4; }
    else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi - _i*4) = MEM32(esi - _i*4); esi -= ecx * 4; edi -= ecx * 4; }
    ecx = 0; /* rep movsd */
    POP32(esp, edi);
    POP32(esp, esi);
    PUSH32(esp, 0xDA2DC0);
    if (CMP_EQ(_fa, _fb)) goto loc_0017CD16; /* je: equal / zero */

loc_0017CD0B: ;
    PUSH32(esp, 0x403FD0);
    PUSH32(esp, 0x0017CD15u); RECOMP_ABI_CALL(0x00328358u, sub_00328358); /* call 0x00328358 */

loc_0017CD15: ;
    esp += 4; return; /* ret */

loc_0017CD16: ;
    PUSH32(esp, 0x403FBC);
    PUSH32(esp, 0x0017CD20u); RECOMP_ABI_CALL(0x00328358u, sub_00328358); /* call 0x00328358 */

loc_0017CD20: ;
    esp += 4; return; /* ret */

loc_0017CD21: ;
    PUSH32(esp, 3);
    PUSH32(esp, 0x0017CD28u); RECOMP_ABI_CALL(0x0017E6C0u, sub_0017E6C0); /* call 0x0017E6C0 */

loc_0017CD28: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM8(0xDA41DE) = LO8(eax);
    esp += 4; return; /* ret */

}

void sub_001E3400(void);
static void main_menu_select(void);

/* The main menu's choice: "RECOMP SETTINGS" (Ranking's place) opens the
 * Options menu on the recomp's settings. */
#define OPTIONS_ID 7
void sub_0017B010(void)
{
    if (getenv("NGB_MENU_LOG"))
        fprintf(stderr, "[MAINMENU] chosen: entry %u id %u\n", MEM32(esi + 0x94), MEM32(esi + 0x90));
    if (esi == 0xC075C0 && MEM32(esi + 0x94) == 3 && !getenv("NGB_KEEP_RANKING")) {
        s_back_to = 3;
        ngb_recomp_menu_enter();
        MEM32(esi + 0x90) = OPTIONS_ID;
    }
    if (esi == 0xC075C0 && MEM32(esi + 0x94) == 6 && !getenv("NGB_KEEP_RANKING")) {
        s_back_to = 6;
        MEM32(esi + 0x90) = OPTIONS_ID;                 /* OPTIONS, moved up */
    }
    if (esi == 0xC075C0 && MEM32(esi + 0x94) == 7 && !getenv("NGB_KEEP_RANKING")) {
        extern void ngb_main_menu_exit(void);           /* EXIT TO DESKTOP: Are you sure? */
        uint32_t sp = esp;
        PUSH32(esp, 0x40);                              /* the menu's "chosen" sound */
        PUSH32(esp, 6);
        PUSH32(esp, 0);
        sub_001E3400();
        esp = sp;
        ngb_main_menu_exit();
        esp += 4;   /* ret */
        return;
    }
    /* The bar slides from the chosen id (+0x90) while the choice flashes, and the Options menu's id, 7,
     * is EXIT TO DESKTOP's row now: once the game has read it, the row chosen again, or the highlight
     * flashed on EXIT TO DESKTOP (issue #27, found by akirahayabusa). Only this function reads +0x90. */
    int moved = esi == 0xC075C0 && MEM32(esi + 0x90) == OPTIONS_ID && MEM32(esi + 0x94) != OPTIONS_ID &&
                s_back_to >= 0;
    main_menu_select();
    if (moved && MEM32(0xC07650) == OPTIONS_ID)
        MEM32(0xC07650) = (uint32_t)s_back_to;
}

/**
 * sub_0017B010
 * Original: 0x0017B010 - 0x0017B255 (581 bytes, 143 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
static void main_menu_select(void)
{
    RECOMP_ENTRY(0x0017B010u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_0017B010: ;
    _fb = (uint32_t)(0x2C) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x2C;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    PUSH32(esp, edi);
    PUSH32(esp, 0x40);
    PUSH32(esp, 5);
    PUSH32(esp, 0x0017B01Fu); RECOMP_ABI_CALL(0x001E3400u, sub_001E3400); /* call 0x001E3400 */

loc_0017B01F: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    MEM32(0x45A7D4) = ebx;
    MEMF(0x45A7D8) = xmm0.f[0]; /* movss */
    edi = 2;
    MEM32(0x45A7D0) = edi;
    MEM32(esi + 0x50) = edi;
    MEM32(esi + 0x54) = ebx;
    MEMF(esi + 0x58) = xmm0.f[0]; /* movss */
    ecx = 3;
    MEM32(esi + 0x5C) = ecx;
    ebp = 1;
    MEM32(esi + 0x98) = ebx;
    MEM32(esi + 0x9C) = ebp;
    eax = MEM32(esi + 0x90);
    eax = MEM32(eax * 8 + 0x3E9D74);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0xF) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0xF (32-bit) */
    MEM32(0xDA3E90) = eax;
    MEM32(esp + 0x10) = 5;
    MEM32(esp + 0x14) = 6;
    MEM32(esp + 0x18) = 7;
    MEM32(esp + 0x1C) = 8;
    MEM32(esp + 0x20) = 9;
    MEM32(esp + 0x24) = ebx;
    MEM32(esp + 0x28) = ebp;
    MEM32(esp + 0x2C) = edi;
    MEM32(esp + 0x30) = ecx;
    MEM32(esp + 0x34) = 4;
    if (CMP_EQ(_fa, _fb)) goto loc_0017B0D8; /* je: equal / zero */

loc_0017B0C0: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x12) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x12 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0017B13C; /* jne: not equal / not zero */

loc_0017B0C5: ;
    PUSH32(esp, 0x14);
    PUSH32(esp, 0x64);
    eax = 0x5B;
    PUSH32(esp, 0x0017B0D3u); RECOMP_ABI_CALL(0x001E3A80u, sub_001E3A80); /* call 0x001E3A80 */

loc_0017B0D3: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_0017B13C;

loc_0017B0D8: ;
    MEM32(esp + 0xC) = ebx;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    edi = edi;

loc_0017B0E0: ;
    ecx = MEM32(esp + edx * 4 + 0x10);
    PUSH32(esp, 0x0017B0E9u); RECOMP_ABI_CALL(0x001DB520u, sub_001DB520); /* call 0x001DB520 */

loc_0017B0E9: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ebp) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ebp (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0017B0F1; /* jne: not equal / not zero */

loc_0017B0ED: ;
    MEM32(esp + 0xC) = MEM32(esp + 0xC) + 1;
    _fa = (uint32_t)(MEM32(esp + 0xC)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_0017B0F1: ;
    edx++;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(5) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 5 (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_0017B0E0; /* jb: below (unsigned <) */

loc_0017B0F7: ;
    _fa = (uint32_t)(MEM32(esp + 0xC)) & 0xFFFFFFFFu; _fb = (uint32_t)(ebp) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(esp + 0xC), ebp (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0017B13C; /* jne: not equal / not zero */

loc_0017B0FD: ;
    ebp = 0; /* xor self */
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */

loc_0017B101: ;
    ecx = MEM32(esp + edx * 4 + 0x24);
    PUSH32(esp, 0x0017B10Au); RECOMP_ABI_CALL(0x001DB520u, sub_001DB520); /* call 0x001DB520 */

loc_0017B10A: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 1 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0017B110; /* jne: not equal / not zero */

loc_0017B10F: ;
    ebp++;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_0017B110: ;
    edx++;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(5) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 5 (32-bit) */
    if (CMP_B(_fa, _fb)) goto loc_0017B101; /* jb: below (unsigned <) */

loc_0017B116: ;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, 1 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0017B13C; /* jne: not equal / not zero */

loc_0017B11B: ;
    PUSH32(esp, 0x14);
    PUSH32(esp, 0xFFFFFFFEu);
    PUSH32(esp, ebx);
    MEM32(0xC0D030) = edi;
    MEM32(0xC0D034) = ebx;
    MEMF(0xC0D038) = xmm0.f[0]; /* movss */
    PUSH32(esp, 0x0017B139u); RECOMP_ABI_CALL(0x001E3A20u, sub_001E3A20); /* call 0x001E3A20 */

loc_0017B139: ;
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_0017B13C: ;
    MEM8(0x452904) = LO8(ebx);
    MEM8(0x452905) = 0xFF;
    PUSH32(esp, 0x0017B14Eu); RECOMP_ABI_CALL(0x00175950u, sub_00175950); /* call 0x00175950 */

loc_0017B14E: ;
    ebx = 0xC074B8;
    PUSH32(esp, 0x0017B158u); RECOMP_ABI_CALL(0x00178B50u, sub_00178B50); /* call 0x00178B50 */

loc_0017B158: ;
    _fa = (uint32_t)(MEM32(esi + 0x90)) & 0xFFFFFFFFu; _fb = (uint32_t)(6) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(esi + 0x90), 6 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0017B1BB; /* jne: not equal / not zero */

loc_0017B161: ;
    PUSH32(esp, 0x14);
    PUSH32(esp, 0x64);
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    PUSH32(esp, ebx);
    PUSH32(esp, 0x0017B16Du); RECOMP_ABI_CALL(0x001E3A20u, sub_001E3A20); /* call 0x001E3A20 */

loc_0017B16D: ;
    eax = MEM32(esi + 0xB4);
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(edi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, edi (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0017B190; /* je: equal / zero */

loc_0017B17A: ;
    MEM32(esi + 0xB8) = edi;
    MEM32(esi + 0xBC) = ebx;
    MEM32(esi + 0xC0) = 0x14;

loc_0017B190: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    PUSH32(esp, 0x14);
    PUSH32(esp, 0xFFFFFFFEu);
    PUSH32(esp, ebx);
    MEM32(0xC0D030) = edi;
    MEM32(0xC0D034) = ebx;
    MEMF(0xC0D038) = xmm0.f[0]; /* movss */
    PUSH32(esp, 0x0017B1B6u); RECOMP_ABI_CALL(0x001E3A20u, sub_001E3A20); /* call 0x001E3A20 */

loc_0017B1B6: ;
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    goto loc_0017B1DF;

loc_0017B1BB: ;
    eax = MEM32(esi + 0xB4);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0017B1DD; /* je: equal / zero */

loc_0017B1C5: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    MEM32(esi + 0xB8) = eax;
    MEM32(esi + 0xBC) = eax;
    MEM32(esi + 0xC0) = 0x14;

loc_0017B1DD: ;
    ebx = 0; /* xor self */
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */

loc_0017B1DF: ;
    eax = MEM32(esi + 0xD0);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ebx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ebx (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_0017B1F3; /* jne: not equal / not zero */

loc_0017B1E9: ;
    xmm0 = XMM_SCALAR(MEMF(esi + 0xC8)); /* movss */
    goto loc_0017B225;

loc_0017B1F3: ;
    ecx = MEM32(esi + 0xC8);
    edx = MEM32(esi + 0xC4);
    xmm0.f[0] = (float)(int32_t)MEM32(esi + 0xCC); /* cvtsi2ss */
    PUSH32(esp, ecx);
    PUSH32(esp, edx);
    xmm1.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    eax = MEM32(esi + 0xD4);
    PUSH32(esp, ecx);
    xmm0.f[0] = xmm0.f[0] / xmm1.f[0]; /* divss */
    MEMF(esp) = xmm0.f[0]; /* movss */
    PUSH32(esp, 0x0017B222u); RECOMP_ABI_CALL(0x00303C50u, sub_00303C50); /* call 0x00303C50 */

loc_0017B222: ;
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_0017B225: ;
    POP32(esp, edi);
    MEMF(esi + 0xC4) = xmm0.f[0]; /* movss */
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    POP32(esp, ebp);
    MEM32(esi + 0xCC) = ebx;
    MEM32(esi + 0xD4) = ebx;
    MEMF(esi + 0xC8) = xmm0.f[0]; /* movss */
    MEM32(esi + 0xD0) = 0x14;
    POP32(esp, ebx);
    _fb = (uint32_t)(0x2C) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x2C;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 4; return; /* ret */

}


/* ---- Missions: every mission, with the Missions unlock on ----
 *
 * The Missions menu (its set rows: sub_001764F0, sub_00176840, sub_00176E30, sub_00177330, the
 * lifted ones -- src/manual_functions.json -- with their reads marked "Missions:") asks a byte per
 * mission of the system data, 0xDB95AC + its number: bit 0 offered, bit 1 cleared. The game offers
 * ten as Missions is earned (sub_001DB680: the list at 0x4474B8) and the rest as missions are
 * cleared. With the unlock on every mission reads as offered -- nothing written, as the main menu's
 * entries (the system data is checksummed, sub_00164F10, and saved: what was earned stays as it was).
 */
static uint8_t mission_bits(uint32_t mission)
{
    extern int ngb_unlock(int which);                  /* src/cheats.c */
    return (uint8_t)(MEM8(0xDB95AC + mission) | (ngb_unlock(0) ? 1 : 0));
}

/**
 * sub_001764F0
 * Original: 0x001764F0 - 0x00176833 (835 bytes, 201 insns)
 * CC: cdecl, 3 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_001764F0(void)
{
    RECOMP_ENTRY(0x001764F0u);
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

loc_001764F0: ;
    SET_LO8(eax, MEM8(0x9776A8));
    xmm0 = XMM_SCALAR(MEMF(0x4149D8)); /* movss */
    _fb = (uint32_t)(0x6C) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x6C;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x10) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(eax), 0x10 (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_0017650C; /* jne: not equal / not zero */

loc_00176504: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */

loc_0017650C: ;
    eax = MEM32(0x977680);
    xmm1 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x4149D8); /* mulss */
    PUSH32(esp, ebp);
    edx = ((int32_t)eax < 0) ? 0xFFFFFFFF : 0; /* cdq */
    PUSH32(esp, esi);
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - edx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, edi);
    edi = MEM32(esp + 0x80);
    eax = (uint32_t)(((int32_t)(int32_t)(eax)) >> ((1) & 31u));
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sar result */
    ecx = edi + edi * 4 + 0x4474B8;
    MEMF(esp + 0x3C) = xmm1.f[0]; /* movss */
    xmm1 = XMM_ZERO(); /* xorps self = zero */
    _fb = (uint32_t)(2) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - 2;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    MEM32(esp + 0x10) = ecx;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    MEMF(esp + 0x44) = xmm1.f[0]; /* movss */
    xmm1 = XMM_SCALAR(MEMF(esp + 0x84)); /* movss */
    MEM32(esp + 0x14) = eax;
    MEM32(esp + 0xC) = ecx;
    MEM16(esp + 0x26) = LO16(ebx);
    MEM8(esp + 0x2B) = 1;
    MEM8(esp + 0x2C) = 1;
    MEM32(esp + 0x30) = ecx;
    MEM32(esp + 0x48) = 0xFFFFFFFFu;
    MEMF(esp + 0x40) = xmm1.f[0]; /* movss */
    MEM8(esp + 0x2A) = 0x11;
    MEMF(esp + 0x18) = xmm0.f[0]; /* movss */
    goto loc_001765A0;

loc_0017658E: ;
    eax = MEM32(esp + 0x14);
    edi = MEM32(esp + 0x80);
    ecx = MEM32(esp + 0xC);
    /* nop */

loc_001765A0: ;
    edx = ecx;
    { int64_t _r = (int64_t)(int32_t)(edx) * (int64_t)(int32_t)(0x7C);
      edx = (uint32_t)_r;
      _fa = (uint32_t)_r & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); _fb = (_r != (int64_t)(int32_t)_r); } /* imul */
    ebp = edx + eax + -246;
    eax = MEM32(esp + 0x10);
    eax = ZX8(MEM8(ecx + eax));
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x64) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x64 (32-bit) */
    if (CMP_GE(_fas, _fbs)) goto loc_001765C2; /* jge: greater or equal (signed >=) */

loc_001765B9: ;
    _fa = (uint32_t)(mission_bits(eax)) & 0xFFu;   /* Missions: */ _fb = (uint32_t)(3) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test MEM8(eax + 0xDB95AC), 3 (8-bit) */
    if (TEST_NZ(_fa, _fb)) goto loc_001765C9; /* jne: not equal / not zero */

loc_001765C2: ;
    eax = 0x7D4;
    goto loc_001765CE;

loc_001765C9: ;
    _fb = (uint32_t)(0x76B) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0x76B;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_001765CE: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(9) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 9 (32-bit) */
    xmm0 = XMM_SCALAR(MEMF(0x4149D8)); /* movss */
    MEMF(esp + 0x38) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x18)); /* movss */
    MEM32(esp + 0x20) = eax;
    MEM16(esp + 0x24) = LO16(ebp);
    MEM16(esp + 0x28) = 0x1C2;
    MEMF(esp + 0x34) = xmm0.f[0]; /* movss */
    if (CMP_EQ(_fa, _fb)) goto loc_00176708; /* je: equal / zero */

loc_00176601: ;
    esi = MEM32(esp + 0x7C);
    edx = MEM32(esi + 0x68);
    ecx = esi + 0x68;
    { uint32_t _icall_esp = g_esp;
    { uint32_t _icall_target = MEM32(edx + 0x10); PUSH32(esp, 0x0017660Eu); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x0017660Bu); } /* indirect call */
    }

loc_0017660E: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, eax (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00176708; /* jne: not equal / not zero */

loc_00176616: ;
    eax = MEM32(esp + 0xC);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(esi + 0x58)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, MEM32(esi + 0x58) (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00176708; /* jne: not equal / not zero */

loc_00176623: ;
    edx = MEM32(esi + 0x68);
    ecx = esi + 0x68;
    { uint32_t _icall_esp = g_esp;
    { uint32_t _icall_target = MEM32(edx + 0x10); PUSH32(esp, 0x0017662Cu); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x00176629u); } /* indirect call */
    }

loc_0017662C: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, eax (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00176659; /* jne: not equal / not zero */

loc_00176630: ;
    xmm0 = XMM_SCALAR(MEMF(0x415540)); /* movss */
    MEMF(esp + 0x38) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x34)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x414C90); /* mulss */
    MEMF(esp + 0x34) = xmm0.f[0]; /* movss */
    MEM16(esp + 0x28) = 0x1C6;

loc_00176659: ;
    ecx = ZX8(MEM8(0xDB9ADA));
    fp_push((double)SMEM32(0xD21734)); /* fild */
    MEM32(esp + 0x1C) = ecx;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    eax = MEM32(esp + 0x28);
    fp_top() = RECOMP_FP_PC(fp_top() / (double)SMEM32(esp + 0x1C)); /* fidiv dword ptr [esp + 0x1c] */
    MEMF(esp + 0x68) = xmm0.f[0]; /* movss */
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    MEMF(esp + 0x70) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x38)); /* movss */
    _fb = (uint32_t)(2) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 2;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEMF(esp + 0x64) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x34)); /* movss */
    edi = esp + 0x4C;
    MEM32(esp + 0x4C) = 0x7D1;
    MEM16(esp + 0x50) = LO16(ebp);
    MEM16(esp + 0x52) = LO16(ebx);
    MEM16(esp + 0x54) = LO16(eax);
    MEM8(esp + 0x57) = 1;
    MEM8(esp + 0x58) = 1;
    MEM32(esp + 0x5C) = 0;
    MEM32(esp + 0x74) = 0xFFFFFFFFu;
    MEM8(esp + 0x56) = 0x11;
    MEMF(esp + 0x60) = xmm0.f[0]; /* movss */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x414944)); /* fmul dword ptr [0x414944] */
    fp_top() = sin(fp_top()); /* fsin */
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C8)); /* fadd dword ptr [0x4148c8] */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x4149C0)); /* fmul dword ptr [0x4149c0] */
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C0)); /* fadd dword ptr [0x4148c0] */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x84)); /* fmul dword ptr [esp + 0x84] */
    MEMF(esp + 0x6C) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x00176708u); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_00176708: ;
    _fa = (uint32_t)(MEM32(0x43B490)) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0x43B490), 1 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00176762; /* jne: not equal / not zero */

loc_00176711: ;
    ecx = MEM32(0x8F7DFC);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(0x8F7DF8)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, MEM32(0x8F7DF8) (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_00176762; /* jae: above or equal (unsigned >=) */

loc_0017671F: ;
    edx = MEM32(0x8F7DF4);
    edi = MEM32(0x8F7DF0);
    esi = ecx;
    { int64_t _r = (int64_t)(int32_t)(esi) * (int64_t)(int32_t)(0x70);
      esi = (uint32_t)_r;
      _fa = (uint32_t)_r & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); _fb = (_r != (int64_t)(int32_t)_r); } /* imul */
    ecx = ecx << 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, 0);
    PUSH32(esp, 0);
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + edx;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fb = (uint32_t)(edi) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + edi;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = esp + 0x28;
    PUSH32(esp, 0x00176744u); RECOMP_ICALL_SAFE(0x002216B0u, _icall_esp); /* manual call 0x002216B0 */
    }

loc_00176744: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00176762; /* je: equal / zero */

loc_0017674B: ;
    ecx = MEM32(0x8F7DFC);
    eax = MEM32(esp + 0x28);
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    PUSH32(esp, 0x0017675Cu); RECOMP_ABI_CALL(0x00221170u, sub_00221170); /* call 0x00221170 */

loc_0017675C: ;
    MEM32(0x8F7DFC) = MEM32(0x8F7DFC) + 1;
    _fa = (uint32_t)(MEM32(0x8F7DFC)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_00176762: ;
    MEM16(esp + 0x28) = MEM16(esp + 0x28) + 1;
    _fa = (uint32_t)(MEM16(esp + 0x28)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fb = (_fa == 0x8000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(MEM32(0x43B490)) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(0x43B490), 1 (32-bit) */
    edi = MEM32(esp + 0x28);
    MEM32(esp + 0x20) = 0x7D3;
    if (CMP_NE(_fa, _fb)) goto loc_001767C9; /* jne: not equal / not zero */

loc_0017677C: ;
    ecx = MEM32(0x8F7DFC);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(MEM32(0x8F7DF8)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, MEM32(0x8F7DF8) (32-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_001767C9; /* jae: above or equal (unsigned >=) */

loc_0017678A: ;
    eax = MEM32(0x8F7DF4);
    esi = ecx;
    { int64_t _r = (int64_t)(int32_t)(esi) * (int64_t)(int32_t)(0x70);
      esi = (uint32_t)_r;
      _fa = (uint32_t)_r & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); _fb = (_r != (int64_t)(int32_t)_r); } /* imul */
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + eax;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = MEM32(0x8F7DF0);
    ecx = ecx << 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, 0);
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, 0);
    eax = esp + 0x28;
    PUSH32(esp, 0x001767ADu); RECOMP_ICALL_SAFE(0x002216B0u, _icall_esp); /* manual call 0x002216B0 */
    }

loc_001767AD: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001767C9; /* je: equal / zero */

loc_001767B4: ;
    ecx = MEM32(0x8F7DFC);
    eax = edi;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    PUSH32(esp, 0x001767C3u); RECOMP_ABI_CALL(0x00221170u, sub_00221170); /* call 0x00221170 */

loc_001767C3: ;
    MEM32(0x8F7DFC) = MEM32(0x8F7DFC) + 1;
    _fa = (uint32_t)(MEM32(0x8F7DFC)) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */

loc_001767C9: ;
    edx = MEM32(esp + 0x10);
    eax = MEM32(esp + 0xC);
    eax = ZX8(MEM8(eax + edx));
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x64) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x64 (32-bit) */
    if (CMP_GE(_fas, _fbs)) goto loc_00176818; /* jge: greater or equal (signed >=) */

loc_001767DA: ;
    SET_LO8(eax, MEM8(eax + 0xDB9674));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(4) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 4 (8-bit) */
    if (CMP_A(_fa, _fb)) goto loc_00176818; /* ja: above (unsigned >) */

loc_001767E4: ;
    if (CMP_NE(_fa, _fb)) goto loc_001767FF; /* jne: not equal / not zero */

loc_001767E6: ;
    ecx = MEM32(esp + 0x84);
    edx = MEM32(esp + 0x38);
    PUSH32(esp, ecx);
    PUSH32(esp, edx);
    PUSH32(esp, ebx);
    esi = edi;
    eax = ebp;
    PUSH32(esp, 0x001767FDu); RECOMP_ABI_CALL(0x001760B0u, sub_001760B0); /* call 0x001760B0 */

loc_001767FD: ;
    goto loc_00176818;

loc_001767FF: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 1 (8-bit) */
    if (CMP_B(_fa, _fb)) goto loc_00176818; /* jb: below (unsigned <) */

loc_00176803: ;
    ecx = MEM32(esp + 0x84);
    edx = MEM32(esp + 0x38);
    PUSH32(esp, ecx);
    PUSH32(esp, edx);
    PUSH32(esp, edi);
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    PUSH32(esp, 0x00176818u); RECOMP_ABI_CALL(0x00176320u, sub_00176320); /* call 0x00176320 */

loc_00176818: ;
    eax = MEM32(esp + 0xC);
    eax++;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(5) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 5 (32-bit) */
    MEM32(esp + 0xC) = eax;
    if (CMP_B(_fa, _fb)) goto loc_0017658E; /* jb: below (unsigned <) */

loc_0017682A: ;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebp);
    _fb = (uint32_t)(0x6C) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0x6C;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 16; return; /* ret 12 */

    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}

/**
 * sub_00176840
 * Original: 0x00176840 - 0x00176A59 (537 bytes, 133 insns)
 * CC: cdecl, 3 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_00176840(void)
{
    RECOMP_ENTRY(0x00176840u);
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

loc_00176840: ;
    SET_LO8(eax, MEM8(0x9776A8));
    xmm0 = XMM_SCALAR(MEMF(0x45A970)); /* movss */
    _fb = (uint32_t)(0x30) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(esp) < (uint32_t)(0x30));
    esp = esp - 0x30;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0x10) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(eax), 0x10 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (TEST_NZ(_fa, _fb)) goto loc_0017685C; /* jne: not equal / not zero */

loc_00176854: ;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */

loc_0017685C: ;
    eax = MEM32(0x977680);
    PUSH32(esp, ebx);
    edx = ((int32_t)eax < 0) ? 0xFFFFFFFF : 0; /* cdq */
    PUSH32(esp, ebp);
    ebp = ZX8(MEM8(0x4474E5));
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    _cf = (int)((uint32_t)(eax) < (uint32_t)(edx));
    eax = eax - edx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    PUSH32(esp, esi);
    esi = eax;
    if (((1) & 31u)) _cf = (int)(((uint32_t)((int32_t)(int32_t)(esi)) >> ((((1) & 31u)) - 1)) & 1);
    esi = (uint32_t)(((int32_t)(int32_t)(esi)) >> ((1) & 31u));
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sar result */
    _cf = 0; /* xor clears CF */
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(0x64) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, 0x64 (32-bit) */
    _cf = (int)(_fa < _fb);
    ebx = 1;
    if (CMP_GE(_fas, _fbs)) goto loc_00176894; /* jge: greater or equal (signed >=) */

loc_0017687E: ;
    SET_LO8(ecx, mission_bits(ebp));   /* Missions: */
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(2) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ecx), 2 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (TEST_Z(_fa, _fb)) goto loc_0017688E; /* je: equal / zero */

loc_00176889: ;
    _cf = 0; /* logical op clears CF */
    eax = eax | 0xFFFFFFFFu;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    goto loc_00176894;

loc_0017688E: ;
    _fa = (uint32_t)(LO8(ebx)) & 0xFFu; _fb = (uint32_t)(LO8(ecx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ebx), LO8(ecx) (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (TEST_Z(_fa, _fb)) goto loc_00176894; /* je: equal / zero */

loc_00176892: ;
    eax = ebx;

loc_00176894: ;
    xmm1 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    _cf = (int)((eax) != 0);
    eax = (uint32_t)(-(int32_t)eax);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* neg result */
    MEMF(esp + 0x2C) = xmm1.f[0]; /* movss */
    xmm1 = XMM_ZERO(); /* xorps self = zero */
    eax = _cf ? 0xFFFFFFFF : 0; _fb = 0; /* sbb self (CF extend), OF clear */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sbb result */
    _fb = (uint32_t)(0x7D6) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(0x7D6)) >> 32) & 1);
    eax = eax + 0x7D6;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEMF(esp + 0x34) = xmm1.f[0]; /* movss */
    xmm1 = XMM_SCALAR(MEMF(esp + 0x48)); /* movss */
    MEM32(esp + 0x10) = eax;
    SET_LO16(eax, MEM16(esp + 0x44));
    MEMF(esp + 0x30) = xmm1.f[0]; /* movss */
    xmm1 = XMM_SCALAR(MEMF(0x4149D8)); /* movss */
    PUSH32(esp, edi);
    xmm0.f[0] = xmm0.f[0] * xmm1.f[0]; /* mulss */
    edi = esp + 0x14;
    MEM16(esp + 0x18) = LO16(esi);
    MEM16(esp + 0x1A) = LO16(eax);
    MEM16(esp + 0x1C) = 0x1C2;
    MEM8(esp + 0x1F) = LO8(ebx);
    MEM8(esp + 0x20) = LO8(ebx);
    MEM32(esp + 0x24) = 0;
    MEM32(esp + 0x3C) = 0xFFFFFFFFu;
    MEM8(esp + 0x1E) = 0x11;
    MEMF(esp + 0x2C) = xmm1.f[0]; /* movss */
    MEMF(esp + 0x10) = xmm0.f[0]; /* movss */
    MEMF(esp + 0x28) = xmm0.f[0]; /* movss */
    PUSH32(esp, 0x0017691Fu); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_0017691F: ;
    ecx = MEM32(esp + 0x44);
    edx = MEM32(ecx + 0x68);
    _fb = (uint32_t)(0x68) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(ecx) + (uint64_t)(0x68)) >> 32) & 1);
    ecx = ecx + 0x68;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    { uint32_t _icall_esp = g_esp;
    { uint32_t _icall_target = MEM32(edx + 0x10); PUSH32(esp, 0x0017692Cu); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x00176929u); } /* indirect call */
    }

loc_0017692C: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(9) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 9 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_NE(_fa, _fb)) goto loc_001769EA; /* jne: not equal / not zero */

loc_00176935: ;
    eax = ZX8(MEM8(0xDB9ADA));
    fp_push((double)SMEM32(0xD21734)); /* fild */
    MEM16(esp + 0x18) = LO16(esi);
    esi = MEM32(esp + 0x48);
    MEM32(esp + 0x48) = eax;
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    MEMF(esp + 0x30) = xmm0.f[0]; /* movss */
    fp_top() = RECOMP_FP_PC(fp_top() / (double)SMEM32(esp + 0x48)); /* fidiv dword ptr [esp + 0x48] */
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    MEMF(esp + 0x38) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x4149D8)); /* movss */
    MEMF(esp + 0x2C) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(esp + 0x10)); /* movss */
    xmm0.f[0] = xmm0.f[0] * MEMF(0x3EC068); /* mulss */
    edi = esp + 0x14;
    MEM32(esp + 0x14) = 0x7D2;
    MEM16(esp + 0x1A) = LO16(esi);
    MEM16(esp + 0x1C) = 0x1C4;
    MEM8(esp + 0x1F) = LO8(ebx);
    MEM8(esp + 0x20) = LO8(ebx);
    MEM32(esp + 0x24) = 0;
    MEM32(esp + 0x3C) = 0xFFFFFFFFu;
    MEM8(esp + 0x1E) = 0x11;
    MEMF(esp + 0x28) = xmm0.f[0]; /* movss */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x414944)); /* fmul dword ptr [0x414944] */
    fp_top() = sin(fp_top()); /* fsin */
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C8)); /* fadd dword ptr [0x4148c8] */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(0x4149C0)); /* fmul dword ptr [0x4149c0] */
    fp_top() = RECOMP_FP_PC(fp_top() + MEMF(0x4148C0)); /* fadd dword ptr [0x4148c0] */
    fp_top() = RECOMP_FP_PC(fp_top() * MEMF(esp + 0x4C)); /* fmul dword ptr [esp + 0x4c] */
    MEMF(esp + 0x34) = (float)fp_top(); fp_pop(); /* fstp */
    PUSH32(esp, 0x001769E8u); RECOMP_ABI_CALL(0x00221340u, sub_00221340); /* call 0x00221340 */

loc_001769E8: ;
    goto loc_001769EE;

loc_001769EA: ;
    esi = MEM32(esp + 0x48);

loc_001769EE: ;
    ecx = MEM32(0x9776A8);
    _cf = 0; /* logical op clears CF */
    SET_LO8(ecx, LO8(ecx) & 0x10);
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* and result */
    _cf = (int)((LO8(ecx)) != 0);
    SET_LO8(ecx, (uint32_t)(-(int32_t)LO8(ecx)));
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* neg result */
    POP32(esp, edi);
    ecx = _cf ? 0xFFFFFFFF : 0; _fb = 0; /* sbb self (CF extend), OF clear */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sbb result */
    _cf = 0; /* logical op clears CF */
    ecx = ecx & 0xFFFFFFB2u;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fb = (uint32_t)(0x260) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(ecx) + (uint64_t)(0x260)) >> 32) & 1);
    ecx = ecx + 0x260;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(0x64) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, 0x64 (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_GE(_fas, _fbs)) goto loc_00176A50; /* jge: greater or equal (signed >=) */

loc_00176A0A: ;
    SET_LO8(eax, MEM8(ebp + 0xDB9674));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(4) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 4 (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_A(_fa, _fb)) goto loc_00176A50; /* ja: above (unsigned >) */

loc_00176A14: ;
    if (CMP_NE(_fa, _fb)) goto loc_00176A36; /* jne: not equal / not zero */

loc_00176A16: ;
    edx = MEM32(esp + 0x48);
    PUSH32(esp, edx);
    PUSH32(esp, 0x3F4CCCCD);
    PUSH32(esp, esi);
    esi = 0x1C6;
    eax = ecx;
    PUSH32(esp, 0x00176A2Du); RECOMP_ABI_CALL(0x001760B0u, sub_001760B0); /* call 0x001760B0 */

loc_00176A2D: ;
    POP32(esp, esi);
    POP32(esp, ebp);
    POP32(esp, ebx);
    _fb = (uint32_t)(0x30) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(0x30)) >> 32) & 1);
    esp = esp + 0x30;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 16; return; /* ret 12 */

loc_00176A36: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), LO8(ebx) (8-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_B(_fa, _fb)) goto loc_00176A50; /* jb: below (unsigned <) */

loc_00176A3A: ;
    edx = MEM32(esp + 0x48);
    PUSH32(esp, edx);
    PUSH32(esp, 0x3F4CCCCD);
    PUSH32(esp, 0x1C6);
    PUSH32(esp, esi);
    PUSH32(esp, ecx);
    PUSH32(esp, 0x00176A50u); RECOMP_ABI_CALL(0x00176320u, sub_00176320); /* call 0x00176320 */

loc_00176A50: ;
    POP32(esp, esi);
    POP32(esp, ebp);
    POP32(esp, ebx);
    _fb = (uint32_t)(0x30) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(esp) + (uint64_t)(0x30)) >> 32) & 1);
    esp = esp + 0x30;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 16; return; /* ret 12 */

    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}

/**
 * sub_00176E30
 * Original: 0x00176E30 - 0x001771B4 (900 bytes, 229 insns)
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: standard_frame
 */
void sub_00176E30(void)
{
    RECOMP_ENTRY(0x00176E30u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* prologue saves caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_00176E30: ;
    PUSH32(esp, ebp);
    ebp = esp;
    g_ebp = ebp; /* publish frame for frameless callees */
    g_seh_ebp = ebp;
    esp = esp & 0xFFFFFFF0u;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fb = (uint32_t)(0x68) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x68;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    edx = MEM32(ebx + 0x68);
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    edi = ZX8(MEM8(ebp + 8));
    { int64_t _r = (int64_t)(int32_t)(edi) * (int64_t)(int32_t)(0x78);
      edi = (uint32_t)_r;
      _fa = (uint32_t)_r & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); _fb = (_r != (int64_t)(int32_t)_r); } /* imul */
    eax = edi + 0xD219A0;
    esi = ebx + 0x68;
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, eax);
    ecx = esi;
    { uint32_t _icall_target = MEM32(edx + 0x20); PUSH32(esp, 0x00176E54u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x00176E51u); } /* indirect call */
    }

loc_00176E54: ;
    eax = MEM32(esi);
    ecx = esi;
    { uint32_t _icall_esp = g_esp;
    { uint32_t _icall_target = MEM32(eax + 0x10); PUSH32(esp, 0x00176E5Bu); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x00176E58u); } /* indirect call */
    }

loc_00176E5B: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(9) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 9 (32-bit) */
    if (CMP_GE(_fas, _fbs)) goto loc_00176E80; /* jge: greater or equal (signed >=) */

loc_00176E60: ;
    ecx = edi + 0xD219A0;
    eax = ebx + 0x58;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00176E6Eu); RECOMP_ABI_CALL(0x001759B0u, sub_001759B0); /* call 0x001759B0 */

loc_00176E6E: ;
    _fa = (uint32_t)(MEM32(eax + 0xC)) & 0xFFFFFFFFu; _fb = (uint32_t)(1) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(eax + 0xC), 1 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00176E80; /* jne: not equal / not zero */

loc_00176E74: ;
    PUSH32(esp, 0x40);
    PUSH32(esp, 1);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00176E7Du); RECOMP_ABI_CALL(0x001E3400u, sub_001E3400); /* call 0x001E3400 */

loc_00176E7D: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_00176E80: ;
    eax = MEM32(ebx + 0x80);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001771A5; /* jne: not equal / not zero */

loc_00176E8E: ;
    _fa = (uint32_t)(MEM32(edi + 0xD219A8)) & 0xFFFFFFFFu; _fb = (uint32_t)(0x510) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test MEM32(edi + 0xD219A8), 0x510 (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_0017711C; /* je: equal / zero */

loc_00176E9E: ;
    esi = ebx;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00176EA5u); RECOMP_ABI_CALL(0x00176070u, sub_00176070); /* call 0x00176070 */

loc_00176EA5: ;
    eax = ZX8(LO8(eax));
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x64) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x64 (32-bit) */
    if (CMP_GE(_fas, _fbs)) goto loc_0017711C; /* jge: greater or equal (signed >=) */

loc_00176EB1: ;
    _fa = (uint32_t)(mission_bits(eax)) & 0xFFu;   /* Missions: */ _fb = (uint32_t)(3) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test MEM8(eax + 0xDB95AC), 3 (8-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_0017711C; /* je: equal / zero */

loc_00176EBE: ;
    SET_LO8(eax, MEM8(0x452904));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00176ED3; /* je: equal / zero */

loc_00176EC7: ;
    SET_LO8(ecx, MEM8(0x452905));
    MEM8(0x589761) = LO8(ecx);

loc_00176ED3: ;
    esi = ebx;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00176EDAu); RECOMP_ABI_CALL(0x00176070u, sub_00176070); /* call 0x00176070 */

loc_00176EDA: ;
    ecx = ZX8(LO8(eax));
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x64) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x64 (32-bit) */
    MEM8(0x589758) = LO8(eax);
    MEM8(0x58975A) = 3;
    SET_LO8(eax, 1);
    if (CMP_GE(_fas, _fbs)) goto loc_00176F04; /* jge: greater or equal (signed >=) */

loc_00176EF0: ;
    SET_LO8(eax, MEM8(ecx + 0xDB9610));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 1 (8-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_00176EFE; /* jae: above or equal (unsigned >=) */

loc_00176EFA: ;
    SET_LO8(eax, 1);
    goto loc_00176F04;

loc_00176EFE: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(4) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 4 (8-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_00176F04; /* jbe: below or equal (unsigned <=) */

loc_00176F02: ;
    SET_LO8(eax, 4);

loc_00176F04: ;
    PUSH32(esp, 0x40);
    PUSH32(esp, 5);
    MEM8(0x589755) = LO8(eax);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00176F12u); RECOMP_ABI_CALL(0x001E3400u, sub_001E3400); /* call 0x001E3400 */

loc_00176F12: ;
    SET_LO8(eax, MEM8(0x589755));
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 1 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_00176FB2; /* jne: not equal / not zero */

loc_00176F22: ;
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    eax = 2;
    MEM32(esp + 0x18) = eax;
    MEMF(esp + 0x10) = xmm0.f[0]; /* movss */
    MEM32(esp + 0x14) = 0;
    ecx = 8;
    esi = 0x431160;
    edi = esp + 0x30;
    if (!g_df) { uint8_t *_d = (uint8_t*)XBOX_PTR(edi), *_s = (uint8_t*)XBOX_PTR(esi); uint32_t _n = ecx * 4;
      if (_d + _n <= _s || _s + _n <= _d) memcpy(_d, _s, _n);
      else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*4) = MEM32(esi + _i*4); }
      esi += ecx * 4; edi += ecx * 4; }
    else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi - _i*4) = MEM32(esi - _i*4); esi -= ecx * 4; edi -= ecx * 4; }
    ecx = 0; /* rep movsd */
    ecx = 8;
    esi = 0xC0CAD0;
    edi = esp + 0x50;
    if (!g_df) { uint8_t *_d = (uint8_t*)XBOX_PTR(edi), *_s = (uint8_t*)XBOX_PTR(esi); uint32_t _n = ecx * 4;
      if (_d + _n <= _s || _s + _n <= _d) memcpy(_d, _s, _n);
      else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi + _i*4) = MEM32(esi + _i*4); }
      esi += ecx * 4; edi += ecx * 4; }
    else { uint32_t _i; for (_i = 0; _i < ecx; _i++) MEM32(edi - _i*4) = MEM32(esi - _i*4); esi -= ecx * 4; edi -= ecx * 4; }
    ecx = 0; /* rep movsd */
    MEM32(esp + 0x1C) = eax;
    eax = MEM32(0xC0CEF0);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(8) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 8 (32-bit) */
    MEM32(esp + 0x20) = 1;
    MEM32(esp + 0x24) = 0x10;
    if (CMP_AE(_fa, _fb)) goto loc_00176F97; /* jae: above or equal (unsigned >=) */

loc_00176F7A: ;
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

loc_00176F97: ;
    edx = MEM32(ebx + 8);
    MEM32(ebx + 0x140) = edx;
    MEM32(ebx + 0x144) = 0x14;
    POP32(esp, edi);
    POP32(esp, esi);
    esp = ebp;
    POP32(esp, ebp);
    esp += 8; return; /* ret 4 */

loc_00176FB2: ;
    eax = MEM32(ebx + 8);
    MEM32(ebx + 0xC) = 2;
    eax = eax + eax * 2;
    ecx = MEM32(eax * 4 + 0x3E9CA4);
    MEM32(ebx + 0x10) = ecx;
    ecx = ZX8(MEM8(0x589758));
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00176FD5u); RECOMP_ABI_CALL(0x001DB8A0u, sub_001DB8A0); /* call 0x001DB8A0 */

loc_00176FD5: ;
    SET_LO8(eax, LO8(eax) + 1);
    _fa = (uint32_t)(LO8(eax)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fb = (_fa == 0x80u); /* inc result/SF/OF; CF unchanged */
    esi = ZX8(LO8(eax));
    ecx = 1;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, ecx (32-bit) */
    if (CMP_LE(_fas, _fbs)) goto loc_00176FFE; /* jle: less or equal (signed <=) */

loc_00176FE3: ;
    edx = MEM32(ebx + 0xCC);
    MEM8(edx + ecx * 8 + 4) = 1;
    eax = MEM32(ebx + 0xCC);
    MEM8(eax + ecx * 8 + 5) = 1;
    ecx++;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, esi (32-bit) */
    if (CMP_L(_fas, _fbs)) goto loc_00176FE3; /* jl: less (signed <) */

loc_00176FFE: ;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(5) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 5 (32-bit) */
    if (CMP_GE(_fas, _fbs)) goto loc_0017702E; /* jge: greater or equal (signed >=) */

loc_00177003: ;
    eax = esi * 8;
    /* nop */

loc_00177010: ;
    ecx = MEM32(ebx + 0xCC);
    MEM8(eax + ecx + 4) = 0;
    edx = MEM32(ebx + 0xCC);
    MEM8(eax + edx + 5) = 0;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 8;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x28) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x28 (32-bit) */
    if (CMP_L(_fas, _fbs)) goto loc_00177010; /* jl: less (signed <) */

loc_0017702E: ;
    edx = ebx + 0xCC;
    eax = ebx + 0xD4;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0017703Fu); RECOMP_ABI_CALL(0x0017A250u, sub_0017A250); /* call 0x0017A250 */

loc_0017703F: ;
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    MEM32(ebx + 0x124) = ecx;
    MEM32(ebx + 0x128) = ecx;
    eax = ecx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ecx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ecx (32-bit) */
    MEMF(ebx + 0x12C) = xmm0.f[0]; /* movss */
    MEMF(ebx + 0x130) = xmm0.f[0]; /* movss */
    MEM32(ebx + 0x134) = ecx;
    MEM32(ebx + 0x138) = ecx;
    MEM32(ebx + 0x13C) = ecx;
    if (CMP_NE(_fa, _fb)) goto loc_0017707A; /* jne: not equal / not zero */

loc_00177078: ;
    goto loc_001770AC;

loc_0017707A: ;
    edx = MEM32(ebx + 0x130);
    xmm0.f[0] = (float)(int32_t)MEM32(ebx + 0x134); /* cvtsi2ss */
    PUSH32(esp, edx);
    edx = MEM32(ebx + 0x12C);
    PUSH32(esp, edx);
    xmm1.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    eax = MEM32(ebx + 0x13C);
    PUSH32(esp, ecx);
    xmm0.f[0] = xmm0.f[0] / xmm1.f[0]; /* divss */
    MEMF(esp) = xmm0.f[0]; /* movss */
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001770A9u); RECOMP_ABI_CALL(0x00303C50u, sub_00303C50); /* call 0x00303C50 */

loc_001770A9: ;
    _fb = (uint32_t)(0xC) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 0xC;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_001770AC: ;
    MEMF(ebx + 0x12C) = xmm0.f[0]; /* movss */
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    MEMF(ebx + 0x130) = xmm0.f[0]; /* movss */
    MEM32(ebx + 0x134) = ecx;
    MEM32(ebx + 0x138) = 0x14;
    MEM32(ebx + 0x13C) = ecx;
    eax = esi + -1;
    MEM32(ebx + 0x11C) = eax;
    MEM32(ebx + 0x120) = eax;
    MEM32(ebx + 0x110) = ecx;
    MEM32(ebx + 0x114) = ecx;
    MEM32(ebx + 0x118) = ecx;
    ecx = MEM32(ebx + 0x4C);
    eax = 0x39;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, eax (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001771AC; /* je: equal / zero */

loc_0017710B: ;
    MEM32(ebx + 0x4C) = eax;
    eax = MEM32(ebx + 0x44);
    MEM32(ebx + 0x50) = eax;
    POP32(esp, edi);
    POP32(esp, esi);
    esp = ebp;
    POP32(esp, ebp);
    esp += 8; return; /* ret 4 */

loc_0017711C: ;
    edi = MEM32(edi + 0xD219A8);
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0x220) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test edi, 0x220 (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_0017717D; /* je: equal / zero */

loc_0017712A: ;
    PUSH32(esp, 0x40);
    PUSH32(esp, 3);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x00177133u); RECOMP_ABI_CALL(0x001E3400u, sub_001E3400); /* call 0x001E3400 */

loc_00177133: ;
    eax = MEM32(ebx + 8);
    xmm0 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    MEM32(ebx + 0xC) = ecx;
    edx = eax + eax * 2;
    eax = MEM32(edx * 4 + 0x3E9C9C);
    MEM32(ebx + 0x10) = eax;
    MEM32(0xDA3E90) = 1;
    MEM32(0x45A7D0) = 2;
    MEM32(0x45A7D4) = ecx;
    MEMF(0x45A7D8) = xmm0.f[0]; /* movss */
    POP32(esp, edi);
    POP32(esp, esi);
    esp = ebp;
    POP32(esp, ebp);
    esp += 8; return; /* ret 4 */

loc_0017717D: ;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0x800) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* test edi, 0x800 (32-bit) */
    if (TEST_Z(_fa, _fb)) goto loc_001771A5; /* je: equal / zero */

loc_00177185: ;
    PUSH32(esp, 0x40);
    PUSH32(esp, 5);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0017718Eu); RECOMP_ABI_CALL(0x001E3400u, sub_001E3400); /* call 0x001E3400 */

loc_0017718E: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ecx = 0xE;
    eax = ebx;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x0017719Du); RECOMP_ABI_CALL(0x00176D00u, sub_00176D00); /* call 0x00176D00 */

loc_0017719D: ;
    POP32(esp, edi);
    POP32(esp, esi);
    esp = ebp;
    POP32(esp, ebp);
    esp += 8; return; /* ret 4 */

loc_001771A5: ;
    esi = ebx;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001771ACu); RECOMP_ABI_CALL(0x00177330u, sub_00177330); /* call 0x00177330 */

loc_001771AC: ;
    POP32(esp, edi);
    POP32(esp, esi);
    esp = ebp;
    POP32(esp, ebp);
    esp += 8; return; /* ret 4 */

}

/**
 * sub_00177330
 * Original: 0x00177330 - 0x001773EA (186 bytes, 63 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_00177330(void)
{
    RECOMP_ENTRY(0x00177330u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    int _cf = 0; /* carry flag */

loc_00177330: ;
    eax = MEM32(esi + 0x68);
    ecx = esi + 0x68;
    PUSH32(esp, edi);
    { uint32_t _icall_esp = g_esp;
    { uint32_t _icall_target = MEM32(eax + 0x10); PUSH32(esp, 0x0017733Au); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x00177337u); } /* indirect call */
    }

loc_0017733A: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(9) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 9 (32-bit) */
    _cf = (int)(_fa < _fb);
    ecx = esi + 0x68;
    if (CMP_NE(_fa, _fb)) goto loc_00177350; /* jne: not equal / not zero */

loc_00177342: ;
    edx = MEM32(ecx);
    { uint32_t _icall_esp = g_esp;
    { uint32_t _icall_target = MEM32(edx + 0x10); PUSH32(esp, 0x00177347u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x00177344u); } /* indirect call */
    }

loc_00177347: ;
    SET_LO8(ecx, MEM8(eax + eax * 4 + 0x4474B8));
    goto loc_00177363;

loc_00177350: ;
    eax = MEM32(ecx);
    edi = MEM32(esi + 0x58);
    { uint32_t _icall_esp = g_esp;
    { uint32_t _icall_target = MEM32(eax + 0x10); PUSH32(esp, 0x00177358u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x00177355u); } /* indirect call */
    }

loc_00177358: ;
    ecx = edi + eax * 4;
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(ecx) + (uint64_t)(eax)) >> 32) & 1);
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    SET_LO8(ecx, MEM8(ecx + 0x4474B8));

loc_00177363: ;
    eax = ZX8(LO8(ecx));
    _fa = (uint32_t)(MEM32(esi + 0x28)) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(esi + 0x28), eax (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_EQ(_fa, _fb)) goto loc_0017737B; /* je: equal / zero */

loc_0017736B: ;
    ecx = MEM32(esi + 0x14);
    MEM32(esi + 0x20) = 2;
    MEM32(esi + 0x28) = eax;
    MEM32(esi + 0x2C) = ecx;

loc_0017737B: ;
    edx = MEM32(esi + 0x68);
    ecx = esi + 0x68;
    { uint32_t _icall_esp = g_esp;
    { uint32_t _icall_target = MEM32(edx + 0x10); PUSH32(esp, 0x00177384u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x00177381u); } /* indirect call */
    }

loc_00177384: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(9) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 9 (32-bit) */
    _cf = (int)(_fa < _fb);
    ecx = esi + 0x68;
    if (CMP_NE(_fa, _fb)) goto loc_0017739A; /* jne: not equal / not zero */

loc_0017738C: ;
    eax = MEM32(ecx);
    { uint32_t _icall_esp = g_esp;
    { uint32_t _icall_target = MEM32(eax + 0x10); PUSH32(esp, 0x00177391u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x0017738Eu); } /* indirect call */
    }

loc_00177391: ;
    SET_LO8(ecx, MEM8(eax + eax * 4 + 0x4474B8));
    goto loc_001773AD;

loc_0017739A: ;
    edx = MEM32(ecx);
    edi = MEM32(esi + 0x58);
    { uint32_t _icall_esp = g_esp;
    { uint32_t _icall_target = MEM32(edx + 0x10); PUSH32(esp, 0x001773A2u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x0017739Fu); } /* indirect call */
    }

loc_001773A2: ;
    ecx = edi + eax * 4;
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(ecx) + (uint64_t)(eax)) >> 32) & 1);
    ecx = ecx + eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    SET_LO8(ecx, MEM8(ecx + 0x4474B8));

loc_001773AD: ;
    ecx = ZX8(LO8(ecx));
    _cf = 0; /* xor clears CF */
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x64) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x64 (32-bit) */
    _cf = (int)(_fa < _fb);
    POP32(esp, edi);
    if (CMP_GE(_fas, _fbs)) goto loc_001773D2; /* jge: greater or equal (signed >=) */

loc_001773B8: ;
    SET_LO8(ecx, mission_bits(ecx));   /* Missions: */
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(2) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ecx), 2 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (TEST_Z(_fa, _fb)) goto loc_001773C8; /* je: equal / zero */

loc_001773C3: ;
    _cf = 0; /* logical op clears CF */
    eax = eax | 0xFFFFFFFFu;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    goto loc_001773D2;

loc_001773C8: ;
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(1) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test LO8(ecx), 1 (8-bit) */
    _cf = 0; /* nothing borrows from zero */
    if (TEST_Z(_fa, _fb)) goto loc_001773D2; /* je: equal / zero */

loc_001773CD: ;
    eax = 1;

loc_001773D2: ;
    ecx = MEM32(esi + 0x4C);
    _cf = (int)((eax) != 0);
    eax = (uint32_t)(-(int32_t)eax);
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* neg result */
    eax = _cf ? 0xFFFFFFFF : 0; _fb = 0; /* sbb self (CF extend), OF clear */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sbb result */
    _fb = (uint32_t)(0x47) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    _cf = (int)((((uint64_t)(eax) + (uint64_t)(0x47)) >> 32) & 1);
    eax = eax + 0x47;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, eax (32-bit) */
    _cf = (int)(_fa < _fb);
    if (CMP_EQ(_fa, _fb)) goto loc_001773E9; /* je: equal / zero */

loc_001773E0: ;
    MEM32(esi + 0x4C) = eax;
    eax = MEM32(esi + 0x44);
    MEM32(esi + 0x50) = eax;

loc_001773E9: ;
    esp += 4; return; /* ret */

}

