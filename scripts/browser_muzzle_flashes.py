"""Muzzle flashes that pop and are gone, for the browser's renderer archive.

CoD2's view flashes for the MP44, BAR, Bren, Sten, the pistols and the trench
gun draw explosion fireball frames (gfx_exp_fireball_atlas) that grow for
100 ms beside the muzzle; at 120 frames a second they read as puffs of fire
swelling out of the gun's sides. Each muzzle flash effect is rewritten from
the original's own lights and smoke and four brief elements: a burst of rays
at the muzzle (gfx_explosion_flash), a hot core inside it, a tongue of flame
streaking forward out of the barrel (gfx_starburst_1p) and a few sparks. The
burst and the core are gone within 55 ms, the tongue within 45 and the sparks
within 140; every shot differs in size, turn and length.
"""
import re

MUZZLE_FLASHES = 'fx/muzzleflashes/'

# Each effect's family: (burst size, core size, tongue length, tongue width,
# sparks (fewest, most), spark speed). Sizes are in units at the muzzle.
FAMILIES = {
    'pistol': (3.2, 1.2, 8.0, 1.8, (0, 1), 420),
    'rifle': (5.8, 2.1, 20.0, 3.4, (1, 2), 520),
    'standard': (5.0, 1.9, 14.0, 2.8, (0, 2), 480),
    'smg': (4.6, 1.8, 12.0, 2.6, (0, 2), 460),
    'heavy': (5.5, 2.1, 17.0, 3.1, (0, 2), 500),
    'shotgun': (7.5, 2.8, 14.0, 4.2, (2, 4), 560),
    'mg': (6.6, 2.5, 23.0, 3.8, (1, 3), 560),
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
        lines += ['', '\t\tscale\t\t\t' + ' '.join(_number(v) for v in (scale if isinstance(scale, tuple) else (scale,)))]
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


def _channels(rgb, rgb_rand, alpha, alpha_rand, size, size_rand, size_scale, length=_ONE, length_rand=_ONE,
              length_scale=None, velocity=None):
    """Every channel the originals carry, in their order: unspecified
    velocities are still, unused second sizes and lengths stay at one."""
    velocity = velocity or {}
    channels = [('rgb', rgb, None), ('rgbRand', rgb_rand, None), ('alpha', alpha, None),
                ('alphaRand', alpha_rand, None), ('size', size, size_scale), ('sizeRand', size_rand, None),
                ('size2', _ONE, None), ('size2Rand', _ONE, None), ('length', length, length_scale),
                ('lengthRand', length_rand, None), ('rotationDelta', _ZERO, None), ('rotationDeltaRand', _ZERO, None)]
    for name in _VELOCITY_CHANNELS:
        keys, scale = velocity.get(name, (_ZERO, None))
        channels.append((name, keys, scale))
    return channels


def flash_elements(family):
    """The burst, core, tongue and sparks of a family's flash, as effect text."""
    burst, core, tongue_length, tongue_width, (sparks_min, sparks_max), spark_speed = FAMILIES[family]
    hot = ((0, 1, 0.94, 0.8), (1, 1, 0.66, 0.36))
    elements = [
        # Rays at the muzzle, turned any way, swelling a little as they fade.
        _element('Particle', (
            ('name', 'burst'), ('flags', 'useRandomAlpha useRandomSize relative'),
            ('spawnFlags', 'evenDistribution'), ('count', '1'), ('life', '40 55'), ('origin', '3 0 0 4 0 0'),
            ('rotation', '180 -180'), ('sequenceFixedFpsValue', '0.000000')),
            _channels(hot, hot, ((0, 1), (0.25, 0.72), (0.6, 0.24), (1, 0)), ((0, 0.75), (0.25, 0.5), (0.6, 0.15), (1, 0)),
                      ((0, 0.8), (1, 1.15)), ((0, 0.55), (1, 0.8)), burst), 'gfx_explosion_flash'),
        # The white-hot heart of the burst, gone first.
        _element('Particle', (
            ('name', 'core'), ('flags', 'useRandomAlpha useRandomSize relative'), ('spawnFlags', 'evenDistribution'),
            ('count', '1'), ('life', '25 35'), ('origin', '2 0 0'), ('rotation', '180 -180'),
            ('sequenceFixedFrameValue', '7'), ('sequenceFixedFpsValue', '0.000000')),
            _channels(((0, 1, 0.98, 0.9), (1, 1, 0.9, 0.7)), ((0, 1, 0.98, 0.9), (1, 1, 0.9, 0.7)),
                      ((0, 1), (0.5, 0.6), (1, 0)), ((0, 0.8), (0.5, 0.4), (1, 0)), ((0, 1), (1, 0.85)),
                      ((0, 0.75), (1, 0.7)), core), 'gfx_spark_atlas'),
        # A tongue of flame drawn forward out of the barrel: the tail trails
        # its motion, back toward the shooter.
        _element('Tail', (
            ('name', 'tongue'), ('flags', 'useRandomAlpha useRandomSize useRandomLength useRandomVelocity relative'),
            ('life', '30 45'), ('origin', '-3 0 0 -2 0 0'), ('sequenceStartFrameMode', '1'),
            ('sequenceFixedFpsValue', '0.000000')),
            _channels(((0, 1, 0.95, 0.85), (1, 1, 0.72, 0.42)), ((0, 1, 0.95, 0.85), (1, 1, 0.72, 0.42)),
                      ((0, 0.9), (0.4, 0.55), (1, 0)), ((0, 0.6), (0.4, 0.3), (1, 0)), ((0, 0.9), (1, 1.1)),
                      ((0, 0.6), (1, 0.8)), tongue_width, ((0, 0.7), (1, 1)), ((0, 0.45), (1, 0.7)), tongue_length,
                      {'velocityX': (((0, -0.5), (1, -0.5)), 50), 'velocityXRand': (((0, -0.42), (1, -0.42)), None)}),
            'gfx_starburst_1p'),
    ]
    if sparks_max:
        # Sparks flung forward, trailing short streaks, a few each shot.
        elements.append(_element('Tail', (
            ('name', 'sparks'), ('flags', 'useRandomAlpha useRandomSize useRandomLength useRandomVelocity relative'),
            ('count', f'{sparks_min} {sparks_max}'), ('life', '70 140'), ('origin', '1 0 0'),
            ('gravity', '0.2 0.4'), ('sequenceFixedFrameValue', '3'), ('sequenceFixedFpsValue', '0.000000')),
            _channels(((0, 1, 0.9, 0.6), (1, 1, 0.55, 0.2)), ((0, 1, 0.9, 0.6), (1, 1, 0.55, 0.2)),
                      ((0, 1), (0.6, 0.7), (1, 0)), ((0, 0.6), (0.6, 0.35), (1, 0)), _ONE, ((0, 0.6), (1, 0.6)), 0.4,
                      _ONE, ((0, 0.5), (1, 0.5)), 4,
                      {'velocityX': (((0, 1), (1, 1)), spark_speed), 'velocityY': (((0, -0.15), (1, -0.15)), spark_speed),
                       'velocityZ': (((0, -0.12), (1, -0.12)), spark_speed),
                       'velocityXRand': (((0, 0.6), (1, 0.6)), None), 'velocityYRand': (((0, 0.15), (1, 0.15)), None),
                       'velocityZRand': (((0, 0.15), (1, 0.15)), None)}),
            'gfx_spark_atlas'))
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


def muzzle_flash_assets(read):
    """Every rewritten muzzle flash, from read(name) giving an original file's bytes."""
    return {MUZZLE_FLASHES + name: rewrite_flash(read(MUZZLE_FLASHES + name).decode('latin-1'), family)
            for name, family in EFFECTS.items()}
