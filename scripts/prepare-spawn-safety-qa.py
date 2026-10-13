#!/usr/bin/env python3
"""Prepare deterministic real-VM tests for the production final spawn selector."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
policy = (root / 'downstream/server/browser_bots.gsc').read_text().replace(
    '    level thread objectiveState();',
    '    level thread objectiveState();\n    level thread qaSpawnSafety();', 1)
policy = policy.replace('    self botAI(difficulty);\n    self.bot_difficulty = difficulty;',
                        '    self botStop();\n    self disableWeapon();\n    self.bot_difficulty = undefined;')
scenario = r'''
qaSpawnCheck(ok, name)
{
    if(ok) println("[bot-qa] PASS spawn_safety_" + name);
    else println("[bot-qa] FAIL spawn_safety_" + name);
}

qaSpawnSafety()
{
    level endon("intermission");
    wait 15;
    players = getEntArray("player", "classname");
    origins = []; teams = []; states = [];
    if(players.size < 4)
    {
        println("[bot-qa] FAIL spawn_safety_roster");
        return;
    }
    for(i = 0; i < players.size; i++)
    {
        if(!isAlive(players[i]))
        {
            println("[bot-qa] FAIL spawn_safety_alive");
            return;
        }
        origins[i] = players[i].origin;
        teams[i] = players[i].pers["team"];
        states[i] = players[i].sessionstate;
        players[i] botStop();
        players[i] disableWeapon();
        players[i].sessionstate = "spectator";
    }
    actor = players[0];
    enemy = players[1];
    ally = players[2];
    mode = getCvar("g_gametype");
    lastpoint = actor.lastspawnpoint;
    lasttime = actor.lastspawntime;
    actor.sessionstate = "playing";
    actor.pers["team"] = "allies";
    enemy.sessionstate = "playing";
    enemy.pers["team"] = "axis";
    enemy setOrigin((100, 0, 20000));
    points[0] = spawnStruct(); points[0].origin = (0, 0, 20000);
    points[1] = spawnStruct(); points[1].origin = (1200, 0, 20000);
    points[2] = spawnStruct(); points[2].origin = (2400, 0, 20000);
    weights[0] = 1000000; weights[1] = 10; weights[2] = 0;
    selected = actor maps\mp\gametypes\_spawnlogic::getSpawnpoint_Final(points, weights);
    qaSpawnCheck(selected == points[1], "near_enemy_cannot_win_weight");
    actor.lastspawnpoint = points[1];
    weights[1] = -100000; weights[2] = -200000;
    selected = actor maps\mp\gametypes\_spawnlogic::getSpawnpoint_Final(points, weights);
    qaSpawnCheck(selected == points[1], "safe_reuse_and_negative_weights");

    points[1].origin = (400, 0, 20000);
    points[2].origin = (800, 0, 20000);
    selected = actor maps\mp\gametypes\_spawnlogic::getSpawnpoint_Final(points, weights);
    qaSpawnCheck(selected == points[2], "all_threatened_maximize_clearance");

    points[0].origin = (949, 0, 20000);
    points[1].origin = (950, 0, 20000);
    points[2].origin = (2000, 0, 20000);
    weights[1] = 10; weights[2] = 0;
    selected = actor maps\mp\gametypes\_spawnlogic::getSpawnpoint_Final(points, weights);
    qaSpawnCheck(selected == points[1], "850_unit_boundary");

    ally.sessionstate = "playing"; ally.pers["team"] = "allies";
    ally setOrigin(points[1].origin);
    selected = actor maps\mp\gametypes\_spawnlogic::getSpawnpoint_Final(points, weights);
    qaSpawnCheck(selected == points[2], "occupied_safe_point_excluded");
    ally setOrigin((700, 0, 20000));
    enemy.sessionstate = "spectator";
    selected = actor maps\mp\gametypes\_spawnlogic::getSpawnpoint_Final(points, weights);
    qaSpawnCheck(selected == points[0], "spectators_and_teammates_ignored");
    enemy.sessionstate = "dead";
    selected = actor maps\mp\gametypes\_spawnlogic::getSpawnpoint_Final(points, weights);
    qaSpawnCheck(selected == points[0], "dead_players_ignored");

    // Same-team players are still enemies in free-for-all.
    setCvar("g_gametype", "dm");
    selected = actor maps\mp\gametypes\_spawnlogic::getSpawnpoint_Final(points, weights);
    qaSpawnCheck(selected == points[2], "dm_same_team_is_enemy");
    setCvar("g_gametype", mode);
    ally.sessionstate = "spectator";
    selected = actor maps\mp\gametypes\_spawnlogic::getSpawnpoint_Final(points, weights);
    qaSpawnCheck(selected == points[0], "no_enemies_retail_weights");

    // Every point occupied: let the existing retail last resort handle it.
    enemy.sessionstate = "playing";
    enemy setOrigin(points[0].origin);
    ally.sessionstate = "playing"; ally setOrigin(points[1].origin);
    players[3].sessionstate = "playing"; players[3] setOrigin(points[2].origin);
    filtered = actor maps\mp\gametypes\_spawnlogic::filterEnemySafeSpawns(points, weights);
    qaSpawnCheck(filtered.points.size == points.size, "all_occupied_retail_fallback");

    actor.lastspawnpoint = lastpoint;
    actor.lastspawntime = lasttime;
    for(i = 0; i < players.size; i++)
    {
        players[i] setOrigin(origins[i]);
        players[i].pers["team"] = teams[i];
        players[i].sessionstate = states[i];
    }
}
'''
output = root / 'out/spawn-safety/controlled.gsc'
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(policy + scenario)
print(output)
