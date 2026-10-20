import assert from 'node:assert/strict';
import { mkdtempSync, renameSync, rmSync, writeFileSync } from 'node:fs';
import http from 'node:http';
import { once } from 'node:events';
import { tmpdir } from 'node:os';
import path from 'node:path';
import { test } from 'node:test';
import { createCdnRedirects, readAssetPolicy, signDownload } from './cdn.mjs';
import { createGateway } from './server.mjs';

const secret = '0123456789abcdef'.repeat(4);
const renderer = { key: 'cod2-browser-renderer', sha256: 'b'.repeat(64), size: 400356491 };
const audio = { key: 'cod2-browser-audio', sha256: 'c'.repeat(64), size: 72956320 };
const worker = 'https://cod2-data.example.workers.dev';
const policyOf = (...files) => new Map(files.map(file => [file.key, `${file.sha256}:${file.size}`]));

function store(values) {
  const files = new Map(Object.entries(values));
  return name => {
    const value = files.get(path.basename(name));
    if (value === undefined) throw Object.assign(new Error('missing'), { code: 'ENOENT' });
    return value;
  };
}

test('signatures match the vector publish-cdn.py and the Worker check', () => {
  assert.equal(signDownload(secret, 'a'.repeat(64), 123, 1700000000),
    '15e99cc456780c5fa327c94e4e7252b2bca0cad9079c3f8fac227ab5239838d8');
});

test('a published archive gets a signed link that expires in 15 minutes', async () => {
  const redirect = createCdnRedirects({ now: () => 1700000000500, readPolicy: async () => policyOf(renderer),
    read: store({ 'published.json': JSON.stringify({ url: worker, files: { [renderer.sha256]: renderer.size } }),
      'signing-key': secret + '\n' }) });
  const expires = 1700000000 + 900;
  const expected = `${worker}/a/${renderer.sha256}/${renderer.size}?exp=${expires}` +
    `&sig=${signDownload(secret, renderer.sha256, renderer.size, expires)}`;
  assert.equal(await redirect('cod2-browser-renderer'), expected);
  assert.equal(await redirect('COD2-Browser-Renderer'), expected, 'keys match as the framework does');
});

test('anything but the verified copy of the archive served now stays on the Pi', async () => {
  const published = { url: worker, files: { [renderer.sha256]: renderer.size } };
  const cases = [
    ['an archive the Pi changed since publishing', { policy: policyOf({ ...renderer, sha256: 'd'.repeat(64) }) }],
    ['a different size', { published: { ...published, files: { [renderer.sha256]: renderer.size - 1 } } }],
    ['an unpublished key', { key: audio.key, policy: policyOf(renderer, audio) }],
    ['a key the variants disagree on', { policy: new Map([[renderer.key, null]]) }],
    ['no published list', { files: { 'signing-key': secret } }],
    ['no signing key', { files: { 'published.json': JSON.stringify(published) } }],
    ['a short signing key', { files: { 'published.json': JSON.stringify(published), 'signing-key': 'abc' } }],
    ['a plain-HTTP Worker', { published: { ...published, url: 'http://cod2-data.example.workers.dev' } }],
    ['a Worker URL with a path', { published: { ...published, url: worker + '/other' } }],
    ['a corrupt published list', { files: { 'published.json': '{', 'signing-key': secret } }],
    ['no published files', { published: { url: worker } }],
    ['an unreadable asset policy', { policy: new Error('cod2-web is restarting') }],
  ];
  for (const [label, { key = renderer.key, policy = policyOf(renderer), published: list = published, files }] of cases) {
    const redirect = createCdnRedirects({
      readPolicy: async () => { if (policy instanceof Error) throw policy; return policy; },
      read: store(files || { 'published.json': JSON.stringify(list), 'signing-key': secret }) });
    assert.equal(await redirect(key), null, label);
  }
});

test('the asset policy is read again after ten seconds, and a failure is not retried at once', async () => {
  let clock = 0, reads = 0, policy = new Error('restarting');
  const redirect = createCdnRedirects({ now: () => clock,
    readPolicy: async () => { ++reads; if (policy instanceof Error) throw policy; return policy; },
    read: store({ 'published.json': JSON.stringify({ url: worker, files: { [renderer.sha256]: renderer.size } }),
      'signing-key': secret }) });
  assert.equal(await redirect(renderer.key), null);
  policy = policyOf(renderer);
  clock = 9999;
  assert.equal(await redirect(renderer.key), null);
  assert.equal(reads, 1);
  clock = 10000;
  assert.match(await redirect(renderer.key), /^https:\/\/cod2-data\.example\.workers\.dev\/a\//);
  assert.equal(reads, 2);
  policy = policyOf({ ...renderer, sha256: 'd'.repeat(64) });
  clock = 15000;
  assert.match(await redirect(renderer.key), /\/a\/b{64}\//, 'the policy is cached between reads');
  clock = 20000;
  assert.equal(await redirect(renderer.key), null, 'an update on the Pi stops the redirect');
  assert.equal(reads, 3);
});

test('the web server policy is parsed per key and bounded', async () => {
  let body = '', status = 200;
  const web = http.createServer((request, response) => {
    assert.equal(request.url, '/wasm-game-data.json');
    response.writeHead(status); response.end(body);
  });
  web.listen(0, '127.0.0.1'); await once(web, 'listening');
  const port = web.address().port;
  try {
    body = JSON.stringify({ variants: {
      one: { files: [renderer, { key: 'Cod2-Browser-Audio', sha256: audio.sha256, size: audio.size },
        { key: 'broken', sha256: 'not-a-hash', size: 1 }, { key: 'shared', sha256: 'e'.repeat(64), size: 5 }] },
      two: { files: [{ key: 'shared', sha256: 'f'.repeat(64), size: 5 }, renderer] } } });
    assert.deepEqual([...(await readAssetPolicy('127.0.0.1', port))], [
      [renderer.key, `${renderer.sha256}:${renderer.size}`], [audio.key, `${audio.sha256}:${audio.size}`],
      ['broken', null], ['shared', null]]);
    status = 404;
    await assert.rejects(readAssetPolicy('127.0.0.1', port), /unavailable/);
    status = 200; body = ' '.repeat(65537);
    await assert.rejects(readAssetPolicy('127.0.0.1', port), /too large/);
  } finally { await new Promise(resolve => web.close(resolve)); }
});

test('the gateway redirects published archives and proxies the rest to the web server', async () => {
  const directory = mkdtempSync(path.join(tmpdir(), 'cod2-cdn-'));
  const publish = files => {
    writeFileSync(path.join(directory, 'next.json'), JSON.stringify({ url: worker, files }));
    renameSync(path.join(directory, 'next.json'), path.join(directory, 'published.json'));
  };
  writeFileSync(path.join(directory, 'signing-key'), secret + '\n');
  publish({ [renderer.sha256]: renderer.size });
  const served = [];
  const web = http.createServer((request, response) => {
    if (request.url === '/wasm-game-data.json') {
      response.end(JSON.stringify({ variants: { 'cod2-mp': { files: [renderer, audio] } } })); return;
    }
    served.push(`${request.method} ${request.url}`);
    response.writeHead(200, { 'Content-Type': 'application/octet-stream', 'X-From': 'pi' });
    response.end(request.method === 'HEAD' ? undefined : `pi:${request.url}`);
  });
  web.listen(0, '127.0.0.1'); await once(web, 'listening');
  const webPort = web.address().port;
  const gateway = createGateway({ webHost: '127.0.0.1', webPort,
    cdnRedirect: createCdnRedirects({ directory, policyLifetime: 0,
      readPolicy: () => readAssetPolicy('127.0.0.1', webPort) }) });
  gateway.server.listen(0, '127.0.0.1'); await once(gateway.server, 'listening');
  const origin = `http://127.0.0.1:${gateway.server.address().port}`;
  const get = (name, init) => fetch(`${origin}/game-data/files/${name}?variant=cod2-mp`, { redirect: 'manual', ...init });
  try {
    const before = Math.floor(Date.now() / 1000);
    const redirected = await get('cod2-browser-renderer');
    assert.equal(redirected.status, 302);
    assert.equal(redirected.headers.get('Cache-Control'), 'no-store');
    assert.equal(await redirected.text(), '');
    const location = new URL(redirected.headers.get('Location'));
    assert.equal(location.origin + location.pathname, `${worker}/a/${renderer.sha256}/${renderer.size}`);
    const expires = Number(location.searchParams.get('exp'));
    assert.ok(expires >= before + 900 && expires <= Math.floor(Date.now() / 1000) + 900);
    assert.equal(location.searchParams.get('sig'), signDownload(secret, renderer.sha256, renderer.size, expires));

    const proxied = await get('cod2-browser-audio');
    assert.equal(proxied.status, 200);
    assert.equal(proxied.headers.get('X-From'), 'pi');
    assert.equal(await proxied.text(), 'pi:/game-data/files/cod2-browser-audio?variant=cod2-mp');
    const head = await get('cod2-browser-renderer', { method: 'HEAD' });
    assert.equal(head.status, 200, 'only GET downloads go to Cloudflare');
    assert.equal((await get('%E0%A4%A')).status, 200, 'an undecodable key is left to the web server');

    publish({ [audio.sha256]: audio.size });
    assert.equal((await get('cod2-browser-renderer')).status, 200, 'a new published list applies without a restart');
    assert.equal((await get('cod2-browser-audio')).status, 302);
    rmSync(directory, { recursive: true });
    assert.equal((await get('cod2-browser-audio')).status, 200, 'without the directory everything comes from the Pi');
    assert.deepEqual(served, ['GET /game-data/files/cod2-browser-audio?variant=cod2-mp',
      'HEAD /game-data/files/cod2-browser-renderer?variant=cod2-mp', 'GET /game-data/files/%E0%A4%A?variant=cod2-mp',
      'GET /game-data/files/cod2-browser-renderer?variant=cod2-mp',
      'GET /game-data/files/cod2-browser-audio?variant=cod2-mp']);
  } finally {
    rmSync(directory, { recursive: true, force: true });
    await gateway.close(); await new Promise(resolve => web.close(resolve));
  }
});
