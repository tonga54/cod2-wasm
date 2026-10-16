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
const generate = Function('GLImmediate', 'GLEmulation', 'fsTexEnvPass', linked.slice(defsStart, defsEnd) + '\nreturn {pointVarying,pointDefs,pointPass,fsTexEnvPass};');
for (const model of [false, true]) for (const type of [0, 0x0DE1, 0x8513]) for (const matrix of [false, true]) for (const units of [[], [1], [0], [0, 1]]) {
  const uv = matrix ? '(u_textureMatrix0 * v_texCoord0).xy' : 'v_texCoord0.xy';
  const material = `gl_FragColor = texture2D(u_texUnit0, ${uv}) * v_color;`;
  const shader = generate.call({usedTexUnitList: units}, {useTextureMatrix: matrix, TexEnvJIT: {getTexUnitType: () => type}}, {lightingEnabled: model}, material);
  const enabled = type === 0x0DE1 && units.includes(0);
  assert.equal(Boolean(shader.pointPass), enabled);
  if (enabled) {
    assert(shader.pointVarying.includes('highp vec3'));
    assert.equal(shader.pointPass.includes('dFdx(v_cod2Eye)'), !model);
    assert.equal(shader.pointPass.includes('dFdy(v_cod2Eye)'), !model);
    assert.equal(shader.pointPass.includes('highp vec3 n = v_cod2Normal;'), model);
    assert(shader.pointDefs.includes('[4]'));
    for (let i = 0; i < 4; ++i) assert(shader.pointPass.includes('u_cod2LightCount > ' + i));
    assert(shader.fsTexEnvPass.includes(uv));
    assert(shader.pointPass.includes('cod2BaseTexel.rgb * lightSum'));
    assert.equal((shader.fsTexEnvPass + shader.pointPass).match(/texture2D\(/g).length, 1,
      'material and flashes share one texture lookup');
    assert(shader.pointPass.includes(model ? 'max(dot(n,delta), 0.0)' : 'abs(dot(n,delta))'));
    assert(!shader.pointPass.includes('gl_FragColor.a'), 'flash illumination preserves transparency');
  }
}
// Materials without a base sample still receive light without referencing an
// undefined texel, or adding an unconditional sample to unlit frames.
const noSample = generate.call({usedTexUnitList:[0]}, {useTextureMatrix:false,TexEnvJIT:{getTexUnitType:()=>0x0DE1}}, {lightingEnabled:false}, 'gl_FragColor = v_color;');
assert.equal(noSample.fsTexEnvPass, 'gl_FragColor = v_color;');
assert(noSample.pointPass.includes('texture2D(u_texUnit0, v_texCoord0.xy).rgb'));
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
const setShadows = emjs('WebPointLights_SetShadows', ['packed', 'count']);
const drawEnhancements = emjs('WebPointLights_DrawEnhancements', ['view', 'worldShadowEnabled', 'weaponBlur']);
HEAPF32.set([50, 20, 0, 22, -30, 40, 10, 30], 16);
setShadows(64, 2);
HEAPF32.set([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -10, 5, -20, 1], 64);
drawEnhancements(256, 1, 0.75);
assert.equal(lights.shadowDrawCount, 2);
assert.equal(lights.weaponBlur, 0.75);
assert.equal(lights.shadowEye[0], 40);
assert.equal(lights.shadowEye[1], 25);
assert.equal(lights.shadowEye[2], -20);
assert(Math.abs(lights.shadowEye[3] * 22 * 22 - 1) < 1e-6);
const shadowRev = lights.shadowEyeRevision;
for (let i = 0; i < 100; ++i) drawEnhancements(256, 1, 1.5);
assert.equal(lights.shadowEyeRevision, shadowRev, 'repeated world draws reuse transformed shadow casters');
assert.equal(lights.weaponBlur, 1, 'ADS blur clamps above 1');
drawEnhancements(256, 0, -0.5);
assert.equal(lights.shadowDrawCount, 0);
assert.equal(lights.weaponBlur, 0, 'ADS blur clamps below 0');
const modelShader = generate.call({usedTexUnitList:[0]}, {useTextureMatrix:false,TexEnvJIT:{getTexUnitType:()=>0x0DE1}}, {lightingEnabled:true}, 'gl_FragColor = texture2D(u_texUnit0, v_texCoord0.xy) * v_color;');
assert(modelShader.pointDefs.includes('uniform mediump float u_cod2WeaponBlur;'));
assert(modelShader.fsTexEnvPass.includes('if (u_cod2WeaponBlur > 0.0)'));
assert(modelShader.fsTexEnvPass.includes('cod2Near = clamp((13.0 - cod2Depth) * 0.11, 0.0, 1.0)'));
assert(modelShader.fsTexEnvPass.includes('texture(u_texUnit0, cod2Uv + cod2Dx + cod2Dy * 0.5, cod2Bias)'));
const worldShader = generate.call({usedTexUnitList:[0]}, {useTextureMatrix:false,TexEnvJIT:{getTexUnitType:()=>0x0DE1}}, {lightingEnabled:false}, 'gl_FragColor = texture2D(u_texUnit0, v_texCoord0.xy) * v_color;');
assert(worldShader.pointDefs.includes('uniform int u_cod2ShadowCount; uniform highp vec4 u_cod2ShadowEye[4];'));
assert(worldShader.pointPass.includes('if (u_cod2ShadowCount > 0)'));
assert(worldShader.pointPass.includes('gl_FragColor.rgb *= cod2Shadow;'));
let blurUploads = 0, shadowCountUploads = 0, shadowArrayUploads = 0;
const enhGLctx = {
  uniform1i(loc, val) { if (loc === 5) ++shadowCountUploads; else ++countUploads; },
  uniform1f(loc, val) { assert.equal(loc, 4); assert(val >= 0 && val <= 1); ++blurUploads; },
  uniform4fv(loc, buf) { if (loc === 6) { assert.equal(buf.length, 16); ++shadowArrayUploads; } else ++arrayUploads; }
};
const enhRenderer = {
  cod2LightCountLocation: 1, cod2LightEyeLocation: 2, cod2LightColorLocation: 3,
  cod2WeaponBlurLocation: 4, cod2ShadowCountLocation: 5, cod2ShadowEyeLocation: 6,
  cod2LightCount: -1, cod2LightRevision: -1, cod2WeaponBlur: -1, cod2ShadowCount: -1, cod2ShadowRevision: -1
};
drawEnhancements(256, 1, 0.85);
upload.call(enhRenderer, Module, enhGLctx);
assert.equal(blurUploads, 1); assert.equal(shadowCountUploads, 1); assert.equal(shadowArrayUploads, 1);
for (let i = 0; i < 200; ++i) upload.call(enhRenderer, Module, enhGLctx);
assert.equal(blurUploads, 1); assert.equal(shadowCountUploads, 1); assert.equal(shadowArrayUploads, 1);
console.log('PASS: 121 camera transforms, cached uniform uploads, ADS weapon DOF blur, ground shadows, HUD isolation and all 48 shader texture/normal variants');
