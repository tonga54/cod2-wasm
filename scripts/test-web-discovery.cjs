#!/usr/bin/env node
// Run the real EM_JS discovery with controlled HTTP replies and timers.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const source = fs.readFileSync(path.join(__dirname, '../downstream/wasm/web_net.c'), 'utf8');
const start = source.indexOf('EM_JS(void, Web_DiscoverServers, (), {');
assert.ok(start >= 0);
const body = source.slice(source.indexOf('{', start) + 1, source.indexOf('\n});', start));
const room = {id:0,info:'\\hostname\\Toujane | TDM',ping:4};
function fixture(reply) {
  const Module = {}, added = [], allocated = new Set(), timers = new Map(), errors = [];
  let calls = 0, timerId = 0, now = 0;
  const sandbox = vm.createContext({Module, AbortController, Set, Date:{now:()=>now},
    setTimeout(callback, delay) {
      const id = ++timerId;
      const started = now;
      const fire = () => { now = Math.max(now, started + delay); callback(); };
      timers.set(id, {callback:fire,delay});
      if (delay === 750) queueMicrotask(() => {
        if (timers.delete(id)) fire();
      });
      else assert.ok(delay > 0 && delay <= 3000);
      return id;
    },
    clearTimeout(id) {timers.delete(id);},
    fetch:async (url, options) => {
      assert.equal(url, '/servers'); assert.equal(options.cache, 'no-store');
      assert.ok(options.signal instanceof AbortSignal);
      return reply(++calls, options.signal, timers);
    },
    stringToNewUTF8(info) { const pointer = {info}; allocated.add(pointer); return pointer; },
    _web_add_room(id, pointer, ping) { added.push({id,info:pointer.info,ping}); },
    _free(pointer) {assert.ok(allocated.delete(pointer));},
    err(message) {errors.push(message);}
  });
  vm.runInContext('function discover() {' + body + '\n}', sandbox);
  return {discover:sandbox.discover, Module, added, errors,
    clean() {assert.equal(timers.size,0);assert.equal(allocated.size,0);},
    calls:()=>calls, elapsed:()=>now};
}
const response = rooms => ({ok:true,json:async()=>({rooms})});
(async () => {
  const ready = fixture(() => response([room]));
  await ready.discover(); assert.equal(ready.calls(),1);
  assert.deepEqual(ready.added,[room]); ready.clean();

  const starting = fixture(call => response(call < 10 ? [] : [room]));
  await starting.discover(); assert.equal(starting.calls(),10);
  assert.deepEqual(starting.added,[room]); starting.clean();

  const transient = fixture(call => {
    if (call === 1) throw new Error('Network unavailable');
    if (call === 2) return {ok:false};
    if (call === 3) return {ok:true,json:async()=>({})};
    return response([room]);
  });
  await transient.discover(); assert.equal(transient.calls(),4);
  assert.deepEqual(transient.added,[room]); assert.equal(transient.errors.length,0); transient.clean();

  const timedOut = fixture((call, signal, timers) => {
    if (call > 1) return response([room]);
    return new Promise((resolve,reject) => {
      signal.addEventListener('abort',()=>reject(new Error('Timed out')),{once:true});
      queueMicrotask(()=>[...timers.values()].find(t=>t.delay===3000).callback());
    });
  });
  await timedOut.discover(); assert.equal(timedOut.calls(),2);
  assert.deepEqual(timedOut.added,[room]); timedOut.clean();

  const empty = fixture(()=>response([]));
  await empty.discover(); assert.equal(empty.calls(),16);
  assert.equal(empty.added.length,0); empty.clean();
  const offline = fixture(()=>{throw new Error('Offline');});
  await offline.discover(); assert.equal(offline.calls(),16);
  assert.deepEqual(offline.errors,['[servers] Offline']); offline.clean();
  const hanging = fixture((call,signal,timers)=>new Promise((resolve,reject)=>{
    signal.addEventListener('abort',()=>reject(new Error('Timed out')),{once:true});
    queueMicrotask(()=>[...timers.values()][0]?.callback());
  }));
  await hanging.discover(); assert.equal(hanging.calls(),4);
  assert.equal(hanging.elapsed(),12000);
  assert.deepEqual(hanging.errors,['[servers] Timed out']); hanging.clean();

  const malformed = fixture(()=>response([null,{id:8,info:'bad'}, {id:1}, room, room,
    {id:2,info:'x'.repeat(2000),ping:2}]));
  await malformed.discover(); assert.equal(malformed.added.length,2);
  assert.equal(malformed.added[1].info.length,1023); malformed.clean();

  let finishOld;
  const stale = fixture(call=>call===1 ? new Promise(resolve=>{finishOld=resolve;}) :
    response([{...room,id:1}]));
  const first = stale.discover();
  await stale.discover(); finishOld(response([room])); await first;
  assert.deepEqual(stale.added,[{...room,id:1}]); stale.clean();
  console.log('PASS: ready/starting rooms, transient HTTP errors, timeout recovery, bounded retries, stale refreshes, duplicate/malformed rooms and resource cleanup');
})().catch(error=>{console.error(error);process.exitCode=1;});
