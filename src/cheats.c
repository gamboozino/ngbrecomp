/*
 * cheats.c -- the RECOMP SETTINGS cheats, applied once a frame on the
 * game's thread (from Direct3D's Swap, src/d3d/d3d.c). They are for this
 * run only: not saved, off at the next start.
 *
 * Ryu's values (the player's block at 0x58A520, 0x210 bytes a player):
 * 0x58A564 the most health, 0x58A566 the health (16 bits; the HUD bar
 * follows it), 0x58A56C the most ninpo, 0x58A56E the ninpo. 0x589570 is
 * the essence (32 bits, the game keeps it at most 99999).
 *
 * The game guards these: 24 checksums (sub_00164E50, each over a part of the
 * state -- the inventory, health, ninpo, essence, ...) taken each frame, and
 * one changed behind its back (without the flag its own code sets with the
 * change, 0x7F1A94 + n) sends it back to the title screen. A frame a cheat
 * writes in, every flag is set: the checksums are taken anew.
 *
 * NGB_MEMDUMP=<dir>: debugging, for finding the game's values -- while the
 * file <dir>/dump.now exists, the guest's 64 MB go to <dir>/mem_<n>.bin and
 * the contiguous window (0x80000000, 128 MB: the item descriptors, GPU
 * memory) to <dir>/contig_<n>.bin (the file is removed after).
 */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "xbox_memory_layout.h"

/* A debugging option from the environment, read once (these are asked per sprite or per frame). */
#define ENV_ONCE(name) (__extension__({ static const char *v_ = (const char *)1; \
    if (v_ == (const char *)1) v_ = getenv(name); v_; }))


static void memdump_tick(void)
{
    static const char *dir = (const char *)1;
    static int n;
    if (dir == (const char *)1)
        dir = getenv("NGB_MEMDUMP");
    if (!dir)
        return;
    char trig[1024], path[1024];
    snprintf(trig, sizeof trig, "%s/dump.now", dir);
    FILE *t = fopen(trig, "r");
    if (!t)
        return;
    fclose(t);
    remove(trig);
    snprintf(path, sizeof path, "%s/mem_%d.bin", dir, n++);
    FILE *f = fopen(path, "wb");
    if (f) {
        fwrite((void *)XBOX_PTR(0), 1, 64u << 20, f);
        fclose(f);
        fprintf(stderr, "[CHEAT] memory dumped to %s\n", path);
    }
    snprintf(path, sizeof path, "%s/contig_%d.bin", dir, n - 1);
    if ((f = fopen(path, "wb"))) {
        fwrite((void *)XBOX_PTR(0x80000000u), 1, 128u << 20, f);
        fclose(f);
    }
}

/* NGB_POKE=addr:bytes:value,...: debugging, written every frame (hex). */
static void poke_tick(void)
{
    static const char *e = (const char *)1;
    if (e == (const char *)1)
        e = getenv("NGB_POKE");
    if (!e)
        return;
    static const char *gate = (const char *)1;   /* NGB_POKE_GATE=<file>: only while it exists */
    if (gate == (const char *)1)
        gate = getenv("NGB_POKE_GATE");
    if (gate) {
        FILE *g = fopen(gate, "r");
        if (!g)
            return;
        fclose(g);
    }
    const char *p = e;
    while (*p) {
        unsigned a, n, v;
        int used = 0;
        if (sscanf(p, "%x:%x:%x%n", &a, &n, &v, &used) != 3)
            break;
        if (n == 1) MEM8(a) = (uint8_t)v;
        else if (n == 2) MEM16(a) = (uint16_t)v;
        else MEM32(a) = v;
        p += used;
        if (*p == ',') p++;
    }
}

#define N_CHEATS 10
/* infinite health, infinite ninpo, infinite essence, golden scarabs, infinite items, infinite projectiles,
 * all weapons, all armlets, all technique scrolls, one hit kill */
static int s_cheat[N_CHEATS];

int ngb_cheat(int which) { return which >= 0 && which < N_CHEATS ? s_cheat[which] : 0; }
void ngb_cheat_set(int which, int on)
{
    static const char *const name[N_CHEATS] = { "infinite health", "infinite ninpo", "infinite essence", "golden scarabs",
                                                "infinite items", "infinite projectiles", "all weapons",
                                                "all armlets", "all technique scrolls", "one hit kill" };
    if (which < 0 || which >= N_CHEATS)
        return;
    s_cheat[which] = !!on;
    fprintf(stderr, "[CHEAT] %s %s\n", name[which], on ? "on" : "off");
}

/* A cheat changed the game's state: its checksums taken anew, not compared (see above). */
static void cheat_wrote(void)
{
    for (uint32_t a = 0x7F1A94; a < 0x7F1A94 + 24; a++)
        MEM8(a) = 1;
}

/* Infinite Items and Infinite Projectiles: the game takes an item away through one function,
 * sub_00154300 (eax the item, ebx the change, signed -- all 32 bits are read for its sign; a count
 * reaching 0 empties the slot) -- using a restorative from the inventory or by the game itself (the Talisman of Rebirth),
 * throwing or shooting. With the cheat on, a removal of such an item is made as a change of 0, so
 * the game's own path runs and its caller sees it done. What an item is: its descriptor (the table
 * at [0xDB98E4], [0xDB5E24] of them): +0x0C 1 or 3 ammunition (shuriken, incendiary shuriken, smoke
 * bombs; arrows, explosive arrows, spears), +0x18 its category (7 the bow's ammunition: the APFSDS
 * Cores have 0 at +0x0C, #103), +0x0D its class (5 an item), +0x1D its effect (0x12
 * health, 0x13 ninpo, 0x16 the Jewel of the Demon Seal (a Ninpo upgraded, #52), 0x22 the Talisman of
 * Rebirth, and the jewels that raise the limits for good -- 0x14 Life of the Gods, 0x17 Lives of the
 * Thousand Gods (health), 0x15 Spirit of the Devils (Ki): the game itself stops their use at its highest
 * limits (#52; left out until 0.9.46). */
static uint32_t item_record(uint16_t item)
{
    uint32_t base = MEM32(0xDB98E4), count = MEM32(0xDB5E24);
    if (!base || count > 1024 || item >= count)
        return 0;
    uint32_t o = MEM32(base + item * 4u);
    return o && o < 0x800000 - 0x40 ? base + o : 0;
}

static int item_kept(uint16_t item)
{
    uint32_t r = item_record(item);
    if (!r)
        return 0;
    uint8_t ammo = MEM8(r + 0x0C), cls = MEM8(r + 0x0D), effect = MEM8(r + 0x1D);
    if (s_cheat[5] && (ammo == 1 || ammo == 3 || MEM16(r + 0x18) == 7))
        return 1;
    return s_cheat[4] && cls == 5 &&
           (effect == 0x12 || effect == 0x13 || (effect >= 0x14 && effect <= 0x17) || effect == 0x22);
}

static void item_count_change(void);

static void weapons_seen(uint16_t item, int change);
static void scroll_given(uint16_t item, int change);

void sub_00154300(void)
{
    uint16_t item = (uint16_t)eax;
    if (ENV_ONCE("NGB_ITEM_LOG"))                     /* NGB_ITEM_LOG=1: every change of an item's count */
        fprintf(stderr, "[ITEM] %03X %+d\n", (unsigned)item, (int)ebx);
    if ((s_cheat[4] || s_cheat[5]) && (int32_t)ebx < 0 && item_kept(item)) {
        uint32_t keep = ebx;
        if (getenv("NGB_CHEAT_LOG"))
            fprintf(stderr, "[CHEAT] item %03X kept (%d)\n", (unsigned)item, (int)ebx);
        ebx = 0;
        item_count_change();
        ebx = keep;
        weapons_seen(item, 0);
        return;
    }
    int change = (int32_t)ebx;
    item_count_change();
    weapons_seen(item, change);
    scroll_given(item, change);
}

/* The game's sub_00154300 (generated, src/manual_functions.json) */
static void item_count_change(void)
{
    RECOMP_ENTRY(0x00154300u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_00154300: ;
    PUSH32(esp, ecx);
    PUSH32(esp, ebp);
    PUSH32(esp, esi);
    edx = edx | 0xFFFFFFFFu;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    MEM32(esp + 8) = 0;
    esi = 0; /* xor self */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    ecx = 0x5891D0;

loc_00154315: ;
    _fa = (uint32_t)(MEM8(ecx + 2)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(ecx + 2), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00154320; /* je: equal / zero */

loc_0015431B: ;
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(MEM16(ecx)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), MEM16(ecx) (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0015432E; /* je: equal / zero */

loc_00154320: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esi++;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x5893D0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x5893D0 (32-bit) */
    if (CMP_L(_fas, _fbs)) goto loc_00154315; /* jl: less (signed <) */

loc_0015432C: ;
    goto loc_00154330;

loc_0015432E: ;
    edx = esi;

loc_00154330: ;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebx, 0 (32-bit) */
    ebp = edx;
    if (CMP_GE(_fas, _fbs)) goto loc_0015437E; /* jge: greater or equal (signed >=) */

loc_00154336: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFFFFFFu) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0xFFFFFFFFu (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0015442B; /* je: equal / zero */

loc_0015433F: ;
    edx = edx << 2;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    SET_LO8(ecx, MEM8(edx + 0x5891D2));
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0xFF) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), 0xFF (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00154414; /* je: equal / zero */

loc_00154351: ;
    eax = ZX8(LO8(ecx));
    esi = ebx;
    esi = (uint32_t)(-(int32_t)esi);
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* neg result */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, eax (32-bit) */
    if (CMP_GE(_fas, _fbs)) goto loc_00154369; /* jge: greater or equal (signed >=) */

loc_0015435C: ;
    _fb = (uint32_t)(LO8(ebx)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    SET_LO8(ecx, LO8(ecx) + LO8(ebx));
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    MEM8(edx + 0x5891D2) = LO8(ecx);
    goto loc_00154414;

loc_00154369: ;
    MEM8(edx + 0x5891D2) = 0;
    MEM16(edx + 0x5891D0) = 0xFFFF;
    goto loc_00154414;

loc_0015437E: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFFFFFFu) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0xFFFFFFFFu (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001543C9; /* jne: not equal / not zero */

loc_00154383: ;
    edx = 0; /* xor self */
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    ecx = 0x5891D0;
    /* nop */

loc_00154390: ;
    _fa = (uint32_t)(MEM8(ecx + 2)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(ecx + 2), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001543AF; /* je: equal / zero */

loc_00154396: ;
    _fa = (uint32_t)(MEM16(ecx)) & 0xFFFFu; _fb = (uint32_t)(0xFFFF) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(ecx), 0xFFFF (16-bit) */
    if (CMP_AE(_fa, _fb)) goto loc_001543AF; /* jae: above or equal (unsigned >=) */

loc_0015439D: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + 4;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    edx++;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fb = (_fa == 0x80000000u); /* inc result/SF/OF; CF unchanged */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x5893D0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x5893D0 (32-bit) */
    if (CMP_L(_fas, _fbs)) goto loc_00154390; /* jl: less (signed <) */

loc_001543A9: ;
    POP32(esp, esi);
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    POP32(esp, ebp);
    POP32(esp, ecx);
    esp += 4; return; /* ret */

loc_001543AF: ;
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x80) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edx, 0x80 (32-bit) */
    if (CMP_GE(_fas, _fbs)) goto loc_001543A9; /* jge: greater or equal (signed >=) */

loc_001543B7: ;
    ebp = edx;
    MEM8(edx * 4 + 0x5891D2) = 0;
    MEM16(edx * 4 + 0x5891D0) = LO16(eax);

loc_001543C9: ;
    edx = ebp * 4;
    SET_LO8(ecx, MEM8(edx + 0x5891D2));
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0xFF) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), 0xFF (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_00154414; /* je: equal / zero */

loc_001543DB: ;
    esi = MEM32(0xDB5E24);
    eax = ZX16(LO16(eax));
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fb = (uint32_t)(eax) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp esi, eax (32-bit) */
    PUSH32(esp, edi);
    if (CMP_LE(_fas, _fbs)) goto loc_001543FA; /* jle: less or equal (signed <=) */

loc_001543E9: ;
    esi = MEM32(0xDB98E4);
    edi = MEM32(esi + eax * 4);
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp edi, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001543FA; /* je: equal / zero */

loc_001543F6: ;
    _fb = (uint32_t)(edi) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esi = esi + edi;
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    if ((_fa != 0)) goto loc_001543FE; /* jne: not equal / not zero */

loc_001543FA: ;
    SET_LO8(eax, 0); /* xor self */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* xor result */
    goto loc_00154401;

loc_001543FE: ;
    SET_LO8(eax, MEM8(esi + 0x24));

loc_00154401: ;
    _fb = (uint32_t)(LO8(ebx)) & 0xFFu; _fbs = (int32_t)(int8_t)(_fb); /* add source, before the write */
    SET_LO8(ecx, LO8(ecx) + LO8(ebx));
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* add result */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(LO8(ecx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), LO8(ecx) (8-bit) */
    MEM8(edx + 0x5891D2) = LO8(ecx);
    POP32(esp, edi);
    if (CMP_AE(_fa, _fb)) goto loc_00154414; /* jae: above or equal (unsigned >=) */

loc_0015440E: ;
    MEM8(edx + 0x5891D2) = LO8(eax);

loc_00154414: ;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fb = (uint32_t)(0xFFFFFFFFu) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ebp, 0xFFFFFFFFu (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0015442B; /* je: equal / zero */

loc_00154419: ;
    SET_LO8(eax, MEM8(edx + 0x5891D2));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_0015442B; /* je: equal / zero */

loc_00154423: ;
    MEM32(esp + 8) = 1;

loc_0015442B: ;
    eax = MEM32(esp + 8);
    POP32(esp, esi);
    MEM8(0x7F1A94) = 1;
    POP32(esp, ebp);
    POP32(esp, ecx);
    esp += 4; return; /* ret */

}

/* One Hit Kill: a hit that takes health from an enemy takes all of it. Every hit goes through one function,
 * sub_001E1680 (the target's object, the attacker's, the segment hit: Ryu's object is 0x4B4E50, the objects
 * 0x2800 bytes apart, +0x27F0 their health's block at 0x58A3A0 + 0x210 n: +0x194 eight segments (0xFFFF one
 * not had), +0x1C6 their sum; it returns 1 when the sum reaches 0, the enemy killed). A hit doing no damage
 * (guarded, or nothing to take) is left as the game has it; Ryu's are left alone. */
static void damage_apply(void);

void sub_001E1680(void)
{
    uint32_t target = MEM32(esp + 4);
    if (!s_cheat[9] || target == 0x4B4E50 || !MEM8(target + 0x27F0)) {
        damage_apply();
        return;
    }
    uint32_t block = MEM8(target + 0x27F0) * 0x210u + 0x58A3A0;
    uint16_t before = MEM16(block + 0x1C6);
    damage_apply();
    uint16_t after = MEM16(block + 0x1C6);
    if (!before || after >= before)
        return;
    for (int i = 0; i < 8; i++)
        if (MEM16(block + 0x194 + 2 * i) != 0xFFFF)
            MEM16(block + 0x194 + 2 * i) = 0;
    MEM16(block + 0x1C6) = 0;
    if (MEM8(0xD7D2B0) && MEM16(0xD7D2B2) == MEM16(target + 0x2774) && MEM16(0xD7D2B6) == after)   /* its health bar */
        MEM16(0xD7D2B6) = 0;
    SET_LO8(eax, 1);
    if (getenv("NGB_CHEAT_LOG"))
        fprintf(stderr, "[CHEAT] one hit: block %u %u -> 0\n", (unsigned)MEM8(target + 0x27F0), (unsigned)before);
}

/* The game's sub_001E1680 (generated, src/manual_functions.json) */
static void damage_apply(void)
{
    RECOMP_ENTRY(0x001E1680u);
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

loc_001E1680: ;
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    esp = esp - 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    eax = MEM32(esp + 0xC);
    SET_LO8(ecx, MEM8(eax + 0x278E));
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), 0 (8-bit) */
    PUSH32(esp, ebp);
    ebp = MEM32(esp + 0x14);
    MEM8(esp + 7) = 0;
    if (CMP_EQ(_fa, _fb)) goto loc_001E16A2; /* je: equal / zero */

loc_001E169B: ;
    SET_LO8(eax, 0); /* xor self */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* xor result */
    POP32(esp, ebp);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 4; return; /* ret */

loc_001E16A2: ;
    PUSH32(esp, edi);
    edi = ZX8(MEM8(eax + 0x27F0));
    { int64_t _r = (int64_t)(int32_t)(edi) * (int64_t)(int32_t)(0x210);
      edi = (uint32_t)_r;
      _fa = (uint32_t)_r & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); _fb = (_r != (int64_t)(int32_t)_r); } /* imul */
    _fb = (uint32_t)(0x58A3A0) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edi = edi + 0x58A3A0;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    SET_LO8(ecx, MEM8(edi + 0x1F4));
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), 0 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001E16C8; /* jne: not equal / not zero */

loc_001E16C0: ;
    POP32(esp, edi);
    SET_LO8(eax, 0); /* xor self */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* xor result */
    POP32(esp, ebp);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 4; return; /* ret */

loc_001E16C8: ;
    SET_LO8(edx, MEM8(eax + 0x2791));
    PUSH32(esp, esi);
    esi = 0; /* xor self */
    _fa = (uint32_t)(esi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    SET_LO16(esi, MEM16(eax + 0x2774));
    PUSH32(esp, 0);
    PUSH32(esp, 0x001E16DFu); RECOMP_ABI_CALL(0x0016A7E0u, sub_0016A7E0); /* call 0x0016A7E0 */

loc_001E16DF: ;
    edx = ZX8(LO8(eax));
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    PUSH32(esp, 0x001E16EAu); RECOMP_ABI_CALL(0x001E2410u, sub_001E2410); /* call 0x001E2410 */

loc_001E16EA: ;
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001E16F7; /* je: equal / zero */

loc_001E16EE: ;
    POP32(esp, esi);
    POP32(esp, edi);
    SET_LO8(eax, 0); /* xor self */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* xor result */
    POP32(esp, ebp);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 4; return; /* ret */

loc_001E16F7: ;
    _fa = (uint32_t)(MEM8(ebp + 0x278E)) & 0xFFu; _fb = (uint32_t)(2) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp MEM8(ebp + 0x278E), 2 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001E195C; /* jne: not equal / not zero */

loc_001E1704: ;
    eax = ZX8(MEM8(ebp + 0x27F0));
    ecx = MEM32(esp + 0x18);
    esi = eax + eax * 2;
    xmm4 = XMM_SCALAR(MEMF(0x4148C8)); /* movss */
    esi = esi * 4 + 0xDB9AE0;
    PUSH32(esp, ebx);
    SET_LO16(ebx, MEM16(esi + 4));
    eax = ZX16(LO16(ebx));
    xmm1.f[0] = (float)(int32_t)eax; /* cvtsi2ss */
    xmm1.f[0] = xmm1.f[0] * MEMF(esi + 8); /* mulss */
    _fb = (uint32_t)(0x4B4E50) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    ecx = ecx - 0x4B4E50;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    eax = 0x66666667;
    { int64_t _r = (int64_t)(int32_t)eax * (int64_t)(int32_t)ecx;
      eax = (uint32_t)_r; edx = (uint32_t)(_r >> 32); _fa = eax; _fas = (int32_t)eax; _fb = (_r != (int64_t)(int32_t)_r); }
    edx = (uint32_t)(((int32_t)(int32_t)(edx)) >> ((0xC) & 31u));
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sar result */
    ecx = edx;
    ecx = ecx >> 0x1F;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    ecx = ecx + edx;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    xmm1.f[0] = xmm1.f[0] * xmm5.f[0]; /* mulss */
    xmm3 = xmm4; /* movaps */
    xmm2 = xmm4; /* movaps */
    if ((_fa != 0)) goto loc_001E1762; /* jne: not equal / not zero */

loc_001E1755: ;
    PUSH32(esp, 0xA);
    PUSH32(esp, 0x001E175Cu); RECOMP_ABI_CALL(0x00154C20u, sub_00154C20); /* call 0x00154C20 */

loc_001E175C: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    xmm2 = xmm0; /* movaps */

loc_001E1762: ;
    _fb = (uint32_t)(0x4B4E50) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    ebp = ebp - 0x4B4E50;
    _fa = (uint32_t)(ebp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    eax = 0x66666667;
    { int64_t _r = (int64_t)(int32_t)eax * (int64_t)(int32_t)ebp;
      eax = (uint32_t)_r; edx = (uint32_t)(_r >> 32); _fa = eax; _fas = (int32_t)eax; _fb = (_r != (int64_t)(int32_t)_r); }
    edx = (uint32_t)(((int32_t)(int32_t)(edx)) >> ((0xC) & 31u));
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sar result */
    eax = edx;
    eax = eax >> 0x1F;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + edx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    eax = ZX8(LO8(eax));
    ecx = eax + eax * 4;
    ecx = ecx << 0xB;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    edx = ZX8(MEM8(ecx + 0x4B7640));
    { int64_t _r = (int64_t)(int32_t)(edx) * (int64_t)(int32_t)(0xE90);
      edx = (uint32_t)_r;
      _fa = (uint32_t)_r & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); _fb = (_r != (int64_t)(int32_t)_r); } /* imul */
    SET_LO8(eax, MEM8(edx + 0x7D4BC5));
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(eax), 0 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001E17A6; /* jne: not equal / not zero */

loc_001E1799: ;
    PUSH32(esp, 9);
    PUSH32(esp, 0x001E17A0u); RECOMP_ABI_CALL(0x00154C20u, sub_00154C20); /* call 0x00154C20 */

loc_001E17A0: ;
    _fb = (uint32_t)(4) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 4;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    xmm3 = xmm0; /* movaps */

loc_001E17A6: ;
    _fa = (uint32_t)(LO16(ebx)) & 0xFFFFu; _fb = (uint32_t)(0) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(ebx), 0 (16-bit) */
    xmm0 = XMM_ZERO(); /* xorps self = zero */
    xmm2.f[0] = xmm2.f[0] * xmm3.f[0]; /* mulss */
    xmm2.f[0] = xmm2.f[0] * xmm1.f[0]; /* mulss */
    MEMF(esp + 0x14) = xmm2.f[0]; /* movss */
    if (CMP_EQ(_fa, _fb)) goto loc_001E17DC; /* je: equal / zero */

loc_001E17BC: ;
    _fca = xmm0.f[0]; _fcb = MEMF(esi + 8); /* ucomiss */
    eax = (eax & 0xFFFF00FFu) | ((uint32_t)((((_fca == _fcb || (_fca != _fca || _fcb != _fcb))) ? 0x40u : 0u) | (((_fca != _fca || _fcb != _fcb)) ? 0x04u : 0u) | (((_fca < _fcb || (_fca != _fca || _fcb != _fcb))) ? 0x01u : 0u) | 0x02u) << 8); /* lahf (ucomiss) */
    _fa = (uint32_t)(HI8(eax)) & 0xFFu; _fb = (uint32_t)(0x44) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(eax), 0x44 (8-bit) */
    if ((!RECOMP_PARITY8((_fa) & (_fb)))) goto loc_001E17DC; /* jnp: not parity */

loc_001E17C6: ;
    _fca = xmm0.f[0]; _fcb = xmm5.f[0]; /* ucomiss */
    eax = (eax & 0xFFFF00FFu) | ((uint32_t)((((_fca == _fcb || (_fca != _fca || _fcb != _fcb))) ? 0x40u : 0u) | (((_fca != _fca || _fcb != _fcb)) ? 0x04u : 0u) | (((_fca < _fcb || (_fca != _fca || _fcb != _fcb))) ? 0x01u : 0u) | 0x02u) << 8); /* lahf (ucomiss) */
    _fa = (uint32_t)(HI8(eax)) & 0xFFu; _fb = (uint32_t)(0x44) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* test HI8(eax), 0x44 (8-bit) */
    if ((!RECOMP_PARITY8((_fa) & (_fb)))) goto loc_001E17DC; /* jnp: not parity */

loc_001E17CF: ;
    _fca = xmm4.f[0]; _fcb = xmm2.f[0]; /* comiss */
    if ((_fca <= _fcb || (_fca != _fca || _fcb != _fcb))) goto loc_001E17E2; /* jbe: below or equal (unsigned <=) (xmm4.f[0] vs xmm2.f[0]) */

loc_001E17D4: ;
    MEMF(esp + 0x14) = xmm4.f[0]; /* movss */
    goto loc_001E17E2;

loc_001E17DC: ;
    MEMF(esp + 0x14) = xmm0.f[0]; /* movss */

loc_001E17E2: ;
    fp_push(MEMF(esp + 0x14)); /* fld float */
    PUSH32(esp, 0x001E17EBu); RECOMP_ABI_CALL(0x0036FDB8u, sub_0036FDB8); /* call 0x0036FDB8 */

loc_001E17EB: ;
    SET_LO8(ecx, MEM8(0xD7D2BC));
    SET_LO8(ebx, 1);
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(LO8(ebx)) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), LO8(ebx) (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001E1814; /* je: equal / zero */

loc_001E17F7: ;
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(2) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), 2 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001E1816; /* jne: not equal / not zero */

loc_001E17FC: ;
    ecx = MEM32(esp + 0x1C);
    _fa = (uint32_t)(MEM16(ecx + 0x2774)) & 0xFFFFu; _fb = (uint32_t)(0) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(ecx + 0x2774), 0 (16-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001E1816; /* jne: not equal / not zero */

loc_001E180A: ;
    SET_LO8(ecx, MEM8(edi + 0x1F6));
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), 0 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001E1816; /* jne: not equal / not zero */

loc_001E1814: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */

loc_001E1816: ;
    edx = ZX8(MEM8(esp + 0x24));
    SET_LO16(ecx, MEM16(edi + edx * 2 + 0x194));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(LO16(ecx)) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), LO16(ecx) (16-bit) */
    edx = edi + edx * 2 + 0x194;
    if (CMP_B(_fa, _fb)) goto loc_001E1836; /* jb: below (unsigned <) */

loc_001E182F: ;
    MEM16(edx) = 0;
    goto loc_001E183B;

loc_001E1836: ;
    _fb = (uint32_t)(eax) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    ecx = ecx - eax;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    MEM16(edx) = LO16(ecx);

loc_001E183B: ;
    MEM16(edi + 0x1C6) = 0;
    SET_LO16(eax, MEM16(edi + 0x194));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0xFFFF) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0xFFFF (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001E1858; /* je: equal / zero */

loc_001E1851: ;
    MEM16(edi + 0x1C6) = LO16(eax);

loc_001E1858: ;
    SET_LO16(eax, MEM16(edi + 0x196));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0xFFFF) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0xFFFF (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001E186C; /* je: equal / zero */

loc_001E1865: ;
    _fb = (uint32_t)(LO16(eax)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    MEM16(edi + 0x1C6) = MEM16(edi + 0x1C6) + LO16(eax);
    _fa = (uint32_t)(MEM16(edi + 0x1C6)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */

loc_001E186C: ;
    SET_LO16(eax, MEM16(edi + 0x198));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0xFFFF) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0xFFFF (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001E1880; /* je: equal / zero */

loc_001E1879: ;
    _fb = (uint32_t)(LO16(eax)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    MEM16(edi + 0x1C6) = MEM16(edi + 0x1C6) + LO16(eax);
    _fa = (uint32_t)(MEM16(edi + 0x1C6)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */

loc_001E1880: ;
    SET_LO16(eax, MEM16(edi + 0x19A));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0xFFFF) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0xFFFF (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001E1894; /* je: equal / zero */

loc_001E188D: ;
    _fb = (uint32_t)(LO16(eax)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    MEM16(edi + 0x1C6) = MEM16(edi + 0x1C6) + LO16(eax);
    _fa = (uint32_t)(MEM16(edi + 0x1C6)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */

loc_001E1894: ;
    SET_LO16(eax, MEM16(edi + 0x19C));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0xFFFF) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0xFFFF (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001E18A8; /* je: equal / zero */

loc_001E18A1: ;
    _fb = (uint32_t)(LO16(eax)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    MEM16(edi + 0x1C6) = MEM16(edi + 0x1C6) + LO16(eax);
    _fa = (uint32_t)(MEM16(edi + 0x1C6)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */

loc_001E18A8: ;
    SET_LO16(eax, MEM16(edi + 0x19E));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0xFFFF) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0xFFFF (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001E18BC; /* je: equal / zero */

loc_001E18B5: ;
    _fb = (uint32_t)(LO16(eax)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    MEM16(edi + 0x1C6) = MEM16(edi + 0x1C6) + LO16(eax);
    _fa = (uint32_t)(MEM16(edi + 0x1C6)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */

loc_001E18BC: ;
    SET_LO16(eax, MEM16(edi + 0x1A0));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0xFFFF) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0xFFFF (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001E18D0; /* je: equal / zero */

loc_001E18C9: ;
    _fb = (uint32_t)(LO16(eax)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    MEM16(edi + 0x1C6) = MEM16(edi + 0x1C6) + LO16(eax);
    _fa = (uint32_t)(MEM16(edi + 0x1C6)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */

loc_001E18D0: ;
    SET_LO16(eax, MEM16(edi + 0x1A2));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0xFFFF) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0xFFFF (16-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001E18E4; /* je: equal / zero */

loc_001E18DD: ;
    _fb = (uint32_t)(LO16(eax)) & 0xFFFFu; _fbs = (int32_t)(int16_t)(_fb); /* add source, before the write */
    MEM16(edi + 0x1C6) = MEM16(edi + 0x1C6) + LO16(eax);
    _fa = (uint32_t)(MEM16(edi + 0x1C6)) & 0xFFFFu; _fas = (int32_t)(int16_t)(_fa); /* add result */

loc_001E18E4: ;
    SET_LO16(eax, MEM16(edi + 0x1C6));
    _fa = (uint32_t)(LO16(eax)) & 0xFFFFu; _fb = (uint32_t)(0) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp LO16(eax), 0 (16-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001E18F4; /* jne: not equal / not zero */

loc_001E18F0: ;
    MEM8(esp + 0x13) = LO8(ebx);

loc_001E18F4: ;
    ecx = MEM32(esp + 0x1C);
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0x4B4E50) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0x4B4E50 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001E1932; /* je: equal / zero */

loc_001E1900: ;
    SET_LO16(ecx, MEM16(ecx + 0x2774));
    SET_LO16(edx, MEM16(edi + 0x1C4));
    MEM8(0xD7D2B0) = LO8(ebx);
    MEM32(0xD7D2B8) = 0x12C;
    MEM16(0xD7D2B2) = LO16(ecx);
    MEM16(0xD7D2B4) = LO16(edx);
    MEM16(0xD7D2B6) = LO16(eax);

loc_001E1932: ;
    _fb = (uint32_t)(0x58A3A0) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    edi = edi - 0x58A3A0;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    eax = 0x3E0F83E1;
    { int64_t _r = (int64_t)(int32_t)eax * (int64_t)(int32_t)edi;
      eax = (uint32_t)_r; edx = (uint32_t)(_r >> 32); _fa = eax; _fas = (int32_t)eax; _fb = (_r != (int64_t)(int32_t)_r); }
    edx = (uint32_t)(((int32_t)(int32_t)(edx)) >> ((7) & 31u));
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sar result */
    eax = edx;
    eax = eax >> 0x1F;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + edx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    if ((_fa != 0)) goto loc_001E1951; /* jne: not equal / not zero */

loc_001E194B: ;
    MEM8(0x7F1AA3) = LO8(ebx);

loc_001E1951: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_001E195B; /* jne: not equal / not zero */

loc_001E1955: ;
    MEM8(0x7F1AA5) = LO8(ebx);

loc_001E195B: ;
    POP32(esp, ebx);

loc_001E195C: ;
    SET_LO8(eax, MEM8(esp + 0xF));
    POP32(esp, esi);
    POP32(esp, edi);
    POP32(esp, ebp);
    _fb = (uint32_t)(8) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    esp = esp + 8;
    _fa = (uint32_t)(esp) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    esp += 4; return; /* ret */

    #undef fp_push
    #undef fp_pop
    #undef fp_top
    #undef fp_st
    #undef fp_st1
}


/* Golden Scarabs: enough held that, with those already brought to Muramasa (0x589574, at most 50),
 * they make 50 -- each of his rewards at the next visit. The inventory: 0x80 slots of 4 bytes at
 * 0x5891D0 (item, count, -; a count of 0 or item 0xFFFF an empty slot); the scarab is item 0x59.
 * In play only (Ryu has health). */
static void scarab_tick(void)
{
    uint32_t brought = MEM32(0x589574);
    if (brought >= 50 || !MEM16(0x58A564))
        return;
    uint32_t want = 50 - brought, slot = 0, empty = 0;
    for (uint32_t a = 0x5891D0; a < 0x5891D0 + 4 * 0x80; a += 4) {
        if (MEM16(a) == 0x59 && MEM8(a + 2))
            slot = a;
        else if (!empty && (!MEM8(a + 2) || MEM16(a) == 0xFFFF))
            empty = a;
    }
    if (slot && MEM8(slot + 2) >= want)
        return;
    if (!slot && !(slot = empty))
        return;                                        /* the bag is full */
    MEM16(slot) = 0x59;
    MEM8(slot + 2) = (uint8_t)want;
    cheat_wrote();
    if (getenv("NGB_CHEAT_LOG"))
        fprintf(stderr, "[CHEAT] golden scarabs: %u held (%u brought)\n", want, brought);
}

/* All Weapons: every melee and projectile weapon Black gives, at its highest level, put in the inventory
 * (as scarab_tick does) or raised to it, with a full stack of what it fires when none is held. A
 * weapon's count is its level (+0x1E of its descriptor added); Lunar and Kitetsu are one item a level
 * (0x12B, 0x133, 0x134 and 0x182, 0x0CC, 0x183), so a lower one held becomes the top one, and one picked
 * up as usual with the cheat on goes into it. The original's Bow (0x08D) and Plasma Saber (0x0CE) are
 * left out: Black has the Strongbow and the Mk.II. A stack (incendiary shuriken, smoke bombs, what the
 * bows and the spear gun fire) is given once, not again when used up: Infinite Projectiles is the cheat
 * for that.
 *
 * Turned off, it takes back what it gave: each item's state from before the cheat touched it is kept
 * (s_had), with what the game itself gives or takes meanwhile added to it (sub_00154300, the pickups),
 * and put back -- a weapon never had taken away, a Lunar picked up at its own level. A stack is left at
 * what is held when that is less (the cheat's share is used first). A weapon in Ryu's hands is put back
 * once he holds another, so the game never loses the one it draws (a level is changed in his hands, as
 * Muramasa does). Nothing is given or taken under the pause menu: its weapon lists were made when it
 * opened. In play only (Ryu has health).
 *
 * The slots the cheat writes carry its mark (s_mark, new for each record) in their fourth byte, which
 * the game leaves alone and keeps in a save. A record is only ever applied to slots with its mark: a
 * bag with none of them (another save loaded) ends it, and an item whose slot lost it (the story's
 * own changes, not through sub_00154300) is left to the game.
 *
 * All Armlets and All Technique Scrolls are the same for the armlets and bands (class 4 in their records) and
 * the technique scrolls (category 11: the moves go by the scroll in the bag); each list goes by its own cheat
 * (group: 6 + it). A scroll or armlet the cheat holds would make the game refuse the story's own ("can only
 * carry one": the scroll from chapter 2's bridge boss, #167; the Armlet of Celerity in chapter 10, #212):
 * scrolls_carry lets two be carried meanwhile, and the story's goes in as the player's (scroll_given).
 * The game gives them through sub_00154300 like any
 * pickup (its script's give, sub_002D4590, checks nothing held first), so one found with the cheat on is the
 * player's and stays. A worn armlet taken back is taken off too (+4 of the player's block, 0xFFFF none). The
 * Intercept scroll (0x13C) is left out: Black never gives it. */
static const struct { uint16_t item; uint8_t count, stack; uint16_t also[2]; uint8_t group; } s_weapons[] = {
    { 0x001, 3 },                           /* Dragon Sword */
    { 0x0C9, 1 },                           /* True Dragon Sword */
    { 0x026, 1 },                           /* Nunchaku */
    { 0x134, 1, 0, { 0x12B, 0x133 } },      /* Lunar */
    { 0x044, 2 },                           /* War Hammer */
    { 0x095, 3 },                           /* Dabilahro */
    { 0x0CA, 2 },                           /* Vigoorian Flail */
    { 0x183, 1, 0, { 0x182, 0x0CC } },      /* Kitetsu */
    { 0x0E1, 1 },                           /* Unlabored Flawlessness */
    { 0x0CD, 1 },                           /* Dark Dragon Blade */
    { 0x0F7, 1 },                           /* Plasma Saber Mk.II */
    { 0x098, 1 },                           /* Spear Gun */
    { 0x099, 0, 1 },                        /* spears */
    { 0x000, 1 },                           /* Shuriken */
    { 0x030, 1 },                           /* Windmill Shuriken */
    { 0x008, 0, 1 },                        /* Incendiary Shuriken */
    { 0x165, 0, 1 },                        /* Smoke Bomb */
    { 0x0F8, 1 },                           /* Strongbow */
    { 0x08E, 0, 1 },                        /* arrows */
    { 0x0E2, 1 },                           /* the bow with explosive arrows */
    { 0x0E3, 0, 1 },                        /* explosive arrows */
    { 0x029, 1 },                           /* the bow with APFSDS cores */
    { 0x02A, 0, 1 },                        /* APFSDS cores */
    { 0x050, 1, 0, { 0 }, 1 },              /* the Armlet of Tranquility */
    { 0x052, 1, 0, { 0 }, 1 },              /* the Armlet of Fortune */
    { 0x054, 1, 0, { 0 }, 1 },              /* the Armlet of the Sun */
    { 0x0EB, 1, 0, { 0 }, 1 },              /* the Armlet of the Moon */
    { 0x055, 1, 0, { 0 }, 1 },              /* the Armlet of Benediction */
    { 0x166, 1, 0, { 0 }, 1 },              /* the Armlet of Celerity */
    { 0x167, 1, 0, { 0 }, 1 },              /* the Armlet of Potency */
    { 0x174, 1, 0, { 0 }, 1 },              /* the Band of Life */
    { 0x178, 1, 0, { 0 }, 1 },              /* the Band of Spirit */
    { 0x175, 1, 0, { 0 }, 1 },              /* the Band of Power */
    { 0x176, 1, 0, { 0 }, 1 },              /* the Band of Protection */
    { 0x177, 1, 0, { 0 }, 1 },              /* the Band of Wealth */
    { 0x179, 1, 0, { 0 }, 1 },              /* the Band of Light */
    { 0x17A, 1, 0, { 0 }, 1 },              /* the Band of Strength */
    { 0x0CF, 1, 0, { 0 }, 2 },              /* Technique Scroll: Izuna Drop */
    { 0x0D0, 1, 0, { 0 }, 2 },              /* Technique Scroll: Counter Attacks */
    { 0x0D1, 1, 0, { 0 }, 2 },              /* Technique Scroll: Guillotine Throw */
};
#define N_WEAPONS (sizeof s_weapons / sizeof s_weapons[0])

/* What the player had of each, from before the cheat touched it */
static struct { uint8_t seen, wrote, count; uint16_t item; } s_had[N_WEAPONS];
static uint8_t s_mark;

/* a slot the cheat wrote: its mark in the slot's fourth byte */
static void weapons_mark(size_t i, uint32_t slot)
{
    MEM8(slot + 3) = s_mark;
    s_had[i].wrote = 1;
    cheat_wrote();
}

static uint32_t inventory_slot(uint16_t item)
{
    for (uint32_t a = 0x5891D0; a < 0x5891D0 + 4 * 0x80; a += 4)
        if (MEM16(a) == item && MEM8(a + 2))
            return a;
    return 0;
}

static unsigned item_most(uint16_t item)
{
    uint32_t r = item_record(item);
    return r ? MEM8(r + 0x24) : 0;
}

static unsigned item_level(uint16_t item)
{
    uint32_t r = item_record(item);
    return r ? MEM8(r + 0x1E) : 0;
}

static int is_family(size_t i, uint16_t item)
{
    return item == s_weapons[i].item || (s_weapons[i].also[0] && item == s_weapons[i].also[0]) ||
           (s_weapons[i].also[1] && item == s_weapons[i].also[1]);
}

/* the slot holding s_weapons[i] or a lower one of it */
static uint32_t family_slot(size_t i)
{
    uint32_t slot = inventory_slot(s_weapons[i].item);
    for (int k = 0; k < 2 && !slot && s_weapons[i].also[k]; k++)
        slot = inventory_slot(s_weapons[i].also[k]);
    return slot;
}

/* in Ryu's hands: the melee weapon or the projectile weapon he has out */
static int equipped(uint16_t item)
{
    uint32_t p = 0x58A570 + MEM8(0x4B7640) * 0x210u;
    return MEM16(p) == item || MEM16(p + 2) == item;
}

static void weapons_log(const char *what, uint16_t item, unsigned count)
{
    if (getenv("NGB_CHEAT_LOG"))
        fprintf(stderr, "[CHEAT] all weapons: %s %03X x%u\n", what, item, count);
}

/* A change the game made to an item's count (eax, ebx of sub_00154300): the player's own, kept with
 * what they had. A pickup is theirs; a weapon taken away (the story) is gone from what they had too. */
static void weapons_seen(uint16_t item, int change)
{
    for (size_t i = 0; i < N_WEAPONS; i++) {
        if (!s_had[i].seen || !is_family(i, item))
            continue;
        if (change > 0) {
            if (s_had[i].count && s_had[i].item != item) {
                unsigned most = item_most(item), n = (unsigned)change > most ? most : (unsigned)change;
                if (item_level(item) > item_level(s_had[i].item))
                    s_had[i].item = item, s_had[i].count = (uint8_t)n;
            } else {
                unsigned n = s_had[i].count + (unsigned)change, most = item_most(item);
                s_had[i].item = item;
                s_had[i].count = (uint8_t)(n > most ? most : n);
            }
        } else if (change < 0 && !s_weapons[i].stack && s_had[i].count && s_had[i].item == item) {
            int n = s_had[i].count + change;
            s_had[i].count = (uint8_t)(n > 0 ? n : 0);
        }
        if (!family_slot(i))
            s_had[i].wrote = 0;                        /* nothing of the cheat's left to look for */
    }
}

/* the cheat's items whose cheat is off, each back to what the player had (one in Ryu's hands left for later) */
static void weapons_restore(void)
{
    for (size_t i = 0; i < N_WEAPONS; i++) {
        if (!s_had[i].seen || s_cheat[6 + s_weapons[i].group])
            continue;
        uint32_t slot = family_slot(i);
        if (s_had[i].wrote && slot) {
            if (equipped(MEM16(slot)) && (!s_had[i].count || s_had[i].item != MEM16(slot)))
                continue;                              /* in his hands: when he holds another (a level is
                                                          changed in his hands, as Muramasa does) */
            if (!s_had[i].count) {
                uint32_t worn = 0x58A570 + MEM8(0x4B7640) * 0x210u + 4;
                if (s_weapons[i].group == 1 && MEM16(worn) == MEM16(slot))
                    MEM16(worn) = 0xFFFF;              /* the armlet he wears: taken off */
                weapons_log("taken back", MEM16(slot), MEM8(slot + 2));
                MEM8(slot + 2) = 0;
                MEM8(slot + 3) = 0;
                MEM16(slot) = 0xFFFF;
            } else {
                unsigned n = s_had[i].count;
                if (s_weapons[i].stack && MEM8(slot + 2) < n)
                    n = MEM8(slot + 2);
                MEM16(slot) = s_had[i].item;
                MEM8(slot + 2) = (uint8_t)n;
                MEM8(slot + 3) = 0;
                weapons_log("put back", s_had[i].item, n);
            }
            cheat_wrote();
        }
        s_had[i].seen = s_had[i].wrote = 0;
    }
}

/* A technique scroll or armlet All Scrolls / All Armlets gave in an earlier session, kept by a save made while it
 * was on: its slot still carries that session's mark (any mark but this record's). Only where the game waits on
 * the story's own, which a held one made it refuse ("can only carry one"): every technique scroll (the bridge boss
 * of chapter 2, #167) and the Armlet of Celerity, whose pedestal is the switch out of the wall-run room in chapter
 * 10 (#212). With its cheat on it is this session's now (marked again, the player's own state "none", so turning
 * the cheat off takes it; a worn one stays on); with the cheat off it is taken back, off Ryu's arm if he wears it.
 * Other armlets and the weapons a save kept stay the player's, as the cheat says: nothing the story gives waits on
 * them, and one the story gave under an older build with the cheat on would carry the mark too. A mark
 * left in a slot the game emptied is cleared, so an item the game puts there later is not taken for the cheat's. */
static void scrolls_left_over(void)
{
    for (uint32_t a = 0x5891D0; a < 0x5891D0 + 4 * 0x80; a += 4)
        if ((!MEM8(a + 2) || MEM16(a) == 0xFFFF) && MEM8(a + 3)) {
            MEM8(a + 3) = 0;
            cheat_wrote();
        }
    for (size_t i = 0; i < N_WEAPONS; i++) {
        if (!s_weapons[i].group)
            continue;
        uint32_t slot = inventory_slot(s_weapons[i].item);
        uint8_t mark = slot ? MEM8(slot + 3) : 0;
        if (!slot || !(mark & 0x80) || (s_had[i].wrote && mark == s_mark))
            continue;
        if (s_weapons[i].group == 1 && s_weapons[i].item != 0x166)
            continue;                                  /* (left as it was: the player's, cheat on or off) */
        if (s_cheat[6 + s_weapons[i].group]) {
            if (!s_had[i].seen) {
                s_had[i].seen = 1;
                s_had[i].item = 0xFFFF;
                s_had[i].count = 0;
            }
            weapons_mark(i, slot);
            weapons_log("left over from an earlier session, this session's now", MEM16(slot), MEM8(slot + 2));
            continue;
        }
        uint32_t worn = 0x58A570 + MEM8(0x4B7640) * 0x210u + 4;
        if (s_weapons[i].group == 1 && MEM16(worn) == MEM16(slot))
            MEM16(worn) = 0xFFFF;
        weapons_log("left over from an earlier session, taken back", MEM16(slot), MEM8(slot + 2));
        MEM8(slot + 2) = 0;
        MEM8(slot + 3) = 0;
        MEM16(slot) = 0xFFFF;
        cheat_wrote();
    }
}

/* A technique scroll or armlet the game itself gives (a pickup, a chest, the story) into a slot the cheat had
 * marked: the player's now, one again, the mark gone -- so neither turning the cheat off nor scrolls_left_over
 * takes it */
static void scroll_given(uint16_t item, int change)
{
    if (change <= 0)
        return;
    for (size_t i = 0; i < N_WEAPONS; i++)
        if (s_weapons[i].group && s_weapons[i].item == item) {
            uint32_t slot = inventory_slot(item);
            if (slot && MEM8(slot + 3)) {
                MEM8(slot + 3) = 0;
                if (MEM8(slot + 2) > 1)
                    MEM8(slot + 2) = 1;                /* (two only while scrolls_carry allowed them) */
                cheat_wrote();
            }
        }
}

/* While All Scrolls' or All Armlets' own copy of a scroll or armlet is in the bag, its record says two can be
 * carried (+0x24), so the game takes the story's instead of refusing it; back to the game's own limit otherwise. */
static void scrolls_carry(void)
{
    static uint8_t own[N_WEAPONS];   /* the game's limit, as first seen */
    for (size_t i = 0; i < N_WEAPONS; i++) {
        if (!s_weapons[i].group)
            continue;
        uint32_t r = item_record(s_weapons[i].item), slot = inventory_slot(s_weapons[i].item);
        if (!r)
            continue;
        if (!own[i])
            own[i] = MEM8(r + 0x24) ? MEM8(r + 0x24) : 1;
        uint8_t want = slot && s_had[i].wrote && MEM8(slot + 3) == s_mark ? (uint8_t)(own[i] + 1) : own[i];
        if (MEM8(r + 0x24) != want)
            MEM8(r + 0x24) = want;
    }
}

static void weapons_tick(void)
{
    if (!MEM16(0x58A564))
        return;
    int wrote = 0, marked = 0, seen = 0;
    for (size_t i = 0; i < N_WEAPONS; i++)
        wrote |= s_had[i].wrote, seen |= s_had[i].seen;
    for (uint32_t a = 0x5891D0; wrote && a < 0x5891D0 + 4 * 0x80; a += 4)
        marked |= MEM8(a + 2) && MEM16(a) != 0xFFFF && MEM8(a + 3) == s_mark;
    if (wrote && !marked) {
        if (getenv("NGB_CHEAT_LOG"))
            fprintf(stderr, "[CHEAT] all weapons: none of its slots in the bag (another save), the record ends\n");
        memset(s_had, 0, sizeof s_had);
        seen = 0;
    }
    for (size_t i = 0; i < N_WEAPONS; i++) {
        uint32_t slot = s_had[i].wrote ? family_slot(i) : 0;
        if (s_had[i].wrote && (!slot || MEM8(slot + 3) != s_mark))
            s_had[i].wrote = 0;                        /* changed by the game itself: its own now */
    }
    if (MEM8(0xDB9AD5))                                /* under the pause menu: its lists stay true */
        return;
    if (!seen && (s_cheat[6] || s_cheat[7] || s_cheat[8])) {   /* a new record, a new mark (before
                                                                  * scrolls_left_over adopts with it) */
        static unsigned n;
        s_mark = (uint8_t)(0x80 | ((unsigned)time(NULL) + 37u * ++n) % 0x7F);
    }
    weapons_restore();
    scrolls_left_over();
    scrolls_carry();
    if (!s_cheat[6] && !s_cheat[7] && !s_cheat[8])
        return;
    for (size_t i = 0; i < N_WEAPONS; i++) {
        uint16_t item = s_weapons[i].item;
        uint32_t slot = family_slot(i);
        if (!s_cheat[6 + s_weapons[i].group])
            continue;
        if (!s_had[i].seen) {
            if (slot && equipped(MEM16(slot)) && MEM16(slot) != item)
                continue;                              /* a lower one in his hands: when he holds another */
            s_had[i].seen = 1;
            s_had[i].item = slot ? MEM16(slot) : 0xFFFF;
            s_had[i].count = slot ? MEM8(slot + 2) : 0;
            if (s_weapons[i].stack && slot)
                continue;                              /* a stack held: left as it is */
        } else if (s_weapons[i].stack) {
            continue;                                  /* given once */
        }
        /* a lower one picked up beside the top one (weapons_seen has it as the player's): into it */
        for (int k = 0; k < 2 && s_weapons[i].also[k]; k++) {
            uint32_t lower = inventory_slot(s_weapons[i].also[k]);
            if (!lower || equipped(s_weapons[i].also[k]))
                continue;
            if (s_had[i].seen && (!s_had[i].count || item_level(MEM16(lower)) > item_level(s_had[i].item)))
                s_had[i].item = MEM16(lower), s_had[i].count = MEM8(lower + 2);
            uint32_t top = inventory_slot(item);
            if (top) {
                MEM8(lower + 2) = 0;
                MEM8(lower + 3) = 0;
                MEM16(lower) = 0xFFFF;
                weapons_mark(i, top);
            } else {
                MEM16(lower) = item;
                weapons_mark(i, lower);
            }
        }
        unsigned want = s_weapons[i].count ? s_weapons[i].count : item_most(item), most = item_most(item);
        if (!want || !item_record(item))
            continue;
        if (want > most)
            want = most;
        if (want > 254)
            want = 254;                                /* 255: a count the game never changes */
        slot = inventory_slot(item);
        if (slot && MEM8(slot + 2) >= want)
            continue;
        for (uint32_t a = 0x5891D0; !slot && a < 0x5891D0 + 4 * 0x80; a += 4)
            if (!MEM8(a + 2) || MEM16(a) == 0xFFFF)
                slot = a;
        if (!slot)
            continue;                                  /* the bag is full */
        MEM16(slot) = item;
        MEM8(slot + 2) = (uint8_t)want;
        weapons_mark(i, slot);
        weapons_log("given", item, want);
    }
}

/* Ryu's health filled (Infinite Health): the segments (0x58A534 + 2i, at most 0x58A524 + 2i; 0xFFFF a
 * segment not had) -- damage lowers them -- and 0x58A566, their sum, which the game makes again
 * from them. True when anything was filled. Each frame: a blow taking all of it at once still kills
 * (the game sees 0 before the next fill). */
static int health_fill(void)
{
    uint16_t most = MEM16(0x58A564);
    if (!s_cheat[0] || !most || most >= 0x8000)
        return 0;
    int filled = 0;
    for (int i = 0; i < 8; i++) {
        uint16_t m = MEM16(0x58A524 + 2 * i), c = MEM16(0x58A534 + 2 * i);
        if (m != 0xFFFF && m && c != m) {
            MEM16(0x58A534 + 2 * i) = m;
            filled = 1;
        }
    }
    if (MEM16(0x58A566) != most) {
        MEM16(0x58A566) = most;
        filled = 1;
    }
    if (filled)
        cheat_wrote();
    return filled;
}

static void cheat_tick(void)
{
    static int env_read;
    if (!env_read) {                                   /* NGB_CHEATS=111: debugging, on at start */
        const char *e = getenv("NGB_CHEATS");
        for (int i = 0; e && i < N_CHEATS && e[i]; i++)
            s_cheat[i] = e[i] == '1';
        env_read = 1;
    }
    if (health_fill()) {                               /* Infinite Health */
        static int told;
        if (!told++ && getenv("NGB_CHEAT_LOG"))
            fprintf(stderr, "[CHEAT] health filled\n");
    }
    uint16_t most = MEM16(0x58A56C);
    if (s_cheat[1] && most && most < 0x8000 && MEM16(0x58A56E) < most) {
        MEM16(0x58A56E) = most;
        cheat_wrote();
    }
    if (s_cheat[2] && MEM32(0x589570) < 99999) {
        MEM32(0x589570) = 99999;
        cheat_wrote();
    }
    if (s_cheat[3])
        scarab_tick();
    weapons_tick();                                   /* All Weapons, or taking it back */
}

void ngb_aspect_tick(void);   /* src/pc_menu.c */

/* ---- debugging: where Ryu is, and moving him ----
 *
 * Ryu's position is the vector at 0x4B5710 (x y z 1; copies of it, for the frames before, at
 * 0x4B5720/30, 0x4B5F70/A0, 0x4B6730/60, 0x4B6EF0/F20). F8 appends it to ngb_positions.txt in the
 * save directory (NGB_SAVE_DIR or ~/.local/share/NinjaGaidenBlack). NGB_TELEPORT=<file>: when the
 * file appears holding "x y z", Ryu is put there (every copy of his position in his block) and the
 * file removed. */
volatile int ngb_pos_request;

static void pos_tick(void)
{
    if (ngb_pos_request) {
        ngb_pos_request = 0;
        char path[1024];
        const char *dir = getenv("NGB_SAVE_DIR"), *home = getenv("HOME");
        if (dir) snprintf(path, sizeof path, "%s/ngb_positions.txt", dir);
        else snprintf(path, sizeof path, "%s/.local/share/NinjaGaidenBlack/ngb_positions.txt", home ? home : ".");
        FILE *f = fopen(path, "a");
        if (f) {
            fprintf(f, "%.3f %.3f %.3f\n", MEMF(0x4B5710), MEMF(0x4B5714), MEMF(0x4B5718));
            fclose(f);
        }
        fprintf(stderr, "[POS] %.3f %.3f %.3f -> %s\n", MEMF(0x4B5710), MEMF(0x4B5714), MEMF(0x4B5718), path);
    }
    static const char *tp = (const char *)1;
    if (tp == (const char *)1)
        tp = getenv("NGB_TELEPORT");
    if (!tp)
        return;
    FILE *f = fopen(tp, "r");
    if (!f)
        return;
    float x, y, z;
    int ok = fscanf(f, "%f %f %f", &x, &y, &z) == 3;
    fclose(f);
    remove(tp);
    if (!ok)
        return;
    float ox = MEMF(0x4B5710), oy = MEMF(0x4B5714), oz = MEMF(0x4B5718);
    int n = 0;
    for (uint32_t a = 0x4B5000; a < 0x4B8000; a += 16)
        if (MEMF(a) == ox && MEMF(a + 4) == oy && MEMF(a + 8) == oz) {
            MEMF(a) = x; MEMF(a + 4) = y; MEMF(a + 8) = z;
            n++;
        }
    fprintf(stderr, "[POS] teleported from %.3f %.3f %.3f to %.3f %.3f %.3f (%d copies)\n", ox, oy, oz, x, y, z, n);
}

void ngb_aspect_frame_end(void);   /* src/pc_menu.c */

void ngb_frame_tick(void)
{
    { extern void ngb_menu_frame(void); ngb_menu_frame(); }   /* src/menu_mouse.c: the frames, for the controls */
    { extern void ngb_camera_frame(void); ngb_camera_frame(); }   /* src/camera.c: which view the stick turns */
    { extern void ngb_quick_tick(void); ngb_quick_tick(); }
    { extern void ngb_render_list_frame(void); ngb_render_list_frame(); }   /* src/render_list.c: its nodes back */
    pos_tick();
    { extern void ngb_intercept_frame(void); ngb_intercept_frame(); }   /* src/intercept.c: Ryu's Intercept moves */
    { extern void ngb_ut_charge_frame(void); ngb_ut_charge_frame(); }   /* src/ut_charge.c: Essence-Only Charging */
    { extern void ngb_responsive_frame(void); ngb_responsive_frame(); }   /* src/responsive.c: Responsive Menus */
    { extern void ngb_fps_tick(void); ngb_fps_tick(); }   /* src/fps_text.c: the frame counter's lettering */
    { extern void ngb_version_tick(void); ngb_version_tick(); }   /* src/fps_text.c: the version in the main menu */
#ifdef __ANDROID__
    { extern void ngb_android_frame(void); ngb_android_frame(); }   /* src/android_main.c: the clocks' hint */
#endif
    ngb_aspect_frame_end();
    ngb_aspect_tick();
    cheat_tick();
    memdump_tick();
    poke_tick();
#ifndef _WIN32
    { extern void compat_fault_report(void); compat_fault_report(); }   /* NGB_FAULT_COUNT: faults by place */
#endif
}

/* ---- every difficulty from the start ----
 *
 * The game's flags for its menus (sub_001DB520, ecx the flag, eax 1 when
 * set): 6 Normal (always), 7-9 Hard, Very Hard and Master Ninja (bytes
 * 0xDB9472-0xDB9474 of the system data: each difficulty cleared unlocks the
 * next), 5 Ninja Dog (never here: the game offers it after deaths in chapter
 * 1), 1-4 the costumes. NEW GAME shows the difficulty selection when more
 * than Normal is set. With the setting on Ninja Dog through Master Ninja read
 * as set -- nothing is written,
 * so the player's own record (system.dat) is untouched and turning it off
 * puts back what they have earned. */
static int s_all_difficulties, s_all_costumes;
void ngb_set_all_difficulties(int on) { s_all_difficulties = !!on; }
/* Every costume (flags 1-4: the game's Costume Selection at New Game, as after clearing each
 * difficulty), the same way -- and the three it has but never offers (src/costume_menu.c). */
void ngb_set_all_costumes(int on) { s_all_costumes = !!on; }
/* The main menu's MISSIONS, ARCADE NINJA GAIDEN and MOVIE THEATER (flags 10, 11 and 12, set in the system
 * data as the game earns them), the same way: read as set, nothing written. which 0-2 in that order. */
static int s_unlock[3];
void ngb_set_unlock(int which, int on) { if (which >= 0 && which < 3) s_unlock[which] = !!on; }
int ngb_unlock(int which) { return which >= 0 && which < 3 ? s_unlock[which] : 0; }

/**
 * sub_001DB520
 * Original: 0x001DB520 - 0x001DB596 (118 bytes, 33 insns)
 * Category: game_vtable
 * CC: thiscall, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
static void game_flag(void)
{
    RECOMP_ENTRY(0x001DB520u);
    uint32_t ebp = 0;
    ebp = g_ebp;  /* frameless: caller's frame */
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;
    ebp = g_seh_ebp; /* fpo_leaf: inherit caller's frame */

loc_001DB520: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xE) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0xE (32-bit) */
    if (CMP_GE(_fas, _fbs)) goto loc_001DB595; /* jge: greater or equal (signed >=) */

loc_001DB527: ;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fb = (uint32_t)(0xD) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp ecx, 0xD (32-bit) */
    if (CMP_A(_fa, _fb)) goto loc_001DB595; /* ja: above (unsigned >) */

loc_001DB52C: ;
    { uint32_t _jt = MEM32(ecx * 4 + 0x1DB598); /* switch: 14 entries, 13 targets */
    if (_jt == 0x001DB533u) goto loc_001DB533;
    if (_jt == 0x001DB53Bu) goto loc_001DB53B;
    if (_jt == 0x001DB543u) goto loc_001DB543;
    if (_jt == 0x001DB54Bu) goto loc_001DB54B;
    if (_jt == 0x001DB553u) goto loc_001DB553;
    if (_jt == 0x001DB556u) goto loc_001DB556;
    if (_jt == 0x001DB55Eu) goto loc_001DB55E;
    if (_jt == 0x001DB566u) goto loc_001DB566;
    if (_jt == 0x001DB56Eu) goto loc_001DB56E;
    if (_jt == 0x001DB576u) goto loc_001DB576;
    if (_jt == 0x001DB57Eu) goto loc_001DB57E;
    if (_jt == 0x001DB586u) goto loc_001DB586;
    if (_jt == 0x001DB590u) goto loc_001DB590;
    g_seh_ebp = ebp; RECOMP_ITAIL(_jt); return; }

loc_001DB533: ;
    SET_LO8(ecx, MEM8(0xDB941C));
    goto loc_001DB58C;

loc_001DB53B: ;
    SET_LO8(ecx, MEM8(0xDB941D));
    goto loc_001DB58C;

loc_001DB543: ;
    SET_LO8(ecx, MEM8(0xDB941E));
    goto loc_001DB58C;

loc_001DB54B: ;
    SET_LO8(ecx, MEM8(0xDB941F));
    goto loc_001DB58C;

loc_001DB553: ;
    eax = 0; /* xor self */
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* xor result */
    esp += 4; return; /* ret */

loc_001DB556: ;
    SET_LO8(ecx, MEM8(0xDB9472));
    goto loc_001DB58C;

loc_001DB55E: ;
    SET_LO8(ecx, MEM8(0xDB9473));
    goto loc_001DB58C;

loc_001DB566: ;
    SET_LO8(ecx, MEM8(0xDB9474));
    goto loc_001DB58C;

loc_001DB56E: ;
    SET_LO8(ecx, MEM8(0xDB956A));
    goto loc_001DB58C;

loc_001DB576: ;
    SET_LO8(ecx, MEM8(0xDB9417));
    goto loc_001DB58C;

loc_001DB57E: ;
    SET_LO8(ecx, MEM8(0xDB9416));
    goto loc_001DB58C;

loc_001DB586: ;
    SET_LO8(ecx, MEM8(0xDB9418));

loc_001DB58C: ;
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), 0 (8-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_001DB595; /* je: equal / zero */

loc_001DB590: ;
    eax = 1;

loc_001DB595: ;
    esp += 4; return; /* ret */

}

void sub_001DB520(void)
{
    if (ENV_ONCE("NGB_FLAG_LOG"))
        fprintf(stderr, "[FLAG] %u (all difficulties %d)\n", ecx, s_all_difficulties);
    if ((s_all_difficulties && ecx >= 5 && ecx <= 9) || (s_all_costumes && ecx >= 1 && ecx <= 4) ||
        (ecx >= 10 && ecx <= 12 && s_unlock[ecx - 10])) {
        eax = 1;
        esp += 4;   /* ret */
        return;
    }
    game_flag();
}

/* Talisman of Rebirth (Gameplay, setting 43): sub_002FD3E0 is asked, as Ryu's health reaches 0, whether he
 * dies (al 1); with a Talisman of Rebirth (item 0xFB, counted by sub_001542C0) it brings him back
 * (sub_002FD470) and takes one away (al 0). Off: never used, Ryu dies with them kept. */
int ngb_pc_setting(int which);

/**
 * sub_002FD3E0
 * Original: 0x002FD3E0 - 0x002FD46A (138 bytes, 55 insns)
 * CC: cdecl, 0 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_002FD3E0(void)
{
    RECOMP_ENTRY(0x002FD3E0u);
    int _flags = 0; /* fallback flag var */
    uint32_t _fa = 0, _fb = 0;
    int32_t _fas = 0, _fbs = 0;
    (void)_fa; (void)_fb; (void)_fas; (void)_fbs;

loc_002FD3E0: ;
    PUSH32(esp, ebx);
    PUSH32(esp, esi);
    esi = eax;
    ecx = esi;
    _fb = (uint32_t)(0x4B4E50) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* sub source, before the write */
    ecx = ecx - 0x4B4E50;
    _fa = (uint32_t)(ecx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sub result */
    eax = 0x66666667;
    { int64_t _r = (int64_t)(int32_t)eax * (int64_t)(int32_t)ecx;
      eax = (uint32_t)_r; edx = (uint32_t)(_r >> 32); _fa = eax; _fas = (int32_t)eax; _fb = (_r != (int64_t)(int32_t)_r); }
    edx = (uint32_t)(((int32_t)(int32_t)(edx)) >> ((0xC) & 31u));
    _fa = (uint32_t)(edx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* sar result */
    eax = edx;
    eax = eax >> 0x1F;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* shift result */
    _fb = (uint32_t)(edx) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    eax = eax + edx;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    SET_LO8(ebx, 1);
    if ((_fas < 0)) goto loc_002FD465; /* js: sign (negative) */

loc_002FD401: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0x40) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0x40 (32-bit) */
    if (CMP_GE(_fas, _fbs)) goto loc_002FD465; /* jge: greater or equal (signed >=) */

loc_002FD406: ;
    PUSH32(esp, edi);
    edi = ZX8(MEM8(esi + 0x27F0));
    { int64_t _r = (int64_t)(int32_t)(edi) * (int64_t)(int32_t)(0x210);
      edi = (uint32_t)_r;
      _fa = (uint32_t)_r & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); _fb = (_r != (int64_t)(int32_t)_r); } /* imul */
    _fb = (uint32_t)(0x58A3A0) & 0xFFFFFFFFu; _fbs = (int32_t)(int32_t)(_fb); /* add source, before the write */
    edi = edi + 0x58A3A0;
    _fa = (uint32_t)(edi) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* add result */
    SET_LO8(ecx, MEM8(edi + 0x1F4));
    _fa = (uint32_t)(LO8(ecx)) & 0xFFu; _fb = (uint32_t)(0) & 0xFFu;
    _fas = (int32_t)(int8_t)(_fa); _fbs = (int32_t)(int8_t)(_fb); /* cmp LO8(ecx), 0 (8-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_002FD42A; /* jne: not equal / not zero */

loc_002FD424: ;
    POP32(esp, edi);
    POP32(esp, esi);
    SET_LO8(eax, 1);
    POP32(esp, ebx);
    esp += 4; return; /* ret */

loc_002FD42A: ;
    _fa = (uint32_t)(MEM16(edi + 0x1C6)) & 0xFFFFu; _fb = (uint32_t)(0) & 0xFFFFu;
    _fas = (int32_t)(int16_t)(_fa); _fbs = (int32_t)(int16_t)(_fb); /* cmp MEM16(edi + 0x1C6), 0 (16-bit) */
    if (CMP_BE(_fa, _fb)) goto loc_002FD43A; /* jbe: below or equal (unsigned <=) */

loc_002FD434: ;
    POP32(esp, edi);
    POP32(esp, esi);
    SET_LO8(eax, 0); /* xor self */
    _fa = (uint32_t)(LO8(eax)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* xor result */
    POP32(esp, ebx);
    esp += 4; return; /* ret */

loc_002FD43A: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_NE(_fa, _fb)) goto loc_002FD424; /* jne: not equal / not zero */

loc_002FD43E: ;
    esi = 0xFB;
    if (!ngb_pc_setting(43))                     /* Talisman of Rebirth Off: as if none were had -- Ryu dies */
        goto loc_002FD45F;                       /* (bl still 1: dead) */
    PUSH32(esp, 0x002FD448u); RECOMP_ABI_CALL(0x001542C0u, sub_001542C0); /* call 0x001542C0 */

loc_002FD448: ;
    _fa = (uint32_t)(eax) & 0xFFFFFFFFu; _fb = (uint32_t)(0) & 0xFFFFFFFFu;
    _fas = (int32_t)(int32_t)(_fa); _fbs = (int32_t)(int32_t)(_fb); /* cmp eax, 0 (32-bit) */
    if (CMP_EQ(_fa, _fb)) goto loc_002FD45F; /* je: equal / zero */

loc_002FD44C: ;
    ecx = edi;
    PUSH32(esp, 0x002FD453u); RECOMP_ABI_CALL(0x002FD470u, sub_002FD470); /* call 0x002FD470 */

loc_002FD453: ;
    ebx = ebx | 0xFFFFFFFFu;
    _fa = (uint32_t)(ebx) & 0xFFFFFFFFu; _fas = (int32_t)(int32_t)(_fa); /* or result */
    eax = esi;
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, 0x002FD45Du); RECOMP_ICALL_SAFE(0x00154300u, _icall_esp); /* manual call 0x00154300 */
    }

loc_002FD45D: ;
    SET_LO8(ebx, 0); /* xor self */
    _fa = (uint32_t)(LO8(ebx)) & 0xFFu; _fas = (int32_t)(int8_t)(_fa); /* xor result */

loc_002FD45F: ;
    POP32(esp, edi);
    POP32(esp, esi);
    SET_LO8(eax, LO8(ebx));
    POP32(esp, ebx);
    esp += 4; return; /* ret */

loc_002FD465: ;
    POP32(esp, esi);
    SET_LO8(eax, 1);
    POP32(esp, ebx);
    esp += 4; return; /* ret */

}

