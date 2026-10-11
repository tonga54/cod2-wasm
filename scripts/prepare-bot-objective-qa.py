#!/usr/bin/env python3
"""Generate first-party controlled objective scenarios for an isolated native server."""
from pathlib import Path
import argparse
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--carrier-roles', action='store_true', help='CTF casualty/replacement scenarios (12 bots)')
args = parser.parse_args()
root=Path(__file__).resolve().parents[1]
source=(root/'downstream/server/browser_bots.gsc').read_text().replace(
    '    level thread objectiveState();','    level thread objectiveState();\n    level thread qaScenario();',1)
source=source.replace('    self botAI(difficulty);\n    self.bot_difficulty = difficulty;', '    self disableWeapon();\n    self botStop();\n    self.bot_difficulty = undefined;')
scenario=r'''
qaScenario()
{
    level endon("intermission");
    wait 12;
    players = getEntArray("player", "classname");
    actor = undefined;
    other = undefined;
    mode = getCvar("g_gametype");
    team = "allies";
    if(mode == "sd") team = game["attackers"];
    for(i = 0; i < players.size; i++)
    {
        if(!(players[i] isBot()) || !isAlive(players[i])) continue;
        players[i].bot_difficulty = undefined;
        players[i] botStop();
        players[i] disableWeapon();
        if(players[i].pers["team"] == team && !isDefined(actor)) actor = players[i];
        if(players[i].pers["team"] != team && !isDefined(other)) other = players[i];
    }
    if(!isDefined(actor) || !isDefined(other))
    {
        println("[bot-qa] FAIL actors_not_alive mode=" + mode);
        return;
    }
    if(mode == "ctf")
    {
        if(CARRIER_ROLES)
        {
            qaCarrierRoles(players, other);
            return;
        }
        selfTest = actor qaStableRoles();
        actor qaActorElection();
        actor qaActorBelowObjective();
        own = getEnt("allied_flag", "targetname");
        enemy = getEnt("axis_flag", "targetname");
        before = getTeamScore(team);
        actor enableWeapon();
        actor botAI(1);
        actor.bot_difficulty = 1;
        actor setOrigin(enemy.origin + (0,0,4));
        wait 2;
        if(!isDefined(actor.flag))
        {
            println("[bot-qa] FAIL flag_pickup");
            return;
        }
        println("[bot-qa] PASS original_flag_pickup");
        actor setOrigin(own.home_origin + (0,0,4));
        wait 2;
        if(getTeamScore(team) == before + 1 && !isDefined(actor.flag))
            println("[bot-qa] PASS original_flag_capture");
        else println("[bot-qa] FAIL flag_capture ownAtBase=" + own.atbase + " actor=" + actor.origin + " home=" + own.home_origin);
    }
    if(mode == "hq")
    {
        radio = undefined;
        for(t = 0; t < 100 && !isDefined(radio); t++)
        {
            for(i = 0; i < level.radio.size; i++)
                if(!level.radio[i].hidden) radio = level.radio[i];
            wait .5;
        }
        if(!isDefined(radio))
        {
            println("[bot-qa] FAIL radio_not_spawned");
            return;
        }
        park = players[0].origin;
        best = 0;
        for(i = 0; i < players.size; i++)
            if(distance(players[i].origin,radio.origin) > best)
            {
                best = distance(players[i].origin,radio.origin);
                park = players[i].origin;
            }
        for(i = 0; i < players.size; i++) players[i] setOrigin(park);
        actor enableWeapon();
        actor botAI(1);
        actor.bot_difficulty = 1;
        actor setOrigin(radio.origin + (0,0,4));
        started = getTime();
        for(t = 0; t < 180 && radio.team != team; t++) wait .5;
        if(radio.team != team)
        {
            println("[bot-qa] FAIL radio_capture");
            return;
        }
        println("[bot-qa] PASS original_hq_capture ms=" + (getTime()-started));
        actor.bot_difficulty = undefined;
        actor suicide();
        wait 8;
        if(!isAlive(actor) || actor.sessionstate != "playing")
            println("[bot-qa] PASS hq_defender_waits_for_neutralize");
        else
        {
            println("[bot-qa] FAIL hq_defender_early_respawn");
            return;
        }
        other enableWeapon();
        other botAI(1);
        other.bot_difficulty = 1;
        other setOrigin(radio.origin + (0,0,4));
        started = getTime();
        for(t = 0; t < 180 && !radio.hidden; t++) wait .5;
        if(radio.hidden && radio.team == "none")
        {
            println("[bot-qa] PASS original_hq_destroy ms=" + (getTime()-started));
            for(t = 0; t < 120 && (!isAlive(actor) || actor.sessionstate != "playing"); t++) wait .25;
            if(isAlive(actor) && actor.sessionstate == "playing")
                println("[bot-qa] PASS hq_defender_respawns_after_neutralize");
            else println("[bot-qa] FAIL hq_defender_missing_respawn");
        }
        else println("[bot-qa] FAIL radio_destroy");
    }
    if(mode == "sd")
    {
        zones = getEntArray("bombzone", "targetname");
        zone = zones[actor.bot_rank % zones.size];
        park = other.origin;
        for(i = 0; i < players.size; i++)
            if(distance(players[i].origin, zone.origin) > distance(park, zone.origin)) park = players[i].origin;
        for(i = 0; i < players.size; i++) players[i] setOrigin(park);
        if(!(actor qaPlaceAtTrigger(zone)))
        {
            println("[bot-qa] FAIL planting_setup");
            return;
        }
        actor enableWeapon();
        actor botAI(1);
        actor.bot_difficulty = 1;
        started = getTime();
        for(t = 0; t < 60 && !level.bombplanted; t++) wait .5;
        if(!level.bombplanted)
        {
            println("[bot-qa] FAIL bomb_plant");
            return;
        }
        elapsed = getTime()-started;
        if(elapsed < 5000) println("[bot-qa] FAIL shortened_plant_timer");
        else println("[bot-qa] PASS original_bomb_plant ms=" + elapsed);
        actor.bot_difficulty = undefined;
        actor botStop();
        actor setOrigin(park);
        bomb = getEnt("bombtrigger", "targetname");
        if(!(other qaPlaceAtTrigger(bomb)))
        {
            println("[bot-qa] FAIL defusing_setup");
            return;
        }
        other enableWeapon();
        other botAI(1);
        other.bot_difficulty = 1;
        started = getTime();
        for(t = 0; t < 80 && isDefined(bomb); t++) wait .5;
        elapsed = getTime()-started;
        if(isDefined(bomb)) println("[bot-qa] FAIL bomb_defuse");
        else if(elapsed < 10000) println("[bot-qa] FAIL shortened_defuse_timer");
        else println("[bot-qa] PASS original_bomb_defuse ms=" + elapsed);
    }
}

qaStableRoles()
{
    players = getEntArray("player", "classname");
    victim = undefined;
    survivor = undefined;
    for(i = 0; i < players.size; i++)
    {
        player = players[i];
        if(player == self || !(player isBot()) || !isAlive(player) ||
           player.pers["team"] != self.pers["team"]) continue;
        if(!isDefined(victim)) victim = player;
        else survivor = player;
    }
    if(!isDefined(survivor))
    {
        println("[bot-qa] FAIL stable_roles_setup");
        return false;
    }
    rank = survivor.bot_rank;
    victim suicide();
    wait 1.2;
    if(survivor.bot_rank == rank)
        println("[bot-qa] PASS survivor_role_stable_after_teammate_death");
    else println("[bot-qa] FAIL survivor_role_changed_after_teammate_death");
    return true;
}

qaCarrierRoles(players, enemyCarrier)
{
    own = getEnt("allied_flag", "targetname");
    enemy = getEnt("axis_flag", "targetname");
    // Stop low ranks after death so they cannot return during the election.
    for(i = 0; i < players.size; i++)
        if(players[i].pers["team"] == "allies" && players[i].bot_rank < 2)
            players[i] suicide();
    wait 1.2;
    if(level.bot_team_size["allies"] < 4)
    {
        println("[bot-qa] FAIL carrier_roles_require_12_bots");
        return;
    }
    enemyCarrier setOrigin(own.origin + (0,0,4));
    wait 1;
    if(!isDefined(enemyCarrier.flag))
    {
        println("[bot-qa] FAIL carrier_roles_flag_pickup");
        return;
    }
    for(phase = 0; phase < 2; phase++)
    {
        count = 0;
        chosen = undefined;
        for(i = 0; i < players.size; i++)
        {
            player = players[i];
            if(player.pers["team"] != "allies" || !isAlive(player) || player.sessionstate != "playing") continue;
            player botAI(1);
            player ctfObjective();
            if(player.bot_intent == "intercept_flag_carrier")
            {
                count++;
                chosen = player;
            }
            player botStop();
        }
        if(count != 2)
        {
            println("[bot-qa] FAIL surviving_interceptors phase=" + phase + " count=" + count);
            return;
        }
        println("[bot-qa] PASS surviving_interceptors phase=" + phase + " count=" + count);
        if(!phase)
        {
            chosen suicide();
            wait 1.2;
        }
    }
    // Keep the enemy carrier alive; the two teams can carry flags at once.
    // Recovering the enemy flag should still receive a surviving escort.
    carrier = undefined;
    for(i = 0; i < players.size; i++)
        if(players[i].pers["team"] == "allies" && isAlive(players[i]) && !isDefined(carrier)) carrier = players[i];
    carrier setOrigin(enemy.origin + (0,0,4));
    wait 1;
    if(!isDefined(carrier.flag))
    {
        println("[bot-qa] FAIL escort_flag_pickup");
        return;
    }
    // End interception first: deliver our own flag back through original rules.
    enemyCarrier setOrigin(enemy.home_origin + (0,0,4));
    // Both flags away prevents a capture; instead drop and let a teammate return ours.
    enemyCarrier suicide();
    wait .5;
    helper = undefined;
    for(i = 0; i < players.size; i++)
        if(players[i] != carrier && players[i].pers["team"] == "allies" && isAlive(players[i])) helper = players[i];
    helper setOrigin(own.origin + (0,0,4));
    wait 1;
    count = 0;
    for(i = 0; i < players.size; i++)
    {
        player = players[i];
        if(player == carrier || player.pers["team"] != "allies" || !isAlive(player)) continue;
        player botAI(1);
        player ctfObjective();
        if(player.bot_intent == "escort_flag_carrier") count++;
        player botStop();
    }
    if(count > 0 && count <= 2)
        println("[bot-qa] PASS surviving_escorts count=" + count);
    else println("[bot-qa] FAIL surviving_escorts count=" + count);
}

qaActorElection()
{
    point = self.origin + (10000,0,0);
    duty = "qa_approach";
    first = self selectActor(point, 0, 1, duty);
    again = self selectActor(point, 0, 1, duty);
    if(!isDefined(first) || first != again)
    {
        println("[bot-qa] FAIL shared_actor_election");
        return;
    }
    key = self.pers["team"] + "_" + duty + "_1_0";
    cache = level.bot_actor_cache[key];
    cache.until = 0;
    cache.remaining = distance(first.origin, point);
    cache.progressAt = getTime() - 13000;
    first.bot_actor_until = getTime() + 2500;
    replacement = self selectActor(point, 0, 1, duty);
    if(!isDefined(replacement) || replacement == first)
        println("[bot-qa] FAIL stalled_actor_handoff");
    else
    {
        cache.until = 0;
        cache.remaining = distance(replacement.origin, point) + 64;
        cache.progressAt = getTime() - 13000;
        advancing = self selectActor(point, 0, 1, duty);
        if(advancing == replacement && cache.progressAt == getTime())
            println("[bot-qa] PASS shared_actor_cache_stall_handoff_progress_lease");
        else println("[bot-qa] FAIL advancing_actor_lease");
    }
    first.bot_actor_until = 0;
    if(isDefined(replacement)) replacement.bot_actor_until = 0;
    roster = level.bot_player_numbers;
    level.bot_player_numbers = [];
    level.bot_player_numbers[0] = first getEntityNumber();
    cache.until = 0;
    cache.number = first getEntityNumber();
    cache.remaining = distance(first.origin, point);
    cache.progressAt = getTime() - 13000;
    sole = self selectActor(point, 0, 1, duty);
    level.bot_player_numbers = roster;
    if(isDefined(sole) && sole == first)
        println("[bot-qa] PASS sole_actor_keeps_objective");
    else println("[bot-qa] FAIL sole_actor_lost_objective");
    first.bot_actor_until = 0;
}

qaActorBelowObjective()
{
    duty = "qa_upper_floor";
    first = self selectActor(self.origin, 0, 1, duty);
    point = first.origin + (0,0,180);
    key = self.pers["team"] + "_" + duty + "_1_0";
    cache = level.bot_actor_cache[key];
    cache.point = point;
    cache.until = 0;
    cache.remaining = 180;
    cache.progressAt = getTime() - 13000;
    first.bot_actor_point = point;
    first.bot_actor_until = getTime() + 2500;
    replacement = self selectActor(point, 0, 1, duty);
    if(isDefined(replacement) && replacement != first)
        println("[bot-qa] PASS stalled_actor_below_objective_handoff");
    else println("[bot-qa] FAIL stalled_actor_below_objective_retained");
    first.bot_actor_until = 0;
    if(isDefined(replacement)) replacement.bot_actor_until = 0;
}

qaPlaceAtTrigger(trigger)
{
    offsets[0] = (0,0,64);
    offsets[1] = (32,0,64);
    offsets[2] = (-32,0,64);
    offsets[3] = (0,32,64);
    offsets[4] = (0,-32,64);
    offsets[5] = (64,0,64);
    offsets[6] = (-64,0,64);
    offsets[7] = (128,0,64);
    offsets[8] = (-128,0,64);
    offsets[9] = (0,128,64);
    offsets[10] = (0,-128,64);
    // Sloped/brush-bound sites such as Dawnville need a grounded point inside
    // their volume; the model origin alone may be on a steep rubble face.
    offsets[11] = (64,64,64);
    offsets[12] = (64,-64,64);
    offsets[13] = (-64,64,64);
    offsets[14] = (-64,-64,64);
    offsets[15] = (96,32,64);
    offsets[16] = (-96,32,64);
    offsets[17] = (32,96,64);
    offsets[18] = (32,-96,64);
    for(i = 0; i < offsets.size; i++)
    {
        at = trigger.origin + offsets[i] - (0,0,48);
        floor = bulletTrace(at, at - (0,0,160), false, self);
        // The original player setOrigin method raises the supplied Z by one.
        // Compensate when arranging this test directly on the traced floor.
        self setOrigin(floor["position"] - (0,0,1));
        wait 1;
        println("[bot-qa] placement attempt=" + i + " origin=" + self.origin + " zone=" + trigger.origin + " trace=" + floor["position"] + " ground=" + (self isOnGround()) + " touching=" + (self isTouching(trigger)));
        if((self isOnGround()) && (self isTouching(trigger))) return true;
    }
    return false;
}
'''
scenario=scenario.replace('CARRIER_ROLES', 'true' if args.carrier_roles else 'false')
output=root/('out/bot-carriers-controlled.gsc' if args.carrier_roles else 'out/bot-objectives-controlled.gsc')
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(source+scenario)
print(output)
