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

EM_JS(void, WebPointLights_Draw, (const float *view, int enabled, int worldShadowEnabled, int colorOrder), {
    Module.cod2VertexColorOrder = colorOrder;
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
