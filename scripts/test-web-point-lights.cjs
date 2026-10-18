// Exercise the actual scene/view handoff and linked shader/uniform generation.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const bridge = fs.readFileSync('downstream/wasm/web_point_lights.c', 'utf8');
const linked = fs.readFileSync(process.argv[2] || 'out/cod2-wasm-core/site/cod2.js', 'utf8');
const Module = {}, HEAPF32 = new Float32Array(256);
// The draw handoff binds material maps on the engine's own context.
const glCalls = [], sampler = {};
const drawGL = {TEXTURE0: 0x84C0, TEXTURE5: 0x84C5, TEXTURE_2D: 0x0DE1, TEXTURE_MIN_FILTER: 0x2801,
  TEXTURE_MAG_FILTER: 0x2800, TEXTURE_WRAP_S: 0x2802, TEXTURE_WRAP_T: 0x2803, LINEAR_MIPMAP_LINEAR: 0x2703,
  LINEAR: 0x2601, REPEAT: 0x2901,
  createSampler: () => (glCalls.push(['createSampler']), sampler),
  samplerParameteri: (s, name, value) => { assert.equal(s, sampler); glCalls.push(['samplerParameteri', name, value]); },
  samplerParameterf: (s, name, value) => { assert.equal(s, sampler); glCalls.push(['samplerParameterf', name, value]); },
  getExtension: name => name === 'EXT_texture_filter_anisotropic' ? {TEXTURE_MAX_ANISOTROPY_EXT: 0x84FE} : null,
  bindSampler: (unit, s) => { assert.equal(s, sampler); glCalls.push(['bindSampler', unit]); },
  activeTexture: unit => glCalls.push(['activeTexture', unit - 0x84C0]),
  bindTexture: (target, texture) => { assert.equal(target, 0x0DE1); glCalls.push(['bindTexture', texture.name]); }};
const GL = {textures: []}, GLImmediate = {TexEnvJIT: {getActiveTexture: () => 1}};
function emjs(name, args, source = bridge) {
  const start = source.indexOf('EM_JS(void, ' + name + ',');
  assert(start >= 0, name);
  let pos = source.indexOf('{', start), end = pos + 1, depth = 1;
  while (depth) { depth += (source[end] === '{') - (source[end] === '}'); ++end; }
  return Function('Module', 'HEAPF32', 'GLctx', 'GL', 'GLImmediate', ...args, source.slice(pos + 1, end - 1))
    .bind(null, Module, HEAPF32, drawGL, GL, GLImmediate);
}
const set = emjs('WebPointLights_Set', ['packed', 'count']);
// The device hands each draw's material maps over with its lights and shadows.
const drawParams = ['view', 'enabled', 'worldShadowEnabled', 'colorOrder', 'normalMap', 'specularMap', 'detailMap',
  'detailX', 'detailY', 'lightmapped', 'specularScale', 'sunShading', 'sunVisibility'];
const drawHead = 'EM_JS(void, WebPointLights_Draw, (';
const drawSignature = bridge.slice(bridge.indexOf(drawHead) + drawHead.length, bridge.indexOf('), {', bridge.indexOf(drawHead)));
assert.deepEqual(drawSignature.split(',').map(param => param.trim().split(/[\s*]+/).pop()), drawParams);
const device = fs.readFileSync('src/Mac/DirectX_9/CDirect3DDevice.c', 'utf8');
const externHead = 'extern void WebPointLights_Draw(';
const externSignature = device.slice(device.indexOf(externHead) + externHead.length, device.indexOf(');', device.indexOf(externHead)));
assert.equal(externSignature.replace(/\s+/g, ' '), drawSignature.replace(/\s+/g, ' '), 'the device declares the bridge it calls');
const drawFloats = drawSignature.split(',').map(param => /^\s*float\s+\w+\s*$/.test(param));
assert.equal(drawFloats.filter(Boolean).length, 5);
const drawJs = emjs('WebPointLights_Draw', drawParams);
// The engine passes C floats.
const drawScene = (...args) => drawJs(...args.map((value, i) => drawFloats[i] && typeof value === 'number' ? Math.fround(value) : value));
const draw = (view, enabled) => drawScene(view, enabled, 0);
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
const generate = Function('GLImmediate', 'GLEmulation', 'fsTexEnvPass', 'texUnitVaryingList = ""',
  linked.slice(defsStart, defsEnd) + '\nreturn {pointVarying,pointDefs,pointPass,fsTexEnvPass,texUnitVaryingList};');
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
    assert.equal((shader.fsTexEnvPass + shader.pointPass).match(/texture2D\(u_texUnit0/g).length, 1,
      'material, its maps and flashes share one base texture lookup');
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
const drawEnhancements = (view, shadow) => drawScene(view, 0, shadow);
HEAPF32.set([50, 20, 0, 22, -30, 40, 10, 30], 16);
setShadows(64, 2);
HEAPF32.set([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -10, 5, -20, 1], 64);
drawEnhancements(256, 1);
assert.equal(lights.shadowDrawCount, 2);
assert.equal(lights.shadowEye[0], 40);
assert.equal(lights.shadowEye[1], 25);
assert.equal(lights.shadowEye[2], -20);
assert(Math.abs(lights.shadowEye[3] * 22 * 22 - 1) < 1e-6);
const shadowRev = lights.shadowEyeRevision;
for (let i = 0; i < 100; ++i) drawEnhancements(256, 1);
assert.equal(lights.shadowEyeRevision, shadowRev, 'repeated world draws reuse transformed shadow casters');
drawEnhancements(256, 0);
assert.equal(lights.shadowDrawCount, 0);
const modelShader = generate.call({usedTexUnitList:[0]}, {useTextureMatrix:false,TexEnvJIT:{getTexUnitType:()=>0x0DE1}}, {lightingEnabled:true}, 'gl_FragColor = texture2D(u_texUnit0, v_texCoord0.xy) * v_color;');
assert(!modelShader.pointDefs.includes('u_cod2WeaponBlur'), 'ADS does not add a texture-blur uniform');
assert.equal(modelShader.fsTexEnvPass,
  'vec4 cod2BaseTexel = texture2D(u_texUnit0, v_texCoord0.xy);\ngl_FragColor = cod2BaseTexel * v_color;',
  'weapons keep the original texture footprint and vertex lighting');
assert.equal((modelShader.fsTexEnvPass.match(/texture(?:2D)?\(/g) || []).length, 1,
  'weapons and flashes share one unbiased sample without extra passes');
const worldShader = generate.call({usedTexUnitList:[0]}, {useTextureMatrix:false,TexEnvJIT:{getTexUnitType:()=>0x0DE1}}, {lightingEnabled:false}, 'gl_FragColor = texture2D(u_texUnit0, v_texCoord0.xy) * v_color;');
assert(worldShader.pointDefs.includes('uniform int u_cod2ShadowCount; uniform highp vec4 u_cod2ShadowEye[4];'));
assert(worldShader.pointPass.includes('if (u_cod2ShadowCount > 0)'));
assert(worldShader.pointPass.includes('gl_FragColor.rgb *= cod2Shadow;'));
let shadowCountUploads = 0, shadowArrayUploads = 0;
const enhGLctx = {
  uniform1i(loc, val) { if (loc === 5) ++shadowCountUploads; else ++countUploads; },
  uniform1f() { assert.fail("material blur must not upload uniforms"); },
  uniform4fv(loc, buf) { if (loc === 6) { assert.equal(buf.length, 16); ++shadowArrayUploads; } else ++arrayUploads; }
};
const enhRenderer = {
  cod2LightCountLocation: 1, cod2LightEyeLocation: 2, cod2LightColorLocation: 3,
  cod2ShadowCountLocation: 5, cod2ShadowEyeLocation: 6,
  cod2LightCount: -1, cod2LightRevision: -1, cod2ShadowCount: -1, cod2ShadowRevision: -1
};
drawEnhancements(256, 1);
upload.call(enhRenderer, Module, enhGLctx);
assert.equal(shadowCountUploads, 1); assert.equal(shadowArrayUploads, 1);
for (let i = 0; i < 200; ++i) upload.call(enhRenderer, Module, enhGLctx);
assert.equal(shadowCountUploads, 1); assert.equal(shadowArrayUploads, 1);
console.log('PASS: 121 camera transforms, cached uniform uploads, sharp weapon materials, ground shadows, HUD isolation and all 48 shader texture/normal variants');

// A camera change while one channel is inactive must invalidate that channel.
HEAPF32.set([100,20,60,200,1,.7,.3,0],8); set(32,1);
HEAPF32[76]=-10; drawScene(256,1,1);
const oldLightX=lights.eye[0], oldShadowX=lights.shadowEye[0];
HEAPF32[76]=-30; drawScene(256,0,1); drawScene(256,1,0);
assert.equal(lights.eye[0],oldLightX-20);
HEAPF32[76]=-50; drawScene(256,1,0); drawScene(256,0,1);
assert.equal(lights.shadowEye[0],oldShadowX-40);
const revisions=[lights.eyeRevision,lights.shadowEyeRevision];
for(let i=0;i<1000;i++)drawScene(256,1,1);
assert.deepEqual([lights.eyeRevision,lights.shadowEyeRevision],revisions);
drawScene(0,0,0); assert.equal(lights.drawCount,0); assert.equal(lights.shadowDrawCount,0);
console.log('PASS: shared camera transform cache survives inactive light/shadow channels and 1000 combined draws');

// Vertex format is independent of FX existence and is consumed by one draw.
let colorUploads=0;const colorRenderer={cod2ColorOrderLocation:7,cod2ColorOrder:-1,cod2LightCountLocation:null};
const colorGL={uniform1i(loc,value){assert.equal(loc,7);assert(value>=0&&value<=2);colorUploads++;}};
const savedLights=Module.cod2PointLights;delete Module.cod2PointLights;
for(const order of [0,2,1,0]){
 for(let i=0;i<600;i++){
  drawScene(0,0,0,order);assert.equal(Module.cod2VertexColorOrder,order);
  upload.call(colorRenderer,Module,colorGL);assert.equal(colorRenderer.cod2ColorOrder,order);
  assert.equal(Module.cod2VertexColorOrder,0);
 }
}
assert.equal(colorUploads,4);
drawScene(0,0,0,2);upload.call(colorRenderer,Module,colorGL);
upload.call(colorRenderer,Module,colorGL);assert.equal(colorRenderer.cod2ColorOrder,0,'generic subsequent GL draws use RGBA');
Module.cod2PointLights=savedLights;
console.log('PASS: 2400 RGBA/BGRA/ARGB draws with no FX lights; cached uploads and generic-draw reset');

// Aim depth of field filters the resolved scene (web_scene_fx.c). Materials
// keep their authored texels: no per-draw blur, emboss or rim. Relief and
// highlights come only from the material's own normal and specular maps.
for (const shader of [modelShader, worldShader]) {
  const code = shader.pointDefs + shader.fsTexEnvPass + shader.pointPass;
  for (const name of ['u_cod2SceneFx', 'cod2Dof', 'cod2VmBlur', 'cod2SightKeep', 'cod2Periph', 'cod2Wbump', 'cod2Rim'])
    assert(!code.includes(name), name + ' is not compiled into material shaders');
  assert.equal(code.match(/texture2D\(u_texUnit0/g).length, 1, 'one base texture lookup per material sample');
  for (const lookup of code.match(/texture(?:2D|Cube)?\(\w+/g))
    assert(['texture2D(u_texUnit0', 'texture2D(u_cod2DetailMap', 'texture2D(u_cod2NormalMap', 'texture2D(u_cod2SpecMap'].includes(lookup), lookup);
  assert(!/\bcod2Spec\b/.test(code) || code.includes('u_cod2Surface.y > 0.5 ? texture2D(u_cod2SpecMap'),
    'highlights are scaled by the specular map, which only a material with one samples');
}
for (const name of ['WebPointLights_SetAds', 'WebPointLights_ApplyAdsPostFx', 'sceneFx'])
  assert(!bridge.includes(name), name + ' no longer runs per draw or per 2D transition');
assert(!linked.includes('cod2SceneFxLocation'), 'material programs no longer upload scene FX uniforms');
console.log('PASS: material shaders and light uploads carry no ADS blur, emboss or scene FX state');

// The world's sun reaches the shaders once per change, not per scene.
const setSunJs = emjs('WebMaterials_SetSunJs', ['x', 'y', 'z', 'r', 'g', 'b'],
  fs.readFileSync('downstream/wasm/web_materials.c', 'utf8'));
// The engine passes C floats.
const setSun = (...values) => setSunJs(...values.map(Math.fround));
setSun(0.6, 0, 0.8, 0.5, 0.4, 0.3);
const sun = Module.cod2Sun;
assert.equal(sun.revision, 1);
assert.deepEqual([...sun.world, ...sun.color], [0.6, 0, 0.8, 0.5, 0.4, 0.3].map(Math.fround));
for (let i = 0; i < 1000; ++i) setSun(0.6, 0, 0.8, 0.5, 0.4, 0.3);
assert.equal(sun.revision, 1, 'an unchanged sun keeps its revision');
setSun(0.6, 0, 0.8, 0.5, 0.4, 0.31);
assert.equal(Module.cod2Sun, sun); assert.equal(sun.revision, 2);
setSun(0.6, 0, 0.8, 0.5, 0.4, 0.3);
assert.equal(sun.revision, 3);

// Maps bind on units 5-7 through one sampler with mipmaps, repeat and 4x
// anisotropy: the engine sets sampler state only on the stages it binds.
const material = Module.cod2Material;
assert.deepEqual(glCalls.splice(0), [['createSampler'], ['samplerParameteri', 0x2801, 0x2703],
  ['samplerParameteri', 0x2800, 0x2601], ['samplerParameteri', 0x2802, 0x2901], ['samplerParameteri', 0x2803, 0x2901],
  ['samplerParameterf', 0x84FE, 4], ['bindSampler', 5], ['bindSampler', 6], ['bindSampler', 7]],
  'one sampler across every earlier draw');
GL.textures[21] = {name: 'detail'}; GL.textures[22] = {name: 'normal'}; GL.textures[23] = {name: 'specular'};
GL.textures[24] = {name: 'other'};
const rotation = (yaw, pitch) => {
  const cy = Math.cos(yaw), sy = Math.sin(yaw), cp = Math.cos(pitch), sp = Math.sin(pitch);
  return [cy, sy * cp, sy * sp, 0, -sy, cy * cp, cy * sp, 0, 0, -sp, cp, 0, -40, 11, -55, 1];
};
HEAPF32.set(rotation(0.7, -0.3), 64);
const full = [22, 23, 21, 8, 6, 1, 1.5, 0.65, 0.25];
drawScene(256, 1, 1, 0, ...full);
assert.deepEqual([...material.surface], [1, 1, 1, 1.5]);
assert.deepEqual([...material.detail], [8, 6, 1, 0]);
assert.equal(material.model[0], 0.25);
assert.deepEqual(glCalls.splice(0), [['activeTexture', 5], ['bindTexture', 'detail'], ['activeTexture', 6],
  ['bindTexture', 'normal'], ['activeTexture', 7], ['bindTexture', 'specular'], ['activeTexture', 1]],
  'maps bind once and the engine\'s active unit returns');
for (let i = 0; i < 1000; ++i) drawScene(256, 1, 1, 0, ...full);
assert.deepEqual(glCalls, [], '1000 draws of one material bind nothing more');
// A material without maps leaves the units alone; its flags turn them off.
drawScene(256, 1, 1, 0, 0, 0, 0, 8, 6, 0, 1, 0.65, 0);
assert.deepEqual(glCalls, []);
assert.deepEqual([...material.surface], [0, 0, 0, 1]);
assert.deepEqual([...material.detail], [0, 0, 0, 0]);
assert.equal(material.model[0], 0);
// Only a changed unit rebinds, including a name reused for a new texture.
drawScene(256, 1, 1, 0, 24, 23, 21, 8, 6, 1, 1, 0.65, 0);
assert.deepEqual(glCalls.splice(0), [['activeTexture', 6], ['bindTexture', 'other'], ['activeTexture', 1]]);
GL.textures[23] = {name: 'specular 2'};
drawScene(256, 1, 1, 0, 24, 23, 21, 8, 6, 1, 1, 0.65, 0);
assert.deepEqual(glCalls.splice(0), [['activeTexture', 7], ['bindTexture', 'specular 2'], ['activeTexture', 1]]);
// A name without a texture turns its map off instead of sampling the last.
delete GL.textures[24];
drawScene(256, 1, 1, 0, 24, 0, 0, 8, 6, 1, 1, 0.65, 0);
assert.deepEqual(glCalls, []);
assert.deepEqual([...material.surface], [0, 0, 1, 1]);
console.log('PASS: one mipmapped, repeating, anisotropic sampler for units 5-7; maps rebound only when a unit\'s texture changes; flags follow the textures that exist');

// The sun's direction and the sky's (world up) in eye space, transformed
// again only when the view, the sun or r_sunShading changes.
const eyeOf = (v, w) => {
  const e = [0, 1, 2].map(r => v[r] * w[0] + v[4 + r] * w[1] + v[8 + r] * w[2]), l = Math.hypot(...e);
  return e.map(x => x / l);
};
const near = (actual, expected, message) =>
  expected.forEach((x, i) => assert(Math.abs(actual[i] - x) < 1e-6, `${message}: ${[...actual]} vs ${expected}`));
const sunDraw = shading => drawScene(256, 1, 1, 0, 22, 23, 21, 8, 6, 1, 1, shading, 0);
for (const [yaw, pitch] of [[0.7, -0.3], [-2.1, 0.4], [3.1, -1.2]]) {
  const view = rotation(yaw, pitch);
  HEAPF32.set(view, 64); sunDraw(0.65);
  near(material.sunEye, eyeOf(Float32Array.from(view), sun.world), 'sun in eye space');
  near(material.skyEye, eyeOf(Float32Array.from(view), [0, 0, 1]), 'sky in eye space');
  assert.deepEqual([...material.sunColor], [...sun.color, Math.fround(0.65)]);
  material.sunEye[0] = 9;
  for (let i = 0; i < 100; ++i) sunDraw(0.65);
  assert.equal(material.sunEye[0], 9, 'draws sharing a view reuse the transformed sun');
  sunDraw(0.3);
  assert.equal(material.sunColor[3], Math.fround(0.3));
  near(material.sunEye, eyeOf(Float32Array.from(view), sun.world), 'r_sunShading change');
  material.sunEye[0] = 9; setSun(0.6, 0, 0.8, 0.5, 0.4, 0.2); sunDraw(0.3);
  assert.equal(material.sunColor[2], Math.fround(0.2));
  near(material.sunEye, eyeOf(Float32Array.from(view), sun.world), 'sun change');
  setSun(0.6, 0, 0.8, 0.5, 0.4, 0.3);
  material.sunEye[0] = 9; HEAPF32[76] += 5; sunDraw(0.3);
  near(material.sunEye, eyeOf(Float32Array.from(view), sun.world), 'a moved camera');
}
// 2D draws and worlds without a sun leave the last values.
material.sunEye[0] = 9; drawScene(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0.65, 0);
assert.equal(material.sunEye[0], 9);
delete Module.cod2Sun; HEAPF32.set(rotation(1, 1), 64); sunDraw(0.65);
assert.equal(material.sunEye[0], 9);
Module.cod2Sun = sun;
console.log('PASS: sun and sky directions in eye space for 3 views, cached across 300 draws, renewed by the view, the sun or r_sunShading');

// Material uniforms upload per program, only when one of its values changes.
const materialUploads = [];
const materialGL = {uniform1i: (location, value) => materialUploads.push(['1i', location, value]),
  uniform3fv: (location, value) => materialUploads.push(['3fv', location, [...value]]),
  uniform4fv: (location, value) => materialUploads.push(['4fv', location, [...value]])};
const materialProgram = locations => Object.assign({cod2ColorOrderLocation: null, cod2LightCountLocation: null,
  cod2ShadowCountLocation: null, cod2DetailMapLocation: 'detailMap', cod2NormalMapLocation: 'normalMap',
  cod2SpecMapLocation: 'specMap', cod2SurfaceLocation: 'surface', cod2DetailLocation: 'detail', cod2SunEyeLocation: 'sunEye',
  cod2SunColorLocation: 'sunColor', cod2SkyEyeLocation: 'skyEye', cod2ModelLocation: null, cod2MaterialUniforms: null}, locations);
const worldProgram = materialProgram({}), modelProgram = materialProgram({cod2ModelLocation: 'model'});
const detailProgram = materialProgram({cod2SurfaceLocation: null, cod2SunEyeLocation: null, cod2SunColorLocation: null,
  cod2SkyEyeLocation: null});
const samplers = [['1i', 'detailMap', 5], ['1i', 'normalMap', 6], ['1i', 'specMap', 7]];
const all = names => names.map(name => [material[name].length === 3 ? '3fv' : '4fv', name, [...material[name]]]);
HEAPF32.set(rotation(0.2, 0.1), 64);
drawScene(256, 1, 1, 0, 22, 23, 21, 8, 6, 1, 1.5, 0.65, 0.25);
upload.call(worldProgram, Module, materialGL);
assert.deepEqual(materialUploads.splice(0), [...samplers, ...all(['surface', 'detail', 'sunEye', 'sunColor', 'skyEye'])]);
upload.call(modelProgram, Module, materialGL);
assert.deepEqual(materialUploads.splice(0), [...samplers, ...all(['surface', 'detail', 'sunEye', 'sunColor', 'skyEye', 'model'])]);
for (let i = 0; i < 1000; ++i) { upload.call(worldProgram, Module, materialGL); upload.call(modelProgram, Module, materialGL); }
assert.deepEqual(materialUploads, [], 'alternating programs keep their values');
drawScene(256, 1, 1, 0, 0, 23, 0, 0, 0, 1, 1.5, 0.65, 0.25);
upload.call(worldProgram, Module, materialGL);
assert.deepEqual(materialUploads.splice(0), all(['surface', 'detail']));
drawScene(256, 1, 1, 0, 0, 23, 0, 0, 0, 1, 1.5, 0.65, 0.5);
upload.call(worldProgram, Module, materialGL);
assert.deepEqual(materialUploads, [], 'world programs have no model uniform');
upload.call(modelProgram, Module, materialGL);
assert.deepEqual(materialUploads.splice(0), all(['surface', 'detail', 'model']));
// Programs that only apply the detail map still receive it.
upload.call(detailProgram, Module, materialGL);
assert.deepEqual(materialUploads.splice(0), [...samplers, ...all(['detail'])]);
console.log('PASS: material uniforms upload once per program and value across 2000 alternating world/model draws; detail-only programs');

// The generated passes: a lightmapped world variant and a lit model with
// three of the eight fixed-function lights.
const twoD = {TexEnvJIT: {getTexUnitType: () => 0x0DE1}};
for (const matrix of [false, true]) {
  const uv = unit => matrix ? `(u_textureMatrix${unit} * v_texCoord${unit}).xy` : `v_texCoord${unit}.xy`;
  const lightmap = `texture2D(u_texUnit1, ${uv(1)})`;
  const world = generate.call({usedTexUnitList: [0, 1]}, {...twoD, useTextureMatrix: matrix}, {lightingEnabled: false},
    `vec4 c = texture2D(u_texUnit0, ${uv(0)}) * (${lightmap} * 2.0);\ngl_FragColor = vec4(c.rgb, texture2D(u_texUnit0, ${uv(0)}).a);`,
    'varying vec4 v_texCoord0;\nvarying vec4 v_texCoord1;\nvarying vec4 v_color;\n');
  assert.equal(world.texUnitVaryingList, 'varying highp vec4 v_texCoord0;\nvarying highp vec4 v_texCoord1;\nvarying vec4 v_color;\n',
    'UV derivatives read full-precision coordinates');
  assert.equal(world.fsTexEnvPass.split('\n')[0], `vec4 cod2LightmapTexel = ${lightmap};`);
  assert.equal((world.fsTexEnvPass + world.pointPass).split(lightmap).length, 2, 'one lightmap lookup');
  assert.equal((world.fsTexEnvPass + world.pointPass).match(/texture2D\(u_texUnit0/g).length, 1);
  assert(world.pointPass.startsWith('if (u_cod2Detail.z > 0.5)\n'));
  const lit = world.pointPass.indexOf('if (u_cod2Surface.z > 0.5) {');
  assert(lit > 0 && lit < world.pointPass.indexOf('if (u_cod2LightCount > 0)'), 'flashes add after the material');
  assert(world.pointPass.includes('vec4 cod2L = cod2LightmapTexel;'));
  assert(world.pointPass.includes(`cod2Bump(cod2N, ${uv(0)}, 0.75)`));
  assert(!world.pointPass.includes('gl_FragColor.a'), 'materials keep their alpha');
  const model = generate.call({usedTexUnitList: [0]}, {...twoD, useTextureMatrix: matrix},
    {lightingEnabled: true, MAX_LIGHTS: 8, lightEnabled: [true, true, false, true, false, false, false, false]},
    `gl_FragColor = texture2D(u_texUnit0, ${uv(0)}) * v_color;`);
  for (const id of [0, 1, 3]) {
    assert(model.pointDefs.includes(`uniform highp vec4 u_lightAmbient${id}; uniform highp vec4 u_lightDiffuse${id}; uniform highp vec4 u_lightPosition${id};`));
    assert(model.pointPass.includes(`{ highp vec3 l = normalize(u_lightPosition${id}.xyz);`));
  }
  assert(!/u_light\w+2\b/.test(model.pointDefs + model.pointPass), 'disabled lights add nothing');
  assert.deepEqual(model.pointPass.match(/cod2Spec \+= u_lightDiffuse\d/g),
    ['cod2Spec += u_lightDiffuse0', 'cod2Spec += u_lightDiffuse3'], 'highlights from the strongest grid light and the sun');
  assert(model.pointPass.includes(`cod2Bump(cod2N, ${uv(0)}, 1.0)`));
  assert(!model.pointPass.includes('cod2LightmapTexel') && !world.pointPass.includes('u_cod2Model'));
  assert.equal(model.pointPass.match(/texture2D\(u_cod2SpecMap/g).length, 1);
  assert(!model.pointPass.includes('gl_FragColor.a'));
  const single = generate.call({usedTexUnitList: [0]}, {...twoD, useTextureMatrix: matrix},
    {lightingEnabled: true, MAX_LIGHTS: 8, lightEnabled: [false, false, false, false, false, true, false, false]},
    `gl_FragColor = texture2D(u_texUnit0, ${uv(0)}) * v_color;`);
  assert.deepEqual(single.pointPass.match(/cod2Spec \+= u_lightDiffuse\d/g), ['cod2Spec += u_lightDiffuse5']);
}
console.log('PASS: lightmapped world and lit model material passes, with and without texture matrices: one lightmap lookup, enabled lights only, highlights from the grid\'s first and last lights');
