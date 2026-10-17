#ifndef CG_MINIMAP_H
#define CG_MINIMAP_H

/* A top-down map of the level, built from collision traces when the map
 * loads, drawn in place of the compass. */
void CG_MinimapInit(const char *mapName);
int CG_MinimapIsActive(void);
void CG_DrawMinimap(float alpha);

/* The top-left HUD column (clock, scores, kill feed, chat) moves below the
 * minimap while it is on screen. */
void CG_MinimapSetHudVisible(int visible);
float CG_MinimapTopLeftOffset(void);
float CG_MinimapHudElemOffset(int alignScreen, float x);

#endif
