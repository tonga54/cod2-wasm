#include "common_types.h"
#include "web_materials.h"
#include <math.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

/* The DX7 techniques sample only a material's color map and lightmap. The
 * browser keeps the original normal, specular and detail maps of the same
 * material for the per-pixel lighting its generated GL shaders add. */
void WebMaterials_Select(const Material *material, int normalMaps, int specularMaps, WebMaterialMaps *maps)
{
    GfxImage *detail = NULL;

    memset(maps, 0, sizeof(*maps));
    if (!material || !material->textures)
        return;
    for (int i = 0; i < material->textureCount; ++i) {
        const MaterialTextureDef *def = &material->textures[i];
        if (def->semantic == 3 && normalMaps)
            maps->normal = def->u.image;
        else if (def->semantic == 4 && specularMaps)
            maps->specular = def->u.image;
        else if (def->semantic == 2 && def->name && !strcmp(def->name, "detailMap"))
            detail = def->u.image;
    }
    if (!detail || !material->constants)
        return;
    /* The original detail pass tiles the map by the material's detailScale;
     * without one it has no footprint to reproduce. */
    for (int i = 0; i < material->constantCount; ++i) {
        const MaterialConstantDef *constant = &material->constants[i];
        if (!constant->name || strcmp(constant->name, "detailScale"))
            continue;
        if (isfinite(constant->literal[0]) && isfinite(constant->literal[1]) &&
            constant->literal[0] > 0 && constant->literal[1] > 0) {
            maps->detail = detail;
            maps->detailScale[0] = constant->literal[0];
            maps->detailScale[1] = constant->literal[1];
        }
        return;
    }
}

#ifdef __EMSCRIPTEN__
EM_JS(void, WebMaterials_SetSunJs, (float x, float y, float z, float r, float g, float b), {
    var sun = Module.cod2Sun;
    if (!sun) sun = Module.cod2Sun = {world: new Float32Array(3), color: new Float32Array(3), revision: 0};
    if (sun.world[0] === x && sun.world[1] === y && sun.world[2] === z &&
        sun.color[0] === r && sun.color[1] === g && sun.color[2] === b) return;
    sun.world[0] = x; sun.world[1] = y; sun.world[2] = z;
    sun.color[0] = r; sun.color[1] = g; sun.color[2] = b;
    ++sun.revision;
});

/* The DX7 lightmap adds the sun's color once per unit of visibility, at the
 * texture's half scale; the shaders need its direction and that color. */
void WebMaterials_SetSun(const GfxWorld *world)
{
    float x = 0, y = 0, z = 0, r = 0, g = 0, b = 0;
    if (world) {
        float length = sqrtf(world->sunLight.position[0] * world->sunLight.position[0] +
                             world->sunLight.position[1] * world->sunLight.position[1] +
                             world->sunLight.position[2] * world->sunLight.position[2]);
        if (isfinite(length) && length > 0) {
            x = world->sunLight.position[0] / length;
            y = world->sunLight.position[1] / length;
            z = world->sunLight.position[2] / length;
            r = fmaxf(0, world->sunLight.color[0]) * 0.5f;
            g = fmaxf(0, world->sunLight.color[1]) * 0.5f;
            b = fmaxf(0, world->sunLight.color[2]) * 0.5f;
        }
    }
    WebMaterials_SetSunJs(x, y, z, r, g, b);
}
#endif
