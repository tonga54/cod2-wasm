#ifndef CG_SCOREPANEL_H
#define CG_SCOREPANEL_H

/* The WWII score panel in the bottom-left corner: the clock, the standing or
 * the mode name, and one bar per side filling toward the score limit. In the
 * stock gametypes it stands in for the clock and score HUD elements. */
void CG_ScorePanelInit(void);
/* Deathmatch scores sent by the native server, to find the leader. */
void CG_ScorePanelClearDmScores(void);
void CG_ScorePanelSetDmScore(int clientNum, int score);
int CG_ScorePanelHidesHudElem(const hudelem_t *elem);
/* The stock clock's text and color, for the scoreboard; NULL without one. */
const char *CG_ScorePanelClock(vec4_t color);
/* The panel's numbers for the viewer: their side's score on top and the
 * other's (in deathmatch theirs and the best other player's), the score
 * limit, and 1, 0 or -1 when the top one leads, ties or trails, or
 * CG_SCORE_PANEL_NEUTRAL for a view without a side. 0 in another mode. */
#define CG_SCORE_PANEL_NEUTRAL 2
int CG_ScorePanelMatch(int *topScore, int *bottomScore, int *limit, int *standing);
void CG_DrawScorePanel(void);

#endif
