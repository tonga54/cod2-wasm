#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include "headers/PC/cgame_mp/cg_local.h"

extern float sinf(float x);
extern float cosf(float x);
extern float sqrtf(float x);

static pmove_t cg_pmove[1];
static int cg_numSolidEntities;
static centity_t *cg_solidEntities[256];
static int cg_numTriggerEntities;
static centity_t *cg_triggerEntities[256];

extern int CM_PointContents(const vec_t *point, unsigned int model);
extern int CM_TransformedPointContents(const vec_t *point, unsigned int model, const vec_t *origin, const vec_t *angles);
extern int CM_ContentsOfModel(unsigned int model);
extern int CM_BoxTrace(trace_t *results, const vec_t *start, const vec_t *end, const vec_t *mins, const vec_t *maxs, unsigned int model, int brushmask);
extern unsigned int CM_TempBoxModel(const vec_t *mins, const vec_t *maxs, int capsule);
extern int CM_TransformedBoxTraceExternal(trace_t *results, const vec_t *start, const vec_t *end, const vec_t *mins, const vec_t *maxs, unsigned int model, int brushmask, const vec_t *origin, const vec_t *angles);
extern float CM_RadiusOfModel(unsigned int model);
extern int CL_GetCurrentCmdNumber(void);
extern qboolean CL_GetUserCmd(int cmdNumber, usercmd_t *ucmd);
extern void PM_UpdateViewAngles(playerState_t *ps, float msec, usercmd_t *cmd, int handler);
extern float LerpAngle(float from, float to, float frac);
extern void Pmove(pmove_t *pm);
extern void CG_TransitionPlayerState(playerState_t *ps, playerState_t *ops);
extern void BG_PlayerStateToEntityState(playerState_t *ps, entityState_t *es, int, int);
extern void BG_EvaluateTrajectory(trajectory_t *tr, int atTime, vec_t *result);
extern WeaponDef *BG_GetWeaponDef(int weapon);
extern qboolean CG_GetWeapReticleZoom(float *fZoom);
extern float flrand(float min, float max);
extern float randomf(void);
extern float AngleNormalize360(float angle);
extern qboolean BG_PlayerTouchesItem(playerState_t *ps, centity_t *item, int atTime);
extern qboolean BG_CanItemBeGrabbed(centity_t *item, playerState_t *ps, int);
extern void BG_AddPredictableEventToPlayerstate(int event, int eventParm, playerState_t *ps);
extern void CG_AdjustPositionForMover(const vec_t *in, int moverNum, int fromTime, int toTime, vec_t *out, vec_t *outDeltaAngles);
extern void LargeLocal_LargeLocal(void *ll, int size);
extern void *LargeLocal_GetBuf(void *ll);
extern void ZN10LargeLocalD1Ev(void *ll);
extern void Com_Printf(const char *fmt, ...);

extern const dvar_t **dvar_nopredict;
extern const dvar_t **dvar_synchronous;
extern const dvar_t **dvar_developer;
extern const dvar_t **dvar_autoPickup;
extern const dvar_t **dvar_errorDecay;

static inline int dvar_get_int_safe(const dvar_t **pp)
{
    if (!pp || !*pp)
        return 0;
    return (*pp)->current.integer;
}
static inline float dvar_get_float_safe(const dvar_t **pp)
{
    if (!pp || !*pp)
        return 0.0f;
    return (*pp)->current.value;
}
static inline int dvar_get_bool_safe(const dvar_t **pp)
{
    if (!pp || !*pp)
        return 0;
    return (*pp)->current.enabled;
}

#define REAL_PS_SIZE 0x26a8

void CG_ClearSolidList(void);
int CG_PointContents(const vec_t *point, int passEntityNum, int contentmask);
static void CG_InterpolatePlayerState(qboolean grabAngles);
void CG_PredictPlayerState(void);
static void Trace_CalcBounds(const vec_t *start, const vec_t *mins, const vec_t *maxs, float fraction, const vec_t *end, vec3_t *bounds);
void CG_ClipMoveToEntities(const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int skipNumber, int mask, int capsule, trace_t *tr);
void CG_TraceCapsule(trace_t *result, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int skipNumber, int mask);
void CG_BuildSolidList(void);

void CG_ClearSolidList(void)
{
    cg_numSolidEntities = 0;
    cg_numTriggerEntities = 0;
}

int CG_PointContents(const vec_t *point, int passEntityNum, int contentmask)
{
    int contents;
    int i;
    centity_t **pp;

    contents = CM_PointContents(point, 0);

    pp = (centity_t **)&cg_solidEntities;
    for (i = 0; i < cg_numSolidEntities; i++, pp++) {
        centity_t *cent = *pp;
        entityState_t *ent = &cent->nextState;

        if (ent->number == passEntityNum)
            continue;
        if (ent->solid != 0xffffff)
            continue;
        if (ent->index.brushmodel == 0)
            continue;

        contents |= CM_TransformedPointContents(point, ent->index.brushmodel,
                                                cent->lerpOrigin,
                                                cent->lerpAngles);
    }

    return contents & contentmask;
}

static void CG_InterpolatePlayerState(qboolean grabAngles)
{
    playerState_t *out;
    snapshot_t *prev;
    snapshot_t *next;
    usercmd_t cmd;
    float f;
    int i;

    out = &cg->predictedPlayerState;
    prev = cg->snap;
    next = cg->nextSnap;

    memcpy(out, &next->ps, REAL_PS_SIZE);

    if (grabAngles) {
        int cmdNum = CL_GetCurrentCmdNumber();
        CL_GetUserCmd(cmdNum, &cmd);
        PM_UpdateViewAngles(out, (float)cg->frametime, &cmd, 0);
    }

    if (next->serverTime <= prev->serverTime)
        return;

    f = cg->frameInterpolation;

    {
        int nextBob = next->ps.bobCycle;
        int prevBob = prev->ps.bobCycle;
        int adjusted = nextBob;
        if (nextBob < prevBob)
            adjusted = nextBob + 256;
        out->bobCycle = prevBob + (int)((float)(adjusted - prevBob) * f);
    }

    out->aimSpreadScale = prev->ps.aimSpreadScale +
                          (next->ps.aimSpreadScale - prev->ps.aimSpreadScale) * f;

    if (grabAngles) {

        for (i = 0; i < 3; i++) {
            out->origin[i] = prev->ps.origin[i] + (next->ps.origin[i] - prev->ps.origin[i]) * f;
            out->velocity[i] = prev->ps.velocity[i] + (next->ps.velocity[i] - prev->ps.velocity[i]) * f;
        }
    } else {

        for (i = 0; i < 3; i++) {
            out->origin[i] = prev->ps.origin[i] + (next->ps.origin[i] - prev->ps.origin[i]) * f;
            out->viewangles[i] = LerpAngle(prev->ps.viewangles[i], next->ps.viewangles[i], f);
            out->velocity[i] = prev->ps.velocity[i] + (next->ps.velocity[i] - prev->ps.velocity[i]) * f;
        }
    }

    out->viewHeightCurrent = prev->ps.viewHeightCurrent +
                             (next->ps.viewHeightCurrent - prev->ps.viewHeightCurrent) * f;
    out->leanf = prev->ps.leanf + (next->ps.leanf - prev->ps.leanf) * f;
    out->fWeaponPosFrac = prev->ps.fWeaponPosFrac +
                          (next->ps.fWeaponPosFrac - prev->ps.fWeaponPosFrac) * f;
}

/* CL's clock is monotonic, including its bounded post-stall horizon. Find
 * the first unacknowledged sample without copying the entire 128-command
 * ring on every render frame. Duplicated timestamps remain valid. */
static int CG_FirstPredictionCommand(int oldest, int latest, int commandTime)
{
    int low = oldest, high = latest;
    while (low <= high) {
        int middle = low + (high - low) / 2;
        usercmd_t cmd;
        if (!CL_GetUserCmd(middle, &cmd))
            return oldest;
        if (cmd.serverTime <= commandTime)
            low = middle + 1;
        else
            high = middle - 1;
    }
    return low;
}

void CG_PredictPlayerState(void)
{
    byte ll[32];
    byte *oldPlayerState;
    playerState_t *ps;
    snapshot_t *nextSnap;
    int cmdNum, oldest;
    usercmd_t curCmd;
    qboolean moved = 0;
    WeaponDef *weapDef;
    int cg_time;

    LargeLocal_LargeLocal(ll, REAL_PS_SIZE);
    oldPlayerState = (byte *)LargeLocal_GetBuf(ll);

    if (cg->demoType != 0) {
        CG_InterpolatePlayerState(0);
        goto cleanup;
    }
    nextSnap = cg->nextSnap;
    if (nextSnap->ps.pm_flags & 0x400000) {
        CG_InterpolatePlayerState(0);
        goto cleanup;
    }

    if (dvar_get_bool_safe(dvar_nopredict) || dvar_get_bool_safe(dvar_synchronous)) {
        CG_InterpolatePlayerState(1);
        goto cleanup;
    }

    ps = &cg->predictedPlayerState;

    cg_pmove[0].ps = ps;
    cg_pmove[0].handler = 0;

    {
        int mask;
        if (ps->pm_type < 6)
            mask = 0x2810011;
        else
            mask = 0x810011;

        nextSnap = cg->nextSnap;
        if (nextSnap->ps.pm_type == 4)
            mask &= ~0x02010000;

        cg_pmove[0].tracemask = mask;
    }

    cmdNum = CL_GetCurrentCmdNumber();
    oldest = cmdNum - 127;

    {
        usercmd_t oldestCmd;
        if (!CL_GetUserCmd(oldest, &oldestCmd)) {
            if (dvar_get_int_safe(dvar_developer) != 0)
                Com_Printf("CG_PredictPlayerState: CMD_BACKUP exceeded\n");
            goto cleanup;
        }
    }

    memcpy(oldPlayerState, ps, REAL_PS_SIZE);

    CL_GetUserCmd(cmdNum, &curCmd);

    nextSnap = cg->nextSnap;
    memcpy(ps, &nextSnap->ps, REAL_PS_SIZE);

    cg->physicsTime = nextSnap->serverTime;

    ps->oldVelocity[0] = ((playerState_t *)oldPlayerState)->oldVelocity[0];
    ps->oldVelocity[1] = ((playerState_t *)oldPlayerState)->oldVelocity[1];

    nextSnap = cg->nextSnap;
    if (nextSnap->ps.commandTime >= curCmd.serverTime)
        goto postPredict;

    oldest = CG_FirstPredictionCommand(oldest, cmdNum, ps->commandTime);
    moved = 0;
    for (; oldest <= cmdNum; oldest++) {

        CL_GetUserCmd(oldest, &cg_pmove[0].cmd);

        if (cg_pmove[0].cmd.serverTime <= ps->commandTime)
            continue;

        if (cg_pmove[0].cmd.serverTime > curCmd.serverTime)
            continue;

        if (!CL_GetUserCmd(oldest - 1, &cg_pmove[0].oldcmd))
            continue;

        if (ps->commandTime == ((playerState_t *)oldPlayerState)->commandTime) {
            vec3_t oldOrigin;
            vec3_t deltaAngles;
            vec3_t delta;
            float len;
            int isMatch;

            CG_AdjustPositionForMover(
                ps->origin,
                ps->groundEntityNum,
                cg->oldTime,
                cg->physicsTime,
                oldOrigin, deltaAngles);

            ps->delta_angles[1] += (int)(deltaAngles[1] * (65536.0f / 360.0f)) & 0xffff;

            if (dvar_get_int_safe(dvar_developer) != 0) {

                float *oldOrig = &((playerState_t *)oldPlayerState)->origin[0];
                isMatch = (oldOrig[0] == oldOrigin[0] &&
                           oldOrig[1] == oldOrigin[1] &&
                           oldOrig[2] == oldOrigin[2]);
                if (!isMatch) {
                    Com_Printf("CG_PredictPlayerState: prediction not matched\n");
                }
            }

            delta[0] = ((playerState_t *)oldPlayerState)->origin[0] - oldOrigin[0];
            delta[1] = ((playerState_t *)oldPlayerState)->origin[1] - oldOrigin[1];
            delta[2] = ((playerState_t *)oldPlayerState)->origin[2] - oldOrigin[2];

            len = sqrtf(delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2]);
            if (len > 0.1f) {
                float *predictedError;
                float errorDecayVal;

                if (dvar_get_int_safe(dvar_developer) != 0)
                    Com_Printf("Prediction miss: %f\n", (double)len);

                errorDecayVal = dvar_get_float_safe(dvar_errorDecay);
                predictedError = cg->predictedError;

                if (errorDecayVal == 0.0f) {

                    predictedError[0] = 0.0f;
                    predictedError[1] = 0.0f;
                    predictedError[2] = 0.0f;
                } else {
                    float t;
                    int cg_time_val = cg->time;
                    int errorTime = cg->predictedErrorTime;

                    t = 1.0f - (float)(cg_time_val - errorTime) / errorDecayVal;
                    if (t < 0.0f)
                        t = 0.0f;

                    if (t > 0.0f && dvar_get_int_safe(dvar_developer) != 0)
                        Com_Printf("Double prediction decay: %f\n", (double)t);

                    predictedError[0] *= t;
                    predictedError[1] *= t;
                    predictedError[2] *= t;
                }

                predictedError[0] += delta[0];
                predictedError[1] += delta[1];
                predictedError[2] += delta[2];

                cg->predictedErrorTime = cg->oldTime;
            }
        }

        Pmove(&cg_pmove[0]);

        {
            int pm_type;
            pm_type = ps->pm_type;

            if (pm_type <= 5) {
                int inDeadState = (pm_type == 4);
                if (pm_type <= 1 || inDeadState) {
                    int j;
                    for (j = 0; j < cg_numTriggerEntities; j++) {
                        centity_t *cent = cg_triggerEntities[j];
                        entityState_t *ent = &cent->nextState;

                        if (ent->eType == 3 && !inDeadState) {
                            if (dvar_get_bool_safe(dvar_autoPickup) == 0)
                                continue;

                            cg_time = cg->time;

                            if (!BG_PlayerTouchesItem(ps, cent, cg_time))
                                continue;
                            if (cent->miscTime == cg_time)
                                continue;
                            if (!BG_CanItemBeGrabbed(cent, ps, 1))
                                continue;

                            cent->nextState.eFlags |= 0x20;
                            cent->miscTime = cg_time;
                            BG_AddPredictableEventToPlayerstate(0x90, cent->nextState.index.item, ps);
                            continue;
                        }

                        if (ent->solid == 0xffffff) {
                            unsigned int cmodel = ent->index.brushmodel;
                            if (cmodel != 0) {
                                trace_t trace;
                                CM_BoxTrace(&trace,
                                            ps->origin, ps->origin,
                                            cg_pmove[0].mins, cg_pmove[0].maxs,
                                            cmodel, -1);
                            }
                        }
                    }
                }
            }
        }

        moved = 1;
    }

postPredict:

    if (dvar_get_int_safe(dvar_developer) > 1) {
        Com_Printf("[%i : %i] ", cg_pmove[0].cmd.serverTime, cg->time);
    }

    if (moved) {

        vec3_t deltaAngles;
        CG_AdjustPositionForMover(
            ps->origin,
            ps->groundEntityNum,
            cg->physicsTime,
            cg->time,
            ps->origin, deltaAngles);
    } else {

        if (dvar_get_int_safe(dvar_developer) != 0)
            Com_Printf("not moved\n");
    }

    CG_TransitionPlayerState(ps, (playerState_t *)oldPlayerState);

    if (getenv("PTRACE")) {
        static int lastTraceTime;
        if (cg->time - lastTraceTime >= 250 || cg->time < lastTraceTime) {
            lastTraceTime = cg->time;
            fprintf(stderr, "[prediction] time=%d command=%d snapshot=%d flags=0x%x "
                    "lean=%.3f snapshotLean=%.3f origin=(%.2f,%.2f,%.2f) "
                    "velocity=(%.1f,%.1f,%.1f) error=(%.3f,%.3f,%.3f)\n",
                    cg->time, ps->commandTime, cg->nextSnap->ps.commandTime, ps->pm_flags,
                    ps->leanf, cg->nextSnap->ps.leanf,
                    ps->origin[0], ps->origin[1], ps->origin[2],
                    ps->velocity[0], ps->velocity[1], ps->velocity[2],
                    cg->predictedError[0], cg->predictedError[1], cg->predictedError[2]);
        }
    }

cleanup:
    ZN10LargeLocalD1Ev(ll);

    ps = &cg->predictedPlayerState;

    {
        centity_t *cent = &cg_entities[ps->clientNum];

        cent->lerpOrigin[0] = ps->origin[0];
        cent->lerpOrigin[1] = ps->origin[1];
        cent->lerpOrigin[2] = ps->origin[2];

        BG_EvaluateTrajectory(
            (trajectory_t *)&cent->currentState.apos.trType,
            cg->time,
            cent->lerpAngles);
    }

    weapDef = BG_GetWeaponDef(ps->weapon);
    nextSnap = cg->nextSnap;

    if (nextSnap->ps.pm_flags & 0x800000) {
        float fZoom;
        if (CG_GetWeapReticleZoom(&fZoom)) {
            if (weapDef->adsViewErrorMax != 0.0f) {
                if (cg->adsViewErrorDone == 0) {
                    float size, angle, sinAngle, cosAngle;
                    float *kickYaw = &cg->offsetAngles[0];
                    float *kickPitch = &cg->offsetAngles[1];

                    cg->adsViewErrorDone = 1;

                    size = flrand(weapDef->adsViewErrorMin, weapDef->adsViewErrorMax);
                    angle = randomf() * 2.0f * 3.14159265358979323846f;

                    cosAngle = cosf(angle);
                    sinAngle = sinf(angle);

                    *kickYaw = AngleNormalize360(*kickYaw + sinAngle * size);
                    *kickPitch = AngleNormalize360(*kickPitch + cosAngle * size);
                }
            }
        }
    } else {
        cg->adsViewErrorDone = 0;
    }

    {
        entityState_t *es = &cg->predictedPlayerEntity.nextState;
        entityState_t *cur = &cg->predictedPlayerEntity.currentState;

        es->number = (unsigned short)ps->clientNum;
        BG_PlayerStateToEntityState(ps, es, 0, 0);

        memcpy(cur, es, sizeof(entityState_t));
    }
}

static void Trace_CalcBounds(const vec_t *start, const vec_t *mins, const vec_t *maxs, float fraction, const vec_t *end, vec3_t *bounds)
{
    float hit0, hit1, hit2;

    hit0 = start[0] + (end[0] - start[0]) * fraction;
    hit1 = start[1] + (end[1] - start[1]) * fraction;
    hit2 = start[2] + (end[2] - start[2]) * fraction;

    bounds[0][0] = (start[0] < hit0 ? start[0] : hit0) + mins[0];
    bounds[0][1] = (start[1] < hit1 ? start[1] : hit1) + mins[1];
    bounds[0][2] = (start[2] < hit2 ? start[2] : hit2) + mins[2];

    bounds[1][0] = (start[0] > hit0 ? start[0] : hit0) + maxs[0];
    bounds[1][1] = (start[1] > hit1 ? start[1] : hit1) + maxs[1];
    bounds[1][2] = (start[2] > hit2 ? start[2] : hit2) + maxs[2];
}

void CG_ClipMoveToEntities(const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int skipNumber, int mask, int capsule, trace_t *tr)
{
    int i;
    vec3_t bounds[2];
    trace_t trace;
    vec3_t bmins, bmaxs;
    vec3_t angles;
    unsigned int cmodel;
    centity_t **pp;

    Trace_CalcBounds(start, mins, maxs, tr->fraction, end, bounds);

    pp = (centity_t **)&cg_solidEntities;
    for (i = 0; i < cg_numSolidEntities; i++, pp++) {
        centity_t *cent = *pp;
        entityState_t *ent = &cent->nextState;

        if (ent->number == skipNumber)
            continue;

        if (ent->solid == 0xffffff) {

            cmodel = ent->index.brushmodel;

            if (!(CM_ContentsOfModel(cmodel) & mask))
                continue;

            {
                float radius = CM_RadiusOfModel(cmodel);

                if (cent->lerpOrigin[0] - radius >= bounds[1][0])
                    continue;
                if (cent->lerpOrigin[1] - radius >= bounds[1][1])
                    continue;
                if (cent->lerpOrigin[0] + radius <= bounds[0][0])
                    continue;
                if (cent->lerpOrigin[1] + radius <= bounds[0][1])
                    continue;
                if (cent->lerpOrigin[2] - radius >= bounds[1][2])
                    continue;
                if (cent->lerpOrigin[2] + radius <= bounds[0][2])
                    continue;
            }

            angles[0] = cent->lerpAngles[0];
            angles[1] = cent->lerpAngles[1];
            angles[2] = cent->lerpAngles[2];
        } else {

            int solid = ent->solid;
            int w, h1, h2;
            int contentFlags;

            contentFlags = (ent->eType == 1) ? 0x2000000 : 1;
            if (!(contentFlags & mask))
                continue;

            w = solid & 0xFF;
            h1 = (solid >> 8) & 0xFF;
            h2 = (solid >> 16) & 0xFF;

            bmins[0] = bmins[1] = 1.0f - (float)w;
            bmaxs[0] = bmaxs[1] = (float)w - 1.0f;
            bmins[2] = 1.0f - (float)(h1 - 1);
            bmaxs[2] = (float)(h2 - 32) - 1.0f;

            if (cent->lerpOrigin[0] + bmins[0] >= bounds[1][0])
                continue;
            if (cent->lerpOrigin[1] + bmins[1] >= bounds[1][1])
                continue;
            if (cent->lerpOrigin[0] + bmaxs[0] <= bounds[0][0])
                continue;
            if (cent->lerpOrigin[1] + bmaxs[1] <= bounds[0][1])
                continue;
            if (cent->lerpOrigin[2] + bmins[2] >= bounds[1][2])
                continue;
            if (cent->lerpOrigin[2] + bmaxs[2] <= bounds[0][2])
                continue;

            cmodel = CM_TempBoxModel(bmins, bmaxs, capsule);

            angles[0] = 0.0f;
            angles[1] = 0.0f;
            angles[2] = 0.0f;
        }

        CM_TransformedBoxTraceExternal(&trace, start, end, mins, maxs,
                                       cmodel, mask, cent->lerpOrigin, angles);

        if (trace.fraction < tr->fraction) {

            trace.entityNum = ent->number;
            *tr = trace;

            Trace_CalcBounds(start, mins, maxs, trace.fraction, end, bounds);
        } else if (trace.allsolid) {
            trace.entityNum = ent->number;
            *tr = trace;
        } else if (trace.startsolid) {
            tr->startsolid = 1;
        }

        if (tr->allsolid)
            return;
    }
}

void CG_TraceCapsule(trace_t *result, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int skipNumber, int mask)
{
    CM_BoxTrace(result, start, end, mins, maxs, 0, mask);

    if (result->fraction == 1.0f) {
        result->entityNum = 1023;
    } else {
        result->entityNum = 1022;
    }

    if (result->fraction == 0.0f) {
        return;
    }

    CG_ClipMoveToEntities(start, mins, maxs, end, skipNumber, mask, 1, result);
}

void CG_BuildSolidList(void)
{
    int i;
    snapshot_t *snap;

    cg_numSolidEntities = 0;
    cg_numTriggerEntities = 0;

    snap = cg->nextSnap;

    for (i = 0; i < snap->numEntities; i++) {
        entityState_t *snapEnt = &snap->entities[i];
        int entityNum = snapEnt->number;
        centity_t *cent = &cg_entities[entityNum];
        entityState_t *ent = &cent->nextState;

        if (ent->solid == 0xffffff) {
            if (ent->eFlags & 1)
                continue;
            if (CM_ContentsOfModel(ent->index.brushmodel) == 0)
                continue;
        }

        if (ent->eType == 3) {
            cg_triggerEntities[cg_numTriggerEntities] = cent;
            cg_numTriggerEntities++;
        } else if (ent->solid != 0) {
            ((centity_t **)&cg_solidEntities)[cg_numSolidEntities] = cent;
            cg_numSolidEntities++;
        }
    }
}

const unsigned char h1[256] = {
    0xc9, 0xd5, 0x16, 0x38, 0x41, 0x9e, 0xeb, 0x38, 0x10, 0xa6, 0xe7, 0xb8, 0x77, 0x4a, 0x92, 0xb9,
    0x3a, 0x02, 0x71, 0x39, 0xaf, 0x11, 0x1c, 0x3a, 0xe8, 0x77, 0xc8, 0xb9, 0xab, 0x31, 0x95, 0xba,
    0x01, 0x29, 0x0b, 0x3a, 0x85, 0x2a, 0x02, 0x3b, 0xec, 0xac, 0x23, 0xba, 0x02, 0x11, 0x54, 0xbb,
    0xb0, 0x8d, 0x16, 0x3a, 0x00, 0x7c, 0xa3, 0x3b, 0x28, 0x84, 0x87, 0xb9, 0x4f, 0x68, 0xf1, 0xbb,
    0x1a, 0xb2, 0xfe, 0xb9, 0x17, 0x25, 0x2c, 0x3c, 0x0d, 0x58, 0xf8, 0x3a, 0x81, 0x1b, 0x6f, 0xbc,
    0xb8, 0x59, 0x8d, 0xbb, 0xde, 0x60, 0xa3, 0x3c, 0x8c, 0xc8, 0x07, 0x3c, 0xcd, 0x7f, 0xde, 0xbc,
    0x11, 0x5e, 0x73, 0xbc, 0x9a, 0x36, 0x1a, 0x3d, 0x61, 0xa7, 0xd8, 0x3c, 0x23, 0x0d, 0x63, 0xbd,
    0x0e, 0xb6, 0x50, 0xbd, 0xa0, 0x46, 0xc8, 0x3d, 0xd1, 0x8d, 0x0d, 0x3e, 0xfa, 0x91, 0xeb, 0xbe,
    0xfa, 0x91, 0xeb, 0x3e, 0xd1, 0x8d, 0x0d, 0xbe, 0xa0, 0x46, 0xc8, 0xbd, 0x0e, 0xb6, 0x50, 0x3d,
    0x23, 0x0d, 0x63, 0x3d, 0x61, 0xa7, 0xd8, 0xbc, 0x9a, 0x36, 0x1a, 0xbd, 0x11, 0x5e, 0x73, 0x3c,
    0xcd, 0x7f, 0xde, 0x3c, 0x8c, 0xc8, 0x07, 0xbc, 0xde, 0x60, 0xa3, 0xbc, 0xb8, 0x59, 0x8d, 0x3b,
    0x81, 0x1b, 0x6f, 0x3c, 0x0d, 0x58, 0xf8, 0xba, 0x17, 0x25, 0x2c, 0xbc, 0x1a, 0xb2, 0xfe, 0x39,
    0x4f, 0x68, 0xf1, 0x3b, 0x28, 0x84, 0x87, 0x39, 0x00, 0x7c, 0xa3, 0xbb, 0xb0, 0x8d, 0x16, 0xba,
    0x02, 0x11, 0x54, 0x3b, 0xec, 0xac, 0x23, 0x3a, 0x85, 0x2a, 0x02, 0xbb, 0x01, 0x29, 0x0b, 0xba,
    0xab, 0x31, 0x95, 0x3a, 0xe8, 0x77, 0xc8, 0x39, 0xaf, 0x11, 0x1c, 0xba, 0x3a, 0x02, 0x71, 0xb9,
    0x77, 0x4a, 0x92, 0x39, 0x10, 0xa6, 0xe7, 0x38, 0x41, 0x9e, 0xeb, 0xb8, 0xc9, 0xd5, 0x16, 0xb8
};
