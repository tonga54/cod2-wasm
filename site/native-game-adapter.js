(function () {
  'use strict';
  let native;
  let failed = false;
  let dataSet;
  let factoryPromise;
  let moduleAssetQuery = '';
  let clientAttributeWarningReported = false;
  let traceInput = () => {};
  let hadInputCapture = false;
  let chatCancelledCapture = false;
  let keyboardCaptureAfter = 0;
  let updateNotifier;
  let gametag;
  const gametagStorageKey = 'cod2-wasm-gametag';
  const startupStyle = document.createElement('link');
  startupStyle.rel = 'stylesheet';
  startupStyle.href = '/startup.css';
  document.head.appendChild(startupStyle);
  function validGametag(value) {
    const name = String(value || '').trim();
    // The native client has a 32-byte name buffer and parses console commands.
    if (!name || /^(Player|Unknown Soldier)$/i.test(name) ||
        /[\x00-\x1f\x7f";+\\]/.test(name) || new TextEncoder().encode(name).length > 31) return '';
    return name;
  }
  const profileStorageKey = 'cod2-wasm-profile-v1';
  // Saved profiles hold the old 1280x720 default; each takes the new one once.
  const displayStorageKey = 'cod2-wasm-display-v2';
  // Likewise for the old full-resolution normal and specular maps.
  const materialMapsStorageKey = 'cod2-wasm-material-maps-v1';
  let browserVersion = 'Browser dev';
  function prepareProfile(context, fs) {
    let saved = '';
    let legacyName;
    try {
      saved = localStorage.getItem(profileStorageKey) || '';
      legacyName = localStorage.getItem(gametagStorageKey);
      if (!localStorage.getItem(displayStorageKey)) {
        saved = saved.replace(/^seta r_mode "1280x720"\r?$/gm, '');
        localStorage.setItem(profileStorageKey, saved);
        localStorage.setItem(displayStorageKey, '1');
      }
      if (!localStorage.getItem(materialMapsStorageKey)) {
        saved = saved.replace(/^seta r_picmip_(?:bump|spec) "0"\r?$/gm, '');
        localStorage.setItem(profileStorageKey, saved);
        localStorage.setItem(materialMapsStorageKey, '1');
      }
    } catch { /* Storage can be disabled; settings still work for this visit. */ }
    gametag = validGametag(legacyName) || validGametag(context.preferences?.values().playerName) || 'Player';
    const defaults = {
      // The canvas and HUD at 1080 lines; the 3D view renders at r_sceneHeight.
      name: gametag, r_gpuSync: 'off', com_maxfps: '0', r_mode: '1920x1080',
      r_textureMode: 'anisotropic', r_anisotropy: '16', r_aaSamples: '4',
      // Normal and specular maps at half resolution: a quarter of their memory.
      r_picmip_manual: '1', r_picmip: '0', r_picmip_bump: '1', r_picmip_spec: '1', cl_maxpackets: '60', rate: '25000',
      snaps: '30', g_gametype: 'tdm', ui_mapname: 'mp_toujane', ui_netSource: '0',
      ui_browserShowEmpty: '1', ui_browserShowFull: '1', ui_browserShowPure: '0',
      ui_browserMod: '-1', ui_browserFriendlyfire: '-1', ui_browserKillcam: '-1'
    };
    // Defaults run first, then the native config (including key bindings).
    // Saved latched graphics values therefore also survive a fresh engine.
    const initial = Object.entries(defaults).map(([key, value]) => `seta ${key} "${value}"`).join('\n');
    fs.mkdirTree('/profile/main/players/default');
    fs.writeFile('/profile/main/players/active.txt', 'default');
    fs.writeFile('/profile/main/players/default/config_mp.cfg', initial + '\n' + saved + '\n');
  }
  function saveProfile(context, config) {
    try {
      // Password dvars are not player preferences.
      const clean = config.split('\n').filter(line => !/^seta\s+(?:password|rcon_password|cl_cdkey)\s/i.test(line)).join('\n');
      localStorage.setItem(profileStorageKey, clean);
    } catch {
      context.log('Browser storage is unavailable; settings will last only for this visit.', 'error');
    }
  }
  function showOriginalStartup(context) {
    const { loading, loadingTitle, loadingProgress, loadingStatus } = context.elements;
    loading.classList.add('cod2-startup');
    const logo = document.createElement('img');
    logo.src = '/game-data/files/cod2-startup/original.png';
    logo.alt = 'Call of Duty 2';
    logo.width = 512;
    logo.height = 128;
    logo.draggable = false;
    loadingTitle.replaceChildren(logo);
    loadingStatus.lang = 'en';
    loadingProgress.setAttribute('aria-label', 'Call of Duty 2 loading progress');
  }
  function downloadProgress(context) {
    const files = new Map(dataSet.policies.map(file => [file.key, { size: file.size, received: 0 }]));
    const total = [...files.values()].reduce((sum, file) => sum + file.size, 0);
    const megabytes = new Intl.NumberFormat('en-US', { minimumFractionDigits: 1, maximumFractionDigits: 1 });
    return event => {
      const file = files.get(event.key);
      if (event.key && !file) return;
      if (file) {
        const received = event.phase === 'downloading' ? event.received :
          ['validated', 'restored', 'cached'].includes(event.phase) ? file.size : 0;
        file.received = Math.max(file.received, Math.min(file.size, Number(received) || 0));
      }
      const received = [...files.values()].reduce((sum, file) => sum + file.received, 0);
      const percent = total ? Math.floor(received / total * 100) : 100;
      const message = event.phase === 'downloading' ?
        (percent === 100 ? 'Verifying files…' : 'Downloading files…') :
        event.phase === 'checking-cache' ? 'Checking cached files…' : 'Preparing files…';
      context.setLoading(`${message} ${percent}%`,
        `${megabytes.format(received / 1000000)} / ${megabytes.format(total / 1000000)} MB`, percent);
    };
  }
  function filePolicy(file, context) {
    const policy = { ...file, mountName: file.path };
    const expected = String(file.sha256 || '').toLowerCase();
    if (!/^[a-f0-9]{64}$/.test(expected)) throw new Error(`Invalid SHA-256 for ${file.name}.`);
    // Web Crypto can only hash a whole archive held in one buffer (400 MB for
    // the renderer IWD). Hash every download in bounded slices in a worker
    // instead, on HTTP and HTTPS alike. A cached copy was hashed before it was
    // stored and the cache drops it when the manifest version changes, so a
    // start only checks its size and signature.
    delete policy.sha256;
    policy.validateCached = false;
    policy.validate = blob => new Promise((resolve, reject) => {
      const worker = new Worker('/asset-sha256.js');
      const started = performance.now();
      worker.onmessage = event => {
        worker.terminate();
        if (event.data.error) reject(new Error(event.data.error));
        else if (event.data.digest !== expected) reject(new Error(`SHA-256 mismatch: ${file.name}.`));
        else {
          context.log(`[assets] SHA-256 verified: ${file.name} (${Math.round(performance.now() - started)} ms)`);
          resolve();
        }
      };
      worker.onerror = () => {
        worker.terminate();
        reject(new Error(`Unable to verify SHA-256 for ${file.name}.`));
      };
      worker.postMessage({ file: blob });
    });
    return policy;
  }
  // Archives at least this large stay in browser storage and are read on
  // demand; copying cod2_browser_renderer.iwd into the page held 400 MB for
  // the whole session. Smaller archives (sounds the engine streams during a
  // match) are still copied into memory.
  const LAZY_ARCHIVE_BYTES = 128 * 1024 * 1024;
  const ARCHIVE_CHUNK_BYTES = 256 * 1024;
  const ARCHIVE_CACHE_BYTES = 32 * 1024 * 1024;
  const DOWNLOAD_PART_BYTES = 8 * 1024 * 1024;
  function createChunkCache(limit) {
    const chunks = new Map();
    let bytes = 0;
    return {
      get(key) {
        const chunk = chunks.get(key);
        if (chunk) { chunks.delete(key); chunks.set(key, chunk); }
        return chunk;
      },
      set(key, chunk) {
        chunks.set(key, chunk);
        bytes += chunk.length;
        for (const [oldest, old] of chunks) {
          if (bytes <= limit || chunks.size === 1) break;
          chunks.delete(oldest);
          bytes -= old.length;
        }
      },
      get bytes() { return bytes; }
    };
  }
  let archiveChunks;
  const BOM_GUARD = new Uint8Array(1);
  function readRangeByXhr(blob, start, end) {
    // Without a worker, a page's main thread can only read a Blob
    // synchronously through a synchronous XHR of an object URL, as
    // x-user-defined text. Decoding sniffs a byte order mark first, so a
    // leading zero byte keeps a range starting with FF FE, FE FF or EF BB BF
    // from being read as Unicode.
    const url = URL.createObjectURL(new Blob([BOM_GUARD, blob.slice(start, end)]));
    try {
      const request = new XMLHttpRequest();
      request.open('GET', url, false);
      request.overrideMimeType('text/plain; charset=x-user-defined');
      request.send(null);
      const text = request.responseText;
      if (text.length !== end - start + 1) throw new Error(`read ${text.length - 1} of ${end - start} bytes`);
      const bytes = new Uint8Array(end - start);
      for (let i = 0; i < bytes.length; ++i) bytes[i] = text.charCodeAt(i + 1);
      return bytes;
    } finally {
      URL.revokeObjectURL(url);
    }
  }
  // Runs in a worker: reads requested Blob ranges with FileReaderSync into
  // shared memory. control: [state (1 request, 2 done, 3 failed), archive,
  // start, end, bytes read].
  function archiveWorker() {
    onmessage = event => {
      const { control, data, blobs } = event.data;
      onmessage = null;
      postMessage('ready');
      const reader = new FileReaderSync();
      let heapChurn;
      for (;;) {
        for (let state; (state = Atomics.load(control, 0)) !== 1;) Atomics.wait(control, 0, state);
        try {
          const bytes = new Uint8Array(reader.readAsArrayBuffer(blobs[control[1]].slice(control[2], control[3])));
          data.set(bytes);
          control[4] = bytes.length;
          Atomics.store(control, 0, 2);
        } catch {
          Atomics.store(control, 0, 3);
        }
        // Each read's buffer is outside the JS heap, and this thread never
        // idles, so nothing else starts a collection: without 128 KiB of heap
        // allocation per read the worker kept every byte it ever read.
        heapChurn = new Array(16384).fill(0);
      }
    };
  }
  // A cross-origin isolated page can hand reads to a worker and wait for its
  // binary result instead of decoding XHR text.
  async function startArchiveWorker(blobs) {
    if (!globalThis.crossOriginIsolated || typeof SharedArrayBuffer !== 'function' || typeof Worker !== 'function')
      throw new Error('the page is not cross-origin isolated');
    const control = new Int32Array(new SharedArrayBuffer(20));
    const data = new Uint8Array(new SharedArrayBuffer(ARCHIVE_CHUNK_BYTES));
    const url = URL.createObjectURL(new Blob([`(${archiveWorker})();`], { type: 'text/javascript' }));
    try {
      await new Promise((resolve, reject) => {
        const worker = new Worker(url);
        const timer = setTimeout(() => reject(new Error('the archive worker did not start')), 5000);
        worker.onmessage = () => { clearTimeout(timer); resolve(); };
        worker.onerror = event => { clearTimeout(timer); reject(new Error(event.message || 'the archive worker failed')); };
        worker.postMessage({ control, data, blobs });
      });
    } finally {
      URL.revokeObjectURL(url);
    }
    const archives = new Map(blobs.map((blob, index) => [blob, index]));
    return (blob, start, end) => {
      control[1] = archives.get(blob);
      control[2] = start;
      control[3] = end;
      Atomics.store(control, 0, 1);
      Atomics.notify(control, 0);
      const deadline = performance.now() + 10000;
      let state;
      while ((state = Atomics.load(control, 0)) === 1)
        if (performance.now() > deadline) throw new Error('the archive worker stopped responding');
      Atomics.store(control, 0, 0);
      if (state !== 2 || control[4] !== end - start) throw new Error(`read ${control[4]} of ${end - start} bytes`);
      return data.slice(0, end - start);
    };
  }
  let readRange = readRangeByXhr;
  function createArchiveReader(blob, id) {
    archiveChunks ||= createChunkCache(ARCHIVE_CACHE_BYTES);
    let lastIndex = -1, last;
    const chunk = index => {
      if (index === lastIndex) return last;
      const key = index * 16 + id;
      let bytes = archiveChunks.get(key);
      if (!bytes) {
        const start = index * ARCHIVE_CHUNK_BYTES;
        bytes = readRange(blob, start, Math.min(blob.size, start + ARCHIVE_CHUNK_BYTES));
        archiveChunks.set(key, bytes);
      }
      lastIndex = index;
      last = bytes;
      return bytes;
    };
    return {
      size: blob.size,
      // Copies [position, position + length) into target at offset.
      copy(position, length, target, offset) {
        let copied = 0;
        while (copied < length) {
          const at = position + copied;
          const index = Math.floor(at / ARCHIVE_CHUNK_BYTES);
          const bytes = chunk(index);
          const within = at - index * ARCHIVE_CHUNK_BYTES;
          const count = Math.min(length - copied, bytes.length - within);
          if (count <= 0) break;
          target.set(bytes.subarray(within, within + count), offset + copied);
          copied += count;
        }
        return copied;
      }
    };
  }
  function probeArchive(read, blob) {
    // Each IWD is a ZIP: a local file header first, the end of central
    // directory record (written without a comment) last.
    const head = read(blob, 0, 4).join();
    const tail = read(blob, blob.size - 22, blob.size - 18).join();
    if (head !== '80,75,3,4' || tail !== '80,75,5,6') throw new Error('unexpected ZIP signatures');
  }
  function mountArchive(FS, path, reader, onError) {
    const node = FS.create(path, 0o444);
    const base = node.stream_ops;
    node.usedBytes = reader.size;
    node.contents = null;
    node.stream_ops = {
      ...base,
      read(stream, buffer, offset, length, position) {
        if (position >= reader.size) return 0;
        const size = Math.min(reader.size - position, length);
        try {
          return reader.copy(position, size, new Uint8Array(buffer.buffer, buffer.byteOffset, buffer.byteLength), offset);
        } catch (error) {
          onError(error);
          throw new FS.ErrnoError(29);
        }
      },
      write() { throw new FS.ErrnoError(63); },
      mmap() { throw new FS.ErrnoError(43); }
    };
    return node;
  }
  async function downloadArchive(client, remote, policy, onProgress) {
    if (!remote) throw new Error(`The container does not expose ${policy.key}.`);
    if (!remote.valid) {
      const error = new Error(`${remote.name || policy.name || policy.key} is not installed in this container.`);
      error.code = 'OWNER_DATA_MISSING';
      throw error;
    }
    const url = new URL(`${client.baseUrl || '/game-data'}/files/${encodeURIComponent(remote.key)}`, location.href);
    if (client.variant) url.searchParams.set('variant', client.variant);
    const response = await fetch(url.href, { cache: 'no-store', credentials: 'same-origin' });
    if (!response.ok) throw new Error(`Downloading ${remote.name} failed with HTTP ${response.status}.`);
    const total = Number(response.headers.get('content-length')) || 0;
    const type = response.headers.get('content-type') || '';
    let parts;
    if (response.body?.getReader) {
      // Fold the stream into bounded Blob parts as it arrives: the browser can
      // keep those out of this page's memory, a list of every chunk it cannot.
      const reader = response.body.getReader();
      parts = [];
      let chunks = [], pending = 0, received = 0;
      for (;;) {
        const { done, value } = await reader.read();
        if (done) break;
        chunks.push(value);
        pending += value.byteLength;
        received += value.byteLength;
        if (pending >= DOWNLOAD_PART_BYTES) { parts.push(new Blob(chunks)); chunks = []; pending = 0; }
        onProgress({ phase: 'downloading', key: remote.key, received, total });
      }
      if (chunks.length) parts.push(new Blob(chunks));
    } else parts = [await response.blob()];
    return new File(parts, remote.name, { type });
  }
  async function loadAssets(context, onProgress) {
    const client = context.dataClient;
    const state = await client.status();
    if (!state.ready) {
      const error = new Error('The container still needs its required game data.');
      error.code = 'CONTAINER_DATA_REQUIRED';
      throw error;
    }
    const remote = new Map(state.files.map(file => [String(file.key).toLowerCase(), file]));
    return dataSet.load(policy => downloadArchive(client, remote.get(String(policy.key).toLowerCase()), policy, onProgress),
      { onProgress });
  }
  // The stored copies of the archives to read on demand, by entry.
  // ?archives=memory copies every archive, ?archives=demand reads every one on
  // demand, ?archives=xhr skips the worker.
  async function onDemandArchives(entries, context) {
    const mode = new URLSearchParams(location.search).get('archives');
    const files = new Map();
    for (const entry of entries) {
      if (mode === 'memory' || (mode !== 'demand' && mode !== 'xhr' && entry.file.size < LAZY_ARCHIVE_BYTES)) continue;
      // A fresh download is still the in-memory copy; read the stored one so
      // the browser can release it.
      let stored = null;
      try { if (!entry.cached) stored = await dataSet.cache?.get?.(entry.key); } catch { /* Keep the downloaded copy. */ }
      files.set(entry, stored?.file?.size === entry.file.size ? stored.file : entry.file);
    }
    if (!files.size) return files;
    const readers = mode === 'xhr' ? [] : [['a worker', () => startArchiveWorker([...files.values()])]];
    readers.push(['synchronous XHR', () => readRangeByXhr]);
    const failures = [];
    for (const [name, start] of readers) {
      try {
        const started = performance.now();
        const read = await start();
        for (const file of files.values()) probeArchive(read, file);
        readRange = read;
        for (const entry of files.keys())
          context.log(`[assets] reading ${entry.mountName} from browser storage on demand through ${name} ` +
            `(${Math.round(performance.now() - started)} ms to start)`);
        return files;
      } catch (error) {
        failures.push(`${name}: ${error?.message || error}`);
        context.log(`[assets] on-demand reads through ${failures.at(-1)}`);
      }
    }
    for (const entry of files.keys())
      context.log(`[assets] copying ${entry.mountName} into memory: on-demand reads are unavailable ` +
        `(${failures.join('; ')})`, 'error');
    return new Map();
  }
  function closeTransport() {
    const transport = native?.cod2Transport;
    if (!transport) return;
    transport.stopped = true;
    transport.packets.length = 0;
    transport.bytes = 0;
    transport.socket.close(1000);
  }
  function failureMessage(reason) {
    return /memory|enlarge|allocat/i.test(String(reason)) ?
      'The browser ran out of memory for the game. Close other tabs and reload the page.' :
      'The game stopped unexpectedly. Reload the page to play again.';
  }
  // Stops the engine for good and tells the player instead of leaving a
  // frozen frame on screen.
  function fail(context, detail, message) {
    if (failed) return;
    failed = true;
    closeTransport();
    try { native?.pauseMainLoop?.(); } catch { /* The runtime may already be gone. */ }
    context.log(detail, 'error');
    context.setEngineState('crashed');
    context.setStatus?.(message, true);
    context.showLauncher?.();
  }
  function cacheWebGlState(gl, canvas) {
    // Fixed-function draws repeatedly submit the same sampler parameters and
    // camera matrices. Keep their values on the JS side to avoid driver calls.
    let unit = gl.TEXTURE0, bindings = new Map();
    let unitBindings = new Map();
    bindings.set(unit, unitBindings);
    let samplers = new WeakMap(), matrices = new WeakMap(), vectors = new WeakMap();
    let scalars = new WeakMap(), locations = new WeakMap();
    let pipeline = new Map(), enabled = new Map();
    const reset = () => {
      unit = gl.TEXTURE0; bindings = new Map();
      unitBindings = new Map(); bindings.set(unit, unitBindings);
      samplers = new WeakMap(); matrices = new WeakMap(); vectors = new WeakMap();
      scalars = new WeakMap(); locations = new WeakMap();
      pipeline = new Map(); enabled = new Map();
    };
    canvas.addEventListener('webglcontextrestored', reset);
    const capabilities = new Set(['BLEND','CULL_FACE','DEPTH_TEST','DITHER',
      'POLYGON_OFFSET_FILL','SAMPLE_ALPHA_TO_COVERAGE','SAMPLE_COVERAGE','SCISSOR_TEST']
      .map(name => gl[name]).filter(value => value !== undefined));
    for (const method of ['enable','disable']) {
      const original = gl[method], value = method === 'enable';
      gl[method] = function (capability) {
        if (!capabilities.has(capability)) return original.call(this, capability);
        if (enabled.get(capability) === value) return;
        original.call(this, capability); enabled.set(capability, value);
      };
    }
    const scalarStates = {
      depthFunc: value => Number.isInteger(value) && value >= 0x0200 && value <= 0x0207,
      cullFace: value => value === 0x0404 || value === 0x0405 || value === 0x0408,
      frontFace: value => value === 0x0900 || value === 0x0901,
      depthMask: value => typeof value === 'boolean'
    };
    for (const [method, valid] of Object.entries(scalarStates)) {
      const original = gl[method];
      gl[method] = function (value) {
        if (!valid(value)) {
          pipeline.delete(method);
          return original.call(this, value);
        }
        if (pipeline.get(method) === value) return;
        original.call(this, value); pipeline.set(method, value);
      };
    }
    const colorMask = gl.colorMask;
    gl.colorMask = function (r, g, b, a) {
      // Normalize booleans just as WebGL does; each channel still updates.
      const value = (r ? 1 : 0) | (g ? 2 : 0) | (b ? 4 : 0) | (a ? 8 : 0);
      if (pipeline.get('colorMask') === value) return;
      colorMask.call(this, r, g, b, a); pipeline.set('colorMask', value);
    };
    const activeTexture = gl.activeTexture;
    gl.activeTexture = function (value) {
      if (unit === value) return;
      activeTexture.call(this, value); unit = value;
      unitBindings = bindings.get(unit);
      if (!unitBindings) { unitBindings = new Map(); bindings.set(unit, unitBindings); }
    };
    const bindTexture = gl.bindTexture;
    gl.bindTexture = function (target, texture) {
      if (unitBindings.has(target) && unitBindings.get(target) === texture) return;
      bindTexture.call(this, target, texture); unitBindings.set(target, texture);
    };
    for (const method of ['texParameteri','texParameterf']) {
      const original = gl[method];
      gl[method] = function (target, parameter, value) {
        const texture = unitBindings.get(target);
        if (!texture) return original.call(this, target, parameter, value);
        let values = samplers.get(texture);
        if (!values) { values = new Map(); samplers.set(texture, values); }
        const previous = values.get(parameter);
        if (previous?.method === method && previous.value === value) return;
        original.call(this, target, parameter, value);
        if (previous) { previous.method = method; previous.value = value; }
        else values.set(parameter, {method, value});
      };
    }
    const deleteTexture = gl.deleteTexture;
    gl.deleteTexture = function (texture) {
      deleteTexture.call(this, texture);
      if (texture) samplers.delete(texture);
      for (const values of bindings.values())
        for (const [target, bound] of values) if (bound === texture) values.set(target, null);
    };
    const uniformMatrix = gl.uniformMatrix4fv;
    const getUniformLocation = gl.getUniformLocation;
    gl.getUniformLocation = function (program, name) {
      if (!program || typeof name !== 'string') return getUniformLocation.call(this, program, name);
      let names = locations.get(program);
      if (!names) { names = new Map(); locations.set(program, names); }
      // The first array element and its base name identify the same uniform.
      const key = name.replace(/\[0\]$/, '');
      if (names.has(key)) return names.get(key);
      const location = getUniformLocation.call(this, program, name);
      names.set(key, location); return location;
    };
    gl.uniformMatrix4fv = function (location, transpose, values) {
      if (!location || transpose || arguments.length !== 3 || values.length !== 16) {
        if (location) matrices = new WeakMap();
        return uniformMatrix.apply(this, arguments);
      }
      let previous = matrices.get(location);
      if (previous) {
        let same = true;
        for (let i = 0; i < 16; ++i) if (previous[i] !== values[i]) { same = false; break; }
        if (same) return;
      } else { previous = new Float32Array(16); matrices.set(location, previous); }
      uniformMatrix.call(this, location, transpose, values);
      previous.set(values);
    };
    const uniformVector = gl.uniform4fv;
    gl.uniform4fv = function (location, values) {
      // Include the existing four-light uniforms. They must not invalidate
      // every material vector when a muzzle flash updates a 16-float array.
      if (!location || arguments.length !== 2 || !values.length ||
          values.length % 4 || values.length > 64) {
        if (location) vectors = new WeakMap();
        return uniformVector.apply(this, arguments);
      }
      let previous = vectors.get(location);
      if (previous && previous.length === values.length) {
        let same = true;
        for (let i = 0; i < values.length; ++i)
          if (previous[i] !== values[i]) { same = false; break; }
        if (same) return;
      }
      uniformVector.call(this, location, values);
      if (!previous || previous.length !== values.length) {
        previous = new Float32Array(values.length); vectors.set(location, previous);
      }
      previous.set(values);
    };
    const uniformScalar = gl.uniform4f;
    gl.uniform4f = function (location, x, y, z, w) {
      if (!location || arguments.length !== 5) {
        if (location) vectors.delete(location);
        return uniformScalar.apply(this, arguments);
      }
      const a = Math.fround(x), b = Math.fround(y), c = Math.fround(z), d = Math.fround(w);
      let previous = vectors.get(location);
      if (previous?.length === 4 && previous[0] === a && previous[1] === b &&
          previous[2] === c && previous[3] === d) return;
      uniformScalar.call(this, location, x, y, z, w);
      if (!previous || previous.length !== 4) {
        previous = new Float32Array(4); vectors.set(location, previous);
      }
      previous[0] = a; previous[1] = b; previous[2] = c; previous[3] = d;
    };
    if (typeof gl.blendFunc === 'function') {
      const blendFunc = gl.blendFunc;
      gl.blendFunc = function (sfactor, dfactor) {
        if (!Number.isInteger(sfactor) || !Number.isInteger(dfactor)) {
          pipeline.delete('blendFunc');
          return blendFunc.apply(this, arguments);
        }
        const key = (sfactor << 16) ^ dfactor;
        if (pipeline.get('blendFunc') === key) return;
        blendFunc.call(this, sfactor, dfactor); pipeline.set('blendFunc', key);
      };
    }
    if (typeof gl.useProgram === 'function') {
      const useProgram = gl.useProgram;
      gl.useProgram = function (program) {
        if (pipeline.get('program') === program && program) return;
        useProgram.call(this, program); pipeline.set('program', program);
      };
    }
    for (const method of ['uniform1i', 'uniform1f']) {
      if (typeof gl[method] !== 'function') continue;
      const original = gl[method];
      gl[method] = function (location, value) {
        if (!location || arguments.length !== 2) {
          if (location) scalars.delete(location);
          return original.apply(this, arguments);
        }
        const normalized = method === 'uniform1f' ? Math.fround(value) : (value | 0);
        const previous = scalars.get(location);
        if (previous?.method === method && Object.is(previous.value, normalized)) return;
        original.call(this, location, value);
        if (previous) { previous.method = method; previous.value = normalized; }
        else scalars.set(location, { method, value: normalized });
      };
    }
    const linkProgram = gl.linkProgram;
    gl.linkProgram = function (program) {
      linkProgram.call(this, program);
      // Linking resets uniforms, even if the program object is reused.
      matrices = new WeakMap(); vectors = new WeakMap(); scalars = new WeakMap();
      pipeline.delete('program');
      if (program) locations.delete(program);
    };
  }
  function performanceMeter(context) {
    if (!new URLSearchParams(location.search).has('perfDebug')) return;
    const output = document.createElement('output');
    output.id = 'performance-debug';
    Object.assign(output.style, {position:'fixed',top:'0',left:'0',zIndex:10000,
      padding:'4px',background:'#000c',color:'#fff',font:'12px monospace',pointerEvents:'none'});
    document.body.appendChild(output);
    let gl, extension, activeQuery, frame = 0, lastStart, lastReport = 0, state;
    let frameStart, phaseStart, stateStart = performance.now();
    const phases = new Map();
    const totals = new Map();
    const frames = [], cpu = [], gpu = [], queries = [];
    const push = (array, value) => { array.push(value); if (array.length > 600) array.shift(); };
    const average = array => array.length ? array.reduce((sum,value)=>sum+value,0)/array.length : null;
    const percentile = (array,p) => array.length ? [...array].sort((a,b)=>a-b)[Math.floor((array.length-1)*p)] : null;
    return {
      attachGL(value) {
        gl = value;
        extension = gl.getExtension('EXT_disjoint_timer_query_webgl2');
      },
      begin() {
        const nextState = native?._web_client_state();
        if (state !== nextState) {
          state = nextState; frames.length = cpu.length = gpu.length = 0;
          phases.clear();
          stateStart = performance.now(); lastStart = undefined;
          if (gl) for (const query of queries.splice(0)) gl.deleteQuery(query);
        }
        frameStart = performance.now();
        totals.clear();
        phaseStart = frameStart;
        if (extension && ++frame % 31 === 0 && queries.length < 4) {
          activeQuery = gl.createQuery();
          gl.beginQuery(extension.TIME_ELAPSED_EXT,activeQuery);
        }
      },
      mark(name) {
        const now = performance.now();
        if (!phases.has(name)) phases.set(name, []);
        push(phases.get(name), now - phaseStart);
        phaseStart = now;
      },
      add(name, milliseconds) {
        totals.set(name, (totals.get(name) || 0) + milliseconds);
      },
      end() {
        const ended = performance.now();
        // Com_Frame may return without drawing to enforce com_maxfps. Count
        // frames submitted by the real renderer, rather than RAF callbacks.
        const rendered = totals.has('backendDraw');
        if (rendered) {
          if (lastStart !== undefined) push(frames, frameStart-lastStart);
          lastStart = frameStart;
          push(cpu,ended-frameStart);
        }
        for (const [name, milliseconds] of totals) {
          if (!phases.has(name)) phases.set(name, []);
          push(phases.get(name), milliseconds);
        }
        if (activeQuery) {
          gl.endQuery(extension.TIME_ELAPSED_EXT);
          if (rendered) queries.push(activeQuery);
          else gl.deleteQuery(activeQuery);
          activeQuery = undefined;
        }
        if (ended-lastReport < 1000) return;
        lastReport = ended;
        if (extension) {
          const disjoint = gl.getParameter(extension.GPU_DISJOINT_EXT);
          if (disjoint) gpu.length = 0;
          while (queries.length && (disjoint || gl.getQueryParameter(queries[0],gl.QUERY_RESULT_AVAILABLE))) {
            const query = queries.shift();
            if (!disjoint) push(gpu,gl.getQueryParameter(query,gl.QUERY_RESULT)/1e6);
            gl.deleteQuery(query);
          }
        }
        const report = {state,seconds:(ended-stateStart)/1000,samples:frames.length,
          fps:frames.length ? 1000/average(frames) : null,frameMsP50:percentile(frames,.5),frameMsP95:percentile(frames,.95),
          frameMsP99:percentile(frames,.99),frameMsMax:frames.length ? Math.max(...frames) : null,
          framesOver33ms:frames.filter(value=>value>33.34).length,
          mainThreadMsP95:percentile(cpu,.95),mainThreadMsP99:percentile(cpu,.99),
          mainThreadMs:average(cpu),gpuMs:average(gpu),gpuSamples:gpu.length,
          phasesMs:Object.fromEntries([...phases].map(([name, samples]) => [name, average(samples)])),
          wasmMemoryBytes:native?.HEAPU8?.byteLength,
          jsHeapBytes:performance.memory?.usedJSHeapSize ?? null,
          canvas:[context.elements.canvas.width,context.elements.canvas.height],visible:!document.hidden};
        report.antialias = gl?.getContextAttributes?.()?.antialias ?? null;
        output.dataset.report = JSON.stringify(report);
        output.textContent = `FPS ${report.fps?.toFixed(1) ?? 'N/A'} | frame p95 ${report.frameMsP95?.toFixed(1) ?? 'N/A'} ms | main ${report.mainThreadMs?.toFixed(1)} ms | GPU ${report.gpuMs?.toFixed(1) ?? 'N/A'} ms | ${report.canvas.join('×')} | WASM ${(report.wasmMemoryBytes/1048576).toFixed(0)} MiB`;
      }
    };
  }
  function traceWebGl(context, meter) {
    const debug = new URLSearchParams(location.search).has('graphicsDebug');
    const canvas = context.elements.canvas;
    const getContext = canvas.getContext;
    canvas.getContext = function (...args) {
      const gl = getContext.apply(this, args);
      if (!gl || !String(args[0]).startsWith('webgl')) return gl;
      canvas.getContext = getContext;
      if (new URLSearchParams(location.search).get('graphicsCache') !== '0') cacheWebGlState(gl, canvas);
      meter?.attachGL(gl);
      if (meter && new URLSearchParams(location.search).has('perfDriver')) {
        for (const name of ['drawElements','bufferSubData','useProgram','uniformMatrix4fv','uniform4fv','vertexAttribPointer','linkProgram']) {
          const original = gl[name];
          if (typeof original !== 'function') continue;
          gl[name] = function (...values) {
            const started = performance.now();
            const result = original.apply(this, values);
            meter.add(`driver.${name}`, performance.now() - started);
            if (name === 'drawElements' || name === 'linkProgram') meter.add(`driver.${name}.count`, 1);
            return result;
          };
        }
      }
      if (!debug) return gl;
      const linkProgram = gl.linkProgram;
      let linksRemaining = 12;
      gl.linkProgram = function (program) {
        linkProgram.call(this, program);
        context.log(`[web-program] ${JSON.stringify({ linked: gl.getProgramParameter(program, gl.LINK_STATUS),
          log: gl.getProgramInfoLog(program),
          shaders: gl.getAttachedShaders(program).map(shader => ({
            type: gl.getShaderParameter(shader, gl.SHADER_TYPE),
            compiled: gl.getShaderParameter(shader, gl.COMPILE_STATUS),
            log: gl.getShaderInfoLog(shader),
            source: gl.getShaderSource(shader) })) })}`);
        if (--linksRemaining === 0) gl.linkProgram = linkProgram;
      };
      const drawElements = gl.drawElements;
      let remaining = 2;
      gl.drawElements = function (...drawArgs) {
        const before = gl.getError();
        const program = gl.getParameter(gl.CURRENT_PROGRAM);
        const elementBuffer = gl.getParameter(gl.ELEMENT_ARRAY_BUFFER_BINDING);
        const arrays = [];
        for (let i = 0; i < gl.getParameter(gl.MAX_VERTEX_ATTRIBS); ++i) {
          if (!gl.getVertexAttrib(i, gl.VERTEX_ATTRIB_ARRAY_ENABLED)) continue;
          arrays.push({ index: i,
            buffer: Boolean(gl.getVertexAttrib(i, gl.VERTEX_ATTRIB_ARRAY_BUFFER_BINDING)),
            size: gl.getVertexAttrib(i, gl.VERTEX_ATTRIB_ARRAY_SIZE),
            type: gl.getVertexAttrib(i, gl.VERTEX_ATTRIB_ARRAY_TYPE),
            stride: gl.getVertexAttrib(i, gl.VERTEX_ATTRIB_ARRAY_STRIDE),
            offset: gl.getVertexAttribOffset(i, gl.VERTEX_ATTRIB_ARRAY_POINTER) });
        }
        drawElements.apply(this, drawArgs);
        context.log(`[web-driver] ${JSON.stringify({ args: drawArgs, before,
          error: gl.getError(), program: Boolean(program),
          linked: program && gl.getProgramParameter(program, gl.LINK_STATUS),
          indexBytes: elementBuffer && gl.getBufferParameter(gl.ELEMENT_ARRAY_BUFFER, gl.BUFFER_SIZE), arrays })}`);
        if (--remaining === 0) gl.drawElements = drawElements;
      };
      return gl;
    };
  }
  let browserBuildId;
  function loadFactory() {
    if (!factoryPromise) factoryPromise = (async () => {
      let buildId;
      try {
        const response = await fetch('/build-info.json', { cache: 'no-store', signal: AbortSignal.timeout(3000) });
        const build = response.ok && await response.json();
        if (/^[a-f0-9]{64}$/.test(build?.buildId)) browserBuildId = buildId = build.buildId;
        if (/^[a-f0-9]{40}$/.test(build?.revision))
          browserVersion = `Browser ${build.revision.slice(0, 8)}${build.dirty ? ' dev' : ''}`;
      } catch { /* Hosts without metadata still load a fresh engine. */ }
      // A page reload revalidates scripts, but a WASM fetch can reuse an old
      // cached binary. Give the JS loader and binary the same build URL.
      moduleAssetQuery = `?build=${buildId || Date.now()}`;
      if (globalThis.createCod2Client) return globalThis.createCod2Client;
      return new Promise((resolve, reject) => {
        const script = document.createElement('script');
        script.src = `/cod2.js${moduleAssetQuery}`;
        script.onload = () => globalThis.createCod2Client
          ? resolve(globalThis.createCod2Client)
          : reject(new Error('The client module did not register its factory.'));
        script.onerror = () => reject(new Error('Unable to load the WebAssembly client.'));
        document.head.appendChild(script);
      });
    })();
    return factoryPromise;
  }
  function startFullscreenControl(context) {
    let pending = false;
    const available = () => typeof document.documentElement?.requestFullscreen === 'function' &&
      document.fullscreenEnabled !== false;
    async function toggle() {
      if (pending || !available() ||
          (globalThis.navigator?.userActivation && !navigator.userActivation.isActive)) return;
      pending = true;
      try {
        if (document.fullscreenElement) await document.exitFullscreen();
        else await document.documentElement.requestFullscreen({ navigationUI: 'hide' });
      } catch { /* A denied request keeps the current display mode. */ } finally {
        pending = false;
      }
    }
    let keyboardGeneration = 0, escapeLocked = false;
    document.addEventListener('fullscreenchange', async () => {
      const generation = ++keyboardGeneration;
      escapeLocked = false;
      const keyboard = globalThis.navigator?.keyboard;
      if (!keyboard?.lock || !keyboard?.unlock) return;
      if (!document.fullscreenElement) { keyboard.unlock(); return; }
      try {
        await keyboard.lock(['Escape']);
        escapeLocked = generation === keyboardGeneration && Boolean(document.fullscreenElement);
        if (generation !== keyboardGeneration && !document.fullscreenElement) keyboard.unlock();
      } catch { /* Optional keyboard capture keeps browser controls usable. */ }
    });
    for (const type of ['keydown', 'keyup']) window.addEventListener(type, event => {
      if (event.isTrusted && event.key === 'Escape' && type === 'keydown' && event.repeat && escapeLocked) {
        event.preventDefault(); event.stopImmediatePropagation(); return;
      }
      if (!event.isTrusted || event.key !== 'F10' || event.ctrlKey || event.metaKey ||
          event.altKey || event.shiftKey) return;
      event.preventDefault();
      event.stopImmediatePropagation();
      if (type === 'keydown' && !event.repeat) toggle();
    }, true);
  }
  function startUpdateNotifier(context) {
    if (!context.config?.updates) return;
    const script = document.createElement('script');
    script.src = '/update-notifier.js';
    script.onload = () => {
      if (typeof globalThis.createCod2UpdateNotifier !== 'function') return;
      updateNotifier = globalThis.createCod2UpdateNotifier({
        loadedBuild: () => browserBuildId,
        setMenuNotice: available => native?._web_client_update_available?.(available ? 1 : 0),
        canNotify: () => !failed && Boolean(native) && native._web_client_state() === 1 &&
          typeof native._web_client_connected === 'function' && !native._web_client_connected()
      });
    };
    // Update checks are optional and must never block startup or gameplay.
    script.onerror = () => context.log('[updates] Unable to load the update notice.');
    document.head.appendChild(script);
  }
  function startInstallOffer(context) {
    // install-app.js shows the browser's install offer over the loading
    // screen, then in the main menu's free right key-prompt place.
    globalThis.cod2InstallMoment = () => !failed && (native?._web_client_state() >= 1 ?
      native._web_right_prompt_free?.() === 1 : !context.elements.loading.hidden);
  }
  globalThis.WasmGameAdapter = Object.freeze({
    async init(context) {
      startFullscreenControl(context);
      startUpdateNotifier(context);
      startInstallOffer(context);
      // Keep right-click available to the game, including before pointer lock.
      context.elements.canvas.addEventListener('contextmenu', event => event.preventDefault());
      // Window capture runs before the framework's document key guard. A
      // console/chat/menu can open while the shell still reports gameplay;
      // refresh that state before Space is prevented (which cancels SDL's
      // keypress/TEXTINPUT event in the browser).
      window.addEventListener('keydown', event => {
        if (!native || failed) return;
        const nativeState = native._web_client_state();
        chatCancelledCapture = nativeState === 3 && event.key === 'Escape';
        const state = this.readEngineState();
        if (state !== context.shell.engineState()) context.setEngineState(state);
        if (nativeState === 3 &&
            document.pointerLockElement === context.elements.canvas &&
            !event.ctrlKey && !event.metaKey && !event.altKey &&
            (event.key === ' ' || event.key === '/'))
          native._web_chat_char(event.key.charCodeAt(0));
        // Escape can release capture at browser level. A subsequent real game
        // key reacquires it without making the player click the scene again.
        if ((nativeState === 2 || nativeState === 3) && event.isTrusted &&
            event.key !== 'Escape' && !event.ctrlKey && !event.metaKey && !event.altKey &&
            document.pointerLockElement !== context.elements.canvas && Date.now() >= keyboardCaptureAfter) {
          keyboardCaptureAfter = Date.now() + 1000;
          context.shell.requestInputCapture(event);
        }
      }, true);
      if (context.config?.autoStart) {
        showOriginalStartup(context);
        context.showLoading();
        context.setLoading('Preparing the game…', '', 0);
        if (context.elements.loadingKicker) context.elements.loadingKicker.hidden = true;
        context.elements.console.hidden = !new URLSearchParams(location.search).has('debug');
      }
      // The framework listener runs first. Remember actual capture so a
      // rejected initial request is not mistaken for losing an existing lock.
      document.addEventListener('pointerlockchange', () => {
        hadInputCapture = document.pointerLockElement === context.elements.canvas;
      });
      if (new URLSearchParams(location.search).has('inputDebug')) {
        let remaining = 40;
        traceInput = event => {
          if (remaining-- <= 0) return;
          context.log(`[input-trace] ${event} focused=${document.hasFocus()} locked=${document.pointerLockElement === context.elements.canvas} state=${native?._web_client_state() ?? -1}`);
        };
        for (const event of ['pointerlockchange', 'pointerlockerror'])
          document.addEventListener(event, () => traceInput(event));
        for (const event of ['focus', 'blur'])
          window.addEventListener(event, () => traceInput(event));
        document.addEventListener('keydown', event => { if (event.key === 'Escape') traceInput('escape-key'); });
        const canvas = context.elements.canvas;
        const requestPointerLock = canvas.requestPointerLock;
        if (requestPointerLock) canvas.requestPointerLock = function (...args) {
          traceInput('request-pointer-lock');
          try {
            const pending = requestPointerLock.apply(this, args);
            pending?.then(() => traceInput('request-resolved'), error => {
              context.log(`[input-trace] request rejected: ${error.name}: ${error.message}`);
            });
            return pending;
          } catch (error) {
            context.log(`[input-trace] request threw: ${error.name}: ${error.message}`);
            throw error;
          }
        };
      }
      const response = await fetch('/wasm-game-data.json', { cache: 'no-store' });
      if (!response.ok) throw new Error(`HTTP ${response.status} while reading the asset policy.`);
      const policy = (await response.json()).variants[context.variant];
      if (!policy) throw new Error('No asset policy is available for this client.');
      dataSet = context.framework.createOwnerDataSet({
        namespace: policy.namespace, version: policy.version,
        files: policy.files.map(file => filePolicy(file, context))
      });
      // WASM traps raised in an animation frame do not invoke onAbort.
      // Report native runtime failures to the framework as well as the console.
      globalThis.addEventListener('error', event => {
        if (!native || failed) return;
        const stack = String(event.error?.stack || '');
        if (!/\/cod2\.(wasm|js)([?:]|$)/.test(stack + '\n' + String(event.filename))) return;
        fail(context, `[cod2-wasm] runtime: ${stack || event.message}`, failureMessage(stack || event.message));
      });
      context.log('[cod2-wasm] Local WebAssembly client: Toujane / Team Deathmatch.');
      if (context.config?.autoStart) {
        try {
          const gate = await context.dataClient.applyGate();
          if (!gate.ready) throw new Error('The game files are not yet available on the server.');
          await this.start(context);
        } catch (error) {
          context.elements.console.hidden = false;
          context.log(error?.message || String(error));
          context.setLoading('Unable to start the game', error?.message || String(error), 0);
          throw error;
        }
      }
    },
    async start(context) {
      if (native || failed) throw new Error('Reload the page to start a new instance.');
      context.setEngineState('loading');
      try {
        context.setLoading('Preparing the game…', '', 0);
        // Fetch the client script during the download but create its heap
        // afterwards, so the two never hold their memory at the same time.
        const factoryReady = loadFactory();
        factoryReady.catch(() => {});
        const onProgress = downloadProgress(context);
        onProgress({ phase: 'checking-cache' });
        const assets = await loadAssets(context, onProgress);
        const factory = await factoryReady;
        const meter = performanceMeter(context);
        traceWebGl(context, meter);
        context.elements.canvas.addEventListener('webglcontextlost', () => fail(context,
          '[cod2-wasm] WebGL context lost',
          'The browser reset the graphics card. Reload the page to play again.'));
        native = await factory({
          cod2Performance: meter,
          cod2SaveProfile: config => saveProfile(context, config),
          cod2ReloadClient: () => updateNotifier?.reload(),
          canvas: context.elements.canvas, noInitialRun: true,
          // Emscripten's default terminal stdin opens window.prompt("Input:").
          // Browser gameplay receives keyboard events through SDL instead.
          stdin: () => null,
          locateFile: path => `/${path}${moduleAssetQuery}`,
          print: line => {
            const text = String(line);
            if (text === "DrawElements doesn't actually prepareClientAttributes properly.") {
              if (clientAttributeWarningReported) return;
              clientAttributeWarningReported = true;
            }
            context.log(text);
          },
          printErr: line => context.log(String(line), 'error'),
          onAbort(reason) {
            fail(context, `[cod2-wasm] abort: ${String(reason)}`, failureMessage(reason));
          }
        });
        const entries = Array.isArray(assets?.entries) ? assets.entries : Array.from(assets || []);
        const onDemand = await onDemandArchives(entries, context);
        context.setLoading('Preparing the game… 0%', '', 0);
        await context.framework.mountOwnerFiles(native, entries.filter(entry => !onDemand.has(entry)), {
          root: '/game', preservePaths: true, mode: 'memfs', chunkBytes: 1024 * 1024,
          onProgress: event => {
            const percent = event.total ? Math.floor(event.copied / event.total * 100) : 100;
            context.setLoading(`Preparing the game… ${percent}%`, '', percent);
          }
        });
        let archiveId = 0;
        for (const [entry, file] of onDemand) {
          const path = `/game/${entry.mountName}`;
          native.FS.mkdirTree(path.slice(0, path.lastIndexOf('/')));
          mountArchive(native.FS, path, createArchiveReader(file, archiveId++), error => fail(context,
            `[assets] reading ${entry.mountName} failed: ${error?.message || error}`,
            'The game files in this browser could not be read. Reload the page to play again.'));
        }
        prepareProfile(context, native.FS);
        native.FS.chdir('/game');
        context.setLoading('Starting the game…', '', 100);
        native.callMain([
          '+set', 'fs_basepath', '/game', '+set', 'fs_homepath', '/profile',
          '+set', 'dedicated', '0', '+set', 'r_rendererPreference', 'dx7',
          '+set', 'com_hunkMegs', '128', '+set', 'r_fullscreen', '0',
          '+set', 'ui_browserVersion', `"${browserVersion}"`
        ]);
        const state = native._web_client_state();
        if (state === 1 || state === 2) context.showRuntime(state === 2 ? 'gameplay' : 'menu');
      } catch (error) {
        failed = true;
        closeTransport();
        context.setEngineState('crashed');
        throw error;
      }
    },
    readEngineState() {
      if (failed) return 'crashed';
      if (!native) return 'launcher';
      const state = native._web_client_state();
      return state === 2 || state === 3 ? 'gameplay' : state === 1 ? 'menu' : 'loading';
    },
    readCaptureIntent() {
      if (failed || !native) return false;
      const state = native._web_client_state();
      return state === 2 || state === 3;
    },
    captureLost() {
      traceInput('captureLost');
      if (!failed && native && hadInputCapture && !chatCancelledCapture) native._web_capture_lost();
      hadInputCapture = false;
      chatCancelledCapture = false;
    }
  });
})();
