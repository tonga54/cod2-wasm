"""Adapt the owner's evolving smoke/fire billows to the shared blast atlas.

The donor supplies the spatial detail and frame evolution. A cooling emission
curve makes those billows fire; this is an artistic transfer, not a temperature
measurement or a new fluid simulation. Only the private converter needs Pillow
and numpy. Ordinary builds consume the converted, hashed owner pack.
"""
import base64
import struct

SOURCE = 'Textures/Smokefire.a.rtex.dds'
MATERIAL = 'materials/browser_blast_fireball'
IMAGE = 'browser_remix/blast_fire_billows'


def fire_atlas(source):
    import numpy as np
    from PIL import Image
    if source.size != (2048, 2048):
        raise ValueError('Expected the donor 8x8, 256px smoke/fire sequence')
    atlas = Image.new('RGB', (2048, 2048))
    source = source.convert('RGBA')
    for frame in range(64):
        x, y = frame % 8 * 256, frame // 8 * 256
        pixels = np.asarray(source.crop((x, y, x + 256, y + 256)), dtype=np.float32) / 255
        t = frame / 63
        shade = np.sum(pixels[:, :, :3] * [.299, .587, .114], axis=2) / .19
        heat = (.12 + .97 * shade) * (1 - .72 * t)
        rgb = np.stack((np.minimum(1, heat * 1.9), np.clip((heat - .12) * 1.45, 0, 1),
                        np.clip((heat - .5) * 1.6, 0, 1) * .85), axis=2)
        # Bake coverage into emitted RGB; black is transparent under the
        # production controlled-additive blend. Never leave a glowing square.
        rgb *= pixels[:, :, 3:4] * (1 - t * t)
        tile = Image.fromarray(np.round(rgb * 255).astype('uint8'))
        # The donor fills its cells; the old blast occupied only 1/4–1/2 of
        # each cell. Retain a comparable expanding footprint and a black gutter.
        extent = round(256 * (.30 + .26 * (1 - (1 - t) ** 3)))
        tile = tile.resize((extent, extent), Image.Resampling.LANCZOS)
        # Reconstruction ringing must not introduce green/cyan fire fringes.
        filtered = np.asarray(tile).copy()
        filtered[:, :, 1] = np.minimum(filtered[:, :, 1], filtered[:, :, 0])
        filtered[:, :, 2] = np.minimum(filtered[:, :, 2], filtered[:, :, 1])
        tile = Image.fromarray(filtered)
        inset = (256 - extent) // 2
        atlas.paste(tile, (x + inset, y + inset))
    return atlas


def fire_assets(names, read_material, read_image, load):
    from browser_remix_assets import material_textures, write_iwi
    if MATERIAL not in names:
        return {}, {}, {}
    before = read_material(MATERIAL)
    maps = material_textures(before)
    if len(maps) != 1 or maps[0][1:] != ('colorMap', 'browser_blast_fireball') or before[14:16] != b'\x08\x08':
        raise ValueError('Unexpected shared blast atlas/material layout')
    atlas = fire_atlas(load(SOURCE, 2048))
    after = bytearray(before)
    struct.pack_into('<HH', after, 28, *atlas.size)
    struct.pack_into('<I', after, maps[0][0] + 8, len(after))
    after.extend(IMAGE.encode('ascii') + b'\0')
    entries = {MATERIAL: bytes(after), 'images/' + IMAGE + '.iwi': write_iwi([atlas], 'RGB8')}
    metadata = {MATERIAL: {'family': 'blast_fire', 'source': SOURCE, 'frames': 64, 'cell_size': 256,
        'transfer': 'donor billows with cooling emission and expanding footprint; original effect timing and blend'}}
    return entries, metadata, {MATERIAL: base64.b64encode(before).decode('ascii')}
