#include "common_types.h"
#include "imports.h"
#include "bytematch.h"
#include <string.h>
extern server_t sv;
extern serverStatic_t svs;

int endOffset = 0x1;

extern int startOffset;
extern int endOffset;

extern serverStatic_t svs;
extern server_t sv;
extern byte *sv_showcommands_dvar;
extern byte *sv_maxclients_dvar;
extern byte *sv_minPingRate_dvar;
extern byte *sv_padPackets_dvar;
extern byte *sv_showAverageBPS_dvar;
extern byte *showpackets_dvar;
extern byte *sv_maxRate_dvar;
extern byte *sv_fps_dvar;


#define CLIENT_RELIABLESEQUENCE 0x2080c
#define CLIENT_RELIABLEACK 0x20810
#define CLIENT_RELIABLESENT 0x20814
#define CLIENT_NAME 0x20c48
#define CLIENT_STATE 0x0
#define CLIENT_NETCHAN 0x6e5b4
#define CLIENT_SNAPSHOTMSEC 0x20d18
#define CLIENT_PUREAUTH 0x20d1c
#define CLIENT_LASTPACKETTIME 0x20d08
#define CLIENT_RATE 0x6e5a8
#define CLIENT_RATEDELAYED 0x6e5ac
#define CLIENT_NETTYPE 0x6e5c4
#define CLIENT_GAMESTATEMSGSENT 0x20840
#define CLIENT_LASTCLIENTCMD 0x4
#define CLIENT_CMDENTRY_BASE 0xc
#define CLIENT_CMDENTRY_SIZE 0x408
#define CLIENT_CMDENTRY_STROFF 0x400
#define CLIENT_CMDENTRY_MASK 0x7f
#define CLIENT_NETCHAN_SENDFRAG 0x725dc
#define CLIENT_NETCHAN_SENDLEN 0x725e4
#define CLIENT_NETCHAN_SENDSTART 0x725e0
#define CLIENT_DOWNLOAD 0x20c68

#define CACHEDCLIENT_STRIDE 9992

#define SVS_TIME 0x4
#define SVS_FLAGS 0x8
#define SVS_CLIENTS 0xc
#define SVS_NUMENTITIES 0x10
#define SVS_NUMCLIENTS 0x14
#define SVS_NEXTSNAPSHOTENTITIES 0x18
#define SVS_NEXTSNAPSHOTCLIENTS 0x1c
#define SVS_ENTITYSTATES 0x20
#define SVS_CLIENTSTATES 0x24
#define SVS_NUMONEWAYCLIENTS 0x28
#define SVS_ARCHIVEDFRAMECOUNT 0x2c
#define SVS_ARCHIVEDENTITYPARTS 0x30
#define SVS_ARCHIVEDENTITYBUF 0x34
#define SVS_ARCHIVEDENTITYBUFSIZE 0x38
#define SVS_ARCHIVEDENTNUMINDEX 0x3c
#define SVS_ARCHIVEDCLIENTNUMINDEX 0x40
#define SVS_ARCHIVEDFRAMENUM 0x44
#define SVS_ARCHIVEDENTITYDATA 0x48
#define SVS_ARCHIVEDCLIENTDATA 0x4c
#define SVS_CACHEDFRAMES 0x50
#define SVS_NEXTARCHIVEDFRAMENUM 0x54

extern void Com_Printf(const char *fmt, ...);
extern void Com_DPrintf(const char *fmt, ...);
extern void Com_Error(int code, const char *fmt, ...);
extern void MSG_WriteByte(msg_t *msg, int value);
extern void MSG_WriteLong(msg_t *msg, int value);
extern void MSG_WriteString(msg_t *msg, const char *s);
extern void MSG_WriteBit0(msg_t *msg);
extern void MSG_WriteBit1(msg_t *msg);
extern void MSG_WriteBits(msg_t *msg, int value, int bits);
extern int MSG_ReadBit(msg_t *msg);
extern int MSG_ReadBits(msg_t *msg, int bits);
extern int MSG_ReadLong(msg_t *msg);
extern void MSG_Init(msg_t *msg, byte *data, int length);
extern int MSG_WriteBitsCompress(byte *from, byte *to, int size);
extern void MSG_WriteDeltaEntity(msg_t *msg, byte *from, byte *to, int force);
extern void MSG_WriteDeltaClient(msg_t *msg, byte *from, byte *to, int force);
extern void MSG_WriteDeltaPlayerstate(msg_t *msg, byte *from, byte *to);
extern void MSG_WriteDeltaArchivedEntity(msg_t *msg, byte *from, byte *to, int force);
extern qboolean MSG_ReadDeltaClient(msg_t *msg, byte *from, byte *to, int clientNum);
extern void MSG_ReadDeltaPlayerstate(msg_t *msg, byte *from, byte *to);
extern qboolean MSG_ReadDeltaArchivedEntity(msg_t *msg, byte *from, byte *to, int entNum);
extern void SV_DropClient(client_t *client, const char *reason);
extern Bool SV_Netchan_Transmit(client_t *client, int length, byte *data);
extern Bool SV_Netchan_TransmitNextFragment(netchan_t *chan);
extern void SV_WriteDownloadToClient(client_t *client, msg_t *msg);
extern void SV_SendClientVoiceData(client_t *client);
extern byte *SV_GentityNum(int num);
extern byte *SV_SvEntityForGentity(byte *gent);
extern void *G_GetClientState(int clientNum);
extern int GetFollowPlayerState(int clientNum, byte *ps);
extern int G_GetClientArchiveTime(int clientNum);
extern void G_SetClientArchiveTime(int clientNum, int archiveTime);
extern float G_GetFogOpaqueDistSqrd(void);
extern int BoxDistSqrdExceeds(byte *absmin, byte *absmax, byte *org, float distSqrd);
extern int CM_PointLeafnum(byte *p);
extern int CM_LeafCluster(int leafnum);
extern byte *CM_ClusterPVS(int cluster);
extern int CM_BoxLeafnums(byte *mins, byte *maxs, int *leafs, int count, int *lastLeaf);
extern void AddLeanToPosition(float *org, float viewAngleYaw, float leanf, float a, float b);
extern void LargeLocal_LargeLocal(byte *ll, int size);
extern byte *LargeLocal_GetBuf(byte *ll);
extern void ZN10LargeLocalD1Ev(byte *ll);
extern int Sys_IsLANAddress(netadr_t adr);
extern void Dvar_SetInt(const dvar_t *dvar, int value);
extern Bool NET_OutOfBandPrint(netsrc_t sock, netadr_t adr, const char *data);
extern void *SV_GameClientNum(int clientNum);

void SV_UpdateServerCommandsToClient(client_t *client, msg_t *msg);
static cachedSnapshot_t *__attribute_regparm__(1) SV_GetCachedSnapshotInternal(int archivedFrame);
void SV_ArchiveSnapshot(void);
void SV_SendMessageToClient(msg_t *msg, client_t *client);
qboolean SV_GetArchivedClientInfo(int clientNum, int *pArchiveTime, int (*ps)[4], void (*cs)());
Bool SV_GetClientPositionAtTime(int client, int gametime, vec_t *pos);
void SV_SendClientSnapshot(client_t *client);
void SV_SendClientMessages(void);

void SV_UpdateServerCommandsToClient(client_t *client, msg_t *msg)
{
    const dvar_t *showCommands = *(const dvar_t **)imp_sv_debugReliableCmds;
    int reliableAck = client->reliableAcknowledge;
    int i;
    int idx;
    char *cmdStr;

    if (reliableAck + 1 < client->reliableSequence) {
        if (showCommands && showCommands->current.enabled) {
            Com_Printf("Client %s has the following un-ack'd reliable commands:\n", client->name);
            reliableAck = client->reliableAcknowledge;
        }
    }

    for (i = reliableAck + 1; i <= client->reliableSequence; i++) {
        MSG_WriteByte(msg, 4);
        MSG_WriteLong(msg, i);
        idx = i & CLIENT_CMDENTRY_MASK;
        cmdStr = client->reliableCommandInfo[idx].cmd;
        MSG_WriteString(msg, cmdStr);
        if (showCommands && showCommands->current.enabled) {
            Com_Printf("%i: %s\n", i - client->reliableAcknowledge - 1, cmdStr);
        }
    }

    client->reliableSent = client->reliableSequence;
}

static __attribute_regparm__(1)
    cachedSnapshot_t *SV_GetCachedSnapshotInternal(int archivedFrame)
{
    byte msg_buf_ll[16];
    byte *msg_buf;
    msg_t msg;
    serverStatic_t *svs;
    cachedSnapshot_t *cachedFrame = NULL;
    byte *partEntry;

    LargeLocal_LargeLocal(msg_buf_ll, 0x20000);
    msg_buf = LargeLocal_GetBuf(msg_buf_ll);

    svs = (serverStatic_t *)imp_svs;

    if (archivedFrame < 0 || archivedFrame >= svs->nextArchivedSnapshotFrames ||
        archivedFrame < svs->nextArchivedSnapshotFrames - 1200)
        goto return_null;

    partEntry = (byte *)svs->archivedSnapshotFrames + (archivedFrame % 1200) * 8;

    if (*(int *)partEntry < svs->nextArchivedSnapshotBuffer - 0x2000000)
        goto return_null;

    {
        int searchStart = svs->nextCachedSnapshotFrames;
        int searchEnd = searchStart - 0x200;
        byte *cfBase;
        int i;

        if (searchEnd < 0)
            searchEnd = 0;

        if (searchStart - 1 >= searchEnd) {
            cfBase = (byte *)svs->cachedSnapshotFrames;
            for (i = searchStart - 1; i >= searchEnd; i--) {
                cachedSnapshot_t *cf = (cachedSnapshot_t *)(cfBase + (i % 512) * 28);
                if (cf->archivedFrame == archivedFrame) {
                    cachedFrame = cf;
                    svs = (serverStatic_t *)imp_svs;
                    if (cf->first_entity >= svs->nextCachedSnapshotEntities - 0x4000) {
                        if (cf->first_client >= svs->nextCachedSnapshotClients - 0x1000) {
                            goto cleanup;
                        }
                    }
                    break;
                }
            }
        }
    }

    MSG_Init(&msg, msg_buf, 0x20000);
    msg.cursize = *(int *)(partEntry + 4);
    if (msg.cursize <= 0 || msg.cursize > msg.maxsize ||
        *(int *)partEntry < 0 ||
        *(int *)partEntry > svs->nextArchivedSnapshotBuffer - msg.cursize)
        goto return_null;

    {
        int dataOffset = *(int *)partEntry % 0x2000000;
        int remaining = 0x2000000 - dataOffset;
        byte *entBuf;

        if (msg.cursize <= remaining) {
            svs = (serverStatic_t *)imp_svs;
            entBuf = svs->archivedSnapshotBuffer;
            memcpy(msg.data, entBuf + dataOffset, msg.cursize);
        } else {
            svs = (serverStatic_t *)imp_svs;
            entBuf = svs->archivedSnapshotBuffer;
            memcpy(msg.data, entBuf + dataOffset, remaining);
            memcpy(msg.data + remaining, entBuf, msg.cursize - remaining);
        }
    }

    if (MSG_ReadBit(&msg) == 0) {

        int oldArchivedFrame = MSG_ReadLong(&msg);
        cachedSnapshot_t *oldCachedFrame;

        svs = (serverStatic_t *)imp_svs;
        if (msg.overflowed || oldArchivedFrame < 0 || oldArchivedFrame >= archivedFrame ||
            oldArchivedFrame < svs->nextArchivedSnapshotFrames - 0x4b0)
            goto return_null;

        {
            byte *oldPart = (byte *)svs->archivedSnapshotFrames + (oldArchivedFrame % 1200) * 8;
            if (*(int *)oldPart < svs->nextArchivedSnapshotBuffer - 0x2000000)
                goto return_null;
        }

        oldCachedFrame = SV_GetCachedSnapshotInternal(oldArchivedFrame);
        if (oldCachedFrame == NULL)
            goto return_null;

        {
            int frameIdx = svs->nextCachedSnapshotFrames % 512;
            cachedFrame = &svs->cachedSnapshotFrames[frameIdx];
        }
        cachedFrame->archivedFrame = archivedFrame;
        cachedFrame->num_entities = 0;
        cachedFrame->first_entity = svs->nextCachedSnapshotEntities;
        cachedFrame->num_clients = 0;
        cachedFrame->first_client = svs->nextCachedSnapshotClients;
        cachedFrame->usesDelta = 1;
        cachedFrame->time = MSG_ReadLong(&msg);

        {
            int oldNumClients = oldCachedFrame->num_clients;
            byte *oldCachedClient = NULL;
            int oldClientNum = 0x1869f;
            int oldindex = 0;

            if (oldNumClients > 0) {
                int firstIdx = oldCachedFrame->first_client % 0x1000;
                oldCachedClient = (byte *)svs->cachedSnapshotClients + firstIdx * CACHEDCLIENT_STRIDE;
                oldClientNum = *(int *)(oldCachedClient + 4);
            }

            while (MSG_ReadBit(&msg) != 0) {
                int newClientNum = MSG_ReadBits(&msg, 6);
                int clientIdx;
                byte *newCachedClient;

                if (msg.overflowed || cachedFrame->num_clients >= 64)
                    goto return_null;

                while (oldClientNum < newClientNum) {
                    oldindex++;
                    if (oldindex >= oldNumClients) {
                        oldClientNum = 0x1869f;
                        break;
                    }
                    {
                        svs = (serverStatic_t *)imp_svs;
                        byte *cdata = (byte *)svs->cachedSnapshotClients;
                        int nextIdx = (oldCachedFrame->first_client + oldindex) % 0x1000;
                        oldCachedClient = cdata + nextIdx * CACHEDCLIENT_STRIDE;
                        oldClientNum = *(int *)(oldCachedClient + 4);
                    }
                }

                svs = (serverStatic_t *)imp_svs;
                clientIdx = svs->nextCachedSnapshotClients % 0x1000;
                newCachedClient = (byte *)svs->cachedSnapshotClients + clientIdx * CACHEDCLIENT_STRIDE;

                if (oldClientNum == newClientNum) {

                    MSG_ReadDeltaClient(&msg, oldCachedClient + 4, newCachedClient + 4, newClientNum);
                    *(int *)newCachedClient = MSG_ReadBit(&msg);
                    if (*(int *)newCachedClient != 0)
                        MSG_ReadDeltaPlayerstate(&msg, ((char *)oldCachedClient + offsetof(cachedClient_t, ps.commandTime)), ((char *)newCachedClient + offsetof(cachedClient_t, ps.commandTime)));

                    svs = (serverStatic_t *)imp_svs;
                    svs->nextCachedSnapshotClients += 1;
                    if (svs->nextCachedSnapshotClients > 0x7ffffffd)
                        Com_Error(0, "SV_GetCachedSnapshot: too many clients");

                    cachedFrame->num_clients++;

                    oldindex++;
                    if (oldindex >= oldCachedFrame->num_clients) {
                        oldClientNum = 0x1869f;
                    } else {
                        svs = (serverStatic_t *)imp_svs;
                        byte *cdata = (byte *)svs->cachedSnapshotClients;
                        int nextIdx = (oldCachedFrame->first_client + oldindex) % 0x1000;
                        oldCachedClient = cdata + nextIdx * CACHEDCLIENT_STRIDE;
                        oldClientNum = *(int *)(oldCachedClient + 4);
                    }
                } else {

                    MSG_ReadDeltaClient(&msg, NULL, newCachedClient + 4, newClientNum);
                    *(int *)newCachedClient = MSG_ReadBit(&msg);
                    if (*(int *)newCachedClient != 0)
                        MSG_ReadDeltaPlayerstate(&msg, NULL, ((char *)newCachedClient + offsetof(cachedClient_t, ps.commandTime)));

                    svs = (serverStatic_t *)imp_svs;
                    svs->nextCachedSnapshotClients += 1;
                    if (svs->nextCachedSnapshotClients > 0x7ffffffd)
                        Com_Error(0, "SV_GetCachedSnapshot: too many clients");

                    cachedFrame->num_clients++;
                }
            }
        }
    } else {

        svs = (serverStatic_t *)imp_svs;

        {
            int frameIdx = svs->nextCachedSnapshotFrames % 512;
            cachedFrame = &svs->cachedSnapshotFrames[frameIdx];
        }
        cachedFrame->archivedFrame = archivedFrame;
        cachedFrame->num_entities = 0;
        cachedFrame->first_entity = svs->nextCachedSnapshotEntities;
        cachedFrame->num_clients = 0;
        cachedFrame->first_client = svs->nextCachedSnapshotClients;
        cachedFrame->usesDelta = 0;
        cachedFrame->time = MSG_ReadLong(&msg);

        while (MSG_ReadBit(&msg) != 0) {
            int clientNum = MSG_ReadBits(&msg, 6);
            int clientIdx;
            byte *newCachedClient;

            if (msg.overflowed || cachedFrame->num_clients >= 64)
                goto return_null;

            clientIdx = svs->nextCachedSnapshotClients % 0x1000;
            newCachedClient = (byte *)svs->cachedSnapshotClients + clientIdx * CACHEDCLIENT_STRIDE;

            MSG_ReadDeltaClient(&msg, NULL, newCachedClient + 4, clientNum);
            *(int *)newCachedClient = MSG_ReadBit(&msg);
            if (*(int *)newCachedClient != 0)
                MSG_ReadDeltaPlayerstate(&msg, NULL, ((char *)newCachedClient + offsetof(cachedClient_t, ps.commandTime)));

            svs->nextCachedSnapshotClients += 1;
            if (svs->nextCachedSnapshotClients > 0x7ffffffd)
                Com_Error(0, "SV_GetCachedSnapshot: too many clients");

            cachedFrame->num_clients++;
        }
    }

    while (1) {
        int entNum = MSG_ReadBits(&msg, 10);
        int entIdx;
        byte *archivedEnt;
        server_t *sv;

        /* Bit reads signal exhaustion without advancing readcount. Never let
         * a truncated archive loop forever or index baseline[-1]. */
        if (msg.overflowed || entNum < 0 || entNum > 0x3ff)
            goto return_null;
        if (entNum == 0x3ff)
            break;
        if (cachedFrame->num_entities >= 1023)
            goto return_null;

        svs = (serverStatic_t *)imp_svs;
        entIdx = svs->nextCachedSnapshotEntities % 0x4000;
        archivedEnt = (byte *)svs->cachedSnapshotEntities + entIdx * 276;

        sv = (server_t *)imp_sv;
        MSG_ReadDeltaArchivedEntity(&msg, (byte *)&sv->svEntities[entNum].baseline, archivedEnt, entNum);

        svs->nextCachedSnapshotEntities += 1;
        if (svs->nextCachedSnapshotEntities > 0x7ffffffd)
            Com_Error(0, "SV_GetCachedSnapshot: too many entities");

        cachedFrame->num_entities++;
    }

    svs = (serverStatic_t *)imp_svs;
    svs->nextCachedSnapshotFrames += 1;
    if (svs->nextCachedSnapshotFrames > 0x7ffffffd)
        Com_Error(0, "SV_GetCachedSnapshot: too many frames");

    goto cleanup;

return_null:
    cachedFrame = NULL;
cleanup:
    ZN10LargeLocalD1Ev(msg_buf_ll);
    return cachedFrame;
}


static int signedMod512(int x)
{
    int r = x & 0x800001ff;
    if (r < 0) {
        r = ((r - 1) | (int)0xfffffe00) + 1;
    }
    return r;
}

static int signedMod4096(int x)
{
    int r = x & 0x80000fff;
    if (r < 0) {
        r = ((r - 1) | (int)0xfffff000) + 1;
    }
    return r;
}

static int signedMod16384(int x)
{
    int r = x & 0x80003fff;
    if (r < 0) {
        r = ((r - 1) | (int)0xffffc000) + 1;
    }
    return r;
}

static int signedMod33554432(int x)
{
    int r = x & 0x81ffffff;
    if (r < 0) {
        r = ((r - 1) | (int)0xfe000000) + 1;
    }
    return r;
}

void SV_ArchiveSnapshot(void)
{
    byte msg_buf_large_local[24];
    byte *msg_buf;
    byte msg[24];
    serverStatic_t *svs_p = (serverStatic_t *)imp_svs;
    server_t *sv_p = (server_t *)imp_sv;
    byte ps[sizeof(playerState_t)];
    byte archivedEnt[sizeof(archivedEntity_t)];

    LargeLocal_LargeLocal(msg_buf_large_local, 0x20000);
    msg_buf = LargeLocal_GetBuf(msg_buf_large_local);

    if (sv_p->state != 2)
        goto cleanup;

    if (svs_p->archiveEnabled == 0)
        goto cleanup;

    MSG_Init((msg_t *)msg, msg_buf, 0x20000);

    {
        int archivedFrameNum = svs_p->nextCachedSnapshotFrames;
        int oldindex = archivedFrameNum - 0x200;
        int newnum;
        byte *cachedFrames;
        byte *cachedFrame;
        int frameSlot;

        if (oldindex < 0)
            oldindex = 0;

        newnum = svs_p->nextArchivedSnapshotFrames - (*(dvar_t **)(sv_fps_dvar))->current.integer;

        {
            int idx = archivedFrameNum - 1;
            cachedFrames = (byte *)svs_p->cachedSnapshotFrames;

            while (idx >= oldindex) {
                int cfSlot = signedMod512(idx);
                byte *cf = cachedFrames + cfSlot * 28;

                /* Delta bases must stay within the last second. An older base
                 * otherwise persists forever and falls out of the archive ring. */
                if (((cachedSnapshot_t *)cf)->archivedFrame >= newnum) {

                    if (((cachedSnapshot_t *)cf)->usesDelta == 0) {

                        serverStatic_t *svs2 = (serverStatic_t *)imp_svs;
                        if (((cachedSnapshot_t *)cf)->first_entity < svs2->nextCachedSnapshotEntities - 0x4000)
                            break;
                        if (((cachedSnapshot_t *)cf)->first_client < svs2->nextCachedSnapshotClients - 0x1000)
                            break;

                        MSG_WriteBit0((msg_t *)msg);
                        MSG_WriteLong((msg_t *)msg, ((cachedSnapshot_t *)cf)->archivedFrame);
                        MSG_WriteLong((msg_t *)msg, svs2->time);

                        {
                            int to_num_clients = (*(dvar_t **)(sv_maxclients_dvar))->current.integer;
                            int from_num_clients = ((cachedSnapshot_t *)cf)->num_clients;
                            byte *cachedClient = NULL;
                            int newIdx = 0;
                            int oldIdx2 = 0;

                            while (1) {
                                if (newIdx >= to_num_clients) {

                                    while (oldIdx2 < from_num_clients) {
                                        int archClientIdx = oldIdx2 + ((cachedSnapshot_t *)cf)->first_client;
                                        int archSlot = signedMod4096(archClientIdx);
                                        serverStatic_t *svs3 = (serverStatic_t *)imp_svs;
                                        cachedClient = (byte *)svs3->cachedSnapshotClients + archSlot * 9992;

                                        if (*(int *)(cachedClient + 4) >= newIdx)
                                            break;
                                        oldIdx2++;
                                    }
                                    if (oldIdx2 >= from_num_clients)
                                        break;
                                    break;
                                }

                                {
                                    serverStatic_t *svs4 = (serverStatic_t *)imp_svs;
                                    if (svs4->clients[newIdx].state <= 1) {
                                        newIdx++;
                                        continue;
                                    }
                                }

                                if (oldIdx2 < from_num_clients) {
                                    int archClientIdx = oldIdx2 + ((cachedSnapshot_t *)cf)->first_client;
                                    int archSlot = signedMod4096(archClientIdx);
                                    serverStatic_t *svs3 = (serverStatic_t *)imp_svs;
                                    cachedClient = (byte *)svs3->cachedSnapshotClients + archSlot * 9992;
                                    int oldClientNum = *(int *)(cachedClient + 4);

                                    if (oldClientNum == newIdx) {

                                        void *clientState = G_GetClientState(newIdx);
                                        MSG_WriteDeltaClient((msg_t *)msg, cachedClient + 4, (byte *)clientState, 1);

                                        int hasPS = GetFollowPlayerState(newIdx, ps);
                                        if (hasPS) {
                                            MSG_WriteBit1((msg_t *)msg);
                                            MSG_WriteDeltaPlayerstate((msg_t *)msg, ((char *)cachedClient + offsetof(cachedClient_t, ps.commandTime)), ps);
                                        } else {
                                            MSG_WriteBit0((msg_t *)msg);
                                        }
                                        oldIdx2++;
                                        newIdx++;
                                        continue;
                                    }

                                    if (newIdx < oldClientNum) {

                                        void *clientState = G_GetClientState(newIdx);
                                        MSG_WriteDeltaClient((msg_t *)msg, NULL, (byte *)clientState, 1);
                                        int hasPS = GetFollowPlayerState(newIdx, ps);
                                        if (hasPS) {
                                            MSG_WriteBit1((msg_t *)msg);
                                            MSG_WriteDeltaPlayerstate((msg_t *)msg, NULL, ps);
                                        } else {
                                            MSG_WriteBit0((msg_t *)msg);
                                        }
                                        newIdx++;
                                        continue;
                                    }

                                    if (newIdx > oldClientNum) {
                                        oldIdx2++;
                                    }
                                    continue;
                                }

                                {
                                    int fakeOldClientNum = 0x270f;
                                    if (newIdx == fakeOldClientNum) {

                                        void *clientState = G_GetClientState(newIdx);
                                        MSG_WriteDeltaClient((msg_t *)msg, cachedClient + 4, (byte *)clientState, 1);
                                        int hasPS = GetFollowPlayerState(newIdx, ps);
                                        if (hasPS) {
                                            MSG_WriteBit1((msg_t *)msg);
                                            MSG_WriteDeltaPlayerstate((msg_t *)msg, ((char *)cachedClient + offsetof(cachedClient_t, ps.commandTime)), ps);
                                        } else {
                                            MSG_WriteBit0((msg_t *)msg);
                                        }
                                        oldIdx2++;
                                        newIdx++;
                                        continue;
                                    }

                                    void *clientState = G_GetClientState(newIdx);
                                    MSG_WriteDeltaClient((msg_t *)msg, NULL, (byte *)clientState, 1);
                                    int hasPS = GetFollowPlayerState(newIdx, ps);
                                    if (hasPS) {
                                        MSG_WriteBit1((msg_t *)msg);
                                        MSG_WriteDeltaPlayerstate((msg_t *)msg, NULL, ps);
                                    } else {
                                        MSG_WriteBit0((msg_t *)msg);
                                    }
                                    newIdx++;
                                    continue;
                                }
                            }
                        }

                        MSG_WriteBit0((msg_t *)msg);

                        {
                            int numEnts = sv_p->num_entities;
                            int i;

                            if (numEnts > 0) {
                                for (i = 0; i < numEnts; i++) {
                                    byte *gent = SV_GentityNum(i);
                                    if (((gentity_t *)gent)->r.linked == 0)
                                        continue;

                                    int contents = ((gentity_t *)gent)->r.broadcastTime;
                                    if (contents == 0) {

                                        if (!(((gentity_t *)gent)->r.svFlags & 1)) {

                                            svEntity_t *svEnt = (svEntity_t *)SV_SvEntityForGentity(gent);
                                            if (!(((gentity_t *)gent)->r.svFlags & 0x18)) {
                                                if (svEnt->numClusters == 0)
                                                    continue;
                                            }
                                        }
                                    }

                                    memcpy(archivedEnt, gent, sizeof(entityState_t));

                                    {
                                        gentity_t *ge = (gentity_t *)gent;
                                        archivedEntity_t *ae = (archivedEntity_t *)archivedEnt;
                                        ae->r.svFlags = ge->r.svFlags;
                                        if (ge->r.broadcastTime != 0) {
                                            ae->r.svFlags |= 8;
                                        }
                                        ae->r.clientMask[0] = ge->r.clientMask[0];
                                        ae->r.clientMask[1] = ge->r.clientMask[1];

                                        ae->r.absmin[0] = ge->r.absmin[0];
                                        ae->r.absmin[1] = ge->r.absmin[1];
                                        ae->r.absmin[2] = ge->r.absmin[2];

                                        ae->r.absmax[0] = ge->r.absmax[0];
                                        ae->r.absmax[1] = ge->r.absmax[1];
                                        ae->r.absmax[2] = ge->r.absmax[2];
                                    }

                                    {
                                        int entNum = ((gentity_t *)gent)->s.number;
                                        byte *baseline = (byte *)&sv_p->svEntities[entNum].baseline;
                                        MSG_WriteDeltaArchivedEntity((msg_t *)msg, baseline, archivedEnt, 1);
                                    }
                                }
                            }
                        }

                        goto write_frame;
                    }
                    break;
                }
                idx--;
            }
        }
    }

    MSG_WriteBit1((msg_t *)msg);
    {
        serverStatic_t *svs2 = (serverStatic_t *)imp_svs;
        MSG_WriteLong((msg_t *)msg, svs2->time);
    }

    {
        serverStatic_t *svs2 = (serverStatic_t *)imp_svs;
        int frameNum = svs2->nextCachedSnapshotFrames;
        int frameSlot = signedMod512(frameNum);
        byte *cachedFrames = (byte *)svs2->cachedSnapshotFrames;
        cachedSnapshot_t *cachedFrame = (cachedSnapshot_t *)(cachedFrames + frameSlot * 28);

        cachedFrame->archivedFrame = svs2->nextArchivedSnapshotFrames;
        cachedFrame->num_entities = 0;
        cachedFrame->first_entity = svs2->nextCachedSnapshotEntities;
        cachedFrame->num_clients = 0;
        cachedFrame->first_client = svs2->nextCachedSnapshotClients;
        cachedFrame->usesDelta = 0;
        cachedFrame->time = svs2->time;

        {
            byte *maxclients_dvar = *(byte **)&sv_maxclients_dvar;
            byte *maxclients_val;
            int maxClients;
            int c;

            maxclients_val = *(byte **)maxclients_dvar;
            maxClients = *(int *)(maxclients_val + 8);

            for (c = 0; c < maxClients; c++) {

                if (svs2->clients[c].state <= 1)
                    continue;

                {
                    int clientArchIdx = svs2->nextCachedSnapshotClients;
                    int clientSlot = signedMod4096(clientArchIdx);
                    byte *archivedClient = (byte *)svs2->cachedSnapshotClients + clientSlot * 9992;

                    {
                        void *clientState = G_GetClientState(c);
                        memcpy(archivedClient + 4, clientState, sizeof(clientState_t));
                    }

                    MSG_WriteDeltaClient((msg_t *)msg, NULL, archivedClient + 4, 1);

                    {
                        int hasPS = GetFollowPlayerState(c, ((char *)archivedClient + offsetof(cachedClient_t, ps.commandTime)));
                        *(int *)archivedClient = hasPS;

                        if (hasPS) {
                            MSG_WriteBit1((msg_t *)msg);
                            MSG_WriteDeltaPlayerstate((msg_t *)msg, NULL, ((char *)archivedClient + offsetof(cachedClient_t, ps.commandTime)));
                        } else {
                            MSG_WriteBit0((msg_t *)msg);
                        }
                    }

                    {
                        serverStatic_t *svs3 = (serverStatic_t *)imp_svs;
                        int newClientIdx = svs3->nextCachedSnapshotClients + 1;
                        svs3->nextCachedSnapshotClients = newClientIdx;
                        if (newClientIdx > 0x7ffffffd) {
                            Com_Error(0, "\x15svs.nextCachedSnapshotClients wrapped");
                        }
                    }

                    cachedFrame = &svs.cachedSnapshotFrames[frameSlot];
                    cachedFrame->num_clients += 1;

                    maxclients_val = *(byte **)(*(byte **)&sv_maxclients_dvar);
                }
            }
        }

        MSG_WriteBit0((msg_t *)msg);

        {
            int numEnts = sv_p->num_entities;
            int i;

            if (numEnts > 0) {
                for (i = 0; i < numEnts; i++) {
                    byte *gent = SV_GentityNum(i);
                    if (((gentity_t *)gent)->r.linked == 0)
                        continue;

                    int entityContents = ((gentity_t *)gent)->r.broadcastTime;
                    if (entityContents == 0) {
                        if (!(((gentity_t *)gent)->r.svFlags & 1)) {
                            svEntity_t *svEnt = (svEntity_t *)SV_SvEntityForGentity(gent);
                            if (!(((gentity_t *)gent)->r.svFlags & 0x18)) {
                                if (svEnt->numClusters == 0)
                                    continue;
                            }
                        }
                    }

                    {
                        serverStatic_t *svs4 = (serverStatic_t *)imp_svs;
                        int entArchIdx = svs4->nextCachedSnapshotEntities;
                        int entSlot = signedMod16384(entArchIdx);

                        byte *archivedEntSlot = (byte *)svs4->cachedSnapshotEntities + entSlot * 276;

                        memcpy(archivedEntSlot, gent, sizeof(entityState_t));

                        {
                            gentity_t *ge = (gentity_t *)gent;
                            archivedEntity_t *ae = (archivedEntity_t *)archivedEntSlot;
                            ae->r.svFlags = ge->r.svFlags;
                            if (ge->r.broadcastTime != 0) {
                                ae->r.svFlags |= 8;
                            }
                            ae->r.clientMask[0] = ge->r.clientMask[0];
                            ae->r.clientMask[1] = ge->r.clientMask[1];

                            ae->r.absmin[0] = ge->r.absmin[0];
                            ae->r.absmin[1] = ge->r.absmin[1];
                            ae->r.absmin[2] = ge->r.absmin[2];

                            ae->r.absmax[0] = ge->r.absmax[0];
                            ae->r.absmax[1] = ge->r.absmax[1];
                            ae->r.absmax[2] = ge->r.absmax[2];
                        }

                        {
                            int entNum = ((gentity_t *)gent)->s.number;
                            byte *baseline = (byte *)&sv_p->svEntities[entNum].baseline;
                            MSG_WriteDeltaArchivedEntity((msg_t *)msg, baseline, archivedEntSlot, 1);
                        }

                        {
                            serverStatic_t *svs5 = (serverStatic_t *)imp_svs;
                            int newEntIdx = svs5->nextCachedSnapshotEntities + 1;
                            svs5->nextCachedSnapshotEntities = newEntIdx;
                            if (newEntIdx > 0x7ffffffd) {
                                Com_Error(0, "\x15svs.nextCachedSnapshotEntities wrapped");
                            }
                        }

                        {
                            cachedSnapshot_t *cf2 = &svs.cachedSnapshotFrames[frameSlot];
                            cf2->num_entities += 1;
                        }
                    }
                }
            }
        }

        {
            serverStatic_t *svs5 = (serverStatic_t *)imp_svs;
            int newFrameNum = svs5->nextCachedSnapshotFrames + 1;
            svs5->nextCachedSnapshotFrames = newFrameNum;
            if (newFrameNum > 0x7ffffffd) {
                Com_Error(0, "\x15svs.nextCachedSnapshotFrames wrapped");
            }
        }
    }

write_frame:

    MSG_WriteBits((msg_t *)msg, 0x3ff, 10);

    {
        int overflowed = *(int *)msg;
        if (overflowed != 0) {
            Com_DPrintf("SV_ArchiveSnapshot: ignoring snapshot because it overflowed.\n");
            goto cleanup;
        }
    }

    {
        serverStatic_t *svs2 = (serverStatic_t *)imp_svs;
        int archivedFrameCount = svs2->nextArchivedSnapshotFrames;
        int bufIndex;
        byte *entParts;
        int bufOffset;
        int msgDataLen;
        int bufSize;
        int remaining;

        bufIndex = archivedFrameCount % 1200;

        entParts = (byte *)svs2->archivedSnapshotFrames;
        byte *partEntry = entParts + bufIndex * 8;

        bufOffset = svs2->nextArchivedSnapshotBuffer;
        *(int *)partEntry = bufOffset;

        msgDataLen = ((msg_t *)msg)->cursize;
        *(int *)(partEntry + 4) = msgDataLen;

        bufSize = 0x2000000;
        remaining = signedMod33554432(bufOffset);

        if (remaining + msgDataLen <= bufSize) {

            byte *archBuf = svs2->archivedSnapshotBuffer;
            byte *msgData = ((msg_t *)msg)->data;
            memcpy(archBuf + remaining, msgData, msgDataLen);
        } else {

            int firstPart = bufSize - remaining;
            byte *archBuf = svs2->archivedSnapshotBuffer;
            byte *msgData = ((msg_t *)msg)->data;

            memcpy(archBuf + remaining, msgData, firstPart);
            memcpy(archBuf, msgData + firstPart, msgDataLen - firstPart);
        }

        /* Reserve each frame's bytes. Reusing offset zero corrupts every
         * older archive once its decoded cache entry has been evicted. */
        svs2->nextArchivedSnapshotBuffer = bufOffset + msgDataLen;
    }

    {
        serverStatic_t *svs2 = (serverStatic_t *)imp_svs;
        int newCount = svs2->nextArchivedSnapshotFrames + 1;
        svs2->nextArchivedSnapshotFrames = newCount;
        if (newCount > 0x7ffffffd) {
            Com_Error(0, "\x15svs.nextArchivedSnapshotFrames wrapped");
        }
    }

cleanup:
    ZN10LargeLocalD1Ev(msg_buf_large_local);
}

void SV_SendMessageToClient(msg_t *msg, client_t *client)
{
    serverStatic_t *psvs;
    byte compressedBuf_ll[24];
    byte *compressedBuf;
    int compressedSize;
    int idx;
    int rateMsec;
    int rate;
    int messageSize;
    int svsTime;

    LargeLocal_LargeLocal(compressedBuf_ll, MAX_MSGLEN);
    compressedBuf = LargeLocal_GetBuf(compressedBuf_ll);

    *(int *)compressedBuf = *(int *)msg->data;

    compressedSize = MSG_WriteBitsCompress(msg->data + 4, compressedBuf + 4, msg->cursize - 4) + 4;

    if (client->dropReason != NULL) {
        SV_DropClient(client, client->dropReason);
    }

    idx = client->netchan.outgoingSequence & 0x1f;
    {
        clientSnapshot_t *frame = &client->frames[idx];
        frame->messageSize = compressedSize;
        psvs = (serverStatic_t *)imp_svs;
        frame->messageSent = psvs->time;
        frame->messageAcked = -1;
    }

    SV_Netchan_Transmit(client, compressedSize, compressedBuf);

    {
        netadr_t *addr = &client->netchan.remoteAddress;
        if (addr->type == 2 ||
            Sys_IsLANAddress(*addr)) {

            client->nextSnapshotTime = psvs->time - 1;
            ZN10LargeLocalD1Ev(compressedBuf_ll);
            return;
        }
    }

    messageSize = compressedSize;
    if (messageSize > 0x5dc)
        messageSize = 0x5dc;

    rate = client->rate;
    {
        const dvar_t *minPingRate = (*(dvar_t **)(sv_minPingRate_dvar));
        if (minPingRate->current.integer != 0) {
            if (minPingRate->current.integer <= 0x3e7) {
                Dvar_SetInt(minPingRate, 0x3e8);
                minPingRate = (*(dvar_t **)(sv_minPingRate_dvar));
            }
            if (rate > minPingRate->current.integer)
                rate = minPingRate->current.integer;
        }
    }

    rateMsec = (messageSize * 5 * 5 * 5 * 8 + 0xbb80) / rate;

    if ((*(dvar_t **)(showpackets_dvar))->current.enabled != 0) {
        Com_Printf("It would take %ims to send %i bytes to client %s (rate %i)\n",
                   rateMsec, messageSize, client->name, client->rate);
    }

    if (rateMsec < client->snapshotMsec) {
        client->rateDelayed = 0;
        rateMsec = client->snapshotMsec;
    } else {
        client->rateDelayed = 1;
    }

    psvs = (serverStatic_t *)imp_svs;
    svsTime = psvs->time;
    client->nextSnapshotTime = rateMsec + svsTime;

    if (client->state != 4) {
        if (client->downloadName[0] == 0) {
            if (rateMsec + svsTime < svsTime + 0x3e8) {
                client->nextSnapshotTime = svsTime + 0x3e8;
            }
        }
    }

    sv.bpsTotalBytes += compressedSize;

    ZN10LargeLocalD1Ev(compressedBuf_ll);
}

qboolean SV_GetArchivedClientInfo(int clientNum, int *pArchiveTime, int (*ps)[4], void (*cs)())
{
    serverStatic_t *svs = (serverStatic_t *)imp_svs;
    cachedSnapshot_t *snap = NULL;
    int archivedFrame = 0;

    if (svs->archiveEnabled != 0 && *pArchiveTime > 0) {
        int archivedSnapshotCount = svs->nextArchivedSnapshotFrames;
        int rate = (*(dvar_t **)(sv_fps_dvar))->current.integer;
        int minFrame;

        if (rate <= 0)
            return 0;

        archivedFrame = archivedSnapshotCount - (*pArchiveTime * rate / 1000);
        minFrame = archivedSnapshotCount - 0x4b0;

        if (archivedFrame < minFrame) {
            *pArchiveTime = (archivedSnapshotCount - minFrame) * 1000 / rate;
            archivedFrame = minFrame;
        }

        if (archivedFrame < 0) {
            *pArchiveTime = archivedSnapshotCount * 1000 / rate;
            archivedFrame = 0;
        }

        if (svs->nextArchivedSnapshotFrames > archivedFrame) {
            serverStatic_t *svsReload = (serverStatic_t *)imp_svs;
            while (archivedFrame < svsReload->nextArchivedSnapshotFrames) {
                snap = SV_GetCachedSnapshotInternal(archivedFrame);
                if (snap != NULL)
                    break;
                archivedFrame++;
            }
        }

        if (snap == NULL) {
            *pArchiveTime = 0;
        }
    }

    if (snap != NULL) {

        serverStatic_t *svs2 = (serverStatic_t *)imp_svs;
        int deltaTime = svs2->time - snap->time;
        int numClients = snap->num_clients;
        byte *cachedClientsBase;
        int firstIndex;
        byte *cachedClient = NULL;
        byte *psBytes;
        int i;

        /* Report the actual sample age, including cache gaps and clamping. */
        *pArchiveTime = deltaTime;

        if (numClients <= 0)
            return 0;

        cachedClientsBase = (byte *)svs2->cachedSnapshotClients;
        firstIndex = snap->first_client;

        for (i = 0; i < numClients; i++) {
            int idx = (firstIndex + i) % 0x1000;
            byte *cc = cachedClientsBase + idx * CACHEDCLIENT_STRIDE;
            if (*(int *)(cc + 4) == clientNum) {
                cachedClient = cc;
                break;
            }
        }

        if (cachedClient == NULL || *(int *)cachedClient == 0)
            return 0;

        memcpy(ps, ((char *)cachedClient + offsetof(cachedClient_t, ps.commandTime)), sizeof(playerState_t));
        memcpy(cs, cachedClient + 4, sizeof(clientState_t));

        psBytes = (byte *)ps;

        {
            playerState_t *pps = (playerState_t *)psBytes;
            if (pps->commandTime != 0)
                pps->commandTime += deltaTime;
            if (pps->pm_time != 0)
                pps->pm_time += deltaTime;
            if (pps->foliageSoundTime != 0)
                pps->foliageSoundTime += deltaTime;
            if (pps->jumpTime != 0)
                pps->jumpTime += deltaTime;
            if (pps->legsTimer != 0)
                pps->legsTimer += deltaTime;
            if (pps->adsDelayTime != 0)
                pps->adsDelayTime += deltaTime;
            if (pps->viewHeightLerpTime != 0)
                pps->viewHeightLerpTime += deltaTime;
            if (pps->shellshockTime != 0)
                pps->shellshockTime += deltaTime;
        }

        {
            hudelem_t *hud = ((playerState_t *)psBytes)->hud.archival;
            serverStatic_t *svsPtr = (serverStatic_t *)imp_svs;
            for (i = 0; i < 31; i++) {
                if (hud[i].time != 0)
                    hud[i].time += deltaTime;
                if (hud[i].fadeStartTime != 0) {
                    hud[i].fadeStartTime += deltaTime;
                    if (hud[i].fadeStartTime > svsPtr->time)
                        hud[i].fadeStartTime = svsPtr->time;
                }
                if (hud[i].scaleStartTime != 0)
                    hud[i].scaleStartTime += deltaTime;
                if (hud[i].moveStartTime != 0)
                    hud[i].moveStartTime += deltaTime;
            }
        }

        ((playerState_t *)psBytes)->deltaTime += deltaTime;
        return 1;
    }

    if (*pArchiveTime > 0)
        return 0;

    {
        client_t *client = &svs->clients[clientNum];

        if (client->state != 4)
            return 0;

        if (!GetFollowPlayerState(clientNum, (byte *)ps))
            return 0;

        memcpy(cs, G_GetClientState(clientNum), sizeof(clientState_t));
        return 1;
    }
}

Bool SV_GetClientPositionAtTime(int clientNum, int gametime, vec_t *pos)
{
    serverStatic_t *svs = (serverStatic_t *)imp_svs;
    int msPerFrame;
    int targetAge;
    int startTime, endTime;
    int timeRequest;
    int i;
    byte foundStart, foundEnd;
    float startPos[3], endPos[3];
    float endWeight;
    playerState_t ps;
    clientState_t cs;

    i = (*(dvar_t **)(sv_fps_dvar))->current.integer;
    if (i <= 0 || i > 1000)
        return 0;
    msPerFrame = 1000 / i;

    targetAge = svs->time - gametime;
    if (targetAge < 0)
        targetAge = 0;

    startTime = ((targetAge + msPerFrame - 1) / msPerFrame) * msPerFrame;
    endTime = (targetAge / msPerFrame) * msPerFrame;

    timeRequest = startTime;
    foundStart = 0;
    for (i = 10; i > 0; i--) {
        if (SV_GetArchivedClientInfo(clientNum, &timeRequest, (int (*)[4])&ps, (void (*)())&cs)) {
            startTime = timeRequest;
            memcpy(startPos, ps.origin, sizeof(startPos));
            foundStart = 1;
            break;
        }
        timeRequest += msPerFrame;
    }

    timeRequest = endTime;
    foundEnd = 0;
    for (i = 10; i > 0; i--) {
        if (SV_GetArchivedClientInfo(clientNum, &timeRequest, (int (*)[4])&ps, (void (*)())&cs)) {
            endTime = timeRequest;
            memcpy(endPos, ps.origin, sizeof(endPos));
            foundEnd = 1;
            break;
        }
        timeRequest -= msPerFrame;
    }

    if (!foundStart && !foundEnd)
        return 0;
    if (!foundStart) {
        memcpy(pos, endPos, sizeof(endPos));
        return 1;
    }
    if (!foundEnd || startTime == endTime) {
        memcpy(pos, startPos, sizeof(startPos));
        return 1;
    }

    endWeight = (float)(startTime - targetAge) / (float)(startTime - endTime);
    if (endWeight < 0.0f) endWeight = 0.0f;
    if (endWeight > 1.0f) endWeight = 1.0f;
    for (i = 0; i < 3; i++) {
        pos[i] = startPos[i] + endWeight * (endPos[i] - startPos[i]);
    }
    return 1;
}

#    define SV_SVC_SERVERCOMMAND 4
#    define SV_SVC_SNAPSHOT 6
#    define SV_SVC_EOF 7
#    define SV_MAX_SNAPSHOT_ENTITIES 1024
#    define SV_ENTITY_BASELINE_NUMBER 9999
#    define SV_CLIENT_BASELINE_NUMBER 99999
#    define SV_ARCHIVED_ENTITY_STRIDE 276
#    define SV_CACHED_CLIENT_STRIDE 9992

static int SV_SignedModSnapshotEntLocal(int value, int modulus)
{
    int r;
    if (modulus <= 0) {
        return 0;
    }
    r = value % modulus;
    if (r < 0) {
        r += modulus;
    }
    return r;
}

static clientSnapshot_t *SV_ClientFrameLocal(client_t *client, int sequence)
{
    return &client->frames[sequence & 31];
}

static entityState_t *SV_SnapshotEntityLocal(serverStatic_t *svs, int index)
{
    int slot = SV_SignedModSnapshotEntLocal(index, svs->numSnapshotEntities);
    return &svs->snapshotEntities[slot];
}

static clientState_t *SV_SnapshotClientLocal(serverStatic_t *svs, int index)
{
    int slot = SV_SignedModSnapshotEntLocal(index, svs->numSnapshotClients);
    return (clientState_t *)((byte *)svs->snapshotClients + slot * sizeof(clientState_t));
}

static archivedEntity_t *SV_CachedEntityLocal(serverStatic_t *svs, int index)
{
    int slot = index & 0x3fff;
    return (archivedEntity_t *)((byte *)svs->cachedSnapshotEntities + slot * SV_ARCHIVED_ENTITY_STRIDE);
}

static byte *SV_CachedClientLocal(serverStatic_t *svs, int index)
{
    int slot = index & 0xfff;
    return (byte *)svs->cachedSnapshotClients + slot * SV_CACHED_CLIENT_STRIDE;
}

static int SV_ClientIndexLocal(client_t *client)
{
    serverStatic_t *svs = (serverStatic_t *)imp_svs;
    return (int)(client - svs->clients);
}

static int SV_DvarIntLocal(byte *dvarStorage)
{

    const dvar_t *dvar = *(const dvar_t **)dvarStorage;
    if (!dvar) {
        return 0;
    }
    return dvar->current.integer;
}

static void SV_AddSnapshotEntityNumberLocal(snapshotEntityNumbers_t *entityNumbers, int entnum)
{
    if (entityNumbers->numSnapshotEntities == SV_MAX_SNAPSHOT_ENTITIES) {
        return;
    }
    entityNumbers->snapshotEntities[entityNumbers->numSnapshotEntities++] = entnum;
}

static int SV_BitVectorTestLocal(const byte *bits, int bit)
{
    return (bits[bit >> 3] >> (bit & 7)) & 1;
}

static int SV_EntityIsVisibleToClientLocal(byte *entBytes, const vec3_t org, const byte *pvs, float fogOpaqueDistSqrd, int clientNum)
{
    entityState_t *es = (entityState_t *)entBytes;
    entityShared_t *shared = (entityShared_t *)(entBytes + sizeof(entityState_t));
    svEntity_t *svEnt;
    int i;

    if (!shared->linked) {
        return 0;
    }
    if (es->number == clientNum) {
        return 0;
    }
    if (shared->broadcastTime) {
        serverStatic_t *svs = (serverStatic_t *)imp_svs;
        if (shared->broadcastTime < svs->time) {
            shared->broadcastTime = 0;
        }
        return 1;
    }
    if (shared->svFlags & 1) {
        return 0;
    }
    if ((shared->clientMask[clientNum >> 5] >> (clientNum & 31)) & 1) {
        return 0;
    }
    if (shared->svFlags & 0x18) {
        return 1;
    }

    svEnt = (svEntity_t *)SV_SvEntityForGentity(entBytes);
    if (svEnt->numClusters <= 0) {
        return 0;
    }
    for (i = 0; i < svEnt->numClusters && i < 16; ++i) {
        if (SV_BitVectorTestLocal(pvs, svEnt->clusternums[i])) {
            break;
        }
    }
    if (i == svEnt->numClusters) {
        int cluster;
        if (!svEnt->lastCluster) {
            return 0;
        }
        for (cluster = svEnt->clusternums[0] + 1; cluster <= svEnt->lastCluster; ++cluster) {
            if (SV_BitVectorTestLocal(pvs, cluster)) {
                break;
            }
        }
        if (cluster == svEnt->lastCluster) {
            return 0;
        }
    }

    if (fogOpaqueDistSqrd != 0.0f && BoxDistSqrdExceeds((byte *)shared->absmin, (byte *)shared->absmax, (byte *)org, fogOpaqueDistSqrd)) {
        return 0;
    }
    return 1;
}

static int SV_CachedEntityIsVisibleLocal(archivedEntity_t *archEnt, const vec3_t org, const byte *pvs, float fogOpaqueDistSqrd, int clientNum)
{
    entityState_t *es = &archEnt->s;
    archivedEntityShared_t *shared = &archEnt->r;
    int leafs[128];
    int lastLeaf = 0;
    int numLeafs;
    int i;

    if ((shared->clientMask[clientNum >> 5] >> (clientNum & 31)) & 1) {
        return 0;
    }
    if (es->number == clientNum) {
        return 0;
    }
    if (shared->svFlags & 0x18) {
        return 1;
    }

    numLeafs = CM_BoxLeafnums((byte *)shared->absmin, (byte *)shared->absmax, leafs, 128, &lastLeaf);
    if (!numLeafs) {
        return 0;
    }
    for (i = 0; i < numLeafs; ++i) {
        int cluster = CM_LeafCluster(leafs[i]);
        if (cluster != -1 && SV_BitVectorTestLocal(pvs, cluster)) {
            break;
        }
    }
    if (i == numLeafs) {
        return 0;
    }
    if (fogOpaqueDistSqrd != 0.0f && BoxDistSqrdExceeds((byte *)shared->absmin, (byte *)shared->absmax, (byte *)org, fogOpaqueDistSqrd)) {
        return 0;
    }
    return 1;
}

static void SV_CopyEntityToSnapshotLocal(clientSnapshot_t *frame, const byte *src)
{
    serverStatic_t *svs = (serverStatic_t *)imp_svs;
    entityState_t *dst = SV_SnapshotEntityLocal(svs, svs->nextSnapshotEntities);
    memcpy(dst, src, sizeof(entityState_t));
    svs->nextSnapshotEntities++;
    if (svs->nextSnapshotEntities > 0x7ffffffd) {
        Com_Error(0, "\x15svs.nextSnapshotEntities wrapped");
    }
    frame->num_entities++;
}

static void SV_CopyCachedEntityToSnapshotLocal(clientSnapshot_t *frame, archivedEntity_t *src, int deltaTime)
{
    entityState_t tmp;
    memcpy(&tmp, &src->s, sizeof(tmp));
    if (tmp.pos.trTime) {
        tmp.pos.trTime += deltaTime;
    }
    if (tmp.apos.trTime) {
        tmp.apos.trTime += deltaTime;
    }
    if (tmp.time) {
        tmp.time += deltaTime;
    }
    if (tmp.time2) {
        tmp.time2 += deltaTime;
    }
    SV_CopyEntityToSnapshotLocal(frame, (byte *)&tmp);
}

static void SV_CopyCurrentClientsToSnapshotLocal(clientSnapshot_t *frame)
{
    serverStatic_t *svs = (serverStatic_t *)imp_svs;
    int maxClients = SV_DvarIntLocal((byte *)sv_maxclients_dvar);
    int clientNum;

    for (clientNum = 0; clientNum < maxClients; ++clientNum) {
        client_t *client = &svs->clients[clientNum];
        clientState_t *dst;
        clientState_t *src;
        if (client->state <= 1) {
            continue;
        }
        dst = SV_SnapshotClientLocal(svs, svs->nextSnapshotClients);
        src = (clientState_t *)G_GetClientState(clientNum);
        memcpy(dst, src, sizeof(clientState_t));
        if (dst->clientIndex != clientNum) {
            continue;
        }
        svs->nextSnapshotClients++;
        if (svs->nextSnapshotClients > 0x7ffffffd) {
            Com_Error(0, "\x15svs.nextSnapshotClients wrapped");
        }
        frame->num_clients++;
    }
}

static void SV_CopyCachedClientsToSnapshotLocal(clientSnapshot_t *frame, const cachedSnapshot_t *cachedFrame)
{
    serverStatic_t *svs = (serverStatic_t *)imp_svs;
    int i;

    for (i = 0; i < cachedFrame->num_clients; ++i) {
        byte *cachedClient = SV_CachedClientLocal(svs, cachedFrame->first_client + i);
        clientState_t *dst = SV_SnapshotClientLocal(svs, svs->nextSnapshotClients);
        memcpy(dst, cachedClient + 4, sizeof(clientState_t));
        svs->nextSnapshotClients++;
        if (svs->nextSnapshotClients > 0x7ffffffd) {
            Com_Error(0, "\x15svs.nextSnapshotClients wrapped");
        }
        frame->num_clients++;
    }
}

static cachedSnapshot_t *SV_FindArchivedSnapshotForClientLocal(int clientNum, int *archiveTime)
{
    serverStatic_t *svs = (serverStatic_t *)imp_svs;
    int requestedArchiveTime;
    int fps;
    int archivedFrame;
    int oldestFrame;

    *archiveTime = 0;
    if (!svs->archiveEnabled) {
        return NULL;
    }

    requestedArchiveTime = G_GetClientArchiveTime(clientNum);
    if (requestedArchiveTime <= 0) {
        return NULL;
    }

    fps = SV_DvarIntLocal((byte *)sv_fps_dvar);
    if (fps <= 0) {
        return NULL;
    }

    archivedFrame = svs->nextArchivedSnapshotFrames - (requestedArchiveTime * fps) / 1000;
    oldestFrame = svs->nextArchivedSnapshotFrames - 1200;
    if (archivedFrame < oldestFrame) {
        archivedFrame = oldestFrame;
        requestedArchiveTime = ((svs->nextArchivedSnapshotFrames - archivedFrame) * 1000) / fps;
    }
    if (archivedFrame < 0) {
        archivedFrame = 0;
        requestedArchiveTime = (svs->nextArchivedSnapshotFrames * 1000) / fps;
    }

    while (archivedFrame < svs->nextArchivedSnapshotFrames) {
        cachedSnapshot_t *cachedFrame = SV_GetCachedSnapshotInternal(archivedFrame);
        if (cachedFrame) {
            *archiveTime = requestedArchiveTime;
            return cachedFrame;
        }
        ++archivedFrame;
    }

    return NULL;
}

static void SV_BuildVisibleCachedEntitiesLocal(clientSnapshot_t *frame, cachedSnapshot_t *cachedFrame,
                                               const vec3_t org, int clientNum, int deltaTime)
{
    serverStatic_t *svs = (serverStatic_t *)imp_svs;
    snapshotEntityNumbers_t entityNumbers;
    byte *pvs;
    float fogOpaqueDistSqrd;
    int cluster;
    int e;

    entityNumbers.numSnapshotEntities = 0;
    cluster = CM_LeafCluster(CM_PointLeafnum((byte *)org));
    if (cluster >= 0) {
        pvs = CM_ClusterPVS(cluster);
        fogOpaqueDistSqrd = G_GetFogOpaqueDistSqrd();
        if (fogOpaqueDistSqrd == 3.4028234663852886e+38f) {
            fogOpaqueDistSqrd = 0.0f;
        }
        for (e = 0; e < cachedFrame->num_entities; ++e) {
            archivedEntity_t *archEnt = SV_CachedEntityLocal(svs, cachedFrame->first_entity + e);
            if (SV_CachedEntityIsVisibleLocal(archEnt, org, pvs, fogOpaqueDistSqrd, clientNum)) {
                SV_AddSnapshotEntityNumberLocal(&entityNumbers, e);
            }
        }
    }

    for (e = 0; e < entityNumbers.numSnapshotEntities; ++e) {
        archivedEntity_t *archEnt = SV_CachedEntityLocal(svs, cachedFrame->first_entity + entityNumbers.snapshotEntities[e]);
        SV_CopyCachedEntityToSnapshotLocal(frame, archEnt, deltaTime);
    }
}

static void SV_BuildVisibleCurrentEntitiesLocal(clientSnapshot_t *frame, const vec3_t org, int clientNum)
{
    server_t *sv = (server_t *)imp_sv;
    snapshotEntityNumbers_t entityNumbers;
    byte *pvs;
    float fogOpaqueDistSqrd;
    int cluster;
    int entnum;

    entityNumbers.numSnapshotEntities = 0;
    cluster = CM_LeafCluster(CM_PointLeafnum((byte *)org));
    if (cluster >= 0) {
        pvs = CM_ClusterPVS(cluster);
        fogOpaqueDistSqrd = G_GetFogOpaqueDistSqrd();
        if (fogOpaqueDistSqrd == 3.4028234663852886e+38f) {
            fogOpaqueDistSqrd = 0.0f;
        }
        for (entnum = 0; entnum < sv->num_entities; ++entnum) {
            byte *ent = SV_GentityNum(entnum);
            if (SV_EntityIsVisibleToClientLocal(ent, org, pvs, fogOpaqueDistSqrd, clientNum)) {
                SV_AddSnapshotEntityNumberLocal(&entityNumbers, entnum);
            }
        }
    }

    for (entnum = 0; entnum < entityNumbers.numSnapshotEntities; ++entnum) {
        byte *ent = SV_GentityNum(entityNumbers.snapshotEntities[entnum]);
        SV_CopyEntityToSnapshotLocal(frame, ent);
    }
}

static void SV_BuildClientSnapshotLocal(client_t *client)
{
    serverStatic_t *svs = (serverStatic_t *)imp_svs;
    server_t *sv = (server_t *)imp_sv;
    clientSnapshot_t *frame = SV_ClientFrameLocal(client, client->netchan.outgoingSequence);
    cachedSnapshot_t *cachedFrame = NULL;
    int archiveTime = 0;
    int deltaTime = 0;
    int clientNum;
    vec3_t org;

    frame->num_entities = 0;
    frame->num_clients = 0;
    frame->first_entity = svs->nextSnapshotEntities;
    frame->first_client = svs->nextSnapshotClients;

    if (!client->gentity || client->state == 1 || sv->state != 2) {
        return;
    }

    clientNum = SV_ClientIndexLocal(client);
    cachedFrame = SV_FindArchivedSnapshotForClientLocal(clientNum, &archiveTime);
    G_SetClientArchiveTime(clientNum, archiveTime);
    if (cachedFrame) {
        deltaTime = svs->time - cachedFrame->time;
    }

    memcpy(&frame->ps, SV_GameClientNum(clientNum), sizeof(playerState_t));
    clientNum = frame->ps.clientNum;
    if (clientNum > 1023) {
        Com_Error(1, "\x15SV_BuildClientSnapshot: bad gEnt");
    }

    org[0] = frame->ps.origin[0];
    org[1] = frame->ps.origin[1];
    org[2] = frame->ps.origin[2] + frame->ps.viewHeightCurrent;
    AddLeanToPosition(org, frame->ps.viewangles[1], frame->ps.leanf, 16.0f, 20.0f);

    if (cachedFrame) {
        SV_BuildVisibleCachedEntitiesLocal(frame, cachedFrame, org, clientNum, deltaTime);
        SV_CopyCachedClientsToSnapshotLocal(frame, cachedFrame);
    } else {
        SV_BuildVisibleCurrentEntitiesLocal(frame, org, clientNum);
        SV_CopyCurrentClientsToSnapshotLocal(frame);
    }
}

static void SV_WritePacketEntitiesLocal(msg_t *msg, const clientSnapshot_t *from, const clientSnapshot_t *to)
{
    serverStatic_t *svs = (serverStatic_t *)imp_svs;
    server_t *sv = (server_t *)imp_sv;
    int oldindex = 0;
    int newindex = 0;
    int oldnum;
    int newnum;
    entityState_t *oldent = NULL;
    entityState_t *newent = NULL;

    while (newindex < to->num_entities || oldindex < (from ? from->num_entities : 0)) {
        if (newindex < to->num_entities) {
            newent = SV_SnapshotEntityLocal(svs, to->first_entity + newindex);
            newnum = newent->number;
        } else {
            newent = NULL;
            newnum = SV_ENTITY_BASELINE_NUMBER;
        }

        if (from && oldindex < from->num_entities) {
            oldent = SV_SnapshotEntityLocal(svs, from->first_entity + oldindex);
            oldnum = oldent->number;
        } else {
            oldent = NULL;
            oldnum = SV_ENTITY_BASELINE_NUMBER;
        }

        if (newnum == oldnum) {
            MSG_WriteDeltaEntity(msg, (byte *)oldent, (byte *)newent, 0);
            ++oldindex;
            ++newindex;
        } else if (newnum < oldnum) {
            MSG_WriteDeltaEntity(msg, (byte *)&sv->svEntities[newnum].baseline, (byte *)newent, 1);
            ++newindex;
        } else {
            MSG_WriteDeltaEntity(msg, (byte *)oldent, NULL, 1);
            ++oldindex;
        }
    }

    MSG_WriteBits(msg, 1023, 10);
}

static void SV_WritePacketClientsLocal(msg_t *msg, const clientSnapshot_t *from, const clientSnapshot_t *to)
{
    serverStatic_t *svs = (serverStatic_t *)imp_svs;
    int oldindex = 0;
    int newindex = 0;
    int oldnum;
    int newnum;
    clientState_t *oldclient = NULL;
    clientState_t *newclient = NULL;

    while (newindex < to->num_clients || oldindex < (from ? from->num_clients : 0)) {
        if (newindex < to->num_clients) {
            newclient = SV_SnapshotClientLocal(svs, to->first_client + newindex);
            newnum = newclient->clientIndex;
        } else {
            newclient = NULL;
            newnum = SV_CLIENT_BASELINE_NUMBER;
        }

        if (from && oldindex < from->num_clients) {
            oldclient = SV_SnapshotClientLocal(svs, from->first_client + oldindex);
            oldnum = oldclient->clientIndex;
        } else {
            oldclient = NULL;
            oldnum = SV_CLIENT_BASELINE_NUMBER;
        }

        if (newnum == oldnum) {
            MSG_WriteDeltaClient(msg, (byte *)oldclient, (byte *)newclient, 0);
            ++oldindex;
            ++newindex;
        } else if (newnum < oldnum) {
            MSG_WriteDeltaClient(msg, NULL, (byte *)newclient, 1);
            ++newindex;
        } else {
            MSG_WriteDeltaClient(msg, (byte *)oldclient, NULL, 1);
            ++oldindex;
        }
    }

    MSG_WriteBit0(msg);
}

static void SV_WriteSnapshotToClientLocal(client_t *client, msg_t *msg)
{
    serverStatic_t *svs = (serverStatic_t *)imp_svs;
    clientSnapshot_t *frame = SV_ClientFrameLocal(client, client->netchan.outgoingSequence);
    clientSnapshot_t *oldframe = NULL;
    int lastframe = 0;
    int snapFlags;

    if (client->deltaMessage > client->gamestateMessageNum && client->state == 4) {
        lastframe = client->netchan.outgoingSequence - client->deltaMessage;
        if (lastframe > 28) {
            Com_DPrintf("%s: Delta request from out of date packet.\n", client->name);
            lastframe = 0;
        } else {
            oldframe = SV_ClientFrameLocal(client, client->deltaMessage);
            if (oldframe->first_entity < svs->nextSnapshotEntities - svs->numSnapshotEntities) {
                Com_DPrintf("%s: Delta request from out of date entities.\n", client->name);
                oldframe = NULL;
                lastframe = 0;
            }
        }
    }

    MSG_WriteByte(msg, SV_SVC_SNAPSHOT);
    MSG_WriteLong(msg, svs->time);
    MSG_WriteByte(msg, lastframe);

    snapFlags = svs->snapFlagServerBit;
    if (client->pureAuthentic) {
        snapFlags |= 1;
    }
    if (client->state == 4) {
        client->sendAsActive = 1;
    } else if (client->state != 1) {
        client->sendAsActive = 0;
    }
    if (!client->sendAsActive) {
        snapFlags |= 2;
    }
    MSG_WriteByte(msg, snapFlags);

    MSG_WriteDeltaPlayerstate(msg, oldframe ? (byte *)&oldframe->ps : NULL, (byte *)&frame->ps);
    SV_WritePacketEntitiesLocal(msg, oldframe, frame);
    SV_WritePacketClientsLocal(msg, oldframe, frame);

    {
        int padCount = SV_DvarIntLocal((byte *)sv_padPackets_dvar);
        int i;
        for (i = 0; i < padCount; ++i) {
            MSG_WriteByte(msg, 0);
        }
    }

    if (client->download) {
        SV_WriteDownloadToClient(client, msg);
    }
}

static void SV_WriteOverflowRecoveryCommandsLocal(client_t *client, msg_t *msg)
{
    int reliableAck;
    int reliableSent;
    int i;

    Com_Printf("-- Unacknowledged Server Commands for client %i:%s --\n", SV_ClientIndexLocal(client), client->name);
    for (i = client->reliableAcknowledge + 1; i <= client->reliableSent; ++i) {
        int idx = i & CLIENT_CMDENTRY_MASK;
        Com_Printf("cmd %5d: %8d: %s\n", i, client->reliableCommandInfo[idx].time, client->reliableCommandInfo[idx].cmd);
    }
    Com_Printf("---------------------------------------------\n");

    reliableAck = client->reliableAcknowledge + 1;
    reliableSent = client->reliableSent;
    for (i = reliableAck; i <= reliableSent; ++i) {
        int idx = i & CLIENT_CMDENTRY_MASK;
        const char *cmd = client->reliableCommandInfo[idx].cmd;
        if (msg->cursize + (int)strlen(cmd) + 5 > 0x1ffff) {
            break;
        }
        MSG_WriteByte(msg, SV_SVC_SERVERCOMMAND);
        MSG_WriteLong(msg, i);
        MSG_WriteString(msg, cmd);
    }
    if (i - 1 > client->reliableSent) {
        client->reliableSent = i - 1;
    }
}

static void SV_DropOverflowedClientLocal(client_t *client)
{
    Com_Printf("WARNING: client disconnected for msg overflow: %s\n", client->name);
    NET_OutOfBandPrint(1, client->netchan.remoteAddress, "disconnect");
    SV_DropClient(client, "EXE_SERVERMESSAGEOVERFLOW");
}

void SV_SendClientSnapshot(client_t *client)
{
    byte msg_buf_large_local[24];
    byte *msg_buf;
    msg_t msg;

    LargeLocal_LargeLocal(msg_buf_large_local, 0x20000);
    msg_buf = LargeLocal_GetBuf(msg_buf_large_local);

    if (client->state == 4 || client->state == 1) {
        SV_BuildClientSnapshotLocal(client);
    }

    MSG_Init(&msg, msg_buf, 0x20000);
    MSG_WriteLong(&msg, client->lastClientCommand);

    if (client->state == 4 || client->state == 1) {
        SV_UpdateServerCommandsToClient(client, &msg);
        SV_WriteSnapshotToClientLocal(client, &msg);
    } else {
        SV_WriteDownloadToClient(client, &msg);
    }

    MSG_WriteByte(&msg, SV_SVC_EOF);

    if (msg.overflowed) {
        Com_Printf("WARNING: msg overflowed for %s, trying to recover\n", client->name);
        if (client->state == 4 || client->state == 1) {
            MSG_Init(&msg, msg_buf, 0x20000);
            MSG_WriteLong(&msg, client->lastClientCommand);
            SV_WriteOverflowRecoveryCommandsLocal(client, &msg);
            MSG_WriteByte(&msg, SV_SVC_EOF);
        }
        if (msg.overflowed) {
            SV_DropOverflowedClientLocal(client);
        }
    }

    SV_SendMessageToClient(&msg, client);
    ZN10LargeLocalD1Ev(msg_buf_large_local);
}

void SV_SendClientMessages(void)
{
    server_t *sv = (server_t *)imp_sv;
    serverStatic_t *psvs = (serverStatic_t *)imp_svs;
    client_t *c;
    int i, numclients;
    int maxClients;
    int svsTime;
    int sendFrag, messageSize;
    int rate, rateMsec;
    byte *dvar;
    float totalBps, totalUBps;
    int j;

    sv->bpsTotalBytes = 0;
    sv->ubpsTotalBytes = 0;

    c = psvs->clients;
    maxClients = (*(dvar_t **)(imp_sv_maxclients))->current.integer;
    numclients = 0;

    svsTime = psvs->time;

    for (i = 0; i < maxClients; i++, c++) {

        if (c->state == 0 || svsTime < c->nextSnapshotTime)
            continue;

        numclients++;

        sendFrag = c->netchan.unsentFragments;
        if (sendFrag == 0) {

            SV_SendClientSnapshot(c);
            SV_SendClientVoiceData(c);
            continue;
        }

        messageSize = c->netchan.unsentLength - c->netchan.unsentFragmentStart;
        if (messageSize > 0x5dc)
            messageSize = 0x5dc;

        {
            int msgBytes = messageSize * 5 * 5 * 5 * 8;

            rate = c->rate;
            {
                const dvar_t *minPingRate = (*(dvar_t **)(sv_minPingRate_dvar));
                if (minPingRate->current.integer != 0) {
                    if (minPingRate->current.integer <= 0x3e7) {
                        Dvar_SetInt(minPingRate, 0x3e8);
                        minPingRate = (*(dvar_t **)(sv_minPingRate_dvar));
                    }
                    if (rate > minPingRate->current.integer)
                        rate = minPingRate->current.integer;
                }
            }

            rateMsec = (msgBytes + 0xbb80) / rate;

            if ((*(dvar_t **)(showpackets_dvar))->current.enabled != 0) {
                Com_Printf("It would take %ims to send %i bytes to client %s (rate %i)\n",
                           rateMsec, messageSize, c->name, c->rate);
            }
        }

        c->nextSnapshotTime = svsTime + rateMsec;
        SV_Netchan_TransmitNextFragment(&c->netchan);
    }

    if ((*(dvar_t **)(imp_sv_showAverageBPS))->current.enabled == 0 || numclients <= 0)
        return;

    sv = (server_t *)imp_sv;
    totalBps = 0.0f;
    totalUBps = 0.0f;

    for (j = 0; j < 19; j++) {
        int bpsVal = sv->bpsWindow[j + 1];
        sv->bpsWindow[j] = bpsVal;
        totalBps += (float)bpsVal;

        int ubpsVal = sv->ubpsWindow[j + 1];
        sv->ubpsWindow[j] = ubpsVal;
        totalUBps += (float)ubpsVal;
    }

    {
        int sentBps = sv->bpsTotalBytes;
        sv->bpsWindow[19] = sentBps;
        float bpsTotal = (float)sentBps + totalBps;

        int sentUBps = sv->ubpsTotalBytes;
        sv->ubpsWindow[19] = sentUBps;
        float ubpsTotal = (float)sentUBps + totalUBps;

        if (sentBps >= sv->bpsMaxBytes)
            sv->bpsMaxBytes = sentBps;
        if (sentUBps >= sv->ubpsMaxBytes)
            sv->ubpsMaxBytes = sentUBps;

        int counter = sv->bpsWindowSteps + 1;
        sv->bpsWindowSteps = counter;

        if (counter > 19) {
            sv->bpsWindowSteps = 0;
            float bpsAvg = bpsTotal / 20.0f;
            float ubpsAvg = ubpsTotal / 20.0f;
            float compressionRatio = bpsAvg / ubpsAvg;
            float pctSaved = (1.0f - compressionRatio) * 100.0f;
            float totalPctSaved = pctSaved + sv->ucompAve;
            sv->ucompAve = totalPctSaved;
            int numSamples = sv->ucompNum + 1;
            sv->ucompNum = numSamples;
            float avgPctSaved = totalPctSaved / (float)numSamples;
            Com_DPrintf("bpspc(%2.0f) bps(%2.0f) pk(%i) ubps(%2.0f) upk(%i) cr(%2.2f)",
                        (double)(bpsAvg / (float)numclients), (double)bpsAvg,
                        sv->bpsMaxBytes, (double)ubpsAvg,
                        sv->ubpsMaxBytes, (double)pctSaved, (double)avgPctSaved);
        }
    }
}
