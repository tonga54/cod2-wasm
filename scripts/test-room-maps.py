#!/usr/bin/env python3
"""Check the room supervisor's map selection, discovery and rotation contract."""
import http.client
import http.server
import importlib.util
import json
from pathlib import Path
import tempfile
import threading
from unittest.mock import Mock, patch

root = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location('rooms', root / 'downstream/server/rooms.py')
rooms = importlib.util.module_from_spec(spec)
spec.loader.exec_module(rooms)

# Exercise the actual argument/config construction without launching a room.
with tempfile.TemporaryDirectory(prefix='cod2-room-maps-') as directory:
    profile = Path(directory)
    with patch.object(rooms, 'Path', side_effect=lambda name: profile if name == '/profile' else Path(name)), \
         patch.object(rooms.shutil, 'copyfile', side_effect=lambda _, dest: Path(dest).write_text((root / 'server.cfg').read_text())), \
         patch.object(rooms.subprocess, 'Popen') as launch:
        for room_id, mapname in enumerate(rooms.SUPPORTED_MAPS):
            rooms.start_room(room_id, 'Test;quit\nexec hacked+quit', mapname)
            args = launch.call_args.args[0]
            assert args[-2:] == ['+map', mapname], args
            assert args[args.index('sv_maxclients') + 1] == '64'
            assert ';' not in args[args.index('sv_hostname') + 1]
            assert '+' not in args[args.index('sv_hostname') + 1]
            config = (profile / f'room-{room_id}/raw/server.cfg').read_text()
            assert 'set scr_allies ""' in config and 'set scr_axis ""' in config
            assert 'set scr_tdm_scorelimit "100"' in config
            assert 'set scr_tdm_timelimit "15"' in config
            expected = 'gametype tdm map mp_carentan' if room_id == 0 else ''
            assert f'set sv_mapRotationCurrent "{expected}"' in config
        before = launch.call_count
        for mapname in ('mp_dawnville', 'mp_carentan;quit', ''):
            try:
                rooms.start_room(2, 'Test', mapname)
            except ValueError:
                pass
            else:
                raise AssertionError('Unsupported map launched')
        assert launch.call_count == before
rooms.rooms.clear()

# Both maps must remain discoverable after rotation. Other maps/modes are hidden.
udp = Mock()
udp.__enter__ = Mock(return_value=udp)
udp.__exit__ = Mock(return_value=False)
process = Mock()
process.poll.return_value = None
room = {'id': 0, 'port': 28960, 'process': process}
with patch.object(rooms.socket, 'socket', return_value=udp):
    for mapname, gametype in (('mp_toujane', 'tdm'), ('mp_carentan', 'tdm'),
                              ('mp_dawnville', 'tdm'), ('mp_carentan', 'ctf')):
        udp.recv.return_value = b'\xff\xff\xff\xffinfoResponse\n' + (
            f'\\mapname\\{mapname}\\gametype\\{gametype}\\clients\\3').encode()
        info = rooms.info_for(room)
        if mapname in rooms.SUPPORTED_MAPS and gametype == 'tdm':
            assert info['players'] == 3 and f'\\mapname\\{mapname}' in info['info']
        else:
            assert info is None

# Test the internal HTTP API with real parsing, validation and reply handling.
server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), rooms.Handler)
thread = threading.Thread(target=server.serve_forever, daemon=True)
thread.start()
try:
    with patch.object(rooms, 'start_room', return_value=room) as start, \
         patch.object(rooms, 'info_for', return_value={'id': 1, 'port': 28961}):
        def post(body):
            connection = http.client.HTTPConnection(*server.server_address, timeout=2)
            connection.request('POST', '/rooms', json.dumps(body), {'Content-Type': 'application/json'})
            response = connection.getresponse()
            status = response.status
            response.read()
            connection.close()
            return status

        for body, expected in (({'name': 'Legacy'}, 'mp_toujane'),
                               ({'name': 'Carentan', 'map': 'mp_carentan'}, 'mp_carentan')):
            assert post(body) == 201
            assert start.call_args.args == (1, body['name'], expected)
        before = start.call_count
        for mapname in ('mp_dawnville', 'mp_carentan;quit', [], None, 1):
            assert post({'name': 'Invalid', 'map': mapname}) == 400
        assert start.call_count == before
finally:
    server.shutdown()
    server.server_close()
    thread.join(timeout=2)
print('PASS: both room maps, legacy default, faction/limit config, next-map rotation, discovery and HTTP allowlist')
