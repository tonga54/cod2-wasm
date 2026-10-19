#!/usr/bin/env python3
"""The attachment model generator (browser_attachment_models.py) on made-up guns, without the owner's files:
which models each class weapon gets, the bone each rides, the sights centred on the line of sight of the
aiming animation's last frame, the can along the muzzle, the longer magazine and the gun without its own,
the retail winding, unit normals, floats that round the same on every machine, and the finishes' textures
and materials."""
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


def boxes(along, low, high, step=1.0):
    """Boxes side by side along x, so the gun has vertices all along its top."""
    mesh = A.Mesh('blued')
    x = along[0]
    while x < along[1] - 1e-9:
        A.box(mesh, (x, low[0], low[1]), (min(x + step, along[1]), high[0], high[1]))
        x += step
    return mesh.rigid_vertices(), mesh.triangles


# A made-up SMG: a receiver and barrel on j_gun, a box magazine on j_clip, the muzzle at tag_flash.
GUN_BONES = ['j_gun', 'j_bolt', 'j_clip', 'tag_brass', 'tag_flash']
receiver = boxes((-6.0, 9.0), (-0.5, -0.5), (0.5, 1.2))
barrel = boxes((9.0, 16.0), (-0.3, 1.1), (0.3, 1.7))
magazine = A.box(A.Mesh('blued'), (-0.4, -0.35, -5.0), (0.4, 0.35, 0.0))
GUN_SURFACES = E.write_rigid_surfaces([(0, *receiver), (0, *barrel), (2, magazine.rigid_vertices(), magazine.triangles)])
GUN_MATERIALS = ['mtl_test_gun', 'mtl_test_barrel', 'mtl_test_mag']
GUN_PARTS = write_parts(['j_gun'], [(name, 0, (0.0, 0.0, 0.0), (0.0, 0.0, 0.0, 1.0)) for name in GUN_BONES[1:]])
for model in ('viewmodel_test_smg', 'viewmodel_test_rifle', 'viewmodel_test_pistol'):
    files['xmodel/' + model] = E.write_xmodel(model + '0', GUN_MATERIALS, [(-1, -1, -1, 1, 1, 1)] * 5, 20.0)
    files['xmodelparts/' + model + '0'] = GUN_PARTS
    files['xmodelsurfs/' + model + '0'] = GUN_SURFACES
# The hands carry the gun on tag_weapon; tag_view is the eye.
files['xmodel/viewmodel_hands_test'] = E.write_xmodel('viewmodel_hands_test0', ['mtl_hands'], [(-1, -1, -1, 1, 1, 1)] * 3, 9)
files['xmodelparts/viewmodel_hands_test0'] = write_parts(['tag_view'], [('tag_torso', 0, (0, 0, 0), (0, 0, 0, 1)),
                                                                       ('tag_weapon', 1, (0, 0, 0), (0, 0, 0, 1))])
IDLE_WEAPON = ((0.0, 0.0, 0.0, 1.0), (8.0, 0.0, -2.0))
files['xanim/viewmodel_test_idle'] = S.write_xanim({'fps': 30, 'frames': 1, 'bones': [
    ('tag_weapon', [IDLE_WEAPON[0]], [IDLE_WEAPON[1]]),
    ('j_clip', None, [(2.5, 0.0, -0.4)]),
    ('tag_flash', None, [(16.0, 0.0, 1.4)])]})
# The raise ends on its last frame, a step past the one before, as the engine now holds it.
ADS_TORSO = [(about_y(0.0), (0.0, 0.0, 0.0)), (about_y(1.0), (2.0, 0.6, -0.3)), (about_y(-1.5), (3.6, 0.3, -0.5)),
             (about_y(-2.5), (4.0, 0.0, -0.7))]
files['xanim/viewmodel_test_ads_up'] = S.write_xanim({'fps': 30, 'frames': 4, 'bones': [
    ('tag_torso', [q for q, _ in ADS_TORSO], [t for _, t in ADS_TORSO])]})
for weapon, model in (('thompson_mp', 'viewmodel_test_smg'), ('kar98k_sniper_mp', 'viewmodel_test_rifle'),
                      ('colt_mp', 'viewmodel_test_pistol')):
    files['weapons/mp/' + weapon] = ('WEAPONFILE\\gunModel\\%s\\handModel\\viewmodel_hands_test\\idleAnim\\'
                                     'viewmodel_test_idle\\adsUpAnim\\viewmodel_test_ads_up' % model).encode()
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


CLASSES = {'primary': ['thompson_mp', 'kar98k_sniper_mp'], 'secondary': ['colt_mp'], 'weaponAttachments': {
    'thompson_mp': ['grip', 'suppressor', 'reflex', 'lens', 'optic4x', 'extmag'],
    'kar98k_sniper_mp': ['optic4x', 'suppressor'], 'colt_mp': ['suppressor', 'extmag', 'fmj']}}
entries = A.attachment_model_assets(read, CLASSES)
cases = 0


# ---------------------------------------------------------------- what each weapon gets
def model_entries(model, attachment):
    lod = model + A.SUFFIX + attachment + '0'
    return {'xmodel/' + model + A.SUFFIX + attachment, 'xmodelparts/' + lod, 'xmodelsurfs/' + lod}


expected = set()
for finish in A.FINISHES:
    expected |= {'images/' + A.IMAGE + finish + '.iwi', 'materials/' + A.MATERIAL + finish}
for attachment in ('suppressor', 'reflex', 'lens', 'optic4x', 'extmag', 'nomag'):
    expected |= model_entries('viewmodel_test_smg', attachment)
# A scoped rifle keeps its own scope; a pistol takes a can and a longer magazine; grip and FMJ show nothing.
expected |= model_entries('viewmodel_test_rifle', 'suppressor')
for attachment in ('suppressor', 'extmag', 'nomag'):
    expected |= model_entries('viewmodel_test_pistol', attachment)
assert set(entries) == expected, sorted(set(entries) ^ expected)
assert A.gun_attachments(read, 'thompson_mp', {'extmag'})[1] == ['extmag']
assert A.gun_attachments(read, 'kar98k_sniper_mp', {'optic4x', 'extmag'}) == ({}, [])
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


for model, attachment in (('viewmodel_test_smg', a) for a in ('suppressor', 'reflex', 'lens', 'optic4x')):
    name = model + A.SUFFIX + attachment
    surfaces, materials = surfaces_of(name)
    # One bone, named after the one it rides, its surfaces rigid on it in the finishes' materials.
    names, parents, local = A.read_skeleton(entries['xmodelparts/' + name + '0'])
    assert names == ['att_' + A.RIDES[attachment]] and parents == [-1], names
    assert materials and all(m.startswith(A.MATERIAL) and m[len(A.MATERIAL):] in A.FINISHES for m in materials)
    assert len(materials) == len(surfaces) == len(set(materials))
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


def eye_line(frame):
    """The eye and the view's forward axis in j_gun's space, the hands at tag_view and the gun on tag_weapon."""
    tq, tt = ADS_TORSO[frame]
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
eye, ahead = eye_line(3)
before, _ = eye_line(2)
# Within the anim's quaternions stored as shorts.
assert math.dist(gun.eye, eye) < 2e-3 and math.dist(gun.ahead, ahead) < 1e-4, (gun.eye, eye, gun.ahead, ahead)
assert math.dist(before, eye) > 0.3, 'the made-up raise must move the eye on its last frame'
# The reflex's ring, the lens's housing and the scope's glasses sit on that line, OPTIC_AHEAD in front of the eye.
reach = A.OPTIC_AHEAD['thompson_mp']
for attachment, finish, low, high, centre in (('reflex', 'worn', 0.98 * 1.005 - 1e-3, 1.15 * 0.995 + 1e-3, reach),
                                              ('lens', 'blued', 0.66 - 1e-3, 0.78 + 1e-3, reach),
                                              ('lens', 'brass', 0.66 * 0.99 - 1e-3, 0.78 * 1.04 + 1e-3, reach),
                                              ('optic4x', 'glass', 0.0, 0.52 + 1e-3, reach + 3.0)):
    surfaces, materials = surfaces_of('viewmodel_test_smg' + A.SUFFIX + attachment)
    surface = surfaces[materials.index(A.MATERIAL + finish)]
    found = [distance_to_line(v['pos'], eye, ahead) for v in surface['vertices']]
    assert all(low <= r <= high for r, _ in found), (attachment, finish, min(found), max(found))
    alongs = [a for _, a in found]
    assert abs((min(alongs) + max(alongs)) / 2 - centre) < (4.1 if attachment == 'optic4x' else 0.5), attachment
# The lens is open: no glass in its housing, so the eye sees the reticle the game draws.
assert A.MATERIAL + 'glass' not in surfaces_of('viewmodel_test_smg__lens')[1]
cases += 1

# ---------------------------------------------------------------- the can on the muzzle
for model, kind in (('viewmodel_test_smg', 'smg'), ('viewmodel_test_pistol', 'pistol')):
    length, smallest, largest = A.CAN[kind]
    surfaces, _ = surfaces_of(model + '__suppressor')
    points = [v['pos'] for s in surfaces for v in s['vertices']]
    radius = max(math.hypot(p[1], p[2]) for p in points)
    assert abs(max(p[0] for p in points) - length) < 1e-6 and abs(min(p[0] for p in points) + 1.3) < 1e-6
    # Over the made-up barrel, 0.42 from its axis: 0.22 more, within the WWII can's range.
    assert abs(radius - 1.08 * 1.03 * min(max(0.3 * math.sqrt(2) + 0.22, smallest), largest)) < 0.01, (kind, radius)
cases += 1

# ---------------------------------------------------------------- the longer magazine and the gun without its own
surfaces, materials = surfaces_of('viewmodel_test_smg__extmag')
assert materials == ['mtl_test_mag'] and A.read_skeleton(entries['xmodelparts/viewmodel_test_smg__extmag0'])[0] == ['att_j_clip']
points = [v['pos'] for s in surfaces for v in s['vertices']]
# Its well stays where it meets the gun; the rest is longer, by the centre line its slices find.
assert abs(min(p[2] for p in points) + 5.0 * A.MAG_LONGER) < 0.01 * 5.0 * A.MAG_LONGER and abs(max(p[2] for p in points)) < 1e-6
assert abs(max(p[0] for p in points) - 0.4 * A.MAG_FATTER) < 1e-4 and abs(max(p[1] for p in points) - 0.35 * A.MAG_FATTER) < 1e-4
original = A.read_surfaces(GUN_SURFACES)[2]['vertices']
assert sorted(v['uv'] for v in surfaces[0]['vertices']) == sorted(v['uv'] for v in original)
assert len(surfaces[0]['triangles']) == len(magazine.triangles)
lod = 'viewmodel_test_smg__nomag0'
nomag = A.read_surfaces(entries['xmodelsurfs/' + lod])
assert entries['xmodelparts/' + lod] == GUN_PARTS
assert A.E.read_xmodel(entries['xmodel/viewmodel_test_smg__nomag'])['materials'][0] == GUN_MATERIALS[:2]
assert [s['marker'] for s in nomag] == [0, 0] and sum(len(s['triangles']) for s in nomag) == len(receiver[1]) + len(barrel[1])
assert A.write_surfaces(nomag) == E.write_rigid_surfaces([(0, *receiver), (0, *barrel)])
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
     "anims.append(read_anim_locals(read, fields['adsUpAnim'], 2))"),
    ("    meshes, centre = _on_sight_line(gun, [housing, rim], ahead)",
     "    meshes, centre = _on_sight_line(gun, [housing, rim, lathe(Mesh('glass'), [(0.2, 0.6), (0.2, 0.0)])], ahead)"),
    ("        if S._dot(face, S._add(S._add(n[a], n[b]), n[c])) > 0.0:",
     "        if S._dot(face, S._add(S._add(n[a], n[b]), n[c])) < 0.0:"),
    ("0.0 if abs(value) < 1e-9 else round(value, 6)", "value"),
    ("        elif gun.weapon in SNIPERS and attachment == 'optic4x':\n            continue\n", ""),
    ("            if weapon not in MAGAZINES:\n                continue\n", ""),
    ("            if abs(s - well) > MAG_KEEP * length:\n", "            if True:\n"),
    ("            offset = S._scale(S._sub(vertex['pos'], on_line), MAG_FATTER)",
     "            offset = S._sub(vertex['pos'], on_line)"),
    ("    return [body, rings, bore]", "    return [mesh.moved(S.IDENTITY_QUAT, (-1.0, 0.0, 0.0)) for mesh in (body, rings, bore)]"),
]
if not os.environ.get('COD2_ATTACHMENT_MODELS_MUTANT'):
    source = (Path(__file__).resolve().parent / 'browser_attachment_models.py').read_text()
    with tempfile.TemporaryDirectory(prefix='cod2-attachment-models-') as directory:
        directory = Path(directory)
        for name in ('browser_explosives.py', 'browser_smine.py'):
            shutil.copy(Path(__file__).resolve().parent / name, directory / name)
        shutil.copy(__file__, directory / 'test.py')
        for old, new in MUTANTS:
            assert source.count(old) == 1, old
            (directory / 'browser_attachment_models.py').write_text(source.replace(old, new))
            result = subprocess.run([sys.executable, str(directory / 'test.py')], capture_output=True, text=True,
                                    env=dict(os.environ, COD2_ATTACHMENT_MODELS_MUTANT='1'))
            assert result.returncode != 0, f'mutant survived: {new!r}'

print(f'PASS: {cases} cases: the models each class weapon gets, each on the bone it rides; sights on the line of '
      f'sight of the raise\'s last frame, the lens open; the can along the muzzle; the longer magazine in the gun\'s '
      f'material and the gun without its own; retail winding, unit normals, canonical floats, the same bytes twice; '
      f'the finishes\' textures and materials; {len(MUTANTS)} generator mutants fail')
