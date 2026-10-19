#!/usr/bin/env python3
"""Render the class guns' pictures and their attachments' layers from the retail first-person guns,
into the owner's class pictures folder, as CoD WWII's menus show its guns: muzzle to the left, seen a
little from above, lit on nothing.

Every picture of a gun shares one canvas and camera, framed to hold the gun with any attachment it
takes, so the menus lay the layers over the gun and frame what is shown:
- <weapon>.png, the gun;
- <weapon>__extmag.png, the gun with its longer magazine in place of its own;
- <weapon>__suppressor.png, __reflex.png, __lens.png and __optic4x.png, each attachment alone where
  it sits on the gun, hidden where the gun stands in front of it.
The geometry is browser_attachment_models.py's, the same the first-person guns wear in a match.
attachment__highcal.png, a cartridge for High Caliber, joins the icons cut from the owner's captures.

Needs numpy, Pillow and scipy, which the archives' preparation does not:
    /usr/bin/python3 scripts/render-class-pictures.py [--only mp44_mp ...]
"""
import argparse
import json
import math
import struct
import sys
import zipfile
from pathlib import Path

import numpy as np
from PIL import Image
from scipy import ndimage as ndi

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'scripts'))
import browser_attachment_models as B  # noqa: E402
import browser_smine as S  # noqa: E402

YAW, PITCH, FOV = 165.0, 12.0, 18.0
SUPERSAMPLE = 3
MARGIN = 0.03
LIGHTS = [
    (np.array([-0.62, -0.50, 0.60]), np.array([1.55, 1.36, 1.10])),
    (np.array([0.80, -0.30, 0.10]), np.array([0.22, 0.24, 0.28])),
    (np.array([0.55, 0.75, 0.40]), np.array([0.85, 0.74, 0.58])),
    (np.array([-0.50, 0.70, 0.30]), np.array([0.40, 0.36, 0.30])),
]
AMBIENT = np.array([0.13, 0.12, 0.11])
# Specular and gloss of the generated finishes, which the game draws without a specular map.
FINISH_SHINE = {'blued': (0.45, 0.55), 'parkerized': (0.22, 0.3), 'worn': (0.5, 0.55), 'brass': (0.85, 0.7),
                'glass': (1.0, 0.95), 'black': (0.05, 0.2)}
LAYERS = ('suppressor', 'reflex', 'lens', 'optic4x')
# Parts the reload animations bring in from out of view, parked away from the gun when it rests.
LOOSE = {'enfield_mp': ('joint1',), 'enfield_scope_mp': ('joint1',), 'mosin_nagant_mp': ('j_clip',),
         'mosin_nagant_sniper_mp': ('j_clip',), 'kar98k_sniper_mp': ('j_round',),
         'webley_mp': ('j_quickloader', 'j_quickloader_bullets')}


class Assets:
    def __init__(self, folder):
        self.index, self.zips = {}, {}
        for path in sorted(folder.glob('*.iwd')):
            archive = self.zips[path] = zipfile.ZipFile(path)
            for name in archive.namelist():
                if not name.endswith('/'):
                    self.index[name.lower()] = (path, name)

    def read(self, name):
        path, real = self.index[name.lower()]
        return self.zips[path].read(real)


def _rgb565(c):
    return np.array([((c >> 11) & 31) * 255 // 31, ((c >> 5) & 63) * 255 // 63, (c & 31) * 255 // 31], np.int32)


def decode_iwi(data):
    """The top mip of an IWi as RGBA floats."""
    if data[:3] != b'IWi' or data[3] != 5:
        raise ValueError('Unexpected IWi')
    fmt = data[4]
    w, h = struct.unpack_from('<2H', data, 6)
    if fmt in (11, 12, 13):
        bw, bh = (w + 3) // 4, (h + 3) // 4
        size = 8 if fmt == 11 else 16
        top = np.frombuffer(data[len(data) - bw * bh * size:], np.uint8).reshape(bh * bw, size)
        colour = top[:, -8:]
        c0 = colour[:, 0].astype(np.int32) | colour[:, 1].astype(np.int32) << 8
        c1 = colour[:, 2].astype(np.int32) | colour[:, 3].astype(np.int32) << 8
        bits = (colour[:, 4].astype(np.uint32) | colour[:, 5].astype(np.uint32) << 8 |
                colour[:, 6].astype(np.uint32) << 16 | colour[:, 7].astype(np.uint32) << 24)
        p0 = np.stack([(c0 >> 11 & 31) * 255 // 31, (c0 >> 5 & 63) * 255 // 63, (c0 & 31) * 255 // 31], 1)
        p1 = np.stack([(c1 >> 11 & 31) * 255 // 31, (c1 >> 5 & 63) * 255 // 63, (c1 & 31) * 255 // 31], 1)
        four = (fmt != 11) | (c0 > c1)
        p2 = np.where(four[:, None], (2 * p0 + p1) // 3, (p0 + p1) // 2)
        p3 = np.where(four[:, None], (p0 + 2 * p1) // 3, 0)
        palette = np.stack([p0, p1, p2, p3], 1)
        index = (bits[:, None] >> (2 * np.arange(16, dtype=np.uint32))) & 3
        rgb = np.take_along_axis(palette, index[..., None].astype(np.int64).repeat(3, 2), 1)
        alpha = np.full((bh * bw, 16), 255, np.int32)
        if fmt == 11:
            alpha = np.where((~four[:, None]) & (index == 3), 0, 255)
        elif fmt == 12:
            nib = top[:, :8]
            alpha = np.stack([(nib[:, i >> 1] >> ((i & 1) * 4) & 15) * 17 for i in range(16)], 1)
        else:
            a0, a1 = top[:, 0].astype(np.int32), top[:, 1].astype(np.int32)
            abits = np.zeros(len(top), np.uint64)
            for k in range(6):
                abits |= top[:, 2 + k].astype(np.uint64) << np.uint64(8 * k)
            sel = ((abits[:, None] >> (3 * np.arange(16, dtype=np.uint64))) & np.uint64(7)).astype(np.int64)
            eight = a0 > a1
            table = np.zeros((len(top), 8), np.int32)
            table[:, 0], table[:, 1] = a0, a1
            for i in range(1, 7):
                table[:, 1 + i] = np.where(eight, ((7 - i) * a0 + i * a1) // 7, 0)
            for i in range(1, 5):
                table[:, 1 + i] = np.where(eight, table[:, 1 + i], ((5 - i) * a0 + i * a1) // 5)
            table[:, 6] = np.where(eight, table[:, 6], 0)
            table[:, 7] = np.where(eight, table[:, 7], 255)
            alpha = np.take_along_axis(table, sel, 1)
        texels = np.concatenate([rgb, alpha[..., None]], 2).reshape(bh, bw, 4, 4, 4)
        out = texels.transpose(0, 2, 1, 3, 4).reshape(bh * 4, bw * 4, 4)[:h, :w]
    elif fmt == 1:
        out = np.frombuffer(data[len(data) - w * h * 4:], np.uint8).reshape(h, w, 4)[..., [2, 1, 0, 3]]
    elif fmt == 2:
        rgb = np.frombuffer(data[len(data) - w * h * 3:], np.uint8).reshape(h, w, 3)[..., [2, 1, 0]]
        out = np.dstack([rgb, np.full((h, w), 255, np.uint8)])
    else:
        raise ValueError(f'IWi format {fmt}')
    return out.astype(np.float32) / 255.0


def material_textures(assets, material, cache={}):
    if material in cache:
        return cache[material]
    data = assets.read('materials/' + material)

    def string(offset):
        return data[offset:data.index(b'\0', offset)].decode('latin-1')
    count = struct.unpack_from('<H', data, 52)[0]
    technique, table = struct.unpack_from('<II', data, 56)
    textures = {}
    for i in range(count):
        name, _, _, _, _, image = struct.unpack_from('<IBBBBI', data, table + 12 * i)
        key, image_name = string(name), string(image)
        if image_name.startswith('$') or key == 'detailMap':
            continue
        textures[key] = decode_iwi(assets.read('images/' + image_name + '.iwi'))
    cache[material] = (textures, 'alphatest' in string(technique))
    return cache[material]


def quat_matrix(q):
    x, y, z, w = q
    return np.array([[1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)],
                     [2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)],
                     [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)]])


def gun_part(assets, gun, surface, material, loose=()):
    """A retail surface posed in j_gun's space, without the triangles of loose parts."""
    vertices = surface['vertices']
    rot = np.array([quat_matrix(gun.bones[gun.names[v['bone']]][0]) for v in vertices])
    off = np.array([gun.bones[gun.names[v['bone']]][1] for v in vertices])

    def turned(key, moved=False):
        values = np.einsum('nij,nj->ni', rot, np.array([v[key] for v in vertices], np.float64))
        return values + off if moved else values
    textures, alphatest = material_textures(assets, material)
    tris = np.array(surface['triangles'], np.int64).reshape(-1, 3)
    if loose:
        kept = np.array([gun.names[v['bone']] not in loose for v in vertices])
        tris = tris[kept[tris].all(1)]
    return {'pos': turned('pos', True), 'nrm': turned('normal'), 'tan': turned('binormal'),
            'bin': turned('tangent'), 'uv': np.array([v['uv'] for v in vertices], np.float64),
            'tris': tris, 'tex': textures, 'alphatest': alphatest}


def placed(part, pose):
    rot, off = quat_matrix(pose[0]), np.array(pose[1], np.float64)
    out = dict(part)
    out['pos'] = part['pos'] @ rot.T + off
    for key in ('nrm', 'tan', 'bin'):
        out[key] = part[key] @ rot.T
    return out


def finish_textures(finish, cache={}):
    if finish not in cache:
        pixels = np.array(B.texture_pixels(finish), np.float32) / 255.0
        spec, gloss = FINISH_SHINE[finish]
        shine = np.ones_like(pixels)
        shine[..., :3] *= spec
        shine[..., 3] = gloss
        cache[finish] = {'colorMap': pixels, 'specularMap': shine}
    return cache[finish]


def mesh_part(mesh):
    vertices = mesh.rigid_vertices()
    return {'pos': np.array([v[5] for v in vertices], np.float64), 'nrm': np.array([v[0] for v in vertices]),
            'tan': np.array([v[3] for v in vertices]), 'bin': np.array([v[4] for v in vertices]),
            'uv': np.array([v[2] for v in vertices], np.float64),
            'tris': np.array(mesh.triangles, np.int64).reshape(-1, 3), 'tex': finish_textures(mesh.finish),
            'alphatest': False}


def magazine_parts(assets, gun):
    """The longer magazine in j_gun's space, in the gun's own textures."""
    clip = gun.bones['j_clip']
    parts = []
    for index, vertices, triangles in B.extended_magazine(gun):
        textures, alphatest = material_textures(assets, gun.materials[index])
        part = {'pos': np.array([v[5] for v in vertices], np.float64), 'nrm': np.array([v[0] for v in vertices]),
                'tan': np.array([v[3] for v in vertices]), 'bin': np.array([v[4] for v in vertices]),
                'uv': np.array([v[2] for v in vertices], np.float64),
                'tris': np.array(triangles, np.int64).reshape(-1, 3), 'tex': textures, 'alphatest': alphatest}
        parts.append(placed(part, clip))
    return parts


def layer_parts(gun, layer, pistol):
    if layer == 'suppressor':
        return [placed(mesh_part(m), gun.bones['tag_flash']) for m in B.suppressor(gun, pistol)]
    return [mesh_part(m) for m in {'reflex': B.reflex, 'lens': B.lens, 'optic4x': B.optic4x}[layer](gun)]


def rotation(yaw, pitch):
    cy, sy = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    cp, sp = math.cos(math.radians(pitch)), math.sin(math.radians(pitch))
    rz = np.array([[cy, -sy, 0], [sy, cy, 0], [0, 0, 1]])
    rx = np.array([[1, 0, 0], [0, cp, -sp], [0, sp, cp]])
    return rx @ rz


class Camera:
    """Far off down +y at the turned gun; focal and centre fit the framing points in the canvas."""

    def __init__(self, points, width, height, yaw=YAW, pitch=PITCH, lean=0.0, fill=1 - 2 * MARGIN):
        self.rot = rotation(yaw, pitch)
        if lean:
            c, s = math.cos(math.radians(lean)), math.sin(math.radians(lean))
            self.rot = np.array([[c, 0, -s], [0, 1, 0], [s, 0, c]]) @ self.rot
        turned = points @ self.rot.T
        self.centre = (turned.min(0) + turned.max(0)) / 2
        radius = np.linalg.norm(turned - self.centre, axis=1).max()
        self.distance = radius / math.sin(math.radians(FOV / 2)) * 1.05
        self.width, self.height = width, height
        x, y, _ = self._project(turned - self.centre, 1.0, 0.0, 0.0)
        span_x, span_y = x.max() - x.min(), y.max() - y.min()
        self.focal = min(width * fill / span_x, height * fill / span_y)
        self.cx = width / 2 - self.focal * (x.max() + x.min()) / 2
        self.cy = height / 2 - self.focal * (y.max() + y.min()) / 2

    def _project(self, turned, focal, cx, cy):
        depth = self.distance + turned[:, 1]
        return turned[:, 0] * focal / depth + cx, -turned[:, 2] * focal / depth + cy, depth

    def project(self, points, ss):
        turned = points @ self.rot.T - self.centre
        x, y, depth = self._project(turned, self.focal * ss, self.cx * ss, self.cy * ss)
        return x, y, depth, turned

    def eye(self):
        return np.array([0.0, -self.distance, 0.0])


def rasterize(parts, camera, ss):
    W, H = camera.width * ss, camera.height * ss
    buffers = {'z': np.full((H, W), np.inf, np.float32), 'id': np.full((H, W), -1, np.int32),
               'n': np.zeros((H, W, 3), np.float32), 't': np.zeros((H, W, 3), np.float32),
               'b': np.zeros((H, W, 3), np.float32), 'uv': np.zeros((H, W, 2), np.float32)}
    zbuf, idbuf = buffers['z'], buffers['id']
    for pid, part in enumerate(parts):
        x, y, depth, turned = camera.project(part['pos'], ss)
        nrm, tan, bin_ = (part[k] @ camera.rot.T for k in ('nrm', 'tan', 'bin'))
        tris = part['tris']
        if not len(tris):
            continue
        # Cull back faces, as the game does with the retail winding.
        face = np.cross(turned[tris[:, 1]] - turned[tris[:, 0]], turned[tris[:, 2]] - turned[tris[:, 0]])
        tris = tris[np.einsum('ij,ij->i', face, camera.eye() - turned[tris[:, 0]]) < 0]
        colour = part['tex'].get('colorMap') if part['alphatest'] else None
        for i0, i1, i2 in tris:
            xs, ys = np.array([x[i0], x[i1], x[i2]]), np.array([y[i0], y[i1], y[i2]])
            minx, maxx = max(int(math.floor(xs.min())), 0), min(int(math.ceil(xs.max())), W - 1)
            miny, maxy = max(int(math.floor(ys.min())), 0), min(int(math.ceil(ys.max())), H - 1)
            if minx > maxx or miny > maxy:
                continue
            area = (xs[1] - xs[0]) * (ys[2] - ys[0]) - (xs[2] - xs[0]) * (ys[1] - ys[0])
            if abs(area) < 1e-9:
                continue
            gx, gy = np.meshgrid(np.arange(minx, maxx + 1) + 0.5, np.arange(miny, maxy + 1) + 0.5)
            w0 = ((xs[1] - gx) * (ys[2] - gy) - (xs[2] - gx) * (ys[1] - gy)) / area
            w1 = ((xs[2] - gx) * (ys[0] - gy) - (xs[0] - gx) * (ys[2] - gy)) / area
            w2 = 1 - w0 - w1
            inside = (w0 >= 0) & (w1 >= 0) & (w2 >= 0)
            if not inside.any():
                continue
            iz = np.array([1 / depth[i0], 1 / depth[i1], 1 / depth[i2]])
            pw = np.stack([w0 * iz[0], w1 * iz[1], w2 * iz[2]], -1)
            z = 1 / pw.sum(-1)
            pw = pw * z[..., None]
            yy, xx = np.nonzero(inside)
            yy, xx = yy + miny, xx + minx
            zi, wsel = z[inside], pw[inside]
            closer = zi < zbuf[yy, xx]
            if colour is not None:
                closer &= sample(colour, wsel @ part['uv'][[i0, i1, i2]])[:, 3] >= 0.5
            if not closer.any():
                continue
            yy, xx, zi, wsel = yy[closer], xx[closer], zi[closer], wsel[closer]
            zbuf[yy, xx] = zi
            idbuf[yy, xx] = pid
            buffers['n'][yy, xx] = wsel @ nrm[[i0, i1, i2]]
            buffers['t'][yy, xx] = wsel @ tan[[i0, i1, i2]]
            buffers['b'][yy, xx] = wsel @ bin_[[i0, i1, i2]]
            buffers['uv'][yy, xx] = wsel @ part['uv'][[i0, i1, i2]]
    return buffers


def sample(texture, uv):
    h, w = texture.shape[:2]
    u = (uv[:, 0] % 1.0) * w - 0.5
    v = (uv[:, 1] % 1.0) * h - 0.5
    x0, y0 = np.floor(u).astype(int), np.floor(v).astype(int)
    fx, fy = (u - x0)[:, None], (v - y0)[:, None]
    x0, y0 = x0 % w, y0 % h
    x1, y1 = (x0 + 1) % w, (y0 + 1) % h
    return ((texture[y0, x0] * (1 - fx) + texture[y0, x1] * fx) * (1 - fy) +
            (texture[y1, x0] * (1 - fx) + texture[y1, x1] * fx) * fy)


def shade(parts, buffers, visible):
    """RGBA where visible, lit as the class pictures are."""
    H, W = visible.shape
    colour = np.zeros((H, W, 3), np.float32)
    yy, xx = np.nonzero(visible)
    if len(yy):
        ids = buffers['id'][yy, xx]
        n = buffers['n'][yy, xx].astype(np.float64)
        n /= np.linalg.norm(n, axis=1, keepdims=True) + 1e-9
        base = np.full((len(yy), 3), 0.5)
        spec = np.full((len(yy), 3), 0.25)
        gloss = np.full(len(yy), 0.4)
        for pid, part in enumerate(parts):
            sel = ids == pid
            if not sel.any():
                continue
            uv = buffers['uv'][yy[sel], xx[sel]].astype(np.float64)
            tex = part['tex']
            if 'colorMap' in tex:
                base[sel] = sample(tex['colorMap'], uv)[:, :3]
            if 'specularMap' in tex:
                s = sample(tex['specularMap'], uv)
                spec[sel], gloss[sel] = s[:, :3], s[:, 3]
            if 'normalMap' in tex:
                nm = sample(tex['normalMap'], uv)
                nx, ny = nm[:, 3] * 2 - 1, nm[:, 1] * 2 - 1
                nz = np.sqrt(np.clip(1 - nx * nx - ny * ny, 0, 1))
                t = buffers['t'][yy[sel], xx[sel]].astype(np.float64)
                b = buffers['b'][yy[sel], xx[sel]].astype(np.float64)
                t /= np.linalg.norm(t, axis=1, keepdims=True) + 1e-9
                b /= np.linalg.norm(b, axis=1, keepdims=True) + 1e-9
                bent = t * nx[:, None] + b * ny[:, None] + n[sel] * nz[:, None]
                n[sel] = bent / (np.linalg.norm(bent, axis=1, keepdims=True) + 1e-9)
        view = np.array([0.0, -1.0, 0.0])
        lit, shine = base * AMBIENT, np.zeros_like(base)
        for direction, intensity in LIGHTS:
            light = direction / np.linalg.norm(direction)
            ndl = np.clip(n @ light, 0, 1)[:, None]
            lit = lit + base * ndl * intensity
            half = (light + view) / np.linalg.norm(light + view)
            shine = shine + spec * (np.clip(n @ half, 0, 1) ** (8 + gloss * 120))[:, None] * intensity * (ndl > 0)
        rgb = lit + shine * 1.4
        colour[yy, xx] = np.clip(rgb / (1 + rgb * 0.35), 0, 1)
    return np.dstack([colour, visible.astype(np.float32)])


def downsample(image, ss):
    H, W = image.shape[:2]
    pre = image.copy()
    pre[..., :3] *= pre[..., 3:4]
    pre = pre.reshape(H // ss, ss, W // ss, ss, 4).mean((1, 3))
    alpha = pre[..., 3:4]
    rgb = np.where(alpha > 0, pre[..., :3] / np.maximum(alpha, 1e-6), 0)
    return np.dstack([rgb, alpha[..., 0]])


def bleed(rgba):
    """8-bit RGBA whose clear texels take the colour of the nearest visible one."""
    out = np.clip(rgba * 255 + 0.5, 0, 255).astype(np.uint8)
    visible = out[..., 3] > 0
    if visible.any():
        idx = ndi.distance_transform_edt(~visible, return_distances=False, return_indices=True)
        out[..., :3] = np.where(visible[..., None], out[..., :3], out[idx[0], idx[1], :3])
    return out


def save(rgba, path):
    Image.fromarray(bleed(rgba)).save(path, optimize=True)


def render_gun(assets, weapon, takes, out, pistol):
    gun = B.Gun(assets.read, weapon)
    loose = LOOSE.get(weapon, ())
    base = [gun_part(assets, gun, s, m, loose) for s, m in zip(gun.surfaces, gun.materials)]
    layers = {layer: layer_parts(gun, layer, pistol) for layer in LAYERS if layer in takes and
              not (layer == 'optic4x' and weapon in B.SNIPERS)}
    magazine = magazine_parts(assets, gun) if 'extmag' in takes and weapon in B.MAGAZINES else []
    framing = np.concatenate([p['pos'][np.unique(p['tris'])] for group in [base, magazine] + list(layers.values())
                              for p in group if len(p['tris'])])
    width, height = (512, 256) if pistol else (1024, 256)
    camera = Camera(framing, width, height)
    ss = SUPERSAMPLE
    gun_buffers = rasterize(base, camera, ss)
    save(downsample(shade(base, gun_buffers, gun_buffers['id'] >= 0), ss), out / f'{weapon.lower()}.png')
    written = [weapon.lower()]
    if magazine:
        surfaces, materials = B.without_magazine(gun)
        variant = [gun_part(assets, gun, s, m, loose) for s, m in zip(surfaces, materials)] + magazine
        buffers = rasterize(variant, camera, ss)
        save(downsample(shade(variant, buffers, buffers['id'] >= 0), ss), out / f'{weapon.lower()}__extmag.png')
        written.append('extmag')
    for layer, parts in layers.items():
        buffers = rasterize(parts, camera, ss)
        visible = (buffers['id'] >= 0) & (buffers['z'] < gun_buffers['z'])
        save(downsample(shade(parts, buffers, visible), ss), out / f'{weapon.lower()}__{layer}.png')
        written.append(layer)
    return written


def render_cartridge(out, size=128):
    """High Caliber's icon: a long rifle cartridge, brass case and copper ball, as WWII's FMJ icon
    stands, seen from the left and a little above."""
    case = B.lathe(B.Mesh('brass'), [(-2.6, 0.0), (-2.6, 0.30), (-2.5, 0.33), (-2.38, 0.33), (-2.34, 0.29),
                                     (-2.22, 0.29), (-2.18, 0.33), (0.5, 0.31), (0.75, 0.2), (1.2, 0.18)],
                   segments=40)
    ball = B.Mesh('brass')
    B.lathe(ball, [(1.2, 0.18), (1.7, 0.175), (2.2, 0.13), (2.55, 0.06), (2.7, 0.0)], segments=40)
    parts = [mesh_part(case)]
    copper = mesh_part(ball)
    tex = copper['tex']
    copper['tex'] = {'colorMap': tex['colorMap'] * np.array([1.05, 0.72, 0.58, 1.0], np.float32),
                     'specularMap': tex['specularMap']}
    parts.append(copper)
    camera = Camera(np.concatenate([p['pos'] for p in parts]), size, size, yaw=150.0, pitch=18.0, lean=-28.0,
                    fill=0.78)
    buffers = rasterize(parts, camera, 4)
    save(downsample(shade(parts, buffers, buffers['id'] >= 0), 4), out / 'attachment__highcal.png')


def main():
    parser = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    parser.add_argument('--data', type=Path, default=ROOT / 'data/main')
    parser.add_argument('--only', nargs='*', default=None)
    args = parser.parse_args()
    np.seterr(all='ignore')
    assets = Assets(args.data)
    classes = json.loads((ROOT / 'downstream/gateway/classes.json').read_text())
    out = args.data / 'owner_class_pictures'
    out.mkdir(parents=True, exist_ok=True)
    for weapon in classes['primary'] + classes['secondary']:
        if args.only and weapon not in args.only:
            continue
        takes = set(classes['weaponAttachments'].get(weapon, ()))
        written = render_gun(assets, weapon, takes, out, weapon in B.PISTOLS)
        print(weapon, ' '.join(written), flush=True)
    if not args.only:
        render_cartridge(out)
        print('attachment__highcal')


if __name__ == '__main__':
    main()
