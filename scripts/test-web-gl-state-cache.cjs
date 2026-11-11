const assert = require('node:assert/strict');
const fs = require('node:fs');
const source = fs.readFileSync(require('node:path').join(__dirname, '../site/native-game-adapter.js'), 'utf8');
const start = source.indexOf('  function cacheWebGlState(');
const end = source.indexOf('  function performanceMeter(', start);
assert(start >= 0 && end > start);
const cache = Function(source.slice(start, end) + 'return cacheWebGlState;')();
const calls = [];
const gl = {TEXTURE0: 100, DEPTH_TEST: 0xB71, BLEND: 0xBE2};
for (const name of ['activeTexture','bindTexture','texParameteri','texParameterf','deleteTexture','uniformMatrix4fv','uniform4fv','uniform4f','uniform1i','uniform1f','useProgram','blendFunc','linkProgram','enable','disable','depthFunc','cullFace','frontFace','depthMask','colorMask'])
  gl[name] = (...args) => calls.push([name, ...args]);
gl.getUniformLocation = (program,name) => {calls.push(['getUniformLocation',program,name]);return {};};
let restore;
cache(gl, {addEventListener(name, callback) { assert.equal(name, 'webglcontextrestored'); restore = callback; }});
const a = {}, b = {}, matrixLocation = {}, secondLocation = {};
const count = name => calls.filter(value => value[0] === name).length;
const matrix = new Float32Array(16); matrix[0] = matrix[5] = matrix[10] = matrix[15] = 1;
for (let i = 0; i < 600; i++) {
  gl.activeTexture(100); gl.bindTexture(3553, a);
  gl.texParameteri(3553, 10241, 9987); gl.texParameterf(3553, 34046, 4);
  gl.uniformMatrix4fv(matrixLocation, false, matrix);
}
assert.equal(count('bindTexture'), 1); assert.equal(count('texParameteri'), 1);
assert.equal(count('texParameterf'), 1); assert.equal(count('uniformMatrix4fv'), 1);
// Sampler state belongs to the texture, including when shared between units.
gl.activeTexture(101); gl.bindTexture(3553, a); gl.texParameteri(3553, 10241, 9728);
gl.activeTexture(100); gl.texParameteri(3553, 10241, 9987);
assert.equal(count('texParameteri'), 3);
gl.bindTexture(3553, b); gl.texParameteri(3553, 10241, 9987);
assert.equal(count('texParameteri'), 4);
// A matrix can change in place. Uniform locations remain independent.
matrix[12] = 123; gl.uniformMatrix4fv(matrixLocation, false, matrix);
gl.uniformMatrix4fv(secondLocation, false, matrix);
assert.equal(count('uniformMatrix4fv'), 3);
gl.uniformMatrix4fv(matrixLocation, true, matrix);
gl.uniformMatrix4fv(matrixLocation, false, matrix, 0, 16);
assert.equal(count('uniformMatrix4fv'), 5);
gl.uniformMatrix4fv(matrixLocation, false, matrix);
assert.equal(count('uniformMatrix4fv'), 6); // Ranged uploads invalidate prior values.
// Calls without a texture/uniform and invalid/ranged calls retain GL behavior.
gl.bindTexture(3553, null); gl.texParameteri(3553, 10241, 9987);
gl.uniformMatrix4fv(null, false, matrix);
assert.equal(count('texParameteri'), 5); assert.equal(count('uniformMatrix4fv'), 7);
gl.bindTexture(3553, a); gl.deleteTexture(a);
gl.texParameteri(3553, 10241, 9987); assert.equal(count('texParameteri'), 6);
restore(); gl.activeTexture(101); gl.bindTexture(3553, b);
gl.texParameteri(3553, 10241, 9987); gl.uniformMatrix4fv(secondLocation, false, matrix);
assert.equal(count('texParameteri'), 7); assert.equal(count('uniformMatrix4fv'), 8);
console.log('PASS: 600 repeated draws reuse samplers/matrices; texture-unit changes, in-place camera motion, deletion and context restoration invalidate correctly');
// Identical vectors can be skipped, but in-place edits and relinking cannot.
const vectorLocation = {}, vector = new Float32Array([1, .5, 0, 1]);
for(let i=0;i<600;i++) gl.uniform4fv(vectorLocation, vector);
assert.equal(count('uniform4fv'), 1);
vector[1]=.25;gl.uniform4fv(vectorLocation,vector);
assert.equal(count('uniform4fv'), 2);
gl.uniform4fv(vectorLocation,vector,0,4);gl.uniform4fv(vectorLocation,vector);
assert.equal(count('uniform4fv'), 4);
gl.linkProgram({});gl.uniform4fv(vectorLocation,vector);gl.uniformMatrix4fv(secondLocation,false,matrix);
assert.equal(count('uniform4fv'), 5);assert.equal(count('uniformMatrix4fv'), 9);
restore();gl.uniform4fv(vectorLocation,vector);assert.equal(count('uniform4fv'),6);
gl.uniform4fv(null,vector);assert.equal(count('uniform4fv'),7);
console.log('PASS: repeated vector uniforms skip driver uploads; edits, ranged calls, relinking and context restoration upload fresh values');
gl.uniform4f(vectorLocation,0,0,0,0);gl.uniform4fv(vectorLocation,vector);assert.equal(count('uniform4fv'),8);
for(let i=0;i<600;i++) {
 gl.enable(gl.DEPTH_TEST);gl.disable(gl.BLEND);gl.depthFunc(0x203);
 gl.cullFace(0x405);gl.frontFace(0x901);gl.depthMask(true);gl.colorMask(true,true,true,true);
}
for(const method of ['enable','disable','depthFunc','cullFace','frontFace','depthMask','colorMask'])assert.equal(count(method),1,method);
gl.disable(gl.DEPTH_TEST);gl.enable(gl.DEPTH_TEST);assert.equal(count('enable'),2);
gl.depthMask(false);gl.depthMask(true);assert.equal(count('depthMask'),3);
gl.colorMask(true,false,true,false);gl.colorMask(true,true,true,true);assert.equal(count('colorMask'),3);
// Unsupported capabilities/enums still reach GL, retaining validation/error behavior.
gl.enable(0xbad);gl.enable(0xbad);assert.equal(count('enable'),4);
gl.depthFunc(0xbad);gl.depthFunc(0xbad);assert.equal(count('depthFunc'),3);
restore();gl.depthMask(true);gl.enable(gl.DEPTH_TEST);assert.equal(count('depthMask'),4);assert.equal(count('enable'),5);
console.log('PASS: repeated depth/cull/color/enable states avoid driver calls; all transitions, invalid enums and restoration retain behavior');
gl.depthMask(0);gl.depthMask(true);assert.equal(count('depthMask'),6);
const program = {}, first = gl.getUniformLocation(program,'tint');
assert.equal(gl.getUniformLocation(program,'tint'),first);
assert.equal(gl.getUniformLocation(program,'tint[0]'),first);
gl.uniform4fv(first,vector);gl.uniform4f(gl.getUniformLocation(program,'tint'),0,0,0,0);gl.uniform4fv(first,vector);
assert.equal(count('getUniformLocation'),1);
gl.linkProgram(program);assert.notEqual(gl.getUniformLocation(program,'tint'),first);
assert.equal(count('getUniformLocation'),2);
// Updating the existing light arrays must leave unrelated material caches hot.
const lightLocation = {}, materialLocation = {}, lightArray = new Float32Array(16);
const materialColor = new Float32Array([1, .75, .5, 1]);
const arrayCalls = count('uniform4fv');
for (let i=0;i<600;i++) {
  lightArray[0] = i;
  gl.uniform4fv(lightLocation,lightArray);
  gl.uniform4fv(materialLocation,materialColor);
}
assert.equal(count('uniform4fv')-arrayCalls,601);
// Scalar and vector forms refer to the same four float32 values on the GPU.
const scalarLocation = {}, scalarCalls = count('uniform4f');
for(let i=0;i<600;i++)gl.uniform4f(scalarLocation,1/3,.25,.5,1);
assert.equal(count('uniform4f')-scalarCalls,1);
const scalarVectorCalls = count('uniform4fv');
gl.uniform4fv(scalarLocation,new Float32Array([1/3,.25,.5,1]));
assert.equal(count('uniform4fv'),scalarVectorCalls);
gl.uniform4f(scalarLocation,1/3,.5,.5,1);assert.equal(count('uniform4f')-scalarCalls,2);
gl.uniform4fv(scalarLocation,new Float32Array([1/3,.25,.5,1]));assert.equal(count('uniform4fv'),scalarVectorCalls+1);
gl.uniform4fv(lightLocation,new Float32Array(4));gl.uniform4fv(lightLocation,lightArray);
assert.equal(count('uniform4fv'),scalarVectorCalls+3);
// One texture shared between units still has one mutable sampler state.
const shared = {}, samplerCalls = count('texParameteri');
gl.activeTexture(100);gl.bindTexture(3553,shared);gl.texParameteri(3553,10241,9987);
gl.activeTexture(101);gl.bindTexture(3553,shared);gl.texParameteri(3553,10241,9728);
gl.activeTexture(100);gl.texParameteri(3553,10241,9987);
assert.equal(count('texParameteri')-samplerCalls,3);
gl.deleteTexture(shared);gl.activeTexture(101);gl.texParameteri(3553,10241,9987);
assert.equal(count('texParameteri')-samplerCalls,4);
const progA = {}, progB = {}, uIntLoc = {}, uFloatLoc = {};
for (let i = 0; i < 600; i++) {
  gl.useProgram(progA);
  gl.blendFunc(0x0302, 0x0303);
  gl.uniform1i(uIntLoc, 2);
  gl.uniform1f(uFloatLoc, 0.75);
}
assert.equal(count('useProgram'), 1);
assert.equal(count('blendFunc'), 1);
assert.equal(count('uniform1i'), 1);
assert.equal(count('uniform1f'), 1);
gl.useProgram(progB); assert.equal(count('useProgram'), 2);
gl.blendFunc(1, 1); assert.equal(count('blendFunc'), 2);
gl.uniform1i(uIntLoc, 3); assert.equal(count('uniform1i'), 2);
gl.uniform1f(uFloatLoc, 0.25); assert.equal(count('uniform1f'), 2);
gl.linkProgram(progB);
gl.useProgram(progB); assert.equal(count('useProgram'), 3);
gl.uniform1i(uIntLoc, 3); assert.equal(count('uniform1i'), 3);
gl.uniform1f(uFloatLoc, 0.25); assert.equal(count('uniform1f'), 3);
console.log('PASS: 600 changing muzzle-light arrays preserve material caches; scalar/vector float32 equivalence, blend/program/scalar caches and shared samplers retain values');

// Compositor and shadow passes save and restore bindings/capabilities without synchronous GPU readbacks.
{
  const qCalls = [];
  let restore2;
  const gl2 = {
    TEXTURE0: 0x84C0, ACTIVE_TEXTURE: 0x84E0, CURRENT_PROGRAM: 0x8B8D,
    VERTEX_ARRAY_BINDING: 0x85B5, ARRAY_BUFFER: 0x8892, ARRAY_BUFFER_BINDING: 0x8894,
    FRAMEBUFFER: 0x8D40, DRAW_FRAMEBUFFER: 0x8CA9, READ_FRAMEBUFFER: 0x8CA8,
    DRAW_FRAMEBUFFER_BINDING: 0x8CA6, READ_FRAMEBUFFER_BINDING: 0x8CAA,
    STENCIL_TEST: 0x0B90, SCISSOR_TEST: 0x0C11, VIEWPORT: 0x0BA2
  };
  for (const name of ['activeTexture','useProgram','enable','disable','bindVertexArray','deleteVertexArray','bindBuffer','deleteBuffer','bindFramebuffer','deleteFramebuffer'])
    gl2[name] = (...args) => qCalls.push([name, ...args]);
  gl2.isEnabled = cap => { qCalls.push(['isEnabled', cap]); return false; };
  gl2.getParameter = p => { qCalls.push(['getParameter', p]); return p === 0x0BA2 ? [0,0,1920,1080] : null; };
  cache(gl2, {addEventListener(_, cb) { restore2 = cb; }});
  const vao = {}, vbo = {}, fboA = {}, fboB = {}, prog = {};
  gl2.useProgram(prog); gl2.activeTexture(0x84C2); gl2.bindVertexArray(vao);
  gl2.bindBuffer(gl2.ARRAY_BUFFER, vbo); gl2.bindFramebuffer(gl2.FRAMEBUFFER, fboA);
  gl2.enable(gl2.STENCIL_TEST);
  assert.equal(gl2.isEnabled(gl2.SCISSOR_TEST), false); // 1st readback populates cache
  const beforeReads = qCalls.filter(c => c[0] === 'getParameter' || c[0] === 'isEnabled').length;
  for (let i = 0; i < 600; i++) {
    assert.equal(gl2.getParameter(gl2.ACTIVE_TEXTURE), 0x84C2);
    assert.equal(gl2.getParameter(gl2.CURRENT_PROGRAM), prog);
    assert.equal(gl2.getParameter(gl2.VERTEX_ARRAY_BINDING), vao);
    assert.equal(gl2.getParameter(gl2.ARRAY_BUFFER_BINDING), vbo);
    assert.equal(gl2.getParameter(gl2.DRAW_FRAMEBUFFER_BINDING), fboA);
    assert.equal(gl2.getParameter(gl2.READ_FRAMEBUFFER_BINDING), fboA);
    assert.equal(gl2.isEnabled(gl2.STENCIL_TEST), true);
    assert.equal(gl2.isEnabled(gl2.SCISSOR_TEST), false);
  }
  assert.equal(qCalls.filter(c => c[0] === 'getParameter' || c[0] === 'isEnabled').length, beforeReads);
  gl2.bindFramebuffer(gl2.READ_FRAMEBUFFER, fboB);
  assert.equal(gl2.getParameter(gl2.DRAW_FRAMEBUFFER_BINDING), fboA);
  assert.equal(gl2.getParameter(gl2.READ_FRAMEBUFFER_BINDING), fboB);
  gl2.deleteVertexArray(vao); gl2.deleteBuffer(vbo); gl2.deleteFramebuffer(fboA);
  assert.equal(gl2.getParameter(gl2.VERTEX_ARRAY_BINDING), null);
  assert.equal(gl2.getParameter(gl2.ARRAY_BUFFER_BINDING), null);
  assert.equal(gl2.getParameter(gl2.DRAW_FRAMEBUFFER_BINDING), null);
  assert.deepEqual(gl2.getParameter(gl2.VIEWPORT), [0,0,1920,1080]);
  restore2();
  gl2.getParameter(gl2.VERTEX_ARRAY_BINDING);
  assert.equal(qCalls.filter(c => c[0] === 'getParameter').length, 2);
  console.log('PASS: 600 compositor/shadow state saves and restores hit JS cache with zero driver readbacks');
}
