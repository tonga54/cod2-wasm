#ifndef CG_SCOREPOPUP_H
#define CG_SCOREPOPUP_H

/* "Kill +100" next to the crosshair for the points the server awards. */
void CG_ScorePopupInit(void);
void CG_ScorePopupAdd(int event, int points);
void CG_DrawScorePopup(void);

#endif
