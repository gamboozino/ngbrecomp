#!/usr/bin/env python3
"""Check that the shipped shader seed still matches what the renderer asks for.

    check_shader_seed.py <d3d-seed.bin> <a test run's d3d cache dir>

The seed lists the renderer's shaders by their keys (src/d3d/vk/backend.cpp, ShaderCache). A change to how a
key is made -- a field added to FixedKey or PixelKey, a new hash -- leaves every record in the seed unused:
the prewarm thread still makes them all, and a first start compiles everything as it is drawn (the seed of
10-15 matched nothing by 10-31). Run the game once with an empty NV2A_SHADER_CACHE and NGB_D3D_SEED=/dev/null,
then point this at its d3d folder: the share of the run's shaders the seed holds is printed for each kind, and
the exit status is 1 when the seed has none of them.
"""
import struct
import sys


def records(path):
    data = open(path, 'rb').read()
    if data[:8] != b'NGBD3DC1':
        sys.exit('%s: not a shader cache file' % path)
    out, i = [], 8
    while i + 13 <= len(data):
        kind = chr(data[i])
        key = struct.unpack_from('<Q', data, i + 1)[0]
        n = struct.unpack_from('<I', data, i + 9)[0]
        if i + 13 + n > len(data):
            break
        out.append((kind, key, data[i + 13:i + 13 + n]))
        i += 13 + n
    return out


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    seed = records(sys.argv[1])
    run = records(sys.argv[2] + '/recipes.bin')
    vertex = {'V', 'W', 'F'}     # a vertex program (whole, or at an XBE offset) or fixed function: one key space
    # (not the pipelines: the prewarm thread copies the seed's into the run's recipes.bin, so they all look used;
    # a pipeline is made of two of these shaders, and goes stale with them)
    names = {'F': 'vertex', 'P': 'pixel'}
    total_hit = 0
    for kind in ('F', 'P'):
        kinds = vertex if kind == 'F' else {kind}
        have = {key for k, key, _ in seed if k in kinds}
        want = {key for k, key, _ in run if k in kinds}
        hit = len(have & want)
        total_hit += hit
        share = 100.0 * hit / len(want) if want else 100.0
        print('%-6s shaders: the run used %4d, %4d of them in the seed (%.0f%%); the seed holds %d'
              % (names[kind], len(want), hit, share, len(have)))
    if not total_hit:
        print('the seed matches nothing this run used: record it again (docs/renderer.md)')
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
