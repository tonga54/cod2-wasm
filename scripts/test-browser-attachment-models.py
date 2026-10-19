#!/usr/bin/env python3
"""The attachment model generator (browser_attachment_models.py) on made-up guns, without the owner's files:
which models each class weapon gets, the bone each rides, the sights centred on the line of sight of the
aiming animation's last frame with their glass only in the pictures, the 4x Optics from the scoped rifles'
own scopes, the WWII can along the muzzle with its sleeve and below the line of sight, the two clamped
magazines, the longer one and a rifle's box magazine, the gun without its own, the retail winding, unit normals,
floats that round the same on every machine, and the finishes' textures and materials."""
import math
import os
import shutil
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import browser_attachment_models as A
import browser_explosives as E
import browser_smine as S

files = {}


def write_parts(free, parented):
    """xmodelparts: free bones, then (name, parent, translation, quaternion) children."""
    data = struct.pack('<hhh', 20, len(parented), len(free))
    for _, parent, translation, quat in parented:
        data += bytes([parent]) + struct.pack('<3f', *translation)
        data += struct.pack('<3h', *(int(round(v * 32767)) for v in quat[:3]))
    names = free + [bone[0] for bone in parented]
    return data + b''.join(name.encode() + b'\0' for name in names) + bytes(len(names))


def about_y(degrees):
    half = math.radians(degrees) / 2
    return (0.0, math.sin(half), 0.0, math.cos(half))


def boxes(along, low, high, step=1.0, mesh=None):
    """Boxes side by side along x, so the gun has vertices all along its top."""
    mesh = mesh or A.Mesh('blued')
    x = along[0]
    while x < along[1] - 1e-9:
        A.box(mesh, (x, low[0], low[1]), (min(x + step, along[1]), high[0], high[1]))
        x += step
    return mesh


def piece(*meshes):
    """Meshes as one surface's vertices and triangles."""
    vertices, triangles = [], []
    for mesh in meshes:
        start = len(vertices)
        vertices += mesh.rigid_vertices()
        triangles += [start + v for v in mesh.triangles]
    return vertices, triangles


# Made-up guns: a receiver and barrel on j_gun, a box magazine on j_clip, the muzzle at tag_flash.
GUN_BONES = ['j_gun', 'j_bolt', 'j_clip', 'tag_brass', 'tag_flash']
receiver = piece(boxes((-6.0, 9.0), (-0.5, -0.5), (0.5, 1.2)))
barrel = piece(boxes((9.0, 16.0), (-0.3, 1.1), (0.3, 1.7)))
magazine = A.box(A.Mesh('blued'), (-0.4, -0.35, -5.0), (0.4, 0.35, 0.0))
GUN_MATERIALS = ['mtl_test_gun', 'mtl_test_barrel', 'mtl_test_mag']
GUN_SURFACES = [(0, *receiver), (0, *barrel), (2, magazine.rigid_vertices(), magazine.triangles)]
GUN_PARTS = write_parts(['j_gun'], [(name, 0, (0.0, 0.0, 0.0), (0.0, 0.0, 0.0, 1.0)) for name in GUN_BONES[1:]])
# A scope over the receiver, on two legs down to it, as the scoped rifles carry theirs.
SCOPE = A.box(A.Mesh('blued'), (-3.0, -0.4, 2.3), (4.0, 0.4, 3.0))
LEGS = [A.box(A.Mesh('blued'), (x, -0.2, 1.2), (x + 0.5, 0.2, 2.3)) for x in (-1.5, 2.0)]
scope = piece(SCOPE, *LEGS)
# The Springfield's scope is in the rifle's own material: what stands over its receiver and behind its front
# sight, the post at the muzzle left out.
springfield = piece(boxes((-6.0, 9.0), (-0.5, -0.5), (0.5, 1.2)), SCOPE,
                    A.box(A.Mesh('blued'), (12.0, -0.1, 2.3), (12.4, 0.1, 2.6)))
# The Bren: a cover standing up over the front of its receiver, its magazine standing over the middle.
cover = piece(boxes((-6.0, 9.0), (-0.5, -0.5), (0.5, 1.2)), A.box(A.Mesh('blued'), (2.9, -0.4, 1.2),
                                                                  (3.6, 0.4, 5.0)))
top_magazine = A.box(A.Mesh('blued'), (-1.9, -0.35, 0.4), (-1.1, 0.35, 6.4))
MODELS = {
    'viewmodel_test_smg': (GUN_MATERIALS, GUN_SURFACES),
    'viewmodel_test_pistol': (GUN_MATERIALS, GUN_SURFACES),
    'viewmodel_test_sniper': (GUN_MATERIALS + ['mtl_test_kar98_scope'], GUN_SURFACES + [(0, *scope)]),
    'viewmodel_test_kar98k': (GUN_MATERIALS, GUN_SURFACES),
    'viewmodel_test_g43': (GUN_MATERIALS, GUN_SURFACES),
    'viewmodel_test_springfield': (['mtl_test_springfield'] + GUN_MATERIALS[1:],
                                   [(0, *springfield)] + GUN_SURFACES[1:]),
    'viewmodel_test_garand': (GUN_MATERIALS, GUN_SURFACES),
    'viewmodel_test_enfield_scope': (GUN_MATERIALS + ['mtl_test_enfield_scope'], GUN_SURFACES + [(0, *scope)]),
    'viewmodel_test_bren': (GUN_MATERIALS, [(0, *cover), GUN_SURFACES[1],
                                            (2, top_magazine.rigid_vertices(), top_magazine.triangles)]),
    'viewmodel_test_shotgun': (GUN_MATERIALS, GUN_SURFACES),
}
for model, (materials, surfaces) in MODELS.items():
    files['xmodel/' + model] = E.write_xmodel(model + '0', materials, [(-1, -1, -1, 1, 1, 1)] * 5, 20.0)
    files['xmodelparts/' + model + '0'] = GUN_PARTS
    files['xmodelsurfs/' + model + '0'] = E.write_rigid_surfaces(surfaces)
# The hands carry the gun on tag_weapon; tag_view is the eye.
files['xmodel/viewmodel_hands_test'] = E.write_xmodel('viewmodel_hands_test0', ['mtl_hands'], [(-1, -1, -1, 1, 1, 1)] * 3, 9)
files['xmodelparts/viewmodel_hands_test0'] = write_parts(['tag_view'], [('tag_torso', 0, (0, 0, 0), (0, 0, 0, 1)),
                                                                       ('tag_weapon', 1, (0, 0, 0), (0, 0, 0, 1))])
IDLE_WEAPON = ((0.0, 0.0, 0.0, 1.0), (8.0, 0.0, -2.0))
files['xanim/viewmodel_test_idle'] = S.write_xanim({'fps': 30, 'frames': 1, 'bones': [
    ('tag_weapon', [IDLE_WEAPON[0]], [IDLE_WEAPON[1]]),
    ('j_clip', None, [(2.5, 0.0, -0.4)]),
    ('tag_flash', None, [(16.0, 0.0, 1.4)])]})
# The raise ends on its last frame, a step past the one before, as the engine now holds it. The pistol's
# brings the eye low over the bore, the G43's turns the line of sight from the sniper rifle's.
RAISES = {
    'viewmodel_test_ads_up': [(about_y(0.0), (0.0, 0.0, 0.0)), (about_y(1.0), (2.0, 0.6, -0.3)),
                              (about_y(-1.5), (3.6, 0.3, -0.5)), (about_y(-0.5), (4.0, 0.0, -0.7))],
    'viewmodel_test_ads_up_low': [(about_y(0.0), (0.0, 0.0, 0.0)), (about_y(0.0), (4.0, 0.0, 0.1))],
    'viewmodel_test_ads_up_turned': [(about_y(0.0), (0.0, 0.0, 0.0)), (about_y(-6.0), (4.0, 0.0, -0.9))],
}
for anim, frames in RAISES.items():
    files['xanim/' + anim] = S.write_xanim({'fps': 30, 'frames': len(frames), 'bones': [
        ('tag_torso', [q for q, _ in frames], [t for _, t in frames])]})
WEAPONS = {
    'thompson_mp': ('viewmodel_test_smg', 'viewmodel_test_ads_up'),
    'colt_mp': ('viewmodel_test_pistol', 'viewmodel_test_ads_up_low'),
    'kar98k_sniper_mp': ('viewmodel_test_sniper', 'viewmodel_test_ads_up'),
    'kar98k_mp': ('viewmodel_test_kar98k', 'viewmodel_test_ads_up'),
    'g43_mp': ('viewmodel_test_g43', 'viewmodel_test_ads_up_turned'),
    'springfield_mp': ('viewmodel_test_springfield', 'viewmodel_test_ads_up'),
    'm1garand_mp': ('viewmodel_test_garand', 'viewmodel_test_ads_up'),
    'enfield_scope_mp': ('viewmodel_test_enfield_scope', 'viewmodel_test_ads_up'),
    'bren_mp': ('viewmodel_test_bren', 'viewmodel_test_ads_up'),
    'shotgun_mp': ('viewmodel_test_shotgun', 'viewmodel_test_ads_up'),
}
for weapon, (model, raise_anim) in WEAPONS.items():
    files['weapons/mp/' + weapon] = ('WEAPONFILE\\gunModel\\%s\\handModel\\viewmodel_hands_test\\idleAnim\\'
                                     'viewmodel_test_idle\\adsUpAnim\\%s' % (model, raise_anim)).encode()
# The Mk 2's material layout, as test-browser-smine.py makes it up.
template = bytearray(0x5c)
struct.pack_into('<4B', template, 0x0c, 3, 3, 1, 1)
struct.pack_into('<I', template, 0x2c, 0x28128812)
struct.pack_into('<IIII', template, 0x30, 5, 2, 0x5c, 0x44)
struct.pack_into('<I', template, 0x40, 0x5c)
strings = b'phong_replace\0mtl_weapon_mk2fraggrenade_high\0mkfrag\0colorMap\0normalMap\0$identitynormalmap\0'
offsets = {name: 0x5c + strings.index(name.encode() + b'\0') for name in
           ('phong_replace', 'mtl_weapon_mk2fraggrenade_high', 'mkfrag', 'colorMap', 'normalMap', '$identitynormalmap')}
struct.pack_into('<II', template, 0, offsets['mtl_weapon_mk2fraggrenade_high'], offsets['mkfrag'])
struct.pack_into('<III', template, 0x44, offsets['colorMap'], 0x200, offsets['mkfrag'])
struct.pack_into('<III', template, 0x50, offsets['normalMap'], 0x300, offsets['$identitynormalmap'])
files[E.MK2_MATERIAL] = bytes(template) + strings

reads = []


def read(name):
    reads.append(name)
    return files[name]


CLASSES = {'primary': ['thompson_mp', 'kar98k_sniper_mp', 'kar98k_mp', 'g43_mp', 'm1garand_mp', 'bren_mp',
                       'shotgun_mp'],
           'secondary': ['colt_mp'], 'weaponAttachments': {
               'thompson_mp': ['grip', 'suppressor', 'reflex', 'lens', 'optic4x', 'extmag'],
               'kar98k_sniper_mp': ['optic4x', 'suppressor'], 'kar98k_mp': ['optic4x', 'extmag'],
               'g43_mp': ['optic4x', 'extmag'], 'm1garand_mp': ['optic4x'], 'bren_mp': ['optic4x'],
               'shotgun_mp': ['extmag'], 'colt_mp': ['suppressor', 'extmag', 'fmj']}}
entries = A.attachment_model_assets(read, CLASSES)
cases = 0


# ---------------------------------------------------------------- what each weapon gets
def model_entries(model, attachment):
    lod = model + A.SUFFIX + attachment + '0'
    return {'xmodel/' + model + A.SUFFIX + attachment, 'xmodelparts/' + lod, 'xmodelsurfs/' + lod}


expected = set()
for finish in A.FINISHES:
    expected |= {'images/' + A.IMAGE + finish + '.iwi', 'materials/' + A.MATERIAL + finish}
# The SMG has no scope of its nation's to take, so no 4x Optic model; grip and FMJ show nothing.
for attachment in ('suppressor', 'reflex', 'lens', 'extmag', 'nomag'):
    expected |= model_entries('viewmodel_test_smg', attachment)
# A scoped rifle keeps its own scope; the rifles take the scoped rifles'; a pistol a can and a longer magazine.
expected |= model_entries('viewmodel_test_sniper', 'suppressor')
for model in ('viewmodel_test_kar98k', 'viewmodel_test_g43', 'viewmodel_test_garand', 'viewmodel_test_bren'):
    expected |= model_entries(model, 'optic4x')
for attachment in ('suppressor', 'extmag', 'nomag'):
    expected |= model_entries('viewmodel_test_pistol', attachment)
# A rifle with a magazine of its own has it longer; one loaded through the top keeps its stripper clip and has a
# box magazine; the trench gun's tube keeps its look.
expected |= model_entries('viewmodel_test_g43', 'extmag') | model_entries('viewmodel_test_g43', 'nomag')
expected |= model_entries('viewmodel_test_kar98k', A.EXTBOX)
assert set(entries) == expected, sorted(set(entries) ^ expected)
assert A.gun_attachments(read, 'thompson_mp', {'extmag'})[1] == ['extmag']
assert A.gun_attachments(read, 'thompson_mp', {'optic4x'}) == ({}, [])
assert A.gun_attachments(read, 'shotgun_mp', {'extmag'}) == ({}, [])
assert A.gun_attachments(read, 'kar98k_sniper_mp', {'optic4x'}) == ({}, [])
sniper_box = A.gun_attachments(read, 'kar98k_sniper_mp', {'optic4x', 'extmag'})
assert sniper_box[1] == ['extmag'] and set(sniper_box[0]) == model_entries('viewmodel_test_sniper', A.EXTBOX)
# The same inputs give the same bytes.
assert A.attachment_model_assets(read, CLASSES) == entries
cases += 1


# ---------------------------------------------------------------- each model's files
def surfaces_of(name):
    lod = A.E.read_xmodel(entries['xmodel/' + name])['lods'][0][1]
    assert lod == name + '0', lod
    return A.read_surfaces(entries['xmodelsurfs/' + lod]), A.E.read_xmodel(entries['xmodel/' + name])['materials'][0]


def wound_against_normals(surface):
    vertices = surface['vertices']
    for i in range(0, len(surface['triangles']), 3):
        a, b, c = (vertices[v] for v in surface['triangles'][i:i + 3])
        face = S._cross(S._sub(b['pos'], a['pos']), S._sub(c['pos'], a['pos']))
        assert S._dot(face, S._add(S._add(a['normal'], b['normal']), c['normal'])) <= 1e-9


def canonical_floats(vertex):
    for values in (vertex['normal'], vertex['uv'], vertex['binormal'], vertex['tangent'], vertex['pos']):
        for value in values:
            assert value == 0.0 or abs(value) >= 1e-9, value
            assert abs(value - round(value, 6)) <= 2e-7 * max(1.0, abs(value)), value


RETAIL = {'mtl_test_mag', 'mtl_test_kar98_scope', 'mtl_test_springfield', 'mtl_test_enfield_scope'}
for name in sorted(e[len('xmodel/'):] for e in entries if e.startswith('xmodel/') and A.NOMAG not in e):
    attachment = name.split(A.SUFFIX)[1]
    surfaces, materials = surfaces_of(name)
    # One bone, named after the one it rides, its surfaces rigid on it in the finishes' or the retail materials.
    names, parents, local = A.read_skeleton(entries['xmodelparts/' + name + '0'])
    assert names == ['att_' + dict(A.RIDES, **{A.EXTBOX: 'j_gun'})[attachment]] and parents == [-1], names
    assert materials and all(m in RETAIL or m.startswith(A.MATERIAL) and m[len(A.MATERIAL):] in A.FINISHES
                             for m in materials), materials
    assert len(materials) == len(surfaces)
    for surface in surfaces:
        assert surface['marker'] == 0 and surface['triangles'] and max(surface['triangles']) < len(surface['vertices'])
        wound_against_normals(surface)
        for vertex in surface['vertices']:
            assert abs(math.sqrt(S._dot(vertex['normal'], vertex['normal'])) - 1.0) < 1e-5
            assert vertex['colour'] == b'\xff\xff\xff\xff'
            canonical_floats(vertex)
cases += 1

# The canonical rounding the Pi and the build machine agree on.
assert A.canonical((1e-12, -3.7e-17, 0.12345649, -0.0, 2.0000004)) == (0.0, 0.0, 0.123456, 0.0, 2.0)
cases += 1


# ---------------------------------------------------------------- the line of sight
def rotate(q, v):
    x, y, z, w = q
    m = ((1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)),
         (2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)),
         (2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)))
    return tuple(sum(m[r][k] * v[k] for k in range(3)) for r in range(3))


def multiply(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by, aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw, aw * bw - ax * bx - ay * by - az * bz)


def eye_line(raise_anim='viewmodel_test_ads_up', frame=-1):
    """The eye and the view's forward axis in j_gun's space, the hands at tag_view and the gun on tag_weapon."""
    tq, tt = RAISES[raise_anim][frame]
    wq, wt = IDLE_WEAPON
    q = multiply(tq, wq)
    t = tuple(a + b for a, b in zip(tt, rotate(tq, wt)))
    inverse = (-q[0], -q[1], -q[2], q[3])
    eye = rotate(inverse, tuple(-v for v in t))
    return eye, rotate(inverse, (1.0, 0.0, 0.0))


def distance_to_line(point, origin, direction):
    offset = S._sub(point, origin)
    along = S._dot(offset, direction)
    return math.sqrt(max(0.0, S._dot(offset, offset) - along * along)), along


gun = A.Gun(read, 'thompson_mp')
eye, ahead = eye_line()
before, _ = eye_line(frame=-2)
# Within the anim's quaternions stored as shorts.
assert math.dist(gun.eye, eye) < 2e-3 and math.dist(gun.ahead, ahead) < 1e-4, (gun.eye, eye, gun.ahead, ahead)
assert math.dist(before, eye) > 0.3, 'the made-up raise must move the eye on its last frame'
# The reflex's window and the lens's tube and rim sit on that line, OPTIC_AHEAD in front of the eye.
reach = A.OPTIC_AHEAD['thompson_mp']
sights = {'reflex': A.reflex(gun), 'lens': A.lens(gun)}
for attachment, index, low, high, centre in (('reflex', 0, 0.64, 0.8, reach),
                                             ('lens', 0, 0.42, 0.5 * 1.05, reach + (0.35 - 0.45) / 2),
                                             ('lens', 1, 0.42 * 0.98, 0.5 * 1.12, reach + 0.35 + 0.085)):
    found = [distance_to_line(p, eye, ahead) for p in sights[attachment][index].positions]
    assert all(low - 2e-3 <= r <= high + 2e-3 for r, _ in found), (attachment, index, min(found), max(found))
    alongs = [a for _, a in found]
    assert abs((min(alongs) + max(alongs)) / 2 - centre) < 2e-3, (attachment, index, min(alongs), max(alongs))
# Their glass is there for the pictures, on the line, and out of the models: the eye sees the reticle.
for attachment, meshes in sights.items():
    glass = [m for m in meshes if m.picture_only]
    assert len(glass) == 2 and all(m.finish == 'glass' for m in glass), attachment
    assert all(distance_to_line(p, eye, ahead)[0] <= 0.64 + 2e-3 for m in glass for p in m.positions)
    assert A.MATERIAL + 'glass' not in surfaces_of('viewmodel_test_smg__' + attachment)[1], attachment
cases += 1


# ---------------------------------------------------------------- the 4x Optics: the scoped rifles' own scopes
def positions(name, material):
    surfaces, materials = surfaces_of(name)
    return [v['pos'] for v in surfaces[materials.index(material)]['vertices']]


source = A.read_surfaces(files['xmodelsurfs/viewmodel_test_sniper0'])[3]
# The Kar98k is the sniper rifle without its scope: the scope stays where it sits on that rifle.
kar98k, materials = surfaces_of('viewmodel_test_kar98k__optic4x')
assert materials == ['mtl_test_kar98_scope'] and len(kar98k) == 1
assert [v['pos'] for v in kar98k[0]['vertices']] == [v['pos'] for v in source['vertices']]
assert kar98k[0]['triangles'] == source['triangles']
# The G43 takes it along its own line of sight, turned from the sniper rifle's: the same shape, its long edges
# as far off that line as they were off the rifle's, its middle 3 ahead of the reflex's place and its legs on
# the receiver.
g43 = A.Gun(read, 'g43_mp')
sniper = A.Gun(read, 'kar98k_sniper_mp')
moved = positions('viewmodel_test_g43__optic4x', 'mtl_test_kar98_scope')
original = [v['pos'] for v in source['vertices']]
assert len(moved) == len(original)
for i in range(0, len(original), 7):
    for j in range(3, len(original), 11):
        assert abs(math.dist(moved[i], moved[j]) - math.dist(original[i], original[j])) < 1e-4
assert S._dot(g43.ahead, sniper.ahead) < math.cos(math.radians(2.0)), 'the made-up G43 must aim another way'
edge = original.index(max(original, key=lambda p: (p[0], -p[1], -p[2]))), original.index(
    min(original, key=lambda p: (p[0], p[1], p[2])))
for gun_, points in ((sniper, original), (g43, moved)):
    direction = S._normalise(S._sub(points[edge[0]], points[edge[1]]))
    if gun_ is sniper:
        off = S._dot(direction, sniper.ahead)
    else:
        assert abs(S._dot(direction, g43.ahead) - off) < 1e-4
alongs = [distance_to_line(p, g43.eye, g43.ahead)[1] for p in moved]
assert abs((min(alongs) + max(alongs)) / 2 - (A.OPTIC_AHEAD['g43_mp'] + 3.0)) < 0.05, (min(alongs), max(alongs))
assert abs(min(p[2] for p in moved) - 1.2) < 1e-3, min(p[2] for p in moved)
assert A.MATERIAL + 'parkerized' not in surfaces_of('viewmodel_test_g43__optic4x')[1]
# The M1 Garand takes the Springfield's, cut from the rifle's own material: over its receiver, not its front sight.
garand, materials = surfaces_of('viewmodel_test_garand__optic4x')
assert materials == ['mtl_test_springfield'] and len(garand[0]['triangles']) == len(SCOPE.triangles)
assert abs(min(v['pos'][2] for v in garand[0]['vertices']) - 1.2) < 1e-3
# The Bren's sits beside its magazine and under its cover's top, no higher than its iron sights' line, on a
# bracket down to its receiver.
bren = A.Gun(read, 'bren_mp')
at = S._add(bren.eye, S._scale(bren.ahead, A.OPTIC_AHEAD['bren_mp'] + 3.0))
seat = min(p[2] for p in positions('viewmodel_test_bren__optic4x', 'mtl_test_enfield_scope'))
assert abs(seat - (at[2] - 0.2)) < 1e-3, (seat, at)
bracket = positions('viewmodel_test_bren__optic4x', A.MATERIAL + 'parkerized')
assert abs(min(p[2] for p in bracket) - 1.1) < 1e-4 and abs(max(p[2] for p in bracket) - (seat + 0.05)) < 1e-3
assert abs(min(p[0] for p in bracket) - (at[0] - 0.7)) < 1e-4 and abs(max(p[0] for p in bracket) - (at[0] + 0.7)) < 1e-4
cases += 1


# ---------------------------------------------------------------- the can on the muzzle
def flash_space(gun_, point):
    return A._flash_space(gun_, point)


for weapon, model, kind in (('thompson_mp', 'viewmodel_test_smg', 'smg'), ('colt_mp', 'viewmodel_test_pistol', 'pistol')):
    length, radius = A.CAN[kind]
    gun_ = A.Gun(read, weapon)
    surfaces, materials = surfaces_of(model + '__suppressor')
    by = {m[len(A.MATERIAL):]: [v['pos'] for v in s['vertices']] for s, m in zip(surfaces, materials)}
    can = by['blued']
    xs = [p[0] for p in can]
    # tag_flash is the made-up muzzle, its axes j_gun's.
    back = 1.2 if kind == 'smg' else 0.6
    assert abs(max(xs) - length) < 1e-6 and abs(min(xs) + back) < 1e-6, (kind, min(xs), max(xs))
    # As fat as WWII's about its own axis, which is the bore or below it.
    centre_z = (max(p[2] for p in can) + min(p[2] for p in can)) / 2
    assert abs(max(p[1] for p in can) - radius) < 1e-5 and abs(min(p[1] for p in can) + radius) < 1e-5
    assert abs(max(p[2] for p in can) - min(p[2] for p in can) - 2 * radius) < 1e-5 and centre_z <= 1e-9
    # Its dark bore at the muzzle, on the barrel's axis and inside the can's face.
    bore = [p for p in by['black'] if p[0] > length]
    assert bore and all(abs(p[0] - length - 0.004) < 1e-6 for p in bore)
    hole = max(math.hypot(p[1], p[2]) for p in bore)
    assert hole > 0.1 and hole - centre_z <= radius - 0.08 + 1e-5, (kind, hole, centre_z)
    # Never into the line of sight, the eye looking along it to the front sight.
    eye_, tip = flash_space(gun_, gun_.eye), flash_space(gun_, S._add(gun_.eye, gun_.ahead))
    line = S._sub(tip, eye_)
    nearest = min(distance_to_line(p, eye_, line)[0] for points in by.values() for p in points)
    assert nearest > 0.05, (kind, nearest)
    if kind == 'smg':
        # WWII's rusted, perforated sleeve over the made-up barrel's bare 7 inches back to the receiver.
        sleeve = by['rust']
        assert abs(min(p[0] for p in sleeve) + 7.0) < 0.11 and abs(max(p[0] for p in sleeve) + back - 0.05) < 1e-6
        assert abs(max(p[1] for p in sleeve) - radius * 0.9) < 1e-5
        holes = [p for p in by['black'] if p[0] < 0]
        assert len(holes) >= 4 * 10 * 8, len(holes)
    else:
        # The pistol's sights run low over its bore: the can hangs below the bore, which nears its top.
        assert 'rust' not in by
        assert centre_z < -0.1, centre_z
cases += 1

# ---------------------------------------------------------------- the longer magazines and the gun without its own
# The SMG's, hanging under the gun: a second magazine on its left, lower and a little ahead, clamped to the first.
surfaces, materials = surfaces_of('viewmodel_test_smg__extmag')
assert A.read_skeleton(entries['xmodelparts/viewmodel_test_smg__extmag0'])[0] == ['att_j_clip']
assert materials == ['mtl_test_mag', 'mtl_test_mag'] + [A.MATERIAL + f for f in ('blued', 'parkerized', 'worn')], materials
original = A.read_surfaces(files['xmodelsurfs/viewmodel_test_smg0'])[2]['vertices']
shift = (A.DUAL_AHEAD * 0.8, 0.7 + 0.08, -A.DUAL_DROP * 5.0)
assert [v['pos'] for v in surfaces[0]['vertices']] == [v['pos'] for v in original]
for moved_vertex, vertex in zip(surfaces[1]['vertices'], original):
    assert math.dist(moved_vertex['pos'], S._add(vertex['pos'], shift)) < 1e-5
    assert moved_vertex['uv'] == vertex['uv']
assert surfaces[0]['triangles'] == surfaces[1]['triangles'] == A.read_surfaces(files['xmodelsurfs/viewmodel_test_smg0'])[2]['triangles']
for clamp in surfaces[2:4]:
    points = [v['pos'] for v in clamp['vertices']]
    assert min(p[1] for p in points) < -0.35 and max(p[1] for p in points) > 0.35 + 0.78
    assert min(p[0] for p in points) < -0.4 and max(p[0] for p in points) > 0.4 + shift[0]
bolts = [v['pos'] for v in surfaces[4]['vertices']]
assert len(bolts) == 2 * 24 and min(p[1] for p in bolts) >= 0.35 + 0.78
# The pistol's and the rifle's, the gun's own even hanging under the rifle: its well stays where it meets the gun;
# the rest is longer, by the centre line its slices find.
for model in ('viewmodel_test_pistol', 'viewmodel_test_g43'):
    surfaces, materials = surfaces_of(model + '__extmag')
    assert materials == ['mtl_test_mag'], (model, materials)
    points = [v['pos'] for s in surfaces for v in s['vertices']]
    assert abs(min(p[2] for p in points) + 5.0 * A.MAG_LONGER) < 0.01 * 5.0 * A.MAG_LONGER
    assert abs(max(p[2] for p in points)) < 1e-6
    assert abs(max(p[0] for p in points) - 0.4 * A.MAG_FATTER) < 1e-4
    assert abs(max(p[1] for p in points) - 0.35 * A.MAG_FATTER) < 1e-4
    assert sorted(v['uv'] for v in surfaces[0]['vertices']) == sorted(v['uv'] for v in original)
    assert len(surfaces[0]['triangles']) == len(magazine.triangles)
# The Kar98k's box magazine, on j_gun where BOX_MAGAZINES puts it: its body down out of the stock to its
# floorplate, a little wider, the ribs on its sides and the catch at its back.
back, front, top, bottom, width = A.BOX_MAGAZINES['kar98k_mp']
surfaces, materials = surfaces_of('viewmodel_test_kar98k__' + A.EXTBOX)
assert A.read_skeleton(entries['xmodelparts/viewmodel_test_kar98k__' + A.EXTBOX + '0'])[0] == ['att_j_gun']
assert materials == [A.MATERIAL + f for f in ('blued', 'parkerized', 'worn')], materials
by = {m[len(A.MATERIAL):]: [v['pos'] for v in s['vertices']] for s, m in zip(surfaces, materials)}
body = by['blued']
assert abs(min(p[0] for p in body) - back) < 1e-6 and abs(max(p[0] for p in body) - front) < 1e-6
assert abs(max(p[2] for p in body) - top) < 1e-6 and abs(min(p[2] for p in body) - bottom - 0.12) < 1e-6
assert abs(max(p[1] for p in body) - width / 2) < 1e-6 and abs(min(p[1] for p in body) + width / 2) < 1e-6
assert abs(min(p[2] for p in by['worn']) - bottom) < 1e-6 and min(p[0] for p in by['worn']) < back - 0.1
assert len(by['parkerized']) == 4 * 24 and all(abs(abs(p[1]) - width / 2) <= 0.03 + 1e-6 for p in by['parkerized'])
assert 'xmodel/viewmodel_test_kar98k__nomag' not in entries
lod = 'viewmodel_test_smg__nomag0'
nomag = A.read_surfaces(entries['xmodelsurfs/' + lod])
assert entries['xmodelparts/' + lod] == GUN_PARTS
assert A.E.read_xmodel(entries['xmodel/viewmodel_test_smg__nomag'])['materials'][0] == GUN_MATERIALS[:2]
assert [s['marker'] for s in nomag] == [0, 0] and sum(len(s['triangles']) for s in nomag) == len(receiver[1]) + len(barrel[1])
assert A.write_surfaces(nomag) == E.write_rigid_surfaces(GUN_SURFACES[:2])
cases += 1

# ---------------------------------------------------------------- finishes
for finish in A.FINISHES:
    image = entries['images/' + A.IMAGE + finish + '.iwi']
    assert struct.unpack_from('<3sBBBHHH', image) == (b'IWi', 5, 1, 1, A.TEXTURE_SIZE, A.TEXTURE_SIZE, 1)
    assert image == E.write_iwi_argb(A.texture_pixels(finish))
    material = entries['materials/' + A.MATERIAL + finish]
    at = struct.unpack_from('<I', material, 0)[0]
    assert material[at:material.index(b'\0', at)] == (A.MATERIAL + finish).encode()
    assert struct.unpack_from('<HH', material, 0x1c) == (A.TEXTURE_SIZE, A.TEXTURE_SIZE)
assert len({entries['images/' + A.IMAGE + finish + '.iwi'] for finish in A.FINISHES}) == len(A.FINISHES)
cases += 1

# ---------------------------------------------------------------- the generator's regressions fail these checks
MUTANTS = [
    ("anims.append(read_anim_locals(read, fields['adsUpAnim'], None))",
     "anims.append(read_anim_locals(read, fields['adsUpAnim'], 0))"),
    ("    return [lathe(Mesh(finish, picture_only=True), [(at, radius), (at, 0.0)], segments=32),",
     "    return [lathe(Mesh(finish), [(at, radius), (at, 0.0)], segments=32),"),
    ("        if S._dot(face, S._add(S._add(n[a], n[b]), n[c])) > 0.0:",
     "        if S._dot(face, S._add(S._add(n[a], n[b]), n[c])) < 0.0:"),
    ("0.0 if abs(value) < 1e-9 else round(value, 6)", "value"),
    ("            if gun.weapon not in OPTIC_SOURCES:\n                continue\n", ""),
    ("            if weapon not in MAGAZINES:\n                continue\n", ""),
    ("            if abs(s - well) > MAG_KEEP * length:\n", "            if True:\n"),
    ("            offset = S._scale(S._sub(vertex['pos'], on_line), MAG_FATTER)",
     "            offset = S._sub(vertex['pos'], on_line)"),
    ("    drop = min(max(radius - sight_clearance(gun, sleeve_back, length), 0.0), radius - hole - 0.08)",
     "    drop = 0.0"),
    ("    sleeved = not pistol and free > back + 1.0", "    sleeved = False"),
    ("    if not pistol and gun.weapon in DUAL_MAGAZINES and _fed_from_below(gun, mag):", "    if False:"),
    ("    if not pistol and gun.weapon in DUAL_MAGAZINES and _fed_from_below(gun, mag):",
     "    if not pistol and _fed_from_below(gun, mag):"),
    ("        elif attachment == 'extmag' and weapon in BOX_MAGAZINES:\n", "        elif False:\n"),
    ("    body = box(Mesh('blued'), (back, -half, bottom + 0.12), (front, half, top))",
     "    body = box(Mesh('blued'), (back, -half, bottom + 0.12), (front, half, bottom + 1.0))"),
    ("    shift = S._add(S._add(S._scale(lateral, width + gap)", "    shift = S._add(S._add(S._scale(lateral, -width - gap)"),
    ("    top = min(max(tops), at[2] - 0.2)", "    top = max(tops)"),
    ("    points = gun.points(without='j_clip')", "    points = gun.points()"),
    ("    place = S._compose((frame, origin), source)", "    place = S._compose((_sight_frame(rifle), origin), source)"),
    ("    if gun.weapon in OPTIC_IN_PLACE:\n        return parts\n", ""),
    ("min(a[2], b[2], c[2]) > 2.2 and", "min(a[2], b[2], c[2]) > -99.0 and"),
]
if not os.environ.get('COD2_ATTACHMENT_MODELS_MUTANT'):
    source_text = (Path(__file__).resolve().parent / 'browser_attachment_models.py').read_text()
    with tempfile.TemporaryDirectory(prefix='cod2-attachment-models-') as directory:
        directory = Path(directory)
        for name in ('browser_explosives.py', 'browser_smine.py'):
            shutil.copy(Path(__file__).resolve().parent / name, directory / name)
        shutil.copy(__file__, directory / 'test.py')
        for old, new in MUTANTS:
            assert source_text.count(old) == 1, old
            (directory / 'browser_attachment_models.py').write_text(source_text.replace(old, new))
            result = subprocess.run([sys.executable, str(directory / 'test.py')], capture_output=True, text=True,
                                    env=dict(os.environ, COD2_ATTACHMENT_MODELS_MUTANT='1'))
            assert result.returncode != 0, f'mutant survived: {new!r}'

print(f'PASS: {cases} cases: the models each class weapon gets, each on the bone it rides; sights on the line of '
      f'sight of the raise\'s last frame, their glass only in the pictures; the 4x Optics from the scoped rifles\' '
      f'scopes, in place, turned onto another line, cut from the Springfield, on the Bren\'s bracket; the can past '
      f'the muzzle, its sleeve, below the line of sight; two clamped magazines, a longer one and a rifle\'s box '
      f'magazine, the gun without its own; retail winding, unit normals, canonical floats, the same bytes twice; '
      f'the finishes\' textures and '
      f'materials; {len(MUTANTS)} generator mutants fail')
