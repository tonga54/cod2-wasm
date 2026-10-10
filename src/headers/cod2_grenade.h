#ifndef COD2_GRENADE_H
#define COD2_GRENADE_H

/* The remaining unused bit in the existing 27-bit playerstate field. */
#define PMF_GRENADE_THROWBACK 0x04000000
#define GRENADE_THROWBACK_HINT 255
#define GRENADE_THROWBACK_RANGE 64.0f

static inline qboolean BG_GrenadeCanCook(const WeaponDef *def)
{
    return def->weapType == WEAPTYPE_GRENADE &&
        (def->bCookOffHold || def->offhandClass == OFFHAND_CLASS_FRAG_GRENADE);
}

#endif
