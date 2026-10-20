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
python3 "${repo_root}/scripts/test-web-materials.py"
python3 "${repo_root}/scripts/test-web-scene-fx.py"
python3 "${repo_root}/scripts/test-viewmodel-motion.py"
node "${repo_root}/scripts/test-web-scene-fx-scale.cjs"
python3 "${repo_root}/scripts/test-wwii-scope.py"
python3 "${repo_root}/scripts/test-combat-presentation.py"
python3 "${repo_root}/scripts/test-fx-emitter-model.py"
python3 "${repo_root}/scripts/test-model-bounds.py"
python3 "${repo_root}/scripts/test-web-fog.py"
python3 "${repo_root}/scripts/test-web-lod-distance.py"
python3 "${repo_root}/scripts/test-baked-shadow-filter.py"
python3 "${repo_root}/scripts/test-muzzle-flash-appearance.py"
python3 "${repo_root}/scripts/test-muzzle-tail-motion.py"
python3 "${repo_root}/scripts/test-fx-atlas-coords.py"
node "${repo_root}/scripts/test-web-gl-state-cache.cjs"
node "${repo_root}/scripts/test-performance-meter.cjs"
python3 "${repo_root}/scripts/test-web-resolution-input.py"
python3 "${repo_root}/scripts/test-gamepad.py"
python3 "${repo_root}/scripts/test-rumble.py"
python3 "${repo_root}/scripts/test-aim-assist.py"
python3 "${repo_root}/scripts/test-mouse-wheel.py"
python3 "${repo_root}/scripts/test-web-static-index-cache.py"
python3 "${repo_root}/scripts/test-web-static-vertex-cache.py"
python3 "${repo_root}/scripts/test-indexed-model-range.py"
python3 "${repo_root}/scripts/test-model-colors.py"
python3 "${repo_root}/scripts/test-bot-ai.py"
python3 "${repo_root}/scripts/test-bot-player-roster.py"
python3 "${repo_root}/scripts/test-bot-lookat.py"
python3 "${repo_root}/scripts/test-bot-headless-hud.py"
python3 "${repo_root}/scripts/test-bot-frame-cadence.py"
python3 "${repo_root}/scripts/test-script-vector-order.py"
python3 "${repo_root}/scripts/test-objective-interactions.py"
python3 "${repo_root}/scripts/test-linked-player-origin.py"
python3 "${repo_root}/scripts/test-use-trigger-linkage.py"
python3 "${repo_root}/scripts/test-bot-menu.py"
node "${repo_root}/scripts/test-bot-foundation.js"
node "${repo_root}/scripts/test-bot-scheduler.cjs"
python3 "${repo_root}/scripts/test-room-maps.py"
python3 "${repo_root}/scripts/test-class-data.py"
python3 "${repo_root}/scripts/test-weapon-camo.py"
python3 "${repo_root}/scripts/test-class-camo.py"
python3 "${repo_root}/scripts/test-class-select.py"
python3 "${repo_root}/scripts/test-attachments.py"
python3 "${repo_root}/scripts/test-suppressed-shots.py"
python3 "${repo_root}/scripts/test-attachment-sight.py"
python3 "${repo_root}/scripts/test-view-attachments.py"
python3 "${repo_root}/scripts/test-browser-attachment-models.py"
node "${repo_root}/scripts/test-web-classes.cjs" "${site_dir}/cod2.js"
python3 "${repo_root}/scripts/test-sprint.py"
python3 "${repo_root}/scripts/test-spectator-sprint.py"
python3 "${repo_root}/scripts/test-hip-crosshair.py"
python3 "${repo_root}/scripts/test-hit-marker.py"
python3 "${repo_root}/scripts/test-hit-sound-assets.py"
python3 "${repo_root}/scripts/test-menu-music-assets.py"
python3 "${repo_root}/scripts/test-prematch-music-assets.py"
python3 "${repo_root}/scripts/test-owner-sound-pack.py"
python3 "${repo_root}/scripts/test-owner-class-pictures.py"
python3 "${repo_root}/scripts/test-minimap.py"
python3 "${repo_root}/scripts/test-minimap-renderer.py"
python3 "${repo_root}/scripts/test-objective-visibility.py"
python3 "${repo_root}/scripts/test-match-points.py"
python3 "${repo_root}/scripts/test-score-popup.py"
python3 "${repo_root}/scripts/test-reliable-obituaries.py"
python3 "${repo_root}/scripts/test-kill-feed.py"
python3 "${repo_root}/scripts/test-wwii-kill-feed.py"
python3 "${repo_root}/scripts/test-headshot-icon.py"
python3 "${repo_root}/scripts/test-server-command-prefix.py"
python3 "${repo_root}/scripts/test-score-panel.py"
python3 "${repo_root}/scripts/test-score-table.py"
python3 "${repo_root}/scripts/test-ammo-counter.py"
python3 "${repo_root}/scripts/test-weapon-prompt.py"
python3 "${repo_root}/scripts/test-player-names.py"
python3 "${repo_root}/scripts/test-killcam-timing.py"
python3 "${repo_root}/scripts/test-script-clock.py"
python3 "${repo_root}/scripts/test-killcam.py"
python3 "${repo_root}/scripts/test-endgame.py"
python3 "${repo_root}/scripts/test-prematch.py"
python3 "${repo_root}/scripts/test-frozen-controls.py"
python3 "${repo_root}/scripts/test-player-lean.py"
python3 "${repo_root}/scripts/test-movement-release.py"
python3 "${repo_root}/scripts/test-corpse-lifetime.py"
python3 "${repo_root}/scripts/test-dropped-item-lifetime.py"
python3 "${repo_root}/scripts/test-dropped-weapon-state.py"
python3 "${repo_root}/scripts/test-player-collision-turret.py"
python3 "${repo_root}/scripts/test-player-corpse-init.py"
python3 "${repo_root}/scripts/test-client-corpses.py"
python3 "${repo_root}/scripts/test-player-stance-animations.py"
python3 "${repo_root}/scripts/test-animation-controllers.py"
python3 "${repo_root}/scripts/test-animation-pose-weight.py"
python3 "${repo_root}/scripts/test-xanim-final-frame.py"
python3 "${repo_root}/scripts/test-remote-player-controllers.py"
python3 "${repo_root}/scripts/test-chat.py"
python3 "${repo_root}/scripts/test-chat-delivery.py"
python3 "${repo_root}/scripts/test-grenade-lifecycle.py"
python3 "${repo_root}/scripts/test-placed-explosives.py"
python3 "${repo_root}/scripts/test-browser-smine.py"
python3 "${repo_root}/scripts/test-grenade-indicators.py"
python3 "${repo_root}/scripts/test-grenade-warning.py"
python3 "${repo_root}/scripts/test-breath-prompt.py"
python3 "${repo_root}/scripts/test-wwii-fonts.py"
python3 "${repo_root}/scripts/test-wwii-main-menu.py"
python3 "${repo_root}/scripts/test-wwii-settings.py"
python3 "${repo_root}/scripts/test-match-menu.py"
python3 "${repo_root}/scripts/test-pause-menu.py"
python3 "${repo_root}/scripts/test-class-menu-pause.py"
python3 "${repo_root}/scripts/test-pause-overview.py"
python3 "${repo_root}/scripts/test-hud-rotation.py"
python3 "${repo_root}/scripts/test-explosion-radius.py"
python3 "${repo_root}/scripts/test-predicted-shot-effects.py"
python3 "${repo_root}/scripts/test-viewmodel-effects.py"
python3 "${repo_root}/scripts/test-browser-packet-pacing.py"
python3 "${repo_root}/scripts/test-huffman-cache.py"
python3 "${repo_root}/scripts/test-network-bit-fields.py"
python3 "${repo_root}/scripts/test-usercmd-delta.py"
python3 "${repo_root}/scripts/test-snapshot-delta-work.py"
python3 "${repo_root}/scripts/test-prediction-command-window.py"
python3 "${repo_root}/scripts/test-server-frame-wait.py"
python3 "${repo_root}/scripts/test-server-fragment-pacing.py"
python3 "${repo_root}/scripts/test-weapon-fire-modes.py"
python3 "${repo_root}/scripts/test-bullet-penetration.py"
python3 "${repo_root}/scripts/test-empty-weapon-slot.py"
python3 "${repo_root}/scripts/test-respawn-fall-jump.py"
python3 "${repo_root}/scripts/test-step-move.py"
python3 "${repo_root}/scripts/test-mantle-movement.py"
python3 "${repo_root}/scripts/test-mantle-view.py"
python3 "${repo_root}/scripts/test-mantle-hint.py"
python3 "${repo_root}/scripts/test-mantle-prompt.py"
python3 "${repo_root}/scripts/test-melee-damage.py"
python3 "${repo_root}/scripts/test-ads-transitions.py"
python3 "${repo_root}/scripts/test-hold-breath.py"
python3 "${repo_root}/scripts/test-reload-ads.py"
python3 "${repo_root}/scripts/test-reload-cancel.py"
python3 "${repo_root}/scripts/test-weapon-timers.py"
python3 "${repo_root}/scripts/test-weapon-view-ground.py"
python3 "${repo_root}/scripts/test-gameplay-feedback.py"
python3 "${repo_root}/scripts/test-mounted-recoil.py"
python3 "${repo_root}/scripts/test-corner-movement.py"
python3 "${repo_root}/scripts/test-trace-pruning.py"
python3 "${repo_root}/scripts/test-collision-loading.py"
python3 "${repo_root}/scripts/test-mesh-border-trace.py"
python3 "${repo_root}/scripts/test-client-clock-recoil.py"
python3 "${repo_root}/scripts/test-damage-view-kick.py"
python3 "${repo_root}/scripts/test-client-connect-timeout.py"
python3 "${repo_root}/scripts/test-remote-player-motion.py"
python3 "${repo_root}/scripts/test-packet-entity-positions.py"
python3 "${repo_root}/scripts/test-ui-game-modes.py"
python3 "${repo_root}/scripts/test-ui-map-selection.py"
python3 "${repo_root}/scripts/test-ui-menu-clicks.py"
python3 "${repo_root}/scripts/test-ui-create-focus.py"
python3 "${repo_root}/scripts/test-ui-directional-focus.py"
python3 "${repo_root}/scripts/test-script-lexer-eof.py"
python3 "${repo_root}/scripts/test-script-string-restart.py"
python3 "${repo_root}/scripts/test-script-variable-collisions.py"
python3 "${repo_root}/scripts/test-script-notify-ownership.py"
python3 "${repo_root}/scripts/test-script-native-callbacks.py"
python3 "${repo_root}/scripts/test-script-local-archives.py"
python3 "${repo_root}/scripts/test-script-notify-continuation.py"
python3 "${repo_root}/scripts/test-round-script-reuse.py"
node "${repo_root}/scripts/test-input-capture.cjs" "${framework_dir}/dist/wasm-game-framework.js"
node "${repo_root}/scripts/test-fullscreen-control.cjs"
node "${repo_root}/scripts/test-web-shell.cjs"
python3 "${repo_root}/scripts/test-browser-main-menu.py"
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
node "${repo_root}/scripts/test-web-haptics.cjs"
node "${repo_root}/scripts/test-update-notifier.cjs"
python3 "${repo_root}/scripts/test-client-update-menu.py"
python3 "${repo_root}/scripts/test-dev-console.py"
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
assert.equal(build.devConsole, false, 'a player console build is for local debugging only');
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
tracked_assets="$(git -C "${repo_root}" ls-files '*.iwd' '*.wasm' '*.data' '*.wav' '*.mp3')"
if [[ -n "${tracked_assets}" ]]; then
  printf '%s\n' "${tracked_assets}"
  echo 'Private assets or generated binaries are tracked' >&2
  exit 1
fi
git -C "${repo_root}" diff --check
echo 'Browser package and private asset boundaries passed; gameplay still requires browser verification.'
