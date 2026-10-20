import assert from 'node:assert/strict';
import { createHash } from 'node:crypto';
import { test } from 'node:test';
import { signDownload } from '../gateway/cdn.mjs';
import { createHandler, PART_BYTES, verifyLink } from './handler.mjs';
import worker from './worker.mjs';

// The Workers runtime's FixedLengthStream: an identity stream that fails on
// any other byte count.
const lengths = [];
globalThis.FixedLengthStream = class extends TransformStream {
  constructor(length) {
    let seen = 0;
    super({
      transform(chunk, controller) {
        seen += chunk.byteLength;
        if (seen > length) throw new Error('More bytes than the fixed length');
        controller.enqueue(chunk);
      },
      flush() { if (seen !== length) throw new Error('Fewer bytes than the fixed length'); }
    });
    lengths.push(length);
  }
};

const secret = '0123456789abcdef'.repeat(4);
const archive = new TextEncoder().encode('abcdefghij');
const sha256 = createHash('sha256').update(archive).digest('hex');
const now = 1700000000000;
const handler = createHandler({ partBytes: 4, now: () => now });

function assets(parts) {
  const requests = [];
  return { requests, fetch: async request => {
    requests.push({ path: new URL(request.url).pathname, range: request.headers.get('range'),
      encoding: request.headers.get('accept-encoding') });
    const body = parts[new URL(request.url).pathname];
    // As long as a part: only the status check keeps it out of the download.
    return body === undefined ? new Response('nope', { status: 404 }) : new Response(body);
  } };
}
const stored = (bytes = archive) => ({ [`/p/${sha256}/0.part`]: bytes.slice(0, 4),
  [`/p/${sha256}/1.part`]: bytes.slice(4, 8), [`/p/${sha256}/2.part`]: bytes.slice(8) });
function link(size = archive.length, expires = now / 1000 + 900, signature) {
  return `https://cod2-data.example.workers.dev/a/${sha256}/${size}?exp=${expires}` +
    `&sig=${signature ?? signDownload(secret, sha256, size, expires)}`;
}

test('the deployed parts are under the 25 MiB static asset limit', async () => {
  assert.ok(PART_BYTES < 25 * 1024 * 1024);
  assert.deepEqual(Object.keys(await import('./worker.mjs')), ['default'], 'the main module exports only its handler');
  assert.equal(typeof worker.fetch, 'function');
});

test('a link the gateway signed downloads the parts joined in order', async () => {
  const binding = assets(stored());
  lengths.length = 0;
  const response = await handler(new Request(link()), { SIGNING_KEY: secret, ASSETS: binding });
  assert.equal(response.status, 200);
  assert.deepEqual(new Uint8Array(await response.arrayBuffer()), archive);
  assert.deepEqual(lengths, [archive.length], 'Content-Length is the signed size');
  assert.equal(response.headers.get('Access-Control-Allow-Origin'), '*');
  assert.equal(response.headers.get('Cross-Origin-Resource-Policy'), 'cross-origin');
  assert.equal(response.headers.get('Content-Type'), 'application/octet-stream');
  assert.equal(response.headers.get('Cache-Control'), 'no-store, no-transform');
  assert.deepEqual(binding.requests, [0, 1, 2].map(index => ({ path: `/p/${sha256}/${index}.part`, range: null,
    encoding: null })));
});

test('signatures match the vector the gateway and publish-cdn.py produce', async () => {
  const signature = '15e99cc456780c5fa327c94e4e7252b2bca0cad9079c3f8fac227ab5239838d8';
  assert.equal(await verifyLink(secret, 'a'.repeat(64), '123', '1700000000', signature), true);
  assert.equal(await verifyLink(secret, 'a'.repeat(64), '124', '1700000000', signature), false);
  assert.equal(await verifyLink(secret.replace('0', '1'), 'a'.repeat(64), '123', '1700000000', signature), false);
});

test('unsigned, expired or altered requests never reach the parts', async () => {
  const binding = assets(stored());
  const base = 'https://cod2-data.example.workers.dev';
  const signed = signDownload(secret, sha256, archive.length, now / 1000 + 900);
  for (const [label, url] of [
    ['an expired link', link(archive.length, now / 1000)],
    ['an altered signature', link(archive.length, now / 1000 + 900, signed.replace(/^./, c => c === 'a' ? 'b' : 'a'))],
    ['another size', link(archive.length + 1, now / 1000 + 900, signed)],
    ['a later expiry', link(archive.length, now / 1000 + 901, signed)],
    ['an exponent expiry', link(archive.length, '18e8', signDownload(secret, sha256, archive.length, '18e8'))],
    ['no signature', `${base}/a/${sha256}/${archive.length}?exp=${now / 1000 + 900}`],
    ['an uppercase hash', link().replace(sha256, sha256.toUpperCase())],
    ['a zero size', link(0)],
    ['a part directly', `${base}/p/${sha256}/0.part`],
    ['the root', `${base}/`],
  ]) {
    const response = await handler(new Request(url), { SIGNING_KEY: secret, ASSETS: binding });
    assert.equal(response.status, 403, label);
    assert.equal(response.headers.get('Access-Control-Allow-Origin'), '*', label);
  }
  assert.equal((await handler(new Request(link()), { ASSETS: binding })).status, 503, 'no signing key');
  assert.deepEqual(binding.requests, []);
});

test('only GET downloads; a preflight is answered', async () => {
  const preflight = await handler(new Request(link(), { method: 'OPTIONS' }), { SIGNING_KEY: secret });
  assert.equal(preflight.status, 204);
  assert.equal(preflight.headers.get('Access-Control-Allow-Origin'), '*');
  assert.equal(preflight.headers.get('Access-Control-Allow-Methods'), 'GET');
  const post = await handler(new Request(link(), { method: 'POST' }), { SIGNING_KEY: secret });
  assert.equal(post.status, 405);
  assert.equal(post.headers.get('Allow'), 'GET');
});

test('a missing part, or parts of the wrong length, fail the download instead of truncating it', async () => {
  const { [`/p/${sha256}/1.part`]: _, ...gap } = stored();
  const long = stored(new TextEncoder().encode('abcdefghijk'));
  for (const [label, parts] of [['a missing part', gap], ['more bytes than signed', long],
    ['fewer bytes than signed', { ...stored(), [`/p/${sha256}/2.part`]: new TextEncoder().encode('i') }]]) {
    const response = await handler(new Request(link()), { SIGNING_KEY: secret, ASSETS: assets(parts) });
    assert.equal(response.status, 200, label);
    await assert.rejects(response.arrayBuffer(), undefined, label);
  }
});
