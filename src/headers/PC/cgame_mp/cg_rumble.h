#ifndef CG_RUMBLE_H
#define CG_RUMBLE_H

/* Controller vibration from the retail console rumble graphs, felt by the
 * local player in their own view: a kick for every round they fire, the hits
 * they take, their melee impacts and the blasts going off near them. */
void CG_InitRumble(void);
/* Every frame: mixes the playing graphs and drives the pad's two motors. */
void CG_UpdateRumble(void);
/* A fire event, of a player's weapon or a turret's. */
void CG_RumbleFireEvent(const entityState_t *es);
/* An explosion event at the origin, of the weapon that went off. */
void CG_RumbleExplosion(const vec_t *origin, int weapon);
/* earthquake(): its scale, duration in milliseconds and radius. */
void CG_RumbleEarthquake(const vec_t *origin, float intensity, int duration, float radius);
/* The health a hit took. */
void CG_RumbleDamage(int damage);
/* Each player state transition, for the local player's death. */
void CG_RumbleTransition(const playerState_t *ps, const playerState_t *ops);
/* A melee swing event, then the impact where a swing landed. */
void CG_RumbleMeleeSwing(const entityState_t *es);
void CG_RumbleMeleeImpact(const vec_t *origin);

#endif
