#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${COD2_WASM_BUILD_DIR:-${repo_root}/out/cod2-wasm-core}"

"${repo_root}/scripts/build-web.sh"
node "${build_dir}/site/cod2_core_probe.js"
