"""A stand-in for R_AddCmdDrawTriangles2D that rasterizes like the GPU, for the C HUD tests."""

# Each triangle is blended over a window of a transparent canvas, sampling
# the interpolated vertex colors at pixel centers. Samples on an edge go to
# one side only, as with the top-left fill rule: it is the same as sampling
# just off the pixel center, so triangles sharing edges and vertices cover
# every sample once. The canvas keeps the blended color premultiplied, so a
# pixel's color is its rgb over its alpha. rasterShift moves every vertex
# like the sub-pixel offset MacOpenGLUtils_ConvertD3DProjectionMatrixToOpenGL
# adds to RB_Set2D's projection: (-0.075, -0.0375) pixels, or (-0.15, -0.15)
# on card type 2. MaterialHandle is the including test's.
RASTER_C = r'''
#define RASTER_SIZE 320
static float raster[RASTER_SIZE][RASTER_SIZE][4];
static float rasterLeft, rasterTop, rasterShift[2];
static int rasterCalls, rasterTriangles;
static MaterialHandle rasterMaterial;
static void rasterClear(float left, float top) {
    memset(raster, 0, sizeof(raster));
    rasterLeft = left;
    rasterTop = top;
    rasterCalls = rasterTriangles = 0;
    memset(&rasterMaterial, 0, sizeof(rasterMaterial));
}
static double rasterEdge(const float *a, const float *b, double x, double y) {
    return ((double)b[0] - a[0]) * (y - a[1]) - ((double)b[1] - a[1]) * (x - a[0]);
}
static int rasterOwns(const float *a, const float *b) {
    double dx = (double)b[0] - a[0], dy = (double)b[1] - a[1];
    return dy > 0 || (dy == 0 && dx < 0);
}
static int rasterInside(double w, const float *a, const float *b) {
    return w > 0 || (w == 0 && rasterOwns(a, b));
}
void R_AddCmdDrawTriangles2D(const HudVertex2D *verts, int vertexCount, const unsigned short *indices,
                             int indexCount, MaterialHandle material) {
    assert(vertexCount >= 3 && vertexCount <= R_HUD2D_MAX_VERTICES);
    assert(indexCount >= 3 && indexCount <= R_HUD2D_MAX_INDICES && indexCount % 3 == 0);
    rasterCalls++;
    rasterMaterial = material;
    for (int t = 0; t < indexCount; t += 3) {
        HudVertex2D shifted[3];
        const HudVertex2D *v[3];
        for (int k = 0; k < 3; k++) {
            assert(indices[t + k] < vertexCount);
            shifted[k] = verts[indices[t + k]];
            shifted[k].xy[0] += rasterShift[0];
            shifted[k].xy[1] += rasterShift[1];
            v[k] = &shifted[k];
        }
        double area = rasterEdge(v[0]->xy, v[1]->xy, v[2]->xy[0], v[2]->xy[1]);
        if (area == 0)
            continue;
        if (area < 0) {
            const HudVertex2D *swap = v[1];
            v[1] = v[2];
            v[2] = swap;
            area = -area;
        }
        rasterTriangles++;
        float lo[2], hi[2];
        for (int axis = 0; axis < 2; axis++) {
            lo[axis] = fminf(v[0]->xy[axis], fminf(v[1]->xy[axis], v[2]->xy[axis]));
            hi[axis] = fmaxf(v[0]->xy[axis], fmaxf(v[1]->xy[axis], v[2]->xy[axis]));
        }
        int x0 = (int)floorf(lo[0] - rasterLeft) - 1, x1 = (int)ceilf(hi[0] - rasterLeft) + 1;
        int y0 = (int)floorf(lo[1] - rasterTop) - 1, y1 = (int)ceilf(hi[1] - rasterTop) + 1;
        assert(x0 >= 0 && y0 >= 0 && x1 <= RASTER_SIZE && y1 <= RASTER_SIZE);
        for (int py = y0; py < y1; py++) {
            for (int px = x0; px < x1; px++) {
                double x = rasterLeft + px + 0.5, y = rasterTop + py + 0.5;
                double w0 = rasterEdge(v[1]->xy, v[2]->xy, x, y);
                double w1 = rasterEdge(v[2]->xy, v[0]->xy, x, y);
                double w2 = rasterEdge(v[0]->xy, v[1]->xy, x, y);
                if (!rasterInside(w0, v[1]->xy, v[2]->xy) || !rasterInside(w1, v[2]->xy, v[0]->xy) ||
                    !rasterInside(w2, v[0]->xy, v[1]->xy))
                    continue;
                float color[4];
                for (int k = 0; k < 4; k++)
                    color[k] = (float)((w0 * v[0]->rgba[k] + w1 * v[1]->rgba[k] + w2 * v[2]->rgba[k]) / area);
                float *pixel = raster[py][px];
                for (int k = 0; k < 3; k++)
                    pixel[k] = color[k] * color[3] + pixel[k] * (1 - color[3]);
                pixel[3] = color[3] + pixel[3] * (1 - color[3]);
            }
        }
    }
}
/* The canvas pixel whose center is at physical (x, y), or NULL off it. */
static const float *rasterAt(float x, float y) {
    int px = (int)floorf(x - rasterLeft), py = (int)floorf(y - rasterTop);
    if (px < 0 || py < 0 || px >= RASTER_SIZE || py >= RASTER_SIZE)
        return 0;
    return raster[py][px];
}
'''


def hud_vertex_definition(root):
    """The HudVertex2D typedef and the 2D batch limits from r_hud2d.h."""
    source = (root / 'src/headers/PC/gfx_d3d/r_hud2d.h').read_text()
    start = source.index('typedef struct HudVertex2D {')
    end = source.index('} HudVertex2D;', start) + len('} HudVertex2D;')
    limits = [line for line in source.splitlines() if line.startswith('#define R_HUD2D_MAX_')]
    assert len(limits) == 2, limits
    return source[start:end] + '\n' + '\n'.join(limits) + '\n'
