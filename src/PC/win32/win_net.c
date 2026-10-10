#include "common_types.h"
#include "imports.h"
#include "stubs/gcc40_compat.h"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

static qboolean usingSocks;
static qboolean networkingEnabled;
static const dvar_t *net_noudp;
static const dvar_t *net_noipx;
static const dvar_t *net_socksEnabled;
static const dvar_t *net_socksServer;
static const dvar_t *net_socksPort;
static const dvar_t *net_socksUsername;
static const dvar_t *net_socksPassword;
static struct sockaddr socksRelayAddr;
static int ip_socket;
static int numIP;
static byte localIP[16][4];
static char winsockdata[400];
static qboolean winsockInitialized;
static int socks_socket;
static int ipx_socket;
static char socksBuf[4096];

extern void Com_Printf(const char *fmt, ...);
extern void Com_Error(int code, const char *fmt, ...);
extern const dvar_t *Dvar_RegisterBool(const char *name, qboolean value, int flags);
extern const dvar_t *Dvar_RegisterString(const char *name, const char *value, int flags);
extern const dvar_t *Dvar_RegisterInt(const char *name, int value, int min, int max, int flags);
extern void Dvar_SetInt(const dvar_t *dvar, int value);
extern const char *NET_AdrToString(netadr_t adr);
#ifndef _WIN32
extern int WSAStartup(int wVersionRequested, void *lpWSAData);
extern int WSAGetLastError(void);
extern int closesocket(int s);
extern int ioctlsocket(int s, long cmd, unsigned long *argp);
#endif
extern qboolean I_isdigit(int c);
extern int I_stricmp(const char *s1, const char *s2);

typedef struct {
#if defined(__APPLE__) || defined(BYTE_MATCH_DARWIN)
    unsigned char sa_len;
    unsigned char sa_family;
#else
    unsigned short sa_family;
#endif
    unsigned short sa_port;
    unsigned long sa_addr;
    unsigned char sa_zero[8];
} sockaddr_gen;

qboolean Sys_IsLANAddress(netadr_t adr);
void Sys_ShowIP(void);
static qboolean NET_GetDvars(void);
void NET_Sleep(int msec);
qboolean Sys_StringToAdr(const char *s, netadr_t *a);
qboolean Sys_GetPacket(netadr_t *net_from, msg_t *net_message);
void Sys_SendPacket(int length, const void *data, netadr_t to);
void NET_OpenIP(void);
void NET_Config(qboolean enableNetworking);
void NET_Init(void);
void NET_Restart(void);

qboolean Sys_IsLANAddress(netadr_t adr)
{
    int i;

    if (adr.type == NA_LOOPBACK || adr.type == NA_BOT) {
        return 1;
    }

    if (adr.type != NA_IP) {
        return 0;
    }

    if (adr.ip[0] == 10) {
        return 1;
    }

    if (adr.ip[0] == 127) {
        return 1;
    }

    if (adr.ip[0] == 169 && adr.ip[1] == 254) {
        return 1;
    }

    if (adr.ip[0] == 172 && (adr.ip[1] & 0xf0) == 16) {
        return 1;
    }

    if (adr.ip[0] == 192 && adr.ip[1] == 168) {
        return 1;
    }

    for (i = 0; i < numIP; i++) {
        if (adr.ip[0] == localIP[i][0] && adr.ip[1] == localIP[i][1] && adr.ip[2] == localIP[i][2]) {
            return 1;
        }
    }

    return 0;
}

void Sys_ShowIP(void)
{
    int i;

    for (i = 0; i < numIP; i++) {
        Com_Printf("IP: %i.%i.%i.%i\n", localIP[i][0], localIP[i][1], localIP[i][2], localIP[i][3]);
    }
}

static qboolean NET_GetDvars(void)
{
    qboolean modified;

    modified = 0;
    if (net_noudp && net_noudp->modified) {
        modified = 1;
    }

    net_noudp = Dvar_RegisterBool("net_noudp", 0, 0x1021);

    if (net_noipx && net_noipx->modified) {
        modified = 1;
    }

    net_noipx = Dvar_RegisterBool("net_noipx", 0, 0x1021);

    if (net_socksEnabled && net_socksEnabled->modified) {
        modified = 1;
    }

    net_socksEnabled = Dvar_RegisterBool("net_socksEnabled", 0, 0x1021);

    if (net_socksServer && net_socksServer->modified) {
        modified = 1;
    }

    net_socksServer = Dvar_RegisterString("net_socksServer", "", 0x1021);

    if (net_socksPort && net_socksPort->modified) {
        modified = 1;
    }

    net_socksPort = Dvar_RegisterInt("net_socksPort", 1080, 0, 65535, 0x1021);

    if (net_socksUsername && net_socksUsername->modified) {
        modified = 1;
    }

    net_socksUsername = Dvar_RegisterString("net_socksUsername", "", 0x1021);

    if (net_socksPassword && net_socksPassword->modified) {
        modified = 1;
    }

    net_socksPassword = Dvar_RegisterString("net_socksPassword", "", 0x1021);

    return modified;
}

void NET_Sleep(int msec)
{
    fd_set fdset;
    struct timeval timeout;

    if (!ip_socket) {
        return;
    }

    FD_ZERO(&fdset);
    FD_SET(ip_socket, &fdset);

    timeout.tv_sec = msec / 1000;
    timeout.tv_usec = (msec - timeout.tv_sec * 1000) * 1000;

    select(ip_socket + 1, &fdset, NULL, NULL, &timeout);
}

qboolean Sys_StringToAdr(const char *s, netadr_t *a)
{
    struct sockaddr_in sadr;
    struct hostent *h;

    memset(&sadr, 0, sizeof(sadr));
    sadr.sin_family = AF_INET;
    sadr.sin_port = 0;

    if (I_isdigit((signed char)s[0])) {
        sadr.sin_addr.s_addr = inet_addr(s);
    } else {
        h = gethostbyname(s);
        if (!h) {
            return 0;
        }
        sadr.sin_addr.s_addr = *(unsigned long *)h->h_addr_list[0];
    }

    if (sadr.sin_family != AF_INET) {
        return 1;
    }

    a->type = NA_IP;
    *(unsigned long *)a->ip = sadr.sin_addr.s_addr;
    a->port = sadr.sin_port;

    return 1;
}

qboolean Sys_GetPacket(netadr_t *net_from, msg_t *net_message)
{
    int protocol;
    int net_socket;
    int ret;
    socklen_t fromlen;
    struct sockaddr from;
    int err;

    for (protocol = 0; protocol <= 1; protocol++) {
        if (protocol == 0) {
            net_socket = ip_socket;
        } else {
            net_socket = ipx_socket;
        }

        if (!net_socket) {
            continue;
        }

        fromlen = 16;
        ret = recvfrom(net_socket, net_message->data, net_message->maxsize, 0, &from, &fromlen);

        if (ret == -1) {
            err = WSAGetLastError();
            if (err == 10035 || err == 10054) {
                continue;
            }
            Com_Printf("NET_GetPacket: %s\n", strerror(errno));
            continue;
        }

        if (net_socket == ip_socket) {
            memset(from.sa_data + 6, 0, 8);
        }

        if (usingSocks && net_socket == ip_socket) {
            if (memcmp(&from, &socksRelayAddr, fromlen) != 0) {
                goto check_addr;
            }
            if (ret <= 9) {
                continue;
            }
            if (net_message->data[0] != 0 || net_message->data[1] != 0 || net_message->data[2] != 0 || net_message->data[3] != 1) {
                continue;
            }
            net_from->type = NA_IP;
            net_from->ip[0] = net_message->data[4];
            net_from->ip[1] = net_message->data[5];
            net_from->ip[2] = net_message->data[6];
            net_from->ip[3] = net_message->data[7];
            net_from->port = *(unsigned short *)(net_message->data + 8);
            net_message->readcount = 10;
            goto check_size;
        }

    check_addr:
        if (((sockaddr_gen *)&from)->sa_family == AF_INET) {
            sockaddr_gen *from_in = (sockaddr_gen *)&from;
            net_from->type = NA_IP;
            *(unsigned long *)net_from->ip = from_in->sa_addr;
            net_from->port = from_in->sa_port;
        }

        net_message->readcount = 0;

    check_size:
        if (ret != net_message->maxsize) {
            net_message->cursize = ret;
            return 1;
        }

        {
            netadr_t tmp;
            tmp = *net_from;
            Com_Printf("Oversize packet from %s\n", NET_AdrToString(tmp));
        }
    }

    return 0;
}

void Sys_SendPacket(int length, const void *data, netadr_t to)
{
    int net_socket;
    struct sockaddr_in addr;
    int ret;
    int err;

    if (to.type != NA_BROADCAST && to.type != NA_IP) {
        Com_Error(0, "Sys_SendPacket: bad address type");
        return;
    }

    net_socket = ip_socket;

    if (!net_socket) {
        return;
    }

    memset(&addr, 0, sizeof(addr));

    if (to.type == NA_BROADCAST) {
        addr.sin_family = AF_INET;
        addr.sin_port = to.port;
        addr.sin_addr.s_addr = 0xFFFFFFFF;
    } else if (to.type == NA_IP) {
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = *(unsigned long *)to.ip;
        addr.sin_port = to.port;
    }

    if (usingSocks && to.type == NA_IP) {
        socksBuf[0] = 0;
        socksBuf[1] = 0;
        socksBuf[2] = 0;
        socksBuf[3] = 1;
        *(unsigned long *)(socksBuf + 4) = addr.sin_addr.s_addr;
        *(unsigned short *)(socksBuf + 8) = addr.sin_port;
        memcpy(socksBuf + 10, data, length);
        ret = sendto(net_socket, socksBuf, length + 10, 0, (struct sockaddr *)&socksRelayAddr, 16);
    } else {
        ret = sendto(net_socket, data, length, 0, (struct sockaddr *)&addr, 16);
    }

    if (ret != -1) {
        return;
    }

    err = WSAGetLastError();
    if (err == 10035) {
        return;
    }

    if (err == 10049) {
        if (to.type == NA_BROADCAST) {
            return;
        }
    }

    Com_Printf("Sys_SendPacket: %s\n", strerror(errno));
}

void NET_OpenIP(void)
{
    const dvar_t *ip;
    const dvar_t *port;
    int i;
    SOCKET newsocket;
    int _true;
    int sock_i;
    const char *s;
    int err;

    ip = Dvar_RegisterString("net_ip", "localhost", 0x1020);
    port = Dvar_RegisterInt("net_port", 28960, 0, 65535, 0x1020);

    for (i = 0; i < 10; i++) {
        int boundPort;
        boundPort = i + port->current.integer;
        s = ip->current.string;

        _true = 1;
        sock_i = 1;

        if (!s) {
            Com_Printf("Opening IP socket: localhost:%i\n", boundPort);
        } else {
            Com_Printf("Opening IP socket: %s:%i\n", s, boundPort);
        }

        newsocket = socket(2, 2, 17);
        if (newsocket == -1) {
            err = WSAGetLastError();
            if (err != 10047) {
                Com_Printf("WARNING: UDP_OpenSocket: socket: %s\n", strerror(errno));
                ip_socket = 0;
                goto next_check;
            }
            ip_socket = 0;
            goto next_check;
        }

        if (ioctlsocket(newsocket, 0x8004667e, (unsigned long *)&_true) == -1) {
            Com_Printf("WARNING: UDP_OpenSocket: ioctl FIONBIO: %s\n", strerror(errno));
            ip_socket = 0;
            goto next_check;
        }

        if (setsockopt(newsocket, SOL_SOCKET, SO_BROADCAST, &sock_i, sizeof(sock_i)) == -1) {
            Com_Printf("WARNING: UDP_OpenSocket: setsockopt SO_BROADCAST: %s\n", strerror(errno));
            closesocket(newsocket);
            ip_socket = 0;
            goto next_check;
        }

        {
            /* One socket receives every player's UDP traffic. Preserve short
             * bursts during busy ticks instead of dropping them in the kernel. */
            int bufferBytes = 2 * 1024 * 1024;
            if (setsockopt(newsocket, SOL_SOCKET, SO_RCVBUF, &bufferBytes, sizeof(bufferBytes)) == -1)
                Com_Printf("WARNING: UDP receive buffer: %s\n", strerror(errno));
        }

        {
            struct sockaddr_in address;

            memset(&address, 0, sizeof(address));

            if (s && s[0] && I_stricmp(s, "localhost") != 0) {

                address.sin_family = AF_INET;
                address.sin_port = 0;
                if (I_isdigit((signed char)s[0])) {
                    address.sin_addr.s_addr = inet_addr(s);
                } else {
                    struct hostent *h = gethostbyname(s);
                    if (h) {
                        address.sin_addr.s_addr = *(unsigned long *)h->h_addr_list[0];
                    }
                }
            } else {
                address.sin_addr.s_addr = 0;
            }

            if (boundPort == -1) {
                address.sin_port = 0;
            } else {
                address.sin_port = htons(boundPort);
            }
            address.sin_family = AF_INET;

            if (bind(newsocket, (struct sockaddr *)&address, 16) == -1) {
                Com_Printf("WARNING: UDP_OpenSocket: bind: %s\n", strerror(errno));
                closesocket(newsocket);
                ip_socket = 0;
                goto next_check;
            }

            ip_socket = newsocket;
        }

    next_check:
        if (ip_socket) {
            Dvar_SetInt(port, i + port->current.integer);

            if (net_socksEnabled->current.enabled) {

                int sockPort;
                sockPort = i + port->current.integer;

                usingSocks = 0;
                Com_Printf("Opening connection to SOCKS server.\n");

                socks_socket = socket(2, 1, 6);
                if (socks_socket == -1) {
                    WSAGetLastError();
                    Com_Printf("WARNING: NET_OpenSocks: socket: %s\n", strerror(errno));
                    goto do_local_address;
                }

                {
                    struct hostent *hp;
                    hp = gethostbyname(net_socksServer->current.string);
                    if (!hp) {
                        WSAGetLastError();
                        Com_Printf("WARNING: NET_OpenSocks: gethostbyname: %s\n", strerror(errno));
                        goto do_local_address;
                    }
                    if (hp->h_addrtype != AF_INET) {
                        Com_Printf("WARNING: NET_OpenSocks: address type not AF_INET.\n");
                        goto do_local_address;
                    }

                    {
                        struct sockaddr_in saddr;
                        char buf[64];
                        int rlen;
                        int useAuth;

                        memset(&saddr, 0, sizeof(saddr));
                        saddr.sin_family = AF_INET;
                        saddr.sin_addr.s_addr = *(unsigned long *)((struct hostent *)hp)->h_addr_list[0];
                        saddr.sin_port = htons(net_socksPort->current.integer);

                        if (connect(socks_socket, (struct sockaddr *)&saddr, 16) == -1) {
                            WSAGetLastError();
                            Com_Printf("NET_OpenSocks: connect: %s\n", strerror(errno));
                            goto do_local_address;
                        }

                        if (net_socksUsername->current.string[0] || net_socksPassword->current.string[0]) {
                            useAuth = 1;
                        } else {
                            useAuth = 0;
                        }

                        buf[0] = 5;
                        if (useAuth) {
                            buf[1] = 2;
                            buf[2] = (-useAuth) & 2;
                            send(socks_socket, buf, 4, 0);
                        } else {
                            buf[1] = 1;
                            buf[2] = (-useAuth) & 2;
                            send(socks_socket, buf, 3, 0);
                        }

                        if (send(socks_socket, buf, 0, 0) == -1) {

                        }

                        rlen = recv(socks_socket, buf, 64, 0);
                        if (rlen == -1) {
                            WSAGetLastError();
                            Com_Printf("NET_OpenSocks: recv: %s\n", strerror(errno));
                            goto do_local_address;
                        }
                        if (rlen != 2 || buf[0] != 5) {
                            Com_Printf("NET_OpenSocks: bad response\n");
                            goto do_local_address;
                        }

                        if (buf[1] == 0) {

                        } else if (buf[1] == 2) {

                            int ulen, plen;
                            ulen = strlen(net_socksUsername->current.string);
                            plen = strlen(net_socksPassword->current.string);

                            buf[0] = 1;
                            buf[1] = (char)ulen;
                            if (ulen) {
                                memcpy(buf + 2, net_socksUsername->current.string, ulen);
                            }
                            buf[2 + ulen] = (char)plen;
                            if (plen) {
                                memcpy(buf + 2 + ulen + 1, net_socksPassword->current.string, plen);
                            }
                            if (send(socks_socket, buf, 3 + ulen + plen, 0) == -1) {
                                WSAGetLastError();
                                Com_Printf("NET_OpenSocks: send: %s\n", strerror(errno));
                                goto do_local_address;
                            }
                            rlen = recv(socks_socket, buf, 64, 0);
                            if (rlen == -1) {
                                WSAGetLastError();
                                Com_Printf("NET_OpenSocks: recv: %s\n", strerror(errno));
                                goto do_local_address;
                            }
                            if (rlen != 2 || buf[0] != 1) {
                                Com_Printf("NET_OpenSocks: bad response\n");
                                goto do_local_address;
                            }
                            if (buf[1] != 0) {
                                Com_Printf("NET_OpenSocks: authentication failed\n");
                                goto do_local_address;
                            }
                        } else {
                            Com_Printf("NET_OpenSocks: request denied\n");
                            goto do_local_address;
                        }

                        buf[0] = 5;
                        buf[1] = 3;
                        buf[2] = 0;
                        buf[3] = 1;
                        *(unsigned long *)(buf + 4) = 0;
                        {
                            unsigned short sp = (unsigned short)((sockPort >> 8) | (sockPort << 8));
                            *(unsigned short *)(buf + 8) = sp;
                        }
                        if (send(socks_socket, buf, 10, 0) == -1) {
                            WSAGetLastError();
                            Com_Printf("NET_OpenSocks: send: %s\n", strerror(errno));

                        }

                        rlen = recv(socks_socket, buf, 64, 0);
                        if (rlen == -1) {
                            WSAGetLastError();
                            Com_Printf("NET_OpenSocks: recv: %s\n", strerror(errno));
                            goto do_local_address;
                        }
                        if (rlen <= 1 || buf[0] != 5) {
                            Com_Printf("NET_OpenSocks: bad response\n");
                            goto do_local_address;
                        }

                        if (buf[1] != 0) {
                            Com_Printf("NET_OpenSocks: request denied: %i\n", (unsigned char)buf[1]);
                            goto do_local_address;
                        }

                        if (buf[3] != 1) {
                            Com_Printf("NET_OpenSocks: relay address is not IPV4: %i\n", (unsigned char)buf[3]);
                            goto do_local_address;
                        }

                        ((byte *)&socksRelayAddr)[1] = 2;
                        *(unsigned long *)(((byte *)&socksRelayAddr) + 4) = *(unsigned long *)(buf + 4);
                        *(unsigned short *)(((byte *)&socksRelayAddr) + 2) = *(unsigned short *)(buf + 8);
                        memset(((byte *)&socksRelayAddr) + 8, 0, 8);

                        usingSocks = 1;
                    }
                }
            }

        do_local_address:

            {
                char hostname[512];
                struct {
                    int ifc_len;
                    char *ifc_buf;
                } ifconf;
                int tmpSocket;

                numIP = 0;
                ifconf.ifc_len = 512;
                ifconf.ifc_buf = hostname;

                tmpSocket = socket(2, 2, 0);
                if (tmpSocket < 0) {
                    Com_Printf("NET_GetLocalAddress: Unable to create temporary socket, errno = %i\n", errno);
                    return;
                }

                if (ioctl(tmpSocket, 0xc0086924, &ifconf) != 0) {
                    Com_Printf("NET_GetLocalAddress: Unable to get list of network interfaces, errno = %i\n", errno);
                    return;
                }

                {
                    char *ifr = ifconf.ifc_buf;
                    char *ifr_end = ifconf.ifc_buf + ifconf.ifc_len;

                    while (ifr < ifr_end) {
                        char *next_ent;
                        int sa_len;

                        if (ifr[17] != 0x12) {
                            sa_len = ((unsigned char)ifr[16]) - 16;
                            if (sa_len < 0)
                                sa_len = 0;
                            ifr = ifr + 32 + sa_len;
                            continue;
                        }

                        {
                            char *ifr2 = ifconf.ifc_buf;
                            while (ifr2 < ifr_end) {
                                if (ifr2[17] != 2) {
                                    sa_len = ((unsigned char)ifr2[16]) - 16;
                                    if (sa_len < 0)
                                        sa_len = 0;
                                    ifr2 = ifr2 + 32 + sa_len;
                                    continue;
                                }
                                if (strncmp(ifr2, ifr, 16) != 0) {
                                    sa_len = ((unsigned char)ifr2[16]) - 16;
                                    if (sa_len < 0)
                                        sa_len = 0;
                                    ifr2 = ifr2 + 32 + sa_len;
                                    continue;
                                }

                                {
                                    char *p = ifr;
                                    int remaining = 16;
                                    while (*p && remaining) {
                                        p++;
                                        remaining--;
                                    }
                                }

                                if (ifr[20] == 0x18) {
                                    sa_len = ((unsigned char)ifr2[16]) - 16;
                                    if (sa_len < 0)
                                        sa_len = 0;
                                    ifr2 = ifr2 + 32 + sa_len;
                                    continue;
                                }

                                {
                                    char namebuf[16];
                                    unsigned long ipaddr;

                                    strncpy(namebuf, ifr2, 16);
                                    if (ioctl(tmpSocket, 0xc020690d, namebuf) < 0) {
                                        Com_Printf("NET_GetLocalAddress: Unable to get local address for interface %s, errno = %i\n", ifr2, errno);
                                        sa_len = ((unsigned char)ifr2[16]) - 16;
                                        if (sa_len < 0)
                                            sa_len = 0;
                                        ifr2 = ifr2 + 32 + sa_len;
                                        continue;
                                    }

                                    ipaddr = cod2_bswap32(*(unsigned long *)(namebuf + 20));
                                    localIP[numIP][0] = (ipaddr >> 24) & 0xFF;
                                    localIP[numIP][1] = (ipaddr >> 16) & 0xFF;
                                    localIP[numIP][2] = (ipaddr >> 8) & 0xFF;
                                    localIP[numIP][3] = ipaddr & 0xFF;
                                    Com_Printf("IP: %i.%i.%i.%i (%s)\n",
                                               localIP[numIP][0], localIP[numIP][1],
                                               localIP[numIP][2], localIP[numIP][3], ifr2);
                                    numIP++;
                                }

                                sa_len = ((unsigned char)ifr2[16]) - 16;
                                if (sa_len < 0)
                                    sa_len = 0;
                                ifr2 = ifr2 + 32 + sa_len;
                            }
                        }

                        sa_len = ((unsigned char)ifr[16]) - 16;
                        if (sa_len < 0)
                            sa_len = 0;
                        ifr = ifr + 32 + sa_len;
                    }

                    close(tmpSocket);
                }
            }
            return;
        }
    }

    Com_Printf("WARNING: Couldn't allocate IP port\n");
}

void NET_Config(qboolean enableNetworking)
{
    qboolean modified;
    qboolean stop;
    qboolean start;

    modified = NET_GetDvars();

    if (net_noudp->current.enabled && net_noipx->current.enabled) {
        enableNetworking = 0;
    }

    if (enableNetworking == networkingEnabled) {
        if (!modified || !enableNetworking) {
            return;
        }
        start = 1;
        goto close_sockets;
    }

    if (enableNetworking) {
        stop = 0;
        start = 1;
    } else {
        stop = 1;
        start = 0;
    }

    networkingEnabled = enableNetworking;

    if (!stop) {
        goto check_start;
    }

close_sockets:
    if (ip_socket && ip_socket != (SOCKET)-1) {
        closesocket(ip_socket);
        ip_socket = 0;
    }

    if (socks_socket && socks_socket != (SOCKET)-1) {
        closesocket(socks_socket);
        socks_socket = 0;
    }

check_start:
    if (!start) {
        return;
    }

    if (!net_noudp->current.enabled) {
        NET_OpenIP();
    }
}

void NET_Init(void)
{
    int r;

    r = WSAStartup(0x101, &winsockdata);
    if (r) {
        Com_Printf("WARNING: Network initialization failed, returned %d\n", r);
        return;
    }

    winsockInitialized = 1;
    Com_Printf("Network Initialized\n");
    NET_GetDvars();
    NET_Config(1);
}

void NET_Restart(void)
{
    NET_Config(networkingEnabled);
}
