"""The browser's placed explosives, CoD WWII's S-Mine 44 and CoD4's C4.

They are generated from the owner's original files when the private archives
are prepared; the repository holds none of that data:
- weapons/mp/mine_mp and c4_mp edit the American frag's file;
- their viewmodels keep the Mk 2 grenade's skeleton and hand: C4 holds CoD2's
  sticky bomb, the S-Mine an original model (browser_smine.py) whose pin the
  Mk 2's pull-pin animation draws before the hand sets it down;
- the S-Mine's world model, texture and HUD icon are original too;
- their new sounds start from retail aliases, which the owner's sound pack
  may replace like the originals.
"""
import math
import struct

import browser_smine

FRAG = 'weapons/mp/frag_grenade_american_mp'
MK2_MODEL = 'xmodel/viewmodel_mk2'
MK2_PARTS = 'xmodelparts/viewmodel_mk22'
MK2_PULLPIN = 'xanim/viewmodel_mk2_pullpin'
MK2_MATERIAL = 'materials/mtl_weapon_mk2fraggrenade_high'
STICKY_MODEL = 'xmodel/weapon_stickybomb'
STICKY_SURFS = 'xmodelsurfs/weapon_sticky_bomb0'
STICKY_PARTS = 'xmodelparts/weapon_sticky_bomb0'
# The binoculars' hold at the eyes moves the same j_gun: lowered by the C4's
# gunOfs keys, it shows the charge in the right hand below the crosshair.
HOLD_ANIM = 'viewmodel_binoculars_idle'
ICON_TEMPLATE = 'materials/inventory_stickybomb'
MATERIAL = 'mtl_sticky_bomb'
SMINE_MATERIAL = 'mtl_browser_smine'
SMINE_IMAGE = 'browser_smine'
SMINE_PULLPIN = 'viewmodel_smine_pullpin'
SMINE_PLANT = 'viewmodel_smine_plant'
MINE_ICON = 'browser_mine_icon'
C4_ICON = 'inventory_stickybomb'
WEAPONS = ('mine_mp', 'c4_mp')
# Original files the asset closure must carry for them.
DEPENDENCIES = (STICKY_MODEL, ICON_TEMPLATE, 'materials/' + MATERIAL, MK2_MODEL, 'xanim/' + HOLD_ANIM,
                MK2_PULLPIN, MK2_MATERIAL)

# Each new alias starts from the rows of the retail alias it names.
ALIASES = {
    'weap_mine_pullout': 'weap_fraggrenade_pin',
    'weap_mine_throw': 'weap_fraggrenade_fire',
    'weap_mine_land': 'grenade_bounce_default',
    'weap_mine_arm': 'weap_fraggrenade_pin',
    'weap_c4_pullout': 'weap_fraggrenade_pin',
    'weap_c4_throw': 'weap_fraggrenade_fire',
    'weap_c4_land': 'grenade_bounce_default',
    'weap_c4_detonate': 'weap_fraggrenade_pin',
}
ALIAS_TABLE = 'soundaliases/browser_explosives.csv'
STRINGS = 'localizedstrings/browser.str'
STRING_TEXT = (
    'VERSION             "1"\r\n'
    'CONFIG              "browser"\r\n'
    'FILENOTES           "The browser port\'s own strings."\r\n'
    '\r\n'
    'REFERENCE           LANDMINE\r\n'
    'LANG_ENGLISH        "S-Mine 44"\r\n'
    '\r\n'
    'REFERENCE           LANDMINE_DESCRIPTION\r\n'
    'LANG_ENGLISH        "Set down at your feet, it arms and goes off when an enemy steps near. Your squad '
    'sees its emblem over it; hold Use over yours to take it back."\r\n'
    '\r\n'
    'REFERENCE           C4\r\n'
    'LANG_ENGLISH        "C4"\r\n'
    '\r\n'
    'REFERENCE           C4_DESCRIPTION\r\n'
    'LANG_ENGLISH        "Fire throws a charge that sticks where it lands. Aim sets off every charge you '
    'placed, wherever you are."\r\n'
    '\r\n'
    'ENDMARKER\r\n'
).encode('ascii')

_BOUNCE_SURFACES = ('Default', 'Bark', 'Brick', 'Carpet', 'Cloth', 'Concrete', 'Dirt', 'Flesh', 'Foliage', 'Glass',
                    'Grass', 'Gravel', 'Ice', 'Metal', 'Mud', 'Paper', 'Plaster', 'Rock', 'Sand', 'Snow', 'Water',
                    'Wood', 'Asphalt')


def _bounces(parallel, perpendicular):
    values = {}
    for surface in _BOUNCE_SURFACES:
        values[f'parallel{surface}Bounce'] = parallel
        values[f'perpendicular{surface}Bounce'] = perpendicular
    return values


# The S-Mine 44 is set down upright at its owner's feet; it arms after
# 1.2 s, then a foot within 40 units (a metre) clicks it and it goes off
# 0.15 s later, with the retail minefields' blast over a grenade's. The hand
# takes 0.85 s to bring it up and draw its pin, and sets it down 0.3 s after.
# CoD4's C4 sticks where it first hits and waits for its detonator.
MINE = {
    'displayName': 'BROWSER_LANDMINE', 'gunModel': 'viewmodel_smine',
    'worldModel': 'xmodel/weapon_smine', 'projectileModel': 'xmodel/weapon_smine',
    'holdFireAnim': SMINE_PULLPIN, 'fireAnim': SMINE_PLANT, 'lastShotAnim': SMINE_PLANT,
    'holdFireTime': '0.85', 'fireTime': '0.45', 'fireDelay': '0.3',
    'hudIcon': MINE_ICON, 'killIcon': MINE_ICON, 'pullbackSound': 'weap_mine_pullout',
    'fireSound': 'weap_mine_throw', 'ammoName': 'browser_landmine', 'clipName': 'browser_landmine',
    'sharedAmmoCapName': 'BROWSER_LANDMINE', 'maxAmmo': '2', 'startAmmo': '2', 'clipSize': '2',
    'sharedAmmoCap': '2', 'projectileSpeed': '0', 'projectileSpeedUp': '0', 'explosionRadius': '192',
    'explosionInnerDamage': '300', 'explosionOuterDamage': '60', 'projExplosionSound': 'explo_mine',
    **_bounces('0.1', '0.05'),
}
MINE_KEYS = {
    'proximityRadius': '40', 'proximityArmTime': '1.2', 'proximityDelay': '0.15',
    'bounceSound': 'weap_mine_land', 'armSound': 'weap_mine_arm', 'triggerSound': 'minefield_click',
}
C4 = {
    'displayName': 'BROWSER_C4', 'weaponClass': 'item', 'offhandClass': 'None', 'gunModel': 'viewmodel_c4',
    'worldModel': STICKY_MODEL, 'projectileModel': STICKY_MODEL, 'hudIcon': C4_ICON, 'killIcon': C4_ICON,
    'pullbackSound': 'weap_c4_pullout', 'fireSound': 'weap_c4_throw', 'ammoName': 'browser_c4',
    'clipName': 'browser_c4', 'sharedAmmoCapName': 'BROWSER_C4', 'maxAmmo': '2', 'startAmmo': '2',
    'clipSize': '2', 'sharedAmmoCap': '2', 'projectileSpeed': '520', 'projectileSpeedUp': '140',
    'holdFireTime': '0.35', 'explosionRadius': '256', 'explosionInnerDamage': '330',
    'explosionOuterDamage': '70', 'projExplosionSound': 'explo_mine', **_bounces('0', '0'),
    # emptyIdleAnim stays the frag's: with no charge left the hand is out of sight.
    **{key: HOLD_ANIM for key in ('idleAnim', 'meleeAnim', 'raiseAnim', 'dropAnim', 'altRaiseAnim', 'altDropAnim')},
}
C4_KEYS = {'hasDetonator': '1', 'stickiness': 'all', 'detonateSound': 'weap_c4_detonate', 'bounceSound': 'weap_c4_land',
           'gunOfsF': '1', 'gunOfsR': '3.5', 'gunOfsU': '-8.5'}


def weapon_pairs(data):
    if not data.startswith(b'WEAPONFILE\\'):
        raise ValueError('Not a weapon file')
    fields = data[len(b'WEAPONFILE\\'):].decode('latin-1').split('\\')
    if len(fields) % 2:
        raise ValueError('Odd weapon file fields')
    return list(zip(fields[0::2], fields[1::2]))


def edit_weapon(data, values, added):
    """The weapon file with values replacing its own and added keys after them."""
    pairs = weapon_pairs(data)
    names = {key for key, _ in pairs}
    missing = sorted(set(values) - names)
    if missing:
        raise ValueError('The frag file lacks ' + ', '.join(missing))
    if set(added) & names:
        raise ValueError('The frag file already has ' + ', '.join(sorted(set(added) & names)))
    pairs = [(key, values.get(key, value)) for key, value in pairs] + list(added.items())
    return ('WEAPONFILE\\' + '\\'.join(f'{key}\\{value}' for key, value in pairs)).encode('latin-1')


def _cstring(data, position):
    end = data.index(b'\0', position)
    return data[position:end].decode('latin-1'), end + 1


def read_xmodel(data):
    """An xmodel's LOD names, materials per LOD and bone bounds."""
    if struct.unpack_from('<h', data, 0)[0] != 20:
        raise ValueError('Unexpected xmodel version')
    position = 27
    lods = []
    for _ in range(4):
        distance, = struct.unpack_from('<f', data, position)
        name, position = _cstring(data, position + 4)
        lods.append((distance, name))
    coll_lod, collision = struct.unpack_from('<ii', data, position)
    if collision:
        raise ValueError('Collision surfaces are not expected on these models')
    position += 8
    materials = []
    for _, name in lods:
        if not name:
            continue
        count, = struct.unpack_from('<h', data, position)
        position += 2
        names = []
        for _ in range(count):
            material, position = _cstring(data, position)
            names.append(material)
        materials.append(names)
    bounds = [struct.unpack_from('<6f', data, offset) for offset in range(position, len(data), 24)]
    if position + 24 * len(bounds) != len(data):
        raise ValueError('Unexpected bone bounds')
    return {'lods': lods, 'collLod': coll_lod, 'materials': materials, 'bounds': bounds}


def write_xmodel(lod_name, materials, bounds, radius):
    data = struct.pack('<hB6f', 20, 0, -radius, -radius, -radius, radius, radius, radius)
    data += struct.pack('<f', 0.0) + lod_name.encode('ascii') + b'\0'
    data += (struct.pack('<f', 0.0) + b'\0') * 3
    data += struct.pack('<ii', -1, 0)
    data += struct.pack('<h', len(materials)) + b''.join(name.encode('ascii') + b'\0' for name in materials)
    return data + b''.join(struct.pack('<6f', *box) for box in bounds)


def bone_count(parts):
    parented, roots = struct.unpack_from('<hh', parts, 2)
    return parented + roots


def read_rigid_surfaces(data):
    """Each rigid surface's bone, vertices (normal, colour, uv, binormal,
    tangent, position) and triangle indices."""
    version, count = struct.unpack_from('<hh', data, 0)
    if version != 20:
        raise ValueError('Unexpected xmodelsurfs version')
    position, surfaces = 4, []
    for _ in range(count):
        _, vertex_count, triangle_count, bone = struct.unpack_from('<BHHh', data, position)
        position += 7
        if bone < 0:
            raise ValueError('Expected rigid surfaces')
        vertices = []
        for _ in range(vertex_count):
            normal = struct.unpack_from('<3f', data, position)
            colour = data[position + 12:position + 16]
            uv = struct.unpack_from('<2f', data, position + 16)
            binormal = struct.unpack_from('<3f', data, position + 24)
            tangent = struct.unpack_from('<3f', data, position + 36)
            point = struct.unpack_from('<3f', data, position + 48)
            vertices.append((normal, colour, uv, binormal, tangent, point))
            position += 60
        triangles = list(struct.unpack_from('<%dh' % (triangle_count * 3), data, position))
        position += triangle_count * 6
        surfaces.append((bone, vertices, triangles))
    if position != len(data):
        raise ValueError('Unexpected xmodelsurfs size')
    return surfaces


def write_rigid_surfaces(surfaces):
    data = struct.pack('<hh', 20, len(surfaces))
    for bone, vertices, triangles in surfaces:
        data += struct.pack('<BHHh', 0, len(vertices), len(triangles) // 3, bone)
        for normal, colour, uv, binormal, tangent, point in vertices:
            data += (struct.pack('<3f', *normal) + bytes(colour) + struct.pack('<2f', *uv) +
                     struct.pack('<3f', *binormal) + struct.pack('<3f', *tangent) + struct.pack('<3f', *point))
        data += struct.pack('<%dh' % len(triangles), *triangles)
    return data


def _transform(vertices, rotation, scale, offset):
    def turn(vector):
        return tuple(sum(rotation[row][column] * vector[column] for column in range(3)) for row in range(3))
    return [(turn(normal), colour, uv, turn(binormal), turn(tangent),
             tuple(scale * value + offset[axis] for axis, value in enumerate(turn(point))))
            for normal, colour, uv, binormal, tangent, point in vertices]


def _bounds(vertices):
    points = [vertex[5] for vertex in vertices]
    return (tuple(min(point[axis] for point in points) for axis in range(3)) +
            tuple(max(point[axis] for point in points) for axis in range(3)))


def _radius(vertices):
    return math.ceil(max(math.sqrt(sum(value * value for value in vertex[5])) for vertex in vertices) * 1000) / 1000


def _centred(vertices, rotation, scale):
    box = _bounds(_transform(vertices, rotation, 1.0, (0.0, 0.0, 0.0)))
    centre = tuple(-scale * (box[axis] + box[axis + 3]) * 0.5 for axis in range(3))
    return _transform(vertices, rotation, scale, centre)


def _parts_copy(parts):
    if struct.unpack_from('<h', parts, 0)[0] != 20:
        raise ValueError('Unexpected xmodelparts version')
    return parts


def mine_icon_pixels(size=64):
    """The original HUD icon: the S-Mine standing, its canister and igniter in
    the HUD's white with grey shading, a dark edge, 4 x 4 samples a texel."""
    pixels = []
    for y in range(size):
        row = []
        for x in range(size):
            totals = [0.0, 0.0, 0.0, 0.0]
            for sy in range(4):
                for sx in range(4):
                    # A margin of an eighth around the mine.
                    px = ((x + (sx + 0.5) / 4) / size * 2.0 - 1.0) * 1.14
                    py = ((y + (sy + 0.5) / 4) / size * 2.0 - 1.0) * 1.14
                    colour = _mine_icon_sample(px, py)
                    if colour:
                        for channel in range(3):
                            totals[channel] += colour[channel] * colour[3]
                        totals[3] += colour[3]
            alpha = totals[3] / 16.0
            if alpha <= 0.0:
                row.append((0, 0, 0, 0))
                continue
            row.append(tuple(int(round(totals[channel] / totals[3] * 255)) for channel in range(3)) +
                       (int(round(alpha * 255)),))
        pixels.append(row)
    return pixels


def _segment_distance(px, py, ax, ay, bx, by):
    dx, dy = bx - ax, by - ay
    t = max(0.0, min(1.0, ((px - ax) * dx + (py - ay) * dy) / (dx * dx + dy * dy)))
    return math.hypot(px - ax - t * dx, py - ay - t * dy)


def _mine_icon_sample(px, py):
    # y down: three prongs over the stem, collar and tube, then the canister
    # with its rim, a darker band near the foot and a lit left side.
    edge = (0.3, 0.3, 0.29, 1.0)
    for tip in ((-0.24, -0.93), (0.0, -0.96), (0.24, -0.93)):
        distance = _segment_distance(px, py, 0.0, -0.6, tip[0], tip[1])
        if distance <= 0.045:
            return (0.95, 0.95, 0.92, 1.0)
        if distance <= 0.075:
            return edge
    parts = ((0.11, -0.66, -0.56), (0.13, -0.56, -0.38), (0.17, -0.38, -0.26), (0.11, -0.26, -0.06),
             (0.2, -0.06, 0.02))
    for half, top, bottom in parts:
        if abs(px) <= half and top <= py <= bottom:
            if abs(px) > half - 0.035:
                return edge
            return (0.86, 0.86, 0.82, 1.0) if px < 0 else (0.74, 0.74, 0.7, 1.0)
    if abs(px) <= 0.52 and 0.02 <= py <= 0.92:
        corner = max(0.0, abs(px) - 0.44) ** 2 + max(0.0, py - 0.84) ** 2
        if corner > 0.08 ** 2:
            return None
        if abs(px) > 0.485 or py > 0.885 or corner > 0.05 ** 2 or py < 0.06:
            return edge
        if 0.07 <= py <= 0.13:
            return (0.6, 0.6, 0.57, 1.0)
        shade = 0.98 - 0.25 * (px + 0.52) / 1.04
        if 0.66 <= py <= 0.76:
            shade *= 0.72
        return (shade, shade, shade * 0.96, 1.0)
    return None


def write_iwi_argb(pixels):
    """An uncompressed ARGB32 IWI with its mip chain, smallest level first."""
    levels = [pixels]
    while len(levels[-1]) > 1:
        source = levels[-1]
        half = len(source) // 2
        levels.append([[tuple((sum(source[2 * y + dy][2 * x + dx][channel] for dy in (0, 1) for dx in (0, 1)) + 2) // 4
                              for channel in range(4)) for x in range(half)] for y in range(half)])
    blobs = [b''.join(bytes((pixel[2], pixel[1], pixel[0], pixel[3])) for row in level for pixel in row)
             for level in levels]
    total = 28 + sum(len(blob) for blob in blobs)
    ends = [total]
    for blob in blobs[:3]:
        ends.append(ends[-1] - len(blob))
    size = len(pixels)
    return (b'IWi' + bytes((5, 1, 1)) + struct.pack('<HHH', size, size, 1) + struct.pack('<4i', *ends) +
            b''.join(reversed(blobs)))


def write_icon_material(template, name):
    """A 2D material for the image of its name, laid out as the template's."""
    head = bytearray(template[:0x50])
    technique, textures, table = struct.unpack_from('<III', template, 0x38)
    if template[technique:technique + 3] != b'2d\0' or textures != 0x44 or struct.unpack_from('<I', template, 0x34)[0] != 1:
        raise ValueError('Unexpected icon material template')
    strings = b'2d\0'
    material = 0x50 + len(strings)
    strings += name.encode('ascii') + b'\0'
    image = 0x50 + len(strings)
    strings += name.encode('ascii') + b'\0'
    colour_map = 0x50 + len(strings)
    strings += b'colorMap\0'
    struct.pack_into('<I', head, 0x00, material)
    struct.pack_into('<I', head, 0x04, image)
    struct.pack_into('<I', head, 0x38, 0x50)
    struct.pack_into('<I', head, 0x40, 0x50)
    struct.pack_into('<III', head, 0x44, colour_map, struct.unpack_from('<I', template, 0x48)[0], image)
    struct.pack_into('<HH', head, 0x1c, 64, 64)
    return bytes(head) + strings


# Viewmodel space: x away from the eye, y left, z up.
_IDENTITY = ((1, 0, 0), (0, 1, 0), (0, 0, 1))
# The sticky bomb's wide side toward the eye.
_FACE_EYE = ((0, -1, 0), (1, 0, 0), (0, 0, 1))


def explosive_assets(read):
    """Every generated entry, from read(name) giving an original file's bytes."""
    frag = read(FRAG)
    mk2 = read_xmodel(read(MK2_MODEL))
    mk2_parts = _parts_copy(read(MK2_PARTS))
    if mk2['lods'][0][1] != 'viewmodel_mk22' or len(mk2['bounds']) != bone_count(mk2_parts):
        raise ValueError('Unexpected Mk 2 viewmodel')
    sticky = read_rigid_surfaces(read(STICKY_SURFS))
    if len(sticky) != 1 or read_xmodel(read(STICKY_MODEL))['materials'][0] != [MATERIAL]:
        raise ValueError('Unexpected sticky bomb')
    _, sticky_vertices, sticky_triangles = sticky[0]

    # The bag without its trailing cord (everything below x = -3).
    kept = [i for i in range(0, len(sticky_triangles), 3)
            if all(sticky_vertices[index][5][0] > -3.0 for index in sticky_triangles[i:i + 3])]
    order = sorted({sticky_triangles[i + k] for i in kept for k in range(3)})
    remap = {old: new for new, old in enumerate(order)}
    bag = [sticky_vertices[index] for index in order]
    bag_triangles = [remap[sticky_triangles[i + k]] for i in kept for k in range(3)]
    c4_vertices = _centred(bag, _FACE_EYE, 0.55)

    def viewmodel(name, material, surfaces):
        bounds = list(mk2['bounds'])
        for bone, vertices, _ in surfaces:
            bounds[bone] = _bounds(vertices)
        everything = [vertex for _, vertices, _ in surfaces for vertex in vertices]
        return {
            'xmodel/' + name: write_xmodel(name + '0', [material] * len(surfaces), bounds, _radius(everything)),
            'xmodelparts/' + name + '0': mk2_parts,
            'xmodelsurfs/' + name + '0': write_rigid_surfaces(surfaces),
        }

    sticky_parts = _parts_copy(read(STICKY_PARTS))
    if bone_count(sticky_parts) != 1:
        raise ValueError('Unexpected sticky bomb skeleton')
    mine = browser_smine.body()
    held = smine_viewmodel_parts(browser_smine.read_xanim(read(MK2_PULLPIN)))
    assets = {
        'weapons/mp/mine_mp': edit_weapon(frag, MINE, MINE_KEYS),
        'weapons/mp/c4_mp': edit_weapon(frag, C4, C4_KEYS),
        'xmodel/weapon_smine': write_xmodel('weapon_smine0', [SMINE_MATERIAL], [_bounds(mine.vertices)],
                                            _radius(mine.vertices)),
        'xmodelparts/weapon_smine0': sticky_parts,
        'xmodelsurfs/weapon_smine0': write_rigid_surfaces([(0, mine.vertices, mine.triangles)]),
        'xanim/' + SMINE_PULLPIN: browser_smine.write_xanim(held['pullpin']),
        'xanim/' + SMINE_PLANT: browser_smine.write_xanim(held['plant']),
        'images/' + SMINE_IMAGE + '.iwi': write_iwi_argb(browser_smine.texture_pixels()),
        'materials/' + SMINE_MATERIAL: browser_smine.write_model_material(read(MK2_MATERIAL), SMINE_MATERIAL,
                                                                          SMINE_IMAGE),
        'images/' + MINE_ICON + '.iwi': write_iwi_argb(mine_icon_pixels()),
        'materials/' + MINE_ICON: write_icon_material(read(ICON_TEMPLATE), MINE_ICON),
        STRINGS: STRING_TEXT,
    }
    assets.update(viewmodel('viewmodel_c4', MATERIAL, [(0, c4_vertices, bag_triangles)]))
    assets.update(viewmodel('viewmodel_smine', SMINE_MATERIAL, [(0, held['body'].vertices, held['body'].triangles),
                                                                 (1, held['pin'].vertices, held['pin'].triangles)]))
    return assets


def alias_rows(header, rows_of):
    """The new aliases' rows: each source alias's rows under the new name, for
    the table header given; rows_of(alias) lists the source's field dicts."""
    rows = []
    for alias, source in ALIASES.items():
        fields = rows_of(source)
        if not fields:
            raise ValueError(f'No original {source} alias for {alias}')
        for row in fields:
            rows.append([alias if column == 'name' else row.get(column, '') for column in header])
    return rows


def smine_viewmodel_parts(mk2_pullpin, **overrides):
    params = dict(SMINE_VIEW)
    params.update(overrides)
    return browser_smine.viewmodel(mk2_pullpin, **params)


# Where the S-Mine sits in the Mk 2's hand and where the hand holds it up.
SMINE_VIEW = {
    'scale': 0.7, 'centre': (0.27, 0.51, -0.9), 'twist': 0.0, 'pull': (-0.31, 0.94, 0.0),
    'hold_centre': (10.5, -5.8, -5.1), 'hold_axis': (-0.1, 0.0, 1.0), 'hold_twist': 0.0,
    'drop': (6.0, 5.0, -24.0),
}
