#include "cod2_feature_config.h"
#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include <math.h>

vec3_t maxs = { 4.0f, 4.0f, 4.0f };

extern const dvar_t *bg_bobMax;
extern const dvar_t *cg_dumpAnims;
extern const dvar_t *cg_errorDecay;
extern const dvar_t *cg_fov;
extern const dvar_t *cg_fovMin;
extern const dvar_t *cg_fovScale;
extern const dvar_t *cg_paused;
extern const dvar_t *cg_thirdPerson;
extern const dvar_t *cg_thirdPersonAngle;
extern const dvar_t *cg_thirdPersonRange;
extern const dvar_t *cg_viewsize;
extern void SND_SetListener(int entnum, const vec_t *origin, vec3_t *axis);
extern void R_UpdateEffectsNonBolt(void);
extern void CG_AddViewWeapon(playerState_t *ps);
extern void CL_SetLodOrigin(const refdef_t *fd);
extern void CL_IssueDelayedDrawing(int marker);
extern void CL_Input(void);
extern void CL_SetUserCmdAimValues(vec_t *angles);
extern void CL_EndDelayedDrawing(int marker);
extern void CG_UpdateShellShock(const shellshock_parms_t *parms, int start, int duration);
extern void CG_ProcessSnapshots(void);
extern void CG_DrawPlayerSprites(void);
extern void CG_DrawActive(void);
extern void CG_Draw3dHudElems(void);
extern unsigned int CG_Draw2D(void);
extern void CG_AddPacketEntities(void);
extern void CG_AddLocalEntities(void);
extern void CG_AddLagometerFrameInfo(void);

extern vec3_t mins;
extern vec3_t maxs;

extern void Com_Printf(const char *fmt, ...);
extern void FX_FreeSystem(void);
extern int FX_InitSystem(int maxEffects);
extern int Cmd_Argc(void);
extern const char *CG_Argv(int arg);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern int FX_RegisterEffect(const char *name);
extern void FX_PlaySimpleEffect(int effectIndex, const vec_t *origin);
extern double atof(const char *str);
extern int BG_GetViewmodelWeaponIndex(void *ps);
extern int BG_GetNumWeapons(void);
extern int CL_BeginDelayedDrawing(void);
extern void *BG_GetWeaponDef(int weapIndex);
extern int BG_IsAimDownSightWeapon(int weapIndex);
extern float atanf(float x);
extern float sinf(float x);
extern double tan(double x);
extern void CL_ResetSkeletonCache(int level);
extern void CG_UpdateViewWeaponAnim(void *ps);
extern struct DObj_s *Com_GetClientDObj(int handle, int localClientNum);
extern void CG_DObjUpdateInfo(struct DObj_s *obj);
extern void CG_ProcessClientNoteTracks(int clientNum);
extern void CG_CalcEntityLerpPositions(centity_t *cent);
extern void CG_ProcessEntity(centity_t *cent);
extern qboolean CG_DObjGetWorldTagPos(const centity_t *cent, struct DObj_s *obj, unsigned int tagName, vec_t *pos);
extern void DObjDisplayAnim(void *obj);
extern void R_UpdateEffectsBolt(void);
extern void CL_FX_AdjustCamera(void *refdef);
extern void FX_AdjustTime(int serverTime);
extern int CG_PointContents(const vec_t *point, int passEntityNum, int contentmask);
extern void CG_PredictPlayerState(void);
extern void AngleVectors(const vec_t *angles, vec_t *forward, vec_t *right, vec_t *up);
extern void CG_TraceCapsule(trace_t *result, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int skipNumber, int mask);
extern float CL_GetMenuBlurRadius(void);
extern void SetScreenScaling(float scaleX, float scaleY, int x, int y, int w, int h);
extern float BG_GetBobCycle(const void *ps);
extern float BG_GetSpeed(const void *ps, int serverTime);
extern float BG_GetVerticalBobFactor(const void *ps, float bobCycle, float xyspeed, float bobMax);
extern float BG_GetHorizontalBobFactor(const void *ps, float bobCycle, float xyspeed, float bobMax);
extern void BG_CalculateViewAngles(viewState_t *vs, vec_t *angles);
extern void AddLeanToPosition(vec_t *origin, float viewAngle, float leanFrac, float maxStand, float maxCrouch);
extern unsigned int CG_ShakeCamera(void);
extern void CG_PerturbCamera(void);
extern void AnglesToAxis(const vec_t *angles, void *axis);

void CG_FxRestart(void);
void CG_FxTest(void);
float CG_GetViewFov(void);
static void CG_CalcFov(void);
void CG_FxSetTestPosition(void);
static void CG_OffsetThirdPersonView(void);
static void CG_CalcViewValues(void);
void CG_InitView(void);
qboolean CG_DrawActiveFrame(int serverTime, DemoType demoType, CubemapShot cubemapShot, int cubemapSize, qboolean renderScreen);

static struct {
    qboolean valid;
    vec3_t origin;
    vec3_t angles;
    float fovX;
    float fovY;
    int respawnDeadline;
} deathView;

void CG_ResetDeathView(void)
{
    memset(&deathView, 0, sizeof(deathView));
}

void CG_SetRespawnDeadline(int deadline)
{
    deathView.respawnDeadline = deadline;
}

int CG_GetRespawnRemaining(void)
{
    const playerState_t *ps = &cg->nextSnap->ps;
    if ((ps->pm_type != 6 && ps->pm_type != 7) ||
        (ps->pm_flags & 0x400000) || deathView.respawnDeadline <= 0)
        return -1;
    int remaining = deathView.respawnDeadline - cg->time;
    return remaining > 0 ? remaining : 0;
}

static void CG_ApplyDeathView(void)
{
    const playerState_t *ps = &cg->predictedPlayerState;
    if ((ps->pm_type == 6 || ps->pm_type == 7) && !(ps->pm_flags & 0x400000)) {
        if (deathView.valid) {
            memcpy(cg->refdef.vieworg, deathView.origin, sizeof(vec3_t));
            memcpy(cg->refdefViewAngles, deathView.angles, sizeof(vec3_t));
            AnglesToAxis(cg->refdefViewAngles, &cg->refdef.viewaxis);
            cg->refdef.fov_x = deathView.fovX;
            cg->refdef.fov_y = deathView.fovY;
        }
        return;
    }
    if (ps->pm_type > 1 || (ps->pm_flags & 0x400000)) {
        deathView.valid = 0;
        return;
    }
    memcpy(deathView.origin, cg->refdef.vieworg, sizeof(vec3_t));
    memcpy(deathView.angles, cg->refdefViewAngles, sizeof(vec3_t));
    deathView.fovX = cg->refdef.fov_x;
    deathView.fovY = cg->refdef.fov_y;
    deathView.valid = 1;
}

void CG_FxRestart(void)
{
    Com_Printf((const char *)"FX Restarting so off-line changes are loaded.\n");
    FX_FreeSystem();
    FX_InitSystem(1);
}

void CG_FxTest(void)
{
    char *fxName;
    int fx;

    if (Cmd_Argc() - 1 <= 0) {
        Com_Printf((const char *)"Must supply filename from base path.  Optional restart time.\n");
    }

    {
        fxName = cg->testFxName;
        I_strncpyz(fxName, CG_Argv(1), 0x40);

        fx = FX_RegisterEffect(fxName);
        Com_Printf((const char *)"Spawning Fx %s\n", fxName);
        FX_PlaySimpleEffect(fx, (const vec_t *)cg->testFxPos);
        cg->testFxTime = cg->time;

        if (Cmd_Argc() == 3) {
            double d = atof(CG_Argv(2));
            cg->testFxRespawnTime = (int)(d * 1000.0);
        } else {
            cg->testFxRespawnTime = 0;
        }
    }
}

float CG_GetViewFov(void)
{
    int weapIndex;
    char *weapDef;
    float fov_x;

    {
        weapIndex = BG_GetViewmodelWeaponIndex((void *)&cg->predictedPlayerState);
        weapDef = (char *)BG_GetWeaponDef(weapIndex);

        if (cg->predictedPlayerState.pm_type == 5) {
            fov_x = 90.0f;
        } else {
            fov_x = cg_fov->current.value;

            if (BG_IsAimDownSightWeapon(weapIndex)) {
                float fPosLerp = cg->predictedPlayerState.fWeaponPosFrac;
                float adsFov = ((WeaponDef *)weapDef)->fAdsZoomFov;
                int *peBase = (int *)&((cg_t *)cg)->playerEntity.fLastWeaponPosFrac;

                if (fPosLerp == 1.0f) {
                    fov_x = adsFov;
                } else if (fPosLerp != 0.0f) {
                    float transTime;
                    float normalizedLerp;

                    if (peBase[1] != 0) {
                        transTime = ((WeaponDef *)weapDef)->fAdsZoomInFrac;
                    } else {
                        transTime = ((WeaponDef *)weapDef)->fAdsZoomOutFrac;
                    }

                    normalizedLerp = fPosLerp - (1.0f - transTime);
                    if (normalizedLerp > 0.0f) {
                        normalizedLerp /= transTime;
                        if (normalizedLerp > 0.0f) {
                            fov_x -= (fov_x - adsFov) * normalizedLerp;
                        }
                    }
                }
            }
        }
    }

    if (cg->predictedPlayerState.eFlags & 0x300) {
        fov_x = 55.0f;
    }

    fov_x *= cg_fovScale->current.value;

    {
        float fovMin = (cg_fovMin)->current.value;
        fov_x = (fovMin - fov_x < 0.0f) ? fov_x : fovMin;
    }

    return fov_x;
}

static void CG_CalcFov(void)
{
    float fov;
    double halfAngle;
    float tanVal;
    float fov_x, fov_y;

    fov = CG_GetViewFov();

    halfAngle = (double)fov * 0.017453292519943295 * 0.5;
    tanVal = (float)tan(halfAngle) * 0.75f;

    {
        fov_x = (float)((double)atanf(tanVal * cgs->viewAspect) * 57.29577951308232 * 2.0);
        fov_y = (float)((double)atanf(tanVal) * 57.29577951308232 * 2.0);

        if (CG_PointContents((const vec_t *)cg->refdef.vieworg, -1, 0x20)) {
            float phase = (float)(cg->time) / 1000.0f * 0.4f;
            float wave = sinf((float)((double)phase * 3.141592653589793 * 2.0));
            fov_x += wave;
            fov_y -= wave;
        }

        cg->refdef.fov_x = fov_x;
        cg->refdef.fov_y = fov_y;
        cg->zoomSensitivity = fov_x / cg_fov->current.value;
    }
}

void CG_FxSetTestPosition(void)
{
    float *start = cg->refdef.vieworg;
    float *dir = cg->refdef.viewaxis[0];
    float *result = cg->testFxPos;

    result[0] = start[0] + 100.0f * dir[0];
    result[1] = start[1] + 100.0f * dir[1];
    result[2] = start[2] + 100.0f * dir[2];

    Com_Printf((const char *)"\n\nFX Testing position set to: (%f, %f, %f)\n\n",
               (double)result[0], (double)result[1], (double)result[2]);
}

static void CG_OffsetThirdPersonView(void)
{
    byte *origin;
    byte *viewAngles;
    float focusAngles[3];
    float forward[3], right[3], up[3];
    float focusPoint[3];
    float view[3];
    float trace_fraction;
    byte trace[64];
    float scale;
    float dist2d;
    float dz;
    double angle;

    {
        origin = (byte *)cg->refdef.vieworg;
        viewAngles = (byte *)cg->refdefViewAngles;

        cg->refdef.vieworg[2] += cg->predictedPlayerState.viewHeightCurrent;

        focusAngles[0] = cg->refdefViewAngles[0];
        focusAngles[1] = cg->refdefViewAngles[1];
        focusAngles[2] = cg->refdefViewAngles[2];

        if (cg->predictedPlayerState.pm_type > 5) {

            focusAngles[1] = (float)cg->predictedPlayerState.stats[1];
            cg->refdefViewAngles[1] = (float)cg->predictedPlayerState.stats[1];
        }
    }

    if (focusAngles[0] < 45.0f) {

    } else {
        focusAngles[0] = 45.0f;
    }

    AngleVectors(focusAngles, forward, (void *)0, (void *)0);

    focusPoint[0] = forward[0] * 512.0f + *(float *)(origin + 0);
    focusPoint[1] = forward[1] * 512.0f + *(float *)(origin + 4);
    focusPoint[2] = forward[2] * 512.0f + *(float *)(origin + 8);

    view[0] = *(float *)(origin + 0);
    view[1] = *(float *)(origin + 4);
    view[2] = *(float *)(origin + 8) + 8.0f;

    *(float *)(viewAngles + 0) *= 0.5f;

    *(float *)(viewAngles + 4) -= (cg_thirdPersonAngle)->current.value;

    AngleVectors((float *)viewAngles, forward, right, up);

    scale = (cg_thirdPersonRange)->current.value;

    {
        union {
            float f;
            unsigned int u;
        } conv;
        conv.f = scale;
        conv.u ^= 0x80000000u;
        scale = conv.f;
    }

    view[0] += scale * forward[0];
    view[1] += scale * forward[1];
    view[2] += scale * forward[2];

    CG_TraceCapsule((trace_t *)trace, (const vec_t *)origin, (const vec_t *)&mins, (const vec_t *)&maxs,
                    (const vec_t *)view, ((cg_t *)cg)->predictedPlayerState.clientNum, 0x811);

    trace_fraction = *(float *)trace;

    if (trace_fraction != 1.0f) {

        view[0] = *(float *)(origin + 0) + trace_fraction * (view[0] - *(float *)(origin + 0));
        view[1] = *(float *)(origin + 4) + trace_fraction * (view[1] - *(float *)(origin + 4));
        {
            float newZ = *(float *)(origin + 8) + trace_fraction * (view[2] - *(float *)(origin + 8));

            view[2] = newZ + (1.0f - trace_fraction) * 32.0f;
        }

        CG_TraceCapsule((trace_t *)trace, (const vec_t *)origin, (const vec_t *)&mins, (const vec_t *)&maxs,
                        (const vec_t *)view, ((cg_t *)cg)->predictedPlayerState.clientNum, 0x811);
        trace_fraction = *(float *)trace;

        view[0] = *(float *)(origin + 0) + trace_fraction * (view[0] - *(float *)(origin + 0));
        view[1] = *(float *)(origin + 4) + trace_fraction * (view[1] - *(float *)(origin + 4));
        view[2] = *(float *)(origin + 8) + trace_fraction * (view[2] - *(float *)(origin + 8));
    }

    origin = (byte *)cg->refdef.vieworg;
    *(float *)(origin + 0) = view[0];
    *(float *)(origin + 4) = view[1];
    *(float *)(origin + 8) = view[2];

    {
        float dx = focusPoint[0] - *(float *)(origin + 0);
        float dy = focusPoint[1] - *(float *)(origin + 4);
        dz = focusPoint[2] - *(float *)(origin + 8);

        dist2d = dx * dx + dy * dy;
        dist2d = __builtin_sqrtf(dist2d);

        if (dist2d < 1.0f) {
            dist2d = 1.0;
        }

        angle = atan2((double)dz, (double)dist2d);
        ((cg_t *)cg)->refdefViewAngles[0] = (float)(angle * -57.29577951308232);
    }
}

static void CG_CalcViewValues(void)
{
    playerState_t *ps;
    int framerate;
    int bobX, bobY;
    int viewX, viewY;
    float bobCycle, speed, menuBlur;

    memset(&cg->refdef, 0, sizeof(refdef_t));

    menuBlur = CL_GetMenuBlurRadius();
    {
        float cgBlur = *(float *)((char *)imp_cgDC + 0x28);
        cg->refdef.blurRadius = __builtin_sqrtf(cgBlur * cgBlur + menuBlur * menuBlur);
    }

    if (cg->cubemapShot != 0) {
        int size = cg->cubemapSize;
        cg->refdef.x = 0;
        cg->refdef.y = 0;
        cg->refdef.width = size + 2;
        cg->refdef.height = cg->refdef.width;
        cg->refdef.fov_x = (float)(atan2((double)cg->refdef.width, (double)size) * 114.591559f);
        cg->refdef.fov_y = cg->refdef.fov_x;

        cg->refdef.vieworg[0] = cg->predictedPlayerState.origin[0];
        cg->refdef.vieworg[1] = cg->predictedPlayerState.origin[1];
        cg->refdef.vieworg[2] = cg->predictedPlayerState.origin[2];
        cg->refdef.vieworg[2] = cg->refdef.vieworg[2] + cg->predictedPlayerState.viewHeightCurrent;

        if (cg->cubemapShot > 6)
            return;

        switch (cg->cubemapShot) {
        case 1:
            cg->refdef.viewaxis[0][0] = 0.0f;
            cg->refdef.viewaxis[0][1] = 0.0f;
            cg->refdef.viewaxis[0][2] = 1.0f;
            cg->refdef.viewaxis[1][0] = 0.0f;
            cg->refdef.viewaxis[1][1] = 1.0f;
            cg->refdef.viewaxis[1][2] = 0.0f;
            cg->refdef.viewaxis[2][0] = -1.0f;
            cg->refdef.viewaxis[2][1] = 0.0f;
            cg->refdef.viewaxis[2][2] = 0.0f;
            return;
        case 2:
            cg->refdef.viewaxis[0][0] = 0.0f;
            cg->refdef.viewaxis[0][1] = 0.0f;
            cg->refdef.viewaxis[0][2] = -1.0f;
            cg->refdef.viewaxis[1][0] = 0.0f;
            cg->refdef.viewaxis[1][1] = 1.0f;
            cg->refdef.viewaxis[1][2] = 0.0f;
            cg->refdef.viewaxis[2][0] = 1.0f;
            cg->refdef.viewaxis[2][1] = 0.0f;
            cg->refdef.viewaxis[2][2] = 0.0f;
            return;
        case 3:
            cg->refdef.viewaxis[0][0] = -1.0f;
            cg->refdef.viewaxis[0][1] = 0.0f;
            cg->refdef.viewaxis[0][2] = 0.0f;
            cg->refdef.viewaxis[1][0] = 0.0f;
            cg->refdef.viewaxis[1][1] = -1.0f;
            cg->refdef.viewaxis[1][2] = 0.0f;
            cg->refdef.viewaxis[2][0] = 0.0f;
            cg->refdef.viewaxis[2][1] = 0.0f;
            cg->refdef.viewaxis[2][2] = 1.0f;
            return;
        case 4:
            cg->refdef.viewaxis[0][0] = 1.0f;
            cg->refdef.viewaxis[0][1] = 0.0f;
            cg->refdef.viewaxis[0][2] = 0.0f;
            cg->refdef.viewaxis[1][0] = 0.0f;
            cg->refdef.viewaxis[1][1] = 1.0f;
            cg->refdef.viewaxis[1][2] = 0.0f;
            cg->refdef.viewaxis[2][0] = 0.0f;
            cg->refdef.viewaxis[2][1] = 0.0f;
            cg->refdef.viewaxis[2][2] = 1.0f;
            return;
        case 5:
            cg->refdef.viewaxis[0][0] = 0.0f;
            cg->refdef.viewaxis[0][1] = -1.0f;
            cg->refdef.viewaxis[0][2] = 0.0f;
            cg->refdef.viewaxis[1][0] = 1.0f;
            cg->refdef.viewaxis[1][1] = 0.0f;
            cg->refdef.viewaxis[1][2] = 0.0f;
            cg->refdef.viewaxis[2][0] = 0.0f;
            cg->refdef.viewaxis[2][1] = 0.0f;
            cg->refdef.viewaxis[2][2] = 1.0f;
            return;
        case 6:
            cg->refdef.viewaxis[0][0] = 0.0f;
            cg->refdef.viewaxis[0][1] = 1.0f;
            cg->refdef.viewaxis[0][2] = 0.0f;
            cg->refdef.viewaxis[1][0] = -1.0f;
            cg->refdef.viewaxis[1][1] = 0.0f;
            cg->refdef.viewaxis[1][2] = 0.0f;
            cg->refdef.viewaxis[2][0] = 0.0f;
            cg->refdef.viewaxis[2][1] = 0.0f;
            cg->refdef.viewaxis[2][2] = 1.0f;
            return;
        default:
            return;
        }
    }

    if (cg->nextSnap->ps.pm_type == 5)
        framerate = 100;
    else
        framerate = cg_viewsize->current.integer;

    {
        int screenX = cgs->viewX;
        int screenY = cgs->viewY;
        int screenW = cgs->viewWidth;
        int screenH = cgs->viewHeight;

        bobX = (framerate * screenW) / 100;
        bobX &= ~1;
        bobY = (framerate * screenH) / 100;
        bobY &= ~1;

        viewX = screenX + (screenW - bobX) / 2;
        viewY = screenY + (screenH - bobY) / 2;

        cg->refdef.x = viewX;
        cg->refdef.y = viewY;
        cg->refdef.width = bobX;
        cg->refdef.height = bobY;
    }

    SetScreenScaling(1.0f, 1.0f, viewX, viewY, bobX, bobY);

    if (cg->predictedPlayerState.pm_type == 5) {
        cg->refdef.vieworg[0] = cg->predictedPlayerState.origin[0];
        cg->refdef.vieworg[1] = cg->predictedPlayerState.origin[1];
        cg->refdef.vieworg[2] = cg->predictedPlayerState.origin[2];
        cg->refdefViewAngles[0] = cg->predictedPlayerState.viewangles[0];
        cg->refdefViewAngles[1] = cg->predictedPlayerState.viewangles[1];
        cg->refdefViewAngles[2] = cg->predictedPlayerState.viewangles[2];
        AnglesToAxis(cg->refdefViewAngles, &cg->refdef.viewaxis);
        CG_CalcFov();
        return;
    }

    ps = &cg->predictedPlayerState;

    cg->fBobCycle = (float)BG_GetBobCycle(ps);
    cg->xyspeed = BG_GetSpeed(ps, cg->time);
    bobCycle = cg->fBobCycle;
    speed = cg->xyspeed;

    cg->refdef.vieworg[0] = ps->origin[0];
    cg->refdef.vieworg[1] = ps->origin[1];
    cg->refdef.vieworg[2] = ps->origin[2];

    cg->refdefViewAngles[0] = ps->viewangles[0];
    cg->refdefViewAngles[1] = ps->viewangles[1];
    cg->refdefViewAngles[2] = ps->viewangles[2];

    {
        const dvar_t *errorDecay = cg_errorDecay;
        float decay = errorDecay->current.value;
        if (decay > 0.0f) {
            float t = (decay - (float)(cg->time - cg->predictedErrorTime)) / decay;
            if (t > 0.0f && t < 1.0f) {
                cg->refdef.vieworg[0] += t * cg->predictedError[0];
                cg->refdef.vieworg[1] += t * cg->predictedError[1];
                cg->refdef.vieworg[2] += t * cg->predictedError[2];
            } else {
                cg->predictedErrorTime = 0;
            }
        }
    }

    if (cg->renderingThirdPerson)
        goto do_third_person;

    if (ps->eFlags & 0x300) {
        /* The turret's tag_player is the eye point. ps->origin remains the
         * player's feet, so skipping normal view height without this placed
         * the mounted camera inside the wall underneath the gun. */
        int turretNum = ps->viewlocked_entNum;
        cg->refdef.vieworg[2] += ps->viewHeightCurrent;
        if (turretNum >= 0 && turretNum < 1022) {
            centity_t *turret = &cg_entities[turretNum];
            struct DObj_s *obj = Com_GetClientDObj(turretNum, turret->localClientNum);
            if (obj && turret->nextValid && turret->nextState.eType == 9) {
                const scr_const_t *scr = (const scr_const_t *)imp_scr_const;
                CG_DObjGetWorldTagPos(turret, obj, scr->tag_player, cg->refdef.vieworg);
            }
        }
        goto post_lean;
    }

    {
        viewState_t viewState;
        float viewAnglesResult[3];
        float bobMax = (float)bg_bobMax->current.integer;

        viewState.ps = ps;
        viewState.damageTime = cg->v_dmg_time;
        viewState.time = cg->time;
        viewState.v_dmg_pitch = cg->v_dmg_pitch;
        viewState.v_dmg_roll = cg->v_dmg_roll;
        viewState.xyspeed = speed;
        viewState.frametime = (float)cg->frametime * 0.001f;
        viewState.fLastIdleFactor = cg->playerEntity.fLastIdleFactor;
        viewState.weapIdleTime = &cg->weapIdleTime;

        BG_CalculateViewAngles(&viewState, viewAnglesResult);

        cg->refdefViewAngles[0] += viewAnglesResult[0];
        cg->refdefViewAngles[1] += viewAnglesResult[1];
        cg->refdefViewAngles[2] += viewAnglesResult[2];

        cg->refdef.vieworg[2] += ps->viewHeightCurrent;

        {
            float vbob = BG_GetVerticalBobFactor(ps, bobCycle, speed, bobMax);
            cg->refdef.vieworg[2] += vbob;
        }

        {
            float hbob = BG_GetHorizontalBobFactor(ps, bobCycle, speed, bobMax);
            float forward[3], right[3], up[3];
            AngleVectors(cg->refdefViewAngles, forward, right, up);
            cg->refdef.vieworg[0] += hbob * forward[0];
            cg->refdef.vieworg[1] += hbob * forward[1];
            cg->refdef.vieworg[2] += hbob * forward[2];
        }

        {
            int delta = cg->time - cg->landTime;
            if (delta < 150) {
                cg->refdef.vieworg[2] += (float)delta / 150.0f * cg->landChange;
            } else if (delta < 450) {
                cg->refdef.vieworg[2] +=
                    (((float)delta - 150.0f) / -300.0f + 1.0f) * cg->landChange;
            }
        }

        {
            int elapsed = cg->time - cg->stepTime;
            if (elapsed < 0)
                cg->stepTime = cg->time;
            if (elapsed > 99) {

            } else {
                cg->refdef.vieworg[2] +=
                    (float)(100 - elapsed) * cg->stepChange / -100.0f;
            }
        }

        AddLeanToPosition(cg->refdef.vieworg, cg->refdefViewAngles[1],
                          ps->leanf, 16.0f, 20.0f);

        {
            float minZ = cg->predictedPlayerState.origin[2] + 8.0f;
            if (minZ > cg->refdef.vieworg[2])
                cg->refdef.vieworg[2] = minZ;
        }
    }

    goto post_lean;

do_third_person:
    CG_OffsetThirdPersonView();

post_lean:

    CG_ShakeCamera();

    AnglesToAxis(cg->refdefViewAngles, &cg->refdef.viewaxis);

    cg->viewModelOrigin[0] = cg->refdef.vieworg[0];
    cg->viewModelOrigin[1] = cg->refdef.vieworg[1];
    cg->viewModelOrigin[2] = cg->refdef.vieworg[2];
    cg->viewModelAxis[0][0] = cg->refdef.viewaxis[0][0];
    cg->viewModelAxis[0][1] = cg->refdef.viewaxis[0][1];
    cg->viewModelAxis[0][2] = cg->refdef.viewaxis[0][2];
    cg->viewModelAxis[1][0] = cg->refdef.viewaxis[1][0];
    cg->viewModelAxis[1][1] = cg->refdef.viewaxis[1][1];
    cg->viewModelAxis[1][2] = cg->refdef.viewaxis[1][2];
    cg->viewModelAxis[2][0] = cg->refdef.viewaxis[2][0];
    cg->viewModelAxis[2][1] = cg->refdef.viewaxis[2][1];
    cg->viewModelAxis[2][2] = cg->refdef.viewaxis[2][2];
    cg->viewModelAngles[0] = cg->refdefViewAngles[0];
    cg->viewModelAngles[1] = cg->refdefViewAngles[1];
    cg->viewModelAngles[2] = cg->refdefViewAngles[2];

    {
        unsigned int pmType = (unsigned int)cg->predictedPlayerState.pm_type;

        if ((pmType - 4) <= 1 || cg->renderingThirdPerson ||
            (cg->predictedPlayerState.eFlags & 0x300)) {
            CG_PerturbCamera();
            CG_CalcFov();
            return;
        }
    }

    CG_PerturbCamera();

    CG_CalcFov();
}
void CG_InitView(void)
{
    int renderPlayerState;

    ((cg_t *)cg)->refdef.time = cg->time;
    (*(int *)&((cg_t *)cg)->refdef.zNear) = 0x3f800000;

    if (cg_thirdPerson->current.enabled != 0) {
        renderPlayerState = 1;
    } else if (cg->nextSnap->ps.pm_type > 5) {
        renderPlayerState = 1;
    } else {
        renderPlayerState = 0;
    }

    cg->renderingThirdPerson = renderPlayerState;
    CG_PredictPlayerState();
    CL_ResetSkeletonCache(0);

    CG_UpdateViewWeaponAnim(&cg->predictedPlayerState);
    CG_CalcViewValues();
    CG_ApplyDeathView();
    CL_FX_AdjustCamera((void *)&cg->refdef);
    FX_AdjustTime(cg->time);
}

/* Integrate the weapon's camera impulse as a critically damped spring.
 * Analytic integration keeps the kick/return independent of browser FPS. */
static void CG_IntegrateViewKick(float *angles, float *velocity, float centerSpeed, float dt)
{
    float omega = sqrtf(fmaxf(centerSpeed, 1.0f));
    float decay = expf(-omega * fmaxf(dt, 0.0f));
    int i;
    for (i = 0; i < 3; ++i) {
        float x = angles[i];
        float c = velocity[i] + omega * x;
        angles[i] = (x + c * dt) * decay;
        velocity[i] = (velocity[i] - omega * c * dt) * decay;
    }
}

static void CG_UpdateViewKick(void)
{
    const playerState_t *ps = &cg->predictedPlayerState;
    const WeaponDef *weapon = BG_GetWeaponDef(BG_GetViewmodelWeaponIndex((void *)ps));
    vec3_t aim;
    int i;
    if (ps->pm_type >= 5 || !weapon) {
        memset(cg->kickAVel, 0, sizeof(cg->kickAVel));
        memset(cg->kickAngles, 0, sizeof(cg->kickAngles));
    } else {
        float centerSpeed = ps->fWeaponPosFrac == 1.0f
            ? weapon->fAdsViewKickCenterSpeed : weapon->fHipViewKickCenterSpeed;
        CG_IntegrateViewKick(cg->kickAngles, cg->kickAVel, centerSpeed,
                             (float)cg->frametime * 0.001f);
    }
    for (i = 0; i < 3; ++i)
        aim[i] = cg->kickAngles[i] + cg->offsetAngles[i];
    /* The command carries the same kick used by prediction and server aim. */
    CL_SetUserCmdAimValues(aim);
}

qboolean CG_DrawActiveFrame(int serverTime, DemoType demoType,
                            CubemapShot cubemapShot, int cubemapSize,
                            qboolean renderScreen)
{
    snapshot_t *snap;
    snapshot_t *nextSnap;

    cg->oldTime = cg->time;
    cg->time = serverTime;
    cg->bgs.time = serverTime;
    cg->demoType = demoType;
    cg->cubemapShot = cubemapShot;
    cg->cubemapSize = cubemapSize;
    cg->renderScreen = renderScreen;
    cg->frametime = serverTime - cg->oldTime;

    if (cg->frametime < 0) {
        cg->frametime = 0;
        cg->oldTime = serverTime;
    }

    CG_AddLagometerFrameInfo();
    cg->bgs.frametime = cg->frametime;

    if (cg->infoScreenText[0] != '\0') {
        *(void **)imp_bgs = 0;
        return 0;
    }

    *(void **)imp_bgs = &cg->bgs;

    CG_ProcessSnapshots();

    {
        LegacyHacks *legacyHacks = *(LegacyHacks **)imp_legacyHacks;
        nextSnap = cg->nextSnap;
        if (!cg->renderScreen ||
            nextSnap == 0 ||
            (nextSnap->snapFlags & 2) != 0 ||
            (legacyHacks->cl_serverloadmap[0] != '\0' &&
             legacyHacks->cl_serverloadgametype[0] != '\0')) {
            *(void **)imp_bgs = 0;
            return 0;
        }
        legacyHacks->cl_serverloadwaiting = 0;
    }

    cg->clientFrame++;

    {
        float frac = cg->frameInterpolation;
        snap = cg->snap;
        nextSnap = cg->nextSnap;
        float *vieworg = cg->refdef.vieworg;

        vieworg[0] = snap->ps.origin[0] + (nextSnap->ps.origin[0] - snap->ps.origin[0]) * frac;
        vieworg[1] = snap->ps.origin[1] + (nextSnap->ps.origin[1] - snap->ps.origin[1]) * frac;
        vieworg[2] = snap->ps.origin[2] + (nextSnap->ps.origin[2] - snap->ps.origin[2]) * frac;

        vieworg[2] += (float)cg->nextSnap->ps.viewHeightCurrent;
    }

    CL_SetLodOrigin(&cg->refdef);

    FX_AdjustTime(cg->time - cg->nextSnap->ps.deltaTime);

    {
        snapshot_t *shockSnap = cg->snap;
        int shockIndex = shockSnap->ps.shellshockIndex;
        if (shockIndex != 0) {
            cg->shellshock.parms = &cgs->shellshockParms[shockIndex];
            cg->shellshock.startTime = shockSnap->ps.shellshockTime;
            cg->shellshock.duration = shockSnap->ps.shellshockDuration;
        } else {
            cg->shellshock.parms = &cgs->shellshockParms[0];
            cg->shellshock.startTime = cg->testShock.time;
            cg->shellshock.duration = cg->testShock.duration;
        }
        CG_UpdateShellShock(cg->shellshock.parms,
                            cg->shellshock.startTime,
                            cg->shellshock.duration);
    }

    if (cg_thirdPerson->current.enabled != 0 ||
        cg->nextSnap->ps.pm_type > 5) {
        cg->renderingThirdPerson = 1;
    } else {
        cg->renderingThirdPerson = 0;
    }

    CL_ResetSkeletonCache(0);
    CG_AddPacketEntities();
    R_UpdateEffectsNonBolt();
    CG_AddLocalEntities();
    CG_UpdateViewKick();
    CL_Input();
    CG_PredictPlayerState();

    CG_UpdateViewWeaponAnim(&cg->predictedPlayerState);
    CG_CalcViewValues();
    CG_ApplyDeathView();

    SND_SetListener(cg->nextSnap->ps.clientNum, cg->refdef.vieworg, cg->refdef.viewaxis);

    CG_AddViewWeapon(&cg->predictedPlayerState);
    CL_FX_AdjustCamera(&cg->refdef);

    if (cg->testFxRespawnTime > 0) {
        if (cg->time > cg->testFxRespawnTime + cg->testFxTime) {
            FX_PlaySimpleEffect(FX_RegisterEffect(cg->testFxName), cg->testFxPos);
            cg->testFxTime = cg->time;
        }
    }

    if ((cg->nextSnap->ps.pm_flags & 0xc00000) != 0) {
        struct DObj_s *obj = Com_GetClientDObj(cg->nextSnap->ps.clientNum, 0);
        if (obj != 0) {
            CG_DObjUpdateInfo(obj);
            CG_ProcessClientNoteTracks(cg->nextSnap->ps.clientNum);
        }
        CG_CalcEntityLerpPositions(&cg->predictedPlayerEntity);
        CG_ProcessEntity(&cg->predictedPlayerEntity);
    }

    {
        int viewlockedEnt = cg->predictedPlayerState.viewlocked_entNum;
        if (viewlockedEnt != 0x3ff) {
            CG_ProcessEntity(&cg_entities[viewlockedEnt]);
        }
    }

    {
        int dumpClient = cg_dumpAnims->current.integer;
        if (dumpClient >= 0 && cg_paused->current.integer == 0) {
            struct DObj_s *obj = Com_GetClientDObj(dumpClient, 0);
            if (obj != 0) {
                Com_Printf("client:\n");
                DObjDisplayAnim(obj);
            }
        }
    }

    R_UpdateEffectsBolt();

    {
        int delayedHandle = CL_BeginDelayedDrawing();

        CG_DrawPlayerSprites();
        CG_Draw3dHudElems();
        CG_Draw2D();
        CL_EndDelayedDrawing(delayedHandle);

        if (cg->weaponSelect < 0 || cg->weaponSelect > BG_GetNumWeapons()) {
            int slot;
            Com_Printf("WARNING: Invalid weaponSelect setting %i (out of range 0 - %i)\n",
                       cg->weaponSelect, BG_GetNumWeapons());
            for (slot = 1; slot != 3; slot++) {
                int w = (signed char)((char *)&cg->predictedPlayerState)[0x554 + slot];
                if (w != 0)
                    cg->weaponSelect = w;
            }
            cg->weaponSelect = 0;
        }

        CG_DrawActive();
        CL_IssueDelayedDrawing(delayedHandle);
    }

#if COD2_FEATURE_RUMBLE

    {
        extern void CG_UpdateRumble(void);
        CG_UpdateRumble();
    }
#endif

    *(void **)imp_bgs = 0;
    return 1;
}

const unsigned char colorWhiteFaded[160] = {
    0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x40, 0x3f,
    0xff, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x07, 0x1d, 0x1d, 0x00, 0xab, 0x22, 0x1d, 0x00, 0xf8, 0x22, 0x1d, 0x00, 0x45, 0x23, 0x1d, 0x00,
    0x92, 0x23, 0x1d, 0x00, 0xdb, 0x23, 0x1d, 0x00, 0x10, 0x24, 0x1d, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xa4, 0x43, 0x1d, 0x00, 0x32, 0x44, 0x1d, 0x00, 0x00, 0x44, 0x1d, 0x00, 0x00, 0x44, 0x1d, 0x00,
    0x3f, 0x44, 0x1d, 0x00, 0x92, 0x44, 0x1d, 0x00, 0x9f, 0x44, 0x1d, 0x00, 0x00, 0x00, 0x00, 0x00
};
