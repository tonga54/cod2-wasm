import assert from 'node:assert/strict';
import http from 'node:http';
import { once } from 'node:events';
import { test } from 'node:test';
import { createUpdateChecker, readBuildInfo } from './updates.mjs';
import { createGateway } from './server.mjs';

const old = 'a'.repeat(40), latest = 'b'.repeat(40);
const build = { revision: old, buildId: 'c'.repeat(64), dirty: false };
const reply = value => ({ ok: true, text: async () => JSON.stringify(value) });

test('GitHub checks are shared, cached, and refreshed after deployment', async () => {
  let calls = 0, clock = 0, current = build;
  const check = createUpdateChecker({ readBuild: async () => current, now: () => clock,
    fetchImpl: async (url, options) => {
      ++calls;
      assert.equal(options.redirect, 'error');
      assert.ok(!options.headers.Authorization);
      if (url.endsWith('/git/ref/heads/master')) return reply({ object: { sha: latest } });
      assert.equal(url, `https://api.github.com/repos/tonga54/cod2-wasm/compare/${old}...${latest}?per_page=1&page=2`);
      return reply({ status: 'ahead' });
    } });
  const results = await Promise.all(Array.from({ length: 64 }, () => check()));
  assert.equal(calls, 2);
  assert.ok(results.every(result => result.update.available));
  await check(); assert.equal(calls, 2);
  clock = 300001; await check(); assert.equal(calls, 4);
  current = { ...build, revision: latest, buildId: 'd'.repeat(64) };
  const deployed = await check(); assert.equal(calls, 5);
  assert.equal(deployed.current.buildId, current.buildId);
  assert.equal(deployed.update.status, 'current');
  assert.equal(deployed.update.available, false);
});

test('local commits and divergent histories never trigger a normal update', async () => {
  for (const [comparison, status] of [['behind', 'local-ahead'], ['diverged', 'diverged']]) {
    const check = createUpdateChecker({ readBuild: async () => build,
      fetchImpl: async url => reply(url.includes('/git/ref/') ? { object: { sha: latest } } : { status: comparison }) });
    const result = await check();
    assert.equal(result.update.status, status);
    assert.equal(result.update.available, false);
  }
});

test('offline, rate limiting, malformed responses, and missing metadata stay unknown', async () => {
  for (const fetchImpl of [async () => { throw new Error('offline'); },
    async () => ({ ok: false, status: 403 }), async () => reply({ object: { sha: 'invalid' } }),
    async () => ({ ok: true, text: async () => 'x'.repeat(131073) })]) {
    let calls = 0;
    const check = createUpdateChecker({ readBuild: async () => build,
      fetchImpl: (...args) => { ++calls; return fetchImpl(...args); } });
    assert.equal((await check()).update.status, 'unknown');
    assert.equal((await check()).update.available, false);
    assert.equal(calls, 1, 'failure is also cached');
  }
  const missing = createUpdateChecker({ readBuild: async () => { throw new Error('old host'); },
    fetchImpl: () => { throw new Error('must not reach GitHub'); } });
  assert.equal((await missing()).current, null);
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
