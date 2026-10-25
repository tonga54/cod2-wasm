#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include "headers/PC/cgame_mp/cg_overview.h"
#include "headers/PC/cgame_mp/cg_scorepanel.h"
#include "headers/PC/cgame_mp/cg_scoretable.h"
#include "headers/PC/gfx_d3d/r_hud2d.h"

#include <stdlib.h>
#include <string.h>

extern const char *CL_GetConfigString(int index);
extern const char *Info_ValueForKey(const char *s, const char *key);
extern const char *UI_GetGameTypeDisplayName(const char *gameType);
extern const char *SEH_LocalizeTextMessage(const char *msg, const char *context, int errType);
extern void CL_AddReliableCommand(const char *cmd);
extern void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign);
extern void I_strncpyz(char *dest, const char *src, int destsize);
extern char *I_CleanStr(char *string);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);

#define OVERVIEW_MAX_CLIENTS 64
#define OVERVIEW_SCORES_MSEC 2000
/* The player state's flag for a vote cast, as the server and the HUD use it. */
#define OVERVIEW_VOTED 0x100000
/* The name the server gives its bots (SV_AddTestClient). */
#define OVERVIEW_BOT_PREFIX "[BOT]"
#define OVERVIEW_ALIGN_LEFT 1
#define OVERVIEW_ALIGN_TOP 1
/* The emblems are built at the size they are drawn, within these bounds. */
#define OVERVIEW_EMBLEM_MIN_TEXELS 16
#define OVERVIEW_EMBLEM_MAX_TEXELS 512

static struct {
    int size;
    int allies;
    int generation;
    MaterialHandle material;
} s_overviewEmblems;

static int CG_OverviewTeam(int clientNum)
{
    const clientInfo_t *info;

    if ((unsigned)clientNum >= OVERVIEW_MAX_CLIENTS)
        return TEAM_SPECTATOR;
    info = &cg->bgs.clientinfo[clientNum];
    return info->infoValid ? info->team : TEAM_SPECTATOR;
}

int CG_OverviewGet(CG_Overview *overview)
{
    vec4_t color;
    const char *clock;
    int client;
    int team;
    int i;

    memset(overview, 0, sizeof(*overview));
    overview->score = overview->kills = overview->deaths = overview->assists = overview->headshots = -1;
    overview->emblem = -1;
    if (!cg->snap)
        return 0;
    client = cg->snap->ps.clientNum;
    team = CG_OverviewTeam(client);

    I_strncpyz(overview->mode, SEH_LocalizeTextMessage(UI_GetGameTypeDisplayName(cgs->gametype),
                                                       "pause menu gametype display", 0),
               sizeof(overview->mode));
    CG_ScoreTableMapName(overview->map, sizeof(overview->map));
    overview->freeForAll = CG_ScoreTableFreeForAll();
    if (!CG_ScorePanelMatch(&overview->topScore, &overview->bottomScore, &overview->limit, &overview->standing))
        overview->standing = CG_SCORE_PANEL_NEUTRAL;
    /* As on the score panel, a spectator sees the Allies on top. */
    if (!overview->freeForAll)
        overview->emblem = CG_ScoreTableEmblemKind(team == TEAM_AXIS ? TEAM_AXIS : TEAM_ALLIES);

    for (i = 0; i < cg->numScores && i < OVERVIEW_MAX_CLIENTS; ++i) {
        if (cg->scores[i].client != client)
            continue;
        overview->score = cg->scores[i].score;
        overview->deaths = cg->scores[i].deaths;
        overview->kills = CG_ScoreTableKills(client);
        break;
    }
    CG_ScoreTableOwnStats(&overview->assists, &overview->headshots);

    clock = CG_ScorePanelClock(color);
    if (clock && color[3] > 0.0f) {
        I_strncpyz(overview->clock, clock, sizeof(overview->clock));
        memcpy(overview->clockColor, color, sizeof(overview->clockColor));
    }
    return 1;
}

int CG_OverviewTeamSlots(CG_TeamSlots *slots)
{
    int maxClients;
    int i;

    memset(slots, 0, sizeof(*slots));
    slots->team = TEAM_SPECTATOR;
    if (!cg->snap)
        return 0;
    slots->freeForAll = CG_ScoreTableFreeForAll();
    slots->team = CG_OverviewTeam(cg->snap->ps.clientNum);
    for (i = 0; i < OVERVIEW_MAX_CLIENTS; ++i) {
        int team = CG_OverviewTeam(i);

        if (team == TEAM_ALLIES)
            ++slots->allies;
        else if (team == TEAM_AXIS)
            ++slots->axis;
    }
    maxClients = atoi(Info_ValueForKey(CL_GetConfigString(0), "sv_maxclients"));
    if (maxClients > OVERVIEW_MAX_CLIENTS)
        maxClients = OVERVIEW_MAX_CLIENTS;
    /* No team is capped; half the server is each one's share. */
    slots->alliesOpen = (maxClients + 1) / 2 - slots->allies;
    slots->axisOpen = maxClients / 2 - slots->axis;
    if (slots->alliesOpen < 0)
        slots->alliesOpen = 0;
    if (slots->axisOpen < 0)
        slots->axisOpen = 0;
    slots->open = slots->alliesOpen + slots->axisOpen;
    return 1;
}

void CG_OverviewRequestScores(void)
{
    if (!cg->snap || cg->scoresRequestTime + OVERVIEW_SCORES_MSEC >= cg->time)
        return;
    cg->scoresRequestTime = cg->time;
    CL_AddReliableCommand("score");
}

MaterialHandle CG_OverviewEmblems(float size, int *allies)
{
    float x = 0.0f;
    float y = 0.0f;
    float w = 1.0f;
    float h = 1.0f;
    int texels;
    int kind = CG_ScoreTableEmblemKind(TEAM_ALLIES);
    byte *bgra;

    *allies = kind;
    CalcScreenPlacement(&x, &y, &w, &h, OVERVIEW_ALIGN_LEFT, OVERVIEW_ALIGN_TOP);
    texels = (int)(size * h + 0.5f);
    if (texels < OVERVIEW_EMBLEM_MIN_TEXELS)
        texels = OVERVIEW_EMBLEM_MIN_TEXELS;
    else if (texels > OVERVIEW_EMBLEM_MAX_TEXELS)
        texels = OVERVIEW_EMBLEM_MAX_TEXELS;
    if (texels == s_overviewEmblems.size && kind == s_overviewEmblems.allies &&
        R_GetHudImageGeneration() == s_overviewEmblems.generation)
        return s_overviewEmblems.material;
    s_overviewEmblems.size = texels;
    s_overviewEmblems.allies = kind;
    s_overviewEmblems.generation = R_GetHudImageGeneration();
    s_overviewEmblems.material = NULL;
    bgra = (byte *)malloc((size_t)texels * (size_t)texels * 8);
    if (!bgra)
        return NULL;
    CG_ScoreTableRasterizeEmblem(kind, bgra, texels * 8, texels);
    CG_ScoreTableRasterizeEmblem(CG_ScoreTableEmblemKind(TEAM_AXIS), bgra + texels * 4, texels * 8, texels);
    s_overviewEmblems.material = R_CreateHudImageMaterial(R_HUD_IMAGE_PAUSE_EMBLEMS, bgra, 2 * texels, texels);
    free(bgra);
    return s_overviewEmblems.material;
}

void CG_OverviewRatio(int kills, int deaths, char *buf, int size)
{
    if (kills < 0)
        kills = 0;
    if (deaths < 0)
        deaths = 0;
    Com_sprintf(buf, size, "%.2f", deaths > 0 ? (double)kills / (double)deaths : (double)kills);
}

int CG_OverviewVote(CG_Vote *vote)
{
    memset(vote, 0, sizeof(*vote));
    if (!cg->snap || !cg->nextSnap)
        return 0;
    I_strncpyz(vote->map, Info_ValueForKey(CL_GetConfigString(0), "mapname"), sizeof(vote->map));
    I_strncpyz(vote->gametype, cgs->gametype, sizeof(vote->gametype));
    vote->spectator = CG_OverviewTeam(cg->snap->ps.clientNum) == TEAM_SPECTATOR;
    if (!cgs->voteTime)
        return 1;
    vote->active = 1;
    vote->secondsLeft = (cgs->voteTime - cg->time) / 1000;
    if (vote->secondsLeft < 0)
        vote->secondsLeft = 0;
    vote->yes = cgs->voteYes;
    vote->no = cgs->voteNo;
    vote->voted = (cg->nextSnap->ps.eFlags & OVERVIEW_VOTED) != 0;
    I_strncpyz(vote->text, cgs->voteString, sizeof(vote->text));
    I_CleanStr(vote->text);
    return 1;
}

int CG_OverviewVotePlayers(CG_VotePlayer *players, int max)
{
    int count = 0;
    int i;

    if (!cg->snap)
        return 0;
    for (i = 0; i < OVERVIEW_MAX_CLIENTS && count < max; ++i) {
        const clientInfo_t *info = &cg->bgs.clientinfo[i];

        if (!info->infoValid || i == cg->snap->ps.clientNum ||
            !strncmp(info->name, OVERVIEW_BOT_PREFIX, sizeof(OVERVIEW_BOT_PREFIX) - 1))
            continue;
        players[count].clientNum = i;
        I_strncpyz(players[count].name, info->name, sizeof(players[count].name));
        I_CleanStr(players[count].name);
        ++count;
    }
    return count;
}
