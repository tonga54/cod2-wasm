"""Call of Duty: WWII's visible attachments on the class weapons' first-person guns, built from the
retail gun models and numbers when the private archives are prepared; the repository holds none of
their data.

For a gun model viewmodel_x and each attachment its weapon takes that WWII shows on the gun:
- xmodel/viewmodel_x__suppressor rides tag_flash: a can along the muzzle's +x;
- xmodel/viewmodel_x__reflex and __lens ride j_gun, centred on the line of sight the aiming
  animation puts the eye on, so the reticle drawn at the screen's centre sits in the glass; beside
  each, glass/viewmodel_x__reflex or __lens is the window the WWII scope keeps the gun out of;
- xmodel/viewmodel_x__optic4x rides j_gun, a scoped rifle's scope over the receiver; beside it
  glass/viewmodel_x__optic4x is its glass for the WWII scope (cg_wwiiscope_mp.c), which centres
  that glass at the eye while aiming;
- xmodel/viewmodel_x__extmag rides j_clip: the gun's own magazine, its lower part longer, in the
  gun's material; with it the game draws xmodel/viewmodel_x__nomag, the gun without its magazine.
Units are inches, x forward, y left, z up, as the guns are modelled.
"""
import json
import math
import re
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
FINISHES = ('blued', 'parkerized', 'worn', 'brass', 'glass', 'black', 'rust')
MATERIAL = 'mtl_browser_att_'
IMAGE = 'browser_att_'
# The Reflex and Lens Sights wear the gun's own material, renamed with MATERIAL so the WWII scope's compositor
# draws them over a sight's window as it does the finishes: the gun's colour, detail, normal and specular maps,
# their UVs on a plain patch of its metal at its own texel density.
GUN_METAL = 'gun'
# The search reads the colour and specular maps at the mip level whose larger side is at most PATCH_MIP; the patch
# is PATCH_TEXELS square there.
PATCH_MIP = 128
PATCH_TEXELS = 8
# Of the best patches there, the first whose texels at twice the size stand out of their neighbourhood by less
# than this, nine in ten of them (a fraction of full scale), so no screw head, seam or engraving shows on a sight.
PATCH_CANDIDATES = 40
PATCH_DETAIL = 0.035
# The guns whose j_clip is a magazine an Extended Mag lengthens; the others' is a stripper clip or
# an en-bloc clip, a drum or a revolver's.
MAGAZINES = ('thompson_mp', 'greasegun_mp', 'sten_mp', 'PPS42_mp', 'mp40_mp', 'm1carbine_mp', 'SVT40_mp',
             'g43_mp', 'bar_mp', 'bren_mp', 'mp44_mp', 'colt_mp', 'TT30_mp', 'luger_mp')
# The ones whose longer magazine is two clamped together, as WWII's SMGs'; the rifles' is one longer.
DUAL_MAGAZINES = ('thompson_mp', 'greasegun_mp', 'mp40_mp', 'PPS42_mp', 'mp44_mp')
# The rifles loaded through the top, whose Extended Mag is a detachable box magazine under the receiver, as
# WWII's Kar98k's, ahead of the trigger guard: from back to front along x, its top hidden in the stock, its
# bottom and width, in j_gun's space as their side views measure them. The Lee-Enfield's wraps its own.
BOX_MAGAZINES = {
    'kar98k_mp': (-6.2, -3.8, -1.1, -3.6, 0.7), 'kar98k_sniper_mp': (-6.2, -3.8, -1.1, -3.6, 0.7),
    'springfield_mp': (-4.8, -2.4, -0.55, -3.1, 0.7), 'm1garand_mp': (-4.8, -2.3, -1.65, -4.4, 0.74),
    'mosin_nagant_mp': (-2.9, -0.8, -2.0, -4.5, 0.7), 'mosin_nagant_sniper_mp': (-2.9, -0.8, -2.0, -4.5, 0.7),
    'enfield_mp': (-3.25, -1.15, -1.35, -4.5, 0.95), 'enfield_scope_mp': (-3.25, -1.15, -1.35, -4.5, 0.95),
}
EXTBOX = 'extbox'
# How far ahead of the eye each gun's optic sits on the line of sight, by its receiver.
OPTIC_AHEAD = {
    'thompson_mp': 8.0, 'greasegun_mp': 9.5, 'sten_mp': 8.5, 'PPS42_mp': 9.0, 'ppsh_mp': 8.0,
    'mp40_mp': 8.5, 'm1garand_mp': 8.0, 'm1carbine_mp': 7.5, 'SVT40_mp': 8.0, 'g43_mp': 8.0,
    'bar_mp': 9.0, 'bren_mp': 10.0, 'mp44_mp': 8.0, 'shotgun_mp': 11.0, 'enfield_mp': 7.5,
    'mosin_nagant_mp': 6.5, 'kar98k_mp': 7.5,
}
# The Reflex Sight's frame and the Lens Sight's ring: their outer radius and the opening the eye looks through.
# The WWII scope (cg_wwiiscope_mp.c) keeps the gun off the ring's whole disc, SIGHT_RINGS times the opening.
REFLEX_OUTER, REFLEX_INNER = 0.74, 0.64
LENS_OUTER, LENS_INNER = 0.6, 0.5
SIGHT_OPENINGS = {'reflex': REFLEX_INNER, 'lens': LENS_INNER}
SIGHT_RINGS = {'reflex': REFLEX_OUTER / REFLEX_INNER, 'lens': LENS_OUTER / LENS_INNER}
# A suppressor's can: how far it reaches past the muzzle and its radius.
CAN = {'smg': (6.5, 1.0), 'pistol': (4.8, 0.62)}
# How far back from the muzzle a bare SMG barrel takes the sleeve.
SLEEVE_REACH = 12.0
# How much lower the second of two clamped magazines sits, of the magazine's length, and how far ahead, of its
# depth.
DUAL_DROP = 0.12
DUAL_AHEAD = 0.4
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
        # j_gun's pose at full aim from the view weapon's origin: x forward, y left, z up.
        self.aimed = S._compose(hands['tag_weapon'], gun['j_gun'])
        aimed = S._invert(hands['tag_weapon'])
        self.eye = S._add(S._qrot(aimed[0], (0.0, 0.0, 0.0)), aimed[1])
        ahead = S._sub(S._add(S._qrot(aimed[0], (1.0, 0.0, 0.0)), aimed[1]), self.eye)
        self.ahead = S._normalise(ahead)

    def bone_index(self, name):
        return self.names.index(name)

    def points(self, bone=None, without=None):
        """Vertex positions in j_gun's space, of one bone's vertices, of all but one bone's, or all."""
        out = []
        for surface in self.surfaces:
            for vertex in surface['vertices']:
                name = self.names[vertex['bone']]
                if bone is not None and name != bone or name == without:
                    continue
                q, t = self.bones.get(name, (S.IDENTITY_QUAT, (0.0, 0.0, 0.0)))
                out.append(S._add(S._qrot(q, vertex['pos']), t))
        return out


# ---------------------------------------------------------------- building meshes

class Mesh:
    """Vertices (position, normal, uv) and triangles, written as one rigid surface. A picture-only mesh, such as
    a sight's glass the eye must see through in a match, shows in the class pictures alone."""

    def __init__(self, finish, picture_only=False):
        self.finish = finish
        self.picture_only = picture_only
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
        out = Mesh(self.finish, self.picture_only)
        out.positions = [S._add(S._qrot(rotation, p), offset) for p in self.positions]
        out.normals = [S._qrot(rotation, n) for n in self.normals]
        out.uvs = list(self.uvs)
        out.triangles = list(self.triangles)
        return out

    def rigid_vertices(self, uv_of=None):
        """The vertices as write_rigid_surfaces takes them, their UVs uv_of(position, normal) when given, of the
        canonical position and normal, so a normal between two axes takes the same one on the Pi."""
        out = []
        for position, normal, uv in zip(self.positions, self.normals, self.uvs):
            if uv_of:
                uv = uv_of(canonical(position), canonical(normal))
            tangent = S._cross(normal, (0.0, 0.0, 1.0))
            if S._dot(tangent, tangent) < 1e-6:
                tangent = S._cross(normal, (1.0, 0.0, 0.0))
            tangent = S._normalise(tangent)
            binormal = S._cross(normal, tangent)
            out.append((canonical(normal), b'\xff\xff\xff\xff', canonical(uv), canonical(binormal),
                        canonical(tangent), canonical(position)))
        return out


class Surface:
    """Triangles a retail model draws, in their own material, moved into the attachment's space: rigid vertices
    as write_rigid_surfaces takes them, normal, colour, uv, binormal, tangent and position."""

    def __init__(self, material, vertices, triangles):
        self.material = material
        self.vertices = vertices
        self.triangles = triangles

    def moved(self, rotation, offset):
        def turn(vector):
            return canonical(S._qrot(rotation, vector))
        return Surface(self.material, [(turn(n), c, uv, turn(b), turn(t), canonical(S._add(S._qrot(rotation, p), offset)))
                                       for n, c, uv, b, t, p in self.vertices], list(self.triangles))


def retail_surface(gun, index, keep=None, into=None):
    """One of the gun's surfaces in j_gun's space, or into the space of the pose into gives, of the triangles keep
    takes (a function of their three positions there), with the vertices they use."""
    surface = gun.surfaces[index]
    inverse = S._invert(into) if into else None
    vertices = []
    for vertex in surface['vertices']:
        q, t = gun.bones.get(gun.names[vertex['bone']], (S.IDENTITY_QUAT, (0.0, 0.0, 0.0)))
        if inverse:
            q, t = S._compose(inverse, (q, t))
        vertices.append((canonical(S._qrot(q, vertex['normal'])), vertex['colour'], vertex['uv'],
                         canonical(S._qrot(q, vertex['binormal'])), canonical(S._qrot(q, vertex['tangent'])),
                         canonical(S._add(S._qrot(q, vertex['pos']), t))))
    triangles = surface['triangles']
    kept = [triangles[i:i + 3] for i in range(0, len(triangles), 3)
            if keep is None or keep(*(vertices[v][5] for v in triangles[i:i + 3]))]
    used = sorted({v for tri in kept for v in tri})
    remap = {old: new for new, old in enumerate(used)}
    return Surface(gun.materials[index], [vertices[v] for v in used], [remap[v] for tri in kept for v in tri])


def canonical(values):
    """Floats as the build machine and the Pi both write them: their libm leave different last bits
    in trigonometry and cancellation, so values round to a millionth and residues to zero."""
    return tuple(0.0 if abs(value) < 1e-9 else round(value, 6) for value in values)


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


def receiver_top(gun, x, reach=1.5, half_width=0.6, y=0.0, points=None):
    """The gun's top near x, along its centre line or the line y beside it."""
    tops = [p[2] for p in (points or gun.points()) if abs(p[0] - x) < reach and abs(p[1] - y) < half_width]
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


def _flash_space(gun, point):
    q, t = S._invert(gun.bones['tag_flash'])
    return S._add(S._qrot(q, point), t)


def free_barrel(gun, radius, reach=8.0, step=0.1):
    """How far back from the muzzle the barrel runs bare: nothing within 2.5 of its axis stands out of it."""
    local = [_flash_space(gun, p) for p in gun.points()]
    local = [(p[0], math.hypot(p[1], p[2])) for p in local if -reach - step <= p[0] <= 0.05]
    run = 0.0
    while run < reach:
        if any(-(run + step) <= x < -run and radius + 0.12 < r < 2.5 for x, r in local):
            break
        run += step
    return run


def sight_clearance(gun, back, front, margin=0.12):
    """The least room between the bore and the line of sight from back to front along it, less a margin."""
    eye = _flash_space(gun, gun.eye)
    tip = _flash_space(gun, S._add(gun.eye, gun.ahead))
    ahead = S._sub(tip, eye)
    room = []
    for x in (back, (back + front) / 2, front):
        t = (x - eye[0]) / ahead[0] if abs(ahead[0]) > 1e-6 else 0.0
        room.append(math.hypot(eye[1] + ahead[1] * t, eye[2] + ahead[2] * t))
    return min(room) - margin


def _perforations(mesh, radius, start, end, rows=8, pitch=0.42, size=0.15):
    """Rows of dark holes around a sleeve of radius, from start to end along x."""
    x = start
    while x + size <= end:
        for row in range(rows):
            angle = 2.0 * math.pi * (row + 0.5 * (int((x - start) / pitch) % 2)) / rows
            c, s = math.cos(angle), math.sin(angle)
            half = math.atan(size / 2 / radius)
            corners = []
            for dx, da in ((0.0, -half), (size, -half), (size, half), (0.0, half)):
                cc, ss = math.cos(angle + da), math.sin(angle + da)
                corners.append(mesh.vertex((x + dx, radius * 1.004 * cc, radius * 1.004 * ss), (0.0, c, s),
                                           (dx / size, da / half * 0.5 + 0.5)))
            mesh.triangle(corners[0], corners[1], corners[2])
            mesh.triangle(corners[0], corners[2], corners[3])
        x += pitch
    return mesh


def suppressor(gun, pistol):
    """WWII's suppressor in tag_flash's space: a fat can past the muzzle with a band at each end, and on an SMG
    whose barrel runs bare a rusted, perforated sleeve over it back to the receiver. It never rises into the
    line of sight: where that runs low over the bore the can hangs below it, the bore near its top."""
    length, radius = CAN['pistol' if pistol else 'smg']
    barrel = barrel_radius(gun)
    free = free_barrel(gun, barrel, reach=SLEEVE_REACH)
    back = min(free, 0.6 if pistol else 1.2)
    sleeved = not pistol and free > back + 1.0
    sleeve_back = -free if sleeved else -back
    hole = min(barrel * 0.6, radius * 0.4)
    drop = min(max(radius - sight_clearance(gun, sleeve_back, length), 0.0), radius - hole - 0.08)
    eye = _flash_space(gun, gun.eye)
    up = S._normalise((0.0, eye[1], eye[2]))
    axis = S._scale(up, -drop)
    parts = []
    if sleeved:
        sleeve = radius * 0.9
        parts.append(lathe(Mesh('rust'), [(sleeve_back, 0.0), (sleeve_back, sleeve * 0.96),
                                          (sleeve_back + 0.1, sleeve), (-back + 0.05, sleeve)], segments=32))
        parts.append(_perforations(Mesh('black'), sleeve, sleeve_back + 0.5, -back - 0.4, rows=10, pitch=0.5,
                                   size=0.12))
    body = lathe(Mesh('blued'), [(-back, 0.0), (-back, radius * 0.94), (-back + 0.12, radius),
                                 (length - 0.35, radius), (length - 0.12, radius * 0.95), (length, radius * 0.8),
                                 (length, 0.0)], segments=36)
    bands = Mesh('worn')
    for x in (-back + 0.1, length - 0.7):
        lathe(bands, [(x, radius), (x, radius * 1.035), (x + 0.16, radius * 1.035), (x + 0.16, radius)],
              segments=36)
    bore = lathe(Mesh('black'), [(length + 0.004, hole), (length + 0.004, 0.0)], segments=20)
    return [part.moved(S.IDENTITY_QUAT, axis) for part in parts + [body, bands]] + [bore]


def _on_sight_line(gun, meshes, ahead_by):
    """Meshes built about the x axis, turned onto the line of sight ahead_by in front of the eye."""
    rotation = _sight_frame(gun)
    centre = S._add(gun.eye, S._scale(gun.ahead, ahead_by))
    return [mesh.moved(rotation, centre) for mesh in meshes], centre


def _under_opening(gun, centre, inner):
    """Where a sight's mount may stand: the gun's top beneath the sight, of what lies below the opening at the
    sight's side, and the ceiling no part of the mount reaches past, just under the opening, as the eye sees the
    world through it with the rest of the gun kept out."""
    ceiling = centre[2] - inner - 0.02
    points = [p for p in gun.points(without='j_clip') if p[2] < ceiling]
    return receiver_top(gun, centre[0], reach=1.0, y=centre[1], points=points), ceiling


def _mount_box(mesh, low, high, ceiling):
    """A box of the mount, its top no higher than the ceiling; None when nothing of it is left."""
    top = min(high[2], ceiling)
    return box(mesh, low, (high[0], high[1], top)) if top > low[2] + 1e-3 else None


def _glass(radius, at, finish='glass'):
    """A sight's glass for the pictures, facing both ways at at along the line of sight."""
    return [lathe(Mesh(finish, picture_only=True), [(at, radius), (at, 0.0)], segments=32),
            lathe(Mesh(finish, picture_only=True), [(at - 0.01, 0.0), (at - 0.01, radius)], segments=32)]


def reflex(gun):
    """WWII's reflex sight: a round window in a thin bevelled frame, upright on a post and a clamped base over the
    receiver, centred on the line of sight, in the gun's metal; its glass shows in the pictures, the reticle in a
    match."""
    ahead = OPTIC_AHEAD.get(gun.weapon, 7.5)
    outer, inner, depth, bevel = REFLEX_OUTER, REFLEX_INNER, 0.12, 0.03
    frame = lathe(Mesh(GUN_METAL), [(-depth, inner), (-depth, outer - bevel), (-depth + bevel, outer),
                                    (depth - bevel, outer), (depth, outer - bevel), (depth, inner), (-depth, inner)],
                  segments=48)
    meshes, centre = _on_sight_line(gun, [frame] + _glass(inner, 0.0), ahead)
    top, ceiling = _under_opening(gun, centre, inner)
    post = _mount_box(Mesh(GUN_METAL), (centre[0] - 0.16, centre[1] - 0.18, top + 0.25),
                      (centre[0] + 0.16, centre[1] + 0.18, centre[2] - outer + 0.04), ceiling)
    base = _mount_box(Mesh(GUN_METAL), (centre[0] - 0.8, centre[1] - 0.42, top - 0.05),
                      (centre[0] + 0.6, centre[1] + 0.42, top + 0.28), ceiling)
    screw = _mount_box(Mesh(GUN_METAL), (centre[0] - 0.15, centre[1] - 0.52, top),
                       (centre[0] + 0.15, centre[1] - 0.42, top + 0.22), ceiling)
    return meshes + [part for part in (post, base, screw) if part]


def lens(gun):
    """WWII's lens sight: the lens in a thin ring, upright on a short stalk and a block clamped to a base over
    the receiver, centred on the line of sight, in the gun's metal; its lens shows in the pictures, the reticle in
    a match."""
    ahead = OPTIC_AHEAD.get(gun.weapon, 7.5)
    outer, inner, half, bevel = LENS_OUTER, LENS_INNER, 0.08, 0.025
    ring = lathe(Mesh(GUN_METAL), [(-half, inner), (-half, outer - bevel), (-half + bevel, outer),
                                   (half - bevel, outer), (half, outer - bevel), (half, inner), (-half, inner)],
                 segments=48)
    bezel = lathe(Mesh(GUN_METAL), [(half, inner * 1.005), (half + 0.025, inner * 1.02), (half + 0.025, inner * 1.1),
                                    (half, inner * 1.14)], segments=48)
    meshes, centre = _on_sight_line(gun, [ring, bezel] + _glass(inner, 0.0), ahead)
    top, ceiling = _under_opening(gun, centre, inner)
    bottom = centre[2] - outer + 0.03
    block_top = max(top + 0.22, min(top + 0.4, bottom - 0.05))
    base = _mount_box(Mesh(GUN_METAL), (centre[0] - 0.75, centre[1] - 0.34, top - 0.05),
                      (centre[0] + 0.55, centre[1] + 0.34, top + 0.16), ceiling)
    block = _mount_box(Mesh(GUN_METAL), (centre[0] - 0.3, centre[1] - 0.26, top + 0.15),
                       (centre[0] + 0.3, centre[1] + 0.26, block_top), ceiling)
    stalk = _mount_box(Mesh(GUN_METAL), (centre[0] - 0.07, centre[1] - 0.1, block_top - 0.02),
                       (centre[0] + 0.07, centre[1] + 0.1, max(bottom, block_top)), ceiling)
    screws = [_mount_box(Mesh(GUN_METAL), (centre[0] + x - 0.09, centre[1] - 0.42, top),
                         (centre[0] + x + 0.09, centre[1] - 0.34, top + 0.13), ceiling) for x in (-0.5, 0.3)]
    return meshes + [part for part in [base, block, stalk] + screws if part]


def sight_glass(gun, attachment):
    """The Reflex or Lens Sight's window as the WWII scope reads a glass: its centre on the line of sight
    OPTIC_AHEAD in front of the eye, its radius the opening of the sight's ring."""
    rim = S._add(gun.eye, S._scale(gun.ahead, OPTIC_AHEAD.get(gun.weapon, 7.5)))
    return glass_line(gun, rim, SIGHT_OPENINGS[attachment])


# The 4x Optic each gun wears: a CoD2 scope, its nation's where one comes apart from its rifle's model; the
# Kar98k and the Lee-Enfield take their own sniper rifle's as it sits on that rifle.
OPTIC_SOURCES = {
    'kar98k_mp': 'kar98k_sniper_mp', 'g43_mp': 'kar98k_sniper_mp', 'mp44_mp': 'kar98k_sniper_mp',
    'enfield_mp': 'enfield_scope_mp', 'bren_mp': 'enfield_scope_mp',
    'm1garand_mp': 'springfield_mp', 'm1carbine_mp': 'springfield_mp', 'bar_mp': 'springfield_mp',
    'mosin_nagant_mp': 'springfield_mp', 'SVT40_mp': 'springfield_mp',
}
OPTIC_IN_PLACE = ('kar98k_mp', 'enfield_mp')
# Above this much room between a 4x Optic's mount and the gun under it, a bracket fills it.
MOUNT_GAP = 0.25
# The scoped rifles' glasses by gun model, as the WWII scope's table holds them: the rim's centre in j_gun's
# space and its radius.
SCOPE_GLASSES = {model.lower(): ((float(x), float(y), float(z)), float(radius)) for model, x, y, z, radius in re.findall(
    r'\{ "(\w+)", \{ ([-\d.]+)f, ([-\d.]+)f, ([-\d.]+)f \}, ([-\d.]+)f,',
    (ROOT / 'src/PC/cgame_mp/cg_wwiiscope_mp.c').read_text())}
GLASS = 'glass/'


def scope_parts(read, source):
    """A scoped rifle's scope in its j_gun space, with the rifle: its surfaces in a scope material, or the
    Springfield's triangles over its receiver and behind its front sight."""
    rifle = Gun(read, source)
    parts = [retail_surface(rifle, index) for index, material in enumerate(rifle.materials) if 'scope' in material]
    if not parts:
        for index, surface in enumerate(rifle.surfaces):
            if surface['marker'] == rifle.bone_index('j_gun'):
                part = retail_surface(rifle, index, keep=lambda a, b, c: min(a[2], b[2], c[2]) > 2.2 and
                                      max(a[0], b[0], c[0]) < 5.0)
                if part.triangles:
                    parts.append(part)
    return rifle, parts


def optic4x_fit(gun, read):
    """Where the gun's 4x Optic sits: the rifle of OPTIC_SOURCES its scope comes from, the scope's parts in that
    rifle's j_gun space, the move onto the gun's and the bracket under it, both None for a scope that sits on
    the gun as on its rifle. The scope lies along the gun's line of sight over its receiver, OPTIC_AHEAD and 3
    more ahead of the eye, its mount on the highest of the gun beneath the middle of the scope but no higher
    than the iron sights' line: what stands taller, as the Bren's magazine and its cover, the scope clears
    beside it."""
    rifle, parts = scope_parts(read, OPTIC_SOURCES[gun.weapon])
    if gun.weapon in OPTIC_IN_PLACE:
        return rifle, parts, None, None
    source = S._invert((_sight_frame(rifle), rifle.eye))
    local = [S._add(S._qrot(source[0], v[5]), source[1]) for part in parts for v in part.vertices]
    back, front = min(p[0] for p in local), max(p[0] for p in local)
    middle = (back + front) / 2
    at = S._add(gun.eye, S._scale(gun.ahead, OPTIC_AHEAD.get(gun.weapon, 7.5) + 3.0))
    points = gun.points(without='j_clip')
    half = (front - back) * 0.3
    tops = [receiver_top(gun, at[0] + k * half, reach=0.6, y=at[1], points=points) for k in (-1.0, -0.5, 0.0, 0.5, 1.0)]
    top = min(max(tops), at[2] - 0.2)
    frame = _sight_frame(gun)
    lowest = at[2] + min(S._qrot(frame, (p[0] - middle, p[1], p[2]))[2] for p in local)
    up = S._qrot(frame, (0.0, 0.0, 1.0))
    origin = S._add(S._add(at, S._qrot(frame, (-middle, 0.0, 0.0))), S._scale(up, (top - lowest) / up[2]))
    place = S._compose((frame, origin), source)
    mount = None
    if top - tops[2] > MOUNT_GAP:
        mount = box(Mesh('parkerized'), (at[0] - 0.7, at[1] - 0.22, tops[2] - 0.1),
                    (at[0] + 0.7, at[1] + 0.22, top + 0.05))
    return rifle, parts, place, mount


def optic4x(gun, read):
    """The gun's 4x Optic in j_gun's space, as optic4x_fit places it."""
    _, parts, place, mount = optic4x_fit(gun, read)
    if place is None:
        return parts
    return [part.moved(*place) for part in parts] + ([mount] if mount else [])


def optic4x_glass(gun, read):
    """The 4x Optic's glass as the WWII scope reads it, a line of the numbers its table gives a scoped rifle: the
    rim's centre in j_gun's space and its radius, where the gun's full aim leaves that centre from the view
    weapon's origin, forward, left and up, and the degrees it cants the scope clockwise; None when the scope's
    rifle has no glass in the table."""
    rifle, _, place, _ = optic4x_fit(gun, read)
    glass = SCOPE_GLASSES.get(rifle.model.lower())
    if not glass:
        return None
    rim, radius = glass
    if place is not None:
        rim = S._add(S._qrot(place[0], rim), place[1])
    return glass_line(gun, rim, radius)


def glass_line(gun, rim, radius):
    """A glass of radius at rim in j_gun's space as the WWII scope reads it: the rim, the radius, where the
    gun's full aim leaves the rim from the view weapon's origin, forward, left and up, and the degrees it cants
    the gun clockwise."""
    q, t = gun.aimed
    eye = S._add(S._qrot(q, rim), t)
    up = S._qrot(q, (0.0, 0.0, 1.0))
    roll = math.degrees(math.atan2(-up[1], up[2]))
    return ' '.join(f'{value:.4f}' for value in canonical((*rim, radius, *eye, roll))) + '\n'


def _magazine(gun):
    """The gun's magazine in j_clip's space: its triangles on j_clip by surface, their points, centre and principal
    axis, the extent along it and the end at the well, where it meets the gun, with the way out of the well."""
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
        return None
    centre = tuple(sum(p[k] for p in points) / len(points) for k in range(3))
    axis = _principal_axis(points, centre)
    s_values = [S._dot(S._sub(p, centre), axis) for p in points]
    low, high = min(s_values), max(s_values)
    # The well is the end nearer j_clip's origin, where the magazine meets the gun.
    near_high = math.dist(S._add(centre, S._scale(axis, high)), (0.0, 0.0, 0.0))
    near_low = math.dist(S._add(centre, S._scale(axis, low)), (0.0, 0.0, 0.0))
    well, sign = (high, -1.0) if near_high < near_low else (low, 1.0)
    return {'picked': picked, 'points': points, 'centre': centre, 'axis': axis, 's': s_values, 'low': low,
            'high': high, 'well': well, 'sign': sign, 'length': high - low}


def _magazine_vertex(vertex, position):
    return (vertex['normal'], vertex['colour'], vertex['uv'], vertex['binormal'], vertex['tangent'],
            canonical(position))


def _fed_from_below(gun, mag):
    """A magazine hanging under the gun: out of the well it heads mostly down in j_gun's space."""
    out = S._qrot(gun.bones['j_clip'][0], S._scale(mag['axis'], mag['sign']))
    return out[2] < -0.7


def stretched_magazine(gun, mag):
    """The gun's magazine, the part beyond MAG_KEEP of its length from the well stretched so the whole is
    MAG_LONGER as long, a little fatter so the original hides inside, in the gun's own material."""
    centre, axis, well, sign, length = mag['centre'], mag['axis'], mag['well'], mag['sign'], mag['length']
    line = _centre_line(mag['points'], mag['s'], centre, axis, mag['low'], mag['high'])
    stretch = (MAG_LONGER - MAG_KEEP) / (1.0 - MAG_KEEP)
    keep_at = _line_at(line, well + sign * MAG_KEEP * length)
    out = []
    for index, kept in mag['picked']:
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
            vertices.append(_magazine_vertex(vertex, S._add(on_line, offset)))
        out.append(Surface(gun.materials[index], vertices, [remap[v] for tri in kept for v in tri]))
    return out


def dual_magazine(gun, mag, gap=0.08):
    """WWII's longer magazine for a gun fed from below: its own magazine and a second one beside it on the gun's
    left, the side the eye and the pictures see, set lower by DUAL_DROP of its length so its lips clear the
    well and ahead by DUAL_AHEAD of its depth, clamped to the first by a band near the well and a bolted block
    near the base."""
    centre, axis = mag['centre'], mag['axis']
    lateral = S._qrot(S._invert(gun.bones['j_clip'])[0], (0.0, 1.0, 0.0))
    lateral = S._normalise(S._sub(lateral, S._scale(axis, S._dot(lateral, axis))))
    third = S._cross(axis, lateral)
    across = [S._dot(S._sub(p, centre), lateral) for p in mag['points']]
    width = max(across) - min(across)
    deep_all = [S._dot(S._sub(p, centre), third) for p in mag['points']]
    ahead = mag['sign'] * DUAL_AHEAD * (max(deep_all) - min(deep_all))
    lower = mag['sign'] * DUAL_DROP * mag['length']
    shift = S._add(S._add(S._scale(lateral, width + gap), S._scale(axis, lower)), S._scale(third, ahead))
    out = []
    for index, kept in mag['picked']:
        surface = gun.surfaces[index]
        used = sorted({v for tri in kept for v in tri})
        remap = {old: new for new, old in enumerate(used)}
        triangles = [remap[v] for tri in kept for v in tri]
        first = [_magazine_vertex(surface['vertices'][v], surface['vertices'][v]['pos']) for v in used]
        second = [_magazine_vertex(surface['vertices'][v], S._add(surface['vertices'][v]['pos'], shift)) for v in used]
        out.append(Surface(gun.materials[index], first, triangles))
        out.append(Surface(gun.materials[index], second, list(triangles)))
    frame = _quat_from_axes(axis, lateral, third)
    for share, half, finish in ((DUAL_DROP + 0.12, 0.12, 'parkerized'), (0.86, 0.3, 'blued')):
        at = mag['well'] + mag['sign'] * share * mag['length']
        near = [p for p, s in zip(mag['points'], mag['s']) if abs(s - at) < 0.35] or mag['points']
        deep = [S._dot(S._sub(p, centre), third) for p in near]
        side = [S._dot(S._sub(p, centre), lateral) for p in near]
        low = (at - half, min(side) - 0.06, min(deep) + min(ahead, 0.0) - 0.06)
        high = (at + half, max(side) + width + gap + 0.06, max(deep) + max(ahead, 0.0) + 0.06)
        out.append(box(Mesh(finish), low, high).moved(frame, centre))
        if finish == 'blued':
            for k in (0.25, 0.75):
                d = low[2] + (high[2] - low[2]) * k
                out.append(box(Mesh('worn'), (at - 0.09, high[1], d - 0.09),
                               (at + 0.09, high[1] + 0.07, d + 0.09)).moved(frame, centre))
    return out


def extended_magazine(gun, pistol=False):
    """WWII's longer magazine in j_clip's space: a second magazine clamped beside an SMG's own when it hangs
    under the gun, else its own magazine longer."""
    mag = _magazine(gun)
    if not mag:
        return []
    if not pistol and gun.weapon in DUAL_MAGAZINES and _fed_from_below(gun, mag):
        return dual_magazine(gun, mag)
    return stretched_magazine(gun, mag)


def box_magazine(gun):
    """WWII's Extended Mag for a rifle loaded through the top, in j_gun's space: a blued box magazine down out of
    the stock where BOX_MAGAZINES has it, its floorplate, ribs pressed in its sides and a catch at its back."""
    back, front, top, bottom, width = BOX_MAGAZINES[gun.weapon]
    half = width / 2
    body = box(Mesh('blued'), (back, -half, bottom + 0.12), (front, half, top))
    plate = box(Mesh('worn'), (back - 0.06, -half - 0.05, bottom), (front + 0.06, half + 0.05, bottom + 0.14))
    ribs = Mesh('parkerized')
    for share in (0.33, 0.67):
        x = back + (front - back) * share
        for side in (-1.0, 1.0):
            box(ribs, (x - 0.05, side * half - 0.03, bottom + 0.3), (x + 0.05, side * half + 0.03, top - 0.35))
    catch = box(Mesh('worn'), (back - 0.14, -0.2, top - 0.6), (back, 0.2, top - 0.25))
    return [body, plate, ribs, catch]


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
    'rust': ((88, 60, 41), 0.5, (126, 86, 55)),
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


# ---------------------------------------------------------------- the gun's own metal

def material_textures(data):
    """A material's images by the sampler they fill: colorMap, detailMap, normalMap, specularMap."""
    count, _, table = struct.unpack_from('<III', data, 0x34)

    def string(offset):
        return data[offset:data.index(b'\0', offset)].decode('latin-1')
    return {string(name): string(image) for name, _, image in
            (struct.unpack_from('<III', data, table + 12 * k) for k in range(count & 0xffff))}


def renamed_material(data, name):
    """The material as it is under another name: the name appended and the header's name offset on it."""
    out = bytearray(data) + name.encode('ascii') + b'\0'
    struct.pack_into('<I', out, 0x00, len(data))
    return bytes(out)


def _rgb565(value):
    return ((value >> 11) & 31) * 255 // 31, ((value >> 5) & 63) * 255 // 63, (value & 31) * 255 // 31


def _dxt_colours(block, offset, four):
    c0, c1, bits = struct.unpack_from('<HHI', block, offset)
    a, b = _rgb565(c0), _rgb565(c1)
    if four or c0 > c1:
        palette = (a, b, tuple((2 * x + y) // 3 for x, y in zip(a, b)), tuple((x + 2 * y) // 3 for x, y in zip(a, b)))
    else:
        palette = (a, b, tuple((x + y) // 2 for x, y in zip(a, b)), (0, 0, 0))
    return [palette[(bits >> (2 * i)) & 3] for i in range(16)]


def read_iwi(data, side):
    """An IWI's largest mip level whose larger side is at most side, but no smaller than the third, as rows of
    (r, g, b): ARGB32, DXT1 or DXT5."""
    if data[:3] != b'IWi' or data[3] != 5:
        raise ValueError('Unexpected IWI')
    fmt = data[4]
    width, height = struct.unpack_from('<HH', data, 6)
    ends = struct.unpack_from('<4i', data, 12)
    level = 0
    while max(width, height) > side and level < 2:
        level, width, height = level + 1, max(1, width // 2), max(1, height // 2)
    blob = data[ends[level + 1]:ends[level]]
    rows = [[None] * width for _ in range(height)]
    if fmt in (11, 13):
        step, at = (8, 0) if fmt == 11 else (16, 8)
        k = 0
        for by in range(0, height, 4):
            for bx in range(0, width, 4):
                colours = _dxt_colours(blob, k + at, fmt == 13)
                k += step
                for i, colour in enumerate(colours):
                    x, y = bx + i % 4, by + i // 4
                    if x < width and y < height:
                        rows[y][x] = colour
    elif fmt == 1:
        for y in range(height):
            for x in range(width):
                b, g, r = blob[4 * (y * width + x):4 * (y * width + x) + 3]
                rows[y][x] = (r, g, b)
    else:
        raise ValueError('Unexpected IWI format %d' % fmt)
    return rows


def _luminance(rgb):
    return 299 * rgb[0] + 587 * rgb[1] + 114 * rgb[2]


def _triangles(gun, surfaces, keep=None):
    """The corner vertices of the triangles of those surfaces that keep takes, of their positions in j_gun's space."""
    for index in surfaces:
        vertices, triangles = gun.surfaces[index]['vertices'], gun.surfaces[index]['triangles']
        for i in range(0, len(triangles), 3):
            corners = [vertices[v] for v in triangles[i:i + 3]]
            if keep is None or keep([_in_gun(gun, v) for v in corners]):
                yield corners


def _coverage(gun, surfaces, width, height, keep=None):
    """The texels painted at width by height by those surfaces' triangles that keep takes."""
    painted = [[False] * width for _ in range(height)]
    for corners in _triangles(gun, surfaces, keep):
        (ax, ay), (bx, by), (cx, cy) = ((v['uv'][0] * width, v['uv'][1] * height) for v in corners)
        area = (bx - ax) * (cy - ay) - (cx - ax) * (by - ay)
        if abs(area) < 1e-9:
            continue
        for y in range(max(0, int(math.floor(min(ay, by, cy)))), min(height, int(math.ceil(max(ay, by, cy))))):
            for x in range(max(0, int(math.floor(min(ax, bx, cx)))), min(width, int(math.ceil(max(ax, bx, cx))))):
                px, py = x + 0.5, y + 0.5
                w0 = ((bx - px) * (cy - py) - (cx - px) * (by - py)) / area
                w1 = ((cx - px) * (ay - py) - (ax - px) * (cy - py)) / area
                if w0 >= 0.0 and w1 >= 0.0 and w0 + w1 <= 1.0:
                    painted[y][x] = True
    return painted


def _detail(rows):
    """How far each texel's luminance stands from the mean of its 3 by 3 neighbourhood; 0 on the border."""
    light = [[_luminance(texel) for texel in row] for row in rows]
    height, width = len(light), len(light[0])
    out = [[0] * width for _ in range(height)]
    for y in range(1, height - 1):
        for x in range(1, width - 1):
            out[y][x] = abs(light[y][x] - sum(light[y + j][x + i] for j in (-1, 0, 1) for i in (-1, 0, 1)) // 9)
    return out


def _in_gun(gun, vertex):
    q, t = gun.bones.get(gun.names[vertex['bone']], (S.IDENTITY_QUAT, (0.0, 0.0, 0.0)))
    return S._add(S._qrot(q, vertex['pos']), t)


class Metal:
    """Where a gun's sight parts take its metal: its material, renamed, and the patch of its colour map they
    sample, metal that is plain, shiny and of the receiver's tone beneath the sight, with its texel density."""

    def __init__(self, gun, read, mount):
        cx, cy, top = mount
        beneath = lambda points: all(abs(p[0] - cx) < 2.0 and abs(p[1] - cy) < 1.5 and p[2] > top - 1.5 for p in points)
        counts = [sum(1 for _ in _triangles(gun, [i], beneath)) for i in range(len(gun.surfaces))]
        if not any(counts):
            counts = [len(surface['triangles']) for surface in gun.surfaces]
        # The material of the surface most under the sight, and every surface drawn in it, one texture's atlas.
        material = gun.materials[max(range(len(counts)), key=lambda i: counts[i])]
        surfaces = [i for i, m in enumerate(gun.materials) if m == material]
        self.source = 'materials/' + material
        self.name = MATERIAL + gun.model
        textures = material_textures(read(self.source))
        colour_file = read('images/' + textures['colorMap'] + '.iwi')
        self.size = struct.unpack_from('<HH', colour_file, 6)
        colour = read_iwi(colour_file, PATCH_MIP)
        height, width = len(colour), len(colour[0])
        spec = read_iwi(read('images/' + textures['specularMap'] + '.iwi'), PATCH_MIP) if 'specularMap' in textures \
            else None
        light = [[_luminance(texel) for texel in row] for row in colour]
        shine = [[_luminance(spec[y * len(spec) // height][x * len(spec[0]) // width]) if spec else 127500
                  for x in range(width)] for y in range(height)]
        painted = _coverage(gun, surfaces, width, height)
        under = _coverage(gun, surfaces, width, height, beneath)
        tones = sorted(light[y][x] for y in range(height) for x in range(width) if under[y][x]) or \
            sorted(light[y][x] for y in range(height) for x in range(width) if painted[y][x])
        target = tones[len(tones) // 2]
        shines = sorted(shine[y][x] for y in range(height) for x in range(width) if painted[y][x])
        n = PATCH_TEXELS
        # Metal, not wood: shinier than four tenths of the gun's upper quartile.
        metal = n * n * shines[3 * len(shines) // 4] * 2 // 5
        candidates = []
        for y in range(height - n + 1):
            for x in range(width - n + 1):
                cells = [(yy, xx) for yy in range(y, y + n) for xx in range(x, x + n)]
                if not all(painted[yy][xx] for yy, xx in cells):
                    continue
                gleam = sum(shine[yy][xx] for yy, xx in cells)
                if gleam < metal:
                    continue
                total = sum(light[yy][xx] for yy, xx in cells)
                squares = sum(light[yy][xx] ** 2 for yy, xx in cells)
                spread = math.isqrt(n * n * squares - total * total)
                edges = sum(abs(light[yy][xx] - light[yy][xx + 1]) + abs(light[yy][xx] - light[yy + 1][xx])
                            for yy in range(y, y + n - 1) for xx in range(x, x + n - 1)) * n * n // (2 * (n - 1) ** 2)
                receiver = sum(under[yy][xx] for yy, xx in cells)
                # A screw head or a seam: the texel furthest from the patch's mean.
                peak = max(abs(n * n * light[yy][xx] - total) for yy, xx in cells)
                # All in sums over the patch: the receiver's tone and an even surface first, a little for shine and
                # for the receiver itself.
                score = (gleam // 4 - 4 * abs(total - n * n * target) - 2 * spread - edges - peak +
                         receiver * 12750)
                candidates.append((-score, y, x))
        if not candidates:
            raise ValueError(gun.model + ' has no plain patch of metal')
        candidates.sort()
        fine = read_iwi(colour_file, 2 * PATCH_MIP)
        scale = len(fine) // height
        detail = _detail(fine)
        limit = int(PATCH_DETAIL * 255000)
        checked = []
        for _, y, x in candidates[:PATCH_CANDIDATES]:
            values = sorted(detail[yy][xx] for yy in range(y * scale, (y + n) * scale)
                            for xx in range(x * scale, (x + n) * scale))
            checked.append((values[9 * len(values) // 10], y, x))
            if checked[-1][0] < limit:
                break
        _, y, x = checked[-1] if checked[-1][0] < limit else min(checked)
        self.origin = (x / width, y / height)
        self.extent = (n / width, n / height)
        # Texels per inch on the gun at full size, the median over its triangles.
        densities = []
        for corners in _triangles(gun, surfaces):
            a, b, c = (_in_gun(gun, v) for v in corners)
            normal = S._cross(S._sub(b, a), S._sub(c, a))
            area = math.sqrt(S._dot(normal, normal)) / 2
            (u0, v0), (u1, v1), (u2, v2) = (v['uv'] for v in corners)
            texels = abs((u1 - u0) * (v2 - v0) - (u2 - u0) * (v1 - v0)) / 2 * self.size[0] * self.size[1]
            if area > 1e-4 and texels > 0:
                densities.append(math.sqrt(texels / area))
        self.density = round(_percentile(densities, 0.5), 2) if densities else 32.0

    def uv(self, position, normal):
        """A point's UV on the patch: its position, in inches across the face its normal most points out of, at
        the gun's density, folded back and forth inside the patch."""
        axis = max(range(3), key=lambda k: abs(normal[k]))
        across = [position[k] for k in range(3) if k != axis]
        out = []
        for k in range(2):
            inset = 0.5 / PATCH_TEXELS
            t = across[k] * self.density / (self.extent[k] * self.size[k]) % 2.0
            folded = t if t <= 1.0 else 2.0 - t
            out.append(self.origin[k] + self.extent[k] * (inset + folded * (1.0 - 2 * inset)))
        return tuple(out)


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


def mesh_model(name, parts, bone, metal=None):
    """An xmodel of one bone from meshes, a surface for each finish, and retail surfaces as they are; meshes in
    the gun's metal take its renamed material and UVs on its patch, and picture-only meshes stay out of it."""
    by_finish = {}
    surfaces, materials = [], []
    for part in parts:
        if isinstance(part, Surface):
            if part.triangles:
                surfaces.append((part.vertices, part.triangles))
                materials.append(part.material)
        elif not part.picture_only:
            by_finish.setdefault(part.finish, []).append(part)
    for finish in FINISHES + (GUN_METAL,):
        group = by_finish.get(finish)
        if not group:
            continue
        vertices, triangles = [], []
        for mesh in group:
            start = len(vertices)
            vertices += mesh.rigid_vertices(metal.uv if finish == GUN_METAL else None)
            triangles += [start + v for v in mesh.triangles]
        surfaces.append((vertices, triangles))
        materials.append(metal.name if finish == GUN_METAL else MATERIAL + finish)
    return rigid_model(name, surfaces, materials, bone)


def sight_mount(gun):
    """Where the Reflex and Lens Sights stand: their centre's x and y, and the gun's top beneath it."""
    centre = S._add(gun.eye, S._scale(gun.ahead, OPTIC_AHEAD.get(gun.weapon, 7.5)))
    top, _ = _under_opening(gun, centre, REFLEX_INNER)
    return centre[0], centre[1], top


def gun_attachments(read, weapon, takes):
    """The archive entries for one weapon's visible attachments and the names of those it shows."""
    gun = Gun(read, weapon)
    pistol = weapon in PISTOLS
    entries, shown = {}, []
    base = gun.model + SUFFIX
    metal = None
    for attachment in VISIBLE:
        if attachment not in takes:
            continue
        if attachment == 'suppressor':
            entries.update(mesh_model(base + attachment, suppressor(gun, pistol), RIDES[attachment]))
        elif attachment == 'extmag' and weapon in BOX_MAGAZINES:
            entries.update(mesh_model(base + EXTBOX, box_magazine(gun), 'j_gun'))
        elif attachment == 'extmag':
            if weapon not in MAGAZINES:
                continue
            magazine = extended_magazine(gun, pistol)
            if not magazine:
                continue
            entries.update(mesh_model(base + attachment, magazine, RIDES[attachment]))
            surfaces, materials = without_magazine(gun)
            lod = base + NOMAG + '0'
            entries['xmodel/' + base + NOMAG] = _copy_xmodel(gun, lod, materials)
            entries['xmodelparts/' + lod] = gun.parts
            entries['xmodelsurfs/' + lod] = write_surfaces(surfaces)
        elif attachment == 'optic4x':
            if gun.weapon not in OPTIC_SOURCES:
                continue
            entries.update(mesh_model(base + attachment, optic4x(gun, read), RIDES[attachment]))
            glass = optic4x_glass(gun, read)
            if glass:
                entries[GLASS + base + attachment] = glass.encode()
        else:
            if metal is None:
                metal = Metal(gun, read, sight_mount(gun))
                entries['materials/' + metal.name] = renamed_material(read(metal.source), metal.name)
            meshes = {'reflex': reflex, 'lens': lens}[attachment](gun)
            entries.update(mesh_model(base + attachment, meshes, RIDES[attachment], metal))
            entries[GLASS + base + attachment] = sight_glass(gun, attachment).encode()
        shown.append(attachment)
    return entries, shown


def _copy_xmodel(gun, lod, materials):
    bounds = gun.xmodel['bounds']
    points = [b[:3] for b in bounds] + [b[3:] for b in bounds]
    return E.write_xmodel(lod, materials, bounds, _radius(points))


PISTOLS = ('colt_mp', 'webley_mp', 'TT30_mp', 'luger_mp')


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
