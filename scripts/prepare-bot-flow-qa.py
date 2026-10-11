#!/usr/bin/env python3
"""Instrument original first spawns; never assign positions, goals or movement."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
source = (root/'downstream/server/browser_bots.gsc').read_text().replace(
    '    self botAI(difficulty);', '    self botAI(difficulty);\n    self thread qaSpawnDeparture();', 1)
source += r'''
qaSpawnDeparture()
{
    self endon("disconnect");
    self endon("death");
    level endon("intermission");
    for(i = 0; i < 20 && !isAlive(self); i++) wait .25;
    if(!isAlive(self)) return;
    start = self.origin;
    started = getTime();
    for(i = 0; i < 160; i++)
    {
        if(distance(self.origin,start) >= 256)
        {
            println("[bot-flow-qa] departure client=" + self getEntityNumber() + " ms=" + (getTime()-started));
            return;
        }
        wait .25;
    }
    println("[bot-flow-qa] departure_timeout client=" + self getEntityNumber());
}
'''
output = root/'out/bot-flow-controlled.gsc'
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(source)
print(output)
