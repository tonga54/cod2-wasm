#include "common_types.h"
#include <emscripten.h>
#include <math.h>

/* Use the original FX lights in the existing opaque pass. DX7 world vertices
 * have no normals: the fragment shader derives them from their eye position.
 * Four lights bound the GPU work without repeating any geometry or uploads. */
EM_JS(void, WebPointLights_Set, (const float *packed, int count), {
    var lights = Module.cod2PointLights;
    if (!lights) lights = Module.cod2PointLights = {
        world: new Float32Array(32), eye: new Float32Array(16),
        color: new Float32Array(16), view: new Float32Array(16),
        count: 0, drawCount: 0, revision: 0, eyeRevision: 0, transformedRevision: -1
    };
    lights.world.set(HEAPF32.subarray(packed >>> 2, (packed >>> 2) + count * 8));
    lights.count = count;
    ++lights.revision;
});

void WebPointLights_SetScene(const GfxLight *lights, int count)
{
    float packed[32];
    int used = 0;
    for (int i = 0; i < count && used < 4; ++i) {
        const GfxLight *light = &lights[i];
        if (!isfinite(light->position[3]) || light->position[3] <= 0)
            continue;
        int valid = 1;
        for (int j = 0; j < 3; ++j)
            valid &= isfinite(light->position[j]) && isfinite(light->color[j]);
        if (!valid)
            continue;
        float *dst = packed + used++ * 8;
        for (int j = 0; j < 4; ++j)
            dst[j] = light->position[j];
        for (int j = 0; j < 3; ++j)
            dst[j + 4] = fmaxf(0, light->color[j]);
        dst[7] = 0;
    }
    WebPointLights_Set(packed, used);
}

EM_JS(void, WebPointLights_Draw, (const float *view, int enabled, int worldShadowEnabled, int colorOrder,
                                  int normalMap, int specularMap, int detailMap, float detailX, float detailY,
                                  int lightmapped, float specularScale, float sunShading, float sunVisibility,
                                  int camoMap, int camoMask, float camoTiles, float camoReference, float lodBias,
                                  float reflection, float cavity, float variation, float detailRelief,
                                  float sharpen, int viewmodel), {
    Module.cod2VertexColorOrder = colorOrder;
    // The material's original maps, for the generated shaders' per-pixel pass,
    // and a camouflaged gun's pattern and paint mask.
    var material = Module.cod2Material;
    if (!material) {
        material = Module.cod2Material = {
            surface: new Float32Array(4), detail: new Float32Array(4),
            sunEye: new Float32Array(3), skyEye: new Float32Array(4), sunColor: new Float32Array(4),
            model: new Float32Array(4), camo: new Float32Array(4), view: new Float32Array(16),
            // For lightmapped surfaces: how much the folds of the normal map shade
            // them (r_cavity), how much they vary in tone and their terrain layers
            // follow their relief (r_surfaceVariation), how far the detail map
            // tilts their normal (r_detailRelief) and how much a magnified color
            // map is sharpened (r_textureSharpen).
            relief: new Float32Array(4),
            sunShadowEye: new Float32Array(16), sunShadow: new Float32Array(4), sunShadowRect: new Float32Array(4),
            // Coverage, feather, heat and near clip: web_effects.c sets them after these.
            effect: new Float32Array(4),
            sunShadowRevision: -1,
            bound: [null, null, null, null, null], present: [0, 0, 0, 0, 0], sunRevision: -1,
            // The detail, normal and specular maps, then the camouflage's pattern
            // and mask past the units the scene compositor binds (web_scene_fx.c, 8-15).
            units: [5, 6, 7, 16, 17]
        };
        // World shaders compare against the sun's depth map on unit 18
        // (web_sun_shadows.c). Until it exists, a one-texel map at the far
        // plane keeps that shadow sampler valid.
        if (!Module.cod2SunShadowPass || !Module.cod2SunShadowPass.texture) {
            var shadowUnit = GLctx.getParameter(GLctx.ACTIVE_TEXTURE);
            GLctx.activeTexture(GLctx.TEXTURE18);
            material.sunShadowDefault = GLctx.createTexture();
            GLctx.bindTexture(GLctx.TEXTURE_2D, material.sunShadowDefault);
            GLctx.texImage2D(GLctx.TEXTURE_2D, 0, GLctx.DEPTH_COMPONENT16, 1, 1, 0, GLctx.DEPTH_COMPONENT,
                GLctx.UNSIGNED_SHORT, new Uint16Array([65535]));
            GLctx.texParameteri(GLctx.TEXTURE_2D, GLctx.TEXTURE_MIN_FILTER, GLctx.NEAREST);
            GLctx.texParameteri(GLctx.TEXTURE_2D, GLctx.TEXTURE_MAG_FILTER, GLctx.NEAREST);
            GLctx.texParameteri(GLctx.TEXTURE_2D, GLctx.TEXTURE_COMPARE_MODE, GLctx.COMPARE_REF_TO_TEXTURE);
            GLctx.texParameteri(GLctx.TEXTURE_2D, GLctx.TEXTURE_COMPARE_FUNC, GLctx.LEQUAL);
            GLctx.activeTexture(shadowUnit);
        }
    }
    // The engine sets sampler state only on the stages it binds itself:
    // without mipmaps these maps would sparkle wherever they minify.
    if (material.sampler === undefined) {
        var sampler = material.sampler = GLctx.createSampler ? GLctx.createSampler() : null;
        if (sampler) {
            GLctx.samplerParameteri(sampler, GLctx.TEXTURE_MIN_FILTER, GLctx.LINEAR_MIPMAP_LINEAR);
            GLctx.samplerParameteri(sampler, GLctx.TEXTURE_MAG_FILTER, GLctx.LINEAR);
            GLctx.samplerParameteri(sampler, GLctx.TEXTURE_WRAP_S, GLctx.REPEAT);
            GLctx.samplerParameteri(sampler, GLctx.TEXTURE_WRAP_T, GLctx.REPEAT);
            // As sharp at a grazing angle as the color maps' default r_anisotropy.
            var anisotropic = GLctx.getExtension('EXT_texture_filter_anisotropic');
            if (anisotropic) GLctx.samplerParameterf(sampler, anisotropic.TEXTURE_MAX_ANISOTROPY_EXT,
                Math.max(1, Math.min(16, GLctx.getParameter(anisotropic.MAX_TEXTURE_MAX_ANISOTROPY_EXT) || 1)));
            for (var samplerUnit = 0; samplerUnit < 5; ++samplerUnit) GLctx.bindSampler(material.units[samplerUnit], sampler);
        }
    }
    // Compare texture objects: a deleted texture's name can return for another.
    var maps = [detailMap, normalMap, specularMap, camoMap, camoMask], present = material.present, rebound = false;
    for (var unit = 0; unit < 5; ++unit) {
        var texture = maps[unit] ? GL.textures[maps[unit]] : null;
        present[unit] = texture ? 1 : 0;
        if (!texture || material.bound[unit] === texture) continue;
        GLctx.activeTexture(GLctx.TEXTURE0 + material.units[unit]);
        GLctx.bindTexture(GLctx.TEXTURE_2D, texture);
        material.bound[unit] = texture;
        rebound = true;
    }
    if (rebound) GLctx.activeTexture(GLctx.TEXTURE0 + GLImmediate.TexEnvJIT.getActiveTexture());
    var surface = material.surface, detail = material.detail;
    surface[0] = present[1]; surface[1] = present[2]; surface[2] = lightmapped ? 1 : 0; surface[3] = specularScale;
    detail[0] = present[0] ? detailX : 0; detail[1] = present[0] ? detailY : 0; detail[2] = present[0];
    // The color map's mip bias: a scene rendered below the canvas's lines
    // samples the detail of the resolution it is scaled up to.
    detail[3] = lodBias;
    // Whether surfaces with a specular map mirror the sky and ground (r_reflections).
    material.skyEye[3] = reflection;
    material.model[0] = sunVisibility;
    // The arms and weapon, drawn in the depth hack's range, take the sky's
    // light from above the shoulder.
    material.model[1] = viewmodel ? 1 : 0;
    material.relief[0] = cavity > 0 ? Math.min(cavity, 2) : 0;
    material.relief[1] = variation > 0 ? Math.min(variation, 2) : 0;
    material.relief[2] = detailRelief > 0 ? Math.min(detailRelief, 2) : 0;
    material.relief[3] = sharpen > 0 ? Math.min(sharpen, 2) : 0;
    var camo = material.camo, painted = present[3] && present[4] ? 1 : 0;
    camo[0] = painted; camo[1] = painted ? camoTiles : 0; camo[2] = painted ? camoReference : 0;
    var viewChanged = false;
    if (view) {
        var viewOffset = view >>> 2;
        for (var i = 0; i < 16; ++i)
            if (material.view[i] !== HEAPF32[viewOffset + i]) { viewChanged = true; break; }
        if (viewChanged) {
            material.view.set(HEAPF32.subarray(viewOffset, viewOffset + 16));
            material.sunRevision = material.sunShadowRevision = -1;
        }
    }
    var sun = Module.cod2Sun;
    if (view && sun) {
        if (material.sunRevision !== sun.revision || material.sunColor[3] !== sunShading) {
            var v = material.view, w = sun.world;
            var x = v[0] * w[0] + v[4] * w[1] + v[8] * w[2];
            var y = v[1] * w[0] + v[5] * w[1] + v[9] * w[2];
            var z = v[2] * w[0] + v[6] * w[1] + v[10] * w[2];
            var length = Math.sqrt(x * x + y * y + z * z) || 1;
            material.sunEye[0] = x / length; material.sunEye[1] = y / length; material.sunEye[2] = z / length;
            length = Math.sqrt(v[8] * v[8] + v[9] * v[9] + v[10] * v[10]) || 1;
            material.skyEye[0] = v[8] / length; material.skyEye[1] = v[9] / length; material.skyEye[2] = v[10] / length;
            material.sunColor.set(sun.color);
            material.sunColor[3] = sunShading;
            material.sunRevision = sun.revision;
        }
    }
    // The sun's depth map (web_sun_shadows.c) for lightmapped world draws, in
    // this draw's eye space. The view is a rotation and a move: its inverse
    // is its transpose and the opposite move.
    var sunShadow = Module.cod2SunShadow;
    if (view && sunShadow && worldShadowEnabled && sunShadow.params[0] > 0) {
        if (material.sunShadowRevision !== sunShadow.revision) {
            var m = material.view, t = sunShadow.world, e = material.sunShadowEye;
            for (var row = 0; row < 4; ++row) {
                for (var column = 0; column < 3; ++column)
                    e[column * 4 + row] = t[row] * m[column] + t[4 + row] * m[4 + column] + t[8 + row] * m[8 + column];
                var move = t[12 + row];
                for (var k = 0; k < 3; ++k)
                    move -= t[k * 4 + row] * (m[k * 4] * m[12] + m[k * 4 + 1] * m[13] + m[k * 4 + 2] * m[14]);
                e[12 + row] = move;
            }
            material.sunShadowRevision = sunShadow.revision;
            material.sunShadowRect.set(sunShadow.rect);
        }
        material.sunShadow.set(sunShadow.params);
    } else {
        material.sunShadow[0] = 0;
    }
    var lights = Module.cod2PointLights;
    if (!lights) return;
    lights.drawCount = enabled && view ? lights.count : 0;
    lights.shadowDrawCount = worldShadowEnabled && view && lights.shadowCount ? lights.shadowCount : 0;
    if (!lights.drawCount && !lights.shadowDrawCount) return;
    var offset = view >>> 2;
    var changed = false;
    for (var i = 0; i < 16; ++i)
        if (lights.view[i] !== HEAPF32[offset + i]) changed = true;
    var transformLights = lights.drawCount && (changed || lights.transformedRevision !== lights.revision);
    var transformShadows = lights.shadowDrawCount && (changed || lights.shadowTransformedRevision !== lights.shadowRevision);
    // A changed view invalidates inactive data too: a later draw may enable it.
    if (changed) {
        lights.transformedRevision = -1;
        lights.shadowTransformedRevision = -1;
    }
    if (!transformLights && !transformShadows) return;
    for (var i = 0; i < 16; ++i) lights.view[i] = HEAPF32[offset + i];
    var m = lights.view, world = lights.world;
    if (transformLights) {
      for (var i = 0; i < lights.count; ++i) {
        var p = i * 8, q = i * 4;
        for (var j = 0; j < 3; ++j) {
            lights.eye[q + j] = m[j] * world[p] + m[4 + j] * world[p + 1]
                + m[8 + j] * world[p + 2] + m[12 + j];
            lights.color[q + j] = world[p + 4 + j];
        }
        lights.eye[q + 3] = 1 / (world[p + 3] * world[p + 3]);
      }
      lights.transformedRevision = lights.revision;
      ++lights.eyeRevision;
    }
    if (transformShadows) {
      world = lights.shadowWorld;
      for (var i = 0; i < lights.shadowCount; ++i) {
        var p = i * 4;
        for (var j = 0; j < 3; ++j) {
            lights.shadowEye[p + j] = m[j] * world[p] + m[4 + j] * world[p + 1]
                + m[8 + j] * world[p + 2] + m[12 + j];
        }
        lights.shadowEye[p + 3] = 1 / (world[p + 3] * world[p + 3]);
      }
      lights.shadowTransformedRevision = lights.shadowRevision;
      ++lights.shadowEyeRevision;
    }
});

EM_JS(void, WebPointLights_SetShadows, (const float *packed, int count), {
    var lights = Module.cod2PointLights;
    if (!lights) lights = Module.cod2PointLights = {
        world: new Float32Array(32), eye: new Float32Array(16),
        color: new Float32Array(16), view: new Float32Array(16),
        count: 0, drawCount: 0, revision: 0, eyeRevision: 0, transformedRevision: -1
    };
    if (!lights.shadowWorld) {
        lights.shadowWorld = new Float32Array(16);
        lights.shadowEye = new Float32Array(16);
        lights.shadowCount = 0;
        lights.shadowDrawCount = 0;
        lights.shadowRevision = 0;
        lights.shadowEyeRevision = 0;
        lights.shadowTransformedRevision = -1;
    }
    lights.shadowWorld.set(HEAPF32.subarray(packed >>> 2, (packed >>> 2) + count * 4));
    lights.shadowCount = count;
    ++lights.shadowRevision;
});

void WebPointLights_SetShadowCasters(const GfxEntity *entities, int entityCount, const float *viewOrg)
{
    float packed[16];
    float bestDistSq[4];
    int used = 0;

    if (entities && viewOrg && entityCount > 0 &&
        isfinite(viewOrg[0]) && isfinite(viewOrg[1]) && isfinite(viewOrg[2])) {
        for (int i = 0; i < entityCount; ++i) {
            const GfxEntity *ent = &entities[i];
            if ((int)ent->reType != 0 || ent->renderFxFlags != 0x80)
                continue;
            if (!isfinite(ent->origin[0]) || !isfinite(ent->origin[1]) ||
                !isfinite(ent->origin[2]) || !isfinite(ent->lighting.baseCoords[2]))
                continue;
            float dx = ent->origin[0] - viewOrg[0];
            float dy = ent->origin[1] - viewOrg[1];
            float dz = ent->origin[2] - viewOrg[2];
            float distSq = dx * dx + dy * dy + dz * dz;
            if (!isfinite(distSq) || distSq > 1600.0f * 1600.0f)
                continue;
            float height = ent->lighting.baseCoords[2] - ent->origin[2];
            float radius = height <= 14.0f ? 30.0f : (height <= 24.0f ? 25.0f : 22.0f);
            int slot = used;
            if (slot < 4) {
                ++used;
            } else if (distSq < bestDistSq[3]) {
                slot = 3;
            } else {
                continue;
            }
            while (slot > 0 && distSq < bestDistSq[slot - 1]) {
                bestDistSq[slot] = bestDistSq[slot - 1];
                for (int c = 0; c < 4; ++c)
                    packed[slot * 4 + c] = packed[(slot - 1) * 4 + c];
                --slot;
            }
            bestDistSq[slot] = distSq;
            packed[slot * 4 + 0] = ent->origin[0];
            packed[slot * 4 + 1] = ent->origin[1];
            packed[slot * 4 + 2] = ent->origin[2];
            packed[slot * 4 + 3] = radius;
        }
    }
    WebPointLights_SetShadows(packed, used);
}
