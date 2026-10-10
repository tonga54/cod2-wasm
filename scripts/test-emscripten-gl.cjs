// Exercise the real linked GL generator without instantiating the engine.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const source = fs.readFileSync(process.argv[2] || 'out/cod2-wasm-core/site/cod2.js', 'utf8');
const start = source.indexOf('CTexUnit.prototype.genPassLines = function');
const end = source.indexOf('CTexUnit.prototype.getTexType', start);
assert(start >= 0 && end > start, 'linked legacy GL generator is present');
const generate = Function('CTexUnit', 'TEXENVJIT_NAMESPACE_PREFIX',
  source.slice(start, end) + '\nreturn CTexUnit.prototype.genPassLines;')(function () {}, 'tej_');
function run(lines, enabled = true) {
  return generate.call({enabled: () => enabled, env: {genPassLines: () => lines}}, 'out', 'color', 0).join('\n');
}
const matrix = 'texture2D(u_texUnit0, (u_textureMatrix0 * v_texCoord0).xy)';
const cube = 'textureCube(u_texUnit1, normalize((matrix * coords).xyz))';
assert.equal(run([`vec3 c = ${matrix}.rgb;`, `float a = ${matrix}.a;`]),
  `vec4 tej_env0_texload0 = ${matrix};\nvec3 c = tej_env0_texload0.rgb;\nfloat a = tej_env0_texload0.a;`);
assert.equal(run([`vec4 c = ${matrix} * ${cube} + ${matrix} * ${cube};`]),
  `vec4 tej_env0_texload0 = ${matrix};\nvec4 tej_env0_texload1 = ${cube};\nvec4 c = tej_env0_texload0 * tej_env0_texload1 + tej_env0_texload0 * tej_env0_texload1;`);
assert.equal(run([`vec4 c = ${matrix};`]), `vec4 c = ${matrix};`);
assert.equal(run(['vec4 c = color;']), 'vec4 c = color;');
assert.equal(run([], false), 'vec4 out = color;');
assert.throws(() => run(['texture2D(unit, (coords)']), /Unbalanced/);
console.log('PASS: linked GL generator preserves nested texture expressions and duplicate-load optimization');

const sampleStart = source.indexOf('function genTexUnitSampleExpr(');
const sampleEnd = source.indexOf('    function getTypeFromCombineOp(', sampleStart);
const sampleSource = source.slice(sampleStart, sampleEnd);
const samplerStart = source.indexOf('            var samplerType =');
const samplerEnd = source.indexOf('            vsTexCoordInits +=', samplerStart);
assert(sampleStart >= 0 && sampleEnd > sampleStart && samplerStart >= 0 && samplerEnd > samplerStart);
for (const type of [0x0DE1, 0x8513]) {
  const unit = {getTexType: () => type};
  const sample = Function('s_texUnits', 'GL_TEXTURE_1D', 'GL_TEXTURE_2D', 'GL_TEXTURE_3D',
    'GL_TEXTURE_CUBE_MAP', 'TEX_COORD_VARYING_PREFIX', 'TEX_MATRIX_UNIFORM_PREFIX',
    'TEX_UNIT_UNIFORM_PREFIX', sampleSource + '\nreturn genTexUnitSampleExpr(0);')(
    [unit], 0x0DE0, 0x0DE1, 0x806F, 0x8513, 'v_texCoord', 'u_textureMatrix', 'u_texUnit');
  const cubeTarget = type === 0x8513;
  assert.equal(sample, `${cubeTarget ? 'textureCube' : 'texture2D'}(u_texUnit0, (u_textureMatrix0 * v_texCoord0).${cubeTarget ? 'xyz' : 'xy'})`);
  const uniform = Function('GLImmediate', 'texUnit', 'uTexUnitPrefix',
    'var texUnitUniformList = "";\n' + source.slice(samplerStart, samplerEnd) + '\nreturn texUnitUniformList;')(
    {TexEnvJIT: {getTexUnitType: () => type}}, 0, 'u_texUnit');
  assert.equal(uniform, `uniform ${cubeTarget ? 'samplerCube' : 'sampler2D'} u_texUnit0;\n`);
}
console.log('PASS: 2D/cube targets select matching sampler types and coordinate dimensions');

const lightingStart = source.indexOf('          var vsLightingDefs = "";');
const lightingEnd = source.indexOf('          // COD2_POINT_LIGHT_SHADER:', lightingStart);
assert(lightingStart >= 0 && lightingEnd > lightingStart);
const lighting = Function('GLEmulation', source.slice(lightingStart, lightingEnd) + '\nreturn {vsLightingDefs,vsLightingPass};');
for (let mask = 0; mask < 256; mask++) {
  const {vsLightingDefs, vsLightingPass} = lighting({lightingEnabled: true, MAX_LIGHTS: 8,
    lightEnabled: Array.from({length:8}, (_, i) => Boolean(mask & (1 << i)))});
  assert(vsLightingDefs.includes('attribute vec3 a_normal;'));
  assert(vsLightingPass.includes('normalize(u_normalMatrix * a_normal)'));
  assert(vsLightingPass.indexOf('v_color *= a_color;') < vsLightingPass.indexOf('v_color = clamp('));
  for (let i = 0; i < 8; i++)
    assert.equal(vsLightingDefs.includes(`uniform vec4 u_lightDiffuse${i};`), Boolean(mask & (1 << i)));
}
assert.equal(lighting({lightingEnabled:false}).vsLightingPass, '');
console.log('PASS: all 256 light masks generate GPU lighting with normals and original vertex tint/alpha');

// Execute the linked flush against a GPU that keeps prior draws in flight.
// Rewriting the same storage would force a synchronization wait.
const flushStart = source.indexOf('flush(numProvidedIndexes,');
let flushEnd = source.indexOf('{', flushStart) + 1, flushDepth = 1;
while (flushDepth) { flushDepth += (source[flushEnd] === '{') - (source[flushEnd] === '}'); flushEnd++; }
assert(flushStart >= 0);
const gpuHeap = new Uint16Array(1024);gpuHeap.set([0,1,2,2,3,0],8);
let storage, uploadCount=0, allocationCount=0, drawCount=0;
const flushGL = {ARRAY_BUFFER:0x8892,ELEMENT_ARRAY_BUFFER:0x8893,STREAM_DRAW:0x88e0,
  UNSIGNED_SHORT:0x1403,currentArrayBufferBinding:1,currentElementArrayBufferBinding:0,
  bindBuffer(){},bufferData(target,size,usage){
    assert.equal(target,this.ELEMENT_ARRAY_BUFFER);assert.equal(usage,this.STREAM_DRAW);
    assert.equal(size & (size-1),0);storage={size,inFlight:false};allocationCount++;
  },bufferSubData(target,offset,values){
    assert(!storage.inFlight,'upload must not overwrite storage still used by the GPU');
    assert(offset+values.byteLength<=storage.size);assert.deepEqual([...values],[0,1,2,2,3,0]);uploadCount++;
  },drawElements(mode,count,type,offset){assert.equal(count,6);assert.equal(offset,0);storage.inFlight=true;drawCount++;}
};
const flushImmediate={vertexCounter:16,stride:16,mode:4,
  getRenderer:()=>({prepare(){},cleanup(){}})};
const flushGPU={MAX_TEMP_BUFFER_SIZE:1024,buffers:[],getTempIndexBuffer:()=>1,
  log2ceilLookup:size=>Math.ceil(Math.log2(size))};
const flush=Function('GLImmediate','GLctx','GL','HEAPU16','assert',
  'return function '+source.slice(flushStart,flushEnd))(flushImmediate,flushGL,flushGPU,gpuHeap,assert);
for(let i=0;i<600;i++)flush(6,0,16);
assert.equal(drawCount,600);assert.equal(uploadCount,600);assert.equal(allocationCount,600);
flushImmediate.vertexCounter=0;flush(6,0,16);assert.equal(allocationCount,600);
console.log('PASS: 600 indexed draws replace busy GPU storage before uploading, preserving index data and capacity');

// Exercise the actual linked indexed-draw wrapper with sparse index ranges.
const drawStart = source.indexOf('function _glDrawElements(');
const drawEnd = source.indexOf('\nvar _emscripten_glDrawElements', drawStart);
assert(drawStart >= 0 && drawEnd > drawStart);
const heap = new ArrayBuffer(64 * 1024);
const HEAPU16 = new Uint16Array(heap);
const HEAPF32 = new Float32Array(heap);
const gl = { UNSIGNED_SHORT: 0x1403, currentArrayBufferBinding: 0, currentElementArrayBufferBinding: 0 };
let prepared = 0, flushed = 0;
const immediate = {
  totalEnabledClientAttributes: 3, vertexPointer: 4096, stride: 16,
  prepareClientAttributes(count) { prepared = count; },
  flush(count) { flushed = count; }
};
const draw = Function('GLImmediate', 'GLctx', 'HEAPU16', 'HEAPF32', 'HEAP8', 'assert', 'out',
  source.slice(drawStart, drawEnd) + '\nreturn _glDrawElements;')(
  immediate, gl, HEAPU16, HEAPF32, new Uint8Array(heap), assert, () => {});
for (const indices of [[253, 254, 255], [0, 0, 0], [7, 2, 4], [0, 1, 2, 2, 3, 0]]) {
  HEAPU16.set(indices, 0);
  draw(4, indices.length, gl.UNSIGNED_SHORT, 0);
  assert.equal(prepared, Math.max(...indices) + 1, 'restriding covers the highest referenced vertex');
  assert.equal(flushed, indices.length, 'GPU draw still uses index count');
  assert.equal(immediate.firstVertex, Math.min(...indices));
  assert.equal(immediate.lastVertex, Math.max(...indices) + 1);
  assert.equal(immediate.vertexData.byteLength, prepared * immediate.stride);
}
draw(4, 3, gl.UNSIGNED_SHORT, 0, 250, 255);
assert.equal(prepared, 256, 'explicit DrawRangeElements bounds are respected');
assert.equal(immediate.firstVertex, 250);
prepared = flushed = 0;
draw(4, 0, gl.UNSIGNED_SHORT, 0);
assert.equal(prepared, 0);
assert.equal(flushed, 0);
gl.currentArrayBufferBinding = 1;
draw(4, 3, gl.UNSIGNED_SHORT, 0);
assert.equal(prepared, 3, 'GPU vertex buffers do not require client restriding');
assert.equal(flushed, 3);
console.log('PASS: indexed client draws copy the referenced vertex range, including sparse indices and vertex zero');

// In the fixed-function-only build, static GPU attributes are configured by
// the pointer calls, while CPU arrays are still deferred until restriding.
// Losing either route silently drops world geometry, colors or lightmap UVs.
function linkedFunction(name) {
  const begin = source.indexOf(`function ${name}(`);
  assert(begin >= 0, name);
  let end = source.indexOf('{', begin) + 1, depth = 1;
  while (depth) { depth += (source[end] === '{') - (source[end] === '}'); end++; }
  return source.slice(begin, end);
}
const attributes = [], deferred = [];
const fixedGL = {currentArrayBufferBinding: 1,
  vertexAttribPointer(...args) { attributes.push(args); }};
const fixedImmediate = {VERTEX:0,NORMAL:1,COLOR:2,TEXTURE0:3,clientActiveTexture:0,
  setClientAttribute(...args) { deferred.push(args); }};
const pointers = Object.fromEntries(['Vertex','Normal','Color','TexCoord'].map(name => {
  const fn = `_gl${name}Pointer`;
  return [name, Function('GLImmediate','GLctx', `return ${linkedFunction(fn)}`)(fixedImmediate,fixedGL)];
}));
pointers.Vertex(3,0x1406,32,64);
pointers.Color(4,0x1401,32,76);
pointers.TexCoord(2,0x1406,32,80);
fixedImmediate.clientActiveTexture = 1;
pointers.TexCoord(2,0x1406,32,88);
pointers.Normal(0x1406,36,128);
assert.deepEqual(attributes, [[0,3,0x1406,false,32,64],[2,4,0x1401,true,32,76],
  [3,2,0x1406,false,32,80],[4,2,0x1406,false,32,88],[1,3,0x1406,true,36,128]]);
fixedGL.currentArrayBufferBinding = 0;
pointers.Vertex(3,0x1406,24,4096);pointers.Color(4,0x1401,24,4108);
pointers.TexCoord(2,0x1406,24,4112);pointers.Normal(0x1406,36,8192);
assert.equal(attributes.length,5);assert.equal(deferred.length,9);
console.log('PASS: fixed-function GPU world/model attributes retain position, normalized colors/normals and both UV sets; dynamic CPU arrays remain deferred');
