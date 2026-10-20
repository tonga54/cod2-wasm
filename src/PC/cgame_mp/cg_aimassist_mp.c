#include "cod2_feature_config.h"

#if COD2_FEATURE_GAMEPAD

#    include <math.h>

#    include "common_types.h"
#    include "headers/PC/cgame_mp/cg_local.h"
#    include "headers/PC/cgame_mp/cg_aimassist.h"

extern void CG_TraceCapsule(trace_t *result, const vec_t *start, const vec_t *mins, const vec_t *maxs,
                            const vec_t *end, int skipNumber, int mask);
extern float FX_GetServerVisibility(const vec_t *start, const vec_t *end);
extern void CL_Gamepad_SetAimAssist(float slowdown, float yawRate, float pitchRate);

/* Over an enemy the right stick turns a quarter slower, and while either stick
 * moves the view follows 15% of the enemy's motion across it. Nothing turns
 * the view on its own or pulls it onto a target. */
#    define AIM_ASSIST_SLOWDOWN 0.25f
#    define AIM_ASSIST_TRACKING 0.15f
/* Full within the body's half width of its centre line, fading over a further
 * 1.25 degrees, at least 8 units. */
#    define AIM_ASSIST_RADIUS 16.0f
#    define AIM_ASSIST_MARGIN_TAN 0.0218f
#    define AIM_ASSIST_MARGIN_MIN 8.0f
#    define AIM_ASSIST_NEAR 32.0f
#    define AIM_ASSIST_RANGE 3000.0f
/* The followed motion is measured at a fixed height over the feet, so a
 * change of stance is no motion, smoothed over about 0.1 s and held to 90
 * degrees a second, so a respawn or a late snapshot cannot fling the view. */
#    define AIM_ASSIST_TRACK_HEIGHT 40.0f
#    define AIM_ASSIST_SMOOTH_MSEC 100.0f
#    define AIM_ASSIST_MAX_RATE 90.0f
#    define AIM_ASSIST_MAX_GAP_MSEC 200
/* Smoke the server would not see through hides a target here too. */
#    define AIM_ASSIST_MIN_VISIBILITY 0.2f

static struct {
    int target;
    int time;
    float yaw, pitch;
    float yawRate, pitchRate;
} s_aimAssist = { -1 };

static float CG_AimAssistAngle(float degrees)
{
    while (degrees > 180.0f)
        degrees -= 360.0f;
    while (degrees < -180.0f)
        degrees += 360.0f;
    return degrees;
}

static float CG_AimAssistClampRate(float rate)
{
    if (!(rate > -AIM_ASSIST_MAX_RATE))
        return rate == rate ? -AIM_ASSIST_MAX_RATE : 0.0f;
    return rate < AIM_ASSIST_MAX_RATE ? rate : AIM_ASSIST_MAX_RATE;
}

/* A living player of the other team, or anyone else without teams. */
static int CG_AimAssistEnemy(int clientNum, int team)
{
    const centity_t *cent = &cg_entities[clientNum];
    const clientInfo_t *ci = &cg->bgs.clientinfo[clientNum];

    if (!cent->nextValid || cent->nextState.eType != 1 || cent->nextState.clientNum != clientNum)
        return 0;
    /* Hidden (0x20) or dead (0x20000, pm_type above 5). */
    if (cent->nextState.eFlags & 0x20020)
        return 0;
    return ci->infoValid && ci->team != 3 && (!team || ci->team != team);
}

/* The body's centre line from just over the feet to the top of the head, by
 * the crouch (4) and prone (8) flags. */
static void CG_AimAssistBody(const centity_t *cent, float *bottom, float *top)
{
    if (cent->nextState.eFlags & 8) {
        *bottom = 2.0f;
        *top = 18.0f;
    } else if (cent->nextState.eFlags & 4) {
        *bottom = 6.0f;
        *top = 44.0f;
    } else {
        *bottom = 8.0f;
        *top = 66.0f;
    }
}

static void CG_AimAssistClear(void)
{
    s_aimAssist.target = -1;
    s_aimAssist.yawRate = s_aimAssist.pitchRate = 0.0f;
    CL_Gamepad_SetAimAssist(1.0f, 0.0f, 0.0f);
}

void CG_AimAssistFrame(void)
{
    static const vec3_t zero = { 0.0f, 0.0f, 0.0f };
    const float *eye = cg->refdef.vieworg;
    const float *forward = cg->refdef.viewaxis[0], *left = cg->refdef.viewaxis[1], *up = cg->refdef.viewaxis[2];
    float bestScore = 1.0f, bestWeight = 0.0f, dx, dy, dz, yaw, pitch;
    int i, team, best = -1, axis, elapsed;

    if (!cg->nextSnap || cg->nextSnap->ps.clientNum != cg->clientNum || cg->predictedPlayerState.pm_type > 1) {
        CG_AimAssistClear();
        return;
    }
    team = cg->bgs.clientinfo[cg->clientNum].team;
    if (team == 3) {
        CG_AimAssistClear();
        return;
    }

    for (i = 0; i < 64; ++i) {
        const centity_t *cent = &cg_entities[i];
        float base[3], bottom, top, depth, side, height, slope, z, off, outer, score;
        vec3_t point;
        trace_t trace;

        if (i == cg->clientNum || !CG_AimAssistEnemy(i, team))
            continue;
        CG_AimAssistBody(cent, &bottom, &top);
        for (axis = 0; axis < 3; ++axis)
            base[axis] = cent->lerpOrigin[axis] - eye[axis];
        /* The point of the centre line nearest the crosshair's ray. */
        side = base[0] * left[0] + base[1] * left[1] + base[2] * left[2];
        height = base[0] * up[0] + base[1] * up[1] + base[2] * up[2];
        slope = left[2] * left[2] + up[2] * up[2];
        z = slope > 1e-6f ? -(side * left[2] + height * up[2]) / slope : (bottom + top) * 0.5f;
        z = z < bottom ? bottom : z > top ? top : z;
        depth = base[0] * forward[0] + base[1] * forward[1] + (base[2] + z) * forward[2];
        if (depth < AIM_ASSIST_NEAR || depth > AIM_ASSIST_RANGE)
            continue;
        side += z * left[2];
        height += z * up[2];
        off = sqrtf(side * side + height * height);
        outer = depth * AIM_ASSIST_MARGIN_TAN;
        outer = AIM_ASSIST_RADIUS + (outer > AIM_ASSIST_MARGIN_MIN ? outer : AIM_ASSIST_MARGIN_MIN);
        score = off / outer;
        if (score >= bestScore)
            continue;
        for (axis = 0; axis < 3; ++axis)
            point[axis] = cent->lerpOrigin[axis] + (axis == 2 ? z : 0.0f);
        CG_TraceCapsule(&trace, eye, zero, zero, point, cg->clientNum, 0x2000001);
        if (trace.fraction < 1.0f && trace.entityNum != i)
            continue;
        if (FX_GetServerVisibility(eye, point) < AIM_ASSIST_MIN_VISIBILITY)
            continue;
        bestScore = score;
        bestWeight = off <= AIM_ASSIST_RADIUS ? 1.0f : (outer - off) / (outer - AIM_ASSIST_RADIUS);
        best = i;
    }
    if (best < 0) {
        CG_AimAssistClear();
        return;
    }

    dx = cg_entities[best].lerpOrigin[0] - eye[0];
    dy = cg_entities[best].lerpOrigin[1] - eye[1];
    dz = cg_entities[best].lerpOrigin[2] + AIM_ASSIST_TRACK_HEIGHT - eye[2];
    yaw = atan2f(dy, dx) * (180.0f / 3.14159265f);
    pitch = -atan2f(dz, sqrtf(dx * dx + dy * dy)) * (180.0f / 3.14159265f);
    elapsed = cg->time - s_aimAssist.time;
    if (best != s_aimAssist.target || elapsed < 0 || elapsed > AIM_ASSIST_MAX_GAP_MSEC) {
        s_aimAssist.yawRate = s_aimAssist.pitchRate = 0.0f;
    } else if (elapsed > 0) {
        float blend = (float)elapsed / ((float)elapsed + AIM_ASSIST_SMOOTH_MSEC);
        float yawRate = CG_AimAssistClampRate(CG_AimAssistAngle(yaw - s_aimAssist.yaw) * 1000.0f / (float)elapsed);
        float pitchRate = CG_AimAssistClampRate((pitch - s_aimAssist.pitch) * 1000.0f / (float)elapsed);

        s_aimAssist.yawRate += (yawRate - s_aimAssist.yawRate) * blend;
        s_aimAssist.pitchRate += (pitchRate - s_aimAssist.pitchRate) * blend;
    }
    s_aimAssist.target = best;
    s_aimAssist.time = cg->time;
    s_aimAssist.yaw = yaw;
    s_aimAssist.pitch = pitch;
    CL_Gamepad_SetAimAssist(1.0f - AIM_ASSIST_SLOWDOWN * bestWeight,
                            s_aimAssist.yawRate * AIM_ASSIST_TRACKING * bestWeight,
                            s_aimAssist.pitchRate * AIM_ASSIST_TRACKING * bestWeight);
}

#else

typedef int cg_aimassist_mp_translation_unit_not_empty;

#endif
