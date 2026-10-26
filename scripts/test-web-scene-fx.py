#!/usr/bin/env python3
"""Exercise the production scene post-processing view/frame logic."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = root / 'downstream/wasm/web_scene_fx.c'
backend = (root / 'src/PC/gfx_d3d/rb_backend.c').read_text()
# The view begins before RB_Set3D, composes before the first 2D draw and
# always finishes before Present.
begin = backend.index('WebSceneFx_BeginView(vp->viewport.X')
assert begin < backend.index('RB_Set3D();', begin)
# The world depth is linearized with the view's own infinite projection, and
# the aim focus comes from the cgame each scene.
assert backend.index('vp->depthHackNearClip,', begin) < \
    backend.index('-((const float *)&vp->projectionMatrix)[14]);', begin) < backend.index('RB_Set3D();', begin)
# Motion blur compares each view's camera with the earlier ones, by the
# game's time of the scene the view begins.
camera = backend.index('WebSceneFx_SetCamera((const float *)&vp->viewMatrix, (const float *)&vp->projectionMatrix, '
                       'backEnd.sceneDef.time);')
assert backend.rindex('#ifdef __EMSCRIPTEN__', 0, begin) < camera < begin
assert backend.rindex('backEnd.sceneDef = cmd->sceneDef;', 0, camera) > backend.rindex('static void RB_BeginViewCmd', 0, camera)
assert 'extern void WebSceneFx_SetCamera(const float *, const float *, int);' in backend
scene = (root / 'src/PC/gfx_d3d/r_scene.c').read_text()
assert 'WebSceneFx_SetAds(g_webAdsDofFrac, g_webAdsViewmodelStart, g_webAdsViewmodelEnd, g_webAdsNearEnd);' in scene
backdrop = 'WebSceneFx_SetBackdrop(g_webBackdropFrac > g_webMenuBackdropFrac ? g_webBackdropFrac : g_webMenuBackdropFrac);'
assert scene.index('WebSceneFx_SetAds(g_webAdsDofFrac') < scene.index('WebSceneFx_SetAim(g_webAimFrac);') < \
    scene.index(backdrop) < \
    scene.index('WebSceneFx_SetBackdropGray(g_webBackdropGray);') < scene.index('#endif', scene.index('WebSceneFx_SetAim('))
# The end of a match blurs the view behind its scoreboard; the class screens
# of a match blur it and turn it gray; a menu that blurs the world (the pause
# menu, the team and class choices) blurs it too.
endgame = (root / 'src/PC/cgame_mp/cg_endgame_mp.c').read_text()
assert 'float g_webBackdropFrac = 0.0f;' in endgame and 'float g_webBackdropGray = 0.0f;' in endgame
# The countdown before a match grays the view after a menu's gray is set.
assert 'float g_webGrayFrac = 0.0f;' in (root / 'src/PC/cgame_mp/cg_prematch_mp.c').read_text()
assert scene.index('WebSceneFx_SetBackdropGray(g_webBackdropGray);') < scene.index('WebSceneFx_SetGray(g_webGrayFrac);') < \
    scene.index('#endif', scene.index('WebSceneFx_SetAim('))
view = (root / 'src/PC/cgame_mp/cg_view_mp.c').read_text()
calc_view = view.index('static void CG_CalcViewValues(void)\n{')
assert view.index('menuBlur = CL_GetMenuBlurRadius();', calc_view) < view.index(
    'g_webMenuBackdropFrac = CG_MenuBackdropStep(g_webMenuBackdropFrac, menuBlur, cg->frametime);', calc_view) < \
    view.index('if (cg->cubemapShot != 0) {', calc_view)
step_start = view.index('float CG_MenuBackdropStep(float frac, float menuBlur, int frametime)\n{')
step = view[view.rindex('#define CG_MENU_BACKDROP_MSEC', 0, step_start):view.index('\n}\n', step_start) + 3]


def run_step(code):
    with tempfile.TemporaryDirectory(prefix='cod2-menu-backdrop-') as tmp:
        (Path(tmp) / 'step.c').write_text('#include <assert.h>\n' + code + r'''
int main(void) {
    float frac = 0.0f;
    for (int i = 0; i < 9; ++i) frac = CG_MenuBackdropStep(frac, 5.0f, 16);
    assert(frac > 0.9f && frac < 0.98f);
    frac = CG_MenuBackdropStep(frac, 5.0f, 16);
    assert(frac == 1.0f && CG_MenuBackdropStep(frac, 5.0f, 1000) == 1.0f);
    frac = CG_MenuBackdropStep(frac, 0.0f, 75);
    assert(frac > 0.49f && frac < 0.51f);
    assert(CG_MenuBackdropStep(frac, 0.0f, 1000) == 0.0f && CG_MenuBackdropStep(0.0f, 0.0f, 16) == 0.0f);
    assert(CG_MenuBackdropStep(0.5f, 5.0f, 0) == 0.5f);
    return 0;
}
''')
        subprocess.run(['cc', '-std=c99', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        str(Path(tmp) / 'step.c'), '-o', str(Path(tmp) / 'step')], check=True)
        return subprocess.run([str(Path(tmp) / 'step')], capture_output=True).returncode


assert run_step(step) == 0
for before, after in (('menuBlur > 0.0f ? step : -step', 'step'), ('150.0f', '1500.0f'),
                      ('frac > 1.0f ? 1.0f : frac', 'frac')):
    assert step.count(before) == 1 and run_step(step.replace(before, after)) != 0, after
print('PASS: menus that blur the world ease the compositor backdrop in and out over 150 ms; 3 mutants fail')
# The aim fraction is read before any early return, so scopes, which hide the
# gun, still fade the blur out.
weapons = (root / 'src/PC/cgame_mp/cg_weapons.c').read_text()
view_weapon = weapons.index('void CG_AddViewWeapon(playerState_t *ps)\n{')
assert view_weapon < weapons.index('g_webAimFrac = playerState->fWeaponPosFrac;', view_weapon) < \
    weapons.index('return;', view_weapon)
dvars = (root / 'src/PC/gfx_d3d/r_dvars.c').read_text()
assert 'r_motionBlur = ri.Dvar_RegisterBool("r_motionBlur", 1, 0x2001);' in dvars
assert 'r_sceneHeight = ri.Dvar_RegisterInt("r_sceneHeight", 720, 0, 2160, 0x2001);' in dvars
assert 'mtx[2][2] = c;' in (root / 'src/PC/universal/com_math.c').read_text()
set2d = backend.index('static void RB_Set2D(void)')
assert set2d < backend.index('WebSceneFx_EndView();', set2d) < backend.index('backEnd.projection2D = 1;', set2d)
end_frame = backend.index('static void RB_EndFrame_real(void)')
assert end_frame < backend.index('WebSceneFx_EndFrame();', end_frame) < backend.index('vtable[0x44 / 4]', end_frame)

# The compositor does not read back or restore GL write masks because the D3D9
# device sets both from its own state for every clear and draw.
device = (root / 'src/Mac/DirectX_9/CDirect3DDevice.c').read_text()
def device_function(name):
    start = device.index('HRESULT ' + name + '(')
    return device[start:device.index('\nHRESULT ', start + 1)]
clear = device_function('CDirect3DDevice_Clear')
assert 'glColorMask((dev->colorWriteEnable & 1) != 0' in clear and 'glDepthMask(dev->zWriteEnable ? 1 : 0);' in clear
draw = device_function('CDirect3DDevice_DrawIndexedPrimitive')
assert 'glDepthMask(dev->zWriteEnable ? 1 : 0);' in draw and 'glDepthMask(0);' in draw
assert draw.index('glColorMask(\n            (dev->colorWriteEnable & 1) != 0') < draw.index('glColorMask(1, 1, 1, 1);')
compositor = source.read_text()
assert 'COLOR_WRITEMASK' not in compositor and 'DEPTH_WRITEMASK' not in compositor
# The compositor reads the backdrop and its gray where the C side writes
# them, and mixes the composed color toward its Rec. 709 luma last.
assert 'backdrop = H[o + 33];' in compositor and 'var gray = H[o + 34]' in compositor
assert 'gl.uniform4f(C.u_px, 1 / w, 1 / h, w / h, gray > 0 ? gray : 0);' in compositor
compose_shader = compositor[compositor.index('link("compose"'):compositor.index('link("upscale"')]
assert compose_shader.index('if (u_glow.x > 0.0)') < compose_shader.index('if (u_film.w > 0.0) {') < \
    compose_shader.index('col = mix(col, vec3(dot(col, vec3(0.2126, 0.7152, 0.0722))), u_px.w);') < \
    compose_shader.index('o = vec4(col, 1.0);')
# Ambient occlusion reads its strength, radius and projection where the C
# side writes them (55-58), resolves the multisampled depth it needs, runs at
# half the scene's size in two passes, and darkens only world pixels in the
# compose pass, before the aim blur mixes in; its result is the eighth source.
assert 'var ao = H[o + 55] > 0, haze = H[o + 62] > 0, shafts = H[o + 73] > 0;' in compositor
assert ('dof || motion > 0 || vm || ao || haze || shafts || blast || taa ?\n'
        '                    (gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT) : gl.COLOR_BUFFER_BIT, gl.NEAREST);') in compositor
assert 'gl.uniform4f(A.u_aoView, H[o + 15], 0.9995002746582031, H[o + 57], H[o + 58]);' in compositor
assert 'gl.uniform4f(A.u_aoParams, H[o + 56], H[o + 55], w, h);' in compositor
assert 'fx.pass(A, t.aoFbo[0], t.hw, t.hh);' in compositor and 'fx.pass(P.aoBlur, t.aoFbo[1], t.hw, t.hh);' in compositor
assert 'hw: Math.max(1, Math.ceil(w / 2)), hh: Math.max(1, Math.ceil(h / 2)),' in compositor
assert 'gl.uniform4f(C.u_aoMix, ao ? 1 : 0, 0.5 / t.hw, 0.5 / t.hh, 0);' in compositor
assert 'vm ? vm.smear : null, vm ? vm.mesh : null, ao ? t.ao[1] : null,\n                shafts ? t.sh[1] : null, clarity ? t.cl[1] : null];' in compositor
assert ('["u_scene", "u_bloom", "u_depth", "u_dofMed", "u_dofLarge", "u_vmVelocity", "u_vmMesh", "u_ao", "u_shafts",\n'
        '                 "u_clarityMap"]') in compositor
ao_shader = compositor[compositor.index('link("ao"'):compositor.index('link("aoBlur"')]
assert '"  if (d < 0.2 || d >= 0.9999 || r < 1.5) return;\\n"' in ao_shader, 'the viewmodel and the sky stay open'
assert ao_shader.count('"    if (dq < 0.2) continue;\\n"') == 2, 'the viewmodel hides nothing behind it, near or wide'
# A second ring three times as wide, its samples within a fifth of the scene's
# height, weighs 0.6 of the first.
assert '"  highp float rw = min(3.0 * r, 0.2 * u_aoParams.w);\\n"' in ao_shader
assert ao_shader.index('"  highp float rw = min(3.0 * r, 0.2 * u_aoParams.w);\\n"') < \
    ao_shader.index('"  r = min(r, 0.1 * u_aoParams.w);\\n"'), 'the wide ring scales the unclamped radius'
assert '"    ivec2 q = clamp(c + ivec2(vec2(cos(a), sin(a)) * (rw * (float(i) + 0.5) * 0.125)), ivec2(0), m);\\n"' in ao_shader
assert ('"    wide += max(dot(v, n) * inversesqrt(max(vv, 1e-6)) - 0.15, 0.0) * '
        'max(1.0 - vv / (9.0 * r2), 0.0);\\n"') in ao_shader
assert ao_shader.index('"  occlusion += 0.6 * wide;\\n"') < ao_shader.index('"  float fade = 1.0 - smoothstep(1400.0, 2600.0, z);\\n"')
# Local contrast (r_clarity) reads its strength where the C side writes it
# (100): the scene's light at a quarter of its size, spread by the aim blur's
# kernel to a deviation of 5.3 pixels at 480 lines, is the tenth source, on
# unit 21. The compose pass moves each pixel's light from it after the
# sharpening and before the occlusion, never over a smear.
assert 'r_clarity = ri.Dvar_RegisterFloat("r_clarity", 1.0f, 0.0f, 2.0f, 0x2001);' in dvars
assert 'var clarity = H[o + 100] > 0, taa = H[o + 102] > 0;' in compositor
# The film grade lifts the shade by u_tone (101) in place of a contrast curve,
# before the countdown's own contrast.
assert 'gl.uniform4f(C.u_tone, H[o + 101], 0, 0, 0);' in compositor
film_block = compose_shader[compose_shader.index('if (u_film.w > 0.0) {'):]
assert film_block.index('"    col += u_tone.x * col * (1.0 - col) * (1.0 - col);\\n"') < \
    film_block.index('"    col += u_film.z * col * (1.0 - col) * (col - 0.35) * 1.5384615;\\n"')
# Temporal antialiasing (102-118): the scene, its running average and the
# resolved depth go into the taa pass, which writes the other of two histories
# at the scene's size; its result is the compose pass's scene. A frame without
# it, or the first with it, keeps no history.
assert 'gl.uniform4f(T.u_taa, t.taaValid ? H[o + 102] : 1, H[o + 57], H[o + 58], H[o + 15]);' in compositor
assert 'gl.uniformMatrix3fv(T.u_taaRotation, false, H, o + 103, 9);' in compositor
assert 'gl.uniform3f(T.u_taaMove, H[o + 112], H[o + 113], H[o + 114]);' in compositor
assert 'gl.uniform4f(T.u_taaLens, H[o + 115], H[o + 116], H[o + 117], H[o + 118]);' in compositor
taa_js = compositor[compositor.index('            if (taa) {\n'):compositor.index('            if (ao) {\n')]
assert taa_js.index('fx.texture(0, t.color);\n                fx.texture(1, t.taa[t.taaCur]);\n'
                    '                fx.texture(2, t.depth);\n                fx.pass(T, t.taaFbo[cur], w, h);') < \
    taa_js.index('t.taaCur = cur;\n                t.taaValid = true;\n                scene = t.taa[cur];') < \
    taa_js.index('            } else {\n                t.taaValid = false;\n            }')
assert compositor.count('t.taa.push(texture(gl.RGBA8, w, h, gl.LINEAR));') == 1
assert 'var sources = [scene, t.e[1], t.depth,' in compositor
taa_shader = compositor[compositor.index('link("taa"'):compositor.index('link("compose"')]
assert '["u_src", "u_history", "u_depth"], ["u_taa", "u_taaRotation", "u_taaMove", "u_taaLens"]' in taa_shader
# World pixels go back by their depth, without this frame's jitter, to where
# the last camera saw them; the viewmodel stays and keeps at most 0.75; a
# point behind the last camera or off its screen keeps none; the history is
# clipped to the spread of the 3x3 neighbourhood in YCoCg.
assert '"  if (d >= 0.2) {\\n"' in taa_shader and '"    weight = max(weight, 0.25);\\n"' in taa_shader
assert ('"    highp vec3 p = u_taaRotation * vec3((v_uv * 2.0 - 1.0 - u_taaLens.zw) * z / u_taa.yz, z) + '
        'u_taaMove;\\n"') in taa_shader
assert '"    if (p.z <= 0.001) weight = 1.0;\\n"' in taa_shader
assert '"  if (any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0)))) weight = 1.0;\\n"' in taa_shader
assert '"  vec3 spread = max(sqrt(max(m2 * (1.0 / 9.0) - m1 * m1, 0.0)), vec3(0.004));\\n"' in taa_shader
assert '"  h = m1 + (k > 1.0 ? h / k : h);\\n"' in taa_shader
assert 'r_taa = ri.Dvar_RegisterBool("r_taa", 1, 0x2001);' in dvars
# Every 3D draw of the composed view moves by the jitter after its projection
# is converted for GL.
device_text = (root / 'src/Mac/DirectX_9/CDirect3DDevice.c').read_text()
assert device_text.index('MacOpenGLUtils_ConvertD3DProjectionMatrixToOpenGL(\n                    proj,') < \
    device_text.index('                WebSceneFx_JitterProjection(proj);\n') < \
    device_text.index('                glLoadMatrixf(proj);\n')
assert 'gl.uniform4f(C.u_clarity, clarity ? H[o + 100] : 0, 0, 0, 0);' in compositor
assert '"u_haze", "u_hazeUp", "u_hazeSun", "u_hazeSunColor", "u_hazeProj", "u_shaft", "u_blast", "u_blastDepth",\n                 "u_clarity", "u_tone"]' in compositor
clarity_js = compositor[compositor.index('            if (clarity) {\n'):compositor.index('            if (glow) {\n')]
assert clarity_js.index('gl.uniform2f(P.clarity.u_texel, 1 / w, 1 / h);\n                fx.texture(0, t.color);\n'
                        '                fx.pass(P.clarity, t.clFbo[0], qw, qh);') < \
    clarity_js.index('fx.texture(0, t.cl[0]);\n                fx.pass(P.dofBlur, t.clFbo[1], qw, qh);')
assert 'var spread = 5.3 * lines / 4 / 1.4142;' in clarity_js
assert 'gl.uniform2f(P.dofBlur.u_step, spread / qw, spread / qh);' in clarity_js
assert compositor.count('t.cl.push(texture(gl.RGBA8, t.qw, t.qh, gl.LINEAR));') == 1
clarity_shader = compositor[compositor.index('link("clarity"'):compositor.index('link("compose"')]
assert '"  o = vec4(vec3(0.25 * dot(c + d + e + f, vec3(0.2126, 0.7152, 0.0722))), 1.0);\\n}\\n"' in clarity_shader
clarity_at = compose_shader.index('if (u_clarity.x > 0.0 && !smeared) {')
assert compose_shader.index('if (u_film.x > 0.0 && !smeared) {') < clarity_at < \
    compose_shader.index('if (u_aoMix.x > 0.0 && texelFetch(u_depth, ivec2(gl_FragCoord.xy), 0).r >= 0.2)')
assert '"    float cb = texture(u_clarityMap, uv).r;\\n"' in compose_shader
assert ('"    col *= 1.0 + u_clarity.x * clamp((cl - cb) / max(cb, 0.05), -0.35, 0.35) * '
        '(1.0 - smoothstep(0.75, 1.0, cl));\\n"') in compose_shader
assert compose_shader.index('col *= texture(u_ao, v_uv + u_aoMix.yz).r;') < \
    compose_shader.index('if (u_dof2.x > 0.0 || u_dof2.w > 0.0 || u_lens.z > 0.0) {')
assert 'if (u_aoMix.x > 0.0 && texelFetch(u_depth, ivec2(gl_FragCoord.xy), 0).r >= 0.2)' in compose_shader
assert 'r_ssao = ri.Dvar_RegisterBool("r_ssao", 1, 0x2001);' in dvars
# The haze (r_atmosphere) reads its color and density (59-62), the view's up
# (63-65), the sun's direction and color (66-71), the projection (57-58) and
# its falloff with height (72). It hazes world pixels only, after the aim blur
# mixes in and before the scope's glass, the glow and the grade.
assert 'gl.uniform4f(C.u_haze, H[o + 59], H[o + 60], H[o + 61], haze ? H[o + 62] : 0);' in compositor
assert 'gl.uniform3f(C.u_hazeUp, H[o + 63], H[o + 64], H[o + 65]);' in compositor
assert 'gl.uniform3f(C.u_hazeSun, H[o + 66], H[o + 67], H[o + 68]);' in compositor
assert 'gl.uniform3f(C.u_hazeSunColor, H[o + 69], H[o + 70], H[o + 71]);' in compositor
assert 'gl.uniform3f(C.u_hazeProj, H[o + 57], H[o + 58], H[o + 72]);' in compositor
assert '"u_haze", "u_hazeUp", "u_hazeSun", "u_hazeSunColor", "u_hazeProj", "u_shaft", "u_blast", "u_blastDepth",' in compositor
haze_at = compose_shader.index('if (u_haze.a > 0.0) {')
assert compose_shader.index('if (u_dof2.x > 0.0 || u_dof2.w > 0.0 || u_lens.z > 0.0) {') < haze_at < \
    compose_shader.index('if (lensIn > 0.0 && u_lens.w < 1.0) {') < compose_shader.index('if (u_glow.x > 0.0)')
assert '"    if (hd >= 0.2 && hd < 0.99999) {\\n"' in compose_shader, 'the viewmodel and the sky keep their own'
assert '"      float haze = 1.0 - exp(-u_haze.a * hdist * clamp(hf, 0.25, 3.0));\\n"' in compose_shader
assert 'r_atmosphere = ri.Dvar_RegisterBool("r_atmosphere", 1, 0x2001);' in dvars
# The sun's shafts (r_sunShafts) read their strength (73), the sun's place on
# the screen (74-75), how low it stands (76) and its color (77-79). Two passes
# at a quarter of the scene's size need the resolved depth: the lit sky about
# the sun, then each texel's gathering toward it, with nothing past the
# screen's edge. The compose pass lays them over the glow and under the grade,
# by the air before each pixel, the viewmodel taking none; they are its ninth
# source, on unit 20, past the camouflage's 16 and 17 and the shadow and
# feather maps' 18 and 19.
assert 'r_sunShafts = ri.Dvar_RegisterFloat("r_sunShafts", 1.0f, 0.0f, 2.0f, 0x2001);' in dvars
assert 'var low = H[o + 76], S = P.shaftMask;' in compositor
assert 'gl.uniform4f(S.u_sun, H[o + 74], H[o + 75], w / h, 1 / (glowRadius * glowRadius));' in compositor
assert 'gl.uniform4f(P.shafts.u_ray, H[o + 74], H[o + 75], 0.95, mix(0.975, 0.98));' in compositor
assert 'gl.uniform4f(C.u_shaft, shafts ? H[o + 73] : 0, H[o + 77], H[o + 78], H[o + 79]);' in compositor
shafts_js = compositor[compositor.index('            if (shafts) {\n'):compositor.index('            var vmStart = H[o + 7]')]
assert shafts_js.index('fx.texture(0, t.color);\n                fx.texture(1, t.depth);\n'
                       '                fx.pass(S, t.shFbo[0], qw, qh);') < \
    shafts_js.index('fx.texture(0, t.sh[0]);\n                fx.pass(P.shafts, t.shFbo[1], qw, qh);')
assert compositor.count('t.sh.push(texture(gl.RGBA8, t.qw, t.qh, gl.LINEAR));') == 1
shaft_mask = compositor[compositor.index('link("shaftMask"'):compositor.index('link("shafts"')]
assert shaft_mask.count('step(0.99999, texelFetch(u_depth,') == 4, 'only the sky lights the shafts'
shaft_blur = compositor[compositor.index('link("shafts"'):compositor.index('link("dof"')]
assert '"    vec2 inside = step(vec2(0.0), p) * step(p, vec2(1.0));\\n"' in shaft_blur
assert '"    sum += texture(u_src, p).rgb * (w * inside.x * inside.y);\\n"' in shaft_blur
shafts_at = compose_shader.index('if (u_shaft.x > 0.0) {')
assert compose_shader.index('if (u_glow.x > 0.0)') < shafts_at < compose_shader.index('if (u_film.w > 0.0) {')
assert '"      (sd < 0.2 ? 0.0 : 1.0 - exp(-u_near.x / max(u_dof.x - sd, 0.000001) / 900.0));\\n"' in compose_shader
assert 'fx.unit = function(s) { return s < 8 ? s : s + 4; };' in compositor
assert 'gl.uniform1i(gl.getUniformLocation(program, samplers[s]), 8 + fx.unit(s));' in compositor
assert 'gl.activeTexture(gl.TEXTURE8 + fx.unit(unit));' in compositor
# Explosions' shockwaves (r_shockwave) read their four floats each from 80 and
# their depths from 96; the depth is resolved for them. Before anything else
# samples the scene, the compose pass moves where it samples by each front's
# lens, except for the viewmodel and surfaces well before the explosion; the
# sharpening samples about the moved point.
assert 'r_shockwave = ri.Dvar_RegisterBool("r_shockwave", 1, 0x2001);' in dvars
assert 'var blast = H[o + 96] > 0 || H[o + 97] > 0 || H[o + 98] > 0 || H[o + 99] > 0;' in compositor
assert 'gl.uniform4f(C.u_blastDepth, H[o + 96], H[o + 97], H[o + 98], H[o + 99]);' in compositor
assert 'if (blast) gl.uniform4fv(C.u_blast, H.subarray(o + 80, o + 96));' in compositor
assert compose_shader.index('"  highp vec2 uv = v_uv;\\n"') < compose_shader.index('if (u_blastDepth != vec4(0.0)) {') < \
    compose_shader.index('"  vec3 c = texture(u_scene, uv).rgb;\\n"')
assert compose_shader.count('texture(u_scene, v_uv).rgb') == 0
assert '"    highp float bz = bd < 0.2 ? 0.0 : u_near.x / max(u_dof.x - bd, 0.000001);\\n"' in compose_shader
assert '"      if (u_blastDepth[i] <= 0.0 || bz < 0.5 * u_blastDepth[i]) continue;\\n"' in compose_shader
assert compose_shader.count('texture(u_scene, uv + vec2(0.0, u_px.y)).rgb') == 1 and \
    compose_shader.count('texture(u_scene, uv - vec2(u_px.x, 0.0)).rgb') == 1
# The cgame hands every grenade, S-Mine, C4 and rocket blast over, with its
# radius, after the explosion's own effects and before its rumble.
events = (root / 'src/PC/cgame_mp/cg_event_mp.c').read_text()
assert events.count('#ifdef __EMSCRIPTEN__\n            CG_ExplosionShockwave((const vec_t *)position, es->weapon);\n#endif\n'
                    '#if COD2_FEATURE_RUMBLE\n            CG_RumbleExplosion((const vec_t *)position, es->weapon);\n#endif\n'
                    '            return;') == 3
assert ('    if (weapon > 0 && weapon <= BG_GetNumWeapons() && CG_EventWeaponDef(weapon)->iExplosionRadius > 0)\n'
        '        WebSceneFx_AddExplosion(origin, (float)CG_EventWeaponDef(weapon)->iExplosionRadius);') in events
# R_RenderScene hands over the map's exponential fog, under r_fog, and its sun.
assert scene.index('WebMaterials_SetSun(&s_world);') < scene.index('WebSceneFx_SetAtmosphere(fogColor, fogged ? fog->density : 0.0f,')
assert ('int fogged = rg_p->fogIndex && fog->registered && fog->techniqueOffset == 2 &&\n'
        '                         (*(const dvar_t **)imp_r_fog)->current.enabled;') in scene
assert ('const float fogColor[3] = {fog->color.array[2] / 255.0f, fog->color.array[1] / 255.0f,\n'
        '                                       fog->color.array[0] / 255.0f};') in scene
# And the scope's window after the viewmodel's motion: centre 42-43,
# half-axes 44-47, presence 48, open 49, blur 50, darkness 51.
assert 'var lensPresence = H[o + 48], lensBlur = H[o + 50];' in compositor
assert 'gl.uniform4f(C.u_lens, (H[o + 42] + 1) * 0.5 * w, (H[o + 43] + 1) * 0.5 * h, lensPresence, H[o + 49]);' in compositor
assert 'var ax = H[o + 44], ay = H[o + 45], bx = H[o + 46], by = H[o + 47];' in compositor
assert 'gl.uniform4f(C.u_lensFx, lensBlur, H[o + 51],' in compositor
# A scope's glass, drawn with the rifle's projection (52), comes in with its
# drawn eyepiece; a sight's window has none.
assert 'gl.uniform4f(C.u_lensFx, lensBlur, H[o + 51], Math.min(1.5 / Math.max(radius, 1), 0.5),\n' \
    '                    H[o + 52] > 0 ? lensPresence : 0);' in compositor
# The stencil marks the outline the rifle is kept off about the glass after
# the glass's own, when it has area.
stencil = compositor[compositor.index('EM_JS(void, WebSceneFx_ScopeStencil'):compositor.index('EM_JS(void, WebSceneFx_ScopeMask')]
assert stencil.index('gl.drawArrays(gl.TRIANGLE_FAN, 0, 66);\n'
                     '    if (HEAPF32[b + 2] * HEAPF32[b + 5] - HEAPF32[b + 4] * HEAPF32[b + 3] !== 0) {') < \
    stencil.index('gl.uniform2f(L.u_center, HEAPF32[b], HEAPF32[b + 1]);') < \
    stencil.index('gl.uniformMatrix2fv(L.u_axes, false, HEAPF32.subarray(b + 2, b + 6));') < \
    stencil.index('gl.stencilMask(0);')
# Its fan writes no alpha, so it marks the multisampled glass only with alpha
# to coverage off, which a foliage draw just before the rifle's leaves on
# (web_effects.c); the effects' state cache expects it back as it was.
stencil_caps = re.search(r'var caps = \[([^\]]*)\];', stencil)
assert stencil_caps and 'gl.SAMPLE_ALPHA_TO_COVERAGE' in stencil_caps[1], 'the stencil draws without alpha to coverage'
assert stencil.index('enabled.push(gl.isEnabled(caps[c]));\n        gl.disable(caps[c]);') < \
    stencil.index('gl.drawArrays(gl.TRIANGLE_FAN, 0, 66);') < \
    stencil.index('if (enabled[r]) gl.enable(caps[r]);')
assert 'if (cover) GLctx.enable(GLctx.SAMPLE_ALPHA_TO_COVERAGE);' in (root / 'downstream/wasm/web_effects.c').read_text()
# The eyepiece is laid over the glass's reflection, under the glow, the grade
# and a held breath's blackout.
assert compose_shader.index('col = mix(col, glass, lensIn * (1.0 - u_lens.w));') < \
    compose_shader.index('if (u_lens.z > 0.0 && u_lensFx.w > 0.0) {') < compose_shader.index('if (u_glow.x > 0.0)') < \
    compose_shader.index('col *= 1.0 - u_lensFx.y * (1.0 - lensIn);')
# The view weapon's draws, and only those, get its projection and the
# stencil that keeps it off the glass; every way out of the depth hack
# lifts the stencil, and only leaving for another entity restores the
# projection by hand (popping the matrix stack restores it otherwise).
rb_state = (root / 'src/PC/gfx_d3d/rb_state.c').read_text()
assert ('    projection->_11 = scale ? scale[0] : view[0];\n    projection->_22 = scale ? scale[1] : view[5];\n'
        '    RB_SetDepthHackNearClip(nearClip);\n') in rb_state
enter = backend.index('            if (depthRange == 8) {\n')
assert backend.index('RB_SetViewmodelProjection(WebSceneFx_ViewmodelNearClip(backEnd.viewParms->depthHackNearClip),\n'
                     '                                          WebSceneFx_ViewmodelProjection());', enter) < \
    backend.index('WebSceneFx_BeginViewmodel();', enter) < backend.index('RB_SetDepthRange(0.0f, 0.2f);', enter)
assert backend.count('WebSceneFx_BeginViewmodel();') == 1
# Each material's draws begin once the last's have ended, the compositor told which material it is.
assert backend.count('    if (tess.indexCount || tess.optimizedIndexCount)\n        RB_EndSurface();\n\n'
                     '    if (materialTime != materialTimePrev') == 1
assert backend.index('    if (materialTime != materialTimePrev') < \
    backend.index('    WebSceneFx_ViewmodelMaterial(material->info.name);\n#endif\n'
                  '    RB_BeginSurface(material, actualTechType, lightmap);\n')
assert backend.count('                WebSceneFx_EndViewmodel();\n                RB_SetViewmodelProjection(nc, NULL);\n') == 1
assert backend.count('            WebSceneFx_EndViewmodel();\n            RB_SetViewmodelProjection(nc, NULL);\n') == 1
world = backend.index('        if (!isWorldSpaceEntPrev)\n            RB_PopMatrixStack();\n        if (depthRangePrev) {\n')
assert backend.index('            if (depthRangePrev == 8)\n                WebSceneFx_EndViewmodel();\n', world) < \
    backend.index('            RB_SetDepthRange(0.0f, 1.0f);\n            depthRangePrev = 0;', world)
assert 'WebSceneFx_SetScope(g_webScopeLens);' in scene and \
    scene.index('WebSceneFx_SetBackdrop(g_webBackdropFrac') < scene.index('WebSceneFx_SetScope(g_webScopeLens);')
# The cgame's lens and the floats R_RenderScene hands over are the same
# nineteen: the compositor's thirteen scope floats, then the outline about
# the glass.
lens_struct = (root / 'src/headers/PC/cgame_mp/cg_wwiiscope.h').read_text()
lens_struct = lens_struct[lens_struct.index('typedef struct CgWwiiScopeLens {'):lens_struct.index('} CgWwiiScopeLens;')]
lens_floats = 0
for dims in re.findall(r'^\s+float \w+((?:\[\d+\])*);$', lens_struct, re.M):
    count = 1
    for dim in re.findall(r'\[(\d+)\]', dims):
        count *= int(dim)
    lens_floats += count
assert lens_floats == 19, lens_floats
assert 'extern float g_webScopeLens[19];' in scene
assert 'float g_webScopeLens[19];' in (root / 'src/PC/cgame_mp/cg_wwiiscope_mp.c').read_text()

emscripten = '#define EM_JS(ret, name, params, ...) ret name params;\ndouble emscripten_get_now(void);\n'
common_types = r'''
typedef union {int enabled; int integer; float value;} DvarValue;
typedef struct {DvarValue current;} dvar_t;
'''
test = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "SOURCE"
static dvar_t glow, radius0, radius1, intensity0, intensity1, cutoff, desaturation, fullbright, samples, dof, film,
    motion, scene, ssao, atmosphere, sunShafts, shockwave, clarity, taa;
const dvar_t *r_glow = &glow;
const dvar_t *r_glowRadius[2] = {&radius0, &radius1};
const dvar_t *r_glowBloomIntensity[2] = {&intensity0, &intensity1};
const dvar_t *r_glowBloomCutoff = &cutoff;
const dvar_t *r_glowBloomDesaturation = &desaturation;
const dvar_t *r_fullbright = &fullbright;
const dvar_t *r_aaSamples = &samples;
const dvar_t *r_dof_enable = &dof;
const dvar_t *r_filmTweakEnable = &film;
const dvar_t *r_motionBlur = &motion;
const dvar_t *r_sceneHeight = &scene;
const dvar_t *r_ssao = &ssao;
const dvar_t *r_atmosphere = &atmosphere;
const dvar_t *r_sunShafts = &sunShafts;
const dvar_t *r_shockwave = &shockwave;
/* Off unless a check turns them on, so the checks of every other effect alone hold. */
const dvar_t *r_clarity = &clarity;
const dvar_t *r_taa = &taa;
static int shadowRenders, bindsAtShadowRender;
static double clockMs = 1000.0;
double emscripten_get_now(void)
{
    return clockMs;
}
static int binds, composes, bindResult = 1, boundWidth, boundHeight, boundSamples, boundScreenWidth, boundScreenHeight;
static float composed[SCENE_FX_PARAM_COUNT];
/* The sun's shadow map draws at the frame's first full-screen view, before
 * the view binds its target. */
void WebSunShadows_Render(void)
{
    ++shadowRenders;
    bindsAtShadowRender = binds;
}
int WebSceneFx_Bind(int width, int height, int sampleCount, int screenWidth, int screenHeight)
{
    ++binds; boundWidth = width; boundHeight = height; boundSamples = sampleCount;
    boundScreenWidth = screenWidth; boundScreenHeight = screenHeight;
    return bindResult;
}
void WebSceneFx_Compose(const float *params, const float *mesh)
{
    ++composes;
    memcpy(composed, params, sizeof(composed));
    assert(mesh == s_vmMesh);
}
static int stencils, masks, maskOn;
static float stenciled[SCENE_FX_SCOPE_FLOATS], stenciledBell[SCENE_FX_SCOPE_BELL_FLOATS];
void WebSceneFx_ScopeStencil(const float *scope, const float *bell)
{
    ++stencils;
    memcpy(stenciled, scope, sizeof(stenciled));
    memcpy(stenciledBell, bell, sizeof(stenciledBell));
}
void WebSceneFx_ScopeMask(int on)
{
    ++masks;
    maskOn = on;
}
static void defaults(void)
{
    glow.current.enabled = 1; radius0.current.value = 5; radius1.current.value = 12;
    intensity0.current.value = 1; intensity1.current.value = 1; cutoff.current.value = 0.9f;
    desaturation.current.value = 0.75f; fullbright.current.enabled = 0; samples.current.integer = 4;
    dof.current.enabled = 1; film.current.enabled = 1; motion.current.enabled = 1; scene.current.integer = 720;
    ssao.current.enabled = 1; atmosphere.current.enabled = 1; sunShafts.current.value = 1; shockwave.current.enabled = 1;
}
static void frameNear(int x, int width, float nearClip, float worldNearClip)
{
    WebSceneFx_BeginView(x, 0, width, 720, 1280, 720, nearClip, worldNearClip);
    WebSceneFx_EndView();
    WebSceneFx_EndFrame();
}
static void frame(int x, int width, float nearClip)
{
    frameNear(x, width, nearClip, 4.0f);
}
/* A D3D view (row vectors) of CoD's x-forward, y-left, z-up world, with
 * pitch positive down like the game's angles, at the game's millisecond the
 * clock is in. */
static float lens[16] = {1.19f, 0, 0, 0, 0, 2.11f, 0, 0, 0, 0, 0.9995f, 1, 0, 0, -3.998f, 0};
static float lastView[16], view[16];
static void camera(float yaw, float pitch, float x, float y, float z)
{
    const float f[3] = {cosf(pitch) * cosf(yaw), cosf(pitch) * sinf(yaw), -sinf(pitch)};
    const float l[3] = {-sinf(yaw), cosf(yaw), 0};
    const float u[3] = {sinf(pitch) * cosf(yaw), sinf(pitch) * sinf(yaw), cosf(pitch)};
    const float o[3] = {x, y, z};
    memcpy(lastView, view, sizeof(view));
    memset(view, 0, sizeof(view));
    for (int i = 0; i < 3; ++i) {
        view[i * 4] = -l[i]; view[i * 4 + 1] = u[i]; view[i * 4 + 2] = f[i];
        view[12] += l[i] * o[i]; view[13] -= u[i] * o[i]; view[14] -= f[i] * o[i];
    }
    view[15] = 1;
    WebSceneFx_SetCamera(view, lens, (int)clockMs);
}
static float moved(float ms, float yaw, float pitch, float x, float y, float z)
{
    clockMs += ms;
    camera(yaw, pitch, x, y, z);
    frame(0, 1280, 0.1f);
    return composed[SCENE_FX_MOTION];
}
/* How far the camera's move over the exposure takes it. */
static float exposureMove(void)
{
    const float *m = composed + SCENE_FX_MOTION_MOVE;
    return composed[SCENE_FX_MOTION] * sqrtf(m[0] * m[0] + m[1] * m[1] + m[2] * m[2]);
}
static void viewPoint(const float *matrix, const float *p, float *out)
{
    for (int j = 0; j < 3; ++j)
        out[j] = matrix[12 + j] + p[0] * matrix[j] + p[1] * matrix[4 + j] + p[2] * matrix[8 + j];
}
/* The composed rotation and offset take what this camera sees of a world
 * point to what the camera then saw of it. */
static void assertReprojects(const float *then, const float *p)
{
    float now[3], before[3];
    viewPoint(view, p, now);
    viewPoint(then, p, before);
    for (int j = 0; j < 3; ++j) {
        float back = composed[SCENE_FX_MOTION_MOVE + j];
        for (int k = 0; k < 3; ++k)
            back += composed[SCENE_FX_MOTION_ROTATION + k * 3 + j] * now[k];
        assert(fabsf(back - before[j]) < 2e-3f * (1.0f + fabsf(before[j])));
    }
}
int main(void)
{
    const float defaultWorldNearClip = 0.9995002746582031f * 4.0f;
    defaults();
    for (int i = 0; i < 600; ++i) {
        binds = composes = shadowRenders = 0;
        WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 7.996f);
        assert(binds == 1 && boundWidth == 1280 && boundHeight == 720 && boundSamples == 4 && !composes);
        assert(shadowRenders == 1 && bindsAtShadowRender == 0);
        assert(boundScreenWidth == 1280 && boundScreenHeight == 720);
        /* RB_Set2D composes once; later 2D batches and views draw directly. */
        WebSceneFx_EndView(); WebSceneFx_EndView();
        WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 2.0f);
        WebSceneFx_EndView();
        WebSceneFx_EndFrame();
        assert(binds == 1 && composes == 1 && shadowRenders == 1);
    }
    assert(fabsf(composed[SCENE_FX_GLOW0] - 0.42f) < 1e-6f && fabsf(composed[SCENE_FX_GLOW1] - 0.30f) < 1e-6f);
    assert(composed[SCENE_FX_CUTOFF] == 0.9f && composed[SCENE_FX_DESATURATION] == 0.75f);
    assert(composed[SCENE_FX_RADIUS0] == 5 && composed[SCENE_FX_RADIUS1] == 12);
    assert(composed[SCENE_FX_ADS] == 0 && composed[SCENE_FX_NEAR_CLIP] == 0.1f && composed[SCENE_FX_NEAR_BLUR] == 3);
    assert(composed[SCENE_FX_SHARPEN] > 0 && composed[SCENE_FX_FILM] == 1);
    /* The composed view's own projection linearizes the world; until the
     * cgame aims, the near world blur reaches 512 units at full strength. */
    assert(composed[SCENE_FX_WORLD_NEAR_CLIP] == 7.996f);
    assert(composed[SCENE_FX_NEAR_END] == 512 && composed[SCENE_FX_WORLD_BLUR] == 1);

    /* A view left open without a 2D pass still composes before Present. */
    binds = composes = 0;
    WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 4.0f);
    WebSceneFx_EndFrame();
    assert(binds == 1 && composes == 1);
    /* A second full-screen view composes the first before drawing directly. */
    binds = composes = 0;
    WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 4.0f);
    WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 4.0f);
    assert(binds == 1 && composes == 1);
    WebSceneFx_EndFrame();
    assert(composes == 1);

    /* Partial views (cg_viewsize, split views) keep the original direct path
     * and draw no shadow map. */
    binds = composes = shadowRenders = 0;
    frame(8, 1264, 0.1f);
    frame(0, 1024, 0.1f);
    assert(!binds && !composes && !shadowRenders);

    /* r_fullbright clears before its view begins; it renders directly. */
    fullbright.current.enabled = 1;
    frame(0, 1280, 0.1f);
    assert(!binds && !composes);
    fullbright.current.enabled = 0;

    /* With every effect and MSAA off nothing changes, until the player aims. */
    glow.current.enabled = 0; film.current.enabled = 0; samples.current.integer = 1; ssao.current.enabled = 0;
    WebSceneFx_SetAds(0, 6, 15, 512);
    frame(0, 1280, 0.1f);
    assert(!binds && !composes);
    WebSceneFx_SetAds(0.5f, 6, 15, 170);
    frame(0, 1280, 0.1f);
    assert(binds == 1 && composes == 1 && boundSamples == 1);
    assert(composed[SCENE_FX_GLOW0] == 0 && composed[SCENE_FX_GLOW1] == 0 && composed[SCENE_FX_SHARPEN] == 0);
    assert(composed[SCENE_FX_FILM] == 0 && composed[SCENE_FX_CONTRAST] == 0 && composed[SCENE_FX_ADS] == 0.5f);
    assert(composed[SCENE_FX_NEAR_END] == 170);
    dof.current.enabled = 0;
    frame(0, 1280, 0.1f);
    assert(binds == 1 && composes == 1);
    defaults();
    frame(0, 1280, 0.1f);
    assert(binds == 2 && composes == 2 && composed[SCENE_FX_ADS] == 0.5f);
    /* r_dof_enable 0 keeps aiming sharp while the other effects remain. */
    dof.current.enabled = 0;
    frame(0, 1280, 0.1f);
    assert(composes == 3 && composed[SCENE_FX_ADS] == 0);
    defaults();

    /* Glow follows the original r_glow* dvars and stays bounded. */
    intensity0.current.value = 2; intensity1.current.value = NAN; cutoff.current.value = 1.5f;
    desaturation.current.value = -1; radius0.current.value = INFINITY; radius1.current.value = 0;
    frame(0, 1280, NAN);
    assert(fabsf(composed[SCENE_FX_GLOW0] - 0.84f) < 1e-6f && composed[SCENE_FX_GLOW1] == 0);
    assert(composed[SCENE_FX_CUTOFF] == 0.99f && composed[SCENE_FX_DESATURATION] == 0);
    assert(composed[SCENE_FX_RADIUS0] == 0 && composed[SCENE_FX_RADIUS1] == 0);
    assert(composed[SCENE_FX_NEAR_CLIP] == 0.1f);
    defaults();
    frame(0, 1280, 0.25f);
    assert(composed[SCENE_FX_NEAR_CLIP] == 0.25f);
    /* A projection without a usable near plane falls back to r_znear 4. */
    frameNear(0, 1280, 0.1f, NAN);
    assert(composed[SCENE_FX_WORLD_NEAR_CLIP] == defaultWorldNearClip);
    frameNear(0, 1280, 0.1f, -3.998f);
    assert(composed[SCENE_FX_WORLD_NEAR_CLIP] == defaultWorldNearClip);
    frameNear(0, 1280, 0.1f, 0.4f);
    assert(composed[SCENE_FX_WORLD_NEAR_CLIP] == 0.4f);

    /* Focus ranges are clamped to valid, non-empty intervals; without a
     * finite near world focus nothing past the eye blurs. */
    WebSceneFx_SetAds(NAN, NAN, NAN, NAN);
    frame(0, 1280, 0.1f);
    assert(composed[SCENE_FX_ADS] == 0 && composed[SCENE_FX_VM_END] == 2 && composed[SCENE_FX_VM_START] == 0);
    assert(composed[SCENE_FX_NEAR_END] == 1);
    WebSceneFx_SetAds(4, 30, 12, 5000);
    frame(0, 1280, 0.1f);
    assert(composed[SCENE_FX_ADS] == 1 && composed[SCENE_FX_VM_END] == 12 && composed[SCENE_FX_VM_START] == 11.5f);
    assert(composed[SCENE_FX_NEAR_END] == 1024);
    WebSceneFx_SetAds(0.25f, 5, 500, 0.5f);
    frame(0, 1280, 0.1f);
    assert(composed[SCENE_FX_ADS] == 0.25f && composed[SCENE_FX_VM_END] == 64 && composed[SCENE_FX_VM_START] == 5);
    assert(composed[SCENE_FX_NEAR_END] == 1);

    /* A failed offscreen target never composes stale state. */
    bindResult = 0; composes = 0;
    for (int i = 0; i < 10; ++i)
        frame(0, 1280, 0.1f);
    assert(!composes);
    bindResult = 1;
    frame(0, 1280, 0.1f);
    assert(composes == 1);

    /* Motion blur spans 16 ms of the game's time: the camera's turn and move
     * since the newest view at least that old, else the oldest since the
     * last cut. */
    defaults();
    WebSceneFx_SetAds(0, 6, 15, 512);
    clockMs = 5000;
    camera(0.3f, 0.1f, 100, 200, 60);
    frame(0, 1280, 0.1f);
    assert(composed[SCENE_FX_MOTION] == 0);
    /* 10 ms on, only the last view is older: 1.6 times its motion. */
    assert(moved(10, 0.36f, 0.07f, 101.5f, 198.8f, 60.5f) == 1.6f);
    const float points[][3] = {{400, 260, 80}, {130, 180, 40}, {-500, 900, 300}, {100000, 30000, 2000}};
    for (int i = 0; i < 4; ++i)
        assertReprojects(lastView, points[i]);
    assert(composed[SCENE_FX_MOTION_PROJECTION] == 1.19f && composed[SCENE_FX_MOTION_PROJECTION + 1] == 2.11f);
    /* 10 ms later the view 20 ms before is the newest that old: 0.8 times
     * the motion since it. */
    {
        float first[16];
        memcpy(first, lastView, sizeof(first));
        assert(moved(10, 0.38f, 0.07f, 103.4f, 198.8f, 60.5f) == 0.8f);
        for (int i = 0; i < 4; ++i)
            assertReprojects(first, points[i]);
    }
    /* At 250 frames a second it is the view four frames before: 16 ms of
     * motion as it is, 3.04 units of a walk at 190 a second. */
    {
        float fourBack[16];
        for (int i = 1; i <= 5; ++i) {
            moved(4, 0.38f + 0.004f * (float)i, 0.07f, 103.4f + 0.76f * (float)i, 198.8f, 60.5f);
            if (i == 1)
                memcpy(fourBack, view, sizeof(fourBack));
        }
        assert(composed[SCENE_FX_MOTION] == 1.0f && fabsf(exposureMove() - 3.04f) < 0.01f);
        for (int i = 0; i < 4; ++i)
            assertReprojects(fourBack, points[i]);
    }
    /* At 25 a second, 0.4 times the last frame's 40 ms. */
    assert(moved(40, 0.42f, 0.07f, 107.2f, 198.8f, 60.5f) == 0.4f);
    /* Frames bunched within the game's milliseconds, up to five in one as a
     * small room draws them, with a stall of 19 ms now and then: each shows
     * its millisecond's moment, and walking at 190 units a second blurs 3.04
     * units every frame, never none and never twice that. The blur spans at
     * least 16 ms, and a frame in the same millisecond as the last blurs as
     * it did. */
    {
        const double start = clockMs + 1000.0;
        int repeats = 0;
        clockMs = start;
        for (int i = 0; i < 240; ++i) {
            float before[13];
            const int last = (int)clockMs;
            memcpy(before, composed + SCENE_FX_MOTION, sizeof(before));
            clockMs += i % 60 == 59 ? 19.0 : 0.13 + 0.12 * (double)(i % 3);
            camera(0.5f, 0.07f, 100.0f + 0.19f * (float)((int)clockMs - (int)start), 198.8f, 60.5f);
            frame(0, 1280, 0.1f);
            if (clockMs - start < 17.0)
                continue;
            assert(fabsf(exposureMove() - 3.04f) < 0.01f);
            assert(composed[SCENE_FX_MOTION] <= 1.0f && composed[SCENE_FX_MOTION] > 16.0f / 35.0f);
            if ((int)clockMs == last) {
                assert(!memcmp(before, composed + SCENE_FX_MOTION, sizeof(before)));
                ++repeats;
            }
        }
        assert(repeats > 100);
    }
    /* 16 ms apart, at 60 frames a second, each frame blurs the motion since
     * the last as it is. Raising the sights fades it out. */
    assert(moved(1000, 0.40f, 0.07f, 103.4f, 198.8f, 60.5f) == 0);
    assert(moved(16, 0.42f, 0.07f, 103.4f, 198.8f, 60.5f) == 1.0f);
    WebSceneFx_SetAim(0.25f);
    assert(moved(16, 0.44f, 0.07f, 103.4f, 198.8f, 60.5f) == 0.75f);
    WebSceneFx_SetAim(1);
    assert(moved(16, 0.46f, 0.07f, 103.4f, 198.8f, 60.5f) == 0);
    WebSceneFx_SetAim(NAN);
    assert(moved(16, 0.48f, 0.07f, 103.4f, 198.8f, 60.5f) == 1.0f);
    WebSceneFx_SetAim(0);
    /* Cuts start over: a turn past 0.6 rad, a move of 48 units, a gap past
     * 100 ms or one back in time; the frame after one blurs from it alone. A
     * 0.5 rad turn, a fast spin's frame at 60 fps, still blurs. */
    assert(moved(16, 1.14f, 0.07f, 103.4f, 198.8f, 60.5f) == 0);
    assert(moved(16, 0.64f, 0.07f, 103.4f, 198.8f, 60.5f) == 1.0f);
    assert(moved(16, 1.18f, 0.07f, 153.4f, 198.8f, 60.5f) == 0);
    assert(moved(16, 1.18f, 0.07f, 193.4f, 198.8f, 60.5f) == 1.0f);
    assert(moved(150, 1.20f, 0.07f, 193.4f, 198.8f, 60.5f) == 0);
    assert(moved(16, 1.22f, 0.07f, 193.4f, 198.8f, 60.5f) == 1.0f);
    assert(moved(-5, 1.24f, 0.07f, 193.4f, 198.8f, 60.5f) == 0);
    assert(moved(16, 1.26f, 0.07f, 193.4f, 198.8f, 60.5f) == 1.0f);
    /* A frame in the same millisecond is no cut. */
    assert(moved(0, 1.26f, 0.07f, 193.4f, 198.8f, 60.5f) == 1.0f);
    /* A camera standing still or drifting adds nothing. */
    assert(moved(16, 1.26f, 0.07f, 193.4f, 198.8f, 60.5f) == 0);
    assert(moved(16, 1.2601f, 0.07f, 193.4f, 198.8f, 60.5f) == 0);
    assert(moved(16, 1.2606f, 0.07f, 193.4f, 198.8f, 60.5f) == 1.0f);
    assert(moved(16, 1.2606f, 0.07f, 193.42f, 198.8f, 60.5f) == 0);
    assert(moved(16, 1.2606f, 0.07f, 193.52f, 198.8f, 60.5f) == 1.0f);
    /* r_motionBlur 0 draws none and keeps tracking. */
    motion.current.enabled = 0;
    assert(moved(16, 1.30f, 0.07f, 193.52f, 198.8f, 60.5f) == 0);
    motion.current.enabled = 1;
    assert(moved(16, 1.32f, 0.07f, 193.52f, 198.8f, 60.5f) == 1.0f);
    /* A broken camera is no reference for the next one. */
    assert(moved(16, NAN, 0.07f, 193.52f, 198.8f, 60.5f) == 0);
    assert(moved(16, 1.34f, 0.07f, 193.52f, 198.8f, 60.5f) == 0 && !s_viewContinuous);
    assert(moved(16, 1.36f, 0.07f, 193.52f, 198.8f, 60.5f) == 1.0f);
    lens[0] = 0;
    assert(moved(16, 1.38f, 0.07f, 193.52f, 198.8f, 60.5f) == 0);
    lens[0] = 1.19f;
    assert(moved(16, 1.40f, 0.07f, 193.52f, 198.8f, 60.5f) == 0 && !s_viewContinuous);
    assert(moved(16, 1.42f, 0.07f, 193.52f, 198.8f, 60.5f) == 1.0f);
    /* With every other effect off, only a moving camera composes. */
    glow.current.enabled = 0; film.current.enabled = 0; samples.current.integer = 1; ssao.current.enabled = 0;
    binds = composes = 0;
    moved(16, 1.42f, 0.07f, 193.52f, 198.8f, 60.5f);
    assert(!binds && !composes);
    moved(16, 1.44f, 0.07f, 193.52f, 198.8f, 60.5f);
    assert(binds == 1 && composes == 1 && boundSamples == 1 && composed[SCENE_FX_MOTION] == 1.0f);
    assert(composed[SCENE_FX_GLOW0] == 0 && composed[SCENE_FX_SHARPEN] == 0 && composed[SCENE_FX_ADS] == 0);
    WebSceneFx_SetAim(1);
    moved(16, 1.46f, 0.07f, 193.52f, 198.8f, 60.5f);
    WebSceneFx_SetAim(0);
    motion.current.enabled = 0;
    moved(16, 1.48f, 0.07f, 193.52f, 198.8f, 60.5f);
    motion.current.enabled = 1;
    fullbright.current.enabled = 1;
    moved(16, 1.50f, 0.07f, 193.52f, 198.8f, 60.5f);
    fullbright.current.enabled = 0;
    assert(binds == 1 && composes == 1);
    /* Fullbright frames keep tracking; a partial view does not, and the next
     * full view measures from the last full one, 32 ms before. */
    assert(moved(16, 1.52f, 0.07f, 193.52f, 198.8f, 60.5f) == 1.0f && binds == 2);
    clockMs += 16;
    camera(1.54f, 0.07f, 193.52f, 198.8f, 60.5f);
    frame(8, 1264, 0.1f);
    assert(binds == 2);
    assert(moved(16, 1.56f, 0.07f, 193.52f, 198.8f, 60.5f) == 0.5f && binds == 3);

    /* A menu's blurred backdrop composes on its own, whatever the other
     * effects, and stays between none and all of it. */
    assert(SCENE_FX_BACKDROP == 33 && SCENE_FX_BACKDROP_GRAY == 34 && SCENE_FX_PARAM_COUNT == 119);
    defaults();
    glow.current.enabled = 0; film.current.enabled = 0; samples.current.integer = 1; dof.current.enabled = 0;
    motion.current.enabled = 0; ssao.current.enabled = 0;
    binds = composes = 0;
    frame(0, 1280, 0.1f);
    assert(!binds && !composes);
    WebSceneFx_SetBackdrop(0.6f);
    frame(0, 1280, 0.1f);
    assert(binds == 1 && composes == 1 && composed[SCENE_FX_BACKDROP] == 0.6f && composed[SCENE_FX_ADS] == 0);
    assert(composed[SCENE_FX_BACKDROP_GRAY] == 0);
    WebSceneFx_SetBackdrop(4);
    frame(0, 1280, 0.1f);
    assert(composes == 2 && composed[SCENE_FX_BACKDROP] == 1);
    WebSceneFx_SetBackdrop(NAN);
    frame(0, 1280, 0.1f);
    WebSceneFx_SetBackdrop(-1);
    frame(0, 1280, 0.1f);
    assert(composes == 2);
    /* Its gray composes on its own too, within the same bounds. */
    WebSceneFx_SetBackdropGray(0.6f);
    frame(0, 1280, 0.1f);
    assert(composes == 3 && composed[SCENE_FX_BACKDROP_GRAY] == 0.6f && composed[SCENE_FX_BACKDROP] == 0);
    WebSceneFx_SetBackdrop(1);
    WebSceneFx_SetBackdropGray(4);
    frame(0, 1280, 0.1f);
    assert(composes == 4 && composed[SCENE_FX_BACKDROP_GRAY] == 1 && composed[SCENE_FX_BACKDROP] == 1);
    WebSceneFx_SetBackdrop(0);
    WebSceneFx_SetBackdropGray(NAN);
    frame(0, 1280, 0.1f);
    WebSceneFx_SetBackdropGray(-1);
    frame(0, 1280, 0.1f);
    assert(composes == 4);
    defaults();
    frame(0, 1280, 0.1f);
    assert(composes == 5 && composed[SCENE_FX_BACKDROP] == 0 && composed[SCENE_FX_BACKDROP_GRAY] == 0);
    /* The countdown before a match grays the view on its own as well, the
     * larger gray winning, and lifts the film grade's contrast with it, in
     * place of the lift of the shade it gives a full-color view. */
    glow.current.enabled = 0; film.current.enabled = 0; samples.current.integer = 1; dof.current.enabled = 0;
    motion.current.enabled = 0; ssao.current.enabled = 0;
    binds = composes = 0;
    WebSceneFx_SetGray(0.6f);
    frame(0, 1280, 0.1f);
    assert(binds == 1 && composes == 1 && composed[SCENE_FX_BACKDROP_GRAY] == 0.6f && composed[SCENE_FX_CONTRAST] == 0);
    WebSceneFx_SetBackdropGray(0.8f);
    frame(0, 1280, 0.1f);
    assert(composes == 2 && composed[SCENE_FX_BACKDROP_GRAY] == 0.8f);
    WebSceneFx_SetBackdropGray(0.2f);
    frame(0, 1280, 0.1f);
    assert(composes == 3 && composed[SCENE_FX_BACKDROP_GRAY] == 0.6f);
    WebSceneFx_SetBackdropGray(0);
    film.current.enabled = 1;
    WebSceneFx_SetGray(4);
    frame(0, 1280, 0.1f);
    assert(composes == 4 && composed[SCENE_FX_BACKDROP_GRAY] == 1 && fabsf(composed[SCENE_FX_CONTRAST] - 1.2f) < 1e-6f);
    assert(composed[SCENE_FX_TONE] == 0);
    film.current.enabled = 0;
    WebSceneFx_SetGray(NAN);
    frame(0, 1280, 0.1f);
    WebSceneFx_SetGray(-1);
    frame(0, 1280, 0.1f);
    assert(composes == 4);
    defaults();
    frame(0, 1280, 0.1f);
    assert(composes == 5 && composed[SCENE_FX_BACKDROP_GRAY] == 0 && composed[SCENE_FX_CONTRAST] == 0);
    assert(composed[SCENE_FX_TONE] == SCENE_FX_SHADE_LIFT && composed[SCENE_FX_SHARPEN] == 0.1f);
    /* Without the contrast curve the grade keeps its color by saturation. */
    assert(composed[SCENE_FX_SATURATION] == 1.12f);
    /* At gray 0.6 the grade lifts 0.4 of the shade and takes 0.72 contrast. */
    WebSceneFx_SetGray(0.6f);
    frame(0, 1280, 0.1f);
    assert(composes == 6 && fabsf(composed[SCENE_FX_TONE] - 0.4f * SCENE_FX_SHADE_LIFT) < 1e-6f);
    assert(fabsf(composed[SCENE_FX_CONTRAST] - 0.72f) < 1e-6f);
    WebSceneFx_SetGray(0);
    frame(0, 1280, 0.1f);
    assert(composes == 7 && composed[SCENE_FX_TONE] == SCENE_FX_SHADE_LIFT && composed[SCENE_FX_CONTRAST] == 0);
    film.current.enabled = 0;
    frame(0, 1280, 0.1f);
    assert(composes == 8 && composed[SCENE_FX_TONE] == 0 && composed[SCENE_FX_SHARPEN] == 0);
    defaults();

    /* A screen taller than r_sceneHeight renders its scene at that many lines
     * and the screen's aspect, even with every effect off; a shorter one, or
     * r_sceneHeight 0, keeps the screen's size. */
    static const int sizes[][5] = {
        {1920, 1080, 720, 1280, 720}, {1600, 900, 720, 1280, 720}, {1366, 768, 720, 1281, 720},
        {2560, 1600, 720, 1152, 720}, {3840, 2160, 1080, 1920, 1080}, {100, 2000, 1, 1, 1},
        {1920, 1080, 0, 1920, 1080}, {1280, 720, 720, 1280, 720}, {1024, 768, 900, 1024, 768}};
    for (int effects = 0; effects < 2; ++effects) {
        for (unsigned k = 0; k < sizeof(sizes) / sizeof(sizes[0]); ++k) {
            const int *z = sizes[k];
            int scaled = z[3] != z[0] || z[4] != z[1];
            defaults();
            if (!effects) {
                glow.current.enabled = 0; film.current.enabled = 0; samples.current.integer = 1; dof.current.enabled = 0;
                motion.current.enabled = 0; ssao.current.enabled = 0;
            }
            scene.current.integer = z[2];
            binds = composes = 0;
            boundWidth = boundHeight = boundScreenWidth = boundScreenHeight = 0;
            WebSceneFx_BeginView(0, 0, z[0], z[1], z[0], z[1], 0.1f, 4.0f);
            WebSceneFx_EndView();
            WebSceneFx_EndFrame();
            if (!effects && !scaled) {
                assert(!binds && !composes);
                continue;
            }
            assert(binds == 1 && composes == 1 && boundWidth == z[3] && boundHeight == z[4]);
            assert(boundScreenWidth == z[0] && boundScreenHeight == z[1] && boundSamples == (effects ? 4 : 1));
        }
    }
    /* Partial views on a taller screen still draw directly. */
    defaults();
    binds = composes = 0;
    WebSceneFx_BeginView(8, 0, 1904, 1080, 1920, 1080, 0.1f, 4.0f);
    WebSceneFx_EndView();
    WebSceneFx_EndFrame();
    assert(!binds && !composes);

    /* A scope's window: CgWwiiScopeLens's first thirteen floats, last, after
     * the viewmodel's motion; the outline about its glass, its last six, goes
     * to the stencil alone. */
    assert(SCENE_FX_SCOPE == 42 && SCENE_FX_SCOPE_AXES == 44 && SCENE_FX_SCOPE_PRESENCE == 48 &&
           SCENE_FX_SCOPE_OPEN == 49 && SCENE_FX_SCOPE_BLUR == 50 && SCENE_FX_SCOPE_DARK == 51 &&
           SCENE_FX_SCOPE_PROJECTION == 52 && SCENE_FX_SCOPE_NEAR_CLIP == 54 && SCENE_FX_SCOPE_FLOATS == 13 &&
           SCENE_FX_SCOPE_FLOATS == SCENE_FX_SCOPE_END - SCENE_FX_SCOPE && SCENE_FX_SCOPE_LENS_FLOATS == 19);
    {
        const float lens[19] = {0.05f, -0.02f, 0.585f, -0.01f, 0.01f, 1.04f, 1, 0.5f, 0.5f, 0.25f, 1.7f, 3.0f, 0.8f,
                                0.06f, -0.03f, 0.82f, -0.01f, 0.015f, 1.45f};
        float bad[19];
        const float *projection;

        defaults();
        glow.current.enabled = 0; film.current.enabled = 0; samples.current.integer = 1; dof.current.enabled = 0;
        motion.current.enabled = 0; ssao.current.enabled = 0;
        binds = composes = stencils = masks = 0;
        frame(0, 1280, 0.1f);
        assert(!binds && !composes);
        /* With every other effect off a window composes, and its floats reach
         * the compose pass. */
        WebSceneFx_SetScope(lens);
        WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 4.0f);
        assert(binds == 1 && WebSceneFx_ScopeAvailable());
        /* The rifle's projection while the composed view draws it, and its
         * near clip: the window's, unless the depth hack's is further. */
        projection = WebSceneFx_ViewmodelProjection();
        assert(projection && projection[0] == 1.7f && projection[1] == 3.0f);
        assert(WebSceneFx_ViewmodelNearClip(0.1f) == 0.8f && WebSceneFx_ViewmodelNearClip(2.0f) == 2.0f);
        /* The stencil is drawn once per view, before the rifle's first draw;
         * each of its draws is masked and unmasked. */
        WebSceneFx_BeginViewmodel();
        assert(stencils == 1 && masks == 1 && maskOn == 1 && !memcmp(stenciled, lens, sizeof(stenciled)) &&
               !memcmp(stenciledBell, lens + SCENE_FX_SCOPE_FLOATS, sizeof(stenciledBell)));
        WebSceneFx_EndViewmodel();
        assert(masks == 2 && maskOn == 0);
        WebSceneFx_EndViewmodel();
        assert(masks == 2);
        WebSceneFx_BeginViewmodel();
        assert(stencils == 1 && masks == 3 && maskOn == 1);
        /* A view that ends while the rifle draws still lifts the mask. */
        WebSceneFx_EndView();
        assert(masks == 4 && maskOn == 0 && composes == 1);
        for (int i = 0; i < SCENE_FX_SCOPE_FLOATS; ++i)
            assert(composed[SCENE_FX_SCOPE + i] == lens[i]);
        /* The rifle's depth is linearized with the near clip it was drawn with. */
        assert(composed[SCENE_FX_NEAR_CLIP] == 0.8f);
        WebSceneFx_EndFrame();
        /* A Reflex or Lens Sight's window, a glass the rifle draws around
         * with the view's own projection: the attachments' own surfaces stay
         * drawn over it, every other material of the rifle is kept off it,
         * and nothing is left masked after the rifle. */
        {
            float sight[19];
            int savedStencils = stencils, savedMasks = masks;

            memcpy(sight, lens, sizeof(sight));
            sight[10] = sight[11] = sight[12] = 0.0f;
            WebSceneFx_SetScope(sight);
            WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 4.0f);
            masks = 0;
            WebSceneFx_ViewmodelMaterial("mtl_browser_att_blued");
            assert(masks == 0);
            WebSceneFx_BeginViewmodel();
            assert(masks == 1 && maskOn == 0);
            WebSceneFx_ViewmodelMaterial("mtl_browser_att_worn");
            assert(masks == 1);
            WebSceneFx_ViewmodelMaterial("mtl_weapon_thompson");
            assert(masks == 2 && maskOn == 1);
            WebSceneFx_ViewmodelMaterial(NULL);
            assert(masks == 2 && maskOn == 1);
            WebSceneFx_ViewmodelMaterial("mtl_browser_att_parkerized");
            assert(masks == 3 && maskOn == 0);
            WebSceneFx_EndViewmodel();
            assert(masks == 4 && maskOn == 0);
            WebSceneFx_ViewmodelMaterial("mtl_weapon_thompson");
            WebSceneFx_ViewmodelMaterial("mtl_browser_att_blued");
            assert(masks == 4);
            WebSceneFx_EndFrame();
            /* A sight's window, drawn with the view's projection, keeps no
             * outline about it. */
            for (int i = 0; i < SCENE_FX_SCOPE_BELL_FLOATS; ++i)
                assert(stenciledBell[i] == 0);
            /* A scope's glass keeps the attachments off it too. */
            WebSceneFx_SetScope(lens);
            WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 4.0f);
            WebSceneFx_ViewmodelMaterial("mtl_browser_att_parkerized");
            WebSceneFx_BeginViewmodel();
            assert(maskOn == 1);
            WebSceneFx_ViewmodelMaterial("mtl_weapon_kar98k");
            assert(maskOn == 1);
            WebSceneFx_EndViewmodel();
            WebSceneFx_EndFrame();
            stencils = savedStencils;
            masks = savedMasks;
        }
        /* Outside a composed view the rifle keeps the view's projection and
         * near clip and draws without the stencil. */
        assert(!WebSceneFx_ViewmodelProjection());
        assert(WebSceneFx_ViewmodelNearClip(0.1f) == 0.1f);
        WebSceneFx_BeginViewmodel();
        WebSceneFx_EndViewmodel();
        assert(stencils == 1 && masks == 4);
        WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 4.0f);
        WebSceneFx_BeginViewmodel();
        assert(stencils == 2);
        WebSceneFx_EndFrame();
        assert(maskOn == 0);
        /* The rifle's projection alone composes, before its glass opens. */
        memcpy(bad, lens, sizeof(bad));
        bad[6] = 0;
        WebSceneFx_SetScope(bad);
        binds = composes = stencils = 0;
        WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 4.0f);
        WebSceneFx_BeginViewmodel();
        assert(binds == 1 && WebSceneFx_ViewmodelProjection() && !stencils);
        WebSceneFx_EndFrame();
        /* Finite and bounded: the fractions clamp, an outline without area
         * closes the window, a non-positive projection is dropped and the
         * near clip stays within the depth hack's range. */
        for (int i = 0; i < SCENE_FX_SCOPE_LENS_FLOATS; ++i)
            bad[i] = NAN;
        WebSceneFx_SetScope(bad);
        binds = 0;
        frame(0, 1280, 0.1f);
        assert(!binds);
        memcpy(bad, lens, sizeof(bad));
        bad[12] = 1e9f;
        WebSceneFx_SetScope(bad);
        frame(0, 1280, 0.1f);
        assert(composed[SCENE_FX_SCOPE_NEAR_CLIP] == 16.0f && composed[SCENE_FX_NEAR_CLIP] == 16.0f);
        bad[12] = -1;
        WebSceneFx_SetScope(bad);
        frame(0, 1280, 0.1f);
        assert(composed[SCENE_FX_SCOPE_NEAR_CLIP] == 0 && composed[SCENE_FX_NEAR_CLIP] == 0.1f);
        bad[12] = NAN;
        WebSceneFx_SetScope(bad);
        frame(0, 1280, 0.1f);
        assert(composed[SCENE_FX_SCOPE_NEAR_CLIP] == 0 && composed[SCENE_FX_NEAR_CLIP] == 0.1f);
        memcpy(bad, lens, sizeof(bad));
        bad[6] = 3; bad[7] = -1; bad[8] = 2; bad[9] = INFINITY;
        WebSceneFx_SetScope(bad);
        frame(0, 1280, 0.1f);
        assert(composed[SCENE_FX_SCOPE_PRESENCE] == 1 && composed[SCENE_FX_SCOPE_OPEN] == 0 &&
               composed[SCENE_FX_SCOPE_BLUR] == 1 && composed[SCENE_FX_SCOPE_DARK] == 0);
        memcpy(bad, lens, sizeof(bad));
        bad[2] = bad[3] = 0;
        WebSceneFx_SetScope(bad);
        frame(0, 1280, 0.1f);
        assert(composed[SCENE_FX_SCOPE_PRESENCE] == 0 && composed[SCENE_FX_SCOPE_PROJECTION] == 1.7f);
        memcpy(bad, lens, sizeof(bad));
        bad[11] = -3;
        WebSceneFx_SetScope(bad);
        frame(0, 1280, 0.1f);
        assert(composed[SCENE_FX_SCOPE_PROJECTION] == 0 && composed[SCENE_FX_SCOPE_PROJECTION + 1] == 0);
        /* The outline about the glass stays finite, and one without area is
         * dropped. */
        memcpy(bad, lens, sizeof(bad));
        bad[13] = NAN;
        WebSceneFx_SetScope(bad);
        WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 4.0f);
        WebSceneFx_BeginViewmodel();
        assert(stenciledBell[0] == 0 && stenciledBell[1] == lens[14] && stenciledBell[2] == lens[15]);
        WebSceneFx_EndFrame();
        memcpy(bad, lens, sizeof(bad));
        bad[15] = bad[16] = 0;
        WebSceneFx_SetScope(bad);
        WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 4.0f);
        WebSceneFx_BeginViewmodel();
        for (int i = 0; i < SCENE_FX_SCOPE_BELL_FLOATS; ++i)
            assert(stenciledBell[i] == 0);
        WebSceneFx_EndFrame();
        /* Without a composed target, after a failed bind or under
         * r_fullbright, the cgame keeps the stock overlay. */
        WebSceneFx_SetScope(lens);
        bindResult = 0;
        frame(0, 1280, 0.1f);
        assert(!WebSceneFx_ScopeAvailable());
        bindResult = 1;
        frame(0, 1280, 0.1f);
        assert(WebSceneFx_ScopeAvailable());
        fullbright.current.enabled = 1;
        frame(0, 1280, 0.1f);
        assert(!WebSceneFx_ScopeAvailable());
        fullbright.current.enabled = 0;
        frame(0, 1280, 0.1f);
        assert(WebSceneFx_ScopeAvailable());
        WebSceneFx_SetScope(NULL);
        binds = 0;
        frame(0, 1280, 0.1f);
        assert(!binds);
    }

    /* The arms and weapon's own projection at the hip (cg_gunfov_mp.c)
     * composes the view on its own, as a scope's does, and the composed view
     * draws them with it and the depth hack's near clip; a raised scope's
     * comes first, and a broken or non-positive one, or none, is dropped. */
    {
        const float gun[2] = {1.4f, 2.48f};
        const float scope[SCENE_FX_SCOPE_LENS_FLOATS] = {0, 0, 0.5f, 0, 0, 0.9f, 1, 1, 0, 0, 1.7f, 3.0f, 0.8f};
        const float broken[4][2] = {{NAN, 2.48f}, {1.4f, -1.0f}, {0.0f, 2.48f}, {1.4f, INFINITY}};
        const float *projection;

        defaults();
        glow.current.enabled = 0; film.current.enabled = 0; samples.current.integer = 1; dof.current.enabled = 0;
        motion.current.enabled = 0; ssao.current.enabled = 0;
        binds = composes = 0;
        frame(0, 1280, 0.1f);
        assert(!binds && !composes);
        WebSceneFx_SetGunProjection(gun);
        WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 4.0f);
        projection = WebSceneFx_ViewmodelProjection();
        assert(binds == 1 && projection && projection[0] == 1.4f && projection[1] == 2.48f);
        assert(WebSceneFx_ViewmodelNearClip(0.1f) == 0.1f);
        WebSceneFx_SetScope(scope);
        projection = WebSceneFx_ViewmodelProjection();
        assert(projection && projection[0] == 1.7f && projection[1] == 3.0f);
        WebSceneFx_SetScope(NULL);
        WebSceneFx_EndFrame();
        assert(composes == 1 && !WebSceneFx_ViewmodelProjection());
        for (int i = 0; i < 4; ++i) {
            WebSceneFx_SetGunProjection(gun);
            WebSceneFx_SetGunProjection(broken[i]);
            binds = 0;
            WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 4.0f);
            assert(!binds && !WebSceneFx_ViewmodelProjection());
            WebSceneFx_EndFrame();
        }
        WebSceneFx_SetGunProjection(gun);
        WebSceneFx_SetGunProjection(NULL);
        binds = 0;
        frame(0, 1280, 0.1f);
        assert(!binds);
    }

    /* Ambient occlusion (r_ssao) composes on its own, with its strength, its
     * world radius and the composed view's own projection, taken when the
     * view begins; off, or without a usable camera, it adds nothing. */
    {
        assert(SCENE_FX_AO == 55 && SCENE_FX_AO_RADIUS == 56 && SCENE_FX_AO_PROJECTION == 57);
        defaults();
        glow.current.enabled = 0; film.current.enabled = 0; samples.current.integer = 1; dof.current.enabled = 0;
        motion.current.enabled = 0;
        WebSceneFx_SetAds(0, 6, 15, 512);
        frame(0, 1280, 0.1f);
        binds = composes = 0;
        frame(0, 1280, 0.1f);
        assert(binds == 1 && composes == 1 && composed[SCENE_FX_AO] == 0);
        clockMs += 10;
        camera(0.2f, 0.1f, 10, 20, 30);
        frame(0, 1280, 0.1f);
        assert(binds == 2 && composes == 2);
        assert(composed[SCENE_FX_AO] == SCENE_FX_AO_STRENGTH && composed[SCENE_FX_AO_RADIUS] == SCENE_FX_AO_WORLD_RADIUS);
        assert(composed[SCENE_FX_AO_PROJECTION] == 1.19f && composed[SCENE_FX_AO_PROJECTION + 1] == 2.11f);
        /* A camera set for a later view does not reach the composed one. */
        clockMs += 10;
        camera(0.2f, 0.1f, 10, 20, 30);
        WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 4.0f);
        lens[0] = 3.0f; lens[5] = 4.0f;
        WebSceneFx_SetCamera(view, lens, (int)clockMs);
        WebSceneFx_EndView();
        WebSceneFx_EndFrame();
        assert(composed[SCENE_FX_AO_PROJECTION] == 1.19f && composed[SCENE_FX_AO_PROJECTION + 1] == 2.11f);
        lens[0] = 1.19f; lens[5] = 2.11f;
        frame(0, 1280, 0.1f);
        /* A broken projection takes none. */
        lens[0] = 0;
        clockMs += 10;
        camera(0.2f, 0.1f, 10, 20, 30);
        frame(0, 1280, 0.1f);
        assert(composed[SCENE_FX_AO] == 0);
        lens[0] = 1.19f;
        /* r_ssao 0 adds none, and alone composes nothing. */
        ssao.current.enabled = 0;
        binds = composes = 0;
        clockMs += 10;
        camera(0.2f, 0.1f, 10, 20, 30);
        frame(0, 1280, 0.1f);
        assert(!binds && !composes);
        film.current.enabled = 1;
        clockMs += 10;
        camera(0.2f, 0.1f, 10, 20, 30);
        frame(0, 1280, 0.1f);
        assert(composes == 1 && composed[SCENE_FX_AO] == 0);
        /* r_fullbright draws directly. */
        defaults();
        fullbright.current.enabled = 1;
        binds = composes = 0;
        frame(0, 1280, 0.1f);
        assert(!binds && !composes);
        fullbright.current.enabled = 0;
    }

    /* The color maps' mip bias while a scaled scene's view is open: the
     * scale's octaves, never finer than one; none for a full-size scene,
     * after the view or without a target. */
    {
        defaults();
        assert(WebSceneFx_TextureLodBias() == 0);
        WebSceneFx_BeginView(0, 0, 1920, 1080, 1920, 1080, 0.1f, 4.0f);
        assert(fabsf(WebSceneFx_TextureLodBias() - log2f(720.0f / 1080.0f)) < 1e-6f);
        WebSceneFx_EndView();
        assert(WebSceneFx_TextureLodBias() == 0);
        WebSceneFx_EndFrame();
        scene.current.integer = 200;
        WebSceneFx_BeginView(0, 0, 1920, 1080, 1920, 1080, 0.1f, 4.0f);
        assert(WebSceneFx_TextureLodBias() == -1.0f);
        WebSceneFx_EndFrame();
        scene.current.integer = 0;
        WebSceneFx_BeginView(0, 0, 1920, 1080, 1920, 1080, 0.1f, 4.0f);
        assert(WebSceneFx_TextureLodBias() == 0);
        WebSceneFx_EndFrame();
        scene.current.integer = 720;
        bindResult = 0;
        WebSceneFx_BeginView(0, 0, 1920, 1080, 1920, 1080, 0.1f, 4.0f);
        assert(WebSceneFx_TextureLodBias() == 0);
        WebSceneFx_EndFrame();
        bindResult = 1;
    }

    /* Effects take the composed view's near clip back to distance while it
     * is open, and none outside it. */
    {
        defaults();
        assert(WebSceneFx_WorldNearClip() == 0);
        WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 3.998f);
        assert(WebSceneFx_WorldNearClip() == 3.998f);
        WebSceneFx_EndView();
        assert(WebSceneFx_WorldNearClip() == 0);
        WebSceneFx_EndFrame();
    }

    /* The haze (r_atmosphere) composes on its own from the map's exponential
     * fog: its color, a share of its density, how fast it thins with height,
     * and the world's up, the sun and its glow in the composed view's space,
     * taken when the view begins. Off, without fog or under r_fullbright it
     * adds nothing; a broken fog turns it off, a broken sun its glow. */
    {
        const float fog[3] = {0.9f, 0.95f, 1.0f}, sun[3] = {0, 3, 4}, sunColor[3] = {1.2f, 1.0f, 0.8f};
        const float noSun[3] = {0, 0, 0}, badColor[3] = {0.5f, NAN, 0.5f};
        assert(SCENE_FX_HAZE == 59 && SCENE_FX_HAZE_DENSITY == 62 && SCENE_FX_HAZE_UP == 63 && SCENE_FX_HAZE_SUN == 66 &&
               SCENE_FX_HAZE_SUN_COLOR == 69 && SCENE_FX_HAZE_FALLOFF == 72);
        defaults();
        glow.current.enabled = 0; film.current.enabled = 0; samples.current.integer = 1; dof.current.enabled = 0;
        motion.current.enabled = 0; ssao.current.enabled = 0; sunShafts.current.value = 0;
        WebSceneFx_SetAds(0, 6, 15, 512);
        WebSceneFx_SetAtmosphere(fog, 0, sun, sunColor);
        binds = composes = 0;
        clockMs += 10;
        camera(0.3f, 0.1f, 100, 200, 60);
        frame(0, 1280, 0.1f);
        assert(!binds && !composes);
        WebSceneFx_SetAtmosphere(fog, 0.00015f, sun, sunColor);
        clockMs += 10;
        camera(0.3f, 0.1f, 100, 200, 60);
        frame(0, 1280, 0.1f);
        assert(binds == 1 && composes == 1);
        assert(fabsf(composed[SCENE_FX_HAZE_DENSITY] - 0.00015f * SCENE_FX_HAZE_SHARE) < 1e-10f);
        for (int i = 0; i < 3; ++i) {
            /* A view turns a world direction w into R w, R[j][i] = view[i * 4 + j]. */
            float up = view[8 + i], toSun = (view[4 + i] * 3 + view[8 + i] * 4) / 5;
            assert(composed[SCENE_FX_HAZE + i] == fog[i]);
            assert(fabsf(composed[SCENE_FX_HAZE_UP + i] - up) < 1e-6f);
            assert(fabsf(composed[SCENE_FX_HAZE_SUN + i] - toSun) < 1e-6f);
            assert(fabsf(composed[SCENE_FX_HAZE_SUN_COLOR + i] - sunColor[i] * SCENE_FX_HAZE_SUN_GLOW) < 1e-6f);
        }
        /* Looking down 0.1 rad, the world's up leans away from the view. */
        assert(composed[SCENE_FX_HAZE_UP + 1] > 0.99f && composed[SCENE_FX_HAZE_UP + 2] < -0.09f);
        assert(composed[SCENE_FX_HAZE_FALLOFF] == 1.0f / SCENE_FX_HAZE_SCALE_HEIGHT);
        /* A camera set for a later view does not reach the composed one. */
        {
            float up = composed[SCENE_FX_HAZE_UP + 2];
            clockMs += 10;
            camera(0.3f, 0.1f, 100, 200, 60);
            WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 4.0f);
            camera(0.3f, -0.8f, 100, 200, 60);
            WebSceneFx_EndView();
            WebSceneFx_EndFrame();
            assert(composed[SCENE_FX_HAZE_UP + 2] == up);
        }
        /* A view without a camera, or with a broken projection, takes none. */
        frame(0, 1280, 0.1f);
        assert(composed[SCENE_FX_HAZE_DENSITY] > 0);
        WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 4.0f);
        WebSceneFx_EndView();
        WebSceneFx_EndFrame();
        assert(composed[SCENE_FX_HAZE_DENSITY] == 0);
        lens[0] = 0;
        clockMs += 10;
        camera(0.3f, 0.1f, 100, 200, 60);
        frame(0, 1280, 0.1f);
        assert(composed[SCENE_FX_HAZE_DENSITY] == 0);
        lens[0] = 1.19f;
        /* Dawnville's thick fog adds no more than the thinnest map's whole
         * fog, and nothing larger does either. */
        const float capped[] = {0.00025f, 1.0f};
        for (int i = 0; i < 2; ++i) {
            WebSceneFx_SetAtmosphere(fog, capped[i], sun, sunColor);
            clockMs += 10;
            camera(0.3f, 0.1f, 100, 200, 60);
            frame(0, 1280, 0.1f);
            assert(composed[SCENE_FX_HAZE_DENSITY] == SCENE_FX_HAZE_MAX_DENSITY);
        }
        WebSceneFx_SetAtmosphere(fog, 0.0001f, sun, sunColor);
        clockMs += 10;
        camera(0.3f, 0.1f, 100, 200, 60);
        frame(0, 1280, 0.1f);
        assert(fabsf(composed[SCENE_FX_HAZE_DENSITY] - 0.0001f * SCENE_FX_HAZE_SHARE) < 1e-10f);
        /* A broken sun keeps the haze without its glow. */
        WebSceneFx_SetAtmosphere(fog, 0.00015f, noSun, sunColor);
        clockMs += 10;
        camera(0.3f, 0.1f, 100, 200, 60);
        frame(0, 1280, 0.1f);
        assert(composed[SCENE_FX_HAZE_DENSITY] > 0);
        for (int i = 0; i < 3; ++i)
            assert(composed[SCENE_FX_HAZE_SUN + i] == 0 && composed[SCENE_FX_HAZE_SUN_COLOR + i] == 0);
        WebSceneFx_SetAtmosphere(fog, 0.00015f, sun, badColor);
        clockMs += 10;
        camera(0.3f, 0.1f, 100, 200, 60);
        frame(0, 1280, 0.1f);
        assert(composed[SCENE_FX_HAZE_DENSITY] > 0);
        for (int i = 0; i < 3; ++i)
            assert(composed[SCENE_FX_HAZE_SUN + i] == 0 && composed[SCENE_FX_HAZE_SUN_COLOR + i] == 0);
        WebSceneFx_SetAtmosphere(fog, 0.00015f, NULL, sunColor);
        clockMs += 10;
        camera(0.3f, 0.1f, 100, 200, 60);
        frame(0, 1280, 0.1f);
        assert(composed[SCENE_FX_HAZE_DENSITY] > 0 && composed[SCENE_FX_HAZE_SUN + 2] == 0);
        /* A broken fog adds none, and alone composes nothing. */
        const float densities[] = {NAN, -1, INFINITY};
        for (int i = 0; i < 4; ++i) {
            WebSceneFx_SetAtmosphere(i < 3 ? fog : badColor, i < 3 ? densities[i] : 0.00015f, sun, sunColor);
            binds = composes = 0;
            clockMs += 10;
            camera(0.3f, 0.1f, 100, 200, 60);
            frame(0, 1280, 0.1f);
            assert(!binds && !composes);
        }
        WebSceneFx_SetAtmosphere(NULL, 0.00015f, sun, sunColor);
        clockMs += 10;
        camera(0.3f, 0.1f, 100, 200, 60);
        frame(0, 1280, 0.1f);
        assert(!binds && !composes);
        /* r_atmosphere 0 and r_fullbright add none. */
        WebSceneFx_SetAtmosphere(fog, 0.00015f, sun, sunColor);
        atmosphere.current.enabled = 0;
        clockMs += 10;
        camera(0.3f, 0.1f, 100, 200, 60);
        frame(0, 1280, 0.1f);
        assert(!binds && !composes);
        film.current.enabled = 1;
        clockMs += 10;
        camera(0.3f, 0.1f, 100, 200, 60);
        frame(0, 1280, 0.1f);
        assert(composes == 1 && composed[SCENE_FX_HAZE_DENSITY] == 0);
        defaults();
        fullbright.current.enabled = 1;
        binds = composes = 0;
        frame(0, 1280, 0.1f);
        assert(!binds && !composes);
        fullbright.current.enabled = 0;
        WebSceneFx_SetAtmosphere(fog, 0, sun, sunColor);
    }

    /* Shafts of sunlight (r_sunShafts) compose on their own wherever the
     * scene has a sun: how strong, where the sun lies on the screen (its
     * direction in the composed view's space through the projection, kept
     * within 1.5 screens of the centre), how low it stands by the air its
     * light crosses, and its color. They come in as the sun nears the view's
     * axis, none at its side or behind; off, under r_fullbright or without a
     * sun, nothing composes; without a camera they are dark. */
    {
        const float fog[3] = {0.9f, 0.95f, 1.0f}, sunColor[3] = {1.2f, 1.0f, 0.8f}, badColor[3] = {0.5f, NAN, 0.5f};
        const float rad = 3.14159265f / 180.0f;
        assert(SCENE_FX_SHAFTS == 73 && SCENE_FX_SHAFTS_SUN == 74 && SCENE_FX_SHAFTS_LOW == 76 &&
               SCENE_FX_SHAFTS_COLOR == 77);
        defaults();
        glow.current.enabled = 0; film.current.enabled = 0; samples.current.integer = 1; dof.current.enabled = 0;
        motion.current.enabled = 0; ssao.current.enabled = 0; atmosphere.current.enabled = 0;
        WebSceneFx_SetAds(0, 6, 15, 512);
        /* The sun e degrees up and a degrees left of a level camera facing
         * +x: in its view x = -cos e sin a, y = sin e and z = cos e cos a. */
        struct { float e, a, strength, u, v, low; } cases[] = {
            /* Matmata's dusk, 20 degrees up ahead: low, the most light. */
            {20, 0, SCENE_FX_SHAFTS_GAIN_LOW, 0.5f, 0.5f + 0.5f * 2.11f * tanf(20 * rad), 1},
            /* 45 degrees up, as on Toujane: mostly high. */
            {45, 0, 0, 0.5f, 0.5f + 0.5f * 2.11f, (1.41421356f - SCENE_FX_SHAFTS_HIGH_AIR) /
                                                   (SCENE_FX_SHAFTS_LOW_AIR - SCENE_FX_SHAFTS_HIGH_AIR)},
            /* 60 degrees up: high; above the screen, held 1.5 screens out. */
            {60, 0, SCENE_FX_SHAFTS_GAIN_HIGH, 0.5f, 2.0f, 0},
            /* Halfway into view from the left: half the light, held off the screen's left. */
            {20, 69.7658f, 0.5f * SCENE_FX_SHAFTS_GAIN_LOW, -1.0f, 0, 1},
            /* A quarter of the way in, eased: 0.15625 of the light, held off
             * the screen's upper left corner. */
            {20, 75.3602f, 0.15625f * SCENE_FX_SHAFTS_GAIN_LOW, -1.0f, 2.0f, 1},
            /* At the view's side or behind it: none. */
            {20, 81, 0, 0, 0, 0},
            {20, 180, 0, 0, 0, 0},
            /* Just under the horizon ahead: all the air there is. */
            {-10, 0, SCENE_FX_SHAFTS_GAIN_LOW, 0.5f, 0.5f - 0.5f * 2.11f * tanf(10 * rad), 1},
        };
        cases[1].strength = SCENE_FX_SHAFTS_GAIN_HIGH + (SCENE_FX_SHAFTS_GAIN_LOW - SCENE_FX_SHAFTS_GAIN_HIGH) * cases[1].low;
        for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
            const float e = cases[i].e * rad, a = cases[i].a * rad;
            const float sun[3] = {cosf(e) * cosf(a), cosf(e) * sinf(a), sinf(e)};
            const float x = -cosf(e) * sinf(a), z = cosf(e) * cosf(a);
            WebSceneFx_SetAtmosphere(fog, 0, sun, sunColor);
            binds = composes = 0;
            clockMs += 10;
            camera(0, 0, 100, 200, 60);
            frame(0, 1280, 0.1f);
            assert(binds == 1 && composes == 1);
            assert(fabsf(composed[SCENE_FX_SHAFTS] - cases[i].strength) < 1e-4f);
            assert(fabsf(composed[SCENE_FX_SHAFTS_LOW] - cases[i].low) < 1e-4f);
            if (i == 3) {
                assert(fabsf(z - 0.325f) < 1e-3f && 0.5f + 0.5f * 1.19f * x / z < -1.0f);
                assert(composed[SCENE_FX_SHAFTS_SUN + 1] > 0.5f);
            } else {
                assert(fabsf(composed[SCENE_FX_SHAFTS_SUN] - cases[i].u) < 1e-4f);
                assert(fabsf(composed[SCENE_FX_SHAFTS_SUN + 1] - cases[i].v) < 1e-4f);
            }
            if (i == 3)
                assert(composed[SCENE_FX_SHAFTS_SUN] == -1.0f);
            for (int k = 0; k < 3; ++k)
                assert(composed[SCENE_FX_SHAFTS_COLOR + k] == sunColor[k]);
        }
        /* The sun to the right lies right of the centre, and r_sunShafts
         * scales the light up to twice. */
        {
            const float sun[3] = {cosf(20 * rad) * cosf(-10 * rad), cosf(20 * rad) * sinf(-10 * rad), sinf(20 * rad)};
            WebSceneFx_SetAtmosphere(fog, 0, sun, sunColor);
            const float values[] = {2, 5};
            for (int i = 0; i < 2; ++i) {
                sunShafts.current.value = values[i];
                clockMs += 10;
                camera(0, 0, 100, 200, 60);
                frame(0, 1280, 0.1f);
                assert(fabsf(composed[SCENE_FX_SHAFTS] - 2 * SCENE_FX_SHAFTS_GAIN_LOW) < 1e-4f);
                assert(fabsf(composed[SCENE_FX_SHAFTS_SUN] - (0.5f + 0.5f * 1.19f * tanf(10 * rad))) < 1e-4f);
            }
            sunShafts.current.value = 1;
        }
        /* A view without a camera keeps its sun dark. */
        binds = composes = 0;
        frame(0, 1280, 0.1f);
        assert(composes == 1 && composed[SCENE_FX_SHAFTS] == 0 && composed[SCENE_FX_SHAFTS_SUN] == 0 &&
               composed[SCENE_FX_SHAFTS_LOW] == 0);
        /* r_sunShafts 0, r_fullbright, no sun or a broken one: nothing composes. */
        sunShafts.current.value = 0;
        binds = composes = 0;
        clockMs += 10;
        camera(0, 0, 100, 200, 60);
        frame(0, 1280, 0.1f);
        assert(!binds && !composes);
        sunShafts.current.value = 1;
        fullbright.current.enabled = 1;
        clockMs += 10;
        camera(0, 0, 100, 200, 60);
        frame(0, 1280, 0.1f);
        assert(!binds && !composes);
        fullbright.current.enabled = 0;
        {
            const float ahead[3] = {1, 0, 0.36397f}, noSun[3] = {0, 0, 0};
            WebSceneFx_SetAtmosphere(fog, 0, noSun, sunColor);
            clockMs += 10;
            camera(0, 0, 100, 200, 60);
            frame(0, 1280, 0.1f);
            assert(!binds && !composes);
            WebSceneFx_SetAtmosphere(fog, 0, ahead, badColor);
            clockMs += 10;
            camera(0, 0, 100, 200, 60);
            frame(0, 1280, 0.1f);
            assert(!binds && !composes);
            WebSceneFx_SetAtmosphere(fog, 0, ahead, sunColor);
            film.current.enabled = 1;
            clockMs += 10;
            camera(0, 0, 100, 200, 60);
            frame(0, 1280, 0.1f);
            assert(composes == 1 && composed[SCENE_FX_SHAFTS] > 0);
            sunShafts.current.value = 0;
            clockMs += 10;
            camera(0, 0, 100, 200, 60);
            frame(0, 1280, 0.1f);
            assert(composes == 2 && composed[SCENE_FX_SHAFTS] == 0);
        }
        defaults();
        WebSceneFx_SetAtmosphere(fog, 0, NULL, sunColor);
    }

    /* Explosions' shockwaves (r_shockwave) compose on their own while they
     * travel: each one's centre on the screen through the composed view's
     * projection, its front running out to 1.6 times the blast's radius over
     * 450 ms, its bend fading with age and beyond 1.5 radii from the eye, and
     * how far ahead it is. One behind the eye has none; four travel at once,
     * a fifth taking the oldest's place; a broken one takes no place. */
    {
        assert(SCENE_FX_BLAST == 80 && SCENE_FX_BLAST_DEPTH == 96 && SCENE_FX_PARAM_COUNT == 119);
        defaults();
        glow.current.enabled = 0; film.current.enabled = 0; samples.current.integer = 1; dof.current.enabled = 0;
        motion.current.enabled = 0; ssao.current.enabled = 0; atmosphere.current.enabled = 0; sunShafts.current.value = 0;
        WebSceneFx_SetAds(0, 6, 15, 512);
        clockMs += 1000;
        binds = composes = 0;
        camera(0, 0, 0, 0, 0);
        frame(0, 1280, 0.1f);
        assert(!binds && !composes);
        /* A level camera at the origin facing +x sees a point (x, y, z) at
         * x right = -y, up = z and ahead = x. */
        struct { float ms, origin[3], radius; } blast = {0, {400, 100, 50}, 256};
        const float u = 0.5f - 0.5f * 100 * 1.19f / 400, v = 0.5f + 0.5f * 50 * 2.11f / 400;
        const float near = 1.5f * 256 / sqrtf(400 * 400 + 100 * 100 + 50 * 50);
        WebSceneFx_AddExplosion(blast.origin, blast.radius);
        const float ages[] = {0, 225, 449};
        for (int i = 0; i < 3; ++i) {
            const float life = ages[i] / 450, left = 1 - life;
            const float front = 256 * 1.6f * (1 - left * left * left);
            clockMs += i ? ages[i] - ages[i - 1] : 0;
            binds = composes = 0;
            camera(0, 0, 0, 0, 0);
            frame(0, 1280, 0.1f);
            assert(binds == 1 && composes == 1);
            assert(fabsf(composed[SCENE_FX_BLAST] - u) < 1e-5f && fabsf(composed[SCENE_FX_BLAST + 1] - v) < 1e-5f);
            assert(fabsf(composed[SCENE_FX_BLAST + 2] - 0.5f * front * 2.11f / 400) < 1e-4f);
            assert(fabsf(composed[SCENE_FX_BLAST + 3] - SCENE_FX_BLAST_BEND * left * left * near) < 1e-6f);
            assert(fabsf(composed[SCENE_FX_BLAST_DEPTH] - 400) < 1e-3f);
            for (int k = 1; k < SCENE_FX_BLASTS; ++k)
                assert(composed[SCENE_FX_BLAST_DEPTH + k] == 0 && composed[SCENE_FX_BLAST + k * 4 + 3] == 0);
        }
        /* At 450 ms it is gone and nothing composes. */
        clockMs += 1;
        binds = composes = 0;
        camera(0, 0, 0, 0, 0);
        frame(0, 1280, 0.1f);
        assert(!binds && !composes);
        /* Close by it bends the most; far off, less. */
        {
            const float close[3] = {200, 0, 0}, far[3] = {3840, 0, 0};
            WebSceneFx_AddExplosion(close, 256);
            WebSceneFx_AddExplosion(far, 256);
            camera(0, 0, 0, 0, 0);
            frame(0, 1280, 0.1f);
            float bends[2] = {0, 0};
            for (int k = 0; k < SCENE_FX_BLASTS; ++k) {
                if (composed[SCENE_FX_BLAST_DEPTH + k] == 200) bends[0] = composed[SCENE_FX_BLAST + k * 4 + 3];
                if (composed[SCENE_FX_BLAST_DEPTH + k] == 3840) bends[1] = composed[SCENE_FX_BLAST + k * 4 + 3];
            }
            assert(bends[0] == SCENE_FX_BLAST_BEND && fabsf(bends[1] - SCENE_FX_BLAST_BEND * 0.1f) < 1e-6f);
            /* The next view, without a camera, keeps none of them. */
            frame(0, 1280, 0.1f);
            for (int k = 0; k < 4 * SCENE_FX_BLASTS + SCENE_FX_BLASTS; ++k)
                assert(composed[SCENE_FX_BLAST + k] == 0);
            clockMs += 450;
        }
        /* Behind the eye: still travelling, so the view composes, but nothing bends. */
        {
            const float behind[3] = {-300, 0, 0};
            WebSceneFx_AddExplosion(behind, 256);
            binds = composes = 0;
            camera(0, 0, 0, 0, 0);
            frame(0, 1280, 0.1f);
            assert(composes == 1);
            for (int k = 0; k < 4 * SCENE_FX_BLASTS + SCENE_FX_BLASTS; ++k)
                assert(composed[SCENE_FX_BLAST + k] == 0);
            /* Nor does a view without a camera. */
            frame(0, 1280, 0.1f);
            assert(composes == 2);
            for (int k = 0; k < 4 * SCENE_FX_BLASTS + SCENE_FX_BLASTS; ++k)
                assert(composed[SCENE_FX_BLAST + k] == 0);
            clockMs += 450;
        }
        /* Four travel at once; a fifth takes the oldest's place, and broken
         * ones take none. */
        {
            float depths[5];
            for (int n = 0; n < 5; ++n) {
                const float at[3] = {300 + 100 * n, 0, 0};
                depths[n] = at[0];
                WebSceneFx_AddExplosion(at, 128);
                clockMs += 10;
            }
            const float bad[][3] = {{NAN, 0, 0}, {500, INFINITY, 0}, {500, 0, NAN}};
            for (int n = 0; n < 3; ++n)
                WebSceneFx_AddExplosion(bad[n], 128);
            const float fine[3] = {500, 0, 0};
            WebSceneFx_AddExplosion(fine, NAN);
            WebSceneFx_AddExplosion(fine, -5);
            WebSceneFx_AddExplosion(fine, 0);
            WebSceneFx_AddExplosion(NULL, 128);
            camera(0, 0, 0, 0, 0);
            frame(0, 1280, 0.1f);
            int seen[5] = {0, 0, 0, 0, 0};
            for (int k = 0; k < SCENE_FX_BLASTS; ++k)
                for (int n = 0; n < 5; ++n)
                    if (fabsf(composed[SCENE_FX_BLAST_DEPTH + k] - depths[n]) < 1e-3f) ++seen[n];
            assert(!seen[0] && seen[1] == 1 && seen[2] == 1 && seen[3] == 1 && seen[4] == 1);
            clockMs += 450;
        }
        /* A blast reaching further than any is held at 2,048 units. */
        {
            const float at[3] = {4000, 0, 0};
            WebSceneFx_AddExplosion(at, 1e6f);
            clockMs += 225;
            camera(0, 0, 0, 0, 0);
            frame(0, 1280, 0.1f);
            assert(fabsf(composed[SCENE_FX_BLAST + 2] + composed[SCENE_FX_BLAST + 6] + composed[SCENE_FX_BLAST + 10] +
                         composed[SCENE_FX_BLAST + 14] -
                         0.5f * 2048 * 1.6f * 0.875f * 2.11f / 4000) < 1e-4f);
            clockMs += 450;
        }
        /* r_shockwave 0 and r_fullbright compose nothing. */
        {
            const float at[3] = {400, 0, 0};
            WebSceneFx_AddExplosion(at, 256);
            shockwave.current.enabled = 0;
            binds = composes = 0;
            camera(0, 0, 0, 0, 0);
            frame(0, 1280, 0.1f);
            assert(!binds && !composes);
            film.current.enabled = 1;
            camera(0, 0, 0, 0, 0);
            frame(0, 1280, 0.1f);
            assert(composes == 1);
            for (int k = 0; k < 4 * SCENE_FX_BLASTS + SCENE_FX_BLASTS; ++k)
                assert(composed[SCENE_FX_BLAST + k] == 0);
            film.current.enabled = 0;
            shockwave.current.enabled = 1;
            fullbright.current.enabled = 1;
            binds = composes = 0;
            frame(0, 1280, 0.1f);
            assert(!binds && !composes);
            fullbright.current.enabled = 0;
            clockMs += 450;
        }
        defaults();
    }
    /* Local contrast (r_clarity) composes on its own, at 0.28 of the dvar,
     * which holds between 0 and 2; r_fullbright and a broken value take none. */
    {
        assert(SCENE_FX_CLARITY == 100 && SCENE_FX_PARAM_COUNT == 119);
        defaults();
        glow.current.enabled = 0; film.current.enabled = 0; samples.current.integer = 1; dof.current.enabled = 0;
        motion.current.enabled = 0; ssao.current.enabled = 0; atmosphere.current.enabled = 0; sunShafts.current.value = 0;
        binds = composes = 0;
        frame(0, 1280, 0.1f);
        assert(!binds && !composes);
        assert(SCENE_FX_CLARITY_GAIN == 0.28f);
        const float values[][2] = {{1, 0.28f}, {2, 0.56f}, {0.5f, 0.14f}, {5, 0.56f}};
        for (int i = 0; i < 4; ++i) {
            clarity.current.value = values[i][0];
            binds = composes = 0;
            frame(0, 1280, 0.1f);
            assert(binds == 1 && composes == 1 && fabsf(composed[SCENE_FX_CLARITY] - values[i][1]) < 1e-6f);
        }
        const float off[] = {0, -1, NAN};
        for (int i = 0; i < 3; ++i) {
            clarity.current.value = off[i];
            binds = composes = 0;
            frame(0, 1280, 0.1f);
            assert(!binds && !composes);
        }
        film.current.enabled = 1;
        frame(0, 1280, 0.1f);
        assert(composes == 1 && composed[SCENE_FX_CLARITY] == 0);
        clarity.current.value = 1;
        fullbright.current.enabled = 1;
        binds = composes = 0;
        frame(0, 1280, 0.1f);
        assert(!binds && !composes);
        fullbright.current.enabled = 0;
        clarity.current.value = 0;
        defaults();
    }
    /* Temporal antialiasing (r_taa) composes on its own. Each composed view
     * moves within its pixels to the next of the first 8 points of the Halton
     * (2, 3) sequence, centred on the pixel: every 3D draw's clip x and y gain
     * the jitter times w. It keeps 0.85 of the frames before once the camera
     * has moved on continuously, with the rotation and offset into the last
     * view and that view's projection; the first frame and a cut keep none.
     * r_taa 0 and r_fullbright neither compose nor move the view. */
    {
        assert(SCENE_FX_TONE == 101 && SCENE_FX_TAA == 102 && SCENE_FX_TAA_ROTATION == 103 && SCENE_FX_TAA_MOVE == 112);
        assert(SCENE_FX_TAA_PROJECTION == 115 && SCENE_FX_TAA_JITTER == 117 && SCENE_FX_PARAM_COUNT == 119);
        static const float halton2[8] = {0.5f, 0.25f, 0.75f, 0.125f, 0.625f, 0.375f, 0.875f, 0.0625f};
        static const float halton3[8] = {1 / 3.0f, 2 / 3.0f, 1 / 9.0f, 4 / 9.0f, 7 / 9.0f, 2 / 9.0f, 5 / 9.0f, 8 / 9.0f};
        /* A view's projection, column-major, and a point through it. */
        static const float projection[16] = {1.19f, 0.01f, 0.02f, 0.03f, 0.04f, 2.11f, 0.05f, 0.06f,
                                             0.07f, 0.08f, -1.0005f, -1.0f, 0.09f, 0.1f, -8.0f, 0.2f};
        static const float point[4] = {3.0f, -2.0f, -40.0f, 1.0f};
        float jittered[16], clip[4], shifted[4];
        defaults();
        glow.current.enabled = 0; film.current.enabled = 0; samples.current.integer = 1; dof.current.enabled = 0;
        motion.current.enabled = 0; ssao.current.enabled = 0; atmosphere.current.enabled = 0; sunShafts.current.value = 0;
        binds = composes = 0;
        frame(0, 1280, 0.1f);
        assert(!binds && !composes);
        taa.current.enabled = 1;
        for (int i = 0; i < 16; ++i) {
            const float jx = (halton2[i % 8] - 0.5f) * 2.0f / 1280.0f, jy = (halton3[i % 8] - 0.5f) * 2.0f / 720.0f;
            binds = composes = 0;
            memcpy(jittered, projection, sizeof(jittered));
            WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 4.0f);
            WebSceneFx_JitterProjection(jittered);
            WebSceneFx_EndView();
            WebSceneFx_EndFrame();
            assert(binds == 1 && composes == 1);
            assert(fabsf(composed[SCENE_FX_TAA_JITTER] - jx) < 1e-7f && fabsf(composed[SCENE_FX_TAA_JITTER + 1] - jy) < 1e-7f);
            assert(fabsf(jx) < 1.0f / 1280.0f && fabsf(jy) < 1.0f / 720.0f);
            for (int r = 0; r < 4; ++r) {
                clip[r] = shifted[r] = 0;
                for (int c = 0; c < 4; ++c) {
                    clip[r] += projection[c * 4 + r] * point[c];
                    shifted[r] += jittered[c * 4 + r] * point[c];
                }
            }
            assert(fabsf(shifted[0] - (clip[0] + jx * clip[3])) < 1e-5f && fabsf(shifted[1] - (clip[1] + jy * clip[3])) < 1e-5f);
            assert(shifted[2] == clip[2] && shifted[3] == clip[3]);
            /* Without a camera there is nothing to keep. */
            assert(composed[SCENE_FX_TAA] == 1 && composed[SCENE_FX_SHARPEN] == 0);
        }
        /* Outside a composed view nothing moves. */
        memcpy(jittered, projection, sizeof(jittered));
        WebSceneFx_JitterProjection(jittered);
        assert(!memcmp(jittered, projection, sizeof(jittered)));
        WebSceneFx_JitterProjection(NULL);
        /* A view whose target fails to bind moves nothing and takes no point
         * of the sequence. */
        bindResult = 0;
        memcpy(jittered, projection, sizeof(jittered));
        WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 4.0f);
        WebSceneFx_JitterProjection(jittered);
        WebSceneFx_EndView();
        WebSceneFx_EndFrame();
        assert(!memcmp(jittered, projection, sizeof(jittered)));
        bindResult = 1;
        frame(0, 1280, 0.1f);
        assert(composed[SCENE_FX_TAA_JITTER] == 0 &&
               fabsf(composed[SCENE_FX_TAA_JITTER + 1] - (halton3[0] - 0.5f) * 2.0f / 720.0f) < 1e-7f);
        /* A camera moving on keeps 0.85 of the frames before, from the last
         * view's place and projection; the film grade sharpens the average
         * twice as much. */
        film.current.enabled = 1;
        clockMs += 16;
        camera(0.3f, 0.05f, 10, 20, 60);
        frame(0, 1280, 0.1f);
        assert(composed[SCENE_FX_TAA] == 1 && composed[SCENE_FX_SHARPEN] == 0.2f);
        clockMs += 16;
        camera(0.31f, 0.05f, 12, 20, 60);
        frame(0, 1280, 0.1f);
        assert(fabsf(composed[SCENE_FX_TAA] - 0.15f) < 1e-6f && composed[SCENE_FX_SHARPEN] == 0.2f);
        assert(composed[SCENE_FX_TAA_PROJECTION] == 1.19f && composed[SCENE_FX_TAA_PROJECTION + 1] == 2.11f);
        {
            static const float p[][3] = {{100, 40, 70}, {-300, 900, 10}, {35, -12, 64}};
            for (int k = 0; k < 3; ++k) {
                float now[3], before[3];
                viewPoint(view, p[k], now);
                viewPoint(lastView, p[k], before);
                for (int j = 0; j < 3; ++j) {
                    float back = composed[SCENE_FX_TAA_MOVE + j];
                    for (int m = 0; m < 3; ++m)
                        back += composed[SCENE_FX_TAA_ROTATION + m * 3 + j] * now[m];
                    assert(fabsf(back - before[j]) < 2e-3f * (1.0f + fabsf(before[j])));
                }
            }
        }
        /* A cut keeps none. */
        clockMs += 16;
        camera(2.5f, 0.05f, 12, 20, 60);
        frame(0, 1280, 0.1f);
        assert(composed[SCENE_FX_TAA] == 1);
        film.current.enabled = 0;
        /* r_taa 0 and r_fullbright neither compose nor move the view. */
        const int off[][2] = {{0, 0}, {1, 1}};
        for (int i = 0; i < 2; ++i) {
            taa.current.enabled = off[i][0];
            fullbright.current.enabled = off[i][1];
            binds = composes = 0;
            memcpy(jittered, projection, sizeof(jittered));
            WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 4.0f);
            WebSceneFx_JitterProjection(jittered);
            WebSceneFx_EndView();
            WebSceneFx_EndFrame();
            assert(!binds && !composes && !memcmp(jittered, projection, sizeof(jittered)));
        }
        /* Composed for another effect, r_taa 0 keeps nothing and moves nothing. */
        taa.current.enabled = 0;
        fullbright.current.enabled = 0;
        film.current.enabled = 1;
        memcpy(jittered, projection, sizeof(jittered));
        WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 4.0f);
        WebSceneFx_JitterProjection(jittered);
        WebSceneFx_EndView();
        WebSceneFx_EndFrame();
        assert(composed[SCENE_FX_TAA] == 0 && composed[SCENE_FX_TAA_JITTER] == 0 && composed[SCENE_FX_TAA_JITTER + 1] == 0);
        assert(composed[SCENE_FX_SHARPEN] == 0.1f && !memcmp(jittered, projection, sizeof(jittered)));
        defaults();
    }
    puts("PASS: 600 composed frames, single compose per frame before Present, partial/fullbright/disabled views drawn directly, bounded glow, viewmodel and near world focus parameters, camera motion reprojected over 16 ms of the game's time from the views kept since the last cut, steady through frames bunched within its milliseconds, with aim fade, cuts and still cameras, a bounded menu backdrop and its gray, scenes rendered at r_sceneHeight lines and the screen's aspect on taller screens, a scope window composed with its rifle projection and near clip and a stencil drawn once per view and lifted on every exit, bounded window floats, the arms and weapon's own projection at the hip composing on its own behind a raised scope's, the outline a scope's rifle is kept off handed to the stencil and none for a sight's window, the stock overlay without a composed target, the sun's shadow map drawn once at the first full-screen view before its target binds, ambient occlusion composed on its own from the composed view's projection, the color maps' mip bias of a scaled scene, the composed view's near clip for effects, the map's haze with the view's up and sun taken when it begins, the sun's shafts from where it lies on the screen and how low it stands, explosions' shockwaves from where they went off, how long ago and how far away, bounded local contrast composed on its own, the film grade's lift of the shade in place of its contrast, and temporal antialiasing's jitter on every 3D draw, its history kept only while the camera moves on, and nothing from r_taa 0, r_fullbright or a failed bind");
    return 0;
}
'''

# Each mutant must fail the motion checks above.
mutants = [
    ('sum += then[i * 4 + j] * view[i * 4 + k];', 'sum += then[j * 4 + i] * view[k * 4 + i];'),
    ('rotation[k * 3 + j] = sum;', 'rotation[j * 3 + k] = sum;'),
    ('sum -= rotation[k * 3 + j] * view[12 + k];', 'sum += rotation[k * 3 + j] * view[12 + k];'),
    (' * (1.0f - s_aimFrac);', ';'),
    ('gap >= 0 && gap <= SCENE_FX_MOTION_MAX_GAP', 'gap >= 0 && gap <= 1000'),
    ('gap >= 0 && gap', 'gap'),
    # Frames within one of the game's milliseconds taken for cuts, as frames
    # under a millisecond apart were.
    ('gap >= 0 && gap', 'gap >= 1 && gap'),
    ('    s_cameraTime = time;\n', ''),
    ('moved < SCENE_FX_MOTION_MAX_MOVE', 'moved < 1e9f'),
    ('trace > 1.0f + 2.0f * cosf(SCENE_FX_MOTION_MAX_TURN)', 'trace > -1.0f'),
    # The old 0.35 rad limit cut a fast spin at 60 fps; past 0.66 rad is a cut.
    ('#define SCENE_FX_MOTION_MAX_TURN 0.6f', '#define SCENE_FX_MOTION_MAX_TURN 0.35f'),
    ('#define SCENE_FX_MOTION_MAX_TURN 0.6f', '#define SCENE_FX_MOTION_MAX_TURN 0.7f'),
    ('if (turn > SCENE_FX_MOTION_MIN_TURN || moved > SCENE_FX_MOTION_MIN_MOVE)', 'if (1)'),
    ('(float)(SCENE_FX_MOTION_EXPOSURE / (s_cameraTime - past->time))', '(float)(SCENE_FX_MOTION_EXPOSURE / 10.0)'),
    # The last frame's view in place of the one 16 ms before, a history that
    # keeps a view for every frame of a millisecond, one that reaches across
    # a cut, and one too short for the exposure.
    ('past = WebSceneFx_PastView(s_cameraTime - (int)SCENE_FX_MOTION_EXPOSURE);',
     'past = WebSceneFx_PastView(s_cameraTime - 1);'),
    ('    if (!s_pastViewCount || s_pastViews[s_pastViewNewest].time != time) {', '    {'),
    ('    if (!s_viewContinuous)\n        s_pastViewCount = 0;\n', ''),
    ('#define SCENE_FX_MOTION_HISTORY 32', '#define SCENE_FX_MOTION_HISTORY 8'),
    ('s_lastViewValid = valid;', 's_lastViewValid = 1;'),
    ('WebSceneFx_Enabled(r_motionBlur) && ', ''),
    (' && s_motionScale <= 0.0f && ', ' && '),
    (' && s_backdropFrac <= 0.0f &&\n', ' &&\n'),
    ('        s_backdropGray <= 0.0f && !WebSceneFx_ScopeRequested()', '        !WebSceneFx_ScopeRequested()'),
    ('!WebSceneFx_ScopeRequested() && s_gunProjection[0] <= 0.0f', 's_gunProjection[0] <= 0.0f'),
    ('s_gunProjection[0] <= 0.0f && !WebSceneFx_AoEnabled() &&\n', 's_gunProjection[0] <= 0.0f &&\n'),
    ('        !WebSceneFx_HazeEnabled() && !WebSceneFx_ShaftsEnabled()', '        !WebSceneFx_ShaftsEnabled()'),
    # The sun's shafts.
    (' && !WebSceneFx_ShaftsEnabled() && !WebSceneFx_BlastsActive() &&\n', ' && !WebSceneFx_BlastsActive() &&\n'),
    ('           s_hazeSunColor[0] + s_hazeSunColor[1] + s_hazeSunColor[2] > 0.0f;', '           1;'),
    ('    if (!WebSceneFx_ShaftsEnabled() || !(s_viewProjection[0] > 0.0f) || !(s_viewProjection[1] > 0.0f))',
     '    if (!WebSceneFx_ShaftsEnabled())'),
    ('    if (!(facing > 0.0f))\n        return 0.0f;\n', ''),
    ('    facing = facing < 1.0f ? facing * facing * (3.0f - 2.0f * facing) : 1.0f;',
     '    facing = facing < 1.0f ? facing : 1.0f;'),
    ('#define SCENE_FX_SHAFTS_FACING_START 0.15f', '#define SCENE_FX_SHAFTS_FACING_START 0.0f'),
    ('    sunUv[0] = WebSceneFx_Clamp(0.5f + 0.5f * s_hazeSun[0] * s_viewProjection[0] / s_hazeSun[2],',
     '    sunUv[0] = WebSceneFx_Clamp(0.5f - 0.5f * s_hazeSun[0] * s_viewProjection[0] / s_hazeSun[2],'),
    ('    sunUv[1] = WebSceneFx_Clamp(0.5f + 0.5f * s_hazeSun[1] * s_viewProjection[1] / s_hazeSun[2],',
     '    sunUv[1] = WebSceneFx_Clamp(0.5f + 0.5f * s_hazeSun[1] * s_viewProjection[0] / s_hazeSun[2],'),
    ('#define SCENE_FX_SHAFTS_REACH 1.5f', '#define SCENE_FX_SHAFTS_REACH 4.0f'),
    ('        WebSceneFx_Clamp((1.0f / s_hazeSunWorld[2] - SCENE_FX_SHAFTS_HIGH_AIR) /',
     '        WebSceneFx_Clamp((s_hazeSunWorld[2] - SCENE_FX_SHAFTS_HIGH_AIR) /'),
    ('                         (SCENE_FX_SHAFTS_LOW_AIR - SCENE_FX_SHAFTS_HIGH_AIR), 0.0f, 1.0f) : 1.0f;',
     '                         (SCENE_FX_SHAFTS_LOW_AIR - SCENE_FX_SHAFTS_HIGH_AIR), 0.0f, 1.0f) : 0.0f;'),
    ('(SCENE_FX_SHAFTS_GAIN_HIGH + (SCENE_FX_SHAFTS_GAIN_LOW - SCENE_FX_SHAFTS_GAIN_HIGH) * *low)',
     'SCENE_FX_SHAFTS_GAIN_HIGH'),
    ('    return WebSceneFx_Clamp(r_sunShafts->current.value, 0.0f, 2.0f) * facing *',
     '    return r_sunShafts->current.value * facing *'),
    ('    memcpy(params + SCENE_FX_SHAFTS_COLOR, s_hazeSunColor, sizeof(s_hazeSunColor));\n', ''),
    # Explosions' shockwaves.
    (' && !WebSceneFx_BlastsActive() &&', ' &&'),
    ('    return WebSceneFx_Enabled(r_shockwave) && !WebSceneFx_Enabled(r_fullbright);', '    return !WebSceneFx_Enabled(r_fullbright);'),
    ('        !(radius > 0.0f))\n        return;', '        0)\n        return;'),
    ('    if (!origin || !isfinite(origin[0]) || !isfinite(origin[1]) || !isfinite(origin[2]) || !isfinite(radius) ||',
     '    if (!origin || !isfinite(origin[0]) || !isfinite(radius) ||'),
    ('        if (s_blasts[i].time < s_blasts[oldest].time)', '        if (s_blasts[i].time > s_blasts[oldest].time)'),
    ('WebSceneFx_Clamp(radius, 0.0f, SCENE_FX_BLAST_MAX_RADIUS)', 'radius'),
    ('now - s_blasts[i].time < SCENE_FX_BLAST_MS)', 'now - s_blasts[i].time < 1e9)'),
    ('        if (!(p[2] > 1.0f))\n            continue;\n', ''),
    ('(1.0f - left * left * left)', 'life'),
    ('WebSceneFx_Clamp(SCENE_FX_BLAST_NEAR * blast->radius / distance, 0.0f, 1.0f)', '1.0f'),
    ('SCENE_FX_BLAST_BEND * left * left * near', 'SCENE_FX_BLAST_BEND * left * near'),
    ('WebSceneFx_Clamp(0.5f + 0.5f * p[0] * s_viewProjection[0] / p[2], -1.0f, 2.0f)',
     'WebSceneFx_Clamp(0.5f - 0.5f * p[0] * s_viewProjection[0] / p[2], -1.0f, 2.0f)'),
    ('WebSceneFx_Clamp(0.5f + 0.5f * p[1] * s_viewProjection[1] / p[2], -1.0f, 2.0f)',
     'WebSceneFx_Clamp(0.5f + 0.5f * p[1] * s_viewProjection[0] / p[2], -1.0f, 2.0f)'),
    ('        s_blastDepth[i] = p[2];', '        s_blastDepth[i] = distance;'),
    ('    memcpy(params + SCENE_FX_BLAST, s_blastView, sizeof(s_blastView));\n', ''),
    ('        WebSceneFx_HazeView();\n        WebSceneFx_BlastView();\n', '        WebSceneFx_HazeView();\n'),
    ('    memset(s_blastView, 0, sizeof(s_blastView));\n    memset(s_blastDepth, 0, sizeof(s_blastDepth));\n'
     '    if (s_cameraPending && WebSceneFx_CameraValid()) {',
     '    if (s_cameraPending && WebSceneFx_CameraValid()) {'),
    ('#define SCENE_FX_BLAST_REACH 1.6f', '#define SCENE_FX_BLAST_REACH 1.2f'),
    ('fminf(s_hazeDensity * SCENE_FX_HAZE_SHARE, SCENE_FX_HAZE_MAX_DENSITY)', 'fminf(s_hazeDensity, SCENE_FX_HAZE_MAX_DENSITY)'),
    ('fminf(s_hazeDensity * SCENE_FX_HAZE_SHARE, SCENE_FX_HAZE_MAX_DENSITY)', 's_hazeDensity * SCENE_FX_HAZE_SHARE'),
    ('    return WebSceneFx_Enabled(r_atmosphere) && !WebSceneFx_Enabled(r_fullbright) && s_hazeDensity > 0.0f;',
     '    return !WebSceneFx_Enabled(r_fullbright) && s_hazeDensity > 0.0f;'),
    ('    return WebSceneFx_Enabled(r_atmosphere) && !WebSceneFx_Enabled(r_fullbright) && s_hazeDensity > 0.0f;',
     '    return WebSceneFx_Enabled(r_atmosphere) && !WebSceneFx_Enabled(r_fullbright);'),
    ('        s_hazeUp[j] = view[8 + j];', '        s_hazeUp[j] = view[j * 4 + 2];'),
    ('view[j] * s_hazeSunWorld[0] + view[4 + j] * s_hazeSunWorld[1] + view[8 + j] * s_hazeSunWorld[2]',
     'view[j * 4] * s_hazeSunWorld[0] + view[j * 4 + 1] * s_hazeSunWorld[1] + view[j * 4 + 2] * s_hazeSunWorld[2]'),
    ('        s_viewProjection[1] = s_cameraProjection[1];\n        WebSceneFx_HazeView();\n',
     '        s_viewProjection[1] = s_cameraProjection[1];\n'),
    ('        s_hazeSunWorld[i] = valid ? sunDirection[i] / length : 0.0f;',
     '        s_hazeSunWorld[i] = valid ? sunDirection[i] : 0.0f;'),
    ('    valid = isfinite(length) && length > 0.0f;', '    valid = isfinite(length);'),
    ('WebSceneFx_Clamp(fogDensity, 0.0f, 0.01f)', 'fogDensity'),
    ('        valid = isfinite(fogColor[i]);', '        valid = 1;'),
    ('        valid = isfinite(sunColor[i]);', '        valid = 1;'),
    ('    params[SCENE_FX_HAZE_SUN_COLOR] = s_hazeSunColor[0] * SCENE_FX_HAZE_SUN_GLOW;',
     '    params[SCENE_FX_HAZE_SUN_COLOR] = s_hazeSunColor[0];'),
    ('    params[SCENE_FX_HAZE_FALLOFF] = 1.0f / SCENE_FX_HAZE_SCALE_HEIGHT;',
     '    params[SCENE_FX_HAZE_FALLOFF] = SCENE_FX_HAZE_SCALE_HEIGHT;'),
    ('    return s_active ? s_worldNearClip : 0.0f;', '    return s_worldNearClip;'),
    ('    params[SCENE_FX_AO] = WebSceneFx_AoEnabled() && s_viewProjection[0] > 0.0f ? SCENE_FX_AO_STRENGTH : 0.0f;',
     '    params[SCENE_FX_AO] = SCENE_FX_AO_STRENGTH;'),
    ('    return WebSceneFx_Enabled(r_ssao) && !WebSceneFx_Enabled(r_fullbright);', '    return 1;'),
    ('    if (s_cameraPending && WebSceneFx_CameraValid()) {', '    if (WebSceneFx_CameraValid()) {'),
    ('    params[SCENE_FX_AO_PROJECTION + 1] = s_viewProjection[1];', '    params[SCENE_FX_AO_PROJECTION + 1] = s_cameraProjection[1];'),
    ('    return s_active ? s_lodBias : 0.0f;', '    return s_lodBias;'),
    ('WebSceneFx_Clamp(log2f((float)sceneHeight / (float)height), -1.0f, 0.0f)',
     'WebSceneFx_Clamp(log2f((float)sceneHeight / (float)height), -4.0f, 0.0f)'),
    ('    s_scopeStencil = 0;\n    WebSunShadows_Render();\n', '    s_scopeStencil = 0;\n'),
    ('    s_scopeUnavailable = !s_active;\n', '    s_scopeUnavailable = 0;\n'),
    ('        s_scopeUnavailable = 1;\n        return;', '        return;'),
    ('    if (!s_scopeStencil) {', '    if (1) {'),
    ('    s_frameUsed = 1;\n    s_scopeStencil = 0;\n', '    s_frameUsed = 1;\n'),
    ('    WebSceneFx_ScopeMask(!s_scopeExempt);\n    s_scopeMasking = 1;', '    WebSceneFx_ScopeMask(!s_scopeExempt);'),
    ('    WebSceneFx_ScopeMask(!s_scopeExempt);\n', '    WebSceneFx_ScopeMask(1);\n'),
    ('        exempt = !strncmp(name, "mtl_browser_att_", 16);', '        exempt = 0;'),
    ('    if (s_active && name && WebSceneFx_SightWindow())', '    if (s_active && name)'),
    ('    if (s_scopeMasking)\n        WebSceneFx_ScopeMask(!exempt);\n', '    WebSceneFx_ScopeMask(!exempt);\n'),
    ('           !(s_scope[SCENE_FX_SCOPE_PROJECTION - SCENE_FX_SCOPE] > 0.0f);', '           1;'),
    ('    WebSceneFx_EndViewmodel();\n    if (!s_active)\n        return;\n    s_active = 0;',
     '    if (!s_active)\n        return;\n    s_active = 0;'),
    ('    return s_active ? WebSceneFx_ViewmodelScales() : NULL;', '    return WebSceneFx_ViewmodelScales();'),
    # The weapon's own projection: not composing the view, ignored, put
    # before a raised scope's, or kept when infinite or negative.
    ('s_gunProjection[0] <= 0.0f && ', ''),
    ('    return s_gunProjection[0] > 0.0f ? s_gunProjection : NULL;', '    return NULL;'),
    ('    if (projection[0] > 0.0f)\n        return projection;\n', ''),
    ('isfinite(projection[0]) && isfinite(projection[1]) && projection[0] > 0.0f',
     'isfinite(projection[0]) && projection[0] > 0.0f'),
    ('projection[0] > 0.0f &&\n        projection[1] > 0.0f) {', 'projection[0] > 0.0f) {'),
    ('    memcpy(params + SCENE_FX_SCOPE, s_scope, sizeof(s_scope));\n', ''),
    ('        s_scope[i - SCENE_FX_SCOPE] = WebSceneFx_Clamp(s_scope[i - SCENE_FX_SCOPE], 0.0f, 1.0f);', '        (void)0;'),
    ('    if (!(fabsf(axes[0] * axes[3] - axes[2] * axes[1]) > 1e-6f))\n'
     '        s_scope[SCENE_FX_SCOPE_PRESENCE - SCENE_FX_SCOPE] = 0.0f;\n', ''),
    ('    if (!(projection[0] > 0.0f) || !(projection[1] > 0.0f))\n        projection[0] = projection[1] = 0.0f;\n', ''),
    ('    *nearClip = WebSceneFx_Clamp(*nearClip, 0.0f, 16.0f);\n', ''),
    ('        s_scopeBell[i] = isfinite(scope[SCENE_FX_SCOPE_FLOATS + i]) ? scope[SCENE_FX_SCOPE_FLOATS + i] : 0.0f;',
     '        s_scopeBell[i] = scope[SCENE_FX_SCOPE_FLOATS + i];'),
    ('    if (!(projection[0] > 0.0f) || !(fabsf(bellAxes[0] * bellAxes[3] - bellAxes[2] * bellAxes[1]) > 1e-6f))\n'
     '        memset(s_scopeBell, 0, sizeof(s_scopeBell));\n', ''),
    ('    if (!(projection[0] > 0.0f) || !(fabsf(bellAxes[0] * bellAxes[3] - bellAxes[2] * bellAxes[1]) > 1e-6f))',
     '    if (!(fabsf(bellAxes[0] * bellAxes[3] - bellAxes[2] * bellAxes[1]) > 1e-6f))'),
    ('    if (!(projection[0] > 0.0f) || !(fabsf(bellAxes[0] * bellAxes[3] - bellAxes[2] * bellAxes[1]) > 1e-6f))',
     '    if (!(projection[0] > 0.0f))'),
    ('        WebSceneFx_ScopeStencil(s_scope, s_scopeBell);', '        WebSceneFx_ScopeStencil(s_scope, s_scope);'),
    ('    return scope > nearClip ? scope : nearClip;', '    return scope;'),
    ('    return s_active ? WebSceneFx_ScopeNearClip(nearClip) : nearClip;',
     '    return WebSceneFx_ScopeNearClip(nearClip);'),
    ('params[SCENE_FX_NEAR_CLIP] = WebSceneFx_ScopeNearClip(s_nearClip);', 'params[SCENE_FX_NEAR_CLIP] = s_nearClip;'),
    ('params[SCENE_FX_BACKDROP] = s_backdropFrac;', 'params[SCENE_FX_BACKDROP] = 0.0f;'),
    ('s_backdropFrac = WebSceneFx_Clamp(backdropFrac, 0.0f, 1.0f);', 's_backdropFrac = backdropFrac;'),
    ('params[SCENE_FX_BACKDROP_GRAY] = s_backdropGray;', 'params[SCENE_FX_BACKDROP_GRAY] = s_backdropFrac;'),
    ('s_backdropGray = WebSceneFx_Clamp(gray, 0.0f, 1.0f);', 's_backdropGray = gray;'),
    ('!scaled && s_grayFrac <= 0.0f && ', '!scaled && '),
    ('    if (s_grayFrac > params[SCENE_FX_BACKDROP_GRAY])\n        params[SCENE_FX_BACKDROP_GRAY] = s_grayFrac;\n', ''),
    ('params[SCENE_FX_CONTRAST] = 1.2f * s_grayFrac;', 'params[SCENE_FX_CONTRAST] = 0.0f * s_grayFrac;'),
    ('params[SCENE_FX_SATURATION] = 1.12f;', 'params[SCENE_FX_SATURATION] = 0.96f;'),
    ('params[SCENE_FX_TONE] = film ? SCENE_FX_SHADE_LIFT * (1.0f - s_grayFrac) : 0.0f;',
     'params[SCENE_FX_TONE] = film ? SCENE_FX_SHADE_LIFT : 0.0f;'),
    ('params[SCENE_FX_TONE] = film ? SCENE_FX_SHADE_LIFT * (1.0f - s_grayFrac) : 0.0f;',
     'params[SCENE_FX_TONE] = SCENE_FX_SHADE_LIFT * (1.0f - s_grayFrac);'),
    ('s_grayFrac = WebSceneFx_Clamp(grayFrac, 0.0f, 1.0f);', 's_grayFrac = grayFrac;'),
    ('params[SCENE_FX_MOTION_PROJECTION + 1] = s_motionProjection[1];',
     'params[SCENE_FX_MOTION_PROJECTION + 1] = s_motionProjection[0];'),
    ('    WebSceneFx_TrackCamera();\n    WebSceneFx_TrackViewmodel(sceneWidth, sceneHeight, nearClip);\n'
     '    /* r_fullbright clears the frame buffer before it begins the view. */\n'
     '    if (WebSceneFx_Enabled(r_fullbright)) {\n        s_scopeUnavailable = 1;\n        return;\n    }\n',
     '    /* r_fullbright clears the frame buffer before it begins the view. */\n'
     '    if (WebSceneFx_Enabled(r_fullbright)) {\n        s_scopeUnavailable = 1;\n        return;\n    }\n'
     '    WebSceneFx_TrackCamera();\n    WebSceneFx_TrackViewmodel(sceneWidth, sceneHeight, nearClip);\n'),
    ('    if (lines > 0 && lines < height) {', '    if (lines > 0) {'),
    ('(float)lines / (float)height + 0.5f);', '(float)lines / (float)height);'),
    ('        if (*sceneWidth < 1)\n            *sceneWidth = 1;\n', ''),
    ('    scaled = sceneWidth != width || sceneHeight != height;', '    scaled = 0;'),
    ('WebSceneFx_Bind(sceneWidth, sceneHeight, samples, width, height);',
     'WebSceneFx_Bind(width, height, samples, width, height);'),
    # Local contrast.
    ('    return WebSceneFx_Clamp(r_clarity->current.value, 0.0f, 2.0f) * SCENE_FX_CLARITY_GAIN;',
     '    return WebSceneFx_Clamp(r_clarity->current.value, 0.0f, 2.0f);'),
    ('    return WebSceneFx_Clamp(r_clarity->current.value, 0.0f, 2.0f) * SCENE_FX_CLARITY_GAIN;',
     '    return r_clarity->current.value * SCENE_FX_CLARITY_GAIN;'),
    ('        WebSceneFx_Clarity() <= 0.0f && !WebSceneFx_TaaEnabled())', '        !WebSceneFx_TaaEnabled())'),
    ('    params[SCENE_FX_CLARITY] = WebSceneFx_Clarity();\n', ''),
    # Temporal antialiasing.
    (' && !WebSceneFx_TaaEnabled())\n        return;', ')\n        return;'),
    ('    return WebSceneFx_Enabled(r_taa) && !WebSceneFx_Enabled(r_fullbright);', '    return !WebSceneFx_Enabled(r_fullbright);'),
    ('        projection[column * 4 + 1] += s_jitter[1] * projection[column * 4 + 3];',
     '        projection[column * 4 + 1] += s_jitter[1];'),
    ('        projection[column * 4] += s_jitter[0] * projection[column * 4 + 3];',
     '        projection[column * 4] += s_jitter[1] * projection[column * 4 + 3];'),
    ('    if (!s_active || !projection || (s_jitter[0] == 0.0f && s_jitter[1] == 0.0f))',
     '    if (!projection || (s_jitter[0] == 0.0f && s_jitter[1] == 0.0f))'),
    ('        unsigned index = s_jitterPhase++ % SCENE_FX_TAA_PHASES + 1;',
     '        unsigned index = s_jitterPhase++ % SCENE_FX_TAA_PHASES;'),
    ('#define SCENE_FX_TAA_PHASES 8', '#define SCENE_FX_TAA_PHASES 4'),
    ('        s_jitter[0] = (WebSceneFx_Halton(index, 2) - 0.5f) * 2.0f / (float)sceneWidth;',
     '        s_jitter[0] = WebSceneFx_Halton(index, 2) * 2.0f / (float)sceneWidth;'),
    ('        s_jitter[1] = (WebSceneFx_Halton(index, 3) - 0.5f) * 2.0f / (float)sceneHeight;',
     '        s_jitter[1] = (WebSceneFx_Halton(index, 2) - 0.5f) * 2.0f / (float)sceneHeight;'),
    ('        result += fraction * (float)(index % base);', '        result += fraction * (float)(index % 2);'),
    ('    s_jitter[0] = s_jitter[1] = 0.0f;\n    if (s_taaView) {', '    if (s_taaView) {'),
    ('#define SCENE_FX_TAA_HISTORY 0.85f', '#define SCENE_FX_TAA_HISTORY 0.5f'),
    ('            1.0f - SCENE_FX_TAA_HISTORY : 1.0f;', '            1.0f - SCENE_FX_TAA_HISTORY : 1.0f - SCENE_FX_TAA_HISTORY;'),
    ('s_viewContinuous && s_viewProjection[0] > 0.0f && s_taaLastProjection[0] > 0.0f ?',
     's_viewProjection[0] > 0.0f && s_taaLastProjection[0] > 0.0f ?'),
    ('            memcpy(s_taaRotation, rotation, sizeof(s_taaRotation));\n', ''),
    ('            memcpy(s_taaMove, move, sizeof(s_taaMove));\n', ''),
    ('            s_taaLastProjection[1] = s_lastProjection[1];', '            s_taaLastProjection[1] = s_lastProjection[0];'),
    ('    s_lastProjection[0] = s_cameraProjection[0];\n    s_lastProjection[1] = s_cameraProjection[1];\n', ''),
    ('    params[SCENE_FX_SHARPEN] = film ? (s_taaView ? 0.2f : 0.1f) : 0.0f;',
     '    params[SCENE_FX_SHARPEN] = film ? 0.1f : 0.0f;'),
    ('    params[SCENE_FX_TAA_JITTER + 1] = s_jitter[1];\n', ''),
    ('    s_taaView = s_active && WebSceneFx_TaaEnabled();', '    s_taaView = WebSceneFx_TaaEnabled();'),
]

with tempfile.TemporaryDirectory(prefix='cod2-scene-fx-') as tmp:
    path = Path(tmp)
    (path / 'emscripten.h').write_text(emscripten)
    (path / 'common_types.h').write_text(common_types)
    (path / 'test.c').write_text(test.replace('SOURCE', str(source)))
    subprocess.run(['cc', '-std=c99', '-O1', '-g', '-Wall', '-Werror', '-I', str(path),
                    '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
    original = source.read_text()
    for index, (before, after) in enumerate(mutants):
        assert original.count(before) == 1, before
        mutant = path / f'mutant{index}.c'
        mutant.write_text(original.replace(before, after))
        (path / f'mutant{index}-test.c').write_text(test.replace('SOURCE', str(mutant)))
        subprocess.run(['cc', '-std=c99', '-O0', '-w', '-I', str(path), str(path / f'mutant{index}-test.c'),
                        '-lm', '-o', str(path / f'mutant{index}')], check=True)
        result = subprocess.run([str(path / f'mutant{index}')], capture_output=True)
        assert result.returncode != 0, f'mutant survived: {after!r}'
    print(f'PASS: {len(mutants)} motion blur, backdrop, scene size, scope window, haze, sun shaft, shockwave, local contrast, tone and temporal antialiasing mutants fail')
