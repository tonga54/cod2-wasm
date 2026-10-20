#!/usr/bin/env bash
# Build public code images only. Original game archives are prepared on the Pi.
set -euo pipefail
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${repo_root}"
# Create the shared output parent as the runner before root Docker processes
# populate it; host-side regressions need to create temporary fixtures here.
mkdir -p out
revision="$(git rev-parse HEAD)"
test -z "$(git status --porcelain)"
[[ "${revision}" =~ ^[a-f0-9]{40}$ ]]
framework_dir="${COD2_WASM_FRAMEWORK_DIR:-${repo_root}/../wasm-game-framework}"
if [[ ! -d "${framework_dir}/.git" ]]; then
  git clone https://github.com/theodorecharles/wasm-game-framework.git "${framework_dir}"
fi
git -C "${framework_dir}" fetch origin 53bc7e6eeef1ae35dcf3b25dea4e3ec0ab46726f
git -C "${framework_dir}" checkout --detach 53bc7e6eeef1ae35dcf3b25dea4e3ec0ab46726f
docker run --rm --platform linux/amd64 \
  -e COD2_VERIFY_BUILD=0 -e "COD2_BUILD_JOBS=${COD2_BUILD_JOBS:-2}" \
  -v "${repo_root}:/workspace/cod2-wasm" -v "${framework_dir}:/workspace/wasm-game-framework" \
  -v cod2-wasm-emscripten-cache:/emsdk/upstream/emscripten/cache \
  -w /workspace/cod2-wasm emscripten/emsdk:3.1.64 bash -lc \
  'git config --global --add safe.directory /workspace/cod2-wasm; git config --global --add safe.directory /workspace/wasm-game-framework; ./scripts/build-web.sh'
DOCKER_DEFAULT_PLATFORM=linux/arm64 "${framework_dir}/scripts/build-base-image.sh" local/wasm-game-framework:0.9.2
web_image="local/cod2-wasm:release-${revision}"
server_image="local/cod2-pi-server:release-${revision}"
gateway_image="local/cod2-gateway:release-${revision}"
docker build --platform linux/arm64 --build-arg FRAMEWORK_IMAGE=local/wasm-game-framework:0.9.2 --build-arg "VCS_REF=${revision}" \
  --label "org.opencontainers.image.revision=${revision}" -t "${web_image}" .
bash scripts/build-server.sh
docker build --platform linux/amd64 -f downstream/server/Dockerfile -t local/cod2-native-server:release .
COD2_PI_ENGINE_IMAGE=local/cod2-native-server:release bash scripts/prepare-pi-runtime.sh
docker build --platform linux/arm64 -f downstream/server/Dockerfile.pi \
  --label "org.opencontainers.image.revision=${revision}" -t "${server_image}" .
docker build --platform linux/arm64 --label "org.opencontainers.image.revision=${revision}" \
  -t "${gateway_image}" downstream/gateway
python3 scripts/test-pi-updater.py
python3 scripts/test-publish-cdn.py
npm ci --prefix downstream/gateway --ignore-scripts --no-audit --no-fund
npm test --prefix downstream/gateway
node --test downstream/cdn/worker.test.mjs
# These source/JS tests need no private IWDs. The Pi then tests real maps/bots.
for script in test-update-notifier test-fullscreen-control test-web-shell test-engine-version test-input-capture test-web-room-create; do
  node "scripts/${script}.cjs"
done
for script in test-dev-console test-client-update-menu test-browser-main-menu test-respawn-fall-jump test-script-vector-order test-script-native-callbacks test-player-lean test-corner-movement test-reload-cancel test-grenade-lifecycle test-mantle-movement test-mantle-hint test-mantle-view test-usercmd-delta test-hud-rotation test-server-frame-wait test-room-maps test-web-audio-levels test-player-health test-menu-sound-aliases; do
  python3 "scripts/${script}.py"
done
mkdir -p out/pi-release
docker save "${web_image}" "${server_image}" "${gateway_image}" | gzip -1 > out/pi-release/pi-images.tar.gz
python3 - "${revision}" "${web_image}" "${server_image}" "${gateway_image}" <<'PY'
import hashlib,json,subprocess,sys,runpy,tarfile
from pathlib import Path
revision,*tags=sys.argv[1:]
images={}
fingerprint=runpy.run_path('scripts/update-pi.py')['image_fingerprint']
for service,tag in zip(('cod2-web','cod2-server','cod2-gateway'),tags):
    image=json.loads(subprocess.check_output(['docker','image','inspect',tag]))[0]
    assert image['Architecture']=='arm64'
    assert image['Config']['Labels']['org.opencontainers.image.revision']==revision
    images[service]={'tag':tag,'id':image['Id']}
build=json.loads(Path('out/cod2-wasm-core/site/build-info.json').read_text())
assert build['revision']==revision and not build['dirty'] and build['devConsole'] is False
assert not subprocess.check_output(['git','status','--porcelain'])
archive=Path('out/pi-release/pi-images.tar.gz')
# Inspect the configuration actually exported, rather than defaults injected
# by the producer daemon's image-inspect API (notably an unset web User).
with tarfile.open(archive,'r:gz') as saved:
    for entry in json.load(saved.extractfile('manifest.json')):
        config=json.load(saved.extractfile(entry['Config']))
        exported={'Architecture':config['architecture'],'Os':config['os'],
                  'Config':config['config'],'RootFS':{'Layers':config['rootfs']['diff_ids']}}
        for image in images.values():
            if image['tag'] in entry['RepoTags']:
                image['fingerprint']=fingerprint(exported)
assert all('fingerprint' in image for image in images.values())
manifest={'schemaVersion':2,'revision':revision,'platform':'linux/arm64','build':build,
 'images':images,'assetManifestSha256':hashlib.sha256(Path('site/wasm-game-data.json').read_bytes()).hexdigest(),
 'archive':{'name':archive.name,'size':archive.stat().st_size,'sha256':hashlib.sha256(archive.read_bytes()).hexdigest()}}
Path('out/pi-release/manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('Validated public ARM64 package:',revision)
PY
