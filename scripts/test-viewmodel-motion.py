#!/usr/bin/env python3
"""Exercise the viewmodel's own motion blur: the renderer hands the scene
compositor every viewmodel surface it skins, and the compositor turns each
vertex's screen motion since the last frame into the mesh its velocity pass
draws."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = root / 'downstream/wasm/web_scene_fx.c'
compositor = source.read_text()
model_path = root / 'src/PC/gfx_d3d/r_model.c'
model = model_path.read_text()
headers = (root / 'src/headers/common_types.h').read_text()


def function(text, signature):
    start = text.index(signature + '\n{')
    return text[start:text.index('\n}\n', start) + 3]


# The renderer hands over each model surface right after skinning it, in both
# skinning commands, from the buffer the skinning wrote: the viewmodel's to
# the compositor, every other model's to the sun's shadow map.
skin = function(model, 'void R_SkinXModelCmd(SkinXModelCmd *skinCmd, int context)')
assert skin.index('R_SkinXSurfaceSkinned((struct GfxModelSkinnedSurface *)surfPos, boneMatrix);') < \
    skin.index('R_AddWebSkinnedSurface(skinCmd->e, (const struct GfxModelSkinnedSurface *)surfPos);')
assert skin.index('(vec3_t *)(rigidSurf + 8));') < \
    skin.index('R_AddWebRigidSurface(skinCmd->e, (const GfxModelRigidSurface *)rigidSurf);')
rigid = function(model, 'void R_SkinRigidXModelCmd(SkinRigidXModelCmd *skinRigidCmd)')
assert rigid.index('(vec3_t *)(rigidSurf + 8));') < \
    rigid.index('R_AddWebRigidSurface(refEnt, (const GfxModelRigidSurface *)rigidSurf);')
assert rigid.index('R_SkinXSurfaceSkinned((struct GfxModelSkinnedSurface *)surf, (const DObjSkelMat *)mtx);') < \
    rigid.index('R_AddWebSkinnedSurface(refEnt, (const struct GfxModelSkinnedSurface *)surf);')
for body in (skin, rigid):
    calls = list(re.finditer(r'R_AddWeb(?:Skinned|Rigid)Surface\(', body))
    assert len(calls) == 2
    for call in calls:
        assert body.rfind('#ifdef __EMSCRIPTEN__', 0, call.start()) > body.rfind('#endif', 0, call.start())
assert 'out = R_SkinnedSurfaceVerts(skinnedSurf);' in function(
    model, 'static void R_SkinXSurfaceSkinned(struct GfxModelSkinnedSurface *skinnedSurf,\n'
           '                                  const DObjSkelMat *boneMatrix)')
# Each frame's front end collects a new set of surfaces.
frames = (root / 'src/PC/gfx_d3d/r_rendercmds.c').read_text()
begin = function(frames, 'void R_BeginFrame(void)')
assert begin.index('#ifdef __EMSCRIPTEN__') < begin.index('WebSceneFx_BeginFrame();') < begin.index('R_BeginFrame_impl();')
# The view weapon is the depth-hacked entity: it draws into depth [0, 0.2]
# with the depth hack's near clip as the projection's fourth-row z term.
assert 'gun.renderFxFlags = 0x8c;' in (root / 'src/PC/cgame_mp/cg_weapons.c').read_text() and (0x8c & 0x18) == 8
backend = (root / 'src/PC/gfx_d3d/rb_backend.c').read_text()
assert re.search(r'if \(depthRange == 8\) \{\s+#ifdef __EMSCRIPTEN__\s+'
                 r'RB_SetViewmodelProjection\(WebSceneFx_ViewmodelNearClip\(backEnd\.viewParms->depthHackNearClip\),'
                 r'\s+WebSceneFx_ViewmodelProjection\(\)\);\s+WebSceneFx_BeginViewmodel\(\);\s+#else'
                 r'\s+RB_SetDepthHackNearClip\(backEnd\.viewParms->depthHackNearClip\);\s+#endif'
                 r'\s+RB_SetDepthRange\(0\.0f, 0\.2f\);', backend)
rb_state = (root / 'src/PC/gfx_d3d/rb_state.c').read_text()
assert 'activeMatrices->projection.matrix[0]._43 = -nearClip;' in rb_state
assert function(rb_state, 'void RB_SetViewmodelProjection(float nearClip, const float *scale)').count(
    '    RB_SetDepthHackNearClip(nearClip);\n') == 1
# The compositor reads the viewmodel parameters where the C side writes them,
# uploads the rows the C side fills, and binds no buffer the GL emulation
# tracks: its vertices come from the mesh texture.
for text in ('var gray = H[o + 34], smear = H[o + 35];',
             'gl.drawArrays(gl.TRIANGLES, 0, 3 * H[o + 36]);',
             'gl.uniform2f(P.viewmodel.u_vm, H[o + 37], 1 / smear);',
             'gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, 1024, H[o + 39], gl.RGBA, gl.FLOAT, H, mesh);',
             'gl.uniform4f(C.u_vm, vm ? smear : 0, H[o + 40], H[o + 38], H[o + 41]);',
             'vm.mesh = texture(gl.RGBA32F, 1024, 64);',
             'if (fx && fx.t) fx.compose(params >>> 2, mesh >>> 2);',
             'WebSceneFx_Compose(params, s_vmMesh);'):
    assert compositor.count(text) == 1, text
assert 'bindBuffer' not in compositor and 'vertexAttribPointer' not in compositor

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
    motion, scene, ssao, atmosphere;
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
void WebSunShadows_Render(void)
{
}
static double clockMs = 1000.0;
double emscripten_get_now(void)
{
    return clockMs;
}
static int binds, composes;
static float composed[SCENE_FX_PARAM_COUNT];
int WebSceneFx_Bind(int width, int height, int sampleCount, int screenWidth, int screenHeight)
{
    ++binds;
    return 1;
}
void WebSceneFx_Compose(const float *params, const float *mesh)
{
    ++composes;
    memcpy(composed, params, sizeof(composed));
    assert(mesh == s_vmMesh);
}
void WebSceneFx_ScopeStencil(const float *scope, const float *bell)
{
    (void)scope;
    (void)bell;
}
void WebSceneFx_ScopeMask(int on)
{
    (void)on;
}
/* Every effect but motion blur off: only motion composes. */
static void quiet(void)
{
    glow.current.enabled = 0; film.current.enabled = 0; samples.current.integer = 1; dof.current.enabled = 0;
    fullbright.current.enabled = 0; motion.current.enabled = 1; scene.current.integer = 720;
}
/* A D3D view (row vectors) of CoD's x-forward, y-left, z-up world, pitch
 * positive down, and its 1.19 x 2.11 projection with r_znear 4. */
static float lens[16] = {1.19f, 0, 0, 0, 0, 2.11f, 0, 0, 0, 0, 0.9995f, 1, 0, 0, -3.998f, 0};
static float eyeForward[3], eyeLeft[3], eyeUp[3], eyeOrigin[3];
static void camera(float yaw, float pitch, float x, float y, float z)
{
    float view[16] = {0};
    const float f[3] = {cosf(pitch) * cosf(yaw), cosf(pitch) * sinf(yaw), -sinf(pitch)};
    const float l[3] = {-sinf(yaw), cosf(yaw), 0};
    const float u[3] = {sinf(pitch) * cosf(yaw), sinf(pitch) * sinf(yaw), cosf(pitch)};
    const float o[3] = {x, y, z};
    for (int i = 0; i < 3; ++i) {
        view[i * 4] = -l[i]; view[i * 4 + 1] = u[i]; view[i * 4 + 2] = f[i];
        view[12] += l[i] * o[i]; view[13] -= u[i] * o[i]; view[14] -= f[i] * o[i];
        eyeForward[i] = f[i]; eyeLeft[i] = l[i]; eyeUp[i] = u[i]; eyeOrigin[i] = o[i];
    }
    view[15] = 1;
    WebSceneFx_SetCamera(view, lens);
}
/* A 4 x 2 quad facing the eye, its model x along the camera's forward. */
static const float corners[4][3] = {{0, 2, 1}, {0, -2, 1}, {0, -2, -1}, {0, 2, -1}};
static const unsigned short quadTriangles[6] = {0, 1, 2, 0, 2, 3};
static char keyA, keyB;
/* Where the renderer puts a quad: ahead of the camera, right of and above
 * its axis, in the camera's own basis. */
static void placement(float *transform, float ahead, float right, float up)
{
    for (int j = 0; j < 3; ++j) {
        transform[j] = eyeForward[j]; transform[3 + j] = eyeLeft[j]; transform[6 + j] = eyeUp[j];
        transform[9 + j] = eyeOrigin[j] + ahead * eyeForward[j] - right * eyeLeft[j] + up * eyeUp[j];
    }
}
static void quad(const void *key, float ahead, float right, float up)
{
    float transform[12];
    placement(transform, ahead, right, up);
    WebSceneFx_AddViewmodelSurface(key, &corners[0][0], (int)sizeof(corners[0]), 4, quadTriangles, 2, transform);
}
/* A frame ms after the last: the front end begins and skins its viewmodel,
 * then the back end begins the camera's view and composes. */
static void begin(float ms, float yaw, float x)
{
    clockMs += ms;
    WebSceneFx_BeginFrame();
    camera(yaw, 0.1f, x, 200, 60);
}
static void present(void)
{
    WebSceneFx_BeginView(0, 0, 1280, 720, 1280, 720, 0.1f, 4.0f);
    WebSceneFx_EndView();
    WebSceneFx_EndFrame();
}
static float smearOf(int vertex, int axis)
{
    return s_vmMesh[vertex * 8 + 4 + axis];
}
static int near(float value, float expected)
{
    return fabsf(value - expected) < 1e-3f * (1.0f + fabsf(expected));
}
/* Under a hundredth of a pixel: float rounding, not motion. */
static int still(float smear)
{
    return fabsf(smear) < 0.01f;
}
/* A corner (mx, my, mz) of a quad placed (ahead, right, up) sits at view x
 * right - my, y up + mz, z ahead + mx, whatever the camera. */
static float ndcX(int corner, float ahead, float right)
{
    return 1.19f * (right - corners[corner][1]) / (ahead + corners[corner][0]);
}
/* The half smear spans 5 ms of motion: a quarter of the motion in normalized
 * device coordinates, times the screen's size, times 10 ms over the gap. */
static float smearX(float ndcBefore, float ndcNow, float gap)
{
    return 0.25f * (ndcNow - ndcBefore) * 1280.0f * 10.0f / gap;
}
/* Past the knee, 0.4% of the height (2.88 pixels), each pixel of a half
 * smear counts 3.5 times, up to 10% of the height (72 pixels). */
static float boosted(float smear)
{
    float length = fabsf(smear), out = length <= 2.88f ? length : fminf(2.88f + 3.5f * (length - 2.88f), 72.0f);
    return smear < 0.0f ? -out : out;
}
static float tileX(int tx, int ty)
{
    return s_vmMesh[(s_vmTileRow * SCENE_FX_VM_MESH_WIDTH + ty * s_vmTilesX + tx) * 4];
}
static float bigPositions[20000 * 3];
int main(void)
{
    assert(SCENE_FX_VM_MOTION == 35 && SCENE_FX_VM_TRIANGLES == 36 && SCENE_FX_VM_INDEX_ROW == 37 &&
           SCENE_FX_VM_TILE_ROW == 38 && SCENE_FX_VM_ROWS == 39 && SCENE_FX_VM_TILE == 40 &&
           SCENE_FX_VM_TILES_X == 41 && SCENE_FX_SCOPE == 42 && SCENE_FX_SCOPE_END == 55 && SCENE_FX_AO == 55 && SCENE_FX_HAZE == 59 && SCENE_FX_PARAM_COUNT == 73);
    /* The mesh texture holds the most vertices, indices and tiles at once. */
    assert(SCENE_FX_VM_MESH_WIDTH == 1024 && SCENE_FX_VM_MESH_ROWS == 64);
    assert(SCENE_FX_VM_MAX_VERTICES * 2 / 1024 + SCENE_FX_VM_MAX_TRIANGLES * 3 / 4096 +
           SCENE_FX_VM_MAX_TILES / 1024 <= SCENE_FX_VM_MESH_ROWS);
    quiet();

    /* A viewmodel seen for the first time has no motion yet: nothing composes. */
    begin(10, 0.3f, 100); quad(&keyA, 20, 0, 0); present();
    assert(!composes && s_vmMotion == 0);
    /* The quad moves half a unit right in 10 ms: each corner smears by its
     * own screen motion, and the camera standing still composes it alone. */
    begin(10, 0.3f, 100); quad(&keyA, 20, 0.5f, 0); present();
    assert(composes == 1 && binds == 1);
    for (int i = 0; i < 4; ++i) {
        float z = 20 + corners[i][0], x = 0.5f - corners[i][1], y = corners[i][2];
        assert(near(smearOf(i, 0), boosted(smearX(ndcX(i, 20, 0), ndcX(i, 20, 0.5f), 10))) && still(smearOf(i, 1)));
        /* Its GL clip position, with the depth hack's 0.1 near clip. */
        assert(near(s_vmMesh[i * 8], 1.19f * x) && near(s_vmMesh[i * 8 + 1], 2.11f * y));
        assert(near(s_vmMesh[i * 8 + 2], 2 * (0.9995f * z - 0.1f) - z) && near(s_vmMesh[i * 8 + 3], z));
    }
    /* 9.52 pixels of motion: 2.88 to the knee, and 3.5 times the 6.64 past it. */
    assert(near(composed[SCENE_FX_VM_MOTION], 26.12f) && composed[SCENE_FX_MOTION] == 0);
    /* Two triangles after one row of vertices, then the tiles: 72 pixels, as
     * wide as the longest half smear, 18 x 10 of them in one row. */
    assert(composed[SCENE_FX_VM_TRIANGLES] == 2 && composed[SCENE_FX_VM_INDEX_ROW] == 1);
    assert(composed[SCENE_FX_VM_TILE_ROW] == 2 && composed[SCENE_FX_VM_ROWS] == 3);
    assert(composed[SCENE_FX_VM_TILE] == 72 && composed[SCENE_FX_VM_TILES_X] == 18);
    for (int k = 0; k < 6; ++k)
        assert(s_vmMesh[SCENE_FX_VM_MESH_WIDTH * 4 + k] == quadTriangles[k]);
    /* The quad covers pixels 583-735 by 322-398, tiles 8-10 by 4-5. Each
     * tile holds the largest smear within one tile of it. */
    for (int ty = 0; ty < 10; ++ty)
        for (int tx = 0; tx < 18; ++tx) {
            int reached = tx >= 7 && tx <= 11 && ty >= 3 && ty <= 6;
            assert(reached ? near(tileX(tx, ty), 26.12f) : tileX(tx, ty) == 0);
        }

    /* The camera turns and walks with the viewmodel in its hands: the world
     * smears, the viewmodel does not. */
    begin(10, 0.34f, 101.5f); quad(&keyA, 20, 0.5f, 0); present();
    assert(composes == 2 && composed[SCENE_FX_MOTION] == 1.6f && composed[SCENE_FX_VM_MOTION] == 0);
    for (int i = 0; i < 4; ++i)
        assert(still(smearOf(i, 0)) && still(smearOf(i, 1)));

    /* Any frame rate smears the same 10 ms of motion; one within the knee,
     * the bob's and sway's, stays as it is. */
    begin(5, 0.34f, 101.5f); quad(&keyA, 20, 0.75f, 0); present();
    assert(near(composed[SCENE_FX_VM_MOTION], 26.12f));
    begin(40, 0.34f, 101.5f); quad(&keyA, 20, 2.75f, 0); present();
    assert(near(composed[SCENE_FX_VM_MOTION], 26.12f));
    begin(40, 0.34f, 101.5f); quad(&keyA, 20, 3.25f, 0); present();
    assert(near(composed[SCENE_FX_VM_MOTION], 2.38f));
    /* No half smear is longer than 10% of the screen's height, 72 pixels:
     * 28.56 pixels of motion would make 92.76. It keeps its direction. */
    begin(10, 0.34f, 101.5f); quad(&keyA, 20, 1.75f, 0); present();
    assert(near(composed[SCENE_FX_VM_MOTION], 72.0f) && near(smearOf(0, 0), -72.0f) && still(smearOf(0, 1)));

    /* Cuts start over: a camera jump of 48 units or more, a gap past 100 ms
     * or under 1 ms. Each still records where the viewmodel was. */
    int before = composes;
    begin(10, 0.34f, 161.5f); quad(&keyA, 20, 1.25f, 0); present();
    begin(150, 0.34f, 161.5f); quad(&keyA, 20, 1.75f, 0); present();
    begin(0.5f, 0.34f, 161.5f); quad(&keyA, 20, 2.25f, 0); present();
    assert(composes == before && s_vmMotion == 0);
    begin(10, 0.34f, 161.5f); quad(&keyA, 20, 2.75f, 0); present();
    assert(composes == before + 1 && near(composed[SCENE_FX_VM_MOTION], 26.12f));
    /* A surface that moved over a whole screen width on average was swapped
     * for another pose. */
    begin(10, 0.34f, 161.5f); quad(&keyA, 20, -10, 0); present();
    begin(10, 0.34f, 161.5f); quad(&keyA, 20, 10, 0); present();
    assert(s_vmMotion == 0);
    /* A frame without the viewmodel leaves nothing to smear from. */
    begin(10, 0.34f, 161.5f); present();
    begin(10, 0.34f, 161.5f); quad(&keyA, 20, 0.5f, 0); present();
    assert(s_vmMotion == 0);
    /* Another surface, or the same with other vertices, is not the last one. */
    begin(10, 0.34f, 161.5f); quad(&keyB, 20, 1, 0); present();
    assert(s_vmMotion == 0);
    {
        float transform[12];
        placement(transform, 20, 1.5f, 0);
        begin(10, 0.34f, 161.5f);
        WebSceneFx_AddViewmodelSurface(&keyB, &corners[0][0], (int)sizeof(corners[0]), 3, quadTriangles, 1, transform);
        present();
        assert(s_vmMotion == 0);
    }
    /* Surfaces met twice keep their order: each smears by its own motion. A
     * later surface's indices follow the earlier surfaces' vertices. */
    begin(10, 0.34f, 161.5f); quad(&keyA, 20, 0, 3); quad(&keyA, 20, 0, -3); present();
    begin(10, 0.34f, 161.5f); quad(&keyA, 20, 0.5f, 3); quad(&keyA, 20, -0.5f, -3); present();
    assert(near(smearOf(0, 0), 26.12f) && near(smearOf(4, 0), -26.12f) && still(smearOf(0, 1)) && still(smearOf(4, 1)));
    assert(composed[SCENE_FX_VM_TRIANGLES] == 4);
    for (int k = 0; k < 6; ++k)
        assert(s_vmMesh[SCENE_FX_VM_MESH_WIDTH * 4 + 6 + k] == quadTriangles[k] + 4);

    /* r_motionBlur 0 smears nothing and keeps tracking. */
    motion.current.enabled = 0;
    before = composes;
    begin(10, 0.34f, 161.5f); quad(&keyA, 20, 1, 3); quad(&keyA, 20, -1, -3); present();
    assert(composes == before && s_vmMotion == 0);
    motion.current.enabled = 1;
    begin(10, 0.34f, 161.5f); quad(&keyA, 20, 1.5f, 3); quad(&keyA, 20, -1.5f, -3); present();
    assert(composes == before + 1 && near(composed[SCENE_FX_VM_MOTION], 26.12f));
    /* So do r_fullbright frames, which compose nothing. */
    fullbright.current.enabled = 1;
    begin(10, 0.34f, 161.5f); quad(&keyA, 20, 2, 0); present();
    fullbright.current.enabled = 0;
    assert(composes == before + 1);
    begin(10, 0.34f, 161.5f); quad(&keyA, 20, 2.5f, 0); present();
    assert(composes == before + 2 && near(composed[SCENE_FX_VM_MOTION], 26.12f));
    /* A partial view tracks nothing: the next full view measures from the
     * last full one, 20 ms before, whose 4.76 pixels of motion smear 9.46. */
    clockMs += 10;
    WebSceneFx_BeginFrame();
    camera(0.34f, 0.1f, 161.5f, 200, 60);
    quad(&keyA, 20, 2.75f, 0);
    WebSceneFx_BeginView(8, 0, 1264, 720, 1280, 720, 0.1f, 4.0f);
    WebSceneFx_EndView();
    WebSceneFx_EndFrame();
    begin(10, 0.34f, 161.5f); quad(&keyA, 20, 3, 0); present();
    assert(near(composed[SCENE_FX_VM_MOTION], 9.46f));
    /* A viewmodel standing still composes nothing. */
    before = composes;
    begin(10, 0.34f, 161.5f); quad(&keyA, 20, 3, 0); present();
    assert(composes == before && s_vmMotion == 0);
    /* Motion off the screen draws nothing. */
    begin(10, 0.34f, 161.5f); quad(&keyA, 20, 40, 0); present();
    begin(10, 0.34f, 161.5f); quad(&keyA, 20, 40.5f, 0); present();
    assert(composes == before && s_vmMotion == 0 && smearOf(0, 0) != 0);
    /* Triangles reaching behind the eye are left out. */
    begin(10, 0.34f, 161.5f); quad(&keyA, 20, 0, 0); quad(&keyB, -10, 0, 0); present();
    begin(10, 0.34f, 161.5f); quad(&keyA, 20, 0.5f, 0); quad(&keyB, -10, 0.5f, 0); present();
    assert(composes == before + 1 && composed[SCENE_FX_VM_TRIANGLES] == 2);

    /* Vertices may be spread out: a DX7 skinned vertex is 36 bytes. */
    {
        float spread[4][9] = {{0}}, transform[12];
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 9; ++j)
                spread[i][j] = j < 3 ? corners[i][j] : 1000.0f + j;
        placement(transform, 20, 0.5f, 0);
        begin(10, 0.34f, 161.5f);
        WebSceneFx_AddViewmodelSurface(&keyA, &spread[0][0], (int)sizeof(spread[0]), 4, quadTriangles, 2, transform);
        quad(&keyB, 20, 0.5f, 0);
        present();
        for (int i = 0; i < 12; ++i)
            assert(near(s_vmWorld[i], s_vmWorld[12 + i]));
    }
    /* A viewmodel too big for the mesh texture does not blur, and says so;
     * the next frame starts afresh. */
    {
        float transform[12];
        placement(transform, 20, 0, 0);
        begin(10, 0.34f, 161.5f); quad(&keyA, 20, 1, 0); present();
        begin(10, 0.34f, 161.5f); quad(&keyA, 20, 1.5f, 0);
        WebSceneFx_AddViewmodelSurface(&keyB, bigPositions, 12, 20000, quadTriangles, 1, transform);
        present();
        assert(s_vmMotion == 0 && s_vmOverflow);
        begin(10, 0.34f, 161.5f); quad(&keyA, 20, 2, 0); present();
        begin(10, 0.34f, 161.5f); quad(&keyA, 20, 2.5f, 0); present();
        assert(!s_vmOverflow && near(s_vmMotion, 26.12f));
    }
    /* A raised WWII scope draws the rifle with its own 1.7 x 3 projection and
     * 0.8 near clip: the mesh and the smear follow where it is drawn. */
    {
        const float scope[SCENE_FX_SCOPE_LENS_FLOATS] = {0, 0, 0.5f, 0, 0, 0.9f, 1, 1, 0, 0, 1.7f, 3.0f, 0.8f};
        WebSceneFx_SetScope(scope);
        begin(10, 0.34f, 161.5f); quad(&keyA, 20, 0, 0); present();
        begin(10, 0.34f, 161.5f); quad(&keyA, 20, 0.5f, 0); present();
        for (int i = 0; i < 4; ++i) {
            float z = 20 + corners[i][0], x = 0.5f - corners[i][1], y = corners[i][2];
            assert(near(s_vmMesh[i * 8], 1.7f * x) && near(s_vmMesh[i * 8 + 1], 3.0f * y));
            assert(near(s_vmMesh[i * 8 + 2], 2 * (0.9995f * z - 0.8f) - z) && near(s_vmMesh[i * 8 + 3], z));
            assert(near(smearOf(i, 0), boosted(smearX(ndcX(i, 20, 0), ndcX(i, 20, 0.5f), 10) * 1.7f / 1.19f)) &&
                   still(smearOf(i, 1)));
        }
        WebSceneFx_SetScope(NULL);
        begin(10, 0.34f, 161.5f); quad(&keyA, 20, 0.5f, 0); present();
        assert(near(s_vmMesh[0], 1.19f * (0.5f - corners[0][1])));
    }
    puts("PASS: viewmodel vertices smear by their own screen motion over a 10 ms exposure at any frame rate, the bob's and sway's as they are and faster motion 3.5 times past a knee at 0.4% of the height, capped at 10%, the camera's motion cancelled; clip positions use the depth hack's near clip, or a raised WWII scope's projection and near clip; triangles, indices after the vertices and tiles holding the largest smear within a tile; cuts, swapped poses, new or changed surfaces, repeated surfaces, r_motionBlur 0, fullbright and partial views, still and off-screen motion, triangles behind the eye, spread vertices and an oversized viewmodel");
    return 0;
}
'''

# Each mutant must fail the checks above.
mutants = [
    ('    lens[14] = -nearClip;\n', ''),
    ('    memcpy(s_cameraLens, projectionMatrix, sizeof(s_cameraLens));\n', ''),
    ('(float)(SCENE_FX_VM_EXPOSURE / s_viewGap) * 0.25f', '(float)(SCENE_FX_VM_EXPOSURE / s_viewGap) * 0.5f'),
    ('(float)(SCENE_FX_VM_EXPOSURE / s_viewGap)', '(float)(SCENE_FX_VM_EXPOSURE / 10.0)'),
    # The camera's longer exposure, no knee, a knee that counts nothing more,
    # or no cap past it.
    ('(float)(SCENE_FX_VM_EXPOSURE / s_viewGap)', '(float)(SCENE_FX_MOTION_EXPOSURE / s_viewGap)'),
    ('    knee = SCENE_FX_VM_KNEE * (float)height;', '    knee = 1.0f * (float)height;'),
    ('SCENE_FX_VM_GAIN * (length - knee)', '(length - knee)'),
    ('fminf(knee + SCENE_FX_VM_GAIN * (length - knee), cap)', 'knee + SCENE_FX_VM_GAIN * (length - knee)'),
    ('    cap = SCENE_FX_VM_CAP * (float)height;', '    cap = 1.0f * (float)height;'),
    ('smear[0] = (screen[i * 2] - was[0]) * (float)width * scale;',
     'smear[0] = (screen[i * 2] - was[0]) * (float)height * scale;'),
    ('vertex[2] = 2.0f * clip[2] - clip[3];', 'vertex[2] = clip[2];'),
    ('        occurrence += s_vmSurfaces[i].key == surface->key;', '        occurrence += 0;'),
    ('return s_vmLastSurfaces[i].count == surface->count ? &s_vmLastSurfaces[i] : NULL;',
     'return &s_vmLastSurfaces[i];'),
    ('if (jump > SCENE_FX_VM_MAX_JUMP * (float)surface->count)', 'if (0)'),
    ('s_viewContinuous && WebSceneFx_Enabled(r_motionBlur);', 'WebSceneFx_Enabled(r_motionBlur);'),
    ('s_viewContinuous && WebSceneFx_Enabled(r_motionBlur);', 's_viewContinuous;'),
    ('s_vmMotion <= 0.0f && !WebSceneFx_GlowEnabled()', '!WebSceneFx_GlowEnabled()'),
    ('for (y = j > 0 ? j - 1 : 0; y <= j + 1 && y < tilesY; ++y)', 'for (y = j; y <= j && y < tilesY; ++y)'),
    ('for (x = i > 0 ? i - 1 : 0; x <= i + 1 && x < tilesX; ++x)', 'for (x = i; x <= i && x < tilesX; ++x)'),
    ('corners[c] = surface->first + local;', 'corners[c] = local;'),
    ('    s_vmSurfaceCount = 0;\n    s_vmVertexCount = 0;\n', '    s_vmVertexCount = 0;\n'),
    ('if (clip[3] > 0.001f && isfinite(clip[0])', 'if (isfinite(clip[0])'),
    ('        return 0;\n    tx0 = ', '        return 1;\n    tx0 = '),
    ('    s_vmLastScreen ^= 1;\n', ''),
    ('p[0] * transform[j] + p[1] * transform[3 + j] + p[2] * transform[6 + j]',
     'p[0] * transform[j * 3] + p[1] * transform[j * 3 + 1] + p[2] * transform[j * 3 + 2]'),
    ('(const char *)positions + (size_t)i * (size_t)stride', '(const char *)positions + (size_t)i * 12'),
    ('    s_vmOverflow = 0;\n}', '}'),
    ('tile = (int)ceilf(cap);', 'tile = (int)ceilf(cap) + 8;'),
    ('        lens[0] = s_scope[SCENE_FX_SCOPE_PROJECTION - SCENE_FX_SCOPE];\n', ''),
    ('        lens[5] = s_scope[SCENE_FX_SCOPE_PROJECTION + 1 - SCENE_FX_SCOPE];\n', ''),
    ('        nearClip = WebSceneFx_ScopeNearClip(nearClip);\n', ''),
]

# r_model.c's capture hooks, compiled against the engine's own structures.
def struct(name, kind='struct'):
    return re.search(r'^' + kind + r' ' + name + r' \{.*?^\};', headers, re.S | re.M).group(0)


start = model.index('/* A skinned surface\'s vertices: its slice of the locked skinned cache')
verts = model[start:model.index('\nstatic void R_SkinXSurfaceSkinned(', start)]
start = model.index('#ifdef __EMSCRIPTEN__\nextern void WebSceneFx_AddViewmodelSurface(')
hooks = model[start + len('#ifdef __EMSCRIPTEN__\n'):model.index('#endif\n\nvoid R_SkinXModelCmd(', start)]
capture_support = '\n'.join([
    '#include <assert.h>', '#include <stddef.h>', '#include <stdio.h>', '#include <string.h>',
    'typedef float vec_t; typedef vec_t vec2_t[2]; typedef vec_t vec3_t[3]; typedef unsigned char byte;',
    'typedef int refEntityType_t; typedef int surfaceType_t; typedef struct Material *MaterialHandle;',
    'typedef struct FxMemMgr_Emitter FxMemMgr_Emitter; typedef struct IDirect3DVertexBuffer9 IDirect3DVertexBuffer9;',
    'typedef struct IDirect3DIndexBuffer9 IDirect3DIndexBuffer9; typedef struct GfxVertex GfxVertex;',
    'typedef struct XBlendInfo_s XBlendInfo; typedef struct XVertexInfo_s XVertexInfo;',
    'typedef struct XVertexBuffer XVertexBuffer; typedef struct XSurface_s XSurface;',
    'typedef union GfxColor GfxColor; typedef struct GfxVertexDx7 GfxVertexDx7;',
    'typedef struct GfxModelSurface GfxModelSurface; typedef struct GfxModelRigidSurface GfxModelRigidSurface;',
    'typedef struct GfxEntity GfxEntity;',
    struct('XBlendInfo_s'), struct('XVertexInfo_s'), struct('XVertexBuffer'), struct('XRigidSurface'),
    struct('XSurface_s'), struct('GfxColor', 'union'), struct('GfxVertexDx7'), struct('GfxModelSurface'),
    struct('GfxModelSkinnedSurface'), struct('GfxModelRigidSurface'), struct('GfxEntity'),
    'typedef struct { struct { int integer; } current; } dvar_t;',
    'static dvar_t renderer = {{2}}; static dvar_t *r_rendererInUse = &renderer;',
    'static struct { char *skinnedCacheLockAddr; } dx;',
    'static const void *gotKey; static const float *gotPositions, *gotTransform; static int gotStride, gotCount, gotTriangles, calls;',
    'static const unsigned short *gotIndices; static float gotMatrix[12];',
    'void WebSceneFx_AddViewmodelSurface(const void *key, const float *positions, int stride, int vertexCount,',
    '    const unsigned short *indices, int triangleCount, const float *transform)',
    '{',
    '    ++calls; gotKey = key; gotPositions = positions; gotStride = stride; gotCount = vertexCount;',
    '    gotIndices = indices; gotTriangles = triangleCount; gotTransform = transform;',
    '    memcpy(gotMatrix, transform, sizeof(gotMatrix));',
    '}',
    'static const GfxEntity *castEnt; static const float *castPositions, *castTransform; static int castStride, castCount, casts;',
    'static const unsigned short *castIndices; static int castTriangles; static float castMatrix[12];',
    'void WebSunShadows_AddSurface(const GfxEntity *ent, const float *positions, int stride, int vertexCount,',
    '    const unsigned short *indices, int triangleCount, const float *transform)',
    '{',
    '    ++casts; castEnt = ent; castPositions = positions; castStride = stride; castCount = vertexCount;',
    '    castIndices = indices; castTriangles = triangleCount; castTransform = transform;',
    '    memcpy(castMatrix, transform, sizeof(castMatrix));',
    '}',
])
capture_test = capture_support + '\n' + verts + '\n' + 'HOOKS' + r'''
int main(void)
{
    static XVertexInfo info[5];
    static float cache[64], own[64];
    static unsigned short triangles[9] = {0, 1, 2, 1, 2, 3, 2, 3, 4};
    XSurface xsurf = {0};
    struct GfxModelSkinnedSurface skinned = {0};
    GfxModelRigidSurface rigid = {0};
    GfxEntity gun = {0};

    xsurf.vertCount = 5; xsurf.triCount = 3;
    xsurf.triIndices = (int (*)[4])triangles;
    xsurf.verts = (XVertexBuffer *)info;
    gun.renderFxFlags = 0x8c;
    gun.scale = 2;
    for (int i = 0; i < 3; ++i) {
        gun.origin[i] = 100.0f * (i + 1);
        for (int j = 0; j < 3; ++j)
            gun.axis[i][j] = i == j ? 1.0f : 0.25f * (j - i);
    }
    /* A skinned viewmodel surface in the cache: model-space vertices that the
     * entity's scaled axes and origin place, 36 bytes apart under DX7. */
    skinned.surf.xsurf = &xsurf;
    skinned.skinnedCachedOffset = 72;
    dx.skinnedCacheLockAddr = (char *)cache;
    R_AddWebSkinnedSurface(&gun, &skinned);
    assert(calls == 1 && gotKey == &xsurf && gotPositions == (const float *)((char *)cache + 72) && gotStride == 36);
    assert(!casts);
    assert(gotCount == 5 && gotTriangles == 3 && gotIndices == triangles);
    for (int i = 0; i < 3; ++i) {
        assert(gotMatrix[9 + i] == gun.origin[i]);
        for (int j = 0; j < 3; ++j)
            assert(gotMatrix[i * 3 + j] == gun.axis[i][j] * 2);
    }
    /* Its own buffer when the cache was full, 64 bytes apart under DX9. */
    skinned.skinnedCachedOffset = -1;
    skinned.skinnedVert.variant = own;
    renderer.current.integer = 1;
    R_AddWebSkinnedSurface(&gun, &skinned);
    assert(calls == 2 && gotPositions == own && gotStride == 64);
    /* A rigid surface reads its bone-space offsets, a vertex info apart, with
     * the transform that already holds its bone and the entity. */
    rigid.surf.xsurf = &xsurf;
    R_AddWebRigidSurface(&gun, &rigid);
    assert(calls == 3 && gotKey == &xsurf && gotPositions == info[0].offset && gotStride == (int)sizeof(XVertexInfo));
    assert(gotTransform == &rigid.boneAxis[0][0] && gotCount == 5 && gotTriangles == 3);
    assert(!casts);
    /* Entities without the depth hack are not the viewmodel: the sun's shadow
     * map takes their surfaces, from the same buffers with the same placement. */
    gun.renderFxFlags = 0x84;
    R_AddWebSkinnedSurface(&gun, &skinned);
    assert(casts == 1 && castEnt == &gun && castPositions == own && castStride == 64 && castCount == 5);
    assert(castIndices == triangles && castTriangles == 3);
    for (int i = 0; i < 3; ++i) {
        assert(castMatrix[9 + i] == gun.origin[i]);
        for (int j = 0; j < 3; ++j)
            assert(castMatrix[i * 3 + j] == gun.axis[i][j] * 2);
    }
    R_AddWebRigidSurface(&gun, &rigid);
    assert(casts == 2 && castPositions == info[0].offset && castStride == (int)sizeof(XVertexInfo));
    assert(castTransform == &rigid.boneAxis[0][0]);
    gun.renderFxFlags = 0x10;
    R_AddWebSkinnedSurface(&gun, &skinned);
    R_AddWebRigidSurface(&gun, &rigid);
    gun.renderFxFlags = 0x18;
    R_AddWebSkinnedSurface(&gun, &skinned);
    assert(calls == 3 && casts == 5);
    puts("PASS: r_model.c hands the compositor the depth-hacked viewmodel's skinned surfaces from the cache or their own buffer, with the entity's scaled placement and the renderer's vertex stride, and its rigid surfaces' bone-space offsets with their own transform; every other entity's go to the sun's shadow map the same way");
    return 0;
}
'''
capture_mutants = [
    ('    return (ent->renderFxFlags & 0x18) == 8;', '    return (ent->renderFxFlags & 0x8) != 0;'),
    ('transform[i * 3 + j] = ent->axis[i][j] * ent->scale;', 'transform[i * 3 + j] = ent->axis[i][j];'),
    ('transform[i * 3 + j] = ent->axis[i][j] * ent->scale;', 'transform[j * 3 + i] = ent->axis[i][j] * ent->scale;'),
    ('r_rendererInUse->current.integer == 2 ? 0x24 : 0x40', '0x24'),
    ('    return (char *)dx.skinnedCacheLockAddr + skinnedSurf->skinnedCachedOffset;',
     '    return (char *)dx.skinnedCacheLockAddr;'),
    ('((const XVertexInfo *)xsurf->verts)->offset', '((const XVertexInfo *)xsurf->verts)->normal'),
    ('    if (R_IsWebViewmodel(ent))\n        WebSceneFx_AddViewmodelSurface(',
     '    if (1)\n        WebSceneFx_AddViewmodelSurface('),
    ('    else\n        WebSunShadows_AddSurface(ent,', '    else if (0)\n        WebSunShadows_AddSurface(ent,'),
    ('    R_AddWebSurfaceVerts(ent, xsurf, ((const XVertexInfo *)xsurf->verts)->offset',
     '    R_AddWebSurfaceVerts(&(GfxEntity){.renderFxFlags = 8}, xsurf, ((const XVertexInfo *)xsurf->verts)->offset'),
]


def build(path, name, text, sanitize):
    c = path / f'{name}.c'
    c.write_text(text)
    flags = ['-O1', '-g', '-Wall', '-Werror', '-fsanitize=address,undefined', '-fno-sanitize-recover=all'] \
        if sanitize else ['-O0', '-w']
    subprocess.run(['cc', '-std=c99', *flags, '-I', str(path), str(c), '-lm', '-o', str(path / name)], check=True)
    return subprocess.run([str(path / name)], capture_output=not sanitize)


with tempfile.TemporaryDirectory(prefix='cod2-viewmodel-motion-') as tmp:
    path = Path(tmp)
    (path / 'emscripten.h').write_text(emscripten)
    (path / 'common_types.h').write_text(common_types)
    assert build(path, 'motion', test.replace('SOURCE', str(source)), True).returncode == 0
    for index, (before, after) in enumerate(mutants):
        assert compositor.count(before) == 1, before
        mutant = path / f'mutant{index}.c'
        mutant.write_text(compositor.replace(before, after))
        result = build(path, f'motion-mutant{index}', test.replace('SOURCE', str(mutant)), False)
        assert result.returncode != 0, f'mutant survived: {after!r}'
    print(f'PASS: {len(mutants)} viewmodel motion mutants fail')
    assert build(path, 'capture', capture_test.replace('HOOKS', hooks), True).returncode == 0
    for index, (before, after) in enumerate(capture_mutants):
        assert hooks.count(before) + verts.count(before) == 1, before
        text = capture_test.replace('HOOKS', hooks.replace(before, after)).replace(verts, verts.replace(before, after))
        result = build(path, f'capture-mutant{index}', text, False)
        assert result.returncode != 0, f'capture mutant survived: {after!r}'
    print(f'PASS: {len(capture_mutants)} viewmodel capture mutants fail')
