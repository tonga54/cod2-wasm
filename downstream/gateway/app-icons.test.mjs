import assert from 'node:assert/strict';
import { once } from 'node:events';
import { mkdtemp, rm, writeFile } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { crc32, deflateSync } from 'node:zlib';
import test from 'node:test';
import { APP_ICONS, decodeIco, decodePng, encodePng, renderAppIcons } from './app-icons.mjs';
import { createGateway } from './server.mjs';

// The executable's layout: 32-bit images on a near-black ground, rows bottom-up
// in BGRA, then the AND mask. The 48 px one has a bright diamond in its middle.
function iconImage(size, bright) {
  const dib = Buffer.alloc(40 + size * size * 4 + size * Math.ceil(size / 32) * 4);
  dib.writeUInt32LE(40, 0);
  dib.writeInt32LE(size, 4);
  dib.writeInt32LE(size * 2, 8);
  dib.writeUInt16LE(1, 12);
  dib.writeUInt16LE(32, 14);
  for (let y = 0; y < size; ++y) {
    for (let x = 0; x < size; ++x) {
      const inside = Math.abs(x - (size - 1) / 2) + Math.abs(y - (size - 1) / 2) < size * 0.375;
      dib.set(inside ? bright : [3, 4, 4, 255], 40 + ((size - 1 - y) * size + x) * 4);
    }
  }
  return dib;
}
function iconFile() {
  const images = [iconImage(16, [0, 0, 255, 255]), iconImage(48, [120, 230, 240, 255])];
  const directory = Buffer.alloc(6 + images.length * 16);
  directory.writeUInt16LE(1, 2);
  directory.writeUInt16LE(images.length, 4);
  let offset = directory.length;
  images.forEach((image, index) => {
    const entry = 6 + index * 16, size = image.readInt32LE(4);
    directory.set([size, size], entry);
    directory.writeUInt16LE(1, entry + 4);
    directory.writeUInt16LE(32, entry + 6);
    directory.writeUInt32LE(image.length, entry + 8);
    directory.writeUInt32LE(offset, entry + 12);
    offset += image.length;
  });
  return Buffer.concat([directory, ...images]);
}
function chunks(png) {
  const found = [];
  for (let offset = 8; offset < png.length; offset += 12 + png.readUInt32BE(offset)) {
    const length = png.readUInt32BE(offset);
    found.push({ type: png.toString('latin1', offset + 4, offset + 8), crc: png.readUInt32BE(offset + 8 + length),
      typed: png.subarray(offset + 4, offset + 8 + length) });
  }
  return found;
}
function pngFile(width, height, raw) {
  const chunk = (type, body) => {
    const typed = Buffer.concat([Buffer.from(type, 'latin1'), body]), length = Buffer.alloc(4), crc = Buffer.alloc(4);
    length.writeUInt32BE(body.length);
    crc.writeUInt32BE(crc32(typed));
    return Buffer.concat([length, typed, crc]);
  };
  const header = Buffer.alloc(13);
  header.writeUInt32BE(width, 0);
  header.writeUInt32BE(height, 4);
  header.set([8, 6, 0, 0, 0], 8);
  return Buffer.concat([Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]), chunk('IHDR', header),
    chunk('IDAT', deflateSync(raw)), chunk('IEND', Buffer.alloc(0))]);
}
// As extract-startup-logo.py writes logo_cod2: 512 x 128 RGBA, unfiltered rows,
// with silver lettering 377 x 79 in a transparent margin.
const LOGO = { width: 512, height: 128, left: 67, top: 24, right: 443, bottom: 102 };
function logoFile(filters = [0]) {
  const { width, height } = LOGO, stride = width * 4, pixels = new Uint8Array(stride * height);
  for (let y = LOGO.top; y <= LOGO.bottom; ++y) {
    for (let x = LOGO.left; x <= LOGO.right; ++x) pixels.set([200, 205, 215, 255], (y * width + x) * 4);
  }
  const raw = Buffer.alloc((stride + 1) * height);
  for (let y = 0; y < height; ++y) {
    const filter = filters[y % filters.length], row = y * stride;
    raw[row + y] = filter;
    for (let i = 0; i < stride; ++i) {
      const left = i >= 4 ? pixels[row + i - 4] : 0, up = y ? pixels[row - stride + i] : 0;
      const corner = y && i >= 4 ? pixels[row - stride + i - 4] : 0, p = left + up - corner;
      const paeth = Math.abs(p - left) <= Math.abs(p - up) && Math.abs(p - left) <= Math.abs(p - corner) ? left :
        Math.abs(p - up) <= Math.abs(p - corner) ? up : corner;
      raw[row + y + 1 + i] = pixels[row + i] - [0, left, up, (left + up) >> 1, paeth][filter];
    }
  }
  return { pixels, png: pngFile(width, height, raw) };
}
const pixel = ({ width, pixels }, x, y) => [...pixels.subarray((y * width + x) * 4, (y * width + x) * 4 + 4)];
const brightness = colour => Math.max(colour[0], colour[1], colour[2]);

test('decodes the executable icon\'s largest 32-bit image top row first, and PNG rows of every filter', () => {
  const icon = decodeIco(iconFile());
  assert.equal(icon.width, 48);
  assert.deepEqual(pixel(icon, 0, 0), [4, 4, 3, 255], 'BGRA becomes RGBA');
  assert.deepEqual(pixel(icon, 24, 24), [240, 230, 120, 255]);
  assert.throws(() => decodeIco(Buffer.from([0, 0, 2, 0, 0, 0])), /Not an icon/);
  const { pixels, png } = logoFile([0, 1, 2, 3, 4]);
  const logo = decodePng(png);
  assert.deepEqual([logo.width, logo.height], [512, 128]);
  assert.deepEqual(logo.pixels, pixels);
  const encoded = encodePng(logo);
  assert.deepEqual(decodePng(encoded).pixels, pixels, 'the encoder\'s output decodes to the same pixels');
  assert.deepEqual(chunks(encoded).map(chunk => chunk.type), ['IHDR', 'IDAT', 'IEND']);
  for (const chunk of chunks(encoded)) assert.equal(chunk.crc, crc32(chunk.typed), `${chunk.type} checksum`);
  assert.throws(() => decodePng(Buffer.from('not a png')), /Not a PNG/);
});

test('draws every install icon at its size: rounded tiles and full-bleed masks, star over lettering', () => {
  const icons = renderAppIcons(iconFile(), logoFile().png);
  assert.deepEqual(Object.keys(icons), Object.keys(APP_ICONS));
  for (const [name, { size, fullBleed, scale = 1 }] of Object.entries(APP_ICONS)) {
    const image = decodePng(icons[name]);
    assert.deepEqual([image.width, image.height], [size, size], name);
    const corner = pixel(image, 0, 0), middle = pixel(image, size >> 1, size >> 1);
    assert.equal(corner[3], fullBleed ? 255 : 0, `${name}: ${fullBleed ? 'masked by the platform' : 'rounded corners'}`);
    assert.equal(middle[3], 255, name);
    // Star (the diamond) above, lettering below, on the 512 grid scaled about (256, 247).
    const at = (x, y) => [Math.round(size / 2 + (x - 256) * size / 512 * scale),
      Math.round(size * 247 / 512 + (y - 247) * size / 512 * scale)];
    assert.ok(brightness(pixel(image, ...at(256, 202))) > 180, `${name}: the star's middle`);
    assert.ok(brightness(pixel(image, ...at(256, 397))) > 180, `${name}: the lettering`);
    // The icon's near-black ground is keyed out: just inside the star's square matches just outside it.
    const inside = pixel(image, ...at(112, 58)), outside = pixel(image, ...at(100, 46));
    assert.ok(inside.every((value, c) => Math.abs(value - outside[c]) <= 2), `${name}: no square behind the star`);
    if (name === 'app-maskable-512.png') {
      for (let y = 0; y < size; ++y) {
        for (let x = 0; x < size; ++x) {
          if (Math.hypot(x + 0.5 - size / 2, y + 0.5 - size / 2) < size * 0.4) continue;
          assert.ok(brightness(pixel(image, x, y)) <= 0x1a, `nothing outside the safe circle at ${x},${y}`);
        }
      }
    }
  }
  const again = renderAppIcons(iconFile(), logoFile().png);
  for (const name of Object.keys(icons)) assert.ok(icons[name].equals(again[name]), `${name} is deterministic`);
});

test('the gateway serves the drawn icons and the originals, and draws them once artwork appears', async () => {
  const directory = await mkdtemp(join(tmpdir(), 'cod2-icons-'));
  const iconPath = join(directory, 'cod2.ico'), startupPath = join(directory, 'cod2-startup.png');
  const gateway = createGateway({ iconPath, startupPath });
  gateway.server.listen(0, '127.0.0.1');
  await once(gateway.server, 'listening');
  const base = `http://127.0.0.1:${gateway.server.address().port}/game-data/files/`;
  try {
    let response = await fetch(base + 'cod2-icon/app-512.png');
    assert.equal(response.status, 404, 'no icon without the host\'s artwork');
    assert.equal(await response.text(), 'Game artwork unavailable');
    const ico = iconFile(), logo = logoFile().png;
    await writeFile(iconPath, ico);
    await writeFile(startupPath, logo);
    for (const [name, { size }] of Object.entries(APP_ICONS)) {
      response = await fetch(base + 'cod2-icon/' + name);
      assert.equal(response.status, 200, name);
      assert.equal(response.headers.get('content-type'), 'image/png');
      assert.equal(response.headers.get('cache-control'), 'public, max-age=3600');
      assert.equal(response.headers.get('x-content-type-options'), 'nosniff');
      const image = decodePng(Buffer.from(await response.arrayBuffer()));
      assert.deepEqual([image.width, image.height], [size, size], name);
    }
    response = await fetch(base + 'cod2-icon/app-192.png', { method: 'HEAD' });
    assert.equal(response.status, 200);
    assert.ok(Number(response.headers.get('content-length')) > 0);
    assert.equal((await response.arrayBuffer()).byteLength, 0);
    response = await fetch(base + 'cod2-icon/app-192.png', { method: 'POST' });
    assert.equal(response.status, 405);
    assert.equal(response.headers.get('allow'), 'GET, HEAD');
    for (const path of ['cod2-icon/unknown.png', 'cod2-icon/constructor.png']) {
      assert.equal((await fetch(base + path)).status, 404, path);
    }
    response = await fetch(base + 'cod2-icon/original.ico');
    assert.equal(response.headers.get('content-type'), 'image/x-icon');
    assert.ok(Buffer.from(await response.arrayBuffer()).equals(ico));
    response = await fetch(base + 'cod2-startup/original.png');
    assert.ok(Buffer.from(await response.arrayBuffer()).equals(logo));
  } finally {
    await gateway.close();
    await rm(directory, { recursive: true, force: true });
  }
});
