#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
extern char *getenv(const char *name);
extern scr_const_t scr_const;
extern scr_data_t g_scr_data;
extern bgs_t level_bgs;
extern level_locals_t level;

void PlayerCmd_takeWeapon(struct scr_entref_t entref);
void PlayerCmd_takeAllWeapons(scr_entref_t entref);
static void PlayerCmd_setAngles(scr_entref_t entref);
static void PlayerCmd_getAngles(scr_entref_t entref);
void PlayerCmd_useButtonPressed(scr_entref_t entref);
void PlayerCmd_attackButtonPressed(scr_entref_t entref);
void PlayerCmd_meleeButtonPressed(scr_entref_t entref);
void PlayerCmd_playerADS(scr_entref_t entref);
void PlayerCmd_isOnGround(scr_entref_t entref);
void PlayerCmd_pingPlayer(scr_entref_t entref);
static void PlayerCmd_SetViewmodel(scr_entref_t entref);
static void PlayerCmd_GetViewmodel(scr_entref_t entref);
static void PlayerCmd_showScoreboard(scr_entref_t entref);
static void PlayerCmd_setSpawnWeapon(scr_entref_t entref);
static void PlayerCmd_dropItem(scr_entref_t entref);
static void PlayerCmd_Suicide(scr_entref_t entref);
static void PlayerCmd_OpenMenu(scr_entref_t entref);
static void PlayerCmd_OpenMenuNoMouse(scr_entref_t entref);
static void PlayerCmd_CloseMenu(scr_entref_t entref);
static void PlayerCmd_CloseInGameMenu(scr_entref_t entref);
static void PlayerCmd_SetWeaponSlotWeapon(scr_entref_t entref);
static void PlayerCmd_SetWeaponSlotAmmo(scr_entref_t entref);
static void PlayerCmd_GetWeaponSlotClipAmmo(scr_entref_t entref);
static void PlayerCmd_SetWeaponSlotClipAmmo(scr_entref_t entref);
static void PlayerCmd_SetWeaponClipAmmo(scr_entref_t entref);
static void iclientprintln(scr_entref_t entref);
static void iclientprintlnbold(scr_entref_t entref);
void PlayerCmd_spawn(scr_entref_t entref);
static void PlayerCmd_setEnterTime(scr_entref_t entref);
void BodyEnd(gentity_t *ent);
static void PlayerCmd_SetClientDvar(scr_entref_t entref);
static void PlayerCmd_IsTalking(scr_entref_t entref);
static void PlayerCmd_FreezeControls(scr_entref_t entref);
static void PlayerCmd_DisableWeapon(scr_entref_t entref);
static void PlayerCmd_EnableWeapon(scr_entref_t entref);
static void PlayerCmd_SetReverb(scr_entref_t entref);
static void PlayerCmd_DeactivateReverb(scr_entref_t entref);
static void PlayerCmd_SetChannelVolumes(scr_entref_t entref);
static void PlayerCmd_DeactivateChannelVolumes(scr_entref_t entref);
static void ScrCmd_IsLookingAt(scr_entref_t entref);
static void ScrCmd_PlayLocalSound(scr_entref_t entref);
static void PlayerCmd_SayAll(scr_entref_t entref);
static void PlayerCmd_SayTeam(scr_entref_t entref);
static void PlayerCmd_AllowSpectateTeam(scr_entref_t entref);
static void PlayerCmd_GetGuid(scr_entref_t entref);
BuiltinMethod Player_GetMethod(const char **pName);
void PlayerCmd_giveWeapon(scr_entref_t entref);
void PlayerCmd_hasWeapon(scr_entref_t entref);
void PlayerCmd_switchToWeapon(scr_entref_t entref);
void PlayerCmd_switchToOffhand(scr_entref_t entref);
void PlayerCmd_giveStartAmmo(scr_entref_t entref);
void PlayerCmd_giveMaxAmmo(scr_entref_t entref);
void PlayerCmd_getFractionStartAmmo(scr_entref_t entref);
void PlayerCmd_getFractionMaxAmmo(scr_entref_t entref);
static void PlayerCmd_setOrigin(scr_entref_t entref);
static void PlayerCmd_finishPlayerDamage(scr_entref_t entref);
static void PlayerCmd_ClonePlayer(scr_entref_t entref);
void PlayerCmd_getCurrentWeapon(scr_entref_t entref);
void PlayerCmd_getCurrentOffhand(scr_entref_t entref);
static void PlayerCmd_GetWeaponSlotWeapon(scr_entref_t entref);
static void PlayerCmd_GetWeaponSlotAmmo(scr_entref_t entref);

extern void Scr_ObjectError(const char *msg);
extern void Scr_Error(const char *msg);
extern void Com_Printf(const char *fmt, ...);
extern const char *va(const char *fmt, ...);
extern const char *Scr_GetString(unsigned int index);
extern int GScr_GetScriptMenuIndex(const char *menu);
extern void SV_GameSendServerCommand(int clientNum, int type, const char *text);
extern int SV_GetGuid(int clientNum);
extern void Scr_AddInt(int value);
extern void Scr_AddFloat(float value);
extern void Scr_AddVector(vec_t *value);
extern void Scr_GetVector(unsigned int index, vec_t *value);
extern void Scr_AddString(const char *value);
extern void Scr_AddBool(int value);
extern void Scr_AddConstString(unsigned int value);
extern void Scr_AddEntity(gentity_t *ent);
extern void Scr_ParamError(unsigned int index, const char *msg);
extern unsigned int Scr_GetNumParam(void);
extern int Scr_GetType(unsigned int index);
extern int Scr_GetPointerType(unsigned int index);
extern unsigned int Scr_GetConstLowercaseString(unsigned int index);
extern int G_ModelIndex(const char *name);
extern const char *G_ModelName(int index);
extern int G_GetWeaponIndexForName(const char *name);
extern int BG_FindWeaponIndexForName(const char *name);
extern int BG_GetNumWeapons(void);
extern int BG_AmmoForWeapon(int weapon);
extern int BG_ClipForWeapon(int weapon);
extern int BG_GetAmmoTypeMax(int iAmmoIndex);
extern int BG_GetAmmoClipSize(int iClipIndex);
extern int BG_GetWeaponSlotForName(const char *pszSlotName);
extern const char *BG_GetWeaponSlotNameForIndex(int iSlot);
extern qboolean BG_WeaponIsClipOnly(int weapon);
extern WeaponDef *BG_GetWeaponDef(int iWeapon);
extern Bool BG_DoesWeaponNeedSlot(int weapIndex);
extern int BG_GetEmptySlotForWeapon(const playerState_t *pPS, int iWeaponIndex);
extern qboolean BG_TakePlayerWeapon(playerState_t *pPS, int iWeaponIndex);
extern Bool BG_IsWeaponValid(const playerState_t *ps, int weaponIndex);
extern qboolean G_GivePlayerWeapon(playerState_t *pPS, int iWeaponIndex);
extern void G_SelectWeaponIndex(int clientNum, int iWeaponIndex);
extern void G_SetEquippedOffHand(int clientNum, int offHandIndex);
extern qboolean SV_BotIsTestClient(int clientNum);
extern qboolean SV_BotStop(int clientNum);
extern qboolean SV_BotSetMovement(int clientNum, int forward, int right);
extern qboolean SV_BotSetAngles(int clientNum, const vec_t *angles);
extern qboolean SV_BotSetWeapon(int clientNum, int weapon);
extern qboolean SV_BotSetAction(int clientNum, const char *action);
extern void BG_PlayerStateToEntityState(playerState_t *ps, gentity_t *ent, qboolean snap, qboolean forceSnap);
extern void SetClientViewAngle(gentity_t *ent, const vec_t *angles);
extern void Cmd_Score_f(gentity_t *ent);
extern gentity_t *Drop_Weapon(gentity_t *pEnt, int iWeaponIndex, unsigned int tag);
extern const gitem_t *G_FindItem(const char *pickupName);
extern gentity_t *Drop_Item(gentity_t *ent, const gitem_t *item, float angle, qboolean novelocity);
extern int Add_Ammo(gentity_t *ent, int weapon, int count, qboolean fillClip);
extern void GScr_AddEntity(gentity_t *pEnt);
extern void player_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath, int iWeapon, const vec_t *vDir, const hitLocation_t hitLoc, int psTimeOffset);
extern void Scr_MakeGameMessage(int iClientNum, const char *pszCmd);
extern void ClientSpawn(gentity_t *ent, const vec_t *spawn_origin, const vec_t *spawn_angles);
extern int Scr_GetInt(unsigned int index);
extern float Scr_GetFloat(unsigned int index);
extern unsigned int Scr_GetConstString(unsigned int index);
extern int G_FindConfigstringIndex(const char *name, int start, int max, qboolean create, const char *errormsg);
extern gentity_t *Scr_GetEntity(unsigned int index);
extern SoundAlias G_SoundAliasIndex(const char *name);
extern unsigned int Scr_ConstructMessageString(int firstParmIndex, int lastParmIndex, const char *errorContext, char *string, int stringLimit);
extern void G_Say(gentity_t *ent, gentity_t *target, int mode, const char *chatText);
extern void Scr_Notify(gentity_t *ent, unsigned int stringValue, int paramCount);
extern void SV_UnlinkEntity(gentity_t *gEnt);
extern void SV_LinkEntity(gentity_t *gEnt);
extern const char *SL_ConvertToString(unsigned int stringValue);
extern const dvar_t *g_voiceChatTalkingDuration;
extern Bool Dvar_IsValidName(const char *dvarName);
extern char I_CleanChar(char character);
extern int G_IndexForMeansOfDeath(const char *pszMOD);
extern hitLocation_t G_GetHitLocationIndexFromString(int string);
extern const vec_t Vec3NormalizeTo(const vec_t *v, vec_t *out);
extern const float vectoyaw(const vec_t *vec);
extern const byte DirToByte(const vec_t *dir);
extern gentity_t *G_TempEntity(const vec_t *origin, int event);
extern entityHandler_t entityHandlers[20];
extern const dvar_t *player_dmgtimer_timePerPoint;
extern const dvar_t *player_dmgtimer_maxTime;
extern const dvar_t *g_knockback;
extern const dvar_t *g_clonePlayerMaxVelocity;
extern gentity_t *G_SpawnPlayerClone(void);
extern int G_GetFreePlayerCorpseIndex(void);
extern DObj *Com_GetServerDObj(int entNum);
extern XAnimTree *DObjGetTree(const DObj *obj);
extern void XAnimCloneAnimTree(const XAnimTree *from, XAnimTree *to);
extern unsigned char G_SetOrigin(gentity_t *ent, const vec_t *origin);
extern unsigned char G_SetAngle(gentity_t *ent, const vec_t *angle);

static inline __attribute__((always_inline)) gentity_t *PlayerCmd_GetPlayerEntity(scr_entref_t entref)
{
    unsigned int entnum;
    gentity_t *pSelf;

    entnum = entref.entnum;
    if (entref.classnum != 0) {
        Scr_ObjectError("not an entity");
        return NULL;
    }

    pSelf = (gentity_t *)((byte *)imp_g_entities + entnum * sizeof(gentity_t));
    if (!pSelf->client) {
        Scr_ObjectError(va("entity %i is not a player", (int)entnum));
        return NULL;
    }

    return pSelf;
}

static inline __attribute__((always_inline)) void PlayerCmd_AddButtonPressed(scr_entref_t entref, int buttonMask)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    gclient_t *client;

    if (!pSelf)
        return;

    client = pSelf->client;
    Scr_AddInt(((client->buttonsSinceLastFrame | client->buttons) & buttonMask) != 0);
}

static inline __attribute__((always_inline)) qboolean PlayerCmd_HasWeapon(const playerState_t *ps, int weaponIndex)
{
    if (weaponIndex <= 0)
        return 0;

    return ((ps->weapons[weaponIndex >> 5] >> (weaponIndex & 0x1f)) & 1) != 0;
}

static inline __attribute__((always_inline)) int PlayerCmd_GetWeaponSlotParam(unsigned int paramIndex)
{
    scr_string_t slotName = (scr_string_t)Scr_GetConstString(paramIndex);
    int slot = BG_GetWeaponSlotForName(SL_ConvertToString(slotName));

    if (!slot)
        Scr_ParamError(paramIndex, va("Unknown weaponslot name %s. Valid weaponslots are \"primary\" and \"primaryb\"", SL_ConvertToString(slotName)));

    return slot;
}

static inline __attribute__((always_inline)) int PlayerCmd_ClampAmmoCount(int value, int max)
{
    if (value < 0)
        return 0;
    if (value > max)
        return max;
    return value;
}

static inline __attribute__((always_inline)) int PlayerCmd_GetEnvEffectPriority(unsigned int paramIndex)
{
    const scr_const_t *scrConst = (const scr_const_t *)imp_scr_const;
    scr_string_t priority = (scr_string_t)Scr_GetConstString(paramIndex);

    if (priority == scrConst->snd_enveffectsprio_level)
        return 1;
    if (priority == scrConst->snd_enveffectsprio_shellshock)
        return 2;

    Scr_Error("priority must be 'snd_enveffectsprio_level' or 'snd_enveffectsprio_shellshock'\n");
    return 1;
}

static inline __attribute__((always_inline)) int PlayerCmd_GetChannelVolPriority(unsigned int paramIndex)
{
    const scr_const_t *scrConst = (const scr_const_t *)imp_scr_const;
    scr_string_t priority = (scr_string_t)Scr_GetConstString(paramIndex);

    if (priority == scrConst->snd_channelvolprio_holdbreath)
        return 1;
    if (priority == scrConst->snd_channelvolprio_pain)
        return 2;
    if (priority == scrConst->snd_channelvolprio_shellshock)
        return 3;

    Scr_Error("priority must be 'snd_channelvolprio_holdbreath', 'snd_channelvolprio_pain', or 'snd_channelvolprio_shellshock'\n");
    return 1;
}

static inline __attribute__((always_inline)) void PlayerCmd_CleanDvarValue(const char *dvarValue, char *outString, int outLimit)
{
    int i;

    for (i = 0; dvarValue[i] && i < outLimit - 1; ++i) {
        char ch = I_CleanChar((signed char)dvarValue[i]);

        outString[i] = (ch == '"') ? '\'' : ch;
    }

    outString[i] = '\0';
}

void PlayerCmd_takeWeapon(struct scr_entref_t entref)
{
    unsigned short entnum;
    unsigned short classnum;
    gentity_t *pSelf;
    const char *weaponName;
    int iWeaponIndex;
    gclient_t *client;
    int ammoIndex;
    int clipIndex;

    entnum = entref.entnum;
    classnum = entref.classnum;

    if (classnum != 0) {
        Scr_ObjectError("not an entity");
        pSelf = NULL;
    } else {
        gclient_t *clientCheck;
        pSelf = &((gentity_t *)imp_g_entities)[entnum];
        clientCheck = pSelf->client;
        if (clientCheck == NULL) {
            Scr_ObjectError(va("entity %i is not a player", (int)entnum));
        }
    }

    weaponName = (const char *)Scr_GetString(0);
    iWeaponIndex = G_GetWeaponIndexForName(weaponName);

    client = pSelf->client;
    ammoIndex = BG_AmmoForWeapon(iWeaponIndex);
    client->ps.ammo[ammoIndex] = 0;

    client = pSelf->client;
    clipIndex = BG_ClipForWeapon(iWeaponIndex);
    client->ps.ammoclip[clipIndex] = 0;

    BG_TakePlayerWeapon(&pSelf->client->ps, iWeaponIndex);
}

void PlayerCmd_takeAllWeapons(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    int iWeaponIndex;

    pSelf->client->ps.weapon = 0;
    for (iWeaponIndex = 1; iWeaponIndex <= BG_GetNumWeapons(); ++iWeaponIndex) {
        pSelf->client->ps.ammo[BG_AmmoForWeapon(iWeaponIndex)] = 0;
        pSelf->client->ps.ammoclip[BG_ClipForWeapon(iWeaponIndex)] = 0;
        BG_TakePlayerWeapon(&pSelf->client->ps, iWeaponIndex);
    }
}

void PlayerCmd_setAngles(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    vec3_t angles;

    if (!pSelf)
        return;

    Scr_GetVector(0, angles);
    SetClientViewAngle(pSelf, angles);
}

void PlayerCmd_getAngles(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);

    { (void)(Scr_AddVector(pSelf->client->ps.viewangles)); return; }
}

void PlayerCmd_useButtonPressed(scr_entref_t entref)
{
    PlayerCmd_AddButtonPressed(entref, 0x28);
}

void PlayerCmd_attackButtonPressed(scr_entref_t entref)
{
    PlayerCmd_AddButtonPressed(entref, 0x01);
}

void PlayerCmd_meleeButtonPressed(scr_entref_t entref)
{
    PlayerCmd_AddButtonPressed(entref, 0x04);
}

void PlayerCmd_playerADS(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);

    { (void)(Scr_AddFloat(pSelf->client->ps.fWeaponPosFrac)); return; }
}

void PlayerCmd_isOnGround(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);

    if (!pSelf)
        return;

    Scr_AddInt(pSelf->client->ps.groundEntityNum != 0x3ff);
}

void PlayerCmd_pingPlayer(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);

    pSelf->client->ps.eFlags |= 0x00400000;
    pSelf->client->compassPingTime = level.time + 3000;
}

void PlayerCmd_SetViewmodel(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    const char *modelName;

    modelName = Scr_GetString(0);
    if (!modelName || !modelName[0])
        Scr_ParamError(0, "model name string is empty");

    pSelf->client->sess.viewmodelIndex = G_ModelIndex(modelName);
}

void PlayerCmd_GetViewmodel(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);

    { (void)(Scr_AddString(G_ModelName(pSelf->client->sess.viewmodelIndex))); return; }
}

void PlayerCmd_showScoreboard(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);

    if (!pSelf)
        return;

    Cmd_Score_f(pSelf);
}

void PlayerCmd_setSpawnWeapon(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    int iWeaponIndex;

    iWeaponIndex = G_GetWeaponIndexForName(Scr_GetString(0));
    if (BG_IsWeaponValid(&pSelf->client->ps, iWeaponIndex)) {
        pSelf->client->ps.weapon = iWeaponIndex;
        pSelf->client->ps.weaponstate = 0;
    }
}

void PlayerCmd_dropItem(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    const scr_const_t *scr_const = (const scr_const_t *)imp_scr_const;
    const char *pszItemName;
    int iWeaponIndex;
    const gitem_t *item;
    gentity_t *dropped = NULL;
    unsigned int tagName;

    pszItemName = Scr_GetString(0);
    iWeaponIndex = G_GetWeaponIndexForName(pszItemName);
    if (iWeaponIndex) {
        if (Scr_GetNumParam() > 1)
            tagName = Scr_GetConstLowercaseString(1);
        else
            tagName = scr_const->tag_weapon_right;

        dropped = Drop_Weapon(pSelf, iWeaponIndex, tagName);
    } else {
        item = G_FindItem(pszItemName);
        if (item)
            dropped = Drop_Item(pSelf, item, 0.0f, 0);
    }

    { (void)(GScr_AddEntity(dropped)); return; }
}

void PlayerCmd_Suicide(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);

    pSelf->flags &= ~3;
    pSelf->health = 0;
    pSelf->client->ps.stats[0] = 0;
    player_die(pSelf, pSelf, pSelf, 100000, 12, 0, NULL, 0, 0);
}

void PlayerCmd_OpenMenu(scr_entref_t entref)
{
    gentity_t *pSelf;
    const char *menuName;
    int menuIndex;
    static int traceCount;

    pSelf = PlayerCmd_GetPlayerEntity(entref);
    menuName = Scr_GetString(0);
    if (pSelf->client->sess.connected != CON_CONNECTED) {
        if (traceCount++ < 16) {
            if (getenv("MTRACE"))
                Com_Printf("[menu-trace] openMenu client=%u connected=%d menu='%s' rejected\n",
                           entref.entnum, pSelf->client->sess.connected, menuName);
        }
        { (void)(Scr_AddInt(0)); return; }
    }

    menuIndex = GScr_GetScriptMenuIndex(menuName);
    if (traceCount++ < 16) {
        if (getenv("MTRACE"))
            Com_Printf("[menu-trace] openMenu client=%u connected=%d menu='%s' index=%d send='t %d'\n",
                       entref.entnum, pSelf->client->sess.connected, menuName, menuIndex, menuIndex);
    }
    SV_GameSendServerCommand(entref.entnum, 1, va("%c %i", 0x74, menuIndex));
    { (void)(Scr_AddInt(1)); return; }
}

void PlayerCmd_OpenMenuNoMouse(scr_entref_t entref)
{
    gentity_t *pSelf;
    const char *menuName;
    int menuIndex;
    static int traceCount;

    pSelf = PlayerCmd_GetPlayerEntity(entref);
    menuName = Scr_GetString(0);
    if (pSelf->client->sess.connected != CON_CONNECTED) {
        if (traceCount++ < 16) {
            if (getenv("MTRACE"))
                Com_Printf("[menu-trace] openMenuNoMouse client=%u connected=%d menu='%s' rejected\n",
                           entref.entnum, pSelf->client->sess.connected, menuName);
        }
        { (void)(Scr_AddInt(0)); return; }
    }

    menuIndex = GScr_GetScriptMenuIndex(menuName);
    if (traceCount++ < 16) {
        if (getenv("MTRACE"))
            Com_Printf("[menu-trace] openMenuNoMouse client=%u connected=%d menu='%s' index=%d send='t %d 1'\n",
                       entref.entnum, pSelf->client->sess.connected, menuName, menuIndex, menuIndex);
    }
    SV_GameSendServerCommand(entref.entnum, 1, va("%c %i 1", 0x74, menuIndex));
    { (void)(Scr_AddInt(1)); return; }
}

void PlayerCmd_CloseMenu(scr_entref_t entref)
{
    PlayerCmd_GetPlayerEntity(entref);
    SV_GameSendServerCommand(entref.entnum, 1, va("%c", 0x75));
}

void PlayerCmd_CloseInGameMenu(scr_entref_t entref)
{
    PlayerCmd_GetPlayerEntity(entref);
    SV_GameSendServerCommand(entref.entnum, 1, va("%c", 0x4b));
}

void PlayerCmd_SetWeaponSlotWeapon(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    playerState_t *ps;
    const char *weaponName;
    int slot;
    int weaponIndex;
    int oldWeapon;
    int hadWeapon;
    int swapToSecondPrimary;
    int ammoToAdd;
    WeaponDef *weapDef;

    ps = &pSelf->client->ps;
    slot = PlayerCmd_GetWeaponSlotParam(0);
    weaponName = Scr_GetString(1);

    if (weaponName[0]) {
        weaponIndex = G_GetWeaponIndexForName(weaponName);
        if (!weaponIndex)
            Scr_ParamError(1, va("Unknown weapon %s.", weaponName));

        weapDef = BG_GetWeaponDef(weaponIndex);
        if (weapDef->weapSlot != slot &&
            ((weapDef->weapSlot - WEAPSLOT_PRIMARY) > 1 || (slot - WEAPSLOT_PRIMARY) > 1)) {
            Scr_ParamError(1, va("Weapon %s goes in the %s weaponslot, not the %s weaponslot.", weaponName,
                                 BG_GetWeaponSlotNameForIndex(weapDef->weapSlot),
                                 BG_GetWeaponSlotNameForIndex(slot)));
        }
    } else {
        weaponIndex = 0;
        weapDef = NULL;
    }

    oldWeapon = (signed char)ps->weaponslots[slot];
    if (oldWeapon)
        BG_TakePlayerWeapon(ps, oldWeapon);

    if (!weaponIndex)
        return;

    swapToSecondPrimary = (slot == WEAPSLOT_PRIMARYB && !ps->weaponslots[WEAPSLOT_PRIMARY]);
    hadWeapon = PlayerCmd_HasWeapon(ps, weaponIndex);
    G_GivePlayerWeapon(ps, weaponIndex);

    if (swapToSecondPrimary) {
        ps->weaponslots[WEAPSLOT_PRIMARYB] = ps->weaponslots[WEAPSLOT_PRIMARY];
        ps->weaponslots[WEAPSLOT_PRIMARY] = 0;
    }

    ammoToAdd = weapDef->iStartAmmo - ps->ammo[weapDef->iAmmoIndex];
    if (ammoToAdd > 0)
        Add_Ammo(pSelf, weaponIndex, ammoToAdd, !hadWeapon);
}

void PlayerCmd_SetWeaponSlotAmmo(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    int slot;
    int setAmmo;
    int weaponIndex;
    int ammoIndex;

    slot = PlayerCmd_GetWeaponSlotParam(0);
    setAmmo = Scr_GetInt(1);
    weaponIndex = (signed char)pSelf->client->ps.weaponslots[slot];
    if (!weaponIndex)
        return;

    if (BG_WeaponIsClipOnly(weaponIndex)) {
        ammoIndex = BG_ClipForWeapon(weaponIndex);
        if (!ammoIndex)
            return;

        pSelf->client->ps.ammoclip[ammoIndex] = PlayerCmd_ClampAmmoCount(setAmmo, BG_GetAmmoClipSize(ammoIndex));
    } else {
        ammoIndex = BG_AmmoForWeapon(weaponIndex);
        if (!ammoIndex)
            return;

        pSelf->client->ps.ammo[ammoIndex] = PlayerCmd_ClampAmmoCount(setAmmo, BG_GetAmmoTypeMax(ammoIndex));
    }
}

void PlayerCmd_GetWeaponSlotClipAmmo(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    int slot;
    int weaponIndex;
    int clipIndex;

    if (!pSelf)
        return;

    if (pSelf->client->sess.sessionState != SESS_STATE_PLAYING) {
        Scr_AddInt(0);
        return;
    }

    slot = PlayerCmd_GetWeaponSlotParam(0);
    weaponIndex = (signed char)pSelf->client->ps.weaponslots[slot];
    if (!weaponIndex) {
        Scr_AddInt(0);
        return;
    }

    clipIndex = BG_ClipForWeapon(weaponIndex);
    if (!clipIndex) {
        Scr_AddInt(0);
        return;
    }

    Scr_AddInt(pSelf->client->ps.ammoclip[clipIndex]);
}

void PlayerCmd_SetWeaponSlotClipAmmo(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    int slot;
    int setClipAmmo;
    int weaponIndex;
    int clipIndex;

    slot = PlayerCmd_GetWeaponSlotParam(0);
    setClipAmmo = Scr_GetInt(1);
    weaponIndex = (signed char)pSelf->client->ps.weaponslots[slot];
    if (!weaponIndex) {
        Scr_AddInt(0);
        return;
    }

    clipIndex = BG_ClipForWeapon(weaponIndex);
    if (!clipIndex)
        return;

    pSelf->client->ps.ammoclip[clipIndex] = PlayerCmd_ClampAmmoCount(setClipAmmo, BG_GetAmmoClipSize(clipIndex));
}

void PlayerCmd_SetWeaponClipAmmo(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    const char *weaponName;
    int ammoCount;
    int weaponIndex;
    int clipIndex;

    weaponName = Scr_GetString(0);
    ammoCount = Scr_GetInt(1);
    weaponIndex = G_GetWeaponIndexForName(weaponName);
    if (!weaponIndex) {
        Scr_AddInt(0);
        return;
    }

    clipIndex = BG_ClipForWeapon(weaponIndex);
    if (!clipIndex)
        return;

    pSelf->client->ps.ammoclip[clipIndex] = PlayerCmd_ClampAmmoCount(ammoCount, BG_GetAmmoClipSize(clipIndex));
}

void iclientprintln(scr_entref_t entref)
{
    PlayerCmd_GetPlayerEntity(entref);
    Scr_MakeGameMessage(entref.entnum, "f");
}

void iclientprintlnbold(scr_entref_t entref)
{
    PlayerCmd_GetPlayerEntity(entref);
    Scr_MakeGameMessage(entref.entnum, "g");
}

void PlayerCmd_spawn(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    vec3_t spawn_origin;
    vec3_t spawn_angles;

    if (!pSelf)
        return;

    Scr_GetVector(0, spawn_origin);
    Scr_GetVector(1, spawn_angles);
    ClientSpawn(pSelf, spawn_origin, spawn_angles);
}

void PlayerCmd_setEnterTime(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);

    pSelf->client->sess.enterTime = Scr_GetInt(0);
}

void BodyEnd(gentity_t *ent)
{
    ent->s.eFlags &= 0xfff7ffff;
    ent->r.contents = 0x4000000;
    ent->r.svFlags = 0;
}

void PlayerCmd_SetClientDvar(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    const char *pszDvar;
    const char *pszText;
    char szString[1024];
    char szOutString[1024];

    if (!pSelf)
        return;

    pszDvar = Scr_GetString(0);
    if (Scr_GetType(1) == 3) {
        Scr_ConstructMessageString(1, Scr_GetNumParam() - 1, "Client Dvar Value", szString, sizeof(szString));
        pszText = szString;
    } else {
        pszText = Scr_GetString(1);
    }

    if (!Dvar_IsValidName(pszDvar)) {
        Scr_Error(va("Dvar %s has an invalid dvar name", pszDvar));
        return;
    }

    PlayerCmd_CleanDvarValue(pszText, szOutString, sizeof(szOutString));
    if (strncmp(pszDvar, "ui_allow_join", 13) == 0) {
        Com_Printf("setClientCvar client=%u %s='%s'\n",
                   entref.entnum, pszDvar, szOutString);
    }
    SV_GameSendServerCommand(entref.entnum, 1, va("%c %s \"%s\"", 0x76, pszDvar, szOutString));
}

void PlayerCmd_IsTalking(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    int delta;

    if (!pSelf)
        return;

    delta = level.time - pSelf->client->lastVoiceTime;
    Scr_AddInt(delta >= 0 && delta < g_voiceChatTalkingDuration->current.integer);
}

void PlayerCmd_FreezeControls(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);

    pSelf->client->bFrozen = Scr_GetInt(0);
}

void PlayerCmd_DisableWeapon(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);

    pSelf->client->ps.pm_flags |= 0x04000000;
}

void PlayerCmd_EnableWeapon(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);

    pSelf->client->ps.pm_flags &= ~0x04000000;
}

void PlayerCmd_SetReverb(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    unsigned int paramCount;
    const char *pszReverb;
    int priority;
    float drylevel = 1.0f;
    float wetlevel = 0.5f;
    float fadetime = 0.0f;

    if (!pSelf)
        return;

    paramCount = Scr_GetNumParam();
    if (paramCount < 2 || paramCount > 5)
        Scr_Error("USAGE: player setReverb(\"priority\", \"roomtype\", drylevel = 1.0, wetlevel = 0.5, fadetime = 0);\nValid priorities are \"snd_enveffectsprio_level\" or \"snd_enveffectsprio_shellshock\", dry level is a float from 0 (no source sound) to 1 (full source sound), wetlevel is a float from 0 (no effect) to 1 (full effect), fadetime is in sec and modifies drylevel and wetlevel\n");

    if (paramCount >= 3)
        drylevel = Scr_GetFloat(2);
    if (paramCount >= 4)
        wetlevel = Scr_GetFloat(3);
    if (paramCount >= 5)
        fadetime = Scr_GetFloat(4);

    pszReverb = Scr_GetString(1);
    priority = PlayerCmd_GetEnvEffectPriority(0);
    SV_GameSendServerCommand(entref.entnum, 1, va("%c %i \"%s\" %g %g %g", 'r', priority, pszReverb, drylevel, wetlevel, fadetime));
}

void PlayerCmd_DeactivateReverb(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    unsigned int paramCount;
    int priority;
    float fadetime = 0.0f;

    if (!pSelf)
        return;

    paramCount = Scr_GetNumParam();
    if (paramCount < 1 || paramCount > 2)
        Scr_Error("USAGE: player deactivateReverb(\"priority\", fadetime = 0);\nValid priorities are \"snd_enveffectsprio_level\" or \"snd_enveffectsprio_shellshock\", fadetime is the time spent fading to the next lowest active reverb priority level in seconds\n");

    if (paramCount == 2)
        fadetime = Scr_GetFloat(1);

    priority = PlayerCmd_GetEnvEffectPriority(0);
    SV_GameSendServerCommand(entref.entnum, 1, va("%c %i %g", 'D', priority, fadetime));
}

void PlayerCmd_SetChannelVolumes(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    unsigned int paramCount;
    int shockIndex;
    int priority;
    float fadetime = 0.0f;

    if (!pSelf)
        return;

    paramCount = Scr_GetNumParam();
    if (paramCount < 2 || paramCount > 3)
        Scr_Error("USAGE: player setchannelvolumes(\"priority\", \"shock name\", fadetime = 0);\nValid priorities are \"snd_channelvolprio_holdbreath\", \"snd_channelvolprio_pain\", or \"snd_channelvolprio_shellshock\", fadetime is in sec\n");

    if (paramCount == 3)
        fadetime = Scr_GetFloat(2);

    shockIndex = G_FindConfigstringIndex(Scr_GetString(1), 0x48e, 0x10, 0, 0);
    priority = PlayerCmd_GetChannelVolPriority(0);
    SV_GameSendServerCommand(entref.entnum, 1, va("%c %i %i %g", 'E', priority, shockIndex, fadetime));
}

void PlayerCmd_DeactivateChannelVolumes(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    unsigned int paramCount;
    int priority;
    float fadetime = 0.0f;

    if (!pSelf)
        return;

    paramCount = Scr_GetNumParam();
    if (paramCount < 1 || paramCount > 2)
        Scr_Error("USAGE: player deactivatechannelvolumes(\"priority\", fadetime = 0);\nValid priorities are \"snd_channelvolprio_holdbreath\", \"snd_channelvolprio_pain\", or \"snd_channelvolprio_shellshock\", fadetime is the time spent fading to the next lowest active reverb priority level in seconds\n");

    if (paramCount == 2)
        fadetime = Scr_GetFloat(1);

    priority = PlayerCmd_GetChannelVolPriority(0);
    SV_GameSendServerCommand(entref.entnum, 1, va("%c %i %g", 'F', priority, fadetime));
}

void ScrCmd_IsLookingAt(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);

    if (!pSelf)
        return;

    Scr_AddInt(pSelf->client->pLookatEnt == Scr_GetEntity(0));
}

void ScrCmd_PlayLocalSound(scr_entref_t entref)
{
    int soundIndex;

    PlayerCmd_GetPlayerEntity(entref);
    soundIndex = (byte)G_SoundAliasIndex(Scr_GetString(0));
    SV_GameSendServerCommand(entref.entnum, 0, va("%c %i", 's', soundIndex));
}

void PlayerCmd_SayAll(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    char szString[0x400];

    if (!pSelf)
        return;

    Scr_ConstructMessageString(0, Scr_GetNumParam() - 1, "Client Chat Message", szString + 1, sizeof(szString) - 1);
    szString[0] = 0x14;
    G_Say(pSelf, NULL, 0, szString);
}

void PlayerCmd_SayTeam(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    char szString[0x400];

    if (!pSelf)
        return;

    Scr_ConstructMessageString(0, Scr_GetNumParam() - 1, "Client Chat Message", szString + 1, sizeof(szString) - 1);
    szString[0] = 0x14;
    G_Say(pSelf, NULL, 1, szString);
}

void PlayerCmd_AllowSpectateTeam(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    const scr_const_t *scrConst = (const scr_const_t *)imp_scr_const;
    scr_string_t team = (scr_string_t)Scr_GetConstString(0);
    int teamBit;

    if (team == scrConst->axis)
        teamBit = 2;
    else if (team == scrConst->allies)
        teamBit = 4;
    else if (team == scrConst->none)
        teamBit = 1;
    else if (team == scrConst->freelook)
        teamBit = 0x10;
    else {
        Scr_ParamError(0, "team must be \"axis\", \"allies\", \"none\", or \"freelook\"");
        teamBit = 0;
    }

    if (Scr_GetInt(1))
        pSelf->client->sess.noSpectate &= ~teamBit;
    else
        pSelf->client->sess.noSpectate |= teamBit;
}

void PlayerCmd_GetGuid(scr_entref_t entref)
{
    PlayerCmd_GetPlayerEntity(entref);

    if (Scr_GetNumParam())
        Scr_Error("USAGE: self getGuid()\n");

    { (void)(Scr_AddInt(SV_GetGuid(entref.entnum))); return; }
}

static gentity_t *PlayerCmd_GetBotEntity(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);

    if (!pSelf)
        return NULL;

    if (!SV_BotIsTestClient(entref.entnum)) {
        Scr_ObjectError(va("entity %i is not a test client", (int)entref.entnum));
        return NULL;
    }

    return pSelf;
}

static qboolean PlayerCmd_BotParamCount(unsigned int expected, const char *usage)
{
    if (Scr_GetNumParam() == expected)
        return 1;

    Scr_Error(usage);
    return 0;
}

static void PlayerCmd_BotIsBot(scr_entref_t entref)
{
    if (!PlayerCmd_BotParamCount(0, "USAGE: self isBot()\n"))
        return;

    if (!PlayerCmd_GetPlayerEntity(entref))
        return;

    Scr_AddBool(SV_BotIsTestClient(entref.entnum));
}

static void PlayerCmd_BotStop(scr_entref_t entref)
{
    if (!PlayerCmd_BotParamCount(0, "USAGE: self botStop()\n") ||
        !PlayerCmd_GetBotEntity(entref)) {
        return;
    }

    Scr_AddBool(SV_BotStop(entref.entnum));
}

static void PlayerCmd_BotMovement(scr_entref_t entref)
{
    int forward;
    int right;

    if (!PlayerCmd_BotParamCount(2, "USAGE: self botMovement(<forward>, <right>)\n") ||
        !PlayerCmd_GetBotEntity(entref)) {
        return;
    }

    forward = Scr_GetInt(0);
    right = Scr_GetInt(1);
    Scr_AddBool(SV_BotSetMovement(entref.entnum, forward, right));
}

static void PlayerCmd_BotAngles(scr_entref_t entref)
{
    gentity_t *pSelf;
    vec3_t angles;

    if (!PlayerCmd_BotParamCount(1, "USAGE: self botAngles(<angles>)\n"))
        return;

    pSelf = PlayerCmd_GetBotEntity(entref);
    if (!pSelf)
        return;

    Scr_GetVector(0, angles);
    SetClientViewAngle(pSelf, angles);
    Scr_AddBool(SV_BotSetAngles(entref.entnum, angles));
}

static void PlayerCmd_BotWeapon(scr_entref_t entref)
{
    gentity_t *pSelf;
    const char *weaponName;
    int weaponIndex;

    if (!PlayerCmd_BotParamCount(1, "USAGE: self botWeapon(<weapon name>)\n"))
        return;

    pSelf = PlayerCmd_GetBotEntity(entref);
    if (!pSelf)
        return;

    weaponName = Scr_GetString(0);
    weaponIndex = G_GetWeaponIndexForName(weaponName);
    if (!weaponIndex)
        Scr_ParamError(0, va("unknown weapon '%s'", weaponName));

    if (!PlayerCmd_HasWeapon(&pSelf->client->ps, weaponIndex)) {
        Scr_AddBool(0);
        return;
    }

    Scr_AddBool(SV_BotSetWeapon(entref.entnum, weaponIndex));
}

static void PlayerCmd_BotAction(scr_entref_t entref)
{
    const char *action;

    if (!PlayerCmd_BotParamCount(1, "USAGE: self botAction(<+/-action>)\n") ||
        !PlayerCmd_GetBotEntity(entref)) {
        return;
    }

    action = Scr_GetString(0);
    if (!SV_BotSetAction(entref.entnum, action)) {
        Scr_ParamError(0, va("unknown bot action '%s'", action));
        return;
    }

    Scr_AddBool(1);
}

void PlayerCmd_giveWeapon(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    playerState_t *ps;
    int weaponIndex;
    int hadWeapon;
    int ammoToAdd;
    WeaponDef *weapDef;

    weaponIndex = G_GetWeaponIndexForName(Scr_GetString(0));
    ps = &pSelf->client->ps;
    hadWeapon = PlayerCmd_HasWeapon(ps, weaponIndex);
    weapDef = BG_GetWeaponDef(weaponIndex);

    if (BG_DoesWeaponNeedSlot(weaponIndex) && !BG_GetEmptySlotForWeapon(ps, weaponIndex)) {
        Scr_ParamError(0, va("Cannot give %s weapon %s without having an empty weapon slot - player currently has a %s and a %s\n",
                             pSelf->client->sess.cs.name,
                             weapDef->szDisplayName,
                             BG_GetWeaponDef((signed char)ps->weaponslots[WEAPSLOT_PRIMARY])->szDisplayName,
                             BG_GetWeaponDef((signed char)ps->weaponslots[WEAPSLOT_PRIMARYB])->szDisplayName));
    }

    if (G_GivePlayerWeapon(ps, weaponIndex))
        SV_GameSendServerCommand(entref.entnum, 0, va("%c \"%i\"", 0x49, 1));

    ammoToAdd = weapDef->iStartAmmo - ps->ammo[weapDef->iAmmoIndex];
    if (ammoToAdd > 0)
        Add_Ammo(pSelf, weaponIndex, ammoToAdd, !hadWeapon);
}

void PlayerCmd_hasWeapon(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    int weaponIndex;

    if (!pSelf)
        return;

    weaponIndex = BG_FindWeaponIndexForName(Scr_GetString(0));
    Scr_AddBool(PlayerCmd_HasWeapon(&pSelf->client->ps, weaponIndex));
}

void PlayerCmd_switchToWeapon(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    const char *weaponName;
    int weaponIndex;

    if (!pSelf)
        return;

    weaponName = Scr_GetString(0);
    weaponIndex = G_GetWeaponIndexForName(weaponName);
    if (!weaponIndex)
        Scr_ParamError(0, va("unknown weapon '%s'", weaponName));

    if (PlayerCmd_HasWeapon(&pSelf->client->ps, weaponIndex)) {
        G_SelectWeaponIndex(entref.entnum, weaponIndex);
        Scr_AddBool(1);
    } else {
        Scr_AddBool(0);
    }
}

void PlayerCmd_switchToOffhand(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    const char *weaponName;
    int weaponIndex;

    if (!pSelf)
        return;

    weaponName = Scr_GetString(0);
    weaponIndex = G_GetWeaponIndexForName(weaponName);
    if (!weaponIndex)
        Scr_ParamError(0, va("unknown weapon '%s'", weaponName));

    if (PlayerCmd_HasWeapon(&pSelf->client->ps, weaponIndex)) {
        G_SetEquippedOffHand(entref.entnum, weaponIndex);
        Scr_AddBool(1);
    } else {
        Scr_AddBool(0);
    }
}

void PlayerCmd_giveStartAmmo(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    int weaponIndex;
    WeaponDef *weapDef;
    int ammoToAdd;

    weaponIndex = G_GetWeaponIndexForName(Scr_GetString(0));
    if (!PlayerCmd_HasWeapon(&pSelf->client->ps, weaponIndex))
        return;

    weapDef = BG_GetWeaponDef(weaponIndex);
    ammoToAdd = weapDef->iStartAmmo - pSelf->client->ps.ammo[weapDef->iAmmoIndex];
    if (ammoToAdd > 0)
        Add_Ammo(pSelf, weaponIndex, ammoToAdd, 0);
}

void PlayerCmd_giveMaxAmmo(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    int weaponIndex;
    WeaponDef *weapDef;
    int ammoToAdd;

    weaponIndex = G_GetWeaponIndexForName(Scr_GetString(0));
    if (!PlayerCmd_HasWeapon(&pSelf->client->ps, weaponIndex))
        return;

    weapDef = BG_GetWeaponDef(weaponIndex);
    ammoToAdd = BG_GetAmmoTypeMax(weapDef->iAmmoIndex) - pSelf->client->ps.ammo[weapDef->iAmmoIndex];
    if (ammoToAdd > 0)
        Add_Ammo(pSelf, weaponIndex, ammoToAdd, 0);
}

void PlayerCmd_getFractionStartAmmo(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    int weaponIndex;
    WeaponDef *weapDef;
    int ammo;

    weaponIndex = G_GetWeaponIndexForName(Scr_GetString(0));
    if (!PlayerCmd_HasWeapon(&pSelf->client->ps, weaponIndex))
        { (void)(Scr_AddFloat(1.0f)); return; }

    weapDef = BG_GetWeaponDef(weaponIndex);
    if (weapDef->iStartAmmo <= 0)
        { (void)(Scr_AddFloat(1.0f)); return; }

    ammo = pSelf->client->ps.ammo[weapDef->iAmmoIndex];
    if (ammo <= 0)
        { (void)(Scr_AddFloat(0.0f)); return; }
    else
        { (void)(Scr_AddFloat((float)ammo / (float)weapDef->iStartAmmo)); return; }
}

void PlayerCmd_getFractionMaxAmmo(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    int weaponIndex;
    WeaponDef *weapDef;
    int maxAmmo;
    int ammo;

    weaponIndex = G_GetWeaponIndexForName(Scr_GetString(0));
    if (!PlayerCmd_HasWeapon(&pSelf->client->ps, weaponIndex))
        { (void)(Scr_AddFloat(1.0f)); return; }

    weapDef = BG_GetWeaponDef(weaponIndex);
    maxAmmo = BG_GetAmmoTypeMax(weapDef->iAmmoIndex);
    if (maxAmmo <= 0)
        { (void)(Scr_AddFloat(1.0f)); return; }

    ammo = pSelf->client->ps.ammo[weapDef->iAmmoIndex];
    if (ammo <= 0)
        { (void)(Scr_AddFloat(0.0f)); return; }
    else
        { (void)(Scr_AddFloat((float)ammo / (float)maxAmmo)); return; }
}

void PlayerCmd_setOrigin(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    vec3_t vNewOrigin;
    playerState_t *ps;

    Scr_GetVector(0, vNewOrigin);
    SV_UnlinkEntity(pSelf);

    ps = &pSelf->client->ps;
    ps->origin[0] = vNewOrigin[0];
    ps->origin[1] = vNewOrigin[1];
    ps->origin[2] = vNewOrigin[2] + 1.0f;
    ps->eFlags ^= 2;

    BG_PlayerStateToEntityState(ps, pSelf, 1, 1);
    pSelf->r.currentOrigin[0] = ps->origin[0];
    pSelf->r.currentOrigin[1] = ps->origin[1];
    pSelf->r.currentOrigin[2] = ps->origin[2];

    SV_LinkEntity(pSelf);
}

void PlayerCmd_finishPlayerDamage(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    gentity_t *world = (gentity_t *)imp_g_entities + 1022;
    gentity_t *inflictor;
    gentity_t *attacker;
    gclient_t *client;
    vec3_t vPoint;
    vec3_t vDir;
    vec3_t localdir;
    const vec_t *point;
    const vec_t *dir;
    int damage;
    int dflags;
    int mod;
    int iWeapon;
    hitLocation_t hitLoc;
    int psTimeOffset;
    float yaw;
    float viewYaw;
    float maxDamageTime;
    int health;

    damage = Scr_GetInt(2);
    if (damage <= 0)
        return;

    if (Scr_GetType(0) && Scr_GetPointerType(0) == 0x15)
        inflictor = Scr_GetEntity(1);
    else
        inflictor = world;

    if (Scr_GetType(1) && Scr_GetPointerType(1) == 0x15)
        attacker = Scr_GetEntity(1);
    else
        attacker = world;

    dflags = Scr_GetInt(3);
    mod = G_IndexForMeansOfDeath(Scr_GetString(4));
    iWeapon = G_GetWeaponIndexForName(Scr_GetString(5));

    if (Scr_GetType(6)) {
        Scr_GetVector(6, vPoint);
        point = vPoint;
    } else {
        point = NULL;
    }

    if (Scr_GetType(7)) {
        Scr_GetVector(7, vDir);
        dir = vDir;
        Vec3NormalizeTo(dir, localdir);
    } else {
        dir = NULL;
        localdir[0] = 0.0f;
        localdir[1] = 0.0f;
        localdir[2] = 0.0f;
    }

    hitLoc = G_GetHitLocationIndexFromString(Scr_GetConstString(8));
    psTimeOffset = Scr_GetInt(9);
    client = pSelf->client;

    if (!(pSelf->flags & 8) && !(dflags & 4)) {
        int knockback;
        float damageScale;

        if (client->ps.pm_flags & 1)
            damageScale = 0.02f;
        else if (client->ps.pm_flags & 2)
            damageScale = 0.15f;
        else
            damageScale = 0.3f;

        knockback = (int)((float)damage * damageScale);
        if (knockback > 60)
            knockback = 60;

        if (knockback && !(client->ps.eFlags & 0x300)) {
            float velocityScale = (float)knockback * g_knockback->current.value / 250.0f;

            client->ps.velocity[0] += localdir[0] * velocityScale;
            client->ps.velocity[1] += localdir[1] * velocityScale;
            client->ps.velocity[2] += localdir[2] * velocityScale;

            if (!client->ps.pm_time) {
                int pmTime = knockback * 2;

                if (pmTime <= 49)
                    pmTime = 50;
                else if (pmTime >= 201)
                    pmTime = 200;

                client->ps.pm_time = pmTime;
                client->ps.pm_flags |= 0x400;
            }
        }
    }

    if (pSelf->flags & 1)
        return;

    if (iWeapon) {
        WeaponDef *weapDef = BG_GetWeaponDef(iWeapon);

        if (!weapDef->weapType) {
            const vec_t *impactPoint = point ? point : pSelf->r.currentOrigin;
            int clientNum = client->ps.clientNum;
            int clientMaskIndex = clientNum >> 5;
            int clientMaskBit = 1 << (clientNum & 0x1f);
            int firstEvent = (weapDef->bRifleBullet < 1) ? 0xb6 : 0xb7;
            int secondEvent = (weapDef->bRifleBullet < 1) ? 0xb9 : 0xba;
            gentity_t *tent;

            tent = G_TempEntity(impactPoint, firstEvent);
            tent->s.eventParm = DirToByte(localdir);
            tent->s.scale = DirToByte(localdir);
            tent->s.surfType = 7;
            tent->s.otherEntityNum = attacker->s.number;
            tent->r.clientMask[clientMaskIndex] |= clientMaskBit;

            tent = G_TempEntity(impactPoint, secondEvent);
            tent->s.surfType = 7;
            tent->s.otherEntityNum = attacker->s.number;
            tent->s.attackerEntityNum = clientNum;
            tent->r.clientMask[0] = -1;
            tent->r.clientMask[1] = -1;
            tent->r.clientMask[clientMaskIndex] &= ~clientMaskBit;
        }
    }

    client->damage_blood += damage;
    if (dir) {
        client->damage_from[0] = localdir[0];
        client->damage_from[1] = localdir[1];
        client->damage_from[2] = localdir[2];
        client->damage_fromWorld = 0;
    } else {
        client->damage_from[0] = pSelf->r.currentOrigin[0];
        client->damage_from[1] = pSelf->r.currentOrigin[1];
        client->damage_from[2] = pSelf->r.currentOrigin[2];
        client->damage_fromWorld = 1;
    }

    if (pSelf->flags & 2 && pSelf->health - damage <= 0)
        damage = pSelf->health - 1;

    maxDamageTime = player_dmgtimer_maxTime->current.value;
    client->ps.damageTimer += (int)((float)damage * player_dmgtimer_timePerPoint->current.value);

    if (dir)
        client->ps.flinchYaw = (int)vectoyaw(dir);
    else
        client->ps.flinchYaw = 0;

    viewYaw = client->ps.viewangles[1];
    if (viewYaw < 0.0f)
        viewYaw += 360.0f;

    yaw = (float)client->ps.flinchYaw - viewYaw;
    client->ps.flinchYaw = (int)yaw;

    if ((float)client->ps.damageTimer > maxDamageTime)
        client->ps.damageTimer = (int)maxDamageTime;

    client->ps.damageDuration = client->ps.damageTimer;
    pSelf->health -= damage;

    Scr_AddEntity(attacker);
    Scr_AddInt(damage);
    Scr_Notify(pSelf, scr_const.damage, 2);

    health = pSelf->health;
    if (health > 0) {
        void (*pain)(gentity_t *, gentity_t *, int, const vec_t *, int, const vec_t *, hitLocation_t);

        pain = (void (*)(gentity_t *, gentity_t *, int, const vec_t *, int, const vec_t *, hitLocation_t))entityHandlers[pSelf->handler].pain;
        if (pain)
            pain(pSelf, attacker, damage, point, mod, localdir, hitLoc);

        client->ps.stats[0] = pSelf->health;
        return;
    }

    if (health < -999)
        pSelf->health = -999;

    {
        void (*die)(gentity_t *, gentity_t *, gentity_t *, int, int, int, const vec_t *, hitLocation_t, int);

        die = (void (*)(gentity_t *, gentity_t *, gentity_t *, int, int, int, const vec_t *, hitLocation_t, int))entityHandlers[pSelf->handler].die;
        if (die)
            die(pSelf, inflictor, attacker, damage, mod, iWeapon, localdir, hitLoc, psTimeOffset);
    }

    if (pSelf->r.inuse)
        pSelf->client->ps.stats[0] = pSelf->health;
}

void PlayerCmd_ClonePlayer(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    gclient_t *client;
    gentity_t *body;
    XAnimTree *tree;
    corpseInfo_t *corpseInfo;
    int deathAnimDuration;
    int corpseIndex;
    int levelTime;
    float maxVelocity;

    if (!pSelf)
        return;

    deathAnimDuration = Scr_GetInt(0);
    client = pSelf->client;
    levelTime = level.time;

    body = G_SpawnPlayerClone();
    body->s.clientNum = client->ps.clientNum;
    body->s.eFlags = (client->ps.eFlags & ~2) | (body->s.eFlags & 2) | 0xa0000;

    G_SetOrigin(body, client->ps.origin);
    G_SetAngle(body, pSelf->r.currentAngles);

    body->s.pos.trType = 5;
    body->s.pos.trTime = levelTime;
    body->s.pos.trDelta[0] = client->ps.velocity[0];
    body->s.pos.trDelta[1] = client->ps.velocity[1];
    body->s.pos.trDelta[2] = client->ps.velocity[2];
    body->s.eType = 2;
    body->physicsObject = 1;

    tree = DObjGetTree(Com_GetServerDObj(client->ps.clientNum));

    maxVelocity = g_clonePlayerMaxVelocity->current.value;
    if (body->s.pos.trDelta[0] > maxVelocity)
        body->s.pos.trDelta[0] = maxVelocity;
    if (body->s.pos.trDelta[1] > maxVelocity)
        body->s.pos.trDelta[1] = maxVelocity;

    body->corpse.deathAnimStartTime = levelTime;

    corpseIndex = G_GetFreePlayerCorpseIndex();
    corpseInfo = &g_scr_data.playerCorpseInfo[corpseIndex];
    corpseInfo->entnum = body->s.number;
    corpseInfo->time = levelTime;
    corpseInfo->falling = 1;
    corpseInfo->ci = level_bgs.clientinfo[client->ps.clientNum];
    corpseInfo->ci.pXAnimTree = (XAnimTree_s *)corpseInfo->tree;
    XAnimCloneAnimTree(tree, (XAnimTree *)corpseInfo->tree);

    body->s.groundEntityNum = 0x3ff;
    body->r.svFlags = 2;

    body->r.mins[0] = pSelf->r.mins[0];
    body->r.mins[1] = pSelf->r.mins[1];
    body->r.mins[2] = pSelf->r.mins[2];
    body->r.maxs[0] = pSelf->r.maxs[0];
    body->r.maxs[1] = pSelf->r.maxs[1];
    body->r.maxs[2] = pSelf->r.maxs[2];
    body->r.absmin[0] = pSelf->r.absmin[0];
    body->r.absmin[1] = pSelf->r.absmin[1];
    body->r.absmin[2] = pSelf->r.absmin[2];
    body->r.absmax[0] = pSelf->r.absmax[0];
    body->r.absmax[1] = pSelf->r.absmax[1];
    body->r.absmax[2] = pSelf->r.absmax[2];

    body->s.legsAnim = client->ps.legsAnim;
    body->s.torsoAnim = client->ps.torsoAnim;
    body->clipmask = 0x10001;
    body->r.contents = 0x4002000;
    SV_LinkEntity(body);

    body->nextthink = levelTime + deathAnimDuration;
    body->handler = 0xc;
    GScr_AddEntity(body);
}

void PlayerCmd_getCurrentWeapon(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    int weaponIndex;

    if (!pSelf)
        return;

    weaponIndex = pSelf->client->ps.weapon;
    if (pSelf->client->sess.sessionState == SESS_STATE_PLAYING && weaponIndex > 0)
        Scr_AddString(BG_GetWeaponDef(weaponIndex)->szInternalName);
    else
        Scr_AddString("none");
}

void PlayerCmd_getCurrentOffhand(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    int weaponIndex;

    if (!pSelf)
        return;

    weaponIndex = pSelf->client->ps.offHandIndex;
    if (pSelf->client->sess.sessionState == SESS_STATE_PLAYING && weaponIndex > 0)
        Scr_AddString(BG_GetWeaponDef(weaponIndex)->szInternalName);
    else
        Scr_AddString("none");
}

void PlayerCmd_GetWeaponSlotWeapon(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    const scr_const_t *scrConst = (const scr_const_t *)imp_scr_const;
    int slot;
    int weaponIndex;

    if (pSelf->client->sess.sessionState != SESS_STATE_PLAYING)
        { (void)(Scr_AddConstString(scrConst->none)); return; }

    slot = PlayerCmd_GetWeaponSlotParam(0);
    weaponIndex = (signed char)pSelf->client->ps.weaponslots[slot];
    if (!weaponIndex)
        { (void)(Scr_AddConstString(scrConst->none)); return; }

    { (void)(Scr_AddString(BG_GetWeaponDef(weaponIndex)->szInternalName)); return; }
}

void PlayerCmd_GetWeaponSlotAmmo(scr_entref_t entref)
{
    gentity_t *pSelf = PlayerCmd_GetPlayerEntity(entref);
    int slot;
    int weaponIndex;

    if (!pSelf)
        return;

    if (pSelf->client->sess.sessionState != SESS_STATE_PLAYING) {
        Scr_AddInt(0);
        return;
    }

    slot = PlayerCmd_GetWeaponSlotParam(0);
    weaponIndex = (signed char)pSelf->client->ps.weaponslots[slot];
    if (!weaponIndex) {
        Scr_AddInt(0);
        return;
    }

    if (BG_WeaponIsClipOnly(weaponIndex))
        Scr_AddInt(pSelf->client->ps.ammoclip[BG_ClipForWeapon(weaponIndex)]);
    else
        Scr_AddInt(pSelf->client->ps.ammo[BG_AmmoForWeapon(weaponIndex)]);
}

static const BuiltinMethodDef player_methods[] __attribute__((used)) = {
    { "giveweapon", (BuiltinMethod)PlayerCmd_giveWeapon, 0 },
    { "takeweapon", (BuiltinMethod)PlayerCmd_takeWeapon, 0 },
    { "takeallweapons", (BuiltinMethod)PlayerCmd_takeAllWeapons, 0 },
    { "getcurrentweapon", (BuiltinMethod)PlayerCmd_getCurrentWeapon, 0 },
    { "getcurrentoffhand", (BuiltinMethod)PlayerCmd_getCurrentOffhand, 0 },
    { "hasweapon", (BuiltinMethod)PlayerCmd_hasWeapon, 0 },
    { "switchtoweapon", (BuiltinMethod)PlayerCmd_switchToWeapon, 0 },
    { "switchtooffhand", (BuiltinMethod)PlayerCmd_switchToOffhand, 0 },
    { "givestartammo", (BuiltinMethod)PlayerCmd_giveStartAmmo, 0 },
    { "givemaxammo", (BuiltinMethod)PlayerCmd_giveMaxAmmo, 0 },
    { "getfractionstartammo", (BuiltinMethod)PlayerCmd_getFractionStartAmmo, 0 },
    { "getfractionmaxammo", (BuiltinMethod)PlayerCmd_getFractionMaxAmmo, 0 },
    { "setorigin", (BuiltinMethod)PlayerCmd_setOrigin, 0 },
    { "setplayerangles", (BuiltinMethod)PlayerCmd_setAngles, 0 },
    { "getplayerangles", (BuiltinMethod)PlayerCmd_getAngles, 0 },
    { "isbot", (BuiltinMethod)PlayerCmd_BotIsBot, 0 },
    { "botstop", (BuiltinMethod)PlayerCmd_BotStop, 0 },
    { "botmovement", (BuiltinMethod)PlayerCmd_BotMovement, 0 },
    { "botangles", (BuiltinMethod)PlayerCmd_BotAngles, 0 },
    { "botweapon", (BuiltinMethod)PlayerCmd_BotWeapon, 0 },
    { "botaction", (BuiltinMethod)PlayerCmd_BotAction, 0 },
    { "usebuttonpressed", (BuiltinMethod)PlayerCmd_useButtonPressed, 0 },
    { "attackbuttonpressed", (BuiltinMethod)PlayerCmd_attackButtonPressed, 0 },
    { "meleebuttonpressed", (BuiltinMethod)PlayerCmd_meleeButtonPressed, 0 },
    { "playerads", (BuiltinMethod)PlayerCmd_playerADS, 0 },
    { "isonground", (BuiltinMethod)PlayerCmd_isOnGround, 0 },
    { "pingplayer", (BuiltinMethod)PlayerCmd_pingPlayer, 0 },
    { "setviewmodel", (BuiltinMethod)PlayerCmd_SetViewmodel, 0 },
    { "getviewmodel", (BuiltinMethod)PlayerCmd_GetViewmodel, 0 },
    { "sayall", (BuiltinMethod)PlayerCmd_SayAll, 0 },
    { "sayteam", (BuiltinMethod)PlayerCmd_SayTeam, 0 },
    { "showscoreboard", (BuiltinMethod)PlayerCmd_showScoreboard, 0 },
    { "setspawnweapon", (BuiltinMethod)PlayerCmd_setSpawnWeapon, 0 },
    { "dropitem", (BuiltinMethod)PlayerCmd_dropItem, 0 },
    { "finishplayerdamage", (BuiltinMethod)PlayerCmd_finishPlayerDamage, 0 },
    { "suicide", (BuiltinMethod)PlayerCmd_Suicide, 0 },
    { "openmenu", (BuiltinMethod)PlayerCmd_OpenMenu, 0 },
    { "openmenunomouse", (BuiltinMethod)PlayerCmd_OpenMenuNoMouse, 0 },
    { "closemenu", (BuiltinMethod)PlayerCmd_CloseMenu, 0 },
    { "closeingamemenu", (BuiltinMethod)PlayerCmd_CloseInGameMenu, 0 },
    { "freezecontrols", (BuiltinMethod)PlayerCmd_FreezeControls, 0 },
    { "disableweapon", (BuiltinMethod)PlayerCmd_DisableWeapon, 0 },
    { "enableweapon", (BuiltinMethod)PlayerCmd_EnableWeapon, 0 },
    { "setreverb", (BuiltinMethod)PlayerCmd_SetReverb, 0 },
    { "deactivatereverb", (BuiltinMethod)PlayerCmd_DeactivateReverb, 0 },
    { "setchannelvolumes", (BuiltinMethod)PlayerCmd_SetChannelVolumes, 0 },
    { "deactivatechannelvolumes", (BuiltinMethod)PlayerCmd_DeactivateChannelVolumes, 0 },
    { "getweaponslotweapon", (BuiltinMethod)PlayerCmd_GetWeaponSlotWeapon, 0 },
    { "setweaponslotweapon", (BuiltinMethod)PlayerCmd_SetWeaponSlotWeapon, 0 },
    { "getweaponslotammo", (BuiltinMethod)PlayerCmd_GetWeaponSlotAmmo, 0 },
    { "setweaponslotammo", (BuiltinMethod)PlayerCmd_SetWeaponSlotAmmo, 0 },
    { "getweaponslotclipammo", (BuiltinMethod)PlayerCmd_GetWeaponSlotClipAmmo, 0 },
    { "setweaponslotclipammo", (BuiltinMethod)PlayerCmd_SetWeaponSlotClipAmmo, 0 },
    { "setweaponclipammo", (BuiltinMethod)PlayerCmd_SetWeaponClipAmmo, 0 },
    { "iprintln", (BuiltinMethod)iclientprintln, 0 },
    { "iprintlnbold", (BuiltinMethod)iclientprintlnbold, 0 },
    { "spawn", (BuiltinMethod)PlayerCmd_spawn, 0 },
    { "setentertime", (BuiltinMethod)PlayerCmd_setEnterTime, 0 },
    { "cloneplayer", (BuiltinMethod)PlayerCmd_ClonePlayer, 0 },
    { "setclientcvar", (BuiltinMethod)PlayerCmd_SetClientDvar, 0 },
    { "islookingat", (BuiltinMethod)ScrCmd_IsLookingAt, 0 },
    { "playlocalsound", (BuiltinMethod)ScrCmd_PlayLocalSound, 0 },
    { "istalking", (BuiltinMethod)PlayerCmd_IsTalking, 0 },
    { "allowspectateteam", (BuiltinMethod)PlayerCmd_AllowSpectateTeam, 0 },
    { "getguid", (BuiltinMethod)PlayerCmd_GetGuid, 0 },
    { 0, 0, 0 },
};

BuiltinMethod Player_GetMethod(const char **pName)
{
    const BuiltinMethodDef *method;
    const char *name;

    name = *pName;
    for (method = player_methods; method->actionString; method++) {
        if (!strcmp(name, method->actionString)) {
            *pName = method->actionString;
            return method->actionFunc;
        }
    }

    return 0;
}
