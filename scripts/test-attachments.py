#!/usr/bin/env python3
"""Fit WWII's attachments to CoD2's guns: the rules, the player state that
carries them and the production weapon code that applies them."""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
rules_path = root / 'src/PC/bgame/bg_attachment_rules.c'
glue_path = root / 'src/PC/bgame/bg_attachments.c'
weapons_path = root / 'src/PC/bgame/bg_weapons.c'


def function(source, name):
    match = re.search(r'^[^\n;]*\b' + name + r'\([^;]*?\)\n\{', source, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


# The engine types the glue and the extracted weapon code read, in a stand-in
# for common_types.h that comes first on the include path.
stub_types = r'''
#pragma once
#include <stddef.h>
typedef int qboolean;
typedef float vec_t;
typedef vec_t vec3_t[3];
enum { HITLOC_NONE, HITLOC_HELMET, HITLOC_HEAD };
typedef int weapOverlayReticle_t;
typedef struct {
    int weapon, weaponstate, weaponTime, weaponDelay, weaponRestrictKickTime, pm_type, pm_flags, eFlags, weapAnim,
        grenadeTimeLeft, damageCount, adsDelayTime;
    int weaponrechamber[4], weapons[4], ammo[128], ammoclip[128];
    char weaponslots[8];
    float fWeaponPosFrac, aimSpreadScale, viewHeightCurrent;
} playerState_t;
typedef struct {
    const char *szInternalName;
    char *szOverlayMaterial;
    int weapType, weapSlot, bSlotStackable, iAltWeaponIndex, iClipIndex, iAmmoIndex, iClipSize, iMaxAmmo, iStartAmmo;
    int iFireDelay, iFireTime, iRechamberTime, iRechamberBoltTime, iAdsTransInTime, iReloadStartAdd, iReloadAmmoAdd;
    int bNoPartialReload, bClipOnly, bCookOffHold, bBoltAction, bADSFire, bADSPositionInfo, bSegmentedReload,
        bRechamberWhileAds, iPositionReloadTransTime, adsGunKickReducedKickBullets, hipGunKickReducedKickBullets;
    float adsGunKickReducedKickPercent, hipGunKickReducedKickPercent;
    float fAdsZoomFov, fAdsZoomInFrac, fAdsZoomOutFrac, fAdsSpread, fAdsIdleAmount;
    float adsSwayMaxAngle, adsSwayPitchScale, adsSwayYawScale, adsSwayHorizScale, adsSwayVertScale;
    float fHipSpreadStandMin, fHipSpreadDuckedMin, fHipSpreadProneMin, hipSpreadStandMax, hipSpreadDuckedMax,
        hipSpreadProneMax, fHipSpreadFireAdd;
    float fAdsViewKickPitchMin, fAdsViewKickPitchMax, fAdsViewKickYawMin, fAdsViewKickYawMax;
    float fAdsGunKickPitchMin, fAdsGunKickPitchMax, fAdsGunKickYawMin, fAdsGunKickYawMax;
    float fHipViewKickPitchMin, fHipViewKickPitchMax, fHipViewKickYawMin, fHipViewKickYawMax;
    float fHipGunKickPitchMin, fHipGunKickPitchMax, fHipGunKickYawMin, fHipGunKickYawMax;
    float fMaxDamageRange, fMinDamageRange, locationDamageMultipliers[19];
    int armorPiercing;
    weapOverlayReticle_t overlayReticle;
    float overlayWidth, overlayHeight, fOOPosAnimLength[2];
    int hasDetonator;
    int damage, minDamage, bSemiAuto;
} WeaponDef;
typedef struct { int legsAnim, torsoAnim; } entityState_t;
typedef struct { playerState_t *ps; struct { int buttons, serverTime; } cmd; } pmove_t;
typedef struct { int msec; } pml_t;
'''

support = r'''
#include "common_types.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#define BM_ALWAYS_INLINE inline
#define BM_NOINLINE
#define __attribute_regparm__(n)
#define WEAPTYPE_GRENADE 1
WeaponDef *bg_weaponDefs[128];
int bg_iNumWeapons;
static WeaponDef *bg_weapClips[128], *bg_weapAmmoTypes[128];
int BG_GetAmmoClipSize(int i) { return bg_weapClips[i]->iClipSize; }
int BG_GetAmmoTypeMax(int i) { return bg_weapAmmoTypes[i]->iMaxAmmo; }
static int lastEvent, lastParm, events;
static void BG_AddPredictableEventToPlayerstate(int event, int parm, playerState_t *ps) { lastEvent = event; lastParm = parm; events++; }
static void PM_AddEvent(playerState_t *ps, int event) { BG_AddPredictableEventToPlayerstate(event, 0, ps); }
static void PM_WeaponSetAnim(playerState_t *ps, int anim) { ps->weapAnim = anim; }
static void BG_AnimScriptEvent(playerState_t *ps, int a, int b, int c) {}
static void PM_SetProneMovementOverride(playerState_t *ps) {}
static void PM_SetWeaponRechamberBit(playerState_t *ps, int w) { ps->weaponrechamber[w >> 5] |= 1 << (w & 31); }
static int BG_GetViewmodelWeaponIndex(const playerState_t *ps) { return ps->weapon; }
static int BG_GetViewmodelWeaponIndex_core(const playerState_t *ps) { return ps->weapon; }
static float randomf(void) { return 0.5f; }
static struct { struct { int enabled, integer; } current; } off, *player_scopeExitOnDamage = &off, *player_adsExitDelay = &off;
'''

checks = r'''
#define NEAR(a, b) (fabsf((float)(a) - (float)(b)) < 1e-3f)
static WeaponDef defs[16];
enum { THOMPSON = 1, GREASEGUN, GARAND, KAR98K, KAR98K_SNIPER, BAR, BREN, SHOTGUN, COLT, WEBLEY, PPS42 };
static void Define(int index, const char *name, int clip, int ammo, int fire, int rechamber, int ads, float fov,
                   float spread, float hip, float range, const char *overlay)
{
    WeaponDef *def = &defs[index];
    memset(def, 0, sizeof(*def));
    def->szInternalName = name;
    def->szOverlayMaterial = (char *)overlay;
    def->overlayReticle = overlay[0] ? 2 : 0;
    def->overlayWidth = def->overlayHeight = overlay[0] ? 480.0f : 220.0f;
    def->iClipIndex = def->iAmmoIndex = index;
    def->iClipSize = clip;
    def->iMaxAmmo = def->iStartAmmo = ammo;
    def->iFireTime = fire;
    def->iRechamberTime = rechamber;
    def->iRechamberBoltTime = rechamber / 2;
    def->iAdsTransInTime = ads;
    def->fOOPosAnimLength[0] = 1.0f / (float)ads;
    def->fOOPosAnimLength[1] = 1.0f / 400.0f;
    def->fAdsZoomFov = fov;
    def->fAdsZoomInFrac = 0.5f;
    def->fAdsZoomOutFrac = 0.4f;
    def->fAdsSpread = spread;
    def->fAdsIdleAmount = overlay[0] ? 20.0f : 2.0f;
    def->adsSwayMaxAngle = def->adsSwayPitchScale = def->adsSwayYawScale = 4.0f;
    def->adsSwayHorizScale = def->adsSwayVertScale = 2.0f;
    def->fHipSpreadStandMin = hip;
    def->fHipSpreadDuckedMin = hip * 0.8f;
    def->fHipSpreadProneMin = hip * 0.6f;
    def->hipSpreadStandMax = def->hipSpreadDuckedMax = def->hipSpreadProneMax = 5.0f;
    def->fAdsViewKickPitchMin = 60.0f;
    def->fAdsViewKickPitchMax = 70.0f;
    def->fAdsViewKickYawMin = -20.0f;
    def->fAdsViewKickYawMax = 20.0f;
    def->fAdsGunKickPitchMin = def->fAdsGunKickPitchMax = 40.0f;
    def->fMaxDamageRange = range;
    def->fMinDamageRange = range * 2.0f;
    def->locationDamageMultipliers[HITLOC_HELMET] = def->locationDamageMultipliers[HITLOC_HEAD] = 2.0f;
    def->locationDamageMultipliers[4] = 1.0f;
    def->weapSlot = 1;
    def->bADSPositionInfo = 1;
    bg_weaponDefs[index] = bg_weapClips[index] = bg_weapAmmoTypes[index] = def;
}
static WeaponDef Attached(int weapon, unsigned mask)
{
    WeaponDef def = defs[weapon];
    BG_ApplyAttachments(&def, mask);
    return def;
}
static playerState_t Holding(int primary, unsigned primaryMask, int secondary, unsigned secondaryMask)
{
    playerState_t ps;
    memset(&ps, 0, sizeof(ps));
    ps.weapon = primary;
    ps.weaponslots[1] = (char)primary;
    ps.weaponslots[2] = (char)secondary;
    ps.weapons[0] = 1 << primary | 1 << secondary;
    BG_SetSlotAttachments(&ps, 1, primaryMask);
    BG_SetSlotAttachments(&ps, 2, secondaryMask);
    ps.viewHeightCurrent = 60.0f;
    return ps;
}
int main(void)
{
    int ids[ATT_COUNT], i;
    char text[64];
    WeaponDef d;
    playerState_t ps;

    Define(THOMPSON, "thompson_mp", 20, 180, 85, 100, 220, 65.0f, 0.5f, 1.5f, 800.0f, "");
    Define(GREASEGUN, "greasegun_mp", 32, 192, 120, 100, 220, 65.0f, 0.5f, 1.5f, 1000.0f, "");
    Define(GARAND, "m1garand_mp", 8, 96, 135, 100, 300, 50.0f, 0.0f, 0.75f, 2000.0f, "");
    Define(KAR98K, "kar98k_mp", 5, 60, 330, 1000, 300, 50.0f, 0.0f, 1.5f, 2000.0f, "");
    Define(KAR98K_SNIPER, "kar98k_sniper_mp", 5, 60, 330, 1000, 250, 15.0f, 0.0f, 3.0f, 2000.0f, "scope_overlay_german");
    Define(BAR, "bar_mp", 20, 120, 110, 100, 300, 50.0f, 0.0f, 1.0f, 1500.0f, "");
    Define(BREN, "bren_mp", 30, 180, 120, 100, 300, 50.0f, 0.2f, 1.5f, 1200.0f, "");
    Define(SHOTGUN, "shotgun_mp", 6, 60, 283, 733, 300, 65.0f, 5.5f, 6.0f, 384.0f, "");
    Define(COLT, "colt_mp", 7, 21, 125, 100, 267, 80.0f, 1.0f, 3.0f, 384.0f, "");
    Define(WEBLEY, "webley_mp", 6, 18, 135, 100, 275, 80.0f, 1.0f, 3.0f, 384.0f, "");
    Define(PPS42, "pps42_mp", 35, 210, 85, 100, 220, 65.0f, 0.5f, 1.5f, 1000.0f, "");
    bg_iNumWeapons = PPS42;
    defs[THOMPSON].damage = defs[PPS42].damage = 35;
    defs[GREASEGUN].damage = defs[GARAND].damage = defs[COLT].damage = defs[WEBLEY].damage = 45;
    defs[BAR].damage = defs[BREN].damage = 40;
    defs[GARAND].bSemiAuto = defs[COLT].bSemiAuto = defs[WEBLEY].bSemiAuto = 1;
    defs[THOMPSON].minDamage = defs[PPS42].minDamage = defs[GREASEGUN].minDamage = 10;
    defs[COLT].minDamage = defs[WEBLEY].minDamage = 10;
    defs[BAR].minDamage = defs[BREN].minDamage = 28;
    /* The semi-automatic and bolt-action rifles keep their damage at every range. */
    defs[GARAND].minDamage = 45;
    defs[KAR98K].damage = defs[KAR98K].minDamage = defs[KAR98K_SNIPER].damage = defs[KAR98K_SNIPER].minDamage = 90;

    /* The lists: names, weapons matched without case, WWII's limits. */
    assert(BG_AttachmentFind("suppressor") == ATT_SUPPRESSOR && BG_AttachmentFind("Suppressor") < 0);
    assert(BG_AttachmentFind(NULL) < 0 && BG_AttachmentFind("") < 0);
    assert(BG_AttachmentWeapon("pps42_mp") && !strcmp(BG_AttachmentWeapon("pps42_mp")->weapon, "PPS42_mp"));
    assert(!BG_AttachmentWeapon("mg42_bipod_stand_mp") && !BG_AttachmentWeapon(NULL) && !BG_AttachmentsAllowed("binoculars_mp"));
    assert(BG_AttachmentsAllowed("thompson_mp") & ATT_BIT(ATT_SUPPRESSOR));
    assert(!(BG_AttachmentsAllowed("m1garand_mp") & ATT_BIT(ATT_SUPPRESSOR)));
    assert(!(BG_AttachmentsAllowed("webley_mp") & ATT_BIT(ATT_EXTMAG)) && (BG_AttachmentsAllowed("colt_mp") & ATT_BIT(ATT_EXTMAG)));
    /* The PPSh's drum is its long magazine already. */
    assert(!(BG_AttachmentsAllowed("ppsh_mp") & ATT_BIT(ATT_EXTMAG)) && (BG_AttachmentsAllowed("pps42_mp") & ATT_BIT(ATT_EXTMAG)));
    assert(!(BG_AttachmentsAllowed("bren_mp") & ATT_BIT(ATT_LENS)) && (BG_AttachmentsAllowed("bren_mp") & ATT_BIT(ATT_OPTIC4X)));
    assert(BG_AttachmentsAllowed("kar98k_sniper_mp") == (ATT_BIT(ATT_OPTIC4X) | ATT_BIT(ATT_CALIBRATION) | ATT_BIT(ATT_FMJ) |
                                                         ATT_BIT(ATT_EXTMAG) | ATT_BIT(ATT_RAPIDFIRE)));
    assert(BG_AttachmentCount(0) == 0 && BG_AttachmentCount(0x1fff) == ATT_COUNT);
    assert(BG_AttachmentsParse("thompson_mp", "suppressor+reflex+rapidfire", ATT_PRIMARY_MAX) ==
           (int)(ATT_BIT(ATT_SUPPRESSOR) | ATT_BIT(ATT_REFLEX) | ATT_BIT(ATT_RAPIDFIRE)));
    assert(BG_AttachmentsParse("thompson_mp", "none", 3) == 0 && BG_AttachmentsParse("thompson_mp", "", 3) == 0);
    assert(BG_AttachmentsParse("thompson_mp", NULL, 3) == 0 && BG_AttachmentsParse("colt_mp", "suppressor", 1) > 0);
    {
        const char *refused[] = { "grip+", "+grip", "grip++fmj", "grip+grip", "reflex+lens", "grip+fmj+extmag+quickdraw",
                                  "laser", "highcal", "none+grip", "GRIP", "a234567890123456789012345678901234" };
        for (i = 0; i < (int)(sizeof(refused) / sizeof(refused[0])); i++)
            assert(BG_AttachmentsParse("thompson_mp", refused[i], 3) < 0);
    }
    assert(BG_AttachmentsParse("colt_mp", "suppressor+extmag", 1) < 0 && BG_AttachmentsParse("webley_mp", "extmag", 1) < 0);
    assert(BG_AttachmentsParse("not_a_gun_mp", "grip", 3) < 0 && BG_AttachmentsParse("not_a_gun_mp", "none", 3) == 0);
    assert(BG_AttachmentsParseList("m1garand_mp", "optic4x+highcal", 3, ids) == 2 && ids[0] == ATT_OPTIC4X && ids[1] == ATT_HIGHCAL);
    assert(BG_AttachmentsValid("m1garand_mp", ATT_BIT(ATT_OPTIC4X) | ATT_BIT(ATT_GRIP), 2));
    assert(!BG_AttachmentsValid("m1garand_mp", ATT_BIT(ATT_OPTIC4X) | ATT_BIT(ATT_GRIP), 1));
    assert(!BG_AttachmentsValid("m1garand_mp", ATT_BIT(ATT_OPTIC4X) | ATT_BIT(ATT_LENS), 3));
    assert(!BG_AttachmentsValid("m1garand_mp", ATT_BIT(ATT_SUPPRESSOR), 3));
    ids[0] = ATT_SUPPRESSOR; ids[1] = ATT_REFLEX;
    assert(BG_AttachmentsFormatList(ids, 2, text, sizeof(text)) == 17 && !strcmp(text, "suppressor+reflex"));
    assert(BG_AttachmentsFormatList(ids, 0, text, sizeof(text)) == 0 && !text[0]);
    assert(BG_AttachmentsFormatList(ids, 2, text, 17) < 0 && BG_AttachmentsFormatList(ids, 2, text, 0) < 0);
    ids[1] = ATT_COUNT;
    assert(BG_AttachmentsFormatList(ids, 2, text, sizeof(text)) < 0);
    assert(BG_AttachmentConflict(ATT_BIT(ATT_REFLEX), ATT_LENS) == ATT_REFLEX);
    assert(BG_AttachmentConflict(ATT_BIT(ATT_REFLEX), ATT_REFLEX) == ATT_REFLEX);
    assert(BG_AttachmentConflict(ATT_BIT(ATT_REFLEX), ATT_GRIP) < 0 && BG_AttachmentConflict(0, -1) < 0);
    assert(BG_AttachmentConflict(ATT_BIT(ATT_GRIP) | ATT_BIT(ATT_OPTIC4X), ATT_REFLEX) == ATT_OPTIC4X);

    /* Each attachment on the weapon file values, with WWII's numbers. */
    d = Attached(THOMPSON, ATT_BIT(ATT_EXTMAG));
    assert(d.iClipSize == 30 && d.iMaxAmmo == 270 && d.iStartAmmo == 270 && d.iFireTime == 85);
    assert(d.iAdsTransInTime == 242 && NEAR(d.fOOPosAnimLength[0], 1.0f / 242.0f));
    d = Attached(KAR98K, ATT_BIT(ATT_EXTMAG));
    assert(d.iClipSize == 7 && d.iMaxAmmo == 90);
    d = Attached(COLT, ATT_BIT(ATT_EXTMAG));
    assert(d.iClipSize == 10 && d.iMaxAmmo == 31);
    d = Attached(WEBLEY, ATT_BIT(ATT_EXTMAG));
    assert(d.iClipSize == 6 && d.iMaxAmmo == 18);
    /* The Bren's 100-round drum, with two in reserve. */
    d = Attached(BREN, ATT_BIT(ATT_EXTMAG));
    assert(d.iClipSize == 100 && d.iMaxAmmo == 200 && d.iStartAmmo == 200);
    d = Attached(THOMPSON, ATT_BIT(ATT_RAPIDFIRE));
    assert(d.iFireTime == 80 && d.iRechamberTime == 94);
    assert(NEAR(d.fAdsViewKickPitchMax, 80.5f) && NEAR(d.fAdsGunKickPitchMin, 46.0f) && d.iAdsTransInTime == 220);
    /* The Bren and Trench Gun gain the rifles' and SMGs' 6.38%. */
    d = Attached(BREN, ATT_BIT(ATT_RAPIDFIRE));
    assert(d.iFireTime == 113);
    d = Attached(SHOTGUN, ATT_BIT(ATT_RAPIDFIRE));
    assert(d.iFireTime == 266 && d.iRechamberTime == 689 && d.iRechamberBoltTime == 344);
    d = Attached(KAR98K_SNIPER, ATT_BIT(ATT_RAPIDFIRE));
    assert(d.iFireTime == 313 && d.iRechamberTime == 949);
    /* Quickdraw: a third off an SMG's aim, and a harder kick. */
    d = Attached(THOMPSON, ATT_BIT(ATT_QUICKDRAW));
    assert(d.iAdsTransInTime == 143 && NEAR(d.fOOPosAnimLength[0], 1.0f / 143.0f) && NEAR(d.fOOPosAnimLength[1], 1.0f / 400.0f));
    assert(NEAR(d.fAdsViewKickPitchMax, 80.5f));
    assert(Attached(BREN, ATT_BIT(ATT_QUICKDRAW)).iAdsTransInTime == 240);
    assert(Attached(GARAND, ATT_BIT(ATT_QUICKDRAW)).iAdsTransInTime == 252);
    assert(Attached(COLT, ATT_BIT(ATT_QUICKDRAW)).iAdsTransInTime == 224);
    assert(Attached(SHOTGUN, ATT_BIT(ATT_QUICKDRAW)).iAdsTransInTime == 252);
    /* Grip: 15% off every kick, a tenth on the aim. */
    d = Attached(THOMPSON, ATT_BIT(ATT_GRIP));
    assert(NEAR(d.fAdsViewKickPitchMax, 59.5f) && NEAR(d.fAdsViewKickYawMin, -17.0f) && NEAR(d.fAdsGunKickPitchMin, 34.0f));
    assert(d.iAdsTransInTime == 242);
    d = Attached(THOMPSON, ATT_BIT(ATT_STEADYAIM));
    assert(NEAR(d.fHipSpreadStandMin, 1.125f) && d.iAdsTransInTime == 242);
    d = Attached(GREASEGUN, ATT_BIT(ATT_STEADYAIM));
    assert(NEAR(d.fHipSpreadStandMin, 0.9f) && NEAR(d.fHipSpreadProneMin, 0.54f) && NEAR(d.hipSpreadProneMax, 3.0f));
    /* The Trench Gun's pellets a quarter tighter, as through its Reflex Sight. */
    d = Attached(SHOTGUN, ATT_BIT(ATT_STEADYAIM));
    assert(NEAR(d.fHipSpreadStandMin, 4.5f) && NEAR(d.hipSpreadStandMax, 3.75f) && d.iAdsTransInTime == 330);
    d = Attached(KAR98K_SNIPER, ATT_BIT(ATT_CALIBRATION));
    assert(NEAR(d.fAdsIdleAmount, 12.0f) && NEAR(d.adsSwayMaxAngle, 2.4f) && NEAR(d.adsSwayVertScale, 1.2f));
    assert(d.iAdsTransInTime == 275);
    d = Attached(THOMPSON, ATT_BIT(ATT_REFLEX));
    assert(NEAR(d.fAdsZoomFov, 58.5f) && NEAR(d.fAdsSpread, 0.4f) && !d.szOverlayMaterial[0] && d.iAdsTransInTime == 242);
    d = Attached(THOMPSON, ATT_BIT(ATT_LENS));
    assert(NEAR(d.fAdsZoomFov, 52.0f) && NEAR(d.fAdsSpread, 0.425f) && d.iAdsTransInTime == 253);
    /* The costs multiply with the gains: two tenths on and a third off. */
    d = Attached(THOMPSON, ATT_BIT(ATT_REFLEX) | ATT_BIT(ATT_GRIP) | ATT_BIT(ATT_QUICKDRAW));
    assert(d.iAdsTransInTime == 173 && NEAR(d.fAdsViewKickPitchMax, 68.425f));
    d = Attached(GARAND, ATT_BIT(ATT_OPTIC4X));
    assert(NEAR(d.fAdsZoomFov, 23.7f) && !strcmp(d.szOverlayMaterial, "scope_overlay_american") && d.overlayReticle == 3);
    assert(d.iAdsTransInTime == 375);
    assert(d.overlayWidth == 480.0f && d.overlayHeight == 480.0f && NEAR(d.fAdsZoomInFrac, 0.42f) && NEAR(d.fAdsZoomOutFrac, 0.08f));
    d = Attached(KAR98K, ATT_BIT(ATT_OPTIC4X));
    assert(!strcmp(d.szOverlayMaterial, "scope_overlay_german") && d.overlayReticle == 2);
    d = Attached(KAR98K_SNIPER, ATT_BIT(ATT_OPTIC4X));
    assert(NEAR(d.fAdsZoomFov, 23.7f) && d.szOverlayMaterial == defs[KAR98K_SNIPER].szOverlayMaterial);
    assert(d.overlayReticle == 2 && NEAR(d.fAdsZoomInFrac, 0.5f));
    /* FMJ goes through more cover for a tenth of the range. */
    d = Attached(THOMPSON, ATT_BIT(ATT_FMJ));
    assert(d.armorPiercing == 1 && NEAR(d.fMaxDamageRange, 720.0f) && NEAR(d.fMinDamageRange, 1440.0f));
    assert(d.iAdsTransInTime == 220 && NEAR(d.fAdsViewKickPitchMax, 70.0f));
    /* A gun whose damage never falls pays for it in aim time instead. */
    d = Attached(GARAND, ATT_BIT(ATT_FMJ));
    assert(d.armorPiercing == 1 && NEAR(d.fMaxDamageRange, 2000.0f) && NEAR(d.fMinDamageRange, 4000.0f));
    assert(d.iAdsTransInTime == 330);
    d = Attached(KAR98K_SNIPER, ATT_BIT(ATT_FMJ));
    assert(d.armorPiercing == 1 && NEAR(d.fMaxDamageRange, 2000.0f) && d.iAdsTransInTime == 275);
    d = Attached(GARAND, ATT_BIT(ATT_HIGHCAL));
    assert(NEAR(d.locationDamageMultipliers[HITLOC_HEAD], 3.0f) && NEAR(d.locationDamageMultipliers[HITLOC_HELMET], 3.0f));
    assert(NEAR(d.locationDamageMultipliers[4], 1.0f) && NEAR(d.fAdsViewKickPitchMax, 80.5f));
    /* An automatic rifle's headshot stops a point short of killing alone,
     * after the server truncates it, and never falls below the gun's own. */
    d = Attached(BAR, ATT_BIT(ATT_HIGHCAL));
    assert(NEAR(d.locationDamageMultipliers[HITLOC_HEAD], 99.5f / 40.0f) &&
           NEAR(d.locationDamageMultipliers[HITLOC_HELMET], 99.5f / 40.0f));
    assert((int)(40.0f * d.locationDamageMultipliers[HITLOC_HEAD]) == 99);
    {
        WeaponDef high = defs[BAR];
        high.locationDamageMultipliers[HITLOC_HEAD] = 3.0f;
        BG_ApplyAttachments(&high, ATT_BIT(ATT_HIGHCAL));
        assert(NEAR(high.locationDamageMultipliers[HITLOC_HEAD], 3.0f));
    }
    /* An SMG keeps its full damage range; the fall beyond it reaches a
     * quarter further. */
    d = Attached(THOMPSON, ATT_BIT(ATT_RIFLING));
    assert(NEAR(d.fMaxDamageRange, 800.0f) && NEAR(d.fMinDamageRange, 2000.0f) && d.iAdsTransInTime == 242);
    /* The semi-automatic rifles keep full damage at every range: Advanced
     * Rifling is not theirs and changes nothing on them. */
    assert(!(BG_AttachmentsAllowed("m1garand_mp") & ATT_BIT(ATT_RIFLING)) &&
           !(BG_AttachmentsAllowed("m1carbine_mp") & ATT_BIT(ATT_RIFLING)) &&
           !(BG_AttachmentsAllowed("SVT40_mp") & ATT_BIT(ATT_RIFLING)) &&
           !(BG_AttachmentsAllowed("g43_mp") & ATT_BIT(ATT_RIFLING)));
    d = Attached(GARAND, ATT_BIT(ATT_RIFLING));
    assert(!memcmp(&d, &defs[GARAND], sizeof(d)));
    assert(NEAR(Attached(BAR, ATT_BIT(ATT_RIFLING)).fMaxDamageRange, 1875.0f));
    /* The MP44 shares the BAR's damage model, and so its rifling. */
    {
        AttachmentStats bar, mp44;
        memset(&bar, 0, sizeof(bar));
        bar.overlay = "";
        bar.maxDamageRange = 1200.0f;
        bar.minDamageRange = 2500.0f;
        bar.damage = 40;
        bar.minDamage = 28;
        mp44 = bar;
        BG_ApplyAttachmentStats("bar_mp", ATT_BIT(ATT_RIFLING), &bar);
        BG_ApplyAttachmentStats("mp44_mp", ATT_BIT(ATT_RIFLING), &mp44);
        assert(NEAR(mp44.maxDamageRange, 1500.0f) && NEAR(mp44.minDamageRange, 3125.0f) && !memcmp(&bar, &mp44, sizeof(bar)));
    }
    /* WWII's light machine guns take neither Advanced Rifling nor High Caliber, nor its carbine a Lens Sight. */
    assert(NEAR(Attached(BREN, ATT_BIT(ATT_RIFLING)).fMaxDamageRange, 1200.0f));
    assert(NEAR(Attached(BREN, ATT_BIT(ATT_HIGHCAL)).locationDamageMultipliers[HITLOC_HEAD],
                defs[BREN].locationDamageMultipliers[HITLOC_HEAD]));
    assert(!(BG_AttachmentsAllowed("bren_mp") & (ATT_BIT(ATT_RIFLING) | ATT_BIT(ATT_HIGHCAL))));
    assert(!(BG_AttachmentsAllowed("m1carbine_mp") & ATT_BIT(ATT_LENS)) &&
           (BG_AttachmentsAllowed("m1garand_mp") & ATT_BIT(ATT_LENS)));
    assert(NEAR(Attached(COLT, ATT_BIT(ATT_RIFLING)).fMaxDamageRange, 480.0f));
    assert(NEAR(Attached(SHOTGUN, ATT_BIT(ATT_RIFLING)).fMaxDamageRange, 433.92f));
    assert(NEAR(Attached(THOMPSON, ATT_BIT(ATT_SUPPRESSOR)).fMaxDamageRange, 560.0f));
    assert(NEAR(Attached(COLT, ATT_BIT(ATT_SUPPRESSOR)).fMaxDamageRange, 326.4f));
    /* A suppressor on rifling: full damage as suppressed, the fall an
     * eighth short of the gun's own. */
    d = Attached(THOMPSON, ATT_BIT(ATT_SUPPRESSOR) | ATT_BIT(ATT_RIFLING));
    assert(NEAR(d.fMaxDamageRange, 560.0f) && NEAR(d.fMinDamageRange, 1400.0f));
    d = Attached(THOMPSON, ATT_BIT(ATT_SUPPRESSOR) | ATT_BIT(ATT_FMJ));
    assert(NEAR(d.fMaxDamageRange, 504.0f) && NEAR(d.fMinDamageRange, 1008.0f));
    /* Nothing a weapon cannot take, and nothing on an unknown weapon. */
    d = Attached(GARAND, ATT_BIT(ATT_SUPPRESSOR) | ATT_BIT(ATT_CALIBRATION));
    assert(!memcmp(&d, &defs[GARAND], sizeof(d)));
    defs[0].szInternalName = "binoculars_mp";
    d = Attached(0, 0x1fff);
    assert(!memcmp(&d, &defs[0], sizeof(d)));
    assert(Attached(PPS42, ATT_BIT(ATT_EXTMAG)).iClipSize == 52);
    {
        AttachmentStats stats;
        memset(&stats, 0, sizeof(stats));
        stats.overlay = "";
        BG_ApplyAttachmentStats("thompson_mp", ATT_BIT(ATT_SUPPRESSOR), &stats);
        assert(stats.suppressed == 1);
        stats.suppressed = 0;
        BG_ApplyAttachmentStats("thompson_mp", ATT_BIT(ATT_FMJ), &stats);
        assert(!stats.suppressed);
    }

    /* The player state: a mask a slot, in bytes the slots never use. */
    ps = Holding(THOMPSON, 0x1abc, COLT, 0x0101);
    assert((unsigned char)ps.weaponslots[4] == 0xbc && (unsigned char)ps.weaponslots[5] == 0x1a);
    assert((unsigned char)ps.weaponslots[6] == 0x01 && (unsigned char)ps.weaponslots[7] == 0x01);
    assert(ps.weaponslots[1] == THOMPSON && ps.weaponslots[2] == COLT && !ps.weaponslots[0] && !ps.weaponslots[3]);
    assert(BG_GetSlotAttachments(&ps, 1) == 0x1abc && BG_GetSlotAttachments(&ps, 2) == 0x0101);
    assert(BG_GetSlotAttachments(&ps, 0) == 0 && BG_GetSlotAttachments(&ps, 3) == 0);
    BG_SetSlotAttachments(&ps, 0, 0xffff);
    BG_SetSlotAttachments(&ps, 3, 0xffff);
    assert(!ps.weaponslots[0] && !ps.weaponslots[3] && ps.weaponslots[1] == THOMPSON);
    assert(BG_GetWeaponAttachments(&ps, THOMPSON) == 0x1abc && BG_GetWeaponAttachments(&ps, COLT) == 0x0101);
    assert(BG_GetWeaponAttachments(&ps, GARAND) == 0 && BG_GetWeaponAttachments(&ps, 0) == 0);

    /* A player's definition: the shared one without attachments, a kept
     * copy with them, and the same copy for the same set. */
    ps = Holding(THOMPSON, 0, COLT, 0);
    assert(BG_GetPlayerWeaponDef(&ps, THOMPSON) == &defs[THOMPSON] && BG_GetPlayerWeaponDef(NULL, THOMPSON) == &defs[THOMPSON]);
    assert(BG_GetPlayerClipSize(&ps, THOMPSON) == 20 && BG_GetPlayerMaxAmmo(&ps, THOMPSON) == 180);
    ps = Holding(THOMPSON, ATT_BIT(ATT_EXTMAG), COLT, ATT_BIT(ATT_SUPPRESSOR));
    {
        WeaponDef *first = BG_GetPlayerWeaponDef(&ps, THOMPSON);
        assert(first != &defs[THOMPSON] && first->iClipSize == 30 && defs[THOMPSON].iClipSize == 20);
        assert(BG_GetPlayerWeaponDef(&ps, THOMPSON) == first);
        assert(BG_GetPlayerClipSize(&ps, THOMPSON) == 30 && BG_GetPlayerMaxAmmo(&ps, THOMPSON) == 270);
        assert(NEAR(BG_GetPlayerWeaponDef(&ps, COLT)->fMaxDamageRange, 326.4f) && BG_GetPlayerClipSize(&ps, COLT) == 7);
        BG_SetSlotAttachments(&ps, 1, ATT_BIT(ATT_EXTMAG) | ATT_BIT(ATT_RAPIDFIRE));
        assert(BG_GetPlayerWeaponDef(&ps, THOMPSON) != first && BG_GetPlayerWeaponDef(&ps, THOMPSON)->iFireTime == 80);
    }
    /* More sets than the cache holds: each is rebuilt right when needed. */
    for (i = 0; i < 1024; i++) {
        unsigned mask = (unsigned)i & BG_AttachmentsAllowed("thompson_mp");
        WeaponDef *def;
        BG_SetSlotAttachments(&ps, 1, mask);
        def = BG_GetPlayerWeaponDef(&ps, THOMPSON);
        assert(def->iClipSize == (mask & ATT_BIT(ATT_EXTMAG) ? 30 : 20));
        assert(def->iFireTime == (mask & ATT_BIT(ATT_RAPIDFIRE) ? 80 : 85));
        assert(def->iAdsTransInTime == Attached(THOMPSON, mask).iAdsTransInTime);
        assert(def->fAdsViewKickPitchMax == Attached(THOMPSON, mask).fAdsViewKickPitchMax);
    }
    /* Weapons reload with each map: copies follow the new definitions. */
    BG_SetSlotAttachments(&ps, 1, ATT_BIT(ATT_EXTMAG));
    assert(BG_GetPlayerWeaponDef(&ps, THOMPSON)->iClipSize == 30);
    defs[THOMPSON].iClipSize = 50;
    BG_ResetAttachedWeaponDefs();
    assert(BG_GetPlayerWeaponDef(&ps, THOMPSON)->iClipSize == 75);
    defs[THOMPSON].iClipSize = 20;
    BG_ResetAttachedWeaponDefs();

    /* The production weapon code with the player's attachments. */
    ps = Holding(THOMPSON, ATT_BIT(ATT_EXTMAG), COLT, 0);
    ps.weaponstate = WEAPON_RELOADING;
    ps.ammo[THOMPSON] = 270;
    PM_ReloadClip(&ps);
    assert(ps.ammoclip[THOMPSON] == 30 && ps.ammo[THOMPSON] == 240);
    ps.ammoclip[THOMPSON] = 20;
    assert(PM_Weapon_AllowReload(&ps));
    BG_SetSlotAttachments(&ps, 1, 0);
    assert(!PM_Weapon_AllowReload(&ps));
    ps.ammoclip[THOMPSON] = 0;
    PM_ReloadClip(&ps);
    assert(ps.ammoclip[THOMPSON] == 20 && ps.ammo[THOMPSON] == 220);

    {
        float low, high;
        ps = Holding(THOMPSON, ATT_BIT(ATT_STEADYAIM), COLT, 0);
        BG_GetSpreadForWeapon(&ps, THOMPSON, &low, &high);
        assert(NEAR(low, 1.125f) && NEAR(high, 3.75f));
        BG_GetSpreadForWeapon(&ps, COLT, &low, &high);
        assert(NEAR(low, 3.0f) && NEAR(high, 5.0f));
    }

    ps = Holding(THOMPSON, ATT_BIT(ATT_RAPIDFIRE), COLT, 0);
    PM_StartFireTimers(&ps, BG_GetPlayerWeaponDef(&ps, THOMPSON));
    assert(ps.weaponTime == 80 && ps.weaponstate == WEAPON_FIRING);

    ps = Holding(THOMPSON, ATT_BIT(ATT_SUPPRESSOR), COLT, 0);
    ps.ammoclip[THOMPSON] = 5;
    assert(PM_ConsumeFireAmmo(&ps, BG_GetPlayerWeaponDef(&ps, THOMPSON)));
    assert(lastEvent == 0x9e && lastParm == 1 && ps.ammoclip[THOMPSON] == 4);
    ps.ammoclip[THOMPSON] = 1;
    assert(PM_ConsumeFireAmmo(&ps, BG_GetPlayerWeaponDef(&ps, THOMPSON)) && lastEvent == 0xa0 && lastParm == 1);
    BG_SetSlotAttachments(&ps, 1, ATT_BIT(ATT_RIFLING));
    ps.ammoclip[THOMPSON] = 5;
    assert(PM_ConsumeFireAmmo(&ps, BG_GetPlayerWeaponDef(&ps, THOMPSON)) && lastEvent == 0x9e && lastParm == 0);

    {
        pmove_t pm = { &ps, { 0, 0 } };
        pml_t pml = { 44 };
        ps = Holding(THOMPSON, ATT_BIT(ATT_QUICKDRAW), COLT, 0);
        ps.pm_flags = 0x40;
        PM_UpdateAimDownSightLerp(&pm, &pml);
        assert(NEAR(ps.fWeaponPosFrac, 44.0f / 143.0f));
        BG_SetSlotAttachments(&ps, 1, 0);
        ps.fWeaponPosFrac = 0.0f;
        PM_UpdateAimDownSightLerp(&pm, &pml);
        assert(NEAR(ps.fWeaponPosFrac, 0.2f));
    }

    {
        vec3_t gun = { 0, 0, 0 }, kick;
        ps = Holding(THOMPSON, ATT_BIT(ATT_GRIP), COLT, 0);
        ps.fWeaponPosFrac = 1.0f;
        BG_WeaponFireRecoil(&ps, gun, kick);
        assert(NEAR(kick[0], -55.25f) && NEAR(gun[0], 34.0f));
        BG_SetSlotAttachments(&ps, 1, 0);
        gun[0] = 0.0f;
        BG_WeaponFireRecoil(&ps, gun, kick);
        assert(NEAR(kick[0], -65.0f) && NEAR(gun[0], 40.0f));
    }

    /* A weapon leaves its slot without its attachments; the other keeps its own. */
    ps = Holding(THOMPSON, ATT_BIT(ATT_EXTMAG), COLT, ATT_BIT(ATT_SUPPRESSOR));
    assert(BG_TakePlayerWeapon(&ps, THOMPSON));
    assert(!ps.weaponslots[1] && !ps.weaponslots[4] && !ps.weaponslots[5]);
    assert(BG_GetSlotAttachments(&ps, 2) == ATT_BIT(ATT_SUPPRESSOR) && ps.weaponslots[2] == COLT);
    assert(!BG_TakePlayerWeapon(&ps, THOMPSON));

    /* A gun on the ground carries its camouflage and every attachment in the ten bits each of its legs and torso
     * animations, and nothing else. */
    {
        entityState_t es = { 0, 0 };
        unsigned every = (1u << ATT_COUNT) - 1;
        BG_SetItemWeaponState(&es, 10, every);
        assert(BG_GetItemWeaponCamo(&es) == 10 && BG_GetItemWeaponAttachments(&es) == every);
        assert(es.legsAnim >= 0 && es.legsAnim < 1024 && es.torsoAnim >= 0 && es.torsoAnim < 1024);
        for (int camo = 0; camo <= 10; camo++)
            for (int a = 0; a < ATT_COUNT; a++) {
                unsigned mask = ATT_BIT(a) | ATT_BIT((a + 5) % ATT_COUNT);
                BG_SetItemWeaponState(&es, camo, mask);
                assert(BG_GetItemWeaponCamo(&es) == camo && BG_GetItemWeaponAttachments(&es) == mask);
            }
        BG_SetItemWeaponState(&es, 0, 0);
        assert(!es.legsAnim && !es.torsoAnim && !BG_GetItemWeaponCamo(&es) && !BG_GetItemWeaponAttachments(&es));
        /* What the snapshot's ten bits would leave of a wider value is all that is read. */
        es.legsAnim = 0x400 | 5;
        es.torsoAnim = 0x400 | 3 << 6;
        assert(BG_GetItemWeaponAttachments(&es) == 5 && BG_GetItemWeaponCamo(&es) == 3);
    }
    puts("PASS: WWII's lists and limits, every attachment's numbers, the slots' masks in the player state, "
         "cached player definitions, and reload, spread, fire rate, aim time, recoil and suppressed shots; a "
         "dropped gun's camouflage and attachments in its animation fields");
    return 0;
}
'''

mutants = [
    (rules_path, '           BG_AttachmentCount(mask & ATT_OPTICS) <= 1;', '           1;'),
    (rules_path, '        if (id < 0 || (mask & ATT_BIT(id)))\n', '        if (id < 0)\n'),
    (rules_path, '    return !(mask & ~BG_AttachmentsAllowed(weapon)) && BG_AttachmentCount(mask) <= max &&',
     '    return BG_AttachmentCount(mask) <= max &&'),
    (rules_path, '    mask &= entry->allowed;\n', ''),
    (rules_path, '        stats->clipSize = stats->clipSize * 3 / 2;', '        stats->clipSize = stats->clipSize * 2;'),
    (rules_path, 'kind == ATT_KIND_LMG ? 0.8f : 0.84f', 'kind == ATT_KIND_LMG ? 0.84f : 0.84f'),
    (rules_path, 'kind == ATT_KIND_SMG || kind == ATT_KIND_FAST_SMG ? 0.65f', 'kind == ATT_KIND_SMG || kind == ATT_KIND_FAST_SMG ? 0.5f'),
    (rules_path, '        kick *= 0.85f;', '        kick *= 0.9f;'),
    (rules_path, '        stats->adsSpread *= 0.8f;\n        aim *= 1.1f;\n', '        stats->adsSpread *= 0.8f;\n'),
    (rules_path, '        aim *= 1.15f;\n', ''),
    (rules_path, '        aim *= 1.25f;\n', ''),
    (rules_path, '0.84f;\n        kick *= 1.15f;\n', '0.84f;\n'),
    (rules_path, '        kick *= 0.85f;\n        aim *= 1.1f;\n', '        kick *= 0.85f;\n'),
    (rules_path, '            stats->hipSpread[i] *= scale;\n        aim *= 1.1f;\n', '            stats->hipSpread[i] *= scale;\n'),
    (rules_path, '            stats->adsSway[i] *= 0.6f;\n        aim *= 1.1f;\n', '            stats->adsSway[i] *= 0.6f;\n'),
    (rules_path, '(stats->rechamberBoltTime, rate);\n        kick *= 1.15f;\n', '(stats->rechamberBoltTime, rate);\n'),
    (rules_path, '            stats->startAmmo = stats->startAmmo * 3 / 2;\n        }\n        aim *= 1.1f;\n',
     '            stats->startAmmo = stats->startAmmo * 3 / 2;\n        }\n'),
    (rules_path, '            reach *= 0.9f;\n        else\n', '            ;\n        else\n'),
    (rules_path, '        if (stats->minDamage < stats->damage)\n', '        if (1)\n'),
    (rules_path, '        else\n            aim *= 1.1f;\n', '        else\n            ;\n'),
    (rules_path, 'SMG_ATTACHMENTS & ~A(EXTMAG) }', 'SMG_ATTACHMENTS }'),
    (rules_path, '#define SEMI_RIFLE_ATTACHMENTS (RIFLE_ATTACHMENTS & ~A(RIFLING))', '#define SEMI_RIFLE_ATTACHMENTS (RIFLE_ATTACHMENTS)'),
    (glue_path, '    stats->minDamage = def->minDamage;\n', ''),
    (rules_path, '            stats->headMultiplier[i] = raised;\n        }\n        kick *= 1.15f;\n',
     '            stats->headMultiplier[i] = raised;\n        }\n'),
    (rules_path, '            stats->minDamageRange *= scale;\n        }\n        aim *= 1.1f;\n',
     '            stats->minDamageRange *= scale;\n        }\n'),
    (rules_path, 'float scale = kind == ATT_KIND_SHOTGUN ? 1.13f : 1.25f;', 'float scale = kind == ATT_KIND_SHOTGUN ? 1.13f : 1.5f;'),
    (rules_path, '(int)((float)stats->adsTransInTime * aim + 0.5f);', '(int)((float)stats->adsTransInTime * aim);'),
    (rules_path, '    for (i = 0; i < 8; i++)\n        stats->adsKick[i] *= kick;\n',
     '    for (i = 0; i < 2; i++)\n        stats->adsKick[i] *= kick;\n'),
    (rules_path, '    stats->minDamageRange *= reach;\n', ''),
    (rules_path, '        stats->adsIdleAmount *= 0.6f;', '        stats->adsIdleAmount *= 0.5f;'),
    (rules_path, '? 1.054f : 1.0638f;', '? 1.054f : kind == ATT_KIND_LMG ? 1.0989f : 1.0638f;'),
    (rules_path, '            stats->clipSize = 100;', '            stats->clipSize = stats->clipSize * 3 / 2;'),
    (rules_path, '            if (cap > 0.0f && raised > cap)', '            if (0)'),
    (rules_path, 'float cap = !stats->semiAuto && !stats->boltAction &&', 'float cap = !stats->boltAction &&'),
    (rules_path, '                raised = cap > stats->headMultiplier[i] ? cap : stats->headMultiplier[i];', '                raised = cap;'),
    (rules_path, '            stats->minDamageRange *= 1.25f;\n', '            stats->minDamageRange *= 1.25f;\n            stats->maxDamageRange *= 1.25f;\n'),
    (rules_path, 'return time > 0 ? (int)((float)time / rate + 0.5f) : time;', 'return time > 0 ? (int)((float)time / rate) : time;'),
    (rules_path, '        reach *= kind == ATT_KIND_PISTOL ? 0.85f : 0.7f;', '        reach *= 0.7f;'),
    (rules_path, "int ca = *a >= 'A' && *a <= 'Z' ? *a + 32 : *a, cb = *b >= 'A' && *b <= 'Z' ? *b + 32 : *b;",
     'int ca = *a, cb = *b;'),
    (rules_path, '        float scale = kind == ATT_KIND_FAST_SMG || kind == ATT_KIND_SHOTGUN ? 0.75f : 0.6f;',
     '        float scale = 0.6f;'),
    (rules_path, 'kind == ATT_KIND_FAST_SMG || kind == ATT_KIND_SHOTGUN ? 0.75f', 'kind == ATT_KIND_FAST_SMG ? 0.75f'),
    (rules_path, '        if (!stats->overlay || !stats->overlay[0]) {', '        {'),
    (glue_path, '    return (unsigned char)ps->weaponslots[2 + 2 * slot] | (unsigned)(unsigned char)ps->weaponslots[3 + 2 * slot] << 8;',
     '    return (unsigned char)ps->weaponslots[1 + 2 * slot] | (unsigned)(unsigned char)ps->weaponslots[2 + 2 * slot] << 8;'),
    (glue_path, '    if ((unsigned char)ps->weaponslots[2] == weapon)\n        return BG_GetSlotAttachments(ps, 2);\n', ''),
    (glue_path, '            if (s_attachedDefs[i].weapon == weapon && s_attachedDefs[i].mask == mask)\n',
     '            if (s_attachedDefs[i].weapon == weapon)\n'),
    (glue_path, '    if (index >= 0 && s_attachedDefs[index].weapon == weapon && s_attachedDefs[index].mask == mask) {',
     '    if (index >= 0 && s_attachedDefs[index].weapon == weapon) {'),
    (glue_path, '    if (def->iAdsTransInTime > 0)\n        def->fOOPosAnimLength[0] = 1.0f / (float)def->iAdsTransInTime;\n', ''),
    (glue_path, '    return BG_GetAttachedWeaponDef(weapon, mask)->iClipSize;', '    return BG_GetAmmoClipSize(bg_weaponDefs[weapon]->iClipIndex);'),
    (glue_path, '    s_attachedDefCount = 0;\n', ''),
    (glue_path, '    stats->semiAuto = def->bSemiAuto;\n', ''),
    (glue_path, '    es->torsoAnim = (int)((attachments >> 10 & 0x3f) | (unsigned)(camo & 0xf) << 6);',
     '    es->torsoAnim = (int)((unsigned)(camo & 0xf) << 6);'),
    (glue_path, '    return (int)((unsigned)es->torsoAnim >> 6 & 0xf);', '    return (int)((unsigned)es->torsoAnim >> 6);'),
    (glue_path, '    return ((unsigned)es->legsAnim & 0x3ff) | ((unsigned)es->torsoAnim & 0x3f) << 10;',
     '    return (unsigned)es->legsAnim | ((unsigned)es->torsoAnim & 0x3f) << 10;'),
    (glue_path, '    es->legsAnim = (int)(attachments & 0x3ff);', '    es->legsAnim = (int)attachments;'),
    (weapons_path, '    clipSize = BG_GetPlayerClipSize(ps, ps->weapon);\n    ammoToAdd = clipSize - clipAmmo;',
     '    clipSize = bg_weapClips[clipIndex]->iClipSize;\n    ammoToAdd = clipSize - clipAmmo;'),
    (weapons_path, '        BG_SetSlotAttachments(pPS, slot, 0);\n', ''),
    (weapons_path, '(BG_GetWeaponAttachments(ps, ps->weapon) & ATT_BIT(ATT_SUPPRESSOR)) != 0', '0'),
    (weapons_path, '    WeaponDef *weapDef = BG_GetPlayerWeaponDef(ps, BG_GetViewmodelWeaponIndex(ps));\n    qboolean adsRequested;',
     '    WeaponDef *weapDef = bg_weaponDefs[BG_GetViewmodelWeaponIndex(ps)];\n    qboolean adsRequested;'),
    (weapons_path, '    const WeaponDef *weapDef = BG_GetPlayerWeaponDef(ps, weaponIndex);\n    float frac;',
     '    const WeaponDef *weapDef = bg_weaponDefs[weaponIndex];\n    float frac;'),
    (weapons_path, '    WeaponDef *weapDef = BG_GetPlayerWeaponDef(ps, BG_GetViewmodelWeaponIndex_core(ps));',
     '    WeaponDef *weapDef = bg_weaponDefs[BG_GetViewmodelWeaponIndex_core(ps)];'),
]

extracted = ['BG_IsPlayerWeaponInSlot_core', 'BG_TakePlayerWeapon', 'BG_GetSpreadForWeapon', 'PM_ReloadClip',
             'PM_Weapon_AllowReload', 'BG_RandomBetween', 'BG_WeaponFireRecoil', 'PM_UpdateAimDownSightLerp',
             'PM_AddFireSpread', 'PM_TryTakeEmptyClipOnlyWeapon', 'PM_ConsumeFireAmmo', 'PM_StartFireTimers']


def program(rules, glue, weapons):
    constants = weapons[weapons.index('#define WEAPON_READY'):weapons.index('static qboolean PM_WeaponHasInventoryWeapon')]
    body = ''.join(function(weapons, name) for name in extracted)
    return (support + constants + f'#include "{rules}"\n#include "{glue}"\n' + body + checks)


with tempfile.TemporaryDirectory(prefix='cod2-attachments-') as tmp:
    path = Path(tmp)
    (path / 'include').mkdir()
    (path / 'include/common_types.h').write_text(stub_types)
    flags = ['-std=gnu99', '-I', str(path / 'include'), '-I', str(root / 'src'),
             '-fsanitize=address,undefined', '-fno-sanitize-recover=all']
    sources = {rules_path: rules_path.read_text(), glue_path: glue_path.read_text(), weapons_path: weapons_path.read_text()}
    (path / 'test.c').write_text(program(rules_path, glue_path, sources[weapons_path]))
    subprocess.run(['cc', *flags, '-O1', '-g', '-Wall', '-Werror', '-Wno-unused-function', str(path / 'test.c'),
                    '-lm', '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
    for index, (target, before, after) in enumerate(mutants):
        assert sources[target].count(before) == 1, before
        changed = {**sources, target: sources[target].replace(before, after)}
        files = {}
        for name in (rules_path, glue_path):
            files[name] = path / f'mutant{index}-{name.name}'
            files[name].write_text(changed[name])
        (path / f'mutant{index}.c').write_text(program(files[rules_path], files[glue_path], changed[weapons_path]))
        subprocess.run(['cc', *flags, '-O0', '-w', str(path / f'mutant{index}.c'), '-lm', '-o',
                        str(path / f'mutant{index}')], check=True)
        result = subprocess.run([str(path / f'mutant{index}')], capture_output=True)
        assert result.returncode != 0, f'mutant survived: {after!r}'
print(f'PASS: the attachment rules, player state and weapon code; {len(mutants)} mutants fail')
