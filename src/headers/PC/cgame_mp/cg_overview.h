#ifndef CG_OVERVIEW_H
#define CG_OVERVIEW_H

/* What the pause menu shows of the match: the mode and the map, both sides'
 * scores, the viewer's battle stats and the clock for its Overview, and the
 * teams' players and free slots for its Change Team. */
typedef struct {
    char mode[64];
    char map[64];
    int freeForAll;
    /* The score panel's: the viewer's side on top, its limit and standing. */
    int topScore;
    int bottomScore;
    int limit;
    int standing;
    /* The viewer's emblem, -1 for none. */
    int emblem;
    /* -1 until the server has said. */
    int score;
    int kills;
    int deaths;
    int assists;
    int headshots;
    /* Empty without a clock. */
    char clock[16];
    vec4_t clockColor;
} CG_Overview;

typedef struct {
    int freeForAll;
    /* TEAM_ALLIES, TEAM_AXIS, or TEAM_SPECTATOR for neither. */
    int team;
    int allies;
    int axis;
    /* The free slots: half the server's each, and either. */
    int alliesOpen;
    int axisOpen;
    int open;
} CG_TeamSlots;

/* For the pause menu's Vote tab: the map and mode played, and the vote
 * running, as the HUD shows it. */
typedef struct {
    char map[64];
    char gametype[32];
    /* The viewer watches: the server takes no vote from a spectator. */
    int spectator;
    int active;
    int secondsLeft;
    int yes;
    int no;
    /* The viewer has voted, or called the vote. */
    int voted;
    char text[256];
} CG_Vote;

/* A player a vote may kick: the people in the match, not the viewer. */
typedef struct {
    int clientNum;
    char name[32];
} CG_VotePlayer;

/* 0 outside a match. */
int CG_OverviewGet(CG_Overview *overview);
int CG_OverviewTeamSlots(CG_TeamSlots *slots);
/* Asks the server for the scores every two seconds, as the scoreboard does
 * while it shows. */
void CG_OverviewRequestScores(void);
/* The Allies' emblem on the left and the Axis' on the right of one white
 * image, built for size units; NULL without one. allies gets the Allies'
 * emblem kind, -1 when their nation has none. */
MaterialHandle CG_OverviewEmblems(float size, int *allies);
/* "0.00": kills over deaths, or kills alone before the first death. */
void CG_OverviewRatio(int kills, int deaths, char *buf, int size);
/* 0 outside a match. */
int CG_OverviewVote(CG_Vote *vote);
/* The people other than the viewer, bots left out, in client order; how many. */
int CG_OverviewVotePlayers(CG_VotePlayer *players, int max);

#endif
