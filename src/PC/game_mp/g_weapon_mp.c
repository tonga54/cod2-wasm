#include "common_types.h"
#include "imports.h"
#include "bytematch.h"

extern int rand(void);
extern double tan(double x);
extern float sinf(float x);
extern float cosf(float x);
extern float sqrtf(float x);

extern gentity_t *G_TempEntity(const vec_t *origin, int event);
extern void G_Damage(gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
                     const vec_t *dir, const vec_t *point, int damage, int dflags,
                     int mod, hitLocation_t hitLoc, int timeOffset);
extern const byte DirToByte(const vec_t *dir);
extern void G_GetPlayerViewDirection(const gentity_t *ent, vec_t *forward, vec_t *right, vec_t *up);

static vec2_t traceOffsets[5] = {
    {  0.0f,  0.0f },
    {  1.0f,  1.0f },
    {  1.0f, -1.0f },
    { -1.0f,  1.0f },
    { -1.0f, -1.0f },
};

extern float floorf(float x);
extern float ceilf(float x);

extern char *va(const char *format, ...);
extern void Com_DPrintf(const char *fmt, ...);
extern void Com_Error(int code, const char *fmt, ...);
extern void SV_GameSendServerCommand(int clientNum, int svscmd_type, const char *text);
extern int BG_FindWeaponIndexForName(const char *name);
extern int BG_GetWeaponIndexForName(const char *name, void *weaponInfoMem);
extern void *BG_GetWeaponDef(int weaponIndex);
extern void BG_ClearWeaponDef(void);
extern void BG_FillInAmmoItems(BG_RegisterWeapon regWeap);
extern int BG_GetFirstAvailableOffhand(const playerState_t *ps, int offhandClass);
extern void BG_GetSpreadForWeapon(const playerState_t *ps, int weaponIndex, float *minSpread, float *maxSpread);
extern void ClearRegisteredItems(void);
extern int Dvar_GetInt(const char *dvarName);
extern void SV_SetWeaponInfoMemory(void);
extern int BG_WeaponAmmo(const playerState_t *ps, int weapon);
extern Bool SV_GetClientPositionAtTime(int clientNum, int gametime, vec_t *pos);
extern void SV_LinkEntity(gentity_t *ent);
extern void SV_UnlinkEntity(gentity_t *ent);
extern void AngleVectors(vec_t *angles, vec_t *forward, vec_t *right, vec_t *up);
extern void G_GetPlayerViewOrigin(gentity_t *ent, vec_t *origin);
extern float randomf(void);
extern void *fire_rocket(gentity_t *ent, vec_t *start, vec_t *dir);
extern void *fire_grenade(gentity_t *ent, vec_t *start, vec_t *vel, int grenType, int fuseTime);
extern float Vec3Normalize(vec3_t v);
extern void G_LocationalTrace(trace_t *results, const vec_t *start, const vec_t *end, int passEntityNum, int contentmask, unsigned char *priorityMap);
extern void G_CheckHitTriggerDamage(gentity_t *attacker, vec_t *start, vec_t *end, int damage, int mod);
extern qboolean OnSameTeam(gentity_t *ent1, gentity_t *ent2);

extern struct level_locals_t level;
extern void *bg_weaponInfoMem;

void SnapVectorTowards(vec_t *v, vec_t *to);
qboolean LogAccuracyHit(gentity_t *target, gentity_t *attacker);
int G_GetWeaponIndexForName(const char *name);
void G_SetEquippedOffHand(int clientNum, int offHandIndex);
void G_SelectWeaponIndex(int clientNum, int iWeaponIndex);
void Weapon_RocketLauncher_Fire(gentity_t *ent, float spread, weaponParms *wp);
gentity_t *weapon_grenadelauncher_fire(gentity_t *ent, int grenType, weaponParms *wp);
void G_UseOffHand(gentity_t *ent);
Bool Melee_Trace(gentity_t *ent, weaponParms *wp, int damage, float range, float width, float height, trace_t *trace, vec_t *endPos);
void Weapon_Melee(gentity_s (*ent)[16], weaponParms *wp, float range, float width, float height);
void FireWeaponMelee(gentity_s (*ent)[16]);
static void Bullet_Fire_Extended(const gentity_t *source, gentity_s (*attacker)[16], vec_t *start, vec_t *end, float damageMultiplier, int recursion, const weaponParms *wp, const gentity_t *weaponEnt, const int gametime);
void G_SetupWeaponDef(void);
void Bullet_Endpos(float spread, vec_t *end, const weaponParms *wp, float maxRange);
void Bullet_Fire(gentity_s (*attacker)[16], float spread, weaponParms *wp, gentity_s (*weaponEnt)[16], int gametime);
void FireWeaponAntiLag(gentity_s (*ent)[16], int gametime);
qboolean G_GivePlayerWeapon(playerState_t *pPS, int iWeaponIndex);

COD2_ASSERT_FIELD(gclient_t, fGunPitch,    0x27d8);
COD2_ASSERT_FIELD(gclient_t, fGunYaw,      0x27dc);
COD2_ASSERT_FIELD(WeaponDef, iMeleeDamage, 0x01f4);
COD2_ASSERT_FIELD(gentity_t, client,            0x0158);
COD2_ASSERT_FIELD(WeaponDef, iFuseTime,         0x0248);
COD2_ASSERT_FIELD(WeaponDef, iProjectileSpeed,  0x0384);
COD2_ASSERT_FIELD(WeaponDef, iProjectileSpeedUp,0x0388);
COD2_ASSERT_FIELD(playerState_t, velocity,      0x0020);
COD2_ASSERT_FIELD(playerState_t, viewangles,    0x00e8);
COD2_ASSERT_FIELD(playerState_t, offHandIndex,  0x00d0);
COD2_ASSERT_FIELD(trace_t, surfaceFlags,        0x0010);

void SnapVectorTowards(vec_t *v, vec_t *to)
{
    int i;

    for (i = 0; i < 3; i++) {
        if (v[i] >= to[i])
            v[i] = floorf(v[i]);
        else
            v[i] = ceilf(v[i]);
    }
}

qboolean LogAccuracyHit(gentity_t *target, gentity_t *attacker)
{
    if (!((target)->takedamage))
        return 0;
    if (target == attacker)
        return 0;
    if (!((target)->client))
        return 0;
    if (!((attacker)->client))
        return 0;
    if (((((target)->client))->ps.pm_type) > 5)
        return 0;
    if (OnSameTeam(target, attacker))
        return 0;
    return 1;
}

int G_GetWeaponIndexForName(const char *name)
{
    if (!((struct level_locals_t *)imp_level)->initializing)
        return BG_FindWeaponIndexForName(name);
    return BG_GetWeaponIndexForName(name, bg_weaponInfoMem);
}

void G_SetEquippedOffHand(int clientNum, int offHandIndex)
{
    SV_GameSendServerCommand(clientNum, 1, va("%c %i", 0x43, offHandIndex));
}

void G_SelectWeaponIndex(int clientNum, int iWeaponIndex)
{
    SV_GameSendServerCommand(clientNum, 1, va("%c %i", 0x61, iWeaponIndex));
}

void Weapon_Melee(gentity_s (*ent)[16], weaponParms *wp, float range, float width, float height)
{
    int weaponIndex;
    int damage;
    trace_t tr;
    vec3_t endpos;
    int traceEntNum;
    gentity_t *traceEnt;
    gentity_t *tent;

    weaponIndex = ((gentity_t *)ent)->s.weapon;
    damage = ((WeaponDef *)BG_GetWeaponDef(weaponIndex))->iMeleeDamage;

    if (!Melee_Trace((gentity_t *)ent, wp, damage, range, width, height, &tr, endpos))
        return;

    traceEntNum = tr.entityNum;
    traceEnt = &((gentity_t *)imp_g_entities)[traceEntNum];

    tent = (gentity_t *)G_TempEntity(endpos, traceEnt->client ? 0xad : 0xae);
    tent->s.otherEntityNum = traceEntNum;
    tent->s.eventParm = (unsigned char)DirToByte(tr.normal);
    tent->s.weapon = ((gentity_t *)ent)->s.weapon;

    if (traceEntNum == 0x3fe || !traceEnt->takedamage)
        return;

    G_Damage(traceEnt, (gentity_t *)ent, (gentity_t *)ent, wp->forward, endpos,
             damage + (rand() % 5), 7, 0, tr.partGroup, 0);
}

void FireWeaponMelee(gentity_s (*ent)[16])
{
    gentity_t *entity = (gentity_t *)ent;
    weaponParms wp;
    const dvar_t *meleeRange;
    const dvar_t *meleeWidth;
    const dvar_t *meleeHeight;

    if ((entity->client->ps.eFlags & 0x300) && entity->active)
        return;

    (void)BG_GetWeaponDef(entity->s.weapon);
    G_GetPlayerViewOrigin(entity, wp.muzzleTrace);
    G_GetPlayerViewDirection(entity, wp.forward, wp.right, wp.up);

    meleeRange = *(const dvar_t **)imp_player_meleeRange;
    meleeWidth = *(const dvar_t **)imp_player_meleeWidth;
    meleeHeight = *(const dvar_t **)imp_player_meleeHeight;
    Weapon_Melee(ent, &wp, meleeRange->current.value,
                 meleeWidth->current.value, meleeHeight->current.value);
}

void Weapon_RocketLauncher_Fire(gentity_t *ent, float spread, weaponParms *wp)
{
    vec3_t dir;
    vec3_t launchpos;
    float fAimOffset;
    float angle;
    float r;
    float rightOffset;
    float upOffset;
    gclient_t *client;

    fAimOffset = (float)tan((double)spread * 0.017453292519943295) * 16.0f;

    angle = (float)((double)(randomf() * 360.0f) * 0.017453292519943295);
    r = randomf();
    rightOffset = cosf(angle) * r * fAimOffset;
    upOffset = sinf(angle) * r * fAimOffset;

    dir[0] = wp->forward[0] * 16.0f;
    dir[1] = wp->forward[1] * 16.0f;
    dir[2] = 16.0f * wp->forward[2];

    dir[0] += rightOffset * wp->right[0];
    dir[1] += rightOffset * wp->right[1];
    dir[2] += rightOffset * wp->right[2];

    dir[0] += upOffset * wp->up[0];
    dir[1] += upOffset * wp->up[1];
    dir[2] += upOffset * wp->up[2];

    Vec3Normalize(dir);

    launchpos[0] = wp->muzzleTrace[0];
    launchpos[1] = wp->muzzleTrace[1];
    launchpos[2] = wp->muzzleTrace[2];

    fire_rocket(ent, launchpos, dir);

    client = ent->client;
    if (!client)
        return;

    client->ps.velocity[0] += wp->forward[0] * -64.0f;
    client->ps.velocity[1] += wp->forward[1] * -64.0f;
    client->ps.velocity[2] += -64.0f * wp->forward[2];
}

gentity_t *weapon_grenadelauncher_fire(gentity_t *ent, int grenType, weaponParms *wp)
{
    WeaponDef *weapDef;
    vec3_t vTossVel;
    gentity_t *grenade;
    gclient_t *client;
    float dot;

    weapDef = wp->weapDef;

    vTossVel[0] = (float)weapDef->iProjectileSpeed * wp->forward[0];
    vTossVel[1] = (float)weapDef->iProjectileSpeed * wp->forward[1];
    vTossVel[2] = (float)weapDef->iProjectileSpeed * wp->forward[2];
    vTossVel[2] += (float)weapDef->iProjectileSpeedUp;

    grenade = (gentity_t *)fire_grenade(ent, wp->muzzleTrace, vTossVel,
                                        grenType, weapDef->iFuseTime);

    Vec3Normalize(vTossVel);

    client = ent->client;
    dot = vTossVel[0] * client->ps.velocity[0]
        + client->ps.velocity[1] * vTossVel[1]
        + client->ps.velocity[2] * vTossVel[2];

    grenade->s.pos.trDelta[0] += vTossVel[0] * dot;
    grenade->s.pos.trDelta[1] += dot * vTossVel[1];
    grenade->s.pos.trDelta[2] += dot * vTossVel[2];

    return grenade;
}

void G_UseOffHand(gentity_t *ent)
{
    weaponParms wp;
    vec3_t viewang;

    wp.weapDef = BG_GetWeaponDef(ent->client->ps.offHandIndex);

    viewang[0] = ent->client->ps.viewangles[0];
    viewang[1] = ent->client->ps.viewangles[1];
    viewang[2] = ent->client->ps.viewangles[2];
    viewang[0] = ent->client->fGunPitch;
    viewang[1] = ent->client->fGunYaw;

    AngleVectors(viewang, wp.forward, wp.right, wp.up);
    G_GetPlayerViewOrigin(ent, wp.muzzleTrace);

    weapon_grenadelauncher_fire(ent, ent->client->ps.offHandIndex, &wp);
}

Bool Melee_Trace(gentity_t *ent, weaponParms *wp, int damage, float range,
                 float width, float height, trace_t *trace, vec_t *endPos)
{
    vec3_t end;
    int numTraces;
    int i;

    if (width > 0.0f) {
        numTraces = 5;
    } else if (0.0f >= height) {
        numTraces = 1;
    } else {
        numTraces = 5;
    }

    for (i = 0; i < numTraces; i++) {
        end[0] = wp->muzzleTrace[0] + range * wp->forward[0];
        end[1] = wp->muzzleTrace[1] + range * wp->forward[1];
        end[2] = wp->muzzleTrace[2] + range * wp->forward[2];

        end[0] += (width * traceOffsets[i][0]) * wp->right[0];
        end[1] += (width * traceOffsets[i][0]) * wp->right[1];
        end[2] += (width * traceOffsets[i][0]) * wp->right[2];

        end[0] += (height * traceOffsets[i][1]) * wp->up[0];
        end[1] += (height * traceOffsets[i][1]) * wp->up[1];
        end[2] += (height * traceOffsets[i][1]) * wp->up[2];

        G_LocationalTrace(trace, wp->muzzleTrace, end, ent->s.number,
                          0x2802831, (unsigned char *)imp_bulletPriorityMap);

        endPos[0] = wp->muzzleTrace[0] + (end[0] - wp->muzzleTrace[0]) * trace->fraction;
        endPos[1] = wp->muzzleTrace[1] + (end[1] - wp->muzzleTrace[1]) * trace->fraction;
        endPos[2] = wp->muzzleTrace[2] + (end[2] - wp->muzzleTrace[2]) * trace->fraction;

        if (i == 0)
            G_CheckHitTriggerDamage(ent, wp->muzzleTrace, endPos, damage, 7);

        if ((trace->surfaceFlags & 0x10) || trace->fraction != 1.0f)
            return 1;
    }

    return 0;
}

static void Bullet_Fire_Extended(const gentity_t *source, gentity_s (*attacker)[16], vec_t *start, vec_t *end, float damageMultiplier, int recursion, const weaponParms *wp, const gentity_t *weaponEnt, const int gametime)
{
    WeaponDef *weapDef;
    gentity_t *attackerEnt;
    gentity_t *traceEnt;
    trace_t tr;
    vec3_t endpos;
    vec3_t reflect;
    unsigned char *priorityMap;
    int dflags;
    int iMOD;
    int damage;
    float distance;

    if (recursion > 12) {
        Com_DPrintf("Bullet_Fire_Extended: Too many resursions, bullet aborted\n");
        return;
    }

    weapDef = wp->weapDef;
    attackerEnt = (gentity_t *)attacker;

    if (weapDef->bRifleBullet) {
        dflags = 0x20;
        iMOD = 2;
        priorityMap = (unsigned char *)imp_riflePriorityMap;
    } else {
        dflags = 0;
        iMOD = 1;
        priorityMap = (unsigned char *)imp_bulletPriorityMap;
    }

    if (weapDef->armorPiercing)
        dflags |= 2;

    G_LocationalTrace(&tr, start, end, source->s.number, 0x2802831, priorityMap);

    endpos[0] = start[0] + (end[0] - start[0]) * tr.fraction;
    endpos[1] = start[1] + (end[1] - start[1]) * tr.fraction;
    endpos[2] = start[2] + (end[2] - start[2]) * tr.fraction;

    G_CheckHitTriggerDamage(attackerEnt, start, endpos, weapDef->damage, iMOD);

    traceEnt = &((gentity_t *)imp_g_entities)[tr.entityNum];

    reflect[0] = end[0] - start[0];
    reflect[1] = end[1] - start[1];
    reflect[2] = end[2] - start[2];
    Vec3Normalize(reflect);
    {
        float dot;

        dot = reflect[0] * tr.normal[0] + reflect[1] * tr.normal[1] + reflect[2] * tr.normal[2];
        reflect[0] += (-2.0f * dot) * tr.normal[0];
        reflect[1] += (-2.0f * dot) * tr.normal[1];
        reflect[2] += (-2.0f * dot) * tr.normal[2];
    }

    if (!(tr.surfaceFlags & 4) && !traceEnt->client && tr.fraction < 1.0f) {
        gentity_t *tent;

        tent = (gentity_t *)G_TempEntity(endpos, weapDef->bRifleBullet ? 0xb6 : 0xb5);
        tent->s.eventParm = (unsigned char)DirToByte(tr.normal);
        tent->s.eventParm2 = (unsigned char)DirToByte(reflect);
        tent->s.surfType = traceEnt->s.eType == 2 ? 7 : ((tr.surfaceFlags & 0x1f00000) >> 20);
        tent->s.otherEntityNum = weaponEnt->s.number;
    }

    if (tr.contents & 0x10) {
        vec3_t dir;
        float dot;
        float offset;

        dir[0] = end[0] - start[0];
        dir[1] = end[1] - start[1];
        dir[2] = end[2] - start[2];
        Vec3Normalize(dir);

        dot = -(dir[0] * tr.normal[0] + dir[1] * tr.normal[1] + dir[2] * tr.normal[2]);
        offset = 0.0f;
        if (dot >= 0.125f)
            offset = 0.25f / dot;

        start[0] = endpos[0] + dir[0] * offset;
        start[1] = endpos[1] + dir[1] * offset;
        start[2] = endpos[2] + dir[2] * offset;

        Bullet_Fire_Extended(source, attacker, start, end, damageMultiplier,
                             recursion + 1, wp, weaponEnt, gametime);
        return;
    }

    if (!traceEnt->takedamage || traceEnt == attackerEnt)
        return;

    distance = sqrtf((start[0] - endpos[0]) * (start[0] - endpos[0]) +
                     (start[1] - endpos[1]) * (start[1] - endpos[1]) +
                     (start[2] - endpos[2]) * (start[2] - endpos[2]));

    if (weapDef->fMaxDamageRange > distance) {
        damage = weapDef->damage;
    } else if (weapDef->fMinDamageRange > distance) {
        float range;

        range = weapDef->fMinDamageRange - weapDef->fMaxDamageRange;
        if (range == 0.0f) {
            damage = weapDef->damage;
        } else {
            float frac;
            float scaledDamage;

            frac = (distance - weapDef->fMaxDamageRange) / range;
            scaledDamage = (float)weapDef->damage * (1.0f - frac) + (float)weapDef->minDamage * frac;
            damage = (int)scaledDamage;
        }
    } else {
        damage = weapDef->minDamage;
    }

    G_Damage(traceEnt, attackerEnt, attackerEnt, (void *)wp, endpos,
             (int)((float)damage * damageMultiplier), dflags, iMOD,
             tr.partGroup, level.time - gametime);

    if (traceEnt->client && (dflags & 0x20)) {
        if (Dvar_GetInt("scr_friendlyfire") || !OnSameTeam(traceEnt, attackerEnt)) {
            Bullet_Fire_Extended(traceEnt, attacker, endpos, end,
                                 damageMultiplier * 0.5f, recursion + 1,
                                 wp, weaponEnt, gametime);
        }
    }
}

void G_SetupWeaponDef(void)
{
    BG_RegisterWeapon registerWeapon;

    Com_DPrintf("----------------------\n");
    Com_DPrintf("Game: G_SetupWeaponDef\n");

    if (*(int *)imp_bg_iNumWeapons) {
        Com_DPrintf("----------------------\n");
        return;
    }

    SV_SetWeaponInfoMemory();
    ClearRegisteredItems();
    BG_ClearWeaponDef();

    registerWeapon = (BG_RegisterWeapon)imp_G_RegisterWeapon;
    BG_FillInAmmoItems(registerWeapon);

    if ((*(level_locals_t **)imp_level)->initializing)
        BG_GetWeaponIndexForName("defaultweapon_mp", registerWeapon);
    else
        BG_FindWeaponIndexForName("defaultweapon_mp");

    Com_DPrintf("----------------------\n");
}

void Bullet_Endpos(float spread, vec_t *end, const weaponParms *wp, float maxRange)
{
    float aimOffset;
    float randAngle;
    float randRadius;
    float angle;
    float rightOffset;
    float upOffset;

    aimOffset = (float)tan((double)spread * 0.017453292519943295) * maxRange;

    randAngle = randomf();
    randRadius = randomf();
    angle = (float)((double)(randAngle * 360.0f) * 0.017453292519943295);

    rightOffset = cosf(angle) * randRadius * aimOffset;
    upOffset = sinf(angle) * randRadius * aimOffset;

    {
        const vec_t *muzzle = wp->muzzleTrace;
        end[0] = muzzle[0] + maxRange * wp->forward[0];
        end[1] = muzzle[1] + maxRange * wp->forward[1];
        end[2] = muzzle[2] + maxRange * wp->forward[2];
    }

    {
        const vec_t *right = wp->right;
        end[0] += rightOffset * right[0];
        end[1] += rightOffset * right[1];
        end[2] += rightOffset * right[2];
    }

    {
        const vec_t *up = wp->up;
        end[0] += upOffset * up[0];
        end[1] += upOffset * up[1];
        end[2] += upOffset * up[2];
    }
}

void Bullet_Fire(gentity_s (*attacker)[16], float spread, weaponParms *wp, gentity_s (*weaponEnt)[16], int gametime)
{
    vec3_t end;
    vec3_t start;
    vec3_t clientPosition;
    vec3_t savedClientPositions[64];
    byte movedClients[64];
    const dvar_t *antilag;
    int clientNum;

    antilag = *(const dvar_t **)imp_g_antilag;
    memset(savedClientPositions, 0, sizeof(savedClientPositions));
    memset(movedClients, 0, sizeof(movedClients));

    if (antilag && antilag->current.enabled) {
        const dvar_t *svFps;
        int frameMsec;

        svFps = *(const dvar_t **)imp_sv_fps;
        frameMsec = 1000 / svFps->current.integer;

        if (level.time - gametime > frameMsec) {
            for (clientNum = 0; clientNum < level.maxclients; clientNum++) {
                gclient_t *client;
                gentity_t *clientEnt;

                client = &level.clients[clientNum];
                if (client->sess.connected != CON_CONNECTED)
                    continue;
                if (client->sess.sessionState != SESS_STATE_PLAYING)
                    continue;
                if (!SV_GetClientPositionAtTime(clientNum, gametime, clientPosition))
                    continue;

                clientEnt = &((gentity_t *)imp_g_entities)[clientNum];
                savedClientPositions[clientNum][0] = clientEnt->r.currentOrigin[0];
                savedClientPositions[clientNum][1] = clientEnt->r.currentOrigin[1];
                savedClientPositions[clientNum][2] = clientEnt->r.currentOrigin[2];

                SV_UnlinkEntity(clientEnt);
                clientEnt->r.currentOrigin[0] = clientPosition[0];
                clientEnt->r.currentOrigin[1] = clientPosition[1];
                clientEnt->r.currentOrigin[2] = clientPosition[2];
                SV_LinkEntity(clientEnt);

                movedClients[clientNum] = 1;
            }
        }
    }

    if (wp->weapDef->weapClass == WEAPCLASS_SPREAD) {
        int shotIndex;

        start[0] = wp->muzzleTrace[0];
        start[1] = wp->muzzleTrace[1];
        start[2] = wp->muzzleTrace[2];

        for (shotIndex = 0; shotIndex < wp->weapDef->shotCount; shotIndex++) {
            Bullet_Endpos(spread, end, wp, wp->weapDef->fMinDamageRange);
            Bullet_Fire_Extended((gentity_t *)weaponEnt, attacker, start, end,
                                 1.0f, 0, wp, (gentity_t *)weaponEnt,
                                 gametime);
        }
    } else {
        Bullet_Endpos(spread, end, wp, 8192.0f);
        Bullet_Fire_Extended((gentity_t *)weaponEnt, attacker, wp->muzzleTrace,
                             end, 1.0f, 0, wp, (gentity_t *)weaponEnt,
                             gametime);
    }

    if (antilag && antilag->current.enabled) {
        for (clientNum = 0; clientNum < level.maxclients; clientNum++) {
            gentity_t *clientEnt;

            if (!movedClients[clientNum])
                continue;

            clientEnt = &((gentity_t *)imp_g_entities)[clientNum];
            SV_UnlinkEntity(clientEnt);
            clientEnt->r.currentOrigin[0] = savedClientPositions[clientNum][0];
            clientEnt->r.currentOrigin[1] = savedClientPositions[clientNum][1];
            clientEnt->r.currentOrigin[2] = savedClientPositions[clientNum][2];
            SV_LinkEntity(clientEnt);
        }
    }
}

void FireWeaponAntiLag(gentity_s (*ent)[16], int gametime)
{
    gentity_t *entity;
    WeaponDef *weapDef;
    weaponParms wp;
    vec3_t viewang;
    float aimSpreadScale;
    float minSpread;
    float maxSpread;
    float spread;

    entity = (gentity_t *)ent;

    if ((entity->client->ps.eFlags & 0x300) && entity->active)
        return;

    weapDef = BG_GetWeaponDef(entity->s.weapon);
    wp.weapDef = weapDef;

    {
        vec_t *va = entity->client->ps.viewangles;
        viewang[0] = va[0];
        viewang[1] = va[1];
        viewang[2] = va[2];
    }
    viewang[0] = entity->client->fGunPitch;
    viewang[1] = entity->client->fGunYaw;

    AngleVectors(viewang, wp.forward, wp.right, wp.up);
    G_GetPlayerViewOrigin(entity, wp.muzzleTrace);

    aimSpreadScale = entity->client->currentAimSpreadScale;
    BG_GetSpreadForWeapon(&entity->client->ps, entity->s.weapon, &minSpread, &maxSpread);

    if (entity->client->ps.fWeaponPosFrac == 1.0f)
        spread = weapDef->fAdsSpread + (maxSpread - weapDef->fAdsSpread) * aimSpreadScale;
    else
        spread = minSpread + (maxSpread - minSpread) * aimSpreadScale;

    switch (weapDef->weapType) {
    case WEAPTYPE_BULLET:
        Bullet_Fire(ent, spread, &wp, ent, gametime);
        return;
    case WEAPTYPE_GRENADE:
        weapon_grenadelauncher_fire((gentity_t *)ent, entity->s.weapon, &wp);
        return;
    case WEAPTYPE_PROJECTILE:
        Weapon_RocketLauncher_Fire((gentity_t *)ent, spread, &wp);
        return;
    default:
        Com_Error(1, "\x15Unknown weapon type %i for %s\n",
                  weapDef->weapType, weapDef->szInternalName);
        return;
    }
}

qboolean G_GivePlayerWeapon(playerState_t *pPS, int iWeaponIndex)
{
    WeaponDef *weapDef;
    int weaponWord;
    int weaponBitIndex;
    int weaponBit;
    int altWeaponIndex;

    weaponWord = iWeaponIndex >> 5;
    weaponBitIndex = iWeaponIndex & 0x1f;
    weaponBit = 1 << weaponBitIndex;

    if ((pPS->weapons[weaponWord] >> weaponBitIndex) & 1)
        return 0;

    weapDef = BG_GetWeaponDef(iWeaponIndex);
    if (weapDef->weapClass == WEAPCLASS_TURRET ||
        weapDef->weapClass == WEAPCLASS_NON_PLAYER)
        return 0;

    pPS->weapons[weaponWord] |= weaponBit;
    pPS->weaponrechamber[weaponWord] &= ~weaponBit;

    if (weapDef->weapClass == WEAPCLASS_ITEM)
        return 1;

    if (weapDef->offhandClass) {
        if (!pPS->offHandIndex) {
            pPS->offHandIndex = iWeaponIndex;
            G_SetEquippedOffHand(pPS->clientNum, iWeaponIndex);
        } else if (BG_WeaponAmmo(pPS, pPS->offHandIndex) <= 0) {
            WeaponDef *oldOffhandDef;
            int offhandIndex;

            oldOffhandDef = BG_GetWeaponDef(pPS->offHandIndex);
            offhandIndex = BG_GetFirstAvailableOffhand(
                pPS, oldOffhandDef->offhandClass);
            if (!offhandIndex)
                offhandIndex = iWeaponIndex;

            pPS->offHandIndex = offhandIndex;
            G_SetEquippedOffHand(pPS->clientNum, offhandIndex);
        }
    } else if ((unsigned int)(weapDef->weapSlot - WEAPSLOT_PRIMARY) <=
               (WEAPSLOT_PRIMARYB - WEAPSLOT_PRIMARY)) {
        if (!pPS->weaponslots[WEAPSLOT_PRIMARY])
            pPS->weaponslots[WEAPSLOT_PRIMARY] = (char)iWeaponIndex;
        else if (!pPS->weaponslots[WEAPSLOT_PRIMARYB])
            pPS->weaponslots[WEAPSLOT_PRIMARYB] = (char)iWeaponIndex;
    }

    altWeaponIndex = weapDef->iAltWeaponIndex;
    while (altWeaponIndex) {
        int altWeaponWord;
        int altWeaponBitIndex;
        int altWeaponBit;

        altWeaponWord = altWeaponIndex >> 5;
        altWeaponBitIndex = altWeaponIndex & 0x1f;
        altWeaponBit = 1 << altWeaponBitIndex;

        if ((pPS->weapons[altWeaponWord] >> altWeaponBitIndex) & 1)
            return 1;

        pPS->weapons[altWeaponWord] |= altWeaponBit;
        pPS->weaponrechamber[weaponWord] &= ~weaponBit;

        weapDef = BG_GetWeaponDef(altWeaponIndex);
        altWeaponIndex = weapDef->iAltWeaponIndex;
    }

    return 1;
}

const unsigned char __ZZ11Melee_TraceP9gentity_sP11weaponParmsifffP7trace_tPfE12traceOffsets[224] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x80, 0x3f,
    0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x80, 0xbf, 0x00, 0x00, 0x80, 0xbf, 0x00, 0x00, 0x80, 0x3f,
    0x00, 0x00, 0x80, 0xbf, 0x00, 0x00, 0x80, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x61, 0x8c, 0x1c, 0x00, 0xad, 0x8c, 0x1c, 0x00, 0x05, 0x8c, 0x1c, 0x00, 0x9c, 0x8c, 0x1c, 0x00,
    0x8b, 0x8c, 0x1c, 0x00, 0x7d, 0x8c, 0x1c, 0x00, 0x6f, 0x8c, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
