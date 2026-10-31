/*
 * fiber_delete.c -- XAPI's DeleteFiber (sub_0032A280), the lifted one kept here (src/manual_functions.json) with
 * one change: the fiber's host side (src/fibers.c: its slot and host stack) goes with it (#153).
 */
#define RECOMP_GENERATED_CODE
#include "recomp_funcs.h"

void ngb_fiber_deleted(uint32_t guest);   /* src/fibers.c */

/**
 * sub_0032A280
 * Original: 0x0032A280 - 0x0032A293 (19 bytes, 5 insns)
 * Category: game_vtable
 * CC: cdecl, 1 params, returns int_or_void
 * Frame: fpo_leaf
 */
void sub_0032A280(void)
{
    RECOMP_ENTRY(0x0032A280u);

loc_0032A280: ;
    eax = MEM32(esp + 4);
    ngb_fiber_deleted(eax);   /* the one change: the host side goes with it */
    { uint32_t _icall_esp = g_esp;
    PUSH32(esp, MEM32(eax + 8));
    PUSH32(esp, MEM32(eax + 4));
    { uint32_t _icall_target = MEM32(0x3E1504); PUSH32(esp, 0x0032A290u); RECOMP_ICALL_SAFE_AT(_icall_target, _icall_esp, 0x0032A28Au); } /* indirect call */
    }

loc_0032A290: ;
    esp += 8; return; /* ret 4 */

}

