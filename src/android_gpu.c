/*
 * Android: which Vulkan driver the game draws with. The phone's own, or Turnip -- Mesa's open driver for Qualcomm's
 * Adreno GPUs, built from Mesa's source and shipped in the APK (packaging/android/build.sh) -- loaded through
 * libadrenotools, which has the system's Vulkan loader load it for this app alone. Many Adreno 6xx phones' own
 * drivers stop at Vulkan 1.1 without the dynamic rendering, synchronization2 and push descriptors the renderer
 * needs (#87); Turnip has them.
 *
 * Turnip only on an Adreno whose own driver lacks those -- found by a look at the system's driver first, an
 * instance made and let go (NGB_GPU_DRIVER=system|turnip forces one, for testing). There is no setting: a choice of
 * the system's driver on such a phone kept the game from starting, the menu to change it back with included (#151). Turnip finding no GPU it knows (an Adreno too new or too old for it) falls back to the
 * system's. The GPU emulation asks here before it starts Vulkan (xboxrecomp's instance.c, nv2a_vk_custom_loader).
 */
#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#include <adrenotools/driver.h>


#define TURNIP_LIB "libvulkan_freedreno.so"

/* The folder this library was loaded from: the app's native libraries (libadrenotools' hooks and Turnip) */
static int native_dir(char *out, size_t n)
{
    Dl_info info;
    if (!dladdr((void *)native_dir, &info) || !info.dli_fname)
        return 0;
    snprintf(out, n, "%s", info.dli_fname);
    char *slash = strrchr(out, '/');
    if (!slash)
        return 0;
    slash[1] = 0;
    return 1;
}

/* A device the renderer can use through gipa: 1 one it can; 0 none (or no instance or device at all: *made 0);
 * and *qualcomm when the first is an Adreno. The extensions listed and their features on (as instance.c turns
 * them on): a driver may list them and not have them. */
static int usable_device(PFN_vkGetInstanceProcAddr gipa, int *qualcomm, int *made)
{
    if (made)
        *made = 0;
    PFN_vkCreateInstance create = (PFN_vkCreateInstance)gipa(NULL, "vkCreateInstance");
    if (!create)
        return 0;
    VkApplicationInfo app = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, .apiVersion = VK_API_VERSION_1_1 };
    VkInstanceCreateInfo ci = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo = &app };
    VkInstance inst;
    if (create(&ci, NULL, &inst) != VK_SUCCESS)
        return 0;
    PFN_vkDestroyInstance destroy = (PFN_vkDestroyInstance)gipa(inst, "vkDestroyInstance");
    PFN_vkEnumeratePhysicalDevices devices =
        (PFN_vkEnumeratePhysicalDevices)gipa(inst, "vkEnumeratePhysicalDevices");
    PFN_vkGetPhysicalDeviceProperties props =
        (PFN_vkGetPhysicalDeviceProperties)gipa(inst, "vkGetPhysicalDeviceProperties");
    PFN_vkEnumerateDeviceExtensionProperties exts =
        (PFN_vkEnumerateDeviceExtensionProperties)gipa(inst, "vkEnumerateDeviceExtensionProperties");
    PFN_vkGetPhysicalDeviceFeatures2 features =
        (PFN_vkGetPhysicalDeviceFeatures2)gipa(inst, "vkGetPhysicalDeviceFeatures2");
    int ok = 0;
    uint32_t n = 1;
    VkPhysicalDevice dev;
    if (devices && props && exts && devices(inst, &n, &dev) >= 0 && n >= 1) {
        VkPhysicalDeviceProperties p;
        props(dev, &p);
        if (made)
            *made = 1;
        if (qualcomm)
            *qualcomm = p.vendorID == 0x5143;
        uint32_t k = 0;
        exts(dev, NULL, &k, NULL);
        VkExtensionProperties *e = calloc(k ? k : 1, sizeof *e);
        int dyn = p.apiVersion >= VK_API_VERSION_1_3, sync2 = dyn, push = 0;
        if (e && exts(dev, NULL, &k, e) >= 0)
            for (uint32_t i = 0; i < k; i++) {
                dyn |= !strcmp(e[i].extensionName, "VK_KHR_dynamic_rendering");
                sync2 |= !strcmp(e[i].extensionName, "VK_KHR_synchronization2");
                push |= !strcmp(e[i].extensionName, "VK_KHR_push_descriptor");
            }
        free(e);
        if (dyn && sync2 && features) {   /* (on, not only listed) */
            VkPhysicalDeviceDynamicRenderingFeaturesKHR d = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR };
            VkPhysicalDeviceSynchronization2FeaturesKHR s = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR, .pNext = &d };
            VkPhysicalDeviceFeatures2 f2 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &s };
            features(dev, &f2);
            dyn = d.dynamicRendering;
            sync2 = s.synchronization2;
        }
        ok = dyn && sync2 && push;
        fprintf(stderr, "[GPU] %s: Vulkan %u.%u, %s\n", p.deviceName, VK_API_VERSION_MAJOR(p.apiVersion),
                VK_API_VERSION_MINOR(p.apiVersion), ok ? "has what the renderer needs" : "lacks what the renderer needs");
    }
    if (destroy)
        destroy(inst, NULL);
    return ok;
}

PFN_vkGetInstanceProcAddr nv2a_vk_custom_loader(void)
{
    int mode = 0;   /* 0 Auto, 1 System, 2 Turnip */
    const char *env = getenv("NGB_GPU_DRIVER");
    if (env && !strcmp(env, "system"))
        mode = 1;
    else if (env && !strcmp(env, "turnip"))
        mode = 2;
    if (mode == 1)
        return NULL;
    if (mode == 0) {   /* Auto: the system's driver first */
        void *sys = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
        PFN_vkGetInstanceProcAddr gipa = sys ? (PFN_vkGetInstanceProcAddr)dlsym(sys, "vkGetInstanceProcAddr") : NULL;
        int qualcomm = 0, made = 0, ok = gipa ? usable_device(gipa, &qualcomm, &made) : 0;
        /* (no device from the system's at all: an Adreno's broken driver as likely as not -- Turnip tried, it
         * falls back by itself; an Adreno has its kernel driver's node) */
        if (!made)
            qualcomm = access("/dev/kgsl-3d0", F_OK) == 0;
        if (ok || !qualcomm) {
            fprintf(stderr, "[GPU] the system's driver%s\n", ok ? "" : " (not an Adreno: Turnip cannot help)");
            return NULL;
        }
    }
    char dir[1024];
    if (!native_dir(dir, sizeof dir))
        return NULL;
    void *vk = adrenotools_open_libvulkan(RTLD_NOW, ADRENOTOOLS_DRIVER_CUSTOM, NULL, dir, dir, TURNIP_LIB, NULL, NULL);
    PFN_vkGetInstanceProcAddr gipa = vk ? (PFN_vkGetInstanceProcAddr)dlsym(vk, "vkGetInstanceProcAddr") : NULL;
    if (!gipa || !usable_device(gipa, NULL, NULL)) {
        fprintf(stderr, "[GPU] Turnip could not be used here: the system's driver\n");
        return NULL;
    }
    fprintf(stderr, "[GPU] Turnip (Mesa's driver for Adreno)\n");
    return gipa;
}
