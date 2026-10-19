#ifndef CG_WEAPONCAMO_H
#define CG_WEAPONCAMO_H

/* The camouflage the server gave a client's class for one of its weapons,
 * from the client's CS_WEAPON_CAMOS configstring; 0 for none. */
int CG_GetClientWeaponCamo(int clientNum, int weapon);
/* Paints the weapon a scene entity holds, as R_AddRefEntityToScene returned
 * it, when it is clientNum's class weapon with a camouflage. */
void CG_SetWeaponCamo(GfxEntity *sceneEnt, const struct DObj_s *obj, int clientNum, int weapon);
/* Paints a gun on the ground in the camouflage it was dropped with. */
void CG_SetItemWeaponCamo(GfxEntity *sceneEnt, const struct DObj_s *obj, const entityState_t *es);

#endif
