#include "common_types.h"
#include "imports.h"
#include "bytematch.h"

extern float floorf(float x);

extern void PM_ClipVelocity(const vec_t *in, const vec_t *normal, vec_t *out);
extern void PM_playerTrace(pmove_t *pm, trace_t *results, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int passEntityNum, int contentMask);
extern void PM_AddTouchEnt(pmove_t *pm, int entityNum);
extern const vec_t Vec3NormalizeTo(const vec_t *v, vec_t *out);
extern const vec_t Vec3Normalize(vec_t *v);
extern void Vec3Cross(const vec_t *v0, const vec_t *v1, vec_t *cross);
extern qboolean PM_ShouldMakeFootsteps(pmove_t *pm);
extern void PM_FootstepEvent(pmove_t *pm, pml_t *pml, int iOldBobCycle, int iNewBobCycle, qboolean bFootStep);
extern void BG_AddPredictableEventToPlayerstate(int newEvent, int eventParm, playerState_t *ps);
extern void Jump_ClearState(playerState_t *ps);
extern Bool Jump_IsPlayerAboveMax(playerState_t *ps);
extern Bool Jump_GetStepHeight(playerState_t *ps, const vec_t *origin, float *stepSize);
extern void Jump_ClampVelocity(playerState_t *ps, const vec_t *origin);
extern qboolean BG_CheckProne(int passEntityNum, const vec_t *vPos, const float fSize, const float fHeight, const float fYaw, float *pfTorsoHeight, float *pfTorsoPitch, float *pfWaistPitch, const qboolean bAlreadyProne, const qboolean bOnGround, vec_t *vGroundNormal, int handler, proneCheckType_t proneCheckType, float prone_feet_dist);

#define MAX_CLIP_PLANES 5
#define PM_SLIDEMOVE_ABI COD2_REGPARM(3) BM_NOINLINE

static qboolean PM_SLIDEMOVE_ABI PM_SlideMove(pmove_t *pm, pml_t *pml, qboolean gravity);
void PM_StepSlideMove(pmove_t *pm, pml_t *pml, qboolean gravity);

static qboolean PM_SLIDEMOVE_ABI PM_SlideMove(pmove_t *pm, pml_t *pml, qboolean gravity)
{
    playerState_t *ps;
    float time_left;
    int bumpcount;
    int numplanes;
    vec3_t planes[MAX_CLIP_PLANES];
    vec3_t endVelocity;
    vec3_t end;
    trace_t trace;
    int i, j;
    vec3_t clipVelocity;
    vec3_t endClipVelocity;
    vec3_t dir;
    float d;
    float into;
    int permutation[MAX_CLIP_PLANES];
    float dots[MAX_CLIP_PLANES];
    static int traceEnabled = -1;

    if (traceEnabled < 0)
        traceEnabled = getenv("PTRACE") != NULL;

    ps = pm->ps;

    endVelocity[0] = ps->velocity[0];
    endVelocity[1] = ps->velocity[1];
    endVelocity[2] = ps->velocity[2];

    if (gravity) {
        endVelocity[2] -= (float)ps->gravity * pml->frametime;
        ps->velocity[2] = (endVelocity[2] + ps->velocity[2]) * 0.5f;

        if (pml->groundPlane) {
            PM_ClipVelocity(ps->velocity, pml->groundTrace.normal, ps->velocity);
        }
    }

    time_left = pml->frametime;

    if (pml->groundPlane) {
        numplanes = 2;
        planes[0][0] = pml->groundTrace.normal[0];
        planes[0][1] = pml->groundTrace.normal[1];
        planes[0][2] = pml->groundTrace.normal[2];
        Vec3NormalizeTo(ps->velocity, planes[1]);
    } else {
        numplanes = 1;
        Vec3NormalizeTo(ps->velocity, planes[0]);
    }

    for (bumpcount = 0; bumpcount < 4; bumpcount++) {
        end[0] = ps->origin[0] + time_left * ps->velocity[0];
        end[1] = ps->origin[1] + time_left * ps->velocity[1];
        end[2] = ps->origin[2] + time_left * ps->velocity[2];

        PM_playerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs, end, ps->clientNum, pm->tracemask);

        if (traceEnabled) {
        if (trace.fraction < 1.0f) {
            static int cn;
            if (cn++ < 80)
                fprintf(stderr, "[hit] bump=%d frac=%.3f norm=(%.2f,%.2f,%.2f) vel=(%.0f,%.0f,%.0f) "
                                "contents=0x%x surf=0x%x ent=%d mat='%s' nplanes=%d\n",
                        bumpcount, trace.fraction, trace.normal[0], trace.normal[1], trace.normal[2],
                        ps->velocity[0], ps->velocity[1], ps->velocity[2],
                        trace.contents, trace.surfaceFlags, trace.entityNum,
                        trace.material ? trace.material : "(null)", numplanes);
            fflush(stderr);
        }
        }

        if (trace.allsolid) {
            if (traceEnabled) {
            {
                static int n;
                if (n++ < 40)
                    fprintf(stderr, "[stuck] allsolid org=(%.1f,%.1f,%.1f) vel=(%.1f,%.1f,%.1f) "
                                    "frac=%.3f startsolid=%d contents=0x%x surf=0x%x ent=%d mat='%s'\n",
                            ps->origin[0], ps->origin[1], ps->origin[2],
                            ps->velocity[0], ps->velocity[1], ps->velocity[2],
                            trace.fraction, trace.startsolid, trace.contents, trace.surfaceFlags,
                            trace.entityNum, trace.material ? trace.material : "(null)");
                fflush(stderr);
            }
            }
            ps->velocity[2] = 0;
            return 1;
        }

        if (trace.fraction > 0.0f) {
            ps->origin[0] += (end[0] - ps->origin[0]) * trace.fraction;
            ps->origin[1] += (end[1] - ps->origin[1]) * trace.fraction;
            ps->origin[2] += (end[2] - ps->origin[2]) * trace.fraction;
        }

        if (trace.fraction == 1.0f) {
            break;
        }

        PM_AddTouchEnt(pm, trace.entityNum);

        time_left -= time_left * trace.fraction;

        /* Stop before adding a plane beyond all three fixed-size arrays. */
        if (numplanes >= MAX_CLIP_PLANES) {
            ps->velocity[0] = 0;
            ps->velocity[1] = 0;
            ps->velocity[2] = 0;
            return 1;
        }

        for (i = 0; i < numplanes; i++) {
            if (trace.normal[0] * planes[i][0] +
                    trace.normal[1] * planes[i][1] +
                    trace.normal[2] * planes[i][2] >
                0.999f) {
                ps->velocity[0] = trace.normal[0] + ps->velocity[0];
                ps->velocity[1] = trace.normal[1] + ps->velocity[1];
                ps->velocity[2] = trace.normal[2] + ps->velocity[2];
                break;
            }
        }
        if (i < numplanes) {
            continue;
        }

        planes[numplanes][0] = trace.normal[0];
        planes[numplanes][1] = trace.normal[1];
        planes[numplanes][2] = trace.normal[2];
        numplanes++;

        {
            int k;
            float dot;

            for (k = 0; k < numplanes; k++) {
                dot = ps->velocity[0] * planes[k][0] +
                      ps->velocity[1] * planes[k][1] +
                      ps->velocity[2] * planes[k][2];
                dots[k] = dot;

                if (k == 0) {
                    permutation[k] = k;
                } else {
                    int pos = k;
                    int prev = pos - 1;

                    while (prev >= 0 && dot < dots[permutation[prev]]) {
                        permutation[pos] = permutation[prev];
                        pos = prev;
                        prev--;
                    }
                    permutation[pos] = k;
                }
            }
        }

        d = dots[permutation[0]];
        if (d >= 0.1f) {
            continue;
        }

        into = -d;
        if (into > pml->impactSpeed) {
            pml->impactSpeed = into;
        }

        i = permutation[0];
        PM_ClipVelocity(ps->velocity, planes[i], clipVelocity);
        PM_ClipVelocity(endVelocity, planes[i], endClipVelocity);

        for (j = 1; j < numplanes; j++) {
            if (clipVelocity[0] * planes[permutation[j]][0] +
                    clipVelocity[1] * planes[permutation[j]][1] +
                    clipVelocity[2] * planes[permutation[j]][2] >=
                0.1f) {
                continue;
            }

            PM_ClipVelocity(clipVelocity, planes[permutation[j]], clipVelocity);
            PM_ClipVelocity(endClipVelocity, planes[permutation[j]], endClipVelocity);

            if (clipVelocity[0] * planes[permutation[0]][0] +
                    clipVelocity[1] * planes[permutation[0]][1] +
                    clipVelocity[2] * planes[permutation[0]][2] >=
                0.0f) {
                continue;
            }

            Vec3Cross(planes[permutation[0]], planes[permutation[j]], dir);
            Vec3Normalize(dir);

            d = dir[0] * ps->velocity[0] +
                dir[1] * ps->velocity[1] +
                dir[2] * ps->velocity[2];

            clipVelocity[0] = dir[0] * d;
            clipVelocity[1] = dir[1] * d;
            clipVelocity[2] = dir[2] * d;

            d = dir[0] * endVelocity[0] +
                dir[1] * endVelocity[1] +
                dir[2] * endVelocity[2];

            endClipVelocity[0] = dir[0] * d;
            endClipVelocity[1] = dir[1] * d;
            endClipVelocity[2] = dir[2] * d;

            {
                int k;
                for (k = 1; k < numplanes; k++) {
                    if (k == j) {
                        continue;
                    }
                    if (clipVelocity[0] * planes[permutation[k]][0] +
                            clipVelocity[1] * planes[permutation[k]][1] +
                            clipVelocity[2] * planes[permutation[k]][2] >=
                        0.1f) {
                        continue;
                    }
                    ps->velocity[0] = 0;
                    ps->velocity[1] = 0;
                    ps->velocity[2] = 0;
                    return 1;
                }
            }
            break;
        }

        ps->velocity[0] = clipVelocity[0];
        ps->velocity[1] = clipVelocity[1];
        ps->velocity[2] = clipVelocity[2];
        endVelocity[0] = endClipVelocity[0];
        endVelocity[1] = endClipVelocity[1];
        endVelocity[2] = endClipVelocity[2];
    }

    if (gravity) {
        ps->velocity[0] = endVelocity[0];
        ps->velocity[1] = endVelocity[1];
        ps->velocity[2] = endVelocity[2];
    }

    if (ps->pm_time) {
        ps->velocity[0] = endVelocity[0];
        ps->velocity[1] = endVelocity[1];
        ps->velocity[2] = endVelocity[2];
    }

    return (bumpcount != 0);
}

static void PM_ApplyStepEvent(pmove_t *pm, pml_t *pml, playerState_t *ps,
                              const vec_t *start_o, float fStepSize)
{
    float stepDelta;
    int rounded;
    int stepEvent;

    stepDelta = ps->origin[2] - start_o[2];

    {
        float absDelta = stepDelta;
        if (absDelta < 0.0f)
            absDelta = -absDelta;
        if (absDelta <= 0.5f) {
            return;
        }
    }

    rounded = (int)floorf(stepDelta + 0.5f);

    if (rounded == 0) {
        return;
    }

    if (rounded < -16) {
        stepEvent = 0x70;
    } else if (rounded > 24) {
        stepEvent = 0x98;
    } else {
        stepEvent = rounded + 0x80;
    }

    BG_AddPredictableEventToPlayerstate(0x8f, stepEvent, ps);

    {
        float frac, scale;

        frac = ps->origin[2] - start_o[2];
        if (frac < 0.0f)
            frac = -frac;
        frac = frac / fStepSize;
        scale = (1.0f - frac) * 0.8f + 0.2f;

        ps->velocity[0] *= scale;
        ps->velocity[1] *= scale;
        ps->velocity[2] *= scale;
    }

    if (stepEvent < 0) {
        stepEvent = -stepEvent;
    }

    if (stepEvent <= 3) {
        return;
    }
    if (ps->groundEntityNum == 0x3ff) {
        return;
    }
    if (!PM_ShouldMakeFootsteps(pm)) {
        return;
    }

    {
        float vol;
        int oldBob, newBob;

        vol = (float)(stepEvent / 2);
        if (vol > 4.0f) {
            vol = 4.0f;
        }

        oldBob = ps->bobCycle;
        newBob = (int)((float)oldBob + vol * 1.25f + 7.0f) & 0xff;
        ps->bobCycle = newBob;

        PM_FootstepEvent(pm, pml, oldBob, newBob, 1);
    }
}

static qboolean PM_StepCheckProne(pmove_t *pm, playerState_t *ps,
                                  const vec_t *start_o, const vec_t *start_v)
{
    if (ps->pm_flags & 1) {
        if (!BG_CheckProne(
                ps->clientNum,
                ps->origin,
                30.0f,
                ps->maxs[2],
                ps->proneDirection,
                &ps->fTorsoHeight,
                &ps->fTorsoPitch,
                &ps->fWaistPitch,
                1,
                1,
                ps->vLadderVec,
                pm->handler,
                0,
                66.0f)) {
            ps->origin[0] = start_o[0];
            ps->origin[1] = start_o[1];
            ps->origin[2] = start_o[2];

            ps->velocity[0] = start_v[0];
            ps->velocity[1] = start_v[1];
            ps->velocity[2] = start_v[2];

            return 0;
        }
    }
    return 1;
}

void PM_StepSlideMove(pmove_t *pm, pml_t *pml, qboolean gravity)
{
    playerState_t *ps;
    vec3_t start_o, start_v;
    vec3_t down_o, down_v;
    vec3_t up, down;
    trace_t trace;
    float stepSize;
    float fStepSize;
    int iBumps;
    qboolean bHadGround;
    qboolean jumping;
    float delta, delta2;

    ps = pm->ps;

    if (ps->pm_flags & 0x20) {
        Jump_ClearState(ps);
        bHadGround = 0;
    } else if (pml->groundPlane) {
        bHadGround = 1;
    } else {
        if ((ps->pm_flags & 0x80000) && ps->pm_time) {
            Jump_ClearState(ps);
        }
        bHadGround = 0;
    }

    start_o[0] = ps->origin[0];
    start_o[1] = ps->origin[1];
    start_o[2] = ps->origin[2];

    start_v[0] = ps->velocity[0];
    start_v[1] = ps->velocity[1];
    start_v[2] = ps->velocity[2];

    iBumps = PM_SlideMove(pm, pml, gravity);

    if (ps->pm_flags & 1) {
        fStepSize = 10.0f;
    } else {
        fStepSize = 18.0f;
    }

    if (ps->groundEntityNum == 0x3ff) {

        if (ps->pm_flags & 0x80000) {
            if (ps->pm_time) {
                Jump_ClearState(ps);
            }
        }

        if (iBumps && (ps->pm_flags & 0x80000)) {
            if (!Jump_GetStepHeight(ps, start_o, &fStepSize)) {
                if (!(ps->pm_flags & 0x20)) {
                    return;
                }
                if (ps->velocity[2] > 0.0f) {
                    jumping = 0;
                    goto do_step;
                }
                return;
            }
            if (fStepSize < 1.0f) {
                return;
            }
            jumping = 1;
            goto do_step;
        }

        if (!(ps->pm_flags & 0x20)) {
            return;
        }
        if (ps->velocity[2] > 0.0f) {
            jumping = 0;
            goto do_step;
        }
        return;
    }

    jumping = 0;

do_step:

    down_o[0] = ps->origin[0];
    down_o[1] = ps->origin[1];
    down_o[2] = ps->origin[2];

    down_v[0] = ps->velocity[0];
    down_v[1] = ps->velocity[1];
    down_v[2] = ps->velocity[2];

    delta = ps->origin[0] - start_o[0];
    delta2 = ps->origin[1] - start_o[1];

    if (iBumps) {

        float z;

        up[0] = start_o[0];
        up[1] = start_o[1];
        up[2] = start_o[2] + 1.0f + fStepSize;

        PM_playerTrace(pm, &trace, start_o, pm->mins, pm->maxs, up, ps->clientNum, pm->tracemask);

        stepSize = (1.0f + fStepSize) * trace.fraction - 1.0f;

        if (stepSize <= 1.0f) {
            stepSize = 0.0f;
        } else {
            z = stepSize + start_o[2];

            ps->origin[0] = up[0];
            ps->origin[1] = up[1];
            ps->origin[2] = z;

            ps->velocity[0] = start_v[0];
            ps->velocity[1] = start_v[1];
            ps->velocity[2] = start_v[2];

            PM_SlideMove(pm, pml, gravity);
        }
    } else {
        stepSize = 0.0f;
    }

    if (!bHadGround && stepSize == 0.0f) {
        goto check_velocity_change;
    }

    down[0] = ps->origin[0];
    down[1] = ps->origin[1];
    down[2] = ps->origin[2] - stepSize;

    if (bHadGround) {
        down[2] -= 9.0f;
    }

    PM_playerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs, down, ps->clientNum, pm->tracemask);

    /* Reject stepping onto a player; world geometry is ENTITYNUM_WORLD. */
    if (trace.entityNum < 64) {

        ps->origin[0] = down_o[0];
        ps->origin[1] = down_o[1];
        ps->origin[2] = down_o[2];

        ps->velocity[0] = down_v[0];
        ps->velocity[1] = down_v[1];
        ps->velocity[2] = down_v[2];

        return;
    }

    if (trace.fraction < 1.0f) {

        if (trace.normal[2] < 0.3f) {

            ps->origin[0] = down_o[0];
            ps->origin[1] = down_o[1];
            ps->origin[2] = down_o[2];

            ps->velocity[0] = down_v[0];
            ps->velocity[1] = down_v[1];
            ps->velocity[2] = down_v[2];

            return;
        }

        ps->origin[0] += (down[0] - ps->origin[0]) * trace.fraction;
        ps->origin[1] += (down[1] - ps->origin[1]) * trace.fraction;
        ps->origin[2] += (down[2] - ps->origin[2]) * trace.fraction;

        PM_ClipVelocity(ps->velocity, trace.normal, ps->velocity);
    } else {

        if (stepSize == 0.0f) {
            goto check_velocity_change;
        }

        ps->origin[2] -= stepSize;
    }

check_velocity_change: {
    float up_dot, down_dot;

    up_dot = (ps->origin[0] - start_o[0]) * ps->velocity[0] +
             (ps->origin[1] - start_o[1]) * ps->velocity[1];

    down_dot = delta * ps->velocity[0] +
               delta2 * ps->velocity[1];

    down_dot += 0.001f;

    if (down_dot >= up_dot) {

        goto restore_down;
    }
}

    if (jumping) {
        if (Jump_IsPlayerAboveMax(ps)) {
            goto restore_down;
        }
        Jump_ClampVelocity(ps, down_o);
    }

    goto check_landing;

restore_down:
    ps->origin[0] = down_o[0];
    ps->origin[1] = down_o[1];
    ps->origin[2] = down_o[2];

    ps->velocity[0] = down_v[0];
    ps->velocity[1] = down_v[1];
    ps->velocity[2] = down_v[2];

    if (bHadGround) {
        down[0] = ps->origin[0];
        down[1] = ps->origin[1];
        down[2] = ps->origin[2] - 9.0f;

        PM_playerTrace(pm, &trace, ps->origin, pm->mins, pm->maxs, down, ps->clientNum, pm->tracemask);

        if (trace.fraction < 1.0f) {
            ps->origin[0] += (down[0] - ps->origin[0]) * trace.fraction;
            ps->origin[1] += (down[1] - ps->origin[1]) * trace.fraction;
            ps->origin[2] += (down[2] - ps->origin[2]) * trace.fraction;

            PM_ClipVelocity(ps->velocity, trace.normal, ps->velocity);
        }
    }

    if (jumping) {
        Jump_ClampVelocity(ps, down_o);
    }

check_landing:
    if (!bHadGround) {
        return;
    }
    if (ps->pm_type > 5) {
        return;
    }

    if (!PM_StepCheckProne(pm, ps, start_o, start_v)) {
        return;
    }

    PM_ApplyStepEvent(pm, pml, ps, start_o, fStepSize);
}
