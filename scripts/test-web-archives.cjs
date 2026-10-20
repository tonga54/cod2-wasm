// The adapter's archive pipeline against the pinned framework's real owner-data
// cache and copy mount: a download folded into bounded Blob parts, hashed in the
// worker, a large archive read from browser storage on demand through the
// Emscripten node, cached starts without re-hashing, and visible failures.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const {createHash} = require('node:crypto');
const {Worker: Thread} = require('node:worker_threads');
const {sha256Blob} = require('../site/asset-sha256.js');

const frameworkSource = fs.readFileSync(process.argv[2] ||
  path.join(__dirname, '../../wasm-game-framework/dist/wasm-game-framework.js'), 'utf8');
const adapterSource = fs.readFileSync(path.join(__dirname, '../site/native-game-adapter.js'), 'utf8');

// Blobs whose bytes the synchronous XHR stand-in can read, as a browser's blob
// registry serves an object URL. Construction copies raw chunks, as Blob does.
const blobParts = [];
class SyncBlob {
  constructor(parts = [], options = {}) {
    let raw = 0;
    const chunks = parts.map(part => {
      if (part instanceof SyncBlob) return part.bytes;
      const view = typeof part === 'string' ? new TextEncoder().encode(part) :
        part instanceof ArrayBuffer ? new Uint8Array(part) : new Uint8Array(part.buffer, part.byteOffset, part.byteLength);
      raw += view.length;
      return view;
    });
    blobParts.push(raw);
    this.bytes = chunks.length === 1 && parts[0] instanceof SyncBlob ? chunks[0] : Buffer.concat(chunks);
    this.type = String(options.type || '');
  }
  get size() { return this.bytes.length; }
  slice(start = 0, end = this.size, type = '') {
    const blob = Object.create(SyncBlob.prototype);
    blob.bytes = this.bytes.subarray(Math.max(0, start), Math.min(this.size, end));
    blob.type = type;
    return blob;
  }
  async arrayBuffer() {
    return this.bytes.buffer.slice(this.bytes.byteOffset, this.bytes.byteOffset + this.bytes.length);
  }
}
class SyncFile extends SyncBlob {
  constructor(parts, name, options = {}) {
    super(parts, options);
    this.name = name;
    this.lastModified = options.lastModified || 0;
  }
}
const objectUrls = new Map();
let nextUrl = 0;
class TestURL extends URL {
  static createObjectURL(blob) { const url = `blob:test/${++nextUrl}`; objectUrls.set(url, blob); return url; }
  static revokeObjectURL(url) { objectUrls.delete(url); }
}
let syncReads = 0;
let readFailure = null;
class SyncXHR {
  open(method, url, async) {
    assert.equal(method, 'GET');
    assert.equal(async, false, 'the engine reads files synchronously');
    this.url = url;
  }
  overrideMimeType(type) { assert.equal(type, 'text/plain; charset=x-user-defined'); }
  send() {
    ++syncReads;
    if (readFailure) throw readFailure;
    const {bytes} = objectUrls.get(this.url);
    // Decoding sniffs a byte order mark before the declared charset applies.
    if (bytes[0] === 0xff && bytes[1] === 0xfe || bytes[0] === 0xfe && bytes[1] === 0xff) {
      this.responseText = Buffer.from(bytes.subarray(2)).toString('utf16le');
      return;
    }
    if (bytes[0] === 0xef && bytes[1] === 0xbb && bytes[2] === 0xbf) {
      this.responseText = Buffer.from(bytes.subarray(3)).toString('utf8');
      return;
    }
    // x-user-defined maps bytes 0x80-0xff to U+F780-U+F7FF.
    let text = '';
    for (let i = 0; i < bytes.length; i += 8192)
      text += String.fromCharCode(...Array.from(bytes.subarray(i, i + 8192), byte => byte < 0x80 ? byte : 0xf700 + byte));
    this.responseText = text;
  }
}
let hashed = 0, archiveWorkers = 0, blockArchiveWorkers = false;
const threads = [];
const FAILING_OFFSET = 9 * 256 * 1024;
class TestWorker {
  constructor(url) {
    if (url.startsWith('blob:')) return archiveWorker(url);
    assert.equal(url, '/asset-sha256.js');
  }
  postMessage({file}) { ++hashed; sha256Blob(file).then(digest => this.onmessage({data: {digest}})); }
  terminate() {}
}
// The adapter's archive worker source on a real thread, with FileReaderSync
// over the archive bytes; the page waits on it through real shared memory.
function archiveWorker(url) {
  if (blockArchiveWorkers) throw new Error('workers are blocked');
  ++archiveWorkers;
  const prelude = `
    const { parentPort } = require('node:worker_threads');
    class WorkerBlob {
      constructor(bytes, offset = 0) { this.bytes = bytes; this.offset = offset; }
      slice(start, end) { return new WorkerBlob(this.bytes.subarray(start, end), this.offset + start); }
    }
    globalThis.FileReaderSync = class {
      readAsArrayBuffer(blob) {
        if (blob.offset === ${FAILING_OFFSET}) throw new Error('NotReadableError');
        return blob.bytes.slice().buffer;
      }
    };
    globalThis.postMessage = message => parentPort.postMessage(message);
    parentPort.once('message', message =>
      globalThis.onmessage({ data: { ...message, blobs: message.blobs.map(bytes => new WorkerBlob(bytes)) } }));
  `;
  const thread = new Thread(prelude + Buffer.from(objectUrls.get(url).bytes).toString(), { eval: true });
  threads.push(thread);
  const shim = { postMessage(message) { thread.postMessage({ ...message, blobs: message.blobs.map(blob => new Uint8Array(blob.bytes)) }); } };
  thread.on('message', () => shim.onmessage?.({}));
  thread.on('error', error => shim.onerror?.({ message: error.message }));
  return shim;
}

function archive(name, size, seed) {
  const bytes = Buffer.alloc(size);
  for (let i = 0; i < size; i += 4) bytes.writeUInt32LE((Math.imul(i + seed, 2654435761) >>> 0), i);
  // Chunks that begin like byte order marks.
  const chunk = 256 * 1024;
  if (size > 8 * chunk) {
    bytes.set([0xff, 0xfe], 5 * chunk);
    bytes.set([0xfe, 0xff], 6 * chunk);
    bytes.set([0xef, 0xbb, 0xbf], 7 * chunk);
  }
  bytes.set([80, 75, 3, 4], 0);
  bytes.set([80, 75, 5, 6], size - 22);
  return {name, bytes, sha256: createHash('sha256').update(bytes).digest('hex')};
}
const MiB = 1024 * 1024;
const small = archive('cod2_browser_bootstrap.iwd', 4096, 1);
const large = archive('cod2_browser_renderer.iwd', 40 * MiB, 7);
const archives = [small, large];
const manifest = {variants: {test: {namespace: 'cod2-test', version: 'v1', files: archives.map(item => ({
  key: item.name.replace(/_/g, '-').replace('.iwd', ''), name: item.name, path: `main/${item.name}`,
  size: item.bytes.length, magic: [80, 75, 3, 4], sha256: item.sha256
}))}}};
const keyOf = item => manifest.variants.test.files.find(file => file.name === item.name).key;

const downloads = [];
let corruptNextDownload = false;
async function fetchStub(url) {
  if (url === '/wasm-game-data.json') return {ok: true, json: async () => manifest};
  if (url === '/build-info.json') return {ok: false};
  const match = /^http:\/\/test\/game-data\/files\/([^?]+)\?variant=test$/.exec(url);
  assert.ok(match, `unexpected request ${url}`);
  const item = archives.find(entry => keyOf(entry) === match[1]);
  downloads.push(item.name);
  const bytes = Buffer.from(item.bytes);
  if (corruptNextDownload) { bytes[bytes.length - 1] ^= 1; corruptNextDownload = false; }
  let offset = 0;
  return {ok: true, status: 200,
    headers: {get: name => name === 'content-length' ? String(bytes.length) : 'application/zip'},
    body: new ReadableStream({pull(controller) {
      if (offset >= bytes.length) { controller.close(); return; }
      controller.enqueue(new Uint8Array(bytes.subarray(offset, offset + 3 * MiB)));
      offset += 3 * MiB;
    }})};
}
const globals = {Blob: SyncBlob, File: SyncFile, URL: TestURL, XMLHttpRequest: SyncXHR, Worker: TestWorker,
  ReadableStream, AbortSignal, URLSearchParams, TextEncoder, performance, Intl, crypto: {subtle: {}}};
const frameworkSandbox = vm.createContext({...globals});
vm.runInContext(frameworkSource, frameworkSandbox);
const framework = frameworkSandbox.WasmGameFramework;

function emscriptenFs() {
  const files = new Map();
  class ErrnoError extends Error { constructor(errno) { super(`errno ${errno}`); this.errno = errno; } }
  const memfsOps = {
    llseek() {}, write() {}, allocate() {}, mmap() {}, msync() {},
    read(stream, buffer, offset, length, position) {
      const size = Math.max(0, Math.min(stream.node.usedBytes - position, length));
      new Uint8Array(buffer.buffer).set(stream.node.contents.subarray(position, position + size), offset);
      return size;
    }
  };
  const node = () => ({contents: new Uint8Array(0), usedBytes: 0, stream_ops: memfsOps});
  return {files, FS: {
    ErrnoError,
    mkdirTree() {}, chdir() {},
    writeFile(name, data) { files.set(name, {data: String(data)}); },
    stat(name) { if (!files.has(name)) throw new ErrnoError(44); return {size: files.get(name).usedBytes}; },
    open(name, flags) { assert.equal(flags, 'w'); files.set(name, node()); return {node: files.get(name)}; },
    truncate(name, size) { Object.assign(files.get(name), {contents: new Uint8Array(size), usedBytes: size}); },
    write(stream, bytes, offset, length, position) { stream.node.contents.set(bytes.subarray(offset, offset + length), position); },
    close() {}, unlink(name) { files.delete(name); },
    chmod(name, mode) { files.get(name).mode = mode; },
    create(name, mode) { files.set(name, {...node(), mode, created: true}); return files.get(name); }
  }};
}
function read(node, position, length) {
  const heap = new Int8Array(length + 64);
  const count = node.stream_ops.read({node}, heap, 32, length, position);
  return Buffer.from(heap.buffer, 32, count);
}

async function pageLoad(search, {isolated = false} = {}) {
  const {FS, files} = emscriptenFs();
  const listeners = new Map();
  const state = {status: null, launcher: 0, engine: [], paused: 0, logs: [], gets: []};
  const native = {FS, callMain(args) { state.args = args; }, _web_client_state: () => 1,
    pauseMainLoop() { ++state.paused; }};
  const sandbox = vm.createContext({...globals, fetch: fetchStub, crossOriginIsolated: isolated, setTimeout, clearTimeout,
    location: {search, href: 'http://test/'}, addEventListener() {},
    document: {addEventListener() {}, createElement: () => ({}), head: {appendChild() {}}},
    window: {addEventListener() {}},
    createCod2Client: async options => { state.onAbort = options.onAbort; return native; }});
  vm.runInContext(adapterSource, sandbox);
  const canvas = {addEventListener(name, fn) { listeners.set(name, fn); }, getContext: () => null};
  const context = {variant: 'test', config: {}, elements: {canvas, console: {}},
    framework: {...framework, createOwnerDataSet(config) {
      const set = framework.createOwnerDataSet(config);
      return {...set, cache: {...set.cache, get: key => { state.gets.push(key); return set.cache.get(key); }}};
    }},
    dataClient: {baseUrl: '/game-data', variant: 'test', applyGate: async () => ({ready: true}),
      status: async () => ({ready: true, files: manifest.variants.test.files.map(file => ({...file, valid: true}))})},
    preferences: {values: () => ({playerName: 'Tester'})},
    log(line, level) { state.logs.push([String(line), level]); },
    setLoading() {}, showLoading() {}, showRuntime() {},
    setEngineState(value) { state.engine.push(value); },
    setStatus(message, error) { state.status = [message, error]; },
    showLauncher() { ++state.launcher; },
    shell: {engineState: () => 'loading'}};
  const adapter = sandbox.WasmGameAdapter;
  await adapter.init(context);
  await adapter.start(context);
  return {adapter, files, state, listeners};
}
const nodeFor = (files, item) => files.get(`/game/main/${item.name}`);

(async () => {
  // A first visit on demand: both archives are downloaded, hashed once each in
  // the worker and stored; then read from the stored copies.
  let page = await pageLoad('?archives=demand');
  assert.deepEqual(downloads.sort(), archives.map(item => item.name).sort());
  assert.equal(hashed, 2, 'every download is hashed in the worker');
  assert.ok(Math.max(...blobParts) <= 8 * MiB + 3 * MiB,
    `downloaded chunks are folded into bounded parts, not one buffer (${Math.max(...blobParts)} bytes)`);
  assert.deepEqual(page.state.gets.sort(), archives.map(keyOf).sort(),
    'a fresh download is read through the stored copy, not the one in memory');
  for (const item of archives) {
    const node = nodeFor(page.files, item);
    assert.ok(node?.created, `${item.name} is an on-demand node`);
    assert.equal(node.usedBytes, item.bytes.length);
    assert.equal(node.mode, 0o444);
  }
  const lazy = nodeFor(page.files, large);
  const chunk = 256 * 1024;
  for (const [position, length] of [[0, 30], [chunk - 7, 19], [3 * chunk - 1, 2 * chunk + 5], [12345678, 1000000],
                                   [5 * chunk, 3 * chunk + 9], [large.bytes.length - 100, 100],
                                   [large.bytes.length - 10, 4096]]) {
    assert.deepEqual(read(lazy, position, length),
      large.bytes.subarray(position, Math.min(large.bytes.length, position + length)), `read ${position}+${length}`);
  }
  assert.equal(read(lazy, large.bytes.length, 64).length, 0, 'reads at the end return no bytes');
  let before = syncReads;
  read(lazy, chunk + 10, 100);
  assert.equal(syncReads, before, 'a read inside a cached chunk does not touch storage');
  for (let position = 0; position < large.bytes.length; position += chunk) read(lazy, position, chunk);
  before = syncReads;
  read(lazy, large.bytes.length - 50, 10);
  assert.equal(syncReads, before, 'recent chunks stay cached');
  read(lazy, 5, 10);
  assert.equal(syncReads, before + 1, 'the cache holds at most 32 MiB; the oldest chunk was evicted');
  assert.throws(() => lazy.stream_ops.write(), error => error.errno === 63, 'archives are read-only');
  assert.match(page.state.args.join(' '), /\+set fs_basepath \/game/);

  // Storage that can no longer be read stops the engine with a message.
  readFailure = new Error('NetworkError: blob gone');
  assert.throws(() => read(lazy, 2 * MiB + 3, 64), error => error.errno === 29, 'the engine sees EIO');
  readFailure = null;
  assert.equal(page.state.paused, 1);
  assert.equal(page.state.engine.at(-1), 'crashed');
  assert.deepEqual(page.state.status, ['The game files in this browser could not be read. Reload the page to play again.', true]);
  assert.equal(page.state.launcher, 1);
  assert.equal(page.adapter.readEngineState(), 'crashed');

  // A cached start downloads and hashes nothing; size and signature suffice.
  downloads.length = 0;
  hashed = 0;
  page = await pageLoad('');
  assert.deepEqual(downloads, []);
  assert.equal(hashed, 0, 'a cached start does not hash whole archives again');
  assert.deepEqual(page.state.gets, [], 'cached entries are already the stored copies');
  // Archives below 128 MiB are copied into memory by the framework.
  for (const item of archives) {
    const node = nodeFor(page.files, item);
    assert.ok(node && !node.created, `${item.name} is copied`);
    assert.deepEqual(read(node, 0, item.bytes.length), item.bytes);
  }

  // A browser without synchronous Blob reads keeps the copy.
  readFailure = new Error('sync XHR unavailable');
  page = await pageLoad('?archives=demand');
  readFailure = null;
  for (const item of archives) assert.ok(!nodeFor(page.files, item).created, `${item.name} falls back to a copy`);
  assert.ok(page.state.logs.some(([line, level]) => level === 'error' && /copying main\/cod2_browser_renderer\.iwd/.test(line)));
  page = await pageLoad('?archives=memory');
  for (const item of archives) assert.ok(!nodeFor(page.files, item).created, '?archives=memory copies');

  // A cross-origin isolated page reads through the worker: binary ranges in
  // shared memory, no XHR, byte-order-mark lookalikes included.
  page = await pageLoad('?archives=demand', {isolated: true});
  assert.equal(archiveWorkers, 1, 'one worker serves every on-demand archive');
  assert.ok(page.state.logs.some(([line]) => /cod2_browser_renderer\.iwd from browser storage on demand through a worker/.test(line)));
  const viaWorker = nodeFor(page.files, large);
  before = syncReads;
  assert.throws(() => read(viaWorker, FAILING_OFFSET + 3, 64), error => error.errno === 29,
    'a range the worker cannot read is EIO for the engine');
  assert.deepEqual(page.state.status, ['The game files in this browser could not be read. Reload the page to play again.', true]);
  page = await pageLoad('?archives=demand', {isolated: true});
  const fresh = nodeFor(page.files, large);
  for (const [position, length] of [[0, 30], [5 * chunk, 3 * chunk + 9], [12345678, 1000000], [large.bytes.length - 77, 77]])
    assert.deepEqual(read(fresh, position, length), large.bytes.subarray(position, position + length), `worker read ${position}`);
  assert.deepEqual(read(nodeFor(page.files, small), 0, small.bytes.length), small.bytes);
  assert.equal(syncReads, before, 'the worker path never falls back to XHR');
  // ?archives=xhr, or a worker that cannot start, reads through XHR instead.
  page = await pageLoad('?archives=xhr', {isolated: true});
  assert.ok(page.state.logs.some(([line]) => /through synchronous XHR/.test(line)));
  blockArchiveWorkers = true;
  page = await pageLoad('?archives=demand', {isolated: true});
  blockArchiveWorkers = false;
  assert.ok(page.state.logs.some(([line]) => /through synchronous XHR/.test(line)));
  assert.deepEqual(read(nodeFor(page.files, large), 7 * chunk - 5, 10), large.bytes.subarray(7 * chunk - 5, 7 * chunk + 5));

  // A lost WebGL context and an out-of-memory abort stop the engine visibly.
  page = await pageLoad('');
  page.listeners.get('webglcontextlost')({});
  assert.deepEqual(page.state.status, ['The browser reset the graphics card. Reload the page to play again.', true]);
  assert.equal(page.state.paused, 1);
  page = await pageLoad('');
  page.state.onAbort('Cannot enlarge memory arrays to size 4294967296 bytes (OOM)');
  assert.deepEqual(page.state.status,
    ['The browser ran out of memory for the game. Close other tabs and reload the page.', true]);

  // A corrupted download is rejected before it is stored or mounted.
  await framework.createDataCache({namespace: 'cod2-test', version: 'v1'}).clear();
  corruptNextDownload = true;
  await assert.rejects(pageLoad(''), /SHA-256 mismatch/);
  console.log('PASS: downloads folded into bounded parts and hashed in the worker on every origin, large archives ' +
    'read on demand from the stored copy through a read-only node with a 32 MiB chunk cache, by a shared-memory ' +
    'worker when cross-origin isolated and by XHR otherwise, cached starts without re-hashing, copy fallback, and ' +
    'visible failures for unreadable storage, a lost GPU context and memory exhaustion');
})().catch(error => { console.error(error); process.exitCode = 1; })
  .finally(() => Promise.all(threads.map(thread => thread.terminate())));
