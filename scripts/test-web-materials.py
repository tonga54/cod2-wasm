#!/usr/bin/env python3
"""Exercise the production material map selection and sun handoff."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = root / 'downstream/wasm/web_materials.c'

# The device selects maps only for draws that already take the point-light
# shaders, honors the original r_normalMap/r_specularMap overrides and marks
# lightmapped draws by the lightmap it bound itself.
device = (root / 'src/Mac/DirectX_9/CDirect3DDevice.c').read_text()
# Lightmapped layers blended over the world (terrain, decals) take the light,
# flashes and shadows of the surface under them, but only with the lightmap
# the device bound itself; additive and other blends stay on the DX7 path.
assert ('int lightmappedBlend = !is2D && dev->alphaBlendEnable && stage0Target == 0x0DE1 &&\n'
        '                usesLightmap && stage1Bound == 1;') in device
assert ('int worldShadow = (!is2D && dev->zWriteEnable && !dev->alphaBlendEnable &&\n'
        '                stage0Target == 0x0DE1 && usesLightmap) || lightmappedBlend;') in device
assert ('int materialLighting = (pointLight || lightmappedBlend) &&\n'
        '                r_materialLighting && r_materialLighting->current.enabled;') in device
assert 'int lmapAllowed = !dev->alphaBlendEnable || dev->destBlend == 6;' in device
assert device.index('stage1Bound = 1;') > device.index('int lmapAllowed = !dev->alphaBlendEnable || dev->destBlend == 6;')
assert ('!r_normalMap || r_normalMap->current.integer == 0,\n'
        '                                    !r_specularMap || r_specularMap->current.integer != 2, &maps);') in device
assert 'pointLight || lightmappedBlend, worldShadow, webColorOrder,' in device
assert '(int)detailMap, detailScale[0], detailScale[1], materialLighting && stage1Bound == 1,' in device
# 3D draws give the color map the composed view's mip bias, 2D draws none;
# r_reflections turns every draw's mirrored surroundings on or off, and
# r_cavity sets how much a lightmapped surface's folds shade it.
assert ('materialLighting && dev->lighting ? g_webEntitySunVisibility : 0.0f,\n'
        '                (int)camoMap, (int)camoMask, camoTiles, camoReference,\n'
        '                is2D ? 0.0f : WebSceneFx_TextureLodBias(),\n'
        '                r_reflections && r_reflections->current.enabled ? 1.0f : 0.0f,\n'
        '                r_cavity ? r_cavity->current.value : 0.0f);') in device
assert 'extern const dvar_t *r_reflections;' in device
assert 'extern const dvar_t *r_cavity;' in device
draw = device.index('WebMaterials_Select(((materialCommands_t *)imp_tess)->material,')
assert device.index('stage1Bound = 1;') < draw < device.index('WebPointLights_Draw(pointLightViewValid', draw)
# A camouflaged gun is a lit model draw of the frame's painted entity and
# material, whatever r_materialLighting says; its maps pass by GL name.
camo = device.index('R_GetDrawWeaponCamo(backEndState->currentEntity, ((materialCommands_t *)imp_tess)->material, &camo)')
assert device.rindex('if (pointLight && dev->lighting &&', 0, camo) > device.rindex('int pointLight =', 0, camo)
assert camo < device.index('camoMap = CDirect3DDevice_GetImageGLId(camo.pattern);', camo) < \
    device.index('camoMask = CDirect3DDevice_GetImageGLId(camo.mask);', camo) < \
    device.index('WebPointLights_Draw(pointLightViewValid', camo)
assert 'unsigned int normalMap = 0, specularMap = 0, detailMap = 0, camoMap = 0, camoMask = 0;' in device
assert '"${CMAKE_CURRENT_LIST_DIR}/web_materials.c"' in (root / 'downstream/wasm/CMakeLists.txt').read_text()
dvars = (root / 'src/PC/gfx_d3d/r_dvars.c').read_text()
for registration in ('r_materialLighting = ri.Dvar_RegisterBool("r_materialLighting", 1, 0x2001);',
                     'r_sunShading = ri.Dvar_RegisterFloat("r_sunShading", 0.65f, 0.0f, 1.0f, 0x2001);',
                     'r_reflections = ri.Dvar_RegisterBool("r_reflections", 1, 0x2001);',
                     'r_cavity = ri.Dvar_RegisterFloat("r_cavity", 1.0f, 0.0f, 2.0f, 0x2001);'):
    at = dvars.index(registration)
    assert dvars.rindex('#ifdef __EMSCRIPTEN__', 0, at) > dvars.rindex('#endif', 0, at)
scene = (root / 'src/PC/gfx_d3d/r_scene.c').read_text()
assert scene.index('WebPointLights_SetScene(scene.dlights') < scene.index('WebMaterials_SetSun(&s_world);') < \
    scene.index('WebSceneFx_SetAds(g_webAdsDofFrac')
# Models restore the sun the DX7 lights fold into their ambient by the
# entity's own visibility, clamped as the engine's light derivation uses it.
shade = (root / 'src/PC/gfx_d3d/rb_shade.c').read_text()
hw = shade.index('static void RB_SetEntityHwLightsDx7_impl(vec4_t *colorForDir, float sunVisibility)\n{')
assert hw < shade.index('g_webEntitySunVisibility = sunVisibility >= 0.0f ? (sunVisibility <= 1.0f ? '
                        'sunVisibility : 1.0f) : 0.0f;', hw) < shade.index('RB_DeriveEntityLights(', hw)
# The wasm32 loader keeps normal and specular images that exist in the
# archives; native DX7 builds still drop them.
loader = (root / 'src/PC/gfx_d3d/r_material_load_obj.c').read_text().replace('\r\n', '\n')
keep = loader.index('if (isDx7 && (unsigned)(sem - 3) <= 1) {\n#ifdef __EMSCRIPTEN__\n')
assert loader.index('FS_ReadFile(path, NULL) > 0)\n                        img = Image_Register(imageName, sem, imageTrack);', keep) < \
    loader.index('#else\n                    *(int *)(tex + 8) = 0;\n#endif', keep)

emscripten = '#define EM_JS(ret, name, params, ...) ret name params;\n'
common_types = r'''
typedef unsigned char byte;
typedef float vec3_t[3];
typedef float vec4_t[4];
typedef struct GfxImage {int id;} GfxImage;
typedef struct {
    const char *name;
    byte samplerState;
    byte semantic;
    byte unused_0;
    byte unused_1;
    union {GfxImage *image; void *water;} u;
} MaterialTextureDef;
typedef struct {const char *name; vec4_t literal;} MaterialConstantDef;
typedef struct {
    unsigned short textureCount;
    unsigned short constantCount;
    MaterialTextureDef *textures;
    MaterialConstantDef *constants;
} Material;
typedef struct {void *def; vec4_t position; vec3_t color;} GfxLight;
typedef struct {GfxLight sunLight;} GfxWorld;
'''
test = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "SOURCE"
static float sun[6];
static int sunCalls;
void WebMaterials_SetSunJs(float x, float y, float z, float r, float g, float b)
{
    sun[0] = x; sun[1] = y; sun[2] = z; sun[3] = r; sun[4] = g; sun[5] = b;
    ++sunCalls;
}
static GfxImage color = {1}, normal = {2}, specular = {3}, detail = {4};
static int water;
static WebMaterialMaps maps;
static void pick(const Material *material, int normalMaps, int specularMaps)
{
    memset(&maps, 0xa5, sizeof(maps));
    WebMaterials_Select(material, normalMaps, specularMaps, &maps);
}
static void none(void)
{
    assert(!maps.normal && !maps.specular && !maps.detail && !maps.detailScale[0] && !maps.detailScale[1]);
}
static int near(float a, float b) { return fabsf(a - b) < 1e-6f; }
static void sunIs(float x, float y, float z, float r, float g, float b)
{
    assert(near(sun[0], x) && near(sun[1], y) && near(sun[2], z) && near(sun[3], r) && near(sun[4], g) && near(sun[5], b));
}
int main(void)
{
    MaterialTextureDef textures[] = {
        {"colorMap", 0, 2, 0, 0, {&color}},
        {"normalMap", 0, 3, 0, 0, {&normal}},
        {"specularMap", 0, 4, 0, 0, {&specular}},
        {"detailMap", 0, 2, 0, 0, {&detail}},
        {"waterMap", 0, 5, 0, 0, {.water = &water}},
    };
    MaterialConstantDef constants[] = {{"colorTint", {0.5f, 2, 3, 1}}, {"detailScale", {8, 6, 0, 0}}};
    Material material = {5, 2, textures, constants};
    pick(&material, 1, 1);
    assert(maps.normal == &normal && maps.specular == &specular && maps.detail == &detail);
    assert(maps.detailScale[0] == 8 && maps.detailScale[1] == 6);
    /* The original renderer overrides drop the maps they replace. */
    pick(&material, 0, 1);
    assert(!maps.normal && maps.specular == &specular && maps.detail == &detail);
    pick(&material, 1, 0);
    assert(maps.normal == &normal && !maps.specular && maps.detail == &detail);
    /* Any order of the material's textures. */
    MaterialTextureDef reversed[5];
    for (int i = 0; i < 5; ++i)
        reversed[i] = textures[4 - i];
    Material shuffled = {5, 2, reversed, constants};
    pick(&shuffled, 1, 1);
    assert(maps.normal == &normal && maps.specular == &specular && maps.detail == &detail && maps.detailScale[1] == 6);
    /* The color map is not a detail map, and water has no image. */
    Material colorOnly = {1, 2, textures, constants};
    pick(&colorOnly, 1, 1);
    none();
    Material waterOnly = {1, 0, textures + 4, NULL};
    pick(&waterOnly, 1, 1);
    none();
    /* Images missing from the archives stay unbound, and a detail map with no
     * image takes no scale. */
    textures[1].u.image = textures[2].u.image = textures[3].u.image = NULL;
    pick(&material, 1, 1);
    none();
    textures[1].u.image = &normal; textures[2].u.image = &specular; textures[3].u.image = &detail;
    /* A detail map needs its material's positive, finite detailScale. */
    Material noConstants = {5, 0, textures, NULL};
    pick(&noConstants, 1, 1);
    assert(maps.normal == &normal && maps.specular == &specular && !maps.detail && !maps.detailScale[0]);
    Material tintOnly = {5, 1, textures, constants};
    pick(&tintOnly, 1, 1);
    assert(maps.normal == &normal && !maps.detail && !maps.detailScale[0] && !maps.detailScale[1]);
    const float invalid[] = {0, -1, NAN, INFINITY, -INFINITY};
    for (int component = 0; component < 2; ++component) {
        for (int i = 0; i < 5; ++i) {
            constants[1].literal[component] = invalid[i];
            pick(&material, 1, 1);
            assert(maps.normal == &normal && maps.specular == &specular && !maps.detail);
            assert(!maps.detailScale[0] && !maps.detailScale[1]);
        }
        constants[1].literal[component] = component ? 6 : 8;
    }
    constants[0].name = NULL;
    textures[0].name = NULL;
    pick(&material, 1, 1);
    assert(maps.detail == &detail && maps.detailScale[0] == 8);
    textures[3].name = NULL;
    pick(&material, 1, 1);
    assert(maps.normal == &normal && !maps.detail && !maps.detailScale[0]);
    pick(NULL, 1, 1);
    none();
    Material empty = {0, 0, NULL, NULL};
    pick(&empty, 1, 1);
    none();

    /* The sun's direction, normalized, and its color at the lightmap's half
     * scale; no sun when the world has none. */
    GfxWorld world = {{0, {0, 3, 4, 0}, {1.2f, 0.8f, -0.5f}}};
    WebMaterials_SetSun(&world);
    assert(sunCalls == 1);
    sunIs(0, 0.6f, 0.8f, 0.6f, 0.4f, 0);
    WebMaterials_SetSun(NULL);
    assert(sunCalls == 2);
    sunIs(0, 0, 0, 0, 0, 0);
    const float bad[] = {NAN, INFINITY, -INFINITY};
    for (int axis = 0; axis < 3; ++axis) {
        for (int i = 0; i < 3; ++i) {
            GfxWorld broken = world;
            broken.sunLight.position[axis] = bad[i];
            WebMaterials_SetSun(&world);
            WebMaterials_SetSun(&broken);
            sunIs(0, 0, 0, 0, 0, 0);
        }
    }
    GfxWorld dark = {{0, {0, 0, 0, 0}, {1, 1, 1}}};
    WebMaterials_SetSun(&dark);
    sunIs(0, 0, 0, 0, 0, 0);
    puts("PASS: normal, specular and detail maps in any order, r_normalMap/r_specularMap overrides, missing images, "
         "color and water maps skipped, detail only with a positive finite detailScale, sun direction and half color");
    return 0;
}
'''

mutants = [
    ('if (def->semantic == 3 && normalMaps)', 'if (def->semantic == 3)'),
    ('else if (def->semantic == 4 && specularMaps)', 'else if (def->semantic == 4)'),
    ('def->name && !strcmp(def->name, "detailMap")', 'def->name'),
    ('def->name && !strcmp(def->name, "detailMap")', '!strcmp(def->name, "detailMap")'),
    ('    memset(maps, 0, sizeof(*maps));\n', ''),
    ('if (!detail || !material->constants)', 'if (!material->constants)'),
    ('if (!constant->name || strcmp(constant->name, "detailScale"))', 'if (!constant->name)'),
    ('isfinite(constant->literal[0]) && isfinite(constant->literal[1]) &&', ''),
    ('constant->literal[0] > 0 && constant->literal[1] > 0', 'constant->literal[0] > 0'),
    ('constant->literal[0] > 0 && constant->literal[1] > 0', 'constant->literal[1] > 0'),
    ('maps->detailScale[1] = constant->literal[1];', 'maps->detailScale[1] = constant->literal[0];'),
    ('if (isfinite(length) && length > 0) {', 'if (length > 0) {'),
    ('z = world->sunLight.position[2] / length;', 'z = world->sunLight.position[2];'),
    ('r = fmaxf(0, world->sunLight.color[0]) * 0.5f;', 'r = fmaxf(0, world->sunLight.color[0]);'),
    ('b = fmaxf(0, world->sunLight.color[2]) * 0.5f;', 'b = world->sunLight.color[2] * 0.5f;'),
]

with tempfile.TemporaryDirectory(prefix='cod2-web-materials-') as tmp:
    path = Path(tmp)
    (path / 'emscripten.h').write_text(emscripten)
    (path / 'common_types.h').write_text(common_types)
    (path / 'test.c').write_text(test.replace('SOURCE', str(source)))
    flags = ['-std=c99', '-D__EMSCRIPTEN__', '-I', str(path), '-I', str(source.parent)]
    subprocess.run(['cc', *flags, '-O1', '-g', '-Wall', '-Werror', '-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all', str(path / 'test.c'), '-lm', '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
    original = source.read_text()
    for index, (before, after) in enumerate(mutants):
        assert original.count(before) == 1, before
        mutant = path / f'mutant{index}.c'
        mutant.write_text(original.replace(before, after))
        (path / f'mutant{index}-test.c').write_text(test.replace('SOURCE', str(mutant)))
        subprocess.run(['cc', *flags, '-O0', '-w', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        str(path / f'mutant{index}-test.c'), '-lm', '-o', str(path / f'mutant{index}')], check=True)
        result = subprocess.run([str(path / f'mutant{index}')], capture_output=True)
        assert result.returncode != 0, f'mutant survived: {after!r}'
    print(f'PASS: {len(mutants)} material selection and sun mutants fail')
