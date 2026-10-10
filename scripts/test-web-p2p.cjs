#!/usr/bin/env node
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const source = fs.readFileSync(path.join(__dirname, '../downstream/wasm/web_p2p.js'), 'utf8');

const sockets = [];
const connections = [];
const workers = [];
const intervals = new Map();
let intervalId = 0;
let now = 0;
let running = 0;
let connected = 1;
let frames = 0;
let pumps = 0;
const logs = [];
class Socket {
  constructor(url) { this.url = String(url); this.readyState = 0; this.bufferedAmount = 0; this.sent = []; this.closed = null; sockets.push(this); }
  send(data) { this.sent.push(typeof data === 'string' ? JSON.parse(data) : [...data]); }
  close(code) { this.readyState = 3; this.closed = code; }
  open() { this.readyState = 1; this.onopen?.(); }
  receive(data) { this.onmessage({ data: typeof data === 'string' ? data : new Uint8Array(data).buffer }); }
  drop(code, reason) { this.readyState = 3; this.onclose({ code, reason }); }
}
Socket.CONNECTING = 0; Socket.OPEN = 1; Socket.CLOSING = 2; Socket.CLOSED = 3;
class Channel {
  constructor(label, options) { this.label = label; this.options = options; this.readyState = 'connecting'; this.sent = []; this.bufferedAmount = 0; }
  send(data) { this.sent.push([...data]); }
  close() { this.readyState = 'closed'; }
  open() { this.readyState = 'open'; this.onopen?.(); }
}
class PeerConnection {
  constructor(config) {
    this.config = config; this.iceGatheringState = 'complete'; this.remote = null; this.candidates = []; this.closed = false;
    connections.push(this);
  }
  createDataChannel(label, options) { this.channel = new Channel(label, options); return this.channel; }
  async createOffer() { return { type: 'offer', sdp: 'offer-sdp' }; }
  async createAnswer() { return { type: 'answer', sdp: 'answer-sdp' }; }
  async setLocalDescription(description) { this.localDescription = description; }
  async setRemoteDescription(description) { this.remote = description; }
  async addIceCandidate(candidate) { this.candidates.push(candidate); }
  addEventListener() {}
  removeEventListener() {}
  close() { this.closed = true; }
}
class TestWorker {
  constructor(url) { this.url = url; this.terminated = false; workers.push(this); }
  terminate() { this.terminated = true; }
}
const Module = {
  _web_local_server_running: () => running,
  _web_client_connected: () => connected,
  _web_background_frame: () => { ++frames; },
  cod2BackgroundPump: () => { ++pumps; }
};
const document = { hidden: false };
const paints = [];
const context = vm.createContext({
  Module, WebSocket: Socket, RTCPeerConnection: PeerConnection, Worker: TestWorker, document, URL, Blob,
  TextEncoder, Uint8Array, ArrayBuffer, String, Array, Object, Number, JSON, Promise, Map, Set,
  location: { href: 'http://cod2.local/', protocol: 'http:', hostname: 'cod2.local' },
  performance: { now: () => now },
  setTimeout, clearTimeout,
  setInterval(callback) { intervals.set(++intervalId, callback); return intervalId; },
  requestAnimationFrame(callback) { paints.push(callback); return paints.length; },
  clearInterval(id) { intervals.delete(id); },
  out(message) { logs.push(message); }, err(message) { logs.push(message); }
});
context.URL.createObjectURL = () => 'blob:ticker';
context.URL.revokeObjectURL = () => {};
vm.runInContext(source, context);
const p2p = Module.cod2P2P;
const settle = async () => { for (let i = 0; i < 8; ++i) await new Promise(resolve => setImmediate(resolve)); };
const bytes = (...values) => new Uint8Array(values);
const text = value => Buffer.from(value).toString('latin1');
const plain = value => JSON.parse(JSON.stringify(value));

(async () => {
  p2p.clientSend(3, bytes(255, 255, 255, 255, 1));
  p2p.clientSend(3, bytes(2));
  const link = sockets[0];
  assert.equal(link.url, 'ws://cod2.local/signal?room=3');
  assert.equal(link.binaryType, 'arraybuffer');
  assert.equal(link.sent.length, 0);
  link.open();
  await settle();
  assert.deepEqual(link.sent.slice(0, 2), [[255, 255, 255, 255, 1], [2]]);
  const offerer = connections[0];
  assert.deepEqual(plain(offerer.config), { iceServers: [{ urls: 'stun:cod2.local:3478' }] });
  assert.deepEqual(plain(offerer.channel.options), { ordered: false, maxRetransmits: 0 });
  assert.deepEqual(link.sent[2], { type: 'offer', sdp: 'offer-sdp' });
  link.receive(JSON.stringify({ type: 'answer', sdp: 'answer-sdp' }));
  link.receive(JSON.stringify({ type: 'candidate', candidate: { candidate: 'candidate:1' } }));
  await settle();
  assert.deepEqual(plain(offerer.remote), { type: 'answer', sdp: 'answer-sdp' });
  assert.deepEqual(plain(offerer.candidates), [{ candidate: 'candidate:1' }]);

  link.receive([9, 8, 7]);
  assert.ok(pumps > 0);
  let next = p2p.clientNext(1024);
  assert.equal(next.slot, 3);
  assert.deepEqual([...next.packet], [9, 8, 7]);
  link.receive([1, 2, 3, 4]);
  assert.equal(p2p.clientNext(2), null);
  p2p.clientSend(3, bytes(3));
  assert.deepEqual(link.sent.at(-1), [3]);
  offerer.channel.open();
  p2p.clientSend(3, bytes(4));
  assert.deepEqual(offerer.channel.sent, [[4]]);
  assert.deepEqual(link.sent.at(-1), [3]);
  offerer.channel.onmessage({ data: new Uint8Array([5, 6]).buffer });
  assert.deepEqual([...p2p.clientNext(1024).packet], [5, 6]);

  link.drop(4000, 'The host left the game.');
  next = p2p.clientNext(1024);
  assert.equal(text(next.packet), '\xff\xff\xff\xfferror\nThe host left the game.\0');
  p2p.clientSend(3, bytes(1));
  assert.equal(sockets.length, 2, 'a consumed failure lets the player retry');
  const retry = sockets[1];
  retry.drop(1006, '');
  assert.equal(text(p2p.clientNext(1024).packet),
    '\xff\xff\xff\xfferror\nThis game is no longer available. Choose another server from Join Game.\0');
  p2p.clientSend(4, bytes(1));
  const other = sockets[2];
  p2p.closeLink();
  assert.equal(other.closed, 1000);
  assert.equal(p2p.clientNext(1024), null);

  p2p.startHost();
  const lobby = sockets[3];
  assert.equal(lobby.url, 'ws://cod2.local/signal?role=host');
  assert.equal(workers.length, 1);
  lobby.open();
  lobby.receive(JSON.stringify({ type: 'hosted', slot: 2 }));
  lobby.receive(JSON.stringify({ type: 'join', peer: 1 }));
  lobby.receive([0, 1, 255, 255, 255, 255, 7]);
  lobby.receive([0, 2, 1]);
  next = p2p.hostNext(1024);
  assert.equal(next.peer, 1);
  assert.deepEqual([...next.packet], [255, 255, 255, 255, 7]);
  assert.equal(p2p.hostNext(1024), null, 'datagrams from unknown players are ignored');
  p2p.hostSend(1, bytes(4, 5));
  assert.deepEqual(lobby.sent.at(-1), [0, 1, 4, 5]);

  lobby.receive(JSON.stringify({ type: 'offer', peer: 1, sdp: 'offer-sdp' }));
  await settle();
  const answerer = connections.at(-1);
  assert.deepEqual(plain(answerer.remote), { type: 'offer', sdp: 'offer-sdp' });
  assert.deepEqual(lobby.sent.at(-1), { type: 'answer', peer: 1, sdp: 'answer-sdp' });
  lobby.receive(JSON.stringify({ type: 'candidate', peer: 1, candidate: { candidate: 'candidate:2' } }));
  await settle();
  assert.deepEqual(plain(answerer.candidates), [{ candidate: 'candidate:2' }]);
  const channel = new Channel('cod2', {});
  answerer.ondatachannel({ channel });
  channel.open();
  p2p.hostSend(1, bytes(6));
  assert.deepEqual(channel.sent, [[6]]);
  channel.onmessage({ data: new Uint8Array([7]).buffer });
  assert.deepEqual([...p2p.hostNext(1024).packet], [7]);

  running = 1;
  now = 5000;
  const tick = [...intervals.values()].at(-1);
  tick();
  next = p2p.hostNext(1024);
  assert.equal(next.peer, 0xffff);
  assert.equal(text(next.packet), '\xff\xff\xff\xffgetinfo cod2-browser');
  p2p.hostSend(0xffff, new Uint8Array(Buffer.from('\xff\xff\xff\xffinfoResponse\n\\hostname\\Patio\\clients\\2\0', 'latin1')));
  tick();
  assert.deepEqual(lobby.sent.at(-1), { type: 'info', info: '\\hostname\\Patio\\clients\\2' });
  const sentCount = lobby.sent.length;
  now = 6000;
  tick();
  assert.equal(lobby.sent.length, sentCount, 'unchanged room info is not resent');

  const ticker = workers[0];
  paints.shift()();
  ticker.onmessage();
  assert.equal(frames, 0, 'animation frames already drive the visible host');
  now = 6100;
  paints.shift()();
  now = 6150;
  ticker.onmessage();
  assert.equal(frames, 0);
  now = 6260;
  ticker.onmessage();
  now = 6270;
  ticker.onmessage();
  assert.equal(frames, 1, 'a host without animation frames keeps its server ticking at a bounded rate');
  now = 6310;
  ticker.onmessage();
  assert.equal(frames, 2);

  lobby.receive(JSON.stringify({ type: 'leave', peer: 1 }));
  assert.equal(channel.readyState, 'closed');
  assert.ok(answerer.closed);
  p2p.hostSend(1, bytes(1));
  assert.deepEqual(lobby.sent.at(-1), { type: 'info', info: '\\hostname\\Patio\\clients\\2' });

  running = 0;
  now = 60000;
  tick();
  assert.equal(lobby.closed, null, 'a map change keeps the room while the host client reloads');
  running = 1;
  now = 61000;
  tick();
  running = 0;
  connected = 0;
  now = 63500;
  tick();
  assert.equal(lobby.closed, null);
  now = 64500;
  tick();
  assert.equal(lobby.closed, 1000, 'a host that left the match unregisters its room');
  assert.ok(ticker.terminated);
  assert.equal(p2p.hostNext(1024), null);
  console.log('PASS: P2P players relay until their data channel opens, report host departure, retry, and hosts answer, publish info, tick hidden and unregister');
})().catch(error => { console.error(error); process.exit(1); });
