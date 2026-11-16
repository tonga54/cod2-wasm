"""Small, colliding snow particles for the client's outdoor winter emitters."""
from browser_grenade_explosions import _element, _channels, _constant, write_iwi_rgba, feathered_material

EFFECT = 'fx/misc/browser_snowfall.efx'


def weather_assets(read):
    name = 'browser_snowflake'
    size = 32
    pixels = bytearray()
    for y in range(size):
        for x in range(size):
            u, v = (x+.5)/16-1, (y+.5)/16-1
            r2 = u*u+v*v
            # Small soft flecks, not a screen overlay or giant cloud sheets.
            alpha = max(0, 1-r2*1.4)**2
            pixels.extend((239, 246, 255, round(alpha*255)))
    effect = _element('Particle', [
        ('name', 'falling_snow'),
        ('flags', 'usePhysics impactKills useAlpha useRandomSize useRandomVelocity absoluteVel'),
        ('spawnFlags', 'evenDistribution'), ('count', '5'), ('life', '4500 6500'),
        ('origin', '-18 -18 -32 18 18 32'), ('rotation', '-180 180'),
        ('spawnRange', '0 1400'), ('gravity', '0'),
    ], _channels(rgb=((0,.88,.94,1),(1,.88,.94,1)),
        alpha=((0,0),(.08,.85),(.8,.85),(1,0)),
        size=_constant(.9), size_rand=_constant(1.8), size_scale=1,
        velocity=(_constant(7), _constant(2), _constant(-68)),
        velocity_rand=(_constant(16), _constant(-5), _constant(-92)), velocity_scale=1),
        shaders=[name]).encode()
    return {EFFECT: effect,
            'materials/'+name: feathered_material(read('materials/gfx_snow_flakes'), name, size, size, 1, 1),
            'images/'+name+'.iwi': write_iwi_rgba(size,size,pixels)}
