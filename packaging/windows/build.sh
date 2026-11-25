#!/usr/bin/env bash
# Ninja Gaiden Black (static recompilation) for Windows x64, cross-compiled on Linux.
#
#   packaging/windows/build.sh [out-dir]      (default build/windows-release)
#
# Fetches llvm-mingw (clang, lld, libc++, UCRT) and the MSYS2 CLANG64 packages in deps.txt into
# build/windows-deps (or NGB_WIN_DEPS) once, builds the game against them
# (cmake/toolchains/windows-x64-llvm-mingw.cmake) and packs <out-dir>/NinjaGaidenBlack/ -- the
# program, the DLLs it needs that Windows does not ship, the shader seeds -- into
# <out-dir>/NinjaGaidenBlack-windows-x64.zip. Like the AppImage it holds no game data: the first
# launch asks for the player's disc image and installs it into %LOCALAPPDATA%\NinjaGaidenBlack.
#
# Needs: cmake, ninja, curl, bsdtar, python3. The recompiled code must already be generated
# (tools/pipeline.sh).
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
repo=$(cd "$here/../.." && pwd)
deps=$(mkdir -p "${NGB_WIN_DEPS:-$repo/build/windows-deps}" && cd "${NGB_WIN_DEPS:-$repo/build/windows-deps}" && pwd)
out=$(mkdir -p "${1:-$repo/build/windows-release}" && cd "${1:-$repo/build/windows-release}" && pwd)
llvm_mingw=llvm-mingw-20260922-ucrt-ubuntu-22.04-x86_64
msys=https://repo.msys2.org/mingw/clang64

[ -n "$(ls "$repo"/src/recomp/gen/*.c 2>/dev/null)" ] ||
    { echo "No generated code: run tools/pipeline.sh first." >&2; exit 1; }

# The toolchain
if [ ! -x "$deps/$llvm_mingw/bin/clang" ]; then
    curl -fL -o "$deps/$llvm_mingw.tar.xz" \
        "https://github.com/mstorsjo/llvm-mingw/releases/download/20260922/$llvm_mingw.tar.xz"
    bsdtar -xf "$deps/$llvm_mingw.tar.xz" -C "$deps"
fi

# The sysroot: unpacked again whenever deps.txt changes
want=$(grep -v '^#' "$here/deps.txt" | grep .)
if [ "$want" != "$(cat "$deps/sysroot.stamp" 2>/dev/null)" ]; then
    mkdir -p "$deps/pkgs"
    rm -rf "${deps:?}/sysroot"
    mkdir -p "$deps/sysroot"
    for p in $want; do
        [ -s "$deps/pkgs/$p" ] || curl -fL -o "$deps/pkgs/$p" "$msys/$p"
        bsdtar -xf "$deps/pkgs/$p" -C "$deps/sysroot" clang64
    done
    echo "$want" > "$deps/sysroot.stamp"
fi

cmake -S "$repo" -B "$out/build" -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$repo/cmake/toolchains/windows-x64-llvm-mingw.cmake" \
    -DXBOXRECOMP_DIR="$(cd "${XBOXRECOMP_DIR:-$repo/xboxrecomp}" && pwd)" \
    -DNGB_WIN_DEPS="$deps" -DCMAKE_BUILD_TYPE=Release >"$out/configure.log"
# The arcade Ninja Gaiden (English and Japanese) the game hands over to (#18): built when generated.
arcade=$(ninja -C "$out/build" -t targets all | grep -oE '^ninja_(trilogy|tomoe):' | tr -d : | tr '\n' ' ')
ninja -C "$out/build" -j"${JOBS:-$(nproc)}" ninja_gaiden_black $arcade >"$out/build.log"

pkg=$out/NinjaGaidenBlack
rm -rf "${pkg:?}"
mkdir -p "$pkg/share/NinjaGaidenBlack/shader-seed" "$pkg/licenses"
bin=$deps/$llvm_mingw/bin
"$bin/llvm-strip" -o "$pkg/NinjaGaidenBlack.exe" "$out/build/NinjaGaidenBlack.exe"
for a in NinjaTrilogy NinjaTomoe; do
    [ -f "$out/build/$a.exe" ] && "$bin/llvm-strip" -o "$pkg/$a.exe" "$out/build/$a.exe"
done
# The DLLs it needs that Windows does not ship: the C++ and thread runtimes from llvm-mingw (the
# compiler's own), the rest from the sysroot. Not the Vulkan loader: that is the GPU driver's.
python3 "$here/dlldeps.py" "$bin/llvm-objdump" "$pkg/NinjaGaidenBlack.exe" \
    "$deps/$llvm_mingw/x86_64-w64-mingw32/bin" "$deps/sysroot/clang64/bin" |
    while read -r dll; do cp "$dll" "$pkg/"; done
cp "$repo/packaging/d3d-seed.bin" "$pkg/share/NinjaGaidenBlack/"
cp "$repo/packaging/shader-seed/states.bin" "$repo/packaging/shader-seed/pipelines.bin" \
   "$pkg/share/NinjaGaidenBlack/shader-seed/"
cp "$repo"/assets/fonts/OFL-*.txt "$pkg/licenses/"
cp "$repo/LICENSE" "$repo/THIRD_PARTY.md" "$pkg/licenses/"
toolkit=$(cd "${XBOXRECOMP_DIR:-$repo/xboxrecomp}" && pwd)
cp "$repo/src/d3d/NOTICE" "$pkg/licenses/NOTICE-renderer.txt"
cp "$repo/src/LICENSE-Pillow.txt" "$pkg/licenses/"
cp "$toolkit/LICENSE" "$pkg/licenses/LICENSE-xboxrecomp.txt"
cp "$toolkit/NOTICE" "$pkg/licenses/NOTICE-xboxrecomp.txt"
cp "$toolkit/LICENSES/LGPL-2.1.txt" "$pkg/licenses/"
cat > "$pkg/README.txt" <<'TXT'
Ninja Gaiden Black - static recompilation for Windows (Vulkan)

The game itself is not included. Start NinjaGaidenBlack.exe and choose your own copy of
the retail USA Xbox release: a disc image (.iso, a full Redump dump, or the .7z/.zip it came in)
or a folder of extracted files with default.xbe. A disc image is installed once (about 6 GB) into
%LOCALAPPDATA%\NinjaGaidenBlack\game; saves are kept there too, settings in
%APPDATA%\NinjaGaidenBlack.

Needs Windows 10 or 11 (64-bit) and a GPU with Vulkan 1.3 drivers (or older drivers that have
its dynamic rendering and synchronization2 as extensions).
Start it from a command prompt to see its log.
TXT
rm -f "$out/NinjaGaidenBlack-windows-x64.zip"
(cd "$out" && bsdtar -a -cf NinjaGaidenBlack-windows-x64.zip NinjaGaidenBlack)
echo "Packed:"; ls "$pkg"
ls -l "$out/NinjaGaidenBlack-windows-x64.zip"
