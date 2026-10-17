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
void CG_DrawScorePanel(void);

#endif
