"""Call of Duty: WWII's visible attachments on the class weapons' first-person guns, built from the
retail gun models and numbers when the private archives are prepared; the repository holds none of
their data.

For a gun model viewmodel_x and each attachment its weapon takes that WWII shows on the gun:
- xmodel/viewmodel_x__suppressor rides tag_flash: a can along the muzzle's +x;
- xmodel/viewmodel_x__reflex, __lens and __optic4x ride j_gun, centred on the line of sight the
  aiming animation puts the eye on, so the reticle drawn at the screen's centre sits in the glass;
- xmodel/viewmodel_x__extmag rides j_clip: the gun's own magazine, its lower part longer, in the
  gun's material; with it the game draws xmodel/viewmodel_x__nomag, the gun without its magazine.
Units are inches, x forward, y left, z up, as the guns are modelled.
"""
import json
import math
import struct
from pathlib import Path

import browser_explosives as E
import browser_smine as S

ROOT = Path(__file__).resolve().parent.parent
SUFFIX = '__'
# What each visible attachment rides and the textures its parts use.
RIDES = {'suppressor': 'tag_flash', 'reflex': 'j_gun', 'lens': 'j_gun', 'optic4x': 'j_gun', 'extmag': 'j_clip'}
VISIBLE = tuple(RIDES)
NOMAG = 'nomag'
TEXTURE_SIZE = 128
FINISHES = ('blued', 'parkerized', 'worn', 'brass', 'glass', 'black')
MATERIAL = 'mtl_browser_att_'
IMAGE = 'browser_att_'
# The guns whose j_clip is a magazine an Extended Mag lengthens; the others' is a stripper clip or
# an en-bloc clip, or a revolver's, and WWII's longer magazine needs a model of its own.
MAGAZINES = ('thompson_mp', 'greasegun_mp', 'sten_mp', 'PPS42_mp', 'mp40_mp', 'm1carbine_mp', 'SVT40_mp',
             'g43_mp', 'bar_mp', 'bren_mp', 'mp44_mp', 'colt_mp', 'TT30_mp', 'luger_mp')
# How far ahead of the eye each gun's optic sits on the line of sight, by its receiver.
OPTIC_AHEAD = {
    'thompson_mp': 8.0, 'greasegun_mp': 9.5, 'sten_mp': 8.5, 'PPS42_mp': 9.0, 'ppsh_mp': 8.0,
    'mp40_mp': 8.5, 'm1garand_mp': 8.0, 'm1carbine_mp': 7.5, 'SVT40_mp': 8.0, 'g43_mp': 8.0,
    'bar_mp': 9.0, 'bren_mp': 10.0, 'mp44_mp': 3.5, 'shotgun_mp': 7.0, 'enfield_mp': 7.5,
    'mosin_nagant_mp': 6.5, 'kar98k_mp': 7.5,
}
# A WWII can: SMGs' 8.5 long, pistols' 5.5; radius by the barrel it slips over.
CAN = {'smg': (8.5, 0.62, 0.85), 'pistol': (5.5, 0.5, 0.68)}
MAG_LONGER = 1.5
MAG_KEEP = 0.45
MAG_FATTER = 1.04


# ---------------------------------------------------------------- reading the retail files

def weapon_fields(read, weapon):
    fields = read('weapons/mp/' + weapon).decode('latin-1').split('\\')
    return dict(zip(fields[1::2], fields[2::2]))


def read_skeleton(data):
    """Bone names, parent indices (-1 for the free bones, which come first) and local bind poses."""
    if struct.unpack_from('<h', data, 0)[0] != 20:
        raise ValueError('Unexpected xmodelparts version')
    parented, free = struct.unpack_from('<hh', data, 2)
    position = 6
    parents, local = [-1] * free, [(S.IDENTITY_QUAT, (0.0, 0.0, 0.0))] * free
    for _ in range(parented):
        parent = data[position]
        t = struct.unpack_from('<3f', data, position + 1)
        x, y, z = struct.unpack_from('<3h', data, position + 13)
        rem = 0x3fff0001 - x * x - y * y - z * z
        w = int(math.floor(math.sqrt(rem) + 0.5)) if rem > 0 else 0
        parents.append(parent)
        local.append((tuple(v * 3.0518509447574615e-05 for v in (x, y, z, w)), t))
        position += 19
    names = []
    for _ in range(parented + free):
        end = data.index(b'\0', position)
        names.append(data[position:end].decode('latin-1'))
        position = end + 1
    return names, parents, local


def read_surfaces(data):
    """Each surface's header fields and its vertices with their raw bytes, as the loader reads them:
    rigid surfaces name their bone, skinned ones (-1) give each vertex its own and extra weights."""
    version, count = struct.unpack_from('<hh', data, 0)
    if version != 20:
        raise ValueError('Unexpected xmodelsurfs version')
    position, surfaces = 4, []
    for _ in range(count):
        tile = data[position]
        vertex_count, triangle_count, marker = struct.unpack_from('<HHh', data, position + 1)
        position += 7
        if marker == -1:
            position += 2
        vertices = []
        for _ in range(vertex_count):
            start = position
            normal = struct.unpack_from('<3f', data, position)
            colour = data[position + 12:position + 16]
            uv = struct.unpack_from('<2f', data, position + 16)
            binormal = struct.unpack_from('<3f', data, position + 24)
            tangent = struct.unpack_from('<3f', data, position + 36)
            position += 48
            weights = 0
            if marker == -1:
                weights = data[position]
                bone = struct.unpack_from('<h', data, position + 1)[0]
                point = struct.unpack_from('<3f', data, position + 3)
                position += 15
                if weights:
                    position += 1 + 16 * weights
            else:
                bone = marker
                point = struct.unpack_from('<3f', data, position)
                position += 12
            vertices.append({'raw': data[start:position], 'bone': bone, 'weights': weights, 'pos': point,
                             'normal': normal, 'colour': colour, 'uv': uv, 'binormal': binormal,
                             'tangent': tangent})
        triangles = list(struct.unpack_from('<%dH' % (triangle_count * 3), data, position))
        position += triangle_count * 6
        surfaces.append({'tile': tile, 'marker': marker, 'vertices': vertices, 'triangles': triangles})
    if position != len(data):
        raise ValueError('Unexpected xmodelsurfs size')
    return surfaces


def write_surfaces(surfaces):
    """Surfaces as read_surfaces gives them, the skinned ones' extra weights counted again."""
    data = struct.pack('<hh', 20, len(surfaces))
    for surface in surfaces:
        vertices, triangles = surface['vertices'], surface['triangles']
        data += struct.pack('<BHHh', surface['tile'], len(vertices), len(triangles) // 3, surface['marker'])
        if surface['marker'] == -1:
            data += struct.pack('<h', sum(vertex['weights'] for vertex in vertices))
        data += b''.join(vertex['raw'] for vertex in vertices)
        data += struct.pack('<%dH' % len(triangles), *triangles)
    return data


def read_anim_locals(read, name, frame):
    """Each animated bone's local (quaternion, translation) at a frame, None where not animated."""
    anim = S.read_xanim(read('xanim/' + name))
    frame = anim['frames'] - 1 if frame is None else frame
    return {bone: (S._at(quats, frame, S._nlerp) if quats else None,
                   S._at(trans, frame, S._lerp) if trans else None)
            for bone, quats, trans in anim['bones']}


def _posed(names, parents, local, anims, root=None):
    world = []
    for index, name in enumerate(names):
        q, t = local[index]
        for anim in anims:
            if name in anim:
                aq, at = anim[name]
                q = aq if aq is not None else q
                t = tuple(at) if at is not None else t
        if parents[index] < 0:
            world.append(root if root is not None else (q, tuple(t)))
        else:
            world.append(S._compose(world[parents[index]], (q, tuple(t))))
    return dict(zip(names, world))


class Gun:
    """A weapon's first-person gun: its model files, skeleton, surfaces and materials, each bone in
    j_gun's space in the idle pose, and the line of sight when aiming."""

    def __init__(self, read, weapon):
        self.weapon = weapon
        fields = weapon_fields(read, weapon)
        self.model = fields['gunModel']
        self.xmodel = E.read_xmodel(read('xmodel/' + self.model))
        self.lod = self.xmodel['lods'][0][1]
        self.parts = read('xmodelparts/' + self.lod)
        self.names, parents, local = read_skeleton(self.parts)
        self.surfaces = read_surfaces(read('xmodelsurfs/' + self.lod))
        self.materials = self.xmodel['materials'][0]
        idle = read_anim_locals(read, fields['idleAnim'], 0)
        gun = _posed(self.names, parents, local, [idle])
        to_gun = S._invert(gun['j_gun'])
        self.bones = {name: S._compose(to_gun, pose) for name, pose in gun.items()}
        # The hands carry the gun on tag_weapon; aiming moves them so the sights meet the eye at the origin.
        hand_names, hand_parents, hand_local = read_skeleton(read('xmodelparts/' + E.read_xmodel(
            read('xmodel/' + fields['handModel']))['lods'][0][1]))
        anims = [idle]
        if fields.get('adsUpAnim'):
            anims.append(read_anim_locals(read, fields['adsUpAnim'], None))
        hands = _posed(hand_names, hand_parents, hand_local, anims)
        aimed = S._invert(hands['tag_weapon'])
        self.eye = S._add(S._qrot(aimed[0], (0.0, 0.0, 0.0)), aimed[1])
        ahead = S._sub(S._add(S._qrot(aimed[0], (1.0, 0.0, 0.0)), aimed[1]), self.eye)
        self.ahead = S._normalise(ahead)

    def bone_index(self, name):
        return self.names.index(name)

    def points(self, bone=None):
        """Vertex positions in j_gun's space, of one bone's vertices or all."""
        out = []
        for surface in self.surfaces:
            for vertex in surface['vertices']:
                name = self.names[vertex['bone']]
                if bone is not None and name != bone:
                    continue
                q, t = self.bones.get(name, (S.IDENTITY_QUAT, (0.0, 0.0, 0.0)))
                out.append(S._add(S._qrot(q, vertex['pos']), t))
        return out


# ---------------------------------------------------------------- building meshes

class Mesh:
    """Vertices (position, normal, uv) and triangles, written as one rigid surface."""

    def __init__(self, finish):
        self.finish = finish
        self.positions, self.normals, self.uvs, self.triangles = [], [], [], []

    def vertex(self, position, normal, uv):
        self.positions.append(tuple(float(v) for v in position))
        self.normals.append(S._normalise(normal))
        self.uvs.append((float(uv[0]), float(uv[1])))
        return len(self.positions) - 1

    def triangle(self, a, b, c):
        # The retail winding: (b - a) x (c - a) points against the vertex normals.
        p, n = self.positions, self.normals
        face = S._cross(S._sub(p[b], p[a]), S._sub(p[c], p[a]))
        if S._dot(face, S._add(S._add(n[a], n[b]), n[c])) > 0.0:
            b, c = c, b
        self.triangles += [a, b, c]

    def moved(self, rotation, offset):
        """A copy turned by the quaternion rotation and moved by offset."""
        out = Mesh(self.finish)
        out.positions = [S._add(S._qrot(rotation, p), offset) for p in self.positions]
        out.normals = [S._qrot(rotation, n) for n in self.normals]
        out.uvs = list(self.uvs)
        out.triangles = list(self.triangles)
        return out

    def rigid_vertices(self):
        out = []
        for position, normal, uv in zip(self.positions, self.normals, self.uvs):
            tangent = S._cross(normal, (0.0, 0.0, 1.0))
            if S._dot(tangent, tangent) < 1e-6:
                tangent = S._cross(normal, (1.0, 0.0, 0.0))
            tangent = S._normalise(tangent)
            binormal = S._cross(normal, tangent)
            out.append((normal, b'\xff\xff\xff\xff', uv, binormal, tangent, position))
        return out


def lathe(mesh, profile, segments=24):
    """A surface of revolution about the x axis: profile is (x, radius) from back to front."""
    total, lengths = 0.0, [0.0]
    for (x0, r0), (x1, r1) in zip(profile, profile[1:]):
        total += math.hypot(x1 - x0, r1 - r0)
        lengths.append(total)
    total = total or 1.0
    for k in range(len(profile) - 1):
        (x0, r0), (x1, r1) = profile[k], profile[k + 1]
        length = math.hypot(x1 - x0, r1 - r0) or 1.0
        nx, nr = -(r1 - r0) / length, (x1 - x0) / length
        ring = []
        for s in range(segments + 1):
            angle = 2.0 * math.pi * s / segments
            c, d = math.cos(angle), math.sin(angle)
            ring.append([mesh.vertex((x, r * c, r * d), (nx, nr * c, nr * d), (s / segments, lengths[k + i] / total))
                         for i, (x, r) in enumerate(((x0, r0), (x1, r1)))])
        for s in range(segments):
            a0, b0 = ring[s]
            a1, b1 = ring[s + 1]
            if r0 > 0.0 or r1 > 0.0:
                mesh.triangle(a0, a1, b0)
                mesh.triangle(b0, a1, b1)
    return mesh


def box(mesh, low, high):
    (x0, y0, z0), (x1, y1, z1) = low, high
    faces = (((1, 0, 0), ((x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (x1, y0, z1))),
             ((-1, 0, 0), ((x0, y1, z0), (x0, y0, z0), (x0, y0, z1), (x0, y1, z1))),
             ((0, 1, 0), ((x1, y1, z0), (x0, y1, z0), (x0, y1, z1), (x1, y1, z1))),
             ((0, -1, 0), ((x0, y0, z0), (x1, y0, z0), (x1, y0, z1), (x0, y0, z1))),
             ((0, 0, 1), ((x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1))),
             ((0, 0, -1), ((x0, y1, z0), (x1, y1, z0), (x1, y0, z0), (x0, y0, z0))))
    for normal, quad in faces:
        corners = [mesh.vertex(p, normal, uv) for p, uv in zip(quad, ((0, 0), (1, 0), (1, 1), (0, 1)))]
        mesh.triangle(corners[0], corners[1], corners[2])
        mesh.triangle(corners[0], corners[2], corners[3])
    return mesh


def _percentile(values, fraction):
    values = sorted(values)
    if not values:
        return 0.0
    position = fraction * (len(values) - 1)
    low = int(math.floor(position))
    high = min(low + 1, len(values) - 1)
    return values[low] + (values[high] - values[low]) * (position - low)


def barrel_radius(gun):
    """The barrel's radius within an inch of the muzzle, about tag_flash's axis."""
    q, t = gun.bones['tag_flash']
    inverse = S._invert((q, t))
    radial = []
    for point in gun.points():
        x, y, z = S._add(S._qrot(inverse[0], point), inverse[1])
        r = math.hypot(y, z)
        if -1.2 < x < 0.3 and r < 0.9:
            radial.append(r)
    return min(max(_percentile(radial, 0.9) if radial else 0.4, 0.25), 0.7)


def receiver_top(gun, x, reach=1.5, half_width=0.6):
    """The gun's top near x along its centre line."""
    tops = [p[2] for p in gun.points() if abs(p[0] - x) < reach and abs(p[1]) < half_width]
    return _percentile(tops, 0.9) if tops else 0.0


def _sight_frame(gun):
    """The rotation taking +x to the line of sight and +z to the up nearest the gun's."""
    ahead = gun.ahead
    up = S._normalise(S._sub((0.0, 0.0, 1.0), S._scale(ahead, ahead[2])))
    left = S._cross(up, ahead)
    return _quat_from_axes(ahead, left, up)


def _quat_from_axes(x, y, z):
    m00, m01, m02 = x[0], y[0], z[0]
    m10, m11, m12 = x[1], y[1], z[1]
    m20, m21, m22 = x[2], y[2], z[2]
    trace = m00 + m11 + m22
    if trace > 0:
        s = math.sqrt(trace + 1.0) * 2
        q = ((m21 - m12) / s, (m02 - m20) / s, (m10 - m01) / s, 0.25 * s)
    elif m00 > m11 and m00 > m22:
        s = math.sqrt(1.0 + m00 - m11 - m22) * 2
        q = (0.25 * s, (m01 + m10) / s, (m02 + m20) / s, (m21 - m12) / s)
    elif m11 > m22:
        s = math.sqrt(1.0 + m11 - m00 - m22) * 2
        q = ((m01 + m10) / s, 0.25 * s, (m12 + m21) / s, (m02 - m20) / s)
    else:
        s = math.sqrt(1.0 + m22 - m00 - m11) * 2
        q = ((m02 + m20) / s, (m12 + m21) / s, 0.25 * s, (m10 - m01) / s)
    length = math.sqrt(sum(v * v for v in q))
    return tuple(v / length for v in q)


def suppressor(gun, pistol):
    """A knurled rear collar slipped over the muzzle, the long tube, a chamfered cap and a dark bore."""
    length, smallest, largest = CAN['pistol' if pistol else 'smg']
    r = min(max(barrel_radius(gun) + 0.22, smallest), largest)
    back, collar = -1.3, r * 1.08
    body = lathe(Mesh('blued'), [(back, 0.0), (back, collar * 0.86), (back + 0.08, collar), (back + 1.1, collar),
                                 (back + 1.18, r), (length - 0.35, r), (length - 0.1, r * 0.93),
                                 (length, r * 0.78), (length, r * 0.42)], segments=28)
    rings = Mesh('worn')
    x = back + 0.18
    while x < back + 1.0:
        lathe(rings, [(x, collar * 1.004), (x + 0.04, collar * 1.03), (x + 0.08, collar * 1.004)], segments=28)
        x += 0.16
    bore = lathe(Mesh('black'), [(length, r * 0.42), (length - 0.4, r * 0.38), (length - 0.4, 0.0)], segments=20)
    return [body, rings, bore]


def _on_sight_line(gun, meshes, ahead_by):
    """Meshes built about the x axis, turned onto the line of sight ahead_by in front of the eye."""
    rotation = _sight_frame(gun)
    centre = S._add(gun.eye, S._scale(gun.ahead, ahead_by))
    return [mesh.moved(rotation, centre) for mesh in meshes], centre


def reflex(gun):
    ahead = OPTIC_AHEAD.get(gun.weapon, 7.5)
    ring_outer, ring_inner, depth = 0.95, 0.80, 0.26
    ring = lathe(Mesh('blued'), [(-depth, ring_inner), (-depth, ring_outer), (depth, ring_outer),
                                 (depth, ring_inner), (-depth, ring_inner)], segments=32)
    meshes, centre = _on_sight_line(gun, [ring], ahead)
    top = receiver_top(gun, centre[0])
    post = box(Mesh('blued'), (centre[0] - 0.22, centre[1] - 0.2, top + 0.3),
               (centre[0] + 0.22, centre[1] + 0.2, centre[2] - ring_outer + 0.04))
    base = box(Mesh('parkerized'), (centre[0] - 1.4, centre[1] - 0.5, top - 0.05),
               (centre[0] + 0.9, centre[1] + 0.5, top + 0.36))
    return meshes + [post, base]


def lens(gun):
    ahead = OPTIC_AHEAD.get(gun.weapon, 7.5)
    tube = lathe(Mesh('blued'), [(-0.9, 0.42), (-0.9, 0.52), (0.7, 0.52), (0.85, 0.58)], segments=28)
    rim = lathe(Mesh('brass'), [(0.7, 0.58), (0.95, 0.64), (1.1, 0.58), (1.1, 0.44)], segments=28)
    glass = lathe(Mesh('glass'), [(1.02, 0.44), (1.02, 0.0)], segments=24)
    meshes, centre = _on_sight_line(gun, [tube, rim, glass], ahead)
    top = receiver_top(gun, centre[0], reach=1.0)
    mount = box(Mesh('parkerized'), (centre[0] - 0.7, centre[1] - 0.3, top - 0.05),
                (centre[0] + 0.5, centre[1] + 0.3, centre[2] - 0.42))
    return meshes + [mount]


def optic4x(gun, length=8.0):
    ahead = OPTIC_AHEAD.get(gun.weapon, 7.5) + 3.0
    h = length / 2
    tube = lathe(Mesh('blued'), [(-h, 0.40), (-h, 0.56), (-h + 1.4, 0.56), (-h + 2.0, 0.40), (h - 2.4, 0.40),
                                 (h - 1.6, 0.66), (h, 0.66), (h, 0.52)], segments=32)
    front = lathe(Mesh('glass'), [(h - 0.08, 0.52), (h - 0.08, 0.0)], segments=24)
    back = lathe(Mesh('glass'), [(-h + 0.06, 0.40), (-h + 0.06, 0.0)], segments=24)
    turret = box(Mesh('blued'), (-0.28, -0.28, 0.36), (0.28, 0.28, 0.78))
    side = box(Mesh('blued'), (-0.24, -0.72, -0.24), (0.24, -0.36, 0.24))
    meshes, centre = _on_sight_line(gun, [tube, front, back, turret, side], ahead)
    for along in (-h + 2.4, h - 2.9):
        ring_meshes, at = _on_sight_line(
            gun, [lathe(Mesh('parkerized'), [(-0.22, 0.44), (-0.22, 0.52), (0.22, 0.52), (0.22, 0.44)], segments=24)],
            ahead + along)
        top = receiver_top(gun, at[0], reach=0.6)
        meshes += ring_meshes + [box(Mesh('parkerized'), (at[0] - 0.2, at[1] - 0.2, top - 0.05),
                                     (at[0] + 0.2, at[1] + 0.2, at[2] - 0.46))]
    return meshes


def extended_magazine(gun):
    """The gun's magazine surfaces in j_clip's space, the part beyond MAG_KEEP of its length from the
    magazine well stretched so the whole is MAG_LONGER as long, a little fatter so the original hides
    inside: (surface index, vertices, triangles) in the gun's own material."""
    clip = gun.bone_index('j_clip')
    picked = []
    for index, surface in enumerate(gun.surfaces):
        triangles = surface['triangles']
        kept = [triangles[i:i + 3] for i in range(0, len(triangles), 3)
                if all(surface['vertices'][v]['bone'] == clip for v in triangles[i:i + 3])]
        if kept:
            picked.append((index, kept))
    points = [vertex['pos'] for index, kept in picked for tri in kept
              for vertex in (gun.surfaces[index]['vertices'][v] for v in tri)]
    if not points:
        return []
    centre = tuple(sum(p[k] for p in points) / len(points) for k in range(3))
    axis = _principal_axis(points, centre)
    s_values = [S._dot(S._sub(p, centre), axis) for p in points]
    low, high = min(s_values), max(s_values)
    # The well is the end nearer j_clip's origin, where the magazine meets the gun.
    near_high = math.dist(S._add(centre, S._scale(axis, high)), (0.0, 0.0, 0.0))
    near_low = math.dist(S._add(centre, S._scale(axis, low)), (0.0, 0.0, 0.0))
    well, sign = (high, -1.0) if near_high < near_low else (low, 1.0)
    length = high - low
    line = _centre_line(points, s_values, centre, axis, low, high)
    stretch = (MAG_LONGER - MAG_KEEP) / (1.0 - MAG_KEEP)
    keep_at = _line_at(line, well + sign * MAG_KEEP * length)
    out = []
    for index, kept in picked:
        surface = gun.surfaces[index]
        used = sorted({v for tri in kept for v in tri})
        remap = {old: new for new, old in enumerate(used)}
        vertices = []
        for old in used:
            vertex = surface['vertices'][old]
            s = S._dot(S._sub(vertex['pos'], centre), axis)
            on_line = _line_at(line, s)
            offset = S._scale(S._sub(vertex['pos'], on_line), MAG_FATTER)
            if abs(s - well) > MAG_KEEP * length:
                on_line = S._add(keep_at, S._scale(S._sub(on_line, keep_at), stretch))
            vertices.append((vertex['normal'], vertex['colour'], vertex['uv'], vertex['binormal'], vertex['tangent'],
                             S._add(on_line, offset)))
        out.append((index, vertices, [remap[v] for tri in kept for v in tri]))
    return out


def _principal_axis(points, centre):
    """The direction of greatest spread, by power iteration on the covariance."""
    c = [[sum((p[i] - centre[i]) * (p[j] - centre[j]) for p in points) for j in range(3)] for i in range(3)]
    v = (0.3, 0.2, 1.0)
    for _ in range(64):
        v = S._normalise(tuple(sum(c[i][j] * v[j] for j in range(3)) for i in range(3)))
    return v


def _centre_line(points, s_values, centre, axis, low, high, slices=12):
    line = []
    for k in range(slices):
        a = low + (high - low) * k / slices
        b = low + (high - low) * (k + 1) / slices
        inside = [p for p, s in zip(points, s_values) if a <= s <= b]
        if inside:
            line.append(((a + b) / 2, tuple(sum(p[i] for p in inside) / len(inside) for i in range(3))))
    return line


def _line_at(line, s):
    if s <= line[0][0]:
        return line[0][1]
    if s >= line[-1][0]:
        return line[-1][1]
    for (s0, p0), (s1, p1) in zip(line, line[1:]):
        if s0 <= s <= s1:
            t = (s - s0) / (s1 - s0) if s1 > s0 else 0.0
            return tuple(p0[i] + (p1[i] - p0[i]) * t for i in range(3))
    return line[-1][1]


def without_magazine(gun):
    """The gun's surfaces without the triangles on j_clip and the vertices only they used."""
    clip = gun.bone_index('j_clip')
    surfaces, materials = [], []
    for surface, material in zip(gun.surfaces, gun.materials):
        triangles = surface['triangles']
        kept = [v for i in range(0, len(triangles), 3) for v in triangles[i:i + 3]
                if not all(surface['vertices'][v]['bone'] == clip for v in triangles[i:i + 3])]
        if not kept:
            continue
        used = sorted(set(kept))
        remap = {old: new for new, old in enumerate(used)}
        surfaces.append({'tile': surface['tile'], 'marker': surface['marker'],
                         'vertices': [surface['vertices'][v] for v in used],
                         'triangles': [remap[v] for v in kept]})
        materials.append(material)
    return surfaces, materials


# ---------------------------------------------------------------- textures

def _value_noise(x, y, cell, seed):
    def lattice(ix, iy):
        n = (ix * 374761393 + iy * 668265263 + seed * 2147483647) & 0xffffffff
        n = (n ^ (n >> 13)) * 1274126177 & 0xffffffff
        return ((n ^ (n >> 16)) & 0xffff) / 65535.0
    fx, fy = x / cell, y / cell
    ix, iy = int(math.floor(fx)), int(math.floor(fy))
    tx, ty = fx - ix, fy - iy
    tx, ty = tx * tx * (3 - 2 * tx), ty * ty * (3 - 2 * ty)
    period = max(1, TEXTURE_SIZE // cell)
    a, b = lattice(ix % period, iy % period), lattice((ix + 1) % period, iy % period)
    c, d = lattice(ix % period, (iy + 1) % period), lattice((ix + 1) % period, (iy + 1) % period)
    return (a * (1 - tx) + b * tx) * (1 - ty) + (c * (1 - tx) + d * tx) * ty


def _fbm(x, y, seed):
    return (0.5 * _value_noise(x, y, 32, seed) + 0.25 * _value_noise(x, y, 16, seed + 1) +
            0.125 * _value_noise(x, y, 8, seed + 2) + 0.0625 * _value_noise(x, y, 4, seed + 3)) / 0.9375


FINISH_COLOURS = {
    # base colour, variation, and a wear colour taken where the noise runs high
    'blued': ((41, 41, 44), 0.36, None),
    'parkerized': ((51, 51, 48), 0.5, None),
    'worn': ((77, 74, 69), 0.4, (133, 128, 117)),
    'brass': ((158, 117, 51), 0.35, (196, 158, 92)),
    'glass': ((26, 36, 43), 0.2, None),
    'black': ((9, 9, 9), 0.0, None),
}


def texture_pixels(finish, seed=7):
    base, variation, wear = FINISH_COLOURS[finish]
    pixels = []
    for y in range(TEXTURE_SIZE):
        row = []
        for x in range(TEXTURE_SIZE):
            n = _fbm(x, y, seed)
            scale = 1.0 - variation / 2 + variation * n
            colour = [c * scale for c in base]
            if wear is not None and n > 0.62:
                t = min(1.0, (n - 0.62) * 5.0)
                colour = [c * (1 - t) + w * t for c, w in zip(colour, wear)]
            row.append(tuple(max(0, min(255, int(round(c)))) for c in colour) + (255,))
        pixels.append(row)
    return pixels


# ---------------------------------------------------------------- writing models

def write_single_bone_parts(name):
    return struct.pack('<hhh', 20, 0, 1) + name.encode('ascii') + b'\0' + b'\0'


def _bounds(points):
    return (tuple(min(p[k] for p in points) for k in range(3)) + tuple(max(p[k] for p in points) for k in range(3)))


def _radius(points):
    return math.ceil(max(math.sqrt(sum(v * v for v in p)) for p in points) * 1000) / 1000


def rigid_model(name, surfaces, materials, bone):
    """An xmodel of one bone, named after what it rides, from (vertices, triangles) surfaces."""
    points = [vertex[5] for vertices, _ in surfaces for vertex in vertices]
    lod = name + '0'
    return {
        'xmodel/' + name: E.write_xmodel(lod, materials, [_bounds(points)], _radius(points)),
        'xmodelparts/' + lod: write_single_bone_parts('att_' + bone),
        'xmodelsurfs/' + lod: E.write_rigid_surfaces([(0, vertices, triangles) for vertices, triangles in surfaces]),
    }


def mesh_model(name, meshes, bone):
    by_finish = {}
    for mesh in meshes:
        by_finish.setdefault(mesh.finish, []).append(mesh)
    surfaces, materials = [], []
    for finish in FINISHES:
        group = by_finish.get(finish)
        if not group:
            continue
        vertices, triangles = [], []
        for mesh in group:
            start = len(vertices)
            vertices += mesh.rigid_vertices()
            triangles += [start + v for v in mesh.triangles]
        surfaces.append((vertices, triangles))
        materials.append(MATERIAL + finish)
    return rigid_model(name, surfaces, materials, bone)


def gun_attachments(read, weapon, takes):
    """The archive entries for one weapon's visible attachments and the names of those it shows."""
    gun = Gun(read, weapon)
    pistol = weapon in PISTOLS
    entries, shown = {}, []
    base = gun.model + SUFFIX
    for attachment in VISIBLE:
        if attachment not in takes:
            continue
        if attachment == 'suppressor':
            entries.update(mesh_model(base + attachment, suppressor(gun, pistol), RIDES[attachment]))
        elif attachment == 'extmag':
            if weapon not in MAGAZINES:
                continue
            magazine = extended_magazine(gun)
            if not magazine:
                continue
            entries.update(rigid_model(base + attachment, [(v, t) for _, v, t in magazine],
                                       [gun.materials[index] for index, _, _ in magazine], RIDES[attachment]))
            surfaces, materials = without_magazine(gun)
            lod = base + NOMAG + '0'
            entries['xmodel/' + base + NOMAG] = _copy_xmodel(gun, lod, materials)
            entries['xmodelparts/' + lod] = gun.parts
            entries['xmodelsurfs/' + lod] = write_surfaces(surfaces)
        elif gun.weapon in SNIPERS and attachment == 'optic4x':
            continue
        else:
            meshes = {'reflex': reflex, 'lens': lens, 'optic4x': optic4x}[attachment](gun)
            entries.update(mesh_model(base + attachment, meshes, RIDES[attachment]))
        shown.append(attachment)
    return entries, shown


def _copy_xmodel(gun, lod, materials):
    bounds = gun.xmodel['bounds']
    points = [b[:3] for b in bounds] + [b[3:] for b in bounds]
    return E.write_xmodel(lod, materials, bounds, _radius(points))


PISTOLS = ('colt_mp', 'webley_mp', 'TT30_mp', 'luger_mp')
# Scoped rifles already wear their scope; WWII's 4x Optic on them is a shorter one, not modelled yet.
SNIPERS = ('springfield_mp', 'enfield_scope_mp', 'mosin_nagant_sniper_mp', 'kar98k_sniper_mp')


def finish_assets(read, template):
    entries = {}
    for finish in FINISHES:
        image = IMAGE + finish
        entries['images/' + image + '.iwi'] = E.write_iwi_argb(texture_pixels(finish))
        material = bytearray(S.write_model_material(read(template), MATERIAL + finish, image))
        struct.pack_into('<HH', material, 0x1c, TEXTURE_SIZE, TEXTURE_SIZE)
        entries['materials/' + MATERIAL + finish] = bytes(material)
    return entries


def attachment_model_assets(read, classes=None):
    """Every generated entry, from read(name) giving an original file's bytes."""
    if classes is None:
        classes = json.loads((ROOT / 'downstream/gateway/classes.json').read_text())
    entries = finish_assets(read, E.MK2_MATERIAL)
    for weapon in classes['primary'] + classes['secondary']:
        gun_entries, _ = gun_attachments(read, weapon, set(classes['weaponAttachments'].get(weapon, ())))
        entries.update(gun_entries)
    return entries
