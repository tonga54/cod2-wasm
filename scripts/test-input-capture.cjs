// Isolated adapter contract test; no game or browser is substituted at runtime.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const frameworkSource = fs.readFileSync(process.argv[2] ||
  path.join(__dirname, '../../wasm-game-framework/dist/wasm-game-framework.js'), 'utf8');
const guardStart = frameworkSource.indexOf('    function protectCapturedKey(');
const guardEnd = frameworkSource.indexOf('    function refreshAuthoritativeState(', guardStart);
assert.ok(guardStart >= 0 && guardEnd > guardStart, 'pinned framework key guard is present');
const makeKeyGuard = Function('ENGINE_STATES', 'engineState', 'inputCaptured', 'config',
  frameworkSource.slice(guardStart, guardEnd) + 'return protectCapturedKey;');
const listeners = new Map();
const canvasListeners = new Map();
const windowListeners = new Map();
const canvas = {addEventListener(name, fn) { canvasListeners.set(name, fn); }};
let escapes = 0;
let starts = 0;
let loadingShown = 0;
let nativeState = 2;
let fullscreenRequests = 0;
let shellState = 'launcher';
const publishedStates = [];
const loadingMessages = [];
const window = {
  addEventListener(name, fn, capture) {
    assert.equal(capture, true, 'input state must refresh before the document key guard');
    windowListeners.set(name, fn);
  }
};
const document = {
  pointerLockElement: null,
  fullscreenElement: null,
  documentElement: {requestFullscreen(options) {
    assert.equal(options.navigationUI, 'hide');
    ++fullscreenRequests;
    return Promise.resolve();
  }},
  createElement: () => ({}),
  head: {appendChild(style) {
    assert.equal(style.rel, 'stylesheet');
    assert.equal(style.href, '/startup.css');
  }},
  addEventListener(name, fn) {
    if (!listeners.has(name)) listeners.set(name, []);
    listeners.get(name).push(fn);
  }
};
const native = {
  FS: { mkdirTree() {}, chdir() {} },
  callMain(args) {
    ++starts;
    assert.ok(!args.includes('+connect'), 'automatic launch stops at the native main menu');
    assert.ok(args.includes('ui_netSource'), 'Join Game defaults to LAN discovery');
    assert.ok(!args.includes('name'), 'player identity belongs to the native game settings');
  },
  _web_client_state: () => nativeState,
  _web_capture_lost: () => { ++escapes; }
};
const sandbox = {
  document, window, location: {search: ''}, URLSearchParams,
  addEventListener() {},
  crypto: {subtle: {}},
  fetch: async () => ({ok: true, json: async () => ({variants: {test: {files: [
    {key:'saved',size:2000000}, {key:'large',size:18000000}
  ]}}})}),
  createCod2Client: async options => {
    assert.equal(options.stdin(), null, 'browser stdin must not open a terminal prompt');
    return native;
  }
};
vm.createContext(sandbox);
vm.runInContext(fs.readFileSync('site/native-game-adapter.js', 'utf8'), sandbox);
const adapter = sandbox.WasmGameAdapter;
const context = {
  variant: 'test', config: {autoStart: true}, elements: {canvas, console: {hidden: false},
    loading: {classList: {add(name) { assert.equal(name, 'cod2-startup'); }}},
    loadingTitle: {replaceChildren(logo) {assert.equal(logo.src, '/game-data/files/cod2-startup/original.png');}},
    loadingProgress: {setAttribute(name, value) {assert.equal(name, 'aria-label');assert.ok(value);}},
    loadingStatus: {}}, log() {},
  framework: {createOwnerDataSet: x => ({policies:x.files}), mountOwnerFiles: async (native,assets,options) => {
    options.onProgress({phase:'mounting',copied:10000000,total:20000000});
    options.onProgress({phase:'mounted',copied:20000000,total:20000000});
  }},
  dataClient: {applyGate: async () => ({ready:true}), load: async (data,options) => {
    options.onProgress({phase:'restored',key:'saved',bytes:2000000,total:2});
    assert.equal(loadingMessages.at(-1)[2],10,'restored files count toward available bytes');
    options.onProgress({phase:'downloading',key:'large',received:9000000,total:18000000});
    assert.deepEqual(loadingMessages.at(-1),['Descargando archivos… 55%','11,0 / 20,0 MB',55],
      'percentage follows received bytes and manifest sizes, not the file count');
    options.onProgress({phase:'downloading',key:'large',received:8000000,total:0});
    assert.equal(loadingMessages.at(-1)[2],55,'duplicate or stale progress never moves backward');
    options.onProgress({phase:'validated',key:'large',bytes:18000000,total:2});
    options.onProgress({phase:'cached',key:'large',bytes:18000000,total:2});
    assert.equal(loadingMessages.at(-1)[2],100,'validation/cache events do not double-count bytes');
    return [];
  }},
  preferences: {values: () => ({playerName: 'Test'})},
  shell: {engineState: () => shellState},
  setEngineState(state) { shellState = state; publishedStates.push(state); },
  setLoading(...values) {loadingMessages.push(values);}, showRuntime() {}, showLoading() { ++loadingShown; }
};
(async () => {
  await adapter.init(context);
  assert.equal(starts, 1, 'initialization starts the actual engine without a Play click');
  assert.ok(loadingMessages.some(([message,,percent])=>message==='Preparando el juego… 50%' && percent===50),
    'preparation has its own measured progress after downloading');
  const fullscreenClick = canvasListeners.get('pointerdown');
  fullscreenClick({isTrusted:false,button:0});
  fullscreenClick({isTrusted:true,button:2});
  assert.equal(fullscreenRequests, 0, 'only an actual primary click requests fullscreen');
  fullscreenClick({isTrusted:true,button:0});
  fullscreenClick({isTrusted:true,button:0});
  assert.equal(fullscreenRequests, 1, 'concurrent clicks share the pending request');
  await Promise.resolve(); await Promise.resolve(); await Promise.resolve();
  document.fullscreenElement = document.documentElement;
  fullscreenClick({isTrusted:true,button:0});
  assert.equal(fullscreenRequests, 1, 'clicks in fullscreen do not request it again');
  document.fullscreenElement = null;
  assert.equal(loadingShown, 1);
  assert.equal(context.elements.console.hidden, true, 'diagnostic output is hidden during normal loading');
  for (const captured of [false, true]) {
    document.pointerLockElement = captured ? canvas : null;
    let prevented = false;
    canvasListeners.get('contextmenu')({preventDefault() { prevented = true; }});
    assert.equal(prevented, true, 'right-click must not open Save Image, with or without pointer lock');
  }
  assert.equal(canvasListeners.has('mousedown'), false, 'native right-button input stays available');
  // Console/chat/menu key catchers change the authoritative native state
  // without a mouse event. The next key must reach SDL text input even while
  // the pointer was captured; gameplay must still protect its owned keys.
  publishedStates.length = 0;
  for (const captured of [false, true]) {
    document.pointerLockElement = captured ? canvas : null;
    for (const state of [2, 1, 1, 2]) {
      nativeState = state;
      windowListeners.get('keydown')();
      let prevented = false;
      makeKeyGuard({GAMEPLAY:'gameplay'}, shellState, () => captured, {})({
        key:' ', preventDefault() { prevented = true; }
      });
      assert.equal(prevented, captured && state === 2,
        'Space is protected for gameplay and stays printable in the console');
    }
  }
  assert.deepEqual(publishedStates, ['gameplay', 'menu', 'gameplay', 'menu', 'gameplay'],
    'publish native transitions once, including returning from the console');
  nativeState = 1;
  shellState = 'gameplay';
  windowListeners.get('keydown')();
  assert.equal(shellState, 'menu', 'repair shell state changed by another input listener');
  // Restore gameplay for the following pointer-capture lifecycle checks.
  nativeState = 2;
  function change(captured) {
    document.pointerLockElement = captured ? canvas : null;
    // Match the framework listener order: notify before adapter listeners.
    if (!captured) adapter.captureLost();
    for (const callback of listeners.get('pointerlockchange')) callback();
  }
  change(false);
  assert.equal(escapes, 0, 'a rejected initial request is not a capture loss');
  change(true);
  change(false);
  assert.equal(escapes, 1, 'losing a real pointer lock opens the native menu');
  change(false);
  assert.equal(escapes, 1, 'duplicate loss events do not inject another Escape');
  change(true);
  change(false);
  assert.equal(escapes, 2, 'capture can be reacquired and released');
  console.log('PASS: byte-weighted download/cache and preparation progress, automatic launch, console/chat Space state transitions, fullscreen and pointer-lock lifecycle');
})().catch(error => { console.error(error); process.exitCode = 1; });
