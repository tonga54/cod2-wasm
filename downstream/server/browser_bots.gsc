// First-party bot lifecycle. Original gametype callbacks retain responsibility
// for factions, weapon restrictions, spawning, round rules and respawning.
main()
{
    level.bot_actor_cache = [];
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
    if(count > 32) count = 32;
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
    players = undefined;
    for(i = existing; i < count; i++)
    {
        // HQ restarts all team members after its original five-second count.
        // Admit the first pair to start that count, then populate incrementally
        // after it finishes instead of spawning the whole roster twice.
        if(i == 2 && getCvar("g_gametype") == "hq")
        {
            for(grace = 0; grace < 40; grace++)
            {
                if(isDefined(level.roundStarted) && level.roundStarted) break;
                wait .25;
            }
        }
        bot = addTestClient();
        if(isDefined(bot))
            bot thread joinGame(difficulty);
        bot = undefined;
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
        players = undefined;
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
        self.pers["bot_weapon_role"] = slot;
    }
    self closeMenu();
    self closeInGameMenu();
    self botAI(difficulty);
    self.bot_difficulty = difficulty;
    // Native commands use the normal use button to request a respawn. S&D and
    // HQ still decide whether that player may respawn; no forced spawn here.
}

objectiveState()
{
    level endon("intermission");
    phase = 0;
    nextRoster = 0;
    for(;;)
    {
        if(getTime() >= nextRoster)
        {
            nextRoster = getTime() + 500;
            level.bot_mode = getCvar("g_gametype");
            // Native DM/TDM patrols do not consume the scripted objective
            // roster. Avoid rebuilding arrays/ranks for every living bot.
            if(level.bot_mode == "tdm" || level.bot_mode == "dm")
            {
                if(getCvarInt("scr_bot_debug") && (!isDefined(level.bot_match_debug_at) || getTime() >= level.bot_match_debug_at))
                {
                    level.bot_match_debug_at = getTime() + 5000;
                    println("[bots] match mode=" + level.bot_mode + " allies=" + getTeamScore("allies") + " axis=" + getTeamScore("axis"));
                }
                wait .5;
                continue;
            }
            // Keep the shared cache independent of entity lifetimes across rounds.
            players = getEntArray("player", "classname");
            numbers = [];
            phases = [];
            for(j = 0; j < 5; j++) phases[j] = [];
            counts["allies"] = 0;
            counts["axis"] = 0;
            counts["none"] = 0;
            ranks["allies"] = 0;
            ranks["axis"] = 0;
            ranks["none"] = 0;
            for(i = 0; i < players.size; i++)
            {
                player = players[i];
                numbers[i] = player getEntityNumber();
                player.bot_rank = undefined;
                if(!(player isBot()) || !isDefined(player.pers["team"]) || player.pers["team"] == "spectator")
                    continue;
                // Death releases a duty, not every survivor's team identity.
                // Otherwise each casualty flips parity/site/defender roles and
                // sends living bots back across the map on the next refresh.
                team = player.pers["team"];
                bucket = numbers[i] % 5;
                phases[bucket][phases[bucket].size] = numbers[i];
                if(!isDefined(ranks[team])) ranks[team] = 0;
                player.bot_rank = ranks[team];
                ranks[team]++;
                // Only surviving players count toward current team strength.
                if(!isAlive(player) || player.sessionstate != "playing")
                {
                    player.bot_actor_until = 0;
                    continue;
                }
                if(!isDefined(counts[team])) counts[team] = 0;
                counts[team]++;
            }
            level.bot_player_numbers = numbers;
            level.bot_phases = phases;
            level.bot_team_size = counts;
            player = undefined;
            players = undefined;
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
        }
        // One shared scheduler replaces a sleeping policy and two endon
        // listeners per bot. Numbers and deadlines carry no entity references.
        carriers = undefined;
        for(i = 0; i < level.bot_phases[phase].size; i++)
        {
            player = botPlayer(level.bot_phases[phase][i]);
            if(!isDefined(player) || !isDefined(player.bot_difficulty)) continue;
            if(!isDefined(player.bot_objective_next) || getTime() >= player.bot_objective_next)
            {
                // No yield between decisions: share one current CTF snapshot
                // for this phase, then release all entity references below.
                if(level.bot_mode == "ctf" && !isDefined(carriers)) carriers = ctfCarriers();
                player objectiveThink(carriers);
                interval = 500;
                if(player.bot_difficulty == 0) interval = 750;
                else if(player.bot_difficulty == 2) interval = 250;
                player.bot_objective_next = getTime() + interval;
            }
        }
        player = undefined;
        carriers = undefined;
        phase = (phase + 1) % 5;
        wait .05;
    }
}

objectiveThink(carriers)
{
    if(isAlive(self) && self.sessionstate == "playing" && isDefined(self.bot_rank))
    {
        mode = level.bot_mode;
        if(mode == "ctf") self ctfObjective(carriers);
        else if(mode == "hq") self hqObjective();
        else if(mode == "sd") self sdObjective();
        else if(mode == "tdm") self tdmObjective();
        else self botClearGoal(); // DM: every other live player is hostile.
    }
    else self botClearGoal();
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
selectActor(point, group, groups, duty)
{
    // Share elections across the team. A 32-bot objective must not rescan the
    // entire roster independently for every covering bot's decision.
    key = self.pers["team"] + "_" + duty + "_" + groups + "_" + group;
    if(!isDefined(level.bot_actor_cache[key]))
        level.bot_actor_cache[key] = spawnStruct();
    cache = level.bot_actor_cache[key];
    if(isDefined(cache.until) && getTime() < cache.until && distance(cache.point, point) < 64)
    {
        if(cache.number < 0) return undefined;
        cached = botPlayer(cache.number);
        if(isDefined(cached) && isAlive(cached) && cached.sessionstate == "playing" &&
           isDefined(cached.pers["team"]) && cached.pers["team"] == self.pers["team"] &&
           !isDefined(cached.flag) && (!(cached isBot()) || (isDefined(cached.bot_rank) &&
           (groups <= 1 || cached.bot_rank % groups == group))) &&
           !(duty == "attack_flag" && isDefined(cached.bot_actor_duty) && cached.bot_actor_duty == "recover" &&
           cached.bot_actor_until > getTime())) return cached;
    }
    // At most once per shared election, replace an approach that has made no
    // progress for twelve seconds. Never interrupt an original use timer or
    // an actor already at the trigger. Scalar state survives no entity refs.
    if(isDefined(cache.number) && cache.number >= 0 && distance(cache.point, point) < 64)
    {
        previous = botPlayer(cache.number);
        if(isDefined(previous) && isAlive(previous))
        {
            remaining = distance(previous.origin, point);
            // XY proximity below a balcony is not arrival at the trigger.
            // Keep the lease near the correct floor or during original use;
            // otherwise the normal stall handoff must remain possible.
            nearTrigger = remaining < 192 && previous.origin[2] > point[2] - 64 && previous.origin[2] < point[2] + 64;
            if(!isDefined(cache.remaining) || remaining < cache.remaining - 48 ||
               nearTrigger || isDefined(previous.progressbackground))
            {
                cache.remaining = remaining;
                cache.progressAt = getTime();
            }
            else if(getTime() - cache.progressAt > 12000)
            {
                cache.retryNumber = cache.number;
                cache.retryUntil = getTime() + 8000;
                previous.bot_actor_until = 0;
            }
        }
    }
    actor = undefined;
    best = 1000000;
    leaseActor = undefined;
    retryActor = undefined;
    for(i = 0; i < level.bot_player_numbers.size; i++)
    {
        player = botPlayer(level.bot_player_numbers[i]);
        if(!isDefined(player) || !isAlive(player) || player.sessionstate != "playing" ||
           !isDefined(player.pers["team"]) || player.pers["team"] != self.pers["team"] ||
           isDefined(player.flag)) continue;
        // Cover a human or bot already interacting instead of competing for
        // the trigger and resetting their original progress timer.
        if(isDefined(player.progressbackground) && distance(player.origin, point) < 256)
        {
            actor = player;
            break;
        }
        if(!(player isBot()) || !isDefined(player.bot_rank)) continue;
        if(groups > 1 && player.bot_rank % groups != group) continue;
        if(duty == "attack_flag" && isDefined(player.bot_actor_duty) && player.bot_actor_duty == "recover" &&
           player.bot_actor_until > getTime()) continue;
        if(isDefined(cache.retryUntil) && getTime() < cache.retryUntil &&
           (player getEntityNumber()) == cache.retryNumber)
        {
            retryActor = player;
            continue;
        }
        if(isDefined(player.bot_actor_until) && getTime() < player.bot_actor_until &&
           isDefined(player.bot_actor_duty) && player.bot_actor_duty == duty &&
           player.bot_actor_group == group && player.bot_actor_groups == groups &&
           distance(player.bot_actor_point, point) < 64) leaseActor = player;
        d = distance(player.origin, point);
        // Keep scoped support on overwatch when a similarly close assault
        // bot can interact; a sniper can still take over when needed.
        if(isDefined(player.pers["bot_weapon_role"]) && player.pers["bot_weapon_role"] == 3) d += 192;
        if(d < best)
        {
            best = d;
            actor = player;
        }
    }
    // A sole surviving bot keeps trying; a handoff requires a replacement.
    if(!isDefined(actor) && isDefined(retryActor))
    {
        actor = retryActor;
        cache.progressAt = getTime();
    }
    // Keep one actor approaching the trigger. Reelecting the nearest on every
    // update made teammates exchange jobs at corners before either arrived.
    if(isDefined(leaseActor) && (!isDefined(actor) || !isDefined(actor.progressbackground))) actor = leaseActor;
    else if(isDefined(actor) && (actor isBot()))
    {
        // Store only scalars/vectors on the actor. No cross-entity references
        // or per-decision structs survive a round teardown.
        actor.bot_actor_point = point;
        actor.bot_actor_group = group;
        actor.bot_actor_groups = groups;
        actor.bot_actor_duty = duty;
        actor.bot_actor_until = getTime() + 2500;
    }
    number = -1;
    if(isDefined(actor)) number = actor getEntityNumber();
    if(!isDefined(cache.number) || cache.number != number || distance(cache.point, point) >= 64)
    {
        cache.progressAt = getTime();
        cache.remaining = 1000000;
        if(isDefined(actor)) cache.remaining = distance(actor.origin, point);
    }
    cache.number = number;
    cache.point = point;
    cache.until = getTime() + 500;
    return actor;
}

ctfCarriers()
{
    state = spawnStruct();
    state.allied = getEnt("allied_flag", "targetname");
    state.axis = getEnt("axis_flag", "targetname");
    for(i = 0; i < level.bot_player_numbers.size; i++)
    {
        player = botPlayer(level.bot_player_numbers[i]);
        if(!isDefined(player) || !isAlive(player) || !isDefined(player.flag)) continue;
        if(isDefined(state.allied) && player.flag == state.allied) state.alliedCarrier = player;
        if(isDefined(state.axis) && player.flag == state.axis) state.axisCarrier = player;
    }
    return state;
}

ctfObjective(carriers)
{
    if(!isDefined(carriers)) carriers = ctfCarriers();
    own = carriers.allied;
    enemy = carriers.axis;
    friendlyCarrier = carriers.axisCarrier;
    enemyCarrier = carriers.alliedCarrier;
    if(self.pers["team"] == "axis")
    {
        temp = own;
        own = enemy;
        enemy = temp;
        friendlyCarrier = carriers.alliedCarrier;
        enemyCarrier = carriers.axisCarrier;
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
    if(!own.atbase && own.origin[2] > -5000)
    {
        actor = self selectActor(own.origin, 0, 1, "recover");
        if(isDefined(actor) && actor == self)
        {
            self setObjective(own.origin, 12, 3, undefined, "recover_own_flag");
            return;
        }
    }
    // Carriers are publicly marked on the original compass. This supplies a
    // movement objective, never permission to shoot an unseen player.
    if(isDefined(enemyCarrier))
    {
        // Elect one living responder per stable squad. Fixed ranks 0/1 left
        // the carrier unchallenged after those two teammates died.
        actor = self selectActor(enemyCarrier.origin, self.bot_rank % 2, 2, "intercept");
        if(isDefined(actor) && actor == self)
        {
            self setObjective(enemyCarrier.origin, 96, 0, undefined, "intercept_flag_carrier");
            return;
        }
    }
    if(isDefined(friendlyCarrier))
    {
        actor = self selectActor(friendlyCarrier.origin, self.bot_rank % 2, 2, "escort");
        if(isDefined(actor) && actor == self)
            self setObjective(friendlyCarrier.origin, 140, 1, undefined, "escort_flag_carrier");
        else if(self.bot_rank % 4 == 2)
            self setObjective(own.home_origin, 160, 1, undefined, "defend_flag_base");
        else self botClearGoal(); // Scout the return lanes instead of a carrier pile-up.
        return;
    }
    actor = undefined;
    if(enemy.origin[2] > -5000) actor = self selectActor(enemy.origin, 0, 1, "attack_flag");
    if(isDefined(actor) && actor == self)
        self setObjective(enemy.origin, 12, 0, undefined, "capture_enemy_flag");
    else if(level.bot_team_size[self.pers["team"]] > 2 && self.bot_rank % 4 == 0 && own.atbase)
        self setObjective(own.home_origin, 160, 1, undefined, "defend_flag_base");
    else if(enemy.origin[2] > -5000)
        self setObjective(enemy.origin, 160, 1, undefined, "cover_flag_attack");
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
    size = level.bot_team_size[self.pers["team"]];
    // Large teams need mobile infantry as well as a fixed defense. Support
    // weapons and snipers cover the radio; rifle/SMG patrols contest approaches
    // and respond to observed contacts instead of all parking at one point.
    mobile = size > 4 && self.bot_rank % 4 < 2;
    if(radio.team == self.pers["team"])
    {
        if(mobile) self botClearGoal();
        else self setObjective(radio.origin, 160, 1, undefined, "defend_headquarters");
    }
    else
    {
        // Elect capture/destroy actors before assigning mobile duties. Even
        // mobile infantry must perform an interaction when selected for it.
        actor = undefined;
        if(size > 2) actor = self selectActor(radio.origin, self.bot_rank % 2, 2, "radio");
        if(size > 2 && (!isDefined(actor) || actor != self))
        {
            if(mobile) self botClearGoal();
            else self setObjective(radio.origin, 144, 1, undefined, "cover_headquarters_capture");
        }
        else if(radio.team == "none")
            self setObjective(radio.origin, 48, 0, undefined, "capture_headquarters");
        else self setObjective(radio.origin, 48, 0, undefined, "destroy_headquarters");
    }
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
            actor = self selectActor(bomb.origin, 0, 1, "defuse");
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
    actor = self selectActor(zone.origin, group, valid.size, "plant");
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
