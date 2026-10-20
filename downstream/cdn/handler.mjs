// Serves the game archives publish-cdn.py uploads, for download links the Pi's
// gateway signs. Static assets are capped at 25 MiB, so each archive is stored
// as 24 MiB parts and joined here; every request runs the Worker first, so the
// parts are never served on their own.
export const PART_BYTES = 24 * 1024 * 1024;
const LINK = /^\/a\/([0-9a-f]{64})\/([1-9][0-9]{0,11})$/;
const CORS = { 'Access-Control-Allow-Origin': '*' };
const encoder = new TextEncoder();

function hexBytes(value) {
  if (!/^[0-9a-f]{64}$/.test(value)) return null;
  const bytes = new Uint8Array(32);
  for (let i = 0; i < 32; i++) bytes[i] = parseInt(value.slice(i * 2, i * 2 + 2), 16);
  return bytes;
}

// The gateway's signDownload() signs the same message with the same key.
export async function verifyLink(secret, sha256, size, expires, signature) {
  const expected = hexBytes(signature);
  if (!expected) return false;
  const key = await crypto.subtle.importKey('raw', encoder.encode(secret), { name: 'HMAC', hash: 'SHA-256' },
    false, ['verify']);
  return crypto.subtle.verify('HMAC', key, expected, encoder.encode(`${sha256}:${size}:${expires}`));
}

export function createHandler({ partBytes = PART_BYTES, now = Date.now } = {}) {
  return async function fetch(request, env) {
    if (request.method === 'OPTIONS') {
      return new Response(null, { status: 204, headers: { ...CORS, 'Access-Control-Allow-Methods': 'GET',
        'Access-Control-Max-Age': '86400' } });
    }
    if (request.method !== 'GET') return new Response(null, { status: 405, headers: { ...CORS, Allow: 'GET' } });
    if (!env.SIGNING_KEY) return new Response(null, { status: 503, headers: CORS });
    const url = new URL(request.url);
    const link = LINK.exec(url.pathname);
    const expires = url.searchParams.get('exp') || '';
    if (!link || !/^[0-9]{1,12}$/.test(expires) || Number(expires) * 1000 <= now() ||
        !await verifyLink(env.SIGNING_KEY, link[1], link[2], expires, url.searchParams.get('sig') || '')) {
      return new Response(null, { status: 403, headers: CORS });
    }
    const size = Number(link[2]);
    // Sets Content-Length, which the browser's download progress reads, and
    // fails the download if the parts do not add up to the signed size.
    const { readable, writable } = new FixedLengthStream(size);
    (async () => {
      for (let index = 0; index * partBytes < size; index++) {
        const part = await env.ASSETS.fetch(new Request(new URL(`/p/${link[1]}/${index}.part`, url)));
        if (!part.ok) throw new Error(`Part ${index} of ${link[1]} is missing`);
        await part.body.pipeTo(writable, { preventClose: true });
      }
      await writable.close();
    })().catch(error => writable.abort(error).catch(() => {}));
    return new Response(readable, { headers: { ...CORS, 'Content-Type': 'application/octet-stream',
      'Cache-Control': 'no-store, no-transform', 'Cross-Origin-Resource-Policy': 'cross-origin' } });
  };
}
