#!/usr/bin/env bash
set -euo pipefail
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
site_dir="${1:-${repo_root}/out/cod2-wasm-core/site}"
framework_dir="${2:-${COD2_WASM_FRAMEWORK_DIR:-${repo_root}/../wasm-game-framework}}"
node --check "${site_dir}/native-game-adapter.js"
node --check "${site_dir}/update-notifier.js"
node --check "${site_dir}/asset-sha256.js"
node --check "${site_dir}/cod2.js"
node "${repo_root}/scripts/test-emscripten-gl.cjs" "${site_dir}/cod2.js"
node "${repo_root}/scripts/test-web-point-lights.cjs" "${site_dir}/cod2.js"
python3 "${repo_root}/scripts/test-web-point-lights.py"
python3 "${repo_root}/scripts/test-baked-shadow-filter.py"
python3 "${repo_root}/scripts/test-muzzle-flash-appearance.py"
node "${repo_root}/scripts/test-web-gl-state-cache.cjs"
node "${repo_root}/scripts/test-performance-meter.cjs"
python3 "${repo_root}/scripts/test-web-resolution-input.py"
python3 "${repo_root}/scripts/test-web-static-index-cache.py"
python3 "${repo_root}/scripts/test-web-static-vertex-cache.py"
python3 "${repo_root}/scripts/test-indexed-model-range.py"
python3 "${repo_root}/scripts/test-model-colors.py"
python3 "${repo_root}/scripts/test-bot-ai.py"
python3 "${repo_root}/scripts/test-bot-menu.py"
node "${repo_root}/scripts/test-bot-foundation.js"
python3 "${repo_root}/scripts/test-room-maps.py"
python3 "${repo_root}/scripts/test-sprint.py"
python3 "${repo_root}/scripts/test-movement-release.py"
python3 "${repo_root}/scripts/test-corpse-lifetime.py"
python3 "${repo_root}/scripts/test-dropped-item-lifetime.py"
python3 "${repo_root}/scripts/test-player-collision-turret.py"
python3 "${repo_root}/scripts/test-player-corpse-init.py"
python3 "${repo_root}/scripts/test-client-corpses.py"
python3 "${repo_root}/scripts/test-player-stance-animations.py"
python3 "${repo_root}/scripts/test-chat.py"
python3 "${repo_root}/scripts/test-chat-delivery.py"
python3 "${repo_root}/scripts/test-grenade-lifecycle.py"
python3 "${repo_root}/scripts/test-grenade-indicators.py"
python3 "${repo_root}/scripts/test-explosion-radius.py"
python3 "${repo_root}/scripts/test-predicted-shot-effects.py"
python3 "${repo_root}/scripts/test-browser-packet-pacing.py"
python3 "${repo_root}/scripts/test-huffman-cache.py"
python3 "${repo_root}/scripts/test-network-bit-fields.py"
python3 "${repo_root}/scripts/test-snapshot-delta-work.py"
python3 "${repo_root}/scripts/test-prediction-command-window.py"
python3 "${repo_root}/scripts/test-server-frame-wait.py"
python3 "${repo_root}/scripts/test-server-fragment-pacing.py"
python3 "${repo_root}/scripts/test-weapon-fire-modes.py"
python3 "${repo_root}/scripts/test-ads-transitions.py"
python3 "${repo_root}/scripts/test-hold-breath.py"
python3 "${repo_root}/scripts/test-reload-ads.py"
python3 "${repo_root}/scripts/test-gameplay-feedback.py"
python3 "${repo_root}/scripts/test-mounted-recoil.py"
python3 "${repo_root}/scripts/test-corner-movement.py"
python3 "${repo_root}/scripts/test-trace-pruning.py"
python3 "${repo_root}/scripts/test-collision-loading.py"
python3 "${repo_root}/scripts/test-mesh-border-trace.py"
python3 "${repo_root}/scripts/test-client-clock-recoil.py"
python3 "${repo_root}/scripts/test-client-connect-timeout.py"
python3 "${repo_root}/scripts/test-remote-player-motion.py"
python3 "${repo_root}/scripts/test-packet-entity-positions.py"
python3 "${repo_root}/scripts/test-ui-game-modes.py"
python3 "${repo_root}/scripts/test-ui-menu-clicks.py"
python3 "${repo_root}/scripts/test-ui-create-focus.py"
python3 "${repo_root}/scripts/test-script-lexer-eof.py"
python3 "${repo_root}/scripts/test-script-string-restart.py"
node "${repo_root}/scripts/test-input-capture.cjs" "${framework_dir}/dist/wasm-game-framework.js"
node "${repo_root}/scripts/test-fullscreen-control.cjs"
node "${repo_root}/scripts/test-gametag.cjs"
node "${repo_root}/scripts/test-engine-version.cjs"
node "${repo_root}/scripts/test-asset-sha256.cjs"
node "${repo_root}/scripts/test-lan-validation.cjs"
node "${repo_root}/scripts/test-web-net-errors.cjs"
node "${repo_root}/scripts/test-web-background.cjs"
python3 "${repo_root}/scripts/test-web-background-frame.py"
node "${repo_root}/scripts/test-web-discovery.cjs"
node "${repo_root}/scripts/test-web-room-create.cjs"
node "${repo_root}/scripts/test-web-qport.cjs"
node "${repo_root}/scripts/test-web-audio.cjs"
node "${repo_root}/scripts/test-update-notifier.cjs"
python3 "${repo_root}/scripts/test-update-local.py"
node "${framework_dir}/scripts/check-game-package.js" "${site_dir}"
node - "${site_dir}" <<'NODE'
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const site = process.argv[2];
const expected = [
  'asset-sha256.js', 'build-info.json', 'cod2-diagnostic.svg', 'cod2.js', 'cod2.wasm', 'native-game-adapter.js',
  'startup.css', 'update-notifier.js', 'wasm-game-data.json', 'wasm-game-framework.json', 'wasm-game.json'
];
assert.deepEqual(fs.readdirSync(site).sort(), expected.sort());
assert.deepEqual([...fs.readFileSync(path.join(site, 'cod2.wasm')).subarray(0, 8)], [0,97,115,109,1,0,0,0]);
const config = JSON.parse(fs.readFileSync(path.join(site, 'wasm-game.json')));
assert.deepEqual(Object.keys(config.variants), ['cod2-mp']);
assert.equal(config.engine, 'IW 2.0 reconstruction');
assert.equal(config.adapter, '/native-game-adapter.js');
assert.equal(config.pointerLock, true);
assert.equal(config.autoStart, true);
assert.equal(config.updates, true);
const build = JSON.parse(fs.readFileSync(path.join(site, 'build-info.json')));
assert.match(build.revision, /^[a-f0-9]{40}$/);
assert.match(build.buildId, /^[a-f0-9]{64}$/);
assert.equal(typeof build.dirty, 'boolean');
assert.equal(config.identity, false);
assert.equal(config.variants['cod2-mp'].title, 'Call of Duty 2 Multiplayer');
assert.equal(config.variants['cod2-mp'].shortTitle, 'Call of Duty 2 Multiplayer');
assert.equal(config.variants['cod2-mp'].icon, '/game-data/files/cod2-icon/original.ico');
assert.equal(config.controller?.mode, 'disabled');
const files = JSON.parse(fs.readFileSync(path.join(site, 'wasm-game-data.json'))).variants['cod2-mp'].files;
assert.ok(files.length > 0);
for (const file of files) {
  assert.match(file.key, /^[a-z0-9-]+$/);
  assert.match(file.path, /^main\/[a-z0-9_]+\.iwd$/);
  assert.match(file.sha256, /^[a-f0-9]{64}$/);
  assert.deepEqual(file.magic, [80,75,3,4]);
  assert.ok(Number.isSafeInteger(file.size) && file.size > 0);
}
assert.equal(JSON.parse(fs.readFileSync(path.join(site, 'wasm-game-framework.json'))).version, '0.9.2');
assert.ok(!fs.readdirSync(site).some(name => /\.(iwd|data|html)$/i.test(name)));
NODE
tracked_assets="$(git -C "${repo_root}" ls-files '*.iwd' '*.wasm' '*.data')"
if [[ -n "${tracked_assets}" ]]; then
  printf '%s\n' "${tracked_assets}"
  echo 'Private assets or generated binaries are tracked' >&2
  exit 1
fi
git -C "${repo_root}" diff --check
echo 'Browser package and private asset boundaries passed; gameplay still requires browser verification.'
