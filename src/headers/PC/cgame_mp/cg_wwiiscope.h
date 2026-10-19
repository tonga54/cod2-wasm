#ifndef CG_WWIISCOPE_H
#define CG_WWIISCOPE_H

#include "common_types.h"

/* Scoped rifles as Call of Duty: WWII draws them in the browser: the rifle
 * stays on screen while it is raised to the eye, the eyepiece's glass becomes
 * a window onto the magnified world, the world around the eyepiece is blurred
 * and blacked out while the breath is held, and the reticle turns with the
 * scope as the bolt is worked. cg_wwiiScope 0, a rifle whose eyepiece is not
 * known or a browser without the scene compositor keep the stock overlay. */

/* A scope's glass, in the space of the gun's j_gun bone: the centre of its
 * rim, nearest the muzzle, and the rim's radius. The glass faces -x, towards
 * the eye. */
typedef struct CgWwiiScopeGlass {
    const char *gunModel;
    float rim[3];
    float radius;
    /* Where the stock ADS pose leaves the rim's centre in front of the rifle's
     * origin at full aim and at rest: units forward, left and up. Aiming
     * moves the rifle so it is centred, and narrows the rifle's projection
     * so the glass fills CG_WWIISCOPE_LENS_RADIUS of the screen's height. */
    float eye[3];
    /* Degrees the stock ADS pose cants the scope clockwise at full aim; aiming
     * turns the rifle back so the reticle is level. */
    float roll;
} CgWwiiScopeGlass;

/* The glass as seen this frame, in normalized device coordinates (x right,
 * y up): its centre and the half-axes of its outline. */
typedef struct CgWwiiScopeLens {
    float center[2];
    float axes[2][2];
    /* 0 while the stock glass is drawn, 1 once the glass is a window. */
    float presence;
    /* 0 while the window shows the glass's blurred reflection, 1 once it
     * shows the magnified world and the reticle. */
    float open;
    /* How much of the world around the eyepiece is blurred and blacked out. */
    float blur;
    float dark;
    /* The rifle's projection scales in x and y, or 0 for the world's. */
    float projection[2];
    /* The rifle's near clip distance, or 0 for the view's. */
    float nearClip;
} CgWwiiScopeLens;

#define CG_WWIISCOPE_GLASS_COUNT 4
extern const CgWwiiScopeGlass cg_wwiiScopeGlasses[CG_WWIISCOPE_GLASS_COUNT];

void CG_WwiiScopeInit(void);
/* Closes the window for a frame that draws no view weapon. */
void CG_WwiiScopeClear(void);
const CgWwiiScopeGlass *CG_WwiiScopeFindGlass(const char *gunModel);
/* The scoped rifle's glass when the WWII scope draws this weapon, else NULL. */
const CgWwiiScopeGlass *CG_WwiiScopeGlassForWeapon(int weapon);
/* The part of the raise over which a weapon zooms in: the stock fraction,
 * but a WWII scope zooms over at least the last CG_WWIISCOPE_ZOOM_IN_FRAC so
 * the window opens with the zoom. */
float CG_WwiiScopeZoomInFrac(int weapon);
/* The glass's rim on screen, from the j_gun bone's world axes and origin and
 * the rifle's projection scales: its centre and half-axes along the scope's
 * right and up. Returns 0 when the glass faces away from the eye, reaches
 * behind it or has no area. */
int CG_WwiiScopeProjectGlass(const CgWwiiScopeGlass *glass, const vec3_t *boneAxis, const vec_t *boneOrigin,
                             const vec_t *eye, const vec3_t *viewAxis, const float *projection, CgWwiiScopeLens *lens);
/* Turns the rifle as it is raised: levels its reticle and rolls it out while
 * the bolt is worked. */
void CG_WwiiScopeAdjustViewWeapon(const playerState_t *ps, int weapon, vec_t *angles);
/* Moves the rifle, once every other offset is applied to origin: centres
 * the glass of its resting pose on the view axis, whatever sway the stock
 * code left when aiming began, and keeps the glass at the eye while the
 * bolt is worked. */
void CG_WwiiScopePlaceViewWeapon(const playerState_t *ps, int weapon, vec_t *origin);
/* After the rifle is added: where its glass is, for the scene compositor and
 * the reticle. Returns whether the rifle stays drawn while scoped. */
qboolean CG_WwiiScopeUpdate(const playerState_t *ps, int weapon, qboolean drawGun);
/* Whether this frame's view weapon keeps only the light of its muzzle flash. */
qboolean CG_WwiiScopeMuzzleLightsOnly(void);
/* Draws the reticle over the window. Returns the remaining visibility of the
 * hip crosshair, as CG_DrawWeapReticle, or -1 when the stock overlay draws. */
float CG_DrawWwiiScopeReticle(void);
const CgWwiiScopeLens *CG_WwiiScopeLens(void);

#endif
