/* Exercise the renderer implementation itself, with guard bytes after each
 * requested light capacity. Link with --gc-sections to omit unrelated code. */
#ifndef LIGHT_SOURCE
#define LIGHT_SOURCE "../src/PC/gfx_d3d/rb_light.c"
#endif
#include LIGHT_SOURCE
#include <math.h>

r_global_permanent_t rgp;
static float intensity[3] = {0.3f, 0.6f, 0.1f};
void *imp_vec3_colorintensity = intensity;

float Vec3Normalize(vec3_t v)
{
    float length = sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    if (length > 0)
        for (int i = 0; i < 3; ++i) v[i] /= length;
    return length;
}

int main(void)
{
    static GfxWorld world;
    static MaterialTechniqueSet set;
    static Material material;
    static MaterialTechnique technique;
    vec4_t colors[6];
    D3DLIGHT9 storage[10];
    unsigned seed = 17;
    rgp.world = &world;
    material.techniqueSet = &set;
    world.sunLight.color[0] = 0.8f;
    world.sunLight.position[2] = 1.0f;
    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 3; ++j)
            gridBasisDirs[i][j] = (i & (1 << j)) ? 0.57735f : -0.57735f;

    for (int branch = 0; branch < 2; ++branch) {
        set.techniques[15] = branch ? &technique : NULL;
        *((unsigned char *)&technique + 0xe) = 1;
        for (int capacity = 0; capacity <= 8; ++capacity) {
            for (int sample = 0; sample < 256; ++sample) {
                for (int i = 0; i < 24; ++i) {
                    seed = seed * 1664525u + 1013904223u;
                    ((float *)colors)[i] = sample ? (seed >> 16) / 16384.0f : 1.0f;
                }
                memset(storage, 0xa5, sizeof(storage));
                int count = RB_DeriveEntityLights(colors, 0.7f, &material, storage, capacity);
                if (count < 0 || count > capacity) {
                    fprintf(stderr, "invalid light count %d for capacity %d\n", count, capacity);
                    return 1;
                }
                for (unsigned i = capacity * sizeof(*storage); i < sizeof(storage); ++i) {
                    if (((unsigned char *)storage)[i] != 0xa5) {
                        fprintf(stderr, "light overflow: branch=%d capacity=%d sample=%d offset=%u\n",
                                branch, capacity, sample, i);
                        return 1;
                    }
                }
            }
        }
    }
    puts("Light bounds: 4608 cases passed (both paths, capacities 0..8).");
    return 0;
}
