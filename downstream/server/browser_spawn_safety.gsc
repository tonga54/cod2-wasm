// Applied only to the weighted DM/TDM/CTF/HQ selector. Round and spectator
// random spawns keep their retail rules and map-authored point sets.
filterEnemySafeSpawns(spawnpoints, weights)
{
    result = spawnStruct();
    result.points = spawnpoints;
    result.weights = weights;
    enemies = [];
    players = getEntArray("player", "classname");
    freeForAll = getCvar("g_gametype") == "dm";
    for(i = 0; i < players.size; i++)
    {
        player = players[i];
        if(player == self || player.sessionstate != "playing" || !isAlive(player))
            continue;
        if(!freeForAll && player.pers["team"] == self.pers["team"])
            continue;
        enemies[enemies.size] = player;
    }
    if(enemies.size == 0)
        return result;

    // 850 units matches the retail HQ exclusion radius. If every free point
    // is threatened, maximize nearest-enemy distance instead of going random.
    clearance = [];
    bestClearance = -1;
    for(i = 0; i < spawnpoints.size; i++)
    {
        clearance[i] = -1;
        if(positionWouldTelefrag(spawnpoints[i].origin))
            continue;
        nearest = 1000000;
        for(j = 0; j < enemies.size; j++)
        {
            dist = distance(spawnpoints[i].origin, enemies[j].origin);
            if(dist < nearest)
                nearest = dist;
        }
        clearance[i] = nearest;
        if(nearest > bestClearance)
            bestClearance = nearest;
    }
    // Preserve the retail last resort when all map points are occupied.
    if(bestClearance < 0)
        return result;

    result.points = [];
    result.weights = [];
    for(i = 0; i < spawnpoints.size; i++)
    {
        if(clearance[i] < 0)
            continue;
        if(clearance[i] < 850 && clearance[i] < bestClearance)
            continue;
        slot = result.points.size;
        result.points[slot] = spawnpoints[i];
        result.weights[slot] = weights[i];
    }
    return result;
}
