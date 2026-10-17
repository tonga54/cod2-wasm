#ifndef CG_GRENADEWARNING_H
#define CG_GRENADEWARNING_H

/* A live frag grenade that passed the stock warning's checks. */
typedef struct CG_GrenadeThreat {
    const centity_t *cent;
    float distanceSq;
    /* The blast radius, within cg_hudGrenadeIconMaxRange. */
    float range;
} CG_GrenadeThreat;

/* The WWII grenade warning: a white ring around a cream stick grenade near
 * the screen's edge toward the grenade, with a triangle outside the ring
 * pointing at it. It grows as the grenade nears and turns red where the blast
 * would kill. It stands in for the stock warning. */
void CG_GrenadeWarningInit(void);
int CG_GrenadeWarningActive(void);
/* threats are sorted nearest first. Returns 0, having drawn nothing, when the
 * stock warning should be drawn. */
int CG_DrawGrenadeWarnings(const CG_GrenadeThreat *threats, int count);

#endif
