#include "common_types.h"
#include "imports.h"

extern volatile qboolean fx_camera_valid;

extern void *Com_GetClientDObj(int entNum, int localClientNum);
extern int DObjGetBoneIndex(void *dobj, unsigned int bone);
extern void FxScheduler_PlayEffect(const FxScheduler *scheduler, const EffectTemplate *fx,
                                    const vec_t *org, const vec3_t *axis, const FxBoltInfo *bolt);
extern float Vec3Normalize(vec_t *v);
extern void PerpendicularVector(const vec_t *src, vec_t *dst);
extern int FX_Init(int rendererExists);
extern void FX_Free(int freeAll);
extern void FxHelper_AdjustCamera(void *helper, void *refdef, float zfar);
extern void FxHelper_AdjustTime(void *helper, int time);
extern void FxHelper_WarpTime(void *helper, int time);
extern float FxScheduler_GetEffectLength(void *scheduler, EffectTemplate *fx);

extern byte *fx_scheduler_ptr;
extern byte *fx_helper_ptr;

extern int effectActiveCountBolt;
extern int privateEffectActiveCountBolt;
extern int effectActiveCountNonBolt;
extern int privateEffectActiveCountNonBolt;

int FX_GetBoneIndex(const int entNum, unsigned int bone);
void FX_PlaySimpleEffect(EffectTemplate *fx, const vec_t *org);
void FX_PlayEffect(EffectTemplate *fx, const vec_t *org, const vec_t *fwd);
void FX_PlayOrientedEffect(EffectTemplate *fx, const vec_t *org, const vec_t *fwd, const vec_t *up);
void FX_PlayEntityEffect(EffectTemplate *fx, const vec_t *org, vec3_t *axis, const FxBoltInfo *bolt);
int FX_InitSystem(int rendererExists);
void FX_FreeSystem(void);
void FX_FreeActive(void);
void FX_AdjustCamera(PrimType (*refdef)[256], float zfar);
void FX_AdjustTime(int time);
void FX_WarpTime(int time);
float FX_GetEffectLength(EffectTemplate *fx);
void Server_SwitchToValidFxScheduler(void);

int FX_GetBoneIndex(const int entNum, unsigned int bone)
{
    void *pObj = Com_GetClientDObj(entNum, 0);
    if (pObj == NULL)
        return -1;
    return DObjGetBoneIndex(pObj, bone);
}

void FX_PlaySimpleEffect(EffectTemplate *fx, const vec_t *org)
{
    const vec3_t axis[3] = { {1, 0, 0}, {0, 1, 0}, {0, 0, 1} };
    FxScheduler_PlayEffect(*(void **)*(void **)&fx_scheduler_ptr, fx, org, axis, NULL);
}

void FX_PlayEffect(EffectTemplate *fx, const vec_t *org, const vec_t *fwd)
{
    FX_PlayOrientedEffect(fx, org, fwd, NULL);
}

void FX_PlayOrientedEffect(EffectTemplate *fx, const vec_t *org, const vec_t *fwd, const vec_t *up)
{
    vec3_t axis[3];
    if (!fwd) {
        FX_PlaySimpleEffect(fx, org);
        return;
    }
    memcpy(axis[0], fwd, sizeof(vec3_t));
    if (Vec3Normalize(axis[0]) == 0.0f) {
        FX_PlaySimpleEffect(fx, org);
        return;
    }
    if (up) {
        float projection = up[0] * axis[0][0] + up[1] * axis[0][1] + up[2] * axis[0][2];
        for (int i = 0; i < 3; ++i) axis[2][i] = up[i] - projection * axis[0][i];
        if (Vec3Normalize(axis[2]) == 0.0f) PerpendicularVector(axis[0], axis[2]);
    } else {
        PerpendicularVector(axis[0], axis[2]);
    }
    axis[1][0] = axis[2][1] * axis[0][2] - axis[2][2] * axis[0][1];
    axis[1][1] = axis[2][2] * axis[0][0] - axis[2][0] * axis[0][2];
    axis[1][2] = axis[2][0] * axis[0][1] - axis[2][1] * axis[0][0];
    FxScheduler_PlayEffect(*(void **)*(void **)&fx_scheduler_ptr, fx, org, axis, NULL);
}

void FX_PlayEntityEffect(EffectTemplate *fx, const vec_t *org, vec3_t *axis, const FxBoltInfo *bolt)
{
    FxScheduler_PlayEffect(*(void **)*(void **)&fx_scheduler_ptr, fx, org, axis, bolt);
}

int FX_InitSystem(int rendererExists)
{
    return FX_Init((unsigned char)rendererExists);
}

void FX_FreeSystem(void)
{
    FX_Free(1);
}

void FX_FreeActive(void)
{
    FX_Free(0);
}

void FX_AdjustCamera(PrimType (*refdef)[256], float zfar)
{
    FxHelper_AdjustCamera(*(void **)*(void **)&fx_helper_ptr, refdef, zfar);
    fx_camera_valid = 1;
}

extern void *imp_effectActiveCountBolt;
extern void *imp_effectActiveCountNonBolt;
extern void *imp_privateEffectActiveCountBolt;
extern void *imp_privateEffectActiveCountNonBolt;
void FX_AdjustTime(int time)
{
    fx_camera_valid = 0;
    FxHelper_AdjustTime(*(void **)*(void **)&fx_helper_ptr, time);
    *(int *)imp_privateEffectActiveCountBolt = *(int *)imp_effectActiveCountBolt;
    *(int *)imp_privateEffectActiveCountNonBolt = *(int *)imp_effectActiveCountNonBolt;
}

void FX_WarpTime(int time)
{
    FxHelper_WarpTime(*(void **)*(void **)&fx_helper_ptr, time);
}

float FX_GetEffectLength(EffectTemplate *fx)
{
    return FxScheduler_GetEffectLength(*(void **)*(void **)&fx_scheduler_ptr, fx);
}

void Server_SwitchToValidFxScheduler(void)
{

}
