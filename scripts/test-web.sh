#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${COD2_WASM_BUILD_DIR:-${repo_root}/out/cod2-wasm-core}"

"${repo_root}/scripts/build-web.sh"
node "${build_dir}/site/cod2_core_probe.js"
node --check "${build_dir}/site/asset-validator.js"

test -f "${build_dir}/site/index.html"
test -f "${build_dir}/site/cod2_core_probe.wasm"
test -f "${build_dir}/CMakeFiles/cod2_client_objects.dir/web_main.c.o"

port="${COD2_TEST_PORT:-18014}"
python3 -m http.server "${port}" --bind 127.0.0.1 --directory "${build_dir}/site" >/dev/null 2>&1 &
server_pid=$!
trap 'kill "${server_pid}" 2>/dev/null || true' EXIT

server_ready=0
for _ in 1 2 3 4 5 6 7 8 9 10; do
    if curl --fail --silent --show-error "http://127.0.0.1:${port}/index.html" >/dev/null; then
        server_ready=1
        break
    fi
    sleep 0.1
done
test "${server_ready}" = 1

curl --fail --silent --show-error "http://127.0.0.1:${port}/asset-validator.js" | grep -q "Owner data mount validated"
curl --fail --silent --show-error "http://127.0.0.1:${port}/cod2_core_probe.wasm" >/dev/null

echo "Static HTTP, JavaScript syntax, native checksum, and full object-graph checks passed."
