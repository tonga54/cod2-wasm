"""The S-Mine 44 as CoD WWII shows it, built from nothing but numbers.

A green canister with a yellow band and three detonator plugs on its lid,
and the igniter standing out of it: a rusty tube, a green collar and stem
and three thin prongs. In the hand its safety pin rides the Mk 2 grenade's
pin bone, so the retail pull-pin animation's left hand draws it out.
Units are inches, z up; the canister's foot is 1.5 below the origin, where
a resting grenade's origin stands above the ground.
"""
import math
import struct

WHITE = b'\xff\xff\xff\xff'
FOOT = -1.5
CAN_RADIUS = 2.0
CAN_TOP = 3.45
TEXTURE_SIZE = 256

# Atlas regions, in texels of the square texture: (x0, y0, x1, y1).
SIDE = (0, 0, 256, 128)
LID_CENTRE = (64.0, 192.0)
LID_TEXELS = 58.0
PLAIN = (136, 136, 160, 160)
RIM = (168, 136, 200, 168)
TUBE = (136, 176, 200, 248)
GREEN = (208, 136, 248, 200)
DARK = (208, 208, 248, 228)
METAL = (208, 236, 248, 252)

# The canister's side runs down the side strip from its top (v 0) to its
# bevel (v 1); the band is a yellow ring a little above the foot.
SIDE_TOP = 3.2
SIDE_BOTTOM = FOOT + 0.15
BAND = (-0.95, -0.35)
PLUG_RADIUS = 0.42
PLUG_RING = 1.12
PLUG_ANGLES = (90.0, 210.0, 330.0)
SEGMENTS = 28


def _uv(region, fu, fv):
    x0, y0, x1, y1 = region
    return ((x0 + 0.5 + (x1 - x0 - 1) * fu) / TEXTURE_SIZE, (y0 + 0.5 + (y1 - y0 - 1) * fv) / TEXTURE_SIZE)


def _side_uv(fu, z):
    return _uv(SIDE, fu, (SIDE_TOP - z) / (SIDE_TOP - SIDE_BOTTOM))


def _lid_uv(x, y):
    return ((LID_CENTRE[0] + x / CAN_RADIUS * LID_TEXELS) / TEXTURE_SIZE,
            (LID_CENTRE[1] - y / CAN_RADIUS * LID_TEXELS) / TEXTURE_SIZE)


class Mesh:
    """Vertices as the xmodelsurfs writer takes them: normal, colour, uv,
    binormal, tangent and position; triangles wound as the originals are."""

    def __init__(self):
        self.vertices = []
        self.triangles = []

    def vertex(self, point, normal, uv, tangent=(1.0, 0.0, 0.0)):
        n = _normalise(normal)
        t = _normalise(_sub(tangent, _scale(n, _dot(tangent, n)))) if abs(_dot(tangent, n)) < 0.99 else _perpendicular(n)
        b = _cross(n, t)
        self.vertices.append((n, WHITE, uv, b, t, tuple(point)))
        return len(self.vertices) - 1

    def face(self, a, b, c):
        """A triangle whose (v1 - v0) x (v2 - v0) points into the solid, as the
        retail meshes' do, whatever order it was given in."""
        pa, pb, pc = (self.vertices[i][5] for i in (a, b, c))
        normal = _add(_add(self.vertices[a][0], self.vertices[b][0]), self.vertices[c][0])
        if _dot(_cross(_sub(pb, pa), _sub(pc, pa)), normal) > 0.0:
            b, c = c, b
        self.triangles += [a, b, c]

    def quad(self, a, b, c, d):
        self.face(a, b, c)
        self.face(a, c, d)

    def extend(self, other):
        base = len(self.vertices)
        self.vertices += other.vertices
        self.triangles += [index + base for index in other.triangles]

    def transformed(self, rotation, scale, offset):
        out = Mesh()
        for n, colour, uv, b, t, p in self.vertices:
            out.vertices.append((_turn(rotation, n), colour, uv, _turn(rotation, b), _turn(rotation, t),
                                 _add(_scale(_turn(rotation, p), scale), offset)))
        out.triangles = list(self.triangles)
        return out


def _add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def _sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def _scale(a, s):
    return (a[0] * s, a[1] * s, a[2] * s)


def _dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def _cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def _normalise(a):
    length = math.sqrt(_dot(a, a))
    return _scale(a, 1.0 / length) if length > 1e-12 else (0.0, 0.0, 1.0)


def _perpendicular(n):
    return _normalise(_cross(n, (0.0, 1.0, 0.0) if abs(n[1]) < 0.9 else (1.0, 0.0, 0.0)))


def _turn(rotation, v):
    return tuple(sum(rotation[row][column] * v[column] for column in range(3)) for row in range(3))


def lathe(mesh, profile, uv_of, segments=SEGMENTS, centre=(0.0, 0.0), phase=0.0):
    """A body of revolution about z: each consecutive pair of (radius, z)
    points is a band with its own vertices, so bands meet at hard edges;
    uv_of(band, fu, along, radius, z) maps a point of a band."""
    cx, cy = centre
    for band in range(len(profile) - 1):
        (r0, z0), (r1, z1) = profile[band], profile[band + 1]
        dr, dz = r1 - r0, z1 - z0
        length = math.hypot(dr, dz)
        if length < 1e-9:
            continue
        nr, nz = dz / length, -dr / length
        rows = []
        for segment in range(segments + 1):
            fu = segment / segments
            angle = 2.0 * math.pi * fu + phase
            c, s = math.cos(angle), math.sin(angle)
            row = []
            for along, (r, z) in ((0.0, (r0, z0)), (1.0, (r1, z1))):
                point = (cx + c * r, cy + s * r, z)
                normal = (c * nr, s * nr, nz) if abs(nz) < 0.999 else (0.0, 0.0, nz)
                row.append(mesh.vertex(point, normal, uv_of(band, fu, along, r, z, c, s), (-s, c, 0.0)))
            rows.append(row)
        for segment in range(segments):
            a, b = rows[segment]
            c, d = rows[segment + 1]
            if r0 < 1e-6:
                mesh.face(a, b, d)
            elif r1 < 1e-6:
                mesh.face(a, b, c)
            else:
                mesh.quad(a, b, d, c)


def _region_uv(region):
    return lambda band, fu, along, r, z, c, s: _uv(region, fu, along)


def _flat_uv(region):
    return lambda band, fu, along, r, z, c, s: _uv(region, 0.5 + 0.45 * c * min(r, 1.0), 0.5 + 0.45 * s * min(r, 1.0))


def canister():
    mesh = Mesh()
    # Foot bevel, side, the lid's crimped rim and the lid, with the rim in metal.
    profile = ((0.0, FOOT), (1.86, FOOT), (CAN_RADIUS, SIDE_BOTTOM), (CAN_RADIUS, SIDE_TOP), (2.07, 3.3),
               (2.07, 3.46), (1.97, 3.57), (1.84, 3.57), (1.79, CAN_TOP), (0.0, CAN_TOP))

    def uv_of(band, fu, along, r, z, c, s):
        if band == 0:
            return _uv(PLAIN, 0.5 + 0.4 * c * r / CAN_RADIUS, 0.5 + 0.4 * s * r / CAN_RADIUS)
        if band in (1, 2):
            return _side_uv(fu, z)
        if band in (3, 4, 5, 6, 7):
            return _uv(RIM, fu, along)
        return _lid_uv(c * r, s * r)
    lathe(mesh, profile, uv_of)
    for angle in PLUG_ANGLES:
        a = math.radians(angle)
        centre = (math.cos(a) * PLUG_RING, math.sin(a) * PLUG_RING)

        def plug_uv(band, fu, along, r, z, c, s, centre=centre):
            if band == 0:
                return _uv(GREEN, fu, 0.3 + 0.4 * along)
            return _lid_uv(centre[0] + c * r, centre[1] + s * r)
        lathe(mesh, ((PLUG_RADIUS, CAN_TOP - 0.02), (PLUG_RADIUS, CAN_TOP + 0.14), (PLUG_RADIUS - 0.06, CAN_TOP + 0.2),
                     (0.0, CAN_TOP + 0.2)), plug_uv, segments=14, centre=centre)
    return mesh


# The igniter: nut, rusty tube, collar, stem and cap, bottom to top.
IGNITER = ((0.56, CAN_TOP - 0.02), (0.56, 3.72), (0.34, 3.72), (0.34, 4.95), (0.47, 4.95), (0.47, 5.5),
           (0.37, 5.5), (0.37, 6.42), (0.4, 6.42), (0.4, 6.62), (0.0, 6.62))
IGNITER_REGIONS = (DARK, DARK, TUBE, TUBE, GREEN, GREEN, GREEN, DARK, DARK, DARK)
PIN_HEIGHT = 5.95
PRONG_BASE = 6.62
PRONG_TIP = (0.62, 7.95)


def igniter():
    mesh = Mesh()

    def uv_of(band, fu, along, r, z, c, s):
        region = IGNITER_REGIONS[band]
        return _uv(region, fu, along if region is not TUBE else (z - 3.72) / (4.95 - 3.72))
    lathe(mesh, IGNITER, uv_of, segments=16)
    # Three thin prongs fanning out of the cap.
    for k in range(3):
        a = math.radians(30.0 + 120.0 * k)
        out = (math.cos(a), math.sin(a), 0.0)
        side = (-math.sin(a), math.cos(a), 0.0)
        base = _add(_scale(out, 0.16), (0.0, 0.0, PRONG_BASE - 0.05))
        tip = _add(_scale(out, PRONG_TIP[0]), (0.0, 0.0, PRONG_TIP[1]))
        axis = _normalise(_sub(tip, base))
        across = _normalise(_cross(axis, side))
        corners = []
        for t, half in ((0.0, 0.075), (1.0, 0.04)):
            centre = _add(base, _scale(_sub(tip, base), t))
            corners.append([_add(_add(centre, _scale(side, sx * half)), _scale(across, sy * half))
                            for sx, sy in ((-1, -1), (1, -1), (1, 1), (-1, 1))])
        for face in range(4):
            i, j = face, (face + 1) % 4
            normal = _normalise(_add(_sub(corners[0][i], base), _sub(corners[0][j], base)))
            quad = [mesh.vertex(p, normal, _uv(GREEN, fu, fv), axis)
                    for p, fu, fv in ((corners[0][i], 0.2, 0.9), (corners[0][j], 0.8, 0.9),
                                      (corners[1][j], 0.8, 0.1), (corners[1][i], 0.2, 0.1))]
            mesh.quad(*quad)
        cap = [mesh.vertex(p, axis, _uv(GREEN, 0.5, 0.1), side) for p in corners[1]]
        mesh.quad(*cap)
    return mesh


def body():
    mesh = canister()
    mesh.extend(igniter())
    return mesh


def safety_pin():
    """The pin through the stem and the ring it is drawn by, its middle at
    the origin and drawn out along -x."""
    mesh = Mesh()
    metal = _region_uv(METAL)
    # The pin along x, through the stem and out on the ring's side.
    rows = []
    for k in range(9):
        a = 2.0 * math.pi * k / 8
        c, s = math.cos(a), math.sin(a)
        rows.append([mesh.vertex((x, 0.055 * c, 0.055 * s), (0.0, c, s), _uv(METAL, k / 8, f), (1.0, 0.0, 0.0))
                     for x, f in ((-0.62, 0.0), (0.52, 1.0))])
    for k in range(8):
        mesh.quad(rows[k][0], rows[k][1], rows[k + 1][1], rows[k + 1][0])
    # The ring, hanging in the xz plane beyond the pin's head.
    major, minor = 0.3, 0.045
    centre = (-0.62 - major, 0.0, -0.05)
    grid = []
    for i in range(17):
        u = 2.0 * math.pi * i / 16
        cu, su = math.cos(u), math.sin(u)
        ring = []
        for j in range(7):
            v = 2.0 * math.pi * j / 6
            cv, sv = math.cos(v), math.sin(v)
            normal = (cu * cv, sv, su * cv)
            point = (centre[0] + cu * (major + minor * cv), minor * sv, centre[2] + su * (major + minor * cv))
            ring.append(mesh.vertex(point, normal, metal(0, i / 16, j / 6, 0, 0, 0, 0), (-su, 0.0, cu)))
        grid.append(ring)
    for i in range(16):
        for j in range(6):
            mesh.quad(grid[i][j], grid[i + 1][j], grid[i + 1][j + 1], grid[i][j + 1])
    return mesh


# The texture: worn green paint, a yellow band, stencils, rust and metal.
def _hash(x, y, seed):
    h = (int(x) * 374761393 + int(y) * 668265263 + seed * 2147483647) & 0xffffffff
    h = ((h ^ (h >> 13)) * 1274126177) & 0xffffffff
    return ((h ^ (h >> 16)) & 0xffff) / 65535.0


def _noise(x, y, cell, seed, wrap=None):
    """Smooth value noise in [0, 1]; wrap repeats it every wrap texels in x."""
    gx, gy = x / cell, y / cell
    x0, y0 = int(math.floor(gx)), int(math.floor(gy))
    fx, fy = gx - x0, gy - y0
    fx, fy = fx * fx * (3 - 2 * fx), fy * fy * (3 - 2 * fy)

    def at(i, j):
        if wrap:
            i %= max(1, int(round(wrap / cell)))
        return _hash(i, j, seed)
    top = at(x0, y0) + (at(x0 + 1, y0) - at(x0, y0)) * fx
    bottom = at(x0, y0 + 1) + (at(x0 + 1, y0 + 1) - at(x0, y0 + 1)) * fx
    return top + (bottom - top) * fy


def _fbm(x, y, seed, wrap=None):
    return (0.5 * _noise(x, y, 32.0, seed, wrap) + 0.3 * _noise(x, y, 12.0, seed + 1, wrap) +
            0.2 * _noise(x, y, 4.0, seed + 2, wrap))


def _mix(a, b, t):
    return tuple(a[i] + (b[i] - a[i]) * t for i in range(3))


PAINT = (100.0, 146.0, 30.0)
PAINT_DARK = (58.0, 86.0, 20.0)
GRIME = (58.0, 60.0, 34.0)
YELLOW = (180.0, 152.0, 40.0)
RUST = (92.0, 56.0, 34.0)
RUST_DARK = (46.0, 32.0, 24.0)
IRON = (52.0, 48.0, 42.0)
STEEL = (168.0, 168.0, 160.0)
# Five by seven capitals for the stencilling, one row a string.
_GLYPHS = {
    'S': ('01111', '10000', '10000', '01110', '00001', '00001', '11110'),
    'M': ('10001', '11011', '10101', '10101', '10001', '10001', '10001'),
    'I': ('01110', '00100', '00100', '00100', '00100', '00100', '01110'),
    '.': ('00000', '00000', '00000', '00000', '00000', '01100', '01100'),
    '4': ('00010', '00110', '01010', '10010', '11111', '00010', '00010'),
    ' ': ('00000',) * 7,
}


def _stencil(text, x, y, column, row):
    """Whether the texel at (column, row) is inked by text set from (x, y)."""
    if row < y or row >= y + 7:
        return False
    offset = column - x
    if offset < 0 or offset >= 6 * len(text):
        return False
    glyph = _GLYPHS[text[offset // 6]]
    return offset % 6 < 5 and glyph[row - y][offset % 6] == '1'


def _paint(x, y, wrap=None):
    wear = _fbm(x, y, 11, wrap)
    colour = _mix(PAINT_DARK, PAINT, min(1.0, max(0.0, 0.25 + wear)))
    grime = _fbm(x * 1.7, y * 1.7, 23, wrap)
    if grime > 0.62:
        colour = _mix(colour, GRIME, min(0.6, (grime - 0.62) * 2.4))
    speck = _hash(x, y, 5)
    if speck > 0.985:
        colour = _mix(colour, GRIME, 0.5)
    return colour


def texture_pixels():
    size = TEXTURE_SIZE
    pixels = [[(0, 0, 0, 255)] * size for _ in range(size)]
    band_top = (SIDE_TOP - BAND[1]) / (SIDE_TOP - SIDE_BOTTOM) * (SIDE[3] - SIDE[1])
    band_bottom = (SIDE_TOP - BAND[0]) / (SIDE_TOP - SIDE_BOTTOM) * (SIDE[3] - SIDE[1])
    for y in range(size):
        for x in range(size):
            if y < SIDE[3]:
                colour = _paint(x, y, wrap=SIDE[2])
                if band_top <= y < band_bottom:
                    colour = _mix(YELLOW, colour, 0.25 * _fbm(x, y, 31, SIDE[2]))
                # A darker crust toward the foot, from standing in the dirt.
                colour = _mix(colour, GRIME, max(0.0, (y - 96) / 32.0) * 0.45)
                for start in (20, 148):
                    if _stencil('S.MI.44', start, 38, x, y):
                        colour = _mix(colour, (30.0, 30.0, 24.0), 0.85)
                # Paint worn back to steel along the crimp.
                if y < 4 and _hash(x, y, 7) > 0.5:
                    colour = _mix(colour, STEEL, 0.4)
            elif x < 130:
                dx, dy = x + 0.5 - LID_CENTRE[0], y + 0.5 - LID_CENTRE[1]
                colour = _paint(x * 1.3, y * 1.3)
                radius = math.hypot(dx, dy) / LID_TEXELS
                if radius > 0.86:
                    colour = _mix(colour, PAINT_DARK, min(1.0, (radius - 0.86) * 4.0))
                for angle in PLUG_ANGLES:
                    a = math.radians(angle)
                    px = LID_CENTRE[0] + math.cos(a) * PLUG_RING / CAN_RADIUS * LID_TEXELS
                    py = LID_CENTRE[1] - math.sin(a) * PLUG_RING / CAN_RADIUS * LID_TEXELS
                    ex, ey = x + 0.5 - px, y + 0.5 - py
                    distance = math.hypot(ex, ey) / (PLUG_RADIUS / CAN_RADIUS * LID_TEXELS)
                    if distance < 1.0:
                        colour = _mix(colour, PAINT, 0.35)
                        # A screwdriver slot across each plug.
                        slot = abs(ex * math.sin(a + 0.6) - ey * math.cos(a + 0.6))
                        if slot < 1.1 and distance < 0.85:
                            colour = (26.0, 30.0, 18.0)
                    elif distance < 1.18:
                        colour = _mix(colour, GRIME, 0.55)
                if _stencil('S.MI.44', 44, 220, x, y):
                    colour = _mix(colour, (30.0, 30.0, 24.0), 0.7)
            else:
                colour = _paint(x, y)
                if RIM[0] <= x < RIM[2] and RIM[1] <= y < RIM[3]:
                    colour = _mix(IRON, (110.0, 104.0, 92.0), _fbm(x * 2, y * 2, 41))
                elif TUBE[0] <= x < TUBE[2] and TUBE[1] <= y < TUBE[3]:
                    colour = _mix(RUST_DARK, RUST, _fbm(x * 2, y * 2, 43))
                elif DARK[0] <= x < DARK[2] and DARK[1] <= y < DARK[3]:
                    colour = _mix(IRON, RUST_DARK, _fbm(x * 2, y * 2, 47))
                elif METAL[0] <= x < METAL[2] and METAL[1] <= y < METAL[3]:
                    colour = _mix((120.0, 120.0, 114.0), STEEL, _fbm(x * 3, y * 3, 53))
                elif GREEN[0] <= x < GREEN[2] and GREEN[1] <= y < GREEN[3]:
                    colour = _mix(PAINT, (126.0, 166.0, 50.0), 0.35 * _fbm(x * 2, y * 2, 59))
            pixels[y][x] = tuple(max(0, min(255, int(round(c)))) for c in colour) + (255,)
    return pixels


def write_model_material(template, name, image):
    """A lit model material for one colour image, laid out as the Mk 2
    grenade's: phong_replace over the image and the identity normal map."""
    count, technique, table = struct.unpack_from('<III', template, 0x34)
    if template[technique:technique + 14] != b'phong_replace\0' or table != 0x44 or count != 2:
        raise ValueError('Unexpected model material template')
    entries = [struct.unpack_from('<III', template, 0x44 + 12 * k) for k in range(2)]
    def string(offset):
        return template[offset:template.index(b'\0', offset)]
    if [string(entry[0]) for entry in entries] != [b'colorMap', b'normalMap'] or string(entries[1][2]) != b'$identitynormalmap':
        raise ValueError('Unexpected model material textures')
    head = bytearray(template[:0x5c])
    strings = bytearray()

    def add(text):
        offset = 0x5c + len(strings)
        strings.extend(text + b'\0')
        return offset
    technique_at = add(b'phong_replace')
    name_at = add(name.encode('ascii'))
    image_at = add(image.encode('ascii'))
    colour_at = add(b'colorMap')
    normal_at = add(b'normalMap')
    identity_at = add(b'$identitynormalmap')
    struct.pack_into('<II', head, 0x00, name_at, image_at)
    struct.pack_into('<HH', head, 0x1c, TEXTURE_SIZE, TEXTURE_SIZE)
    struct.pack_into('<I', head, 0x38, technique_at)
    struct.pack_into('<I', head, 0x40, technique_at)
    struct.pack_into('<III', head, 0x44, colour_at, entries[0][1], image_at)
    struct.pack_into('<III', head, 0x50, normal_at, entries[1][1], identity_at)
    return bytes(head) + bytes(strings)


# Viewmodel animations (xanim version 14), every track expanded per frame.
def _quat_from_shorts(a, b, c):
    w = math.sqrt(max(0.0, 0x3fff0001 - a * a - b * b - c * c))
    return (a / 32767.0, b / 32767.0, c / 32767.0, w / 32767.0)


def read_xanim(data):
    """Frames per second, frame count and each bone's (name, quaternions,
    translations): None, one value, or one per frame, keys interpolated."""
    position = [0]

    def take(fmt):
        value = struct.unpack_from(fmt, data, position[0])
        position[0] += struct.calcsize(fmt)
        return value if len(value) > 1 else value[0]
    if take('<h') != 14:
        raise ValueError('Unexpected xanim version')
    frames, bone_count, flags, fps = take('<H'), take('<H'), take('<B'), take('<h')
    if flags & 3:
        raise ValueError('Looping and delta animations are not expected here')
    small = frames <= 0x100

    def indices(count):
        if count >= frames:
            return list(range(count))
        if small:
            values = list(data[position[0]:position[0] + count])
            position[0] += count
            return values
        return [take('<H') for _ in range(count)]

    def expand(keys, values, lerp):
        out = []
        for frame in range(frames):
            if frame <= keys[0]:
                out.append(values[0])
                continue
            for k in range(len(keys) - 1):
                if keys[k] <= frame <= keys[k + 1]:
                    t = (frame - keys[k]) / (keys[k + 1] - keys[k])
                    out.append(lerp(values[k], values[k + 1], t))
                    break
            else:
                out.append(values[-1])
        return out
    flip = simple = b''
    if bone_count:
        size = ((bone_count - 1) >> 3) + 1
        flip = data[position[0]:position[0] + size]
        simple = data[position[0] + size:position[0] + 2 * size]
        position[0] += 2 * size
    names = []
    for _ in range(bone_count):
        end = data.index(b'\0', position[0])
        names.append(data[position[0]:end].decode('latin-1'))
        position[0] = end + 1
    bones = []
    for index, name in enumerate(names):
        is_simple = (simple[index >> 3] >> (index & 7)) & 1
        is_flipped = (flip[index >> 3] >> (index & 7)) & 1
        count = take('<H')
        quats = None
        if count:
            keys = indices(count) if count > 1 else [0]
            values = []
            for _ in range(count):
                a, b, c = (0, 0, take('<h')) if is_simple else take('<3h')
                values.append(_quat_from_shorts(a, b, c))
            if count == 1 and is_flipped:
                values = [tuple(-v for v in values[0])]
            quats = values if count == 1 else expand(keys, values, _nlerp)
        count = take('<H')
        trans = None
        if count:
            keys = indices(count) if count > 1 else [0]
            values = [take('<3f') for _ in range(count)]
            trans = values if count == 1 else expand(keys, values, _lerp)
        bones.append((name, quats, trans))
    notes = take('<B')
    if notes:
        raise ValueError('Notetracks are not expected here')
    if position[0] != len(data):
        raise ValueError('Unexpected xanim size')
    return {'fps': fps, 'frames': frames, 'bones': bones}


def _lerp(a, b, t):
    return tuple(a[i] + (b[i] - a[i]) * t for i in range(3))


def _nlerp(a, b, t):
    if sum(a[i] * b[i] for i in range(4)) < 0.0:
        b = tuple(-v for v in b)
    q = tuple(a[i] + (b[i] - a[i]) * t for i in range(4))
    length = math.sqrt(sum(v * v for v in q))
    return tuple(v / length for v in q)


def _short(value):
    return max(-32767, min(32767, int(round(value * 32767.0))))


def write_xanim(anim):
    """A non-looping xanim: each track keyed once when it holds still, else
    every frame; quaternions with w >= 0, which the loader rebuilds."""
    frames, bones = anim['frames'], anim['bones']
    size = ((len(bones) - 1) >> 3) + 1 if bones else 0
    data = struct.pack('<hHHBh', 14, frames, len(bones), 0, anim['fps'])
    data += bytes(size) * 2
    data += b''.join(name.encode('latin-1') + b'\0' for name, _, _ in bones)
    for name, quats, trans in bones:
        if quats is None:
            data += struct.pack('<H', 0)
        else:
            if len(quats) not in (1, frames):
                raise ValueError('A quaternion track needs one value or one per frame: ' + name)
            shorts = []
            for q in quats:
                if q[3] < 0.0:
                    q = tuple(-v for v in q)
                shorts.append((_short(q[0]), _short(q[1]), _short(q[2])))
            if all(s == shorts[0] for s in shorts):
                shorts = shorts[:1]
            data += struct.pack('<H', len(shorts)) + b''.join(struct.pack('<3h', *s) for s in shorts)
        if trans is None:
            data += struct.pack('<H', 0)
        else:
            if len(trans) not in (1, frames):
                raise ValueError('A translation track needs one value or one per frame: ' + name)
            values = [tuple(float(v) for v in t) for t in trans]
            packed = [struct.pack('<3f', *v) for v in values]
            if all(p == packed[0] for p in packed):
                packed = packed[:1]
            data += struct.pack('<H', len(packed)) + b''.join(packed)
    return data + b'\0'


# Rigid transforms (q, t): a point p goes to q p q* + t.
def _qmul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by, aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw, aw * bw - ax * bx - ay * by - az * bz)


def _qconj(q):
    return (-q[0], -q[1], -q[2], q[3])


def _qrot(q, v):
    return _qmul(_qmul(q, (v[0], v[1], v[2], 0.0)), _qconj(q))[:3]


def _compose(a, b):
    return (_qmul(a[0], b[0]), _add(_qrot(a[0], b[1]), a[1]))


def _invert(a):
    q = _qconj(a[0])
    return (q, _scale(_qrot(q, a[1]), -1.0))


def _between(a, b):
    """The shortest rotation taking unit vector a to unit vector b."""
    axis = _cross(a, b)
    w = 1.0 + _dot(a, b)
    if w < 1e-6:
        axis, w = _perpendicular(a), 0.0
    length = math.sqrt(_dot(axis, axis) + w * w)
    return (axis[0] / length, axis[1] / length, axis[2] / length, w / length)


def _about(axis, degrees):
    half = math.radians(degrees) * 0.5
    axis = _normalise(axis)
    return (axis[0] * math.sin(half), axis[1] * math.sin(half), axis[2] * math.sin(half), math.cos(half))


def _smooth(t):
    t = max(0.0, min(1.0, t))
    return t * t * (3.0 - 2.0 * t)


IDENTITY_QUAT = (0.0, 0.0, 0.0, 1.0)


def _track(bones, name):
    for bone, quats, trans in bones:
        if bone == name:
            return quats, trans
    raise ValueError('The animation has no ' + name)


def _at(values, time, lerp):
    """A per-frame or constant track's value at a fractional frame."""
    if values is None:
        return None
    if len(values) == 1:
        return values[0]
    time = max(0.0, min(len(values) - 1.0, time))
    low = int(math.floor(time))
    high = min(low + 1, len(values) - 1)
    return lerp(values[low], values[high], time - low)


def _local(bones, name, time):
    quats, trans = _track(bones, name)
    q = _at(quats, time, _nlerp) or IDENTITY_QUAT
    t = _at(trans, time, _lerp) or (0.0, 0.0, 0.0)
    return (q, t)


def _left_arm(name):
    return name.endswith('_le') or '_le_' in name or name[-4:-1] == '_le'


# The Mk 2's pull-pin: its grenade comes up into the right hand by frame 5,
# the left hand works the pin until frame 12 and draws it by 14; then it
# winds the grenade back to throw, which the S-Mine never does.
PULL_FRAMES = 15
HOLD_FRAMES = 7
PLANT_FRAMES = 11


def derive_pullpin(mk2, reach, hold_centre, hold_axis, hold_twist, mine_centre):
    """The S-Mine's pull-pin from the Mk 2's: the same hands until the pin
    is out, the left one reaching `reach` (in the grenade's frame) further
    to find the S-Mine's pin; then, while the left hand drops the pin out of
    sight as the Mk 2's does, the right hand stands the mine upright beside
    the view, its centre at hold_centre and its igniter along hold_axis in
    the torso's frame."""
    bones = mk2['bones']
    last = mk2['frames'] - 1
    frames = PULL_FRAMES + HOLD_FRAMES
    cut = PULL_FRAMES - 1

    def source_time(frame):
        if frame <= cut:
            return float(frame)
        return cut + (frame - cut) / HOLD_FRAMES * (last - cut)
    weapon_cut = _local(bones, 'tag_weapon', cut)
    axis_now = _qrot(weapon_cut[0], (0.0, 0.0, 1.0))
    turn = _qmul(_about(hold_axis, hold_twist), _between(_normalise(axis_now), _normalise(hold_axis)))
    centre_now = _add(_qrot(weapon_cut[0], mine_centre), weapon_cut[1])
    hold = (turn, _sub(hold_centre, _qrot(turn, centre_now)))

    def hold_at(frame):
        k = _smooth((frame - cut) / HOLD_FRAMES) if frame > cut else 0.0
        q = _nlerp(IDENTITY_QUAT, hold[0], k)
        return (q, _scale(hold[1], k))

    def reach_at(time):
        # The left hand's offset, full while it works the pin and gone once
        # it has dropped it out of sight.
        weight = _smooth((time - 2.0) / 4.0) * (1.0 - _smooth((time - cut) / (last - cut)))
        return _scale(_qrot(_local(bones, 'tag_weapon', time)[0], reach), weight)
    out = []
    for name, quats, trans in bones:
        new_q, new_t = [], []
        for frame in range(frames):
            time = source_time(frame)
            if name == 'j_clip':
                weapon_new = _compose(hold_at(frame), _local(bones, 'tag_weapon', min(time, cut)))
                pin = _compose(_local(bones, 'tag_weapon', time), _local(bones, 'j_clip', time))
                pin = (pin[0], _add(pin[1], reach_at(time)))
                local = _compose(_invert(weapon_new), pin)
            elif name in ('tag_weapon', 'j_shoulder_ri'):
                local = _compose(hold_at(frame), _local(bones, name, min(time, cut)))
            elif _left_arm(name):
                local = _local(bones, name, time)
                if name == 'j_shoulder_le':
                    local = (local[0], _add(local[1], reach_at(time)))
            else:
                local = _local(bones, name, min(time, cut))
            new_q.append(local[0])
            new_t.append(local[1])
        out.append((name, new_q if quats is not None else None, new_t if trans is not None else None))
    return {'fps': mk2['fps'], 'frames': frames, 'bones': out}


def derive_plant(pullpin, drop):
    """From the upright hold, the hand sets the mine down out of sight:
    everything sinks by `drop` (in the view's frame), slowly, then faster."""
    last = pullpin['frames'] - 1
    out = []
    for name, quats, trans in pullpin['bones']:
        q = [quats[-1]] if quats is not None else None
        t = [trans[-1]] if trans is not None else None
        if name == 'tag_torso':
            base = trans[-1] if trans is not None else (0.0, 0.0, 0.0)
            t = [_add(base, _scale(drop, (frame / (PLANT_FRAMES - 1)) ** 2)) for frame in range(PLANT_FRAMES)]
        out.append((name, q, t))
    del last
    return {'fps': pullpin['fps'], 'frames': PLANT_FRAMES, 'bones': out}


# The middle of the Mk 2's pin ring in its pin bone's frame, which the
# pull-pin animation's left fingers close on.
MK2_RING = (-0.24, 0.135, -0.45)
CAN_MIDDLE = 0.5 * (FOOT + CAN_TOP)


def viewmodel(mk2_pullpin, scale, centre, twist, pull, hold_centre, hold_axis, hold_twist, drop):
    """The S-Mine in the hand: its body in the Mk 2's grenade frame, centred
    on `centre`, its pin in the pin bone's frame, and the animations moving
    them. The pin leaves the stem along `pull`, toward the left hand."""
    spin = ((math.cos(math.radians(twist)), -math.sin(math.radians(twist)), 0.0),
            (math.sin(math.radians(twist)), math.cos(math.radians(twist)), 0.0), (0.0, 0.0, 1.0))
    origin = _sub(centre, (0.0, 0.0, scale * CAN_MIDDLE))
    held = body().transformed(spin, scale, origin)
    hole = _add(origin, (0.0, 0.0, scale * PIN_HEIGHT))
    pull = _normalise((pull[0], pull[1], 0.0))
    side = _cross((0.0, 0.0, 1.0), pull)
    # The pin mesh's -x along the pull, z up the stem.
    pin_turn = ((-pull[0], side[0], 0.0), (-pull[1], side[1], 0.0), (0.0, 0.0, 1.0))
    pin_gun = safety_pin().transformed(pin_turn, scale, hole)
    ring_gun = _add(hole, _scale(_turn(pin_turn, (-0.92, 0.0, -0.05)), scale))
    rest = _local(mk2_pullpin['bones'], 'j_clip', 0)
    reach = _sub(ring_gun, _add(_qrot(rest[0], MK2_RING), rest[1]))
    # In the pin bone's frame, displaced by the reach the clip's track gains.
    inverse = _invert((rest[0], _add(rest[1], reach)))
    pin = Mesh()
    pin.triangles = list(pin_gun.triangles)
    for n, colour, uv, b, t, p in pin_gun.vertices:
        pin.vertices.append((_qrot(inverse[0], n), colour, uv, _qrot(inverse[0], b), _qrot(inverse[0], t),
                             _add(_qrot(inverse[0], p), inverse[1])))
    pullpin = derive_pullpin(mk2_pullpin, reach, hold_centre, hold_axis, hold_twist, centre)
    plant = derive_plant(pullpin, drop)
    return {'body': held, 'pin': pin, 'pullpin': pullpin, 'plant': plant}
