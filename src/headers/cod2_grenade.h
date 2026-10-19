#ifndef COD2_GRENADE_H
#define COD2_GRENADE_H

/* The remaining unused bit in the existing 27-bit playerstate field. */
#define PMF_GRENADE_THROWBACK 0x04000000
#define GRENADE_THROWBACK_HINT 255
/* Its owner may take back the mine they stand by; cursorHintString names
 * its weapon (g_missile_mp.c). */
#define PLACED_PICKUP_HINT 254
/* How far off the squad emblem over a placed explosive shows: the server
 * sends the explosive to its owner's side that far, walls or not
 * (g_missile_mp.c), and the cgame draws the emblem (cg_explosives_mp.c). */
#define PLACED_EMBLEM_RANGE 1600.0f
#define GRENADE_THROWBACK_RANGE 64.0f

/* CoD4's stickiness names, in its order. */
#define WEAPSTICKINESS_NONE 0
#define WEAPSTICKINESS_ALL 1
#define WEAPSTICKINESS_GROUND 2
/* The first event number after EV_OBITUARY: a detonator pressed. */
#define EV_DETONATE 0xc7

/* A mine waits for an enemy and a charge for its detonator, never for a fuse. */
static inline qboolean BG_IsPlacedExplosive(const WeaponDef *def)
{
    return def->weapType == WEAPTYPE_GRENADE && (def->hasDetonator || def->proximityRadius > 0.0f);
}

static inline qboolean BG_GrenadeCanCook(const WeaponDef *def)
{
    return def->weapType == WEAPTYPE_GRENADE && !BG_IsPlacedExplosive(def) &&
        (def->bCookOffHold || def->offhandClass == OFFHAND_CLASS_FRAG_GRENADE);
}

#endif
