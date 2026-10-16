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
    if (lights.sceneActive && lights.sceneFx) {
        var mode = enabled && view ? (enabled > 1 ? 2 : 1) : 0;
        var ads = mode ? lights.adsFrac : 0;
        var focus = mode ? lights.focusDist : 256;
        var vmEnd = mode ? (lights.viewmodelEnd || 15) : 0;
        var fx = lights.sceneFx;
        if (fx[0] !== ads || fx[1] !== focus || fx[2] !== mode || fx[3] !== vmEnd) {
            fx[0] = ads;
            fx[1] = focus;
            fx[2] = mode;
            fx[3] = vmEnd;
            ++lights.sceneFxRevision;
        }
    }
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

EM_JS(void, WebPointLights_SetAds, (float adsFrac, float focusDist, float viewmodelEnd), {
    var lights = Module.cod2PointLights;
    if (!lights) lights = Module.cod2PointLights = {
        world: new Float32Array(32), eye: new Float32Array(16),
        color: new Float32Array(16), view: new Float32Array(16),
        count: 0, drawCount: 0, revision: 0, eyeRevision: 0, transformedRevision: -1
    };
    if (!lights.sceneFx) {
        lights.sceneFx = new Float32Array(4);
        lights.sceneFxRevision = 0;
    }
    lights.adsFrac = Math.fround(adsFrac > 0 ? (adsFrac > 1 ? 1 : adsFrac) : 0);
    lights.focusDist = Math.fround(focusDist > 1 ? focusDist : 256);
    lights.viewmodelEnd = Math.fround(viewmodelEnd > 6 ? viewmodelEnd : 15);
    lights.postFxPending = 1;
    lights.sceneActive = 1;
});

void WebPointLights_SetAdsDof(float adsFrac, float focusDist, float viewmodelEnd)
{
    if (!isfinite(adsFrac) || adsFrac < 0.0f)
        adsFrac = 0.0f;
    if (adsFrac > 1.0f)
        adsFrac = 1.0f;
    if (!isfinite(focusDist) || focusDist < 1.0f)
        focusDist = 256.0f;
    if (!isfinite(viewmodelEnd) || viewmodelEnd < 6.0f)
        viewmodelEnd = 15.0f;
    WebPointLights_SetAds(adsFrac, focusDist, viewmodelEnd);
}

EM_JS(void, WebPointLights_ApplyAdsPostFx, (), {
    var lights = Module.cod2PointLights;
    if (!lights || !lights.postFxPending) return;
    lights.postFxPending = 0;
    if (typeof GLctx === "undefined" || !GLctx || typeof GLImmediate === "undefined" || !GLImmediate) return;
    var w = GLctx.drawingBufferWidth || (GLctx.canvas && GLctx.canvas.width) || 0;
    var h = GLctx.drawingBufferHeight || (GLctx.canvas && GLctx.canvas.height) || 0;
    if (w <= 0 || h <= 0) return;
    var fx = lights.postFx;
    if (!fx || fx.gl !== GLctx) {
        var vs = GLctx.createShader(GLctx.VERTEX_SHADER);
        GLctx.shaderSource(vs, "#version 300 es\nprecision highp float;\nout highp vec2 v_uv;\nvoid main() {\n  vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);\n  v_uv = p;\n  gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);\n}\n");
        GLctx.compileShader(vs);
        var fs = GLctx.createShader(GLctx.FRAGMENT_SHADER);
        GLctx.shaderSource(fs,
            "#version 300 es\nprecision highp float;\nin highp vec2 v_uv;\nout highp vec4 outColor;\nuniform sampler2D u_scene;\nuniform highp vec4 u_params;\n" +
            "void main() {\n" +
            "  vec2 px = vec2(1.0 / u_params.z, 1.0 / u_params.w);\n" +
            "  vec3 base = texture(u_scene, v_uv).rgb;\n" +
            "  vec3 n = texture(u_scene, v_uv + vec2(0.0, px.y)).rgb;\n" +
            "  vec3 s = texture(u_scene, v_uv - vec2(0.0, px.y)).rgb;\n" +
            "  vec3 e = texture(u_scene, v_uv + vec2(px.x, 0.0)).rgb;\n" +
            "  vec3 w = texture(u_scene, v_uv - vec2(px.x, 0.0)).rgb;\n" +
            "  vec3 avg4 = 0.25 * (n + s + e + w);\n" +
            "  vec3 minC = min(base, min(min(n, s), min(e, w)));\n" +
            "  vec3 maxC = max(base, max(max(n, s), max(e, w)));\n" +
            "  vec3 amp = clamp(min(minC, 1.0 - maxC) / max(maxC, vec3(0.0001)), 0.0, 1.0);\n" +
            "  vec3 color = clamp(base + (base - avg4) * (0.48 + 0.36 * amp), 0.0, 1.0);\n" +
            "  vec2 bpx = px * 4.5;\n" +
            "  vec3 b0 = texture(u_scene, clamp(v_uv + vec2(bpx.x, bpx.y), vec2(0.001), vec2(0.999))).rgb;\n" +
            "  vec3 b1 = texture(u_scene, clamp(v_uv + vec2(-bpx.x, bpx.y), vec2(0.001), vec2(0.999))).rgb;\n" +
            "  vec3 b2 = texture(u_scene, clamp(v_uv + vec2(bpx.x, -bpx.y), vec2(0.001), vec2(0.999))).rgb;\n" +
            "  vec3 b3 = texture(u_scene, clamp(v_uv + vec2(-bpx.x, -bpx.y), vec2(0.001), vec2(0.999))).rgb;\n" +
            "  vec3 wideAvg = 0.25 * (b0 + b1 + b2 + b3);\n" +
            "  float wideLuma = dot(wideAvg, vec3(0.299, 0.587, 0.114));\n" +
            "  float baseLuma = dot(base, vec3(0.299, 0.587, 0.114));\n" +
            "  float ao = clamp((wideLuma - baseLuma) * 0.32, 0.0, 0.10);\n" +
            "  color *= 1.0 - ao;\n" +
            "  color += wideAvg * (max(wideLuma - 0.58, 0.0) * 0.22);\n" +
            "  float ads = u_params.x;\n" +
            "  if (ads > 0.005) {\n" +
            "    vec2 d = v_uv - vec2(0.5, 0.515);\n" +
            "    float aspect = u_params.z / max(u_params.w, 1.0);\n" +
            "    vec2 da = vec2(d.x * aspect, d.y);\n" +
            "    float sightWindow = 1.0 - smoothstep(0.048, 0.130, length(vec2(d.x * aspect * 1.15, (v_uv.y - 0.535) * 1.1)));\n" +
            "    float postSlot = 1.0 - smoothstep(0.015, 0.036, length(vec2(d.x * aspect * 1.9, max(0.0, 0.482 - v_uv.y) * 2.2)));\n" +
            "    float sharpKeep = max(sightWindow, postSlot * step(0.468, v_uv.y));\n" +
            "    float gunBody = (1.0 - smoothstep(0.435, 0.502, v_uv.y)) * (1.0 - smoothstep(0.085, 0.145, abs(v_uv.x - 0.5)));\n" +
            "    float sightEars = (1.0 - smoothstep(0.475, 0.545, v_uv.y)) * smoothstep(0.016, 0.034, abs(v_uv.x - 0.5)) * (1.0 - smoothstep(0.058, 0.105, abs(v_uv.x - 0.5)));\n" +
            "    float armAxis = v_uv.x - (0.35 + 0.34 * v_uv.y);\n" +
            "    float leftArm = (1.0 - smoothstep(0.32, 0.44, v_uv.y)) * (1.0 - smoothstep(0.065, 0.145, abs(armAxis)));\n" +
            "    float pistolHands = (1.0 - smoothstep(0.28, 0.40, v_uv.y)) * (1.0 - smoothstep(0.095, 0.175, abs(v_uv.x - 0.52)));\n" +
            "    float periph = smoothstep(0.22, 0.62, length(da)) * 0.58;\n" +
            "    float coc = max(max(gunBody, max(sightEars * 0.94, max(leftArm, pistolHands) * 0.96)), periph) * (1.0 - sharpKeep) * ads;\n" +
            "    if (coc > 0.015) {\n" +
            "      vec2 stepUv = px * (u_params.y * coc);\n" +
            "      vec3 sum = base * 0.12;\n" +
            "      float wSum = 0.12;\n" +
            "      const vec2 taps[12] = vec2[12](\n" +
            "        vec2(0.326, 0.406), vec2(-0.540, 0.174), vec2(0.096, -0.636), vec2(0.440, -0.521),\n" +
            "        vec2(-0.696, -0.354), vec2(0.818, 0.085), vec2(-0.203, 0.825), vec2(-0.841, 0.433),\n" +
            "        vec2(0.556, 0.784), vec2(-0.312, -0.923), vec2(0.962, -0.268), vec2(-0.978, -0.062)\n" +
            "      );\n" +
            "      for (int i = 0; i < 12; ++i) {\n" +
            "        vec3 st = texture(u_scene, clamp(v_uv + taps[i] * stepUv, vec2(0.001), vec2(0.999))).rgb;\n" +
            "        float tl = dot(st, vec3(0.299, 0.587, 0.114));\n" +
            "        float bw = 1.0 + 1.45 * tl * tl * coc;\n" +
            "        sum += st * bw;\n" +
            "        wSum += bw;\n" +
            "      }\n" +
            "      color = mix(color, sum / wSum, clamp(coc * 1.12, 0.0, 1.0));\n" +
            "    }\n" +
            "  }\n" +
            "  float luma = dot(color, vec3(0.299, 0.587, 0.114));\n" +
            "  color = mix(color, vec3(luma), 0.06);\n" +
            "  vec3 tint = mix(vec3(0.88, 0.93, 1.02), vec3(1.07, 1.03, 0.94), smoothstep(0.08, 0.85, luma));\n" +
            "  color *= tint;\n" +
            "  color = clamp((color - 0.5) * 1.13 + 0.505, 0.0, 1.0);\n" +
            "  outColor = vec4(color, 1.0);\n" +
            "}\n");
        GLctx.compileShader(fs);
        var prog = GLctx.createProgram();
        GLctx.attachShader(prog, vs);
        GLctx.attachShader(prog, fs);
        GLctx.linkProgram(prog);
        var tex = GLctx.createTexture();
        var vao = GLctx.createVertexArray();
        fx = lights.postFx = {
            gl: GLctx,
            program: prog,
            tex: tex,
            vao: vao,
            uScene: GLctx.getUniformLocation(prog, "u_scene"),
            uParams: GLctx.getUniformLocation(prog, "u_params"),
            width: 0,
            height: 0
        };
    }
    GLctx.activeTexture(GLctx.TEXTURE7);
    GLctx.bindTexture(GLctx.TEXTURE_2D, fx.tex);
    if (fx.width !== w || fx.height !== h) {
        GLctx.texParameteri(GLctx.TEXTURE_2D, GLctx.TEXTURE_MIN_FILTER, GLctx.LINEAR);
        GLctx.texParameteri(GLctx.TEXTURE_2D, GLctx.TEXTURE_MAG_FILTER, GLctx.LINEAR);
        GLctx.texParameteri(GLctx.TEXTURE_2D, GLctx.TEXTURE_WRAP_S, 0x812F);
        GLctx.texParameteri(GLctx.TEXTURE_2D, GLctx.TEXTURE_WRAP_T, 0x812F);
        GLctx.copyTexImage2D(GLctx.TEXTURE_2D, 0, GLctx.RGB, 0, 0, w, h, 0);
        fx.width = w;
        fx.height = h;
    } else {
        GLctx.copyTexSubImage2D(GLctx.TEXTURE_2D, 0, 0, 0, 0, 0, w, h);
    }
    GLctx.useProgram(fx.program);
    GLctx.uniform1i(fx.uScene, 7);
    var radiusPx = Math.fround(9.2 * (h / 720.0));
    GLctx.uniform4f(fx.uParams, lights.adsFrac || 0, radiusPx, w, h);
    GLctx.disable(0x0B71);
    GLctx.disable(0x0BE2);
    GLctx.disable(0x0B44);
    GLctx.depthMask(false);
    GLctx.colorMask(true, true, true, true);
    GLctx.bindVertexArray(fx.vao);
    GLctx.drawArrays(GLctx.TRIANGLES, 0, 3);
    GLctx.bindVertexArray(null);
    GLImmediate.lastArrayBuffer = null;
    GLctx.currentArrayBufferBinding = 0;
    GLctx.currentElementArrayBufferBinding = 0;
    GLctx.activeTexture(GLctx.TEXTURE0);
    GLImmediate.fixedFunctionProgram = null;
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
