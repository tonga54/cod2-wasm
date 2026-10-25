"""Grenade explosions after CoD WWII's, for the browser's renderer archive.

A frag's blast plays the impact table's grenadeExp effect for the surface it
lands on. CoD2's lit the ground for 100 ms under three to five fireball
sprites that lasted 100 to 300 ms, threw its surface's debris and left two to
three seconds of thin puffs. CoD WWII's frag goes off in a white flash and a
ball of fire that rolls out and darkens into a thick, dark cloud, throws
sparks and grit, rings the ground with dust and leaves smoke rising for
seconds.

Each surface's effect keeps its own debris, dust and crater, its grit and
crater held back until the flash and its light have passed, and runs two
generated effects in place of its fire, light and smoke: the fire, shared by
every surface, and a cloud of smoke and dust in its surface's color. Water
keeps its splash. The textures are made here:
- browser_blast_fireball, sixteen frames a fireball plays over its life:
  white-hot, swelling orange, then cooling red and breaking up into smoke;
- browser_blast_smoke, sixteen shapes of dense smoke lit from above.

The Pi rebuilds this archive and must match the published manifest byte for
byte, so the textures are made with correctly rounded operations only (+, -,
*, /, sqrt and floor) and seeded generators: no libm curve.
"""
from collections import namedtuple
import math
import random
import re
import struct

from browser_muzzle_flashes import GLOW, _Noise, _decay, _flame, _number, _smoothstep, split_elements, write_iwi

EXPLOSIONS = 'fx/explosions/'
FIRE = 'fx/explosions/browser_grenade_fire.efx'
SMOKE = 'fx/smoke/browser_grenade_smoke_{}.efx'
FIREBALL, SMOKE_TEXTURE = 'browser_blast_fireball', 'browser_blast_smoke'
# Each texture: frame width and height, columns, frames, and the material whose layout it takes.
TEXTURES = {FIREBALL: (128, 128, 4, 16, 'materials/gfx_fire_ball'),
            SMOKE_TEXTURE: (128, 128, 4, 16, 'materials/gfx_smokepuff_atlas')}

# The dust a blast raises from each surface, multiplying the smoke texture's
# grey; the elements each surface keeps from its original, by name.
Surface = namedtuple('Surface', 'dust kept')
SURFACES = {
    'blacktop': Surface((0.5, 0.47, 0.43), ('Decal', 'grain')),
    'concrete': Surface((0.58, 0.56, 0.53), ('decal', 'grain')),
    'dirt': Surface((0.62, 0.52, 0.38), ('dirt_eject', 'spinning_dust', 'blast_decal', 'rockychunks_high',
                                         'rockychunks_out', 'dirt_chunks')),
    'mud': Surface((0.42, 0.38, 0.3), ('mud_out', 'splat_decal', 'dirt_eject', 'dirt_chunks', 'spinning_smoke',
                                       'blast_decal', 'rockychunks_high', 'rockychunks_out')),
    'snow': Surface((0.88, 0.9, 0.93), ('ring_outter', 'snow_flakes_up', 'spinning_mist', 'blast_decal',
                                        'rockychunks_high', 'rockychunks_out', 'chunks', 'grain')),
    'wood': Surface((0.55, 0.5, 0.42), ('decal', 'grain', 'wood_splinter')),
}
# CoD2's grit sprites, broad pebble masks in front of the fire, wait out the
# flash and shrink so the fireball does not show through them as a mesh.
GRIT = 'gfx_dirtchunks'
GRIT_DELAY_MS = 180
GRIT_SIZE = 0.5
# A crater shows once the blast's light has gone: lit by it, its frayed rim
# glints across the ground.
CRATER_DELAY_MS = 250
# The blast's own smoke, dark grey-brown, lit orange by the fire as it forms.
SMOKE_DARK = (0.28, 0.26, 0.24)
SMOKE_LIT = (0.62, 0.45, 0.3)

_ZERO = ((0, 0), (1, 0))
_ONE = ((0, 1), (1, 1))
_VELOCITIES = ('X', 'Y', 'Z')


def _mix(a, b, t):
    return tuple(x + (y - x) * t for x, y in zip(a, b))


def _channel(name, keys, scale=None):
    lines = [f'\t{name}', '\t{', '\t\tcurve', '\t\t[']
    lines += ['\t\t\t' + ' '.join(_number(v) for v in key) for key in keys]
    lines.append('\t\t]')
    if scale is not None:
        lines += ['', '\t\tscale\t\t\t' + _number(scale)]
    lines.append('\t}')
    return '\n'.join(lines)


def _channels(rgb=((0, 1, 1, 1), (1, 1, 1, 1)), rgb_rand=None, alpha=_ONE, alpha_rand=None, size=_ONE,
              size_rand=None, size_scale=None, length=_ONE, length_rand=None, length_scale=None, spin=_ZERO,
              spin_rand=None, spin_scale=None, velocity=None, velocity_scale=None, velocity_rand=None):
    """Every channel the originals carry, in their order. Velocities are
    (x, y, z) curves sharing one scale, their random ends likewise."""
    velocity = velocity or (_ZERO, _ZERO, _ZERO)
    velocity_rand = velocity_rand or velocity
    channels = [('rgb', rgb, None), ('rgbRand', rgb_rand or rgb, None), ('alpha', alpha, None),
                ('alphaRand', alpha_rand or alpha, None), ('size', size, size_scale),
                ('sizeRand', size_rand or size, None), ('size2', _ONE, None), ('size2Rand', _ONE, None),
                ('length', length, length_scale), ('lengthRand', length_rand or length, None),
                ('rotationDelta', spin, spin_scale), ('rotationDeltaRand', spin_rand or spin, None)]
    channels += [('velocity' + axis, keys, velocity_scale) for axis, keys in zip(_VELOCITIES, velocity)]
    channels += [('velocity' + axis + 'Rand', keys, None) for axis, keys in zip(_VELOCITIES, velocity_rand)]
    channels += [('velocity2' + axis, _ZERO, None) for axis in _VELOCITIES]
    channels += [('velocity2' + axis + 'Rand', _ZERO, None) for axis in _VELOCITIES]
    return channels


def _element(kind, fields, channels, shaders=None, playfx=None):
    lines = [kind, '{']
    for key, value in fields:
        lines += [f'\t{key}\t\t\t\t{value}', '']
    for name, keys, scale in channels:
        lines += [_channel(name, keys, scale), '']
    if shaders:
        lines += ['\tshaders', '\t['] + [f'\t\t{shader}' for shader in shaders] + ['\t]']
    if playfx:
        lines += ['\tplayfx', '\t[', f'\t\t{playfx}', '\t]']
    if lines[-1] == '':
        lines.pop()
    lines += ['}', '']
    return '\n'.join(lines)


def _constant(value):
    return ((0, value), (1, value))


def _sequence(fields):
    """A sequence played once over the life, ending on its last frame."""
    return fields + (('sequenceStartFrameMode', '0'), ('sequenceFixedFrameValue', '1'), ('sequencePlayRateMode', '1'),
                     ('sequenceLoopMode', '1'), ('sequenceLoopTimes', '0'))


def _random_frame(fields):
    """One frame of the atlas, drawn at random and held."""
    return fields + (('sequenceStartFrameMode', '1'), ('sequenceFixedFpsValue', '0.000000'))


def fire_effect():
    """The flash, fireball, sparks, embers and light every surface shares.
    The effect's X axis is the surface's normal: up from the ground."""
    elements = [
        # A white flash over the blast, gone within a tenth of a second.
        _element('Particle', (
            ('name', 'flash'), ('flags', 'useRandomSize'), ('count', '1'), ('life', '70 100'),
            ('cullrange', '4000'), ('origin', '12 0 0 20 0 0'), ('rotation', '0 360')),
            _channels(rgb=((0, 1, 1, 1), (1, 1, 0.72, 0.45)), alpha=((0, 1), (0.35, 0.75), (1, 0)),
                      size=((0, 0.6), (0.25, 1), (1, 0.85)), size_rand=((0, 0.5), (0.25, 0.85), (1, 0.75)),
                      size_scale=115), (GLOW,)),
        # The fireball: five to seven overlapping balls of fire rolling out
        # from the blast and rising, each playing the sixteen frames from
        # white-hot to broken red over its life, turning slowly.
        _element('Particle', _sequence((
            ('name', 'fireball'),
            ('flags', 'useRandomColors useRandomSize useRandomRotationDelta useRandomVelocity relative'),
            ('spawnFlags', 'orgOnSphere axisFromSphere evenDistribution'), ('count', '4 6'), ('life', '320 480'),
            ('delay', '0 45'), ('cullrange', '4000'), ('origin', '14 0 0 30 0 0'), ('radius', '6 14'),
            ('rotation', '0 360'), ('gravity', '40 90'))),
            _channels(rgb=((0, 0.85, 0.62, 0.42), (0.4, 0.8, 0.52, 0.32), (1, 0.68, 0.4, 0.24)),
                      rgb_rand=((0, 0.8, 0.55, 0.35), (0.4, 0.72, 0.45, 0.28), (1, 0.58, 0.34, 0.2)),
                      alpha=((0, 1), (0.6, 0.9), (1, 0)), size=((0, 0.35), (0.15, 0.75), (0.5, 0.95), (1, 1)),
                      size_rand=((0, 0.3), (0.15, 0.6), (0.5, 0.75), (1, 0.8)), size_scale=100,
                      spin=_constant(-0.35), spin_rand=_constant(0.35), spin_scale=10,
                      velocity=(((0, 1), (0.3, 0.35), (1, 0.1)), _ZERO, _ZERO),
                      velocity_rand=(((0, 0.45), (0.3, 0.15), (1, 0.05)), _ZERO, _ZERO), velocity_scale=260),
            (FIREBALL,)),
        # Sparks thrown out over the ground, trailing streaks as they fall.
        _element('Tail', _random_frame((
            ('name', 'sparks'), ('flags', 'useRandomSize useRandomLength useRandomVelocity useRandomAlpha'),
            ('count', '16 24'), ('life', '220 480'), ('cullrange', '3000'), ('origin', '8 0 0'),
            ('gravity', '-400 -250'))),
            _channels(rgb=((0, 1, 0.95, 0.75), (1, 1, 0.55, 0.2)), alpha=((0, 1), (0.7, 0.8), (1, 0)),
                      alpha_rand=((0, 0.8), (0.7, 0.5), (1, 0)), size=_constant(2), size_rand=_constant(1.2),
                      size_scale=1, length=_constant(1), length_rand=_constant(0.5), length_scale=55,
                      velocity=(_constant(0.15), _constant(-1), _constant(-1)),
                      velocity_rand=(_constant(0.95), _constant(1), _constant(1)), velocity_scale=1400),
            ('gfx_spark_atlas',)),
        # Embers flung up that glow, redden and fall back.
        _element('Particle', _random_frame((
            ('name', 'embers'), ('flags', 'useRandomColors useRandomSize useRandomVelocity'), ('count', '8 14'),
            ('life', '900 1700'), ('delay', '30 120'), ('cullrange', '3000'), ('origin', '10 0 0 24 0 0'),
            ('gravity', '-320'))),
            _channels(rgb=((0, 1, 0.75, 0.35), (1, 0.8, 0.25, 0.05)), rgb_rand=((0, 1, 0.6, 0.2), (1, 0.6, 0.12, 0)),
                      alpha=((0, 1), (0.75, 0.9), (1, 0)), size=_constant(1.8), size_rand=_constant(1), size_scale=1,
                      velocity=(_constant(0.38), _constant(-0.5), _constant(-0.5)),
                      velocity_rand=(_constant(1), _constant(0.5), _constant(0.5)), velocity_scale=520),
            ('gfx_spark_atlas',)),
        # The blast lights its surroundings warmly, dimming and reddening
        # within a sixth of a second.
        _element('Light', (('name', 'light'), ('life', '160'), ('cullrange', '4000'), ('origin', '16 0 0')),
                 _channels(rgb=((0, 1, 0.82, 0.55), (0.4, 1, 0.62, 0.3), (1, 0.8, 0.4, 0.15)),
                           size=((0, 1), (0.25, 0.55), (0.6, 0.15), (1, 0)), size_scale=420)),
    ]
    return '\n'.join(elements)


def smoke_effect(dust):
    """The blast's smoke and its surface's dust: a dark cloud that billows out
    and climbs, a plume above it, a ring of dust over the ground and a haze
    left drifting up."""
    lit, dark = SMOKE_LIT, SMOKE_DARK
    cloud = ((0, *lit), (0.08, *dark), (0.6, *_mix(dark, dust, 0.45)), (1, *_mix(dark, dust, 0.8)))
    cloud_rand = ((0, *_mix(lit, dark, 0.3)), (0.08, *_mix(dark, dust, 0.15)), (0.6, *_mix(dark, dust, 0.6)),
                  (1, *dust))
    elements = [
        _element('Particle', _random_frame((
            ('name', 'burst'),
            ('flags', 'useAlpha useRandomColors useRandomSize useRandomRotationDelta useRandomVelocity relative'),
            ('spawnFlags', 'orgOnSphere axisFromSphere evenDistribution'), ('count', '8 10'),
            ('life', '2800 4200'), ('delay', '80 160'), ('cullrange', '4000'), ('origin', '16 0 0 40 0 0'),
            ('radius', '8 20'), ('rotation', '0 360'), ('gravity', '18 36'))),
            _channels(rgb=cloud, rgb_rand=cloud_rand, alpha=((0, 0), (0.08, 0.85), (0.4, 0.65), (1, 0)),
                      size=((0, 0.3), (0.1, 0.6), (0.4, 0.85), (1, 1)),
                      size_rand=((0, 0.25), (0.1, 0.48), (0.4, 0.68), (1, 0.8)), size_scale=92,
                      spin=_constant(-0.15), spin_rand=_constant(0.15), spin_scale=10,
                      velocity=(((0, 1), (0.2, 0.25), (1, 0.05)), _ZERO, _ZERO),
                      velocity_rand=(((0, 0.55), (0.2, 0.14), (1, 0.03)), _ZERO, _ZERO), velocity_scale=340),
            (SMOKE_TEXTURE,)),
        _element('Particle', _random_frame((
            ('name', 'plume'),
            ('flags', 'useAlpha useRandomColors useRandomSize useRandomRotationDelta useRandomVelocity relative'),
            ('count', '3 4'), ('life', '3600 5200'), ('delay', '120 240'), ('cullrange', '5000'),
            ('origin', '40 -10 -10 70 10 10'), ('rotation', '0 360'), ('gravity', '12 24'))),
            _channels(rgb=((0, *lit), (0.1, *dark), (1, *_mix(dark, dust, 0.7))),
                      rgb_rand=((0, *_mix(lit, dark, 0.4)), (0.1, *_mix(dark, dust, 0.2)), (1, *dust)),
                      alpha=((0, 0), (0.06, 0.7), (0.5, 0.48), (1, 0)), size=((0, 0.35), (0.3, 0.8), (1, 1.15)),
                      size_rand=((0, 0.3), (0.3, 0.65), (1, 0.9)), size_scale=105,
                      spin=_constant(-0.1), spin_rand=_constant(0.1), spin_scale=10,
                      velocity=(((0, 1), (0.25, 0.2), (1, 0.05)), _constant(-0.12), _constant(-0.12)),
                      velocity_rand=(((0, 0.72), (0.25, 0.14), (1, 0.03)), _constant(0.12), _constant(0.12)),
                      velocity_scale=420),
            (SMOKE_TEXTURE,)),
        _element('Particle', _random_frame((
            ('name', 'dust_ring'),
            ('flags', 'useAlpha useRandomColors useRandomSize useRandomRotationDelta useRandomVelocity relative'),
            ('spawnFlags', 'orgOnCylinder axisFromSphere evenDistribution'), ('count', '10 13'),
            ('life', '1300 2300'), ('cullrange', '3000'), ('origin', '4 0 0 10 0 0'), ('radius', '16 24'),
            ('height', '0'), ('rotation', '0 360'), ('gravity', '6 14'))),
            _channels(rgb=_constant_rgb(dust), rgb_rand=_constant_rgb(_mix(dust, dark, 0.35)),
                      alpha=((0, 0), (0.05, 0.5), (0.4, 0.3), (1, 0)), size=((0, 0.3), (0.2, 0.7), (1, 1)),
                      size_rand=((0, 0.25), (0.2, 0.55), (1, 0.75)), size_scale=70,
                      spin=_constant(-0.2), spin_rand=_constant(0.2), spin_scale=10,
                      velocity=(((0, 1), (0.15, 0.3), (0.5, 0.08), (1, 0)), _ZERO, _ZERO),
                      velocity_rand=(((0, 0.7), (0.15, 0.2), (0.5, 0.05), (1, 0)), _ZERO, _ZERO),
                      velocity_scale=720),
            (SMOKE_TEXTURE,)),
        _element('Particle', _random_frame((
            ('name', 'haze'), ('flags', 'useAlpha useRandomSize useRandomVelocity relative'),
            ('spawnFlags', 'orgOnSphere evenDistribution'), ('count', '2 3'), ('life', '4500 6500'),
            ('delay', '500 1100'), ('cullrange', '4000'), ('origin', '30 0 0 60 0 0'), ('radius', '30'),
            ('rotation', '0 360'), ('gravity', '4 8'))),
            _channels(rgb=_constant_rgb(_mix(dark, dust, 0.75)), alpha=((0, 0), (0.15, 0.22), (0.6, 0.14), (1, 0)),
                      size=((0, 0.7), (1, 1.35)), size_rand=((0, 0.55), (1, 1.1)), size_scale=110,
                      velocity=(_constant(0.6), _constant(-0.5), _constant(-0.5)),
                      velocity_rand=(_constant(1), _constant(0.5), _constant(0.5)), velocity_scale=30),
            (SMOKE_TEXTURE,)),
    ]
    return '\n'.join(elements)


def _constant_rgb(rgb):
    return ((0, *rgb), (1, *rgb))


def _runner(name, effect):
    return _element('FxRunner', (('name', name), ('cullrange', '5000')), _channels(),
                    playfx='/' + effect.removesuffix('.efx'))


def _delayed(element, earliest):
    """The element starting no sooner than earliest milliseconds, a delay line
    added after its name when it has none."""
    def delay(match):
        times = [max(earliest, float(v)) for v in match.group(2).split()]
        return match.group(1) + ' '.join(_number(v) for v in times)
    delayed, count = re.subn(r'(^\tdelay[ \t]+)([\d. \t]+?)(?=\r?$)', delay, element, count=1, flags=re.M)
    if count:
        return delayed
    newline = '\r\n' if '\r\n' in element else '\n'
    return re.sub(r'(^\tname[^\r\n]*\r?\n)', lambda match: match.group(1) + newline + f'\tdelay\t\t\t\t{earliest}' +
                  newline, element, count=1, flags=re.M)


def settle_grit(kind, element):
    """A grit sprite or streak starting no sooner than GRIT_DELAY_MS and
    GRIT_SIZE as large, a crater no sooner than CRATER_DELAY_MS; anything else
    as it is."""
    if kind == 'Decal':
        return _delayed(element, CRATER_DELAY_MS)
    if kind not in ('Particle', 'Tail') or GRIT not in element.split():
        return element

    def scale(match):
        return match.group(1) + ' '.join(_number(float(v) * GRIT_SIZE) for v in match.group(2).split())
    element = _delayed(element, GRIT_DELAY_MS)
    return re.sub(r'(\n\tsize\r?\n\t\{[^}]*?\n\t\tscale[ \t]+)([\d. \t]+?)(?=\r?\n)', scale, element, count=1)


def rewrite_surface(original, surface):
    """The original's kept elements, its grit settled, then the runners for
    the shared fire and the surface's smoke, in the original's line endings."""
    newline = '\r\n' if '\r\n' in original else '\n'
    kept_names = SURFACES[surface].kept
    kept = []
    for kind, element in split_elements(original):
        for line in element.splitlines():
            fields = line.split()
            if len(fields) == 2 and fields[0] == 'name':
                if fields[1] in kept_names:
                    kept.append(settle_grit(kind, element))
                break
    names = [next(line.split()[1] for line in element.splitlines() if line.split()[:1] == ['name'])
             for element in kept]
    if sorted(names) != sorted(kept_names):
        raise ValueError(f'grenadeExp_{surface}: expected {sorted(kept_names)}, found {sorted(names)}')
    runners = '\n'.join((_runner('browser_fire', FIRE), _runner('browser_smoke', SMOKE.format(surface))))
    return ((newline * 2).join(kept + [runners.replace('\n', newline)])).encode('latin-1')


def _lobes(rng, count, near, far, small, large):
    """count lobes about a central one: (direction x, y, distance, radius)."""
    lobes = [(0.0, 0.0, 0.0, large)]
    for _ in range(count):
        ux, uy = _direction(rng)
        lobes.append((ux, uy, rng.uniform(near, far), rng.uniform(small, large)))
    return lobes


def _direction(rng):
    while True:
        x, y = rng.uniform(-1, 1), rng.uniform(-1, 1)
        length = x * x + y * y
        if 0.01 < length <= 1:
            length = math.sqrt(length)
            return x / length, y / length


def fireball_frame(n, count=16, size=128):
    """Frame n of the fireball: a white-hot knot that swells into lobes of
    rolling fire, hottest where the fire is thickest, then cools from its edges
    inward, reddens and breaks up into smoke (dark here: the material adds its
    light). Every frame draws the same lobes, a step further out."""
    t = n / (count - 1)
    grow = 1 - (1 - t) * (1 - t) * (1 - t)
    lobes = _lobes(random.Random(77), 11, 0.22, 0.55, 0.13, 0.25)
    centre_x = sum(ux * d for ux, uy, d, r in lobes) / len(lobes)
    centre_y = sum(uy * d for ux, uy, d, r in lobes) / len(lobes)
    spread = 0.42 + 0.58 * grow
    swell = 0.6 + 0.4 * grow
    placed = [((ux * d - centre_x) * spread, (uy * d - centre_y) * spread, r * swell) for ux, uy, d, r in lobes]
    warp_x, warp_y, billows, holes = (_Noise(301 + k) for k in range(4))
    drift = 1.4 * t
    heat_scale = 1.0 - 0.72 * t
    pixels = []
    for y in range(size):
        for x in range(size):
            dx, dy = (x + 0.5) / (size / 2) - 1, (y + 0.5) / (size / 2) - 1
            rim = math.sqrt(dx * dx + dy * dy)
            if rim >= 1:
                pixels.append((0.0, 0.0, 0.0))
                continue
            nx, ny = dx * 2 + 5, dy * 2 + 5
            wx = dx + 0.16 * (warp_x.fbm(nx + drift, ny, 2) - 0.5)
            wy = dy + 0.16 * (warp_y.fbm(nx, ny - drift, 2) - 0.5)
            mass = 0.0
            for lx, ly, lr in placed:
                ox, oy = wx - lx, wy - ly
                mass += _decay(2.4 * (ox * ox + oy * oy) / (lr * lr))
            billow = billows.fbm(wx * 2.6 + 11, wy * 2.6 + 7 - drift, 3)
            thick = mass * (0.65 + 0.7 * billow)
            shape = thick / (0.7 + thick)
            body = _smoothstep(0.22 + 0.08 * t, 0.5 + 0.08 * t, shape)
            hole = holes.fbm(wx * 1.8 + 3, wy * 1.8 + 9 + drift, 2)
            burning = _smoothstep(0.05 + 0.62 * t, 0.35 + 0.62 * t, hole + 0.2 * (1 - t) * billow)
            heat = body * (0.2 + 1.45 * shape * shape) * heat_scale * (0.25 + 0.75 * burning)
            pixels.append(_flame(heat * (1 - _smoothstep(0.86, 1.0, rim))))
    return pixels


def smoke_frame(seed, size=128):
    """A billowing mass of smoke: seven to ten soft lobes torn at their edges
    by noise, thick and dark at its heart, lit from above and shadowed where
    smoke lies toward the light. Its grey takes the effect's colors; its alpha
    is the smoke's density."""
    rng = random.Random(seed)
    lobes = _lobes(rng, rng.randint(6, 9), 0.22, 0.5, 0.2, 0.42)
    warp_x, warp_y, billows, detail = (_Noise(seed * 5 + k) for k in range(4))
    density = [0.0] * (size * size)
    for y in range(size):
        for x in range(size):
            dx, dy = (x + 0.5) / (size / 2) - 1, (y + 0.5) / (size / 2) - 1
            wx = dx + 0.2 * (warp_x.fbm(dx * 2 + 3, dy * 2 + 3, 3) - 0.5)
            wy = dy + 0.2 * (warp_y.fbm(dx * 2 + 9, dy * 2 + 9, 3) - 0.5)
            mass = 0.0
            for ux, uy, d, r in lobes:
                ox, oy = wx - ux * d, wy - uy * d
                mass += _decay(1.8 * (ox * ox + oy * oy) / (r * r))
            density[y * size + x] = mass * (0.45 + 0.9 * billows.fbm(wx * 2.4 + 7, wy * 2.4 + 3, 4))
    # The light comes from above and a little to the left: a pixel whose smoke
    # thins toward it faces the light.
    step_x, step_y = -size // 64, -size // 32
    pixels = []
    for y in range(size):
        for x in range(size):
            dx, dy = (x + 0.5) / (size / 2) - 1, (y + 0.5) / (size / 2) - 1
            here = density[y * size + x]
            sx, sy = min(size - 1, max(0, x + step_x)), min(size - 1, max(0, y + step_y))
            facing = _smoothstep(-0.5, 0.5, here - density[sy * size + sx])
            lit = (0.58 + 0.42 * facing) * (1 - 0.2 * _smoothstep(0.6, 1.8, here))
            lit *= 0.95 - 0.08 * dy + 0.1 * (detail.fbm(dx * 4.5 + 1, dy * 4.5 + 1, 3) - 0.5)
            rim = math.sqrt(dx * dx + dy * dy)
            alpha = 0.92 * _smoothstep(0.15, 1.3, here) * (1 - _smoothstep(0.82, 1.0, rim))
            grey = min(1.0, max(0.0, lit))
            pixels.append((grey, grey, grey, alpha))
    return pixels


def texture_pixels(name):
    """A texture's width, height and rows of RGB (fireball) or RGBA (smoke)
    bytes, its frames laid out left to right and top to bottom."""
    frame_width, frame_height, columns, count, _ = TEXTURES[name]
    if name == FIREBALL:
        frames = [fireball_frame(n, count, frame_width) for n in range(count)]
    else:
        frames = [smoke_frame(401 + n, frame_width) for n in range(count)]
    channels = len(frames[0][0])
    rows = (count + columns - 1) // columns
    width, height = frame_width * columns, frame_height * rows
    pixels = bytearray(width * height * channels)
    for n, frame in enumerate(frames):
        left, top = (n % columns) * frame_width, (n // columns) * frame_height
        for y in range(frame_height):
            row = ((top + y) * width + left) * channels
            pixels[row:row + frame_width * channels] = bytes(
                min(255, int(c * 255 + 0.5)) for p in frame[y * frame_width:(y + 1) * frame_width] for c in p)
    return width, height, bytes(pixels)


def write_iwi_rgba(width, height, rgba):
    """An uncompressed ARGB32 IWI with its whole mip chain, smallest first,
    each level averaging its four parents' color and alpha."""
    levels = [(width, height, rgba)]
    while levels[-1][0] > 1 or levels[-1][1] > 1:
        w, h, source = levels[-1]
        half_w, half_h = max(1, w // 2), max(1, h // 2)
        level = bytearray(half_w * half_h * 4)
        for y in range(half_h):
            rows = (2 * y, min(h - 1, 2 * y + 1))
            for x in range(half_w):
                columns = (2 * x, min(w - 1, 2 * x + 1))
                for c in range(4):
                    total = sum(source[(row * w + column) * 4 + c] for row in rows for column in columns)
                    level[(y * half_w + x) * 4 + c] = (total + 2) // 4
        levels.append((half_w, half_h, bytes(level)))
    blobs = [b''.join(bytes((data[i + 2], data[i + 1], data[i], data[i + 3])) for i in range(0, len(data), 4))
             for _, _, data in levels]
    total = 28 + sum(len(blob) for blob in blobs)
    ends = [total]
    for blob in blobs[:3]:
        ends.append(ends[-1] - len(blob))
    return (b'IWi' + bytes((5, 1, 0)) + struct.pack('<HHH', width, height, 1) + struct.pack('<4i', *ends) +
            b''.join(reversed(blobs)))


def feathered_material(template, name, width, height, rows, columns):
    """An effect material for the image of its name, laid out as the
    template's soft-edged one (effect_zfeather, blended, or
    effect_zfeather_add_nofog, added) with its feather parameters, with the
    atlas's rows and columns."""
    head = bytearray(template[:0x50])
    technique_at = struct.unpack_from('<I', template, 0x38)[0]
    technique = template[technique_at:template.index(b'\0', technique_at)]
    if (technique not in (b'effect_zfeather', b'effect_zfeather_add_nofog') or
            struct.unpack_from('<HH', template, 0x34) != (1, 1) or struct.unpack_from('<II', template, 0x3c) != (0x44, 0x50)):
        raise ValueError('Unexpected feathered effect material template')
    constant = bytearray(template[0x50:0x64])
    strings = technique + b'\0'
    material = 0x64 + len(strings)
    strings += name.encode('ascii') + b'\0'
    colour_map = 0x64 + len(strings)
    strings += b'colorMap\0'
    feather = 0x64 + len(strings)
    strings += b'featherParms\0'
    struct.pack_into('<II', head, 0x00, material, material)
    head[14], head[15] = rows, columns
    struct.pack_into('<HH', head, 0x1c, width, height)
    struct.pack_into('<I', head, 0x38, 0x64)
    struct.pack_into('<III', head, 0x44, colour_map, struct.unpack_from('<I', template, 0x48)[0], material)
    struct.pack_into('<I', constant, 0, feather)
    return bytes(head) + bytes(constant) + strings


def grenade_explosion_assets(read):
    """The rewritten surface effects, the fire and smoke they run, and the
    textures and materials those draw, from read(name) giving an original
    file's bytes."""
    assets = {}
    for surface in SURFACES:
        name = f'{EXPLOSIONS}grenadeexp_{surface}.efx'
        assets[name] = rewrite_surface(read(name).decode('latin-1'), surface)
        assets[SMOKE.format(surface)] = smoke_effect(SURFACES[surface].dust).replace('\n', '\r\n').encode('latin-1')
    assets[FIRE] = fire_effect().replace('\n', '\r\n').encode('latin-1')
    for name, (frame_width, frame_height, columns, count, template) in TEXTURES.items():
        width, height, pixels = texture_pixels(name)
        assets['images/' + name + '.iwi'] = (write_iwi(width, height, pixels) if name == FIREBALL else
                                             write_iwi_rgba(width, height, pixels))
        assets['materials/' + name] = feathered_material(read(template), name, width, height,
                                                         (count + columns - 1) // columns, columns)
    return assets
