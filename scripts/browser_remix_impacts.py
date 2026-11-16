"""Owner impact decals: aligned color, normal and height; original mark behavior."""
import base64
from browser_remix_assets import replace_material, normal_image, write_iwi

IMPACTS = {
    'impact_stone': ('Props/Bullet_Hole_Concrete_01/Bullet_Hole_Concrete_01', ['gfx_bullethit_stone']),
    'impact_plaster': ('Props/Bullet_Hole_Concrete_01/Bullet_Hole_Concrete_01', ['gfx_bullethit_plaster', 'gfx_bullethit_plaster2']),
    'impact_wood1': ('Textures/Bullet_Hole_Wood_01/Bullet_Hole_Wood_01', ['gfx_bullethit_wood1']),
    'impact_wood2': ('deps/mods/project/Textures/Bullet_Hole_Wood_02/Bullet_Hole_Wood_02', ['gfx_bullethit_wood2']),
}


def impact_assets(read, load):
    from PIL import Image
    entries, materials, originals = {}, {}, {}
    for family, (stem, targets) in IMPACTS.items():
        color = load(stem + '_albedo.a.rtex.dds', 512).convert('RGBA')
        normal = normal_image(load(stem + '_normal_OTH_Normal.n.rtex.dds', 512))
        height = load(stem + '_height.h.rtex.dds', 512).convert('L')
        if normal.size != color.size or height.size != color.size:
            raise ValueError('Unaligned impact maps: ' + family)
        detail = Image.new('RGBA', color.size, (128, 128, 128, 255)); detail.putalpha(height)
        for suffix, image in [('c', color), ('n', normal), ('d', detail)]:
            entries[f'images/browser_remix/{family}_{suffix}.iwi'] = write_iwi([image], 'DXT5')
        for target in targets:
            name = 'materials/' + target
            original = read(name)
            entries[name] = replace_material(original, family, height=True, specular=False)
            materials[name] = {'family': family, 'transfer': 'aligned donor color, tangent normal and height decal'}
            originals[name] = base64.b64encode(original).decode('ascii')
    return entries, materials, originals
