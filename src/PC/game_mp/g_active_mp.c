#include "common_types.h"
#include "imports.h"
#include <string.h>
extern bgs_t level_bgs;
extern void Com_Printf(const char *fmt, ...);

#ifndef qfalse
#    define qfalse 0
#    define qtrue 1
#endif

extern int Scr_IsSystemActive(int);
extern void Scr_AddEntity(gentity_t *ent);
extern void Scr_Notify(gentity_t *ent, int stringValue, unsigned int paramcount);
extern void BG_WeaponFireRecoil(playerState_t *ps, float *kickAVel, float *kickAVel_out);
extern void BG_Player_DoControllers(void *obj, gentity_t *self, int *partBits, clientInfo_t *ci, int serverTime);
extern void *Com_GetServerDObj(int entityNum);
extern void SV_GetUsercmd(int clientNum, usercmd_t *ucmd);
extern void ClientThink_real(gentity_t *ent, usercmd_t *ucmd);
extern unsigned char G_SetFixedLink(gentity_t *ent, int mode);
extern unsigned char G_SetOrigin(gentity_t *ent, const vec_t *origin);
extern unsigned char G_SetAngle(gentity_t *ent, const vec_t *angles);
extern void SV_LinkEntity(gentity_t *ent);
extern void SV_UnlinkEntity(gentity_t *ent);
extern void SV_GameDropClient(int clientNum, const char *reason);
extern void SV_GameSendServerCommand(int clientNum, int type, const char *text);
extern const char *va(const char *fmt, ...);
extern void Pmove(pmove_t *pm);
extern int PM_GetEffectiveStance(playerState_t *ps);
extern int PM_GetViewHeightLerpTime(const playerState_t *ps, int iTarget, qboolean bDown);
extern float AngleNormalize180(float angle);
extern qboolean Cmd_FollowCycle_f(gentity_t *ent, int dir);
extern void StopFollowing(gentity_t *ent);
extern int CM_AreaEntities(const vec_t *mins, const vec_t *maxs, int *entityList, int maxcount, int contentmask);
extern void ShrinkBoundsToHeight(vec_t *mins, vec_t *maxs);
extern qboolean SV_EntityContact(const vec_t *mins, const vec_t *maxs, const gentity_t *gEnt);
extern qboolean BG_PlayerTouchesItem(playerState_t *ps, entityState_t *item, int atTime);
extern void vectoangles(vec_t *dir, vec_t *angles);
extern void AnglesToAxis(const vec_t *angles, vec3_t *axis);
extern float sqrtf(float x);
extern float crandom(void);
extern float Vec2Normalize(vec_t *v);
extern qboolean SV_GetArchivedClientInfo(int clientNum, int *pArchiveTime, playerState_t *ps, clientState_t *cs);
extern void HudElem_UpdateClient(gclient_t *client, int clientNum, int which);
extern void G_Damage(gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, const vec_t *dir, const vec_t *point, int damage, int dflags, int mod, int hitLoc, int timeOffset);
extern void FireWeaponAntiLag(gentity_t *ent, int gameTime);
extern void FireWeaponMelee(gentity_t *ent);
extern void G_UseOffHand(gentity_t *ent);
extern void player_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath, int iWeapon, const vec_t *vDir, int hitLoc, int psTimeOffset);
extern const char *G_ModelName(int index);
extern int G_TagIndex(const char *name);
extern const char *SL_ConvertToString(unsigned int stringValue);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern void ClientSpawn(gentity_t *ent, const vec_t *spawn_origin, const vec_t *spawn_angles);
extern void Player_UpdateLookAtEntity(gentity_t *ent);
extern void Player_UpdateCursorHints(gentity_t *ent);
extern void G_UpdateGrenadeHint(gentity_t *ent);
extern void G_BeginGrenadeInput(gentity_t *ent, const usercmd_t *cmd);
extern void G_EndGrenadeInput(gentity_t *ent);
extern void G_GrenadeCookOff(gentity_t *ent);
extern void G_GetPlayerViewOrigin(const gentity_t *ent, vec_t *origin);
extern int G_GetNonPVSFriendlyInfo(gentity_t *pSelf, vec_t *vPosition, int iLastUpdateEnt);
extern void BG_PlayerStateToEntityState(playerState_t *ps, gentity_t *ent, qboolean snap, qboolean forceSnap);
extern float BG_GetSpeed(const playerState_t *ps, int serverTime);
extern void BG_CalculateViewAngles(viewState_t *vs, vec_t *angles);
extern void BG_CalculateWeaponPosition_Sway(playerState_t *ps, vec_t *swayViewAngles, vec_t *swayOffset, vec_t *swayAngles, float ssSwayScale, int frametime);
extern WeaponDef *BG_GetWeaponDef(int iWeapon);
extern qboolean BG_IsAimDownSightWeapon(int iWeapon);
extern void BG_CalculateWeaponAngles(weaponState_t *ws, vec_t *angles);
extern void MatrixMultiply(const vec_t *in1, const vec_t *in2, vec_t *out);
extern void AxisToAngles(const vec_t *axis, vec_t *angles);
extern gentity_t *G_Spawn(void);
extern void BG_UpdatePlayerDObj(struct DObj_s *pDObj, entityState_t *es, clientInfo_t *ci, int attachIgnoreCollision);
extern void BG_PlayerAnimation(const struct DObj_s *pDObj, entityState_t *es, clientInfo_t *ci);
extern void G_SafeDObjFree(gentity_t *ent);
extern qboolean SV_DObjExists(gentity_t *ent);
extern int G_DObjCalcPose(gentity_t *ent);
extern void SV_XModelDebugBoxes(gentity_t *ent);
extern void turret_think_client(gentity_t *self);
extern void SV_SetConfigstring(int index, const char *val);

#define g_entities ((gentity_t *)imp_g_entities)
extern struct level_locals_t level;
extern const dvar_t *g_synchronousClients;
extern const dvar_t *g_inactivity;
extern const dvar_t *g_gravity;
extern const dvar_t *g_speed;
extern const dvar_t *g_smoothClients;
extern const dvar_t *g_debugLocDamage;
extern const dvar_t *g_mantleBlockTimeBuffer;
extern entityHandler_t entityHandlers[20];

#define SCR_CONST() ((const scr_const_t *)imp_scr_const)

COD2_ASSERT_FIELD(gclient_t, sess.sessionState, 0x26a8);
COD2_ASSERT_FIELD(gclient_t, sess.noSpectate,   0x2740);
COD2_ASSERT_FIELD(gclient_t, spectatorClient,   0x27a8);
COD2_ASSERT_FIELD(gclient_t, noclip,            0x27ac);
COD2_ASSERT_FIELD(gclient_t, ufo,               0x27b0);
COD2_ASSERT_FIELD(gclient_t, buttons,           0x27bc);
COD2_ASSERT_FIELD(gclient_t, oldbuttons,        0x27c0);
COD2_ASSERT_FIELD(gclient_t, buttonsSinceLastFrame, 0x27c8);
COD2_ASSERT_FIELD(gclient_t, lastServerTime,    0x289c);
COD2_ASSERT_FIELD(gclient_t, ps.pm_type,        0x0004);
COD2_ASSERT_FIELD(WeaponDef, overlayReticle,    0x0278);
COD2_ASSERT_FIELD(level_locals_t, teamScores,   0x01fc);
COD2_ASSERT_FIELD(gclient_t, buttons,              0x27bc);
COD2_ASSERT_FIELD(gclient_t, oldbuttons,           0x27c0);
COD2_ASSERT_FIELD(gclient_t, buttonsSinceLastFrame, 0x27c8);

#define BYTE_AT(base, off) (*(byte *)((byte *)(base) + (off)))

static inline __attribute__((always_inline)) clientInfo_t *G_ClientInfoForEntity(const gentity_t *ent);
static inline __attribute__((always_inline)) qboolean G_UpdateClientInfoModel(gentity_t *ent, gclient_t *client, clientInfo_t *ci);
static void G_ClientEndFrameIntermission(gentity_t *ent, gclient_t *client);

void ClientImpacts(gentity_t *ent, pmove_t *pm);
void Player_UpdateActivate(gentity_t *ent);
qboolean G_ClientCanSpectateTeam(gclient_t *client, team_t team);
qboolean ClientInactivityTimer(gclient_t *client);
void G_SetLastServerTime(int clientNum, int lastServerTime);
void G_SetClientContents(gentity_t *pEnt);
qboolean GetFollowPlayerState(int clientNum, playerState_t *ps);
void G_PlayerController(gentity_t *self, int *partBits);
void G_PlayerEvent(int clientNum, int event);
void SpectatorThink(gentity_t *ent, usercmd_t *ucmd);
void SpectatorClientEndFrame(gentity_t *ent);
void G_TouchTriggers(gentity_t *ent);
void ClientEvents(gentity_t *ent, int oldEventSequence);
void P_DamageFeedback(gentity_t *player);
qboolean StuckInClient(gentity_t *self);
static void __attribute_regparm__(3) G_PlayerStateToEntityStateExtrapolate(playerState_t *ps, entityState_t *s, int time, qboolean snap);
void G_RunClient(gentity_t *ent);
void ClientThink(int clientNum);
void ClientEndFrame(gentity_t *ent);

void ClientImpacts(gentity_t *ent, pmove_t *pm)
{
    int i, j;
    gentity_t *other;
    void (*entTouch)(gentity_t *, gentity_t *, int);
    void (*otherTouch)(gentity_t *, gentity_t *, int);
    int numtouch;
    int *touchents;
    int entityNum;
    int duplicate;

    entTouch = ((entityHandler_t *)imp_entityHandlers)[ent->handler].touch;

    numtouch = pm->numtouch;
    if (numtouch <= 0)
        return;

    touchents = pm->touchents;
    entityNum = touchents[0];

    for (i = 0;;) {
        other = &g_entities[entityNum];

        if (((int (*)(int))Scr_IsSystemActive)(1)) {
            Scr_AddEntity(other);
            Scr_Notify(ent, SCR_CONST()->touch, 1);
            Scr_AddEntity(ent);
            Scr_Notify(other, SCR_CONST()->touch, 1);
        }

        otherTouch = ((entityHandler_t *)imp_entityHandlers)[other->handler].touch;
        if (otherTouch) {
            otherTouch(other, ent, 1);
        }

        if (entTouch) {
            entTouch(ent, other, 1);
        }

    next_iteration:
        i++;
        if (i >= pm->numtouch)
            break;

        entityNum = touchents[i];

        if (i > 0) {

            duplicate = 0;
            for (j = 0; j < i; j++) {
                if (touchents[j] == entityNum) {
                    duplicate = 1;
                    break;
                }
            }
            if (duplicate)
                goto next_iteration;
        }
    }
}

qboolean G_ClientCanSpectateTeam(gclient_t *client, team_t team)
{
    return !((client->sess.noSpectate >> team) & 1);
}

qboolean ClientInactivityTimer(gclient_t *client)
{
    int inactivity = (*(const dvar_t **)imp_g_inactivity)->current.integer;
    int clientNum;

    if (!inactivity) {
        client->inactivityTime = ((struct level_locals_t *)imp_level)->time + 60000;
        client->inactivityWarning = 0;
        return 1;
    }

    if (*(short *)&client->sess.cmd.forwardmove || (client->sess.cmd.buttons & 0x401)) {
        client->inactivityTime = ((struct level_locals_t *)imp_level)->time + inactivity * 1000;
        client->inactivityWarning = 0;
        return 1;
    }

    if (client->sess.localClient)
        return 1;

    clientNum = (int)(client - level.clients);

    if (((struct level_locals_t *)imp_level)->time > client->inactivityTime) {
        SV_GameDropClient(clientNum, "GAME_DROPPEDFORINACTIVITY");
        return 0;
    }

    if (((struct level_locals_t *)imp_level)->time > client->inactivityTime - 10000 && !client->inactivityWarning) {
        client->inactivityWarning = 1;
        SV_GameSendServerCommand(clientNum, 0, va("%c \"GAME_INACTIVEDROPWARNING\"", 99));
    }

    return 1;
}

void G_SetLastServerTime(int clientNum, int lastServerTime)
{
    gentity_t *ent = &g_entities[clientNum];
    gclient_t *client;
    int levelTime = ((struct level_locals_t *)imp_level)->time;

    if (levelTime - lastServerTime >= 1001)
        lastServerTime = levelTime - 1000;

    client = ent->client;

    if (lastServerTime >= client->lastServerTime || lastServerTime >= levelTime)
        client->lastServerTime = lastServerTime;
}

void G_SetClientContents(gentity_t *pEnt)
{
    gclient_t *client = pEnt->client;

    if (client->noclip || client->ufo) {
        pEnt->r.contents = 0;
        return;
    }

    if (client->sess.sessionState == SESS_STATE_DEAD)
        pEnt->r.contents = 0;
    else
        pEnt->r.contents = 0x2000000;
}

qboolean GetFollowPlayerState(int clientNum, playerState_t *ps)
{
    gentity_t *ent = &g_entities[clientNum];
    gclient_t *client = ent->client;

    if (!(client->ps.pm_flags & 0x800000))
        return 0;

    memcpy(ps, client, sizeof(playerState_t));

    memset((byte *)ps + 0x7A8, 0, 0xF80);

    return 1;
}

void G_PlayerController(gentity_t *self, int *partBits)
{
    bgs_t *gameStatics = &level_bgs;
    clientInfo_t *ci = &gameStatics->clientinfo[self->s.clientNum];
    void *obj;
    int frametime;

    *(void **)imp_bgs = gameStatics;
    frametime = ((struct level_locals_t *)imp_level)->frametime;
    obj = Com_GetServerDObj(self->s.number);
    BG_Player_DoControllers(obj, self, partBits, ci, frametime);
}

void G_PlayerEvent(int clientNum, int event)
{
    gentity_t *ent = &g_entities[clientNum];
    gclient_t *client = ent->client;
    vec3_t kickAVel;
    int idx;
    int bit;

    idx = event - 0x9E;
    if ((unsigned)idx > 0x11)
        return;
    bit = 1 << idx;
    if (!(bit & 0x20007))
        return;

    BG_WeaponFireRecoil(&client->ps, client->vGunSpeed, kickAVel);
}

void SpectatorThink(gentity_t *ent, usercmd_t *ucmd)
{
    gclient_t *client = ent->client;

    int oldButtons = client->buttons;
    int buttons = client->sess.cmd.buttons;
    pmove_t pm;

    client->oldbuttons = oldButtons;
    client->buttons = buttons;
    client->buttonsSinceLastFrame |= buttons & ~oldButtons;

    if (client->sess.forceSpectatorClient < 0) {
        if (!(client->sess.noSpectate & 0x10) && client->spectatorClient >= 0 &&
            ((buttons & 4) != (oldButtons & 4))) {
            StopFollowing(ent);
            buttons = client->buttons;
        }
    }

    if ((buttons & 1) && !(client->oldbuttons & 1)) {
        Cmd_FollowCycle_f(ent, 1);
    } else if ((buttons & 0x1000) && !(client->oldbuttons & 0x1000)) {
        Cmd_FollowCycle_f(ent, -1);
    }

    if (client->ps.pm_flags & 0x400000)
        return;

    client->ps.pm_type = 4;

    client->ps.speed = -(((client->sess.noSpectate >> 4) ^ 1) & 1) & 0x190;

    memset(&pm, 0, sizeof(pm));
    pm.ps = &client->ps;
    pm.cmd = *ucmd;
    pm.tracemask = 0x00800811;
    pm.handler = 1;

    Pmove(&pm);

    {
        vec_t *dst = ent->r.currentOrigin;
        vec_t *src = client->ps.origin;
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
    }
    SV_UnlinkEntity(ent);
}

void SpectatorClientEndFrame(gentity_t *ent)
{
    gclient_t *client = ent->client;
    playerState_t ps;
    clientState_t cs;
    int psTime;
    int flags;

    ent->r.svFlags = (ent->r.svFlags & 0xfd) | 1;
    ent->takedamage = 0;
    ent->r.contents = 0;
    client->ps.pm_flags &= ~0x00800000;
    ent->s.eType = 5;
    client->ps.viewmodelIndex = 0;
    client->fGunPitch = 0.0f;
    client->fGunYaw = 0.0f;

    if (client->sess.forceSpectatorClient >= 0) {
        client->spectatorClient = client->sess.forceSpectatorClient;

        for (;;) {
            if (client->sess.archiveTime < 0) {
                client->sess.archiveTime = 0;
            }

            psTime = client->sess.archiveTime - client->sess.psOffsetTime;
            if (SV_GetArchivedClientInfo(client->sess.forceSpectatorClient, &psTime, &ps, &cs) &&
                !((client->sess.noSpectate >> cs.team) & 1)) {
                goto useArchivedState;
            }

            if (!client->sess.archiveTime) {
                client->sess.forceSpectatorClient = -1;
                client->spectatorClient = -1;
                break;
            }

            client->sess.archiveTime -= 50;
        }
    }

    if (client->spectatorClient < 0 && (client->sess.noSpectate & 0x10)) {
        Cmd_FollowCycle_f(ent, 1);
    }

    if (client->spectatorClient >= 0) {
        psTime = client->sess.archiveTime + client->sess.psOffsetTime;
        if (SV_GetArchivedClientInfo(client->spectatorClient, &psTime, &ps, &cs) &&
            !((client->sess.noSpectate >> cs.team) & 1)) {
            goto useArchivedState;
        }
    }

    StopFollowing(ent);
    flags = client->ps.pm_flags & ~0x02000000;
    client->ps.pm_flags = flags;
    if ((client->sess.noSpectate & 4) &&
        (client->sess.noSpectate & 2) &&
        (client->sess.noSpectate & 1)) {
        client->ps.pm_flags &= ~0x03000000;
    } else {
        client->ps.pm_flags = flags | 0x01000000;
    }
    return;

useArchivedState:
    flags = (ps.eFlags & ~0x00100000) | (client->ps.eFlags & 0x00100000);
    memcpy(client, &ps, sizeof(playerState_t));
    HudElem_UpdateClient(client, ent->s.number, 2);
    client->ps.eFlags = flags;

    flags = (client->ps.pm_flags & ~0x00800000) | 0x00400000;
    client->ps.pm_flags = flags;

    if (client->sess.forceSpectatorClient >= 0) {
        client->ps.pm_flags = flags & ~0x03000000;
    } else if (client->sess.noSpectate & 0x10) {
        client->ps.pm_flags = (client->ps.pm_flags | 0x01400000) & ~0x02000000;
    } else {
        client->ps.pm_flags = (flags & ~0x00800000) | 0x03400000;
    }
}

void G_TouchTriggers(gentity_t *ent)
{
    static vec3_t range = { 40.0f, 40.0f, 52.0f };
    int touch[1024];
    vec3_t mins;
    vec3_t maxs;
    int num;
    int i;
    gclient_t *client = ent->client;
    fn_touch entTouch;

    if (client->ps.pm_type > 1)
        return;

    mins[0] = client->ps.origin[0] - range[0];
    mins[1] = client->ps.origin[1] - range[1];
    mins[2] = client->ps.origin[2] - range[2];
    maxs[0] = client->ps.origin[0] + range[0];
    maxs[1] = client->ps.origin[1] + range[1];
    maxs[2] = client->ps.origin[2] + range[2];

    num = CM_AreaEntities(mins, maxs, touch, 1024, 0x405c0008);

    mins[0] = client->ps.origin[0] + ent->r.mins[0];
    mins[1] = client->ps.origin[1] + ent->r.mins[1];
    mins[2] = client->ps.origin[2] + ent->r.mins[2];
    maxs[0] = client->ps.origin[0] + ent->r.maxs[0];
    maxs[1] = client->ps.origin[1] + ent->r.maxs[1];
    maxs[2] = client->ps.origin[2] + ent->r.maxs[2];
    ShrinkBoundsToHeight(mins, maxs);

    entTouch = ((entityHandler_t *)imp_entityHandlers)[ent->handler].touch;

    for (i = 0; i < num; ++i) {
        gentity_t *hit = &g_entities[touch[i]];
        fn_touch hitTouch = ((entityHandler_t *)imp_entityHandlers)[hit->handler].touch;

        if (!hitTouch && !entTouch)
            continue;

        if (hit->s.eType == 3 && getenv("DBGSPAM")) {
            Com_Printf("[touch] num=%d i=%d touch[i]=%d hit#=%d eType=%d posTr=%d\n",
                       num, i, touch[i], hit->s.number, hit->s.eType, hit->s.pos.trType);
        }
        if (hit->s.eType == 3 && !BG_PlayerTouchesItem(&client->ps, &hit->s, level.time))
            continue;

        if (!SV_EntityContact(mins, maxs, hit))
            continue;

        if (Scr_IsSystemActive(1)) {
            Scr_AddEntity(ent);
            Scr_Notify(hit, SCR_CONST()->touch, 1);
            Scr_AddEntity(hit);
            Scr_Notify(ent, SCR_CONST()->touch, 1);
        }

        if (hitTouch)
            hitTouch(hit, ent, 1);
    }
}

void ClientEvents(gentity_t *ent, int oldEventSequence)
{
    gclient_t *client = ent->client;
    int eventSequence = client->ps.eventSequence;
    int i;

    i = eventSequence - 4;
    if (i <= oldEventSequence) {
        i = oldEventSequence;
    }

    for (; i < client->ps.eventSequence; ++i) {
        int index = i & 3;
        int event = client->ps.events[index];
        int eventParm = client->ps.eventParms[index];

        if ((unsigned)(event - 0x74) <= 0x16) {
            if (ent->s.eType != 1) {
                break;
            }

            if (eventParm > 99) {
                G_Damage(ent, NULL, NULL, NULL, NULL, (int)(client->ps.stats[0] * 1.100000023841858f), 0, 11, 0, 0);
            } else {
                float damageFrac = (float)eventParm * 0.009999999776482582f;
                if (damageFrac != 0.0f) {
                    G_Damage(ent, NULL, NULL, NULL, NULL, (int)(client->ps.stats[0] * damageFrac), 0, 11, 0, 0);
                }
            }
            continue;
        }

        if ((unsigned)(event - 0x9e) > 0x27) {
            continue;
        }

        switch (event) {
        case 0x9e: /* EV_FIRE_WEAPON */
        case 0x9f: /* EV_FIRE_WEAPONB */
        case 0xa0: /* EV_FIRE_WEAPON_LASTSHOT */
        case 0xaf: /* EV_FIRE_WEAPON_MG42 */
            if (((const dvar_t *)*(void **)imp_g_antilag)->current.enabled) {
                FireWeaponAntiLag(ent, client->lastServerTime);
            } else {
                FireWeaponAntiLag(ent, level.time);
            }
            break;
        case 0xa4: /* EV_FIRE_MELEE */
            FireWeaponMelee(ent);
            break;
        case 0xa6: /* EV_USE_OFFHAND; selecting it (0xa7) must not throw. */
            G_UseOffHand(ent);
            break;
        case 0xa8: /* EV_BINOCULAR_ENTER */
            Scr_Notify(ent, SCR_CONST()->binocular_enter, 0);
            break;
        case 0xa9: /* EV_BINOCULAR_EXIT */
            Scr_Notify(ent, SCR_CONST()->binocular_exit, 0);
            break;
        case 0xaa: /* EV_BINOCULAR_FIRE */
            Scr_Notify(ent, SCR_CONST()->binocular_fire, 0);
            break;
        case 0xab: /* EV_BINOCULAR_RELEASE */
            Scr_Notify(ent, SCR_CONST()->binocular_release, 0);
            break;
        case 0xac: /* EV_BINOCULAR_DROP */
            Scr_Notify(ent, SCR_CONST()->binocular_drop, 0);
            break;
        case 0xc5: /* EV_GRENADE_SUICIDE */
            if (ent->client)
                G_GrenadeCookOff(ent);
            break;
        default:
            break;
        }
    }
}

void P_DamageFeedback(gentity_t *player)
{
    gclient_t *client = player->client;
    int damage;
    float damageFloat;
    float kick;
    float negativeKick;

    if (client->ps.pm_type > 5)
        return;

    client->ps.damageCount = 0;

    if (client->damage_blood <= 0)
        return;
    if (client->sess.maxHealth <= 0)
        return;

    damage = (client->damage_blood * 100) / client->sess.maxHealth;
    if (damage > 127) {
        damage = 127;
        damageFloat = 127.0f;
    } else {
        damageFloat = (float)damage;
    }

    client->ps.aimSpreadScale += damageFloat;
    if (client->ps.aimSpreadScale > 255.0f)
        client->ps.aimSpreadScale = 255.0f;

    kick = damageFloat * 0.2f;
    if (kick < 5.0f) {
        kick = 5.0f;
        negativeKick = -5.0f;
    } else if (kick > 90.0f) {
        kick = 90.0f;
        negativeKick = -90.0f;
    } else {
        negativeKick = -kick;
    }

    if (client->damage_fromWorld) {
        client->v_dmg_roll = 0.0f;
        client->v_dmg_pitch = negativeKick;
        client->ps.damagePitch = 255;
        client->ps.damageYaw = 255;
        client->damage_fromWorld = 0;
    } else {
        vec3_t angles;
        vec3_t viewaxis[3];
        vec_t *damageFrom = client->damage_from;

        vectoangles(damageFrom, angles);
        AnglesToAxis(client->ps.viewangles, viewaxis);

        client->v_dmg_roll = (damageFrom[0] * viewaxis[1][0] + damageFrom[1] * viewaxis[1][1] + damageFrom[2] * viewaxis[1][2]) * negativeKick;
        client->v_dmg_pitch = (damageFrom[0] * viewaxis[0][0] + damageFrom[1] * viewaxis[0][1] + damageFrom[2] * viewaxis[0][2]) * kick;
        client->ps.damagePitch = (int)(angles[0] * (256.0f / 360.0f));
        client->ps.damageYaw = (int)(angles[1] * (256.0f / 360.0f));
    }

    ++client->ps.damageEvent;
    client->damageTime = ((struct level_locals_t *)imp_level)->time - 20;
    client->ps.damageCount = damage;
    client->damage_blood = 0;
}

qboolean StuckInClient(gentity_t *self)
{
    gclient_t *selfClient = self->client;
    int i;

    if (!(selfClient->ps.pm_flags & 0x800000))
        return qfalse;
    if (selfClient->sess.sessionState)
        return qfalse;
    if (self->r.contents != 0x2000000 && self->r.contents != 0x4000000)
        return qfalse;

    for (i = 0; i < level.maxclients; ++i) {
        gentity_t *ent = &g_entities[i];
        gclient_t *client;
        vec2_t vDelta;
        float radius;
        float otherSpeed;
        float selfSpeed;
        float ejectSpeed;
        const dvar_t *ejectDvar;

        if (!ent->r.inuse || !ent->takedamage || !ent->client)
            continue;

        client = ent->client;
        if (client->sess.connected != CON_CONNECTED)
            continue;
        if (!(client->ps.pm_flags & 0x800000))
            continue;
        if (client->sess.sessionState)
            continue;
        if (ent == self)
            continue;
        if (ent->health <= 0)
            continue;
        if (ent->r.contents != 0x2000000 && ent->r.contents != 0x4000000)
            continue;

        if (ent->r.absmin[0] > self->r.absmax[0])
            continue;
        if (self->r.absmin[0] > ent->r.absmax[0])
            continue;
        if (ent->r.absmin[1] > self->r.absmax[1])
            continue;
        if (self->r.absmin[1] > ent->r.absmax[1])
            continue;
        if (ent->r.absmin[2] > self->r.absmax[2])
            continue;
        if (self->r.absmin[2] > ent->r.absmax[2])
            continue;

        vDelta[0] = ent->r.currentOrigin[0] - self->r.currentOrigin[0];
        vDelta[1] = ent->r.currentOrigin[1] - self->r.currentOrigin[1];
        radius = self->r.maxs[0] + ent->r.maxs[0];
        if (vDelta[0] * vDelta[0] + vDelta[1] * vDelta[1] > radius * radius)
            continue;

        vDelta[0] = ent->r.currentOrigin[0] - self->r.currentOrigin[0] + crandom();
        vDelta[1] = ent->r.currentOrigin[1] - self->r.currentOrigin[1] + crandom();
        Vec2Normalize(vDelta);

        ejectDvar = *(const dvar_t **)imp_g_playerCollisionEjectSpeed;
        ejectSpeed = (float)ejectDvar->current.integer;

        otherSpeed = sqrtf(client->ps.velocity[0] * client->ps.velocity[0] + client->ps.velocity[1] * client->ps.velocity[1]) > 0.0f ? ejectSpeed : 0.0f;
        selfSpeed = sqrtf(selfClient->ps.velocity[0] * selfClient->ps.velocity[0] + selfClient->ps.velocity[1] * selfClient->ps.velocity[1]) > 0.0f ? ejectSpeed : 0.0f;

        if (otherSpeed < 0.0001f && selfSpeed < 0.0001f) {
            otherSpeed = (float)client->ps.speed;
            selfSpeed = (float)selfClient->ps.speed;
        }

        client->ps.velocity[0] = otherSpeed * vDelta[0];
        client->ps.velocity[1] = otherSpeed * vDelta[1];
        client->ps.pm_time = 300;
        /* Timed collision ejection is a movement flag. The same bit in
           eFlags means mounted turret and makes view code require its tag. */
        client->ps.pm_flags |= 0x200;

        selfClient->ps.velocity[0] = -selfSpeed * vDelta[0];
        selfClient->ps.velocity[1] = -selfSpeed * vDelta[1];
        selfClient->ps.pm_time = 300;
        selfClient->ps.pm_flags |= 0x200;
        return qtrue;
    }

    return qfalse;
}

static void __attribute_regparm__(3)
    G_PlayerStateToEntityStateExtrapolate(playerState_t *ps, entityState_t *s, int time, qboolean snap)
{
    int eventSequence;
    int i;

    s->pos.trType = TR_LINEAR_STOP;
    s->pos.trBase[0] = ps->origin[0];
    s->pos.trBase[1] = ps->origin[1];
    s->pos.trBase[2] = ps->origin[2];
    s->pos.trDelta[0] = ps->velocity[0];
    s->pos.trDelta[1] = ps->velocity[1];
    s->pos.trDelta[2] = ps->velocity[2];
    s->pos.trTime = time;
    s->pos.trDuration = 50;

    s->apos.trType = TR_INTERPOLATE;
    s->apos.trBase[0] = ps->viewangles[0];
    s->apos.trBase[1] = ps->viewangles[1];
    s->apos.trBase[2] = ps->viewangles[2];

    s->angles2[1] = (float)ps->movementDir;
    s->eFlags = ps->eFlags;

    if (ps->entityEventSequence - ps->eventSequence < 0) {
        if (ps->eventSequence - ps->entityEventSequence > 4) {
            ps->entityEventSequence = ps->eventSequence - 4;
        }
        s->eventParm = ps->eventParms[ps->entityEventSequence & 3] & 0xff;
        ++ps->entityEventSequence;
    } else {
        s->eventParm = 0;
    }

    if (ps->oldEventSequence - ps->eventSequence > 0)
        ps->oldEventSequence = ps->eventSequence;

    eventSequence = ps->oldEventSequence;
    while (eventSequence != ps->eventSequence) {
        const int eventSlot = eventSequence & 3;
        const int event = ps->events[eventSlot] & 0xff;
        const int *singleEvents = (const int *)imp_singleClientEvents;
        int singleEventIndex = 0;

        {
            gclient_t *evClient = g_entities[s->number].client;
            int evIdx = (unsigned char)(event - 0x9E);
            if (evIdx <= 0x11 && ((1 << evIdx) & 0x20007)) {
                vec3_t kickAVel;
                BG_WeaponFireRecoil(&evClient->ps, evClient->vGunSpeed, kickAVel);
            }
        }

        while (singleEvents[singleEventIndex] > 0 && singleEvents[singleEventIndex] != event) {
            ++singleEventIndex;
        }

        if (singleEvents[singleEventIndex] < 0) {
            const int outSlot = s->eventSequence & 3;
            s->events[outSlot] = event;
            s->eventParms[outSlot] = ps->eventParms[eventSlot] & 0xff;
            ++s->eventSequence;
        }

        ++eventSequence;
    }
    ps->oldEventSequence = eventSequence;

    s->weapon = ps->weapon & 0xff;
    s->groundEntityNum = (unsigned short)ps->groundEntityNum;
    s->eType = (ps->pm_flags & 0x00c00000) ? 1 : 5;

    if (snap) {
        for (i = 0; i < 3; ++i) {
            s->pos.trBase[i] = (float)(int)s->pos.trBase[i];
            s->apos.trBase[i] = (float)(int)s->apos.trBase[i];
        }
    }

    s->legsAnim = ps->legsAnim;
    s->torsoAnim = ps->torsoAnim;
    s->clientNum = ps->clientNum;

    if (ps->eFlags & 0x300) {
        s->otherEntityNum = ps->viewlocked_entNum;
    }

    if (ps->pm_type > 5) {
        s->eFlags |= 0x20000;
    } else {
        s->eFlags &= ~0x20000;
    }

    if (ps->pm_flags & 0x40) {
        s->eFlags |= 0x40000;
    } else {
        s->eFlags &= ~0x40000;
    }

    s->leanf = ps->leanf;

    if (PM_GetEffectiveStance(ps) == 1) {
        float frac;
        int lerpTime;

        lerpTime = PM_GetViewHeightLerpTime(ps, ps->viewHeightLerpTarget, ps->viewHeightLerpDown);
        frac = (float)(ps->commandTime - ps->viewHeightLerpTime) / (float)lerpTime;
        if (frac < 0.0f) {
            frac = 0.0f;
        }
        if (frac > 1.0f) {
            frac = 1.0f;
        }
        if (!ps->viewHeightLerpDown) {
            frac = 1.0f - frac;
        }

        s->fTorsoHeight = frac * ps->fTorsoHeight;
        s->fTorsoPitch = frac * AngleNormalize180(ps->fTorsoPitch);
        s->fWaistPitch = frac * AngleNormalize180(ps->fWaistPitch);
    } else {
        s->fTorsoHeight = 0.0f;
        s->fTorsoPitch = 0.0f;
        s->fWaistPitch = 0.0f;
    }
}

void ClientThink_real(gentity_t *ent, usercmd_t *ucmd)
{
    gclient_t *client;
    pmove_t pm;
    viewState_t vs;
    weaponState_t ws;
    vec3_t angles;
    vec3_t viewangles;
    WeaponDef *weapDef;
    int oldEventSequence;
    int msec;

    client = ent->client;

    if (client->sess.connected != 2)
        goto done;

    if (ucmd->serverTime > level.time + 200)
        ucmd->serverTime = level.time + 200;
    if (ucmd->serverTime < level.time - 1000)
        ucmd->serverTime = level.time - 1000;

    msec = ucmd->serverTime - client->ps.commandTime;
    if (msec <= 0) {
        if ((int)(ent - g_entities) == client->ps.clientNum)
            goto done;
    } else if (msec > 200) {
        msec = 200;
    }

    if (client->bFrozen)
        client->ps.pm_flags |= 0x8000;
    else
        client->ps.pm_flags &= ~0x8000;

    if (client->sess.sessionState == 3) {
        client->oldbuttons = client->buttons;
        client->buttons = ucmd->buttons;
        client->buttonsSinceLastFrame |= ~client->oldbuttons & client->buttons;
        goto done;
    }

    if (client->sess.sessionState == 2) {
        SpectatorThink(ent, ucmd);
        goto done;
    }

    if (!ClientInactivityTimer(client))
        goto done;

    oldEventSequence = client->ps.eventSequence;

    memset(&pm, 0, sizeof(pm));
    pm.ps = &client->ps;
    pm.cmd = *ucmd;
    pm.oldcmd = client->sess.oldcmd;
    pm.tracemask = (client->ps.pm_type < 6) ? 0x02810011 : 0x00810011;
    pm.handler = 1;
#ifdef PM_STATETRACE
    if (ent->s.number == 0 && (ucmd->buttons & 0x400)) {
        static int n;
        if (n++ < 40)
            fprintf(stderr, "[srv-jump] cmd.buttons=0x%x oldcmd.buttons=0x%x pm_type=%d pm_flags=0x%x jumpTime=%d srvTime=%d cmdTime=%d\n",
                    ucmd->buttons, client->sess.oldcmd.buttons, client->ps.pm_type, client->ps.pm_flags,
                    client->ps.jumpTime, ucmd->serverTime, client->ps.commandTime);
        fflush(stderr);
    }
#endif

    client->oldOrigin[0] = client->ps.origin[0];
    client->oldOrigin[1] = client->ps.origin[1];
    client->oldOrigin[2] = client->ps.origin[2];

    client->oldbuttons = client->buttons;
    client->buttons = ucmd->buttons;
    client->latched_buttons = ~client->oldbuttons & client->buttons;
    client->buttonsSinceLastFrame |= client->latched_buttons;

    memset(&vs, 0, sizeof(vs));
    vs.ps = &client->ps;
    vs.damageTime = client->damageTime;
    vs.time = level.time;
    vs.v_dmg_pitch = client->v_dmg_pitch;
    vs.v_dmg_roll = client->v_dmg_roll;
    vs.xyspeed = BG_GetSpeed(&client->ps, level.time);
    vs.frametime = (float)msec * 0.001f;
    vs.fLastIdleFactor = client->fLastIdleFactor;
    vs.weapIdleTime = &client->weapIdleTime;

    BG_CalculateViewAngles(&vs, angles);

    viewangles[0] = client->ps.viewangles[0] + angles[0];
    viewangles[1] = client->ps.viewangles[1] + angles[1];
    viewangles[2] = client->ps.viewangles[2] + angles[2];

    weapDef = BG_GetWeaponDef(client->ps.weapon);

    BG_CalculateWeaponPosition_Sway(&client->ps,
                                    client->swayViewAngles,
                                    client->swayOffset,
                                    client->swayAngles,
                                    1.0f,
                                    msec);

    memset(&ws, 0, sizeof(ws));
    ws.ps = &client->ps;
    ws.xyspeed = vs.xyspeed;
    ws.frametime = vs.frametime;
    ws.vLastMoveAng[0] = client->vLastMoveAng[0];
    ws.vLastMoveAng[1] = client->vLastMoveAng[1];
    ws.vLastMoveAng[2] = client->vLastMoveAng[2];
    ws.fLastIdleFactor = client->fLastIdleFactor;
    ws.time = vs.time;
    ws.damageTime = client->damageTime;
    ws.v_dmg_pitch = client->v_dmg_pitch;
    ws.v_dmg_roll = client->v_dmg_roll;
    ws.vGunOffset[0] = client->vGunOffset[0];
    ws.vGunOffset[1] = client->vGunOffset[1];
    ws.vGunOffset[2] = client->vGunOffset[2];
    ws.vGunSpeed[0] = client->vGunSpeed[0];
    ws.vGunSpeed[1] = client->vGunSpeed[1];
    ws.vGunSpeed[2] = client->vGunSpeed[2];
    ws.swayAngles[0] = client->swayAngles[0];
    ws.swayAngles[1] = client->swayAngles[1];
    ws.swayAngles[2] = client->swayAngles[2];
    ws.weapIdleTime = &client->weapIdleTime;

    BG_CalculateWeaponAngles(&ws, angles);

    if (BG_IsAimDownSightWeapon(ws.ps->weapon) && ws.ps->fWeaponPosFrac != 0.0f && !weapDef->overlayReticle) {
        vec3_t vAxis[3];
        vec3_t vAxis2[3];
        vec3_t vAxis3[3];

        AnglesToAxis(angles, vAxis);
        AnglesToAxis(viewangles, vAxis2);
        MatrixMultiply((const vec_t *)vAxis, (const vec_t *)vAxis2, (vec_t *)vAxis3);
        AxisToAngles((const vec_t *)vAxis3, viewangles);
    }

    client->vLastMoveAng[0] = ws.vLastMoveAng[0];
    client->vLastMoveAng[1] = ws.vLastMoveAng[1];
    client->vLastMoveAng[2] = ws.vLastMoveAng[2];
    client->fLastIdleFactor = ws.fLastIdleFactor;
    client->vGunOffset[0] = ws.vGunOffset[0];
    client->vGunOffset[1] = ws.vGunOffset[1];
    client->vGunOffset[2] = ws.vGunOffset[2];
    client->vGunSpeed[0] = ws.vGunSpeed[0];
    client->vGunSpeed[1] = ws.vGunSpeed[1];
    client->vGunSpeed[2] = ws.vGunSpeed[2];
    client->fGunPitch = viewangles[0];
    client->fGunYaw = viewangles[1];

    G_BeginGrenadeInput(ent, &pm.cmd);
    Pmove(&pm);
    G_EndGrenadeInput(ent);

    if (pm.mantleStarted) {
        gentity_t *owner;
        gentity_t *blocker;
        int duration;

        owner = &g_entities[pm.ps->clientNum];
        blocker = G_Spawn();
        blocker->parent = COD2_GEntityHandle(owner);
        blocker->r.ownerNum = pm.ps->clientNum;
        blocker->r.contents = 0x10000;
        blocker->clipmask = 0x10000;
        blocker->r.svFlags = 0x21;
        blocker->s.eType = 5;
        blocker->handler = 0x13;
        blocker->r.mins[0] = owner->r.mins[0];
        blocker->r.mins[1] = owner->r.mins[1];
        blocker->r.mins[2] = owner->r.mins[2];
        blocker->r.maxs[0] = owner->r.maxs[0];
        blocker->r.maxs[1] = owner->r.maxs[1];
        blocker->r.maxs[2] = owner->r.maxs[2];
        G_SetOrigin(blocker, pm.mantleEndPos);
        SV_LinkEntity(blocker);

        duration = level.time + pm.mantleDuration + g_mantleBlockTimeBuffer->current.integer;
        blocker->nextthink = duration;
    }

    ent->s.animMovetype = (client->ps.pm_flags & 2) != 0;

    if (oldEventSequence != client->ps.eventSequence) {
        ent->eventTime = level.time;
        ent->r.eventTime = level.time;
    }

    if (g_smoothClients->current.enabled) {
        G_PlayerStateToEntityStateExtrapolate(&client->ps, &ent->s, client->ps.commandTime, qtrue);
    } else {
        BG_PlayerStateToEntityState(&client->ps, ent, qtrue, qtrue);
    }

    ent->r.currentOrigin[0] = ent->s.pos.trBase[0];
    ent->r.currentOrigin[1] = ent->s.pos.trBase[1];
    ent->r.currentOrigin[2] = ent->s.pos.trBase[2];
    ent->r.mins[0] = pm.mins[0];
    ent->r.mins[1] = pm.mins[1];
    ent->r.mins[2] = pm.mins[2];
    ent->r.maxs[0] = pm.maxs[0];
    ent->r.maxs[1] = pm.maxs[1];
    ent->r.maxs[2] = pm.maxs[2];

    ClientEvents(ent, oldEventSequence);
    SV_LinkEntity(ent);

    client = ent->client;
    if (!client->noclip)
        G_TouchTriggers(ent);

    ent->r.currentOrigin[0] = client->ps.origin[0];
    ent->r.currentOrigin[1] = client->ps.origin[1];
    ent->r.currentOrigin[2] = client->ps.origin[2];
    ent->r.currentAngles[0] = 0.0f;
    ent->r.currentAngles[1] = client->ps.viewangles[1];
    ent->r.currentAngles[2] = 0.0f;

    ClientImpacts(ent, &pm);

    client = ent->client;
    if (oldEventSequence != client->ps.eventSequence)
        ent->eventTime = level.time;

    Player_UpdateActivate(ent);

done:
    (void)0;
}

void G_RunClient(gentity_t *ent)
{
    gclient_t *client;
    int pm_type;

    if (g_synchronousClients->current.enabled) {
        client = ent->client;
        client->sess.cmd.serverTime = level.time;
        ClientThink_real(ent, &client->sess.cmd);
    }

    client = ent->client;

    if (client->noclip)
        return;

    if (ent->tagInfo) {

        pm_type = (client->sess.sessionState == SESS_STATE_DEAD) ? 7 : 1;
        client->ps.pm_type = pm_type;
        G_SetFixedLink(ent, 2);
        G_SetOrigin(ent, ent->r.currentOrigin);
        G_SetAngle(ent, ent->r.currentAngles);
        ent->s.pos.trType = TR_INTERPOLATE;
        ent->s.apos.trType = TR_INTERPOLATE;
        SV_LinkEntity(ent);

        client->ps.origin[0] = ent->r.currentOrigin[0];
        client->ps.origin[1] = ent->r.currentOrigin[1];
        client->ps.origin[2] = ent->r.currentOrigin[2];
    } else {

        pm_type = client->ps.pm_type;
        if (pm_type == 1 || pm_type == 7)
            client->ps.pm_type = pm_type - 1;
    }
}

void ClientThink(int clientNum)
{
    gentity_t *ent = &g_entities[clientNum];
    gclient_t *client = ent->client;
    usercmd_t *cmd = &client->sess.cmd;
    usercmd_t *oldcmd = &client->sess.oldcmd;

    *(void **)imp_bgs = (void *)imp_level_bgs;

    memcpy(oldcmd, cmd, sizeof(*cmd));
    SV_GetUsercmd(clientNum, cmd);

    client->lastCmdTime = level.time;

    if (!g_synchronousClients->current.enabled) {
        ClientThink_real(ent, cmd);
    }

    *(void **)imp_bgs = NULL;
}

static inline __attribute__((always_inline)) clientInfo_t *G_ClientInfoForEntity(const gentity_t *ent)
{
    return &level_bgs.clientinfo[ent->s.clientNum];
}

static inline __attribute__((always_inline)) qboolean G_UpdateClientInfoModel(gentity_t *ent, gclient_t *client, clientInfo_t *ci)
{
    const char *modelName;
    const char *tagName;
    int i;
    qboolean bChanged = qfalse;

    modelName = G_ModelName(ent->model);
    client->sess.cs.modelindex = ent->model;
    if (strcmp(ci->model, modelName)) {
        I_strncpyz(ci->model, modelName, sizeof(ci->model));
        bChanged = qtrue;
    }

    for (i = 0; i < 6; ++i) {
        if (!ent->attachModelNames[i]) {
            ci->attachModelNames[i][0] = 0;
            ci->attachTagNames[i][0] = 0;
            client->sess.cs.attachModelIndex[i] = 0;
            client->sess.cs.attachTagIndex[i] = 0;
            continue;
        }

        modelName = G_ModelName(ent->attachModelNames[i]);
        client->sess.cs.attachModelIndex[i] = ent->attachModelNames[i];
        if (strcmp(ci->attachModelNames[i], modelName)) {
            I_strncpyz(ci->attachModelNames[i], modelName, sizeof(ci->attachModelNames[i]));
            bChanged = qtrue;
        }

        tagName = SL_ConvertToString(ent->attachTagNames[i]);
        client->sess.cs.attachTagIndex[i] = G_TagIndex(tagName);
        if (strcmp(ci->attachTagNames[i], tagName)) {
            I_strncpyz(ci->attachTagNames[i], tagName, sizeof(ci->attachTagNames[i]));
            bChanged = qtrue;
        }
    }

    return bChanged;
}

static void G_ClientEndFrameIntermission(gentity_t *ent, gclient_t *client)
{
    ent->r.svFlags = (ent->r.svFlags & 0xfd) | 1;
    ent->takedamage = 0;
    ent->r.contents = 0;

    client->ps.pm_flags &= 0xfc7fffff;
    client->ps.pm_type = 5;
    client->ps.eFlags &= 0xffdfffbf;
    client->ps.damageCount = 0;
    ent->s.eType = 5;

    SV_SetConfigstring(5, va("%i", level.teamScores[1]));
    SV_SetConfigstring(6, va("%i", level.teamScores[2]));
}

void ClientEndFrame(gentity_t *ent)
{
    gclient_t *client = ent->client;
    clientInfo_t *ci;
    qboolean bChanged;
    vec3_t spawn_origin;
    vec3_t spawn_angles;
    vec3_t vViewPos;
    void *obj;
    int compassInfo;
    int compassEnt;

    ent->handler = 10;
    client->ps.deltaTime = 0;

    ci = G_ClientInfoForEntity(ent);
    bChanged = G_UpdateClientInfoModel(ent, client, ci);

    if (client->sess.connected != CON_CONNECTED) {
        client->buttonsSinceLastFrame = 0;
        return;
    }

    if (client->sess.sessionState == SESS_STATE_INTERMISSION) {
        G_ClientEndFrameIntermission(ent, client);
        client->buttonsSinceLastFrame = 0;
        return;
    }

    if (client->sess.sessionState == SESS_STATE_SPECTATOR) {
        SpectatorClientEndFrame(ent);
        client->buttonsSinceLastFrame = 0;
        return;
    }

    if (client->ps.clientNum != ent->s.number) {
        spawn_origin[0] = client->ps.origin[0];
        spawn_origin[1] = client->ps.origin[1];
        spawn_origin[2] = client->ps.origin[2];
        spawn_angles[0] = 0.0f;
        spawn_angles[1] = client->ps.viewangles[1];
        spawn_angles[2] = 0.0f;
        ClientSpawn(ent, spawn_origin, spawn_angles);
        client->buttonsSinceLastFrame = 0;
        return;
    }

    ent->r.svFlags = (ent->r.svFlags | 2) & 0xfe;
    ent->takedamage = 1;

    client->ps.pm_flags |= 0x00800000;
    client->ps.pm_flags &= 0xfcffffff;
    client->ps.viewmodelIndex = client->sess.viewmodelIndex;

    G_SetClientContents(ent);

    client->dropWeaponTime = 0;
    if (client->compassPingTime <= level.time) {
        client->ps.eFlags &= 0xffbfffff;
    }

    if (client->noclip) {
        client->ps.pm_type = 2;
    } else if (client->ufo) {
        client->ps.pm_type = 3;
    } else if (client->sess.sessionState == SESS_STATE_DEAD) {
        client->ps.pm_type = (ent->count == 1) ? 6 : 7;
        ent->r.svFlags = (ent->r.svFlags | 1) & 0xfd;
        ent->takedamage = 0;
    } else {
        client->ps.pm_type = ent->tagInfo != 0;
    }

#ifdef PM_STATETRACE
    {
        static int last;
        int now = level.time;
        if (ent->s.number == 0 && now - last >= 1000) {
            last = now;
            fprintf(stderr, "[state] cl=0 pm_type=%d sessState=%d health=%d noclip=%d ufo=%d tagInfo=%p count=%d eFlags=0x%x\n",
                    client->ps.pm_type, client->sess.sessionState, ent->health, client->noclip,
                    client->ufo, (void *)ent->tagInfo, ent->count, client->ps.eFlags);
            fflush(stderr);
        }
    }
#endif

    client->ps.gravity = (int)(*(const dvar_t **)imp_g_gravity)->current.value;
    client->ps.speed = (*(const dvar_t **)imp_g_speed)->current.integer;
    client->currentAimSpreadScale = client->ps.aimSpreadScale / 255.0f;

    Player_UpdateLookAtEntity(ent);
    Player_UpdateCursorHints(ent);
    G_UpdateGrenadeHint(ent);
    P_DamageFeedback(ent);

    if (level.time - client->lastCmdTime > 1000) {
        ent->s.eFlags |= 0x80;
    } else {
        ent->s.eFlags &= 0xffffff7f;
    }

    client->ps.stats[0] = ent->health;
    /* ClientThink already exports events into this persistent ring. Clearing
     * its sequence here discards them before the next snapshot is sent. */

    if (g_smoothClients->current.enabled) {
        G_PlayerStateToEntityStateExtrapolate(&client->ps, &ent->s, client->ps.commandTime, qtrue);
    } else {
        BG_PlayerStateToEntityState(&client->ps, ent, qtrue, qtrue);
    }

    if (ent->health > 0 && StuckInClient(ent)) {
        ent->r.contents = 0x4000000;
    }

    G_GetPlayerViewOrigin(ent, vViewPos);
    compassInfo = G_GetNonPVSFriendlyInfo(ent, vViewPos, client->iLastCompassFriendlyInfoEnt);
    client->ps.iCompassFriendInfo = compassInfo;
    if (compassInfo) {
        compassEnt = compassInfo & 0x3f;
        client->iLastCompassFriendlyInfoEnt = compassEnt;
        if (BYTE_AT((byte *)imp_g_entities + compassEnt * sizeof(gentity_t), 0x0a) & 0x40) {
            client->ps.eFlags |= 0x00800000;
        } else {
            client->ps.eFlags &= 0xff7fffff;
        }
    } else {
        client->iLastCompassFriendlyInfoEnt = 0x3ff;
    }

    if (ent->s.eType != 1) {
        client->buttonsSinceLastFrame = 0;
        return;
    }

    ent->handler = (ent->health <= 0) ? 11 : 9;

    ci = G_ClientInfoForEntity(ent);
    ci->lerpMoveDir = (float)ent->s.eventParm;
    ci->lerpLean = ent->s.leanf;
    ci->playerAngles[0] = client->ps.viewangles[0];
    ci->playerAngles[1] = client->ps.viewangles[1];
    ci->playerAngles[2] = client->ps.viewangles[2];

    if (bChanged) {
        G_SafeDObjFree(ent);
    }

    obj = Com_GetServerDObj(ent->s.number);
    BG_UpdatePlayerDObj((struct DObj_s *)obj, &ent->s, ci, ent->attachIgnoreCollision);
    BG_PlayerAnimation((const struct DObj_s *)obj, &ent->s, ci);

    if ((BYTE_AT(client, 0x0e) & 0x80) && (client->ps.eFlags & 0x300)) {
        turret_think_client(&level.gentities[client->ps.viewlocked_entNum]);
    }

    if (g_debugLocDamage->current.enabled && SV_DObjExists(ent)) {
        G_DObjCalcPose(ent);
        SV_XModelDebugBoxes(ent);
    }

    client->buttonsSinceLastFrame = 0;
}
