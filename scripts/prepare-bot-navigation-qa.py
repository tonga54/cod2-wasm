#!/usr/bin/env python3
"""Generate isolated, first-party ladder and elevated-spawn escape scenarios."""
import argparse
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--roof-exits", action="store_true", help="Also exercise complex elevated escape routes on Toujane and Carentan")
args = parser.parse_args()

root = Path(__file__).resolve().parents[1]
source = (root / 'downstream/server/browser_bots.gsc').read_text().replace(
    '    level thread objectiveState();',
    '    level thread objectiveState();\n    level thread qaNavigation();', 1)
source = source.replace(
    '    self botAI(difficulty);\n    self.bot_difficulty = difficulty;',
    '    self disableWeapon();\n    self botStop();\n    self.bot_difficulty = undefined;')
scenario = r'''
qaNavigation()
{
    level endon("intermission");
    wait 12;
    map = getCvar("mapname");
    if(map != "mp_toujane" && (map != "mp_carentan" || !ROOF_EXITS))
    {
        println("[bot-nav-qa] FAIL scenario_requires_toujane_or_carentan");
        return;
    }
    players = getEntArray("player", "classname");
    actor = undefined;
    for(i = 0; i < players.size; i++)
    {
        if(!(players[i] isBot()) || !isAlive(players[i])) continue;
        players[i].bot_difficulty = undefined;
        players[i] botStop();
        players[i] disableWeapon();
        if(!isDefined(actor)) actor = players[i];
    }
    if(!isDefined(actor))
    {
        println("[bot-nav-qa] FAIL no_actor");
        return;
    }
    // Stopped test clients still have solid bodies. Park them at distinct
    // authored spawn points far from the test route during fixture setup.
    start = (1327,1760,144);
    finish = (1708,2191,-22);
    if(map == "mp_toujane")
    {
        start = (2630,880,182);
        finish = (2263,2480,72);
    }
    spawns = getEntArray("mp_tdm_spawn", "classname");
    used = [];
    for(i = 0; i < players.size; i++)
    {
        if(players[i] == actor || !(players[i] isBot())) continue;
        best = -1;
        bestDistance = 512;
        for(j = 0; j < spawns.size; j++)
        {
            if(isDefined(used[j])) continue;
            score = distance(spawns[j].origin,start);
            endDistance = distance(spawns[j].origin,finish);
            if(endDistance < score) score = endDistance;
            if(score > bestDistance) { best = j; bestDistance = score; }
        }
        if(best < 0)
        {
            println("[bot-nav-qa] FAIL no_remote_fixture_spawn");
            return;
        }
        used[best] = true;
        players[i] setOrigin(spawns[best].origin);
    }
    wait .5;
    // Collision-verified endpoints of the original ladder. Teleport only to
    // arrange the initial fixture; both tested journeys use normal bot input.
    if(map == "mp_toujane")
    {
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
        if(!ROOF_EXITS) return;
        actor setOrigin((2630,880,182));
        goal = (2263,2480,72);
    }
    else
    {
        actor setOrigin((1327,1760,144));
        goal = (1708,2191,-22);
    }
    wait 1;
    actor botAI(1);
    before = actor.health;
    for(t = 0; t < 240; t++)
    {
        actor botGoal(goal,12,3,-1);
        wait .5;
        // Test leaving the elevated area safely. Completing an objective is
        // covered separately by the original mode interaction scenarios.
        if(actor.origin[2] < start[2] - 96 && distance(actor.origin,start) > 128 && actor isOnGround())
        {
            if(actor.health < before)
            {
                println("[bot-nav-qa] FAIL roof_exit_damage map=" + map + " health=" + actor.health + " before=" + before);
                actor botStop();
                return;
            }
            println("[bot-nav-qa] PASS roof_exit map=" + map + " health=" + actor.health + " before=" + before + " position=" + actor.origin);
            actor botStop();
            return;
        }
    }
    println("[bot-nav-qa] FAIL roof_exit map=" + map + " position=" + actor.origin);
    actor botStop();
}
'''
output = root / 'out/bot-navigation-controlled.gsc'
output.parent.mkdir(exist_ok=True)
output.write_text(source + scenario.replace("ROOF_EXITS", "true" if args.roof_exits else "false"))
print(f'Generated first-party navigation scenario: {output}')
