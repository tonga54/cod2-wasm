"""Store the donor grenade smoke's shading in the full color range.

The shared soot atlas only uses 0..50 out of 255. Encoding that range in
DXT5 throws away most of its billow shading. Dedicated grenade maps store
the same field at full range; original compressed alpha blocks are retained.
"""
import base64
import struct

MATERIALS = {
    'materials/browser_smoke_screen_gfx_dark_smoke_atlas_bg': ('dark_smoke_atlas', 2),
    'materials/browser_smoke_screen_gfx_smokepuff_atlas': ('smokpuff_atlased', 4),
}
SOURCE = 'Textures/Smoke1.a.rtex.dds'


def preserve_alpha(encoded, original):
    if (original[:5] != b'IWi\x05\x0d' or encoded[:5] != original[:5] or
            len(encoded) != len(original) or encoded[6:10] != original[6:10] or
            (len(original) - 28) % 16):
        raise ValueError('Smoke alpha preservation requires matching DXT5 mip chains')
    result = bytearray(encoded)
    result[:28] = original[:28]
    for start in range(28, len(result), 16):
        result[start:start + 8] = original[start:start + 8]
    return bytes(result)


def smoke_assets(names, read_material, read_image, load):
    from PIL import Image
    from browser_remix_assets import material_textures, write_iwi
    source = load(SOURCE, 2048).convert('L')
    if source.size != (2048, 2048):
        raise ValueError('Expected the donor 8x8 smoke atlas at 2048 pixels')
    field = source.point(lambda value: min(255, round(value * 255 / 50)))
    entries, materials, originals = {}, {}, {}
    for name, (image, grid) in MATERIALS.items():
        if name not in names:
            continue
        before = read_material(name)
        maps = material_textures(before)
        if len(maps) != 1 or maps[0][1:] != ('colorMap', image) or before[14:16] != bytes((grid, grid)):
            raise ValueError('Unexpected smoke-screen layout: ' + name)
        original = read_image('images/' + image + '.iwi')
        if struct.unpack_from('<HH', original, 6) != (grid * 256, grid * 256):
            raise ValueError('Unexpected smoke-screen resolution: ' + image)
        atlas = Image.new('RGBA', (grid * 256, grid * 256))
        frames = [round(i * 63 / (grid * grid - 1)) for i in range(grid * grid)]
        for i, frame in enumerate(frames):
            x, y = frame % 8 * 256, frame // 8 * 256
            atlas.paste(field.crop((x, y, x + 256, y + 256)).convert('RGBA'),
                        (i % grid * 256, i // grid * 256))
        target = 'browser_remix/smoke_volume_' + image
        entries['images/' + target + '.iwi'] = preserve_alpha(write_iwi([atlas]), original)
        after = bytearray(before)
        struct.pack_into('<I', after, maps[0][0] + 8, len(after))
        after.extend(target.encode('ascii') + b'\0')
        entries[name] = bytes(after)
        originals[name] = base64.b64encode(before).decode('ascii')
        materials[name] = {'family': 'smoke_volume', 'source': SOURCE, 'frames': frames,
            'transfer': 'full-range donor billow shading; original compressed alpha at every mip',
            'alpha_source': 'images/' + image + '.iwi'}
    return entries, materials, originals
