![Ninja Gaiden Black Recompiled](docs/images/banner.jpg)

# Ninja Gaiden Black Recompiled

**A PC port of Ninja Gaiden Black by static recompilation.**

A native Linux and Windows version of *Ninja Gaiden Black* (Xbox, 2005), made by statically
recompiling the game's own code to run directly on a PC — not an emulator. It draws with its own
Vulkan renderer, which takes over the game's Direct3D calls instead of emulating the Xbox GPU. It
plays at a steady 60 fps on modest hardware (including the Steam Deck) and adds PC options: higher
resolutions, widescreen and ultrawide, keyboard and mouse, rebindable controls and more.

**You need your own copy of the game.** This project contains none of the game's data (graphics,
sound, movies, text): on first start it asks for your disc image and installs the game from it. It is made for the **retail USA
release** (NTSC-U, title ID `5443000D`) and checks for it.

## Features

- Native Linux (AppImage) and Windows (x64) builds; Steam Deck friendly
- Render resolution up to 10x, anisotropic filtering, MSAA, sharper shadows
- 16:9, 16:10 and 21:9 as well as 4:3: the 3D fills the screen, the HUD goes to the screen's
  corners, menus keep the game's own 16:9 look
- Keyboard and mouse with key prompts in the game, rebindable controls for keyboard, mouse and
  controller, mouse support in the menus
- Skippable intros and movies, unlock-all options for difficulties and costumes, optional cheats
- Smooth frame pacing and low input latency; shaders prepared in the background (no first-time
  stutter)

## Getting started

### Linux and Steam Deck

1. Download `NinjaGaidenBlack-x86_64.AppImage` from the releases page and make it executable
   (`chmod +x`, or Properties → Allow executing).
2. Run it. On the first start a window asks for your copy of the game: a disc image (`.iso`, a full
   Redump dump, or the `.7z`/`.zip` it came in) or a folder of extracted files with `default.xbe`.
   The game is installed once (about 6 GB).
3. On a Steam Deck, add the AppImage to Steam (Add a Non-Steam Game) to play it in Game Mode, and
   choose 16:10 in the settings.

Needs a 64-bit Linux with glibc 2.35 or newer and a GPU with Vulkan 1.3 drivers.

### Windows

1. Download `NinjaGaidenBlack-windows-x64.zip` and extract it anywhere.
2. Run `NinjaGaidenBlack.exe` and choose your disc image as above.

Needs Windows 10 or 11 (64-bit) and a GPU with Vulkan 1.3 drivers.

### macOS (experimental)

Build the Apple Silicon app using the commands below, extract the resulting zip, and open
`Ninja Gaiden Black.app`. Choose your own disc image or extracted game folder on first start.
Current dependencies require macOS 26. Intel Macs and the xemu renderer are unsupported;
packages are signed ad hoc, without Developer ID signing or notarization.

### Where things are kept

| | Linux | Windows | macOS |
|---|---|---|---|
| Installed game, saves | `~/.local/share/NinjaGaidenBlack` | `%LOCALAPPDATA%\NinjaGaidenBlack` | `~/Library/Application Support/NinjaGaidenBlack/saves` |
| Settings (`settings.ini`, `controls.ini`) | `~/.config/NinjaGaidenBlack` | `%APPDATA%\NinjaGaidenBlack` | `~/Library/Application Support/NinjaGaidenBlack/config` |
| Shader cache | `~/.cache/NinjaGaidenBlack` | `%LOCALAPPDATA%\NinjaGaidenBlack\shaders` | `~/Library/Application Support/NinjaGaidenBlack/cache/NinjaGaidenBlack/shaders` |
| Log | the terminal | `log.txt` beside the saves (or the console it was started from) | `log.txt` beside the saves |

**Portable mode:** with a file named `portable.txt` next to `NinjaGaidenBlack.exe` (or the AppImage or Mac app),
all of these go into a `userdata` folder next to it instead. The first-run setup offers this as a
checkbox. The folder must be writable, so not inside Program Files. Delete `portable.txt` to go back.

## Settings

Everything is in the game's own menus: **RECOMP SETTINGS** on the main menu, and the **Settings** tab
of the pause menu. Changes apply at once and are saved.

- **Graphics**: render resolution (1x–10x), texture filtering (the console's, or 2x–16x
  anisotropic), anti-aliasing (MSAA), shadow resolution
- **Display**: aspect ratio (4:3, 16:9, 16:10, 21:9), window size, windowed / fullscreen /
  borderless, V-Sync (Off, Mailbox, FIFO, Adaptive), frame-rate counter
- **Startup**: skip the warnings and intro movies, all difficulties from the start, all costumes
- **Cheats** (for the session only): infinite health, ninpo, essence; golden scarabs
- **Controls**: rebind every action (two keys or mouse buttons and a controller button each)

The game's own Options (sound, movie aspect, controller vibration and so on) are where they always
were.

## Controls

A controller works as on the Xbox. The keyboard and mouse can be used at any time — the game follows
whichever was touched last, and its button prompts change to match.

| Action | Keys | | Action | Keys |
|---|---|---|---|---|
| Move | W A S D | | Camera | mouse (numpad 8 4 5 6) |
| Attack | left mouse, J | | Special attack | right mouse, K |
| Jump | Space | | Projectile | E |
| Ninpo | F | | Wind run | C |
| Guard / roll | Shift | | Center camera | Q, middle mouse |
| Map | Tab | | Karma window | R |
| Pause | Esc, Enter | | First-person view | Z |

After loading, the right stick (or the mouse) looks around in first person, as on the Xbox; click the
right stick (V) to turn the third-person camera instead. **Invert Camera X / Y** (RECOMP SETTINGS →
Controls) flips the stick and mouse for both; the game's own Options → Controller Settings → 1st Person
Camera Control can flip first-person look back.

In the menus the mouse points and clicks, Esc goes back and Q / E switch the pause menu's tabs.
Movies can be skipped with Start, A or B (or the keys for them); in-game cutscenes with Start.

Other keys: **F10** frame-rate counter, **F11** or **Alt+Enter** fullscreen.

## Known issues

- **Water** in some places (the pond in Chapter 2, for example) can look banded instead of clear, so
  far reported on Nvidia cards on Windows (#16).
- **Controller vibration on Windows** may only start working after switching Vibration off and on
  again in the game's Options (#15).
- **Installing directly from an ISO** fails on some Windows PCs (#6, #14). Until that is fixed:
  extract the ISO to a folder named `game` next to `NinjaGaidenBlack.exe`, and it is found
  automatically.
- **Steam Deck:** do the first start (installing the game) in Desktop Mode. In Game Mode the window
  to choose your disc image cannot open, and the game closes (#11).
- The **Ninja Gaiden arcade cabinet** in Han's bar does not work: the game closes when you play it
  (#18). The NES games are a separate program on the disc (`NinjaTrilogy.xbe`) that the Xbox switches
  to, and only Ninja Gaiden Black itself is recompiled. Save before trying it.
- At **21:9**, objects at the far left and right of the screen can appear a moment late (the game
  decides what to draw for a narrower view).
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
systems), `packaging/windows/build.sh` (cross-compiled from Linux with llvm-mingw), and
`packaging/macos/build.sh` (native Apple Silicon). Packages contain no game data.

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
[docs/renderer.md](docs/renderer.md). For contributors and coding agents: [AGENTS.md](AGENTS.md).

## Credits

- [xboxrecomp](https://github.com/sp00nznet/xboxrecomp) by sp00nznet: the recompiler and Xbox
  runtime this is built on (MIT)
- [xemu](https://xemu.app): the NV2A GPU model and the APU's voice processor (the files used here are
  LGPL-2.1-or-later; xemu as a whole is GPL-2.0)
- TGP482's [SCDA-XBERecomp](https://github.com/TGP482/SCDA-XBERecomp) and
  [Xbox-XBERecomp](https://github.com/TGP482/Xbox-XBERecomp): the renderer's shader, vertex and
  texture code (MIT)
- Fonts: EB Garamond and Liberation Serif (SIL Open Font License)
- Keyboard and mouse prompts: Xelu's Free Controller & Key Prompts (CC0)
- Banner lettering: Chakra Petch by Cadson Demak (SIL Open Font License)

### Contributors

- [akirahayabusa](https://github.com/akirahayabusa): the freeze in scenes with the sun (#40), folders with
  non-ASCII names on Windows (#41), the Difficulty Selection highlight (#42)
- [ManemanFTW](https://github.com/ManemanFTW): the Windows start-up hang (#9) and the audio crackling on
  Windows (#34)
- [HammerD72](https://github.com/HammerD72): the audio timing fix (#32)

*Ninja Gaiden* is a trademark of Koei Tecmo. This project is not affiliated with or endorsed by Koei
Tecmo or Team Ninja.

## License

This project's code is free software: you can redistribute it and/or modify it under the terms of
the GNU General Public License as published by the Free Software Foundation, either version 2 of the
License, or (at your option) any later version (`GPL-2.0-or-later`; the text of version 2 is in
[LICENSE](LICENSE)). It is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY.
The parts it is built from keep their own licenses: see [THIRD_PARTY.md](THIRD_PARTY.md).
