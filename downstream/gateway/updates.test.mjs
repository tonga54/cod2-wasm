import assert from 'node:assert/strict';
import http from 'node:http';
import { once } from 'node:events';
import { test } from 'node:test';
import { createUpdateChecker, readBuildInfo } from './updates.mjs';
import { createGateway } from './server.mjs';

const old = 'a'.repeat(40), latest = 'b'.repeat(40);
const build = { revision: old, buildId: 'c'.repeat(64), dirty: false };

test('players only receive installed builds and never repository-update popups', async () => {
  let current=build;
  const check=createUpdateChecker({readBuild:async()=>current,
    fetchImpl:()=>{throw Error('Player version checks must not contact GitHub');}});
  for(const result of await Promise.all(Array.from({length:64},()=>check()))){
    assert.deepEqual(result.current,build);assert.equal(result.update.available,false);
    assert.equal(result.update.status,'host-managed');
  }
  current={...build,revision:latest,buildId:'d'.repeat(64)};
  assert.deepEqual((await check()).current,current);
  assert.equal((await check()).update.available,false);
  const offline=createUpdateChecker({readBuild:async()=>{throw Error('missing');}});
  assert.equal((await offline()).current,null);
  assert.equal((await offline()).update.status,'unknown');
});

test('metadata validation and HTTP version endpoint are read-only', async () => {
  let value = build, calls = 0;
  const web = http.createServer((request, response) => {
    assert.equal(request.url, '/build-info.json');
    response.end(JSON.stringify(value));
  });
  web.listen(0, '127.0.0.1'); await once(web, 'listening');
  const gateway = createGateway({ updateChecker: async () => {
    ++calls; return { current: build, update: { status: 'current', available: false } };
  } });
  gateway.server.listen(0, '127.0.0.1'); await once(gateway.server, 'listening');
  try {
    assert.deepEqual(await readBuildInfo('127.0.0.1', web.address().port), build);
    value = { ...build, revision: '../../arbitrary' };
    await assert.rejects(readBuildInfo('127.0.0.1', web.address().port), /Invalid/);
    value = { ...build, buildId: 'x'.repeat(4096) };
    await assert.rejects(readBuildInfo('127.0.0.1', web.address().port), /too large/);
    const origin = `http://127.0.0.1:${gateway.server.address().port}`;
    const response = await fetch(origin + '/version');
    assert.equal(response.status, 200);
    assert.equal(response.headers.get('Cache-Control'), 'no-store');
    assert.equal((await response.json()).current.revision, old);
    assert.equal((await fetch(origin + '/version', { method: 'POST', body: '{}' })).status, 405);
    assert.equal(calls, 1);
  } finally {
    await gateway.close(); await new Promise(resolve => web.close(resolve));
  }
});
