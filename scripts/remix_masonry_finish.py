"""Refine preserved stone trims without repainting their coverage or joints."""
import base64
import struct

DONOR = 'Level_Specific/Carentan/textures/rock_moss_set_02_diff_4k.a.rtex.dds'
COLORS = {'v_dec_stonebottom_01', 'v_dec_stonebottom_02', 'v_dec_stonebottom_03'}
MATERIALS = {'materials/v_dec_stonewall05', 'materials/v_dec_stonewall06', 'materials/v_dec_stonewall07'}
DETAIL = 'browser_remix/masonry_mineral_detail'


def mineral_detail(donor):
    import numpy as np
    from PIL import Image, ImageFilter
    # Inside one continuous rock UV island, excluding the atlas's dilated edges.
    # The box is normalized to the owner's 2048px source, not the export size.
    box = tuple(round(v * donor.width / 2048) for v in (280, 360, 460, 540))
    patch = donor.crop(box).convert('L').resize((128, 128), Image.Resampling.LANCZOS)
    raw = np.asarray(patch, dtype=np.float32)
    low = np.asarray(patch.filter(ImageFilter.GaussianBlur(4)), dtype=np.float32)
    grain = np.clip((raw - low) * 0.45, -7, 7)
    # Mirrored edges are value-continuous in both repeat directions. Only the
    # mineral grain transfers; the rock's moss color and large cracks do not.
    tile = np.block([[grain, grain[:, ::-1]], [grain[::-1], grain[::-1, ::-1]]])
    return Image.fromarray(np.rint(127.5 + tile).astype(np.uint8), 'L').convert('RGBA')


def trim_normal(normal):
    import numpy as np
    from PIL import Image, ImageFilter
    raw = np.asarray(normal, dtype=np.float32)
    low = np.asarray(normal.filter(ImageFilter.GaussianBlur(1.25)), dtype=np.float32)
    result = raw.copy()
    # Keep the original GGGR tangent frame and broader mortar/stone shape.
    # Pixel-sized chiseling moves to the fine finish rather than dominating
    # diffuse light. Filter in native texels; never blur across UV atlas islands.
    for channel in (1, 3):
        result[:, :, channel] = 127.5 + 0.4 * (low[:, :, channel] - 127.5) + 0.09 * (raw[:, :, channel] - low[:, :, channel])
    result[:, :, 0] = result[:, :, 2] = result[:, :, 1]
    return Image.fromarray(np.clip(np.rint(result), 0, 255).astype(np.uint8), 'RGBA')


def masonry_assets(names, read_material, read_image, load):
    from browser_remix_assets import material_textures, write_iwi
    from remix_weapon_finish import decode_iwi
    entries, materials, originals = {}, {}, {}
    for name in names:
        if name not in MATERIALS:
            continue
        data = read_material(name)
        if len(data) < 68:
            continue
        textures = material_textures(data)
        maps = {key: image for _, key, image in textures}
        if maps.get('colorMap') not in COLORS:
            continue
        if maps.get('detailMap') != 'core_01' or 'normalMap' not in maps:
            raise ValueError('Unexpected stone trim layout: ' + name)
        family = 'trim_' + maps['colorMap']
        normal_name = 'browser_remix/' + family + '_n'
        image_name = 'images/' + normal_name + '.iwi'
        if image_name not in entries:
            normal = decode_iwi(read_image('images/' + maps['normalMap'] + '.iwi'))
            entries[image_name] = write_iwi([trim_normal(normal)])
        changed = bytearray(data)
        for offset, key, image in textures:
            replacement = normal_name if key == 'normalMap' else DETAIL if key == 'detailMap' else None
            if replacement:
                struct.pack_into('<I', changed, offset + 8, len(changed))
                changed.extend(replacement.encode('ascii') + b'\0')
        entries[name] = bytes(changed)
        materials[name] = {'family': family, 'original_color': maps['colorMap'],
                           'transfer': 'donor mineral grain; restrained microrelief; original color, opacity, specular and UVs'}
        originals[name] = base64.b64encode(data).decode('ascii')
    if materials:
        entries['images/' + DETAIL + '.iwi'] = write_iwi([mineral_detail(load(DONOR, 0))])
    return entries, materials, originals
