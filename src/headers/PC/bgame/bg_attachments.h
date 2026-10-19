#ifndef BG_ATTACHMENTS_H
#define BG_ATTACHMENTS_H

#include "bg_attachment_rules.h"

/* A player's attachments for each weapon slot (1 primary, 2 primaryb) ride
 * in playerState weaponslots[4..7], bytes the multiplayer slots never use
 * and the snapshot already sends: prediction and the server see the same set.
 * A slot loses its attachments with its weapon. */
unsigned BG_GetSlotAttachments(const playerState_t *ps, int slot);
void BG_SetSlotAttachments(playerState_t *ps, int slot, unsigned mask);
/* The attachments on the weapon when it is in one of the slots. */
unsigned BG_GetWeaponAttachments(const playerState_t *ps, int weapon);
/* The weapon's definition with the player's attachments: the shared one when
 * there are none, else a copy kept until the weapons reload. */
WeaponDef *BG_GetPlayerWeaponDef(const playerState_t *ps, int weapon);
WeaponDef *BG_GetAttachedWeaponDef(int weapon, unsigned mask);
/* The weapon's clip and its ammunition's reserve for the player: the shared
 * clip and ammunition limits without attachments. */
int BG_GetPlayerClipSize(const playerState_t *ps, int weapon);
int BG_GetPlayerMaxAmmo(const playerState_t *ps, int weapon);
void BG_ApplyAttachments(WeaponDef *def, unsigned mask);
void BG_ResetAttachedWeaponDefs(void);
/* A gun on the ground keeps the camouflage and attachments it was dropped
 * with, in its item entity's player animation fields, which items never use
 * and the snapshot already sends: legsAnim holds the attachments' low ten
 * bits, torsoAnim the rest under the camouflage. */
void BG_SetItemWeaponState(entityState_t *es, int camo, unsigned attachments);
int BG_GetItemWeaponCamo(const entityState_t *es);
unsigned BG_GetItemWeaponAttachments(const entityState_t *es);

#endif
