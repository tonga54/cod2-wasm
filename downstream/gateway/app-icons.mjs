import { deflateSync, inflateSync } from 'node:zlib';

// Chrome installs a web app only with a PNG icon of at least 144 px; the
// executable's icon is 48 px. These are drawn from the host's originals: the
// executable's star over the logo's lettering, on the loading screen's
// gradient (startup.css).
export const APP_ICONS = Object.freeze({
  'app-192.png': Object.freeze({ size: 192 }),
  'app-512.png': Object.freeze({ size: 512 }),
  // Masks keep at least a centred circle 80% of the icon wide.
  'app-maskable-512.png': Object.freeze({ size: 512, fullBleed: true, scale: 0.72 }),
  // iOS and Safari round the corners themselves.
  'apple-touch-icon.png': Object.freeze({ size: 180, fullBleed: true, scale: 0.84 })
});

// On a 512 grid, as wide as the logo: the star 300 square from (106, 52) and
// the lettering at its own size within 392 x 94 from (60, 352), both scaled
// about (256, 247), the pair's middle; the tile inset 26 with corners of
// radius 92 and a gold rim 3 wide.
const GRID = 512;
const PIVOT_Y = 247;
const STAR = { x: 106, y: 52, width: 300, height: 300 };
const LETTERING = { x: 60, y: 352, width: 392, height: 94 };
const TILE_INSET = 26;
const TILE_RADIUS = 92;
const RIM_WIDTH = 3;
const RIM = [0xcc, 0xb8, 0x78];
const RIM_ALPHA = 0.45;
// #1a1a17 at the middle, #0c0c0b at 58% of the way out, black at the edge.
const CENTRE = [0x1a, 0x1a, 0x17];
const MIDDLE = [0x0c, 0x0c, 0x0b];
const MIDDLE_STOP = 0.58;
const PNG_SIGNATURE = Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]);
const CRC_TABLE = Int32Array.from({ length: 256 }, (_, n) => {
  for (let k = 0; k < 8; ++k) n = n & 1 ? 0xedb88320 ^ (n >>> 1) : n >>> 1;
  return n;
});

function crc32(bytes) {
  let crc = -1;
  for (const byte of bytes) crc = CRC_TABLE[(crc ^ byte) & 255] ^ (crc >>> 8);
  return (crc ^ -1) >>> 0;
}

function paeth(left, up, corner) {
  const p = left + up - corner, pa = Math.abs(p - left), pb = Math.abs(p - up), pc = Math.abs(p - corner);
  return pa <= pb && pa <= pc ? left : pb <= pc ? up : corner;
}

const clamp = (value, low, high) => Math.min(high, Math.max(low, value));

// 8-bit RGBA without interlacing, as extract-startup-logo.py writes.
export function decodePng(buffer) {
  if (buffer.length < 8 || !buffer.subarray(0, 8).equals(PNG_SIGNATURE)) throw new Error('Not a PNG image');
  let width = 0, height = 0, offset = 8;
  const data = [];
  while (offset + 12 <= buffer.length) {
    const length = buffer.readUInt32BE(offset), type = buffer.toString('latin1', offset + 4, offset + 8);
    const body = buffer.subarray(offset + 8, offset + 8 + length);
    if (body.length !== length) throw new Error('Truncated PNG chunk');
    offset += 12 + length;
    if (type === 'IHDR') {
      width = body.readUInt32BE(0);
      height = body.readUInt32BE(4);
      if (body[8] !== 8 || body[9] !== 6 || body[12] !== 0) throw new Error('Expected an 8-bit RGBA PNG');
    } else if (type === 'IDAT') data.push(body);
    else if (type === 'IEND') break;
  }
  if (!width || !height || width > 4096 || height > 4096) throw new Error('Unexpected PNG dimensions');
  const stride = width * 4, raw = inflateSync(Buffer.concat(data));
  if (raw.length !== (stride + 1) * height) throw new Error('Unexpected PNG data length');
  const pixels = new Uint8Array(stride * height);
  for (let y = 0; y < height; ++y) {
    const filter = raw[y * (stride + 1)], row = y * stride, source = row + y + 1;
    if (filter > 4) throw new Error('Unexpected PNG filter');
    for (let i = 0; i < stride; ++i) {
      const left = i >= 4 ? pixels[row + i - 4] : 0, up = y ? pixels[row - stride + i] : 0;
      let predicted = 0;
      if (filter === 1) predicted = left;
      else if (filter === 2) predicted = up;
      else if (filter === 3) predicted = (left + up) >> 1;
      else if (filter === 4) predicted = paeth(left, up, y && i >= 4 ? pixels[row - stride + i - 4] : 0);
      pixels[row + i] = raw[source + i] + predicted;
    }
  }
  return { width, height, pixels };
}

export function encodePng({ width, height, pixels }) {
  const stride = width * 4, raw = Buffer.alloc((stride + 1) * height);
  for (let y = 0; y < height; ++y) {
    const row = y * stride, target = row + y + 1;
    raw[target - 1] = 4;
    for (let i = 0; i < stride; ++i) {
      const left = i >= 4 ? pixels[row + i - 4] : 0, up = y ? pixels[row - stride + i] : 0;
      const corner = y && i >= 4 ? pixels[row - stride + i - 4] : 0;
      raw[target + i] = pixels[row + i] - paeth(left, up, corner);
    }
  }
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
  return Buffer.concat([PNG_SIGNATURE, chunk('IHDR', header), chunk('IDAT', deflateSync(raw)),
    chunk('IEND', Buffer.alloc(0))]);
}

// The icon's largest 32-bit image: bottom-up BGRA rows, then the AND mask.
export function decodeIco(buffer) {
  if (buffer.length < 6 || buffer.readUInt16LE(0) !== 0 || buffer.readUInt16LE(2) !== 1) throw new Error('Not an icon');
  let best = null;
  for (let index = 0, count = buffer.readUInt16LE(4); index < count; ++index) {
    const entry = 6 + index * 16;
    if (entry + 16 > buffer.length) throw new Error('Truncated icon directory');
    const width = buffer[entry] || 256, height = buffer[entry + 1] || 256;
    const size = buffer.readUInt32LE(entry + 8), offset = buffer.readUInt32LE(entry + 12);
    if (offset + size > buffer.length) throw new Error('Truncated icon image');
    const image = buffer.subarray(offset, offset + size);
    if (image.length < 40 + width * height * 4 || image.readUInt32LE(0) !== 40 || image.readInt32LE(4) !== width ||
        image.readInt32LE(8) !== height * 2 || image.readUInt16LE(14) !== 32 || image.readUInt32LE(16) !== 0) continue;
    if (!best || width > best.width) best = { width, height, image };
  }
  if (!best) throw new Error('The icon has no 32-bit image');
  const { width, height, image } = best, pixels = new Uint8Array(width * height * 4);
  for (let y = 0; y < height; ++y) {
    for (let x = 0; x < width; ++x) {
      const from = 40 + ((height - 1 - y) * width + x) * 4, to = (y * width + x) * 4;
      pixels[to] = image[from + 2];
      pixels[to + 1] = image[from + 1];
      pixels[to + 2] = image[from];
      pixels[to + 3] = image[from + 3];
    }
  }
  return { width, height, pixels };
}

// The executable's star is opaque, on the near black of its corners (4, 4, 3):
// lifted off that ground, each pixel is the star's colour premultiplied by a
// coverage that grows with its brightness.
function keyedStar({ width, height, pixels }) {
  const keyed = new Float32Array(width * height * 4), ground = [0, 1, 2].map(c => pixels[c] * pixels[3] / 255);
  for (let i = 0; i < pixels.length; i += 4) {
    const covered = pixels[i + 3] / 255;
    for (let c = 0; c < 3; ++c) {
      keyed[i + c] = Math.max(0, pixels[i + c] * covered - ground[c]) * 255 / Math.max(1, 255 - ground[c]);
    }
    keyed[i + 3] = Math.min(255, 3 * Math.max(keyed[i], keyed[i + 1], keyed[i + 2]));
  }
  return { width, height, pixels: keyed };
}

// The logo's lettering and glow, without its transparent margins.
function lettering({ width, height, pixels }) {
  let left = width, top = height, right = -1, bottom = -1;
  for (let y = 0; y < height; ++y) {
    for (let x = 0; x < width; ++x) {
      if (pixels[(y * width + x) * 4 + 3] <= 8) continue;
      left = Math.min(left, x); right = Math.max(right, x);
      top = Math.min(top, y); bottom = Math.max(bottom, y);
    }
  }
  if (right < 0) throw new Error('The logo is empty');
  left = Math.max(0, left - 7); top = Math.max(0, top - 7);
  right = Math.min(width - 1, right + 7); bottom = Math.min(height - 1, bottom + 7);
  const cropWidth = right - left + 1, cropHeight = bottom - top + 1;
  const cropped = new Float32Array(cropWidth * cropHeight * 4);
  for (let y = 0; y < cropHeight; ++y) {
    for (let x = 0; x < cropWidth; ++x) {
      const from = ((top + y) * width + left + x) * 4, to = (y * cropWidth + x) * 4, alpha = pixels[from + 3] / 255;
      cropped[to] = pixels[from] * alpha;
      cropped[to + 1] = pixels[from + 1] * alpha;
      cropped[to + 2] = pixels[from + 2] * alpha;
      cropped[to + 3] = pixels[from + 3];
    }
  }
  return { width: cropWidth, height: cropHeight, pixels: cropped };
}

// Mitchell-Netravali, B = C = 1/3: enlarges the 48 px star without ringing.
function mitchell(x) {
  x = Math.abs(x);
  if (x < 1) return (7 * x * x * x - 12 * x * x + 16 / 3) / 6;
  if (x < 2) return (-7 / 3 * x * x * x + 12 * x * x - 20 * x + 32 / 3) / 6;
  return 0;
}

function filterTaps(source, target) {
  const scale = target / source, shrink = Math.min(1, scale), support = 2 / shrink, taps = [];
  for (let i = 0; i < target; ++i) {
    const center = (i + 0.5) / scale - 0.5, row = [];
    let total = 0;
    for (let j = Math.floor(center - support) + 1; j <= Math.floor(center + support); ++j) {
      const weight = mitchell((j - center) * shrink);
      if (!weight) continue;
      row.push(clamp(j, 0, source - 1), weight);
      total += weight;
    }
    for (let k = 1; k < row.length; k += 2) row[k] /= total;
    taps.push(row);
  }
  return taps;
}

function resize(image, width, height) {
  if (image.width === width && image.height === height) return image;
  const columns = filterTaps(image.width, width), rows = filterTaps(image.height, height);
  const across = new Float32Array(width * image.height * 4), result = new Float32Array(width * height * 4);
  for (let y = 0; y < image.height; ++y) {
    for (let x = 0; x < width; ++x) {
      const taps = columns[x], to = (y * width + x) * 4;
      for (let k = 0; k < taps.length; k += 2) {
        const from = (y * image.width + taps[k]) * 4, weight = taps[k + 1];
        for (let c = 0; c < 4; ++c) across[to + c] += image.pixels[from + c] * weight;
      }
    }
  }
  for (let y = 0; y < height; ++y) {
    const taps = rows[y];
    for (let x = 0; x < width; ++x) {
      const to = (y * width + x) * 4;
      for (let k = 0; k < taps.length; k += 2) {
        const from = (taps[k] * width + x) * 4, weight = taps[k + 1];
        for (let c = 0; c < 4; ++c) result[to + c] += across[from + c] * weight;
      }
    }
  }
  return { width, height, pixels: result };
}

// startup.css's radial gradient: an ellipse 75% x 65% of the size about
// (50%, 42%), on a rounded tile with a rim unless the platform masks it.
function tile(pixels, size, fullBleed) {
  const unit = size / GRID;
  const near = TILE_INSET * unit, far = size - near, radius = TILE_RADIUS * unit, rim = RIM_WIDTH * unit;
  for (let y = 0; y < size; ++y) {
    const py = y + 0.5, dy = (py - size * 0.42) / (size * 0.65);
    for (let x = 0; x < size; ++x) {
      const px = x + 0.5, reach = Math.min(1, Math.hypot((px - size * 0.5) / (size * 0.75), dy));
      const outer = reach >= MIDDLE_STOP, low = outer ? MIDDLE : CENTRE;
      const keep = outer ? 1 - (reach - MIDDLE_STOP) / (1 - MIDDLE_STOP) : 1, t = outer ? 0 : reach / MIDDLE_STOP;
      let r = (low[0] + (MIDDLE[0] - low[0]) * t) * keep, g = (low[1] + (MIDDLE[1] - low[1]) * t) * keep;
      let b = (low[2] + (MIDDLE[2] - low[2]) * t) * keep, covered = 1;
      if (!fullBleed) {
        const qx = Math.max(near + radius - px, 0, px - (far - radius));
        const qy = Math.max(near + radius - py, 0, py - (far - radius));
        const inside = qx || qy ? radius - Math.hypot(qx, qy) : Math.min(px - near, far - px, py - near, far - py);
        const gold = clamp(rim - inside + 0.5, 0, 1) * RIM_ALPHA;
        covered = clamp(inside + 0.5, 0, 1);
        r += (RIM[0] - r) * gold; g += (RIM[1] - g) * gold; b += (RIM[2] - b) * gold;
      }
      const at = (y * size + x) * 4;
      pixels[at] = r * covered;
      pixels[at + 1] = g * covered;
      pixels[at + 2] = b * covered;
      pixels[at + 3] = 255 * covered;
    }
  }
}

// Premultiplied source-atop: nothing is drawn past the tile's edge.
function draw(canvas, size, image, left, top, width, height) {
  const placed = resize(image, Math.round(width), Math.round(height));
  const x0 = Math.round(left), y0 = Math.round(top);
  for (let y = 0; y < placed.height; ++y) {
    if (y0 + y < 0 || y0 + y >= size) continue;
    for (let x = 0; x < placed.width; ++x) {
      if (x0 + x < 0 || x0 + x >= size) continue;
      const from = (y * placed.width + x) * 4, to = ((y0 + y) * size + x0 + x) * 4;
      const alpha = clamp(placed.pixels[from + 3], 0, 255) / 255, under = canvas[to + 3] / 255;
      for (let c = 0; c < 3; ++c) {
        canvas[to + c] = clamp(placed.pixels[from + c], 0, 255 * alpha) * under + canvas[to + c] * (1 - alpha);
      }
    }
  }
}

// Draws into scratch buffers at least size x size, shared by every icon.
function renderIcon(star, logo, { size, fullBleed = false, scale = 1 }, scratch) {
  const canvas = scratch.canvas.subarray(0, size * size * 4), unit = size / GRID * scale;
  tile(canvas, size, fullBleed);
  const at = (x, y) => [size / 2 + (x - GRID / 2) * unit, size * PIVOT_Y / GRID + (y - PIVOT_Y) * unit];
  draw(canvas, size, star, ...at(STAR.x, STAR.y), STAR.width * unit, STAR.height * unit);
  const fit = Math.min(1, LETTERING.width / logo.width, LETTERING.height / logo.height);
  const [left, top] = at(LETTERING.x + (LETTERING.width - logo.width * fit) / 2,
    LETTERING.y + (LETTERING.height - logo.height * fit) / 2);
  draw(canvas, size, logo, left, top, logo.width * fit * unit, logo.height * fit * unit);
  const pixels = scratch.pixels.subarray(0, size * size * 4);
  for (let i = 0; i < pixels.length; i += 4) {
    const alpha = clamp(canvas[i + 3], 0, 255);
    for (let c = 0; c < 3; ++c) pixels[i + c] = alpha ? Math.round(clamp(canvas[i + c], 0, alpha) * 255 / alpha) : 0;
    pixels[i + 3] = Math.round(alpha);
  }
  return { width: size, height: size, pixels };
}

// icon: the executable's ICO; logo: the decoded logo_cod2 PNG. Returns the
// PNG for each name in APP_ICONS.
export function renderAppIcons(icon, logo) {
  const star = keyedStar(decodeIco(icon)), letters = lettering(decodePng(logo));
  const largest = Math.max(...Object.values(APP_ICONS).map(layout => layout.size)) ** 2 * 4;
  const scratch = { canvas: new Float32Array(largest), pixels: new Uint8Array(largest) };
  return Object.fromEntries(Object.entries(APP_ICONS).map(([name, layout]) =>
    [name, encodePng(renderIcon(star, letters, layout, scratch))]));
}
