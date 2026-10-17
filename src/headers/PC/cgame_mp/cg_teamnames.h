#ifndef CG_TEAMNAMES_H
#define CG_TEAMNAMES_H

/* The WWII label above a teammate: the name alone, without its color codes,
 * in a muted teal sans-serif (Barlow SemiBold, baked into cg_namefont_data.h). */
void CG_TeamNamesInit(void);
/* x and y are the centre of the baseline in center_safearea units. Returns 0,
 * having drawn nothing, when the stock label should be drawn instead. */
int CG_DrawTeammateName(const char *name, float x, float y);

#endif
