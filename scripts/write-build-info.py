#!/usr/bin/env python3
"""Stamp a generated browser package with its source revision and content ID."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parent.parent
site = Path(sys.argv[1]) if len(sys.argv) > 1 else root / 'out/cod2-wasm-core/site'
revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip()
dirty = bool(subprocess.check_output(['git', 'status', '--porcelain'], cwd=root, text=True).strip())
digest = hashlib.sha256()
digest.update(revision.encode())
for path in sorted(site.iterdir()):
    if path.is_file() and path.name != 'build-info.json':
        digest.update(path.name.encode() + b'\0')
        digest.update(hashlib.sha256(path.read_bytes()).digest())
info = {'revision': revision, 'dirty': dirty, 'buildId': digest.hexdigest()}
(site / 'build-info.json').write_text(json.dumps(info, indent=2) + '\n')
print(f'Browser build: {revision[:12]}{" (local changes)" if dirty else ""}')
