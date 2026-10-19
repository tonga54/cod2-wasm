#!/usr/bin/env python3
"""Exercise the real reflex and lens sight reticles drawn over the aim."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
sight_path = root / 'src/PC/cgame_mp/cg_attachmentsight_mp.c'

support = r'''
#include <assert.h>
#include <math.h>
#include <string.h>
typedef float vec_t;
typedef vec_t vec4_t[4];
typedef int MaterialHandle;
typedef struct { float fWeaponPosFrac; unsigned attachments; } playerState_t;
typedef struct HudVertex2D { float xy[2]; float st[2]; float rgba[4]; } HudVertex2D;
enum { ATT_REFLEX, ATT_LENS, ATT_OPTIC4X, ATT_QUICKDRAW, ATT_GRIP };
#define ATT_BIT(a) (1u << (a))
static struct { struct { MaterialHandle whiteMaterial; } media; } cgsStorage = { { 77 } }, *cgs = &cgsStorage;

static unsigned BG_GetWeaponAttachments(const playerState_t *ps, int weapon)
{
    assert(weapon == 3);
    return ps->attachments;
}
/* A 1.5 scale with an offset, as a wide screen places the centre. */
static void CalcScreenPlacement(float *x, float *y, float *w, float *h, int horzAlign, int vertAlign)
{
    assert(horzAlign == 2 && vertAlign == 2 && *w == 1.0f && *h == 1.0f);
    *x = *x * 1.5f + 100.0f;
    *y = *y * 1.5f + 10.0f;
    *w *= 1.5f;
    *h *= 1.5f;
}
typedef struct {
    HudVertex2D verts[128];
    unsigned short indices[512];
    int vertexCount, indexCount;
    MaterialHandle material;
} Draw;
static Draw draws[4];
static int drawCount;
static void R_AddCmdDrawTriangles2D(const HudVertex2D *verts, int vertexCount, const unsigned short *indices,
                                    int indexCount, MaterialHandle material)
{
    assert(drawCount < 4 && vertexCount <= 128 && indexCount <= 512);
    memcpy(draws[drawCount].verts, verts, sizeof(*verts) * vertexCount);
    memcpy(draws[drawCount].indices, indices, sizeof(*indices) * indexCount);
    draws[drawCount].vertexCount = vertexCount;
    draws[drawCount].indexCount = indexCount;
    draws[drawCount].material = material;
    drawCount++;
}
'''

checks = r'''
#define NEAR(a, b) (fabsf((a) - (b)) <= 1e-4f * (1.0f + fabsf(b)))
#define CX 580.0f
#define CY 370.0f

static int Sight(unsigned attachments, float frac)
{
    playerState_t ps = { frac, attachments };

    drawCount = 0;
    CG_DrawAttachmentSight(&ps, 3, 320.0f, 240.0f);
    return drawCount;
}

/* Every triangle joins three distinct vertices, all wind the same way, and
 * together they cover the polygon between the inner and outer radii. */
static void Mesh(const Draw *d, float inner, float outer)
{
    const float polygon = 12.0f * sinf(6.28318531f / 24.0f);
    float area = 0.0f;
    int i, sign = 0;

    assert(d->material == 77 && d->indexCount % 3 == 0);
    for (i = 0; i < d->indexCount; i += 3) {
        const HudVertex2D *a = &d->verts[d->indices[i]], *b = &d->verts[d->indices[i + 1]], *c = &d->verts[d->indices[i + 2]];
        float cross;

        assert(d->indices[i] < d->vertexCount && d->indices[i + 1] < d->vertexCount && d->indices[i + 2] < d->vertexCount);
        cross = (b->xy[0] - a->xy[0]) * (c->xy[1] - a->xy[1]) - (b->xy[1] - a->xy[1]) * (c->xy[0] - a->xy[0]);
        assert(fabsf(cross) > 1e-6f);
        if (!sign)
            sign = cross > 0.0f ? 1 : -1;
        assert((cross > 0.0f ? 1 : -1) == sign);
        area += 0.5f * fabsf(cross);
    }
    assert(NEAR(area, polygon * (outer * outer - inner * inner)));
}

/* The vertices from first, a ring of 24 at the radius with the color's alpha
 * times the fade and the feather. */
static void Ring(const Draw *d, int first, float radius, const float *color, float alpha)
{
    int i;

    for (i = first; i < first + 24; i++) {
        const HudVertex2D *v = &d->verts[i];

        assert(NEAR(hypotf(v->xy[0] - CX, v->xy[1] - CY), radius));
        assert(v->rgba[0] == color[0] && v->rgba[1] == color[1] && v->rgba[2] == color[2]);
        assert(NEAR(v->rgba[3], color[3] * alpha) && v->st[0] == 0.5f && v->st[1] == 0.5f);
    }
}

int main(void)
{
    static const float red[4] = { 1.0f, 0.16f, 0.1f, 0.95f }, amber[4] = { 1.0f, 0.78f, 0.32f, 0.9f };
    const Draw *d;

    /* Nothing without a reflex or lens sight, nor before the last of the aim-in. */
    assert(!Sight(0, 1.0f) && !Sight(ATT_BIT(ATT_OPTIC4X), 1.0f));
    assert(!Sight(ATT_BIT(ATT_GRIP) | ATT_BIT(ATT_QUICKDRAW), 1.0f));
    assert(!Sight(ATT_BIT(ATT_REFLEX), 0.0f) && !Sight(ATT_BIT(ATT_REFLEX), 0.3f));
    assert(!Sight(ATT_BIT(ATT_REFLEX), 0.6f) && !Sight(ATT_BIT(ATT_LENS), 0.6f));

    /* The reflex dot: a centre, a solid disc and a glow fading to nothing. */
    assert(Sight(ATT_BIT(ATT_REFLEX) | ATT_BIT(ATT_GRIP), 1.0f) == 1);
    d = &draws[0];
    assert(d->vertexCount == 49 && d->indexCount == 216);
    assert(NEAR(d->verts[0].xy[0], CX) && NEAR(d->verts[0].xy[1], CY) && NEAR(d->verts[0].rgba[3], 0.95f));
    Ring(d, 1, 0.9f * 1.5f, red, 1.0f);
    Ring(d, 25, 2.5f * 1.5f, red, 0.0f);
    Mesh(d, 0.0f, 2.5f * 1.5f);

    /* It eases in: a quarter of the way through the fade shows 0.15625 of it. */
    assert(Sight(ATT_BIT(ATT_REFLEX), 0.7f) == 1);
    assert(NEAR(draws[0].verts[0].rgba[3], 0.95f * 0.15625f));
    Ring(&draws[0], 1, 0.9f * 1.5f, red, 0.15625f);

    /* The lens sight: a feathered ring and a small dot. */
    assert(Sight(ATT_BIT(ATT_LENS), 1.0f) == 2);
    d = &draws[0];
    assert(d->vertexCount == 96 && d->indexCount == 432);
    Ring(d, 0, 5.625f * 1.5f, amber, 0.0f);
    Ring(d, 24, 6.225f * 1.5f, amber, 1.0f);
    Ring(d, 48, 6.775f * 1.5f, amber, 1.0f);
    Ring(d, 72, 7.375f * 1.5f, amber, 0.0f);
    Mesh(d, 5.625f * 1.5f, 7.375f * 1.5f);
    d = &draws[1];
    assert(d->vertexCount == 49 && d->indexCount == 216 && NEAR(d->verts[0].rgba[3], 0.9f));
    Ring(d, 1, 0.5f * 1.5f, amber, 1.0f);
    Ring(d, 25, 1.1f * 1.5f, amber, 0.0f);
    Mesh(d, 0.0f, 1.1f * 1.5f);
    return 0;
}
'''

mutants = [
    ('ps->fWeaponPosFrac <= SIGHT_FADE_START', 'ps->fWeaponPosFrac <= 0.0f'),
    ('    alpha = alpha * alpha * (3.0f - 2.0f * alpha);\n', ''),
    ('if (!(mask & (ATT_BIT(ATT_REFLEX) | ATT_BIT(ATT_LENS)))', 'if (!mask'),
    ('        const float alphas[3] = { 1.0f, 1.0f, 0.0f };', '        const float alphas[3] = { 1.0f, 1.0f, 1.0f };'),
    ('        int next = (i + 1) % SIGHT_SEGMENTS;', '        int next = i + 1;'),
    ('            indices[indexCount++] = (unsigned short)(a + SIGHT_SEGMENTS);', '            indices[indexCount++] = a;'),
    ('    CalcScreenPlacement(&x, &y, &w, &scale, 2, 2);\n', ''),
    ('        CG_DrawSightRings(x, y, dot, dotAlphas, 3, s_sightLens, alpha);\n', ''),
    ('(SIGHT_REFLEX_RADIUS + SIGHT_REFLEX_GLOW) * scale', '(SIGHT_REFLEX_RADIUS + SIGHT_REFLEX_GLOW)'),
    ('    vertex->rgba[3] = color[3] * alpha;', '    vertex->rgba[3] = color[3];'),
]


def program(source):
    return support + source[source.index('#define SIGHT_SEGMENTS'):] + checks


with tempfile.TemporaryDirectory(prefix='cod2-attachment-sight-') as tmp:
    path = Path(tmp)
    source = sight_path.read_text()
    flags = ['-std=gnu99', '-fsanitize=address,undefined', '-fno-sanitize-recover=all']
    (path / 'test.c').write_text(program(source))
    subprocess.run(['cc', *flags, '-O1', '-g', '-Wall', '-Werror', str(path / 'test.c'), '-lm', '-o',
                    str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
    for index, (before, after) in enumerate(mutants):
        assert source.count(before) == 1, before
        (path / f'mutant{index}.c').write_text(program(source.replace(before, after)))
        subprocess.run(['cc', *flags, '-O0', '-w', str(path / f'mutant{index}.c'), '-lm', '-o',
                        str(path / f'mutant{index}')], check=True)
        result = subprocess.run([str(path / f'mutant{index}')], capture_output=True)
        assert result.returncode != 0, f'mutant survived: {after!r}'
print(f'PASS: reflex and lens reticles, their fade, placement, rings and meshes; {len(mutants)} mutants fail')
