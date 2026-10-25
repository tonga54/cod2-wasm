"""Muzzle flashes after CoD WWII's, a different flame every shot, for the browser's renderer archive.

CoD2's view flashes for the MP44, BAR, Bren, Sten, the pistols and the trench
gun drew explosion fireball frames (gfx_exp_fireball_atlas) that grew for
100 ms beside the muzzle; at 120 frames a second they read as puffs of fire
swelling out of the gun's sides. CoD WWII lights the muzzle with a broad,
warm glow, white at its heart and orange to salmon at its rim, over flames
too brief to study. Each muzzle flash effect is rewritten from the original's own lights
and smoke and five generated elements: that glow, a flame core drawn from
eight burst shapes and stretched by its own width and height, one to three
tongues of flame from eight side views fanning out of the barrel, a few
sparks and a puff of smoke left in the air. Every shot draws its own shapes,
sizes, turns, strengths and fan; the flames are gone within 45 ms, the glow
within 75.

The Pi rebuilds this archive and must match the published manifest byte for
byte, so the textures are made with correctly rounded operations only (+, -,
*, /, sqrt and floor) and seeded generators: no libm curve.
"""
from collections import namedtuple
import math
import random
import re
import struct

MUZZLE_FLASHES = 'fx/muzzleflashes/'
MATERIAL_TEMPLATE = 'materials/gfx_spark_atlas'
GLOW, BURST, TONGUE = 'browser_muzzle_glow', 'browser_muzzle_burst', 'browser_muzzle_tongue'
# Each texture: frame width and height, columns, frames.
TEXTURES = {GLOW: (128, 128, 1, 1), BURST: (128, 128, 4, 8), TONGUE: (64, 256, 8, 8)}

# Sizes in units at the muzzle; counts as (fewest, most); a puff of 0 leaves
# the original's own smoke puffs alone.
Family = namedtuple('Family', 'glow burst tongue_length tongue_width tongues sparks spark_speed puff')
FAMILIES = {
    'pistol': Family(9.0, 3.4, 8.0, 2.0, (1, 1), (0, 1), 420, 7.0),
    'smg': Family(12.0, 4.6, 12.0, 2.6, (1, 2), (0, 2), 460, 9.0),
    'standard': Family(13.0, 5.0, 14.0, 2.8, (1, 2), (0, 2), 480, 10.0),
    'heavy': Family(14.0, 5.5, 17.0, 3.1, (1, 2), (0, 2), 500, 11.0),
    'rifle': Family(15.5, 5.8, 20.0, 3.4, (1, 2), (1, 2), 520, 0.0),
    'mg': Family(17.0, 6.6, 23.0, 3.8, (1, 3), (1, 3), 560, 13.0),
    'shotgun': Family(19.5, 7.5, 14.0, 4.2, (2, 3), (2, 4), 560, 0.0),
}

EFFECTS = {
    'heavy_view.efx': 'heavy', 'heavy.efx': 'heavy',
    'mg42hv_view.efx': 'mg', 'mg42hv.efx': 'mg',
    'pistolflash_view.efx': 'pistol', 'pistolflash.efx': 'pistol',
    'rifleflash.efx': 'rifle',
    'shotgunflash.efx': 'shotgun',
    'standardflashview.efx': 'standard', 'standardflashworld.efx': 'standard',
    'thompson_view.efx': 'smg', 'thompson.efx': 'smg',
}

# Flash elements live this long at most; smoke and lights are kept.
FLASH_LIFE_MS = 200

_ZERO = ((0, 0), (1, 0))
_ONE = ((0, 1), (1, 1))
_VELOCITY_CHANNELS = ('velocityX', 'velocityY', 'velocityZ', 'velocityXRand', 'velocityYRand', 'velocityZRand',
                      'velocity2X', 'velocity2Y', 'velocity2Z', 'velocity2XRand', 'velocity2YRand', 'velocity2ZRand')


def _number(value):
    text = f'{value:.4f}'.rstrip('0').rstrip('.')
    return text if text not in ('', '-0') else '0'


def _channel(name, keys, scale=None):
    lines = [f'\t{name}', '\t{', '\t\tcurve', '\t\t[']
    lines += ['\t\t\t' + ' '.join(_number(v) for v in key) for key in keys]
    lines.append('\t\t]')
    if scale is not None:
        lines += ['', '\t\tscale\t\t\t' + _number(scale)]
    lines.append('\t}')
    return '\n'.join(lines)


def _element(kind, fields, channels, shader):
    lines = [kind, '{']
    for key, value in fields:
        lines += [f'\t{key}\t\t\t\t{value}', '']
    for name, keys, scale in channels:
        lines += [_channel(name, keys, scale), '']
    lines += ['\tshaders', '\t[', f'\t\t{shader}', '\t]', '}', '']
    return '\n'.join(lines)


def _channels(rgb, alpha, alpha_rand, size, size_rand, size_scale, size2=_ONE, size2_rand=_ONE, size2_scale=None,
              length=_ONE, length_rand=_ONE, length_scale=None, velocity=None, rgb_rand=None):
    """Every channel the originals carry, in their order: unspecified
    velocities are still, unused second sizes and lengths stay at one."""
    velocity = velocity or {}
    channels = [('rgb', rgb, None), ('rgbRand', rgb_rand or rgb, None), ('alpha', alpha, None),
                ('alphaRand', alpha_rand, None),
                ('size', size, size_scale), ('sizeRand', size_rand, None), ('size2', size2, size2_scale),
                ('size2Rand', size2_rand, None), ('length', length, length_scale), ('lengthRand', length_rand, None),
                ('rotationDelta', _ZERO, None), ('rotationDeltaRand', _ZERO, None)]
    for name in _VELOCITY_CHANNELS:
        keys, scale = velocity.get(name, (_ZERO, None))
        channels.append((name, keys, scale))
    return channels


def flash_elements(family):
    """The glow, core, tongues, sparks and puff of a family's flash, as effect text."""
    f = FAMILIES[family]
    elements = [
        # CoD WWII's warm glow about the muzzle, its strength and warmth drawn
        # per shot.
        _element('Particle', (
            ('name', 'glow'), ('flags', 'useRandomColors useRandomAlpha useRandomSize relative'),
            ('spawnFlags', 'evenDistribution'), ('count', '1'), ('life', '50 75'), ('origin', '2 0 0 4 0 0'),
            ('rotation', '180 -180'), ('sequenceFixedFpsValue', '0.000000')),
            _channels(((0, 1, 1, 1), (1, 1, 0.82, 0.62)), ((0, 1), (0.12, 0.85), (0.45, 0.3), (1, 0)),
                      ((0, 0.55), (0.12, 0.45), (0.45, 0.15), (1, 0)), ((0, 0.85), (1, 1.1)), ((0, 0.6), (1, 0.8)),
                      f.glow, rgb_rand=((0, 1, 0.84, 0.66), (1, 1, 0.68, 0.45))), GLOW),
        # One of eight flame bursts, turned any way, its width and height
        # drawn apart.
        _element('Particle', (
            ('name', 'core'), ('flags', 'useRandomColors useRandomAlpha useRandomSize useRandomSize2 relative'),
            ('spawnFlags', 'evenDistribution'), ('count', '1'), ('life', '30 45'), ('origin', '1 0 0 3 0 0'),
            ('rotation', '180 -180'), ('nonUniformScale', '1'), ('sequenceStartFrameMode', '1'),
            ('sequenceFixedFpsValue', '0.000000')),
            _channels(((0, 1, 1, 1), (1, 1, 0.8, 0.6)), ((0, 1), (0.3, 0.75), (0.7, 0.2), (1, 0)),
                      ((0, 0.7), (0.3, 0.45), (0.7, 0.1), (1, 0)), ((0, 0.8), (1, 1.15)), ((0, 0.55), (1, 0.8)),
                      f.burst, ((0, 0.8), (1, 1.15)), ((0, 0.55), (1, 0.8)), f.burst,
                      rgb_rand=((0, 1, 0.9, 0.75), (1, 1, 0.7, 0.5))), BURST),
        # Tongues of flame drawn forward out of the barrel, fanning a little:
        # a tail trails its motion, back toward the shooter.
        _element('Tail', (
            ('name', 'tongue'), ('flags', 'useRandomAlpha useRandomSize useRandomLength useRandomVelocity relative'),
            ('count', f'{f.tongues[0]} {f.tongues[1]}'), ('life', '25 40'), ('origin', '-3 0 0 -2 0 0'),
            ('sequenceStartFrameMode', '1'), ('sequenceFixedFpsValue', '0.000000')),
            _channels(((0, 1, 1, 1), (1, 1, 0.85, 0.7)), ((0, 0.95), (0.35, 0.6), (1, 0)),
                      ((0, 0.6), (0.35, 0.3), (1, 0)), ((0, 0.9), (1, 1.1)), ((0, 0.65), (1, 0.85)), f.tongue_width,
                      length=((0, 0.75), (1, 1)), length_rand=((0, 0.45), (1, 0.7)), length_scale=f.tongue_length,
                      velocity={'velocityX': (((0, -0.5), (1, -0.5)), 50), 'velocityXRand': (((0, -0.42), (1, -0.42)), None),
                                'velocityY': (((0, -0.08), (1, -0.08)), 50), 'velocityYRand': (((0, 0.08), (1, 0.08)), None),
                                'velocityZ': (((0, -0.08), (1, -0.08)), 50), 'velocityZRand': (((0, 0.08), (1, 0.08)), None)}),
            TONGUE),
    ]
    if f.sparks[1]:
        # Sparks flung forward, trailing short streaks.
        elements.append(_element('Tail', (
            ('name', 'sparks'), ('flags', 'useRandomAlpha useRandomSize useRandomLength useRandomVelocity relative'),
            ('count', f'{f.sparks[0]} {f.sparks[1]}'), ('life', '70 140'), ('origin', '1 0 0'), ('gravity', '0.2 0.4'),
            ('sequenceFixedFrameValue', '3'), ('sequenceFixedFpsValue', '0.000000')),
            _channels(((0, 1, 0.9, 0.6), (1, 1, 0.55, 0.2)), ((0, 1), (0.6, 0.7), (1, 0)), ((0, 0.6), (0.6, 0.35), (1, 0)),
                      _ONE, ((0, 0.6), (1, 0.6)), 0.4, length_rand=((0, 0.5), (1, 0.5)), length_scale=4,
                      velocity={'velocityX': (((0, 1), (1, 1)), f.spark_speed),
                                'velocityY': (((0, -0.15), (1, -0.15)), f.spark_speed),
                                'velocityZ': (((0, -0.12), (1, -0.12)), f.spark_speed),
                                'velocityXRand': (((0, 0.6), (1, 0.6)), None), 'velocityYRand': (((0, 0.15), (1, 0.15)), None),
                                'velocityZRand': (((0, 0.15), (1, 0.15)), None)}),
            'gfx_spark_atlas'))
    if f.puff:
        # A puff of smoke left in the air as the gun moves on, rising as it
        # drifts forward and thins.
        elements.append(_element('Particle', (
            ('name', 'puff'), ('flags', 'useAlpha useRandomAlpha useRandomSize useRandomVelocity'),
            ('count', '1'), ('life', '450 750'), ('origin', '3 0 0 6 0 0'), ('rotation', '180 -180'),
            ('sequenceStartFrameMode', '1'), ('sequenceFixedFpsValue', '0.000000')),
            _channels(((0, 0.62, 0.6, 0.58), (1, 0.7, 0.68, 0.66)), ((0, 0), (0.08, 0.16), (1, 0)),
                      ((0, 0), (0.08, 0.09), (1, 0)), ((0, 0.3), (1, 1)), ((0, 0.25), (1, 0.75)), f.puff,
                      velocity={'velocityX': (((0, 0.5), (1, 0.5)), 60), 'velocityXRand': (((0, 0.9), (1, 0.9)), None),
                                'velocityY': (((0, -0.1), (1, -0.1)), 60), 'velocityYRand': (((0, 0.1), (1, 0.1)), None),
                                'velocityZ': (((0, 0.07), (1, 0.07)), 60), 'velocityZRand': (((0, 0.2), (1, 0.2)), None)}),
            'gfx_smokepuff_atlas'))
    return elements


def split_elements(text):
    """The original's elements as (kind, text) in order."""
    elements, i = [], 0
    for match in re.finditer(r'^(\w+)\s*\n\{', text, re.M):
        start = match.start()
        depth, j = 0, text.index('{', start)
        while True:
            if text[j] == '{':
                depth += 1
            elif text[j] == '}':
                depth -= 1
                if not depth:
                    break
            j += 1
        if start >= i:
            elements.append((match.group(1), text[start:j + 1]))
            i = j + 1
    return elements


def life_max(element):
    match = re.search(r'^[ \t]*life[ \t]+([\d.]+)(?:[ \t]+([\d.]+))?', element, re.M)
    if not match:
        return 0
    return max(float(value) for value in match.groups() if value)


def rewrite_flash(original, family):
    """The original's lights and lasting smoke, then the family's flash, in
    the original's line endings."""
    newline = '\r\n' if '\r\n' in original else '\n'
    kept = [element for kind, element in split_elements(original)
            if kind.lower() == 'light' or life_max(element) > FLASH_LIFE_MS]
    flash = '\n'.join(flash_elements(family)).replace('\n', newline)
    return ((newline * 2).join(kept + [flash])).encode('latin-1')


def _decay(x):
    """About exp(-x) for x >= 0: (1 + x/8)^-8."""
    y = 1.0 / (1.0 + x * 0.125)
    y *= y
    y *= y
    return y * y


def _smoothstep(a, b, x):
    t = min(1.0, max(0.0, (x - a) / (b - a)))
    return t * t * (3 - 2 * t)


def _turn(x, y, angle):
    """(x, y) turned by a small angle, by its sine and cosine series."""
    a2 = angle * angle
    c = 1 - a2 / 2 + a2 * a2 / 24
    s = angle * (1 - a2 / 6 + a2 * a2 / 120)
    return x * c - y * s, x * s + y * c


class _Noise:
    """Smooth value noise on a wrapping lattice, summed over octaves (0 to 1)."""

    def __init__(self, seed, size=64):
        rng = random.Random(seed)
        self.size = size
        self.values = [rng.random() for _ in range(size * size)]

    def value(self, x, y):
        size, values = self.size, self.values
        x0, y0 = math.floor(x), math.floor(y)
        fx, fy = x - x0, y - y0
        fx, fy = fx * fx * (3 - 2 * fx), fy * fy * (3 - 2 * fy)
        x0 %= size
        y0 %= size
        x1, y1 = (x0 + 1) % size, (y0 + 1) % size
        a, b = values[y0 * size + x0], values[y0 * size + x1]
        c, d = values[y1 * size + x0], values[y1 * size + x1]
        top = a + (b - a) * fx
        return top + (c + (d - c) * fx - top) * fy

    def fbm(self, x, y, octaves):
        total, amplitude, norm = 0.0, 1.0, 0.0
        for _ in range(octaves):
            total += self.value(x, y) * amplitude
            norm += amplitude
            x, y, amplitude = x * 2.03 + 17.1, y * 2.03 + 31.7, amplitude * 0.5
        return total / norm


def _flame(i):
    """A flame's color by its heat: deep red, orange, yellow, white."""
    return min(1.0, i * 1.9), min(1.0, max(0.0, (i - 0.12) * 1.45)), min(1.0, max(0.0, (i - 0.5) * 1.6)) * 0.85


def glow_frame(size=128):
    """A broad white-hot heart under an orange glow that reddens at its rim."""
    pixels = []
    for y in range(size):
        for x in range(size):
            dx, dy = (x + 0.5) / (size / 2) - 1, (y + 0.5) / (size / 2) - 1
            r2 = dx * dx + dy * dy
            heat = (1.6 * _decay(r2 / 0.03) + 0.6 * _decay(r2 / 0.12) + 0.07 * _decay(r2 / 0.45)) * (
                1 - _smoothstep(0.8, 1.0, math.sqrt(r2)))
            pixels.append((min(1.0, heat * 1.6), min(1.0, heat * 0.92), min(1.0, max(0.0, (heat - 0.55) * 1.6)) * 0.8))
    return pixels


# Unit steps of 360 / n degrees, exact by square roots.
_STEPS = {3: (-0.5, math.sqrt(3) / 2), 4: (0.0, 1.0), 5: ((math.sqrt(5) - 1) / 4, math.sqrt(10 + 2 * math.sqrt(5)) / 4),
          6: (0.5, math.sqrt(3) / 2)}


def _direction(rng):
    while True:
        x, y = rng.uniform(-1, 1), rng.uniform(-1, 1)
        length = x * x + y * y
        if 0.01 < length <= 1:
            length = math.sqrt(length)
            return x / length, y / length


def burst_frame(seed, size=128):
    """A muzzle flash seen from near its axis: a hot, ragged core and three
    to six jets of flame at uneven angles, with shorter ones between."""
    rng = random.Random(seed)
    count = rng.randint(3, 6)
    step_x, step_y = _STEPS[count]
    px, py = _direction(rng)
    jets = []
    for _ in range(count):
        jx, jy = _turn(px, py, rng.uniform(-0.4, 0.4) * 6.2832 / count)
        jets.append((jx, jy, rng.uniform(0.5, 0.95), rng.uniform(0.2, 0.42), 1.0))
        px, py = px * step_x - py * step_y, px * step_y + py * step_x
    for _ in range(rng.randint(3, 6)):
        jx, jy = _direction(rng)
        jets.append((jx, jy, rng.uniform(0.22, 0.5), rng.uniform(0.12, 0.25), 0.7))
    warp, stretch, turbulence, licks = (_Noise(seed * 7 + k) for k in (1, 2, 3, 4))
    pixels = []
    for y in range(size):
        for x in range(size):
            dx, dy = (x + 0.5) / (size / 2) - 1, (y + 0.5) / (size / 2) - 1
            r = math.sqrt(dx * dx + dy * dy)
            if r >= 1:
                pixels.append((0.0, 0.0, 0.0))
                continue
            nx, ny = dx * 2.5 + 8, dy * 2.5 + 8
            ux, uy = _turn(dx / r, dy / r, 1.4 * (warp.fbm(nx, ny, 3) - 0.5) * (0.25 + r))
            rw = r * (1 + 0.7 * (stretch.fbm(nx, ny, 3) - 0.5))
            jet = 0.0
            for qx, qy, length, width, strength in jets:
                along = rw / length
                if along >= 1:
                    continue
                w = width * (1 - 0.6 * along) + 0.03
                # 2 (1 - cos) is about the squared angle off the jet.
                jet = max(jet, strength * _decay(2 * (1 - (ux * qx + uy * qy)) / (w * w)) * (1 - along))
            lick = licks.fbm(ux * 1.5 + rw * 7 + 4, uy * 1.5 + 4, 3)
            t = turbulence.fbm(dx * 6 + 3, dy * 6 + 3, 4)
            core = 0.17 + 0.08 * t
            heat = (1.25 * _decay(rw * rw / (core * core)) + jet * (0.35 + 0.9 * t) * (0.6 + 0.8 * lick) +
                    0.1 * _decay(rw * rw / 0.25) * t)
            pixels.append(_flame(heat * (1 - _smoothstep(0.85, 1.0, r))))
    return pixels


def tongue_frame(seed, width=64, height=256):
    """A muzzle flash from the side, its base at the bottom: the jet swells
    where the gas overruns itself and thins to a ragged tip."""
    rng = random.Random(seed)
    base = rng.uniform(0.35, 0.55)
    swell_at, swell, swell_width = rng.uniform(0.2, 0.45), rng.uniform(0.25, 0.9), rng.uniform(0.08, 0.16)
    taper, knot, end = rng.uniform(0.0, 0.8), rng.uniform(0.2, 0.6), rng.uniform(0.75, 1.0)
    warp, turbulence = _Noise(seed * 5 + 1), _Noise(seed * 5 + 2)
    pixels = []
    for y in range(height):
        v = 1 - (y + 0.5) / height
        fade = 1 - v / end
        if fade <= 0:
            pixels.extend([(0.0, 0.0, 0.0)] * width)
            continue
        root = math.sqrt(fade)
        offset = (v - swell_at) / swell_width
        w = base * (1 + swell * _decay(offset * offset)) * (root + (fade - root) * taper) + 0.02
        along = ((0.6 * _decay(v * v / 0.0144) + 0.65 + knot * _decay(offset * offset / 0.64)) *
                 (root + (fade - root) * 0.4) * _smoothstep(0.004, 0.05, v) * (1 - _smoothstep(0.95, 1.0, v)))
        for x in range(width):
            u = (x + 0.5) / (width / 2) - 1
            uw = u + 0.35 * (warp.fbm(u * 1.5 + 5, v * 6 + 5, 3) - 0.5) * (0.3 + v)
            t = turbulence.fbm(u * 2 + 9, v * 9 + 9, 4)
            sides = 1 - _smoothstep(0.75, 1.0, abs(u))
            pixels.append(_flame(_decay(1.6 * uw * uw / (w * w)) * along * (0.55 + 0.8 * t) * 1.1 * sides))
    return pixels


def texture_pixels(name):
    """A texture's width, height and rows of RGB bytes, its frames laid out
    left to right and top to bottom."""
    frame_width, frame_height, columns, count = TEXTURES[name]
    if name == GLOW:
        frames = [glow_frame(frame_width)]
    elif name == BURST:
        frames = [burst_frame(101 + n, frame_width) for n in range(count)]
    else:
        frames = [tongue_frame(201 + n, frame_width, frame_height) for n in range(count)]
    rows = (count + columns - 1) // columns
    width, height = frame_width * columns, frame_height * rows
    pixels = bytearray(width * height * 3)
    for n, frame in enumerate(frames):
        left, top = (n % columns) * frame_width, (n // columns) * frame_height
        for y in range(frame_height):
            row = ((top + y) * width + left) * 3
            pixels[row:row + frame_width * 3] = bytes(min(255, int(c * 255 + 0.5))
                                                      for p in frame[y * frame_width:(y + 1) * frame_width] for c in p)
    return width, height, bytes(pixels)


def write_iwi(width, height, rgb):
    """An uncompressed ARGB32 IWI with its whole mip chain, smallest first,
    each level halving both sides down to one."""
    levels = [(width, height, rgb)]
    while levels[-1][0] > 1 or levels[-1][1] > 1:
        w, h, source = levels[-1]
        half_w, half_h = max(1, w // 2), max(1, h // 2)
        level = bytearray(half_w * half_h * 3)
        for y in range(half_h):
            rows = (2 * y, min(h - 1, 2 * y + 1))
            for x in range(half_w):
                columns = (2 * x, min(w - 1, 2 * x + 1))
                for c in range(3):
                    total = sum(source[(row * w + column) * 3 + c] for row in rows for column in columns)
                    level[(y * half_w + x) * 3 + c] = (total + 2) // 4
        levels.append((half_w, half_h, bytes(level)))
    blobs = [b''.join(bytes((data[i + 2], data[i + 1], data[i], 255)) for i in range(0, len(data), 3))
             for _, _, data in levels]
    total = 28 + sum(len(blob) for blob in blobs)
    ends = [total]
    for blob in blobs[:3]:
        ends.append(ends[-1] - len(blob))
    return (b'IWi' + bytes((5, 1, 0)) + struct.pack('<HHH', width, height, 1) + struct.pack('<4i', *ends) +
            b''.join(reversed(blobs)))


def effect_material(template, name, width, height, rows, columns):
    """An additive effect material for the image of its name, laid out as the
    template's, with the atlas's rows and columns."""
    head = bytearray(template[:0x50])
    technique = struct.unpack_from('<I', template, 0x38)[0]
    if (template[technique:technique + 17] != b'effect_add_nofog\0' or struct.unpack_from('<I', template, 0x3c)[0] != 0x44
            or struct.unpack_from('<HH', template, 0x34) != (1, 0)):
        raise ValueError('Unexpected effect material template')
    strings = b'effect_add_nofog\0'
    material = 0x50 + len(strings)
    strings += name.encode('ascii') + b'\0'
    colour_map = 0x50 + len(strings)
    strings += b'colorMap\0'
    struct.pack_into('<II', head, 0x00, material, material)
    head[14], head[15] = rows, columns
    struct.pack_into('<HH', head, 0x1c, width, height)
    struct.pack_into('<I', head, 0x38, 0x50)
    struct.pack_into('<I', head, 0x40, 0x50)
    struct.pack_into('<III', head, 0x44, colour_map, struct.unpack_from('<I', template, 0x48)[0], material)
    return bytes(head) + strings


def muzzle_flash_assets(read):
    """Every rewritten muzzle flash and its textures and materials, from
    read(name) giving an original file's bytes."""
    assets = {MUZZLE_FLASHES + name: rewrite_flash(read(MUZZLE_FLASHES + name).decode('latin-1'), family)
              for name, family in EFFECTS.items()}
    template = read(MATERIAL_TEMPLATE)
    for name, (frame_width, frame_height, columns, count) in TEXTURES.items():
        width, height, rgb = texture_pixels(name)
        assets['images/' + name + '.iwi'] = write_iwi(width, height, rgb)
        assets['materials/' + name] = effect_material(template, name, width, height, (count + columns - 1) // columns,
                                                      columns)
    return assets
