# Third-party components

This project's own code is under the GNU General Public License, version 3 or (at your option) any
later version (GPL-3.0-or-later, [LICENSE](LICENSE); up to 0.9.24, GPL-2.0-or-later); the
`xboxrecomp/` directory keeps its MIT license. The project is built from, and its release packages contain, the following parts, each under
its own license.

## In this repository

| Component | Where | License |
|---|---|---|
| TGP482's [Xbox XBE Recomp SDK](https://github.com/TGP482/Xbox-XBERecomp) / [SCDA-XBERecomp](https://github.com/TGP482/SCDA-XBERecomp) renderer code | `src/d3d/gpu`, and followed by `src/d3d/hle.cpp`, `src/d3d/vk/backend.cpp` | MIT — see `src/d3d/NOTICE` |
| TGP482's [SCDA-XBERecomp](https://github.com/TGP482/SCDA-XBERecomp) DirectSound code | followed by `src/audio/dsound_hle.cpp` | MIT — see `src/audio/NOTICE` |
| EB Garamond | `assets/fonts` | SIL Open Font License 1.1 (`assets/fonts/OFL-EBGaramond.txt`) |
| Liberation Serif | `assets/fonts` | SIL Open Font License 1.1 (`assets/fonts/OFL-Liberation.txt`) |
| Xelu's Free Controller & Key Prompts | `assets/prompts/keys`, `assets/prompts/ps`, `assets/prompts/switch`, `assets/prompts/xbox` | CC0 (`assets/prompts/License-Xelu.txt`) |
| Kenney's Mobile Controls (the touch controls' sticks and d-pad) | `assets/touch` | CC0 (`assets/touch/License-Kenney.txt`) |
| SDL2's Android activity (Java) | compiled from SDL's source by `packaging/android/build.sh` | zlib |

## The runtime: xboxrecomp

[xboxrecomp](https://github.com/sp00nznet/xboxrecomp) (the recompiler and the Xbox runtime this game
runs on, in `xboxrecomp/` with this port's changes) is MIT licensed (`xboxrecomp/LICENSE`); its
`NOTICE` lists what it contains or fetches under other licenses, among them:

| Component | License |
|---|---|
| xemu's NV2A GPU model and Vulkan renderer, MCPX APU (`src/nv2a_vk`, `src/apu`) | LGPL-2.1-or-later |
| xemu's shader generators (`src/nv2a_vk/xemu/pgraph/glsl/psh.c`, `vsh-prog.c`, `vsh-prog.h`, `pgraph/vsh_regs.h`) | GPL-2.0 or GPL-3.0 |
| volk, Vulkan Memory Allocator, Vulkan-Headers | MIT / Apache-2.0 |
| SPIRV-Reflect | Apache-2.0 |
| stb (stb_truetype, stb_image) | MIT / public domain |
| xxHash | BSD-2-Clause |

## Libraries in the release packages

| Library | License | Package |
|---|---|---|
| SDL2 | zlib | all |
| GLib, libintl, libiconv | LGPL-2.1-or-later | Linux, Windows (Windows: libintl, libiconv; Android uses its own small stand-in, `xboxrecomp/src/nv2a_vk/shim/glib-lite`) |
| PCRE2 | BSD-3-Clause | Linux, Windows |
| glslang | BSD-3-Clause and others (see its LICENSE) | all |
| SPIRV-Tools | Apache-2.0 | Linux, Windows |
| LLVM libc++ (the NDK's, linked in) | Apache-2.0 with LLVM exception | Android |
| Mesa's Turnip (Vulkan driver for Adreno, built from Mesa's release) | MIT | Android |
| libadrenotools | BSD-2-Clause | Android |
| libepoxy | MIT | Linux |
| Qt 6 (the setup window) | LGPL-3.0 | Linux |
| LLVM libc++, libunwind | Apache-2.0 with LLVM exception | Windows |
| mingw-w64 winpthreads | MIT and ZPL-2.1 | Windows |

The LGPL libraries are shared libraries inside the packages and can be replaced by other builds of the
same version.

## The game

*Ninja Gaiden Black* © Koei Tecmo / Team Ninja. None of the game's data (graphics, sound, movies,
text) is in this repository, and its recompiled code is generated from the player's own copy when
building (`tools/pipeline.sh`); the game's data is installed from it on the first start. The
repository does include C translations of some of the game's functions, taken from that generated code
and modified for this port (the menus, the mouse, the cheats: `src/main_menu.c`, `src/pause_tab.c`,
`src/menu_mouse.c` and others, listed in `src/manual_functions.json`), and the Direct3D functions of
the Xbox SDK the game was built with (`src/d3d/d3d_orig.c`), behind the renderer's hooks.
