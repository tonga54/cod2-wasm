#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include "headers/PC/cgame_mp/cg_wwiiscope.h"
#include "headers/PC/gfx_d3d/r_hud2d.h"
#include <math.h>
#include <string.h>

/* Each scope's glass in the space of the gun's j_gun bone, read from the
 * sniper viewmodels' xmodelsurfs: the rim of the glass dome nearest the
 * muzzle (+x), centred on the scope's axis. The Mosin-Nagant's glass is the
 * inner disk within its brass ring. The eye positions and cants were measured
 * in a match with each rifle at full aim and at rest. */
const CgWwiiScopeGlass cg_wwiiScopeGlasses[CG_WWIISCOPE_GLASS_COUNT] = {
    { "viewmodel_kar98_scope", { -6.116f, -0.057f, 2.996f }, 0.612f, { 1.778f, -0.044f, 0.017f }, 1.74f },
    { "viewmodel_springfield", { -5.968f, 0.255f, 3.675f }, 0.578f, { 2.942f, 0.064f, -0.084f }, 0.0f },
    { "viewmodel_enfield_scope", { -5.70f, 1.504f, 2.382f }, 0.635f, { 2.509f, 0.063f, -0.065f }, -0.39f },
    { "viewmodel_mosinnagant_cloth", { -5.24f, -0.022f, 3.111f }, 0.265f, { 3.714f, -0.213f, 0.123f }, 0.0f },
};

extern int I_stricmp(const char *s0, const char *s1);

const CgWwiiScopeGlass *CG_WwiiScopeFindGlass(const char *gunModel)
{
    int i;

    if (!gunModel)
        return NULL;
    for (i = 0; i < CG_WWIISCOPE_GLASS_COUNT; i++) {
        if (!I_stricmp(cg_wwiiScopeGlasses[i].gunModel, gunModel))
            return &cg_wwiiScopeGlasses[i];
    }
    return NULL;
}

#ifdef __EMSCRIPTEN__

extern const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags);
extern WeaponDef *BG_GetWeaponDef(int iWeapon);
extern int BG_GetNumWeapons(void);
extern int BG_GetViewmodelWeaponIndex(const playerState_t *ps);
extern unsigned int SL_FindString(const char *str);
extern int DObjGetBoneIndex(void *obj, unsigned int boneName);
extern DObjAnimMat *DObjGetRotTransArray(const struct DObj_s *obj);
extern void ConvertQuatToMat(const DObjAnimMat *mat, float axis[3][3]);
extern void MatrixMultiply(const float (*in1)[3], const float (*in2)[3], float (*out)[3]);
extern float XAnimGetWeight(const struct XAnimTree_s *tree, unsigned int animIndex);
extern int WebSceneFx_ScopeAvailable(void);
extern const dvar_t *cg_fov;
extern const dvar_t *cg_fovScale;
extern const dvar_t *cg_fovMin;

/* The glass's radius on screen at full aim, as a fraction of half the
 * screen's height: WWII's eyepieces reach just past the top and bottom. */
#define CG_WWIISCOPE_LENS_RADIUS 1.04f
/* A scope raised to the eye zooms over this last part of the raise, while
 * its window opens; the stock scopes snap in over the last 5 to 42 percent. */
#define CG_WWIISCOPE_ZOOM_IN_FRAC 0.4f
/* Through this part of the raise the stock glass gives way to the window. */
#define CG_WWIISCOPE_PRESENCE_START 0.5f
#define CG_WWIISCOPE_PRESENCE_END 0.8f
/* Seconds for the view around the eyepiece to black out when the breath is
 * held, and to come back. */
#define CG_WWIISCOPE_DARK_IN 0.15f
#define CG_WWIISCOPE_DARK_OUT 0.2f
/* Points nearer the eye than this are not projected. */
#define CG_WWIISCOPE_MIN_DEPTH 0.05f
/* With the window up, the rifle is clipped this near the eye: the stock
 * rechamber anims bring the hand on the bolt past the eye, while every
 * eyepiece stays further, even in the fire anims' kick. */
#define CG_WWIISCOPE_NEAR_CLIP 0.8f
/* A rifle whose scoped rechamber is its idle pose rolls this many degrees
 * anticlockwise while the bolt is worked and moves its glass this far left
 * and down, in halves of the screen's height, as WWII's scopes do. */
#define CG_WWIISCOPE_BOLT_ROLL 8.0f
#define CG_WWIISCOPE_BOLT_LEFT 0.4f
#define CG_WWIISCOPE_BOLT_DOWN 0.12f
/* The view weapon's rechamber anims, from the hip and scoped, as
 * CG_RegisterWeapon numbers them. */
#define CG_WWIISCOPE_RECHAMBER_ANIM 6
#define CG_WWIISCOPE_ADS_RECHAMBER_ANIM 20
#define CG_WWIISCOPE_RECHAMBERING 4
#define CG_WWIISCOPE_HOLDING_BREATH 0x4000
#define CG_WWIISCOPE_MAX_WEAPONS 128
#define CG_WWIISCOPE_UNKNOWN -2
#define CG_WWIISCOPE_NONE -1

typedef struct CgWwiiScopeImage {
    MaterialHandle overlay;
    MaterialHandle lens;
    float edge;
    int generation;
} CgWwiiScopeImage;

/* This frame's bolt: the weights of the view weapon's fire anims and of its
 * animated rechamber, and how far out a procedural rechamber is. */
typedef struct CgWwiiScopeBolt {
    float fire;
    float rechamber;
    float procedural;
} CgWwiiScopeBolt;

/* Read by R_RenderScene: the floats of this frame's CgWwiiScopeLens. */
float g_webScopeLens[13];

static const dvar_t *cg_wwiiScope;
static signed char s_glassIndex[CG_WWIISCOPE_MAX_WEAPONS];
/* The scoped rechamber anim is the idle pose. */
static unsigned char s_idleBolt[CG_WWIISCOPE_MAX_WEAPONS];
static CgWwiiScopeImage s_images[R_HUD_IMAGE_SCOPE_LENS_COUNT];
static CgWwiiScopeLens s_lens;
/* The view weapon this frame is a WWII scope's rifle: its stock overlay
 * stays off even before the window opens. */
static int s_viewWeapon;
static int s_lightsOnly;
static float s_dark;
static int s_darkTime;

static float CG_WwiiScopeClamp01(float value)
{
    return value > 0.0f ? (value < 1.0f ? value : 1.0f) : 0.0f;
}

static float CG_WwiiScopeSmoothstep(float edge0, float edge1, float value)
{
    float t = CG_WwiiScopeClamp01((value - edge0) / (edge1 - edge0));

    return t * t * (3.0f - 2.0f * t);
}

static const CgWwiiScopeGlass *CG_WwiiScopeWeaponGlass(int weapon)
{
    if (weapon <= 0 || weapon >= CG_WWIISCOPE_MAX_WEAPONS || weapon > BG_GetNumWeapons())
        return NULL;
    if (s_glassIndex[weapon] == CG_WWIISCOPE_UNKNOWN) {
        const WeaponDef *weapDef = BG_GetWeaponDef(weapon);
        const CgWwiiScopeGlass *glass = NULL;

        /* Binoculars (class 9) keep their own overlay. */
        if (weapDef && weapDef->overlayReticle && weapDef->weapClass != 9 && weapDef->szOverlayMaterial[0])
            glass = CG_WwiiScopeFindGlass(weapDef->szGunXModel);
        s_glassIndex[weapon] = glass ? (signed char)(glass - cg_wwiiScopeGlasses) : CG_WWIISCOPE_NONE;
        s_idleBolt[weapon] = glass && (!weapDef->szXAnims[20] || !weapDef->szXAnims[20][0] ||
                                       (weapDef->szXAnims[1] && !I_stricmp(weapDef->szXAnims[20], weapDef->szXAnims[1])));
    }
    return s_glassIndex[weapon] >= 0 ? &cg_wwiiScopeGlasses[s_glassIndex[weapon]] : NULL;
}

static int CG_WwiiScopeFullView(void)
{
    const vidConfig_t *vid = &((clientStatic_t *)imp_cls)->vidConfig;

    return !cg->refdef.x && !cg->refdef.y && cg->refdef.width == (int)vid->width &&
           cg->refdef.height == (int)vid->height;
}

/* The compositor only composes a view that fills the screen. */
const CgWwiiScopeGlass *CG_WwiiScopeGlassForWeapon(int weapon)
{
    if (!cg_wwiiScope || !cg_wwiiScope->current.enabled || !WebSceneFx_ScopeAvailable() || !CG_WwiiScopeFullView())
        return NULL;
    return CG_WwiiScopeWeaponGlass(weapon);
}

/* The lens image of a scope overlay, built once per renderer into one of the
 * HUD's lens slots. */
static MaterialHandle CG_WwiiScopeLensImage(MaterialHandle overlay, float *edge)
{
    int generation = R_GetHudImageGeneration();
    int i, slot = -1;

    if (!overlay)
        return NULL;
    for (i = 0; i < R_HUD_IMAGE_SCOPE_LENS_COUNT; i++) {
        if (s_images[i].overlay == overlay) {
            if (s_images[i].generation == generation) {
                *edge = s_images[i].edge;
                return s_images[i].lens;
            }
            slot = i;
            break;
        }
        if (!s_images[i].overlay && slot < 0)
            slot = i;
    }
    if (slot < 0)
        return NULL;
    s_images[slot].overlay = overlay;
    s_images[slot].generation = generation;
    s_images[slot].lens = R_CreateHudLensMaterial(R_HUD_IMAGE_SCOPE_LENS + slot, overlay, &s_images[slot].edge);
    *edge = s_images[slot].edge;
    return s_images[slot].lens;
}

void CG_WwiiScopeInit(void)
{
    int weapon;

    cg_wwiiScope = Dvar_RegisterBool_mac("cg_wwiiScope", 1, 0x1001);
    memset(s_glassIndex, CG_WWIISCOPE_UNKNOWN, sizeof(s_glassIndex));
    memset(s_idleBolt, 0, sizeof(s_idleBolt));
    memset(s_images, 0, sizeof(s_images));
    memset(&s_lens, 0, sizeof(s_lens));
    memset(g_webScopeLens, 0, sizeof(g_webScopeLens));
    s_viewWeapon = 0;
    s_lightsOnly = 0;
    s_dark = 0.0f;
    s_darkTime = 0;
    /* The match's scopes are registered with the level; their lenses are
     * built now rather than on the first raise. */
    for (weapon = 1; weapon <= BG_GetNumWeapons() && weapon < CG_WWIISCOPE_MAX_WEAPONS; weapon++) {
        const weaponInfo_t *weapInfo = &(*(weaponInfo_t **)imp_cg_weapons)[weapon];
        float edge;

        if (weapInfo->registered && CG_WwiiScopeWeaponGlass(weapon))
            CG_WwiiScopeLensImage(weapInfo->hADSOverlay, &edge);
    }
}

void CG_WwiiScopeClear(void)
{
    memset(&s_lens, 0, sizeof(s_lens));
    memset(g_webScopeLens, 0, sizeof(g_webScopeLens));
    s_viewWeapon = 0;
    s_lightsOnly = 0;
}

float CG_WwiiScopeZoomInFrac(int weapon)
{
    const WeaponDef *weapDef = BG_GetWeaponDef(weapon);

    if (CG_WwiiScopeGlassForWeapon(weapon) && weapDef->fAdsZoomInFrac < CG_WWIISCOPE_ZOOM_IN_FRAC)
        return CG_WWIISCOPE_ZOOM_IN_FRAC;
    return weapDef->fAdsZoomInFrac;
}

/* As CG_GetViewFov zooms the world: over the last frac of the raise, smoothed. */
static float CG_WwiiScopeZoomFraction(float posFrac, float frac)
{
    if (posFrac >= 1.0f)
        return 1.0f;
    if (!(frac > 0.0f))
        return 0.0f;
    return CG_WwiiScopeSmoothstep(0.0f, 1.0f, (posFrac - (1.0f - frac)) / frac);
}

static float CG_WwiiScopeZoom(const playerState_t *ps, int weapon)
{
    const WeaponDef *weapDef = BG_GetWeaponDef(weapon);

    return CG_WwiiScopeZoomFraction(ps->fWeaponPosFrac, cg->playerEntity.bPositionToADS ?
                                    CG_WwiiScopeZoomInFrac(weapon) : weapDef->fAdsZoomOutFrac);
}

/* The move, in units forward, left and up, that puts the glass of the resting
 * pose on the view axis, aim of the way: hand is where the rifle's origin
 * already sits from the eye, with the sway the stock code froze at the
 * start of the raise. */
static void CG_WwiiScopeCentreOffset(const CgWwiiScopeGlass *glass, const float *hand, float aim, float *offset)
{
    offset[0] = 0.0f;
    offset[1] = glass->eye[0] > 0.0f ? -(hand[1] + glass->eye[1]) * aim : 0.0f;
    offset[2] = glass->eye[0] > 0.0f ? -(hand[2] + glass->eye[2]) * aim : 0.0f;
}

/* How far through its roll a scoped bolt is at fraction t of the rechamber:
 * out over the first third, back over the last. */
static float CG_WwiiScopeBoltEnvelope(float t)
{
    return CG_WwiiScopeSmoothstep(0.04f, 0.3f, t) * (1.0f - CG_WwiiScopeSmoothstep(0.6f, 0.95f, t));
}

/* The move, in units, that takes a glass of radius units out by envelope:
 * it covers the same part of the screen whatever the rifle's projection. */
static void CG_WwiiScopeBoltShift(float radius, float envelope, float *offset)
{
    float units = radius / CG_WWIISCOPE_LENS_RADIUS * envelope;

    offset[1] += CG_WWIISCOPE_BOLT_LEFT * units;
    offset[2] -= CG_WWIISCOPE_BOLT_DOWN * units;
}

/* The stock fire and rechamber anims were made for the hip: at the eye they
 * pull the scope back and drop it below the screen. WWII's stays at the eye,
 * swinging out to the side. The glass keeps these shares of a fire anim's
 * move forward, left and up, and of a rechamber's, which leaves it at the
 * eye's distance while the bolt is worked. */
static const float s_fireKeep[3] = { 0.15f, 0.8f, 0.3f };
static const float s_rechamberKeep[3] = { 0.0f, 0.8f, 0.3f };
/* A glass smaller than this, magnified more, keeps less of the moves left
 * and up, so it swings across as much of the screen as the others. */
#define CG_WWIISCOPE_BOLT_GLASS 0.55f

/* glassModel is the glass's rim centre in the rifle's own space, eye where
 * it rests: the move that leaves keep of the anim's move. */
static void CG_WwiiScopeSteady(const CgWwiiScopeGlass *glass, const float *glassModel, const float *keep, float weight,
                               float *offset)
{
    float scale = fminf(1.0f, glass->radius / CG_WWIISCOPE_BOLT_GLASS);
    int i;

    for (i = 0; i < 3; i++)
        offset[i] -= (1.0f - keep[i] * (i ? scale : 1.0f)) * (glassModel[i] - glass->eye[i]) * weight;
}

/* The fraction of the rechamber done. */
static float CG_WwiiScopeBoltProgress(const playerState_t *ps, const WeaponDef *weapDef)
{
    if (ps->weaponstate != CG_WWIISCOPE_RECHAMBERING || weapDef->iRechamberTime <= 0)
        return -1.0f;
    return 1.0f - (float)ps->weaponTime / (float)weapDef->iRechamberTime;
}

/* The anim weights, not the weapon state: the client plays a fire or
 * rechamber anim to its end, past its state's time. A rifle whose scoped
 * rechamber is its idle pose works its bolt procedurally, while
 * PM_StartRechamber's scoped anim plays. */
static void CG_WwiiScopeBoltState(const playerState_t *ps, int weapon, CgWwiiScopeBolt *bolt)
{
    static const unsigned char fireAnims[4] = { 3, 5, 18, 19 };
    const struct XAnimTree_s *tree = (*(weaponInfo_t **)imp_cg_weapons)[weapon].tree;
    float scoped, progress;
    int i;

    memset(bolt, 0, sizeof(*bolt));
    if (!tree)
        return;
    for (i = 0; i < 4; i++)
        bolt->fire = fmaxf(bolt->fire, XAnimGetWeight(tree, fireAnims[i]));
    scoped = XAnimGetWeight(tree, CG_WWIISCOPE_ADS_RECHAMBER_ANIM);
    bolt->rechamber = fmaxf(XAnimGetWeight(tree, CG_WWIISCOPE_RECHAMBER_ANIM), s_idleBolt[weapon] ? 0.0f : scoped);
    if (s_idleBolt[weapon]) {
        progress = CG_WwiiScopeBoltProgress(ps, BG_GetWeaponDef(weapon));
        if (progress >= 0.0f)
            bolt->procedural = CG_WwiiScopeBoltEnvelope(progress) * scoped;
    }
}

void CG_WwiiScopeAdjustViewWeapon(const playerState_t *ps, int weapon, vec_t *angles)
{
    const CgWwiiScopeGlass *glass = CG_WwiiScopeGlassForWeapon(weapon);
    CgWwiiScopeBolt bolt;
    float aim;

    s_lightsOnly = 0;
    if (!glass || !(ps->fWeaponPosFrac > 0.0f))
        return;
    /* Raised, the rifle is drawn with its own projection, which a world-space
     * muzzle sprite would not follow. */
    s_lightsOnly = 1;
    aim = CG_WwiiScopeSmoothstep(0.0f, 1.0f, ps->fWeaponPosFrac);
    angles[2] -= glass->roll * aim;
    CG_WwiiScopeBoltState(ps, weapon, &bolt);
    angles[2] -= CG_WWIISCOPE_BOLT_ROLL * bolt.procedural * aim;
}

qboolean CG_WwiiScopeMuzzleLightsOnly(void)
{
    return s_lightsOnly;
}

/* A world point in normalized device coordinates for projection scales
 * projection[0] and [1]: x right, y up. */
static int CG_WwiiScopeProjectPoint(const vec_t *point, const vec_t *eye, const vec3_t *viewAxis,
                                   const float *projection, float *ndc)
{
    float delta[3], forward, left, up;
    int i;

    for (i = 0; i < 3; i++)
        delta[i] = point[i] - eye[i];
    forward = delta[0] * viewAxis[0][0] + delta[1] * viewAxis[0][1] + delta[2] * viewAxis[0][2];
    if (!(forward > CG_WWIISCOPE_MIN_DEPTH))
        return 0;
    left = delta[0] * viewAxis[1][0] + delta[1] * viewAxis[1][1] + delta[2] * viewAxis[1][2];
    up = delta[0] * viewAxis[2][0] + delta[1] * viewAxis[2][1] + delta[2] * viewAxis[2][2];
    ndc[0] = -left / forward * projection[0];
    ndc[1] = up / forward * projection[1];
    return 1;
}

/* The glass's rim on screen from the j_gun bone's world axes and origin: its
 * centre, and the half-axes along the scope's right and up. Returns 0 when
 * the glass faces away from the eye or reaches behind it. */
int CG_WwiiScopeProjectGlass(const CgWwiiScopeGlass *glass, const vec3_t *boneAxis, const vec_t *boneOrigin,
                             const vec_t *eye, const vec3_t *viewAxis, const float *projection, CgWwiiScopeLens *lens)
{
    float center[3], rim[4][3], ndc[4][2];
    float facing = 0.0f;
    int i, k;

    for (i = 0; i < 3; i++)
        center[i] = boneOrigin[i] + glass->rim[0] * boneAxis[0][i] + glass->rim[1] * boneAxis[1][i] +
                    glass->rim[2] * boneAxis[2][i];
    for (i = 0; i < 3; i++)
        facing -= boneAxis[0][i] * (eye[i] - center[i]);
    if (!(facing > 0.0f))
        return 0;
    /* Right, left, up and down on the rim. */
    for (i = 0; i < 3; i++) {
        rim[0][i] = center[i] - glass->radius * boneAxis[1][i];
        rim[1][i] = center[i] + glass->radius * boneAxis[1][i];
        rim[2][i] = center[i] + glass->radius * boneAxis[2][i];
        rim[3][i] = center[i] - glass->radius * boneAxis[2][i];
    }
    for (k = 0; k < 4; k++) {
        if (!CG_WwiiScopeProjectPoint(rim[k], eye, viewAxis, projection, ndc[k]))
            return 0;
    }
    for (i = 0; i < 2; i++) {
        lens->center[i] = 0.25f * (ndc[0][i] + ndc[1][i] + ndc[2][i] + ndc[3][i]);
        lens->axes[0][i] = 0.5f * (ndc[0][i] - ndc[1][i]);
        lens->axes[1][i] = 0.5f * (ndc[2][i] - ndc[3][i]);
    }
    return fabsf(lens->axes[0][0] * lens->axes[1][1] - lens->axes[1][0] * lens->axes[0][1]) > 1e-6f;
}

/* The j_gun bone's axes and origin in the rifle's own space this frame,
 * from the skeleton CG_UpdateViewWeaponAnim built. */
static int CG_WwiiScopeGunBoneModel(struct DObj_s *obj, float (*axis)[3], vec_t *origin)
{
    unsigned int name = SL_FindString("j_gun");
    const DObjAnimMat *mats;
    int bone;

    if (!obj || !name)
        return 0;
    bone = DObjGetBoneIndex(obj, name);
    mats = bone >= 0 ? DObjGetRotTransArray(obj) : NULL;
    if (!mats)
        return 0;
    ConvertQuatToMat(&mats[bone], axis);
    origin[0] = mats[bone].trans[0];
    origin[1] = mats[bone].trans[1];
    origin[2] = mats[bone].trans[2];
    return 1;
}

/* The j_gun bone's world axes and origin where the rifle was placed. */
static int CG_WwiiScopeGunBone(struct DObj_s *obj, vec3_t *axis, vec_t *origin)
{
    float local[3][3], trans[3];
    int i;

    if (!CG_WwiiScopeGunBoneModel(obj, local, trans))
        return 0;
    MatrixMultiply((const float (*)[3])local, (const float (*)[3])cg->viewModelAxis, (float (*)[3])axis);
    for (i = 0; i < 3; i++)
        origin[i] = cg->viewModelOrigin[i] + trans[0] * cg->viewModelAxis[0][i] + trans[1] * cg->viewModelAxis[1][i] +
                    trans[2] * cg->viewModelAxis[2][i];
    return 1;
}

void CG_WwiiScopePlaceViewWeapon(const playerState_t *ps, int weapon, vec_t *origin)
{
    const CgWwiiScopeGlass *glass = CG_WwiiScopeGlassForWeapon(weapon);
    CgWwiiScopeBolt bolt;
    float hand[3], offset[3], aim, steady;
    int i, k;

    if (!glass || !(ps->fWeaponPosFrac > 0.0f))
        return;
    aim = CG_WwiiScopeSmoothstep(0.0f, 1.0f, ps->fWeaponPosFrac);
    for (k = 0; k < 3; k++) {
        hand[k] = 0.0f;
        for (i = 0; i < 3; i++)
            hand[k] += (origin[i] - cg->refdef.vieworg[i]) * cg->refdef.viewaxis[k][i];
    }
    CG_WwiiScopeCentreOffset(glass, hand, aim, offset);
    CG_WwiiScopeBoltState(ps, weapon, &bolt);
    CG_WwiiScopeBoltShift(glass->radius, bolt.procedural * aim, offset);
    /* Only about the resting pose at full aim, never against the raise. */
    steady = CG_WwiiScopeSmoothstep(0.9f, 1.0f, ps->fWeaponPosFrac);
    if (steady > 0.0f && glass->eye[0] > 0.0f && (bolt.fire > 0.0f || bolt.rechamber > 0.0f)) {
        float axis[3][3], trans[3], glassModel[3];

        if (CG_WwiiScopeGunBoneModel((*(weaponInfo_t **)imp_cg_weapons)[weapon].viewModelDObj, axis, trans)) {
            for (i = 0; i < 3; i++)
                glassModel[i] = trans[i] + glass->rim[0] * axis[0][i] + glass->rim[1] * axis[1][i] +
                                glass->rim[2] * axis[2][i];
            CG_WwiiScopeSteady(glass, glassModel, s_fireKeep, bolt.fire * steady, offset);
            CG_WwiiScopeSteady(glass, glassModel, s_rechamberKeep, bolt.rechamber * steady, offset);
        }
    }
    for (i = 0; i < 3; i++)
        origin[i] += offset[0] * cg->refdef.viewaxis[0][i] + offset[1] * cg->refdef.viewaxis[1][i] +
                     offset[2] * cg->refdef.viewaxis[2][i];
}

/* The rifle's projection scales in x and y, as CG_CalcFov and the renderer
 * build them for a cg_fov-style field of view: the hip view's, narrowing as
 * aim goes from 0 to 1 until a glass of radius units, eyeForward ahead, fills
 * CG_WWIISCOPE_LENS_RADIUS of half the screen's height. Both 0 at the hip. */
static void CG_WwiiScopeProjectionScales(float hipFov, float aspect, float aim, float radius, float eyeForward,
                                         float *projection)
{
    float hipTan, scopeTan;

    projection[0] = projection[1] = 0.0f;
    if (!(aim > 0.0f) || !(eyeForward > CG_WWIISCOPE_MIN_DEPTH) || !(aspect > 0.0f) || !(radius > 0.0f) ||
        !(hipFov > 0.0f && hipFov < 180.0f))
        return;
    hipTan = tanf(hipFov * 0.00872664626f) * 0.75f;
    scopeTan = radius / (CG_WWIISCOPE_LENS_RADIUS * eyeForward);
    projection[1] = 1.0f / (hipTan + (scopeTan - hipTan) * (aim < 1.0f ? aim : 1.0f));
    projection[0] = projection[1] / aspect;
}

/* forward stands in for eye[0] until the eye is measured. */
static void CG_WwiiScopeProjection(const playerState_t *ps, const CgWwiiScopeGlass *glass, float forward,
                                   float *projection)
{
    float hipFov = cg_fov->current.value * cg_fovScale->current.value;

    if (hipFov < cg_fovMin->current.value)
        hipFov = cg_fovMin->current.value;
    CG_WwiiScopeProjectionScales(hipFov, cgs->viewAspect, CG_WwiiScopeSmoothstep(0.0f, 1.0f, ps->fWeaponPosFrac),
                                 glass->radius, glass->eye[0] > 0.0f ? glass->eye[0] : forward, projection);
}

/* The blackout around the glass moved toward target over a frame. */
static float CG_WwiiScopeDarkStep(float dark, float target, float seconds)
{
    float rate = seconds / (target > dark ? CG_WWIISCOPE_DARK_IN : CG_WWIISCOPE_DARK_OUT);

    if (!(rate > 0.0f))
        return dark;
    return target > dark ? fminf(target, dark + rate) : fmaxf(target, dark - rate);
}

qboolean CG_WwiiScopeUpdate(const playerState_t *ps, int weapon, qboolean drawGun)
{
    const CgWwiiScopeGlass *glass = drawGun ? CG_WwiiScopeGlassForWeapon(weapon) : NULL;
    CgWwiiScopeLens *lens = &s_lens;
    const weaponInfo_t *weapInfo;
    vec3_t boneAxis[3];
    vec3_t boneOrigin;
    float forward;
    int i;

    memset(lens, 0, sizeof(*lens));
    s_viewWeapon = 0;
    if (glass && !cg->renderingThirdPerson && !(ps->eFlags & 0x300) && ps->fWeaponPosFrac > 0.0f) {
        weapInfo = &(*(weaponInfo_t **)imp_cg_weapons)[weapon];
        if (CG_WwiiScopeGunBone(weapInfo->viewModelDObj, boneAxis, boneOrigin)) {
            s_viewWeapon = 1;
            forward = 0.0f;
            for (i = 0; i < 3; i++)
                forward += (boneOrigin[i] + glass->rim[0] * boneAxis[0][i] + glass->rim[1] * boneAxis[1][i] +
                            glass->rim[2] * boneAxis[2][i] - cg->refdef.vieworg[i]) * cg->refdef.viewaxis[0][i];
            CG_WwiiScopeProjection(ps, glass, forward, lens->projection);
            if (lens->projection[0] > 0.0f &&
                CG_WwiiScopeProjectGlass(glass, (const vec3_t *)boneAxis, boneOrigin, cg->refdef.vieworg,
                                         (const vec3_t *)cg->refdef.viewaxis, lens->projection, lens)) {
                lens->presence = CG_WwiiScopeSmoothstep(CG_WWIISCOPE_PRESENCE_START, CG_WWIISCOPE_PRESENCE_END,
                                                        ps->fWeaponPosFrac);
                lens->nearClip = CG_WWIISCOPE_NEAR_CLIP * lens->presence;
                lens->open = CG_WwiiScopeZoom(ps, weapon);
                lens->blur = lens->open;
                /* A window closed for more than a frame starts over in the light. */
                if (cg->time - s_darkTime > 100 || cg->time < s_darkTime)
                    s_dark = 0.0f;
                s_darkTime = cg->time;
                s_dark = CG_WwiiScopeDarkStep(
                    s_dark, (ps->pm_flags & CG_WWIISCOPE_HOLDING_BREATH) && lens->open >= 1.0f ? 1.0f : 0.0f,
                    (float)cg->frametime * 0.001f);
                lens->dark = s_dark;
            }
        }
    }
    memcpy(g_webScopeLens, lens, sizeof(g_webScopeLens));
    return s_viewWeapon;
}

const CgWwiiScopeLens *CG_WwiiScopeLens(void)
{
    return &s_lens;
}

/* The overlay's quad in screen pixels: its lens circle, edge of its half
 * width, on the glass's outline, its top along the scope's up. */
static void CG_WwiiScopeReticleQuad(const CgWwiiScopeLens *lens, float edge, float width, float height, float alpha,
                                    HudVertex2D *verts)
{
    static const float corners[4][2] = { { -1.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, -1.0f }, { -1.0f, -1.0f } };
    float cx = (lens->center[0] + 1.0f) * 0.5f * width;
    float cy = (1.0f - lens->center[1]) * 0.5f * height;
    float right[2], up[2];
    int i;

    right[0] = lens->axes[0][0] * 0.5f * width / edge;
    right[1] = -lens->axes[0][1] * 0.5f * height / edge;
    up[0] = lens->axes[1][0] * 0.5f * width / edge;
    up[1] = -lens->axes[1][1] * 0.5f * height / edge;
    for (i = 0; i < 4; i++) {
        verts[i].xy[0] = cx + corners[i][0] * right[0] + corners[i][1] * up[0];
        verts[i].xy[1] = cy + corners[i][0] * right[1] + corners[i][1] * up[1];
        verts[i].st[0] = corners[i][0] > 0.0f ? 1.0f : 0.0f;
        verts[i].st[1] = corners[i][1] > 0.0f ? 0.0f : 1.0f;
        verts[i].rgba[0] = verts[i].rgba[1] = verts[i].rgba[2] = 1.0f;
        verts[i].rgba[3] = alpha;
    }
}

float CG_DrawWwiiScopeReticle(void)
{
    static const unsigned short indices[6] = { 0, 1, 2, 0, 2, 3 };
    const vidConfig_t *vid = &((clientStatic_t *)imp_cls)->vidConfig;
    const weaponInfo_t *weapInfo;
    const CgWwiiScopeLens *lens = &s_lens;
    HudVertex2D verts[4];
    MaterialHandle material;
    float edge = 0.0f;

    if (!s_viewWeapon)
        return -1.0f;
    if (!(lens->presence > 0.0f) || !(lens->open > 0.01f))
        return 1.0f - lens->open;
    weapInfo = &(*(weaponInfo_t **)imp_cg_weapons)[BG_GetViewmodelWeaponIndex(&cg->predictedPlayerState)];
    material = CG_WwiiScopeLensImage(weapInfo->hADSOverlay, &edge);
    if (!material || !(edge > 0.0f))
        return 1.0f - lens->open;
    CG_WwiiScopeReticleQuad(lens, edge, (float)vid->width, (float)vid->height, lens->open, verts);
    R_AddCmdDrawTriangles2D(verts, 4, indices, 6, material);
    return 1.0f - lens->open;
}

#else

void CG_WwiiScopeInit(void)
{
}

void CG_WwiiScopeClear(void)
{
}

const CgWwiiScopeGlass *CG_WwiiScopeGlassForWeapon(int weapon)
{
    (void)weapon;
    return NULL;
}

float CG_WwiiScopeZoomInFrac(int weapon)
{
    extern WeaponDef *BG_GetWeaponDef(int iWeapon);

    return BG_GetWeaponDef(weapon)->fAdsZoomInFrac;
}

void CG_WwiiScopeAdjustViewWeapon(const playerState_t *ps, int weapon, vec_t *angles)
{
    (void)ps;
    (void)weapon;
    (void)angles;
}

void CG_WwiiScopePlaceViewWeapon(const playerState_t *ps, int weapon, vec_t *origin)
{
    (void)ps;
    (void)weapon;
    (void)origin;
}

qboolean CG_WwiiScopeUpdate(const playerState_t *ps, int weapon, qboolean drawGun)
{
    (void)ps;
    (void)weapon;
    (void)drawGun;
    return 0;
}

qboolean CG_WwiiScopeMuzzleLightsOnly(void)
{
    return 0;
}

float CG_DrawWwiiScopeReticle(void)
{
    return -1.0f;
}

const CgWwiiScopeLens *CG_WwiiScopeLens(void)
{
    static const CgWwiiScopeLens closed;

    return &closed;
}

#endif
