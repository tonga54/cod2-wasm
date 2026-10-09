#!/usr/bin/env node
// Execute the browser's real creation callback, including selected map and failure cleanup.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const source = fs.readFileSync(path.join(__dirname, '../downstream/wasm/web_net.c'), 'utf8');
const start = source.indexOf('EM_JS(void, Web_CreateServer, (const char *name, const char *mapName), {');
assert.ok(start >= 0);
const body = source.slice(source.indexOf('{', start) + 1, source.indexOf('\n});', start));

async function check(map, reply, expected) {
  const Module = {}, requests = [], results = [], allocated = new Set();
  let finish;
  const context = vm.createContext({Module,
    UTF8ToString: value => value,
    fetch: (url, options) => {
      assert.equal(url, '/servers'); assert.equal(options.method, 'POST');
      requests.push(JSON.parse(options.body));
      return new Promise(resolve => { finish = resolve; });
    },
    stringToNewUTF8: value => { allocated.add(value); return value; },
    _web_created_room: (id, error) => results.push([id, error]),
    _free: value => assert.ok(allocated.delete(value))
  });
  vm.runInContext('function create(name, mapName) {' + body + '\n}', context);
  context.create('Selected room', map);
  context.create('Duplicate', 'mp_toujane');
  assert.equal(requests.length, 1);
  assert.deepEqual(requests[0], {name: 'Selected room', map});
  finish({ok: reply.ok, json: async () => reply.value});
  for (let i = 0; i < 10; ++i) await Promise.resolve();
  assert.deepEqual(results, [expected]);
  assert.equal(Module.roomCreationPending, false);
  assert.equal(allocated.size, 0);
}
(async () => {
  await check('mp_carentan', {ok: true, value: {id: 1}}, [1, 0]);
  await check('mp_toujane', {ok: true, value: {id: 2}}, [2, 0]);
  await check('mp_carentan', {ok: false, value: {error: 'Server unavailable'}}, [-1, 'Server unavailable']);
  await check('mp_carentan', {ok: true, value: {id: 8}}, [-1, 'Respuesta de partida inválida.']);
  console.log('PASS: selected map/name, duplicate creation guard, valid room IDs and error cleanup');
})().catch(error => { console.error(error); process.exitCode = 1; });
