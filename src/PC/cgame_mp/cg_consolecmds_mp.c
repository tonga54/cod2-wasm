#include "cod2_feature_config.h"
#include "common_types.h"
#include "imports.h"
#include "headers/PC/cgame_mp/cg_local.h"
#include <stdlib.h>

extern dvar_t *cg_viewsize;

extern int CG_CrosshairPlayer(void);
extern void Cmd_ArgvBuffer(int arg, char *buf, int bufSize);
extern const char *va(const char *fmt, ...);
extern void Cbuf_AddText(const char *text);
extern void Dvar_SetInt(void *dvar, int value);
extern void Com_Printf(const char *fmt, ...);
extern int CG_ScoreboardDisplayed(void);
extern int Cmd_Argc(void);
extern void CG_SetShellShockParmsFromDvars(byte *parms);
extern int CG_LoadShellShockDvars(const char *name);
extern qboolean CG_SaveShellShockDvars(const char *name);
extern float floorf(float x);
extern void Cmd_ArgsBuffer(char *buf, int bufSize);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern void CL_AddReliableCommand(const char *cmd);
extern const char *CG_Argv(int arg);
extern int I_stricmp(const char *s1, const char *s2);
extern void CL_AddCgameCommand(const char *cmdName);
extern qboolean CL_Popup(const char *name);
extern const char *UI_SafeTranslateString(const char *key);
extern void *Com_GetClientDObj(int clientNum, int localClientNum);
extern void CG_TraceCapsule(trace_t *result, const vec_t *start, const vec_t *mins,
                            const vec_t *maxs, const vec_t *end, int skipNumber, int mask);

void CG_TargetCommand_f(void);
static void CG_SizeUp_f(void);
static void CG_SizeDown_f(void);
static void CG_Viewpos_f(void);
void CG_ScoresUp_f(void);
void CG_ScoresDown_f(void);
void CG_PrevWeapon_f(void);
void CG_NextWeapon_f(void);
void CG_WeaponSlot_f(qboolean next, qboolean ignoreEmpty);
void CG_FxSetTestPosition(void);
void CG_FxTest(void);
void CG_FxRestart(void);
static void CG_ShellShock_f(void);
static void CG_ShellShock_Load_f(void);
static void CG_ShellShock_Save_f(void);
static void CG_TellTarget_f(void);
static void CG_QuickMessage_f(void);
static void CG_VoiceChat_f(void);
static void CG_TeamVoiceChat_f(void);
qboolean CG_ConsoleCommand(void);
qboolean CG_IsConsoleCommandName(const char *cmd);
void CG_InitConsoleCommands(void);

static const consoleCommand_t commandsList[] = {
    { "tcmd", CG_TargetCommand_f },
    { "sizeup", CG_SizeUp_f },
    { "sizedown", CG_SizeDown_f },
    { "viewpos", CG_Viewpos_f },
    { "+scores", CG_ScoresDown_f },
    { "-scores", CG_ScoresUp_f },
    { "weapprev", CG_PrevWeapon_f },
    { "weapnext", CG_NextWeapon_f },
    { "weaponslot", (void (*)(void))CG_WeaponSlot_f },
    { "fxSetTestPosition", CG_FxSetTestPosition },
    { "fxTest", CG_FxTest },
    { "fxRestart", CG_FxRestart },
    { "cg_shellshock", CG_ShellShock_f },
    { "cg_shellshock_load", CG_ShellShock_Load_f },
    { "cg_shellshock_save", CG_ShellShock_Save_f },
    { "tell_target", CG_TellTarget_f },
    { "quickmessage", CG_QuickMessage_f },
    { "VoiceChat", CG_VoiceChat_f },
    { "VoiceTeamChat", CG_TeamVoiceChat_f },
    { NULL, NULL }
};

void CG_TargetCommand_f(void)
{
    int targetNum;
    char test[4];

    targetNum = CG_CrosshairPlayer();
    if (targetNum == 0)
        return;

    Cmd_ArgvBuffer(1, test, 4);
    Cbuf_AddText(va("gc %i %i", targetNum, atoi(test)));
}

static void CG_SizeUp_f(void)
{
    dvar_t *dvar = cg_viewsize;
    int val = dvar->current.integer;

    Dvar_SetInt(dvar, val + 10);
}

static void CG_SizeDown_f(void)
{
    dvar_t *dvar = cg_viewsize;
    int val = dvar->current.integer;

    Dvar_SetInt(dvar, val - 10);
}

static void CG_Viewpos_f(void)
{

    Com_Printf("(%i %i %i) : %i\n",
               (int)cg->refdef.vieworg[0],
               (int)cg->refdef.vieworg[1],
               (int)cg->refdef.vieworg[2],
               (int)cg->refdefViewAngles[1]);
    if (getenv("PTRACE")) {
        extern void R_DebugStaticModels(const float *view);
        const playerState_t *predicted = &cg->predictedPlayerState;
        R_DebugStaticModels(predicted->origin);
        Com_Printf("[viewpos] predicted client=%d origin=(%.1f %.1f %.1f) angles=(%.1f %.1f %.1f) health=%d\n",
                   predicted->clientNum, predicted->origin[0], predicted->origin[1], predicted->origin[2],
                   predicted->viewangles[0], predicted->viewangles[1], predicted->viewangles[2], predicted->stats[0]);
        Com_Printf("[viewpos] type=%d flags=%x ground=%d height=%.1f thirdperson=%d viewlocked=%d\n",
                   predicted->pm_type, predicted->pm_flags, predicted->groundEntityNum,
                   predicted->viewHeightCurrent, cg->renderingThirdPerson, predicted->viewlocked_entNum);
        Com_Printf("[viewpos] velocity=(%.1f %.1f %.1f)\n", predicted->velocity[0],
                   predicted->velocity[1], predicted->velocity[2]);
        for (int axis = 0; axis < 4; ++axis) {
            vec3_t end = {predicted->origin[0], predicted->origin[1], predicted->origin[2]};
            trace_t trace;
            end[axis / 2] += (axis & 1) ? -64.0f : 64.0f;
            CG_TraceCapsule(&trace, predicted->origin, predicted->mins, predicted->maxs,
                            end, predicted->clientNum, 0x2810011);
            Com_Printf("[viewpos] trace=%d frac=%.3f allsolid=%d startsolid=%d ent=%d normal=(%.2f %.2f %.2f) surface=%x contents=%x\n",
                       axis, trace.fraction, trace.allsolid, trace.startsolid, trace.entityNum,
                       trace.normal[0], trace.normal[1], trace.normal[2], trace.surfaceFlags, trace.contents);
        }
        if (cg->snap) {
            const playerState_t *server = &cg->snap->ps;
            Com_Printf("[viewpos] server client=%d origin=(%.1f %.1f %.1f) angles=(%.1f %.1f %.1f) health=%d\n",
                       server->clientNum, server->origin[0], server->origin[1], server->origin[2],
                       server->viewangles[0], server->viewangles[1], server->viewangles[2], server->stats[0]);
        }
        if (cg->nextSnap) {
            Com_Printf("[viewpos] killcam=%d delta=%d type=%d scores=%d local=%d viewed=%d\n",
                       cg->inKillCam, cg->nextSnap->ps.deltaTime, cg->nextSnap->ps.pm_type,
                       cg->numScores, cg->clientNum, cg->nextSnap->ps.clientNum);
            Com_Printf("[viewpos] entities=%d clients=%d\n", cg->nextSnap->numEntities, cg->nextSnap->numClients);
            Com_Printf("[viewpos] time=%d snap=%d next=%d fraction=%.3f\n", cg->time,
                       cg->snap ? cg->snap->serverTime : -1, cg->nextSnap->serverTime,
                       cg->frameInterpolation);
            for (int i = 0; i < cg->nextSnap->numEntities; ++i) {
                const entityState_t *entity = &cg->nextSnap->entities[i];
                if (entity->eType != 1 || entity->clientNum < 0 || entity->clientNum >= 64) continue;
                const centity_t *cent = &cg_entities[entity->number];
                const clientInfo_t *info = &cg->bgs.clientinfo[entity->clientNum];
                Com_Printf("[viewpos] player entity=%d client=%d flags=%x origin=(%.1f %.1f %.1f) lerp=(%.1f %.1f %.1f) valid=%d model='%s' dobj=%p\n",
                           entity->number, entity->clientNum, entity->eFlags,
                           entity->pos.trBase[0], entity->pos.trBase[1], entity->pos.trBase[2],
                           cent->lerpOrigin[0], cent->lerpOrigin[1], cent->lerpOrigin[2],
                           info->infoValid, info->model, Com_GetClientDObj(entity->number, 0));
                Com_Printf("[viewpos] trajectory player=%d current=(%d %d %d %.1f %.1f %.1f) next=(%d %d %d %.1f %.1f %.1f)\n",
                           entity->number,
                           cent->currentState.pos.trType, cent->currentState.pos.trTime, cent->currentState.pos.trDuration,
                           cent->currentState.pos.trBase[0], cent->currentState.pos.trBase[1], cent->currentState.pos.trBase[2],
                           cent->nextState.pos.trType, cent->nextState.pos.trTime, cent->nextState.pos.trDuration,
                           cent->nextState.pos.trBase[0], cent->nextState.pos.trBase[1], cent->nextState.pos.trBase[2]);
                Com_Printf("[viewpos] delta player=%d current=(%.1f %.1f %.1f) next=(%.1f %.1f %.1f)\n",
                           entity->number, cent->currentState.pos.trDelta[0], cent->currentState.pos.trDelta[1],
                           cent->currentState.pos.trDelta[2], cent->nextState.pos.trDelta[0],
                           cent->nextState.pos.trDelta[1], cent->nextState.pos.trDelta[2]);
                {
                    DObj *obj = Com_GetClientDObj(entity->number, 0);
                    extern void DObjDisplayAnim(DObj *obj);
                    extern const char *SL_ConvertToString(unsigned int stringValue);
                    if (obj) {
                        Com_Printf("[viewpos] anim player=%d legs=%d torso=%d\n", entity->number,
                                   cent->nextState.legsAnim, cent->nextState.torsoAnim);
                        DObjDisplayAnim(obj);
                        if (obj->skel && obj->numModels && obj->models[0]->parts) {
                            const XModelParts *parts = obj->models[0]->parts;
                            for (int b = 0; b < parts->numBones && b < 12; ++b) {
                                const DObjAnimMat *mat = &obj->skel->mat[b];
                                Com_Printf("[viewpos] bone=%d %s pos=(%.2f %.2f %.2f) quat=(%.2f %.2f %.2f %.2f) weight=%.3f\n",
                                           b, parts->hierarchy ? SL_ConvertToString(parts->hierarchy->names[b]) : "?",
                                           mat->trans[0], mat->trans[1], mat->trans[2], mat->quat[0], mat->quat[1],
                                           mat->quat[2], mat->quat[3], mat->transWeight);
                            }
                        }
                    }
                }
#ifdef __EMSCRIPTEN__
                extern GfxScene scene;
                for (int s = 0; s < scene.def.entityCount; ++s) {
                    const GfxSceneEntity *rendered = &scene.sceneEnts[s];
                    if (rendered->cent != cent) continue;
                    Com_Printf("[viewpos] scene player=%d cull=%d surfaces=%d min=(%.1f %.1f %.1f) max=(%.1f %.1f %.1f)\n",
                               entity->number, rendered->cullState, rendered->surfCount,
                               rendered->curMins[0], rendered->curMins[1], rendered->curMins[2],
                               rendered->curMaxs[0], rendered->curMaxs[1], rendered->curMaxs[2]);
                }
#endif
            }
        }
    }
}

void CG_ScoresUp_f(void)
{

    if (!CG_ScoreboardDisplayed())
        return;

    cg->showScores = 0;
    cg->scoreFadeTime = cg->time;
}

void CG_ScoresDown_f(void)
{
    int currentTime = cg->time;
    int lastScoreTime = cg->scoresRequestTime;

    if (lastScoreTime + 2000 < currentTime) {
        cg->scoresRequestTime = currentTime;
        CL_AddReliableCommand("score");

        if (!CG_ScoreboardDisplayed()) {
            cg->numScores = 0;
            cg->scoresTop = 0;
            cg->showScores = 1;
        }
        return;
    }

    cg->showScores = 1;
}

static void CG_ShellShock_f(void)
{
    char arg[256];
    int argc;
    double duration;

    argc = Cmd_Argc();

    switch (argc) {
    case 2:
        break;
    case 3:
        Cmd_ArgvBuffer(2, arg, 256);
        if (!CG_LoadShellShockDvars(arg))
            return;
        break;
    default:
        Com_Printf("USAGE: cg_shellshock <duration> <filename?>\n");
        return;
    }

    Cmd_ArgvBuffer(1, arg, 256);
    duration = atof(arg);

    CG_SetShellShockParmsFromDvars((byte *)cgs->shellshockParms);

    cg->testShock.time = cg->time;
    cg->testShock.duration = (int)floorf((float)duration * 1000.0f + 0.5f);
}

static void CG_ShellShock_Load_f(void)
{
    char name[64];

    if (Cmd_Argc() != 2) {
        Com_Printf("USAGE: cg_shellshock_load <name>\n");
        return;
    }

    Cmd_ArgvBuffer(1, name, 64);
    CG_LoadShellShockDvars(name);
}

static void CG_ShellShock_Save_f(void)
{
    char name[64];

    if (Cmd_Argc() != 2) {
        Com_Printf("USAGE: cg_shellshock_save <name>\n");
        return;
    }

    Cmd_ArgvBuffer(1, name, 64);
    CG_SaveShellShockDvars(name);
}

static void CG_TellTarget_f(void)
{
    int clientNum;
    char command[128];
    char message[128];

    clientNum = CG_CrosshairPlayer();
    if (clientNum == -1)
        return;

    Cmd_ArgsBuffer(message, 128);
    Com_sprintf(command, 128, "tell %i \"%s\"", clientNum, message);
    CL_AddReliableCommand(command);
}

static void CG_QuickMessage_f(void)
{
    snapshot_t *nextSnap = cg->nextSnap;

    if (nextSnap == NULL)
        return;
    if (!(nextSnap->ps.pm_flags & 0x800000))
        return;

    CL_Popup("UIMENU_WM_QUICKMESSAGE");
}

static void CG_VoiceChat_f(void)
{
    char chatCmd[64];
    snapshot_t *nextSnap;

    if (Cmd_Argc() != 2)
        return;

    nextSnap = cg->nextSnap;

    if (nextSnap != NULL && nextSnap->ps.pm_type != 5 && !(nextSnap->ps.pm_flags & 0x800000)) {
        Com_Printf("%s\n", UI_SafeTranslateString("CGAME_NOSPECTATORVOICECHAT"));
        return;
    }

    Cmd_ArgvBuffer(1, chatCmd, 64);
    Cbuf_AddText(va("cmd vsay %s\n", chatCmd));
}

static void CG_TeamVoiceChat_f(void)
{
    char chatCmd[64];
    snapshot_t *nextSnap;

    if (Cmd_Argc() != 2)
        return;

    nextSnap = cg->nextSnap;

    if (nextSnap != NULL && nextSnap->ps.pm_type != 5 && !(nextSnap->ps.pm_flags & 0x800000)) {
        Com_Printf("%s\n", UI_SafeTranslateString("CGAME_NOSPECTATORVOICECHAT"));
        return;
    }

    Cmd_ArgvBuffer(1, chatCmd, 64);
    Cbuf_AddText(va("cmd vsay_team %s\n", chatCmd));
}

qboolean CG_IsConsoleCommandName(const char *cmd)
{
    byte *cmdList = (byte *)commandsList;
    const char *name;

    if (!cmd)
        return 0;

    name = *(const char **)cmdList;
    while (name != NULL) {
        if (I_stricmp(cmd, name) == 0)
            return 1;
        cmdList += 8;
        name = *(const char **)cmdList;
    }

    return 0;
}

qboolean CG_ConsoleCommand(void)
{
    const char *cmd;
    const consoleCommand_t *cmd_p;
    const char *name;
    int i;

    if (cg->nextSnap == NULL)
        return 0;

    cmd = CG_Argv(0);
    name = commandsList[0].cmd;
    if (name == NULL)
        return 0;

    i = 0;
    cmd_p = commandsList;
    for (;;) {
        if (I_stricmp(cmd, name) == 0) {
            if (commandsList[i].function != NULL)
                commandsList[i].function();
            return 1;
        }
        i++;
        cmd_p++;
        name = cmd_p->cmd;
        if (name == NULL)
            return 0;
    }
}

void CG_InitConsoleCommands(void)
{
    byte *cmdList = (byte *)commandsList;
    const char *name;

    name = *(const char **)cmdList;
    while (name != NULL) {
        CL_AddCgameCommand(name);
        cmdList += 8;
        name = *(const char **)cmdList;
    }

    CL_AddCgameCommand("kill");
    CL_AddCgameCommand("give");
    CL_AddCgameCommand("take");
    CL_AddCgameCommand("god");
    CL_AddCgameCommand("demigod");
    CL_AddCgameCommand("notarget");
    CL_AddCgameCommand("noclip");
    CL_AddCgameCommand("ufo");
    CL_AddCgameCommand("levelshot");
    CL_AddCgameCommand("setviewpos");
    CL_AddCgameCommand("jumptonode");
    CL_AddCgameCommand("stats");
    CL_AddCgameCommand("say");
    CL_AddCgameCommand("say_team");
    CL_AddCgameCommand("tell");
    CL_AddCgameCommand("team");
    CL_AddCgameCommand("follow");
    CL_AddCgameCommand("callvote");
    CL_AddCgameCommand("vote");
    CL_AddCgameCommand("follownext");
    CL_AddCgameCommand("followprev");
    CL_AddCgameCommand("printentities");
    CL_AddCgameCommand("muteplayer");
    CL_AddCgameCommand("unmuteplayer");

#if COD2_FEATURE_RUMBLE

    {
        extern void CG_InitRumble(void);
        CG_InitRumble();
    }
#endif
}
