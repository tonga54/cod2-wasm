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
checks. A close visible enemy can alert the bot. Nearby teammates can share a
contact that a bot actually saw, for at most 1.6 seconds. Reports neither
propagate between teams nor authorize shooting. An enemy that disappears is
remembered at its last visible position for a limited time; its hidden live
position does not update that memory or authorize firing. Incoming damage can
turn the bot toward the impact direction.

Each shared contact admits at most two investigating teammates, with expiring
reservations. Other bots continue their own patrols instead of the whole team
converging on the latest report. Dead responders and bots assigned an objective
release their reservation.

Easy, Normal and Hard use 700, 650 and 200 ms base reaction delays, respectively.
Normal's combat tuning is deliberately forgiving while retaining its tactical
decisions: 170 degrees/second turn rate, up to 2.6 degrees of error per aim axis,
300 ms bursts and 450 ms pauses. Hard retains its previous settings. Tracking includes a small difficulty-dependent correction for
visible movement, including jumping. Briefly losing and regaining the same
opponent does not repeatedly restart the reaction delay. Objective decisions run
every 750, 500 and 250 ms. Difficulty does not change health, weapon damage,
ammunition, recoil or the weapon's mechanical firing cadence.

Easy and Normal snipers additionally need 650 / 450 ms of completed ADS with
the weapon ready. Loss of visibility, incomplete ADS, a weapon cooldown or a
target change resets that stabilization; Hard retains its original ADS rule.
For Normal, the first-shot reaction is at least 850 ms at range and 1,100 ms
inside 384 units; Easy uses 1,050 / 1,350 ms. Sniper aim error scales smoothly
from 1.25 times base error at range to four times at point-blank distance.
These restrictions curb instant close-range scoped shots without changing
sniper damage: an accurate shot can still kill with one bullet.

The balance regression samples generated attack/aim commands over 64 seeds,
checks first-shot delays across difficulties and checks stabilization resets.
Normal's sampled mean rifle yaw error was 1.24 degrees versus Hard's 0.19.
Close scoped commands missed a 30-unit-wide target horizontally in 21/64
Normal samples versus 0/64 Hard samples. This is synthetic command geometry,
not a measured in-game hit rate or a guarantee about difficulty perception.
The change uses one extra timer per bot (256 bytes across 64 slots), no new
collision traces, and less frequent aim-error updates for Easy/Normal.

Local validation of this balance adjustment passed ASan/UBSan, native i386 and
WebAssembly builds, and simulation cadence checks. A 60-second Dawnville TDM
run with 32 Normal bots sustained 20 ticks/s at 29.23% of one core; the largest
sampled single-bot AI call was 6 ms. This is local Docker performance without
browser traffic, not a Raspberry Pi measurement. Logs use `out/bot-medium-`;
the runtime report is `out/bot-medium-runtime/mp_dawnville-tdm.json`.

Normal and Hard bots search nearby reachable cover when wounded or reloading.
Bots avoid firing into a teammate along their actual aim. SMGs close to a
shorter engagement range than rifles and machine guns; bots advance toward
visible distant opponents rather than permanently orbiting them. Short combat
dodges have pauses and do not move a bot out of a held capture objective.
Carrying a flag or holding use takes priority over combat strafing. They reload
empty magazines and switch to an owned loaded weapon when out of ammunition.
They also request an ordinary reload at or below one third of a magazine after a
quiet interval: no visible target, no active contact memory, at least 2.5 seconds
since last seeing an enemy, spare ammunition and an idle weapon. Ladders, drops,
flag runs and use interactions suppress this proactive reload. The original
engine still owns ammunition transfer and reload timing.

Cover selection examines at most two nearby candidates per bot attempt and
continues from a saved cursor. Occlusion is tested before expensive floor and
clearance validation. Cover visibility shares the 12-ray tactical allowance;
walk validation spends the existing navigation attempt/time allowance. Bots
deferred by a busy budget retry before those that already received work, so
fixed client ordering cannot monopolize cover checks. Active teammate cover
positions are reserved to reduce crowding behind the same obstacle.

## Objectives and navigation

CTF uses public flag/base/carrier positions for movement. HQ uses only the
currently announced radio. S&D uses the active bomb sites and planted bomb.
These positions do not bypass combat visibility checks. A human already using an
objective, or the currently interacting bot, retains that interaction while
other bots cover it. Actor selection has a 2.5-second lease during approach, so
small distance changes do not make two bots repeatedly exchange the task. Dead
players release their roles; survivors are ranked again every half second. CTF
limits the escort/intercept group and retains base security. HQ assigns up to
two bots to capture/neutralize while larger teams cover the entrances. Bots wait
to land before starting a bomb interaction, then hold ordinary use through the
original plant or defuse timer.

Actor elections are cached by team, duty and site group for 500 ms; teammates
reuse the result rather than each scanning the roster. Cached actors are
revalidated for death, team, group and flag carrying. HQ selects a nearby actor
from each of two groups instead of always sending the first two roster entries.
Tactical ranks include dead teammates while team strength counts only survivors:
a casualty releases its duty without changing every survivor’s parity, bomb
site or base-defense assignment. Joining, leaving or switching teams can still
change the roster order.
CTF selects one flag attacker, spreads the supporting bots around the approach,
and gives friendly-flag recovery priority over attacking. Flag carriers cannot
be assigned a competing interaction duty. The shared election checks progress
only when its 500 ms cache refreshes. Twelve seconds without improving the
approach by 48 units temporarily excludes the stalled actor for eight seconds;
actors within 192 units or already interacting retain their role. If no replacement
exists, the sole eligible bot keeps trying. Scoped weapon
roles incur a 192-unit election penalty, allowing nearby assault bots to interact
while scopes cover, without preventing a sniper from taking over when needed.

Teams receive a mix of SMGs, rifles, automatic support weapons and an original
scoped rifle for every four bots. Faction weapon callbacks still enforce the
server's restrictions. Weapon roles are assault (SMG), flanking (rifle), support
(automatic) and marksman (scope). Rifles approach a contact from an offset lane;
nearby assault and support bots can investigate a teammate's recent sighting.
Objectives and use holds take priority over these combat roles. Snipers favor
elevated positions with traversal exits, relocate between bounded observation
periods and wait for the normal scope transition before firing. A 96-slot spatial
catalogue collects high navigation landings with O(1) registration and no extra
geometry queries. Patrols sample this catalogue in addition to normal nodes;
objective overwatch also considers its positions within 768 units of the anchor.
The catalogue advertises candidates, not proven routes: normal A* and collision
still decide whether a bot can reach them.

Each previously unmeasured patrol candidate uses eight cached static rays:
four crouched sightlines and four short low-cover probes. A usable long lane
with cover and fewer exposed directions scores above an equally high open roof.
The winning direction anchors a bounded scan rather than accumulating turns.
Patrol snipers hold a reached firing position for 10–18 seconds, continue aiming
at visible targets, and do not abandon a committed route to chase distant contact
memory. Damage or a visible enemy within 224 units requests relocation, with a
five-second cooldown and a 15-second penalty around the compromised position.
Failed guard approaches also temporarily penalize the failed area. Objective
actors/carriers keep priority over this relocation behavior. Guard arrival uses
3D distance so standing under a balcony does not count as occupying it. Holding
a perch or guard position consumes no route-search admission. The catalogue is
192 bytes; cached facing reuses an existing alignment byte, preserving the
60-byte navigation node size. Three relocation integers per bot add 768 bytes
at the 64-client limit. There are no per-decision heap allocations. Snipers use
crouching and short breath-hold requests. DM/TDM patrols remember eight recently occupied areas and
penalize destinations already reserved by teammates. The map center has only a
small preference, so exploration does not continually collapse onto the same
central cells. Guards and escorts select separate reachable positions around
their objective and pause between scans instead of spinning continuously.

Navigation grows from actual collision, spawn points and objective positions.
The grid distinguishes floors and uses eight horizontal neighbors, allowing safe
diagonal routes through buildings and stairways. Failed links can refine locally
to 16-unit spacing to enter doors between the ordinary 32-unit grid lines.
Refined links merge back into the ordinary grid in open space. Arrival and
movement thresholds allow bots to finish these tighter links. Ground sampling
starts within step height to avoid accidentally selecting the ceiling or roof
above a room. Walk checks combine sampled floor height with standing-body
clearance. Original ladder collision materials create climb/descend links to
accessible upper floors and rooftops. Ordinary movement owns ladder attachment,
collision, climbing and falling; AI does not teleport or write player velocity.
Directed drop links let bots leave roofs and elevated rooms. They check a full
standing capsule at departure and landing, reject damaging falls, and can use a
normal jump pulse over a low parapet when headroom permits. These links use the
configured original jump height and never invent a reverse climb. General
obstacle/gap jumps and mantle routes remain unsupported.

A shared priority queue favors central combat lanes while weighting insertion
order so continuous central growth cannot starve older spawn/roof frontiers.
Local scouting can extend up to six cells toward an objective, including a short
detour around a wall. Spawn seeding, map expansion and all bots' scouts share at
most 128 attempts and a four-millisecond work budget per server frame. Spawn
collision checks are incremental too. Background growth reserves part of that
budget for local work. Time is checked between operations, so one collision
operation may exceed it on a slow host; this is not a latency guarantee. The
spawn envelope plus a 512-unit margin bounds exploration. Failed static links
are cached rather than repeatedly tracing the same wall.

Routing uses A* with geometric movement costs and a fixed binary heap.
Generation stamps avoid clearing the entire graph for each search. All bots
share a maximum of two searches and a two-millisecond search budget per frame,
checked every 32 node expansions; each search expands at most 2,048 nodes.
Requests are serviced in round-robin client order. Deferred bots keep their
current route and retry on a later frame. Searches
share fixed buffers, and bots cache up to 64 route nodes. Valid routes survive
the next decision tick: only a changed purpose, a destination that moved at
least 96 units, exhaustion or a blocked route requires a new search. Flat
collinear links reuse floor checks and need one shortcut capsule trace. Turns,
stairs and special traversals retain their individual nodes. Flat link
construction batches collinear body sweeps into one trace while retaining every
ground sample; stairs keep piecewise body traces. Patrol destinations survive
partial routes for up to 20 seconds, allowing bots to leave spawn components as
the graph grows.

An admitted search applies a soft cost to the next 24 route cells reserved by
living teammates. It favors another short lane when available, while retaining
the only passage when necessary. Reservations use 160 KiB of fixed shared
storage, generation stamps and one constant-time lookup per expanded edge;
there is no per-edge roster scan or collision query. DM and enemy routes never
contribute reservations. Valid cached routes still cost no new search.

Guard positions penalize nearby reserved guard slots and living human teammate
positions, then evaluate only two
candidates for objective visibility, an outward firing lane and low cover.
These checks and previously uncached sniper lookout checks share a total of
12 tactical rays per simulation tick across all bots. Guard assignments last
about three seconds (eight for scoped overwatch); if the budget is busy, bots retain their current position.
This cap covers position evaluation, not combat visibility or normal movement
collision, which still run as before.

Immediate collision checks and local steering help bots pass corners and
teammates. Progress toward the active waypoint is checked separately from
displacement, so moving in a small circle still triggers recovery. A blocked
route receives a per-bot eight-second edge exclusion; repeated failures trigger
a six-second exploration detour before retrying the strategic objective.
Recovery also checks route progress during combat when the bot is actually
following its route; deliberately stopping to shoot is excluded. A detour
discards the failed cached patrol/perch destination. When normal movement
collision identifies a blocking teammate, a stable priority lets one bot back
away for up to 700 ms. Humans and objective users/carriers take precedence,
then client number breaks ties. Yielding reuses the collision result and normal
movement checks, with no additional traces or player-position writes.

Stationary guards can also step aside before a human collides with them. A
shared human roster refreshes every 500 ms; individual decisions inspect only
that roster. A nearby teammate approaching the bot's lane (or already blocked
and facing it) triggers at most one clearance test, within the shared tactical
budget. Different floors, walls, enemies and critical objective interactions
are excluded. The bot gives lateral room for 700 ms, tries the other side if
blocked, and permits its guard slot to be reassigned around the human. It keeps
its objective, releases scoped/crouched firing while yielding and uses normal
collision-checked movement. Paused test clients are not classified as humans.
This cooperation and incremental-cover state add less than 1 KiB of fixed
storage for 64 slots; there are no new allocations or navigation-node fields.
Disconnected objective projections also trigger recovery, including repeated
arrivals at the same nearby cell outside a reachable route. Partial routes that
still approach the objective during graph warm-up keep their objective; only
consecutive decisions without progress count as failures. Announced model
objectives such as radios are projected onto their walking floor; their model
center cannot silently choose an unreachable roof instead. Path decisions are
revisited every 1.2 seconds, with prompt advancement between cached waypoints
and earlier replanning when the route fails. Final objective approach checks are
cached for 400 ms; immediate movement collision remains checked every tick.
Navigation and command generation allocate no per-frame heap memory.

Movement and aim commands run once per original 50 ms simulation tick,
including each tick of a delayed-frame catch-up. One shared objective scheduler
assigns five client phases across the original 50 ms ticks, preserving the
250/500/750 ms difficulty intervals. Its roster contains client numbers; active
entities are resolved only during a decision and released before waiting.
Perception preserves the difficulty view cone and smoke checks; a rotating third sightline candidate
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
general jump/mantle route planner, tactical grenade planner or mounted-gun
strategy. Complex routes, coordination and stalemates still warrant playtesting.

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

Round restarts reuse compiled scripts while creating fresh runtime state.
Suspended frames retain their exact local-variable order, and variable cleanup
preserves shared references when hash collisions or notification cancellation
move sibling entries. The original S&D script can restore an empty secondary
weapon slot from its `"none"` value, without raising an invalid-weapon error.

## Validation

The normal static suite includes the production bot controller and regression
tests for route progress, blocked-route alternatives, guard spacing, ladder
links/input, frontier fairness, offset doorways, tight arrival radii, shared
32-bot work limits and fair admission, stable routes, moving destinations,
circular-motion recovery, expiring team contact reports, bounded A* searches,
soft route reservations and the single-passage fallback, useful guard firing
lanes, the shared 32-bot tactical trace cap, reinforcement leases, yielding and
combat route recovery,
directed roof exits and fall rejection, warm-up time limits, jump tracking,
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

Add `--roof-exits` when generating to also probe elevated escapes on Toujane,
Carentan and three authored Dawnville spawns. They require a safe, grounded
exit without damage; objective completion has its own scenario above. Require
the expected pass count so a stalled or unfinished scenario cannot pass silently:

```sh
python3 scripts/prepare-bot-navigation-qa.py --roof-exits
python3 scripts/test-bot-runtime.py --map mp_dawnville --mode tdm \
  --bots 8 --seconds 140 --expect-passes 3 \
  --policy out/bot-navigation-controlled.gsc --output out/bot-roof-final
```

Use one expected pass for Carentan; Toujane has three (ladder ascent, descent,
roof escape) and uses a 180-second observation. These controlled cases test
specific routes, not every roof in each map. The runner recognizes failures
from both objective and navigation scenarios.

Separately observe autonomous matches on Toujane, Carentan and Dawnville: check that bots
reach objectives, score, change roles, obey round transitions and fight under
the selected difficulty. Include a real browser client when measuring network
latency; bots do not consume browser snapshot traffic or client rendering.

## Reproduce routing and runtime measurements

```sh
# Production C controller, large-graph routing benchmark and sanitizers
python3 scripts/test-bot-ai.py --benchmark --sanitize

# Build the current native server, then run an isolated 16-bot match
bash scripts/build-server.sh
python3 scripts/test-bot-runtime.py --map mp_carentan --mode tdm --bots 16 --seconds 120
```

The runtime check creates and removes its own network-isolated Docker container.
It does not alter running rooms. JSON and full logs go to `out/bot-runtime/`.
The report includes CPU as a percentage of one core, sampled map coverage per
bot, objective intents and cumulative routing work. CPU includes navigation
warm-up and normal combat; this has no human browser, snapshot-stream or
rendering load. Five-second position samples are diagnostic, not a guarantee
that every map route is reachable. Use `--binary`, `--policy` and `--output` for
before/after runs or the generated controlled objective scenarios.

Before integration with the Pi server performance changes, October 11, 2026
local validation ran autonomous 16-bot TDM matches on
Carentan, Toujane and Dawnville, plus a DM smoke match. Controlled CTF checks
completed pickup/capture; HQ capture and neutralization each took 9.5 seconds;
S&D plant and defuse took 5.5 and 10.5 seconds and the next round started.
These interaction scenarios arrange initial positions, so they do not certify
autonomous objective completion from every spawn.

That Carentan build ran for 120 seconds with 16 Normal bots, using 73.1%
of one CPU core in the local amd64 Docker runtime. Position samples covered
1–12 coarse areas per bot and the teams scored 4–3. A prior-version comparison
used 90.8% of one core, but had different movement and combat activity; this
is not a controlled claim of a fixed CPU improvement. The synthetic routing
benchmark (1,000 searches on approximately 9,200 nodes, with ASan/UBSan) took
about 0.03 ms per search and expanded 96 nodes per diagonal route. Native CPU,
synthetic path time and browser latency are different measurements. This
pre-integration revision was not load-tested with human browser clients on the Pi.


## Raspberry Pi deployment validation

Run the same isolated match on the Pi with its ARM64 server image and i386
engine closure (the runner invokes the image's QEMU interpreter):

```sh
python3 scripts/test-bot-runtime.py --runtime pi \
  --image local/cod2-pi-server:dev \
  --binary out/pi-runtime/usr/local/bin/cod2_lnxded \
  --map mp_carentan --mode tdm --bots 32 --difficulty 2 --seconds 90
```

`--assets` and `--config` can point to an existing private installation. Results
also include simulation ticks per second and the highest sampled bot/game work
from the engine's aggregate profiler. These are server simulation measurements;
they do not measure browser FPS or network latency with human clients.


On October 11, 2026 the integrated controller was exercised on the Raspberry Pi
at `192.168.1.28`, with 32 Hard bots per isolated TDM match:

| Map | Measurement | Simulation ticks/s | CPU, one core | Final score |
| --- | ---: | ---: | ---: | --- |
| Carentan | 90 s | 20.0 | 48.11% | 8–7 |
| Toujane | 60 s | 20.0 | 45.42% | 17–15 |
| Dawnville | 60 s | 20.0 | 51.63% | 10–5 |

The existing `ed9f47c` engine used 53.26% of one core in a separate 90-second
Carentan reference match. Combat and spawn evolution differ between runs, so
this is not a fixed percentage performance guarantee. The Pi reported no
thermal/voltage throttling during these checks. Existing web/gateway services
and their sessions remained active while isolated QA containers ran.


Controlled Pi checks passed original CTF pickup/capture, HQ capture and
neutralization (9.5 seconds each), and two successive S&D rounds with plant
(5.5 seconds) and defuse (10.5 seconds). No engine/script errors were reported.
These scenarios arrange actors near the objective and verify interaction rules
and round lifecycle, not autonomous route completion from every spawn.


## Local coordination and performance validation — 2026-10-11

This refinement was tested locally; it was not deployed to or benchmarked on
the Raspberry Pi while it was in use. Final-source native and WebAssembly
compilation passed. Production-controller tests passed under ASan/UBSan,
including shared tactical budgets, route congestion, single-passage fallback,
contact leases, objective/combat recovery and collision-driven yielding.

Local native Docker matches with 32 Hard bots measured:

| Map / mode | Duration | Simulation ticks/s | CPU, one core | Max single-bot AI call |
| --- | ---: | ---: | ---: | ---: |
| Carentan / TDM | 60 s | 20.0 | 29.70% | 11 ms |
| Toujane / CTF | 70 s | 20.0 | 20.07% | 4 ms |
| Dawnville / DM | 60 s | 20.0 | 18.82% | 7 ms |
| Carentan / HQ, separate run | 60 s | 20.0 | 31.26% | 4 ms |

These are local averages including navigation warm-up, without browser traffic;
they are not Raspberry Pi measurements or a fixed performance improvement.
HQ also showed a 151 ms general game-frame peak during player startup, before
the first announced radio. A separate 30-second local reference run, rebuilding
the HEAD bot controller and using its original policy with the same remaining
engine objects, also showed a 148 ms general game-frame peak. This startup
hitch is therefore also present without these AI changes. Average tick rate
does not guarantee hitch-free frames. The earlier HQ match reached ownership
and scored; the separate repeat still had a neutral radio. The short autonomous
CTF run did not produce a
capture, so mode intent and movement should not be mistaken for completion.

Controlled eight-bot checks passed original flag pickup/capture, HQ capture and
neutralization (9.5 s each), and bomb plant/defuse (5.5 / 10.5 s). As above, these
arrange actors near objectives and verify rules rather than every map route.
The sanitizer routing benchmark took approximately 0.03 ms/search on 9,216
connected cells, or 0.06 ms/search with 31 peers reserving one lane; the latter
expanded 262 nodes/search and used 160 KiB of fixed reservation storage.

The general static suite passed up to `test-script-local-archives.py`, whose
existing 32-bit pointer-layout assertion fails with this Mac's 64-bit compiler.
An i386 ASan attempt in Docker timed out. The remaining suite and final browser
package checks passed separately; the complete suite is not claimed green.
Local reports are in `out/bot-tactics-final/`, `out/bot-tactics-final-solo/`,
`out/bot-tactics-baseline/runtime/` and `out/bot-tactics-controlled/`; test/build
logs use the `out/bot-tactics-` prefix.

## Strategic sniper and role continuity validation — 2026-10-11

All work in this pass ran locally; the Raspberry Pi was neither accessed nor
restarted. The final native controller and WebAssembly client compiled. The
production-controller harness passed ASan/UBSan, including protected versus
exposed sightlines, shared ray limits, elevated objective positions, committed
sniper routes, crouched holding, floor separation, damage/close-threat relocation,
cover/interaction priority, failed-slot rejection and zero-search holding. The
node-size assertion preserves the existing 60-byte node. The routing benchmark
remained around 0.03 ms/search, or 0.06 ms with 31 route reservations.

Sequential local Docker matches (32 Hard bots):

| Version / map / mode | Duration | Ticks/s | CPU, one core | Max single-bot AI call |
| --- | ---: | ---: | ---: | ---: |
| Before this pass / Toujane / CTF | 120 s | 20.0 | 30.07% | 9 ms |
| Updated / Toujane / CTF | 120 s | 20.0 | 28.83% | 8 ms |
| Updated controller / Dawnville / DM | 60 s | 20.0 | 28.63% | 7 ms |

These are individual stochastic matches, not a statistically established speedup
or Pi capacity estimate. General game-frame peaks in the CTF pair were 36 and
48 ms; average tick rate alone does not establish hitch-free execution. The DM
run preceded the final script-only stable-rank correction, which applies to
team assignments.

The final CTF log records two scoped bots stationary and crouched on elevated
positions, 264–265 units above the lowest spawn reference. One stayed at
(2109, 1199, 256) across three consecutive five-second samples. The catalogue
reached 74 occupied slots. This verifies actual arrival and holding in that run,
not that every candidate is reachable or every sniper reliably finds a post.
No holding sample appeared during the shorter Dawnville DM run.

A 120-second eight-bot navigation-only run from real spawns captured a flag.
An intermediate 32-bot combat run also captured once; both the reference and
final 120-second combat repeats ended CTF 0–0. Completion is therefore still
inconsistent, and these checks do not establish an 8/10 or 9/10 gameplay rating.

Controlled engine scenarios additionally validate survivor rank continuity after
a teammate's death, shared actor elections, stalled-actor handoff, progress lease
retention and the sole-actor fallback, followed by original flag pickup/capture.
HQ capture and destruction both retained their original 9.5-second observed
completion; S&D planted in 5.5 seconds and defused in 10.5 seconds. The bot
foundation, native menu and room/map integration checks also passed. These
arranged scenarios test policy and original rules separately from autonomous
map traversal.

Reports and logs: `out/bot-strategy-baseline/`, `out/bot-strategy-final/`,
`out/bot-strategy-controlled/`, and the `out/bot-strategy-` build/test logs.
The earlier full-suite host ABI limitation above remains; this pass does not
claim a completely green full static suite.

## Elevated spawn escape correction — 2026-10-11

The native controller called `Dvar_GetFloat` without a visible float-return
declaration. Its implicit integer-return ABI misread jump/fall limits and could
disable safe drop discovery. The declaration is now explicit. The test stub
is defined after production code so it cannot hide this integration error;
removing the declaration causes the harness to fail compilation.

When A* exhausts a disconnected component, it now approaches an unexplored,
reachable exit instead of repeatedly selecting the roof edge closest to the
ground-level objective. For a lower objective, it prefers a descending frontier
even when that initially increases horizontal distance. It reuses the existing
search parents and shared navigation allowance. Roof-first discovery also checks original ladder material
from below and validates both landings. A bot already attached to a verified
ladder can resume its climb past a floor approach it has already left.

No additional navigation arrays or per-frame graph scans were introduced.
The shared limits remain 128 navigation attempts / 4 ms, two path searches /
2 ms, and 12 tactical rays per frame. Geometry is cached; the attachment
recovery needs no collision traces. Limits are checked between work units,
so they do not guarantee that every individual AI call finishes within 4 ms.

The initial local native reference failed two of three Dawnville elevated-spawn
escapes after 120 seconds, and Carentan also timed out. The final Dawnville
repeat landed safely in 2, 51.5 and 6.5 seconds. The sloped-roof case remains
slow; this is not proof that all wandering has disappeared. Carentan passed
without damage, and Toujane passed ascent, descent and safe roof exit. These fixtures use teleport
only to arrange the initial position; traversal uses normal movement and
original collision/damage rules. They do not establish universal map coverage.

Native i386 and WebAssembly builds passed, as did ASan/UBSan navigation tests,
the bot foundation contract and simulation cadence checks. The synthetic
benchmark remained about 0.024 ms/search, or 0.059 ms/search with 31 teammates
reserving the same lane. All tests in this correction ran locally; the Raspberry
Pi and its running match were not accessed.

Sequential local native Dawnville TDM matches, 32 Hard bots, 60 seconds each:

| Controller | Ticks/s | CPU, one core | Max single-bot AI call | Max game frame |
| --- | ---: | ---: | ---: | ---: |
| Before rooftop correction | 20.0 | 28.54% | 4 ms | 39 ms |
| Final rooftop correction | 20.0 | 29.07% | 5 ms | 36 ms |

These include navigation warm-up and ordinary combat; controlled scenarios and
builds were stopped before measuring. They are individual stochastic local
matches, not a statistically established performance change or Pi measurement.
The final TDM score was 20–24. Reports are in
`out/bot-roof-performance-baseline/` and `out/bot-roof-performance-complete/`.

Reports: `out/bot-roof-baseline/`, `out/bot-roof-final/`,
`out/bot-roof-complete/`, `out/bot-roof-descent/` and `out/bot-roof-` build/test logs.

## Human cooperation and bounded cover validation — 2026-10-11

This pass adds human right of way while guarding, occupancy-aware guard
assignments, incremental shared-budget cover checks and quiet-interval reloads.
It preserves the Normal aiming adjustment above. Production-controller tests
passed ASan/UBSan, including stationary guards yielding, an obstructed sidestep
using the opposite side, wall/floor/interaction exclusions, human-occupied guard
slots, all 32 wounded clients eventually receiving cover work, exhausted-budget
deferral and safe reload exclusions. Human cooperation was verified in the
controller harness; these checks are not a live human/browser playtest.

Native i386 and WebAssembly builds, simulation cadence and bot foundation
checks passed. The controlled native Carentan HQ scenario completed both
capture and destruction in 9.5 seconds, preserving original timing.

Sequential local Dawnville TDM measurements, 32 Normal bots, 90 seconds each:

| Controller | Ticks/s | CPU, one core | Max single-bot AI call | Max game frame |
| --- | ---: | ---: | ---: | ---: |
| Before cooperation/cover changes | 20.0 | 28.99% | 14 ms | 37 ms |
| Updated | 20.0 | 28.95% | 2 ms | 37 ms |

These are individual stochastic matches including navigation warm-up; the
similar average CPU and smaller sampled AI peak do not establish a universal
speedup. They exclude human browser/network traffic. The Raspberry Pi was not
accessed or changed. Reports: `out/bot-coop-performance-baseline/`,
`out/bot-coop-performance-final/`, `out/bot-coop-controlled/` and
`out/bot-coop-` build/test logs.
