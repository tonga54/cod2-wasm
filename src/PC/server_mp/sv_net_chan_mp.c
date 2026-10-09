#include "common_types.h"
#include "imports.h"
#include <stddef.h>

COD2_ASSERT_FIELD(client_t, reliableAcknowledge,     0x20810);
COD2_ASSERT_FIELD(client_t, messageAcknowledge,      0x20818);
COD2_ASSERT_FIELD(client_t, challenge,               0x20820);
COD2_ASSERT_FIELD(client_t, reliableCommandInfo,     0x40c);
COD2_ASSERT_FIELD(client_t, lastClientCommandString, 0x20844);
COD2_ASSERT_FIELD(client_t, name,                    0x20c48);
COD2_ASSERT_FIELD(client_t, state,                   0x0);

#define CLIENT_RELIABLEACK_OFF 0x20810
#define CLIENT_MSGACK_OFF 0x20818
#define CLIENT_CHALLENGE_OFF 0x20820
#define CLIENT_RELCMDINFO_OFF 0x40c
#define CLIENT_LASTCLIENTCMDSTR_OFF 0x20844
#define CLIENT_NAME_OFF 0x20c48
#define CLIENT_STATE_OFF 0x0
#define CLIENT_NETCHAN_OFF ((int)offsetof(client_t, netchan))
#define CLIENT_NETCHAN_PPROF_OFF ((int)(offsetof(client_t, netchan) + offsetof(netchan_t, pProf)))
#define CLIENT_SERVERID_OFF ((int)offsetof(client_t, serverId))
#define CLIENT_STRIDE ((int)sizeof(client_t))
#define SVSCMD_SIZE 1032

extern serverStatic_t svs;
extern server_t sv;
extern byte *net_profile_dvar;

#define SVS_CLIENTS_OFF 0xc
#define SVS_POOBPROF_OFF 0xa074

extern Bool Netchan_TransmitNextFragment(netchan_t *chan);
extern Bool Netchan_Transmit(netchan_t *chan, int length, byte *data);
extern void NetProf_PrepProfiling(netProfileInfo_t *prof);
extern void NetProf_AddPacket(netProfileStream_t *stream, int iLength, qboolean bFragment);
extern void NetProf_UpdateStatistics(netProfileStream_t *stream);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern void Com_Printf(const char *fmt, ...);
extern void CL_DrawString(int x, int y, const char *str, int color, int size);

void SV_Netchan_Decode(client_t *client, byte *data, int size);
Bool SV_Netchan_TransmitNextFragment(netchan_t *chan);
Bool SV_Netchan_Transmit(client_t *client, int length, byte *data);
void SV_Netchan_AddOOBProfilePacket(int iLength);
void SV_Netchan_PrintProfileStats(qboolean bPrintToConsole);

static float _fminf(float a, float b)
{
    return (a < b) ? a : b;
}

static float _fmaxf(float a, float b)
{
    return (a > b) ? a : b;
}

void SV_Netchan_Decode(client_t *client, byte *data, int size)
{
    byte *cl = (byte *)client;
    int reliableAcknowledge;
    const char *string;
    byte key;
    int index;
    int i;

    reliableAcknowledge = *(int *)(cl + CLIENT_RELIABLEACK_OFF);
    string = (const char *)(cl + CLIENT_RELCMDINFO_OFF + (reliableAcknowledge & 0x7f) * SVSCMD_SIZE);

    key = (byte)(*(int *)(cl + CLIENT_SERVERID_OFF));
    key ^= (byte)(*(int *)(cl + CLIENT_CHALLENGE_OFF));
    key ^= (byte)(*(int *)(cl + CLIENT_MSGACK_OFF));

    index = 0;
    for (i = 0; i < size; i++) {
        byte ch;

        if (string[index] == '\0') {
            ch = (unsigned char)string[0];
            index = 1;
        } else {
            ch = (unsigned char)string[index];
            index++;
        }

        key ^= (byte)(ch << (i & 1));

        data[i] ^= key;
    }
}

Bool SV_Netchan_TransmitNextFragment(netchan_t *chan)
{
    return Netchan_TransmitNextFragment(chan);
}

Bool SV_Netchan_Transmit(client_t *client, int length, byte *data)
{
    byte *cl = (byte *)client;
    int outgoingSequence;
    const char *string;
    byte key;
    int index;
    int dataSize;
    byte *encodeData;
    int i;
    netchan_t *netchan;

    dataSize = length - 4;
    encodeData = data + 4;

    string = (const char *)(cl + CLIENT_LASTCLIENTCMDSTR_OFF);

    outgoingSequence = *(int *)(cl + CLIENT_NETCHAN_OFF);
    key = (byte)(*(int *)(cl + CLIENT_CHALLENGE_OFF));
    key ^= (byte)outgoingSequence;

    index = 0;
    for (i = 0; i < dataSize; i++) {
        byte ch;

        if (string[index] == '\0') {
            ch = (unsigned char)string[0];
            index = 1;
        } else {
            ch = (unsigned char)string[index];
            index++;
        }

        key ^= (byte)(ch << (i & 1));

        encodeData[i] ^= key;
    }

    netchan = (netchan_t *)(cl + CLIENT_NETCHAN_OFF);
    return Netchan_Transmit(netchan, length, data);
}

void SV_Netchan_AddOOBProfilePacket(int iLength)
{
    netProfileInfo_t *pOOBProf;

    /* +8 was the x86 offset of dvar->current; on x64 the name pointer + alignment move current to
       16, so the hardcoded read got garbage and skipped the disabled-profiling early-out. */
    if ((*(const dvar_t **)net_profile_dvar)->current.integer == 0)
        return;

    {
        byte *svs = (byte *)imp_svs;
        NetProf_PrepProfiling((netProfileInfo_t *)(svs + SVS_POOBPROF_OFF));
        pOOBProf = *(netProfileInfo_t **)(svs + SVS_POOBPROF_OFF);
        NetProf_AddPacket((netProfileStream_t *)pOOBProf, iLength, 0);
    }
}

void SV_Netchan_PrintProfileStats(qboolean bPrintToConsole)
{
    char szLine[1024];
    int iYPos;
    int iTotalBPSSent;
    int iTotalPacketsSent;
    int iTotalFragmentsSent;
    int iTotalBPSRecieved;
    int iTotalPacketsRecieved;
    int iTotalFragmentsRecieved;
    int iTotalMaxSent;
    int iTotalMinSent;
    int iTotalMaxRecieved;
    int iTotalMinRecieved;
    int iFragmentTotal;
    netProfileInfo_t *pOOBProf;
    netProfileInfo_t *pProf;
    byte *clientBase;
    int numClients;
    int totalPackets;
    char szClientName[17];
    int i;

    clientBase = *(byte **)(((byte *)&svs) + SVS_CLIENTS_OFF);
    if (clientBase == NULL)
        return;

    pOOBProf = *(netProfileInfo_t **)(((byte *)&svs) + SVS_POOBPROF_OFF);
    if (pOOBProf != NULL) {
        NetProf_UpdateStatistics(&pOOBProf->send);
        NetProf_UpdateStatistics(&pOOBProf->recieve);
    }

    numClients = *(int *)((byte *)((byte *)&sv) + 8);
    for (i = 0; i < numClients; i++) {
        byte *cl = clientBase + (long)i * CLIENT_STRIDE;
        if (*(int *)(cl + CLIENT_STATE_OFF) == 0)
            continue;
        pProf = *(netProfileInfo_t **)(cl + CLIENT_NETCHAN_PPROF_OFF);
        if (pProf == NULL)
            continue;
        NetProf_UpdateStatistics(&pProf->send);
        NetProf_UpdateStatistics(&pProf->recieve);
    }

    if (bPrintToConsole) {
        Com_Printf("\n\n");
    }

    Com_sprintf(szLine, 1024, "====================");
    if (bPrintToConsole) {
        Com_Printf("%s\n", szLine);
        iYPos = 0x50;
    } else {
        CL_DrawString(0x20, 0x5a, szLine, 0, 8);
        iYPos = 0x5a;
    }

    Com_sprintf(szLine, 1024, "Server Network Profile:");
    if (bPrintToConsole) {
        Com_Printf("%s\n\n", szLine);
    } else {
        CL_DrawString(0x20, iYPos + 0xa, szLine, 0, 8);
        iYPos += 0x14;
    }

    Com_sprintf(szLine, 1024,
                "                    | Sent To                | Recieved From          | Total Source Traffic   |");
    if (bPrintToConsole) {
        Com_Printf("%s\n", szLine);
    } else {
        CL_DrawString(0x20, iYPos + 0xa, szLine, 0, 8);
        iYPos += 0xa;
    }

    Com_sprintf(szLine, 1024,
                "              Source|   bps|  max|  min|frag%%|   bps|  max|  min|frag%%|   bps|  max|  min|frag%%|");
    if (bPrintToConsole) {
        Com_Printf("%s\n", szLine);
    } else {
        CL_DrawString(0x20, iYPos + 0xa, szLine, 0, 8);
        iYPos += 0xa;
    }

    pOOBProf = *(netProfileInfo_t **)(((byte *)&svs) + SVS_POOBPROF_OFF);
    if (pOOBProf != NULL) {
        iTotalBPSSent = pOOBProf->send.iBytesPerSecond;
        iTotalPacketsSent = pOOBProf->send.iCountedPackets;
        iTotalFragmentsSent = pOOBProf->send.iCountedFragments;
        iTotalBPSRecieved = pOOBProf->recieve.iBytesPerSecond;
        iTotalPacketsRecieved = pOOBProf->recieve.iCountedPackets;
        iTotalFragmentsRecieved = pOOBProf->recieve.iCountedFragments;

        iTotalMaxSent = pOOBProf->send.iLargestPacket;
        if (iTotalMaxSent <= 0)
            iTotalMaxSent = 0;

        iTotalMinSent = pOOBProf->send.iSmallestPacket;
        if (iTotalMinSent >= 9999)
            iTotalMinSent = 9999;

        iTotalMaxRecieved = pOOBProf->recieve.iLargestPacket;
        if (iTotalMaxRecieved <= 0)
            iTotalMaxRecieved = 0;

        iTotalMinRecieved = pOOBProf->recieve.iSmallestPacket;
        if (iTotalMinRecieved >= 9999)
            iTotalMinRecieved = 9999;
    } else {
        iTotalBPSSent = 0;
        iTotalBPSRecieved = 0;
        iTotalPacketsSent = 0;
        iTotalFragmentsSent = 0;
        iTotalPacketsRecieved = 0;
        iTotalFragmentsRecieved = 0;
        iTotalMaxSent = 0;
        iTotalMinSent = 9999;
        iTotalMaxRecieved = 0;
        iTotalMinRecieved = 9999;
    }

    numClients = *(int *)((byte *)((byte *)&sv) + 8);
    for (i = 0; i < numClients; i++) {
        byte *cl = clientBase + (long)i * CLIENT_STRIDE;
        if (*(int *)(cl + CLIENT_STATE_OFF) == 0)
            continue;
        pProf = *(netProfileInfo_t **)(cl + CLIENT_NETCHAN_PPROF_OFF);
        if (pProf == NULL)
            continue;

        iTotalBPSSent += pProf->send.iBytesPerSecond;
        iTotalPacketsSent += pProf->send.iCountedPackets;
        iTotalFragmentsSent += pProf->send.iCountedFragments;
        iTotalBPSRecieved += pProf->recieve.iBytesPerSecond;
        iTotalPacketsRecieved += pProf->recieve.iCountedPackets;
        iTotalFragmentsRecieved += pProf->recieve.iCountedFragments;

        if (pProf->send.iLargestPacket > iTotalMaxSent)
            iTotalMaxSent = pProf->send.iLargestPacket;
        if (pProf->send.iSmallestPacket < iTotalMinSent)
            iTotalMinSent = pProf->send.iSmallestPacket;
        if (pProf->recieve.iLargestPacket > iTotalMaxRecieved)
            iTotalMaxRecieved = pProf->recieve.iLargestPacket;
        if (pProf->recieve.iSmallestPacket < iTotalMinRecieved)
            iTotalMinRecieved = pProf->recieve.iSmallestPacket;
    }

    totalPackets = iTotalPacketsRecieved + iTotalPacketsSent;
    if (totalPackets > 0 && (iTotalFragmentsSent + iTotalFragmentsRecieved) > 0) {
        iFragmentTotal = (iTotalFragmentsSent + iTotalFragmentsRecieved) * 100 / totalPackets;
    } else {
        iFragmentTotal = 0;
    }

    Com_sprintf(szLine, 1024,
                "              Totals:%6i|%5i|%5i| %3i%%|%6i|%5i|%5i| %3i%%|%6i|%5i|%5i| %3i%%|",
                iTotalBPSSent, iTotalMaxSent, iTotalMinSent, 0,
                iTotalBPSRecieved, iTotalMaxRecieved, iTotalMinRecieved, 0,
                iTotalBPSSent + iTotalBPSRecieved,
                (double)_fmaxf((float)iTotalMaxSent, (float)iTotalMaxRecieved),
                (double)_fminf((float)iTotalMinSent, (float)iTotalMinRecieved),
                iFragmentTotal);
    if (bPrintToConsole) {
        Com_Printf("%s\n", szLine);
    } else {
        CL_DrawString(0x20, iYPos + 0xa, szLine, 0, 8);
        iYPos += 0xa;
    }

    pOOBProf = *(netProfileInfo_t **)(((byte *)&svs) + SVS_POOBPROF_OFF);
    if (pOOBProf != NULL) {
        int oobTotalPackets;
        int oobFragPerc;

        oobTotalPackets = pOOBProf->send.iCountedPackets + pOOBProf->recieve.iCountedPackets;
        if (oobTotalPackets > 0 && (pOOBProf->send.iCountedFragments + pOOBProf->recieve.iCountedFragments) > 0) {
            oobFragPerc = (pOOBProf->send.iCountedFragments + pOOBProf->recieve.iCountedFragments) * 100 / oobTotalPackets;
        } else {
            oobFragPerc = 0;
        }

        Com_sprintf(szLine, 1024,
                    "  OutOfBand Messages: %5i|%5i|%5i| %3i%%| %5i|%5i|%5i| %3i%%| %5i|%5i|%5i| %3i%%|",
                    pOOBProf->send.iBytesPerSecond,
                    pOOBProf->send.iLargestPacket,
                    pOOBProf->send.iSmallestPacket,
                    pOOBProf->send.iFragmentPercentage,
                    pOOBProf->recieve.iBytesPerSecond,
                    pOOBProf->recieve.iLargestPacket,
                    pOOBProf->recieve.iSmallestPacket,
                    pOOBProf->recieve.iFragmentPercentage,
                    pOOBProf->send.iBytesPerSecond + pOOBProf->recieve.iBytesPerSecond,
                    (double)_fmaxf((float)pOOBProf->send.iLargestPacket, (float)pOOBProf->recieve.iLargestPacket),
                    (double)_fminf((float)pOOBProf->send.iSmallestPacket, (float)pOOBProf->recieve.iSmallestPacket),
                    oobFragPerc);
    } else {
        Com_sprintf(szLine, 1024,
                    "  OutOfBand Messages:     0|    0|    0|   - |     0|    0|    0|   - |     0|    0|    0|   - |");
    }
    if (bPrintToConsole) {
        Com_Printf("%s\n", szLine);
    } else {
        CL_DrawString(0x20, iYPos + 0xa, szLine, 0, 8);
        iYPos += 0xa;
    }

    numClients = *(int *)((byte *)((byte *)&sv) + 8);
    for (i = 0; i < numClients; i++) {
        byte *cl = clientBase + (long)i * CLIENT_STRIDE;

        if (*(int *)(cl + CLIENT_STATE_OFF) == 0)
            continue;

        strncpy(szClientName, (const char *)(cl + CLIENT_NAME_OFF), 0x11);
        szClientName[16] = '\0';

        pProf = *(netProfileInfo_t **)(cl + CLIENT_NETCHAN_PPROF_OFF);
        if (pProf == NULL) {

            Com_sprintf(szLine, 1024,
                        "#%2i-%16s:     0|    0|    0|   0%%|     0|    0|    0|   0%%|     0|    0|    0|   0%%|",
                        i, szClientName);
        } else {
            int clTotalPackets;
            int clFragPerc;

            clTotalPackets = pProf->send.iCountedPackets + pProf->recieve.iCountedPackets;
            if (clTotalPackets > 0 && (pProf->send.iCountedFragments + pProf->recieve.iCountedFragments) > 0) {
                clFragPerc = (pProf->send.iCountedFragments + pProf->recieve.iCountedFragments) * 100 / clTotalPackets;
            } else {
                clFragPerc = 0;
            }

            Com_sprintf(szLine, 1024,
                        "#%2i-%16s: %5i|%5i|%5i| %3i%%| %5i|%5i|%5i| %3i%%| %5i|%5i|%5i| %3i%%|",
                        i, szClientName,
                        pProf->send.iBytesPerSecond,
                        pProf->send.iLargestPacket,
                        pProf->send.iSmallestPacket,
                        pProf->send.iFragmentPercentage,
                        pProf->recieve.iBytesPerSecond,
                        pProf->recieve.iLargestPacket,
                        pProf->recieve.iSmallestPacket,
                        pProf->recieve.iFragmentPercentage,
                        pProf->send.iBytesPerSecond + pProf->recieve.iBytesPerSecond,
                        (double)_fmaxf((float)pProf->send.iLargestPacket, (float)pProf->recieve.iLargestPacket),
                        (double)_fminf((float)pProf->send.iSmallestPacket, (float)pProf->recieve.iSmallestPacket),
                        clFragPerc);
        }

        if (bPrintToConsole) {
            Com_Printf("%s\n", szLine);
        } else {
            CL_DrawString(0x20, iYPos + 0xa, szLine, 0, 8);
            iYPos += 0xa;
        }
    }
}
