#!/usr/bin/env bash
set -euo pipefail
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
docker build --platform linux/amd64 \
  -f "${repo_root}/downstream/server/Dockerfile.build" \
  -t local/cod2-native-build:dev "${repo_root}/downstream/server"
docker run --rm --platform linux/amd64 --cpus "${COD2_BUILD_JOBS:-4}" \
  -v "${repo_root}:/workspace/cod2-wasm" \
  -e "COD2_BUILD_JOBS=${COD2_BUILD_JOBS:-4}" local/cod2-native-build:dev \
  bash -lc 'cmake -S . -B out/cod2-native -DCMAKE_BUILD_TYPE=Release \
    -DCOD2_WWWDL_LIBS= \
    -DCOD2_FEATURE_CFLAGS="-DCOD2_FEATURE_WWW_DOWNLOAD=0 -DCOD2_FEATURE_PUNKBUSTER=0" \
    && cmake --build out/cod2-native --target cod2_lnxded -j "$COD2_BUILD_JOBS"'

python3 "${repo_root}/scripts/test-native-file-listing.py"
