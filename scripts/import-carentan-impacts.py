#!/usr/bin/env python3
"""Update only impact decals in the existing private owner visual pack."""
import argparse
import base64
import hashlib
import json
from pathlib import Path
import zipfile
from PIL import Image
from browser_remix_assets import PACK
from browser_remix_impacts import impact_assets

root = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('source', type=Path)
args = parser.parse_args()
base = args.source / 'rtx-remix/mods/gameReadyAssets'
path = root / 'data/main' / PACK
with zipfile.ZipFile(path) as z:
    report = json.loads(z.read('remix-report.json'))
    if report['schema'] != 1:
        raise ValueError('Unknown owner visual pack')
    entries = {name: z.read(name) for name in report['entries']}
    for name, data in entries.items():
        if hashlib.sha256(data).hexdigest() != report['entries'][name]:
            raise ValueError('Corrupt owner pack: ' + name)

def load(relative, size):
    source = base / relative
    report['sources'][relative] = hashlib.sha256(source.read_bytes()).hexdigest()
    with Image.open(source) as im:
        image = im.copy()
    image.thumbnail((size, size), Image.Resampling.LANCZOS)
    return image

with zipfile.ZipFile(root / 'data/browser/main/cod2_browser_renderer.iwd') as renderer:
    def read(name):
        data = renderer.read(name)
        if name in report['source_materials']:
            original = base64.b64decode(report['source_materials'][name], validate=True)
            if data not in (original, entries[name]):
                raise ValueError('Impact material has local changes: ' + name)
            return original
        return data
    updates, materials, originals = impact_assets(read, load)
entries.update(updates); report['materials'].update(materials); report['source_materials'].update(originals)
report['entries'] = {n: hashlib.sha256(d).hexdigest() for n, d in entries.items()}
tmp = path.with_suffix('.tmp')
with zipfile.ZipFile(tmp, 'w') as z:
    for name, data in sorted(entries.items()):
        info = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0)); info.compress_type = zipfile.ZIP_DEFLATED
        z.writestr(info, data)
    z.writestr('remix-report.json', json.dumps(report, indent=2) + '\n')
tmp.replace(path)
path.with_suffix('.json').write_text(json.dumps(report, indent=2) + '\n')
print('Updated', len(materials), 'impact materials with', len(updates) - len(materials), 'aligned textures')
