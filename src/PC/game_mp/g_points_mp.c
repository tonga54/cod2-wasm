#include "common_types.h"
#include "imports.h"
#include <string.h>
#include "headers/PC/game_mp/g_points.h"

extern level_locals_t level;
extern int I_stricmp(const char *s1, const char *s2);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern char *va(const char *fmt, ...);
extern void SV_GameSendServerCommand(int clientNum, int type, const char *text);

enum {
    POINTS_MAX_CLIENTS = 64,
    POINTS_MAX_KEPT = 64,
    POINTS_KEY_SIZE = 32,
    POINTS_MOD_MELEE = 7,
    POINTS_HITLOC_HEAD = 2,
    POINTS_CTF_CAPTURE_SCORE = 10,
    POINTS_CTF_RETURN_SCORE = 2
};

enum {
    POINTS_GT_TEAM = 0,
    POINTS_GT_DM = 1,
    POINTS_GT_CTF = 2,
    POINTS_GT_SD = 3
};

typedef struct {
    int points;
    int kills;
    int assists;
    int headshots;
    int persId;
    int inUse;
    unsigned long long damagedBy;
    int damageOrder[POINTS_MAX_CLIENTS];
    int damageCount;
} G_PointsClient;

/* What a person who left the match had, under their gametag. */
typedef struct {
    char key[POINTS_KEY_SIZE];
    int points;
    int kills;
    int assists;
    int headshots;
    int score;
    int deaths;
} G_PointsKept;

/* Objectives are worth whole kills: a capture three, a return one, a plant
 * or a defuse two. */
static const int s_pointsValues[POINTS_EVENT_COUNT] = {
    100, /* POINTS_KILL */
    75,  /* POINTS_ASSISTED_KILL */
    150, /* POINTS_HEADSHOT */
    25,  /* POINTS_ASSIST */
    300, /* POINTS_FLAG_CAPTURE */
    100, /* POINTS_FLAG_RETURN */
    200, /* POINTS_BOMB_PLANT */
    200  /* POINTS_BOMB_DEFUSE */
};

static G_PointsClient s_pointsClients[POINTS_MAX_CLIENTS];
static G_PointsKept s_pointsKept[POINTS_MAX_KEPT];
static int s_pointsKeptCount;
static int s_pointsGametype;

static int G_PointsTeam(int clientNum)
{
    return level.clients[clientNum].sess.cs.team;
}

static int G_PointsConnected(int clientNum)
{
    return level.clients[clientNum].sess.connected == CON_CONNECTED;
}

static int G_PointsRetailScore(int clientNum)
{
    return level.clients[clientNum].sess.score;
}

static void G_PointsSend(int clientNum, const char *text)
{
    SV_GameSendServerCommand(clientNum, SV_CMD_RELIABLE, text);
}

static int G_PointsValidClient(int clientNum)
{
    return clientNum >= 0 && clientNum < POINTS_MAX_CLIENTS && clientNum < level.maxclients;
}

static int G_PointsEnemies(int a, int b)
{
    int teamA = G_PointsTeam(a);

    if (teamA == TEAM_SPECTATOR || G_PointsTeam(b) == TEAM_SPECTATOR)
        return 0;
    return teamA == TEAM_FREE || teamA != G_PointsTeam(b);
}

static void G_PointsAward(int clientNum, int event)
{
    int value = s_pointsValues[event];

    s_pointsClients[clientNum].points += value;
    G_PointsSend(clientNum, va("cod2_points %i %i", event, value));
}

/* The match's tallies; the damage of the current life stays. */
static void G_PointsClear(G_PointsClient *slot)
{
    slot->points = 0;
    slot->kills = 0;
    slot->assists = 0;
    slot->headshots = 0;
}

static void G_PointsForgetAttacker(int clientNum)
{
    unsigned long long mask = ~(1ull << clientNum);
    int i;

    for (i = 0; i < POINTS_MAX_CLIENTS; ++i)
        s_pointsClients[i].damagedBy &= mask;
}

/* A gametag as the gateway keys its classes (classes.mjs playerKey): color
 * codes, case and surrounding spaces aside. The name every new browser has
 * and the engine's for a blank one are nobody's. */
static int G_PointsNameKey(const char *name, char *key)
{
    static const char *const shared[] = { "Player", "Unknown Soldier", "UnnamedPlayer" };
    int length = 0;
    int i;

    for (; *name; ++name) {
        if (name[0] == '^' && name[1] >= '0' && name[1] <= '9')
            ++name;
        else if ((length || *name != ' ') && length < POINTS_KEY_SIZE - 1)
            key[length++] = *name;
    }
    while (length && key[length - 1] == ' ')
        --length;
    key[length] = '\0';
    for (i = 0; i < (int)(sizeof(shared) / sizeof(shared[0])); ++i) {
        if (!I_stricmp(key, shared[i]))
            return 0;
    }
    return length > 0;
}

static G_PointsKept *G_PointsFindKept(const char *key)
{
    int i;

    for (i = 0; i < s_pointsKeptCount; ++i) {
        if (!I_stricmp(s_pointsKept[i].key, key))
            return &s_pointsKept[i];
    }
    return NULL;
}

static void G_PointsForgetKept(G_PointsKept *kept)
{
    int index = (int)(kept - s_pointsKept);

    memmove(kept, kept + 1, (s_pointsKeptCount - index - 1) * sizeof(*kept));
    --s_pointsKeptCount;
}

/* A person leaving keeps their tallies for their gametag until the match
 * starts over; two sessions under one gametag add up, and the oldest gives
 * way when the list is full. Bots keep nothing. */
static void G_PointsKeep(int clientNum)
{
    const gclient_t *client = &level.clients[clientNum];
    const G_PointsClient *slot = &s_pointsClients[clientNum];
    G_PointsKept *kept;
    char key[POINTS_KEY_SIZE];

    if (client->sess.localClient || !G_PointsNameKey(client->sess.cs.name, key))
        return;
    if (!slot->points && !slot->kills && !slot->assists && !slot->headshots && !client->sess.score &&
        !client->sess.deaths)
        return;
    kept = G_PointsFindKept(key);
    if (!kept) {
        if (s_pointsKeptCount == POINTS_MAX_KEPT)
            G_PointsForgetKept(&s_pointsKept[0]);
        kept = &s_pointsKept[s_pointsKeptCount++];
        memset(kept, 0, sizeof(*kept));
        strcpy(kept->key, key);
    }
    kept->points += slot->points;
    kept->kills += slot->kills;
    kept->assists += slot->assists;
    kept->headshots += slot->headshots;
    kept->score += client->sess.score;
    kept->deaths += client->sess.deaths;
}

/* A person joining under a gametag that left this match takes back what it
 * had: the match points beside the retail score and deaths. */
static void G_PointsTakeBack(int clientNum)
{
    gclient_t *client = &level.clients[clientNum];
    G_PointsClient *slot = &s_pointsClients[clientNum];
    G_PointsKept *kept;
    char key[POINTS_KEY_SIZE];

    if (client->sess.localClient || !G_PointsNameKey(client->sess.cs.name, key))
        return;
    kept = G_PointsFindKept(key);
    if (!kept)
        return;
    slot->points = kept->points;
    slot->kills = kept->kills;
    slot->assists = kept->assists;
    slot->headshots = kept->headshots;
    client->sess.score = kept->score;
    client->sess.deaths = kept->deaths;
    G_PointsForgetKept(kept);
}

void G_PointsInitGame(int restart, const char *gametype)
{
    int newMatch;
    int i;

    if (!I_stricmp(gametype, "dm"))
        s_pointsGametype = POINTS_GT_DM;
    else if (!I_stricmp(gametype, "ctf"))
        s_pointsGametype = POINTS_GT_CTF;
    else if (!I_stricmp(gametype, "sd"))
        s_pointsGametype = POINTS_GT_SD;
    else
        s_pointsGametype = POINTS_GT_TEAM;

    /* SD plays every round as a map_restart and keeps the match going. */
    newMatch = !restart || s_pointsGametype != POINTS_GT_SD;
    if (newMatch)
        s_pointsKeptCount = 0;
    for (i = 0; i < POINTS_MAX_CLIENTS; ++i) {
        s_pointsClients[i].damagedBy = 0;
        if (newMatch)
            G_PointsClear(&s_pointsClients[i]);
    }
}

/* ClientConnect calls this for every client on each map load and restart,
 * once the client's name is set. Script pers ids are recycled, so a
 * disconnect also clears the slot; a slot it freed takes a person joining. */
void G_PointsClientConnect(int clientNum, int scriptPersId)
{
    G_PointsClient *slot;
    int joining;

    if (clientNum < 0 || clientNum >= POINTS_MAX_CLIENTS)
        return;
    slot = &s_pointsClients[clientNum];
    joining = !slot->inUse;
    if (joining || slot->persId != scriptPersId)
        G_PointsClear(slot);
    slot->inUse = 1;
    slot->persId = scriptPersId;
    slot->damagedBy = 0;
    G_PointsForgetAttacker(clientNum);
    if (joining)
        G_PointsTakeBack(clientNum);
}

/* ClientDisconnect calls this before it clears the client's name. */
void G_PointsClientDisconnect(int clientNum)
{
    if (clientNum < 0 || clientNum >= POINTS_MAX_CLIENTS)
        return;
    G_PointsKeep(clientNum);
    memset(&s_pointsClients[clientNum], 0, sizeof(s_pointsClients[clientNum]));
}

/* The deathmatch panel compares the player's kills with the leader's; a
 * player back in the match brings theirs to everyone's panel. */
void G_PointsClientBegin(int clientNum)
{
    char text[1024];
    int length;
    int i;

    if (s_pointsGametype != POINTS_GT_DM || !G_PointsValidClient(clientNum))
        return;
    length = Com_sprintf(text, sizeof(text), "cod2_dmscores");
    for (i = 0; i < level.maxclients && i < POINTS_MAX_CLIENTS; ++i) {
        if (!G_PointsConnected(i) || length > (int)sizeof(text) - 32)
            continue;
        length += Com_sprintf(text + length, sizeof(text) - length, " %i %i", i, G_PointsRetailScore(i));
    }
    G_PointsSend(clientNum, text);
    if (G_PointsRetailScore(clientNum))
        G_PointsSend(-1, va("cod2_dmscore %i %i", clientNum, G_PointsRetailScore(clientNum)));
}

void G_PointsClientSpawn(int clientNum)
{
    if (clientNum >= 0 && clientNum < POINTS_MAX_CLIENTS)
        s_pointsClients[clientNum].damagedBy = 0;
}

void G_PointsPlayerDamaged(gentity_t *victim, gentity_t *attacker, int damage)
{
    int victimNum;
    int attackerNum;

    if (damage <= 0 || !victim || !victim->client || !attacker || !attacker->client)
        return;
    victimNum = victim->s.number;
    attackerNum = attacker->s.number;
    if (victimNum == attackerNum || !G_PointsValidClient(victimNum) || !G_PointsValidClient(attackerNum))
        return;
    if (!(s_pointsClients[victimNum].damagedBy & (1ull << attackerNum)))
        s_pointsClients[victimNum].damageOrder[attackerNum] = ++s_pointsClients[victimNum].damageCount;
    if (G_PointsEnemies(attackerNum, victimNum))
        s_pointsClients[victimNum].damagedBy |= 1ull << attackerNum;
}

/* The victim's killcam lists who hurt them: the killer, then the others in
 * the order they first hit. */
static void G_PointsSendAttackers(int victimNum, int attackerNum, unsigned long long damagedBy)
{
    const G_PointsClient *slot = &s_pointsClients[victimNum];
    int order[POINTS_MAX_CLIENTS];
    char text[512];
    int count = 0;
    int length;
    int i, j;

    for (i = 0; i < level.maxclients && i < POINTS_MAX_CLIENTS; ++i) {
        if (i == attackerNum || !(damagedBy & (1ull << i)) || !G_PointsConnected(i))
            continue;
        for (j = count; j > 0 && slot->damageOrder[order[j - 1]] > slot->damageOrder[i]; --j)
            order[j] = order[j - 1];
        order[j] = i;
        ++count;
    }
    length = Com_sprintf(text, sizeof(text), "cod2_attackers %i", attackerNum);
    for (i = 0; i < count; ++i)
        length += Com_sprintf(text + length, sizeof(text) - length, " %i", order[i]);
    G_PointsSend(victimNum, text);
}

void G_PointsPlayerKilled(gentity_t *victim, gentity_t *attacker, int meansOfDeath, int hitLoc)
{
    unsigned long long damagedBy;
    int victimNum;
    int attackerNum;
    int assisted = 0;
    int i;

    if (!victim || !victim->client || !G_PointsValidClient(victim->s.number))
        return;
    victimNum = victim->s.number;
    damagedBy = s_pointsClients[victimNum].damagedBy;
    s_pointsClients[victimNum].damagedBy = 0;

    if (!attacker || !attacker->client)
        return;
    attackerNum = attacker->s.number;
    if (attackerNum != victimNum && G_PointsValidClient(attackerNum))
        G_PointsSendAttackers(victimNum, attackerNum, damagedBy);
    if (attackerNum == victimNum || !G_PointsValidClient(attackerNum) || !G_PointsEnemies(attackerNum, victimNum))
        return;
    ++s_pointsClients[attackerNum].kills;

    /* Free-for-all has no teammates to assist. */
    if (s_pointsGametype != POINTS_GT_DM) {
        for (i = 0; i < level.maxclients && i < POINTS_MAX_CLIENTS; ++i) {
            if (i == attackerNum || !(damagedBy & (1ull << i)))
                continue;
            if (!G_PointsConnected(i) || !G_PointsEnemies(i, victimNum))
                continue;
            G_PointsAward(i, POINTS_ASSIST);
            ++s_pointsClients[i].assists;
            assisted = 1;
        }
    }

    if (hitLoc == POINTS_HITLOC_HEAD && meansOfDeath != POINTS_MOD_MELEE) {
        G_PointsAward(attackerNum, POINTS_HEADSHOT);
        ++s_pointsClients[attackerNum].headshots;
    } else
        G_PointsAward(attackerNum, assisted ? POINTS_ASSISTED_KILL : POINTS_KILL);
}

/* Retail CTF adds 10 to the carrier's score for a capture and 2 for a
 * return; kills only move it by one. */
void G_PointsScoreChanged(int clientNum, int oldScore, int newScore)
{
    if (!G_PointsValidClient(clientNum) || oldScore == newScore)
        return;
    if (s_pointsGametype == POINTS_GT_CTF) {
        if (newScore - oldScore == POINTS_CTF_CAPTURE_SCORE)
            G_PointsAward(clientNum, POINTS_FLAG_CAPTURE);
        else if (newScore - oldScore == POINTS_CTF_RETURN_SCORE)
            G_PointsAward(clientNum, POINTS_FLAG_RETURN);
    } else if (s_pointsGametype == POINTS_GT_DM) {
        G_PointsSend(-1, va("cod2_dmscore %i %i", clientNum, newScore));
    }
}

/* Scripts only zero both team scores outside level initialization when a
 * match starts over (SD resetScores after the warm-up round). */
void G_PointsTeamScoresChanged(int initializing, int axisScore, int alliesScore)
{
    int i;

    if (initializing || axisScore != 0 || alliesScore != 0)
        return;
    for (i = 0; i < POINTS_MAX_CLIENTS; ++i)
        G_PointsClear(&s_pointsClients[i]);
    s_pointsKeptCount = 0;
}

/* SD logs "A;<guid>;<entnum>;<team>;<name>;bomb_plant" and "...;bomb_defuse". */
void G_PointsLogLine(const char *line)
{
    const char *field;
    const char *action;
    int clientNum = 0;
    int digits = 0;
    int event;

    if (!line || line[0] != 'A' || line[1] != ';')
        return;
    field = strchr(line + 2, ';');
    action = strrchr(line, ';');
    if (!field || action == field)
        return;
    for (++field; *field >= '0' && *field <= '9' && digits < 3; ++field, ++digits)
        clientNum = clientNum * 10 + (*field - '0');
    if (!digits || *field != ';')
        return;

    ++action;
    if (!strcmp(action, "bomb_plant\n") || !strcmp(action, "bomb_plant"))
        event = POINTS_BOMB_PLANT;
    else if (!strcmp(action, "bomb_defuse\n") || !strcmp(action, "bomb_defuse"))
        event = POINTS_BOMB_DEFUSE;
    else
        return;
    if (G_PointsValidClient(clientNum) && G_PointsConnected(clientNum))
        G_PointsAward(clientNum, event);
}

int G_PointsGet(int clientNum)
{
    if (clientNum < 0 || clientNum >= POINTS_MAX_CLIENTS)
        return 0;
    return s_pointsClients[clientNum].points;
}

int G_PointsKills(int clientNum)
{
    if (clientNum < 0 || clientNum >= POINTS_MAX_CLIENTS)
        return 0;
    return s_pointsClients[clientNum].kills;
}

int G_PointsAssists(int clientNum)
{
    if (clientNum < 0 || clientNum >= POINTS_MAX_CLIENTS)
        return 0;
    return s_pointsClients[clientNum].assists;
}

int G_PointsHeadshots(int clientNum)
{
    if (clientNum < 0 || clientNum >= POINTS_MAX_CLIENTS)
        return 0;
    return s_pointsClients[clientNum].headshots;
}

/* Players before spectators, then by points. The input is the retail rank
 * order, which the stable sort keeps for ties. */
void G_PointsSortClients(int *clients, int count)
{
    int i;
    int j;

    for (i = 1; i < count; ++i) {
        int client = clients[i];
        int spectator = G_PointsTeam(client) == TEAM_SPECTATOR;
        int points = G_PointsGet(client);

        for (j = i; j > 0; --j) {
            int other = clients[j - 1];
            int otherSpectator = G_PointsTeam(other) == TEAM_SPECTATOR;

            if (otherSpectator < spectator || (otherSpectator == spectator && G_PointsGet(other) >= points))
                break;
            clients[j] = other;
        }
        clients[j] = client;
    }
}
