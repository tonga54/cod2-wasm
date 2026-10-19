#ifndef R_HUD2D_H
#define R_HUD2D_H

/* Screen-space triangles with per-vertex texture coordinates and colors, for
 * HUD elements that a stretched rectangle cannot draw. Positions are physical
 * pixels, like CL_DrawStretchPicPhysical. */
typedef struct HudVertex2D {
    float xy[2];
    float st[2];
    float rgba[4];
} HudVertex2D;

#define R_HUD2D_MAX_VERTICES 1024
#define R_HUD2D_MAX_INDICES 3072

/* Images the cgame builds at runtime, each drawn through a copy of $raw. */
#define R_HUD_IMAGE_MINIMAP 0
#define R_HUD_IMAGE_MINIMAP_ARROW 1
#define R_HUD_IMAGE_MINIMAP_DOT 2
#define R_HUD_IMAGE_NAME_FONT 3
#define R_HUD_IMAGE_MANTLE_ARROW 4
#define R_HUD_IMAGE_GRENADE_RING 5
#define R_HUD_IMAGE_GRENADE_POINTER 6
#define R_HUD_IMAGE_HEADSHOT 7
#define R_HUD_IMAGE_TEAM_EMBLEMS 8
#define R_HUD_IMAGE_KILLCAM 9
#define R_HUD_IMAGE_ENDGAME 10
#define R_HUD_IMAGE_MENU_BACKDROP 11
#define R_HUD_IMAGE_MENU_SHADE 12
#define R_HUD_IMAGE_MENU_PILL 13
#define R_HUD_IMAGE_MENU_CURSOR 14
#define R_HUD_IMAGE_PREMATCH 15
#define R_HUD_IMAGE_MENU_ARROW 16
#define R_HUD_IMAGE_MENU_ROUNDEL 17
#define R_HUD_IMAGE_MINIMAP_OVERVIEW 18
#define R_HUD_IMAGE_PAUSE_EMBLEMS 19
/* The lens of each scope overlay a match uses (cg_wwiiscope_mp.c), in four
 * slots from this one. */
#define R_HUD_IMAGE_SCOPE_LENS 20
#define R_HUD_IMAGE_SCOPE_LENS_COUNT 4
#define R_HUD_IMAGE_COUNT 24

/* Icons whose white silhouette or emblem has been built; each is read once. */
#define R_HUD_SILHOUETTE_COUNT 64
#define R_HUD_SILHOUETTE_MAX_SIZE 128

void R_AddCmdDrawTriangles2D(const HudVertex2D *verts, int vertexCount, const unsigned short *indices, int indexCount, MaterialHandle material);
/* bgra is width * height texels in memory order B, G, R, A. Returns NULL when
 * the image cannot be built; the caller keeps the stock HUD then. */
MaterialHandle R_CreateHudImageMaterial(int slot, const byte *bgra, int width, int height);
/* A white copy of a 2D icon that keeps only its alpha, trimmed to the visible
 * shape, so a tinted draw is a flat silhouette. aspect gets its width over its
 * height. Returns NULL, with aspect 0, unless the icon's image is a DXT3 or
 * DXT5 IWi of at most R_HUD_SILHOUETTE_MAX_SIZE texels a side. */
MaterialHandle R_GetHudSilhouetteMaterial(MaterialHandle icon, float *aspect);
/* A white copy of the whole icon whose opacity follows its brightness, for a
 * faint one-color emblem. Built under the same conditions as a silhouette. */
MaterialHandle R_GetHudEmblemMaterial(MaterialHandle icon);

/* Pictures in their own colors that the menus draw; each name is read once. */
#define R_HUD_PICTURE_COUNT 64
#define R_HUD_PICTURE_MAX_SIZE 1024
/* An A8R8G8B8 IWi with no picmip, sides of powers of two up to
 * R_HUD_PICTURE_MAX_SIZE and every mip down to 1 x 1, smallest first: 1 with
 * its size, else 0. */
int R_GetPictureSize(const byte *file, int fileLen, int *width, int *height);
/* images/<name>.iwi as R_GetPictureSize reads it, drawn through a copy of $raw
 * with its mips. st gets the texture coordinates of its visible texels, as
 * {s1, t1, s2, t2}, and aspect their width over their height. Returns NULL,
 * with aspect 0, when there is no such picture or nothing in it is visible. */
MaterialHandle R_GetHudPictureMaterial(const char *name, float *st, float *aspect);
/* The top mip of a DXT3 or DXT5 IWi file as white BGRA texels with its alpha. */
int R_DecodeIconAlpha(const byte *file, int fileLen, byte *bgra, int maxSize, int *width, int *height);
/* Images a gray copy is built from: DXT1, DXT3 or DXT5 IWis of at most this
 * many texels a side. */
#define R_HUD_GRAY_MAX_SIZE 2048
/* The top mip of a DXT1, DXT3 or DXT5 IWi file as opaque gray BGRA texels:
 * tone maps each texel's Rec. 709 luma to its gray. bgra holds width * height
 * texels, read from the file's header by R_GetGrayImageSize. */
int R_GetGrayImageSize(const byte *file, int fileLen, int *width, int *height);
int R_DecodeGrayImage(const byte *file, int fileLen, byte *bgra, const byte *tone);
/* An opaque gray copy of a 2D material's image through tone, built into slot.
 * aspect gets its width over its height. Returns NULL, with aspect 0, when the
 * image is not one R_DecodeGrayImage reads. */
MaterialHandle R_CreateHudGrayMaterial(int slot, MaterialHandle source, const byte *tone, float *aspect);
/* Lens images are at most this many texels a side. */
#define R_HUD_LENS_MAX_SIZE 512
/* The top mip of a DXT1, DXT3 or DXT5 IWi file of at most R_HUD_GRAY_MAX_SIZE
 * texels a side as BGRA texels in their own colors, halved until it fits
 * R_HUD_LENS_MAX_SIZE. width and height get the result's size. */
int R_DecodeLensImage(const byte *file, int fileLen, byte *bgra, int *width, int *height);
/* A scope overlay's lens without the black around it: within the circle
 * where the centred image first turns opaque it keeps the reticle, the dirt
 * and the darkening rim, and outside it is clear. Returns that radius as a
 * fraction of the image's half width. */
float R_ClearOutsideLens(byte *bgra, int width, int height);
/* R_ClearOutsideLens applied to a 2D material's image, built into slot. edge
 * gets the lens radius as a fraction of the half width, 0 on failure. */
MaterialHandle R_CreateHudLensMaterial(int slot, MaterialHandle overlay, float *edge);
void R_ShutdownHudImages(void);
/* Changes whenever R_ShutdownHudImages releases the images and the fonts
 * shut down with them, so materials and fonts kept from before are gone. */
int R_GetHudImageGeneration(void);
/* Bounds of the loaded world's drawn surfaces, without the sky. */
int R_GetWorldDrawBounds(vec_t *mins, vec_t *maxs);

/* Render command 6: the header is followed by vertexCount vertices, then
 * indexCount 16-bit indices, padded to 4 bytes. */
typedef struct GfxCmdDrawTriangles2D {
    GfxCmdHeader header;
    const Material *material;
    unsigned short vertexCount;
    unsigned short indexCount;
} GfxCmdDrawTriangles2D;

typedef struct GfxTriangles2DVertex {
    float xy[2];
    float st[2];
    GfxColor color;
} GfxTriangles2DVertex;

#endif
