// First-party WWII match start. Players who spawn before the match begins wait
// frozen, free to look and aim, through one countdown that the HUD draws. It
// starts when the first person spawns or chooses to spectate, and the
// gametype's clock starts when it ends. S&D counts down through its first
// round's grace period instead, which its own round clock already covers.
main()
{
	length = 15;
	if(getCvar("scr_prematch_time") != "")
		length = getCvarFloat("scr_prematch_time");
	if(length > 60)
		length = 60;
	level.browser_prematch_clock = getCvar("g_gametype") != "sd";
	if(!level.browser_prematch_clock)
	{
		if(!game["matchstarted"] || game["roundsplayed"] > 0)
			return;
		if(level.graceperiod < length)
			length = level.graceperiod;
	}
	if(length <= 0)
		return;
	level.browser_prematch_length = length;
	if(!level.browser_prematch_clock)
		begin();
	level thread onPlayerConnect();
	level thread run();
}

begin()
{
	level.browser_prematch_start = getTime();
	level.browser_prematch_end = level.browser_prematch_start + int(level.browser_prematch_length * 1000);
}

run()
{
	level endon("intermission");
	while(!isDefined(level.browser_prematch_end) || getTime() < level.browser_prematch_end)
	{
		if(level.browser_prematch_clock)
			holdClock();
		wait .05;
	}
	if(level.browser_prematch_clock)
		holdClock();
	level notify("browser_prematch_over");
	players = getEntArray("player", "classname");
	for(i = 0; i < players.size; i++)
		players[i] freezeControls(false);
}

// The time limit counts from the end of the countdown; until then the clock
// shows all of it.
holdClock()
{
	level.starttime = getTime();
	if(isDefined(level.clock) && isDefined(level.timelimit) && level.timelimit > 0)
		level.clock setTimer(level.timelimit * 60);
}

onPlayerConnect()
{
	level endon("browser_prematch_over");
	for(;;)
	{
		level waittill("connecting", player);
		player thread onPlayerSpawned();
		player thread onPlayerSpectating();
	}
}

// A person who would rather watch starts the match for the bots.
onPlayerSpectating()
{
	self endon("disconnect");
	level endon("browser_prematch_over");
	self waittill("joined_spectators");
	if(!self isBot() && !isDefined(level.browser_prematch_end))
		begin();
}

// Bots wait frozen for the first person, who starts the countdown.
onPlayerSpawned()
{
	self endon("disconnect");
	level endon("browser_prematch_over");
	for(;;)
	{
		self waittill("spawned_player");
		self freezeControls(true);
		if(self isBot())
			continue;
		if(!isDefined(level.browser_prematch_end))
			begin();
		self setClientCvar("cg_prematch", level.browser_prematch_start + " " + level.browser_prematch_end);
	}
}
