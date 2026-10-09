#include "common_types.h"
#include "imports.h"

#include <string.h>
extern refexport_t re;

extern float sinf(float x);
extern float cosf(float x);
extern double tan(double x);
extern int CM_BoxTrace(trace_t *results, const vec_t *start, const vec_t *end, const vec_t *mins, const vec_t *maxs, unsigned int model, int brushmask);
extern GfxEntity *R_AddRefEntityToScene(const GfxEntity *ent, GfxModel model, const struct centity_s *cent);
extern void FX_AddScheduledEffects(void);
extern void FX_UpdateAllNonBolt(void);
extern void FX_UpdateAllBolt(void);
extern void FxArchive_ReadData(FxArchive *arch, void *data, int size);
extern void FxArchive_WriteData(FxArchive *arch, void *data, int size);

void FxHelper_FxHelper(const FxHelper *_this);
void FxHelper_Init(const FxHelper *_this);
void FxHelper_AdjustTime(const FxHelper *_this, int intime);
void FxHelper_WarpTime(const FxHelper *_this, int intime);
void FxHelper_Trace(const FxHelper *_this, trace_t *tr, vec_t *start, const vec_t *min, const vec_t *max, vec_t *end, int skipEntNum, int flags);
void FxHelper_AddFxToScene(const FxHelper *_this, GfxEntity *ent, const struct XModel *model);
void FxHelper_SetIgnorePrecacheErrors(const FxHelper *_this, int ignore);
const char *FxHelper_GetMaterialName(const FxHelper *_this, MaterialHandle handle);
int FxHelper_GetMaterialSubimageCount(const FxHelper *_this, MaterialHandle handle);
Bool FxHelper_IsMaterialRefractive(const FxHelper *_this, MaterialHandle handle);
void FxHelper_AddLightToScene(const FxHelper *_this, vec_t *org, float radius, float red, float green, float blue);
void FxHelper_CameraShake(const FxHelper *_this, vec_t *origin, float intensity, int radius, int time_);
int FxHelper_GetSeed(const FxHelper *_this);
Bool FxHelper_CullSphere(const FxHelper *_this, const vec_t *worldPos, float radius, int planeCount);
Bool FxHelper_CullSpherePreviousFrame(const FxHelper *_this, const vec_t *worldPos, float radius);
Bool FxHelper_CullCylinder(const FxHelper *_this, const vec_t *worldPos0, const vec_t *worldPos1, float radius0, float radius1, int planeCount);
void FxHelper_CalcFrustumPlanes(const FxHelper *_this, refdef_t *refdef, float zfar);
void FxHelper_AdjustCamera(const FxHelper *_this, refdef_t *refdef, float zfar);
void FxHelper_Archive(const FxHelper *_this, FxArchive *arch);

#define RE (&re)
#define FX_HELPER (*(FxHelper **)imp_theFxHelper)

void FxHelper_FxHelper(const FxHelper *_this)
{
    FxHelper *self = (FxHelper *)_this;
    self->mTime = 0;
    self->mOldTime = 0;
    self->mFrameTime = 0;
    self->mTimeFrozen = 0;
}

void FxHelper_Init(const FxHelper *_this)
{
    FxHelper *self = (FxHelper *)_this;
    self->time = 0;
    self->mTime = 0;
    self->mOldTime = 0;
    self->mFrameTime = 0;
    self->mTimeFrozen = 0;
    self->mSeed = 0;
}

void FxHelper_AdjustTime(const FxHelper *_this, int intime)
{
    FxHelper *self = (FxHelper *)_this;
    char *frozenStruct = *(char **)(void *)imp_fx_freeze;

    if (frozenStruct[8]) {
        self->mFrameTime = 0;
    } else {
        if (self->time == 0) {
            self->mFrameTime = 0;
        } else {
            int delta = intime - self->time;
            self->mFrameTime = delta;
            if (delta < 0) {
                self->mFrameTime = 0;
            } else if (delta > 200) {
                self->mFrameTime = 200;
            }
        }
        self->mOldTime = self->mTime;
        self->mTime += self->mFrameTime;
    }

    self->time = intime;
    self->mSeed = intime;
    memcpy(&self->mPrevCamera, &self->mCamera, sizeof(FxCamera));
}

void FxHelper_WarpTime(const FxHelper *_this, int intime)
{
    FxHelper *self = (FxHelper *)_this;
    char *frozenStruct = *(char **)(void *)imp_fx_freeze;
    int frameTime;

    if (frozenStruct[8] || self->time == 0) {
        self->mFrameTime = 0;
        goto set_time_seed;
    }

    self->mOldTime = self->mTime;
    frameTime = intime - self->mTime;

    if (frameTime < 0) {
        self->mTime = intime;
    set_time_seed:
        self->time = intime;
        self->mSeed = intime;
        return;
    }

    self->mFrameTime = 0;

    while (frameTime > 200) {
        self->mFrameTime = 200;
        FX_AddScheduledEffects();
        FX_UpdateAllNonBolt();
        FX_UpdateAllBolt();
        self->mOldTime = self->mTime;
        self->mTime += self->mFrameTime;
        frameTime -= 200;
    }

    self->mFrameTime = frameTime;
    FX_AddScheduledEffects();
    FX_UpdateAllNonBolt();
    FX_UpdateAllBolt();
    self->mTime += self->mFrameTime;
    self->time = intime;
    self->mSeed = intime;
}

void FxHelper_Trace(const FxHelper *_this, trace_t *tr, vec_t *start, const vec_t *min, const vec_t *max, vec_t *end, int skipEntNum, int flags)
{
    CM_BoxTrace(tr, start, end, min, max, 0, flags);
    if (tr->fraction == 1.0f) {
        tr->entityNum = 1023;
    } else {
        tr->entityNum = 1022;
    }
}

void FxHelper_AddFxToScene(const FxHelper *_this, GfxEntity *ent, const struct XModel *model)
{
    GfxModel sceneModel;
    sceneModel.model = (struct XModel *)model;
    R_AddRefEntityToScene(ent, sceneModel, NULL);
}

void FxHelper_SetIgnorePrecacheErrors(const FxHelper *_this, int ignore)
{
    RE->SetIgnorePrecacheErrors((unsigned char)ignore);
}

const char *FxHelper_GetMaterialName(const FxHelper *_this, MaterialHandle handle)
{
    return RE->GetMaterialName(handle);
}

int FxHelper_GetMaterialSubimageCount(const FxHelper *_this, MaterialHandle handle)
{
    return RE->GetMaterialSubimageCount(handle);
}

Bool FxHelper_IsMaterialRefractive(const FxHelper *_this, MaterialHandle handle)
{
    return RE->IsMaterialRefractive(handle);
}

void FxHelper_AddLightToScene(const FxHelper *_this, vec_t *org, float radius, float red, float green, float blue)
{
    RE->AddLightToScene(org, radius, red, green, blue);
}

void FxHelper_CameraShake(const FxHelper *_this, vec_t *origin, float intensity, int radius, int time_)
{
}

int FxHelper_GetSeed(const FxHelper *_this)
{
    FxHelper *self = (FxHelper *)_this;
    int seed = self->mSeed;
    self->mSeed = seed * 0x369d035;
    return seed;
}

Bool FxHelper_CullSphere(const FxHelper *_this, const vec_t *worldPos, float radius, int planeCount)
{
    FxHelper *helper = FX_HELPER;
    float negRadius = -radius;
    int i;

    for (i = 0; i < planeCount; i++) {
        float dot = worldPos[0] * helper->mCamera.frustum[i][0] + worldPos[1] * helper->mCamera.frustum[i][1] + worldPos[2] * helper->mCamera.frustum[i][2] - helper->mCamera.frustum[i][3];
        if (dot < negRadius) {
            return 1;
        }
    }
    return 0;
}

Bool FxHelper_CullSpherePreviousFrame(const FxHelper *_this, const vec_t *worldPos, float radius)
{
    FxHelper *helper = FX_HELPER;
    float negRadius = -radius;
    int numPlanes = helper->mPrevCamera.numPlanes;
    int i;

    for (i = 0; i < numPlanes; i++) {
        float dot = worldPos[0] * helper->mPrevCamera.frustum[i][0] + worldPos[1] * helper->mPrevCamera.frustum[i][1] + worldPos[2] * helper->mPrevCamera.frustum[i][2] - helper->mPrevCamera.frustum[i][3];
        if (dot < negRadius) {
            return 1;
        }
    }
    return 0;
}

Bool FxHelper_CullCylinder(const FxHelper *_this, const vec_t *worldPos0, const vec_t *worldPos1, float radius0, float radius1, int planeCount)
{
    FxHelper *helper = FX_HELPER;
    float negRadius0 = -radius0;
    float negRadius1 = -radius1;
    int i;

    for (i = 0; i < planeCount; i++) {
        float dot0 = worldPos0[0] * helper->mCamera.frustum[i][0] + worldPos0[1] * helper->mCamera.frustum[i][1] + worldPos0[2] * helper->mCamera.frustum[i][2] - helper->mCamera.frustum[i][3];
        if (dot0 <= negRadius0) {
            float dot1 = worldPos1[0] * helper->mCamera.frustum[i][0] + worldPos1[1] * helper->mCamera.frustum[i][1] + worldPos1[2] * helper->mCamera.frustum[i][2] - helper->mCamera.frustum[i][3];
            if (dot1 <= negRadius1) {
                return 1;
            }
        }
    }
    return 0;
}

void FxHelper_CalcFrustumPlanes(const FxHelper *_this, refdef_t *refdef, float zfar)
{
    FxHelper *self = (FxHelper *)_this;
    float fAngle, s, c;
    int i;

    self->mCamera.frustum[0][0] = refdef->viewaxis[0][0];
    self->mCamera.frustum[0][1] = refdef->viewaxis[0][1];
    self->mCamera.frustum[0][2] = refdef->viewaxis[0][2];

    fAngle = (float)((double)refdef->fov_x * 0.008726646259971648);
    s = sinf(fAngle);
    c = cosf(fAngle);

    self->mCamera.frustum[1][0] = s * refdef->viewaxis[0][0] + c * refdef->viewaxis[1][0];
    self->mCamera.frustum[1][1] = s * refdef->viewaxis[0][1] + c * refdef->viewaxis[1][1];
    self->mCamera.frustum[1][2] = s * refdef->viewaxis[0][2] + c * refdef->viewaxis[1][2];

    self->mCamera.frustum[2][0] = s * refdef->viewaxis[0][0] - c * refdef->viewaxis[1][0];
    self->mCamera.frustum[2][1] = s * refdef->viewaxis[0][1] - c * refdef->viewaxis[1][1];
    self->mCamera.frustum[2][2] = s * refdef->viewaxis[0][2] - c * refdef->viewaxis[1][2];

    fAngle = (float)((double)refdef->fov_y * 0.008726646259971648);
    s = sinf(fAngle);
    c = cosf(fAngle);

    self->mCamera.frustum[3][0] = s * refdef->viewaxis[0][0] + c * refdef->viewaxis[2][0];
    self->mCamera.frustum[3][1] = s * refdef->viewaxis[0][1] + c * refdef->viewaxis[2][1];
    self->mCamera.frustum[3][2] = s * refdef->viewaxis[0][2] + c * refdef->viewaxis[2][2];

    self->mCamera.frustum[4][0] = s * refdef->viewaxis[0][0] - c * refdef->viewaxis[2][0];
    self->mCamera.frustum[4][1] = s * refdef->viewaxis[0][1] - c * refdef->viewaxis[2][1];
    self->mCamera.frustum[4][2] = s * refdef->viewaxis[0][2] - c * refdef->viewaxis[2][2];

    self->mCamera.numPlanes = 5;

    if (zfar > 0.0f) {
        self->mCamera.frustum[5][0] = -refdef->viewaxis[0][0];
        self->mCamera.frustum[5][1] = -refdef->viewaxis[0][1];
        self->mCamera.frustum[5][2] = -refdef->viewaxis[0][2];
        self->mCamera.numPlanes = 6;
    }

    for (i = 0; i < self->mCamera.numPlanes; i++) {
        self->mCamera.frustum[i][3] = self->mCamera.vieworg[0] * self->mCamera.frustum[i][0] + self->mCamera.vieworg[1] * self->mCamera.frustum[i][1] + self->mCamera.vieworg[2] * self->mCamera.frustum[i][2];
    }

    if (zfar > 0.0f) {
        self->mCamera.frustum[5][3] -= zfar;
    }
}

void FxHelper_AdjustCamera(const FxHelper *_this, refdef_t *refdef, float zfar)
{
    FxHelper *self = (FxHelper *)_this;

    self->mCamera.vieworg[0] = refdef->vieworg[0];
    self->mCamera.vieworg[1] = refdef->vieworg[1];
    self->mCamera.vieworg[2] = refdef->vieworg[2];

    FxHelper_CalcFrustumPlanes(_this, refdef, zfar);

    if (refdef->fov_x == 80.0f) {
        self->adsZoomFactor = 1.0f;
    } else {
        double tanRef = tan(40.0 * 0.017453292519943295);
        double tanCur = tan((double)refdef->fov_x * 0.5 * 0.017453292519943295);
        self->adsZoomFactor = (float)(tanRef / tanCur);
    }
}

void FxHelper_Archive(const FxHelper *_this, FxArchive *arch)
{
    FxHelper *self = (FxHelper *)_this;
    int i;
    unsigned char *archFlags = (unsigned char *)arch;

    if (archFlags[4]) {
        FxArchive_ReadData(arch, &i, 4);
        self->mTime = i;
    } else {
        i = self->mTime;
        FxArchive_WriteData(arch, &i, 4);
    }

    if (archFlags[4]) {
        FxArchive_ReadData(arch, &i, 4);
        self->mOldTime = i;
    } else {
        i = self->mOldTime;
        FxArchive_WriteData(arch, &i, 4);
    }

    if (archFlags[4]) {
        FxArchive_ReadData(arch, &i, 4);
        self->mFrameTime = i;
    } else {
        i = self->mFrameTime;
        FxArchive_WriteData(arch, &i, 4);
    }

    if (archFlags[4]) {
        FxArchive_ReadData(arch, &i, 4);
        self->mTimeFrozen = i;
    } else {
        i = self->mTimeFrozen;
        FxArchive_WriteData(arch, &i, 4);
    }

    if (archFlags[4]) {
        self->time = 0;
    }
}
