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
    eax = 0;
    if (MEM16(0xDB9C6E) != 0xFFFF) {
        esp += 4;
        return;
    }
    /* Back on in play (not under the pause menu, which would take the presses) after a fight it missed -- the
     * battle's counts (karma, combo, kills, time: 0x8DB6D8 on) changed while it was off, or in the ten seconds
     * before it went off (hidden as the fight ended, in the tally): the karma window turned off and on again, as
     * its button does, so what it had on screen from that tally goes (#214). Off and on outside a fight, the
     * window is left alone. The setting is read once: the Toggle HUD key changes it from another thread. */
    int on = ngb_pc_setting(24);
    static int shown = 1, recent;
    static unsigned frame, changed_at;
    static uint8_t battle[0x68], last[0x68];
    frame++;
    for (uint32_t i = 0; i < sizeof last; i++)
        if (last[i] != MEM8(0x8DB6D8 + i)) {
            last[i] = MEM8(0x8DB6D8 + i);
            changed_at = frame;
        }
    if (on && !shown && !MEM8(0xDB9AD5)) {
        int changed = recent;
        for (uint32_t i = 0; i < sizeof battle && !changed; i++)
            changed = battle[i] != MEM8(0x8DB6D8 + i);
        if (changed) {
            extern void ngb_karma_window_refresh(void);   /* src/ingame_menu.c */
            ngb_karma_window_refresh();
        }
    }
    if (!on && shown) {
        for (uint32_t i = 0; i < sizeof battle; i++)
            battle[i] = MEM8(0x8DB6D8 + i);
        recent = frame - changed_at < 600;
    }
    if (on && !MEM8(0xDB9AD5))
        shown = 1;
    else if (!on)
        shown = 0;
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
