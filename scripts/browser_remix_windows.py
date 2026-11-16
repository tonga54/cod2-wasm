"""Optional projected owner window frames; the runtime needs only stdlib."""
import hashlib
import json
from pathlib import Path
import zipfile

PACK = 'owner_carentan_windows.zip'
MATERIALS = {'materials/v_window02', 'materials/v_window07', 'materials/v_window08'}


def apply_window_pack(renderer, path):
    path = Path(path)
    if not path.is_file():
        return None
    with zipfile.ZipFile(path) as archive:
        report = json.loads(archive.read('windows-report.json'))
        if report.get('schema') != 1 or set(report['originals']) != MATERIALS:
            raise ValueError('Unexpected window pack schema/materials')
        updates = {}
        for name, digest in report['entries'].items():
            if '..' in Path(name).parts or not (name in MATERIALS or name.startswith('images/browser_remix/window_frame_')):
                raise ValueError('Unexpected window resource: ' + name)
            data = archive.read(name)
            if hashlib.sha256(data).hexdigest() != digest:
                raise ValueError('Corrupt window resource: ' + name)
            if name in MATERIALS:
                before = renderer[name]
                if hashlib.sha256(before).hexdigest() != digest:
                    if hashlib.sha256(before).hexdigest() != report['originals'][name]:
                        raise ValueError('Window base changed: ' + name)
                    restored = bytearray(data[:len(before)])
                    restored[52:56] = before[52:56]
                    restored[60:68] = before[60:68]
                    if bytes(restored) != before:
                        raise ValueError('Window render state changed: ' + name)
            updates[name] = data
        if not MATERIALS.issubset(updates):
            raise ValueError('Incomplete window materials')
        renderer.update(updates)
    return report
