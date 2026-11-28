"""RECOMP_SPIN marks a loop that waits on memory, not one that searches it."""
import pytest
from . import config
from .translator import FunctionTranslator


def _lift(code_hex):
    b = bytes.fromhex(code_hex)
    base = 0x10000
    config._install([config.Section('.text', base, len(b), 0, len(b), True)],
                    entry_point=base, kernel_thunk_addr=base, origin='spin-block')
    db = {base: dict(start=hex(base), end=base + len(b), _addr=base, size=len(b))}
    return FunctionTranslator(b, db).translate_function(base, db[base])


@pytest.mark.parametrize('name, code_hex, spin', [
    # mov ecx,[edx]; cmp ecx,eax; jb: the same place until something writes it
    ('wait', '8b0a3bc872fac3', True),
    # dec ecx; cmp dword [edx],0; jne: a wait that counts its rounds
    ('wait with a count', '49833a0075fac3', True),
    # mov eax,[0x12345678]; cmp dword [eax+8],0; je: through a pointer read anew
    ('wait through a pointer', 'a178563412837808 0074f5c3', True),
    # mov eax,[0x12345678]; mov eax,[eax+0x10]; test eax,eax; je: the register
    # loaded anew, then used
    ('wait through a pointer, one register', 'a1785634128b401085c074f4c3', True),
    # mov eax,[0x12345678]; add eax,8; cmp dword [eax],0; je
    ('wait at a pointer plus eight', 'a17856341283c00883380074f3c3', True),
    # mov al,[edx]; cmp byte [eax+0x12345678],0; je: part of a register loaded anew
    ('wait indexed by a byte read anew', '8a0280b8785634120074f5c3', True),
    # xor ah,al; cmp byte [eax+0x12345678],0; je: not a zeroing, a step
    ('search stepped by xor', '30c480b8785634120074f5c3', False),
    # lea esi,[esi+1]; cmp byte [esi],0; jne
    ('search stepped by lea', '8d7601803e0075f8c3', False),
    # inc al; movzx ebp,al; cmp word [esi+ebp*2],0x100; jb: a table walked
    ('table search', 'fec00fb6e866813c6e000172f3c3', False),
    # inc eax; mov al,[edx]; cmp byte [eax+0x12345678],0; je: the upper part steps, the low byte reloaded
    ('search stepped under a byte reload', '408a0280b8785634120074f4c3', False),
    # mov edx,ecx; mov ecx,ebx; mov ebx,eax; inc eax; cmp dword [edx],0; je: a step reaching the address late
    ('search delayed through three copies', '89ca89d989c340833a0074f4c3', False),
    # sub eax,eax; mov al,[ecx]; cmp byte [eax+0x12345678],0; je: zeroed, then a byte read anew
    ('wait indexed after sub zeroing', '29c08a0180b8785634120074f3c3', True),
    # mov eax,[eax]; test eax,eax; jne: a list walked
    ('list walk', '8b0085c075fac3', False),
    # mov ebx,eax; mov eax,[ebx+4]; test eax,eax; jne: a list walked through a copy
    ('list walk through a copy', '8bd88b430485c075f7c3', False),
    # mov al,[ecx]; inc ecx; test al,al; jne: a string measured
    ('string scan', '8a014184c075f9c3', False),
])
def test_spin_block(name, code_hex, spin):
    text = _lift(code_hex.replace(' ', ''))
    assert ('RECOMP_SPIN(' in text) == spin, name
