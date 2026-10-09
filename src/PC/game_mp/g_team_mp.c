#include "common_types.h"
#include "imports.h"
#include <string.h>

extern void G_GetPlayerViewOrigin(gentity_t *ent, vec3_t origin);
extern void G_GetPlayerViewDirection(gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up);
extern void G_TraceCapsule(trace_t *trace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passEntityNum, int contentMask);
extern int G_IsPlaying(gentity_t *ent);

extern gentity_t g_entities[];
extern level_locals_t level;
extern byte *sv_maxclients;
extern vec3_t *trace_mins;

#define STAT_IDENT_CLIENT_NUM 3
#define STAT_IDENT_CLIENT_HEALTH 4
#define PMF_FOLLOW 0x400000

qboolean OnSameTeam(gentity_t *ent1, gentity_t *ent2);
void TeamplayInfoMessage(gentity_t *ent);
void CheckTeamStatus(void);

qboolean OnSameTeam(gentity_t *ent1, gentity_t *ent2)
{
    gclient_t *cl1;
    gclient_t *cl2;
    int team1;

    cl1 = ent1->client;
    if (!cl1)
        return 0;

    cl2 = ent2->client;
    if (!cl2)
        return 0;

    team1 = cl1->sess.cs.team;
    if (team1 == 0)
        return 0;

    if (team1 == cl2->sess.cs.team)
        return 1;

    return 0;
}

void TeamplayInfoMessage(gentity_t *ent)
{
    gclient_t *client;
    vec3_t vStart;
    vec3_t vForward;
    vec3_t vEnd;
    trace_t trace;
    int identEnt;
    int compassTime;
    gentity_t *identGent;
    gclient_t *identClient;

    client = ent->client;

    if (client->sess.sessionState != SESS_STATE_PLAYING) {

        G_GetPlayerViewOrigin(ent, vStart);
        G_GetPlayerViewDirection(ent, vForward, NULL, NULL);

        if (client->ps.viewHeightCurrent < 8.0f) {
            vStart[2] += 8.0f - client->ps.viewHeightCurrent;
        }
    } else if (client->sess.cs.team != 0) {

        G_GetPlayerViewOrigin(ent, vStart);
        G_GetPlayerViewDirection(ent, vForward, NULL, NULL);
    } else {

        client->ps.stats[STAT_IDENT_CLIENT_NUM] = -1;
        client = ent->client;
        client->ps.stats[STAT_IDENT_CLIENT_HEALTH] = 0;
        return;
    }

    vEnd[0] = vStart[0] + vForward[0] * 8192.0f;
    vEnd[1] = vStart[1] + vForward[1] * 8192.0f;
    vEnd[2] = vStart[2] + vForward[2] * 8192.0f;

    G_TraceCapsule(&trace, vStart, (vec_t *)trace_mins, (vec_t *)trace_mins, vEnd, client->ps.clientNum, 0x2000001);

    identEnt = trace.entityNum;

    if (identEnt <= 63) {
        identGent = &((gentity_t *)imp_g_entities)[identEnt];
        identClient = identGent->client;

        if (identClient != NULL) {
            if (!G_IsPlaying(ent)) {

                compassTime = identGent->health;
                goto store;
            }

            if (identClient->sess.cs.team == client->sess.cs.team) {
                compassTime = identGent->health;
                goto store;
            }
        }
    }

    identEnt = -1;
    compassTime = 0;

store:
    client = ent->client;
    client->ps.stats[STAT_IDENT_CLIENT_NUM] = identEnt;
    client = ent->client;
    client->ps.stats[STAT_IDENT_CLIENT_HEALTH] = compassTime;
}

void CheckTeamStatus(void)
{
    level_locals_t *level;
    int time;
    int lastTime;
    int maxClients;
    int i;
    gentity_t *ent;
    gclient_t *cl;

    level = (level_locals_t *)imp_level;
    time = level->time;
    lastTime = level->lastTeammateHealthTime;

    if (time - lastTime <= 0)
        return;

    level->lastTeammateHealthTime = time;

    maxClients = (*(const dvar_t **)imp_g_maxclients)->current.integer;
    if (maxClients <= 0)
        return;

    for (i = 0; i < maxClients; i++) {
        ent = &((gentity_t *)imp_g_entities)[i];

        if (!ent->r.inuse)
            continue;

        cl = ent->client;
        if (cl == NULL)
            continue;
        if (cl->ps.pm_flags & PMF_FOLLOW)
            continue;

        TeamplayInfoMessage(ent);
    }

    return;
}
