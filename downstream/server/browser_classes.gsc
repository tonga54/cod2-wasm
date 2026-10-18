// First-party custom classes. Players pick one of their five classes where
// the gametype offered their faction's weapons; any class serves either team.
// The original callbacks keep spawning, restrictions, messages and rounds.
main()
{
	level.browser_class_primary = primaryWeapons();
	level.browser_class_secondary = secondaryWeapons();
	level.browser_class_lethal = lethalWeapons();
	level.browser_class_tactical = tacticalWeapons();
	precacheWeapons(level.browser_class_primary);
	precacheWeapons(level.browser_class_secondary);
	precacheWeapons(level.browser_class_lethal);
	precacheWeapons(level.browser_class_tactical);

	precacheMenu("browser_class");
	game["menu_weapon_allies"] = "browser_class";
	game["menu_weapon_axis"] = "browser_class";
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
	return weapons;
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

listed(weapons, weapon)
{
	for(i = 0; i < weapons.size; i++)
		if(weapons[i] == weapon)
			return true;
	return false;
}

// "class,primary,secondary,lethal,tactical" from the class menu; a bare
// weapon name, as bots send, goes to the original callback unchanged.
menuClass(response)
{
	tokens = strtok(response, ",");
	if(tokens.size != 5 || tokens[0] != "class")
	{
		self [[level.browser_class_weapon]](response);
		return;
	}
	if(!listed(level.browser_class_primary, tokens[1]) || !listed(level.browser_class_secondary, tokens[2]) ||
	   !listed(level.browser_class_lethal, tokens[3]) || !listed(level.browser_class_tactical, tokens[4]))
	{
		if(isDefined(self.pers["team"]) && (self.pers["team"] == "allies" || self.pers["team"] == "axis"))
			self openMenu(game["menu_weapon_" + self.pers["team"]]);
		return;
	}
	loadout = [];
	for(i = 0; i < 4; i++)
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
		player thread onPlayerSpawned();
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
// the original grenade count for its primary and one smoke grenade.
applyClass()
{
	if(!isDefined(self.pers["browser_class"]) || !isDefined(self.pers["weapon"]))
		return;
	loadout = self.pers["browser_class"];
	// A restricted or different weapon was chosen since; keep the gametype's.
	if(loadout[0] != self.pers["weapon"])
		return;

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
		self setWeaponClipAmmo(loadout[2], maps\mp\gametypes\_weapons::getWeaponBasedGrenadeCount(loadout[0]));
		self switchToOffhand(loadout[2]);
	}
	self giveWeapon("binoculars_mp");
	self giveWeapon(loadout[0]);
	self giveMaxAmmo(loadout[0]);
	self setSpawnWeapon(loadout[0]);
}
