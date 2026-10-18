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
    gl.attachShader(scene, shader(gl.FRAGMENT_SHADER, '#version 300 es\nprecision highp float;uniform vec4 u_color;uniform float u_checker;uniform float u_cell;out vec4 o;\n' +
      'void main(){vec2 p=floor(gl_FragCoord.xy/u_cell);float c=mod(p.x+p.y,2.0);o=vec4(mix(u_color.rgb,u_color.rgb*0.2,c*u_checker),1.0);}'));
    gl.linkProgram(scene);
    check(gl.getProgramParameter(scene, gl.LINK_STATUS), 'fixture program linked');
    const sceneVao = gl.createVertexArray();
    const location = {};
    for (const name of ['u_rect', 'u_z', 'u_tilt', 'u_color', 'u_checker', 'u_cell']) location[name] = gl.getUniformLocation(scene, name);
    const rect = (x0, y0, x1, y1, z, color, checker, tilt = 0, cell = 2) => {
      gl.uniform4f(location.u_rect, x0, y0, x1, y1); gl.uniform1f(location.u_z, z); gl.uniform1f(location.u_tilt, tilt);
      gl.uniform4f(location.u_color, color[0], color[1], color[2], 1); gl.uniform1f(location.u_checker, checker);
      gl.uniform1f(location.u_cell, cell);
      gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
    };
    // Window depths of the infinite projection: the world with r_znear 4, and the
    // DX7 viewmodel pass in depth range [0, 0.2] with a 0.1 near clip.
    const c = 0.9995002746582031, near = 0.1, worldNear = c * 4;
    const vmDepth = z => 2 * (0.2 * (c - near / z)) - 1;
    const worldDepth = z => 2 * (c - worldNear / z) - 1;
    function drawScene(layout = fixture) {
      gl.useProgram(scene); gl.bindVertexArray(sceneVao);
      gl.viewport(0, 0, W, H);
      gl.disable(gl.BLEND); gl.enable(gl.DEPTH_TEST); gl.depthFunc(gl.LESS); gl.depthMask(true); gl.colorMask(true, true, true, true);
      gl.clearColor(0, 0, 0, 1); gl.clearDepth(1); gl.clearStencil(0);
      gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT | gl.STENCIL_BUFFER_BIT);
      layout();
    }
    // Black distant world, a white world square at the centre and a white
    // viewmodel strip on the right.
    function squares() {
      rect(-1, -1, 1, 1, worldDepth(2000), [0, 0, 0], 0);
      rect(-0.1, -0.1, 0.1, 0.1, worldDepth(1999), [1, 1, 1], 0);
      rect(0.5, -1, 1, 1, vmDepth(5), [1, 1, 1], 0);
    }
    function fixture() {
      rect(-1, -1, 1, 1, worldDepth(2000), [0.55, 0.5, 0.42], 1);                // distant world texture detail
      rect(0.55, 0.55, 0.75, 0.8, worldDepth(1800), [1, 1, 1], 0);              // sky highlight
      rect(-1, -1, -0.3, -0.3, worldDepth(1500), [0.08, 0.08, 0.08], 0);        // dark wall
      rect(-0.7, -0.9, -0.45, -0.4, worldDepth(1400), [0.9, 0.9, 0.9], 0, 0.3); // slanted edge on it
      rect(-0.28, -1, 0.12, -0.85, worldDepth(120), [0.5, 0.45, 0.35], 1, 0, 6);  // ground at the feet
      rect(-0.28, -0.85, 0.12, -0.7, worldDepth(380), [0.5, 0.45, 0.35], 1, 0, 6); // ground further out
      rect(0.15, -1, 0.95, -0.25, vmDepth(5), [0.4, 0.33, 0.25], 1, 0, 6);      // receiver near the eye
      rect(0.4, -0.15, 0.95, 0.15, vmDepth(9.6), [0.62, 0.5, 0.42], 1, 0, 6);   // trigger hand ahead of it
      rect(-0.012, -0.25, 0.012, -0.02, vmDepth(20), [0.6, 0.6, 0.6], 1);       // front sight post
    }
    // Camera motion since the last frame, as WebSceneFx_TrackCamera hands it
    // over: a column-major rotation and an offset from this view's space into
    // the last one's, for a 90-degree 16:9 projection.
    const turn = (angle, scale = 1) => ({scale, move: [0, 0, 0],
      rotation: [Math.cos(angle), 0, -Math.sin(angle), 0, 1, 0, Math.sin(angle), 0, Math.cos(angle)]});
    const strafe = (units, scale = 1) => ({scale, move: [units, 0, 0], rotation: [1, 0, 0, 0, 1, 0, 0, 0, 1]});
    // The production viewmodel range for a muzzle 28 units out (a Thompson).
    function params(ads, {glow = 1, film = 1, nearEnd = 512, motion = null, backdrop = 0} = {}) {
      HEAPF32.set([0.42 * glow, 0.30 * glow, 0.9, 0.75, 5, 12, ads, 4.55, 17.5, near, 3,
        film ? 0.147 : 0, 0.96, film ? 0.35 : 0, film, worldNear, nearEnd, 1,
        ...(motion ? [motion.scale, ...motion.rotation, ...motion.move] : Array(13).fill(0)), 1, 16 / 9, backdrop], 16);
      return 64;
    }
    function read() {
      const pixels = new Uint8Array(W * H * 4);
      gl.readPixels(0, 0, W, H, gl.RGBA, gl.UNSIGNED_BYTE, pixels);
      return pixels;
    }
    function render(samples, ads, options) {
      check(bind(gl, Module, HEAPF32, W, H, samples) === 1, `offscreen scene bound with ${samples} sample(s)`);
      drawScene(options?.layout);
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
    const receiver = [0.3, -0.9, 0.85, -0.35], hand = [0.45, -0.1, 0.9, 0.1], post = [-0.008, -0.2, 0.008, -0.05];
    const centreWorld = [-0.05, 0.02, 0.05, 0.12], edgeWorld = [0.0, -0.6, 0.14, -0.3];
    const feet = [-0.26, -0.98, 0.0, -0.87], ground = [-0.26, -0.83, 0.0, -0.72];
    const kept = (px, r) => region(px, ...r).variance / region(graded, ...r).variance;
    check(region(graded, ...receiver).variance > region(raw, ...receiver).variance * 0.8, 'hip fire keeps receiver texture detail');
    check(region(graded, -0.4, 0.1, 0.4, 0.5).variance > region(raw, -0.4, 0.1, 0.4, 0.5).variance * 0.8, 'hip fire keeps world detail');
    check(region(graded, ...feet).variance > region(raw, ...feet).variance * 0.8, 'hip fire keeps the ground at the feet detailed');
    check(kept(aim, receiver) < 0.1, 'aiming blurs the receiver near the eye');
    check(kept(aim, hand) > 0.1 && kept(aim, hand) < 0.9 && kept(aim, hand) > 20 * kept(aim, receiver),
      'the hand ahead of the receiver softens far less than the receiver');
    check(same(aim, graded, post), 'aiming keeps the front sight beyond the viewmodel focus range sharp');
    check(kept(aim, feet) < 0.2, 'aiming blurs the ground at the feet');
    check(!same(aim, graded, ground) && kept(aim, ground) < 0.97 && kept(aim, ground) > 3 * kept(aim, feet),
      'ground further out softens less, easing into focus');
    check(same(aim, graded, centreWorld), 'aiming keeps the target at the screen centre sharp');
    check(!same(aim, graded, edgeWorld), 'the blurred receiver softens its silhouette over the world');
    check(same(aim, graded, [-0.95, 0.2, -0.6, 0.9]), 'the distant world stays sharp');
    check(same(render(4, 1, {nearEnd: 100}), graded, feet), 'ground beyond a nearer crosshair focus stays sharp');
    const halo = [0.50, 0.6, 0.545, 0.75];
    check(region(graded, ...halo).mean > region(raw, ...halo).mean + 6, 'r_glow blooms around bright highlights');
    check(region(aim, ...halo).mean > region(noGlow, ...halo).mean + 6, 'r_glowBloomIntensity scales the bloom');
    check(Math.abs(region(aim, -0.95, 0.2, -0.6, 0.5).mean - region(noGlow, -0.95, 0.2, -0.6, 0.5).mean) < 1,
      'mid-tone world surfaces do not bloom');
    const wall = [-0.98, -0.95, -0.32, -0.32], partial = px => region(px, ...wall).values.filter(v => v > 40 && v < 200).length;
    check(partial(render(4, 0, {glow: 0, film: 0})) > partial(plain) + 100, 'r_aaSamples multisamples the offscreen scene');

    // Camera motion blur. A turn of 0.03 rad moves the centre 19.2 pixels.
    check(same(render(4, 0, {motion: turn(0.03, 0)}), graded, [-1, -1, 1, 1]), 'without camera motion nothing smears');
    const turning = render(4, 0, {motion: turn(0.03)}); snapshot('turn');
    check(kept(turning, feet) < 0.5 && kept(turning, ground) < 0.5, 'turning smears the world near and far');
    check(same(turning, graded, receiver) && same(turning, graded, hand), 'turning leaves the viewmodel sharp');
    // Strafing 2 units moves the ground at 120 units 10.7 pixels, at 380 units
    // 3.4, and the world at 1,400 units and beyond under one.
    const strafing = render(4, 0, {motion: strafe(2)}); snapshot('strafe');
    check(kept(strafing, feet) < 0.6 && kept(strafing, ground) > 1.3 * kept(strafing, feet),
      'strafing smears near surfaces more than farther ones');
    check(same(strafing, graded, [-0.95, 0.2, -0.6, 0.9]) && same(strafing, graded, centreWorld),
      'strafing leaves the distant world sharp');
    check(same(strafing, graded, receiver), 'strafing leaves the viewmodel sharp');
    const pixel = (px, x, y) => px[(y * W + x) * 4 + 1];
    // The square covers pixels 576-703 by 324-395 and the strip starts at 960.
    const smeared = render(4, 0, {glow: 0, film: 0, layout: squares, motion: turn(0.03)}); snapshot('turn-squares');
    check(pixel(smeared, 708, 360) > 20 && pixel(smeared, 571, 360) > 20, 'a turn smears the square sideways');
    check(pixel(smeared, 718, 360) === 0 && pixel(smeared, 561, 360) === 0, 'the smear spans the turn and no more');
    check(pixel(smeared, 640, 400) === 0 && pixel(smeared, 640, 319) === 0, 'a turn does not smear vertically');
    check(pixel(smeared, 955, 360) === 0 && pixel(smeared, 950, 200) === 0, 'the viewmodel stays out of the world smear');
    check(pixel(smeared, 960, 360) === 255 && pixel(smeared, 962, 100) === 255, 'the viewmodel edge stays sharp');
    const flick = render(4, 0, {glow: 0, film: 0, layout: squares, motion: turn(0.03, 20)});
    check(pixel(flick, 716, 360) > 0 && pixel(flick, 728, 360) === 0 && pixel(flick, 551, 360) === 0,
      'a flick smears 6% of the screen height at most');

    // Behind the end of a match's scoreboard the whole view, the viewmodel
    // too, blurs by about 20 pixels at 720 lines. Across the square's right
    // edge the light falls from 84% to 16% over two deviations.
    const backdrop = render(4, 0, {glow: 0, film: 0, layout: squares, backdrop: 1}); snapshot('backdrop');
    const peak = pixel(backdrop, 640, 360);
    let x84 = 640, x16 = 640;
    while (x84 < W - 1 && pixel(backdrop, x84, 360) > 0.84 * peak) ++x84;
    while (x16 < W - 1 && pixel(backdrop, x16, 360) > 0.16 * peak) ++x16;
    const deviation = (x16 - x84) / 2;
    check(deviation > 14 && deviation < 28, `the backdrop blurs with a ${deviation}-pixel deviation`);
    check(pixel(backdrop, 958, 360) > 0 && pixel(backdrop, 962, 360) < 255, 'the backdrop blurs the viewmodel too');
    const halfway = render(4, 0, {glow: 0, film: 0, layout: squares, backdrop: 0.5});
    check(pixel(halfway, 720, 360) > 0 && pixel(halfway, 720, 360) < pixel(backdrop, 720, 360),
      'half the backdrop blurs less');

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
    check(passes(0, {glow: 0, motion: turn(0.03)}) === '1+1', 'motion blur adds no pass');
    check(passes(0, {glow: 0, backdrop: 1}) === '5+1', 'the backdrop adds four 1/4-resolution passes');

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
