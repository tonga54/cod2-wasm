#include "common_types.h"
#include "cod2_sprint.h"
#include "imports.h"
#include "bytematch.h"
#include <math.h>

extern void CG_PointContents();
extern void CG_TraceCapsule();
extern void G_PlayerEvent();
extern void G_TraceCapsule();
extern void SV_PointContents();
extern pmoveHandler_t pmoveHandlers[2];
extern viewLerpWaypoint_t viewLerp_CrouchProne[13];
extern viewLerpWaypoint_t viewLerp_CrouchStand[10];
extern viewLerpWaypoint_t viewLerp_ProneCrouch[8];
extern viewLerpWaypoint_t viewLerp_StandCrouch[10];

#define BG_PRONE_TURNED_ABI COD2_REGPARM(2) COD2_SSEREGPARM
#define PM_ACCELERATE_ABI COD2_REGPARM(3) COD2_SSEREGPARM
#define PM_REGPARM2_ABI COD2_REGPARM(2) BM_NOINLINE

extern float AngleDelta(float angle1, float angle2);
extern float AngleNormalize360Accurate(float angle);
extern float AngleNormalize180Accurate(float angle);
extern float vectoyaw(float *v);
extern const dvar_t *bg_prone_yawcap;
extern const dvar_t *bg_ladder_yawcap;
extern qboolean BG_CheckProne(int passEntityNum, const vec_t *vPos, const float fSize, const float fHeight, const float fYaw, float *pfTorsoHeight, float *pfTorsoPitch, float *pfWaistPitch, const qboolean bAlreadyProne, const qboolean bOnGround, vec_t *vGroundNormal, int handler, proneCheckType_t proneCheckType, float prone_feet_dist);
extern float Vec3Normalize(vec_t *v);
extern float Vec2Normalize(vec_t *v);
extern void Com_Printf(const char *fmt, ...);

extern const dvar_t *friction;
extern const dvar_t *stopspeed;
extern const dvar_t *inertiaMax;
extern const dvar_t *inertiaAngle;
extern const dvar_t *inertiaDebug;
extern const dvar_t *player_view_pitch_up;
extern const dvar_t *player_view_pitch_down;
extern const dvar_t *player_spectateSpeedScale;
extern const dvar_t *player_footstepsThreshhold;
static const vec3_t CorrectSolidDeltas[26] = {

    { 0.0f, 0.0f, 1.0f },
    { -1.0f, 0.0f, 1.0f },
    { 0.0f, -1.0f, 1.0f },
    { 1.0f, 0.0f, 1.0f },
    { 0.0f, 1.0f, 1.0f },
    { -1.0f, 0.0f, 0.0f },
    { 0.0f, -1.0f, 0.0f },
    { 1.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, -1.0f },
    { -1.0f, 0.0f, -1.0f },
    { 0.0f, -1.0f, -1.0f },
    { 1.0f, 0.0f, -1.0f },
    { 0.0f, 1.0f, -1.0f },
    { -1.0f, -1.0f, 1.0f },
    { 1.0f, -1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f },
    { -1.0f, 1.0f, 1.0f },
    { -1.0f, -1.0f, 0.0f },
    { 1.0f, -1.0f, 0.0f },
    { 1.0f, 1.0f, 0.0f },
    { -1.0f, 1.0f, 0.0f },
    { -1.0f, -1.0f, -1.0f },
    { 1.0f, -1.0f, -1.0f },
    { 1.0f, 1.0f, -1.0f },
    { -1.0f, 1.0f, -1.0f },
};

void PM_trace(pmove_t *pm, trace_t *results, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int passEntityNum, int contentMask);
void PM_AddTouchEnt(pmove_t *pm, int entityNum);
void PM_AddEvent(playerState_t *ps, int newEvent);
int PM_GetEffectiveStance(playerState_t *ps);
int PM_GroundSurfaceType(pml_t *pml);
int PM_GetViewHeightLerpTime(const playerState_t *ps, int iTarget, qboolean bDown);
void PM_SetProneMovementOverride(playerState_t *ps);
float BG_GetSpeed(const playerState_t *ps, int time);
static qboolean BG_PRONE_TURNED_ABI BG_CheckProneTurned(byte *ps_bytes, int handler, float newProneYaw);
qboolean PM_ShouldMakeFootsteps(pmove_t *pm);
void PM_ClipVelocity(const vec_t *in, const vec_t *normal, vec_t *out);
static void PM_ACCELERATE_ABI PM_Accelerate(playerState_t *ps, pml_t *pml, const vec_t *wishdir, float wishspeed, float accel);
static void PM_REGPARM2_ABI PM_Friction(playerState_t *ps, pml_t *pml);
void PM_UpdateLean(playerState_t *ps, float msec, usercmd_t *cmd, void (*capsuleTrace)());
void PM_UpdateViewAngles(playerState_t *ps, float msec, usercmd_t *cmd, int handler);
void PM_UpdatePronePitch(pmove_t *pm, pml_t *pml);
void PM_playerTrace(pmove_t *pm, trace_t *results, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int passEntityNum, int contentMask);
static void PM_REGPARM2_ABI PM_SetMovementDir(pmove_t *pm, pml_t *pml);
static void PM_REGPARM2_ABI PM_AirMove(pmove_t *pm, pml_t *pml);
static void PM_WalkMove(pmove_t *pm, pml_t *pml);
static void PM_NoclipMove(pmove_t *pm, pml_t *pml);
static void PM_REGPARM2_ABI PM_LadderMove(pmove_t *pm, pml_t *pml);
static void PM_REGPARM2_ABI PM_ViewHeightAdjust(pmove_t *pm, pml_t *pml);
static void PM_REGPARM2_ABI PM_CheckDuck(pmove_t *pm, pml_t *pml);
void PM_FootstepEvent(pmove_t *pm, pml_t *pml, int iOldBobCycle, int iNewBobCycle, qboolean bFootStep);
static void PM_REGPARM2_ABI PM_Footsteps(pmove_t *pm, pml_t *pml);
static void PM_REGPARM2_ABI PM_GroundTrace(pmove_t *pm, pml_t *pml);
void Pmove(pmove_t *pm);

extern void PM_StepSlideMove(pmove_t *pm, pml_t *pml, qboolean gravity);
extern void PM_AdjustAimSpreadScale(pmove_t *pm, pml_t *pml);
extern void PM_UpdateAimDownSightFlag(pmove_t *pm, pml_t *pml);
extern void PM_UpdateAimDownSightLerp(pmove_t *pm, pml_t *pml);
extern void PM_Weapon(pmove_t *pm, pml_t *pml);
extern void PM_ResetWeaponState(playerState_t *ps);
extern void BG_AnimUpdatePlayerStateConditions(pmove_t *pmove);
extern int BG_AnimScriptEvent(playerState_t *ps, int event, int isContinue, int force);
extern int BG_AnimScriptAnimation(playerState_t *ps, aistateEnum_t state, scriptAnimMoveTypes_t movetype, qboolean isContinue);
extern Bool Jump_Check(pmove_t *pm, pml_t *pml);
extern void Jump_ClearState(playerState_t *ps);
extern float Jump_ReduceFriction(playerState_t *ps);
extern void Mantle_CapView(playerState_t *ps);
extern void Mantle_Check(pmove_t *pm, pml_t *pml);
extern void Mantle_Move(pmove_t *pm, playerState_t *ps, pml_t *pml);
extern void AngleVectors(const vec_t *angles, vec_t *forward, vec_t *right, vec_t *up);
extern void Sys_SnapVector(vec_t *v);

extern void Jump_ActivateSlowdown(playerState_t *ps);
extern void PM_ExitAimDownSight(playerState_t *ps);
extern int BG_PlayAnim(playerState_t *ps, int animNum, int bodyPart, int forceDuration, qboolean setTimer, qboolean isContinue, qboolean force);
extern float PitchForYawOnNormal(float fYaw, const vec_t *normal);
extern const dvar_t *bg_fallDamageMinHeight;
extern const dvar_t *bg_fallDamageMaxHeight;

void PM_trace(pmove_t *pm, trace_t *results, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int passEntityNum, int contentMask)
{

    unsigned char handlerByte = pm->handler;
    pmove_trace handler = pmoveHandlers[handlerByte].trace;
    handler(results, start, mins, maxs, end, passEntityNum, contentMask);
}

void PM_AddTouchEnt(pmove_t *pm, int entityNum)
{
    int numtouch;
    int i;

    if (entityNum == 0x3fe)
        return;

    numtouch = pm->numtouch;

    if (numtouch == 0x20)
        return;

    for (i = 0; i < numtouch; i++) {
        if (pm->touchents[i] == entityNum)
            return;
    }

    pm->touchents[numtouch] = entityNum;
    numtouch += 1;
    pm->numtouch = numtouch;
}

void BG_AddPredictableEventToPlayerstate(int newEvent, int eventParm, playerState_t *ps);

void PM_AddEvent(playerState_t *ps, int newEvent)
{
    BG_AddPredictableEventToPlayerstate(newEvent, 0, ps);
}

int PM_GetEffectiveStance(playerState_t *ps)
{
    int val = ps->viewHeightTarget;

    if (val == 0x28)
        return 2;
    return val == 0xb;
}

int PM_GroundSurfaceType(pml_t *pml)
{

    unsigned int val = pml->groundTrace.surfaceFlags;

    if (val & 0x2000)
        return 0;
    return (val & 0x1f00000) >> 20;
}

int PM_GetViewHeightLerpTime(const playerState_t *ps, int iTarget, qboolean bDown)
{

    if (iTarget == 0xb)
        return 0x190;

    if (iTarget == 0x28) {
        if (bDown)
            return 0xc8;
        return 0x190;
    }

    return 0xc8;
}

void PM_SetProneMovementOverride(playerState_t *ps)
{
    int flags = ps->pm_flags;
    if (flags & 1)
        ps->pm_flags = flags | 0x800;
}

float BG_GetSpeed(const playerState_t *ps, int time)
{

    if (ps->pm_flags & 0x20) {

        int elapsed = time - ps->jumpTime;
        if (elapsed > 0x1f3)
            return ps->velocity[2];
        return 0.0f;
    }

    {
        float vx = ps->velocity[0];
        float vy = ps->velocity[1];
        return sqrtf(vx * vx + vy * vy);
    }
}

static qboolean BG_CheckProneTurned_impl(byte *ps_bytes, int handler, float newProneYaw)
{
    playerState_t *ps = (playerState_t *)ps_bytes;
    float oldProneYaw = ps->viewangles[1];
    float delta = AngleDelta(newProneYaw, oldProneYaw);
    float absDelta = (float)fabs(delta);
    float t = absDelta / 240.0f;
    float scale = 1.0f - t;
    float adjustedYaw = newProneYaw - (delta * scale);
    float normalizedYaw = AngleNormalize360Accurate(adjustedYaw);
    float proneFeetDist = t * 45.0f + scale * 66.0f;

    return BG_CheckProne(
        ps->clientNum,
        ps->origin,
        ps->maxs[0],
        30.0f,
        normalizedYaw,
        &ps->fTorsoHeight,
        &ps->fTorsoPitch,
        &ps->fWaistPitch,
        1,
        ps->groundEntityNum != 0x3ff ? 1 : 0,
        NULL,
        (unsigned char)handler,
        0,
        proneFeetDist
    );
}

static qboolean BG_PRONE_TURNED_ABI BG_CheckProneTurned(byte *ps_bytes, int handler, float newProneYaw)
{
    return BG_CheckProneTurned_impl(ps_bytes, handler, newProneYaw);
}

qboolean PM_ShouldMakeFootsteps(pmove_t *pm)
{
    playerState_t *ps = pm->ps;
    int flags, hasSprintFlag, stance;

    flags = ps->pm_flags;
    hasSprintFlag = flags & 0x100;

    stance = ps->viewHeightTarget;
    if (stance == 0x28)
        return 0;
    if (stance == 0xb)
        return 0;

    if (hasSprintFlag)
        return 0;

    {
        float threshold = (*(const dvar_t **)imp_player_footstepsThreshhold)->current.value;
        return pm->xyspeed >= threshold;
    }
}

void PM_ClipVelocity(const vec_t *in, const vec_t *normal, vec_t *out)
{
    float dot, scale, overbounce;

    dot = in[0] * normal[0] + in[1] * normal[1] + in[2] * normal[2];

    scale = dot + fabsf(dot) * (-0.001f);
    overbounce = -scale;

    out[0] = in[0] + normal[0] * overbounce;
    out[1] = in[1] + normal[1] * overbounce;
    out[2] = in[2] + normal[2] * overbounce;
}

#define PMF_PRONE 0x00000001
#define PMF_DUCKED 0x00000002
#define PMF_MANTLE 0x00000004
#define PMF_LADDER 0x00000020
#define PMF_ADS 0x00000040
#define PMF_JUMPING 0x00080000
#define ENTITYNUM_WORLD 0x3fe
#define ENTITYNUM_NONE 0x3ff

static float PM_DvarFloat(const dvar_t *dvar, float fallback)
{
    return dvar ? dvar->current.value : fallback;
}

static int PM_DvarInt(const dvar_t *dvar, int fallback)
{
    return dvar ? dvar->current.integer : fallback;
}

static float PM_DotProduct(const vec_t *a, const vec_t *b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

static float PM_VectorLength2D(const vec_t *v)
{
    return sqrtf(v[0] * v[0] + v[1] * v[1]);
}

static void PM_VectorCopy(const vec_t *src, vec_t *dst)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
}

static void PM_VectorClear(vec_t *v)
{
    v[0] = 0.0f;
    v[1] = 0.0f;
    v[2] = 0.0f;
}

static int PM_AbsInt(int value)
{
    return value < 0 ? -value : value;
}

static float PM_CmdScale(const playerState_t *ps, const usercmd_t *cmd)
{
    int maxMove;
    float total;
    float scale;

    maxMove = PM_AbsInt(cmd->forwardmove);
    if (PM_AbsInt(cmd->rightmove) > maxMove)
        maxMove = PM_AbsInt(cmd->rightmove);

    if (!maxMove)
        return 0.0f;

    total = sqrtf((float)(cmd->forwardmove * cmd->forwardmove +
                          cmd->rightmove * cmd->rightmove));
    if (total <= 0.0f)
        return 0.0f;

    scale = (float)ps->speed * (float)maxMove / (127.0f * total);

    if (ps->pm_flags & PMF_SPRINT)
        scale *= SPRINT_SPEED_SCALE;

    if ((ps->pm_flags & 0x1) || ps->leanf != 0.0f)
        scale *= 0.40f;

    if (ps->pm_type == 2)
        scale *= 3.0f;
    else if (ps->pm_type == 3)
        scale *= 6.0f;
    else if (ps->pm_type == 4)
        scale *= PM_DvarFloat(player_spectateSpeedScale, 2.0f);
    else {

        int stance = PM_GetEffectiveStance((playerState_t *)ps);
        if (stance == 1)
            scale *= 0.15f;
        else if (stance == 2)
            scale *= 0.65f;
    }

    return scale;
}

static void PM_UpdateSprint(pmove_t *pm, pml_t *pml)
{
    playerState_t *ps = pm->ps;
    /* Holding breath shares Shift; aiming takes precedence over sprinting.
     * Recompute for every command on both sides of the network. */
    ps->pm_flags &= ~PMF_SPRINT;
    if (ps->pm_type != 0 || !pml->walking ||
        ps->groundEntityNum == ENTITYNUM_NONE ||
        !(pm->cmd.buttons & BUTTON_SPRINT) || pm->cmd.forwardmove <= 0 ||
        (ps->pm_flags & (0x1 | 0x2 | PMF_MANTLE | PMF_LADDER | 0x40 | 0x800 | 0x10000)) ||
        (ps->eFlags & 0x300) || ps->leanf != 0.0f ||
        ps->fWeaponPosFrac > 0.0f || ps->weaponstate != 0 ||
        (pm->cmd.buttons & (0x1 | 0x4 | 0x8 | 0x10 | 0x20 | 0x40 | 0x80 |
                            0x4000 | 0x10000 | 0x20000)))
        return;
    ps->pm_flags |= PMF_SPRINT;
}

static void PM_DropTimers(playerState_t *ps, int msec)
{
    /* Timed jump/land/weapon gestures must release the movement animation. */
    if (ps->legsTimer > 0) {
        ps->legsTimer -= msec;
        if (ps->legsTimer < 0)
            ps->legsTimer = 0;
    }
    if (ps->torsoTimer > 0) {
        ps->torsoTimer -= msec;
        if (ps->torsoTimer < 0)
            ps->torsoTimer = 0;
    }

    if (ps->pm_time > 0) {
        ps->pm_time -= msec;
        if (ps->pm_time <= 0) {
            ps->pm_time = 0;
            ps->pm_flags &= ~(0x200 | 0x400 | PMF_JUMPING);
        }
    }

    /* PM_Weapon owns weapon and cook timers. Decrementing weaponDelay here
     * can consume its zero transition before the reload/fire action sees it. */

    if (ps->foliageSoundTime > 0) {
        ps->foliageSoundTime -= msec;
        if (ps->foliageSoundTime < 0)
            ps->foliageSoundTime = 0;
    }

    if (ps->damageTimer > 0) {
        ps->damageTimer -= msec;
        if (ps->damageTimer < 0)
            ps->damageTimer = 0;
    }

    if (ps->damageDuration > 0) {
        ps->damageDuration -= msec;
        if (ps->damageDuration < 0)
            ps->damageDuration = 0;
    }

    /* PM_UpdateHoldBreath owns both breath usage and recovery. Decrementing
     * here cancels its usage increment and allows an unlimited steady aim. */
}

void PM_UpdateLean(playerState_t *ps, float msec, usercmd_t *cmd, void (*capsuleTrace)())
{
    (void)msec;
    (void)cmd;
    (void)capsuleTrace;
    ps->leanf = 0.0f;
}

void PM_UpdateViewAngles(playerState_t *ps, float msec, usercmd_t *cmd, int handler)
{
    int i;
    int minPitch;
    int maxPitch;
    float oldViewYaw;
    float newViewYaw;
    int proneBlocked;
    const float shortToAngle = 360.0f / 65536.0f;
    const float angleToShort = 65536.0f / 360.0f;

    if (!ps || !cmd)
        return;

    if (ps->pm_type == 5)
        return;

    if (ps->pm_type > 5) {

        int yaw = (short)(cmd->angles[1] + ps->delta_angles[1]);
        if (ps->stats[1] == 0x3e7)
            ps->stats[1] = (int)((float)yaw * shortToAngle);
        goto do_lean;
    }

    oldViewYaw = ps->viewangles[1];

    minPitch = (int)(PM_DvarFloat(player_view_pitch_up, 85.0f) * angleToShort) & 0xffff;
    maxPitch = (int)(PM_DvarFloat(player_view_pitch_down, 85.0f) * angleToShort) & 0xffff;

    for (i = 0; i < 3; ++i) {
        int angle = (short)(cmd->angles[i] + ps->delta_angles[i]);

        if (i == 0) {
            if (angle > maxPitch) {
                angle = maxPitch;
                ps->delta_angles[0] = angle - cmd->angles[0];
            } else if (angle < -minPitch) {
                angle = -minPitch;
                ps->delta_angles[0] = angle - cmd->angles[0];
            }
        }

        ps->viewangles[i] = (float)angle * shortToAngle;
    }

    newViewYaw = ps->viewangles[1];

    if (ps->eFlags & 0x300) {

        for (i = 0; i < 2; ++i) {
            float delta = AngleDelta(ps->viewAngleClampBase[i], ps->viewangles[i]);
            float range = ps->viewAngleClampRange[i];
            float x;

            if (delta > range)
                x = delta - range;
            else if (-range <= delta)
                continue;
            else
                x = range + delta;

            ps->delta_angles[i] += (int)(x * angleToShort) & 0xffff;
            if (x <= 0.0f)
                ps->viewangles[i] = AngleNormalize360Accurate(
                    ps->viewAngleClampBase[i] + range);
            else
                ps->viewangles[i] = AngleNormalize360Accurate(
                    ps->viewAngleClampBase[i] - range);
        }
        return;
    }

    if (ps->pm_flags & PMF_MANTLE) {
        Mantle_CapView(ps);
        return;
    }

    if ((ps->pm_flags & PMF_LADDER) && ps->groundEntityNum == 0x3ff &&
        bg_ladder_yawcap->current.value != 0.0f) {
        float yaw0 = vectoyaw(ps->vLadderVec) + 180.0f;
        float delta = AngleDelta(yaw0, ps->viewangles[1]);
        float cap = bg_ladder_yawcap->current.value;
        float x;

        if (delta > cap)
            x = delta - cap;
        else if (-cap <= delta)
            goto prone_check;
        else
            x = cap + delta;

        ps->delta_angles[1] += (int)(x * angleToShort) & 0xffff;
        if (x <= 0.0f)
            ps->viewangles[1] = AngleNormalize360Accurate(
                yaw0 + bg_ladder_yawcap->current.value);
        else
            ps->viewangles[1] = AngleNormalize360Accurate(
                yaw0 - bg_ladder_yawcap->current.value);
    }

prone_check:

    if ((ps->pm_flags & 0x1) && !(ps->eFlags & 0x300)) {
        float proneDelta = AngleDelta(ps->proneDirection, ps->viewangles[1]);
        float yawcapM5 = bg_prone_yawcap->current.value - 5.0f;
        float newProneYaw;

        if (proneDelta <= yawcapM5 && -yawcapM5 <= proneDelta &&
            (*(short *)&cmd->forwardmove == 0 ||
             (proneDelta == proneDelta && proneDelta == 0.0f))) {
            proneBlocked = 0;
        } else {

            float t70 = msec * 55.0f;
            float t72 = t70 * 0.001f;

            if (t72 <= (float)fabs(proneDelta)) {
                if (proneDelta <= 0.0f)
                    newProneYaw = ps->proneDirection + t72;
                else
                    newProneYaw = ps->proneDirection + t70 * -0.001f;
            } else {
                newProneYaw = ps->viewangles[1];
            }

            proneBlocked = 0;
            i = 1;
            for (;;) {
                if (BG_CheckProneTurned((byte *)ps, handler, newProneYaw)) {

                    if (BG_CheckProne(ps->clientNum, ps->origin, ps->maxs[0],
                                      30.0f, ps->viewangles[1], 0, 0, 0, 1,
                                      ps->groundEntityNum != 0x3ff, 0, handler,
                                      0, 45.0f)) {
                        if (BG_CheckProne(ps->clientNum, ps->origin, ps->maxs[0],
                                          30.0f, newProneYaw, 0, 0, 0, 1,
                                          ps->groundEntityNum != 0x3ff, 0,
                                          handler, 0, 45.0f))
                            ps->proneDirection = newProneYaw;
                        else
                            proneBlocked = 1;
                    } else {
                        proneBlocked = 1;
                    }
                    break;
                }
                if (i == 0)
                    break;
                {

                    float d = AngleDelta(ps->proneDirection, newProneYaw);
                    float step;
                    i = ((float)fabs(d) > 1.0f) ? 1 : 0;
                    if (i) {
                        step = (d <= 0.0f) ? -1.0f : 1.0f;
                    } else {
                        proneBlocked = 1;
                        step = d;
                    }
                    newProneYaw = AngleNormalize360Accurate(step + newProneYaw);
                }
            }
        }

        {
            float proneDelta2 = AngleDelta(ps->proneDirection, ps->viewangles[1]);
            if (proneDelta2 != 0.0f) {
                float runYaw = ps->proneDirection;
                int bRetry;
                i = 1;
                for (;;) {

                    bRetry = BG_CheckProne(ps->clientNum, ps->origin, ps->maxs[0],
                                           30.0f, runYaw, 0, 0, 0, 1,
                                           ps->groundEntityNum != 0x3ff, 0,
                                           handler, 0, 45.0f);
                    if (bRetry) {
                        if (BG_CheckProneTurned((byte *)ps, handler, runYaw))
                            ps->proneDirection = runYaw;
                        else
                            goto resolve_step;
                        break;
                    }
                resolve_step:
                    if (i == 0)
                        break;
                    {
                        float step, stepShort;
                        i = ((float)fabs(proneDelta2) > 1.0f) ? 1 : 0;
                        if (i) {
                            if (proneDelta2 <= 0.0f) {
                                step = -1.0f;
                                stepShort = -angleToShort;
                            } else {
                                step = 1.0f;
                                stepShort = angleToShort;
                            }
                        } else {
                            step = proneDelta2;
                            stepShort = proneDelta2 * angleToShort;
                        }

                        ps->delta_angles[1] += (int)stepShort & 0xffff;
                        ps->viewangles[1] = AngleNormalize360Accurate(
                            step + ps->viewangles[1]);

                        proneDelta2 = AngleDelta(ps->proneDirection, ps->viewangles[1]);
                        if (bRetry == 0)
                            runYaw = AngleNormalize360Accurate(runYaw + proneDelta2);
                    }
                    proneBlocked = 1;
                }
            }
        }

        {
            float proneDelta3 = AngleDelta(ps->proneDirection, ps->viewangles[1]);
            float yawcap = bg_prone_yawcap->current.value;
            float x;
            if (proneDelta3 > yawcap)
                x = proneDelta3 - yawcap;
            else if (-yawcap <= proneDelta3)
                goto prone_smooth;
            else
                x = proneDelta3 + yawcap;

            ps->delta_angles[1] += (int)(x * angleToShort) & 0xffff;
            if (x <= 0.0f)
                ps->viewangles[1] = AngleNormalize360Accurate(
                    ps->proneDirection + bg_prone_yawcap->current.value);
            else
                ps->viewangles[1] = AngleNormalize360Accurate(
                    ps->proneDirection - bg_prone_yawcap->current.value);
        }

    prone_smooth:

        if (proneBlocked) {
            float deltaYaw1;
            ps->pm_flags |= 0x10000;
            deltaYaw1 = AngleDelta(oldViewYaw, ps->viewangles[1]);
            if ((float)fabs(deltaYaw1) <= 1.0f) {
                float d2 = AngleDelta(newViewYaw, ps->viewangles[1]);
                if (d2 * deltaYaw1 > 0.0f) {
                    float adj = deltaYaw1 * 0.98f;
                    ps->viewangles[1] = AngleNormalize360Accurate(
                        adj + ps->viewangles[1]);
                    ps->delta_angles[1] += (int)(adj * angleToShort) & 0xffff;
                }
            }
        }

        {
            float pitchDelta = AngleDelta(ps->proneTorsoPitch, ps->viewangles[0]);
            float x;
            if (pitchDelta > 45.0f)
                x = pitchDelta - 45.0f;
            else if (pitchDelta >= -45.0f)
                goto type_check;
            else
                x = pitchDelta + 45.0f;

            ps->delta_angles[0] += (int)(x * angleToShort) & 0xffff;
            if (x <= 0.0f)
                ps->viewangles[0] = AngleNormalize180Accurate(
                    45.0f + ps->proneTorsoPitch);
            else
                ps->viewangles[0] = AngleNormalize180Accurate(
                    ps->proneTorsoPitch - 45.0f);
        }
    }

type_check:

    if (ps->pm_type == 3 || ps->pm_type == 2 || ps->pm_type == 4)
        return;

do_lean:

    PM_UpdateLean(ps, msec, cmd, pmoveHandlers[(unsigned char)handler].trace);
}

void PM_playerTrace(pmove_t *pm, trace_t *results, const vec_t *start,
                    const vec_t *mins, const vec_t *maxs, const vec_t *end,
                    int passEntityNum, int contentMask)
{
    int entityNum;
    int numtouch;
    int i;

    pmoveHandlers[(unsigned char)pm->handler].trace(results, start, mins, maxs,
                                                    end, passEntityNum, contentMask);

    if (!results->startsolid || !(results->contents & 0x02000000))
        return;

    entityNum = results->entityNum;
    if (entityNum != 0x3fe) {
        numtouch = pm->numtouch;
        if (numtouch != 0x20) {
            for (i = 0; i < numtouch; i++) {
                if (pm->touchents[i] == entityNum)
                    goto retry;
            }
            pm->touchents[numtouch] = entityNum;
            numtouch += 1;
            pm->numtouch = numtouch;
        }
    }

retry:
    pm->tracemask &= ~0x02000000;

    pmoveHandlers[(unsigned char)pm->handler].trace(results, start, mins, maxs, end,
                                                    passEntityNum,
                                                    contentMask & ~0x02000000);
}

static void PM_ACCELERATE_ABI PM_Accelerate(playerState_t *ps, pml_t *pml, const vec_t *wishdir,
                                            float wishspeed, float accel)
{
    float addspeed, accelspeed;

    if (ps->pm_flags & 0x20) {
        vec3_t push;
        float pushLen, t;
        push[0] = wishspeed * wishdir[0] - ps->velocity[0];
        push[1] = wishspeed * wishdir[1] - ps->velocity[1];
        push[2] = wishspeed * wishdir[2] - ps->velocity[2];
        pushLen = Vec3Normalize(push);
        t = fminf(pushLen, accel * pml->frametime * wishspeed);
        ps->velocity[0] += t * push[0];
        ps->velocity[1] += t * push[1];
        ps->velocity[2] += t * push[2];
        return;
    }

    addspeed = wishspeed - (ps->velocity[0] * wishdir[0] + ps->velocity[1] * wishdir[1]
                            + ps->velocity[2] * wishdir[2]);
    if (addspeed <= 0.0f)
        return;

    accelspeed = fminf(addspeed, fmaxf(stopspeed->current.value, wishspeed) * (accel * pml->frametime));

    if (ps->pm_type != 2 && inertiaMax->current.value < accelspeed
        && (ps->oldVelocity[0] * ps->oldVelocity[0] + ps->oldVelocity[1] * ps->oldVelocity[1]) >= 0.0001) {
        vec2_t newVel, oldVel;
        float dot;
        newVel[0] = ps->velocity[0] + accelspeed * wishdir[0];
        newVel[1] = ps->velocity[1] + accelspeed * wishdir[1];
        oldVel[0] = ps->oldVelocity[0];
        oldVel[1] = ps->oldVelocity[1];
        Vec2Normalize(oldVel);
        Vec2Normalize(newVel);
        dot = oldVel[0] * newVel[0] + oldVel[1] * newVel[1];
        if (dot < inertiaAngle->current.value) {
            if (inertiaDebug->current.enabled) {
                Com_Printf("angle is %f (oldVel is (%f,%f), vel is (%f, %f))\n",
                           (double)dot, (double)oldVel[0], (double)oldVel[1],
                           (double)newVel[0], (double)newVel[1]);
                Com_Printf("clamping acceleration from %f to %f\n",
                           (double)accelspeed, (double)inertiaMax->current.value);
            }
            accelspeed = inertiaMax->current.value;
        }
    }

    ps->velocity[0] += accelspeed * wishdir[0];
    ps->velocity[1] += accelspeed * wishdir[1];
    ps->velocity[2] += accelspeed * wishdir[2];
}

static void PM_REGPARM2_ABI PM_Friction(playerState_t *ps, pml_t *pml)
{
    float speed;
    float control;
    float drop;
    float newspeed;
    float vx, vy, vz;

    vx = ps->velocity[0];
    vy = ps->velocity[1];
    vz = ps->velocity[2];
    if (pml->walking && !(ps->pm_flags & PMF_LADDER))
        vz = 0.0f;
    speed = sqrtf(vx * vx + vy * vy + vz * vz);

    if (speed < 1.0f) {
        ps->velocity[0] = 0.0f;
        ps->velocity[1] = 0.0f;
        ps->velocity[2] = 0.0f;
        return;
    }

    drop = 0.0f;

    if ((ps->pm_flags & PMF_LADDER) || (pml->walking && !(pml->groundTrace.surfaceFlags & 2) &&
        !(ps->pm_flags & 0x400) && ps->pm_type != 4)) {
        control = speed;
        if (PM_DvarFloat(stopspeed, 100.0f) > control)
            control = PM_DvarFloat(stopspeed, 100.0f);

        if (ps->pm_flags & PMF_JUMPING)
            control *= Jump_ReduceFriction(ps);
        if (ps->pm_flags & 0x200)
            control *= 0.3f;

        drop += control * PM_DvarFloat(friction, 5.5f) * pml->frametime;
    }

    if (ps->pm_type == 4)
        drop += speed * 5.0f * pml->frametime;

    newspeed = speed - drop;
    if (newspeed < 0.0f)
        newspeed = 0.0f;
    newspeed /= speed;

    ps->velocity[0] *= newspeed;
    ps->velocity[1] *= newspeed;
    ps->velocity[2] *= newspeed;
}

static void PM_REGPARM2_ABI PM_CheckDuck(pmove_t *pm, pml_t *pml)
{
    playerState_t *ps = pm->ps;
    int wasCrouched;
    int oldViewHeightTarget = ps->viewHeightTarget;
    trace_t trace;
    vec3_t vEnd;
    vec3_t vPoint;
    float frac;
    float delta;
    vec3_t normal;

    pm->proneChange = 0;

    if (ps->pm_type == 4) {

        pm->mins[0] = -8.0f;
        pm->mins[1] = -8.0f;
        pm->mins[2] = -8.0f;
        pm->maxs[0] = 8.0f;
        pm->maxs[1] = 8.0f;
        pm->maxs[2] = 16.0f;
        ps->pm_flags &= ~0x3;
        if (pm->cmd.buttons & 0x100) {
            pm->cmd.buttons &= ~0x100;
            BG_AddPredictableEventToPlayerstate(140, 0, ps);
        }
        ps->viewHeightTarget = 0;
        ps->viewHeightCurrent = 0.0f;
        return;
    }

    wasCrouched = ps->pm_flags & 0x1;

    pm->mins[0] = ps->mins[0];
    pm->mins[1] = ps->mins[1];
    pm->maxs[0] = ps->maxs[0];
    pm->maxs[1] = ps->maxs[1];
    pm->mins[2] = ps->mins[2];

    if (ps->pm_type > 5) {
        pm->maxs[2] = ps->maxs[2];
        ps->viewHeightTarget = 8;
        PM_ViewHeightAdjust(pm, pml);
        return;
    }

    if (ps->eFlags & 0x300) {

        if (ps->eFlags & 0x100) {
            if (ps->eFlags & 0x200) {
                ps->pm_flags &= ~0x3;
                goto compute_target;
            }
            ps->pm_flags = (ps->pm_flags | 0x1) & ~0x2;
            goto compute_target;
        }

        ps->pm_flags = (ps->pm_flags | 0x2) & ~0x1;
        goto compute_target;
    }

    if ((short)ps->pm_flags < 0) {

        goto compute_target;
    }

    if (ps->pm_flags & 0x20) {

        if (pm->cmd.buttons & 0x300) {
            pm->cmd.buttons &= ~0x300;
            BG_AddPredictableEventToPlayerstate(140, 0, ps);
        }
    }

    if (pm->cmd.buttons & 0x100) {

        if (ps->pm_flags & 0x1)
            goto set_prone;
        if (ps->groundEntityNum == 0x3ff)
            goto compute_target;
        if (BG_CheckProne(ps->clientNum, ps->origin, pm->maxs[0],
                          30.0f, ps->viewangles[1],
                          &ps->fTorsoHeight, &ps->fTorsoPitch, &ps->fWaistPitch,
                          0, 1, NULL, (unsigned char)pm->handler, 0, 66.0f))
            goto set_prone;
        if (ps->groundEntityNum != 0x3ff) {
            ps->pm_flags |= 0x10000;
            if (!(pm->cmd.buttons & 0x2000))
                BG_AddPredictableEventToPlayerstate(140, 0, ps);
        }
        goto compute_target;
    }

    if (pm->cmd.buttons & 0x200) {

        if (ps->pm_flags & 0x1) {

            pm->maxs[2] = 50.0f;
            PM_playerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs,
                           ps->origin, ps->clientNum, pm->tracemask & ~0x02000000);
            if (!trace.allsolid) {
                ps->pm_flags = (ps->pm_flags & ~0x1) | 0x2;
            } else if (!(pm->cmd.buttons & 0x2000)) {
                BG_AddPredictableEventToPlayerstate(142, 2, ps);
            }
        } else {
            ps->pm_flags |= 0x2;
        }
        goto compute_target;
    }

    if (ps->pm_flags & 0x1) {

        pm->maxs[2] = ps->maxs[2];
        PM_playerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs,
                       ps->origin, ps->clientNum, pm->tracemask & ~0x02000000);
        if (!trace.allsolid) {
            ps->pm_flags &= ~0x3;
        } else {
            pm->maxs[2] = 50.0f;
            PM_playerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs,
                           ps->origin, ps->clientNum, pm->tracemask & ~0x02000000);
            if (!trace.allsolid)
                ps->pm_flags = (ps->pm_flags & ~0x1) | 0x2;
            else if (!(pm->cmd.buttons & 0x2000))
                BG_AddPredictableEventToPlayerstate(142, 1, ps);
        }
        goto compute_target;
    }

    if (ps->pm_flags & 0x2) {

        pm->maxs[2] = ps->maxs[2];
        PM_playerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs,
                       ps->origin, ps->clientNum, pm->tracemask & ~0x02000000);
        if (!trace.allsolid) {
            ps->pm_flags &= ~0x2;
        } else if (!(pm->cmd.buttons & 0x2000)) {
            BG_AddPredictableEventToPlayerstate(141, 1, ps);
        }
    }
    goto compute_target;

set_prone:
    ps->pm_flags = (ps->pm_flags | 0x1) & ~0x2;
    goto compute_target;

compute_target:
    if (ps->viewHeightLerpTime == 0) {
        if (ps->pm_flags & 0x1) {

            if (ps->viewHeightTarget == 60) {
                ps->viewHeightTarget = 40;
            } else if (ps->viewHeightTarget != 11) {
                ps->viewHeightTarget = 11;
                pm->proneChange = 1;
                BG_PlayAnim(ps, 0, 2, 0, 0, 1, 1);
                Jump_ActivateSlowdown(ps);
            }
        } else if (ps->viewHeightTarget == 11) {
            ps->viewHeightTarget = 40;
            pm->proneChange = 1;
            BG_PlayAnim(ps, 0, 2, 0, 0, 1, 1);
        } else {

            ps->viewHeightTarget = (ps->pm_flags & 0x2) ? 40 : 60;
        }
    }

    /* Fire one transition at the actual stance boundary. The 60->11 and
     * 11->60 paths stage through crouch, so the original prone transition
     * clips play instead of being skipped or restarted by each usercmd. */
    if (ps->viewHeightTarget != oldViewHeightTarget) {
        int event = oldViewHeightTarget == 11 ? ANIM_ET_PRONE_TO_CROUCH :
            ps->viewHeightTarget == 11 ? ANIM_ET_CROUCH_TO_PRONE :
            ps->viewHeightTarget == 40 ? ANIM_ET_STAND_TO_CROUCH : ANIM_ET_CROUCH_TO_STAND;
        BG_AnimScriptEvent(ps, event, 0, 0);
    }

    PM_ViewHeightAdjust(pm, pml);

    if (ps->viewHeightTarget == 40) {
        pm->maxs[2] = 50.0f;
        ps->eFlags = (ps->eFlags | 0x4) & ~0x8;
        ps->pm_flags = (ps->pm_flags | 0x2) & ~0x1;
    } else if (ps->viewHeightTarget == 11) {
        pm->maxs[2] = 30.0f;
        ps->eFlags = (ps->eFlags | 0x8) & ~0x4;
        ps->pm_flags = (ps->pm_flags | 0x1) & ~0x2;
    } else {
        pm->maxs[2] = ps->maxs[2];
        ps->eFlags &= ~0xc;
        ps->pm_flags &= ~0x3;
    }

    if (!(ps->pm_flags & 0x1) || wasCrouched != 0)
        return;

    if (pm->cmd.forwardmove != 0) {
        ps->pm_flags &= ~0x8;
        PM_ExitAimDownSight(ps);
    }

    vEnd[0] = ps->origin[0];
    vEnd[1] = ps->origin[1];
    vEnd[2] = ps->origin[2] + 10.0f;

    PM_playerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs, vEnd,
                   ps->clientNum, pm->tracemask & ~0x02000000);
    frac = trace.fraction;
    vEnd[0] = ps->origin[0] + (vEnd[0] - ps->origin[0]) * frac;
    vEnd[1] = ps->origin[1] + (vEnd[1] - ps->origin[1]) * frac;
    vEnd[2] = ps->origin[2] + (vEnd[2] - ps->origin[2]) * frac;

    PM_playerTrace(pm, &trace, vEnd, pm->mins, pm->maxs, ps->origin,
                   ps->clientNum, pm->tracemask & ~0x02000000);
    frac = trace.fraction;
    ps->origin[0] = vEnd[0] + (ps->origin[0] - vEnd[0]) * frac;
    ps->origin[1] = vEnd[1] + (ps->origin[1] - vEnd[1]) * frac;
    ps->origin[2] = vEnd[2] + (ps->origin[2] - vEnd[2]) * frac;

    ps->proneDirection = ps->viewangles[1];

    vPoint[0] = ps->origin[0];
    vPoint[1] = ps->origin[1];
    vPoint[2] = ps->origin[2] - 0.25f;

    PM_playerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs, vPoint,
                   ps->clientNum, pm->tracemask & ~0x02000000);

    normal[0] = trace.normal[0];
    normal[1] = trace.normal[1];
    normal[2] = trace.normal[2];
    if (trace.startsolid || trace.fraction >= 1.0f)
        ps->proneDirectionPitch = 0.0f;
    else
        ps->proneDirectionPitch = PitchForYawOnNormal(ps->proneDirection, normal);

    delta = AngleDelta(ps->proneDirectionPitch, ps->viewangles[0]);
    if (delta < -45.0f)
        ps->proneTorsoPitch = ps->viewangles[0] - 45.0f;
    else if (delta > 45.0f)
        ps->proneTorsoPitch = 45.0f + ps->viewangles[0];
    else
        ps->proneTorsoPitch = ps->proneDirectionPitch;
}

static void PM_REGPARM2_ABI PM_SetMovementDir(pmove_t *pm, pml_t *pml)
{
    playerState_t *ps = pm->ps;
    int fmove = pm->cmd.forwardmove;
    int smove = pm->cmd.rightmove;

    (void)pml;

    /* angles2.y and lerpMoveDir carry signed degrees, not Quake's eight
     * direction indices. Backward clips already face against their travel. */
    if (fmove || smove) {
        float yaw = atan2f((float)-smove, (float)fmove) * 57.29577951308232f;
        if (fmove < 0)
            yaw = AngleNormalize180Accurate(yaw + 180.0f);
        else if (PM_AbsInt(smove) > PM_AbsInt(fmove))
            yaw += smove < 0 ? -90.0f : 90.0f;
        ps->movementDir = (int)yaw;
    }
}

static void PM_ApplyGroundPlane(playerState_t *ps, pml_t *pml)
{
    float velDot;

    if (!pml->groundPlane)
        return;

    velDot = PM_DotProduct(ps->velocity, pml->groundTrace.normal);
    if (velDot < 0.0f)
        PM_ClipVelocity(ps->velocity, pml->groundTrace.normal, ps->velocity);
}

static void PM_WalkMove(pmove_t *pm, pml_t *pml)
{
    playerState_t *ps = pm->ps;
    vec3_t wishvel;
    vec3_t wishdir;
    float scale;
    float wishspeed;

    if (Jump_Check(pm, pml)) {
        PM_AirMove(pm, pml);
        return;
    }

    PM_Friction(ps, pml);

    /* Releasing movement plants the feet on ordinary ground. Shared
     * prediction/server physics must stop together rather than let the old
     * walking or sprint velocity coast through several neutral commands.
     * Airborne motion, slippery surfaces and timed landing/knockback retain
     * their momentum; Jump_Check above already owns a newly started jump. */
    if (ps->pm_type == 0 && !pm->cmd.forwardmove && !pm->cmd.rightmove &&
        pml->walking && !(pml->groundTrace.surfaceFlags & 2) &&
        !(ps->pm_flags & (PMF_LADDER | PMF_JUMPING | 0x200 | 0x400))) {
        ps->velocity[0] = ps->velocity[1] = ps->velocity[2] = 0.0f;
    }

    scale = PM_CmdScale(ps, &pm->cmd);

    wishvel[0] = pml->forward[0] * (float)pm->cmd.forwardmove +
                 pml->right[0] * (float)pm->cmd.rightmove;
    wishvel[1] = pml->forward[1] * (float)pm->cmd.forwardmove +
                 pml->right[1] * (float)pm->cmd.rightmove;
    wishvel[2] = 0.0f;

    PM_VectorCopy(wishvel, wishdir);
    wishspeed = Vec3Normalize(wishdir) * scale;

    PM_Accelerate(ps, pml, wishdir, wishspeed, 9.0f);
    PM_ApplyGroundPlane(ps, pml);
    PM_StepSlideMove(pm, pml, 0);
    PM_SetMovementDir(pm, pml);
}

static void PM_REGPARM2_ABI PM_AirMove(pmove_t *pm, pml_t *pml)
{
    playerState_t *ps = pm->ps;
    vec3_t wishvel;
    vec3_t wishdir;
    signed char fmove, smove;
    float ffmove, fsmove;
    int maxMove;
    float total;
    float scale;
    float wishspeed;
    int i;

    PM_Friction(ps, pml);

    fmove = pm->cmd.forwardmove;
    ffmove = (float)fmove;
    smove = pm->cmd.rightmove;
    fsmove = (float)smove;

    {
        int absf, abss;

        total = sqrtf((float)(fmove * fmove + smove * smove));
        absf = fmove < 0 ? -fmove : fmove;
        abss = smove < 0 ? -smove : smove;
        if (absf >= abss)
            abss = absf;
        maxMove = abss;
    }

    if (maxMove == 0) {
        scale = 0.0f;
    } else {
        scale = (float)ps->speed * (float)maxMove / (127.0f * total);
        if ((ps->pm_flags & 0x100) || ps->leanf != 0.0f)
            scale *= 0.4f;
        if (ps->pm_type == 2)
            scale *= 3.0f;
        else if (ps->pm_type == 3)
            scale *= 6.0f;
        else if (ps->pm_type == 4)
            scale *= (*(const dvar_t **)imp_player_spectateSpeedScale)->current.value;
    }

    pml->forward[2] = 0.0f;
    pml->right[2] = 0.0f;
    Vec3Normalize(pml->forward);
    Vec3Normalize(pml->right);

    for (i = 0; i < 2; i++)
        wishvel[i] = ffmove * pml->forward[i] + fsmove * pml->right[i];
    wishvel[2] = 0.0f;

    wishdir[0] = wishvel[0];
    wishdir[1] = wishvel[1];
    wishdir[2] = 0.0f;
    wishspeed = Vec3Normalize(wishdir) * scale;

    PM_Accelerate(ps, pml, wishdir, wishspeed, 1.0f);

    if (pml->groundPlane) {
        const vec_t *normal = pml->groundTrace.normal;
        vec_t *out = ps->velocity;
        float dot = ps->velocity[0] * normal[0] +
                    ps->velocity[1] * normal[1] +
                    ps->velocity[2] * normal[2];
        float overbounce = -(dot + fabsf(dot) * (-0.001f));
        out[0] = ps->velocity[0] + normal[0] * overbounce;
        out[1] = ps->velocity[1] + normal[1] * overbounce;
        out[2] = ps->velocity[2] + normal[2] * overbounce;
    }

    PM_StepSlideMove(pm, pml, 1);
    PM_SetMovementDir(pm, pml);
}

static void PM_NoclipMove(pmove_t *pm, pml_t *pml)
{
    playerState_t *ps = pm->ps;
    vec3_t wishvel;
    vec3_t wishdir;
    float scale;
    float wishspeed;

    PM_Friction(ps, pml);

    scale = PM_CmdScale(ps, &pm->cmd);
    wishvel[0] = pml->forward[0] * (float)pm->cmd.forwardmove +
                 pml->right[0] * (float)pm->cmd.rightmove;
    wishvel[1] = pml->forward[1] * (float)pm->cmd.forwardmove +
                 pml->right[1] * (float)pm->cmd.rightmove;
    wishvel[2] = pml->forward[2] * (float)pm->cmd.forwardmove;

    if (pm->cmd.buttons & 0x80)
        wishvel[2] += 127.0f;
    if (pm->cmd.buttons & 0x40)
        wishvel[2] -= 127.0f;

    PM_VectorCopy(wishvel, wishdir);
    wishspeed = Vec3Normalize(wishdir) * scale;
    PM_Accelerate(ps, pml, wishdir, wishspeed, 8.0f);

    ps->origin[0] += ps->velocity[0] * pml->frametime;
    ps->origin[1] += ps->velocity[1] * pml->frametime;
    ps->origin[2] += ps->velocity[2] * pml->frametime;
    PM_SetMovementDir(pm, pml);
}

static void PM_CheckLadderMove(pmove_t *pm, pml_t *pml)
{
    playerState_t *ps = pm->ps;
    qboolean wasLadder = (ps->pm_flags & PMF_LADDER) != 0;
    vec3_t direction, end;
    trace_t trace;

    ps->pm_flags &= ~PMF_LADDER;
    /* Only live, standing players climb. Jumping away must not reattach on
       the very next prediction/server movement step. */
    if (ps->pm_type != 0 || (ps->pm_flags & 0x3) ||
        ((ps->pm_flags & PMF_JUMPING) && pm->cmd.serverTime - ps->jumpTime < 500) ||
        (!wasLadder && pml->walking && pm->cmd.forwardmove <= 0))
        return;

    if (wasLadder) {
        direction[0] = -ps->vLadderVec[0];
        direction[1] = -ps->vLadderVec[1];
    } else {
        direction[0] = pml->forward[0];
        direction[1] = pml->forward[1];
    }
    direction[2] = 0.0f;
    if (Vec3Normalize(direction) == 0.0f)
        return;
    end[0] = ps->origin[0] + direction[0] * 2.0f;
    end[1] = ps->origin[1] + direction[1] * 2.0f;
    end[2] = ps->origin[2];
    PM_playerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs, end,
                   ps->clientNum, pm->tracemask);
    /* The visible wooden rungs can be just in front of the ladder brush.
       Its authored player-clip volume, not the rung's wood material, marks
       the climbable surface. Keep the same short reach for this second trace. */
    if (!(trace.surfaceFlags & 0x8) && (pm->tracemask & 0x10000))
        PM_playerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs, end,
                       ps->clientNum, 0x10000);
    /* SURF_LADDER is carried by the original BSP's collision material. */
    if (trace.fraction == 1.0f || trace.startsolid ||
        !(trace.surfaceFlags & 0x8) || fabsf(trace.normal[2]) >= 0.3f)
        return;
    ps->pm_flags |= PMF_LADDER;
    PM_VectorCopy(trace.normal, ps->vLadderVec);
    Jump_ClearState(ps);
    if (!wasLadder)
        ps->velocity[2] = 0.0f;
}

static void PM_REGPARM2_ABI PM_LadderMove(pmove_t *pm, pml_t *pml)
{
    playerState_t *ps = pm->ps;
    vec3_t wishvel, wishdir;
    float scale, wishspeed, into, climb;

    if (Jump_Check(pm, pml)) {
        PM_AirMove(pm, pml);
        return;
    }

    if (!pm->cmd.forwardmove && !pm->cmd.rightmove)
        PM_Friction(ps, pml);
    scale = PM_CmdScale(ps, &pm->cmd) * 0.5f;
    wishvel[0] = pml->forward[0] * pm->cmd.forwardmove + pml->right[0] * pm->cmd.rightmove;
    wishvel[1] = pml->forward[1] * pm->cmd.forwardmove + pml->right[1] * pm->cmd.rightmove;
    wishvel[2] = 0.0f;
    into = PM_DotProduct(wishvel, ps->vLadderVec);
    wishvel[0] -= into * ps->vLadderVec[0];
    wishvel[1] -= into * ps->vLadderVec[1];
    /* Forward climbs while looking level/up; looking down reverses it. */
    climb = fmaxf(-1.0f, fminf(1.0f, (pml->forward[2] + 0.5f) * 2.0f));
    wishvel[2] = climb * pm->cmd.forwardmove;
    PM_VectorCopy(wishvel, wishdir);
    wishspeed = Vec3Normalize(wishdir) * scale;
    PM_Accelerate(ps, pml, wishdir, wishspeed, 9.0f);

    /* Keep contact while climbing and move onto the roof as the capsule
       clears the last rung. Normal collision/step traces still apply. */
    if (pm->cmd.forwardmove) {
        ps->velocity[0] -= ps->vLadderVec[0] * 20.0f;
        ps->velocity[1] -= ps->vLadderVec[1] * 20.0f;
    }
    PM_ApplyGroundPlane(ps, pml);
    PM_StepSlideMove(pm, pml, 0);
    PM_SetMovementDir(pm, pml);
}

static void PM_GetViewHeightLerp(const viewLerpWaypoint_t *table, int frac,
                                 float *pfViewHeight, float *pfOffset)
{
    int i;

    if (frac == 0) {
        *pfViewHeight = table[0].fViewHeight;
        *pfOffset = (float)table[0].iOffset;
        return;
    }

    for (i = 1;; i++) {
        const viewLerpWaypoint_t *cur = &table[i];

        if (frac == cur->iFrac) {
            *pfViewHeight = cur->fViewHeight;
            *pfOffset = (float)cur->iOffset;
            return;
        }

        if (frac < cur->iFrac) {
            const viewLerpWaypoint_t *prev = &table[i - 1];
            float t = (float)(frac - prev->iFrac) /
                      (float)(cur->iFrac - prev->iFrac);
            *pfOffset = (float)prev->iOffset +
                        (float)(cur->iOffset - prev->iOffset) * t;
            *pfViewHeight = prev->fViewHeight +
                            (cur->fViewHeight - prev->fViewHeight) * t;
            return;
        }

        if (cur->iFrac == -1)
            break;
    }

    *pfViewHeight = table[0].fViewHeight;
    *pfOffset = (float)table[0].iOffset;
}

static void PM_REGPARM2_ABI PM_ViewHeightAdjust(pmove_t *pm, pml_t *pml)
{
    playerState_t *ps = pm->ps;
    int target = ps->viewHeightTarget;
    float current = ps->viewHeightCurrent;
    int frac;
    int lerpTarget;
    qboolean bDown;
    int duration;
    float fViewHeight;
    float fOffset;
    const viewLerpWaypoint_t *table;

    if (target == 0) {
        if (ps->pm_type == 4)
            ps->viewHeightCurrent = 0.0f;
        else
            ps->viewHeightCurrent = (float)target;
        return;
    }

    if (current == (float)target && ps->viewHeightLerpTime == 0)
        return;

    if (target != 11 && target != 40 && target != 60) {

        ps->viewHeightLerpTime = 0;
        if ((float)ps->viewHeightTarget > current) {
            current += 180.0f * pml->frametime;
            ps->viewHeightCurrent = current;
            if (current >= (float)ps->viewHeightTarget)
                ps->viewHeightCurrent = (float)ps->viewHeightTarget;
        } else {
            current += -180.0f * pml->frametime;
            ps->viewHeightCurrent = current;
            if (current <= (float)ps->viewHeightTarget)
                ps->viewHeightCurrent = (float)ps->viewHeightTarget;
        }
        return;
    }

    if (ps->viewHeightLerpTime != 0) {
        bDown = ps->viewHeightLerpDown;
        lerpTarget = ps->viewHeightLerpTarget;

        if (lerpTarget == 11)
            duration = 0x190;
        else if (lerpTarget == 40)
            duration = bDown ? 0xc8 : 0x190;
        else
            duration = 0xc8;

        frac = ((pm->cmd.serverTime - ps->viewHeightLerpTime) * 100) / duration;
        if (frac < 0)
            frac = 0;
        if (frac >= 100) {

            ps->viewHeightCurrent = (float)lerpTarget;
            ps->viewHeightLerpTime = 0;
            ps->viewHeightLerpPosAdj = 0.0f;
            frac = 100;
        } else {
            if (lerpTarget == 11)
                table = viewLerp_CrouchProne;
            else if (lerpTarget == 40)
                table = bDown ? viewLerp_StandCrouch : viewLerp_ProneCrouch;
            else
                table = viewLerp_CrouchStand;

            PM_GetViewHeightLerp(table, frac, &fViewHeight, &fOffset);

            ps->viewHeightCurrent = fViewHeight;

            if (fabsf(ps->viewHeightLerpPosAdj - fOffset) > 0.05f) {
                vec3_t vFlatForward;
                float savedY = ps->velocity[1];
                float savedZ = ps->velocity[2];
                float diff = fOffset - ps->viewHeightLerpPosAdj;
                float move;

                if (ps->groundEntityNum == 0x3ff)
                    diff *= 0.5f;
                move = diff / pml->frametime;

                vFlatForward[0] = pml->forward[0];
                vFlatForward[1] = pml->forward[1];
                vFlatForward[2] = 0.0f;
                Vec3Normalize(vFlatForward);

                ps->velocity[0] = move * vFlatForward[0];
                ps->velocity[1] = move * vFlatForward[1];
                ps->velocity[2] = move * vFlatForward[2];

                PM_StepSlideMove(pm, pml, 1);

                ps->velocity[1] = savedY;
                ps->velocity[2] = savedZ;
                ps->viewHeightLerpPosAdj = fOffset;
            }
        }
    } else {
        frac = 0;
    }

    if (ps->viewHeightLerpTime == 0) {
        if ((float)ps->viewHeightTarget == ps->viewHeightCurrent)
            return;

        ps->viewHeightLerpTime = pm->cmd.serverTime;
        target = ps->viewHeightTarget;
        if (target == 11) {
            ps->viewHeightLerpDown = 1;
            ps->viewHeightLerpTarget = (ps->viewHeightCurrent <= 40.0f) ? target : 40;
        } else if (target == 40) {
            ps->viewHeightLerpDown = (ps->viewHeightCurrent > 40.0f);
            ps->viewHeightLerpTarget = 40;
        } else if (target == 60) {
            ps->viewHeightLerpDown = 0;
            ps->viewHeightLerpTarget = (ps->viewHeightCurrent >= 40.0f) ? target : 40;
        }
        return;
    }

    lerpTarget = ps->viewHeightLerpTarget;
    if (ps->viewHeightTarget == lerpTarget)
        return;
    if (ps->viewHeightTarget > lerpTarget) {
        if (ps->viewHeightLerpDown)
            goto restart_lerp;
        return;
    }

    if (ps->viewHeightLerpDown)
        return;

restart_lerp: {
    int iLerpFrac = 100 - frac;
    int newDuration;

    ps->viewHeightLerpDown ^= 1;

    if (ps->viewHeightLerpDown) {
        if (lerpTarget == 60)
            ps->viewHeightLerpTarget = 40;
        else if (lerpTarget == 40)
            ps->viewHeightLerpTarget = 11;
    } else {
        if (lerpTarget == 11)
            ps->viewHeightLerpTarget = 40;
        else if (lerpTarget == 40)
            ps->viewHeightLerpTarget = 60;
    }

    if (iLerpFrac == 100) {
        ps->viewHeightCurrent = (float)ps->viewHeightLerpTarget;
        ps->viewHeightLerpTime = 0;
        ps->viewHeightLerpPosAdj = 0.0f;
        return;
    }

    lerpTarget = ps->viewHeightLerpTarget;
    if (lerpTarget == 11)
        newDuration = 0x190;
    else if (lerpTarget == 40)
        newDuration = ps->viewHeightLerpDown ? 0xc8 : 0x190;
    else
        newDuration = 0xc8;

    ps->viewHeightLerpTime =
        pm->cmd.serverTime -
        (int)((float)iLerpFrac * 0.01f * (float)newDuration);

    if (lerpTarget == 11) {
        table = viewLerp_CrouchProne;
    } else if (lerpTarget == 40) {
        table = ps->viewHeightLerpDown ? viewLerp_StandCrouch
                                       : viewLerp_ProneCrouch;
    } else {
        table = viewLerp_CrouchStand;
    }

    PM_GetViewHeightLerp(table, iLerpFrac, &fViewHeight, &fOffset);
    ps->viewHeightLerpPosAdj = fOffset;
}
}

void PM_UpdatePronePitch(pmove_t *pm, pml_t *pml)
{
    (void)pm;
    (void)pml;
}

void PM_FootstepEvent(pmove_t *pm, pml_t *pml, int iOldBobCycle, int iNewBobCycle, qboolean bFootStep)
{
    int surface, event;
    if (!bFootStep || !pml->walking || pm->ps->groundEntityNum == ENTITYNUM_NONE ||
        !((iOldBobCycle ^ iNewBobCycle) & 0x40) ||
        (pml->groundTrace.surfaceFlags & 0x2000))
        return;

    surface = PM_GroundSurfaceType(pml);
    if ((unsigned)surface >= 23)
        surface = 0;
    /* Each movement sound family has the 23 original surface variants. */
    event = pm->ps->viewHeightTarget == 11 ? 47 :
            pm->ps->viewHeightTarget == 40 ? 24 : 1;
    PM_AddEvent(pm->ps, event + surface);
}

static void PM_REGPARM2_ABI PM_Footsteps(pmove_t *pm, pml_t *pml)
{
    int oldBob;
    int newBob;
    playerState_t *ps = pm->ps;
    scriptAnimMoveTypes_t moveType;
    int stance = PM_GetEffectiveStance(ps);
    qboolean moving = pm->xyspeed > 1.0f &&
        (pm->cmd.forwardmove || pm->cmd.rightmove);
    qboolean backwards = pm->cmd.forwardmove < 0;
    qboolean walking = (ps->pm_flags & 0x40) ||
        (PM_AbsInt(pm->cmd.forwardmove) < 127 && PM_AbsInt(pm->cmd.rightmove) < 127);

    /* Movement selects the original playeranim.script entries even when no
       ground footstep can play (notably while attached to a ladder). */
    if (ps->pm_flags & PMF_LADDER) {
        moveType = ps->velocity[2] < 0.0f ? ANIM_MT_CLIMBDOWN : ANIM_MT_CLIMBUP;
        ps->movementDir = (int)AngleDelta(vectoyaw(ps->vLadderVec) + 180.0f,
                                        ps->viewangles[1]);
    } else if (!moving || !pml->walking) {
        moveType = stance == 1 ? ANIM_MT_IDLEPRONE :
                   stance == 2 ? ANIM_MT_IDLECR : ANIM_MT_IDLE;
    } else if (stance == 1) {
        moveType = backwards ? ANIM_MT_WALKPRONEBK : ANIM_MT_WALKPRONE;
    } else if (stance == 2) {
        moveType = walking ? (backwards ? ANIM_MT_WALKCRBK : ANIM_MT_WALKCR) :
                            (backwards ? ANIM_MT_RUNCRBK : ANIM_MT_RUNCR);
    } else {
        moveType = walking ? (backwards ? ANIM_MT_WALKBK : ANIM_MT_WALK) :
                            (backwards ? ANIM_MT_RUNBK : ANIM_MT_RUN);
    }
    BG_AnimScriptAnimation(ps, AISTATE_COMBAT, moveType, 1);

    if (!PM_ShouldMakeFootsteps(pm))
        return;

    oldBob = pm->ps->bobCycle;
    newBob = (oldBob + (int)(pm->xyspeed * pml->frametime * 0.75f)) & 0xff;
    pm->ps->bobCycle = newBob;

    if ((oldBob ^ newBob) & 0x40)
        PM_FootstepEvent(pm, pml, oldBob, newBob, 1);
}

static void PM_REGPARM2_ABI PM_GroundTrace(pmove_t *pm, pml_t *pml)
{
    playerState_t *ps = pm->ps;
    vec3_t start;
    vec3_t point;
    trace_t trace;
    int i;
    float frac;

    start[0] = ps->origin[0];
    start[1] = ps->origin[1];
    point[0] = ps->origin[0];
    point[1] = ps->origin[1];
    if ((ps->eFlags & 0x300) == 0) {
        start[2] = ps->origin[2] + 0.25f;
        point[2] = ps->origin[2] - 0.25f;
    } else {
        start[2] = ps->origin[2];
        point[2] = start[2] - 1.0f;
    }

    PM_playerTrace(pm, &trace, start, pm->mins, pm->maxs, point,
                   ps->clientNum, pm->tracemask);

    pml->groundTrace = trace;

    if (trace.allsolid) {

        for (i = 0; i < 26; i++) {
            point[0] = ps->origin[0] + CorrectSolidDeltas[i][0];
            point[1] = ps->origin[1] + CorrectSolidDeltas[i][1];
            point[2] = ps->origin[2] + CorrectSolidDeltas[i][2];

            PM_playerTrace(pm, &trace, point, pm->mins, pm->maxs, point,
                           ps->clientNum, pm->tracemask);

            if (!trace.startsolid)
                break;
        }

        if (i == 26) {
            ps->groundEntityNum = ENTITYNUM_NONE;
            pml->groundPlane = 0;
            pml->almostGroundPlane = 0;
            pml->walking = 0;
            Jump_ClearState(ps);
            return;
        }

        ps->origin[0] = point[0];
        ps->origin[1] = point[1];
        ps->origin[2] = point[2];

        point[0] = ps->origin[0];
        point[1] = ps->origin[1];
        point[2] = ps->origin[2] - 1.0f;

        PM_playerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs, point,
                       ps->clientNum, pm->tracemask);

        pml->groundTrace = trace;

        frac = trace.fraction;
        ps->origin[0] += (point[0] - ps->origin[0]) * frac;
        ps->origin[1] += (point[1] - ps->origin[1]) * frac;
        ps->origin[2] += (point[2] - ps->origin[2]) * frac;
    }

    if (trace.startsolid) {

        start[2] = ps->origin[2] - 0.001f;

        PM_playerTrace(pm, &trace, start, pm->mins, pm->maxs, point,
                       ps->clientNum, pm->tracemask);

        if (trace.startsolid) {
            ps->groundEntityNum = ENTITYNUM_NONE;
            pml->groundPlane = 0;
            pml->almostGroundPlane = 0;
            pml->walking = 0;
            return;
        }

        pml->groundTrace = trace;
    }

    if (trace.fraction == 1.0f) {
        if (ps->groundEntityNum == ENTITYNUM_NONE) {
            point[0] = ps->origin[0];
            point[1] = ps->origin[1];
            point[2] = ps->origin[2] - 1.0f;

            PM_playerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs, point,
                           ps->clientNum, pm->tracemask);

            pml->almostGroundPlane = (trace.fraction != 1.0f);
        } else {
            point[0] = ps->origin[0];
            point[1] = ps->origin[1];
            point[2] = ps->origin[2] - 64.0f;

            PM_playerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs, point,
                           ps->clientNum, pm->tracemask);

            if (trace.fraction == 1.0f) {
                BG_AnimScriptEvent(ps, pm->cmd.forwardmove < 0 ? 4 : 3, 0, 1);
                pml->almostGroundPlane = 0;
                ps->groundEntityNum = ENTITYNUM_NONE;
                pml->groundPlane = 0;
                pml->walking = 0;
                return;
            }

            pml->almostGroundPlane = (trace.fraction < 0.015625f);
        }

        ps->groundEntityNum = ENTITYNUM_NONE;
        pml->groundPlane = 0;
        pml->walking = 0;
        return;
    }

    if (!(ps->pm_flags & 0x20) && ps->velocity[2] > 0.0f &&
        (ps->velocity[0] * trace.normal[0] +
         ps->velocity[1] * trace.normal[1] +
         ps->velocity[2] * trace.normal[2]) > 10.0f) {
        BG_AnimScriptEvent(ps, pm->cmd.forwardmove < 0 ? 4 : 3, 0, 0);
        pml->almostGroundPlane = 0;
        ps->groundEntityNum = ENTITYNUM_NONE;
        pml->groundPlane = 0;
        pml->walking = 0;
        return;
    }

    if (trace.normal[2] < 0.7f) {
        ps->groundEntityNum = ENTITYNUM_NONE;
        pml->groundPlane = 1;
        pml->almostGroundPlane = 1;
        pml->walking = 0;
        Jump_ClearState(ps);
        return;
    }

    pml->groundPlane = 1;
    pml->almostGroundPlane = 1;
    pml->walking = 1;

    if (ps->groundEntityNum == ENTITYNUM_NONE) {

        float vz = pml->previous_velocity[2];
        float grav = (float)ps->gravity;
        float halfg = grav * 0.5f;
        float disc = vz * vz +
                     (pml->previous_origin[2] - ps->origin[2]) * (halfg * -4.0f);
        float fallHeight = 0.0f;
        int damage = 0;
        int impactEvent = 0;

        if (disc >= 0.0f) {
            float fallVel = vz + (-vz - sqrtf(disc)) / (halfg + halfg) * grav;
            fallHeight = fallVel * fallVel / (grav + grav);
        }

        if (disc >= 0.0f) {
            float maxH = bg_fallDamageMaxHeight->current.value;
            float minH = bg_fallDamageMinHeight->current.value;

            if (minH >= maxH) {
                Com_Printf("bg_fallDamageMaxHeight must be greater than "
                           "bg_fallDamageMinHeight\n");
                damage = 0;
            } else if (minH < fallHeight &&
                       !(pml->groundTrace.surfaceFlags & 0x1) &&
                       ps->pm_type <= 5) {
                if (fallHeight >= maxH) {
                    damage = 100;
                } else {
                    int d = (int)((fallHeight - minH) / (maxH - minH) * 100.0f);
                    damage = d > 100 ? 100 : d;
                    if (d < 0)
                        damage = 0;
                }
            } else {
                damage = 0;
            }

            if (fallHeight <= 12.0f) {
                impactEvent = 0;
            } else {
                int e = (int)((fallHeight - 12.0f) / 26.0f * 4.0f + 4.0f);
                BG_AnimScriptEvent(ps, 5, 0, 1);
                impactEvent = e >= 25 ? 24 : e;
            }

            if (damage == 0) {
                int surf;

                if (fallHeight <= 4.0f) {

                } else if (fallHeight < 8.0f) {
                    surf = (pml->groundTrace.surfaceFlags & 0x1f00000) >> 0x14;
                    BG_AddPredictableEventToPlayerstate(
                        (!(pml->groundTrace.surfaceFlags & 0x20) && surf != 0)
                            ? surf + 24
                            : 0,
                        0, ps);
                } else if (fallHeight < 12.0f) {
                    surf = (pml->groundTrace.surfaceFlags & 0x1f00000) >> 0x14;
                    BG_AddPredictableEventToPlayerstate(
                        (!(pml->groundTrace.surfaceFlags & 0x20) && surf != 0)
                            ? surf + 1
                            : 0,
                        0, ps);
                } else {
                    ps->velocity[0] *= 0.67f;
                    ps->velocity[1] *= 0.67f;
                    ps->velocity[2] *= 0.67f;
                    surf = (pml->groundTrace.surfaceFlags & 0x20)
                               ? 93
                               : ((pml->groundTrace.surfaceFlags & 0x1f00000) >> 0x14) + 93;
                    BG_AddPredictableEventToPlayerstate(surf, impactEvent, ps);
                }
            } else {
                if (damage <= 99 && !(pml->groundTrace.surfaceFlags & 0x2)) {
                    int t = damage * 0x23 + 0x1f4;
                    ps->pm_time = t > 0x7d0 ? 0x7d0 : t;
                    ps->pm_flags |= 0x200;
                    ps->velocity[0] *= 0.2f;
                    ps->velocity[1] *= 0.2f;
                    ps->velocity[2] *= 0.2f;
                } else {
                    ps->velocity[0] *= 0.67f;
                    ps->velocity[1] *= 0.67f;
                    ps->velocity[2] *= 0.67f;
                }

                impactEvent = (pml->groundTrace.surfaceFlags & 0x20)
                                  ? 116
                                  : ((pml->groundTrace.surfaceFlags & 0x1f00000) >> 0x14) + 116;
                BG_AddPredictableEventToPlayerstate(impactEvent, damage, ps);
            }
        }
    }

    ps->groundEntityNum = trace.entityNum;

    {
        int entityNum = trace.entityNum;
        if (entityNum != 0x3fe) {
            int numtouch = pm->numtouch;
            if (numtouch != 0x20) {
                int j;
                for (j = 0; j < numtouch; j++) {
                    if (pm->touchents[j] == entityNum)
                        return;
                }
                pm->touchents[numtouch] = entityNum;
                pm->numtouch = numtouch + 1;
            }
        }
    }
}

void Pmove(pmove_t *pm)
{
    playerState_t *ps;
    int finalTime;

    if (!pm || !pm->ps)
        return;

    ps = pm->ps;
    finalTime = pm->cmd.serverTime;
#ifdef PM_CMDTRACE
    {
        static int n;
        static int lastB;
        if (pm->cmd.buttons != lastB || (n++ % 240) == 0) {
            fprintf(stderr, "[cmd] buttons=0x%x fwd=%d right=%d weap=%d vhTarget=%d vhCur=%d pm_flags=0x%x pm_type=%d\n",
                    pm->cmd.buttons, pm->cmd.forwardmove, pm->cmd.rightmove, pm->cmd.weapon,
                    ps->viewHeightTarget, ps->viewHeightCurrent, ps->pm_flags, ps->pm_type);
            fflush(stderr);
            lastB = pm->cmd.buttons;
        }
    }
#endif

    if (finalTime < ps->commandTime)
        return;

    if (finalTime > ps->commandTime + 1000)
        ps->commandTime = finalTime - 1000;

    pm->numtouch = 0;
    pm->mantleStarted = 0;

    while (ps->commandTime != finalTime) {
        pml_t pml;
        int msec;

        msec = finalTime - ps->commandTime;
        if (msec > 66)
            msec = 66;
        if (msec < 1)
            msec = 1;

        pm->cmd.serverTime = ps->commandTime + msec;

        /* The animated corpse is a separate entity. A dead player's state
         * must consume commands without running the noclip movement path. */
        if (ps->pm_type == 6 || ps->pm_type == 7) {
            PM_VectorCopy(ps->mins, pm->mins);
            PM_VectorCopy(ps->maxs, pm->maxs);
            pm->mantleStarted = 0;
            ps->velocity[0] = ps->velocity[1] = ps->velocity[2] = 0.0f;
            ps->eFlags &= ~0x40;
            ps->pm_flags &= ~(PMF_LADDER | PMF_SPRINT);
            pm->xyspeed = 0.0f;
            PM_ResetWeaponState(ps);
            ps->commandTime = pm->cmd.serverTime;
            pm->oldcmd = pm->cmd;
            continue;
        }

        BG_AnimUpdatePlayerStateConditions(pm);

        {
            extern int PM_WeaponAmmoAvailable(playerState_t * ps);
            if ((pm->cmd.buttons & 0x4) == 0)
                ps->eFlags &= ~0x200000;
            else
                ps->eFlags |= 0x200000;
            ps->eFlags &= ~0x40;
            if (ps->pm_type != 5) {
                if (!(ps->pm_flags & 0x1000) && (ps->weaponstate == 0 || ps->weaponstate == 3) &&
                    PM_WeaponAmmoAvailable(ps) && (pm->cmd.buttons & 0x1))
                    ps->eFlags |= 0x40;
                if (ps->pm_type <= 5 && (pm->cmd.buttons & 0x4001) == 0)
                    ps->pm_flags &= ~0x1000;
            } else {
                if ((pm->cmd.buttons & 0x4001) == 0)
                    ps->pm_flags &= ~0x1000;
            }
        }

        memset(&pml, 0, sizeof(pml));
        pml.msec = msec;
        pml.frametime = (float)msec * 0.001f;
        PM_VectorCopy(ps->origin, pml.previous_origin);
        PM_VectorCopy(ps->velocity, pml.previous_velocity);
        pml.previous_waterlevel = 0;

        PM_DropTimers(ps, msec);
        PM_AdjustAimSpreadScale(pm, &pml);
        PM_UpdateViewAngles(ps, (float)msec, &pm->cmd, pm->handler);
        AngleVectors(ps->viewangles, pml.forward, pml.right, pml.up);

        PM_CheckDuck(pm, &pml);
        PM_UpdateAimDownSightFlag(pm, &pml);
        ps->pm_flags &= ~PMF_SPRINT;

        if (ps->pm_type <= 5) {
            /* Mantling owns root motion until the ledge animation finishes.
             * Walking/air sliding would cancel that motion against the wall. */
            Mantle_Check(pm, &pml);
            if (ps->pm_flags & PMF_MANTLE) {
                ps->groundEntityNum = ENTITYNUM_NONE;
                ps->pm_flags &= ~PMF_LADDER;
                Mantle_Move(pm, ps, &pml);
            } else {
                PM_GroundTrace(pm, &pml);
                PM_CheckLadderMove(pm, &pml);
                PM_UpdateSprint(pm, &pml);

                if (ps->pm_flags & PMF_LADDER) {
                    PM_LadderMove(pm, &pml);
                } else if (pml.walking && ps->groundEntityNum != ENTITYNUM_NONE) {
                    PM_WalkMove(pm, &pml);
                } else {
                    PM_AirMove(pm, &pml);
                }

                PM_GroundTrace(pm, &pml);
                pm->xyspeed = PM_VectorLength2D(ps->velocity);
                PM_Footsteps(pm, &pml);
            }
        } else {
            ps->groundEntityNum = ENTITYNUM_NONE;
            PM_NoclipMove(pm, &pml);
            PM_ResetWeaponState(ps);
        }

        /* Advance the aim pose once per command step, before weapon logic
         * uses it for firing. Calling it again in PM_Weapon doubles speed. */
        PM_UpdateAimDownSightLerp(pm, &pml);
        if (ps->pm_type <= 5)
            PM_Weapon(pm, &pml);
        PM_ViewHeightAdjust(pm, &pml);

        pm->xyspeed = PM_VectorLength2D(ps->velocity);
        Sys_SnapVector(ps->velocity);

        ps->commandTime = pm->cmd.serverTime;
        pm->oldcmd = pm->cmd;
    }
}

pmoveHandler_t pmoveHandlers[2] = {
    { &CG_TraceCapsule, &CG_PointContents, 0x0 },
    { &G_TraceCapsule, &SV_PointContents, &G_PlayerEvent }
};
viewLerpWaypoint_t viewLerp_CrouchProne[13] = {
    { 0x0, 40.0f, 0x0 },
    { 0xb, 38.0f, 0x0 },
    { 0x16, 33.0f, 0x0 },
    { 0x22, 25.0f, 0x0 },
    { 0x2d, 16.0f, 0x0 },
    { 0x32, 15.0f, 0x0 },
    { 0x37, 16.0f, 0x0 },
    { 0x46, 18.0f, 0x0 },
    { 0x5a, 17.0f, 0x0 },
    { 0x64, 11.0f, 0x0 },
    { 0xffffffff, 0.0f, 0x0 },
    { 0x0, 0.0f, 0x0 },
    { 0x0, 0.0f, 0x0 }
};
viewLerpWaypoint_t viewLerp_CrouchStand[10] = {
    { 0x0, 40.0f, 0x0 },
    { 0x5, 40.5f, 0x0 },
    { 0xa, 41.5f, 0x0 },
    { 0x14, 44.0f, 0x0 },
    { 0x46, 56.0f, 0x0 },
    { 0x60, 58.5f, 0x0 },
    { 0x63, 59.5f, 0x0 },
    { 0x64, 60.0f, 0x0 },
    { 0xffffffff, 0.0f, 0x0 },
    { 0x0, 0.0f, 0x0 }
};
viewLerpWaypoint_t viewLerp_ProneCrouch[8] = {
    { 0x0, 11.0f, 0x0 },
    { 0x5, 10.0f, 0x0 },
    { 0x1e, 21.0f, 0x0 },
    { 0x32, 25.0f, 0x0 },
    { 0x43, 31.0f, 0x0 },
    { 0x53, 34.0f, 0x0 },
    { 0x64, 40.0f, 0x0 },
    { 0xffffffff, 0.0f, 0x0 }
};
viewLerpWaypoint_t viewLerp_StandCrouch[10] = {
    { 0x0, 60.0f, 0x0 },
    { 0x1, 59.5f, 0x0 },
    { 0x4, 58.5f, 0x0 },
    { 0x1e, 56.0f, 0x0 },
    { 0x50, 44.0f, 0x0 },
    { 0x5a, 41.5f, 0x0 },
    { 0x5f, 40.5f, 0x0 },
    { 0x64, 40.0f, 0x0 },
    { 0xffffffff, 0.0f, 0x0 },
    { 0x0, 0.0f, 0x0 }
};
