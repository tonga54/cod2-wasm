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
/* Every frame, whether the server has the viewer at intermission. It also
 * takes the backdrop a menu asked for while the last frame was painted. */
void CG_EndgameFrame(int intermission);
/* Draws the end of the match where the scoreboard goes; 0 outside it. */
int CG_DrawEndgame(float fade);

/* How much the renderer blurs the view behind the scoreboard or a menu, 0
 * to 1, and how much of its color the view behind a menu loses. */
extern float g_webBackdropFrac;
extern float g_webBackdropGray;
/* Set while a menu that wants the match blurred and gray behind it paints,
 * as the class screens do; the frame after takes it. */
extern float g_webMenuBackdrop;

#endif
