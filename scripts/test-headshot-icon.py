#!/usr/bin/env python3
"""Run the production WWII headshot symbol: its image's shape against the model
fitted to the capture, its size for the screen, and when it is built."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/cgame_mp/cg_headshoticon_mp.c').read_text()
feed = (root / 'src/PC/cgame_mp/cg_killfeed_mp.c').read_text()
hud2d_header = (root / 'src/headers/PC/gfx_d3d/r_hud2d.h').read_text()
hud2d = (root / 'src/PC/gfx_d3d/r_hud2d.c').read_text()

# The kill feed starts it again every level; it has a HUD image slot of its own.
assert feed.count('#include "headers/PC/cgame_mp/cg_headshoticon.h"\n') == 1
assert feed.count('    CG_HeadshotIconInit();\n') == 1
slots = {name: int(re.search(r'^#define ' + name + r' (\d+)$', hud2d_header, re.M).group(1))
         for name in ('R_HUD_IMAGE_HEADSHOT', 'R_HUD_IMAGE_COUNT')}
names = re.findall(r'"([^"]+)"', re.search(r's_hudImageNames\[R_HUD_IMAGE_COUNT\] = \{(.*?)\};', hud2d, re.S).group(1))
assert len(names) == slots['R_HUD_IMAGE_COUNT'] and len(set(names)) == len(names)
assert names[slots['R_HUD_IMAGE_HEADSHOT']] == '*hud_headshot'

icon = source[source.index('/* The symbol on the WWII capture'):]

support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define R_HUD_IMAGE_HEADSHOT ''' + str(slots['R_HUD_IMAGE_HEADSHOT']) + r'''
typedef unsigned char byte;
typedef const char *MaterialHandle;
/* A screen screenHeight pixels tall: 480 virtual units to its height. */
static float screenHeight = 720;
static float GetRealHeightFromVirtualHeight(float height) { return height * screenHeight / 480.0f; }
static byte image[160 * 160 * 4];
static int imageSize, creates, failCreate;
static MaterialHandle R_CreateHudImageMaterial(int slot, const byte *bgra, int width, int height) {
    creates++;
    assert(slot == R_HUD_IMAGE_HEADSHOT && width == height && width > 0 && width * height * 4 <= (int)sizeof(image));
    memcpy(image, bgra, width * height * 4);
    imageSize = width;
    return failCreate ? NULL : "*hud_headshot";
}
'''

checks = r'''
static int texels;
static int near(float a, float b, float tolerance) { return fabsf(a - b) <= tolerance; }
/* The image's alpha at a point in radii of the ring from its centre, y down. */
static float alphaAt(float x, float y) {
    int tx = (int)floorf(x * texels + 0.5f * imageSize), ty = (int)floorf(y * texels + 0.5f * imageSize);
    return tx < 0 || ty < 0 || tx >= imageSize || ty >= imageSize ? 0 : image[(ty * imageSize + tx) * 4 + 3] / 255.0f;
}
/* The image covers as much of a rectangle, x0 to x1 across and y0 to y1 down,
 * as the model does, within 0.002 and 1%. */
static int covers(float x0, float x1, float y0, float y1, float model) {
    float sum = 0, step = 0.25f / (float)texels;
    for (float y = y0 + 0.5f * step; y < y1; y += step)
        for (float x = x0 + 0.5f * step; x < x1; x += step) sum += step * step * alphaAt(x, y);
    return fabsf(sum - model) <= 0.002f + 0.01f * model;
}
/* A new level at a screen height: the image, radius texels to the ring's
 * outside and a clear texel beyond, is built at the first draw. */
static void build(float height, int radius) {
    float extent = 0;
    CG_HeadshotIconInit();
    screenHeight = height;
    assert(CG_HeadshotIconMaterial(11.75f, &extent));
    texels = radius;
    assert(imageSize == 2 * radius + 2 && near(extent, (float)imageSize / (2.0f * radius), 1e-6f));
}

int main(void) {
    float extent = 0;

    /* Before the screen has a size there is no image; then it is built once,
     * a texel a pixel with the ring 11.75 units wide: 9 texels in radius at
     * 720 lines, in a 20-texel image. */
    CG_HeadshotIconInit();
    screenHeight = 0;
    assert(!CG_HeadshotIconMaterial(11.75f, &extent) && !creates && extent == 1.0f);
    screenHeight = 720;
    assert(!strcmp(CG_HeadshotIconMaterial(11.75f, &extent), "*hud_headshot") && creates == 1 && imageSize == 20);
    assert(near(extent, 20.0f / 18.0f, 1e-6f));
    screenHeight = 1080;
    assert(!strcmp(CG_HeadshotIconMaterial(11.75f, &extent), "*hud_headshot") && creates == 1 && imageSize == 20);
    /* A new level has none until it is built, then one for its screen, from 8
     * to 64 texels a radius. */
    CG_HeadshotIconInit();
    screenHeight = 0;
    assert(!CG_HeadshotIconMaterial(11.75f, &extent) && creates == 1 && extent == 1.0f);
    build(1080, 14); build(2160, 27); build(4320, 53); build(8640, 64); build(240, 8);
    assert(creates == 6);
    /* An image that cannot be made leaves none, without a try every frame,
     * until the next level. */
    failCreate = 1;
    CG_HeadshotIconInit();
    screenHeight = 720;
    assert(!CG_HeadshotIconMaterial(11.75f, &extent) && !CG_HeadshotIconMaterial(11.75f, &extent) && creates == 7);
    failCreate = 0;
    CG_HeadshotIconInit();
    assert(CG_HeadshotIconMaterial(11.75f, &extent) && creates == 8);

    /* The shape at 2160 lines, where the capture was taken, and at twice that:
     * white texels whose alpha is the model's coverage, mirror-symmetric on a
     * clear border, with the model's area, centre of area and spread, and its
     * features where the model has them. */
    for (int pass = 0; pass < 2; pass++) {
        double area = 0, sy = 0, syy = 0;
        int partial = 0;
        build(pass ? 4320.0f : 2160.0f, pass ? 53 : 27);
        for (int y = 0; y < imageSize; y++)
            for (int x = 0; x < imageSize; x++) {
                const byte *t = &image[(y * imageSize + x) * 4];
                double cy = (y + 0.5 - 0.5 * imageSize) / texels, a = t[3] / 255.0;
                assert(t[0] == 255 && t[1] == 255 && t[2] == 255);
                assert(t[3] == image[(y * imageSize + imageSize - 1 - x) * 4 + 3]);
                if (x == 0 || y == 0 || x == imageSize - 1 || y == imageSize - 1) assert(!t[3]);
                partial += t[3] > 0 && t[3] < 255;
                area += a; sy += a * cy; syy += a * cy * cy;
            }
        sy /= area; syy = syy / area - sy * sy; area /= (double)texels * texels;
        assert(fabs(area - 1.749) < 0.01 && fabs(sy + 0.0454) < 0.003 && fabs(syy - 0.2668) < 0.003 && partial > 100);
        /* The ring's arcs between the ticks, and the ticks narrowing inward:
         * the top one to a point that meets the cranium, the bottom one to a
         * point short of the chin, the sides nearly even to the cheeks. */
        assert(covers(0.5f, 1.05f, -1.05f, -0.5f, 0.05491f) && covers(-1.05f, -0.5f, 0.5f, 1.05f, 0.05491f));
        assert(covers(-0.2f, 0.2f, -0.82f, -0.66f, 0.01777f) && covers(-0.2f, 0.2f, 0.6f, 0.82f, 0.01973f));
        assert(covers(0.6f, 0.82f, -0.2f, 0.2f, 0.03258f) && covers(-0.82f, -0.6f, -0.2f, 0.2f, 0.03258f));
        /* The cranium and its top. */
        assert(covers(-0.6f, 0.6f, -0.62f, -0.1f, 0.44803f) && covers(-0.6f, 0.6f, -0.66f, -0.5f, 0.08238f));
        /* The slanted eyes: the bridge between them and the cheeks outside,
         * whose tops meet the side ticks. */
        assert(covers(-0.12f, 0.12f, -0.05f, 0.25f, 0.0593f) && covers(0.12f, 0.6f, -0.05f, 0.25f, 0.05439f));
        assert(covers(-0.6f, 0.6f, -0.1f, 0.05f, 0.11315f));
        assert(alphaAt(0.287f, 0.1f) == 0 && alphaAt(-0.287f, 0.1f) == 0 && alphaAt(0.1f, 0.06f) == 1);
        /* The nose, the jaw and the hollows beside it under the cheekbones;
         * the chin clear of the bottom tick. */
        assert(covers(-0.2f, 0.2f, 0.18f, 0.4f, 0.04733f) && covers(-0.6f, 0.6f, 0.4f, 0.6f, 0.06978f));
        assert(covers(0.3f, 0.62f, 0.25f, 0.45f, 0.01315f));
        assert(alphaAt(0.0f, 0.33f) == 0 && alphaAt(0.02f, 0.6f) == 0 && alphaAt(0.62f, -0.45f) == 0);
    }
    return 0;
}
'''

mutants = [
    [('#define HEADSHOT_ICON_RING_INNER 0.866f', '#define HEADSHOT_ICON_RING_INNER 0.85f')],
    [('#define HEADSHOT_ICON_CRANIUM_Y -0.314f', '#define HEADSHOT_ICON_CRANIUM_Y -0.29f')],
    [('#define HEADSHOT_ICON_CRANIUM_HALF_WIDTH 0.484f', '#define HEADSHOT_ICON_CRANIUM_HALF_WIDTH 0.46f')],
    [('#define HEADSHOT_ICON_CRANIUM_HALF_HEIGHT 0.314f', '#define HEADSHOT_ICON_CRANIUM_HALF_HEIGHT 0.29f')],
    [('#define HEADSHOT_ICON_CRANIUM_POWER 2.39f', '#define HEADSHOT_ICON_CRANIUM_POWER 2.0f')],
    [('#define HEADSHOT_ICON_EYE_X 0.287f', '#define HEADSHOT_ICON_EYE_X 0.26f')],
    [('#define HEADSHOT_ICON_EYE_Y 0.1f', '#define HEADSHOT_ICON_EYE_Y 0.13f')],
    [('#define HEADSHOT_ICON_EYE_LENGTH 0.234f', '#define HEADSHOT_ICON_EYE_LENGTH 0.2f')],
    [('#define HEADSHOT_ICON_EYE_WIDTH 0.112f', '#define HEADSHOT_ICON_EYE_WIDTH 0.09f')],
    [('#define HEADSHOT_ICON_EYE_COS 0.896f', '#define HEADSHOT_ICON_EYE_COS 1.0f'),
     ('#define HEADSHOT_ICON_EYE_SIN 0.445f', '#define HEADSHOT_ICON_EYE_SIN 0.0f')],
    [('along = (dx * HEADSHOT_ICON_EYE_COS - dy * HEADSHOT_ICON_EYE_SIN)',
      'along = (dx * HEADSHOT_ICON_EYE_COS + dy * HEADSHOT_ICON_EYE_SIN)'),
     ('across = (dx * HEADSHOT_ICON_EYE_SIN + dy * HEADSHOT_ICON_EYE_COS)',
      'across = (dy * HEADSHOT_ICON_EYE_COS - dx * HEADSHOT_ICON_EYE_SIN)')],
    [('#define HEADSHOT_ICON_NOSE_TOP 0.191f', '#define HEADSHOT_ICON_NOSE_TOP 0.22f')],
    [('#define HEADSHOT_ICON_NOSE_BOTTOM 0.382f', '#define HEADSHOT_ICON_NOSE_BOTTOM 0.35f')],
    [('#define HEADSHOT_ICON_NOSE_HALF_WIDTH 0.139f', '#define HEADSHOT_ICON_NOSE_HALF_WIDTH 0.1f')],
    [('#define HEADSHOT_ICON_SAMPLES 4', '#define HEADSHOT_ICON_SAMPLES 1')],
    [('#define HEADSHOT_ICON_TEXELS_MIN 8', '#define HEADSHOT_ICON_TEXELS_MIN 4')],
    [('#define HEADSHOT_ICON_TEXELS_MAX 64', '#define HEADSHOT_ICON_TEXELS_MAX 96')],
    [('s_headshotIconSide = { 0.565f, 0.087f, 0.062f }', 's_headshotIconSide = { 0.6f, 0.087f, 0.062f }')],
    [('s_headshotIconSide = { 0.565f, 0.087f, 0.062f }', 's_headshotIconSide = { 0.565f, 0.11f, 0.062f }')],
    [('s_headshotIconTop = { 0.637f, 0.11f, 0.011f }', 's_headshotIconTop = { 0.6f, 0.11f, 0.011f }')],
    [('s_headshotIconTop = { 0.637f, 0.11f, 0.011f }', 's_headshotIconTop = { 0.637f, 0.14f, 0.011f }')],
    [('s_headshotIconBottom = { 0.63f, 0.129f, 0.0f }', 's_headshotIconBottom = { 0.6f, 0.129f, 0.0f }')],
    [('s_headshotIconBottom = { 0.63f, 0.129f, 0.0f }', 's_headshotIconBottom = { 0.63f, 0.1f, 0.0f }')],
    [('{ -0.08f, 0.503f }', '{ -0.08f, 0.45f }')],
    [('{ 0.028f, 0.543f }', '{ 0.028f, 0.48f }')],
    [('{ 0.137f, 0.558f }', '{ 0.137f, 0.5f }')],
    [('{ 0.353f, 0.314f }', '{ 0.353f, 0.36f }')],
    [('{ 0.57f, 0.087f }', '{ 0.62f, 0.087f }')],
    [('    float ax = fabsf(x);\n', '    float ax = x;\n')],
    [('if (r2 >= HEADSHOT_ICON_RING_INNER * HEADSHOT_ICON_RING_INNER)', 'if (r2 >= HEADSHOT_ICON_RING_INNER)')],
    [('    f = (along - tick->end) / (HEADSHOT_ICON_RING_INNER - tick->end);\n', '    f = 1.0f;\n')],
    [('if (y >= a[0] && y <= b[0])', 'if (y <= b[0])')],
    [('if (y >= HEADSHOT_ICON_NOSE_TOP && y <= HEADSHOT_ICON_NOSE_BOTTOM &&', 'if (y >= HEADSHOT_ICON_NOSE_TOP &&')],
    [('            texel[1] = 255;\n', '            texel[1] = 250;\n')],
    [('texel[3] = (byte)(covered * 255 /', 'texel[3] = (byte)(covered * 240 /')],
    [('if (!s_headshotIconTried && radius > 0.0f)', 'if (!s_headshotIconTried)')],
    [('        s_headshotIconTried = 1;\n', '')],
    [('    s_headshotIconTried = 0;\n', '')],
    [('    s_headshotIcon = NULL;\n', '')],
    [('    s_headshotIconExtent = 1.0f;\n', '')],
    [('texels = (int)ceilf(radius);', 'texels = (int)radius;')],
    [('size = 2 * (texels + 1);', 'size = 2 * texels;')],
    [('s_headshotIconExtent = (float)size / (float)(2 * texels);', 's_headshotIconExtent = 1.0f;')],
]

variants = [icon]
for pairs in mutants:
    variant = icon
    for old, new in pairs:
        assert variant.count(old) == 1, old
        variant = variant.replace(old, new)
    variants.append(variant)

with tempfile.TemporaryDirectory(prefix='cod2-headshot-icon-') as directory:
    path = Path(directory) / 'icon.c'
    binary = Path(directory) / 'icon'
    for index, variant in enumerate(variants):
        path.write_text(support + variant + checks)
        subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-Wall', '-Werror' if index == 0 else '-w', str(path), '-lm', '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr[-2000:])
print('PASS: the WWII headshot symbol is built once per level at the first draw after the screen has a size, a '
      'texel per pixel of its ring\'s radius from 8 to 64 (9 at 720 lines), with a clear texel around it, and not '
      'retried until the next level when it cannot be made; at the capture\'s 2160 lines and at twice that it is '
      'white and mirror-symmetric with the fitted model\'s area, centre of area, spread, ring, ticks, cranium, '
      f'slanted eyes, nose, jaw and chin; {len(mutants)} failing regression mutants')
