#include "common_types.h"
#include <emscripten.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

/* The first full-screen 3D view of a frame renders into an offscreen target
 * (multisampled when r_aaSamples > 1). Before the first 2D draw it is composed
 * into the canvas: CoD2's DX9 glow (r_glow*), WWII-style motion blur of the
 * camera and of the viewmodel's own movement (r_motionBlur), a CoD4-style
 * depth of field on the viewmodel and the near world while aiming, contrast
 * adaptive sharpening and a light film grade.
 * A raised WWII scope (cg_wwiiscope_mp.c) turns its glass into a window: the
 * rifle is drawn with its own projection and near clip and kept off the glass
 * by the stencil, so the magnified world shows through, and the world around
 * the eyepiece blurs, or blacks out while the breath is held. A round ring of
 * worn metal is drawn about the glass in place of the rifle's own eyepiece,
 * whose bell the stencil keeps the rifle off as well.
 * The HUD and any later view draw straight into the canvas. A canvas taller
 * than r_sceneHeight lines renders that 3D view at r_sceneHeight and scales
 * it up as it composes, so the HUD gets the canvas's resolution for the
 * scene's cost. */

extern const dvar_t *r_glow;
extern const dvar_t *r_glowRadius[2];
extern const dvar_t *r_glowBloomIntensity[2];
extern const dvar_t *r_glowBloomCutoff;
extern const dvar_t *r_glowBloomDesaturation;
extern const dvar_t *r_fullbright;
extern const dvar_t *r_aaSamples;
extern const dvar_t *r_dof_enable;
extern const dvar_t *r_filmTweakEnable;
extern const dvar_t *r_motionBlur;
extern const dvar_t *r_sceneHeight;
extern const dvar_t *r_ssao;
extern const dvar_t *r_atmosphere;

/* Draws the sun's shadow map of the frame's moving models (web_sun_shadows.c). */
void WebSunShadows_Render(void);

enum {
    SCENE_FX_GLOW0,
    SCENE_FX_GLOW1,
    SCENE_FX_CUTOFF,
    SCENE_FX_DESATURATION,
    SCENE_FX_RADIUS0,
    SCENE_FX_RADIUS1,
    SCENE_FX_ADS,
    SCENE_FX_VM_START,
    SCENE_FX_VM_END,
    SCENE_FX_NEAR_CLIP,
    SCENE_FX_NEAR_BLUR,
    SCENE_FX_SHARPEN,
    SCENE_FX_SATURATION,
    SCENE_FX_CONTRAST,
    SCENE_FX_FILM,
    SCENE_FX_WORLD_NEAR_CLIP,
    SCENE_FX_NEAR_END,
    SCENE_FX_WORLD_BLUR,
    /* The camera's motion over the exposure as a multiple of its motion since
     * the last frame; 0 draws no motion blur. */
    SCENE_FX_MOTION,
    /* Column-major rotation, then offset, taking a point from this view's
     * space into the last frame's. */
    SCENE_FX_MOTION_ROTATION,
    SCENE_FX_MOTION_MOVE = SCENE_FX_MOTION_ROTATION + 9,
    /* The projection's x and y scales. */
    SCENE_FX_MOTION_PROJECTION = SCENE_FX_MOTION_MOVE + 3,
    /* How much of the whole view blurs behind a screen-filling menu. */
    SCENE_FX_BACKDROP = SCENE_FX_MOTION_PROJECTION + 2,
    /* How much of its color the view behind such a menu loses. */
    SCENE_FX_BACKDROP_GRAY,
    /* The longest half smear of the viewmodel's own motion in scene pixels;
     * 0 draws none. */
    SCENE_FX_VM_MOTION,
    /* The viewmodel's triangles, the mesh texture rows where its indices and
     * tiles start, and the rows to upload. */
    SCENE_FX_VM_TRIANGLES,
    SCENE_FX_VM_INDEX_ROW,
    SCENE_FX_VM_TILE_ROW,
    SCENE_FX_VM_ROWS,
    /* The tile size in scene pixels, and the tiles across. */
    SCENE_FX_VM_TILE,
    SCENE_FX_VM_TILES_X,
    /* A scope's glass as a window onto the world (CgWwiiScopeLens): its
     * centre and half-axes in normalized device coordinates, how present and
     * how open the window is, how much of the world around it blurs and
     * darkens, and the rifle's own projection scales and near clip. */
    SCENE_FX_SCOPE,
    SCENE_FX_SCOPE_AXES = SCENE_FX_SCOPE + 2,
    SCENE_FX_SCOPE_PRESENCE = SCENE_FX_SCOPE_AXES + 4,
    SCENE_FX_SCOPE_OPEN,
    SCENE_FX_SCOPE_BLUR,
    SCENE_FX_SCOPE_DARK,
    SCENE_FX_SCOPE_PROJECTION,
    SCENE_FX_SCOPE_NEAR_CLIP = SCENE_FX_SCOPE_PROJECTION + 2,
    SCENE_FX_SCOPE_END,
    /* Ambient occlusion from the scene's depth (r_ssao): how much a fully
     * enclosed point darkens, 0 for none; the radius it looks around in
     * world units; and the view's projection x and y scales. */
    SCENE_FX_AO = SCENE_FX_SCOPE_END,
    SCENE_FX_AO_RADIUS,
    SCENE_FX_AO_PROJECTION,
    /* The map's haze (r_atmosphere): its color and its added density per
     * unit, 0 for none; then in this view's space the world's up and the
     * direction toward the sun, the sun's glow in it, and how fast it thins
     * with height, per unit. */
    SCENE_FX_HAZE = SCENE_FX_AO_PROJECTION + 2,
    SCENE_FX_HAZE_DENSITY = SCENE_FX_HAZE + 3,
    SCENE_FX_HAZE_UP,
    SCENE_FX_HAZE_SUN = SCENE_FX_HAZE_UP + 3,
    SCENE_FX_HAZE_SUN_COLOR = SCENE_FX_HAZE_SUN + 3,
    SCENE_FX_HAZE_FALLOFF = SCENE_FX_HAZE_SUN_COLOR + 3,
    SCENE_FX_PARAM_COUNT
};

#define SCENE_FX_SCOPE_FLOATS (SCENE_FX_SCOPE_END - SCENE_FX_SCOPE)

/* r_ssao's darkening of an enclosed point, and the radius it measures. */
#define SCENE_FX_AO_STRENGTH 0.85f
#define SCENE_FX_AO_WORLD_RADIUS 22.0f

/* r_atmosphere adds this share of the map's own fog density, at most the
 * thinnest map's whole fog (Dawnville's own is the thickest), thickest at
 * the eye's height and below and thinning above it over the scale height
 * in units; toward the sun the haze takes up to this share of its light. */
#define SCENE_FX_HAZE_SHARE 0.6f
#define SCENE_FX_HAZE_MAX_DENSITY 0.0001f
#define SCENE_FX_HAZE_SCALE_HEIGHT 640.0f
#define SCENE_FX_HAZE_SUN_GLOW 0.45f

/* InfinitePerspectiveMatrix's depth scale times the default r_znear of 4. */
#define SCENE_FX_DEFAULT_WORLD_NEAR_CLIP (0.9995002746582031f * 4.0f)

/* Milliseconds of camera motion the blur spans, at any frame rate. */
#define SCENE_FX_MOTION_EXPOSURE 16.0
/* The viewmodel's own motion smears over 10 ms. A half smear up to the knee,
 * a fraction of the screen's height, is the bob and sway and stays as it is;
 * a faster motion (a switch, a reload, the sprint pose) counts each pixel past
 * the knee SCENE_FX_VM_GAIN times, up to the cap. */
#define SCENE_FX_VM_EXPOSURE 10.0
#define SCENE_FX_VM_KNEE 0.004f
#define SCENE_FX_VM_GAIN 3.5f
#define SCENE_FX_VM_CAP 0.10f
/* Two views further apart in time or space than these are a cut: a spawn,
 * a teleport or a new killcam, not motion. A fast spin at 60 fps turns
 * about half a radian a frame. */
#define SCENE_FX_MOTION_MAX_GAP 100.0
#define SCENE_FX_MOTION_MAX_TURN 0.6f
#define SCENE_FX_MOTION_MAX_MOVE 48.0f
/* Below these, a turn in radians or a move in units, the camera stands still. */
#define SCENE_FX_MOTION_MIN_TURN 0.0002f
#define SCENE_FX_MOTION_MIN_MOVE 0.05f

/* The viewmodel surfaces skinned in one frame. */
#define SCENE_FX_VM_MAX_SURFACES 64
#define SCENE_FX_VM_MAX_VERTICES 16384
#define SCENE_FX_VM_MAX_TRIANGLES 32768
#define SCENE_FX_VM_MAX_TILES 8192
/* The mesh texture the viewmodel's motion is drawn from: rows of 1024 RGBA
 * float texels. A vertex takes two (its clip position, then its smear), an
 * index a quarter and a tile one. These rows hold the most of each. */
#define SCENE_FX_VM_MESH_WIDTH 1024
#define SCENE_FX_VM_MESH_ROWS 64
/* A surface whose vertices moved further on average than this, in normalized
 * device coordinates, between two frames was swapped for another pose. */
#define SCENE_FX_VM_MAX_JUMP 1.0f

static float s_adsFrac;
static float s_vmStart = 6.0f;
static float s_vmEnd = 15.0f;
static float s_nearEnd = 512.0f;
static float s_nearClip = 0.1f;
static float s_worldNearClip = SCENE_FX_DEFAULT_WORLD_NEAR_CLIP;
static int s_active;
static int s_frameUsed;
static float s_aimFrac;
static float s_cameraView[16];
static float s_cameraProjection[2];
/* The composed view's projection x and y scales, for its ambient occlusion. */
static float s_viewProjection[2];
/* The composed view's color-map mip bias while it renders below the canvas. */
static float s_lodBias;
static float s_cameraLens[16];
static int s_cameraPending;
static float s_lastView[16];
static double s_lastViewTime;
static int s_lastViewValid;
/* Whether this view follows the last one without a cut, and how long after. */
static int s_viewContinuous;
static double s_viewGap;
static float s_motionScale;
static float s_motionRotation[9];
static float s_motionMove[3];
static float s_motionProjection[2];
static float s_backdropFrac;
static float s_backdropGray;
static float s_grayFrac;
/* The scene's fog and sun for its haze, and its up and sun in the composed
 * view's space. */
static float s_hazeColor[3];
static float s_hazeDensity;
static float s_hazeSunWorld[3];
static float s_hazeSunColor[3];
static float s_hazeUp[3];
static float s_hazeSun[3];

typedef struct {
    const void *key;
    const unsigned short *indices;
    int first;
    int count;
    int triangles;
} WebSceneFxVmSurface;

/* This frame's viewmodel surfaces, their vertices in the world, and the
 * screen positions of the last frame's, each taken from the frame's view. */
static WebSceneFxVmSurface s_vmSurfaces[SCENE_FX_VM_MAX_SURFACES];
static int s_vmSurfaceCount;
static int s_vmVertexCount;
static int s_vmTriangleCount;
static int s_vmOverflow;
static int s_vmOverflowWarned;
static float s_vmWorld[SCENE_FX_VM_MAX_VERTICES * 3];
static WebSceneFxVmSurface s_vmLastSurfaces[SCENE_FX_VM_MAX_SURFACES];
static int s_vmLastSurfaceCount;
static float s_vmScreens[2][SCENE_FX_VM_MAX_VERTICES * 2];
static int s_vmLastScreen;
static float s_vmTiles[SCENE_FX_VM_MAX_TILES * 2];
static float s_vmMesh[SCENE_FX_VM_MESH_ROWS * SCENE_FX_VM_MESH_WIDTH * 4];
static float s_vmMotion;
static int s_vmTriangles;
static int s_vmIndexRow;
static int s_vmTileRow;
static int s_vmRows;
static int s_vmTile;
static int s_vmTilesX;
/* After the scope's floats in a CgWwiiScopeLens, the outline the rifle is
 * kept off about a scope's glass, centre and half-axes, which only the
 * stencil needs. */
#define SCENE_FX_SCOPE_BELL_FLOATS 6
#define SCENE_FX_SCOPE_LENS_FLOATS (SCENE_FX_SCOPE_FLOATS + SCENE_FX_SCOPE_BELL_FLOATS)
static float s_scope[SCENE_FX_SCOPE_FLOATS];
static float s_scopeBell[SCENE_FX_SCOPE_BELL_FLOATS];
/* The window's stencil is drawn once per view, before the rifle's first draw. */
static int s_scopeStencil;
static int s_scopeMasking;
/* The material drawing now stays over a sight's window. */
static int s_scopeExempt;
/* Set when the last full view could not compose, so the cgame keeps the
 * stock overlay: the window needs the scene's own depth and stencil. */
static int s_scopeUnavailable;

EM_JS(int, WebSceneFx_Bind, (int width, int height, int samples, int screenWidth, int screenHeight), {
    if (typeof GLctx === "undefined" || !GLctx || !GLctx.blitFramebuffer) return 0;
    var gl = GLctx;
    var fx = Module.cod2SceneFx;
    if (!fx || fx.gl !== gl) {
        fx = Module.cod2SceneFx = {
            gl: gl, failed: false, width: 0, height: 0, requested: -1, samples: 0,
            t: null, programs: null, vao: null, drawing: false,
            maxSamples: gl.getParameter(gl.MAX_SAMPLES) | 0,
            discard: [gl.COLOR_ATTACHMENT0, gl.DEPTH_STENCIL_ATTACHMENT],
            still: new Float32Array([128 / 255, 128 / 255, 0, 0]), far: new Float32Array([1])
        };
        fx.build = function() {
            var header = "#version 300 es\nprecision mediump float;\nin highp vec2 v_uv;\nout vec4 o;\n";
            var vs = gl.createShader(gl.VERTEX_SHADER);
            gl.shaderSource(vs, "#version 300 es\nout highp vec2 v_uv;\nvoid main() {\n" +
                "  vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);\n" +
                "  v_uv = p;\n  gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);\n}\n");
            gl.compileShader(vs);
            var previous = gl.getParameter(gl.CURRENT_PROGRAM);
            var programs = {}, ok = true;
            var link = function(name, body, samplers, uniforms, vertex) {
                var fs = gl.createShader(gl.FRAGMENT_SHADER);
                gl.shaderSource(fs, header + body);
                gl.compileShader(fs);
                var program = gl.createProgram();
                vertex = vertex || vs;
                gl.attachShader(program, vertex);
                gl.attachShader(program, fs);
                gl.linkProgram(program);
                if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
                    console.warn("[scene-fx] " + name + " failed: " + gl.getProgramInfoLog(program) +
                        " " + gl.getShaderInfoLog(vertex) + " " + gl.getShaderInfoLog(fs));
                    ok = false;
                    return;
                }
                gl.useProgram(program);
                var entry = { program: program };
                for (var s = 0; s < samplers.length; ++s)
                    gl.uniform1i(gl.getUniformLocation(program, samplers[s]), 8 + s);
                for (var u = 0; u < uniforms.length; ++u)
                    entry[uniforms[u]] = gl.getUniformLocation(program, uniforms[u]);
                programs[name] = entry;
            };
            var quarter = "  vec3 c = texture(u_src, v_uv + u_texel * vec2(-1.0, -1.0)).rgb;\n" +
                "  vec3 d = texture(u_src, v_uv + u_texel * vec2(1.0, -1.0)).rgb;\n" +
                "  vec3 e = texture(u_src, v_uv + u_texel * vec2(-1.0, 1.0)).rgb;\n" +
                "  vec3 f = texture(u_src, v_uv + u_texel * vec2(1.0, 1.0)).rgb;\n";
            /* Every pass costs a fixed GPU setup whatever its size, so the glow
             * and the aim blur each use one 2D pass instead of separable ones. */
            link("bright", "uniform sampler2D u_src;\nuniform highp vec2 u_texel;\nuniform vec3 u_bright;\n" +
                "vec3 bright(vec2 p) {\n" +
                "  vec3 c = texture(u_src, v_uv + u_texel * p).rgb;\n" +
                "  float w = clamp((max(c.r, max(c.g, c.b)) - u_bright.x) * u_bright.y, 0.0, 1.0);\n" +
                "  return c * (w * w);\n}\n" +
                "void main() {\n" +
                "  vec3 b = vec3(0.0);\n" +
                "  for (int y = -3; y <= 3; y += 2)\n" +
                "    for (int x = -3; x <= 3; x += 2) b += bright(vec2(x, y));\n" +
                "  b *= 0.0625;\n" +
                "  o = vec4(mix(b, vec3(dot(b, vec3(0.299, 0.587, 0.114))), u_bright.z), 1.0);\n}\n",
                ["u_src"], ["u_texel", "u_bright"]);
            link("bloom", "uniform sampler2D u_src;\nuniform highp vec2 u_texel;\nuniform vec3 u_narrow;\n" +
                "uniform vec3 u_wideOffset;\nuniform vec4 u_wideWeight;\nuniform vec2 u_mix;\n" +
                "void main() {\n" +
                "  float no[3] = float[3](-u_narrow.x, 0.0, u_narrow.x);\n" +
                "  float nw[3] = float[3](u_narrow.z, u_narrow.y, u_narrow.z);\n" +
                "  float wo[7] = float[7](-u_wideOffset.z, -u_wideOffset.y, -u_wideOffset.x, 0.0,\n" +
                "    u_wideOffset.x, u_wideOffset.y, u_wideOffset.z);\n" +
                "  float ww[7] = float[7](u_wideWeight.w, u_wideWeight.z, u_wideWeight.y, u_wideWeight.x,\n" +
                "    u_wideWeight.y, u_wideWeight.z, u_wideWeight.w);\n" +
                "  vec3 narrow = vec3(0.0), wide = vec3(0.0);\n" +
                "  for (int y = 0; y < 3; ++y)\n" +
                "    for (int x = 0; x < 3; ++x)\n" +
                "      narrow += texture(u_src, v_uv + vec2(no[x], no[y]) * u_texel).rgb * (nw[x] * nw[y]);\n" +
                "  for (int y = 0; y < 7; ++y)\n" +
                "    for (int x = 0; x < 7; ++x)\n" +
                "      wide += texture(u_src, v_uv + vec2(wo[x], wo[y]) * u_texel).rgb * (ww[x] * ww[y]);\n" +
                "  o = vec4(narrow * u_mix.x + wide * u_mix.y, 1.0);\n}\n",
                ["u_src"], ["u_texel", "u_narrow", "u_wideOffset", "u_wideWeight", "u_mix"]);
            link("dofBlur", "uniform sampler2D u_src;\nuniform highp vec2 u_step;\n" +
                "void main() {\n" +
                "  float k[5] = float[5](0.03515625, 0.328125, 0.2734375, 0.328125, 0.03515625);\n" +
                "  float d[5] = float[5](-3.1111111, -1.3333333, 0.0, 1.3333333, 3.1111111);\n" +
                "  vec4 sum = vec4(0.0);\n" +
                "  for (int y = 0; y < 5; ++y)\n" +
                "    for (int x = 0; x < 5; ++x)\n" +
                "      sum += texture(u_src, v_uv + vec2(d[x], d[y]) * u_step) * (k[x] * k[y]);\n" +
                "  o = sum;\n}\n",
                ["u_src"], ["u_step"]);
            /* Blur radius as a fraction of the largest. CoD4's r_dof_bias of 0.5
             * squares the viewmodel's circle of confusion, so the hand ahead of
             * the receiver stays crisper than the receiver. The viewmodel draws
             * into depth [0, 0.2] with the depth hack near clip. */
            var coc = "float vmBlur(highp float d) {\n" +
                "  if (d >= 0.2) return 0.0;\n" +
                "  highp float z = u_dof.y / max(u_dof.x - d * 5.0, 0.00001);\n" +
                "  float c = clamp((u_dof.z - z) * u_dof.w, 0.0, 1.0);\n" +
                "  return c * c;\n}\n";
            var nearWorld = "float worldBlur(highp float d) {\n" +
                "  highp float z = u_near.x / max(u_dof.x - d, 0.000001);\n" +
                "  float c = clamp((u_near.y - z) * u_near.z, 0.0, 1.0);\n" +
                "  return c * c * u_near.w;\n}\n";
            /* A viewmodel pixel's half smear in pixels, its distance in units,
             * with the world 1000 units behind it, and the filter's weights
             * for a sample d pixels away. */
            var viewmodelMotion = "highp vec2 vmSmear(ivec2 q) {\n" +
                "  return (texelFetch(u_vmVelocity, q, 0).rg * 255.0 - 128.0) * (u_vm.x / 127.0);\n}\n" +
                "highp float vmDistance(highp float d) {\n" +
                "  return d < 0.2 ? u_dof.y / max(u_dof.x - d * 5.0, 0.00001) : 1000.0;\n}\n" +
                "float cone(highp float d, highp float len) { return clamp(1.0 - d / max(len, 0.0001), 0.0, 1.0); }\n" +
                "float cylinder(highp float d, highp float len) {\n" +
                "  return 1.0 - smoothstep(0.95 * len, 1.05 * len + 0.0001, d);\n}\n";
            /* The drawn eyepiece of a raised scope, coming in with its window
             * (u_lensFx.w): a turned ring of blued steel from the glass's edge
             * to EYE_RING glass radii, worn to bare metal at its edges and in
             * patches, scratched round, with lathe lines and grime. Its wear
             * turns with the scope (u runs round the glass's own outline); its
             * light comes from the screen's upper left, scaled by the light of
             * the world in the glass, and it reflects the blurred world about
             * it. Hashes are integer, so every GPU draws the same metal, and
             * wrap round the ring. */
            var drawnEyepiece = "const float EYE_RING = 1.15;\nconst vec3 EYE_LIGHT = vec3(-0.43, 0.62, 0.656);\n" +
                /* The glass's radius in pixels, from the pixel and a half of
                 * its edge's fade. */
                "highp float eyeRadius() { return 1.5 / u_lensFx.z; }\n" +
                "highp float eyeHash(highp vec2 p, highp float period) {\n" +
                "  highp uvec2 q = uvec2(ivec2(mod(p.x, period), p.y) + 4096);\n" +
                "  highp uint h = (q.x * 1597334677u) ^ (q.y * 3812015801u);\n" +
                "  h = (h ^ (h >> 16u)) * 2246822519u;\n" +
                "  h ^= h >> 13u;\n" +
                "  return float(h) * (1.0 / 4294967295.0);\n}\n" +
                "float eyeNoise(highp vec2 p, highp float period) {\n" +
                "  highp vec2 i = floor(p);\n" +
                "  vec2 f = fract(p);\n" +
                "  f = f * f * (3.0 - 2.0 * f);\n" +
                "  return mix(mix(eyeHash(i, period), eyeHash(i + vec2(1.0, 0.0), period), f.x),\n" +
                "    mix(eyeHash(i + vec2(0.0, 1.0), period), eyeHash(i + vec2(1.0, 1.0), period), f.x), f.y);\n}\n" +
                "float eyeFbm(highp vec2 p, highp float period) {\n" +
                "  return 0.55 * eyeNoise(p, period) + 0.3 * eyeNoise(p * 2.0, period * 2.0) +\n" +
                "    0.15 * eyeNoise(p * 4.0, period * 4.0);\n}\n" +
                /* The light of the world in the glass, blurred. */
                "vec3 eyeLight() {\n" +
                "  highp vec2 c = u_lens.xy * u_px.xy, r = eyeRadius() * 0.45 * u_px.xy;\n" +
                "  return 0.2 * (texture(u_dofLarge, c).rgb + texture(u_dofLarge, c + vec2(r.x, 0.0)).rgb +\n" +
                "    texture(u_dofLarge, c - vec2(r.x, 0.0)).rgb + texture(u_dofLarge, c + vec2(0.0, r.y)).rgb +\n" +
                "    texture(u_dofLarge, c - vec2(0.0, r.y)).rgb);\n}\n" +
                "float eyeLevel(vec3 light) { return clamp(dot(light, vec3(0.2126, 0.7152, 0.0722)) * 2.2, 0.3, 1.4); }\n" +
                /* The ring at t across it, 0 at the glass and 1 at its rim, and u
                 * round it; rd is the screen's direction from the glass's centre.
                 * From the glass out: a black gap, a thin lip that catches the
                 * light, the matte crown, and the outer edge rolling away. */
                "vec3 eyeMetal(float t, highp float u, vec2 rd, vec3 light) {\n" +
                "  float lean = t < 0.12 ? -0.9 : mix(-0.25, 1.2, smoothstep(0.12, 1.0, t));\n" +
                "  vec3 n = vec3(rd * sin(lean), cos(lean));\n" +
                "  float diffuse = max(dot(n, EYE_LIGHT), 0.0);\n" +
                "  float spec = pow(max(dot(n, normalize(EYE_LIGHT + vec3(0.0, 0.0, 1.0))), 0.0), 8.0);\n" +
                "  float mottle = eyeFbm(vec2(u * 14.0, t * 2.5), 14.0);\n" +
                "  float edges = max(1.0 - smoothstep(0.1, 0.28, t), smoothstep(0.72, 0.96, t));\n" +
                "  float wear = clamp(0.6 * smoothstep(0.55, 0.8, eyeFbm(vec2(u * 22.0, t * 3.0 + 5.0), 22.0)) +\n" +
                "    0.7 * edges * smoothstep(0.4, 0.8, eyeNoise(vec2(u * 70.0, t * 2.0), 70.0)), 0.0, 1.0);\n" +
                "  float scratch = 0.5 * smoothstep(0.86, 0.97, eyeNoise(vec2(u * 90.0, t * 26.0), 90.0)) * (1.0 - 0.5 * edges);\n" +
                "  float lathe = 0.85 + 0.3 * eyeHash(vec2(floor(t * 30.0), 3.0), 4096.0);\n" +
                "  vec3 steel = vec3(0.2, 0.205, 0.215) * mix(0.7, 1.15, mottle);\n" +
                "  vec3 albedo = mix(steel, vec3(0.47, 0.45, 0.42), max(wear, scratch)) * lathe;\n" +
                "  vec3 env = texture(u_dofLarge, v_uv + n.xy * n.z * 2.0 * eyeRadius() * 0.35 * u_px.xy).rgb;\n" +
                "  float level = eyeLevel(light);\n" +
                "  vec3 col = albedo * (0.3 + 0.7 * diffuse) * level +\n" +
                "    env * (0.04 + 0.25 * pow(1.0 - n.z, 3.0)) * mix(0.4, 1.0, wear) +\n" +
                "    vec3(0.18 * spec * mix(0.3, 1.0, wear) * level);\n" +
                "  col += smoothstep(0.035, 0.06, t) * (1.0 - smoothstep(0.09, 0.13, t)) *\n" +
                "    vec3(0.16, 0.14, 0.12) * level * (0.5 + 0.5 * diffuse);\n" +
                "  return col * smoothstep(0.0, 0.04, t) * mix(1.0, 0.6, smoothstep(0.93, 1.0, t));\n}\n";
            /* Each viewmodel vertex is read from the mesh texture by its index,
             * which is itself read by gl_VertexID, so this pass binds no vertex
             * or index buffer for the GL emulation to lose track of. The smear
             * is stored as a fraction of the frame's longest in RG8. */
            var viewmodelVertex = gl.createShader(gl.VERTEX_SHADER);
            gl.shaderSource(viewmodelVertex, "#version 300 es\nuniform highp sampler2D u_mesh;\nuniform highp vec2 u_vm;\n" +
                "out highp vec2 v_uv;\nout highp vec2 v_smear;\nvoid main() {\n" +
                "  int t = int(u_vm.x) * 1024 + (gl_VertexID >> 2);\n" +
                "  highp vec4 q = texelFetch(u_mesh, ivec2(t & 1023, t >> 10), 0);\n" +
                "  int c = gl_VertexID & 3;\n" +
                "  int v = int(c == 0 ? q.x : (c == 1 ? q.y : (c == 2 ? q.z : q.w)));\n" +
                "  ivec2 p = ivec2((v & 511) << 1, v >> 9);\n" +
                "  v_uv = vec2(0.0);\n" +
                "  v_smear = texelFetch(u_mesh, p + ivec2(1, 0), 0).xy * u_vm.y;\n" +
                "  gl_Position = texelFetch(u_mesh, p, 0);\n}\n");
            gl.compileShader(viewmodelVertex);
            link("viewmodel", "in highp vec2 v_smear;\n" +
                "void main() {\n  o = vec4(clamp(v_smear * (127.0 / 255.0) + 128.0 / 255.0, 0.0, 1.0), 0.0, 1.0);\n}\n",
                ["u_mesh"], ["u_vm"], viewmodelVertex);
            /* Ambient occlusion at half the scene's resolution. Each world
             * pixel's position and normal come from the depth, and eight
             * depth samples spiral out over a fixed world radius: the closer
             * a sample sits above the surface, the more light it hides. The
             * viewmodel and the sky stay open. R holds the result, G and B
             * the linear depth for the blur. */
            link("ao", "uniform highp sampler2D u_depth;\nuniform highp vec4 u_aoView;\nuniform highp vec4 u_aoParams;\n" +
                "highp float aoZ(highp float d) { return u_aoView.x / max(u_aoView.y - d, 0.000001); }\n" +
                "highp vec3 aoPos(ivec2 q, highp float z) {\n" +
                "  highp vec2 ndc = (vec2(q) + 0.5) / u_aoParams.zw * 2.0 - 1.0;\n" +
                "  return vec3(ndc * z / u_aoView.zw, z);\n}\n" +
                "void main() {\n" +
                "  ivec2 m = ivec2(u_aoParams.zw) - 1;\n" +
                "  ivec2 c = min(ivec2(gl_FragCoord.xy) * 2, m);\n" +
                "  highp float d = texelFetch(u_depth, c, 0).r;\n" +
                "  highp float z = aoZ(d);\n" +
                "  highp float zn = clamp(z / 4096.0, 0.0, 1.0) * 255.0;\n" +
                "  o = vec4(1.0, floor(zn) / 255.0, fract(zn), 1.0);\n" +
                "  highp float r = u_aoParams.x * u_aoView.w / z * 0.5 * u_aoParams.w;\n" +
                "  if (d < 0.2 || d >= 0.9999 || r < 1.5) return;\n" +
                "  r = min(r, 0.1 * u_aoParams.w);\n" +
                "  highp vec3 p = aoPos(c, z);\n" +
                "  ivec2 cl = max(c - ivec2(2, 0), ivec2(0)), cr = min(c + ivec2(2, 0), m);\n" +
                "  ivec2 cd = max(c - ivec2(0, 2), ivec2(0)), cu = min(c + ivec2(0, 2), m);\n" +
                "  highp float zl = aoZ(texelFetch(u_depth, cl, 0).r), zr = aoZ(texelFetch(u_depth, cr, 0).r);\n" +
                "  highp float zd = aoZ(texelFetch(u_depth, cd, 0).r), zu = aoZ(texelFetch(u_depth, cu, 0).r);\n" +
                "  highp vec3 dx = abs(zr - z) < abs(z - zl) ? aoPos(cr, zr) - p : p - aoPos(cl, zl);\n" +
                "  highp vec3 dy = abs(zu - z) < abs(z - zd) ? aoPos(cu, zu) - p : p - aoPos(cd, zd);\n" +
                "  highp vec3 n = cross(dx, dy);\n" +
                "  n *= inversesqrt(max(dot(n, n), 1e-20));\n" +
                "  if (dot(n, p) > 0.0) n = -n;\n" +
                "  highp float r2 = u_aoParams.x * u_aoParams.x;\n" +
                "  float a0 = 6.2831853 * fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));\n" +
                "  float occlusion = 0.0;\n" +
                "  for (int i = 0; i < 8; ++i) {\n" +
                "    float a = a0 + float(i) * 2.3999632;\n" +
                "    ivec2 q = clamp(c + ivec2(vec2(cos(a), sin(a)) * (r * (float(i) + 0.5) * 0.125)), ivec2(0), m);\n" +
                "    highp float dq = texelFetch(u_depth, q, 0).r;\n" +
                "    if (dq < 0.2) continue;\n" +
                "    highp vec3 v = aoPos(q, aoZ(dq)) - p;\n" +
                "    highp float vv = dot(v, v);\n" +
                "    occlusion += max(dot(v, n) * inversesqrt(max(vv, 1e-6)) - 0.1, 0.0) * max(1.0 - vv / r2, 0.0);\n" +
                "  }\n" +
                /* Far surfaces fade out, where fog and texels blur. */
                "  float fade = 1.0 - smoothstep(1400.0, 2600.0, z);\n" +
                "  o.r = clamp(1.0 - u_aoParams.y * fade * occlusion * 0.25, 0.0, 1.0);\n}\n",
                ["u_depth"], ["u_aoView", "u_aoParams"]);
            /* A 4 x 4 box over the half-resolution occlusion that keeps to
             * samples near the centre's depth, so edges stay sharp. */
            link("aoBlur", "uniform sampler2D u_src;\n" +
                "void main() {\n" +
                "  ivec2 c = ivec2(gl_FragCoord.xy), m = textureSize(u_src, 0) - 1;\n" +
                "  vec4 centre = texelFetch(u_src, c, 0);\n" +
                "  highp float zc = centre.g + centre.b / 255.0;\n" +
                "  float sum = 0.0, weight = 0.0;\n" +
                "  for (int y = -2; y < 2; ++y)\n" +
                "    for (int x = -2; x < 2; ++x) {\n" +
                "      vec4 t = texelFetch(u_src, clamp(c + ivec2(x, y), ivec2(0), m), 0);\n" +
                "      float w = max(1.0 - abs(t.g + t.b / 255.0 - zc) / (0.03 * zc + 0.0006), 0.0);\n" +
                "      sum += t.r * w;\n" +
                "      weight += w;\n" +
                "    }\n" +
                "  o = vec4(weight > 0.0 ? sum / weight : centre.r, 0.0, 0.0, 1.0);\n}\n",
                ["u_src"], []);
            link("dof", "uniform sampler2D u_src;\nuniform highp sampler2D u_depth;\n" +
                "uniform highp vec2 u_texel;\nuniform highp vec4 u_dof;\n" + coc +
                "float cocAt(ivec2 p, ivec2 m) { return vmBlur(texelFetch(u_depth, min(p, m), 0).r); }\n" +
                "void main() {\n" + quarter +
                "  ivec2 b = ivec2(gl_FragCoord.xy) * 4;\n" +
                "  ivec2 m = textureSize(u_depth, 0) - 1;\n" +
                "  float k = max(max(cocAt(b, m), cocAt(b + ivec2(3, 0), m)),\n" +
                "    max(max(cocAt(b + ivec2(0, 3), m), cocAt(b + ivec2(3, 3), m)), cocAt(b + ivec2(2, 2), m)));\n" +
                "  o = vec4(0.25 * (c + d + e + f), k);\n}\n",
                ["u_src", "u_depth"], ["u_texel", "u_dof"]);
            link("compose", "uniform sampler2D u_scene;\nuniform sampler2D u_bloom;\n" +
                "uniform highp sampler2D u_depth;\nuniform sampler2D u_dofMed;\nuniform sampler2D u_dofLarge;\n" +
                "uniform highp vec4 u_px;\nuniform vec4 u_glow;\nuniform highp vec4 u_dof;\nuniform vec4 u_dof2;\nuniform vec4 u_film;\n" +
                "uniform highp vec4 u_near;\nuniform vec2 u_tier;\n" +
                "uniform highp vec4 u_motion;\nuniform highp mat3 u_motionRotation;\nuniform highp vec3 u_motionMove;\n" +
                "uniform highp sampler2D u_vmVelocity;\nuniform highp sampler2D u_vmMesh;\nuniform highp vec4 u_vm;\n" +
                "uniform highp vec4 u_lens;\nuniform highp vec4 u_lensInv;\nuniform vec4 u_lensFx;\n" +
                "uniform sampler2D u_ao;\nuniform highp vec4 u_aoMix;\n" +
                "uniform highp vec4 u_haze;\nuniform highp vec3 u_hazeUp;\nuniform highp vec3 u_hazeSun;\n" +
                "uniform vec3 u_hazeSunColor;\nuniform highp vec3 u_hazeProj;\n" +
                coc + nearWorld + viewmodelMotion +
                drawnEyepiece + "void main() {\n" +
                "  vec3 c = texture(u_scene, v_uv).rgb;\n" +
                "  vec3 col = c;\n" +
                "  bool smeared = false;\n" +
                /* Inside the scope's glass: 1 within its outline, which the
                 * stencil kept clear of the rifle, fading over a pixel. */
                "  float lensIn = 0.0;\n" +
                "  highp float lensR = 2.0;\n" +
                "  highp vec2 lensDp = vec2(0.0), lensQ = vec2(1.0, 0.0);\n" +
                "  if (u_lens.z > 0.0) {\n" +
                "    lensDp = gl_FragCoord.xy - u_lens.xy;\n" +
                "    lensQ = vec2(dot(u_lensInv.xy, lensDp), dot(u_lensInv.zw, lensDp));\n" +
                "    lensR = length(lensQ);\n" +
                "    lensIn = 1.0 - smoothstep(1.0 - u_lensFx.z, 1.0, lensR);\n" +
                "  }\n" +
                /* Toward the edge of a scope's glass its colours part a little,
                 * red outward and blue inward. */
                "  if (u_lensFx.w > 0.0 && lensR > 0.8 && lensR < 1.0) {\n" +
                "    highp vec2 shift = lensDp / max(length(lensDp), 1.0) *\n" +
                "      (0.0035 * eyeRadius() * u_lensFx.w * smoothstep(0.8, 1.0, lensR)) * u_px.xy;\n" +
                "    c = vec3(texture(u_scene, v_uv + shift).r, c.g, texture(u_scene, v_uv - shift).b);\n" +
                "    col = c;\n" +
                "  }\n" +
                /* Each world pixel goes back to where the last frame's camera saw
                 * it, and the scene is averaged along that path. The viewmodel
                 * moves with the camera: it stays sharp and stays out of the
                 * world's average. */
                "  if (u_motion.x > 0.0) {\n" +
                "    highp float depth = texelFetch(u_depth, ivec2(gl_FragCoord.xy), 0).r;\n" +
                "    highp vec2 ndc = v_uv * 2.0 - 1.0;\n" +
                "    highp float z = min(u_near.x / max(u_dof.x - depth, 0.000001), 1000000.0);\n" +
                "    highp vec3 p = u_motionRotation * vec3(ndc * z / u_motion.yz, z) + u_motionMove;\n" +
                "    highp vec2 v = (ndc - p.xy * u_motion.yz / max(p.z, 0.001)) * (0.5 * u_motion.x);\n" +
                "    float len = length(v / u_px.xy);\n" +
                "    if (depth >= 0.2 && p.z > 1.0 && len > 1.0) {\n" +
                "      float n = min(len, u_motion.w);\n" +
                "      highp vec2 path = v / u_px.xy * (n / len);\n" +
                "      int taps = int(clamp(ceil(n * 0.25), 3.0, 16.0));\n" +
                "      float jitter = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));\n" +
                "      ivec2 last = textureSize(u_scene, 0) - 1;\n" +
                "      vec3 sum = c;\n" +
                "      float weight = 1.0;\n" +
                /* Color and depth come from the same texel, so no filtered tap
                 * blends the viewmodel into the world. */
                "      for (int i = 0; i < 16; ++i) {\n" +
                "        if (i >= taps) break;\n" +
                "        ivec2 q = clamp(ivec2(gl_FragCoord.xy + path * ((float(i) + jitter) / float(taps) - 0.5)),\n" +
                "          ivec2(0), last);\n" +
                "        float w = step(0.2, texelFetch(u_depth, q, 0).r);\n" +
                "        sum += texelFetch(u_scene, q, 0).rgb * w;\n" +
                "        weight += w;\n" +
                "      }\n" +
                "      col = sum / weight;\n" +
                "      smeared = true;\n" +
                "    }\n" +
                "  }\n" +
                /* The viewmodel smears along its own motion. Each pixel near it
                 * gathers along the largest viewmodel smear within a tile, as in
                 * McGuire et al.'s reconstruction filter: a sample's smear spreads
                 * over this pixel when it reaches it from in front, this pixel's
                 * own smear shows what is behind it, and two moving samples blend.
                 * The viewmodel is always in front of the world. */
                "  if (u_vm.x > 0.0) {\n" +
                "    ivec2 px = ivec2(gl_FragCoord.xy);\n" +
                "    int k = int(u_vm.y);\n" +
                "    highp int tile = int(u_vm.z) * 1024 + (px.y / k) * int(u_vm.w) + px.x / k;\n" +
                "    highp vec2 reach = texelFetch(u_vmMesh, ivec2(tile & 1023, tile >> 10), 0).xy;\n" +
                "    highp float span = length(reach);\n" +
                "    if (span >= 0.5) {\n" +
                "      highp float dX = texelFetch(u_depth, px, 0).r;\n" +
                "      highp float lenX = dX < 0.2 ? length(vmSmear(px)) : 0.0;\n" +
                "      highp float zX = vmDistance(dX);\n" +
                "      highp float self = 1.0 / max(lenX, 0.5);\n" +
                "      highp vec3 sum = col * self;\n" +
                "      highp float weight = self;\n" +
                "      int taps = int(clamp(ceil(span * 0.5), 3.0, 24.0));\n" +
                "      float jitter = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));\n" +
                "      ivec2 last = textureSize(u_scene, 0) - 1;\n" +
                "      for (int i = 0; i < 24; ++i) {\n" +
                "        if (i >= taps) break;\n" +
                "        highp vec2 offset = reach * ((float(i) + jitter) / float(taps) * 2.0 - 1.0);\n" +
                "        ivec2 q = clamp(ivec2(gl_FragCoord.xy + offset), ivec2(0), last);\n" +
                "        highp float dY = texelFetch(u_depth, q, 0).r;\n" +
                "        if (dX >= 0.2 && dY >= 0.2) continue;\n" +
                "        highp float lenY = dY < 0.2 ? length(vmSmear(q)) : 0.0;\n" +
                "        highp float zY = vmDistance(dY), d = length(offset);\n" +
                "        float a = clamp(1.0 + zX - zY, 0.0, 1.0) * cone(d, lenY) +\n" +
                "          clamp(1.0 + zY - zX, 0.0, 1.0) * cone(d, lenX) + 2.0 * cylinder(d, lenY) * cylinder(d, lenX);\n" +
                "        if (a <= 0.0) continue;\n" +
                "        sum += texelFetch(u_scene, q, 0).rgb * a;\n" +
                "        weight += a;\n" +
                "      }\n" +
                "      if (weight > self + 0.001) {\n" +
                "        col = sum / weight;\n" +
                "        smeared = true;\n" +
                "      }\n" +
                "    }\n" +
                "  }\n" +
                "  if (u_film.x > 0.0 && !smeared) {\n" +
                "    vec3 n = texture(u_scene, v_uv + vec2(0.0, u_px.y)).rgb;\n" +
                "    vec3 s = texture(u_scene, v_uv - vec2(0.0, u_px.y)).rgb;\n" +
                "    vec3 e = texture(u_scene, v_uv + vec2(u_px.x, 0.0)).rgb;\n" +
                "    vec3 w = texture(u_scene, v_uv - vec2(u_px.x, 0.0)).rgb;\n" +
                "    vec3 lo = min(c, min(min(n, s), min(e, w)));\n" +
                "    vec3 hi = max(c, max(max(n, s), max(e, w)));\n" +
                "    vec3 wt = -u_film.x * sqrt(clamp(min(lo, 1.0 - hi) / max(hi, vec3(0.0001)), 0.0, 1.0));\n" +
                "    col = clamp((c + (n + s + e + w) * wt) / (1.0 + 4.0 * wt), 0.0, 1.0);\n" +
                "  }\n" +
                /* The blurred occlusion, half a texel back where its 4 x 4
                 * box centres; the viewmodel keeps its own light. */
                "  if (u_aoMix.x > 0.0 && texelFetch(u_depth, ivec2(gl_FragCoord.xy), 0).r >= 0.2)\n" +
                "    col *= texture(u_ao, v_uv + u_aoMix.yz).r;\n" +
                "  if (u_dof2.x > 0.0 || u_dof2.w > 0.0 || u_lens.z > 0.0) {\n" +
                "    highp float depth = texelFetch(u_depth, ivec2(gl_FragCoord.xy), 0).r;\n" +
                "    vec4 large = texture(u_dofLarge, v_uv);\n" +
                "    float k;\n" +
                "    if (depth < 0.2) {\n" +
                "      k = vmBlur(depth);\n" +
                "    } else {\n" +
                "      vec2 centered = (v_uv - 0.5) * vec2(u_px.z, 1.0);\n" +
                "      k = max(worldBlur(depth),\n" +
                "        clamp(2.0 * large.a, 0.0, 1.0) * smoothstep(u_dof2.y, u_dof2.z, length(centered)));\n" +
                "    }\n" +
                "    k = max(k * u_dof2.x, u_dof2.w);\n" +
                /* Through an open scope the world is sharp and the eyepiece is
                 * in focus; around it the magnified world is thrown out of
                 * focus, as WWII's scopes do. */
                "    if (u_lens.z > 0.0) {\n" +
                "      if (depth < 0.2) k *= 1.0 - u_lens.w;\n" +
                "      else k = max(k * (1.0 - lensIn * u_lens.w), u_lensFx.x * smoothstep(1.0, 1.2, lensR));\n" +
                "    }\n" +
                "    if (k > 0.003) {\n" +
                "      vec3 small = 0.25 * (texture(u_scene, v_uv + u_px.xy * vec2(0.5, 1.5)).rgb +\n" +
                "        texture(u_scene, v_uv + u_px.xy * vec2(-1.5, 0.5)).rgb +\n" +
                "        texture(u_scene, v_uv + u_px.xy * vec2(-0.5, -1.5)).rgb +\n" +
                "        texture(u_scene, v_uv + u_px.xy * vec2(1.5, -0.5)).rgb);\n" +
                "      vec3 med = texture(u_dofMed, v_uv).rgb;\n" +
                "      col = k < u_tier.x ? mix(col, small, k / u_tier.x) :\n" +
                "        (k < u_tier.y ? mix(small, med, (k - u_tier.x) / (u_tier.y - u_tier.x)) :\n" +
                "          mix(med, large.rgb, (k - u_tier.y) / (1.0 - u_tier.y)));\n" +
                "    }\n" +
                "  }\n" +
                /* The air between the eye and the world, in the map's fog
                 * color: denser at the eye's height and below, thinner above
                 * it, integrated along the ray, and brighter toward the sun.
                 * The viewmodel and the sky keep their own. */
                "  if (u_haze.a > 0.0) {\n" +
                "    highp float hd = texelFetch(u_depth, ivec2(gl_FragCoord.xy), 0).r;\n" +
                "    if (hd >= 0.2 && hd < 0.99999) {\n" +
                "      highp float hz = u_near.x / max(u_dof.x - hd, 0.000001);\n" +
                "      highp vec3 hp = vec3((v_uv * 2.0 - 1.0) * hz / u_hazeProj.xy, hz);\n" +
                "      highp float hdist = length(hp);\n" +
                "      highp float hk = dot(hp, u_hazeUp) * u_hazeProj.z;\n" +
                "      float hf = abs(hk) > 0.001 ? (1.0 - exp(-hk)) / hk : 1.0;\n" +
                "      float haze = 1.0 - exp(-u_haze.a * hdist * clamp(hf, 0.25, 3.0));\n" +
                "      float glow = pow(max(dot(hp / hdist, u_hazeSun), 0.0), 6.0);\n" +
                "      col = mix(col, u_haze.rgb + u_hazeSunColor * glow, haze);\n" +
                "    }\n" +
                "  }\n" +
                /* Until the scope is at the eye its glass shows a dim, curved
                 * reflection of the world. */
                "  if (lensIn > 0.0 && u_lens.w < 1.0) {\n" +
                "    highp vec2 lc = u_lens.xy * u_px.xy;\n" +
                "    vec3 glass = texture(u_dofLarge, clamp(lc + (v_uv - lc) * 1.8, 0.0, 1.0)).rgb;\n" +
                "    glass = glass * (0.16 + 0.3 * (1.0 - lensR * lensR)) + vec3(0.012, 0.016, 0.024);\n" +
                "    col = mix(col, glass, lensIn * (1.0 - u_lens.w));\n" +
                "  }\n" +
                /* A scope's drawn eyepiece: the glass darkens into its seat, the
                 * ring shades what lies just round it, and the ring itself. */
                "  if (u_lens.z > 0.0 && u_lensFx.w > 0.0) {\n" +
                "    float beyond = smoothstep(EYE_RING - u_lensFx.z, EYE_RING, lensR);\n" +
                "    float ring = smoothstep(1.0 - u_lensFx.z, 1.0, lensR) * (1.0 - beyond);\n" +
                "    col *= 1.0 - 0.5 * u_lensFx.w * lensIn * smoothstep(0.86, 1.0, lensR);\n" +
                "    col *= 1.0 - 0.25 * u_lensFx.w * beyond * (1.0 - smoothstep(EYE_RING, EYE_RING + 0.08, lensR));\n" +
                "    if (ring > 0.0) {\n" +
                "      highp float u = atan(lensQ.y, lensQ.x) * 0.15915494 + 0.5;\n" +
                "      vec3 metal = eyeMetal(clamp((lensR - 1.0) / (EYE_RING - 1.0), 0.0, 1.0), u,\n" +
                "        lensDp / max(length(lensDp), 0.001), eyeLight());\n" +
                "      col = mix(col, metal, ring * u_lensFx.w);\n" +
                "    }\n" +
                "  }\n" +
                "  if (u_glow.x > 0.0)\n" +
                "    col = 1.0 - (1.0 - col) * (1.0 - min(texture(u_bloom, v_uv).rgb, vec3(1.0)));\n" +
                "  if (u_film.w > 0.0) {\n" +
                "    float l = dot(col, vec3(0.2126, 0.7152, 0.0722));\n" +
                "    col = mix(vec3(l), col, u_film.y);\n" +
                "    col = clamp(col * mix(vec3(0.975, 0.99, 1.03), vec3(1.025, 1.005, 0.965), smoothstep(0.05, 0.75, l)), 0.0, 1.0);\n" +
                "    col += u_film.z * col * (1.0 - col) * (col - 0.35) * 1.5384615;\n" +
                "  }\n" +
                "  if (u_px.w > 0.0)\n" +
                "    col = mix(col, vec3(dot(col, vec3(0.2126, 0.7152, 0.0722))), u_px.w);\n" +
                /* Holding the breath blacks out everything around the glass,
                 * the rifle's glow included. */
                "  if (u_lens.z > 0.0)\n" +
                "    col *= 1.0 - u_lensFx.y * (1.0 - lensIn);\n" +
                "  o = vec4(col, 1.0);\n}\n",
                ["u_scene", "u_bloom", "u_depth", "u_dofMed", "u_dofLarge", "u_vmVelocity", "u_vmMesh", "u_ao"],
                ["u_px", "u_glow", "u_dof", "u_dof2", "u_film", "u_near", "u_tier",
                 "u_motion", "u_motionRotation", "u_motionMove", "u_vm", "u_lens", "u_lensInv", "u_lensFx", "u_aoMix",
                 "u_haze", "u_hazeUp", "u_hazeSun", "u_hazeSunColor", "u_hazeProj"]);
            /* A scene smaller than the canvas, filtered up to it with Catmull-Rom
             * in five bilinear taps (the corner taps weigh under 0.5%), as the
             * page scales the canvas again with its own bilinear filter. */
            link("upscale", "uniform sampler2D u_src;\nuniform highp vec2 u_size;\n" +
                "void main() {\n" +
                "  highp vec2 p = v_uv * u_size;\n" +
                "  highp vec2 c1 = floor(p - 0.5) + 0.5;\n" +
                "  highp vec2 f = p - c1;\n" +
                "  highp vec2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));\n" +
                "  highp vec2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);\n" +
                "  highp vec2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));\n" +
                "  highp vec2 w3 = f * f * (-0.5 + 0.5 * f);\n" +
                "  highp vec2 w12 = w1 + w2;\n" +
                "  highp vec2 t0 = (c1 - 1.0) / u_size, t3 = (c1 + 2.0) / u_size, t12 = (c1 + w2 / w12) / u_size;\n" +
                "  vec3 c = texture(u_src, vec2(t12.x, t0.y)).rgb * (w12.x * w0.y) +\n" +
                "    texture(u_src, vec2(t0.x, t12.y)).rgb * (w0.x * w12.y) +\n" +
                "    texture(u_src, t12).rgb * (w12.x * w12.y) +\n" +
                "    texture(u_src, vec2(t3.x, t12.y)).rgb * (w3.x * w12.y) +\n" +
                "    texture(u_src, vec2(t12.x, t3.y)).rgb * (w12.x * w3.y);\n" +
                "  float total = w12.x * w0.y + w0.x * w12.y + w12.x * w12.y + w3.x * w12.y + w12.x * w3.y;\n" +
                "  o = vec4(clamp(c / total, 0.0, 1.0), 1.0);\n}\n",
                ["u_src"], ["u_size"]);
            /* The scope's glass outline as a 64-sided fan in normalized device
             * coordinates, for the stencil that keeps the rifle out of it. */
            var lensVs = gl.createShader(gl.VERTEX_SHADER);
            gl.shaderSource(lensVs, "#version 300 es\nuniform highp vec2 u_center;\nuniform highp mat2 u_axes;\n" +
                "void main() {\n" +
                "  float a = float(gl_VertexID - 1) * 0.09817477042;\n" +
                "  vec2 rim = gl_VertexID == 0 ? vec2(0.0) : u_axes * vec2(cos(a), sin(a));\n" +
                "  gl_Position = vec4(u_center + rim, 0.0, 1.0);\n}\n");
            gl.compileShader(lensVs);
            var lensFs = gl.createShader(gl.FRAGMENT_SHADER);
            gl.shaderSource(lensFs, "#version 300 es\nprecision mediump float;\nout vec4 o;\nvoid main() { o = vec4(0.0); }\n");
            gl.compileShader(lensFs);
            var lens = gl.createProgram();
            gl.attachShader(lens, lensVs);
            gl.attachShader(lens, lensFs);
            gl.linkProgram(lens);
            if (gl.getProgramParameter(lens, gl.LINK_STATUS)) {
                programs.lens = { program: lens, u_center: gl.getUniformLocation(lens, "u_center"),
                    u_axes: gl.getUniformLocation(lens, "u_axes") };
            } else {
                console.warn("[scene-fx] lens failed: " + gl.getProgramInfoLog(lens) + " " +
                    gl.getShaderInfoLog(lensVs) + " " + gl.getShaderInfoLog(lensFs));
                ok = false;
            }
            gl.useProgram(previous);
            if (!ok) return false;
            fx.programs = programs;
            fx.vao = gl.createVertexArray();
            return true;
        };
        /* The canvas's viewport, which a scaled scene's draws set in canvas
         * pixels, goes to the scene's at its scale. The engine sets no scissor
         * box, so the canvas-sized default covers the scene's target. */
        fx.scale = function(screenWidth, screenHeight) {
            var sx = fx.width / screenWidth, sy = fx.height / screenHeight;
            var viewport = gl.viewport;
            fx.screenWidth = screenWidth;
            fx.screenHeight = screenHeight;
            fx.ownViewport = Object.getOwnPropertyDescriptor(gl, "viewport");
            fx.canvasViewport = Array.from(gl.getParameter(gl.VIEWPORT));
            gl.viewport = function(x, y, w, h) {
                var x0 = Math.round(x * sx), y0 = Math.round(y * sy);
                fx.canvasViewport = [x, y, w, h];
                viewport.call(gl, x0, y0, Math.round((x + w) * sx) - x0, Math.round((y + h) * sy) - y0);
            };
            fx.scaled = true;
            gl.viewport.apply(gl, fx.canvasViewport);
        };
        fx.unscale = function() {
            if (!fx.scaled) return false;
            fx.scaled = false;
            if (fx.ownViewport) Object.defineProperty(gl, "viewport", fx.ownViewport);
            else delete gl.viewport;
            gl.viewport.apply(gl, fx.canvasViewport);
            return true;
        };
        fx.release = function() {
            var t = fx.t;
            fx.t = null;
            fx.width = fx.height = fx.samples = 0;
            if (!t) return;
            for (var i = 0; i < t.framebuffers.length; ++i) gl.deleteFramebuffer(t.framebuffers[i]);
            for (var j = 0; j < t.renderbuffers.length; ++j) gl.deleteRenderbuffer(t.renderbuffers[j]);
            for (var k = 0; k < t.textures.length; ++k) gl.deleteTexture(t.textures[k]);
        };
        fx.allocate = function(w, h, samples) {
            fx.release();
            if (!fx.programs && !fx.build()) return false;
            samples = samples > 1 ? Math.min(samples, fx.maxSamples) : 0;
            var t = {
                qw: Math.max(1, Math.ceil(w / 4)), qh: Math.max(1, Math.ceil(h / 4)),
                ew: Math.max(1, Math.ceil(w / 8)), eh: Math.max(1, Math.ceil(h / 8)),
                hw: Math.max(1, Math.ceil(w / 2)), hh: Math.max(1, Math.ceil(h / 2)),
                textures: [], framebuffers: [], renderbuffers: [], ms: null, q: [], qFbo: [], e: [], eFbo: [],
                ao: [], aoFbo: []
            };
            var ok = true;
            var previousUnit = gl.getParameter(gl.ACTIVE_TEXTURE);
            gl.activeTexture(gl.TEXTURE8);
            var texture = function(format, tw, th, filter) {
                var id = gl.createTexture();
                t.textures.push(id);
                gl.bindTexture(gl.TEXTURE_2D, id);
                gl.texStorage2D(gl.TEXTURE_2D, 1, format, tw, th);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, filter);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, filter);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
                return id;
            };
            var renderbuffer = function(format) {
                var id = gl.createRenderbuffer();
                t.renderbuffers.push(id);
                gl.bindRenderbuffer(gl.RENDERBUFFER, id);
                gl.renderbufferStorageMultisample(gl.RENDERBUFFER, samples, format, w, h);
                return id;
            };
            var framebuffer = function(color, depth, colorBuffer, depthBuffer) {
                var id = gl.createFramebuffer();
                t.framebuffers.push(id);
                gl.bindFramebuffer(gl.FRAMEBUFFER, id);
                if (color) gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, color, 0);
                if (depth) gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.DEPTH_STENCIL_ATTACHMENT, gl.TEXTURE_2D, depth, 0);
                if (colorBuffer) gl.framebufferRenderbuffer(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.RENDERBUFFER, colorBuffer);
                if (depthBuffer) gl.framebufferRenderbuffer(gl.FRAMEBUFFER, gl.DEPTH_STENCIL_ATTACHMENT, gl.RENDERBUFFER, depthBuffer);
                if (gl.checkFramebufferStatus(gl.FRAMEBUFFER) !== gl.FRAMEBUFFER_COMPLETE) ok = false;
                return id;
            };
            /* RGB8 keeps the canvas behaviour of alpha:false: destination alpha reads as one. */
            t.color = texture(gl.RGB8, w, h, gl.LINEAR);
            t.depth = texture(gl.DEPTH24_STENCIL8, w, h, gl.NEAREST);
            t.scene = framebuffer(t.color, t.depth, null, null);
            if (samples > 1) {
                t.ms = framebuffer(null, null, renderbuffer(gl.RGB8), renderbuffer(gl.DEPTH24_STENCIL8));
                gl.bindRenderbuffer(gl.RENDERBUFFER, null);
            }
            t.out = texture(gl.RGB8, w, h, gl.LINEAR);
            t.outFbo = framebuffer(t.out, null, null, null);
            for (var i = 0; i < 2; ++i) {
                t.q.push(texture(gl.RGBA8, t.qw, t.qh, gl.LINEAR));
                t.qFbo.push(framebuffer(t.q[i], null, null, null));
                t.e.push(texture(gl.RGBA8, t.ew, t.eh, gl.LINEAR));
                t.eFbo.push(framebuffer(t.e[i], null, null, null));
                /* The raw occlusion and its depth, then the blurred occlusion. */
                t.ao.push(texture(gl.RGBA8, t.hw, t.hh, i ? gl.LINEAR : gl.NEAREST));
                t.aoFbo.push(framebuffer(t.ao[i], null, null, null));
            }
            gl.bindTexture(gl.TEXTURE_2D, null);
            gl.activeTexture(previousUnit);
            gl.bindFramebuffer(gl.FRAMEBUFFER, null);
            fx.t = t;
            if (!ok) {
                fx.release();
                return false;
            }
            fx.width = w;
            fx.height = h;
            fx.samples = samples;
            return true;
        };
        fx.texture = function(unit, texture) {
            gl.activeTexture(gl.TEXTURE8 + unit);
            gl.bindTexture(gl.TEXTURE_2D, texture);
        };
        fx.pass = function(program, framebuffer, w, h) {
            gl.useProgram(program.program);
            gl.bindFramebuffer(gl.FRAMEBUFFER, framebuffer);
            gl.viewport(0, 0, w, h);
            gl.drawArrays(gl.TRIANGLES, 0, 3);
        };
        /* The viewmodel's smear target, made the first time it moves: each
         * viewmodel pixel's smear with its own depth buffer, and the 64 rows
         * of the mesh it is drawn from. The scene's target owns them. */
        fx.viewmodel = function() {
            var t = fx.t;
            if (t.vm) return t.vm.ok;
            var vm = t.vm = { ok: false };
            var texture = function(format, tw, th) {
                var id = gl.createTexture();
                t.textures.push(id);
                fx.texture(0, id);
                gl.texStorage2D(gl.TEXTURE_2D, 1, format, tw, th);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
                return id;
            };
            vm.smear = texture(gl.RGBA8, fx.width, fx.height);
            vm.mesh = texture(gl.RGBA32F, 1024, 64);
            fx.texture(0, null);
            vm.depth = gl.createRenderbuffer();
            t.renderbuffers.push(vm.depth);
            gl.bindRenderbuffer(gl.RENDERBUFFER, vm.depth);
            gl.renderbufferStorage(gl.RENDERBUFFER, gl.DEPTH_COMPONENT24, fx.width, fx.height);
            gl.bindRenderbuffer(gl.RENDERBUFFER, null);
            vm.fbo = gl.createFramebuffer();
            t.framebuffers.push(vm.fbo);
            gl.bindFramebuffer(gl.FRAMEBUFFER, vm.fbo);
            gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, vm.smear, 0);
            gl.framebufferRenderbuffer(gl.FRAMEBUFFER, gl.DEPTH_ATTACHMENT, gl.RENDERBUFFER, vm.depth);
            vm.ok = gl.checkFramebufferStatus(gl.FRAMEBUFFER) === gl.FRAMEBUFFER_COMPLETE;
            if (!vm.ok) console.warn("[scene-fx] viewmodel motion target unavailable; the viewmodel does not blur");
            return vm.ok;
        };
        /* The scene's depth for z-feathered effects (web_effects.c), copied
         * once a view before its first feathered draw: effects write no depth,
         * so it holds the world they fade into. The scene's own depth cannot
         * be sampled while it is the target. It stays bound on unit 19, past
         * the units the compositor binds. */
        fx.featherDepth = function() {
            var t = fx.t;
            if (!t || !fx.drawing) return false;
            var f = t.feather;
            if (!f) {
                f = t.feather = { ok: false, ready: false };
                var unit = gl.getParameter(gl.ACTIVE_TEXTURE);
                f.depth = gl.createTexture();
                t.textures.push(f.depth);
                gl.activeTexture(gl.TEXTURE19);
                gl.bindTexture(gl.TEXTURE_2D, f.depth);
                gl.texStorage2D(gl.TEXTURE_2D, 1, gl.DEPTH24_STENCIL8, fx.width, fx.height);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
                gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
                gl.activeTexture(unit);
                f.fbo = gl.createFramebuffer();
                t.framebuffers.push(f.fbo);
                gl.bindFramebuffer(gl.FRAMEBUFFER, f.fbo);
                gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.DEPTH_STENCIL_ATTACHMENT, gl.TEXTURE_2D, f.depth, 0);
                f.ok = gl.checkFramebufferStatus(gl.FRAMEBUFFER) === gl.FRAMEBUFFER_COMPLETE;
                gl.bindFramebuffer(gl.FRAMEBUFFER, t.ms || t.scene);
                if (!f.ok) console.warn("[scene-fx] effects' depth copy unavailable; effects are not feathered");
            }
            if (!f.ok) return false;
            if (!f.ready) {
                var scissor = gl.isEnabled(gl.SCISSOR_TEST);
                if (scissor) gl.disable(gl.SCISSOR_TEST);
                gl.bindFramebuffer(gl.READ_FRAMEBUFFER, t.ms || t.scene);
                gl.bindFramebuffer(gl.DRAW_FRAMEBUFFER, f.fbo);
                gl.blitFramebuffer(0, 0, fx.width, fx.height, 0, 0, fx.width, fx.height, gl.DEPTH_BUFFER_BIT, gl.NEAREST);
                gl.bindFramebuffer(gl.FRAMEBUFFER, t.ms || t.scene);
                if (scissor) gl.enable(gl.SCISSOR_TEST);
                f.ready = true;
            }
            return true;
        };
        /* Normalized Gaussian over texels 0..2 * pairs, folded into bilinear taps:
         * [center weight, pair weights...] and [pair offsets...]. */
        fx.kernel = function(sigma, pairs) {
            var g = [], total = 0;
            for (var i = 0; i <= pairs * 2; ++i) {
                g.push(sigma > 0.05 ? Math.exp(-i * i / (2 * sigma * sigma)) : (i ? 0 : 1));
                total += i ? 2 * g[i] : g[i];
            }
            var weights = [g[0] / total], offsets = [];
            for (var p = 0; p < pairs; ++p) {
                var a = g[2 * p + 1], b = g[2 * p + 2];
                weights.push((a + b) / total);
                offsets.push(2 * p + 1 + (a + b > 0.000001 ? b / (a + b) : 0));
            }
            return { weights: weights, offsets: offsets };
        };
        fx.compose = function(o, mesh) {
            var t = fx.t, P = fx.programs, H = HEAPF32;
            var scaled = fx.unscale();
            fx.drawing = false;
            if (!t || !P) return;
            var w = fx.width, h = fx.height, qw = t.qw, qh = t.qh, ew = t.ew, eh = t.eh;
            var glow0 = H[o], glow1 = H[o + 1], ads = H[o + 6], motion = H[o + 18], backdrop = H[o + 33];
            var gray = H[o + 34], smear = H[o + 35];
            var lensPresence = H[o + 48], lensBlur = H[o + 50];
            /* Any window the stencil cut also needs its glass. */
            var lens = lensPresence > 0, scopeBlur = lens && lensBlur > 0.004;
            var glow = glow0 > 0 || glow1 > 0, blurBehind = backdrop > 0.004, dof = ads > 0.004 || blurBehind || lens;
            var ao = H[o + 55] > 0, haze = H[o + 62] > 0;
            var lines = h / 480;
            var program = gl.getParameter(gl.CURRENT_PROGRAM);
            var vao = gl.getParameter(gl.VERTEX_ARRAY_BINDING);
            var unit = gl.getParameter(gl.ACTIVE_TEXTURE);
            var viewport = gl.getParameter(gl.VIEWPORT);
            var caps = [gl.SCISSOR_TEST, gl.BLEND, gl.DEPTH_TEST, gl.CULL_FACE, gl.STENCIL_TEST, gl.POLYGON_OFFSET_FILL,
                gl.SAMPLE_ALPHA_TO_COVERAGE];
            var enabled = [];
            for (var c = 0; c < caps.length; ++c) {
                enabled.push(gl.isEnabled(caps[c]));
                gl.disable(caps[c]);
            }
            var vm = smear > 0 && mesh > 0 && fx.viewmodel() ? t.vm : null;
            if (t.ms) {
                gl.bindFramebuffer(gl.READ_FRAMEBUFFER, t.ms);
                gl.bindFramebuffer(gl.DRAW_FRAMEBUFFER, t.scene);
                gl.blitFramebuffer(0, 0, w, h, 0, 0, w, h,
                    dof || motion > 0 || vm || ao || haze ? (gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT) : gl.COLOR_BUFFER_BIT, gl.NEAREST);
                gl.invalidateFramebuffer(gl.READ_FRAMEBUFFER, fx.discard);
            }
            /* Reading the write masks back would stall on the GPU process. Every
             * D3D9 clear and draw sets both masks again, and these passes run
             * without depth testing, so only the color mask is forced here. */
            gl.colorMask(true, true, true, true);
            gl.bindVertexArray(fx.vao);
            if (vm) {
                /* The viewmodel's triangles draw their smear with their own
                 * depth test, so the nearest part's smear wins where parts
                 * overlap. The depth mask stays on: the next D3D9 draw or clear
                 * sets it again. */
                fx.texture(0, vm.mesh);
                gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, 1024, H[o + 39], gl.RGBA, gl.FLOAT, H, mesh);
                gl.useProgram(P.viewmodel.program);
                gl.uniform2f(P.viewmodel.u_vm, H[o + 37], 1 / smear);
                gl.bindFramebuffer(gl.FRAMEBUFFER, vm.fbo);
                gl.viewport(0, 0, w, h);
                gl.depthMask(true);
                gl.clearBufferfv(gl.COLOR, 0, fx.still);
                gl.clearBufferfv(gl.DEPTH, 0, fx.far);
                gl.enable(gl.DEPTH_TEST);
                gl.depthFunc(gl.LESS);
                gl.drawArrays(gl.TRIANGLES, 0, 3 * H[o + 36]);
                gl.disable(gl.DEPTH_TEST);
            }
            if (ao) {
                var A = P.ao;
                gl.useProgram(A.program);
                gl.uniform4f(A.u_aoView, H[o + 15], 0.9995002746582031, H[o + 57], H[o + 58]);
                gl.uniform4f(A.u_aoParams, H[o + 56], H[o + 55], w, h);
                fx.texture(0, t.depth);
                fx.pass(A, t.aoFbo[0], t.hw, t.hh);
                gl.useProgram(P.aoBlur.program);
                fx.texture(0, t.ao[0]);
                fx.pass(P.aoBlur, t.aoFbo[1], t.hw, t.hh);
            }
            if (glow) {
                var cutoff = H[o + 2], knee = Math.max(cutoff - 0.3, 0);
                gl.useProgram(P.bright.program);
                gl.uniform2f(P.bright.u_texel, 1 / w, 1 / h);
                gl.uniform3f(P.bright.u_bright, knee, 1 / Math.max(cutoff - knee + 0.05, 0.05), H[o + 3]);
                fx.texture(0, t.color);
                fx.pass(P.bright, t.eFbo[0], ew, eh);
                /* r_glowRadius is in 480-line pixels; the kernels run at 1/8 scale. */
                var narrow = fx.kernel(Math.min(H[o + 4] * lines / 8, 1.2), 1);
                var wide = fx.kernel(Math.min(H[o + 5] * lines / 8, 2.75), 3);
                var B = P.bloom;
                gl.useProgram(B.program);
                gl.uniform2f(B.u_texel, 1 / ew, 1 / eh);
                gl.uniform3f(B.u_narrow, narrow.offsets[0], narrow.weights[0], narrow.weights[1]);
                gl.uniform3f(B.u_wideOffset, wide.offsets[0], wide.offsets[1], wide.offsets[2]);
                gl.uniform4f(B.u_wideWeight, wide.weights[0], wide.weights[1], wide.weights[2], wide.weights[3]);
                gl.uniform2f(B.u_mix, glow0, glow1);
                fx.texture(0, t.e[0]);
                fx.pass(B, t.eFbo[1], ew, eh);
            }
            var vmStart = H[o + 7], vmEnd = H[o + 8], nearEnd = H[o + 16];
            var invRange = 1 / Math.max(vmEnd - vmStart, 0.5);
            /* The largest blur in quarter texels for a 480-line radius. Its
             * folded binomial deviates 1.414 steps, on top of the quarter image's
             * own 2 pixels; the four scene taps deviate 1.2 pixels. Each tier
             * starts where its image matches that fraction of the largest. */
            var near = Math.min(Math.max(H[o + 10] * lines * 0.6 / 4 / 1.4142, 0.25), 2.5);
            var largest = Math.sqrt(32 * near * near + 4);
            var smallTier = Math.min(1.2 / largest, 0.45);
            var medTier = Math.min(Math.max(2 / largest, smallTier + 0.05), 0.9);
            if (dof) {
                gl.useProgram(P.dof.program);
                gl.uniform2f(P.dof.u_texel, 1 / w, 1 / h);
                gl.uniform4f(P.dof.u_dof, 0.9995002746582031, H[o + 9], vmEnd, invRange);
                fx.texture(0, t.color);
                fx.texture(1, t.depth);
                fx.pass(P.dof, t.qFbo[0], qw, qh);
                /* Behind a menu, two wider passes take the quarter image back
                 * and forth to a deviation of 13 pixels at 480 lines; each
                 * pass's nearer taps stay within twice the blur already there.
                 * Around an open scope two wider passes reach about 9 pixels. */
                var b = lines / 1.5;
                var steps = blurBehind ? [Math.max(near, 0.8 * b), 1.6 * b, 3.2 * b] :
                    (scopeBlur ? [Math.max(near, 0.8 * b), 1.4 * b, 2.2 * b] : [near]);
                gl.useProgram(P.dofBlur.program);
                for (var p = 0; p < steps.length; ++p) {
                    gl.uniform2f(P.dofBlur.u_step, steps[p] / qw, steps[p] / qh);
                    fx.texture(0, t.q[p & 1]);
                    fx.pass(P.dofBlur, t.qFbo[1 - (p & 1)], qw, qh);
                }
            }
            var C = P.compose;
            gl.useProgram(C.program);
            gl.uniform4f(C.u_px, 1 / w, 1 / h, w / h, gray > 0 ? gray : 0);
            gl.uniform4f(C.u_glow, glow ? 1 : 0, 0, 0, 0);
            gl.uniform4f(C.u_dof, 0.9995002746582031, H[o + 9], vmEnd, invRange);
            gl.uniform4f(C.u_dof2, ads > 0.004 ? ads : 0, 0.07, 0.17, blurBehind ? backdrop : 0);
            gl.uniform4f(C.u_film, H[o + 11], H[o + 12], H[o + 13], H[o + 14]);
            /* CoD4 blurs the near world from one unit up to nearEnd. */
            gl.uniform4f(C.u_near, H[o + 15], nearEnd, 1 / Math.max(nearEnd - 1, 1), H[o + 17]);
            gl.uniform2f(C.u_tier, smallTier, medTier);
            /* The longest smear is 10% of the screen's height. */
            gl.uniform4f(C.u_motion, motion > 0 ? motion : 0, H[o + 31], H[o + 32], 0.10 * h);
            if (motion > 0) {
                gl.uniformMatrix3fv(C.u_motionRotation, false, H, o + 19, 9);
                gl.uniform3f(C.u_motionMove, H[o + 28], H[o + 29], H[o + 30]);
            }
            gl.uniform4f(C.u_vm, vm ? smear : 0, H[o + 40], H[o + 38], H[o + 41]);
            /* The glass's outline takes a pixel offset from its centre to the
             * unit circle: the inverse of its half-axes, per pixel. */
            var ax = H[o + 44], ay = H[o + 45], bx = H[o + 46], by = H[o + 47];
            var det = ax * by - bx * ay;
            if (lens && Math.abs(det) > 1e-6) {
                var radius = 0.5 * Math.min(Math.hypot(ax * w, ay * h), Math.hypot(bx * w, by * h));
                gl.uniform4f(C.u_lens, (H[o + 42] + 1) * 0.5 * w, (H[o + 43] + 1) * 0.5 * h, lensPresence, H[o + 49]);
                gl.uniform4f(C.u_lensInv, 2 * by / (det * w), -2 * bx / (det * h), -2 * ay / (det * w), 2 * ax / (det * h));
                /* A scope's glass, drawn with the rifle's projection, comes in
                 * with its drawn eyepiece. */
                gl.uniform4f(C.u_lensFx, lensBlur, H[o + 51], Math.min(1.5 / Math.max(radius, 1), 0.5),
                    H[o + 52] > 0 ? lensPresence : 0);
            } else {
                gl.uniform4f(C.u_lens, 0, 0, 0, 0);
            }
            gl.uniform4f(C.u_aoMix, ao ? 1 : 0, 0.5 / t.hw, 0.5 / t.hh, 0);
            gl.uniform4f(C.u_haze, H[o + 59], H[o + 60], H[o + 61], haze ? H[o + 62] : 0);
            if (haze) {
                gl.uniform3f(C.u_hazeUp, H[o + 63], H[o + 64], H[o + 65]);
                gl.uniform3f(C.u_hazeSun, H[o + 66], H[o + 67], H[o + 68]);
                gl.uniform3f(C.u_hazeSunColor, H[o + 69], H[o + 70], H[o + 71]);
                gl.uniform3f(C.u_hazeProj, H[o + 57], H[o + 58], H[o + 72]);
            }
            var sources = [t.color, t.e[1], t.depth, t.q[0], t.q[1], vm ? vm.smear : null, vm ? vm.mesh : null, ao ? t.ao[1] : null];
            for (var s = 0; s < sources.length; ++s) fx.texture(s, sources[s]);
            if (scaled) {
                fx.pass(C, t.outFbo, w, h);
                gl.useProgram(P.upscale.program);
                gl.uniform2f(P.upscale.u_size, w, h);
                fx.texture(0, t.out);
                fx.pass(P.upscale, null, fx.screenWidth, fx.screenHeight);
            } else {
                fx.pass(C, null, w, h);
            }
            for (var u = sources.length - 1; u >= 0; --u) fx.texture(u, null);
            gl.bindVertexArray(vao);
            gl.useProgram(program);
            gl.activeTexture(unit);
            gl.viewport(viewport[0], viewport[1], viewport[2], viewport[3]);
            for (var r = 0; r < caps.length; ++r)
                if (enabled[r]) gl.enable(caps[r]);
        };
    }
    if (fx.failed || width <= 0 || height <= 0) return 0;
    if (fx.width !== width || fx.height !== height || fx.requested !== samples) {
        var ok = fx.allocate(width, height, samples);
        if (!ok && samples > 1) ok = fx.allocate(width, height, 1);
        fx.requested = samples;
        if (!ok) {
            fx.failed = true;
            console.warn("[scene-fx] offscreen scene target unavailable; drawing directly to the canvas");
            return 0;
        }
    }
    fx.unscale();
    gl.bindFramebuffer(gl.FRAMEBUFFER, fx.t.ms || fx.t.scene);
    if (width !== screenWidth || height !== screenHeight) fx.scale(screenWidth, screenHeight);
    fx.drawing = true;
    if (fx.t.feather) fx.t.feather.ready = false;
    return 1;
});

EM_JS(void, WebSceneFx_Compose, (const float *params, const float *mesh), {
    var fx = Module.cod2SceneFx;
    if (fx && fx.t) fx.compose(params >>> 2, mesh >>> 2);
    else if (fx) { fx.unscale(); fx.drawing = false; }
});

/* Marks the scope's glass in the scene's stencil, bit 0x80, before the
 * rifle's first draw, and the outline about it when it has one, which opens
 * to the rifle's own eyepiece's bell. The D3D9 device never uses the
 * stencil; it sets both write masks again for every draw, and the depth mask
 * too. */
EM_JS(void, WebSceneFx_ScopeStencil, (const float *scope, const float *bell), {
    var fx = Module.cod2SceneFx;
    if (!fx || !fx.t || !fx.programs || !fx.programs.lens) return;
    var gl = fx.gl, o = scope >>> 2, b = bell >>> 2, L = fx.programs.lens;
    var program = gl.getParameter(gl.CURRENT_PROGRAM);
    var vao = gl.getParameter(gl.VERTEX_ARRAY_BINDING);
    var caps = [gl.SCISSOR_TEST, gl.BLEND, gl.DEPTH_TEST, gl.CULL_FACE, gl.POLYGON_OFFSET_FILL];
    var enabled = [];
    for (var c = 0; c < caps.length; ++c) {
        enabled.push(gl.isEnabled(caps[c]));
        gl.disable(caps[c]);
    }
    gl.enable(gl.STENCIL_TEST);
    gl.stencilMask(0x80);
    gl.stencilFunc(gl.ALWAYS, 0x80, 0x80);
    gl.stencilOp(gl.KEEP, gl.KEEP, gl.REPLACE);
    gl.colorMask(false, false, false, false);
    gl.depthMask(false);
    gl.useProgram(L.program);
    gl.uniform2f(L.u_center, HEAPF32[o], HEAPF32[o + 1]);
    gl.uniformMatrix2fv(L.u_axes, false, HEAPF32.subarray(o + 2, o + 6));
    gl.bindVertexArray(fx.vao);
    gl.drawArrays(gl.TRIANGLE_FAN, 0, 66);
    if (HEAPF32[b + 2] * HEAPF32[b + 5] - HEAPF32[b + 4] * HEAPF32[b + 3] !== 0) {
        gl.uniform2f(L.u_center, HEAPF32[b], HEAPF32[b + 1]);
        gl.uniformMatrix2fv(L.u_axes, false, HEAPF32.subarray(b + 2, b + 6));
        gl.drawArrays(gl.TRIANGLE_FAN, 0, 66);
    }
    gl.stencilMask(0);
    gl.stencilOp(gl.KEEP, gl.KEEP, gl.KEEP);
    gl.colorMask(true, true, true, true);
    gl.bindVertexArray(vao);
    gl.useProgram(program);
    for (var r = 0; r < caps.length; ++r)
        if (enabled[r]) gl.enable(caps[r]);
});

/* While the rifle draws, nothing of it lands on the glass. */
EM_JS(void, WebSceneFx_ScopeMask, (int on), {
    var fx = Module.cod2SceneFx;
    if (!fx) return;
    var gl = fx.gl;
    if (on) {
        gl.enable(gl.STENCIL_TEST);
        gl.stencilFunc(gl.NOTEQUAL, 0x80, 0x80);
        gl.stencilOp(gl.KEEP, gl.KEEP, gl.KEEP);
        gl.stencilMask(0);
    } else {
        gl.disable(gl.STENCIL_TEST);
    }
});

static float WebSceneFx_Clamp(float value, float lo, float hi)
{
    if (!isfinite(value) || value < lo)
        return lo;
    return value > hi ? hi : value;
}

static int WebSceneFx_Enabled(const dvar_t *dvar)
{
    return dvar && dvar->current.enabled;
}

static int WebSceneFx_GlowEnabled(void)
{
    return WebSceneFx_Enabled(r_glow) && !WebSceneFx_Enabled(r_fullbright);
}

static int WebSceneFx_AoEnabled(void)
{
    return WebSceneFx_Enabled(r_ssao) && !WebSceneFx_Enabled(r_fullbright);
}

static int WebSceneFx_HazeEnabled(void)
{
    return WebSceneFx_Enabled(r_atmosphere) && !WebSceneFx_Enabled(r_fullbright) && s_hazeDensity > 0.0f;
}

static int WebSceneFx_Samples(void)
{
    return r_aaSamples ? r_aaSamples->current.integer : 1;
}

/* The scene's size for a view filling a screen this big: r_sceneHeight lines
 * at the screen's aspect, or the screen's own when it is no taller. */
static void WebSceneFx_SceneSize(int width, int height, int *sceneWidth, int *sceneHeight)
{
    int lines = r_sceneHeight ? r_sceneHeight->current.integer : 0;

    *sceneWidth = width;
    *sceneHeight = height;
    if (lines > 0 && lines < height) {
        *sceneHeight = lines;
        *sceneWidth = (int)floorf((float)width * (float)lines / (float)height + 0.5f);
        if (*sceneWidth < 1)
            *sceneWidth = 1;
    }
}

void WebSceneFx_SetAds(float adsFrac, float viewmodelStart, float viewmodelEnd, float nearEnd)
{
    s_adsFrac = WebSceneFx_Clamp(adsFrac, 0.0f, 1.0f);
    s_vmEnd = WebSceneFx_Clamp(viewmodelEnd, 2.0f, 64.0f);
    s_vmStart = WebSceneFx_Clamp(viewmodelStart, 0.0f, s_vmEnd - 0.5f);
    s_nearEnd = WebSceneFx_Clamp(nearEnd, 1.0f, 1024.0f);
}

/* Aiming down the sights fades the motion blur out. */
void WebSceneFx_SetAim(float aimFrac)
{
    s_aimFrac = WebSceneFx_Clamp(aimFrac, 0.0f, 1.0f);
}

void WebSceneFx_SetBackdrop(float backdropFrac)
{
    s_backdropFrac = WebSceneFx_Clamp(backdropFrac, 0.0f, 1.0f);
}

void WebSceneFx_SetBackdropGray(float gray)
{
    s_backdropGray = WebSceneFx_Clamp(gray, 0.0f, 1.0f);
}

/* The WWII countdown before a match takes the view's color away. */
void WebSceneFx_SetGray(float grayFrac)
{
    s_grayFrac = WebSceneFx_Clamp(grayFrac, 0.0f, 1.0f);
}

/* The scene's exponential fog, its color and density per unit (0 without
 * fog), and the direction toward its sun and the sun's color, for the haze.
 * A broken value turns the haze off, or its glow. */
void WebSceneFx_SetAtmosphere(const float *fogColor, float fogDensity, const float *sunDirection, const float *sunColor)
{
    float length = 0.0f;
    int i, valid = fogColor != NULL;

    for (i = 0; valid && i < 3; ++i)
        valid = isfinite(fogColor[i]);
    s_hazeDensity = valid ? WebSceneFx_Clamp(fogDensity, 0.0f, 0.01f) : 0.0f;
    for (i = 0; i < 3; ++i)
        s_hazeColor[i] = valid ? WebSceneFx_Clamp(fogColor[i], 0.0f, 1.0f) : 0.0f;
    if (sunDirection && sunColor)
        length = sqrtf(sunDirection[0] * sunDirection[0] + sunDirection[1] * sunDirection[1] +
                       sunDirection[2] * sunDirection[2]);
    valid = isfinite(length) && length > 0.0f;
    for (i = 0; valid && i < 3; ++i)
        valid = isfinite(sunColor[i]);
    for (i = 0; i < 3; ++i) {
        s_hazeSunWorld[i] = valid ? sunDirection[i] / length : 0.0f;
        s_hazeSunColor[i] = valid ? WebSceneFx_Clamp(sunColor[i], 0.0f, 2.0f) : 0.0f;
    }
}

/* The world's up and the sun's direction in the composed view's space: a view
 * turns a world direction w into R w, R[j][i] = view[i * 4 + j]. */
static void WebSceneFx_HazeView(void)
{
    const float *view = s_cameraView;
    int j;

    for (j = 0; j < 3; ++j) {
        s_hazeUp[j] = view[8 + j];
        s_hazeSun[j] = view[j] * s_hazeSunWorld[0] + view[4 + j] * s_hazeSunWorld[1] + view[8 + j] * s_hazeSunWorld[2];
    }
}

/* The cgame's scope window for the scene about to render, as the floats of a
 * CgWwiiScopeLens; NULL closes it. A window without a usable outline, or a
 * rifle projection with a non-positive scale, is dropped; the rifle's near
 * clip is kept within the depth hack's range. Only a scope's glass, drawn
 * with the rifle's projection, keeps the outline about it, when that has
 * area. */
void WebSceneFx_SetScope(const float *scope)
{
    float *axes = s_scope + (SCENE_FX_SCOPE_AXES - SCENE_FX_SCOPE);
    float *projection = s_scope + (SCENE_FX_SCOPE_PROJECTION - SCENE_FX_SCOPE);
    float *nearClip = s_scope + (SCENE_FX_SCOPE_NEAR_CLIP - SCENE_FX_SCOPE);
    const float *bellAxes = s_scopeBell + 2;
    int i;

    memset(s_scope, 0, sizeof(s_scope));
    memset(s_scopeBell, 0, sizeof(s_scopeBell));
    if (!scope)
        return;
    for (i = 0; i < SCENE_FX_SCOPE_FLOATS; ++i)
        s_scope[i] = isfinite(scope[i]) ? scope[i] : 0.0f;
    for (i = SCENE_FX_SCOPE_PRESENCE; i <= SCENE_FX_SCOPE_DARK; ++i)
        s_scope[i - SCENE_FX_SCOPE] = WebSceneFx_Clamp(s_scope[i - SCENE_FX_SCOPE], 0.0f, 1.0f);
    if (!(fabsf(axes[0] * axes[3] - axes[2] * axes[1]) > 1e-6f))
        s_scope[SCENE_FX_SCOPE_PRESENCE - SCENE_FX_SCOPE] = 0.0f;
    if (!(projection[0] > 0.0f) || !(projection[1] > 0.0f))
        projection[0] = projection[1] = 0.0f;
    *nearClip = WebSceneFx_Clamp(*nearClip, 0.0f, 16.0f);
    for (i = 0; i < SCENE_FX_SCOPE_BELL_FLOATS; ++i)
        s_scopeBell[i] = isfinite(scope[SCENE_FX_SCOPE_FLOATS + i]) ? scope[SCENE_FX_SCOPE_FLOATS + i] : 0.0f;
    if (!(projection[0] > 0.0f) || !(fabsf(bellAxes[0] * bellAxes[3] - bellAxes[2] * bellAxes[1]) > 1e-6f))
        memset(s_scopeBell, 0, sizeof(s_scopeBell));
}

int WebSceneFx_ScopeAvailable(void)
{
    return !s_scopeUnavailable;
}

static int WebSceneFx_ScopeRequested(void)
{
    return s_scope[SCENE_FX_SCOPE_PRESENCE - SCENE_FX_SCOPE] > 0.0f ||
           s_scope[SCENE_FX_SCOPE_PROJECTION - SCENE_FX_SCOPE] > 0.0f;
}

/* The rifle's projection scales while the composed view draws it, or NULL
 * for the view's own. */
const float *WebSceneFx_ViewmodelProjection(void)
{
    const float *projection = s_scope + (SCENE_FX_SCOPE_PROJECTION - SCENE_FX_SCOPE);

    return s_active && projection[0] > 0.0f ? projection : NULL;
}

static float WebSceneFx_ScopeNearClip(float nearClip)
{
    float scope = s_scope[SCENE_FX_SCOPE_NEAR_CLIP - SCENE_FX_SCOPE];

    return scope > nearClip ? scope : nearClip;
}

/* The rifle's near clip while the composed view draws it: the depth hack's
 * nearClip, or the window's when that is further. */
float WebSceneFx_ViewmodelNearClip(float nearClip)
{
    return s_active ? WebSceneFx_ScopeNearClip(nearClip) : nearClip;
}

/* A Reflex or Lens Sight's window: a glass the rifle is drawn around with
 * the view's own projection. */
static int WebSceneFx_SightWindow(void)
{
    return s_scope[SCENE_FX_SCOPE_PRESENCE - SCENE_FX_SCOPE] > 0.0f &&
           !(s_scope[SCENE_FX_SCOPE_PROJECTION - SCENE_FX_SCOPE] > 0.0f);
}

/* Before each material's draws. A sight's window is the disc of its ring:
 * the attachments' own surfaces, this port's mtl_browser_att_ materials,
 * stay drawn over it, so the ring shows whole, the iron sights before it
 * cut away at its edge. */
void WebSceneFx_ViewmodelMaterial(const char *name)
{
    int exempt = 0;

    if (s_active && name && WebSceneFx_SightWindow())
        exempt = !strncmp(name, "mtl_browser_att_", 16);
    if (exempt == s_scopeExempt)
        return;
    s_scopeExempt = exempt;
    if (s_scopeMasking)
        WebSceneFx_ScopeMask(!exempt);
}

/* Around the rifle's draws in the composed view: the stencil keeps every
 * part of it off the glass, so the world shows through. */
void WebSceneFx_BeginViewmodel(void)
{
    if (!s_active || !(s_scope[SCENE_FX_SCOPE_PRESENCE - SCENE_FX_SCOPE] > 0.0f))
        return;
    if (!s_scopeStencil) {
        WebSceneFx_ScopeStencil(s_scope, s_scopeBell);
        s_scopeStencil = 1;
    }
    WebSceneFx_ScopeMask(!s_scopeExempt);
    s_scopeMasking = 1;
}

void WebSceneFx_EndViewmodel(void)
{
    if (!s_scopeMasking)
        return;
    WebSceneFx_ScopeMask(0);
    s_scopeMasking = 0;
}

/* The D3D view and projection matrices of the view about to begin. */
void WebSceneFx_SetCamera(const float *viewMatrix, const float *projectionMatrix)
{
    if (!viewMatrix || !projectionMatrix)
        return;
    memcpy(s_cameraView, viewMatrix, sizeof(s_cameraView));
    memcpy(s_cameraLens, projectionMatrix, sizeof(s_cameraLens));
    s_cameraProjection[0] = projectionMatrix[0];
    s_cameraProjection[1] = projectionMatrix[5];
    s_cameraPending = 1;
}

static int WebSceneFx_CameraValid(void)
{
    int i;

    for (i = 0; i < 16; ++i)
        if (!isfinite(s_cameraView[i]))
            return 0;
    return isfinite(s_cameraProjection[0]) && isfinite(s_cameraProjection[1]) &&
           s_cameraProjection[0] > 0.0f && s_cameraProjection[1] > 0.0f;
}

/* A view takes a world point p to R p + t, with R[j][i] = view[i * 4 + j] and
 * t[j] = view[12 + j]. From this view's space into the last one's, a point
 * turns by R_last R^T and then moves by t_last - R_last R^T t. */
static void WebSceneFx_TrackCamera(void)
{
    const float *view = s_cameraView, *last = s_lastView;
    float rotation[9], move[3];
    float turn, moved, trace;
    double now, gap;
    int valid, i, j, k;

    s_motionScale = 0.0f;
    s_viewContinuous = 0;
    if (!s_cameraPending) {
        s_lastViewValid = 0;
        return;
    }
    s_cameraPending = 0;
    valid = WebSceneFx_CameraValid();
    now = emscripten_get_now();
    gap = now - s_lastViewTime;
    if (valid && s_lastViewValid && gap >= 1.0 && gap <= SCENE_FX_MOTION_MAX_GAP) {
        for (j = 0; j < 3; ++j)
            for (k = 0; k < 3; ++k) {
                float sum = 0.0f;
                for (i = 0; i < 3; ++i)
                    sum += last[i * 4 + j] * view[i * 4 + k];
                rotation[k * 3 + j] = sum;
            }
        for (j = 0; j < 3; ++j) {
            float sum = last[12 + j];
            for (k = 0; k < 3; ++k)
                sum -= rotation[k * 3 + j] * view[12 + k];
            move[j] = sum;
        }
        trace = rotation[0] + rotation[4] + rotation[8];
        turn = 0.5f * sqrtf((rotation[5] - rotation[7]) * (rotation[5] - rotation[7]) +
                            (rotation[6] - rotation[2]) * (rotation[6] - rotation[2]) +
                            (rotation[1] - rotation[3]) * (rotation[1] - rotation[3]));
        moved = sqrtf(move[0] * move[0] + move[1] * move[1] + move[2] * move[2]);
        if (trace > 1.0f + 2.0f * cosf(SCENE_FX_MOTION_MAX_TURN) && moved < SCENE_FX_MOTION_MAX_MOVE) {
            s_viewContinuous = 1;
            s_viewGap = gap;
            if (WebSceneFx_Enabled(r_motionBlur) && s_aimFrac < 1.0f &&
                (turn > SCENE_FX_MOTION_MIN_TURN || moved > SCENE_FX_MOTION_MIN_MOVE)) {
                s_motionScale = (float)(SCENE_FX_MOTION_EXPOSURE / gap) * (1.0f - s_aimFrac);
                memcpy(s_motionRotation, rotation, sizeof(s_motionRotation));
                memcpy(s_motionMove, move, sizeof(s_motionMove));
                s_motionProjection[0] = s_cameraProjection[0];
                s_motionProjection[1] = s_cameraProjection[1];
            }
        }
    }
    memcpy(s_lastView, view, sizeof(s_lastView));
    s_lastViewTime = now;
    s_lastViewValid = valid;
}

/* The renderer skins the viewmodel again every frame. */
void WebSceneFx_BeginFrame(void)
{
    s_vmSurfaceCount = 0;
    s_vmVertexCount = 0;
    s_vmTriangleCount = 0;
    s_vmOverflow = 0;
}

/* A viewmodel surface the renderer has just skinned: vertexCount positions
 * stride bytes apart, taken into the world by transform (three axis rows, then
 * the origin), drawn as triangleCount triangles of indices. key names the
 * surface from one frame to the next. */
void WebSceneFx_AddViewmodelSurface(const void *key, const float *positions, int stride, int vertexCount,
                                    const unsigned short *indices, int triangleCount, const float *transform)
{
    WebSceneFxVmSurface *surface;
    float *world;
    int i, j;

    if (!key || !positions || !indices || !transform || vertexCount <= 0 || triangleCount <= 0 ||
        stride < 3 * (int)sizeof(float) || stride % (int)sizeof(float))
        return;
    if (s_vmSurfaceCount >= SCENE_FX_VM_MAX_SURFACES || vertexCount > SCENE_FX_VM_MAX_VERTICES - s_vmVertexCount ||
        triangleCount > SCENE_FX_VM_MAX_TRIANGLES - s_vmTriangleCount) {
        if (!s_vmOverflowWarned)
            printf("[scene-fx] the viewmodel has over %d surfaces, %d vertices or %d triangles; it does not blur\n",
                   SCENE_FX_VM_MAX_SURFACES, SCENE_FX_VM_MAX_VERTICES, SCENE_FX_VM_MAX_TRIANGLES);
        s_vmOverflowWarned = 1;
        s_vmOverflow = 1;
        return;
    }
    surface = &s_vmSurfaces[s_vmSurfaceCount++];
    surface->key = key;
    surface->indices = indices;
    surface->first = s_vmVertexCount;
    surface->count = vertexCount;
    surface->triangles = triangleCount;
    world = s_vmWorld + s_vmVertexCount * 3;
    for (i = 0; i < vertexCount; ++i) {
        const float *p = (const float *)((const char *)positions + (size_t)i * (size_t)stride);
        for (j = 0; j < 3; ++j)
            world[i * 3 + j] = p[0] * transform[j] + p[1] * transform[3 + j] + p[2] * transform[6 + j] + transform[9 + j];
    }
    s_vmVertexCount += vertexCount;
    s_vmTriangleCount += triangleCount;
}

/* The last frame's copy of this surface: the same key, met as many times
 * before it, with as many vertices. */
static const WebSceneFxVmSurface *WebSceneFx_LastViewmodelSurface(int index)
{
    const WebSceneFxVmSurface *surface = &s_vmSurfaces[index];
    int occurrence = 0, i;

    for (i = 0; i < index; ++i)
        occurrence += s_vmSurfaces[i].key == surface->key;
    for (i = 0; i < s_vmLastSurfaceCount; ++i) {
        if (s_vmLastSurfaces[i].key != surface->key || occurrence--)
            continue;
        return s_vmLastSurfaces[i].count == surface->count ? &s_vmLastSurfaces[i] : NULL;
    }
    return NULL;
}

/* Raise the tiles under a triangle spanning lo to hi in normalized device
 * coordinates to its fastest vertex's smear. A triangle off the screen
 * raises none. */
static int WebSceneFx_MarkViewmodelTiles(const float *lo, const float *hi, const float *smear, int width, int height,
                                         int tile, int tilesX, int tilesY)
{
    float x0 = (lo[0] + 1.0f) * 0.5f * (float)width, x1 = (hi[0] + 1.0f) * 0.5f * (float)width;
    float y0 = (lo[1] + 1.0f) * 0.5f * (float)height, y1 = (hi[1] + 1.0f) * 0.5f * (float)height;
    float speed = smear[0] * smear[0] + smear[1] * smear[1];
    int tx, ty, tx0, tx1, ty0, ty1;

    if (x1 < 0.0f || y1 < 0.0f || x0 >= (float)width || y0 >= (float)height)
        return 0;
    tx0 = x0 > 0.0f ? (int)(x0 / (float)tile) : 0;
    ty0 = y0 > 0.0f ? (int)(y0 / (float)tile) : 0;
    tx1 = x1 < (float)width ? (int)(x1 / (float)tile) : tilesX - 1;
    ty1 = y1 < (float)height ? (int)(y1 / (float)tile) : tilesY - 1;
    for (ty = ty0; ty <= ty1; ++ty)
        for (tx = tx0; tx <= tx1; ++tx) {
            float *max = s_vmTiles + (ty * tilesX + tx) * 2;
            if (speed > max[0] * max[0] + max[1] * max[1]) {
                max[0] = smear[0];
                max[1] = smear[1];
            }
        }
    return 1;
}

/* This view's camera, with the depth hack's near clip, puts each viewmodel
 * vertex on the screen, and the vertex smears along the way it moved there
 * since the last frame: the viewmodel's own motion, which the camera's does
 * not change. Its half smear spans 5 ms of that motion either side, longer
 * past the knee. The mesh holds each vertex's GL clip position and smear, the
 * triangles with every corner in front of the eye, and for each tile of the
 * screen the largest smear within one tile of it. A tile is as wide as the
 * longest half smear, so that covers every smear that reaches the tile. */
static void WebSceneFx_TrackViewmodel(int width, int height, float nearClip)
{
    const float *view = s_cameraView;
    float *screen = s_vmScreens[s_vmLastScreen ^ 1];
    const float *last = s_vmScreens[s_vmLastScreen];
    float lens[16], m[16], *index;
    float knee, cap, scale, motion = 0.0f;
    int blur, tile, tilesX, tilesY, vertexRows, tileRow, s, t, i, j, k, n = 0;

    s_vmMotion = 0.0f;
    s_vmTriangles = s_vmIndexRow = s_vmTileRow = s_vmRows = s_vmTile = s_vmTilesX = 0;
    if (!s_lastViewValid || s_vmOverflow) {
        s_vmLastSurfaceCount = 0;
        return;
    }
    memcpy(lens, s_cameraLens, sizeof(lens));
    /* A raised WWII scope draws the rifle with its own projection scales and
     * near clip, so its smear has to be measured with them too. */
    if (s_scope[SCENE_FX_SCOPE_PROJECTION - SCENE_FX_SCOPE] > 0.0f) {
        lens[0] = s_scope[SCENE_FX_SCOPE_PROJECTION - SCENE_FX_SCOPE];
        lens[5] = s_scope[SCENE_FX_SCOPE_PROJECTION + 1 - SCENE_FX_SCOPE];
        nearClip = WebSceneFx_ScopeNearClip(nearClip);
    }
    lens[14] = -nearClip;
    for (i = 0; i < 4; ++i)
        for (j = 0; j < 4; ++j) {
            float sum = 0.0f;
            for (k = 0; k < 4; ++k)
                sum += view[i * 4 + k] * lens[k * 4 + j];
            m[i * 4 + j] = sum;
        }
    knee = SCENE_FX_VM_KNEE * (float)height;
    cap = SCENE_FX_VM_CAP * (float)height;
    tile = (int)ceilf(cap);
    tilesX = (width + tile - 1) / tile;
    tilesY = (height + tile - 1) / tile;
    blur = tilesX * tilesY <= SCENE_FX_VM_MAX_TILES && s_viewContinuous && WebSceneFx_Enabled(r_motionBlur);
    scale = blur ? (float)(SCENE_FX_VM_EXPOSURE / s_viewGap) * 0.25f : 0.0f;
    for (s = 0; s < s_vmSurfaceCount; ++s) {
        const WebSceneFxVmSurface *surface = &s_vmSurfaces[s];
        const WebSceneFxVmSurface *before = blur ? WebSceneFx_LastViewmodelSurface(s) : NULL;
        float jump = 0.0f;

        for (i = surface->first; i < surface->first + surface->count; ++i) {
            const float *p = s_vmWorld + i * 3;
            float clip[4], *vertex = s_vmMesh + i * 8;
            for (j = 0; j < 4; ++j)
                clip[j] = p[0] * m[j] + p[1] * m[4 + j] + p[2] * m[8 + j] + m[12 + j];
            screen[i * 2] = screen[i * 2 + 1] = NAN;
            if (clip[3] > 0.001f && isfinite(clip[0]) && isfinite(clip[1]) && isfinite(clip[2]) && isfinite(clip[3])) {
                screen[i * 2] = clip[0] / clip[3];
                screen[i * 2 + 1] = clip[1] / clip[3];
            }
            vertex[0] = clip[0];
            vertex[1] = clip[1];
            vertex[2] = 2.0f * clip[2] - clip[3];
            vertex[3] = clip[3];
            if (before) {
                const float *was = last + (before->first + i - surface->first) * 2;
                if (isfinite(screen[i * 2]) && isfinite(was[0]))
                    jump += fabsf(screen[i * 2] - was[0]) + fabsf(screen[i * 2 + 1] - was[1]);
            }
        }
        if (jump > SCENE_FX_VM_MAX_JUMP * (float)surface->count)
            before = NULL;
        for (i = surface->first; i < surface->first + surface->count; ++i) {
            float *smear = s_vmMesh + i * 8 + 4;
            smear[0] = smear[1] = smear[2] = smear[3] = 0.0f;
            if (before) {
                const float *was = last + (before->first + i - surface->first) * 2;
                if (isfinite(screen[i * 2]) && isfinite(was[0])) {
                    float length;
                    smear[0] = (screen[i * 2] - was[0]) * (float)width * scale;
                    smear[1] = (screen[i * 2 + 1] - was[1]) * (float)height * scale;
                    length = sqrtf(smear[0] * smear[0] + smear[1] * smear[1]);
                    if (length > knee) {
                        float boosted = fminf(knee + SCENE_FX_VM_GAIN * (length - knee), cap);
                        smear[0] *= boosted / length;
                        smear[1] *= boosted / length;
                    }
                }
            }
        }
    }
    if (blur) {
        vertexRows = (s_vmVertexCount * 2 + SCENE_FX_VM_MESH_WIDTH - 1) / SCENE_FX_VM_MESH_WIDTH;
        index = s_vmMesh + vertexRows * SCENE_FX_VM_MESH_WIDTH * 4;
        memset(s_vmTiles, 0, sizeof(float) * 2 * (size_t)(tilesX * tilesY));
        for (s = 0; s < s_vmSurfaceCount; ++s) {
            const WebSceneFxVmSurface *surface = &s_vmSurfaces[s];
            for (t = 0; t < surface->triangles; ++t) {
                float lo[2] = {INFINITY, INFINITY}, hi[2] = {-INFINITY, -INFINITY};
                float fastest = 0.25f;
                int corners[3], quickest = -1, c;
                for (c = 0; c < 3; ++c) {
                    int local = surface->indices[t * 3 + c];
                    const float *at, *smear;
                    if (local >= surface->count)
                        break;
                    corners[c] = surface->first + local;
                    at = screen + corners[c] * 2;
                    if (!isfinite(at[0]))
                        break;
                    smear = s_vmMesh + corners[c] * 8 + 4;
                    if (smear[0] * smear[0] + smear[1] * smear[1] >= fastest) {
                        fastest = smear[0] * smear[0] + smear[1] * smear[1];
                        quickest = corners[c];
                    }
                    for (j = 0; j < 2; ++j) {
                        lo[j] = fminf(lo[j], at[j]);
                        hi[j] = fmaxf(hi[j], at[j]);
                    }
                }
                if (c < 3)
                    continue;
                index[n++] = (float)corners[0];
                index[n++] = (float)corners[1];
                index[n++] = (float)corners[2];
                if (quickest >= 0 &&
                    WebSceneFx_MarkViewmodelTiles(lo, hi, s_vmMesh + quickest * 8 + 4, width, height, tile, tilesX, tilesY))
                    motion = fmaxf(motion, sqrtf(fastest));
            }
        }
        tileRow = vertexRows + (n + SCENE_FX_VM_MESH_WIDTH * 4 - 1) / (SCENE_FX_VM_MESH_WIDTH * 4);
        for (j = 0; j < tilesY && motion > 0.0f; ++j)
            for (i = 0; i < tilesX; ++i) {
                float *out = s_vmMesh + (tileRow * SCENE_FX_VM_MESH_WIDTH + j * tilesX + i) * 4;
                float best = -1.0f;
                int x, y;
                for (y = j > 0 ? j - 1 : 0; y <= j + 1 && y < tilesY; ++y)
                    for (x = i > 0 ? i - 1 : 0; x <= i + 1 && x < tilesX; ++x) {
                        const float *max = s_vmTiles + (y * tilesX + x) * 2;
                        if (max[0] * max[0] + max[1] * max[1] > best) {
                            best = max[0] * max[0] + max[1] * max[1];
                            out[0] = max[0];
                            out[1] = max[1];
                        }
                    }
                out[2] = out[3] = 0.0f;
            }
        if (motion > 0.0f) {
            s_vmMotion = motion;
            s_vmTriangles = n / 3;
            s_vmIndexRow = vertexRows;
            s_vmTileRow = tileRow;
            s_vmRows = tileRow + (tilesX * tilesY + SCENE_FX_VM_MESH_WIDTH - 1) / SCENE_FX_VM_MESH_WIDTH;
            s_vmTile = tile;
            s_vmTilesX = tilesX;
        }
    }
    s_vmLastScreen ^= 1;
    memcpy(s_vmLastSurfaces, s_vmSurfaces, sizeof(s_vmSurfaces[0]) * (size_t)s_vmSurfaceCount);
    s_vmLastSurfaceCount = s_vmSurfaceCount;
}

static void WebSceneFx_FillParams(float *params)
{
    int glow = WebSceneFx_GlowEnabled();
    int film = WebSceneFx_Enabled(r_filmTweakEnable);
    int dof = WebSceneFx_Enabled(r_dof_enable);

    params[SCENE_FX_GLOW0] = glow && r_glowBloomIntensity[0] ?
        WebSceneFx_Clamp(r_glowBloomIntensity[0]->current.value, 0.0f, 8.0f) * 0.42f : 0.0f;
    params[SCENE_FX_GLOW1] = glow && r_glowBloomIntensity[1] ?
        WebSceneFx_Clamp(r_glowBloomIntensity[1]->current.value, 0.0f, 8.0f) * 0.30f : 0.0f;
    params[SCENE_FX_CUTOFF] = r_glowBloomCutoff ?
        WebSceneFx_Clamp(r_glowBloomCutoff->current.value, 0.0f, 0.99f) : 0.9f;
    params[SCENE_FX_DESATURATION] = r_glowBloomDesaturation ?
        WebSceneFx_Clamp(r_glowBloomDesaturation->current.value, 0.0f, 1.0f) : 0.75f;
    params[SCENE_FX_RADIUS0] = r_glowRadius[0] ?
        WebSceneFx_Clamp(r_glowRadius[0]->current.value, 0.0f, 32.0f) : 5.0f;
    params[SCENE_FX_RADIUS1] = r_glowRadius[1] ?
        WebSceneFx_Clamp(r_glowRadius[1]->current.value, 0.0f, 32.0f) : 12.0f;
    params[SCENE_FX_ADS] = dof ? s_adsFrac : 0.0f;
    params[SCENE_FX_VM_START] = s_vmStart;
    params[SCENE_FX_VM_END] = s_vmEnd;
    params[SCENE_FX_NEAR_CLIP] = WebSceneFx_ScopeNearClip(s_nearClip);
    params[SCENE_FX_NEAR_BLUR] = 3.0f;
    params[SCENE_FX_SHARPEN] = film ? 0.147f : 0.0f;
    params[SCENE_FX_SATURATION] = 0.96f;
    params[SCENE_FX_CONTRAST] = film ? 0.35f : 0.0f;
    params[SCENE_FX_FILM] = film ? 1.0f : 0.0f;
    params[SCENE_FX_WORLD_NEAR_CLIP] = s_worldNearClip;
    params[SCENE_FX_NEAR_END] = s_nearEnd;
    params[SCENE_FX_WORLD_BLUR] = 1.0f;
    params[SCENE_FX_MOTION] = s_motionScale;
    memcpy(params + SCENE_FX_MOTION_ROTATION, s_motionRotation, sizeof(s_motionRotation));
    memcpy(params + SCENE_FX_MOTION_MOVE, s_motionMove, sizeof(s_motionMove));
    params[SCENE_FX_MOTION_PROJECTION] = s_motionProjection[0];
    params[SCENE_FX_MOTION_PROJECTION + 1] = s_motionProjection[1];
    params[SCENE_FX_BACKDROP] = s_backdropFrac;
    params[SCENE_FX_BACKDROP_GRAY] = s_backdropGray;
    /* Gray as a menu's backdrop is, and with the captures' lifted highlights,
     * which the film grade's contrast curve gives at 1.2. */
    if (s_grayFrac > params[SCENE_FX_BACKDROP_GRAY])
        params[SCENE_FX_BACKDROP_GRAY] = s_grayFrac;
    if (film)
        params[SCENE_FX_CONTRAST] += 0.85f * s_grayFrac;
    params[SCENE_FX_VM_MOTION] = s_vmMotion;
    params[SCENE_FX_VM_TRIANGLES] = (float)s_vmTriangles;
    params[SCENE_FX_VM_INDEX_ROW] = (float)s_vmIndexRow;
    params[SCENE_FX_VM_TILE_ROW] = (float)s_vmTileRow;
    params[SCENE_FX_VM_ROWS] = (float)s_vmRows;
    params[SCENE_FX_VM_TILE] = (float)s_vmTile;
    params[SCENE_FX_VM_TILES_X] = (float)s_vmTilesX;
    memcpy(params + SCENE_FX_SCOPE, s_scope, sizeof(s_scope));
    params[SCENE_FX_AO] = WebSceneFx_AoEnabled() && s_viewProjection[0] > 0.0f ? SCENE_FX_AO_STRENGTH : 0.0f;
    params[SCENE_FX_AO_RADIUS] = SCENE_FX_AO_WORLD_RADIUS;
    params[SCENE_FX_AO_PROJECTION] = s_viewProjection[0];
    params[SCENE_FX_AO_PROJECTION + 1] = s_viewProjection[1];
    memcpy(params + SCENE_FX_HAZE, s_hazeColor, sizeof(s_hazeColor));
    params[SCENE_FX_HAZE_DENSITY] = WebSceneFx_HazeEnabled() && s_viewProjection[0] > 0.0f ?
        fminf(s_hazeDensity * SCENE_FX_HAZE_SHARE, SCENE_FX_HAZE_MAX_DENSITY) : 0.0f;
    memcpy(params + SCENE_FX_HAZE_UP, s_hazeUp, sizeof(s_hazeUp));
    memcpy(params + SCENE_FX_HAZE_SUN, s_hazeSun, sizeof(s_hazeSun));
    params[SCENE_FX_HAZE_SUN_COLOR] = s_hazeSunColor[0] * SCENE_FX_HAZE_SUN_GLOW;
    params[SCENE_FX_HAZE_SUN_COLOR + 1] = s_hazeSunColor[1] * SCENE_FX_HAZE_SUN_GLOW;
    params[SCENE_FX_HAZE_SUN_COLOR + 2] = s_hazeSunColor[2] * SCENE_FX_HAZE_SUN_GLOW;
    params[SCENE_FX_HAZE_FALLOFF] = 1.0f / SCENE_FX_HAZE_SCALE_HEIGHT;
}

/* The mip bias the 3D draws of the composed view give their color maps. */
float WebSceneFx_TextureLodBias(void)
{
    return s_active ? s_lodBias : 0.0f;
}

/* The composed view's near clip times the infinite projection's depth scale,
 * which takes its depth back to distance; 0 outside it. */
float WebSceneFx_WorldNearClip(void)
{
    return s_active ? s_worldNearClip : 0.0f;
}

void WebSceneFx_EndView(void)
{
    float params[SCENE_FX_PARAM_COUNT];

    WebSceneFx_EndViewmodel();
    if (!s_active)
        return;
    s_active = 0;
    WebSceneFx_FillParams(params);
    WebSceneFx_Compose(params, s_vmMesh);
}

void WebSceneFx_BeginView(int x, int y, int width, int height, int screenWidth, int screenHeight,
                          float depthHackNearClip, float worldNearClip)
{
    int samples;
    int sceneWidth;
    int sceneHeight;
    int scaled;
    float nearClip;

    WebSceneFx_EndView();
    if (s_frameUsed || x || y || width != screenWidth || height != screenHeight || width <= 0 || height <= 0)
        return;
    s_frameUsed = 1;
    s_scopeStencil = 0;
    WebSunShadows_Render();
    WebSceneFx_SceneSize(width, height, &sceneWidth, &sceneHeight);
    nearClip = isfinite(depthHackNearClip) && depthHackNearClip > 0.0f ? depthHackNearClip : 0.1f;
    s_viewProjection[0] = s_viewProjection[1] = 0.0f;
    if (s_cameraPending && WebSceneFx_CameraValid()) {
        s_viewProjection[0] = s_cameraProjection[0];
        s_viewProjection[1] = s_cameraProjection[1];
        WebSceneFx_HazeView();
    }
    WebSceneFx_TrackCamera();
    WebSceneFx_TrackViewmodel(sceneWidth, sceneHeight, nearClip);
    /* r_fullbright clears the frame buffer before it begins the view. */
    if (WebSceneFx_Enabled(r_fullbright)) {
        s_scopeUnavailable = 1;
        return;
    }
    samples = WebSceneFx_Samples();
    scaled = sceneWidth != width || sceneHeight != height;
    if (!scaled && s_grayFrac <= 0.0f && s_vmMotion <= 0.0f && !WebSceneFx_GlowEnabled() && !WebSceneFx_Enabled(r_filmTweakEnable) && samples <= 1 &&
        !(WebSceneFx_Enabled(r_dof_enable) && s_adsFrac > 0.0f) && s_motionScale <= 0.0f && s_backdropFrac <= 0.0f &&
        s_backdropGray <= 0.0f && !WebSceneFx_ScopeRequested() && !WebSceneFx_AoEnabled() && !WebSceneFx_HazeEnabled())
        return;
    s_nearClip = nearClip;
    s_worldNearClip = isfinite(worldNearClip) && worldNearClip > 0.0f ? worldNearClip : SCENE_FX_DEFAULT_WORLD_NEAR_CLIP;
    s_active = WebSceneFx_Bind(sceneWidth, sceneHeight, samples, width, height);
    s_scopeUnavailable = !s_active;
    /* Sampling a mip finer by the scale's octaves gives the color maps the
     * detail of the canvas the scene is scaled up to, as upscalers do. */
    s_lodBias = s_active && scaled ? WebSceneFx_Clamp(log2f((float)sceneHeight / (float)height), -1.0f, 0.0f) : 0.0f;
}

void WebSceneFx_EndFrame(void)
{
    WebSceneFx_EndView();
    s_frameUsed = 0;
}
