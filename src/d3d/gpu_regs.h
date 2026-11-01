/*
 * Guest memory reads and writes that may land on a device's registers -- the GPU's (the NV2A aperture at 0xFD000000,
 * 16 MB) or the USB controllers' (0xFED00000 and 0xFED08000, 4 KB each): there, the device's model is called
 * directly; anywhere else, plain guest memory. A register access through MEM32
 * faults and is emulated in the signal handler -- a trip through the kernel each, a few microseconds on a desktop,
 * some 20 on a phone, and D3D's push buffer kickoff, the GPU's interrupt handler and the pads' made thousands a second. For
 * the few hot places that touch the registers through pointers (src/d3d/gpu_regs.c, d3d_orig.c's kickoff);
 * NGB_FAULT_COUNT=1 lists where the rest come from.
 */
#pragma once
#include <stdint.h>

#ifdef NGB_NV2A_VK
int nv2a_reg_live(void);                             /* xboxrecomp/src/nv2a_vk/host/nv2a_runtime.c */
uint32_t nv2a_reg_read32(uint32_t off);
void nv2a_reg_write32(uint32_t off, uint32_t val);
#define GPU_REGS_LIVE() nv2a_reg_live()
#else
#define GPU_REGS_LIVE() 0   /* (no GPU model in the build: the aperture is plain memory, as it always was here) */
#define nv2a_reg_read32(off) 0u
#define nv2a_reg_write32(off, val) ((void)0)
#endif
int xbox_OhciRead32(uint32_t xbox_va, uint32_t *val);   /* xboxrecomp/src/usb/ohci.c */
int xbox_OhciWrite32(uint32_t xbox_va, uint32_t val);

#define GPU_REGS_BASE 0xFD000000u
#define GPU_REGS_SIZE 0x01000000u

#define USB_REGS_BASE 0xFED00000u
#define USB_REGS_SIZE 0x00010000u   /* (both controllers, and the gap: xbox_Ohci* says which is one) */

static inline uint32_t GR32(uint32_t a)
{
    uint32_t v;
    if (a - GPU_REGS_BASE < GPU_REGS_SIZE && GPU_REGS_LIVE())
        return nv2a_reg_read32(a - GPU_REGS_BASE);
    if (a - USB_REGS_BASE < USB_REGS_SIZE && xbox_OhciRead32(a, &v))
        return v;
    return MEM32(a);
}

static inline void GW32(uint32_t a, uint32_t v)
{
    if (a - GPU_REGS_BASE < GPU_REGS_SIZE && GPU_REGS_LIVE())
        nv2a_reg_write32(a - GPU_REGS_BASE, v);
    else if (!(a - USB_REGS_BASE < USB_REGS_SIZE && xbox_OhciWrite32(a, v)))
        MEM32(a) = v;
}
