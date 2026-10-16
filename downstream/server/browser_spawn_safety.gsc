// Applied only to the weighted DM/TDM/CTF/HQ selector. Round and spectator
// random spawns keep their retail rules and map-authored point sets.
filterEnemySafeSpawns(spawnpoints, weights)
{
    result = spawnStruct();
    result.points = spawnpoints;
    result.weights = weights;
    enemies = [];
    living = [];
    hostile = [];
    players = getEntArray("player", "classname");
    freeForAll = getCvar("g_gametype") == "dm";
    for(i = 0; i < players.size; i++)
    {
        player = players[i];
        if(player == self || player.sessionstate != "playing" || !isAlive(player))
            continue;
        living[living.size] = player;
        hostile[living.size - 1] = freeForAll || player.pers["team"] != self.pers["team"];
        if(!hostile[living.size - 1])
            continue;
        enemies[enemies.size] = player;
    }
    if(living.size == 0)
        return result;

    // Clearance is checked once per respawn, using squared distances. Allies
    // do not count as enemies, but spawning against their body is still bad.
    clearance = [];
    crowded = [];
    bestClearance = -1;
    now = getTime();
    for(i = 0; i < spawnpoints.size; i++)
    {
        clearance[i] = -1;
        if(positionWouldTelefrag(spawnpoints[i].origin))
            continue;
        crowded[i] = false;
        nearest = 1000000000;
        for(j = 0; j < living.size; j++)
        {
            dist = distanceSquared(spawnpoints[i].origin, living[j].origin);
            if(dist < 128*128)
                crowded[i] = true;
            if(hostile[j] && dist < nearest)
                nearest = dist;
        }
        // Reserve a just-used point while the new player's body is being
        // linked. A later spawn must not reuse it just because it is not yet
        // visible to the collision query.
        if(isDefined(spawnpoints[i].lastspawntime) &&
           isDefined(spawnpoints[i].lastspawnedplayer) &&
           spawnpoints[i].lastspawnedplayer != self &&
           now - spawnpoints[i].lastspawntime >= 0 &&
           now - spawnpoints[i].lastspawntime < 750)
            crowded[i] = true;
        clearance[i] = nearest;
        if(nearest > bestClearance)
            bestClearance = nearest;
    }
    // Preserve the retail last resort when all map points are occupied.
    if(bestClearance < 0)
        return result;

    // Enemy separation takes priority over allied crowding. If there is no
    // point 1000 units away, keep the farthest available ones. Only relax the
    // allied/recent-use buffer when every enemy-safe candidate needs it.
    hasRoom = false;
    for(i = 0; i < spawnpoints.size; i++)
    {
        if(clearance[i] < 0 || (clearance[i] < 1000*1000 && clearance[i] < bestClearance))
            continue;
        if(!crowded[i])
            hasRoom = true;
    }
    result.points = [];
    result.weights = [];
    for(i = 0; i < spawnpoints.size; i++)
    {
        if(clearance[i] < 0)
            continue;
        if(clearance[i] < 1000*1000 && clearance[i] < bestClearance)
            continue;
        if(hasRoom && crowded[i])
            continue;
        slot = result.points.size;
        result.points[slot] = spawnpoints[i];
        result.weights[slot] = weights[i];
    }
    return preferCoveredSpawns(result, enemies);
}

preferCoveredSpawns(result, enemies)
{
    if(enemies.size == 0)
        return result;
    // At most four candidates and two nearby enemies each: eight visibility
    // traces per respawn, never a map-wide visibility search or per-frame work.
    tested = [];
    for(attempt = 0; attempt < 4 && attempt < result.points.size; attempt++)
    {
        best = undefined;
        offset = randomInt(result.points.size);
        for(i = 0; i < result.points.size; i++)
        {
            index = i + offset;
            if(index >= result.points.size) index -= result.points.size;
            if(isDefined(tested[index])) continue;
            if(!isDefined(best) || result.weights[index] > result.weights[best])
                best = index;
        }
        tested[best] = true;
        point = result.points[best];
        first = undefined; second = undefined;
        firstDist = 1600*1600; secondDist = 1600*1600;
        for(i = 0; i < enemies.size; i++)
        {
            dist = distanceSquared(point.origin, enemies[i].origin);
            if(dist < firstDist)
            {
                second = first; secondDist = firstDist;
                first = enemies[i]; firstDist = dist;
            }
            else if(dist < secondDist)
            {
                second = enemies[i]; secondDist = dist;
            }
        }
        exposed = false;
        if(isDefined(first))
            exposed = bulletTracePassed(first.origin + (0,0,48), point.origin + (0,0,48), false, undefined);
        if(!exposed && isDefined(second))
            exposed = bulletTracePassed(second.origin + (0,0,48), point.origin + (0,0,48), false, undefined);
        if(!exposed)
        {
            weight = result.weights[best];
            result.points = []; result.weights = [];
            result.points[0] = point; result.weights[0] = weight;
            return result;
        }
    }
    // Crowded maps may offer no covered candidate. Keep the distance-filtered
    // pool and original mode weights rather than selecting a nearby enemy.
    return result;
}
