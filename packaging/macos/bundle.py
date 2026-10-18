#!/usr/bin/env python3
"""Bundle an Apple Silicon build and its runtime dependencies, without game data."""
import argparse
import json
import plistlib
import re
import shutil
import subprocess
import tempfile
from pathlib import Path


def run(*args):
    return subprocess.check_output([str(a) for a in args], text=True)


def dependencies(path):
    return [line.strip().split(' (compatibility')[0]
            for line in run('otool', '-L', path).splitlines()[1:]]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('build', type=Path)
    ap.add_argument('app', type=Path)
    ap.add_argument('--prefix', type=Path, required=True)
    ap.add_argument('--toolkit', type=Path)
    args = ap.parse_args()
    args.build = args.build.resolve()
    args.app = args.app.resolve()
    args.prefix = args.prefix.resolve()
    if args.app.exists():
        ap.error('output already exists; choose a new app path')
    root = Path(__file__).resolve().parents[2]
    toolkit = args.toolkit or root / 'xboxrecomp'
    macos = args.app / 'Contents/MacOS'
    libs = args.app / 'Contents/Frameworks'
    resources = args.app / 'Contents/Resources'
    for directory in (macos, libs, resources):
        directory.mkdir(parents=True)
    binary = macos / 'NinjaGaidenBlack.bin'
    shutil.copy2(args.build / 'NinjaGaidenBlack', binary)
    shutil.copy2(args.build / 'ngb-setup', macos / 'ngb-setup')
    bundled = {}
    queue = [binary]
    # volk dlopens the loader; the loader discovers MoltenVK through our ICD.
    dynamic = ['libvulkan.1.dylib', 'libMoltenVK.dylib']
    if 'sdl2-compat' in str((args.prefix / 'lib/libSDL2.dylib').resolve()):
        dynamic.append('libSDL3.0.dylib')
    for name in dynamic:
        source = args.prefix / 'lib' / name
        shutil.copy2(source.resolve(), libs / name)
        bundled[name] = source
        queue.append(libs / name)
    processed = set()
    while queue:
        target = queue.pop(0)
        if target in processed:
            continue
        processed.add(target)
        for dep in dependencies(target):
            if dep.startswith(('/usr/lib/', '/System/Library/')):
                continue
            if dep.startswith('@loader_path/'):
                source = bundled.get(target.name, target).parent / dep[len('@loader_path/'):]
            elif dep.startswith('@rpath/'):
                source = args.prefix / 'lib' / dep[len('@rpath/'):]
            else:
                source = Path(dep)
            name = source.name
            if name == target.name:
                continue  # a dylib's own install ID
            if name not in bundled:
                shutil.copy2(source.resolve(), libs / name)
                bundled[name] = source
                queue.append(libs / name)
            run('install_name_tool', '-change', dep, '@rpath/' + name, target)
    for lib in libs.iterdir():
        run('install_name_tool', '-id', '@rpath/' + lib.name, lib)
        run('install_name_tool', '-add_rpath', '@loader_path', lib)
    if 'libSDL3.0.dylib' in bundled:
        (libs / 'libSDL3.dylib').symlink_to('libSDL3.0.dylib')
    (libs / 'libvulkan.dylib').symlink_to('libvulkan.1.dylib')
    run('install_name_tool', '-add_rpath', '@executable_path/../Frameworks', binary)
    run('clang', '-arch', 'arm64', '-O2', root / 'packaging/macos/launcher.c', '-o', macos / 'NinjaGaidenBlack')
    icd = resources / 'vulkan/icd.d'
    icd.mkdir(parents=True)
    (icd / 'MoltenVK_icd.json').write_text(json.dumps({
        'file_format_version': '1.0.0',
        'ICD': {'library_path': '../../../Frameworks/libMoltenVK.dylib',
                'api_version': '1.3.0', 'is_portability_driver': True}}, indent=2))
    with tempfile.TemporaryDirectory() as temp:
        iconset = Path(temp) / 'ngb.iconset'
        iconset.mkdir()
        for size in (16, 32, 128, 256, 512):
            for scale in (1, 2):
                suffix = '@2x' if scale == 2 else ''
                run('sips', '-z', str(size * scale), str(size * scale),
                    root / 'packaging/appimage/ngb.png', '--out',
                    iconset / f'icon_{size}x{size}{suffix}.png')
        run('iconutil', '-c', 'icns', iconset, '-o', resources / 'ngb.icns')
    (args.app / 'Contents/Info.plist').write_bytes(plistlib.dumps({
        'CFBundleExecutable': 'NinjaGaidenBlack', 'CFBundlePackageType': 'APPL'}))
    run(args.prefix / 'bin/macdeployqt', args.app,
        '-executable=' + str(macos / 'ngb-setup'), '-no-strip', '-no-codesign')
    minimum = (0, 0)
    for path in args.app.rglob('*'):
        if path.is_file() and not path.is_symlink():
            if 'Mach-O' not in run('file', '-b', path):
                continue
            commands = run('otool', '-l', path)
            for rpath in re.findall(r'cmd LC_RPATH\n\s+cmdsize \d+\n\s+path (.+) \(offset', commands):
                if rpath.startswith(str(args.prefix) + '/'):
                    run('install_name_tool', '-delete_rpath', rpath, path)
            for dep in dependencies(path):
                if (Path(dep).name != path.name and dep.startswith('/')
                        and not dep.startswith(('/usr/lib/', '/System/Library/'))):
                    raise RuntimeError(f'Unbundled dependency in {path}: {dep}')
            versions = re.findall(r'minos ([0-9.]+)', run('vtool', '-show-build', path))
            for version in versions:
                minimum = max(minimum, tuple(map(int, version.split('.'))))
    version = re.search(r'VALUE "ProductVersion", "([^"]+)"',
                        (root / 'packaging/windows/ngb.rc').read_text()).group(1)
    info = dict(CFBundleExecutable='NinjaGaidenBlack', CFBundleName='Ninja Gaiden Black',
                CFBundleDisplayName='Ninja Gaiden Black', CFBundleIconFile='ngb.icns', CFBundleIdentifier='io.github.gamboozino.ngbrecomp.macos',
                CFBundlePackageType='APPL', CFBundleVersion=version, CFBundleShortVersionString=version,
                LSMinimumSystemVersion='.'.join(map(str, minimum)), NSHighResolutionCapable=True)
    (args.app / 'Contents/Info.plist').write_bytes(plistlib.dumps(info))
    share = resources / 'share/NinjaGaidenBlack'
    (share / 'shader-seed').mkdir(parents=True)
    shutil.copy2(root / 'packaging/d3d-seed.bin', share)
    for name in ('states.bin', 'pipelines.bin'):
        shutil.copy2(root / 'packaging/shader-seed' / name, share / 'shader-seed')
    licenses = resources / 'licenses'
    licenses.mkdir()
    for source in [*root.glob('assets/fonts/OFL-*.txt'),
                   root / 'assets/prompts/License-Xelu.txt',
                   root / 'LICENSE', root / 'THIRD_PARTY.md',
                   toolkit / 'LICENSES/LGPL-2.1.txt']:
        shutil.copy2(source, licenses)
    for source, name in ((root / 'src/d3d/NOTICE', 'NOTICE-renderer.txt'),
                         (toolkit / 'LICENSE', 'LICENSE-xboxrecomp.txt'),
                         (toolkit / 'NOTICE', 'NOTICE-xboxrecomp.txt')):
        shutil.copy2(source, licenses / name)
    run('codesign', '--force', '--deep', '--sign', '-', args.app)
    run('codesign', '--verify', '--deep', '--strict', args.app)
    print(args.app.resolve())


if __name__ == '__main__':
    main()
