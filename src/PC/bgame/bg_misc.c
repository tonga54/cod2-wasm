#include "common_types.h"
#include "imports.h"

extern const dvar_t *player_view_pitch_up;
extern const dvar_t *player_view_pitch_down;
extern const dvar_t *bg_ladder_yawcap;
extern const dvar_t *bg_prone_yawcap;
extern const dvar_t *bg_foliagesnd_minspeed;
extern const dvar_t *bg_foliagesnd_maxspeed;
extern const dvar_t *bg_foliagesnd_slowinterval;
extern const dvar_t *bg_foliagesnd_fastinterval;
extern const dvar_t *bg_foliagesnd_resetinterval;
extern const dvar_t *bg_fallDamageMinHeight;
extern const dvar_t *bg_fallDamageMaxHeight;
extern const dvar_t *inertiaMax;
extern const dvar_t *inertiaDebug;
extern const dvar_t *inertiaAngle;
extern const dvar_t *friction;
extern const dvar_t *stopspeed;
extern const dvar_t *bg_swingSpeed;
extern const dvar_t *bg_bobAmplitudeStanding;
extern const dvar_t *bg_bobAmplitudeDucked;
extern const dvar_t *bg_bobAmplitudeProne;
extern const dvar_t *bg_bobMax;
extern const dvar_t *bg_aimSpreadMoveSpeedThreshold;
extern const dvar_t *player_breath_hold_time;
extern const dvar_t *player_breath_gasp_time;
extern const dvar_t *player_breath_fire_delay;
extern const dvar_t *player_breath_gasp_scale;
extern const dvar_t *player_breath_hold_lerp;
extern const dvar_t *player_breath_gasp_lerp;
extern const dvar_t *player_breath_snd_lerp;
extern const dvar_t *player_breath_snd_delay;
extern const dvar_t *player_toggleBinoculars;
extern const dvar_t *player_scopeExitOnDamage;
extern const dvar_t *player_adsExitDelay;
extern const dvar_t *player_dmgtimer_timePerPoint;
extern const dvar_t *player_dmgtimer_maxTime;
extern const dvar_t *player_dmgtimer_minScale;
extern const dvar_t *player_dmgtimer_stumbleTime;
extern const dvar_t *player_dmgtimer_flinchTime;
/* migrated from the ILP32 data blob to typed C (x64 port Stage 2). */
char *eventnames[199] = {
    "EV_NONE",
    "EV_FOOTSTEP_RUN_DEFAULT",
    "EV_FOOTSTEP_RUN_BARK",
    "EV_FOOTSTEP_RUN_BRICK",
    "EV_FOOTSTEP_RUN_CARPET",
    "EV_FOOTSTEP_RUN_CLOTH",
    "EV_FOOTSTEP_RUN_CONCRETE",
    "EV_FOOTSTEP_RUN_DIRT",
    "EV_FOOTSTEP_RUN_FLESH",
    "EV_FOOTSTEP_RUN_FOLIAGE",
    "EV_FOOTSTEP_RUN_GLASS",
    "EV_FOOTSTEP_RUN_GRASS",
    "EV_FOOTSTEP_RUN_GRAVEL",
    "EV_FOOTSTEP_RUN_ICE",
    "EV_FOOTSTEP_RUN_METAL",
    "EV_FOOTSTEP_RUN_MUD",
    "EV_FOOTSTEP_RUN_PAPER",
    "EV_FOOTSTEP_RUN_PLASTER",
    "EV_FOOTSTEP_RUN_ROCK",
    "EV_FOOTSTEP_RUN_SAND",
    "EV_FOOTSTEP_RUN_SNOW",
    "EV_FOOTSTEP_RUN_WATER",
    "EV_FOOTSTEP_RUN_WOOD",
    "EV_FOOTSTEP_RUN_ASPHALT",
    "EV_FOOTSTEP_WALK_DEFAULT",
    "EV_FOOTSTEP_WALK_BARK",
    "EV_FOOTSTEP_WALK_BRICK",
    "EV_FOOTSTEP_WALK_CARPET",
    "EV_FOOTSTEP_WALK_CLOTH",
    "EV_FOOTSTEP_WALK_CONCRETE",
    "EV_FOOTSTEP_WALK_DIRT",
    "EV_FOOTSTEP_WALK_FLESH",
    "EV_FOOTSTEP_WALK_FOLIAGE",
    "EV_FOOTSTEP_WALK_GLASS",
    "EV_FOOTSTEP_WALK_GRASS",
    "EV_FOOTSTEP_WALK_GRAVEL",
    "EV_FOOTSTEP_WALK_ICE",
    "EV_FOOTSTEP_WALK_METAL",
    "EV_FOOTSTEP_WALK_MUD",
    "EV_FOOTSTEP_WALK_PAPER",
    "EV_FOOTSTEP_WALK_PLASTER",
    "EV_FOOTSTEP_WALK_ROCK",
    "EV_FOOTSTEP_WALK_SAND",
    "EV_FOOTSTEP_WALK_SNOW",
    "EV_FOOTSTEP_WALK_WATER",
    "EV_FOOTSTEP_WALK_WOOD",
    "EV_FOOTSTEP_WALK_ASPHALT",
    "EV_FOOTSTEP_PRONE_DEFAULT",
    "EV_FOOTSTEP_PRONE_BARK",
    "EV_FOOTSTEP_PRONE_BRICK",
    "EV_FOOTSTEP_PRONE_CARPET",
    "EV_FOOTSTEP_PRONE_CLOTH",
    "EV_FOOTSTEP_PRONE_CONCRETE",
    "EV_FOOTSTEP_PRONE_DIRT",
    "EV_FOOTSTEP_PRONE_FLESH",
    "EV_FOOTSTEP_PRONE_FOLIAGE",
    "EV_FOOTSTEP_PRONE_GLASS",
    "EV_FOOTSTEP_PRONE_GRASS",
    "EV_FOOTSTEP_PRONE_GRAVEL",
    "EV_FOOTSTEP_PRONE_ICE",
    "EV_FOOTSTEP_PRONE_METAL",
    "EV_FOOTSTEP_PRONE_MUD",
    "EV_FOOTSTEP_PRONE_PAPER",
    "EV_FOOTSTEP_PRONE_PLASTER",
    "EV_FOOTSTEP_PRONE_ROCK",
    "EV_FOOTSTEP_PRONE_SAND",
    "EV_FOOTSTEP_PRONE_SNOW",
    "EV_FOOTSTEP_PRONE_WATER",
    "EV_FOOTSTEP_PRONE_WOOD",
    "EV_FOOTSTEP_PRONE_ASPHALT",
    "EV_JUMP_DEFAULT",
    "EV_JUMP_BARK",
    "EV_JUMP_BRICK",
    "EV_JUMP_CARPET",
    "EV_JUMP_CLOTH",
    "EV_JUMP_CONCRETE",
    "EV_JUMP_DIRT",
    "EV_JUMP_FLESH",
    "EV_JUMP_FOLIAGE",
    "EV_JUMP_GLASS",
    "EV_JUMP_GRASS",
    "EV_JUMP_GRAVEL",
    "EV_JUMP_ICE",
    "EV_JUMP_METAL",
    "EV_JUMP_MUD",
    "EV_JUMP_PAPER",
    "EV_JUMP_PLASTER",
    "EV_JUMP_ROCK",
    "EV_JUMP_SAND",
    "EV_JUMP_SNOW",
    "EV_JUMP_WATER",
    "EV_JUMP_WOOD",
    "EV_JUMP_ASPHALT",
    "EV_LANDING_DEFAULT",
    "EV_LANDING_BARK",
    "EV_LANDING_BRICK",
    "EV_LANDING_CARPET",
    "EV_LANDING_CLOTH",
    "EV_LANDING_CONCRETE",
    "EV_LANDING_DIRT",
    "EV_LANDING_FLESH",
    "EV_LANDING_FOLIAGE",
    "EV_LANDING_GLASS",
    "EV_LANDING_GRASS",
    "EV_LANDING_GRAVEL",
    "EV_LANDING_ICE",
    "EV_LANDING_METAL",
    "EV_LANDING_MUD",
    "EV_LANDING_PAPER",
    "EV_LANDING_PLASTER",
    "EV_LANDING_ROCK",
    "EV_LANDING_SAND",
    "EV_LANDING_SNOW",
    "EV_LANDING_WATER",
    "EV_LANDING_WOOD",
    "EV_LANDING_ASPHALT",
    "EV_LANDING_PAIN_DEFAULT",
    "EV_LANDING_PAIN_BARK",
    "EV_LANDING_PAIN_BRICK",
    "EV_LANDING_PAIN_CARPET",
    "EV_LANDING_PAIN_CLOTH",
    "EV_LANDING_PAIN_CONCRETE",
    "EV_LANDING_PAIN_DIRT",
    "EV_LANDING_PAIN_FLESH",
    "EV_LANDING_PAIN_FOLIAGE",
    "EV_LANDING_PAIN_GLASS",
    "EV_LANDING_PAIN_GRASS",
    "EV_LANDING_PAIN_GRAVEL",
    "EV_LANDING_PAIN_ICE",
    "EV_LANDING_PAIN_METAL",
    "EV_LANDING_PAIN_MUD",
    "EV_LANDING_PAIN_PAPER",
    "EV_LANDING_PAIN_PLASTER",
    "EV_LANDING_PAIN_ROCK",
    "EV_LANDING_PAIN_SAND",
    "EV_LANDING_PAIN_SNOW",
    "EV_LANDING_PAIN_WATER",
    "EV_LANDING_PAIN_WOOD",
    "EV_LANDING_PAIN_ASPHALT",
    "EV_FOLIAGE_SOUND",
    "EV_STANCE_FORCE_STAND",
    "EV_STANCE_FORCE_CROUCH",
    "EV_STANCE_FORCE_PRONE",
    "EV_STEP_VIEW",
    "EV_ITEM_PICKUP",
    "EV_AMMO_PICKUP",
    "EV_NOAMMO",
    "EV_EMPTYCLIP",
    "EV_EMPTY_OFFHAND",
    "EV_RESET_ADS",
    "EV_RELOAD",
    "EV_RELOAD_FROM_EMPTY",
    "EV_RELOAD_START",
    "EV_RELOAD_END",
    "EV_RAISE_WEAPON",
    "EV_PUTAWAY_WEAPON",
    "EV_WEAPON_ALT",
    "EV_PULLBACK_WEAPON",
    "EV_FIRE_WEAPON",
    "EV_FIRE_WEAPONB",
    "EV_FIRE_WEAPON_LASTSHOT",
    "EV_RECHAMBER_WEAPON",
    "EV_EJECT_BRASS",
    "EV_MELEE_SWIPE",
    "EV_FIRE_MELEE",
    "EV_PREP_OFFHAND",
    "EV_USE_OFFHAND",
    "EV_SWITCH_OFFHAND",
    "EV_BINOCULAR_ENTER",
    "EV_BINOCULAR_EXIT",
    "EV_BINOCULAR_FIRE",
    "EV_BINOCULAR_RELEASE",
    "EV_BINOCULAR_DROP",
    "EV_MELEE_HIT",
    "EV_MELEE_MISS",
    "EV_FIRE_WEAPON_MG42",
    "EV_FIRE_QUADBARREL_1",
    "EV_FIRE_QUADBARREL_2",
    "EV_BULLET_TRACER",
    "EV_SOUND_ALIAS",
    "EV_SOUND_ALIAS_AS_MASTER",
    "EV_BULLET_HIT_SMALL",
    "EV_BULLET_HIT_LARGE",
    "EV_SHOTGUN_HIT",
    "EV_BULLET_HIT_AP",
    "EV_BULLET_HIT_CLIENT_SMALL",
    "EV_BULLET_HIT_CLIENT_LARGE",
    "EV_GRENADE_BOUNCE",
    "EV_GRENADE_EXPLODE",
    "EV_ROCKET_EXPLODE",
    "EV_ROCKET_EXPLODE_NOMARKS",
    "EV_CUSTOM_EXPLODE",
    "EV_CUSTOM_EXPLODE_NOMARKS",
    "EV_BULLET",
    "EV_PLAY_FX",
    "EV_PLAY_FX_ON_TAG",
    "EV_EARTHQUAKE",
    "EV_GRENADE_SUICIDE",
    "EV_OBITUARY",
};

extern const int singleClientEvents[7];
extern const dvar_t *player_moveThreshhold;
extern const dvar_t *player_footstepsThreshhold;
extern const dvar_t *player_strafeSpeedScale;
extern const dvar_t *player_backSpeedScale;
extern const dvar_t *player_spectateSpeedScale;
extern const dvar_t *player_turnAnims;

extern int I_stricmp(const char *s1, const char *s2);
extern int G_GetWeaponIndexForName(const char *name);
extern qboolean BG_DoesWeaponNeedSlot(int weapon);
extern int BG_GetMaxPickupableAmmo(const playerState_t *ps, int weapon);
extern qboolean BG_WeaponIsClipOnly(int weapon);
extern void Com_Error(errorParm_t code, const char *fmt, ...);
extern const char *va(const char *format, ...);
extern const dvar_t *Dvar_RegisterFloat(const char *dvarName, float value, float min, float max, unsigned int flags);
extern const dvar_t *Dvar_RegisterInt(const char *dvarName, int value, int min, int max, unsigned int flags);
extern const dvar_t *Dvar_RegisterBool(const char *dvarName, qboolean value, unsigned int flags);
extern void Jump_RegisterDvars(void);
extern void Mantle_RegisterDvars(void);
extern double sin(double);
extern double cos(double);
extern const vec_t Vec3NormalizeTo(const vec_t *v, vec_t *out);

void BG_RegisterDvars(void);
const gitem_t *BG_FindItemForWeapon(int weapon);
const gitem_t *G_FindItem(const char *pickupName);
void BG_AddPredictableEventToPlayerstate(int newEvent, int eventParm, playerState_t *ps);
extern int PM_GetEffectiveStance(void *ps);
extern int PM_GetViewHeightLerpTime(void *ps, int current, int target);
extern float AngleNormalize180(float angle);
extern void AngleVectors(const vec_t *angles, vec_t *forward, vec_t *right, vec_t *up);
extern const vec_t Vec3Normalize(vec_t *v);
extern const float vectopitch(const vec_t *vec);
extern const float AngleSubtract(const float a1, const float a2);
void BG_PlayerStateToEntityState(playerState_t *ps, entityState_t *s, qboolean snap, int handler);
void BG_EvaluateTrajectoryDelta(const trajectory_t *tr, int atTime, vec_t *result);
qboolean BG_CanItemBeGrabbed(const entityState_t *ent, const playerState_t *ps, qboolean bTouched);
qboolean BG_CheckProneValid(int passEntityNum, const vec_t *vPos, const float fSize, const float fHeight, const float fYaw, float *pfTorsoHeight, float *pfTorsoPitch, float *pfWaistPitch, const qboolean bAlreadyProne, const qboolean bOnGround, vec_t *vGroundNormal, int handler, proneCheckType_t proneCheckType, float prone_feet_dist);
qboolean BG_CheckProne(int passEntityNum, const vec_t *vPos, const float fSize, const float fHeight, const float fYaw, float *pfTorsoHeight, float *pfTorsoPitch, float *pfWaistPitch, const qboolean bAlreadyProne, const qboolean bOnGround, vec_t *vGroundNormal, int handler, proneCheckType_t proneCheckType, float prone_feet_dist);
void BG_EvaluateTrajectory(const trajectory_t *tr, int atTime, vec_t *result);
qboolean BG_PlayerTouchesItem(playerState_t *ps, entityState_t *item, int atTime);

static void BG_Vec3Clear(vec_t *out)
{
    out[0] = 0.0f;
    out[1] = 0.0f;
    out[2] = 0.0f;
}

static void BG_Vec3Copy(const vec_t *in, vec_t *out)
{
    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
}

static void BG_Vec3Scale(const vec_t *in, float scale, vec_t *out)
{
    out[0] = in[0] * scale;
    out[1] = in[1] * scale;
    out[2] = in[2] * scale;
}

static void BG_Vec3Mad(const vec_t *start, float scale, const vec_t *dir, vec_t *out)
{
    out[0] = start[0] + dir[0] * scale;
    out[1] = start[1] + dir[1] * scale;
    out[2] = start[2] + dir[2] * scale;
}

void BG_RegisterDvars(void)
{
    player_view_pitch_up = Dvar_RegisterFloat("player_view_pitch_up", 85.0f, 0.0f, 90.0f, 0x1180);
    player_view_pitch_down = Dvar_RegisterFloat("player_view_pitch_down", 85.0f, 0.0f, 90.0f, 0x1180);
    bg_ladder_yawcap = Dvar_RegisterFloat("bg_ladder_yawcap", 100.0f, 0.0f, 360.0f, 0x1180);
    bg_prone_yawcap = Dvar_RegisterFloat("bg_prone_yawcap", 85.0f, 0.0f, 360.0f, 0x1180);
    bg_foliagesnd_minspeed = Dvar_RegisterFloat("bg_foliagesnd_minspeed", 40.0f, 0.0f, 3.4028235e+38f, 0x1180);
    bg_foliagesnd_maxspeed = Dvar_RegisterFloat("bg_foliagesnd_maxspeed", 180.0f, 0.0f, 3.4028235e+38f, 0x1180);
    bg_foliagesnd_slowinterval = Dvar_RegisterInt("bg_foliagesnd_slowinterval", 1500, 0, 0x7fffffff, 0x1180);
    bg_foliagesnd_fastinterval = Dvar_RegisterInt("bg_foliagesnd_fastinterval", 500, 0, 0x7fffffff, 0x1180);
    bg_foliagesnd_resetinterval = Dvar_RegisterInt("bg_foliagesnd_resetinterval", 500, 0, 0x7fffffff, 0x1180);
    bg_fallDamageMinHeight = Dvar_RegisterFloat("bg_fallDamageMinHeight", 256.0f, 1.0f, 3.4028235e+38f, 0x1188);
    bg_fallDamageMaxHeight = Dvar_RegisterFloat("bg_fallDamageMaxHeight", 480.0f, 1.0f, 3.4028235e+38f, 0x1188);
    inertiaMax = Dvar_RegisterFloat("inertiaMax", 50.0f, 0.0f, 1000.0f, 0x1180);
    inertiaDebug = Dvar_RegisterBool("inertiaDebug", 0, 0x1180);
    inertiaAngle = Dvar_RegisterFloat("inertiaAngle", 0.0f, -1.0f, 1.0f, 0x1180);
    friction = Dvar_RegisterFloat("friction", 5.5f, 0.0f, 100.0f, 0x1180);
    stopspeed = Dvar_RegisterFloat("stopspeed", 100.0f, 0.0f, 1000.0f, 0x1180);
    bg_swingSpeed = Dvar_RegisterFloat("bg_swingSpeed", 0.2f, 0.0f, 1.0f, 0x1080);
    bg_bobAmplitudeStanding = Dvar_RegisterFloat("bg_bobAmplitudeStanding", 0.007f, 0.0f, 1.0f, 0x1180);
    bg_bobAmplitudeDucked = Dvar_RegisterFloat("bg_bobAmplitudeDucked", 0.0075f, 0.0f, 1.0f, 0x1180);
    bg_bobAmplitudeProne = Dvar_RegisterFloat("bg_bobAmplitudeProne", 0.03f, 0.0f, 1.0f, 0x1180);
    bg_bobMax = Dvar_RegisterFloat("bg_bobMax", 8.0f, 0.0f, 36.0f, 0x1180);
    bg_aimSpreadMoveSpeedThreshold = Dvar_RegisterFloat("bg_aimSpreadMoveSpeedThreshold", 11.0f, 0.0f, 300.0f, 0x1180);
    player_breath_hold_time = Dvar_RegisterFloat("player_breath_hold_time", 4.5f, 0.0f, 30.0f, 0x1180);
    player_breath_gasp_time = Dvar_RegisterFloat("player_breath_gasp_time", 1.0f, 0.0f, 30.0f, 0x1180);
    player_breath_fire_delay = Dvar_RegisterFloat("player_breath_fire_delay", 0.0f, 0.0f, 30.0f, 0x1180);
    player_breath_gasp_scale = Dvar_RegisterFloat("player_breath_gasp_scale", 4.5f, 0.0f, 50.0f, 0x1180);
    player_breath_hold_lerp = Dvar_RegisterFloat("player_breath_hold_lerp", 4.0f, 0.0f, 50.0f, 0x1180);
    player_breath_gasp_lerp = Dvar_RegisterFloat("player_breath_gasp_lerp", 6.0f, 0.0f, 50.0f, 0x1180);
    player_breath_snd_lerp = Dvar_RegisterFloat("player_breath_snd_lerp", 2.0f, 0.0f, 100.0f, 0x1180);
    player_breath_snd_delay = Dvar_RegisterFloat("player_breath_snd_delay", 1.0f, 0.0f, 2.0f, 0x1180);
    player_toggleBinoculars = Dvar_RegisterBool("player_toggleBinoculars", 1, 0x1100);
    player_scopeExitOnDamage = Dvar_RegisterBool("player_scopeExitOnDamage", 0, 0x1180);
    player_adsExitDelay = Dvar_RegisterInt("player_adsExitDelay", 0, 0, 1000, 0x1180);
    player_moveThreshhold = Dvar_RegisterFloat("player_moveThreshhold", 10.0f, 1.0e-8f, 20.0f, 0x11c0);
    player_footstepsThreshhold = Dvar_RegisterFloat("player_footstepsThreshhold", 0.0f, 0.0f, 50000.0f, 0x11c0);
    player_strafeSpeedScale = Dvar_RegisterFloat("player_strafeSpeedScale", 0.8f, 0.0f, 20.0f, 0x1180);
    player_backSpeedScale = Dvar_RegisterFloat("player_backSpeedScale", 0.7f, 0.0f, 20.0f, 0x1180);
    player_spectateSpeedScale = Dvar_RegisterFloat("player_spectateSpeedScale", 2.0f, 0.0f, 20.0f, 0x1180);
    player_turnAnims = Dvar_RegisterBool("player_turnAnims", 0, 0x1180);
    player_dmgtimer_timePerPoint = Dvar_RegisterFloat("player_dmgtimer_timePerPoint", 100.0f, 0.0f, 3.4028235e+38f, 0x1180);
    player_dmgtimer_maxTime = Dvar_RegisterFloat("player_dmgtimer_maxTime", 750.0f, 0.0f, 3.4028235e+38f, 0x1180);
    player_dmgtimer_minScale = Dvar_RegisterFloat("player_dmgtimer_minScale", 0.0f, 0.0f, 1.0f, 0x1180);
    player_dmgtimer_stumbleTime = Dvar_RegisterInt("player_dmgtimer_stumbleTime", 500, 0, 2000, 0x1180);
    player_dmgtimer_flinchTime = Dvar_RegisterInt("player_dmgtimer_flinchTime", 500, 0, 2000, 0x1180);
    Jump_RegisterDvars();
    Mantle_RegisterDvars();
}

const gitem_t *BG_FindItemForWeapon(int weapon)
{
    return &((const gitem_t *)imp_bg_itemlist)[weapon];
}

const gitem_t *G_FindItem(const char *pickupName)
{
    int iIndex;
    int count;
    char *base;
    int weaponIndex;

    count = *(int *)imp_bg_numItems;
    base = (char *)imp_bg_itemlist;

    for (iIndex = 0x81; iIndex < count; iIndex++) {
        char *it = base + iIndex * 44;
        if (I_stricmp(((gitem_t *)it)->pickup_name, pickupName) == 0) {
            return (const gitem_t *)it;
        }
        if (I_stricmp(*(const char **)it, pickupName) == 0) {
            return (const gitem_t *)it;
        }
    }

    weaponIndex = G_GetWeaponIndexForName(pickupName);
    if (weaponIndex == 0) {
        return NULL;
    }
    return (const gitem_t *)(base + weaponIndex * 44);
}

void BG_AddPredictableEventToPlayerstate(int newEvent, int eventParm, playerState_t *ps)
{
    int seq;
    int index;

    if (newEvent == 0)
        return;

    seq = ps->eventSequence;
    index = seq & 3;
    ps->events[index] = (unsigned char)newEvent;
    ps->eventParms[index] = (unsigned char)eventParm;
    ps->eventSequence = seq + 1;
}

void BG_PlayerStateToEntityState(playerState_t *ps, entityState_t *s, qboolean snap, int handler)
{
    int eFlags;
    int i;
    int stance;
    int eventSequence;
    int eventOld;
    int event;
    float lerpFrac;
    int lerpTime;
    int elapsed;

    {
        int val = ps->pm_flags & 0xc00000;
        int eType;
        if (val != 0) {
            eType = 1;
        } else {
            eType = 5;
        }
        s->eType = eType;
    }

    s->pos.trType = 1;

    s->pos.trBase[0] = ps->origin[0];
    s->pos.trBase[1] = ps->origin[1];
    s->pos.trBase[2] = ps->origin[2];

    if (snap) {

        s->pos.trBase[0] = (float)(int)s->pos.trBase[0];
        s->pos.trBase[1] = (float)(int)s->pos.trBase[1];
        s->pos.trBase[2] = (float)(int)s->pos.trBase[2];
    }

    s->apos.trType = 1;

    s->apos.trBase[0] = ps->viewangles[0];
    s->apos.trBase[1] = ps->viewangles[1];
    s->apos.trBase[2] = ps->viewangles[2];

    if (snap) {
        s->apos.trBase[0] = (float)(int)s->apos.trBase[0];
        s->apos.trBase[1] = (float)(int)s->apos.trBase[1];
        s->apos.trBase[2] = (float)(int)s->apos.trBase[2];
    }

    s->angles2[1] = (float)ps->movementDir;

    s->legsAnim = ps->legsAnim;

    s->torsoAnim = ps->torsoAnim;

    s->clientNum = ps->clientNum;

    eFlags = ps->eFlags;
    s->eFlags = eFlags;

    if (eFlags & 0x300) {

        s->otherEntityNum = ps->viewlocked_entNum;
    }

    if (ps->pm_type > 5) {

        eFlags = s->eFlags;
        eFlags |= 0x20000;
        s->eFlags = eFlags;
    } else {

        eFlags = s->eFlags;
        eFlags &= ~0x20000;
        s->eFlags = eFlags;
    }

    if (ps->pm_flags & 0x40) {

        eFlags |= 0x40000;
        s->eFlags = eFlags;
    } else {

        eFlags &= ~0x40000;
        s->eFlags = eFlags;
    }

    s->leanf = ps->leanf;

    stance = PM_GetEffectiveStance(ps);

    if (stance == 1) {

        int viewHeightLerpTarget = ps->viewHeightLerpTime;
        if (viewHeightLerpTarget != 0) {

            lerpTime = PM_GetViewHeightLerpTime(ps, ps->viewHeightLerpTarget, ps->viewHeightLerpDown);

            elapsed = ps->commandTime - ps->viewHeightLerpTime;
            lerpFrac = (float)elapsed / (float)lerpTime;

            if (lerpFrac < 0.0f) {
                lerpFrac = 0.0f;
            } else {

                if (1.0f < lerpFrac) {
                    lerpFrac = 1.0f;
                }
            }

            if (ps->viewHeightLerpDown != 0) {

            } else {

                lerpFrac = 1.0f - lerpFrac;
            }
        } else {
            lerpFrac = 1.0f;
        }

        s->fTorsoHeight = lerpFrac * ps->fTorsoHeight;

        s->fTorsoPitch = AngleNormalize180(ps->fTorsoPitch) * lerpFrac;

        s->fWaistPitch = AngleNormalize180(ps->fWaistPitch) * lerpFrac;
    } else {

        s->fTorsoHeight = 0;
        s->fTorsoPitch = 0;
        s->fWaistPitch = 0;
    }

    eventOld = ps->entityEventSequence;
    eventSequence = ps->eventSequence;

    if (eventOld < eventSequence) {

        if (eventSequence - eventOld > 4) {

            eventOld = eventSequence - 4;
            ps->entityEventSequence = eventOld;
        }

        {
            int idx = eventOld & 3;
            s->eventParm = (unsigned char)ps->eventParms[idx];
            ps->entityEventSequence = eventOld + 1;
        }
    } else {

        s->eventParm = 0;
    }

    i = ps->oldEventSequence;
    if (i != ps->eventSequence) {
        byte handlerIdx = (unsigned char)handler;

        byte *handlerTable = (byte *)imp_pmoveHandlers + handlerIdx * 12;

        while (i != ps->eventSequence) {
            int slot = i & 3;
            event = (unsigned char)ps->events[slot];

            {
                void (*eventCallback)(int, int) = *(void (**)(int, int))(handlerTable + 8);
                if (eventCallback) {
                    eventCallback(s->number, event & 0xff);
                }
            }

            if ((event & 0xff) == 0x8c) {

            } else {

                int j = 0;
                int found = 0;
                while (singleClientEvents[j + 1] > 0) {
                    j++;
                    if (singleClientEvents[j] == (event & 0xff)) {
                        found = 1;
                        break;
                    }
                }

                if (!found) {

                    if (singleClientEvents[j + 1] < 0) {

                        int seqOut = s->eventSequence;
                        int outSlot = seqOut & 3;
                        s->events[outSlot] = event & 0xff;

                        s->eventParms[outSlot] = (unsigned char)ps->eventParms[slot];

                        s->eventSequence = seqOut + 1;
                    }
                }
            }

            i++;
        }
    }

    ps->oldEventSequence = i;

    s->weapon = (unsigned char)ps->weapon;

    s->groundEntityNum = (unsigned short)ps->groundEntityNum;
}

void BG_EvaluateTrajectoryDelta(const trajectory_t *tr, int atTime, vec_t *result)
{
    float elapsed;
    float scale;

    switch (tr->trType) {
    case TR_STATIONARY:
    case TR_INTERPOLATE:
        BG_Vec3Clear(result);
        return;

    case TR_LINEAR:
        BG_Vec3Copy(tr->trDelta, result);
        return;

    case TR_LINEAR_STOP:
        if (atTime > tr->trTime + tr->trDuration) {
            BG_Vec3Clear(result);
        } else {
            BG_Vec3Copy(tr->trDelta, result);
        }
        return;

    case TR_SINE:
        elapsed = (float)(atTime - tr->trTime) / (float)tr->trDuration;
        scale = (float)cos((double)elapsed * 3.141592653589793 * 2.0) * 0.5f;
        BG_Vec3Scale(tr->trDelta, scale, result);
        return;

    case TR_GRAVITY:
        elapsed = (float)(atTime - tr->trTime) * 0.001f;
        result[0] = tr->trDelta[0];
        result[1] = tr->trDelta[1];
        result[2] = tr->trDelta[2] + elapsed * -800.0f;
        return;

    case TR_ACCELERATE:
        if (atTime > tr->trTime + tr->trDuration) {
            BG_Vec3Clear(result);
        } else {
            elapsed = (float)(atTime - tr->trTime) * 0.001f;
            BG_Vec3Scale(tr->trDelta, elapsed * elapsed, result);
        }
        return;

    case TR_DECELERATE:
        if (tr->trTime + tr->trDuration >= atTime) {
            elapsed = (float)(atTime - tr->trTime) * 0.001f;
            BG_Vec3Scale(tr->trDelta, elapsed, result);
        } else {
            BG_Vec3Clear(result);
        }
        return;

    default:
        Com_Error(ERR_DROP, (const char *)"\x15"
                                          "BG_EvaluateTrajectoryDelta: unknown trType: %i",
                  tr->trType);
        return;
    }
}

qboolean BG_CanItemBeGrabbed(const entityState_t *ent, const playerState_t *ps, qboolean bTouched)
{
    int index;
    int giType;
    int weapon;
    char *base;

    index = ent->index.item;
    if (index <= 0 || index >= *(int *)imp_bg_numItems) {
        Com_Error(ERR_DROP, va((const char *)"\x15"
                                             "BG_CanItemBeGrabbed: index out of range (index is %i, eType is %i)",
                               index, ent->eType));
        index = ent->index.item;
    }

    base = (char *)imp_bg_itemlist;

    if (ent->clientNum == ps->clientNum) {
        return 0;
    }

    giType = ((gitem_t *)base)[index].giType;

    switch (giType) {
    case 0:
        Com_Error(ERR_DROP, (const char *)"\x15"
                                          "BG_CanItemBeGrabbed: IT_BAD");
        return 0;

    case 1:
        weapon = ((gitem_t *)base)[index].giTag;
        if (BG_DoesWeaponNeedSlot(weapon)) {
            if (!(ps->weapons[weapon >> 5] & (1 << (weapon & 0x1f)))) {
                if (bTouched) {
                    return 0;
                }
            }
        }
        if (BG_GetMaxPickupableAmmo(ps, weapon) <= 0) {
            return 0;
        }
        return 1;

    case 2:
        weapon = ((gitem_t *)base)[index].giTag;
        if (!(ps->weapons[weapon >> 5] & (1 << (weapon & 0x1f)))) {
            if (!BG_WeaponIsClipOnly(weapon)) {
                return 0;
            }
        }
        if (BG_GetMaxPickupableAmmo(ps, weapon) <= 0) {
            return 0;
        }
        return 1;

    case 3:
        if (ps->stats[0] >= ps->stats[2]) {
            return 0;
        }
        return 1;

    default:
        return 0;
    }
}

qboolean BG_CheckProneValid(int passEntityNum, const vec_t *vPos, const float fSize, const float fHeight, const float fYaw, float *pfTorsoHeight, float *pfTorsoPitch, float *pfWaistPitch, const qboolean bAlreadyProne, const qboolean bOnGround, vec_t *vGroundNormal, int handler, proneCheckType_t proneCheckType, float prone_feet_dist)
{
    typedef void (*BG_ProneTraceFn)(trace_t *trace, const vec_t *start, const vec_t *mins, const vec_t *maxs, const vec_t *end, int passEntityNum, int contentMask);

    pmoveHandler_t *handlers = (pmoveHandler_t *)imp_pmoveHandlers;
    BG_ProneTraceFn traceFunc = (BG_ProneTraceFn)handlers[(unsigned char)handler].trace;
    int iTraceMask = (proneCheckType < PCT_ACTOR) ? 0x00810011 : 0x00820011;
    trace_t trace;
    vec3_t vMins;
    vec3_t vMaxs;
    vec3_t vStart;
    vec3_t vEnd;
    vec3_t vForward;
    vec3_t vRight;
    vec3_t vUp;
    vec3_t vDelta;
    vec3_t origin;
    vec3_t waistPoint;
    vec3_t lowerPoint;
    vec3_t firstHit;
    vec3_t secondHit;
    float fTraceHeight;
    float scale;
    float frac;
    float firstScale = 0.0f;
    int bFirstTraceHit;
    qboolean success;

    vMins[0] = vPos[0] - fSize;
    vMins[1] = vPos[1] - fSize;
    vMins[2] = vPos[2];
    vMaxs[0] = vPos[0] + fSize;
    vMaxs[1] = vPos[1] + fSize;
    vMaxs[2] = vPos[2] + fHeight;

    if (!bAlreadyProne) {
        vMins[0] = -fSize;
        vMins[1] = -fSize;
        vMins[2] = 0.0f;
        vMaxs[0] = fSize;
        vMaxs[1] = fSize;
        vMaxs[2] = fHeight;

        vStart[0] = vPos[0];
        vStart[1] = vPos[1];
        vStart[2] = vPos[2];
        vEnd[0] = vPos[0];
        vEnd[1] = vPos[1];
        vEnd[2] = vPos[2] + 10.0f;

        traceFunc(&trace, vStart, vMins, vMaxs, vEnd, passEntityNum, iTraceMask);
        if (trace.startsolid) {
            return 0;
        }
    }

    if (bOnGround && vGroundNormal && vGroundNormal[2] < 0.699999988079071f) {
        return 0;
    }

    vMins[0] = -6.0f;
    vMins[1] = -6.0f;
    vMins[2] = -6.0f;
    vMaxs[0] = 6.0f;
    vMaxs[1] = 6.0f;
    vMaxs[2] = 6.0f;

    vEnd[0] = 0.0f;
    vEnd[1] = fYaw - 180.0f;
    vEnd[2] = 0.0f;
    AngleVectors(vEnd, vForward, vRight, vUp);

    fTraceHeight = fHeight - 6.0f;
    vStart[0] = vPos[0];
    vStart[1] = vPos[1];
    vStart[2] = vPos[2] + fTraceHeight;

    scale = prone_feet_dist - 6.0f;
    vEnd[0] = vStart[0] + scale * vForward[0];
    vEnd[1] = vStart[1] + scale * vForward[1];
    vEnd[2] = vStart[2] + scale * vForward[2];

    traceFunc(&trace, vStart, vMins, vMaxs, vEnd, passEntityNum, iTraceMask);
    frac = trace.fraction;

    if (frac < 1.0f) {
        if (!bOnGround) {
            return 0;
        }

        firstScale = scale * frac + 6.0f;
        if (fSize + 2.0f > firstScale) {
            return 0;
        }

        if (48.0f + fTraceHeight * 0.699999988079071f > firstScale) {
            float len;

            vEnd[2] += 22.0f;
            vDelta[0] = vEnd[0] - vStart[0];
            vDelta[1] = vEnd[1] - vStart[1];
            vDelta[2] = vEnd[2] - vStart[2];
            len = Vec3NormalizeTo(vDelta, vForward);

            traceFunc(&trace, vStart, vMins, vMaxs, vEnd, passEntityNum, iTraceMask);
            frac = trace.fraction;
            if (frac >= 1.0f) {
                goto no_first_trace_hit;
            }

            firstScale = len * frac + 6.0f;
            if (48.0f + fTraceHeight * 0.699999988079071f > firstScale) {
                return 0;
            }
        }

        bFirstTraceHit = 1;
    } else {
    no_first_trace_hit:
        bFirstTraceHit = 0;
        frac = trace.fraction;
    }

    firstHit[0] = vStart[0] + (vEnd[0] - vStart[0]) * frac;
    firstHit[1] = vStart[1] + (vEnd[1] - vStart[1]) * frac;
    firstHit[2] = vStart[2] + (vEnd[2] - vStart[2]) * frac;

    vStart[0] = vPos[0] + 48.0f * vForward[0];
    vStart[1] = vPos[1] + 48.0f * vForward[1];
    vStart[2] = vPos[2] + 48.0f * vForward[2] + fTraceHeight;

    vEnd[0] = vStart[0];
    vEnd[1] = vStart[1];
    vEnd[2] = vStart[2] - (fTraceHeight + 2.5f * fSize - 6.0f);

    traceFunc(&trace, vStart, vMins, vMaxs, vEnd, passEntityNum, iTraceMask);

    if (trace.fraction >= 1.0f) {
        goto prone_clear_or_air_success;
    }
    if (trace.normal[2] < 0.699999988079071f) {
        return 0;
    }

    waistPoint[0] = vStart[0] + (vEnd[0] - vStart[0]) * trace.fraction;
    waistPoint[1] = vStart[1] + (vEnd[1] - vStart[1]) * trace.fraction;
    waistPoint[2] = vStart[2] + (vEnd[2] - vStart[2]) * trace.fraction - 6.0f;

    if (bFirstTraceHit) {
        float downDist = fTraceHeight + 2.5f * fSize - 6.0f;
        float hitDist = downDist * trace.fraction + 6.0f;
        if (-0.75f * hitDist > firstScale - hitDist) {
            goto prone_clear_or_air_success;
        }
    }

    vDelta[0] = firstHit[0] - waistPoint[0] + 6.0f * vForward[0];
    vDelta[1] = firstHit[1] - waistPoint[1] + 6.0f * vForward[1];
    vDelta[2] = firstHit[2] - waistPoint[2] + 6.0f * vForward[2] + 6.0f;
    Vec3Normalize(vDelta);

    {
        float backScale = scale - 48.0f;
        vEnd[0] = vStart[0] + backScale * vDelta[0];
        vEnd[1] = vStart[1] + backScale * vDelta[1];
        vEnd[2] = vStart[2] + backScale * vDelta[2];
    }

    vEnd[0] = 0.5f * (vPos[0] + scale * vForward[0] + vEnd[0]);
    vEnd[1] = 0.5f * (vPos[1] + scale * vForward[1] + vEnd[1]);

    traceFunc(&trace, vStart, vMins, vMaxs, vEnd, passEntityNum, iTraceMask);
    frac = trace.fraction;
    if (frac < 1.0f) {
        vStart[0] = vStart[0] + (vEnd[0] - vStart[0]) * frac;
        vStart[1] = vStart[1] + (vEnd[1] - vStart[1]) * frac;
        vStart[2] = vStart[2] + (vEnd[2] - vStart[2]) * frac + 18.0f;
        vEnd[2] += 18.0f;

        traceFunc(&trace, vStart, vMins, vMaxs, vEnd, passEntityNum, iTraceMask);
        if (trace.fraction < 1.0f) {
            goto prone_clear_or_air_success;
        }
        frac = trace.fraction;
    }

    secondHit[0] = vStart[0] + (vEnd[0] - vStart[0]) * frac;
    secondHit[1] = vStart[1] + (vEnd[1] - vStart[1]) * frac;
    secondHit[2] = vStart[2] + (vEnd[2] - vStart[2]) * frac;

    vStart[0] = secondHit[0];
    vStart[1] = secondHit[1];
    vStart[2] = secondHit[2];
    vEnd[0] = secondHit[0];
    vEnd[1] = secondHit[1];
    vEnd[2] = secondHit[2] - ((secondHit[2] - waistPoint[2]) * 2.0f + fSize);

    traceFunc(&trace, vStart, vMins, vMaxs, vEnd, passEntityNum, iTraceMask);
    if (trace.fraction >= 1.0f) {
        goto prone_clear_or_air_success;
    }
    if (trace.normal[2] < 0.699999988079071f) {
        return 0;
    }

    lowerPoint[0] = vStart[0] + (vEnd[0] - vStart[0]) * trace.fraction;
    lowerPoint[1] = vStart[1] + (vEnd[1] - vStart[1]) * trace.fraction;
    lowerPoint[2] = vStart[2] + (vEnd[2] - vStart[2]) * trace.fraction - 6.0f;

    origin[0] = vPos[0];
    origin[1] = vPos[1];
    origin[2] = vPos[2];

    vDelta[0] = waistPoint[0] - origin[0];
    vDelta[1] = waistPoint[1] - origin[1];
    vDelta[2] = waistPoint[2] - origin[2];
    {
        float fTorsoPitch = vectopitch(vDelta);
        float fPitchDiff;
        vDelta[0] = lowerPoint[0] - waistPoint[0];
        vDelta[1] = lowerPoint[1] - waistPoint[1];
        vDelta[2] = lowerPoint[2] - waistPoint[2];
        fPitchDiff = AngleSubtract(vectopitch(vDelta), fTorsoPitch);
        success = (fPitchDiff >= -50.0f && fPitchDiff <= 70.0f);
    }

    vMins[0] = -0.0f;
    vMins[1] = -0.0f;
    vMins[2] = -0.0f;
    vMaxs[0] = 0.0f;
    vMaxs[1] = 0.0f;
    vMaxs[2] = 0.0f;

    vStart[0] = origin[0];
    vStart[1] = origin[1];
    vStart[2] = origin[2] + 5.0f;
    vEnd[0] = waistPoint[0];
    vEnd[1] = waistPoint[1];
    vEnd[2] = waistPoint[2] + 5.0f;
    traceFunc(&trace, vStart, vMins, vMaxs, vEnd, passEntityNum, iTraceMask);
    if (trace.fraction < 1.0f) {
        success = 0;
    }

    vStart[0] = vEnd[0];
    vStart[1] = vEnd[1];
    vStart[2] = vEnd[2];
    vEnd[0] = lowerPoint[0];
    vEnd[1] = lowerPoint[1];
    vEnd[2] = lowerPoint[2] + 5.0f;
    traceFunc(&trace, vStart, vMins, vMaxs, vEnd, passEntityNum, iTraceMask);
    if (trace.fraction < 1.0f) {
        success = 0;
    }

    if (pfTorsoHeight) {
        *pfTorsoHeight = 0.0f;
    }
    if (pfTorsoPitch) {
        vDelta[0] = origin[0] - waistPoint[0];
        vDelta[1] = origin[1] - waistPoint[1];
        vDelta[2] = origin[2] - waistPoint[2];
        *pfTorsoPitch = AngleNormalize180(vectopitch(vDelta));
    }
    if (pfWaistPitch) {
        vDelta[0] = waistPoint[0] - lowerPoint[0];
        vDelta[1] = waistPoint[1] - lowerPoint[1];
        vDelta[2] = waistPoint[2] - lowerPoint[2];
        *pfWaistPitch = AngleNormalize180(vectopitch(vDelta));
    }

    if (!success) {
        goto prone_clear_or_air_success;
    }

    return 1;

prone_clear_or_air_success:
    if (bOnGround) {
        return 0;
    }
    if (pfTorsoHeight) {
        *pfTorsoHeight = 0.0f;
    }
    if (pfTorsoPitch) {
        *pfTorsoPitch = 0.0f;
    }
    if (pfWaistPitch) {
        *pfWaistPitch = 0.0f;
    }
    return 1;
}

qboolean BG_CheckProne(int passEntityNum, const vec_t *vPos, const float fSize, const float fHeight, const float fYaw, float *pfTorsoHeight, float *pfTorsoPitch, float *pfWaistPitch, const qboolean bAlreadyProne, const qboolean bOnGround, vec_t *vGroundNormal, int handler, proneCheckType_t proneCheckType, float prone_feet_dist)
{
    return BG_CheckProneValid(passEntityNum, vPos, fSize, fHeight, fYaw, pfTorsoHeight, pfTorsoPitch, pfWaistPitch, bAlreadyProne, bOnGround, vGroundNormal, (unsigned char)handler, proneCheckType, prone_feet_dist);
}

void BG_EvaluateTrajectory(const trajectory_t *tr, int atTime, vec_t *result)
{
    float elapsed;
    float speed;
    float scale;
    int endTime;

    switch (tr->trType) {
    case TR_STATIONARY:
    case TR_INTERPOLATE:
    case TR_GRAVITY_PAUSED:
        BG_Vec3Copy(tr->trBase, result);
        return;

    case TR_LINEAR:
        elapsed = (float)(atTime - tr->trTime) * 0.001f;
        BG_Vec3Mad(tr->trBase, elapsed, tr->trDelta, result);
        return;

    case TR_LINEAR_STOP:
        endTime = tr->trTime + tr->trDuration;
        if (atTime > endTime) {
            atTime = endTime;
        }
        elapsed = (float)(atTime - tr->trTime) * 0.001f;
        if (elapsed < 0.0f) {
            elapsed = 0.0f;
        }
        BG_Vec3Mad(tr->trBase, elapsed, tr->trDelta, result);
        return;

    case TR_SINE:
        elapsed = (float)(atTime - tr->trTime) / (float)tr->trDuration;
        scale = (float)sin((double)elapsed * 3.141592653589793 * 2.0);
        BG_Vec3Mad(tr->trBase, scale, tr->trDelta, result);
        return;

    case TR_GRAVITY:
        elapsed = (float)(atTime - tr->trTime) * 0.001f;
        result[0] = tr->trBase[0] + tr->trDelta[0] * elapsed;
        result[1] = tr->trBase[1] + tr->trDelta[1] * elapsed;
        result[2] = tr->trBase[2] + tr->trDelta[2] * elapsed + -400.0f * elapsed * elapsed;
        return;

    case TR_ACCELERATE:
        endTime = tr->trTime + tr->trDuration;
        if (atTime > endTime) {
            atTime = endTime;
        }
        elapsed = (float)(atTime - tr->trTime) * 0.001f;
        speed = Vec3NormalizeTo(tr->trDelta, result) / ((float)tr->trDuration * 0.001f);
        scale = 0.5f * speed * elapsed * elapsed;
        result[0] = tr->trBase[0] + result[0] * scale;
        result[1] = tr->trBase[1] + result[1] * scale;
        result[2] = tr->trBase[2] + result[2] * scale;
        return;

    case TR_DECELERATE:
        endTime = tr->trTime + tr->trDuration;
        if (endTime < atTime) {
            atTime = endTime;
        }
        elapsed = (float)(atTime - tr->trTime) * 0.001f;
        speed = Vec3NormalizeTo(tr->trDelta, result) / ((float)tr->trDuration * 0.001f);
        scale = -0.5f * speed * elapsed * elapsed;
        result[0] = tr->trBase[0] + tr->trDelta[0] * elapsed + result[0] * scale;
        result[1] = tr->trBase[1] + tr->trDelta[1] * elapsed + result[1] * scale;
        result[2] = tr->trBase[2] + tr->trDelta[2] * elapsed + result[2] * scale;
        return;

    default: {
        Com_Error(1, "BG_EvaluateTrajectory: unknown trajectory type %d", tr->trType);
        return;
    }
    }
}

qboolean BG_PlayerTouchesItem(playerState_t *ps, entityState_t *item, int atTime)
{
    vec3_t origin;
    float dx, dy, dz;

    BG_EvaluateTrajectory(&item->pos, atTime, origin);

    dx = ps->origin[0] - origin[0];
    if (__builtin_expect(dx > 36.0f || dx < -36.0f, 0))
        return 0;

    dy = ps->origin[1] - origin[1];
    if (__builtin_expect(dy > 36.0f || dy < -36.0f, 0))
        return 0;

    dz = ps->origin[2] - origin[2];
    if (__builtin_expect(dz > 18.0f || dz < -88.0f, 0))
        return 0;

    return 1;
}

const int singleClientEvents[7] = { 140, 141, 142, 143, 185, 186, -1 }; /* truncated 40->7 (blob over-capture) */
