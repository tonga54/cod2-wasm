#!/usr/bin/env python3
"""A browser that goes away leaves its match at once.

When a browser's WebSocket closes, the gateway sends "disconnect" out of band
from the UDP port it gave that browser (downstream/gateway/filter.test.mjs);
the engine used to ignore it and kept the player for sv_timeout. This runs the
production SV_ConnectionlessPacket, SV_DisconnectPacket and NET_CompareAdr with
the engine calls stubbed: the player at that exact address is dropped as having
disconnected, connecting or in the game; a browser on another port of the same
gateway, a bot, free and zombie slots and a slot past sv_maxclients are left
alone, and no other command drops anyone.
"""
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
server = (root / 'src/PC/server_mp/sv_main_mp.c').read_text()
network = (root / 'src/PC/qcommon/net_chan_mp.c').read_text()


def function(text, name):
    match = re.search(r'[^\n;]*\b' + name + r'\([^;{]*?\)\n\{', text)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end] + '\n'


compare = function(network, 'NET_CompareAdr')
leave = function(server, 'SV_DisconnectPacket')
packet = function(server, 'SV_ConnectionlessPacket')

support = r'''
#include <stdio.h>
#include <string.h>
#include <strings.h>
typedef int qboolean;
typedef unsigned char byte;
typedef enum { NA_BOT = 0, NA_BAD = 1, NA_LOOPBACK = 2, NA_BROADCAST = 3, NA_IP = 4 } netadrtype_t;
typedef struct { netadrtype_t type; byte ip[4]; unsigned short port; } netadr_t;
typedef struct { netadr_t remoteAddress; } netchan_t;
typedef struct { int state; netchan_t netchan; } client_t;
typedef struct { client_t *clients; } serverStatic_t;
typedef struct { int cursize; char *text; } msg_t;
typedef struct { struct { int integer; int enabled; } current; } dvar_t;
#define COD2_IS_PATCH_13 1

#define SLOTS 9
static client_t clients[SLOTS];
static serverStatic_t svs = { clients };
static dvar_t maxclientsDvar = { { SLOTS - 1, 0 } }, packetInfoDvar = { { 0, 1 } };
static const dvar_t *sv_maxclients = &maxclientsDvar, *sv_packet_info = &packetInfoDvar;
static char calls[512];
static char token[64];

static void call(const char *text)
{
    if (calls[0])
        strcat(calls, " ");
    strcat(calls, text);
}

void Com_Printf(const char *fmt, ...) { (void)fmt; }
void MSG_BeginReading(msg_t *msg) { (void)msg; }
int MSG_ReadLong(msg_t *msg) { (void)msg; return -1; }
void SV_Netchan_AddOOBProfilePacket(int size) { (void)size; }
char *MSG_ReadStringLine(msg_t *msg) { return msg->text; }
void SV_Cmd_TokenizeString(const char *text) { sscanf(text, "%63s", token); }
const char *SV_Cmd_Argv(int arg) { return arg ? "" : token; }
int I_strnicmp(const char *a, const char *b, int n) { return strncasecmp(a, b, n); }
int I_stricmp(const char *a, const char *b) { return strcasecmp(a, b); }
const char *NET_AdrToString(netadr_t a) { (void)a; return "address"; }
void PB_HandleServerOobPacket(netadr_t *from, msg_t *msg) { (void)from; (void)msg; call("pb"); }
void SV_VoicePacket(netadr_t from, msg_t *msg) { (void)from; (void)msg; call("voice"); }
void SVC_Status(netadr_t from) { (void)from; call("status"); }
void SVC_Info(netadr_t from) { (void)from; call("info"); }
void SV_GetChallenge(netadr_t from) { (void)from; call("challenge"); }
void SV_DirectConnect(netadr_t from) { (void)from; call("connect"); }
void SV_AuthorizeIpPacket(netadr_t from) { (void)from; call("authorize"); }
void SVC_RemoteCommand(netadr_t from, msg_t *msg) { (void)from; (void)msg; call("rcon"); }

/* As the engine's: the client goes to CS_ZOMBIE. */
void SV_DropClient(client_t *drop, const char *reason)
{
    char text[64];
    snprintf(text, sizeof text, "drop%d:%s", (int)(drop - clients), reason);
    call(text);
    drop->state = 1;
}
'''

checks = r'''
static netadr_t gateway(int last, int port)
{
    netadr_t address;
    memset(&address, 0, sizeof address);
    address.type = NA_IP;
    address.ip[0] = 172;
    address.ip[1] = 18;
    address.ip[3] = (byte)last;
    address.port = (unsigned short)port;
    return address;
}

static void receive(netadr_t from, const char *text)
{
    char line[64];
    msg_t msg;
    snprintf(line, sizeof line, "%s", text);
    msg.cursize = 4 + (int)strlen(line);
    msg.text = line;
    SV_ConnectionlessPacket(from, &msg);
}

int main(void)
{
    /* Free and zombie slots that had the leaving browser's address, another
     * browser on the same gateway, a bot, the leaving browser, a browser
     * still connecting, the same port on another host, one more browser, and
     * past sv_maxclients a browser that is not checked. */
    static const int state[SLOTS] = { 0, 1, 4, 4, 4, 2, 4, 3, 4 };
    static const int host[SLOTS] = { 5, 5, 5, 0, 5, 5, 6, 5, 5 };
    static const int port[SLOTS] = { 4000, 4000, 4001, 4000, 4000, 4002, 4000, 4003, 4004 };
    static const int want[SLOTS] = { 0, 1, 1, 4, 1, 1, 4, 3, 4 };
    const char *order = "info drop4:EXE_DISCONNECTED drop5:EXE_DISCONNECTED rcon connect challenge "
                        "drop2:EXE_DISCONNECTED";
    int failures = 0;
    int i;

    for (i = 0; i < SLOTS; i++) {
        clients[i].state = state[i];
        clients[i].netchan.remoteAddress = gateway(host[i], port[i]);
    }
    clients[3].netchan.remoteAddress.type = NA_BOT;
    memset(clients[3].netchan.remoteAddress.ip, 0, 4);

    receive(gateway(5, 4000), "getinfo xyz");
    receive(gateway(5, 4000), "disconnect");
    receive(gateway(5, 4000), "DISCONNECT");
    receive(gateway(5, 4002), "disconnect now");
    receive(gateway(5, 4004), "disconnect");
    receive(gateway(5, 4003), "disconnected");
    receive(gateway(5, 4003), "rcon secret status");
    receive(gateway(5, 4003), "connect \"\\name\\x\"");
    receive(gateway(5, 4003), "getchallenge");
    receive(gateway(5, 4001), "Disconnect");

    if (strcmp(calls, order)) {
        printf("calls %s\nwant  %s\n", calls, order);
        failures++;
    }
    for (i = 0; i < SLOTS; i++) {
        if (clients[i].state != want[i]) {
            printf("client %d: state %d, want %d\n", i, clients[i].state, want[i]);
            failures++;
        }
    }
    return failures != 0;
}
'''

mutants = [
    # As it was: the command was read and ignored.
    ('    if (I_stricmp(c, "disconnect") == 0)\n        SV_DisconnectPacket(from);\n', '    I_stricmp(c, "disconnect");\n'),
    # The gateway's address alone, as sequenced packets are matched before the qport.
    ('NET_CompareAdr(from, cl->netchan.remoteAddress)',
     '(from.type == cl->netchan.remoteAddress.type && !memcmp(from.ip, cl->netchan.remoteAddress.ip, 4))'),
    ('cl->state >= 2 && NET_CompareAdr', 'cl->state >= 1 && NET_CompareAdr'),
    ('cl->state >= 2 && NET_CompareAdr', 'cl->state >= 3 && NET_CompareAdr'),
    ('            SV_DropClient(cl, "EXE_DISCONNECTED");', '            SV_DropClient(cl, "EXE_TIMEDOUT");'),
    ('i < sv_maxclients->current.integer; ++i, ++cl) {\n        if (cl->state >= 2',
     'i <= sv_maxclients->current.integer; ++i, ++cl) {\n        if (cl->state >= 2'),
]

with tempfile.TemporaryDirectory(prefix='cod2-closed-browser-') as directory:
    path = Path(directory)
    for index, (before, after) in enumerate([(None, None)] + mutants):
        leaving, receiving = leave, packet
        if before is not None:
            source = leave + packet
            assert source.count(before) == 1, before
            leaving, receiving = leave.replace(before, after), packet.replace(before, after)
        (path / 'test.c').write_text(support + compare + leaving + receiving + checks)
        flags = ['-Wall', '-Werror'] if before is None else ['-w']
        subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        *flags, str(path / 'test.c'), '-o', str(path / 'test')], check=True)
        result = subprocess.run([str(path / 'test')], capture_output=True, text=True)
        if before is None:
            assert result.returncode == 0, result.stdout + result.stderr
        else:
            assert result.returncode == 1, f'mutant survived or crashed: {after!r}\n{result.stdout}{result.stderr[-2000:]}'

print('PASS: "disconnect" from the exact UDP address of a connecting or playing browser drops it as disconnected, '
      'once; other browsers behind the same gateway, bots, free and zombie slots, slots past sv_maxclients and every '
      f'other command drop nobody; {len(mutants)} mutants fail')
