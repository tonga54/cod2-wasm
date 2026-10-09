import http from 'node:http';
import dgram from 'node:dgram';
import { readFile } from 'node:fs/promises';
import { pathToFileURL } from 'node:url';
import WebSocket, { WebSocketServer } from 'ws';

export function createGateway({ webHost = 'cod2-web', webPort = 8088,
  udpHost = 'cod2-server', udpPort = 28960, maxClients = 64,
  iconPath = '/game-ui/cod2.ico', startupPath = '/game-ui/cod2-startup.png',
  roomsEnabled = false, managerPort = 8090 } = {}) {
  const reservations = new Map();
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
        timeout: method === 'POST' ? 30000 : 2000 }, reply => {
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
  const server = http.createServer((request, response) => {
    if (roomsEnabled && request.url === '/servers') {
      if (!['GET', 'POST'].includes(request.method)) {
        json(response, 405, { error: 'Unsupported method' }); return;
      }
      if (request.method === 'POST' && !sameOrigin(request)) {
        json(response, 403, { error: 'Invalid origin' }); return;
      }
      (async () => {
        let body;
        if (request.method === 'POST') {
          const chunks = []; let bytes = 0;
          for await (const chunk of request) {
            bytes += chunk.length;
            if (bytes > 1024) { json(response, 413, {error:'Request too large'}); return; }
            chunks.push(chunk);
          }
          try { body = JSON.parse(Buffer.concat(chunks)); }
          catch { json(response, 400, {error:'Invalid JSON'}); return; }
          if (!body || typeof body.name !== 'string' || body.name.length > 128) {
            json(response, 400, {error:'Nombre de partida inválido.'}); return;
          }
          body = {name:body.name};
        }
        const result = await manager(request.method, body);
        if (request.method === 'GET' && Array.isArray(result.value.rooms)) {
          for (const room of result.value.rooms) {
            room.connections = [...wss.clients].filter(ws => ws.roomId === room.id).length +
              (reservations.get(room.id) || 0);
            room.maxPlayers = maxClients;
          }
        }
        json(response, result.status, result.value);
      })().catch(() => json(response, 503, {error:'El servidor de partidas no está disponible.'}));
      return;
    }
    const artwork = request.url === '/game-data/files/cod2-icon/original.ico'
      ? { path: iconPath, type: 'image/x-icon' }
      : request.url === '/game-data/files/cod2-startup/original.png'
        ? { path: startupPath, type: 'image/png' } : null;
    if (artwork) {
      if (!['GET', 'HEAD'].includes(request.method)) {
        response.writeHead(405, { Allow: 'GET, HEAD' }); response.end(); return;
      }
      readFile(artwork.path).then(icon => {
        response.writeHead(200, { 'Content-Type': artwork.type,
          'Content-Length': icon.length, 'Cache-Control': 'public, max-age=3600',
          'X-Content-Type-Options': 'nosniff' });
        response.end(request.method === 'HEAD' ? undefined : icon);
      }).catch(() => { response.writeHead(404); response.end('Game artwork unavailable'); });
      return;
    }
    if (request.url === '/gateway/health') {
      response.writeHead(200, { 'Content-Type': 'application/json', 'Cache-Control': 'no-store' });
      response.end(JSON.stringify({ gateway: 'ready', clients: wss.clients.size, maxClients }));
      return;
    }
    const upstream = http.request({ hostname: webHost, port: webPort,
      method: request.method, path: request.url, headers: request.headers }, reply => {
      response.writeHead(reply.statusCode, reply.headers);
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
  });
  const wss = new WebSocketServer({ noServer: true, maxPayload: 65507,
    perMessageDeflate: false });
  server.on('upgrade', async (request, socket, head) => {
    const roomMatch = /^\/game(?:\?room=([0-2]))?$/.exec(request.url);
    const roomId = roomMatch?.[1] ? Number(roomMatch[1]) : 0;
    const occupied = [...wss.clients].filter(ws => ws.roomId === roomId).length + (reservations.get(roomId) || 0);
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
        destination = room.port;
      }
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
    const udp = dgram.createSocket('udp4');
    let udpReady = false;
    let closed = false;
    let alive = true;
    const closeUdp = () => {
      if (closed) return;
      closed = true;
      try { udp.close(); } catch {}
    };
    ws.on('error', closeUdp);
    ws.on('close', closeUdp);
    ws.on('pong', () => { alive = true; });
    ws.on('message', (data, binary) => {
      if (!binary || !data.length) { ws.close(1003, 'Binary datagrams required'); return; }
      if (udpReady && !closed) udp.send(data, error => {
        if (error && ws.readyState === WebSocket.OPEN) ws.close(1011, 'UDP send failed');
      });
    });
    udp.on('message', data => {
      if (ws.readyState === WebSocket.OPEN && ws.bufferedAmount < 262144)
        ws.send(data, { binary: true, compress: false });
    });
    udp.on('error', error => {
      console.error('[gateway] UDP:', error.code);
      ws.close(1011, 'Dedicated server unavailable');
      closeUdp();
    });
    // Fixed destination: browsers cannot choose arbitrary UDP hosts or ports.
    udp.connect(destination, udpHost, () => {
      udpReady = true;
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
  const gateway = createGateway({
    webHost: process.env.WEB_HOST || 'cod2-web',
    webPort: Number(process.env.WEB_PORT || 8088),
    udpHost: process.env.UDP_HOST || 'cod2-server',
    udpPort: Number(process.env.UDP_PORT || 28960),
    roomsEnabled: true,
  });
  gateway.server.listen(Number(process.env.PORT || 8088), '0.0.0.0',
    () => console.log('[gateway] LAN HTTP/WebSocket endpoint ready'));
  for (const signal of ['SIGTERM', 'SIGINT'])
    process.once(signal, async () => { await gateway.close(); process.exit(0); });
}
