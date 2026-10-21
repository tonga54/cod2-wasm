// Compile the linked effect passes (web_effects.c) on a real GPU and read their pixels:
// the DX9 zfeather against a depth texture, the heat of fire and alpha-to-coverage's alpha.
import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
const [linkedJs, playwrightModule, outputDir = 'out/web-effects-qa'] = process.argv.slice(2);
if (!linkedJs || !playwrightModule)
  throw Error('Usage: node scripts/test-web-effects.mjs LINKED_JS PLAYWRIGHT_MODULE [OUTPUT_DIR]');
const {chromium} = await import(pathToFileURL(path.resolve(playwrightModule)));
fs.mkdirSync(outputDir, {recursive: true});
const source = fs.readFileSync(linkedJs, 'utf8');
const slice = (from, to) => {
  const start = source.indexOf(from), end = source.indexOf(to, start);
  if (start < 0 || end <= start) throw Error('Missing linked code: ' + from);
  return source.slice(start, end);
};
const generate = Function('GLImmediate', 'GLEmulation', 'fsTexEnvPass', 'texUnitVaryingList=""',
  slice('          // COD2_POINT_LIGHT_SHADER:', '          var vsSource =') +
  ';return {pointDefs,pointPass,fsTexEnvPass,cod2EffectPass,pointLights,pointUV};');
const coverage = Function('pointLights', 'GLEmulation', 'fsAlphaTestPass', 'pointUV',
  slice('          // COD2_ALPHA_COVERAGE:', '          var fsSource = [ "precision mediump float;",') + '\nreturn fsAlphaTestPass;');
const s = generate.call({usedTexUnitList: [0]}, {useTextureMatrix: false, TexEnvJIT: {getTexUnitType: () => 0x0DE1}},
  {lightingEnabled: false}, 'gl_FragColor = texture2D(u_texUnit0, v_texCoord0.xy) * v_color;');
if (!s.cod2EffectPass) throw Error('The unlit pass carries no effects');
const alphaPass = coverage(s.pointLights, {alphaTestFunc: 518}, 'if (!(gl_FragColor.a >= u_alphaTestRef)) { discard; }', s.pointUV);
const fragment = '#version 300 es\nprecision mediump float;\nout highp vec4 result;\nuniform sampler2D u_texUnit0;\n' +
  'uniform float u_alphaTestRef;\nin highp vec4 v_texCoord0;\nin mediump vec4 v_color;\n' +
  (s.pointDefs + '\nvoid main(){\n' + s.fsTexEnvPass + '\n' + s.pointPass + '\n' + s.cod2EffectPass + '\n' + alphaPass + '\n}\n')
    .replace(/\bvarying\b/g, 'in').replace(/texture2D/g, 'texture').replace(/gl_FragColor/g, 'result');

const browser = await chromium.launch({headless: false, executablePath: process.env.CHROME_EXECUTABLE || undefined});
try {
  const page = await browser.newPage({viewport: {width: 600, height: 400}});
  await page.setContent('<body style="background:#222;color:white;font:16px sans-serif"><canvas width="64" height="64"></canvas><pre></pre>');
  const result = await page.evaluate(fragment => {
    const canvas = document.querySelector('canvas'), checks = [];
    const gl = canvas.getContext('webgl2', {antialias: false, alpha: true, premultipliedAlpha: false, preserveDrawingBuffer: true});
    const check = (value, message) => { if (!value) throw Error(message); checks.push(message); };
    check(!!gl, 'WebGL2 available');
    const compile = (type, text) => {
      const shader = gl.createShader(type); gl.shaderSource(shader, text); gl.compileShader(shader);
      check(gl.getShaderParameter(shader, gl.COMPILE_STATUS), gl.getShaderInfoLog(shader) || 'shader compiled'); return shader;
    };
    // A plane facing the eye at a uniform distance; UVs over the screen, scaled.
    const program = gl.createProgram();
    gl.attachShader(program, compile(gl.VERTEX_SHADER, '#version 300 es\nprecision highp float;uniform float eyeZ;uniform float uvScale;' +
      'uniform vec4 color;out highp vec4 v_texCoord0;out mediump vec4 v_color;out highp vec3 v_cod2Eye;' +
      'void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.-1.,0,1);' +
      'v_texCoord0=vec4(p*uvScale,0,1);v_color=color;v_cod2Eye=vec3((p-.5)*eyeZ,-eyeZ);}'));
    gl.attachShader(program, compile(gl.FRAGMENT_SHADER, fragment));
    gl.linkProgram(program);
    check(gl.getProgramParameter(program, gl.LINK_STATUS), gl.getProgramInfoLog(program) || 'program linked');
    gl.useProgram(program);
    const u = name => gl.getUniformLocation(program, name);
    gl.uniform1i(u('u_texUnit0'), 0); gl.uniform1i(u('u_cod2FeatherDepth'), 19);
    gl.uniform1i(u('u_cod2DetailMap'), 5); gl.uniform1i(u('u_cod2NormalMap'), 6); gl.uniform1i(u('u_cod2SpecMap'), 7);
    const texture = (size, texel) => {
      const t = gl.createTexture(), pixels = new Uint8Array(size * size * 4);
      for (let y = 0; y < size; ++y) for (let x = 0; x < size; ++x) pixels.set(texel(x, y, size), (y * size + x) * 4);
      gl.activeTexture(gl.TEXTURE0); gl.bindTexture(gl.TEXTURE_2D, t);
      gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, size, size, 0, gl.RGBA, gl.UNSIGNED_BYTE, pixels); gl.generateMipmap(gl.TEXTURE_2D);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR_MIPMAP_LINEAR);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    };
    // The scene's depth as the infinite projection stores it, on unit 19: a
    // surface 2 units behind the plane on the left, the far distance on the right.
    const k = 0.9995002746582031, worldNear = k * 4;
    const depthAt = z => k - worldNear / z;
    const depth = (left, right) => {
      const packed = new Uint32Array(64 * 64);
      for (let y = 0; y < 64; ++y) for (let x = 0; x < 64; ++x)
        packed[y * 64 + x] = (Math.round(Math.min(depthAt(x < 32 ? left : right), 1) * 0xffffff) << 8) >>> 0;
      gl.activeTexture(gl.TEXTURE19); gl.bindTexture(gl.TEXTURE_2D, gl.createTexture());
      gl.texImage2D(gl.TEXTURE_2D, 0, gl.DEPTH24_STENCIL8, 64, 64, 0, gl.DEPTH_STENCIL, gl.UNSIGNED_INT_24_8, packed);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST); gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
      gl.activeTexture(gl.TEXTURE0);
    };
    function draw({effect = [0, 0, 0, 0], eyeZ = 100, color = [1, 1, 1, 1], uvScale = 1, ref = 0} = {}) {
      gl.uniform4fv(u('u_cod2Effect'), effect); gl.uniform1f(u('eyeZ'), eyeZ); gl.uniform4fv(u('color'), color);
      gl.uniform1f(u('uvScale'), uvScale); gl.uniform1f(u('u_alphaTestRef'), ref);
      gl.clearColor(0, 0, 0, 0); gl.clear(gl.COLOR_BUFFER_BIT);
      gl.drawArrays(gl.TRIANGLES, 0, 3);
      const px = new Uint8Array(64 * 64 * 4); gl.readPixels(0, 0, 64, 64, gl.RGBA, gl.UNSIGNED_BYTE, px); return px;
    }
    const at = (px, x, y) => [...px.subarray((y * 64 + x) * 4, (y * 64 + x) * 4 + 4)];
    const near = (a, b, tolerance) => Math.abs(a - b) <= tolerance;
    const same = (a, b) => a.every((v, i) => v === b[i]);
    // A flat orange flame texel, opaque.
    texture(16, () => [200, 120, 60, 255]);
    depth(102, 10000);
    const plain = draw(), off = draw({effect: [0, 0, 0, worldNear]});
    check(same(plain, off), 'no feather, no heat: the original pixels');
    check(same(at(plain, 10, 10), [200, 120, 60, 255]), 'fixture keeps its texel');
    // featherParms.x 0.25: 2 units from the surface keeps half; the far side all.
    const alpha = draw({effect: [0, 0.25, 0, worldNear]});
    check(near(at(alpha, 10, 32)[3], 128, 2) && same(at(alpha, 10, 32).slice(0, 3), [200, 120, 60]),
      `a surface 2 units behind fades the alpha to half: ${at(alpha, 10, 32)}`);
    check(at(alpha, 54, 32)[3] === 255, 'far surfaces keep the alpha');
    // A negative scale, a blend without alpha, fades the color too.
    const color = draw({effect: [0, -0.25, 0, worldNear]});
    check(at(color, 10, 32).every((v, i) => near(v, [100, 60, 30, 128][i], 2)), `the color fades with it: ${at(color, 10, 32)}`);
    check(same(at(color, 54, 32), [200, 120, 60, 255]), 'and keeps it far from a surface');
    // The same gap in front of the surface as behind it; within 1 / scale
    // units of the near clip it fades as well.
    depth(98, 10000);
    const front = at(draw({effect: [0, 0.25, 0, worldNear]}), 10, 32);
    check(near(front[3], 128, 2), `the gap counts either way: ${front}`);
    depth(10000, 10000);
    check(near(at(draw({effect: [0, 0.25, 0, worldNear], eyeZ: 6}), 32, 32)[3], 128, 3), '2 units past the near clip keeps half');
    check(at(draw({effect: [0, 0.25, 0, worldNear], eyeZ: 4}), 32, 32)[3] <= 2, 'at the near clip nothing');
    check(at(draw({effect: [0, 0.025, 0, worldNear], eyeZ: 24}), 32, 32)[3] < 140 &&
      at(draw({effect: [0, 0.025, 0, worldNear], eyeZ: 60}), 32, 32)[3] === 255, 'smoke fades over 40 units before the eye');
    // Heat: the color gains the gain times its luminance.
    const lum = (0.299 * 200 + 0.587 * 120 + 0.114 * 60) / 255, heated = at(draw({effect: [0, 0, 0.65, 0]}), 32, 32);
    check([200, 120, 60].every((v, i) => near(heated[i], Math.min(255, v * (1 + 0.65 * lum)), 2)) && heated[3] === 255,
      `fire burns hotter by its brightness: ${heated}`);
    // Coverage: alpha across the screen from 0 to 1 at a texel per pixel
    // turns from clear to solid over about a pixel about the reference; the
    // alpha test alone keeps every texel at or past it.
    texture(64, x => [255, 255, 255, Math.round(x / 63 * 255)]);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST); gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
    const tested = draw({ref: 0.5}), covered = draw({effect: [1, 0, 0, 0], ref: 0.5});
    const row = px => Array.from({length: 64}, (_, x) => at(px, x, 20)[3]);
    check(row(tested).every((a, x) => (x < 32 ? a === 0 : a >= 127)), `the plain test keeps the texels at or past 0.5: ${row(tested)}`);
    const ramp = row(covered).map((a, x) => [x, a]).filter(([, a]) => a > 0 && a < 255);
    check(row(covered).slice(0, 30).every(a => a === 0) && row(covered).slice(33).every(a => a === 255) && ramp.length <= 1,
      `coverage ramps across a pixel about the reference: ${row(covered)}`);
    // Far away (eight texels a pixel, three mips down) the alpha grows by
    // three quarters, so leaves and grass keep their coverage: an alpha of
    // 0.4 passes there, but not up close, nor under the plain test.
    texture(64, () => [255, 255, 255, 102]);
    const solid = px => at(px, 32, 32)[3];
    check(solid(draw({effect: [1, 0, 0, 0], uvScale: 8, ref: 0.5})) === 255, 'three mips down 0.4 covers the pixel');
    check(solid(draw({effect: [1, 0, 0, 0], uvScale: 1, ref: 0.5})) === 0, 'up close 0.4 stays clear');
    check(solid(draw({uvScale: 8, ref: 0.5})) === 0, 'the plain test drops it at any distance');
    check(solid(draw({effect: [1, 0, 0, 0], uvScale: 1.5, ref: 0.5})) === 0, 'half a mip down is not enough');
    check(gl.getError() === gl.NO_ERROR, 'no WebGL errors');
    const debug = gl.getExtension('WEBGL_debug_renderer_info');
    const result = {passed: true, checks, renderer: debug ? gl.getParameter(debug.UNMASKED_RENDERER_WEBGL) : gl.getParameter(gl.RENDERER)};
    document.querySelector('pre').textContent = JSON.stringify(result, null, 2);
    return result;
  }, fragment);
  fs.writeFileSync(path.join(outputDir, 'gpu.json'), JSON.stringify(result, null, 2));
  console.log(`PASS: ${result.checks.length} real WebGL2 effect checks on ${result.renderer}`);
} finally {
  await browser.close();
}
