#!/usr/bin/env python3
"""Set down and take back S-Mines through the real dedicated server.

Requires Docker, the native server image and the private browser archives;
no network. A test client answers the class menu with the S-Mine, sets mines
down with G and takes them back by holding Use, as a player does; a mine left
down stays through its owner's death and goes when they spawn again.
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile
import time
import uuid

ROOT = Path(__file__).resolve().parents[1]
# A first-party QA script in the bot script's slot: it adds the test client
# and plays it.
CHECKS = r'''
main()
{
	level thread run();
}

check(name, ok, detail)
{
	if(ok)
		println("[mine-qa] PASS " + name);
	else
		println("[mine-qa] FAIL " + name + " " + detail);
}

// What _menus.gsc does with a response from the class menu (see
// test-browser-classes-runtime.py).
answer(p, response)
{
	p closeMenu();
	p closeInGameMenu();
	p [[level.weapon]](response);
}

mines()
{
	return getEntArray("grenade", "classname");
}

state(p)
{
	down = mines();
	text = "alive=" + isAlive(p) + " carried=" + p getAmmoCount("mine_mp") + " down=" + down.size + " at=" + p.origin;
	for(i = 0; i < down.size; i++)
		text = text + " mine" + i + "=" + down[i].origin;
	return text;
}

// A tap of G: the hand sets one down a step ahead.
place(p)
{
	p botAction("+frag");
	wait .1;
	p botAction("-frag");
	wait 2.5;
}

use(p, seconds)
{
	p botAction("+activate");
	wait seconds;
	p botAction("-activate");
	wait .3;
}

run()
{
	wait 2;
	p = addTestClient();
	wait 2;
	p [[level.allies]]();
	wait .5;
	answer(p, "class,m1garand_mp,colt_mp,mine_mp,smoke_grenade_american_mp");
	wait 2;
	p botStop();
	check("spawns-with-two-mines", isAlive(p) && p getAmmoCount("mine_mp") == 2 && mines().size == 0, state(p));

	place(p);
	check("g-sets-one-down", p getAmmoCount("mine_mp") == 1 && mines().size == 1, state(p));
	use(p, 1);
	check("held-use-takes-it-back", p getAmmoCount("mine_mp") == 2 && mines().size == 0, state(p));

	place(p);
	p botAngles((0, 90, 0));
	wait .5;
	place(p);
	check("both-set-down", p getAmmoCount("mine_mp") == 0 && mines().size == 2, state(p));
	use(p, 1);
	use(p, 1);
	check("both-taken-back", p getAmmoCount("mine_mp") == 2 && mines().size == 0, state(p));

	place(p);
	use(p, .15);
	check("a-tap-of-use-leaves-it", p getAmmoCount("mine_mp") == 1 && mines().size == 1, state(p));
	use(p, 1);
	check("a-hold-takes-it", p getAmmoCount("mine_mp") == 2 && mines().size == 0, state(p));

	// As in WWII, the mine stays through its owner's death and goes when they
	// spawn again, with both mines.
	place(p);
	p suicide();
	wait .05;
	check("stays-through-the-death", !isAlive(p) && mines().size == 1, state(p));
	for(i = 0; i < 40 && !isAlive(p); i++)
		wait .25;
	wait 1;
	p botStop();
	check("gone-when-its-owner-spawns-again", isAlive(p) && p getAmmoCount("mine_mp") == 2 && mines().size == 0,
		state(p));
	println("[mine-qa] DONE");
}
'''
EXPECTED = 9


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--map', choices=['mp_toujane', 'mp_carentan', 'mp_dawnville', 'mp_burgundy', 'mp_railyard', 'mp_brecourt'], default='mp_toujane')
    parser.add_argument('--image', default='local/cod2-native-server:dev')
    parser.add_argument('--assets', type=Path, default=ROOT / 'data/browser/main')
    parser.add_argument('--binary', type=Path, default=ROOT / 'out/cod2-native/cod2_lnxded')
    parser.add_argument('--classes', type=Path, default=ROOT / 'downstream/server/browser_classes.gsc')
    parser.add_argument('--log', type=Path, default=ROOT / 'out/placed-explosives-runtime/server.log')
    args = parser.parse_args()
    for item in (args.assets, args.binary, args.classes):
        if not item.exists():
            parser.error(f'missing {item}')
    args.log.parent.mkdir(parents=True, exist_ok=True)
    name = 'cod2-mine-qa-' + uuid.uuid4().hex[:10]
    log = ''
    with tempfile.TemporaryDirectory(prefix='cod2-mine-qa-') as directory:
        checks = Path(directory) / 'checks.gsc'
        checks.write_text(CHECKS)
        try:
            subprocess.run(['docker', 'run', '-d', '--name', name, '--platform', 'linux/amd64', '--network', 'none',
                            '--ulimit', 'core=0', '--read-only', '--tmpfs', '/profile:uid=1000,gid=1000,size=32m',
                            '--tmpfs', '/tmp:size=16m', '-v', f'{args.assets.resolve()}:/game/main:ro',
                            '-v', f'{args.binary.resolve()}:/qa-engine:ro',
                            '-v', f'{args.classes.resolve()}:/qa-classes.gsc:ro',
                            '-v', f'{checks}:/qa-checks.gsc:ro', '-v', f'{(ROOT / "server.cfg").resolve()}:/qa-server.cfg:ro',
                            '--entrypoint', '/bin/sh', args.image, '-c',
                            'mkdir -p /profile/raw/maps/mp; cp /qa-classes.gsc /profile/raw/maps/mp/browser_classes.gsc; '
                            'cp /qa-checks.gsc /profile/raw/maps/mp/browser_bots.gsc; cp /qa-server.cfg /profile/raw/server.cfg; '
                            'exec /qa-engine +set dedicated 1 +set fs_basepath /game +set fs_homepath /profile '
                            '+set net_ip 127.0.0.1 +set sv_maxclients 16 +exec server.cfg '
                            '+set scr_bot_count 1 +set scr_forcerespawn 1 +set scr_tdm_timelimit 0 '
                            f'+set scr_tdm_scorelimit 0 +set g_gametype tdm +map {args.map}'],
                           check=True, capture_output=True)
            deadline = time.monotonic() + 120
            while time.monotonic() < deadline:
                log = subprocess.run(['docker', 'logs', name], capture_output=True, text=True).stdout
                if '[mine-qa] DONE' in log or re.search(r'script (?:compile|runtime) error|Error during initialization', log, re.I):
                    break
                time.sleep(1)
        finally:
            log = subprocess.run(['docker', 'logs', name], capture_output=True, text=True).stdout or log
            subprocess.run(['docker', 'rm', '-f', name], capture_output=True)
    args.log.write_text(log)
    errors = re.findall(r'.*script (?:compile|runtime) error.*', log, re.I)
    failures = re.findall(r'\[mine-qa\] FAIL .*', log)
    passes = re.findall(r'\[mine-qa\] PASS (\S+)', log)
    if errors or failures or len(passes) != EXPECTED:
        raise SystemExit('\n'.join(errors + failures) + f'\n{len(passes)}/{EXPECTED} passed; log: {args.log}')
    print(f'PASS: {EXPECTED} S-Mine checks on {args.map} through the dedicated server: a class with the S-Mine spawns '
          'with two, G sets one down, a held Use takes it back and a tap leaves it, both go down and come back, and '
          'a mine stays through its owner\'s death and goes when they spawn again with both')


if __name__ == '__main__':
    main()
