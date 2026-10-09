#include "common_types.h"
#include "imports.h"
#include "bytematch.h"

extern void Com_Error(int code, const char *fmt, ...);

COD2_ASSERT_FIELD(gclient_t, sess.sessionState, 0x26a8);

static char line[1024];
/* gc_orders: migrated from the ILP32 data blob to typed C (re-lays-out per
 * target; trailing blob bytes were inter-symbol padding). (x64 port Stage 2.) */
char *gc_orders[7] = {
    "GAME_GC_HOLDYOURPOSITION", "GAME_GC_HOLDTHISPOSITION", "GAME_GC_COMEHERE", "GAME_GC_COVERME",
    "GAME_GC_GUARDLOCATION", "GAME_GC_SEARCHDESTROY", "GAME_GC_REPORT",
};
extern level_locals_t level;

#define g_entities ((gentity_t *)imp_g_entities)
extern const dvar_t *g_cheats;
extern const dvar_t *g_allowVote;
extern const dvar_t *g_oldVoting;
extern const dvar_t *g_deadChat;
extern const dvar_t *g_dedicated;
extern const dvar_t *g_gametype;
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern char *va(const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);
extern void G_LogPrintf(const char *fmt, ...);
extern int SV_GetClientPing(int clientNum);
extern int SV_GetGuid(int clientNum);
extern void SV_GameSendServerCommand(int clientNum, int type, const char *text);
extern void SV_SetConfigstring(int index, const char *val);
extern int SV_Cmd_Argc(void);
extern void SV_Cmd_ArgvBuffer(int arg, char *buffer, int bufferLength);
extern int I_stricmp(const char *s1, const char *s2);
extern int I_strnicmp(const char *s1, const char *s2, size_t n);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern char *I_CleanStr(char *string);
extern qboolean OnSameTeam(gentity_t *ent1, gentity_t *ent2);
extern char *strchr(const char *s, int c);
extern int Dvar_GetInt(const char *dvarName);
extern const dvar_t *Dvar_RegisterString_mac(const char *dvarName, const char *value, int flags);
extern void Cbuf_ExecuteText(int exec_when, const char *text);
extern void SV_GetConfigstring(int index, char *buffer, int bufferLength);
extern qboolean SV_MapExists(const char *name);
extern void Scr_AddString(const char *value);
extern void Scr_Notify(gentity_t *ent, unsigned short stringValue, int paramcount);
extern void Scr_VoteCalled(gentity_t *self, char *command, char *param1, char *param2);
extern qboolean Scr_IsValidGameType(const char *pszGameType);
extern const char *Scr_GetGameTypeNameForScript(const char *pszGameTypeScript);
extern void G_GetPlayerViewOrigin(const gentity_t *ent, vec_t *origin);
extern void G_GetPlayerViewDirection(const gentity_t *ent, vec_t *forward, vec_t *right, vec_t *up);
extern void G_TraceCapsule(trace_t *results, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int passEntityNum, int contentmask);
extern unsigned char G_SetOrigin(gentity_t *ent, const vec_t *origin);
extern void SetClientViewAngle(gentity_t *ent, const vec_t *angle);
extern void TeleportPlayer(gentity_t *player, vec_t *origin, vec_t *angles);
extern int BG_GetNumWeapons(void);
extern qboolean BG_DoesWeaponRequireSlot(int weaponIndex);
extern qboolean BG_IsAnyEmptyPrimaryWeaponSlot(const playerState_t *ps);
extern qboolean BG_TakePlayerWeapon(playerState_t *pPS, int iWeaponIndex);
extern int BG_AmmoForWeapon(int weapon);
extern int BG_ClipForWeapon(int weapon);
extern qboolean G_GivePlayerWeapon(playerState_t *pPS, int iWeaponIndex);
extern int Add_Ammo(gentity_t *ent, int weapon, int count, qboolean fillClip);
extern const gitem_t *G_FindItem(const char *pickupName);
extern gentity_t *G_Spawn(void);
extern void G_GetItemClassname(const gitem_t *item, scr_string_t *out);
extern void G_SpawnItem(gentity_t *ent, const gitem_t *item);
extern void Touch_Item(gentity_t *ent, gentity_t *other, qboolean bTouched);
extern unsigned char G_FreeEntity(gentity_t *ent);
extern void G_SelectWeaponIndex(int clientNum, int iWeaponIndex);
extern qboolean G_ClientCanSpectateTeam(gclient_t *client, team_t team);
extern qboolean SV_GetArchivedClientInfo(int clientNum, int *pArchiveTime, playerState_t *ps, clientState_t *cs);
extern void player_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath, int iWeapon, const vec_t *vDir, const hitLocation_t hitLoc, int psTimeOffset);
extern char *vtos(const vec_t *v);
extern unsigned char G_PrintEntities(void);
extern double atof(const char *nptr);
extern int atoi(const char *nptr);
extern void Scr_PlayerVote(gentity_t *self, char *option);

enum {
    GCMDS_MAX_CLIENTS = 64
};

void DeathmatchScoreboardMessage(gentity_t *ent);
void Cmd_Score_f(gentity_t *ent);
qboolean CheatsOk(gentity_t *ent);
char *ConcatArgs(int start);
void G_setfog(const char *fogstring);
qboolean Cmd_FollowCycle_f(gentity_t *ent, int dir);
qboolean G_IsPlaying(gentity_t *ent);
void Cmd_CallVote_f(gentity_t *ent);
void Cmd_Vote_f(gentity_t *ent);
void Cmd_SetViewpos_f(gentity_t *ent);
void Cmd_MenuResponse_f(gentity_t *pEnt);
static void G_SayTo(gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message);
void G_Say(gentity_t *ent, gentity_t *target, int mode, const char *chatText);
void Cmd_GameCommand_f(gentity_t *ent);
void StopFollowing(gentity_t *ent);
void Cmd_Give_f(gentity_t *ent);
void Cmd_Take_f(gentity_t *ent);
void ClientCommand(int clientNum);

void DeathmatchScoreboardMessage(gentity_t *ent)
{
    level_locals_t *lvl = &level;
    gclient_t *client;
    int clientNum;
    char entry[1024];
    int i;
    int j;
    int numSorted;
    int ping;
    char string[1400];
    int stringlength;

    string[0] = '\0';
    stringlength = 0;

    numSorted = lvl->numConnectedClients;
    if (numSorted > GCMDS_MAX_CLIENTS) {
        numSorted = GCMDS_MAX_CLIENTS;
    }

    for (i = 0; i < numSorted; ++i) {
        clientNum = lvl->sortedClients[i];
        client = &lvl->clients[clientNum];

        if (client->sess.connected == CON_CONNECTING) {
            Com_sprintf(entry, sizeof(entry), " %i %i %i %i %i",
                        lvl->sortedClients[i], client->sess.score, -1,
                        client->sess.deaths, client->sess.status_icon);
        } else {
            ping = SV_GetClientPing(clientNum);
            Com_sprintf(entry, sizeof(entry), " %i %i %i %i %i",
                        lvl->sortedClients[i], client->sess.score, ping,
                        client->sess.deaths, client->sess.status_icon);
        }

        j = strlen(entry);
        if (stringlength + j > 1024) {
            break;
        }

        strcpy(string + stringlength, entry);
        stringlength += j;
    }

    SV_GameSendServerCommand(ent - g_entities, SV_CMD_RELIABLE,
                             va("%c %i %i %i%s", 98, i, lvl->teamScores[TEAM_AXIS],
                                lvl->teamScores[TEAM_ALLIES], string));
}

void Cmd_Score_f(gentity_t *ent)
{
    DeathmatchScoreboardMessage(ent);
}

qboolean CheatsOk(gentity_t *ent)
{
    if (!(*(const dvar_t **)imp_g_cheats)->current.enabled) {
        SV_GameSendServerCommand(ent - g_entities, SV_CMD_CAN_IGNORE,
                                 va("%c \"GAME_CHEATSNOTENABLED\"", 101));
        return 0;
    }

    {
        int health = ent->health;
        if (health <= 0) {
            SV_GameSendServerCommand(ent - g_entities, SV_CMD_CAN_IGNORE,
                                     va("%c \"GAME_MUSTBEALIVECOMMAND\"", 101));
            return 0;
        }
    }

    return 1;
}

static inline __attribute__((always_inline)) char *ConcatArgs_impl(int start)
{
    char arg[1024];
    int argc;
    int i;
    int len;
    int tlen;

    argc = SV_Cmd_Argc();
    len = 0;

    for (i = start; i < argc; ++i) {
        SV_Cmd_ArgvBuffer(i, arg, sizeof(arg));
        tlen = strlen(arg);

        if (len + tlen >= (int)sizeof(line) - 1) {
            break;
        }

        memcpy(line + len, arg, tlen);
        len += tlen;

        if (i != argc - 1) {
            line[len] = ' ';
            ++len;
        }
    }

    line[len] = '\0';
    return line;
}

char *ConcatArgs(int start)
{
    char arg[1024];
    int argc;
    int i;
    int len;
    int tlen;

    argc = SV_Cmd_Argc();
    len = 0;

    for (i = start; i < argc; ++i) {
        SV_Cmd_ArgvBuffer(i, arg, sizeof(arg));
        tlen = strlen(arg);

        if (len + tlen >= (int)sizeof(line) - 1) {
            break;
        }

        memcpy(line + len, arg, tlen);
        len += tlen;

        if (i != argc - 1) {
            line[len] = ' ';
            ++len;
        }
    }

    line[len] = '\0';
    return line;
}

void G_setfog(const char *fogstring)
{
    level_locals_t *lvl = &level;
    float fTime;
    float r;
    float g;
    float b;
    float fDensity;
    float fFar;
    float fNear;

    SV_SetConfigstring(12, fogstring);

    lvl->fFogOpaqueDist = 3.402823466e+38f;
    lvl->fFogOpaqueDistSqrd = 3.402823466e+38f;

    if (sscanf(fogstring, "%f %f %f %f %f %f %f", &fNear, &fFar, &fDensity, &r, &g, &b, &fTime) != 7) {
        return;
    }

    if (fDensity < 1.0f) {
        return;
    }

    lvl->fFogOpaqueDist = fFar;
    lvl->fFogOpaqueDistSqrd = fFar * fFar;
}

qboolean Cmd_FollowCycle_f(gentity_t *ent, int dir)
{
    int clientnum;
    int original;
    gclient_t *client;
    playerState_t ps;
    clientState_t cs;
    int maxclients;

    if (dir != 1 && dir != -1) {
        Com_Error(1, "\x15"
                     "Cmd_FollowCycle_f: bad dir %i",
                  dir);
    }

    client = ent->client;

    if (((gclient_t *)client)->sess.sessionState != 2)
        return 0;

    if (((gclient_t *)client)->sess.forceSpectatorClient >= 0)
        return 0;

    clientnum = ((gclient_t *)client)->spectatorClient;
    if (clientnum < 0)
        clientnum = 0;

    original = clientnum;

    for (;;) {
        clientnum += dir;

        maxclients = level.maxclients;
        if (clientnum >= maxclients) {
            clientnum = 0;
        }
        if (clientnum == -1) {
            clientnum = maxclients - 1;
        }

        client = ent->client;
        if (SV_GetArchivedClientInfo(clientnum, &client->sess.archiveTime, &ps, &cs)) {

            if (G_ClientCanSpectateTeam(ent->client, cs.team)) {

                ((gclient_t *)client)->spectatorClient = clientnum;
                ((gclient_t *)client)->sess.sessionState = 2;
                return 1;
            }
        }

        if (clientnum == original)
            return 0;
    }
}

qboolean G_IsPlaying(gentity_t *ent)
{

    int state;
    state = ent->client->sess.sessionState;
    return state == SESS_STATE_PLAYING;
}

void Cmd_CallVote_f(gentity_t *ent)
{
    level_locals_t *lvl = &level;
    char cleanName[64];
    char arg1[256];
    char arg2[256];
    char arg3[256];
    const dvar_t *mapname;
    const char *kickCommand;
    int kicknum;
    int i;

    if (!(*(const dvar_t **)imp_g_allowVote)->current.enabled) {
        SV_GameSendServerCommand(ent - g_entities, SV_CMD_CAN_IGNORE,
                                 va("%c \"GAME_VOTINGNOTENABLED\"", 101));
        return;
    }

    if ((*(const dvar_t **)imp_g_oldVoting)->current.enabled) {
        if (lvl->voteTime) {
            SV_GameSendServerCommand(ent - g_entities, SV_CMD_CAN_IGNORE,
                                     va("%c \"GAME_VOTEALREADYINPROGRESS\"", 101));
            return;
        }

        if (ent->client->sess.voteCount > 2) {
            SV_GameSendServerCommand(ent - g_entities, SV_CMD_CAN_IGNORE,
                                     va("%c \"GAME_MAXVOTESCALLED\"", 101));
            return;
        }

        if (ent->client->sess.cs.team == TEAM_SPECTATOR) {
            SV_GameSendServerCommand(ent - g_entities, SV_CMD_CAN_IGNORE,
                                     va("%c \"GAME_NOSPECTATORCALLVOTE\"", 101));
            return;
        }
    }

    SV_Cmd_ArgvBuffer(1, arg1, sizeof(arg1));
    SV_Cmd_ArgvBuffer(2, arg2, sizeof(arg2));
    SV_Cmd_ArgvBuffer(3, arg3, sizeof(arg3));

    if (strchr(arg1, ';') || strchr(arg2, ';') || strchr(arg3, ';')) {
        SV_GameSendServerCommand(ent - g_entities, SV_CMD_CAN_IGNORE,
                                 va("%c \"GAME_INVALIDVOTESTRING\"", 101));
        return;
    }

    if (!(*(const dvar_t **)imp_g_oldVoting)->current.enabled) {
        Scr_VoteCalled(ent, arg1, arg2, arg3);
        return;
    }

    if (I_stricmp(arg1, "map_restart") && I_stricmp(arg1, "map_rotate") && I_stricmp(arg1, "typemap") && I_stricmp(arg1, "map") && I_stricmp(arg1, "g_gametype") && I_stricmp(arg1, "kick") && I_stricmp(arg1, "clientkick") && I_stricmp(arg1, "tempBanUser") && I_stricmp(arg1, "tempBanClient")) {
        SV_GameSendServerCommand(ent - g_entities, SV_CMD_CAN_IGNORE,
                                 va("%c \"GAME_INVALIDVOTESTRING\"", 101));
        SV_GameSendServerCommand(ent - g_entities, SV_CMD_CAN_IGNORE,
                                 va("%c \"GAME_VOTECOMMANDSARE\x15 map_restart, map_rotate, map <mapname>, g_gametype <typename>, typemap <typename> <mapname>, kick <player>, clientkick <clientnum>, tempBanUser <player>, tempBanClient <clientNum>\"", 101));
        return;
    }

    if (lvl->voteExecuteTime) {
        lvl->voteExecuteTime = 0;
        Cbuf_ExecuteText(2, va("%s\n", lvl->voteString));
    }

    if (!I_stricmp(arg1, "typemap")) {
        if (!Scr_IsValidGameType(arg2)) {
            SV_GameSendServerCommand(ent - g_entities, SV_CMD_CAN_IGNORE,
                                     va("%c \"GAME_INVALIDGAMETYPE\"", 101));
            return;
        }

        if (!I_stricmp(arg2, (*(const dvar_t **)imp_g_gametype)->current.string))
            arg2[0] = '\0';

        SV_Cmd_ArgvBuffer(3, arg3, sizeof(arg3));
        if (!SV_MapExists(arg3)) {
            SV_GameSendServerCommand(ent - g_entities, SV_CMD_CAN_IGNORE,
                                     va("%c \"\x15the server doesn't have that map\"", 101));
            return;
        }

        mapname = Dvar_RegisterString_mac("mapname", "", 0x1044);
        if (!I_stricmp(arg3, mapname->current.string))
            arg3[0] = '\0';

        if (arg2[0]) {
            if (arg3[0]) {
                Com_sprintf(lvl->voteString, sizeof(lvl->voteString), "g_gametype %s; map %s", arg2, arg3);
                Com_sprintf(lvl->voteDisplayString, sizeof(lvl->voteDisplayString),
                            "GAME_VOTE_GAMETYPE\x14%s\x15 - \x14GAME_VOTE_MAP\x15%s",
                            Scr_GetGameTypeNameForScript(arg2), arg3);
            } else {
                Com_sprintf(lvl->voteString, sizeof(lvl->voteString), "g_gametype %s; map_restart", arg2);
                Com_sprintf(lvl->voteDisplayString, sizeof(lvl->voteDisplayString),
                            "GAME_VOTE_GAMETYPE\x14%s", Scr_GetGameTypeNameForScript(arg2));
            }
        } else if (arg3[0]) {
            Com_sprintf(lvl->voteString, sizeof(lvl->voteString), "map %s", arg3);
            Com_sprintf(lvl->voteDisplayString, sizeof(lvl->voteDisplayString), "GAME_VOTE_MAP\x15%s", arg3);
        } else {
            SV_GameSendServerCommand(ent - g_entities, SV_CMD_CAN_IGNORE,
                                     va("%c \"GAME_TYPEMAP_NOCHANGE\"", 101));
            return;
        }
    } else if (!I_stricmp(arg1, "g_gametype")) {
        if (!Scr_IsValidGameType(arg2)) {
            SV_GameSendServerCommand(ent - g_entities, SV_CMD_CAN_IGNORE,
                                     va("%c \"GAME_INVALIDGAMETYPE\"", 101));
            return;
        }

        Com_sprintf(lvl->voteString, sizeof(lvl->voteString), "%s %s; map_restart", arg1, arg2);
        Com_sprintf(lvl->voteDisplayString, sizeof(lvl->voteDisplayString),
                    "GAME_VOTE_GAMETYPE\x14%s", Scr_GetGameTypeNameForScript(arg2));
    } else if (!I_stricmp(arg1, "map_restart")) {
        Com_sprintf(lvl->voteString, sizeof(lvl->voteString), "%s", arg1);
        Com_sprintf(lvl->voteDisplayString, sizeof(lvl->voteDisplayString), "GAME_VOTE_MAPRESTART");
    } else if (!I_stricmp(arg1, "map_rotate")) {
        Com_sprintf(lvl->voteString, sizeof(lvl->voteString), "%s", arg1);
        Com_sprintf(lvl->voteDisplayString, sizeof(lvl->voteDisplayString), "GAME_VOTE_NEXTMAP");
    } else if (!I_stricmp(arg1, "map")) {
        if (!SV_MapExists(arg2)) {
            SV_GameSendServerCommand(ent - g_entities, SV_CMD_CAN_IGNORE,
                                     va("%c \"\x15the server doesn't have that map\"", 101));
            return;
        }

        Com_sprintf(lvl->voteString, sizeof(lvl->voteString), "%s %s", arg1, arg2);
        Com_sprintf(lvl->voteDisplayString, sizeof(lvl->voteDisplayString), "GAME_VOTE_MAP\x15%s", arg2);
    } else if (!I_stricmp(arg1, "kick") || !I_stricmp(arg1, "clientkick") || !I_stricmp(arg1, "tempBanUser") || !I_stricmp(arg1, "tempBanClient")) {
        if (!I_stricmp(arg1, "kick") || !I_stricmp(arg1, "tempBanUser")) {
            kicknum = 64;
            for (i = 0; i < 64; ++i) {
                if (lvl->clients[i].sess.connected == CON_CONNECTED) {
                    I_strncpyz(cleanName, lvl->clients[i].sess.cs.name, sizeof(cleanName));
                    I_CleanStr(cleanName);
                    if (!I_stricmp(cleanName, arg2))
                        kicknum = i;
                }
            }

            if (kicknum == 64) {
                SV_GameSendServerCommand(ent - g_entities, SV_CMD_CAN_IGNORE,
                                         va("%c \"GAME_CLIENTNOTONSERVER\"", 101));
                return;
            }
        } else {
            kicknum = atoi(arg2);
            if (kicknum == 0 && I_stricmp(arg2, "0")) {
                SV_GameSendServerCommand(ent - g_entities, SV_CMD_CAN_IGNORE,
                                         va("%c \"GAME_CLIENTNOTONSERVER\"", 101));
                return;
            }

            if (kicknum < 0 || kicknum > 63 || lvl->clients[kicknum].sess.connected != CON_CONNECTED) {
                SV_GameSendServerCommand(ent - g_entities, SV_CMD_CAN_IGNORE,
                                         va("%c \"GAME_CLIENTNOTONSERVER\"", 101));
                return;
            }

            I_strncpyz(cleanName, lvl->clients[kicknum].sess.cs.name, sizeof(cleanName));
            I_CleanStr(cleanName);
        }

        kickCommand = (arg1[0] == 't' || arg1[0] == 'T') ? "tempBanClient" : "clientkick";
        Com_sprintf(lvl->voteString, sizeof(lvl->voteString), "%s \"%d\"", kickCommand, kicknum);
        Com_sprintf(lvl->voteDisplayString, sizeof(lvl->voteDisplayString),
                    "GAME_VOTE_KICK\x15(%i)%s", kicknum, lvl->clients[kicknum].sess.cs.name);
    } else {
        return;
    }

    SV_GameSendServerCommand(-1, SV_CMD_CAN_IGNORE,
                             va("%c \"GAME_CALLEDAVOTE\x15%s\"", 101, ent->client->sess.cs.name));
    Com_Printf("[vote] called by client %i: %s; eligible=%i\n",
               (int)(ent - g_entities), lvl->voteString, lvl->numVotingClients);
    lvl->voteTime = lvl->time + 30000;
    lvl->voteYes = 1;
    lvl->voteNo = 0;

    for (i = 0; i < lvl->maxclients; ++i)
        lvl->clients[i].ps.eFlags &= ~0x100000;

    ent->client->ps.eFlags |= 0x100000;
    SV_SetConfigstring(0xf, va("%i", lvl->voteTime));
    SV_SetConfigstring(0x10, lvl->voteDisplayString);
    SV_SetConfigstring(0x11, va("%i", lvl->voteYes));
    SV_SetConfigstring(0x12, va("%i", lvl->voteNo));
}

void Cmd_Vote_f(gentity_t *ent)
{
    level_locals_t *lvl = &level;
    char msg[64];

    if ((*(const dvar_t **)imp_g_oldVoting)->current.enabled) {
        if (!lvl->voteTime) {
            SV_GameSendServerCommand(ent - g_entities, SV_CMD_CAN_IGNORE,
                                     va("%c \"GAME_NOVOTEINPROGRESS\"", 101));
            return;
        }

        if (ent->client->ps.eFlags & 0x100000) {
            SV_GameSendServerCommand(ent - g_entities, SV_CMD_CAN_IGNORE,
                                     va("%c \"GAME_VOTEALREADYCAST\"", 101));
            return;
        }

        if (ent->client->sess.cs.team == TEAM_SPECTATOR) {
            SV_GameSendServerCommand(ent - g_entities, SV_CMD_CAN_IGNORE,
                                     va("%c \"GAME_NOSPECTATORVOTE\"", 101));
            return;
        }

        SV_GameSendServerCommand(ent - g_entities, SV_CMD_CAN_IGNORE,
                                 va("%c \"GAME_VOTECAST\"", 101));
        ent->client->ps.eFlags |= 0x100000;
    }

    SV_Cmd_ArgvBuffer(1, msg, sizeof(msg));
    if (msg[0] == 'y' || msg[0] == 'Y' || msg[0] == '1') {
        if ((*(const dvar_t **)imp_g_oldVoting)->current.enabled) {
            ++lvl->voteYes;
            SV_SetConfigstring(0x11, va("%i", lvl->voteYes));
        } else {
            Scr_PlayerVote(ent, "yes");
        }
    } else if ((*(const dvar_t **)imp_g_oldVoting)->current.enabled) {
        ++lvl->voteNo;
        SV_SetConfigstring(0x12, va("%i", lvl->voteNo));
    } else {
        Scr_PlayerVote(ent, "no");
    }
}

void Cmd_SetViewpos_f(gentity_t *ent)
{
    vec3_t origin;
    vec3_t angles = { 0.0f, 0.0f, 0.0f };
    char buffer[1024];
    int i;

    if (!(*(const dvar_t **)imp_g_cheats)->current.enabled) {
        SV_GameSendServerCommand(ent - g_entities, 0, va("%c \"GAME_CHEATSNOTENABLED\"", 101));
        return;
    }

    if (SV_Cmd_Argc() != 5) {
        SV_GameSendServerCommand(ent - g_entities, 0, va("%c \"GAME_USAGE\x15: setviewpos x y z yaw\"", 101));
        return;
    }

    for (i = 0; i < 3; ++i) {
        SV_Cmd_ArgvBuffer(i + 1, buffer, sizeof(buffer));
        origin[i] = (float)atof(buffer);
    }

    SV_Cmd_ArgvBuffer(4, buffer, sizeof(buffer));
    angles[1] = (float)atof(buffer);
    origin[2] -= ent->client->ps.viewHeightCurrent;
    TeleportPlayer(ent, origin, angles);
}

void Cmd_MenuResponse_f(gentity_t *pEnt)
{
    char szServerId[1024];
    char szMenuName[1024];
    char szResponse[1024];

    if (SV_Cmd_Argc() == 4) {
        int menuIndex;
        int serverId;

        SV_Cmd_ArgvBuffer(1, szServerId, sizeof(szServerId));
        serverId = atoi(szServerId);
        if (serverId != Dvar_GetInt("sv_serverId")) {
            return;
        }

        SV_Cmd_ArgvBuffer(2, szMenuName, sizeof(szMenuName));
        menuIndex = atoi(szMenuName);
        if ((unsigned int)menuIndex <= 0x1f) {
            SV_GetConfigstring(menuIndex + 0x4de, szMenuName, sizeof(szMenuName));
        }

        SV_Cmd_ArgvBuffer(3, szResponse, sizeof(szResponse));
    } else {
        szMenuName[0] = '\0';
        strcpy(szResponse, "bad");
    }

    if (getenv("MTRACE"))
        Com_Printf("[menu-trace] server response menu='%s' value='%s'\n", szMenuName, szResponse);
    Scr_AddString(szResponse);
    Scr_AddString(szMenuName);
    Scr_Notify(pEnt, ((const scr_const_t *)imp_scr_const)->menuresponse, 2);
}

static void G_SayTo(gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message)
{
    int command;

    if (!other)
        return;

    if (!other->r.inuse)
        return;

    if (!other->client)
        return;

    if (other->client->sess.connected != CON_CONNECTED)
        return;

    if (mode == 1 && !OnSameTeam(ent, other))
        return;

    if (!(*(const dvar_t **)imp_g_deadChat)->current.enabled && ent->client->sess.sessionState != SESS_STATE_PLAYING && other->client->sess.sessionState == SESS_STATE_PLAYING) {
        return;
    }

    command = (mode == 1) ? 'i' : 'h';
    SV_GameSendServerCommand(other - g_entities, SV_CMD_CAN_IGNORE,
                             va("%c \"\x15%s%c%c%s\"", command, name, '^', color, message));
}

void G_Say(gentity_t *ent, gentity_t *target, int mode, const char *chatText)
{
    char cleanname[64];
    char szStateString[64];
    char name[128];
    char text[150];
    const char *stateColor;
    const char *teamString;
    int clientNum;
    int color;
    int j;

    if (mode == 1 && ent->client->sess.cs.team != TEAM_AXIS && ent->client->sess.cs.team != TEAM_ALLIES)
        mode = 0;

    I_strncpyz(cleanname, ent->client->sess.cs.name, sizeof(cleanname));
    I_CleanStr(cleanname);

    switch (ent->client->sess.cs.team) {
    case TEAM_AXIS:
        stateColor = "^9";
        break;
    case TEAM_ALLIES:
        stateColor = "^8";
        break;
    case TEAM_SPECTATOR:
        Com_sprintf(szStateString, sizeof(szStateString), "\x15(\x14GAME_SPECTATOR\x15)");
        goto state_ready;
    default:
        stateColor = "";
        break;
    }

    if (ent->client->sess.sessionState) {
        Com_sprintf(szStateString, sizeof(szStateString), "\x15%s(\x14GAME_DEAD\x15)", stateColor);
    } else {
        Com_sprintf(szStateString, sizeof(szStateString), "\x15%s", stateColor);
    }

state_ready:
    if (mode == 1) {
        teamString = (ent->client->sess.cs.team == TEAM_AXIS) ? "GAME_AXIS" : "GAME_ALLIES";
        clientNum = ent->s.number;
        G_LogPrintf("sayteam;%d;%d;%s;%s\n", SV_GetGuid(clientNum), clientNum, cleanname, chatText);
        Com_sprintf(name, sizeof(name), "%s(\x14%s\x15)%s%s: ", szStateString, teamString, cleanname, "^7");
        color = '5';
    } else if (mode == 2) {
        Com_sprintf(name, sizeof(name), "%s[%s]%s: ", szStateString, cleanname, "^7");
        color = '3';
    } else {
        clientNum = ent->s.number;
        G_LogPrintf("say;%d;%d;%s;%s\n", SV_GetGuid(clientNum), clientNum, cleanname, chatText);
        Com_sprintf(name, sizeof(name), "%s%s%s: ", szStateString, cleanname, "^7");
        color = '7';
    }

    I_strncpyz(text, chatText, sizeof(text));

    if (target) {
        G_SayTo(ent, target, mode, color, name, text);
        return;
    }

    if ((*(const dvar_t **)imp_g_dedicated)->current.integer)
        Com_Printf("%s%s\n", name, text);

    for (j = 0; j < level.numConnectedClients; ++j)
        G_SayTo(ent, &g_entities[j], mode, color, name, text);
}

void Cmd_GameCommand_f(gentity_t *ent)
{
    char str[1024];
    int player;
    int order;

    SV_Cmd_ArgvBuffer(1, str, sizeof(str));
    player = atoi(str);
    SV_Cmd_ArgvBuffer(2, str, sizeof(str));
    order = atoi(str);

    if ((unsigned int)player > 63 || (unsigned int)order > 6)
        return;

    G_Say(ent, &((gentity_t *)imp_g_entities)[player], 2, gc_orders[order]);
    G_Say(ent, ent, 2, gc_orders[order]);
}

void StopFollowing(gentity_t *ent)
{
    gclient_t *client = ent->client;
    vec3_t vAngles;
    vec3_t vPos;
    vec3_t vForward;
    vec3_t vUp;
    vec3_t vEnd;
    vec3_t vMins;
    vec3_t vMaxs;
    trace_t trace;
    float fraction;

    client->sess.forceSpectatorClient = -1;
    client->spectatorClient = -1;

    if (!(client->ps.pm_flags & 0x00400000))
        return;

    G_GetPlayerViewOrigin(ent, vPos);
    G_GetPlayerViewDirection(ent, vForward, NULL, vUp);

    {
        const float *va = client->ps.viewangles;
        vAngles[0] = va[0];
        vAngles[1] = va[1];
        vAngles[2] = va[2];
        vAngles[0] += 15.0f;
    }

    vEnd[0] = -40.0f * vForward[0] + vPos[0];
    vEnd[1] = -40.0f * vForward[1] + vPos[1];
    vEnd[2] = -40.0f * vForward[2] + vPos[2];
    vEnd[0] = 10.0f * vUp[0] + vEnd[0];
    vEnd[1] = 10.0f * vUp[1] + vEnd[1];
    vEnd[2] = 10.0f * vUp[2] + vEnd[2];

    vMins[0] = -8.0f;
    vMins[1] = -8.0f;
    vMins[2] = -8.0f;
    vMaxs[0] = 8.0f;
    vMaxs[1] = 8.0f;
    vMaxs[2] = 8.0f;

    G_TraceCapsule(&trace, vPos, vMins, vMaxs, vEnd, 0x3ff, 0x00810011);
    fraction = trace.fraction;
    vPos[0] += (vEnd[0] - vPos[0]) * fraction;
    vPos[1] += (vEnd[1] - vPos[1]) * fraction;
    vPos[2] += (vEnd[2] - vPos[2]) * fraction;

    client->ps.clientNum = (int)(ent - g_entities);
    client->ps.eFlags &= ~0x300;
    client->ps.viewlocked = 0;
    client->ps.viewlocked_entNum = 0x3ff;
    client->ps.pm_flags &= ~0x00400040;
    *(int *)((char *)&client->ps + 0xdc) = 0;

    G_SetOrigin(ent, vPos);
    {
        float *origin = client->ps.origin;
        origin[0] = vPos[0];
        origin[1] = vPos[1];
        origin[2] = vPos[2];
    }

    SetClientViewAngle(ent, vAngles);
    client->ps.shellshockIndex = 0;
    client->ps.shellshockTime = 0;
    client->ps.shellshockDuration = 0;
}

static void G_TakeAmmoForWeapon(gentity_t *ent, int weapon, int amount)
{
    playerState_t *ps = &ent->client->ps;
    int ammoIndex;
    int clipIndex;

    ammoIndex = BG_AmmoForWeapon(weapon);
    ps->ammo[ammoIndex] -= amount;
    if (ps->ammo[ammoIndex] >= 0)
        return;

    clipIndex = BG_ClipForWeapon(weapon);
    ps->ammoclip[clipIndex] += ps->ammo[ammoIndex];
    ps->ammo[ammoIndex] = 0;

    if (ps->ammoclip[clipIndex] < 0)
        ps->ammoclip[clipIndex] = 0;
}

static void G_ClearAmmoForWeapon(gentity_t *ent, int weapon)
{
    playerState_t *ps = &ent->client->ps;

    ps->ammo[BG_AmmoForWeapon(weapon)] = 0;
    ps->ammoclip[BG_ClipForWeapon(weapon)] = 0;
}

void Cmd_Give_f(gentity_t *ent)
{
    const gitem_t *item;
    gentity_t *spawned;
    playerState_t *ps;
    int maxCurrentWeapon;
    int amount;
    int give_all;
    int i;

    if (!CheatsOk(ent))
        return;

    amount = atoi(ConcatArgs_impl(2));
    ConcatArgs_impl(1);

    if (!line[0])
        return;

    give_all = !I_stricmp(line, "all");

    if (give_all || !I_strnicmp(line, "health", 6)) {
        if (amount) {
            ent->health += amount;
        } else {
            ent->health = ent->client->ps.stats[0];
        }

        if (!give_all)
            return;
    }

    if (give_all || !I_stricmp(line, "weapons")) {
        ps = &ent->client->ps;
        level.initializing = 1;

        maxCurrentWeapon = (signed char)ps->weaponslots[2];
        BG_TakePlayerWeapon(ps, (signed char)ps->weaponslots[1]);
        BG_TakePlayerWeapon(ps, (signed char)ps->weaponslots[2]);

        for (i = 1; i <= BG_GetNumWeapons(); ++i) {
            if (!BG_DoesWeaponRequireSlot(i) || i <= maxCurrentWeapon || BG_IsAnyEmptyPrimaryWeaponSlot(ps)) {
                G_GivePlayerWeapon(ps, i);
            }
        }

        for (i = 1; BG_IsAnyEmptyPrimaryWeaponSlot(ps) && i <= BG_GetNumWeapons(); ++i) {
            if (BG_DoesWeaponRequireSlot(i))
                G_GivePlayerWeapon(ps, i);
        }

        level.initializing = 0;

        if (!give_all)
            return;
    }

    if (give_all || !I_strnicmp(line, "ammo", 4)) {
        if (amount) {
            if (ent->client->ps.weapon)
                Add_Ammo(ent, ent->client->ps.weapon, amount, 1);
        } else {
            for (i = 1; i <= BG_GetNumWeapons(); ++i)
                Add_Ammo(ent, i, 0x3e6, 1);
        }

        if (!give_all)
            return;
    }

    if (!I_strnicmp(line, "allammo", 7) && amount) {
        for (i = 1; i <= BG_GetNumWeapons(); ++i)
            Add_Ammo(ent, i, amount, 1);
        return;
    }

    if (give_all)
        return;

    item = G_FindItem(line);
    if (!item)
        return;

    level.initializing = 1;
    spawned = G_Spawn();
    spawned->r.currentOrigin[0] = ent->r.currentOrigin[0];
    spawned->r.currentOrigin[1] = ent->r.currentOrigin[1];
    spawned->r.currentOrigin[2] = ent->r.currentOrigin[2];
    G_GetItemClassname(item, &spawned->classname);
    G_SpawnItem(spawned, item);
    spawned->active = 1;
    Touch_Item(spawned, ent, 1);
    spawned->active = 0;

    if (spawned->r.inuse)
        G_FreeEntity(spawned);

    level.initializing = 0;
}

void Cmd_Take_f(gentity_t *ent)
{
    playerState_t *ps;
    int amount;
    int take_all;
    int i;

    if (!CheatsOk(ent))
        return;

    amount = atoi(ConcatArgs_impl(2));
    ConcatArgs_impl(1);

    if (!line[0])
        return;

    ps = &ent->client->ps;
    take_all = !I_stricmp(line, "all");

    if (take_all || !I_strnicmp(line, "health", 6)) {
        if (amount) {
            ent->health -= amount;
            if (ent->health <= 0)
                ent->health = 1;
        } else {
            ent->health = 1;
        }

        if (!take_all)
            return;
    }

    if (take_all || !I_stricmp(line, "weapons")) {
        for (i = 1; i <= BG_GetNumWeapons(); ++i) {
            BG_TakePlayerWeapon(ps, i);
            G_ClearAmmoForWeapon(ent, i);
        }

        if (ps->weapon) {
            ps->weapon = 0;
            G_SelectWeaponIndex((int)(ent - g_entities), 0);
        }

        if (!take_all)
            return;
    }

    if (take_all || !I_strnicmp(line, "ammo", 4)) {
        if (amount) {
            if (ps->weapon)
                G_TakeAmmoForWeapon(ent, ps->weapon, amount);
        } else {
            for (i = 1; i <= BG_GetNumWeapons(); ++i)
                G_ClearAmmoForWeapon(ent, i);
        }

        if (!take_all)
            return;
    }

    if (!I_strnicmp(line, "allammo", 7) && amount) {
        for (i = 1; i <= BG_GetNumWeapons(); ++i)
            G_TakeAmmoForWeapon(ent, i, amount);
    }
}

void ClientCommand(int clientNum)
{
    char cmd[1024];
    char arg[1024];
    char cleanname[64];
    char targetCleanName[64];
    gentity_t *ent;
    gentity_t *target;
    gclient_t *client;
    const char *message;
    int targetNum;
    int senderNum;
    int targetClientNum;

    ent = &g_entities[clientNum];
    client = ent->client;
    if (!client)
        return;

    SV_Cmd_ArgvBuffer(0, cmd, sizeof(cmd));

    if (!I_stricmp(cmd, "say")) {
        if (SV_Cmd_Argc() <= 1)
            return;

        G_Say(ent, NULL, 0, ConcatArgs_impl(1));
        return;
    }

    if (!I_stricmp(cmd, "say_team")) {
        if (SV_Cmd_Argc() <= 1)
            return;

        G_Say(ent, NULL, 1, ConcatArgs_impl(1));
        return;
    }

    if (!I_stricmp(cmd, "tell")) {
        if (SV_Cmd_Argc() <= 1)
            return;

        SV_Cmd_ArgvBuffer(1, arg, sizeof(arg));
        targetNum = atoi(arg);
        if (targetNum < 0 || targetNum >= level.maxclients)
            return;

        target = &g_entities[targetNum];
        if (!target->r.inuse || !target->client)
            return;

        ConcatArgs_impl(2);

        I_strncpyz(cleanname, ent->client->sess.cs.name, sizeof(cleanname));
        I_CleanStr(cleanname);
        I_strncpyz(targetCleanName, target->client->sess.cs.name, sizeof(targetCleanName));
        I_CleanStr(targetCleanName);

        senderNum = ent->s.number;
        targetClientNum = target->s.number;
        G_LogPrintf("tell;%d;%d;%s;%d;%d;%s;%s\n",
                    SV_GetGuid(senderNum), senderNum, cleanname,
                    SV_GetGuid(targetClientNum), targetClientNum, targetCleanName, line);

        G_Say(ent, target, 2, line);
        G_Say(ent, ent, 2, line);
        return;
    }

    if (!I_stricmp(cmd, "score")) {
        DeathmatchScoreboardMessage(ent);
        return;
    }

    if (client->ps.pm_type == 5)
        return;

    if (!I_stricmp(cmd, "mr")) {
        Cmd_MenuResponse_f(ent);
        return;
    }

    if (!I_stricmp(cmd, "give")) {
        Cmd_Give_f(ent);
        return;
    }

    if (!I_stricmp(cmd, "take")) {
        Cmd_Take_f(ent);
        return;
    }

    if (!I_stricmp(cmd, "god")) {
        if (!CheatsOk(ent))
            return;

        ent->flags ^= 1;
        message = (ent->flags & 1) ? "GAME_GODMODE_ON" : "GAME_GODMODE_OFF";
        SV_GameSendServerCommand(ent - g_entities, SV_CMD_CAN_IGNORE, va("%c \"%s\"", 101, message));
        return;
    }

    if (!I_stricmp(cmd, "demigod")) {
        if (!CheatsOk(ent))
            return;

        ent->flags ^= 2;
        message = (ent->flags & 2) ? "GAME_DEMI_GODMODE_ON" : "GAME_DEMI_GODMODE_OFF";
        SV_GameSendServerCommand(ent - g_entities, SV_CMD_CAN_IGNORE, va("%c \"%s\"", 101, message));
        return;
    }

    if (!I_stricmp(cmd, "notarget")) {
        if (!CheatsOk(ent))
            return;

        ent->flags ^= 4;
        message = (ent->flags & 4) ? "GAME_NOTARGETON" : "GAME_NOTARGETOFF";
        SV_GameSendServerCommand(ent - g_entities, SV_CMD_CAN_IGNORE, va("%c \"%s\"", 101, message));
        return;
    }

    if (!I_stricmp(cmd, "noclip")) {
        if (!CheatsOk(ent))
            return;

        if (client->noclip) {
            client->noclip = 0;
            message = "GAME_NOCLIPOFF";
        } else {
            client->noclip = 1;
            message = "GAME_NOCLIPON";
        }

        SV_GameSendServerCommand(ent - g_entities, SV_CMD_CAN_IGNORE, va("%c \"%s\"", 101, message));
        return;
    }

    if (!I_stricmp(cmd, "ufo")) {
        if (!CheatsOk(ent))
            return;

        if (client->ufo) {
            client->ufo = 0;
            message = "GAME_UFOOFF";
        } else {
            client->ufo = 1;
            message = "GAME_UFOON";
        }

        SV_GameSendServerCommand(ent - g_entities, SV_CMD_CAN_IGNORE, va("%c \"%s\"", 101, message));
        return;
    }

    if (!I_stricmp(cmd, "kill")) {
        if (client->sess.sessionState != SESS_STATE_PLAYING)
            return;

        ent->flags &= ~3;
        ent->health = 0;
        client->ps.stats[0] = 0;
        player_die(ent, ent, ent, 100000, 12, 0, NULL, HITLOC_NONE, 0);
        return;
    }

    if (!I_stricmp(cmd, "follownext")) {
        Cmd_FollowCycle_f(ent, 1);
        return;
    }

    if (!I_stricmp(cmd, "followprev")) {
        Cmd_FollowCycle_f(ent, -1);
        return;
    }

    if (!I_stricmp(cmd, "where")) {
        SV_GameSendServerCommand(ent - g_entities, SV_CMD_CAN_IGNORE,
                                 va("%c \"\x15%s\n\"", 101, vtos(ent->r.currentOrigin)));
        return;
    }

    if (!I_stricmp(cmd, "callvote")) {
        Cmd_CallVote_f(ent);
        return;
    }

    if (!I_stricmp(cmd, "vote")) {
        Cmd_Vote_f(ent);
        return;
    }

    if (!I_stricmp(cmd, "gc")) {
        Cmd_GameCommand_f(ent);
        return;
    }

    if (!I_stricmp(cmd, "setviewpos")) {
        Cmd_SetViewpos_f(ent);
        return;
    }

    if (!I_stricmp(cmd, "entitycount")) {
        if ((*(const dvar_t **)imp_g_cheats)->current.enabled)
            Com_Printf("entity count = %i\n", level.num_entities);
        return;
    }

    if (!I_stricmp(cmd, "printentities")) {
        G_PrintEntities();
        return;
    }

    SV_GameSendServerCommand(clientNum, SV_CMD_CAN_IGNORE,
                             va("%c \"GAME_UNKNOWNCLIENTCOMMAND\x15%s\"", 101, cmd));
}
