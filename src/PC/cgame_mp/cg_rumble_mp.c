#include "cod2_feature_config.h"

#if COD2_FEATURE_RUMBLE

#    include <string.h>
#    include <stdlib.h>
#    include <math.h>

#    define __m128 __m128_cod2
#    include "common_types.h"
#    undef __m128
#    include "headers/PC/cgame_mp/cg_local.h"
#    include "headers/PC/cgame_mp/cg_rumble.h"

extern int FS_FOpenFileByMode(const char *filename, int *fh, int mode);
extern int FS_Read(void *buffer, int len, int fh);
extern void FS_FCloseFile(int fh);
extern const char *Com_Parse(const char **buf);
extern void Com_BeginParseSession(const char *name);
extern void Com_EndParseSession(void);
extern void Com_Printf(const char *fmt, ...);
extern void Com_DPrintf(const char *fmt, ...);
extern const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags);
extern void Cmd_AddCommand(const char *cmd, void (*func)(void));
extern void Cmd_ArgvBuffer(int arg, char *buf, int bufSize);
extern int Cmd_Argc(void);
extern int Sys_Milliseconds(void);
extern WeaponDef *BG_GetWeaponDef(int weaponIndex);
extern int BG_GetNumWeapons(void);
extern float Vec3Distance(const vec_t *a, const vec_t *b);

extern void CL_Gamepad_SetRumble(float lowFreq, float highFreq);

#    define MAX_RUMBLE_GRAPH_KNOTS 16
/* The retail IWDs ship 39 graph pairs. */
#    define MAX_RUMBLE_INFOS 64
#    define MAX_ACTIVE_RUMBLES 32
#    define RUMBLE_DURATION_MS 1000
/* One kick a round: an automatic weapon's graph spans its fire interval.
 * Single shots, bolts and rockets keep a longer tail. */
#    define RUMBLE_SHOT_MIN_MS 50
#    define RUMBLE_SEMI_FIRE_MS 250
#    define RUMBLE_BOLT_FIRE_MS 400
#    define RUMBLE_ROCKET_FIRE_MS 700
/* Under the kicks the motors keep a floor. An automatic weapon's lasts until
 * a little after its next round is due, so a burst buzzes without a break
 * until it stops: a reload, an empty magazine, the trigger let go. A single
 * shot's holds the strong motor long enough to spin up and be felt. */
#    define RUMBLE_BURST_LOW 0.4f
#    define RUMBLE_BURST_HIGH 0.3f
#    define RUMBLE_BURST_SLACK_MS 60
#    define RUMBLE_KICK_LOW 0.7f
#    define RUMBLE_KICK_HIGH 0.5f
#    define RUMBLE_KICK_MS 100
/* A blast is felt out to three times its damage radius, fading with distance. */
#    define RUMBLE_EXPLOSION_GRAPH "artillery_quake"
#    define RUMBLE_EXPLOSION_REACH 3.0f
/* earthquake() scales of a quarter and up shake the pad fully. */
#    define RUMBLE_EARTHQUAKE_GAIN 4.0f
/* The retail damage_light and damage_heavy graphs are the same: the hit's
 * size sets the strength, from 40% up to full for half the health. */
#    define RUMBLE_DAMAGE_GRAPH "damage_heavy"
#    define RUMBLE_DAMAGE_BASE 0.4f
#    define RUMBLE_DAMAGE_PER_POINT 0.012f
/* The server reports a melee impact without its attacker: one this close to
 * the view this soon after the local player's swing is theirs. */
#    define RUMBLE_MELEE_GRAPH "melee_medium"
#    define RUMBLE_MELEE_WINDOW_MS 1000
#    define RUMBLE_MELEE_REACH 128.0f

typedef struct {
    char name[64];
    int knotCount;
    vec2_t knots[MAX_RUMBLE_GRAPH_KNOTS];
    int loaded;
} RumbleGraph;

typedef struct {
    char name[64];
    RumbleGraph high;
    RumbleGraph low;
    int loaded;
} RumbleInfo;

typedef struct {
    int active;
    int infoIndex;
    int startTime;
    int duration;
    float scale;
    int loop;
} ActiveRumble;

static RumbleInfo g_rumbleInfos[MAX_RUMBLE_INFOS];
static int g_rumbleInfoCount;
static ActiveRumble g_activeRumbles[MAX_ACTIVE_RUMBLES];
static const dvar_t *cg_drawRumbleDebug;
static int g_rumbleMeleeTime;
static int g_rumbleMeleeWeapon;
/* The fire floor's motors and the Sys_Milliseconds it ends at. */
static float g_rumbleFloorLow, g_rumbleFloorHigh;
static int g_rumbleFloorEnd;

static int I_stricmp_local(const char *a, const char *b);

static qboolean CG_ParseRumbleGraph(RumbleGraph *graph, const char *buffer,
                                    const char *fileName)
{
    const char *bufPtr = buffer;
    const char *token;
    int declaredCount;
    int i;

    Com_BeginParseSession(fileName);

    token = Com_Parse(&bufPtr);
    if (I_stricmp_local(token, "RUMBLEGRAPHFILE") != 0) {
        Com_Printf("^1\"%s\" does not appear to be a rumble graph file\n", fileName);
        Com_EndParseSession();
        return 0;
    }

    token = Com_Parse(&bufPtr);
    declaredCount = atoi(token);

    for (i = 0; i < MAX_RUMBLE_GRAPH_KNOTS; ++i) {
        token = Com_Parse(&bufPtr);
        if (token[0] == '\0' || token[0] == '}')
            break;
        graph->knots[i][0] = (float)atof(token);

        token = Com_Parse(&bufPtr);
        if (token[0] == '\0' || token[0] == '}')
            break;
        graph->knots[i][1] = (float)atof(token);
    }

    if (i >= MAX_RUMBLE_GRAPH_KNOTS) {
        Com_Printf("^3WARNING: \"%s\" has too many graph knots\n", fileName);
        Com_EndParseSession();
        return 0;
    }
    Com_EndParseSession();

    if (declaredCount != i) {
        Com_Printf("^1\"%s\" Error in parsing a rumble graph file\n", fileName);
        return 0;
    }

    graph->knotCount = i;
    graph->loaded = 1;
    return 1;
}

static int I_stricmp_local(const char *a, const char *b)
{
    for (;;) {
        int ca = *a++, cb = *b++;
        if (ca >= 'A' && ca <= 'Z')
            ca += 32;
        if (cb >= 'A' && cb <= 'Z')
            cb += 32;
        if (ca != cb)
            return ca - cb;
        if (ca == '\0')
            return 0;
    }
}

static qboolean CG_ReadRumbleGraph(RumbleGraph *graph, const char *fileName)
{
    char path[256];
    char buffer[4096];
    int fh = 0;
    int len;

    if (!fileName || !fileName[0])
        return 0;

    {
        int n = 0;
        const char *p = "rumble/";
        while (*p)
            path[n++] = *p++;
        p = fileName;
        while (*p && n < (int)sizeof(path) - 5)
            path[n++] = *p++;
        p = ".rmb";
        while (*p)
            path[n++] = *p++;
        path[n] = '\0';
    }

    Com_DPrintf("Parsing rumble graph file \"%s\"..\n", path);

    len = FS_FOpenFileByMode(path, &fh, 0 );
    if (len <= 0 || !fh) {
        Com_Printf("^1Could not load rumble graph file \"%s\"\n", path);
        if (fh)
            FS_FCloseFile(fh);
        return 0;
    }
    if (len > (int)sizeof(buffer) - 1)
        len = (int)sizeof(buffer) - 1;
    FS_Read(buffer, len, fh);
    buffer[len] = '\0';
    FS_FCloseFile(fh);

    return CG_ParseRumbleGraph(graph, buffer, path);
}

static qboolean CG_LoadRumbleGraph(RumbleInfo *info, const char *highName,
                                   const char *lowName)
{
    info->high.loaded = 0;
    info->low.loaded = 0;

    if (highName && highName[0]) {
        if (!CG_ReadRumbleGraph(&info->high, highName))
            return 0;
    }
    if (lowName && lowName[0]) {
        if (!CG_ReadRumbleGraph(&info->low, lowName))
            return 0;
    }
    return (info->high.loaded || info->low.loaded);
}

int CG_GetRumbleInfoIndexFromName(const char *name)
{
    int i;
    for (i = 0; i < g_rumbleInfoCount; ++i) {
        if (I_stricmp_local(g_rumbleInfos[i].name, name) == 0)
            return i;
    }
    return -1;
}

int CG_LoadRumble(const char *name)
{
    int idx;
    char highName[64], lowName[64];
    RumbleInfo *info;

    if (!name || !name[0])
        return -1;

    idx = CG_GetRumbleInfoIndexFromName(name);
    if (idx >= 0)
        return g_rumbleInfos[idx].loaded ? idx : -1;

    if (g_rumbleInfoCount >= MAX_RUMBLE_INFOS) {
        Com_Printf("^1No more room to allocate anymore rumble infos\n");
        return -1;
    }

    {
        int n = 0;
        const char *p = name;
        while (*p && n < 60) {
            highName[n] = *p;
            lowName[n] = *p;
            n++;
            p++;
        }
        highName[n] = '_';
        highName[n + 1] = 'h';
        highName[n + 2] = '\0';
        lowName[n] = '_';
        lowName[n + 1] = 'l';
        lowName[n + 2] = '\0';
    }

    /* A missing graph keeps its entry, so it is looked for and reported once
     * rather than on every shot. */
    idx = g_rumbleInfoCount++;
    info = &g_rumbleInfos[idx];
    memset(info, 0, sizeof(*info));
    {
        int n = 0;
        const char *p = name;
        while (*p && n < 63) {
            info->name[n++] = *p++;
        }
        info->name[n] = '\0';
    }
    if (!CG_LoadRumbleGraph(info, highName, lowName)) {
        Com_Printf("^1Could not load rumble info file '%s'\n", name);
        return -1;
    }
    info->loaded = 1;
    return idx;
}

void CG_InvalidateActiveRumble(int index)
{
    if (index < 0 || index >= MAX_ACTIVE_RUMBLES)
        return;
    g_activeRumbles[index].active = 0;
    g_activeRumbles[index].infoIndex = -1;
    g_activeRumbles[index].loop = 0;
}

void CG_StopAllRumbles(void)
{
    int i;
    for (i = 0; i < MAX_ACTIVE_RUMBLES; ++i)
        CG_InvalidateActiveRumble(i);
    g_rumbleFloorLow = g_rumbleFloorHigh = 0.0f;
    CL_Gamepad_SetRumble(0.0f, 0.0f);
}

static int CG_FindClosestToDyingActiveRumble(void)
{
    int i, best = -1, now = Sys_Milliseconds();
    int bestRemaining = 0x7fffffff;
    for (i = 0; i < MAX_ACTIVE_RUMBLES; ++i) {
        int remaining;
        if (!g_activeRumbles[i].active)
            return i;
        if (g_activeRumbles[i].loop)
            continue;
        remaining = g_activeRumbles[i].duration - (now - g_activeRumbles[i].startTime);
        if (remaining < bestRemaining) {
            bestRemaining = remaining;
            best = i;
        }
    }
    return best;
}

void CG_RemoveInactiveRumbles(void)
{
    int i, now = Sys_Milliseconds();
    for (i = 0; i < MAX_ACTIVE_RUMBLES; ++i) {
        if (!g_activeRumbles[i].active)
            continue;
        if (g_activeRumbles[i].loop)
            continue;
        if (now - g_activeRumbles[i].startTime >= g_activeRumbles[i].duration)
            CG_InvalidateActiveRumble(i);
    }
}

static float CG_EvalRumbleGraph(const RumbleGraph *g, float t)
{
    int i;
    if (!g->loaded || g->knotCount <= 0)
        return 0.0f;
    if (t <= g->knots[0][0])
        return g->knots[0][1];
    if (t >= g->knots[g->knotCount - 1][0])
        return g->knots[g->knotCount - 1][1];
    for (i = 1; i < g->knotCount; ++i) {
        if (t <= g->knots[i][0]) {
            float t0 = g->knots[i - 1][0], v0 = g->knots[i - 1][1];
            float t1 = g->knots[i][0], v1 = g->knots[i][1];
            float span = t1 - t0;
            float frac = (span > 0.0f) ? (t - t0) / span : 0.0f;
            return v0 + (v1 - v0) * frac;
        }
    }
    return g->knots[g->knotCount - 1][1];
}

void CG_CalcActiveRumbles(void)
{
    int i, now = Sys_Milliseconds();
    float low = 0.0f, high = 0.0f;

    for (i = 0; i < MAX_ACTIVE_RUMBLES; ++i) {
        ActiveRumble *ar = &g_activeRumbles[i];
        RumbleInfo *info;
        float t;
        if (!ar->active)
            continue;
        if (ar->infoIndex < 0 || ar->infoIndex >= g_rumbleInfoCount)
            continue;
        info = &g_rumbleInfos[ar->infoIndex];

        t = (float)(now - ar->startTime) / (float)ar->duration;
        if (ar->loop) {
            t = t - (float)((int)t);
            if (t < 0.0f)
                t += 1.0f;
        } else if (t > 1.0f) {
            t = 1.0f;
        }

        high += CG_EvalRumbleGraph(&info->high, t) * ar->scale;
        low += CG_EvalRumbleGraph(&info->low, t) * ar->scale;
    }
    if (now - g_rumbleFloorEnd < 0) {
        low = fmaxf(low, g_rumbleFloorLow);
        high = fmaxf(high, g_rumbleFloorHigh);
    }

    if (low > 1.0f)
        low = 1.0f;
    if (high > 1.0f)
        high = 1.0f;
    if (low < 0.0f)
        low = 0.0f;
    if (high < 0.0f)
        high = 0.0f;

    CL_Gamepad_SetRumble(low, high);

    if (cg_drawRumbleDebug && cg_drawRumbleDebug->current.enabled &&
        (low > 0.0f || high > 0.0f)) {
        Com_Printf("Rumble: Low %.3f, High %.3f\n", low, high);
    }
}

void CG_UpdateRumble(void)
{
    CG_RemoveInactiveRumbles();
    CG_CalcActiveRumbles();
}

/* Plays a graph stretched over the duration at a fraction of its strength;
 * returns whether it started. */
static int CG_PlayRumbleInternal(const char *name, int loop, int duration, float scale)
{
    int infoIndex, slot;

    if (!name || !name[0] || duration <= 0 || !(scale > 0.0f))
        return 0;

    infoIndex = CG_LoadRumble(name);
    if (infoIndex < 0)
        return 0;

    slot = CG_FindClosestToDyingActiveRumble();
    if (slot < 0)
        return 0;

    g_activeRumbles[slot].active = 1;
    g_activeRumbles[slot].infoIndex = infoIndex;
    g_activeRumbles[slot].startTime = Sys_Milliseconds();
    g_activeRumbles[slot].duration = duration;
    g_activeRumbles[slot].scale = scale < 1.0f ? scale : 1.0f;
    g_activeRumbles[slot].loop = loop;
    return 1;
}

void CG_PlayRumble(const char *name)
{
    CG_PlayRumbleInternal(name, 0, RUMBLE_DURATION_MS, 1.0f);
}
void CG_PlayLoopRumble(const char *name)
{
    CG_PlayRumbleInternal(name, 1, RUMBLE_DURATION_MS, 1.0f);
}

/* The local player's own view: not a killcam, nor a player they follow. */
static int CG_RumbleOwnView(void)
{
    return cg->nextSnap && cg->nextSnap->ps.clientNum == cg->clientNum;
}

static const WeaponDef *CG_RumbleWeaponDef(int weapon)
{
    if (weapon <= 0 || weapon > BG_GetNumWeapons())
        return NULL;
    return BG_GetWeaponDef(weapon);
}

/* The retail turrets and Panzerschreck name no fire rumble although their
 * graphs ship; any other gun without one feels the default weapon's. */
static const char *CG_FireRumbleName(const WeaponDef *weapDef)
{
    static const struct {
        const char *prefix;
        const char *rumble;
    } unnamed[] = {
        { "mg42", "mg42_fire" },
        { "30cal", "30cal_fire" },
        { "panzerschreck", "panzerschreck_fire" },
    };
    int i;

    if (weapDef->fireRumble && weapDef->fireRumble[0])
        return weapDef->fireRumble;
    for (i = 0; i < (int)(sizeof(unnamed) / sizeof(unnamed[0])); ++i) {
        if (weapDef->szInternalName &&
            !strncmp(weapDef->szInternalName, unnamed[i].prefix, strlen(unnamed[i].prefix)))
            return unnamed[i].rumble;
    }
    return "defaultweapon_fire";
}

static int CG_FireRumbleDuration(const WeaponDef *weapDef)
{
    if (weapDef->weapClass == WEAPCLASS_ROCKETLAUNCHER)
        return RUMBLE_ROCKET_FIRE_MS;
    if (weapDef->bBoltAction)
        return RUMBLE_BOLT_FIRE_MS;
    if (weapDef->bSemiAuto)
        return RUMBLE_SEMI_FIRE_MS;
    if (weapDef->iFireTime < RUMBLE_SHOT_MIN_MS)
        return RUMBLE_SHOT_MIN_MS;
    return weapDef->iFireTime < RUMBLE_SEMI_FIRE_MS ? weapDef->iFireTime : RUMBLE_SEMI_FIRE_MS;
}

/* Every round the local player fires, from their own gun or the turret they
 * man; grenades and placed explosives kick only when they go off. */
void CG_RumbleFireEvent(const entityState_t *es)
{
    const snapshot_t *snap = cg->nextSnap;
    const WeaponDef *weapDef;

    if (!CG_RumbleOwnView())
        return;
    if (es->eType == 9 ? !(snap->ps.eFlags & 0x300) || snap->ps.viewlocked_entNum != es->number
                       : es->number != cg->clientNum)
        return;
    weapDef = CG_RumbleWeaponDef(es->weapon);
    if (!weapDef || (weapDef->weapType != WEAPTYPE_BULLET && weapDef->weapClass != WEAPCLASS_ROCKETLAUNCHER))
        return;
    CG_PlayRumbleInternal(CG_FireRumbleName(weapDef), 0, CG_FireRumbleDuration(weapDef), 1.0f);
    if (!weapDef->bSemiAuto && !weapDef->bBoltAction && weapDef->weapClass != WEAPCLASS_ROCKETLAUNCHER) {
        g_rumbleFloorLow = RUMBLE_BURST_LOW;
        g_rumbleFloorHigh = RUMBLE_BURST_HIGH;
        g_rumbleFloorEnd = Sys_Milliseconds() + weapDef->iFireTime + RUMBLE_BURST_SLACK_MS;
    } else {
        g_rumbleFloorLow = RUMBLE_KICK_LOW;
        g_rumbleFloorHigh = RUMBLE_KICK_HIGH;
        g_rumbleFloorEnd = Sys_Milliseconds() + RUMBLE_KICK_MS;
    }
}

/* A grenade, S-Mine, C4 or rocket going off near the view; smoke, which
 * deals no damage, has no radius and stays still. */
void CG_RumbleExplosion(const vec_t *origin, int weapon)
{
    const WeaponDef *weapDef = CG_RumbleWeaponDef(weapon);

    if (!weapDef || weapDef->iExplosionRadius <= 0 || !CG_RumbleOwnView())
        return;
    CG_PlayRumbleInternal(RUMBLE_EXPLOSION_GRAPH, 0, RUMBLE_DURATION_MS,
                          1.0f - Vec3Distance(origin, cg->refdef.vieworg) /
                                     ((float)weapDef->iExplosionRadius * RUMBLE_EXPLOSION_REACH));
}

void CG_RumbleEarthquake(const vec_t *origin, float intensity, int duration, float radius)
{
    if (!(radius > 0.0f) || !CG_RumbleOwnView())
        return;
    CG_PlayRumbleInternal(RUMBLE_EXPLOSION_GRAPH, 0, duration,
                          intensity * RUMBLE_EARTHQUAKE_GAIN *
                              (1.0f - Vec3Distance(origin, cg->refdef.vieworg) / radius));
}

void CG_RumbleDamage(int damage)
{
    if (damage <= 0 || !CG_RumbleOwnView())
        return;
    CG_PlayRumbleInternal(RUMBLE_DAMAGE_GRAPH, 0, RUMBLE_DURATION_MS,
                          RUMBLE_DAMAGE_BASE + (float)damage * RUMBLE_DAMAGE_PER_POINT);
}

/* The killing hit sends no damage feedback (P_DamageFeedback skips the dead):
 * dying, from playing (pm_type below 4) to dead (above 5), kicks fully. */
void CG_RumbleTransition(const playerState_t *ps, const playerState_t *ops)
{
    if (ps->clientNum == ops->clientNum && ops->pm_type < 4 && ps->pm_type > 5 && CG_RumbleOwnView())
        CG_PlayRumbleInternal(RUMBLE_DAMAGE_GRAPH, 0, RUMBLE_DURATION_MS, 1.0f);
}

void CG_RumbleMeleeSwing(const entityState_t *es)
{
    if (es->number != cg->clientNum || !CG_RumbleOwnView())
        return;
    g_rumbleMeleeTime = Sys_Milliseconds();
    g_rumbleMeleeWeapon = es->weapon;
}

void CG_RumbleMeleeImpact(const vec_t *origin)
{
    const WeaponDef *weapDef = CG_RumbleWeaponDef(g_rumbleMeleeWeapon);

    if (!weapDef || Sys_Milliseconds() - g_rumbleMeleeTime > RUMBLE_MELEE_WINDOW_MS || !CG_RumbleOwnView() ||
        Vec3Distance(origin, cg->refdef.vieworg) > RUMBLE_MELEE_REACH)
        return;
    g_rumbleMeleeWeapon = 0;
    CG_PlayRumbleInternal(weapDef->meleeImpactRumble && weapDef->meleeImpactRumble[0]
                              ? weapDef->meleeImpactRumble
                              : RUMBLE_MELEE_GRAPH,
                          0, RUMBLE_DURATION_MS, 1.0f);
}

void CG_StopRumble(const char *name)
{
    int idx = CG_GetRumbleInfoIndexFromName(name);
    int i;
    if (idx < 0)
        return;
    for (i = 0; i < MAX_ACTIVE_RUMBLES; ++i) {
        if (g_activeRumbles[i].active && g_activeRumbles[i].infoIndex == idx)
            CG_InvalidateActiveRumble(i);
    }
}

static void CG_PlayRumble_f(void)
{
    char arg[256];
    if (Cmd_Argc() != 2) {
        Com_Printf("USAGE: playrumble <rumblename>\n");
        return;
    }
    Cmd_ArgvBuffer(1, arg, sizeof(arg));
    if (!CG_PlayRumbleInternal(arg, 0, RUMBLE_DURATION_MS, 1.0f))
        Com_Printf("^1Could not play rumble asset '%s'\n", arg);
}

void CG_InitRumble(void)
{
    static int commandAdded;
    int i;
    g_rumbleInfoCount = 0;
    g_rumbleMeleeWeapon = 0;
    g_rumbleFloorLow = g_rumbleFloorHigh = 0.0f;
    for (i = 0; i < MAX_ACTIVE_RUMBLES; ++i)
        CG_InvalidateActiveRumble(i);

    cg_drawRumbleDebug = Dvar_RegisterBool_mac("cg_drawrumbledebug", 0, 0x0);
    /* Every map starts the cgame again; the command outlives it. */
    if (!commandAdded) {
        Cmd_AddCommand("playrumble", CG_PlayRumble_f);
        commandAdded = 1;
    }
}

void CG_ShutdownRumble(void)
{
    CG_StopAllRumbles();
}

#else

typedef int cg_rumble_mp_translation_unit_not_empty;

#endif
