#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include "cod2_feature_config.h"
#include <math.h>

#if COD2_FEATURE_GAMEPAD
extern void CL_Gamepad_Init(void);
extern void CL_Gamepad_Shutdown(void);
extern void CL_Gamepad_Move(usercmd_t *cmd);
extern void CL_Gamepad_NoteKbmActive(void);
#endif

extern const dvar_t *cl_yawspeed;
extern const dvar_t *cl_pitchspeed;
extern const dvar_t *cl_anglespeedkey;
extern const dvar_t *cl_talking;
extern const dvar_t *cl_bypassMouseInput;
extern const dvar_t *cl_analog_attack_threshold;
extern const dvar_t *cl_stanceHoldTime;
extern const dvar_t *cl_maxpackets;
extern const dvar_t *cl_freelook;
extern const dvar_t *cl_showSend;
extern const dvar_t *cl_sensitivity;
extern const dvar_t *cl_mouseAccel;
extern const dvar_t *cl_showMouseRate;
extern const dvar_t *m_pitch;
extern const dvar_t *m_yaw;
extern const dvar_t *m_forward;
extern const dvar_t *m_side;
extern const dvar_t *m_filter;
extern int atoi(const char *nptr);
extern const char *Cmd_Argv(int arg);
extern void Com_Printf(const char *fmt, ...);
extern void Cmd_AddCommand(const char *cmd, void (*func)(void));
extern void Cmd_RemoveCommand(const char *cmd);
extern const dvar_t *Dvar_RegisterBool_mac(const char *name, int value, int flags);
extern const dvar_t *Dvar_RegisterFloat(const char *name, float value, float min, float max, int flags);
extern const dvar_t *Dvar_RegisterInt(const char *name, int value, int min, int max, int flags);
extern void MSG_Init(msg_t *buf, byte *data, int length);
extern void MSG_WriteString(msg_t *msg, const char *s);
extern void MSG_WriteShort(msg_t *msg, int c);
extern void MSG_WriteByte(msg_t *msg, int c);
extern void MSG_WriteData(msg_t *buf, const void *data, int length);
extern void MSG_WriteLong(msg_t *msg, int c);
extern void MSG_WriteBits(msg_t *msg, int value, int bits);
extern int MSG_WriteBitsCompress(const byte *datasrc, byte *buffdest, int bytecount);
extern void MSG_SetDefaultUserCmd(void *from, void *to);
extern void MSG_WriteDeltaUsercmdKey(msg_t *msg, int key, void *from, void *to);
extern int Com_HashKey(const char *string, int maxlen);
extern void CL_Netchan_Transmit(void *chan, byte *data, int length);
extern void CL_Netchan_TransmitNextFragment(void *chan);
extern void NET_OutOfBandVoiceData(netsrc_t sock, netadr_t adr, byte *format, int len);
extern void CL_SyncGpu(void);
extern void CL_SendCmdInternal(void);
extern Bool PM_IsBinocularsADS(const playerState_t *ps);
extern const signed char ClampChar(const int i);
extern float sqrtf(float x);
extern qboolean Sys_IsLANAddress(netadr_t adr);
extern struct clientStatic_t cls;
extern int com_frameTime;
extern unsigned int frame_msec;

void CL_WritePacketDbg(const char *fmt, int serverId)
{
    (void)fmt;
    (void)serverId;
}
extern void UI_MouseEvent(int dx, int dy);
extern void UI_MouseEventAbsolute(int x, int y);
static kbutton_t playersKb[1][28];
__attribute__((used, packed, aligned(4)))
UInt32 kb[8] = {
    (UInt32)playersKb,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

#define kb ((kbutton_t *)kb[0])

typedef struct
{
    const char *name;
    void (*handler)(void);
} inputCommandDef_t;

void IN_MLookDown(void);
void IN_CenterView(void);
void IN_KeyDown(kbutton_t *b);
void IN_KeyUp(kbutton_t *b);
void IN_UpDown(void);
void IN_UpUp(void);
void IN_LeftDown(void);
void IN_LeftUp(void);
void IN_RightDown(void);
void IN_RightUp(void);
void IN_ForwardDown(void);
void IN_ForwardUp(void);
void IN_BackDown(void);
void IN_BackUp(void);
void IN_LookupDown(void);
void IN_LookupUp(void);
void IN_LookdownDown(void);
void IN_LookdownUp(void);
void IN_MoveleftDown(void);
void IN_MoveleftUp(void);
void IN_MoverightDown(void);
void IN_MoverightUp(void);
void IN_SpeedDown(void);
void IN_SpeedUp(void);
void IN_StrafeDown(void);
void IN_StrafeUp(void);
void IN_Attack_Down(void);
void IN_Attack_Up(void);
void IN_Breath_Down(void);
void IN_Breath_Up(void);
void IN_MeleeBreath_Down(void);
void IN_MeleeBreath_Up(void);
void IN_Frag_Down(void);
void IN_Frag_Up(void);
void IN_Smoke_Down(void);
void IN_Smoke_Up(void);
void IN_Binoculars_Down(void);
void IN_Binoculars_Up(void);
void IN_BreathBinoculars_Down(void);
void IN_BreathBinoculars_Up(void);
void IN_Melee_Down(void);
void IN_Melee_Up(void);
void IN_Activate_Down(void);
void IN_Activate_Up(void);
void IN_Reload_Down(void);
void IN_Reload_Up(void);
void IN_UseReload_Down(void);
void IN_UseReload_Up(void);
void IN_LeanLeft_Down(void);
void IN_LeanLeft_Up(void);
void IN_LeanRight_Down(void);
void IN_LeanRight_Up(void);
void IN_Stance_Down(void);
void IN_Stance_Up(void);
void IN_ToggleADS(void);
void IN_LeaveADS(void);
void IN_LowerStance(void);
void IN_RaiseStance(void);
void IN_ToggleCrouch(void);
void IN_ToggleProne(void);
void IN_GoProne(void);
void IN_GoCrouch(void);
void IN_GoStandDown(void);
void IN_GoStandUp(void);
void IN_TalkDown(void);
void IN_TalkUp(void);
Bool IsTalking(void);
void CL_MouseEvent(const int dx, const int dy);
void CL_WriteVoicePacket(void);
void CL_WritePacket(void);
void CL_InitInput(void);
void CL_ShutdownInput(void);
void CL_CmdButtons(usercmd_t *cmd);
void CL_MouseMove(usercmd_t *cmd);
void IN_DownDown(void);
void IN_DownUp(void);
void IN_Prone_Down(void);
void IN_Prone_Up(void);
void IN_MLookUp(void);
void CL_AdjustAngles(void);
void CL_KeyMove(usercmd_t *cmd);
usercmd_t CL_CreateCmd(void);
void CL_SendCmdInternal(void);

static const inputCommandDef_t s_inputCommands[] = {
    { "centerview", IN_CenterView },
    { "+moveup", IN_UpDown },
    { "-moveup", IN_UpUp },
    { "+movedown", IN_DownDown },
    { "-movedown", IN_DownUp },
    { "+left", IN_LeftDown },
    { "-left", IN_LeftUp },
    { "+right", IN_RightDown },
    { "-right", IN_RightUp },
    { "+forward", IN_ForwardDown },
    { "-forward", IN_ForwardUp },
    { "+back", IN_BackDown },
    { "-back", IN_BackUp },
    { "+lookup", IN_LookupDown },
    { "-lookup", IN_LookupUp },
    { "+lookdown", IN_LookdownDown },
    { "-lookdown", IN_LookdownUp },
    { "+strafe", IN_StrafeDown },
    { "-strafe", IN_StrafeUp },
    { "+moveleft", IN_MoveleftDown },
    { "-moveleft", IN_MoveleftUp },
    { "+moveright", IN_MoverightDown },
    { "-moveright", IN_MoverightUp },
    { "+speed", IN_SpeedDown },
    { "-speed", IN_SpeedUp },
    { "+attack", IN_Attack_Down },
    { "-attack", IN_Attack_Up },
    { "+melee", IN_Melee_Down },
    { "-melee", IN_Melee_Up },
    { "+holdbreath", IN_Breath_Down },
    { "-holdbreath", IN_Breath_Up },
    { "+melee_breath", IN_MeleeBreath_Down },
    { "-melee_breath", IN_MeleeBreath_Up },
    { "+frag", IN_Frag_Down },
    { "-frag", IN_Frag_Up },
    { "+smoke", IN_Smoke_Down },
    { "-smoke", IN_Smoke_Up },
    { "+binoculars", IN_Binoculars_Down },
    { "-binoculars", IN_Binoculars_Up },
    { "+breath_binoculars", IN_BreathBinoculars_Down },
    { "-breath_binoculars", IN_BreathBinoculars_Up },
    { "+activate", IN_Activate_Down },
    { "-activate", IN_Activate_Up },
    { "+reload", IN_Reload_Down },
    { "-reload", IN_Reload_Up },
    { "+usereload", IN_UseReload_Down },
    { "-usereload", IN_UseReload_Up },
    { "+leanleft", IN_LeanLeft_Down },
    { "-leanleft", IN_LeanLeft_Up },
    { "+leanright", IN_LeanRight_Down },
    { "-leanright", IN_LeanRight_Up },
    { "+prone", IN_Prone_Down },
    { "-prone", IN_Prone_Up },
    { "+stance", IN_Stance_Down },
    { "-stance", IN_Stance_Up },
    { "+mlook", IN_MLookDown },
    { "-mlook", IN_MLookUp },
    { "toggleads", IN_ToggleADS },
    { "leaveads", IN_LeaveADS },
    { "lowerstance", IN_LowerStance },
    { "raisestance", IN_RaiseStance },
    { "togglecrouch", IN_ToggleCrouch },
    { "toggleprone", IN_ToggleProne },
    { "goprone", IN_GoProne },
    { "gocrouch", IN_GoCrouch },
    { "+gostand", IN_GoStandDown },
    { "-gostand", IN_GoStandUp },
    { "+talk", IN_TalkDown },
    { "-talk", IN_TalkUp },
};
void CL_Input(void);
void CL_SendCmd(void);

static inline __attribute__((always_inline)) qboolean CL_ConsumeButtonPress(kbutton_t *button)
{

    if (*(const unsigned short *)&button->active) {
        button->wasPressed = 0;
        return 1;
    }
    return 0;
}

static inline __attribute__((always_inline)) float CL_KeyState(kbutton_t *key)
{
    unsigned int msec;

    msec = key->msec;
    key->msec = 0;

    if (key->active) {
        if (key->downtime) {
            msec += com_frameTime - key->downtime;
        } else {
            msec += com_frameTime;
        }

        key->downtime = com_frameTime;
    }

    if ((int)msec <= 0) {
        return 0.0f;
    }

    if (msec >= frame_msec) {
        return 1.0f;
    }

    return (float)msec / (float)frame_msec;
}

static inline __attribute__((always_inline)) int CL_KeyMoveValue(kbutton_t *key)
{
    return (int)(CL_KeyState(key) * 127.0f);
}

static float CL_ClampMouseAxisDelta(float delta, float maxSpeed)
{
    float maxDelta;

    if (!isfinite(delta)) {
        return 0.0f;
    }

    if (!isfinite(maxSpeed) || maxSpeed == 0.0f) {
        return delta;
    }

    maxDelta = (float)frame_msec * maxSpeed * 0.0010000000474974513f;
    if (delta > maxDelta) {
        return maxDelta;
    }

    if (delta < -maxDelta) {
        return -maxDelta;
    }

    return delta;
}

static float CL_FiniteOrDefault(float value, float defaultValue)
{
    return isfinite(value) ? value : defaultValue;
}

void IN_MLookDown(void)
{
    kb[13].active = 1;
}

void IN_CenterView(void)
{
    clientActive_t *cl = *(clientActive_t **)imp_cl;
    cl->viewangles[0] = (float)cl->snap.ps.delta_angles[0] * -0.0054931640625f;
}

void IN_KeyDown(kbutton_t *b)
{
    const char *keyText;
    int key;

    keyText = Cmd_Argv(1);
    key = keyText[0] ? atoi(keyText) : -1;

    if (key == b->down[0] || key == b->down[1]) {
        return;
    }

    if (!b->down[0]) {
        b->down[0] = key;
    } else if (!b->down[1]) {
        b->down[1] = key;
    } else {
        Com_Printf("Three keys down for a button!\n");
        return;
    }

    if (b->active) {
        return;
    }

    b->downtime = atoi(Cmd_Argv(2));
    b->active = 1;
    b->wasPressed = 1;
}

void IN_KeyUp(kbutton_t *b)
{
    const char *keyText;
    int key;
    int uptime;

    keyText = Cmd_Argv(1);
    if (!keyText[0]) {
        b->down[1] = 0;
        b->down[0] = 0;
        b->active = 0;
        return;
    }

    key = atoi(keyText);
    if (key == b->down[0]) {
        b->down[0] = 0;
    } else if (key == b->down[1]) {
        b->down[1] = 0;
    } else {
        return;
    }

    if (b->down[0] || b->down[1]) {
        return;
    }

    b->active = 0;
    uptime = atoi(Cmd_Argv(2));
    if (uptime) {
        b->msec += uptime - b->downtime;
    } else {
        b->msec += *(unsigned int *)imp_frame_msec / 2;
    }

    b->active = 0;
}

void IN_UpDown(void)
{
    int *stance;

    IN_KeyDown(&kb[12]);
    if (kb[25].active || kb[11].active ) {
        return;
    }

    stance = &(*(LegacyHacks **)imp_legacyHacks)->cl_stance;
    if (*stance > 1) {
        *stance = 1;
        return;
    }

    if (*stance == 1) {
        *stance = 0;
        return;
    }

    IN_KeyDown(&kb[10]);
}

void IN_UpUp(void)
{
    IN_KeyUp(&kb[12]);
    IN_KeyUp(&kb[10]);
}

void IN_LeftDown(void)
{
    IN_KeyDown(kb);
}

void IN_LeftUp(void)
{
    IN_KeyUp(kb);
}

void IN_RightDown(void)
{
    IN_KeyDown(&kb[1]);
}

void IN_RightUp(void)
{
    IN_KeyUp(&kb[1]);
}

void IN_ForwardDown(void)
{
    IN_KeyDown(&kb[2]);
}

void IN_ForwardUp(void)
{
    IN_KeyUp(&kb[2]);
}

void IN_BackDown(void)
{
    IN_KeyDown(&kb[3]);
}

void IN_BackUp(void)
{
    IN_KeyUp(&kb[3]);
}

void IN_LookupDown(void)
{
    IN_KeyDown(&kb[4]);
}

void IN_LookupUp(void)
{
    IN_KeyUp(&kb[4]);
}

void IN_LookdownDown(void)
{
    IN_KeyDown(&kb[5]);
}

void IN_LookdownUp(void)
{
    IN_KeyUp(&kb[5]);
}

void IN_MoveleftDown(void)
{
    IN_KeyDown(&kb[6]);
}

void IN_MoveleftUp(void)
{
    IN_KeyUp(&kb[6]);
}

void IN_MoverightDown(void)
{
    IN_KeyDown(&kb[7]);
}

void IN_MoverightUp(void)
{
    IN_KeyUp(&kb[7]);
}

void IN_SpeedDown(void)
{
    IN_KeyDown(&kb[9]);
}

void IN_SpeedUp(void)
{
    IN_KeyUp(&kb[9]);
}

void IN_StrafeDown(void)
{
    IN_KeyDown(&kb[8]);
}

void IN_StrafeUp(void)
{
    IN_KeyUp(&kb[8]);
}

void IN_Attack_Down(void)
{
    IN_KeyDown(&kb[14]);
}

void IN_Attack_Up(void)
{
    IN_KeyUp(&kb[14]);
}

void IN_Breath_Down(void)
{
    IN_KeyDown(&kb[15]);
}

void IN_Breath_Up(void)
{
    IN_KeyUp(&kb[15]);
}

void IN_MeleeBreath_Down(void)
{
    IN_KeyDown(&kb[19]);
    IN_KeyDown(&kb[15]);
}

void IN_MeleeBreath_Up(void)
{
    IN_KeyUp(&kb[19]);
    IN_KeyUp(&kb[15]);
}

void IN_Frag_Down(void)
{
    IN_KeyDown(&kb[16]);
}

void IN_Frag_Up(void)
{
    IN_KeyUp(&kb[16]);
}

void IN_Smoke_Down(void)
{
    IN_KeyDown(&kb[17]);
}

void IN_Smoke_Up(void)
{
    IN_KeyUp(&kb[17]);
}

void IN_Binoculars_Down(void)
{
    IN_KeyDown(&kb[18]);
}

void IN_Binoculars_Up(void)
{
    IN_KeyUp(&kb[18]);
}

void IN_BreathBinoculars_Down(void)
{
    IN_KeyDown(&kb[15]);
    IN_KeyDown(&kb[18]);
}

void IN_BreathBinoculars_Up(void)
{
    IN_KeyUp(&kb[15]);
    IN_KeyUp(&kb[18]);
}

void IN_Melee_Down(void)
{
    IN_KeyDown(&kb[19]);
}

void IN_Melee_Up(void)
{
    IN_KeyUp(&kb[19]);
}

void IN_Activate_Down(void)
{
    IN_KeyDown(&kb[20]);
}

void IN_Activate_Up(void)
{
    IN_KeyUp(&kb[20]);
}

void IN_Reload_Down(void)
{
    IN_KeyDown(&kb[21]);
}

void IN_Reload_Up(void)
{
    IN_KeyUp(&kb[21]);
}

void IN_UseReload_Down(void)
{
    IN_KeyDown(&kb[22]);
}

void IN_UseReload_Up(void)
{
    IN_KeyUp(&kb[22]);
}

void IN_LeanLeft_Down(void)
{
    IN_KeyDown(&kb[23]);
}

void IN_LeanLeft_Up(void)
{
    IN_KeyUp(&kb[23]);
}

void IN_LeanRight_Down(void)
{
    IN_KeyDown(&kb[24]);
}

void IN_LeanRight_Up(void)
{
    IN_KeyUp(&kb[24]);
}

void IN_Stance_Down(void)
{
    clientActive_t *cl;
    LegacyHacks **lh;

    if (kb[25].active || kb[11].active ) {
        return;
    }

    cl = *(clientActive_t **)imp_cl;
    cl->stanceHeld = 1;
    lh = (LegacyHacks **)imp_legacyHacks;
    cl->stancePosition = (*lh)->cl_stance;
    cl->stanceTime = *(int *)imp_com_frameTime;
    if (cl->stancePosition != 1) {
        (*lh)->cl_stance = 1;
    }
}

void IN_Stance_Up(void)
{
    clientActive_t **clp;
    clientActive_t *ptr;
    if (kb[25].active || kb[11].active )
        return;
    clp = (clientActive_t **)imp_cl;
    ptr = *clp;
    if (ptr->stanceHeld && ptr->stancePosition == 1) {
        (*(LegacyHacks **)imp_legacyHacks)->cl_stance = 0;
    }
    (*clp)->stanceHeld = 0;
}

void IN_ToggleADS(void)
{
    clientActive_t *p = *(clientActive_t **)imp_cl;
    p->usingAds = (p->usingAds == 0) ? 1 : 0;
}

void IN_LeaveADS(void)
{
    (*(clientActive_t **)imp_cl)->usingAds = 0;
}

void IN_LowerStance(void)
{
    int *statePtr;
    int val;
    if (kb[25].active || kb[11].active )
        return;
    statePtr = &(*(LegacyHacks **)imp_legacyHacks)->cl_stance;
    val = *statePtr;
    if (val <= 0)
        *statePtr = 1;
    else if (val == 1)
        *statePtr = 2;
}

void IN_RaiseStance(void)
{
    if (kb[25].active != 0 || kb[11].active != 0 )
        return;
    int *stance = &(*(LegacyHacks **)imp_legacyHacks)->cl_stance;
    if (*stance > 1)
        *stance = 1;
    else if (*stance == 1)
        *stance = 0;
}

void IN_ToggleCrouch(void)
{
    if (kb[25].active != 0 || kb[11].active != 0 )
        return;
    int *stance = &(*(LegacyHacks **)imp_legacyHacks)->cl_stance;
    *stance = (*stance != 1) ? 1 : 0;
}

void IN_ToggleProne(void)
{
    if (kb[25].active != 0 || kb[11].active != 0 )
        return;
    int *stance = &(*(LegacyHacks **)imp_legacyHacks)->cl_stance;
    *stance = (*stance != 2) ? 2 : 0;
}

void IN_GoProne(void)
{
    if (kb[25].active != 0 || kb[11].active != 0 )
        return;
    (*(LegacyHacks **)imp_legacyHacks)->cl_stance = 2;
}

void IN_GoCrouch(void)
{
    if (kb[25].active != 0 || kb[11].active != 0 )
        return;
    (*(LegacyHacks **)imp_legacyHacks)->cl_stance = 1;
}

void IN_GoStandDown(void)
{
    int *stance;

    IN_KeyDown(&kb[12]);
    stance = &(*(LegacyHacks **)imp_legacyHacks)->cl_stance;
    if (!*stance) {
        IN_KeyDown(&kb[10]);
        return;
    }

    if (kb[25].active || kb[11].active ) {
        return;
    }

    *stance = 0;
}

void IN_GoStandUp(void)
{
    IN_KeyUp(&kb[12]);
    IN_KeyUp(&kb[10]);
}

void IN_TalkDown(void)
{
    IN_KeyDown(&kb[27]);
}

void IN_TalkUp(void)
{
    IN_KeyUp(&kb[27]);
}

Bool IsTalking(void)
{
    return kb[27].active;
}

void CL_MouseEvent(const int dx_in, const int dy_in)
{
    clientActive_t *ptr = *(clientActive_t **)imp_cl;
    int dx = dx_in, dy = dy_in;
    int index;
    if (!(ptr->keyCatchers & 8) || cl_bypassMouseInput->current.enabled) {
        index = ptr->mouseIndex;
        ptr->mouseDx[index] += dx;
        ptr->mouseDy[index] += dy;
    } else {
        UI_MouseEvent(dx, dy);
    }
}

void CL_MouseEventAbsolute(const int x, const int y, const int dx_in, const int dy_in)
{
    clientActive_t *ptr = *(clientActive_t **)imp_cl;
    int dx = dx_in, dy = dy_in;
    int index;

    if (!(ptr->keyCatchers & 8) || cl_bypassMouseInput->current.enabled) {
        index = ptr->mouseIndex;
        ptr->mouseDx[index] += dx;
        ptr->mouseDy[index] += dy;
    } else {
        UI_MouseEventAbsolute(x, y);
    }
}

void CL_WriteVoicePacket(void)
{
    clientConnection_t *clc;
    clientActive_t *cl;
    msg_t msg;
    byte data[MAX_MSGLEN];
    int voicePacket;

    clc = *(clientConnection_t **)imp_clc;
    if (clc->demoplaying || (clc->state != CA_ACTIVE && clc->state != CA_LOADING)) {
        return;
    }

    MSG_Init(&msg, data, sizeof(data));
    MSG_WriteString(&msg, (const char *)"v");
    MSG_WriteShort(&msg, clc->qport);

    cl = *(clientActive_t **)imp_cl;
    MSG_WriteByte(&msg, (byte)cl->voicePacketCount);

    for (voicePacket = 0; voicePacket < cl->voicePacketCount; ++voicePacket) {
        MSG_WriteByte(&msg, cl->voicePackets[voicePacket].dataSize);
        MSG_WriteData(&msg, cl->voicePackets[voicePacket].data, cl->voicePackets[voicePacket].dataSize);
    }

    if ((*(const dvar_t **)imp_cl_showSend)->current.enabled) {
        Com_Printf((const char *)"voice: %i\n", msg.cursize);
    }

    clc = *(clientConnection_t **)imp_clc;
    NET_OutOfBandVoiceData(clc->netchan.sock, clc->serverAddress, msg.data, msg.cursize);
}

void CL_WritePacket(void)
{
    clientConnection_t *clc_ptr;
    clientActive_t *cl_ptr;
    msg_t buf;
    byte data[MAX_MSGLEN];
    usercmd_t nullcmd;
    byte compressedBuf[MAX_MSGLEN];
    int compressedSize;
    int i;
    int key;
    usercmd_t *lastCmd;

    clc_ptr = *(clientConnection_t **)imp_clc;

    if (clc_ptr->demoplaying)
        return;
    if (clc_ptr->state == 1 || clc_ptr->state == 2)
        return;

    cl_ptr = *(clientActive_t **)imp_cl;

    MSG_SetDefaultUserCmd((void *)&cl_ptr->snap.ps, (void *)&nullcmd);

    MSG_Init(&buf, data, MAX_MSGLEN);

    {
        int serverId = cl_ptr->serverId;
        CL_WritePacketDbg("[CL_WritePacket] serverId=%d\n", serverId);
        MSG_WriteByte(&buf, serverId);
    }

    MSG_WriteLong(&buf, clc_ptr->serverMessageSequence);
    MSG_WriteLong(&buf, clc_ptr->serverCommandSequence);

    i = clc_ptr->reliableAcknowledge + 1;
    while (i <= clc_ptr->reliableSequence) {
        MSG_WriteBits(&buf, 2, 3);
        MSG_WriteLong(&buf, i);
        {
            int idx = (i & 0x7f) << 10;
            MSG_WriteString(&buf, (const char *)&clc_ptr->reliableCommands[i & 0x7f]);
        }
        i++;
    }

    {
        clientActive_t *cl2 = *(clientActive_t **)imp_cl;
        const dvar_t *packetdup = *(const dvar_t **)imp_cl_packetdup;
        int cmdNum = clc_ptr->netchan.outgoingSequence;
        int dupIdx = (cmdNum - packetdup->current.integer - 1) & 0x1f;
        compressedSize = cl2->cmdNumber - cl2->outPackets[dupIdx].p_cmdNumber;
    }

    if (compressedSize > 0x20) {
        Com_Printf((const char *)"MAX_PACKET_USERCMDS\n");
        compressedSize = 0x20;
    }

    if (compressedSize <= 0) {
        lastCmd = &nullcmd;
        goto write_footer;
    }

    {
        const dvar_t *showSend = *(const dvar_t **)imp_cl_showSend;
        if (showSend->current.enabled) {
            Com_Printf((const char *)"(%i)", compressedSize);
        }
    }

    {
        const dvar_t *nodelta = *(const dvar_t **)imp_cl_nodelta;
        clientActive_t *cl3 = *(clientActive_t **)imp_cl;
        int snap = cl3->snap.valid;

        if (nodelta->current.enabled || !snap) {
            goto write_nodelta;
        } else {
            clientConnection_t *clc2 = *(clientConnection_t **)imp_clc;
            if (clc2->demowaiting || clc2->serverMessageSequence != cl3->snap.messageNum) {
                goto write_nodelta;
            }

            MSG_WriteBits(&buf, 0, 3);
        }
    }

    goto write_cmdcount;

write_nodelta:
    MSG_WriteBits(&buf, 1, 3);

write_cmdcount:
    MSG_WriteByte(&buf, compressedSize);

    {
        clientConnection_t *clc3 = *(clientConnection_t **)imp_clc;
        key = clc3->serverMessageSequence;
        key ^= clc3->checksumFeed;
        {
            key ^= Com_HashKey(clc3->serverCommands[clc3->serverCommandSequence & 0x7f], 0x20);
        }
    }

    if (compressedSize > 0) {
        usercmd_t *prevCmd = &nullcmd;
        clientActive_t *cl4 = *(clientActive_t **)imp_cl;
        for (i = 0; i != compressedSize; i++) {
            int idx = (cl4->cmdNumber - compressedSize + 1 + i) & 0x7f;

            usercmd_t *curCmd = &cl4->cmds[idx];
            MSG_WriteDeltaUsercmdKey(&buf, key, (void *)prevCmd, (void *)curCmd);
            prevCmd = curCmd;
        }
        lastCmd = prevCmd;
    } else {
        lastCmd = &nullcmd;
    }

write_footer:

    MSG_WriteBits(&buf, 3, 3);

    {
        byte *msgData = buf.data;
        memcpy(compressedBuf, msgData, 9);
        compressedSize = 9 + MSG_WriteBitsCompress(msgData + 9, compressedBuf + 9, buf.cursize - 9);
    }

    {
        clientConnection_t *clc4 = *(clientConnection_t **)imp_clc;
        clientActive_t *cl5 = *(clientActive_t **)imp_cl;
        int slot = clc4->netchan.outgoingSequence & 0x1f;

        cl5->outPackets[slot].p_realtime = cls.realtime;
        cl5->outPackets[slot].p_serverTime = lastCmd->serverTime;
        cl5->outPackets[slot].p_cmdNumber = cl5->cmdNumber;
        clc4->lastPacketSentTime = cls.realtime;

        {
            const dvar_t *showSend2 = *(const dvar_t **)imp_cl_showSend;
            if (showSend2->current.enabled) {
                Com_Printf((const char *)"%i ", compressedSize);
            }
        }

        CL_Netchan_Transmit((void *)&clc4->netchan, compressedBuf, compressedSize);

        if (clc4->netchan.unsentFragments) {
            do {
                CL_Netchan_TransmitNextFragment((void *)&clc4->netchan);
            } while ((*(clientConnection_t **)imp_clc)->netchan.unsentFragments);
        }
    }
}

void CL_InitInput(void)
{
    Cmd_AddCommand("centerview", IN_CenterView);
    Cmd_AddCommand("+moveup", IN_UpDown);
    Cmd_AddCommand("-moveup", IN_UpUp);
    Cmd_AddCommand("+movedown", IN_DownDown);
    Cmd_AddCommand("-movedown", IN_DownUp);
    Cmd_AddCommand("+left", IN_LeftDown);
    Cmd_AddCommand("-left", IN_LeftUp);
    Cmd_AddCommand("+right", IN_RightDown);
    Cmd_AddCommand("-right", IN_RightUp);
    Cmd_AddCommand("+forward", IN_ForwardDown);
    Cmd_AddCommand("-forward", IN_ForwardUp);
    Cmd_AddCommand("+back", IN_BackDown);
    Cmd_AddCommand("-back", IN_BackUp);
    Cmd_AddCommand("+lookup", IN_LookupDown);
    Cmd_AddCommand("-lookup", IN_LookupUp);
    Cmd_AddCommand("+lookdown", IN_LookdownDown);
    Cmd_AddCommand("-lookdown", IN_LookdownUp);
    Cmd_AddCommand("+strafe", IN_StrafeDown);
    Cmd_AddCommand("-strafe", IN_StrafeUp);
    Cmd_AddCommand("+moveleft", IN_MoveleftDown);
    Cmd_AddCommand("-moveleft", IN_MoveleftUp);
    Cmd_AddCommand("+moveright", IN_MoverightDown);
    Cmd_AddCommand("-moveright", IN_MoverightUp);
    Cmd_AddCommand("+speed", IN_SpeedDown);
    Cmd_AddCommand("-speed", IN_SpeedUp);
    Cmd_AddCommand("+attack", IN_Attack_Down);
    Cmd_AddCommand("-attack", IN_Attack_Up);
    Cmd_AddCommand("+melee", IN_Melee_Down);
    Cmd_AddCommand("-melee", IN_Melee_Up);
    Cmd_AddCommand("+holdbreath", IN_Breath_Down);
    Cmd_AddCommand("-holdbreath", IN_Breath_Up);
    Cmd_AddCommand("+melee_breath", IN_MeleeBreath_Down);
    Cmd_AddCommand("-melee_breath", IN_MeleeBreath_Up);
    Cmd_AddCommand("+frag", IN_Frag_Down);
    Cmd_AddCommand("-frag", IN_Frag_Up);
    Cmd_AddCommand("+smoke", IN_Smoke_Down);
    Cmd_AddCommand("-smoke", IN_Smoke_Up);
    Cmd_AddCommand("+binoculars", IN_Binoculars_Down);
    Cmd_AddCommand("-binoculars", IN_Binoculars_Up);
    Cmd_AddCommand("+breath_binoculars", IN_BreathBinoculars_Down);
    Cmd_AddCommand("-breath_binoculars", IN_BreathBinoculars_Up);
    Cmd_AddCommand("+activate", IN_Activate_Down);
    Cmd_AddCommand("-activate", IN_Activate_Up);
    Cmd_AddCommand("+reload", IN_Reload_Down);
    Cmd_AddCommand("-reload", IN_Reload_Up);
    Cmd_AddCommand("+usereload", IN_UseReload_Down);
    Cmd_AddCommand("-usereload", IN_UseReload_Up);
    Cmd_AddCommand("+leanleft", IN_LeanLeft_Down);
    Cmd_AddCommand("-leanleft", IN_LeanLeft_Up);
    Cmd_AddCommand("+leanright", IN_LeanRight_Down);
    Cmd_AddCommand("-leanright", IN_LeanRight_Up);
    Cmd_AddCommand("+prone", IN_Prone_Down);
    Cmd_AddCommand("-prone", IN_Prone_Up);
    Cmd_AddCommand("+stance", IN_Stance_Down);
    Cmd_AddCommand("-stance", IN_Stance_Up);
    Cmd_AddCommand("+mlook", IN_MLookDown);
    Cmd_AddCommand("-mlook", IN_MLookUp);
    Cmd_AddCommand("toggleads", IN_ToggleADS);
    Cmd_AddCommand("leaveads", IN_LeaveADS);
    Cmd_AddCommand("lowerstance", IN_LowerStance);
    Cmd_AddCommand("raisestance", IN_RaiseStance);
    Cmd_AddCommand("togglecrouch", IN_ToggleCrouch);
    Cmd_AddCommand("toggleprone", IN_ToggleProne);
    Cmd_AddCommand("goprone", IN_GoProne);
    Cmd_AddCommand("gocrouch", IN_GoCrouch);
    Cmd_AddCommand("+gostand", IN_GoStandDown);
    Cmd_AddCommand("-gostand", IN_GoStandUp);
    Cmd_AddCommand("+talk", IN_TalkDown);
    Cmd_AddCommand("-talk", IN_TalkUp);

    cl_analog_attack_threshold = Dvar_RegisterFloat("cl_analog_attack_threshold", 0.8f, 0.0001f, 1.0f, 0x1000);
    cl_stanceHoldTime = Dvar_RegisterInt("cl_stanceHoldTime", 300, 0, 1000, 0x1000);
    {
        const dvar_t *nodelta = Dvar_RegisterBool_mac("cl_nodelta", 0, 0x1000);
        *(const dvar_t **)imp_cl_nodelta = nodelta;
    }

#if COD2_FEATURE_GAMEPAD
    CL_Gamepad_Init();
#endif
}

void CL_ShutdownInput(void)
{
    Cmd_RemoveCommand("centerview");
    Cmd_RemoveCommand("+moveup");
    Cmd_RemoveCommand("-moveup");
    Cmd_RemoveCommand("+movedown");
    Cmd_RemoveCommand("-movedown");
    Cmd_RemoveCommand("+left");
    Cmd_RemoveCommand("-left");
    Cmd_RemoveCommand("+right");
    Cmd_RemoveCommand("-right");
    Cmd_RemoveCommand("+forward");
    Cmd_RemoveCommand("-forward");
    Cmd_RemoveCommand("+back");
    Cmd_RemoveCommand("-back");
    Cmd_RemoveCommand("+lookup");
    Cmd_RemoveCommand("-lookup");
    Cmd_RemoveCommand("+lookdown");
    Cmd_RemoveCommand("-lookdown");
    Cmd_RemoveCommand("+strafe");
    Cmd_RemoveCommand("-strafe");
    Cmd_RemoveCommand("+moveleft");
    Cmd_RemoveCommand("-moveleft");
    Cmd_RemoveCommand("+moveright");
    Cmd_RemoveCommand("-moveright");
    Cmd_RemoveCommand("+speed");
    Cmd_RemoveCommand("-speed");
    Cmd_RemoveCommand("+attack");
    Cmd_RemoveCommand("-attack");
    Cmd_RemoveCommand("+melee");
    Cmd_RemoveCommand("-melee");
    Cmd_RemoveCommand("+holdbreath");
    Cmd_RemoveCommand("-holdbreath");
    Cmd_RemoveCommand("+melee_breath");
    Cmd_RemoveCommand("-melee_breath");
    Cmd_RemoveCommand("+frag");
    Cmd_RemoveCommand("-frag");
    Cmd_RemoveCommand("+smoke");
    Cmd_RemoveCommand("-smoke");
    Cmd_RemoveCommand("+binoculars");
    Cmd_RemoveCommand("-binoculars");
    Cmd_RemoveCommand("+breath_binoculars");
    Cmd_RemoveCommand("-breath_binoculars");
    Cmd_RemoveCommand("+activate");
    Cmd_RemoveCommand("-activate");
    Cmd_RemoveCommand("+reload");
    Cmd_RemoveCommand("-reload");
    Cmd_RemoveCommand("+usereload");
    Cmd_RemoveCommand("-usereload");
    Cmd_RemoveCommand("+leanleft");
    Cmd_RemoveCommand("-leanleft");
    Cmd_RemoveCommand("+leanright");
    Cmd_RemoveCommand("-leanright");
    Cmd_RemoveCommand("+prone");
    Cmd_RemoveCommand("-prone");
    Cmd_RemoveCommand("+stance");
    Cmd_RemoveCommand("-stance");
    Cmd_RemoveCommand("+mlook");
    Cmd_RemoveCommand("-mlook");
    Cmd_RemoveCommand("toggleads");
    Cmd_RemoveCommand("leaveads");
    Cmd_RemoveCommand("lowerstance");
    Cmd_RemoveCommand("raisestance");
    Cmd_RemoveCommand("togglecrouch");
    Cmd_RemoveCommand("toggleprone");
    Cmd_RemoveCommand("goprone");
    Cmd_RemoveCommand("gocrouch");
    Cmd_RemoveCommand("+gostand");
    Cmd_RemoveCommand("-gostand");
    Cmd_RemoveCommand("+talk");
    Cmd_RemoveCommand("-talk");

#if COD2_FEATURE_GAMEPAD
    CL_Gamepad_Shutdown();
#endif
}

void CL_CmdButtons(usercmd_t *cmd)
{
    clientActive_t **clp = (clientActive_t **)imp_cl;
    byte *kbBase = (byte *)kb;

#define KB_AT(offset) ((kbutton_t *)(kbBase + (offset)))
#define CONSUME(offset, bit)                         \
    do {                                             \
        kbutton_t *b__ = KB_AT(offset);              \
        if (*(const unsigned short *)&b__->active) { \
            cmd->buttons |= (bit);                   \
            b__->wasPressed = 0;                     \
        }                                            \
    } while (0)

    CONSUME(0x118, 0x1);
    CONSUME(0x12c, 0x8000);
    CONSUME(0x140, 0x10000);
    CONSUME(0x154, 0x20000);
    CONSUME(0x168, 0x4000);
    CONSUME(0x17c, 0x4);
    CONSUME(0x190, 0x8);
    CONSUME(0x1a4, 0x10);
    CONSUME(0x1b8, 0x20);
    CONSUME(0x1cc, 0x40);
    CONSUME(0x1e0, 0x80);
    CONSUME(0x1f4, 0x100);
    CONSUME(0x208, 0x200);
    CONSUME(0xc8, 0x400);

    if ((*clp)->keyCatchers && !cl_bypassMouseInput->current.enabled) {
        cmd->buttons |= 0x40000;
    }

    if ((int)((*clp)->snap.ps.pm_type - 2) <= 2 && CL_ConsumeButtonPress(KB_AT(0xf0))) {
        cmd->buttons |= 0x400;
    }

    if ((*clp)->cgameInShellshock) {
        cmd->buttons |= 0x800;
    }

#undef CONSUME
#undef KB_AT
}

void CL_MouseMove(usercmd_t *cmd)
{
    clientActive_t *cl;
    float mx;
    float my;
    float rate;
    float sensitivity;
    float accel;
    float baseSensitivity;
    float cgameSensitivity;
    float yawDelta;
    float pitchDelta;
    int index;

#define KB_AT(offset) ((kbutton_t *)((byte *)kb + (offset)))

    cl = *(clientActive_t **)imp_cl;
    if (m_filter->current.enabled) {
        mx = (float)(cl->mouseDx[0] + cl->mouseDx[1]) * 0.5f;
        my = (float)(cl->mouseDy[0] + cl->mouseDy[1]) * 0.5f;
    } else {
        index = cl->mouseIndex;
        mx = (float)cl->mouseDx[index];
        my = (float)cl->mouseDy[index];
    }

    index = cl->mouseIndex ^ 1;
    cl->mouseIndex = index;
    cl->mouseDx[index] = 0;
    cl->mouseDy[index] = 0;

#if COD2_FEATURE_GAMEPAD

    if (mx * mx + my * my > 9.0f)
        CL_Gamepad_NoteKbmActive();
#endif

    if (frame_msec == 0) {
        return;
    }

    rate = sqrtf(mx * mx + my * my) / (float)frame_msec;
    if (!isfinite(rate)) {
        return;
    }

    accel = CL_FiniteOrDefault(cl_mouseAccel->current.value, 0.0f);
    baseSensitivity = CL_FiniteOrDefault(cl_sensitivity->current.value, 5.0f);
    cgameSensitivity = CL_FiniteOrDefault(cl->cgameSensitivity, 1.0f);
    sensitivity = (rate * accel + baseSensitivity) * cgameSensitivity;
    if (!isfinite(sensitivity)) {
        return;
    }

    if (rate != 0.0f && cl_showMouseRate->current.enabled) {
        Com_Printf((const char *)"%f : %f\n", rate, sensitivity);
    }

    if ((short)cl->snap.ps.pm_flags < 0) {
        return;
    }

    if (cl->snap.ps.eFlags & 0x300) {
        mx *= 2.5f;
        my *= 2.0f;
    } else {
        mx *= sensitivity;
        my *= sensitivity;
    }

    if (mx == 0.0f && my == 0.0f) {
        return;
    }

    if (KB_AT(0xa0)->active) {
        cmd->rightmove = ClampChar((int)cmd->rightmove + (int)(mx * CL_FiniteOrDefault(m_side->current.value, 0.25f)));
    } else {
        yawDelta = mx * CL_FiniteOrDefault(m_yaw->current.value, 0.022f);
        yawDelta = CL_ClampMouseAxisDelta(yawDelta, CL_FiniteOrDefault(cl->cgameMaxYawSpeed, 0.0f));
        cl->viewangles[1] -= yawDelta;
    }

    if ((KB_AT(0x104)->active || cl_freelook->current.enabled) && !KB_AT(0xa0)->active) {
        pitchDelta = my * CL_FiniteOrDefault(m_pitch->current.value, 0.022f);
        pitchDelta = CL_ClampMouseAxisDelta(pitchDelta, CL_FiniteOrDefault(cl->cgameMaxPitchSpeed, 0.0f));
        cl->viewangles[0] += pitchDelta;
    } else {
        cmd->forwardmove = ClampChar((int)cmd->forwardmove - (int)(my * CL_FiniteOrDefault(m_forward->current.value, 0.25f)));
    }

#undef KB_AT
}

void IN_DownDown(void)
{
    IN_KeyDown(&kb[11]);
    if (kb[25].active || kb[11].active )
        (*(LegacyHacks **)imp_legacyHacks)->cl_stanceTemp = 1;
    else
        (*(LegacyHacks **)imp_legacyHacks)->cl_stanceTemp = 0;
}

void IN_DownUp(void)
{
    IN_KeyUp(&kb[11]);
    if (kb[25].active || kb[11].active )
        (*(LegacyHacks **)imp_legacyHacks)->cl_stanceTemp = 1;
    else
        (*(LegacyHacks **)imp_legacyHacks)->cl_stanceTemp = 0;
}

void IN_Prone_Down(void)
{
    IN_KeyDown(&kb[25]);
    if (kb[25].active || kb[11].active )
        (*(LegacyHacks **)imp_legacyHacks)->cl_stanceTemp = 1;
    else
        (*(LegacyHacks **)imp_legacyHacks)->cl_stanceTemp = 0;
}

void IN_Prone_Up(void)
{
    IN_KeyUp(&kb[25]);
    if (kb[25].active || kb[11].active )
        (*(LegacyHacks **)imp_legacyHacks)->cl_stanceTemp = 1;
    else
        (*(LegacyHacks **)imp_legacyHacks)->cl_stanceTemp = 0;
}

void IN_MLookUp(void)
{
    kb[13].active = 0;
    if ((*(const dvar_t **)imp_cl_freelook)->current.enabled == 0) {
        clientActive_t *cl = *(clientActive_t **)imp_cl;
        cl->viewangles[0] = (float)cl->snap.ps.delta_angles[0] * -0.0054931640625f;
    }
}

void CL_AdjustAngles(void)
{
    clientActive_t *cl;
    float speed;

#define KB_AT(offset) ((kbutton_t *)((byte *)kb + (offset)))

    speed = (float)cls.frametime * 0.0010000000474974513f;
    if (KB_AT(0xb4)->active) {
        speed *= cl_anglespeedkey->current.value;
    }

    if (!KB_AT(0xa0)->active) {
        cl = *(clientActive_t **)imp_cl;
        cl->viewangles[1] -= CL_KeyState(KB_AT(0x14)) * (speed * cl_yawspeed->current.value);

        cl = *(clientActive_t **)imp_cl;
        cl->viewangles[1] += CL_KeyState(KB_AT(0x00)) * (speed * cl_yawspeed->current.value);
    }

    cl = *(clientActive_t **)imp_cl;
    cl->viewangles[0] -= CL_KeyState(KB_AT(0x50)) * (speed * cl_pitchspeed->current.value);

    cl = *(clientActive_t **)imp_cl;
    cl->viewangles[0] += CL_KeyState(KB_AT(0x64)) * (speed * cl_pitchspeed->current.value);

#undef KB_AT
}

void CL_KeyMove(usercmd_t *cmd)
{
    clientActive_t *cl;
    int *legacyStance;
    int side;
    int forward;

#define KB_AT(offset) ((kbutton_t *)((byte *)kb + (offset)))

    cl = *(clientActive_t **)imp_cl;

    (void)cl;

    {
        int clStance = (*(LegacyHacks **)imp_legacyHacks)->cl_stance;
        if (clStance == 1)
            cmd->buttons |= 0x200;
        else if (clStance == 2)
            cmd->buttons |= 0x100;
    }

    if (((KB_AT(0xb4)->active != 0) == (cl->usingAds == 0)) || PM_IsBinocularsADS(&cl->snap.ps)) {
        cmd->buttons |= 0x1000;
    } else {
        cmd->buttons &= ~0x1000;
    }

    if (KB_AT(0xa0)->active) {
        side = CL_KeyMoveValue(KB_AT(0x14)) - CL_KeyMoveValue(KB_AT(0x00));
    } else {
        side = 0;
    }

    side += CL_KeyMoveValue(KB_AT(0x8c));
    side -= CL_KeyMoveValue(KB_AT(0x78));

    forward = CL_KeyMoveValue(KB_AT(0x28)) - CL_KeyMoveValue(KB_AT(0x3c));

    cmd->forwardmove = ClampChar(forward);
    cmd->rightmove = ClampChar(side);

#if COD2_FEATURE_GAMEPAD

    CL_Gamepad_Move(cmd);
#endif

#undef KB_AT
}

usercmd_t CL_CreateCmd(void)
{
    clientActive_t *cl;
    usercmd_t cmd;
    float oldPitch;
    float pitchDelta;
    int currentCmdTime;
    int i;

    cl = *(clientActive_t **)imp_cl;
    oldPitch = cl->viewangles[0];

    CL_AdjustAngles();

    memset(&cmd, 0, sizeof(cmd));

    CL_CmdButtons(&cmd);
    CL_KeyMove(&cmd);
    CL_MouseMove(&cmd);

    pitchDelta = cl->viewangles[0] - oldPitch;
    if (pitchDelta > 90.0f) {
        cl->viewangles[0] = oldPitch + 90.0f;
    } else if (-pitchDelta > 90.0f) {
        cl->viewangles[0] = oldPitch - 90.0f;
    }

    cmd.weapon = (byte)cl->cgameUserCmdValue;
    cmd.offHandIndex = (byte)cl->cgameUserHoldableValue;

    currentCmdTime = cl->serverTime;
    if (cl->serverTime - cl->snap.serverTime >= 5001) {
        currentCmdTime = cl->snap.serverTime + 5000;
    }
    cmd.serverTime = currentCmdTime;

    for (i = 0; i < 3; ++i) {
        cmd.angles[i] = ((int)((cl->viewangles[i] + cl->cgameKickAngles[i]) * 182.04444885253906f)) & 0xffff;
    }

    return cmd;
}

void CL_SendCmdInternal(void)
{
    clientConnection_t *clc;
    clientActive_t *cl;
    outPacket_t *outPacket;
    const int *serverAddrWords;
    int connectElapsed;

    clc = *(clientConnection_t **)imp_clc;
    if (clc->state > CA_LOADING) {
        cl = *(clientActive_t **)imp_cl;
        ++cl->cmdNumber;
        cl->cmds[cl->cmdNumber & 127] = CL_CreateCmd();
    }

    if (clc->demoplaying || clc->state == CA_CINEMATIC || clc->state == CA_LOGO) {
        goto not_ready;
    }

    connectElapsed = cls.realtime - clc->connectTime;
    if (clc->demowaiting) {
        if (connectElapsed <= 49) {
            goto not_ready;
        }
    } else if ((unsigned int)(clc->state - CA_PRIMED) > 1) {

        if (clc->serverAddress.type != NA_LOOPBACK && connectElapsed <= 999)
            goto not_ready;
    }

    serverAddrWords = (const int *)&clc->serverAddress;
    if (clc->serverAddress.type == NA_LOOPBACK || Sys_IsLANAddress(clc->serverAddress)) {
        CL_WritePacket();
        return;
    }

    cl = *(clientActive_t **)imp_cl;
    outPacket = &cl->outPackets[(clc->netchan.outgoingSequence - 1) & 31];
    if (cls.realtime - outPacket->p_realtime >= 1000 / cl_maxpackets->current.integer) {
        CL_WritePacket();
        return;
    }

not_ready:
    if (cl_showSend->current.enabled) {
        Com_Printf((const char *)". ");
    }
}

void CL_Input(void)
{
    if ((*(clientConnection_t **)imp_clc)->state != 8)
        return;
    CL_SyncGpu();
    CL_SendCmdInternal();
}

void CL_SendCmd(void)
{
    int state = (*(clientConnection_t **)imp_clc)->state;
    if (state <= 4 || state == 8)
        return;
    CL_SendCmdInternal();
}
