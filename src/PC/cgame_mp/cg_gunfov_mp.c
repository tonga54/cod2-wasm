#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include "headers/PC/cgame_mp/cg_gunfov.h"
#include <math.h>
#include <string.h>

/* A Reflex or Lens Sight's window opens with the view's projection at this
 * part of the raise (cg_wwiiscope_mp.c's CG_WWIISCOPE_WINDOW_START). */
#define CG_GUNFOV_RAISED 0.6f

/* Call of Duty: WWII holds a long gun nearer the eye and further right than
 * Call of Duty 2's viewmodels stand, so its barrel runs along the view from
 * the lower right and the hand on the stock takes the forearm's place,
 * turned a little in and canted so its left side shows: its pose at
 * cg_gunHip 1, a little up, as the narrower field of view draws the weapon
 * lower on screen. Call of Duty 2's scopes reach back nearly to the eye: a
 * scoped gun stands further out, its nose turned in and canted a little, so
 * the rifle crosses from the lower right toward the middle. Neither is
 * pitched: a nose turned down reads as a weapon pointed at the ground. A
 * pistol keeps Call of Duty 2's own pose; drawn nearer the middle it read as
 * small and too close to the eye. */
static const float s_hipLongGun[CG_GUNFOV_HIP_POSE] = {-3.0f, -1.5f, 0.75f, 0.0f, -2.0f, -4.0f};
static const float s_hipScoped[CG_GUNFOV_HIP_POSE] = {3.0f, -0.75f, 1.5f, 0.0f, -6.0f, -4.0f};

/* Between weapons the hip offset eases over about this many seconds. */
#define CG_GUNFOV_HIP_EASE 0.1f

float g_webGunProjection[2];

/* How far the raise has taken the weapon toward the view's own projection
 * and place: 0 at the hip, 1 once a sight's window opens. */
static float CG_GunFovRaised(float raise)
{
    float blend = raise > 0.0f ? raise / CG_GUNFOV_RAISED : 0.0f;

    if (blend >= 1.0f)
        return 1.0f;
    return blend * blend * (3.0f - 2.0f * blend);
}

float CG_GunFovProjection(float gunFov, float viewFovX, float viewFovY, float raise, float *projection)
{
    float viewX, viewY, gun, blend, tanY;

    projection[0] = projection[1] = 0.0f;
    if (!(gunFov >= 1.0f && gunFov < 179.0f) || !(viewFovX > 0.0f && viewFovX < 179.0f) ||
        !(viewFovY > 0.0f && viewFovY < 179.0f))
        return 1.0f;
    blend = CG_GunFovRaised(raise);
    if (blend >= 1.0f)
        return 1.0f;
    viewX = tanf(viewFovX * 0.00872664626f);
    viewY = tanf(viewFovY * 0.00872664626f);
    /* CG_CalcFov's vertical half-angle for a cg_fov-style angle. */
    gun = tanf(gunFov * 0.00872664626f) * 0.75f;
    tanY = gun + (viewY - gun) * blend;
    projection[0] = viewY / (viewX * tanY);
    projection[1] = 1.0f / tanY;
    return viewY / tanY;
}

void CG_GunFovHipTarget(int weapClass, int scoped, float scale, float *pose)
{
    const float *hip = NULL;
    int i;

    switch (weapClass) {
    case WEAPCLASS_RIFLE:
    case WEAPCLASS_MG:
    case WEAPCLASS_SMG:
    case WEAPCLASS_SPREAD:
    case WEAPCLASS_ROCKETLAUNCHER:
        hip = scoped ? s_hipScoped : s_hipLongGun;
        break;
    default:
        break;
    }
    if (!(scale > 0.0f) || !isfinite(scale))
        hip = NULL;
    for (i = 0; i < CG_GUNFOV_HIP_POSE; i++)
        pose[i] = hip ? hip[i] * (scale < 1.0f ? scale : 1.0f) : 0.0f;
}

float CG_GunFovHipShare(float raise)
{
    return 1.0f - CG_GunFovRaised(raise);
}

#ifdef __EMSCRIPTEN__

extern const dvar_t *Dvar_RegisterFloat(const char *name, float value, float min, float max, int flags);
extern int WebSceneFx_ScopeAvailable(void);

const dvar_t *cg_gunFov;
const dvar_t *cg_gunHip;
static float s_spread = 1.0f;
static vec3_t s_eye;
static vec3_t s_axis[3];
static float s_hip[CG_GUNFOV_HIP_POSE];
static int s_hipHeld;

void CG_GunFovInit(void)
{
    /* Call of Duty: WWII's own field of view. */
    cg_gunFov = Dvar_RegisterFloat("cg_gunFov", 65.0f, 0.0f, 120.0f, 0x1001);
    cg_gunHip = Dvar_RegisterFloat("cg_gunHip", 1.0f, 0.0f, 1.0f, 0x1001);
    s_hipHeld = 0;
    CG_GunFovClear();
}

void CG_GunFovClear(void)
{
    g_webGunProjection[0] = g_webGunProjection[1] = 0.0f;
    s_spread = 1.0f;
}

/* The compositor draws the viewmodel with its own projection only in a view
 * that fills the screen, and only while it composes. */
static int CG_GunFovComposed(void)
{
    const vidConfig_t *vid = &((clientStatic_t *)imp_cls)->vidConfig;

    return !cg->refdef.x && !cg->refdef.y && cg->refdef.width == (int)vid->width &&
           cg->refdef.height == (int)vid->height && WebSceneFx_ScopeAvailable();
}

void CG_GunFovUpdate(const playerState_t *ps, qboolean drawGun)
{
    CG_GunFovClear();
    memcpy(s_eye, cg->refdef.vieworg, sizeof(s_eye));
    memcpy(s_axis, cg->refdef.viewaxis, sizeof(s_axis));
    if (!drawGun || cg->renderingThirdPerson || !cg_gunFov || !CG_GunFovComposed())
        return;
    s_spread = CG_GunFovProjection(cg_gunFov->current.value, cg->refdef.fov_x, cg->refdef.fov_y, ps->fWeaponPosFrac,
                                   g_webGunProjection);
}

void CG_GunFovFollowScope(const float *projection)
{
    float viewY = tanf(cg->refdef.fov_y * 0.00872664626f);

    if (projection && projection[0] > 0.0f && projection[1] > 0.0f && viewY > 0.0f && isfinite(viewY))
        s_spread = projection[1] * viewY;
}

float CG_GunFovHipAngle(float viewHipFov)
{
    return cg_gunFov && cg_gunFov->current.value >= 1.0f ? cg_gunFov->current.value : viewHipFov;
}

void CG_GunFovPoint(vec_t *point)
{
    float offset[3], forward = 0.0f, left = 0.0f, up = 0.0f;
    int i;

    if (s_spread == 1.0f)
        return;
    for (i = 0; i < 3; i++) {
        offset[i] = point[i] - s_eye[i];
        forward += offset[i] * s_axis[0][i];
        left += offset[i] * s_axis[1][i];
        up += offset[i] * s_axis[2][i];
    }
    for (i = 0; i < 3; i++)
        point[i] = s_eye[i] + forward * s_axis[0][i] + s_spread * (left * s_axis[1][i] + up * s_axis[2][i]);
}

void CG_GunFovHipPose(const playerState_t *ps, const WeaponDef *weapDef, float *pose)
{
    float target[CG_GUNFOV_HIP_POSE], ease = 1.0f, share = CG_GunFovHipShare(ps->fWeaponPosFrac);
    int i;

    CG_GunFovHipTarget(weapDef ? (int)weapDef->weapClass : -1,
                       weapDef && weapDef->overlayReticle && weapDef->szOverlayMaterial && weapDef->szOverlayMaterial[0],
                       cg_gunHip ? cg_gunHip->current.value : 0.0f, target);
    if (s_hipHeld)
        ease = cg->frametime > 0 ? 1.0f - expf(-0.001f * (float)cg->frametime / CG_GUNFOV_HIP_EASE) : 0.0f;
    for (i = 0; i < CG_GUNFOV_HIP_POSE; i++) {
        s_hip[i] += (target[i] - s_hip[i]) * ease;
        pose[i] = s_hip[i] * share;
    }
    s_hipHeld = 1;
}

#else

void CG_GunFovInit(void)
{
}

void CG_GunFovUpdate(const playerState_t *ps, qboolean drawGun)
{
    (void)ps;
    (void)drawGun;
}

void CG_GunFovFollowScope(const float *projection)
{
    (void)projection;
}

void CG_GunFovClear(void)
{
}

float CG_GunFovHipAngle(float viewHipFov)
{
    return viewHipFov;
}

void CG_GunFovPoint(vec_t *point)
{
    (void)point;
}

void CG_GunFovHipPose(const playerState_t *ps, const WeaponDef *weapDef, float *pose)
{
    int i;

    (void)ps;
    (void)weapDef;
    for (i = 0; i < CG_GUNFOV_HIP_POSE; i++)
        pose[i] = 0.0f;
}

#endif
