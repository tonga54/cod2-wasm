#ifndef COD2_GRENADE_H
#define COD2_GRENADE_H

/* The remaining unused bit in the existing 27-bit playerstate field. */
#define PMF_GRENADE_THROWBACK 0x04000000
#define GRENADE_THROWBACK_HINT 255
/* Its owner may take back the mine they stand by; cursorHintString names
 * its weapon (g_missile_mp.c). */
#define PLACED_PICKUP_HINT 254
/* How far off the squad emblem over a placed mine shows: the server sends
 * the mine to its owner's side that far, walls or not (g_missile_mp.c), and
 * the cgame draws the emblem (cg_explosives_mp.c). */
#define PLACED_EMBLEM_RANGE 1600.0f
#define GRENADE_THROWBACK_RANGE 64.0f

/* A mine waits for an enemy, never for a fuse. */
static inline qboolean BG_IsPlacedExplosive(const WeaponDef *def)
{
    return def->weapType == WEAPTYPE_GRENADE && def->proximityRadius > 0.0f;
}

static inline qboolean BG_GrenadeCanCook(const WeaponDef *def)
{
    return def->weapType == WEAPTYPE_GRENADE && !BG_IsPlacedExplosive(def) &&
        (def->bCookOffHold || def->offhandClass == OFFHAND_CLASS_FRAG_GRENADE);
}

#endif
