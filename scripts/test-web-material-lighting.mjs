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
  // The texture environment as the engine generates it for the lightmapped
  // world: terrain layers blend by its vertex alpha.
  for (const matrix of [false, true]) {
    const uv = unit => matrix ? `(u_textureMatrix${unit} * v_texCoord${unit}).xy` : `v_texCoord${unit}.xy`;
    const material = `vec4 tej_env0_texload0 = texture2D(u_texUnit0, ${uv(0)});\n` +
      'vec3 tej_env0_color = v_color.rgb * tej_env0_texload0.rgb;\nfloat tej_env0_alpha = v_color.a * tej_env0_texload0.a;\n' +
      `vec4 tej_env1_texload1 = texture2D(u_texUnit1, ${uv(1)});\n` +
      'gl_FragColor = vec4(tej_env0_color * tej_env1_texload1.rgb * 2.0, tej_env0_alpha);';
    const s = generate.call({usedTexUnitList: [0, 1]}, {useTextureMatrix: matrix, TexEnvJIT: {getTexUnitType: () => 0x0DE1}},
      {lightingEnabled: false, MAX_LIGHTS: 8, lightEnabled: []}, material);
    const fragment = 'precision mediump float; uniform sampler2D u_texUnit0; uniform sampler2D u_texUnit1;' +
      'uniform mat4 u_textureMatrix0; uniform mat4 u_textureMatrix1; varying highp vec4 v_texCoord0; varying highp vec4 v_texCoord1;' +
      `varying mediump vec4 v_color; ${s.pointDefs}\nvoid main(){${s.fsTexEnvPass}\n${s.pointPass}}`;
    shaders[name].push({model: false, matrix, jit: true, source: '#version 300 es\nout highp vec4 result;\n' +
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
    // A slope turns the world's plane away from the camera about its y axis.
    // eyeScale widens the plane, and each pixel, by its factor.
    const vs = compile(gl.VERTEX_SHADER, '#version 300 es\nprecision highp float;uniform vec3 normal;uniform float uvScale;uniform float slope;' +
      'uniform float eyeScale;uniform float vertexAlpha;' +
      'out highp vec3 v_cod2Eye;out mediump vec3 v_cod2Normal;out highp vec4 v_texCoord0;out highp vec4 v_texCoord1;out mediump vec4 v_color;' +
      'void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);gl_Position=vec4(p*2.-1.,0,1);v_texCoord0=vec4(p*uvScale,0,1);' +
      'v_texCoord1=vec4(p,0,1);v_cod2Eye=vec3((p-.5)*16.*eyeScale,-40.+slope*(p.x-.5)*16.*eyeScale);v_cod2Normal=normal;v_color=vec4(.5,.5,.5,vertexAlpha);}');
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
    const uniformType = (p, name) => {
      for (let i = 0, n = gl.getProgramParameter(p, gl.ACTIVE_UNIFORMS); i < n; ++i)
        if (gl.getActiveUniform(p, i).name === name) return gl.getActiveUniform(p, i).type;
      return null;
    };
    // Uniforms default to off: no maps, no lightmap shading, no model sun;
    // r_reflections on where the sky carries it.
    function draw(entry, set = {}) {
      const p = entry.program; gl.useProgram(p);
      const u = name => gl.getUniformLocation(p, name);
      gl.uniformMatrix4fv(u('u_textureMatrix0'), false, identity); gl.uniformMatrix4fv(u('u_textureMatrix1'), false, identity);
      gl.uniform1i(u('u_texUnit0'), 0); gl.uniform1i(u('u_texUnit1'), 1);
      gl.uniform1i(u('u_cod2DetailMap'), 5); gl.uniform1i(u('u_cod2NormalMap'), 6); gl.uniform1i(u('u_cod2SpecMap'), 7);
      // A shadow sampler may not share a unit with the color maps; with no
      // map bound the sun's shadow stays off (u_cod2SunShadow is zero).
      gl.uniform1i(u('u_cod2SunShadowMap'), 18);
      gl.uniform3fv(u('normal'), set.normal || [0, 0, 1]); gl.uniform1f(u('uvScale'), set.uvScale || 1);
      gl.uniform1f(u('slope'), set.slope || 0); gl.uniform1f(u('eyeScale'), set.eyeScale || 1);
      gl.uniform1f(u('vertexAlpha'), set.vertexAlpha === undefined ? 1 : set.vertexAlpha);
      gl.uniform4fv(u('u_cod2Surface'), set.surface || [0, 0, 0, 1]); gl.uniform4fv(u('u_cod2Detail'), set.detail || [0, 0, 0, 0]);
      gl.uniform4fv(u('u_cod2Model'), [set.sunVisibility || 0, 0, 0, 0]);
      gl.uniform4fv(u('u_cod2Relief'), [set.cavity || 0, set.variation || 0, set.detailRelief || 0, set.sharpen || 0]);
      gl.uniform3fv(u('u_cod2SunEye'), set.sun || [0, 0, 1]);
      if (uniformType(p, 'u_cod2SkyEye') === gl.FLOAT_VEC4)
        gl.uniform4fv(u('u_cod2SkyEye'), [...(set.sky || [0, 1, 0]), set.reflection === undefined ? 1 : set.reflection]);
      else gl.uniform3fv(u('u_cod2SkyEye'), set.sky || [0, 1, 0]);
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
        // COD2_RELIEF_CAVITY: a map leaning 45 degrees from the surface takes
        // 0.85 of the indirect light where the bake sees no sun; in sunlight
        // only the sun's incidence shades it.
        const cavityLit = {surface: [1, 0, 1, 1], shading: 0, sun: [0, 0, 1]};
        lightmap(0);
        texture(6, 16, solid([128, 128, 255, 128]));
        const shadeFlat = center(draw(after, cavityLit));
        texture(6, 16, solid([128, 128, 255, 255]));
        close(center(draw(after, cavityLit)), L.map(l => base * 2 * l * (0.5 + 0.5 * Math.SQRT1_2)), 2,
          `${kind}: a leaning map takes less indirect light in the shade (flat ${shadeFlat})`);
        lightmap(255);
        close(center(draw(after, cavityLit)), L.map((l, c) => base * 2 * (l - S[c] + S[c] * Math.SQRT1_2)), 2,
          `${kind}: in sunlight only the sun's incidence shades it`);
        // COD2_RELIEF_CREASE: where the map's x slope turns from leaning +u to
        // leaning -u the surface folds into a groove and darkens, where it turns
        // back an edge lightens; the flat map does not change. The plane spans
        // 16 units over 64 pixels, so a 32-texel map's texel is half a unit and
        // a 64-texel map's a quarter: the same groove at twice the resolution
        // turns over two texels and folds as much.
        {
          const groove = (size, at) => texture(6, size, x =>
            [128, 128, 255, x < at ? 147 : (size === 64 && x === at ? 128 : 109)]);
          const creaseLit = {surface: [1, 0, 1, 1], shading: 0, sun: [0, 0, 1]};
          const px = (pixels, x, y = 32) => pixels[(y * 64 + x) * 4];
          lightmap(0);
          groove(32, 16);
          const off32 = draw(after, creaseLit), on32 = draw(after, {...creaseLit, cavity: 1});
          check(same(draw(after, {...creaseLit, cavity: 0}), off32), `${kind}: r_cavity 0 folds nothing`);
          const dark32 = px(off32, 31) - px(on32, 31);
          check(dark32 > 15 && Math.abs(px(on32, 10) - px(off32, 10)) <= 1,
            `${kind}: a groove darkens (${px(off32, 31)} to ${px(on32, 31)}), the flat map keeps its light`);
          check(px(on32, 63) > px(off32, 63) + 2, `${kind}: an edge lightens (${px(off32, 63)} to ${px(on32, 63)})`);
          check(px(off32, 31) - px(draw(after, {...creaseLit, cavity: 2}), 31) > dark32 * 1.5, `${kind}: r_cavity 2 folds deeper`);
          groove(64, 32);
          const off64 = draw(after, creaseLit), on64 = draw(after, {...creaseLit, cavity: 1});
          const dark64 = px(off64, 31) - px(on64, 31);
          check(Math.abs(dark64 - dark32) <= 2, `${kind}: the groove at twice the resolution folds as much (${dark32} and ${dark64})`);
          // A slight fold, the stone's own grain, takes none.
          texture(6, 32, x => [128, 128, 255, x < 16 ? 133 : 123]);
          check(same(draw(after, creaseLit), draw(after, {...creaseLit, cavity: 1}), 1), `${kind}: a slight fold takes no shade`);
          // The same groove along v, in the map's y (green).
          texture(6, 32, (x, y) => [128, y < 16 ? 147 : 109, 255, 128]);
          const offV = draw(after, creaseLit), onV = draw(after, {...creaseLit, cavity: 1});
          const darkV = px(offV, 32, 31) - px(onV, 32, 31);
          check(Math.abs(darkV - dark32) <= 2, `${kind}: a groove along v folds as much (${darkV})`);
          // A minified map folds over a pixel's footprint: at two texels a pixel,
          // sampled a mip down, a gentler groove darkens 22% too.
          texture(6, 32, x => [128, 128, 255, x < 16 ? 147 : 128]);
          const offMin = draw(after, {...creaseLit, uvScale: 4}), onMin = draw(after, {...creaseLit, uvScale: 4, cavity: 1});
          const minified = (px(offMin, 7) - px(onMin, 7)) / px(offMin, 7);
          check(minified > 0.15 && minified < 0.3, `${kind}: a minified map folds over a pixel's footprint (${(minified * 100).toFixed(0)}%)`);
          texture(6, 32, (x, y) => [128, y < 16 ? 147 : 128, 255, 128]);
          const offMinV = draw(after, {...creaseLit, uvScale: 4}), onMinV = draw(after, {...creaseLit, uvScale: 4, cavity: 1});
          const minifiedV = (px(offMinV, 32, 7) - px(onMinV, 32, 7)) / px(offMinV, 32, 7);
          check(minifiedV > 0.15 && minifiedV < 0.3, `${kind}: along v too (${(minifiedV * 100).toFixed(0)}%)`);
          // Pixels of a unit and a half fold half as much: a sharp groove there
          // darkens 28% instead of 45%.
          texture(6, 64, x => [128, 128, 255, x < 32 ? 255 : 0]);
          const offFar = draw(after, {...creaseLit, eyeScale: 6}), onFar = draw(after, {...creaseLit, eyeScale: 6, cavity: 1});
          const far = (px(offFar, 31) - px(onFar, 31)) / px(offFar, 31);
          check(far > 0.15 && far < 0.35, `${kind}: pixels of 1.5 units fold less (${(far * 100).toFixed(0)}%)`);
          check(same(draw(before, creaseLit), draw(before, {...creaseLit, cavity: 1})), `${kind}: the baseline has no folds`);
          lightmap(255);
        }
        texture(6, 16, solid([128, 128, 255, 128]));
        // Highlights: the sun's, in sunlight only, scaled by the map.
        const spec = center(draw(after, {surface: [0, 1, 1, 1], shading: 1, reflection: 0}));
        close(spec, L.map((l, c) => base * 2 * l + 128 / 255 * S[c] * 2), 2, `${kind}: sun highlight at the mirror direction`);
        lightmap(0);
        const shade = draw(before);
        check(same(draw(after, {surface: [0, 1, 1, 1], shading: 1, reflection: 0}), shade, 1), `${kind}: no highlight in shadow`);
        // Reflection: the lightmap's indirect light mirrored by the specular
        // map, brighter toward the sky, more at grazing angles the glossier
        // the map, none with r_reflections 0 or a black map.
        const glossy = [255, 255, 255, 247], rough = [255, 255, 255, 0], graze = 3.08;
        texture(7, 16, solid(glossy));
        check(same(draw(after, {surface: [0, 1, 1, 1], reflection: 0}), shade, 1), `${kind}: r_reflections 0 mirrors nothing`);
        // A reflection along the horizon: mix(0.35, 1, smoothstep(-0.3, 0.7, 0)).
        const sideways = 0.35 + 0.65 * 0.216;
        close(center(draw(after, {surface: [0, 1, 1, 1]})), L.map(l => base * 2 * l + l * 2 * sideways * 0.2), 2,
          `${kind}: a glossy map mirrors the indirect light at 0.2 head-on`);
        const mirrored = (set, from = shade) => center(draw(after, {surface: [0, 1, 1, 1], ...set}))[0] - center(from)[0];
        const up = mirrored({sky: [0, 0, 1]}), side = mirrored({}), down = mirrored({sky: [0, 0, -1]});
        check(up > side + 10 && side > down + 3, `${kind}: the sky's side mirrors brighter than the ground's (${down} < ${side} < ${up})`);
        const tilted = draw(before, {slope: graze});
        const glossyFront = side, glossyGraze = mirrored({slope: graze}, tilted);
        texture(7, 16, solid(rough));
        const roughFront = mirrored({}), roughGraze = mirrored({slope: graze}, tilted);
        check(glossyGraze >= glossyFront + 10 && roughGraze <= roughFront + 2 && roughFront * 3 < glossyFront,
          `${kind}: grazing angles mirror more on a glossy map (${glossyFront} to ${glossyGraze}) than a rough one (${roughFront} to ${roughGraze})`);
        texture(7, 16, solid([0, 0, 0, 247]));
        check(same(draw(after, {surface: [0, 1, 1, 1], slope: graze}), tilted, 1), `${kind}: a black specular map mirrors nothing`);
        // In sunlight only the indirect share is mirrored, not the sun's.
        texture(7, 16, solid(glossy));
        lightmap(255);
        const sunlit = {surface: [0, 1, 1, 1], shading: 0, sun: [0, 0, -1]};
        const sunMirror = mirrored(sunlit, draw(after, {...sunlit, reflection: 0}));
        check(sunMirror > 4 && sunMirror < glossyFront * 0.6,
          `${kind}: sunlight mirrors its indirect share only (${sunMirror} vs ${glossyFront} in shade)`);
        texture(7, 16, solid([128, 128, 128, 255]));
        // Detail maps multiply at twice their value, only when flagged.
        texture(5, 16, solid([64, 64, 64, 255]));
        check(same(draw(after), original), `${kind}: an unflagged detail map changes nothing`);
        close(center(draw(after, {detail: [1, 1, 1, 0]})), L.map(l => base * 2 * l * (64 / 255) * 2), 2, `${kind}: detail map at twice its value`);
        texture(5, 16, solid([128, 128, 128, 255]));
        check(same(draw(after, {detail: [1, 1, 1, 0]}), original, 1), `${kind}: a mid-grey detail map keeps the texture`);
        // COD2_SURFACE_VARIATION: over eight repeats of a flat map the tone moves
        // by at most 22% on the ground and 8% on a wall, about the same mean, the
        // paler patches warmer and the darker cooler about the same color; it
        // rides on the map's coordinates alone.
        {
          const ground = {surface: [0, 0, 1, 1], shading: 0, sun: [0, 0, 1], sky: [0, 0, 1], uvScale: 8};
          const wall = {...ground, sky: [0, 1, 0]};
          const plain = draw(after, ground);
          check(same(draw(after, {...ground, variation: 0}), plain) && same(draw(before, {...ground, variation: 1}), draw(before, ground)),
            `${kind}: r_surfaceVariation 0, or the baseline, varies nothing`);
          const ratios = (pixels, channel) => {
            const out = [];
            for (let j = 0; j < 64 * 64; ++j) out.push(pixels[j * 4 + channel] / Math.max(plain[j * 4 + channel], 1));
            return out;
          };
          const spread = r => [Math.min(...r), Math.max(...r)];
          const groundGreen = ratios(draw(after, {...ground, variation: 1}), 1), wallGreen = ratios(draw(after, {...wall, variation: 1}), 1);
          const [gLow, gHigh] = spread(groundGreen), [wLow, wHigh] = spread(wallGreen);
          check(gLow >= 0.76 && gHigh <= 1.25 && gHigh - gLow > 0.15,
            `${kind}: the ground's tone moves within 22% (${gLow.toFixed(3)} to ${gHigh.toFixed(3)})`);
          check(wLow >= 0.9 && wHigh <= 1.1 && wHigh - wLow > 0.05 && (gHigh - gLow) > 2 * (wHigh - wLow),
            `${kind}: a wall's within 8% (${wLow.toFixed(3)} to ${wHigh.toFixed(3)})`);
          const mean = r => r.reduce((a, b) => a + b, 0) / r.length;
          check(Math.abs(mean(groundGreen) - 1) < 0.05, `${kind}: about the same mean (${mean(groundGreen).toFixed(3)})`);
          const varied = draw(after, {...ground, variation: 1});
          const order = [...Array(64 * 64).keys()].sort((a, b) => groundGreen[a] - groundGreen[b]);
          const warmth = list => mean(list.map(j => varied[j * 4 + 2] / Math.max(varied[j * 4], 1)));
          check(warmth(order.slice(-400)) < warmth(order.slice(0, 400)) - 0.02,
            `${kind}: its paler patches are warmer (blue to red ${warmth(order.slice(-400)).toFixed(3)} against ${warmth(order.slice(0, 400)).toFixed(3)})`);
          const hue = mean(ratios(varied, 2)) / mean(ratios(varied, 0));
          check(Math.abs(hue - 1) < 0.015, `${kind}: about the map's own color (blue to red ${hue.toFixed(3)} of the plain map's)`);
          const [g2Low, g2High] = spread(ratios(draw(after, {...ground, variation: 2}), 1));
          check(g2High - g2Low > 1.6 * (gHigh - gLow), `${kind}: r_surfaceVariation 2 varies more (${g2Low.toFixed(3)} to ${g2High.toFixed(3)})`);
          check(same(draw(after, {...ground, variation: 1, eyeScale: 3}), varied, 1), `${kind}: it holds still on the surface as the eye moves off`);
        }
        // COD2_DETAIL_RELIEF: a detail map rising along +u leans the surface
        // toward -u: lit from there it brightens, from +u it darkens, with or
        // without a normal map; minified four texels a pixel, it lies flat.
        {
          texture(5, 16, x => [40 + 12 * x, 40 + 12 * x, 40 + 12 * x, 255]);
          for (const surface of [[1, 0, 1, 1], [0, 0, 1, 1]]) {
            const lit = {surface, shading: 1, detail: [1, 1, 1, 0]};
            const px = pixels => pixels[(32 * 64 + 32) * 4];
            for (const [side, sun, sign] of [['-u', [-0.6, 0, 0.8], 1], ['+u', [0.6, 0, 0.8], -1]]) {
              const flat = px(draw(after, {...lit, sun})), relief = px(draw(after, {...lit, sun, detailRelief: 1}));
              check(sign * (relief - flat) > 5, `${kind}${surface[0] ? '' : ', no normal map'}: lit from ${side} the grain ${sign > 0 ? 'brightens' : 'darkens'} (${flat} to ${relief})`);
            }
            const flat = draw(after, {...lit, sun: [-0.6, 0, 0.8]});
            check(same(draw(after, {...lit, sun: [-0.6, 0, 0.8], detailRelief: 0}), flat), `${kind}: r_detailRelief 0 lifts nothing`);
            const far = {...lit, sun: [-0.6, 0, 0.8], uvScale: 16};
            check(same(draw(after, {...far, detailRelief: 1}), draw(after, far), 1), `${kind}: minified four texels a pixel, it lies flat`);
          }
          texture(5, 16, solid([128, 128, 128, 255]));
        }
        // COD2_TEXTURE_SHARPEN: beside an edge in a magnified map the dark side
        // darkens and the light side lightens; texels well inside either half,
        // and a map minified four texels a pixel, keep their color.
        {
          texture(0, 16, x => x < 8 ? [60, 60, 60, 200] : [180, 180, 180, 200]);
          const plane = {surface: [0, 0, 1, 1], shading: 0, sun: [0, 0, 1]};
          const px = (pixels, x) => pixels[(32 * 64 + x) * 4];
          const soft = draw(after, plane), sharp = draw(after, {...plane, sharpen: 1});
          check(px(sharp, 29) < px(soft, 29) - 3 && px(sharp, 34) > px(soft, 34) + 3,
            `${kind}: an edge gains contrast (${px(soft, 29)} to ${px(sharp, 29)}, ${px(soft, 34)} to ${px(sharp, 34)})`);
          // The map repeats: texels 4 and 12 lie farthest from both of its edges.
          check(Math.abs(px(sharp, 18) - px(soft, 18)) <= 1 && Math.abs(px(sharp, 50) - px(soft, 50)) <= 1,
            `${kind}: texels well inside keep their color`);
          check(same(draw(after, {...plane, sharpen: 0}), soft) && same(draw(before, {...plane, sharpen: 1}), draw(before, plane)),
            `${kind}: r_textureSharpen 0, or the baseline, sharpens nothing`);
          const far = {...plane, uvScale: 16};
          check(same(draw(after, {...far, sharpen: 1}), draw(after, far), 1), `${kind}: minified four texels a pixel, nothing`);
          check(sharp.every((v, j) => j % 4 !== 3 || v === 200), `${kind}: sharpening keeps the alpha`);
          texture(0, 16, solid([100, 100, 100, 200]));
        }
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
        check(same(draw(after, {surface: [0, 1, 0, 1], normal: [0.95, 0, 0.312], reflection: 0}),
          draw(before, {normal: [0.95, 0, 0.312]}), 1), `${kind}: r_reflections 0 mirrors nothing`);
        texture(7, 16, solid([0, 0, 0, 255]));
        check(same(draw(after, {surface: [0, 1, 0, 1], normal: [0.95, 0, 0.312]}), draw(before, {normal: [0.95, 0, 0.312]}), 1),
          `${kind}: a black specular map reflects nothing`);
        texture(7, 16, solid([128, 128, 128, 255]));
      }
      check(draw(after, {surface: [1, 1, 1, 1], detail: [1, 1, 1, 0], sunVisibility: 1}).every((v, j) => j % 4 !== 3 || v === 200),
        `${kind}: every map keeps the material's alpha`);
    }
    // COD2_TERRAIN_BLEND: a layer at half its vertex alpha takes over where its
    // texture is brighter than its mean and gives way where darker, about the
    // same average opacity; at full vertex alpha, under r_surfaceVariation 0
    // and in the baseline its alpha is as authored.
    for (const index of [4, 5]) {
      const before = programs.before[index], after = programs.after[index], kind = 'terrain layer' + (after.matrix ? ', matrix UV' : '');
      texture(0, 16, (x, y) => ((x >> 1) + (y >> 1)) % 2 ? [190, 190, 190, 255] : [70, 70, 70, 255]);
      lightmap(255);
      const layer = {surface: [0, 0, 1, 1], shading: 0, sun: [0, 0, 1], vertexAlpha: 0.5};
      const authored = draw(after, layer), blended = draw(after, {...layer, variation: 1});
      const alphaAt = (pixels, x, y) => pixels[(y * 64 + x) * 4 + 3];
      check([...Array(64 * 64).keys()].every(j => Math.abs(authored[j * 4 + 3] - 128) <= 1), `${kind}: r_surfaceVariation 0 keeps the vertex alpha`);
      let bright = 0, dark = 0, total = 0, count = 0;
      for (let y = 2; y < 62; y += 4) for (let x = 2; x < 62; x += 4) {
        const texelBright = ((x >> 3) + (y >> 3)) % 2 === 1, a = alphaAt(blended, x, y);
        if (texelBright) bright += a; else dark += a;
        total += a; ++count;
      }
      bright /= count / 2; dark /= count / 2;
      check(bright > 175 && dark < 80, `${kind}: brighter texels take over, darker give way (${bright.toFixed(0)} and ${dark.toFixed(0)})`);
      check(Math.abs(total / count - 128) < 12, `${kind}: about the same average opacity (${(total / count).toFixed(0)})`);
      const full = draw(after, {...layer, vertexAlpha: 1, variation: 1});
      check(full.every((v, j) => j % 4 !== 3 || v === 255), `${kind}: at full vertex alpha it is as authored`);
      check(same(draw(before, {...layer, variation: 1}), draw(before, layer)), `${kind}: the baseline blends as authored`);
      texture(0, 16, solid([100, 100, 100, 200]));
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
