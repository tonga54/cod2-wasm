// First-party rejoining. A person who leaves a match and joins it again under
// the same gametag connects with the score, deaths and match points they left
// with; the server keeps them meanwhile (g_points_mp.c). S&D holds score and
// deaths in pers between its rounds and sets them from there on each spawn.
main()
{
	if(getCvar("g_gametype") == "sd")
		level thread onPlayerConnect();
}

onPlayerConnect()
{
	for(;;)
	{
		level waittill("connecting", player);
		// A new connection: what the server gave back, if anything. Each
		// later round: pers, shown by a player the round does not spawn too,
		// so they still have it when they leave.
		if(!isDefined(player.pers["score"]))
		{
			player.pers["score"] = player.score;
			player.pers["deaths"] = player.deaths;
		}
		else
		{
			player.score = player.pers["score"];
			player.deaths = player.pers["deaths"];
		}
	}
}
