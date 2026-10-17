#ifndef CG_SCORETABLE_H
#define CG_SCORETABLE_H

/* The WWII scoreboard: the match title and clock, then each team's block of
 * striped rows with its name, score and emblem, the viewer's side on top. */
void CG_ScoreTableInit(void);
int CG_ScoreTableEnabled(void);
/* While it shows, the scoreboard takes the place of the whole HUD. */
int CG_ScoreTableHidesHud(void);
/* Our server ends the scores command with the kills of the listed players;
 * without them the Kills column stays empty. */
void CG_ScoreTableClearKills(void);
void CG_ScoreTableSetKills(int clientNum, int kills);
/* Builds the team emblems for the current screen size; cheap once built. */
void CG_ScoreTablePrepare(void);
void CG_DrawScoreTable(float fade);

#endif
