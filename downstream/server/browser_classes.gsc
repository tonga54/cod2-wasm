// First-party custom classes. A player joining a match picks a class before
// anything else and the gametype assigns the team; later choices replace the
// faction's weapon menu. Any class serves either team. The original callbacks
// keep team balance, spawning, restrictions, messages and rounds.
main()
{
	level.browser_class_primary = primaryWeapons();
	level.browser_class_secondary = secondaryWeapons();
	level.browser_class_lethal = lethalWeapons();
	level.browser_class_tactical = tacticalWeapons();
	level.browser_class_camos = camoIds();
	precacheWeapons(level.browser_class_primary);
	precacheWeapons(level.browser_class_secondary);
	precacheWeapons(level.browser_class_lethal);
	precacheWeapons(level.browser_class_tactical);

	level.browser_class_menu = "browser_class";
	precacheMenu(level.browser_class_menu);
	game["menu_weapon_allies"] = level.browser_class_menu;
	game["menu_weapon_axis"] = level.browser_class_menu;
	// The connect callbacks open it for a player without a team, in place of
	// the server's information and the team choice.
	game["menu_serverinfo"] = level.browser_class_menu;
	level.browser_class_weapon = level.weapon;
	level.weapon = ::menuClass;
	level thread onPlayerConnect();
}

precacheWeapons(weapons)
{
	for(i = 0; i < weapons.size; i++)
		precacheItem(weapons[i]);
}

primaryWeapons()
{
	weapons = [];
	weapons[0] = "thompson_mp";
	weapons[1] = "greasegun_mp";
	weapons[2] = "sten_mp";
	weapons[3] = "PPS42_mp";
	weapons[4] = "ppsh_mp";
	weapons[5] = "mp40_mp";
	weapons[6] = "m1garand_mp";
	weapons[7] = "m1carbine_mp";
	weapons[8] = "enfield_mp";
	weapons[9] = "mosin_nagant_mp";
	weapons[10] = "SVT40_mp";
	weapons[11] = "kar98k_mp";
	weapons[12] = "g43_mp";
	weapons[13] = "bar_mp";
	weapons[14] = "bren_mp";
	weapons[15] = "mp44_mp";
	weapons[16] = "springfield_mp";
	weapons[17] = "enfield_scope_mp";
	weapons[18] = "mosin_nagant_sniper_mp";
	weapons[19] = "kar98k_sniper_mp";
	weapons[20] = "shotgun_mp";
	return weapons;
}

secondaryWeapons()
{
	weapons = [];
	weapons[0] = "colt_mp";
	weapons[1] = "webley_mp";
	weapons[2] = "TT30_mp";
	weapons[3] = "luger_mp";
	return weapons;
}

lethalWeapons()
{
	weapons = [];
	weapons[0] = "frag_grenade_american_mp";
	weapons[1] = "frag_grenade_british_mp";
	weapons[2] = "frag_grenade_russian_mp";
	weapons[3] = "frag_grenade_german_mp";
	weapons[4] = "mine_mp";
	weapons[5] = "c4_mp";
	return weapons;
}

// The land mine and C4 come two to a life; C4 is taken out with G instead of
// thrown from the hand.
placedExplosive(weapon)
{
	return weapon == "mine_mp" || weapon == "c4_mp";
}

tacticalWeapons()
{
	weapons = [];
	weapons[0] = "smoke_grenade_american_mp";
	weapons[1] = "smoke_grenade_british_mp";
	weapons[2] = "smoke_grenade_russian_mp";
	weapons[3] = "smoke_grenade_german_mp";
	return weapons;
}

// The camouflage a class paints on its primary and its pistol; "none"
// keeps the gun's own finish.
camoIds()
{
	camos = [];
	camos[0] = "none";
	camos[1] = "frogskin";
	camos[2] = "brownspot";
	camos[3] = "splinter";
	camos[4] = "oakleaf";
	camos[5] = "peapattern";
	camos[6] = "planetree";
	camos[7] = "desert";
	camos[8] = "winter";
	camos[9] = "ambush";
	camos[10] = "ember";
	return camos;
}

listed(weapons, weapon)
{
	for(i = 0; i < weapons.size; i++)
		if(weapons[i] == weapon)
			return true;
	return false;
}

onTeam()
{
	return isDefined(self.pers["team"]) && (self.pers["team"] == "allies" || self.pers["team"] == "axis");
}

// "class,primary,secondary,lethal,tactical,primaryCamo,secondaryCamo" from the
// class menu, or without the camouflage from a client built before it; a bare
// weapon name, as bots send, goes to the original callback unchanged.
menuClass(response)
{
	tokens = strtok(response, ",");
	if((tokens.size != 5 && tokens.size != 7) || tokens[0] != "class")
	{
		self [[level.browser_class_weapon]](response);
		return;
	}
	if(tokens.size == 5)
	{
		tokens[5] = "none";
		tokens[6] = "none";
	}
	if(!listed(level.browser_class_primary, tokens[1]) || !listed(level.browser_class_secondary, tokens[2]) ||
	   !listed(level.browser_class_lethal, tokens[3]) || !listed(level.browser_class_tactical, tokens[4]) ||
	   !listed(level.browser_class_camos, tokens[5]) || !listed(level.browser_class_camos, tokens[6]))
	{
		self openMenu(level.browser_class_menu);
		return;
	}
	if(!self onTeam())
	{
		if(isDefined(game["state"]) && game["state"] == "intermission")
			return;
		// The gametype's own choice: the smaller team, or the one behind.
		self [[level.autoassign]]();
		// It opens the team's weapon menu, the class menu just answered.
		self closeMenu();
		if(!self onTeam())
			return;
	}
	loadout = [];
	for(i = 0; i < 6; i++)
		loadout[i] = tokens[i + 1];
	self.pers["browser_class"] = loadout;
	before = self getWeaponSlotWeapon("primary");
	self [[level.browser_class_weapon]](loadout[0]);
	// S&D swaps weapons in place before a round; others wait for a spawn.
	if(isAlive(self) && self.sessionstate == "playing" && before != "none" && before != loadout[0] &&
	   self getWeaponSlotWeapon("primary") == loadout[0])
		self applyClass();
}

onPlayerConnect()
{
	for(;;)
	{
		level waittill("connecting", player);
		// A client slot keeps its configstring: not its last owner's paint.
		player setWeaponCamos("");
		player thread onPlayerSpawned();
		player thread onMenuResponse();
	}
}

// Without a team, the in-game menu's Choose Class opens the class menu too:
// choosing there assigns the team.
onMenuResponse()
{
	self endon("disconnect");
	for(;;)
	{
		self waittill("menuresponse", menu, response);
		if(menu != game["menu_ingame"] || response != "changeweapon" || self onTeam())
			continue;
		// After _menus.gsc has closed the in-game menu for the same response.
		waittillframeend;
		self openMenu(level.browser_class_menu);
	}
}

onPlayerSpawned()
{
	self endon("disconnect");
	for(;;)
	{
		self waittill("spawned_player");
		self applyClass();
	}
}

// After the gametype's own loadout: the class's weapons in its slots, with
// the original grenade count for its primary and one smoke grenade, and
// their camouflage for every client to paint.
applyClass()
{
	if(!isDefined(self.pers["browser_class"]) || !isDefined(self.pers["weapon"]))
		return;
	loadout = self.pers["browser_class"];
	// A restricted or different weapon was chosen since; keep the gametype's.
	if(loadout[0] != self.pers["weapon"])
	{
		self setWeaponCamos("");
		return;
	}

	self takeAllWeapons();
	self setWeaponSlotWeapon("primaryb", loadout[1]);
	self giveMaxAmmo(loadout[1]);
	if(getCvarInt("scr_allow_smokegrenades"))
	{
		self giveWeapon(loadout[3]);
		self setWeaponClipAmmo(loadout[3], 1);
	}
	if(getCvarInt("scr_allow_fraggrenades"))
	{
		self giveWeapon(loadout[2]);
		if(placedExplosive(loadout[2]))
			self setWeaponClipAmmo(loadout[2], 2);
		else
			self setWeaponClipAmmo(loadout[2], maps\mp\gametypes\_weapons::getWeaponBasedGrenadeCount(loadout[0]));
		if(loadout[2] != "c4_mp")
			self switchToOffhand(loadout[2]);
	}
	self giveWeapon("binoculars_mp");
	self giveWeapon(loadout[0]);
	self giveMaxAmmo(loadout[0]);
	self setSpawnWeapon(loadout[0]);
	self setWeaponCamos(loadout[0] + " " + loadout[4] + " " + loadout[1] + " " + loadout[5]);
}
