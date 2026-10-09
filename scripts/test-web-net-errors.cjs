#!/usr/bin/env node
// Exercise the actual EM_JS transport callbacks without a game or occupied LAN slots.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const source = fs.readFileSync(path.join(__dirname, '../downstream/wasm/web_net.c'), 'utf8');
const start = source.indexOf('EM_JS(void, WebNet_Open, (int room), {');
assert.ok(start >= 0);
const body = source.slice(source.indexOf('{', start) + 1, source.indexOf('\n});', start));
function fixture(rooms, fetchFails = false) {
  const sockets = [];
  class Socket {
    constructor() { this.readyState = 0; this.sent = []; sockets.push(this); }
    close() { this.readyState = 3; }
    send(packet) { this.sent.push(packet); }
  }
  const Module = {};
  const context = vm.createContext({Module, WebSocket:Socket, URL, TextEncoder, Uint8Array,
    ArrayBuffer, AbortController, setTimeout, clearTimeout,
    location:{href:'http://192.168.1.10:8088/', protocol:'http:'}, out(){}, err(){},
    fetch:async () => {
      if (fetchFails) throw new Error('Network unavailable');
      return {ok:true, json:async()=>({rooms})};
    }});
  vm.runInContext('function open(room) {' + body + '\n}', context);
  function error() {
    const state = Module.cod2Transport;
    assert.equal(state.packets.length, 1);
    assert.deepEqual([...state.packets[0].slice(0, 4)], [255,255,255,255]);
    assert.equal(state.bytes, state.packets[0].length);
    return new TextDecoder().decode(state.packets[0].slice(4));
  }
  return {Module, sockets, open:context.open, error};
}
async function reject(f) {
  f.open(0);
  const socket = f.sockets[0];
  socket.readyState = 3;
  const closed = socket.onclose({code:1006, reason:''});
  f.open(0); // Frame retries must not erase the pending native error.
  assert.equal(f.sockets.length, 1);
  await closed;
}
(async()=>{
  const full = fixture([{id:0,players:2}]);
  await reject(full);
  assert.equal(full.error(), 'error\nEXE_SERVERISFULL\0');
  const loading = fixture([{id:0,players:0,connections:2}]);
  await reject(loading);
  assert.equal(loading.error(), 'error\nEXE_SERVERISFULL\0');
  const gone = fixture([]);
  await reject(gone);
  assert.match(gone.error(), /no longer available/);
  const offline = fixture([], true);
  await reject(offline);
  assert.match(offline.error(), /Could not connect/);
  const dropped = fixture([{id:0,players:1}]);
  dropped.open(0);
  dropped.Module.cod2Transport.sends.push(new Uint8Array([1,2,3]));
  dropped.sockets[0].readyState = 1;
  dropped.sockets[0].onopen();
  assert.equal(dropped.sockets[0].sent.length,1);
  dropped.sockets[0].readyState = 3;
  await dropped.sockets[0].onclose({code:1006,reason:''});
  assert.equal(dropped.error(), 'error\nEXE_SERVER_DISCONNECTED\0');
  const intentional = fixture([]);
  intentional.open(0);
  intentional.Module.cod2Transport.stopped = true;
  await intentional.sockets[0].onclose({code:1000,reason:''});
  assert.equal(intentional.Module.cod2Transport.packets.length,0);
  const changedRoom = fixture([{id:0,players:2}]);
  changedRoom.open(0);
  const pending = changedRoom.sockets[0].onclose({code:1006,reason:''});
  changedRoom.open(1);
  await pending;
  assert.equal(changedRoom.Module.cod2Transport.room,1);
  assert.equal(changedRoom.Module.cod2Transport.packets.length,0);
  console.log('PASS: full room, disappeared room, unreachable host, dropped connection, intentional disconnect and stale callbacks');
})().catch(error => {console.error(error);process.exitCode=1;});
