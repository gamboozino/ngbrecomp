#!/usr/bin/env bash
# Ninja Gaiden Black (static recompilation) for Linux as an AppImage.
#
#   packaging/appimage/build.sh [out-dir]      (default build/appimage)
#
# Compiles the game and the xboxrecomp toolkit (xboxrecomp/ in this
# repository, or XBOXRECOMP_DIR) in an Ubuntu 22.04 container (Dockerfile here), so the
# AppImage needs only glibc 2.35, and packs it with appimagetool into
# <out-dir>/NinjaGaidenBlack-x86_64.AppImage.
#
# The recompiled code must already be generated (tools/pipeline.sh). The
# AppImage holds the program and the libraries a system may lack (SDL2,
# glib, libepoxy; libstdc++ linked in). It holds no game data:
# the first launch asks for the player's disc image and installs the files
# into ~/.local/share/NinjaGaidenBlack/game (see src/install.c).
set -euo pipefail

here=$(cd "$(dirname "$0")" && pwd)
repo=$(cd "$here/../.." && pwd)
toolkit=$(cd "${XBOXRECOMP_DIR:-$repo/xboxrecomp}" && pwd)
out=$(mkdir -p "${1:-$repo/build/appimage}" && cd "${1:-$repo/build/appimage}" && pwd)
image=ngb-build:jammy
jobs=${JOBS:-$(nproc)}

[ -n "$(ls "$repo"/src/recomp/gen/*.c 2>/dev/null)" ] ||
    { echo "No generated code: run tools/pipeline.sh first." >&2; exit 1; }

docker build -q -t "$image" "$here" >/dev/null

# The toolkit is the copy in the repository (xboxrecomp/), or XBOXRECOMP_DIR
# mounted beside it.
mounts=(-v "$repo":/work/ngb-recomp -v "$out":/out)
toolkit_in=/work/ngb-recomp/xboxrecomp
if [ "$toolkit" != "$repo/xboxrecomp" ]; then
    mounts+=(-v "$toolkit":/work/xboxrecomp:ro)
    toolkit_in=/work/xboxrecomp
fi

docker run --rm -m 14g -u "$(id -u):$(id -g)" -e HOME=/tmp "${mounts[@]}" \
    "$image" bash -euc '
cmake -S /work/ngb-recomp -B /out/build -G Ninja \
    -DXBOXRECOMP_DIR='"$toolkit_in"' \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_EXE_LINKER_FLAGS="-static-libstdc++ -static-libgcc -fuse-ld=lld" \
    -DCMAKE_PREFIX_PATH=/usr/local >/out/configure.log
ninja -C /out/build -j'"$jobs"' >/out/build.log

app=/out/AppDir
rm -rf "$app"
mkdir -p "$app/usr/bin" "$app/usr/lib"
cp /out/build/NinjaGaidenBlack /out/build/ngb-setup "$app/usr/bin/"
llvm-strip "$app/usr/bin/NinjaGaidenBlack" "$app/usr/bin/ngb-setup"
# The arcade Ninja Gaiden (English and Japanese) the game hands over to (#18).
for a in NinjaTrilogy NinjaTomoe; do
    [ -f "/out/build/$a" ] && cp "/out/build/$a" "$app/usr/bin/" && llvm-strip "$app/usr/bin/$a"
done

# Bundle what a system may lack; never glibc, the Vulkan loader or GPU and
# window-system libraries, which must be the systems own.
for lib in $(ldd "$app/usr/bin/NinjaGaidenBlack" | awk "/=>/ {print \$3}"); do
    case "$(basename "$lib")" in
    libSDL2-2.0.so*|libglib-2.0.so*|libpcre.so*|libepoxy.so*) cp -L "$lib" "$app/usr/lib/" ;;
    esac
done

# The setup window: Qt and what it needs that a system may lack, and the
# xcb and Wayland platform plugins (found through qt.conf beside it).
qtp=$(dirname "$(dirname "$(find /usr/lib -path "*qt6/plugins/platforms/libqxcb.so" | head -1)")")
mkdir -p "$app/usr/plugins"
for d in platforms/libqxcb.so platforms/libqwayland-generic.so \
         wayland-shell-integration wayland-decoration-client platforminputcontexts; do
    mkdir -p "$app/usr/plugins/$(dirname "$d")"
    cp -rL "$qtp/$d" "$app/usr/plugins/$(dirname "$d")/"
done
printf "[Paths]\nPrefix = ..\nPlugins = plugins\n" > "$app/usr/bin/qt.conf"
# Never bundled: glibc, the C++ runtime, GPU, X11/Wayland, font and
# session libraries, which must be the systems own.
system_lib="^(ld-linux|libc|libm|libdl|librt|libpthread|libresolv|libutil|libstdc\+\+|libgcc_s|libGL|libEGL|libGLX|libGLdispatch|libOpenGL|libdrm|libgbm|libvulkan|libX11|libXau|libXdmcp|libXext|libXrender|libxcb\.so|libxcb-(shm|render|xfixes|randr|shape|sync|xkb|xinput|glx|dri2|dri3|present)\.so|libwayland-|libfontconfig|libfreetype|libharfbuzz|libexpat|libz\.|libdbus-1|libudev|libsystemd|libgpg-error|libselinux|libmount|libblkid|libffi|liblzma|liblz4|libzstd|libgcrypt|libcap|libbsd|libmd\.)"
bundle_deps() {
    ldd "$1" | awk "/=>/ {print \$3}" | while read -r lib; do
        b=$(basename "$lib")
        echo "$b" | grep -Eq "$system_lib" && continue
        [ -e "$app/usr/lib/$b" ] || cp -L "$lib" "$app/usr/lib/"
    done
}
bundle_deps "$app/usr/bin/ngb-setup"
for so in $(find "$app/usr/plugins" -name "*.so"); do bundle_deps "$so"; done

patchelf --set-rpath "\$ORIGIN/../lib" "$app/usr/bin/NinjaGaidenBlack" "$app/usr/bin/ngb-setup"
for a in NinjaTrilogy NinjaTomoe; do [ -f "$app/usr/bin/$a" ] && patchelf --set-rpath "\$ORIGIN/../lib" "$app/usr/bin/$a"; done
for so in "$app"/usr/lib/*.so*; do patchelf --set-rpath "\$ORIGIN" "$so"; done
for so in $(find "$app/usr/plugins" -name "*.so"); do patchelf --set-rpath "\$ORIGIN/../../lib" "$so"; done
echo "Bundled:"; ls "$app/usr/lib"
echo "Needs from the system:"; readelf -d "$app/usr/bin/NinjaGaidenBlack" | sed -n "s/.*NEEDED.*\[\(.*\)\]/  \1/p"
'

app=$out/AppDir
# Pipelines recorded while testing, built in the background at startup so a
# first run does not stall on them (see NV2A_SHADER_SEED in src/main.c). The
# game's vertex programs are not in it: the renderer reads them from the
# player's default.xbe (tools/strip_shader_seed.py).
# The Direct3D renderer's seed (src/d3d/vk/backend.cpp, the shader cache): its shaders and pipelines
# are made in the background at the first start; vertex programs are places in the player's XBE.
mkdir -p "$app/usr/share/NinjaGaidenBlack"
cp "$here/../d3d-seed.bin" "$app/usr/share/NinjaGaidenBlack/d3d-seed.bin"
mkdir -p "$app/usr/share/NinjaGaidenBlack/shader-seed"
cp "$here/../shader-seed/states.bin" "$here/../shader-seed/pipelines.bin" \
   "$app/usr/share/NinjaGaidenBlack/shader-seed/"
# The fonts built into the program (SIL Open Font License): their licences.
mkdir -p "$app/usr/share/licenses/NinjaGaidenBlack"
cp "$repo"/assets/fonts/OFL-*.txt "$app/usr/share/licenses/NinjaGaidenBlack/"
cp "$repo/LICENSE" "$repo/THIRD_PARTY.md" "$app/usr/share/licenses/NinjaGaidenBlack/"
cp "$repo/src/d3d/NOTICE" "$app/usr/share/licenses/NinjaGaidenBlack/NOTICE-renderer.txt"
cp "$repo/src/LICENSE-Pillow.txt" "$app/usr/share/licenses/NinjaGaidenBlack/"
cp "$toolkit/LICENSE" "$app/usr/share/licenses/NinjaGaidenBlack/LICENSE-xboxrecomp.txt"
cp "$toolkit/NOTICE" "$app/usr/share/licenses/NinjaGaidenBlack/NOTICE-xboxrecomp.txt"
cp "$toolkit/LICENSES/LGPL-2.1.txt" "$app/usr/share/licenses/NinjaGaidenBlack/"
cp "$here/ngb.png" "$app/ninja-gaiden-black.png"
cp "$here/ngb.png" "$app/.DirIcon"
cat > "$app/ninja-gaiden-black.desktop" <<'DESKTOP'
[Desktop Entry]
Type=Application
Name=Ninja Gaiden Black
GenericName=Ninja Gaiden Black (static recompilation)
Comment=Ninja Gaiden Black, recompiled for Linux with Vulkan; asks for your own disc image
Exec=NinjaGaidenBlack %f
Icon=ninja-gaiden-black
Categories=Game;ActionGame;
Terminal=false
StartupWMClass=ninja-gaiden-black
DESKTOP
cat > "$app/AppRun" <<'APPRUN'
#!/bin/sh
# The program finds its bundled libraries through its rpath, so the tools it
# starts (the file dialog, bsdtar) keep the system's.
here="$(dirname "$(readlink -f "$0")")"
exec "$here/usr/bin/NinjaGaidenBlack" "$@"
APPRUN
chmod +x "$app/AppRun"

tool=$here/.cache/appimagetool-x86_64.AppImage
if [ ! -x "$tool" ]; then
    mkdir -p "$here/.cache"
    wget -qO "$tool" https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage
    chmod +x "$tool"
fi
# Update information for AppImageUpdate and the like (#115): the newest release on GitHub, pre-releases
# included, by the .zsync made beside the AppImage (zsyncmake, in the current folder), which goes up with each release
command -v zsyncmake >/dev/null || { echo "zsyncmake is needed for the .zsync file (zsync package)"; exit 1; }
(cd "$out" && ARCH=x86_64 "$tool" --appimage-extract-and-run --no-appstream \
    -u "gh-releases-zsync|gamboozino|ngbrecomp|latest-all|NinjaGaidenBlack-x86_64.AppImage.zsync" "$app" \
    "$out/NinjaGaidenBlack-x86_64.AppImage" >"$out/appimagetool.log" 2>&1) ||
    { tail -20 "$out/appimagetool.log"; exit 1; }
ls -la "$out/NinjaGaidenBlack-x86_64.AppImage" "$out/NinjaGaidenBlack-x86_64.AppImage.zsync"
