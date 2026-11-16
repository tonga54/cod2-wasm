"""Validated optional owner Enfield mesh pack; no USD dependency at runtime."""
import hashlib
import json
from pathlib import Path
import zipfile

PACK = 'owner_carentan_enfield.zip'
MODEL = 'xmodel/viewmodel_enfield'
SURFACE = 'xmodelsurfs/viewmodel_enfield2'


def apply_model_pack(renderer, path):
    path = Path(path)
    if not path.is_file(): return None
    with zipfile.ZipFile(path) as z:
        report = json.loads(z.read('model-report.json'))
        if report.get('schema') != 1: raise ValueError('Unknown Enfield pack format')
        updates = {}
        for name, digest in report['entries'].items():
            if (name not in (MODEL, SURFACE) and not name.startswith(('materials/browser_remix/enfield_', 'images/browser_remix/enfield_'))) or '..' in Path(name).parts:
                raise ValueError('Unexpected Enfield asset: '+name)
            data = z.read(name)
            if hashlib.sha256(data).hexdigest() != digest: raise ValueError('Corrupt Enfield asset: '+name)
            if name in (MODEL, SURFACE) and hashlib.sha256(renderer[name]).hexdigest() not in (digest, report['originals'][name]):
                raise ValueError('Enfield base changed; reimport: '+name)
            updates[name] = data
        renderer.update(updates)
    return report
