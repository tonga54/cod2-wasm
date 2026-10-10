#!/usr/bin/env python3
"""Check the room supervisor's map selection, discovery and rotation contract."""
import http.client
import http.server
import importlib.util
import json
from pathlib import Path
import resource
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
         patch.object(rooms.shutil, 'copyfile', side_effect=lambda src, dest: Path(dest).write_text((root / ('downstream/server/browser_bots.gsc' if src.endswith('.gsc') else 'server.cfg')).read_text())), \
         patch.object(rooms.subprocess, 'Popen') as launch:
        for case, (mapname, gametype) in enumerate((m, g) for m in rooms.SUPPORTED_MAPS for g in rooms.SUPPORTED_GAMETYPES):
            room_id = case % 3
            created = rooms.start_room(room_id, 'Test;quit\nexec hacked+quit', mapname, gametype)
            assert len(created['ownerToken']) == 64 and len(created['instance']) == 32
            args = launch.call_args.args[0]
            assert args[-2:] == ['+map', mapname], args
            assert args[args.index('sv_maxclients') + 1] == '64'
            assert args[args.index('g_gametype') + 1] == gametype
            assert args.index('g_gametype') > args.index('+exec')
            assert ';' not in args[args.index('sv_hostname') + 1]
            assert '+' not in args[args.index('sv_hostname') + 1]
            config = (profile / f'room-{room_id}/raw/server.cfg').read_text()
            assert 'set scr_allies ""' in config and 'set scr_axis ""' in config
            assert 'set scr_tdm_scorelimit "100"' in config
            assert 'set scr_tdm_timelimit "15"' in config
            expected = f'gametype {gametype} map mp_carentan' if mapname == 'mp_toujane' else ''
            assert f'set g_gametype "{gametype}"' in config
            assert f'set sv_mapRotation "gametype {gametype} map mp_toujane map mp_carentan"' in config
            assert f'set sv_mapRotationCurrent "{expected}"' in config
        created = rooms.start_room(0, 'Bots', 'mp_carentan', 'tdm', 8, 'hard')
        assert created['botCount'] == 8 and created['botDifficulty'] == 'hard'
        config = (profile / 'room-0/raw/server.cfg').read_text()
        assert 'set scr_bot_count "8"' in config
        assert 'set scr_bot_difficulty "2"' in config
        assert (profile / 'room-0/raw/maps/mp/browser_bots.gsc').read_text() == (root / 'downstream/server/browser_bots.gsc').read_text()
        before = launch.call_count
        for mapname in ('mp_dawnville', 'mp_carentan;quit', ''):
            try:
                rooms.start_room(2, 'Test', mapname)
            except ValueError:
                pass
            else:
                raise AssertionError('Unsupported map launched')
        for gametype in ('sw', 'ctf;quit', '', None):
            try:
                rooms.start_room(2, 'Test', 'mp_carentan', gametype)
            except ValueError:
                pass
            else:
                raise AssertionError('Unsupported mode launched')
        assert launch.call_count == before
rooms.rooms.clear()

# Startup serves discovery without launching an automatic game process.
with patch.object(resource, 'setrlimit'), \
     patch.object(rooms.http.server, 'ThreadingHTTPServer') as listener, \
     patch.object(rooms.threading, 'Thread'), \
     patch.object(rooms.signal, 'signal'), \
     patch.object(rooms.subprocess, 'Popen') as launch:
    rooms.main()
    listener.return_value.serve_forever.assert_called_once()
    launch.assert_not_called()
    assert rooms.rooms == {}
rooms.stopping.clear()

# Both maps and all five modes remain discoverable after rotation.
udp = Mock()
udp.__enter__ = Mock(return_value=udp)
udp.__exit__ = Mock(return_value=False)
process = Mock()
process.poll.return_value = None
room = {'id': 0, 'port': 28960, 'process': process, 'ownerToken': 'a'*64, 'instance': 'b'*32}
with patch.object(rooms.socket, 'socket', return_value=udp):
    for mapname, gametype in ([(m, g) for m in rooms.SUPPORTED_MAPS for g in rooms.SUPPORTED_GAMETYPES] +
                              [('mp_dawnville', 'tdm'), ('mp_carentan', 'sw')]):
        udp.recv.return_value = b'\xff\xff\xff\xffinfoResponse\n' + (
            f'\\mapname\\{mapname}\\gametype\\{gametype}\\clients\\3').encode()
        info = rooms.info_for(room)
        if mapname in rooms.SUPPORTED_MAPS and gametype in rooms.SUPPORTED_GAMETYPES:
            assert 'ownerToken' not in info
            assert info['players'] == 3 and f'\\mapname\\{mapname}' in info['info']
        else:
            assert info is None

    udp.recv.return_value = b'\xff\xff\xff\xffinfoResponse\n\\mapname\\mp_toujane\\gametype\\tdm\\clients\\9\\bots\\8'
    info = rooms.info_for(room)
    assert info['players'] == 9 and info['bots'] == 8 and info['humans'] == 1

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
            assert start.call_args.args == (0, body['name'], expected, 'tdm', 0, 'normal')
        for gametype in rooms.SUPPORTED_GAMETYPES:
            assert post({'name':'Selected mode', 'map':'mp_carentan', 'gametype':gametype}) == 201
            assert start.call_args.args == (0, 'Selected mode', 'mp_carentan', gametype, 0, 'normal')
        for difficulty in rooms.BOT_DIFFICULTIES:
            assert post({'name': 'Bots', 'botCount': 8, 'botDifficulty': difficulty}) == 201
            assert start.call_args.args == (0, 'Bots', 'mp_toujane', 'tdm', 8, difficulty)
        before = start.call_count
        for mapname in ('mp_dawnville', 'mp_carentan;quit', [], None, 1):
            assert post({'name': 'Invalid', 'map': mapname}) == 400
        for gametype in ('sw', 'ctf;quit', '', [], None, 1):
            assert post({'name':'Invalid', 'gametype':gametype}) == 400
        for count in (-1, 17, 1.5, True, None, '8', []):
            assert post({'name':'Invalid', 'botCount':count}) == 400
        for difficulty in ('expert', 'hard;quit', '', None, [], 1):
            assert post({'name':'Invalid', 'botDifficulty':difficulty}) == 400
        assert start.call_count == before

    # All three slots belong to user-created games, including the first slot.
    def create(room_id, name, mapname, gametype, bot_count, bot_difficulty):
        child = Mock()
        child.poll.return_value = None
        created = {'id': room_id, 'port': rooms.BASE_PORT + room_id,
                   'process': child, 'last_used': 0, 'ownerToken': rooms.secrets.token_hex(32), 'instance': rooms.secrets.token_hex(16)}
        rooms.rooms[room_id] = created
        return created

    def discovery():
        connection = http.client.HTTPConnection(*server.server_address, timeout=2)
        connection.request('GET', '/rooms')
        response = connection.getresponse()
        assert response.status == 200
        value = json.loads(response.read())
        connection.close()
        return value

    with patch.object(rooms, 'start_room', side_effect=create), \
         patch.object(rooms, 'info_for', side_effect=lambda room: {
             'id': room['id'], 'port': room['port'], 'players': 0}):
        assert discovery() == {'rooms': [], 'maxRooms': 3}
        for room_id in range(rooms.MAX_ROOMS):
            assert post({'name': f'User {room_id}', 'map': 'mp_carentan'}) == 201
            assert set(rooms.rooms) == set(range(room_id + 1))
        assert {room['id'] for room in discovery()['rooms']} == {0, 1, 2}
        assert post({'name': 'Full'}) == 409
        rooms.rooms[1]['process'].poll.return_value = 0
        assert post({'name': 'Replacement'}) == 201
        assert rooms.rooms[1]['process'].poll() is None
        def delete(body):
            connection = http.client.HTTPConnection(*server.server_address, timeout=2)
            connection.request('DELETE', '/rooms', json.dumps(body), {'Content-Type': 'application/json'})
            response = connection.getresponse()
            status, value = response.status, json.loads(response.read())
            connection.close()
            return status, value
        token = rooms.rooms[0]['ownerToken']
        instance = rooms.rooms[0]['instance']
        with patch.object(rooms, 'stop_room') as stop:
            for body, expected in (({'id':0,'ownerToken':'f'*64},403),
                                   ({'id':0,'ownerToken':''},403),
                                   ({'id':True,'ownerToken':token},400),
                                   ({'id':8,'ownerToken':token},400)):
                assert delete(body)[0] == expected
            stop.assert_not_called()
            assert delete({'id':0,'ownerToken':token}) == (200, {'deleted':0})
            assert stop.call_count == 1 and set(rooms.rooms) == {1,2}
            assert delete({'id':0,'ownerToken':token})[0] == 404
            assert post({'name':'New owner'}) == 201
            assert rooms.rooms[0]['instance'] != instance
            assert rooms.rooms[0]['ownerToken'] != token
            assert delete({'id':0,'ownerToken':token})[0] == 403
            assert stop.call_count == 1

finally:
    server.shutdown()
    server.server_close()
    thread.join(timeout=2)

# Empty user games expire in every slot; an occupied game remains available.
with patch.object(rooms.stopping, 'wait', side_effect=[False, True]), \
     patch.object(rooms.time, 'monotonic', return_value=rooms.IDLE_SECONDS + 1), \
     patch.object(rooms, 'info_for', side_effect=lambda room: {
         'players': 8, 'humans': 1 if room['id'] == 1 else 0}), \
     patch.object(rooms, 'stop_room') as stop:
    rooms.cleanup()
    assert sorted(call.args[0]['id'] for call in stop.call_args_list) == [0, 2]
    assert set(rooms.rooms) == {1}
rooms.rooms.clear()
print('PASS: empty startup, three user slots, idle cleanup, both maps/five modes, rotation, discovery and HTTP allowlist')
