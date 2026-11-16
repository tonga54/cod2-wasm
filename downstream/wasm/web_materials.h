#ifndef WEB_MATERIALS_H
#define WEB_MATERIALS_H

typedef struct {
    GfxImage *normal;
    GfxImage *specular;
    GfxImage *detail;
    float detailScale[2];
} WebMaterialMaps;

void WebMaterials_Select(const Material *material, int normalMaps, int specularMaps, WebMaterialMaps *maps);
int WebMaterials_IsImpact(const Material *material);
float WebMaterials_MasonryRoughness(const Material *material);
void WebMaterials_SetSun(const GfxWorld *world);

#endif
