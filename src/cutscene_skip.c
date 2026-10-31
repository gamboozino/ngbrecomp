/*
 * cutscene_skip.c -- Skip Any Cutscene (Gameplay, #139): Start skips an in-game cutscene not yet watched.
 *
 * sub_001DBEE0 runs the game's events, the in-game cutscenes among them, once a frame. Start skips the one
 * playing when Start is down on the pad (bit 0x10 of the player's buttons at 0xD219A8, 0x78 a player), the event
 * has been watched before (a byte an event from 0xDB9477, by the event's number at 0xDB9C6E; the chapter 1
 * waterfall is 0x6E) and its script has made it skippable (0x8D3A66, set and cleared by the script's own
 * commands in sub_001DD300; 0xDB9888 non-zero besides). The body is the lifted one, kept here (listed in
 * src/manual_functions.json so the pipeline does not generate it), with the last of those tests passed while
 * the option is on. The watched bytes are left alone, so the saves are the same with it on or off. Movies are
 * src/movie_skip.c's. Responsive Menus (src/responsive.c) may count a Start pressed before the script allowed
 * the skip as pressed once it does.
 */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"

int ngb_pc_setting(int which);   /* src/ingame_menu.c: 34 Skip Any Cutscene */
void ngb_responsive_event_start(void);   /* src/responsive.c */
void ngb_responsive_paused(void);
int ngb_responsive_cutscene(void);

/**
 * sub_001DBEE0
 * Original: 0x001DBEE0 - 0x001DC11E (574 bytes, 155 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: standard_frame
 */
void sub_001DBEE0(void)
{
    RECOMP_ENTRY(0x001DBEE0u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* prologue saves caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    int kept = 0;

loc_001DBEE0: ;
    PUSH32(esp, ebp);
    ebp = esp;
    g_ebp = ebp; /* publish frame for frameless callees */
    g_seh_ebp = ebp;
    esp = esp & 0xFFFFFFF8u;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    _fb = (uint32_t)(0x10) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 0x10;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    eax = MEM32(0xDB9890);
    PUSH32(esp, ebx);
    PUSH32(esp, ebp);
    ebp = 0; /* xor self */
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(ebp) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, ebp (32-bit) */
    PUSH32(esp, esi);
    PUSH32(esp, edi);
    MEM32(esp + 0x18) = 0xC;
    if (CMP_NE(_fa, _fb)) goto loc_001DBF11; /* jne: not equal / not zero */

loc_001DBF00: ;
    eax = esp + 0x18;
    PUSH32(esp, 2);
    PUSH32(esp, eax);
    SET_LO8(ecx, 1);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001DBF0Eu); RECOMP_ABI_CALL(0x0021A3E0u, sub_0021A3E0); /* call 0x0021A3E0 */

loc_001DBF0E: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */

loc_001DBF11: ;
    ecx = MEM32(0x977104);
    eax = 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, eax (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001DBF25; /* je: equal / zero */

loc_001DBF20: ;
    MEM32(0x97767C) = eax;

loc_001DBF25: ;
    edi = 1;
    MEM32(0xD21E70) = edi;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001DBF35u); RECOMP_ABI_CALL(0x001DC120u, sub_001DC120); /* call 0x001DC120 */

loc_001DBF35: ;
    ngb_responsive_event_start();   /* (src/responsive.c: a new event, nothing kept from the last) */
    SET_LO8(ebx, MEM8(0xDB9AD5));
    esi = 0; /* xor self */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    MEM32(esp + 0x14) = edi;
    MEM8(0xD7D2BD) = 0;
    MEM32(esp + 0x10) = ebp;
    MEM8(0xD52B3D) = 0;
    MEM32(esp + 0x18) = ebp;

loc_001DBF57: ;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(ebp) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, ebp (32-bit) */
    MEM32(esp + 0x1C) = esi;
    if (CMP_EQ(_fa, _fb)) goto loc_001DBFB2; /* je: equal / zero */

loc_001DBF5F: ;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001DBF64u); RECOMP_ABI_CALL(0x001DC3C0u, sub_001DC3C0); /* call 0x001DC3C0 */

loc_001DBF64: ;
    eax = MEM32(0xD21E4C);
    ecx = MEM32(0x9570F0);
    eax = eax << 5;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(0xD21C00) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0xD21C00;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    ebx = MEM32(eax);
    ebx = ebx & 0xFFFFFFEFu;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, ecx);
    MEM32(eax) = ebx;
    MEM32(eax + 4) = edi;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001DBF87u); RECOMP_ICALL_SAFE(0x0032A293u, _icall_esp); /* manual call 0x0032A293 */
    }

loc_001DBF87: ;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(2) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 2 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001DBFAA; /* je: equal / zero */

loc_001DBF8C: ;
    SET_LO16(eax, MEM16(0x8D3A5E));
    MEM16(0x8D3A5C) = MEM16(0x8D3A5C) + 1;
    _fa = (uint32_t)(MEM16(0x8D3A5C)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fb = (_fa == 0x8000u); /* inc result/SF/OF; CF unchanged */
    SET_LO16(eax, LO16(eax) + 1);
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fb = (_fa == 0x8000u); /* inc result/SF/OF; CF unchanged */
    edx = ZX16(LO16(eax));
    MEM16(0x8D3A5E) = LO16(eax);
    MEM32(0xDB9AD0) = edx;

loc_001DBFAA: ;
    SET_LO8(ebx, MEM8(0xDB9AD5));
    esi = 0; /* xor self */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */

loc_001DBFB2: ;
    _fa = (uint32_t)(LO8(ebx)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ebx), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001DBFF7; /* je: equal / zero */

loc_001DBFB6: ;
    _fa = (uint32_t)(MEM32(esp + 0x10)) & 0xFFFFFFFFu; _fb = (uint32_t)(ebp) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(esp + 0x10), ebp (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001DBFF7; /* jne: not equal / not zero */

loc_001DBFBC: ;
    ngb_responsive_paused();   /* (src/responsive.c: the pause menu's Start is not a press for the event) */
    eax = MEM32(0xD21E4C);
    eax = eax << 5;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(0xD21C00) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0xD21C00;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    MEM32(eax) = MEM32(eax) & 0xFFFFFFEFu;
    _fa = (uint32_t)(MEM32(eax)) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* and result */
    MEM32(eax + 4) = edi;
    eax = MEM32(0x9570F0);
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, eax);
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001DBFDAu); RECOMP_ICALL_SAFE(0x0032A293u, _icall_esp); /* manual call 0x0032A293 */
    }

loc_001DBFDA: ;
    SET_LO8(ebx, MEM8(0xDB9AD5));
    _fa = (uint32_t)(MEM8(0xD7D2BD)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(0xD7D2BD), LO8(ebx) (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001DC0E4; /* je: equal / zero */

loc_001DBFEC: ;
    MEM8(0xD7D2BD) = LO8(ebx);
    goto loc_001DC0E4;

loc_001DBFF7: ;
    eax = (uint32_t)(int32_t)SMEM16(0x8D3A50);
    _fb = (uint32_t)(ebp) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    eax = eax - ebp;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    if ((_fa == 0)) goto loc_001DC00C; /* je: equal / zero */

loc_001DC002: ;
    eax--;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x7FFFFFFFu); /* dec result/SF/OF; CF unchanged */
    if ((_fa != 0)) goto loc_001DC01B; /* jne: not equal / not zero */

loc_001DC005: ;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001DC00Au); RECOMP_ABI_CALL(0x001DC400u, sub_001DC400); /* call 0x001DC400 */

loc_001DC00A: ;
    goto loc_001DC015;

loc_001DC00C: ;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001DC011u); RECOMP_ABI_CALL(0x001DCC30u, sub_001DCC30); /* call 0x001DCC30 */

loc_001DC011: ;
    MEM32(esp + 0x14) = ebp;

loc_001DC015: ;
    SET_LO8(ebx, MEM8(0xDB9AD5));

loc_001DC01B: ;
    _fa = (uint32_t)(MEM16(0x8D3A50)) & 0xFFFFu; _fb = (uint32_t)(LO16(ebp)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(0x8D3A50), LO16(ebp) (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001DC0E4; /* je: equal / zero */

loc_001DC028: ;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001DC02Du); RECOMP_ABI_CALL(0x001DCD60u, sub_001DCD60); /* call 0x001DCD60 */

loc_001DC02D: ;
    kept = ngb_responsive_cutscene();   /* Responsive Menus: a press kept from before it was skippable */
    esi = eax;
    SET_LO8(eax, MEM8(0xDB9888));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001DC0AB; /* je: equal / zero */

loc_001DC038: ;
    ecx = ZX8(MEM8(0xDB932E));
    { int64_t _r = (int64_t)(int32_t)(ecx) * (int64_t)(int32_t)(0x78);
      ecx = (uint32_t)_r;
      _fa = (uint32_t)_r & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); _fb = (_r != (int64_t)(int32_t)_r); } /* imul */
    _fa = (uint32_t)(MEM8(ecx + 0xD219A8)) & 0xFFu; _fb = (uint32_t)(0x10) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test MEM8(ecx + 0xD219A8), 0x10 (8-bit) */
    if (TEST_Z(_fa, _fb) && !kept) goto loc_001DC0AB; /* je: equal / zero */

loc_001DC04B: ;
    edx = ZX16(MEM16(0xDB9C6E));
    SET_LO8(eax, MEM8(edx + 0xDB9477));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    /* the one change: with Skip Any Cutscene on, a cutscene not yet watched skips too */
    if (CMP_EQ(_fa, _fb) && !ngb_pc_setting(34)) goto loc_001DC0AB; /* je: equal / zero */

loc_001DC05C: ;
    _fa = (uint32_t)(MEM16(0x8D3A66)) & 0xFFFFu; _fb = (uint32_t)(LO16(edi)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(0x8D3A66), LO16(edi) (16-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001DC0AB; /* jne: not equal / not zero */

loc_001DC065: ;
    SET_LO16(eax, MEM16(0x8D3A54));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(2) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 2 (16-bit) */
    MEM16(0x8D3A66) = 2;
    if (CMP_EQ(_fa, _fb)) goto loc_001DC0AB; /* je: equal / zero */

loc_001DC07A: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(6) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 6 (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001DC0AB; /* je: equal / zero */

loc_001DC080: ;
    eax = MEM32(0x8D3A70);
    MEM32(esp + 0x18) = eax;
    ecx = 0; /* xor self */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    eax = esp + 0x18;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001DC094u); RECOMP_ABI_CALL(0x001DD690u, sub_001DD690); /* call 0x001DD690 */

loc_001DC094: ;
    MEM32(0x8D3A70) = eax;
    MEM16(0x8D3A50) = LO16(edi);
    MEM16(0x8D3A66) = LO16(ebp);
    MEM32(esp + 0x18) = edi;

loc_001DC0AB: ;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(2) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 2 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001DC0C0; /* jne: not equal / not zero */

loc_001DC0B0: ;
    _fa = (uint32_t)(LO8(ebx)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ebx), 0 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001DC0C0; /* jne: not equal / not zero */

loc_001DC0B4: ;
    MEM8(0xDB5A68) = MEM8(0xDB5A68) + 1;
    _fa = (uint32_t)(MEM8(0xDB5A68)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fb = (_fa == 0x80u); /* inc result/SF/OF; CF unchanged */
    MEM32(esp + 0x10) = edi;
    goto loc_001DC0E4;

loc_001DC0C0: ;
    _fa = (uint32_t)(MEM32(esp + 0x1C)) & 0xFFFFFFFFu; _fb = (uint32_t)(2) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(esp + 0x1C), 2 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001DC0E4; /* jne: not equal / not zero */

loc_001DC0C7: ;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(2) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, 2 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001DC0E4; /* je: equal / zero */

loc_001DC0CC: ;
    _fa = (uint32_t)(LO8(ebx)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ebx), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001DC0E4; /* je: equal / zero */

loc_001DC0D0: ;
    SET_LO8(eax, MEM8(0xDB5A68));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_LE(_fas, _fbs)) goto loc_001DC0E0; /* jle: less or equal (signed <=) */

loc_001DC0D9: ;
    SET_LO8(eax, LO8(eax) - 1);
    _fa = (uint32_t)(LO8(eax)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fb = (_fa == 0x7Fu); /* dec result/SF/OF; CF unchanged */
    MEM8(0xDB5A68) = LO8(eax);

loc_001DC0E0: ;
    MEM32(esp + 0x10) = ebp;

loc_001DC0E4: ;
    _fa = (uint32_t)(MEM32(esp + 0x14)) & 0xFFFFFFFFu; _fb = (uint32_t)(ebp) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(esp + 0x14), ebp (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001DBF57; /* jne: not equal / not zero */

loc_001DC0EE: ;
    _fa = (uint32_t)(MEM32(esp + 0x18)) & 0xFFFFFFFFu; _fb = (uint32_t)(ebp) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp MEM32(esp + 0x18), ebp (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001DC0FB; /* je: equal / zero */

loc_001DC0F4: ;
    MEM8(0xD52B3D) = 1;

loc_001DC0FB: ;
    eax = MEM32(0xD21E4C);
    ecx = MEM32(0x9570F0);
    eax = eax << 5;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(0xD21C00) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + 0xD21C00;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, ecx);
    MEM32(eax) = ebp;
    g_ebp = ebp; /* frame stays current across calls */
    g_seh_ebp = ebp;
    PUSH32(esp, 0x001DC116u); RECOMP_ICALL_SAFE(0x0032A293u, _icall_esp); /* manual call 0x0032A293 */
    }

loc_001DC116: ;
    POP32(esp, edi);
    POP32(esp, esi);
    POP32(esp, ebp);
    POP32(esp, ebx);
    esp = ebp;
    POP32(esp, ebp);
    esp += 4; return; /* ret */

}

