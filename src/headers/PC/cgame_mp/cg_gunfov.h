#ifndef CG_GUNFOV_H
#define CG_GUNFOV_H

#include "common_types.h"

/* The arms and weapon drawn with a field of view of their own, as Call of
 * Duty: WWII frames them: cg_gunFov, a cg_fov-style angle (0 draws them with
 * the view's), narrower than the view's, so the weapon stands larger and the
 * forearm leaves the bottom of the screen. Raising the sights blends to the
 * view's own projection by the time a sight's window opens, so iron sights,
 * Reflex and Lens Sights and scopes aim as before. Effects bolted to the
 * weapon and the shooter's own tracer are drawn with the view's projection,
 * so they start where the weapon's projection shows the tag they leave.
 *
 * At the hip the weapon is also held as WWII holds it (cg_gunHip, 0 for the
 * viewmodel's own pose): a long gun nearer the eye and further right, along
 * the view, a scoped one further out and turned so the rifle crosses from the
 * lower right with its eyepiece at the eye's height, and a pistol in the
 * middle, pointed ahead. That pose leaves over the same part of the raise,
 * and its turn while sprinting. */

/* The weapon's projection, as r_scene.c hands it to the scene compositor:
 * its x and y scales, both 0 for the view's own. */
extern float g_webGunProjection[2];

void CG_GunFovInit(void);
/* The weapon's projection scales for a cg_fov-style gunFov, the view's
 * horizontal and vertical fields of view in degrees and how far the sights
 * are raised (0 to 1). Returns how much further the weapon's projection
 * spreads a point from the view axis than the view's does: 1, with both
 * scales 0, for the view's own. */
float CG_GunFovProjection(float gunFov, float viewFovX, float viewFovY, float raise, float *projection);
/* Each frame, before the view weapon is added. */
void CG_GunFovUpdate(const playerState_t *ps, qboolean drawGun);
/* Once a raised WWII scope has placed the rifle: the rifle's projection
 * scales, when it has its own. */
void CG_GunFovFollowScope(const float *projection);
/* A frame without a view weapon. */
void CG_GunFovClear(void);
/* The cg_fov-style angle a WWII scope's projection narrows from as the
 * rifle is raised: the weapon's, else viewHipFov. */
float CG_GunFovHipAngle(float viewHipFov);
/* Moves point, on the view weapon, to where the view's projection draws
 * what the weapon's projection draws at point. */
void CG_GunFovPoint(vec_t *point);
/* A hip pose: the offset in the weapon's forward, left and up, as cg_gun_x,
 * cg_gun_y and cg_gun_z move it, then the turn added to the weapon's pitch,
 * yaw and roll in degrees, about the eye. */
#define CG_GUNFOV_HIP_POSE 6
/* The hip pose for a weapon of weapClass, scoped when it has a scope's
 * overlay, at cg_gunHip scale: zeros for a class WWII holds as Call of
 * Duty 2 does. */
void CG_GunFovHipTarget(int weapClass, int scoped, float scale, float *pose);
/* How much of the hip pose stays as the sights are raised (0 to 1): all of
 * it at the hip, none once a sight's window opens. */
float CG_GunFovHipShare(float raise);
/* Each frame, the pose the view weapon of weapDef, with the player's
 * attachments (NULL for none), takes: eased from the last weapon's, times
 * the share the raise leaves. */
void CG_GunFovHipPose(const playerState_t *ps, const WeaponDef *weapDef, float *pose);

#endif
