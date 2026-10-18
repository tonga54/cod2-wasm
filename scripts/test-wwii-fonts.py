#!/usr/bin/env python3
"""Run the production WWII fonts behind the real R_RegisterFont: which stock
fonts they replace, their sizes against the stock fonts', the glyph table the
engine searches, the atlas and its mipmaps, and when the stock fonts stay."""
from pathlib import Path
import hashlib
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/gfx_d3d/r_font_wwii.c').read_text()
header = (root / 'src/headers/PC/gfx_d3d/r_font_wwii.h').read_text()
fonts = (root / 'src/PC/gfx_d3d/r_font.c').read_text()
data = (root / 'src/headers/PC/gfx_d3d/r_font_wwii_data.h').read_text()
static_tests = (root / 'scripts/test-static.sh').read_text()


def function(text, name):
    match = re.search(r'^((?:static )?(?:inline )?[^\n;]*\b' + name + r'\([^;]*?\))\s*\{', text, re.M)
    assert match, name
    start = match.start()
    # Declarations split over lines: take the return type above the name.
    while start > 0:
        previous = text.rfind('\n', 0, start - 1) + 1
        line = text[previous:start - 1]
        if not line.strip() or line.rstrip().endswith((';', '}', '*/')) or line.startswith('#'):
            break
        start = previous
    brace = text.index('{', match.end() - 1)
    depth, end = 1, brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end] + '\n'


# Every registration tries the WWII font first and every shutdown drops it.
assert fonts.count('#include "headers/PC/gfx_d3d/r_font_wwii.h"\n') == 1
assert fonts.count('    font = R_LoadWwiiFont(fontName);\n    if (!font)\n        font = R_LoadFont(fontName, imageTrack);\n') == 1
assert fonts.count('    registeredFontCount = 0;\n    R_ShutdownWwiiFonts();\n') == 1
assert 'FontHandle R_LoadWwiiFont(const char *fontName);' in header and 'void R_ShutdownWwiiFonts(void);' in header
assert 'FontHandle R_RegisterWwiiFont(const char *fontName);' in header
assert static_tests.count('python3 "${repo_root}/scripts/test-wwii-fonts.py"') == 1

# The embedded faces are the committed fonts, under their license.
for weight in ('Medium', 'SemiBold', 'Bold', 'Regular'):
    ttf = root / f'third_party/barlow/Barlow-{weight}.ttf'
    assert f' *   Barlow-{weight}.ttf, SHA-256 {hashlib.sha256(ttf.read_bytes()).hexdigest()}.' in data, weight
    assert re.search(r'static const unsigned char r_wwiiFont' + weight + r'\[\d+\] = \{', data), weight
assert 'SIL OPEN FONT LICENSE Version 1.1' in (root / 'third_party/barlow/OFL.txt').read_text()
stb = (root / 'third_party/stb/stb_truetype.h').read_text()
assert stb.startswith('// stb_truetype.h - v1.26 - public domain') and 'ALTERNATIVE A - MIT License' in stb

includes = ('#include "common_types.h"\n#include "imports.h"\n#include "headers/PC/gfx_d3d/r_font_wwii.h"\n'
            '#include "headers/PC/gfx_d3d/r_font_wwii_data.h"\n')
assert source.startswith(includes)
module = source.replace(includes, f'#include "{root}/src/headers/PC/gfx_d3d/r_font_wwii_data.h"\n')
assert module.count('#include "../../../third_party/stb/stb_truetype.h"') == 1
module = module.replace('#include "../../../third_party/stb/stb_truetype.h"', f'#include "{root}/third_party/stb/stb_truetype.h"')

registry = ('static int registeredFontCount;\nstatic Font *registeredFont[16];\n' +
            function(fonts, 'R_GetCharacterGlyph_inl') + function(fonts, 'R_GetCharacterGlyph') +
            function(fonts, 'R_RegisterFont') + function(fonts, 'R_RegisterWwiiFont') + function(fonts, 'R_ShutdownFonts') +
            function(fonts, 'R_NormalizedTextScale') + function(fonts, 'R_TextWidth_inl') + function(fonts, 'R_TextWidth'))

support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
typedef unsigned char byte;
typedef unsigned int DWORD;
typedef int D3DFORMAT;
typedef int D3DCUBEMAP_FACES;
enum { D3DFMT_A8R8G8B8 = 21, D3DFMT_A8L8 = 51 };
typedef struct { struct { int enabled; } current; } dvar_t;
typedef struct GfxImage { const char *name; struct { void *map; } texture; int serial; } GfxImage;
typedef struct { byte samplerState; union { GfxImage *image; } u; } MaterialTextureDef;
typedef struct Material { struct { const char *name; } info; int stateBits[2]; unsigned short textureCount;
                          MaterialTextureDef *textures; } Material;
typedef const Material *MaterialHandle;
typedef struct { unsigned short letter; signed char x0, y0; byte dx, pixelWidth, pixelHeight; float s0, t0, s1, t1; } Glyph;
typedef struct Font_s { const char *name; int pixelHeight; int glyphCount; MaterialHandle material; Glyph *glyphs; } Font;
typedef Font *FontHandle;
typedef struct { const Material *rawMaterial, *defaultMaterial; } r_global_permanent_t;
typedef struct { const dvar_t *(*Dvar_RegisterBool)(const char *name, int value, int flags);
                 unsigned int (*SEH_ReadCharFromString)(const char **text, int *punctuation); } refimport_t;
typedef struct { int width, height; } vidConfig_t;
#define __attribute_regparm__(n)

static MaterialTextureDef rawTexture = { 0x32, { 0 } };
static Material rawMaterial = { { "$raw" }, { 7, 9 }, 1, &rawTexture }, defaultMaterial = { { "$default" }, { 0, 0 }, 1, &rawTexture };
r_global_permanent_t rgp = { &rawMaterial, &defaultMaterial };
static dvar_t fontsDvar = { { 1 } };
static int dvarRegistrations;
static const dvar_t *RegisterBool(const char *name, int value, int flags) {
    assert(!strcmp(name, "r_wwiiFonts") && value == 1 && flags == 0x2021);
    dvarRegistrations++;
    return &fontsDvar;
}
static unsigned int ReadChar(const char **text, int *punctuation) { (void)punctuation; return (unsigned char)*(*text)++; }
refimport_t ri = { RegisterBool, ReadChar };
vidConfig_t vidConfig = { 1280, 720 };
int I_stricmp(const char *a, const char *b) { return strcasecmp(a, b); }

/* Images: the setup's size and flags, and every mipmap's alpha under a white luminance. */
#define MAX_IMAGES 32
#define MAX_LEVELS 13
typedef struct { GfxImage *image; int width, height, flags, format, released; byte *levels[MAX_LEVELS]; int levelCount; } Upload;
static Upload uploads[MAX_IMAGES];
static int uploadCount, setups, releases, unbinds, failSetup, serials;
static byte imageBacking;
static Upload *uploadFor(const GfxImage *image) {
    for (int i = uploadCount - 1; i >= 0; i--) if (uploads[i].image == image && !uploads[i].released) return &uploads[i];
    return NULL;
}
void Image_Setup(GfxImage *image, int width, int height, int depth, int imageFlags, DWORD usage, D3DFORMAT imageFormat) {
    assert(depth == 1 && usage == 0 && uploadCount < MAX_IMAGES && image->name && image->name[0] == '*');
    setups++;
    if (failSetup) { image->texture.map = NULL; return; }
    image->texture.map = &imageBacking;
    image->serial = ++serials;
    uploads[uploadCount++] = (Upload){ image, width, height, imageFlags, imageFormat, 0, { 0 }, 0 };
}
D3DCUBEMAP_FACES Image_CubemapFace(int face) { assert(face == 0); return 0; }
void Image_UploadData(GfxImage *image, D3DFORMAT format, int face, int mipLevel, const byte *src) {
    Upload *u = uploadFor(image);
    int w, h;
    assert(u && format == D3DFMT_A8L8 && face == 0 && mipLevel == u->levelCount && mipLevel < MAX_LEVELS);
    w = u->width >> mipLevel; h = u->height >> mipLevel; if (w < 1) w = 1; if (h < 1) h = 1;
    u->levels[mipLevel] = malloc((size_t)w * h);
    for (int i = 0; i < w * h; i++) {
        assert(src[i * 2] == 255);
        u->levels[mipLevel][i] = src[i * 2 + 1];
    }
    u->levelCount++;
}
void RB_UnbindImage(const GfxImage *image) { assert(uploadFor(image)); unbinds++; }
void Image_Release(GfxImage *image) {
    Upload *u = uploadFor(image);
    assert(u && image->texture.map);
    u->released = 1; releases++; image->texture.map = NULL;
}
static int stockLoads;
static Font stockFont = { "stock", 16, 0, NULL, NULL };
static FontHandle R_LoadFont(const char *fontName, int imageTrack) { (void)fontName; (void)imageTrack; stockLoads++; return &stockFont; }
static void R_Error(int code, const char *fmt, ...) { (void)code; (void)fmt; abort(); }
'''

checks = r'''
static int near(double a, double b, double tolerance) { return fabs(a - b) <= tolerance; }
static int texels(const Font *font) {
    const Upload *u = uploadFor(font->material->textures[0].u.image);
    const Glyph *H = &font->glyphs['H' - 0x20];
    return (int)lround((H->s1 - H->s0) * u->width / H->pixelWidth);
}
static const Upload *atlas(const Font *font) { return uploadFor(font->material->textures[0].u.image); }
static float alphaAt(const Font *font, const Glyph *g, int tx, int ty) {
    const Upload *u = atlas(font);
    int x = (int)lround(g->s0 * u->width) + tx, y = (int)lround(g->t0 * u->height) + ty;
    return u->levels[0][y * u->width + x] / 255.0f;
}
/* The ink's top and bottom in font units from the pen, for a letter whose
 * rows are full or empty but at its edges ("I", "z"): each edge row counts by
 * its coverage over a full row's. */
static double fullRow;
static void inkSpan(const Font *font, unsigned letter, double *top, double *bottom) {
    const Glyph *g = R_GetCharacterGlyph((FontHandle)font, letter);
    int t = texels(font), rows = g->pixelHeight * t, first = -1, last = -1;
    double sums[1024], full = 0;
    assert(rows < 1024);
    for (int y = 0; y < rows; y++) {
        sums[y] = 0;
        for (int x = 0; x < g->pixelWidth * t; x++) sums[y] += alphaAt(font, g, x, y);
        if (sums[y] > full) full = sums[y];
    }
    for (int y = 0; y < rows; y++) if (sums[y] > 0.01 * full) { if (first < 0) first = y; last = y; }
    assert(first >= 0 && last > first);
    *top = g->y0 + (first + 1 - fmin(1.0, sums[first] / full)) / t;
    *bottom = g->y0 + (last + fmin(1.0, sums[last] / full)) / t;
    fullRow = full / t;
}
/* An "I"'s stem in font units: the coverage of a full row. */
static double stemWidth(const Font *font) {
    double top, bottom;
    inkSpan(font, 'I', &top, &bottom);
    return fullRow;
}
static double capHeight(const Font *font, unsigned letter) {
    double top, bottom;
    inkSpan(font, letter, &top, &bottom);
    return bottom - top;
}
static double inkBottom(const Font *font, unsigned letter) {
    double top, bottom;
    inkSpan(font, letter, &top, &bottom);
    return bottom;
}
static int advance(const Font *font, const char *text) { return R_TextWidth(text, 0, (FontHandle)font); }

typedef struct { const char *name; int pixelHeight; double cap; int baseline; double width, stem; } Expect;
/* The stock fonts' units: their pixel heights, the coverage height of their
 * "H" and where it ends above the pen. The width is "The quick brown fox
 * jumps over the lazy dog" in advances over the cap height, and the stem an
 * "I"'s over its height: Barlow SemiBold at 95% 26.33 and 0.157, Bold 28.05
 * and 0.201, Medium 27.45 and 0.137, Regular 27.09 and 0.101 (Pillow,
 * Barlow's own metrics). Regular has no stock font: the killcam's. */
#define FONT_CASES 10
static const Expect expect[FONT_CASES] = {
    { "fonts/smallFont", 12, 7.0, 2, 26.33, 0.157 }, { "fonts/normalFont", 16, 11.0, 2, 26.33, 0.157 },
    { "fonts/bigFont", 24, 13.12, 5, 26.33, 0.157 }, { "fonts/extraBigFont", 32, 21.0, 5, 26.33, 0.157 },
    { "fonts/boldFont", 30, 17.1, 6, 28.05, 0.201 }, { "fonts/consoleFont", 16, 9.0, 3, 27.45, 0.137 },
    { "fonts/smallDevFont", 16, 9.17, 2, 26.33, 0.157 }, { "fonts/bigDevFont", 24, 13.12, 3, 26.33, 0.157 },
    { "fonts/qerFont", 12, 7.0, 3, 26.33, 0.157 }, { "$regularfont", 32, 21.0, 5, 27.09, 0.101 },
};
static const char *sample = "The quick brown fox jumps over the lazy dog";

static void reset(int height) {
    R_ShutdownFonts();
    for (int i = 0; i < uploadCount; i++) {
        assert(uploads[i].released);
        for (int l = 0; l < uploads[i].levelCount; l++) free(uploads[i].levels[l]);
    }
    uploadCount = 0;
    vidConfig.height = height; vidConfig.width = height * 16 / 9;
}

int main(void) {
    int cases = 0;

    /* The setting registers on the first font and is off-able. */
    reset(720);
    FontHandle font = R_RegisterFont("$font", 0);
    assert(dvarRegistrations == 1 && font != &stockFont && !stockLoads);

    /* Every stock font, by its UI alias or its file in any case. */
    static const char *const aliases[][2] = {
        { "$font", "fonts/normalFont" }, { "$smallfont", "fonts/smallFont" }, { "$bigfont", "fonts/bigFont" },
        { "$extrabigfont", "fonts/extraBigFont" }, { "$boldfont", "fonts/boldFont" },
        { "$consolefont", "fonts/consoleFont" }, { "$smalldevfont", "fonts/smallDevFont" },
        { "$bigdevfont", "FONTS/BIGDEVFONT" }, { "$qerfont", "fonts/qerfont" }, { "$regularfont", "fonts/regularFont" },
    };
    for (int i = 0; i < 10; i++) {
        reset(720);
        Font *a = R_RegisterFont(aliases[i][0], 0), *b = R_RegisterFont(aliases[i][1], 0);
        assert(a != &stockFont && b != &stockFont && a != b && !stockLoads);
        assert(!strcmp(a->name, aliases[i][0]) && !strcmp(b->name, aliases[i][1]));
        assert(a->pixelHeight == b->pixelHeight && a->glyphCount == 219);
        /* The console's own font is drawn a unit to a pixel, the UI's alias
         * scaled; the others share one face. */
        if (i == 5) assert(a->glyphs != b->glyphs && texels(a) == 2 && texels(b) == 1);
        else assert(a->glyphs == b->glyphs && a->material == b->material);
        assert(R_RegisterFont(aliases[i][0], 0) == a);
        cases++;
    }
    /* The registry finds a name in any case, as the renderer's debug font
     * and the cgame's dev font are the same. */
    reset(720);
    font = R_RegisterFont("fonts/smalldevfont", 1);
    assert(font != &stockFont && R_RegisterFont("fonts/smallDevFont", 1) == font && R_RegisterFont("FONTS/SMALLDEVFONT", 1) == font);
    /* Anything else is left to the stock loader. */
    static const char *const others[] = { "fonts/objectiveFont", "fonts/", "$", "normalFont", "gfx/fonts/normalFont",
                                          "fonts/normalFontX", "", "font", "sound/normalFont" };
    for (int i = 0; i < 9; i++) {
        reset(720);
        stockLoads = 0;
        assert(R_RegisterFont(others[i], 0) == &stockFont && stockLoads == 1);
        cases++;
    }

    /* Each keeps its stock font's units: the pixel height, an "H" as tall as
     * the stock one ending where it does, and Barlow's widths at 95% for the
     * shared face. Text width is the advances' sum. */
    for (int h = 0; h < 4; h++) {
        int height = (int[]){ 480, 720, 1080, 2160 }[h];
        for (int i = 0; i < FONT_CASES; i++) {
            reset(height);
            const Font *f = R_RegisterFont(expect[i].name, 0);
            int t = texels(f), sum = 0;
            assert(f->pixelHeight == expect[i].pixelHeight);
            /* Barlow's I rounds its corners, which takes ink off its end
             * rows: the thinnest, Regular's, measures 0.085 short. */
            assert(near(capHeight(f, 'I'), expect[i].cap, 0.1));
            assert(near(inkBottom(f, 'I'), -expect[i].baseline, 0.08));
            assert(near(stemWidth(f) / expect[i].cap, expect[i].stem, 0.005));
            for (const char *c = sample; *c; c++) sum += f->glyphs[*c - 0x20].dx;
            assert(advance(f, sample) == sum);
            /* Advances round to whole units, which moves a line up to 3%. */
            assert(near(sum / expect[i].cap, expect[i].width, expect[i].width * 0.03));
            /* Lowercase is Barlow's, its x-height 0.73 of the capitals, on the
             * same baseline. */
            assert(near(capHeight(f, 'z') / capHeight(f, 'I'), 0.73, 0.03));
            assert(near(inkBottom(f, 'z'), -expect[i].baseline, 0.08));
            assert(near(R_NormalizedTextScale((FontHandle)f, 0.3f), 48.0f * 0.3f / expect[i].pixelHeight, 1e-6));
            /* Texels to a unit follow the screen's lines over 480, up to 3;
             * the console's own font stays at one. */
            int want = i == 5 ? 1 : (int[]){ 1, 2, 2, 3 }[h];
            assert(t == want);
            cases++;
        }
    }

    /* The table R_GetCharacterGlyph searches: 0x20 to 0x7f in order, then
     * the rest of Windows-1252 ascending; other letters draw a full stop. */
    reset(1080);
    font = R_RegisterFont("$font", 0);
    {
        int n = 0;
        for (int letter = 0x20; letter < 0x100; letter++) {
            int defined = letter < 0x80 || letter >= 0xa0 ||
                          !(letter == 0x81 || letter == 0x8d || letter == 0x8f || letter == 0x90 || letter == 0x9d);
            if (!defined) { assert(R_GetCharacterGlyph(font, letter) == &font->glyphs[14]); continue; }
            assert(font->glyphs[n].letter == letter && R_GetCharacterGlyph(font, letter) == &font->glyphs[n]);
            n++;
        }
        assert(n == 219 && font->glyphs[14].letter == '.');
        assert(R_GetCharacterGlyph(font, 0x100) == &font->glyphs[14] && R_GetCharacterGlyph(font, 0x2014) == &font->glyphs[14]);
        /* DEL and the no-break space are spaces; the euro sign, the dashes,
         * the curly quotes and the accents have ink. */
        const Glyph *space = R_GetCharacterGlyph(font, ' ');
        assert(!space->pixelWidth && space->dx > 0 && R_GetCharacterGlyph(font, 0x7f)->dx == space->dx &&
               R_GetCharacterGlyph(font, 0xa0)->dx == space->dx && !R_GetCharacterGlyph(font, 0xa0)->pixelWidth);
        static const unsigned inked[] = { 0x80, 0x85, 0x91, 0x93, 0x96, 0x97, 0x99, 0xa9, 0xc9, 0xd1, 0xe7, 0xfc, 0xff };
        for (int i = 0; i < 13; i++) assert(R_GetCharacterGlyph(font, inked[i])->pixelWidth > 0);
        assert(R_GetCharacterGlyph(font, 0x97)->dx > R_GetCharacterGlyph(font, 0x96)->dx);
        {
            double top, bottom, capTop, capBottom;
            inkSpan(font, 0xc9, &top, &bottom);
            inkSpan(font, 'I', &capTop, &capBottom);
            assert(top < capTop - 1.0 && near(bottom, capBottom, 0.1));
        }
    }

    /* The atlas: a power-of-two width and whole 4-texel rows, each glyph's
     * texels two apart from the next, a clear texel inside its quad, no ink
     * anywhere else, and a texel to a unit for every quad. */
    for (int i = 0; i < FONT_CASES; i++) {
        reset(1440);
        const Font *f = R_RegisterFont(expect[i].name, 0);
        const Upload *u = atlas(f);
        int t = texels(f);
        unsigned short *owner = calloc((size_t)u->width * u->height, sizeof(*owner));
        assert((u->width & (u->width - 1)) == 0 && u->width >= 64 && u->height % 4 == 0 && u->height <= 2048);
        assert(u->flags == 1 && u->format == D3DFMT_A8L8);
        assert(f->material->textures[0].samplerState == 0x3a && f->material->textureCount == 1);
        assert(f->material->stateBits[0] == 7 && f->material->stateBits[1] == 9);
        assert(!strncmp(f->material->info.name, "wwii_", 5) && f->material->textures[0].u.image->name[0] == '*');
        for (int pass = 0; pass < 2; pass++)
            for (int g = 0; g < 219; g++) {
                const Glyph *gl = &f->glyphs[g];
                int x0 = (int)lround(gl->s0 * u->width), y0 = (int)lround(gl->t0 * u->height);
                int x1 = (int)lround(gl->s1 * u->width), y1 = (int)lround(gl->t1 * u->height);
                if (!gl->pixelWidth) { assert(!gl->pixelHeight); continue; }
                assert(near(gl->s0 * u->width, x0, 1e-3) && near(gl->t1 * u->height, y1, 1e-3));
                assert(x1 - x0 == gl->pixelWidth * t && y1 - y0 == gl->pixelHeight * t);
                assert(x0 >= 2 && y0 >= 2 && x1 <= u->width - 2 && y1 <= u->height - 2);
                for (int y = y0 - 2; y < y1 + 2; y++)
                    for (int x = x0 - 2; x < x1 + 2; x++) {
                        int inside = x >= x0 && x < x1 && y >= y0 && y < y1;
                        unsigned short *o = &owner[y * u->width + x];
                        if (pass == 0) {
                            if (inside) { assert(!*o); *o = (unsigned short)(g + 1); }
                        } else {
                            assert(inside ? *o == g + 1 : !*o);
                            if (!inside || x == x0 || y == y0 || x == x1 - 1 || y == y1 - 1)
                                assert(u->levels[0][y * u->width + x] == 0);
                        }
                    }
            }
        for (int p = 0; p < u->width * u->height; p++) if (!owner[p]) assert(u->levels[0][p] == 0);
        free(owner);
        /* Mipmaps down to a texel, each the rounded mean of four. */
        {
            int w = u->width, h = u->height, levels = 1;
            while (w > 1 || h > 1) { w = w > 1 ? w / 2 : 1; h = h > 1 ? h / 2 : 1; levels++; }
            assert(u->levelCount == levels);
            for (int l = 1; l < levels; l++) {
                int pw = u->width >> (l - 1), ph = u->height >> (l - 1), cw = pw > 1 ? pw / 2 : 1, ch = ph > 1 ? ph / 2 : 1;
                if (pw < 1) pw = 1; if (ph < 1) ph = 1;
                for (int y = 0; y < ch; y++)
                    for (int x = 0; x < cw; x++) {
                        int ax = pw > 1 ? 2 * x : x, ay = ph > 1 ? 2 * y : y, bx = pw > 1 ? ax + 1 : ax, by = ph > 1 ? ay + 1 : ay;
                        const byte *p = u->levels[l - 1];
                        assert(u->levels[l][y * cw + x] == (p[ay * pw + ax] + p[ay * pw + bx] + p[by * pw + ax] + p[by * pw + bx] + 2) / 4);
                    }
            }
        }
        cases++;
    }

    /* Advances, quads and metrics do not depend on the screen; only the
     * texels do. */
    {
        reset(720);
        const Font *a = R_RegisterFont("$bigfont", 0);
        Glyph small[219];
        memcpy(small, a->glyphs, sizeof(small));
        reset(2160);
        const Font *b = R_RegisterFont("$bigfont", 0);
        for (int g = 0; g < 219; g++) assert(small[g].dx == b->glyphs[g].dx && small[g].letter == b->glyphs[g].letter);
        assert(texels(b) == 3);
    }

    /* Fonts that differ only in their baseline share one atlas: the big
     * dev font's glyphs are the big font's, two units lower down. Every
     * glyph's texels are its quad's in units, wherever the atlas sits. */
    reset(1080);
    setups = 0;
    {
        const Font *big = R_RegisterFont("$bigfont", 0), *dev = R_RegisterFont("fonts/bigDevFont", 0);
        assert(setups == 1 && big->material == dev->material && big->glyphs != dev->glyphs);
        for (int g = 0; g < 219; g++) {
            assert(big->glyphs[g].dx == dev->glyphs[g].dx && big->glyphs[g].s0 == dev->glyphs[g].s0 &&
                   big->glyphs[g].pixelHeight == dev->glyphs[g].pixelHeight);
            if (big->glyphs[g].pixelHeight) assert(dev->glyphs[g].y0 == big->glyphs[g].y0 + 2);
        }
        const Font *small = R_RegisterFont("$smallfont", 0), *normal = R_RegisterFont("$font", 0);
        assert(setups == 3 && small->material != normal->material);
    }

    /* A shutdown releases every image and the next registration builds
     * anew; one face serves every name of its font. */
    reset(1080);
    setups = releases = unbinds = 0;
    R_RegisterFont("$font", 0); R_RegisterFont("fonts/normalFont", 0); R_RegisterFont("$bigfont", 0);
    R_RegisterFont("fonts/consoleFont", 0); R_RegisterFont("$consolefont", 0); R_RegisterFont("fonts/bigDevFont", 0);
    assert(setups == 4);
    R_ShutdownFonts();
    assert(releases == 4 && unbinds == 4);
    font = R_RegisterFont("$font", 0);
    assert(setups == 5 && font != &stockFont && dvarRegistrations >= 2);

    /* The stock fonts stay without $raw, with the default material in its
     * place, when the image cannot be made, with the setting off, and once
     * sixteen names are taken. */
    reset(720);
    stockLoads = 0;
    rgp.rawMaterial = NULL;
    assert(R_RegisterFont("$font", 0) == &stockFont && stockLoads == 1);
    rgp.rawMaterial = &defaultMaterial;
    assert(R_RegisterFont("$smallfont", 0) == &stockFont && stockLoads == 2);
    rgp.rawMaterial = &rawMaterial;
    reset(720);
    failSetup = 1; setups = releases = 0;
    assert(R_RegisterFont("$font", 0) == &stockFont && stockLoads == 3 && setups == 1 && !releases);
    failSetup = 0;
    assert(R_RegisterFont("fonts/normalFont", 0) != &stockFont && setups == 2);
    reset(720);
    fontsDvar.current.enabled = 0;
    assert(R_RegisterFont("$font", 0) == &stockFont && stockLoads == 4);
    fontsDvar.current.enabled = 1;
    {
        static const char *const names[] = { "$font", "fonts/normalFont", "FONTS/NORMALFONT", "Fonts/NormalFont",
                                             "$smallfont", "fonts/smallFont", "$bigfont", "fonts/bigFont",
                                             "$extrabigfont", "fonts/extraBigFont", "$boldfont", "fonts/boldFont",
                                             "$consolefont", "fonts/consoleFont", "fonts/smallDevFont", "fonts/bigDevFont" };
        reset(720);
        for (int i = 0; i < 16; i++) assert(R_LoadWwiiFont(names[i]) != NULL);
        assert(R_LoadWwiiFont("fonts/qerFont") == NULL);
    }

    /* A font only the WWII lettering has registers once under any case, and
     * is NULL, without the stock loader or an error, with the setting off,
     * for a name no WWII font has, and with sixteen fonts registered. */
    reset(720);
    stockLoads = 0;
    font = R_RegisterWwiiFont("$regularfont");
    assert(font && font != &stockFont && font->pixelHeight == 32 && !stockLoads);
    assert(R_RegisterWwiiFont("$REGULARFONT") == font && R_RegisterFont("$regularfont", 0) == font);
    reset(720);
    fontsDvar.current.enabled = 0;
    assert(R_RegisterWwiiFont("$regularfont") == NULL && !stockLoads);
    fontsDvar.current.enabled = 1;
    assert(R_RegisterWwiiFont("fonts/objectiveFont") == NULL && !stockLoads);
    reset(720);
    for (int i = 0; i < 16; i++) R_RegisterFont("fonts/objectiveFont", 0);
    assert(stockLoads == 16 && R_RegisterWwiiFont("$regularfont") == NULL);
    printf("%d\n", cases);
    return 0;
}
'''

mutants = [
    ('    { "normalFont", R_WWII_SEMIBOLD, 16, 11.0f, 2, 0.95f, 0 },', '    { "normalFont", R_WWII_SEMIBOLD, 16, 10.0f, 2, 0.95f, 0 },'),
    ('    { "smallFont", R_WWII_SEMIBOLD, 12, 7.0f, 2, 0.95f, 0 },', '    { "smallFont", R_WWII_SEMIBOLD, 12, 7.0f, 3, 0.95f, 0 },'),
    ('    { "bigFont", R_WWII_SEMIBOLD, 24, 13.12f, 5, 0.95f, 0 },', '    { "bigFont", R_WWII_SEMIBOLD, 24, 13.12f, 5, 1.0f, 0 },'),
    ('    { "extraBigFont", R_WWII_SEMIBOLD, 32, 21.0f, 5, 0.95f, 0 },', '    { "extraBigFont", R_WWII_SEMIBOLD, 30, 21.0f, 5, 0.95f, 0 },'),
    ('    { "boldFont", R_WWII_BOLD, 30, 17.1f, 6, 1.0f, 0 },', '    { "boldFont", R_WWII_SEMIBOLD, 30, 17.1f, 6, 1.0f, 0 },'),
    ('    { "consoleFont", R_WWII_MEDIUM, 16, 9.0f, 3, 1.0f, 1 },', '    { "consoleFont", R_WWII_MEDIUM, 16, 9.0f, 3, 1.0f, 0 },'),
    ('    { "smallDevFont", R_WWII_SEMIBOLD, 16, 9.17f, 2, 0.95f, 0 },', '    { "smallDevFont", R_WWII_SEMIBOLD, 16, 9.17f, 3, 0.95f, 0 },'),
    ('    { "bigDevFont", R_WWII_SEMIBOLD, 24, 13.12f, 3, 0.95f, 0 },', ''),
    ('#define R_WWII_FONT_TEXELS_MAX 3', '#define R_WWII_FONT_TEXELS_MAX 4'),
    ('#define R_WWII_FONT_MARGIN 1', '#define R_WWII_FONT_MARGIN 0'),
    ('#define R_WWII_FONT_GAP 2', '#define R_WWII_FONT_GAP 1'),
    ('#define R_WWII_FONT_IMAGE_FLAGS 1', '#define R_WWII_FONT_IMAGE_FLAGS 3'),
    ('#define R_WWII_FONT_SAMPLER_STATE 0x3a', '#define R_WWII_FONT_SAMPLER_STATE 0x32'),
    ('#define R_WWII_FONT_NAMES 16', '#define R_WWII_FONT_NAMES 17'),
    ('    if (!I_stricmp(fontName, "$font")) {\n        file = "normalFont";', '    if (0) {\n        file = "normalFont";'),
    ('        if (I_stricmp(prefix, "fonts/"))\n            return NULL;\n', ''),
    ('    if (letter == 0x7f || letter == 0xa0)\n        return 0x20;\n', ''),
    ('0x20ac, 0, 0x201a,', '0x20ac, 0x20ac, 0x201a,'),
    ('0x2013, 0x2014,', '0x2014, 0x2013,'),
    ('    texels = (int)floorf((float)vidConfig.height / 480.0f + 0.5f);', '    texels = (int)floorf((float)vidConfig.height / 480.0f);'),
    ('    rawPixels = profile->rawPixels && fontName[0] != \'$\';', '    rawPixels = profile->rawPixels;'),
    ('        int y0 = face->glyphs[i].y0 - profile->baseline;', '        int y0 = face->glyphs[i].y0 + profile->baseline;'),
    ('        dx = (int)floorf((float)advance * advanceScale + 0.5f);', '        dx = (int)floorf((float)advance * advanceScale);'),
    ('    units = profile->capHeight / (float)boxY1;', '    units = profile->capHeight / 700.0f * 1.02f;'),
    ('alpha[y1 * width + x0] + alpha[y1 * width + x1] + 2) / 4);', 'alpha[y1 * width + x0] + alpha[y1 * width + x1]) / 4);'),
    ('        if (face->profile == profile && face->atlas->texelsPerUnit == texels)',
     '        if (face->profile == profile)'),
    ('        if (atlas->face == profile->face && atlas->capHeight == profile->capHeight &&',
     '        if (atlas->face == profile->face &&'),
    ('        int y0 = face->glyphs[i].y0 - profile->baseline;', '        int y0 = face->glyphs[i].y0;'),
    ('    s_wwiiFontNameCount = 0;\n    r_wwiiFonts = NULL;', '    r_wwiiFonts = NULL;'),
    ('    for (i = 0; i < s_wwiiFontAtlasCount; i++)\n        R_WwiiFontReleaseAtlas(&s_wwiiFontAtlases[i]);\n', ''),
    ('    if (!raw || raw == rgp.defaultMaterial || raw->textureCount != 1 || !raw->textures)\n        return NULL;\n    return raw;',
     '    if (!raw)\n        return NULL;\n    return raw;'),
    ('r_wwiiFonts = ri.Dvar_RegisterBool("r_wwiiFonts", 1, 0x2021);', 'r_wwiiFonts = ri.Dvar_RegisterBool("r_wwiiFonts", 1, 0x2001);'),
    ('    if (!fontName || !r_wwiiFonts || !r_wwiiFonts->current.enabled || s_wwiiFontNameCount == R_WWII_FONT_NAMES)',
     '    if (!fontName || !r_wwiiFonts || s_wwiiFontNameCount == R_WWII_FONT_NAMES)'),
    ('    atlas->texture.samplerState = R_WWII_FONT_SAMPLER_STATE;\n', ''),
    ('#define R_WWII_FONT_ATLAS_ROW 4', '#define R_WWII_FONT_ATLAS_ROW 3'),
    ('            texels[i * 2] = 255;', '            texels[i * 2] = 0;'),
]

registry_mutants = [
    ('        if (I_stricmp(fontName, *(const char **)registeredFont[fontIndex]) == 0)\n'
     '            return registeredFont[fontIndex];\n    }\n    if (registeredFontCount > 15)\n        return 0;\n',
     '    }\n    if (registeredFontCount > 15)\n        return 0;\n'),
    ('    if (registeredFontCount > 15)\n        return 0;\n', ''),
    ('    if (!font)\n        return 0;\n', '    if (!font)\n        font = R_LoadFont(fontName, 0);\n'),
]

variants = [(module, registry)]
for old, new in mutants:
    assert module.count(old) == 1, old
    variants.append((module.replace(old, new), registry))
for old, new in registry_mutants:
    assert registry.count(old) == 1, old
    variants.append((module, registry.replace(old, new)))

with tempfile.TemporaryDirectory(prefix='cod2-wwii-fonts-') as directory:
    path = Path(directory) / 'fonts.c'
    binary = Path(directory) / 'fonts'
    for index, (variant, fonts_registry) in enumerate(variants):
        path.write_text(support + variant + fonts_registry + checks)
        flags = ['-Wall', '-Werror', '-Wno-unused-function', '-Wno-unused-but-set-variable', '-Wno-misleading-indentation'] if index == 0 else ['-w']
        subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all']
                       + flags + [str(path), '-lm', '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr[-2000:])
        if index == 0:
            cases = int(result.stdout)
print('PASS: every stock font (normal, small, big, extra big, bold, console, both dev fonts and qer, by their UI aliases '
      'or their files in any case) registers as Barlow rasterized from the embedded faces: the size-picked UI fonts '
      'share SemiBold at 95% width, bold text is Bold, the console Medium and the killcam\'s own font Regular, which '
      'registers optionally (NULL without the setting, a WWII font or a free slot); each keeps its stock pixel height, an H as '
      'tall as the stock one ending at its baseline, and Barlow\'s widths; the glyph table is the one R_GetCharacterGlyph '
      'searches; atlases are packed without overlap on clear borders with exact mipmaps, one texel per unit per 480 '
      'lines up to 3 and one for the console\'s own font; images are released on shutdown and rebuilt; without $raw, an '
      f'image, the setting or a free name the stock fonts stay; {cases} font and size cases; '
      f'{len(mutants) + len(registry_mutants)} failing regression mutants')
