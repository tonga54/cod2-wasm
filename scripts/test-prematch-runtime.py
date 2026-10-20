#!/usr/bin/env python3
"""Hold a match for its WWII countdown through the real dedicated server.

Requires Docker, the native server image and the private browser archives;
no network. Test clients spawn into the room's countdown script and are told
to walk and fire; the checks read back whether they could, and the clock.
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile
import time
import uuid

ROOT = Path(__file__).resolve().parents[1]
# A first-party QA script in the bot script's slot. Test clients are bots,
# which never start the countdown, so the first person's spawn is called as
# that spawn's handler calls it.
COMMON = r'''
main()
{
	level thread run();
}

check(name, ok, detail)
{
	if(ok)
		println("[prematch-qa] PASS " + name + " " + detail);
	else
		println("[prematch-qa] FAIL " + name + " " + detail);
}

weapon(team)
{
	if(team == "axis")
		return "mp40_mp";
	if(game["allies"] == "british")
		return "sten_mp";
	if(game["allies"] == "russian")
		return "ppsh_mp";
	return "thompson_mp";
}

join(p, team)
{
	if(team == "allies")
		p [[level.allies]]();
	else
		p [[level.axis]]();
	wait .25;
	p closeMenu();
	p closeInGameMenu();
	p [[level.weapon]](weapon(team));
}

// A second of walking and firing: how far it went and how many rounds left
// the clip.
push(p)
{
	start = p.origin;
	clip = p getWeaponSlotClipAmmo("primary");
	p botMovement(127, 64);
	p botAction("+fire");
	wait 1;
	p botAction("-fire");
	p botMovement(0, 0);
	p.qa_moved = distance(start, p.origin);
	p.qa_fired = clip - p getWeaponSlotClipAmmo("primary");
}

pushed(p)
{
	return "alive=" + isAlive(p) + " moved=" + p.qa_moved + " fired=" + p.qa_fired;
}

waitForMatch()
{
	while(getTime() < level.browser_prematch_end + 250)
		wait .05;
}

since(t)
{
	d = getTime() - t;
	if(d < 0)
		d = 0 - d;
	return d;
}
'''
CHECKS_TDM = COMMON + r'''
run()
{
	wait 1;
	a = addTestClient();
	wait .5;
	b = addTestClient();
	wait 1;
	join(a, "allies");
	join(b, "axis");
	wait 1;
	check("bots-wait-frozen-for-a-person", isAlive(a) && isAlive(b) && !isDefined(level.browser_prematch_end),
		"alive=" + isAlive(a) + isAlive(b));
	check("time-limit-held-while-waiting", since(level.starttime) < 250, "starttime=" + level.starttime + " now=" + getTime());
	push(a);
	check("frozen-bot-neither-walks-nor-fires", a.qa_moved < 1 && a.qa_fired == 0, pushed(a));

	maps\mp\browser_prematch::begin();
	check("countdown-runs-scr-prematch-time", level.browser_prematch_end - level.browser_prematch_start == 3000,
		"start=" + level.browser_prematch_start + " end=" + level.browser_prematch_end);
	c = addTestClient();
	wait .5;
	join(c, "allies");
	wait .5;
	push(c);
	check("a-spawn-during-the-countdown-waits-frozen", isAlive(c) && c.qa_moved < 1 && c.qa_fired == 0, pushed(c));
	check("time-limit-held-through-the-countdown", since(level.starttime) < 250,
		"starttime=" + level.starttime + " now=" + getTime());

	waitForMatch();
	check("time-limit-starts-with-the-match", since(level.starttime) >= 150 && since(level.starttime) < 500,
		"starttime=" + level.starttime + " end=" + level.browser_prematch_end + " now=" + getTime());
	push(a);
	check("the-match-releases-everyone", a.qa_moved > 8 && a.qa_fired > 0, pushed(a));
	b suicide();
	wait 4.5;
	push(b);
	check("a-spawn-after-the-start-is-free", isAlive(b) && b.qa_moved > 8 && b.qa_fired > 0, pushed(b));
	println("[prematch-qa] DONE");
}
'''
# S&D: the warm-up before both teams exist has no countdown; the first round
# after the restart counts down from its start without waiting for anyone.
CHECKS_SD = COMMON + r'''
run()
{
	if(game["matchstarted"])
	{
		firstRound();
		return;
	}
	wait 1;
	a = addTestClient();
	wait .5;
	b = addTestClient();
	wait 1;
	join(a, "allies");
	join(b, "axis");
	wait .5;
	push(a);
	check("sd-warm-up-has-no-countdown", !isDefined(level.browser_prematch_end) && a.qa_moved > 8, pushed(a));
}

firstRound()
{
	check("sd-first-round-counts-down-from-its-start", isDefined(level.browser_prematch_end) &&
		level.browser_prematch_end - level.browser_prematch_start == 3000 && game["roundsplayed"] == 0,
		"rounds=" + game["roundsplayed"]);
	wait .5;
	players = getEntArray("player", "classname");
	p = undefined;
	for(i = 0; i < players.size; i++)
		if(isAlive(players[i]) && players[i].pers["team"] == game["attackers"])
			p = players[i];
	push(p);
	check("sd-round-start-waits-frozen", p.qa_moved < 1 && p.qa_fired == 0, pushed(p));
	waitForMatch();
	push(p);
	check("sd-countdown-end-releases-the-round", p.qa_moved > 8 && p.qa_fired > 0, pushed(p));
	println("[prematch-qa] DONE");
}
'''
GAMETYPES = {
    'tdm': (CHECKS_TDM, 9, '+set scr_tdm_timelimit 5 +set scr_tdm_scorelimit 0 +set scr_forcerespawn 1 +set g_gametype tdm'),
    'sd': (CHECKS_SD, 4, '+set scr_sd_timelimit 0 +set scr_sd_scorelimit 0 +set g_gametype sd'),
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--map', choices=['mp_toujane', 'mp_carentan', 'mp_dawnville', 'mp_burgundy', 'mp_railyard', 'mp_brecourt'], default='mp_toujane')
    parser.add_argument('--gametype', choices=sorted(GAMETYPES), default='tdm')
    parser.add_argument('--image', default='local/cod2-native-server:dev')
    parser.add_argument('--assets', type=Path, default=ROOT / 'data/browser/main')
    parser.add_argument('--binary', type=Path, default=ROOT / 'out/cod2-native/cod2_lnxded')
    parser.add_argument('--prematch', type=Path, default=ROOT / 'downstream/server/browser_prematch.gsc')
    parser.add_argument('--log', type=Path, default=ROOT / 'out/prematch-runtime/server.log')
    args = parser.parse_args()
    for item in (args.assets, args.binary, args.prematch):
        if not item.exists():
            parser.error(f'missing {item}')
    args.log.parent.mkdir(parents=True, exist_ok=True)
    source, expected, gametype = GAMETYPES[args.gametype]
    name = 'cod2-prematch-qa-' + uuid.uuid4().hex[:10]
    log = ''
    with tempfile.TemporaryDirectory(prefix='cod2-prematch-qa-') as directory:
        checks = Path(directory) / 'checks.gsc'
        checks.write_text(source)
        try:
            subprocess.run(['docker', 'run', '-d', '--name', name, '--platform', 'linux/amd64', '--network', 'none',
                            '--ulimit', 'core=0', '--read-only', '--tmpfs', '/profile:uid=1000,gid=1000,size=32m',
                            '--tmpfs', '/tmp:size=16m', '-v', f'{args.assets.resolve()}:/game/main:ro',
                            '-v', f'{args.binary.resolve()}:/qa-engine:ro',
                            '-v', f'{args.prematch.resolve()}:/qa-prematch.gsc:ro',
                            '-v', f'{checks}:/qa-checks.gsc:ro', '-v', f'{(ROOT / "server.cfg").resolve()}:/qa-server.cfg:ro',
                            '--entrypoint', '/bin/sh', args.image, '-c',
                            'mkdir -p /profile/raw/maps/mp; cp /qa-prematch.gsc /profile/raw/maps/mp/browser_prematch.gsc; '
                            'cp /qa-checks.gsc /profile/raw/maps/mp/browser_bots.gsc; cp /qa-server.cfg /profile/raw/server.cfg; '
                            'exec /qa-engine +set dedicated 1 +set fs_basepath /game +set fs_homepath /profile '
                            '+set net_ip 127.0.0.1 +set sv_maxclients 16 +exec server.cfg '
                            f'+set scr_bot_count 1 +set scr_prematch_time 3 {gametype} +map {args.map}'],
                           check=True, capture_output=True)
            deadline = time.monotonic() + 120
            while time.monotonic() < deadline:
                log = subprocess.run(['docker', 'logs', name], capture_output=True, text=True).stdout
                if '[prematch-qa] DONE' in log or re.search(r'script (?:compile|runtime) error|Error during initialization',
                                                           log, re.I):
                    break
                time.sleep(1)
        finally:
            log = subprocess.run(['docker', 'logs', name], capture_output=True, text=True).stdout or log
            subprocess.run(['docker', 'rm', '-f', name], capture_output=True)
    args.log.write_text(log)
    errors = re.findall(r'.*script (?:compile|runtime) error.*', log, re.I)
    failures = re.findall(r'\[prematch-qa\] FAIL .*', log)
    passes = re.findall(r'\[prematch-qa\] PASS (\S+)', log)
    if errors or failures or len(passes) != expected:
        raise SystemExit('\n'.join(errors + failures) + f'\n{len(passes)}/{expected} passed; log: {args.log}')
    if args.gametype == 'sd':
        print(f'PASS: {expected} S&D checks on {args.map} through the dedicated server: no countdown in the warm-up, '
              'then the first round held frozen for its countdown from its start and released at its end')
    else:
        print(f'PASS: {expected} TDM checks on {args.map} through the dedicated server: bots frozen and the clock held '
              'until the first person spawns, a 3 s countdown that freezes later spawns too, and the clock, every '
              'player and later spawns free when the match begins')


if __name__ == '__main__':
    main()
