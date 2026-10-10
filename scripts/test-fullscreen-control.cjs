// Exercise the real control with browser transitions, denied requests and capture.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const source = fs.readFileSync('site/native-game-adapter.js', 'utf8');
const control = source.slice(source.indexOf('  function startFullscreenControl('),
  source.indexOf('  function startUpdateNotifier('));
assert.ok(control.includes('function startFullscreenControl('));

function browser() {
  const elements = [], docEvents = new Map(), keyEvents = new Map();
  class Element {
    constructor(tag) {
      this.tag = tag; this.isConnected = true; this.attrs = {}; this.events = new Map();
      elements.push(this);
    }
    append(...children) { this.children = children; }
    appendChild(child) { this.append(child); }
    setAttribute(name, value) { this.attrs[name] = value; }
    addEventListener(name, fn) { this.events.set(name, fn); }
    focus(options) { assert.equal(options.preventScroll, true); document.activeElement = this; }
  }
  const canvas = new Element('canvas');
  let requests = 0, exits = 0, resolveEntry, rejectEntry, failure;
  const document = {
    body: new Element('body'), fullscreenElement: null, fullscreenEnabled: true,
    activeElement: canvas, pointerLockElement: null,
    documentElement: new Element('html'),
    addEventListener(name, fn) { docEvents.set(name, fn); },
    createElement: tag => new Element(tag),
    async exitFullscreen() {
      ++exits;
      if (failure === 'exit') throw Error('Exit denied');
      document.fullscreenElement = null;
      docEvents.get('fullscreenchange')();
    }
  };
  document.documentElement.requestFullscreen = options => {
    assert.equal(options.navigationUI, 'hide');
    ++requests;
    if (failure === 'throw') throw Error('Entry denied');
    return new Promise((resolve, reject) => { resolveEntry = resolve; rejectEntry = reject; });
  };
  const window = {addEventListener(name, fn, capture) {
    assert.equal(capture, true, 'shortcut runs before game input'); keyEvents.set(name, fn);
  }};
  const sandbox = vm.createContext({document, window});
  vm.runInContext(control, sandbox);
  sandbox.startFullscreenControl({elements:{canvas}});
  const panel = elements.find(el => el.id === 'cod2-fullscreen-controls');
  const button = elements.find(el => el.id === 'cod2-fullscreen-toggle');
  const label = button.children[0];
  const status = elements.find(el => el.id === 'cod2-fullscreen-status');
  function click(trusted = true) { button.events.get('click')({isTrusted:trusted}); }
  function key(type = 'keydown', options = {}) {
    const event = {key:'F10',isTrusted:true,...options,prevented:false,stopped:false,
      preventDefault() { this.prevented = true; },
      stopImmediatePropagation() { this.stopped = true; }};
    keyEvents.get(type)(event); return event;
  }
  return {document, panel, button, label, status, canvas, click, key,
    requests: () => requests, exits: () => exits,
    focusButton(from = canvas) {
      document.activeElement = from; button.events.get('pointerdown')();
      document.activeElement = button;
    },
    enter() {
      document.fullscreenElement = document.documentElement;
      docEvents.get('fullscreenchange')(); resolveEntry();
    },
    escape() { document.fullscreenElement = null; docEvents.get('fullscreenchange')(); },
    deny() { rejectEntry(Error('Permission denied')); },
    fail(value) { failure = value; }, newElement: tag => new Element(tag)};
}

const settle = async () => { await Promise.resolve(); await Promise.resolve(); };
(async () => {
  const b = browser();
  assert.equal(b.label.textContent, 'Enter fullscreen');
  assert.equal(b.button.attrs['aria-pressed'], 'false');
  assert.equal(b.button.attrs['aria-keyshortcuts'], 'F10');
  assert.equal(b.requests(), 0, 'loading the control never changes display mode');
  b.click(false); assert.equal(b.requests(), 0, 'synthetic input does not enter fullscreen');
  b.focusButton(); b.click(); b.click(); b.key();
  assert.equal(b.requests(), 1, 'rapid clicks and shortcuts share one pending transition');
  assert.equal(b.button.disabled, true);
  b.enter(); await settle();
  assert.equal(b.label.textContent, 'Exit fullscreen');
  assert.equal(b.button.attrs['aria-pressed'], 'true');
  assert.equal(b.button.disabled, false);
  assert.equal(b.document.activeElement, b.canvas, 'button returns keyboard input to the game');
  b.click(); await settle();
  assert.equal(b.exits(), 1); assert.equal(b.label.textContent, 'Enter fullscreen');

  // A captured pointer cannot click a toolbar: the shortcut remains available.
  b.document.pointerLockElement = b.canvas;
  const shortcut = b.key();
  assert.equal(shortcut.prevented && shortcut.stopped, true, 'F10 is consumed before the engine');
  b.key('keydown', {repeat:true}); b.key('keyup');
  assert.equal(b.requests(), 2, 'held keys and key release never toggle again');
  b.enter(); await settle();
  b.escape();
  assert.equal(b.label.textContent, 'Enter fullscreen', 'Esc/browser exits update the visible control');
  assert.equal(b.button.attrs['aria-pressed'], 'false');
  for (const options of [{isTrusted:false},{ctrlKey:true},{altKey:true},{metaKey:true},
    {shiftKey:true},{key:'Enter'}]) assert.equal(b.key('keydown', options).stopped, false);
  assert.equal(b.requests(), 2, 'other shortcuts retain their normal behavior');

  b.focusButton(); b.click(); b.deny(); await settle();
  assert.equal(b.status.hidden, false); assert.match(b.status.textContent, /Try again/);
  assert.equal(b.button.disabled, false, 'a rejected request remains retryable');
  assert.equal(b.label.textContent, 'Enter fullscreen');
  assert.equal(b.document.activeElement, b.canvas);
  b.fail('throw'); b.click(); await settle(); assert.equal(b.button.disabled, false);
  b.fail();
  const input = b.newElement('input');
  b.focusButton(input); b.click(); b.enter(); await settle();
  assert.equal(b.document.activeElement, input, 'startup name editing keeps its input focus');
  assert.equal(b.status.hidden, true, 'a retry clears stale error text');
  b.fail('exit'); b.click(); await settle();
  assert.equal(b.label.textContent, 'Exit fullscreen', 'a denied exit follows actual browser state');
  b.fail(); b.click(); await settle(); assert.equal(b.label.textContent, 'Enter fullscreen');

  for (const type of ['pointerdown','pointerup','pointermove','mousedown','mouseup','mousemove','click','keydown','keyup','keypress']) {
    let stopped = false; b.panel.events.get(type)({stopPropagation() {stopped = true;}});
    assert.equal(stopped, true, `${type} cannot become a shot or native menu input`);
  }
  b.document.fullscreenEnabled = false; b.escape();
  assert.equal(b.button.disabled, true);
  const before = b.requests(); b.click(); b.key(); assert.equal(b.requests(), before);
  console.log('PASS: explicit fullscreen entry/exit, F10 under pointer capture, external exits, pending/denied transitions, focus restoration and isolated toolbar input');
})().catch(error => {console.error(error); process.exitCode = 1;});
