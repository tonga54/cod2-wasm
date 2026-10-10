#!/usr/bin/env bash
# Extract the existing engine and its exact i386 library closure for the Pi.
set -euo pipefail
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
image="${COD2_PI_ENGINE_IMAGE:-local/cod2-native-server:dev}"
mkdir -p "${repo_root}/out"
work_dir="$(mktemp -d "${repo_root}/out/pi-runtime.XXXXXX")"
trap 'rm -rf -- "${work_dir}"' EXIT

docker run --rm --platform linux/amd64 --entrypoint sh "${image}" -c '
  set -eu
  ldd /usr/local/bin/cod2_lnxded > /tmp/pi-dependencies
  ! grep -q "not found" /tmp/pi-dependencies
  { printf "%s\n" /usr/local/bin/cod2_lnxded;
    awk '\''/=> \// {print $3} /^[[:space:]]*\// {print $1}'\'' /tmp/pi-dependencies;
  } | sed "s|^/||" | sort -u > /tmp/pi-files
  tar -C / -chf - -T /tmp/pi-files
' | tar -xf - -C "${work_dir}"

# Fail before replacing a previous prepared runtime if the payload is wrong.
python3 - "${work_dir}/usr/local/bin/cod2_lnxded" <<'PY'
from pathlib import Path
import sys
header = Path(sys.argv[1]).read_bytes()[:20]
assert header[:6] == b'\x7fELF\x01\x01', 'Expected a little-endian ELF32 engine'
assert int.from_bytes(header[18:20], 'little') == 3, 'Expected the i386 engine'
PY
test -s "${work_dir}/lib/ld-linux.so.2"
rm -rf -- "${repo_root}/out/pi-runtime"
mv "${work_dir}" "${repo_root}/out/pi-runtime"
echo "Prepared the engine and its i386 runtime in out/pi-runtime."
