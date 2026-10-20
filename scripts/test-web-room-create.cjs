#!/usr/bin/env node
// Execute the browser's real creation callback, including selected map and failure cleanup.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const source = fs.readFileSync(path.join(__dirname, '../downstream/wasm/web_net.c'), 'utf8');
const start = source.indexOf('EM_JS(void, Web_CreateServer, (const char *name, const char *mapName, const char *gameType, int botCount, int botDifficulty, int timeLimit, int scoreLimit), {');
assert.ok(start >= 0);
const body = source.slice(source.indexOf('{', start) + 1, source.indexOf('\n});', start));

async function check(map, reply, expected, gametype = 'tdm', botCount = 0, difficulty = 1, timeLimit = -1, scoreLimit = -1) {
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
  vm.runInContext('function create(name, mapName, gameType, botCount, botDifficulty, timeLimit, scoreLimit) {' + body + '\n}', context);
  context.create('Selected room', map, gametype, botCount, difficulty, timeLimit, scoreLimit);
  context.create('Duplicate', 'mp_toujane', 'dm', 0, 1, 5, 10);
  assert.equal(requests.length, 1);
  // A negative rule leaves the mode's own, for the room to keep.
  const rules = {...(timeLimit >= 0 && {timeLimit}), ...(scoreLimit >= 0 && {scoreLimit})};
  assert.deepEqual(requests[0], {name: 'Selected room', map, gametype, botCount, botDifficulty: ['easy','normal','hard'][difficulty], ...rules});
  finish({ok: reply.ok, json: async () => reply.value});
  for (let i = 0; i < 10; ++i) await Promise.resolve();
  assert.deepEqual(results, [expected]);
  assert.equal(Module.roomCreationPending, false);
  assert.equal(allocated.size, 0);
}
(async () => {
  for (const gametype of ['dm', 'tdm', 'ctf', 'hq', 'sd'])
    for (const map of ['mp_toujane', 'mp_carentan', 'mp_dawnville'])
      await check(map, {ok: true, value: {id: 0}}, [0, 0], gametype);
  for (const difficulty of [0,1,2])
    await check('mp_carentan', {ok: true, value: {id: 1}}, [1, 0], 'tdm', 8, difficulty);
  await check('mp_carentan', {ok: true, value: {id: 1}}, [1, 0], 'tdm', 32, 2);
  for (const [gametype, timeLimit, scoreLimit] of [['tdm', 15, 100], ['sd', 0, 0], ['ctf', 30, -1], ['hq', -1, 300]])
    await check('mp_dawnville', {ok: true, value: {id: 2}}, [2, 0], gametype, 0, 1, timeLimit, scoreLimit);
  await check('mp_toujane', {ok: true, value: {id: 2}}, [2, 0]);
  await check('mp_carentan', {ok: false, value: {error: 'Server unavailable'}}, [-1, 'Server unavailable']);
  await check('mp_carentan', {ok: true, value: {id: 8}}, [-1, 'Invalid server response.']);
  console.log('PASS: selected map/mode/name/rules, duplicate creation guard, valid room IDs and error cleanup');
})().catch(error => { console.error(error); process.exitCode = 1; });
