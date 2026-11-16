#!/usr/bin/env python3
"""Import the owner's Austin truck, including its complete visual LOD chain."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import zipfile

import numpy as np
from PIL import Image
from pxr import Usd, UsdShade
import browser_attachment_models as A
from browser_remix_assets import normal_image, specular_image, metalness_image, replace_material, write_iwi
from remix_static_models import PACK, PREFIX, apply_prop_pack, read_model, replace_all_model_lods

TARGET = 'vehicle_opel_blitz_woodland_static'
SOURCE = 'rtx-remix/mods/gameReadyAssets/Props/Austin_30cwt_truck_001/Austin_30cwt_truck03_GR.usda'
# Budgets are per material (trailer, chassis, wheels, cabin, glass, interior).
# The geometric/attribute error limit may retain more triangles than requested.
BUDGETS = [(5000, 3500, 8000, 7000, 256, 4000),
           (1200, 700, 2000, 1800, 100, 700),
           (350, 200, 650, 500, 40, 200),
           (100, 80, 180, 160, 24, 60)]
ERRORS = (.003, .012, .025, .04)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    spec = importlib.util.spec_from_file_location('owner_props', Path(__file__).with_name('import-carentan-props.py'))
    props = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(props)
    source = args.source / SOURCE
    raw_groups = props.usd_meshes(source)
    # The reference has its rear board hanging down. Close that board so the
    # existing map's solid rear cover is still represented by visible wood.
    # Reflect Y/Z about a transverse hinge; align its top with the sideboard.
    door = next(g for g in raw_groups if g['mesh'].endswith('/Trailer_Door/Trailer_Door'))
    side = next(g for g in raw_groups if g['mesh'].endswith('/Trailer_mirror_me/Trailer_mirror_me'))
    hinge_y = .5 * (door['points'][:, 1].min() + door['points'][:, 1].max())
    hinge_z = .5 * (side['points'][:, 2].max() + door['points'][:, 2].min())
    door['points'][:, 1] = 2 * hinge_y - door['points'][:, 1]
    door['points'][:, 2] = 2 * hinge_z - door['points'][:, 2]
    door['normals'][:, 1:] *= -1
    groups = props.merge_texture_groups(raw_groups)
    expected = ['Truck_Trailer', 'Truck_Underside', 'Truck_Wheels', 'Truck_Cabin', None, 'Car_01_Inside_Inside']
    stems = [g['textures']['diffuse_texture'].name.removesuffix('_BaseColor.a.rtex.dds')
             if g['textures'] else None for g in groups]
    if stems != expected:
        raise ValueError('Unexpected donor material layout; review the truck before importing')
    # This exact untextured donor material is thin glass, not a missing body map.
    stage = Usd.Stage.Open(str(source))
    glass = UsdShade.Shader(stage.GetPrimAtPath('/ReferenceTarget/XForms/World/Looks/glass/Shader'))
    if not glass or not glass.GetInput('thin_walled').Get() or glass.GetInput('ior_constant').Get() != 2:
        raise ValueError('Unexpected donor glass material')
    # Donor nose faces -Y; retail vehicle nose faces +X.
    rotation = np.array([[0, -1, 0], [1, 0, 0], [0, 0, 1]])
    for g in groups:
        g['points'] = g['points'] @ rotation.T
        g['normals'] = g['normals'] @ rotation.T
    points = np.concatenate([g['points'] for g in groups])
    lo, hi = points.min(0), points.max(0)
    archives = [zipfile.ZipFile(p) for p in sorted((root / 'data/main').glob('*.iwd'))]
    try:
        index = {n.lower(): (z, n) for z in archives for n in z.namelist()}
        def read(name):
            z, n = index[name.lower()]
            return z.read(n)
        pack = root / 'data/main' / PACK
        with zipfile.ZipFile(pack) as z:
            previous = json.loads(z.read('props-report.json'))
        assets = {n: read(n) for n in previous['originals']}
        report = apply_prop_pack(assets, pack)
        entries = {n: assets[n] for n in report['entries']}
        name = 'xmodel/' + TARGET
        original = read(name)
        model = read_model(original)
        old = A.read_surfaces(read('xmodelsurfs/' + model['lods'][0][1]))
        p = np.asarray([v['pos'] for s in old for v in s['vertices']])
        bounds = p.min(0), p.max(0)
        def record(path):
            report['sources'][str(path.relative_to(args.source))] = hashlib.sha256(path.read_bytes()).hexdigest()
        record(source)
        materials = []
        for i, group in enumerate(groups):
            family = 'prop_truck_' + str(i)
            material = 'browser_remix/' + family
            is_glass = not group['textures']
            for key, suffix in [('diffuse_texture', 'c'), ('normalmap_texture', 'n'), ('reflectionroughness_texture', 's')]:
                if is_glass:
                    # The legacy renderer cannot transmit/refract the donor's
                    # MDL glass. Bridge it with the retail alpha-blend technique,
                    # faint neutral tint and F0 ~= 0.11 for the donor's IOR 2.
                    im = {'c': Image.new('RGBA', (4, 4), (150, 160, 165, 40)),
                          'n': Image.new('RGBA', (4, 4), (128, 128, 255, 255)),
                          's': Image.new('L', (4, 4), 31)}[suffix]
                else:
                    path = group['textures'][key]
                    record(path)
                    im = Image.open(path).convert('RGBA')
                    limit = 1024 if suffix == 's' else 2048
                    im.thumbnail((limit, limit), Image.Resampling.LANCZOS)
                if suffix == 'n':
                    im = normal_image(im)
                elif suffix == 's':
                    im = specular_image(im)
                entries[f'images/{material}_{suffix}.iwi'] = write_iwi([im], 'DXT5')
            if not is_glass:
                path = group['textures']['metallic_texture']
                record(path)
                im = Image.open(path).convert('L')
                im.thumbnail((2048, 2048), Image.Resampling.LANCZOS)
                entries[f'images/{material}_metal.iwi'] = write_iwi([metalness_image(im)], 'DXT5')
            template_name = 'mtl_opel_blitz_glass_woodland' if is_glass else 'mtl_opel_blitz_woodland'
            data = bytearray(replace_material(read('materials/' + template_name), family, metallic=not is_glass))
            struct.pack_into('<I', data, 0, len(data))
            data.extend(material.encode() + b'\0')
            entries['materials/' + material] = bytes(data)
            report['materialTemplates'][material] = template_name
            materials.append(material)
        names, rows, levels = [], [], []
        for level, (budgets, error) in enumerate(zip(BUDGETS, ERRORS)):
            lod = PREFIX + TARGET + str(level)
            # Retail static vehicles share the near skeleton; their farther
            # geometry names need not have separate xmodelparts resources.
            parts_name = 'xmodelparts/' + model['lods'][level][1]
            parts = read(parts_name if parts_name.lower() in index else 'xmodelparts/' + model['lods'][0][1])
            if A.read_skeleton(parts)[0] != ['']:
                raise ValueError('Truck level is not a rigid one-bone model')
            surfaces, row = [], []
            for i, group in enumerate(groups):
                # The thin separate cargo boards define the silhouette and
                # cover height. Keep their seams even at the farthest level.
                coarse_uv = None if i == 0 else (None, .35, .10, .025)[level]
                chunks = list(props.surface(group, lo, hi, bounds, budgets[i], error,
                                            coarse_uv=coarse_uv))
                surfaces.extend(chunks)
                row.extend([materials[i]] * len(chunks))
            entries['xmodelsurfs/' + lod] = A.write_surfaces(surfaces)
            entries['xmodelparts/' + lod] = parts
            names.append(lod)
            rows.append(row)
            levels.append({'vertices': sum(len(s['vertices']) for s in surfaces),
                           'triangles': sum(len(s['triangles']) // 3 for s in surfaces),
                           'surfaces': len(surfaces)})
            print('LOD', level, levels[-1], flush=True)
        entries[name] = replace_all_model_lods(original, names, rows)
        report['originals'][name] = hashlib.sha256(original).hexdigest()
        report['models'][TARGET] = {'source': SOURCE, 'completeLods': True,
            'bounds': [b.tolist() for b in bounds], **levels[0], 'levels': levels,
            'glassApproximation': 'retail alpha blend, constant neutral tint, donor IOR 2 F0'}
        report['entries'] = {n: hashlib.sha256(d).hexdigest() for n, d in entries.items()}
        tmp = pack.with_suffix('.tmp')
        with zipfile.ZipFile(tmp, 'w') as z:
            for n, data in sorted(entries.items()):
                info = zipfile.ZipInfo(n, (1980, 1, 1, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                z.writestr(info, data)
            z.writestr('props-report.json', json.dumps(report, indent=2) + '\n')
        # Validate the complete candidate before replacing the installed pack.
        apply_prop_pack({n: read(n) for n in report['originals']}, tmp)
        tmp.replace(pack)
        pack.with_suffix('.json').write_text(json.dumps(report, indent=2) + '\n')
    finally:
        for z in archives:
            z.close()


if __name__ == '__main__':
    main()
