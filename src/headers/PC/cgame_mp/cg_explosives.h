#ifndef CG_EXPLOSIVES_H
#define CG_EXPLOSIVES_H

/* Placed explosives on the HUD: the player's own and, in a team mode, their
 * teammates' mines and charges marked where they lie with the squad's emblem,
 * pulsing slowly (cg_hudExplosiveIcons), and G taking out or putting away a
 * carried charge's detonator. */
void CG_ExplosivesInit(void);
void CG_DrawExplosiveIcons(void);
void CG_UpdateDetonatorSelect(void);
/* The Allies' star and the Axis' cross, each in its ring, side by side in
 * size x size cells of B, G, R, A texels: white, with a dark edge. */
void CG_ExplosiveEmblemRaster(byte *bgra, int size);

#endif
