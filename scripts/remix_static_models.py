"""Portable owner static-model pack with original collision and explicit LOD scope."""
import hashlib
import json
from pathlib import Path
import struct
import zipfile

PACK = 'owner_carentan_props.zip'
PREFIX = 'browser_remix/prop_'


def read_model(data):
    if struct.unpack_from('<h', data)[0] != 20:
        raise ValueError('Unsupported model version')
    p, lods = 27, []
    for _ in range(4):
        end = data.index(0, p + 4)
        lods.append((struct.unpack_from('<f', data, p)[0], data[p+4:end].decode('ascii')))
        p = end + 1
    collision_start = p
    collision_count = struct.unpack_from('<i', data, p + 4)[0]
    p += 8
    if not 0 <= collision_count <= 1024:
        raise ValueError('Invalid collision count')
    for _ in range(collision_count):
        triangles = struct.unpack_from('<i', data, p)[0]
        if not 0 <= triangles <= 1000000:
            raise ValueError('Invalid collision triangle count')
        p += 4 + triangles * 48 + 36
    collision = data[collision_start:p]
    materials = []
    for _, name in lods:
        if not name:
            continue
        count = struct.unpack_from('<H', data, p)[0]
        p += 2
        row = []
        for _ in range(count):
            end = data.index(0, p)
            row.append(data[p:end].decode('ascii'))
            p = end + 1
        materials.append(row)
    tail = data[p:]
    if len(tail) % 24:
        raise ValueError('Malformed bone bounds')
    return {'header': data[:27], 'lods': lods, 'collision': collision,
            'materials': materials, 'tail': tail}


def replace_near_model(data, lod_name, materials, distance):
    model = read_model(data)
    lods = list(model['lods'])
    lods[0] = (min(lods[0][0], distance) if lods[1][1] else lods[0][0], lod_name)
    rows = [materials] + model['materials'][1:]
    return (model['header'] + b''.join(struct.pack('<f', d) + n.encode() + b'\0' for d, n in lods)
            + model['collision'] + b''.join(struct.pack('<H', len(row))
            + b''.join(n.encode() + b'\0' for n in row) for row in rows) + model['tail'])


def replace_all_model_lods(data, names, materials):
    """Replace a complete visual identity, retaining every original LOD distance."""
    model = read_model(data)
    if len(names) != 4 or len(materials) != 4 or not all(n for _, n in model['lods']):
        raise ValueError('Complete LOD replacement requires four populated levels')
    if not all(n.startswith(PREFIX) for n in names):
        raise ValueError('Generated LOD names must use the owner namespace')
    return (model['header'] + b''.join(struct.pack('<f', d) + n.encode() + b'\0'
            for (d, _), n in zip(model['lods'], names)) + model['collision']
            + b''.join(struct.pack('<H', len(row)) + b''.join(n.encode() + b'\0'
            for n in row) for row in materials) + model['tail'])


def apply_prop_pack(renderer, path):
    path = Path(path)
    if not path.is_file():
        return None
    with zipfile.ZipFile(path) as z:
        report = json.loads(z.read('props-report.json'))
        if report.get('schema') != 1:
            raise ValueError('Unknown static model pack')
        updates = {}
        for name, digest in report['entries'].items():
            generated = any(name.startswith(kind + '/' + PREFIX)
                            for kind in ('images', 'materials', 'xmodelsurfs', 'xmodelparts'))
            if '..' in Path(name).parts or not (generated or name in report['originals'] and name.startswith('xmodel/')):
                raise ValueError('Unexpected prop resource: ' + name)
            data = z.read(name)
            if hashlib.sha256(data).hexdigest() != digest:
                raise ValueError('Corrupt prop resource: ' + name)
            if name in report['originals']:
                before = renderer[name]
                if hashlib.sha256(before).hexdigest() not in (digest, report['originals'][name]):
                    raise ValueError('Prop base changed; reimport: ' + name)
                old, new = read_model(before), read_model(data)
                complete = report['models'][name.removeprefix('xmodel/')].get('completeLods', False)
                if complete:
                    if [d for d, _ in old['lods']] != [d for d, _ in new['lods']]:
                        raise ValueError('Complete prop replacement changes LOD distances: ' + name)
                    refs = ['xmodelsurfs/' + n for _, n in new['lods']] + ['xmodelparts/' + n for _, n in new['lods']]
                    refs += ['materials/' + n for row in new['materials'] for n in row]
                    if len(new['materials']) != 4 or any(not n.startswith(PREFIX) for _, n in new['lods']) or any(n not in report['entries'] for n in refs):
                        raise ValueError('Incomplete generated prop LOD chain: ' + name)
                elif old['lods'][1:] != new['lods'][1:] or old['materials'][1:] != new['materials'][1:]:
                    raise ValueError('Near-only prop changes distant LODs: ' + name)
                if any(old[k] != new[k] for k in ('header', 'collision', 'tail')):
                    raise ValueError('Prop pack changes collision, bounds or distant LODs: ' + name)
            updates[name] = data
        renderer.update(updates)
    return report
