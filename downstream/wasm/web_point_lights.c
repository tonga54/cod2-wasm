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

EM_JS(void, WebPointLights_Draw, (const float *view, int enabled), {
    var lights = Module.cod2PointLights;
    if (!lights) return;
    lights.drawCount = enabled && view ? lights.count : 0;
    if (!lights.drawCount) return;
    var offset = view >>> 2;
    var changed = lights.transformedRevision !== lights.revision;
    for (var i = 0; i < 16; ++i)
        if (lights.view[i] !== HEAPF32[offset + i]) changed = true;
    if (!changed) return;
    for (var i = 0; i < 16; ++i) lights.view[i] = HEAPF32[offset + i];
    var m = lights.view, world = lights.world;
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
});
