#!/usr/bin/python3
"""Small, internal-only supervisor for Toujane/Carentan multiplayer rooms."""
import concurrent.futures
import http.server
import json
import os
from pathlib import Path
import re
import secrets
import shutil
import signal
import socket
import subprocess
import threading
import time

MAX_ROOMS = 3
BASE_PORT = 28960
IDLE_SECONDS = 300
SUPPORTED_MAPS = ('mp_toujane', 'mp_carentan')
SUPPORTED_GAMETYPES = ('dm', 'tdm', 'ctf', 'hq', 'sd')
MAX_BOTS = 16
BOT_DIFFICULTIES = ('easy', 'normal', 'hard')
DEFAULT_NAME = 'Toujane - Carentan'
rooms = {}
lock = threading.Lock()
stopping = threading.Event()


def info_for(room):
    if room['process'].poll() is not None:
        return None
    started = time.monotonic()
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp:
            udp.settimeout(0.4)
            udp.connect(('127.0.0.1', room['port']))
            udp.send(b'\xff\xff\xff\xffgetinfo cod2-browser')
            data = udp.recv(4096)
        prefix = b'\xff\xff\xff\xffinfoResponse\n'
        if not data.startswith(prefix):
            return None
        info = data[len(prefix):].split(b'\x00', 1)[0].decode('latin1').strip()
        fields = info.split('\\')
        values = dict(zip(fields[1::2], fields[2::2]))
        if values.get('mapname') not in SUPPORTED_MAPS or values.get('gametype') not in SUPPORTED_GAMETYPES:
            return None
        players = int(values.get('clients', '0'))
        bots = min(players, max(0, int(values.get('bots', '0'))))
        return {'id': room['id'], 'port': room['port'], 'info': info,
                'instance': room.get('instance', ''),
                'ping': max(1, round((time.monotonic() - started) * 1000)),
                'players': players, 'bots': bots, 'humans': players - bots,
                'botCount': room.get('botCount', 0),
                'botDifficulty': room.get('botDifficulty', 'normal')}
    except (OSError, ValueError):
        return None


def start_room(room_id, name, mapname='mp_toujane', gametype='tdm', bot_count=0, bot_difficulty='normal'):
    if mapname not in SUPPORTED_MAPS:
        raise ValueError('Unsupported map')
    if gametype not in SUPPORTED_GAMETYPES:
        raise ValueError('Unsupported game mode')
    if type(bot_count) is not int or not 0 <= bot_count <= MAX_BOTS:
        raise ValueError('Choose between 0 and 16 bots.')
    if bot_difficulty not in BOT_DIFFICULTIES:
        raise ValueError('Choose Easy, Normal or Hard bots.')
    profile = Path('/profile') / f'room-{room_id}'
    raw = profile / 'raw'
    raw.mkdir(parents=True, exist_ok=True)
    shutil.copyfile('/config/server.cfg', raw / 'server.cfg')
    if bot_count:
        scripts = raw / 'maps/mp'
        scripts.mkdir(parents=True, exist_ok=True)
        shutil.copyfile('/config/browser_bots.gsc', scripts / 'browser_bots.gsc')
    # Start the rotation after the selected map so it changes at round end.
    remaining = SUPPORTED_MAPS[SUPPORTED_MAPS.index(mapname) + 1:]
    rotation = f'gametype {gametype} ' + ' '.join('map ' + item for item in remaining) if remaining else ''
    full_rotation = f'gametype {gametype} ' + ' '.join('map ' + item for item in SUPPORTED_MAPS)
    with (raw / 'server.cfg').open('a') as config:
        config.write(f'\nset g_gametype "{gametype}"\n'
                     f'set sv_mapRotation "{full_rotation}"\n'
                     f'set sv_mapRotationCurrent "{rotation}"\n'
                     f'set scr_bot_count "{bot_count}"\n'
                     f'set scr_bot_difficulty "{BOT_DIFFICULTIES.index(bot_difficulty)}"\n')
    # Names are one console argument. No arbitrary commands, paths,
    # launch options, UDP destinations or shell interpretation are accepted.
    name = re.sub(r'[^A-Za-z0-9 _|.-]', '', name).strip()[:32] or DEFAULT_NAME
    args = ['/usr/local/bin/cod2_lnxded', '+set', 'dedicated', '1',
            '+set', 'fs_basepath', '/game', '+set', 'fs_homepath', str(profile),
            '+set', 'net_ip', '0.0.0.0', '+set', 'net_port', str(BASE_PORT + room_id),
            '+set', 'sv_maxclients', '64', '+exec', 'server.cfg',
            '+set', 'g_gametype', gametype, '+set', 'sv_hostname', f'"{name}"',
            '+map', mapname]
    child = subprocess.Popen(args, stdin=subprocess.DEVNULL, start_new_session=True)
    room = {'id': room_id, 'port': BASE_PORT + room_id, 'process': child,
            'last_used': time.monotonic(), 'instance': secrets.token_hex(16),
            'ownerToken': secrets.token_hex(32), 'botCount': bot_count,
            'botDifficulty': bot_difficulty}
    rooms[room_id] = room
    return room


def stop_room(room):
    child = room['process']
    if child.poll() is None:
        os.killpg(child.pid, signal.SIGTERM)
        try:
            child.wait(timeout=2)
        except subprocess.TimeoutExpired:
            os.killpg(child.pid, signal.SIGKILL)
            child.wait(timeout=2)


class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self, *_args):
        pass

    def reply(self, status, value):
        body = json.dumps(value).encode()
        self.send_response(status)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Cache-Control', 'no-store')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        try:
            self.wfile.write(body)
        except (BrokenPipeError, ConnectionResetError):
            pass

    def do_GET(self):
        if self.path != '/rooms':
            return self.reply(404, {'error': 'Not found'})
        with lock:
            current = list(rooms.values())
        with concurrent.futures.ThreadPoolExecutor(max_workers=MAX_ROOMS) as pool:
            ready = [info for info in pool.map(info_for, current) if info]
        self.reply(200, {'rooms': ready, 'maxRooms': MAX_ROOMS})

    def do_POST(self):
        if self.path != '/rooms':
            return self.reply(404, {'error': 'Not found'})
        try:
            length = int(self.headers.get('Content-Length', '0'))
            if not 0 < length <= 1024:
                raise ValueError()
            body = json.loads(self.rfile.read(length))
            name = body.get('name', DEFAULT_NAME)
            if not isinstance(name, str):
                raise ValueError()
        except (ValueError, TypeError, AttributeError):
            return self.reply(400, {'error': 'Invalid server name.'})
        mapname = body.get('map', 'mp_toujane')
        if not isinstance(mapname, str) or mapname not in SUPPORTED_MAPS:
            return self.reply(400, {'error': 'Choose Toujane or Carentan.'})
        gametype = body.get('gametype', 'tdm')
        if not isinstance(gametype, str) or gametype not in SUPPORTED_GAMETYPES:
            return self.reply(400, {'error': 'Choose a supported game mode.'})
        bot_count = body.get('botCount', 0)
        if type(bot_count) is not int or not 0 <= bot_count <= MAX_BOTS:
            return self.reply(400, {'error': 'Choose between 0 and 16 bots.'})
        bot_difficulty = body.get('botDifficulty', 'normal')
        if not isinstance(bot_difficulty, str) or bot_difficulty not in BOT_DIFFICULTIES:
            return self.reply(400, {'error': 'Choose Easy, Normal or Hard bots.'})
        with lock:
            if stopping.is_set():
                return self.reply(503, {'error': 'The host is shutting down.'})
            available = [i for i in range(MAX_ROOMS)
                         if i not in rooms or rooms[i]['process'].poll() is not None]
            if not available:
                return self.reply(409, {'error': 'Three servers are already running. Join an existing server.'})
            room = start_room(available[0], name, mapname, gametype, bot_count, bot_difficulty)
        for _ in range(40):
            info = info_for(room)
            if info:
                return self.reply(201, {**info, 'ownerToken': room['ownerToken']})
            if room['process'].poll() is not None:
                break
            if stopping.wait(0.25):
                break
        with lock:
            stop_room(room)
        self.reply(503, {'error': 'Unable to start the server.'})

    def do_DELETE(self):
        if self.path != '/rooms':
            return self.reply(404, {'error': 'Not found'})
        try:
            length = int(self.headers.get('Content-Length', '0'))
            if not 0 < length <= 1024:
                raise ValueError()
            body = json.loads(self.rfile.read(length))
            room_id, token = body.get('id'), body.get('ownerToken')
            if type(room_id) is not int or not 0 <= room_id < MAX_ROOMS or not isinstance(token, str):
                raise ValueError()
        except (ValueError, TypeError, AttributeError):
            return self.reply(400, {'error': 'Invalid server.'})
        with lock:
            room = rooms.get(room_id)
            if not room:
                return self.reply(404, {'error': 'This server no longer exists.'})
            if not re.fullmatch(r'[0-9a-f]{64}', token) or not secrets.compare_digest(token, room['ownerToken']):
                return self.reply(403, {'error': 'Only the creator can delete this server.'})
            stop_room(room)
            del rooms[room_id]
        self.reply(200, {'deleted': room_id})


def cleanup():
    while not stopping.wait(15):
        with lock:
            current = list(rooms.values())
        for room in current:
            info = info_for(room)
            if info and info.get('humans', info['players']):
                room['last_used'] = time.monotonic()
            elif time.monotonic() - room['last_used'] > IDLE_SECONDS:
                with lock:
                    stop_room(room)
                    if rooms.get(room['id']) is room:
                        del rooms[room['id']]


def main():
    import resource
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    server = http.server.ThreadingHTTPServer(('0.0.0.0', 8090), Handler)
    server.daemon_threads = True
    threading.Thread(target=cleanup, daemon=True).start()

    def stop(_signum, _frame):
        stopping.set()
        threading.Thread(target=server.shutdown, daemon=True).start()

    signal.signal(signal.SIGTERM, stop)
    signal.signal(signal.SIGINT, stop)
    try:
        server.serve_forever()
    finally:
        stopping.set()
        server.server_close()
        with lock:
            for room in list(rooms.values()):
                stop_room(room)


if __name__ == '__main__':
    main()
