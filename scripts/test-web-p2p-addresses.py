#!/usr/bin/env python3
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
source = (root / 'downstream/wasm/web_net.c').read_text()
helpers = source[source.index('static qboolean WebNet_IsHostPeer'):source.index('EM_JS(void, WebNet_Open')]
routing = source[source.index('qboolean Sys_IsLANAddress'):source.index('void NET_Config')]
code = r'''
#include <assert.h>
#include <string.h>
typedef int qboolean;
enum {NA_BOT, NA_BAD, NA_LOOPBACK, NA_BROADCAST, NA_IP};
typedef struct {int type; unsigned char ip[4]; unsigned short port;} netadr_t;
typedef struct {unsigned char *data; int maxsize, cursize, readcount;} msg_t;
typedef struct {struct {int enabled;} current;} dvar_t;
static dvar_t running;
static const dvar_t *com_sv_running = &running;
static int hostReceives, hostPeer = -1, hostLength, clientSlot = -1, clientLength, opened = -1, sent;
static int queuedHost, queuedClient, queuedRoom;
static void Com_Printf(const char *format, ...) { (void)format; }
static int WebP2P_HostReceive(void *data, int capacity, int *peer) {
    ++hostReceives; (void)capacity;
    if (!queuedHost) return 0;
    memcpy(data, "host", 4); *peer = queuedHost; queuedHost = 0; return 4;
}
static void WebP2P_HostSend(int peer, const void *data, int length) { (void)data; hostPeer = peer; hostLength = length; }
static int WebP2P_ClientReceive(void *data, int capacity, int *slot) {
    (void)capacity;
    if (!queuedClient) return 0;
    memcpy(data, "peer", 4); *slot = queuedClient; queuedClient = 0; return 4;
}
static void WebP2P_ClientSend(int slot, const void *data, int length) { (void)data; clientSlot = slot; clientLength = length; }
static int WebNet_Receive(void *data, int capacity, int *room) {
    (void)capacity;
    if (!queuedRoom) return 0;
    memcpy(data, "room", 4); *room = queuedRoom - 1; queuedRoom = 0; return 4;
}
static void WebNet_Open(int room) { opened = room; }
static void WebNet_Send(const void *data, int length) { (void)data; (void)length; ++sent; }
''' + helpers + routing + r'''
static netadr_t ip(int a, int b, int c, int d, unsigned short port) {
    netadr_t address = {NA_IP, {a, b, c, d}, port};
    return address;
}
static void parsed(const char *name, int a, int b, int c, int d) {
    netadr_t address;
    assert(Sys_StringToAdr(name, &address));
    assert(address.type == NA_IP && address.ip[0] == a && address.ip[1] == b && address.ip[2] == c && address.ip[3] == d);
}
int main(void) {
    const char *rejected[] = {"127.0.0.0", "127.0.0.4", "127.0.0.12", "127.0.1.0", "127.0.1.01", "127.0.1.251",
        "127.0.1.2500", "127.0.1.", "127.0.1.7x", "10.66.0.1", "cod2master.activision.com"};
    netadr_t address;
    unsigned char buffer[16];
    msg_t message = {buffer, sizeof buffer, 0, 0};
    parsed("cod2-server", 127, 0, 0, 1);
    parsed("127.0.0.3", 127, 0, 0, 3);
    parsed("127.0.1.1", 127, 0, 1, 1);
    parsed("127.0.1.250", 127, 0, 1, 250);
    for (unsigned i = 0; i < sizeof rejected / sizeof *rejected; ++i) assert(!Sys_StringToAdr(rejected[i], &address));

    netadr_t loopback = {NA_LOOPBACK};
    assert(Sys_IsLANAddress(loopback));
    assert(Sys_IsLANAddress(ip(127, 0, 0, 2, 0x2071)) && !Sys_IsLANAddress(ip(127, 0, 0, 4, 0x2071)));
    assert(Sys_IsLANAddress(ip(127, 0, 1, 9, 0x2071)) && !Sys_IsLANAddress(ip(127, 0, 2, 9, 0x2071)));
    assert(Sys_IsLANAddress(ip(10, 66, 1, 2, 0x2071)) && !Sys_IsLANAddress(ip(10, 67, 1, 2, 0x2071)));
    assert(!Sys_IsLANAddress(ip(192, 168, 1, 1, 0x2071)));

    Sys_SendPacket(7, "payload", ip(10, 66, 1, 2, 0x2071));
    assert(hostPeer == 258 && hostLength == 7);
    Sys_SendPacket(5, "hello", ip(127, 0, 1, 5, 0x2071));
    assert(clientSlot == 5 && clientLength == 5);
    clientSlot = -1;
    Sys_SendPacket(5, "hello", ip(127, 0, 1, 5, 0x2171));
    Sys_SendPacket(5, "hello", ip(127, 0, 1, 251, 0x2071));
    assert(clientSlot == -1);
    Sys_SendPacket(5, "hello", ip(127, 0, 0, 2, 0x2071));
    assert(opened == 1 && sent == 1);
    Sys_SendPacket(5, "hello", ip(127, 0, 0, 4, 0x2071));
    Sys_SendPacket(5, "hello", ip(8, 8, 8, 8, 0x2071));
    Sys_SendPacket(0, "", ip(127, 0, 0, 1, 0x2071));
    assert(opened == 1 && sent == 1);

    queuedHost = 0x0102; queuedClient = 3; queuedRoom = 2;
    assert(Sys_GetPacket(&address, &message) && hostReceives == 0);
    assert(address.ip[0] == 127 && address.ip[2] == 1 && address.ip[3] == 3 && address.port == 0x2071);
    assert(Sys_GetPacket(&address, &message) && address.ip[2] == 0 && address.ip[3] == 2);
    assert(!Sys_GetPacket(&address, &message) && queuedHost == 0x0102);
    running.current.enabled = 1;
    queuedClient = 4;
    assert(Sys_GetPacket(&address, &message) && message.cursize == 4 && !memcmp(buffer, "host", 4));
    assert(address.ip[0] == 10 && address.ip[1] == 66 && address.ip[2] == 1 && address.ip[3] == 2 && address.port == 0x2071);
    assert(Sys_GetPacket(&address, &message) && address.ip[2] == 1 && address.ip[3] == 4);
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cod2-p2p-addresses-') as directory:
    path = Path(directory)
    (path / 'test.c').write_text(code)
    subprocess.run(['cc', '-std=c99', '-Wall', '-Wno-unused-function', '-fsanitize=address,undefined',
                    str(path / 'test.c'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('PASS: browser-hosted room and player addresses parse, count as LAN, route to P2P or dedicated transports, and host packets wait for a running server')
