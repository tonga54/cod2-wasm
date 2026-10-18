#ifndef CG_HITMARKER_H
#define CG_HITMARKER_H

/* WWII's hit marker over the gametype scripts' damage_feedback element: a
 * thin white X for a hit, a longer red one for a kill. */
void CG_HitMarkerInit(void);
/* Every obituary once, as the snapshot that brings it arrives: attacker is -1
 * without a killer. */
void CG_HitMarkerObituary(int attacker, int target);
/* Up to four lines in physical pixels, each x0, y0, x1, y1, width pixels
 * across, in one draw. Their ends fade out over a pixel and their sides over
 * feather pixels, at most one, centered on their edges, so they keep their
 * width and length at any angle. A level line a whole number of pixels wide
 * whose sides fall between pixels draws them solid with a half-pixel feather,
 * even with the 2D projection's sub-pixel offset. */
void CG_DrawHudLines(const float lines[][4], int count, float width, float feather, const vec_t *color);

#endif
