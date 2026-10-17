#!/usr/bin/env bash
# Native Apple Silicon app, with the first-run installer and no game data.
# Generate src/recomp/gen with tools/pipeline.sh before building.
set -euo pipefail
here=$(cd "$(dirname "$0")" && pwd)
repo=$(cd "$here/../.." && pwd)
out=$(mkdir -p "${1:-$repo/build/macos-release}" && cd "${1:-$repo/build/macos-release}" && pwd)
prefix=${NGB_MAC_PREFIX:-$(brew --prefix)}
cmake -S "$repo" -B "$out/build" -G Ninja \
    -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_PREFIX_PATH="$prefix" -DCMAKE_BUILD_TYPE=Release \
    -DXBOXRECOMP_DIR="${XBOXRECOMP_DIR:-$repo/xboxrecomp}" >"$out/configure.log"
cmake --build "$out/build" -j"${JOBS:-$(sysctl -n hw.ncpu)}" >"$out/build.log"
app="$out/Ninja Gaiden Black.app"
rm -rf "$app"
python3 "$here/bundle.py" "$out/build" "$app" --prefix "$prefix" \
    --toolkit "${XBOXRECOMP_DIR:-$repo/xboxrecomp}"
rm -f "$out/NinjaGaidenBlack-macos-arm64.zip"
ditto -c -k --sequesterRsrc --keepParent "$app" "$out/NinjaGaidenBlack-macos-arm64.zip"
echo "Packed: $out/NinjaGaidenBlack-macos-arm64.zip"
