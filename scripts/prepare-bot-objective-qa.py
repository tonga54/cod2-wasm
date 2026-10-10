#!/usr/bin/env python3
"""Generate first-party controlled objective scenarios for an isolated native server."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=(root/'downstream/server/browser_bots.gsc').read_text().replace(
    '    level thread objectiveState();','    level thread objectiveState();\n    level thread qaScenario();',1)
source=source.replace('    self botAI(difficulty);\n    self thread objectiveThink(difficulty);', '    self disableWeapon();\n    self botStop();')
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
        players[i] notify("bot_objectives_stop");
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
        own = getEnt("allied_flag", "targetname");
        enemy = getEnt("axis_flag", "targetname");
        before = getTeamScore(team);
        actor enableWeapon();
        actor botAI(1);
        actor thread objectiveThink(1);
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
        actor thread objectiveThink(1);
        actor setOrigin(radio.origin + (0,0,4));
        started = getTime();
        for(t = 0; t < 180 && radio.team != team; t++) wait .5;
        if(radio.team != team)
        {
            println("[bot-qa] FAIL radio_capture");
            return;
        }
        println("[bot-qa] PASS original_hq_capture ms=" + (getTime()-started));
        actor notify("bot_objectives_stop");
        actor botStop();
        actor setOrigin(park);
        other enableWeapon();
        other botAI(1);
        other thread objectiveThink(1);
        other setOrigin(radio.origin + (0,0,4));
        started = getTime();
        for(t = 0; t < 180 && !radio.hidden; t++) wait .5;
        if(radio.hidden && radio.team == "none")
            println("[bot-qa] PASS original_hq_destroy ms=" + (getTime()-started));
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
        actor thread objectiveThink(1);
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
        actor notify("bot_objectives_stop");
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
        other thread objectiveThink(1);
        started = getTime();
        for(t = 0; t < 80 && isDefined(bomb); t++) wait .5;
        elapsed = getTime()-started;
        if(isDefined(bomb)) println("[bot-qa] FAIL bomb_defuse");
        else if(elapsed < 10000) println("[bot-qa] FAIL shortened_defuse_timer");
        else println("[bot-qa] PASS original_bomb_defuse ms=" + elapsed);
    }
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
output=root/'out/bot-objectives-controlled.gsc'
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(source+scenario)
print(output)
