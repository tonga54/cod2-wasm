#include "common_types.h"
#include "web_planar.h"
#include <emscripten.h>
#include <math.h>
#include <string.h>

extern const dvar_t *r_reflections;
static int s_available, s_drawing;
static float s_height;

/* A reflected surface-list pass reuses the submitted geometry and materials.
 * It keeps a single framebuffer bound throughout the list: switching targets
 * per mesh forces costly tile resolves on mobile/Metal GPUs. Two color targets
 * keep last frame readable while this one is built, without weapons or HUD. */
EM_JS(float, WebPlanar_NewFrame, (), {
    var fx = Module.cod2SceneFx, camera = Module.cod2WaterCamera;
    if (!fx || !fx.drawing || !fx.t || !camera || !camera.valid) return NaN;
    var gl = fx.gl, t = fx.t;
    var p = t.planar;
    if (!p) p = t.planar = { candidate: null, write: 0, read: 1, slots: [], frame: 0 };
    var candidate = p.candidate;
    p.candidate = null; p.drew = false; p.frame = fx.frame;
    if (!candidate) return NaN;
    if (!p.slots.length) {
        p.width = Math.max(1, Math.ceil(fx.width / 2));
        p.height = Math.max(1, Math.ceil(fx.height / 2));
        p.levels = Math.min(6, Math.floor(Math.log2(Math.max(p.width, p.height))) + 1);
        p.viewportDirect = fx.scaled ? fx.viewportDirect : gl.viewport;
        var active = gl.getParameter(gl.ACTIVE_TEXTURE), rb = gl.getParameter(gl.RENDERBUFFER_BINDING);
        gl.activeTexture(gl.TEXTURE26);
        p.depth = gl.createRenderbuffer(); t.renderbuffers.push(p.depth);
        gl.bindRenderbuffer(gl.RENDERBUFFER, p.depth);
        gl.renderbufferStorage(gl.RENDERBUFFER, gl.DEPTH_COMPONENT24, p.width, p.height);
        p.ok = true;
        for (var i = 0; i < 2; ++i) {
            var slot = { color: gl.createTexture(), fbo: gl.createFramebuffer(), valid: false };
            t.textures.push(slot.color); t.framebuffers.push(slot.fbo); p.slots.push(slot);
            gl.bindTexture(gl.TEXTURE_2D, slot.color);
            gl.texStorage2D(gl.TEXTURE_2D, p.levels, gl.RGBA8, p.width, p.height);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR_MIPMAP_LINEAR);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
            gl.bindFramebuffer(gl.FRAMEBUFFER, slot.fbo);
            gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, slot.color, 0);
            gl.framebufferRenderbuffer(gl.FRAMEBUFFER, gl.DEPTH_ATTACHMENT, gl.RENDERBUFFER, p.depth);
            p.ok = p.ok && gl.checkFramebufferStatus(gl.FRAMEBUFFER) === gl.FRAMEBUFFER_COMPLETE;
        }
        gl.bindTexture(gl.TEXTURE_2D, p.slots[p.read].color);
        gl.bindRenderbuffer(gl.RENDERBUFFER, rb); gl.activeTexture(active);
        gl.bindFramebuffer(gl.FRAMEBUFFER, t.ms || t.scene);
        if (!p.ok) console.warn('[planar] reflection target unavailable; keeping screen-space/sky reflections');
    }
    if (!p.ok) return NaN;
    p.plane = candidate.height;
    var slot = p.slots[p.write];
    slot.valid = false; slot.plane = p.plane;
    slot.view = new Float32Array(camera.view);
    slot.sourceView = new Float32Array(camera.view);
    for (var k = 0; k < 4; ++k) {
        slot.view[12 + k] += 2 * p.plane * slot.view[8 + k];
        slot.view[8 + k] = -slot.view[8 + k];
    }
    slot.projection = new Float32Array(camera.projection);
    slot.jitter = new Float32Array(camera.jitter);
    slot.time = camera.time; slot.frame = fx.frame;
    return p.plane;
});

void WebPlanar_BeginFrame(void)
{
    s_available = s_drawing = 0;
    if (!r_reflections || !r_reflections->current.enabled) return;
    s_height = WebPlanar_NewFrame();
    s_available = isfinite(s_height);
}

EM_JS(int, WebPlanar_BindDraw, (), {
    var fx = Module.cod2SceneFx, p = fx && fx.t && fx.t.planar;
    if (!fx || !fx.drawing || !p || !p.ok || !p.slots.length) return 0;
    var gl = fx.gl;
    p.viewport = gl.getParameter(gl.VIEWPORT);
    p.scissor = gl.isEnabled(gl.SCISSOR_TEST); p.stencil = gl.isEnabled(gl.STENCIL_TEST);
    gl.disable(gl.SCISSOR_TEST); gl.disable(gl.STENCIL_TEST);
    gl.bindFramebuffer(gl.FRAMEBUFFER, p.slots[p.write].fbo);
    p.viewportDirect.call(gl, 0, 0, p.width, p.height);
    if (!p.drew) {
        gl.colorMask(true, true, true, true); gl.depthMask(true);
        gl.clearBufferfv(gl.COLOR, 0, new Float32Array(4));
        gl.clearBufferfv(gl.DEPTH, 0, new Float32Array([1]));
    }
    p.drew = true; fx.planarDrawing = true;
    return 1;
});

int WebPlanar_BeginDraw(int eligible)
{
    if (!eligible || !s_available || s_drawing) return 0;
    s_drawing = WebPlanar_BindDraw();
    return s_drawing;
}

int WebPlanar_IsDrawing(void) { return s_drawing; }

EM_JS(void, WebPlanar_RestoreDraw, (), {
    var fx = Module.cod2SceneFx, p = fx.t.planar, gl = fx.gl;
    gl.bindFramebuffer(gl.FRAMEBUFFER, fx.t.ms || fx.t.scene);
    p.viewportDirect.apply(gl, p.viewport);
    if (p.scissor) gl.enable(gl.SCISSOR_TEST);
    if (p.stencil) gl.enable(gl.STENCIL_TEST);
    fx.planarDrawing = false;
});

void WebPlanar_EndDraw(void)
{
    if (s_drawing) WebPlanar_RestoreDraw();
    s_drawing = 0;
}

void WebPlanar_MirrorView(float *view)
{
    if (!s_drawing || !view) return;
    for (int k = 0; k < 4; ++k) {
        view[12 + k] += 2.0f * s_height * view[8 + k];
        view[8 + k] = -view[8 + k];
    }
}

EM_JS(void, WebPlanar_Uniforms, (const float *view, int reflected, float height), {
    var m = Module.cod2Material;
    if (!m || !m.planarProjection) return;
    m.planarProjection[3] = 0; m.planarClip.fill(0);
    if (!view) return;
    var v = HEAPF32.subarray(view >>> 2, (view >>> 2) + 16);
    if (reflected) {
        // World z above the selected water plane, expressed in this eye space.
        m.planarClip.set([v[8], v[9], v[10], -v[8]*v[12]-v[9]*v[13]-v[10]*v[14]-height-0.125]);
        m.waterProjection[3] = 0;
        return;
    }
    if (m.environment[2] === 0) return;
    var fx = Module.cod2SceneFx, p = fx && fx.t && fx.t.planar, now = Module.cod2WaterCamera;
    var slot = p && p.slots[p.read];
    if (!fx || !fx.drawing || !slot || !slot.valid || slot.frame !== fx.frame - 1 || !now || !now.valid ||
        now.time < slot.time || now.time - slot.time > 100) return;
    var old = slot.sourceView, distance2 = 0, rotationTrace = 0;
    for (var k=0;k<3;++k) {
        var at=k*4;
        var delta=-v[at]*v[12]-v[at+1]*v[13]-v[at+2]*v[14]+
            old[at]*old[12]+old[at+1]*old[13]+old[at+2]*old[14];
        distance2 += delta*delta;
        rotationTrace += old[at]*v[at]+old[at+1]*v[at+1]+old[at+2]*v[at+2];
    }
    if (distance2>4096 || rotationTrace<2.7) return;
    var a = slot.view, b = m.planarView;
    for (var c = 0; c < 3; ++c)
        for (var r = 0; r < 3; ++r)
            b[c*4+r] = a[r]*v[c] + a[4+r]*v[4+c] + a[8+r]*v[8+c];
    for (var r = 0; r < 3; ++r) b[12+r] = a[12+r]-b[r]*v[12]-b[4+r]*v[13]-b[8+r]*v[14];
    b[3] = b[7] = b[11] = 0; b[15] = 1;
    m.planarPlane.set([v[8],v[9],v[10],-v[8]*v[12]-v[9]*v[13]-v[10]*v[14]-slot.plane]);
    m.planarProjection.set([slot.projection[0],slot.projection[1],p.levels-1,1]);
    m.planarJitter.set(slot.jitter);
});

void WebPlanar_Material(const float *view) { WebPlanar_Uniforms(view, s_drawing, s_height); }

EM_JS(void, WebPlanar_AddCandidate, (const float *positions, int stride, int count, const unsigned short *indices, int indexCount, int indexBase, const float *world, const float *view, int road), {
    var fx = Module.cod2SceneFx;
    if (!fx || !fx.drawing || !fx.t || fx.planarDrawing) return;
    var p = fx.t.planar;
    if (!p) p = fx.t.planar = { candidate: null, write: 0, read: 1, slots: [], frame: fx.frame };
    var w = HEAPF32.subarray(world >>> 2, (world >>> 2)+16), v = HEAPF32.subarray(view >>> 2, (view >>> 2)+16);
    // A draw's vertex range can include unrelated surfaces at other heights.
    // Select an actual submitted horizontal triangle, never the whole range.
    var triangles=Math.floor(indexCount/3), samples=Math.min(triangles,128);
    var base=positions>>>2, step=stride>>>2, index=indices>>>1;
    var cameraZ=-v[8]*v[12]-v[9]*v[13]-v[10]*v[14], xyz=new Float32Array(9), eye=new Float32Array(9);
    var projection=Module.cod2WaterCamera.projection;
    for (var i=0;i<samples;++i) {
        var tri=index+Math.floor(i*(triangles-1)/Math.max(samples-1,1))*3, valid=true;
        for(var j=0;j<3;++j){
            var vertex=HEAPU16[tri+j]-indexBase;
            if(vertex<0 || vertex>=count){valid=false;break;}
            var at=base+vertex*step, x=HEAPF32[at], y=HEAPF32[at+1], z=HEAPF32[at+2];
            for(var k=0;k<3;++k)xyz[j*3+k]=w[k]*x+w[4+k]*y+w[8+k]*z+w[12+k];
        }
        if(!valid)continue;
        var lo=Math.min(xyz[2],xyz[5],xyz[8]), hi=Math.max(xyz[2],xyz[5],xyz[8]), height=(lo+hi)*.5;
        if(!Number.isFinite(height) || hi-lo>.25 || cameraZ-height<4 || cameraZ-height>384)continue;
        var area=Math.abs((xyz[3]-xyz[0])*(xyz[7]-xyz[1])-(xyz[6]-xyz[0])*(xyz[4]-xyz[1]));
        if(!(area>2))continue;
        // Prefer the water occupying this view, not a large patch behind the
        // camera that happens to share a submitted vertex buffer. Clip edges
        // to the near plane before estimating their viewport footprint.
        for(var j=0;j<3;++j)for(var k=0;k<3;++k)
            eye[j*3+k]=v[k]*xyz[j*3]+v[4+k]*xyz[j*3+1]+v[8+k]*xyz[j*3+2]+v[12+k];
        var minX=1,minY=1,maxX=-1,maxY=-1;
        for(var j=0;j<3;++j){
            var a=j*3,b=((j+1)%3)*3;
            for(var edge=0;edge<2;++edge){
                var x=eye[a],y=eye[a+1],z=eye[a+2];
                if(!edge){if(z>-4)continue;}
                else {
                    if((z<=-4)===(eye[b+2]<=-4))continue;
                    var t=(-4-z)/(eye[b+2]-z);
                    x+=(eye[b]-x)*t;y+=(eye[b+1]-y)*t;z=-4;
                }
                x=projection[0]*x/-z;y=projection[1]*y/-z;
                minX=Math.min(minX,x);maxX=Math.max(maxX,x);minY=Math.min(minY,y);maxY=Math.max(maxY,y);
            }
        }
        var footprint=(Math.min(maxX,1)-Math.max(minX,-1))*(Math.min(maxY,1)-Math.max(minY,-1));
        if(minX>=1 || maxX<=-1 || minY>=1 || maxY<=-1 || !(footprint>.0001))continue;
        var score=footprint*(road?4:1);
        // Small batching/visibility changes must not alternate water heights.
        if(Math.abs(height-p.plane)<.25)score*=1.5;
        if(!p.candidate || score>p.candidate.score)p.candidate={height:height,score:score};
    }
});

void WebPlanar_Candidate(const Material *material, const float *positions,
                         int stride, int count, const unsigned short *indices, int indexCount,
                         int indexBase, const float *world, const float *view)
{
    if (s_drawing || !r_reflections || !r_reflections->current.enabled || !material ||
        !material->textures || !positions || !indices || !world || !view || count < 3 || indexCount < 3 || stride < 12) return;
    for (int i = 0; i < material->textureCount; ++i) {
        const MaterialTextureDef *def = &material->textures[i];
        if (def->semantic != 2 || !def->name || strcmp(def->name, "detailMap") || !def->u.image) continue;
        const char *name = def->u.image->name;
        if (!name) return;
        int road = !strcmp(name, "browser_remix/cobble_wet_d");
        if (road || !strcmp(name, "browser_remix/mud_d") || !strcmp(name, "browser_remix/track_d"))
            WebPlanar_AddCandidate(positions, stride, count, indices, indexCount, indexBase, world, view, road);
        return;
    }
}

EM_JS(void, WebPlanar_FinishFrame, (), {
    var fx=Module.cod2SceneFx, p=fx && fx.t && fx.t.planar;
    if (!p || !p.drew || !p.ok) return;
    var gl=fx.gl, active=gl.getParameter(gl.ACTIVE_TEXTURE), slot=p.slots[p.write];
    gl.activeTexture(gl.TEXTURE26); gl.bindTexture(gl.TEXTURE_2D,slot.color); gl.generateMipmap(gl.TEXTURE_2D);
    slot.valid=true; p.read=p.write; p.write^=1; p.drew=false;
    gl.activeTexture(active);
});

void WebPlanar_EndFrame(void) { WebPlanar_EndDraw(); WebPlanar_FinishFrame(); }
