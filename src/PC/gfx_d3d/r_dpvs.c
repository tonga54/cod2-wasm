#include "common_types.h"
extern GfxBackEndData *frontEndDataOut;
extern refimport_t ri;
extern GfxScene scene;
#include "imports.h"
#include "bytematch.h"

COD2_ASSERT_FIELD(GfxScene, viewCount, 0x0);
COD2_ASSERT_FIELD(GfxScene, def, 0x4);

#define R_DPVS_REGPARM3_ABI COD2_REGPARM(3) BM_NOINLINE

static int dpvsConfig;
static struct DpvsScene dpvsScene;

extern unsigned char dpvsGlob[224];

#define dpvsG (*(struct DpvsGlobals *)(void *)&dpvsGlob)

extern const unsigned char standardFrustumSidePlanes[];

static int g_avis_count = 0;
void R_AddVisSurf_diag_count(void)
{
    g_avis_count++;
}
int R_AddVisSurf_diag_get(void)
{
    int c = g_avis_count;
    g_avis_count = 0;
    return c;
}

extern void R_UpdateXModelBounds(void *sceneEnt, void *ent);
extern void R_SkinSceneEnt(void *sceneEnt, void *ent);
extern void R_AddXModelSurfaces(int entIndex);
extern r_globals_t rg;
extern r_global_permanent_t rgp;
extern void R_Error(int level, const char *fmt, ...);
extern void Vec3Cross(const vec_t *v0, const vec_t *v1, vec_t *cross);
extern float Vec3Normalize(vec_t *v);
extern int BoxOnPlaneSide(const vec_t *emins, const vec_t *emaxs, const cplane_t *p);
extern void ExpandBounds(const vec_t *addedmins, const vec_t *addedmaxs, vec_t *mins, vec_t *maxs);
extern float Vec3Distance(const vec_t *v0, const vec_t *v1);
extern const char *XModelGetName(void *model);
extern void R_AddDebugString(void *debugGlobals, const vec_t *origin, const void *color, float scale, const char *text);
extern void R_SkinStaticModel(void *sceneEnt, void *ent, int smodelIndex);
extern int R_AddStaticModelToScene(int smodelIndex);
extern void R_AddDrawSurfForSurface(void *surf, int entIndex);
void R_DrawModel(int entIndex);
float R_GetFarPlaneDist(void);
void R_ClearDpvsScene(void);
void R_SetCullDist(float dist);
static int R_DPVS_REGPARM3_ABI R_FilterEntityIntoCells_r_impl(mnode_t *node, int entIndex, const vec_t *mins, const vec_t *maxs);
int R_CellForPoint(const vec_t *origin);
static vec3_t *R_DPVS_REGPARM3_ABI R_ChopPortalWinding_impl(vec3_t *vertsIn, int *vertexCount, const float *plane, vec3_t *vertsOut);
static void R_DPVS_REGPARM3_ABI R_GetSidePlaneNormals(vec3_t *winding, int vertexCount, vec3_t *normals);
static void R_DPVS_REGPARM3_ABI R_GetSidePlaneNormals_impl(vec3_t *winding, int vertexCount, vec3_t *normals);
static void R_DPVS_REGPARM3_ABI R_AddStaticModelWithCull(int smodelIndex, const DpvsPlane *planes, int planeCount, int stackLevel);
static void R_DPVS_REGPARM3_ABI R_AddStaticModelWithCull_impl(int smodelIndex, const DpvsPlane *planes, int planeCount, int stackLevel);
void R_FrustumClipPlanes(const D3DMATRIX *viewProjMtx, vec4_t *sidePlanes, int sidePlaneCount, DpvsPlane *frustumPlanes);
static void R_DPVS_REGPARM3_ABI R_AddWorldSurfaceWithCull(int surfIndex, const DpvsPlane *planes, int planeCount, int stackLevel);
static void R_DPVS_REGPARM3_ABI R_AddWorldSurfaceWithCull_impl(int surfIndex, const DpvsPlane *planes, int planeCount, int stackLevel);

static inline float R_DpvsPlaneBoundsTest(const DpvsPlane *plane, const float *bounds)
{
    return plane->coeffs[0] * *(float *)((byte *)bounds + plane->side[0]) + plane->coeffs[1] * *(float *)((byte *)bounds + plane->side[1]) + plane->coeffs[2] * *(float *)((byte *)bounds + plane->side[2]) + plane->coeffs[3];
}

static inline int R_CullByFrustumPlanes(DpvsPlane *planes, int planeCount, int stackLevel, const float *bounds)
{
    int i;
    for (i = 0; i < planeCount; i++) {
        if ((unsigned char)planes[i].u.frontal > (unsigned)stackLevel)
            continue;
        planes[i].u.frontal = 0xFF;
        if (R_DpvsPlaneBoundsTest(&planes[i], bounds) <= 0.0f)
            return 0;
    }
    return 1;
}

static inline int R_CullByOccluders(int stackLevel, const float *bounds)
{
    int occCount = dpvsG.occluderCount;
    GfxOccluder **occTable = dpvsG.occluderList;
    int i;
    for (i = 0; i < occCount; i++) {
        GfxOccluder *occ = occTable[i];
        if (stackLevel > occ->ignoreStackLevel)
            continue;
        int planeCount = occ->viewPlaneCount;
        DpvsPlane *planes = occ->viewPlanes;
        if (planeCount <= 0)
            return 0;
        int j;
        int allBehind = 1;
        for (j = 0; j < planeCount; j++) {
            if ((unsigned char)planes[j].u.frontal > (unsigned)stackLevel) {
                allBehind = 0;
                break;
            }
            planes[j].u.frontal = 0xFF;
            if (R_DpvsPlaneBoundsTest(&planes[j], bounds) > 0.0f) {
                allBehind = 0;
                break;
            }
        }
        if (allBehind && j >= planeCount)
            return 0;
    }
    return 1;
}

static inline void R_DpvsPlaneSetSideBytes(DpvsPlane *plane)
{
    plane->side[0] = (*(int *)&plane->coeffs[0] > 0) ? 0x0C : 0x00;
    plane->side[1] = (*(int *)&plane->coeffs[1] > 0) ? 0x10 : 0x04;
    plane->side[2] = (*(int *)&plane->coeffs[2] > 0) ? 0x14 : 0x08;
    plane->u.frontal = 0xFF;
}

static void R_DPVS_REGPARM3_ABI R_AddAabbTreeSurfaces_r_impl(GfxAabbTree *tree, DpvsPlane *planes, int planeCount, int stackLevel);

static inline float R_DpvsPlaneFarBoundsTest(const DpvsPlane *plane, const GfxAabbTree *tree)
{
    byte *tp = (byte *)tree;
    return plane->coeffs[0] * *(float *)(tp + 0x0C - plane->side[0]) + plane->coeffs[1] * *(float *)(((char *)tp + offsetof(GfxAabbTree, maxs[2])) - plane->side[1]) + plane->coeffs[2] * *(float *)(tp + 0x1C - plane->side[2]) + plane->coeffs[3];
}
static int R_DPVS_REGPARM3_ABI R_GetFurtherCellList_r_impl(const GfxCell *cell, const DpvsPlane *parentPlane, const DpvsPlane *planes, int planeCount, vec3_t *v, const GfxCell **list, int count);
static void R_DPVS_REGPARM3_ABI R_AddVisibleSurfacesInCell(const GfxCell *cell, const DpvsPlane *planes, int planeCount);
static void R_DPVS_REGPARM3_ABI R_VisitPortalsForCell(const GfxCell *cell, GfxPortal *parentPortal, const DpvsPlane *parentPlane, const DpvsPlane *planes, int planeCount, DpvsClipChildren clipChildren);
static void R_DPVS_REGPARM3_ABI R_VisitPortalsForCell_impl(const GfxCell *cell, GfxPortal *parentPortal, const DpvsPlane *parentPlane, const DpvsPlane *planes, int planeCount, DpvsClipChildren clipChildren);
static void R_VisitPortals(const GfxCell *cell, const DpvsPlane *parentPlane, const DpvsPlane *planes, int planeCount);
static void R_VisitPortals_impl(const GfxCell *cell, const DpvsPlane *parentPlane, const DpvsPlane *planes, int planeCount);
void R_AddWorldSurfacesDpvs(const GfxViewParms *viewParms, int cameraCellIndex);
static void R_AddWorldSurfacesDpvs_impl(const GfxViewParms *viewParms, int cameraCellIndex);

static qboolean R_DpvsShouldFallbackAllCells(int cameraCellIndex)
{
    if (cameraCellIndex < 0)
        return 0;
    if (!(*(const dvar_t **)imp_r_drawWorld)->current.integer)
        return 0;
    return *(int *)&dpvsG.farPlanePtr == 0;
}

void R_DrawModel(int entIndex)
{
    if (!dpvsG.drawXmodels)
        return;

    GfxSceneEntity *sceneEnt = &scene.sceneEnts[entIndex];
    GfxEntity *ent = &scene.def.entities[entIndex];

    R_UpdateXModelBounds(sceneEnt, ent);
    R_SkinSceneEnt(sceneEnt, ent);
    sceneEnt->cullState = 5;
    R_AddXModelSurfaces(entIndex);
}

float R_GetFarPlaneDist(void)
{
    float farPlaneDist = (*(const dvar_t **)imp_r_zfar)->current.value;

    if (farPlaneDist == 0.0f) {
        r_globals_t *rgg = (r_globals_t *)imp_rg;
        if (rgg->fogIndex && rgg->fogSettings[2].registered && rgg->fogSettings[2].techniqueOffset == 1) {
            farPlaneDist = rgg->fogSettings[2].fogEnd;
        }
    }

    float cullDist = *(float *)&dpvsConfig;
    return cullDist > farPlaneDist ? cullDist : farPlaneDist;
}

void R_ClearDpvsScene(void)
{
    r_global_permanent_t *rgpp;

    dpvsScene.modelRefCount = 0;

    rgpp = &rgp;
    if (!rgpp->world)
        return;

    for (int i = 0; i < rgpp->world->cellCount; i++) {
        rgpp->world->cells[i].modelRefs = NULL;
    }
}

void R_SetCullDist(float dist)
{
    *(float *)&dpvsConfig = dist > 0.0f ? dist : 0.0f;
}

static int R_DPVS_REGPARM3_ABI R_FilterEntityIntoCells_r_impl(mnode_t *node, int entIndex, const vec_t *mins, const vec_t *maxs)
{
    int cellIndex = node->cellIndex;

    if (cellIndex == -2) {

        cplane_t *plane = node->u.node.plane;
        int side = BoxOnPlaneSide(mins, maxs, plane);

        if (side == 3) {

            if (plane->type > 2) {

                int cell1 = R_FilterEntityIntoCells_r_impl(node->u.node.children[0], entIndex, mins, maxs);
                int cell2 = R_FilterEntityIntoCells_r_impl(node->u.node.children[1], entIndex, mins, maxs);
                if (cell1 == cell2)
                    return cell1;
                return -2;
            } else {

                vec3_t localmins, localmaxs;
                localmins[0] = mins[0];
                localmins[1] = mins[1];
                localmins[2] = mins[2];
                localmins[plane->type] = plane->dist;
                localmaxs[0] = maxs[0];
                localmaxs[1] = maxs[1];
                localmaxs[2] = maxs[2];
                localmaxs[plane->type] = plane->dist;

                int cell1 = R_FilterEntityIntoCells_r_impl(node->u.node.children[1], entIndex, mins, localmaxs);

                if (maxs[plane->type] > localmins[plane->type]) {
                    int cell2 = R_FilterEntityIntoCells_r_impl(node->u.node.children[0], entIndex, localmins, maxs);
                    if (cell1 != cell2)
                        return -2;
                    return cell2;
                }
                return cell1;
            }
        } else {

            return R_FilterEntityIntoCells_r_impl(node->u.node.children[side - 1], entIndex, mins, maxs);
        }
    }

    if (cellIndex < 0)
        return cellIndex;

    GfxCell *cells = rgp.world->cells;
    GfxCell *cell = &cells[cellIndex];

    if (dpvsScene.modelRefCount >= 4096) {
        R_Error(1, "^1Max xmodel refs (%i) exceeded\n", 4096);
        return node->cellIndex;
    }

    GfxSceneModelCellRef *ref = cell->modelRefs;
    while (ref) {
        if (ref->entIndex == entIndex) {
            ExpandBounds(mins, maxs, ref->mins, ref->maxs);
            return node->cellIndex;
        }
        ref = (GfxSceneModelCellRef *)(intptr_t)ref->next;
    }

    int idx = dpvsScene.modelRefCount;
    GfxSceneModelCellRef *newRef = &dpvsScene.modelRefs[idx];
    dpvsScene.modelRefCount = idx + 1;

    newRef->entIndex = entIndex;
    newRef->mins[0] = mins[0];
    newRef->mins[1] = mins[1];
    newRef->mins[2] = mins[2];
    newRef->maxs[0] = maxs[0];
    newRef->maxs[1] = maxs[1];
    newRef->maxs[2] = maxs[2];
    newRef->next = (int)(intptr_t)cell->modelRefs;
    cell->modelRefs = newRef;

    return node->cellIndex;
}

static int R_DPVS_REGPARM3_ABI R_FilterEntityIntoCells_r(mnode_t *node, int entIndex, const vec_t *mins, const vec_t *maxs)
{
    return R_FilterEntityIntoCells_r_impl(node, entIndex, mins, maxs);
}

int R_CellForPoint(const vec_t *origin)
{
    GfxWorld *world;
    mnode_t *node;
    float *plane;
    float dot;

    world = rgp.world;
    if (!world) {
        R_Error(1, "\x15R_CellForCamera: bad model");
        world = rgp.world;
    }

    node = world->nodes;
    for (;;) {

        if (node->contents != -1)
            return node->cellIndex;

        plane = (float *)node->u.node.plane;
        dot = origin[0] * plane[0] + origin[1] * plane[1] + origin[2] * plane[2] - plane[3];

        if (dot > 0.0f)
            node = node->u.node.children[0];
        else
            node = node->u.node.children[1];
    }
}

static vec3_t *R_DPVS_REGPARM3_ABI R_ChopPortalWinding_impl(vec3_t *vertsIn, int *vertexCount, const float *plane, vec3_t *vertsOut)
{
    int count = *vertexCount;
    if (count <= 0) {
        *vertexCount = 0;
        return NULL;
    }

    float nx = plane[0], ny = plane[1], nz = plane[2], dist = plane[3];
    float distForVert[128];
    byte sideForVert[129];
    int frontCount = 0, backCount = 0;
    int i;

    for (i = 0; i < count; i++) {
        float d = nx * vertsIn[i][0] + ny * vertsIn[i][1] + nz * vertsIn[i][2] + dist - 0.001f;
        distForVert[i] = d;
        sideForVert[i] = 2;
        if (d < -0.001f) {
            sideForVert[i] = 1;
            backCount++;
        } else if (d > 0.001f) {
            sideForVert[i] = 0;
            frontCount++;
        }
    }

    if (frontCount == 0) {
        *vertexCount = 0;
        return NULL;
    }
    if (backCount == 0)
        return vertsIn;

    sideForVert[count] = sideForVert[0];
    distForVert[count] = distForVert[0];

    int newVertCount = 0;
    float *out = (float *)vertsOut;

    for (i = 0;;) {
        byte side = sideForVert[i];

        if (side == 2) {

            out[0] = vertsIn[i][0];
            out[1] = vertsIn[i][1];
            out[2] = vertsIn[i][2];
            newVertCount++;
            out += 3;
        } else if (side == 0) {

            out[0] = vertsIn[i][0];
            out[1] = vertsIn[i][1];
            out[2] = vertsIn[i][2];
            newVertCount++;
            out += 3;
        }

        int next = i + 1;
        byte nextSide = sideForVert[next];

        if (nextSide != 2 && nextSide != side) {
            float d0 = distForVert[i];
            float frac = d0 / (d0 - distForVert[next]);
            int nextVert = next % count;
            float *a = (float *)vertsIn[i];
            float *b = (float *)vertsIn[nextVert];
            out[0] = a[0] + frac * (b[0] - a[0]);
            out[1] = a[1] + frac * (b[1] - a[1]);
            out[2] = a[2] + frac * (b[2] - a[2]);
            newVertCount++;
            out += 3;
        }

        if (next >= count || newVertCount > 127)
            break;
        i = next;
    }

    *vertexCount = newVertCount;
    return vertsOut;
}

static vec3_t *R_DPVS_REGPARM3_ABI R_ChopPortalWinding(vec3_t *vertsIn, int *vertexCount, const float *plane, vec3_t *vertsOut)
{
    return R_ChopPortalWinding_impl(vertsIn, vertexCount, plane, vertsOut);
}

static void R_DPVS_REGPARM3_ABI R_GetSidePlaneNormals_impl(vec3_t *winding, int vertexCount, vec3_t *normals)
{
    float *eyePos = dpvsG.eyePos;
    float eyeDist = dpvsG.eyeW;

    if (eyeDist != 0.0f) {

        vec3_t diff[129];
        int i;

        for (i = 0; i < vertexCount; i++) {
            diff[i][0] = winding[i][0] - eyePos[0];
            diff[i][1] = winding[i][1] - eyePos[1];
            diff[i][2] = winding[i][2] - eyePos[2];
        }

        diff[vertexCount][0] = diff[0][0];
        diff[vertexCount][1] = diff[0][1];
        diff[vertexCount][2] = diff[0][2];

        for (i = 0; i < vertexCount; i++) {
            Vec3Cross(diff[i + 1], diff[i], normals[i]);
            Vec3Normalize(normals[i]);
        }
    } else {

        int prevIdx = vertexCount - 1;
        int i;

        if (vertexCount <= 0)
            return;

        for (i = 0; i < vertexCount; i++) {
            vec3_t delta;
            delta[0] = winding[i][0] - winding[prevIdx][0];
            delta[1] = winding[i][1] - winding[prevIdx][1];
            delta[2] = winding[i][2] - winding[prevIdx][2];

            Vec3Cross(eyePos, delta, normals[prevIdx]);
            Vec3Normalize(normals[prevIdx]);

            prevIdx = i;
        }
    }
}

static void R_DPVS_REGPARM3_ABI R_GetSidePlaneNormals(vec3_t *winding, int vertexCount, vec3_t *normals)
{
    R_GetSidePlaneNormals_impl(winding, vertexCount, normals);
}

static void R_DPVS_REGPARM3_ABI R_AddStaticModelWithCull_impl(int smodelIndex, const DpvsPlane *planes, int planeCount, int stackLevel)
{
    int *smodelDync = (int *)((byte *)rg.smodelDyncs + smodelIndex * 8);
    int viewCount = scene.viewCount;

    if (smodelDync[0] == viewCount)
        return;

    GfxStaticModelInstance *smodelInst = &rgp.world->smodelInsts[smodelIndex];

    if (smodelInst->cullDist != 0.0f) {
        float dist = Vec3Distance(smodelInst->origin, rg.lodParms.origin);
        float scaledDist = dist * rg.lodParms.scale + rg.lodParms.bias;
        if (scaledDist > smodelInst->cullDist) {
            smodelDync[0] = viewCount;
            return;
        }
    }

    const float *bounds = (const float *)&smodelInst->mins;
    if (!R_CullByFrustumPlanes((DpvsPlane *)planes, planeCount, stackLevel, bounds))
        return;

    if (!R_CullByOccluders(stackLevel, bounds))
        return;

    smodelDync[0] = viewCount;

    if ((*(const dvar_t **)imp_r_showSModelNames)->current.enabled) {
        const char *name = XModelGetName(smodelInst->model);
        DebugGlobals *debugGlobals = &frontEndDataOut->debugGlobals;
        R_AddDebugString(debugGlobals, smodelInst->origin, (const void *)imp_colorWhite, 0.3f, name);
    }

    int entIndex = R_AddStaticModelToScene(smodelIndex);
    if (entIndex < 0)
        return;

    GfxSceneEntity *sceneEnt = &scene.sceneEnts[entIndex];
    GfxEntity *ent = &scene.def.entities[entIndex];

    R_SkinStaticModel(sceneEnt, ent, smodelIndex);
    sceneEnt->cullState = 5;

    R_AddXModelSurfaces(entIndex);
}

static void R_DPVS_REGPARM3_ABI R_AddStaticModelWithCull(int smodelIndex, const DpvsPlane *planes, int planeCount, int stackLevel)
{
    R_AddStaticModelWithCull_impl(smodelIndex, planes, planeCount, stackLevel);
}

void R_FrustumClipPlanes(const D3DMATRIX *viewProjMtx, vec4_t *sidePlanes, int sidePlaneCount, DpvsPlane *frustumPlanes)
{
    int planeIndex, term;
    const float *mtx = (const float *)viewProjMtx;
    for (planeIndex = 0; planeIndex < sidePlaneCount; planeIndex++) {
        const float *side = (const float *)&sidePlanes[planeIndex];
        float *fp = frustumPlanes[planeIndex].coeffs;

        for (term = 0; term < 4; term++)
            fp[term] = side[0] * mtx[term * 4 + 0] + side[1] * mtx[term * 4 + 1] + side[2] * mtx[term * 4 + 2] + side[3] * mtx[term * 4 + 3];

        float invLen = 1.0f / __builtin_sqrtf(fp[0] * fp[0] + fp[1] * fp[1] + fp[2] * fp[2]);
        fp[0] *= invLen;
        fp[1] *= invLen;
        fp[2] *= invLen;
        fp[3] *= invLen;

        frustumPlanes[planeIndex].side[0] = (fp[0] > 0.0f) ? 0x0C : 0x00;
        frustumPlanes[planeIndex].side[1] = (fp[1] > 0.0f) ? 0x10 : 0x04;
        frustumPlanes[planeIndex].side[2] = (fp[2] > 0.0f) ? 0x14 : 0x08;
        frustumPlanes[planeIndex].u.frontal = 0xFF;
    }
}

static void R_DPVS_REGPARM3_ABI R_AddWorldSurfaceWithCull_impl(int surfIndex, const DpvsPlane *planes, int planeCount, int stackLevel)
{

    int *surfVisData = (int *)rg.surfaces;
    int viewCount = scene.viewCount;

    if (!surfVisData)
        return;

    if (surfVisData[surfIndex] == viewCount)
        return;

    GfxSurface *surf = &rgp.world->surfaces[surfIndex];
    srfTriangles_t *tris = surf->tris;

    if (!tris) {
        return;
    }

    if (planeCount > 0 && (*(const dvar_t **)imp_r_portalFineCull)->current.enabled) {
        const float *bounds = (const float *)tris->bounds;
        if (!R_CullByFrustumPlanes((DpvsPlane *)planes, planeCount, stackLevel, bounds))
            return;
        if (!R_CullByOccluders(stackLevel, bounds))
            return;
    }

    surfVisData[surfIndex] = viewCount;

    int entIndex = surf->sortGroup + 0x800;

    R_AddDrawSurfForSurface(surf, entIndex);
}

static void R_DPVS_REGPARM3_ABI R_AddWorldSurfaceWithCull(int surfIndex, const DpvsPlane *planes, int planeCount, int stackLevel)
{
    R_AddWorldSurfaceWithCull_impl(surfIndex, planes, planeCount, stackLevel);
}

static void R_AddStaticModelDirect(int smodelIndex)
{
    int *smodelDync = (int *)((byte *)rg.smodelDyncs + smodelIndex * 8);
    int viewCount = scene.viewCount;

    if (smodelDync[0] == viewCount)
        return;
    smodelDync[0] = viewCount;

    GfxStaticModelInstance *inst = &rgp.world->smodelInsts[smodelIndex];

    if (inst->cullDist != 0.0f) {
        float dist = Vec3Distance(inst->origin, rg.lodParms.origin);
        float scaledDist = dist * rg.lodParms.scale + rg.lodParms.bias;
        if (scaledDist > inst->cullDist)
            return;
    }

    if ((*(const dvar_t **)imp_r_showSModelNames)->current.enabled) {
        const char *name = XModelGetName(inst->model);
        DebugGlobals *debugGlobals = &frontEndDataOut->debugGlobals;
        R_AddDebugString(debugGlobals, inst->origin, (const void *)imp_colorWhite, 0.3f, name);
    }

    int entIndex = R_AddStaticModelToScene(smodelIndex);
    if (entIndex < 0)
        return;

    GfxSceneEntity *sceneEnt = &scene.sceneEnts[entIndex];
    GfxEntity *ent = &scene.def.entities[entIndex];
    R_SkinStaticModel(sceneEnt, ent, smodelIndex);
    sceneEnt->cullState = 5;
    R_AddXModelSurfaces(entIndex);
}

static void R_AddWorldSurfaceDirect(int surfIndex)
{
    int *surfVisData = (int *)rg.surfaces;
    int viewCount = scene.viewCount;
    if (surfVisData[surfIndex] == viewCount)
        return;
    surfVisData[surfIndex] = viewCount;

    GfxSurface *surf = &rgp.world->surfaces[surfIndex];
    R_AddDrawSurfForSurface(surf, surf->sortGroup + 0x800);
}

static void R_DPVS_REGPARM3_ABI R_AddAabbTreeSurfaces_r_impl(GfxAabbTree *tree, DpvsPlane *planes, int planeCount, int stackLevel)
{
    float *bounds = (float *)tree;
    int i;

    if (!R_CullByFrustumPlanes(planes, planeCount, stackLevel, bounds))
        return;

    int occCount = dpvsG.occluderCount;
    GfxOccluder **occTable = dpvsG.occluderList;
    for (i = 0; i < occCount; i++) {
        GfxOccluder *occ = occTable[i];
        if (stackLevel > occ->ignoreStackLevel)
            continue;
        int occPlaneCount = occ->viewPlaneCount;
        DpvsPlane *occPlanes = occ->viewPlanes;
        if (occPlaneCount <= 0)
            return;
        int occluded = 1;
        int j;
        for (j = 0; j < occPlaneCount; j++) {
            DpvsPlane *op = &occPlanes[j];
            if ((unsigned char)op->u.frontal > (unsigned)stackLevel) {
                op->u.frontal = stackLevel;
                continue;
            }
            op->u.frontal = 0xFF;
            if (R_DpvsPlaneBoundsTest(op, bounds) > 0.0f)
                occluded = 0;
            else
                op->u.frontal = stackLevel;
        }
        if (occluded)
            return;
    }

    int recursionNeeded = 0;
    if (planeCount > 0) {
        for (i = 0; i < planeCount; i++) {
            DpvsPlane *p = &planes[i];
            if ((unsigned char)p->u.frontal > (unsigned)stackLevel)
                continue;
            if (R_DpvsPlaneFarBoundsTest(p, tree) >= 0.0f)
                p->u.frontal = stackLevel;
            else
                recursionNeeded = 1;
        }
    }

    if (!recursionNeeded) {
        for (i = 0; i < occCount; i++) {
            if (stackLevel <= occTable[i]->ignoreStackLevel) {
                recursionNeeded = 1;
                break;
            }
        }
    }

    int childStackLevel = stackLevel + 1;

    if (recursionNeeded) {
        if (tree->childCount > 0) {

            for (i = 0; i < tree->childCount; i++) {
                GfxAabbTree *child = (GfxAabbTree *)((byte *)(intptr_t)tree->children + i * sizeof(GfxAabbTree));
                R_AddAabbTreeSurfaces_r_impl(child, planes, planeCount, childStackLevel);
            }
        } else {

            byte smodelFlag = dpvsG.drawSmodels;

            if (smodelFlag && tree->staticModelCount > 0) {
                for (i = 0; i < tree->staticModelCount; i++)
                    R_AddStaticModelWithCull_impl(tree->staticModels[i], (const DpvsPlane *)planes, planeCount, childStackLevel);
            }

            for (i = 0; i < tree->surfaceCount; i++)
                R_AddWorldSurfaceWithCull_impl(tree->startSurfIndex + i, (const DpvsPlane *)planes, planeCount, childStackLevel);
        }
    } else {

        byte smodelFlag = dpvsG.drawSmodels;

        if (smodelFlag && tree->staticModelCount > 0) {
            for (i = 0; i < tree->staticModelCount; i++)
                R_AddStaticModelDirect(tree->staticModels[i]);
        }

        for (i = 0; i < tree->surfaceCount; i++)
            R_AddWorldSurfaceDirect(tree->startSurfIndex + i);
    }
}

static void R_DPVS_REGPARM3_ABI R_AddAabbTreeSurfaces_r(GfxAabbTree *tree, DpvsPlane *planes, int planeCount, int stackLevel)
{
    R_AddAabbTreeSurfaces_r_impl(tree, planes, planeCount, stackLevel);
}

static int R_DPVS_REGPARM3_ABI R_GetFurtherCellList_r_impl(const GfxCell *cell, const DpvsPlane *parentPlane, const DpvsPlane *planes, int planeCount, vec3_t *v, const GfxCell **list, int count)
{
    float *eyeDir = dpvsG.eyePos;
    int portalCount = cell->portalCount;
    int portalIndex;

    for (portalIndex = 0; portalIndex < portalCount; portalIndex++) {
        GfxPortal *portal = &cell->portals[portalIndex];
        const GfxCell *destCell = portal->cell;

        int already = 0;
        int k;
        for (k = 0; k < count; k++) {
            if (list[k] == destCell) {
                already = 1;
                break;
            }
        }
        if (already)
            continue;

        if (portal->writable.isAncestor)
            continue;

        float *portalPlane = portal->plane.coeffs;
        float dot = portalPlane[0] * eyeDir[0] + portalPlane[1] * eyeDir[1] + portalPlane[2] * eyeDir[2] + portalPlane[3] * eyeDir[3];
        if (dot > 0.0f)
            continue;

        vec3_t *portalVerts = portal->vertices;
        int vertCount = portal->vertexCount;

        vec3_t *w;
        if (parentPlane) {
            w = R_ChopPortalWinding_impl(portalVerts, &vertCount, (const float *)parentPlane, v);
            if (!vertCount)
                continue;

            if (*(int *)&dpvsG.farPlanePtr) {
                vec3_t *altBuf = (w != v) ? v : v + 128;
                w = R_ChopPortalWinding_impl(w, &vertCount, (const float *)parentPlane, altBuf);
                if (!vertCount)
                    continue;
            }
        } else {
            w = portalVerts;
        }

        int pi;
        for (pi = 0; pi < planeCount; pi++) {
            vec3_t *altBuf = (w != v) ? v : v + 128;
            w = R_ChopPortalWinding_impl(w, &vertCount, (const float *)&planes[pi], altBuf);
            if (!vertCount)
                break;
        }
        if (!vertCount)
            continue;

        int occCount = dpvsG.occluderCount;
        GfxOccluder **occTable = dpvsG.occluderList;
        int occluded = 0;
        int oi;
        for (oi = 0; oi < occCount && !occluded; oi++) {
            GfxOccluder *occ = occTable[oi];
            int occPlaneCount = occ->viewPlaneCount;
            DpvsPlane *occPlanes = occ->viewPlanes;
            if (occPlaneCount <= 0) {
                occluded = 1;
                break;
            }

            int allBehind = 1;
            int opi;
            for (opi = 0; opi < occPlaneCount && allBehind; opi++) {
                float *op = occPlanes[opi].coeffs;

                float *vert = (float *)portalVerts;
                float d = op[0] * vert[0] + op[1] * vert[1] + op[2] * vert[2] + op[3];
                if (d > 0.0f)
                    allBehind = 0;

                int vi;
                for (vi = 1; vi < (int)portal->vertexCount && allBehind; vi++) {
                    vert = (float *)((byte *)portalVerts + vi * 12);
                    d = op[0] * vert[0] + op[1] * vert[1] + op[2] * vert[2] + op[3];
                    if (d > 0.0f)
                        allBehind = 0;
                }
            }
            if (allBehind)
                occluded = 1;
        }
        if (occluded)
            continue;

        if (vertCount > 0) {
            list[count] = destCell;
            count = R_GetFurtherCellList_r_impl(destCell, parentPlane, planes, planeCount, v, list, count + 1);
        }
    }

    return count;
}

static int R_DPVS_REGPARM3_ABI R_GetFurtherCellList_r(const GfxCell *cell, const DpvsPlane *parentPlane, const DpvsPlane *planes, int planeCount, vec3_t *v, const GfxCell **list, int count)
{
    return R_GetFurtherCellList_r_impl(cell, parentPlane, planes, planeCount, v, list, count);
}

extern void CG_CullIn(void *poseCtx);
extern void CG_UsedDObjCalcPose(void *poseCtx);
extern void R_AddBModelSurfaces(void *sceneEnt, int entIndex);
extern void R_SkinSceneEnt(void *sceneEnt, void *ent);
extern void R_AddDebugBox(void *debugGlobals, const float *mins, const float *maxs, const float *color);
extern void R_AddDebugLine(void *debugGlobals, const vec_t *start, const vec_t *end, const float *color);
extern void R_AddDebugPolygon(void *debugGlobals, const float *color, int vertCount, const vec_t *verts);
extern int Com_ConvexHull(const vec2_t *points, int maxPoints, vec2_t *hull);
extern int WindingContainsCoplanarPoint(const vec3_t *verts, int vertCount, const vec_t *planeNormal, const vec_t *point);
extern int R_CullPointAndRadius(const vec_t *origin, float radius, const DpvsPlane *planes, int planeCount);
extern void RB_ShowLightVisCachePoints(const GfxViewParms *viewParms, const DpvsPlane *planes, int planeCount);
extern void *R_GetGfxEntityDObj(void *sceneEnt, void *ent);
extern void ClearBounds(vec_t *mins, vec_t *maxs);
extern void AddPointToBounds(const vec_t *v, vec_t *mins, vec_t *maxs);
extern void R_dpvs_diag_print(int cameraCellIndex, int drawWorld, const void *cellPtr);
extern void LargeLocal_LargeLocal(const LargeLocal *_this, int size);
extern void *LargeLocal_GetBuf(const LargeLocal *_this);
extern void ZN10LargeLocalD1Ev(LargeLocal *_this);

static void R_DPVS_REGPARM3_ABI R_AddVisibleSurfacesInCell_impl(const GfxCell *cell, const DpvsPlane *planes, int planeCount)
{
    int i;

    if (dpvsG.drawWorld) {
        GfxAabbTree *tree = cell->aabbTree;
        if (tree && tree->childCount > 0) {

            if ((*(const dvar_t **)imp_r_portalFineCull)->current.enabled) {
                for (i = 0; i < tree->childCount; i++) {
                    GfxAabbTree *child = (GfxAabbTree *)((byte *)(intptr_t)tree->children + i * sizeof(GfxAabbTree));
                    R_AddAabbTreeSurfaces_r_impl(child, (DpvsPlane *)planes, planeCount, 0);
                }
            } else {

                int startSurf = tree->startSurfIndex;
                for (i = 0; i < tree->surfaceCount; i++)
                    R_AddWorldSurfaceWithCull_impl(startSurf + i, planes, planeCount, 0);
            }
        } else if (tree) {

            int startSurf = tree->startSurfIndex;
            for (i = 0; i < tree->surfaceCount; i++)
                R_AddWorldSurfaceWithCull_impl(startSurf + i, planes, planeCount, 0);
        }
    } else {

    }

    if (dpvsG.drawEntities) {
        GfxSceneModelCellRef *modelRef = cell->modelRefs;
        while (modelRef) {
            int entIndex = modelRef->entIndex;
            GfxSceneEntity *sceneEnt2 = &scene.sceneEnts[entIndex];
            int surfaceType = sceneEnt2->cullState;

            if (surfaceType == 5) {

                modelRef = (GfxSceneModelCellRef *)(intptr_t)modelRef->next;
                continue;
            }

            GfxEntity *ent = &scene.def.entities[entIndex];
            int entType = ent->reType;

            if (entType <= 1) {

                if (dpvsG.drawXmodels) {

                    int lodLevel = *(int *)&ent->lighting.dx7.sunVisibility;
                    if (lodLevel > 1) {

                        float *bounds = (float *)modelRef->mins;
                        if (!R_CullByFrustumPlanes((DpvsPlane *)planes, planeCount, 0, bounds))
                            goto next_modelref;
                        if (!R_CullByOccluders(0, bounds))
                            goto next_modelref;
                    } else {

                        float *bounds = (float *)sceneEnt2->curMins;
                        if (!R_CullByFrustumPlanes((DpvsPlane *)planes, planeCount, 0, bounds))
                            goto next_modelref;
                        if (!R_CullByOccluders(0, bounds))
                            goto next_modelref;
                    }
                    CG_UsedDObjCalcPose((void *)sceneEnt2->cent);
                    R_UpdateXModelBounds(sceneEnt2, ent);
                }

                CG_CullIn((void *)sceneEnt2->cent);
                R_SkinSceneEnt(sceneEnt2, ent);
                sceneEnt2->cullState = 5;
                R_AddXModelSurfaces(entIndex);

            } else if (entType == 3) {

                if (dpvsG.drawBmodels) {
                    float *bounds = (float *)(modelRef->mins);
                    if (!R_CullByFrustumPlanes((DpvsPlane *)planes, planeCount, 0, bounds))
                        goto next_modelref;
                    if (!R_CullByOccluders(0, bounds))
                        goto next_modelref;

                    if ((*(const dvar_t **)imp_r_showPortals)->current.integer & 1) {
                        DebugGlobals *debugGlobals = &frontEndDataOut->debugGlobals;
                        R_AddDebugBox(debugGlobals, (float *)modelRef->mins, (float *)modelRef->maxs, (const float *)imp_colorLtYellow);
                    }
                }
                R_AddBModelSurfaces(sceneEnt2, entIndex);
            }

        next_modelref:
            modelRef = (GfxSceneModelCellRef *)(intptr_t)modelRef->next;
        }
    }

    if (dpvsG.drawWorld) {
        int cullGroupCount = cell->cullGroupCount;
        int *cullGroups = cell->cullGroups;
        if (cullGroupCount > 0) {
            for (i = 0; i < cullGroupCount; i++) {
                int groupIdx = cullGroups[i];

                int *groupVisData = (int *)rg.cullGroups;
                int viewCount = scene.viewCount;
                if (groupVisData[groupIdx] == viewCount)
                    continue;

                GfxCullGroup *cg = &rgp.world->cullGroups[groupIdx];
                float *bounds = cg->mins;

                if (!R_CullByFrustumPlanes((DpvsPlane *)planes, planeCount, 0, bounds))
                    continue;
                if (!R_CullByOccluders(0, bounds))
                    continue;

                if ((*(const dvar_t **)imp_r_showPortals)->current.integer & 1) {
                    DebugGlobals *debugGlobals = &frontEndDataOut->debugGlobals;
                    R_AddDebugBox(debugGlobals, cg->mins, cg->maxs, (const float *)imp_colorLtYellow);
                }

                groupVisData[groupIdx] = viewCount;
                int surfStart = cg->startSurfIndex;
                int surfCount = cg->surfaceCount;
                int j;
                for (j = 0; j < surfCount; j++)
                    R_AddWorldSurfaceDirect(surfStart + j);
            }
        }
    }
}

static void R_DPVS_REGPARM3_ABI R_AddVisibleSurfacesInCell(const GfxCell *cell, const DpvsPlane *planes, int planeCount)
{
    R_AddVisibleSurfacesInCell_impl(cell, planes, planeCount);
}

static inline int R_OccluderVertexInFront(const GfxOccluder *occ, const float *planeCoeffs)
{
    int vertCount = occ->vertexCount;
    vec3_t *verts = occ->vertices;
    for (int i = 0; i < vertCount; i++) {
        float dot = planeCoeffs[0] * verts[i][0] + planeCoeffs[1] * verts[i][1] + planeCoeffs[2] * verts[i][2] + planeCoeffs[3];
        if (dot > 0.0f)
            return 1;
    }
    return 0;
}

static inline void R_ComputeOccluderEdgePlane(const vec3_t *v0, const vec3_t *v1, DpvsPlane *outPlane)
{
    float *eye = dpvsG.eyePos;
    float w = dpvsG.eyeW;
    vec3_t cross_result;

    if (w == 0.0f) {

        vec3_t delta;
        delta[0] = (*v1)[0] - (*v0)[0];
        delta[1] = (*v1)[1] - (*v0)[1];
        delta[2] = (*v1)[2] - (*v0)[2];
        Vec3Cross((vec_t *)eye, (vec_t *)delta, (vec_t *)cross_result);
    } else {

        vec3_t d0, d1;
        d0[0] = (*v0)[0] - eye[0];
        d0[1] = (*v0)[1] - eye[1];
        d0[2] = (*v0)[2] - eye[2];
        d1[0] = (*v1)[0] - eye[0];
        d1[1] = (*v1)[1] - eye[1];
        d1[2] = (*v1)[2] - eye[2];
        Vec3Cross((vec_t *)d1, (vec_t *)d0, (vec_t *)cross_result);
    }

    outPlane->coeffs[0] = cross_result[0];
    outPlane->coeffs[1] = cross_result[1];
    outPlane->coeffs[2] = cross_result[2];
    Vec3Normalize(outPlane->coeffs);
    float dot = outPlane->coeffs[0] * (*v0)[0] + outPlane->coeffs[1] * (*v0)[1] + outPlane->coeffs[2] * (*v0)[2];
    outPlane->coeffs[3] = -dot;
    R_DpvsPlaneSetSideBytes(outPlane);
}

static inline void R_PortalQueueInsert(GfxPortal *portal, float priority)
{
    int count = dpvsG.portalQueueCount;
    byte *queueBase = (byte *)dpvsG.portalQueue;

    if (count > 0xFF) {
        R_Error(1, "More than %i queued portals", 0x100);
    }

    portal->writable.isQueued = 1;

    int idx = count;
    int parent = (idx - 1) >> 1;
    while (parent >= 0) {
        float parentPrio = *(float *)(queueBase + parent * 8 + 4);
        if (priority >= parentPrio)
            break;

        *(int *)(queueBase + idx * 8) = *(int *)(queueBase + parent * 8);
        *(float *)(queueBase + idx * 8 + 4) = parentPrio;
        idx = parent;
        parent = (idx - 1) >> 1;
        if (idx <= 0)
            break;
    }

    *(GfxPortal **)(queueBase + idx * 8) = portal;
    *(float *)(queueBase + idx * 8 + 4) = priority;
    dpvsG.portalQueueCount = count + 1;
}

static inline vec2_t *R_AllocHullPointsBuf(void)
{
    void **freeList = (void **)&dpvsG.portalPool;
    void *buf = *freeList;
    if (!buf) {
        R_Error(1, "more than %i queued portals", 0x100);
        buf = *freeList;
    }
    *freeList = *(void **)buf;
    return (vec2_t *)buf;
}

static inline void R_FreeHullPointsBuf(vec2_t *buf)
{
    void **freeList = (void **)&dpvsG.portalPool;
    *(void **)buf = *freeList;
    *freeList = buf;
}

static inline float R_PortalMinDot(const vec3_t *verts, int vertCount, const float *planeCoeffs)
{
    float first = planeCoeffs[0] * verts[0][0] + planeCoeffs[1] * verts[0][1] + planeCoeffs[2] * verts[0][2] + planeCoeffs[3];
    float last = planeCoeffs[0] * verts[vertCount - 1][0] + planeCoeffs[1] * verts[vertCount - 1][1] + planeCoeffs[2] * verts[vertCount - 1][2] + planeCoeffs[3];

    float minDot;
    if (first > last) {
        minDot = last;

        int limit = vertCount - 2;
        if (limit > 0) {
            float d = planeCoeffs[0] * verts[1][0] + planeCoeffs[1] * verts[1][1] + planeCoeffs[2] * verts[1][2] + planeCoeffs[3];
            if (d > minDot) {

                for (int i = 1; i < limit; i++) {
                    d = planeCoeffs[0] * verts[i + 1][0] + planeCoeffs[1] * verts[i + 1][1] + planeCoeffs[2] * verts[i + 1][2] + planeCoeffs[3];
                    if (d <= minDot)
                        break;
                }
            }
            if (d < minDot)
                minDot = d;
        }
    } else {
        minDot = first;

        int limit = vertCount - 2;
        if (limit > 0) {
            int lastIdx = vertCount - 2;
            float d = planeCoeffs[0] * verts[lastIdx][0] + planeCoeffs[1] * verts[lastIdx][1] + planeCoeffs[2] * verts[lastIdx][2] + planeCoeffs[3];
            if (d <= minDot) {
                minDot = d;
            }
            for (int i = 0; i < limit - 1; i++) {
                int vi = lastIdx - 1 - i;
                if (vi < 0)
                    break;
                d = planeCoeffs[0] * verts[vi][0] + planeCoeffs[1] * verts[vi][1] + planeCoeffs[2] * verts[vi][2] + planeCoeffs[3];
                if (d > minDot)
                    break;
                minDot = d;
            }
        }
    }
    return minDot;
}

static void R_DPVS_REGPARM3_ABI R_VisitPortalsForCell_impl(const GfxCell *cell, GfxPortal *parentPortal, const DpvsPlane *parentPlane, const DpvsPlane *planes, int planeCount, DpvsClipChildren clipChildren)
{
    const GfxCell **cellList;
    vec3_t scratchBuf[128];
    vec3_t altBuf[128];
    int i;

    dpvsG.occluderCount = 0;
    dpvsG.viewPlaneCount = 0;

    int occluderCount = cell->occluderCount;
    for (i = 0; i < occluderCount; i++) {
        GfxOccluder *occ = cell->occluders[i];

        DpvsPlane *eyePlane = dpvsG.clipPlanes;
        if (eyePlane) {

            if (!R_OccluderVertexInFront(occ, eyePlane->coeffs))
                continue;
        }

        int visibleThroughFrustum = 1;
        for (int pi = 0; pi < planeCount; pi++) {
            if (!R_OccluderVertexInFront(occ, planes[pi].coeffs)) {
                visibleThroughFrustum = 0;
                break;
            }
        }
        if (!visibleThroughFrustum)
            continue;

        if (dpvsG.occluderCount == 0x400) {
            R_Error(1, "More than %i active occluders", 0x400);
        }

        int occIdx = dpvsG.occluderCount;
        GfxOccluder **occTable = dpvsG.occluderList;
        occTable[occIdx] = occ;
        dpvsG.occluderCount = occIdx + 1;

        occ->ignoreStackLevel = 0x7FFFFFFF;

        int planeBase = dpvsG.viewPlaneCount;
        occ->viewPlaneCount = planeBase;
        byte *planeBuf = (byte *)dpvsG.viewPlanes;
        occ->viewPlanes = (DpvsPlane *)(planeBuf + planeBase * sizeof(DpvsPlane));

        int basePlaneCount = occ->planeCount;
        for (int pi = 0; pi < basePlaneCount; pi++) {
            DpvsPlane *srcPlane = &occ->planes[pi];
            srcPlane->u.frontal = 0;

            float *eye = dpvsG.eyePos;
            float dot = srcPlane->coeffs[0] * eye[0] + srcPlane->coeffs[1] * eye[1] + srcPlane->coeffs[2] * eye[2] + srcPlane->coeffs[3] * dpvsG.eyeW;
            if (dot <= 0.0f)
                continue;

            srcPlane->u.frontal = 1;

            if (dpvsG.viewPlaneCount == 0x1800) {
                R_Error(1, "More than %i occluder planes", 0x1800);
            }

            int destIdx = dpvsG.viewPlaneCount;
            DpvsPlane *destPlane = (DpvsPlane *)((byte *)dpvsG.viewPlanes + destIdx * sizeof(DpvsPlane));
            *destPlane = *srcPlane;
            dpvsG.viewPlaneCount = destIdx + 1;
        }

        int edgeCount = occ->edgeCount;
        for (int ei = 0; ei < edgeCount; ei++) {
            GfxOccluderEdge *edge = &occ->edges[ei];
            DpvsPlane *plane0 = edge->plane[0];
            DpvsPlane *plane1 = edge->plane[1];

            if (plane0->u.frontal == plane1->u.frontal)
                continue;

            if ((*(const dvar_t **)imp_r_showPortals)->current.integer) {
                DebugGlobals *debugGlobals = &frontEndDataOut->debugGlobals;
                R_AddDebugLine(debugGlobals, (vec_t *)edge->vertex[0], (vec_t *)edge->vertex[1], (const float *)imp_colorMagenta);
            }

            byte frontal0 = plane0->u.frontal;
            vec3_t *baseVert, *otherVert;
            if (frontal0 == 1) {
                baseVert = edge->vertex[1];
                otherVert = edge->vertex[0];
            } else {
                baseVert = edge->vertex[0];
                otherVert = edge->vertex[1];
            }

            int destIdx = dpvsG.viewPlaneCount;
            DpvsPlane *edgePlane = (DpvsPlane *)((byte *)dpvsG.viewPlanes + destIdx * sizeof(DpvsPlane));
            R_ComputeOccluderEdgePlane(otherVert, baseVert, edgePlane);
            dpvsG.viewPlaneCount = destIdx + 1;
        }

        int totalPlanes = dpvsG.viewPlaneCount;
        occ->viewPlaneCount = totalPlanes - occ->viewPlaneCount;
    }

    R_AddVisibleSurfacesInCell_impl(cell, planes, planeCount);

    {
        GfxPortal *ancestor = parentPortal;
        while (ancestor) {
            ancestor->writable.isAncestor = 1;
            ancestor = ancestor->writable.queuedParent;
        }
    }

    if (clipChildren) {

        int portalCount = cell->portalCount;
        GfxPortal *portals = cell->portals;
        for (i = 0; i < portalCount; i++) {
            GfxPortal *portal = &portals[i];

            if (portal->writable.isAncestor)
                continue;

            float *portalPlaneCoeffs = portal->plane.coeffs;
            float *eye = dpvsG.eyePos;
            float eyeDot = portalPlaneCoeffs[0] * eye[0] + portalPlaneCoeffs[1] * eye[1] + portalPlaneCoeffs[2] * eye[2] + portalPlaneCoeffs[3] * dpvsG.eyeW;
            if (eyeDot > 0.0f)
                continue;

            {

                int vertCount = portal->vertexCount;
                vec3_t *verts = portal->vertices;

                int frustumVisible = 1;
                for (int pi = 0; pi < planeCount; pi++) {
                    const float *pc = planes[pi].coeffs;
                    int anyInFront = 0;
                    for (int vi = 0; vi < vertCount; vi++) {
                        float d = pc[0] * verts[vi][0] + pc[1] * verts[vi][1] + pc[2] * verts[vi][2] + pc[3];
                        if (d > 0.0f) {
                            anyInFront = 1;
                            break;
                        }
                    }
                    if (!anyInFront) {
                        frustumVisible = 0;
                        break;
                    }
                }
                if (!frustumVisible)
                    continue;

                int occCount = dpvsG.occluderCount;
                int visible = 1;
                GfxOccluder **occPtrTable = dpvsG.occluderList;
                for (int oi = 0; oi < occCount; oi++) {
                    GfxOccluder *occPtr = occPtrTable[oi];
                    int vpCount = occPtr->viewPlaneCount;
                    DpvsPlane *vpPlanes = occPtr->viewPlanes;

                    if (vpCount == 0) {
                        visible = 0;
                        break;
                    }

                    int allBehind = 1;
                    for (int pi = 0; pi < vpCount; pi++) {
                        int anyInFront = 0;
                        for (int vi = 0; vi < vertCount; vi++) {
                            float d = vpPlanes[pi].coeffs[0] * verts[vi][0] + vpPlanes[pi].coeffs[1] * verts[vi][1] + vpPlanes[pi].coeffs[2] * verts[vi][2] + vpPlanes[pi].coeffs[3];
                            if (d > 0.0f) {
                                anyInFront = 1;
                                break;
                            }
                        }
                        if (anyInFront) {
                            allBehind = 0;
                            break;
                        }
                    }
                    if (allBehind) {
                        visible = 0;
                        break;
                    }
                }
                if (!visible)
                    continue;

                float w = dpvsG.eyeW;
                if (w != 0.0f) {

                    float *portalP = portal->plane.coeffs;
                    float dot4 = portalP[0] * eye[0] + portalP[1] * eye[1] + portalP[2] * eye[2] + portalP[3] * w;
                    if (dot4 > -0.125f) {

                        if (WindingContainsCoplanarPoint(portal->vertices, portal->vertexCount, &portal->plane.coeffs[0], eye)) {
                            portal->writable.queuedParent = 0;
                            GfxCell *destCell = portal->cell;
                            R_VisitPortalsForCell(destCell, portal, &portal->plane, planes, planeCount, clipChildren);
                        }
                        continue;
                    }
                }

                int vertCount2 = portal->vertexCount;
                vec3_t *w2 = R_ChopPortalWinding_impl(portal->vertices, &vertCount2, (const float *)parentPlane, scratchBuf);
                if (vertCount2 == 0)
                    continue;

                DpvsPlane *farPlane = dpvsG.farPlanePtr;
                if (farPlane) {
                    vec3_t *altDest = (w2 == scratchBuf) ? altBuf : scratchBuf;
                    if (w2 == altDest)
                        altDest = altBuf;
                    w2 = R_ChopPortalWinding_impl(w2, &vertCount2, (const float *)farPlane, altDest);
                    if (vertCount2 == 0)
                        continue;
                }

                for (int pi = 0; pi < planeCount; pi++) {
                    vec3_t *altDest = (w2 == scratchBuf) ? altBuf : scratchBuf;
                    w2 = R_ChopPortalWinding_impl(w2, &vertCount2, (const float *)&planes[pi], altDest);
                    if (vertCount2 == 0)
                        break;
                }
                if (vertCount2 == 0)
                    continue;

                if (!portal->hullPoints) {
                    portal->hullPoints = R_AllocHullPointsBuf();
                    portal->hullPointCount = 0;
                }

                for (int vi = 0; vi < vertCount2; vi++) {
                    if (portal->hullPointCount >= 64) {

                        vec2_t hullResult[64];
                        int hullCount = Com_ConvexHull(portal->hullPoints, 64, hullResult);
                        if (hullCount >= 64) {
                            R_Error(1, "More than %i points on a clipped portal's convex hull", 64);
                        }
                        portal->hullPointCount = hullCount;
                        memcpy(portal->hullPoints, hullResult, hullCount * sizeof(vec2_t));
                    }

                    float hx = w2[vi][0] * portal->hullAxis[0][0] + w2[vi][1] * portal->hullAxis[0][1] + w2[vi][2] * portal->hullAxis[0][2];
                    float hy = w2[vi][0] * portal->hullAxis[1][0] + w2[vi][1] * portal->hullAxis[1][1] + w2[vi][2] * portal->hullAxis[1][2];
                    portal->hullPoints[portal->hullPointCount][0] = hx;
                    portal->hullPoints[portal->hullPointCount][1] = hy;
                    portal->hullPointCount++;
                }

                if (portal->writable.isQueued) {

                    if (portal->writable.queuedParent != parentPortal)
                        portal->writable.queuedParent = 0;
                    continue;
                }

                portal->writable.queuedParent = parentPortal;

                float *nearPlaneCoeffs = dpvsG.eyePlane.coeffs;
                float priority = R_PortalMinDot(portal->vertices, portal->vertexCount, nearPlaneCoeffs);
                R_PortalQueueInsert(portal, priority);
            }
        }
    } else {

        const GfxCell *furtherCells[128];
        int cellCount = R_GetFurtherCellList_r_impl(cell, parentPlane, planes, planeCount, scratchBuf, furtherCells, 0);
        for (i = 0; i < cellCount; i++) {
            R_AddVisibleSurfacesInCell_impl(furtherCells[i], planes, planeCount);
        }
    }

    {
        GfxPortal *ancestor = parentPortal;
        while (ancestor) {
            ancestor->writable.isAncestor = 0;
            GfxPortal *next = ancestor->writable.queuedParent;
            ancestor = next;
        }
    }
}

static void R_DPVS_REGPARM3_ABI R_VisitPortalsForCell(const GfxCell *cell, GfxPortal *parentPortal, const DpvsPlane *parentPlane, const DpvsPlane *planes, int planeCount, DpvsClipChildren clipChildren)
{
    R_VisitPortalsForCell_impl(cell, parentPortal, parentPlane, planes, planeCount, clipChildren);
}

static inline GfxPortal *R_PortalQueuePopMin(void)
{
    byte *queueBase = (byte *)dpvsG.portalQueue;
    GfxPortal *top = *(GfxPortal **)queueBase;
    top->writable.isQueued = 0;

    int count = dpvsG.portalQueueCount - 1;
    dpvsG.portalQueueCount = count;

    if (count > 0) {

        int idx = 0;
        int child = 1;
        while (child < count) {
            float childPrio = *(float *)(queueBase + child * 8 + 4);

            int rightChild = child + 1;
            if (rightChild <= count) {
                float rightPrio = *(float *)(queueBase + rightChild * 8 + 4);
                if (childPrio > rightPrio) {

                    child = rightChild;
                    childPrio = rightPrio;
                }
            }

            float lastPrio = *(float *)(queueBase + count * 8 + 4);
            if (lastPrio <= childPrio)
                break;

            *(int *)(queueBase + idx * 8) = *(int *)(queueBase + child * 8);
            *(float *)(queueBase + idx * 8 + 4) = childPrio;
            idx = child;
            child = idx * 2 + 1;
        }

        *(int *)(queueBase + idx * 8) = *(int *)(queueBase + count * 8);
        *(float *)(queueBase + idx * 8 + 4) = *(float *)(queueBase + count * 8 + 4);
    }

    return top;
}

static inline float R_Polygon2DSignedArea(const vec2_t *pts, int count)
{
    float area = 0.0f;
    for (int i = 1; i < count; i++) {
        area += pts[i][0] * (pts[i][1] - pts[i - 1][1]);
    }
    return area * 0.125f;
}

extern void R_AddDebugPolygon(void *debugGlobals, const float *color, int vertCount, const vec_t *verts);

static void R_VisitPortals_impl(const GfxCell *cell, const DpvsPlane *parentPlane, const DpvsPlane *planes, int planeCount)
{
    LargeLocal hullPointsPool_large_local;
    DpvsPlane childPlanes[64];
    vec3_t portalVerts[64];
    vec3_t normals[64];
    vec3_t bevelVerts[5];
    vec3_t bevelNormals[5];
    vec2_t hull[64];
    byte portalQueue[3072];
    int iteration, childPlaneCount, hullPointCount, i;
    float clipArea;
    int clipChildren;

    LargeLocal_LargeLocal(&hullPointsPool_large_local, 0x20000);
    byte *poolBuf = (byte *)LargeLocal_GetBuf(&hullPointsPool_large_local);

    for (i = 0; i < 255; i++) {
        *(void **)(poolBuf + (i + 1) * 0x200) = poolBuf + (i + 2) * 0x200;
    }
    *(void **)(poolBuf + 255 * 0x200 + 0x200) = 0;

    for (i = 0; i < 255; i++) {
        *(void **)(poolBuf + i * 0x200) = poolBuf + (i + 1) * 0x200;
    }
    *(void **)(poolBuf + 0x1FE00) = 0;

    *(void **)&dpvsG.portalPool = poolBuf;

    dpvsG.portalQueue = (GfxPortal **)portalQueue;
    dpvsG.portalQueueCount = 0;

    R_VisitPortalsForCell(cell, 0, parentPlane, planes, planeCount, DPVS_CLIP_CHILDREN);

    iteration = 0;

    while (dpvsG.portalQueueCount > 0) {

        GfxPortal *portal = R_PortalQueuePopMin();

        hullPointCount = Com_ConvexHull(portal->hullPoints, portal->hullPointCount, hull);

        R_FreeHullPointsBuf(portal->hullPoints);
        portal->hullPoints = 0;

        if (hullPointCount == 0)
            continue;

        iteration++;
        if (iteration == (*(const dvar_t **)imp_r_portalWalkLimit)->current.integer)
            break;

        float nd = -portal->plane.coeffs[3];
        float baseX = nd * portal->plane.coeffs[0];
        float baseY = nd * portal->plane.coeffs[1];
        float baseZ = nd * portal->plane.coeffs[2];

        for (i = 0; i < hullPointCount; i++) {
            portalVerts[i][0] = baseX + hull[i][0] * portal->hullAxis[0][0] + hull[i][1] * portal->hullAxis[1][0];
            portalVerts[i][1] = baseY + hull[i][0] * portal->hullAxis[0][1] + hull[i][1] * portal->hullAxis[1][1];
            portalVerts[i][2] = baseZ + hull[i][0] * portal->hullAxis[0][2] + hull[i][1] * portal->hullAxis[1][2];
        }

        if ((*(const dvar_t **)imp_r_showPortals)->current.integer) {
            if (!((*(const dvar_t **)imp_r_portalBevelsOnly)->current.enabled)) {
                DebugGlobals *debugGlobals = &frontEndDataOut->debugGlobals;
                R_AddDebugPolygon(debugGlobals, (const float *)&dpvsConfig, hullPointCount, (vec_t *)portalVerts);
            }
        }

        int useNormalPlanes = (hullPointCount <= 10);
        int doBevels, forceBevels;

        if (useNormalPlanes && !((*(const dvar_t **)imp_r_portalBevelsOnly)->current.enabled)) {
            float bevelThreshold = (*(const dvar_t **)imp_r_portalBevels)->current.value;
            if (bevelThreshold > 0.0f) {
                doBevels = 1;
                forceBevels = 0;
            } else {
                doBevels = 0;
                forceBevels = 0;
            }
        } else {

            doBevels = 1;
            forceBevels = 1;
        }

        R_GetSidePlaneNormals((vec3_t *)portalVerts, hullPointCount, normals);

        if (!doBevels) {
            float minClipArea = (*(const dvar_t **)imp_r_portalMinClipArea)->current.value;
            if (minClipArea <= 0.0f) {
                childPlaneCount = 0;
                clipChildren = 1;
            } else {
                childPlaneCount = 0;
                clipChildren = 1;
            }
        } else {

            const D3DMATRIX *viewProj = dpvsG.inverseViewProjectionMatrix;
            const float *mtx = (const float *)viewProj;
            vec2_t screenVerts[64];
            float minX = 1.0f, maxX = -1.0f, minY = 1.0f, maxY = -1.0f;
            int nearClip = 0;

            float fz = portalVerts[0][0] * mtx[0x0C] + portalVerts[0][1] * mtx[0x1C] + portalVerts[0][2] * mtx[0x2C] + mtx[0x3C];
            if (fz < 0.125f) {

                clipChildren = 1;
                minX = -1.0f;
                maxX = 1.0f;
                minY = -1.0f;
                maxY = 1.0f;
                nearClip = 1;
            }

            if (!nearClip) {

                for (i = 0; i < hullPointCount; i++) {
                    float fx = portalVerts[i][0];
                    float fy = portalVerts[i][1];
                    float fzz = portalVerts[i][2];
                    float w = fx * mtx[0x0C] + fy * mtx[0x1C] + fzz * mtx[0x2C] + mtx[0x3C];
                    if (w < 0.125f) {
                        nearClip = 1;
                        clipChildren = 1;
                        minX = -1.0f;
                        maxX = 1.0f;
                        minY = -1.0f;
                        maxY = 1.0f;
                        break;
                    }
                    float invW = 1.0f / w;
                    float sx = (fx * mtx[0x00] + fy * mtx[0x10] + fzz * mtx[0x20] + mtx[0x30]) * invW;
                    float sy = (fx * mtx[0x04] + fy * mtx[0x14] + fzz * mtx[0x24] + mtx[0x34]) * invW;
                    float sz = (fx * mtx[0x08] + fy * mtx[0x18] + fzz * mtx[0x28] + mtx[0x38]) * invW;
                    screenVerts[i][0] = sx;
                    screenVerts[i][1] = sy;
                    if (sx < minX)
                        minX = sx;
                    if (sx > maxX)
                        maxX = sx;
                    if (sy < minY)
                        minY = sy;
                    if (sy > maxY)
                        maxY = sy;
                }
            }

            if (!nearClip) {

                float sizeX = maxX - minX;
                float sizeY = maxY - minY;
                float area = sizeX * sizeY * 0.25f;
                float minClipArea = (*(const dvar_t **)imp_r_portalMinClipArea)->current.value;
                clipChildren = (minClipArea <= area) ? 0 : 1;
            }

            childPlaneCount = 0;

            if (forceBevels) {

            }
        }

        if (useNormalPlanes && hullPointCount > 0) {
            for (i = 0; i < hullPointCount; i++) {
                float lenSq = normals[i][0] * normals[i][0] + normals[i][1] * normals[i][1] + normals[i][2] * normals[i][2];
                if (lenSq == 0.0f)
                    continue;
                childPlanes[childPlaneCount].coeffs[0] = normals[i][0];
                childPlanes[childPlaneCount].coeffs[1] = normals[i][1];
                childPlanes[childPlaneCount].coeffs[2] = normals[i][2];
                float dot = normals[i][0] * portalVerts[i][0] + normals[i][1] * portalVerts[i][1] + normals[i][2] * portalVerts[i][2];
                childPlanes[childPlaneCount].coeffs[3] = 0.001f - dot;
                R_DpvsPlaneSetSideBytes(&childPlanes[childPlaneCount]);
                childPlaneCount++;
            }
        }

        if ((*(const dvar_t **)imp_r_showPortals)->current.integer) {

        }

        DpvsPlane *eyePlane = dpvsG.clipPlanes;
        if (eyePlane) {

            childPlanes[childPlaneCount] = *eyePlane;
            float minDot = R_PortalMinDot((const vec3_t *)portalVerts, hullPointCount, eyePlane->coeffs);
            if (minDot > 0.0f) {
                childPlanes[childPlaneCount].coeffs[3] -= minDot;
            }
            childPlaneCount++;
        }

        DpvsPlane *farPlane = dpvsG.farPlanePtr;
        if (farPlane) {
            childPlanes[childPlaneCount] = *farPlane;
            childPlaneCount++;
        }

        GfxCell *destCell = portal->cell;
        R_VisitPortalsForCell(destCell, portal, &portal->plane, childPlanes, childPlaneCount,
                              clipChildren ? DPVS_CLIP_CHILDREN : DPVS_DONT_CLIP_CHILDREN);
    }

    while (dpvsG.portalQueueCount > 0) {
        GfxPortal *portal = R_PortalQueuePopMin();
        if (portal->hullPoints) {
            R_FreeHullPointsBuf(portal->hullPoints);
            portal->hullPoints = 0;
        }
    }

    ZN10LargeLocalD1Ev(&hullPointsPool_large_local);
}

static void R_VisitPortals(const GfxCell *cell, const DpvsPlane *parentPlane, const DpvsPlane *planes, int planeCount)
{
    R_VisitPortals_impl(cell, parentPlane, planes, planeCount);
}

static inline void R_DpvsInitEyePlane(DpvsPlane *plane, const float *normal)
{
    float *eye = dpvsG.eyePos;
    plane->coeffs[0] = normal[0];
    plane->coeffs[1] = normal[1];
    plane->coeffs[2] = normal[2];
    float dot = normal[0] * eye[0] + normal[1] * eye[1] + normal[2] * eye[2];
    float w = dpvsG.eyeW;
    float epsilon = (w != 0.0f) ? 0.1f : 262144.0f;
    plane->coeffs[3] = epsilon - dot;
    R_DpvsPlaneSetSideBytes(plane);
}

static inline void R_DpvsInitFarPlane(DpvsPlane *plane, const float *viewDir, float cullDist)
{
    float *eye = dpvsG.eyePos;
    float negDir[3];
    negDir[0] = -viewDir[0];
    negDir[1] = -viewDir[1];
    negDir[2] = -viewDir[2];
    plane->coeffs[0] = negDir[0];
    plane->coeffs[1] = negDir[1];
    plane->coeffs[2] = negDir[2];
    float dot = negDir[0] * eye[0] + negDir[1] * eye[1] + negDir[2] * eye[2];
    plane->coeffs[3] = cullDist - dot;
    R_DpvsPlaneSetSideBytes(plane);
}

static inline int R_CullBoundsAgainstFrustumAndOccluders(const float *bounds, const DpvsPlane *frustumPlanes, int frustumPlaneCount)
{
    int i;
    for (i = 0; i < frustumPlaneCount; i++) {
        DpvsPlane *plane = (DpvsPlane *)&frustumPlanes[i];
        plane->u.frontal = 0xFF;
        if (R_DpvsPlaneBoundsTest(plane, bounds) <= 0.0f)
            return 0;
    }

    int occCount = dpvsG.occluderCount;
    if (occCount > 0) {
        GfxOccluder **occTable = dpvsG.occluderList;
        for (i = 0; i < occCount; i++) {
            GfxOccluder *occ = (GfxOccluder *)occTable[i];
            int planeCount = occ->viewPlaneCount;
            DpvsPlane *planes = occ->viewPlanes;
            if (planeCount <= 0)
                return 0;
            int j;
            for (j = 0; j < planeCount; j++) {
                planes[j].u.frontal = 0xFF;
                if (R_DpvsPlaneBoundsTest(&planes[j], bounds) > 0.0f)
                    break;
            }
            if (j >= planeCount)
                return 0;
        }
    }
    return 1;
}

extern void R_AddBModelSurfaces(void *sceneEnt, int entIndex);


static void R_AddWorldSurfacesDpvs_impl(const GfxViewParms *viewParms, int cameraCellIndex)
{
    LargeLocal activeOccluderBuffer_large_local;
    LargeLocal occluderPlaneBuffer_large_local;
    DpvsPlane frustumPlanes[6];
    int frustumPlaneCount;
    int i;

    LargeLocal_LargeLocal(&activeOccluderBuffer_large_local, 0x1000);
    byte *activeOccluderBuf = (byte *)LargeLocal_GetBuf(&activeOccluderBuffer_large_local);
    LargeLocal_LargeLocal(&occluderPlaneBuffer_large_local, 0x1e000);
    byte *occluderPlaneBuf = (byte *)LargeLocal_GetBuf(&occluderPlaneBuffer_large_local);

    dpvsG.occluderList = (GfxOccluder **)activeOccluderBuf;
    dpvsG.viewPlanes = (struct DpvsPlane *)occluderPlaneBuf;

    byte drawWorld = (byte)(*(const dvar_t **)imp_r_drawWorld)->current.integer;
    dpvsG.drawWorld = drawWorld;
    byte drawEntities = (byte)(*(const dvar_t **)imp_r_drawEntities)->current.integer;
    dpvsG.drawEntities = drawEntities;
    byte drawBModels = ((*(const dvar_t **)imp_r_drawBModels)->current.integer && drawEntities) ? 1 : 0;
    dpvsG.drawBmodels = drawBModels;
    byte drawSModels = ((*(const dvar_t **)imp_r_drawSModels)->current.integer && drawEntities) ? 1 : 0;
    dpvsG.drawSmodels = drawSModels;
    byte drawXModels = ((*(const dvar_t **)imp_r_drawXModels)->current.integer && drawEntities) ? 1 : 0;
    dpvsG.drawXmodels = drawXModels;

    dpvsG.occluderCount = 0;

    dpvsG.viewProjectionMatrix = &viewParms->viewProjectionMatrix;
    dpvsG.inverseViewProjectionMatrix = &viewParms->inverseViewProjectionMatrix;

    R_FrustumClipPlanes(&viewParms->viewProjectionMatrix, (vec4_t *)standardFrustumSidePlanes, 4, frustumPlanes);

    float projW = viewParms->projectionMatrix.m[3][3];
    float *eye = dpvsG.eyePos;
    float *nearPlaneNormal = dpvsG.eyePlane.coeffs;
    const float *viewDir;

    if (projW != 0.0f) {

        float *negViewDir = dpvsG.eyePos;
        negViewDir[0] = -viewParms->axis[0][0];
        negViewDir[1] = -viewParms->axis[0][1];
        negViewDir[2] = -viewParms->axis[0][2];
        *(int *)&dpvsG.eyeW = 0;

        nearPlaneNormal[0] = viewParms->axis[0][0];
        nearPlaneNormal[1] = viewParms->axis[0][1];
        nearPlaneNormal[2] = viewParms->axis[0][2];

        float dot = nearPlaneNormal[0] * negViewDir[0] + nearPlaneNormal[1] * negViewDir[1] + nearPlaneNormal[2] * negViewDir[2];
        dpvsG.eyePlane.coeffs[3] = 262144.0f - dot;
        R_DpvsPlaneSetSideBytes(&dpvsG.eyePlane);

        dpvsG.clipPlanes = 0;
        dpvsG.farPlanePtr = 0;

        viewDir = viewParms->axis[0];
    } else {

        eye[0] = viewParms->origin[0];
        eye[1] = viewParms->origin[1];
        eye[2] = viewParms->origin[2];
        *(int *)&dpvsG.eyeW = 0x3f800000;

        viewDir = viewParms->axis[0];

        nearPlaneNormal[0] = viewDir[0];
        nearPlaneNormal[1] = viewDir[1];
        nearPlaneNormal[2] = viewDir[2];

        float dot = nearPlaneNormal[0] * eye[0] + nearPlaneNormal[1] * eye[1] + nearPlaneNormal[2] * eye[2];
        dpvsG.eyePlane.coeffs[3] = 0.1f - dot;
        R_DpvsPlaneSetSideBytes(&dpvsG.eyePlane);

        dpvsG.clipPlanes = &dpvsG.eyePlane;
    }

    float farPlaneDist = (*(const dvar_t **)imp_r_zfar)->current.value;
    if (farPlaneDist == 0.0f) {
        if (rg.fogIndex && rg.fogSettings[2].registered && rg.fogSettings[2].techniqueOffset == 1) {
            farPlaneDist = rg.fogSettings[2].fogEnd;
        }
    }
    float cullDist = *(float *)&dpvsConfig;
    if (cullDist < farPlaneDist)
        cullDist = farPlaneDist;

    if (cullDist > 0.0f) {

        R_DpvsInitFarPlane(&dpvsG.farPlane, viewDir, cullDist);
        dpvsG.farPlanePtr = &dpvsG.farPlane;
    } else {
        dpvsG.farPlanePtr = 0;
    }

    DpvsPlane *nearPlane = dpvsG.clipPlanes;
    DpvsPlane *farPlane = dpvsG.farPlanePtr;

    if (nearPlane) {

        frustumPlanes[4] = *nearPlane;
        frustumPlaneCount = 5;
    } else {
        frustumPlaneCount = 4;
    }

    if (farPlane) {

        frustumPlanes[frustumPlaneCount] = *farPlane;
        frustumPlaneCount++;
    }

    rg.debugViewParms = viewParms;

    {
        int entityCount = scene.def.entityCount;
        GfxEntity *entities = scene.def.entities;

        for (i = 0; i < entityCount; i++) {
            GfxEntity *ent = &entities[i];
            int entType = ent->reType;

            if (entType <= 1) {

                GfxSceneEntity *sceneEnt = &scene.sceneEnts[i];
                void *dobj = R_GetGfxEntityDObj(sceneEnt, ent);

                vec3_t objmins, objmaxs;

                ((void (*)(void *, vec3_t *, vec3_t *))ri.DObjGetBounds)(dobj, &objmins, &objmaxs);

                float scale = ent->scale;
                float bounds[6];

                bounds[0] = scale * objmins[0] + ent->origin[0];
                bounds[1] = scale * objmins[1] + ent->origin[1];
                bounds[2] = scale * objmins[2] + ent->origin[2];

                bounds[3] = scale * objmaxs[0] + ent->origin[0];
                bounds[4] = scale * objmaxs[1] + ent->origin[1];
                bounds[5] = scale * objmaxs[2] + ent->origin[2];

                if (!R_CullBoundsAgainstFrustumAndOccluders(bounds, frustumPlanes, frustumPlaneCount))
                    continue;

                float sizeX = bounds[3] - bounds[0];
                if (sizeX <= 1536.0f) {
                    float sizeY = bounds[4] - bounds[1];
                    if (sizeY > 1536.0f) {
                        float sizeZ = bounds[5] - bounds[2];
                        if (sizeZ <= 1536.0f)
                            goto entity_filter_cells;
                    } else {
                        goto entity_filter_cells;
                    }
                } else {
                    float sizeY = bounds[4] - bounds[1];
                    if (sizeY <= 1536.0f) {
                        float sizeZ = bounds[5] - bounds[2];
                        if (sizeZ <= 1536.0f)
                            goto entity_filter_cells;
                    }
                }

                goto entity_process;

            entity_filter_cells: {
                GfxWorld *world = rgp.world;
                mnode_t *bspNodes = world->nodes;
                int cell = R_FilterEntityIntoCells_r_impl(bspNodes, i, bounds, bounds + 3);
                if (cell != -1)
                    continue;
            }

            entity_process:
                R_UpdateXModelBounds(&scene.sceneEnts[i], ent);
                CG_CullIn((void *)scene.sceneEnts[i].cent);
                R_SkinSceneEnt(&scene.sceneEnts[i], ent);
                scene.sceneEnts[i].cullState = 5;
                R_AddXModelSurfaces(i);

            } else if (entType == 3) {

                GfxSceneEntity *sceneEnt = &scene.sceneEnts[i];
                void *bmodel = (void *)sceneEnt->u.data;
                vec3_t boundsMin, boundsMax, transformed;

                if (!bmodel || (unsigned int)bmodel < 0x1000)
                    continue;

                ClearBounds(boundsMin, boundsMax);

                for (int corner = 0; corner < 8; corner++) {
                    int xi = corner & 1;
                    int yi = (corner >> 1) & 1;
                    int zi = (corner >> 2) & 1;
                    float localX = ((float *)bmodel)[xi * 3];
                    float localY = ((float *)bmodel)[yi * 3 + 1];
                    float localZ = ((float *)bmodel)[zi * 3 + 2];

                    transformed[0] = ent->origin[0] + localX * ent->axis[0][0] + localY * ent->axis[1][0] + localZ * ent->axis[2][0];
                    transformed[1] = ent->origin[1] + localX * ent->axis[0][1] + localY * ent->axis[1][1] + localZ * ent->axis[2][1];
                    transformed[2] = ent->origin[2] + localX * ent->axis[0][2] + localY * ent->axis[1][2] + localZ * ent->axis[2][2];

                    AddPointToBounds(transformed, boundsMin, boundsMax);
                }

                float bounds[6];
                bounds[0] = boundsMin[0];
                bounds[1] = boundsMin[1];
                bounds[2] = boundsMin[2];
                bounds[3] = boundsMax[0];
                bounds[4] = boundsMax[1];
                bounds[5] = boundsMax[2];

                if (!R_CullBoundsAgainstFrustumAndOccluders(bounds, frustumPlanes, frustumPlaneCount))
                    continue;

                GfxWorld *world = rgp.world;
                mnode_t *bspNodes = world->nodes;
                int cell = R_FilterEntityIntoCells_r_impl(bspNodes, i, boundsMin, boundsMax);
                if (cell != -1)
                    continue;

                R_AddBModelSurfaces(sceneEnt, i);
            }
        }
    }

    if (!(*(const dvar_t **)imp_r_skipPvs)->current.enabled) {
        GfxWorld *world = rgp.world;
        GfxCell *cells = world->cells;

        if (cameraCellIndex < 0) {
            int cellCount = world->cellCount;
            for (i = 0; i < cellCount; i++)
                R_AddVisibleSurfacesInCell_impl(&cells[i], frustumPlanes, frustumPlaneCount);
        } else {
            GfxCell *cameraCell = &cells[cameraCellIndex];
            if ((*(const dvar_t **)imp_r_singleCell)->current.enabled) {
                dpvsG.farPlanePtr = 0;
                R_AddVisibleSurfacesInCell_impl(cameraCell, frustumPlanes, frustumPlaneCount);
            } else {
                R_VisitPortals(cameraCell, &dpvsG.eyePlane, frustumPlanes, frustumPlaneCount);
            }
        }
    }

    {
        GfxWorld *world = rgp.world;
        int skySurfCount = world->skySurfCount;
        if (dpvsG.farPlanePtr != 0 && skySurfCount > 0) {
            int *skyStartSurfs = world->skyStartSurfs;
            for (i = 0; i < skySurfCount; i++) {
                int surfIndex = skyStartSurfs[i];
                R_AddWorldSurfaceWithCull_impl(surfIndex, frustumPlanes, frustumPlaneCount - 1, 0);
            }
        }
    }

    {
        int dlightCount = scene.dlightCount;
        if (dlightCount > 0) {
            for (i = 0; i < dlightCount; i++) {
                float *dlightOrigin = (float *)scene.dlights[i].position;
                float dlightRadius = scene.dlights[i].position[3];
                int result = R_CullPointAndRadius(dlightOrigin, dlightRadius, frustumPlanes, frustumPlaneCount);
                scene.dlightCulled[i] = (result == 2) ? 1 : 0;
            }
        }
    }

    if ((*(const dvar_t **)imp_r_vc_makelog)->current.integer)
        RB_ShowLightVisCachePoints(viewParms, frustumPlanes, frustumPlaneCount);

    ZN10LargeLocalD1Ev(&occluderPlaneBuffer_large_local);
    ZN10LargeLocalD1Ev(&activeOccluderBuffer_large_local);
}

void R_AddWorldSurfacesDpvs(const GfxViewParms *viewParms, int cameraCellIndex)
{
    R_AddWorldSurfacesDpvs_impl(viewParms, cameraCellIndex);
}

const unsigned char standardFrustumSidePlanes[64] = { 0, 0, 128, 191, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 128, 63, 0, 0, 128, 63, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 128, 63, 0, 0, 0, 0, 0, 0, 128, 191, 0, 0, 0, 0, 0, 0, 128, 63, 0, 0, 0, 0, 0, 0, 128, 63, 0, 0, 0, 0, 0, 0, 128, 63 };

const unsigned char __ZZ14R_VisitPortalsPK7GfxCellPK9DpvsPlaneS4_iE5color[128] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x80, 0x3e,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
