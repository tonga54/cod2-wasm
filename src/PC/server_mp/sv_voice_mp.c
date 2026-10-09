#include "common_types.h"
#include "imports.h"
#include <string.h>

extern void LargeLocal_LargeLocal(LargeLocal *ll, int size);
extern void *LargeLocal_GetBuf(LargeLocal *ll);
extern void ZN10LargeLocalD1Ev(LargeLocal *ll);
extern void MSG_Init(msg_t *msg, byte *data, int length);
extern void MSG_WriteString(msg_t *msg, const char *s);
extern void MSG_WriteByte(msg_t *msg, int c);
extern void MSG_WriteData(msg_t *msg, const void *data, int length);
extern int MSG_ReadByte(msg_t *msg);
extern int MSG_ReadShort(msg_t *msg);
extern void MSG_ReadData(msg_t *msg, void *data, int len);
extern void NET_OutOfBandVoiceData(netsrc_t sock, netadr_t adr, byte *data, int len);
extern void G_BroadcastVoice(gentity_t *talker, VoicePacket_t *voicePacket);
extern void Com_Printf(const char *msg, ...);

extern serverStatic_t svs;
extern byte *sv_voice_dvar;

void SV_SendClientVoiceData(client_t *client);
Bool SV_ClientWantsVoiceData(int clientNum);
Bool SV_ClientHasClientMuted(int listener, int talker);
void SV_QueueVoicePacket(int talkerNum, int clientNum, VoicePacket_t *voicePacket);
void SV_UserVoice(client_t *cl, msg_t *msg);
void SV_PreGameUserVoice(client_t *cl, msg_t *msg);

void SV_SendClientVoiceData(client_t *client)
{
    byte _ll_buf[16];
    byte *buf;
    msg_t msg;
    netadr_t adr;
    int i;

    LargeLocal_LargeLocal((LargeLocal *)_ll_buf, 0x20000);
    buf = (byte *)LargeLocal_GetBuf((LargeLocal *)_ll_buf);

    if (client->state != 4) {
        ZN10LargeLocalD1Ev((LargeLocal *)_ll_buf);
        return;
    }

    if (client->voicePacketCount == 0) {
        ZN10LargeLocalD1Ev((LargeLocal *)_ll_buf);
        return;
    }

    MSG_Init(&msg, buf, 0x20000);
    MSG_WriteString(&msg, "voicePacket");

    MSG_WriteByte(&msg, client->voicePacketCount);

    if (client->voicePacketCount > 0) {
        for (i = 0; i < client->voicePacketCount; i++) {

            MSG_WriteByte(&msg, client->voicePackets[i].talker);

            MSG_WriteByte(&msg, client->voicePackets[i].dataSize);

            MSG_WriteData(&msg, client->voicePackets[i].data, client->voicePackets[i].dataSize);
        }
    }

    if (msg.overflowed) {
        Com_Printf("WARNING: voice msg overflowed for %s\n", client->name);
        ZN10LargeLocalD1Ev((LargeLocal *)_ll_buf);
        return;
    }

    adr = client->netchan.remoteAddress;
    NET_OutOfBandVoiceData(1, adr, msg.data, msg.cursize);

    client->voicePacketCount = 0;
    ZN10LargeLocalD1Ev((LargeLocal *)_ll_buf);
}

Bool SV_ClientWantsVoiceData(int clientNum)
{
    serverStatic_t *svs = (serverStatic_t *)imp_svs;
    return svs->clients[clientNum].sendVoice;
}

Bool SV_ClientHasClientMuted(int listener, int talker)
{
    serverStatic_t *svs = (serverStatic_t *)imp_svs;
    return svs->clients[listener].muteList[talker];
}

void SV_QueueVoicePacket(int talkerNum, int clientNum, VoicePacket_t *voicePacket)
{
    serverStatic_t *svs = (serverStatic_t *)imp_svs;
    client_t *client = &svs->clients[clientNum];
    int count;

    count = client->voicePacketCount;
    if (count > 39)
        return;

    client->voicePackets[count].dataSize = voicePacket->dataSize;

    memcpy(client->voicePackets[count].data, voicePacket->data, voicePacket->dataSize);

    client->voicePackets[count].talker = (byte)talkerNum;

    client->voicePacketCount = count + 1;
}

void SV_UserVoice(client_t *cl, msg_t *msg)
{
    VoicePacket_t voicePacket;
    int packetCount;
    int dataSize;
    int i;

    if (*(byte *)(*(int *)(*(int *)imp_sv_voice) + 8) == 0)
        return;

    packetCount = MSG_ReadByte(msg);
    if (packetCount <= 0)
        return;

    for (i = 0; i < packetCount; i++) {
        dataSize = MSG_ReadByte(msg);

        if ((unsigned int)(dataSize - 1) > 255) {
            Com_Printf("Received invalid voice packet of size %i from %s\n",
                       dataSize, cl->name);
            return;
        }

        voicePacket.dataSize = dataSize;

        MSG_ReadData(msg, voicePacket.data, dataSize);

        G_BroadcastVoice(*(gentity_t **)&cl->gentity,
                         &voicePacket);
    }
}

void SV_PreGameUserVoice(client_t *cl, msg_t *msg)
{
    byte voiceData[256];
    int packetCount;
    int dataSize;
    int clientNum;
    int i, j;
    serverStatic_t *svs;
    client_t *otherCl;

    if (*(byte *)(*(int *)(*(int *)imp_sv_voice) + 8) == 0)
        return;

    svs = (serverStatic_t *)imp_svs;
    clientNum = (int)(cl - svs->clients);

    packetCount = MSG_ReadByte(msg);
    if (packetCount <= 0)
        return;

    for (i = 0; i < packetCount; i++) {
        dataSize = MSG_ReadShort(msg);

        if ((unsigned int)(dataSize - 1) > 255) {
            Com_Printf("Received invalid voice packet of size %i from %s\n",
                       dataSize, cl->name);
            return;
        }

        MSG_ReadData(msg, voiceData, dataSize);

        for (j = 0; j < 64; j++) {
            if (j == clientNum)
                goto sv_pregame_next;

            otherCl = &svs->clients[j];

            if (otherCl->state <= 1)
                goto sv_pregame_next;

            if (otherCl->muteList[clientNum] != 0)
                goto sv_pregame_next;

            if (otherCl->sendVoice == 0)
                goto sv_pregame_next;

            {
                int count = otherCl->voicePacketCount;
                if (count <= 39) {

                    otherCl->voicePackets[count].dataSize = dataSize;

                    memcpy(otherCl->voicePackets[count].data, voiceData, dataSize);

                    otherCl->voicePackets[count].talker = (byte)clientNum;

                    otherCl->voicePacketCount = count + 1;
                }
            }

        sv_pregame_next:;
        }
    }
}
