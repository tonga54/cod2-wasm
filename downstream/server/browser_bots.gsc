// First-party bot lifecycle. Original gametype callbacks retain responsibility
// for factions, weapon restrictions, spawning, round rules and respawning.
main()
{
    level thread objectiveState();
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

    // Revalidate the faction on map rotation (British <-> American allies).
    faction = game[self.pers["team"]];
    if(!isDefined(self.pers["weapon"]) || !isDefined(self.pers["bot_faction"]) ||
       self.pers["bot_faction"] != faction)
    {
        faction = game[self.pers["team"]];
        // Allocate within each team: entity parity otherwise gives a whole
        // team the same weapon. One scoped rifle per four bots, with original
        // faction restrictions still enforced by the weapon callback.
        rank = 0;
        players = getEntArray("player", "classname");
        for(i = 0; i < players.size; i++)
            if(players[i] isBot() && isDefined(players[i].pers["team"]) &&
               players[i].pers["team"] == self.pers["team"] &&
               (players[i] getEntityNumber()) < (self getEntityNumber())) rank++;
        slot = rank % 4;
        if(!isDefined(faction)) faction = game["allies"];
        choices[0] = "mp40_mp";
        choices[1] = "kar98k_mp";
        choices[2] = "mp44_mp";
        choices[3] = "kar98k_sniper_mp";
        if(faction == "american")
        {
            choices[0] = "thompson_mp";
            choices[1] = "m1garand_mp";
            choices[2] = "bar_mp";
            choices[3] = "springfield_mp";
        }
        if(faction == "british")
        {
            choices[0] = "sten_mp";
            choices[1] = "enfield_mp";
            choices[2] = "bren_mp";
            choices[3] = "enfield_scope_mp";
        }
        if(faction == "russian")
        {
            choices[0] = "ppsh_mp";
            choices[1] = "mosin_nagant_mp";
            choices[2] = "pps42_mp";
            choices[3] = "mosin_nagant_sniper_mp";
        }
        weapon = choices[slot];
        self [[level.weapon]](weapon);
        self.pers["bot_faction"] = faction;
    }
    self closeMenu();
    self closeInGameMenu();
    self botAI(difficulty);
    self thread objectiveThink(difficulty);
    // Native commands use the normal use button to request a respawn. S&D and
    // HQ still decide whether that player may respawn; no forced spawn here.
}

objectiveState()
{
    level endon("intermission");
    for(;;)
    {
        level.bot_players = getEntArray("player", "classname");
        counts["allies"] = 0;
        counts["axis"] = 0;
        counts["none"] = 0;
        for(i = 0; i < level.bot_players.size; i++)
        {
            player = level.bot_players[i];
            if(!(player isBot()) || !isDefined(player.pers["team"]) || player.pers["team"] == "spectator")
                continue;
            team = player.pers["team"];
            if(!isDefined(counts[team])) counts[team] = 0;
            player.bot_rank = counts[team];
            counts[team]++;
        }
        level.bot_team_size = counts;
        if(getCvarInt("scr_bot_debug") && (!isDefined(level.bot_match_debug_at) || getTime() >= level.bot_match_debug_at))
        {
            level.bot_match_debug_at = getTime() + 5000;
            status = "mode=" + getCvar("g_gametype") + " allies=" + getTeamScore("allies") + " axis=" + getTeamScore("axis");
            if(isDefined(level.bombplanted)) status += " bombPlanted=" + level.bombplanted;
            if(isDefined(level.roundended)) status += " roundEnded=" + level.roundended;
            if(isDefined(level.radio))
                for(j = 0; j < level.radio.size; j++)
                    if(isDefined(level.radio[j].hidden) && !level.radio[j].hidden)
                        status += " radioTeam=" + level.radio[j].team;
            println("[bots] match " + status);
        }
        wait .5;
    }
}

objectiveThink(difficulty)
{
    self notify("bot_objectives_stop");
    self endon("bot_objectives_stop");
    self endon("disconnect");
    level endon("intermission");
    for(;;)
    {
        if(isAlive(self) && self.sessionstate == "playing" && isDefined(self.bot_rank))
        {
            mode = getCvar("g_gametype");
            if(mode == "ctf") self ctfObjective();
            else if(mode == "hq") self hqObjective();
            else if(mode == "sd") self sdObjective();
            else if(mode == "tdm") self tdmObjective();
            else self botClearGoal(); // DM: every other live player is hostile.
        }
        else self botClearGoal();
        if(difficulty == 0) wait .75;
        else if(difficulty == 2) wait .25;
        else wait .5;
    }
}

setObjective(point, radius, behavior, entity, intent)
{
    number = -1;
    if(isDefined(entity)) number = entity getEntityNumber();
    self botGoal(point, radius, behavior, number);
    self.bot_intent = intent;
    if(getCvarInt("scr_bot_debug") && (!isDefined(self.bot_debug_at) || getTime() >= self.bot_debug_at))
    {
        self.bot_debug_at = getTime() + 5000;
        println("[bots] objective client=" + (self getEntityNumber()) + " mode=" + getCvar("g_gametype") +
                " intent=" + intent + " point=" + point);
    }
}

// A role is based on the bot's rank within its team, not entity parity (which
// otherwise assigns the whole automatically-balanced team the same role).
selectActor(point, group, groups)
{
    actor = undefined;
    best = 1000000;
    for(i = 0; i < level.bot_players.size; i++)
    {
        player = level.bot_players[i];
        if(!isAlive(player) || player.sessionstate != "playing" ||
           !isDefined(player.pers["team"]) || player.pers["team"] != self.pers["team"]) continue;
        // Cover a human or bot already interacting instead of competing for
        // the trigger and resetting their original progress timer.
        if(isDefined(player.progressbackground) && distance(player.origin, point) < 256) return player;
        if(!(player isBot()) || !isDefined(player.bot_rank)) continue;
        if(groups > 1 && player.bot_rank % groups != group) continue;
        d = distance(player.origin, point);
        if(d < best)
        {
            best = d;
            actor = player;
        }
    }
    return actor;
}

ctfObjective()
{
    own = getEnt("allied_flag", "targetname");
    enemy = getEnt("axis_flag", "targetname");
    if(self.pers["team"] == "axis")
    {
        temp = own;
        own = enemy;
        enemy = temp;
    }
    if(!isDefined(own) || !isDefined(enemy) || !isDefined(own.home_origin))
    {
        self botClearGoal();
        return;
    }
    if(isDefined(self.flag))
    {
        self setObjective(own.home_origin, 12, 3, undefined, "return_enemy_flag");
        return;
    }
    friendlyCarrier = undefined;
    enemyCarrier = undefined;
    for(i = 0; i < level.bot_players.size; i++)
    {
        player = level.bot_players[i];
        if(!isAlive(player) || !isDefined(player.flag)) continue;
        if(player.flag == enemy) friendlyCarrier = player;
        if(player.flag == own) enemyCarrier = player;
    }
    if(!own.atbase && own.origin[2] > -5000)
    {
        actor = self selectActor(own.origin, 0, 1);
        if(isDefined(actor) && actor == self)
        {
            self setObjective(own.origin, 12, 3, undefined, "recover_own_flag");
            return;
        }
    }
    // Carriers are publicly marked on the original compass. This supplies a
    // movement objective, never permission to shoot an unseen player.
    if(isDefined(enemyCarrier) && (self.bot_rank % 3 == 0 || !isDefined(friendlyCarrier)))
    {
        self setObjective(enemyCarrier.origin, 96, 0, undefined, "intercept_flag_carrier");
        return;
    }
    if(isDefined(friendlyCarrier))
    {
        if(self.bot_rank % 3 != 0)
            self setObjective(friendlyCarrier.origin, 140, 1, undefined, "escort_flag_carrier");
        else
            self setObjective(own.home_origin, 160, 1, undefined, "defend_flag_base");
        return;
    }
    if(level.bot_team_size[self.pers["team"]] > 2 && self.bot_rank % 4 == 0 && own.atbase)
        self setObjective(own.home_origin, 160, 1, undefined, "defend_flag_base");
    else if(enemy.origin[2] > -5000)
        self setObjective(enemy.origin, 12, 0, undefined, "capture_enemy_flag");
    else self botClearGoal();
}

hqObjective()
{
    radio = undefined;
    if(isDefined(level.radio))
    {
        for(i = 0; i < level.radio.size; i++)
        {
            if(isDefined(level.radio[i].hidden) && !level.radio[i].hidden)
            {
                radio = level.radio[i];
                break;
            }
        }
    }
    // Do not know a future, unannounced radio location.
    if(!isDefined(radio))
    {
        self botClearGoal();
        return;
    }
    if(radio.team == self.pers["team"])
        self setObjective(radio.origin, 160, 1, undefined, "defend_headquarters");
    else if(radio.team == "none")
        self setObjective(radio.origin, 48, 0, undefined, "capture_headquarters");
    else
        self setObjective(radio.origin, 48, 0, undefined, "destroy_headquarters");
}

sdObjective()
{
    if(!isDefined(level.roundended) || level.roundended)
    {
        self botClearGoal();
        return;
    }
    if(level.bombplanted)
    {
        bomb = getEnt("bombtrigger", "targetname");
        if(!isDefined(bomb))
        {
            self botClearGoal();
            return;
        }
        if(self.pers["team"] == level.planting_team)
            self setObjective(bomb.origin, 180, 1, undefined, "defend_planted_bomb");
        else
        {
            actor = self selectActor(bomb.origin, 0, 1);
            // Preserve the current user throughout the original hold timer.
            if(isDefined(actor) && actor == self)
            {
                behavior = 0;
                radius = 12;
                if(self isTouching(bomb))
                {
                    behavior = 2;
                    radius = 512;
                }
                self setObjective(bomb.origin, radius, behavior, bomb, "defuse_bomb");
            }
            else self setObjective(bomb.origin, 180, 1, undefined, "cover_defuser");
        }
        return;
    }
    zones = getEntArray("bombzone", "targetname");
    valid = [];
    for(i = 0; i < zones.size; i++)
    {
        if(level.bombmode == 0 && isDefined(zones[i].script_bombmode_original)) valid[valid.size] = zones[i];
        if(level.bombmode == 1 && isDefined(zones[i].script_bombmode_single)) valid[valid.size] = zones[i];
        if(level.bombmode == 2 && isDefined(zones[i].script_bombmode_dual)) valid[valid.size] = zones[i];
    }
    if(!valid.size)
    {
        self botClearGoal();
        return;
    }
    group = self.bot_rank % valid.size;
    zone = valid[group];
    plantingTeam = game["attackers"];
    if(level.bombmode == 2 && isDefined(zone.script_team)) plantingTeam = zone.script_team;
    if(self.pers["team"] != plantingTeam)
    {
        self setObjective(zone.origin, 180, 1, undefined, "defend_bomb_site");
        return;
    }
    actor = self selectActor(zone.origin, group, valid.size);
    // A planting field is a boolean, not the player. Keep the closest planter
    // stationary so actor selection stays stable while the five seconds pass.
    if(isDefined(actor) && actor == self)
    {
        behavior = 0;
        radius = 12;
        if(self isTouching(zone))
        {
            behavior = 2;
            radius = 512;
        }
        self setObjective(zone.origin, radius, behavior, zone, "plant_bomb");
    }
    else self setObjective(zone.origin, 160, 1, undefined, "cover_planter");
}

tdmObjective()
{
    // Independent patrols give the team several lanes of attack. Combat and
    // last-seen enemy pursuit run natively; followers previously parked in a
    // pack around one wandering leader instead of searching the arena.
    self botClearGoal();
}
