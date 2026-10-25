#!/usr/bin/env python3
"""Choose custom classes through the real dedicated server's menu path.

Requires Docker, the native server image and the private browser archives;
no network. Test clients answer the class menu as the browser does, through
the gametype's own menu handler, and the loadouts are read back after spawn.
Clients that have just joined answer it before choosing a team, as a player
does when the class menu opens first, and get a team from the gametype.
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile
import time
import uuid

ROOT = Path(__file__).resolve().parents[1]
# A first-party QA script in the bot script's slot: it adds test clients and
# answers the class menu for them.
CHECKS = r'''
main()
{
	level thread run();
}

check(name, ok, detail)
{
	if(ok)
		println("[class-qa] PASS " + name);
	else
		println("[class-qa] FAIL " + name + " " + detail);
}

loadout(p)
{
	return "team=" + p.pers["team"] + " alive=" + isAlive(p) + " primary=" + p getWeaponSlotWeapon("primary") +
		" secondary=" + p getWeaponSlotWeapon("primaryb");
}

onTeam(p)
{
	return isDefined(p.pers["team"]) && (p.pers["team"] == "allies" || p.pers["team"] == "axis");
}

// What _menus.gsc does with a response from the faction's weapon menu, the
// class menu once browser_classes.gsc took its place. (This VM delivers a
// script notify's arguments to waittill in reverse, so the response is not
// sent as one; real clients' responses arrive through the engine.)
answer(p, response)
{
	p closeMenu();
	p closeInGameMenu();
	p [[level.weapon]](response);
}

run()
{
	wait 2;
	players = [];
	for(i = 0; i < 9; i++)
	{
		players[i] = addTestClient();
		wait .5;
	}
	wait 2;
	check("class-menu-replaces-weapon-menus", game["menu_weapon_allies"] == "browser_class" &&
		game["menu_weapon_axis"] == "browser_class", game["menu_weapon_allies"] + " " + game["menu_weapon_axis"]);
	// The connect callback opens it for a player without a team, first.
	check("connect-opens-the-class-menu", game["menu_serverinfo"] == "browser_class", game["menu_serverinfo"]);
	players[0] [[level.allies]]();
	players[1] [[level.axis]]();
	players[2] [[level.allies]]();
	players[3] [[level.axis]]();
	wait .5;
	// A client from before camouflage and attachments ends after the tactical.
	answer(players[0], "class,kar98k_mp,luger_mp,frag_grenade_german_mp,smoke_grenade_russian_mp");
	answer(players[1], "class,ppsh_mp,colt_mp,frag_grenade_russian_mp,smoke_grenade_british_mp,none,none," +
		"extmag+suppressor,suppressor");
	answer(players[2], "class,mg42_bipod_stand_mp,colt_mp,frag_grenade_american_mp,smoke_grenade_american_mp");
	answer(players[3], "mp40_mp");
	wait 1;

	p = players[0];
	check("allies-carry-a-german-class", p.pers["team"] == "allies" && isAlive(p) &&
		p getWeaponSlotWeapon("primary") == "kar98k_mp" && p getWeaponSlotWeapon("primaryb") == "luger_mp" &&
		p getAmmoCount("frag_grenade_german_mp") == 3 && p getAmmoCount("smoke_grenade_russian_mp") == 1 &&
		p getAmmoCount("frag_grenade_" + game["allies"] + "_mp") == 0 &&
		p getAmmoCount("smoke_grenade_" + game["allies"] + "_mp") == 0, loadout(p));
	p = players[1];
	check("axis-carry-soviet-weapons-on-any-map", p.pers["team"] == "axis" && isAlive(p) &&
		p getWeaponSlotWeapon("primary") == "ppsh_mp" && p getWeaponSlotWeapon("primaryb") == "colt_mp" &&
		p getAmmoCount("frag_grenade_russian_mp") == 1 && p getAmmoCount("smoke_grenade_british_mp") == 1 &&
		p getAmmoCount("frag_grenade_german_mp") == 0, loadout(p));
	// The PPSh's 71 rounds and 213 in reserve, half again.
	check("attachments-fit-at-spawn-with-full-magazines", p getWeaponSlotClipAmmo("primary") == 106 &&
		p getWeaponSlotAmmo("primary") == 319 && p getWeaponSlotClipAmmo("primaryb") == 7,
		loadout(p) + " clip=" + p getWeaponSlotClipAmmo("primary") + " ammo=" + p getWeaponSlotAmmo("primary"));
	check("older-clients-carry-no-attachments", players[0] getWeaponSlotClipAmmo("primary") == 5 &&
		players[0].pers["browser_class"][6] == "none" && players[0].pers["browser_class"][7] == "none", loadout(players[0]));
	p = players[2];
	check("invalid-class-rejected", !isAlive(p) && !isDefined(p.pers["weapon"]) &&
		!isDefined(p.pers["browser_class"]), loadout(p));
	answer(p, "class,m1garand_mp,colt_mp,frag_grenade_american_mp,smoke_grenade_american_mp,none,none,suppressor,none");
	answer(p, "class,thompson_mp,colt_mp,frag_grenade_american_mp,smoke_grenade_american_mp,none,none,reflex+lens,none");
	answer(p, "class,thompson_mp,colt_mp,frag_grenade_american_mp,smoke_grenade_american_mp,none,none,none," +
		"suppressor+extmag");
	wait .5;
	check("invalid-attachments-rejected", !isAlive(p) && !isDefined(p.pers["browser_class"]), loadout(p));
	p = players[3];
	check("bare-weapon-keeps-the-gametype-loadout", isAlive(p) && p getWeaponSlotWeapon("primary") == "mp40_mp" &&
		p getWeaponSlotWeapon("primaryb") == "luger_mp" && p getAmmoCount("frag_grenade_german_mp") == 1 &&
		!isDefined(p.pers["browser_class"]), loadout(p));
	check("engine-validates-attachments", validWeaponAttachments("thompson_mp", "reflex+grip+extmag", "primary") &&
		!validWeaponAttachments("thompson_mp", "reflex+lens", "primary") &&
		!validWeaponAttachments("thompson_mp", "grip+extmag+fmj+rifling", "primary") &&
		!validWeaponAttachments("colt_mp", "suppressor+extmag", "primaryb") &&
		!validWeaponAttachments("m1garand_mp", "suppressor", "primary") &&
		validWeaponAttachments("m1garand_mp", "none", "primary") &&
		!p setWeaponSlotAttachments("primary", "highcal") && p setWeaponSlotAttachments("primary", "extmag") &&
		p getWeaponSlotClipAmmo("primary") == 48, loadout(p) + " clip=" + p getWeaponSlotClipAmmo("primary"));

	// Players who have just joined choose a class before any team: the
	// gametype's auto-assign gives each the smaller team, then the class.
	p = players[4];
	check("joined-without-a-team", p.pers["team"] == "spectator" && !isAlive(p), loadout(p));
	answer(p, "class,ppsh_mp,luger_mp,frag_grenade_german_mp,smoke_grenade_american_mp");
	answer(players[5], "class,thompson_mp,TT30_mp,frag_grenade_russian_mp,smoke_grenade_german_mp,frogskin,ember");
	answer(players[6], "class,bren_mp,webley_mp,frag_grenade_american_mp,smoke_grenade_british_mp");
	// An invalid class leaves a player without a team, an unknown camouflage too.
	answer(players[7], "class,ppsh_mp,luger_mp,frag_grenade_german_mp,mg42_bipod_stand_mp");
	answer(players[8], "class,ppsh_mp,luger_mp,frag_grenade_german_mp,smoke_grenade_american_mp,Frogskin,none");
	wait 1;
	check("class-first-assigns-a-team", onTeam(p) && isAlive(p) && p getWeaponSlotWeapon("primary") == "ppsh_mp" &&
		p getWeaponSlotWeapon("primaryb") == "luger_mp" && p getAmmoCount("smoke_grenade_american_mp") == 1 &&
		p getAmmoCount("frag_grenade_german_mp") == maps\mp\gametypes\_weapons::getWeaponBasedGrenadeCount("ppsh_mp"),
		loadout(p));
	counts["allies"] = 0;
	counts["axis"] = 0;
	for(i = 0; i < 7; i++)
		if(onTeam(players[i]))
			counts[players[i].pers["team"]]++;
	check("class-first-balances-the-teams", counts["allies"] + counts["axis"] == 7 &&
		counts["allies"] - counts["axis"] <= 1 && counts["axis"] - counts["allies"] <= 1 &&
		isAlive(players[5]) && isAlive(players[6]), "allies=" + counts["allies"] + " axis=" + counts["axis"]);
	p = players[7];
	check("class-first-invalid-keeps-no-team", p.pers["team"] == "spectator" && !isAlive(p) &&
		!isDefined(p.pers["browser_class"]), loadout(p));
	// The camouflage rides with the class; a client from before it paints nothing.
	p = players[5];
	check("class-keeps-its-camouflage", isAlive(p) && p getWeaponSlotWeapon("primary") == "thompson_mp" &&
		p getWeaponSlotWeapon("primaryb") == "TT30_mp" && p.pers["browser_class"].size == 8 &&
		p.pers["browser_class"][4] == "frogskin" && p.pers["browser_class"][5] == "ember", loadout(p));
	p = players[4];
	check("class-without-camouflage-paints-none", isAlive(p) && p.pers["browser_class"].size == 8 &&
		p.pers["browser_class"][4] == "none" && p.pers["browser_class"][5] == "none", loadout(p));
	p = players[8];
	check("unknown-camouflage-rejected", p.pers["team"] == "spectator" && !isAlive(p) &&
		!isDefined(p.pers["browser_class"]), loadout(p));

	// A class chosen while alive waits for the next spawn.
	p = players[0];
	answer(p, "class,thompson_mp,webley_mp,frag_grenade_british_mp,smoke_grenade_german_mp,winter,none," +
		"extmag+rapidfire,quickdraw");
	wait .5;
	check("alive-change-waits-for-spawn", p getWeaponSlotWeapon("primary") == "kar98k_mp" &&
		p getWeaponSlotWeapon("primaryb") == "luger_mp", loadout(p));
	p suicide();
	// A server restriction still decides, as for the original weapon menus.
	setCvar("scr_allow_kar98k", "0");
	answer(players[3], "class,kar98k_mp,TT30_mp,frag_grenade_american_mp,smoke_grenade_american_mp");
	players[3] suicide();
	wait 6;
	check("respawn-applies-the-new-class", isAlive(p) && p getWeaponSlotWeapon("primary") == "thompson_mp" &&
		p getWeaponSlotWeapon("primaryb") == "webley_mp" && p getAmmoCount("frag_grenade_british_mp") == 1 &&
		p getAmmoCount("smoke_grenade_german_mp") == 1 && p getAmmoCount("frag_grenade_german_mp") == 0 &&
		p.pers["browser_class"][4] == "winter" && p.pers["browser_class"][5] == "none", loadout(p));
	// The Thompson's 20 rounds and 180 in reserve, half again.
	check("respawn-applies-the-new-attachments", p getWeaponSlotClipAmmo("primary") == 30 &&
		p getWeaponSlotAmmo("primary") == 270 && p getWeaponSlotClipAmmo("primaryb") == 6,
		loadout(p) + " clip=" + p getWeaponSlotClipAmmo("primary") + " ammo=" + p getWeaponSlotAmmo("primary"));
	// A weapon given again starts without them.
	p takeAllWeapons();
	p giveWeapon("thompson_mp");
	p setWeaponSlotClipAmmo("primary", 999);
	check("weapon-leaves-its-slot-without-attachments", p getWeaponSlotClipAmmo("primary") == 20,
		loadout(p) + " clip=" + p getWeaponSlotClipAmmo("primary"));
	p = players[3];
	check("restricted-primary-keeps-the-previous-weapon", isAlive(p) && p getWeaponSlotWeapon("primary") == "mp40_mp" &&
		p getWeaponSlotWeapon("primaryb") == "luger_mp" && p getAmmoCount("frag_grenade_american_mp") == 0, loadout(p));
	println("[class-qa] DONE");
}
'''
# S&D before its match: the gametype swaps a living player's weapons in place.
# One client, so the match never starts and the level does not restart.
CHECKS_SD = CHECKS[:CHECKS.index('run()\n')] + r'''
run()
{
	wait 2;
	p = addTestClient();
	wait 2;
	check("sd-connect-opens-the-class-menu", game["menu_serverinfo"] == "browser_class", game["menu_serverinfo"]);
	// Chosen before any team, as a player who has just joined does.
	answer(p, "class,kar98k_mp,TT30_mp,frag_grenade_russian_mp,smoke_grenade_german_mp");
	wait 1;
	check("sd-class-first-assigns-a-team", onTeam(p), loadout(p));
	check("sd-spawn-applies-the-class", isAlive(p) && p getWeaponSlotWeapon("primary") == "kar98k_mp" &&
		p getWeaponSlotWeapon("primaryb") == "TT30_mp" && p getAmmoCount("smoke_grenade_german_mp") == 1 &&
		p getAmmoCount("frag_grenade_russian_mp") == maps\mp\gametypes\_weapons::getWeaponBasedGrenadeCount("kar98k_mp"),
		loadout(p));
	// The gametype's own swap would hand over the allies' pistol and frags.
	answer(p, "class,thompson_mp,luger_mp,frag_grenade_german_mp,smoke_grenade_russian_mp,none,none,extmag,none");
	wait 1;
	check("sd-pre-round-swap-applies-the-whole-class", isAlive(p) && p getWeaponSlotWeapon("primary") == "thompson_mp" &&
		p getWeaponSlotWeapon("primaryb") == "luger_mp" && p getAmmoCount("smoke_grenade_russian_mp") == 1 &&
		p getWeaponSlotClipAmmo("primary") == 30 && p getWeaponSlotAmmo("primary") == 270 &&
		p getAmmoCount("frag_grenade_german_mp") == maps\mp\gametypes\_weapons::getWeaponBasedGrenadeCount("thompson_mp") &&
		p getAmmoCount("frag_grenade_" + game["allies"] + "_mp") == 0 && p getAmmoCount("frag_grenade_russian_mp") == 0 &&
		p getAmmoCount("smoke_grenade_german_mp") == 0, loadout(p));
	println("[class-qa] DONE");
}
'''
GAMETYPES = {
    'tdm': (CHECKS, 22, '+set scr_bot_count 1 +set scr_forcerespawn 1 +set scr_tdm_timelimit 0 '
                        '+set scr_tdm_scorelimit 0 +set g_gametype tdm'),
    # A bot count only loads the checks in place of the bot script.
    'sd': (CHECKS_SD, 4, '+set scr_bot_count 1 +set g_gametype sd'),
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--map', choices=['mp_toujane', 'mp_carentan', 'mp_dawnville', 'mp_burgundy', 'mp_railyard', 'mp_brecourt',
                                          'mp_matmata', 'mp_leningrad'], default='mp_toujane')
    parser.add_argument('--gametype', choices=sorted(GAMETYPES), default='tdm')
    parser.add_argument('--image', default='local/cod2-native-server:dev')
    parser.add_argument('--assets', type=Path, default=ROOT / 'data/browser/main')
    parser.add_argument('--binary', type=Path, default=ROOT / 'out/cod2-native/cod2_lnxded')
    parser.add_argument('--classes', type=Path, default=ROOT / 'downstream/server/browser_classes.gsc')
    parser.add_argument('--log', type=Path, default=ROOT / 'out/class-runtime/server.log')
    args = parser.parse_args()
    for item in (args.assets, args.binary, args.classes):
        if not item.exists():
            parser.error(f'missing {item}')
    args.log.parent.mkdir(parents=True, exist_ok=True)
    source, expected, gametype = GAMETYPES[args.gametype]
    name = 'cod2-class-qa-' + uuid.uuid4().hex[:10]
    log = ''
    with tempfile.TemporaryDirectory(prefix='cod2-class-qa-') as directory:
        checks = Path(directory) / 'checks.gsc'
        checks.write_text(source)
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
                            f'{gametype} +map {args.map}'], check=True, capture_output=True)
            deadline = time.monotonic() + 90
            while time.monotonic() < deadline:
                log = subprocess.run(['docker', 'logs', name], capture_output=True, text=True).stdout
                if '[class-qa] DONE' in log or re.search(r'script (?:compile|runtime) error|Error during initialization', log, re.I):
                    break
                time.sleep(1)
        finally:
            log = subprocess.run(['docker', 'logs', name], capture_output=True, text=True).stdout or log
            subprocess.run(['docker', 'rm', '-f', name], capture_output=True)
    args.log.write_text(log)
    errors = re.findall(r'.*script (?:compile|runtime) error.*', log, re.I)
    failures = re.findall(r'\[class-qa\] FAIL .*', log)
    passes = re.findall(r'\[class-qa\] PASS (\S+)', log)
    if errors or failures or len(passes) != expected:
        raise SystemExit('\n'.join(errors + failures) + f'\n{len(passes)}/{expected} passed; log: {args.log}')
    if args.gametype == 'sd':
        print(f'PASS: {expected} S&D class checks on {args.map} through the dedicated server: the class menu first, '
              'a team assigned with the class, the class at spawn, then whole before the match instead of the team\'s '
              'pistol and frags')
    else:
        print(f'PASS: {expected} class checks on {args.map} through the dedicated server: the class menu first, a '
              'class chosen without a team assigning the smaller one, any faction\'s weapons for either team, '
              'attachments fitted with full magazines, invalid classes, camouflage and attachments rejected, '
              'camouflage and attachments kept with their class, bare weapons and restrictions kept, changes '
              'applied at respawn')


if __name__ == '__main__':
    main()
