#include "common_types.h"
#include "imports.h"
extern GfxBackEndData *backEndData;

extern float floorf(float x);

static vec3_t gridBasisDirs[8];
static int s_vc_log;

static int s_vc_logCount_00c96c84;
#define s_vc_logCount s_vc_logCount_00c96c84

extern r_global_permanent_t rgp;
extern r_backEndGlobals_t backEnd;

extern const float colorRed[];
extern const float colorGreen[];
extern const float colorYellow[];
extern const float colorWhite[];
extern const float vec3_origin[];

extern void Com_StripExtension(const char *src, char *dest);
extern qboolean FS_WriteFile(const char *path, const void *buffer, int size);
extern void Z_FreeInternal(void *ptr);
extern void *Z_MallocInternal(int size);
extern int FS_ReadFile(const char *path, void **buffer);
extern void FS_FreeFile(void *buffer);
extern void Com_Error(int level, const char *fmt, ...);
extern float Vec3Normalize(vec3_t v);
extern int R_CullPointAndRadius(const vec_t *pt, float radius, const DpvsPlane *clipPlanes, int clipPlaneCount);
extern void R_AddDebugString(void *debugGlobals, const vec_t *origin, const float *color, float scale, const char *text);
extern void R_AddDebugBox(void *debugGlobals, const vec_t *mins, const vec_t *maxs, const float *color);
extern int CM_BoxSightTrace(int oldHitNum, const vec_t *start, const vec_t *end, const vec_t *mins, const vec_t *maxs, unsigned int brushmask, int zero);

void RB_SaveLightVisHistory(void);
int RB_DeriveEntityLights(vec4_t *colorForDir, float sunVisibility, const Material *material, D3DLIGHT9 *lights, int maxLights);
void RB_ShowLightVisCachePoints(const vec_t *viewOrigin, const DpvsPlane *clipPlanes, int clipPlaneCount);
float RB_GetLightingAtPoint(const GfxLightGrid *lightGrid, const vec_t *samplePos, vec4_t *colorForDir);
void RB_SetupEntityLighting(const GfxEntity *ent, GfxEntityLighting *lighting);
void RB_InitLightVisHistory(const char *bspName);

static inline __attribute__((always_inline)) void BuildGridFilename(const char *baseName, char *filename)
{
    unsigned int len;

    Com_StripExtension(baseName, filename);
    len = strlen(filename) + 5;
    if (len > 63) {
        Com_Error(1, "light grid log filename '%s.grid' is too long\n", filename);
    }
    strcat(filename, ".grid");
}

typedef struct {
    unsigned short x;
    unsigned short y;
    unsigned short z;
} vc_log_entry_t;

static inline __attribute__((always_inline)) const dvar_t *RB_LightGridContrastDvar(void)
{
    return *(const dvar_t **)imp_r_vc_makelog;
}

static inline __attribute__((always_inline)) const dvar_t *RB_LightGridEnableTweaksDvar(void)
{
    return *(const dvar_t **)imp_r_showLightGrid;
}

static inline __attribute__((always_inline)) const dvar_t *RB_LightGridSpreadDvar(void)
{
    return *(const dvar_t **)imp_r_vc_showlog;
}

static inline __attribute__((always_inline)) const dvar_t *RB_LightGridUseTweakedValuesDvar(void)
{
    return *(const dvar_t **)imp_r_showMissingLightGrid;
}

static int VC_SearchLog(int gridX, int gridY, int gridZ, int *outMid)
{
    int bot = 0;
    int top = s_vc_logCount - 1;
    vc_log_entry_t *entries = (vc_log_entry_t *)s_vc_log;

    while (bot <= top) {
        int mid = (bot + top) >> 1;
        int cmp;
        vc_log_entry_t *e = &entries[mid];

        cmp = gridX - (int)e->x;
        if (cmp == 0) {
            cmp = gridY - (int)e->y;
            if (cmp == 0) {
                cmp = gridZ - (int)e->z;
                if (cmp == 0) {
                    if (outMid)
                        *outMid = mid;
                    return 1;
                }
            }
        }
        if (cmp < 0) {
            top = mid - 1;
        } else {
            bot = mid + 1;
        }
    }
    if (outMid)
        *outMid = bot;
    return 0;
}

static void VC_InsertLog(int insertAt, int gridX, int gridY, int gridZ)
{
    vc_log_entry_t *entries = (vc_log_entry_t *)s_vc_log;
    int remaining = s_vc_logCount - insertAt;

    if (s_vc_logCount > 0x3fffff)
        return;

    memmove(&entries[insertAt + 1], &entries[insertAt], remaining * sizeof(vc_log_entry_t));
    entries[insertAt].x = (unsigned short)gridX;
    entries[insertAt].y = (unsigned short)gridY;
    entries[insertAt].z = (unsigned short)gridZ;
    s_vc_logCount++;
}

void RB_SaveLightVisHistory(void)
{
    char filename[64];
    GfxWorld *world;

    if (!s_vc_log)
        return;

    world = rgp.world;
    if (!world)
        return;

    BuildGridFilename(world->name, filename);
    FS_WriteFile(filename, (void *)s_vc_log, s_vc_logCount * 6);
    Z_FreeInternal((void *)s_vc_log);
    s_vc_log = 0;
    s_vc_logCount = 0;
}

static void VecScaleAdd(vec3_t out, float scale, const vec3_t dir)
{
    out[0] += scale * dir[0];
    out[1] += scale * dir[1];
    out[2] += scale * dir[2];
}

static float VecDot(const vec3_t a, const vec3_t b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static void SetupDirectionalLight(D3DLIGHT9 *light, const float *ambient, const float *diffuse, const float *direction)
{
    light->Type = 3;
    light->Ambient.r = ambient[0];
    light->Ambient.g = ambient[1];
    light->Ambient.b = ambient[2];
    if (diffuse) {
        light->Diffuse.r = diffuse[0];
        light->Diffuse.g = diffuse[1];
        light->Diffuse.b = diffuse[2];
    }
    if (direction) {
        light->Direction.x = direction[0];
        light->Direction.y = direction[1];
        light->Direction.z = direction[2];
    }
    light->Range = 1.0f;
}

int RB_DeriveEntityLights(vec4_t *colorForDir, float sunVisibility, const Material *material, D3DLIGHT9 *lights, int maxLights)
{
    GfxWorld *world;
    MaterialTechniqueSet *techSet;
    MaterialTechnique *technique;
    int lightCount;
    int i, j;
    vec3_t errorInDir[8];
    vec3_t errorDirAvg;
    vec3_t lightColor;
    vec3_t lightDir;
    float errorTotal;
    float dotProduct;
    float scale;
    float sumX, sumY, sumZ;
    float ambientR, ambientG, ambientH;
    D3DLIGHT9 *light;
    float *sunDirPtr;
    float sunColorR, sunColorG, sunColorB;
    (void)0;

    if (maxLights <= 0)
        return 0;

    techSet = material->techniqueSet;
    technique = techSet->techniques[15];
    if (!technique || maxLights < 2)
        goto fallback;

    if (*((unsigned char *)technique + 0xe) == 0)
        goto fallback;

    memset(lights, 0, maxLights * sizeof(D3DLIGHT9));

    {
        float *chan0 = (float *)colorForDir;
        float *chan1 = chan0 + 8;
        float *chan2 = chan0 + 16;
        float *chan1End = chan2;
        float avgR = 0.0f, avgG = 0.0f, avgB = 0.0f;

        do {
            avgR += *chan0++;
            avgG += *chan1++;
            avgB += *chan2++;
        } while (chan1 != chan1End);

        lights[0].Type = 3;
        lights[0].Ambient.r = avgR * 0.125f * 0.5f;
        lights[0].Ambient.g = avgG * 0.125f * 0.5f;
        lights[0].Ambient.b = avgB * 0.125f * 0.5f;
        lights[0].Direction.z = 1.0f;

        world = rgp.world;
        sunColorR = sunVisibility * world->sunLight.color[0] + world->sunLight.u.dir.ambientColor[0];
        sunColorG = sunVisibility * world->sunLight.color[1] + world->sunLight.u.dir.ambientColor[1];
        sunColorB = sunVisibility * world->sunLight.color[2] + world->sunLight.u.dir.ambientColor[2];

        light = &lights[1];
        light->Type = 3;
        light->Ambient.r = sunColorR * 0.5f;
        light->Ambient.g = sunColorG * 0.5f;
        light->Ambient.b = sunColorB * 0.5f;
        light->Direction.z = 1.0f;
        lightCount = 2;
    }
    return lightCount;

fallback:

    memset(lights, 0, maxLights * sizeof(D3DLIGHT9));

    {
        float *chan0 = (float *)colorForDir;
        float *chan1 = chan0 + 8;
        float *chan2 = chan0 + 16;

        for (i = 0; i < 8; i++) {
            errorInDir[i][0] = chan0[i];
            errorInDir[i][1] = chan1[i];
            errorInDir[i][2] = chan2[i];
        }
    }

    light = lights;
    lightCount = 0;

    for (;;) {
        lightCount++;
        /* The last light is the sun, filled after this loop. Reserve its
         * slot before generating another grid light. */
        if (lightCount >= maxLights)
            break;

        errorDirAvg[0] = 0.0f;
        errorDirAvg[1] = 0.0f;
        errorDirAvg[2] = 0.0f;

        {
            float *sunDir = (float *)imp_vec3_colorintensity;
            float sdx = sunDir[0];
            float sdy = sunDir[1];
            float sdz = sunDir[2];
            unsigned int abs_mask = 0x7FFFFFFF;

            errorTotal = 0.0f;

            for (i = 0; i < 8; i++) {
                float dot = sdx * errorInDir[i][0] + sdy * errorInDir[i][1] + sdz * errorInDir[i][2];
                float absDot;

                {
                    union {
                        float f;
                        unsigned int u;
                    } uv;
                    uv.f = dot;
                    uv.u &= abs_mask;
                    absDot = uv.f;
                }
                errorDirAvg[0] += absDot * gridBasisDirs[i][0];
                errorDirAvg[1] += absDot * gridBasisDirs[i][1];
                errorDirAvg[2] += absDot * gridBasisDirs[i][2];
                errorTotal += dot * dot;
            }
        }

        if (errorTotal < 0.0008f) {

            goto deriveLightDone;
        }

        Vec3Normalize(errorDirAvg);

        lightColor[0] = 0.0f;
        lightColor[1] = 0.0f;
        lightColor[2] = 0.0f;
        {
            float edx = errorDirAvg[0];
            float edy = errorDirAvg[1];
            float edz = errorDirAvg[2];
            float totalWeight = 0.0f;

            for (i = 0; i < 8; i++) {
                float dot = edx * gridBasisDirs[i][0] + edy * gridBasisDirs[i][1] + edz * gridBasisDirs[i][2];
                if (dot > 0.01f) {
                    lightColor[0] += dot * errorInDir[i][0];
                    lightColor[1] += dot * errorInDir[i][1];
                    lightColor[2] += dot * errorInDir[i][2];
                    totalWeight += dot;
                }
            }

            if (totalWeight != 0.0f) {
                scale = 1.0f / totalWeight;
                lightColor[0] *= scale;
                lightColor[1] *= scale;
                lightColor[2] *= scale;
            }
        }

        sumX = 0.0f;
        sumY = 0.0f;
        sumZ = 0.0f;
        {
            for (i = 0; i < 8; i++) {
                float dot = gridBasisDirs[i][0] * errorDirAvg[0] + gridBasisDirs[i][1] * errorDirAvg[1] + gridBasisDirs[i][2] * errorDirAvg[2];

                if (dot > 0.0f) {
                    float negDot = -dot;
                    errorInDir[i][0] += negDot * lightColor[0];
                    errorInDir[i][1] += negDot * lightColor[1];
                    errorInDir[i][2] += negDot * lightColor[2];
                }

                sumX += errorInDir[i][0];
                sumY += errorInDir[i][1];
                sumZ += errorInDir[i][2];
            }
        }

        ambientR = sumX * 0.125f;
        ambientG = sumY * 0.125f;
        ambientH = sumZ * 0.125f;

        {
            float *sunDir = (float *)imp_vec3_colorintensity;
            float sdx = sunDir[0];
            float sdy = sunDir[1];
            float sdz = sunDir[2];
            float newError = 0.0f;

            for (i = 0; i < 8; i++) {
                float dot;
                errorInDir[i][0] -= ambientR;
                errorInDir[i][1] -= ambientG;
                errorInDir[i][2] -= ambientH;
                dot = sdx * errorInDir[i][0] + sdy * errorInDir[i][1] + sdz * errorInDir[i][2];
                newError += dot * dot;
            }

            if (errorTotal - newError < 0.02f) {
                goto deriveLightDone;
            }
        }

        light->Type = 3;
        light->Ambient.r = ambientR * 0.5f;
        light->Ambient.g = ambientG * 0.5f;
        light->Ambient.b = ambientH * 0.5f;
        light->Diffuse.r = lightColor[0] * 0.5f;
        light->Diffuse.g = lightColor[1] * 0.5f;
        light->Diffuse.b = lightColor[2] * 0.5f;

        {
            float lenSq = errorDirAvg[0] * errorDirAvg[0] + errorDirAvg[1] * errorDirAvg[1] + errorDirAvg[2] * errorDirAvg[2];
            if (lenSq < 0.000001f) {
                light->Direction.x = 0.0f;
                light->Direction.y = 0.0f;
                light->Direction.z = 1.0f;
            } else {
                light->Direction.x = -errorDirAvg[0];
                light->Direction.y = -errorDirAvg[1];
                light->Direction.z = -errorDirAvg[2];
            }
        }

        light++;
        continue;

    deriveLightDone:

        light++;
        break;
    }

    {
        D3DLIGHT9 *sunLight;
        int prevCount = lightCount - 1;
        sunLight = &lights[prevCount];

        world = rgp.world;

        sunLight->Type = 3;
        sunLight->Ambient.r = world->sunLight.u.dir.ambientColor[0] * 0.5f;
        sunLight->Ambient.g = world->sunLight.u.dir.ambientColor[1] * 0.5f;
        sunLight->Ambient.b = world->sunLight.u.dir.ambientColor[2] * 0.5f;
        sunLight->Diffuse.r = sunVisibility * world->sunLight.color[0] * 0.5f;
        sunLight->Diffuse.g = sunVisibility * world->sunLight.color[1] * 0.5f;
        sunLight->Diffuse.b = sunVisibility * world->sunLight.color[2] * 0.5f;
        sunLight->Direction.x = -world->sunLight.position[0];
        sunLight->Direction.y = -world->sunLight.position[1];
        sunLight->Direction.z = -world->sunLight.position[2];
    }

    return lightCount;
}

void RB_ShowLightVisCachePoints(const vec_t *viewOrigin, const DpvsPlane *clipPlanes, int clipPlaneCount)
{
    int spread;
    int x, y, z;
    int dx, dy, dz;
    vec3_t origin;
    void *debugGlobals;
    const float *debugColor;

    if (!s_vc_log)
        return;

    spread = RB_LightGridSpreadDvar()->current.integer;
    if (spread <= 0)
        return;

    x = ((int)floorf(viewOrigin[0] + 131072.0f + 0.5f)) >> 5;
    y = ((int)floorf(viewOrigin[1] + 131072.0f + 0.5f)) >> 5;
    z = ((int)floorf(131072.0f + viewOrigin[2])) >> 6;

    origin[0] = (float)(x * 32 - 0x20000);
    origin[1] = (float)(y * 32 - 0x20000);
    origin[2] = (float)(z * 64 - 0x20000);

    for (dz = -1; dz < 2; dz++) {
        int gz = z + dz;
        if (gz < 0 || (gz << 6) > 0x80000)
            continue;

        float gridZ = (float)((gz << 6) - 0x20000);

        for (dy = -spread; dy <= spread; dy++) {
            int gy = y + dy;
            if (gy < 0 || (gy << 5) > 0x80000)
                continue;

            float gridY = (float)((gy << 5) - 0x20000);

            for (dx = -spread; dx <= spread; dx++) {
                int gx = x + dx;
                if (gx < 0 || (gx << 5) > 0x80000)
                    continue;

                {
                    int bot = 0;
                    int top = s_vc_logCount - 1;
                    vc_log_entry_t *entries = (vc_log_entry_t *)s_vc_log;
                    int found = 0;

                    while (bot <= top) {
                        int mid = (bot + top) >> 1;
                        vc_log_entry_t *e = &entries[mid];
                        int cmp;

                        cmp = gx - (int)e->x;
                        if (cmp == 0) {
                            cmp = gy - (int)e->y;
                            if (cmp == 0) {
                                cmp = gz - (int)e->z;
                                if (cmp == 0) {

                                    if (mid >= 0)
                                        goto next_dx;
                                    found = 1;
                                    break;
                                }
                            }
                        }
                        if (cmp < 0) {
                            top = mid - 1;
                        } else {
                            bot = mid + 1;
                        }
                    }
                }

                origin[0] = (float)((gx << 5) - 0x20000);
                origin[1] = gridY;
                origin[2] = gridZ;

                if (R_CullPointAndRadius(origin, 0.0f, clipPlanes, clipPlaneCount) == 2)
                    goto next_dx;

                if ((unsigned int)dz <= 1) {
                    debugColor = colorGreen;
                } else {
                    debugColor = colorRed;
                }

                debugGlobals = &backEndData->debugGlobals;
                R_AddDebugString(debugGlobals, origin, debugColor, 1.0f, ".");

            next_dx:;
            }
        }
    }
}

static int GridPointLookup(const GfxLightGrid *lightGrid, unsigned int keyHigh, unsigned char keyLow, int *outIndex)
{
    int bot = 0;
    int top = lightGrid->pointCount - 1;
    GfxLightGridPoint *points = lightGrid->points;

    while (bot <= top) {
        int mid = (bot + top) >> 1;
        int cmp;
        GfxLightGridPoint *p = &points[mid];

        cmp = (int)p->xyzHighBits - (int)keyHigh;
        if (cmp == 0) {
            cmp = ((int)p->xyzLowBitsAndSunVisible - (int)keyLow) & ~1;
            if (cmp == 0) {
                *outIndex = mid;
                return 1;
            }
        }
        if (cmp < 0) {
            bot = mid + 1;
        } else {
            top = mid - 1;
        }
    }
    *outIndex = bot;
    return 0;
}

static inline __attribute__((always_inline)) void PackGridKey(int gridX, int gridY, int gridZ, unsigned int *keyHigh, unsigned char *keyLow)
{
    *keyHigh = ((gridX & ~3) << 19) | ((gridY & ~3) << 8) | (gridZ >> 2);
    *keyLow = ((gridX & 3) << 6) | ((gridY & 3) << 4) | ((gridZ & 3) << 2);
}

static void AccumulateGridColors(vec4_t *colorForDir, GfxLightGridColors *gridColors, float weight)
{
    float scale = weight * (1.0f / 255.0f);
    int channelIndex;
    float *outPtr = (float *)colorForDir;

    for (channelIndex = 0; channelIndex < 3; channelIndex++) {
        unsigned char *rgb = &gridColors->rgb[channelIndex][0];
        int j;
        for (j = 0; j < 8; j++) {
            *outPtr += (float)rgb[j] * scale;
            outPtr++;
        }
    }
}

float RB_GetLightingAtPoint(const GfxLightGrid *lightGrid, const vec_t *samplePos, vec4_t *colorForDir)
{
    int x0, y0, z0;
    float wx1, wx0, wy1, wy0, wz1, wz0;
    float maxWeight, sunWeight;
    int cornerIndex;
    GfxWorld *world;
    int doDebug;

    {
        float fx = samplePos[0] + 131072.0f;
        float fy = samplePos[1] + 131072.0f;
        float fz = samplePos[2] + 131072.0f;

        x0 = ((int)floorf(fx)) >> 5;
        wx1 = fx * 0.03125f - (float)x0;
        wx0 = 1.0f - wx1;

        y0 = ((int)floorf(fy)) >> 5;
        wy1 = fy * 0.03125f - (float)y0;
        wy0 = 1.0f - wy1;

        z0 = ((int)floorf(fz)) >> 6;
        wz1 = fz * 0.015625f - (float)z0;
        wz0 = 1.0f - wz1;
    }

    doDebug = RB_LightGridEnableTweaksDvar()->current.enabled;
    if (doDebug) {
        vec3_t boxMins, boxMaxs;
        vec3_t sampleBoxMins, sampleBoxMaxs;
        void *debugGlobals;

        boxMins[0] = (float)(x0 * 32 - 0x20000);
        boxMins[1] = (float)(y0 * 32 - 0x20000);
        boxMins[2] = (float)(z0 * 64 - 0x20000);
        boxMaxs[0] = boxMins[0] + 32.0f;
        boxMaxs[1] = boxMins[1] + 32.0f;
        boxMaxs[2] = boxMins[2] + 64.0f;

        debugGlobals = &backEndData->debugGlobals;
        R_AddDebugBox(debugGlobals, boxMins, boxMaxs, colorWhite);

        sampleBoxMins[0] = samplePos[0] - 0.5f;
        sampleBoxMins[1] = samplePos[1] - 0.5f;
        sampleBoxMins[2] = samplePos[2] - 0.5f;
        sampleBoxMaxs[0] = samplePos[0] + 0.5f;
        sampleBoxMaxs[1] = samplePos[1] + 0.5f;
        sampleBoxMaxs[2] = samplePos[2] + 0.5f;
        R_AddDebugBox(debugGlobals, sampleBoxMins, sampleBoxMaxs, colorYellow);
    }

    memset(colorForDir, 0, 0x60);
    maxWeight = 0.0f;
    sunWeight = 0.0f;

    for (cornerIndex = 0; cornerIndex < 8; cornerIndex++) {
        int cx = (cornerIndex & 1);
        int gridX = x0 + cx;
        int cy = (cornerIndex >> 1) & 1;
        int gridY = y0 + cy;
        int cz = (cornerIndex >> 2) & 1;
        int gridZ = z0 + cz;
        unsigned int keyHigh;
        unsigned char keyLow;
        int pointIndex;
        int gridIndex;
        GfxLightGridPoint *gridPoint;
        float weight;
        int debugTrace;

        debugTrace = RB_LightGridContrastDvar()->current.integer;
        if (debugTrace) {

            int bot, top, found;
            vc_log_entry_t *entries = (vc_log_entry_t *)s_vc_log;
            top = s_vc_logCount - 1;
            bot = 0;
            found = 0;

            while (bot <= top) {
                int mid = (bot + top) >> 1;
                vc_log_entry_t *e = &entries[mid];
                int cmp;

                cmp = gridX - (int)e->x;
                if (cmp == 0) {
                    cmp = gridY - (int)e->y;
                    if (cmp == 0) {
                        cmp = gridZ - (int)e->z;
                        if (cmp == 0) {
                            found = 1;
                            break;
                        }
                    }
                }
                if (cmp < 0) {
                    top = mid - 1;
                } else {
                    bot = mid + 1;
                }
            }

            if (!found) {

                if (s_vc_logCount <= 0x3fffff) {
                    int insertAt = bot;
                    int remaining = s_vc_logCount - insertAt;
                    memmove(&entries[insertAt + 1], &entries[insertAt], remaining * sizeof(vc_log_entry_t));
                    entries[insertAt].x = (unsigned short)gridX;
                    entries[insertAt].y = (unsigned short)gridY;
                    entries[insertAt].z = (unsigned short)gridZ;
                    s_vc_logCount++;
                }
            }

            gridY = gridY;
        }

        PackGridKey(gridX, gridY, gridZ, &keyHigh, &keyLow);

        if (!GridPointLookup(lightGrid, keyHigh, keyLow, &gridIndex))
            continue;

        gridPoint = &lightGrid->points[gridIndex];

        if (gridPoint->needsTrace & (1 << cornerIndex)) {

            float gridPosX = (float)(gridX * 32 - 0x20000);
            float gridPosY = (float)(gridY * 32 - 0x20000);
            float gridPosZ = (float)(gridZ * 64 - 0x20000);
            vec3_t traceDir;
            vec3_t nudgedGridPos;

            traceDir[0] = samplePos[0] - gridPosX;
            traceDir[1] = samplePos[1] - gridPosY;
            traceDir[2] = samplePos[2] - gridPosZ;
            Vec3Normalize(traceDir);

            nudgedGridPos[0] = gridPosX + 0.01f * traceDir[0];
            nudgedGridPos[1] = gridPosY + 0.01f * traceDir[1];
            nudgedGridPos[2] = gridPosZ + 0.01f * traceDir[2];

            if (CM_BoxSightTrace(0, samplePos, nudgedGridPos, vec3_origin, vec3_origin, 0, 0x2001)) {

                doDebug = RB_LightGridEnableTweaksDvar()->current.enabled;
                if (doDebug) {
                    vec3_t bmin, bmax;
                    bmin[0] = gridPosX - 1.1f;
                    bmin[1] = gridPosY - 1.1f;
                    bmin[2] = gridPosZ - 1.1f;
                    bmax[0] = gridPosX + 1.1f;
                    bmax[1] = gridPosY + 1.1f;
                    bmax[2] = gridPosZ + 1.1f;
                    R_AddDebugBox(&backEndData->debugGlobals, bmin, bmax, colorRed);
                }
                continue;
            }

            doDebug = RB_LightGridEnableTweaksDvar()->current.enabled;
            if (doDebug) {
                vec3_t bmin, bmax;
                bmin[0] = gridPosX - 1.0f;
                bmin[1] = gridPosY - 1.0f;
                bmin[2] = gridPosZ - 1.0f;
                bmax[0] = gridPosX + 1.0f;
                bmax[1] = gridPosY + 1.0f;
                bmax[2] = gridPosZ + 1.0f;
                R_AddDebugBox(&backEndData->debugGlobals, bmin, bmax, colorRed);
            }
        } else {

            doDebug = RB_LightGridEnableTweaksDvar()->current.enabled;
            if (doDebug) {
                float gridPosX = (float)(gridX * 32 - 0x20000);
                float gridPosY = (float)(gridY * 32 - 0x20000);
                float gridPosZ = (float)(gridZ * 64 - 0x20000);
                vec3_t bmin, bmax;
                bmin[0] = gridPosX - 0.9f;
                bmin[1] = gridPosY - 0.9f;
                bmin[2] = gridPosZ - 0.9f;
                bmax[0] = gridPosX + 0.9f;
                bmax[1] = gridPosY + 0.9f;
                bmax[2] = gridPosZ + 0.9f;
                R_AddDebugBox(&backEndData->debugGlobals, bmin, bmax, colorGreen);
            }
        }

        weight = (cx ? wx1 : wx0) * (cy ? wy1 : wy0) * (cz ? wz1 : wz0);
        maxWeight += weight;

        {
            GfxLightGridColors *colors = &lightGrid->colors[gridPoint->colorsIndex];
            AccumulateGridColors(colorForDir, colors, weight);
        }

        if (gridPoint->xyzLowBitsAndSunVisible & 1) {
            sunWeight += weight;
        }
    }

    if (maxWeight >= 0.98f)
        return sunWeight;

    if (maxWeight == 0.0f) {

        int nearX, nearY, nearZ;
        unsigned int keyHigh;
        unsigned char keyLow;
        int gridIndex;
        GfxLightGridColors *colors;

        nearX = x0 + (wx1 > 0.5f ? 1 : 0);
        nearY = y0 + (wy1 > 0.5f ? 1 : 0);
        nearZ = z0 + (wz1 > 0.5f ? 1 : 0);

        world = rgp.world;
        if (world && RB_LightGridUseTweakedValuesDvar()->current.enabled)
            goto setDefaultColors;

        if (lightGrid->pointCount == 0)
            goto setDefaultColors;

        PackGridKey(nearX, nearY, nearZ, &keyHigh, &keyLow);

        {
            int bot = 0;
            int top = lightGrid->pointCount - 1;
            GfxLightGridPoint *points = lightGrid->points;

            while (bot <= top) {
                int mid = (bot + top) >> 1;
                GfxLightGridPoint *p = &points[mid];
                int cmp;

                cmp = (int)p->xyzHighBits - (int)keyHigh;
                if (cmp == 0) {
                    cmp = ((int)p->xyzLowBitsAndSunVisible - (int)keyLow) & ~1;
                    if (cmp == 0) {
                        gridIndex = mid;
                        goto foundNearest;
                    }
                }
                if (cmp < 0) {
                    bot = mid + 1;
                } else {
                    top = mid - 1;
                }
            }
            gridIndex = bot;
        }

        {
            GfxLightGridPoint *points = lightGrid->points;
            colors = &lightGrid->colors[points[gridIndex].colorsIndex];
        }

        {
            float *outPtr = (float *)colorForDir;
            int channelIndex;
            for (channelIndex = 0; channelIndex < 3; channelIndex++) {
                unsigned char *rgb = &colors->rgb[channelIndex][0];
                int j;
                for (j = 0; j < 8; j++) {
                    *outPtr += (float)rgb[j] * (1.0f / 255.0f);
                    outPtr++;
                }
            }
        }

        if (lightGrid->points[gridIndex].xyzLowBitsAndSunVisible & 1) {
            sunWeight = 1.0f;
        } else {

        }
        return sunWeight;

    foundNearest: {
        GfxLightGridPoint *points = lightGrid->points;
        colors = &lightGrid->colors[points[gridIndex].colorsIndex];
    }
        {
            float *outPtr = (float *)colorForDir;
            int channelIndex;
            for (channelIndex = 0; channelIndex < 3; channelIndex++) {
                unsigned char *rgb = &colors->rgb[channelIndex][0];
                int j;
                for (j = 0; j < 8; j++) {
                    *outPtr += (float)rgb[j] * (1.0f / 255.0f);
                    outPtr++;
                }
            }
        }
        if (lightGrid->points[gridIndex].xyzLowBitsAndSunVisible & 1) {
            sunWeight = 1.0f;
        }
        return sunWeight;

    setDefaultColors:

        {
            float one = 1.0f;
            float *v = (float *)colorForDir;

            v[0] = 0.0f;
            v[1] = one;
            v[2] = 0.0f;
            v[3] = one;

            v[4] = 0.0f;
            v[5] = one;
            v[6] = 0.0f;
            v[7] = one;

            v[8] = 0.0f;
            v[9] = 0.0f;
            v[10] = one;
            v[11] = one;

            v[12] = 0.0f;
            v[13] = 0.0f;
            v[14] = one;
            v[15] = one;

            v[16] = 0.0f;
            v[17] = 0.0f;
            v[18] = 0.0f;
            v[19] = 0.0f;

            v[20] = one;
            v[21] = one;
            v[22] = one;
            v[23] = one;
        }
        return sunWeight;
    }

    {
        float invWeight = 1.0f / maxWeight;
        float *ptr = (float *)colorForDir;
        int k;

        sunWeight *= invWeight;

        for (k = 0; k < 6; k++) {
            ptr[0] *= invWeight;
            ptr[1] *= invWeight;
            ptr[2] *= invWeight;
            ptr[3] *= invWeight;
            ptr += 4;
        }
    }

    return sunWeight;
}

void RB_SetupEntityLighting(const GfxEntity *ent, GfxEntityLighting *lighting)
{
    vec3_t lightOrigin;
    GfxWorld *world;

    lighting->viewCount = backEnd.viewCount;

    if ((signed char)((unsigned char *)&ent->renderFxFlags)[0] < 0) {

        lightOrigin[0] = ent->lighting.origin[0];
        lightOrigin[1] = ent->lighting.origin[1];
        lightOrigin[2] = ent->lighting.origin[2];
    } else {

        lightOrigin[0] = ent->origin[0];
        lightOrigin[1] = ent->origin[1];
        lightOrigin[2] = ent->origin[2] + 4.0f;
    }

    world = rgp.world;
    lighting->sunVisibility = RB_GetLightingAtPoint(&world->lightGrid, lightOrigin, lighting->colorForDir);
}

void RB_InitLightVisHistory(const char *bspName)
{
    char filename[64];
    void *buffer;
    int fileLen;
    int copySize;
    const dvar_t **contrastSlot;

    s_vc_log = 0;
    s_vc_logCount = 0;

    contrastSlot = (const dvar_t **)imp_r_vc_makelog;
    if (!(*contrastSlot)->current.integer)
        return;

    s_vc_log = (int)Z_MallocInternal(0x1800000);

    if ((*contrastSlot)->current.integer != 2)
        return;

    BuildGridFilename(bspName, filename);

    fileLen = FS_ReadFile(filename, &buffer);
    if (fileLen < 0)
        return;

    if ((unsigned int)fileLen / 6u * 6u == (unsigned int)fileLen) {

        copySize = (unsigned int)fileLen < 0x1800001u ? fileLen : 0x1800000;

        memcpy((void *)s_vc_log, buffer, copySize);
        s_vc_logCount = (unsigned int)copySize / 6u;
    }

    FS_FreeFile(buffer);
}

const unsigned char __ZZ13R_AddDebugBoxP12DebugGlobalsPKfS2_S2_E10iEdgePairs[96] = {
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
    0x05, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00
};
