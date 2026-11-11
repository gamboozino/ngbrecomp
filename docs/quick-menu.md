# Quick menu maintenance

Quick Menu 0.13 uses an NG2-style category menu, ordered from top to bottom:
Ninpo, Ranged Weapons, Melee Weapons and Consumables. The spear gun stays
under Melee Weapons. All art
and sounds come from Black; the camera fixes are a separate modification.

With the menu closed, Up always opens Melee Weapons and Down always opens
Consumables, regardless of the last category used. The same applies to the
actions' keyboard bindings (Up/1 and Down/2 by default). Left/Right (including
rebound Previous/Next actions) open Melee Weapons on the card before or the next
one at once (held, they keep stepping, as inside the menu); in play the game
does nothing with the d-pad. Once open, Up/Down changes
category and Left/Right the card, a press going round; held, Up goes to Melee
Weapons and Down to Consumables, and Left/Right step on to the first or last card
and stop there; A confirms and B cancels. A weapon or Ninpo confirmed closes the menu; a
consumable used with Quick Menu Pause on leaves it open, so several can be used in a row (#200). Each category opens on the card it
was last left on, else on the equipped weapon or the last item used. Keyboard defaults are Up/Down (also 1/2), Left/Right, Enter and
Escape/Backspace. The six actions are rebindable in Controls. Existing
profiles keep their original four bindings; new confirm/cancel actions get
their defaults. The old `quick_weapons` / `quick_items` keys now mean
Category Up / Category Down. Do not migrate them by overwriting user choices.

Opening is allowed during an action. The native inventory gate `0x00166BD0`
is checked every guest frame; until it allows selection, the world continues,
category changes and confirmation are blocked, and no pause is acquired.
Left/Right can browse the current category; B can cancel while waiting.
Once ready, Quick Menu Pause can hold gameplay. Off leaves gameplay live,
and another action can lock categories/confirmation again. A press blocked
while waiting is discarded, including a held press across the transition.
Host confirmation stays pending until the guest rechecks the gate; a stale
live-mode request leaves the menu open. Audio keeps its existing service.

The lower-left cross has a large selected card, smaller left/right entries
and folded category panels above/below. The 384 x 312 premultiplied layer
uses Black's `saveload.spr` etched panel, `mis_menu.spr` silver frame and
grey `cos_menu.spr` Costume Selection arrows. Weapon/item textures retain
their full native glow; quantities use native data. Selected weapon/item
names, level text, the waiting-for-action message and empty-category labels
are not displayed. An empty category keeps its blank card and normal
navigation/cancel behavior; confirmation requires an owned entry.
No NG2 artwork is used.

The native paper inside each card is tinted to 44.89% of its original RGB
brightness (two successive 33% reductions), preserving its alpha. Frames,
icons, arrows and quantities are drawn afterward with their own colours.
The runtime tint changes no game asset file.

RECOMP SETTINGS -> Gameplay -> **Quick Menu** offers Off, 25%, 50%, 75%, 100%, 125%, 150%:
off by default, and otherwise the menu's size. It saves as `quick_menu_scale`
in settings.ini (a percentage, 0 for Off; missing or invalid values are Off;
a size saved before, in tens, is the nearest quarter). Off disables opening,
rendering, pause and modal input capture. Pending commands/open requests are
discarded and an owned pause reference is released, including when disabled
while open. Re-enabling does not turn an already held direction into an
opener. **Quick Menu Pause** is beside it (`quick_menu_freeze`, on by default).
The presenter scales the entire layer
uniformly around its lower-left anchor, including cards, icons, arrows and
quantities. The background effect still covers the game picture. Changing
scale does not resize render targets or alter the game's render resolution.

Only when Quick Menu Pause has unlocked does a GPU pass lightly blur and
dim the background. Waiting for an action and Pause Off bypass that pass.
The menu is drawn afterward and stays sharp. The pass blits the current
display copy to at most 1920 wide, averages it into a 640 x 360 image (each
small pixel the whole area it covers, so nothing shimmers), blurs that with a
Gaussian across and down (sigma 4 small pixels) and draws it back at 0.80
brightness. It never reads the frame back to the CPU, holds a screenshot,
or waits for the whole GPU to idle.

## Integration points

- `quick_select.c`: the replacement of the game's pause-check function
  `0x00166960`. The original check remains in that file. Its addresses refer
  to the retail Xbox game's virtual memory, not to the host EXE's layout.
  The installer verifies the retail XBE sections; the mod also checks its
  task-table and inventory assumptions before opening. Unknown layouts or
  renderers leave the quick menu disabled.
- `quick_ui.c` / `quick_select.h`: host input snapshots, carousel state and
  a small transparent premultiplied layer. The event thread polls physical
  controllers independently of Black's logical controller-slot assignment.
- `d3d/vk/quick_menu.inc`: the GPU layer pass. `backend.cpp` owns its image
  and sampler, and invokes it once on the completed display copy. It also
  owns the pause-only background scratch image. These passes use
  the renderer's upload ring, frame fences, pipeline and shader helpers.
  It does not read back the game picture or wait for the whole GPU to idle.
- `bindings.c/h`: six appended actions, preserving the existing action IDs
  and setting names. Keyboard and controller gameplay filtering consumes
  only the physical controls assigned to quick actions while the menu owns
  them. Existing combat bindings are retained when quick actions are rebound.
- `launcher.c/h` stores the percentage and shares its stepping/validation
  between both settings menus. In `ingame_menu.c`'s `ngb_pc_setting`, 19 is
  Quick Menu Pause and 20 the Quick Menu size (18 is Smoother Glow).
- `main.c`: polls the host input; `ingame_menu.c` forwards SDL events. `cheats.c` invokes
  the guest state guard at frame end. `pc_menu.c`, `pause_tab.c`,
  `launcher.c/h`, and `ingame_menu.c` expose the settings (the Gameplay page).
- `manual_functions.json` and CMake register the replacement and sources.

The saved pause key remains `quick_menu_freeze` for existing profiles; its
display name is **Quick Menu Pause**. Do not renumber persisted controls or
change this key as part of an upstream update.

## Pause and inventory contract

Selection with pause enabled, after the action gate unlocks, owns one reference on the native pause-depth
counter (`0xDB5A68`). Black's main loop suspends its gameplay tasks while
input, rendering and music keep their normal cadence. Closing releases only
that reference. No nested yield loop holds gameplay or a captured picture.

Weapon changes enter the native inventory resource transition `0x002FD820`,
queue the equip through `0x002EAA20`, and leave through `0x00166B90`. That
exit drains pending resources and restores world and player state. Selection
with pause disabled stays live; committing an equipment change still uses
this short native transition to protect shared model/world resources.
Projectile weapons and Ninpo use the same native transition, with the slot
read from Black's descriptor (classes 1/2/3, slots 0/1/2). No slot is inferred
from the UI category, including the mission spear-gun variant. Bow ammunition
comes from the native required-component list; an ownership count of one
must not appear as one arrow. Healing still excludes permanent upgrades.

Healing effects and inventory debit use the game's own routines. Successful
health/ninpo refill uses recovery sound `0xA1` / `0xA6` at volume `0x40`.
These are the inventory gauge's recovery effects. A successful weapon
selection or item use also plays confirmation sound 5. Item recovery audio
follows confirmation by 100 ms, dispatched on a later guest frame without
delaying the effect or blocking gameplay/music. A rapid subsequent command
flushes the pending recovery before confirming the next selection; leaving
gameplay clears it. Weapon confirmation follows the complete resource
transition so its audio pause cannot interrupt the new tone. Confirming the
already equipped weapon also plays the tone without reloading its assets.
Each actual carousel step queues navigation sound 1 for the guest thread,
including held-button repeat. Opening, blocked movement with a single entry,
cancellation and failed use do not play confirmation/recovery audio.
Assets, including the Costume Selection arrow, are decoded from the player's
Black `spr.afs`; no game assets belong in a release or source archive.
The opacity-based picture bounds determine size and alignment only. Draw
the complete decoded texture: the faint glow around bottles extends beyond
those bounds and would otherwise end at a visible rectangular crop.

## Regression checks

When the quick menu or the code around it changes:

1. Use copied saves, a headless window, `SDL_AUDIODRIVER=dummy` and `NGB_AUDIO=0`
   (AGENTS.md). The menu reads the physical controller and the keyboard, so drive it
   with keys on the test display (Up/1, Down/2, Left/Right, Enter, Escape), not
   `RECOMP_PAD_LIVE`.
2. Verify both pause modes before/during/after selection: 60 Hz frame service,
   gameplay, held attacks/jump/projectile and movement, and no simulation catch-up.
3. Open during jumps, attacks and dashes: the action finishes before category
   changes, equip/use and optional pause unlock, and an early confirmation does not
   fire later. Equip melee/projectile weapons and Ninpo repeatedly; the model is
   visible and collision intact. Health/ninpo use plays the native sound and takes
   exactly one item. Check the title/loading/cutscene/native-menu guards and rebinding.
4. Inspect the composition at 4:3, 16:9, 16:10, 21:9 and 32:9, low and high render scales:
   blur/dimming only once the action ends with Pause On, never while waiting or with
   Pause Off, the menu sharp. Every Quick Menu size and Off, saved and reloaded. Off
   must not leave a hidden pause, draw, consume gameplay controls or run a pending
   confirmation.
