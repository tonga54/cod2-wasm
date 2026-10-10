#!/usr/bin/env bash
# Run from the build machine after building the current game for ARM64 delivery.
set -euo pipefail
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ $# -lt 1 || $# -gt 2 ]]; then
  echo "Usage: $0 user@pi-host [directory-name]" >&2
  exit 2
fi
target="$1"
remote_dir="${2:-cod2-wasm}"
[[ "${target}" =~ ^[A-Za-z0-9_.-]+@[A-Za-z0-9_.:-]+$ ]] || {
  echo 'Use an SSH target in user@host form.' >&2; exit 2;
}
[[ "${remote_dir}" =~ ^[A-Za-z0-9][A-Za-z0-9_.-]*$ ]] || {
  echo 'The destination must be a directory name under the SSH user home.' >&2; exit 2;
}
web_image="${COD2_PI_WEB_IMAGE:-local/cod2-wasm:dev}"
gateway_image="${COD2_PI_GATEWAY_IMAGE:-local/cod2-gateway:dev}"
for image in "${web_image}" "${gateway_image}"; do
  [[ "${image}" =~ ^[A-Za-z0-9_./:@-]+$ ]] || { echo "Invalid image name" >&2; exit 2; }
  test "$(docker image inspect "${image}" --format '{{.Architecture}}')" = arm64 || {
    echo "${image} must be built for linux/arm64 before deploying to the Pi." >&2; exit 1;
  }
done
if [[ -n "$(git -C "${repo_root}" status --porcelain)" ]]; then
  echo 'Commit the source changes before deploying a reproducible Pi build.' >&2
  exit 1
fi
revision="$(git -C "${repo_root}" rev-parse HEAD)"
# Match the image to the committed source before sending any private data.
docker run --rm --entrypoint node "${web_image}" -e '
  const build = require("/opt/game-site/build-info.json");
  if (build.dirty || build.revision !== process.argv[1]) {
    throw new Error("Rebuild/stamp the web image from the clean current revision first");
  }
' "${revision}"
python3 "${repo_root}/scripts/check-private-assets.py"
"${repo_root}/scripts/prepare-pi-runtime.sh"

ssh_options=()
ssh_work_dir=""
if [[ -n "${COD2_PI_SSH_CONTROL:-}" ]]; then
  ssh_options+=(-o "ControlPath=${COD2_PI_SSH_CONTROL}")
else
  ssh_work_dir="$(mktemp -d "${repo_root}/out/pi-ssh.XXXXXX")"
  ssh_options+=(-o "ControlPath=${ssh_work_dir}/control" -o ControlMaster=auto -o ControlPersist=5m)
fi
cleanup() {
  if [[ -n "${ssh_work_dir}" ]]; then
    ssh "${ssh_options[@]}" -O exit "${target}" >/dev/null 2>&1 || true
    rm -rf -- "${ssh_work_dir}"
  fi
}
trap cleanup EXIT
if [[ -n "${COD2_PI_KNOWN_HOSTS:-}" ]]; then
  ssh_options+=(-o "UserKnownHostsFile=${COD2_PI_KNOWN_HOSTS}" -o StrictHostKeyChecking=yes)
fi
printf -v rsync_ssh '%q ' ssh "${ssh_options[@]}"
# Never replace an unrelated checkout, dirty source tree or running match.
ssh "${ssh_options[@]}" "${target}" "python3 - '${remote_dir}'" <<'PY'
import json, pathlib, shutil, subprocess, sys, urllib.request
assert subprocess.check_output(['uname', '-m'], text=True).strip() == 'aarch64', '64-bit ARM Linux is required'
assert subprocess.check_output(['getconf', 'PAGESIZE'], text=True).strip() == '4096', 'The i386 runtime requires a 4 KiB-page kernel; see docs/RASPBERRY_PI.md'
assert shutil.disk_usage(pathlib.Path.home()).free > 2 * 1024 ** 3, 'At least 2 GiB free space is required'
subprocess.run(['docker', 'compose', 'version'], check=True)
path = pathlib.Path.home() / sys.argv[1]
if path.exists():
    assert (path / '.git').is_dir(), 'Destination exists and is not a Git checkout'
    assert subprocess.check_output(['git', '-C', str(path), 'remote', 'get-url', 'origin'], text=True).strip() == 'https://github.com/tonga54/cod2-wasm.git', 'Unexpected repository'
    assert not subprocess.check_output(['git', '-C', str(path), 'status', '--porcelain']), 'Pi source has local changes'
    assert subprocess.check_output(['git', '-C', str(path), 'branch', '--show-current'], text=True).strip() == 'master', 'Pi checkout must use master'
try:
    with urllib.request.urlopen('http://127.0.0.1:8088/gateway/health', timeout=2) as reply:
        health = json.load(reply)
    assert health.get('clients', 0) == 0, 'Players are connected; leave matches before updating'
except OSError:
    pass
PY
ssh "${ssh_options[@]}" "${target}" "set -eu; if test ! -d '${remote_dir}'; then git clone --depth 1 --branch master https://github.com/tonga54/cod2-wasm.git '${remote_dir}'; fi; git -C '${remote_dir}' fetch origin; git -C '${remote_dir}' merge --ff-only origin/master; test \"\$(git -C '${remote_dir}' rev-parse HEAD)\" = '${revision}'; mkdir -p '${remote_dir}/out/pi-runtime' '${remote_dir}/data/browser'"

images_file="${repo_root}/out/pi-images.tar.gz"
docker save "${web_image}" "${gateway_image}" | gzip -1 > "${images_file}"
rsync -az -e "${rsync_ssh}" "${repo_root}/out/pi-runtime/" "${target}:${remote_dir}/out/pi-runtime/"
rsync -az -e "${rsync_ssh}" "${repo_root}/data/browser/" "${target}:${remote_dir}/data/browser/"
rsync -az -e "${rsync_ssh}" "${images_file}" "${target}:${remote_dir}/out/pi-images.tar.gz"
ssh "${ssh_options[@]}" "${target}" "set -eu; cd '${remote_dir}'; docker load < out/pi-images.tar.gz; rm out/pi-images.tar.gz; docker tag '${web_image}' local/cod2-wasm:pi; docker tag '${gateway_image}' local/cod2-gateway:pi; docker compose -p cod2-wasm-pi -f compose.yaml -f compose.pi.yaml build cod2-server; docker compose -p cod2-wasm-pi -f compose.yaml -f compose.pi.yaml up -d --no-build; docker compose -p cod2-wasm-pi -f compose.yaml -f compose.pi.yaml ps"
echo 'Pi services started. Open http://PI_LAN_IP:8088/ and create a room.'
