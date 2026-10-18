// Run the scene compositor's own JS against a WebGL 2 context that keeps state:
// a scene smaller than the canvas renders at its size and is filtered up to it.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const original = fs.readFileSync('downstream/wasm/web_scene_fx.c', 'utf8');

function emJs(source, name) {
  const start = source.indexOf(`EM_JS(${name.type}, ${name.name}, (`);
  assert(start >= 0, name.name);
  const params = source.slice(source.indexOf('(', source.indexOf(', (', start) + 1) + 1, source.indexOf('), {', start));
  const end = source.indexOf('\n});\n', start);
  const body = source.slice(source.indexOf('), {', start) + 4, end);
  return {params: params.split(',').map(param => param.trim().split(/\s+|\*/).filter(Boolean).pop()), body};
}

const enums = {
  VIEWPORT: 0x0BA2, SCISSOR_BOX: 0x0C10, MAX_SAMPLES: 0x8D57, CURRENT_PROGRAM: 0x8B8D, ACTIVE_TEXTURE: 0x84E0,
  VERTEX_ARRAY_BINDING: 0x85B5, TEXTURE0: 0x84C0, TEXTURE8: 0x84C8, TEXTURE_2D: 0x0DE1, FRAMEBUFFER: 0x8D40,
  READ_FRAMEBUFFER: 0x8CA8, DRAW_FRAMEBUFFER: 0x8CA9, RENDERBUFFER: 0x8D41, FRAMEBUFFER_COMPLETE: 0x8CD5,
  COLOR_ATTACHMENT0: 0x8CE0, DEPTH_STENCIL_ATTACHMENT: 0x821A, LINK_STATUS: 0x8B82, VERTEX_SHADER: 0x8B31,
  FRAGMENT_SHADER: 0x8B30, RGB8: 0x8051, RGBA8: 0x8058, DEPTH24_STENCIL8: 0x88F0, LINEAR: 0x2601, NEAREST: 0x2600,
  TEXTURE_MIN_FILTER: 0x2801, TEXTURE_MAG_FILTER: 0x2800, TEXTURE_WRAP_S: 0x2802, TEXTURE_WRAP_T: 0x2803,
  CLAMP_TO_EDGE: 0x812F, SCISSOR_TEST: 0x0C11, BLEND: 0x0BE2, DEPTH_TEST: 0x0B71, CULL_FACE: 0x0B44,
  STENCIL_TEST: 0x0B90, POLYGON_OFFSET_FILL: 0x8037, COLOR_BUFFER_BIT: 0x4000, DEPTH_BUFFER_BIT: 0x0100,
  TRIANGLES: 4
};

// The methods live on the prototype like a real context's, so replacing one on
// the context shadows it and deleting the replacement restores it.
class Context {
  constructor(width, height) {
    Object.assign(this, enums);
    this.state = {viewport: [0, 0, width, height], framebuffer: null, program: null,
      unit: 0, units: [], vao: null, draws: [], sizes: new Map(), attachments: new Map(), uniforms: new Map()};
  }
  viewport(x, y, w, h) { this.state.viewport = [x, y, w, h]; }
  getParameter(name) {
    const s = this.state;
    // Chrome answers these from its client; anything else waits on the GPU process.
    assert([enums.VIEWPORT, enums.MAX_SAMPLES, enums.CURRENT_PROGRAM, enums.ACTIVE_TEXTURE,
      enums.VERTEX_ARRAY_BINDING].includes(name), `synchronous query ${name}`);
    return {[enums.VIEWPORT]: Int32Array.from(s.viewport),
      [enums.MAX_SAMPLES]: 4, [enums.CURRENT_PROGRAM]: s.program, [enums.ACTIVE_TEXTURE]: enums.TEXTURE0 + s.unit,
      [enums.VERTEX_ARRAY_BINDING]: s.vao}[name];
  }
  createShader() { return {}; }
  createProgram() { return {}; }
  createVertexArray() { return {}; }
  createTexture() { return {}; }
  createRenderbuffer() { return {}; }
  createFramebuffer() { return {}; }
  getUniformLocation(program, name) { return {program, name}; }
  uniform2f(location, x, y) { if (location.program === this.state.program) this.state.uniforms.set(location.name, [x, y]); }
  getProgramParameter() { return true; }
  checkFramebufferStatus() { return enums.FRAMEBUFFER_COMPLETE; }
  isEnabled() { return false; }
  useProgram(program) { this.state.program = program; }
  activeTexture(unit) { this.state.unit = unit - enums.TEXTURE0; }
  bindTexture(target, texture) { this.state.units[this.state.unit] = texture; }
  texStorage2D(target, levels, format, w, h) { this.state.sizes.set(this.state.units[this.state.unit], [w, h]); }
  bindFramebuffer(target, framebuffer) { if (target !== enums.READ_FRAMEBUFFER) this.state.framebuffer = framebuffer; }
  framebufferTexture2D(target, attachment, textarget, texture) {
    if (attachment === enums.COLOR_ATTACHMENT0) this.state.attachments.set(this.state.framebuffer, texture);
  }
  bindVertexArray(vao) { this.state.vao = vao; }
  drawArrays() {
    const s = this.state;
    s.draws.push({program: s.program, framebuffer: s.framebuffer, viewport: [...s.viewport], source: s.units[8],
      size: s.uniforms.get('u_size')});
    s.uniforms = new Map();
  }
}
for (const name of ['shaderSource', 'compileShader', 'attachShader', 'linkProgram', 'getProgramInfoLog',
  'getShaderInfoLog', 'uniform1i', 'texParameteri', 'bindRenderbuffer',
  'renderbufferStorageMultisample', 'framebufferRenderbuffer', 'deleteFramebuffer', 'deleteRenderbuffer',
  'deleteTexture', 'enable', 'disable', 'blitFramebuffer', 'invalidateFramebuffer', 'colorMask',
  'uniform3f', 'uniform4f', 'uniformMatrix3fv'])
  Context.prototype[name] = function () {};

function run(source) {
  const bindJs = emJs(source, {type: 'int', name: 'WebSceneFx_Bind'});
  const composeJs = emJs(source, {type: 'void', name: 'WebSceneFx_Compose'});
  assert.deepEqual(bindJs.params, ['width', 'height', 'samples', 'screenWidth', 'screenHeight']);
  const context = vm.createContext({Module: {}, HEAPF32: new Float32Array(64), console: {warn() {}}, GLctx: null});
  const bind = vm.runInContext(`(function (${bindJs.params}) {${bindJs.body}\n})`, context);
  const compose = vm.runInContext(`(function (${composeJs.params}) {${composeJs.body}\n})`, context);
  const own = gl => Object.prototype.hasOwnProperty.call(gl, 'viewport');
  const box = (gl, key) => [...gl.state[key]];

  // A 1080-line canvas over a 720-line scene: the scene's target is its own
  // size and the 3D pass's canvas viewports land on it at its scale.
  let gl = context.GLctx = new Context(1920, 1080);
  assert.equal(bind(1280, 720, 4, 1920, 1080), 1);
  let fx = context.Module.cod2SceneFx;
  assert.deepEqual(gl.state.sizes.get(fx.t.color), [1280, 720]);
  assert.deepEqual(gl.state.sizes.get(fx.t.out), [1280, 720]);
  assert.equal(gl.state.framebuffer, fx.t.ms);
  assert.deepEqual(box(gl, 'viewport'), [0, 0, 1280, 720]);
  gl.viewport(960, 540, 960, 540);
  assert.deepEqual(box(gl, 'viewport'), [640, 360, 640, 360]);
  gl.viewport(2, 2, 2, 2);
  assert.deepEqual(box(gl, 'viewport'), [1, 1, 2, 2]);
  gl.viewport(0, 0, 1920, 1080);
  assert.deepEqual(box(gl, 'viewport'), [0, 0, 1280, 720]);

  // Composing renders the effects at the scene's size, filters the result up
  // to the whole canvas and hands the canvas's own viewport back to the HUD.
  gl.state.draws = [];
  compose(0);
  assert(!own(gl));
  assert.deepEqual(box(gl, 'viewport'), [0, 0, 1920, 1080]);
  const draws = gl.state.draws;
  assert.equal(draws.length, 2);
  assert.equal(draws[0].program, fx.programs.compose.program);
  assert.equal(draws[0].framebuffer, fx.t.outFbo);
  assert.equal(gl.state.attachments.get(fx.t.outFbo), fx.t.out);
  assert.deepEqual(draws[0].viewport, [0, 0, 1280, 720]);
  assert.equal(draws[1].program, fx.programs.upscale.program);
  assert.equal(draws[1].framebuffer, null);
  assert.equal(draws[1].source, fx.t.out);
  assert.deepEqual(draws[1].size, [1280, 720]);
  assert.deepEqual(draws[1].viewport, [0, 0, 1920, 1080]);
  gl.viewport(10, 20, 30, 40);
  assert.deepEqual(box(gl, 'viewport'), [10, 20, 30, 40]);

  // Binding again before a compose starts from the canvas's viewport, not
  // the scaled one, and scales it once.
  gl.viewport(0, 0, 1920, 1080);
  bind(1280, 720, 4, 1920, 1080);
  bind(1280, 720, 4, 1920, 1080);
  assert.deepEqual(box(gl, 'viewport'), [0, 0, 1280, 720]);
  gl.viewport(0, 0, 1920, 1080);
  assert.deepEqual(box(gl, 'viewport'), [0, 0, 1280, 720]);
  compose(0);
  assert.deepEqual(box(gl, 'viewport'), [0, 0, 1920, 1080]);

  // Without a target to compose, the canvas still gets its viewport back.
  bind(1280, 720, 4, 1920, 1080);
  const target = fx.t;
  fx.t = null;
  compose(0);
  assert(!own(gl));
  assert.deepEqual(box(gl, 'viewport'), [0, 0, 1920, 1080]);
  fx.t = target;

  // A scene as big as the canvas draws its effects straight into it.
  gl = context.GLctx = new Context(1280, 720);
  assert.equal(bind(1280, 720, 4, 1280, 720), 1);
  fx = context.Module.cod2SceneFx;
  assert(!own(gl));
  gl.viewport(0, 0, 1280, 720);
  compose(0);
  assert.equal(gl.state.draws.length, 1);
  assert.equal(gl.state.draws[0].program, fx.programs.compose.program);
  assert.equal(gl.state.draws[0].framebuffer, null);
  assert.deepEqual(gl.state.draws[0].viewport, [0, 0, 1280, 720]);

  // A viewport the context already replaced, such as a state cache's, is the
  // one scaled and the one left in place.
  gl = context.GLctx = new Context(1920, 1080);
  const cached = [];
  gl.viewport = function (x, y, w, h) { cached.push([x, y, w, h]); Context.prototype.viewport.call(this, x, y, w, h); };
  const cachedViewport = gl.viewport;
  bind(1280, 720, 4, 1920, 1080);
  gl.viewport(0, 0, 1920, 1080);
  assert.deepEqual(cached.at(-1), [0, 0, 1280, 720]);
  compose(0);
  assert.equal(gl.viewport, cachedViewport);
  assert.deepEqual(cached.at(-1), [0, 0, 1920, 1080]);

  // An uneven aspect keeps the canvas's edges on the scene's edges.
  gl = context.GLctx = new Context(1366, 768);
  bind(1281, 720, 1, 1366, 768);
  gl.viewport(0, 0, 1366, 768);
  assert.deepEqual(box(gl, 'viewport'), [0, 0, 1281, 720]);
  compose(0);
  assert.deepEqual(box(gl, 'viewport'), [0, 0, 1366, 768]);
  assert.deepEqual(gl.state.draws.at(-1).viewport, [0, 0, 1366, 768]);
}

run(original);
const mutants = [
  ['            else delete gl.viewport;\n', ''],
  ['            if (fx.ownViewport) Object.defineProperty(gl, "viewport", fx.ownViewport);\n            else delete gl.viewport;',
   '            delete gl.viewport;'],
  ['            gl.viewport.apply(gl, fx.canvasViewport);\n            return true;', '            return true;'],
  ['    if (width !== screenWidth || height !== screenHeight) fx.scale(screenWidth, screenHeight);\n', ''],
  ['fx.pass(P.upscale, null, fx.screenWidth, fx.screenHeight);', 'fx.pass(P.upscale, null, w, h);'],
  ['fx.pass(C, t.outFbo, w, h);', 'fx.pass(C, null, w, h);'],
  ['                fx.texture(0, t.out);\n', ''],
  ['                gl.uniform2f(P.upscale.u_size, w, h);\n', ''],
  ['                gl.useProgram(P.upscale.program);\n                gl.uniform2f', '                gl.uniform2f'],
  ['    fx.unscale();\n    gl.bindFramebuffer(gl.FRAMEBUFFER, fx.t.ms || fx.t.scene);',
   '    gl.bindFramebuffer(gl.FRAMEBUFFER, fx.t.ms || fx.t.scene);'],
  ['Math.round((x + w) * sx) - x0', 'Math.round(w * sx)'],
  ['            fx.canvasViewport = Array.from(gl.getParameter(gl.VIEWPORT));\n',
   '            fx.canvasViewport = Array.from(gl.getParameter(gl.VIEWPORT));\n            gl.getParameter(gl.SCISSOR_BOX);\n'],
  ['    else if (fx) fx.unscale();\n', ''],
  ['var sx = fx.width / screenWidth, sy = fx.height / screenHeight;', 'var sx = 1, sy = 1;'],
];
for (const [before, after] of mutants) {
  assert.equal(original.split(before).length, 2, before);
  assert.throws(() => run(original.replace(before, after)), undefined, `mutant survived: ${after}`);
}
console.log(`PASS: a 720-line scene on 1080-line and 768-line canvases renders at its size, its canvas viewport scales onto it once with no synchronous GL query, compose filters it up to the whole canvas from the scene's texel size and hands the canvas its viewport back, a context's own viewport replacement included; a full-size scene composes straight to the canvas; ${mutants.length} failing mutants`);
