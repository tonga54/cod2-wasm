import { createHmac } from 'node:crypto';
import { readFileSync } from 'node:fs';
import http from 'node:http';
import path from 'node:path';

const SHA256 = /^[a-f0-9]{64}$/;

// downstream/cdn/handler.mjs checks the same message with the same key.
export function signDownload(secret, sha256, size, expires) {
  return createHmac('sha256', secret).update(`${sha256}:${size}:${expires}`).digest('hex');
}

// Each archive key the web server serves, as "sha256:size"; null for a key
// the variants disagree on.
export function readAssetPolicy(webHost, webPort) {
  return new Promise((resolve, reject) => {
    const request = http.get({ hostname: webHost, port: webPort, path: '/wasm-game-data.json', timeout: 2000 },
      response => {
        const chunks = []; let bytes = 0;
        response.on('data', chunk => {
          bytes += chunk.length;
          if (bytes > 65536) response.destroy(new Error('Asset policy too large'));
          else chunks.push(chunk);
        });
        response.on('error', reject);
        response.on('end', () => {
          try {
            if (response.statusCode !== 200) throw new Error('Asset policy unavailable');
            const files = new Map();
            for (const variant of Object.values(JSON.parse(Buffer.concat(chunks)).variants || {})) {
              for (const file of variant.files || []) {
                const key = String(file.key).toLowerCase();
                const value = SHA256.test(file.sha256) && Number.isSafeInteger(file.size) && file.size > 0
                  ? `${file.sha256}:${file.size}` : null;
                files.set(key, files.has(key) && files.get(key) !== value ? null : value);
              }
            }
            resolve(files);
          } catch (error) { reject(error); }
        });
      });
    request.on('timeout', () => request.destroy(new Error('Asset policy timed out')));
    request.on('error', reject);
  });
}

// Sends an archive's download to the Cloudflare copy publish-cdn.py verified,
// only while that copy is the archive the web server serves now; otherwise
// the caller serves it from the Pi.
export function createCdnRedirects({ directory = '/cdn', readPolicy, read = readFileSync, now = Date.now,
  lifetime = 900, policyLifetime = 10000 } = {}) {
  let policy = null, policyAt = -Infinity;
  return async function redirect(key) {
    let published, secret, origin;
    try {
      published = JSON.parse(read(path.join(directory, 'published.json'), 'utf8'));
      secret = read(path.join(directory, 'signing-key'), 'utf8').trim();
      const url = new URL(published.url);
      if (url.protocol !== 'https:' || url.pathname !== '/' || url.search || url.hash || url.username) return null;
      origin = url.origin;
    } catch { return null; }
    if (!SHA256.test(secret) || !published.files || typeof published.files !== 'object') return null;
    if (now() - policyAt >= policyLifetime) {
      policy = await readPolicy().catch(() => null);
      policyAt = now();
    }
    const value = policy?.get(String(key).toLowerCase());
    if (!value) return null;
    const [sha256, size] = value.split(':');
    if (published.files[sha256] !== Number(size)) return null;
    const expires = Math.floor(now() / 1000) + lifetime;
    return `${origin}/a/${sha256}/${size}?exp=${expires}&sig=${signDownload(secret, sha256, size, expires)}`;
  };
}
