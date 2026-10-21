#ifndef CG_SCOPEGLINT_H
#define CG_SCOPEGLINT_H

#include "common_types.h"

/* A rival's sniper rifle raised to his eye glints at the players it is aimed
 * at, as scopes do in Modern Warfare (2019), though more faintly: a small
 * white flare with a streak across it on the front of the scope, brightest
 * along his aim and gone CG_SCOPEGLINT_EDGE_DEG off it, smaller and fainter
 * the farther he is, coming in as the rifle is raised. A wall, another player
 * or smoke that hides the scope hides it. cg_scopeGlint 0 turns it off. */

/* The centre of the front of a sniper rifle's scope, in the space of its
 * world model's tag_flash, which the models leave unturned. */
typedef struct CgScopeGlintLens {
    const char *worldModel;
    float lens[3];
} CgScopeGlintLens;

#define CG_SCOPEGLINT_LENS_COUNT 4
extern const CgScopeGlintLens cg_scopeGlintLenses[CG_SCOPEGLINT_LENS_COUNT];

void CG_ScopeGlintInit(void);
const CgScopeGlintLens *CG_ScopeGlintFindLens(const char *worldModel);
/* After a player is added to the scene: notes where the scope of a rival's
 * sniper rifle raised to his eye is, and where he aims. */
void CG_ScopeGlintAddPlayer(const centity_t *cent);
/* Once the view is placed: glints each scope noted this frame that is aimed
 * near the eye and in its sight. */
void CG_DrawScopeGlints(void);

#endif
