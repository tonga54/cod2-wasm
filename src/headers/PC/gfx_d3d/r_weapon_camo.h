#ifndef R_WEAPON_CAMO_H
#define R_WEAPON_CAMO_H

/* Camouflage on weapon models and in the class screens. The patterns, the
 * paint mask of each gun material and the screens' pictures are built on
 * first use and released with the renderer. */

#define R_WEAPON_CAMO_SIZE 256
/* Pattern tiles across one unit of a gun's texture coordinates, and across
 * a menu picture: a primary's, or a pistol's smaller silhouette. */
#define R_WEAPON_CAMO_TILES 2.5f
#define R_WEAPON_CAMO_PICTURE_TILES 2.6f
#define R_WEAPON_CAMO_SILHOUETTE_TILES 1.6f
/* The frame's painted entities; one a player, the view weapon, a killcam. */
#define R_WEAPON_CAMO_TAGS 128

typedef struct {
    GfxImage *pattern;
    GfxImage *mask;
    float tiles;
    /* The median brightness of the material's painted texels: its grain and
     * wear shade the paint relative to it. */
    float reference;
} R_WeaponCamoDraw;

/* The pattern as a 2D material for the screens' swatches, or NULL. */
MaterialHandle R_GetWeaponCamoMaterial(int camo);
/* A copy of a gun's menu picture wearing the camouflage: the retail weapon
 * image, or a HUD icon's silhouette filled with the pattern and cropped as
 * R_GetHudSilhouetteMaterial crops it. aspect gets its width over its
 * height. NULL, with aspect 0, when the picture cannot be read. */
MaterialHandle R_GetWeaponCamoPicture(MaterialHandle picture, int silhouette, int camo, int style, float *aspect);
/* Paints the surfaces of ent, as R_AddRefEntityToScene returned it, that
 * use one of obj's materials named materialName, for this frame. */
void R_SetEntityWeaponCamo(GfxEntity *ent, const struct DObj_s *obj, const char *materialName, int camo, int style);
/* For the backend's draw of material on ent: 1, with what it takes, when
 * the frame painted it. */
int R_GetDrawWeaponCamo(const GfxEntity *ent, const Material *material, R_WeaponCamoDraw *draw);
/* With the frame's entities, after the backend has drawn them. */
void R_ClearWeaponCamoEntities(void);
void R_ShutdownWeaponCamos(void);

#endif
