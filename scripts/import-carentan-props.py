#!/usr/bin/env python3
"""Fit detailed owner props to the existing map instances, keeping collision."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import zipfile
import numpy as np
from PIL import Image
from pxr import Usd, UsdGeom, UsdShade
import browser_attachment_models as A
from browser_remix_assets import write_iwi, normal_image, specular_image, metalness_image, replace_material
from remix_static_models import PACK, PREFIX, read_model, replace_near_model

# Source object, original model(s), optional quarter-turn around world up.
ASSETS = {
    'barrel': ('Props/wine_barrel_01/wine_barrel_01_2k.usd', ['prop_wooden_barrel1'], 0),
    'crate': ('Props/Crate_001/SM_Crate_01.usda', ['crate01', 'crate02'], 0),
    'chair': ('Props/Wooden_chair_01/Wooden_Chair_01.usda', ['furniture_plainchair'], 0),
    # The bent donor is a complete curved barricade, not a single bag. Keep
    # it on the matching footprint; short/end/snow sections need their own
    # geometry or finish rather than squeezing/recoloring this whole wall.
    'sandbag_curve': ('Props/sandbags/Sandbag_01_Bent_LP.usd', ['military_sandbag_curvedsection'], 0),
    'foliage_shrub_a': ('Props/Bramble_01/Bramble_01.usd', ['caen_bush_full_01'], 0),
    'foliage_shrub_b': ('Props/Bramble_01/Bramble_02.usd', ['caen_spikeybush_01'], 0),
    'foliage_hedge': ('Props/Bramble_01/Bramble_02.usd',
                      ['caen_hedge_wall_1', 'caen_hedge_wall_2',
                       'caen_hedge_wall_3', 'caen_hedge_wall_group_1'], 0),
    'foliage_grass': ('Props/grass_medium_01/grass_medium_01_la.usd', ['caen_grasstuft_5'], 0),
    # The full source has finer individual leaves and exposed branch geometry.
    # Reduce it ourselves; the donor's LOD uses larger foliage silhouettes.
    'foliage_tree_oak': ('Props/Tree_001/Tree_001.usda',
                         ['tree_grey_oak_med_a', 'tree_grey_oak_lg_a', 'tree_grey_oak_xl_a'], 0),
}


def usd_meshes(path):
    stage = Usd.Stage.Open(str(path))
    cache = UsdGeom.XformCache()
    up = str(UsdGeom.GetStageUpAxis(stage))
    axis = np.eye(3) if up == 'Z' else np.array([[1, 0, 0], [0, 0, 1], [0, -1, 0]])
    result = []
    for prim in stage.Traverse():
        if not prim.IsA(UsdGeom.Mesh) or UsdGeom.Imageable(prim).ComputeVisibility() == 'invisible':
            continue
        mesh = UsdGeom.Mesh(prim)
        matrix = np.asarray(cache.GetLocalToWorldTransform(prim))
        points = (np.asarray(mesh.GetPointsAttr().Get()) @ matrix[:3, :3] + matrix[3, :3]) @ axis
        ids = np.asarray(mesh.GetFaceVertexIndicesAttr().Get(), dtype=np.int64)
        counts = np.asarray(mesh.GetFaceVertexCountsAttr().Get(), dtype=np.int64)
        normals = np.asarray(mesh.GetNormalsAttr().Get()) @ np.linalg.inv(matrix[:3, :3]).T @ axis
        interpolation = str(mesh.GetNormalsInterpolation())
        if interpolation in ('vertex', 'varying'):
            normals = normals[ids]
        elif interpolation == 'uniform':
            normals = np.repeat(normals, counts, axis=0)
        elif interpolation == 'constant':
            normals = np.repeat(normals, len(ids), axis=0)
        uv_attr = UsdGeom.PrimvarsAPI(prim).GetPrimvar('st')
        uv = np.asarray(uv_attr.ComputeFlattened()).copy()
        if str(uv_attr.GetInterpolation()) in ('vertex', 'varying'):
            uv = uv[ids]
        if len(normals) != len(ids) or len(uv) != len(ids):
            raise ValueError('Unsupported mesh interpolation: ' + str(prim.GetPath()))
        uv[:, 1] = 1 - uv[:, 1]
        binding = UsdShade.MaterialBindingAPI(prim).ComputeBoundMaterial()[0]
        face_material = [binding] * len(counts)
        for subset in UsdGeom.Subset.GetAllGeomSubsets(mesh):
            material = UsdShade.MaterialBindingAPI(subset.GetPrim()).ComputeBoundMaterial()[0]
            for index in subset.GetIndicesAttr().Get():
                face_material[index] = material
        groups, start = {}, 0
        for count, material in zip(counts, face_material):
            if not material:
                raise ValueError('Mesh has unbound faces')
            key = str(material.GetPath())
            group = groups.setdefault(key, {'corners': [], 'material': material})
            for j in range(1, count - 1):
                group['corners'].append((start, start + j, start + j + 1))
            start += count
        for group in groups.values():
            triangles = np.asarray(group['corners'])
            shader = UsdShade.Shader(group['material'].GetPrim().GetChild('Shader'))
            textures = {}
            for key in ('diffuse_texture', 'normalmap_texture', 'reflectionroughness_texture', 'metallic_texture'):
                value = shader.GetInput(key).Get() if shader else None
                if value and value.path:
                    textures[key] = Path(value.resolvedPath or path.parent / value.path).resolve()
            result.append({'points': points[ids], 'normals': normals, 'uv': uv,
                           'triangles': triangles, 'textures': textures, 'mesh': str(prim.GetPath())})
    if not result:
        raise ValueError('No visible meshes in ' + str(path))
    return result


def merge_texture_groups(groups):
    """Branches sharing a material become one simplification/draw stream."""
    merged = {}
    for group in groups:
        key = tuple(sorted((k, str(v)) for k, v in group['textures'].items()))
        dest = merged.setdefault(key, {'textures': group['textures'], 'points': [],
                                      'normals': [], 'uv': [], 'triangles': [], 'count': 0})
        dest['triangles'].append(group['triangles'] + dest['count'])
        for attr in ('points', 'normals', 'uv'):
            dest[attr].append(group[attr])
        dest['count'] += len(group['points'])
    return [{**group, **{attr: np.concatenate(group[attr])
                        for attr in ('points', 'normals', 'uv', 'triangles')}}
            for group in merged.values()]


def root_bounds(points):
    """Locate the grounded trunk, independently of an asymmetric crown."""
    lo, hi = points.min(0), points.max(0)
    root = points[points[:, 2] <= lo[2] + (hi[2] - lo[2]) * 0.06]
    return np.array([root.min(0), root.max(0)])


def hedge_groups(groups, bounds):
    """Overlapping grounded brambles, rather than stretching one across a wall.

    Each cell retains its own stems and foliage. Alternating orientation and
    slightly varied crowns break the repeated outline without moving any
    geometry outside the original hedge's footprint or changing collision.
    """
    points = np.concatenate([g['points'] for g in groups])
    lo, hi = points.min(0), points.max(0)
    extent = bounds[1] - bounds[0]
    count = max(2, int(np.ceil(extent[1] / (extent[0] * 0.85))))
    cell_length = extent[1] / (1 + 0.72 * (count - 1))
    result = []
    for cell in range(count):
        height = 1.0 if cell == 0 else 0.88 + 0.12 * ((cell * 5) % 7) / 6
        width = 1.0 if cell == count - 1 else 0.90 + 0.10 * ((cell * 3) % 5) / 4
        size = np.array([extent[0] * width, cell_length, extent[2] * height])
        origin = bounds[0] + [extent[0] * (1 - width) * 0.5,
                              cell * cell_length * 0.72, 0]
        for group in groups:
            p = (group['points'] - lo) / (hi - lo)
            n = group['normals'].copy()
            if cell % 2:
                p[:, :2] = 1 - p[:, :2]
                n[:, :2] *= -1
            p = p * size + origin
            n *= (hi - lo) / size
            n /= np.maximum(np.linalg.norm(n, axis=1, keepdims=True), 1e-10)
            result.append({**group, 'points': p, 'normals': n})
    return merge_texture_groups(result), count


def surface(group, lo, hi, bounds, triangle_budget, max_error, anchors=None, coarse_uv=None):
    fit = (bounds[1] - bounds[0]) / np.maximum(hi - lo, 1e-8)
    points = (group['points'] - lo) * fit + bounds[0]
    derivative = np.broadcast_to(fit, points.shape).copy()
    if anchors is not None:
        # Four knots preserve both the center and width of the grounded trunk,
        # then fit the outer crown. A plain bounding-box fit moves/shrinks the
        # trunk relative to the unchanged collision when the crown is lopsided.
        source, target = anchors
        for axis in (0, 1):
            src = np.array([lo[axis], source[0, axis], source[1, axis], hi[axis]])
            dst = np.array([bounds[0][axis], target[0, axis], target[1, axis], bounds[1][axis]])
            if np.any(np.diff(src) <= 0) or np.any(np.diff(dst) <= 0):
                raise ValueError('Tree root must be inside its crown bounds')
            positions = group['points'][:, axis]
            points[:, axis] = np.interp(positions, src, dst)
            interval = np.clip(np.searchsorted(src, positions, side='right') - 1, 0, 2)
            derivative[:, axis] = (np.diff(dst) / np.diff(src))[interval]
    normals = group['normals'] / derivative
    normals /= np.maximum(np.linalg.norm(normals, axis=1, keepdims=True), 1e-10)
    uv = group['uv']
    # Welding preserves every normal/UV seam, including both sides of foliage.
    keys = np.round(np.concatenate((points, normals, uv), axis=1), 6)
    _, first, inverse = np.unique(keys, axis=0, return_index=True, return_inverse=True)
    p, n, uv = points[first], normals[first], uv[first]
    tri = inverse[group['triangles']]
    if len(tri) > triangle_budget:
        vertices = np.concatenate((p, n, uv), axis=1).astype('<f4')
        payload = (struct.pack('<IIIf', len(p), tri.size, triangle_budget * 3, max_error)
                   + vertices.tobytes() + tri.astype('<u4').tobytes())
        command = ['node', str(Path(__file__).with_name('remix-simplify.mjs'))]
        if coarse_uv is not None:
            command.extend(['--coarse-uv', str(coarse_uv)])
        result = subprocess.run(command,
                                input=payload, stdout=subprocess.PIPE, check=True).stdout
        count, error = struct.unpack_from('<If', result)
        reduced = np.frombuffer(result, '<u4', offset=8).copy()
        if len(reduced) != count or count % 3 or not count or reduced.max() >= len(p) or error > max_error + 0.00001:
            raise ValueError('Invalid simplified geometry')
        print(f'  retained {count // 3}/{len(tri)} triangles, relative error {error:.5f}', flush=True)
        tri = reduced.reshape(-1, 3)
    e1, e2 = p[tri[:, 1]] - p[tri[:, 0]], p[tri[:, 2]] - p[tri[:, 0]]
    u1, u2 = uv[tri[:, 1]] - uv[tri[:, 0]], uv[tri[:, 2]] - uv[tri[:, 0]]
    det = u1[:, 0] * u2[:, 1] - u1[:, 1] * u2[:, 0]
    inv = np.divide(1, det, out=np.zeros_like(det), where=np.abs(det) > 1e-12)
    tv = (e1 * u2[:, 1, None] - e2 * u1[:, 1, None]) * inv[:, None]
    bv = (e2 * u1[:, 0, None] - e1 * u2[:, 0, None]) * inv[:, None]
    t, b = np.zeros_like(p), np.zeros_like(p)
    for corner in range(3):
        np.add.at(t, tri[:, corner], tv)
        np.add.at(b, tri[:, corner], bv)
    t -= n * np.sum(n * t, axis=1, keepdims=True)
    bad = np.linalg.norm(t, axis=1) < 1e-8
    t[bad] = np.cross(n[bad], [0, 0, 1])
    t[np.linalg.norm(t, axis=1) < 1e-8] = [1, 0, 0]
    t /= np.maximum(np.linalg.norm(t, axis=1, keepdims=True), 1e-10)
    cross = np.cross(n, t)
    b = cross * np.where(np.sum(cross * b, axis=1, keepdims=True) < 0, -1, 1)
    reverse = np.sum(np.cross(e1, e2) * n[tri].sum(axis=1), axis=1) > 0
    tri[reverse] = tri[reverse][:, [0, 2, 1]]
    # Rigid meshes use static GPU buffers. Up to 8000 triangles means at most
    # 24000 unique vertices even with every corner split, below signed limits,
    # without flooding the scene's surface table with tiny draw batches.
    for start in range(0, len(tri), 8000):
        used, indices = np.unique(tri[start:start+8000].reshape(-1), return_inverse=True)
        vertices = []
        for i in used:
            raw = struct.pack('<3f4B2f3f3f3f', *n[i], 255, 255, 255, 255, *uv[i], *b[i], *t[i], *p[i])
            vertices.append({'raw': raw, 'bone': 0, 'weights': 0, 'pos': p[i]})
        yield {'tile': 0, 'marker': 0, 'vertices': vertices, 'triangles': indices.tolist()}


def main():
    root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('--only', choices=(*ASSETS, 'truck'), help='Update one family in an existing pack')
    parser.add_argument('--materials-only', action='store_true', help='Refresh one family without rebuilding geometry')
    args = parser.parse_args()
    if args.only == 'truck':
        if args.materials_only:
            parser.error('The truck importer currently rebuilds its complete LOD chain')
        subprocess.run([sys.executable, str(Path(__file__).with_name('import-carentan-truck.py')), str(args.source)], check=True)
        return
    if args.materials_only and (not args.only or not (root / 'data/main' / PACK).is_file()):
        parser.error('--materials-only requires --only and an existing pack')
    base = args.source / 'rtx-remix/mods/gameReadyAssets'
    with zipfile.ZipFile(root / 'data/browser/main/cod2_browser_renderer.iwd') as renderer:
        available = set(renderer.namelist())
    archives = [zipfile.ZipFile(p) for p in sorted((root / 'data/main').glob('*.iwd'))]
    index = {n.lower(): (z, n) for z in archives for n in z.namelist()}
    def read(name):
        z, n = index[name.lower()]
        return z.read(n)
    entries, originals, sources, models = {}, {}, {}, {}
    material_templates = {}
    if args.only and (root / 'data/main' / PACK).is_file():
        from remix_static_models import apply_prop_pack
        with zipfile.ZipFile(root / 'data/main' / PACK) as previous:
            original_names = json.loads(previous.read('props-report.json'))['originals']
        existing = {name: read(name) for name in original_names}
        report = apply_prop_pack(existing, root / 'data/main' / PACK)
        entries = {name: existing[name] for name in report['entries']}
        originals.update(report['originals']); sources.update(report['sources']); models.update(report['models'])
        material_templates.update(report.get('materialTemplates', {}))
    def record(path):
        sources[str(path.relative_to(args.source))] = hashlib.sha256(path.read_bytes()).hexdigest()
    for family, (relative, targets, turn) in ASSETS.items():
        if args.only and family != args.only:
            continue
        source = base / relative
        record(source)
        groups = usd_meshes(source)
        is_tree = family == 'foliage_tree_oak'
        is_hedge = family == 'foliage_hedge'
        if is_tree or is_hedge:
            groups = merge_texture_groups(groups)
        # The table records any orientation adjustment explicitly.
        if turn:
            angle = turn * np.pi / 2
            rotation = np.array([[np.cos(angle), -np.sin(angle), 0], [np.sin(angle), np.cos(angle), 0], [0, 0, 1]])
            for group in groups:
                group['points'] = group['points'] @ rotation.T
                group['normals'] = group['normals'] @ rotation.T
        points = np.concatenate([g['points'] for g in groups])
        lo, hi = points.min(0), points.max(0)
        for target in targets:
            name = 'xmodel/' + target
            if name not in available:
                continue
            original = read(name)
            model = read_model(original)
            original_lod = model['lods'][0][1]
            parts = read('xmodelparts/' + original_lod)
            if A.read_skeleton(parts)[0] != ['']:
                raise ValueError('Expected a rigid one-bone prop: ' + target)
            old_surfaces = A.read_surfaces(read('xmodelsurfs/' + original_lod))
            old_points = np.asarray([v['pos'] for s in old_surfaces for v in s['vertices']])
            bounds = old_points.min(0), old_points.max(0)
            anchors = (root_bounds(points), root_bounds(old_points)) if is_tree else None
            model_groups, clusters = hedge_groups(groups, bounds) if is_hedge else (groups, 1)
            model_lo, model_hi = bounds if is_hedge else (lo, hi)
            surfaces, materials = [], []
            for i, group in enumerate(model_groups):
                texture_family = 'prop_' + family + '_' + str(i)
                material_name = 'browser_remix/' + texture_family
                material_path = 'materials/' + material_name
                if material_path not in entries or (args.only or is_tree) and target == targets[0]:
                    authored_metal = 'metallic_texture' in group['textures']
                    # Bare timber and woven sandbag cloth use a zero metal mask
                    # selects the dielectric finish without inventing metal from
                    # pale wood or the old grayscale specular intensity.
                    authored_finish = authored_metal or family in ('crate', 'sandbag_curve')
                    for key, suffix in [('diffuse_texture', 'c'), ('normalmap_texture', 'n'), ('reflectionroughness_texture', 's')]:
                        path = group['textures'][key]
                        record(path)
                        im = Image.open(path).convert('RGBA')
                        limit = (4096 if family == 'crate' and suffix == 'c' else
                                 2048 if suffix == 'c' or authored_finish and suffix == 'n' else 1024)
                        im.thumbnail((limit,) * 2, Image.Resampling.LANCZOS)
                        if suffix == 'n':
                            im = normal_image(im)
                        elif suffix == 's':
                            im = specular_image(im)
                            # Keep the legacy RGB fallback. The authored finish
                            # derives Fresnel from base color and the metal mask.
                            if authored_metal:
                                path = group['textures']['metallic_texture']; record(path)
                                metal = Image.open(path).convert('L').resize(im.size, Image.Resampling.LANCZOS)
                                pixels = np.asarray(im).copy()
                                pixels[:, :, :3] = np.rint(28 + np.asarray(metal)[:, :, None] / 255 * 115).astype(np.uint8)
                                im = Image.fromarray(pixels)
                        entries[f'images/{material_name}_{suffix}.iwi'] = write_iwi([im], 'DXT5')
                    if authored_metal:
                        path = group['textures']['metallic_texture']; record(path)
                        metal = Image.open(path).convert('L')
                        metal.thumbnail((2048, 2048), Image.Resampling.LANCZOS)
                        entries[f'images/{material_name}_metal.iwi'] = write_iwi([metalness_image(metal)], 'DXT5')
                    elif authored_finish:
                        entries[f'images/{material_name}_metal.iwi'] = write_iwi(
                            [metalness_image(Image.new('L', (4, 4), 0))], 'DXT5')
                    template_name = model['materials'][0][0]
                    if is_tree:
                        leaf = 'leaf' in group['textures']['diffuse_texture'].name.lower()
                        template_name = next(n for n in model['materials'][0] if ('canopy' in n) == leaf)
                    elif is_hedge and 'bark' in group['textures']['diffuse_texture'].name.lower():
                        trunk = read_model(read('xmodel/tree_grey_oak_med_a'))
                        template_name = next(n for n in trunk['materials'][0] if 'canopy' not in n)
                    template = read('materials/' + template_name)
                    material = bytearray(replace_material(template, texture_family, metallic=authored_finish))
                    struct.pack_into('<I', material, 0, len(material))
                    material.extend(material_name.encode() + b'\0')
                    entries[material_path] = bytes(material)
                    material_templates[material_name] = template_name
                if args.materials_only:
                    continue
                chunks = list(surface(group, model_lo, model_hi, bounds,
                                      24000 if is_hedge else 18000 if is_tree else 12000 if 'shrub' in family else 6500 if family == 'sandbag_curve' else 3500,
                                      0.012 if is_hedge else 0.0005 if family == 'barrel' else 0.005, anchors))
                surfaces.extend(chunks)
                materials.extend([material_name] * len(chunks))
            if args.materials_only:
                print(target, 'materials refreshed; existing geometry retained', flush=True)
                continue
            lod = PREFIX + target + '0'
            entries['xmodelsurfs/' + lod] = A.write_surfaces(surfaces)
            entries['xmodelparts/' + lod] = parts
            entries[name] = replace_near_model(original, lod, materials,
                                              800 if is_tree else 600 if is_hedge else 360 if 'foliage' in family else 480)
            originals[name] = hashlib.sha256(original).hexdigest()
            models[target] = {'source': relative, 'vertices': sum(len(s['vertices']) for s in surfaces),
                              'triangles': sum(len(s['triangles']) // 3 for s in surfaces), 'surfaces': len(surfaces),
                              'bounds': [b.tolist() for b in bounds]}
            if is_tree:
                models[target]['rootBounds'] = [a.tolist() for a in anchors]
            if is_hedge:
                models[target]['clusters'] = clusters
            print(target, models[target]['vertices'], 'vertices,', models[target]['triangles'], 'triangles', flush=True)
    report = {'schema': 1, 'sources': sources, 'originals': originals, 'models': models,
              'materialTemplates': material_templates,
              'entries': {n: hashlib.sha256(d).hexdigest() for n, d in entries.items()}}
    target = root / 'data/main' / PACK
    tmp = target.with_suffix('.tmp')
    with zipfile.ZipFile(tmp, 'w') as z:
        for name, data in sorted(entries.items()):
            info = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0)); info.compress_type = zipfile.ZIP_DEFLATED
            z.writestr(info, data)
        z.writestr('props-report.json', json.dumps(report, indent=2) + '\n')
    tmp.replace(target)
    target.with_suffix('.json').write_text(json.dumps(report, indent=2) + '\n')
    for z in archives:
        z.close()
    if not args.only:
        subprocess.run([sys.executable, str(Path(__file__).with_name('import-carentan-truck.py')), str(args.source)], check=True)


if __name__ == '__main__':
    main()
