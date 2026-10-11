#!/usr/bin/env python3
"""Generate isolated, first-party ladder and elevated-spawn escape scenarios."""
import argparse
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--roof-exits", action="store_true", help="Also exercise elevated escape routes on Toujane, Carentan and Dawnville")
parser.add_argument("--upper-objective", action="store_true", help="Reach Carentan's elevated radio position from the floor below")
args = parser.parse_args()
if args.upper_objective:
    args.roof_exits = True

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
    if(UPPER_OBJECTIVE && map != "mp_carentan")
    {
        println("[bot-nav-qa] FAIL upper_objective_requires_carentan");
        return;
    }
    if(map == "mp_dawnville" && ROOF_EXITS)
    {
        qaDawnvilleRoofs();
        return;
    }
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
    if(UPPER_OBJECTIVE)
    {
        start = (-831.6,2098.8,-8);
        finish = (-831.6,2098.8,179.4);
    }
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
        actor setOrigin(start);
        goal = finish;
    }
    wait 1;
    actor botAI(1);
    before = actor.health;
    upperAttempt = 0;
    for(t = 0; t < 240; t++)
    {
        if(UPPER_OBJECTIVE) actor botGoal(goal,48,0,-1);
        else actor botGoal(goal,12,3,-1);
        wait .5;
        if(UPPER_OBJECTIVE && distance(actor.origin,goal) < 80 && actor isOnGround())
        {
            println("[bot-nav-qa] PASS upper_objective phase=" + upperAttempt + " ms=" + (t+1)*500 + " position=" + actor.origin);
            actor botStop();
            if(upperAttempt == 0)
            {
                // Reuse learned map geometry, as an actual later life does.
                actor setOrigin(start);
                wait 1;
                actor botAI(1);
                upperAttempt = 1;
                t = -1;
                continue;
            }
            return;
        }
        // Test leaving the elevated area safely. Completing an objective is
        // covered separately by the original mode interaction scenarios.
        if(!UPPER_OBJECTIVE && actor.origin[2] < start[2] - 96 && distance(actor.origin,start) > 128 && actor isOnGround())
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
    if(UPPER_OBJECTIVE) println("[bot-nav-qa] FAIL upper_objective position=" + actor.origin);
    else println("[bot-nav-qa] FAIL roof_exit map=" + map + " position=" + actor.origin);
    actor botStop();
}

qaDawnvilleRoofs()
{
    // Authored TDM spawns on three different elevated structures. Only the
    // fixture uses setOrigin; escape and landing use original movement.
    starts[0] = (-22,-16906,192);
    starts[1] = (-361,-18197,180);
    starts[2] = (1015,-15300,134);
    goals[0] = (56,-17011,56);
    goals[1] = (-17,-18134,68);
    goals[2] = (901,-15604,19);
    players = getEntArray("player", "classname");
    for(i = 0; i < players.size; i++)
    {
        players[i].bot_difficulty = undefined;
        players[i] botStop();
        players[i] disableWeapon();
        if(i < 3) players[i] setOrigin(starts[i]);
        else players[i] setOrigin((2824 + (i-3)*40,-15592,-32));
    }
    if(players.size < 3)
    {
        println("[bot-nav-qa] FAIL requires_three_actors");
        return;
    }
    wait 1;
    for(i = 0; i < 3; i++) players[i] thread qaDawnvilleExit(i,goals[i]);
}

qaDawnvilleExit(index, goal)
{
    start = self.origin;
    before = self.health;
    started = getTime();
    self botAI(1);
    for(t = 0; t < 240; t++)
    {
        self botGoal(goal,12,3,-1);
        wait .5;
        if(self.origin[2] < start[2]-64 && distance(self.origin,start) > 96 && self isOnGround())
        {
            if(self.health < before)
                println("[bot-nav-qa] FAIL roof_exit_damage map=mp_dawnville case=" + index + " health=" + self.health);
            else println("[bot-nav-qa] PASS roof_exit map=mp_dawnville case=" + index + " ms=" + (getTime()-started) + " position=" + self.origin);
            self botStop();
            return;
        }
    }
    println("[bot-nav-qa] FAIL roof_exit map=mp_dawnville case=" + index + " position=" + self.origin);
    self botStop();
}
'''
output = root / ('out/bot-upper-controlled.gsc' if args.upper_objective else 'out/bot-navigation-controlled.gsc')
output.parent.mkdir(exist_ok=True)
output.write_text(source + scenario.replace("ROOF_EXITS", "true" if args.roof_exits else "false")
                  .replace("UPPER_OBJECTIVE", "true" if args.upper_objective else "false"))
print(f'Generated first-party navigation scenario: {output}')
