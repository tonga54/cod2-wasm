#ifndef CG_ATTACHMENTSIGHT_H
#define CG_ATTACHMENTSIGHT_H

/* The Reflex and Lens Sight attachments' reticles, over the iron sights while
 * aiming: CoD2 has no models for them. x and y are the crosshair's centre in
 * virtual units. */
void CG_DrawAttachmentSight(const playerState_t *ps, int weapon, float x, float y);

#endif
