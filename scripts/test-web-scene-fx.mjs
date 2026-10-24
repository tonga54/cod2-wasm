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
  const result = await page.evaluate(({bindBody, composeBody, stencilBody, maskBody, cache}) => {
    const canvas = document.querySelector('canvas');
    const gl = canvas.getContext('webgl2', {antialias: false, alpha: false, depth: true, stencil: true, preserveDrawingBuffer: true});
    const checks = [], images = {};
    const check = (value, message) => { if (!value) throw Error(message); checks.push(message); };
    check(!!gl, 'WebGL2 available');
    // Production wraps the context with its redundant-state cache first.
    Function(cache + 'return cacheWebGlState;')()(gl, canvas);
    const Module = {}, HEAPF32 = new Float32Array(1 << 19);
    const warnings = [], warn = console.warn;
    console.warn = (...args) => { warnings.push(args.join(' ')); warn(...args); };
    const bindScene = Function('GLctx', 'Module', 'HEAPF32', 'width', 'height', 'samples', 'screenWidth', 'screenHeight', bindBody);
    const bind = (gl, Module, HEAPF32, width, height, samples, screenWidth = width, screenHeight = height) =>
      bindScene(gl, Module, HEAPF32, width, height, samples, screenWidth, screenHeight);
    const compose = Function('GLctx', 'Module', 'HEAPF32', 'params', 'mesh', composeBody);
    const stencil = Function('Module', 'HEAPF32', 'scope', 'bell', stencilBody);
    const mask = Function('Module', 'on', maskBody);
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
    function params(ads, {glow = 1, film = 1, nearEnd = 512, motion = null, backdrop = 0, gray = 0, vm = null} = {}) {
      HEAPF32.set([0.42 * glow, 0.30 * glow, 0.9, 0.75, 5, 12, ads, 4.55, 17.5, near, 3,
        film ? 0.147 : 0, 0.96, film ? 0.35 : 0, film, worldNear, nearEnd, 1,
        ...(motion ? [motion.scale, ...motion.rotation, ...motion.move] : Array(13).fill(0)), 1, 16 / 9, backdrop, gray,
        ...(vm ? [vm.smear, vm.triangles, vm.indexRow, vm.tileRow, vm.rows, vm.tile, vm.tilesX] : Array(7).fill(0))], 16);
      return 64;
    }
    // The viewmodel mesh as WebSceneFx_TrackViewmodel lays it out in rows of
    // 1024 texels: two a vertex (GL clip position, half smear in pixels), four
    // indices a texel, then for each 72-pixel tile the largest smear within
    // one tile of it. Each quad is a rectangle in normalized device
    // coordinates, z units from the eye, with a half smear in pixels.
    const MESH = 4096;
    function viewmodelMesh(quads) {
      const tile = Math.ceil(0.10 * H), tilesX = Math.ceil(W / tile), tilesY = Math.ceil(H / tile);
      const indexRow = Math.ceil(quads.length * 8 / 1024), tileRow = indexRow + Math.ceil(quads.length * 6 / 4096);
      const rows = tileRow + Math.ceil(tilesX * tilesY / 1024);
      const mesh = new Float32Array(rows * 4096), tiles = new Float32Array(tilesX * tilesY * 2);
      const speedAt = (v, i) => Math.hypot(v[i], v[i + 1]);
      let smear = 0;
      quads.forEach(({rect: [x0, y0, x1, y1], z, smear: v}, n) => {
        const depth = 2 * (c - near / z) - 1;
        [[x0, y0], [x1, y0], [x1, y1], [x0, y1]].forEach(([x, y], k) =>
          mesh.set([x * z, y * z, depth * z, z, v[0], v[1], 0, 0], (n * 4 + k) * 8));
        [0, 1, 2, 0, 2, 3].forEach((k, i) => { mesh[indexRow * 4096 + n * 6 + i] = n * 4 + k; });
        smear = Math.max(smear, speedAt(v, 0));
        for (let ty = Math.floor((y0 + 1) / 2 * H / tile); ty <= Math.floor((y1 + 1) / 2 * H / tile); ++ty)
          for (let tx = Math.floor((x0 + 1) / 2 * W / tile); tx <= Math.floor((x1 + 1) / 2 * W / tile); ++tx)
            if (speedAt(v, 0) > speedAt(tiles, (ty * tilesX + tx) * 2)) tiles.set(v, (ty * tilesX + tx) * 2);
      });
      for (let ty = 0; ty < tilesY; ++ty)
        for (let tx = 0; tx < tilesX; ++tx) {
          let best = (ty * tilesX + tx) * 2;
          for (let y = Math.max(ty - 1, 0); y <= Math.min(ty + 1, tilesY - 1); ++y)
            for (let x = Math.max(tx - 1, 0); x <= Math.min(tx + 1, tilesX - 1); ++x)
              if (speedAt(tiles, (y * tilesX + x) * 2) > speedAt(tiles, best)) best = (y * tilesX + x) * 2;
          mesh.set([tiles[best], tiles[best + 1]], (tileRow * 1024 + ty * tilesX + tx) * 4);
        }
      HEAPF32.set(mesh, MESH);
      return {smear, triangles: quads.length * 2, indexRow, tileRow, rows, tile, tilesX};
    }
    function read() {
      const pixels = new Uint8Array(W * H * 4);
      gl.readPixels(0, 0, W, H, gl.RGBA, gl.UNSIGNED_BYTE, pixels);
      return pixels;
    }
    function render(samples, ads, options) {
      check(bind(gl, Module, HEAPF32, W, H, samples) === 1, `offscreen scene bound with ${samples} sample(s)`);
      drawScene(options?.layout);
      compose(gl, Module, HEAPF32, params(ads, options), MESH * 4);
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
    check(pixel(flick, 730, 360) > 0 && pixel(flick, 741, 360) === 0 && pixel(flick, 538, 360) === 0,
      'a flick smears 10% of the screen height at most');

    // r_atmosphere: the map's haze over world pixels by their distance, denser
    // below the eye than above it, brighter toward the sun; the viewmodel and
    // the sky keep their own. Parameters 57-72 as WebSceneFx_FillParams lays them.
    const hazeAt = (density, sunColor = [0, 0, 0], sun = [0, 0, 1]) =>
      HEAPF32.set([1, 16 / 9, 0.2, 0.5, 0.8, density, 0, 1, 0, ...sun, ...sunColor, 1 / 640], 16 + 57);
    function hazeLayout() {
      rect(-1, -1, 0.5, 0.6, worldDepth(2000), [0, 0, 0], 0);   // the world 2,000 units away
      rect(0.5, -1, 1, 1, vmDepth(5), [1, 1, 1], 0);           // the viewmodel; the sky above the world
    }
    const clearAir = render(1, 0, {glow: 0, film: 0, layout: hazeLayout});
    hazeAt(0);
    check(same(render(1, 0, {glow: 0, film: 0, layout: hazeLayout}), clearAir, [-1, -1, 1, 1]), 'no fog, no haze');
    hazeAt(0.0001);
    const hazy = render(1, 0, {glow: 0, film: 0, layout: hazeLayout}); snapshot('haze');
    const level = r => region(hazy, ...r).mean;
    const atEye = 127.5 * (1 - Math.exp(-0.0001 * 2000 * Math.hypot(1, 0.25)));
    check(Math.abs(level([-0.27, -0.02, -0.23, 0.02]) - atEye) < 2, `at the eye's height the fog's color over 2,000 units: ${level([-0.27, -0.02, -0.23, 0.02])} vs ${atEye}`);
    check(level([-0.27, -0.62, -0.23, -0.58]) > level([-0.27, -0.02, -0.23, 0.02]) + 3 &&
      level([-0.27, 0.38, -0.23, 0.42]) < level([-0.27, -0.02, -0.23, 0.02]) - 3, 'denser below the eye, thinner above');
    check(same(hazy, clearAir, [0.55, -0.9, 1, 0.9]), 'the viewmodel keeps its own');
    check(same(hazy, clearAir, [-1, 0.65, 0.45, 1]), 'the sky keeps its own');
    hazeAt(0.0001, [0.45, 0.45, 0.45]);
    const sunward = render(1, 0, {glow: 0, film: 0, layout: hazeLayout});
    check(region(sunward, -0.27, -0.02, -0.23, 0.02).mean > level([-0.27, -0.02, -0.23, 0.02]) + 10, 'toward the sun it glows');
    hazeAt(0.0001, [0.45, 0.45, 0.45], [-1, 0, 0]);
    const away = render(1, 0, {glow: 0, film: 0, layout: hazeLayout});
    check(Math.abs(region(away, -0.27, -0.02, -0.23, 0.02).mean - level([-0.27, -0.02, -0.23, 0.02])) < 0.5,
      `away from the sun it does not: ${region(away, -0.27, -0.02, -0.23, 0.02).mean} vs ${level([-0.27, -0.02, -0.23, 0.02])}`);
    hazeAt(0.01);
    check(same(render(1, 0, {glow: 0, film: 0, layout: hazeLayout}), clearAir, [0.55, -0.9, 1, 0.9]),
      'the viewmodel keeps its own in the thickest haze');
    hazeAt(0, [0, 0, 0], [0, 0, 0]);

    // The viewmodel's own motion. A white viewmodel square over the black
    // world covers pixels 576-703 by 324-395 and moves right with a half
    // smear of 15 pixels; its tiles reach pixels 504-791.
    function viewmodelSquare() {
      rect(-1, -1, 1, 1, worldDepth(2000), [0, 0, 0], 0);
      rect(-0.1, -0.1, 0.1, 0.1, vmDepth(10), [1, 1, 1], 0);
    }
    const square = {rect: [-0.1, -0.1, 0.1, 0.1], z: 10, smear: [15, 0]};
    const still = render(4, 0, {glow: 0, film: 0, layout: viewmodelSquare});
    check(same(render(4, 0, {glow: 0, film: 0, layout: viewmodelSquare, vm: {...viewmodelMesh([square]), smear: 0}}),
      still, [-1, -1, 1, 1]), 'without viewmodel motion nothing smears');
    const moving = render(4, 0, {glow: 0, film: 0, layout: viewmodelSquare, vm: viewmodelMesh([square])}); snapshot('viewmodel');
    check(pixel(moving, 640, 360) === 255, 'the moving viewmodel stays solid inside its smear');
    check(pixel(moving, 700, 360) < 250 && pixel(moving, 700, 360) > 128, 'the world shows through its trailing edge');
    check(pixel(moving, 709, 360) > 20 && pixel(moving, 571, 360) > 20, 'it smears over the world beyond both edges');
    check(pixel(moving, 722, 360) === 0 && pixel(moving, 556, 360) === 0, 'the smear reaches no further than its motion');
    check(pixel(moving, 640, 400) === 0 && pixel(moving, 640, 319) === 0, 'sideways motion does not smear vertically');
    check(same(moving, still, [-1, 0.5, 1, 1]) && same(moving, still, [-1, -1, -0.2, 1]), 'the world away from it stays sharp');
    // A switch's fast motion smears 60 pixels either side, beyond the old
    // 43.2-pixel cap, unbroken to its end and no further.
    const fast = render(4, 0, {glow: 0, film: 0, layout: viewmodelSquare, vm: viewmodelMesh([{...square, smear: [60, 0]}])});
    check([710, 725, 740, 755].every(x => pixel(fast, x, 360) > 0) && [570, 555, 540, 525].every(x => pixel(fast, x, 360) > 0),
      'a fast viewmodel motion smears 60 pixels beyond both edges');
    check(pixel(fast, 766, 360) === 0 && pixel(fast, 513, 360) === 0, 'the fast smear reaches no further than its motion');
    // Where viewmodel parts overlap, the nearer part's smear is drawn. The far
    // part is drawn last; RG8 stores each half smear over the longest, 12.
    const smearAt = (x, y) => {
      const texel = new Uint8Array(4);
      gl.bindFramebuffer(gl.READ_FRAMEBUFFER, Module.cod2SceneFx.t.vm.fbo);
      gl.readPixels(x, y, 1, 1, gl.RGBA, gl.UNSIGNED_BYTE, texel);
      gl.bindFramebuffer(gl.READ_FRAMEBUFFER, null);
      return [texel[0], texel[1]];
    };
    render(4, 0, {glow: 0, film: 0, vm: viewmodelMesh([{rect: [-0.5, -0.2, 0, 0.2], z: 8, smear: [0, 10]},
      {rect: [-0.3, -0.2, 0.2, 0.2], z: 15, smear: [12, 0]}])});
    const near12 = (texel, r, g) => Math.abs(texel[0] - r) <= 1 && Math.abs(texel[1] - g) <= 1;
    check(near12(smearAt(544, 360), 128, 234) && near12(smearAt(384, 360), 128, 234), 'the nearer part\'s smear wins where parts overlap');
    check(near12(smearAt(704, 360), 255, 128) && near12(smearAt(960, 360), 128, 128), 'each part draws its own smear, the rest none');

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
    // Behind the class screens of a match the view also turns gray: each
    // pixel takes its Rec. 709 luma, wholly or halfway.
    const colour = render(4, 0, {glow: 0, film: 0});
    const gray = render(4, 0, {glow: 0, film: 0, gray: 1}); snapshot('gray');
    const halfGray = render(4, 0, {glow: 0, film: 0, gray: 0.5});
    const spread = (px, i) => Math.max(px[i], px[i + 1], px[i + 2]) - Math.min(px[i], px[i + 1], px[i + 2]);
    let grayest = 0, vivid = 0;
    for (let i = 0; i < W * H * 4; i += 4) {
      grayest = Math.max(grayest, spread(gray, i));
      if (spread(colour, i) > spread(colour, vivid)) vivid = i;
    }
    check(grayest <= 1 && spread(colour, vivid) > 20, 'a gray backdrop leaves no color');
    const luma = 0.2126 * colour[vivid] + 0.7152 * colour[vivid + 1] + 0.0722 * colour[vivid + 2];
    check(Math.abs(gray[vivid] - luma) <= 2, 'the gray keeps each pixel\'s luma');
    check(spread(halfGray, vivid) > 5 && spread(halfGray, vivid) < spread(colour, vivid) - 5,
      'half the gray keeps half the color');

    // A raised scope (cg_wwiiscope_mp.c's CgWwiiScopeLens) over a flat gray world: its glass at the centre, 144
    // pixels across its radius, the rifle's own faceted eyepiece a white 16-sided annulus round it out to 1.6
    // glass radii, kept off the outline the stencil marks, which opens with the window to its bell, 1.65 glass
    // radii. The drawn eyepiece rings the glass out to 1.15 of them.
    const glassR = 0.4, R = glassR * H / 2, SCOPE = 2048, BELL = 2080;
    const annulus = gl.createProgram();
    gl.attachShader(annulus, shader(gl.VERTEX_SHADER, '#version 300 es\nuniform vec3 u_r;uniform float u_aspect;\n' +
      'void main(){int k=gl_VertexID%6;int s=gl_VertexID/6+((k==2||k==4||k==5)?1:0);bool o=k==1||k==2||k==4;\n' +
      'float a=float(s)*0.3926991;gl_Position=vec4(vec2(cos(a)/u_aspect,sin(a))*(o?u_r.y:u_r.x),u_r.z,1.0);}'));
    gl.attachShader(annulus, shader(gl.FRAGMENT_SHADER, '#version 300 es\nprecision highp float;out vec4 o;void main(){o=vec4(1.0);}'));
    gl.linkProgram(annulus);
    check(gl.getProgramParameter(annulus, gl.LINK_STATUS), 'eyepiece fixture program linked');
    // A sight's window, drawn with the view's projection, is the same but for that projection. With lens the
    // rifle's eyepiece covers the glass too, as its own dark lens does; with coverage the draw before the
    // rifle's, foliage, left alpha to coverage on.
    let coverageAfterStencil = false;
    function scopeFrame({open = 1, sight = false, dark = 0, world = [0.5, 0.5, 0.5], samples = 1, coverage = false,
                         lens = false, mark = stencil} = {}) {
      bind(gl, Module, HEAPF32, W, H, samples);
      // The D3D9 device clears the stencil with every bit writable.
      gl.stencilMask(0xff);
      drawScene(() => rect(-1, -1, 1, 1, worldDepth(2000), world, 0));
      const outline = glassR + (1.65 - 1) * glassR * open;
      const scope = [0, 0, glassR * H / W, 0, 0, glassR, 1, open, open, dark, sight ? 0 : 1.7, sight ? 0 : 3.0, 0.8];
      HEAPF32.set(scope, SCOPE);
      HEAPF32.set([0, 0, outline * H / W, 0, 0, outline], BELL);
      if (coverage) gl.enable(gl.SAMPLE_ALPHA_TO_COVERAGE);
      mark(Module, HEAPF32, SCOPE * 4, BELL * 4);
      coverageAfterStencil = gl.isEnabled(gl.SAMPLE_ALPHA_TO_COVERAGE);
      gl.disable(gl.SAMPLE_ALPHA_TO_COVERAGE);
      mask(Module, 1);
      gl.useProgram(annulus);
      gl.uniform3f(gl.getUniformLocation(annulus, 'u_r'), lens ? 0 : 0.95 * glassR, 1.6 * glassR, vmDepth(2));
      gl.uniform1f(gl.getUniformLocation(annulus, 'u_aspect'), W / H);
      gl.drawArrays(gl.TRIANGLES, 0, 16 * 6);
      mask(Module, 0);
      const at = params(0, {glow: 0, film: 0});
      HEAPF32.set(scope, at / 4 + 42);
      compose(gl, Module, HEAPF32, at, MESH * 4);
      return read();
    }
    const polar = (px, r, degrees) => pixel(px, Math.round(W / 2 + r * Math.cos(degrees * Math.PI / 180)),
      Math.round(H / 2 + r * Math.sin(degrees * Math.PI / 180)));
    const around = (px, r, from = 0, to = 360) => { const v = []; for (let a = from; a < to; ++a) v.push(polar(px, r, a)); return v; };
    const mean = v => v.reduce((a, b) => a + b, 0) / v.length;
    const scoped = scopeFrame(); snapshot('scope');
    const world = pixel(scoped, 40, 40);
    check(Math.abs(world - 128) <= 2 && Math.abs(pixel(scoped, W / 2, H / 2) - world) <= 2, 'the world shows round and through the scope');
    check([1.3, 1.45, 1.55].every(k => around(scoped, k * R).every(v => Math.abs(v - world) <= 2)),
      'the rifle\'s faceted eyepiece is kept off its bell\'s outline, the world showing round the drawn one');
    // Over a black world whatever shows round the glass is the eyepiece.
    const night = scopeFrame({world: [0, 0, 0]}), edges = [];
    for (let a = 0; a < 360; a += 2) {
      let edge = 0;
      for (let r = R; r < 1.45 * R; r += 0.25) if (polar(night, r, a) > 0) edge = r;
      edges.push(edge);
    }
    const roundness = Math.max(...edges) - Math.min(...edges);
    check(roundness < 3 && Math.abs(mean(edges) - 1.15 * R) < 4, `the drawn eyepiece is round: its rim ${mean(edges).toFixed(1)} px out, within ${roundness.toFixed(2)} px all round`);
    const band = (from, to) => mean([1.03, 1.06, 1.09, 1.12].flatMap(k => around(scoped, k * R, from, to)));
    check(band(100, 170) > band(280, 350) + 8, `it is lit from the upper left (${band(100, 170).toFixed(0)} against ${band(280, 350).toFixed(0)})`);
    const ring = around(scoped, 1.08 * R);
    const grain = mean(ring.map((v, i) => Math.abs(v - ring[(i + 1) % ring.length])));
    check(grain > 1.5, `its metal is worn and scratched, not flat (${grain.toFixed(2)} levels a degree)`);
    check(mean(around(scoped, 0.98 * R)) < mean(around(scoped, 0.5 * R)) - 10, 'the glass darkens into its seat');
    const shut = scopeFrame({open: 0});
    check(around(shut, 1.3 * R).filter(v => v > 200).length > 0.9 * 360, 'until the window opens the rifle\'s own eyepiece shows round the ring');
    const held = scopeFrame({dark: 1});
    check(around(held, 1.08 * R).every(v => v <= 3) && Math.abs(pixel(held, W / 2, H / 2) - world) <= 2,
      'a held breath blacks out the eyepiece with everything round the glass');
    // The rifle kept off the same outline, to see that no ring is drawn.
    const bare = scopeFrame({sight: true});
    check(around(bare, 1.08 * R).every(v => Math.abs(v - world) <= 2), 'a sight\'s window draws no eyepiece round its glass');
    // In the multisampled scene, after a foliage draw left alpha to coverage on, the glass is still marked: the
    // world shows through it and the rifle's own lens stays off it. Marked with that coverage on, as the stencil
    // was before, the fan's zero alpha covers no sample and the lens covers the glass.
    const glassValues = px => [0, 0.3, 0.6].flatMap(k => around(px, k * R));
    const throughGlass = px => glassValues(px).every(v => Math.abs(v - world) <= 2);
    const foliage = scopeFrame({samples: 4, coverage: true, lens: true});
    check(throughGlass(foliage), 'after foliage left alpha to coverage on, the multisampled glass shows the world, ' +
      `not the rifle's lens (${Math.min(...glassValues(foliage))} to ${Math.max(...glassValues(foliage))} against ${world})`);
    check(coverageAfterStencil, 'the stencil leaves alpha to coverage as the effects\' state cache expects it');
    const coveredStencil = Function('Module', 'HEAPF32', 'scope', 'bell',
      stencilBody.replace(/,\s*gl\.SAMPLE_ALPHA_TO_COVERAGE\]/, ']'));
    const covered = scopeFrame({samples: 4, coverage: true, lens: true, mark: coveredStencil});
    check(pixel(covered, W / 2, H / 2) > 200 && !throughGlass(covered),
      'marked with alpha to coverage on, the rifle\'s lens covers the glass');
    check(throughGlass(scopeFrame({samples: 4, lens: true})), 'without it the multisampled glass shows the world');
    HEAPF32.fill(0, 16 + 42, 16 + 55);
    gl.stencilMask(0xff);

    // A scene of half the canvas's lines renders at its size from the canvas
    // viewport the engine sets, and is filtered up to the whole canvas with
    // Catmull-Rom. The square's right edge falls between scene columns 351 and
    // 352: canvas pixels 703 and 704 read 203 and 52, where bilinear filtering
    // reads 191 and 64, and the overshoot either side clamps.
    check(bind(gl, Module, HEAPF32, W / 2, H / 2, 1, W, H) === 1, 'a scene below the canvas binds at its own size');
    drawScene(squares);
    check(String(gl.getParameter(gl.VIEWPORT)) === `0,0,${W / 2},${H / 2}`, 'the canvas viewport lands on the scene at its scale');
    compose(gl, Module, HEAPF32, params(0, {glow: 0, film: 0}));
    check(String(gl.getParameter(gl.VIEWPORT)) === `0,0,${W},${H}` && !Object.prototype.hasOwnProperty.call(gl, 'viewport'),
      'composing hands the canvas its own viewport back');
    const upscaled = read(); snapshot('upscaled');
    check(pixel(upscaled, 640, 360) === 255 && pixel(upscaled, 640, 300) === 0 && pixel(upscaled, 1100, 100) === 255 &&
      pixel(upscaled, 100, 100) === 0, 'the upscaled scene fills the canvas');
    const reads = (x, value) => Math.abs(pixel(upscaled, x, 360) - value) <= 2;
    check(reads(703, 203) && reads(704, 52) && reads(575, 52) && reads(576, 203), 'Catmull-Rom keeps edges sharper than bilinear');
    check(pixel(upscaled, 702, 360) === 255 && pixel(upscaled, 705, 360) === 0 && pixel(upscaled, 574, 360) === 0 &&
      pixel(upscaled, 577, 360) === 255, 'an edge spans two canvas pixels and its overshoot clamps');
    gl.viewport(0, 0, W, H);

    // Each pass has a fixed GPU cost, so the pass count is part of the frame budget.
    const passes = (ads, options, divisor = 1) => {
      bind(gl, Module, HEAPF32, W / divisor, H / divisor, 4, W, H); drawScene();
      const drawArrays = gl.drawArrays, blit = gl.blitFramebuffer;
      let draws = 0, blits = 0;
      gl.drawArrays = function(...args) { ++draws; return drawArrays.apply(this, args); };
      gl.blitFramebuffer = function(...args) { ++blits; return blit.apply(this, args); };
      compose(gl, Module, HEAPF32, params(ads, options), MESH * 4);
      gl.drawArrays = drawArrays; gl.blitFramebuffer = blit;
      return `${draws}+${blits}`;
    };
    check(passes(0, {glow: 0}) === '1+1', 'grade and sharpening alone: one resolve and one full-screen pass');
    check(passes(0) === '3+1', 'glow adds two 1/8-resolution passes');
    check(passes(1) === '5+1', 'aiming adds two 1/4-resolution passes');
    check(passes(0, {glow: 0, motion: turn(0.03)}) === '1+1', 'motion blur adds no pass');
    check(passes(0, {glow: 0, vm: viewmodelMesh([square])}) === '2+1', 'viewmodel motion adds its own full-resolution draw');
    check(passes(0, {glow: 0, backdrop: 1}) === '5+1', 'the backdrop adds four 1/4-resolution passes');
    check(passes(0, {glow: 0, gray: 1}) === '1+1', 'its gray adds no pass');
    check(passes(0, {glow: 0}, 2) === '2+1', 'a scene below the canvas adds one full-canvas pass');

    // A steady scaled frame queries nothing the GPU process has to answer.
    bind(gl, Module, HEAPF32, W / 2, H / 2, 4, W, H); drawScene(); compose(gl, Module, HEAPF32, params(0));
    {
      const queried = [], getParameter = gl.getParameter, checkStatus = gl.checkFramebufferStatus;
      gl.getParameter = function(pname) { queried.push(pname); return getParameter.call(this, pname); };
      gl.checkFramebufferStatus = function(target) { queried.push('checkFramebufferStatus'); return checkStatus.call(this, target); };
      bind(gl, Module, HEAPF32, W / 2, H / 2, 4, W, H); drawScene(); compose(gl, Module, HEAPF32, params(0));
      gl.getParameter = getParameter; gl.checkFramebufferStatus = checkStatus;
      const answered = new Set([gl.CURRENT_PROGRAM, gl.VERTEX_ARRAY_BINDING, gl.ACTIVE_TEXTURE, gl.VIEWPORT]);
      check(queried.length && queried.every(pname => answered.has(pname)), 'a scaled frame makes no synchronous GL queries');
    }
    gl.viewport(0, 0, W, H);

    // Once its target exists, a frame with viewmodel motion queries nothing
    // the GPU process has to answer, and leaves the caller's vertex array and
    // buffers bound: the velocity pass reads its vertices from a texture.
    {
      const mesh = viewmodelMesh([square]);
      render(4, 0, {glow: 0, film: 0, layout: viewmodelSquare, vm: mesh});
      const callerVao = gl.createVertexArray(), arrayBuffer = gl.createBuffer(), indexBuffer = gl.createBuffer();
      check(bind(gl, Module, HEAPF32, W, H, 4) === 1, 'a frame with viewmodel motion binds');
      drawScene(viewmodelSquare);
      gl.bindVertexArray(callerVao);
      gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, indexBuffer);
      gl.bindBuffer(gl.ARRAY_BUFFER, arrayBuffer);
      const queried = [], getParameter = gl.getParameter, checkStatus = gl.checkFramebufferStatus;
      gl.getParameter = function(pname) { queried.push(pname); return getParameter.call(this, pname); };
      gl.checkFramebufferStatus = function(target) { queried.push('checkFramebufferStatus'); return checkStatus.call(this, target); };
      compose(gl, Module, HEAPF32, params(0, {glow: 0, film: 0, vm: mesh}), MESH * 4);
      gl.getParameter = getParameter; gl.checkFramebufferStatus = checkStatus;
      const answered = new Set([gl.CURRENT_PROGRAM, gl.VERTEX_ARRAY_BINDING, gl.ACTIVE_TEXTURE, gl.VIEWPORT]);
      check(queried.length && queried.every(pname => answered.has(pname)), 'a frame with viewmodel motion makes no synchronous GL queries');
      check(gl.getParameter(gl.VERTEX_ARRAY_BINDING) === callerVao && gl.getParameter(gl.ARRAY_BUFFER_BINDING) === arrayBuffer &&
        gl.getParameter(gl.ELEMENT_ARRAY_BUFFER_BINDING) === indexBuffer, 'the viewmodel pass leaves the caller\'s vertex array and buffers bound');
      check(same(read(), moving, [-1, -1, 1, 1]), 'the same viewmodel motion composes the same frame');
      const released = [8, 9, 10, 11, 12, 13, 14].every(unit => {
        gl.activeTexture(gl.TEXTURE0 + unit);
        return gl.getParameter(gl.TEXTURE_BINDING_2D) === null;
      });
      check(released, 'a frame with viewmodel motion releases every post texture unit');
      gl.activeTexture(gl.TEXTURE0);
      gl.bindVertexArray(null);
      gl.deleteVertexArray(callerVao); gl.deleteBuffer(arrayBuffer); gl.deleteBuffer(indexBuffer);
    }

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
    for (let unit = 8; unit < 15; ++unit) {
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
  }, {bindBody: emjs('WebSceneFx_Bind'), composeBody: emjs('WebSceneFx_Compose'),
      stencilBody: emjs('WebSceneFx_ScopeStencil'), maskBody: emjs('WebSceneFx_ScopeMask'), cache});
  for (const [name, url] of Object.entries(result.images))
    fs.writeFileSync(path.join(outputDir, name + '.png'), Buffer.from(url.split(',')[1], 'base64'));
  console.log(result.checks.map(message => 'ok - ' + message).join('\n'));
  console.log(`PASS: ${result.checks.length} real WebGL2 scene compositor checks; images in ${outputDir}`);
} finally {
  await browser.close();
}
