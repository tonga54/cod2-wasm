#!/usr/bin/env python3
"""Run the production WWII grenade warning behind the real stock threat checks:
where it sits, how it turns, grows and reddens, the images it builds, and when
the stock warning stays."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'src/PC/cgame_mp/cg_grenadewarning_mp.c').read_text()
header = (root / 'src/headers/PC/cgame_mp/cg_grenadewarning.h').read_text()
draw_source = (root / 'src/PC/cgame_mp/cg_draw_mp.c').read_text()
main = (root / 'src/PC/cgame_mp/cg_main_mp.c').read_text()
hud2d_header = (root / 'src/headers/PC/gfx_d3d/r_hud2d.h').read_text()
hud2d = (root / 'src/PC/gfx_d3d/r_hud2d.c').read_text()


def function(text, name):
    match = re.search(r'^((?:static )?[^\n;]*\b' + name + r'\([^;]*?\))\s*\{', text, re.M)
    assert match, name
    brace = text.index('{', match.end() - 1)
    depth, end = 1, brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


# Every level registers the setting and builds the images anew, each in a HUD
# image slot of its own.
assert draw_source.count('#include "headers/PC/cgame_mp/cg_grenadewarning.h"\n') == 1
assert draw_source.count('CG_DrawGrenadeWarnings(') == 1
assert main.count('#include "headers/PC/cgame_mp/cg_grenadewarning.h"\n') == 1
assert main.count('    CG_MantlePromptInit();\n    CG_GrenadeWarningInit();\n') == 1
slots = {name: int(re.search(r'^#define ' + name + r' (\d+)$', hud2d_header, re.M).group(1))
         for name in ('R_HUD_IMAGE_GRENADE_RING', 'R_HUD_IMAGE_GRENADE_POINTER', 'R_HUD_IMAGE_COUNT')}
names = re.findall(r'"([^"]+)"', re.search(r's_hudImageNames\[R_HUD_IMAGE_COUNT\] = \{(.*?)\};', hud2d, re.S).group(1))
assert len(names) == slots['R_HUD_IMAGE_COUNT']
assert names[slots['R_HUD_IMAGE_GRENADE_RING']] == '*hud_grenade_ring'
assert names[slots['R_HUD_IMAGE_GRENADE_POINTER']] == '*hud_grenade_pointer'

warning = source[source.index("/* The ring's middle radius"):]
threat_type = header[header.index('typedef struct CG_GrenadeThreat'):header.index('} CG_GrenadeThreat;') + 19]
distance = function(draw_source, 'CG_GrenadeThreatDistance')
stock = function(draw_source, 'CG_DrawGrenadeIndicators')

support = r'''
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define WEAPTYPE_GRENADE 1
#define OFFHAND_CLASS_FRAG_GRENADE 1
#define R_HUD_IMAGE_GRENADE_RING ''' + str(slots['R_HUD_IMAGE_GRENADE_RING']) + r'''
#define R_HUD_IMAGE_GRENADE_POINTER ''' + str(slots['R_HUD_IMAGE_GRENADE_POINTER']) + r'''
typedef unsigned char byte;
typedef int qboolean;
typedef float vec_t, vec3_t[3], vec4_t[4];
typedef const char *MaterialHandle;
typedef struct { int number, eType, eFlags, time, weapon; } entityState_t;
typedef struct { entityState_t nextState; int nextValid; vec3_t lerpOrigin; } centity_t;
typedef struct { int pm_type, weapon, stats[3]; float fWeaponPosFrac; vec3_t origin; } playerState_t;
typedef struct { int numEntities; entityState_t entities[256]; } snapshot_t;
typedef struct { snapshot_t *snap, *nextSnap; playerState_t predictedPlayerState; int time, renderingThirdPerson;
                 vec3_t refdefViewAngles; struct { int width, height; } refdef; } cg_t;
typedef struct { struct { MaterialHandle grenadeIcon, grenadePointer; } media; } cgs_t;
typedef struct { int weapType, offhandClass, iExplosionRadius, iExplosionInnerDamage, iExplosionOuterDamage,
                 overlayReticle; } WeaponDef;
typedef struct { struct { float value, vector[4]; int enabled; } current; } dvar_t;
''' + threat_type + r'''
static cg_t cgState, *cg = &cgState;
static cgs_t cgsState, *cgs = &cgsState;
static centity_t entities[1024], *cg_entities = entities;
static snapshot_t snap;
static WeaponDef defs[4];
static int hidden;
static dvar_t maxRange, maxHeight, inScope, iconOffset, iconWidth, iconHeight, arrowWidth, arrowHeight, pivot, pulseFreq,
              pulseMax, pulseMin, warningDvar = { { 0, { 0 }, 1 } };
static dvar_t *cg_hudGrenadeIconMaxRange = &maxRange, *cg_hudGrenadeIconMaxHeight = &maxHeight,
              *cg_hudGrenadeIconInScope = &inScope, *cg_hudGrenadeIconOffset = &iconOffset,
              *cg_hudGrenadeIconWidth = &iconWidth, *cg_hudGrenadeIconHeight = &iconHeight,
              *cg_hudGrenadePointerWidth = &arrowWidth, *cg_hudGrenadePointerHeight = &arrowHeight,
              *cg_hudGrenadePointerPivot = &pivot, *cg_hudGrenadePointerPulseFreq = &pulseFreq,
              *cg_hudGrenadePointerPulseMax = &pulseMax, *cg_hudGrenadePointerPulseMin = &pulseMin;
static int registered;
static const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags) {
    assert(!strcmp(name, "cg_hudGrenadeWarning") && value == 1 && flags == 0x1001);
    registered++;
    return &warningDvar;
}
static int BG_GetNumWeapons(void) { return 3; }
static WeaponDef *BG_GetWeaponDef(int index) { assert(index >= 1 && index <= 3); return &defs[index]; }
static int CG_AreHudMenusHidden(void) { return hidden; }
static float vectoyaw(const vec_t *v) { return v[0] == 0 && v[1] == 0 ? 0 : atan2f(v[1], v[0]) * 57.29577951f; }
/* A screen screenHeight pixels tall: 480 virtual units to its height. */
static float screenHeight = 720;
static float GetVirtualWidthFromRealWidth(float width) { return width * 480.0f / screenHeight; }
static float GetVirtualHeightFromRealHeight(float height) { return height * 480.0f / screenHeight; }
static float GetRealHeightFromVirtualHeight(float height) { return height * screenHeight / 480.0f; }
static byte ringImage[256 * 256 * 4], pointerImage[64 * 64 * 4];
static int ringW, ringH, pointerW, pointerH, creates, failSlot = -1;
static MaterialHandle R_CreateHudImageMaterial(int slot, const byte *bgra, int width, int height) {
    creates++;
    assert(width > 0 && height > 0);
    if (slot == R_HUD_IMAGE_GRENADE_RING) {
        assert(width * height * 4 <= (int)sizeof(ringImage));
        memcpy(ringImage, bgra, width * height * 4); ringW = width; ringH = height;
    } else {
        assert(slot == R_HUD_IMAGE_GRENADE_POINTER && width * height * 4 <= (int)sizeof(pointerImage));
        memcpy(pointerImage, bgra, width * height * 4); pointerW = width; pointerH = height;
    }
    if (slot == failSlot)
        return NULL;
    return slot == R_HUD_IMAGE_GRENADE_RING ? "*hud_grenade_ring" : "*hud_grenade_pointer";
}
enum { STRETCH, ROTATED, HANDLE };
typedef struct { int kind; MaterialHandle material; float x, y, w, h, angle; vec4_t color; } Drawn;
static Drawn drawn[64];
static int drawCount;
static void record(int kind, float x, float y, float w, float h, float angle, int horzAlign, int vertAlign,
                   const vec_t *color, MaterialHandle material) {
    assert(drawCount < 64 && horzAlign == 7 && vertAlign == 7 && material);
    assert(isfinite(x) && isfinite(y) && isfinite(w) && isfinite(h) && isfinite(angle));
    for (int i = 0; i < 4; i++) assert(isfinite(color[i]));
    drawn[drawCount++] = (Drawn){ kind, material, x, y, w, h, angle, { color[0], color[1], color[2], color[3] } };
}
static void CL_DrawStretchPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float s1, float t1,
                              float s2, float t2, const vec_t *color, MaterialHandle material) {
    assert(s1 == 0 && t1 == 0 && s2 == 1 && t2 == 1);
    record(STRETCH, x, y, w, h, 0, horzAlign, vertAlign, color, material);
}
static void CG_DrawRotatedPic(float x, float y, float w, float h, int horzAlign, int vertAlign, float angle,
                              const vec_t *color, MaterialHandle material) {
    record(ROTATED, x, y, w, h, angle, horzAlign, vertAlign, color, material);
}
static void UI_DrawHandlePic(float x, float y, float w, float h, int horzAlign, int vertAlign, const vec_t *color,
                             MaterialHandle material) {
    record(HANDLE, x, y, w, h, 0, horzAlign, vertAlign, color, material);
}
'''

checks = r'''
static int near(float a, float b, float tolerance) { return fabsf(a - b) <= tolerance; }
static const float red[3] = { 1.0f, 0.37f, 0.24f };
/* Texels per radius and image sizes at 720 lines, where the largest ring's
 * middle radius, 1.5 x 17.9 units, is 40.3 pixels. */
static int texels = 41, ringSize = 88, arrowW = 22, arrowH = 12;
/* Where the ring is the captures' size, a sixth of CoD2's frag range in from
 * its edge. */
static const float capturesDistance = 256.0f * 5 / 6;

static void reset(int width, int height) {
    memset(&cgState, 0, sizeof(cgState)); memset(&snap, 0, sizeof(snap)); memset(entities, 0, sizeof(entities));
    cg_entities = entities; cg->snap = cg->nextSnap = &snap;
    cg->predictedPlayerState.weapon = 1;
    cg->predictedPlayerState.stats[0] = cg->predictedPlayerState.stats[2] = 100;
    cg->refdef.width = width; cg->refdef.height = height; screenHeight = (float)height;
    cgsState.media.grenadeIcon = "hud_grenadeicon"; cgsState.media.grenadePointer = "hud_grenadepointer";
    /* CoD2's frags: a 256-unit blast, 200 damage at its centre and 50 at its edge. */
    for (int i = 1; i <= 3; i++) defs[i] = (WeaponDef){ WEAPTYPE_GRENADE, OFFHAND_CLASS_FRAG_GRENADE, 256, 200, 50, 0 };
    maxRange.current.value = maxHeight.current.value = 256; inScope.current.enabled = 1; iconOffset.current.value = 50;
    iconWidth.current.value = iconHeight.current.value = arrowWidth.current.value = 25; arrowHeight.current.value = 12;
    pivot.current.vector[0] = 12; pivot.current.vector[1] = 27; pulseFreq.current.value = 1.7f;
    pulseMin.current.value = 0.3f; pulseMax.current.value = 1.85f; hidden = 0; drawCount = 0;
}
static void grenade(int number, int weapon, float yaw, float distance) {
    centity_t *cent = &entities[number];
    float r = yaw * 0.01745329252f;
    *cent = (centity_t){ .nextState = { .number = number, .eType = 4, .weapon = weapon }, .nextValid = 1,
                         .lerpOrigin = { cosf(r) * distance, sinf(r) * distance, 0 } };
    snap.entities[snap.numEntities++] = cent->nextState;
}
static void frame(void) { drawCount = 0; CG_DrawGrenadeIndicators(); }
static int stockDrawn(int warnings) {
    return drawCount == 2 * warnings && drawn[0].kind == HANDLE && !strcmp(drawn[0].material, "hud_grenadeicon");
}

/* The spec, from the WWII captures. The ring's middle radius is 17.9 units a
 * sixth of the warning's range in from its edge, 0.9 of that at the edge and
 * 1.5 at the player's feet. At that size its centre lies on an ellipse 0.86 of
 * the way from the screen's centre to its edges, in the grenade's direction
 * (up is ahead); the triangle's tip, 1.49 radii out, keeps its place at every
 * size. The warning turns red over the 16 units outside the lethal radius. */
typedef struct { float x, y, radius, dx, dy, red, tipX, tipY; } Spec;
static float clamp01(float v) { return v < 0 ? 0 : v > 1 ? 1 : v; }
static Spec spec(float bearing, float distance, float range, float lethal) {
    float r = bearing * 0.01745329252f;
    float halfW = 0.5f * (float)cgState.refdef.width * 480.0f / screenHeight;
    float halfH = 0.5f * (float)cgState.refdef.height * 480.0f / screenHeight;
    float a = 0.86f * halfW, b = 0.86f * halfH, e;
    Spec s;
    s.dx = -sinf(r); s.dy = -cosf(r);
    s.radius = 17.9f * (0.9f + 0.6f * clamp01(1 - distance / range));
    e = 1 / sqrtf(s.dx * s.dx / (a * a) + s.dy * s.dy / (b * b));
    s.tipX = s.dx * (e + 1.49f * 17.9f); s.tipY = s.dy * (e + 1.49f * 17.9f);
    s.x = s.tipX - s.dx * 1.49f * s.radius; s.y = s.tipY - s.dy * 1.49f * s.radius;
    s.red = lethal > 0 ? clamp01((lethal + 16 - distance) / 16) : 0;
    return s;
}
static int colored(const float *color, const float *rgb, float alpha) {
    return near(color[0], rgb[0], 1e-4f) && near(color[1], rgb[1], 1e-4f) && near(color[2], rgb[2], 1e-4f) &&
           near(color[3], alpha, 1e-4f);
}
/* Four draws from drawn[i]: the ring's image over its shadow, one unit down
 * and right at 0.6 black, then the triangle's over its own, turned so that
 * its tip points along the grenade's direction, centred 1.37 radii out. */
static int warningAt(int i, Spec s) {
    static const float black[3] = { 0, 0, 0 };
    const Drawn *ringShadow = &drawn[i], *ring = &drawn[i + 1], *arrowShadow = &drawn[i + 2], *arrow = &drawn[i + 3];
    float side = (float)ringSize / (float)texels * s.radius;
    float w = (float)arrowW / (float)texels * s.radius, h = (float)arrowH / (float)texels * s.radius;
    float cx = s.x + s.dx * 1.37f * s.radius, cy = s.y + s.dy * 1.37f * s.radius, color[3];
    const float tol = 2e-3f;
    for (int k = 0; k < 3; k++) color[k] = 1 + (red[k] - 1) * s.red;
    return i + 4 <= drawCount &&
           ring->kind == STRETCH && !strcmp(ring->material, "*hud_grenade_ring") && near(ring->w, side, tol) &&
           near(ring->h, side, tol) && near(ring->x + 0.5f * side, s.x, tol) && near(ring->y + 0.5f * side, s.y, tol) &&
           colored(ring->color, color, 1) &&
           ringShadow->kind == STRETCH && !strcmp(ringShadow->material, "*hud_grenade_ring") &&
           near(ringShadow->x, ring->x + 1, tol) && near(ringShadow->y, ring->y + 1, tol) &&
           near(ringShadow->w, side, tol) && near(ringShadow->h, side, tol) && colored(ringShadow->color, black, 0.6f) &&
           arrow->kind == ROTATED && !strcmp(arrow->material, "*hud_grenade_pointer") && near(arrow->w, w, tol) &&
           near(arrow->h, h, tol) && near(arrow->x + 0.5f * w, cx, tol) && near(arrow->y + 0.5f * h, cy, tol) &&
           near(sinf(arrow->angle * 0.01745329252f), s.dx, 1e-4f) &&
           near(-cosf(arrow->angle * 0.01745329252f), s.dy, 1e-4f) && colored(arrow->color, color, 1) &&
           arrowShadow->kind == ROTATED && !strcmp(arrowShadow->material, "*hud_grenade_pointer") &&
           near(arrowShadow->x, arrow->x + 1, tol) && near(arrowShadow->y, arrow->y + 1, tol) &&
           near(arrowShadow->w, w, tol) && near(arrowShadow->h, h, tol) &&
           near(arrowShadow->angle, arrow->angle, 1e-4f) && colored(arrowShadow->color, black, 0.6f);
}

static const byte ringBgr[3] = { 235, 242, 245 }, bodyBgr[3] = { 210, 233, 238 };
static int is(const byte *texel, const byte *bgr) { return texel[0] == bgr[0] && texel[1] == bgr[1] && texel[2] == bgr[2]; }
static const byte *ringTexel(int x, int y) { return &ringImage[(y * ringW + x) * 4]; }
/* The ring image's alpha at a point in radii from its centre. */
static float alphaAt(float x, float y) {
    int tx = (int)floorf(x * texels + 0.5f * ringW), ty = (int)floorf(y * texels + 0.5f * ringH);
    return tx < 0 || ty < 0 || tx >= ringW || ty >= ringH ? 0 : ringTexel(tx, ty)[3] / 255.0f;
}
/* The grenade's width, in radii, across its axis at a distance along it from
 * the head's top: the axis runs 27 degrees off upright, the head up and to the
 * left, centred on the ring 0.032 to its upper right. */
static float bodyWidth(float along) {
    float sum = 0, step = 0.25f / (float)texels, u = along - 0.75f;
    for (float across = -0.3f; across <= 0.3f; across += step)
        sum += step * alphaAt(0.032f * 0.891f + u * 0.454f + across * 0.891f, -0.032f * 0.454f + u * 0.891f - across * 0.454f);
    return sum;
}
static float ringWidthAt(float angle) {
    float sum = 0, step = 0.25f / (float)texels, r = angle * 0.01745329252f;
    for (float d = 0.85f; d <= 1.15f; d += step) sum += step * alphaAt(d * cosf(r), d * sinf(r));
    return sum;
}

int main(void) {
    int cases = 0;
    const float lethal = 256.0f * (200 - 100) / (200 - 50);

    /* Before the setting exists, the stock warning. */
    reset(1280, 720);
    grenade(64, 1, 90, 100);
    assert(!CG_GrenadeWarningActive());
    frame();
    assert(stockDrawn(1) && !creates);
    CG_GrenadeWarningInit();
    assert(registered == 1 && CG_GrenadeWarningActive());

    /* The images wait for the first grenade, then are built once, sized for
     * the screen. */
    reset(1280, 720);
    frame();
    assert(drawCount == 0 && !creates);
    grenade(64, 1, 90, 100);
    frame();
    assert(creates == 2 && ringW == ringSize && ringH == ringSize && pointerW == arrowW && pointerH == arrowH);
    assert(drawCount == 4 && warningAt(0, spec(90, 100, 256, lethal)));
    frame();
    assert(creates == 2 && drawCount == 4);

    /* Every direction and turn of the view, near and far: the ring orbits the
     * screen's centre toward the grenade, the triangle turns with it, the size
     * follows the distance and the tip holds its place. */
    for (int heading = 0; heading < 360; heading += 15)
        for (int turn = 0; turn < 360; turn += 45)
            for (int d = 0; d < 6; d++) {
                float dist = (float[]){ 1, 64, 128, 170, 200, 255.9f }[d], radius;
                Spec s = spec((float)(heading - turn), dist, 256, lethal);
                reset(1280, 720);
                grenade(64, 1, (float)heading, dist);
                cg->refdefViewAngles[1] = (float)turn;
                frame();
                assert(drawCount == 4 && warningAt(0, s));
                radius = drawn[1].w * texels / ringSize;
                assert(near(drawn[1].x + 0.5f * drawn[1].w + s.dx * 1.49f * radius, s.tipX, 2e-3f) &&
                       near(drawn[1].y + 0.5f * drawn[1].h + s.dy * 1.49f * radius, s.tipY, 2e-3f));
                cases++;
            }
    /* At the captures' size the ring's centre is on the ellipse; behind, it is
     * 0.86 of the way down, and white. */
    reset(1280, 720);
    grenade(64, 1, 180, capturesDistance);
    frame();
    assert(near(drawn[1].color[0], 1, 1e-4f) && near(drawn[1].color[1], 1, 1e-4f) && near(drawn[1].color[2], 1, 1e-4f));
    assert(near(drawn[1].x + 0.5f * drawn[1].w, 0, 1e-3f) && near(drawn[1].y + 0.5f * drawn[1].h, 0.86f * 240, 1e-2f));
    assert(near(drawn[1].w, (float)ringSize / texels * 17.9f, 1e-3f));

    /* The captures, 1024 x 576 frames, at their size: rings centred
     * (188.77, 224.73) and (294.21, 184.19) pixels from the centre, 1.2 a unit. */
    for (int c = 0; c < 2; c++) {
        float px = c ? 294.21f : 188.77f, py = c ? 184.19f : 224.73f, len = sqrtf(px * px + py * py);
        reset(1024, 576);
        grenade(64, 1, atan2f(-px / len, -py / len) * 57.29577951f, capturesDistance);
        frame();
        assert(near((drawn[1].x + 0.5f * drawn[1].w) * 1.2f, px, 2.0f) && near((drawn[1].y + 0.5f * drawn[1].h) * 1.2f, py, 2.0f));
    }

    /* Red within the lethal radius of a full-health player, 170.7 units for
     * CoD2's frags, fading to white over the next 16. */
    for (int d = 0; d < 7; d++) {
        float dist = (float[]){ 20, 100, 170, 172, lethal + 8, 186, 190 }[d];
        reset(1280, 720);
        grenade(64, 1, 30, dist);
        frame();
        assert(warningAt(0, spec(30, dist, 256, lethal)));
        cases++;
    }
    assert(near(drawn[1].color[1], 1, 1e-4f));
    reset(1280, 720);
    grenade(64, 1, 30, lethal + 8);
    frame();
    assert(near(drawn[1].color[1], 0.685f, 1e-3f) && near(drawn[1].color[2], 0.62f, 1e-3f));
    /* The lethal radius follows the player's maximum health, not the health
     * left: with 50 the whole blast kills, with 250 none of it, with none set
     * 100 is assumed. A blast whose edge kills is red throughout. */
    reset(1280, 720); cg->predictedPlayerState.stats[2] = 50; grenade(64, 1, 30, 250); frame();
    assert(warningAt(0, spec(30, 250, 256, 256)));
    reset(1280, 720); cg->predictedPlayerState.stats[2] = 250; grenade(64, 1, 30, 5); frame();
    assert(warningAt(0, spec(30, 5, 256, 0)) && near(drawn[1].color[1], 1, 1e-4f));
    reset(1280, 720); cg->predictedPlayerState.stats[2] = 0; grenade(64, 1, 30, 190); frame();
    assert(warningAt(0, spec(30, 190, 256, lethal)) && near(drawn[1].color[1], 1, 1e-4f));
    reset(1280, 720); cg->predictedPlayerState.stats[0] = 40; grenade(64, 1, 30, 190); frame();
    assert(warningAt(0, spec(30, 190, 256, lethal)) && near(drawn[1].color[1], 1, 1e-4f));
    reset(1280, 720); defs[2].iExplosionInnerDamage = 300; defs[2].iExplosionOuterDamage = 120; grenade(64, 2, 30, 250);
    frame();
    assert(warningAt(0, spec(30, 250, 256, 256)) && near(drawn[1].color[1], 0.37f, 1e-4f));
    /* A shorter warning range sizes by that range. */
    reset(1280, 720); maxRange.current.value = 200; grenade(64, 1, 30, 200.0f * 5 / 6); frame();
    assert(warningAt(0, spec(30, 200.0f * 5 / 6, 200, lethal)) && near(drawn[1].w, (float)ringSize / texels * 17.9f, 1e-3f));

    /* The four nearest of five, the farthest drawn first and the nearest on
     * top. */
    reset(1280, 720);
    grenade(64, 1, 0, 30); grenade(65, 1, 90, 60); grenade(66, 1, 180, 90); grenade(67, 1, 270, 120); grenade(68, 1, 0, 200);
    frame();
    assert(drawCount == 16 && warningAt(0, spec(270, 120, 256, lethal)) && warningAt(4, spec(180, 90, 256, lethal)) &&
           warningAt(8, spec(90, 60, 256, lethal)) && warningAt(12, spec(0, 30, 256, lethal)));

    /* The ring image, sized at 720 lines: the HUD text's white ring 0.088
     * radii thick and the cream grenade, smooth-edged on a clear border. The
     * grenade's width along its axis is the captures': a head 0.362 wide, a
     * collar 0.2, a waist 0.148 and a grip end 0.199, 1.5 long; its centre of
     * area and its area are the captures' within 0.012 radii and 3%. */
    {
        double bodyArea = 0, ringArea = 0, sx = 0, sy = 0;
        int partial = 0;
        for (int y = 0; y < ringH; y++)
            for (int x = 0; x < ringW; x++) {
                const byte *t = ringTexel(x, y);
                float cx = (x + 0.5f - 0.5f * ringW) / texels, cy = (y + 0.5f - 0.5f * ringH) / texels;
                float r = sqrtf(cx * cx + cy * cy);
                assert(is(t, ringBgr) || is(t, bodyBgr));
                if (x == 0 || y == 0 || x == ringW - 1 || y == ringH - 1) assert(!t[3]);
                if (t[3] && r > 0.9f) assert(is(t, ringBgr));
                if (r < 0.84f) assert(is(t, bodyBgr));
                partial += t[3] > 0 && t[3] < 255;
                if (is(t, bodyBgr)) { bodyArea += t[3] / 255.0; sx += t[3] / 255.0 * cx; sy += t[3] / 255.0 * cy; }
                else ringArea += t[3] / 255.0;
            }
        sx /= bodyArea; sy /= bodyArea;
        bodyArea /= (double)texels * texels; ringArea /= (double)texels * texels;
        assert(partial > 150 && fabs(bodyArea - 0.346) < 0.346 * 0.03 && fabs(ringArea - 0.55) < 0.55 * 0.03);
        assert(fabs(sx + 0.034) < 0.012 && fabs(sy + 0.136) < 0.012);
        assert(alphaAt(1.0f, 0.01f) == 1 && alphaAt(-1.0f, 0.01f) == 1 && alphaAt(0.01f, 1.0f) == 1 && alphaAt(0.01f, -1.0f) == 1);
        assert(!alphaAt(0.5f, 0) && !alphaAt(1.1f, 0) && !alphaAt(0.9f, 0.9f));
        for (int a = 0; a < 360; a += 30) assert(near(ringWidthAt((float)a), 0.088f, 0.012f));
        assert(near(bodyWidth(0.2f), 0.362f, 0.02f) && near(bodyWidth(0.6f), 0.202f, 0.02f));
        assert(near(bodyWidth(1.0f), 0.148f, 0.02f) && near(bodyWidth(1.4f), 0.199f, 0.02f));
        assert(bodyWidth(0.04f) > 0.3f && bodyWidth(-0.04f) < 0.02f && bodyWidth(1.46f) > 0.15f && bodyWidth(1.54f) < 0.02f);
    }
    /* The triangle, tip up in its box: white, symmetric, three quarters of its
     * area below its middle, its tip one or two texels wide and its long side
     * nearly the box's width, with the area of a right isosceles triangle 0.24
     * radii tall within 5%. */
    {
        double area = 0, upper = 0, lower = 0;
        int top = -1, bottom = -1;
        for (int y = 0; y < pointerH; y++)
            for (int x = 0; x < pointerW; x++) {
                const byte *t = &pointerImage[(y * pointerW + x) * 4];
                assert(is(t, ringBgr) && t[3] == pointerImage[(y * pointerW + pointerW - 1 - x) * 4 + 3]);
                if (x == 0 || y == 0 || x == pointerW - 1 || y == pointerH - 1) assert(!t[3]);
                area += t[3] / 255.0;
                if (y < pointerH / 2) upper += t[3]; else lower += t[3];
                if (t[3] && top < 0) top = y;
                if (t[3]) bottom = y;
            }
        assert(fabs(area / texels / texels - 0.0576) < 0.0576 * 0.05 && lower > 2.5 * upper);
        for (int x = 0; x < pointerW; x++) {
            if (pointerImage[(top * pointerW + x) * 4 + 3]) assert(x == pointerW / 2 - 1 || x == pointerW / 2);
        }
        assert(pointerImage[(bottom * pointerW + 2) * 4 + 3] && pointerImage[(bottom * pointerW + pointerW - 3) * 4 + 3]);
    }

    /* Larger screens build larger images, a texel per pixel at the largest
     * size, up to 96 texels a radius; the warning keeps its size in units. */
    CG_GrenadeWarningInit();
    texels = 61; ringSize = 130; arrowW = 32; arrowH = 18;
    reset(1920, 1080); grenade(64, 1, 45, 150); frame();
    assert(creates == 4 && ringW == 130 && ringH == 130 && pointerW == 32 && pointerH == 18);
    assert(warningAt(0, spec(45, 150, 256, lethal)));
    CG_GrenadeWarningInit();
    texels = 96; ringSize = 204; arrowW = 50; arrowH = 26;
    reset(3840, 2160); grenade(64, 1, 45, 150); frame();
    assert(creates == 6 && ringW == 204 && pointerW == 50 && pointerH == 26 && warningAt(0, spec(45, 150, 256, lethal)));
    /* And tiny ones no fewer than 8. */
    CG_GrenadeWarningInit();
    texels = 8; ringSize = 20; arrowW = 6; arrowH = 4;
    reset(214, 120); grenade(64, 1, 45, 150); frame();
    assert(creates == 8 && ringW == 20 && pointerW == 6 && pointerH == 4 && warningAt(0, spec(45, 150, 256, lethal)));
    texels = 41; ringSize = 88; arrowW = 22; arrowH = 12;

    /* Images that cannot be made leave the stock warning, without a try every
     * frame; the next level tries again. Before the view has a size nothing
     * is built and the stock warning stays. */
    CG_GrenadeWarningInit();
    failSlot = R_HUD_IMAGE_GRENADE_RING;
    reset(1280, 720); grenade(64, 1, 90, 100);
    frame();
    assert(stockDrawn(1) && creates == 10);
    frame();
    assert(stockDrawn(1) && creates == 10);
    CG_GrenadeWarningInit();
    failSlot = -1;
    reset(0, 0); screenHeight = 720; grenade(64, 1, 90, 100);
    frame();
    assert(stockDrawn(1) && creates == 10);
    reset(1280, 720); grenade(64, 1, 90, 100);
    frame();
    assert(creates == 12 && drawCount == 4 && warningAt(0, spec(90, 100, 256, lethal)));

    /* The stock warning's guards hold. */
    for (int gate = 0; gate < 4; gate++) {
        reset(1280, 720); grenade(64, 1, 90, 100);
        if (gate == 0) cg->predictedPlayerState.pm_type = 4;
        if (gate == 1) cg->predictedPlayerState.stats[0] = 0;
        if (gate == 2) hidden = 1;
        if (gate == 3) cg->renderingThirdPerson = 1;
        frame();
        assert(drawCount == 0);
    }

    /* Switched off, the stock warning. */
    warningDvar.current.enabled = 0;
    assert(!CG_GrenadeWarningActive());
    reset(1280, 720); grenade(64, 1, 90, 100);
    frame();
    assert(stockDrawn(1));
    printf("%d\n", cases);
    return 0;
}
'''

mutants = [
    [('#define GRENADE_WARNING_RADIUS 17.9f', '#define GRENADE_WARNING_RADIUS 20.0f')],
    [('#define GRENADE_WARNING_RING_WIDTH 0.088f', '#define GRENADE_WARNING_RING_WIDTH 0.12f')],
    [('#define GRENADE_WARNING_POINTER_BASE 1.25f', '#define GRENADE_WARNING_POINTER_BASE 1.1f')],
    [('#define GRENADE_WARNING_POINTER_HEIGHT 0.24f', '#define GRENADE_WARNING_POINTER_HEIGHT 0.3f')],
    [('#define GRENADE_WARNING_SCALE_FAR 0.9f', '#define GRENADE_WARNING_SCALE_FAR 0.7f')],
    [('#define GRENADE_WARNING_SCALE_NEAR 1.5f', '#define GRENADE_WARNING_SCALE_NEAR 1.3f')],
    [('#define GRENADE_WARNING_ORBIT 0.86f', '#define GRENADE_WARNING_ORBIT 0.5f')],
    [('#define GRENADE_WARNING_BODY_SIN 0.454f', '#define GRENADE_WARNING_BODY_SIN 0.891f'),
     ('#define GRENADE_WARNING_BODY_COS 0.891f', '#define GRENADE_WARNING_BODY_COS 0.454f')],
    [('#define GRENADE_WARNING_BODY_OFFSET 0.032f', '#define GRENADE_WARNING_BODY_OFFSET -0.032f')],
    [('#define GRENADE_WARNING_BODY_LENGTH 1.5f', '#define GRENADE_WARNING_BODY_LENGTH 1.3f')],
    [('#define GRENADE_WARNING_BODY_REACH 0.87f', '#define GRENADE_WARNING_BODY_REACH 0.0f')],
    [('#define GRENADE_WARNING_RED_BAND 16.0f', '#define GRENADE_WARNING_RED_BAND 48.0f')],
    [('#define GRENADE_WARNING_DEFAULT_HEALTH 100', '#define GRENADE_WARNING_DEFAULT_HEALTH 50')],
    [('#define GRENADE_WARNING_SHADOW_OFFSET 1.0f', '#define GRENADE_WARNING_SHADOW_OFFSET 0.0f')],
    [('#define GRENADE_WARNING_SHADOW_ALPHA 0.6f', '#define GRENADE_WARNING_SHADOW_ALPHA 1.0f')],
    [('#define GRENADE_WARNING_SAMPLES 4', '#define GRENADE_WARNING_SAMPLES 1')],
    [('#define GRENADE_WARNING_TEXELS_MIN 8', '#define GRENADE_WARNING_TEXELS_MIN 4')],
    [('#define GRENADE_WARNING_TEXELS_MAX 96', '#define GRENADE_WARNING_TEXELS_MAX 64')],
    [('{ 0.0f, 0.181f }, { 0.435f, 0.181f }', '{ 0.0f, 0.15f }, { 0.435f, 0.15f }')],
    [('{ 0.84f, 0.074f }, { 1.158f, 0.074f }', '{ 0.84f, 0.1f }, { 1.158f, 0.1f }')],
    [('{ 1.291f, 0.0995f }, { 1.5f, 0.0995f }', '{ 1.291f, 0.074f }, { 1.5f, 0.074f }')],
    [('s_grenadeWarningRingBgr[3] = { 235, 242, 245 }', 's_grenadeWarningRingBgr[3] = { 255, 255, 255 }')],
    [('s_grenadeWarningBodyBgr[3] = { 210, 233, 238 }', 's_grenadeWarningBodyBgr[3] = { 235, 242, 245 }')],
    [('s_grenadeWarningRed = { 1.0f, 0.37f, 0.24f }', 's_grenadeWarningRed = { 1.0f, 1.0f, 1.0f }')],
    [('    if (weapon->iExplosionOuterDamage >= health)\n        return (float)weapon->iExplosionRadius;',
      '    if (weapon->iExplosionOuterDamage >= health)\n        return 0.0f;')],
    [('(float)(weapon->iExplosionInnerDamage - health) /', '(float)(weapon->iExplosionOuterDamage - health) /')],
    [('red = lethal > 0.0f ?', 'red = 1 ?')],
    [('threat->range > 0.0f ? 1.0f - distance / threat->range', 'threat->range > 0.0f ? distance / threat->range')],
    [(' +\n            (GRENADE_WARNING_RADIUS - radius) * (GRENADE_WARNING_POINTER_BASE + GRENADE_WARNING_POINTER_HEIGHT);',
      ';')],
    [('GRENADE_WARNING_ORBIT * halfWidth, GRENADE_WARNING_ORBIT * halfHeight',
      'GRENADE_WARNING_ORBIT * halfHeight, GRENADE_WARNING_ORBIT * halfHeight')],
    [('7, 7, -angle, color, s_grenadeWarningPointer', '7, 7, angle, color, s_grenadeWarningPointer')],
    [('    dx = -sinf(radians);', '    dx = sinf(radians);')],
    [('    for (i = count - 1; i >= 0; --i)', '    for (i = 0; i < count; ++i)')],
    [('    if (s_grenadeWarningTried)\n        return s_grenadeWarningRing && s_grenadeWarningPointer;\n', '')],
    [('    s_grenadeWarningTried = 0;\n', '')],
    [('Dvar_RegisterBool_mac("cg_hudGrenadeWarning", 1, 0x1001)', 'Dvar_RegisterBool_mac("cg_hudGrenadeWarning", 0, 0x1001)')],
    [('return cg_hudGrenadeWarning && cg_hudGrenadeWarning->current.enabled;', 'return cg_hudGrenadeWarning != 0;')],
    [('texel[3] = (byte)((ring + body) * 255', 'texel[3] = (byte)(ring * 255')],
    [('return y <= half && fabsf(x) <= y + half;', 'return y >= -half && fabsf(x) <= half - y;')],
    [('(GRENADE_WARNING_POINTER_BASE + 0.5f * GRENADE_WARNING_POINTER_HEIGHT) * radius',
      'GRENADE_WARNING_POINTER_BASE * radius')],
    [('    CG_DrawRotatedPic(px + GRENADE_WARNING_SHADOW_OFFSET', '    if (0) CG_DrawRotatedPic(px + GRENADE_WARNING_SHADOW_OFFSET')],
    [('ps->stats[2] > 0 ? ps->stats[2]', 'ps->stats[0] > 0 ? ps->stats[0]')],
    [('if (halfWidth <= 0.0f || halfHeight <= 0.0f || !CG_GrenadeWarningMaterials())',
      'if (!CG_GrenadeWarningMaterials())')],
]
stock_mutants = [
    ('    if (CG_GrenadeWarningActive() && CG_DrawGrenadeWarnings(threats, count))\n        return;\n', ''),
    ('CG_DrawGrenadeWarnings(threats, count)', 'CG_DrawGrenadeWarnings(threats, 1)'),
    ('        threats[slot].range = range;\n', '        threats[slot].range = 256.0f;\n'),
]

variants = [(warning, stock)]
for pairs in mutants:
    variant = warning
    for old, new in pairs:
        assert variant.count(old) == 1, old
        variant = variant.replace(old, new)
    variants.append((variant, stock))
for old, new in stock_mutants:
    assert stock.count(old) == 1, old
    variants.append((warning, stock.replace(old, new)))

with tempfile.TemporaryDirectory(prefix='cod2-grenade-warning-') as directory:
    path = Path(directory) / 'warning.c'
    binary = Path(directory) / 'warning'
    for index, (warning_variant, stock_variant) in enumerate(variants):
        path.write_text(support + warning_variant + distance + stock_variant + checks)
        subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-Wno-unused-function', str(path), '-lm', '-o', str(binary)], check=True)
        result = subprocess.run([str(binary)], capture_output=True, text=True)
        assert (result.returncode == 0) == (index == 0), (index, result.stderr[-2000:])
        if index == 0:
            cases = int(result.stdout)
print('PASS: while cg_hudGrenadeWarning is on, the stock grenade warning hands its four nearest threats to the WWII '
      'warning behind the same guards: a white ring around a cream stick grenade, centred on an ellipse 0.86 of the '
      'way to the screen\'s edges toward the grenade, as on both captures within 2 pixels, with a triangle outside '
      'it turned toward the grenade; 17.9 units in radius a sixth of the range in from its edge, where a full-health '
      'player\'s warning is white as on the captures, 0.9 of that at the edge and 1.5 at the feet, the tip holding its '
      'place; red within the lethal radius of a full-health player and white 16 units beyond; nearest on top; images '
      'built once per level for the screen\'s resolution, with the captures\' '
      'shapes, colours and areas; without images, without a view size or switched off, the stock warning stays; '
      f'{cases} direction, distance and colour cases; {len(variants) - 1} failing regression mutants')
