#!/usr/bin/env bash
# Ninja Gaiden Black (static recompilation) for Android (arm64), cross-compiled on Linux. Work in progress.
#
#   packaging/android/build.sh [out-dir]      (default build/android-release)
#
# Builds SDL2, glslang, libadrenotools and Turnip (Mesa's Vulkan driver for Adreno GPUs, from Mesa's source:
# src/android_gpu.c) with the NDK into build/android-deps (or NGB_ANDROID_DEPS) once, then the game as
# libmain.so, the library SDL's Java activity loads, and packs both with that activity into
# <out-dir>/NinjaGaidenBlack-android-arm64.apk, signed with a debug key (build/android-deps/debug.keystore).
# Like the other packages it holds no game data: the game files go into the app's folder on the phone
# (src/android_main.c). GLib is not used there: the GPU model takes the slice of it it needs from xboxrecomp's
# shim/glib-lite.
#
# Needs: the Android SDK (ANDROID_HOME, default ~/Android/Sdk) with an NDK, build-tools and a platform, a JDK,
# cmake, ninja, curl, git, python3 (with venv: Mesa's build tools go into one there), bison, flex. The
# recompiled code must already be generated (tools/pipeline.sh).
set -euo pipefail
trap 'echo "Failed: see the logs in ${out:-build/android-release} and ${deps:-build/android-deps}." >&2' ERR

here=$(cd "$(dirname "$0")" && pwd)
repo=$(cd "$here/../.." && pwd)
deps=$(mkdir -p "${NGB_ANDROID_DEPS:-$repo/build/android-deps}" && cd "${NGB_ANDROID_DEPS:-$repo/build/android-deps}" && pwd)
out=$(mkdir -p "${1:-$repo/build/android-release}" && cd "${1:-$repo/build/android-release}" && pwd)
sdk=${ANDROID_HOME:-$HOME/Android/Sdk}
ndk=${ANDROID_NDK_HOME:-$(ls -d "$sdk"/ndk/* 2>/dev/null | sort -V | tail -1 || true)}
tools=$(ls -d "$sdk"/build-tools/* 2>/dev/null | sort -V | tail -1 || true)
platform=$(ls -d "$sdk"/platforms/android-* 2>/dev/null | sort -V | tail -1 || true)
abi=arm64-v8a
api=29   # thread-local guest registers need the ELF TLS that Android 10 brought
sdl=2.32.10
glslang=16.0.0
mesa=26.2.6
adrenotools=8fae8ce   # (github.com/bylaws/libadrenotools)

[ -f "$ndk/build/cmake/android.toolchain.cmake" ] || { echo "No Android NDK: set ANDROID_NDK_HOME." >&2; exit 1; }
[ -x "$tools/aapt2" ] && [ -f "$platform/android.jar" ] ||
    { echo "No Android build-tools or platform in $sdk: set ANDROID_HOME." >&2; exit 1; }
[ -n "$(ls "$repo"/src/recomp/gen/*.c 2>/dev/null)" ] ||
    { echo "No generated code: run tools/pipeline.sh first." >&2; exit 1; }

android_cmake() {   # source-dir build-dir [cmake args...]
    local src=$1 bld=$2; shift 2
    cmake -S "$src" -B "$bld" -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE="$ndk/build/cmake/android.toolchain.cmake" \
        -DANDROID_ABI=$abi -DANDROID_PLATFORM=android-$api -DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON "$@"
}

# The dependencies, built once into $deps/$abi
prefix=$deps/$abi
mkdir -p "$deps/src"
# (built again when it is not aligned for 16 KB pages, as Android 15's devices may have)
if [ ! -f "$prefix/lib/libSDL2.so" ] ||
   ! "$ndk"/toolchains/llvm/prebuilt/*/bin/llvm-readelf -lW "$prefix/lib/libSDL2.so" | grep -q 'LOAD.*0x4000$'; then
    [ -d "$deps/src/SDL" ] || { curl -fL "https://github.com/libsdl-org/SDL/releases/download/release-$sdl/SDL2-$sdl.tar.gz" |
        tar -xz -C "$deps/src" && mv "$deps/src/SDL2-$sdl" "$deps/src/SDL"; }
    android_cmake "$deps/src/SDL" "$deps/build-SDL-$abi" -DCMAKE_INSTALL_PREFIX="$prefix" \
        -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST=OFF >"$deps/build-SDL-$abi.log"
    cmake --build "$deps/build-SDL-$abi" --target install >>"$deps/build-SDL-$abi.log"
fi
# (with HLSL input: the Direct3D renderer's shaders are HLSL, src/d3d/vk/spirv.cpp)
if [ ! -f "$prefix/lib/libglslang.a" ] ||
   ! grep -q '^ENABLE_HLSL:BOOL=ON' "$deps/build-glslang-$abi/CMakeCache.txt" 2>/dev/null; then
    [ -d "$deps/src/glslang" ] || { curl -fL "https://github.com/KhronosGroup/glslang/archive/refs/tags/$glslang.tar.gz" |
        tar -xz -C "$deps/src" && mv "$deps/src/glslang-$glslang" "$deps/src/glslang"; }
    android_cmake "$deps/src/glslang" "$deps/build-glslang-$abi" -DCMAKE_INSTALL_PREFIX="$prefix" \
        -DBUILD_SHARED_LIBS=OFF -DENABLE_OPT=OFF -DENABLE_HLSL=ON -DGLSLANG_TESTS=OFF -DGLSLANG_ENABLE_INSTALL=ON \
        -DENABLE_GLSLANG_BINARIES=OFF >"$deps/build-glslang-$abi.log"
    cmake --build "$deps/build-glslang-$abi" --target install >>"$deps/build-glslang-$abi.log"
fi

# libadrenotools: the system's Vulkan loader made to load another driver for this app (again when a run stopped
# partway, or the pin moved: the stamp is written last)
if [ "$(cat "$prefix/adrenotools.stamp" 2>/dev/null)" != "$adrenotools" ] || [ ! -f "$prefix/lib/liblinkernsbypass.a" ]; then
    [ -d "$deps/src/libadrenotools" ] || git clone -q --recursive https://github.com/bylaws/libadrenotools.git \
        "$deps/src/libadrenotools"
    git -C "$deps/src/libadrenotools" checkout -q $adrenotools && git -C "$deps/src/libadrenotools" submodule update -q
    android_cmake "$deps/src/libadrenotools" "$deps/build-adrenotools-$abi" -DCMAKE_INSTALL_PREFIX="$prefix" \
        -DGEN_INSTALL_TARGET=ON >"$deps/build-adrenotools.log"
    cmake --build "$deps/build-adrenotools-$abi" --target install >>"$deps/build-adrenotools.log"
    cp "$deps/build-adrenotools-$abi/lib/linkernsbypass/liblinkernsbypass.a" "$prefix/lib/"
    echo "$adrenotools" > "$prefix/adrenotools.stamp"
fi
# Turnip: Mesa's Vulkan driver for Adreno, built from Mesa's release (a build folder of its own for each version;
# a run that stopped partway picks up where it was)
tb=$deps/build-turnip-$mesa-2   # (-2: with the two extensions below)
turnip=$tb/src/freedreno/vulkan/libvulkan_freedreno.so
if [ ! -f "$turnip" ]; then
    if [ ! -f "$deps/src/mesa-$mesa/meson.build" ]; then   # (unpacked whole, then put in place)
        curl -fL -o "$deps/mesa-$mesa.tar.xz" "https://archive.mesa3d.org/mesa-$mesa.tar.xz"
        rm -rf "${deps:?}/src/mesa-$mesa.part" "${deps:?}/src/mesa-$mesa"
        mkdir -p "$deps/src/mesa-$mesa.part"
        tar -xJf "$deps/mesa-$mesa.tar.xz" -C "$deps/src/mesa-$mesa.part"
        mv "$deps/src/mesa-$mesa.part/mesa-$mesa" "$deps/src/mesa-$mesa"
        rm -rf "${deps:?}/src/mesa-$mesa.part" "$deps/mesa-$mesa.tar.xz"
    fi
    # Mesa offers a KHR extension on Android only from the API level that took it in: dynamic rendering from 33,
    # synchronization2 from 31. Built for $api, Turnip left both out, and on a phone whose system Vulkan loader is
    # older than 1.3 the renderer had no other way to them (#155). Offered from $api here.
    sed -i -e "s/\"VK_KHR_dynamic_rendering\": 33,/\"VK_KHR_dynamic_rendering\": $api,/" \
        -e "s/\"VK_KHR_synchronization2\": 31,/\"VK_KHR_synchronization2\": $api,/" \
        "$deps/src/mesa-$mesa/src/vulkan/util/vk_extensions.py"
    [ -x "$deps/venv/bin/meson" ] || { python3 -m venv "$deps/venv" &&
        "$deps/venv/bin/pip" install -q meson==1.12.1 Mako==1.4.3 PyYAML==6.0.3 packaging==26.3; }
    bin=$(echo "$ndk"/toolchains/llvm/prebuilt/*/bin)
    cat > "$deps/android-$abi.cross" <<CROSS
[binaries]
ar = '$bin/llvm-ar'
c = ['$bin/aarch64-linux-android$api-clang']
cpp = ['$bin/aarch64-linux-android$api-clang++', '-fno-exceptions', '-fno-unwind-tables', '-fno-asynchronous-unwind-tables', '-static-libstdc++']
c_ld = 'lld'
cpp_ld = 'lld'
strip = '$bin/llvm-strip'
pkg-config = ['/usr/bin/false']

[built-in options]
c_link_args = ['-Wl,-z,max-page-size=16384']
cpp_link_args = ['-Wl,-z,max-page-size=16384', '-static-libstdc++']

[host_machine]
system = 'android'
cpu_family = 'aarch64'
cpu = 'armv8'
endian = 'little'
CROSS
    [ -f "$tb/build.ninja" ] || (cd "$deps/src/mesa-$mesa" && PATH="$deps/venv/bin:$PATH" meson setup "$tb" \
        --cross-file "$deps/android-$abi.cross" -Dbuildtype=release -Dplatforms=android -Dplatform-sdk-version=$api \
        -Dandroid-stub=true -Dgallium-drivers= -Dvulkan-drivers=freedreno -Dfreedreno-kmds=kgsl \
        -Dandroid-libbacktrace=disabled -Degl=disabled -Dgles1=disabled -Dgles2=disabled -Dopengl=false -Dglx=disabled \
        -Dllvm=disabled -Dxmlconfig=disabled -Dvalgrind=disabled -Dzstd=disabled -Db_lto=false \
        -Dcpp_args=-Wno-c++11-narrowing) >"$deps/build-turnip.log"
    PATH="$deps/venv/bin:$PATH" ninja -C "$tb" >>"$deps/build-turnip.log"
fi

# The game
android_cmake "$repo" "$out/build" -DCMAKE_FIND_ROOT_PATH="$prefix" -DCMAKE_PREFIX_PATH="$prefix" \
    -DXBOXRECOMP_DIR="$(cd "${XBOXRECOMP_DIR:-$repo/xboxrecomp}" && pwd)" >"$out/configure.log"
ninja -C "$out/build" -j"${JOBS:-$(nproc)}" ninja_gaiden_black >"$out/build.log"

# The APK: SDL's Java side and our activity, the manifest, the two libraries
apk=$out/apk
rm -rf "${apk:?}"
mkdir -p "$apk/classes" "$apk/dex" "$apk/root/lib/$abi"
javac -source 8 -target 8 -nowarn -Xlint:none -cp "$platform/android.jar" -d "$apk/classes" \
    "$deps"/src/SDL/android-project/app/src/main/java/org/libsdl/app/*.java \
    "$here"/java/io/github/gamboozino/ngbrecomp/*.java 2>"$apk/javac.log"
"$tools/d8" --release --min-api $api --lib "$platform/android.jar" --output "$apk/dex" \
    $(find "$apk/classes" -name '*.class')
"$tools/aapt2" compile --dir "$here/res" -o "$apk/res.zip"   # (the icon: the AppImage's)
# the licences, in the APK's assets/licenses (as the other packages carry them)
lic=$apk/assets/licenses
mkdir -p "$lic"
toolkit=$(cd "${XBOXRECOMP_DIR:-$repo/xboxrecomp}" && pwd)
cp "$repo/LICENSE" "$repo/THIRD_PARTY.md" "$repo"/assets/fonts/OFL-*.txt "$repo/assets/prompts/License-Xelu.txt" \
   "$repo/assets/touch/License-Kenney.txt" "$lic/"
cp "$repo/src/d3d/NOTICE" "$lic/NOTICE-renderer.txt"
cp "$toolkit/LICENSE" "$lic/LICENSE-xboxrecomp.txt"
cp "$toolkit/NOTICE" "$lic/NOTICE-xboxrecomp.txt"
cp "$toolkit/LICENSES/LGPL-2.1.txt" "$lic/"
cp "$deps/src/SDL/LICENSE.txt" "$lic/LICENSE-SDL2.txt"
cp "$deps/src/glslang/LICENSE.txt" "$lic/LICENSE-glslang.txt"
cp "$deps/src/libadrenotools/LICENSE" "$lic/LICENSE-libadrenotools.txt"
cp "$deps/src/mesa-$mesa/docs/license.rst" "$lic/LICENSE-Mesa-Turnip.txt"
"$tools/aapt2" link -o "$apk/base.apk" -I "$platform/android.jar" --manifest "$here/AndroidManifest.xml" \
    -A "$apk/assets" "$apk/res.zip"
cp "$apk/dex/classes.dex" "$apk/root/"
ndkbin=$(echo "$ndk"/toolchains/llvm/prebuilt/*/bin)
"$ndkbin/llvm-strip" -o "$apk/root/lib/$abi/libmain.so" "$out/build/libmain.so"
cp "$prefix/lib/libSDL2.so" "$apk/root/lib/$abi/"
for lib in libhook_impl libmain_hook; do   # (libadrenotools' hooks: the two the driver loading uses)
    cp "$prefix/lib/$lib.so" "$apk/root/lib/$abi/"
done
"$ndkbin/llvm-strip" -o "$apk/root/lib/$abi/libvulkan_freedreno.so" "$turnip"
python3 - "$apk/base.apk" "$apk/root" <<'PY'
import os, sys, zipfile
with zipfile.ZipFile(sys.argv[1], 'a', zipfile.ZIP_DEFLATED) as z:
    for d, _, files in os.walk(sys.argv[2]):
        for f in files:
            path = os.path.join(d, f)
            z.write(path, os.path.relpath(path, sys.argv[2]))
PY
"$tools/zipalign" -f -p 16 "$apk/base.apk" "$apk/aligned.apk"
key=$deps/debug.keystore
[ -f "$key" ] || keytool -genkeypair -keystore "$key" -storepass android -keypass android -alias ngb \
    -keyalg RSA -keysize 2048 -validity 10000 -dname "CN=Ninja Gaiden Black debug" >/dev/null 2>&1
"$tools/apksigner" sign --ks "$key" --ks-pass pass:android --ks-key-alias ngb \
    --out "$out/NinjaGaidenBlack-android-arm64.apk" "$apk/aligned.apk"
ls -l "$out/NinjaGaidenBlack-android-arm64.apk"
