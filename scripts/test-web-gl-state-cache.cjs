const assert = require('node:assert/strict');
const fs = require('node:fs');
const source = fs.readFileSync(require('node:path').join(__dirname, '../site/native-game-adapter.js'), 'utf8');
const start = source.indexOf('  function cacheWebGlState(');
const end = source.indexOf('  function performanceMeter(', start);
assert(start >= 0 && end > start);
const cache = Function(source.slice(start, end) + 'return cacheWebGlState;')();
const calls = [];
const gl = {TEXTURE0: 100, DEPTH_TEST: 0xB71, BLEND: 0xBE2};
for (const name of ['activeTexture','bindTexture','texParameteri','texParameterf','deleteTexture','uniformMatrix4fv','uniform4fv','uniform4f','linkProgram','enable','disable','depthFunc','cullFace','frontFace','depthMask','colorMask'])
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
