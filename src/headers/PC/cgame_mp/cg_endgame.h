#ifndef CG_ENDGAME_H
#define CG_ENDGAME_H

/* The WWII end of a match. Over the darkened view it first announces who
 * won, the viewer's victory or defeat, both sides with their emblems and
 * scores, and why the match ended; then the scoreboard takes over with that
 * result in its header, over a blurred view, until the map changes. */
void CG_EndgameInit(void);
/* A game message as the server sent it, before translation: the stock
 * scripts print the limit that ended the match. */
void CG_EndgameMessage(const char *message);
/* Every frame, whether the server has the viewer at intermission. */
void CG_EndgameFrame(int intermission);
/* Draws the end of the match where the scoreboard goes; 0 outside it. */
int CG_DrawEndgame(float fade);

/* How much the renderer blurs the view behind the scoreboard, 0 to 1. */
extern float g_webBackdropFrac;

#endif
