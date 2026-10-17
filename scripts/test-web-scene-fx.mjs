// Real WebGL2 checks for the scene compositor in downstream/wasm/web_scene_fx.c.
import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath, pathToFileURL} from 'node:url';

const [playwrightModule, outputDir = 'out/web-scene-fx-qa'] = process.argv.slice(2);
if (!playwrightModule) throw Error('Usage: node scripts/test-web-scene-fx.mjs PLAYWRIGHT_MODULE [OUTPUT_DIR]');
const {chromium} = await import(pathToFileURL(path.resolve(playwrightModule)));
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const source = fs.readFileSync(path.join(root, 'downstream/wasm/web_scene_fx.c'), 'utf8');
function emjs(name) {
  const header = source.indexOf(', ' + name + ', (');
  if (header < 0) throw Error(name);
  const start = source.indexOf('{', source.indexOf(')', header));
  let depth = 0, quote = false;
  for (let i = start; i < source.length; ++i) {
    const c = source[i];
    if (quote) { if (c === '\\') ++i; else if (c === '"') quote = false; continue; }
    if (c === '"') quote = true;
    else if (c === '{') ++depth;
    else if (c === '}' && !--depth) return source.slice(start + 1, i);
  }
  throw Error('unterminated ' + name);
}
const adapter = fs.readFileSync(path.join(root, 'site/native-game-adapter.js'), 'utf8');
const cache = adapter.slice(adapter.indexOf('  function cacheWebGlState('), adapter.indexOf('  function performanceMeter('));
fs.mkdirSync(outputDir, {recursive: true});

const browser = await chromium.launch({headless: false, executablePath: process.env.CHROME_EXECUTABLE || undefined,
  args: ['--disable-background-timer-throttling', '--disable-renderer-backgrounding']});
try {
  const page = await browser.newPage({viewport: {width: 1300, height: 760}});
  await page.setContent('<body style="margin:0;background:#111"><canvas width="1280" height="720"></canvas></body>');
  const result = await page.evaluate(({bindBody, composeBody, cache}) => {
    const canvas = document.querySelector('canvas');
    const gl = canvas.getContext('webgl2', {antialias: false, alpha: false, depth: true, stencil: true, preserveDrawingBuffer: true});
    const checks = [], images = {};
    const check = (value, message) => { if (!value) throw Error(message); checks.push(message); };
    check(!!gl, 'WebGL2 available');
    // Production wraps the context with its redundant-state cache first.
    Function(cache + 'return cacheWebGlState;')()(gl, canvas);
    const Module = {}, HEAPF32 = new Float32Array(64);
    const warnings = [], warn = console.warn;
    console.warn = (...args) => { warnings.push(args.join(' ')); warn(...args); };
    const bind = Function('GLctx', 'Module', 'HEAPF32', 'width', 'height', 'samples', bindBody);
    const compose = Function('GLctx', 'Module', 'HEAPF32', 'params', composeBody);
    const W = 1280, H = 720;

    const shader = (type, text) => { const s = gl.createShader(type); gl.shaderSource(s, text); gl.compileShader(s);
      if (!gl.getShaderParameter(s, gl.COMPILE_STATUS)) throw Error(gl.getShaderInfoLog(s)); return s; };
    const scene = gl.createProgram();
    gl.attachShader(scene, shader(gl.VERTEX_SHADER, '#version 300 es\nuniform vec4 u_rect;uniform float u_z;uniform float u_tilt;\n' +
      'void main(){vec2 p=vec2((gl_VertexID&1)==1?u_rect.z:u_rect.x,(gl_VertexID&2)==2?u_rect.w:u_rect.y);p.x+=u_tilt*p.y;gl_Position=vec4(p,u_z,1.0);}'));
    gl.attachShader(scene, shader(gl.FRAGMENT_SHADER, '#version 300 es\nprecision highp float;uniform vec4 u_color;uniform float u_checker;out vec4 o;\n' +
      'void main(){vec2 p=floor(gl_FragCoord.xy/2.0);float c=mod(p.x+p.y,2.0);o=vec4(mix(u_color.rgb,u_color.rgb*0.2,c*u_checker),1.0);}'));
    gl.linkProgram(scene);
    check(gl.getProgramParameter(scene, gl.LINK_STATUS), 'fixture program linked');
    const sceneVao = gl.createVertexArray();
    const location = {};
    for (const name of ['u_rect', 'u_z', 'u_tilt', 'u_color', 'u_checker']) location[name] = gl.getUniformLocation(scene, name);
    const rect = (x0, y0, x1, y1, z, color, checker, tilt = 0) => {
      gl.uniform4f(location.u_rect, x0, y0, x1, y1); gl.uniform1f(location.u_z, z); gl.uniform1f(location.u_tilt, tilt);
      gl.uniform4f(location.u_color, color[0], color[1], color[2], 1); gl.uniform1f(location.u_checker, checker);
      gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
    };
    // Window depth of the DX7 viewmodel pass: depth range [0, 0.2], 0.1 near clip.
    const c = 0.9995002746582031, near = 0.1;
    const vmDepth = z => 2 * (0.2 * (c - near / z)) - 1;
    function drawScene() {
      gl.useProgram(scene); gl.bindVertexArray(sceneVao);
      gl.viewport(0, 0, W, H);
      gl.disable(gl.BLEND); gl.enable(gl.DEPTH_TEST); gl.depthFunc(gl.LESS); gl.depthMask(true); gl.colorMask(true, true, true, true);
      gl.clearColor(0, 0, 0, 1); gl.clearDepth(1); gl.clearStencil(0);
      gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT | gl.STENCIL_BUFFER_BIT);
      rect(-1, -1, 1, 1, 0.8, [0.55, 0.5, 0.42], 1);                    // world texture detail
      rect(0.55, 0.55, 0.75, 0.8, 0.79, [1, 1, 1], 0);                  // sky highlight
      rect(-1, -1, -0.3, -0.3, 0.79, [0.08, 0.08, 0.08], 0);            // dark wall
      rect(-0.7, -0.9, -0.45, -0.4, 0.78, [0.9, 0.9, 0.9], 0, 0.3);     // slanted edge on it
      rect(0.15, -1, 0.95, -0.25, vmDepth(5), [0.4, 0.33, 0.25], 1);    // receiver near the eye
      rect(-0.012, -0.25, 0.012, -0.02, vmDepth(20), [0.6, 0.6, 0.6], 1); // front sight post
    }
    function params(ads, {glow = 1, film = 1} = {}) {
      HEAPF32.set([0.42 * glow, 0.30 * glow, 0.9, 0.75, 5, 12, ads, 6, 15, near, 6,
        film ? 0.147 : 0, 0.96, film ? 0.35 : 0, film], 16);
      return 64;
    }
    function read() {
      const pixels = new Uint8Array(W * H * 4);
      gl.readPixels(0, 0, W, H, gl.RGBA, gl.UNSIGNED_BYTE, pixels);
      return pixels;
    }
    function render(samples, ads, options) {
      check(bind(gl, Module, HEAPF32, W, H, samples) === 1, `offscreen scene bound with ${samples} sample(s)`);
      drawScene();
      compose(gl, Module, HEAPF32, params(ads, options));
      return read();
    }
    function direct() { gl.bindFramebuffer(gl.FRAMEBUFFER, null); drawScene(); return read(); }
    const region = (px, x0, y0, x1, y1) => {   // normalized device coordinates
      const values = [];
      const X0 = Math.round((x0 + 1) / 2 * W), X1 = Math.round((x1 + 1) / 2 * W);
      const Y0 = Math.round((y0 + 1) / 2 * H), Y1 = Math.round((y1 + 1) / 2 * H);
      for (let y = Y0; y < Y1; ++y) for (let x = X0; x < X1; ++x) values.push(px[(y * W + x) * 4 + 1]);
      const mean = values.reduce((a, b) => a + b, 0) / values.length;
      return {mean, variance: values.reduce((a, b) => a + (b - mean) ** 2, 0) / values.length, values};
    };
    const same = (a, b, r) => { const A = region(a, ...r).values, B = region(b, ...r).values; return A.every((v, i) => v === B[i]); };
    const snapshot = name => { images[name] = canvas.toDataURL('image/png'); };

    const raw = direct(); snapshot('direct');
    const plain = render(1, 0, {glow: 0, film: 0});
    check(same(plain, raw, [-1, -1, 1, 1]), 'with every effect off the composed frame equals direct rendering');
    const graded = render(4, 0); snapshot('hipfire');
    const aim = render(4, 1); snapshot('aim');
    const noGlow = render(4, 1, {glow: 0});
    const receiver = [0.3, -0.9, 0.85, -0.35], post = [-0.008, -0.2, 0.008, -0.05];
    const centreWorld = [-0.05, 0.02, 0.05, 0.12], edgeWorld = [0.0, -0.6, 0.14, -0.3];
    check(region(graded, ...receiver).variance > region(raw, ...receiver).variance * 0.8, 'hip fire keeps receiver texture detail');
    check(region(graded, -0.4, 0.1, 0.4, 0.5).variance > region(raw, -0.4, 0.1, 0.4, 0.5).variance * 0.8, 'hip fire keeps world detail');
    check(region(aim, ...receiver).variance < region(graded, ...receiver).variance * 0.1, 'aiming blurs the receiver near the eye');
    check(same(aim, graded, post), 'aiming keeps the front sight beyond the viewmodel focus range sharp');
    check(same(aim, graded, centreWorld), 'aiming keeps the target at the screen centre sharp');
    check(!same(aim, graded, edgeWorld), 'the blurred receiver softens its silhouette over the world');
    check(same(aim, graded, [-0.95, 0.2, -0.6, 0.9]), 'world away from the viewmodel stays sharp');
    const halo = [0.50, 0.6, 0.545, 0.75];
    check(region(graded, ...halo).mean > region(raw, ...halo).mean + 6, 'r_glow blooms around bright highlights');
    check(region(aim, ...halo).mean > region(noGlow, ...halo).mean + 6, 'r_glowBloomIntensity scales the bloom');
    check(Math.abs(region(aim, -0.95, 0.2, -0.6, 0.5).mean - region(noGlow, -0.95, 0.2, -0.6, 0.5).mean) < 1,
      'mid-tone world surfaces do not bloom');
    const wall = [-0.98, -0.95, -0.32, -0.32], partial = px => region(px, ...wall).values.filter(v => v > 40 && v < 200).length;
    check(partial(render(4, 0, {glow: 0, film: 0})) > partial(plain) + 100, 'r_aaSamples multisamples the offscreen scene');

    // Each pass has a fixed GPU cost, so the pass count is part of the frame budget.
    const passes = (ads, options) => {
      bind(gl, Module, HEAPF32, W, H, 4); drawScene();
      const drawArrays = gl.drawArrays, blit = gl.blitFramebuffer;
      let draws = 0, blits = 0;
      gl.drawArrays = function(...args) { ++draws; return drawArrays.apply(this, args); };
      gl.blitFramebuffer = function(...args) { ++blits; return blit.apply(this, args); };
      compose(gl, Module, HEAPF32, params(ads, options));
      gl.drawArrays = drawArrays; gl.blitFramebuffer = blit;
      return `${draws}+${blits}`;
    };
    check(passes(0, {glow: 0}) === '1+1', 'grade and sharpening alone: one resolve and one full-screen pass');
    check(passes(0) === '3+1', 'glow adds two 1/8-resolution passes');
    check(passes(1) === '5+1', 'aiming adds two 1/4-resolution passes');

    // Production state is restored exactly, including the cached WebGL entry points.
    check(bind(gl, Module, HEAPF32, W, H, 4) === 1, 'rebind without reallocating');
    drawScene();
    const otherVao = gl.createVertexArray(), otherTexture = gl.createTexture();
    gl.activeTexture(gl.TEXTURE3); gl.bindTexture(gl.TEXTURE_2D, otherTexture);
    gl.bindVertexArray(otherVao); gl.viewport(10, 20, 300, 200);
    const caps = [gl.SCISSOR_TEST, gl.BLEND, gl.DEPTH_TEST, gl.CULL_FACE, gl.STENCIL_TEST, gl.POLYGON_OFFSET_FILL];
    for (const cap of caps) gl.enable(cap);
    gl.colorMask(true, false, true, false); gl.depthMask(true);
    // Chrome answers these from client state; anything else waits on the GPU process.
    const cached = new Set([gl.CURRENT_PROGRAM, gl.VERTEX_ARRAY_BINDING, gl.ACTIVE_TEXTURE, gl.VIEWPORT]);
    const queried = [], getParameter = gl.getParameter, checkStatus = gl.checkFramebufferStatus;
    gl.getParameter = function(pname) { queried.push(pname); return getParameter.call(this, pname); };
    gl.checkFramebufferStatus = function(target) { queried.push('checkFramebufferStatus'); return checkStatus.call(this, target); };
    check(bind(gl, Module, HEAPF32, W, H, 4) === 1, 'steady-state bind');
    gl.useProgram(scene);
    compose(gl, Module, HEAPF32, params(1));
    gl.getParameter = getParameter; gl.checkFramebufferStatus = checkStatus;
    check(queried.length && queried.every(pname => cached.has(pname)), 'a composed frame makes no synchronous GL queries');
    check(gl.getParameter(gl.CURRENT_PROGRAM) === scene, 'program restored');
    check(gl.getParameter(gl.VERTEX_ARRAY_BINDING) === otherVao, 'vertex array restored');
    check(gl.getParameter(gl.ACTIVE_TEXTURE) === gl.TEXTURE3 && gl.getParameter(gl.TEXTURE_BINDING_2D) === otherTexture, 'texture unit restored');
    check(String(gl.getParameter(gl.VIEWPORT)) === '10,20,300,200', 'viewport restored');
    check(String(gl.getParameter(gl.COLOR_WRITEMASK)) === 'true,true,true,true' && gl.getParameter(gl.DEPTH_WRITEMASK) === true,
      'composite writes every channel and leaves depth writes alone');
    check(caps.every(cap => gl.isEnabled(cap)), 'capabilities restored');
    check(gl.getParameter(gl.FRAMEBUFFER_BINDING) === null, 'later 2D draws reach the canvas');
    for (let unit = 8; unit < 14; ++unit) {
      gl.activeTexture(gl.TEXTURE0 + unit);
      check(gl.getParameter(gl.TEXTURE_BINDING_2D) === null, 'post texture unit ' + unit + ' released');
    }
    gl.activeTexture(gl.TEXTURE3);
    // The state cache observed every change: a following disable still reaches GL.
    gl.disable(gl.BLEND); check(!gl.isEnabled(gl.BLEND), 'state cache stays coherent with GL');
    for (const cap of caps) gl.disable(cap);
    gl.colorMask(true, true, true, true);
    check(gl.getError() === gl.NO_ERROR, 'no WebGL errors');
    check(!warnings.length, 'no compositor warnings');
    return {checks, images};
  }, {bindBody: emjs('WebSceneFx_Bind'), composeBody: emjs('WebSceneFx_Compose'), cache});
  for (const [name, url] of Object.entries(result.images))
    fs.writeFileSync(path.join(outputDir, name + '.png'), Buffer.from(url.split(',')[1], 'base64'));
  console.log(result.checks.map(message => 'ok - ' + message).join('\n'));
  console.log(`PASS: ${result.checks.length} real WebGL2 scene compositor checks; images in ${outputDir}`);
} finally {
  await browser.close();
}
