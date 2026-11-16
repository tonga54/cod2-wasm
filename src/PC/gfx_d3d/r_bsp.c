#include "common_types.h"
extern dvar_t *r_rendererInUse;
#include "imports.h"
extern refimport_t ri;
#include "bytematch.h"
#include <string.h>

extern void R_FreeStaticVertexBuffer(void *vb);
extern void *R_AllocStaticVertexBuffer(void *vb_out, int size);
extern void R_FinishStaticVertexBuffer(void *vb);
extern void Com_Memcpy(void *dest, const void *src, int count);
extern void R_InterpretSunLightParseParamsIntoLights(void *sunParse, void *lights);
extern float ColorNormalize(float *in, float *out);
extern void *R_LoadWorldInternal(const char *name);
extern void RB_InitLightVisHistory(const char *name);
extern void R_FlushSun(void);
extern void R_ResetShadowCookies(void);
extern void R_InitStaticModelIndexCache(void);
extern void *Hunk_AllocInternal(int size);
extern void R_InitStaticModelDynamicData(int index);
extern void *Image_Register(const char *name, int flag1, int flag2);

extern r_global_permanent_t rgp;

extern byte r_frontEndData_ptr[];

extern vec3_t vec3_colorintensity;

static inline __attribute__((always_inline)) dvar_t *R_DvarFromImport(void *imp)
{
    return *(dvar_t **)imp;
}

void R_ResetSunLightOverride(void);
void R_ReleaseWorld(void);
void R_GetWorldBounds(vec_t *min, vec_t *max);
void R_InterpretSunLightParseParams(SunLightParseParams *sunParse);
void R_SetSunLightOverride(const vec_t *sunColor);
IDirect3DVertexBuffer9 *R_CreateWorldVertexBuffer(GfxWorldVertex *vertices, int vertexCount);
void R_ReloadWorld(void);
void R_ShutdownWorld(void);
void R_UpdateLightsFromDvars(void);
void R_LoadWorld(const char *name, int *checksum);
void R_ResetSunLightParseParams(void);

void R_ResetSunLightOverride(void)
{
    byte *world = (*(byte **)&rgp.world);

    vec_t *dst = (vec_t *)&((GfxWorld *)world)->sunLight.color[0];
    const vec_t *src = (const vec_t *)&((GfxWorld *)world)->sunColorFromBsp[0];
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
}

void R_ReleaseWorld(void)
{

    if (*(void **)((*(byte **)&rgp.world) + 0x30) != NULL) {
        R_FreeStaticVertexBuffer(*(void **)((*(byte **)&rgp.world) + 0x30));
        *(void **)((*(byte **)&rgp.world) + 0x30) = NULL;
    }
}

void R_GetWorldBounds(vec_t *min, vec_t *max)
{

    const vec_t *bmin = (const vec_t *)((*(byte **)&rgp.world) + 0x13c);
    min[0] = bmin[0];
    min[1] = bmin[1];
    min[2] = bmin[2];

    const vec_t *bmax = (const vec_t *)((*(byte **)&rgp.world) + 0x148);
    max[0] = bmax[0];
    max[1] = bmax[1];
    max[2] = bmax[2];
}

void R_InterpretSunLightParseParams(SunLightParseParams *sunParse)
{

    R_InterpretSunLightParseParamsIntoLights(sunParse, (*(byte **)&rgp.world) + 0xb4);

    {
        byte *world = (*(byte **)&rgp.world);
        vec_t *dst = (vec_t *)&((GfxWorld *)world)->sunColorFromBsp[0];
        const vec_t *src = (const vec_t *)&((GfxWorld *)world)->sunLight.color[0];
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
    }
}

void R_SetSunLightOverride(const vec_t *sunColor)
{
    const dvar_t *device = r_rendererInUse;

    if (device->current.integer == 2)
        return;

    {
        byte *world = (*(byte **)&rgp.world);

        vec_t *dst = (vec_t *)&((GfxWorld *)world)->sunLight.color[0];
        dst[0] = sunColor[0];
        dst[1] = sunColor[1];
        dst[2] = sunColor[2];
    }
}

IDirect3DVertexBuffer9 *R_CreateWorldVertexBuffer(GfxWorldVertex *vertices, int vertexCount)
{
    void *worldVb;
    byte *dataPtr;
    int sizeVerts;
    int vertIndex;

    sizeVerts = (r_rendererInUse->current.integer == 2) ? 0x20 : 0x44;
    sizeVerts *= vertexCount;

    dataPtr = (byte *)R_AllocStaticVertexBuffer(&worldVb, sizeVerts);

    if (r_rendererInUse->current.integer != 2) {

        Com_Memcpy(dataPtr, vertices, sizeVerts);
    } else {

        for (vertIndex = 0; vertIndex < vertexCount; vertIndex++) {
            byte *src = (byte *)vertices + vertIndex * 0x44;
            byte *dst = dataPtr + vertIndex * 0x20;

            *(int *)(dst + 0x00) = *(int *)(src + 0x00);
            *(int *)(dst + 0x04) = *(int *)(src + 0x04);
            *(int *)(dst + 0x08) = *(int *)(src + 0x08);

            *(int *)(dst + 0x0c) = *(int *)(src + 0x18);
            *(int *)(dst + 0x10) = *(int *)(src + 0x1c);
            *(int *)(dst + 0x14) = *(int *)(src + 0x20);
            *(int *)(dst + 0x18) = *(int *)(src + 0x24);
            *(int *)(dst + 0x1c) = *(int *)(src + 0x28);
        }
    }

    R_FinishStaticVertexBuffer(worldVb);
#ifdef __EMSCRIPTEN__
    if (r_rendererInUse->current.integer == 2) {
        extern void WebWorldNormals_Set(const void *, const float *, int, int, int);
        WebWorldNormals_Set(worldVb, (const float *)vertices, vertexCount,
                            sizeof(*vertices), (int)((byte *)&vertices[0].normal - (byte *)vertices));
    }
#endif
    return (IDirect3DVertexBuffer9 *)worldVb;
}

void R_ReloadWorld(void)
{
    byte *world = (*(byte **)&rgp.world);

    (*(void **)&((GfxWorld *)world)->vd.worldVb) = R_CreateWorldVertexBuffer(
        ((GfxWorld *)world)->vd.vertices,
        ((GfxWorld *)world)->vertexCount);
}

void R_ShutdownWorld(void)
{
    byte *world = (*(byte **)&rgp.world);

    if (world == NULL)
        return;

    {
        void *vb = (*(void **)&((GfxWorld *)world)->vd.worldVb);
        if (vb != NULL) {
            R_FreeStaticVertexBuffer(vb);
            *(void **)((*(byte **)&rgp.world) + 0x30) = NULL;
        }
    }

    (*(void **)&rgp.world) = NULL;
}

void R_UpdateLightsFromDvars(void)
{
    byte sunParse[128];
    dvar_t *dvar;
    int channelIter;
    byte *world;

    dvar = R_DvarFromImport(imp_r_lightTweakAmbient);
    *(float *)(sunParse + 0x40) = dvar->current.value;

    dvar = R_DvarFromImport(imp_r_lightTweakDiffuseFraction);
    *(float *)(sunParse + 0x50) = dvar->current.value;

    dvar = R_DvarFromImport(imp_r_lightTweakSunLight);
    *(float *)(sunParse + 0x54) = dvar->current.value;

    dvar = R_DvarFromImport(imp_r_lightTweakAmbientColor);
    for (channelIter = 0; channelIter < 3; channelIter++) {
        *(float *)(sunParse + 0x44 + channelIter * 4) = (float)dvar->current.color[channelIter];
    }
    ColorNormalize((float *)(sunParse + 0x44), (float *)(sunParse + 0x44));

    dvar = R_DvarFromImport(imp_r_lightTweakSunColor);
    for (channelIter = 0; channelIter < 3; channelIter++) {
        *(float *)(sunParse + 0x58 + channelIter * 4) = (float)dvar->current.color[channelIter];
    }
    ColorNormalize((float *)(sunParse + 0x58), (float *)(sunParse + 0x58));

    dvar = R_DvarFromImport(imp_r_lightTweakSunDiffuseColor);
    for (channelIter = 0; channelIter < 3; channelIter++) {
        *(float *)(sunParse + 0x64 + channelIter * 4) = (float)dvar->current.color[channelIter];
    }
    ColorNormalize((float *)(sunParse + 0x64), (float *)(sunParse + 0x64));

    sunParse[0x70] = 1;

    dvar = R_DvarFromImport(imp_r_lightTweakSunDirection);
    {
        vec_t *src = dvar->current.vector;
        *(int *)(sunParse + 0x74) = *(int *)(src + 0);
        *(int *)(sunParse + 0x78) = *(int *)(src + 1);
        *(int *)(sunParse + 0x7c) = *(int *)(src + 2);
    }

    world = (*(byte **)&rgp.world);
    if (world) {   /* no world (e.g. main menu, no map loaded) -> nothing to update */
        R_InterpretSunLightParseParamsIntoLights(sunParse, world + 0xb4);

        {
            vec_t *dst = (vec_t *)&((GfxWorld *)world)->sunColorFromBsp[0];
            const vec_t *src = (const vec_t *)&((GfxWorld *)world)->sunLight.color[0];
            dst[0] = src[0];
            dst[1] = src[1];
            dst[2] = src[2];
        }
    }
}

void R_LoadWorld(const char *name, int *checksum)
{
    byte *world;
    byte *worldData;
    refimport_t *refimport;
    dvar_t *dvar;
    byte *frontEnd;
    int i;

    RB_InitLightVisHistory(name);
    world = (byte *)R_LoadWorldInternal(name);
    (*(void **)&rgp.world) = world;

    if (checksum != NULL) {
        *checksum = (*(int *)&((GfxWorld *)world)->checksum);
    }

    world = *(byte **)&rgp.world;
    worldData = ((char *)world + offsetof(GfxWorld, sunParse.name[0]));

    refimport = (refimport_t *)&ri;

    refimport->Dvar_SetFloat(R_DvarFromImport(imp_r_lightTweakAmbient), (*(float *)&((GfxWorld *)worldData)->sunParse.name[12]));
    refimport->Dvar_SetFloat(R_DvarFromImport(imp_r_lightTweakDiffuseFraction), (*(float *)&((GfxWorld *)worldData)->sunParse.name[28]));
    refimport->Dvar_SetFloat(R_DvarFromImport(imp_r_lightTweakSunLight), (*(float *)&((GfxWorld *)worldData)->sunParse.name[32]));

    refimport->Dvar_SetColor(R_DvarFromImport(imp_r_lightTweakAmbientColor),
                             (*(float *)&((GfxWorld *)worldData)->sunParse.name[16]), (*(float *)&((GfxWorld *)worldData)->sunParse.name[20]),
                             (*(float *)&((GfxWorld *)worldData)->sunParse.name[24]), 1.0f);
    refimport->Dvar_SetColor(R_DvarFromImport(imp_r_lightTweakSunColor),
                             (*(float *)&((GfxWorld *)worldData)->sunParse.name[36]), (*(float *)&((GfxWorld *)worldData)->sunParse.name[40]),
                             (*(float *)&((GfxWorld *)worldData)->sunParse.name[44]), 1.0f);
    refimport->Dvar_SetColor(R_DvarFromImport(imp_r_lightTweakSunDiffuseColor),
                             (*(float *)&((GfxWorld *)worldData)->sunParse.name[48]), (*(float *)&((GfxWorld *)worldData)->sunParse.name[52]),
                             (*(float *)&((GfxWorld *)worldData)->sunParse.name[56]), 1.0f);

    refimport->Dvar_SetVec3(R_DvarFromImport(imp_r_lightTweakSunDirection),
                            ((GfxWorld *)worldData)->sunParse.ambientScale, ((GfxWorld *)worldData)->sunParse.ambientColor[0],
                            ((GfxWorld *)worldData)->sunParse.ambientColor[1]);

    dvar = R_DvarFromImport(imp_r_lightTweakAmbient);
    refimport->Dvar_SetModified(dvar);
    dvar = R_DvarFromImport(imp_r_lightTweakDiffuseFraction);
    refimport->Dvar_SetModified(dvar);
    dvar = R_DvarFromImport(imp_r_lightTweakSunLight);
    refimport->Dvar_SetModified(dvar);
    dvar = R_DvarFromImport(imp_r_lightTweakAmbientColor);
    refimport->Dvar_SetModified(dvar);
    dvar = R_DvarFromImport(imp_r_lightTweakSunColor);
    refimport->Dvar_SetModified(dvar);
    dvar = R_DvarFromImport(imp_r_lightTweakSunDiffuseColor);
    refimport->Dvar_SetModified(dvar);
    dvar = R_DvarFromImport(imp_r_lightTweakSunDirection);
    refimport->Dvar_SetModified(dvar);

    R_UpdateLightsFromDvars();
    R_FlushSun();
    R_ResetShadowCookies();
    R_InitStaticModelIndexCache();

    frontEnd = (byte *)imp_rg;
    world = (*(byte **)&rgp.world);

    (*(void **)&((r_globals_t *)frontEnd)->smodelDyncs) = Hunk_AllocInternal(((GfxWorld *)world)->smodelCount * 8);
    world = (*(byte **)&rgp.world);
    (*(void **)&((r_globals_t *)frontEnd)->surfaces) = Hunk_AllocInternal(((GfxWorld *)world)->surfaceCount * 4);
    world = (*(byte **)&rgp.world);
    (*(void **)&((r_globals_t *)frontEnd)->cullGroups) = Hunk_AllocInternal(((GfxWorld *)world)->cullGroupCount * 4);

    world = (*(byte **)&rgp.world);
    if (((GfxWorld *)world)->smodelCount > 0) {
        for (i = 0; i < ((GfxWorld *)world)->smodelCount; i++) {
            R_InitStaticModelDynamicData(i);
        }
    }

    {
        const dvar_t *device = r_rendererInUse;
        if (device->current.integer == 2) {
            (*(void **)&rgp.sunHalfAngleImage) =
                Image_Register("$sunhalfangle", 1, 0);
        }
    }
}

void R_ResetSunLightParseParams(void)
{
    R_UpdateLightsFromDvars();
}

vec3_t vec3_colorintensity = { 0.299f, 0.587f, 0.114f }; /* truncated 8->3 (blob over-capture) */
