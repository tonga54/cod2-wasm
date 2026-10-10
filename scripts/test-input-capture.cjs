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
const canvas = {focus() { document.activeElement = canvas; }, addEventListener(name, fn) { canvasListeners.set(name, fn); }};
let escapes = 0;
let starts = 0;
let loadingShown = 0;
let nativeState = 2;
let fullscreenRequests = 0;
let keyboardCaptureRequests = 0;
let clockMs = Date.now();
const chatCharacters = [];
let shellState = 'launcher';
const publishedStates = [];
const loadingMessages = [];
const window = {
  addEventListener(name, fn, capture) {
    assert.equal(capture, true, 'input state must refresh before the document key guard');
    windowListeners.set(name, fn);
  }
};
class Element {
  constructor() { this.listeners = new Map(); }
  append(...children) { this.children = children; }
  setAttribute(name, value) { this[name] = value; }
  addEventListener(name, fn) { this.listeners.set(name, fn); }
}
const document = {
  pointerLockElement: null,
  fullscreenElement: null,
  documentElement: {requestFullscreen(options) {
    assert.equal(options.navigationUI, 'hide');
    ++fullscreenRequests;
    return Promise.resolve();
  }},
  createElement: () => new Element(),
  body: {appendChild() {}},
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
    assert.equal(args[args.indexOf('name') + 1], '"Test"', 'gametag reaches the native player name');
  },
  _web_client_state: () => nativeState,
  _web_chat_char: code => chatCharacters.push(code),
  _web_capture_lost: () => { ++escapes; }
};
const sandbox = {
  document, window, location: {search: ''}, URLSearchParams, TextEncoder,
  Date: {now: () => clockMs},
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
    assert.deepEqual(loadingMessages.at(-1),['Downloading files… 55%','11.0 / 20.0 MB',55],
      'percentage follows received bytes and manifest sizes, not the file count');
    options.onProgress({phase:'downloading',key:'large',received:8000000,total:0});
    assert.equal(loadingMessages.at(-1)[2],55,'duplicate or stale progress never moves backward');
    options.onProgress({phase:'validated',key:'large',bytes:18000000,total:2});
    options.onProgress({phase:'cached',key:'large',bytes:18000000,total:2});
    assert.equal(loadingMessages.at(-1)[2],100,'validation/cache events do not double-count bytes');
    return [];
  }},
  preferences: {values: () => ({playerName: 'Test'})},
  shell: {engineState: () => shellState, requestInputCapture(event) {
    assert.equal(event.isTrusted,true); ++keyboardCaptureRequests;
  }},
  setEngineState(state) { shellState = state; publishedStates.push(state); },
  setLoading(...values) {loadingMessages.push(values);}, showRuntime() {}, showLoading() { ++loadingShown; }
};
(async () => {
  await adapter.init(context);
  assert.equal(starts, 1, 'initialization starts the actual engine without a Play click');
  assert.ok(loadingMessages.some(([message,,percent])=>message==='Preparing the game… 50%' && percent===50),
    'preparation has its own measured progress after downloading');
  assert.equal(canvasListeners.has('pointerdown'), false, 'game clicks must not change fullscreen');
  assert.equal(fullscreenRequests, 0, 'engine startup must not request fullscreen');
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
    for (const state of [2, 3, 3, 1, 1, 2]) {
      nativeState = state;
      windowListeners.get('keydown')({key:' '});
      let prevented = false;
      makeKeyGuard({GAMEPLAY:'gameplay'}, shellState, () => captured, {})({
        key:' ', preventDefault() { prevented = true; }
      });
      assert.equal(prevented, captured && (state === 2 || state === 3),
        'captured gameplay/chat protect browser keys; chat explicitly forwards printable text');
    }
  }
  assert.deepEqual(publishedStates, ['gameplay', 'menu', 'gameplay', 'menu', 'gameplay'],
    'publish native transitions once, including returning from the console');
  assert.deepEqual(chatCharacters, [32,32], 'captured chat forwards each Space once');
  nativeState = 3;
  document.pointerLockElement = canvas;
  for (const event of [{key:'/'},{key:'a'},{key:'?',shiftKey:true},{key:' ',ctrlKey:true},
    {key:' ',metaKey:true},{key:'/',altKey:true}]) windowListeners.get('keydown')(event);
  assert.deepEqual(chatCharacters, [32,32,47], 'only shell-cancelled printable keys are forwarded');
  assert.equal(adapter.readCaptureIntent(),true,'in-game chat retains the mouse capture intent');
  nativeState = 1;
  assert.equal(adapter.readCaptureIntent(),false,'menus and console release the pointer');
  shellState = 'gameplay';
  windowListeners.get('keydown')({key:' '});
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
  change(true);
  nativeState = 3;
  windowListeners.get('keydown')({key:'Escape',isTrusted:true});
  nativeState = 2;
  change(false);
  assert.equal(escapes,2,'chat cancellation must not inject a second Escape that opens the menu');
  windowListeners.get('keydown')({key:'w',isTrusted:true});
  assert.equal(keyboardCaptureRequests,1,'the next game key requests capture without a click');
  for (const event of [{key:'w'}, {key:'w',isTrusted:true,ctrlKey:true}, {key:'Escape',isTrusted:true}])
    windowListeners.get('keydown')(event);
  nativeState = 1;
  windowListeners.get('keydown')({key:'w',isTrusted:true});
  assert.equal(keyboardCaptureRequests,1,'menus, shortcuts, Escape and synthetic keys do not acquire capture');
  nativeState = 2;
  clockMs += 2000;
  windowListeners.get('keydown')({key:'Enter',isTrusted:true,target:{closest:()=>true}});
  assert.equal(keyboardCaptureRequests,1,'fullscreen toolbar keys must not acquire game input');
  console.log('PASS: byte-weighted download/cache and preparation progress, automatic launch, console/chat Space and slash input, retained chat capture, explicit fullscreen and pointer-lock lifecycle');
})().catch(error => { console.error(error); process.exitCode = 1; });
