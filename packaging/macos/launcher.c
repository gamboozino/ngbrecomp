/* Finder entry point: paths to the bundled libraries and shader seeds. */
#include <mach-o/dyld.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int main(int argc, char **argv)
{
    char exe[PATH_MAX], macos[PATH_MAX], resources[PATH_MAX], path[PATH_MAX];
    uint32_t size = sizeof exe;
    if (_NSGetExecutablePath(exe, &size) || !realpath(exe, macos))
        return 1;
    char *slash = strrchr(macos, '/');
    if (!slash)
        return 1;
    *slash = 0;
    snprintf(path, sizeof path, "%s/../Resources", macos);
    if (!realpath(path, resources))
        return 1;
    snprintf(path, sizeof path, "%s/share/NinjaGaidenBlack/d3d-seed.bin", resources);
    setenv("NGB_D3D_SEED", path, 0);
    snprintf(path, sizeof path, "%s/share/NinjaGaidenBlack/shader-seed", resources);
    setenv("NV2A_SHADER_SEED", path, 0);
    snprintf(path, sizeof path, "%s/vulkan/icd.d/MoltenVK_icd.json", resources);
    setenv("VK_DRIVER_FILES", path, 0);
    /* SDL and volk must use the same Vulkan loader. */
    snprintf(path, sizeof path, "%s/../Frameworks/libvulkan.1.dylib", macos);
    setenv("SDL_VULKAN_LIBRARY", path, 0);
    snprintf(exe, sizeof exe, "%s/NinjaGaidenBlack.bin", macos);
    argv[0] = exe;
    execv(exe, argv);
    perror("Cannot launch Ninja Gaiden Black");
    return 1;
}
