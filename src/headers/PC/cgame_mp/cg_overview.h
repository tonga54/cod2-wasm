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

#endif
