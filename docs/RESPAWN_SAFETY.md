# Respawn separation and cover

The weighted DM/TDM/CTF/HQ selector previously rejected overlapping players and
preferred points at least 850 map units from enemies. It did not give nearby
teammates extra space, and distance alone could admit an exposed spawn.

The filter now applies these priorities in order:

1. Reject physically occupied spawn points when any free point exists.
2. Keep points at least 1000 units from the nearest living enemy. If none meet
   that distance, retain the free points with the greatest enemy clearance.
3. Prefer a 128-unit buffer from all living players, including teammates, and
   avoid another player's spawn point for 750 ms after it is selected. If every
   remaining point violates this soft buffer, relax it so respawns still work.
4. Examine up to four candidates in their existing mode-weight order, breaking
   ties randomly. Prefer one with no direct bullet sightline from either of its
   two nearest enemies within 1600 units. At most eight extra sight traces run
   per respawn. If the candidates are exposed, retain the distance-filtered pool
   and the original mode weights.

Distances use one squared-distance calculation per living player and candidate.
The filter runs only during selection, with no new frame loop, timer, renderer
work or change to the respawn countdown. It retains each mode's authored spawn
set and objective/team weights. Spectator and S&D round spawns are unchanged.

This reduces avoidable close/exposed spawns; it cannot guarantee an empty or
hidden point in a saturated map. If every authored point is physically occupied,
the original last-resort behavior remains. The bounded visibility check does not
test every enemy or simulate bullet penetration through cover.

## Validation

`scripts/prepare-spawn-safety-qa.py` generates controlled scenarios executed by
the actual native server and GSC VM, using four connected bots. Twenty checks
passed on each of Carentan, Toujane and Dawnville (60 total): enemy threshold,
negative/reuse weights, occupied points, no-enemy friendly spacing, dead and
spectating players, free-for-all hostility, reservation expiry, crowded-map
fallbacks, enemy priority, unweighted selection, and exposed/covered positions
found in the actual BSP collision geometry.

Separate Carentan matches completed two death/respawn cycles in each of DM,
TDM, CTF and HQ, followed by normal patrol movement after every respawn. All
16 lifecycle assertions passed using normal bot input and the original mode
callbacks, without forcing the spawn function. Test containers were removed.

The generated private package passed `scripts/check-private-assets.py`, including
all 4745 entries, hashes, reviewed overrides and the three maps/five modes. The
browser package with the matching asset manifest passed the framework contract.
No original private assets were committed.

The candidate assets and browser package are staged under ignored
`out/respawn-qa/main` and `out/respawn-qa/site`. This avoids changing archives
mounted by existing rooms. The running service and its asset directory were not
updated. Normal packaging applies this change through
`scripts/prepare-browser-bootstrap.py` and `scripts/browser_spawn_safety.py`.

To repeat the controlled checks after preparing the candidate assets:

```sh
python3 scripts/prepare-spawn-safety-qa.py
python3 scripts/test-bot-runtime.py --assets out/respawn-qa/main \
  --map mp_carentan --mode tdm --bots 4 --seconds 30 \
  --expect-passes 20 --stop-after-passes \
  --policy out/spawn-safety/controlled.gsc --output out/respawn-qa/controlled
```
