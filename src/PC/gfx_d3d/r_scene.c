#include "common_types.h"
extern dvar_t *r_rendererInUse;
extern DxGlobals dx;
#include "imports.h"

extern struct GfxScene scene;
static int warnCount_007f1dcc;
static int warnCount_007f1dd0;
static GfxViewParms lockPvsViewParms;
__attribute__((used)) byte s_XModelSurfaceSize[8] = {
    0x00,
    0x00,
    0x00,
    0x10,
    0x38,
    0x10,
    0x00,
    0x00,
};
__attribute__((used, aligned(4)))
surfaceType_t s_entitySurface[6] = {
    2,
    0,
    0,
    0,
    0,
    0,
};

#define s_entitySurface (s_entitySurface[0])

extern GfxBackEndData *frontEndDataOut;
extern r_global_permanent_t rgp;
extern r_globals_t rg;
extern refimport_t ri;
extern const dvar_t **r_dlightLimit;
extern void qsort(void *base, unsigned int nmemb, unsigned int size, int (*compar)(const void *, const void *));

extern const float *colorWhite;

extern void AxisCopy(vec3_t *in, vec3_t *out);
extern void R_Error(int level, const char *msg, ...);
extern const char *va(const char *format, ...);

void R_UpdateXModelBounds(GfxSceneEntity *sceneEnt, GfxEntity *ent);
void R_SkinSceneDObj(GfxSceneEntity *sceneEnt, GfxEntity *ent);
void R_ClearDpvsScene(void);
void R_DrawModel(int entIndex);
void R_AddScaledDebugString(const char *pos, const char *tag, const char *origin, const char *color, const char *str);
int XSurfaceGetNumTris(XSurface *xsurf);
int XSurfaceGetNumVerts(XSurface *xsurf);
const char *XModelGetName(void *model);
const char *DObjGetModel(void *dobj, int lod);
void MatrixForViewer(float (*mtx)[4], const vec_t *origin, vec3_t *axis);
void InfinitePerspectiveMatrix(float (*mtx)[4], float fov_x, float fov_y, float zNear);
void MatrixMultiply44(const float (*in1)[4], const float (*in2)[4], float (*out)[4]);
void MatrixInverse44(const float *mat, float *dst);
void R_AddCmdClearScreen(int whichToClear, const vec_t *color, float depth, int stencil);
extern double tan(double);

static int R_CompareDumpSceneEntities(const void *e0, const void *e1);
static int R_CompareDrawSurfs(const void *drawSurf0, const void *drawSurf1);
void R_UpdateGfxEntityBounds(GfxEntity *ent);
void R_SkinGfxEntity(GfxEntity *ent);
void R_DecomposeSort(unsigned int sortValue, int *entIndex, const Material **material, int *lmapIndex);
void R_ClearScene(void);
void R_AddLightToScene(const vec_t *org, float radius, float r, float g, float b);
void R_DefaultVertexFrames(int vertCount, GfxWorldVertex *verts);
void qsortDrawSurfs(GfxDrawSurf *drawSurfs, int drawSurfCount);
void R_AddXModelSurfaces(int entIndex);
void R_AddBModelSurfaces(GfxSceneEntity *sceneEnt, int entIndex);
void R_AddPolyToScene(MaterialHandle materialHandle, int lmapIndex, int vertCount, const GfxWorldVertex *verts);
void R_AddDrawSurfForSurface(GfxSurface *surf, int entIndex);
static void __attribute_regparm__(2) R_SetViewParmsForScene(const refdef_t *refdef, GfxViewParms *viewParms);
void R_SetLodOrigin(const refdef_t *refdef);
static void __attribute_regparm__(1) R_AddClearCommandsForFrameBuffer(int dynamicShadowType);
void R_RenderScene(const refdef_t *refdef);
int R_AddStaticModelToScene(int smodelIndex);
GfxEntity *R_AddRefEntityToScene(const GfxEntity *refEnt, GfxModel sceneModel, const struct centity_s *cent);

static int R_CompareDumpSceneEntities(const void *e0, const void *e1)
{
    int index0, index1;
    GfxEntity *ent0, *ent1;
    GfxSceneEntity *sceneEnt0, *sceneEnt1;
    int type;
    int diff;

    index0 = *(const int *)e0;
    index1 = *(const int *)e1;
    ent0 = &frontEndDataOut->entities[index0];
    ent1 = &frontEndDataOut->entities[index1];

    type = ent0->reType;
    if (type != (int)ent1->reType)
        return type - (int)ent1->reType;

    sceneEnt0 = &scene.sceneEnts[index0];
    sceneEnt1 = &scene.sceneEnts[index1];

    switch (type) {
    case 0:
        return (int)DObjGetModel((void *)sceneEnt0->u.obj, 0) - (int)DObjGetModel((void *)sceneEnt1->u.obj, 0);
    case 1:
    case 2:
        return (int)((byte *)sceneEnt0->u.data - (byte *)sceneEnt1->u.data);
    case 3:
        return ((int)((byte *)sceneEnt0->u.data - (byte *)sceneEnt1->u.data)) >> 5;
    default:
        diff = ((int)((byte *)ent0->customMaterial - (byte *)ent1->customMaterial)) >> 2;
        return -(diff * 0x0F0F0F0F);
    }
}

void R_UpdateGfxEntityBounds(GfxEntity *ent)
{
    int entIndex;

    entIndex = ent - scene.def.entities;
    R_UpdateXModelBounds(&scene.sceneEnts[entIndex], ent);
}

void R_SkinGfxEntity(GfxEntity *ent)
{
    int entIndex;
    GfxSceneEntity *sceneEnt;

    entIndex = ent - scene.def.entities;
    sceneEnt = &scene.sceneEnts[entIndex];
    R_UpdateXModelBounds(sceneEnt, ent);
    R_SkinSceneDObj(sceneEnt, ent);
}

void R_DecomposeSort(unsigned int sortValue, int *entIndex, const Material **material, int *lmapIndex)
{
    int ent, matIndex;

    ent = (((int)sortValue >= 0) ? (sortValue >> 4) : (sortValue >> 19)) & 0xfff;
    if (ent >= 0x800)
        ent = 0x7fe;
    *entIndex = ent;

    matIndex = (((int)sortValue >= 0) ? (sortValue >> 21) : (sortValue >> 9)) & 0x3ff;
    *material = rgp.sortedMaterials[matIndex];

    *lmapIndex = (((int)sortValue >= 0) ? (sortValue >> 16) : (sortValue >> 4)) & 0x1f;
}

static int R_CompareDrawSurfs(const void *drawSurf0, const void *drawSurf1)
{
    const GfxDrawSurf *surf0;
    const GfxDrawSurf *surf1;

    surf0 = (const GfxDrawSurf *)drawSurf0;
    surf1 = (const GfxDrawSurf *)drawSurf1;
    if (surf0->sort < surf1->sort) {
        return -1;
    }

    if (surf0->sort > surf1->sort) {
        return 1;
    }

    return 0;
}

void R_ClearScene(void)
{
    GfxBackEndData *buf;

    scene.viewCount++;
    scene.dlightCount = 0;
    scene.drawSurfCount = 0;
    buf = frontEndDataOut;
    scene.drawSurfs = &buf->drawSurfs[buf->drawSurfCount];
    scene.polyCount = 0;
    scene.def.entityCount = 0;
    scene.def.entities = &buf->entities[buf->entityCount];
    scene.sceneEntMaterialCount = 0;
    R_ClearDpvsScene();
}

void R_AddLightToScene(const vec_t *org, float radius, float r, float g, float b)
{
    GfxLight *light;
    int dlightCount;
    r_global_permanent_t *rgpp;

    if (!rg.registered)
        return;
    rgpp = &rgp;
    if (!rgpp->world)
        return;
    if (radius <= 0.0f)
        return;

    dlightCount = scene.dlightCount;
    if (dlightCount >= (*(const dvar_t **)imp_r_dlightLimit)->current.integer)
        return;

    light = &scene.dlights[dlightCount];
    scene.dlightCount = dlightCount + 1;
    light->def = rgpp->dlightDef;
    light->position[0] = org[0];
    light->position[1] = org[1];
    light->position[2] = org[2];
    light->position[3] = radius;
    light->color[0] = r;
    light->color[1] = g;
    light->color[2] = b;
}

void R_DefaultVertexFrames(int vertCount, GfxWorldVertex *verts)
{
    for (int i = 0; i < vertCount; i++) {

        float *normal = (float *)&verts[i].normal;
        normal[0] = 0.0f;
        normal[1] = 0.0f;
        normal[2] = 1.0f;

        float *binormal = (float *)&verts[i].binormal;
        binormal[0] = 0.0f;
        binormal[1] = 1.0f;
        binormal[2] = 0.0f;

        float *tangent = (float *)&verts[i].tangent;
        tangent[0] = 1.0f;
        tangent[1] = 0.0f;
        tangent[2] = 0.0f;
    }
}

void qsortDrawSurfs(GfxDrawSurf *drawSurfs, int drawSurfCount)
{
    if (drawSurfCount > 1) {
        qsort(drawSurfs, (unsigned int)drawSurfCount, sizeof(*drawSurfs), R_CompareDrawSurfs);
    }
}

void R_AddXModelSurfaces(int entIndex)
{
    GfxSceneEntity *sceneEnt;
    GfxEntity *ent;
    GfxModelSurface *modelSurf;
    const Material *material;
    GfxBackEndData *buf;
    GfxDrawSurf *drawSurf;
    unsigned int sortValue;
    int i, surfCount, entIdx, surfType;
    int totalTriCount, totalVertCount;
    r_globals_t *rgg;

    sceneEnt = &scene.sceneEnts[entIndex];
    ent = &scene.def.entities[entIndex];
    modelSurf = (GfxModelSurface *)sceneEnt->surfs;
    surfCount = sceneEnt->surfCount;
    totalTriCount = 0;
    totalVertCount = 0;

    for (i = 0; i < surfCount; i++) {
        material = sceneEnt->materials[i];
        if (!material)
            continue;

        entIdx = (modelSurf->surfType == 5) ? 0x7ff : entIndex;

        buf = frontEndDataOut;
        if (buf->drawSurfCount <= 0xffff) {
            drawSurf = &buf->drawSurfs[buf->drawSurfCount];
            surfType = modelSurf->surfType;
            if (surfType == 2 && (*(const dvar_t **)imp_fx_sort)->current.enabled) {
                sortValue = (unsigned int)((entIdx << 19) + (material->info.sortedIndex << 9) + 0x800001f2u);
            } else {
                sortValue = (unsigned int)(surfType + (entIdx << 4) + (material->info.sortedIndex << 21) + 0x1f0000);
            }
            drawSurf->sort = sortValue;
            drawSurf->surface = (const surfaceType_t *)modelSurf;
            scene.drawSurfCount++;
            buf->drawSurfCount++;
        }

        if ((*(const dvar_t **)imp_r_showTriCounts)->current.enabled)
            totalTriCount += XSurfaceGetNumTris(modelSurf->xsurf);
        else if ((*(const dvar_t **)imp_r_showVertCounts)->current.enabled)
            totalVertCount += XSurfaceGetNumVerts(modelSurf->xsurf);

        modelSurf = (GfxModelSurface *)((byte *)modelSurf + s_XModelSurfaceSize[modelSurf->surfType]);
    }

    rgg = &rg;
    if ((*(const dvar_t **)imp_r_showTriCounts)->current.enabled) {
        R_AddScaledDebugString(
            (char *)&frontEndDataOut->debugGlobals,
            (char *)rgg->debugViewParms,
            (const char *)&ent->origin,
            (const char *)imp_colorCyan,
            va("%i", totalTriCount));
    } else if ((*(const dvar_t **)imp_r_showVertCounts)->current.enabled) {
        R_AddScaledDebugString(
            (char *)&frontEndDataOut->debugGlobals,
            (char *)rgg->debugViewParms,
            (const char *)&ent->origin,
            (const char *)imp_colorCyan,
            va("%i", totalVertCount));
    } else if ((*(const dvar_t **)imp_r_showSurfCounts)->current.enabled) {
        R_AddScaledDebugString(
            (char *)&frontEndDataOut->debugGlobals,
            (char *)rgg->debugViewParms,
            (const char *)&ent->origin,
            (const char *)imp_colorCyan,
            va("%i", surfCount));
    }
}

void R_AddBModelSurfaces(GfxSceneEntity *sceneEnt, int entIndex)
{
    const GfxBrushModel *bmodel;
    GfxBackEndData *buf;
    GfxDrawSurf *drawSurf;
    GfxSurface *surf;
    const Material *material;
    const surfaceType_t *surface;
    int lmapIndex;
    int surfType;
    unsigned int sortValue;
    int count;
    int dvarVal;

    sceneEnt->cullState = 5;

    dvarVal = (*(const dvar_t **)imp_r_drawBModels)->current.enabled;
    if (!dvarVal)
        return;

    bmodel = sceneEnt->u.bmodel;
    if (bmodel->surfaceCount <= 0)
        return;

    for (count = 0; count < bmodel->surfaceCount; count++) {
        surf = &rgp.world->surfaces[bmodel->startSurfIndex + count];
        lmapIndex = surf->lightmapIndex;
        material = surf->material;
        surface = surf->data;

        buf = frontEndDataOut;
        if (buf->drawSurfCount > 0xffff)
            continue;

        drawSurf = &buf->drawSurfs[buf->drawSurfCount];
        surfType = *surface;

        if (surfType == 2 && (*(const dvar_t **)imp_fx_sort)->current.enabled) {
            sortValue = (material->info.sortedIndex << 9) + ((entIndex << 19) + 0x80000002) + (lmapIndex << 4);
        } else {
            sortValue = surfType + (entIndex << 4) + (material->info.sortedIndex << 21) + (lmapIndex << 16);
        }

        drawSurf->sort = sortValue;
        drawSurf->surface = surface;
        scene.drawSurfCount++;
        frontEndDataOut->drawSurfCount++;
    }
}

void R_AddPolyToScene(MaterialHandle materialHandle, int lmapIndex, int vertCount, const GfxWorldVertex *verts)
{
    GfxBackEndData *buf;
    srfPoly_t *poly;
    GfxDrawSurf *drawSurf;
    GfxWorldVertex *destVerts;
    unsigned int sortValue;
    int entIndex;
    int surfType;
    int vc;

    if (!rg.registered)
        return;

    if (!materialHandle)
        materialHandle = rgp.defaultMaterial;

    buf = frontEndDataOut;
    vc = (unsigned short)vertCount;

    if (vc + buf->polyVertCount > 0x2000)
        return;
    if (buf->polyCount > 0x7ff)
        return;

    poly = &buf->polys[buf->polyCount];
    poly->surfaceType = 1;
    poly->material = materialHandle;
    poly->lmapIndex = (unsigned short)lmapIndex;
    poly->vertCount = (unsigned short)vertCount;

    destVerts = &buf->polyVerts[buf->polyVertCount];
    poly->verts = destVerts;
    memcpy(destVerts, verts, vc * sizeof(GfxWorldVertex));

    entIndex = (scene.polyCount & 0x7FF) + 0x800;

    buf = frontEndDataOut;
    if (buf->drawSurfCount <= 0xffff) {
        drawSurf = &buf->drawSurfs[buf->drawSurfCount];
        surfType = poly->surfaceType;

        if (surfType == 2 && (*(const dvar_t **)imp_fx_sort)->current.enabled) {
            sortValue = (entIndex << 19) + (materialHandle->info.sortedIndex << 9) + 0x80000002u + (lmapIndex << 4);
        } else {
            sortValue = surfType + (entIndex << 4) + (materialHandle->info.sortedIndex << 21) + (lmapIndex << 16);
        }

        drawSurf->sort = sortValue;
        drawSurf->surface = (const surfaceType_t *)poly;
        scene.drawSurfCount++;
        frontEndDataOut->drawSurfCount++;
    }

    frontEndDataOut->polyCount++;
    frontEndDataOut->polyVertCount += vc;
    scene.polyCount++;
}

void R_dpvs_diag_print(int cameraCellIndex, int drawWorld, int cellPtr)
{
    (void)cameraCellIndex;
    (void)drawWorld;
    (void)cellPtr;
}

void R_AddDrawSurfForSurface(GfxSurface *surf, int entIndex)
{
    GfxBackEndData *buf;
    GfxDrawSurf *drawSurf;
    const Material *material;
    const surfaceType_t *surface;
    unsigned int sortValue;
    int lmapIndex;
    int surfType;

    lmapIndex = surf->lightmapIndex;
    material = surf->material;
    surface = surf->data;

    buf = frontEndDataOut;
    if (buf->drawSurfCount > 0xffff)
        return;

    drawSurf = &buf->drawSurfs[buf->drawSurfCount];
    surfType = *surface;

    if (surfType == 2 && (*(const dvar_t **)imp_fx_sort)->current.enabled) {
        sortValue = (entIndex << 19) + (material->info.sortedIndex << 9) + 0x80000002u + (lmapIndex << 4);
    } else {
        sortValue = surfType + (entIndex << 4) + (material->info.sortedIndex << 21) + (lmapIndex << 16);
    }

    drawSurf->sort = sortValue;
    drawSurf->surface = surface;
    scene.drawSurfCount++;
    frontEndDataOut->drawSurfCount++;
}

static void __attribute_regparm__(2)
    R_SetViewParmsForScene(const refdef_t *refdef, GfxViewParms *viewParms)
{
    float zNear;

    memset(viewParms, 0, sizeof(*viewParms));

    viewParms->viewport.X = refdef->x;
    viewParms->viewport.Y = refdef->y;
    viewParms->viewport.Width = refdef->width;
    viewParms->viewport.Height = refdef->height;
    viewParms->viewport.MinZ = 0.0f;
    viewParms->viewport.MaxZ = 1.0f;

    viewParms->origin[0] = refdef->vieworg[0];
    viewParms->origin[1] = refdef->vieworg[1];
    viewParms->origin[2] = refdef->vieworg[2];
    viewParms->axis[0][0] = refdef->viewaxis[0][0];
    viewParms->axis[0][1] = refdef->viewaxis[0][1];
    viewParms->axis[0][2] = refdef->viewaxis[0][2];
    viewParms->axis[1][0] = refdef->viewaxis[1][0];
    viewParms->axis[1][1] = refdef->viewaxis[1][1];
    viewParms->axis[1][2] = refdef->viewaxis[1][2];
    viewParms->axis[2][0] = refdef->viewaxis[2][0];
    viewParms->axis[2][1] = refdef->viewaxis[2][1];
    viewParms->axis[2][2] = refdef->viewaxis[2][2];

    MatrixForViewer((float (*)[4]) & viewParms->viewMatrix, viewParms->origin, viewParms->axis);

    zNear = refdef->zNear;
    if (!(zNear > 0.0f)) {
        zNear = (*(const dvar_t **)imp_r_znear)->current.value;
        if (zNear < 0.01f) {
            zNear = 0.01f;
        }
    }

    InfinitePerspectiveMatrix((float (*)[4]) & viewParms->projectionMatrix, refdef->fov_x, refdef->fov_y, zNear);
    viewParms->depthHackNearClip = (*(const dvar_t **)imp_r_znear_depthhack)->current.value;

    MatrixMultiply44(
        (const float (*)[4]) & viewParms->viewMatrix,
        (const float (*)[4]) & viewParms->projectionMatrix,
        (float (*)[4]) & viewParms->viewProjectionMatrix);
    MatrixInverse44((const float *)&viewParms->viewProjectionMatrix, (float *)&viewParms->inverseViewProjectionMatrix);
}

void R_SetLodOrigin(const refdef_t *refdef)
{
    const dvar_t *lockPvs;
    float invFovScale;
    r_globals_t *rgg;

    lockPvs = *(const dvar_t **)imp_r_lockPvs;
    if (lockPvs->modified) {
        ri.Dvar_ClearModified(lockPvs);
        R_SetViewParmsForScene(refdef, &lockPvsViewParms);
    }

    rgg = &rg;
    if (lockPvs->current.enabled) {
        rgg->lodParms.origin[0] = lockPvsViewParms.origin[0];
        rgg->lodParms.origin[1] = lockPvsViewParms.origin[1];
        rgg->lodParms.origin[2] = lockPvsViewParms.origin[2];
    } else {
        rgg->lodParms.origin[0] = refdef->vieworg[0];
        rgg->lodParms.origin[1] = refdef->vieworg[1];
        rgg->lodParms.origin[2] = refdef->vieworg[2];
    }

    rgg->lodParms.scale = (*(const dvar_t **)imp_r_lodScale)->current.value;
    rgg->lodParms.bias = (*(const dvar_t **)imp_r_lodBias)->current.value;

    if (refdef->fov_x != 80.0f) {
        invFovScale =
            (float)(tan((double)(refdef->fov_x * 0.5f) * 0.017453292519943295) / tan(0.69813170079773179));
        rgg->lodParms.scale *= invFovScale;
        rgg->lodParms.bias *= invFovScale;
    }
}

static void __attribute_regparm__(1)
    R_AddClearCommandsForFrameBuffer(int dynamicShadowType)
{
    int whichToClear;
    vec4_t clearColor;
    const float inv255 = 0.003921568859368563f;
    r_globals_t *rgg;

    whichToClear = 7;
    if (dx.multiSampleQuality != 0 && dynamicShadowType == 1) {
        whichToClear = 1;
    }

    rgg = &rg;
    if (rgg->fogSettings[2].registered != 0) {
        clearColor[0] = rgg->fogSettings[2].color.array[2] * inv255;
        clearColor[1] = rgg->fogSettings[2].color.array[1] * inv255;
        clearColor[2] = rgg->fogSettings[2].color.array[0] * inv255;
    } else {
        const byte *dvarColor = (*(const dvar_t **)imp_r_clearColor)->current.color;

        clearColor[0] = dvarColor[0] * inv255;
        clearColor[1] = dvarColor[1] * inv255;
        clearColor[2] = dvarColor[2] * inv255;
    }

    clearColor[3] = 1.0f;
    R_AddCmdClearScreen(whichToClear, clearColor, 1.0f, 0);
}

static void R_RenderScene_diag(int registered, int norefresh, int drawSurfCount)
{
    (void)registered;
    (void)norefresh;
    (void)drawSurfCount;
}
extern unsigned char dpvsGlob[];
static void R_WorldCheck_diag(void *rgp_field, void *cell_ptr, int cellIdx)
{
    (void)rgp_field;
    (void)cell_ptr;
    (void)cellIdx;
}

extern void *R_AllocViewParms(void);
extern int R_CellForPoint(const void *viewParms);
extern void R_AddWorldSurfacesDpvs(const void *viewParms, int cellIdx);
extern void CG_AddMarks(void);
extern void FX_DrawScheduledEffects(void);
extern int R_BeginDrawGroupSection(int section);
extern void R_BeginDrawGroupLoop(int section, int viewIndex);
extern int R_EndDrawGroupLoop(int section, int viewIndex);
extern void R_EndDrawGroupSection(int section);
extern void R_AddCmdBeginView(int entityCount, const GfxSceneDef *sceneDef, const GfxViewParms *viewParms, const GfxLodParms *lodParms);
extern void R_AddCmdSetRenderTarget(int target);
extern void R_AddCmdDrawSurfs(void *drawSurfs, int drawSurfCount, int techType);
extern void R_AddCmdDrawSun(int viewIndex);
extern void R_AddCmdDrawSunPostEffects(int viewIndex);
extern void R_UnlockSkinnedCache(void);
extern void R_AddCmdApplyEarlyPostEffects(void);
extern void R_AddCmdApplyLatePostEffects(float blurRadius);
extern int R_GetPointLightPartitions(void *drawSurfs, int drawSurfCount, void *partitions, int maxPartitions);
extern void R_AddCmdLightProperties(int index, const void *light);
extern void R_AddCmdDrawFullScreenColoredQuad(float x, float y, float w, float h, const void *material, const float *color);
extern void R_AddCmdSetViewport(int x, int y, int w, int h);
extern void Com_Printf(const char *fmt, ...);
void R_RenderScene(const refdef_t *refdef)
{
    r_globals_t *rg_p = &rg;
    r_global_permanent_t *rgp_p = &rgp;
    void *viewParms;
    void *viewParmsDraw;
    int drawSurfStart, drawSurfCount;
    int viewIndex;
    float blurRadius;
    byte isSplitscreen;
    int pointLightPartitions[256 * 3];
    int pointLightCount;
    int debugEntIndices[2048];

    R_RenderScene_diag(rg_p->registered, (*(const dvar_t **)imp_r_norefresh)->current.enabled,
                       (int)rgp_p->world ? 1 : 0);

    if (!rg_p->registered)
        return;
    if ((*(const dvar_t **)imp_r_norefresh)->current.enabled)
        return;

    drawSurfCount = 0;
    {
        void *world = rgp_p->world;
        if (!world) {
            R_Error(1, "R_RenderScene: no world loaded");

        }
    }

    scene.def.time = ((const refdef_t *)refdef)->time;
    scene.def.floatTime = (float)((const refdef_t *)refdef)->time * 0.001f;

    memcpy(rg_p->viewOrg, refdef->vieworg, 12);
    memcpy(rg_p->viewDir, refdef->viewaxis[0], 12);

    viewParms = R_AllocViewParms();
    R_SetViewParmsForScene(refdef, (GfxViewParms *)viewParms);

    blurRadius = ((const refdef_t *)refdef)->blurRadius;

    {
        char *vidCfg = (char *)imp_vidConfig;
        if (((const refdef_t *)refdef)->width == ((const vidConfig_t *)imp_vidConfig)->width)
            isSplitscreen = 0;
        else
            isSplitscreen = 0;
        (void)isSplitscreen;
    }

    viewParmsDraw = viewParms;
    if ((*(const dvar_t **)imp_r_lockPvs)->current.enabled)
        viewParmsDraw = &lockPvsViewParms;

    {
        r_globals_t *rg_ptr = &rg;
        GfxFog *fogActive = &rg_ptr->fogSettings[2];
        GfxFog *fogPrev = &rg_ptr->fogSettings[3];
        GfxFog *fogTarget = &rg_ptr->fogSettings[4];
        int sceneTime = scene.def.time;
        int sunTime = fogTarget->finishTime;

        if (sceneTime >= sunTime) {

            memcpy(fogActive, fogTarget, sizeof(GfxFog));
        } else {
            int prevTime = fogTarget->startTime;
            if (prevTime != fogTarget->techniqueOffset) {

                int duration = sunTime - prevTime;
                float frac;
                if (duration <= 0) {
                    frac = 1.0f;
                } else {
                    frac = (float)(sceneTime - prevTime) / (float)duration;
                    if (frac > 1.0f)
                        frac = 1.0f;
                }

                int i;
                for (i = 0; i < 3; i++) {
                    float prev = *((float *)&fogPrev->fogStart + i);
                    float curr = *((float *)&fogTarget->fogStart + i);
                    *((float *)&fogActive->fogStart + i) = prev + (curr - prev) * frac;
                }

                for (i = 0; i < 4; i++) {
                    byte prev = ((byte *)&fogPrev->color)[i];
                    byte curr = ((byte *)&fogTarget->color)[i];
                    ((byte *)&fogActive->color)[i] = (byte)(prev + (int)(curr - prev) * frac);
                }
                fogActive->techniqueOffset = fogTarget->techniqueOffset;
                fogActive->registered = 1;
                fogActive->clearScreen = fogTarget->clearScreen ? 1 : fogPrev->clearScreen;
            } else {
                memcpy(fogActive, fogTarget, sizeof(GfxFog));
                fogTarget->finishTime = 0;
            }
        }

        if (rg_ptr->fogIndex) {
            GfxBackEndData *fed = frontEndDataOut;
            memcpy(&fed->fogSettings, fogActive, sizeof(GfxFog));
        } else {
            GfxBackEndData *fed = frontEndDataOut;
            fed->fogSettings.techniqueOffset = 0;
        }
    }

    viewIndex = ((const refdef_t *)refdef)->viewIndex;

    {
        int cellIdx;
        R_WorldCheck_diag(rgp_p->world, NULL, 0);
        cellIdx = R_CellForPoint(viewParmsDraw);
        {
            GfxWorld *world = rgp_p->world;
            if (world && world->cells) {
                R_AddWorldSurfacesDpvs(viewParmsDraw, cellIdx);
            }
        }
    }

    CG_AddMarks();
    FX_DrawScheduledEffects();

#ifdef __EMSCRIPTEN__
    extern void WebPointLights_SetScene(const GfxLight *, int);
    WebPointLights_SetScene(scene.dlights, scene.dlightCount);
#endif

    drawSurfStart = (int)(intptr_t)scene.drawSurfs;
    drawSurfCount = scene.drawSurfCount;
    qsortDrawSurfs((GfxDrawSurf *)(intptr_t)drawSurfStart, drawSurfCount);

    {
        int isDx7 = (r_rendererInUse->current.integer == 2);
        if (!isDx7 && (*(const dvar_t **)imp_r_dlightLimit)->current.integer) {
            pointLightCount = R_GetPointLightPartitions(
                (void *)(intptr_t)drawSurfStart, drawSurfCount,
                pointLightPartitions, 0x100);
        } else {
            pointLightCount = 0;
        }
    }

    {
        int isFullbright = (*(const dvar_t **)imp_r_fullbright)->current.enabled;
        int isDx7 = (r_rendererInUse->current.integer == 2);
        const GfxLodParms *lodParms = &rg_p->lodParms;

        if (isFullbright) {

            if (!R_BeginDrawGroupSection(3)) {
                R_AddCmdSetRenderTarget(0);
                {
                    R_AddCmdSetViewport(0, 0, ((const vidConfig_t *)imp_vidConfig)->width, ((const vidConfig_t *)imp_vidConfig)->height);
                }
                R_AddClearCommandsForFrameBuffer(0);
            }
            R_BeginDrawGroupLoop(3, viewIndex);
            R_AddCmdBeginView(scene.viewCount, &scene.def, viewParms, lodParms);
            R_AddCmdDrawSurfs((void *)(intptr_t)drawSurfStart, drawSurfCount, 3);
            R_AddCmdDrawSun(viewIndex);
            R_EndDrawGroupLoop(3, viewIndex);
            R_EndDrawGroupSection(3);

            if (!R_BeginDrawGroupSection(4)) {
                R_AddCmdSetViewport(0, 0, ((const vidConfig_t *)imp_vidConfig)->width, ((const vidConfig_t *)imp_vidConfig)->height);
            }
            R_BeginDrawGroupLoop(4, viewIndex);
            R_AddCmdBeginView(scene.viewCount, &scene.def, viewParms, lodParms);
        } else if (isDx7) {

            R_AddCmdBeginView(scene.viewCount, &scene.def, viewParms, lodParms);
            R_AddCmdSetRenderTarget(0);
            R_AddClearCommandsForFrameBuffer(0);
            {
                GfxWorld *world = rgp_p->world;
                R_AddCmdLightProperties(0, (char *)&world->sunLight);
            }
            R_AddCmdDrawSurfs((void *)(intptr_t)drawSurfStart, drawSurfCount, 1);
            R_AddCmdDrawSurfs((void *)(intptr_t)drawSurfStart, drawSurfCount, 6);
            R_AddCmdDrawSun(viewIndex);
            R_AddCmdDrawSurfs((void *)(intptr_t)drawSurfStart, drawSurfCount, 0x15);
        } else if ((*(const dvar_t **)imp_r_debugShader)->current.integer) {

            R_AddCmdBeginView(scene.viewCount, &scene.def, viewParms, lodParms);
            R_AddCmdSetRenderTarget(0);
            R_AddClearCommandsForFrameBuffer(0);
            R_AddCmdDrawSurfs((void *)(intptr_t)drawSurfStart, drawSurfCount, 0x21);
        } else {

            if (!R_BeginDrawGroupSection(2)) {
                R_AddCmdSetRenderTarget(0);
                {
                    R_AddCmdSetViewport(0, 0, ((const vidConfig_t *)imp_vidConfig)->width, ((const vidConfig_t *)imp_vidConfig)->height);
                }
                R_AddClearCommandsForFrameBuffer(0);
                {
                    GfxWorld *world = rgp_p->world;
                    R_AddCmdLightProperties(0, (char *)&world->sunLight);
                }
            }
            R_BeginDrawGroupLoop(2, viewIndex);
            R_AddCmdBeginView(scene.viewCount, &scene.def, viewParms, lodParms);
            R_AddCmdDrawSurfs((void *)(intptr_t)drawSurfStart, drawSurfCount, 1);
            R_AddCmdDrawSurfs((void *)(intptr_t)drawSurfStart, drawSurfCount, 6);
            R_AddCmdDrawSun(viewIndex);
            R_EndDrawGroupLoop(2, viewIndex);
            if (!isSplitscreen)
                R_AddCmdApplyEarlyPostEffects();
            R_EndDrawGroupSection(2);

            R_BeginDrawGroupSection(3);
            R_BeginDrawGroupLoop(3, viewIndex);
            R_AddCmdBeginView(scene.viewCount, &scene.def, viewParms, lodParms);

            if (pointLightCount > 0) {
                int p;
                for (p = 0; p < pointLightCount; p++) {
                    int *part = &pointLightPartitions[p * 3];
                    void *light = (void *)(intptr_t)part[0];
                    void *pDrawSurfs = (void *)((char *)scene.drawSurfs + part[1] * 8);
                    int pDrawSurfCount = part[2];
                    void *world = rgp_p->world;
                    R_AddCmdDrawFullScreenColoredQuad(0, 0, 1.0f, 1.0f, ((r_global_permanent_t *)rgp_p)->clearAlphaStencilMaterial, (const float *)imp_colorWhite);
                    R_AddCmdLightProperties(0, light);
                    R_AddCmdDrawSurfs(pDrawSurfs, pDrawSurfCount, 0x12);
                }
            }

            R_AddCmdDrawSurfs((void *)(intptr_t)drawSurfStart, drawSurfCount, 0x15);
            R_EndDrawGroupLoop(3, viewIndex);
            R_EndDrawGroupSection(3);

            if (!R_BeginDrawGroupSection(4)) {
                R_AddCmdSetViewport(0, 0, ((const vidConfig_t *)imp_vidConfig)->width, ((const vidConfig_t *)imp_vidConfig)->height);
            }
            R_BeginDrawGroupLoop(4, viewIndex);
            R_AddCmdBeginView(scene.viewCount, &scene.def, viewParms, lodParms);
            if (!isSplitscreen)
                R_AddCmdApplyLatePostEffects(blurRadius);
            R_AddCmdDrawSunPostEffects(viewIndex);
        }

        {
            int showTris = (*(const dvar_t **)imp_r_showTris)->current.integer;
            if (showTris) {
                if (showTris & 2)
                    R_AddCmdClearScreen(6, (const vec_t *)imp_colorWhite, 1.0f, 0);
                R_AddCmdDrawSurfs((void *)(intptr_t)drawSurfStart, drawSurfCount, 0x1d);
            }
        }

        R_UnlockSkinnedCache();
    }

    {
        int debugEntCounts = (*(const dvar_t **)imp_r_debugEntCounts)->current.integer;
        if (debugEntCounts && debugEntCounts < scene.def.entityCount) {

            ri.Dvar_SetInt(*(const dvar_t **)imp_r_debugEntCounts, 0);

        }
    }
}

void R_DebugStaticModels(const float *view)
{
    GfxWorld *world = rgp.world;
    int i;
    if (!world)
        return;
    ri.Printf(0, "[smodels] count=%d view=%d\n", world->smodelCount, scene.viewCount);
    for (i = 0; i < world->smodelCount; ++i) {
        GfxStaticModelInstance *m = &world->smodelInsts[i];
        const char *name = XModelGetName(m->model);
        if (!strstr(name, "vehicle_"))
            continue;
        ri.Printf(0, "[smodels] %d %s origin=(%.1f %.1f %.1f) min=(%.1f %.1f %.1f) max=(%.1f %.1f %.1f) scale=%.2f cull=%.1f last=%d lodscale=%.2f bias=%.1f\n",
                  i, name, m->origin[0], m->origin[1], m->origin[2],
                  m->mins[0], m->mins[1], m->mins[2], m->maxs[0], m->maxs[1], m->maxs[2],
                  m->scale, m->cullDist, ((int *)rg.smodelDyncs)[i * 2], rg.lodParms.scale, rg.lodParms.bias);
        for (int j = 0; j < scene.def.entityCount; ++j) {
            GfxEntity *ent = &scene.def.entities[j];
            GfxSceneEntity *se = &scene.sceneEnts[j];
            if (ent->reType == 2 && se->u.model == m->model && ent->origin[0] == m->origin[0])
                ri.Printf(0, "[smodels] scene=%d cull=%d surfaces=%d type=%d skins=%p material=%s\n",
                          j, se->cullState, se->surfCount, se->surfs ? *se->surfs : -1,
                          m->model->xskins, se->surfCount && se->materials[0] ? se->materials[0]->info.name : "none");
        }
    }
}

int R_AddStaticModelToScene(int smodelIndex)
{
    GfxBackEndData *buf;
    GfxEntity *backEndRefEnt;
    GfxSceneEntity *sceneEnt;
    GfxStaticModelInstance *smodelInst;
    GfxWorld *world;
    int entIndex;
    refimport_t *rii = &ri;

    if (!(*(const dvar_t **)imp_r_drawEntities)->current.enabled)
        return -1;

    entIndex = scene.def.entityCount;
    if ((unsigned int)entIndex > 0x7fd) {
        buf = frontEndDataOut;
        if (buf->frameCount == warnCount_007f1dcc)
            return -1;
        warnCount_007f1dcc = buf->frameCount;
        rii->Printf(2, "too many scene entities (more than %%i)\n", 0x7fe);
        return -1;
    }

    buf = frontEndDataOut;
    if ((unsigned int)buf->entityCount > 0x1ff7) {
        if (buf->frameCount == warnCount_007f1dcc)
            return -1;
        warnCount_007f1dcc = buf->frameCount;
        rii->Printf(2, "too many visible models (more than %i)\n", 0x1ff8);
        return -1;
    }

    buf->entityCount++;
    scene.def.entityCount++;

    if (entIndex < 0)
        return -1;

    backEndRefEnt = &scene.def.entities[entIndex];
    world = rgp.world;
    smodelInst = &world->smodelInsts[smodelIndex];

    memset(backEndRefEnt, 0, sizeof(GfxEntity));
    backEndRefEnt->reType = 2;
    backEndRefEnt->origin[0] = smodelInst->origin[0];
    backEndRefEnt->origin[1] = smodelInst->origin[1];
    backEndRefEnt->origin[2] = smodelInst->origin[2];
    AxisCopy(smodelInst->axis, backEndRefEnt->axis);
    backEndRefEnt->scale = smodelInst->scale;

    if (r_rendererInUse->current.integer == 2) {

        backEndRefEnt->lighting.dx7.colorForDir = (FxMemMgr_Emitter * (*)[2])(world->smodelLightingColorTable + smodelIndex);
        *(int *)&backEndRefEnt->lighting.dx7.sunVisibility = ((int *)world->smodelLightingSunVisTable)[smodelIndex];
    } else {
        backEndRefEnt->lighting.baseCoords[0] = smodelInst->baseLightingCoords[0];
        backEndRefEnt->lighting.baseCoords[1] = smodelInst->baseLightingCoords[1];
        backEndRefEnt->lighting.baseCoords[2] = smodelInst->baseLightingCoords[2];
    }

    sceneEnt = &scene.sceneEnts[entIndex];
    memset(sceneEnt, 0, sizeof(*sceneEnt));
    sceneEnt->cullState = 2;
    sceneEnt->u.model = smodelInst->model;
    sceneEnt->curMins[0] = smodelInst->mins[0];
    sceneEnt->curMins[1] = smodelInst->mins[1];
    sceneEnt->curMins[2] = smodelInst->mins[2];
    sceneEnt->curMaxs[0] = smodelInst->maxs[0];
    sceneEnt->curMaxs[1] = smodelInst->maxs[1];
    sceneEnt->curMaxs[2] = smodelInst->maxs[2];

    return entIndex;
}

GfxEntity *R_AddRefEntityToScene(const GfxEntity *refEnt, GfxModel sceneModel, const struct centity_s *cent)
{

    {
        extern int g_gfxV60DllActive;
        extern void *g_gfxV60AddRefEntity;
        if (g_gfxV60DllActive && g_gfxV60AddRefEntity)
            return ((GfxEntity * (*)(const GfxEntity *, GfxModel, const struct centity_s *))
                        g_gfxV60AddRefEntity)(refEnt, sceneModel, cent);
    }
    {
        GfxBackEndData *buf;
        GfxEntity *backEndRefEnt;
        GfxSceneEntity *sceneEnt;
        const Material *material;
        GfxDrawSurf *drawSurf;
        unsigned int sortValue;
        int entIndex, surfType;

        if ((int)refEnt->reType > 1) {

            if ((*(const dvar_t **)imp_com_statmon)->current.enabled) {
                if ((unsigned int)frontEndDataOut->entityCount > 0x1cc4)
                    ri.StatMon_Warning(5, 0xbb8, "gfx/2d/warning@models.jpg");
            }

            if (refEnt->reType > 9)
                R_Error(1, "R_AddRefEntityToScene: bad reType %i", refEnt->reType);

            buf = frontEndDataOut;
            if ((unsigned int)buf->entityCount > 0x1ff7) {
                if (buf->frameCount == warnCount_007f1dd0)
                    return NULL;
                warnCount_007f1dd0 = buf->frameCount;
                ri.Printf(2, "refEntity buffer exceeded - not drawing model\n");
                return NULL;
            }

            if (!(*(const dvar_t **)imp_r_drawEntities)->current.enabled)
                return NULL;

            entIndex = scene.def.entityCount;
            if ((unsigned int)entIndex > 0x7fd) {
                if (buf->frameCount == warnCount_007f1dcc)
                    return NULL;
                warnCount_007f1dcc = buf->frameCount;
                ri.Printf(2, "too many scene entities (more than %i)\n", 0x7fe);
                return NULL;
            }

            buf->entityCount++;
            scene.def.entityCount++;

            if (entIndex < 0)
                return NULL;

            sceneEnt = &scene.sceneEnts[entIndex];
            backEndRefEnt = &scene.def.entities[entIndex];

            memcpy(backEndRefEnt, refEnt, sizeof(GfxEntity));
            if (backEndRefEnt->scale == 0.0f)
                backEndRefEnt->scale = 1.0f;

            memset(sceneEnt, 0, sizeof(*sceneEnt));

            if ((int)refEnt->reType > 3) {

                material = refEnt->customMaterial;
                buf = frontEndDataOut;
                if (buf->drawSurfCount <= 0xffff) {
                    drawSurf = &buf->drawSurfs[buf->drawSurfCount];
                    surfType = s_entitySurface;
                    if (surfType == 2 && (*(const dvar_t **)imp_fx_sort)->current.enabled) {
                        sortValue = (unsigned int)((entIndex << 19) + (material->info.sortedIndex << 9) + 0x800001f2u);
                    } else {
                        sortValue = (unsigned int)(surfType + (entIndex << 4) + (material->info.sortedIndex << 21) + 0x1f0000);
                    }
                    drawSurf->sort = sortValue;
                    drawSurf->surface = &s_entitySurface;
                    scene.drawSurfCount++;
                    buf->drawSurfCount++;
                }
            } else {
                sceneEnt->u = sceneModel;
            }
        } else {

            if (!(*(const dvar_t **)imp_r_drawEntities)->current.enabled)
                return NULL;

            entIndex = scene.def.entityCount;
            if ((unsigned int)entIndex > 0x7fd) {
                buf = frontEndDataOut;
                if (buf->frameCount == warnCount_007f1dcc)
                    return NULL;
                warnCount_007f1dcc = buf->frameCount;
                ri.Printf(2, "too many scene entities (more than %i)\n", 0x7fe);
                return NULL;
            }

            buf = frontEndDataOut;
            if ((unsigned int)buf->entityCount > 0x1ff7) {
                if (buf->frameCount == warnCount_007f1dcc)
                    return NULL;
                warnCount_007f1dcc = buf->frameCount;
                ri.Printf(2, "too many visible models (more than %i)\n", 0x1ff8);
                return NULL;
            }

            buf->entityCount++;
            scene.def.entityCount++;

            if (entIndex < 0)
                return NULL;

            sceneEnt = &scene.sceneEnts[entIndex];
            backEndRefEnt = &scene.def.entities[entIndex];

            memcpy(backEndRefEnt, refEnt, sizeof(GfxEntity));
            if (backEndRefEnt->scale == 0.0f)
                backEndRefEnt->scale = 1.0f;

            memset(sceneEnt, 0, sizeof(*sceneEnt));
            sceneEnt->u = sceneModel;
            sceneEnt->cent = cent;

            if (refEnt->reType == 1)
                R_DrawModel(entIndex);
        }

        return backEndRefEnt;
    }
}
