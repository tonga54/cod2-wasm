#!/usr/bin/env python3
"""The S-Mine generator (browser_smine.py) on made-up inputs, without the
owner's files: animations survive their own writer and the loader's reading,
the pull-pin keeps the Mk 2's hands until the pin is out, the left hand finds
the S-Mine's pin and the right stands the mine upright, the plant sinks the
whole hold, the material keeps the Mk 2's layout, and the model and texture
keep their conventions."""
import math
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import browser_smine as smine

FRAMES = 18


def quat(axis, degrees):
    return smine._about(axis, degrees)


def close(a, b, tolerance=1e-3):
    return all(abs(x - y) <= tolerance for x, y in zip(a, b))


# A made-up Mk 2 pull-pin: the bones the derivation reads, moving every frame.
def made_up_pullpin():
    def per_frame(f):
        return [f(frame) for frame in range(FRAMES)]
    bones = [
        ('tag_cambone', None, None),
        ('tag_torso', None, [(-1.29, -1.15, 1.95)]),
        ('j_shoulder_le', per_frame(lambda k: quat((1, 0, 0), 10 * k)), per_frame(lambda k: (-1.0, 8.0 - 0.1 * k, -2.4))),
        ('j_shoulder_ri', per_frame(lambda k: quat((0, 1, 0), -5 * k)), per_frame(lambda k: (1.2, -10.0, -1.5 - 0.3 * k))),
        ('tag_weapon', per_frame(lambda k: quat((0.3, 0.2, 1.0), 4 * k)), per_frame(lambda k: (16.0 - 0.1 * k, -9.0 + k, -5.5))),
        ('j_elbow_le', per_frame(lambda k: quat((0, 0, 1), 3 * k)), [(12.09, 0.0, 0.0)]),
        ('j_elbow_ri', per_frame(lambda k: quat((0, 0, 1), -3 * k)), [(-12.09, 0.0, 0.0)]),
        ('j_gun', None, None),
        ('j_clip', per_frame(lambda k: quat((1, 0, 0), 20 * k)),
         per_frame(lambda k: (-0.25, 0.3, 1.51) if k <= 12 else (-0.25 - 3 * (k - 12), 0.3 + 6 * (k - 12), 1.51 + k - 12))),
        ('j_index_le_0', per_frame(lambda k: quat((0, 1, 0), k)), [(4.15, 1.1, 0.0)]),
    ]
    return {'fps': 24, 'frames': FRAMES, 'bones': bones}


def world(anim, name, frame, parent='tag_weapon'):
    torso = smine._local(anim['bones'], 'tag_torso', frame)
    if name in ('tag_weapon', 'j_shoulder_ri', 'j_shoulder_le'):
        return smine._compose(torso, smine._local(anim['bones'], name, frame))
    return smine._compose(world(anim, parent, frame), smine._local(anim['bones'], name, frame))


cases = 0
# The writer and reader agree, quaternions to the shorts' precision, and the
# file is what the loader expects: every track keyed once or every frame.
mk2 = made_up_pullpin()
data = smine.write_xanim(mk2)
again = smine.read_xanim(data)
assert again['frames'] == FRAMES and again['fps'] == 24 and [b[0] for b in again['bones']] == [b[0] for b in mk2['bones']]
for (name, q1, t1), (_, q2, t2) in zip(mk2['bones'], again['bones']):
    assert (q1 is None) == (q2 is None) and (t1 is None) == (t2 is None), name
    for a, b in zip(q1 or [], q2 or []):
        assert abs(abs(sum(x * y for x, y in zip(a, b))) - 1.0) < 1e-6, name
    for a, b in zip(t1 or [], t2 or []):
        assert close(a, b, 1e-6), name
version, frames, bones, flags, fps = struct.unpack_from('<hHHBh', data, 0)
assert (version, frames, bones, flags, fps) == (14, FRAMES, len(mk2['bones']), 0, 24) and data[-1:] == b'\0'
cases += 1
# Keyframed tracks are spread over every frame as the loader interpolates.
keyed = bytearray(struct.pack('<hHHBh', 14, 5, 1, 0, 24)) + b'\0\0' + b'j_gun\0'
keyed += struct.pack('<H', 0) + struct.pack('<H', 2) + bytes([0, 4]) + struct.pack('<3f', 0, 0, 0) + struct.pack('<3f', 4, 8, 0)
keyed += b'\0'
spread = smine.read_xanim(bytes(keyed))['bones'][0][2]
assert [round(t[0], 4) for t in spread] == [0, 1, 2, 3, 4] and close(spread[2], (2, 4, 0))
cases += 1

# The derived pull-pin: the Mk 2's hands to frame 14 but for the left hand's
# reach, then the mine upright at the hold; the pin stays in the left hand.
reach = (0.5, -0.4, 1.2)
hold_centre, hold_axis, mine_centre = (10.5, -5.8, -5.1), smine._normalise((-0.1, 0.0, 1.0)), (0.27, 0.51, -0.9)
pull = smine.derive_pullpin(mk2, reach, hold_centre, hold_axis, 0.0, mine_centre)
assert pull['frames'] == smine.PULL_FRAMES + smine.HOLD_FRAMES
last = pull['frames'] - 1
for frame in range(smine.PULL_FRAMES):
    for name in ('tag_weapon', 'j_shoulder_ri', 'j_elbow_ri'):
        assert close(smine._local(pull['bones'], name, frame)[1], smine._local(mk2['bones'], name, frame)[1]), (name, frame)
# In the torso's frame, where the hold is given.
gun = smine._local(pull['bones'], 'tag_weapon', last)
assert close(smine._qrot(gun[0], (0, 0, 1)), hold_axis) and close(smine._add(smine._qrot(gun[0], mine_centre), gun[1]), hold_centre)
# The right arm moves with the mine: their relation at the hold is frame 14's.
cut = smine.PULL_FRAMES - 1
for frame in (cut, last):
    relation = smine._compose(smine._invert(world(pull, 'tag_weapon', frame)), world(pull, 'j_shoulder_ri', frame))
    before = smine._compose(smine._invert(world(mk2, 'tag_weapon', cut)), world(mk2, 'j_shoulder_ri', cut))
    assert close(relation[1], before[1]) and abs(abs(sum(x * y for x, y in zip(relation[0], before[0]))) - 1) < 1e-6
# While it works the pin the left shoulder reaches by the grenade's frame;
# the pin, wherever the left hand takes it, sits where the Mk 2's would plus that reach.
middle = 9
offset = smine._sub(smine._local(pull['bones'], 'j_shoulder_le', middle)[1], smine._local(mk2['bones'], 'j_shoulder_le', middle)[1])
assert close(offset, smine._qrot(smine._local(mk2['bones'], 'tag_weapon', middle)[0], reach))
for frame in range(pull['frames']):
    source = frame if frame <= cut else cut + (frame - cut) / smine.HOLD_FRAMES * (FRAMES - 1 - cut)
    pin = world(pull, 'j_clip', frame)
    expected = world(mk2, 'j_clip', source)
    left = smine._sub(smine._local(pull['bones'], 'j_shoulder_le', frame)[1], smine._local(mk2['bones'], 'j_shoulder_le', source)[1])
    assert close(pin[1], smine._add(expected[1], left), 2e-3), frame
# Once the left hand has dropped the pin out of sight it is the Mk 2's again.
assert close(smine._local(pull['bones'], 'j_shoulder_le', last)[1], smine._local(mk2['bones'], 'j_shoulder_le', FRAMES - 1)[1])
cases += 4

# The plant: the hold, sinking by the whole drop, slowly at first.
drop = (6.0, 5.0, -24.0)
plant = smine.derive_plant(pull, drop)
assert plant['frames'] == smine.PLANT_FRAMES
torso = smine._track(plant['bones'], 'tag_torso')[1]
base = smine._track(pull['bones'], 'tag_torso')[1][-1]
assert close(torso[0], base) and close(torso[-1], smine._add(base, drop))
steps = [torso[k + 1][2] - torso[k][2] for k in range(len(torso) - 1)]
assert all(b < a for a, b in zip(steps, steps[1:]))
for name, quats, trans in plant['bones']:
    if name != 'tag_torso':
        assert (quats is None or len(quats) == 1) and (trans is None or len(trans) == 1), name
cases += 1

# The material: the Mk 2's layout with this name and image and the identity
# normal map; another layout is refused.
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
material = smine.write_model_material(bytes(template) + strings, 'mtl_browser_smine', 'browser_smine')


def text(at):
    return material[at:material.index(b'\0', at)].decode()


assert text(struct.unpack_from('<I', material, 0)[0]) == 'mtl_browser_smine'
assert text(struct.unpack_from('<I', material, 4)[0]) == 'browser_smine'
assert text(struct.unpack_from('<I', material, 0x38)[0]) == 'phong_replace' == text(struct.unpack_from('<I', material, 0x40)[0])
colour, normal = struct.unpack_from('<III', material, 0x44), struct.unpack_from('<III', material, 0x50)
assert (text(colour[0]), colour[1], text(colour[2])) == ('colorMap', 0x200, 'browser_smine')
assert (text(normal[0]), normal[1], text(normal[2])) == ('normalMap', 0x300, '$identitynormalmap')
assert material[0x0c:0x1c] == bytes(template[0x0c:0x1c]) and struct.unpack_from('<HH', material, 0x1c) == (256, 256)
for broken in (bytes(template) + strings.replace(b'phong_replace', b'phong_replacX'),
               bytes(template[:0x34]) + struct.pack('<I', 3) + bytes(template[0x38:]) + strings):
    try:
        smine.write_model_material(broken, 'x', 'y')
    except ValueError:
        pass
    else:
        raise AssertionError('accepted another material layout')
cases += 1

# The model: canister foot 1.5 below the origin, upright igniter and prongs
# above the lid, unit normals, every triangle wound into the solid.
body = smine.body()
points = [vertex[5] for vertex in body.vertices]
assert abs(min(p[2] for p in points) - smine.FOOT) < 1e-6 and abs(max(p[2] for p in points) - smine.PRONG_TIP[1]) < 0.05
assert abs(max(math.hypot(p[0], p[1]) for p in points if p[2] < 3.0) - smine.CAN_RADIUS) < 1e-6
for vertex in body.vertices + smine.safety_pin().vertices:
    assert abs(math.sqrt(sum(v * v for v in vertex[0])) - 1.0) < 1e-4 and vertex[1] == smine.WHITE
    assert all(0.0 <= uv <= 1.0 for uv in vertex[2])
inward = 0
for mesh in (body, smine.safety_pin()):
    for k in range(0, len(mesh.triangles), 3):
        a, b, c = (mesh.vertices[i] for i in mesh.triangles[k:k + 3])
        cross = smine._cross(smine._sub(b[5], a[5]), smine._sub(c[5], a[5]))
        normal = smine._add(smine._add(a[0], b[0]), c[0])
        if smine._dot(cross, cross) > 1e-12:
            assert smine._dot(cross, normal) <= 0.0
            inward += 1
assert inward > 900 and len(body.vertices) < 0x8000
cases += 1
# The texture: its size, opaque, the same every time, green paint above a
# yellow band on the canister's side.
pixels = smine.texture_pixels()
assert len(pixels) == smine.TEXTURE_SIZE and all(len(row) == smine.TEXTURE_SIZE for row in pixels)
assert all(pixel[3] == 255 for row in pixels for pixel in row) and pixels == smine.texture_pixels()
band = int((smine.SIDE_TOP - sum(smine.BAND) / 2) / (smine.SIDE_TOP - smine.SIDE_BOTTOM) * smine.SIDE[3])
green, yellow = pixels[60][100], pixels[band][100]
assert green[1] > green[0] * 1.2 and green[1] > green[2] * 2
assert yellow[0] > yellow[2] * 2.5 and yellow[1] > yellow[2] * 2.5 and yellow[0] > yellow[1] and yellow[0] > green[0] * 1.3
cases += 1
print(f'PASS: {cases} S-Mine generator cases: xanims survive the writer and keyed reading; the pull-pin keeps the Mk 2\'s '
      'hands to the pin, the left hand reaching the S-Mine\'s pin and dropping it with the Mk 2\'s, the right standing the '
      'mine upright at the hold; the plant sinks the hold by its drop, faster each frame; the material keeps the Mk 2\'s '
      'layout and refuses others; the model and texture keep their foot, winding, normals and paint')
