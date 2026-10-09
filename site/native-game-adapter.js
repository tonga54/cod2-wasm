(function () {
  'use strict';
  let native;
  let failed = false;
  let dataSet;
  let factoryPromise;
  let clientAttributeWarningReported = false;
  let traceInput = () => {};
  let hadInputCapture = false;
  let updateNotifier;
  const startupStyle = document.createElement('link');
  startupStyle.rel = 'stylesheet';
  startupStyle.href = '/startup.css';
  document.head.appendChild(startupStyle);
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
    loadingProgress.setAttribute('aria-label', 'Progreso de carga de Call of Duty 2');
  }
  function lanFilePolicy(file, context) {
    const policy = { ...file, mountName: file.path };
    if (globalThis.crypto?.subtle) return policy;
    const expected = String(file.sha256 || '').toLowerCase();
    if (!/^[a-f0-9]{64}$/.test(expected)) throw new Error(`SHA-256 inválido para ${file.name}.`);
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
        else if (event.data.digest !== expected) reject(new Error(`SHA-256 incorrecto: ${file.name}.`));
        else {
          context.log(`[assets] SHA-256 verificado: ${file.name} (${Math.round(performance.now() - started)} ms)`);
          resolve();
        }
      };
      worker.onerror = () => {
        worker.terminate();
        reject(new Error(`No se pudo verificar SHA-256 de ${file.name}.`));
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
  function performanceMeter(context) {
    if (!new URLSearchParams(location.search).has('perfDebug')) return;
    const output = document.createElement('output');
    output.id = 'performance-debug';
    Object.assign(output.style, {position:'fixed',top:'0',left:'0',zIndex:10000,
      padding:'4px',background:'#000c',color:'#fff',font:'12px monospace',pointerEvents:'none'});
    document.body.appendChild(output);
    let gl, extension, activeQuery, frame = 0, lastStart, lastReport = 0, state;
    let frameStart, stateStart = performance.now();
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
          stateStart = performance.now(); lastStart = undefined;
          if (gl) for (const query of queries.splice(0)) gl.deleteQuery(query);
        }
        frameStart = performance.now();
        if (lastStart !== undefined) push(frames,frameStart-lastStart);
        lastStart = frameStart;
        if (extension && ++frame % 30 === 0 && queries.length < 4) {
          activeQuery = gl.createQuery();
          gl.beginQuery(extension.TIME_ELAPSED_EXT,activeQuery);
        }
      },
      end() {
        const ended = performance.now();
        push(cpu,ended-frameStart);
        if (activeQuery) {
          gl.endQuery(extension.TIME_ELAPSED_EXT); queries.push(activeQuery); activeQuery = undefined;
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
          mainThreadMs:average(cpu),gpuMs:average(gpu),gpuSamples:gpu.length,
          wasmMemoryBytes:native?.HEAPU8?.byteLength,
          jsHeapBytes:performance.memory?.usedJSHeapSize ?? null,
          canvas:[context.elements.canvas.width,context.elements.canvas.height],visible:!document.hidden};
        output.dataset.report = JSON.stringify(report);
        output.textContent = `FPS ${report.fps?.toFixed(1) ?? 'n/d'} | frame p95 ${report.frameMsP95?.toFixed(1) ?? 'n/d'} ms | main ${report.mainThreadMs?.toFixed(1)} ms | GPU ${report.gpuMs?.toFixed(1) ?? 'n/d'} ms | WASM ${(report.wasmMemoryBytes/1048576).toFixed(0)} MiB`;
      }
    };
  }
  function traceWebGl(context, meter) {
    const debug = new URLSearchParams(location.search).has('graphicsDebug');
    if (!debug && !meter) return;
    const canvas = context.elements.canvas;
    const getContext = canvas.getContext;
    canvas.getContext = function (...args) {
      const gl = getContext.apply(this, args);
      if (!gl || !String(args[0]).startsWith('webgl')) return gl;
      canvas.getContext = getContext;
      meter?.attachGL(gl);
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
  function loadFactory() {
    if (globalThis.createCod2Client) return Promise.resolve(globalThis.createCod2Client);
    if (!factoryPromise) factoryPromise = new Promise((resolve, reject) => {
      const script = document.createElement('script');
      script.src = '/cod2.js';
      script.onload = () => globalThis.createCod2Client
        ? resolve(globalThis.createCod2Client)
        : reject(new Error('El módulo del cliente no registró su factory.'));
      script.onerror = () => reject(new Error('No se pudo cargar el cliente WebAssembly.'));
      document.head.appendChild(script);
    });
    return factoryPromise;
  }
  function startUpdateNotifier(context) {
    if (!context.config?.updates) return;
    const script = document.createElement('script');
    script.src = '/update-notifier.js';
    script.onload = () => {
      if (typeof globalThis.createCod2UpdateNotifier !== 'function') return;
      updateNotifier = globalThis.createCod2UpdateNotifier({
        canNotify: () => !failed && Boolean(native) && native._web_client_state() === 1 &&
          typeof native._web_client_connected === 'function' && !native._web_client_connected()
      });
    };
    // Update checks are optional and must never block startup or gameplay.
    script.onerror = () => context.log('[updates] No se pudo cargar el aviso de actualización.');
    document.head.appendChild(script);
  }
  globalThis.WasmGameAdapter = Object.freeze({
    async init(context) {
      startUpdateNotifier(context);
      // Keep right-click available to the game, including before pointer lock.
      context.elements.canvas.addEventListener('contextmenu', event => event.preventDefault());
      // Window capture runs before the framework's document key guard. A
      // console/chat/menu can open while the shell still reports gameplay;
      // refresh that state before Space is prevented (which cancels SDL's
      // keypress/TEXTINPUT event in the browser).
      window.addEventListener('keydown', () => {
        if (!native || failed) return;
        const state = this.readEngineState();
        if (state !== context.shell.engineState()) context.setEngineState(state);
      }, true);
      if (context.config?.autoStart) {
        showOriginalStartup(context);
        context.showLoading();
        context.setLoading('Loading…', '', 0);
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
      if (!response.ok) throw new Error(`HTTP ${response.status} al leer la política de assets.`);
      const policy = (await response.json()).variants[context.variant];
      if (!policy) throw new Error('No hay una política para este cliente.');
      dataSet = context.framework.createOwnerDataSet({
        namespace: policy.namespace, version: policy.version,
        files: policy.files.map(file => lanFilePolicy(file, context))
      });
      // WASM traps raised in an animation frame do not invoke onAbort.
      // Report native runtime failures to the framework as well as the console.
      globalThis.addEventListener('error', event => {
        if (!native || failed) return;
        const stack = String(event.error?.stack || '');
        if (!stack.includes('/cod2.wasm:') && !String(event.filename).endsWith('/cod2.js')) return;
        failed = true;
        closeTransport();
        context.log(`[cod2-wasm] runtime: ${stack || event.message}`, 'error');
        context.setEngineState('crashed');
      });
      context.log('[cod2-wasm] Cliente WebAssembly local: Toujane / Team Deathmatch.');
      if (context.config?.autoStart) {
        try {
          const gate = await context.dataClient.applyGate();
          if (!gate.ready) throw new Error('Los archivos del juego todavía no están disponibles en el servidor.');
          await this.start(context);
        } catch (error) {
          context.elements.console.hidden = false;
          context.log(error?.message || String(error));
          context.setLoading('No se pudo iniciar el juego', error?.message || String(error), 0);
          throw error;
        }
      }
    },
    async start(context) {
      if (native || failed) throw new Error('Recargá la página para iniciar una instancia nueva.');
      context.setEngineState('loading');
      try {
        context.setLoading('Loading…', '', 10);
        const factory = await loadFactory();
        const meter = performanceMeter(context);
        traceWebGl(context, meter);
        native = await factory({
          cod2Performance: meter,
          canvas: context.elements.canvas, noInitialRun: true,
          // Emscripten's default terminal stdin opens window.prompt("Input:").
          // Browser gameplay receives keyboard events through SDL instead.
          stdin: () => null,
          locateFile: path => `/${path}`,
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
        context.setLoading('Loading…', '', 50);
        const assets = await context.dataClient.load(dataSet);
        await context.framework.mountOwnerFiles(native, assets, {
          root: '/game', preservePaths: true, mode: 'memfs', chunkBytes: 1024 * 1024
        });
        native.FS.mkdirTree('/profile');
        native.FS.chdir('/game');
        context.setLoading('Loading…', '', 85);
        native.callMain([
          '+set', 'fs_basepath', '/game', '+set', 'fs_homepath', '/profile',
          '+set', 'dedicated', '0', '+set', 'r_rendererPreference', 'dx7',
          '+set', 'r_gpuSync', 'off', '+set', 'com_hunkMegs', '128',
          '+set', 'r_fullscreen', '0', '+set', 'r_mode', '3',
          '+set', 'g_gametype', 'tdm', '+set', 'ui_mapname', 'mp_toujane',
          '+set', 'ui_netSource', '0', '+set', 'ui_browserShowEmpty', '1',
          '+set', 'ui_browserShowFull', '1', '+set', 'ui_browserShowPure', '0',
          '+set', 'ui_browserMod', '-1', '+set', 'ui_browserFriendlyfire', '-1',
          '+set', 'ui_browserKillcam', '-1'
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
      updateNotifier?.render();
      if (failed) return 'crashed';
      if (!native) return 'launcher';
      const state = native._web_client_state();
      return state === 2 ? 'gameplay' : state === 1 ? 'menu' : 'loading';
    },
    readCaptureIntent() {
      return !failed && Boolean(native) && native._web_client_state() === 2;
    },
    captureLost() {
      traceInput('captureLost');
      if (!failed && native && hadInputCapture) native._web_capture_lost();
      hadInputCapture = false;
    }
  });
})();
