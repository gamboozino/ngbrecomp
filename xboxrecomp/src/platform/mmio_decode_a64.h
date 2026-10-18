/*
 * mmio_decode_a64.h -- the AArch64 twin of mmio_decode.h, for trapped MMIO on
 * POSIX hosts (macOS arm64, Linux arm64).
 *
 * The device page is PROT_NONE; the access arrives as SIGBUS (Darwin) or
 * SIGSEGV (Linux) with the faulting data address in si_addr and the thread
 * state in the ucontext. Unlike x86-64, the decoder never has to work out the
 * address: si_addr is exact for a data abort, so every addressing mode is
 * served by the same code. What it decodes from the instruction word is the
 * direction, the width, the destination/source register, sign extension and
 * any base-register writeback; the instruction is always 4 bytes.
 *
 * Covered: LDR/STR, LDRB/STRB, LDRH/STRH, LDRSB/LDRSH/LDRSW in the unsigned
 * immediate, unscaled (LDUR/STUR), pre/post-index and register-offset forms;
 * the SIMD&FP LDR/STR of B/H/S/D/Q; LDP/STP/LDPSW (int and S/D). Exclusive,
 * acquire/release, atomic and unprivileged forms return 0 -- stepping an
 * instruction that was not understood corrupts the guest silently, which is
 * worse than a fault naming the opcode.
 *
 * Clang emits exactly these for a `volatile` MEM32()/MEM16()/MEM8() in lifted
 * code (volatile accesses are never merged into pairs, so LDP/STP is a
 * backstop, not the common case).
 */
#ifndef MMIO_DECODE_A64_H
#define MMIO_DECODE_A64_H

#include <stdint.h>

#if defined(__aarch64__) && !defined(_WIN32)

#if defined(__APPLE__)
#include <sys/ucontext.h>
#if defined(__DARWIN_OPAQUE_ARM_THREAD_STATE64) && __DARWIN_OPAQUE_ARM_THREAD_STATE64
/* arm64e: pc/lr/sp/fp are ptrauth-signed opaque fields; use the
 * __darwin_arm_thread_state64_{get,set}_* accessors there. This runtime is
 * built arm64 (unsigned), so the plain fields are what the kernel reads back. */
#error "mmio_decode_a64.h: arm64e opaque thread state not handled"
#endif
#define A64_CTX_PC(uc)   ((uc)->uc_mcontext->__ss.__pc)
#define A64_CTX_X(uc, n) ((uc)->uc_mcontext->__ss.__x[(n)])
#define A64_CTX_FP(uc)   ((uc)->uc_mcontext->__ss.__fp)
#define A64_CTX_LR(uc)   ((uc)->uc_mcontext->__ss.__lr)
#define A64_CTX_SP(uc)   ((uc)->uc_mcontext->__ss.__sp)
#define A64_CTX_HAS_V 1
#define A64_CTX_V(uc, n) ((uc)->uc_mcontext->__ns.__v[(n)])
#elif defined(__linux__)
#include <ucontext.h>
#include <asm/sigcontext.h>      /* struct _aarch64_ctx, fpsimd_context */
#define A64_CTX_PC(uc)   ((uc)->uc_mcontext.pc)
#define A64_CTX_X(uc, n) ((uc)->uc_mcontext.regs[(n)])
#define A64_CTX_FP(uc)   ((uc)->uc_mcontext.regs[29])
#define A64_CTX_LR(uc)   ((uc)->uc_mcontext.regs[30])
#define A64_CTX_SP(uc)   ((uc)->uc_mcontext.sp)
/* The FP/SIMD file is a record in uc_mcontext.__reserved: a chain of
 * _aarch64_ctx headers (magic, size), the fpsimd_context among them and a
 * zero magic at the end. rt_sigreturn restores the V registers from it, so
 * writing there is how a load lands. Streaming-SVE state, which an SVE
 * record would supersede, does not arise in lifted code: clang's volatile
 * MEM32() accesses are plain LDR/STR. */
static inline __uint128_t *mmio_a64_linux_vregs(ucontext_t *uc)
{
    unsigned char *p = (unsigned char *)uc->uc_mcontext.__reserved;
    unsigned char *end = p + sizeof(uc->uc_mcontext.__reserved);
    while (p + sizeof(struct _aarch64_ctx) <= end) {
        struct _aarch64_ctx *h = (struct _aarch64_ctx *)p;
        if (h->magic == 0 || h->size < sizeof(*h))
            break;
        if (h->magic == FPSIMD_MAGIC) {
            if (p + sizeof(struct fpsimd_context) > end) return 0;
            return ((struct fpsimd_context *)p)->vregs;
        }
        p += h->size;
    }
    return 0;
}
#define A64_CTX_HAS_V 1
#define A64_CTX_V_OK(uc) (mmio_a64_linux_vregs(uc) != 0)
#define A64_CTX_V(uc, n) (mmio_a64_linux_vregs(uc)[(n)])
#endif
#ifndef A64_CTX_V_OK
#define A64_CTX_V_OK(uc) 1
#endif

typedef uint64_t (*mmio_a64_read_fn)(void *dev, uint32_t off, int size);
typedef void     (*mmio_a64_write_fn)(void *dev, uint32_t off, uint64_t val, int size);

/* x0..x28, then fp (x29), lr (x30); 31 is SP when used as a base. */
static inline uint64_t *mmio_a64_xreg(ucontext_t *uc, int r)
{
    if (r < 29) return (uint64_t *)&A64_CTX_X(uc, r);
    if (r == 29) return (uint64_t *)&A64_CTX_FP(uc);
    if (r == 30) return (uint64_t *)&A64_CTX_LR(uc);
    return (uint64_t *)&A64_CTX_SP(uc);
}

/* 1 if the instruction at pc was serviced against (rd, wr) and pc advanced.
 * off is the device offset of the faulting data address (from si_addr). */
static inline int mmio_emulate_a64(ucontext_t *uc, uint32_t off, void *dev,
                                   mmio_a64_read_fn rd, mmio_a64_write_fn wr)
{
    uint32_t insn = *(const uint32_t *)(uintptr_t)A64_CTX_PC(uc);
    uint32_t op29_27 = (insn >> 27) & 7;
    int V = (insn >> 26) & 1;

    if (!rd || !wr)
        return 0;
    if (V && !A64_CTX_V_OK(uc))                  /* no FP/SIMD record to edit */
        return 0;

    if (op29_27 == 7) {                          /* load/store register */
        int size = (insn >> 30) & 3;
        int opc  = (insn >> 22) & 3;
        int rt   = insn & 31, rn = (insn >> 5) & 31;
        int form = (insn >> 24) & 3;             /* 01 unsigned imm, 00 other */
        int bytes, is_load, sext64 = 0, sext32 = 0, wb = 0;
        int64_t wb_imm = 0;
        uint64_t v;

        if (form == 0) {
            int bit21 = (insn >> 21) & 1, sub = (insn >> 10) & 3;
            if (bit21 == 0) {
                /* 00 unscaled, 01 post-index, 10 unprivileged, 11 pre-index */
                if (sub == 2) return 0;
                if (sub == 1 || sub == 3) {
                    wb = 1;
                    wb_imm = (int64_t)(int32_t)(((insn >> 12) & 0x1FFu) << 23) >> 23;
                }
            } else if (sub != 2) {
                return 0;                        /* only register offset */
            }
        } else if (form != 1) {
            return 0;
        }

        if (V) {
#if A64_CTX_HAS_V
            if (opc & 2) { if (size != 0) return 0; bytes = 16; }
            else bytes = 1 << size;
            is_load = opc & 1;
            if (is_load) {
                if (bytes == 16) {
                    __uint128_t lo = rd(dev, off, 8), hi = rd(dev, off + 8, 8);
                    A64_CTX_V(uc, rt) = lo | (hi << 64);
                } else {
                    A64_CTX_V(uc, rt) = rd(dev, off, bytes);
                }
            } else {
                __uint128_t q = A64_CTX_V(uc, rt);
                if (bytes == 16) {
                    wr(dev, off, (uint64_t)q, 8);
                    wr(dev, off + 8, (uint64_t)(q >> 64), 8);
                } else {
                    wr(dev, off, (uint64_t)q, bytes);
                }
            }
#else
            return 0;
#endif
        } else {
            bytes = 1 << size;
            switch (opc) {
            case 0: is_load = 0; break;
            case 1: is_load = 1; break;
            case 2:                              /* LDRS* to 64-bit        */
                if (size == 3) return 0;         /* PRFM                    */
                is_load = 1; sext64 = 1; break;
            default:                             /* LDRS* to 32-bit (B, H) */
                if (size >= 2) return 0;
                is_load = 1; sext32 = 1; break;
            }
            if (is_load) {
                v = rd(dev, off, bytes);
                if (sext64) {
                    int sh = 64 - bytes * 8;
                    v = (uint64_t)(((int64_t)(v << sh)) >> sh);
                } else if (sext32) {
                    int sh = 32 - bytes * 8;
                    v = (uint32_t)(((int32_t)((uint32_t)v << sh)) >> sh);
                } else if (size != 3) {
                    v &= (1ULL << (bytes * 8)) - 1;   /* W write zeroes 63:32 */
                }
                if (rt != 31) *mmio_a64_xreg(uc, rt) = v;
            } else {
                v = rt == 31 ? 0 : *mmio_a64_xreg(uc, rt);
                wr(dev, off, v, bytes);
            }
        }
        if (wb) *mmio_a64_xreg(uc, rn) += (uint64_t)wb_imm;
        A64_CTX_PC(uc) += 4;
        return 1;
    }

    /* LDP/STP: bits 25:23 are 001 post-index, 010 signed offset, 011
     * pre-index; 000 is LDNP/STNP and 1xx is not a pair. */
    if (op29_27 == 5 && ((insn >> 23) & 7) >= 1 && ((insn >> 23) & 7) <= 3) {
        int opc = (insn >> 30) & 3, L = (insn >> 22) & 1;
        int mode = (insn >> 23) & 3;             /* 1 post, 2 offset, 3 pre */
        int rt = insn & 31, rt2 = (insn >> 10) & 31, rn = (insn >> 5) & 31;
        int64_t imm7 = (int64_t)(int32_t)(((insn >> 15) & 0x7Fu) << 25) >> 25;
        int bytes, sext = 0;

        if (V) {
#if A64_CTX_HAS_V
            bytes = 4 << opc;
            if (bytes > 8) return 0;        /* Q pairs; opc 3 is unallocated */
#else
            return 0;
#endif
        } else if (opc == 0) bytes = 4;
        else if (opc == 1) {                /* LDPSW; the store form is STGP */
            if (!L) return 0;
            bytes = 4; sext = 1;
        }
        else if (opc == 2) bytes = 8;
        else return 0;

        if (L) {
            uint64_t a = rd(dev, off, bytes), b = rd(dev, off + bytes, bytes);
            if (sext) {
                a = (uint64_t)(int64_t)(int32_t)a;
                b = (uint64_t)(int64_t)(int32_t)b;
            }
#if A64_CTX_HAS_V
            if (V) { A64_CTX_V(uc, rt) = a; A64_CTX_V(uc, rt2) = b; } else
#endif
            {
                if (rt != 31) *mmio_a64_xreg(uc, rt) = a;
                if (rt2 != 31) *mmio_a64_xreg(uc, rt2) = b;
            }
        } else {
            uint64_t a, b;
#if A64_CTX_HAS_V
            if (V) { a = (uint64_t)A64_CTX_V(uc, rt); b = (uint64_t)A64_CTX_V(uc, rt2); } else
#endif
            {
                a = rt == 31 ? 0 : *mmio_a64_xreg(uc, rt);
                b = rt2 == 31 ? 0 : *mmio_a64_xreg(uc, rt2);
            }
            wr(dev, off, a, bytes);
            wr(dev, off + bytes, b, bytes);
        }
        if (mode == 1 || mode == 3)
            *mmio_a64_xreg(uc, rn) += (uint64_t)(imm7 * bytes);
        A64_CTX_PC(uc) += 4;
        return 1;
    }
    return 0;
}

#endif /* __aarch64__ && !_WIN32 */
#endif /* MMIO_DECODE_A64_H */
