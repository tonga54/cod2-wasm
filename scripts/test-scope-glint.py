#!/usr/bin/env python3
"""The sniper scope glint (cg_scopeglint_mp.c).

Checks the hooks that note each player once his weapon is added, draw the
glints once the view is placed and start the module with the cgame; runs the
production module against simulated players, views, traces and smoke under
ASan and UBSan, with mutants that must fail; and, when the retail IWDs are in
data/main, checks the lens table against the sniper rifles' world models and
the multiplayer weapon files.
"""
from pathlib import Path
import math
import re
import struct
import subprocess
import tempfile
import zipfile

root = Path(__file__).resolve().parent.parent
module = (root / 'src/PC/cgame_mp/cg_scopeglint_mp.c').read_text()
header = (root / 'src/headers/PC/cgame_mp/cg_scopeglint.h').read_text()
players = (root / 'src/PC/cgame_mp/cg_players_mp.c').read_text()
view = (root / 'src/PC/cgame_mp/cg_view_mp.c').read_text()
main = (root / 'src/PC/cgame_mp/cg_main_mp.c').read_text()
com_math = (root / 'src/PC/universal/com_math.c').read_text()


def function(text, name):
    match = re.search(r'^[^\n]*\b' + re.escape(name) + r'\([^;{]*?\)\s*\{', text, re.M)
    assert match, name
    end = text.index('{', match.start()) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


# A player is noted once his model, which carries his gun, is in the scene,
# before the teleport bit (0x2), set on every other spawn or teleport, skips
# the rest; the glints are drawn once the view is placed for the frame, with
# the tracers, before the scene is rendered; the module starts with the cgame.
assert function(players, 'CG_Player').endswith(
    '    CG_AddCEntityToScene(body, obj, cent);\n    CG_ScopeGlintAddPlayer(cent);\n\n'
    '    if (es->eFlags & 0x2)\n        return;\n\n    CG_AddPlayerWeapon(body, 0, cent, 1);\n}\n')
frame = function(view, 'CG_DrawActiveFrame')
assert frame.index('    CG_AddPacketEntities();\n') < frame.index('    CG_CalcViewValues();\n    CG_ApplyDeathView();\n') < \
    frame.index('    CG_AddLocalEntities();\n    CG_DrawScopeGlints();\n') < frame.index('        CG_DrawActive();\n')
assert main.count('    CG_WwiiScopeInit();\n    CG_ScopeGlintInit();\n') == 1
for text in (players, view, main):
    assert text.count('#include "headers/PC/cgame_mp/cg_scopeglint.h"\n') == 1

body = '\n'.join(line for line in module.splitlines() if not line.startswith('#include')) + '\n'
declarations = '\n'.join(line for line in header.splitlines() if not line.startswith('#include'))
math_body = ''.join(function(com_math, name) for name in ('ConvertQuatToMat', 'AngleVectors', 'AnglesToAxis'))

support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
typedef unsigned char byte;
typedef float vec_t;
typedef vec_t vec3_t[3];
typedef enum { TEAM_FREE, TEAM_AXIS, TEAM_ALLIES, TEAM_SPECTATOR } team_t;
typedef struct { struct { int enabled; } current; } dvar_t;
typedef struct { const char *szWorldModel; char *szOverlayMaterial; int overlayReticle; int weapClass; } WeaponDef;
typedef struct { float quat[4]; float trans[3]; float transWeight; } DObjAnimMat;
struct DObj_s { int unused; };
typedef struct { int eFlags, clientNum, number, weapon; } entityState_t;
typedef struct { entityState_t nextState; vec3_t lerpOrigin, lerpAngles; int localClientNum; } centity_t;
typedef struct { int infoValid; team_t team; vec3_t playerAngles; } clientInfo_t;
typedef struct { struct { int clientNum; } ps; } snapshot_t;
typedef struct {
    int clientFrame, time;
    snapshot_t *nextSnap;
    struct { float fov_y; vec3_t vieworg; vec3_t viewaxis[3]; } refdef;
    struct { clientInfo_t clientinfo[64]; } bgs;
} cg_t;
typedef struct Material { int unused; } *MaterialHandle;
typedef struct { struct { MaterialHandle tracerMaterial; } media; } cgs_t;
typedef struct {
    vec3_t xyz;
    vec3_t normal;
    union { unsigned int packed; byte array[4]; } color;
    float texCoord[2];
    float lmapCoord[2];
    vec3_t binormal;
    vec3_t tangent;
} GfxWorldVertex;
typedef struct {
    float fraction; vec3_t normal; int surfaceFlags, contents; const char *material;
    unsigned short entityNum, partName, partGroup; unsigned char allsolid, startsolid;
} trace_t;
typedef struct { unsigned short tag_flash; } scr_const_t;
''' + declarations + r'''
static snapshot_t snap;
static cg_t cgState;
cg_t *cg = &cgState;
static struct Material tracer;
static cgs_t cgsState = { { &tracer } };
cgs_t *cgs = &cgsState;
scr_const_t scr_const = { 77 };
''' + math_body + r'''
/* 1 the scoped Kar98k; 2 the iron-sighted Kar98k; 3 binoculars, here on the
 * scoped Kar98k's model; 4 a scope the table lacks; 5 the Springfield, its
 * model in another case; 6 the scoped Lee-Enfield; 7 the scoped
 * Mosin-Nagant; 8 a gun on the scoped Kar98k's model without a scope. */
#define WEAPONS 8
static char overlay[] = "scope_overlay_german", none[] = "";
static WeaponDef defs[WEAPONS + 1] = {
    { "", none, 0, 0 },
    { "xmodel/weapon_kar98_scoped", overlay, 2, 0 },
    { "xmodel/weapon_kar98", none, 0, 0 },
    { "xmodel/weapon_kar98_scoped", overlay, 4, 9 },
    { "xmodel/weapon_g43_scoped", overlay, 2, 0 },
    { "XMODEL/Weapon_Springfield", overlay, 3, 0 },
    { "xmodel/weapon_enfield_scope", overlay, 2, 0 },
    { "xmodel/weapon_mosinnagantscoped_cloth", overlay, 2, 0 },
    { "xmodel/weapon_kar98_scoped", none, 0, 0 },
};
static int defLookups;
WeaponDef *BG_GetWeaponDef(int weapon) { assert(weapon > 0 && weapon <= WEAPONS); defLookups++; return &defs[weapon]; }
int BG_GetNumWeapons(void) { return WEAPONS; }
int I_stricmp(const char *a, const char *b) { return strcasecmp(a, b); }
static dvar_t glintDvar;
static const char *dvarName;
static int dvarValue = -1, dvarFlags;
const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags)
{
    dvarName = name;
    dvarValue = value;
    dvarFlags = flags;
    glintDvar.current.enabled = value;
    return &glintDvar;
}

static struct DObj_s dobjs[64];
static int noDObj[64], noTag[64];
static DObjAnimMat tags[64];
struct DObj_s *Com_GetClientDObj(int handle, int localClientNum)
{
    assert(handle >= 0 && handle < 64 && localClientNum == 0);
    return noDObj[handle] ? NULL : &dobjs[handle];
}
DObjAnimMat *CG_DObjGetLocalTagMatrix(const centity_t *cent, struct DObj_s *obj, unsigned int tagName)
{
    int i = (int)(obj - dobjs);
    assert(tagName == scr_const.tag_flash && cent->nextState.number == i);
    return noTag[i] ? NULL : &tags[i];
}

/* Every trace stops at traceFraction on traceEntity; the smoke between lets
 * smoke through. */
static float traceFraction = 1.0f, smoke = 1.0f;
static unsigned short traceEntity = 1023;
static int traces, traceSkip, traceMask;
static vec3_t traceStart, traceEnd;
void CG_TraceCapsule(trace_t *result, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end,
                     int skipNumber, int mask)
{
    assert(!mins[0] && !mins[1] && !mins[2] && !maxs[0] && !maxs[1] && !maxs[2]);
    memset(result, 0, sizeof(*result));
    result->fraction = traceFraction;
    result->entityNum = traceEntity;
    memcpy(traceStart, start, sizeof(vec3_t));
    memcpy(traceEnd, end, sizeof(vec3_t));
    traceSkip = skipNumber;
    traceMask = mask;
    traces++;
}
float FX_GetServerVisibility(const vec_t *start, const vec_t *end)
{
    assert(!memcmp(start, traceStart, sizeof(vec3_t)) && !memcmp(end, traceEnd, sizeof(vec3_t)));
    return smoke;
}

#define MAX_POLYS 256
static struct { MaterialHandle material; int lmapIndex, framed; GfxWorldVertex verts[4]; } polys[MAX_POLYS];
static int polyCount, framedVerts;
void CL_DefaultVertexFrames(int vertCount, GfxWorldVertex *verts)
{
    assert(vertCount == 4);
    for (int i = 0; i < vertCount; i++)
        verts[i].tangent[0] = 1.0f;
    framedVerts += vertCount;
}
void CL_AddPolyToScene(MaterialHandle mtlHandle, unsigned short lmapIndex, unsigned short vertCount,
                       const GfxWorldVertex *verts)
{
    assert(polyCount < MAX_POLYS && vertCount == 4);
    polys[polyCount].material = mtlHandle;
    polys[polyCount].lmapIndex = lmapIndex;
    polys[polyCount].framed = verts[0].tangent[0] == 1.0f && verts[3].tangent[0] == 1.0f;
    memcpy(polys[polyCount].verts, verts, sizeof(polys[polyCount].verts));
    polyCount++;
}
'''

checks = r'''
/* The values pinned here, not the module's own names for them. */
#define FULL 1.0f
#define SCREEN_RADIUS 0.02f
#define MIN_RADIUS 1.5f
#define ADS 0x40000
#define PERIOD 900
#define LEVEL (1.0f / 255.0f)
static int near(float a, float b, float tolerance) { return fabsf(a - b) <= tolerance; }
static centity_t ents[64];
static int present[64], lastGlints;
/* tag_flash in the player's model: ahead, right and at eye height. */
static const float tagAt[3] = { 30.0f, -6.0f, 58.0f };
static const float tan30 = 0.57735027f;

static void viewer(int client, team_t team)
{
    snap.ps.clientNum = client;
    cg->bgs.clientinfo[client].infoValid = 1;
    cg->bgs.clientinfo[client].team = team;
}
static void player(int client, team_t team, float x, float y, float z, float yaw, int weapon, int flags)
{
    memset(&ents[client], 0, sizeof(ents[client]));
    ents[client].nextState.clientNum = ents[client].nextState.number = client;
    ents[client].nextState.weapon = weapon;
    ents[client].nextState.eFlags = flags;
    ents[client].lerpOrigin[0] = x;
    ents[client].lerpOrigin[1] = y;
    ents[client].lerpOrigin[2] = z;
    ents[client].lerpAngles[1] = yaw;
    cg->bgs.clientinfo[client].infoValid = 1;
    cg->bgs.clientinfo[client].team = team;
    noDObj[client] = noTag[client] = 0;
    memset(&tags[client], 0, sizeof(tags[client]));
    tags[client].quat[3] = 1.0f;
    tags[client].transWeight = 2.0f;
    memcpy(tags[client].trans, tagAt, sizeof(tagAt));
    present[client] = 1;
}
/* The lens in the world for an unturned tag at tagAt, by the model's yaw. */
static void lensAt(int client, int lensIndex, float *out)
{
    const float *lens = cg_scopeGlintLenses[lensIndex].lens;
    float yaw = ents[client].lerpAngles[1] * 3.14159265f / 180.0f, local[3];
    for (int i = 0; i < 3; i++)
        local[i] = tagAt[i] + lens[i];
    out[0] = ents[client].lerpOrigin[0] + local[0] * cosf(yaw) - local[1] * sinf(yaw);
    out[1] = ents[client].lerpOrigin[1] + local[0] * sinf(yaw) + local[1] * cosf(yaw);
    out[2] = ents[client].lerpOrigin[2] + local[2];
}
static void aimFrom(int client, const float *lens, float degrees)
{
    float d[3];
    for (int i = 0; i < 3; i++)
        d[i] = cg->refdef.vieworg[i] - lens[i];
    cg->bgs.clientinfo[client].playerAngles[0] = -atan2f(d[2], sqrtf(d[0] * d[0] + d[1] * d[1])) * 180.0f / 3.14159265f;
    cg->bgs.clientinfo[client].playerAngles[1] = atan2f(d[1], d[0]) * 180.0f / 3.14159265f + degrees;
}
/* Aims the player degrees beside the eye, seen from his lens. */
static void aim(int client, int lensIndex, float degrees)
{
    float lens[3];
    lensAt(client, lensIndex, lens);
    aimFrom(client, lens, degrees);
}
/* The eye at (0, 0, 60) looking along +x, its left +y; a 60 degree high view. */
static void look(void)
{
    memset(&cg->refdef, 0, sizeof(cg->refdef));
    cg->refdef.fov_y = 60.0f;
    cg->refdef.vieworg[2] = 60.0f;
    cg->refdef.viewaxis[0][0] = cg->refdef.viewaxis[1][1] = cg->refdef.viewaxis[2][2] = 1.0f;
}
/* One frame at time: every present player added, then the glints drawn.
 * Returns the glints drawn. */
static int frame(int time)
{
    cg->clientFrame++;
    cg->time = time;
    polyCount = framedVerts = traces = 0;
    for (int i = 0; i < 64; i++)
        if (present[i])
            CG_ScopeGlintAddPlayer(&ents[i]);
    CG_DrawScopeGlints();
    assert(polyCount % 3 == 0 && framedVerts == polyCount * 4);
    lastGlints = polyCount / 3;
    return lastGlints;
}
/* The first time from after at which client's shimmer is at its brightest. */
static int peak(int client, int after)
{
    int t = after;
    while ((t + client * 211) % PERIOD)
        t++;
    return t;
}
/* Frames every 16 ms after time, up to client's next shimmer peak, which it
 * returns unframed. */
static int toPeak(int client, int time)
{
    int end = peak(client, time + 16);
    for (time += 16; time < end; time += 16)
        frame(time);
    return end;
}
/* Frames every 16 ms from time until well after a raise, ending on and
 * framing client's shimmer peak, which it returns. */
static int settle(int client, int time)
{
    int end = peak(client, time + 266);
    for (; time < end; time += 16)
        frame(time);
    frame(end);
    return end;
}
static float alphaOf(int glint) { return polys[glint * 3].verts[0].color.array[3] / 255.0f; }
/* The quad's centre and half extents along the view's left and up. */
static void quadShape(int index, float *centre, float *width, float *height)
{
    const GfxWorldVertex *v = polys[index].verts;
    for (int i = 0; i < 3; i++)
        centre[i] = 0.25f * (v[0].xyz[i] + v[1].xyz[i] + v[2].xyz[i] + v[3].xyz[i]);
    *width = 0.5f * (v[0].xyz[1] - v[1].xyz[1]);
    *height = 0.5f * (v[0].xyz[2] - v[3].xyz[2]);
}
static void clear(void)
{
    memset(present, 0, sizeof(present));
    traceFraction = 1.0f;
    traceEntity = 1023;
    smoke = 1.0f;
}

int main(void)
{
    float lens[3], centre[3], width, height, distance, toEye[3];
    int cases = 0, t = 100000;

    /* The lenses are found by their world models in any case, nothing else. */
    for (int i = 0; i < CG_SCOPEGLINT_LENS_COUNT; i++) {
        char upper[64];
        snprintf(upper, sizeof(upper), "%s", cg_scopeGlintLenses[i].worldModel);
        for (char *c = upper; *c; c++)
            *c = (char)(*c >= 'a' && *c <= 'z' ? *c - 32 : *c);
        assert(CG_ScopeGlintFindLens(cg_scopeGlintLenses[i].worldModel) == &cg_scopeGlintLenses[i]);
        assert(CG_ScopeGlintFindLens(upper) == &cg_scopeGlintLenses[i]);
    }
    assert(!CG_ScopeGlintFindLens(NULL) && !CG_ScopeGlintFindLens("xmodel/weapon_kar98") &&
           !CG_ScopeGlintFindLens("xmodel/weapon_kar98_scoped_obj"));
    cases++;

    /* The switch: cg_scopeGlint, archived, on. */
    CG_ScopeGlintInit();
    assert(!strcmp(dvarName, "cg_scopeGlint") && dvarValue == 1 && dvarFlags == 0x1001);
    cases++;

    cg->nextSnap = &snap;
    look();
    viewer(0, TEAM_ALLIES);

    /* An Axis sniper 1000 units ahead facing the eye: nothing while his
     * sights are down. He raises his scoped Kar98k: nothing as they start up,
     * a little more each frame, half of it half way through the 250 ms raise,
     * all of it once up. */
    player(1, TEAM_AXIS, 1000, 0, 0, 180, 1, 0);
    aim(1, 0, 0);
    assert(frame(t) == 0 && traces == 0);
    {
        int start = peak(1, t + 400) - 125;
        float last = 0.0f;
        ents[1].nextState.eFlags = ADS;
        assert(frame(start) == 0 && traces == 0);
        for (int ms = 16; ms < 125; ms += 16) {
            assert(frame(start + ms) == 1 && alphaOf(0) < FULL * 0.5f && alphaOf(0) >= last);
            last = alphaOf(0);
        }
        assert(frame(start + 125) == 1 && near(alphaOf(0), FULL * 0.5f, LEVEL));
        t = settle(1, start + 125);
        assert(lastGlints == 1 && near(alphaOf(0), FULL, LEVEL));
    }
    cases++;

    /* The three quads, each framed before it is added, in the tracer's
     * material without a lightmap: the flare about the lens, two units
     * toward the eye, a streak three times as long across it and a spike
     * half as long up it, weighted to whiten the tracer's yellow image. */
    lensAt(1, 0, lens);
    for (int i = 0; i < 3; i++)
        toEye[i] = cg->refdef.vieworg[i] - lens[i];
    distance = sqrtf(toEye[0] * toEye[0] + toEye[1] * toEye[1] + toEye[2] * toEye[2]);
    for (int k = 0; k < 3; k++) {
        assert(polys[k].material == &tracer && polys[k].lmapIndex == 0x1f && polys[k].framed);
        quadShape(k, centre, &width, &height);
        for (int i = 0; i < 3; i++)
            assert(near(centre[i], lens[i] + toEye[i] / distance * 2.0f, 2e-3f));
    }
    {
        float radius = distance * tan30 * SCREEN_RADIUS;
        const GfxWorldVertex *v = polys[0].verts;
        quadShape(0, centre, &width, &height);
        assert(near(width, radius, 2e-3f) && near(height, radius, 2e-3f));
        quadShape(1, centre, &width, &height);
        assert(near(width, 3.0f * radius, 5e-3f) && near(height, 0.3f * radius, 2e-3f));
        quadShape(2, centre, &width, &height);
        assert(near(width, 0.25f * radius, 2e-3f) && near(height, 1.5f * radius, 2e-3f));
        /* The first corner is the upper left, round to the upper right, the
         * lower right and the lower left. */
        assert(v[0].xyz[1] > v[1].xyz[1] && v[0].xyz[2] > v[3].xyz[2] && v[2].xyz[1] < v[3].xyz[1]);
        assert(v[0].texCoord[0] == 1 && v[0].texCoord[1] == 0 && v[1].texCoord[0] == 0 && v[1].texCoord[1] == 0 &&
               v[2].texCoord[0] == 0 && v[2].texCoord[1] == 1 && v[3].texCoord[0] == 1 && v[3].texCoord[1] == 1);
        for (int i = 0; i < 4; i++)
            assert(v[i].lmapCoord[0] == v[i].texCoord[0] && v[i].lmapCoord[1] == v[i].texCoord[1]);
        assert(v[0].color.array[0] == (byte)(255.0f * FULL * 0.52f + 0.5f) &&
               v[0].color.array[1] == (byte)(255.0f * FULL * 0.54f + 0.5f) &&
               v[0].color.array[2] == (byte)(255.0f * FULL + 0.5f));
        assert(polys[1].verts[2].color.array[3] == (byte)(255.0f * FULL * 0.55f + 0.5f) &&
               polys[2].verts[1].color.array[3] == polys[1].verts[2].color.array[3]);
    }
    /* The scope must be in sight: a trace from the eye to the lens, past the
     * viewer, through the world and players. */
    assert(traces == 1 && traceSkip == 0 && traceMask == 0x2000001);
    for (int i = 0; i < 3; i++)
        assert(traceStart[i] == cg->refdef.vieworg[i] && near(traceEnd[i], lens[i], 2e-3f));
    cases++;

    /* All of it within 6 degrees of his aim, half at 15, none from 24; the
     * flare narrows to 60% toward the edge. */
    {
        static const float angles[] = { 0, 3, 5.9f, 9, 15, 20, 23.9f, 24.1f, 40, 90, 180 };
        float last = 2.0f;
        for (unsigned k = 0; k < sizeof(angles) / sizeof(angles[0]); k++) {
            float a = angles[k], s = (24.0f - a) / 18.0f, facing;
            s = s > 1 ? 1 : s < 0 ? 0 : s;
            facing = s * s * (3 - 2 * s);
            aim(1, 0, a);
            t = toPeak(1, t);
            if (facing <= 0.0f) {
                assert(frame(t) == 0);
                continue;
            }
            assert(frame(t) == 1 && near(alphaOf(0), FULL * facing, LEVEL + 2e-3f) && alphaOf(0) <= last);
            last = alphaOf(0);
            quadShape(0, centre, &width, &height);
            assert(near(width, distance * tan30 * SCREEN_RADIUS * (0.6f + 0.4f * facing), 3e-3f));
        }
        /* Pitched off the eye just as much. */
        aim(1, 0, 0);
        cg->bgs.clientinfo[1].playerAngles[0] += 15.0f;
        t = toPeak(1, t);
        assert(frame(t) == 1 && near(alphaOf(0), FULL * 0.5f, LEVEL + 2e-3f));
        aim(1, 0, 0);
    }
    cases++;

    /* Lowered, nothing; raised again it comes in from nothing. */
    ents[1].nextState.eFlags = 0;
    assert(frame(t + 16) == 0);
    ents[1].nextState.eFlags = ADS;
    assert(frame(t + 32) == 0);
    t = settle(1, t + 32);
    assert(lastGlints == 1 && near(alphaOf(0), FULL, LEVEL));
    /* Out of view for up to 200 ms it stays up; for longer, or after the
     * clock goes back, it comes in again. */
    present[1] = 0;
    for (int ms = 16; ms < 150; ms += 16)
        assert(frame(t + ms) == 0);
    present[1] = 1;
    assert(frame(t + 150) == 1 && alphaOf(0) >= 0.75f * FULL - LEVEL);
    present[1] = 0;
    for (int ms = 166; ms < 360; ms += 16)
        frame(t + ms);
    present[1] = 1;
    assert(frame(t + 360) == 0);
    t = settle(1, t + 360);
    assert(lastGlints == 1 && near(alphaOf(0), FULL, LEVEL));
    assert(frame(t - 5000) == 0);
    t = settle(1, t - 5000);
    assert(lastGlints == 1 && near(alphaOf(0), FULL, LEVEL));
    cases++;

    /* The shimmer: from all of the flare to 75% and back over 900 ms. */
    {
        float lo = 2, hi = 0;
        for (int ms = 15; ms <= PERIOD; ms += 15) {
            assert(frame(t + ms) == 1);
            lo = fminf(lo, alphaOf(0));
            hi = fmaxf(hi, alphaOf(0));
        }
        assert(near(hi, FULL, LEVEL) && near(lo, 0.75f * FULL, 2 * LEVEL));
        t += PERIOD;
    }
    cases++;

    /* Only a rival's: never a teammate's, anyone's without teams, everyone's
     * to a spectator, never a spectator's or an unknown player's, never the
     * viewer's own. */
    cg->bgs.clientinfo[1].team = TEAM_ALLIES;
    assert(frame(t + 16) == 0);
    cg->bgs.clientinfo[0].team = cg->bgs.clientinfo[1].team = TEAM_FREE;
    assert(frame(t + 32) == 1);
    cg->bgs.clientinfo[0].team = TEAM_SPECTATOR;
    cg->bgs.clientinfo[1].team = TEAM_ALLIES;
    assert(frame(t + 48) == 1);
    cg->bgs.clientinfo[1].team = TEAM_SPECTATOR;
    for (int team = TEAM_FREE; team <= TEAM_SPECTATOR; team++) {
        cg->bgs.clientinfo[0].team = (team_t)team;
        assert(frame(t + 52 + team) == 0);
    }
    cg->bgs.clientinfo[0].team = TEAM_ALLIES;
    cg->bgs.clientinfo[1].team = TEAM_AXIS;
    cg->bgs.clientinfo[1].infoValid = 0;
    assert(frame(t + 80) == 0);
    cg->bgs.clientinfo[1].infoValid = 1;
    snap.ps.clientNum = 1;
    assert(frame(t + 92) == 0);
    cg->bgs.clientinfo[1].team = TEAM_FREE;
    assert(frame(t + 96) == 0);
    cg->bgs.clientinfo[1].team = TEAM_AXIS;
    snap.ps.clientNum = 64;
    assert(frame(t + 112) == 0);
    snap.ps.clientNum = 0;
    assert(frame(t + 128) == 1);
    t += 128;
    cases++;

    /* Only a scope of its own: not an iron-sighted rifle, binoculars, a scope
     * the table lacks or a gun without a scope on a scoped rifle's model;
     * each looked up once. The other rifles' lenses are theirs, the
     * Springfield's whatever its name's case. */
    {
        static const int without[] = { 2, 3, 4, 8, 0, -1, WEAPONS + 1, 128, 200 };
        static const int rifles[] = { 5, 6, 7 }, lenses[] = { 1, 2, 3 };
        const unsigned count = sizeof(without) / sizeof(without[0]);
        int lookups = defLookups;
        for (unsigned k = 0; k < 2 * count; k++) {
            ents[1].nextState.weapon = without[k % count];
            t += 16;
            assert(frame(t) == 0);
        }
        assert(defLookups == lookups + 4);
        for (int k = 0; k < 3; k++) {
            ents[1].nextState.weapon = rifles[k];
            aim(1, lenses[k], 0);
            t = settle(1, t + 16);
            assert(lastGlints == 1 && near(alphaOf(0), FULL, LEVEL));
            lensAt(1, lenses[k], lens);
            for (int i = 0; i < 3; i++)
                assert(near(traceEnd[i], lens[i], 2e-3f));
        }
        ents[1].nextState.weapon = 1;
        aim(1, 0, 0);
        t = settle(1, t + 16);
    }
    cases++;

    /* Not from a player on a turret, hidden or dead, and his raise starts
     * over; not without his model or its tag_flash. */
    {
        static const int flags[] = { 0x100, 0x200, 0x20, 0x20000 };
        for (int k = 0; k < 4; k++) {
            t = settle(1, t + 16);
            ents[1].nextState.eFlags = ADS | flags[k];
            assert(frame(t + 16) == 0);
            ents[1].nextState.eFlags = ADS;
            assert(frame(t + 32) == 0);
            t += 32;
        }
        t = settle(1, t + 16);
        noDObj[1] = 1;
        assert(frame(t + 16) == 0);
        noDObj[1] = 0;
        noTag[1] = 1;
        assert(frame(t + 32) == 0);
        noTag[1] = 0;
        assert(frame(t + 48) == 1);
        t += 48;
    }
    cases++;

    /* A wall or another player before the lens hides it; the trace ending
     * on the sniper's own body does not. Smoke the server would not see
     * through hides it, thinner smoke dims it. */
    traceFraction = 0.5f;
    traceEntity = 1022;
    assert(frame(t + 16) == 0 && traces == 1);
    traceEntity = 5;
    assert(frame(t + 32) == 0);
    traceFraction = 0.99f;
    traceEntity = 1;
    assert(frame(t + 48) == 1);
    traceFraction = 1.0f;
    traceEntity = 1023;
    smoke = 0.19f;
    assert(frame(t + 64) == 0);
    smoke = NAN;
    assert(frame(t + 80) == 0);
    smoke = 0.5f;
    t = toPeak(1, t + 80);
    assert(frame(t) == 1 && near(alphaOf(0), FULL * 0.5f, LEVEL));
    smoke = 1.0f;
    cases++;

    /* A far scope keeps its share of the view's height, through a zoom too;
     * a near one its 1.5 units; none within 24 units of the eye. */
    player(1, TEAM_AXIS, 4000, 0, 0, 180, 1, ADS);
    aim(1, 0, 0);
    assert(frame(t + 16) == 1);
    lensAt(1, 0, lens);
    distance = sqrtf(lens[0] * lens[0] + lens[1] * lens[1] + (lens[2] - 60.0f) * (lens[2] - 60.0f));
    quadShape(0, centre, &width, &height);
    assert(near(width, distance * tan30 * SCREEN_RADIUS, 1e-2f));
    cg->refdef.fov_y = 8.0f;
    assert(frame(t + 32) == 1);
    quadShape(0, centre, &width, &height);
    assert(near(width, distance * tanf(4.0f * 3.14159265f / 180.0f) * SCREEN_RADIUS, 1e-2f));
    cg->refdef.fov_y = 60.0f;
    player(1, TEAM_AXIS, 60, 0, 0, 180, 1, ADS);
    aim(1, 0, 0);
    assert(frame(t + 48) == 1);
    quadShape(0, centre, &width, &height);
    assert(near(width, MIN_RADIUS, 1e-4f) && near(height, MIN_RADIUS, 1e-4f));
    player(1, TEAM_AXIS, 25, 0, 0, 180, 1, ADS);
    aim(1, 0, 0);
    assert(frame(t + 64) == 0 && traces == 0);
    t += 64;
    cases++;

    /* The lens rides tag_flash: turned with the gun, pitched up 30 degrees
     * and rolled 20 about its barrel, it is carried round the tag in the
     * model, then by the player's yaw. His aim is his view's, not his
     * model's. */
    {
        float th = -30.0f * 3.14159265f / 180.0f, roll = 20.0f * 3.14159265f / 180.0f;
        float qp[4] = { 0, sinf(th / 2), 0, cosf(th / 2) }, qr[4] = { sinf(roll / 2), 0, 0, cosf(roll / 2) };
        float fwd[3] = { cosf(th), 0, -sinf(th) }, up0[3] = { sinf(th), 0, cosf(th) };
        float left[3], up[3], local[3], expect[3], yaw = 150.0f * 3.14159265f / 180.0f;
        const float *l = cg_scopeGlintLenses[0].lens;
        player(1, TEAM_AXIS, 800, 100, 0, 150, 1, ADS);
        /* The roll about the barrel, then the pitch. */
        tags[1].quat[0] = qp[3] * qr[0] + qp[0] * qr[3] + qp[1] * qr[2] - qp[2] * qr[1];
        tags[1].quat[1] = qp[3] * qr[1] - qp[0] * qr[2] + qp[1] * qr[3] + qp[2] * qr[0];
        tags[1].quat[2] = qp[3] * qr[2] + qp[0] * qr[1] - qp[1] * qr[0] + qp[2] * qr[3];
        tags[1].quat[3] = qp[3] * qr[3] - qp[0] * qr[0] - qp[1] * qr[1] - qp[2] * qr[2];
        for (int i = 0; i < 3; i++) {
            float y = i == 1 ? 1.0f : 0.0f;
            left[i] = y * cosf(roll) + up0[i] * sinf(roll);
            up[i] = up0[i] * cosf(roll) - y * sinf(roll);
            local[i] = tagAt[i] + l[0] * fwd[i] + l[1] * left[i] + l[2] * up[i];
        }
        expect[0] = 800 + local[0] * cosf(yaw) - local[1] * sinf(yaw);
        expect[1] = 100 + local[0] * sinf(yaw) + local[1] * cosf(yaw);
        expect[2] = local[2];
        aimFrom(1, expect, 0);
        t = toPeak(1, t);
        assert(frame(t) == 1 && near(alphaOf(0), FULL, LEVEL));
        for (int i = 0; i < 3; i++)
            assert(near(traceEnd[i], expect[i], 2e-3f));
    }
    cases++;

    /* Drawn once a frame, and only in the frame its players were noted. */
    player(1, TEAM_AXIS, 1000, 0, 0, 180, 1, ADS);
    aim(1, 0, 0);
    assert(frame(t + 16) == 1);
    polyCount = 0;
    CG_DrawScopeGlints();
    assert(polyCount == 0);
    cg->clientFrame++;
    cg->time = t + 32;
    CG_ScopeGlintAddPlayer(&ents[1]);
    cg->clientFrame++;
    cg->time = t + 48;
    CG_DrawScopeGlints();
    assert(polyCount == 0);
    assert(frame(t + 64) == 1);
    t += 64;
    cases++;

    /* Every rival sniper in view glints, 63 of them within the notes. */
    clear();
    for (int c = 1; c < 64; c++) {
        player(c, TEAM_AXIS, 1000, (float)(c - 32) * 30.0f, 0, 180, 1, ADS);
        aim(c, 0, 0);
    }
    for (int ms = 16; ms <= 300; ms += 16)
        frame(t + ms);
    assert(frame(t + 316) == 63);
    t += 316;
    clear();
    cases++;

    /* Off, nothing at all. */
    player(1, TEAM_AXIS, 1000, 0, 0, 180, 1, ADS);
    aim(1, 0, 0);
    assert(frame(t + 16) == 1);
    glintDvar.current.enabled = 0;
    assert(frame(t + 32) == 0 && traces == 0);
    glintDvar.current.enabled = 1;
    assert(frame(t + 48) == 1);
    cases++;

    printf("%d\n", cases);
    return 0;
}
'''

mutants = [
    ('#define CG_SCOPEGLINT_ADS 0x40000', '#define CG_SCOPEGLINT_ADS 0x4000'),
    ('#define CG_SCOPEGLINT_FULL_DEG 6.0f', '#define CG_SCOPEGLINT_FULL_DEG 10.0f'),
    ('#define CG_SCOPEGLINT_EDGE_DEG 24.0f', '#define CG_SCOPEGLINT_EDGE_DEG 45.0f'),
    ('#define CG_SCOPEGLINT_RAISE_MSEC 250', '#define CG_SCOPEGLINT_RAISE_MSEC 100'),
    ('#define CG_SCOPEGLINT_GAP_MSEC 200', '#define CG_SCOPEGLINT_GAP_MSEC 400'),
    ('#define CG_SCOPEGLINT_SCREEN_RADIUS 0.02f', '#define CG_SCOPEGLINT_SCREEN_RADIUS 0.03f'),
    ('#define CG_SCOPEGLINT_MIN_RADIUS 1.5f', '#define CG_SCOPEGLINT_MIN_RADIUS 3.0f'),
    ('#define CG_SCOPEGLINT_BRIGHTNESS 1.0f', '#define CG_SCOPEGLINT_BRIGHTNESS 0.85f'),
    ('#define CG_SCOPEGLINT_SHIMMER 0.25f', '#define CG_SCOPEGLINT_SHIMMER 0.5f'),
    ('#define CG_SCOPEGLINT_PULL 2.0f', '#define CG_SCOPEGLINT_PULL -2.0f'),
    ('#define CG_SCOPEGLINT_NEAR 24.0f', '#define CG_SCOPEGLINT_NEAR 0.0f'),
    ('#define CG_SCOPEGLINT_SIGHT_MASK 0x2000001', '#define CG_SCOPEGLINT_SIGHT_MASK 0x1'),
    ('#define CG_SCOPEGLINT_MIN_VISIBILITY 0.2f', '#define CG_SCOPEGLINT_MIN_VISIBILITY 0.1f'),
    ('static const float s_glintColor[3] = { 0.52f, 0.54f, 1.0f };',
     'static const float s_glintColor[3] = { 1.0f, 0.94f, 0.82f };'),
    ('    return CG_ScopeGlintSmoothstep(CG_SCOPEGLINT_EDGE_DEG, CG_SCOPEGLINT_FULL_DEG, degrees);',
     '    return CG_ScopeGlintSmoothstep(CG_SCOPEGLINT_FULL_DEG, CG_SCOPEGLINT_EDGE_DEG, degrees);'),
    ('    return ci->infoValid && ci->team != TEAM_SPECTATOR && (viewTeam == TEAM_FREE || ci->team != viewTeam);',
     '    return ci->infoValid && (viewTeam == TEAM_FREE || ci->team != viewTeam);'),
    ('    return ci->infoValid && ci->team != TEAM_SPECTATOR && (viewTeam == TEAM_FREE || ci->team != viewTeam);',
     '    return ci->infoValid && ci->team != TEAM_SPECTATOR && ci->team != viewTeam;'),
    ('    return ci->infoValid && ci->team != TEAM_SPECTATOR && (viewTeam == TEAM_FREE || ci->team != viewTeam);',
     '    return ci->team != TEAM_SPECTATOR && (viewTeam == TEAM_FREE || ci->team != viewTeam);'),
    ('        if (weapDef && weapDef->overlayReticle && weapDef->weapClass != 9 && weapDef->szOverlayMaterial[0])',
     '        if (weapDef && weapDef->overlayReticle && weapDef->szOverlayMaterial[0])'),
    ('        if (weapDef && weapDef->overlayReticle && weapDef->weapClass != 9 && weapDef->szOverlayMaterial[0])',
     '        if (weapDef && weapDef->weapClass != 9)'),
    ('        if (!I_stricmp(cg_scopeGlintLenses[i].worldModel, worldModel))',
     '        if (!strcmp(cg_scopeGlintLenses[i].worldModel, worldModel))'),
    ('        s_lensIndex[weapon] = lens ? (signed char)(lens - cg_scopeGlintLenses) : CG_SCOPEGLINT_NONE;\n', ''),
    ('    if (!(es->eFlags & CG_SCOPEGLINT_ADS) || (es->eFlags & (CG_SCOPEGLINT_TURRET | CG_SCOPEGLINT_GONE))) {',
     '    if (!(es->eFlags & CG_SCOPEGLINT_ADS)) {'),
    ('        s_raisedAt[clientNum] = -1;\n        return;\n', '        return;\n'),
    ('    if (s_raisedAt[clientNum] < 0 || time < s_raisedAt[clientNum] || time < s_seenAt[clientNum] ||',
     '    if (s_raisedAt[clientNum] < 0 ||'),
    ('    if (!lens || clientNum == viewer || viewer < 0 || viewer >= CG_SCOPEGLINT_CLIENTS ||',
     '    if (!lens || viewer < 0 || viewer >= CG_SCOPEGLINT_CLIENTS ||'),
    ('        local[i] = tagOrigin[i] + lens[0] * tagAxis[0][i] + lens[1] * tagAxis[1][i] + lens[2] * tagAxis[2][i];',
     '        local[i] = tagOrigin[i] + lens[0] * tagAxis[i][0] + lens[1] * tagAxis[i][1] + lens[2] * tagAxis[i][2];'),
    ('        out[i] = origin[i] + local[0] * axis[0][i] + local[1] * axis[1][i] + local[2] * axis[2][i];',
     '        out[i] = origin[i] + local[0] * axis[i][0] + local[1] * axis[i][1] + local[2] * axis[i][2];'),
    ('    AngleVectors(cg->bgs.clientinfo[clientNum].playerAngles, sight->forward, NULL, NULL);',
     '    AngleVectors(cent->lerpAngles, sight->forward, NULL, NULL);'),
    ('        facing = CG_ScopeGlintFacing(sight->forward, toEye) * sight->raise;',
     '        facing = CG_ScopeGlintFacing(sight->forward, toEye);'),
    ('        if (trace.fraction < 1.0f && trace.entityNum != sight->clientNum)', '        if (trace.fraction < 1.0f)'),
    ('        if (trace.fraction < 1.0f && trace.entityNum != sight->clientNum)\n            continue;\n', ''),
    ('        if (!(visibility >= CG_SCOPEGLINT_MIN_VISIBILITY))', '        if (visibility < CG_SCOPEGLINT_MIN_VISIBILITY)'),
    ('CG_SCOPEGLINT_BRIGHTNESS * facing * visibility * CG_ScopeGlintShimmer(cg->time, sight->clientNum);',
     'CG_SCOPEGLINT_BRIGHTNESS * facing * CG_ScopeGlintShimmer(cg->time, sight->clientNum);'),
    ('                 (CG_SCOPEGLINT_EDGE_SIZE + (1.0f - CG_SCOPEGLINT_EDGE_SIZE) * facing);', '                 1.0f;'),
    ('    return radius > CG_SCOPEGLINT_MIN_RADIUS ? radius : CG_SCOPEGLINT_MIN_RADIUS;', '    return radius;'),
    ('    float phase = (float)((unsigned int)(time + clientNum * 211) % CG_SCOPEGLINT_SHIMMER_MSEC) /',
     '    float phase = (float)((unsigned int)time % CG_SCOPEGLINT_SHIMMER_MSEC) /'),
    ('    CG_ScopeGlintQuad(centre, left, up, CG_SCOPEGLINT_STREAK_LENGTH * radius, CG_SCOPEGLINT_STREAK_WIDTH * radius,',
     '    CG_ScopeGlintQuad(centre, left, up, CG_SCOPEGLINT_STREAK_WIDTH * radius, CG_SCOPEGLINT_STREAK_LENGTH * radius,'),
    ('    CG_ScopeGlintColor(brightness * CG_SCOPEGLINT_STREAK, streak);', '    CG_ScopeGlintColor(brightness, streak);'),
    ('    color[3] = (byte)(255.0f * brightness + 0.5f);', '    color[3] = 255;'),
    ('        verts[i].texCoord[1] = verts[i].lmapCoord[1] = corners[i][1] > 0.0f ? 0.0f : 1.0f;',
     '        verts[i].texCoord[1] = verts[i].lmapCoord[1] = corners[i][1] > 0.0f ? 1.0f : 0.0f;'),
    ('    if (s_sightFrame != cg->clientFrame)\n        return;\n', ''),
    ('        }\n    }\n    s_sightCount = 0;\n}\n', '        }\n    }\n}\n'),
    ('            CL_DefaultVertexFrames(4, &verts[4 * k]);\n', ''),
    ('            CL_AddPolyToScene(cgs->media.tracerMaterial, 0x1f, 4, &verts[4 * k]);',
     '            CL_AddPolyToScene(cgs->media.tracerMaterial, 0, 4, &verts[4 * k]);'),
    ('    if (!cg_scopeGlint || !cg_scopeGlint->current.enabled || clientNum < 0 || clientNum >= CG_SCOPEGLINT_CLIENTS)',
     '    if (!cg_scopeGlint || clientNum < 0 || clientNum >= CG_SCOPEGLINT_CLIENTS)'),
    ('    cg_scopeGlint = Dvar_RegisterBool_mac("cg_scopeGlint", 1, 0x1001);',
     '    cg_scopeGlint = Dvar_RegisterBool_mac("cg_scopeGlint", 1, 0x1000);'),
]


def mutate(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new)


def run(directory, name, source, flags, expect_pass):
    path = Path(directory) / f'{name}.c'
    binary = Path(directory) / name
    path.write_text(source)
    subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                    *flags, str(path), '-lm', '-o', str(binary)], check=True)
    result = subprocess.run([str(binary)], capture_output=True, text=True)
    assert (result.returncode == 0) == expect_pass, (name, result.stdout[-500:], result.stderr[-3000:])
    return result


strict = ['-Wall', '-Werror', '-Wno-unused-function']
with tempfile.TemporaryDirectory(prefix='cod2-scope-glint-') as directory:
    cases = int(run(directory, 'glint', support + body + checks, strict, True).stdout)
    for index, (before, after) in enumerate(mutants):
        run(directory, f'glint{index}', support + mutate(body, before, after) + checks, ['-w'], False)


# The retail assets, when present: later IWDs override earlier ones.
archives = [zipfile.ZipFile(path) for path in sorted((root / 'data/main').glob('iw_*.iwd'))]


def asset(name):
    data = None
    for archive in archives:
        try:
            data = archive.read(name)
        except KeyError:
            pass
    return data


def cstring(data, offset):
    end = data.index(b'\0', offset)
    return data[offset:end].decode('latin1'), end + 1


def world_model(name):
    """tag_flash's parent, offset and rotation, and every vertex, of a world
    model whose surfaces all ride its root."""
    data = asset(name)
    offset = 27
    lods = []
    for _ in range(4):
        offset += 4
        lod, offset = cstring(data, offset)
        lods.append(lod)
    parts = asset('xmodelparts/' + lods[0])
    count, roots = struct.unpack_from('<hh', parts, 2)
    bones = []
    offset = 6
    for _ in range(count):
        bones.append((parts[offset], struct.unpack_from('<3f', parts, offset + 1),
                      struct.unpack_from('<3h', parts, offset + 13)))
        offset += 19
    names = []
    for _ in range(roots + count):
        bone, offset = cstring(parts, offset)
        names.append(bone)
    parent, flash_offset, flash_quat = bones[names.index('tag_flash') - roots]
    surfs = asset('xmodelsurfs/' + lods[0])
    points = []
    offset = 4
    for _ in range(struct.unpack_from('<h', surfs, 2)[0]):
        offset += 1
        vertices, triangles = struct.unpack_from('<HH', surfs, offset)
        offset += 4
        rigid = struct.unpack_from('<h', surfs, offset)[0]
        offset += 2 if rigid != -1 else 4
        for _ in range(vertices):
            offset += 12 + 4 + 8 + 24
            if rigid != -1:
                bone = rigid
            else:
                weights = surfs[offset]
                bone = struct.unpack_from('<h', surfs, offset + 1)[0]
                offset += 3
            position = struct.unpack_from('<3f', surfs, offset)
            offset += 12
            if rigid == -1 and weights:
                offset += 1 + 16 * weights
            assert bone == 0, (name, names[bone])
            points.append(position)
        offset += 6 * triangles
    assert offset == len(surfs), name
    return parent, flash_offset, flash_quat, points


table = re.search(r'^const CgScopeGlintLens cg_scopeGlintLenses\[CG_SCOPEGLINT_LENS_COUNT\] = \{\n(.*?)^\};\n', module,
                  re.M | re.S)[1]
rows = re.findall(r'\{ "([\w/]+)", \{ ([-\d.]+)f, ([-\d.]+)f, ([-\d.]+)f \} \},', table)
assert len(rows) == int(re.search(r'#define CG_SCOPEGLINT_LENS_COUNT (\d+)', header)[1])
asset_note = 'no retail IWDs in data/main: the lens table was not checked against the world models'
if archives:
    for model, *numbers in rows:
        lens = [float(v) for v in numbers]
        parent, flash, quat, points = world_model(model)
        # tag_flash hangs unturned off the root, so its space is the model's
        # moved to the muzzle.
        assert parent == 0 and quat == (0, 0, 0), (model, parent, quat)
        centre = [flash[i] + lens[i] for i in range(3)]

        def radial(p):
            return math.hypot(p[1] - centre[1], p[2] - centre[2])

        # Nothing of the scope's tube reaches past the lens; the ring that
        # ends it is centred on the lens and goes all the way round.
        tube = [p for p in points if abs(p[0] - centre[0]) < 8 and radial(p) < 0.9]
        assert abs(max(p[0] for p in tube) - centre[0]) < 0.05, (model, max(p[0] for p in tube), centre[0])
        ring = [p for p in points if abs(p[0] - centre[0]) < 0.6 and radial(p) < 1.0]
        assert abs((min(p[1] for p in ring) + max(p[1] for p in ring)) / 2 - centre[1]) < 0.1, model
        assert abs((min(p[2] for p in ring) + max(p[2] for p in ring)) / 2 - centre[2]) < 0.1, model
        sectors = {int(math.degrees(math.atan2(p[2] - centre[2], p[1] - centre[1])) // 30) for p in ring
                   if radial(p) > 0.25}
        assert len(sectors) >= 8, (model, sorted(sectors))
    # The table's rifles are the multiplayer weapons with a scope of their own.
    scoped = set()
    for name in sorted({name for archive in archives for name in archive.namelist()}):
        if name.startswith('weapons/mp/') and name != 'weapons/mp/':
            fields = asset(name).decode('latin1').split('\\')
            values = dict(zip(fields[1::2], fields[2::2]))
            if values.get('adsOverlayShader') and values.get('weaponClass') != 'item':
                scoped.add(values['worldModel'])
    assert scoped == {model for model, *_ in rows}, scoped
    asset_note = (f'the {len(rows)} lenses against their world models\' tubes, and the table against the '
                  f'multiplayer weapons with a scope of their own')

print(f'PASS: a player is noted once his model is in the scene, whatever his teleport bit, and the glints drawn once '
      f'the view is placed; {cases} '
      f'cases of a rival\'s raised sniper scope glinting at the eye: coming in over the raise, all of it within 6 '
      f'degrees of his aim and none from 24, shimmering, sized to the view, on its lens with the gun\'s tag, hidden '
      f'by walls, players and thick smoke and dimmed by thin, never for teammates, spectators, the viewer, a scope '
      f'the table lacks, binoculars, a turret, the hidden or the dead; {len(mutants)} mutants fail; {asset_note}')
