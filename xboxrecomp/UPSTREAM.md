# Where this copy comes from

This directory is [xboxrecomp](https://github.com/sp00nznet/xboxrecomp) by sp00nznet (MIT, see
`LICENSE`), kept in the Ninja Gaiden Black repository with the changes made for this port. It is
based on upstream commit `1409a7d` (2026-09-30); the changes since were made on a `linux-vulkan`
branch (71 commits, plus the Windows work) and are not in upstream.

What the changes add, in short:

- **Linux / POSIX host**: the 4 GB guest window, vectored exception handling over `sigaction`, real
  thread suspension and a single virtual CPU scheduler for the guest's threads (`src/platform`).
- **Windows host on the same scheduler**: the virtual CPU ported to Windows (suspend and context
  redirect), placeholder-based guest memory reservation, per-thread IRQL with clang-mingw
  (`src/platform/win32_host.c`, `win32_vcpu.h`, `src/kernel/xbox_memory_layout.c`).
- **GPU**: xemu's NV2A model and Vulkan renderer (`src/nv2a_vk`, LGPL-2.1-or-later, imported
  verbatim and then extended): dirty-page tracking, render scale, shadow-map scale, SPIR-V and
  pipeline caches, texture dumping and replacement, frame pacing and presentation (mailbox, FIFO,
  adaptive V-Sync), an FPS overlay hook.
- **Audio**: the MCPX APU's voices resampled to 48 kHz, a stereo downmix with reverb, an SDL2 sink
  paced by the output device.
- **Kernel and lifter fixes** found running Ninja Gaiden Black: interrupt lines, thread objects,
  events, file and video calls; lifter flag joins, conversions, switch tables; differential testing
  tools (`tools/`).

Its third-party sources are not stored here: `third_party/fetch.sh` fetches them at pinned
revisions.

The Apple Silicon macOS port imports `src/platform/mmio_decode_a64.h` from upstream
commit `eb0aa1d` (v0.14).
