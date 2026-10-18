// Compile the linked material passes on a real GPU and read their pixels.
import fs from 'node:fs';
import path from 'node:path';
import {pathToFileURL} from 'node:url';
const [baseline, candidate, playwrightModule, outputDir = 'out/web-material-qa'] = process.argv.slice(2);
if (!baseline || !candidate || !playwrightModule)
  throw Error('Usage: node scripts/test-web-material-lighting.mjs BASELINE_JS CANDIDATE_JS PLAYWRIGHT_MODULE [OUTPUT_DIR]');
const {chromium} = await import(pathToFileURL(path.resolve(playwrightModule)));
fs.mkdirSync(outputDir, {recursive: true});
const shaders = {};
for (const [name, file] of Object.entries({before: baseline, after: candidate})) {
  const source = fs.readFileSync(file, 'utf8');
  const start = source.indexOf('          // COD2_POINT_LIGHT_SHADER:'), end = source.indexOf('          var vsSource =', start);
  if (start < 0 || end <= start) throw Error('Missing linked point-light generator in ' + file);
  const generate = Function('GLImmediate', 'GLEmulation', 'fsTexEnvPass', 'texUnitVaryingList=""',
    source.slice(start, end) + ';return {pointDefs,pointPass,fsTexEnvPass};');
  shaders[name] = [];
  for (const model of [false, true]) for (const matrix of [false, true]) {
    const uv = unit => matrix ? `(u_textureMatrix${unit} * v_texCoord${unit}).xy` : `v_texCoord${unit}.xy`;
    const material = model ? `gl_FragColor = texture2D(u_texUnit0, ${uv(0)}) * v_color;` :
      `gl_FragColor = vec4(texture2D(u_texUnit0, ${uv(0)}).rgb * texture2D(u_texUnit1, ${uv(1)}).rgb * 2.0, texture2D(u_texUnit0, ${uv(0)}).a);`;
    const s = generate.call({usedTexUnitList: model ? [0] : [0, 1]}, {useTextureMatrix: matrix, TexEnvJIT: {getTexUnitType: () => 0x0DE1}},
      {lightingEnabled: model, MAX_LIGHTS: 8, lightEnabled: [true, true, false, false, false, false, false, false]}, material);
    const fragment = 'precision mediump float; uniform sampler2D u_texUnit0; uniform sampler2D u_texUnit1;' +
      'uniform mat4 u_textureMatrix0; uniform mat4 u_textureMatrix1; varying highp vec4 v_texCoord0; varying highp vec4 v_texCoord1;' +
      `varying mediump vec4 v_color; ${s.pointDefs}\nvoid main(){${s.fsTexEnvPass}\n${s.pointPass}}`;
    shaders[name].push({model, matrix, source: '#version 300 es\nout highp vec4 result;\n' +
      fragment.replace(/varying/g, 'in').replace(/texture2D/g, 'texture').replace(/gl_FragColor/g, 'result')});
  }
}
const browser = await chromium.launch({headless: false, executablePath: process.env.CHROME_EXECUTABLE || undefined});
try {
  const page = await browser.newPage({viewport: {width: 900, height: 700}});
  await page.setContent('<body style="background:#222;color:white;font:16px sans-serif"><h2>Material lighting GPU checks</h2><canvas width="64" height="64"></canvas><pre></pre>');
  const result = await page.evaluate(shaders => {
    const canvas = document.querySelector('canvas'), checks = [];
    const gl = canvas.getContext('webgl2', {antialias: false, preserveDrawingBuffer: true});
    const check = (value, message) => { if (!value) throw Error(message); checks.push(message); };
    check(!!gl, 'WebGL2 available');
    const compile = (type, source) => {
      const s = gl.createShader(type); gl.shaderSource(s, source); gl.compileShader(s);
      check(gl.getShaderParameter(s, gl.COMPILE_STATUS), gl.getShaderInfoLog(s) || 'shader compiled'); return s;
    };
    // A plane facing the camera, or tilted for models; UVs grow with x and y.
    const vs = compile(gl.VERTEX_SHADER, '#version 300 es\nprecision highp float;uniform vec3 normal;uniform float uvScale;' +
      'out highp vec3 v_cod2Eye;out mediump vec3 v_cod2Normal;out highp vec4 v_texCoord0;out highp vec4 v_texCoord1;out mediump vec4 v_color;' +
      'void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.-1.,0,1);v_texCoord0=vec4(p*uvScale,0,1);' +
      'v_texCoord1=vec4(p,0,1);v_cod2Eye=vec3((p-.5)*16.,-40.);v_cod2Normal=normal;v_color=vec4(.5,.5,.5,1);}');
    const programs = {};
    for (const [name, list] of Object.entries(shaders)) programs[name] = list.map(shader => {
      const p = gl.createProgram(); gl.attachShader(p, vs); gl.attachShader(p, compile(gl.FRAGMENT_SHADER, shader.source));
      gl.linkProgram(p); check(gl.getProgramParameter(p, gl.LINK_STATUS), gl.getProgramInfoLog(p) || 'program linked');
      return {...shader, program: p};
    });
    const texture = (unit, size, texel) => {
      const t = gl.createTexture(), pixels = new Uint8Array(size * size * 4);
      for (let y = 0; y < size; ++y) for (let x = 0; x < size; ++x) pixels.set(texel(x, y), (y * size + x) * 4);
      gl.activeTexture(gl.TEXTURE0 + unit); gl.bindTexture(gl.TEXTURE_2D, t);
      gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, size, size, 0, gl.RGBA, gl.UNSIGNED_BYTE, pixels); gl.generateMipmap(gl.TEXTURE_2D);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR_MIPMAP_LINEAR);
      gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
    };
    const solid = rgba => () => rgba;
    const identity = new Float32Array([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]);
    // Uniforms default to off: no maps, no lightmap shading, no model sun.
    function draw(entry, set = {}) {
      const p = entry.program; gl.useProgram(p);
      const u = name => gl.getUniformLocation(p, name);
      gl.uniformMatrix4fv(u('u_textureMatrix0'), false, identity); gl.uniformMatrix4fv(u('u_textureMatrix1'), false, identity);
      gl.uniform1i(u('u_texUnit0'), 0); gl.uniform1i(u('u_texUnit1'), 1);
      gl.uniform1i(u('u_cod2DetailMap'), 5); gl.uniform1i(u('u_cod2NormalMap'), 6); gl.uniform1i(u('u_cod2SpecMap'), 7);
      gl.uniform3fv(u('normal'), set.normal || [0, 0, 1]); gl.uniform1f(u('uvScale'), set.uvScale || 1);
      gl.uniform4fv(u('u_cod2Surface'), set.surface || [0, 0, 0, 1]); gl.uniform4fv(u('u_cod2Detail'), set.detail || [0, 0, 0, 0]);
      gl.uniform4fv(u('u_cod2Model'), [set.sunVisibility || 0, 0, 0, 0]);
      gl.uniform3fv(u('u_cod2SunEye'), set.sun || [0, 0, 1]); gl.uniform3fv(u('u_cod2SkyEye'), set.sky || [0, 1, 0]);
      gl.uniform4fv(u('u_cod2SunColor'), [0.3, 0.25, 0.2, set.shading === undefined ? 0.65 : set.shading]);
      gl.uniform4fv(u('u_lightModelAmbient'), [0.1, 0.1, 0.1, 1]); gl.uniform4fv(u('u_materialAmbient'), [1, 1, 1, 1]);
      gl.uniform4fv(u('u_materialDiffuse'), [1, 1, 1, 1]);
      for (const i of [0, 1]) {
        gl.uniform4fv(u('u_lightAmbient' + i), [0.25, 0.25, 0.25, 1]);
        gl.uniform4fv(u('u_lightDiffuse' + i), set.diffuse || [0, 0, 0, 1]);
        gl.uniform4fv(u('u_lightPosition' + i), i ? [0, 1, 0, 0] : [0, 0, 1, 0]);
      }
      gl.drawArrays(gl.TRIANGLES, 0, 3);
      const px = new Uint8Array(64 * 64 * 4); gl.readPixels(0, 0, 64, 64, gl.RGBA, gl.UNSIGNED_BYTE, px); return px;
    }
    const center = px => [...px.subarray((32 * 64 + 32) * 4, (32 * 64 + 32) * 4 + 4)];
    const same = (a, b, tolerance = 0) => a.every((v, i) => Math.abs(v - b[i]) <= tolerance);
    const close = (pixel, rgb, tolerance, message) =>
      check(rgb.every((v, i) => Math.abs(pixel[i] - Math.round(v * 255)) <= tolerance), `${message}: ${pixel} vs ${rgb.map(v => Math.round(v * 255))}`);
    texture(0, 16, solid([100, 100, 100, 200]));
    texture(5, 16, solid([128, 128, 128, 255]));
    texture(6, 16, solid([128, 128, 255, 128]));
    texture(7, 16, solid([128, 128, 128, 255]));
    const lightmap = alpha => texture(1, 16, solid([128, 115, 102, alpha]));
    const base = 100 / 255, L = [128 / 255, 115 / 255, 102 / 255], S = [0.3, 0.25, 0.2];
    for (let i = 0; i < 4; ++i) {
      const before = programs.before[i], after = programs.after[i], kind = (after.model ? 'model' : 'world') + (after.matrix ? ', matrix UV' : '');
      lightmap(255);
      const original = draw(before);
      check(same(draw(after), original), `${kind}: every map and flag off draws the original pixels`);
      check(original.every((v, j) => j % 4 !== 3 || v === (after.model ? 200 : 200)), `${kind}: fixture keeps its alpha`);
      if (!after.model) {
        // The bake already holds the sun: a flat surface facing it, or any
        // surface at r_sunShading 0, keeps the bake.
        check(same(draw(after, {surface: [0, 0, 1, 1], shading: 1}), original, 1), `${kind}: sun along the normal keeps the bake`);
        check(same(draw(after, {surface: [0, 0, 1, 1], shading: 0, sun: [1, 0, 0]}), original, 1), `${kind}: r_sunShading 0 keeps the bake`);
        close(center(draw(after, {surface: [0, 0, 1, 1], shading: 1, sun: [1, 0, 0]})), L.map((l, c) => base * 2 * (l - S[c])), 2,
          `${kind}: a surface edge-on to the sun keeps only the indirect light`);
        close(center(draw(after, {surface: [0, 0, 1, 1], shading: 0.65, sun: [Math.sqrt(0.75), 0, 0.5]})),
          L.map((l, c) => base * 2 * (l - S[c] + S[c] * (0.35 + 0.65 * 0.5))), 2, `${kind}: r_sunShading 0.65 at 60 degrees`);
        // Relief: the map's x (alpha) leans toward +u, its y (green) toward +v.
        const lit = {surface: [1, 0, 1, 1], shading: 0};
        for (const [axis, sun, texel] of [['x', [0.6, 0, 0.8], v => [128, 128, 255, v]], ['y', [0, 0.6, 0.8], v => [128, v, 255, 128]]]) {
          texture(6, 16, solid(texel(128)));
          const flat = center(draw(after, {...lit, sun}));
          texture(6, 16, solid(texel(255)));
          const toward = center(draw(after, {...lit, sun}));
          texture(6, 16, solid(texel(0)));
          const away = center(draw(after, {...lit, sun}));
          check(toward[0] > flat[0] + 8 && away[0] < flat[0] - 8, `${kind}: relief along ${axis} faces the sun (${away[0]} < ${flat[0]} < ${toward[0]})`);
          close(flat, L.map(l => base * 2 * l), 1, `${kind}: a flat normal map along ${axis} keeps the bake`);
        }
        texture(6, 16, solid([128, 128, 255, 128]));
        // Highlights: the sun's, in sunlight only, scaled by the map.
        const spec = center(draw(after, {surface: [0, 1, 1, 1], shading: 1}));
        close(spec, L.map((l, c) => base * 2 * l + 128 / 255 * S[c] * 2), 2, `${kind}: sun highlight at the mirror direction`);
        lightmap(0);
        const shade = draw(before);
        check(same(draw(after, {surface: [0, 1, 1, 1], shading: 1}), shade, 1), `${kind}: no highlight in shadow`);
        lightmap(255);
        // Detail maps multiply at twice their value, only when flagged.
        texture(5, 16, solid([64, 64, 64, 255]));
        check(same(draw(after), original), `${kind}: an unflagged detail map changes nothing`);
        close(center(draw(after, {detail: [1, 1, 1, 0]})), L.map(l => base * 2 * l * (64 / 255) * 2), 2, `${kind}: detail map at twice its value`);
        texture(5, 16, solid([128, 128, 128, 255]));
        check(same(draw(after, {detail: [1, 1, 1, 0]}), original, 1), `${kind}: a mid-grey detail map keeps the texture`);
      } else {
        const color = base * 0.5;
        // Lit models keep the vertex lighting; flat normals change nothing.
        check(same(draw(after, {sunVisibility: 1, diffuse: [0.4, 0.4, 0.4, 1]}), original, 1), `${kind}: directional lights with flat normals keep the vertex color`);
        // Ambient-only models restore the sun's incidence the grid folded in.
        check(same(draw(after, {sunVisibility: 1, shading: 1}), original, 1), `${kind}: sun along the normal keeps the ambient`);
        check(same(draw(after, {sunVisibility: 1, shading: 0, sun: [1, 0, 0]}), original, 1), `${kind}: r_sunShading 0 keeps the ambient`);
        const ambient = 0.1 + 0.25 * 2;
        close(center(draw(after, {sunVisibility: 1, shading: 1, sun: [1, 0, 0]})), S.map(s => color * (ambient - s) / ambient), 2,
          `${kind}: edge-on sun removes its share of the ambient`);
        // Sky reflection: from the map's color, stronger at grazing angles.
        texture(7, 16, solid([255, 255, 255, 255]));
        const front = center(draw(after, {surface: [0, 1, 0, 1]}))[0] - center(original)[0];
        const graze = center(draw(after, {surface: [0, 1, 0, 1], normal: [0.95, 0, 0.312]}))[0] -
          center(draw(before, {normal: [0.95, 0, 0.312]}))[0];
        check(front > 4 && graze > front * 1.3, `${kind}: reflection grows toward grazing angles (${front} to ${graze})`);
        texture(7, 16, solid([0, 0, 0, 255]));
        check(same(draw(after, {surface: [0, 1, 0, 1], normal: [0.95, 0, 0.312]}), draw(before, {normal: [0.95, 0, 0.312]}), 1),
          `${kind}: a black specular map reflects nothing`);
        texture(7, 16, solid([128, 128, 128, 255]));
      }
      check(draw(after, {surface: [1, 1, 1, 1], detail: [1, 1, 1, 0], sunVisibility: 1}).every((v, j) => j % 4 !== 3 || v === 200),
        `${kind}: every map keeps the material's alpha`);
    }
    check(gl.getError() === gl.NO_ERROR, 'no WebGL errors');
    const debug = gl.getExtension('WEBGL_debug_renderer_info');
    const result = {passed: true, checks, renderer: debug ? gl.getParameter(debug.UNMASKED_RENDERER_WEBGL) : gl.getParameter(gl.RENDERER)};
    document.querySelector('pre').textContent = JSON.stringify(result, null, 2);
    return result;
  }, shaders);
  fs.writeFileSync(path.join(outputDir, 'gpu.json'), JSON.stringify(result, null, 2));
  console.log(`PASS: ${result.checks.length} real WebGL2 material lighting checks on ${result.renderer}`);
} finally {
  await browser.close();
}
