# Multiplayer bot behavior

Configure 0–16 bots and Easy, Normal or Hard difficulty when creating a room.
The default remains no bots. Bot population and difficulty belong to the room
and persist across its map rotation. All UI and bot labels are in English.

The [mode table in the README](../README.md#bots) describes their objectives.
The first-party policy in `downstream/server/browser_bots.gsc` assigns roles;
the native controller in `src/PC/server_mp/sv_bot_ai.inc` supplies movement,
aim and ordinary user commands. Retail gametype scripts retain ownership of
spawning, faction weapons, ammunition, damage, scoring and round transitions.

## Perception, combat and difficulty

New targets must pass the difficulty's view cone and collision/smoke visibility
checks. A close visible enemy can alert the bot. An enemy that disappears is
remembered at its last visible position for a limited time; its hidden live
position does not update that memory or authorize firing. Incoming damage can
turn the bot toward the impact direction.

Easy, Normal and Hard use 700, 400 and 200 ms reaction delays, respectively,
with progressively faster turns, smaller aiming error and longer bursts with
shorter pauses. Tracking includes a small difficulty-dependent correction for
visible movement, including jumping. Briefly losing and regaining the same
opponent does not repeatedly restart the reaction delay. Objective decisions run every 750, 500 and 250 ms. Difficulty
does not change health, weapon damage, ammunition, recoil or firing cadence.

Normal and Hard bots search nearby reachable cover when wounded or reloading.
Bots avoid firing into a teammate along their actual aim. SMGs close to a
shorter engagement range than rifles and machine guns; bots advance toward
visible distant opponents rather than permanently orbiting them. Short combat
dodges have pauses and do not move a bot out of a held capture objective.
Carrying a flag or holding use takes priority over combat strafing. They reload
empty magazines and switch to an owned loaded weapon when out of ammunition.

## Objectives and navigation

CTF uses public flag/base/carrier positions for movement. HQ uses only the
currently announced radio. S&D uses the active bomb sites and planted bomb.
These positions do not bypass combat visibility checks. A human already using
an objective, or the currently interacting bot, retains that interaction while
other bots cover it. Bots wait to land before starting a bomb interaction, then
hold ordinary use through the original plant or defuse timer.

Teams receive a mix of SMGs, rifles, automatic support weapons and an original
scoped rifle for every four bots. Faction weapon callbacks still enforce the
server's restrictions. Snipers favor elevated positions and long sight lines,
relocate between bounded observation periods and wait for the normal scope
transition before firing. They use crouching and short breath-hold requests.
TDM patrols independently search lanes toward the shared fighting area. Guards
and escorts select separate reachable positions around their objective and
pause between scans instead of spinning continuously.

Navigation grows from actual collision, spawn points and objective positions.
The grid distinguishes floors and uses eight horizontal neighbors, allowing
safe diagonal routes through buildings and stairways. Ground sampling starts
within step height to avoid accidentally selecting the ceiling or roof above
a room. Original ladder collision materials create climb/descend links to
accessible upper floors and rooftops. Ordinary movement owns ladder attachment,
collision, climbing and falling; AI does not teleport or write player velocity.
Jump-only and mantle-only routes remain unsupported.

A shared priority queue expands the arena's central combat lanes before remote
spawn courtyards. Each path decision also scouts up to six nearby cells toward
the objective or shared fighting area, with its own four-millisecond budget.
This keeps distant spawn nodes connected as the shared graph warms up. Local
branches can initially go away from their destination to reach a doorway around
a wall, reject repeated scout cells and avoid choosing an idle patrol cell.
Warm-up uses at most 128 edge attempts and a four-millisecond time budget,
checked every eight attempts. A single collision attempt can exceed that budget
on a slow host; it is a work bound, not a latency guarantee. The spawn envelope
plus a 512-unit margin bounds exploration so exterior ground planes do not
consume the graph. Searches share fixed buffers and bots cache up to 64 route
nodes, smoothing short sections after checking floors and corners. Replanning
does not force a bot to walk backward to a grid center.

Immediate collision checks and local steering help bots pass corners and
teammates. A blocked route receives a per-bot eight-second edge exclusion;
repeated failures trigger a six-second exploration detour before retrying the
strategic objective. Disconnected objective projections also trigger recovery.
Path decisions run every 1.2 seconds, with prompt advancement between cached
waypoints. Navigation and command generation allocate no per-frame heap memory.

This is multiplayer AI written for this project. It does not reuse campaign
scripts or promise campaign-level tactical behavior. It has no authored navmesh,
jump/mantle route planner, tactical grenade planner or mounted-gun strategy.
Complex routes, coordination and stalemates still warrant playtesting.

## Objective engine corrections

The objective work also repairs reconstructed engine behavior shared by humans
and bots:

- Both constant-vector compilation paths now preserve source x/y/z order,
  matching runtime vectors. This fixes misplaced map radios and kill volumes.
- Circular trigger contact recognizes players inside the flag radius.
- Brush use triggers enter the use list; team restrictions and cursor hint
  fields match the fields written by the original script methods.
- Linked-player transforms include the translation row without reading outside
  the rotation matrix during a plant or defuse hold.
- Use triggers register their final contents/flags. Scripted origin, angle and
  health writes run the engine callbacks, so dropped flags and the moved defuse
  trigger update collision/network state and player health stays synchronized.
- Private packaging includes dynamically named original flag/radio models and
  objective materials with their original dependencies. Retail asset bytes are
  preserved; generated archives remain private.

## Validation

The normal static suite includes the production bot controller and regression
tests for route progress, blocked-route alternatives, guard spacing, ladder
links/input, warm-up time limits, jump tracking, scoped ADS and weapon-range
pursuit. It also tests vector order, objective contact/use, linked-player
transforms and trigger/field dispatch. The vector and linked-player tests run under ASan/UBSan.
The tests reject variants with the old reversed vectors, outside-only flag
contact, wrong use flag/team/hint fields, three-row transform and missing field
callbacks.

For native objective interaction tests, run:

```sh
python3 scripts/prepare-bot-objective-qa.py
```

This generates `out/bot-objectives-controlled.gsc` from the first-party policy
and an authored scenario. Mount it as `/config/browser_bots.gsc` in an isolated
test server, enable `scr_bot_debug`, and create CTF, HQ and S&D rooms with eight
bots. Keep production games on `downstream/server/browser_bots.gsc`.

The scenario arranges player positions and disables unrelated combat, then
enables the real bot objective policy for the actor. It checks original flag
pickup/capture, HQ capture/neutralization, and plant/defuse holds. It never
increments score, forces an objective trigger or shortens the original timers.
Successful actions print `[bot-qa] PASS`; a missed action prints `FAIL`.

Controlled tests establish interaction correctness, not navigation quality.
Native tests on the Raspberry Pi with eight Normal bots per room completed
flag pickup/capture, HQ capture/neutralization and bomb plant/defuse on both
Toujane and Carentan. HQ actions took 9.5 seconds; plant and defuse checks
completed after 5.5 and 10.5 seconds, preserving their original hold timers.
Autonomous matches also produced flag captures, contested HQ ownership and
bomb planting with subsequent round transitions. These are reproducible test
observations, not a guarantee that every route or combat situation is bug-free.

To verify a real ladder round trip, run:

```sh
python3 scripts/prepare-bot-navigation-qa.py
```

Mount the generated `out/bot-navigation-controlled.gsc` in an isolated Toujane
TDM room. The scenario arranges one actor at an original ladder, then uses only
normal bot input for the climb onto the roof and the return to the ground.
Both landings must pass position and original grounded-state checks. This
scenario completed both phases on the Raspberry Pi; it verifies that route
replanning preserves the link during a climb, not every ladder on every map.

Separately observe autonomous matches on Toujane, Carentan and Dawnville: check that bots
reach objectives, score, change roles, obey round transitions and fight under
the selected difficulty. Include a real browser client when measuring network
latency; bots do not consume browser snapshot traffic or client rendering.
