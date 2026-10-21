#include "common_types.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include "headers/PC/cgame_mp/cg_scopeglint.h"
#include <math.h>
#include <string.h>

extern const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags);
extern WeaponDef *BG_GetWeaponDef(int iWeapon);
extern int BG_GetNumWeapons(void);
extern int I_stricmp(const char *s0, const char *s1);
extern struct DObj_s *Com_GetClientDObj(int handle, int localClientNum);
extern DObjAnimMat *CG_DObjGetLocalTagMatrix(const centity_t *cent, struct DObj_s *obj, unsigned int tagName);
extern void ConvertQuatToMat(const DObjAnimMat *mat, float axis[3][3]);
extern void AnglesToAxis(const vec_t *angles, vec3_t *axis);
extern void AngleVectors(const vec_t *angles, vec_t *forward, vec_t *right, vec_t *up);
extern void CG_TraceCapsule(trace_t *result, const vec_t *start, const vec_t *mins, const vec_t *maxs,
                            const vec_t *end, int skipNumber, int mask);
extern float FX_GetServerVisibility(const vec_t *start, const vec_t *end);
extern void CL_DefaultVertexFrames(int vertCount, GfxWorldVertex *verts);
extern void CL_AddPolyToScene(MaterialHandle mtlHandle, unsigned short lmapIndex, unsigned short vertCount,
                              const GfxWorldVertex *verts);
extern scr_const_t scr_const;

/* The front of each sniper rifle's scope, read from the world models'
 * xmodelsurfs: the centre of the ring that ends the scope's tube, the
 * Lee-Enfield's left of the barrel. */
const CgScopeGlintLens cg_scopeGlintLenses[CG_SCOPEGLINT_LENS_COUNT] = {
    { "xmodel/weapon_kar98_scoped", { -25.32f, -0.06f, 2.45f } },
    { "xmodel/weapon_springfield", { -20.97f, 0.21f, 2.14f } },
    { "xmodel/weapon_enfield_scope", { -19.68f, 1.55f, 1.37f } },
    { "xmodel/weapon_mosinnagantscoped_cloth", { -27.73f, -0.11f, 1.94f } },
};

/* The entity flags of a player with his sights raised, on a turret, and
 * hidden or dead. */
#define CG_SCOPEGLINT_ADS 0x40000
#define CG_SCOPEGLINT_TURRET 0x300
#define CG_SCOPEGLINT_GONE 0x20020
/* All of the glint within this many degrees of the aim, none beyond. */
#define CG_SCOPEGLINT_FULL_DEG 6.0f
#define CG_SCOPEGLINT_EDGE_DEG 24.0f
/* The glint comes in over the sniper rifles' 250 ms raise, and starts over
 * when the player comes back after going unseen this long. */
#define CG_SCOPEGLINT_RAISE_MSEC 250
#define CG_SCOPEGLINT_GAP_MSEC 200
/* The flare's half width: this share of half the view's height, so a far
 * scope shows through any zoom, but at least MIN_RADIUS units, so a near one
 * covers its glass; this share of it at the edge of the glint. */
#define CG_SCOPEGLINT_SCREEN_RADIUS 0.02f
#define CG_SCOPEGLINT_MIN_RADIUS 1.5f
#define CG_SCOPEGLINT_EDGE_SIZE 0.6f
/* The streak across the flare and the shorter one up it, in flare half
 * widths, at this share of its brightness. */
#define CG_SCOPEGLINT_STREAK_LENGTH 3.0f
#define CG_SCOPEGLINT_STREAK_WIDTH 0.3f
#define CG_SCOPEGLINT_SPIKE_LENGTH 1.5f
#define CG_SCOPEGLINT_SPIKE_WIDTH 0.25f
#define CG_SCOPEGLINT_STREAK 0.55f
/* The brightest the flare gets, and how much of it shimmers away and back
 * over each period. */
#define CG_SCOPEGLINT_BRIGHTNESS 1.0f
#define CG_SCOPEGLINT_SHIMMER 0.25f
#define CG_SCOPEGLINT_SHIMMER_MSEC 900
/* Units the flare is drawn toward the eye, ahead of the rim around the glass. */
#define CG_SCOPEGLINT_PULL 2.0f
/* No glint from nearer than this. */
#define CG_SCOPEGLINT_NEAR 24.0f
/* The world and players hide the scope from the eye. Smoke the server would
 * not see through hides it too, and thinner smoke dims it. */
#define CG_SCOPEGLINT_SIGHT_MASK 0x2000001
#define CG_SCOPEGLINT_MIN_VISIBILITY 0.2f
#define CG_SCOPEGLINT_QUADS 3
#define CG_SCOPEGLINT_CLIENTS 64
#define CG_SCOPEGLINT_MAX_WEAPONS 128
#define CG_SCOPEGLINT_UNKNOWN -2
#define CG_SCOPEGLINT_NONE -1

typedef struct CgScopeGlintSight {
    int clientNum;
    float raise;
    vec3_t lens;
    vec3_t forward;
} CgScopeGlintSight;

/* The tracer's image is yellow but for its core: these weights leave its glow
 * white, apart from a muzzle flash's orange, and its core a cool white. */
static const float s_glintColor[3] = { 0.52f, 0.54f, 1.0f };

static const dvar_t *cg_scopeGlint;
static signed char s_lensIndex[CG_SCOPEGLINT_MAX_WEAPONS];
/* The scopes noted in frame s_sightFrame. */
static CgScopeGlintSight s_sights[CG_SCOPEGLINT_CLIENTS];
static int s_sightCount;
static int s_sightFrame;
/* When each player's sights went up, -1 while they are down, and when he was
 * last seen with them up. */
static int s_raisedAt[CG_SCOPEGLINT_CLIENTS];
static int s_seenAt[CG_SCOPEGLINT_CLIENTS];

static float CG_ScopeGlintSmoothstep(float edge0, float edge1, float value)
{
    float t = (value - edge0) / (edge1 - edge0);

    t = t > 0.0f ? (t < 1.0f ? t : 1.0f) : 0.0f;
    return t * t * (3.0f - 2.0f * t);
}

const CgScopeGlintLens *CG_ScopeGlintFindLens(const char *worldModel)
{
    int i;

    if (!worldModel)
        return NULL;
    for (i = 0; i < CG_SCOPEGLINT_LENS_COUNT; i++) {
        if (!I_stricmp(cg_scopeGlintLenses[i].worldModel, worldModel))
            return &cg_scopeGlintLenses[i];
    }
    return NULL;
}

/* The lens of a weapon with a scope of its own. Binoculars (class 9) have an
 * overlay but nothing on a rifle. */
static const CgScopeGlintLens *CG_ScopeGlintWeaponLens(int weapon)
{
    if (weapon <= 0 || weapon >= CG_SCOPEGLINT_MAX_WEAPONS || weapon > BG_GetNumWeapons())
        return NULL;
    if (s_lensIndex[weapon] == CG_SCOPEGLINT_UNKNOWN) {
        const WeaponDef *weapDef = BG_GetWeaponDef(weapon);
        const CgScopeGlintLens *lens = NULL;

        if (weapDef && weapDef->overlayReticle && weapDef->weapClass != 9 && weapDef->szOverlayMaterial[0])
            lens = CG_ScopeGlintFindLens(weapDef->szWorldModel);
        s_lensIndex[weapon] = lens ? (signed char)(lens - cg_scopeGlintLenses) : CG_SCOPEGLINT_NONE;
    }
    return s_lensIndex[weapon] >= 0 ? &cg_scopeGlintLenses[s_lensIndex[weapon]] : NULL;
}

/* Anyone without teams, else a player of another team; never a spectator.
 * A spectator sees everyone's. */
static int CG_ScopeGlintRival(const clientInfo_t *ci, team_t viewTeam)
{
    return ci->infoValid && ci->team != TEAM_SPECTATOR && (viewTeam == TEAM_FREE || ci->team != viewTeam);
}

/* How far in a player's glint is at time: none when his sights went up or
 * when he comes back into view, all of it RAISE_MSEC later. */
static float CG_ScopeGlintRaise(int clientNum, int time)
{
    if (s_raisedAt[clientNum] < 0 || time < s_raisedAt[clientNum] || time < s_seenAt[clientNum] ||
        time - s_seenAt[clientNum] > CG_SCOPEGLINT_GAP_MSEC)
        s_raisedAt[clientNum] = time;
    s_seenAt[clientNum] = time;
    return CG_ScopeGlintSmoothstep(0.0f, (float)CG_SCOPEGLINT_RAISE_MSEC, (float)(time - s_raisedAt[clientNum]));
}

/* How much of its glint a scope aimed along forward turns on an eye in the
 * direction toEye, both of unit length. */
static float CG_ScopeGlintFacing(const vec_t *forward, const vec_t *toEye)
{
    float c = forward[0] * toEye[0] + forward[1] * toEye[1] + forward[2] * toEye[2];
    float degrees = acosf(c > 1.0f ? 1.0f : c < -1.0f ? -1.0f : c) * (180.0f / 3.14159265f);

    return CG_ScopeGlintSmoothstep(CG_SCOPEGLINT_EDGE_DEG, CG_SCOPEGLINT_FULL_DEG, degrees);
}

/* The flare's half width in units, distance units from the eye of a view
 * whose half height is tanHalfFovY a unit ahead. */
static float CG_ScopeGlintRadius(float distance, float tanHalfFovY)
{
    float radius = distance * tanHalfFovY * CG_SCOPEGLINT_SCREEN_RADIUS;

    return radius > CG_SCOPEGLINT_MIN_RADIUS ? radius : CG_SCOPEGLINT_MIN_RADIUS;
}

/* Each player's scope shimmers on its own beat. */
static float CG_ScopeGlintShimmer(int time, int clientNum)
{
    float phase = (float)((unsigned int)(time + clientNum * 211) % CG_SCOPEGLINT_SHIMMER_MSEC) /
                  (float)CG_SCOPEGLINT_SHIMMER_MSEC;

    return 1.0f - CG_SCOPEGLINT_SHIMMER * (0.5f - 0.5f * cosf(phase * 6.28318531f));
}

/* The lens in the world, from tag_flash's axes and origin in the player's
 * model and the model's axes and origin in the world. */
static void CG_ScopeGlintPlaceLens(const float *lens, const float (*tagAxis)[3], const float *tagOrigin,
                                   const vec3_t *axis, const vec_t *origin, vec_t *out)
{
    float local[3];
    int i;

    for (i = 0; i < 3; i++)
        local[i] = tagOrigin[i] + lens[0] * tagAxis[0][i] + lens[1] * tagAxis[1][i] + lens[2] * tagAxis[2][i];
    for (i = 0; i < 3; i++)
        out[i] = origin[i] + local[0] * axis[0][i] + local[1] * axis[1][i] + local[2] * axis[2][i];
}

/* World vertices are RGBA; the tracer's material adds its colour. */
static void CG_ScopeGlintColor(float brightness, byte *color)
{
    int i;

    brightness = brightness > 0.0f ? (brightness < 1.0f ? brightness : 1.0f) : 0.0f;
    for (i = 0; i < 3; i++)
        color[i] = (byte)(255.0f * brightness * s_glintColor[i] + 0.5f);
    color[3] = (byte)(255.0f * brightness + 0.5f);
}

/* One of the glint's quads about centre, width units along left and height
 * along up, the tracer's round image across it. */
static void CG_ScopeGlintQuad(const vec_t *centre, const vec_t *left, const vec_t *up, float width, float height,
                              const byte *color, GfxWorldVertex *verts)
{
    static const float corners[4][2] = { { 1.0f, 1.0f }, { -1.0f, 1.0f }, { -1.0f, -1.0f }, { 1.0f, -1.0f } };
    int i, k;

    for (i = 0; i < 4; i++) {
        for (k = 0; k < 3; k++)
            verts[i].xyz[k] = centre[k] + corners[i][0] * width * left[k] + corners[i][1] * height * up[k];
        for (k = 0; k < 4; k++)
            verts[i].color.array[k] = color[k];
        verts[i].texCoord[0] = verts[i].lmapCoord[0] = corners[i][0] > 0.0f ? 1.0f : 0.0f;
        verts[i].texCoord[1] = verts[i].lmapCoord[1] = corners[i][1] > 0.0f ? 0.0f : 1.0f;
    }
}

/* The flare, the streak across it and the shorter one up it. */
static void CG_ScopeGlintQuads(const vec_t *centre, const vec_t *left, const vec_t *up, float radius,
                               float brightness, GfxWorldVertex *verts)
{
    byte flare[4], streak[4];

    CG_ScopeGlintColor(brightness, flare);
    CG_ScopeGlintColor(brightness * CG_SCOPEGLINT_STREAK, streak);
    CG_ScopeGlintQuad(centre, left, up, radius, radius, flare, &verts[0]);
    CG_ScopeGlintQuad(centre, left, up, CG_SCOPEGLINT_STREAK_LENGTH * radius, CG_SCOPEGLINT_STREAK_WIDTH * radius,
                      streak, &verts[4]);
    CG_ScopeGlintQuad(centre, left, up, CG_SCOPEGLINT_SPIKE_WIDTH * radius, CG_SCOPEGLINT_SPIKE_LENGTH * radius,
                      streak, &verts[8]);
}

void CG_ScopeGlintInit(void)
{
    int i;

    cg_scopeGlint = Dvar_RegisterBool_mac("cg_scopeGlint", 1, 0x1001);
    memset(s_lensIndex, CG_SCOPEGLINT_UNKNOWN, sizeof(s_lensIndex));
    for (i = 0; i < CG_SCOPEGLINT_CLIENTS; i++) {
        s_raisedAt[i] = -1;
        s_seenAt[i] = 0;
    }
    s_sightCount = 0;
}

void CG_ScopeGlintAddPlayer(const centity_t *cent)
{
    const entityState_t *es = &cent->nextState;
    int clientNum = es->clientNum, viewer;
    const CgScopeGlintLens *lens;
    struct DObj_s *obj;
    const DObjAnimMat *tag;
    CgScopeGlintSight *sight;
    float tagAxis[3][3];
    vec3_t axis[3];

    if (s_sightFrame != cg->clientFrame) {
        s_sightFrame = cg->clientFrame;
        s_sightCount = 0;
    }
    if (!cg_scopeGlint || !cg_scopeGlint->current.enabled || clientNum < 0 || clientNum >= CG_SCOPEGLINT_CLIENTS)
        return;
    if (!(es->eFlags & CG_SCOPEGLINT_ADS) || (es->eFlags & (CG_SCOPEGLINT_TURRET | CG_SCOPEGLINT_GONE))) {
        s_raisedAt[clientNum] = -1;
        return;
    }
    viewer = cg->nextSnap->ps.clientNum;
    lens = CG_ScopeGlintWeaponLens(es->weapon);
    if (!lens || clientNum == viewer || viewer < 0 || viewer >= CG_SCOPEGLINT_CLIENTS ||
        s_sightCount >= CG_SCOPEGLINT_CLIENTS ||
        !CG_ScopeGlintRival(&cg->bgs.clientinfo[clientNum], cg->bgs.clientinfo[viewer].team))
        return;
    obj = Com_GetClientDObj(es->number, cent->localClientNum);
    tag = obj ? CG_DObjGetLocalTagMatrix(cent, obj, scr_const.tag_flash) : NULL;
    if (!tag)
        return;
    sight = &s_sights[s_sightCount++];
    sight->clientNum = clientNum;
    sight->raise = CG_ScopeGlintRaise(clientNum, cg->time);
    ConvertQuatToMat(tag, tagAxis);
    AnglesToAxis(cent->lerpAngles, axis);
    CG_ScopeGlintPlaceLens(lens->lens, (const float (*)[3])tagAxis, tag->trans, (const vec3_t *)axis,
                           cent->lerpOrigin, sight->lens);
    AngleVectors(cg->bgs.clientinfo[clientNum].playerAngles, sight->forward, NULL, NULL);
}

void CG_DrawScopeGlints(void)
{
    static const vec3_t zero = { 0.0f, 0.0f, 0.0f };
    const float *eye = cg->refdef.vieworg;
    float tanHalfFovY;
    int i, k;

    if (s_sightFrame != cg->clientFrame)
        return;
    tanHalfFovY = tanf(cg->refdef.fov_y * 0.00872664626f);
    for (i = 0; i < s_sightCount; i++) {
        const CgScopeGlintSight *sight = &s_sights[i];
        GfxWorldVertex verts[CG_SCOPEGLINT_QUADS * 4];
        vec3_t toEye, centre;
        float distance, facing, visibility, radius, brightness;
        trace_t trace;

        for (k = 0; k < 3; k++)
            toEye[k] = eye[k] - sight->lens[k];
        distance = sqrtf(toEye[0] * toEye[0] + toEye[1] * toEye[1] + toEye[2] * toEye[2]);
        if (!(distance > CG_SCOPEGLINT_NEAR))
            continue;
        for (k = 0; k < 3; k++)
            toEye[k] /= distance;
        facing = CG_ScopeGlintFacing(sight->forward, toEye) * sight->raise;
        if (!(facing > 0.0f))
            continue;
        CG_TraceCapsule(&trace, eye, zero, zero, sight->lens, cg->nextSnap->ps.clientNum, CG_SCOPEGLINT_SIGHT_MASK);
        if (trace.fraction < 1.0f && trace.entityNum != sight->clientNum)
            continue;
        visibility = FX_GetServerVisibility(eye, sight->lens);
        if (!(visibility >= CG_SCOPEGLINT_MIN_VISIBILITY))
            continue;
        for (k = 0; k < 3; k++)
            centre[k] = sight->lens[k] + toEye[k] * CG_SCOPEGLINT_PULL;
        radius = CG_ScopeGlintRadius(distance, tanHalfFovY) *
                 (CG_SCOPEGLINT_EDGE_SIZE + (1.0f - CG_SCOPEGLINT_EDGE_SIZE) * facing);
        brightness = CG_SCOPEGLINT_BRIGHTNESS * facing * visibility * CG_ScopeGlintShimmer(cg->time, sight->clientNum);
        CG_ScopeGlintQuads(centre, cg->refdef.viewaxis[1], cg->refdef.viewaxis[2], radius, brightness, verts);
        for (k = 0; k < CG_SCOPEGLINT_QUADS; k++) {
            CL_DefaultVertexFrames(4, &verts[4 * k]);
            CL_AddPolyToScene(cgs->media.tracerMaterial, 0x1f, 4, &verts[4 * k]);
        }
    }
    s_sightCount = 0;
}
