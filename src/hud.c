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
    /* The karma window turned off as the HUD goes off and on again as it comes back, by its button, as the player
     * would (#214): skipped while the HUD is off, the window's tally after a fight froze, and its Combo, Kill and
     * Time stayed on screen once the HUD was back; turned on again, it starts over. Pressed only as needed, by the
     * window's own state (0x8DB6DC, its state machine sub_00207760: 0 hidden, 1 sliding in, 7 sliding out -- its
     * place, 0x8DB6D8, reads 0xFF06 both hidden and as it starts in): off only if it shows, on again
     * only if it showed and is hidden -- a press the game missed (two at once, the game took the first only; one
     * as the pause menu closes, a button still held) leaves it as it is rather than the other way round. In play
     * only, not under the pause menu (which would take the press): a change made there is pressed when play goes
     * on. The setting is read once: the Toggle HUD key changes it from another thread. */
    int on = ngb_pc_setting(24);
    static int hidden_for = -1;   /* the HUD off: whether the window showed then (-1: the HUD is on) */
    if (!MEM8(0xDB9AD5)) {
        extern void ngb_karma_window_press(void);   /* src/ingame_menu.c */
        int shows = MEM16(0x8DB6DC) != 0 && MEM16(0x8DB6DC) != 7;
        if (!on && hidden_for < 0) {
            hidden_for = shows;
            if (shows)
                ngb_karma_window_press();
        } else if (on && hidden_for >= 0) {
            if (hidden_for && !shows)
                ngb_karma_window_press();
            hidden_for = -1;
        }
    }
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
