#ifndef G_POINTS_H
#define G_POINTS_H

/* Match points shown on the scoreboard. The retail `score` field keeps
 * counting kills because the gametype scripts end DM matches with it and
 * restore it between SD rounds. Event codes are sent to the scoring client as
 * "cod2_points <event> <points>". */
enum pointsEvent_t {
    POINTS_KILL = 0,
    POINTS_ASSISTED_KILL = 1,
    POINTS_HEADSHOT = 2,
    POINTS_ASSIST = 3,
    POINTS_FLAG_CAPTURE = 4,
    POINTS_FLAG_RETURN = 5,
    POINTS_BOMB_PLANT = 6,
    POINTS_BOMB_DEFUSE = 7,
    POINTS_EVENT_COUNT = 8
};

#ifndef CG_POINTS_CLIENT_ONLY
void G_PointsInitGame(int restart, const char *gametype);
void G_PointsClientConnect(int clientNum, int scriptPersId);
void G_PointsClientBegin(int clientNum);
void G_PointsClientDisconnect(int clientNum);
void G_PointsClientSpawn(int clientNum);
void G_PointsPlayerDamaged(gentity_t *victim, gentity_t *attacker, int damage);
void G_PointsPlayerKilled(gentity_t *victim, gentity_t *attacker, int meansOfDeath, int hitLoc);
void G_PointsScoreChanged(int clientNum, int oldScore, int newScore);
void G_PointsTeamScoresChanged(int initializing, int axisScore, int alliesScore);
void G_PointsLogLine(const char *line);
int G_PointsGet(int clientNum);
void G_PointsSortClients(int *clients, int count);
#endif

#endif
