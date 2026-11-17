"""Decode CCS C28x word-addressed HEX or TI 16-bit hexadecimal DAT exports.

This is an offline decoder, never a debugger/flash tool. Python 3.9+, stdlib only.
"""
from pathlib import Path


def load_ccs_hex(text):
    """CCS C28x format: word addresses, MSB-first text within each 16-bit word.

    This is intentionally not a general byte-addressed Intel HEX decoder.
    """
    memory, upper, eof = {}, 0, False
    for lineno, line in enumerate(text.splitlines(), 1):
        line = line.strip()
        if not line:
            continue
        if eof:
            raise ValueError('Data after HEX EOF')
        if not line.startswith(':'):
            raise ValueError(f'Line {lineno}: expected Intel HEX record')
        b = bytes.fromhex(line[1:])
        if len(b) < 5 or len(b) != b[0] + 5 or sum(b) & 255:
            raise ValueError(f'Line {lineno}: HEX checksum/length failure')
        n, address, kind, data = b[0], int.from_bytes(b[1:3], 'big'), b[3], b[4:-1]
        if kind == 0:
            if n % 2:
                raise ValueError('C28x payload must contain whole 16-bit words')
            for k in range(n // 2):
                target = upper + address + k
                if target in memory:
                    raise ValueError(f'Overlapping word at {target:#x}')
                memory[target] = int.from_bytes(data[2*k:2*k+2], 'big')
        elif kind == 1:
            if n or address:
                raise ValueError('Invalid EOF record')
            eof = True
        elif kind == 4 and n == 2 and address == 0:
            upper = int.from_bytes(data, 'big') << 16
        else:
            raise ValueError(f'Unsupported C28x HEX record type {kind}')
    if not eof:
        raise ValueError('Missing HEX EOF')
    return memory


def load_ti_dat(text):
    lines = [s.split('//')[0].strip() for s in text.splitlines()]
    lines = [s for s in lines if s]
    if not lines:
        raise ValueError('Empty export')
    header = [int(x, 16) for x in lines[0].split()]
    if len(header) != 5 or header[:2] != [0x1651, 1]:
        raise ValueError('Expected TI DAT header: 1651 1 <word-address> <page> <word-count>')
    values = [int(x, 16) for line in lines[1:] for x in line.split()]
    if len(values) != header[4] or any(v < 0 or v > 0xFFFF for v in values):
        raise ValueError('TI DAT word count/16-bit range mismatch')
    return {header[2] + i: v for i, v in enumerate(values)}


def load_memory(path):
    text = Path(path).read_text(encoding='utf-8-sig')
    return load_ccs_hex(text) if text.lstrip().startswith(':') else load_ti_dat(text)

