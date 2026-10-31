![Ninja Gaiden Black Recompiled](docs/images/banner.jpg)

# Ninja Gaiden Black Recompiled

**A PC port of Ninja Gaiden Black by static recompilation.**

A native Linux and Windows version of *Ninja Gaiden Black* (Xbox, 2005), with an experimental Android
one, made by statically
recompiling the game's own code to run directly on a PC — not an emulator. It draws with its own
Vulkan renderer, which takes over the game's Direct3D calls instead of emulating the Xbox GPU. It
plays at a steady 60 fps on modest hardware (including the Steam Deck) and adds PC options: higher
resolutions, widescreen and ultrawide, keyboard and mouse, rebindable controls and more.

**You need your own copy of the game.** This project contains none of the game's data (graphics,
sound, movies, text): on first start it asks for your disc image and installs the game from it. It is made for the **retail USA
release** (NTSC-U, title ID `5443000D`) and checks for it.

## Features

- Native Linux (AppImage) and Windows (x64) builds; Steam Deck friendly
- Android (arm64), **experimental**, with Xbox 360 touch controls
- Render resolution up to 10x, anisotropic filtering, MSAA, sharper shadows
- 16:9, 16:10, 21:9 and 32:9 as well as 4:3: the 3D fills the screen, the HUD goes to the screen's
  corners, optional HUD-free view, menus keep the game's own 16:19
- Keyboard and mouse with key prompts in the game, rebindable controls for keyboard, mouse and
  controller, mouse support in the menus
- Skippable intros, movies and cutscenes (an option), unlock-all options for difficulties and costumes, optional cheats
- The arcade Ninja Gaiden on the Han's Bar cabinet and the main menu, recompiled as well
- Sound mixed on the PC: positional 3D sound, the console's room reverb, and 5.1 surround on a 5.1 or
  7.1 output
- Smooth frame pacing and low input latency; shaders prepared in the background

## Getting started

On every system, the first start after installing takes longer to load (the shaders are prepared then and
kept); later starts are quicker.

### Linux and Steam Deck

1. Download `NinjaGaidenBlack-x86_64.AppImage` from the releases page and make it executable
   (`chmod +x`, or Properties → Allow executing).
2. Run it. On the first start a window asks for your copy of the game: a disc image (`.iso`, a full
   Redump dump, or the `.7z`/`.zip` it came in) or a folder of extracted files with `default.xbe`.
   The game is installed once (about 6 GB).
3. On a Steam Deck, add the AppImage to Steam (Add a Non-Steam Game) to play it in Game Mode, and
   choose 16:10 in the settings.

   If it closes as soon as you press Play in Game Mode, that's a bug in the current stable Steam
   client with non-Steam games (fixed in the Steam Beta). Until the next stable release, either
   switch Steam to the Beta, or set the game's Launch Options in Steam to
   `APPIMAGE_EXTRACT_AND_RUN=1 %command%` (#11).

Needs a 64-bit Linux with glibc 2.35 or newer and a GPU with Vulkan 1.3 drivers (or older drivers that
have its dynamic rendering and synchronization2 as extensions).

### Windows

1. Download `NinjaGaidenBlack-windows-x64.zip` and extract it anywhere.
2. Run `NinjaGaidenBlack.exe` and choose your disc image as above.

Needs Windows 10 or 11 (64-bit) and a GPU with Vulkan 1.3 drivers (or older drivers that have its dynamic
rendering and synchronization2 as extensions).

**Antivirus warnings:** the program isn't code-signed, and a new unsigned program is sometimes
flagged by one antivirus's heuristics (McAfee quarantined it in #88; VirusTotal shows no other
detections). That's a false positive. Each release lists the SHA-256 hash of both downloads in its
notes and in `SHA256SUMS.txt`, so you can check that your download is the one published here: in
PowerShell, `Get-FileHash NinjaGaidenBlack-windows-x64.zip`; on Linux, `sha256sum -c SHA256SUMS.txt`.
If you'd rather not run a prebuilt program, you can build it yourself from your own disc
([Building from source](#building-from-source)).

### Android (experimental)

The Android version is new and has only been tested on one phone (a Pixel 9a, where it plays at 60 fps).
Expect problems on other phones, and please report them (below).

**Install:**

1. Download `NinjaGaidenBlack-android-arm64.apk` from the releases page and open it on the phone.
   Allow installing apps from your browser or file manager when Android asks.
2. Copy your disc image (`.iso`, from the retail USA disc) to the phone or its SD card, anywhere. A `.7z`
   or `.zip` has to be unpacked on a computer first: the phone has no unpacker.
3. Start the game. The first time, it asks you to choose the image with Android's file picker, then
   installs the game into its own folder (about 6 GB, a minute or two; with an SD card in, it asks
   whether to put it on the phone or on the card). It needs no storage permission. You can delete the
   image afterwards.

Plays with the on-screen touch controls (an Xbox 360 pad), or with any controller the phone supports:
the touch controls hide while one is connected. Needs Android 10 or newer (13 or newer for full speed:
the game tells the system how long its frames take, which keeps the CPU clocks up), a GPU with Vulkan
1.3, or an older Vulkan with its dynamic rendering, synchronization2 and push descriptors as extensions
(on Adreno, Turnip has them).

On Adreno GPUs (Snapdragon phones) whose own driver lacks what the game needs, it uses Turnip, Mesa's
open driver for Adreno, built into the app; otherwise the phone's own.

Not there yet on Android: the arcade Ninja Gaiden, and "Return to Main Menu" from the pause menu (it
closes the game instead).

**Reporting a problem:** open an issue and give the phone's model, its chip (Snapdragon, Tensor,
Dimensity, Exynos...), the Android version, what happened, and the log: `log.txt` in
`Android/data/io.github.gamboozino.ngbrecomp/files` on the phone's storage. Some file managers can't
open that folder on recent Android; a computer can, over USB (or `adb pull
/sdcard/Android/data/io.github.gamboozino.ngbrecomp/files/log.txt`).

### macOS (unsupported)

The macOS code is a community contribution (#49) that I can't build or test, so there are no
official macOS packages and macOS bugs may go unfixed. Build the Apple Silicon app using the commands below, extract the resulting zip, and open
`Ninja Gaiden Black.app`. Choose your own disc image or extracted game folder on first start.
Current dependencies require macOS 26. Intel Macs and the xemu renderer are unsupported;
packages are signed ad hoc, without Developer ID signing or notarization.

### Where things are kept

| | Linux | Windows | macOS | Android |
|---|---|---|---|---|
| Installed game, saves | `~/.local/share/NinjaGaidenBlack` | `%LOCALAPPDATA%\NinjaGaidenBlack` | `~/Library/Application Support/NinjaGaidenBlack/saves` | `Android/data/io.github.gamboozino.ngbrecomp/files` |
| Settings (`settings.ini`, `controls.ini`) | `~/.config/NinjaGaidenBlack` | `%APPDATA%\NinjaGaidenBlack` | `~/Library/Application Support/NinjaGaidenBlack/config` | `config` in the folder above |
| Shader cache | `~/.cache/NinjaGaidenBlack` | `%LOCALAPPDATA%\NinjaGaidenBlack\shaders` | `~/Library/Application Support/NinjaGaidenBlack/cache/NinjaGaidenBlack/shaders` | `cache` in the folder above |
| Log | the terminal, or `log.txt` beside the saves when started without one | `log.txt` beside the saves (or the console it was started from) | `log.txt` beside the saves | `log.txt` beside the saves |

**Portable mode:** with a file named `portable.txt` next to `NinjaGaidenBlack.exe` (or the AppImage or Mac app),
all of these go into a `userdata` folder next to it instead. The first-run setup offers this as a
checkbox. The folder must be writable, so not inside Program Files. Delete `portable.txt` to go back.

## Settings

Everything is in the game's own menus: **RECOMP SETTINGS** on the main menu, and the **Settings** tab
of the pause menu. Changes apply at once and are saved.

- **Graphics**: render resolution (1x–10x), texture filtering (the console's, or 2x–16x
  anisotropic), anti-aliasing (MSAA), shadow resolution, Smoother Glow (a softer, fainter glow on
  bright lights; off is the console's look), Lower Detail Under Load, Multithreaded Renderer (drawing on a
  thread of its own, for slower CPUs; off unless chosen, may have bugs)
- **Display**: aspect ratio (4:3, 16:9, 16:10, 21:9, 32:9), window size, windowed / fullscreen /
  borderless, V-Sync (Off, Mailbox, FIFO, Adaptive), frame-rate counter, the HUD on or off, skip the
  warnings and intro movies
- **Sound**: Positional Audio (3D sounds placed left and right: Clear, the default, by level only;
  Console, through the sound chip's filters as on the Xbox, duller; Off, centred), Audio Output (Auto, Stereo
  or 5.1; Auto is 5.1 on a 5.1 or 7.1 output; a change needs a restart of the game)
- **Gameplay** (each off unless chosen): the Quick Menu, an NG2-style d-pad menu for weapons, Ninpo
  and items, and whether the game pauses while it is open; **Intercept** (Black or Hurricane), the Hurricane
  Pack's guard counter Black left out: tap guard as an attack lands to turn it aside; **Essence-Only
  Charging**, Ultimate Techniques charged only by absorbing essence, not by holding attack (#120); **Skip Any
  Cutscene**, Start skipping movies and in-game cutscenes not yet watched (#139); **Responsive Menus**, Start
  taken at once at the title, in a cutscene's opening wait and in the pause menu after equipping, where the game
  ignores it for a while (#116)
- **Cheats** (for the session only): infinite health, ninpo, essence, items, projectiles; golden scarabs.
  Its **Unlocks** page: all difficulties from the start, all costumes, and the main menu's Missions (with every
  mission), Arcade Ninja Gaiden and Movie Theater without earning them (saved); All Weapons (every melee and
  projectile weapon at its highest level), All Armlets (the armlets and bands) and All Scrolls (the Izuna Drop,
  Counter Attack and Guillotine Throw technique scrolls), for the session: turned off, each takes back what it
  gave and puts back your own pickups; a save made while one is on keeps what it gave
- **Controls**: rebind every action (two keys or mouse buttons and a controller button each); camera
  inversion; start in third person; Camera Sensitivity; Toggle HUD (not bound unless chosen). The
  bindings follow the game's own Button Settings: an action keeps its key or button whatever the
  game's Options assign

The game's own Options (sound, movie aspect, controller vibration and so on) are where they always
were.

## Controls

On Android, an Xbox 360 pad is drawn over the screen: the two sticks, the d-pad, A B X Y, the bumpers
and triggers, Back, Start and the stick clicks. It hides while a controller is connected.

A controller works as on the Xbox. The keyboard and mouse can be used at any time — the game follows
whichever was touched last, and its button prompts change to match (or stay on one set: "Button
Prompts" on the "Controls" page, Auto, Xbox, PlayStation or PC).

| Action | Keys | | Action | Keys |
|---|---|---|---|---|
| Move | W A S D | | Camera | mouse (numpad 8 4 5 6) |
| Attack | left mouse, J | | Special attack | right mouse, K |
| Jump | Space | | Projectile | E |
| Ninpo | F | | Wind run | C |
| Guard / roll | Shift | | Center camera | Q, middle mouse |
| Map | Tab | | Karma window | R |
| Pause | Esc, Enter | | First-person view | Z |

After loading, the right stick (or the mouse) turns the third-person camera; click it (V) to look around
in first person. Turn off **Start in Third Person** (RECOMP SETTINGS → Controls) to start each load in
first person, as on the Xbox. **Invert Camera X / Y**
flips the stick and mouse for the third-person camera, **Invert First-Person X** first-person left and
right; first-person up and down is the game's own Options → Controller Settings → 1st Person Camera
Control. **Camera Sensitivity** (25%–300%) sets how fast the mouse, the right stick and the camera keys
turn it; over 100% the camera turns faster than the game's own top speed (the third-person camera's
left and right; its up and down stays the game's).

With the **Quick Menu** on (RECOMP SETTINGS → Gameplay), Up and Down on the d-pad (or the Up / Down
arrows, 1 / 2) open it on the melee weapons or the items, and Left and Right on the previous or next
melee weapon; Up and Down then change category, Left and Right choose, A / Enter equips or uses, B /
Escape closes. Its keys are in Controls.

In the menus the mouse points and clicks, Esc goes back and Q / E switch the pause menu's tabs.
With Skip Any Cutscene (Gameplay), movies and in-game cutscenes can be skipped with Start (or
its key), watched before or not.

Other keys: **F10** frame-rate counter, **F11** or **Alt+Enter** fullscreen.

## The arcade Ninja Gaiden

The 1988 arcade Ninja Gaiden is a separate program on the disc (`NinjaTrilogy.xbe`, and
`NinjaTomoe.xbe` for the Japanese version) that the Xbox switches to. It is recompiled too: the
packages carry `NinjaTrilogy` and `NinjaTomoe` next to the game, and the game hands over to them from
the main menu's **ARCADE NINJA GAIDEN** and the cabinet in Han's Bar. In the arcade, Back pauses:
**B** quits back to Ninja Gaiden Black, **Y** goes to the arcade's title. It opens in the window mode
the game was in.

## Known issues

- Above 1x render resolution, a faint light line can show across the dagger in the pause menu (#83).
- The Windows build has been tested under Proton and in a Windows 11 virtual machine; reports from
  real Windows PCs are especially welcome.

When reporting a graphics glitch, pressing **F7** while it is on screen records the next few seconds of
drawing to `~/ngb-trace-<n>` on Linux, or `C:\Users\<you>\ngb-trace-<n>` on Windows — attach that
folder (zipped), it shows exactly what went wrong.

## Building from source

The build needs the game's code, so it starts from your own disc image.

Requirements: clang, CMake, Ninja, Python 3, git, SDL2, GLib, glslang and SPIRV-Tools (Qt 6
Widgets for the setup window, optional). The recompiler and Xbox runtime,
[xboxrecomp](https://github.com/sp00nznet/xboxrecomp) with this port's changes, is included in
`xboxrecomp/`.

```sh
xboxrecomp/third_party/fetch.sh                # once: Vulkan headers, volk, VMA and the like
tools/pipeline.sh /path/to/your/ngb.iso        # extract, disassemble and lift to C (a few minutes)
cmake -S . -B build/linux -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
ninja -C build/linux
build/linux/NinjaGaidenBlack
```

Packages: `packaging/appimage/build.sh` (in a Docker container, for an AppImage that runs on older
systems), `packaging/windows/build.sh` (cross-compiled from Linux with llvm-mingw),
`packaging/android/build.sh` (below), and `packaging/macos/build.sh` (native Apple Silicon). Packages
contain no game data.

For Android, after the fetch and pipeline steps above (on Linux), install the Android SDK with an NDK,
build-tools and a platform (Android Studio's SDK Manager, or `sdkmanager "ndk;27.2.12479018"
"build-tools;35.0.0" "platforms;android-35"`), and a JDK, then:

```sh
packaging/android/build.sh      # ANDROID_HOME=<sdk> if it is not ~/Android/Sdk
adb install -r build/android-release/NinjaGaidenBlack-android-arm64.apk
```

It builds SDL2 and glslang for the phone once (into `build/android-deps`), then the game, and signs the
APK with a key it makes there (`debug.keystore`): keep that file, as Android only installs an update
over an app signed with the same key.

On macOS, install the build dependencies below, then run the fetch and pipeline steps above
and package the app:

```sh
brew install cmake ninja sdl2-compat sdl3 libepoxy glib glslang molten-vk vulkan-loader qt
packaging/macos/build.sh
```

The zip is `build/macos-release/NinjaGaidenBlack-macos-arm64.zip`. Homebrew is needed only to
build; runtime libraries, including MoltenVK, are bundled. `NGB_MAC_PREFIX` overrides the build
dependency prefix (default: `brew --prefix`).

How it works, and the details of every feature: [docs/TECHNICAL.md](docs/TECHNICAL.md) and
[docs/renderer.md](docs/renderer.md). How to contribute: [CONTRIBUTING.md](CONTRIBUTING.md); the layout, tools
and test rules, also for coding agents: [AGENTS.md](AGENTS.md).

## Credits

- [xboxrecomp](https://github.com/sp00nznet/xboxrecomp) by sp00nznet: the recompiler and Xbox
  runtime this is built on (MIT)
- [xemu](https://xemu.app): the NV2A GPU model and the APU's voice processor (the files used here are
  LGPL-2.1-or-later, its shader generators GPL-2.0 or 3.0; xemu as a whole is GPL-2.0)
- TGP482's [SCDA-XBERecomp](https://github.com/TGP482/SCDA-XBERecomp) and
  [Xbox-XBERecomp](https://github.com/TGP482/Xbox-XBERecomp): the renderer's shader, vertex and
  texture code, and the sound mixer's model (MIT)
- Fonts: EB Garamond and Liberation Serif (SIL Open Font License)
- Keyboard, mouse and controller prompts, and the touch controls' buttons: Xelu's Free Controller & Key
  Prompts (CC0)
- The touch controls' sticks and d-pad: Kenney's [Mobile Controls](https://kenney.nl/assets/mobile-controls) (CC0)
- Banner lettering: Chakra Petch by Cadson Demak (SIL Open Font License)

### Contributors

Code:

- [akirahayabusa](https://github.com/akirahayabusa): the freeze in scenes with the sun (#40), folders
  with non-ASCII names on Windows (#41), the Difficulty Selection highlight (#42); and the Hurricane
  patch (vb-zh x2.1), whose Intercept switch showed where Black had left the mechanic out (#109, #132)
- [ManemanFTW](https://github.com/ManemanFTW) (Manuel Flores): the Windows start-up hang (#9) and the
  audio crackling on Windows (#34)
- [HammerD72](https://github.com/HammerD72): the audio timing fix (#32); the two spots where the game
  picks the camera view at a load, for Start in Third Person (#23); impact sparks at 21:9 (#65); Smoother
  Glow (#64); the Quick Menu (#60)
- [retro-meister](https://github.com/retro-meister): the macOS port (#49), and the community macOS builds

Testing, saves and traces:

- [Radarzo](https://github.com/Radarzo), [hj-0](https://github.com/hj-0),
  [hyperspeedgx](https://github.com/hyperspeedgx), [serguntchik](https://github.com/serguntchik),
  [PaulAndrewClift](https://github.com/PaulAndrewClift) and
  [phantuannghia95-rgb](https://github.com/phantuannghia95-rgb): seven test builds, screenshots and logs
  on Nvidia cards that tracked down the banded water (#16)
- [HammerD72](https://github.com/HammerD72): testing on Nvidia across many issues (shadows #53, water
  #16, menus and controls)
- [DAVID106-debug](https://github.com/DAVID106-debug): the missing particles and the Xbox comparison
  (#51), the high-resolution shadow flicker (#53)
- [hyperspeedgx](https://github.com/hyperspeedgx): saves across chapters 1 to 9, traces for the torch
  embers (#51) and the Chapter 5 roof (#29)
- [hj-0](https://github.com/hj-0): saves for chapters 11 to 13 and a guide to moving saves from xemu
  (#61)
- [yoyooo941-sys](https://github.com/yoyooo941-sys): the save that made the volcano strip reproducible
  (#55)
- [firepro1211](https://github.com/firepro1211) and
  [huokunlongCHAIN](https://github.com/huokunlongCHAIN): the Chapter 5 roof, including a comparison on
  the Xbox One (#29)
- [matitone71](https://github.com/matitone71): the Steam Deck Game Mode log that found the launch
  problem (#11)

To report a bug, test a build or send a change, see [CONTRIBUTING.md](CONTRIBUTING.md).

*Ninja Gaiden* is a trademark of Koei Tecmo. This project is not affiliated with or endorsed by Koei
Tecmo or Team Ninja.

## License

This project's code is free software: you can redistribute it and/or modify it under the terms of
the GNU General Public License as published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version (`GPL-3.0-or-later`; the text is in [LICENSE](LICENSE)).
Up to 0.9.24 it was GPL-2.0-or-later. It is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY.
The parts it is built from keep their own licenses: see [THIRD_PARTY.md](THIRD_PARTY.md).
