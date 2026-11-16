"""Decode owner BC6H panoramas before exposure, retaining sub-byte cloud detail."""
import ctypes
import hashlib
from pathlib import Path
import struct
import subprocess
import sys


def read_bc6h(path):
    import numpy as np
    data = Path(path).read_bytes()
    if len(data) < 148 or data[:4] != b'DDS ' or data[84:88] != b'DX10':
        raise ValueError('Expected a DX10 DDS panorama')
    height, width = struct.unpack_from('<II', data, 12)
    fmt, dimension, misc, layers, _ = struct.unpack_from('<5I', data, 128)
    if fmt not in (95, 96) or dimension != 3 or misc & 4 or layers != 1:
        raise ValueError('Expected one BC6H 2D image')
    if not 1 <= width <= 16384 or not 1 <= height <= 8192:
        raise ValueError('Panorama dimensions out of range')
    length = ((width + 3) // 4) * ((height + 3) // 4) * 16
    if len(data) < 148 + length:
        raise ValueError('Truncated BC6H blocks')
    root = Path(__file__).resolve().parent.parent
    source = root / 'scripts/remix_bc6h.c'
    header = root / 'third_party/bcdec/bcdec.h'
    digest = hashlib.sha256(source.read_bytes() + header.read_bytes()).hexdigest()[:16]
    library = root / 'out/remix-tools' / (digest + ('.dylib' if sys.platform == 'darwin' else '.so'))
    if not library.is_file():
        library.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(['cc', '-O2', '-shared', '-fPIC', str(source), '-o', str(library)], check=True)
    decoder = ctypes.CDLL(str(library)).remix_bc6h
    decoder.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int]
    decoder.restype = None
    blocks = ctypes.create_string_buffer(data[148:148 + length])
    pixels = np.empty((height, width, 3), dtype=np.float32)
    decoder(blocks, pixels.ctypes.data, width, height, int(fmt == 96))
    return np.maximum(np.nan_to_num(pixels, nan=0, posinf=65504, neginf=0), 0)
