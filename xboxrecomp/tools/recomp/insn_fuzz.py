"""Differential fuzzing of the lifter, one instruction at a time.

    python -m tools.recomp.insn_fuzz XBE [--mnemonics a,b,...] [--per 6] [--trials 24]

Collects real encodings of the chosen instructions from the XBE's code,
lifts each one on its own (Lifter.lift_instruction), compiles the C into a
harness, and runs harness and Unicorn on the same random register, x87 and
memory state. Any difference in a general register, an XMM lane, an x87
stack slot or a memory byte is a lifting bug, reported with the encoding
and the inputs.

Flag-setting instructions (cmp, comiss, fcomi, ...) are not testable this
way -- the lifter turns flags into deferred comparisons the next jcc reads --
and are skipped. x87 results are compared with a relative tolerance: the
lifted code computes in double, the console in 80 bits.
"""

import argparse
import math
import os
import random
import struct
import subprocess
import sys
import tempfile

import capstone
import unicorn as uc
from unicorn import x86_const as x

from tools.recomp.disasm import Disassembler
from tools.recomp.lifter import Lifter

HERE = os.path.dirname(os.path.abspath(__file__))
RUNTIME = os.path.join(HERE, '../../templates/runtime')

MEM_SIZE = 64 << 20
SCRATCH = 0x00800000          # registers point around here
GPRS = ['eax', 'ecx', 'edx', 'ebx', 'esp', 'ebp', 'esi', 'edi']
UC_GPR = [x.UC_X86_REG_EAX, x.UC_X86_REG_ECX, x.UC_X86_REG_EDX, x.UC_X86_REG_EBX,
          x.UC_X86_REG_ESP, x.UC_X86_REG_EBP, x.UC_X86_REG_ESI, x.UC_X86_REG_EDI]

DEFAULT = ('movss movaps movups movlps movhps movhlps movlhps movsd shufps unpcklps '
           'unpckhps mulss addss subss divss mulps addps subps divps sqrtss sqrtps '
           'xorps andps andnps orps minss maxss minps maxps rsqrtss rcpss rcpps '
           'rsqrtps cvtsi2ss cvtss2si cvttss2si cvtps2pi cvttps2pi cvtss2sd cvtsd2ss '
           'fld fst fstp fild fistp fist fxch fadd faddp fsub fsubp fsubr fsubrp fmul fmulp '
           'fdiv fdivp fdivr fdivrp fchs fabs fsqrt fsin fcos fsincos fpatan fptan fscale '
           'f2xm1 fyl2x frndint fld1 fldz fldpi fldln2 fldlg2 fiadd fimul fidiv fisub '
           'cmpneqps cmpltps cmpleps cmpeqps cmpnltps cmpnleps').split()


def collect(xbe, mnemonics, per):
    d = open(xbe, 'rb').read()
    base = struct.unpack_from('<I', d, 0x104)[0]
    n = struct.unpack_from('<I', d, 0x11C)[0]
    sh = struct.unpack_from('<I', d, 0x120)[0] - base
    cs = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    cs.detail = True
    cs.skipdata = True
    found = {}
    for i in range(n):
        fl, va, vs, ra, rs = struct.unpack_from('<IIIII', d, sh + i * 56)
        if not fl & 4:                     # executable sections only
            continue
        for ins in cs.disasm(d[ra:ra + rs], va):
            m = ins.mnemonic
            if m not in mnemonics:
                continue
            odd = False
            for op in ins.operands:
                if op.type == capstone.x86.X86_OP_MEM:
                    if op.mem.segment or any(rr and ins.reg_name(rr) not in GPRS
                                             for rr in (op.mem.base, op.mem.index)):
                        odd = True
            if odd:
                continue
            key = (m, ins.op_str.split(',')[0].split('[')[0], bytes(ins.bytes[:2]))
            lst = found.setdefault(m, {})
            # distinct encodings: one per (mnemonic, first operand, opcode)
            if key not in lst and len(lst) < per:
                lst[key] = ins
    return [ins for m in mnemonics for ins in found.get(m, {}).values()]


def rand_float(r):
    k = r.random()
    if k < 0.05:
        return 0.0
    if k < 0.10:
        return -0.0
    if k < 0.15:
        return float(r.choice([0.5, 1.5, 2.5, -0.5, -1.5, 1.0, -1.0]))
    if k < 0.55:
        return r.uniform(-4, 4)
    if k < 0.85:
        return r.uniform(-1000, 1000)
    return r.uniform(-1e6, 1e6)


def f2u(v):
    return struct.unpack('<I', struct.pack('<f', v))[0]


def d2ext(v):
    """double -> (mantissa, exponent word) of an 80-bit extended."""
    if v == 0.0:
        return (0, 0x8000 if math.copysign(1, v) < 0 else 0)
    m, e = math.frexp(abs(v))           # v = m * 2**e, 0.5 <= m < 1
    mant = int(m * (1 << 64)) & 0xFFFFFFFFFFFFFFFF
    exp = (e - 1 + 16383) | (0x8000 if v < 0 else 0)
    return (mant, exp)


def ext2d(t):
    mant, exp = t[0], t[1]
    sign = -1.0 if exp & 0x8000 else 1.0
    e = exp & 0x7FFF
    if e == 0 and mant == 0:
        return sign * 0.0
    if e == 0x7FFF:
        return float('nan') if mant & 0x7FFFFFFFFFFFFFFF else sign * float('inf')
    return sign * math.ldexp(mant / float(1 << 63), e - 16383)


def addr_regs(insn_cs):
    used = set()
    for op in insn_cs.operands:
        if op.type == capstone.x86.X86_OP_MEM:
            for rr in (op.mem.base, op.mem.index):
                if rr:
                    used.add(GPRS.index(insn_cs.reg_name(rr)))
    return used


def make_state(r, depth, insn_cs):
    s = {}
    s['gpr'] = [SCRATCH + r.randrange(0x1000, 0x40000) & ~3 for _ in range(8)]
    used = addr_regs(insn_cs)
    for i in (0, 1, 2, 3, 6, 7):
        if i not in used and r.random() < 0.5:
            s['gpr'][i] = r.getrandbits(32)
    for i in used:                        # an index register stays small
        if s['gpr'][i] > SCRATCH and any(op.type == capstone.x86.X86_OP_MEM and op.mem.index
                                         and GPRS.index(insn_cs.reg_name(op.mem.index)) == i
                                         for op in insn_cs.operands):
            s['gpr'][i] = r.randrange(0, 64)
    s['xmm'] = [[f2u(rand_float(r)) for _ in range(4)] for _ in range(8)]
    s['top'] = r.randrange(8)
    s['depth'] = depth
    s['fp'] = [rand_float(r) for _ in range(8)]
    return s


def lift(insn_cs):
    dis = Disassembler()
    ins = dis._decode_instruction(insn_cs)
    lines = Lifter().lift_instruction(ins)
    return lines


HARNESS_HEAD = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#define RECOMP_GENERATED_CODE 1
#include "recomp_types.h"
ptrdiff_t g_xbox_mem_offset;
RECOMP_TLS RecompRegs g_regs = { .r_fp_control_word = 0x027F };   /* the registers (recomp_regs.h) */
uint32_t g_recomp_capture_va;
int recomp_capture_entry(uint32_t va) { (void)va; return 0; }
static RecompXmm *X[8];
static uint32_t *G[8];
#define fp_push(v) do { double _fp_value = (v); g_fp_top = (g_fp_top + 7u) & 7u; g_fp_stack[g_fp_top] = _fp_value; } while (0)
#define fp_pop() (g_fp_top = (g_fp_top + 1u) & 7u)
#define fp_top() g_fp_stack[g_fp_top]
#define fp_st(i) g_fp_stack[(g_fp_top + (i)) & 7u]
#define fp_st1() fp_st(1)
'''

HARNESS_MAIN = r'''
typedef void (*fn_t)(void);
int main(int argc, char **argv)
{
    static uint8_t *mem;
    RecompXmm *xs[8] = { &g_xmm0, &g_xmm1, &g_xmm2, &g_xmm3, &g_xmm4, &g_xmm5, &g_xmm6, &g_xmm7 };
    uint32_t *gs[8] = { &g_eax, &g_ecx, &g_edx, &g_ebx, &g_esp, &g_ebp, &g_esi, &g_edi };
    memcpy(X, xs, sizeof xs); memcpy(G, gs, sizeof gs);
    mem = calloc(1, 64u << 20);
    g_xbox_mem_offset = (ptrdiff_t)mem;
    FILE *in = fopen(argv[1], "rb"), *out = fopen(argv[2], "wb");
    uint32_t idx, nreg;
    while (fread(&idx, 4, 1, in) == 1) {
        uint32_t gpr[8], xmm[8][4], top, npage;
        double fp[8];
        fread(gpr, 4, 8, in); fread(xmm, 4, 32, in); fread(&top, 4, 1, in); fread(fp, 8, 8, in);
        fread(&npage, 4, 1, in);
        uint32_t pages[16];
        for (uint32_t i = 0; i < npage; i++) { fread(&pages[i], 4, 1, in); fread(mem + pages[i], 1, 4096, in); }
        for (int i = 0; i < 8; i++) { *G[i] = gpr[i]; memcpy(X[i], xmm[i], 16); g_fp_stack[i] = fp[i]; }
        g_fp_top = (int)top; g_fp_control_word = 0x027F;
        TESTS[idx]();
        for (int i = 0; i < 8; i++) { gpr[i] = *G[i]; memcpy(xmm[i], X[i], 16); fp[i] = g_fp_stack[i]; }
        top = (uint32_t)g_fp_top;
        fwrite(gpr, 4, 8, out); fwrite(xmm, 4, 32, out); fwrite(&top, 4, 1, out); fwrite(fp, 8, 8, out);
        for (uint32_t i = 0; i < npage; i++) fwrite(mem + pages[i], 1, 4096, out);
    }
    (void)nreg;
    fclose(out);
    return 0;
}
'''


def pages_for(insn_cs, gpr):
    """Pages a memory operand may touch, given the register values."""
    pages = set()
    for op in insn_cs.operands:
        if op.type == capstone.x86.X86_OP_MEM:
            ea = op.mem.disp
            if op.mem.base:
                ea += gpr[GPRS.index(insn_cs.reg_name(op.mem.base))]
            if op.mem.index:
                ea += gpr[GPRS.index(insn_cs.reg_name(op.mem.index))] * op.mem.scale
            ea &= 0xFFFFFFFF
            if ea + 16 >= MEM_SIZE:
                return None                # outside the sandbox
            for a in (ea, ea + 15):
                pages.add(a & ~0xFFF)
    return sorted(pages)


def run_unicorn(insn_cs, s, pages, pagedata):
    mu = uc.Uc(uc.UC_ARCH_X86, uc.UC_MODE_32)
    mu.mem_map(0, MEM_SIZE)
    code = 0x03F00000
    mu.mem_write(code, bytes(insn_cs.bytes))
    for p, b in zip(pages, pagedata):
        mu.mem_write(p, b)
    for r, v in zip(UC_GPR, s['gpr']):
        mu.reg_write(r, v)
    for i in range(8):
        a, b, c, d = s['xmm'][i]
        mu.reg_write(x.UC_X86_REG_XMM0 + i, a | b << 32 | c << 64 | d << 96)
    mu.reg_write(x.UC_X86_REG_FPCW, 0x027F)
    # x87: physical register (top+i)&7 holds st(i); the first `depth` are valid.
    top = s['top']
    tag = 0xFFFF
    for i in range(8):                  # FP0..7 are the physical registers
        phys = (top + i) & 7
        mu.reg_write(x.UC_X86_REG_FP0 + phys, d2ext(s['fp'][phys]))
        if i < s['depth']:
            tag &= ~(3 << (2 * phys))
    mu.reg_write(x.UC_X86_REG_FPSW, top << 11)
    mu.reg_write(x.UC_X86_REG_FPTAG, tag)
    mu.emu_start(code, code + insn_cs.size, count=1)
    out = {'gpr': [mu.reg_read(r) & 0xFFFFFFFF for r in UC_GPR]}
    out['xmm'] = []
    for i in range(8):
        v = mu.reg_read(x.UC_X86_REG_XMM0 + i)
        out['xmm'].append([(v >> (32 * k)) & 0xFFFFFFFF for k in range(4)])
    ntop = (mu.reg_read(x.UC_X86_REG_FPSW) >> 11) & 7
    out['top'] = ntop
    out['fp'] = [ext2d(mu.reg_read(x.UC_X86_REG_FP0 + i)) for i in range(8)]
    out['mem'] = [bytes(mu.mem_read(p, 4096)) for p in pages]
    return out


def close(a, b, rel):
    if a is None or b is None:
        return True
    if math.isnan(a) and math.isnan(b):
        return True
    if math.isinf(a) or math.isinf(b):
        return a == b
    return abs(a - b) <= rel * max(1.0, abs(a), abs(b))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('xbe')
    ap.add_argument('--mnemonics', default=','.join(DEFAULT))
    ap.add_argument('--per', type=int, default=6)
    ap.add_argument('--trials', type=int, default=24)
    ap.add_argument('--seed', type=int, default=1)
    a = ap.parse_args()
    r = random.Random(a.seed)
    insns = collect(a.xbe, a.mnemonics.split(','), a.per)
    print(f'{len(insns)} encodings')

    tests, bodies = [], []
    for ins in insns:
        try:
            lines = lift(ins)
        except Exception as e:
            print(f'LIFT ERROR {ins.mnemonic} {ins.op_str}: {e}')
            continue
        if any('/*' in l and l.strip().startswith('/*') for l in lines) and len(lines) == 1:
            print(f'UNLIFTED  {ins.mnemonic} {ins.op_str}: {lines[0]}')
            continue
        bodies.append((ins, lines))

    with tempfile.TemporaryDirectory() as td:
        src = os.path.join(td, 'h.c')
        with open(src, 'w') as f:
            f.write(HARNESS_HEAD)
            for k, (ins, lines) in enumerate(bodies):
                f.write(f'static void t{k}(void) {{ int _flags = 0; uint32_t _fa = 0, _fb = 0; '
                        f'int32_t _fas = 0, _fbs = 0; int _cf = 0; uint32_t ebp_unused = 0; '
                        f'(void)_flags; (void)_fa; (void)_fb; (void)_fas; (void)_fbs; (void)_cf; (void)ebp_unused; uint32_t ebp = g_ebp;\n')
                for l in lines:
                    f.write('    ' + l + '\n')
                f.write('    g_ebp = ebp;\n}\n')
            f.write('static void (*TESTS[])(void) = {' + ','.join(f't{k}' for k in range(len(bodies))) + '};\n')
            f.write(HARNESS_MAIN)
        exe = os.path.join(td, 'h')
        cc = subprocess.run(['cc', '-O1', '-w', '-I', RUNTIME, src, '-o', exe, '-lm'],
                            capture_output=True, text=True)
        if cc.returncode:
            print(cc.stderr[-4000:])
            sys.exit(1)

        # Inputs
        cases = []
        for k, (ins, lines) in enumerate(bodies):
            for t in range(a.trials):
                s = make_state(r, r.choice([3, 4, 6]), ins)
                pages = pages_for(ins, s['gpr'])
                if pages is None:
                    continue
                data = [bytes(struct.pack('<1024I', *[f2u(rand_float(r)) if r.random() < 0.8
                                                     else r.getrandbits(32) for _ in range(1024)]))
                        for _ in pages]
                cases.append((k, s, pages, data))
        inp = os.path.join(td, 'in.bin')
        with open(inp, 'wb') as f:
            for k, s, pages, data in cases:
                f.write(struct.pack('<I', k))
                f.write(struct.pack('<8I', *s['gpr']))
                for v in s['xmm']:
                    f.write(struct.pack('<4I', *v))
                f.write(struct.pack('<I', s['top']))
                f.write(struct.pack('<8d', *s['fp']))
                f.write(struct.pack('<I', len(pages)))
                for p, d in zip(pages, data):
                    f.write(struct.pack('<I', p)); f.write(d)
        outp = os.path.join(td, 'out.bin')
        subprocess.run([exe, inp, outp], check=True, timeout=600)
        res = open(outp, 'rb').read()

    off = 0
    bad = {}
    for k, s, pages, data in cases:
        ins, lines = bodies[k]
        gpr = struct.unpack_from('<8I', res, off); off += 32
        xmm = [list(struct.unpack_from('<4I', res, off + 16 * i)) for i in range(8)]; off += 128
        top = struct.unpack_from('<I', res, off)[0]; off += 4
        fp = list(struct.unpack_from('<8d', res, off)); off += 64
        mem = [res[off + 4096 * i: off + 4096 * (i + 1)] for i in range(len(pages))]
        off += 4096 * len(pages)
        try:
            u = run_unicorn(ins, s, pages, data)
        except uc.UcError as e:
            continue
        why = []
        m = ins.mnemonic
        for i in range(8):
            if gpr[i] != u['gpr'][i]:
                why.append(f'{GPRS[i]} lifted {gpr[i]:08X} hw {u["gpr"][i]:08X}')
        for i in range(8):
            for l in range(4):
                if xmm[i][l] != u['xmm'][i][l]:
                    fl = struct.unpack('<f', struct.pack('<I', xmm[i][l]))[0]
                    fu = struct.unpack('<f', struct.pack('<I', u['xmm'][i][l]))[0]
                    if m.startswith(('rsqrt', 'rcp')) and close(fl, fu, 1e-3):
                        continue          # approximations by definition
                    why.append(f'xmm{i}[{l}] lifted {xmm[i][l]:08X} ({fl:g}) hw {u["xmm"][i][l]:08X} ({fu:g})')
        if m.startswith('f'):
            if top != u['top']:
                why.append(f'fp top lifted {top} hw {u["top"]}')
            else:
                tol = 1e-6 if m in ('fsin', 'fcos', 'fsincos', 'fpatan', 'fptan', 'f2xm1', 'fyl2x') else 1e-12
                for i in range(8):
                    if not close(fp[i], u['fp'][i], tol):
                        why.append(f'fp phys{i} lifted {fp[i]!r} hw {u["fp"][i]!r}')
        for p, a_, b_ in zip(pages, mem, u['mem']):
            if a_ != b_:
                diffs = [j for j in range(0, 4096, 4) if a_[j:j + 4] != b_[j:j + 4]]
                j = diffs[0]
                why.append(f'mem {p + j:08X} lifted {a_[j:j+4].hex()} hw {b_[j:j+4].hex()} (+{len(diffs) - 1} more)')
        if why:
            key = (ins.mnemonic, ins.op_str)
            bad.setdefault(key, (ins, lines, s, why))
    for (mn, ops), (ins, lines, s, why) in bad.items():
        print(f'\nMISMATCH {mn} {ops}   [{ins.bytes.hex()} at {ins.address:08X}]')
        print('  lifted: ' + ' '.join(lines)[:300])
        print('  input xmm0..2: ' + ' '.join(f'{struct.unpack("<f", struct.pack("<I", v))[0]:g}' for xm in s['xmm'][:3] for v in xm))
        print(f'  input top {s["top"]} st0..2: ' + ' '.join(repr(s['fp'][(s['top'] + i) & 7]) for i in range(3)))
        for w in why[:6]:
            print('  ' + w)
    print(f'\n{len(bodies)} lifted encodings, {len(cases)} cases, {len(bad)} mismatching encodings')


if __name__ == '__main__':
    main()
