"""The owner's pictures of class weapons, which the class menus draw in place of HUD icon silhouettes."""
import struct
import zlib

# The owner may keep a folder of pictures beside the original IWDs, never in
# Git: a PNG for each class weapon it pictures, named after the weapon. The
# class menus draw them in their own colors; the HUD keeps the white icons.
OWNER_PICTURES_DIR = 'owner_class_pictures'
PICTURE_FOLDER = 'images/browser_class/'
# As r_hud2d.c reads a picture: sides up to R_HUD_PICTURE_MAX_SIZE, and only
# texels above HUD_SILHOUETTE_ALPHA_MIN visible.
PICTURE_MAX_SIZE = 1024
PICTURE_ALPHA_MIN = 16
# A gun's pictures with each attachment the menus lay over it, or its longer magazine in place.
ATTACHMENT_LAYERS = ('extmag', 'suppressor', 'reflex', 'lens', 'optic4x')
IWI_HEADER = struct.Struct('<3sBBBHHH4i')
IWI_ARGB32 = 1
IWI_NOPICMIP = 1


def read_png(data, name):
    """The width, height and RGBA bytes of an 8-bit RGBA PNG without interlacing."""
    def invalid(reason):
        return ValueError(f'{OWNER_PICTURES_DIR}/{name}: {reason}')

    if data[:8] != b'\x89PNG\r\n\x1a\n':
        raise invalid('not a PNG file')
    position, header, compressed, ended = 8, None, [], False
    while position + 12 <= len(data) and not ended:
        length, kind = struct.unpack_from('>I4s', data, position)
        body = data[position + 8:position + 8 + length]
        if len(body) != length or position + 12 + length > len(data):
            raise invalid('a chunk runs past the end of the file')
        if struct.unpack_from('>I', data, position + 8 + length)[0] != zlib.crc32(kind + body):
            raise invalid(f'damaged {kind.decode("latin-1")} chunk')
        if kind == b'IHDR':
            header = struct.unpack('>IIBBBBB', body) if length == 13 else ()
        elif kind == b'IDAT':
            compressed.append(body)
        ended = kind == b'IEND'
        position += 12 + length
    if not header or not ended or not compressed:
        raise invalid('missing its header, image data or end')
    width, height, depth, color, method, filtering, interlace = header
    if (depth, color, method, filtering, interlace) != (8, 6, 0, 0, 0):
        raise invalid('use 8-bit RGBA without interlacing')
    if not 1 <= width <= PICTURE_MAX_SIZE or not 1 <= height <= PICTURE_MAX_SIZE or \
            width & (width - 1) or height & (height - 1):
        raise invalid(f'{width} x {height}; use sides of 1 to {PICTURE_MAX_SIZE} texels, each a power of two')
    try:
        raw = zlib.decompress(b''.join(compressed))
    except zlib.error:
        raise invalid('damaged image data') from None
    stride = width * 4
    if len(raw) != height * (stride + 1):
        raise invalid('image data of the wrong size')
    rows, previous = [], bytearray(stride)
    for y in range(height):
        kind, line = raw[y * (stride + 1)], bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        if kind == 1:
            for x in range(4, stride):
                line[x] = (line[x] + line[x - 4]) & 255
        elif kind == 2:
            for x in range(stride):
                line[x] = (line[x] + previous[x]) & 255
        elif kind == 3:
            for x in range(stride):
                line[x] = (line[x] + ((line[x - 4] if x >= 4 else 0) + previous[x]) // 2) & 255
        elif kind == 4:
            for x in range(stride):
                a, b, c = (line[x - 4] if x >= 4 else 0), previous[x], (previous[x - 4] if x >= 4 else 0)
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                line[x] = (line[x] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        elif kind != 0:
            raise invalid(f'unknown row filter {kind}')
        rows.append(bytes(line))
        previous = line
    pixels = b''.join(rows)
    if max(pixels[3::4]) <= PICTURE_ALPHA_MIN:
        raise invalid('nothing in it is visible')
    return width, height, pixels


def halve(width, height, rgba):
    """The next mip: each texel the mean of the two or four it covers, colors weighted by their alpha."""
    half_width, half_height = max(width // 2, 1), max(height // 2, 1)
    stride = width * 4
    starts, step = ((0, 4), 8) if width > 1 else ((0,), 4)
    out = bytearray()
    for y in range(half_height):
        lines = [rgba[row * stride:(row + 1) * stride] for row in sorted({2 * y, min(2 * y + 1, height - 1)})]
        count = len(lines) * len(starts)
        alphas = list(zip(*(line[start + 3::step] for line in lines for start in starts)))
        weights = [sum(texels) for texels in alphas]
        row = bytearray(half_width * 4)
        for channel in range(3):
            colors = zip(*(line[start + channel::step] for line in lines for start in starts))
            row[channel::4] = bytes((sum(c * a for c, a in zip(texels, alpha)) + weight // 2) // weight if weight else
                                    (sum(texels) + count // 2) // count
                                    for texels, alpha, weight in zip(colors, alphas, weights))
        row[3::4] = bytes((weight + count // 2) // count for weight in weights)
        out += row
    return half_width, half_height, bytes(out)


def package_picture(data, name):
    """The picture as the engine loads it: an A8R8G8B8 IWi without picmip, every mip from 1 x 1 up to its size."""
    width, height, rgba = read_png(data, name)
    levels = [(width, height, rgba)]
    while levels[-1][0] > 1 or levels[-1][1] > 1:
        levels.append(halve(*levels[-1]))
    mips = []
    for _, _, texels in reversed(levels):
        bgra = bytearray(len(texels))
        bgra[0::4], bgra[1::4], bgra[2::4], bgra[3::4] = texels[2::4], texels[1::4], texels[0::4], texels[3::4]
        mips.append(bytes(bgra))
    total = IWI_HEADER.size + sum(len(mip) for mip in mips)
    # The file's size, then its size without the largest mip, two, three.
    largest = [len(mip) for mip in reversed(mips)]
    sizes = [total - sum(largest[:level]) for level in range(4)]
    return IWI_HEADER.pack(b'IWi', 5, IWI_ARGB32, IWI_NOPICMIP, width, height, 1, *sizes) + b''.join(mips)


def load_owner_pictures(folder, weapons, attachments=()):
    """The owner's pictures in folder by the archive name each takes, or None without the folder.

    Each file is, in any case, <weapon>.png for one of the class weapons; <weapon>__<layer>.png for
    one of a gun's attachment layers (render-class-pictures.py draws them on the gun's canvas); or
    attachment__<attachment>.png for an attachment's icon."""
    if not folder.is_dir():
        return None
    names = {weapon.lower(): weapon for weapon in weapons}
    names.update({f'{weapon.lower()}__{layer}': weapon for weapon in weapons for layer in ATTACHMENT_LAYERS})
    names.update({f'attachment__{attachment}': attachment for attachment in attachments})
    pictures = {}
    for path in sorted(folder.iterdir()):
        if path.name.startswith('.'):
            continue
        stem = path.name[:-4].lower() if path.name.lower().endswith('.png') else None
        if not path.is_file() or stem not in names:
            raise ValueError(f'{OWNER_PICTURES_DIR}/{path.name}: name it <weapon>.png after a class weapon, '
                             'such as frag_grenade_american_mp.png, <weapon>__<layer>.png for one of '
                             f'{", ".join(ATTACHMENT_LAYERS)} or attachment__<attachment>.png')
        if PICTURE_FOLDER + stem + '.iwi' in pictures:
            raise ValueError(f'{OWNER_PICTURES_DIR}/{path.name}: {names[stem]} has two pictures')
        pictures[PICTURE_FOLDER + stem + '.iwi'] = package_picture(path.read_bytes(), path.name)
    return pictures


def verify_owner_pictures(entries, owner):
    """Check that a package holds the owner's pictures, and only them, as the generator makes them.

    owner is what load_owner_pictures returned."""
    packaged = {name: data for name, data in entries.items() if name.startswith(PICTURE_FOLDER)}
    assert packaged == (owner or {}), 'Packaged class pictures differ from the owner\'s'
