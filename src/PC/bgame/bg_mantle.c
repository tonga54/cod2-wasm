#include "common_types.h"
#include "imports.h"
#include <string.h>

static inline float fabsf_local(float x)
{
    return __builtin_fabsf(x);
}

static const dvar_t *mantle_enable;
static const dvar_t *mantle_debug;
static const dvar_t *mantle_check_range;
static const dvar_t *mantle_check_radius;
static const dvar_t *mantle_check_angle;
static const dvar_t *mantle_view_yawcap;

__attribute__((used, aligned(4)))
UInt32 s_mantleTrans[24] = {
    0x00000001,
    0x00000008,
    0x42640000,
    0x00000002,
    0x00000008,
    0x424c0000,
    0x00000003,
    0x00000009,
    0x42340000,
    0x00000004,
    0x00000009,
    0x421c0000,
    0x00000005,
    0x00000009,
    0x42040000,
    0x00000006,
    0x0000000a,
    0x41d80000,
    0x00000007,
    0x0000000a,
    0x41a80000,
    0x00000000,
    0x00000000,
    0x00000000,
};
__attribute__((used, aligned(4)))
const char *s_mantleAnimNames[] = {
    "mp_mantle_root",
    "mp_mantle_up_57",
    "mp_mantle_up_51",
    "mp_mantle_up_45",
    "mp_mantle_up_39",
    "mp_mantle_up_33",
    "mp_mantle_up_27",
    "mp_mantle_up_21",
    "mp_mantle_over_high",
    "mp_mantle_over_mid",
    "player_mantle_over_low",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};
static char (*s_mantleAnims)[64];

#define s_mantleTrans ((MantleAnimTransition *)s_mantleTrans)

extern const dvar_t *Dvar_RegisterBool(const char *name, int value, int flags);
extern const dvar_t *Dvar_RegisterFloat(const char *name, float value, float min, float max, int flags);
extern void *Hunk_AllocAlignInternal(int size, int align);
extern void *XAnimCreateAnims(const char *name, int count, void *allocFunc);
extern void XAnimBlend(void *anims, unsigned int animIndex, const char *name, unsigned int children, unsigned int num, unsigned int flags);
extern XAnimParts *XAnimPrecache(const char *name, void *(*allocFunc)(int));
extern void XAnimCreate(void *anims, int index, const char *name);
extern void XAnimGetAbsDelta(void *anims, int animIndex, float *rot, float *delta, float frac);
extern int XAnimGetLengthMsec(void *anims, int animIndex);
extern void Com_Error(int level, const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);
extern float AngleDelta(float a, float b);
extern float AngleNormalize360Accurate(float a);
extern void VectorAngleMultiply(float *trans, float yaw);
extern void BG_AddPredictableEventToPlayerstate(int event, int param, playerState_t *ps);
extern int BG_AnimScriptAnimation(playerState_t *ps, int anim, int moveType, int force);
extern int BG_AnimScriptEvent(playerState_t *ps, int anim, int p3, int p4);
extern void PM_trace(pmove_t *pm, void *trace, float *start, float *mins, float *maxs, float *end, int entityNum, int contentMask);
extern const char *va(const char *fmt, ...);
extern float Vec3Normalize(float *v);
extern float vectoyaw(float *v);
extern double acos(double x);

void Mantle_RegisterDvars(void);
static void *MantleXAnimPrecacheAlloc(int size);
void Mantle_ShutdownAnims(void);
void Mantle_ClearHint(playerState_t *ps);
void Mantle_CapView(playerState_t *ps);
Bool Mantle_IsWeaponInactive(playerState_t *ps);
void Mantle_CreateAnims(MantleAnimAlloc xanimAlloc);
static void __attribute_regparm__(3) Mantle_GetAnimDelta(MantleState *mstate, int time, float *delta);
void Mantle_Move(pmove_t *pm, playerState_t *ps, pml_t *pml);
static Bool __attribute_regparm__(3) Mantle_CheckLedge(pmove_t *pm, pml_t *pml, MantleResults *mr, float height);
void Mantle_Check(pmove_t *pm, pml_t *pml);

void Mantle_RegisterDvars(void)
{
    mantle_enable = Dvar_RegisterBool("mantle_enable", 1, 0x1180);
    mantle_debug = Dvar_RegisterBool("mantle_debug", 0, 0x1180);
    mantle_check_range = Dvar_RegisterFloat("mantle_check_range", 20.0f, 0.0f, 128.0f, 0x1180);
    mantle_check_radius = Dvar_RegisterFloat("mantle_check_radius", 0.1f, 0.0f, 15.0f, 0x1180);
    mantle_check_angle = Dvar_RegisterFloat("mantle_check_angle", 60.0f, 0.0f, 180.0f, 0x1180);
    mantle_view_yawcap = Dvar_RegisterFloat("mantle_view_yawcap", 60.0f, 0.0f, 180.0f, 0x1180);
}

static void *MantleXAnimPrecacheAlloc(int size)
{
    return Hunk_AllocAlignInternal(size, 4);
}

void Mantle_ShutdownAnims(void)
{
    s_mantleAnims = 0;
}

void Mantle_ClearHint(playerState_t *ps)
{
    ps->mantleState.flags &= ~8;
}

void Mantle_CapView(playerState_t *ps)
{
    float yawcap;
    float delta;
    float negcap;

    if (!mantle_enable->current.enabled)
        return;

    delta = AngleDelta(ps->mantleState.yaw, ps->viewangles[1]);

    yawcap = mantle_view_yawcap->current.value;
    negcap = -yawcap;

    if (delta < negcap || delta > yawcap) {

        while (delta < negcap)
            delta += yawcap;
        while (delta > yawcap)
            delta -= yawcap;
    } else {
        return;
    }

    {
        float clampVal;
        if (delta > 0.0f)
            clampVal = negcap;
        else
            clampVal = yawcap;

        {
            int viewDelta = (int)(delta * 182.04444885253906f) & 0xffff;
            ps->delta_angles[1] += viewDelta;
        }

        ps->viewangles[1] = AngleNormalize360Accurate(clampVal + ps->mantleState.yaw);
    }
}

Bool Mantle_IsWeaponInactive(playerState_t *ps)
{
    MantleState *mstate;

    if (!mantle_enable->current.enabled)
        return 0;

    mstate = &ps->mantleState;

    if (!(ps->pm_flags & 4))
        return 0;

    return s_mantleTrans[mstate->transIndex].overAnimIndex != 10;
}

void Mantle_CreateAnims(MantleAnimAlloc xanimAlloc)
{
    int i;
    float rot[4];
    float delta[3];

    if (s_mantleAnims != 0)
        return;

    s_mantleAnims = XAnimCreateAnims("PLAYER_MANTLE", 11, xanimAlloc);
    XAnimBlend(s_mantleAnims, 0, s_mantleAnimNames[0], 1, 10, 0);

    for (i = 1; i < 11; i++) {
        XAnimPrecache(s_mantleAnimNames[i], MantleXAnimPrecacheAlloc);
        XAnimCreate(s_mantleAnims, i, s_mantleAnimNames[i]);
    }

    for (i = 0; i < 7; i++) {
        int upAnimIdx = s_mantleTrans[i].upAnimIndex;
        int overAnimIdx = s_mantleTrans[i].overAnimIndex;

        XAnimGetAbsDelta(s_mantleAnims, upAnimIdx, rot, delta, 1.0f);
        if (fabsf_local(delta[0] - 16.0f) > 1.0f)
            Com_Error(1, "Mantle anim [%s] has X translation %f, should be %f\n", s_mantleAnimNames[upAnimIdx], (double)delta[0], 16.0);
        if (fabsf_local(delta[1]) > 1.0f)
            Com_Error(1, "Mantle anim [%s] has Y translation %f, should be %f\n", s_mantleAnimNames[upAnimIdx], (double)delta[1], 0.0);
        if (fabsf_local(delta[2] - s_mantleTrans[i].height) > 1.0f)
            Com_Error(1, "Mantle anim [%s] has Z translation %f, should be %f\n", s_mantleAnimNames[upAnimIdx], (double)delta[2], (double)s_mantleTrans[i].height);

        XAnimGetAbsDelta(s_mantleAnims, overAnimIdx, rot, delta, 1.0f);
        if (fabsf_local(delta[0] - 31.0f) > 1.0f)
            Com_Error(1, "Mantle anim [%s] has X translation %f, should be %f\n", s_mantleAnimNames[overAnimIdx], (double)delta[0], 31.0);
        if (fabsf_local(delta[1]) > 1.0f)
            Com_Error(1, "Mantle anim [%s] has Y translation %f, should be %f\n", s_mantleAnimNames[overAnimIdx], (double)delta[1], 0.0);
        if (fabsf_local(delta[2] + 18.0f) > 1.0f)
            Com_Error(1, "Mantle anim [%s] has Z translation %f, should be %f\n", s_mantleAnimNames[overAnimIdx], (double)delta[2], -18.0);
    }
}

static void __attribute_regparm__(3) Mantle_GetAnimDelta(MantleState *mstate, int time, float *delta)
{
    float rot[4];
    float trans[3];
    int upLen;
    int overLen = 0;

    upLen = XAnimGetLengthMsec(s_mantleAnims, s_mantleTrans[mstate->transIndex].upAnimIndex);

    if (mstate->flags & 1) {
        overLen = XAnimGetLengthMsec(s_mantleAnims, s_mantleTrans[mstate->transIndex].overAnimIndex);
    }

    if (upLen >= time) {

        float frac = (float)time / (float)upLen;
        XAnimGetAbsDelta(s_mantleAnims, s_mantleTrans[mstate->transIndex].upAnimIndex, rot, delta, frac);
    } else {

        XAnimGetAbsDelta(s_mantleAnims, s_mantleTrans[mstate->transIndex].upAnimIndex, rot, trans, 1.0f);

        {
            float overFrac = (float)(time - upLen) / (float)overLen;
            XAnimGetAbsDelta(s_mantleAnims, s_mantleTrans[mstate->transIndex].overAnimIndex, rot, delta, overFrac);
        }

        delta[0] += trans[0];
        delta[1] += trans[1];
        delta[2] += trans[2];
    }

    VectorAngleMultiply(delta, mstate->yaw);
}

void Mantle_Move(pmove_t *pm, playerState_t *ps, pml_t *pml)
{
    MantleState *mstate;
    int upLen;
    int mantleLength;
    int prevTime;
    int deltaTime;
    float trans[3];
    float prevTrans[3];
    float scale;

    if (!mantle_enable->current.enabled)
        return;

    mstate = &ps->mantleState;

    mstate->flags &= ~8;

    if (mstate->flags & 2) {
        BG_AddPredictableEventToPlayerstate(0x8d, 0, ps);
    }

    {
        int idx = mstate->transIndex;
        upLen = XAnimGetLengthMsec(s_mantleAnims, s_mantleTrans[idx].upAnimIndex);
    }

    {
        int overLen = 0;
        if (mstate->flags & 1) {
            int idx = mstate->transIndex;
            overLen = XAnimGetLengthMsec(s_mantleAnims, s_mantleTrans[idx].overAnimIndex);
        }
        mantleLength = upLen + overLen;
    }

    prevTime = mstate->timer;

    {
        int newTime = prevTime + pml->msec;
        if (newTime > mantleLength)
            newTime = mantleLength;
        mstate->timer = newTime;
    }

    deltaTime = mstate->timer - prevTime;

    Mantle_GetAnimDelta(mstate, prevTime, prevTrans);
    Mantle_GetAnimDelta(mstate, mstate->timer, trans);

    {
        int currentUpLen;
        int moveType;

        currentUpLen = XAnimGetLengthMsec(s_mantleAnims, s_mantleTrans[mstate->transIndex].upAnimIndex);
        if (mstate->flags & 1) {
            XAnimGetLengthMsec(s_mantleAnims, s_mantleTrans[mstate->transIndex].overAnimIndex);
        }

        if (currentUpLen < mstate->timer) {
            moveType = s_mantleTrans[mstate->transIndex].overAnimIndex + 20;
        } else {
            moveType = s_mantleTrans[mstate->transIndex].upAnimIndex + 20;
        }

        BG_AnimScriptAnimation(ps, 3, moveType, 1);
    }

    trans[0] -= prevTrans[0];
    trans[1] -= prevTrans[1];
    trans[2] -= prevTrans[2];

    ps->origin[0] += trans[0];
    ps->origin[1] += trans[1];
    ps->origin[2] += trans[2];

    scale = 1.0f / ((float)deltaTime * 0.001f);
    ps->velocity[0] = trans[0] * scale;
    ps->velocity[1] = trans[1] * scale;
    ps->velocity[2] = trans[2] * scale;

    if (mantleLength == mstate->timer) {
        ps->pm_flags &= ~4;
        pm->mantleStarted = 0;

        if (mstate->flags & 1) {
            BG_AnimScriptEvent(ps, 3, 0, 1);
        }

        if (mstate->flags & 4) {
            BG_AddPredictableEventToPlayerstate(0x8c, 0, ps);
            ps->eFlags &= ~0x4000;
        }
    }
}

static Bool __attribute_regparm__(3) Mantle_CheckLedge(pmove_t *pm, pml_t *pml, MantleResults *mr, float height)
{
    trace_t tr;
    trace_t wallTr;
    playerState_t *ps = pm->ps;
    float playerRadius = ps->maxs[0];
    float mins[3], maxs[3];
    float start[3], end[3];

    {
        const char *msg = va("Checking for ledge at %f units", (double)height);
        if (mantle_debug->current.enabled)
            Com_Printf("%s\n", msg);
    }

    mins[0] = -playerRadius;
    mins[1] = -playerRadius;
    mins[2] = 0.0f;
    maxs[0] = playerRadius;
    maxs[1] = playerRadius;
    maxs[2] = playerRadius * 2.0f;

    start[0] = mr->startPos[0];
    start[1] = mr->startPos[1];
    start[2] = mr->startPos[2];

    start[2] += height;

    end[0] = mr->dir[0] * 16.0f + start[0];
    end[1] = mr->dir[1] * 16.0f + start[1];
    end[2] = mr->dir[2] * 16.0f + start[2];

    PM_trace(pm, &tr, start, mins, maxs, end, ps->clientNum, pm->tracemask);

    if (tr.startsolid || tr.fraction < 1.0f) {

        if (mantle_debug->current.enabled)
            Com_Printf("%s\n", "Mantle Failed: Can't reach ledge from below");
        return 0;
    }

    start[0] = end[0];
    start[1] = end[1];
    start[2] = end[2];
    end[2] = mr->startPos[2] + 18.0f;

    PM_trace(pm, &tr, start, mins, maxs, end, ps->clientNum, pm->tracemask);

    if (tr.startsolid || tr.fraction == 1.0f) {

        if (mantle_debug->current.enabled)
            Com_Printf("%s\n", "Mantle Failed: Can't find ledge");
        return 0;
    }

    if (tr.normal[2] < 0.7f) {
        if (mantle_debug->current.enabled)
            Com_Printf("%s\n", "Mantle Failed: Surface too steep");
        return 0;
    }

    mr->ledgePos[0] = end[0];
    mr->ledgePos[1] = end[1];
    mr->ledgePos[2] = start[2] + (end[2] - start[2]) * tr.fraction;

    maxs[2] = 50.0f;

    {
        PM_trace(pm, &tr, mr->ledgePos, mins, maxs, mr->ledgePos, ps->clientNum, pm->tracemask);
        if (tr.startsolid) {
            if (mantle_debug->current.enabled)
                Com_Printf("%s\n", "Mantle Failed: No room on ledge");
            return 0;
        }
    }

    ps->mantleState.flags |= 8;
    mr->flags |= 8;

    if (mantle_debug->current.enabled)
        Com_Printf("%s\n", "Mantle: Found ledge");

    if (!(pm->cmd.buttons & 0x400))
        return 1;

    if (mr->flags & 1) {

        float wallMins[3], wallMaxs[3];
        float wallStart[3], wallEnd[3];

        wallMins[0] = ps->mins[0];
        wallMins[1] = ps->mins[1];
        wallMins[2] = ps->mins[2];
        wallMaxs[0] = ps->maxs[0];
        wallMaxs[1] = ps->maxs[1];
        wallMaxs[2] = 50.0f;

        wallStart[0] = mr->ledgePos[0];
        wallStart[1] = mr->ledgePos[1];
        wallStart[2] = mr->ledgePos[2];

        wallEnd[0] = mr->dir[0] * 31.0f + wallStart[0];
        wallEnd[1] = mr->dir[1] * 31.0f + wallStart[1];
        wallEnd[2] = mr->dir[2] * 31.0f + wallStart[2] - 18.0f;

        PM_trace(pm, &wallTr, wallStart, wallMins, wallMaxs, wallEnd, ps->clientNum, pm->tracemask);

        if (!wallTr.startsolid && wallTr.fraction >= 1.0f) {

            mr->endPos[0] = wallEnd[0];
            mr->endPos[1] = wallEnd[1];
            mr->endPos[2] = wallStart[2] + (wallEnd[2] - wallStart[2]) * wallTr.fraction;
        } else {

            mr->flags &= ~1;
            mr->endPos[0] = mr->ledgePos[0];
            mr->endPos[1] = mr->ledgePos[1];
            mr->endPos[2] = mr->ledgePos[2];
        }
    } else {

        mr->endPos[0] = mr->ledgePos[0];
        mr->endPos[1] = mr->ledgePos[1];
        mr->endPos[2] = mr->ledgePos[2];
    }

    if (!(ps->eFlags & 4)) {

        float *pMins = ps->mins;
        float *pMaxs = ps->maxs;

        PM_trace(pm, &tr, mr->ledgePos, pMins, pMaxs, mr->ledgePos, ps->clientNum, pm->tracemask);
        if (tr.startsolid)
            mr->flags |= 2;

        PM_trace(pm, &tr, mr->endPos, pMins, pMaxs, mr->endPos, ps->clientNum, pm->tracemask);
        if (!tr.startsolid)
            mr->flags |= 4;
    }

    {
        MantleState *mantleState = &ps->mantleState;
        float heightDiff;
        int bestTrans = 0;
        float bestDist;
        int mantleTime;
        float animDelta[3];

        mantleState->yaw = vectoyaw(mr->dir);
        mantleState->timer = 0;

        heightDiff = mr->ledgePos[2] - mr->startPos[2];
        bestDist = fabsf_local(s_mantleTrans[0].height - heightDiff);
        {
            int j;
            for (j = 1; j < 7; j++) {
                float dist = fabsf_local(s_mantleTrans[j].height - heightDiff);
                if (dist < bestDist) {
                    bestTrans = j;
                    bestDist = dist;
                }
            }
        }

        mantleState->transIndex = bestTrans;
        mantleState->flags = mr->flags;

        {
            int idx = bestTrans;
            int upAnimLen = XAnimGetLengthMsec(s_mantleAnims, s_mantleTrans[idx].upAnimIndex);
            int overAnimLen = 0;
            if (mantleState->flags & 1) {
                overAnimLen = XAnimGetLengthMsec(s_mantleAnims, s_mantleTrans[idx].overAnimIndex);
            }
            mantleTime = upAnimLen + overAnimLen;
        }

        Mantle_GetAnimDelta(mantleState, mantleTime, animDelta);
        {
            ps->origin[0] = mr->endPos[0] - animDelta[0];
            ps->origin[1] = mr->endPos[1] - animDelta[1];
            ps->origin[2] = mr->endPos[2] - animDelta[2];
        }

        ps->pm_flags |= 4;
        ps->eFlags |= 0x4000;

        pm->mantleEndPos[0] = mr->endPos[0];
        pm->mantleEndPos[1] = mr->endPos[1];
        pm->mantleEndPos[2] = mr->endPos[2];
        pm->mantleDuration = mantleTime;
        pm->mantleStarted = 1;
    }

    return 1;
}

void Mantle_Check(pmove_t *pm, pml_t *pml)
{
    playerState_t *ps;
    float playerRadius;
    float mins[3], maxs[3];
    float start[3], end[3];
    float traceDir[3];
    float mantleDir[3];
    trace_t trace;
    float len;
    MantleResults mresults;
    trace_t *tr;
    MantleResults *results;

    if (mantle_debug->current.enabled)
        Com_Printf("%s\n", "Mantle_Check");

    if (!mantle_enable->current.enabled) {
        if (mantle_debug->current.enabled)
            Com_Printf("%s\n", "Mantle disabled");
        return;
    }

    ps = pm->ps;

    ps->mantleState.flags &= ~8;

    if (ps->pm_type > 5) {
        if (mantle_debug->current.enabled)
            Com_Printf("%s\n", "Mantle Failed: Bad movement type");
        return;
    }

    if (ps->pm_flags & 4) {
        if (mantle_debug->current.enabled)
            Com_Printf("%s\n", "Mantle Failed: Already mantling");
        return;
    }

    if (ps->eFlags & 0xc) {
        if (mantle_debug->current.enabled)
            Com_Printf("%s\n", "Mantle Failed: Prone or going prone");
        return;
    }

    if (ps->weaponstate >= 0x11 && ps->weaponstate <= 0x16) {
        if (mantle_debug->current.enabled)
            Com_Printf("%s\n", "Mantle Failed: Bad weapon state");
        return;
    }

    {
        float checkRadius;
        float scale;
        float checkRange;

        playerRadius = ps->maxs[0];
        checkRadius = mantle_check_radius->current.value;

        mins[0] = -checkRadius;
        mins[1] = -checkRadius;
        mins[2] = ps->mins[2];

        maxs[0] = checkRadius;
        maxs[1] = checkRadius;
        maxs[2] = ps->maxs[2];

        scale = playerRadius - checkRadius;
        checkRange = scale + mantle_check_range->current.value;

        traceDir[0] = pml->forward[0];
        traceDir[1] = pml->forward[1];
        traceDir[2] = 0.0f;

        Vec3Normalize(traceDir);

        start[0] = ps->origin[0] + traceDir[0] * (-scale);
        start[1] = ps->origin[1] + traceDir[1] * (-scale);
        start[2] = ps->origin[2] + traceDir[2] * (-scale);

        end[0] = ps->origin[0] + traceDir[0] * checkRange;
        end[1] = ps->origin[1] + traceDir[1] * checkRange;
        end[2] = ps->origin[2] + traceDir[2] * checkRange;
    }

    tr = &trace;
    results = &mresults;

    PM_trace(pm, tr, start, mins, maxs, end, ps->clientNum, 0x1000000);

    if (tr->allsolid || tr->startsolid) {
        if (mantle_debug->current.enabled)
            Com_Printf("%s\n", "Mantle Failed: Wall trace started in solid");
        return;
    }

    if (tr->fraction == 1.0f) {
        if (mantle_debug->current.enabled)
            Com_Printf("%s\n", "Mantle Failed: Trace went full distance");
        return;
    }

    if (!(tr->surfaceFlags & 0x6000000)) {
        if (mantle_debug->current.enabled)
            Com_Printf("%s\n", "Mantle Failed: Surface not mantleable");
        return;
    }

    mantleDir[0] = -tr->normal[0];
    mantleDir[1] = -tr->normal[1];
    mantleDir[2] = 0.0f;

    len = Vec3Normalize(mantleDir);
    if (len < 0.0001f) {
        if (mantle_debug->current.enabled)
            Com_Printf("%s\n", "Mantle Failed: Bad wall normal");
        return;
    }

    {
        float dot = traceDir[0] * mantleDir[0] + traceDir[1] * mantleDir[1] + traceDir[2] * mantleDir[2];
        double angle = acos((double)dot) * 57.29577951308232;
        if (angle > (double)mantle_check_angle->current.value) {
            if (mantle_debug->current.enabled)
                Com_Printf("%s\n", "Mantle Failed: Bad approach angle");
            return;
        }
    }

    memset(results, 0, sizeof(*results));

    results->dir[0] = mantleDir[0];
    results->dir[1] = mantleDir[1];
    results->dir[2] = mantleDir[2];

    results->startPos[0] = ps->origin[0];
    results->startPos[1] = ps->origin[1];
    results->startPos[2] = ps->origin[2];

    if (tr->surfaceFlags & 0x4000000) {
        results->flags |= 1;
    }

    if (Mantle_CheckLedge(pm, pml, results, 60.0f))
        return;
    if (Mantle_CheckLedge(pm, pml, results, 40.0f))
        return;
    Mantle_CheckLedge(pm, pml, results, 20.0f);
}
