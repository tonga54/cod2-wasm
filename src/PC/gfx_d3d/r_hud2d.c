#include "common_types.h"
#include "imports.h"
#include "headers/PC/gfx_d3d/r_hud2d.h"
#include <string.h>

extern r_global_permanent_t rgp;
extern void Image_Generate2D(GfxImage *image, byte *pixels, int width, int height, int imageFormat);
extern void Image_Release(GfxImage *image);
extern void RB_UnbindImage(const GfxImage *image);
extern void ClearBounds(vec3_t mins, vec3_t maxs);
extern void ExpandBounds(vec3_t mins, vec3_t maxs, vec3_t boundsMin, vec3_t boundsMax);

/* Linear filtering, no mipmaps, clamped in U and V (see RB_SetSampler). */
#define HUD_IMAGE_SAMPLER_STATE 0x32

static const char *const s_hudImageNames[R_HUD_IMAGE_COUNT] = {
    "*hud_minimap",
    "*hud_minimap_arrow",
    "*hud_minimap_dot",
};

static GfxImage s_hudImages[R_HUD_IMAGE_COUNT];
static MaterialTextureDef s_hudTextures[R_HUD_IMAGE_COUNT];
static Material s_hudMaterials[R_HUD_IMAGE_COUNT];

static void R_ReleaseHudImage(int slot)
{
    GfxImage *image = &s_hudImages[slot];

    if (image->texture.map) {
        RB_UnbindImage(image);
        Image_Release(image);
    }
    memset(&s_hudMaterials[slot], 0, sizeof(s_hudMaterials[slot]));
}

MaterialHandle R_CreateHudImageMaterial(int slot, const byte *bgra, int width, int height)
{
    const Material *raw = rgp.rawMaterial;
    GfxImage *image;

    if (slot < 0 || slot >= R_HUD_IMAGE_COUNT || !bgra || width <= 0 || height <= 0)
        return NULL;
    /* $raw is the alpha-blended 2D material; without it the copy would draw
     * with the wrong technique. */
    if (!raw || raw == rgp.defaultMaterial || raw->textureCount != 1 || !raw->textures)
        return NULL;

    R_ReleaseHudImage(slot);
    image = &s_hudImages[slot];
    memset(image, 0, sizeof(*image));
    image->name = s_hudImageNames[slot];
    Image_Generate2D(image, (byte *)bgra, width, height, D3DFMT_A8R8G8B8);
    if (!image->texture.map)
        return NULL;

    s_hudTextures[slot] = raw->textures[0];
    s_hudTextures[slot].samplerState = HUD_IMAGE_SAMPLER_STATE;
    s_hudTextures[slot].u.image = image;
    s_hudMaterials[slot] = *raw;
    s_hudMaterials[slot].info.name = s_hudImageNames[slot] + 1;
    s_hudMaterials[slot].textures = &s_hudTextures[slot];
    return &s_hudMaterials[slot];
}

void R_ShutdownHudImages(void)
{
    int slot;

    for (slot = 0; slot < R_HUD_IMAGE_COUNT; slot++)
        R_ReleaseHudImage(slot);
}

int R_GetWorldDrawBounds(vec_t *mins, vec_t *maxs)
{
    const GfxWorld *world = rgp.world;
    int i;

    ClearBounds(mins, maxs);
    if (!world)
        return 0;

    for (i = 0; i < world->surfaceCount; i++) {
        const GfxSurface *surf = &world->surfaces[i];

        if (!surf->tris || !surf->material || (surf->material->info.gameFlags & 8))
            continue;
        ExpandBounds(surf->tris->bounds[0], surf->tris->bounds[1], mins, maxs);
    }
    return mins[0] <= maxs[0] && mins[1] <= maxs[1] && mins[2] <= maxs[2];
}
