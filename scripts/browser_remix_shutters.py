"""Validate optional baked owner shutters; ordinary builds need no USD/Pillow."""
import hashlib
import json
from pathlib import Path
import zipfile

PACK='owner_carentan_shutters.zip'
MATERIALS={'materials/v_shutter_01','materials/v_shutter_02','materials/mtl_caen_window_shutter_03'}


def apply_shutter_pack(renderer,path):
    path=Path(path)
    if not path.is_file():return None
    with zipfile.ZipFile(path) as archive:
        report=json.loads(archive.read('shutters-report.json'))
        if report.get('schema')!=1 or set(report['originals'])!=MATERIALS:raise ValueError('Unexpected shutter pack schema/materials')
        updates={}
        for name,digest in report['entries'].items():
            if '..' in Path(name).parts or not (name in MATERIALS or name.startswith('images/browser_remix/shutter_')):
                raise ValueError('Unexpected shutter resource: '+name)
            data=archive.read(name)
            if hashlib.sha256(data).hexdigest()!=digest:raise ValueError('Corrupt shutter resource: '+name)
            if name in MATERIALS:
                before=renderer[name]
                if hashlib.sha256(before).hexdigest()==digest:
                    updates[name]=data;continue
                if hashlib.sha256(before).hexdigest()!=report['originals'][name]:raise ValueError('Shutter base changed: '+name)
                restored=bytearray(data[:len(before)])
                restored[52:56]=before[52:56];restored[60:68]=before[60:68]
                if bytes(restored)!=before:raise ValueError('Shutter render state changed: '+name)
            updates[name]=data
        if not MATERIALS.issubset(updates):raise ValueError('Incomplete shutter materials')
        renderer.update(updates)
    return report
