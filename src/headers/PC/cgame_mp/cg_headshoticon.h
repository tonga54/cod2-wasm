#ifndef CG_HEADSHOTICON_H
#define CG_HEADSHOTICON_H

/* The WWII headshot symbol: a skull inside a reticle ring with four ticks,
 * white on a clear border, so a tinted draw colours it. */
void CG_HeadshotIconInit(void);
/* diameter is the ring's outer diameter in virtual units, for which the image
 * is built once per cgame; a square diameter * *extent a side, centred on the
 * symbol, draws the ring that wide. Returns NULL when there is no image. */
MaterialHandle CG_HeadshotIconMaterial(float diameter, float *extent);
/* The symbol for an image of the caller's: size * size BGRA texels, white with
 * its coverage in alpha, size being 2 * (texels + 1) for a ring of radius
 * texels. */
void CG_HeadshotIconRaster(byte *bgra, int size, int texels);

#endif
