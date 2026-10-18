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
#define R_HUD_IMAGE_COUNT 10

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
/* The top mip of a DXT3 or DXT5 IWi file as white BGRA texels with its alpha. */
int R_DecodeIconAlpha(const byte *file, int fileLen, byte *bgra, int maxSize, int *width, int *height);
void R_ShutdownHudImages(void);
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
