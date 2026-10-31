# Ninja Gaiden Black — technical notes

How the recompilation works and how its PC features are built, for whoever works on it. The
player-facing summary is the README; how to build and test is in AGENTS.md.

## How it runs

The game's x86 code (`default.xbe`) is disassembled and lifted to C by
[xboxrecomp](https://github.com/sp00nznet/xboxrecomp)'s pipeline (`tools/pipeline.sh`: about
11.5k functions, 2.2M lines, written to `src/recomp/gen/`, never committed -- it is made from the
player's own disc) and compiled for the host. Nothing is emulated at the CPU level. The rest of the
console is xboxrecomp's runtime:

- **Kernel**: the Xbox kernel's services (threads, files, memory, the hard disk's partitions as
  sparse image files) on the host's.
- **One virtual CPU** (`platform/vcpu.h`): the Xbox has a single CPU and a priority scheduler, and
  the game depends on it (a thread made ready above the running one runs at once). Guest threads
  share one token; waits give it up; a higher-priority thread preempts the holder -- on Linux with a
  signal, on Windows by suspending it and redirecting it into a yield (`vcpu_win_trampoline`).
- **GPU**: xemu's NV2A model runs the game's command stream (fences, vertical blanks, flips); the
  drawing is done by a renderer of the game's Direct3D calls on Vulkan (below).
- **Audio**: the game's DirectSound calls on a host mixer (below) into SDL2's output, stereo or 5.1.
- **Input**: SDL2 controllers, keyboard and mouse, as the Xbox's USB pad through an OHCI model.

The arcade Ninja Gaiden (#18) is another program on the disc that the Xbox reboots into
(`XLaunchNewImage`): `NinjaTrilogy.xbe`, or `NinjaTomoe.xbe` (the Japanese build, chosen by the game's
region flag 0xDB9890). Each is lifted the same way (`tools/pipeline_trilogy.sh [tomoe]`, into
`src/recomp/gen_trilogy` / `gen_tomoe`) and built as its own program from `src/trilogy/main.c`, drawn by
the GPU model's own NV2A renderer. `src/launch.c` is the reboot: the runtime hands it the launch data
page, which it saves and passes on (`RECOMP_LAUNCH_PAGE`), then it starts the other program in place
of this one -- exec on Linux and macOS, a new process on Windows -- with the window's current mode.
The arcade is a 68000 emulator written in x86 that keeps the 68000's flags with setcc/daa/das and 16-bit
mul/imul, which is what made the recompiler's overflow flag, decimal adjust and narrow multiplies matter.

## Audio

The game's 34 DirectSound calls (`tools/dsound_symbols.txt`, named by XbSymbolDatabase) and the arcade's 12
(`tools/dsound_symbols_arcade.txt`, the same in NinjaTrilogy and NinjaTomoe) are hooked as Direct3D's are
(`tools/take_dsound.py [trilogy|tomoe]`: `src/audio/dsound_hooks.c` and `dsound_hooks_arcade.c`, the recompiled
bodies kept in `dsound_orig*.c`, the arcade's left out of its generation by `src/manual_functions_arcade.json`) and taken by a host mixer (`src/audio/dsound_hle.cpp`, after SCDA-XBERecomp's,
`src/audio/NOTICE`): each sound buffer read from guest memory as it plays (PCM and Xbox ADPCM, its
loop region, pitch, volume and mix bins -- a bin a buffer lists twice plays twice, as the chip adds every
slot, and each bin takes its headroom, the library's one step unless the program sets it: the game sets
none, the arcade leaves it), position notifications (the arcade's double buffer: set at its next
`DirectSoundDoWork`, on its own thread), the library's 3D pass (distance, rolloff, the direction by its own octant estimate), and dsstdfx's
I3DL2 reverb on the send bin 10 with the room the game sets per area. dsstdfx's crosstalk effect on the 3D
submixes (each side takes the other's output of 11 samples before through a one pole filter: 0x7645A1, 0xF88313 in the
image's code, which the library decrypts as it loads it) ran for "Console" in 0.9.40 only: its bass came out mostly out
of phase, and xemu running the image on its DSP matches "Console" without it (#126). The sound chip is not emulated.
Positional Audio places a mono 3D sound through the library's own HRTF tables (0x395430 the filter per
direction, 0x383E70 the filters: 31 sign and magnitude taps per ear, the far ear's delay in its last
byte): "Console" filters it as the chip does, "Clear" (the default; "Panning" before 0.9.40)
takes only each ear's level from its filter, the pair scaled to constant power (0.9.40 alone used each filter's RMS gain, +2 to +5 dB by direction, heard as boomy), "Off" plays it centred. Audio Output (Sound; the arcade has it from the game, `NGB_AUDIO_OUTPUT`) is
Stereo, 5.1, or Auto: 5.1 when the default output device has six channels or more. On 5.1 the program is
told its speakers are surround (with Dolby Digital) and mixes for them, and the output is FL FR C LFE BL
BR; on stereo the centre is folded into the front pair. `NGB_AUDIO_CHANNELS=2|6` overrides it,
`NGB_AUDIO_ENGINE=apu` goes back to the recompiled library on the emulated chip (xemu's voice
processor, 0.9.29 and before: no reverb, no 5.1). `NGB_DSOUND_LOG=1` logs the calls (`=3` every Lock, `=2` every volume
change too), `NGB_DSOUND_WAV=<file>` records what plays (either engine), `NGB_DSOUND_REVERB=0` leaves the
reverb out, `NGB_DSOUND_XTALK=1` adds the crosstalk effect.

`kill -USR1 <pid>` (Linux) prints the guest thread's host backtrace, every lifted function a named
`sub_XXXXXXXX`, and its guest registers -- use it instead of gdb, which swallows the TF single-steps
the APU and AC97 register traps depend on.

## Windows

The same code, cross-compiled with llvm-mingw against MSYS2 CLANG64 libraries
(`packaging/windows/build.sh`, `cmake/toolchains/windows-x64-llvm-mingw.cmake`). It uses the same
host backends as Linux (`XBOXRECOMP_SDL_BACKENDS`: SDL2 audio and input, no XInput, XAudio2 or
Media Foundation) and the virtual CPU (`XBOXRECOMP_WIN_VCPU`: `platform/win32_host.c`, and
`platform/win32_vcpu.h` force-included to route the Win32 waits through it). The guest's 4 GB
window is one placeholder reservation (`VirtualAlloc2`) whose regions are mapped in place. Notes:
clang targeting MinGW accepts `__declspec(thread)` but makes a plain global of it (thread-local
storage must be keyed on `_MSC_VER`); `<windows.h>` is in every file there (its `near` and `far`
macros); Windows sleeps in milliseconds, so sub-millisecond waits use a high-resolution waitable
timer (`vcpu_nap_us`). Under Proton, `XDG_CONFIG_HOME` and `SDL_AUDIODRIVER` do not reach the game.

## Android (experimental)

The same code for arm64, built with the NDK (`packaging/android/build.sh`): SDL's Java activity loads the
game as `libmain.so` (`src/android_main.c` sets its folders from the environment as on Linux, under the
app's folder on shared storage, and sends the log to `log.txt` and logcat; an `env.txt` there sets
environment variables, a `settings.ini` put there replaces the settings at the next start). What differs
from Linux x86-64:

- **The CPU:** the guest's registers and the trapped device registers (the GPU's, the APU's) go through
  the arm64 paths the macOS port brought, opened to Linux arm64: the fault's write bit from the ESR record
  in the signal frame, the A64 load/store decoder (`platform/mmio_decode_a64.h`), the AAPCS64 fiber
  switch with ELF names (`src/fibers.c`). Fault handlers get a 256 KiB signal stack per guest thread
  (`compat_signal_stack`): the C library's is 16 KiB. Bionic lacks `memfd_create` before API 30 (the
  syscall), `pthread_cancel`, `explicit_bzero` and, before API 33, `backtrace`
  (`platform/android_backtrace.h`, from the unwinder).
- **The clocks:** the guest threads take turns on one virtual CPU, so no core looks busy and the governor
  kept them at 0.7-1.1 GHz: 50 ms frames on a Pixel 9a. Each frame reports its time to Android's
  performance hints (API 33+, `ngb_android_frame`), which raise them: 60 fps.
- **The GPU:** xemu's renderer is asked for none of the draw features Mali and Adreno lack (as on
  macOS: the Direct3D renderer draws); DXT textures are decoded to BGRA as they are made where the GPU
  has no BC formats (`bc_`, `src/d3d/vk/backend.cpp`); the swapchain is made upright with the system
  turning it (`s_system_turns`); glslang is built with HLSL input. GLib is replaced by the slice of it the
  NV2A code uses (`xboxrecomp/src/nv2a_vk/shim/glib-lite`).
- **Input:** touch controls (`src/touch_pad.c`): an Xbox 360 pad of embedded sprites, drawn by the
  presenter over the whole window (`layer_draw`, a premultiplied picture blended by one draw), read into
  the keyboard's pad state while no controller is connected and taken through the pad's bindings. The
  prompts are the pad's until a key is pressed.
- **The first start:** the disc image is chosen with the system's file picker, which gives the app that
  one file and needs no storage permission (`NgbActivity.java`: its path where readable, else an open
  descriptor, read through it), and installed into the app's folder or, with an SD card in and chosen,
  the app's folder on the card, remembered there (`src/android_install.c`, `src/install.c`); there is no
  unpacker, so `.7z`/`.zip` must be unpacked first.
- **The GPU driver:** below Vulkan 1.3 the renderers take dynamic rendering and synchronization2 from
  their extensions (`instance.c`; `NV2A_VK_NO13=1` forces that path on the desktop, for testing). On an
  Adreno whose own driver lacks them, Turnip (Mesa's Vulkan driver for Adreno, built from Mesa's
  release by `packaging/android/build.sh`) is loaded for the app by libadrenotools (`src/android_gpu.c`,
  through `nv2a_vk_custom_loader`; no setting, `NGB_GPU_DRIVER=system|turnip` forces one for testing);
  the window's surface is then made through that loader (`present_sdl.c`).
- **Defaults:** fullscreen, the Multithreaded Renderer on (60 fps against 42-58 without on a Pixel 9a).
  The arcade programs and Return to Main Menu's restart are not there yet (no programs to start beside
  the app).

## Presentation

The game's frame clock is the emulated display's 60 Hz vblank. Each picture the renderer finishes
(at the game's Swap, a couple of milliseconds after a vblank) is presented at once from the
renderer's thread (`present_sdl.c`, `nv2a_present_set_external`); the vblank tick presents only what
was not (a loading pause). Presented at the tick, every frame waited a whole refresh; through the
GPU-command thread, presents came out unevenly. V-Sync's Mailbox is mailbox where the system has it
(no tearing, no queue: a FIFO queue fills against the game's own clock and adds 30-50 ms), FIFO on as
few images as allowed where it has not; its FIFO is FIFO (every frame in turn: steadier where
mailbox, like Nvidia's Fast sync, now and then shows a frame twice, #17); Off is immediate. `NV2A_FPS_LOG=1` logs the frame rate and
`[LAT]` lines: each picture's age when presented and the spacing of presents.

## Settings, menus and features

The settings are in the game's own menus, in its style: the main menu's RECOMP SETTINGS
(where Ranking was) opens the Options screen on them -- Graphics, Display, Sound, Gameplay
and Cheats, each a page of rows like Video Settings', and Controls (the bindings' panel,
below), under a SETTINGS header (`src/pc_menu.c`;
the header and category words are drawn in Liberation Serif in the game's pictures' style,
`src/pause_tab.c`) -- and the pause menu has a Settings tab before Exit (A to enter its page,
left/right on the category strip at its top for Graphics, Display, Sound, Gameplay, Cheats or Controls,
up/down for the rows, left/right or A to change one -- or a click: on a value's left half back,
anywhere else on the row on -- B to leave; L and R go on to the next tab, as on the
game's own pages; `NGB_NO_SETTINGS_TAB=1` leaves the game's six tabs). They are: aspect ratio
(4:3, the game's own 4:3 mode; 16:9, its widescreen; 16:10, widescreen with the 3D drawn for
16:10; it applies at once: the 3D view's projection, the presenter and the window, whose sizes
follow the shape -- only the paused picture behind the pause menu keeps the old shape until
play resumes. The game has one layout for its HUD and menus, drawn for 4:3: in 16:9 and 16:10
they are stretched sideways, as the console did on a widescreen TV), window size (the sizes offered match the
aspect ratio), windowed/fullscreen/borderless (a frameless window over the whole display),
render resolution (1x-10x), anisotropic filtering (Off, the console's own filtering; 2x, 4x,
8x or 16x: trilinear blending between mipmaps and anisotropic filtering at that level, and
mipmaps made for the many textures the game gives none -- DXT ones decoded for it -- which stops
rock, wood and gold-leaf textures sparkling and crawling in the distance as the camera moves), anti-aliasing (MSAA Off, 2x, 4x
or 8x: the display-sized targets drawn multisampled and resolved as each pass ends; with the
new renderer only), shadow resolution (Ryu's shadow map at 2x, 4x, 8x or 10x the original
128x128, for smooth instead of blocky edges), V-Sync (Off, Mailbox, FIFO, or Adaptive:
synced, but a late frame is shown at once instead of waiting a refresh -- Vulkan's FIFO relaxed), an FPS
counter, and skipping the intros (the boot movie, which the game itself will not let you skip;
the New Game prologue and the story movies play -- the game's own Start does not end the prologue -- and
Skip Any Cutscene lets Start end them; from the next start). They are saved to `~/.config/NinjaGaidenBlack/settings.ini` as they change.
`--settings` shows the old settings screen before the game.

All Difficulties (Cheats > Unlocks, saved): New Game offers every difficulty, Ninja Dog to Master Ninja,
without clearing the one before (or dying for Ninja Dog). The game's menus ask one function (sub_001DB520, now in
`src/cheats.c`) for their unlock flags -- 7-9 are those three, read from the system data --
and with the setting on it answers yes for them; nothing is written, so the player's own record
is untouched and turning it off puts back what they have earned. Off, Ninja Dog is as the game
has it: offered after dying in chapter 1. Missions, Arcade Ninja Gaiden and Movie Theater (Cheats >
Unlocks, saved) answer yes for flags 10-12 the same way: the main menu's MISSIONS, ARCADE NINJA GAIDEN
and MOVIE THEATER, which the game shows once earned (the list's entries 2, 4 and 5); a change made in
RECOMP SETTINGS shows or hides them at once (`src/main_menu.c`). With Missions on every mission is
offered too: the Missions menu asks a byte per mission (0xDB95AC + its number, bit 0 offered, bit 1
cleared; the game offers ten as Missions is earned and the rest as missions are cleared), and its
reads (four lifted functions in `src/main_menu.c`) see bit 0 set; nothing written there either.

Intercept (Gameplay; `src/intercept.c`, #109): Hurricane Pack 1's guard counter. Black kept its
Technique scroll (item 0x26), Ryu's move and its karma, and removed one instruction: in sub_0001ACD0
(Ryu's next action from his guard, `bl`), a guard raised under 6 ticks before an attack lands, with guard
pressed this tick and not in the 14 before, set the action to 0x0D; the code still branches around where it
was. The option puts it back, and lets Ryu do it without the scroll. Black uses Black's own move for it
(state 0x8EC); Hurricane switches four of Ryu's move records and five bytes of his loaded character data
(chr_dat2, at *0x4B7528) to the Hurricane Pack's move (0x99), once a frame and only while they hold one
kind's values. While it is on, the Intercept karma (1000 each, at most 50 a battle, in battles
with a countdown: sub_002EBB00) is paid on every difficulty, not only Very Hard and Master Ninja, and the
battle's results show the "Intercept" line again (sub_00208120, from the counts Black still keeps at
0x8DB734; Black only stopped drawing it).

Essence-Only Charging (Gameplay; `src/ut_charge.c`, #120): while Ryu is in a charging action (kind 0xB9,
the player block's +4 at 0x4968B0), sub_00023520 counts its frames in 0x5895E4 and raises the charge
level 0x5895E2 when the count reaches either of two marks (0x5895E6, 0x5895E8: about 100 and 220 frames,
from sub_00029140); the release picks the plain attack, the Ultimate Technique or the stronger one by that
level (script opcode 0x83 in sub_00016300; for the Dragon Sword actions 0x20A, 0x20D and 0x20F). Essence
absorbed while charging (sub_001E2EB0) raises the level itself and sets the count just past that level's
mark (0x65, 0xDD). With the option on, the count's steps of a few frames are taken back each frame, so only
essence raises the level.

Skip Any Cutscene (Gameplay; `src/cutscene_skip.c`, #139): sub_001DBEE0 runs the events, the in-game
cutscenes among them, once a frame, and Start skips the one playing when Start is down (bit 0x10 of the player's
buttons at 0xD219A8, 0x78 a player), the event has been watched (one byte an event from 0xDB9477, by its number at
0xDB9C6E: the chapter 1 waterfall is 0x6E, its byte 0xDB94E5) and its script has made it skippable (0x8D3A66, set
and cleared by the script's own commands in sub_001DD300). The function is taken from the generated code with
the watched test passed while the option is on; a cutscene its script never makes skippable still is not, and
the watched bytes are not written, so saves do not change. The same option lets Start end a movie
(`src/movie_skip.c`). Off unless chosen (0.9.43 alone had it on).

Responsive Menus (Gameplay; `src/responsive.c`, #116), presses the game ignores for a while taken at once. The
title reads no button until its frame count (0x45A7F4, while 0x45A7F8 is 1) passes 0x43B688 + 0x43B694 (40
frames of fade-in, then 60 with PRESS START up); once the prompt is drawn the count is set to the mark. A
cutscene's script often waits before making it skippable (the chapter 1 waterfall 180 frames: a wait of frames
counted down in 0x8D3A52 while 0x8D3A50 is 2), and the skip jumps the script to its end mark without running
what lies between, so the skip is not let through early: a new Start press, when the script reaches its
skippable command (3 0) a few commands on with no jump between, ends the waits before it and is kept for the skip. The event's state
is reset as sub_001DBEE0 starts each event. The pause menu's message box (sub_00212500, taken: "equipped" and
the like, 0x8DB848 the message, 0x8DB84C its 60 frames) returns busy while up and its callers set bit 0x1000 of
0xDA41E8, which keeps the menu from closing; with the option it returns 0. Equipping also leaves the list in a
state (bit 1 of 0xDA41E8) that takes the next Start to leave the list; while the message is up the bit is cleared.

Cheats, for getting past a hard spot (`src/cheats.c`, applied each frame; for the session
only, never saved, so a new start has them off): Infinite Health (Ryu's health refilled each
frame -- the game keeps it per segment at 0x58A534, maxima at 0x58A524, and 0x58A566 is their
sum; a single blow taking all of it at once still kills), Infinite Ninpo (0x58A56C/0x58A56E),
Infinite Essence (0x589570, the game's cap of 99,999) and Golden Scarabs (enough in the bag --
inventory slots at 0x5891D0, item 0x59 -- to make 50 with those already given to Muramasa,
0x589574: every one of his rewards at the next visit). All Weapons puts every melee and projectile
weapon Black gives in the bag, or raises it, at its highest level (a weapon's count is its level; Lunar
and Kitetsu are one item a level, 0x12B/0x133/0x134 and 0x182/0x0CC/0x183, and a lower one held
becomes the top one), with a full stack of what it fires once a game is loaded: the Strongbow, the
bows for explosive arrows (0x0E2) and APFSDS cores (0x029), the Spear Gun, both shuriken, incendiary
shuriken and smoke bombs. The original's Bow (0x08D) and Plasma Saber (0x0CE) are left out, as
Black replaces them with the Strongbow and the Mk.II. Turned off, it takes them back: what the player had
of each before is kept, with what the game gives meanwhile (sub_00154300: a Lunar picked up goes into
the cheat's while it is on), and put back -- a weapon held is put back once Ryu holds another, nothing
under the pause menu. The slots it writes carry a mark in their fourth byte (the game leaves it alone and
saves it): a record is only applied to slots with its mark, so a save loaded meanwhile is never touched. A save
made while it is on keeps the weapons: the record is the session's, so after a restart they are the save's own.
The game guards its state with 24
checksums (sub_00164E50) and returns to the title when one changes without the flag its own
code sets (0x7F1A94 + n); a cheat that writes sets them all. Unlocks also has All
Costumes (saved): the Costume Selection at New Game offers every costume, as after clearing
the game, through the same flag function as All Difficulties (flags 1-4). The game has eight costumes (chr_ryu.afs, chr_ryua-g.afs) and its Costume Selection
lists five (0 and 4-7, the table at 0x3E9D10), the locked ones as white shapes; here it lists only
those unlocked (`src/costume_menu.c`), and with All Costumes on costumes 1-3 too, after the game's five:
the game has no pictures of them, so each is its default costume's picture as a silhouette with
the name across it (EXTRA 1-3, made at run time from the player's copy, `src/pause_tab.c`).
Debugging aids there: `NGB_CHEATS=111` turns them on at start, `NGB_POKE=addr:bytes:value,...`
writes guest memory every frame (only while the file `NGB_POKE_GATE` names exists, if set) and
`NGB_MEMDUMP=<dir>` dumps the 64 MB of guest memory each time `<dir>/dump.now` appears.

Back from OPTIONS or RECOMP SETTINGS the main menu's cursor returns to the entry chosen (the
game puts it where OPTIONS was before it moved, the last entry: now EXIT TO DESKTOP). With All
Difficulties' Ninja Dog the difficulty selection has five rows, a case the game never shows
there: they are spaced closer (38 instead of 44) so the first clears the header.

On a wide screen (16:9, 16:10, 21:9, 32:9) the game is in its own widescreen mode the whole time, with
its 3D drawn for the screen's shape (both of its aspect constants, 0x414AF4 and 0x414AF8, are that
shape): every menu, the HUD, chapter cards and clear screens are the game's own 16:9 ones, drawn as it
draws them for a widescreen TV, kept 16:9 (`src/d3d/vk/backend.cpp`): at 21:9 and 32:9 narrowed to the middle
16:9, at 16:10 shortened to it, a 2D screen or menu whole there with bars beyond (at the sides at
21:9, above and below at 16:10). The bars are filled with the screen's own colours: each side's edge,
16 bands of it, sampled over the screen's first 2 s and then kept still, so a menu's animation does
not move them (`EdgeSample`; `NGB_BAR_FILL=0` makes them black). A menu drawn over the paused game
(the save statue's) has the paused game around it instead. Which frames are 2D screens: a menu
running in the frame, or (the frame before was one) no 3D drawn in this frame so far. In play the 3D fills the screen and the HUD keeps its 16:9 shape but goes
to the screen's corners and sides: each frame its pieces are grouped into elements (pieces within a few
pixels of each other -- a gauge with its caps and the words under it, the health bar with the karma, a
prompt with its icon, a line of subtitles), and each element keeps to the nearer side (or the middle,
when it is about the middle), to the top when it starts near the top (the health bar, the karma
window opened below it with it), the bottom when it ends near the bottom, else the middle; only what
is drawn after the frame's 3D counts (the 3D's own screen-space mist and spray are left as drawn), and
screen effects in play -- anything covering most of the screen or reading a render target: the
underwater tint, flashes, the bloom -- fill the whole screen as drawn (`HudGroup`, `HudPlace`;
`NGB_HUD_GROUP_LOG=1` logs them, `NGB_HUD_CENTER=1` keeps the whole HUD in the middle 16:9). A screen
of panels drawn over the game (a chapter clear: an element about the middle of the screen, over a third
of its width and nearly half its height, across its middle -- an area's name is not one -- for 4 frames
running; over after 8 without) is kept whole in
the middle, as a menu: what is inside the panel's box goes with it, the HUD around it stays where it is. At 4:3 the game is in its 4:3 mode and everything is its 4:3 picture.
(Earlier versions kept the game in its 4:3 mode in play and narrowed its HUD to its 4:3 shape,
switching modes for the menus; the screens it could not tell apart in time -- item pickups, chapter
clears, leaving the save menu -- came out half one way and half the other. `NGB_HUD_FIXED=1` brings
that back, for comparison.) Movies play in the widescreen mode too, where the game follows its own Movie Display
Ratio (Options, Video Settings): 16:9 across the screen, or 4:3 with bars at the sides. (Switching to
its 4:3 mode for a 4:3 movie narrowed it twice, #28.) The mouse is mapped to the menus the same way.
`NGB_HUD_RATIO_LOG=1` logs the 2D draws not drawn texel for pixel; `NGB_MODE_LOG=1` the mode each
frame.
Aspect ratio also offers 21:9 (64:27;
window sizes 1720x720 to 5120x2160) and 32:9 (two 16:9 screens side by side, 2560x720 to 7680x2160, #74):
the 3D widened as for 16:10. The game's own choice of what to draw is left as it is; no object
has been seen appearing late at the far edges.

The boot "Warnings" screen is always skipped (`NGB_KEEP_WARNINGS=1` shows it).

For glitches that only show in play: F7 records the next 300 frames to `~/ngb-trace-<n>/` -- a
small picture of each frame and a line per draw (its state: targets, textures, combiner stages,
blending, fog, vertex colour, clears, and `stk`: the game's return addresses on its stack, innermost
first, which say what drew it) in `draws.txt` (`NGB_TRACE_FRAMES=<n>` frames;
`NGB_TRACE_PRE=1` only the screen-space draws and the first draw after each target change;
`NGB_TRACE_NOPIC=1` no pictures, `=copy` pictures only around a copy into a 512x512 target, the
cut cross-fade's). F8 appends Ryu's position to `ngb_positions.txt`
in the save directory.

The pause menu's Exit tab has a page of its own: Return to Game (the game's Exit: the menu
closes), Return to Main Menu and Exit to Desktop, the last two after an "Are you sure?"
Yes/No box in the game's menus' style (a near-black panel between gold rules, the game's own
letters, the chosen answer white in the violet haze of its menu items). The main menu's last entry is EXIT TO DESKTOP, behind the same box (OPTIONS moved
up into the never-shown COMING ATTRACTIONS' place). The game has no way back to its title
from play but the console's reset, so Return to Main Menu starts the program again in the
same process (Steam and the like see one game running throughout), skips the intros and
presses Start at the title; the window closes and opens again on the way.

F11 or Alt+Enter toggles fullscreen and
F10 the FPS counter (frames per second, average and worst frame time over the last half
second). Compiled shaders are kept in `~/.cache/NinjaGaidenBlack/shaders` (the new renderer's in its
`d3d` folder: how each shader is made, its SPIR-V and the pipelines), and all of them are made
again in the background at the next start, before the game needs them; a seed shipped with the
AppImage (`packaging/d3d-seed.bin`, recorded over the first chapters; the game's vertex programs in
it are only places in the player's own `default.xbe`) does the same on the very first start. So
an effect does not stutter the frame it first appears. The AppImage also carries the pipelines
recorded while testing (`packaging/shader-seed`), built in the background at startup;
the game's vertex programs are not in it, only where each is in `default.xbe`, and they are
read from the player's own copy (`tools/strip_shader_seed.py` makes the seed from a
recorded shader cache).

(The main menu's Video Settings page has no PC row: the settings are under RECOMP SETTINGS.)

Keyboard and mouse: whichever of them or a controller was used last is the one the game listens
to (touch the other and it switches; xboxrecomp's input). The keys by default (`src/keyboard.c`;
the game's default actions in brackets):

| Keys | Pad | | Keys | Pad |
|---|---|---|---|---|
| W A S D | left stick (move) | | mouse, numpad 8 4 5 6 | right stick |
| left mouse, J | X (attack) | | right mouse, K | Y (special attack) |
| Space | A (jump) | | E | B (projectile) |
| F | Y+B (ninpo) | | C | X+A (wind run) |
| Shift | L (guard) | | Q, middle mouse | R (center camera) |
| Tab | Black (map) | | R | White (karma window) |
| Esc, Enter | START | | Backspace | BACK |
| arrows | d-pad | | Z / V | stick clicks |

Controls (the Settings tab's Controls category and RECOMP SETTINGS' CONTROLS; the game's own
Options > Controller Settings > Button Settings is left as the game has it, for the controller)
changes them: each action in play has two keys or mouse
buttons and a controller button; A (Enter, a click) waits for the new one -- Esc cancels; for the
controller, its next button, or six seconds -- and X (Delete) clears it. A key taken from another
action leaves it; a controller button taken from another action swaps with it. They are kept in
`controls.ini` beside `settings.ini` (`src/bindings.c`); in play the keyboard and mouse press the
pad as bound and the controller's buttons are moved to their actions (`xbox_pad_remap`, weak in
xboxrecomp's input). The sticks are not moved; the menus keep their own controls, the controller
as it is, so no binding can lock them up. Play's key prompts follow the bindings.
`NGB_FIXED_KEYS=1` keeps the keys above, `NGB_NO_PAD_REMAP=1` the controller as it is,
`NGB_CONTROLS_IN_OPTIONS=1` puts the panel over the game's Button Settings too.

In the menus Esc is B (back), Q and E are L and R (the pause menu's tabs) and the mouse points;
in play the mouse is captured and its motion is the right stick. The pointer is shown only in the
menus while the keyboard and mouse are in use. While the keyboard is in use the
game's button prompts are keys and mouse buttons instead -- Xelu's dark keyboard and mouse prompts
(CC0, `assets/prompts/keys`, the same artist's as the controller buttons; a key the set has no picture
of, its blank key with the key's name; `src/pause_tab.c`), in play the ones bound to each action, in
the menus the mouse's buttons (left to choose, right to go back), K, Space, Shift and Q/E;
`NGB_PAD_PROMPTS=1` keeps the pad's.
With a PlayStation pad in use (SDL's controller type: a DualShock 3 or 4, a DualSense, or an adapter
that says so) the prompts are its buttons (#113): Xelu's PS3 set (`assets/prompts/ps`), the symbols in
their colours on black as on the DualShock 2 of the game's time -- Cross, Circle, Square and Triangle for
A, B, X and Y, L2 and R2 for the triggers, L1 and R1 for Black and White (the buttons they are on), over the
pause menu's L and R plates too (on a dark plate the size of the game's), and R1 drawn the size of L1 (the
game draws White's icon smaller); the Controls page names them the same way. The Controls page shows the
two-button actions (Ninpo, Wind Run) as the pad's button pictures, Xelu's Xbox 360 ones on an Xbox pad
(`assets/prompts/xbox`). The game's Button Settings
page keeps its picture of the Xbox controller. The Controls page's Button Prompts picks the set: Auto (the
device in use, as above), Xbox, PlayStation or PC (the keys), saved in `controls.ini` (`button_prompts`).
`NGB_PAD_PROMPTS=ps` shows the PlayStation's whatever the setting, any other value the Xbox's.
The techniques' "hold the button" icons (sprites 0x1CE-0x1D3) are a texture of their own, six
cells with an arrow over the button: with PlayStation or PC prompts each button is redrawn as that set's,
the game's arrow kept over it (`hold_check`, #125); the keys are play's, not the menu's.

In the game's menus (main menu, Options, the pause menu and its tabs, message boxes) the mouse
points: the game's own cursor follows it onto the item under it (a press of the pad at a time,
so the game moves it as it would), a click there chooses it, a right click is B, the wheel
up/down, the side buttons the triggers. While a controller is the device in use it does
nothing in the menus; moving it makes the keyboard and mouse the device in use again. (`NGB_MOUSE_LOG=1` shows what it finds.)

The button prompts ("Select", "Back") can be clicked: the button they show. Esc is B (back) in
the game's menus.

The main menu has RECOMP SETTINGS in Ranking's place (Xbox Live's scoreboards, gone; choosing it
sent the game to the dashboard's account setup, which closed the program) and Options > General Settings no Flag
Select (the country the scoreboards wanted) -- `src/main_menu.c`; `NGB_KEEP_RANKING=1` keeps
both.

Total Play Time: saves made by builds before the clock fix hold the 999:59 cap (the PC's uptime
was counted); such a total is started again from 0 when loaded (`src/playtime.c`).

The environment still overrides the file, for testing: `NGB_WIDESCREEN=0|1`,
`NGB_FULLSCREEN=0|1`, `NGB_VSYNC=0|1`, `NV2A_SCALE=n`, `NV2A_SHOW_FPS=1`,
`NGB_NO_SETTINGS=1` (skip the screen), `NGB_AUDIO=0` (mute). `NV2A_FPS_LOG=1` prints the
FPS counter to stderr, and `NV2A_FPS_LOG=/path/to/file` appends it to that file (on a Steam Deck:
`NV2A_FPS_LOG=/home/deck/ngb-fps.txt %command%` as the launch options). `NV2A_EARLY_PRESENT=1` shows a finished frame before the title flips to it
(a refresh less latency); off by default, because on a busy frame it made the rate fall to 40-50.
`NV2A_REFRESH_HZ=n` runs the emulated display at n Hz (testing only: the game steps its logic
once a frame, so at 120 it plays at double speed).
The game measures its own GPU load: a callback it puts at the end of each frame's push buffer (0x30E060)
records how far into the frame the GPU got there, and over 94% the game lowers its quality level, which
swaps Ryu's shadow map for the round shadow under him (sub_001D5E30). The emulated GPU only reads the
push buffer while the renderer draws, so that callback reports no GPU time (`ngb_gpu_frame_reached`,
src/d3d/d3d.c); on some PCs it got there late now and then and the shadow came and went (#50).
Graphics > Lower Detail Under Load keeps the game's measurement, which on a slower PC steadied the frame
time (#114); `NGB_GPU_LOAD=1` or `0` overrides the setting.

The game draws each frame from render lists it builds as trees of nodes, all cut from one pool of
768 (src/render_list.c). When the pool is used up a node is silently left out, and the HUD, the 2D
layer and the screen effects go in last: in a fight with enough enemies, blood and sparks for 768
nodes before them, the HUD disappeared for the busiest ten or twenty frames of a multi-kill, its
sprites piling up until a quieter frame drew them all at once (#96). sub_00308BF0, the allocating
insert, gives out 4096 more nodes of ours after the game's; `NGB_NODE_POOL=0` keeps the game's limit.
The weapon-hit sparks are lines the game draws 2.7 to 3 pixels wide; `NGB_SPARK_WIDTH=0.5` (0.1 to 1)
makes them thinner, never under a pixel (#117).

## The renderer

The game is drawn by a renderer that implements its Direct3D calls on Vulkan (`src/d3d`,
`docs/renderer.md`; its shader translation, vertex and texture code come from TGP482's
[SCDA-XBERecomp](https://github.com/TGP482/SCDA-XBERecomp) /
[Xbox-XBERecomp](https://github.com/TGP482/Xbox-XBERecomp), MIT). xemu's NV2A
emulation still runs the GPU's command stream (the game's fences and vertical blanks come from
it) but draws nothing; the renderer draws on the same Vulkan device and the same window, so the
settings and the FPS counter work as before. Render resolution, texture filtering
and shadow resolution apply to it. It needs Vulkan 1.3's dynamic rendering and synchronization2
(or those as extensions of an older driver) and push descriptors; without them the game says what is missing
and ends (#47).

`NGB_RENDERER=xemu` draws with xemu's NV2A renderer instead, as before (on Steam:
`NGB_RENDERER=xemu %command%`). `NGB_RENDERER=mirror` keeps xemu drawing and showing the game
and renders with the new one unseen, for comparing (`NGB_D3D_DUMP=dir` writes its frames).

## Bring-up tools

- `kill -USR1 <pid>` prints the guest thread's host backtrace (every lifted function
  is a named `sub_XXXXXXXX`) and its guest registers. It needs no ptrace, so use it
  instead of gdb: gdb swallows the TF single-steps that the register traps depend on.
- `RECOMP_WATCHDOG_SECS=N` dumps guest state and exits after N seconds.
- `tools/feedback.sh [secs]` runs an instrumented build, records every
  indirect-branch target it reaches, reseeds function detection and regenerates.
  Run it after the game reaches new code.
- `XBOX_LOG_LEVEL=4` and `RECOMP_KERNEL_LOG_BUDGET=100000` give a full kernel and
  file trace.
- `-DNGB_OPT_LEVEL=-O3` builds every target at another level (`-O2` by default; on the attract demo -O3
  measured no different). `-DNGB_PGO=generate` builds a profile-recording binary: a training run writes
  `$LLVM_PROFILE_FILE` when stopped with SIGINT or SIGTERM; `llvm-profdata merge` the runs and build with
  `-DNGB_PGO=<file.profdata>` (the same clang version). Measured on the attract demo: about 4% less CPU,
  not used for releases (each toolchain needs its own profile, and its compiler's choices in code the
  training never ran are untested: one read a GPU register with a `bt` the MMIO decoder lacked).

The defaults in `bringup_defaults()` (`src/main.c`) stand in for hardware that isn't
modelled yet. Each one should disappear when the real thing exists.
