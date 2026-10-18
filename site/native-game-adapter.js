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
  let fullscreenControl;
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
  function lanFilePolicy(file, context) {
    const policy = { ...file, mountName: file.path };
    if (globalThis.crypto?.subtle) return policy;
    const expected = String(file.sha256 || '').toLowerCase();
    if (!/^[a-f0-9]{64}$/.test(expected)) throw new Error(`Invalid SHA-256 for ${file.name}.`);
    // The framework's custom validator runs for downloads AND cached files.
    // Preserve the manifest hash in this closure; use the worker instead of
    // its Web-Crypto-only built-in validator on an HTTP LAN origin.
    delete policy.sha256;
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
  function closeTransport() {
    const transport = native?.cod2Transport;
    if (!transport) return;
    transport.stopped = true;
    transport.packets.length = 0;
    transport.bytes = 0;
    transport.socket.close(1000);
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
    let pending = false, failedRequest = false, nativeClient, sentState;
    const available = () => typeof document.documentElement?.requestFullscreen === 'function' &&
      document.fullscreenEnabled !== false;
    function sync(client = nativeClient) {
      nativeClient = client;
      const state = [available() ? (document.fullscreenElement ? 2 : 1) : 0,
        pending ? 1 : 0, failedRequest ? 1 : 0];
      const key = state.join(':');
      if (client?._web_client_fullscreen_state && key !== sentState &&
          client._web_client_fullscreen_state(...state)) sentState = key;
    }
    async function toggle() {
      if (pending || !available() ||
          (globalThis.navigator?.userActivation && !navigator.userActivation.isActive)) return;
      pending = true;
      failedRequest = false;
      sync();
      try {
        if (document.fullscreenElement) await document.exitFullscreen();
        else await document.documentElement.requestFullscreen({ navigationUI: 'hide' });
      } catch {
        failedRequest = true;
      } finally {
        pending = false;
        sync();
      }
    }
    let keyboardGeneration = 0, escapeLocked = false;
    document.addEventListener('fullscreenchange', async () => {
      sync();
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
    return { toggle, sync };
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
  globalThis.WasmGameAdapter = Object.freeze({
    async init(context) {
      fullscreenControl = startFullscreenControl(context);
      startUpdateNotifier(context);
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
        files: policy.files.map(file => lanFilePolicy(file, context))
      });
      // WASM traps raised in an animation frame do not invoke onAbort.
      // Report native runtime failures to the framework as well as the console.
      globalThis.addEventListener('error', event => {
        if (!native || failed) return;
        const stack = String(event.error?.stack || '');
        if (!/\/cod2\.(wasm|js)([?:]|$)/.test(stack + '\n' + String(event.filename))) return;
        failed = true;
        closeTransport();
        context.log(`[cod2-wasm] runtime: ${stack || event.message}`, 'error');
        context.setEngineState('crashed');
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
        const factory = await loadFactory();
        const meter = performanceMeter(context);
        traceWebGl(context, meter);
        native = await factory({
          cod2Performance: meter,
          cod2SaveProfile: config => saveProfile(context, config),
          cod2ReloadClient: () => updateNotifier?.reload(),
          cod2ToggleFullscreen: () => fullscreenControl?.toggle(),
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
            failed = true;
            closeTransport();
            context.log(`[cod2-wasm] abort: ${String(reason)}`, 'error');
            context.setEngineState('crashed');
          }
        });
        const onProgress = downloadProgress(context);
        onProgress({ phase: 'checking-cache' });
        const assets = await context.dataClient.load(dataSet, { onProgress });
        context.setLoading('Preparing the game… 0%', '', 0);
        await context.framework.mountOwnerFiles(native, assets, {
          root: '/game', preservePaths: true, mode: 'memfs', chunkBytes: 1024 * 1024,
          onProgress: event => {
            const percent = event.total ? Math.floor(event.copied / event.total * 100) : 100;
            context.setLoading(`Preparing the game… ${percent}%`, '', percent);
          }
        });
        prepareProfile(context, native.FS);
        native.FS.chdir('/game');
        context.setLoading('Starting the game…', '', 100);
        native.callMain([
          '+set', 'fs_basepath', '/game', '+set', 'fs_homepath', '/profile',
          '+set', 'dedicated', '0', '+set', 'r_rendererPreference', 'dx7',
          '+set', 'com_hunkMegs', '128', '+set', 'r_fullscreen', '0',
          '+set', 'ui_browserVersion', `"${browserVersion}"`
        ]);
        fullscreenControl?.sync(native);
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
      fullscreenControl?.sync(native);
      const state = native._web_client_state();
      return state === 2 || state === 3 ? 'gameplay' : state === 1 ? 'menu' : 'loading';
    },
    readCaptureIntent() {
      if (failed || !native) return false;
      fullscreenControl?.sync(native);
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
