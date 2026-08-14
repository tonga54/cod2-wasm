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
cmake --build "${build_dir}" --target cod2_client_objects cod2_core_probe --parallel

manifest="${build_dir}/site/owner-manifest.json"
if [[ -n "${COD2_OWNER_DATA:-}" ]]; then
    "${repo_root}/scripts/generate-owner-manifest.sh" "${COD2_OWNER_DATA}" > "${manifest}"
    echo "Generated a private owner-data manifest from ${COD2_OWNER_DATA}"
else
    cmake -E rm -f "${manifest}"
    echo "No owner-data manifest generated (set COD2_OWNER_DATA to the local main directory)."
fi

if [[ "${COD2_ATTEMPT_CLIENT_LINK:-0}" == "1" ]]; then
    cmake --build "${build_dir}" --target cod2_client --parallel
fi

echo "Compiled the reconstructed multiplayer client object graph and diagnostic site."
echo "The client link remains blocked; this artifact is not a playable game build."
