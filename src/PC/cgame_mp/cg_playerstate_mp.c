#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_local.h"

extern const dvar_t *cg_hudDamageIconTime;

extern char *cg_globUI[];
extern void *imp_legacyHacks;
extern void *imp_cg_hudDamageIconTime;

extern void AngleVectors(const vec_t *angles, vec_t *forward, vec_t *right, vec_t *up);
extern const float AngleNormalize360(const float angle);
extern float randomf(void);
extern void CL_SetADS(int ads);
extern void CG_SetEquippedOffHand(int offHandIndex);
extern void CG_HoldBreathInit(void);
extern void CG_ResetLowHealthOverlay(void);
extern void CG_MenuShowNotify(int menuToShow);
extern void CG_EntityEvent(centity_t *cent, int event);

void CG_Respawn(void);
void CG_DamageFeedback(int yawByte, int pitchByte, int damage);
void CG_TransitionPlayerState(playerState_t *ps, playerState_t *ops);

#define CG_SNAP_PTR 0x20
#define CG_OFF_25BB0 0x25bb0
#define CG_OFF_25BBC 0x25bbc
#define CG_OFF_25BC4 0x25bc4
#define CG_OFF_25C94 0x25c94
#define CG_OFF_25C98 0x25c98
#define CG_OFF_28490 0x28490
#define CG_OFF_284C4 0x284c4
#define CG_OFF_285D4 0x285d4
#define CG_OFF_285E0 0x285e0
#define CG_OFF_285EC 0x285ec
#define CG_OFF_28594 0x28594
#define CG_OFF_285A0 0x285a0
#define CG_OFF_2826C 0x2826c
#define CG_OFF_2BDE8 0x2bde8
#define CG_OFF_2BDEC 0x2bdec
#define CG_OFF_2BE30 0x2be30
#define CG_OFF_2BE48 0x2be48
#define CG_OFF_2BE50 0x2be50
#define CG_OFF_2BE54 0x2be54
#define CG_OFF_2BE70 0x2be70
#define CG_OFF_2BE74 0x2be74
#define CG_OFF_2BE80 0x2be80
#define CG_OFF_2BED4 0x2bed4
#define CG_OFF_2BF0C 0x2bf0c
#define CG_OFF_2BF10 0x2bf10
#define CG_OFF_2BF14 0x2bf14
#define CG_OFF_2BF1C 0x2bf1c
#define CG_OFF_2C030 0x2c030
#define CG_OFF_2C03C 0x2c03c
#define CG_OFF_2C0A4 0x2c0a4
#define CG_OFF_2C0B0 0x2c0b0
#define CG_OFF_2C50C 0x2c50c
#define CG_OFF_2CD10 0x2cd10

#define CENT_EVENT_PARM 0x190

#define CG_INT(base, off) (*(int *)((char *)(base) + (off)))
#define CG_FLOAT(base, off) (*(float *)((char *)(base) + (off)))
#define CG_PTR(base, off) (*(char **)((char *)(base) + (off)))

static inline __attribute__((always_inline)) void VectorClear3Int(void *base, int off)
{
    float *v = (float *)((char *)base + off);
    v[0] = 0.0f;
    v[1] = 0.0f;
    v[2] = 0.0f;
}

void CG_Respawn(void)
{
    char *snap;
    extern void CG_ResetDeathView(void);

    CG_ResetDeathView();

    memset(cg->kickAVel, 0, sizeof(cg->kickAVel));
    memset(cg->kickAngles, 0, sizeof(cg->kickAngles));

    CG_INT(cg, CG_OFF_25BBC) = 0;

    snap = CG_PTR(cg, CG_SNAP_PTR);
    memcpy((char *)cg + CG_OFF_25BC4, snap + 0xc, sizeof(playerState_t));

    CG_INT(cg, CG_OFF_2BE50) = CG_INT(cg, CG_OFF_25C98);

    CG_INT(cg, CG_OFF_2BE54) = CG_INT(cg, CG_OFF_25BB0);

    CG_INT(cg, CG_OFF_2BE70) = CG_INT(cg, CG_OFF_25C94);

    CG_INT(cg, CG_OFF_2BDE8) = 0;
    CG_INT(cg, CG_OFF_2BDEC) = 0;
    CG_INT(cg, CG_OFF_2BE30) = 0;

    VectorClear3Int(cg, CG_OFF_285D4);
    VectorClear3Int(cg, CG_OFF_285E0);
    VectorClear3Int(cg, CG_OFF_285EC);

    VectorClear3Int(cg, CG_OFF_2C03C);
    VectorClear3Int(cg, CG_OFF_2C030);

    CG_FLOAT(cg, CG_OFF_2BF1C) = 0.0f;

    memset((char *)cg + CG_OFF_28490, 0, 48);

    CG_INT(cg, CG_OFF_2BED4) = 0;

    CG_FLOAT(cg, CG_OFF_2BF10) = 0.0f;
    CG_FLOAT(cg, CG_OFF_2BF14) = 0.0f;

    VectorClear3Int(cg, CG_OFF_2C0A4);
    VectorClear3Int(cg, CG_OFF_2C0B0);

    memset((char *)cg + CG_OFF_2BE74, 0, 0x60);

    memset((char *)cg + CG_OFF_2C50C, 0, 0x90);

    VectorClear3Int(cg, CG_OFF_284C4);

    CG_INT(cg, CG_OFF_2CD10) = 0;

    {
        char *uiBase = *(char **)imp_legacyHacks;
        CG_INT(uiBase, 8) = 0;
    }

    CL_SetADS(0);

    CG_SetEquippedOffHand(CG_INT(cg, CG_OFF_25C94));

    CG_HoldBreathInit();

    CG_ResetLowHealthOverlay();
}

void CG_DamageFeedback(int yawByte, int pitchByte, int damage)
{
    float count;
    float negCount;
    vec3_t dir;
    vec3_t angles;
    float dot;
    int slot;
    int i;
    char *snap;

    CG_INT(cg, CG_OFF_2BE48) = CG_INT(cg, CG_OFF_25BB0);

    count = (float)damage * 0.2f;

    if (count < 5.0f) {
        count = 5.0f;
        negCount = -5.0f;
    } else if (count > 90.0f) {
        count = 90.0f;
        negCount = -90.0f;
    } else {
        negCount = -count;
    }

    if (yawByte == 255 && pitchByte == 255) {

        CG_FLOAT(cg, CG_OFF_2BF14) = 0.0f;
        CG_FLOAT(cg, CG_OFF_2BF10) = negCount;
    } else {

        float yaw = (float)yawByte / 255.0f * 360.0f;

        float pitch = (float)pitchByte / 255.0f * 360.0f;

        angles[0] = pitch;
        angles[1] = yaw;
        angles[2] = 0.0f;

        AngleVectors(angles, dir, NULL, NULL);

        {
            float *up = (float *)((char *)cg + CG_OFF_285A0);
            float *right = (float *)((char *)cg + CG_OFF_28594);

            dot = dir[0] * up[0] + dir[1] * up[1] + dir[2] * up[2];
            CG_FLOAT(cg, CG_OFF_2BF14) = dot * negCount;

            dot = dir[0] * right[0] + dir[1] * right[1] + dir[2] * right[2];
            CG_FLOAT(cg, CG_OFF_2BF10) = dot * count;
        }

        slot = 0;
        {

            char *p = (char *)cg;
            for (i = 1; i < 8; i++) {
                p += 12;
                if (CG_INT(p, CG_OFF_2BE74) < CG_INT(cg, CG_OFF_2BE74 + slot * 12)) {
                    slot = i;
                }
            }
        }

        snap = CG_PTR(cg, CG_SNAP_PTR);
        CG_INT(cg, CG_OFF_2BE74 + slot * 12) = CG_INT(snap, 8);

        {
            const dvar_t *damageIconTime = cg_hudDamageIconTime;
            CG_INT(cg, CG_OFF_2BE74 + slot * 12 + 4) =
                damageIconTime ? damageIconTime->current.integer : 2000;
        }

        {
            float r = randomf();
            float jitteredYaw = yaw + (r - 0.5f) * 20.0f;
            float normYaw = AngleNormalize360(jitteredYaw);
            CG_FLOAT(cg, CG_OFF_2BE74 + slot * 12 + 8) = normYaw;
        }

    }

    CG_INT(cg, CG_OFF_2BF0C) = CG_INT(cg, CG_OFF_25BB0) + 500;

    snap = CG_PTR(cg, CG_SNAP_PTR);
    CG_INT(cg, CG_OFF_2BED4) = CG_INT(snap, 8);

    CG_MenuShowNotify(0);
}

void CG_TransitionPlayerState(playerState_t *ps, playerState_t *ops)
{
    char *cent;
    int i;
    int idx;
    int event;

    if (ps->damageEvent != ops->damageEvent) {

        if (ps->damageCount != 0) {
            CG_DamageFeedback(ps->damageYaw, ps->damagePitch, ps->damageCount);
        }
    }

    cent = (char *)cg + CG_OFF_2826C;

    i = ps->eventSequence - 4;

    while (i < ps->eventSequence) {

        if (i < ops->eventSequence) {

            if (i < ops->eventSequence - 4) {
                i++;
                continue;
            }
            idx = i & 3;
            event = ps->events[idx];
            if (event == ops->events[idx]) {
                i++;
                continue;
            }
        } else {

            idx = i & 3;
            event = ps->events[idx];
        }

        CG_INT(cent, CENT_EVENT_PARM) = ps->eventParms[idx];

        CG_EntityEvent((centity_t *)cent, event);

        i++;
    }
}
