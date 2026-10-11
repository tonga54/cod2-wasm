#!/usr/bin/env python3
"""Arrange deaths; let the original mode and normal bot input handle respawns."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = (root / 'downstream/server/browser_bots.gsc').read_text().replace(
    '    level thread objectiveState();',
    '    level thread objectiveState();\n    level thread qaRespawns();', 1)
source = source.replace('    self botAI(difficulty);\n    self.bot_difficulty = difficulty;',
                        '    self botStop();\n    self disableWeapon();\n    self.bot_difficulty = undefined;')
scenario = r'''
qaNoWeapons()
{
    self endon("disconnect");
    level endon("intermission");
    for(;;)
    {
        if(isAlive(self)) self disableWeapon();
        wait .1;
    }
}

qaRespawns()
{
    level endon("intermission");
    wait 12;
    mode = getCvar("g_gametype");
    if(mode == "sd")
        while(!level.roundstarted && !level.roundended) wait .25;
    players = getEntArray("player", "classname");
    actor = undefined;
    for(i = 0; i < players.size; i++)
    {
        if(!(players[i] isBot()) || !isAlive(players[i]) || players[i].sessionstate != "playing") continue;
        players[i] botStop();
        players[i] thread qaNoWeapons();
        if(!isDefined(actor) && players[i].pers["bot_weapon_role"] != 3) actor = players[i];
    }
    if(!isDefined(actor))
    {
        println("[bot-qa] FAIL no_respawn_actor");
        return;
    }
    actor botAI(1);
    if(mode == "sd")
    {
        friends = 0;
        enemies = 0;
        for(i = 0; i < players.size; i++)
        {
            if(!isAlive(players[i]) || players[i].sessionstate != "playing") continue;
            if(players[i].pers["team"] == actor.pers["team"]) friends++;
            else enemies++;
        }
        if(friends < 2 || enemies < 1 || level.roundended)
        {
            println("[bot-qa] FAIL sd_respawn_setup friends=" + friends + " enemies=" + enemies);
            return;
        }
    }
    for(cycle = 0; cycle < 2; cycle++)
    {
        actor suicide();
        started = getTime();
        wait .1;
        if(mode == "sd")
        {
            // Teammates remain alive. Pressing use must not bypass the round.
            wait 8;
            if((!isAlive(actor) || actor.sessionstate != "playing") && !level.roundended)
                println("[bot-qa] PASS sd_no_midround_respawn");
            else println("[bot-qa] FAIL sd_midround_respawn");
            return;
        }
        for(t = 0; t < 180 && (!isAlive(actor) || actor.sessionstate != "playing"); t++) wait .25;
        if(!isAlive(actor) || actor.sessionstate != "playing")
        {
            println("[bot-qa] FAIL original_respawn mode=" + mode + " cycle=" + cycle + " state=" + actor.sessionstate);
            return;
        }
        println("[bot-qa] PASS original_respawn mode=" + mode + " cycle=" + cycle + " ms=" + (getTime()-started));
        // Fixtures leave objective policy disabled, so every respawn must
        // resume its native patrol without explicitly enabling AI again.
        {
            origin = actor.origin;
            for(t = 0; t < 80 && distance(actor.origin, origin) < 96; t++) wait .25;
            if(distance(actor.origin, origin) >= 96)
                println("[bot-qa] PASS resumed_patrol mode=" + mode + " cycle=" + cycle);
            else
            {
                println("[bot-qa] FAIL stationary_respawn mode=" + mode + " cycle=" + cycle);
                return;
            }
        }
        wait 1;
    }
}
'''
output = root / 'out/bot-respawns-controlled.gsc'
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(source + scenario)
# Exercise use-to-respawn, including the spectator waiting phase in CTF/HQ.
(root / 'out/bot-respawns.cfg').write_text((root / 'server.cfg').read_text() + '\nset scr_forcerespawn "0"\n')
print(output)
