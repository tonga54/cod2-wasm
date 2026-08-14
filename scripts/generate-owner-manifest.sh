#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "usage: $0 /absolute/path/to/Call-of-Duty-2/main" >&2
    exit 2
fi

owner_main="$1"
if [[ "${owner_main}" != /* || ! -d "${owner_main}" ]]; then
    echo "error: owner data must be an existing absolute main directory" >&2
    exit 1
fi

files=(
    iw_00.iwd iw_01.iwd iw_02.iwd iw_03.iwd iw_04.iwd iw_05.iwd iw_06.iwd iw_07.iwd
    iw_08.iwd iw_09.iwd iw_10.iwd iw_11.iwd iw_12.iwd iw_13.iwd iw_14.iwd iw_15.iwd
    localized_english_iw00.iwd localized_english_iw01.iwd
    localized_english_iw02.iwd localized_english_iw03.iwd
    localized_english_iw04.iwd localized_english_iw05.iwd
    localized_english_iw06.iwd localized_english_iw07.iwd
    localized_english_iw08.iwd localized_english_iw09.iwd
    localized_english_iw10.iwd localized_english_iw11.iwd
)

printf '{\n  "schema": 1,\n  "game": "Call of Duty 2",\n  "files": [\n'
for ((i = 0; i < ${#files[@]}; ++i)); do
    name="${files[$i]}"
    path="${owner_main}/${name}"
    if [[ ! -f "${path}" || -L "${path}" ]]; then
        echo "error: missing regular, non-symlink owner file: ${path}" >&2
        exit 1
    fi
    header="$(od -An -N4 -tx1 "${path}" | tr -d ' \n')"
    if [[ "${header}" != "504b0304" ]]; then
        echo "error: ${name} is not a ZIP/IWD file (header ${header})" >&2
        exit 1
    fi
    size="$(stat -c '%s' "${path}")"
    sha256="$(sha256sum "${path}" | cut -d ' ' -f 1)"
    comma=','
    if ((i + 1 == ${#files[@]})); then comma=''; fi
    printf '    {"path":"main/%s","size":%s,"sha256":"%s","header":"%s"}%s\n' \
        "${name}" "${size}" "${sha256}" "${header}" "${comma}"
done
printf '  ]\n}\n'
