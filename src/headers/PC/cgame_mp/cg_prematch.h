#ifndef CG_PREMATCH_H
#define CG_PREMATCH_H

/* The WWII countdown before a match. A player who spawns while the server
 * holds the match sees the view in black and white with the mode and map, the
 * mode's objective, the viewer's team and the seconds left at the right, in
 * place of the HUD, until it begins. */
void CG_PrematchInit(void);
/* The server's "start end" in server time, from setClientCvar cg_prematch. */
void CG_PrematchSetCountdown(const char *value);
/* Whether the viewer, playing, waits for the match to begin. */
int CG_PrematchActive(void);
/* Every frame: how gray the renderer draws the view. */
void CG_PrematchFrame(void);
void CG_DrawPrematch(void);

/* How much of the view's color the renderer takes away, 0 to 1. */
extern float g_webGrayFrac;

#endif
