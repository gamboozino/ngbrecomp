/**
 * The guest CPU's registers: one thread-local struct, shared by the generated
 * code (templates/runtime/recomp_types.h) and the runtime.
 *
 * Each register used to be its own thread-local variable. In an executable
 * that is a fixed offset from the thread pointer, but in a shared library
 * loaded with dlopen -- how Android loads the game -- every thread-local
 * variable a function touches costs a call into the dynamic linker's TLS
 * resolver: 11% of the game thread's time on a Pixel 4a. One struct costs one
 * such call per function, whatever registers it uses.
 *
 * The old names stay as macros (the members are r_ named: the generated code
 * defines eax and the like as macros of its own), so the generated code and the runtime read
 * g_eax as before. Declare registers only through this header.
 */
#ifndef RECOMP_REGS_H
#define RECOMP_REGS_H

#include <stdint.h>

#ifndef RECOMP_TLS
#  if defined(_MSC_VER)
#    define RECOMP_TLS __declspec(thread)
#  elif defined(__GNUC__) || defined(__clang__)
#    define RECOMP_TLS __thread
#  else
#    define RECOMP_TLS _Thread_local
#  endif
#endif

#ifndef RECOMP_MMX_DEFINED
#define RECOMP_MMX_DEFINED
typedef union RecompMmx {
    int8_t   b[8];
    uint8_t  ub[8];
    int16_t  w[4];
    uint16_t uw[4];
    int32_t  d[2];
    uint32_t ud[2];
    uint64_t q;
} RecompMmx;
#endif

#ifndef RECOMP_XMM_DEFINED
#define RECOMP_XMM_DEFINED
typedef union RecompXmm {
    float    f[4];
    double   d[2];
    uint32_t u[4];
    int32_t  i[4];
    uint64_t q[2];
} RecompXmm;
#endif

typedef struct RecompRegs {
    uint32_t r_eax, r_ecx, r_edx, r_esp, r_ebx, r_esi, r_edi, r_ebp;
    uint32_t r_fs_base;            /* the fs: base (the thread's TIB) */
    uint32_t r_seh_ebp;
    int r_df;                      /* the direction flag */
    int r_fp_top, r_fp_cmp;          /* x87: stack top, last compare */
    uint16_t r_fp_control_word, r_fp_cc;
    double r_fp_stack[8];
    RecompXmm r_xmm0, r_xmm1, r_xmm2, r_xmm3, r_xmm4, r_xmm5, r_xmm6, r_xmm7;
    RecompMmx r_mm0, r_mm1, r_mm2, r_mm3, r_mm4, r_mm5, r_mm6, r_mm7;
} RecompRegs;

extern RECOMP_TLS RecompRegs g_regs;   /* (defined in xbox_memory_layout.c) */

/* Android arm64: even one thread-local access per function goes through the dynamic linker's resolver (11% of
 * the game thread on a Pixel 4a, a call per recompiled function called). The thread's own TLS_SLOT_APP, the
 * thread pointer's word 2, free for apps from API 29, holds a pointer to its registers instead: two loads. A
 * thread's first use fills it in (recomp_regs_slot, xbox_memory_layout.c). */
#if defined(__ANDROID__) && defined(__aarch64__)
/* (one asm statement, no memory read the compiler sees: computed once per function. Empty, the slot is filled by
 * recomp_regs_slot_asm, which keeps every register but x0 -- the asm's x16, x17 and x30 are its clobbers) */
static inline __attribute__((always_inline)) RecompRegs *recomp_regs_here(void)
{
    register RecompRegs *r __asm__("x0");
    __asm__("mrs x0, tpidr_el0\n\t"
            "ldr x0, [x0, #16]\n\t"
            "cbnz x0, 1f\n\t"
            "bl recomp_regs_slot_asm\n"
            "1:"
            : "=r"(r) : : "x16", "x17", "x30", "cc");
    return r;
}
#  define RECOMP_REGS (*recomp_regs_here())
#else
#  define RECOMP_REGS g_regs
#endif

#define g_eax (RECOMP_REGS.r_eax)
#define g_ecx (RECOMP_REGS.r_ecx)
#define g_edx (RECOMP_REGS.r_edx)
#define g_esp (RECOMP_REGS.r_esp)
#define g_ebx (RECOMP_REGS.r_ebx)
#define g_esi (RECOMP_REGS.r_esi)
#define g_edi (RECOMP_REGS.r_edi)
#define g_ebp (RECOMP_REGS.r_ebp)
#define g_fs_base (RECOMP_REGS.r_fs_base)
#define g_seh_ebp (RECOMP_REGS.r_seh_ebp)
#define g_df (RECOMP_REGS.r_df)
#define g_fp_top (RECOMP_REGS.r_fp_top)
#define g_fp_cmp (RECOMP_REGS.r_fp_cmp)
#define g_fp_control_word (RECOMP_REGS.r_fp_control_word)
#define g_fp_cc (RECOMP_REGS.r_fp_cc)
#define g_fp_stack (RECOMP_REGS.r_fp_stack)
#define g_xmm0 (RECOMP_REGS.r_xmm0)
#define g_xmm1 (RECOMP_REGS.r_xmm1)
#define g_xmm2 (RECOMP_REGS.r_xmm2)
#define g_xmm3 (RECOMP_REGS.r_xmm3)
#define g_xmm4 (RECOMP_REGS.r_xmm4)
#define g_xmm5 (RECOMP_REGS.r_xmm5)
#define g_xmm6 (RECOMP_REGS.r_xmm6)
#define g_xmm7 (RECOMP_REGS.r_xmm7)
#define g_mm0 (RECOMP_REGS.r_mm0)
#define g_mm1 (RECOMP_REGS.r_mm1)
#define g_mm2 (RECOMP_REGS.r_mm2)
#define g_mm3 (RECOMP_REGS.r_mm3)
#define g_mm4 (RECOMP_REGS.r_mm4)
#define g_mm5 (RECOMP_REGS.r_mm5)
#define g_mm6 (RECOMP_REGS.r_mm6)
#define g_mm7 (RECOMP_REGS.r_mm7)

#endif /* RECOMP_REGS_H */
