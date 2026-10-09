#include "common_types.h"
#include "imports.h"
#include <string.h>

unsigned char riflePriorityMap[19] = { 0x1, 0x9, 0x9, 0x9, 0x8, 0x7, 0x6, 0x6, 0x6, 0x6, 0x5, 0x5, 0x4, 0x4, 0x4, 0x4, 0x3, 0x3, 0x0 };

char *modNames[15] = { (char *)&str_002b64e0, (char *)&str_002b64ec, (char *)&str_002b6500, (char *)&str_002b6514, (char *)&str_002b6520, (char *)&str_002b6534, (char *)&str_002b6544, (char *)&str_002b655c, (char *)&str_002b6568, (char *)&str_002b6578, (char *)&str_002b6584, (char *)&str_002b6594, (char *)&str_002b65a0, (char *)&str_002b65ac, (char *)&str_002b65c0 };

extern float sqrtf(float x);

extern int I_stricmp(const char *s1, const char *s2);
extern short Scr_AllocString(const char *str, int flag);
extern int FS_FOpenFileByMode(const char *filename, int *f, int mode);
extern int FS_Read(void *buffer, int len, int f);
extern void FS_FCloseFile(fileHandle_t f);
extern int Info_Validate(const char *s);
extern int ParseConfigStringToStruct(float *dest, void *fields, int numFields, const char *buffer, int unused1, int unused2, void (*callback)(byte *, const char *));
extern void Com_Error(int level, const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);
extern void *BG_GetWeaponDef(int weapon);
extern void G_GetPlayerViewOrigin(gentity_t *ent, vec_t *origin);
extern float Vec3Normalize(vec_t *v);
extern float Vec3NormalizeTo(const vec_t *v, vec_t *out);
extern int G_LocationalTracePassed(const vec_t *start, const vec_t *end, int entityNum, int contentmask);
extern void Scr_AddEntity(gentity_t *ent);
extern void Scr_AddInt(int value);
extern void Scr_Notify(gentity_t *ent, int name, int numArgs);
extern int LogAccuracyHit(gentity_t *target, gentity_t *attacker);
extern int CM_AreaEntities(const vec_t *mins, const vec_t *maxs, int *entityList, int maxcount, int areatype);
extern void G_TraceCapsule(void *results, const vec_t *start, const vec_t *end, const vec_t *end2, const vec_t *mins, int passEntityNum, int contentmask);
extern void Scr_PlayerDamage(gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, int damage, int dflags, int mod, int weapon, const vec_t *dir, const vec_t *point, hitLocation_t hitLoc, int timeOffset);
extern int Com_GetServerDObj(int clientNum);
extern int BG_AnimScriptEvent(void *ps, int event, int isContinue, int force);
extern void Scr_PlayerKilled(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath, int iWeapon, const vec_t *vDir, hitLocation_t hitLoc, int psTimeOffset, int deathAnimDuration);
extern gentity_t *fire_grenade(gentity_t *self, vec_t *start, vec_t *dir, int grenadeWPID, int clientNum);
extern void Cmd_Score_f(gentity_t *ent);
extern void SV_UnlinkEntity(gentity_t *ent);
extern void SV_LinkEntity(gentity_t *ent);
extern float vectoyaw(const vec_t *vec);
extern float crandom(void);
extern float randomf(void);

extern unsigned char bulletPriorityMap[19];
extern unsigned char riflePriorityMap[19];
extern char *modNames[15];
extern float g_fHitLocDamageMult[19];

__attribute__((used, packed, aligned(4)))
const char *g_HitLocNames[] = {
    "none",
    "helmet",
    "head",
    "neck",
    "torso_upper",
    "torso_lower",
    "right_arm_upper",
    "left_arm_upper",
    "right_arm_lower",
    "left_arm_lower",
    "right_hand",
    "left_hand",
    "right_leg_upper",
    "left_leg_upper",
    "right_leg_lower",
    "left_leg_lower",
    "right_foot",
    "left_foot",
    "gun",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

static scr_string_t g_HitLocConstNames[19];
#define SCR_CONST() ((const scr_const_t *)imp_scr_const)

#define g_entities ((gentity_t *)imp_g_entities)

#define level (*(struct level_locals_t *)imp_level)
extern bgs_t level_bgs;
extern entityHandler_t entityHandlers[20];
extern const dvar_t *g_debugDamage;

extern int g_sNextDmgTableId;
extern void *imp_bgs;
extern int g_phys_world;

static void G_HitLocStrcpy(byte *pMember, const char *pszKeyValue)
{
    strcpy((char *)pMember, pszKeyValue);
}

void G_ParseHitLocDmgTable(void)
{
    struct {
        const char *name;
        int offset;
        int type;
    } hitLocDmgFields[19];
    char buffer[0x2000];
    int hFile;
    int iFileLength;
    int i;
    const int headerSize = sizeof("LOCDMGTABLE") - 1;

    for (i = 0; i < 19; i++) {
        g_fHitLocDamageMult[i] = 1.0f;
        hitLocDmgFields[i].name = g_HitLocNames[i];
        hitLocDmgFields[i].offset = i * sizeof(float);
        hitLocDmgFields[i].type = 6;
        g_HitLocConstNames[i] = Scr_AllocString(g_HitLocNames[i], 1);
    }

    g_fHitLocDamageMult[18] = 0.0f;

    iFileLength = FS_FOpenFileByMode("info/mp_lochit_dmgtable", &hFile, 0);
    if (iFileLength <= headerSize) {
        Com_Error(1, "Could not load hitloc damage table %s\n", "info/mp_lochit_dmgtable");
    }

    FS_Read(buffer, headerSize, hFile);

    if (memcmp(buffer, "LOCDMGTABLE", headerSize) != 0) {
        Com_Error(1, "\"%s\" does not appear to be a hitloc damage table\n", "info/mp_lochit_dmgtable");
    }

    iFileLength -= headerSize;
    if (iFileLength > 0x1fff) {
        Com_Error(1, "\"%s\" Is too long of a hitloc damage table to parse\n", "info/mp_lochit_dmgtable");
    }

    FS_Read(buffer, iFileLength, hFile);
    buffer[iFileLength] = '\0';
    FS_FCloseFile(hFile);

    if (!Info_Validate(buffer)) {
        Com_Error(1, "\"%s\" is not a valid hitloc damage table\n", "info/mp_lochit_dmgtable");
    }

    if (!ParseConfigStringToStruct(g_fHitLocDamageMult, hitLocDmgFields, 19, buffer, 0, 0, G_HitLocStrcpy)) {
        Com_Error(1, "Error parsing hitloc damage table %s\n", "info/mp_lochit_dmgtable");
    }
}

int G_IndexForMeansOfDeath(const char *pszMOD)
{
    int i;

    for (i = 0; i < 15; i++) {
        if (I_stricmp(pszMOD, modNames[i]) == 0) {
            return i;
        }
    }

    Com_Printf("Unknown means of death string '%s'\n", pszMOD);
    return 0;
}

scr_string_t G_GetHitLocationString(hitLocation_t hitLoc)
{
    return g_HitLocConstNames[hitLoc];
}

hitLocation_t G_GetHitLocationIndexFromString(int sString)
{
    int i;

    for (i = 0; i < 19; i++) {
        if (g_HitLocConstNames[i] == (scr_string_t)sString) {
            return (hitLocation_t)i;
        }
    }

    return (hitLocation_t)0;
}

float CanDamage(gentity_t *targ, const vec_t *origin)
{
    vec3_t dest;
    vec3_t endpoints[5];
    vec3_t forward;
    vec3_t eyeOrigin;
    int hits;
    int i;

    if (targ->client != NULL) {

        G_GetPlayerViewOrigin(targ, eyeOrigin);

        float halfHeight = (eyeOrigin[2] - targ->r.currentOrigin[2]) * 0.5f;

        forward[0] = origin[0] - targ->r.currentOrigin[0];
        forward[1] = origin[1] - targ->r.currentOrigin[1];
        forward[2] = 0.0f;
        Vec3Normalize(forward);

        float right_x = -forward[1];
        float right_y = forward[0];
        float right_z = forward[2];

        float mid_x = (eyeOrigin[0] + targ->r.currentOrigin[0]) * 0.5f;
        float mid_y = (eyeOrigin[1] + targ->r.currentOrigin[1]) * 0.5f;
        float mid_z = (eyeOrigin[2] + targ->r.currentOrigin[2]) * 0.5f;

        dest[0] = mid_x;
        dest[1] = mid_y;
        dest[2] = mid_z;

        endpoints[0][0] = mid_x + right_x * 15.0f;
        endpoints[0][1] = mid_y + right_y * 15.0f;
        float z_plus = mid_z + right_z * 15.0f;
        endpoints[0][2] = z_plus + halfHeight;

        endpoints[1][0] = endpoints[0][0];
        endpoints[1][1] = endpoints[0][1];
        endpoints[1][2] = z_plus - halfHeight;

        endpoints[2][0] = mid_x + right_x * -15.0f;
        endpoints[2][1] = mid_y + right_y * -15.0f;
        float z_minus = mid_z + right_z * -15.0f;
        endpoints[2][2] = z_minus + halfHeight;

        endpoints[3][0] = endpoints[2][0];
        endpoints[3][1] = endpoints[2][1];
        endpoints[3][2] = z_minus - halfHeight;

        hits = 0;
        for (i = 0; i < 5; i++) {
            vec_t *ep = (i == 0) ? dest : endpoints[i - 1];
            if (G_LocationalTracePassed(origin, ep, targ->s.number, 0x802891) >= 1) {
                hits++;
            }
        }

        if (hits == 0) {
            return 0.0f;
        }
        if (hits > 3) {
            return 1.0f;
        }
        return (float)hits / 3.0f;
    } else {

        float mid_x = (targ->r.absmin[0] + targ->r.absmax[0]) * 0.5f;
        float mid_y = (targ->r.absmin[1] + targ->r.absmax[1]) * 0.5f;
        float mid_z = (targ->r.absmin[2] + targ->r.absmax[2]) * 0.5f;

        dest[0] = mid_x;
        dest[1] = mid_y;
        dest[2] = mid_z;

        endpoints[0][0] = mid_x + 15.0f;
        endpoints[0][1] = mid_y + 15.0f;
        endpoints[0][2] = mid_z;

        endpoints[1][0] = endpoints[0][0];
        endpoints[1][1] = mid_y - 15.0f;
        endpoints[1][2] = mid_z;

        endpoints[2][0] = mid_x - 15.0f;
        endpoints[2][1] = endpoints[0][1];
        endpoints[2][2] = mid_z;

        endpoints[3][0] = endpoints[2][0];
        endpoints[3][1] = endpoints[1][1];
        endpoints[3][2] = mid_z;

        for (i = 0; i < 5; i++) {
            vec_t *ep = (i == 0) ? dest : endpoints[i - 1];
            if (G_LocationalTracePassed(ep, origin, targ->s.number, 0x802891)) {
                return 1.0f;
            }
        }
        return 0.0f;
    }
}

static float G_GetHitLocDamageMult(int weapon, hitLocation_t hitLoc)
{
    void *weapDef;

    if (weapon == 0) {
        return g_fHitLocDamageMult[hitLoc];
    }

    weapDef = BG_GetWeaponDef(weapon);
    if (weapDef == NULL || ((WeaponDef *)weapDef)->weapType != WEAPTYPE_BULLET) {
        return g_fHitLocDamageMult[hitLoc];
    }

    return ((WeaponDef *)weapDef)->locationDamageMultipliers[hitLoc];
}

static int G_IsPlayerDamageable(gentity_t *targ)
{
    gclient_t *client = targ->client;

    if (targ->takedamage != 0) {
        if (client->noclip == 0 && client->ufo == 0) {
            if (client->sess.connected == 2) {
                return 1;
            }
        }
        return 0;
    }
    return 0;
}

void G_Damage(gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, const vec_t *dir, const vec_t *point, int damage, int dflags, int mod, hitLocation_t hitLoc, int timeOffset)
{
    int weapon;
    float fDamage;
    int iDamage;
    vec3_t localdir;
    int health;

    if (targ->client != NULL) {

        if (!G_IsPlayerDamageable(targ)) {
            return;
        }

        if (inflictor != NULL) {
            weapon = inflictor->s.weapon;
        } else if (attacker != NULL) {
            weapon = attacker->s.weapon;
        } else {
            weapon = 0;
        }

        fDamage = (float)damage;
        fDamage *= G_GetHitLocDamageMult(weapon, hitLoc);
        iDamage = (int)fDamage;

        if (iDamage <= 0) {
            return;
        }

        Scr_PlayerDamage(targ, inflictor, attacker, iDamage, dflags, mod, weapon, point, dir, hitLoc, timeOffset);
        return;
    }

    if (targ->takedamage == 0) {
        return;
    }

    if (inflictor == NULL) {

        inflictor = &g_entities[1022];
    }

    if (attacker == NULL) {
        attacker = &g_entities[1022];
    }

    Vec3NormalizeTo(dir, localdir);

    if (targ->flags & 1) {
        return;
    }

    if (damage <= 0) {
        damage = 1;
    }

    health = targ->health;
    if (targ->flags & 2) {

        if (health - damage <= 0) {
            damage = health - 1;
        }
    }

    {
        if ((*(const dvar_t **)imp_g_debugDamage)->current.enabled) {
            Com_Printf("target:%i health:%i damage:%i\n", targ->s.number, health, damage);
        }
    }

    health = targ->health;
    health -= damage;
    targ->health = health;

    Scr_AddEntity(attacker);
    Scr_AddInt(damage);
    Scr_Notify(targ, SCR_CONST()->damage, 2);

    if (health > 0) {

        void (*pain)(gentity_t *, gentity_t *, int, const vec_t *, int, hitLocation_t) =
            ((entityHandler_t *)imp_entityHandlers)[targ->handler].pain;
        if (pain != NULL) {
            pain(targ, attacker, damage, point, mod, hitLoc);
        }
    } else {

        if (health < -999) {
            targ->health = -999;
        }

        Scr_AddEntity(attacker);
        Scr_Notify(targ, SCR_CONST()->death, 1);

        void (*die)(gentity_t *, gentity_t *, gentity_t *, int, int, int, const vec_t *, hitLocation_t, int) =
            ((entityHandler_t *)imp_entityHandlers)[targ->handler].die;
        if (die != NULL) {
            int iWeapon = inflictor->s.weapon;
            die(targ, inflictor, attacker, damage, mod, iWeapon, localdir, hitLoc, timeOffset);
        }
    }
}

qboolean G_RadiusDamage(const vec_t *origin, gentity_t *inflictor, gentity_t *attacker, float fInnerDamage, float fOuterDamage, float radius, gentity_t *ignore, int mod)
{
    vec3_t mins, maxs;
    int entityList[1024];
    int numListedEntities;
    int e;
    int i;
    vec3_t v;
    vec3_t dir;
    vec3_t dest;
    float dist;
    float points;
    float damageScale;
    int hitClient;
    gentity_t *ent;
    float radiusTweak;
    void *tr_buf[7];

    if (attacker == NULL) {
        hitClient = 0;
        goto done;
    }

    if (radius < 1.0f) {
        radius = 1.0f;
    }

    float searchRadius = radius * 1.4142135f;

    for (i = 0; i < 3; i++) {
        mins[i] = origin[i] - searchRadius;
        maxs[i] = origin[i] + searchRadius;
    }

    numListedEntities = CM_AreaEntities(mins, maxs, entityList, 1024, -1);
    if (numListedEntities <= 0) {
        hitClient = 0;
        goto done;
    }

    radiusTweak = radius * 0.2f;
    hitClient = 0;

    for (e = 0; e < numListedEntities; e++) {
        int entIdx = entityList[e];
        ent = &g_entities[entIdx];

        if (ent == ignore) {
            continue;
        }

        if (ent->takedamage == 0) {
            continue;
        }

        if (ent->r.bmodel == 0) {

            v[0] = ent->r.currentOrigin[0] - origin[0];
            v[1] = ent->r.currentOrigin[1] - origin[1];
            v[2] = ent->r.currentOrigin[2] - origin[2];
        } else {

            for (i = 0; i < 3; i++) {
                float emin = ent->r.absmin[i];
                float emax = ent->r.absmax[i];
                if (emin > origin[i]) {
                    v[i] = emin - origin[i];
                } else if (emax < origin[i]) {
                    v[i] = origin[i] - emax;
                } else {
                    v[i] = 0.0f;
                }
            }
        }

        dist = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

        if (dist >= radius) {
            continue;
        }

        if (ent->client != NULL) {
            if (level.bPlayerIgnoreRadiusDamage != 0) {
                continue;
            }
        }

        points = fInnerDamage - fOuterDamage;
        points = (1.0f - dist / radius) * points + fOuterDamage;

        damageScale = CanDamage(ent, origin);
        if (damageScale > 0.0f) {
            if (LogAccuracyHit(ent, attacker)) {
                hitClient = 1;
            }

            dir[0] = ent->r.currentOrigin[0] - origin[0];
            dir[1] = ent->r.currentOrigin[1] - origin[1];
            dir[2] = ent->r.currentOrigin[2] - origin[2] + 24.0f;

            G_Damage(ent, inflictor, attacker, dir, origin,
                     (int)(points * damageScale), 1, mod, 0, 0);
        } else {

            dest[0] = (ent->r.absmin[0] + ent->r.absmax[0]) * 0.5f;
            dest[1] = (ent->r.absmin[1] + ent->r.absmax[1]) * 0.5f;
            dest[2] = (ent->r.absmin[2] + ent->r.absmax[2]) * 0.5f;

            {
                byte *pw = *(byte **)&g_phys_world;
                G_TraceCapsule(tr_buf, origin, (const vec_t *)pw, (const vec_t *)pw, dest, 0x3ff, 0x811);
            }

            if (*(float *)tr_buf >= 1.0f) {
                continue;
            }

            dest[0] -= origin[0];
            dest[1] -= origin[1];
            dest[2] -= origin[2];
            float traceDist = sqrtf(dest[0] * dest[0] + dest[1] * dest[1] + dest[2] * dest[2]);
            if (radiusTweak <= traceDist) {
                continue;
            }

            if (LogAccuracyHit(ent, attacker)) {
                hitClient = 1;
            }

            dir[0] = ent->r.currentOrigin[0] - origin[0];
            dir[1] = ent->r.currentOrigin[1] - origin[1];
            dir[2] = ent->r.currentOrigin[2] - origin[2] + 24.0f;

            G_Damage(ent, inflictor, attacker, dir, origin,
                     (int)(points * 0.1f), 1, mod, 0, 0);
        }
    }

done:
    return hitClient;
}

void player_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath, int iWeapon, const vec_t *vDir, const hitLocation_t hitLoc, int psTimeOffset)
{
    gclient_t *cl;
    bgs_t *savedBgs;
    int dobj;
    vec3_t dir;
    vec3_t launchvel;
    vec3_t launchspot;
    int animResult;
    int i;
    float yaw;
    const scr_const_t *scr = SCR_CONST();

    cl = self->client;
    dobj = Com_GetServerDObj(cl->ps.clientNum);
    if (!dobj) {
        return;
    }

    cl = self->client;
    if ((unsigned int)cl->ps.pm_type > 1) {
        return;
    }
    if (cl->ps.pm_flags & 0x00400000) {
        return;
    }

    /* Client commands can trigger death between server frames, when the
       shared animation context is unset. Always use the server context. */
    savedBgs = *(bgs_t **)imp_bgs;
    *(bgs_t **)imp_bgs = &level_bgs;

    if (attacker->s.eType == 9) {
        int ownerNum = attacker->r.ownerNum;
        if (ownerNum != 0x3ff) {
            attacker = &g_entities[ownerNum];
        }
    }

    Scr_AddEntity(attacker);
    Scr_Notify(self, scr->death, 1);

    if (iWeapon != 0) {
        gclient_t *att_cl = attacker->client;
        if (att_cl != NULL && (att_cl->ps.eFlags & 0x300) != 0) {

            int ownerIdx = attacker->s.otherEntityNum;
            gentity_t *mg42Ent = &g_entities[ownerIdx];
            if (mg42Ent->s.eType == 9) {
                iWeapon = mg42Ent->s.weapon;
            }
        }
    }

    cl = self->client;
    if (cl->ps.grenadeTimeLeft != 0) {

        float lv_x, lv_y, lv_z;

        lv_x = crandom();
        lv_y = crandom();
        lv_z = randomf();

        launchvel[0] = lv_x * 160.0f;
        launchvel[1] = lv_y * 160.0f;
        launchvel[2] = lv_z * 160.0f;

        launchspot[0] = self->r.currentOrigin[0];
        launchspot[1] = self->r.currentOrigin[1];
        launchspot[2] = self->r.currentOrigin[2] + 40.0f;

        fire_grenade(self, launchspot, launchvel,
                     cl->ps.grenadeTimeLeft, cl->ps.offHandIndex);
        cl = self->client;
    }

    {
        int state = 0;
        if (cl->ps.pm_type == 1) {
            state = 1;
        }
        cl->ps.pm_type = state + 6;
    }

    animResult = BG_AnimScriptEvent(&cl->ps, 1, 0, 1);

    Scr_PlayerKilled(self, inflictor, attacker, damage, meansOfDeath,
                     iWeapon, vDir, hitLoc, psTimeOffset, animResult);

    {
        int maxClients = level.maxclients;

        if (maxClients > 0) {
            for (i = 0; i < maxClients; i++) {
                gclient_t *client_i = &level.clients[i];
                if (client_i->sess.connected == 2 && client_i->sess.sessionState == 2) {
                    if (client_i->spectatorClient == self->s.number) {
                        Cmd_Score_f(&g_entities[i]);
                    }
                }
            }
        }
    }

    self->takedamage = 1;
    self->r.contents = 0x4000000;
    self->r.currentAngles[2] = 0.0f;

    if (attacker != NULL && attacker != self) {

        dir[0] = attacker->r.currentOrigin[0] - self->r.currentOrigin[0];
        dir[1] = attacker->r.currentOrigin[1] - self->r.currentOrigin[1];
        dir[2] = attacker->r.currentOrigin[2] - self->r.currentOrigin[2];
    } else if (inflictor != NULL && inflictor != self) {

        dir[0] = inflictor->r.currentOrigin[0] - self->r.currentOrigin[0];
        dir[1] = inflictor->r.currentOrigin[1] - self->r.currentOrigin[1];
        dir[2] = inflictor->r.currentOrigin[2] - self->r.currentOrigin[2];
    } else {

        cl = self->client;
        cl->ps.stats[1] = (int)self->r.currentAngles[1];
        goto after_yaw;
    }

    cl = self->client;
    yaw = vectoyaw(dir);
    cl->ps.stats[1] = (int)yaw;
    vectoyaw(dir);

after_yaw:
    cl = self->client;
    cl->ps.viewangles[0] = self->r.currentAngles[0];
    cl->ps.viewangles[1] = self->r.currentAngles[1];
    cl->ps.viewangles[2] = self->r.currentAngles[2];

    self->s.loopSound = 0;
    SV_UnlinkEntity(self);
    self->r.maxs[2] = 30.0f;
    SV_LinkEntity(self);
    self->health = 0;
    self->handler = 11;
    *(bgs_t **)imp_bgs = savedBgs;
}

const char str_002b64e0[] = "MOD_UNKNOWN";
const char str_002b64ec[] = "MOD_PISTOL_BULLET";
const char str_002b6500[] = "MOD_RIFLE_BULLET";
const char str_002b6514[] = "MOD_GRENADE";
const char str_002b6520[] = "MOD_GRENADE_SPLASH";
const char str_002b6534[] = "MOD_PROJECTILE";
const char str_002b6544[] = "MOD_PROJECTILE_SPLASH";
const char str_002b655c[] = "MOD_MELEE";
const char str_002b6568[] = "MOD_HEAD_SHOT";
const char str_002b6578[] = "MOD_CRUSH";
const char str_002b6584[] = "MOD_TELEFRAG";
const char str_002b6594[] = "MOD_FALLING";
const char str_002b65a0[] = "MOD_SUICIDE";
const char str_002b65ac[] = "MOD_TRIGGER_HURT";
const char str_002b65c0[] = "MOD_EXPLOSIVE";
