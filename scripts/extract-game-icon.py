#!/usr/bin/env python3
"""Extract the original MP executable's icon resources into a private ICO."""
import argparse
from pathlib import Path
import struct
import zipfile

parser = argparse.ArgumentParser()
parser.add_argument('archive', type=Path)
parser.add_argument('--output', type=Path, default=Path('data/browser/web/cod2.ico'))
args = parser.parse_args()
with zipfile.ZipFile(args.archive) as archive:
    names = [n for n in archive.namelist() if n.lower().endswith('/cod2mp_s.exe')]
    if len(names) != 1:
        raise SystemExit('Expected exactly one original CoD2MP_s.exe')
    pe = archive.read(names[0])

def u16(offset): return struct.unpack_from('<H', pe, offset)[0]
def u32(offset): return struct.unpack_from('<I', pe, offset)[0]

header = u32(0x3c)
assert pe[header:header+4] == b'PE\0\0'
optional = header + 24
assert u16(optional) == 0x10b, 'Expected PE32'
sections = optional + u16(header + 20)

def file_offset(rva):
    for i in range(u16(header + 6)):
        section = sections + i * 40
        virtual_size, address, raw_size, offset = struct.unpack_from('<IIII', pe, section + 8)
        if address <= rva < address + max(virtual_size, raw_size):
            return offset + rva - address
    raise ValueError(f'Unmapped resource RVA {rva:#x}')

resource = file_offset(u32(optional + 96 + 2 * 8))
leaves = {}

def visit(offset, keys=()):
    count = u16(resource + offset + 12) + u16(resource + offset + 14)
    for i in range(count):
        name, child = struct.unpack_from('<II', pe, resource + offset + 16 + i * 8)
        key = keys + (name,)
        if child & 0x80000000:
            visit(child & 0x7fffffff, key)
        else:
            rva, size = struct.unpack_from('<II', pe, resource + child)
            start = file_offset(rva)
            leaves[key] = pe[start:start+size]

visit(0)
group_key = sorted(key for key in leaves if key[0] == 14)[0]
group = leaves[group_key]
reserved, kind, count = struct.unpack_from('<HHH', group)
assert reserved == 0 and kind == 1 and count > 0
directory = bytearray(struct.pack('<HHH', 0, 1, count))
images = []
offset = 6 + 16 * count
for i in range(count):
    entry = group[6+i*14:6+(i+1)*14]
    icon_id = struct.unpack_from('<H', entry, 12)[0]
    candidates = [key for key in leaves if key[:2] == (3, icon_id)]
    key = next((key for key in candidates if key[2] == group_key[2]), candidates[0])
    pixels = leaves[key]
    assert len(pixels) == struct.unpack_from('<I', entry, 8)[0]
    directory.extend(entry[:12] + struct.pack('<I', offset))
    images.append(pixels)
    offset += len(pixels)
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_bytes(directory + b''.join(images))
print(f'Extracted {count} original icon sizes to {args.output} ({offset} bytes)')
