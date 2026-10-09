#include "common_types.h"
#include "imports.h"
#include <string.h>
#include <stdlib.h>

vec3_t playerMaxs = { 15.0f, 15.0f, 70.0f };

extern level_locals_t level;

extern void Com_Printf(const char *fmt, ...);
extern void AngleVectors(const vec_t *angles, vec_t *forward, vec_t *right, vec_t *up);
extern float AngleDelta(float a1, float a2);
extern float AngleNormalize180(float angle);
extern float AngleNormalize360(float angle);
extern void CalculateRanks(void);
extern void Scr_Notify(gentity_t *ent, unsigned short name, int numArgs);
extern int Scr_IsSystemActive(int flag);
extern void Scr_AddString(const char *str);
extern void Scr_PlayerConnect(gentity_t *ent);
extern void Scr_PlayerDisconnect(gentity_t *ent);
extern void StopFollowing(gentity_t *ent);
extern void HudElem_ClientDisconnect(gentity_t *ent);
extern unsigned char G_FreeEntity(gentity_t *ent);
extern unsigned char G_InitGentity(gentity_t *ent);
extern unsigned char G_EntUnlink(gentity_t *ent);
extern unsigned char G_SetOrigin(gentity_t *ent, const vec_t *origin);
extern void G_SetClientContents(gentity_t *ent);
extern void G_ClientStopUsingTurret(gentity_t *ent);
extern int G_DObjGetWorldTagPos(gentity_t *ent, unsigned short tag, vec_t *origin);
extern void SV_UnlinkEntity(gentity_t *ent);
extern int SV_inSnapshot(vec_t *vPosition, int clientNum);
extern int OnSameTeam(gentity_t *a, gentity_t *b);
extern int SV_ClientHasClientMuted(int clientNum, int mutedClientNum);
extern int SV_ClientWantsVoiceData(int clientNum);
extern void SV_QueueVoicePacket(int talkerNum, int clientNum, VoicePacket_t *voicePacket);
extern void SV_GetUserinfo(int clientNum, char *buffer, int bufSize);
extern void SV_GetUsercmd(int clientNum, usercmd_t *dest);
extern int SV_IsLocalClient(int clientNum);
extern int Info_Validate(const char *s);
extern const char *Info_ValueForKey(const char *s, const char *key);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern int I_stricmp(const char *s1, const char *s2);
extern void Scr_SetString(scr_string_t *dest, unsigned int value);
extern void ClientEndFrame(gentity_t *ent);
extern void ClientThink_real(gentity_t *ent, usercmd_t *ucmd);
extern void BG_PlayerStateToEntityState(playerState_t *ps, gentity_t *ent, int snap, int forceOverride);
extern float BG_GetBobCycle(const playerState_t *ps);
extern float BG_GetSpeed(const playerState_t *ps, int serverTime);
extern float BG_GetVerticalBobFactor(const playerState_t *ps, float bobCycle, float xyspeed, float bobMax);
extern float BG_GetHorizontalBobFactor(const playerState_t *ps, float bobCycle, float xyspeed, float bobMax);
extern void AddLeanToPosition(vec_t *origin, float viewAngle, float leanFrac, float maxStand, float maxCrouch);
extern int ColorIndex(int c);
extern void Com_Error(int level, const char *fmt, ...);

extern vec3_t playerMins;
extern vec3_t playerMaxs;
extern const dvar_t *g_password;

extern gentity_t g_entities[];
extern level_locals_t level;

extern const dvar_t *voice_global;
extern const dvar_t *voice_deadChat;
extern const dvar_t *voice_localEcho;
extern const dvar_t *bg_bobMax;
extern bgs_t level_bgs;

void G_GetPlayerViewDirection(const gentity_t *ent, vec_t *forward, vec_t *right, vec_t *up);
void ClientBegin(int clientNum);
void ClientDisconnect(int clientNum);
int G_GetNonPVSFriendlyInfo(gentity_t *pSelf, vec_t *vPosition, int iLastUpdateEnt);
void G_BroadcastVoice(gentity_t *talker, VoicePacket_t *voicePacket);
void SetClientViewAngle(gentity_t *ent, const vec_t *angle);
void ClientSpawn(gentity_t *ent, const vec_t *spawn_origin, const vec_t *spawn_angles);
void G_GetPlayerViewOrigin(const gentity_t *ent, vec_t *origin);
void ClientUserinfoChanged(int clientNum);
char *ClientConnect(int clientNum, int scriptPersId);

#define GENTITY_STRIDE sizeof(gentity_s)
#define CLIENT_STRIDE sizeof(gclient_s)
#define SCR_CONST() ((const scr_const_t *)imp_scr_const)

static inline __attribute__((always_inline)) gclient_s *G_ClientForNum(int clientNum)
{
    return &level.clients[clientNum];
}
static inline __attribute__((always_inline)) gentity_s *G_EntityForNum(int entNum)
{
    return &((gentity_s *)imp_g_entities)[entNum];
}
static inline __attribute__((always_inline)) level_locals_t *G_Level(void)
{
    return &level;
}

void G_GetPlayerViewDirection(const gentity_t *ent, vec_t *forward, vec_t *right, vec_t *up)
{
    gclient_s *client = ent->client;
    AngleVectors(client->ps.viewangles, forward, right, up);
}

void ClientBegin(int clientNum)
{
    gclient_s *client = G_ClientForNum(clientNum);
    gentity_s *ent = G_EntityForNum(clientNum);
    const scr_const_t *scr = SCR_CONST();

    client->sess.connected = 2;
    client->ps.pm_type = 4;

    CalculateRanks();

    if (getenv("DBGSPAM"))
        Com_Printf("[team-trace] ClientBegin client=%d ent=%p begin=%u\n",
                   clientNum, (void *)ent, (unsigned int)scr->begin);
    Scr_Notify(ent, scr->begin, 0);
}

void ClientDisconnect(int clientNum)
{
    gclient_s *client = G_ClientForNum(clientNum);
    gentity_s *ent = G_EntityForNum(clientNum);
    level_locals_t *level = G_Level();
    const scr_const_t *scr = SCR_CONST();
    int i;

    if (Scr_IsSystemActive(1)) {
        Scr_AddString("disconnect");
        Scr_AddString("^1teleport");
        Scr_Notify(ent, scr->menuresponse, 2);
    }

    if (level->maxclients > 0) {
        for (i = 0; i < level->maxclients; i++) {
            gclient_s *otherClient = G_ClientForNum(i);
            gentity_s *otherEnt = G_EntityForNum(i);

            if (otherClient->sess.connected == 0)
                continue;
            if (otherClient->sess.sessionState != 2)
                continue;
            if (otherClient->spectatorClient != clientNum)
                continue;

            StopFollowing(otherEnt);
        }
    }

    HudElem_ClientDisconnect(ent);

    if (Scr_IsSystemActive(1)) {
        Scr_PlayerDisconnect(ent);
    }

    G_FreeEntity(ent);

    client->sess.connected = 0;
    memset(&client->sess.cs, 0, sizeof(clientState_t));

    CalculateRanks();
}

int G_GetNonPVSFriendlyInfo(gentity_t *pSelf, vec_t *vPosition, int iLastUpdateEnt)
{
    gclient_s *selfClient;
    int team;
    int iBaseEnt;
    int iEntCount;
    int ent_num;
    gentity_s *pEnt;
    gclient_s *pEntClient;
    int dx, dy;
    float scaleX, scaleY, ratioX, ratioY;

    selfClient = pSelf->client;
    team = selfClient->sess.cs.team;

    if (team == 0 || team == 3)
        return 0;

    if (iLastUpdateEnt == 0x3ff)
        iBaseEnt = 0;
    else
        iBaseEnt = iLastUpdateEnt + 1;

    for (iEntCount = 0; iEntCount < 64; iEntCount++) {
        ent_num = (iBaseEnt + iEntCount) % 64;
        if (ent_num < 0)
            ent_num += 64;

        pEnt = G_EntityForNum(ent_num);

        if (pEnt->r.inuse == 0)
            continue;

        pEntClient = pEnt->client;
        if (pEntClient == NULL)
            continue;

        if (pEntClient->sess.sessionState != 0)
            continue;

        if (pEntClient->sess.cs.team != team)
            continue;

        if (pEnt == pSelf)
            continue;

        if (SV_inSnapshot(vPosition, pEnt->s.number))
            continue;

        {
            int result = pEnt->s.number;
            float ex = pEnt->r.currentOrigin[0];
            float ey = pEnt->r.currentOrigin[1];

            dx = (int)(ex - vPosition[0] + 0.5f);
            dy = (int)(ey - vPosition[1] + 0.5f);

            if (dx > 0x400) {
                scaleX = 1024.0f / (float)dx;
                ratioX = 1.0f;
            } else if (dx < -1022) {
                scaleX = -1022.0f / (float)dx;
                ratioX = 1.0f;
            } else {
                scaleX = 1.0f;
                ratioX = 1.0f;
            }

            if (dy > 0x400) {
                scaleY = 1024.0f / (float)dy;
            } else if (dy < -1022) {
                scaleY = -1022.0f / (float)dy;
            } else {
                scaleY = ratioX;
            }

            if (ratioX > scaleX) {
                if (scaleX > scaleY) {

                    dx = (int)((float)dx * scaleY);
                } else {

                    dy = (int)((float)dy * scaleX);
                }
            } else if (ratioX > scaleY) {

                dx = (int)((float)dx * scaleY);
            }

            if (dx > 0x400)
                dx = 0x402;
            else if (dx < -1022)
                dx = -1020;
            else
                dx = dx + 2;

            if (dy > 0x400)
                dy = 0x402;
            else if (dy < -1022)
                dy = -1020;
            else
                dy = dy + 2;

            result &= 0xffff803f;
            {
                int scaledDx = (dx < 0 ? (dx + 3) : dx) >> 2;
                scaledDx = (scaledDx + 0xff) & 0x1ff;
                result |= (scaledDx << 6);
            }
            result &= 0xff007fff;
            {
                int scaledDy = (dy < 0 ? (dy + 3) : dy) >> 2;
                scaledDy = (scaledDy + 0xff) & 0x1ff;
                result |= (scaledDy << 15);
            }
            result &= 0xffffff;
            {
                int yawByte = (int)(pEnt->r.currentAngles[1] * 0.7111111283302307f);
                result |= (yawByte << 24);
            }

            return result;
        }
    }

    return 0;
}

void G_BroadcastVoice(gentity_t *talker, VoicePacket_t *voicePacket)
{
    gclient_s *talkerClient;
    gentity_s *otherEnt;
    gclient_s *otherClient;
    int otherPlayer;
    int talkerTeam;

    talkerClient = talker->client;

    {
        level_locals_t *lev = G_Level();
        talkerClient->lastVoiceTime = lev->time;
    }

    for (otherPlayer = 0; otherPlayer < 64; otherPlayer++) {
        otherEnt = G_EntityForNum(otherPlayer);

        if (otherEnt->r.inuse == 0)
            continue;

        otherClient = otherEnt->client;
        if (otherClient == NULL)
            continue;

        {
            if (voice_global->current.enabled)
                goto check_team;
        }

        if (!OnSameTeam(talker, (gentity_t *)otherEnt)) {
            talkerClient = talker->client;
            talkerTeam = talkerClient->sess.cs.team;
            if (talkerTeam != 0) {
                continue;
            }
        }

        talkerClient = talker->client;
    check_team:

        {
            int otherSessState = otherEnt->client->sess.sessionState;
            int talkerSessState = talkerClient->sess.sessionState;
            if (otherSessState == talkerSessState)
                goto check_can_send;

            if (otherSessState - 1 == 0 || talkerSessState - 1 == 0) {
                if (!voice_deadChat->current.enabled)
                    continue;
            }
        }

    check_can_send:

        if (otherEnt == (gentity_s *)talker) {
            if (!voice_localEcho->current.enabled)
                continue;
        }

        if (SV_ClientHasClientMuted(otherPlayer, talker->s.number))
            continue;

        if (!SV_ClientWantsVoiceData(otherPlayer))
            continue;

        SV_QueueVoicePacket(talker->s.number, otherPlayer, voicePacket);
    }
}

void SetClientViewAngle(gentity_t *ent, const vec_t *angle)
{
    vec3_t newAngle;
    gclient_s *client;
    int i;
    float delta;

    newAngle[0] = angle[0];
    newAngle[1] = angle[1];
    newAngle[2] = angle[2];

    client = ent->client;

    if ((client->ps.pm_flags & 1) != 0 && (client->ps.eFlags & 0x300) == 0) {

        delta = AngleNormalize180(AngleDelta(client->ps.proneDirection, newAngle[1]));

        if (delta > 45.0f) {
            float overflow = delta - 45.0f;
            client = ent->client;
            client->ps.delta_angles[1] += (int)(overflow * 182.04444885253906f) & 0xffff;
            if (overflow > 0.0f)
                newAngle[1] = AngleNormalize360(client->ps.proneDirection - 45.0f);
            else
                newAngle[1] = AngleNormalize360(45.0f + client->ps.proneDirection);
        } else if (delta < -45.0f) {
            float overflow = delta + 45.0f;
            client = ent->client;
            client->ps.delta_angles[1] += (int)(overflow * 182.04444885253906f) & 0xffff;
            if (overflow > 0.0f)
                newAngle[1] = AngleNormalize360(client->ps.proneDirection - 45.0f);
            else
                newAngle[1] = AngleNormalize360(45.0f + client->ps.proneDirection);
        }

        client = ent->client;
        delta = AngleNormalize180(AngleDelta(client->ps.proneTorsoPitch, newAngle[0]));

        if (delta > 45.0f) {
            float overflow = delta - 45.0f;
            client = ent->client;
            client->ps.delta_angles[0] += (int)(overflow * 182.04444885253906f) & 0xffff;
            if (overflow > 0.0f)
                newAngle[0] = AngleNormalize180(client->ps.proneTorsoPitch - 45.0f);
            else
                newAngle[0] = AngleNormalize180(15.0f + client->ps.proneTorsoPitch);
        } else if (delta < -15.0f) {
            float overflow = delta + 15.0f;
            client = ent->client;
            client->ps.delta_angles[0] += (int)(overflow * 182.04444885253906f) & 0xffff;
            if (overflow > 0.0f)
                newAngle[0] = AngleNormalize180(client->ps.proneTorsoPitch - 45.0f);
            else
                newAngle[0] = AngleNormalize180(15.0f + client->ps.proneTorsoPitch);
        }

        client = ent->client;
    }

    for (i = 0; i < 3; i++) {
        int cmdAngle = (int)(newAngle[i] * 182.04444885253906f) & 0xffff;
        client->ps.delta_angles[i] = cmdAngle - client->sess.cmd.angles[i];
        client = ent->client;
    }

    ent->r.currentAngles[0] = newAngle[0];
    ent->r.currentAngles[1] = newAngle[1];
    ent->r.currentAngles[2] = newAngle[2];

    client = ent->client;
    client->ps.viewangles[0] = newAngle[0];
    client->ps.viewangles[1] = newAngle[1];
    client->ps.viewangles[2] = newAngle[2];
}

void ClientSpawn(gentity_t *ent, const vec_t *spawn_origin, const vec_t *spawn_angles)
{
    gclient_s *client;
    level_locals_t *lev;
    int clientNum;
    int iFlags;
    int savedSpawnCount;
    byte savedSess[0x100];
    const scr_const_t *scr_data;
    vec_t *pMins;
    vec_t *pMaxs;

    clientNum = ent - (gentity_s *)imp_g_entities;

    client = ent->client;
    lev = G_Level();

    if (client->ps.pm_flags & 0x800000) {
        if (client->ps.eFlags & 0x300) {

            int turretEntNum = client->ps.viewlocked_entNum;
            gentity_s *turretEnt = G_EntityForNum(turretEntNum);
            G_ClientStopUsingTurret((gentity_t *)turretEnt);
        }
    }

    G_EntUnlink(ent);

    if (ent->r.linked != 0)
        SV_UnlinkEntity(ent);

    ent->s.groundEntityNum = 0x3ff;

    scr_data = SCR_CONST();
    Scr_SetString(&ent->classname, scr_data->player);

    ent->clipmask = 0x2810011;
    ent->r.svFlags |= 1;
    ent->takedamage = 0;

    G_SetClientContents(ent);

    ent->handler = 0xa;
    ent->flags = 0x1000;

    pMins = ent->r.mins;
    ent->r.mins[0] = playerMins[0];
    pMins[1] = playerMins[1];
    pMins[2] = playerMins[2];

    pMaxs = ent->r.maxs;
    ent->r.maxs[0] = playerMaxs[0];
    pMaxs[1] = playerMaxs[1];
    pMaxs[2] = playerMaxs[2];

    iFlags = client->ps.eFlags & 0x100002;

    memcpy(savedSess, &client->sess, sizeof(clientSession_t));
    savedSpawnCount = client->ps.stats[5];

    memset(client, 0, CLIENT_STRIDE);

    memcpy(&client->sess, savedSess, sizeof(clientSession_t));

    client->spectatorClient = -1;
    client->useHoldEntity = 0x3ff;

    savedSpawnCount++;
    client->ps.stats[5] = savedSpawnCount;

    client->ps.stats[2] = client->sess.maxHealth;
    client->ps.eFlags = iFlags;

    client->sess.cs.clientIndex = clientNum;
    client->ps.clientNum = clientNum;
    client->ps.viewlocked_entNum = 0x3ff;

    SV_GetUsercmd(client - lev->clients, &client->sess.cmd);

    client->ps.eFlags ^= 2;

    client->ps.mins[0] = ent->r.mins[0];
    client->ps.mins[1] = pMins[1];
    client->ps.mins[2] = pMins[2];

    client->ps.maxs[0] = ent->r.maxs[0];
    client->ps.maxs[1] = pMaxs[1];
    client->ps.maxs[2] = pMaxs[2];

    client->ps.viewHeightTarget = 0x3c;
    client->ps.viewHeightCurrent = 60.0f;
    client->ps.viewHeightLerpTime = 0;
    client->ps.viewHeightLerpPosAdj = 0;

    G_SetOrigin(ent, spawn_origin);

    client->ps.origin[0] = spawn_origin[0];
    client->ps.origin[1] = spawn_origin[1];
    client->ps.origin[2] = spawn_origin[2];

    client->ps.pm_flags |= 0x1000;

    SetClientViewAngle(ent, spawn_angles);

    {
        int dvarVal = (*(const dvar_t **)imp_g_inactivity)->current.integer;
        int time = dvarVal * 5 * 5 * 5;
        client->inactivityTime = lev->time + time * 8;
    }

    client->buttons = client->sess.cmd.buttons;

    {
        lev->clientIsSpawning = 1;
        client->lastSpawnTime = lev->time;
        client->sess.cmd.serverTime = lev->time;
        client->ps.commandTime = lev->time - 100;

        ClientEndFrame(ent);
        ClientThink_real(ent, &client->sess.cmd);

        lev->clientIsSpawning = 0;

        BG_PlayerStateToEntityState(&client->ps, ent, 1, 1);
    }
}

void G_GetPlayerViewOrigin(const gentity_t *ent, vec_t *origin)
{
    gclient_s *client;
    float fBobCycle, xyspeed;
    float vertBob, horzBob;
    vec3_t vRight;
    float bobMax;

    client = ent->client;

    if (client->ps.eFlags & 0x300) {

        const scr_const_t *scr_data = SCR_CONST();
        int turretEntNum = client->ps.viewlocked_entNum;
        gentity_s *turretEnt = G_EntityForNum(turretEntNum);

        if (!G_DObjGetWorldTagPos((gentity_t *)turretEnt, scr_data->tag_player, origin)) {
            Com_Error(1, "G_GetPlayerViewOrigin: couldn't find tag");
        }
        return;
    }

    origin[0] = client->ps.origin[0];
    origin[1] = client->ps.origin[1];
    origin[2] = client->ps.origin[2];

    origin[2] += client->ps.viewHeightCurrent;

    fBobCycle = BG_GetBobCycle(&client->ps);

    {
        level_locals_t *lev = G_Level();
        xyspeed = BG_GetSpeed(&client->ps, lev->time);
    }

    {
        bobMax = bg_bobMax->current.value;
        vertBob = BG_GetVerticalBobFactor(&client->ps, fBobCycle, xyspeed, bobMax);
    }
    origin[2] += vertBob;

    {
        bobMax = bg_bobMax->current.value;
        horzBob = BG_GetHorizontalBobFactor(&client->ps, fBobCycle, xyspeed, bobMax);
    }

    AngleVectors(client->ps.viewangles, NULL, vRight, NULL);

    origin[0] += horzBob * vRight[0];
    origin[1] += horzBob * vRight[1];
    origin[2] += horzBob * vRight[2];

    AddLeanToPosition(origin, client->ps.viewangles[1], client->ps.leanf, 20.0f, 16.0f);

    {
        float minZ = client->ps.origin[2] + 8.0f;
        if (minZ > origin[2])
            origin[2] = minZ;
    }
}

static void CleanName(const char *in, char *out, int outSize)
{
    int len = 0;
    int colorlessLen = 0;
    int spaces = 0;
    char *dst = out;

    *out = 0;

    while (*in) {
        if (*out == 0 && *in == ' ') {
            in++;
            continue;
        }

        if (*in == '^') {
            if (in[1] == 0)
                break;
            if (ColorIndex((unsigned char)in[1])) {
                if (len > 29)
                    break;
                *dst++ = '^';
                *dst++ = in[1];
                len += 2;
            }
            in += 2;
            continue;
        }

        if (*in == ' ') {
            spaces++;
            if (spaces > 3) {
                in++;
                continue;
            }
        } else {
            spaces = 0;
        }

        if (len > 30)
            break;

        *dst++ = *in;
        colorlessLen++;
        len++;
        in++;
    }

    *dst = 0;

    if (*out == 0 || colorlessLen == 0) {
        I_strncpyz(out, "UnnamedPlayer", outSize);
    }
}

void ClientUserinfoChanged(int clientNum)
{
    gentity_s *ent;
    gclient_s *client;
    char userinfo[0x400];
    char oldname[0x400];

    ent = G_EntityForNum(clientNum);
    client = ent->client;

    SV_GetUserinfo(clientNum, userinfo, 0x400);

    if (!Info_Validate(userinfo)) {

        userinfo[0] = '\\';
        userinfo[1] = 'n';
        userinfo[2] = 'a';
        userinfo[3] = 'm';
        userinfo[4] = 'e';
        userinfo[5] = '\\';
        userinfo[6] = 'b';
        userinfo[7] = 'a';
        userinfo[8] = 'd';
        userinfo[9] = 'i';
        userinfo[10] = 'n';
        userinfo[11] = 'f';
        userinfo[12] = 'o';
        userinfo[13] = 0;
    }

    client->sess.localClient = SV_IsLocalClient(clientNum);

    {
        int handicap = atoi(Info_ValueForKey(userinfo, "handicap"));
        client->sess.predictItemPickup = (handicap != 0) ? 1 : 0;
    }

    if (client->sess.connected == 2) {
        level_locals_t *lev = G_Level();
        if (lev->manualNameChange != 0) {
            const char *name = Info_ValueForKey(userinfo, "name");
            char *shortName = client->sess.newnetname;

            {
                int len = 0, colorlessLen = 0, spaces = 0;
                char *dst = shortName;
                const char *src = name;

                shortName[0] = 0;

                while (*src) {
                    if (*shortName == 0 && *src == ' ') {
                        src++;
                        continue;
                    }
                    if (*src == '^') {
                        if (src[1] == 0)
                            break;
                        if (ColorIndex((unsigned char)src[1])) {
                            if (len > 29)
                                break;
                            *dst++ = '^';
                            *dst++ = src[1];
                            len += 2;
                        }
                        src += 2;
                        continue;
                    }
                    if (*src == ' ') {
                        spaces++;
                        if (spaces > 3) {
                            src++;
                            continue;
                        }
                    } else {
                        spaces = 0;
                    }
                    if (len > 30)
                        break;
                    *dst++ = *src;
                    colorlessLen++;
                    len++;
                    src++;
                }
                *dst = 0;
                if (*shortName == 0 || colorlessLen == 0) {
                    I_strncpyz(shortName, "UnnamedPlayer", 0x1f);
                }
            }

            {
                char *longName = client->sess.cs.name;
                goto do_score_update;
            }
        }
    }

    {
        char *longName = client->sess.cs.name;

        I_strncpyz(oldname, longName, 0x400);

        {
            const char *name = Info_ValueForKey(userinfo, "name");
            int len = 0, colorlessLen = 0, spaces = 0;
            char *dst = longName;
            const char *src = name;

            longName[0] = 0;

            while (*src) {
                if (*longName == 0 && *src == ' ') {
                    src++;
                    continue;
                }
                if (*src == '^') {
                    if (src[1] == 0)
                        break;
                    if (ColorIndex((unsigned char)src[1])) {
                        if (len > 29)
                            break;
                        *dst++ = '^';
                        *dst++ = src[1];
                        len += 2;
                    }
                    src += 2;
                    continue;
                }
                if (*src == ' ') {
                    spaces++;
                    if (spaces > 3) {
                        src++;
                        continue;
                    }
                } else {
                    spaces = 0;
                }
                if (len > 30)
                    break;
                *dst++ = *src;
                colorlessLen++;
                len++;
                src++;
            }
            *dst = 0;
            if (*longName == 0 || colorlessLen == 0) {
                I_strncpyz(longName, "UnnamedPlayer", 0x1f);
            }
        }

        I_strncpyz(client->sess.newnetname, longName, 0x20);

    do_score_update:

        {
            clientInfo_t *ci = &level_bgs.clientinfo[clientNum];

            ci->clientNum = clientNum;
            I_strncpyz(ci->name, longName, sizeof(ci->name));
            ci->team = client->sess.cs.team;
        }
    }
}

char *ClientConnect(int clientNum, int scriptPersId)
{
    gentity_s *ent;
    gclient_s *client;
    clientInfo_t *ci;
    int pXAnimTree;
    char userinfo[0x400];

    ent = G_EntityForNum(clientNum);
    client = G_ClientForNum(clientNum);

    memset(client, 0, CLIENT_STRIDE);

    ci = &level_bgs.clientinfo[clientNum];

    pXAnimTree = (int)ci->pXAnimTree;
    memset(ci, 0, sizeof(*ci));
    ci->pXAnimTree = (XAnimTree_s *)pXAnimTree;

    ci->infoValid = 1;
    ci->nextValid = 1;

    client->sess.connected = 1;
    client->sess.scriptPersId = (unsigned short)scriptPersId;

    client->sess.cs.team = 3;
    client->sess.sessionState = 2;
    client->spectatorClient = -1;
    client->sess.forceSpectatorClient = -1;

    G_InitGentity((gentity_t *)ent);
    ent->handler = 0;
    ent->client = (gclient_t *)client;
    client->useHoldEntity = 0x3ff;
    client->sess.cs.clientIndex = clientNum;
    client->ps.clientNum = clientNum;

    ClientUserinfoChanged(clientNum);

    SV_GetUserinfo(clientNum, userinfo, 0x400);

    if (client->sess.localClient == 0) {
        const char *password = Info_ValueForKey(userinfo, "password");
        const char *serverPassword = g_password ? g_password->current.string : "";

        if (*serverPassword != 0) {
            if (I_stricmp(serverPassword, "")) {
                if (strcmp(serverPassword, password)) {
                    G_FreeEntity((gentity_t *)ent);
                    return "GAME_INVALIDPASSWORD";
                }
            }
        }
    }

    Scr_PlayerConnect((gentity_t *)ent);
    CalculateRanks();

    return NULL;
}
