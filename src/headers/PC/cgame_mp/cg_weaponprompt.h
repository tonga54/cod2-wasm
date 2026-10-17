#ifndef CG_WEAPONPROMPT_H
#define CG_WEAPONPROMPT_H

/* The WWII prompt for a weapon on the ground: the use key in gold, then "Press
 * to swap for" or "Press to pick up" and the weapon's name, with the weapon's
 * silhouette centred below. It stands in for the stock cursor hint of every
 * weapon except mounted guns. */
void CG_WeaponPromptInit(void);
int CG_WeaponPromptActive(void);
void CG_DrawWeaponPrompt(const rectDef_t *rect, int textStyle, int weapon, MaterialHandle icon, const vec_t *fadeColor);

#endif
