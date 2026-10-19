#!/usr/bin/env python3
"""Exercise the owner's class pictures: the PNGs the generator accepts, the IWis it writes, the loader that draws
them through $raw with their mips, and the class menus that prefer them to HUD silhouettes."""
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import types
import zlib

root = Path(__file__).resolve().parent.parent
module_path = root / 'scripts/browser_class_pictures.py'
module_source = module_path.read_text()
hud2d = (root / 'src/PC/gfx_d3d/r_hud2d.c').read_text()
hud2d_header = (root / 'src/headers/PC/gfx_d3d/r_hud2d.h').read_text()
menu = (root / 'src/PC/ui_mp/ui_class_menu.c').read_text()
class_header = (root / 'src/headers/PC/ui_mp/ui_class_data.h').read_text()
rules = (root / 'src/PC/bgame/bg_attachment_rules.c').read_text()
COUNTS = {}


def load(source):
    module = types.ModuleType('browser_class_pictures')
    exec(compile(source, str(module_path), 'exec'), module.__dict__)
    return module


def raises(call, message=''):
    try:
        call()
    except ValueError as caught:
        assert message in str(caught), (message, str(caught))
        return
    raise AssertionError('expected ValueError')


def paeth(a, b, c):
    p = a + b - c
    pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
    return a if pa <= pb and pa <= pc else b if pb <= pc else c


def png(width, height, rgba, filters=(0,), depth=8, color=6, interlace=0, damage=None):
    """An RGBA PNG whose rows cycle through filters; damage edits (chunks, idat) before writing."""
    stride = width * 4
    rows = []
    for y in range(height):
        line, up = rgba[y * stride:(y + 1) * stride], rgba[(y - 1) * stride:y * stride] if y else bytes(stride)
        kind = filters[y % len(filters)]
        out = bytearray([kind])
        for x in range(stride):
            left, above, corner = line[x - 4] if x >= 4 else 0, up[x], up[x - 4] if x >= 4 else 0
            predictor = (0, left, above, (left + above) // 2, paeth(left, above, corner))[kind] if kind < 5 else 0
            out.append((line[x] - predictor) & 255)
        rows.append(bytes(out))
    idat = zlib.compress(b''.join(rows))
    chunks = [[b'IHDR', struct.pack('>IIBBBBB', width, height, depth, color, 0, 0, interlace)],
              [b'IDAT', idat[:len(idat) // 2]], [b'IDAT', idat[len(idat) // 2:]], [b'IEND', b'']]
    if damage:
        chunks = damage(chunks)

    def chunk(kind, body, crc=None):
        crc = zlib.crc32(kind + body) if crc is None else crc
        return struct.pack('>I', len(body)) + kind + body + struct.pack('>I', crc)
    return b'\x89PNG\r\n\x1a\n' + b''.join(chunk(*item) for item in chunks)


def noise(count, seed):
    state, out = seed, bytearray()
    for _ in range(count):
        state = (state * 1103515245 + 12345) & 0x7fffffff
        out.append(state >> 16 & 255)
    return out


def picture(width, height, seed, box=None, faint=10):
    """Noisy texels, opaque inside box = (x0, y0, x1, y1) and faint outside it."""
    texels = noise(width * height * 4, seed)
    for y in range(height):
        for x in range(width):
            inside = box is None or box[0] <= x <= box[2] and box[1] <= y <= box[3]
            texels[(y * width + x) * 4 + 3] = 255 if inside else (x * 7 + y * 3) % (faint + 1)
    return bytes(texels)


def reference_mips(width, height, rgba):
    """Each mip by hand, texel by texel: alpha-weighted colors, rounded means."""
    levels = [(width, height, rgba)]
    while width > 1 or height > 1:
        w, h = max(width // 2, 1), max(height // 2, 1)
        out = bytearray()
        for y in range(h):
            for x in range(w):
                cover = {(min(2 * x + dx, width - 1), min(2 * y + dy, height - 1)) for dx in (0, 1) for dy in (0, 1)}
                cover = [(cx if width > 1 else 0, cy if height > 1 else 0) for cx, cy in cover]
                cover = sorted(set(cover))
                texels = [rgba[(cy * width + cx) * 4:(cy * width + cx) * 4 + 4] for cx, cy in cover]
                weight = sum(t[3] for t in texels)
                for c in range(3):
                    if weight:
                        out.append((sum(t[c] * t[3] for t in texels) + weight // 2) // weight)
                    else:
                        out.append((sum(t[c] for t in texels) + len(texels) // 2) // len(texels))
                out.append((weight + len(texels) // 2) // len(texels))
        width, height, rgba = w, h, bytes(out)
        levels.append((width, height, rgba))
    return levels


SHAPES = [(16, 8, (3, 2, 9, 5)), (4, 4, None), (1, 1, None), (8, 1, (2, 0, 5, 0)), (1, 4, (0, 1, 0, 2)),
          (32, 64, (5, 9, 20, 50))]
FIXTURES = {f'{w}x{h}': (w, h, picture(w, h, 7 * w + h, box)) for w, h, box in SHAPES}
ACCEPTED = {name: png(w, h, rgba, (0, 1, 2, 3, 4)) for name, (w, h, rgba) in FIXTURES.items()}
ACCEPTED['one IDAT, Paeth only'] = png(16, 8, FIXTURES['16x8'][2], (4,),
                                       damage=lambda c: [c[0], [b'IDAT', c[1][1] + c[2][1]], c[3]])
ACCEPTED['an ancillary chunk'] = png(16, 8, FIXTURES['16x8'][2], damage=lambda c: [c[0], [b'tEXt', b'k\0v']] + c[1:])
A = ACCEPTED['16x8']
REJECTED = {
    'empty': b'',
    'not a PNG': b'GIF89a' + A[6:],
    'a damaged signature': A[:6] + b'\x1b' + A[7:],
    # The last byte of the header's CRC.
    'a damaged chunk': A[:32] + bytes([A[32] ^ 1]) + A[33:],
    'truncated': A[:-20],
    'no header': png(16, 8, FIXTURES['16x8'][2], damage=lambda c: c[1:]),
    'no end': png(16, 8, FIXTURES['16x8'][2], damage=lambda c: c[:3]),
    'no image data': png(16, 8, FIXTURES['16x8'][2], damage=lambda c: [c[0], c[3]]),
    'a short header': png(16, 8, FIXTURES['16x8'][2], damage=lambda c: [[b'IHDR', c[0][1][:12]]] + c[1:]),
    '16-bit': png(16, 8, FIXTURES['16x8'][2], depth=16),
    'RGB': png(16, 8, FIXTURES['16x8'][2], color=2),
    'gray and alpha': png(16, 8, FIXTURES['16x8'][2], color=4),
    'interlaced': png(16, 8, FIXTURES['16x8'][2], interlace=1),
    '12 wide': png(12, 8, picture(12, 8, 3, (1, 1, 5, 5))),
    '6 high': png(16, 6, picture(16, 6, 3, (1, 1, 5, 4))),
    '2048 wide': png(2048, 1, picture(2048, 1, 3, (9, 0, 90, 0))),
    'no texels': png(0, 8, b''),
    'nothing visible': png(8, 8, picture(8, 8, 5, (9, 9, 9, 9), faint=16)),
    'damaged image data': png(16, 8, FIXTURES['16x8'][2],
                              damage=lambda c: [c[0], [b'IDAT', b'\x78\x9c\xff\xff'], c[3]]),
    'short image data': png(16, 8, FIXTURES['16x8'][2],
                            damage=lambda c: [c[0], [b'IDAT', zlib.compress(zlib.decompress(c[1][1] + c[2][1])[:-1])],
                                              c[3]]),
    'an unknown filter': png(16, 8, FIXTURES['16x8'][2], (0, 5)),
}
# A picture that only just shows: one texel at alpha 17.
EDGE = picture(8, 8, 9, (9, 9, 9, 9), faint=16)
EDGE = EDGE[:4 * 27 + 3] + b'\x11' + EDGE[4 * 27 + 4:]
ACCEPTED['one texel at 17'] = png(8, 8, EDGE)


def write_folder(folder, files):
    folder.mkdir(parents=True, exist_ok=True)
    for name, data in files.items():
        (folder / name).write_bytes(data)
    return folder


WEAPONS = ['thompson_mp', 'TT30_mp', 'frag_grenade_american_mp', 'smoke_grenade_german_mp']
ATTACHMENTS = ['reflex', 'grip', 'suppressor']


def checks(m, folder):
    for name, data in ACCEPTED.items():
        if name in FIXTURES:
            assert m.read_png(data, 'x.png') == FIXTURES[name], name
        else:
            assert m.read_png(data, 'x.png')[2], name
    for name, data in REJECTED.items():
        raises(lambda: m.read_png(data, 'x.png'), 'owner_class_pictures/x.png')

    # Each mip from 1 x 1 up, as BGRA, after the header D3D and the engine read.
    for name, (w, h, rgba) in FIXTURES.items():
        iwi = m.package_picture(ACCEPTED[name], name)
        levels = reference_mips(w, h, rgba)
        body = b''.join(bytes(b for i in range(0, len(t), 4) for b in (t[i + 2], t[i + 1], t[i], t[i + 3]))
                        for _, _, t in reversed(levels))
        largest = [lw * lh * 4 for lw, lh, _ in levels]
        total = 28 + sum(largest)
        assert struct.unpack_from('<3sBBBHHH4i', iwi) == (b'IWi', 5, 1, 1, w, h, 1, total, total - sum(largest[:1]),
                                                          total - sum(largest[:2]), total - sum(largest[:3])), name
        assert iwi[28:] == body and len(iwi) == total, name
    raises(lambda: m.package_picture(REJECTED['RGB'], 'x.png'), 'RGBA')

    # The folder: missing, then pictures named after class weapons in any case.
    assert m.load_owner_pictures(folder / 'none', WEAPONS) is None
    assert m.load_owner_pictures(write_folder(folder / 'empty', {'.DS_Store': b'\0'}), WEAPONS) == {}
    good = write_folder(folder / 'good', {'TT30_MP.png': ACCEPTED['16x8'], 'frag_grenade_american_mp.PNG': ACCEPTED['4x4'],
                                          '.DS_Store': b'junk'})
    assert m.load_owner_pictures(good, WEAPONS) == {
        'images/browser_class/tt30_mp.iwi': m.package_picture(ACCEPTED['16x8'], 'x'),
        'images/browser_class/frag_grenade_american_mp.iwi': m.package_picture(ACCEPTED['4x4'], 'x')}
    for case, files, message in (
            ('unknown', {'mine_mp.png': A}, 'mine_mp.png: name it'),
            ('suffix', {'tt30_mp.jpg': A}, 'tt30_mp.jpg: name it'),
            ('stem', {'tt30_mp_2.png': A}, 'tt30_mp_2.png: name it'),
            ('bad', {'tt30_mp.png': REJECTED['interlaced']}, 'tt30_mp.png: use 8-bit RGBA')):
        raises(lambda: m.load_owner_pictures(write_folder(folder / case, files), WEAPONS), message)
    (folder / 'nested' / 'tt30_mp.png').mkdir(parents=True)
    raises(lambda: m.load_owner_pictures(folder / 'nested', WEAPONS), 'tt30_mp.png: name it')
    twice = write_folder(folder / 'twice', {'tt30_mp.png': A, 'TT30_mp.png': A})
    if len(list(twice.iterdir())) == 2:
        raises(lambda: m.load_owner_pictures(twice, WEAPONS), 'TT30_mp has two pictures')

    # A gun's attachment layers and the attachments' icons, for the attachments named.
    layered = write_folder(folder / 'layered', {'TT30_mp__Suppressor.png': ACCEPTED['16x8'],
                                                'thompson_mp__extmag.png': ACCEPTED['4x4'],
                                                'attachment__reflex.png': ACCEPTED['1x1']})
    assert m.load_owner_pictures(layered, WEAPONS, ATTACHMENTS) == {
        'images/browser_class/tt30_mp__suppressor.iwi': m.package_picture(ACCEPTED['16x8'], 'x'),
        'images/browser_class/thompson_mp__extmag.iwi': m.package_picture(ACCEPTED['4x4'], 'x'),
        'images/browser_class/attachment__reflex.iwi': m.package_picture(ACCEPTED['1x1'], 'x')}
    raises(lambda: m.load_owner_pictures(layered, WEAPONS), 'attachment__reflex.png: name it')
    for case, name in (('grip layer', 'tt30_mp__grip.png'), ('other gun', 'mine_mp__suppressor.png'),
                       ('other icon', 'attachment__laser.png'), ('bare layer', 'tt30_mp__.png'),
                       ('layer of a layer', 'tt30_mp__extmag__lens.png')):
        raises(lambda: m.load_owner_pictures(write_folder(folder / case, {name: A}), WEAPONS, ATTACHMENTS),
               name + ': name it')

    # The checker accepts the pictures the generator makes, and only them.
    owner = m.load_owner_pictures(good, WEAPONS)
    package = dict(owner, **{'images/hud_us_grenade.iwi': b'retail', 'materials/browser_class': b'x'})
    m.verify_owner_pictures(package, owner)
    m.verify_owner_pictures({'images/hud_us_grenade.iwi': b'retail'}, None)
    m.verify_owner_pictures({}, {})
    name = 'images/browser_class/tt30_mp.iwi'
    bad_packages = [(dict(package, **{name: owner[name][:-1] + b'\0'}), owner),
                    ({key: value for key, value in package.items() if key != name}, owner),
                    (dict(package, **{'images/browser_class/thompson_mp.iwi': owner[name]}), owner),
                    (package, None), (package, {}), ({}, owner)]
    for bad, expected in bad_packages:
        try:
            m.verify_owner_pictures(bad, expected)
        except AssertionError:
            continue
        raise AssertionError('accepted an altered package')
    COUNTS['packages'] = len(bad_packages)


with tempfile.TemporaryDirectory(prefix='cod2-class-pictures-') as directory:
    folder = Path(directory)
    module = load(module_source)
    # The generator refuses what the loader would.
    assert module.PICTURE_MAX_SIZE == int(re.search(r'#define R_HUD_PICTURE_MAX_SIZE (\d+)', hud2d_header)[1])
    assert module.PICTURE_ALPHA_MIN == int(re.search(r'#define HUD_SILHOUETTE_ALPHA_MIN (\d+)', hud2d)[1])
    checks(module, folder / 'live')
    mutants = [
        ("if data[:8] != b'\\x89PNG\\r\\n\\x1a\\n':", "if data[:4] != b'\\x89PNG':"),
        ("if struct.unpack_from('>I', data, position + 8 + length)[0] != zlib.crc32(kind + body):", 'if False:'),
        ('if len(body) != length or position + 12 + length > len(data):', 'if False:'),
        ("            header = struct.unpack('>IIBBBBB', body) if length == 13 else ()\n",
         "            header = struct.unpack('>IIBBBBB', body[:13]) if length >= 12 else ()\n"),
        ('    if not header or not ended or not compressed:', '    if not header:'),
        ('(depth, color, method, filtering, interlace) != (8, 6, 0, 0, 0)', '(depth, color) != (8, 6)'),
        ('(depth, color, method, filtering, interlace) != (8, 6, 0, 0, 0)', '(color, interlace) != (6, 0)'),
        (' or \\\n            width & (width - 1) or height & (height - 1):', ':'),
        ('not 1 <= width <= PICTURE_MAX_SIZE', 'not 1 <= width'),
        ("    if len(raw) != height * (stride + 1):\n        raise invalid('image data of the wrong size')\n", ''),
        ('                line[x] = (line[x] + line[x - 4]) & 255', '                line[x] = (line[x] + line[x - 3]) & 255'),
        ('line[x] = (line[x] + ((line[x - 4] if x >= 4 else 0) + previous[x]) // 2) & 255',
         'line[x] = (line[x] + (line[x - 4] + previous[x]) // 2) & 255'),
        ('(a if pa <= pb and pa <= pc else b if pb <= pc else c)', '(a if pa < pb and pa < pc else b if pb <= pc else c)'),
        ('a, b, c = (line[x - 4] if x >= 4 else 0), previous[x], (previous[x - 4] if x >= 4 else 0)',
         'a, b, c = (line[x - 4] if x >= 4 else 0), previous[x], 0'),
        ("        elif kind != 0:\n            raise invalid(f'unknown row filter {kind}')\n", ''),
        ('if max(pixels[3::4]) <= PICTURE_ALPHA_MIN:', 'if max(pixels[3::4]) < PICTURE_ALPHA_MIN:'),
        ('(sum(c * a for c, a in zip(texels, alpha)) + weight // 2) // weight if weight else',
         '(sum(texels) + count // 2) // count if weight else'),
        ('(sum(c * a for c, a in zip(texels, alpha)) + weight // 2) // weight', 'sum(c * a for c, a in zip(texels, alpha)) // weight'),
        ('        row[3::4] = bytes((weight + count // 2) // count for weight in weights)',
         '        row[3::4] = bytes(max(alpha) for alpha in alphas)'),
        ('sorted({2 * y, min(2 * y + 1, height - 1)})', 'sorted({2 * y})'),
        ('bgra[0::4], bgra[1::4], bgra[2::4], bgra[3::4] = texels[2::4], texels[1::4], texels[0::4], texels[3::4]',
         'bgra[0::4], bgra[1::4], bgra[2::4], bgra[3::4] = texels[0::4], texels[1::4], texels[2::4], texels[3::4]'),
        ('    for _, _, texels in reversed(levels):', '    for _, _, texels in levels:'),
        ('    sizes = [total - sum(largest[:level]) for level in range(4)]', '    sizes = [total] * 4'),
        ('IWI_NOPICMIP = 1', 'IWI_NOPICMIP = 3'),
        ("        if path.name.startswith('.'):\n            continue\n", ''),
        ('    names = {weapon.lower(): weapon for weapon in weapons}', '    names = {weapon: weapon for weapon in weapons}'),
        ("stem = path.name[:-4].lower() if path.name.lower().endswith('.png') else None",
         "stem = path.name[:-4].lower()"),
        ('        if not path.is_file() or stem not in names:', '        if stem not in names:'),
        ("    assert packaged == (owner or {}), 'Packaged class pictures differ from the owner\\'s'",
         "    assert set(packaged) == set(owner or {}), 'Packaged class pictures differ from the owner\\'s'"),
        ("    if not folder.is_dir():\n        return None\n", "    if not folder.is_dir():\n        return {}\n"),
        ("    names.update({f'{weapon.lower()}__{layer}': weapon for weapon in weapons for layer in ATTACHMENT_LAYERS})\n",
         ''),
        ("    names.update({f'attachment__{attachment}': attachment for attachment in attachments})\n", ''),
        ("ATTACHMENT_LAYERS = ('extmag', 'suppressor', 'reflex', 'lens', 'optic4x')",
         "ATTACHMENT_LAYERS = ('extmag', 'suppressor', 'reflex', 'lens', 'optic4x', 'grip')"),
        ("f'{weapon.lower()}__{layer}'", "f'{weapon}__{layer}'"),
    ]
    if len(list((folder / 'live' / 'twice').iterdir())) == 2:
        mutants.append(("        if PICTURE_FOLDER + stem + '.iwi' in pictures:\n", '        if False:\n'))
    for number, (old, new) in enumerate(mutants):
        assert module_source.count(old) == 1, old
        try:
            checks(load(module_source.replace(old, new)), folder / f'mutant{number}')
        except (AssertionError, ValueError, IndexError, KeyError, TypeError, OSError, struct.error, zlib.error):
            continue
        raise AssertionError(f'mutant survived: {old!r} -> {new!r}')
    COUNTS['python'] = len(mutants)

    # The engine's loader, fed the generator's files.
    files = {name: module.package_picture(ACCEPTED[name], name) for name in FIXTURES}
    good = files['16x8']
    header = struct.Struct('<3sBBBHHH4i')

    def headed(**fields):
        values = dict(zip(('tag', 'version', 'format', 'flags', 'width', 'height', 'depth'),
                          header.unpack_from(good)[:7]))
        values.update(fields)
        return header.pack(*values.values(), *header.unpack_from(good)[7:]) + good[28:]
    files.update({
        'tag': headed(tag=b'IWj'), 'version': headed(version=6), 'dxt5': headed(format=13), 'flags0': headed(flags=0),
        'flags3': headed(flags=3), 'depth': headed(depth=2), 'short': good[:-1], 'long': good + b'\0',
        'headless': good[:27],
        # Sides of 6 with the 31 texels of their mips halved down, so only the powers of two refuse them.
        'wide': header.pack(b'IWi', 5, 1, 1, 6, 4, 1, 0, 0, 0, 0) + b'\xff' * (4 * 31),
        'high': header.pack(b'IWi', 5, 1, 1, 4, 6, 1, 0, 0, 0, 0) + b'\xff' * (4 * 31),
        # Bigger sides the generator refuses, with every mip so only the size check refuses them.
        'huge': header.pack(b'IWi', 5, 1, 1, 2048, 1, 1, 0, 0, 0, 0) + bytes(4 * (2048 * 2 - 1)),
        'limit': header.pack(b'IWi', 5, 1, 1, 1024, 1, 1, 0, 0, 0, 0) + bytes(4 * (1024 * 2 - 2)) + b'\xff' * 4,
        # Every texel at alpha 16: nothing to draw.
        'faint': header.pack(b'IWi', 5, 1, 1, 4, 4, 1, 0, 0, 0, 0) + b'\x10' * (4 * 21),
    })
    for name, data in files.items():
        (folder / f'{name}.iwi').write_bytes(data)

support = r'''
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char byte; typedef float vec_t; typedef vec_t vec3_t[3];
typedef struct GfxImageFileHeader { char tag[3]; byte version, format, flags; short dimensions[3]; int fileSizeForPicmip[4]; } GfxImageFileHeader;
typedef int D3DFORMAT; typedef unsigned long DWORD;
#define D3DFMT_A8R8G8B8 21
typedef struct { unsigned short id, byteCount; } GfxCmdHeader;
typedef union { unsigned int packed; byte array[4]; } GfxColor;
typedef struct GfxImage { int mapType; struct { void *map; } texture; int width, height; const char *name; } GfxImage;
typedef struct { const char *name; byte samplerState, semantic, unused_0, unused_1; union { GfxImage *image; } u; } MaterialTextureDef;
typedef struct Material { struct { const char *name; byte gameFlags; } info; int stateBits[2];
    unsigned short textureCount, constantCount; void *techniqueSet; MaterialTextureDef *textures; void *constants; } Material;
typedef Material *MaterialHandle;
typedef struct { int surfaceType; vec3_t bounds[2]; } srfTriangles_t;
typedef struct { const Material *material; unsigned short lightmapIndex, sortGroup; srfTriangles_t *tris; } GfxSurface;
typedef struct { int surfaceCount; GfxSurface *surfaces; } GfxWorld;
static struct { Material *defaultMaterial, *rawMaterial; GfxWorld *world; } rgp;
''' + hud2d_header + r'''
static int textureIds, setups, released, unbound, failSetup, reads, frees, uploads;
static int uploadLevels[16]; static long uploadOffsets[16];
static const byte *currentFile;
static void Image_Generate2D(GfxImage *image, byte *pixels, int width, int height, int format) {
    (void)image; (void)pixels; (void)width; (void)height; (void)format; assert(0);
}
static void Image_Setup(GfxImage *image, int width, int height, int depth, int flags, DWORD usage, D3DFORMAT format) {
    assert(image->name && image->name[0] == '*' && !image->texture.map && depth == 1 && usage == 0);
    /* No picmip, and every mip D3D makes for the size. */
    assert(flags == 1 && format == D3DFMT_A8R8G8B8);
    setups++; uploads = 0; image->width = width; image->height = height;
    if (!failSetup) image->texture.map = (void *)(intptr_t)++textureIds;
}
static void Image_UploadData(GfxImage *image, D3DFORMAT format, int face, int level, const byte *src) {
    static byte copy[1024 * 1024 * 4];
    int w = image->width >> level > 0 ? image->width >> level : 1, h = image->height >> level > 0 ? image->height >> level : 1;
    assert(image->texture.map && format == D3DFMT_A8R8G8B8 && face == 0 && uploads < 16);
    memcpy(copy, src, (size_t)w * h * 4);
    uploadLevels[uploads] = level; uploadOffsets[uploads++] = (long)(src - currentFile);
}
static void Image_Release(GfxImage *image) { assert(image->texture.map); released++; image->texture.map = NULL; }
static void RB_UnbindImage(const GfxImage *image) { (void)image; unbound++; }
static void ClearBounds(vec3_t mins, vec3_t maxs) { (void)mins; (void)maxs; }
static void ExpandBounds(vec3_t a, vec3_t b, vec3_t c, vec3_t d) { (void)a; (void)b; (void)c; (void)d; }
/* Files are copied to the heap so reads outside them trip the sanitizer. */
static char *fileNames[32]; static byte *fileData[32]; static int fileLens[32], fileCount;
static int FS_ReadFile(const char *path, void **buffer) {
    reads++;
    for (int i = 0; i < fileCount; i++) {
        if (!strcmp(path, fileNames[i])) {
            *buffer = malloc(fileLens[i]); memcpy(*buffer, fileData[i], fileLens[i]);
            currentFile = *buffer;
            return fileLens[i];
        }
    }
    *buffer = NULL;
    return -1;
}
static void FS_FreeFile(void *buffer) { assert(buffer); frees++; free(buffer); }
static int Com_sprintf(char *dest, int size, const char *fmt, ...) {
    va_list args; va_start(args, fmt); int n = vsnprintf(dest, size, fmt, args); va_end(args);
    return n < size ? n : -1;
}
'''

checks_c = r'''
static void add(const char *dir, const char *name, const char *path) {
    char source[512]; FILE *f;
    snprintf(source, sizeof(source), "%s/%s.iwi", dir, name);
    assert((f = fopen(source, "rb")));
    fseek(f, 0, SEEK_END); fileLens[fileCount] = (int)ftell(f); rewind(f);
    fileData[fileCount] = malloc(fileLens[fileCount]);
    assert(fread(fileData[fileCount], 1, fileLens[fileCount], f) == (size_t)fileLens[fileCount]);
    fclose(f);
    fileNames[fileCount++] = strdup(path);
}
static int sized(const char *name, int *w, int *h) {
    char path[96];
    snprintf(path, sizeof(path), "images/browser_class/%s.iwi", name);
    for (int i = 0; i < fileCount; i++)
        if (!strcmp(fileNames[i], path))
            return R_GetPictureSize(fileData[i], fileLens[i], w, h);
    assert(0);
    return 0;
}
static int same(float a, float b) { return a > b - 1e-6f && a < b + 1e-6f; }

int main(int argc, char **argv) {
    static const char *const cases[] = { "16x8", "4x4", "1x1", "8x1", "1x4", "32x64", "tag", "version", "dxt5",
        "flags0", "flags3", "depth", "wide", "high", "short", "long", "headless", "huge", "limit", "faint" };
    char path[96];
    int w = -1, h = -1;
    assert(argc == 2);
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        snprintf(path, sizeof(path), "images/browser_class/%s.iwi", cases[i]);
        add(argv[1], cases[i], path);
    }
    /* The generator's files are what the loader takes; nothing else is. */
    assert(sized("16x8", &w, &h) && w == 16 && h == 8);
    assert(sized("32x64", &w, &h) && w == 32 && h == 64 && sized("1x1", &w, &h) && w == 1 && h == 1);
    assert(sized("8x1", &w, &h) && w == 8 && h == 1 && sized("1x4", &w, &h) && w == 1 && h == 4);
    assert(sized("limit", &w, &h) && w == 1024 && h == 1 && sized("faint", &w, &h) && w == 4);
    w = h = -1;
    static const char *const refused[] = { "tag", "version", "dxt5", "flags0", "flags3", "depth", "wide", "high",
                                           "short", "long", "headless", "huge" };
    for (unsigned i = 0; i < sizeof(refused) / sizeof(refused[0]); i++) assert(!sized(refused[i], &w, &h));
    assert(w == -1 && h == -1 && !R_GetPictureSize(NULL, 100, &w, &h));

    static MaterialTextureDef rawTexdef = {"colorMap", 0x11, 1, 0, 0, {NULL}};
    static Material raw = {{"$raw", 0}, {0x28129165, 7}, 1, 0, (void *)0x1234, &rawTexdef, NULL};
    static Material fallback = {{"$default", 1}};
    float st[4] = {-1, -1, -1, -1}, aspect = -1.0f;
    rgp.rawMaterial = &raw; rgp.defaultMaterial = &fallback;

    /* Opaque texels 3..9 by 2..5 of 16 x 8, faint ones around: one texel more each way. */
    Material *a = R_GetHudPictureMaterial("browser_class/16x8", st, &aspect);
    assert(a && reads == 1 && frees == 1 && setups == 1 && aspect == 1.5f);
    assert(same(st[0], 2 / 16.0f) && same(st[1], 1 / 8.0f) && same(st[2], 11 / 16.0f) && same(st[3], 7 / 8.0f));
    /* Smallest mip first: 1x1, 2x1, 4x2, 8x4, 16x8 after the 28-byte header. */
    assert(uploads == 5);
    static const int levels[5] = {4, 3, 2, 1, 0};
    static const long offsets[5] = {28, 32, 40, 72, 200};
    for (int i = 0; i < 5; i++) assert(uploadLevels[i] == levels[i] && uploadOffsets[i] == offsets[i]);
    /* $raw's technique and state, its own texture, filtered across the mips and clamped. */
    assert(a->techniqueSet == raw.techniqueSet && a->stateBits[0] == raw.stateBits[0] && a->stateBits[1] == raw.stateBits[1]);
    assert(a->textureCount == 1 && a->textures != raw.textures && a->textures[0].samplerState == 0x3a);
    assert(!strcmp(a->textures[0].name, "colorMap") && rawTexdef.samplerState == 0x11 && rawTexdef.u.image == NULL);
    assert(!strcmp(a->textures[0].u.image->name, "*browser_class/16x8") && !strcmp(a->info.name, "browser_class/16x8"));
    /* Read once. */
    st[0] = -1; aspect = -1;
    assert(R_GetHudPictureMaterial("browser_class/16x8", st, &aspect) == a && reads == 1 && aspect == 1.5f && same(st[0], 0.125f));

    Material *tall = R_GetHudPictureMaterial("browser_class/32x64", st, &aspect);
    assert(tall && tall != a && uploads == 7 && uploadLevels[0] == 6 && uploadLevels[6] == 0);
    assert(uploadOffsets[6] == 28 + 4L * (1 + 2 + 8 + 32 + 128 + 512) && same(aspect, 18.0f / 44.0f));
    assert(same(st[0], 4 / 32.0f) && same(st[1], 8 / 64.0f) && same(st[2], 22 / 32.0f) && same(st[3], 52 / 64.0f));
    /* A picture opaque to its edges keeps them; a one-texel or one-row picture works. */
    assert(R_GetHudPictureMaterial("browser_class/4x4", st, &aspect) && aspect == 1.0f && st[0] == 0 && st[3] == 1);
    assert(R_GetHudPictureMaterial("browser_class/1x1", st, &aspect) && aspect == 1.0f && uploads == 1);
    assert(R_GetHudPictureMaterial("browser_class/8x1", st, &aspect) && same(aspect, 6.0f) && uploads == 4);
    assert(R_GetHudPictureMaterial("browser_class/1x4", st, &aspect) && same(aspect, 0.25f) && same(st[1], 0.0f));

    /* Failures are remembered: a missing name, a bad file, nothing visible, no texture. */
    int before = reads;
    assert(!R_GetHudPictureMaterial("browser_class/none", st, &aspect) && aspect == 0.0f && reads == before + 1);
    assert(!R_GetHudPictureMaterial("browser_class/none", st, &aspect) && reads == before + 1);
    assert(!R_GetHudPictureMaterial("browser_class/short", st, &aspect) && frees == reads - 1);
    assert(!R_GetHudPictureMaterial("browser_class/faint", st, &aspect) && setups == 6);
    failSetup = 1;
    assert(!R_GetHudPictureMaterial("browser_class/limit", st, &aspect) && setups == 7 && aspect == 0.0f);
    failSetup = 0;
    assert(!R_GetHudPictureMaterial("browser_class/limit", st, &aspect) && setups == 7 && reads == before + 4);
    assert(frees == reads - 1 && !R_GetHudPictureMaterial(NULL, st, &aspect));
    /* Names past the cache's room, and without $raw, are refused unread. */
    static char longName[80];
    memset(longName, 'x', 71);
    assert(!R_GetHudPictureMaterial(longName, st, &aspect) && reads == before + 4);
    longName[70] = '\0';
    assert(!R_GetHudPictureMaterial(longName, st, &aspect) && reads == before + 5);
    rgp.rawMaterial = NULL;
    assert(!R_GetHudPictureMaterial("browser_class/wide", st, &aspect) && reads == before + 5);
    rgp.rawMaterial = &raw;

    /* The cache holds R_HUD_PICTURE_COUNT names, then refuses new ones; eleven are in it. */
    static char names[R_HUD_PICTURE_COUNT][24];
    int held = 11;
    for (int i = held; i < R_HUD_PICTURE_COUNT; i++) {
        snprintf(names[i], sizeof(names[i]), "browser_class/n%02d", i);
        assert(!R_GetHudPictureMaterial(names[i], st, &aspect));
    }
    int full = reads;
    assert(!R_GetHudPictureMaterial("browser_class/past", st, &aspect) && reads == full);
    assert(R_GetHudPictureMaterial("browser_class/16x8", st, &aspect) == a && reads == full);

    /* Shutdown releases each live picture once and empties the cache. */
    R_ShutdownHudImages();
    assert(released == 6 && unbound == 6);
    Material *again = R_GetHudPictureMaterial("browser_class/16x8", st, &aspect);
    assert(again && reads == full + 1 && setups == 8 && again->textures[0].u.image->texture.map);
    R_ShutdownHudImages();
    assert(released == 7);
    for (int i = 0; i < fileCount; i++) { free(fileData[i]); free(fileNames[i]); }
    return 0;
}
'''

picture_code = hud2d[hud2d.index('/* Linear filtering'):]


def build_and_run(body, directory, name):
    source = Path(directory) / f'{name}.c'
    binary = Path(directory) / name
    source.write_text(support + body + checks_c)
    subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    '-Wno-unused-function', str(source), '-lm', '-o', str(binary)], check=True)
    return subprocess.run([str(binary), directory], capture_output=True, text=True)


def mutate(source, old, new):
    assert source.count(old) == 1, old
    return source.replace(old, new)


loader_mutants = [
    ('(w & (w - 1)) || (h & (h - 1))', '(w & (w - 1))'),
    (' || header->flags != IWI_FLAG_NOPICMIP ||', ' ||'),
    ('header->version != IWI_VERSION || header->format != IWI_FORMAT_ARGB32', 'header->version != IWI_VERSION'),
    ('w > R_HUD_PICTURE_MAX_SIZE || h > R_HUD_PICTURE_MAX_SIZE', 'w > 2 * R_HUD_PICTURE_MAX_SIZE || h > R_HUD_PICTURE_MAX_SIZE'),
    ('    if (fileLen != size)\n        return 0;\n', '    if (fileLen < size)\n        return 0;\n'),
    ('    while (R_MipSize(width, levels - 1) > 1 || R_MipSize(height, levels - 1) > 1)',
     '    while (R_MipSize(width, levels - 1) > 1 && R_MipSize(height, levels - 1) > 1)'),
    ('R_AlphaBounds((const byte *)file + fileLen - width * height * 4, width, height, bounds)',
     'R_AlphaBounds((const byte *)file + sizeof(GfxImageFileHeader), width, height, bounds)'),
    ('                mip += R_MipSize(width, level) * R_MipSize(height, level) * 4;\n', ''),
    ('            entry->st[2] = (float)(bounds[2] + 1) / (float)width;', '            entry->st[2] = (float)bounds[2] / (float)width;'),
    ('(float)(bounds[2] - bounds[0] + 1) / (float)(bounds[3] - bounds[1] + 1)', '(float)width / (float)height'),
    ('            entry->texture.samplerState = HUD_PICTURE_SAMPLER_STATE;\n', ''),
    ('#define HUD_PICTURE_SAMPLER_STATE 0x3a', '#define HUD_PICTURE_SAMPLER_STATE 0x32'),
    ('        if (s_pictureCount == R_HUD_PICTURE_COUNT)\n            return NULL;\n', ''),
    ('        if (!strcmp(s_pictures[i].name + 1, name))', '        if (!strcmp(s_pictures[i].name + 1, name) && s_pictures[i].material)'),
    ('    if (!name || !raw || strlen(name) + 1 >= sizeof(entry->name))', '    if (!name || !raw)'),
    ('        R_ReleaseImage(&s_pictures[slot].image);\n', ''),
    ('    s_pictureCount = 0;\n    s_hudImageGeneration++;', '    s_hudImageGeneration++;'),
]
with tempfile.TemporaryDirectory(prefix='cod2-class-pictures-c-') as directory:
    for name, data in files.items():
        (Path(directory) / f'{name}.iwi').write_bytes(data)
    result = build_and_run(picture_code, directory, 'loader')
    assert result.returncode == 0, result.stderr[-3000:]
    for number, (old, new) in enumerate(loader_mutants):
        result = build_and_run(mutate(picture_code, old, new), directory, f'loader{number}')
        assert result.returncode != 0, f'loader mutant survived: {old!r} -> {new!r}'
    COUNTS['loader'] = len(loader_mutants)


def function(source, name):
    match = re.search(r'^((?:static )?[^\n;]*\b' + name + r'\([^;]*?\))\s*\{', source, re.M)
    assert match, name
    brace = source.index('{', match.end() - 1)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


# The class menus' pictures: an owner's in its colors, else the retail image or the silhouette, tinted.
menu_code = ''.join(function(menu, name) for name in (
    'Cac_ImagesFresh', 'Cac_Image', 'Cac_Picture', 'Cac_ClassPicture', 'Cac_Layer', 'Cac_AttachmentIcon',
    'Cac_DrawIcon', 'Cac_LayeredPicture', 'Cac_GunPicture'))
layers = re.search(r'^static const struct \{\n    int attachment;\n    const char \*suffix;\n\} s_cacLayers\[\] = '
                   r'\{.*?^#define CAC_LAYER_COUNT [^\n]+\n', menu, re.M | re.S)[0]
menu_support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "headers/PC/bgame/bg_weapon_camo.h"
typedef float vec_t; typedef vec_t vec4_t[4];
typedef struct Material { int id; } Material;
typedef Material *MaterialHandle;
''' + class_header + rules + layers + r'''
#define CAC_ALIGN_TOP 1
#define CAC_RETAIL_IMAGE_ASPECT (199.0f / 112.0f)
enum { WWII_COLOR_WHITE = 4 };
typedef struct { float x, y, w, h; int horz; } CacRect;
static struct {
    int imageGeneration; MaterialHandle images[64]; float aspects[64]; float st[64][4]; int pictures[64];
    MaterialHandle layers[64][CAC_LAYER_COUNT]; float layerSt[64][CAC_LAYER_COUNT][4];
    unsigned char layerRead[64][CAC_LAYER_COUNT];
    MaterialHandle icons[ATT_COUNT]; float iconSt[ATT_COUNT][4], iconAspects[ATT_COUNT]; unsigned char iconRead[ATT_COUNT];
} s_cac;
const ClassWeapon g_classWeapons[] = {
    { "thompson_mp", CLASS_SLOT_PRIMARY, 0, 0, NULL, NULL, "weapon_thompson" },
    { "TT30_mp", CLASS_SLOT_SECONDARY, 0, 0, NULL, NULL, "gfx/icons/hud@tt30.tga" },
    { "frag_grenade_american_mp", CLASS_SLOT_LETHAL, 0, 0, NULL, NULL, "gfx/icons/hud@us_grenade.tga" },
    { "smoke_grenade_german_mp", CLASS_SLOT_TACTICAL, 0, 0, NULL, NULL, "hud_us_smokegrenade" },
    { "PPS42_mp", CLASS_SLOT_PRIMARY, 0, 0, NULL, NULL, "weapon_pps42" },
};
const int g_classWeaponCount = 5;
static Material picture = {1}, tt30 = {2}, retail = {3}, icon = {4}, silhouette = {5};
static Material gun = {6}, can = {7}, longer = {8}, dot = {9}, reflexIcon = {10}, paintedGun = {11};
static const vec4_t white = {1, 1, 1, 1}, tint = {0.7f, 0.7f, 0.68f, 1};
static int generation, pictureCalls, registerCalls;
static char asked[64][64];
static struct { float x, y, w, h, st[4]; const vec_t *color; MaterialHandle material; int draws; } drawn;
static struct { float x, y, w, h, st[4]; const vec_t *color; MaterialHandle material; } logged[8];
static int logCount, camoCalls, camoAsked;
static MaterialHandle camoInput;
static int R_GetHudImageGeneration(void) { return generation; }
static MaterialHandle R_GetHudPictureMaterial(const char *name, float *st, float *aspect) {
    assert(pictureCalls < 64);
    snprintf(asked[pictureCalls++], 64, "%s", name);
    *aspect = 0.0f;
    if (!strcmp(name, "browser_class/frag_grenade_american_mp")) {
        st[0] = 0.25f; st[1] = 0.125f; st[2] = 0.75f; st[3] = 0.875f; *aspect = 0.5f; return &picture;
    }
    if (!strcmp(name, "browser_class/tt30_mp")) { st[0] = st[1] = 0; st[2] = st[3] = 0.5f; *aspect = 2.0f; return &tt30; }
    /* A gun on a 4:1 canvas, its can reaching past the muzzle, its longer magazine lower, a sight above. */
    static const struct { const char *name; float st[4], aspect; Material *material; } layered[] = {
        { "browser_class/pps42_mp", { 0.1f, 0.4f, 0.9f, 0.7f }, 32.0f / 3.0f, &gun },
        { "browser_class/pps42_mp__suppressor", { 0.05f, 0.45f, 0.3f, 0.55f }, 10.0f, &can },
        { "browser_class/pps42_mp__extmag", { 0.1f, 0.4f, 0.9f, 0.85f }, 7.1f, &longer },
        { "browser_class/pps42_mp__reflex", { 0.5f, 0.3f, 0.6f, 0.45f }, 2.7f, &dot },
        { "browser_class/attachment__reflex", { 0.0f, 0.25f, 1.0f, 0.75f }, 2.0f, &reflexIcon },
    };
    for (unsigned i = 0; i < sizeof(layered) / sizeof(layered[0]); i++)
        if (!strcmp(name, layered[i].name)) {
            memcpy(st, layered[i].st, sizeof(layered[i].st));
            *aspect = layered[i].aspect;
            return layered[i].material;
        }
    return NULL;
}
static const WeaponCamoGun ppsCamo = { "PPS42_mp", "mtl_weapon_pps42", WEAPON_CAMO_BODY };
const WeaponCamoGun *BG_WeaponCamo_Gun(const char *weapon) { return !strcmp(weapon, "PPS42_mp") ? &ppsCamo : NULL; }
static MaterialHandle R_GetWeaponCamoPicture(MaterialHandle image, int silhouette, int camo, int style, float *aspect) {
    assert(!silhouette && style == WEAPON_CAMO_BODY);
    camoCalls++; camoInput = image; camoAsked = camo; *aspect = 1.0f;
    return &paintedGun;
}
static MaterialHandle CL_RegisterMaterialNoMip(const char *name, int track) {
    assert(track == 3); registerCalls++;
    return !strcmp(name, "weapon_thompson") ? &retail : &icon;
}
static MaterialHandle R_GetHudSilhouetteMaterial(MaterialHandle image, float *aspect) {
    assert(image == &icon); *aspect = 3.0f; return &silhouette;
}
static const vec_t *UI_WWIIMenu_Color(int color) { assert(color == WWII_COLOR_WHITE); return white; }
static void CL_DrawStretchPic(float x, float y, float w, float h, int horz, int vert, float s1, float t1, float s2,
                              float t2, const vec_t *color, MaterialHandle material) {
    assert(horz == 2 && vert == CAC_ALIGN_TOP);
    drawn.x = x; drawn.y = y; drawn.w = w; drawn.h = h; drawn.st[0] = s1; drawn.st[1] = t1; drawn.st[2] = s2;
    drawn.st[3] = t2; drawn.color = color; drawn.material = material; drawn.draws++;
    assert(logCount < 8);
    logged[logCount].x = x; logged[logCount].y = y; logged[logCount].w = w; logged[logCount].h = h;
    logged[logCount].st[0] = s1; logged[logCount].st[1] = t1; logged[logCount].st[2] = s2; logged[logCount].st[3] = t2;
    logged[logCount].color = color; logged[logCount].material = material; logCount++;
}
'''
menu_checks = r'''
static int near(float a, float b) { return fabsf(a - b) < 1e-3f; }
static int askedFor(const char *name) {
    int count = 0;
    for (int i = 0; i < pictureCalls; i++)
        count += !strcmp(asked[i], name);
    return count;
}
static void gunPicture(int weapon, int camo, unsigned attachments) {
    CacRect box = { 10, 20, 200, 50, 2 };
    logCount = 0;
    camoCalls = 0;
    Cac_GunPicture(weapon, camo, attachments, &box, tint);
}
/* Every layer at one scale on the canvas, whose texels stay square on its 4:1, the whole framed in the box. */
static void sameCanvas(void) {
    float fx = logged[0].w / (logged[0].st[2] - logged[0].st[0]), fy = logged[0].h / (logged[0].st[3] - logged[0].st[1]);
    float left = 1e9f, top = 1e9f, right = -1e9f, bottom = -1e9f;
    for (int i = 0; i < logCount; i++) {
        assert(near(logged[i].w, (logged[i].st[2] - logged[i].st[0]) * fx));
        assert(near(logged[i].h, (logged[i].st[3] - logged[i].st[1]) * fy));
        assert(near(logged[i].x - logged[0].x, (logged[i].st[0] - logged[0].st[0]) * fx));
        assert(near(logged[i].y - logged[0].y, (logged[i].st[1] - logged[0].st[1]) * fy));
        assert(logged[i].color == white);
        left = fminf(left, logged[i].x);
        top = fminf(top, logged[i].y);
        right = fmaxf(right, logged[i].x + logged[i].w);
        bottom = fmaxf(bottom, logged[i].y + logged[i].h);
    }
    assert(near(fx / fy, 4.0f) && near(left - 10, 210 - right) && near(top - 20, 70 - bottom));
    assert(near(right - left, 200) || near(bottom - top, 50));
}
static void paint(int weapon) {
    CacRect box = {10, 20, 100, 50, 2};
    drawn.draws = 0;
    Cac_Picture(weapon, &box, tint);
}
int main(void) {
    /* The owner's picture, by the weapon's name in lower case, fit by its proportions, drawn in its own colors. */
    paint(2);
    assert(pictureCalls == 1 && !strcmp(asked[0], "browser_class/frag_grenade_american_mp") && registerCalls == 0);
    assert(drawn.draws == 1 && drawn.material == &picture && drawn.color == white);
    assert(drawn.w == 25 && drawn.h == 50 && drawn.x == 47.5f && drawn.y == 20);
    assert(drawn.st[0] == 0.25f && drawn.st[1] == 0.125f && drawn.st[2] == 0.75f && drawn.st[3] == 0.875f);
    paint(1);
    assert(!strcmp(asked[1], "browser_class/tt30_mp") && drawn.material == &tt30 && drawn.color == white);
    assert(drawn.w == 100 && drawn.h == 50 && drawn.st[2] == 0.5f && registerCalls == 0);
    /* Without one: the retail menu image, then a silhouette, whole and tinted as asked. */
    paint(0);
    assert(!strcmp(asked[2], "browser_class/thompson_mp") && registerCalls == 1 && drawn.material == &retail);
    assert(drawn.color == tint && drawn.st[0] == 0 && drawn.st[1] == 0 && drawn.st[2] == 1 && drawn.st[3] == 1);
    assert(drawn.w > 88.8f && drawn.w < 88.9f && drawn.h == 50);
    paint(3);
    assert(registerCalls == 2 && drawn.material == &silhouette && drawn.color == tint && drawn.w == 100);
    assert(drawn.h > 33.3f && drawn.h < 33.4f && drawn.st[2] == 1);
    /* Each weapon is looked up once, until the renderer's images are rebuilt. */
    paint(2); paint(0); paint(3);
    assert(pictureCalls == 4 && registerCalls == 2 && drawn.material == &silhouette);
    generation++;
    paint(2);
    assert(pictureCalls == 5 && drawn.material == &picture && drawn.color == white);
    paint(5); paint(-1);
    assert(drawn.draws == 0 && pictureCalls == 5);

    /* A gun with the owner's picture wears its attachments' layers on its canvas, the longer magazine in place of
     * its own, in layer order; another attachment adds nothing. */
    gunPicture(4, WEAPON_CAMO_NONE, 0);
    assert(logCount == 1 && logged[0].material == &gun && logged[0].color == white);
    gunPicture(4, WEAPON_CAMO_NONE, ATT_BIT(ATT_SUPPRESSOR));
    assert(logCount == 2 && logged[0].material == &gun && logged[1].material == &can);
    sameCanvas();
    assert(near(logged[1].x, 10.0f) && near(logged[0].w, 200.0f * 0.8f / 0.85f));
    gunPicture(4, WEAPON_CAMO_NONE, ATT_BIT(ATT_EXTMAG));
    assert(logCount == 1 && logged[0].material == &longer && logged[0].st[3] == 0.85f);
    sameCanvas();
    gunPicture(4, WEAPON_CAMO_NONE, ATT_BIT(ATT_GRIP) | ATT_BIT(ATT_REFLEX) | ATT_BIT(ATT_SUPPRESSOR) | ATT_BIT(ATT_EXTMAG));
    assert(logCount == 3 && logged[0].material == &longer && logged[1].material == &can && logged[2].material == &dot);
    sameCanvas();
    /* A layer the owner did not make leaves the plain picture. */
    gunPicture(4, WEAPON_CAMO_NONE, ATT_BIT(ATT_LENS));
    assert(logCount == 1 && logged[0].material == &gun && near(logged[0].w, 200.0f));
    /* Painted, the gun or its longer magazine takes the camouflage; what sits on it keeps its finish. */
    gunPicture(4, 3, ATT_BIT(ATT_EXTMAG) | ATT_BIT(ATT_SUPPRESSOR));
    assert(camoCalls == 1 && camoInput == &longer && camoAsked == 3);
    assert(logCount == 2 && logged[0].material == &paintedGun && logged[1].material == &can);
    /* Layers are read once an image generation. */
    assert(askedFor("browser_class/pps42_mp__suppressor") == 1 && askedFor("browser_class/pps42_mp__lens") == 1);
    generation++;
    gunPicture(4, WEAPON_CAMO_NONE, ATT_BIT(ATT_SUPPRESSOR));
    assert(logCount == 2 && askedFor("browser_class/pps42_mp__suppressor") == 2);
    /* Only an owner's picture of a primary or a pistol takes layers. */
    int before = pictureCalls;
    gunPicture(0, WEAPON_CAMO_NONE, ATT_BIT(ATT_SUPPRESSOR));
    assert(logCount == 1 && logged[0].material == &retail);
    gunPicture(2, WEAPON_CAMO_NONE, ATT_BIT(ATT_SUPPRESSOR));
    assert(logCount == 1 && logged[0].material == &picture);
    for (int i = before; i < pictureCalls; i++)
        assert(!strstr(asked[i], "__"));

    /* An attachment's icon, fit in its share of the box, in the color asked; none without the owner's. */
    CacRect cell = { 0, 0, 40, 40, 2 };
    logCount = 0;
    assert(Cac_DrawIcon(ATT_REFLEX, &cell, 0.5f, tint) && logCount == 1 && logged[0].material == &reflexIcon);
    assert(logged[0].color == tint && near(logged[0].w, 20) && near(logged[0].h, 10) && near(logged[0].x, 10));
    assert(near(logged[0].y, 15) && logged[0].st[1] == 0.25f && logged[0].st[3] == 0.75f);
    assert(!Cac_DrawIcon(ATT_GRIP, &cell, 0.5f, tint) && logCount == 1);
    Cac_DrawIcon(ATT_REFLEX, &cell, 0.5f, tint);
    Cac_DrawIcon(ATT_GRIP, &cell, 0.5f, tint);
    assert(askedFor("browser_class/attachment__reflex") == 1 && askedFor("browser_class/attachment__grip") == 1);
    generation++;
    Cac_DrawIcon(ATT_REFLEX, &cell, 0.5f, tint);
    assert(askedFor("browser_class/attachment__reflex") == 2);
    assert(!Cac_DrawIcon(ATT_COUNT, &cell, 0.5f, tint) && !Cac_DrawIcon(-1, &cell, 0.5f, tint));
    return 0;
}
'''
menu_mutants = [
    ('            name[i] = (char)(name[i] >= \'A\' && name[i] <= \'Z\' ? name[i] + 32 : name[i]);', '            ;'),
    ('"browser_class/%s", g_classWeapons[index].weapon', '"%s", g_classWeapons[index].weapon'),
    ('s_cac.pictures[index] ? UI_WWIIMenu_Color(WWII_COLOR_WHITE) : color', 'color'),
    ('st[0], st[1], st[2], st[3], s_cac.pictures[index]', '0.0f, 0.0f, 1.0f, 1.0f, s_cac.pictures[index]'),
    ('        s_cac.pictures[index] = image != NULL;\n', ''),
    ('        s_cac.st[index][2] = s_cac.st[index][3] = 1.0f;\n', ''),
    ('            s_cac.images[index] = image;\n            *aspect', '            *aspect'),
    ('        if (s_cacLayers[layer].attachment == ATT_EXTMAG) {', '        if (0) {'),
    ('        span[0] = sts[i][0] < span[0] ? sts[i][0] : span[0];\n', ''),
    ('    canvas = aspect * (st[3] - st[1]) / (st[2] - st[0]);', '    canvas = aspect;'),
    ('        if (picture && painted > 0.0f)\n            drawn[0] = picture;\n', ''),
    ('        s_cac.layerRead[index][layer] = 1;\n', ''),
    ('    if (image && aspect > 0.0f && attachments && s_cac.pictures[index] &&',
     '    if (image && aspect > 0.0f && attachments &&'),
    ('        g_classWeapons[index].slot <= CLASS_SLOT_SECONDARY &&\n        Cac_LayeredPicture', '        Cac_LayeredPicture'),
    ('"attachment__%s"', '"%s"'),
    ('"%s__%s", g_classWeapons[index].weapon', '"%s_%s", g_classWeapons[index].weapon'),
    ('        path[i] = (char)(path[i] >= \'A\' && path[i] <= \'Z\' ? path[i] + 32 : path[i]);', '        ;'),
    ('        memset(s_cac.layerRead, 0, sizeof(s_cac.layerRead));\n', ''),
    ('        memset(s_cac.iconRead, 0, sizeof(s_cac.iconRead));\n', ''),
    ('st[0], st[1], st[2], st[3], color, icon);', '0.0f, 0.0f, 1.0f, 1.0f, color, icon);'),
    ('                          sts[i][0], sts[i][1], sts[i][2], sts[i][3], UI_WWIIMenu_Color(WWII_COLOR_WHITE), drawn[i]);',
     '                          0.0f, 0.0f, 1.0f, 1.0f, UI_WWIIMenu_Color(WWII_COLOR_WHITE), drawn[i]);'),
]
with tempfile.TemporaryDirectory(prefix='cod2-class-pictures-menu-') as directory:
    for number, variant in enumerate([menu_code] + [mutate(menu_code, old, new) for old, new in menu_mutants]):
        source, binary = Path(directory) / f'menu{number}.c', Path(directory) / f'menu{number}'
        source.write_text(menu_support + variant + menu_checks)
        subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-I', str(root / 'src'), '-fsanitize=address,undefined',
                        '-fno-sanitize-recover=all', '-Wno-unused-function', str(source), '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (number == 0), (number, result.stderr[-2000:])
    COUNTS['menu'] = len(menu_mutants)

# The generator packages the folder with the class lists; the checker compares it and skips it among retail files.
generator = (root / 'scripts/prepare-browser-bootstrap.py').read_text()
checker = (root / 'scripts/check-private-assets.py').read_text()
assert 'load_owner_pictures(root / "data/main" / OWNER_PICTURES_DIR, classes["primary"] + classes["secondary"]' in generator
assert re.search(r'load_owner_pictures\([^)]*classes\["tactical"\], classes\["attachments"\]\)', generator)
assert re.search(r'load_owner_pictures\([^)]*classes\["tactical"\],\s*classes\["attachments"\]\)', checker)
assert 'renderer.update(pictures)' in generator and generator.index('renderer.update(pictures)') < generator.index(
    'archives["cod2_browser_renderer.iwd"] = renderer')
assert 'PICTURE_FOLDER)):\n                    continue' in checker and 'verify_owner_pictures(entries, owner_pictures)' in checker

print(f'PASS: {len(ACCEPTED)} accepted and {len(REJECTED)} rejected PNGs; the IWis hold every mip from 1 x 1, '
      f'alpha-weighted, as BGRA after the header the engine reads; the folder takes <weapon>.png, '
      f'<weapon>__<layer>.png for the five attachment layers and attachment__<attachment>.png in any case for class '
      f'weapons and attachments only; the checker rejects {COUNTS["packages"]} altered packages; under ASan/UBSan the '
      f'loader reads the generator\'s files, uploads each mip smallest first, crops to the visible texels plus one, '
      f'filters across mips through $raw, remembers failures, caps its cache and releases on shutdown; the class menus '
      f'ask for browser_class/<weapon>, draw a picture untinted and fall back to retail images and silhouettes, lay '
      f'an owner\'s gun\'s attachment layers on its canvas with the longer magazine in place and the camouflage on the '
      f'gun, and draw the attachments\' icons, each read once an image generation; '
      f'{COUNTS["python"]} Python, {COUNTS["loader"]} loader and {COUNTS["menu"]} menu mutants fail')
