#include "common_types.h"
#include "imports.h"

extern float sqrtf(float);

extern const dvar_t *Dvar_RegisterFloat(const char *name, float value, float min, float max, int flags);
extern const dvar_t *Dvar_RegisterBool(const char *name, int value, int flags);
extern int PM_GetEffectiveStance(playerState_t *ps);
extern void PM_AddEvent(playerState_t *ps, int event);
extern int PM_GroundSurfaceType(pml_t *pml);
extern int BG_AnimScriptEvent(playerState_t *ps, int event, int isContinue, int force);
extern float Vec3Normalize(vec_t *v);

#define qtrue 1
#define qfalse 0

static const dvar_t *jump_height;
static const dvar_t *jump_stepSize;
static const dvar_t *jump_slowdownEnable;
static const dvar_t *jump_ladderPushVel;
static const dvar_t *jump_spreadAdd;

void Jump_RegisterDvars(void)
{
    jump_height = Dvar_RegisterFloat("jump_height", 39.0f, 0.0f, 128.0f, 0x1180);
    jump_stepSize = Dvar_RegisterFloat("jump_stepSize", 18.0f, 0.0f, 64.0f, 0x1180);
    jump_slowdownEnable = Dvar_RegisterBool("jump_slowdownEnable", 1, 0x1180);
    jump_ladderPushVel = Dvar_RegisterFloat("jump_ladderPushVel", 128.0f, 0.0f, 1024.0f, 0x1180);
    jump_spreadAdd = Dvar_RegisterFloat("jump_spreadAdd", 64.0f, 0.0f, 512.0f, 0x1180);
}

static inline __attribute__((always_inline)) void Jump_ClearState_core(playerState_t *ps)
{
    ps->pm_flags &= ~0x80000;
    ps->jumpOriginZ = 0.0f;
}

void Jump_ClearState(playerState_t *ps)
{
    Jump_ClearState_core(ps);
}

Bool Jump_GetStepHeight(playerState_t *ps, const vec_t *origin, float *stepSize)
{
    if (!(ps->jumpOriginZ + jump_height->current.value > origin[2])) {
        return qfalse;
    }

    *stepSize = jump_stepSize->current.value;

    if (origin[2] + *stepSize > ps->jumpOriginZ + jump_height->current.value) {
        *stepSize = ps->jumpOriginZ + jump_height->current.value - origin[2];
    }
    return qtrue;
}

Bool Jump_IsPlayerAboveMax(playerState_t *ps)
{
    return ps->origin[2] >= ps->jumpOriginZ + jump_height->current.value;
}

void Jump_ActivateSlowdown(playerState_t *ps)
{
    if (ps->pm_time == 0) {
        ps->pm_flags |= 0x80000;
        ps->pm_time = 1800;
    }
}

void Jump_ApplySlowdown(playerState_t *ps)
{
    float scale;

    if (ps->pm_time > 1800) {
        Jump_ClearState_core(ps);
        scale = 0.65f;
    } else if (ps->pm_time == 0) {
        if (ps->jumpOriginZ + 18.0f > ps->origin[2]) {
            ps->pm_time = 1800;
            scale = 0.65f;
        } else {
            ps->pm_time = 1200;
            scale = 0.5f;
        }
    } else {
        scale = 1.0f;
    }

    if (!jump_slowdownEnable->current.enabled)
        scale = 1.0f;

    ps->velocity[0] *= scale;
    ps->velocity[1] *= scale;
    ps->velocity[2] *= scale;
}

float Jump_ReduceFriction(playerState_t *ps)
{
    if (ps->pm_time > 1800) {
        Jump_ClearState_core(ps);
        return 1.0f;
    }

    if (!jump_slowdownEnable->current.enabled)
        return 1.0f;

    if (ps->pm_time > 1699)
        return 2.5f;

    return (float)ps->pm_time * 1.5f * 0.0005882352706976235f + 1.0f;
}

Bool Jump_Check(pmove_t *pm, pml_t *pml)
{
    playerState_t *ps = pm->ps;
    vec3_t flatForward, pushOffDir;
    float jumpVel, factor, dot, reflectScale, pushVel;

    if (pm->cmd.serverTime - ps->jumpTime <= 499)
        return qfalse;

    if (ps->pm_flags & 0x1000)
        return qfalse;

    if (ps->pm_flags & 0x4)
        return qfalse;

    if (ps->pm_type > 5)
        return qfalse;

    if (PM_GetEffectiveStance(ps) != 0)
        return qfalse;

    if (!(pm->cmd.buttons & 0x400))
        return qfalse;

    if (pm->oldcmd.buttons & 0x400) {
        pm->cmd.buttons &= ~0x400;
        return qfalse;
    }

    jumpVel = jump_height->current.value * 2.0f * (float)ps->gravity;

    if ((ps->pm_flags & 0x80000) && ps->pm_time <= 1800) {
        if (jump_slowdownEnable->current.enabled) {
            if (ps->pm_time > 1699) {
                factor = 2.5f;
            } else {
                factor = (float)ps->pm_time * 1.5f * 0.0005882352706976235f + 1.0f;
            }
        } else {
            factor = 1.0f;
        }
        jumpVel /= factor;
    }

    pml->walking = 0;
    pml->groundPlane = 0;
    pml->almostGroundPlane = 0;

    ps->groundEntityNum = 1023;
    ps->jumpTime = pm->cmd.serverTime;
    ps->jumpOriginZ = ps->origin[2];
    ps->pm_flags |= 0x80000;
    ps->pm_time = 0;

    ps->velocity[2] = sqrtf(jumpVel);

    ps->aimSpreadScale += jump_spreadAdd->current.value;
    if (ps->aimSpreadScale > 255.0f)
        ps->aimSpreadScale = 255.0f;

    if (ps->pm_flags & 0x20) {

        PM_AddEvent(ps, 0x5b);
    } else {
        int surfType = PM_GroundSurfaceType(pml);
        if (surfType != 0) {
            PM_AddEvent(ps, surfType + 0x46);
        }
    }

    if (ps->pm_flags & 0x20) {
        ps->velocity[2] *= 0.75f;

        flatForward[0] = pml->forward[0];
        flatForward[1] = pml->forward[1];
        flatForward[2] = 0.0f;
        Vec3Normalize(flatForward);

        dot = ps->vLadderVec[0] * pml->forward[0] +
              ps->vLadderVec[1] * pml->forward[1] +
              ps->vLadderVec[2] * pml->forward[2];

        if (dot >= 0.0f) {

            pushOffDir[0] = flatForward[0];
            pushOffDir[1] = flatForward[1];
            pushOffDir[2] = flatForward[2];
        } else {

            reflectScale = dot * -2.0f;
            pushOffDir[0] = flatForward[0] + reflectScale * ps->vLadderVec[0];
            pushOffDir[1] = flatForward[1] + reflectScale * ps->vLadderVec[1];
            pushOffDir[2] = flatForward[2] + reflectScale * ps->vLadderVec[2];
            Vec3Normalize(pushOffDir);
        }

        pushVel = jump_ladderPushVel->current.value;
        ps->velocity[0] = pushOffDir[0] * pushVel;
        ps->velocity[1] = pushOffDir[1] * pushVel;

        ps->pm_flags &= ~0x20;
    }

    if (pm->cmd.forwardmove < 0) {
        BG_AnimScriptEvent(ps, 4, 0, 1);
    } else {
        BG_AnimScriptEvent(ps, 3, 0, 1);
    }

    return qtrue;
}

void Jump_ClampVelocity(playerState_t *ps, const vec_t *origin)
{
    float heightAboveStart, maxHeight, maxVel;

    heightAboveStart = ps->origin[2] - origin[2];
    if (heightAboveStart <= 0.0f)
        return;

    maxHeight = ps->jumpOriginZ + jump_height->current.value - ps->origin[2];

    if (maxHeight < 0.1f) {
        ps->velocity[2] = 0.0f;
        return;
    }

    maxVel = sqrtf(maxHeight * 2.0f * (float)ps->gravity);
    if (ps->velocity[2] > maxVel)
        ps->velocity[2] = maxVel;
}
