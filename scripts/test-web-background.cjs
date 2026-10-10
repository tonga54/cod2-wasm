#!/usr/bin/env node
// Run the real scheduler and packet callback with throttled timers and lost focus.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
function body(file, signature) {
  const source = fs.readFileSync(path.join(__dirname, '..', file), 'utf8');
  const start = source.indexOf(signature);
  assert.ok(start >= 0);
  return source.slice(source.indexOf('{', start) + 1, source.indexOf('\n});', start));
}
let now = 0, connected = true, frames = 0, releases = 0, interval;
const visibility = {}, focus = {}, sockets = [];
const document = {hidden:false, addEventListener:(name, fn)=>visibility[name]=fn};
const Module = {
  _web_client_connected:()=>connected,
  _web_background_frame:()=>{
    // The frame sees the incoming packet already queued.
    assert.ok(Module.cod2Transport?.packets.length || frames === 0 || intervalRunning);
    frames++;
    if (Module.cod2Transport) {Module.cod2Transport.packets.length=0;Module.cod2Transport.bytes=0;}
  },
  _web_input_lost:()=>releases++
};
let intervalRunning = false;
class Socket {constructor(){this.readyState=0;sockets.push(this);} close(){this.readyState=3;} send(){} }
const context = vm.createContext({Module, document, window:{addEventListener:(name, fn)=>focus[name]=fn},
  performance:{now:()=>now}, setInterval:fn=>interval=fn, WebSocket:Socket, URL, ArrayBuffer, Uint8Array,
  location:{href:'http://localhost:8097/',protocol:'http:'}, out(){},err(){} });
vm.runInContext(body('downstream/wasm/web_platform.c','EM_JS(void, WebBackground_Init, (void), {'), context);
vm.runInContext('function open(room) {' + body('downstream/wasm/web_net.c','EM_JS(void, WebNet_Open, (int room), {') + '\n}', context);
context.open(0);
Module.cod2BackgroundPump(); assert.equal(frames,0);
focus.blur(); assert.equal(releases,1);
document.hidden=true;visibility.visibilitychange();assert.equal(releases,2);assert.equal(frames,1);
// No timer callbacks for six minutes: received snapshots must still keep sending commands.
for (let i=1;i<=7200;i++) {now=i*50;sockets[0].onmessage({data:new Uint8Array([1,2,3]).buffer});}
assert.equal(frames,1441);assert.ok(Module.cod2Transport.packets.length < 5);
now+=1000;intervalRunning=true;interval();intervalRunning=false;assert.equal(frames,1442);
connected=false;now+=1000;interval();assert.equal(frames,1442);
connected=true;document.hidden=false;visibility.visibilitychange();now+=1000;interval();assert.equal(frames,1442);
sockets[0].onmessage({data:new Uint8Array([4]).buffer});assert.equal(frames,1442);
// Ignore old sockets and malformed packets, including background pump side effects.
Module.cod2Transport.stopped=true;document.hidden=true;now+=1000;
sockets[0].onmessage({data:new Uint8Array([4]).buffer});assert.equal(frames,1442);
console.log('PASS: hidden networking survives six minutes without timers, visible/disconnected frames stay idle, focus releases inputs');
