/*
 * kitetsu_levels.c -- the Kitetsu's sheathe sound at every level (#229).
 *
 * Ryu's motion scripts (sub_00188F30) gate a weapon's extras by what he has equipped: opcode 0x23, case 1, asks
 * sub_00154240 whether an item is in one of his 8 equipped slots (the player block's 0x58A570) and runs the next
 * block if so. The sheathe sounds are such blocks, one per sword: 0x40000102 then 0x40000103 for the Dragon Sword
 * (0x001), the Kitetsu (0x0CC) and the True Dragon Sword (0x0C9). Black made the Kitetsu one item a level (0x182,
 * 0x0CC, 0x183), but its scripts still name only 0x0CC (102 checks in cmn.afs and the chr_ files, none for 0x182 or
 * 0x183), so at its first and last level the Kitetsu put away silently. The lifted function, kept here
 * (src/manual_functions.json), takes either of those for 0x0CC.
 */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"

/**
 * sub_00154240
 * Original: 0x00154240 - 0x00154285
 * CC: ecx = character index (low byte), esi = item (low 16 bits); eax = 1 when equipped, else 0
 * Frame: fpo_leaf
 */
void sub_00154240(void)
{
    RECOMP_ENTRY(0x00154240u);

    ecx = ZX8(LO8(ecx)) * 0x2800;
    SET_LO8(edx, MEM8(ecx + 0x4B75DE));
    eax = 0;
    if (LO8(edx) != 0) {   /* not a player */
        esp += 4; return;
    }
    ecx = ZX8(MEM8(ecx + 0x4B7640)) * 0x210 + 0x58A570;
    uint16_t want = LO16(esi);
    for (edx = 0; (int32_t)edx < 8; edx++, ecx += 2) {
        uint16_t have = MEM16(ecx);
        if (have == want || (want == 0x0CC && (have == 0x182 || have == 0x183))) {
            eax = 1;
            break;
        }
    }
    esp += 4; return;
}
