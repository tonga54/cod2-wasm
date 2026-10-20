import http from 'node:http';
import dgram from 'node:dgram';
import { readFile } from 'node:fs/promises';
import { pathToFileURL } from 'node:url';
import WebSocket, { WebSocketServer } from 'ws';
import { createUpdateChecker, readBuildInfo } from './updates.mjs';
import { createMaintenanceCheck } from './maintenance.mjs';
import { ClassError, createClassStore, defaultClasses, normalizeClasses, playerKey } from './classes.mjs';
import { createCdnRedirects, readAssetPolicy } from './cdn.mjs';

// Start New Server's rules, as ui_match_menu.c offers and rooms.py accepts
// them: minutes, and each mode's score; 0 for none.
export const TIME_LIMITS = [0, 5, 10, 15, 20, 30, 45, 60];
export const SCORE_LIMITS = { dm: [0, 10, 20, 30, 50, 75, 100, 150], tdm: [0, 25, 50, 75, 100, 150, 200, 300],
  ctf: [0, 1, 3, 5, 7, 10, 15, 20], hq: [0, 60, 120, 180, 300, 450, 600, 900], sd: [0, 3, 4, 5, 6, 7, 10, 15] };

// The connectionless commands a browser client sends its game server
// (cl_main_mp.c, cl_main_pc_mp.c). rcon, voice, authorization and anything
// else reaching SV_ConnectionlessPacket is dropped here.
export const CLIENT_OOB_COMMANDS = new Set(['getchallenge', 'connect', 'getinfo', 'getstatus']);
// The client netchan sends at most 1400 bytes; connect carries ~1 KiB.
export const MAX_CLIENT_DATAGRAM = 2048;
// A client sends ~60 commands a second. Budgets well above that drop a flood
// as UDP loss before it reaches the single-threaded engine.
export const CLIENT_BUDGET = Object.freeze({ packets: [200, 400], bytes: [131072, 262144], oob: [10, 20],
  closeAfterDrops: 1000, dropWindowMs: 10000 });
// Snapshots queued behind a stalled TCP connection are stale; past this many
// bytes they are dropped as UDP would, instead of arriving seconds late.
export const SERVER_BACKLOG_BYTES = 65536;

// SV_ConnectionlessPacket's command: the bytes after 0xffffffff up to the
// first whitespace or control byte. Anything odd (quotes, leading spaces) is
// simply not an allowed command. null when the datagram is sequenced.
export function oobCommand(data) {
  if (data.length < 4 || data.readInt32LE(0) !== -1) return null;
  let end = 4;
  while (end < data.length && end < 20 && data[end] > 32) ++end;
  return data.toString('latin1', 4, end).toLowerCase();
}

function tokenBucket([rate, burst], now) {
  let tokens = burst, last = now();
  return cost => {
    const time = now();
    tokens = Math.min(burst, tokens + (time - last) * rate / 1000);
    last = time;
    if (tokens < cost) return false;
    tokens -= cost;
    return true;
  };
}

export function createGateway({ webHost = 'cod2-web', webPort = 8088,
  udpHost = 'cod2-server', udpPort = 28960, maxClients = 64,
  iconPath = '/game-ui/cod2.ico', startupPath = '/game-ui/cod2-startup.png',
  roomsEnabled = false, managerPort = 8090, updateChecker, maintenanceCheck = createMaintenanceCheck(),
  classStore = null, cdnRedirect = null, udpSocketFactory = () => dgram.createSocket('udp4'),
  clientBudget = CLIENT_BUDGET, now = () => performance.now() } = {}) {
  const reservations = new Map();
  const artworkCache = new Map();
  // Drawing the install icons holds the event loop that forwards game packets
  // for about 0.1 s, so it is done once, as the gateway starts.
  let appIcons = null;
  function drawAppIcons() {
    appIcons ||= Promise.all([import('./app-icons.mjs'), readFile(iconPath), readFile(startupPath)])
      .then(([{ renderAppIcons }, icon, logo]) => renderAppIcons(icon, logo));
    appIcons.catch(() => { appIcons = null; });
    return appIcons;
  }
  drawAppIcons().catch(() => {});
  const wsBinaryOptions = Object.freeze({ binary: true, compress: false });
  function roomConnections(roomId) {
    let count = reservations.get(roomId) || 0;
    for (const client of wss.clients) if (client.roomId === roomId) ++count;
    return count;
  }
  const checkUpdates = updateChecker || createUpdateChecker({
    readBuild: () => readBuildInfo(webHost, webPort)
  });
  function sameOrigin(request) {
    try {
      const origin = new URL(request.headers.origin);
      return ['http:', 'https:'].includes(origin.protocol) && origin.host === request.headers.host;
    } catch { return false; }
  }
  function manager(method, body) {
    return new Promise((resolve, reject) => {
      const encoded = body === undefined ? null : Buffer.from(JSON.stringify(body));
      const upstream = http.request({ hostname: udpHost, port: managerPort, path: '/rooms', method,
        headers: encoded ? { 'Content-Type': 'application/json', 'Content-Length': encoded.length } : {},
        timeout: method === 'POST' ? 30000 : method === 'DELETE' ? 6000 : 2000 }, reply => {
        const chunks = []; let bytes = 0;
        reply.on('data', chunk => {
          bytes += chunk.length;
          if (bytes > 16384) reply.destroy(new Error('Room response too large'));
          else chunks.push(chunk);
        });
        reply.on('error', reject);
        reply.on('end', () => {
          try { resolve({ status: reply.statusCode, value: JSON.parse(Buffer.concat(chunks)) }); }
          catch (error) { reject(error); }
        });
      });
      upstream.on('timeout', () => upstream.destroy(new Error('Room manager timed out')));
      upstream.on('error', reject);
      upstream.end(encoded);
    });
  }
  function json(response, status, value) {
    response.writeHead(status, { 'Content-Type': 'application/json', 'Cache-Control': 'no-store' });
    response.end(JSON.stringify(value));
  }
  async function handleClasses(request, response, url) {
    if (request.method === 'GET') {
      const key = playerKey(url.searchParams.get('player'));
      if (!key) { json(response, 400, {error:'Choose a gametag in Options to save classes.'}); return; }
      if (!classStore) { json(response, 503, {error:'Classes are unavailable.'}); return; }
      const classes = classStore.get(key);
      json(response, 200, {classes:classes || defaultClasses(), saved:Boolean(classes)});
      return;
    }
    if (request.method !== 'PUT') { json(response, 405, {error:'Unsupported method'}); return; }
    if (!sameOrigin(request)) { json(response, 403, {error:'Invalid origin'}); return; }
    const chunks = []; let bytes = 0;
    for await (const chunk of request) {
      bytes += chunk.length;
      if (bytes > 4096) { json(response, 413, {error:'Request too large'}); return; }
      chunks.push(chunk);
    }
    let body;
    try { body = JSON.parse(Buffer.concat(chunks)); }
    catch { json(response, 400, {error:'Invalid JSON'}); return; }
    const key = playerKey(body?.player);
    if (!key) { json(response, 400, {error:'Choose a gametag in Options to save classes.'}); return; }
    let classes;
    try { classes = normalizeClasses(body.classes); }
    catch (error) {
      if (error instanceof ClassError) { json(response, 400, {error:error.message}); return; }
      throw error;
    }
    if (!classStore) { json(response, 503, {error:'Classes are unavailable.'}); return; }
    classStore.put(key, classes);
    json(response, 200, {classes, saved:true});
  }
  function proxy(request, response) {
    const upstream = http.request({ hostname: webHost, port: webPort,
      method: request.method, path: request.url, headers: request.headers }, reply => {
      response.writeHead(reply.statusCode, { 'x-frame-options': 'DENY', 'referrer-policy': 'same-origin',
        ...reply.headers });
      reply.pipe(response);
      reply.on('error', () => response.destroy());
    });
    upstream.on('error', () => {
      if (!response.headersSent) response.writeHead(502, { 'Content-Type': 'text/plain' });
      response.end('Game web server unavailable');
    });
    request.on('aborted', () => upstream.destroy());
    response.on('close', () => { if (!response.writableFinished) upstream.destroy(); });
    request.pipe(upstream);
  }
  const server = http.createServer((request, response) => {
    let url = null;
    try { url = new URL(request.url, 'http://gateway'); } catch {}
    if (url?.pathname === '/classes') {
      handleClasses(request, response, url)
        .catch(() => { if (!response.headersSent) json(response, 503, {error:'Classes are unavailable.'}); });
      return;
    }
    if (request.url === '/version') {
      if (request.method !== 'GET') {
        json(response, 405, { error: 'Unsupported method' }); return;
      }
      checkUpdates().then(result => json(response, 200, result))
        .catch(() => json(response, 503, { error: 'Version check unavailable' }));
      return;
    }
    if (roomsEnabled && request.url === '/servers') {
      if (request.method === 'POST' && maintenanceCheck()) {
        json(response, 503, {error:'The server is updating. Please try again shortly.'}); return;
      }
      if (!['GET', 'POST', 'DELETE'].includes(request.method)) {
        json(response, 405, { error: 'Unsupported method' }); return;
      }
      if (request.method !== 'GET' && !sameOrigin(request)) {
        json(response, 403, { error: 'Invalid origin' }); return;
      }
      (async () => {
        let body;
        if (request.method !== 'GET') {
          const chunks = []; let bytes = 0;
          for await (const chunk of request) {
            bytes += chunk.length;
            if (bytes > 1024) { json(response, 413, {error:'Request too large'}); return; }
            chunks.push(chunk);
          }
          try { body = JSON.parse(Buffer.concat(chunks)); }
          catch { json(response, 400, {error:'Invalid JSON'}); return; }
          if (request.method === 'DELETE') {
            if (!body || !Number.isInteger(body.id) || body.id < 0 || body.id > 2 ||
                typeof body.ownerToken !== 'string' || !/^[0-9a-f]{64}$/.test(body.ownerToken)) {
              json(response, 400, {error:'Invalid server.'}); return;
            }
            body = {id:body.id, ownerToken:body.ownerToken};
          } else {
            if (!body || typeof body.name !== 'string' || body.name.length > 128) {
              json(response, 400, {error:'Invalid server name.'}); return;
            }
            const map = body.map ?? 'mp_toujane';
            if (!['mp_toujane', 'mp_carentan', 'mp_dawnville', 'mp_burgundy', 'mp_railyard', 'mp_brecourt'].includes(map)) {
              json(response, 400, {error:'Choose Toujane, Carentan, Dawnville, Burgundy, Stalingrad or Brecourt.'}); return;
            }
            const gametype = body.gametype === undefined ? 'tdm' : body.gametype;
            if (!['dm', 'tdm', 'ctf', 'hq', 'sd'].includes(gametype)) {
              json(response, 400, {error:'Choose a supported game mode.'}); return;
            }
            const botCount = body.botCount === undefined ? 0 : body.botCount;
            const botDifficulty = body.botDifficulty === undefined ? 'normal' : body.botDifficulty;
            if (!Number.isInteger(botCount) || botCount < 0 || botCount > 32) {
              json(response, 400, {error:'Choose between 0 and 32 bots.'}); return;
            }
            if (!['easy', 'normal', 'hard'].includes(botDifficulty)) {
              json(response, 400, {error:'Choose Easy, Normal or Hard bots.'}); return;
            }
            const { timeLimit, scoreLimit } = body;
            if ((timeLimit !== undefined && !TIME_LIMITS.includes(timeLimit)) ||
                (scoreLimit !== undefined && !SCORE_LIMITS[gametype].includes(scoreLimit))) {
              json(response, 400, {error:'Choose a supported time and score limit.'}); return;
            }
            body = {name:body.name, map, gametype, botCount, botDifficulty, timeLimit, scoreLimit};
          }
        }
        const result = await manager(request.method, body);
        if (request.method === 'DELETE' && result.status === 200) {
          for (const client of wss.clients)
            if (client.roomId === body.id) client.close(1001, 'The creator deleted the server.');
        }
        if (request.method === 'GET' && Array.isArray(result.value.rooms)) {
          for (const room of result.value.rooms) {
            room.connections = roomConnections(room.id);
            room.maxPlayers = maxClients;
            room.maxHumans = maxClients - Math.max(room.botCount || 0, room.bots || 0);
          }
        }
        json(response, result.status, result.value);
      })().catch(() => json(response, 503, {error:'The server manager is unavailable.'}));
      return;
    }
    const appIcon = /^\/game-data\/files\/cod2-icon\/([a-z0-9-]+\.png)$/.exec(request.url)?.[1];
    const artwork = request.url === '/game-data/files/cod2-icon/original.ico'
      ? { type: 'image/x-icon', load: () => readFile(iconPath) }
      : request.url === '/game-data/files/cod2-startup/original.png'
        ? { type: 'image/png', load: () => readFile(startupPath) }
        : appIcon ? { type: 'image/png', load: () => drawAppIcons().then(icons =>
          icons[appIcon] ?? Promise.reject(new Error('Unknown icon'))) } : null;
    if (artwork) {
      if (!['GET', 'HEAD'].includes(request.method)) {
        response.writeHead(405, { Allow: 'GET, HEAD' }); response.end(); return;
      }
      const cached = artworkCache.get(request.url);
      (cached ? Promise.resolve(cached) : artwork.load().then(buf => {
        artworkCache.set(request.url, buf);
        return buf;
      })).then(icon => {
        response.writeHead(200, { 'Content-Type': artwork.type,
          'Content-Length': icon.length, 'Cache-Control': 'public, max-age=3600',
          'X-Content-Type-Options': 'nosniff' });
        response.end(request.method === 'HEAD' ? undefined : icon);
      }).catch(() => { response.writeHead(404); response.end('Game artwork unavailable'); });
      return;
    }
    if (request.url === '/gateway/health') {
      response.writeHead(200, { 'Content-Type': 'application/json', 'Cache-Control': 'no-store' });
      response.end(JSON.stringify({ gateway: 'ready', clients: wss.clients.size,
        pendingClients: [...reservations.values()].reduce((sum, count) => sum + count, 0),
        updating: maintenanceCheck(), maxClients }));
      return;
    }
    const archive = cdnRedirect && request.method === 'GET' && /^\/game-data\/files\/([^/]+)$/.exec(url?.pathname);
    if (archive) {
      let key = null;
      try { key = decodeURIComponent(archive[1]); } catch {}
      (key ? cdnRedirect(key) : Promise.resolve(null)).catch(() => null).then(location => {
        if (response.destroyed) return;
        if (!location) { proxy(request, response); return; }
        response.writeHead(302, { Location: location, 'Cache-Control': 'no-store' });
        response.end();
      });
      return;
    }
    proxy(request, response);
  });
  // Slow or idle request headers must not hold sockets open.
  server.headersTimeout = 15000;
  server.requestTimeout = 30000;
  const wss = new WebSocketServer({ noServer: true, maxPayload: MAX_CLIENT_DATAGRAM,
    perMessageDeflate: false });
  server.on('upgrade', async (request, socket, head) => {
    if (maintenanceCheck()) {
      socket.end('HTTP/1.1 503 Updating\r\nConnection: close\r\nContent-Length: 0\r\n\r\n'); return;
    }
    const roomMatch = /^\/game(?:\?room=([0-2]))?$/.exec(request.url);
    const roomId = roomMatch?.[1] ? Number(roomMatch[1]) : 0;
    const occupied = roomConnections(roomId);
    const status = !roomMatch || (!roomsEnabled && request.url !== '/game') ? 404 : !sameOrigin(request) ? 403 :
      occupied >= maxClients ? 503 : 0;
    if (status) {
      socket.end(`HTTP/1.1 ${status} Rejected\r\nConnection: close\r\nContent-Length: 0\r\n\r\n`);
      return;
    }
    reservations.set(roomId, (reservations.get(roomId) || 0) + 1);
    try {
      let destination = udpPort;
      if (roomsEnabled) {
        const list = await manager('GET');
        const room = list.value.rooms?.find(room => room.id === roomId);
        if (!room || room.port !== udpPort + roomId) throw new Error('Room unavailable');
        const bots = Math.max(room.botCount || 0, room.bots || 0);
        if (roomConnections(roomId) > maxClients - bots) throw new Error('Room full');
        destination = room.port;
      }
      // The host can begin its idle transition while the manager request waits.
      if (maintenanceCheck()) throw new Error('Server update in progress');
      wss.handleUpgrade(request, socket, head, ws => {
        ws.roomId = roomId;
        wss.emit('connection', ws, destination);
      });
    } catch {
      if (!socket.destroyed) socket.end('HTTP/1.1 503 Unavailable\r\nConnection: close\r\nContent-Length: 0\r\n\r\n');
    } finally {
      reservations.set(roomId, reservations.get(roomId) - 1);
    }
  });
  wss.on('connection', (ws, destination) => {
    const udp = udpSocketFactory();
    let udpReady = false;
    let closed = false;
    let alive = true;
    let pending = [];
    let pendingBytes = 0;
    const packetBudget = tokenBucket(clientBudget.packets, now);
    const byteBudget = tokenBucket(clientBudget.bytes, now);
    const oobBudget = tokenBucket(clientBudget.oob, now);
    let drops = 0, dropWindow = now();
    // One send callback per peer avoids allocating a closure for every command.
    const sentToUdp = error => {
      if (error && ws.readyState === WebSocket.OPEN) ws.close(1011, 'UDP send failed');
    };
    const admitted = data => {
      const command = oobCommand(data);
      if (packetBudget(1) && byteBudget(data.length) &&
          (command === null || (CLIENT_OOB_COMMANDS.has(command) && oobBudget(1)))) return true;
      const time = now();
      if (time - dropWindow > clientBudget.dropWindowMs) { drops = 0; dropWindow = time; }
      if (++drops > clientBudget.closeAfterDrops) ws.close(1008, 'Too many packets');
      return false;
    };
    const closeUdp = () => {
      if (closed) return;
      closed = true;
      pending.length = 0;
      pendingBytes = 0;
      try { udp.close(); } catch {}
    };
    ws.on('error', closeUdp);
    ws.on('close', closeUdp);
    ws.on('pong', () => { alive = true; });
    ws.on('message', (data, binary) => {
      if (!binary || !data.length) { ws.close(1003, 'Binary datagrams required'); return; }
      if (closed || !admitted(data)) return;
      if (udpReady) udp.send(data, sentToUdp);
      else {
        // WebSocket open can precede UDP DNS/connect readiness. Preserve the
        // initial challenge and fragment order rather than forcing a retry.
        if (pending.length >= 16 || pendingBytes + data.length > 65536) {
          ws.close(1009, 'Connection startup queue full'); return;
        }
        pending.push(data);
        pendingBytes += data.length;
      }
    });
    udp.on('message', data => {
      if (ws.readyState === WebSocket.OPEN && ws.bufferedAmount < SERVER_BACKLOG_BYTES)
        ws.send(data, wsBinaryOptions);
    });
    udp.on('error', error => {
      console.error('[gateway] UDP:', error.code);
      ws.close(1011, 'Dedicated server unavailable');
      closeUdp();
    });
    // Fixed destination: browsers cannot choose arbitrary UDP hosts or ports.
    udp.connect(destination, udpHost, () => {
      if (closed || ws.readyState !== WebSocket.OPEN) return;
      udpReady = true;
      for (const packet of pending) udp.send(packet, sentToUdp);
      pending = [];
      pendingBytes = 0;
      console.log('[gateway] client UDP port', udp.address().port);
    });
    const heartbeat = setInterval(() => {
      if (!alive) { ws.terminate(); return; }
      alive = false;
      ws.ping();
    }, 15000);
    heartbeat.unref();
    ws.once('close', () => clearInterval(heartbeat));
  });
  return { server, wss, async close() {
    for (const client of wss.clients) client.terminate();
    await new Promise(resolve => wss.close(resolve));
    await new Promise(resolve => server.close(resolve));
  } };
}

if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  let classStore = null;
  try { classStore = createClassStore(process.env.COD2_CLASSES_DB || '/data/classes.sqlite'); }
  catch (error) { console.error('[gateway] classes unavailable:', error.message); }
  const webHost = process.env.WEB_HOST || 'cod2-web', webPort = Number(process.env.WEB_PORT || 8088);
  const gateway = createGateway({
    webHost,
    webPort,
    udpHost: process.env.UDP_HOST || 'cod2-server',
    udpPort: Number(process.env.UDP_PORT || 28960),
    roomsEnabled: true,
    classStore,
    // Without publish-cdn.py's files in this directory, every archive comes from the Pi.
    cdnRedirect: createCdnRedirects({ directory: process.env.COD2_CDN_DIR || '/cdn',
      readPolicy: () => readAssetPolicy(webHost, webPort) }),
  });
  gateway.server.listen(Number(process.env.PORT || 8088), '0.0.0.0',
    () => console.log('[gateway] LAN HTTP/WebSocket endpoint ready'));
  for (const signal of ['SIGTERM', 'SIGINT'])
    process.once(signal, async () => { await gateway.close(); process.exit(0); });
}
