"""Material classes for the retail hand atlases, preserving their painted maps.

Detail alpha is classification data: cloth=0, skin=85, leather=170, metal=255.
Regions follow the original UV islands, not color guesses (snow is not metal).
"""
import struct
from browser_remix_assets import cstring, material_textures
from browser_grenade_explosions import write_iwi_rgba

PROFILES = {
    'mtl_hands_us': ('hand_us_c', 'standard'),
    'mtl_hands_german': ('hand_german_c', 'standard'),
    'mtl_hands_british_bare': ('hand_british_bare_c', 'british'),
    'mtl_hands_german_winter': ('hand_german_winter_c', 'winter'),
    'mtl_russian_winter_sleeve': ('hand_russain_winter_color', 'russian'),
    'mtl_hands_hair': ('hand_hair_c', 'skin'),
}


def inside(x, y, points):
    result = False
    previous = points[-1]
    for current in points:
        ax, ay = previous
        bx, by = current
        if (ay > y) != (by > y) and x < (bx-ax)*(y-ay)/(by-ay)+ax:
            result = not result
        previous = current
    return result


def category(profile, x, y):
    # Coordinates are in the original 1024 x 512 atlas, independently of mask
    # resolution. Padding around UV islands may share the adjacent category.
    if profile == 'winter':
        return 0
    if profile == 'russian':
        return 85 if x >= 816 else 0
    if profile == 'standard':
        if (x-26)**2 + (y-220)**2 < 15**2:
            return 255  # Cuff button, not a bright patch of fabric.
        return 0 if x < 512 else 85
    if profile == 'british':
        if 56 <= x < 156 and y < 174 or x < 56 and 216 <= y < 392:
            return 255  # Watch case/face and separate metal buckle island.
        if x < 56 and y < 216 or inside(x, y, (
                (153, 9), (184, 50), (180, 140), (139, 231), (140, 330),
                (167, 390), (134, 400), (104, 351), (81, 281), (88, 224),
                (123, 154), (139, 63))):
            return 170
        return 0 if 70 <= x < 512 and y >= 400 else 85
    if profile == 'skin':
        return 85
    raise ValueError('Unknown character atlas: ' + profile)


def bind_mask(data, image):
    out = bytearray(data)
    def string(value):
        offset = len(out)
        out.extend(value.encode('ascii') + b'\0')
        return offset
    entries = []
    for offset, name, _ in material_textures(data):
        if name != 'detailMap':
            entries.append(data[offset:offset+12])
    entries.append(struct.pack('<IBBBBI', string('detailMap'), 0, 2, 0, 0, string(image)))
    struct.pack_into('<H', out, 52, len(entries))
    struct.pack_into('<I', out, 60, len(out))
    out.extend(b''.join(entries))
    count, table = struct.unpack_from('<H', data, 54)[0], struct.unpack_from('<I', data, 64)[0]
    constants = []
    for index in range(count):
        entry = data[table+index*20:table+(index+1)*20]
        if cstring(data, struct.unpack_from('<I', entry)[0]) != 'detailScale':
            constants.append(entry)
    constants.append(struct.pack('<I4f', string('detailScale'), 1, 1, 0, 0))
    struct.pack_into('<H', out, 54, len(constants))
    struct.pack_into('<I', out, 64, len(out))
    out.extend(b''.join(constants))
    return bytes(out)


def character_material_assets(read):
    assets = {}
    for name, (color, profile) in PROFILES.items():
        material = read('materials/' + name)
        maps = {key: image for _, key, image in material_textures(material)}
        if maps.get('colorMap') != color:
            raise ValueError('Character UV atlas changed: ' + name)
        image = 'browser_remix/' + name + '_character'
        width, height = 128, 64
        pixels = bytes(value for y in range(height) for x in range(width)
                       for value in (128, 128, 128, category(profile, (x+.5)*8, (y+.5)*8)))
        assets['images/' + image + '.iwi'] = write_iwi_rgba(width, height, pixels)
        assets['materials/' + name] = bind_mask(material, image)
    return assets
