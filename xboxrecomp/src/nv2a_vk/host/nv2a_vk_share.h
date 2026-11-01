/*
 * The NV2A renderer's Vulkan device, shared with another renderer in the
 * same process: ngb-recomp's Direct3D renderer draws on it and hands the
 * presenter its frames, so the window, the overlay and the settings stay
 * the same whichever renderer draws.
 */
#ifndef NV2A_VK_SHARE_H
#define NV2A_VK_SHARE_H
#include <stdbool.h>
#include <stdint.h>
#include <volk.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct nv2a_vk_shared {
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    uint32_t queue_family;
    VkQueue queue;
    /* Vulkan 1.3's dynamic rendering and synchronization2 on (core, or their
     * extensions on an older driver: the core names' functions are then the
     * extensions'): what the Direct3D renderer needs. */
    bool features13;
    /* VK_KHR_push_descriptor on (without it, the renderer binds a descriptor
     * set per draw: the Mali drivers of many MediaTek phones). */
    bool push_descriptor;
    uint32_t api_version;   /* the version the device was made for (VMA's) */
} nv2a_vk_shared;

/* 1 and filled in once the device exists, 0 before, -1 when there is none
 * (headless, or the renderer failed). */
int nv2a_vk_share(nv2a_vk_shared *out);

/* Around every queue submission or wait, on any thread: the renderer's own
 * are on the PFIFO thread, a sharer's on its own. */
void nv2a_vk_queue_lock(void);
void nv2a_vk_queue_unlock(void);

/* While set, the renderer executes the pushbuffer without drawing: no
 * draws, clears, blits or surface traffic (the sharer draws instead). */
void nv2a_vk_set_draws_off(bool off);
bool nv2a_vk_draws_off(void);

/* The quality settings in force (nv2a_window.h's nv2a_set_quality, else the
 * renderer's defaults): render scale 1-8, smooth texture filtering, shadow
 * map scale 1-8. */
void nv2a_quality_get(int *render_scale, int *anisotropy, int *shadow_scale, int *msaa);

/* The picture the presenter shows in place of the renderer's display image:
 * a 2D BGRA colour image in TRANSFER_SRC_OPTIMAL, rows top down, left valid
 * until the next call. VK_NULL_HANDLE goes back to the renderer's picture. */
void nv2a_present_set_external(VkImage image, uint32_t width, uint32_t height);

#ifdef __cplusplus
}
#endif
#endif
