#ifndef CG_PLAYERNAMES_H
#define CG_PLAYERNAMES_H

/* The WWII labels above players: the name alone, without its color codes, in
 * a squared sans-serif (Barlow SemiBold, baked into cg_namefont_data.h), teal
 * above teammates and red above the enemy under the crosshair. x and y are the
 * centre of the baseline in center_safearea units. */
void CG_PlayerNamesInit(void);
/* Returns 0, having drawn nothing, when the stock label should be drawn. */
int CG_DrawTeammateName(const char *name, float x, float y);
/* CoD2 has no enemy label, so nothing stands in while this draws nothing. */
void CG_DrawEnemyName(const char *name, float x, float y, float alpha);

#endif
