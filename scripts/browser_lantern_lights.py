"""Warm local illumination for the existing, map-authored lantern emitters."""
from browser_grenade_explosions import _element, _channels, _constant

EFFECT = 'fx/props/glow_latern.efx'


def lantern_light_assets(read):
    # _load.gsc repeats this effect every 300 ms. Two complementary 600 ms
    # envelopes overlap without a dark gap or a doubled peak. Random amplitude
    # varies gently between consecutive flames, rather than flashing on/off.
    warm = (0.84, 0.44, 0.16)
    def envelope(scale):
        return ((0, 0, 0, 0), (0.5, *(v * scale for v in warm)), (1, 0, 0, 0))
    light = _element('Light', (
        ('name', 'browser_lantern_light'), ('flags', 'useRandomColors'),
        ('life', '600'), ('spawnRange', '0 800'), ('cullrange', '800'),
        ('origin', '2 0 0'),
    ), _channels(rgb=envelope(0.94), rgb_rand=envelope(1.06),
                 size=_constant(1), size_scale=144))
    original = read(EFFECT)
    return {EFFECT: original + b'\r\n' + light.replace('\n', '\r\n').encode('ascii')}
