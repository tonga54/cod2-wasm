#include "common_types.h"
#include "imports.h"

int g_qport = 0x0;

extern int g_qport;
extern const dvar_t *net_profile;
extern const dvar_t *net_showprofile;
extern const dvar_t *net_lanauthorize;
extern const dvar_t *showpackets;
extern const dvar_t *showdrop;
extern const dvar_t *packetDebug;
static char s[64];

/* netsrcString: netsrc -> name. Migrated from the ILP32 data blob to typed C so
 * it re-lays-out per target (2 char* = 8 B on x86, 16 B on x64). The blob's
 * 28-byte extent was 8 B of pointers + 20 B of inter-symbol padding (next symbol
 * g_console_field_width is distinct; nothing reads netsrcString past [1]) -- the
 * padding is dropped. (x64 port Stage 2.) */
char *netsrcString[2] = {
    "client1",
    "server",
};
static int net_iProfilingOn;
extern loopback_t loopbacks[2];

COD2_ASSERT_FIELD(dvar_t, current, 0x8);
COD2_ASSERT_FIELD(LegacyHacks, cl_running, 0x4);

extern void Com_Printf(const char *fmt, ...);
extern void Com_DPrintf(const char *fmt, ...);
extern void Com_Error(int level, const char *fmt, ...);
extern int Com_sprintf(char *dest, int size, const char *fmt, ...);
extern void Com_Memcpy(void *dest, const void *src, int count);
extern void SV_Netchan_PrintProfileStats(int bDumpRecvStats);
extern void CL_Netchan_PrintProfileStats(int bDumpRecvStats);
extern qboolean Sys_GetPacket(netadr_t *net_from, msg_t *net_message);
extern qboolean Sys_StringToAdr(const char *s, netadr_t *a);
extern void Sys_SendPacket(int length, const void *data, netadr_t to);
extern int Sys_Milliseconds(void);
#ifndef _WIN32
extern void *malloc(int size);
#endif
extern void free(void *ptr);
extern char *strchr(const char *s, int c);
extern int atoi(const char *s);
extern void I_strncpyz(char *dest, const char *src, int size);
extern const dvar_t *Dvar_RegisterBool(const char *name, qboolean defaultValue, int flags);
extern const dvar_t *Dvar_RegisterInt(const char *name, int defaultValue, int minValue, int maxValue, int flags);
extern void Cmd_AddCommand(const char *cmd_name, void (*function)(void));
extern void MSG_Init(msg_t *buf, byte *data, int length);
extern void MSG_WriteLong(msg_t *sb, int c);
extern void MSG_WriteShort(msg_t *sb, int c);
extern void MSG_WriteData(msg_t *buf, const void *data, int length);
extern void MSG_BeginReading(msg_t *msg);
extern int MSG_ReadLong(msg_t *msg);
extern int MSG_ReadShort(msg_t *msg);
extern void SV_Netchan_AddOOBProfilePacket(int size);
extern void CL_Netchan_AddOOBProfilePacket(int size);
extern void LargeLocal_LargeLocal(void *ll, int size);
extern void *LargeLocal_GetBuf(void *ll);
extern void ZN10LargeLocalD1Ev(void *ll);
extern void *imp_com_sv_running;
extern void *imp_legacyHacks;

void NetProf_PrepProfiling(netProfileInfo_t **pProf);
void NetProf_AddPacket(netProfileStream_t *pProfStream, int iSize, qboolean bFragment);
void NetProf_UpdateStatistics(netProfileStream_t *pStream);
void Net_DumpProfile_f(void);
void Net_DisplayProfile(void);
void Netchan_Init(int port);
void Netchan_Setup(netsrc_t sock, netchan_t *chan, struct netadr_t adr, int qport);
int NET_CompareAdrSigned(netadr_t *a, netadr_t *b);
qboolean NET_IsLocalAddress(netadr_t adr);
qboolean NET_GetPacket(netadr_t *net_from, msg_t *net_message);
const char *NET_AdrToString(netadr_t a);
qboolean NET_StringToAdr(const char *s, netadr_t *a);
Bool NET_SendPacket(netsrc_t sock, int length, const void *data, netadr_t to);
void NET_OutOfBandVoiceData(netsrc_t sock, netadr_t adr, byte *format, int len);
Bool NET_OutOfBandData(netsrc_t sock, netadr_t adr, byte *format, int len);
Bool NET_OutOfBandPrint(netsrc_t sock, netadr_t adr, const char *data);
qboolean NET_CompareAdr(netadr_t a, netadr_t b);
qboolean NET_CompareBaseAdr(netadr_t a, netadr_t b);
qboolean NET_GetLoopPacket(netsrc_t sock, netadr_t *net_from, msg_t *net_message);
Bool Netchan_TransmitNextFragment(netchan_t *chan);
Bool Netchan_Transmit(netchan_t *chan, int length, const byte *data);
qboolean Netchan_Process(netchan_t *chan, msg_t *msg);

void NetProf_PrepProfiling(netProfileInfo_t **pProf)
{

    int net_profile_val = (int)net_profile->current.integer;

    if (net_profile_val != 0) {

        if (net_iProfilingOn != 0) {

            if (*pProf != NULL)
                return;
            goto alloc_prof;
        }

        {

            int sv_running = (*(const dvar_t **)imp_com_sv_running)->current.integer;
            if (sv_running != 0) {

                int lh_field = (*(LegacyHacks **)imp_legacyHacks)->cl_running;
                if (lh_field != 0 && net_iProfilingOn == 2) {

                    net_iProfilingOn = 1;
                } else {
                    net_iProfilingOn = 2;
                }
            } else {
                net_iProfilingOn = 1;
            }
            Com_Printf("Net Profiling turned on: %s\n", g_qport);
            if (*pProf != NULL)
                return;
            goto alloc_prof;
        }
    } else {

        if (net_iProfilingOn != 0) {

            net_iProfilingOn = 0;
            Com_Printf("Net Profiling turned off\n");
            if (*pProf != NULL) {
                free(*pProf);
                *pProf = NULL;
            }
            return;
        }

        if (*pProf != NULL) {
            free(*pProf);
            *pProf = NULL;
        }
        return;
    }

alloc_prof:
    *pProf = (netProfileInfo_t *)malloc(0x5e0);
    memset(*pProf, 0, sizeof(netProfileInfo_t));
    return;
}

static __attribute__((always_inline)) inline void NetProf_AddPacket_core(netProfileStream_t *pProfStream, int iSize, qboolean bFragment)
{

    int newIdx = (pProfStream->iCurrPacket + 1) % 60;
    pProfStream->iCurrPacket = newIdx;
    netProfilePacket_t *pPacket = &pProfStream->packets[newIdx];
    pPacket->iTime = Sys_Milliseconds();
    pPacket->iSize = iSize;
    pPacket->bFragment = bFragment;
}

void NetProf_AddPacket(netProfileStream_t *pProfStream, int iSize, qboolean bFragment)
{
    NetProf_AddPacket_core(pProfStream, iSize, bFragment);
}

void NetProf_UpdateStatistics(netProfileStream_t *pStream)
{
    int iCurTime = Sys_Milliseconds();
    netProfilePacket_t *pBase = pStream->packets;
    int iNumPackets = 0;
    int iNumFragments = 0;
    int iOldestPacket = -1;
    int iTotalBytes = 0;
    int iSmallestSize = 0x270f;
    int iLargestSize = 0;
    int i;

    for (i = 0; i < 60; i++) {
        netProfilePacket_t *p = &pBase[i];
        if (p->iTime == 0)
            continue;

        if (Sys_Milliseconds() > p->iTime + 0x3e8)
            continue;
        iNumPackets++;
        iNumFragments += (p->bFragment != 0);
        if (p->iTime < iCurTime) {
            iOldestPacket = i;
            iCurTime = p->iTime;
        }
        iTotalBytes += p->iSize;
        if (p->iSize < iSmallestSize)
            iSmallestSize = p->iSize;
        if (p->iSize > iLargestSize)
            iLargestSize = p->iSize;
    }

    if (iNumPackets == 0) {
        pStream->iBytesPerSecond = 0;
        pStream->iLastBPSCalcTime = 0;
        pStream->iCountedPackets = 0;
        pStream->iCountedFragments = 0;
        pStream->iFragmentPercentage = 0;
        pStream->iLargestPacket = 0;
        pStream->iSmallestPacket = 0;
        return;
    }

    if (iNumFragments != 0) {

        int fragPct = iNumFragments * 100 / iNumPackets;
        pStream->iFragmentPercentage = fragPct;
        pStream->iLargestPacket = iLargestSize;
        pStream->iSmallestPacket = iSmallestSize;
    } else {
        pStream->iFragmentPercentage = 0;
        pStream->iLargestPacket = iLargestSize;
        pStream->iSmallestPacket = iSmallestSize;
    }

    {
        int lastCalc = pStream->iLastBPSCalcTime + 0x64;
        int now = Sys_Milliseconds();
        if (lastCalc < now) {

            int timeDelta = Sys_Milliseconds() - iCurTime;

            if (iOldestPacket != -1) {

                netProfilePacket_t *oldest = &pBase[iOldestPacket];
                iTotalBytes -= oldest->iSize;
                iNumPackets--;
                iNumFragments -= (oldest->bFragment != 0);
            }

            if (timeDelta <= 0 || iNumPackets == 0) {

                pStream->iBytesPerSecond = 0;
            } else {
                if (iTotalBytes == 0) {
                    pStream->iBytesPerSecond = 0;
                } else {

                    pStream->iBytesPerSecond = (int)((float)iTotalBytes / ((float)timeDelta * 0.001f));
                }
                pStream->iLastBPSCalcTime = Sys_Milliseconds();
            }
        }
        pStream->iCountedPackets = iNumPackets;
        pStream->iCountedFragments = iNumFragments;
    }
}

void Net_DumpProfile_f(void)
{
    if (net_iProfilingOn == 0) {
        Com_Printf("Network profiling is not on. Set net_profile to turn on network profiling\n");
        return;
    }
    if (net_iProfilingOn == 1) {
        CL_Netchan_PrintProfileStats(1);
        return;
    }
    SV_Netchan_PrintProfileStats(1);
}

void Net_DisplayProfile(void)
{
    if (net_iProfilingOn == 0)
        return;
    if (net_iProfilingOn == 1) {
        CL_Netchan_PrintProfileStats(0);
        return;
    }
    SV_Netchan_PrintProfileStats(0);
}

void Netchan_Init(int port)
{

    showpackets = Dvar_RegisterBool("showpackets", 0, 0x1000);
    showdrop = Dvar_RegisterBool("showdrop", 0, 0x1000);
    packetDebug = Dvar_RegisterBool("packetDebug", 0, 0x1000);
    g_qport = (unsigned short)port;
    net_profile = Dvar_RegisterInt("net_profile", 0, 0, 2, 0x1000);
    net_showprofile = Dvar_RegisterInt("net_showprofile", 0, 0, 3, 0x1000);
    net_lanauthorize = Dvar_RegisterBool("net_lanauthorize", 0, 0x1000);
    Cmd_AddCommand("net_dumpprofile", Net_DumpProfile_f);
}

void Netchan_Setup(netsrc_t sock, netchan_t *chan, struct netadr_t adr, int qport)
{
    memset(chan, 0, sizeof(netchan_t));
    chan->sock = sock;
    chan->remoteAddress = adr;
    chan->qport = qport;
    chan->dropped = 0;
    chan->outgoingSequence = 1;
    NetProf_PrepProfiling(&chan->pProf);
}

int NET_CompareAdrSigned(netadr_t *a, netadr_t *b)
{
    int diff;
    if (a->type != b->type) {
        return (int)a->type - (int)b->type;
    }
    if (a->type == 2) {

        return 0;
    }
    if (a->type != 4) {
        Com_Printf("NET_CompareAdrSigned: bad address type\n");
        return 0;
    }

    if (a->port != b->port) {
        return (int)(unsigned short)a->port - (int)(unsigned short)b->port;
    }

    diff = memcmp(a->ip, b->ip, 4);
    return diff;
}

qboolean NET_IsLocalAddress(netadr_t adr)
{
    if (adr.type == 2 || adr.type == 0)
        return 1;
    return 0;
}

qboolean NET_GetPacket(netadr_t *net_from, msg_t *net_message)
{
    return Sys_GetPacket(net_from, net_message);
}

const char *NET_AdrToString(netadr_t a)
{
    if (a.type == 2) {

        Com_sprintf(s, 0x40, "loopback");
    } else if (a.type == 4) {

        unsigned short portHost = (a.port >> 8) | (a.port << 8);
        Com_sprintf(s, 0x40, "%i.%i.%i.%i:%i",
                    (int)a.ip[0],
                    (int)a.ip[1],
                    (int)a.ip[2],
                    (int)a.ip[3],
                    (int)(short)portHost);
    }

    return s;
}

qboolean NET_StringToAdr(const char *s, netadr_t *a)
{
    char base[0x400];
    char *port;

    if (memcmp(s, "loopback", 0xa) == 0 || memcmp(s, "localhost", 0xa) == 0) {
        a->port = 0;
        a->type = 0;
        *(int *)a->ip = 0;
        a->type = 2;
        return 1;
    }

    I_strncpyz(base, s, 0x400);
    port = strchr(base, ':');
    if (port != NULL) {
        *port = 0;
        port++;
    }

    if (!Sys_StringToAdr(base, a)) {
        a->type = 1;
        return 0;
    }

    if (*(int *)a->ip == -1) {

        a->type = 1;
        return 0;
    }

    if (port != NULL) {
        int portNum = atoi(port);

        a->port = (unsigned short)((portNum >> 8) | (portNum << 8));
        return 1;
    } else {

        a->port = 0x2071;
        return 1;
    }
}

Bool NET_SendPacket(netsrc_t sock, int length, const void *data, netadr_t to)
{
    if (showpackets->current.enabled) {
        if (*(int *)data != -1) {
            Com_Printf("[client %i] send packet %4i\n", 0, length);
        }
    }

    if (to.type == 2) {

        int idx;
        loopmsg_t *msg;
        loopback_t *loop;

        if (sock <= 0) {

            idx = 1;
            loop = &loopbacks[idx];
            int sendSlot = loop->send & 0xf;
            loop->send++;
            msg = &loop->msgs[sendSlot];
            memcpy(msg, data, length);
            msg->datalen = length;
            msg->port = 0;
            return 1;
        } else if (sock == 1) {

            unsigned short portVal = to.port;
            idx = 0;
            loop = &loopbacks[idx];
            int sendSlot = loop->send & 0xf;
            loop->send++;
            msg = &loop->msgs[sendSlot];
            memcpy(msg, data, length);
            msg->datalen = length;
            msg->port = portVal;
            return 1;
        } else {

            return 0;
        }
    } else if (to.type == 1 || to.type == 0) {

        return 0;
    } else {

        Sys_SendPacket(length, data, to);
        return 1;
    }
}

void NET_OutOfBandVoiceData(netsrc_t sock, netadr_t adr, byte *format, int len)
{
    byte string[0x8000 + 4];

    string[0] = 0xff;
    string[1] = 0xff;
    string[2] = 0xff;
    string[3] = 0xff;
    memcpy(string + 4, format, len);
    len += 4;

    NET_SendPacket(sock, len, string, adr);

    if (sock == 1) {
        SV_Netchan_AddOOBProfilePacket(len);
    } else {
        CL_Netchan_AddOOBProfilePacket(len);
    }
}

Bool NET_OutOfBandData(netsrc_t sock, netadr_t adr, byte *format, int len)
{

    char string_large_local[8];
    byte *string;
    int i;
    Bool res;

    LargeLocal_LargeLocal(string_large_local, MAX_MSGLEN);
    string = (byte *)LargeLocal_GetBuf(string_large_local);
    string[0] = 0xff;
    string[1] = 0xff;
    string[2] = 0xff;
    string[3] = 0xff;

    for (i = 0; i < len; i++) {
        string[4 + i] = format[i];
    }
    len += 4;

    {
        netadr_t to = adr;
        res = NET_SendPacket(sock, len, string, to);
    }

    if (sock == 1) {
        SV_Netchan_AddOOBProfilePacket(len);
    } else {
        CL_Netchan_AddOOBProfilePacket(len);
    }

    ZN10LargeLocalD1Ev(string_large_local);
    return res > 0 ? 1 : 0;
}

Bool NET_OutOfBandPrint(netsrc_t sock, netadr_t adr, const char *data)
{

    char string_large_local[8];
    byte *string;
    int iLength;
    Bool res;

    LargeLocal_LargeLocal(string_large_local, MAX_MSGLEN);
    string = (byte *)LargeLocal_GetBuf(string_large_local);
    string[0] = 0xff;
    string[1] = 0xff;
    string[2] = 0xff;
    string[3] = 0xff;

    if (showpackets->current.enabled) {
        Com_DPrintf("OOB Print: %s\n", data);
    }

    iLength = strlen(data);
    if (iLength + 1 > 0x3ffc) {
        Com_DPrintf("OOB Packet is %i bytes - too large to send\n", iLength);
        ZN10LargeLocalD1Ev(string_large_local);
        return 0;
    }

    memcpy(string + 4, data, iLength + 1);
    iLength = strlen((char *)string);

    res = NET_SendPacket(sock, iLength, string, adr);

    if (sock == 1) {
        SV_Netchan_AddOOBProfilePacket(iLength);
    } else {
        CL_Netchan_AddOOBProfilePacket(iLength);
    }

    res = res > 0 ? 1 : 0;
    ZN10LargeLocalD1Ev(string_large_local);
    return res;
}

qboolean NET_CompareAdr(netadr_t a, netadr_t b)
{
    int diff;
    if (a.type != b.type) {
        diff = (int)a.type - (int)b.type;
        return diff == 0 ? 1 : 0;
    }
    if (a.type == 2) {

        return 1;
    }
    if (a.type != 4) {
        Com_Printf("NET_CompareAdr: bad address type\n");
        return 0;
    }

    if (a.port != b.port) {
        return 0;
    }

    diff = memcmp(a.ip, b.ip, 4);
    return diff == 0 ? 1 : 0;
}

qboolean NET_CompareBaseAdr(netadr_t a, netadr_t b)
{
    int diff;
    if (a.type != b.type) {
        diff = (int)a.type - (int)b.type;
        return diff == 0 ? 1 : 0;
    }
    if (a.type == 2) {

        diff = (int)(unsigned short)a.port - (int)(unsigned short)b.port;
        return diff == 0 ? 1 : 0;
    }
    if (a.type == 0) {

        diff = (int)(unsigned short)a.port - (int)(unsigned short)b.port;
        return diff == 0 ? 1 : 0;
    }
    if (a.type != 4) {
        Com_Printf("NET_CompareBaseAdr: bad address type\n");
        diff = 0;
        return diff == 0 ? 1 : 0;
    }

    diff = memcmp(a.ip, b.ip, 4);
    return diff == 0 ? 1 : 0;
}

qboolean NET_GetLoopPacket(netsrc_t sock, netadr_t *net_from, msg_t *net_message)
{

    loopback_t *loop = &loopbacks[(int)sock];
    int send = loop->send;
    int get = loop->get;
    int pending = send - get;

    if (pending > 16) {

        loop->get = send - 16;
        get = loop->get;
    }

    if (get >= loop->send) {
        return 0;
    }

    {
        int slot = get & 0xf;
        loop->get = get + 1;
        loopmsg_t *m = &loop->msgs[slot];

        memcpy(net_message->data, m, m->datalen);
        net_message->cursize = m->datalen;

        *(int *)net_from->ip = 0;
        *(int *)&net_from->port = 0;
        net_from->type = 2;
        net_from->port = (unsigned short)m->port;
        return 1;
    }
}

Bool Netchan_TransmitNextFragment(netchan_t *chan)
{
    byte send_buf[0x578];
    msg_t send;
    int fragmentLength;
    int res;
    int iSize;

    NetProf_PrepProfiling(&chan->pProf);

    MSG_Init(&send, send_buf, 0x578);
    MSG_WriteLong(&send, chan->outgoingSequence | 0x80000000);

    if ((int)chan->sock <= 0) {
        MSG_WriteShort(&send, chan->qport);
    }

    {
        int fragStart = chan->unsentFragmentStart;
        if (fragStart + 0x514 > chan->unsentLength) {
            fragmentLength = chan->unsentLength - fragStart;
        } else {
            fragmentLength = 0x514;
        }

        if (chan->sock == 1) {
            MSG_WriteLong(&send, fragStart);
        } else {
            MSG_WriteShort(&send, fragStart);
        }
        MSG_WriteShort(&send, fragmentLength);
        MSG_WriteData(&send, chan->unsentBuffer + fragStart, fragmentLength);
    }

    {
        netadr_t to = chan->remoteAddress;
        res = (int)NET_SendPacket(chan->sock, send.cursize, send.data, to);
    }
    iSize = send.cursize;

    if (net_iProfilingOn != 0) {
        NetProf_AddPacket_core(&chan->pProf->send, iSize, 1);
        if (net_showprofile->current.integer & 2) {
            Com_Printf("%s send%s: %i\n",
                       netsrcString[chan->sock],
                       " fragment",
                       iSize);
        }
    }

    if (showpackets->current.enabled) {
        Com_Printf("[client %i] %s send %4i : s=%i fragment=%i,%i\n",
                   0,
                   netsrcString[chan->sock],
                   send.cursize,
                   chan->outgoingSequence - 1,
                   chan->unsentFragmentStart,
                   fragmentLength);
    }

    chan->unsentFragmentStart += fragmentLength;

    if (chan->unsentFragmentStart == chan->unsentLength) {
        if (fragmentLength != 0x514) {

            chan->outgoingSequence++;
            chan->unsentFragments = 0;
        }
    }

    return res > 0 ? 1 : 0;
}

Bool Netchan_Transmit(netchan_t *chan, int length, const byte *data)
{
    byte send_buf[0x578];
    msg_t send;
    int res;
    int iSize;

    if (length > 0x4000) {
        Com_Error(1, "Netchan_Transmit: length = %i", length);
    }

    chan->unsentFragmentStart = 0;

    if (length > 0x513) {

        chan->unsentFragments = 1;
        chan->unsentLength = length;
        Com_Memcpy(chan->unsentBuffer, data, length);
        Netchan_TransmitNextFragment(chan);
        return 1;
    }

    NetProf_PrepProfiling(&chan->pProf);

    MSG_Init(&send, send_buf, 0x578);
    MSG_WriteLong(&send, chan->outgoingSequence);
    chan->outgoingSequence++;

    if ((int)chan->sock <= 0) {
        MSG_WriteShort(&send, chan->qport);
    }

    if (packetDebug->current.enabled) {
        Com_Printf("Adding %i byte payload to packet\n", length);
    }
    MSG_WriteData(&send, data, length);
    if (packetDebug->current.enabled) {
        Com_Printf("Sending %i byte packet\n", send.cursize);
    }

    {
        netadr_t to = chan->remoteAddress;
        res = (int)NET_SendPacket(chan->sock, send.cursize, send.data, to);
    }
    iSize = send.cursize;

    if (net_iProfilingOn != 0) {
        NetProf_AddPacket_core(&chan->pProf->send, iSize, 0);
        if (net_showprofile->current.integer & 2) {
            Com_Printf("%s send%s: %i\n",
                       netsrcString[chan->sock],
                       "",
                       iSize);
        }
    }

    if (showpackets->current.enabled) {
        Com_Printf("[client %i] %s send %4i : s=%i ack=%i\n",
                   1,
                   netsrcString[chan->sock],
                   send.cursize,
                   chan->outgoingSequence - 1,
                   chan->incomingSequence);
    }

    return res > 0 ? 1 : 0;
}

qboolean Netchan_Process(netchan_t *chan, msg_t *msg)
{
    int sequence;
    int fragmented;
    int fragmentStart;
    int fragmentLength;
    int iSize;

    NetProf_PrepProfiling(&chan->pProf);

    MSG_BeginReading(msg);
    sequence = MSG_ReadLong(msg);

    if (sequence < 0) {
        sequence &= 0x7fffffff;
        fragmented = 1;
    } else {
        fragmented = 0;
    }

    if (chan->sock == 1) {
        MSG_ReadShort(msg);
    }

    if (fragmented) {

        if (chan->sock == 0) {
            fragmentStart = MSG_ReadLong(msg);
        } else {
            fragmentStart = MSG_ReadShort(msg);
        }
        fragmentLength = MSG_ReadShort(msg);
    } else {
        fragmentStart = 0;
        fragmentLength = 0;
    }

    iSize = msg->cursize;

    if (net_iProfilingOn != 0) {
        NetProf_AddPacket_core(&chan->pProf->recieve, iSize, fragmented);
        if (net_showprofile->current.integer & 2) {
            Com_Printf("%s recieve%s: %i\n",
                       netsrcString[chan->sock],
                       fragmented ? " fragment" : "",
                       iSize);
        }
    }

    if (showpackets->current.enabled) {
        if (!fragmented) {
            Com_Printf("[client %i] %s recv %4i : s=%i\n",
                       1,
                       netsrcString[chan->sock],
                       msg->cursize,
                       sequence);
        } else {
            Com_Printf("[client %i] %s recv %4i : s=%i fragment=%i,%i\n",
                       1,
                       netsrcString[chan->sock],
                       msg->cursize,
                       sequence,
                       fragmentStart,
                       fragmentLength);
        }
    }

#define CHAN_INCOMING_SEQUENCE (*(int *)((char *)chan + 0xc))
#define CHAN_DROPPED (*(int *)((char *)chan + 0x8))

    if (sequence <= CHAN_INCOMING_SEQUENCE) {
        if (showdrop->current.enabled || showpackets->current.enabled) {
            Com_Printf("[client %i] %s:Out of order packet %i at %i\n",
                       1,
                       NET_AdrToString(chan->remoteAddress),
                       sequence,
                       CHAN_INCOMING_SEQUENCE);
        }
        return 0;
    }

    CHAN_DROPPED = sequence - CHAN_INCOMING_SEQUENCE - 1;
    if (CHAN_DROPPED > 0) {
        if (showdrop->current.enabled || showpackets->current.enabled) {
            Com_Printf("[client %i] %s: Dropped %i packets at %i\n",
                       1,
                       NET_AdrToString(chan->remoteAddress),
                       CHAN_DROPPED,
                       sequence);
        }
    }

    if (!fragmented) {
        CHAN_INCOMING_SEQUENCE = sequence;
        return 1;
    }

    if (sequence != chan->fragmentSequence) {
        chan->fragmentSequence = sequence;
        chan->fragmentLength = 0;
    }

    if (fragmentStart != chan->fragmentLength) {
        if (showdrop->current.enabled || showpackets->current.enabled) {
            Com_Printf("%s:Dropped a message fragment\n",
                       NET_AdrToString(chan->remoteAddress),
                       sequence);
        }
        return 0;
    }

    if (fragmentLength < 0 ||
        msg->readcount + fragmentLength > msg->cursize ||
        fragmentStart + fragmentLength > MAX_MSGLEN) {
        if (showdrop->current.enabled || showpackets->current.enabled) {
            Com_Printf("%s:illegal fragment length\n",
                       NET_AdrToString(chan->remoteAddress));
        }
        return 0;
    }

    memcpy(chan->fragmentBuffer + fragmentStart,
           msg->data + msg->readcount,
           fragmentLength);

    chan->fragmentLength += fragmentLength;

    if (fragmentLength == 0x514) {

        return 0;
    }

    if (chan->fragmentLength > msg->maxsize) {
        Com_Printf("%s:fragmentLength %i > msg->maxsize\n",
                   NET_AdrToString(chan->remoteAddress),
                   chan->fragmentLength);
        return 0;
    }

    {
        *(int *)(msg->data) = sequence;
        memcpy(msg->data + 4, chan->fragmentBuffer, chan->fragmentLength);
        msg->cursize = chan->fragmentLength + 4;
        chan->fragmentLength = 0;
        MSG_BeginReading(msg);
        MSG_ReadLong(msg);
    }

    CHAN_INCOMING_SEQUENCE = sequence;
    return 1;
#undef CHAN_INCOMING_SEQUENCE
#undef CHAN_DROPPED
}
