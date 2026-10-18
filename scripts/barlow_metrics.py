"""Advance widths, ink bounds and cap height of the committed Barlow fonts,
read straight from their TrueType tables."""
from pathlib import Path
import struct

root = Path(__file__).resolve().parent.parent


class Font:
    def __init__(self, name):
        data = (root / 'third_party/barlow' / f'Barlow-{name}.ttf').read_bytes()
        tables = {}
        for i in range(struct.unpack_from('>H', data, 4)[0]):
            tag, _, offset, length = struct.unpack_from('>4sIII', data, 12 + 16 * i)
            tables[tag.decode()] = data[offset:offset + length]
        self.units = struct.unpack_from('>H', tables['head'], 18)[0]
        long_loca = struct.unpack_from('>h', tables['head'], 50)[0]
        glyphs = struct.unpack_from('>H', tables['maxp'], 4)[0]
        metrics = struct.unpack_from('>H', tables['hhea'], 34)[0]
        self.cap = struct.unpack_from('>h', tables['OS/2'], 88)[0]
        hmtx = tables['hmtx']
        self.advance = [struct.unpack_from('>H', hmtx, 4 * min(i, metrics - 1))[0] for i in range(glyphs)]
        loca = tables['loca']
        if long_loca:
            offsets = struct.unpack_from(f'>{glyphs + 1}I', loca)
        else:
            offsets = [2 * v for v in struct.unpack_from(f'>{glyphs + 1}H', loca)]
        glyf = tables['glyf']
        self.ink = [struct.unpack_from('>hhhh', glyf, offsets[i] + 2) if offsets[i + 1] > offsets[i] else None
                    for i in range(glyphs)]
        self.cmap = {}
        cmap = tables['cmap']
        for i in range(struct.unpack_from('>H', cmap, 2)[0]):
            platform, encoding, offset = struct.unpack_from('>HHI', cmap, 4 + 8 * i)
            if (platform, encoding) in ((3, 1), (0, 3)) and struct.unpack_from('>H', cmap, offset)[0] == 4:
                self._format4(cmap, offset)
                break

    def _format4(self, cmap, offset):
        count = struct.unpack_from('>H', cmap, offset + 6)[0] // 2
        ends = struct.unpack_from(f'>{count}H', cmap, offset + 14)
        starts = struct.unpack_from(f'>{count}H', cmap, offset + 16 + 2 * count)
        deltas = struct.unpack_from(f'>{count}h', cmap, offset + 16 + 4 * count)
        range_at = offset + 16 + 6 * count
        ranges = struct.unpack_from(f'>{count}H', cmap, range_at)
        for seg in range(count):
            for code in range(starts[seg], ends[seg] + 1):
                if code == 0xffff:
                    continue
                if ranges[seg]:
                    at = range_at + 2 * seg + ranges[seg] + 2 * (code - starts[seg])
                    glyph = struct.unpack_from('>H', cmap, at)[0]
                    glyph = (glyph + deltas[seg]) & 0xffff if glyph else 0
                else:
                    glyph = (code + deltas[seg]) & 0xffff
                self.cmap[code] = glyph

    def ink_width(self, text, tracking=0.0, width=1.0):
        """Ink from the first letter's left to the last one's right, in caps,
        with tracking caps between letters and the advances scaled by width."""
        glyphs = [self.cmap[ord(c)] for c in text]
        pen = 0.0
        left = right = None
        for i, glyph in enumerate(glyphs):
            ink = self.ink[glyph]
            if ink:
                x0, x1 = pen + ink[0] * width / self.cap, pen + ink[2] * width / self.cap
                left = x0 if left is None else min(left, x0)
                right = x1 if right is None else max(right, x1)
            pen += self.advance[glyph] * width / self.cap
            if i + 1 < len(glyphs):
                pen += tracking
        return right - left
