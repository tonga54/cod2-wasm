"""Adapt the owner's complete door surfaces to the retail brush UV layouts."""
import base64

# These are complete opaque doors, not tile/trim/window atlases. The panel
# door is stored sideways in retail; rotate its tangent frame along with UVs.
DOORS = {
    'v_door_01': ('joinery_plank', 'Textures/Door_01/Door_001', False),
    'v_door_03': ('joinery_panel', 'Level_Specific/Carentan/textures/Door_02', True),
}


def orient(image, sideways, normal=False):
    from PIL import Image
    if not sideways:
        return image.copy()
    rotated = image.transpose(Image.Transpose.ROTATE_270)
    if normal:
        # u'=-v, v'=u. BC5's XY are in RG; reflect the signed Y channel.
        import numpy as np
        data = np.asarray(rotated.convert('RGB')).copy()
        x, y = data[:, :, 0].copy(), data[:, :, 1].copy()
        data[:, :, 0], data[:, :, 1] = 255-y, x
        rotated = Image.fromarray(data, 'RGB')
    return rotated


def joinery_assets(names, read_material, read_image, load, size=2048):
    from PIL import Image
    from browser_remix_assets import material_textures, replace_material, write_iwi, normal_image, specular_image, cstring
    from remix_weapon_finish import decode_iwi
    import struct
    entries, materials, originals = {}, {}, {}
    for name in names:
        if not name.startswith('materials/') or name.startswith('materials/statemaps/'):
            continue
        data = read_material(name)
        if len(data) < 68:
            continue
        maps = {key: image for _, key, image in material_textures(data)}
        color = maps.get('colorMap')
        if color not in DOORS:
            continue
        technique = cstring(data, struct.unpack_from('<I', data, 56)[0])
        if technique not in ('phong_replace_spec_detail', 'phong_replace_spec'):
            raise ValueError('Unexpected door technique: ' + name + ': ' + technique)
        original_color = decode_iwi(read_image('images/' + color + '.iwi'))
        if original_color.getchannel('A').getextrema() != (255,255):
            raise ValueError('Door color contains coverage: ' + color)
        family, prefix, sideways = DOORS[color]
        if 'images/browser_remix/' + family + '_c.iwi' not in entries:
            def source(suffix, normal=False):
                image = load(prefix + '_' + suffix + '.rtex.dds', min(size,2048))
                return orient(image, sideways, normal)
            color_image = source('BaseColor.a').convert('RGBA')
            color_image.putalpha(255)
            normal = normal_image(source('OTH_Normal.n', True))
            rough = source('Roughness.r')
            height = source('Height.h').convert('L')
            detail = Image.new('RGBA', height.size, (128,128,128,255))
            detail.putalpha(height)
            for channel, image, compression in [('c',color_image,'DXT1'), ('n',normal,'DXT5'),
                                                ('s',specular_image(rough),'DXT5'), ('d',detail,'DXT5')]:
                entries[f'images/browser_remix/{family}_{channel}.iwi'] = write_iwi([image],compression)
        entries[name] = replace_material(data, family, height=True)
        originals[name] = base64.b64encode(data).decode('ascii')
        materials[name] = {'family':family,'original_color':color,
            'transfer':'complete donor door with aligned color, normal, roughness and height; original brush geometry and render state',
            'rotation':'clockwise 90 with tangent basis' if sideways else 'none'}
    return entries, materials, originals
