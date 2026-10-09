#include "common_types.h"
#include <emscripten.h>
#include <string.h>

/* Room addresses are virtual 127.0.0.1..3 endpoints, never arbitrary UDP hosts.
 * A browser opens a gameplay socket only after selecting a native-menu room. */
extern void Com_Printf(const char *fmt, ...);

EM_JS(void, WebNet_Open, (int room), {
    let state = Module.cod2Transport;
    if (state && state.room === room &&
        (state.socket.readyState < 2 || state.failurePending)) return;
    if (state) { state.stopped = true; state.socket.close(1000); }
    const url = new URL('/game', location.href);
    url.searchParams.set('room', String(room));
    url.protocol = location.protocol === 'https:' ? 'wss:' : 'ws:';
    const socket = new WebSocket(url);
    socket.binaryType = 'arraybuffer';
    state = { socket, room, packets: [], bytes: 0, sends: [], sendBytes: 0,
        stopped: false, failurePending: false, opened: false, dropped: 0 };
    Module.cod2Transport = state;
    socket.onopen = () => {
        if (state.stopped) return;
        state.opened = true;
        out('[web-net] gateway connected');
        for (const packet of state.sends) socket.send(packet);
        state.sends.length = 0; state.sendBytes = 0;
    };
    socket.onmessage = event => {
        if (state.stopped || !(event.data instanceof ArrayBuffer)) return;
        const packet = new Uint8Array(event.data);
        if (!packet.length || packet.length > 65507) return;
        while (state.packets.length &&
               (state.packets.length >= 128 || state.bytes + packet.length > 262144)) {
            state.bytes -= state.packets.shift().length;
            ++state.dropped;
        }
        state.packets.push(packet);
        state.bytes += packet.length;
    };
    socket.onerror = () => err('[web-net] gateway connection failed');
    socket.onclose = async event => {
        state.packets.length = 0;
        state.bytes = 0;
        state.sends.length = 0;
        state.sendBytes = 0;
        if (state.stopped || Module.cod2Transport !== state) return;
        state.failurePending = true;
        err('[web-net] gateway closed: ' + event.code + ' ' + event.reason);
        let message = state.opened ? 'EXE_SERVER_DISCONNECTED' :
            'Could not connect to the game server. Please try again from Join Game.';
        if (!state.opened) {
            // Browsers hide a rejected WebSocket upgrade's HTTP status. Read the
            // same-origin room list to distinguish a full room from a lost host.
            const controller = new AbortController();
            const timeout = setTimeout(() => controller.abort(), 3000);
            try {
                const response = await fetch('/servers', {cache:'no-store', signal:controller.signal});
                if (response.ok) {
                    const list = await response.json();
                    const selected = list.rooms?.find(candidate => candidate.id === room);
                    if (!selected) message = 'This game is no longer available. Choose another server from Join Game.';
                    else {
                        const capacity = Number.isInteger(selected.maxPlayers) && selected.maxPlayers > 0
                            ? selected.maxPlayers : 64;
                        if (selected.players >= capacity || selected.connections >= capacity)
                            message = 'EXE_SERVERISFULL';
                    }
                }
            } catch (_) { /* Keep the connection error when the host is unreachable. */ }
            finally { clearTimeout(timeout); }
        }
        if (state.stopped || Module.cod2Transport !== state) return;
        // Deliver through the engine's normal packet loop. Calling Com_Error
        // directly from an asynchronous JS callback would bypass its frame guard.
        const text = new TextEncoder().encode('error\n' + message + '\0');
        const packet = new Uint8Array(4 + text.length);
        packet.fill(255, 0, 4);
        packet.set(text, 4);
        state.packets.push(packet);
        state.bytes = packet.length;
    };
});

EM_JS(int, WebNet_Receive, (void *destination, int capacity, int *room), {
    const state = Module.cod2Transport;
    if (!state) return 0;
    while (state.packets.length) {
        const packet = state.packets.shift();
        state.bytes -= packet.length;
        if (packet.length > capacity) { ++state.dropped; continue; }
        HEAPU8.set(packet, destination);
        HEAP32[room >> 2] = state.room;
        return packet.length;
    }
    return 0;
});

EM_JS(void, WebNet_Send, (const void *data, int length), {
    const state = Module.cod2Transport;
    if (!state) return;
    if (state.socket.readyState === WebSocket.CONNECTING) {
        if (state.sends.length < 16 && state.sendBytes + length <= 262144) {
            state.sends.push(HEAPU8.slice(data, data + length));
            state.sendBytes += length;
        } else ++state.dropped;
        return;
    }
    if (state.socket.readyState !== WebSocket.OPEN) return;
    if (state.socket.bufferedAmount > 262144) { ++state.dropped; return; }
    state.socket.send(HEAPU8.subarray(data, data + length));
});

qboolean Sys_IsLANAddress(netadr_t address) {
    return address.type == NA_LOOPBACK ||
        (address.type == NA_IP && address.ip[0] == 127 && !address.ip[1] && !address.ip[2] &&
         address.ip[3] >= 1 && address.ip[3] <= 3);
}

void Sys_ShowIP(void) { Com_Printf("Browser transport: same-host WebSocket gateway\n"); }

qboolean Sys_StringToAdr(const char *name, netadr_t *address) {
    int last = 1;
    if (strcmp(name, "cod2-server")) {
        if (strncmp(name, "127.0.0.", 8) || name[8] < '1' || name[8] > '3' || name[9]) return 0;
        last = name[8] - '0';
    }
    memset(address, 0, sizeof(*address));
    address->type = NA_IP;
    address->ip[0] = 127;
    address->ip[3] = last;
    return 1;
}

qboolean Sys_GetPacket(netadr_t *from, msg_t *message) {
    int room = 0;
    int size = WebNet_Receive(message->data, message->maxsize, &room);
    if (!size) return 0;
    Sys_StringToAdr("cod2-server", from);
    from->ip[3] = room + 1;
    from->port = 0x2071; /* network byte order, UDP 28960 */
    message->cursize = size;
    message->readcount = 0;
    return 1;
}

void Sys_SendPacket(int length, const void *data, netadr_t to) {
    if (length <= 0 || length > 65507 || to.type != NA_IP ||
        to.ip[0] != 127 || to.ip[1] || to.ip[2] || to.ip[3] < 1 || to.ip[3] > 3 || to.port != 0x2071) return;
    WebNet_Open(to.ip[3] - 1);
    WebNet_Send(data, length);
}

void NET_Config(qboolean enabled) {
    if (!enabled) EM_ASM({
        const state = Module.cod2Transport;
        if (state) { state.stopped = true; state.socket.close(1000); }
        Module.cod2Transport = null;
    });
}
void NET_OpenIP(void) { }
void NET_Init(void) {
    extern int g_qport;
    /* Each browser engine starts its relative clock near zero. Assign an
     * independent 16-bit channel id before CL_Init copies it, so browsers
     * behind the UDP gateway do not all identify as the same client. */
    g_qport = EM_ASM_INT({
        if (globalThis.crypto?.getRandomValues)
            return globalThis.crypto.getRandomValues(new Uint16Array(1))[0];
        return Math.floor(Math.random() * 65536);
    });
}
void NET_Restart(void) { NET_Config(0); }
/* Browser frame scheduling performs the wait, allowing WebSocket callbacks. */
void NET_Sleep(int milliseconds) { (void)milliseconds; }

extern clientStatic_t cls;
extern void CL_SetServerInfo(serverInfo_t *server, const char *info, int ping);
extern void Cbuf_ExecuteText(int when, const char *text);
extern char *va(const char *format, ...);
extern void Dvar_SetStringByName(const char *name, const char *value);
extern qboolean UI_SetActiveMenu(int menu);

EMSCRIPTEN_KEEPALIVE void web_add_room(int room, const char *info, int ping) {
    if (room < 0 || room > 2 || cls.numlocalservers >= 128) return;
    serverInfo_t *server = &cls.localServers[cls.numlocalservers++];
    memset(server, 0, sizeof(*server));
    Sys_StringToAdr("cod2-server", &server->adr);
    server->adr.ip[3] = room + 1;
    server->adr.port = 0x2071;
    server->dirty = 1;
    CL_SetServerInfo(server, info, ping > 0 ? ping : 1);
}

EM_JS(void, Web_DiscoverServers, (), {
    const generation = (Module.roomListGeneration || 0) + 1;
    Module.roomListGeneration = generation;
    // Cached clients can reach Join Game while the dedicated room is still
    // starting. An empty first reply must not leave discovery stuck forever.
    return (async () => {
        const deadline = Date.now() + 12000;
        for (let attempt = 0; attempt < 16; ++attempt) {
            if (Module.roomListGeneration !== generation) return;
            const remaining = deadline - Date.now();
            if (remaining <= 0) return;
            const controller = new AbortController();
            const timeout = setTimeout(() => controller.abort(), Math.min(3000, remaining));
            try {
                const response = await fetch('/servers', {cache:'no-store', signal:controller.signal});
                if (!response.ok) throw new Error('Server list unavailable');
                const list = await response.json();
                if (Module.roomListGeneration !== generation) return;
                if (!Array.isArray(list.rooms)) throw new Error('Invalid server list');
                const seen = new Set();
                for (const room of list.rooms) {
                    if (!room || !Number.isInteger(room.id) || room.id < 0 || room.id > 2 ||
                        typeof room.info !== 'string' || seen.has(room.id)) continue;
                    seen.add(room.id);
                    const info = stringToNewUTF8(room.info.slice(0, 1023));
                    try { _web_add_room(room.id, info, room.ping); }
                    finally { _free(info); }
                }
                if (seen.size) return;
            } catch (error) {
                if (Module.roomListGeneration !== generation) return;
                if (attempt === 15 || Date.now() >= deadline) err('[servers] ' + error.message);
            } finally { clearTimeout(timeout); }
            if (attempt < 15 && Module.roomListGeneration === generation && Date.now() < deadline)
                await new Promise(resolve => setTimeout(resolve, Math.min(750, deadline - Date.now())));
        }
    })();
});

EMSCRIPTEN_KEEPALIVE void web_created_room(int room, const char *error) {
    if (room < 0 || room > 2) {
        Dvar_SetStringByName("com_errorMessage", error);
        UI_SetActiveMenu(1);
        return;
    }
    Cbuf_ExecuteText(2, va("connect 127.0.0.%d:28960\n", room + 1));
}

EM_JS(void, Web_CreateServer, (const char *name), {
    if (Module.roomCreationPending) return;
    Module.roomCreationPending = true;
    fetch('/servers', {method:'POST', headers:{'Content-Type':'application/json'},
        body:JSON.stringify({name:UTF8ToString(name).slice(0, 128)})})
      .then(async response => {
        const result = await response.json();
        if (!response.ok) throw new Error(result.error || 'No se pudo crear la partida.');
        if (!Number.isInteger(result.id) || result.id < 1 || result.id > 2)
            throw new Error('Respuesta de partida inválida.');
        _web_created_room(result.id, 0);
      }).catch(error => {
        const message = stringToNewUTF8(String(error.message).slice(0, 512));
        _web_created_room(-1, message);
        _free(message);
      }).finally(() => { Module.roomCreationPending = false; });
});
