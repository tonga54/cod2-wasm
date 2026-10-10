// First-party bot lifecycle. Original gametype callbacks retain responsibility
// for factions, weapon restrictions, spawning, round rules and respawning.
main()
{
    level thread populate();
}

populate()
{
    level endon("intermission");
    wait 1;
    count = getCvarInt("scr_bot_count");
    difficulty = getCvarInt("scr_bot_difficulty");
    if(count < 0) count = 0;
    if(count > 16) count = 16;
    if(difficulty < 0 || difficulty > 2) difficulty = 1;

    // map_restart and map rotation can retain connected test clients.
    players = getEntArray("player", "classname");
    existing = 0;
    for(i = 0; i < players.size; i++)
    {
        if(players[i] isBot())
        {
            players[i] thread joinGame(difficulty);
            existing++;
        }
    }
    for(i = existing; i < count; i++)
    {
        bot = addTestClient();
        if(isDefined(bot))
            bot thread joinGame(difficulty);
        wait .25;
    }
}

joinGame(difficulty)
{
    self endon("disconnect");
    level endon("intermission");
    // Let the original connect callback finish its spectator/menu setup.
    wait .25;
    self.pers["skipserverinfo"] = true;
    if(!isDefined(self.pers["team"]) || self.pers["team"] == "spectator")
        self [[level.autoassign]]();

    if(!isDefined(self.pers["weapon"]))
    {
        faction = game[self.pers["team"]];
        slot = (self getEntityNumber()) % 3;
        choices[0] = "mp40_mp";
        choices[1] = "mp44_mp";
        choices[2] = "kar98k_mp";
        if(faction == "american")
        {
            choices[0] = "thompson_mp";
            choices[1] = "bar_mp";
            choices[2] = "m1garand_mp";
        }
        if(faction == "british")
        {
            choices[0] = "sten_mp";
            choices[1] = "bren_mp";
            choices[2] = "enfield_mp";
        }
        weapon = choices[slot];
        self [[level.weapon]](weapon);
    }
    self closeMenu();
    self closeInGameMenu();
    self botAI(difficulty);
    // Native commands use the normal use button to request a respawn. S&D and
    // HQ still decide whether that player may respawn; no forced spawn here.
}
