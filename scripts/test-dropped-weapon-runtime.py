#!/usr/bin/env python3
"""A gun dropped on death keeps its attachments through the real dedicated server.

Requires Docker, the native server image and the private browser archives;
no network. A test client answers the class menu with a Thompson with an
Extended Mag and a Suppressor and dies; a second one, its primary slot empty,
stands on the gun it dropped and takes it with Use, as a player does, with its
longer clip and reserve. Before dropped guns kept their attachments the second
took a plain Thompson's 20 and 160.
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile
import time
import uuid

ROOT = Path(__file__).resolve().parents[1]
# A first-party QA script in the bot script's slot: it adds the test clients
# and plays them.
CHECKS = r'''
main()
{
	level thread run();
}

check(name, ok, detail)
{
	if(ok)
		println("[drop-qa] PASS " + name);
	else
		println("[drop-qa] FAIL " + name + " " + detail);
}

// What _menus.gsc does with a response from the class menu (see
// test-browser-classes-runtime.py).
answer(p, response)
{
	p closeMenu();
	p closeInGameMenu();
	p [[level.weapon]](response);
}

guns()
{
	return getEntArray("weapon_thompson_mp", "classname");
}

state(p)
{
	down = guns();
	text = "alive=" + isAlive(p) + " primary=" + p getWeaponSlotWeapon("primary") + " clip=" +
		p getWeaponSlotClipAmmo("primary") + " reserve=" + p getWeaponSlotAmmo("primary") + " down=" + down.size +
		" at=" + p.origin;
	for(i = 0; i < down.size; i++)
		text = text + " gun" + i + "=" + down[i].origin;
	return text;
}

// With the pistol in hand, as the gun it held is gone, onto the gun on the
// ground, looking down at it, and Use, as a player takes a gun they lack.
walkOnto(p)
{
	p switchToWeapon("colt_mp");
	wait 1.5;
	down = guns();
	if(down.size)
		p setOrigin(down[0].origin);
	p botAngles((80, 0, 0));
	wait .5;
	p botAction("+activate");
	wait 1;
	p botAction("-activate");
	wait .5;
}

run()
{
	wait 2;
	a = addTestClient();
	wait 1;
	b = addTestClient();
	wait 2;
	a [[level.allies]]();
	b [[level.allies]]();
	wait .5;
	answer(a, "class,thompson_mp,colt_mp,frag_grenade_american_mp,smoke_grenade_american_mp,ember,none,extmag+suppressor,none");
	answer(b, "class,m1garand_mp,colt_mp,frag_grenade_american_mp,smoke_grenade_american_mp");
	wait 2;
	a botStop();
	b botStop();
	check("spawns-with-the-longer-clip", isAlive(a) && a getWeaponSlotWeapon("primary") == "thompson_mp" &&
		a getWeaponSlotClipAmmo("primary") == 30, state(a));

	a suicide();
	wait .2;
	check("drops-it-on-death", !isAlive(a) && guns().size == 1, state(a));

	b takeWeapon("m1garand_mp");
	walkOnto(b);
	check("taken-by-another", b getWeaponSlotWeapon("primary") == "thompson_mp" && guns().size == 0, state(b));
	check("with-its-longer-clip", b getWeaponSlotClipAmmo("primary") == 30 &&
		b getWeaponSlotAmmo("primary") == 240, state(b));
	println("[drop-qa] DONE");
}
'''
EXPECTED = 4


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--map', choices=['mp_toujane', 'mp_carentan', 'mp_dawnville', 'mp_burgundy', 'mp_railyard'], default='mp_toujane')
    parser.add_argument('--image', default='local/cod2-native-server:dev')
    parser.add_argument('--assets', type=Path, default=ROOT / 'data/browser/main')
    parser.add_argument('--binary', type=Path, default=ROOT / 'out/cod2-native/cod2_lnxded')
    parser.add_argument('--classes', type=Path, default=ROOT / 'downstream/server/browser_classes.gsc')
    parser.add_argument('--log', type=Path, default=ROOT / 'out/dropped-weapon-runtime/server.log')
    args = parser.parse_args()
    for item in (args.assets, args.binary, args.classes):
        if not item.exists():
            parser.error(f'missing {item}')
    args.log.parent.mkdir(parents=True, exist_ok=True)
    name = 'cod2-drop-qa-' + uuid.uuid4().hex[:10]
    log = ''
    with tempfile.TemporaryDirectory(prefix='cod2-drop-qa-') as directory:
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
                            '+set scr_bot_count 2 +set scr_forcerespawn 1 +set scr_tdm_timelimit 0 '
                            f'+set scr_tdm_scorelimit 0 +set g_gametype tdm +map {args.map}'],
                           check=True, capture_output=True)
            deadline = time.monotonic() + 120
            while time.monotonic() < deadline:
                log = subprocess.run(['docker', 'logs', name], capture_output=True, text=True).stdout
                if '[drop-qa] DONE' in log or re.search(r'script (?:compile|runtime) error|Error during initialization', log, re.I):
                    break
                time.sleep(1)
        finally:
            log = subprocess.run(['docker', 'logs', name], capture_output=True, text=True).stdout or log
            subprocess.run(['docker', 'rm', '-f', name], capture_output=True)
    args.log.write_text(log)
    errors = re.findall(r'.*script (?:compile|runtime) error.*', log, re.I)
    failures = re.findall(r'\[drop-qa\] FAIL .*', log)
    passes = re.findall(r'\[drop-qa\] PASS (\S+)', log)
    if errors or failures or len(passes) != EXPECTED:
        raise SystemExit('\n'.join(errors + failures) + f'\n{len(passes)}/{EXPECTED} passed; log: {args.log}')
    print(f'PASS: {EXPECTED} dropped-gun checks on {args.map} through the dedicated server: a Thompson with an '
          'Extended Mag and a Suppressor spawns with its longer clip, drops on death, and the player who takes it '
          'has that clip and reserve')


if __name__ == '__main__':
    main()
