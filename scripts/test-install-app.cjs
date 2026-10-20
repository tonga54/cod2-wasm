// The browser's install offer: icons Chrome accepts, served by the gateway, and an offer in the main menu's right
// key-prompt place that never takes the game's cursor or appears in a match or an installed window.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const {prepareWebShell} = require('./prepare-web-shell.cjs');

const root = path.join(__dirname, '..');
const read = file => fs.readFileSync(path.join(root, file), 'utf8');
const listeners = new Map(), frames = new Map(), appended = [];
let nextFrame = 1, moment = false, standalone = false;
class Element {
  constructor() { this.attributes = new Map(); this.hidden = false; }
  setAttribute(name, value) { this.attributes.set(name, String(value)); }
  removeAttribute(name) { this.attributes.delete(name); }
  toggleAttribute(name, force) { if (force) this.attributes.set(name, ''); else this.attributes.delete(name); return force; }
  getBoundingClientRect() { return {left: 1700, right: 1830, top: 940, bottom: 970}; }
}
const sandbox = {
  matchMedia: query => ({get matches() { return standalone && query === '(display-mode: standalone)'; }}),
  requestAnimationFrame(callback) { frames.set(nextFrame, callback); return nextFrame++; },
  cancelAnimationFrame(id) { frames.delete(id); },
  addEventListener(type, callback, options) { listeners.set(type, {callback, options}); },
  document: {createElement: tag => Object.assign(new Element(), {tag}), body: {appendChild: node => appended.push(node)}},
  cod2InstallMoment: () => moment
};
vm.createContext(sandbox);
vm.runInContext(read('site/install-app.js'), sandbox);
const dispatch = (type, event = {}) => listeners.get(type).callback(event);
function frame() {
  const pending = [...frames.values()];
  frames.clear();
  for (const callback of pending) callback();
}
function offer(prompt = async () => ({outcome: 'accepted'})) {
  const event = {prevented: 0, prompts: 0, preventDefault() { ++this.prevented; },
    prompt() { ++this.prompts; return prompt(); }};
  dispatch('beforeinstallprompt', event);
  return event;
}
function press(x, y, button = 0) {
  const event = {clientX: x, clientY: y, button, prevented: false, stopped: false,
    preventDefault() { this.prevented = true; }, stopImmediatePropagation() { this.stopped = true; }};
  dispatch('pointerdown', event);
  return event;
}

assert.equal(listeners.get('pointerdown').options, true, 'clicks are taken before the canvas and SDL see them');
assert.deepEqual({...listeners.get('pointermove').options}, {capture: true, passive: true});
moment = true;
frame();
assert.equal(appended.length, 0, 'nothing is shown before the browser offers installation');
assert.equal(press(1750, 950).prevented, false, 'without an offer every click belongs to the game');

const adapterMoment = sandbox.cod2InstallMoment;
delete sandbox.cod2InstallMoment;
const early = offer();
frame();
assert.equal(appended.length, 0, 'an offer before the adapter loads waits for it');
sandbox.cod2InstallMoment = adapterMoment;
moment = false;
const first = offer();
assert.equal(early.prompts + first.prompts, 0);
assert.equal(first.prevented, 1, 'the browser\'s own mini-infobar gives way to the menu offer');
frame();
assert.equal(appended.length, 0, 'the offer waits for the loading screen or the main menu');
moment = true;
frame();
assert.equal(appended.length, 1);
const label = appended[0];
assert.equal(label.id, 'cod2-install-app');
assert.equal(label.hidden, false);
assert.equal(label.attributes.get('aria-hidden'), 'true');
assert.match(label.innerHTML, /<svg[^>]*>.*<\/svg>Install app$/);
moment = false;
frame();
assert.equal(label.hidden, true, 'a match hides the offer');
moment = true;
frame();
assert.equal(label.hidden, false, 'leaving the match shows it again');
assert.equal(appended.length, 1, 'one label is reused');

dispatch('pointermove', {clientX: 1750, clientY: 950});
assert.ok(label.attributes.has('data-hover'), 'the game cursor over the label lights it gold');
dispatch('pointermove', {clientX: 900, clientY: 950});
assert.ok(!label.attributes.has('data-hover'));
for (const [x, y, button] of [[1750, 950, 2], [1690, 950, 0], [1750, 975, 0]]) {
  const click = press(x, y, button);
  assert.ok(!click.prevented && !click.stopped, `the game keeps clicks at ${x},${y} button ${button}`);
}
assert.equal(first.prompts, 0);
dispatch('pointermove', {clientX: 1750, clientY: 950});
const install = press(1750, 950);
assert.ok(install.prevented && install.stopped, 'the install click never reaches the menu under the label');
assert.equal(first.prompts, 1, 'the click itself, a user activation, prompts');
assert.equal(label.hidden, true);
assert.ok(!label.attributes.has('data-hover'));
assert.equal(frames.size, 0, 'a spent offer stops watching the game');
assert.equal(press(1750, 950).stopped, false, 'an offer prompts once');
assert.equal(first.prompts, 1);

// Chromium may offer again after a refusal; installing from its own menu ends the offer too.
const refused = offer(() => { throw new Error('NotAllowedError'); });
frame();
assert.equal(label.hidden, false);
assert.doesNotThrow(() => press(1750, 950), 'a refused prompt is contained');
assert.equal(refused.prompts, 1);
const rejected = offer(() => Promise.reject(new Error('dismissed')));
frame();
press(1750, 950);
assert.equal(rejected.prompts, 1);
offer();
frame();
assert.equal(label.hidden, false);
dispatch('appinstalled');
assert.equal(label.hidden, true);
assert.equal(frames.size, 0);

// The installed app's own window never offers itself.
standalone = true;
offer();
frame();
assert.equal(label.hidden, true);
assert.equal(press(1750, 950).stopped, false);
standalone = false;
frame();
assert.equal(label.hidden, false);

// The adapter's moment: the loading screen until the native menu runs, then the main menu outside a match.
const adapter = read('site/native-game-adapter.js');
assert.match(adapter, /startUpdateNotifier\(context\);\n\s*startInstallOffer\(context\);/);

// The label as the WWII menu's right key prompt, from its own constants.
const menu = read('src/PC/ui_mp/ui_wwii_menu.c');
const constant = name => Number(menu.match(new RegExp(`#define ${name} ([\\d.]+)f`))[1]);
const css = read('site/startup.css');
const rule = css.slice(css.indexOf('#cod2-install-app {'), css.indexOf('}', css.indexOf('#cod2-install-app {')));
const units = property => Number(rule.match(new RegExp(`${property}: calc\\(.*?([\\d.]+) \\* var\\(--cod2-unit\\)\\)`))[1]);
const size = constant('WWII_PROMPT_CAP') / 0.7, pad = units('padding');
assert.ok(Math.abs(units('font-size') - size) < 1e-3, 'capitals as tall as the prompts\'');
assert.ok(Math.abs(units('right') + pad - constant('WWII_PROMPT_RIGHT')) < 1e-3, 'the pen ends where the prompt\'s does');
assert.ok(Math.abs(units('bottom') + pad + 0.1 * size - constant('WWII_PROMPT_BASELINE')) < 1e-3, 'on the prompts\' baseline');
assert.ok(Math.abs(units('gap') - constant('WWII_PROMPT_GAP')) < 1e-3);
assert.match(rule, /--cod2-unit: calc\(min\(100vh, 56\.25vw\) \/ 480\);/, 'units of the 16:9 view, 480 tall');
assert.match(rule, /pointer-events: none;/, 'the game keeps its single cursor over the label');
assert.ok(Number(rule.match(/z-index: (\d+);/)[1]) > 50, 'above the framework\'s loading screen');
assert.match(css, /#cod2-install-app\[data-hover\] \{\s*color: #ccb878;/, 'the menu\'s focused gold');

// Chrome needs a PNG of at least 144 px for "any"; the original ICO declares its real sizes so it is never taken for one.
(async () => {
  const {APP_ICONS} = await import(path.join(root, 'downstream/gateway/app-icons.mjs'));
  const icons = JSON.parse(read('site/wasm-game.json')).variants['cod2-mp'].pwa.icons;
  const served = src => APP_ICONS[src.match(/^\/game-data\/files\/cod2-icon\/([a-z0-9-]+\.png)$/)?.[1]];
  for (const icon of icons.filter(icon => icon.type === 'image/png')) {
    assert.ok(served(icon.src), `the gateway draws ${icon.src}`);
    assert.equal(icon.sizes, `${served(icon.src).size}x${served(icon.src).size}`, icon.src);
  }
  assert.ok(icons.some(icon => icon.type === 'image/png' && icon.purpose === 'any' && parseInt(icon.sizes, 10) >= 144));
  assert.ok(icons.some(icon => icon.type === 'image/png' && icon.purpose === 'maskable'));
  assert.ok(served(icons.find(icon => icon.purpose === 'maskable').src).fullBleed);
  const original = icons.find(icon => icon.type === 'image/x-icon');
  assert.equal(original.sizes, '16x16 32x32 48x48', 'the executable\'s own small sizes, never "any"');
  const framework = process.env.COD2_WASM_FRAMEWORK_DIR || path.join(root, '../wasm-game-framework');
  const shell = prepareWebShell(fs.readFileSync(path.join(framework, 'dist/index.html'), 'utf8'));
  const touch = shell.match(/<link rel="apple-touch-icon" href="([^"]+)">/)[1];
  assert.ok(served(touch)?.fullBleed, 'Safari rounds its own corners on a full-bleed icon');
  console.log('PASS: Chrome-installable icons the gateway draws, Safari\'s icon, and an install offer in the main menu\'s ' +
    'right key-prompt place that keeps the game\'s cursor and clicks, prompts once per offer and never shows in a ' +
    'match or an installed window');
})().catch(error => { console.error(error); process.exitCode = 1; });
