#!/usr/bin/env python3
"""Keep a room's bots through its map rotation on the real dedicated server.

Requires Docker, the native server image and the private browser archives; no
network. The production bot script fills a TDM room with four bots on
Toujane; a QA script in the countdown's slot counts every connected bot on each
map, those on a team among them, and rotates to Carentan, then Stalingrad,
whose Allies are British, American and Soviet in turn. The four bots must play
on every map, and no client may connect after them.
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile
import time
import uuid

ROOT = Path(__file__).resolve().parents[1]
BOTS = 4
MAPS = (('mp_toujane', 'british'), ('mp_carentan', 'american'), ('mp_railyard', 'russian'))
# getEntArray() with no key returns connecting clients too, which are not yet
# "player" entities. The bot script adds a bot every 0.25 s from 1 s and enables
# each 0.25 s later; its governor first samples the load at 20 s.
CHECKS = r'''
main()
{
	level thread run();
}

run()
{
	wait 8;
	connected = 0;
	joined = 0;
	entities = getEntArray();
	for(i = 0; i < entities.size; i++)
	{
		if(!isPlayer(entities[i]) || !(entities[i] isBot()))
			continue;
		connected++;
		if(entities[i].classname != "player" || !isDefined(entities[i].pers["team"]))
			continue;
		if(entities[i].pers["team"] != "spectator")
			joined++;
	}
	println("[rotation-qa] map=" + getCvar("mapname") + " allies=" + game["allies"] +
		" connected=" + connected + " joined=" + joined);
	rotation = getCvarInt("qa_rotation");
	if(rotation >= 2)
	{
		println("[rotation-qa] DONE");
		return;
	}
	setCvar("qa_rotation", rotation + 1);
	exitLevel(false);
}
'''
CONFIG = f'''set scr_bot_count "{BOTS}"
set scr_bot_difficulty "1"
set g_gametype "tdm"
set scr_tdm_timelimit "0"
set scr_tdm_scorelimit "0"
set sv_mapRotation "gametype tdm map {MAPS[1][0]} map {MAPS[2][0]}"
set sv_mapRotationCurrent ""
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--image', default='local/cod2-native-server:dev')
    parser.add_argument('--assets', type=Path, default=ROOT / 'data/browser/main')
    parser.add_argument('--binary', type=Path, default=ROOT / 'out/cod2-native/cod2_lnxded')
    parser.add_argument('--bots', type=Path, default=ROOT / 'downstream/server/browser_bots.gsc')
    parser.add_argument('--log', type=Path, default=ROOT / 'out/bot-rotation-runtime/server.log')
    args = parser.parse_args()
    for item in (args.assets, args.binary, args.bots):
        if not item.exists():
            parser.error(f'missing {item}')
    args.log.parent.mkdir(parents=True, exist_ok=True)
    name = 'cod2-rotation-qa-' + uuid.uuid4().hex[:10]
    log = ''
    with tempfile.TemporaryDirectory(prefix='cod2-rotation-qa-') as directory:
        checks, config = Path(directory) / 'checks.gsc', Path(directory) / 'rotation.cfg'
        checks.write_text(CHECKS)
        config.write_text(CONFIG)
        try:
            subprocess.run(['docker', 'run', '-d', '--name', name, '--platform', 'linux/amd64', '--network', 'none',
                            '--ulimit', 'core=0', '--read-only', '--tmpfs', '/profile:uid=1000,gid=1000,size=32m',
                            '--tmpfs', '/tmp:size=16m', '-v', f'{args.assets.resolve()}:/game/main:ro',
                            '-v', f'{args.binary.resolve()}:/qa-engine:ro',
                            '-v', f'{args.bots.resolve()}:/qa-bots.gsc:ro', '-v', f'{checks}:/qa-checks.gsc:ro',
                            '-v', f'{(ROOT / "server.cfg").resolve()}:/qa-server.cfg:ro', '-v', f'{config}:/qa-rotation.cfg:ro',
                            '--entrypoint', '/bin/sh', args.image, '-c',
                            'mkdir -p /profile/raw/maps/mp; cp /qa-bots.gsc /profile/raw/maps/mp/browser_bots.gsc; '
                            'cp /qa-checks.gsc /profile/raw/maps/mp/browser_prematch.gsc; '
                            'cp /qa-server.cfg /profile/raw/server.cfg; cp /qa-rotation.cfg /profile/raw/rotation.cfg; '
                            'exec /qa-engine +set dedicated 1 +set fs_basepath /game +set fs_homepath /profile '
                            '+set net_ip 127.0.0.1 +set sv_maxclients 16 +exec server.cfg +exec rotation.cfg '
                            f'+map {MAPS[0][0]}'],
                           check=True, capture_output=True)
            deadline = time.monotonic() + 240
            while time.monotonic() < deadline:
                log = subprocess.run(['docker', 'logs', name], capture_output=True, text=True).stdout
                if '[rotation-qa] DONE' in log or re.search(r'script (?:compile|runtime) error|Error during initialization',
                                                            log, re.I):
                    break
                time.sleep(1)
        finally:
            log = subprocess.run(['docker', 'logs', name], capture_output=True, text=True).stdout or log
            subprocess.run(['docker', 'rm', '-f', name], capture_output=True)
    args.log.write_text(log)
    errors = re.findall(r'.*script (?:compile|runtime) error.*', log, re.I)
    seen = [(mapname, allies, int(connected), int(joined)) for mapname, allies, connected, joined in re.findall(
        r'\[rotation-qa\] map=(\S+) allies=(\S+) connected=(\d+) joined=(\d+)', log)]
    expected = [(mapname, allies, BOTS, BOTS) for mapname, allies in MAPS]
    connects = len(re.findall(r'Going from CS_FREE to CS_CONNECTED', log))
    if errors or seen != expected or connects != BOTS or '[rotation-qa] DONE' not in log:
        raise SystemExit('\n'.join(errors) + f'\nmaps (map, allies, connected bots, bots on a team): {seen}\n'
                         f'expected: {expected}\nclients connected: {connects}, expected {BOTS}; log: {args.log}')
    print(f'PASS: {BOTS} bots connect once and play on {", ".join(m for m, _ in MAPS)} through the dedicated '
          'server\'s map rotation, under British, American and Soviet Allies, with no client left connecting')


if __name__ == '__main__':
    main()
