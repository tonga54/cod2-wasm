#ifndef CG_AMMOCOUNTER_H
#define CG_AMMOCOUNTER_H

/* The WWII ammo counter in the bottom-right corner: the clip in large digits
 * beside the reserve, the frag and smoke grenades as icons, and a RELOAD, LOW
 * AMMO or NO AMMO hint under the crosshair. It stands in for the stock ammo
 * and offhand menu items and moves the weapon name and fire-mode icon clear of
 * itself. */
void CG_AmmoCounterInit(void);
int CG_AmmoCounterActive(void);
void CG_AmmoCounterPlaceWeaponName(rectDef_t *rect, float *scale);
void CG_AmmoCounterPlaceModeIcon(rectDef_t *rect);
void CG_DrawAmmoCounter(void);

#endif
