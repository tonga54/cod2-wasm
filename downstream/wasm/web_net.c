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
    // Runs in a worker: the gateway's packets go into the shared ring as they
    // arrive, each after its length (4 bytes, little-endian); the page's go
    // out as it posts them.
    function receiver() {
        let socket = null, meta = null, ring = null;
        self.onmessage = ({data}) => {
            if (data.url) {
                meta = new Int32Array(data.ring, 0, 4);
                ring = new Uint8Array(data.ring, 16);
                socket = new WebSocket(data.url);
                socket.binaryType = 'arraybuffer';
                socket.onopen = () => self.postMessage({event: 'open'});
                socket.onerror = () => self.postMessage({event: 'error'});
                socket.onclose = event => self.postMessage({event: 'close', code: event.code, reason: event.reason});
                socket.onmessage = event => {
                    if (!(event.data instanceof ArrayBuffer)) return;
                    const packet = new Uint8Array(event.data), size = ring.length, n = packet.length;
                    const write = Atomics.load(meta, 0);
                    if (!n || n > 65507) return;
                    if (((write - Atomics.load(meta, 1)) >>> 0) + 4 + n > size) { Atomics.add(meta, 2, 1); return; }
                    const put = (offset, chunk) => {
                        const at = (offset >>> 0) % size, first = Math.min(chunk.length, size - at);
                        ring.set(chunk.subarray(0, first), at);
                        ring.set(chunk.subarray(first), 0);
                    };
                    put(write, new Uint8Array([n & 255, n >> 8 & 255, n >> 16 & 255, n >>> 24]));
                    put(write + 4, packet);
                    Atomics.store(meta, 0, write + 4 + n);
                    if (Atomics.load(meta, 3)) self.postMessage({event: 'packet'});
                };
            } else if (data.packet) {
                if (socket.readyState !== 1) return;
                if (socket.bufferedAmount > 8192) { Atomics.add(meta, 2, 1); return; }
                socket.send(data.packet);
            } else if (data.close) socket.close(data.close);
        };
    }
    // A socket for the code below whose packets reach the frame through the
    // ring, held by a worker; if the worker never reaches the gateway, the
    // page connects itself and its packets take the usual path.
    function workerSocket(href) {
        const shared = new SharedArrayBuffer(16 + 262144);
        const source = URL.createObjectURL(new Blob(['(' + receiver + ')();'], {type: 'text/javascript'}));
        let worker = null, direct = null, ended = false;
        const socket = { readyState: 0, bufferedAmount: 0, onopen: null, onmessage: null, onerror: null, onclose: null,
            ring: { meta: new Int32Array(shared, 0, 4), bytes: new Uint8Array(shared, 16) },
            send(packet) {
                if (direct) { direct.send(packet); return; }
                const copy = packet.slice();
                worker.postMessage({packet: copy}, [copy.buffer]);
            },
            close(code) {
                socket.readyState = Math.max(socket.readyState, 2);
                if (direct) direct.close(code);
                else { worker.postMessage({close: code}); setTimeout(() => worker.terminate(), 2000); }
            } };
        const closed = event => {
            if (ended) return;
            ended = true;
            socket.readyState = 3;
            Atomics.store(socket.ring.meta, 1, Atomics.load(socket.ring.meta, 0));
            socket.onclose?.(event);
        };
        const fallback = () => {
            if (direct || ended) return;
            worker.terminate();
            URL.revokeObjectURL(source);
            if (socket.readyState) { closed({code: 1006, reason: ''}); return; }
            direct = new WebSocket(href);
            direct.binaryType = 'arraybuffer';
            Object.defineProperty(socket, 'bufferedAmount', {get: () => direct.bufferedAmount});
            direct.onopen = () => { socket.readyState = 1; socket.onopen?.(); };
            direct.onmessage = event => socket.onmessage?.(event);
            direct.onerror = () => socket.onerror?.();
            direct.onclose = closed;
        };
        worker = new Worker(source);
        if (!Module.cod2NetVisibility && typeof document !== 'undefined') {
            // The worker wakes a hidden page's frames with its packets.
            Module.cod2NetVisibility = () => {
                const ring = Module.cod2Transport?.socket.ring;
                if (ring) Atomics.store(ring.meta, 3, document.hidden ? 1 : 0);
            };
            document.addEventListener('visibilitychange', Module.cod2NetVisibility);
        }
        worker.onmessage = ({data}) => {
            if (data.event === 'packet') Module.cod2BackgroundPump?.();
            else if (data.event === 'open') { URL.revokeObjectURL(source); socket.readyState = 1; socket.onopen?.(); }
            else if (data.event === 'error') { if (socket.readyState) socket.onerror?.(); }
            else if (data.event === 'close') {
                if (!socket.readyState) { fallback(); return; }
                worker.terminate();
                closed(data);
            }
        };
        worker.onerror = event => { event.preventDefault?.(); fallback(); };
        Atomics.store(socket.ring.meta, 3, typeof document !== 'undefined' && document.hidden ? 1 : 0);
        worker.postMessage({url: href, ring: shared});
        return socket;
    }
    // With memory the page can share, a worker receives: its packets arrive as
    // they come, even while the browser holds the page's own tasks back, as it
    // does on a busy main thread while the mouse drags (firing while turning).
    let socket = null;
    if (globalThis.crossOriginIsolated && typeof SharedArrayBuffer === 'function' && typeof Worker === 'function') {
        try { socket = workerSocket(url.href); } catch (_) { socket = null; }
    }
    if (!socket) {
        socket = new WebSocket(url);
        socket.binaryType = 'arraybuffer';
    }
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
        Module.cod2BackgroundPump?.();
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
    const ring = state.socket.ring;
    if (!ring) return 0;
    const meta = ring.meta, bytes = ring.bytes, size = bytes.length;
    let read = Atomics.load(meta, 1);
    while (read !== Atomics.load(meta, 0)) {
        const at = (read >>> 0) % size;
        const length = (bytes[at] | bytes[(at + 1) % size] << 8 | bytes[(at + 2) % size] << 16 |
            bytes[(at + 3) % size] << 24) >>> 0;
        const start = (at + 4) % size, first = Math.min(length, size - start);
        read = (read + 4 + length) | 0;
        if (length > capacity) { Atomics.store(meta, 1, read); ++state.dropped; continue; }
        HEAPU8.set(bytes.subarray(start, start + first), destination);
        HEAPU8.set(bytes.subarray(0, length - first), destination + first);
        Atomics.store(meta, 1, read);
        HEAP32[room >> 2] = state.room;
        return length;
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
    /* Commands are sent ~60 times a second and each repeats the last ones, so
     * a stalled uplink drops them as UDP would rather than queue seconds of
     * stale input behind the stall. */
    if (state.socket.bufferedAmount > 8192) { ++state.dropped; return; }
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
                Module.roomListInstances = new Map(list.rooms.filter(room => room && Number.isInteger(room.id))
                    .map(room => [room.id, room.instance]));
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

EM_JS(void, Web_CreateServer, (const char *name, const char *mapName, const char *gameType, int botCount, int botDifficulty, int timeLimit, int scoreLimit), {
    if (Module.roomCreationPending) return;
    Module.roomCreationPending = true;
    fetch('/servers', {method:'POST', headers:{'Content-Type':'application/json'},
        body:JSON.stringify({name:UTF8ToString(name).slice(0, 128), map:UTF8ToString(mapName),
            gametype:UTF8ToString(gameType), botCount:botCount,
            botDifficulty:['easy', 'normal', 'hard'][botDifficulty] || 'normal',
            timeLimit:timeLimit >= 0 ? timeLimit : undefined, scoreLimit:scoreLimit >= 0 ? scoreLimit : undefined})})
      .then(async response => {
        const result = await response.json();
        if (!response.ok) throw new Error(result.error || 'Unable to create the server.');
        if (!Number.isInteger(result.id) || result.id < 0 || result.id > 2)
            throw new Error('Invalid server response.');
        if (/^[0-9a-f]{64}$/.test(result.ownerToken) && /^[0-9a-f]{32}$/.test(result.instance)) {
            const ownership = {instance:result.instance, ownerToken:result.ownerToken};
            (Module.ownedRooms ||= new Map()).set(result.id, ownership);
            try { localStorage.setItem('cod2-owned-room-' + result.id, JSON.stringify(ownership)); } catch (_) {}
        }
        _web_created_room(result.id, 0);
      }).catch(error => {
        const message = stringToNewUTF8(String(error.message).slice(0, 512));
        _web_created_room(-1, message);
        _free(message);
      }).finally(() => { Module.roomCreationPending = false; });
});

EM_JS(int, Web_CanDeleteServer, (const char *address), {
    const match = /^127\.0\.0\.([1-3]):28960$/.exec(UTF8ToString(address));
    if (!match) return 0;
    const room = Number(match[1]) - 1;
    Module.ownedRooms ||= new Map();
    if (!Module.ownedRooms.has(room)) {
        try { Module.ownedRooms.set(room, JSON.parse(localStorage.getItem('cod2-owned-room-' + room))); } catch (_) {}
    }
    const owned = Module.ownedRooms.get(room);
    return Boolean(owned && owned.instance === Module.roomListInstances?.get(room) &&
        /^[0-9a-f]{64}$/.test(owned.ownerToken));
});

EMSCRIPTEN_KEEPALIVE void web_deleted_room(int room, const char *error) {
    if (room < 0) {
        Dvar_SetStringByName("com_errorMessage", error);
        UI_SetActiveMenu(1);
    } else {
        extern void UI_RunMenuScript(const char **args);
        const char *refresh = "UpdateFilter";
        UI_RunMenuScript(&refresh);
    }
}

EM_JS(void, Web_DeleteServer, (const char *address), {
    if (Module.roomDeletionPending) return;
    const match = /^127\.0\.0\.([1-3]):28960$/.exec(UTF8ToString(address));
    const room = match ? Number(match[1]) - 1 : -1;
    const owned = Module.ownedRooms?.get(room);
    const fail = error => {
        const message = stringToNewUTF8(String(error.message).slice(0, 512));
        try { _web_deleted_room(-1, message); } finally { _free(message); }
    };
    if (!owned || owned.instance !== Module.roomListInstances?.get(room)) {
        fail(new Error('You can only delete servers created in this browser.')); return;
    }
    Module.roomDeletionPending = true;
    fetch('/servers', {method:'DELETE', headers:{'Content-Type':'application/json'},
        body:JSON.stringify({id:room, ownerToken:owned.ownerToken})})
      .then(async response => {
        const result = await response.json();
        if (!response.ok) throw new Error(result.error || 'Unable to delete the server.');
        Module.ownedRooms.delete(room);
        Module.roomListInstances.delete(room);
        try { localStorage.removeItem('cod2-owned-room-' + room); } catch (_) {}
        _web_deleted_room(room, 0);
      }).catch(fail).finally(() => { Module.roomDeletionPending = false; });
});
