// Exercise the sun shadow map's GL side (downstream/wasm/web_sun_shadows.c):
// its depth pass draws the casters into a comparison texture kept on unit 18
// and leaves every binding and capability of the GL emulation as it found it.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const source = fs.readFileSync('downstream/wasm/web_sun_shadows.c', 'utf8');
function emjs(ret, name, args) {
  const start = source.indexOf(`EM_JS(${ret}, ${name},`);
  assert(start >= 0, name);
  let pos = source.indexOf('{', start), end = pos + 1, depth = 1;
  while (depth) { depth += (source[end] === '{') - (source[end] === '}'); ++end; }
  return Function('Module', 'HEAPF32', 'HEAPU32', 'GLctx', 'WebGL2RenderingContext', 'console', ...args,
    source.slice(pos + 1, end - 1));
}
const E = {VERTEX_SHADER: 1, FRAGMENT_SHADER: 2, LINK_STATUS: 3, VERTEX_ARRAY_BINDING: 4, ARRAY_BUFFER_BINDING: 5,
  ACTIVE_TEXTURE: 6, DRAW_FRAMEBUFFER_BINDING: 7, READ_FRAMEBUFFER_BINDING: 8, CURRENT_PROGRAM: 9, VIEWPORT: 10,
  ARRAY_BUFFER: 11, ELEMENT_ARRAY_BUFFER: 12, FLOAT: 13, TEXTURE0: 0x84C0, TEXTURE18: 0x84C0 + 18, TEXTURE_2D: 14,
  DEPTH_COMPONENT16: 15, TEXTURE_MIN_FILTER: 16, TEXTURE_MAG_FILTER: 17, LINEAR: 18, TEXTURE_WRAP_S: 19,
  TEXTURE_WRAP_T: 20, CLAMP_TO_EDGE: 21, TEXTURE_COMPARE_MODE: 22, COMPARE_REF_TO_TEXTURE: 23,
  TEXTURE_COMPARE_FUNC: 24, LEQUAL: 25, FRAMEBUFFER: 26, DRAW_FRAMEBUFFER: 27, READ_FRAMEBUFFER: 28,
  DEPTH_ATTACHMENT: 29, NONE: 0, FRAMEBUFFER_COMPLETE: 30, BLEND: 31, CULL_FACE: 32, SCISSOR_TEST: 33,
  STENCIL_TEST: 34, POLYGON_OFFSET_FILL: 35, DEPTH_TEST: 36, DEPTH: 37, LESS: 38, TRIANGLES: 39, UNSIGNED_INT: 40,
  STREAM_DRAW: 41};
const heap = new ArrayBuffer(4096), HEAPF32 = new Float32Array(heap), HEAPU32 = new Uint32Array(heap);
let calls, state, linkOk, complete, objects = 0;
const object = kind => ({kind, id: ++objects});
function reset() {
  calls = [];
  state = {program: 'engine program', vao: 'engine vao', arrayBuffer: 'engine vbo', unit: E.TEXTURE0 + 1,
    draw: 'scene fbo', read: 'scene fbo', viewport: [0, 0, 1280, 720], units: {}, mask: [1, 1, 1, 1],
    enabled: new Set([E.BLEND, E.DEPTH_TEST, E.SCISSOR_TEST]), vaoState: new Map()};
}
const gl = {...E,
  createShader: type => object(type === E.VERTEX_SHADER ? 'vertex' : 'fragment'),
  shaderSource: (shader, text) => { shader.text = text; },
  compileShader() {}, createProgram: () => object('program'), attachShader: (p, s) => { (p.shaders ||= []).push(s); },
  linkProgram: p => calls.push(['link', p.shaders.map(s => s.text)]),
  getProgramParameter: () => linkOk, getProgramInfoLog: () => 'program log', getShaderInfoLog: () => 'shader log',
  getUniformLocation: (p, name) => ({program: p, name}),
  createVertexArray: () => object('vao'), createBuffer: () => object('buffer'),
  createTexture: () => object('texture'), createFramebuffer: () => object('framebuffer'),
  deleteTexture: t => calls.push(['deleteTexture', t.id]), deleteFramebuffer: f => calls.push(['deleteFramebuffer', f.id]),
  getParameter(name) {
    return {[E.VERTEX_ARRAY_BINDING]: state.vao, [E.ARRAY_BUFFER_BINDING]: state.arrayBuffer, [E.ACTIVE_TEXTURE]: state.unit,
      [E.DRAW_FRAMEBUFFER_BINDING]: state.draw, [E.READ_FRAMEBUFFER_BINDING]: state.read,
      [E.CURRENT_PROGRAM]: state.program, [E.VIEWPORT]: Int32Array.from(state.viewport)}[name];
  },
  bindVertexArray: vao => { state.vao = vao; calls.push(['bindVertexArray', vao.kind ? 'pass vao' : vao]); },
  bindBuffer(target, buffer) {
    if (target === E.ARRAY_BUFFER) state.arrayBuffer = buffer;
    else state.vaoState.set(state.vao, buffer);
    calls.push(['bindBuffer', target, buffer.kind ? 'pass ' + buffer.id : buffer]);
  },
  enableVertexAttribArray: index => calls.push(['enableVertexAttribArray', index]),
  vertexAttribPointer: (...args) => calls.push(['vertexAttribPointer', ...args]),
  activeTexture: unit => { state.unit = unit; calls.push(['activeTexture', unit - E.TEXTURE0]); },
  bindTexture: (target, texture) => { state.units[state.unit] = texture; calls.push(['bindTexture', target, texture.id]); },
  texStorage2D: (...args) => calls.push(['texStorage2D', ...args]),
  texParameteri: (...args) => calls.push(['texParameteri', ...args]),
  bindFramebuffer(target, fbo) {
    if (target !== E.READ_FRAMEBUFFER) state.draw = fbo;
    if (target !== E.DRAW_FRAMEBUFFER) state.read = fbo;
    calls.push(['bindFramebuffer', target, fbo.kind ? 'shadow fbo' : fbo]);
  },
  framebufferTexture2D: (...args) => calls.push(['framebufferTexture2D', args[0], args[1], args[2], args[3].id, args[4]]),
  drawBuffers: buffers => calls.push(['drawBuffers', buffers]), readBuffer: buffer => calls.push(['readBuffer', buffer]),
  checkFramebufferStatus: () => complete ? E.FRAMEBUFFER_COMPLETE : 0,
  isEnabled: cap => state.enabled.has(cap),
  enable: cap => { state.enabled.add(cap); calls.push(['enable', cap]); },
  disable: cap => { state.enabled.delete(cap); calls.push(['disable', cap]); },
  depthMask: value => calls.push(['depthMask', value]), depthFunc: value => calls.push(['depthFunc', value]),
  colorMask: (...mask) => { state.mask = mask.map(Number); calls.push(['colorMask', ...mask]); },
  clearBufferfv: (buffer, index, value) => calls.push(['clearBufferfv', buffer, index, [...value]]),
  useProgram: program => { state.program = program; calls.push(['useProgram', program.kind ? 'pass program' : program]); },
  uniformMatrix4fv: (location, transpose, value) => calls.push(['uniformMatrix4fv', location.name, transpose, [...value]]),
  bufferData: (target, data, usage) => calls.push(['bufferData', target, data.constructor.name, [...data], usage]),
  drawElements: (...args) => calls.push(['drawElements', ...args])};
// The engine's viewport can be scaled by the scene compositor: the pass
// calls the context's own.
const WebGL2RenderingContext = {prototype: {viewport(x, y, w, h) { assert.equal(this, gl); state.viewport = [x, y, w, h];
  calls.push(['viewport', x, y, w, h]); }}};
gl.viewport = () => assert.fail('the scaled viewport must not be called');
const warnings = [], fakeConsole = {warn: text => warnings.push(text)};
const drawJs = emjs('int', 'WebSunShadows_Draw', ['positions', 'vertexCount', 'indices', 'indexCount', 'clip', 'size']);
const receiversJs = emjs('void', 'WebSunShadows_SetReceivers', ['texture', 'params']);
const Module = {};
const draw = (...args) => drawJs(Module, HEAPF32, HEAPU32, gl, WebGL2RenderingContext, fakeConsole, ...args);
// Two triangles: positions at byte 256, indices at byte 512, clip at byte 1024.
HEAPF32.set([1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12], 64);
HEAPU32.set([0, 1, 2, 0, 2, 3], 128);
const clip = Array.from({length: 16}, (_, i) => i * 0.5 - 3);
HEAPF32.set(clip, 256);
const engineState = () => ({program: state.program, vao: state.vao, arrayBuffer: state.arrayBuffer, unit: state.unit,
  draw: state.draw, read: state.read, viewport: [...state.viewport], enabled: [...state.enabled].sort()});

// The first map: its program, vertex array and texture, then the casters.
reset(); linkOk = true; complete = true;
const before = engineState();
assert.equal(draw(256, 4, 512, 6, 1024, 2048), 1);
assert.deepEqual(engineState(), before, 'every binding, the viewport and the capabilities return');
assert.deepEqual(state.mask, [1, 1, 1, 1]);
const pass = Module.cod2SunShadowPass;
assert.equal(pass.size, 2048);
assert.equal(state.units[E.TEXTURE18], pass.texture, 'the map stays bound on unit 18');
const at = name => calls.findIndex(call => call[0] === name);
const link = calls.find(call => call[0] === 'link')[1];
assert(link[0].includes('layout(location = 0) in vec3 a_position;') && link[0].includes('gl_Position = u_clip * vec4(a_position, 1.0);'));
assert(link[1].includes('void main() {}'), 'depth only');
assert.deepEqual(calls.slice(at('bindVertexArray'), at('bindVertexArray') + 7), [
  ['bindVertexArray', 'pass vao'], ['bindBuffer', E.ARRAY_BUFFER, 'pass ' + pass.positions.id], ['enableVertexAttribArray', 0],
  ['vertexAttribPointer', 0, 3, E.FLOAT, false, 12, 0], ['bindBuffer', E.ELEMENT_ARRAY_BUFFER, 'pass ' + pass.indices.id],
  ['bindVertexArray', 'engine vao'], ['bindBuffer', E.ARRAY_BUFFER, 'engine vbo']], 'the pass owns its buffers');
assert.deepEqual(calls.filter(call => ['texStorage2D', 'texParameteri'].includes(call[0])), [
  ['texStorage2D', E.TEXTURE_2D, 1, E.DEPTH_COMPONENT16, 2048, 2048],
  ['texParameteri', E.TEXTURE_2D, E.TEXTURE_MIN_FILTER, E.LINEAR], ['texParameteri', E.TEXTURE_2D, E.TEXTURE_MAG_FILTER, E.LINEAR],
  ['texParameteri', E.TEXTURE_2D, E.TEXTURE_WRAP_S, E.CLAMP_TO_EDGE], ['texParameteri', E.TEXTURE_2D, E.TEXTURE_WRAP_T, E.CLAMP_TO_EDGE],
  ['texParameteri', E.TEXTURE_2D, E.TEXTURE_COMPARE_MODE, E.COMPARE_REF_TO_TEXTURE],
  ['texParameteri', E.TEXTURE_2D, E.TEXTURE_COMPARE_FUNC, E.LEQUAL]], 'a filtered comparison map');
assert.deepEqual(calls.filter(call => ['framebufferTexture2D', 'drawBuffers', 'readBuffer'].includes(call[0])), [
  ['framebufferTexture2D', E.FRAMEBUFFER, E.DEPTH_ATTACHMENT, E.TEXTURE_2D, pass.texture.id, 0],
  ['drawBuffers', [E.NONE]], ['readBuffer', E.NONE]]);
const viewportAt = calls.findIndex(call => call[0] === 'viewport' && call[3] === 2048);
const drawCalls = calls.slice(viewportAt - 1);
assert.deepEqual(drawCalls.slice(0, 6), [['bindFramebuffer', E.FRAMEBUFFER, 'shadow fbo'], ['viewport', 0, 0, 2048, 2048],
  ['depthMask', true], ['clearBufferfv', E.DEPTH, 0, [1]], ['enable', E.DEPTH_TEST], ['depthFunc', E.LESS]]);
assert.deepEqual(drawCalls.find(call => call[0] === 'uniformMatrix4fv'), ['uniformMatrix4fv', 'u_clip', false, clip]);
assert.deepEqual(drawCalls.filter(call => call[0] === 'bufferData'), [
  ['bufferData', E.ARRAY_BUFFER, 'Float32Array', [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12], E.STREAM_DRAW],
  ['bufferData', E.ELEMENT_ARRAY_BUFFER, 'Uint32Array', [0, 1, 2, 0, 2, 3], E.STREAM_DRAW]]);
assert.deepEqual(drawCalls.find(call => call[0] === 'drawElements'), ['drawElements', E.TRIANGLES, 6, E.UNSIGNED_INT, 0]);
const drawAt = calls.findIndex(call => call[0] === 'drawElements');
for (const cap of [E.BLEND, E.CULL_FACE, E.SCISSOR_TEST, E.STENCIL_TEST, E.POLYGON_OFFSET_FILL]) {
  const off = calls.findIndex(call => call[0] === 'disable' && call[1] === cap);
  assert(off >= 0 && off < drawAt, 'casters draw without blending, culling, scissor, stencil or offset');
}
assert(calls.findIndex(call => call[0] === 'colorMask' && !call[1]) < drawAt);
// The element buffer went to the pass's own vertex array, not the engine's.
assert.equal(state.vaoState.get(state.vao), undefined);
assert(warnings.length === 0);

// Later frames reuse everything; a frame without casters only clears.
reset();
assert.equal(draw(256, 4, 512, 6, 1024, 2048), 1);
assert(!calls.some(call => ['link', 'texStorage2D', 'deleteTexture'].includes(call[0])));
reset();
assert.equal(draw(256, 0, 512, 0, 1024, 2048), 0);
assert(calls.some(call => call[0] === 'clearBufferfv') && !calls.some(call => call[0] === 'drawElements'));
assert.deepEqual(engineState(), before);
// A new size replaces the map.
reset();
const oldTexture = pass.texture.id;
assert.equal(draw(256, 4, 512, 6, 1024, 1024), 1);
assert(calls.some(call => call[0] === 'deleteTexture' && call[1] === oldTexture));
assert.deepEqual(calls.find(call => call[0] === 'texStorage2D'), ['texStorage2D', E.TEXTURE_2D, 1, E.DEPTH_COMPONENT16, 1024, 1024]);
assert.deepEqual(engineState(), before);

// A map the GPU cannot render to, or a caster program that fails to link,
// draws nothing again and leaves the engine's state alone.
delete Module.cod2SunShadowPass;
reset(); complete = false;
assert.equal(draw(256, 4, 512, 6, 1024, 2048), 0);
assert.match(warnings.pop(), /depth map unavailable/);
assert.deepEqual(engineState(), before);
reset();
assert.equal(draw(256, 4, 512, 6, 1024, 2048), 0);
assert.deepEqual(calls, []);
delete Module.cod2SunShadowPass;
reset(); complete = true; linkOk = false;
assert.equal(draw(256, 4, 512, 6, 1024, 2048), 0);
assert.match(warnings.pop(), /caster program failed/);
reset();
assert.equal(draw(256, 4, 512, 6, 1024, 2048), 0);
assert.deepEqual(calls, []);
// No context, no map.
assert.equal(drawJs(Module, HEAPF32, HEAPU32, undefined, WebGL2RenderingContext, fakeConsole, 256, 4, 512, 6, 1024, 2048), 0);

// The world's shaders take the frame's matrix, parameters and rectangle.
const receivers = (...args) => receiversJs(Module, HEAPF32, HEAPU32, gl, WebGL2RenderingContext, fakeConsole, ...args);
const texture = Array.from({length: 16}, (_, i) => (i + 1) / 64), params = [1, 1 / 2048, 8, 0.0002, 0.25, 0.3, 0.4, 0.45];
HEAPF32.set(texture, 320); HEAPF32.set(params, 352);
receivers(1280, 1408);
const shadow = Module.cod2SunShadow, arrays = [shadow.world, shadow.params, shadow.rect];
assert.deepEqual([...shadow.world], texture.map(Math.fround));
assert.deepEqual([...shadow.params], params.slice(0, 4).map(Math.fround));
assert.deepEqual([...shadow.rect], params.slice(4).map(Math.fround));
assert.equal(shadow.revision, 1);
HEAPF32[352] = 0;
receivers(1280, 1408);
assert.equal(shadow.params[0], 0); assert.equal(shadow.revision, 2);
assert.deepEqual([shadow.world, shadow.params, shadow.rect], arrays, 'no allocation per frame');
console.log('PASS: the sun shadow pass builds its depth-only program, vertex array and filtered comparison map on unit 18, ' +
  'draws the casters with 32-bit indices from the heap, clears without casters, replaces the map on a new size, ' +
  'stops for good without a map or program, and returns every binding, the viewport and the capabilities; the ' +
  'world shaders\' matrix, parameters and rectangle update in place each frame');
