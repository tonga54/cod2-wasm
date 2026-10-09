const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');

class Element {
  constructor(tag) { this.tag = tag; this.children = []; this.listeners = {}; }
  setAttribute() {}
  append(...children) { this.children.push(...children); }
  appendChild(child) { this.append(child); }
  addEventListener(type, fn) { this.listeners[type] = fn; }
}
const body = new Element('body');
const listeners = {};
const old = 'a'.repeat(64), deployed = 'b'.repeat(64);
let current = old, available = false, notify = false, reloads = 0, poll, offline = false;
const document = { body, hidden: false, createElement: tag => new Element(tag),
  addEventListener: (type, fn) => { listeners[type] = fn; }, removeEventListener() {} };
const sandbox = { document, AbortSignal,
  location: { reload: () => { ++reloads; } },
  setInterval: fn => { poll = fn; return 1; }, clearInterval() {}, addEventListener() {},
  fetch: async url => {
    if (offline) throw new Error('offline');
    return { ok: true, json: async () => url === '/build-info.json' ? { buildId: current } :
      { current: { buildId: current }, update: { available, revision: 'c'.repeat(40) } } };
  }
};
vm.createContext(sandbox);
vm.runInContext(fs.readFileSync('site/update-notifier.js', 'utf8'), sandbox);
(async () => {
  const notifier = sandbox.createCod2UpdateNotifier({ canNotify: () => notify });
  await notifier.ready;
  const panel = body.children[0], message = panel.children[0];
  const [action, help, later] = panel.children[1].children;
  assert.equal(panel.hidden, true);
  available = true; await poll();
  assert.equal(panel.hidden, true, 'never display during a match/loading');
  notify = true; notifier.render();
  assert.equal(panel.hidden, false);
  assert.match(message.textContent, /anfitrión/);
  assert.equal(action.hidden, true, 'a push is not a deployed build');
  assert.equal(help.hidden, false);
  later.listeners.click(); assert.equal(panel.hidden, true);
  await poll(); assert.equal(panel.hidden, true, 'dismissal lasts for this version');
  current = deployed; await poll();
  assert.equal(panel.hidden, false, 'deployed version supersedes dismissed repo notice');
  assert.equal(action.hidden, false);
  assert.equal(help.hidden, true);
  assert.equal(reloads, 0, 'updates never auto-reload');
  notify = false;
  action.listeners.click(); assert.equal(reloads, 0, 'starting a match prevents stale reload clicks');
  notifier.render(); assert.equal(panel.hidden, true);
  notify = true; notifier.render(); action.listeners.click(); assert.equal(reloads, 1);
  offline = true; await poll(); assert.equal(reloads, 1, 'network errors leave game alone');
  offline = false;
  available = false; current = old; await poll(); assert.equal(panel.hidden, true);
  console.log('PASS: deferred notices, push versus deployment, dismissal, explicit reload, active-match guard, offline behavior');
})().catch(error => { console.error(error); process.exitCode = 1; });
