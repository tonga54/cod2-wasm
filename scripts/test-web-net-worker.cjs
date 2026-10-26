#!/usr/bin/env node
// Exercise the worker transport with the actual EM_JS open, receive and send
// callbacks and the worker's own code, over simulated worker and socket links.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const source = fs.readFileSync(path.join(__dirname, '../downstream/wasm/web_net.c'), 'utf8');
function body(signature) {
  const start = source.indexOf(signature);
  assert.ok(start >= 0, signature);
  return source.slice(source.indexOf('{', start + signature.length - 1) + 1, source.indexOf('\n});', start));
}
const openBody = body('EM_JS(void, WebNet_Open, (int room), {');
const receiveBody = body('EM_JS(int, WebNet_Receive, (void *destination, int capacity, int *room), {');
const sendBody = body('EM_JS(void, WebNet_Send, (const void *data, int length), {');
const RING = 262144;
const tick = async (rounds = 4) => { for (let i = 0; i < rounds; ++i) await new Promise(resolve => setImmediate(resolve)); };

function fixture({isolated = true, workerThrows = false} = {}) {
  const blobs = new Map(), revoked = [], workers = [], direct = [], pumps = [];
  const listeners = {};
  const document = {hidden: false, addEventListener(type, fn) { (listeners[type] ||= []).push(fn); }};
  class TestURL extends URL {
    static createObjectURL(blob) { const url = 'blob:test/' + blobs.size; blobs.set(url, blob); return url; }
    static revokeObjectURL(url) { revoked.push(url); }
  }
  class Blob { constructor(parts, options) { this.parts = parts; this.type = options.type; } }
  // The page's own socket, for the fallback.
  class PageSocket {
    constructor(url) { this.url = String(url); this.readyState = 0; this.bufferedAmount = 0; this.sent = []; direct.push(this); }
    send(packet) { this.sent.push(new Uint8Array(packet)); }
    close(code) { this.readyState = 3; this.closedWith = code; }
  }
  PageSocket.CONNECTING = 0; PageSocket.OPEN = 1;
  // A dedicated worker: the blob's source runs in its own global, and
  // messages cross asynchronously in each direction until it is terminated.
  class Worker {
    constructor(url) {
      if (workerThrows) throw new Error('Refused by the content security policy');
      const blob = blobs.get(url);
      assert.ok(blob, 'the worker runs a blob made for it');
      assert.equal(blob.type, 'text/javascript');
      this.url = url; this.terminated = false; this.sockets = [];
      const worker = this;
      class ServerSocket {
        constructor(href) { this.url = href; this.readyState = 0; this.bufferedAmount = 0; this.sent = []; worker.sockets.push(this); }
        send(packet) { this.sent.push(new Uint8Array(packet)); }
        close(code) { this.readyState = 3; this.closedWith = code; this.onclose?.({code, reason: ''}); }
        open() { this.readyState = 1; this.onopen?.(); }
        deliver(bytes) {
          const buffer = new this.realm.ArrayBuffer(bytes.length);
          new Uint8Array(buffer).set(bytes);
          this.onmessage?.({data: buffer});
        }
      }
      this.global = {WebSocket: ServerSocket, Atomics, Math,
        postMessage: data => setImmediate(() => { if (!worker.terminated) worker.onmessage?.({data}); })};
      this.global.self = this.global;
      vm.createContext(this.global);
      ServerSocket.prototype.realm = {ArrayBuffer: vm.runInContext('ArrayBuffer', this.global)};
      vm.runInContext(blob.parts.join(''), this.global);
      workers.push(this);
    }
    postMessage(data) { setImmediate(() => { if (!this.terminated) this.global.onmessage({data}); }); }
    terminate() { this.terminated = true; }
  }
  const Module = {cod2BackgroundPump: () => pumps.push(document.hidden)};
  const heap = new Uint8Array(1 << 17), heap32 = new Int32Array(heap.buffer);
  const context = vm.createContext({Module, WebSocket: PageSocket, URL: TestURL, Blob, TextEncoder, Uint8Array,
    ArrayBuffer, AbortController, setTimeout, clearTimeout, document, SharedArrayBuffer, Atomics, Math,
    crossOriginIsolated: isolated, Worker: workerThrows || isolated ? Worker : undefined,
    location: {href: 'https://tonga-cod2.example/', protocol: 'https:'}, out() {}, err() {},
    fetch: async () => ({ok: true, json: async () => ({rooms: [{id: 0, players: 1}]})}),
    HEAPU8: heap, HEAP32: heap32});
  vm.runInContext('function open(room) {' + openBody + '\n}\n' +
    'function receive(destination, capacity, room) {' + receiveBody + '\n}\n' +
    'function send(data, length) {' + sendBody + '\n}', context);
  const pageArrayBuffer = vm.runInContext('ArrayBuffer', context);
  return {Module, workers, direct, revoked, document, listeners, pumps, heap, heap32, context,
    open: room => context.open(room),
    state: () => Module.cod2Transport,
    // One packet the frame takes, or null.
    take(capacity = 65536) {
      const size = context.receive(0x1000, capacity, 0x10);
      return size ? {bytes: heap.slice(0x1000, 0x1000 + size), room: heap32[0x10 >> 2]} : null;
    },
    pagePacket(bytes) { const buffer = new pageArrayBuffer(bytes.length); new Uint8Array(buffer).set(bytes); return buffer; },
    hide(hidden) { document.hidden = hidden; for (const fn of listeners.visibilitychange || []) fn(); }};
}
const packet = (n, seed) => Uint8Array.from({length: n}, (_, i) => (i * 31 + seed) & 255);

(async () => {
  // A page that can share memory receives through a worker; its socket is
  // the one the gateway room names.
  const f = fixture();
  f.open(2);
  const state = f.state();
  assert.ok(state.socket.ring, 'the transport reads a shared ring');
  assert.equal(f.workers.length, 1);
  assert.match(f.workers[0].url, /^blob:/);
  await tick();
  const worker = f.workers[0], server = worker.sockets[0];
  assert.equal(server.url, 'wss://tonga-cod2.example/game?room=2');
  assert.equal(state.socket.readyState, 0);

  // Commands made while it connects wait in the page, then go out in order.
  f.heap.set([9, 8, 7, 6], 0x200);
  f.context.send(0x200, 4);
  f.context.send(0x201, 2);
  assert.equal(state.sends.length, 2);
  server.open();
  await tick();
  assert.equal(state.socket.readyState, 1);
  assert.equal(state.opened, true);
  assert.deepEqual(server.sent.map(p => [...p]), [[9, 8, 7, 6], [8, 7]]);
  assert.deepEqual(f.revoked, [worker.url], 'the blob is let go once the worker runs');

  // The frame takes the gateway's packets in order, tagged with the room,
  // without the page receiving a single message: the worker writes them.
  const sent = [packet(1, 1), packet(1400, 2), packet(37, 3)];
  for (const bytes of sent) server.deliver(bytes);
  for (const bytes of sent) {
    const taken = f.take();
    assert.deepEqual([...taken.bytes], [...bytes]);
    assert.equal(taken.room, 2);
  }
  assert.equal(f.take(), null, 'nothing more to take');

  // Packets keep their bytes across the ring's end, many times over.
  let seed = 10;
  for (let round = 0; round < 700; ++round) {
    const sizes = [1000 + (round * 97) % 2000, 1 + round % 13, 4093];
    const batch = sizes.map(n => packet(n, seed++));
    for (const bytes of batch) server.deliver(bytes);
    for (const bytes of batch) assert.deepEqual([...f.take().bytes], [...bytes], `round ${round}`);
  }
  assert.equal(f.take(), null);
  const meta = state.socket.ring.meta;
  assert.ok((Atomics.load(meta, 0) >>> 0) > 4 * RING, 'the ring wrapped several times');

  // A full ring drops what no longer fits and keeps what came first; once the
  // frame catches up, packets flow again.
  const backlog = [];
  for (let i = 0; i < 300; ++i) { const bytes = packet(1000, 50 + i); backlog.push(bytes); server.deliver(bytes); }
  const kept = Math.floor(RING / 1004);
  assert.equal(Atomics.load(meta, 2), 300 - kept);
  for (let i = 0; i < kept; ++i) assert.deepEqual([...f.take().bytes], [...backlog[i]]);
  assert.equal(f.take(), null);
  server.deliver(packet(5, 99));
  assert.deepEqual([...f.take().bytes], [...packet(5, 99)]);

  // A packet larger than the frame's buffer is skipped and counted; the next
  // one still arrives.
  server.deliver(packet(3000, 1));
  server.deliver(packet(20, 2));
  assert.deepEqual([...f.take(2048).bytes], [...packet(20, 2)]);
  assert.equal(state.dropped, 1);

  // The ring's positions run on across 32 bits.
  Atomics.store(meta, 0, 0x7ffffff0);
  Atomics.store(meta, 1, 0x7ffffff0);
  for (let i = 0; i < 40; ++i) server.deliver(packet(500 + i, i));
  for (let i = 0; i < 40; ++i) assert.deepEqual([...f.take().bytes], [...packet(500 + i, i)]);
  assert.ok(Atomics.load(meta, 0) < 0, 'past 2^31');
  Atomics.store(meta, 0, -8);
  Atomics.store(meta, 1, -8);
  server.deliver(packet(64, 7));
  assert.deepEqual([...f.take().bytes], [...packet(64, 7)], 'and past 2^32');

  // Commands are copied as sent: the engine's buffer may change at once. An
  // uplink holding more than 8 KiB drops them, as UDP would.
  f.heap.set([1, 2, 3, 4, 5], 0x300);
  f.context.send(0x300, 5);
  f.heap.fill(0, 0x300, 0x305);
  await tick();
  assert.deepEqual([...server.sent.at(-1)], [1, 2, 3, 4, 5]);
  server.bufferedAmount = 8193;
  const before = server.sent.length, drops = Atomics.load(meta, 2);
  f.context.send(0x300, 5);
  await tick();
  assert.equal(server.sent.length, before);
  assert.equal(Atomics.load(meta, 2), drops + 1);
  server.bufferedAmount = 0;

  // A hidden page's frames are driven by the worker's packets; a visible
  // page's are not.
  server.deliver(packet(10, 1));
  await tick();
  assert.deepEqual(f.pumps, []);
  f.hide(true);
  server.deliver(packet(10, 2));
  server.deliver(packet(10, 3));
  await tick();
  assert.deepEqual(f.pumps, [true, true]);
  f.hide(false);
  server.deliver(packet(10, 4));
  await tick();
  assert.equal(f.pumps.length, 2);
  while (f.take());

  // The gateway closing a live connection reports it through the engine's
  // packet loop, ahead of anything left in the ring.
  server.deliver(packet(30, 5));
  server.close(1006);
  await tick(8);
  assert.equal(state.socket.readyState, 3);
  assert.equal(worker.terminated, true);
  const error = f.take();
  assert.equal(new TextDecoder().decode(error.bytes.slice(4)), 'error\nEXE_SERVER_DISCONNECTED\0');
  assert.equal(f.take(), null, 'the closed connection leaves nothing behind');

  // Leaving a room closes the worker's socket and ends the worker.
  const leave = fixture();
  leave.open(0);
  await tick();
  leave.workers[0].sockets[0].open();
  await tick();
  leave.state().stopped = true;
  leave.state().socket.close(1000);
  await tick(8);
  assert.equal(leave.workers[0].sockets[0].closedWith, 1000);
  assert.equal(leave.workers[0].terminated, true);
  assert.equal(leave.state().packets.length, 0);

  // A worker that never reaches the gateway hands over to the page's own
  // socket, whose packets take the page's queue; sends and the uplink's
  // backlog follow it.
  const handover = fixture();
  handover.open(1);
  await tick();
  handover.workers[0].sockets[0].close(1006);
  await tick(8);
  assert.equal(handover.workers[0].terminated, true);
  assert.equal(handover.direct.length, 1);
  const page = handover.direct[0];
  assert.equal(page.url, 'wss://tonga-cod2.example/game?room=1');
  assert.equal(handover.state().socket.readyState, 0, 'still connecting: no error yet');
  page.readyState = 1;
  page.onopen();
  assert.equal(handover.state().opened, true);
  page.onmessage({data: handover.pagePacket(packet(12, 4))});
  assert.deepEqual([...handover.take().bytes], [...packet(12, 4)]);
  handover.heap.set([4, 3, 2], 0x400);
  handover.context.send(0x400, 3);
  assert.deepEqual([...page.sent[0]], [4, 3, 2]);
  page.bufferedAmount = 9000;
  handover.context.send(0x400, 3);
  assert.equal(page.sent.length, 1);
  assert.equal(handover.state().dropped, 1);
  page.readyState = 3;
  await page.onclose({code: 1006, reason: ''});
  await tick(8);
  assert.equal(new TextDecoder().decode(handover.take().bytes.slice(4)), 'error\nEXE_SERVER_DISCONNECTED\0');

  // A worker the page may not start, or a page that cannot share memory,
  // connects directly as before.
  for (const options of [{workerThrows: true}, {isolated: false}]) {
    const plain = fixture(options);
    plain.open(0);
    assert.equal(plain.direct.length, 1, JSON.stringify(options));
    assert.equal(plain.state().socket, plain.direct[0]);
    assert.equal(plain.state().socket.ring, undefined);
    assert.equal(plain.workers.length, 0);
  }
  console.log('PASS: the worker transport takes packets in order and intact across the ring\'s end and 32-bit ' +
    'positions, drops what a full ring or the frame cannot hold, copies commands, drops them behind a stalled ' +
    'uplink, wakes only hidden pages, reports a closed connection through the packet loop, ends with the room, ' +
    'hands a failed worker over to the page\'s socket and keeps the page\'s socket without shared memory');
})().catch(error => { console.error(error); process.exitCode = 1; });
