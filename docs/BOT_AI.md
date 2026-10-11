# Multiplayer bot behavior

Configure 0–32 bots and Easy, Normal or Hard difficulty when creating a room.
The menu offers None, 1–16 and 20/24/28/32 presets; the room API accepts every integer in the range.
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
a room. Walk checks combine sampled floor height with standing-body clearance.
Original ladder collision materials create climb/descend links to
accessible upper floors and rooftops. Ordinary movement owns ladder attachment,
collision, climbing and falling; AI does not teleport or write player velocity.
Directed drop links let bots leave roofs and elevated rooms. They check a full
standing capsule at departure and landing, reject damaging falls, and can use
a normal jump pulse over a low parapet when headroom permits. These links use the configured original jump height and never invent a
reverse climb. General obstacle/gap jumps and mantle routes remain unsupported.

A shared priority queue favors central combat lanes while weighting insertion
order so continuous central growth cannot starve older spawn/roof frontiers.
Path decisions scout up to six nearby cells toward the objective or shared
fighting area, sharing one two-millisecond allowance across all bots in the tick.
This keeps distant spawn nodes connected as the shared graph warms up. Local
branches can initially go away from their destination to reach a doorway around
a wall, reject repeated scout cells and avoid choosing an idle patrol cell.
Warm-up uses at most 128 edge attempts and a four-millisecond time budget,
checked after each attempt. A single collision attempt can exceed that budget
on a slow host; it is a work bound, not a latency guarantee. The spawn envelope
plus a 512-unit margin bounds exploration so exterior ground planes do not
consume the graph. Failed static edges are cached so nearby walls do not
repeatedly consume collision traces. Narrow doorways and short stair lips can
add validated half-cell links that rejoin the ordinary grid as soon as possible.
Walking arrivals tolerate the sampled step-height difference; ladder and drop
landings retain their three-dimensional checks. Crowded-spawn steering uses
the original pmove retry for an existing player-body overlap, while native
movement still owns collision and rejects walls.

A* searches use Euclidean edge costs and a shared fixed heap, with at most
2,048 expanded nodes and a two-millisecond limit checked every 32 expansions.
At most two queued paths run per simulation tick, in round-robin client order.
An incomplete search retains a usable route toward its closest explored point.
Valid routes survive a replan while their intent and destination remain stable.
Patrol goals penalize the last eight visited areas and nearby teammates' claimed
destinations, with capped penalties so a large team is not pushed outside the
fighting area. Searches share fixed buffers and bots cache up to 64 route nodes. Flat, collinear sections reuse their sampled floor checks and need just
one shortcut capsule trace; turns and stairs retain their individual nodes. Replanning
does not force a bot to walk backward to a grid center.

Immediate collision checks and local steering help bots pass corners and
teammates. A blocked route receives a per-bot eight-second edge exclusion;
repeated failures trigger a six-second exploration detour before retrying the
strategic objective. Disconnected objective projections also trigger recovery,
including repeated arrivals at the same nearby cell outside a reachable route.
Partial routes that still approach the objective during graph warm-up keep
their objective; only consecutive decisions without progress count as failures.
Announced model objectives such as radios are projected onto their walking
floor; their model center cannot silently choose an unreachable roof instead.
Path decisions run every 1.2 seconds, with prompt advancement between cached
waypoints. Navigation and command generation allocate no per-frame heap memory.

Movement and aim commands run once per original 50 ms simulation tick,
including each tick of a delayed-frame catch-up. Objective script threads start
at staggered phases and reuse the shared mode state. Perception preserves the
difficulty view cone and smoke checks; a rotating third sightline candidate
prevents blocked nearby opponents from permanently hiding the next visible
one. A visibility result is reused only within the same tick.

Test clients skip HUD-only teammate name/health queries and graphical crosshair
hitbox rays. Human clients retain those original queries. Collision-linked
scripted look-at brushes still use the original occlusion/hitbox trace when
the bot's view ray can intersect their current bounds. Combat visibility,
bullet collision, damage, weapon cadence, models, animations, effects and
render settings are retained.

Opt-in `scr_bot_profile 1` reports aggregate five-second timings for planning,
AI, movement, game scripts and snapshots. It defaults off. The native x86 build
uses `-msse2 -mfpmath=sse`, with standard floating-point semantics and no
fast-math. Set `-DCOD2_SSE2_MATH=OFF` when building for older x86 CPUs that lack
SSE2; the 32-bit ABI is unchanged.

This is multiplayer AI written for this project. It does not reuse campaign
scripts or promise campaign-level tactical behavior. It has no authored navmesh,
general jump/mantle route planner, tactical grenade planner or mounted-gun strategy.
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
links/input, frontier fairness,
directed roof exits and fall rejection, shared warm-up/scout time limits,
bounded guided paths, fair planning queues, route reuse, sparse doorways,
crowded-spawn escape, fine-step arrival and jump tracking,
scoped ADS and weapon-range pursuit. It also tests vector order, objective contact/use, linked-player
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
flag pickup/capture, HQ capture/neutralization and bomb plant/defuse on
Toujane, Carentan and Dawnville. HQ actions took 9.5 seconds; plant and defuse checks
completed after 5.5 and 10.5 seconds, preserving their original hold timers.
Autonomous matches also produced flag captures, contested HQ ownership and
bomb planting with subsequent round transitions. These are reproducible test
observations, not a guarantee that every route or combat situation is bug-free.

To verify a real ladder round trip, run:

```sh
python3 scripts/prepare-bot-navigation-qa.py
```

Mount the generated `out/bot-navigation-controlled.gsc` in an isolated Toujane
TDM room. The default scenario arranges one actor at an original ladder, then uses only
normal bot input for the climb onto the roof and the return to the ground.
Both landings must pass position and original grounded-state checks. This
scenario completed both phases on the Raspberry Pi; it verifies that route
replanning preserves the link during a climb, not every ladder on every map.

Add `--roof-exits` when generating to also probe complex elevated escapes on
Toujane and Carentan. These are diagnostic stress cases, not certified routes;
some enclosed roofs remain disconnected. They test a safe, grounded exit
without damage, while objective completion has its own scenario above.

Separately observe autonomous matches on Toujane, Carentan and Dawnville: check that bots
reach objectives, score, change roles, obey round transitions and fight under
the selected difficulty. Include a real browser client when measuring network
latency; bots do not consume browser snapshot traffic or client rendering.
