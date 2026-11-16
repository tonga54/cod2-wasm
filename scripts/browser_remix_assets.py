"""Private, portable Carentan Remix material pack; never executes mod scripts.

The optional converter uses Pillow >= 13 and numpy. Runtime/ordinary builds
only need the standard library and the already converted owner pack.
"""
from pathlib import Path
import hashlib
import io
import json
import math
import re
import struct
import zipfile
import base64

PACK = 'owner_carentan_visuals.zip'
TEX = 'Textures/'
LEVEL = 'Level_Specific/Carentan/textures/'


def pbr(prefix):
    return [prefix + suffix for suffix in ('_BaseColor.a.rtex.dds',
            '_OTH_Normal.n.rtex.dds', '_Roughness.r.rtex.dds')]


def scanned(prefix):
    return [prefix + suffix for suffix in ('_diff_4k.a.rtex.dds',
            '_nor_gl_4k_OTH_Normal.n.rtex.dds', '_rough_4k.r.rtex.dds')]


FAMILIES = {
    'cobble': pbr(TEX + 'Cobble_001'),
    # Water is a separate shader layer. Keep the aligned stone bed intact
    # beneath it; the donor wet normal/height flatten the entire old puddle.
    'cobble_wet': pbr(TEX + 'Cobble_001')[:2] +
                  pbr('Props/Road_Cobble_Wet/textures/Cobble_001_Wet_T')[2:],
    'grass': pbr(TEX + 'Grass_Patchy_002'),
    'mud': scanned(TEX + 'brown_mud'),
    'track': pbr(TEX + 'Track_01'),
    'rubble': pbr(TEX + 'Rubble_Pile_Final'),
    'wood': scanned(TEX + 'dark_wooden_planks'),
    'brick': pbr(LEVEL + 'Bricks_Messy_Blocks_002'),
    'brick_old': pbr(TEX + 'Bricks/Brick_Wall_Large_Old_001_F1'),
    'stone': pbr(LEVEL + 'Wall_Stone_001'),
    'stone_dark': pbr(LEVEL + 'Wall_Stone_003'),
    'sandstone': scanned(LEVEL + 'large_sandstone_blocks_01'),
    'pavers': scanned(LEVEL + 'stone_pavers'),
    'concrete': scanned(LEVEL + 'concrete_wall_007'),
    'plaster': pbr(LEVEL + '3_tiled_plane_Plaster_Beige_001'),
    'plaster_cracked': pbr(LEVEL + 'Brick_Messy_Plaster_Crack03'),
    'plaster_blue': scanned(LEVEL + 'blue_plaster_weathered'),
    'slate': scanned(LEVEL + 'roof_slates_03'),
    'roof': scanned(LEVEL + 'grey_roof_tiles_02'),
    'wallpaper': pbr(LEVEL + 'Wallpaper_Plain'),
}

# Deliberate image matches: an atlas, trim, decal, snowy surface or model UV
# cannot safely take an unrelated tile. Shared images cover all map variants.
TARGETS = {
    'cobble': 'cobble cobblestone_01 cobblestone_03',
    'cobble_wet': 'caen_road_cobblestone_01',
    'grass': 'v_grass',
    'mud': 'v_mud_01 mudroad01',
    'track': 'v_mud_track',
    'rubble': 'v_rubble01 rubble_01 rubble_white',
    'wood': 'v_wood_floor1 v_wood_floor2 v_woodfloor01 v_woodwall wood01 wood02 '
            'wood_floor2 wood_h_01 wood_v_01 stalingrad_greywood_floor '
            'caen_ceiling_wood_01 caen_wall_ext_wood_01 egypt_wood_gereric1 market_wood1',
    'brick': 'v_brick01 stalingrad_brickwall01 stalingrad_brickwall03 '
             'stalingrad_brickwall05 stalingrad_brickwall06 stalingrad_brickwall09',
    'brick_old': 'v_brick02 hill400_wall_ext_brick_01_03',
    'stone': 'v_stonewall01 v_stonewall02 v_stonewall_01 v_stonewall_02 silotown_stone',
    'stone_dark': 'v_stonewall03 v_stonewall04 v_stonewall_03 v_wall_ext_stone_01 v_wall_ext_stone_04',
    'sandstone': 'egypt_sandstone_wall1 egypt_sandstone_broken_generic1 '
                 'egypt_brick_sand_generic1_body1 egypt_brick_sand_generic1_body2 toujane_stonewall1',
    'pavers': 'v_core_floor_01 v_core_floor_02',
    'concrete': 'stalingrad_concrete01 stalingrad_concrete04 egypt_concrete_floor1_body '
                'egypt_concrete_generic1_body stalingrad_concrete_floor01 stalingrad_concrete_ceiling01 '
                'stalingrad_concretewall03 stalingrad_concretewall06',
    'plaster': 'v_plasterwall egypt_plaster_stucco1 egypt_plaster_stucco1_body1 '
               'egypt_plaster_exteriorwall01_body egypt_plaster_exteriorwall02 '
               'toujane_plasterwallmid stalingrad_plasterwall01 caen_wall_ext_plaster_02 white_wall white_wall2',
    'plaster_cracked': 'v_plasterwall_03',
    'plaster_blue': 'blue_wall',
    'slate': 'v_roof01 v_roof02',
    'roof': 'v_roof03 v_roof04',
    'wallpaper': 'villers_wallparer1',
}
IMAGE_FAMILY = {name: family for family, names in TARGETS.items() for name in names.split()}
HEIGHTS = {
    'cobble': TEX+'Cobble_001_Height.a.rtex.dds',
    'cobble_wet': TEX + 'Cobble_001_Height.a.rtex.dds',
    'grass': TEX+'Grass_Patchy_002_Height.a.rtex.dds',
    'mud': TEX+'brown_mud_disp_4k.a.rtex.dds',
    'track': TEX+'Track_01_Height.a.rtex.dds',
    'rubble': TEX+'Rubble_Pile_Final_Height.a.rtex.dds',
    'wood': TEX+'dark_wooden_planks_disp_4k.a.rtex.dds',
    'brick': LEVEL+'Bricks_Messy_Blocks_002_Height.h.rtex.dds',
    'brick_old': TEX+'Bricks/Brick_Wall_Large_Old_001_F1_Height.a.rtex.dds',
    'stone_dark': LEVEL+'Wall_Stone_003_Height.h.rtex.dds',
    'pavers': LEVEL+'stone_pavers_disp_4k.h.rtex.dds',
    'plaster': LEVEL+'3_tiled_plane_Plaster_Beige_001_Height.h.rtex.dds',
    'plaster_cracked': LEVEL+'Brick_Messy_Plaster_Crack03_Height.h.rtex.dds',
    'plaster_blue': LEVEL+'blue_plaster_weathered_disp_4k.h.rtex.dds',
    'slate': LEVEL+'roof_slates_03_disp_4k.h.rtex.dds',
    'roof': LEVEL+'grey_roof_tiles_02_disp_4k.h.rtex.dds',
}

# Source panoramas are linear BC6H, very dark before Remix's exposure.
# Keep different weather for France, North Africa and the Russian maps.
SKIES = {
    'silotown_ft': ('sky_sun', 1.25),
    'dawnville2_ft': ('sky', 2.5),
    'rhine_ft': ('sky03', 3.0),
    'toujane_ft': ('sky_sun', 1.45),
    'libyatown_ft': ('sky02', 3.0),
    'trainyard_ft': ('sky03', 6.0),
    'moscow_ft': ('sky03', 5.5),
}


def cstring(data, offset):
    if offset < 0 or offset >= len(data):
        raise ValueError('Invalid material string offset')
    end = data.index(0, offset)
    return data[offset:end].decode('ascii')


def material_textures(data):
    if len(data) < 68:
        raise ValueError('Truncated material')
    count = struct.unpack_from('<H', data, 52)[0]
    table = struct.unpack_from('<I', data, 60)[0]
    if table + count * 12 > len(data):
        raise ValueError('Truncated material texture table')
    return [(table + i * 12, cstring(data, struct.unpack_from('<I', data, table + i * 12)[0]),
             cstring(data, struct.unpack_from('<I', data, table + i * 12 + 8)[0])) for i in range(count)]


def replace_material(data, family, *, height=False, specular=True, metallic=False):
    """Append pointers/tables, retaining technique, alpha, UV scale and flags."""
    if metallic and (height or family in HEIGHTS):
        raise ValueError('Height and metalness cannot share the detail alpha')
    textures = material_textures(data)
    out = bytearray(data)
    entries = []
    def string(s):
        offset = len(out)
        out.extend(s.encode('ascii') + b'\0')
        return offset
    maps = {'colorMap': ('c', 2), 'normalMap': ('n', 3), 'specularMap': ('s', 4)}
    if not specular:
        del maps['specularMap']
    if family in HEIGHTS or height:
        maps['detailMap'] = ('d', 2)
    elif metallic:
        maps['detailMap'] = ('metal', 2)
    present = set()
    for offset, name, image in textures:
        entry = bytearray(data[offset:offset + 12])
        if name in maps:
            channel, semantic = maps[name]
            struct.pack_into('<I', entry, 8, string(f'browser_remix/{family}_{channel}'))
            entry[5] = semantic
            present.add(name)
        elif name == 'detailMap':
            struct.pack_into('<I', entry, 8, string('browser_remix/neutral_detail'))
        entries.append(entry)
    for name, (channel, semantic) in maps.items():
        if name not in present:
            entries.append(struct.pack('<IBBBBI', string(name), 0, semantic, 0, 0,
                                       string(f'browser_remix/{family}_{channel}')))
    struct.pack_into('<H', out, 52, len(entries))
    struct.pack_into('<I', out, 60, len(out))
    out.extend(b''.join(entries))
    if family in HEIGHTS or height or metallic:
        count = struct.unpack_from('<H', data, 54)[0]
        table = struct.unpack_from('<I', data, 64)[0]
        if table + count * 20 > len(data):
            raise ValueError('Invalid material constant table')
        constants = []
        found = False
        for i in range(count):
            offset = table + i * 20
            entry = bytearray(data[offset:offset+20])
            if cstring(data, struct.unpack_from('<I', entry)[0]) == 'detailScale':
                struct.pack_into('<4f', entry, 4, 1, 1, 0, 0)
                found = True
            constants.append(entry)
        if not found:
            constants.append(struct.pack('<I4f', string('detailScale'), 1, 1, 0, 0))
        struct.pack_into('<H', out, 54, len(constants))
        struct.pack_into('<I', out, 64, len(out))
        out.extend(b''.join(constants))
    return bytes(out)


def write_iwi(faces, compression='DXT5'):
    """Complete CoD2 mip chain, smallest first; cubemaps are mip-major."""
    from PIL import Image
    size = faces[0].size
    if len(faces) not in (1, 6) or any(im.size != size for im in faces):
        raise ValueError('Inconsistent image faces')
    if any(v < 1 or v & (v - 1) for v in size):
        raise ValueError('IWI dimensions must be powers of two')
    if len(faces) == 6 and size[0] != size[1]:
        raise ValueError('Cubemap faces must be square')
    levels = []
    while True:
        chunks = []
        for im in faces:
            if compression == 'RGB8':
                chunks.append(im.convert('RGB').tobytes('raw', 'BGR'))
                continue
            buf = io.BytesIO()
            # DXT encoders require a complete block, even for the mip tail.
            im = im.convert('RGBA')
            if im.width < 4 or im.height < 4:
                im = im.resize((max(4, im.width), max(4, im.height)), Image.Resampling.NEAREST)
            im.save(buf, format='DDS', pixel_format=compression)
            encoded = buf.getvalue()[128:]
            if compression == 'DXT3':
                # Encode explicit alpha ourselves: Pillow's BC2 writer can
                # retain the low nibble instead of quantizing 8-bit alpha.
                import numpy as np
                alpha = np.asarray(im.getchannel('A'),dtype=np.uint16)
                alpha = ((alpha*15+127)//255).astype(np.uint8)
                blocks = alpha.reshape(im.height//4,4,im.width//4,4).transpose(0,2,1,3).reshape(-1,16)
                packed = np.frombuffer(encoded,dtype=np.uint8).copy().reshape(-1,16)
                packed[:,:8] = blocks[:,::2] | (blocks[:,1::2] << 4)
                encoded = packed.tobytes()
            chunks.append(encoded)
        levels.append(b''.join(chunks))
        if faces[0].size == (1, 1):
            break
        faces = [im.resize((max(1, im.width // 2), max(1, im.height // 2)), Image.Resampling.BOX) for im in faces]
    total = 28 + sum(map(len, levels))
    ends = [total - sum(map(len, levels[:i])) for i in range(4)]
    return (b'IWi' + bytes((5, {'DXT1': 11, 'DXT3': 12, 'DXT5': 13, 'RGB8': 2}[compression], 5 if len(faces) == 6 else 1))
            + struct.pack('<HHH4I', *size, 1, *ends) + b''.join(reversed(levels)))


def normal_image(image):
    """BC5 stores XY in RG; CoD2's tangent normal uses alpha/green (GGGR)."""
    import numpy as np
    from PIL import Image
    a = np.asarray(image.convert('RGB')).copy()
    x = a[:, :, 0]
    y = a[:, :, 1]
    return Image.fromarray(np.stack((y, y, y, x), axis=-1), 'RGBA')


def metalness_image(image):
    """Neutral detail RGB, authored linear metalness in independently compressed alpha."""
    from PIL import Image
    result = Image.new('RGBA', image.size, (128, 128, 128, 0))
    result.putalpha(image.convert('L'))
    return result


def specular_image(image):
    """Map perceptual roughness to the engine's 32 cosine-power entries."""
    import numpy as np
    from PIL import Image
    rough = np.asarray(image.convert('L'), dtype=np.float32) / 255
    power = np.clip(2 / np.maximum(rough, .12) ** 4 - 2, 1, 2048)
    table = np.arange(31, dtype=np.float32)
    powers = np.minimum(-math.log(10) / np.log(np.cos((180 - 175 * table / 30) * math.pi / 720)), 2048)
    index = np.abs(np.log(power[:, :, None]) - np.log(powers)).argmin(axis=-1)
    # These are dielectric building materials, not metal; retain restrained F0.
    rgb = np.full((*rough.shape, 3), 28, dtype=np.uint8)
    return Image.fromarray(np.dstack((rgb, np.rint(index * 255 / 31).astype(np.uint8))), 'RGBA')


def panorama_faces(image, side, exposure):
    """Equirectangular to D3D/OpenGL cube order, Z up in the game world."""
    import numpy as np
    from PIL import Image
    pixels = (np.asarray(image.convert('RGB'), dtype=np.float32) / 255
              if isinstance(image, Image.Image) else np.asarray(image, dtype=np.float32).copy())
    # Keep the HDR source in float through exposure and the filmic shoulder.
    # Quantizing the dark linear source first produces visible cloud bands.
    pixels *= exposure
    pixels = np.clip((pixels * (2.51 * pixels + .03)) / (pixels * (2.43 * pixels + .59) + .14), 0, 1)
    pixels = np.where(pixels <= .0031308, pixels * 12.92, 1.055 * pixels ** (1 / 2.4) - .055)
    axis = (np.arange(side, dtype=np.float32) + .5) * (2 / side) - 1
    s, t = np.meshgrid(axis, axis)
    one = np.ones_like(s)
    # Cube textures are addressed in world coordinates by the sky technique.
    vectors = [(one, -t, -s), (-one, -t, s), (s, one, t),
               (s, -one, -t), (s, -t, one), (-s, -t, -one)]
    h, w = pixels.shape[:2]
    result = []
    for x, y, z in vectors:
        length = np.sqrt(x*x + y*y + z*z)
        u = (np.arctan2(y, x) / (2 * np.pi) + .5) * w - .5
        v = (np.arccos(np.clip(z / length, -1, 1)) / np.pi) * h - .5
        x0, y0 = np.floor(u).astype(int), np.floor(v).astype(int)
        fx, fy = (u - x0)[..., None], (v - y0)[..., None]
        a = pixels[np.clip(y0, 0, h-1), x0 % w] * (1-fx) + pixels[np.clip(y0, 0, h-1), (x0+1) % w] * fx
        b = pixels[np.clip(y0+1, 0, h-1), x0 % w] * (1-fx) + pixels[np.clip(y0+1, 0, h-1), (x0+1) % w] * fx
        # Sub-LSB deterministic dither keeps the final 8-bit sky gradient smooth.
        noise = np.random.default_rng(311 + len(result)).uniform(-.5, .5, (side, side, 1))
        result.append(Image.fromarray(np.clip(np.rint((a*(1-fy)+b*fy)*255 + noise), 0, 255).astype(np.uint8), 'RGB'))
    return result


def weapon_family(name, textures):
    if not name.startswith(('materials/mtl_weapon_', 'materials/mtl_browser_att_viewmodel_')):
        return None
    if any(word in name for word in ('scope', 'grenade', 'shell', 'round', 'clip', 'bullet')):
        return None
    if name.startswith('materials/mtl_weapon_'):
        return 'finish_' + name.split('mtl_weapon_', 1)[1]
    color = textures.get('colorMap', '')
    if not color.startswith('weapon_') or not color.endswith('_c'):
        return None
    return 'finish_' + color[len('weapon_'):-len('_c')]


def refresh_weapon_finishes(source, renderer_path, output, size=2048):
    """Update only weapon atlases and matching attachment materials in a pack."""
    from PIL import Image
    from remix_weapon_finish import weapon_finish
    output = Path(output)
    with zipfile.ZipFile(output) as archive:
        report = json.loads(archive.read('remix-report.json'))
        entries = {name: archive.read(name) for name in report['entries']}
    for name, digest in report['entries'].items():
        if hashlib.sha256(entries[name]).hexdigest() != digest:
            raise ValueError('Corrupt Carentan pack entry: ' + name)
    donor_name = 'Uniform/test/textures/Enfield_002_001_Enfield_01_BaseColor.a.rtex.dds'
    donor_path = Path(source) / 'rtx-remix/mods/gameReadyAssets' / donor_name
    with Image.open(donor_path) as image:
        donor = image.copy()
    converted = {}
    with zipfile.ZipFile(renderer_path) as renderer:
        for name in renderer.namelist():
            if not name.startswith(('materials/mtl_weapon_', 'materials/mtl_browser_att_viewmodel_')):
                continue
            data = (base64.b64decode(report['source_materials'][name], validate=True)
                    if name in report['source_materials'] else renderer.read(name))
            technique = cstring(data, struct.unpack_from('<I', data, 56)[0])
            if not technique.startswith(('phong_replace', 'phong_blend')):
                continue
            maps = {key: image for _, key, image in material_textures(data)}
            family = weapon_family(name, maps)
            if not family or not all(key in maps and 'images/' + maps[key] + '.iwi' in renderer.namelist()
                                     for key in ('colorMap', 'normalMap', 'specularMap')):
                continue
            signature = tuple(maps[key] for key in ('colorMap', 'normalMap', 'specularMap'))
            if family in converted and converted[family] != signature:
                raise ValueError('Conflicting weapon atlases: ' + family)
            if family not in converted:
                print('Refining weapon:', family, flush=True)
                finish = weapon_finish(renderer.read, maps, donor, size)
                for channel, image in finish.items():
                    entries[f'images/browser_remix/{family}_{channel}.iwi'] = image
                converted[family] = signature
            entries[name] = replace_material(data, family, metallic=(f'images/browser_remix/{family}_metal.iwi' in entries))
            report['source_materials'][name] = base64.b64encode(data).decode('ascii')
            report['materials'][name] = {'family': family, 'original_color': maps['colorMap'],
                'transfer': ('explicit steel/dielectric UV mask and bounded roughness; original color, normals and markings' if family in ('finish_thompson', 'finish_m1garand', 'finish_mp40') else 'fine donor grain; reduced steel microrelief; original UVs, bevels and markings')}
    if not converted:
        raise ValueError('No weapon atlases available')
    report['sources'][donor_name] = hashlib.sha256(donor_path.read_bytes()).hexdigest()
    report['weapon_finish_revision'] = 5
    report['entries'] = {name: hashlib.sha256(data).hexdigest() for name, data in entries.items()}
    temporary = output.with_suffix('.tmp')
    with zipfile.ZipFile(temporary, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
        for name, data in sorted(entries.items()):
            info = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, data)
        archive.writestr('remix-report.json', json.dumps(report, indent=2) + '\n')
    temporary.replace(output)
    output.with_suffix('.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


def family_images(family, load, size):
    """Aligned donor color, normal, roughness and height for one world family."""
    entries = {}
    for channel, relative in zip(('c', 'n', 's'), FAMILIES[family]):
        im = load(relative, size if channel == 'c' else min(size, 1024 if channel == 'n' else 512))
        if channel == 'n':
            im = normal_image(im)
        elif channel == 's':
            im = specular_image(im)
        entries[f'images/browser_remix/{family}_{channel}.iwi'] = write_iwi([im], 'DXT1' if channel == 'c' else 'DXT5')
    if family in HEIGHTS:
        # RGB is subtle detail color; alpha carries continuous donor height.
        height = load(HEIGHTS[family], min(size, 1024)).convert('L')
        detail = height.point([round(120 + value * 16 / 255) for value in range(256)]).convert('RGBA')
        detail.putalpha(height)
        entries[f'images/browser_remix/{family}_d.iwi'] = write_iwi([detail], 'DXT5')
    return entries


def refresh_masonry_finishes(source, renderer_path, output, size=2048):
    from remix_masonry_finish import masonry_assets
    return refresh_surface_finishes(source, renderer_path, output, size, masonry_assets, 'masonry_finish_revision')


def refresh_road_bed(source, renderer_path, output, size=2048):
    """Restore the aligned stone bed without expanding standing-water coverage."""
    def convert(names, material, read_image, load):
        name = 'materials/mtl_caen_road_cobblestone_01'
        if name not in names:
            return {}, {}, {}
        before = material(name)
        maps = {kind: image for _, kind, image in material_textures(before)}
        if IMAGE_FAMILY.get(maps.get('colorMap')) != 'cobble_wet':
            raise ValueError('Unexpected wet-road source material')
        entries = family_images('cobble_wet', load, size)
        entries[name] = replace_material(before, 'cobble_wet')
        metadata = {name: {'family': 'cobble_wet', 'original_color': maps['colorMap'],
                          'transfer': 'aligned dry stone bed with original wet roughness boundaries for sparse water'}}
        return entries, metadata, {name: base64.b64encode(before).decode('ascii')}
    return refresh_surface_finishes(source, renderer_path, output, size, convert, 'road_bed_revision')


def refresh_joinery(source, renderer_path, output, size=2048):
    from remix_joinery import joinery_assets
    return refresh_surface_finishes(source, renderer_path, output, size,
        lambda *args: joinery_assets(*args, size=size), 'joinery_revision')


def refresh_fire(source, renderer_path, output, size=2048):
    from remix_fire_finish import fire_assets
    return refresh_surface_finishes(source, renderer_path, output, size, fire_assets, 'fire_finish_revision')


def refresh_smoke(source, renderer_path, output, size=2048):
    from remix_smoke_finish import smoke_assets
    return refresh_surface_finishes(source, renderer_path, output, size, smoke_assets, 'smoke_finish_revision')


def refresh_surface_finishes(source, renderer_path, output, size, convert, revision):
    """Update selected surfaces while retaining all unrelated owner assets."""
    from PIL import Image
    output = Path(output)
    with zipfile.ZipFile(output) as archive:
        report = json.loads(archive.read('remix-report.json'))
        entries = {name: archive.read(name) for name in report['entries']}
    for name, digest in report['entries'].items():
        if hashlib.sha256(entries[name]).hexdigest() != digest:
            raise ValueError('Corrupt Carentan pack entry: ' + name)
    def load(relative, limit):
        path = Path(source) / 'rtx-remix/mods/gameReadyAssets' / relative
        report['sources'][relative] = hashlib.sha256(path.read_bytes()).hexdigest()
        with Image.open(path) as image:
            result = image.copy()
        if limit:
            result.thumbnail((limit, limit), Image.Resampling.LANCZOS)
        return result
    with zipfile.ZipFile(renderer_path) as renderer:
        def material(name):
            current = renderer.read(name)
            original = report['source_materials'].get(name)
            if original:
                original = base64.b64decode(original, validate=True)
                if current not in (original, entries.get(name)):
                    raise ValueError('Surface material has local changes: ' + name)
                return original
            return current
        updates, materials, originals = convert(renderer.namelist(), material, renderer.read, load)
        if not materials:
            raise ValueError('No matching surface materials available')
        entries.update(updates)
        report['materials'].update(materials)
        report['source_materials'].update(originals)
        for name in renderer.namelist():
            if name.endswith('.d3dbsp'):
                data = renderer.read(name)
                length, offset = struct.unpack_from('<II', data, 8)
                names = [cstring(data, i) for i in range(offset, offset + length, 72)]
                report['maps'][Path(name).stem] = [n for n in names if 'materials/' + n in report['materials']]
    report[revision] = 1
    report['entries'] = {name: hashlib.sha256(data).hexdigest() for name, data in entries.items()}
    temporary = output.with_suffix('.tmp')
    with zipfile.ZipFile(temporary, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
        for name, data in sorted(entries.items()):
            info = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, data)
        archive.writestr('remix-report.json', json.dumps(report, indent=2) + '\n')
    temporary.replace(output)
    output.with_suffix('.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


def build_pack(source, renderer_path, output, size=2048):
    from PIL import Image
    source = Path(source)
    base = source / 'rtx-remix/mods/gameReadyAssets'
    if not (base / 'mod.usda').is_file():
        raise ValueError('Expected the owner Carentan RTX Remix folder')
    entries, material_report, source_hashes, source_materials = {}, {}, {}, {}
    previous, previous_entries = {}, {}
    if Path(output).is_file():
        with zipfile.ZipFile(output) as archive:
            previous_report = json.loads(archive.read('remix-report.json'))
            previous = previous_report.get('source_materials', {})
            previous_entries = previous_report.get('entries', {})
    donor_weapon = None
    def load(relative, limit):
        path = base / relative
        source_hashes[relative] = hashlib.sha256(path.read_bytes()).hexdigest()
        with Image.open(path) as original:
            im = original.copy()
        if limit:
            im.thumbnail((limit, limit), Image.Resampling.LANCZOS)
        return im
    with zipfile.ZipFile(renderer_path) as original:
        for name in original.namelist():
            if not name.startswith('materials/') or name.startswith(('materials/statemaps/', 'materials/browser_remix/')):
                continue
            data = original.read(name)
            if len(data) < 68:
                continue
            # Hand classification belongs to the bootstrap's UV-island masks,
            # not the donor texture conversion (or its source-material report).
            if any(key == 'detailMap' and image.startswith('browser_remix/') and image.endswith('_character')
                   for _, key, image in material_textures(data)):
                continue
            if b'browser_remix/' in data:
                if name not in previous:
                    raise ValueError('Reimport requires the original material or the previous owner pack: '+name)
                if hashlib.sha256(data).hexdigest() != previous_entries.get(name):
                    raise ValueError('Imported material has local changes; preserve them before reimport: '+name)
                data = base64.b64decode(previous[name], validate=True)
            if len(data) < 68:
                continue
            technique = cstring(data, struct.unpack_from('<I', data, 56)[0])
            if not technique.startswith(('phong_replace', 'phong_blend')):
                continue
            textures = material_textures(data)
            color = next((image for _, key, image in textures if key == 'colorMap'), '')
            maps_by_name = {key: image for _, key, image in textures}
            family = weapon_family(name, maps_by_name)
            if (family and
                    all(key in maps_by_name and 'images/'+maps_by_name[key]+'.iwi' in original.namelist()
                        for key in ('colorMap', 'normalMap', 'specularMap'))):
                from remix_weapon_finish import weapon_finish
                if donor_weapon is None:
                    donor_weapon = load('Uniform/test/textures/Enfield_002_001_Enfield_01_BaseColor.a.rtex.dds', 0)
                print('Refining weapon:', family, flush=True)
                finish = weapon_finish(original.read, maps_by_name, donor_weapon, size)
                for channel, image in finish.items():
                    entries[f'images/browser_remix/{family}_{channel}.iwi'] = image
                entries[name] = replace_material(data, family, metallic=('metal' in finish))
                material_report[name] = {'family': family, 'original_color': color,
                    'transfer': ('explicit steel/dielectric UV mask and bounded roughness; original color, normals and markings' if family in ('finish_thompson', 'finish_m1garand', 'finish_mp40') else 'fine donor grain; reduced steel microrelief; original UVs, bevels and markings')}
                source_materials[name] = base64.b64encode(data).decode('ascii')
                continue
            family = IMAGE_FAMILY.get(color)
            # Preserve opacity cutouts and snow coverage even if a name matches.
            if family and not re.search('snow|decal|gfx_|bullethit', name):
                entries[name] = replace_material(data, family)
                material_report[name] = {'family': family, 'original_color': color}
                source_materials[name] = base64.b64encode(data).decode('ascii')
        used = sorted({m['family'] for m in material_report.values() if m['family'] in FAMILIES})
        for family in used:
            print('Converting material:', family, flush=True)
            entries.update(family_images(family, load, size))
        entries['images/browser_remix/neutral_detail.iwi'] = write_iwi([Image.new('RGBA', (4, 4), (128, 128, 128, 255))])
        # Reusable detail and decals retain the original UVs and collision.
        # Oak/pine trunks previously shared a 32px detail texture.
        bark = load('Props/Tree_001/textures/bark_willow_02_diff_4k.a.rtex.dds', 1024).convert('L')
        from PIL import ImageOps
        bark = ImageOps.autocontrast(bark).point([round(92 + value * 72 / 255) for value in range(256)])
        entries['images/tree_bark_01.iwi'] = write_iwi([bark.convert('RGB')], 'DXT1')
        # The distant single-tree card is a whole-tree billboard, so the donor's
        # baked LOD image fits its UVs without importing the enormous leaf mesh.
        entries['images/green_single_tree.iwi'] = write_iwi([
            load('Props/Tree_001/textures/FullTree_albedo.a.rtex.dds', min(size, 2048))])
        for target, relative in {
            'bullethit_wood1': 'Textures/Bullet_Hole_Wood_01/Bullet_Hole_Wood_01_albedo.a.rtex.dds',
            'bullethit_wood2': 'deps/mods/project/Textures/Bullet_Hole_Wood_02/Bullet_Hole_Wood_02_albedo.a.rtex.dds',
        }.items():
            if f'images/{target}.iwi' in original.namelist():
                entries[f'images/{target}.iwi'] = write_iwi([load(relative, 512)])
        from browser_remix_impacts import impact_assets
        def impact_material(name):
            data = original.read(name)
            if b'browser_remix/' in data:
                if hashlib.sha256(data).hexdigest() != previous_entries.get(name):
                    raise ValueError('Impact material changed since conversion: ' + name)
                data = base64.b64decode(previous[name], validate=True)
            return data
        marks, mark_materials, mark_originals = impact_assets(impact_material, load)
        entries.update(marks); material_report.update(mark_materials); source_materials.update(mark_originals)
        from remix_masonry_finish import masonry_assets
        trims, trim_materials, trim_originals = masonry_assets(original.namelist(), impact_material, original.read, load)
        entries.update(trims); material_report.update(trim_materials); source_materials.update(trim_originals)
        from remix_joinery import joinery_assets
        doors, door_materials, door_originals = joinery_assets(original.namelist(), impact_material, original.read, load, size)
        entries.update(doors); material_report.update(door_materials); source_materials.update(door_originals)
        for name, (sky, exposure) in SKIES.items():
            if f'images/{name}.iwi' not in original.namelist():
                continue
            print('Converting sky:', name, flush=True)
            from remix_hdr import read_bc6h
            relative = f'Skies/hdr_Skies/{sky}.dds'
            source_hashes[relative] = hashlib.sha256((base / relative).read_bytes()).hexdigest()
            hdr = read_bc6h(base / relative)
            if name in ('trainyard_ft', 'moscow_ft'):
                # Bright diffuse winter cloud, with lifted lower cloud layers.
                # Exposure alone left the horizon almost black under map fog.
                import numpy as np
                luma = np.sum(hdr * np.array([.2126, .7152, .0722]), axis=-1, keepdims=True)
                hdr = hdr * .4 + luma * .6 + np.array([.025, .028, .032])
            faces = panorama_faces(hdr, min(size, 1024), exposure)
            entries[f'images/{name}.iwi'] = write_iwi(faces, 'RGB8')
        # The donor smoke is an 8x8 atlas. Retile it per original material;
        # preserve the effect's timing, collision, coverage and gameplay.
        smoke_source = load('Textures/Smoke1.a.rtex.dds', 2048).convert('RGBA')
        smoke_materials = []
        for name in original.namelist():
            if not name.startswith('materials/') or name.startswith('materials/statemaps/'):
                continue
            data = original.read(name)
            if len(data) < 68:
                continue
            technique = cstring(data, struct.unpack_from('<I', data, 56)[0])
            if not technique.startswith(('effect', 'particle_cloud')):
                continue
            color = next((image for _, key, image in material_textures(data) if key == 'colorMap'), '')
            if color not in ('smoke', 'whitesmoke', 'dark_smoke_atlas', 'smokpuff_atlased', 'browser_blast_smoke'):
                continue
            rows, cols = data[14], data[15]
            if not rows or not cols or rows*cols > 64:
                continue
            # Only one layout per destination image is safe to override.
            smoke_materials.append((name, color, rows, cols))
        for color in sorted({row[1] for row in smoke_materials}):
            layouts = {(r, c) for _, image, r, c in smoke_materials if image == color}
            if len(layouts) != 1:
                continue
            rows, cols = layouts.pop()
            tile = min(256, 2048 // max(rows, cols))
            atlas = Image.new('RGBA', (cols*tile, rows*tile))
            cell = smoke_source.width // 8
            for i in range(rows*cols):
                frame = round(i * 63 / max(1, rows*cols-1)) if rows*cols > 1 else 27
                x, y = frame % 8 * cell, frame // 8 * cell
                atlas.paste(smoke_source.crop((x, y, x+cell, y+cell)).resize((tile, tile), Image.Resampling.LANCZOS), (i%cols*tile, i//cols*tile))
            entries[f'images/{color}.iwi'] = write_iwi([atlas])
        from remix_smoke_finish import smoke_assets
        smoke, smoke_report, smoke_originals = smoke_assets(original.namelist(), impact_material,
            lambda name: entries[name] if name in entries else original.read(name), load)
        entries.update(smoke); material_report.update(smoke_report); source_materials.update(smoke_originals)
        from remix_fire_finish import fire_assets
        fire, fire_report, fire_originals = fire_assets(original.namelist(), impact_material, original.read, load)
        entries.update(fire); material_report.update(fire_report); source_materials.update(fire_originals)
        maps = {}
        for name in original.namelist():
            if not name.endswith('.d3dbsp'):
                continue
            data = original.read(name)
            length, offset = struct.unpack_from('<II', data, 8)
            names = [cstring(data, i) for i in range(offset, offset+length, 72)]
            maps[Path(name).stem] = [n for n in names if 'materials/'+n in material_report]
    report = {'schema': 1, 'source': 'Call_of_Duty_2_Carentan / Cod2_rtx_mp_carentan 2.0',
              'color_size': size, 'weapon_finish_revision': 5, 'masonry_finish_revision': 1, 'joinery_revision': 1, 'smoke_finish_revision': 1, 'fire_finish_revision': 1, 'road_bed_revision': 1, 'materials': material_report, 'maps': maps, 'source_materials': source_materials,
              'skies': SKIES, 'smoke_materials': smoke_materials, 'sources': source_hashes,
              'limitations': ['Raster lighting; no RTX path tracing or DLSS.',
                  'Level meshes need per-map fitting; separate owner model packs adapt portable weapons and props.',
                  'BC6H skies retain floating-point HDR through tone mapping; the browser receives LDR cubemaps.']}
    report['entries'] = {name: hashlib.sha256(data).hexdigest() for name, data in entries.items()}
    output = Path(output)
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_suffix('.tmp')
    with zipfile.ZipFile(temporary, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
        for name, data in sorted(entries.items()):
            info = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, data)
        archive.writestr('remix-report.json', json.dumps(report, indent=2)+'\n')
    temporary.replace(output)
    output.with_suffix('.json').write_text(json.dumps(report, indent=2)+'\n')
    return report


def apply_owner_pack(renderer, path):
    """Merge into the normal private archive so existing delivery/cache works."""
    path = Path(path)
    if not path.is_file():
        return None
    with zipfile.ZipFile(path) as archive:
        report = json.loads(archive.read('remix-report.json'))
        if report.get('schema') != 1:
            raise ValueError('Unsupported Carentan pack version')
        updates = {}
        for name, digest in report['entries'].items():
            if not name.startswith(('images/', 'materials/')) or '..' in Path(name).parts:
                raise ValueError('Nonvisual entry in Carentan pack: '+name)
            data = archive.read(name)
            if hashlib.sha256(data).hexdigest() != digest:
                raise ValueError('Corrupt Carentan pack entry: '+name)
            if name.startswith('materials/') and name not in renderer:
                raise ValueError('Carentan pack targets an unavailable material: '+name)
            if name.startswith('materials/') and renderer[name] not in (
                    data, base64.b64decode(report['source_materials'][name], validate=True)):
                raise ValueError('Material changed since Carentan conversion; reimport the pack: '+name)
            updates[name] = data
        renderer.update(updates)
        return report
