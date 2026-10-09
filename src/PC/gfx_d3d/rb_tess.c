#include "common_types.h"
extern DxGlobals dx;
extern GfxBackEndData *backEndData;
extern dvar_t *r_rendererInUse;
extern const dvar_t *r_railCoreWidth;
extern struct DxState dxState;
#include <math.h>
#include "imports.h"
#include "bytematch.h"

#define RB_TESS_REGPARM2_ABI COD2_REGPARM(2) BM_NOINLINE
#define RB_TESS_REGPARM3_SSE_ABI COD2_REGPARM(3) COD2_SSEREGPARM BM_NOINLINE

extern void RB_EndSurface(void);
extern void RB_BeginSurface(const Material *material, MaterialTechniqueType techType, int lmapIndex);
extern void RB_ChangeIndices(IDirect3DIndexBuffer9 *ib);
extern void RB_ChangeStreamSource(int streamIndex, IDirect3DVertexBuffer9 *vb, int vertexOffset, int vertexStride);
extern void RB_DrawTechnique(MaterialVertexDeclType vertDeclType, const GfxDrawPrimArgs *args);
extern void RB_PushMatrixStack(void);
extern void RB_PopMatrixStack(void);
extern D3DMATRIX *RB_GetActiveWorldMatrix(void);
extern void RB_ChangedWorldMatrix(float worldScale);
extern void Com_Memcpy(void *dest, const void *src, int count);
extern int XSurfaceGetNumVerts(const XSurface *surface);
extern int XSurfaceGetNumTris(const XSurface *surface);
extern void XSurfaceGetTris(const XSurface *surface, r_index_t *dstIndices, int offset);
extern float Vec3Normalize(vec3_t v);
extern void Vec3Cross(const vec3_t v0, const vec3_t v1, vec3_t cross);
extern int VecNCompareCustomEpsilon(const vec_t *v0, const vec_t *v1, float epsilon, int coordCount);
extern void Vec3RotateTranspose(const vec_t *scaledWorldUp, const vec_t *viewAxis, vec_t *viewUp);
extern void MakeNormalVectors(const vec_t *forward, vec_t *right, vec_t *up);
extern float sinf(float x);
extern float cosf(float x);
extern float floorf(float x);

static inline char *RB_TessBase(void)
{
    return (char *)imp_tess;
}

void RB_TessBad(const surfaceType_t *surfType);
void RB_TessParticleCloud(const GfxEntity *re);
void RB_TessXModelRigid(const surfaceType_t *surfType);
#ifndef __EMSCRIPTEN__
static void RB_TESS_REGPARM3_SSE_ABI RB_AddQuadStampDx7(const vec_t *origin, const vec_t *left, const vec_t *up, int nativeColor, float s0, float t0, float s1, float t1);
#endif
#ifndef __EMSCRIPTEN__
static void RB_TESS_REGPARM3_SSE_ABI RB_AddQuadStamp(const vec_t *origin, const vec_t *left, const vec_t *up, int nativeColor, float s0, float t0, float s1, float t1);
#endif
static void RB_TESS_REGPARM3_SSE_ABI RB_AddQuadStampDx7_impl(const vec_t *origin, const vec_t *left, const vec_t *up, int nativeColor, float s0, float t0, float s1, float t1);
static void RB_TESS_REGPARM3_SSE_ABI RB_AddQuadStamp_impl(const vec_t *origin, const vec_t *left, const vec_t *up, int nativeColor, float s0, float t0, float s1, float t1);
static void RB_TESS_REGPARM2_ABI RB_BuildSprite_impl(const char *re, const float *worldRadius);
static void RB_TESS_REGPARM3_SSE_ABI RB_AddLineDx7_impl(const vec_t *start, const vec_t *end, float width, D3DCOLOR nativeColor, float s0, float t0, float s1, float t1);
static void RB_TESS_REGPARM3_SSE_ABI RB_AddLine_impl(const vec_t *start, const vec_t *end, float width, D3DCOLOR nativeColor, float s0, float t0, float s1, float t1);
#ifndef __EMSCRIPTEN__
static void RB_TESS_REGPARM2_ABI RB_BuildSprite(const char *re, const float *worldRadius);
#endif
#ifndef __EMSCRIPTEN__
static void RB_TESS_REGPARM3_SSE_ABI RB_AddLineDx7(const vec_t *start, const vec_t *end, float width, D3DCOLOR nativeColor, float s0, float t0, float s1, float t1);
#endif
#ifndef __EMSCRIPTEN__
static void RB_TESS_REGPARM3_SSE_ABI RB_AddLine(const vec_t *start, const vec_t *end, float width, D3DCOLOR nativeColor, float s0, float t0, float s1, float t1);
#endif
void RB_TessEntity(const GfxEntity *re);
void RB_TessBackEndEntity(const surfaceType_t *surfType);
void RB_TessPoly(const surfaceType_t *surfType);
void RB_TessStaticModelCached(const surfaceType_t *surfType);
void RB_TessXModelSkinned(const surfaceType_t *surfType);
void RB_TessTriangles(const surfaceType_t *surfType);

void RB_TessBad(const surfaceType_t *surfType)
{
}

void RB_TessParticleCloud(const GfxEntity *re)
{
    char *tess;
    char *backEnd;
    char *dxGlobals;
    GfxDrawPrimArgs args;
    float viewAxis[4];
    vec3_t worldUp, scaledWorldUp, viewUp;
    float localViewAxis[9];
    float *camAxis;
    float scale, invLen, len;
    float oneOver255 = 0.003921568859368563f;
    IDirect3DIndexBuffer9 *ib;
    IDirect3DVertexBuffer9 *vb;

    if (r_rendererInUse->current.integer == 2) {
        return;
    }

    tess = RB_TessBase();
    if (((materialCommands_t *)tess)->indexCount != 0 || ((materialCommands_t *)tess)->optimizedIndexCount != 0) {
        RB_EndSurface();
    }

    args.vertexCount = 0;
    args.primCount = 0;
    args.firstVertexFromBase = 0;
    args.u.buf.baseVertex = 0;
    args.u.buf.baseIndex = 0x800;

    if (re->radius[0] == re->radius[1] ||
        VecNCompareCustomEpsilon(re->origin, re->endpos, 0.001f, 3)) {

        viewAxis[0] = re->radius[0];
        viewAxis[1] = 0.0f;
        viewAxis[2] = 0.0f;
        viewAxis[3] = re->radius[1];
    } else {

        worldUp[0] = re->endpos[0] - re->origin[0];
        worldUp[1] = re->endpos[1] - re->origin[1];
        worldUp[2] = re->endpos[2] - re->origin[2];
        Vec3Normalize(worldUp);

        scale = re->radius[1];
        scaledWorldUp[0] = worldUp[0] * scale;
        scaledWorldUp[1] = worldUp[1] * scale;
        scaledWorldUp[2] = worldUp[2] * scale;

        backEnd = (char *)imp_backEnd;
        camAxis = (float *)((char *)&((r_backEndGlobals_t *)backEnd)->viewParms->viewMatrix);
        localViewAxis[0] = camAxis[0];
        localViewAxis[1] = camAxis[1];
        localViewAxis[2] = camAxis[2];
        localViewAxis[3] = camAxis[4];
        localViewAxis[4] = camAxis[5];
        localViewAxis[5] = camAxis[6];
        localViewAxis[6] = camAxis[8];
        localViewAxis[7] = camAxis[9];
        localViewAxis[8] = camAxis[10];

        Vec3RotateTranspose(scaledWorldUp, localViewAxis, viewUp);

        if (viewUp[0] < 0.001f && viewUp[1] < 0.001f) {

            viewAxis[0] = re->radius[0];
            viewAxis[1] = 0.0f;
            viewAxis[2] = 0.0f;
            viewAxis[3] = re->radius[1];
        } else {

            float vx = viewUp[0];
            float vy = viewUp[1];

            viewAxis[0] = vy;
            viewAxis[1] = -vx;
            viewAxis[2] = vx;
            viewAxis[3] = vy;

            len = __builtin_sqrtf(vy * vy + (-vx) * (-vx));

            scale = re->radius[0];
            invLen = scale / len;
            viewAxis[0] *= invLen;
            viewAxis[1] *= invLen;

            if (scale > len) {
                viewAxis[2] *= invLen;
                viewAxis[3] *= invLen;
            }
        }
    }

    backEnd = (char *)imp_backEnd;
    ((r_backEndGlobals_t *)backEnd)->codeConsts[49][0] = viewAxis[0];
    ((r_backEndGlobals_t *)backEnd)->codeConsts[49][1] = viewAxis[1];
    ((r_backEndGlobals_t *)backEnd)->codeConsts[49][2] = viewAxis[2];
    ((r_backEndGlobals_t *)backEnd)->codeConsts[49][3] = viewAxis[3];

    ((r_backEndGlobals_t *)backEnd)->codeConsts[48][0] = (float)re->materialRGBA[0] * oneOver255;
    ((r_backEndGlobals_t *)backEnd)->codeConsts[48][1] = (float)re->materialRGBA[1] * oneOver255;
    ((r_backEndGlobals_t *)backEnd)->codeConsts[48][2] = (float)re->materialRGBA[2] * oneOver255;
    ((r_backEndGlobals_t *)backEnd)->codeConsts[48][3] = (float)re->materialRGBA[3] * oneOver255;

    dxGlobals = (char *)imp_dx;
    ib = ((DxGlobals *)dxGlobals)->particleCloudIndexBuffer;
    if (ib != dxState.indexBuffer) {
        RB_ChangeIndices(ib);
    }

    vb = ((DxGlobals *)dxGlobals)->particleCloudVertexBuffer;
    if (vb != dxState.streams[0].vb ||
        dxState.streams[0].offset != 0 ||
        dxState.streams[0].stride != 0x14) {
        RB_ChangeStreamSource(0, vb, 0, 0x14);
    }

    RB_DrawTechnique(2, &args);
}

void RB_TessXModelRigid(const surfaceType_t *surfType)
{
    char *tess;
    XSurface *xsurf;
    GfxDrawPrimArgs args;
    IDirect3DVertexBuffer9 *vb;
    IDirect3DIndexBuffer9 *ib;
    int vertexStride;
    D3DMATRIX *worldMatrix;
    float *boneAxis;
    char *entity;

    tess = RB_TessBase();

    if (((materialCommands_t *)tess)->indexCount != 0 || ((materialCommands_t *)tess)->optimizedIndexCount != 0) {
        RB_EndSurface();
    }

    xsurf = *(XSurface **)((byte *)surfType + 4);

    args.firstVertexFromBase = 0;
    args.vertexCount = (int)xsurf->vertCount;
    args.primCount = (int)xsurf->triCount;
    args.u.buf.baseVertex = 0;
    args.u.buf.baseIndex = 0;

    ib = xsurf->indexBuffer;
    if (ib != dxState.indexBuffer) {
        RB_ChangeIndices(ib);
    }

    vb = xsurf->surfRigid.vb;
    if (r_rendererInUse->current.integer == 2) {
        vertexStride = 0x24;
    } else {
        vertexStride = 0x40;
    }

    if (vb != dxState.streams[0].vb ||
        dxState.streams[0].offset != 0 ||
        dxState.streams[0].stride != vertexStride) {
        RB_ChangeStreamSource(0, vb, 0, vertexStride);
    }

    RB_PushMatrixStack();

    entity = (char *)((r_backEndGlobals_t *)imp_backEnd)->currentEntity;
    boneAxis = (float *)((byte *)surfType + 8);

    worldMatrix = RB_GetActiveWorldMatrix();

    ((float *)worldMatrix)[0] = boneAxis[0];
    ((float *)worldMatrix)[1] = boneAxis[1];
    ((float *)worldMatrix)[2] = boneAxis[2];
    ((float *)worldMatrix)[3] = 0.0f;

    ((float *)worldMatrix)[4] = boneAxis[3];
    ((float *)worldMatrix)[5] = boneAxis[4];
    ((float *)worldMatrix)[6] = boneAxis[5];
    ((float *)worldMatrix)[7] = 0.0f;

    ((float *)worldMatrix)[8] = boneAxis[6];
    ((float *)worldMatrix)[9] = boneAxis[7];
    ((float *)worldMatrix)[10] = boneAxis[8];
    ((float *)worldMatrix)[11] = 0.0f;

    ((float *)worldMatrix)[12] = boneAxis[9];
    ((float *)worldMatrix)[13] = boneAxis[10];
    ((float *)worldMatrix)[14] = boneAxis[11];
    ((float *)worldMatrix)[15] = 1.0f;

    RB_ChangedWorldMatrix(((GfxEntity *)entity)->scale);

    RB_DrawTechnique(0, &args);
    RB_PopMatrixStack();
}

static void RB_TESS_REGPARM3_SSE_ABI RB_AddQuadStampDx7_impl(const vec_t *origin, const vec_t *left, const vec_t *up,
                                                             int nativeColor, float s0, float t0, float s1, float t1)
{
    char *tess, *backEnd, *viewParms;
    unsigned short *indices;
    char *v0, *v1, *v2, *v3;
    int vc, ic;
    float lx, ly, lz, ux, uy, uz, nx, ny, nz;
    float *from;
    void *savedMat;

    tess = RB_TessBase();

    if (((materialCommands_t *)tess)->vertexCount + 4 > 0x154a ||
        ((materialCommands_t *)tess)->indexCount + 6 > 0x100000) {
        savedMat = (*(void **)&((materialCommands_t *)tess)->declType);
        RB_EndSurface();
        RB_BeginSurface((*(void **)&((materialCommands_t *)tess)->material),
                        (*(int *)&((materialCommands_t *)tess)->techType),
                        ((materialCommands_t *)tess)->lmapIndex);
        if ((*(void **)&((materialCommands_t *)tess)->declType) != savedMat) {
            if (((materialCommands_t *)tess)->indexCount != 0 || ((materialCommands_t *)tess)->optimizedIndexCount != 0)
                RB_EndSurface();
            (*(void **)&((materialCommands_t *)tess)->declType) = savedMat;
        }
    }

    vc = ((materialCommands_t *)tess)->vertexCount;
    ic = ((materialCommands_t *)tess)->indexCount;
    indices = (*(unsigned short **)&((materialCommands_t *)tess)->indices);

    indices[ic + 0] = (unsigned short)vc;
    indices[ic + 1] = (unsigned short)(vc + 1);
    indices[ic + 2] = (unsigned short)(vc + 3);
    indices[ic + 3] = (unsigned short)(vc + 3);
    indices[ic + 4] = (unsigned short)(vc + 1);
    indices[ic + 5] = (unsigned short)(vc + 2);

    v0 = (char *)imp_tess + (unsigned short)vc * 36;
    v1 = v0 + 36;
    v2 = v0 + 72;
    v3 = v0 + 108;

    lx = left[0] + up[0];
    ly = left[1] + up[1];
    lz = left[2] + up[2];
    ux = left[0] - up[0];
    uy = left[1] - up[1];
    uz = left[2] - up[2];

    ((GfxVertexDx7 *)v0)->xyz[0] = lx + origin[0];
    ((GfxVertexDx7 *)v0)->xyz[1] = ly + origin[1];
    ((GfxVertexDx7 *)v0)->xyz[2] = lz + origin[2];

    ((GfxVertexDx7 *)v1)->xyz[0] = origin[0] - ux;
    ((GfxVertexDx7 *)v1)->xyz[1] = origin[1] - uy;
    ((GfxVertexDx7 *)v1)->xyz[2] = origin[2] - uz;

    ((GfxVertexDx7 *)v2)->xyz[0] = origin[0] - lx;
    ((GfxVertexDx7 *)v2)->xyz[1] = origin[1] - ly;
    ((GfxVertexDx7 *)v2)->xyz[2] = origin[2] - lz;

    ((GfxVertexDx7 *)v3)->xyz[0] = ux + origin[0];
    ((GfxVertexDx7 *)v3)->xyz[1] = uy + origin[1];
    ((GfxVertexDx7 *)v3)->xyz[2] = uz + origin[2];

    backEnd = (char *)imp_backEnd;
    viewParms = (char *)((r_backEndGlobals_t *)backEnd)->viewParms;
    from = ((GfxViewParms *)viewParms)->axis[0];
    nx = -from[0];
    ny = -from[1];
    nz = -from[2];

    ((GfxVertexDx7 *)v0)->normal[0] = nx;
    ((GfxVertexDx7 *)v0)->normal[1] = ny;
    ((GfxVertexDx7 *)v0)->normal[2] = nz;
    ((GfxVertexDx7 *)v1)->normal[0] = nx;
    ((GfxVertexDx7 *)v1)->normal[1] = ny;
    ((GfxVertexDx7 *)v1)->normal[2] = nz;
    ((GfxVertexDx7 *)v2)->normal[0] = nx;
    ((GfxVertexDx7 *)v2)->normal[1] = ny;
    ((GfxVertexDx7 *)v2)->normal[2] = nz;
    ((GfxVertexDx7 *)v3)->normal[0] = nx;
    ((GfxVertexDx7 *)v3)->normal[1] = ny;
    ((GfxVertexDx7 *)v3)->normal[2] = nz;

    (*(int *)&((GfxVertexDx7 *)v0)->color.packed) = nativeColor;
    (*(int *)&((GfxVertexDx7 *)v1)->color.packed) = nativeColor;
    (*(int *)&((GfxVertexDx7 *)v2)->color.packed) = nativeColor;
    (*(int *)&((GfxVertexDx7 *)v3)->color.packed) = nativeColor;

    ((GfxVertexDx7 *)v0)->texCoord[0] = s0;
    ((GfxVertexDx7 *)v0)->texCoord[1] = t0;
    ((GfxVertexDx7 *)v1)->texCoord[0] = s1;
    ((GfxVertexDx7 *)v1)->texCoord[1] = t0;
    ((GfxVertexDx7 *)v2)->texCoord[0] = s1;
    ((GfxVertexDx7 *)v2)->texCoord[1] = t1;
    ((GfxVertexDx7 *)v3)->texCoord[0] = s0;
    ((GfxVertexDx7 *)v3)->texCoord[1] = t1;

    ((materialCommands_t *)tess)->vertexCount = vc + 4;
    ((materialCommands_t *)tess)->indexCount = ic + 6;
}

#ifdef __EMSCRIPTEN__

static void RB_TESS_REGPARM3_SSE_ABI RB_AddQuadStampDx7(const vec_t *origin, const vec_t *left, const vec_t *up,
                                                        int nativeColor, float s0, float t0, float s1, float t1)
{
    RB_AddQuadStampDx7_impl(origin, left, up, nativeColor, s0, t0, s1, t1);
}
#else

static void RB_TESS_REGPARM3_SSE_ABI RB_AddQuadStampDx7(const vec_t *origin, const vec_t *left, const vec_t *up,
                                                        int nativeColor, float s0, float t0, float s1, float t1)
{
    RB_AddQuadStampDx7_impl(origin, left, up, nativeColor, s0, t0, s1, t1);
}

#endif

static void RB_TESS_REGPARM3_SSE_ABI RB_AddQuadStamp_impl(const vec_t *origin, const vec_t *left, const vec_t *up,
                                                          int nativeColor, float s0, float t0, float s1, float t1)
{
    char *tess, *backEnd, *viewParms;
    unsigned short *indices;
    char *v0, *v1, *v2, *v3;
    int vc, ic;
    float lx, ly, lz, ux, uy, uz, nx, ny, nz;
    float *from;
    void *savedMat;

    tess = RB_TessBase();

    if (((materialCommands_t *)tess)->vertexCount + 4 > 0x154a ||
        ((materialCommands_t *)tess)->indexCount + 6 > 0x100000) {
        savedMat = (*(void **)&((materialCommands_t *)tess)->declType);
        RB_EndSurface();
        RB_BeginSurface((*(void **)&((materialCommands_t *)tess)->material),
                        (*(int *)&((materialCommands_t *)tess)->techType),
                        ((materialCommands_t *)tess)->lmapIndex);
        if ((*(void **)&((materialCommands_t *)tess)->declType) != savedMat) {
            if (((materialCommands_t *)tess)->indexCount != 0 || ((materialCommands_t *)tess)->optimizedIndexCount != 0)
                RB_EndSurface();
            (*(void **)&((materialCommands_t *)tess)->declType) = savedMat;
        }
    }

    vc = ((materialCommands_t *)tess)->vertexCount;
    ic = ((materialCommands_t *)tess)->indexCount;
    indices = (*(unsigned short **)&((materialCommands_t *)tess)->indices);

    indices[ic + 0] = (unsigned short)vc;
    indices[ic + 1] = (unsigned short)(vc + 1);
    indices[ic + 2] = (unsigned short)(vc + 3);
    indices[ic + 3] = (unsigned short)(vc + 3);
    indices[ic + 4] = (unsigned short)(vc + 1);
    indices[ic + 5] = (unsigned short)(vc + 2);

    v0 = (char *)imp_tess + (unsigned short)vc * 64;
    v1 = v0 + 64;
    v2 = v0 + 128;
    v3 = v0 + 192;

    lx = left[0];
    ly = left[1];
    lz = left[2];
    ux = up[0];
    uy = up[1];
    uz = up[2];

    ((GfxVertex *)v0)->xyzw[0] = origin[0] + lx + ux;
    ((GfxVertex *)v0)->xyzw[1] = origin[1] + ly + uy;
    ((GfxVertex *)v0)->xyzw[2] = origin[2] + lz + uz;

    ((GfxVertex *)v1)->xyzw[0] = origin[0] - lx + ux;
    ((GfxVertex *)v1)->xyzw[1] = origin[1] - ly + uy;
    ((GfxVertex *)v1)->xyzw[2] = origin[2] - lz + uz;

    ((GfxVertex *)v2)->xyzw[0] = origin[0] - lx - ux;
    ((GfxVertex *)v2)->xyzw[1] = origin[1] - ly - uy;
    ((GfxVertex *)v2)->xyzw[2] = origin[2] - lz - uz;

    ((GfxVertex *)v3)->xyzw[0] = origin[0] + lx - ux;
    ((GfxVertex *)v3)->xyzw[1] = origin[1] + ly - uy;
    ((GfxVertex *)v3)->xyzw[2] = origin[2] + lz - uz;

    ((GfxVertex *)v0)->xyzw[3] = 1.0f;
    ((GfxVertex *)v1)->xyzw[3] = 1.0f;
    ((GfxVertex *)v2)->xyzw[3] = 1.0f;
    ((GfxVertex *)v3)->xyzw[3] = 1.0f;

    backEnd = (char *)imp_backEnd;
    viewParms = (char *)((r_backEndGlobals_t *)backEnd)->viewParms;
    from = ((GfxViewParms *)viewParms)->axis[0];
    nx = -from[0];
    ny = -from[1];
    nz = -from[2];

    ((GfxVertex *)v0)->normal[0] = nx;
    ((GfxVertex *)v0)->normal[1] = ny;
    ((GfxVertex *)v0)->normal[2] = nz;
    ((GfxVertex *)v1)->normal[0] = nx;
    ((GfxVertex *)v1)->normal[1] = ny;
    ((GfxVertex *)v1)->normal[2] = nz;
    ((GfxVertex *)v2)->normal[0] = nx;
    ((GfxVertex *)v2)->normal[1] = ny;
    ((GfxVertex *)v2)->normal[2] = nz;
    ((GfxVertex *)v3)->normal[0] = nx;
    ((GfxVertex *)v3)->normal[1] = ny;
    ((GfxVertex *)v3)->normal[2] = nz;

    (*(int *)&((GfxVertex *)v0)->color.packed) = nativeColor;
    (*(int *)&((GfxVertex *)v1)->color.packed) = nativeColor;
    (*(int *)&((GfxVertex *)v2)->color.packed) = nativeColor;
    (*(int *)&((GfxVertex *)v3)->color.packed) = nativeColor;

    ((GfxVertex *)v0)->texCoord[0] = s0;
    ((GfxVertex *)v0)->texCoord[1] = t0;
    ((GfxVertex *)v1)->texCoord[0] = s1;
    ((GfxVertex *)v1)->texCoord[1] = t0;
    ((GfxVertex *)v2)->texCoord[0] = s1;
    ((GfxVertex *)v2)->texCoord[1] = t1;
    ((GfxVertex *)v3)->texCoord[0] = s0;
    ((GfxVertex *)v3)->texCoord[1] = t1;

    ((GfxVertex *)v0)->binormal[0] = up[0];
    ((GfxVertex *)v0)->binormal[1] = up[1];
    ((GfxVertex *)v0)->binormal[2] = up[2];
    ((GfxVertex *)v1)->binormal[0] = up[0];
    ((GfxVertex *)v1)->binormal[1] = up[1];
    ((GfxVertex *)v1)->binormal[2] = up[2];
    ((GfxVertex *)v2)->binormal[0] = up[0];
    ((GfxVertex *)v2)->binormal[1] = up[1];
    ((GfxVertex *)v2)->binormal[2] = up[2];
    ((GfxVertex *)v3)->binormal[0] = up[0];
    ((GfxVertex *)v3)->binormal[1] = up[1];
    ((GfxVertex *)v3)->binormal[2] = up[2];

    ((GfxVertex *)v0)->tangent[0] = -left[0];
    ((GfxVertex *)v0)->tangent[1] = -left[1];
    ((GfxVertex *)v0)->tangent[2] = -left[2];
    ((GfxVertex *)v1)->tangent[0] = -left[0];
    ((GfxVertex *)v1)->tangent[1] = -left[1];
    ((GfxVertex *)v1)->tangent[2] = -left[2];
    ((GfxVertex *)v2)->tangent[0] = -left[0];
    ((GfxVertex *)v2)->tangent[1] = -left[1];
    ((GfxVertex *)v2)->tangent[2] = -left[2];
    ((GfxVertex *)v3)->tangent[0] = -left[0];
    ((GfxVertex *)v3)->tangent[1] = -left[1];
    ((GfxVertex *)v3)->tangent[2] = -left[2];

    ((materialCommands_t *)tess)->vertexCount = vc + 4;
    ((materialCommands_t *)tess)->indexCount = ic + 6;
}

#ifdef __EMSCRIPTEN__

static void RB_TESS_REGPARM3_SSE_ABI RB_AddQuadStamp(const vec_t *origin, const vec_t *left, const vec_t *up,
                                                     int nativeColor, float s0, float t0, float s1, float t1)
{
    RB_AddQuadStamp_impl(origin, left, up, nativeColor, s0, t0, s1, t1);
}
#else

static void RB_TESS_REGPARM3_SSE_ABI RB_AddQuadStamp(const vec_t *origin, const vec_t *left, const vec_t *up,
                                                     int nativeColor, float s0, float t0, float s1, float t1)
{
    RB_AddQuadStamp_impl(origin, left, up, nativeColor, s0, t0, s1, t1);
}

#endif

static void RB_TESS_REGPARM2_ABI RB_BuildSprite_impl(const char *re, const float *worldRadius)
{
    float worldOrigin[3], left[3], up[3];
    float scale, angle, sinA, cosA;
    int nativeColor;
    int ncols, nrows, totalFrames, frame, row, col;
    float s0, t0, s1, t1;
    char *backEnd, *viewParms;
    char *material;

    worldOrigin[0] = ((GfxEntity *)re)->origin[0];
    worldOrigin[1] = ((GfxEntity *)re)->origin[1];
    worldOrigin[2] = ((GfxEntity *)re)->origin[2];

    if (*(unsigned char *)(re + 5) & 0x10) {
        worldOrigin[2] += worldRadius[1];
    }

    angle = ((GfxEntity *)re)->rotation;

    backEnd = (char *)imp_backEnd;
    viewParms = (char *)((r_backEndGlobals_t *)backEnd)->viewParms;

    if (angle == 0.0f) {

        scale = worldRadius[0];
        left[0] = ((GfxViewParms *)viewParms)->axis[1][0] * scale;
        left[1] = ((GfxViewParms *)viewParms)->axis[1][1] * scale;
        left[2] = ((GfxViewParms *)viewParms)->axis[1][2] * scale;
        scale = worldRadius[1];
        up[0] = ((GfxViewParms *)viewParms)->axis[2][0] * scale;
        up[1] = ((GfxViewParms *)viewParms)->axis[2][1] * scale;
        up[2] = ((GfxViewParms *)viewParms)->axis[2][2] * scale;
    } else {

        float radians = (float)((double)angle * 0.017453292519943295);
        sinA = sinf(radians);
        cosA = cosf(radians);

        left[0] = cosA * worldRadius[0] * ((GfxViewParms *)viewParms)->axis[1][0] + (-sinA) * worldRadius[0] * ((GfxViewParms *)viewParms)->axis[2][0];
        left[1] = cosA * worldRadius[0] * ((GfxViewParms *)viewParms)->axis[1][1] + (-sinA) * worldRadius[0] * ((GfxViewParms *)viewParms)->axis[2][1];
        left[2] = cosA * worldRadius[0] * ((GfxViewParms *)viewParms)->axis[1][2] + (-sinA) * worldRadius[0] * ((GfxViewParms *)viewParms)->axis[2][2];

        up[0] = cosA * worldRadius[1] * ((GfxViewParms *)viewParms)->axis[2][0] + sinA * worldRadius[1] * ((GfxViewParms *)viewParms)->axis[1][0];
        up[1] = cosA * worldRadius[1] * ((GfxViewParms *)viewParms)->axis[2][1] + sinA * worldRadius[1] * ((GfxViewParms *)viewParms)->axis[1][1];
        up[2] = cosA * worldRadius[1] * ((GfxViewParms *)viewParms)->axis[2][2] + sinA * worldRadius[1] * ((GfxViewParms *)viewParms)->axis[1][2];
    }

    memcpy(&nativeColor, ((GfxEntity *)re)->materialRGBA, sizeof(nativeColor));

    material = (*(char **)&((GfxEntity *)re)->customMaterial);
    if (!material)
        return;

    ncols = ((Material *)material)->info.textureAtlasRowCount;
    nrows = ((Material *)material)->info.textureAtlasColumnCount;
    if (!ncols)
        ncols = 1;
    if (!nrows)
        nrows = 1;
    totalFrames = ncols * nrows;

    if (totalFrames == 1) {
        s0 = 0.0f;
        t0 = 0.0f;
        s1 = 1.0f;
        t1 = 1.0f;
    } else {
        float inv_nrows = 1.0f / (float)nrows;
        float inv_ncols = 1.0f / (float)ncols;
        frame = ((GfxEntity *)re)->materialSubimageIndex;
        row = frame / nrows;
        col = frame % nrows;
        s0 = (float)col * inv_nrows;
        t0 = (float)row * inv_ncols;
        s1 = s0 + inv_nrows;
        t1 = t0 + inv_ncols;
    }

    if (r_rendererInUse->current.integer == 2) {
        RB_AddQuadStampDx7_impl(worldOrigin, left, up, nativeColor, s0, t0, s1, t1);
    } else {
        RB_AddQuadStamp_impl(worldOrigin, left, up, nativeColor, s0, t0, s1, t1);
    }
}

#ifdef __EMSCRIPTEN__

static void RB_TESS_REGPARM2_ABI RB_BuildSprite(const char *re, const float *worldRadius)
{
    RB_BuildSprite_impl(re, worldRadius);
}
#else

static void RB_TESS_REGPARM2_ABI RB_BuildSprite(const char *re, const float *worldRadius)
{
    RB_BuildSprite_impl(re, worldRadius);
}

#endif

static void RB_TESS_REGPARM3_SSE_ABI RB_AddLineDx7_impl(const vec_t *start, const vec_t *end, float width,
                                                        D3DCOLOR nativeColor, float s0, float t0, float s1, float t1)
{
    char *tess = RB_TessBase();
    char *backEnd = (char *)imp_backEnd;
    void *savedMat;
    int vc, ic;
    unsigned short *indices;
    char *v0, *v1, *v2, *v3;
    float v1v[3], v2v[3], tangent[3], halfTangent[3], binormal[3], normal[3];
    float *viewOrigin;

    viewOrigin = (float *)((char *)((r_backEndGlobals_t *)backEnd)->viewParms);
    v1v[0] = start[0] - viewOrigin[0];
    v1v[1] = start[1] - viewOrigin[1];
    v1v[2] = start[2] - viewOrigin[2];
    v2v[0] = end[0] - viewOrigin[0];
    v2v[1] = end[1] - viewOrigin[1];
    v2v[2] = end[2] - viewOrigin[2];
    Vec3Cross(v1v, v2v, tangent);
    Vec3Normalize(tangent);

    halfTangent[0] = tangent[0] * width;
    halfTangent[1] = tangent[1] * width;
    halfTangent[2] = tangent[2] * width;

    binormal[0] = end[0] - start[0];
    binormal[1] = end[1] - start[1];
    binormal[2] = end[2] - start[2];
    Vec3Normalize(binormal);

    Vec3Cross(tangent, binormal, normal);

    if (((materialCommands_t *)tess)->vertexCount + 4 > 0x154a || ((materialCommands_t *)tess)->indexCount + 6 > 0x100000) {
        savedMat = (*(void **)&((materialCommands_t *)tess)->declType);
        RB_EndSurface();
        RB_BeginSurface((*(void **)&((materialCommands_t *)tess)->material), (*(int *)&((materialCommands_t *)tess)->techType), ((materialCommands_t *)tess)->lmapIndex);
        if ((*(void **)&((materialCommands_t *)tess)->declType) != savedMat) {
            if (((materialCommands_t *)tess)->indexCount != 0 || ((materialCommands_t *)tess)->optimizedIndexCount != 0)
                RB_EndSurface();
            (*(void **)&((materialCommands_t *)tess)->declType) = savedMat;
        }
    }

    vc = ((materialCommands_t *)tess)->vertexCount;
    ic = ((materialCommands_t *)tess)->indexCount;
    indices = (*(unsigned short **)&((materialCommands_t *)tess)->indices);

    indices[ic + 0] = (unsigned short)vc;
    indices[ic + 1] = (unsigned short)(vc + 1);
    indices[ic + 2] = (unsigned short)(vc + 2);
    indices[ic + 3] = (unsigned short)(vc + 2);
    indices[ic + 4] = (unsigned short)(vc + 1);
    indices[ic + 5] = (unsigned short)(vc + 3);

    v0 = tess + (unsigned short)vc * 36;
    v1 = v0 + 36;
    v2 = v0 + 72;
    v3 = v0 + 108;

    ((GfxVertexDx7 *)v0)->xyz[0] = start[0] + halfTangent[0];
    ((GfxVertexDx7 *)v0)->xyz[1] = start[1] + halfTangent[1];
    ((GfxVertexDx7 *)v0)->xyz[2] = start[2] + halfTangent[2];
    ((GfxVertexDx7 *)v1)->xyz[0] = start[0] - halfTangent[0];
    ((GfxVertexDx7 *)v1)->xyz[1] = start[1] - halfTangent[1];
    ((GfxVertexDx7 *)v1)->xyz[2] = start[2] - halfTangent[2];
    ((GfxVertexDx7 *)v2)->xyz[0] = end[0] + halfTangent[0];
    ((GfxVertexDx7 *)v2)->xyz[1] = end[1] + halfTangent[1];
    ((GfxVertexDx7 *)v2)->xyz[2] = end[2] + halfTangent[2];
    ((GfxVertexDx7 *)v3)->xyz[0] = end[0] - halfTangent[0];
    ((GfxVertexDx7 *)v3)->xyz[1] = end[1] - halfTangent[1];
    ((GfxVertexDx7 *)v3)->xyz[2] = end[2] - halfTangent[2];

    ((GfxVertexDx7 *)v0)->normal[0] = normal[0];
    ((GfxVertexDx7 *)v0)->normal[1] = normal[1];
    ((GfxVertexDx7 *)v0)->normal[2] = normal[2];
    ((GfxVertexDx7 *)v1)->normal[0] = normal[0];
    ((GfxVertexDx7 *)v1)->normal[1] = normal[1];
    ((GfxVertexDx7 *)v1)->normal[2] = normal[2];
    ((GfxVertexDx7 *)v2)->normal[0] = normal[0];
    ((GfxVertexDx7 *)v2)->normal[1] = normal[1];
    ((GfxVertexDx7 *)v2)->normal[2] = normal[2];
    ((GfxVertexDx7 *)v3)->normal[0] = normal[0];
    ((GfxVertexDx7 *)v3)->normal[1] = normal[1];
    ((GfxVertexDx7 *)v3)->normal[2] = normal[2];

    (*(int *)&((GfxVertexDx7 *)v0)->color.packed) = nativeColor;
    (*(int *)&((GfxVertexDx7 *)v1)->color.packed) = nativeColor;
    (*(int *)&((GfxVertexDx7 *)v2)->color.packed) = nativeColor;
    (*(int *)&((GfxVertexDx7 *)v3)->color.packed) = nativeColor;

    ((GfxVertexDx7 *)v0)->texCoord[0] = s0;
    ((GfxVertexDx7 *)v0)->texCoord[1] = t0;
    ((GfxVertexDx7 *)v1)->texCoord[0] = s1;
    ((GfxVertexDx7 *)v1)->texCoord[1] = t0;
    ((GfxVertexDx7 *)v2)->texCoord[0] = s0;
    ((GfxVertexDx7 *)v2)->texCoord[1] = t1;
    ((GfxVertexDx7 *)v3)->texCoord[0] = s1;
    ((GfxVertexDx7 *)v3)->texCoord[1] = t1;

    ((materialCommands_t *)tess)->vertexCount = vc + 4;
    ((materialCommands_t *)tess)->indexCount = ic + 6;
}

#ifdef __EMSCRIPTEN__

static void RB_TESS_REGPARM3_SSE_ABI RB_AddLineDx7(const vec_t *start, const vec_t *end, float width,
                                                   D3DCOLOR nativeColor, float s0, float t0, float s1, float t1)
{
    RB_AddLineDx7_impl(start, end, width, nativeColor, s0, t0, s1, t1);
}
#else

static void RB_TESS_REGPARM3_SSE_ABI RB_AddLineDx7(const vec_t *start, const vec_t *end, float width,
                                                   D3DCOLOR nativeColor, float s0, float t0, float s1, float t1)
{
    RB_AddLineDx7_impl(start, end, width, nativeColor, s0, t0, s1, t1);
}

#endif

static void RB_TESS_REGPARM3_SSE_ABI RB_AddLine_impl(const vec_t *start, const vec_t *end, float width,
                                                     D3DCOLOR nativeColor, float s0, float t0, float s1, float t1)
{
    char *tess = RB_TessBase();
    char *backEnd = (char *)imp_backEnd;
    void *savedMat;
    int vc, ic;
    unsigned short *indices;
    char *v0, *v1, *v2, *v3;
    float v1v[3], v2v[3], tangent[3], halfTangent[3], binormal[3], normal[3];
    float *viewOrigin;

    if (((materialCommands_t *)tess)->vertexCount + 4 > 0x154a ||
        ((materialCommands_t *)tess)->indexCount + 6 > 0x100000) {
        savedMat = (*(void **)&((materialCommands_t *)tess)->declType);
        RB_EndSurface();
        RB_BeginSurface((*(void **)&((materialCommands_t *)tess)->material), (*(int *)&((materialCommands_t *)tess)->techType), ((materialCommands_t *)tess)->lmapIndex);
        if ((*(void **)&((materialCommands_t *)tess)->declType) != savedMat) {
            if (((materialCommands_t *)tess)->indexCount != 0 || ((materialCommands_t *)tess)->optimizedIndexCount != 0)
                RB_EndSurface();
            (*(void **)&((materialCommands_t *)tess)->declType) = savedMat;
        }
    }

    viewOrigin = (float *)((char *)((r_backEndGlobals_t *)backEnd)->viewParms);
    v1v[0] = start[0] - viewOrigin[0];
    v1v[1] = start[1] - viewOrigin[1];
    v1v[2] = start[2] - viewOrigin[2];
    v2v[0] = end[0] - viewOrigin[0];
    v2v[1] = end[1] - viewOrigin[1];
    v2v[2] = end[2] - viewOrigin[2];
    Vec3Cross(v1v, v2v, tangent);
    Vec3Normalize(tangent);

    halfTangent[0] = tangent[0] * width;
    halfTangent[1] = tangent[1] * width;
    halfTangent[2] = tangent[2] * width;

    binormal[0] = end[0] - start[0];
    binormal[1] = end[1] - start[1];
    binormal[2] = end[2] - start[2];
    Vec3Normalize(binormal);

    Vec3Cross(tangent, binormal, normal);

    vc = ((materialCommands_t *)tess)->vertexCount;
    ic = ((materialCommands_t *)tess)->indexCount;
    indices = (*(unsigned short **)&((materialCommands_t *)tess)->indices);

    indices[ic + 0] = (unsigned short)vc;
    indices[ic + 1] = (unsigned short)(vc + 1);
    indices[ic + 2] = (unsigned short)(vc + 2);
    indices[ic + 3] = (unsigned short)(vc + 2);
    indices[ic + 4] = (unsigned short)(vc + 1);
    indices[ic + 5] = (unsigned short)(vc + 3);

    v0 = tess + (unsigned short)vc * 64;
    v1 = v0 + 64;
    v2 = v0 + 128;
    v3 = v0 + 192;

    ((GfxVertex *)v0)->xyzw[0] = start[0] + halfTangent[0];
    ((GfxVertex *)v0)->xyzw[1] = start[1] + halfTangent[1];
    ((GfxVertex *)v0)->xyzw[2] = start[2] + halfTangent[2];
    ((GfxVertex *)v1)->xyzw[0] = start[0] - halfTangent[0];
    ((GfxVertex *)v1)->xyzw[1] = start[1] - halfTangent[1];
    ((GfxVertex *)v1)->xyzw[2] = start[2] - halfTangent[2];
    ((GfxVertex *)v2)->xyzw[0] = end[0] + halfTangent[0];
    ((GfxVertex *)v2)->xyzw[1] = end[1] + halfTangent[1];
    ((GfxVertex *)v2)->xyzw[2] = end[2] + halfTangent[2];
    ((GfxVertex *)v3)->xyzw[0] = end[0] - halfTangent[0];
    ((GfxVertex *)v3)->xyzw[1] = end[1] - halfTangent[1];
    ((GfxVertex *)v3)->xyzw[2] = end[2] - halfTangent[2];

    ((GfxVertex *)v0)->xyzw[3] = 1.0f;
    ((GfxVertex *)v1)->xyzw[3] = 1.0f;
    ((GfxVertex *)v2)->xyzw[3] = 1.0f;
    ((GfxVertex *)v3)->xyzw[3] = 1.0f;

    ((GfxVertex *)v0)->normal[0] = normal[0];
    ((GfxVertex *)v0)->normal[1] = normal[1];
    ((GfxVertex *)v0)->normal[2] = normal[2];
    ((GfxVertex *)v1)->normal[0] = normal[0];
    ((GfxVertex *)v1)->normal[1] = normal[1];
    ((GfxVertex *)v1)->normal[2] = normal[2];
    ((GfxVertex *)v2)->normal[0] = normal[0];
    ((GfxVertex *)v2)->normal[1] = normal[1];
    ((GfxVertex *)v2)->normal[2] = normal[2];
    ((GfxVertex *)v3)->normal[0] = normal[0];
    ((GfxVertex *)v3)->normal[1] = normal[1];
    ((GfxVertex *)v3)->normal[2] = normal[2];

    (*(int *)&((GfxVertex *)v0)->color.packed) = nativeColor;
    (*(int *)&((GfxVertex *)v1)->color.packed) = nativeColor;
    (*(int *)&((GfxVertex *)v2)->color.packed) = nativeColor;
    (*(int *)&((GfxVertex *)v3)->color.packed) = nativeColor;

    ((GfxVertex *)v0)->texCoord[0] = s0;
    ((GfxVertex *)v0)->texCoord[1] = t0;
    ((GfxVertex *)v1)->texCoord[0] = s1;
    ((GfxVertex *)v1)->texCoord[1] = t0;
    ((GfxVertex *)v2)->texCoord[0] = s0;
    ((GfxVertex *)v2)->texCoord[1] = t1;
    ((GfxVertex *)v3)->texCoord[0] = s1;
    ((GfxVertex *)v3)->texCoord[1] = t1;

    ((GfxVertex *)v0)->binormal[0] = binormal[0];
    ((GfxVertex *)v0)->binormal[1] = binormal[1];
    ((GfxVertex *)v0)->binormal[2] = binormal[2];
    ((GfxVertex *)v1)->binormal[0] = binormal[0];
    ((GfxVertex *)v1)->binormal[1] = binormal[1];
    ((GfxVertex *)v1)->binormal[2] = binormal[2];
    ((GfxVertex *)v2)->binormal[0] = binormal[0];
    ((GfxVertex *)v2)->binormal[1] = binormal[1];
    ((GfxVertex *)v2)->binormal[2] = binormal[2];
    ((GfxVertex *)v3)->binormal[0] = binormal[0];
    ((GfxVertex *)v3)->binormal[1] = binormal[1];
    ((GfxVertex *)v3)->binormal[2] = binormal[2];

    ((GfxVertex *)v0)->tangent[0] = tangent[0];
    ((GfxVertex *)v0)->tangent[1] = tangent[1];
    ((GfxVertex *)v0)->tangent[2] = tangent[2];
    ((GfxVertex *)v1)->tangent[0] = tangent[0];
    ((GfxVertex *)v1)->tangent[1] = tangent[1];
    ((GfxVertex *)v1)->tangent[2] = tangent[2];
    ((GfxVertex *)v2)->tangent[0] = tangent[0];
    ((GfxVertex *)v2)->tangent[1] = tangent[1];
    ((GfxVertex *)v2)->tangent[2] = tangent[2];
    ((GfxVertex *)v3)->tangent[0] = tangent[0];
    ((GfxVertex *)v3)->tangent[1] = tangent[1];
    ((GfxVertex *)v3)->tangent[2] = tangent[2];

    ((materialCommands_t *)tess)->vertexCount = vc + 4;
    ((materialCommands_t *)tess)->indexCount = ic + 6;
}

#ifdef __EMSCRIPTEN__

static void RB_TESS_REGPARM3_SSE_ABI RB_AddLine(const vec_t *start, const vec_t *end, float width,
                                                D3DCOLOR nativeColor, float s0, float t0, float s1, float t1)
{
    RB_AddLine_impl(start, end, width, nativeColor, s0, t0, s1, t1);
}
#else

static void RB_TESS_REGPARM3_SSE_ABI RB_AddLine(const vec_t *start, const vec_t *end, float width,
                                                D3DCOLOR nativeColor, float s0, float t0, float s1, float t1)
{
    RB_AddLine_impl(start, end, width, nativeColor, s0, t0, s1, t1);
}

#endif

void RB_TessBackEndEntity(const surfaceType_t *surfType)
{
    RB_TessEntity((const GfxEntity *)((r_backEndGlobals_t *)imp_backEnd)->currentEntity);
}

void RB_TessPoly(const surfaceType_t *surfType)
{
    char *tess;
    int sortedIndex;
    int vertCount;
    int indexCount;
    int vertBase;
    int i;
    int triCount;
    char *src;
    char *dest;
    unsigned short *indices;

    tess = RB_TessBase();

    if ((*(int *)&((materialCommands_t *)tess)->declType) != 1) {
        if (((materialCommands_t *)tess)->indexCount != 0 || ((materialCommands_t *)tess)->optimizedIndexCount != 0) {
            RB_EndSurface();
        }
        tess = RB_TessBase();
        (*(int *)&((materialCommands_t *)tess)->declType) = 1;
    }

    vertCount = (int)*(unsigned short *)((byte *)surfType + 0xa);
    indexCount = vertCount * 3 - 6;

    if (vertCount + ((materialCommands_t *)tess)->vertexCount > 0x154a ||
        indexCount + ((materialCommands_t *)tess)->indexCount > 0x100000) {
        sortedIndex = (*(int *)&((materialCommands_t *)tess)->declType);
        RB_EndSurface();
        tess = RB_TessBase();
        RB_BeginSurface(
            ((materialCommands_t *)tess)->material,
            (*(MaterialTechniqueType *)&((materialCommands_t *)tess)->techType),
            ((materialCommands_t *)tess)->lmapIndex);
        tess = RB_TessBase();
        if (sortedIndex != (*(int *)&((materialCommands_t *)tess)->declType)) {
            if (((materialCommands_t *)tess)->indexCount != 0 || ((materialCommands_t *)tess)->optimizedIndexCount != 0) {
                RB_EndSurface();
            }
            tess = RB_TessBase();
            (*(int *)&((materialCommands_t *)tess)->declType) = sortedIndex;
            vertCount = (int)*(unsigned short *)((byte *)surfType + 0xa);
        }
    }

    if (r_rendererInUse->current.integer == 2) {

        src = *(char **)((byte *)surfType + 0xc);
        for (i = 0; i < vertCount; i++) {
            char *srcVert = src + i * 0x44;
            tess = RB_TessBase();
            vertBase = ((materialCommands_t *)tess)->vertexCount;
            dest = tess + (vertBase + i) * 32;

            *(int *)(dest + 0) = *(int *)(srcVert + 0);
            *(int *)(dest + 4) = *(int *)(srcVert + 4);
            *(int *)(dest + 8) = *(int *)(srcVert + 8);

            *(int *)(dest + 0xc) = *(int *)(srcVert + 0x18);

            *(int *)(dest + 0x10) = *(int *)(srcVert + 0x1c);
            *(int *)(dest + 0x14) = *(int *)(srcVert + 0x20);

            *(int *)(dest + 0x18) = *(int *)(srcVert + 0x24);
            *(int *)(dest + 0x1c) = *(int *)(srcVert + 0x28);
        }
    } else {

        tess = RB_TessBase();
        vertBase = ((materialCommands_t *)tess)->vertexCount;
        dest = tess + vertBase * 68;
        src = *(char **)((byte *)surfType + 0xc);
        memcpy(dest, src, vertCount * 68);
    }

    triCount = vertCount - 2;
    if (triCount > 0) {
        tess = RB_TessBase();
        for (i = 0; i < triCount; i++) {
            int idxOff = ((materialCommands_t *)tess)->indexCount;
            indices = (unsigned short *)((*(char **)&((materialCommands_t *)tess)->indices) + idxOff * 2);
            vertBase = (*(unsigned short *)&((materialCommands_t *)tess)->vertexCount);

            indices[0] = (unsigned short)vertBase;
            indices[1] = (unsigned short)(i + vertBase + 1);
            indices[2] = (unsigned short)(i + vertBase + 2);
            ((materialCommands_t *)tess)->indexCount += 3;
        }
    }

    vertCount = (int)*(unsigned short *)((byte *)surfType + 0xa);
    tess = RB_TessBase();
    ((materialCommands_t *)tess)->vertexCount += vertCount;
}

void RB_TessStaticModelCached(const surfaceType_t *surfType)
{
    char *tess;
    int triIndexCount;
    int sortedIndex;
    int baseVertIndex;
    char *dest;
    char *src;

    triIndexCount = (int)(*(short *)((byte *)*(void **)((byte *)surfType + 4) + 4)) * 3;

    tess = RB_TessBase();

    if (triIndexCount + ((materialCommands_t *)tess)->optimizedIndexCount > 0x100000) {
        sortedIndex = (*(int *)&((materialCommands_t *)tess)->declType);
        RB_EndSurface();
        RB_BeginSurface(
            ((materialCommands_t *)tess)->material,
            (*(MaterialTechniqueType *)&((materialCommands_t *)tess)->techType),
            ((materialCommands_t *)tess)->lmapIndex);
        tess = RB_TessBase();
        if (sortedIndex != (*(int *)&((materialCommands_t *)tess)->declType)) {
            if (((materialCommands_t *)tess)->indexCount != 0 || ((materialCommands_t *)tess)->optimizedIndexCount != 0) {
                RB_EndSurface();
                tess = RB_TessBase();
            }
            (*(int *)&((materialCommands_t *)tess)->declType) = sortedIndex;
        }
    }

    (*(int *)&((materialCommands_t *)tess)->optimizedVertexSource) = 2;
    ((materialCommands_t *)tess)->firstOptimizedVertex = 0;
    ((materialCommands_t *)tess)->optimizedVertexCount = 0x10000;

    baseVertIndex = ((materialCommands_t *)tess)->optimizedIndexCount;
    dest = (*(char **)&((materialCommands_t *)tess)->optimizedIndices) + baseVertIndex * 2;
    ((materialCommands_t *)tess)->optimizedIndexCount = baseVertIndex + triIndexCount;

    src = (char *)dx.smodelCacheIndices + *(int *)(*(void **)((byte *)surfType + 8)) * 12;
    Com_Memcpy(dest, src, triIndexCount * 2);
}

void RB_TessXModelSkinned(const surfaceType_t *surfType)
{
    GfxModelSkinnedSurface *skinSurf = (GfxModelSkinnedSurface *)surfType;
    XSurface *xsurf;
    char *tess;
    int vertexStride;
    GfxDrawPrimArgs args;
    IDirect3DVertexBuffer9 *vb;
    int sortedIndex;
    int vertexCount;
    int triIndexCount;
    int vertBase;
    int isDx7;

    xsurf = skinSurf->surf.xsurf;

    if (skinSurf->skinnedCachedOffset >= 0) {

        tess = RB_TessBase();

        if (((materialCommands_t *)tess)->indexCount != 0 || ((materialCommands_t *)tess)->optimizedIndexCount != 0) {
            RB_EndSurface();
        }

        isDx7 = (r_rendererInUse->current.integer == 2);
        vertexStride = isDx7 ? 0x24 : 0x40;

        args.u.buf.baseIndex = 0;
        args.u.buf.baseVertex = skinSurf->skinnedCachedOffset / vertexStride;
        args.firstVertexFromBase = 0;
        args.vertexCount = (int)xsurf->vertCount;
        args.primCount = (int)xsurf->triCount;

        if (xsurf->indexBuffer != dxState.indexBuffer) {
            RB_ChangeIndices(xsurf->indexBuffer);
        }

        {
            vb = (IDirect3DVertexBuffer9 *)backEndData->skinnedCacheVb->buffer;
        }

        if (vb != dxState.streams[0].vb ||
            dxState.streams[0].offset != 0 ||
            dxState.streams[0].stride != vertexStride) {
            RB_ChangeStreamSource(0, vb, 0, vertexStride);
        }

        RB_DrawTechnique(0, &args);
    } else {

        vertexCount = XSurfaceGetNumVerts(xsurf);
        triIndexCount = XSurfaceGetNumTris(xsurf) * 3;

        tess = RB_TessBase();
        if (vertexCount + ((materialCommands_t *)tess)->vertexCount > 0x154a ||
            triIndexCount + ((materialCommands_t *)tess)->indexCount > 0x100000) {
            sortedIndex = (*(int *)&((materialCommands_t *)tess)->declType);
            RB_EndSurface();
            tess = RB_TessBase();
            RB_BeginSurface(
                ((materialCommands_t *)tess)->material,
                (*(MaterialTechniqueType *)&((materialCommands_t *)tess)->techType),
                ((materialCommands_t *)tess)->lmapIndex);
            tess = RB_TessBase();
            if (sortedIndex != (*(int *)&((materialCommands_t *)tess)->declType)) {
                if (((materialCommands_t *)tess)->indexCount != 0 || ((materialCommands_t *)tess)->optimizedIndexCount != 0) {
                    RB_EndSurface();
                }
                tess = RB_TessBase();
                (*(int *)&((materialCommands_t *)tess)->declType) = sortedIndex;
            }
        }

        if (((materialCommands_t *)tess)->indexCount & 1) {
            sortedIndex = (*(int *)&((materialCommands_t *)tess)->declType);
            RB_EndSurface();
            tess = RB_TessBase();
            RB_BeginSurface(
                ((materialCommands_t *)tess)->material,
                (*(MaterialTechniqueType *)&((materialCommands_t *)tess)->techType),
                ((materialCommands_t *)tess)->lmapIndex);
            tess = RB_TessBase();
            if (sortedIndex != (*(int *)&((materialCommands_t *)tess)->declType)) {
                if (((materialCommands_t *)tess)->indexCount != 0 || ((materialCommands_t *)tess)->optimizedIndexCount != 0) {
                    RB_EndSurface();
                }
                tess = RB_TessBase();
                (*(int *)&((materialCommands_t *)tess)->declType) = sortedIndex;
            }
        }

        vertBase = (*(unsigned short *)&((materialCommands_t *)tess)->vertexCount);

        if (r_rendererInUse->current.integer == 2) {

            Com_Memcpy(tess + vertBase * 36, (void *)skinSurf->skinnedVert.variant, vertexCount * 36);
        } else {

            Com_Memcpy(tess + vertBase * 64, (void *)skinSurf->skinnedVert.variant, vertexCount * 64);
        }

        tess = RB_TessBase();
        ((materialCommands_t *)tess)->vertexCount += vertexCount;

        XSurfaceGetTris(xsurf,
                        (r_index_t *)((*(char **)&((materialCommands_t *)tess)->indices) + ((materialCommands_t *)tess)->indexCount * 2),
                        vertBase);

        ((materialCommands_t *)tess)->indexCount += triIndexCount;
    }
}

int g_tt_last_cached = -1;
void *g_tt_last_tess = 0;
int g_tt_seq = 0;
int g_tt_last_seq = 0;
void RB_TessTriangles(const surfaceType_t *surfType)
{
    char *tess;
    int sortedIndex;
    srfTriangles_t *tri = (srfTriangles_t *)surfType;

    tess = RB_TessBase();

    if (((materialCommands_t *)tess)->optimizedIndexCount != 0) {
        if (((materialCommands_t *)tess)->firstOptimizedVertex != tri->firstVertex ||
            ((materialCommands_t *)tess)->optimizedVertexCount != (int)tri->vertexCount) {

            sortedIndex = (*(int *)&((materialCommands_t *)tess)->declType);
            RB_EndSurface();
            RB_BeginSurface(
                ((materialCommands_t *)tess)->material,
                (*(MaterialTechniqueType *)&((materialCommands_t *)tess)->techType),
                ((materialCommands_t *)tess)->lmapIndex);
            if (sortedIndex != (*(int *)&((materialCommands_t *)tess)->declType)) {
                if (((materialCommands_t *)tess)->indexCount != 0 || ((materialCommands_t *)tess)->optimizedIndexCount != 0) {
                    RB_EndSurface();
                }
                tess = RB_TessBase();
                (*(int *)&((materialCommands_t *)tess)->declType) = sortedIndex;
            }
        }
    }

    if ((int)tri->indexCount + ((materialCommands_t *)tess)->optimizedIndexCount > 0x100000) {
        sortedIndex = (*(int *)&((materialCommands_t *)tess)->declType);
        RB_EndSurface();
        RB_BeginSurface(
            ((materialCommands_t *)tess)->material,
            (*(MaterialTechniqueType *)&((materialCommands_t *)tess)->techType),
            ((materialCommands_t *)tess)->lmapIndex);
        tess = RB_TessBase();
        if (sortedIndex != (*(int *)&((materialCommands_t *)tess)->declType)) {
            if (((materialCommands_t *)tess)->indexCount != 0 || ((materialCommands_t *)tess)->optimizedIndexCount != 0) {
                RB_EndSurface();
                tess = RB_TessBase();
            }
            (*(int *)&((materialCommands_t *)tess)->declType) = sortedIndex;
        }
    }

    (*(int *)&((materialCommands_t *)tess)->optimizedVertexSource) = 1;

    Com_Memcpy(
        (*(char **)&((materialCommands_t *)tess)->optimizedIndices) + ((materialCommands_t *)tess)->optimizedIndexCount * 2,
        (void *)tri->indices,
        (int)tri->indexCount * 2);

    ((materialCommands_t *)tess)->optimizedIndexCount += (int)tri->indexCount;
    ((materialCommands_t *)tess)->firstOptimizedVertex = tri->firstVertex;
    ((materialCommands_t *)tess)->optimizedVertexCount = (int)tri->vertexCount;
}

void RB_TessEntity(const GfxEntity *re)
{
    char *tess = RB_TessBase();
    byte *ent = (byte *)re;
    int reType = *(int *)ent;
    int isDx7;
    int color = 0;

    if ((unsigned)(reType - 4) > 5)
        return;

    isDx7 = (r_rendererInUse->current.integer == 2);

    switch (reType) {
    case 4:
    {
        float worldRadius[2];
        float screenOffset[2];

        if (ent[5] & 0x20) {

            float screenHeight = ((GfxEntity *)ent)->radius[1];
            byte *backEnd = (byte *)imp_backEnd;
            int *viewParms = (int *)((int)((r_backEndGlobals_t *)backEnd)->viewParms);
            float *projMatrix = (float *)&((GfxViewParms *)viewParms)->viewProjectionMatrix;
            float w;

            w = ((GfxEntity *)ent)->origin[0] * projMatrix[0xc / 4] +
                ((GfxEntity *)ent)->origin[1] * projMatrix[0x1c / 4] +
                ((GfxEntity *)ent)->origin[2] * projMatrix[0x2c / 4] +
                projMatrix[0x3c / 4];

            if (w <= 0.0f)
                return;

            {
                float *orthoRow = (float *)&((GfxViewParms *)viewParms)->inverseViewProjectionMatrix;
                int k;
                screenHeight *= 2.0f;
                for (k = 0; k < 3; k++)
                    worldRadius[k] = screenHeight * orthoRow[0x10 / 4 + k];

                {
                    float *viewRight = ((GfxViewParms *)viewParms)->axis[2];
                    float dotRight = worldRadius[0] * viewRight[0] +
                                     worldRadius[1] * viewRight[1] +
                                     worldRadius[2] * viewRight[2];
                    dotRight *= w;
                    screenOffset[0] = dotRight;
                }
            }

            screenOffset[1] = screenOffset[0];

            RB_BuildSprite_impl((const char *)re, screenOffset);
            return;
        }

        worldRadius[0] = ((GfxEntity *)ent)->radius[0];
        worldRadius[1] = ((GfxEntity *)ent)->radius[1];

        if (((GfxEntity *)ent)->minScreenRadius > 0.0f) {
            byte *backEnd = (byte *)imp_backEnd;
            int *viewParms = (int *)((int)((r_backEndGlobals_t *)backEnd)->viewParms);
            float *projMatrix = (float *)&((GfxViewParms *)viewParms)->viewProjectionMatrix;
            float w;

            w = ((GfxEntity *)ent)->origin[0] * projMatrix[0xc / 4] +
                ((GfxEntity *)ent)->origin[1] * projMatrix[0x1c / 4] +
                ((GfxEntity *)ent)->origin[2] * projMatrix[0x2c / 4] +
                projMatrix[0x3c / 4];

            if (w <= 0.0f)
                return;

            {
                float *origin = ((GfxEntity *)ent)->origin;
                float *viewOrg = ((r_backEndGlobals_t *)backEnd)->lodParms.origin;
                float sx, sy;
                float scale;
                int *proj = viewParms;
                int k;

                for (k = 0; k < 2; k++) {
                    float val = (origin[0] - viewOrg[0]) * 0.0f;
                    screenOffset[k] = val;
                }

                scale = 1.0f / w;
                screenOffset[0] *= scale;
                screenOffset[1] *= scale;

                float halfScreen = screenOffset[1] * 0.5f;
                if (((GfxEntity *)ent)->minScreenRadius > halfScreen) {
                    float ratio = ((GfxEntity *)ent)->minScreenRadius / halfScreen;
                    worldRadius[0] = ((GfxEntity *)ent)->radius[0] * ratio;
                    worldRadius[1] = ((GfxEntity *)ent)->radius[1] * ratio;
                }
            }
        }

        RB_BuildSprite_impl((const char *)re, worldRadius);
        return;
    }

    case 5:
    {

        return;
    }

    case 6:
        RB_TessParticleCloud(re);
        return;

    case 7:
    {
        float scaleX = ((GfxEntity *)ent)->radius[0];
        float scaleY = ((GfxEntity *)ent)->radius[1];
        float left[3], up[3];
        byte rgba[4];
        int nativeColor;
        float s0, t0, s1, t1;

        if (((GfxEntity *)ent)->rotation == 0.0f) {

            MakeNormalVectors(((GfxEntity *)ent)->axis[0], left, up);

            left[0] *= scaleX;
            left[1] *= scaleX;
            left[2] *= scaleX;
            up[0] *= scaleY;
            up[1] *= scaleY;
            up[2] *= scaleY;
        } else {

            float right[3], fwd[3];
            float angle, sinA, cosA;

            MakeNormalVectors(((GfxEntity *)ent)->axis[0], right, fwd);

            angle = ((GfxEntity *)ent)->rotation * 0.017453292519943295f;
            sinA = sinf(angle);
            cosA = cosf(angle);

            {
                float cs = cosA * scaleX;
                left[0] = cs * right[0];
                left[1] = cs * right[1];
                left[2] = cs * right[2];
                float ns = -sinA * scaleX;
                left[0] += ns * fwd[0];
                left[1] += ns * fwd[1];
                left[2] += ns * fwd[2];

                float cu = cosA * scaleY;
                up[0] = cu * fwd[0];
                up[1] = cu * fwd[1];
                up[2] = cu * fwd[2];
                float su = sinA * scaleY;
                up[0] += su * right[0];
                up[1] += su * right[1];
                up[2] += su * right[2];
            }
        }

        memcpy(&nativeColor, ((GfxEntity *)ent)->materialRGBA, sizeof(nativeColor));

        {
            byte *texInfo = (*(byte **)&((GfxEntity *)ent)->customMaterial);
            int cols = ((Material *)texInfo)->info.textureAtlasRowCount;
            int rows = ((Material *)texInfo)->info.textureAtlasColumnCount;
            int totalFrames = cols * rows;

            if (totalFrames <= 1) {
                s0 = 0.0f;
                t0 = 0.0f;
                s1 = 1.0f;
                t1 = 1.0f;
            } else {
                float invCols = 1.0f / (float)cols;
                float invRows = 1.0f / (float)rows;
                int frame = ((GfxEntity *)ent)->materialSubimageIndex;
                int col = frame % cols;
                int row = frame / cols;
                s0 = (float)col * invCols;
                t0 = (float)row * invRows;
                s1 = invCols + s0;
                t1 = invRows + t0;
            }
        }

        if (isDx7) {
            RB_AddQuadStampDx7_impl(((GfxEntity *)ent)->origin, left, up, nativeColor, s0, t0, s1, t1);
        } else {
            RB_AddQuadStamp_impl(((GfxEntity *)ent)->origin, left, up, nativeColor, s0, t0, s1, t1);
        }
        return;
    }

    case 8:
    {

        memcpy(&color, ((GfxEntity *)ent)->materialRGBA, sizeof(color));

        {
            byte *texInfo = (*(byte **)&((GfxEntity *)ent)->customMaterial);
            int cols = ((Material *)texInfo)->info.textureAtlasRowCount;
            int rows = ((Material *)texInfo)->info.textureAtlasColumnCount;
            float s0, t0, s1, t1;

            if (cols * rows <= 1) {
                s0 = 0.0f;
                t0 = 0.0f;
                s1 = 1.0f;
                t1 = 1.0f;
            } else {
                float invCols = 1.0f / (float)cols;
                float invRows = 1.0f / (float)rows;
                int frame = ((GfxEntity *)ent)->materialSubimageIndex;
                int col = frame % cols;
                int row = frame / cols;
                s0 = (float)col * invCols;
                t0 = (float)row * invRows;
                s1 = invCols + s0;
                t1 = invRows + t0;
            }

            if (isDx7) {
                RB_AddLineDx7_impl(((GfxEntity *)ent)->origin, (float *)(ent + 0x48),
                                   ((GfxEntity *)ent)->radius[0], color, s0, t0, s1, t1);
            } else {
                RB_AddLine_impl(((GfxEntity *)ent)->origin, (float *)(ent + 0x48),
                                ((GfxEntity *)ent)->radius[0], color, s0, t0, s1, t1);
            }
        }
        return;
    }

    case 9:
    {
        float forward[3];
        float midpoint[3];
        float right[3], up[3];
        int numSegments;
        float segLen;
        byte *b_start = (byte *)((GfxEntity *)ent)->origin;
        byte *b_end = (byte *)((GfxEntity *)ent)->endpos;

        {
            float dx = *(float *)(b_end) - *(float *)(b_start);
            float dy = *(float *)(b_end + 4) - *(float *)(b_start + 4);
            float dz = *(float *)(b_end + 8) - *(float *)(b_start + 8);
            segLen = dx * dx + dy * dy + dz * dz;
            segLen = sqrtf(segLen);
        }

        color = (((GfxEntity *)ent)->materialRGBA[0]) |
                (((GfxEntity *)ent)->materialRGBA[1] << 8) |
                (((GfxEntity *)ent)->materialRGBA[2] << 16) |
                (((GfxEntity *)ent)->materialRGBA[3] << 24);

        {
            float railWidth;
            railWidth = r_railCoreWidth->current.value;
            float invLen = segLen * 0.00390625f;

            if (isDx7) {
                RB_AddLineDx7_impl((float *)b_start, (float *)b_end,
                                   railWidth, color, 0.0f, 0.0f, invLen, 1.0f);
            } else {
                RB_AddLine_impl((float *)b_start, (float *)b_end,
                                railWidth, color, 0.0f, 0.0f, invLen, 1.0f);
            }
        }
        return;
    }

    default:
        return;
    }
}

const unsigned char faceAxis_125647[224] = {
    0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x98, 0xfc, 0x0f, 0x00, 0x86, 0x03, 0x10, 0x00, 0x79, 0x03, 0x10, 0x00, 0x28, 0x02, 0x10, 0x00,
    0x77, 0x01, 0x10, 0x00, 0x13, 0xfe, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
