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
#define R_HUD_IMAGE_COUNT 3

void R_AddCmdDrawTriangles2D(const HudVertex2D *verts, int vertexCount, const unsigned short *indices, int indexCount, MaterialHandle material);
/* bgra is width * height texels in memory order B, G, R, A. Returns NULL when
 * the image cannot be built; the caller keeps the stock HUD then. */
MaterialHandle R_CreateHudImageMaterial(int slot, const byte *bgra, int width, int height);
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
