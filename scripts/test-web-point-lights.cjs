// Exercise the actual scene/view handoff and linked shader/uniform generation.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const bridge = fs.readFileSync('downstream/wasm/web_point_lights.c', 'utf8');
const linked = fs.readFileSync(process.argv[2] || 'out/cod2-wasm-core/site/cod2.js', 'utf8');
const Module = {}, HEAPF32 = new Float32Array(256);
function emjs(name, args) {
  const start = bridge.indexOf('EM_JS(void, ' + name + ',');
  let pos = bridge.indexOf('{', start), end = pos + 1, depth = 1;
  while (depth) { depth += (bridge[end] === '{') - (bridge[end] === '}'); ++end; }
  return Function('Module', 'HEAPF32', ...args, bridge.slice(pos + 1, end - 1)).bind(null, Module, HEAPF32);
}
const set = emjs('WebPointLights_Set', ['packed', 'count']);
const draw = emjs('WebPointLights_Draw', ['view', 'enabled']);
draw(0, 0);
HEAPF32.set([100, 20, 60, 200, 1, .7, .3, 0, -10, 30, 90, 100, .2, .3, 1, 0], 8);
set(32, 2);
const lights = Module.cod2PointLights;
const buffers = [lights.world, lights.eye, lights.color, lights.view];
for (let angle = -180; angle <= 180; angle += 3) {
  const a = angle * Math.PI / 180, c = Math.cos(a), s = Math.sin(a);
  HEAPF32.set([c, s, 0, 0, -s, c, 0, 0, 0, 0, 1, 0, -40, 11, -55, 1], 64);
  draw(256, 1);
  assert.equal(lights.drawCount, 2);
  for (let i = 0; i < 2; ++i) {
    const p = i * 8, q = i * 4, x = lights.world[p], y = lights.world[p + 1];
    assert(Math.abs(lights.eye[q] - (HEAPF32[64] * x - HEAPF32[65] * y - 40)) < .00002);
    assert(Math.abs(lights.eye[q + 1] - (HEAPF32[65] * x + HEAPF32[64] * y + 11)) < .00002);
    assert.equal(lights.eye[q + 2], lights.world[p + 2] - 55);
    assert(Math.abs(lights.eye[q + 3] * lights.world[p + 3] ** 2 - 1) < .000001);
    for (let j = 0; j < 3; ++j) assert.equal(lights.color[q + j], lights.world[p + 4 + j]);
  }
  const revision = lights.eyeRevision;
  for (let i = 0; i < 100; ++i) draw(256, 1);
  assert.equal(lights.eyeRevision, revision, 'draws sharing a view reuse transformed lights');
  draw(256, 0); assert.equal(lights.drawCount, 0, 'HUD/particles receive no world illumination');
  draw(256, 1); assert.equal(lights.drawCount, 2);
  draw(0, 1); assert.equal(lights.drawCount, 0);
}
const defsStart = linked.indexOf('          // COD2_POINT_LIGHT_SHADER:');
const defsEnd = linked.indexOf('          var vsSource =', defsStart);
assert(defsStart >= 0 && defsEnd > defsStart);
const generate = Function('GLImmediate', 'GLEmulation', linked.slice(defsStart, defsEnd) + '\nreturn {pointVarying,pointDefs,pointPass};');
for (const model of [false, true]) for (const type of [0, 0x0DE1, 0x8513]) for (const matrix of [false, true]) for (const units of [[], [1], [0], [0, 1]]) {
  const shader = generate.call({usedTexUnitList: units}, {useTextureMatrix: matrix, TexEnvJIT: {getTexUnitType: () => type}}, {lightingEnabled: model});
  const enabled = type === 0x0DE1 && units.includes(0);
  assert.equal(Boolean(shader.pointPass), enabled);
  if (enabled) {
    assert(shader.pointVarying.includes('highp vec3'));
    assert.equal(shader.pointPass.includes('dFdx(v_cod2Eye)'), !model);
    assert.equal(shader.pointPass.includes('dFdy(v_cod2Eye)'), !model);
    assert.equal(shader.pointPass.includes('highp vec3 n = v_cod2Normal;'), model);
    assert(shader.pointDefs.includes('[4]'));
    for (let i = 0; i < 4; ++i) assert(shader.pointPass.includes('u_cod2LightCount > ' + i));
    assert(shader.pointPass.includes(matrix ? '(u_textureMatrix0 * v_texCoord0).xy' : 'v_texCoord0.xy'));
    assert(!shader.pointPass.includes('gl_FragColor.a'), 'flash illumination preserves transparency');
  }
}
assert(linked.includes('fsTexEnvPass, pointPass, fogPass, fsAlphaTestPass'), 'light fades into the original fog');
const glslStart = linked.indexOf('          function cod2Glsl300(');
const glslEnd = linked.indexOf('          this.vertexShader =', glslStart);
assert(glslStart >= 0 && glslEnd > glslStart);
const glsl = Function(linked.slice(glslStart, glslEnd) + '\nreturn cod2Glsl300;')();
const vertex = glsl('attribute vec4 a_position; varying vec4 v_color; varying highp vec3 v_cod2Eye;', true);
const fragment = glsl('precision mediump float; varying vec4 v_color; varying highp vec3 v_cod2Eye; void main(){gl_FragColor=texture2D(t,uv)+textureCube(c,dir);}', false);
assert(vertex.startsWith('#version 300 es\n'));
assert(vertex.includes('out mediump vec4 v_color;') && vertex.includes('out highp vec3 v_cod2Eye;'));
assert(fragment.startsWith('#version 300 es\nout highp vec4 cod2FragColor;'));
assert(fragment.includes('in mediump vec4 v_color;') && fragment.includes('in highp vec3 v_cod2Eye;'));
assert(fragment.includes('cod2FragColor=texture(t,uv)+texture(c,dir)'));
assert(!fragment.includes('GL_OES_standard_derivatives'));
const uploadStart = linked.indexOf('        // COD2_POINT_LIGHT_UNIFORMS:');
const uploadEnd = linked.indexOf('        if (GLImmediate.mode == GLctx.POINTS)', uploadStart);
assert(uploadStart >= 0 && uploadEnd > uploadStart);
let countUploads = 0, arrayUploads = 0;
const GLctx = {uniform1i(location, count) { assert.equal(location, 1); assert(count >= 0 && count <= 4); ++countUploads; },
  uniform4fv(location, buffer) { assert([2, 3].includes(location)); assert.equal(buffer.length, 16); ++arrayUploads; }};
const upload = Function('Module', 'GLctx', linked.slice(uploadStart, uploadEnd));
const renderer = {cod2LightCountLocation: 1, cod2LightEyeLocation: 2, cod2LightColorLocation: 3,
  cod2LightCount: -1, cod2LightRevision: -1};
draw(256, 1); upload.call(renderer, Module, GLctx);
assert.equal(countUploads, 1); assert.equal(arrayUploads, 2);
for (let i = 0; i < 1000; ++i) upload.call(renderer, Module, GLctx);
assert.equal(countUploads, 1); assert.equal(arrayUploads, 2);
draw(256, 0); upload.call(renderer, Module, GLctx); assert.equal(countUploads, 2);
draw(256, 1); upload.call(renderer, Module, GLctx); assert.equal(countUploads, 3); assert.equal(arrayUploads, 2);
HEAPF32[12] = .8; set(32, 2); draw(256, 1); upload.call(renderer, Module, GLctx); assert.equal(arrayUploads, 4);
set(32, 0); draw(256, 1); upload.call(renderer, Module, GLctx); assert.equal(renderer.cod2LightCount, 0);
assert.deepEqual(buffers, [lights.world, lights.eye, lights.color, lights.view]);
for (const key of ['world', 'eye', 'color', 'view']) assert.equal(lights[key], buffers[['world','eye','color','view'].indexOf(key)]);
console.log('PASS: 121 camera transforms, cached uniform uploads, light expiry, HUD isolation and all 48 shader texture/normal variants');
