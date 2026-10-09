#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
archive="${1:-${HOME}/Downloads/Call of Duty 2 - version 1.3 - English.zip}"
data_root="${2:-${repo_root}/data}"

[[ -f "${archive}" ]] || { echo "game ZIP not found: ${archive}" >&2; exit 1; }
[[ -d "${data_root}/main" ]] || {
  echo "expected the validated game assets in ${data_root}/main" >&2
  exit 1
}

python3 - "${archive}" "${data_root}" <<'PY'
from pathlib import Path
from zipfile import ZipFile
import sys

archive = Path(sys.argv[1])
destination = Path(sys.argv[2])
prefix = "Call of Duty 2 - 1.3/"
root_files = {
    "CoD2MP_s.exe",
    "cod2patch.ini",
    "gfx_d3d_mp_x86_s.dll",
    "localization.txt",
    "mss32.dll",
    "version.inf",
}
written = []

with ZipFile(archive) as source:
    for entry in source.infolist():
        name = entry.filename
        if not name.startswith(prefix) or entry.is_dir():
            continue
        relative = name[len(prefix):]
        path = Path(relative)
        if path.is_absolute() or ".." in path.parts:
            raise SystemExit(f"unsafe ZIP path: {name}")
        include = relative in root_files or relative.startswith("miles/")
        if not include:
            continue
        output = destination / path
        output.parent.mkdir(parents=True, exist_ok=True)
        with source.open(entry) as input_file, output.open("wb") as output_file:
            while chunk := input_file.read(1024 * 1024):
                output_file.write(chunk)
        written.append(relative)

missing = root_files - set(written)
if missing:
    raise SystemExit("archive is missing required multiplayer files: " + ", ".join(sorted(missing)))
print(f"Imported {len(written)} multiplayer runtime files into {destination}")
PY

test -s "${data_root}/CoD2MP_s.exe"
test -s "${data_root}/gfx_d3d_mp_x86_s.dll"
test -s "${data_root}/mss32.dll"
echo "The container can use ${data_root} as the CoD2 installation root."
