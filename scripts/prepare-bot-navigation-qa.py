#!/usr/bin/env python3
"""Generate a first-party, isolated Toujane ladder round-trip scenario."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = (root / 'downstream/server/browser_bots.gsc').read_text().replace(
    '    level thread objectiveState();',
    '    level thread objectiveState();\n    level thread qaNavigation();', 1)
source = source.replace(
    '    self botAI(difficulty);\n    self thread objectiveThink(difficulty);',
    '    self disableWeapon();\n    self botStop();')
scenario = r'''
qaNavigation()
{
    level endon("intermission");
    wait 12;
    if(getCvar("mapname") != "mp_toujane")
    {
        println("[bot-nav-qa] FAIL scenario_requires_toujane");
        return;
    }
    players = getEntArray("player", "classname");
    actor = undefined;
    for(i = 0; i < players.size; i++)
    {
        if(!(players[i] isBot()) || !isAlive(players[i])) continue;
        players[i] notify("bot_objectives_stop");
        players[i] botStop();
        players[i] disableWeapon();
        if(!isDefined(actor)) actor = players[i];
    }
    if(!isDefined(actor))
    {
        println("[bot-nav-qa] FAIL no_actor");
        return;
    }
    // Collision-verified endpoints of the original ladder. Teleport only to
    // arrange the initial fixture; both tested journeys use normal bot input.
    bottom = (320,928,12);
    top = (352,928,152);
    actor setOrigin(bottom - (0,0,1));
    wait 1;
    actor botAI(1);
    for(phase = 0; phase < 2; phase++)
    {
        goal = top;
        if(phase) goal = bottom;
        reached = false;
        for(t = 0; t < 120; t++)
        {
            actor botGoal(goal,12,3,-1);
            wait .5;
            if(distance(actor.origin,goal) < 24 && actor isOnGround())
            {
                reached = true;
                break;
            }
        }
        if(!reached)
        {
            println("[bot-nav-qa] FAIL ladder_phase=" + phase + " position=" + actor.origin);
            return;
        }
        println("[bot-nav-qa] PASS ladder_phase=" + phase + " position=" + actor.origin);
    }
    actor botStop();
}
'''
output = root / 'out/bot-navigation-controlled.gsc'
output.parent.mkdir(exist_ok=True)
output.write_text(source + scenario)
print(f'Generated first-party navigation scenario: {output}')
