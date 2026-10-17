#ifndef CG_HEADSHOTICON_H
#define CG_HEADSHOTICON_H

/* The WWII headshot symbol: a skull inside a reticle ring with four ticks,
 * white on a clear border, so a tinted draw colours it. */
void CG_HeadshotIconInit(void);
/* diameter is the ring's outer diameter in virtual units, for which the image
 * is built once per cgame; a square diameter * *extent a side, centred on the
 * symbol, draws the ring that wide. Returns NULL when there is no image. */
MaterialHandle CG_HeadshotIconMaterial(float diameter, float *extent);

#endif
