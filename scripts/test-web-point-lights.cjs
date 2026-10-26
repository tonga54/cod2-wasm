// Exercise the actual scene/view handoff and linked shader/uniform generation.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const bridge = fs.readFileSync('downstream/wasm/web_point_lights.c', 'utf8');
const linked = fs.readFileSync(process.argv[2] || 'out/cod2-wasm-core/site/cod2.js', 'utf8');
const Module = {}, HEAPF32 = new Float32Array(256);
// The draw handoff binds material maps on the engine's own context.
const glCalls = [], sampler = {};
let activeUnit = 0x84C1;
const drawGL = {TEXTURE0: 0x84C0, TEXTURE5: 0x84C5, TEXTURE18: 0x84D2, TEXTURE_2D: 0x0DE1, TEXTURE_MIN_FILTER: 0x2801,
  TEXTURE_MAG_FILTER: 0x2800, TEXTURE_WRAP_S: 0x2802, TEXTURE_WRAP_T: 0x2803, LINEAR_MIPMAP_LINEAR: 0x2703,
  LINEAR: 0x2601, REPEAT: 0x2901, NEAREST: 0x2600, ACTIVE_TEXTURE: 0x84E0, DEPTH_COMPONENT16: 0x81A5,
  DEPTH_COMPONENT: 0x1902, UNSIGNED_SHORT: 0x1403, TEXTURE_COMPARE_MODE: 0x884C, COMPARE_REF_TO_TEXTURE: 0x884E,
  TEXTURE_COMPARE_FUNC: 0x884D, LEQUAL: 0x0203,
  createSampler: () => (glCalls.push(['createSampler']), sampler),
  samplerParameteri: (s, name, value) => { assert.equal(s, sampler); glCalls.push(['samplerParameteri', name, value]); },
  samplerParameterf: (s, name, value) => { assert.equal(s, sampler); glCalls.push(['samplerParameterf', name, value]); },
  getExtension: name => name === 'EXT_texture_filter_anisotropic' ?
    {TEXTURE_MAX_ANISOTROPY_EXT: 0x84FE, MAX_TEXTURE_MAX_ANISOTROPY_EXT: 0x84FF} : null,
  getParameter: name => name === 0x84E0 ? activeUnit : name === 0x84FF ? 16 : assert.fail('parameter ' + name),
  bindSampler: (unit, s) => { assert.equal(s, sampler); glCalls.push(['bindSampler', unit]); },
  activeTexture: unit => { activeUnit = unit; glCalls.push(['activeTexture', unit - 0x84C0]); },
  bindTexture: (target, texture) => { assert.equal(target, 0x0DE1); glCalls.push(['bindTexture', texture.name]); },
  createTexture: () => ({name: 'sun shadow default'}),
  texImage2D: (...args) => glCalls.push(['texImage2D', ...args.slice(1, 8), [...args[8]]]),
  texParameteri: (target, name, value) => { assert.equal(target, 0x0DE1); glCalls.push(['texParameteri', name, value]); }};
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
  'detailX', 'detailY', 'lightmapped', 'specularScale', 'sunShading', 'sunVisibility', 'camoMap', 'camoMask', 'camoTiles',
  'camoReference', 'lodBias', 'reflection', 'cavity', 'variation', 'detailRelief', 'sharpen', 'viewmodel'];
const drawHead = 'EM_JS(void, WebPointLights_Draw, (';
const drawSignature = bridge.slice(bridge.indexOf(drawHead) + drawHead.length, bridge.indexOf('), {', bridge.indexOf(drawHead)));
assert.deepEqual(drawSignature.split(',').map(param => param.trim().split(/[\s*]+/).pop()), drawParams);
const device = fs.readFileSync('src/Mac/DirectX_9/CDirect3DDevice.c', 'utf8');
const externHead = 'extern void WebPointLights_Draw(';
const externSignature = device.slice(device.indexOf(externHead) + externHead.length, device.indexOf(');', device.indexOf(externHead)));
assert.equal(externSignature.replace(/\s+/g, ' '), drawSignature.replace(/\s+/g, ' '), 'the device declares the bridge it calls');
const drawFloats = drawSignature.split(',').map(param => /^\s*float\s+\w+\s*$/.test(param));
assert.equal(drawFloats.filter(Boolean).length, 13);
const drawJs = emjs('WebPointLights_Draw', drawParams);
// The engine passes C floats, and passes every argument: the ones a check
// leaves out are zero.
const drawScene = (...args) => drawJs(...drawParams.map((_, i) => i < args.length ? args[i] : 0)
  .map((value, i) => drawFloats[i] && typeof value === 'number' ? Math.fround(value) : value));
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
  linked.slice(defsStart, defsEnd) + '\nreturn {pointVarying,pointDefs,pointPass,fsTexEnvPass,texUnitVaryingList,cod2EffectPass};');
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
assert(linked.includes('fsTexEnvPass, pointPass, cod2EffectPass, fogPass, fsAlphaTestPass'),
  'light, then the feather and heat of effects, fade into the original fog');
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
  'vec4 cod2BaseTexel = texture2D(u_texUnit0, v_texCoord0.xy, u_cod2Detail.w);\nfloat cod2Paint = 0.0;\n' +
  'if (u_cod2Camo.x > 0.5) cod2BaseTexel.rgb = cod2Camouflage(cod2BaseTexel.rgb, v_texCoord0.xy, cod2Paint);\n' +
  'gl_FragColor = cod2BaseTexel * v_color;',
  'weapons keep the original texture footprint and vertex lighting; a camouflaged gun paints its texel before both');
assert.equal((modelShader.fsTexEnvPass.match(/texture(?:2D)?\(/g) || []).length, 1,
  'weapons and flashes share one sample without extra passes; its bias is the scaled scene\'s, never positive');
// The pattern repeats over the gun's own UVs where the mask paints, as bright
// as the texel under it against the painted median; paint dulls highlights.
assert(modelShader.pointDefs.includes('uniform sampler2D u_cod2CamoMap; uniform sampler2D u_cod2CamoMask; uniform vec4 u_cod2Camo;\n'));
assert(modelShader.pointDefs.includes('vec3 cod2Camouflage(vec3 base, highp vec2 uv, out float paint) {\n' +
  '  paint = texture2D(u_cod2CamoMask, uv).r;\n  vec3 camo = texture2D(u_cod2CamoMap, uv * u_cod2Camo.y).rgb;\n' +
  '  float wear = clamp(dot(base, vec3(0.299, 0.587, 0.114)) / u_cod2Camo.z, 0.5, 1.5);\n' +
  '  return mix(base, camo * (1.0 + 0.4 * (wear - 1.0)), paint);\n}\n'));
const specularLine = 'vec4 cod2S = u_cod2Surface.y > 0.5 ? texture2D(u_cod2SpecMap, v_texCoord0.xy) : vec4(0.0);\n';
assert(modelShader.pointPass.includes(specularLine + 'cod2S.rgb *= 1.0 - 0.5 * cod2Paint;\n'));
const worldShader = generate.call({usedTexUnitList:[0]}, {useTextureMatrix:false,TexEnvJIT:{getTexUnitType:()=>0x0DE1}}, {lightingEnabled:false}, 'gl_FragColor = texture2D(u_texUnit0, v_texCoord0.xy) * v_color;');
for (const code of [worldShader.pointDefs + worldShader.fsTexEnvPass + worldShader.pointPass,
                    noSample.pointDefs + noSample.fsTexEnvPass + noSample.pointPass])
  assert(!/cod2Camo|cod2Paint/.test(code), 'the world and materials without a base texel are never painted');
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
    assert(['texture2D(u_texUnit0', 'texture2D(u_cod2DetailMap', 'texture2D(u_cod2NormalMap', 'texture2D(u_cod2SpecMap',
      'texture2D(u_cod2CamoMask', 'texture2D(u_cod2CamoMap'].includes(lookup), lookup);
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

// Before the sun's depth map exists, a one-texel map at the far plane keeps
// the world shaders' shadow sampler on unit 18 valid, and the engine's
// active unit returns. Maps bind on units 5-7 and 16-17 through one sampler
// with mipmaps, repeat and the GPU's anisotropy up to 16x: the engine sets
// sampler state only on the stages it binds.
const material = Module.cod2Material;
assert.deepEqual(glCalls.splice(0), [['activeTexture', 18], ['bindTexture', 'sun shadow default'],
  ['texImage2D', 0, 0x81A5, 1, 1, 0, 0x1902, 0x1403, [65535]], ['texParameteri', 0x2801, 0x2600],
  ['texParameteri', 0x2800, 0x2600], ['texParameteri', 0x884C, 0x884E], ['texParameteri', 0x884D, 0x0203],
  ['activeTexture', 1],
  ['createSampler'], ['samplerParameteri', 0x2801, 0x2703],
  ['samplerParameteri', 0x2800, 0x2601], ['samplerParameteri', 0x2802, 0x2901], ['samplerParameteri', 0x2803, 0x2901],
  ['samplerParameterf', 0x84FE, 16], ['bindSampler', 5], ['bindSampler', 6], ['bindSampler', 7], ['bindSampler', 16],
  ['bindSampler', 17]],
  'one default shadow map and one sampler across every earlier draw');
assert(material.sunShadowDefault);
// The scene compositor binds its passes' sources from unit 8 and unbinds
// them every frame: the maps' units, whose bindings are kept across frames,
// and their sampler stay clear of them, within WebGL2's 32 combined units.
{
  const sceneFx = fs.readFileSync('downstream/wasm/web_scene_fx.c', 'utf8');
  // ... and the effects' copy of the scene's depth stays bound on unit 19.
  assert.deepEqual([...new Set(sceneFx.match(/gl\.TEXTURE\d+/g))].sort(), ['gl.TEXTURE19', 'gl.TEXTURE8'],
    'the compositor binds from unit 8, the effects\' depth on 19');
  assert(linked.includes('GLctx.uniform1i(this.cod2FeatherDepthLocation, 19);'));
  assert(sceneFx.includes('gl.activeTexture(gl.TEXTURE19);\n                gl.bindTexture(gl.TEXTURE_2D, f.depth);'));
  assert(sceneFx.includes('gl.activeTexture(gl.TEXTURE8 + fx.unit(unit));'));
  assert(sceneFx.includes('gl.uniform1i(gl.getUniformLocation(program, samplers[s]), 8 + fx.unit(s));'));
  const unitSource = sceneFx.match(/fx\.unit = (function\(s\) \{[^}]*\});/);
  assert(unitSource, 'the compositor maps its samplers to units');
  const fxUnit = new Function('return ' + unitSource[1])();
  const sources = sceneFx.match(/var sources = \[([\s\S]*?)\];/);
  assert(sources, 'the compositor names its sources');
  const fxUnits = sources[1].split(',').map((_, i) => 8 + fxUnit(i));
  assert.deepEqual(fxUnits, [8, 9, 10, 11, 12, 13, 14, 15, 20, 21]);
  assert.deepEqual(material.units, [5, 6, 7, 16, 17]);
  for (const unit of [...material.units, 18, 19]) assert(!fxUnits.includes(unit) && unit < 32, `unit ${unit}`);
  assert(!material.units.includes(19), 'the effects\' depth shares no unit with the material maps or the sun\'s');
  // The sun's depth map stays bound on unit 18 across frames.
  const sunShadows = fs.readFileSync('downstream/wasm/web_sun_shadows.c', 'utf8');
  assert.deepEqual([...new Set(sunShadows.match(/gl\.TEXTURE\d+/g))], ['gl.TEXTURE18']);
  assert(linked.includes('GLctx.uniform1i(this.cod2SunShadowMapLocation, 18);'));
}
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
// The color map's mip bias follows each draw: a scaled scene's 3D draws take
// a finer mip, the next 2D draw none.
drawScene(256, 1, 1, 0, ...full, 0, 0, 0, 0, -0.5849625);
assert.equal(material.detail[3], Math.fround(-0.5849625));
drawScene(0, 0, 0, 0, ...full);
assert.equal(material.detail[3], 0);
// r_reflections rides in the sky's w for every draw, whatever its view.
drawScene(256, 1, 1, 0, ...full, 0, 0, 0, 0, 0, 1);
assert.equal(material.skyEye.length, 4);
assert.equal(material.skyEye[3], 1);
drawScene(0, 0, 0, 0, ...full, 0, 0, 0, 0, 0, 1);
assert.equal(material.skyEye[3], 1);
drawScene(256, 1, 1, 0, ...full);
assert.equal(material.skyEye[3], 0);
// r_cavity rides in the relief's x for every draw, at most 2; a broken value is none.
drawScene(256, 1, 1, 0, ...full, 0, 0, 0, 0, 0, 0, 1);
assert.equal(material.relief.length, 4);
assert.equal(material.relief[0], 1);
drawScene(256, 1, 1, 0, ...full, 0, 0, 0, 0, 0, 0, 3);
assert.equal(material.relief[0], 2);
drawScene(0, 0, 0, 0, ...full, 0, 0, 0, 0, 0, 0, 0.5);
assert.equal(material.relief[0], 0.5);
drawScene(256, 1, 1, 0, ...full, 0, 0, 0, 0, 0, 0, NaN);
assert.equal(material.relief[0], 0);
drawScene(256, 1, 1, 0, ...full);
assert.equal(material.relief[0], 0);
// r_surfaceVariation, r_detailRelief and r_textureSharpen ride in its y, z and w
// alike: at most 2, and none for a broken or missing value.
drawScene(256, 1, 1, 0, ...full, 0, 0, 0, 0, 0, 0, 1, 1, 0.5, 2);
assert.deepEqual([...material.relief], [1, 1, 0.5, 2]);
drawScene(256, 1, 1, 0, ...full, 0, 0, 0, 0, 0, 0, 0, 3, 7, 0.25);
assert.deepEqual([...material.relief], [0, 2, 2, 0.25]);
drawScene(256, 1, 1, 0, ...full, 0, 0, 0, 0, 0, 0, 0, NaN, -1, NaN);
assert.deepEqual([...material.relief], [0, 0, 0, 0]);
drawScene(256, 1, 1, 0, ...full, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1);
drawScene(256, 1, 1, 0, ...full);
assert.deepEqual([...material.relief], [0, 0, 0, 0]);
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
// A camouflaged gun's pattern and mask bind on units 16 and 17; it is
// painted only with both, and the next gun without paint turns it off.
GL.textures[25] = {name: 'pattern'}; GL.textures[26] = {name: 'mask'};
const painted = [22, 23, 21, 8, 6, 1, 1.5, 0.65, 0.25, 25, 26, 2.5, 0.37];
drawScene(256, 1, 1, 0, ...painted);
assert.deepEqual(glCalls.splice(0), [['activeTexture', 6], ['bindTexture', 'normal'], ['activeTexture', 16],
  ['bindTexture', 'pattern'], ['activeTexture', 17], ['bindTexture', 'mask'], ['activeTexture', 1]]);
assert.deepEqual([...material.camo], [1, 2.5, Math.fround(0.37), 0]);
for (let i = 0; i < 1000; ++i) drawScene(256, 1, 1, 0, ...painted);
assert.deepEqual(glCalls, [], '1000 draws of one painted gun bind nothing more');
drawScene(256, 1, 1, 0, 22, 23, 21, 8, 6, 1, 1.5, 0.65, 0.25);
assert.deepEqual([...material.camo], [0, 0, 0, 0]);
delete GL.textures[26];
drawScene(256, 1, 1, 0, ...painted);
assert.deepEqual([...material.camo], [0, 0, 0, 0], 'a pattern without its mask paints nothing');
GL.textures[26] = {name: 'mask'};
drawScene(256, 1, 1, 0, ...painted);
assert.deepEqual(glCalls.splice(0), [['activeTexture', 17], ['bindTexture', 'mask'], ['activeTexture', 1]]);
assert.equal(material.camo[0], 1);
console.log('PASS: one mipmapped, repeating, anisotropic sampler for units 5-7 and 16-17, clear of the scene compositor\'s; maps rebound only when a unit\'s texture changes; flags follow the textures that exist; camouflage only with its pattern and mask');

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
// r_reflections reaches world and model programs alike, through the sky.
drawScene(256, 1, 1, 0, 22, 23, 21, 8, 6, 1, 1.5, 0.65, 0.25, 0, 0, 0, 0, 0, 1);
upload.call(worldProgram, Module, materialGL);
assert.deepEqual(materialUploads.splice(0), all(['skyEye']));
assert.equal(all(['skyEye'])[0][2][3], 1);
upload.call(modelProgram, Module, materialGL);
assert.deepEqual(materialUploads.splice(0), all(['skyEye']));
drawScene(256, 1, 1, 0, 22, 23, 21, 8, 6, 1, 1.5, 0.65, 0.25);
upload.call(worldProgram, Module, materialGL);
upload.call(modelProgram, Module, materialGL);
assert.deepEqual(materialUploads.splice(0), [...all(['skyEye']), ...all(['skyEye'])]);
drawScene(256, 1, 1, 0, 0, 23, 0, 0, 0, 1, 1.5, 0.65, 0.25);
upload.call(worldProgram, Module, materialGL);
assert.deepEqual(materialUploads.splice(0), all(['surface', 'detail']));
drawScene(256, 1, 1, 0, 0, 23, 0, 0, 0, 1, 1.5, 0.65, 0.5);
upload.call(worldProgram, Module, materialGL);
assert.deepEqual(materialUploads, [], 'world programs have no model uniform');
upload.call(modelProgram, Module, materialGL);
assert.deepEqual(materialUploads.splice(0), all(['surface', 'detail', 'model']));
// r_cavity reaches the programs that take the relief, once per value.
const reliefProgram = materialProgram({cod2ReliefLocation: 'relief'});
drawScene(256, 1, 1, 0, 22, 23, 21, 8, 6, 1, 1.5, 0.65, 0.25, 0, 0, 0, 0, 0, 0, 1);
upload.call(reliefProgram, Module, materialGL);
assert.deepEqual(materialUploads.splice(0), [...samplers, ...all(['surface', 'detail', 'sunEye', 'sunColor', 'skyEye', 'relief'])]);
assert.equal(all(['relief'])[0][2][0], 1);
for (let i = 0; i < 1000; ++i) upload.call(reliefProgram, Module, materialGL);
assert.deepEqual(materialUploads, []);
drawScene(256, 1, 1, 0, 22, 23, 21, 8, 6, 1, 1.5, 0.65, 0.25, 0, 0, 0, 0, 0, 0, 0.5);
upload.call(reliefProgram, Module, materialGL);
assert.deepEqual(materialUploads.splice(0), all(['relief']));
upload.call(worldProgram, Module, materialGL);
assert(!materialUploads.splice(0).some(([, location]) => location === 'relief'), 'programs without the relief ignore it');
// Programs that only apply the detail map still receive it.
upload.call(detailProgram, Module, materialGL);
assert.deepEqual(materialUploads.splice(0), [...samplers, ...all(['detail'])]);
// A lit model program takes the camouflage's units and flag; the others never do.
const camoProgram = materialProgram({cod2ModelLocation: 'model', cod2CamoLocation: 'camo',
  cod2CamoMapLocation: 'camoMap', cod2CamoMaskLocation: 'camoMask'});
drawScene(256, 1, 1, 0, 22, 23, 21, 8, 6, 1, 1.5, 0.65, 0.25, 25, 26, 2.5, 0.37);
upload.call(camoProgram, Module, materialGL);
assert.deepEqual(materialUploads.splice(0), [...samplers, ['1i', 'camoMap', 16], ['1i', 'camoMask', 17],
  ...all(['surface', 'detail', 'sunEye', 'sunColor', 'skyEye', 'model', 'camo'])]);
upload.call(modelProgram, Module, materialGL);
assert(!materialUploads.splice(0).some(([, location]) => /^camo/.test(location)), 'programs without camouflage ignore its values');
for (let i = 0; i < 1000; ++i) upload.call(camoProgram, Module, materialGL);
assert.deepEqual(materialUploads, []);
drawScene(256, 1, 1, 0, 22, 23, 21, 8, 6, 1, 1.5, 0.65, 0.25);
upload.call(camoProgram, Module, materialGL);
assert.deepEqual(materialUploads.splice(0), all(['camo']), 'an unpainted gun turns the paint off');
// The arms and weapon, drawn in the depth hack's range, reach the model
// programs in the model's y, once per value; world programs never take it.
drawScene(256, 1, 1, 0, 22, 23, 21, 8, 6, 1, 1.5, 0.65, 0.25, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1);
upload.call(modelProgram, Module, materialGL);
assert.deepEqual(materialUploads.splice(0), all(['model']));
assert.deepEqual(all(['model'])[0][2], [Math.fround(0.25), 1, 0, 0]);
for (let i = 0; i < 1000; ++i) upload.call(modelProgram, Module, materialGL);
assert.deepEqual(materialUploads, []);
upload.call(worldProgram, Module, materialGL);
assert(!materialUploads.splice(0).some(([, location]) => location === 'model'), 'world programs ignore the viewmodel');
drawScene(256, 1, 1, 0, 22, 23, 21, 8, 6, 1, 1.5, 0.65, 0.25);
upload.call(modelProgram, Module, materialGL);
assert.deepEqual(materialUploads.splice(0), all(['model']));
assert.equal(all(['model'])[0][2][1], 0, 'every other model turns the shoulder light off');
console.log('PASS: material uniforms upload once per program and value across 2000 alternating world/model draws; detail-only programs; camouflage on lit model programs only; r_reflections in the sky\'s w for both; r_cavity in the relief\'s x; the viewmodel in the model\'s y');

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
  assert.equal((world.fsTexEnvPass + world.pointPass).match(/texture2D\(u_texUnit0/g).length, 1,
    'one lookup of the color map; sharpening and terrain blends add their own, behind their flags');
  assert(world.pointPass.startsWith('if (u_cod2Detail.z > 0.5)\n'));
  const lit = world.pointPass.indexOf('if (u_cod2Surface.z > 0.5) {');
  assert(lit > 0 && lit < world.pointPass.indexOf('if (u_cod2LightCount > 0)'), 'flashes add after the material');
  assert(world.pointPass.includes('vec4 cod2L = cod2LightmapTexel;'));
  // COD2_RELIEF_CAVITY: the world's normal maps at the DX9 shaders' full
  // strength; where they lean far from the surface less indirect light comes.
  assert(world.pointPass.includes(`cod2Bump(cod2N, ${uv(0)}, 1.0)`));
  assert(world.pointPass.includes('vec3 cod2Lit = cod2Indirect * (cod2Sky(cod2B) / cod2Sky(cod2N) * cod2Cavity(cod2B, cod2N, cod2L.a)) +\n' +
    '  cod2SunPart * cod2Incidence;\n'), 'only the indirect light, not the sun, reads the cavity, where the bake sees no sun');
  assert(world.pointDefs.includes('float cod2Cavity(highp vec3 b, highp vec3 n, float sunlit) {\n' +
    '  return 1.0 - 0.5 * (1.0 - max(dot(b, n), 0.0)) * (1.0 - clamp(sunlit, 0.0, 1.0));\n}\n'));
  assert(world.pointDefs.includes('float cod2Sky(highp vec3 n) { return 1.0 + 0.5 * dot(n, u_cod2SkyEye.xyz); }\n'));
  // COD2_RELIEF_CREASE: the normal map's folds per world unit shade all the
  // light of a mapped surface, sun included, after its relief and before the
  // bake divides out, only with r_cavity in the relief's x.
  assert(world.pointDefs.includes('uniform vec4 u_cod2Relief;\n'));
  assert(world.pointDefs.includes('float cod2Crease(highp vec2 uv) {\n'));
  assert(world.pointDefs.includes('  highp float units = 0.5 * span * (sqrt(dot(dp1, dp1) / max(dot(t1, t1), 1e-12)) +\n' +
    '    sqrt(dot(dp2, dp2) / max(dot(t2, t2), 1e-12)));\n'), 'folds are per world unit, whatever the map\'s resolution');
  assert(world.pointDefs.includes('  vec2 mu = texture2D(u_cod2NormalMap, uv + vec2(span / size.x, 0.0)).ag;\n' +
    '  vec2 mv = texture2D(u_cod2NormalMap, uv + vec2(0.0, span / size.y)).ag;\n'), 'one footprint along +u and +v');
  assert(world.pointDefs.includes('  return 2.0 * (mu.x - m.x + mv.y - m.y) / max(units, 1e-6) *\n' +
    '    (1.0 - smoothstep(0.75, 2.25, 0.5 * (length(dp1) + length(dp2))));\n'), 'folds fade out as pixels grow');
  const crease = world.pointPass.indexOf('if (u_cod2Surface.x > 0.5 && u_cod2Relief.x > 0.0) {\n' +
    `float cod2Fold = cod2Crease(${uv(0)});\n` +
    'cod2Lit *= (1.0 - 0.22 * u_cod2Relief.x * smoothstep(0.2, 1.0, -cod2Fold)) *\n' +
    '  (1.0 + 0.06 * u_cod2Relief.x * smoothstep(0.2, 1.0, cod2Fold));\n}\n');
  assert(crease > world.pointPass.indexOf('  cod2SunPart * cod2Incidence;\n') &&
    crease < world.pointPass.indexOf('gl_FragColor.rgb *= max(cod2Lit, vec3(0.0)) / max(cod2L.rgb, vec3(0.004));'),
    'folds shade the relit light before the bake divides out');
  assert(!world.pointPass.includes('gl_FragColor.a'), 'materials keep their alpha');
  // COD2_WORLD_REFLECTION: a specular map mirrors the lightmap's indirect
  // light after the sun's highlight, behind r_reflections in the sky's w.
  assert(world.pointDefs.includes('uniform highp vec4 u_cod2SkyEye;'));
  const reflection = world.pointPass.indexOf('if (u_cod2Surface.y > 0.5 && u_cod2SkyEye.w > 0.0) {');
  assert(reflection > world.pointPass.indexOf('cod2Phong(cod2B, u_cod2SunEye, cod2P)') &&
    reflection < world.pointPass.indexOf('} else if (u_cod2SunShadow.x > 0.0) {'), 'the world mirrors after its highlight');
  assert(world.pointPass.includes('vec3 cod2Env = cod2Indirect * 2.0 * mix(0.35, 1.0, smoothstep(-0.3, 0.7, cod2Up));'));
  assert(world.pointPass.includes('float cod2Fresnel = 0.2 + 0.8 * cod2Gloss * pow(1.0 - max(dot(cod2B, cod2V), 0.0), 5.0);'),
    'rough maps keep little of the grazing gain');
  assert(world.pointPass.includes('* (cod2Fresnel * cod2Gloss * u_cod2Surface.w);'));
  assert.equal(world.pointPass.match(/texture2D\(u_cod2SpecMap/g).length, 1, 'one specular lookup');
  // COD2_SURFACE_VARIATION: two octaves of value noise over the color map's own
  // coordinates vary the tone after the detail map and before the light, the
  // ground more than walls; the paler patches turn drier and warmer and the
  // darker ones cooler, about the map's own color.
  assert(world.pointDefs.includes('highp float cod2Hash(highp vec2 p) {\n'));
  assert(world.pointDefs.includes('float cod2Noise(highp vec2 p) {\n'));
  const variation = world.pointPass.indexOf('if (u_cod2Relief.y > 0.0) {\n' +
    `highp vec2 cod2VarUv = ${uv(0)};\n` +
    'float cod2Var = 0.65 * cod2Noise(cod2VarUv * 0.43 + vec2(17.0, 31.0)) +\n' +
    '  0.35 * cod2Noise(mat2(0.8, -0.6, 0.6, 0.8) * cod2VarUv * 1.37 + vec2(5.0, 9.0));\n' +
    'float cod2Floor = dot(cod2N, u_cod2SkyEye.xyz);\n' +
    'gl_FragColor.rgb *= 1.0 + u_cod2Relief.y * mix(0.08, 0.22, cod2Floor * cod2Floor) * (2.0 * cod2Var - 1.0);\n' +
    'gl_FragColor.rgb *= 1.0 + 0.6 * min(u_cod2Relief.y, 1.5) * (cod2Var - 0.5) * vec3(0.05, -0.01, -0.16);\n}\n');
  assert(variation > world.pointPass.indexOf('if (dot(cod2N, v_cod2Eye) > 0.0) cod2N = -cod2N;') &&
    variation < world.pointPass.indexOf('highp vec3 cod2B = cod2N;'), 'the tone varies before the surface is lit');
  // COD2_DETAIL_RELIEF: the detail map tilts the normal after the normal map,
  // fading as it minifies, behind its own flag.
  assert(world.pointDefs.includes('highp vec3 cod2DetailBump(highp vec3 b, highp vec3 n, highp vec2 uv, float strength) {\n'));
  assert(world.pointDefs.includes('  float fade = clamp(1.5 - 0.5 * texels, 0.0, 1.0);\n  if (fade <= 0.0) return b;\n'),
    'the grain flattens as the detail map minifies');
  assert(world.pointDefs.includes('  highp vec3 r = b - (strength * fade) * ((hu - h) * tu + (hv - h) * tv) * s;\n'),
    'the surface leans away from where the grain rises');
  const detailRelief = world.pointPass.indexOf('if (u_cod2Detail.z > 0.5 && u_cod2Relief.z > 0.0) cod2B = cod2DetailBump(cod2B, cod2N, ' +
    `${uv(0)} * u_cod2Detail.xy, 6.0 * u_cod2Relief.z);\n`);
  assert(detailRelief > world.pointPass.indexOf(`cod2Bump(cod2N, ${uv(0)}, 1.0)`) &&
    detailRelief < world.pointPass.indexOf('float cod2NLG = max(dot(cod2N, u_cod2SunEye), 0.0)'), 'the grain lifts the relit normal');
  // The real texture environment's base texel and alpha: a magnified color map
  // is sharpened against itself a mip and a half up, and a terrain layer blended
  // by its vertex alpha follows its texture's relative brightness.
  const jit = generate.call({usedTexUnitList: [0, 1]}, {...twoD, useTextureMatrix: matrix}, {lightingEnabled: false},
    `vec4 tej_env0_texload0 = texture2D(u_texUnit0, ${uv(0)});\nvec3 tej_env0_color = v_color.rgb * tej_env0_texload0.rgb;\n` +
    'float tej_env0_alpha = v_color.a * tej_env0_texload0.a;\n' +
    `vec4 tej_env1_texload1 = ${lightmap};\ngl_FragColor = vec4(tej_env0_color * tej_env1_texload1.rgb * 2.0, tej_env0_alpha);`,
    'varying vec4 v_texCoord0;\nvarying vec4 v_texCoord1;\nvarying vec4 v_color;\n');
  const baseLine = `vec4 cod2BaseTexel = texture2D(u_texUnit0, ${uv(0)}, u_cod2Detail.w);\n`;
  const sharpen = jit.fsTexEnvPass.indexOf(baseLine + 'if (u_cod2Surface.z > 0.5 && u_cod2Relief.w > 0.0) {\n');
  assert(sharpen > 0 && sharpen < jit.fsTexEnvPass.indexOf('vec4 tej_env0_texload0 = cod2BaseTexel;'), 'sharpened before it is used');
  assert(jit.fsTexEnvPass.includes('  float cod2Sharpen = 0.5 * u_cod2Relief.w * (1.0 - smoothstep(1.0, 2.5, cod2Span));\n'),
    'off past two and a half texels a pixel');
  assert(jit.fsTexEnvPass.includes(`    vec3 cod2Soft = textureLod(u_texUnit0, ${uv(0)}, max(log2(max(cod2Span, 1e-4)) + u_cod2Detail.w, 0.0) + 1.5).rgb;\n` +
    '    cod2BaseTexel.rgb = max(cod2BaseTexel.rgb + cod2Sharpen * (cod2BaseTexel.rgb - cod2Soft), vec3(0.0));\n'));
  const blend = jit.fsTexEnvPass.indexOf('float tej_env0_alpha = v_color.a * tej_env0_texload0.a;\n' +
    'if (u_cod2Surface.z > 0.5 && u_cod2Relief.y > 0.0 && v_color.a < 0.995) {\n' +
    `  float cod2Mean = dot(textureLod(u_texUnit0, ${uv(0)}, 12.0).rgb, vec3(0.299, 0.587, 0.114));\n` +
    '  float cod2Height = dot(tej_env0_texload0.rgb, vec3(0.299, 0.587, 0.114)) / max(cod2Mean, 0.02) - 1.0;\n' +
    '  float cod2Blend = clamp(v_color.a + 0.9 * u_cod2Relief.y * cod2Height * (1.0 - abs(2.0 * v_color.a - 1.0)), 0.0, 1.0);\n' +
    '  tej_env0_alpha = mix(cod2Blend, smoothstep(0.0, 1.0, cod2Blend), 0.6 * min(u_cod2Relief.y, 1.0)) * tej_env0_texload0.a;\n}\n');
  assert(blend > sharpen, 'terrain layers blend by their relief after the base texel');
  assert.equal(jit.fsTexEnvPass.match(/texture2D\(u_texUnit0/g).length, 1);
  assert.equal(jit.fsTexEnvPass.match(/textureLod\(u_texUnit0/g).length, 2, 'the sharpened blur and the layer\'s mean');
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
  assert(model.pointPass.includes('cod2Bumped += (cod2Ambient - cod2Sun) * (cod2Sky(cod2B) / cod2Sky(cod2N) * cod2Cavity(cod2B, cod2N, u_cod2Model.x)) +\n' +
    '  cod2Sun * cod2Incidence;\n'));
  assert(!model.pointPass.includes('cod2LightmapTexel') && !world.pointPass.includes('u_cod2Model'));
  assert(!model.pointPass.includes('cod2Crease('), 'models keep their relief as it was');
  assert(!/cod2Noise|cod2DetailBump|cod2Sharpen|cod2Blend|u_cod2Relief\.[yzw]/.test(model.pointDefs + model.pointPass + model.fsTexEnvPass),
    'models keep their tone, grain, sharpness and alpha');
  assert(model.pointPass.includes('if (u_cod2Surface.y > 0.5 && u_cod2SkyEye.w > 0.0) {'), 'r_reflections turns the models\' reflection off too');
  assert.equal(model.pointPass.match(/texture2D\(u_cod2SpecMap/g).length, 1);
  // COD2_VIEWMODEL_LIGHT: the arms and weapon take the material pass without
  // maps or sun, and the sky's light from above the left shoulder after their
  // relief and before the sun's highlight.
  assert(model.pointPass.includes('if (u_cod2Surface.x > 0.5 || u_cod2Surface.y > 0.5 || u_cod2Model.x > 0.0 || u_cod2Model.y > 0.5) {\n'),
    'the viewmodel is lit by its form without maps or sun');
  const shoulder = model.pointPass.indexOf('if (u_cod2Model.y > 0.5) {\n' +
    'const highp vec3 cod2Shoulder = vec3(-0.454, 0.655, 0.605);\n' +
    'gl_FragColor.rgb *= mix(0.68, 1.28, smoothstep(-0.5, 0.9, dot(cod2B, cod2Shoulder)));\n' +
    'cod2Spec += cod2Flat * 0.6 * cod2Phong(cod2B, cod2Shoulder, cod2P);\n}\n');
  assert(shoulder > model.pointPass.indexOf('gl_FragColor.rgb *= max(cod2Bumped, vec3(0.0)) / max(cod2Flat, vec3(0.004));') &&
    shoulder < model.pointPass.indexOf('cod2Spec += cod2Sun * cod2Phong(cod2B, u_cod2SunEye, cod2P);'),
    'the shoulder light shades the relit model before its highlights');
  assert(Math.abs(Math.hypot(-0.454, 0.655, 0.605) - 1) < 2e-3, 'the shoulder light is a direction');
  assert(!model.pointPass.includes('gl_FragColor.a'));
  const single = generate.call({usedTexUnitList: [0]}, {...twoD, useTextureMatrix: matrix},
    {lightingEnabled: true, MAX_LIGHTS: 8, lightEnabled: [false, false, false, false, false, true, false, false]},
    `gl_FragColor = texture2D(u_texUnit0, ${uv(0)}) * v_color;`);
  assert.deepEqual(single.pointPass.match(/cod2Spec \+= u_lightDiffuse\d/g), ['cod2Spec += u_lightDiffuse5']);
}
console.log('PASS: lightmapped world and lit model material passes, with and without texture matrices: one lightmap lookup, enabled lights only, highlights from the grid\'s first and last lights, the world\'s and models\' reflections behind r_reflections, the world\'s folds per unit behind r_cavity, its tone and terrain blends behind r_surfaceVariation, its detail relief behind r_detailRelief and its magnified color maps sharpened behind r_textureSharpen');

// COD2_SUN_SHADOW: lightmapped world draws take the sun's depth map
// (web_sun_shadows.c) in their own eye space, its parameters and the casters'
// rectangle; every other draw turns it off.
{
  const project = (m, p) => [0, 1, 2].map(r => m[r] * p[0] + m[4 + r] * p[1] + m[8 + r] * p[2] + m[12 + r]);
  const worldToMap = Float32Array.from([0.0003, 0.0001, 0.00006, 0, -0.0001, 0.0003, -0.00002, 0,
    0, 0.00025, -0.0001, 0, 0.71, 0.33, 0.52, 1]);
  Module.cod2SunShadow = {world: worldToMap, params: Float32Array.from([1, 1 / 2048, 8, 0.0002]),
    rect: Float32Array.from([0.25, 0.3, 0.4, 0.45]), revision: 1};
  for (const [yaw, pitch, move] of [[0.7, -0.3, 0], [-2.1, 0.4, 35], [3.1, -1.2, -80]]) {
    const view = rotation(yaw, pitch);
    view[12] += move; view[14] -= move;
    HEAPF32.set(view, 64);
    drawScene(256, 1, 1);
    assert.deepEqual([...material.sunShadow], [...Module.cod2SunShadow.params]);
    assert.deepEqual([...material.sunShadowRect], [...Module.cod2SunShadow.rect]);
    const v = Float32Array.from(view);
    for (const point of [[468, 992, 100], [-1650, -16600, 0], [12, -40, 300]]) {
      const eye = project(v, point);
      const viaEye = project(material.sunShadowEye, eye), direct = project(worldToMap, point);
      viaEye.forEach((x, i) => assert(Math.abs(x - direct[i]) < 2e-5, `eye-space map coordinates ${viaEye} vs ${direct}`));
    }
    assert.deepEqual([...material.sunShadowEye].filter((_, i) => i % 4 === 3).map(x => x + 0), [0, 0, 0, 1]);
    // Draws sharing a view and a map reuse the eye-space matrix.
    material.sunShadowEye[0] = 9;
    for (let i = 0; i < 100; ++i) drawScene(256, 1, 1);
    assert.equal(material.sunShadowEye[0], 9);
    // A 2D draw, a model or a blended particle turns the shadow off, and the
    // next world draw turns it back on with the same matrix.
    drawScene(0, 0, 0); assert.equal(material.sunShadow[0], 0);
    drawScene(256, 1, 0); assert.equal(material.sunShadow[0], 0);
    drawScene(256, 1, 1); assert.equal(material.sunShadow[0], 1); assert.equal(material.sunShadowEye[0], 9);
    // A new frame's map is moved into the eye again.
    Module.cod2SunShadow.revision++;
    drawScene(256, 1, 1);
    assert.notEqual(material.sunShadowEye[0], 9);
  }
  // A camera that moved while only models drew still renews the world's matrix.
  HEAPF32.set(rotation(0.2, 0.1), 64); drawScene(256, 1, 1);
  HEAPF32[76] += 40; drawScene(256, 1, 0);
  drawScene(256, 1, 1);
  const eye = project(Float32Array.from(HEAPF32.subarray(64, 80)), [100, 200, 50]);
  project(material.sunShadowEye, eye).forEach((x, i) => assert(Math.abs(x - project(worldToMap, [100, 200, 50])[i]) < 2e-5));
  // A frame without casters (strength 0) leaves the world's shadow off.
  Module.cod2SunShadow.params[0] = 0; Module.cod2SunShadow.revision++;
  drawScene(256, 1, 1); assert.equal(material.sunShadow[0], 0);
  Module.cod2SunShadow.params[0] = 1; Module.cod2SunShadow.revision++;
  // Programs with the map upload its sampler once and each value when it changes.
  const uploads = [];
  const shadowGL = {uniform1i: (location, value) => uploads.push(['1i', location, value]),
    uniform3fv: (location, value) => uploads.push(['3fv', location]),
    uniform4fv: (location, value) => uploads.push(['4fv', location, [...value]]),
    uniformMatrix4fv: (location, transpose, value) => uploads.push(['m4', location, transpose, [...value]])};
  const shadowProgram = materialProgram({cod2SunShadowEyeLocation: 'sunShadowEye', cod2SunShadowLocation: 'sunShadow',
    cod2SunShadowRectLocation: 'sunShadowRect', cod2SunShadowMapLocation: 'sunShadowMap'});
  drawScene(256, 1, 1, 0, 22, 23, 21, 8, 6, 1, 1.5, 0.65, 0);
  upload.call(shadowProgram, Module, shadowGL);
  assert.deepEqual(uploads.filter(([type]) => type === '1i').map(([, location, value]) => [location, value]),
    [['detailMap', 5], ['normalMap', 6], ['specMap', 7], ['sunShadowMap', 18]]);
  assert.deepEqual(uploads.find(([, location]) => location === 'sunShadowEye'), ['m4', 'sunShadowEye', false, [...material.sunShadowEye]]);
  assert.deepEqual(uploads.find(([, location]) => location === 'sunShadow'), ['4fv', 'sunShadow', [...material.sunShadow]]);
  assert.deepEqual(uploads.find(([, location]) => location === 'sunShadowRect'), ['4fv', 'sunShadowRect', [...material.sunShadowRect]]);
  uploads.length = 0;
  for (let i = 0; i < 1000; ++i) { drawScene(256, 1, 1, 0, 22, 23, 21, 8, 6, 1, 1.5, 0.65, 0); upload.call(shadowProgram, Module, shadowGL); }
  assert.deepEqual(uploads, [], '1000 world draws of one frame upload nothing more');
  drawScene(256, 1, 0, 0, 22, 23, 21, 8, 6, 1, 1.5, 0.65, 0); upload.call(shadowProgram, Module, shadowGL);
  assert.deepEqual(uploads.splice(0), [['4fv', 'sunShadow', [0, ...[...Module.cod2SunShadow.params].slice(1)]]],
    'a draw that takes no shadow uploads only its strength');
  // The world pass compares against the map only where the bake sees the sun
  // and inside the casters' rectangle; its four filtered lookups remove the
  // sun's share of the lightmap, with or without material lighting. Models
  // and the HUD's programs carry none of it.
  const world = generate.call({usedTexUnitList: [0, 1]}, {...twoD, useTextureMatrix: false}, {lightingEnabled: false},
    'vec4 c = texture2D(u_texUnit0, v_texCoord0.xy) * (texture2D(u_texUnit1, v_texCoord1.xy) * 2.0);\ngl_FragColor = c;',
    'varying vec4 v_texCoord0;\nvarying vec4 v_texCoord1;\n');
  assert(world.pointDefs.includes('uniform highp mat4 u_cod2SunShadowEye; uniform highp vec4 u_cod2SunShadow; uniform highp vec4 u_cod2SunShadowRect;\n' +
    'uniform mediump sampler2DShadow u_cod2SunShadowMap;\n'));
  assert(world.pointDefs.includes('  if (u_cod2SunShadow.x <= 0.0 || visible <= 0.004) return 1.0;\n'));
  assert(world.pointDefs.includes('  if (any(lessThan(p.xy, u_cod2SunShadowRect.xy)) || any(greaterThan(p.xy, u_cod2SunShadowRect.zw))) return 1.0;\n'));
  assert.equal(world.pointDefs.match(/textureLod\(u_cod2SunShadowMap/g).length, 4);
  const lit = world.pointPass;
  assert(lit.indexOf('vec3 cod2Indirect = max(cod2L.rgb - cod2SunPart, vec3(0.0));\ncod2SunPart *= cod2SunShadow(cod2L.a);\n') > 0,
    'the bake\'s full sun share comes off first, then only the shadowed share returns');
  assert(lit.indexOf('cod2SunShadow(cod2L.a)') < lit.indexOf('vec3 cod2Lit = '));
  assert(lit.includes('} else if (u_cod2SunShadow.x > 0.0) {\nvec4 cod2L = cod2LightmapTexel;\n' +
    'float cod2Shade = 1.0 - cod2SunShadow(cod2L.a);\n'));
  assert(!lit.includes('gl_FragColor.a'));
  const model = generate.call({usedTexUnitList: [0]}, {...twoD, useTextureMatrix: false},
    {lightingEnabled: true, MAX_LIGHTS: 8, lightEnabled: [true, false, false, false, false, false, false, false]},
    'gl_FragColor = texture2D(u_texUnit0, v_texCoord0.xy) * v_color;');
  assert(!/cod2SunShadow/.test(model.pointDefs + model.pointPass));
  assert(!/cod2SunShadow/.test(noSample.pointDefs + noSample.pointPass));
}
console.log('PASS: the sun\'s depth map in 3 views\' eye space for world draws only, cached per frame and view, uploaded per program and value, and compared only where the bake sees the sun');

// COD2_EFFECTS (web_effects.c): every textured program takes the draw's
// effect uniform; only unlit ones, the effects', feather into the scene's
// depth on unit 19 and burn hotter, after their light and before their fog.
{
  const make = (units, lighting) => generate.call({usedTexUnitList: units}, {...twoD, useTextureMatrix: false},
    {lightingEnabled: lighting, MAX_LIGHTS: 8, lightEnabled: [true, false, false, false, false, false, false, false]},
    units.includes(1) ? 'gl_FragColor = texture2D(u_texUnit0, v_texCoord0.xy) * (texture2D(u_texUnit1, v_texCoord1.xy) * 2.0);' :
      'gl_FragColor = texture2D(u_texUnit0, v_texCoord0.xy) * v_color;', 'varying vec4 v_texCoord0;\nvarying vec4 v_texCoord1;\n');
  const effect = make([0], false), world = make([0, 1], false), model = make([0], true), untextured = make([], false);
  for (const shader of [effect, world, model])
    assert(shader.pointDefs.includes('uniform highp vec4 u_cod2Effect;\n'), 'coverage reaches every textured program');
  assert.equal(untextured.cod2EffectPass, '');
  for (const shader of [world, model]) {
    assert.equal(shader.cod2EffectPass, '', 'the lit world and models neither feather nor burn');
    assert(!shader.pointDefs.includes('u_cod2FeatherDepth'));
  }
  assert(effect.pointDefs.includes('uniform highp sampler2D u_cod2FeatherDepth;\n'));
  assert.equal(effect.cod2EffectPass, 'if (u_cod2Effect.y != 0.0) {\n' +
    'highp float cod2Z = -v_cod2Eye.z, cod2Scale = abs(u_cod2Effect.y);\n' +
    'highp float cod2SceneZ = u_cod2Effect.w / max(0.9995002746582031 -\n' +
    '  texelFetch(u_cod2FeatherDepth, ivec2(gl_FragCoord.xy), 0).r, 0.000001);\n' +
    'float cod2Feather = clamp(abs(cod2SceneZ - cod2Z) * cod2Scale, 0.0, 1.0) *\n' +
    '  clamp((cod2Z - u_cod2Effect.w / 0.9995002746582031) * cod2Scale, 0.0, 1.0);\n' +
    'if (u_cod2Effect.y > 0.0) gl_FragColor.a *= cod2Feather; else gl_FragColor *= cod2Feather;\n}\n' +
    'if (u_cod2Effect.z > 0.0)\n' +
    '  gl_FragColor.rgb *= 1.0 + u_cod2Effect.z * dot(gl_FragColor.rgb, vec3(0.299, 0.587, 0.114));\n',
    'the DX9 zfeather: fade by the gap to the scene\'s depth and to the near clip, then the heat');
  assert(!effect.pointPass.includes('gl_FragColor.a'), 'the light keeps transparency; only the feather fades it');
  assert(linked.includes('this.cod2EffectLocation = GLctx.getUniformLocation(this.program, "u_cod2Effect");'));
  assert(linked.includes('this.cod2FeatherDepthLocation = GLctx.getUniformLocation(this.program, "u_cod2FeatherDepth");'));

  // COD2_ALPHA_COVERAGE: textured GREATER/GEQUAL tests turn into coverage
  // when the draw asks for it; every other test, and untextured draws, keep
  // the original.
  const coverageStart = linked.indexOf('          // COD2_ALPHA_COVERAGE:');
  const coverageEnd = linked.indexOf('          var fsSource = [ "precision mediump float;",', coverageStart);
  assert(coverageStart >= 0 && coverageEnd > coverageStart);
  const wrap = Function('pointLights', 'GLEmulation', 'fsAlphaTestPass', 'pointUV',
    linked.slice(coverageStart, coverageEnd) + '\nreturn fsAlphaTestPass;');
  const greater = 'if (!(gl_FragColor.a > u_alphaTestRef)) { discard; }', gequal = 'if (!(gl_FragColor.a >= u_alphaTestRef)) { discard; }';
  for (const [func, test] of [[516, greater], [518, gequal]]) {
    for (const uv of ['v_texCoord0.xy', '(u_textureMatrix0 * v_texCoord0).xy']) {
      const pass = wrap(true, {alphaTestFunc: func}, test, uv);
      assert(pass.startsWith('if (u_cod2Effect.x > 0.5) {\n'));
      assert(pass.includes(`  highp vec2 cod2Texels = ${uv} * vec2(textureSize(u_texUnit0, 0));\n`));
      assert(pass.includes('  float cod2Mip = max(0.5 * log2(max(max(dot(cod2Dx, cod2Dx), dot(cod2Dy, cod2Dy)), 1e-8)) + u_cod2Detail.w, 0.0);\n' +
        '  gl_FragColor.a *= 1.0 + 0.25 * cod2Mip;\n'), 'a quarter more alpha per mip level, after the scaled scene\'s bias');
      assert(pass.includes('  gl_FragColor.a = (gl_FragColor.a - u_alphaTestRef) / max(fwidth(gl_FragColor.a), 0.0001) + 0.5;\n' +
        '  if (gl_FragColor.a <= 0.0) discard;\n  gl_FragColor.a = min(gl_FragColor.a, 1.0);\n'));
      assert(pass.endsWith('} else {\n' + test + '\n}\n'), 'the original test without coverage');
    }
  }
  for (const [lit, func, test] of [[false, 518, gequal], [true, 515, 'if (!(gl_FragColor.a <= u_alphaTestRef)) { discard; }'],
                                   [true, 512, 'discard;'], [true, 519, '']])
    assert.equal(wrap(lit, {alphaTestFunc: func}, test, 'v_texCoord0.xy'), test);

  // The per-draw bridge: coverage only in the composed view's multisampled
  // scene, toggled when it changes; a feather only with the scene's depth
  // copied and the view's near clip, which goes along with it.
  const effectsSource = fs.readFileSync('downstream/wasm/web_effects.c', 'utf8');
  const bridgeStart = effectsSource.indexOf('EM_JS(void, WebEffects_Draw, (float coverage, float feather, float heat, float worldNearClip, const float *wind), {');
  assert(bridgeStart >= 0);
  let bodyStart = effectsSource.indexOf('{', effectsSource.indexOf('), {', bridgeStart) + 2), bodyEnd = bodyStart + 1, depth = 1;
  while (depth) { depth += (effectsSource[bodyEnd] === '{') - (effectsSource[bodyEnd] === '}'); ++bodyEnd; }
  const capabilities = [];
  const effectGL = {SAMPLE_ALPHA_TO_COVERAGE: 0x809E, enable: c => capabilities.push(['enable', c]),
    disable: c => capabilities.push(['disable', c])};
  const M = {}, windHeap = new Float32Array(64);
  const bridge = Function('Module', 'GLctx', 'HEAPF32', 'coverage', 'feather', 'heat', 'worldNearClip', 'wind',
    effectsSource.slice(bodyStart + 1, bodyEnd - 1)).bind(null, M, effectGL, windHeap);
  const f = Math.fround;
  bridge(1, 0.25, 0.65, 4);
  assert.deepEqual(capabilities, [], 'nothing before the first material');
  const effectValues = M.cod2Material = {effect: new Float32Array(4)};
  let copies = 0, copied = true;
  M.cod2SceneFx = {t: {}, drawing: true, samples: 4, featherDepth: () => (++copies, copied)};
  bridge(1, 0, 0, f(3.998));
  assert.deepEqual([...effectValues.effect], [1, 0, 0, 0]);
  assert.deepEqual(capabilities.splice(0), [['enable', 0x809E]]);
  for (let i = 0; i < 1000; ++i) bridge(1, 0, 0, f(3.998));
  assert.deepEqual(capabilities, [], '1000 draws of leaves toggle nothing more');
  bridge(0, 0.25, f(0.65), f(3.998));
  assert.deepEqual(capabilities.splice(0), [['disable', 0x809E]]);
  assert.deepEqual([...effectValues.effect], [0, 0.25, f(0.65), f(3.998)]);
  assert.equal(copies, 1);
  bridge(0, f(-0.025), 0, f(3.998));
  assert.deepEqual([...effectValues.effect], [0, f(-0.025), 0, f(3.998)], 'a blend without alpha fades the color');
  copied = false;
  bridge(0, 0.25, f(0.65), f(3.998));
  assert.deepEqual([...effectValues.effect], [0, 0, f(0.65), 0], 'no depth copy, no feather; the heat stays');
  copied = true;
  copies = 0;
  bridge(0, 0.25, 0, 0);
  assert.deepEqual([...effectValues.effect], [0, 0, 0, 0]);
  assert.equal(copies, 0, 'no near clip, no copy');
  M.cod2SceneFx.drawing = false;
  bridge(1, 0.25, 0, f(3.998));
  assert.deepEqual([...effectValues.effect], [0, 0, 0, 0]);
  assert.equal(copies, 0, 'outside the composed view nothing is copied');
  M.cod2SceneFx.drawing = true;
  M.cod2SceneFx.samples = 1;
  bridge(1, 0, 0, f(3.998));
  assert.equal(effectValues.effect[0], 0, 'a single-sampled scene keeps the alpha test');
  delete M.cod2SceneFx;
  bridge(1, 0.25, f(0.65), f(3.998));
  assert.deepEqual([...effectValues.effect], [0, 0, f(0.65), 0]);
  assert.deepEqual(capabilities, []);
  // A swaying model's push and where it stands land in the material's own
  // arrays, read from the draw's eight floats; a still draw's zeros too.
  const wind = effectValues.wind = new Float32Array(4), windAt = effectValues.windAt = new Float32Array(4);
  windHeap.set([1.5, -2, 0.25, 1, 2800, 400, 1.5, 123.5], 8);
  bridge(1, 0, 0, f(3.998), 8 * 4);
  assert.deepEqual([...wind], [1.5, -2, 0.25, 1]);
  assert.deepEqual([...windAt], [2800, 400, 1.5, 123.5]);
  assert(effectValues.wind === wind && effectValues.windAt === windAt, 'the arrays are kept, not replaced');
  bridge(1, 0, 0, f(3.998), 32 * 4);
  assert.deepEqual([...wind, ...windAt], [0, 0, 0, 0, 0, 0, 0, 0]);

  // Programs with the effects upload the depth's unit once and their values
  // when they change.
  const uploads = [];
  const effectUploadGL = {uniform1i: (location, value) => uploads.push(['1i', location, value]),
    uniform3fv: location => uploads.push(['3fv', location]), uniform4fv: (location, value) => uploads.push(['4fv', location, [...value]]),
    uniformMatrix4fv: () => {}};
  const effectProgram = materialProgram({cod2EffectLocation: 'effect', cod2FeatherDepthLocation: 'featherDepth'});
  drawScene(256, 1, 1, 0, 22, 23, 21, 8, 6, 1, 1.5, 0.65, 0);
  Module.cod2Material.effect.set([0, 0.25, 0.65, 3.998]);
  upload.call(effectProgram, Module, effectUploadGL);
  assert(uploads.some(([type, location, value]) => type === '1i' && location === 'featherDepth' && value === 19));
  assert.deepEqual(uploads.find(([, location]) => location === 'effect'), ['4fv', 'effect', [...Module.cod2Material.effect]]);
  uploads.length = 0;
  for (let i = 0; i < 1000; ++i) upload.call(effectProgram, Module, effectUploadGL);
  assert.deepEqual(uploads, []);
  Module.cod2Material.effect[1] = 0;
  upload.call(effectProgram, Module, effectUploadGL);
  assert.deepEqual(uploads.splice(0), [['4fv', 'effect', [0, 0, f(0.65), f(3.998)]]]);
  Module.cod2Material.effect.fill(0);
}
console.log('PASS: the effect uniform in every textured program, the zfeather and heat in unlit ones only, coverage wrapping GREATER/GEQUAL tests with the mip-level alpha, toggled once per change in the multisampled scene, feathering only with the depth copy and the near clip, uploads per program and value');
