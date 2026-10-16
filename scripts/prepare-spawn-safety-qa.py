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
    // Controlled team cases have the same rules regardless of the host mode.
    setCvar("g_gametype", "tdm");
    lastpoint = actor.lastspawnpoint;
    lasttime = actor.lastspawntime;
    actor.sessionstate = "playing";
    actor.pers["team"] = "allies";
    enemy.sessionstate = "playing";
    enemy.pers["team"] = "axis";
    enemy setOrigin((100, 0, 20000));
    points[0] = spawnStruct(); points[0].origin = (0, 0, 20000);
    points[1] = spawnStruct(); points[1].origin = (1200, 0, 20000);
    points[2] = spawnStruct(); points[2].origin = (1500, 0, 20000);
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

    points[0].origin = (1099, 0, 20000);
    points[1].origin = (1100, 0, 20000);
    points[2].origin = (1500, 0, 20000);
    weights[1] = 10; weights[2] = 0;
    selected = actor maps\mp\gametypes\_spawnlogic::getSpawnpoint_Final(points, weights);
    qaSpawnCheck(selected == points[1], "1000_unit_boundary");

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
    setCvar("g_gametype", "tdm");
    ally.sessionstate = "spectator";
    selected = actor maps\mp\gametypes\_spawnlogic::getSpawnpoint_Final(points, weights);
    qaSpawnCheck(selected == points[0], "no_enemies_retail_weights");

    // A teammate need not overlap the exact spawn hull to crowd it. This
    // must also work before the opposing team has joined the game.
    ally.sessionstate = "playing";
    ally setOrigin(points[0].origin + (64,0,0));
    selected = actor maps\mp\gametypes\_spawnlogic::getSpawnpoint_Final(points, weights);
    qaSpawnCheck(selected == points[2], "near_ally_without_enemies");
    points[1].origin = (1400, 0, 20000);
    selected = actor maps\mp\gametypes\_spawnlogic::getSpawnpoint_Final(points, weights);
    qaSpawnCheck(selected == points[1], "keep_team_weights_after_crowd_filter");
    ally.sessionstate = "dead";
    selected = actor maps\mp\gametypes\_spawnlogic::getSpawnpoint_Final(points, weights);
    qaSpawnCheck(selected == points[0], "dead_ally_does_not_crowd");
    ally.sessionstate = "playing"; ally setOrigin((700, 0, 20000));
    points[0].lastspawnedplayer = ally; points[0].lastspawntime = getTime();
    selected = actor maps\mp\gametypes\_spawnlogic::getSpawnpoint_Final(points, weights);
    qaSpawnCheck(selected == points[1], "recent_spawn_reserved");
    points[0].lastspawntime = getTime() - 750;
    selected = actor maps\mp\gametypes\_spawnlogic::getSpawnpoint_Final(points, weights);
    qaSpawnCheck(selected == points[0], "reservation_expires");
    ally setOrigin(points[0].origin + (128,0,0));
    selected = actor maps\mp\gametypes\_spawnlogic::getSpawnpoint_Final(points, weights);
    qaSpawnCheck(selected == points[0], "128_unit_ally_boundary");

    // Tight friendly clusters must relax only the proximity buffer, without
    // blocking respawns or undoing the enemy/physical-occupancy checks.
    ally setOrigin((1163, 0, 20000));
    points[1].origin = (1131, 0, 20000);
    points[2].origin = (1195, 0, 20000);
    selected = actor maps\mp\gametypes\_spawnlogic::getSpawnpoint_Final(points, weights);
    qaSpawnCheck(selected == points[0], "all_friendly_crowded_still_spawns");
    points[1].origin = (1400, 0, 20000);
    points[2].origin = (1500, 0, 20000);
    enemy.sessionstate = "playing"; enemy setOrigin((1560, 0, 20000));
    selected = actor maps\mp\gametypes\_spawnlogic::getSpawnpoint_Final(points, weights);
    qaSpawnCheck(selected == points[0], "enemy_clearance_before_friendly_spacing");
    enemy.sessionstate = "spectator";

    // Unweighted round/spectator selectors preserve their point order/rules.
    actor.lastspawnpoint = undefined;
    selected = actor maps\mp\gametypes\_spawnlogic::getSpawnpoint_Final(points);
    qaSpawnCheck(selected == points[0], "unweighted_selector_unchanged");

    // Every point occupied: let the existing retail last resort handle it.
    enemy.sessionstate = "playing";
    enemy setOrigin(points[0].origin);
    ally.sessionstate = "playing"; ally setOrigin(points[1].origin);
    players[3].sessionstate = "playing"; players[3] setOrigin(points[2].origin);
    filtered = actor maps\mp\gametypes\_spawnlogic::filterEnemySafeSpawns(points, weights);
    qaSpawnCheck(filtered.points.size == points.size, "all_occupied_retail_fallback");

    // Exercise the coverage stage with an exposed/covered pair in the actual
    // BSP. Distance/occupancy filtering was checked separately above.
    ally.sessionstate = "spectator";
    players[3].sessionstate = "spectator";
    mapPoints = getEntArray("mp_tdm_spawn", "classname");
    println("[spawn-qa] map points=" + mapPoints.size);
    observer = undefined; visible = undefined; covered = undefined;
    sightChecks = 0;
    visibleChecks = 0;
    for(e = 0; e < mapPoints.size && sightChecks < 1024; e++)
    {
        visible = undefined; covered = undefined;
        for(p = 0; p < mapPoints.size && sightChecks < 1024; p++)
        {
            d = distanceSquared(mapPoints[e].origin, mapPoints[p].origin);
            if(d < 128*128 || d >= 1600*1600) continue;
            sightChecks++;
            exposed = bulletTracePassed(mapPoints[e].origin + (0,0,48), mapPoints[p].origin + (0,0,48), false, undefined);
            if(exposed) visibleChecks++;
            if(exposed) visible = mapPoints[p];
            else covered = mapPoints[p];
            if(isDefined(visible) && isDefined(covered)) break;
        }
        if(isDefined(visible) && isDefined(covered))
        {
            observer = mapPoints[e];
            break;
        }
    }
    if(isDefined(observer))
    {
        enemy setOrigin(observer.origin);
        coverPoints = []; coverWeights = [];
        coverPoints[0] = visible; coverPoints[1] = covered;
        coverWeights[0] = 1000000; coverWeights[1] = 0;
        pool = spawnStruct(); pool.points = coverPoints; pool.weights = coverWeights;
        observers = []; observers[0] = enemy;
        filtered = actor maps\mp\gametypes\_spawnlogic::preferCoveredSpawns(pool, observers);
        qaSpawnCheck(filtered.points.size == 1 && filtered.points[0] == covered, "actual_map_cover_beats_team_weight");
    }
    else
    {
        println("[spawn-qa] map sight checks=" + sightChecks + " visible=" + visibleChecks);
        qaSpawnCheck(false, "actual_map_cover_fixture");
    }

    actor.lastspawnpoint = lastpoint;
    actor.lastspawntime = lasttime;
    setCvar("g_gametype", mode);
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
