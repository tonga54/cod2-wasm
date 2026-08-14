#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${COD2_WASM_BUILD_DIR:-${repo_root}/out/cod2-wasm-core}"

if ! command -v emcmake >/dev/null 2>&1; then
    if [[ -z "${EMSDK:-}" || ! -f "${EMSDK}/emsdk_env.sh" ]]; then
        echo "error: activate Emscripten or set EMSDK to an emsdk checkout" >&2
        exit 1
    fi
    export EMSDK_QUIET=1
    # shellcheck disable=SC1091
    source "${EMSDK}/emsdk_env.sh"
fi

emcmake cmake \
    -S "${repo_root}/downstream/wasm" \
    -B "${build_dir}" \
    -DCMAKE_BUILD_TYPE=Release
cmake --build "${build_dir}" --parallel

echo "Built honest native-core probe under ${build_dir}/site"
echo "This artifact is not a playable Call of Duty 2 build."
