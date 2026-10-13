# Call of Duty 2 Multiplayer — in your browser

A WebAssembly / WebGL 2 port of the reconstructed CoD2 IW 2.0 engine, with a
native dedicated server and a WebSocket-to-UDP gateway. Each browser runs its
own game client; the host runs the authoritative multiplayer simulation.

`main` is the sole maintained branch and the GitHub default. Updates and the
Raspberry Pi deployment follow `origin/main`. Production runs at **30 ticks per
second**; see the [Pi tick-rate measurements](docs/TICKRATE_BENCHMARK_2026-10-12.md)
for the tested loads and limits.

Play on a LAN using the original maps, models, textures, animations, sounds and
menus. The project is under active development. Combat, respawn, room creation
and map rotation have been exercised on the supported maps. An eight-client Carentan session has also been measured on a
Raspberry Pi 5; larger matches and lower-end client hardware need broader testing.

## Maps and game modes

| Map | Allies | Axis |
| --- | --- | --- |
| Carentan, France (`mp_carentan`) | American | German Normandy |
| Dawnville, France (`mp_dawnville`) | American | German Normandy |
| Toujane, Tunisia (`mp_toujane`) | British | Afrika Korps |

All three maps support the original **Deathmatch, Team Deathmatch, Capture the Flag,
Headquarters, and Search and Destroy** modes. Select a map and Game Type in
**Start New Server**. Click Game Type to cycle forward; right-click to go back.
Rooms rotate through Toujane, Carentan and Dawnville while keeping their selected mode. TDM uses
100 team points or 15 minutes; the other modes use their original scripts and
settings.

The host starts with **no rooms**. Users create their own named servers, with
up to three concurrent rooms and 64 configured player slots per room. An empty
room closes after five minutes. **Join Game → Delete my server** lets its owner
remove it immediately. Ownership is stored in that browser and checked with a
private token; changing browsers or clearing site storage loses that record.
The slot limit is configuration, not a claim that 64-player matches have been
load-tested.

## Bots

In **Start New Server**, choose **Bots** (None, 1–16, 20, 24, 28 or 32) and **Bot Difficulty**
(Easy, Normal or Hard), alongside the room name, map and Game Type. Left-click
cycles forward; right-click cycles backward. The default is **None**. Settings
belong to that room and survive map rotation.

| Difficulty | Reaction delay | Aim and turning |
| --- | --- | --- |
| Easy | 700 ms | Slower turns and wider aiming error |
| Normal | 400 ms | Balanced turns and accuracy |
| Hard | 200 ms | Faster turns and smaller aiming error |

Bots are server-controlled players, marked **[BOT]** in their names. They join
teams through the original gametype callbacks, choose faction-appropriate
SMGs, rifles, automatic weapons and scoped sniper rifles, patrol navigable routes, acquire visible enemies,
shoot, reload and respawn under the mode's rules. Walls, other players and
smoke block target visibility. Health, ammunition, recoil and weapon cadence
use the same gameplay rules as human players.
DM/TDM bot scheduling is independent of the objective-roster refresh deadline,
so the 30 Hz server cannot enter an uninitialized objective list when a script
wait finishes early.

| Game Type | Bot objectives |
| --- | --- |
| Deathmatch | Patrol and fight other players, including other bots. |
| Team Deathmatch | Spread across patrol areas, share recent visible contacts, flank and engage at weapon-appropriate ranges. |
| Capture the Flag | Attack the enemy flag, return it home, recover dropped friendly flags, intercept enemy carriers, escort friendly carriers and guard the base. |
| Headquarters | Approach the announced radio, capture or neutralize it, then defend it under the original respawn rules. |
| Search and Destroy | Split between bomb sites, select a planter or defuser and have teammates cover them; defend sites and planted bombs. |

Planting and defusing use ordinary held-use input and the original timers;
bots do not award themselves objectives or bypass round rules. A bot already
interacting, or a human interacting nearby, keeps that role while teammates
cover. Difficulty also controls sight angle, bursts and how often objectives
are reconsidered. Normal and Hard bots seek nearby cover when wounded or
reloading. Snipers favor elevated, sheltered firing lanes, hold their position
instead of chasing distant targets, and relocate when compromised. Stalled
objective approaches can hand their role to another bot. Lost enemies are remembered only at their last visible position.

Navigation is built from map collision, spawn locations and announced
objectives, with bounded work per server frame and shared spatial lookups.
Cached A* routes, local doorway refinement, route-progress checks and temporary
blocked-edge exclusions improve progress. Navigation and search budgets are
shared across all bots; patrols remember visited areas and spread their destinations.
Route reservations favor less crowded lanes, and blocked teammates use a stable
priority to yield passage. Guards evaluate firing lanes and low cover within a
shared tactical trace budget. Original ladder materials connect accessible
rooftops. Directed, collision-checked drops and low-parapet jumps provide
exits from supported elevated areas; scoped bots seek lookout positions and wait to finish aiming.
This is first-party multiplayer AI, not the retail campaign AI. Complex routes
and tactical coordination can still need improvements.
See [Bot behavior and validation](docs/BOT_AI.md) for the implementation and
the distinction between autonomous matches and controlled interaction tests.

Bots occupy ordinary player slots: eight bots leave 56 human slots, and 32 bots leave 32. They do not
keep a room alive after its last human leaves; the usual five-minute idle
cleanup still applies. No browser or rendering process is needed for each bot.
For a custom room creator, POST `/servers` with `botCount` (integer 0–32) and
`botDifficulty` (`easy`, `normal`, `hard`); omitted fields retain the defaults.

## Additions beyond the original game

These are intentional changes, rather than features claimed to be part of
retail CoD2:

- **Configurable multiplayer bots:** first-party server AI with per-room quantity
  and difficulty settings, objectives for all five modes, cover, normal weapon
  rules and collision-based navigation.
- **Sprint:** Shift + W increases movement speed, lowers and swings the weapon,
  and uses the running body animation. It also stands up from crouch/prone when
  headroom permits. Sprint currently has no stamina limit.
  Melee moves to V; scoped Hold Breath continues to use Shift.
- **Melee damage:** butt strikes deal at most 50 damage without random bonuses
  or bullet hit-location multipliers. Two hits kill a player with 100 health;
  an injured player can still die from one. Weaker melee weapons retain their
  original damage. This server rule leaves the retail weapon assets intact.
- **Mounted MG42 overheating:** five seconds of continuous fire fills the heat
  bar, followed by eight seconds of cooling. Heat belongs to the gun and is
  synchronized with its user. Mounted aim also kicks upward and sideways, with
  stronger recoil during sustained bursts.
- **Custom LAN weapon balance:** `downstream/weapon-balance.json` retains retail
  close body damage, cadence, magazines and reloads, reduces automatic headshot
  multipliers from ×3 to ×2, shortens SMG full-damage range, and adjusts ranged
  damage and hip-fire dispersion for Bren/MP44. Rifles, scopes, pistols, shotgun,
  grenades and mounted MG42 retain their original weapon definitions; the
  mounted heat/recoil rules are implemented separately.
- **Larger visual effects:** brief muzzle sprites are 40% larger with stronger
  color/alpha; frag fire, smoke and dust are 35% larger. Original effect textures,
  particle counts, lifetimes and grenade damage remain unchanged.
- **Smoother baked shadows:** sun visibility uses all four source samples when
  preparing each lightmap texel at map load. Shadow edges have less aliasing,
  with the same uploaded texture size and no additional gameplay render passes.
- **Browser hosting and controls:** create/delete rooms without a desktop game
  installation on each client, cached asset downloads with actual byte progress,
  English interface text, update notices, compact chat, and an explicit
  fullscreen button/F10. Change your gametag in **Options → Game Options**.
  The native profile saves the name, key bindings, mouse, sound and graphics
  settings in this browser, including across reloads and game updates. Browser
  storage must be enabled; clearing site data removes the saved profile.
- **Browser menus and video settings:** the main menu omits profile selection,
  Multiplayer Options and the online-play notice; settings omit voice chat.
  The menu version follows the deployed Git revision. Applying graphics
  settings releases renderer resources with the correct WebAssembly signature
  and resizes the canvas, including while connected to a match.
- **First-person weapon effects:** live players use the viewmodel muzzle for
  flashes, brass and tracers. BAR muzzle tails transform to world space once;
  ADS raise/lower poses follow the predicted aiming fraction without an extra
  half-second crossfade that can leave sights tilted during transitions.
- **Remote player poses:** other players see vertical aim in the correct
  direction and Q/E as a lateral lean. Controllers rotate in model axes after
  skeletal composition, preserving movement animations. The respawn/teleport
  toggle no longer suppresses a living player's aim or lean.
- **Pi server collision performance:** immutable map brushes use cached bounds
  for consecutive groups, preserving collision order, capsule dimensions and
  contact tolerances. A Pi 5 sample with eight protocol clients and 16 Hard bots
  sustained 30.2 updates/s/client at 86.8% of one core, versus 29.2 updates/s and
  93.9% before. This is a measured sample, not a maximum-player guarantee; see
  [the performance report](docs/SERVER_PERFORMANCE_2026-10-13.md).
- **Bounded cleanup and respawn feedback:** bodies remain for eight seconds in
  an eight-body pool, dropped weapons/ammunition expire after twenty seconds,
  and TDM uses a three-second respawn countdown.
- **Movement conveniences:** C toggles crouch. Space first stands up from crouch
  or prone, including when the stance key is held; another press while standing
  jumps. Ordinary ground movement stops
  promptly when movement keys are released. Holding Space does not automatically
  jump again on landing. Steps require a clear path and a walkable landing;
  recovery from a small collision overlap does not inflict fall damage.
  Prone movement crawls at 15% of standing speed with the original forward
  and backward crawling animations. Mantling requires a fresh press, checks its
  path for obstacles, and stops safely if that path becomes blocked.
- **Fall damage and blood feedback:** falls above 256 game units progressively
  deal damage, reaching a lethal fall at 480 units. The thresholds are controlled
  by `bg_fallDamageMinHeight` and `bg_fallDamageMaxHeight`. Damage scales from
  maximum health, so the same fall remains dangerous when already wounded.
  Damaging falls and other hits briefly flash the original blood overlay, fading
  over 650 ms, with the existing low-health pulse retained.

## Original behavior restored in the port

- Semi-automatic weapons fire once per valid trigger press. Rapid clicks cannot
  bypass fire, bolt, raise or reload timers. Original magazines and weapon-specific
  ADS/reload timing are preserved. The M1 Garand reloads only after its eight-round
  clip is empty; the Lee-Enfield needs space for a complete five-round charger.
- Stale weapon selections after a respawn or loadout change recover to an owned
  gun, preferring the primary slots. Removed weapons, grenades and binoculars
  cannot leave a newly armed player waiting on an invalid weapon selection.
- Reloading returns to held/toggled aiming unless the player cancels aim.
  Switching weapons or striking cancels the current reload without adding
  ammunition; segmented reloads retain only completed insertions.
  Scoped Hold Breath lasts at most 4.5 seconds, with 5.5 seconds of recovery
  after exhaustion.
- Frag grenades have their original 3.5-second fuse, can be cooked in the hand,
  bounce off players, and can be picked up and returned with G while preserving
  the remaining fuse. Holding one too long kills its holder. Nearby live frags
  show the original icon and directional warning inside their blast radius,
  growing with proximity;
  cover can still block splash damage.
- Blast shellshock keeps the first-person weapon visible. Killcam samples use
  actual archive timestamps, including when the server tick rate changes.
- The player console is disabled, including its toggle command and saved bindings.
- Hits show the original red directional damage arc and low-health blood overlay.
  Teammate names remain visible without aiming at them. Kill notifications reach
  all players/spectators and expire normally.
- Original muzzle FX lights illuminate nearby floors, walls and opaque models,
  including during scoped shots. Their authored colors, radii and lifetimes are
  retained. Scopes stay clear of muzzle sprites and smoke.
- Other players use lateral/diagonal movement animations and blended stance
  transitions. Separate corpse bodies survive respawning, and dropped weapons
  remain available for pickup until their cleanup deadline.
- Original audio, tracers, wall impacts, bullet marks, map props, ladders and
  killcam are available. Smoke effects and sustained mounted-MG firing have
  dedicated crash regressions. Collision fixes cover angled walls and finite
  mesh corners on both maps.

## Controls

Defaults can be changed in **Options → Controls**.

| Action | Default |
| --- | --- |
| Move / lean | W A S D / Q E |
| Fire / aim | Left / right mouse button |
| Sprint / scoped Hold Breath | Shift; sprint while moving forward |
| Melee / interact / reload | V / F / R |
| Frag / smoke | G / 4 |
| Cook or return a frag | Hold G, then release to throw |
| Toggle crouch / go prone | C / Ctrl |
| Stand up or jump | Space |
| Public / team chat | T / Y |
| Send / cancel chat | Enter / Escape |
| Quick messages | H |
| Enter / exit fullscreen | Top-right button or F10 |

Chat keeps mouse capture while typing and displays five wrapped lines for eight
seconds by default. Clicking the canvas does not change fullscreen mode.

## Host locally

### Requirements

- Git, Python 3.9+, Node.js, and Docker with Compose.
- A local copy of the owner's original **CoD2 1.3 English IWD files**, including
  `localized_english_*.iwd`, placed in ignored `data/main/`.
- The original MP executable/installation ZIP for extracting the favicon, or an
  already prepared `data/browser/web/cod2.ico`. The startup logo is extracted
  from `data/main/iw_09.iwd`.
- The sibling [wasm-game-framework](https://github.com/theodorecharles/wasm-game-framework)
  checkout. Builds pin version **0.9.2**, commit
  `53bc7e6eeef1ae35dcf3b25dea4e3ec0ab46726f`.

The standard dedicated server uses a **32-bit x86 Linux binary** inside an amd64
container. Docker Desktop handles its execution on Apple Silicon. The browser
client itself runs on the player's machine, independent of the host CPU.

### Build and start

```sh
git clone https://github.com/tonga54/cod2-wasm.git
cd cod2-wasm
git clone https://github.com/theodorecharles/wasm-game-framework.git ../wasm-game-framework

# Import your original IWD files into data/main/ first.
python3 scripts/prepare-browser-bootstrap.py
python3 scripts/check-private-assets.py

# Set COD2_ORIGINAL_ZIP if your original installation ZIP is elsewhere.
./scripts/build-docker.sh
docker compose up -d
```

Emscripten **3.1.64** runs in Docker if it is not installed locally. Set
`COD2_BUILD_JOBS` to control build parallelism and `COD2_WASM_FRAMEWORK_DIR` to
use another framework checkout location.

Open **http://localhost:8088/** on the host, or **http://HOST_LAN_IP:8088/** from
another computer on the same network. Use **Start New Server** to create a room,
then **Join Game** on the other browsers. The server does not create matches at
startup.

### Services and private data

| Service | Responsibility |
| --- | --- |
| `cod2-web` | Framework shell, compiled browser client, private asset delivery |
| `cod2-gateway` | HTTP/WebSocket entry point on port 8088, room discovery/ownership, UDP forwarding |
| `cod2-server` | Internal room supervisor and one authoritative engine process per room |

The game network is internal to Docker; raw UDP and the room supervisor are not
published to the LAN. Private files are mounted read-only. Set `COD2_WEB_PORT`
to change the public port or `COD2_WEB_DATA_DIR` to use another private-data
location.

The prepared two-map subset is approximately **275.6 MB**, including 518 original
sound files. Browsers download it once per site/cache version and keep it in
IndexedDB. Size/SHA-256 checks verify each archive. Original owner IWDs stay
untouched in `data/main/`; the prepared copies stay in `data/browser/`. Original
assets are excluded from Git and image layers. Generated WASM binaries are ignored by Git and shipped
in the web runtime image.

### Raspberry Pi

The [Raspberry Pi installation guide](docs/RASPBERRY_PI.md) covers a fresh host,
the Pi 5 kernel requirement, building on your computer, deployment over SSH,
updates and troubleshooting. The repository includes a dedicated
[`Dockerfile.pi`](downstream/server/Dockerfile.pi),
[`compose.pi.yaml`](compose.pi.yaml), and deployment/runtime preparation scripts.
Deployments preserve an existing `~/cod2-edge/compose.security.yaml` overlay for
HTTPS, password protection and request limits. Use the configured HTTPS address
on secured installations; credentials and certificates stay outside the repository.
The tested host is a Pi 5 with 8 GB RAM and 64-bit Debian; the required host page
size is 4 KiB. The web/gateway/supervisor run as ARM64 processes;
only the 32-bit x86 game executable uses QEMU. This preserves the engine's
32-bit layouts without claiming a native ARM engine port. Measure the host
under your expected player/room load before increasing it.

After preparing your own original assets, build on your computer and deploy:

```sh
DOCKER_DEFAULT_PLATFORM=linux/arm64 ./scripts/build-docker.sh
./scripts/deploy-pi.sh USER@PI_LAN_IP
```

Open **http://PI_LAN_IP:8088/** from another computer on the same LAN. The Pi
starts automatically after reboot, with users creating their own rooms. Follow
the full guide before running these commands on a new installation.

## Performance and verification

The default client renders at **1280×720**, with full-resolution textures,
anti-aliasing and 4× anisotropic filtering where supported. Rendering follows
the display cadence, including 120 Hz. Performance changes cache static vertex
buffers, texture/sampler/render state, shader uniforms and animation work;
they do not reduce texture quality or remove map geometry/effects. Up to four
FX point lights are combined into existing opaque geometry passes. Their base
texture sample is shared with the material, and authored model normals prevent
flashes from illuminating the back side. Partial texture mip chains use their
existing final level instead of sampling black; no mip generation, additional
lights, geometry, resolution or post-processing passes are introduced. The browser
uses Emscripten's fixed-function-only GL path to avoid restoring an unused
programmable pipeline between draws; shader attributes, lighting and texture
filtering remain intact. Static model colors and index buffers also stay on the
GPU between draws: exact mesh ranges are cached, and authored indices retain
all of their vertices. Writes, layout changes and map teardown refresh or release
the cached data. Dynamic/animated geometry keeps its original streaming path;
its color conversion now needs one pass instead of two, preserving every channel
and alpha byte. Texture bindings and common uniform calls avoid temporary
strings/argument arrays. Four-light uniform updates preserve unrelated material
caches; all authored light values still reach the GPU.

The authoritative simulation is configured at **30 Hz** (33 ms simulation
steps), with clients requesting **30 snapshots/s**, up to **60 input packets/s**
and a 25 KB/s client rate. Actual snapshot delivery remains subject to bandwidth
limits and server load. The higher tick rate increases simulation work; validate
CPU and packet cadence with the intended player count before deployment. Remote motion
interpolation uses
sample timestamps/velocities, smooths sample boundaries and allows bounded
50 ms extrapolation across short gaps. The server preserves fractional player
positions so integer rounding does not introduce small speed changes between
rendered frames; the existing snapshot format carries these coordinates.
Command time stays monotonic after
stalls. Local shot effects are predicted once, avoiding duplicate sound/recoil. Local
prediction finds the first unacknowledged command in at most eight lookups
instead of scanning the full 128-command history each frame; it still replays
the same outstanding inputs and performs the same collision checks.

Server optimizations include UDP-readiness/deadline waits, cached byte-identical
Huffman encoding/decoding, byte-fragment bit fields, bounded local fragment
batches, larger receive buffers and gateway startup packet buffering. These
reduce work and avoid needless waits while preserving packet fields and
simulation rules. Snapshot writers search backward for the last changed field
and take an equality fast path for unchanged ammunition/objectives; regression
checks preserve the original packet bytes and overflow behavior. Collision
traces reject geometry outside the actual segment or beyond the nearest hit and avoid testing shared BSP brushes repeatedly;
the same collision geometry, contact margins and hit results are retained.
An exact-bit, bounded cache also reuses identical immutable-world sweeps in
the server and client. It resets on map changes, has independent native thread
storage, and excludes moving entities and temporary/transformed models. Replayed
prediction commands and repeated sightline checks can reuse their complete
results without rounding positions or skipping entity collisions.

The [30 versus 50 Hz Pi benchmark](docs/TICKRATE_BENCHMARK_2026-10-12.md)
keeps **30 Hz** as the default. With two automated protocol clients and no bots,
both rates were sustained (about 21% versus 27% of one core). With eight
automated clients sending about 58 inputs/s each and 16 Hard bots, 50 Hz
saturated a core and delivered snapshots in bursts separated by seconds.
High client FPS and low average ping do not rule out interpolation jitter or
irregular snapshot delivery. These tests measure server/network work, not
browser FPS; the report includes the fractional-position correction and its
validation limits.

An earlier 60-second Carentan test on a Raspberry Pi 5 kept **eight real browser players**
connected throughout, with sprint, automatic fire, smoke and frags exercised.
Each player received 1,200 game packets (**20/s**); the server container used
**34.0% of one CPU core**, with no CPU throttling or container restarts. The
worst per-client packet-gap p95 was **54.7 ms**, and the maximum was **64.8 ms**.
The eight renderers shared one development Mac and sent approximately 10–27
input packets/s each, so this is not eight independent PCs at 60 inputs/s or
eight players continuously firing. See the [Pi load-test instructions](docs/RASPBERRY_PI.md#repeat-a-connected-player-load-test)
for the measurement scope and a reproducible command.

A subsequent 60-second Carentan run sustained **32 Hard bots plus eight spawned
browser clients** at **20 game packets/s per client**, using **57.4% of one CPU
core**. Local status p95 was **18.1 ms**, server-reported ping p95 **45 ms**, and
the maximum server packet gap **112.6 ms**. Explicit realtime/capture checks
passed. This is a LAN load sample with eight renderers sharing one development
Mac; it does not guarantee minimum FPS or latency on every client/network.

Bot planning shares bounded work across the entire room. A* searches prioritize
routes toward objectives, retain usable routes and rotate pending requests
between clients. Nearby doors and short stair steps get collision-checked local
half cells; the rest of the map keeps its normal grid. Patrol destinations
penalize recently visited areas and teammates' destinations. Movement runs on
every original 50 ms simulation tick, including catch-up ticks. Bot-only HUD
name/crosshair queries are omitted because test clients have no renderer;
human labels, scripted look-at triggers, bullet hitboxes and graphics remain.
The native x86 build uses standard SSE2 float math to avoid expensive emulated
x87 arithmetic on the Pi, without fast-math. See [bot scaling and validation](docs/BOT_AI.md)
for the work limits and measurement scope.

The optional **`?perfDebug=1`** overlay reports actual submitted frames and
frame-time percentiles. Building/smoke tests in Carentan and sustained MG fire
in Toujane measured about 120 FPS on the development Mac's 120 Hz display;
recent muzzle-light checks also held about 120 FPS. Results depend on the
client, host, network and player count. The project does not guarantee zero
latency or a fixed frame rate on every device.

The 2026-10-11 lighting/mipmap follow-up retained 120 FPS at the same Carentan
spectator camera and 1280×720 resolution. A warm reference repeat measured
4.37 ms main-thread / 3.15 ms GPU against 4.33 / 3.19 ms afterward; the initial
reference had 4.11 / 2.64 ms. These short desktop samples establish no reliable
GPU speedup and do not represent a low-end laptop. WebGL2 pixel checks verify
front/back model lighting, unchanged two-sided world lighting, matrix UVs and
partial-mip sampling. The updated client also completed a live Carentan
connection, weapon selection and firing check without shader errors.

```sh
# Browser package, shared gameplay, rendering, networking and UI regressions
./scripts/test-static.sh

# Gateway room/startup/ownership checks
(cd downstream/gateway && npm ci && npm test)

# Native i386 directory enumeration (also runs during the server build)
python3 scripts/test-native-file-listing.py

# Private assets and balance audit
python3 scripts/check-private-assets.py
python3 scripts/test-weapon-balance.py

# Optional GPU fixture from the linked production shaders (after build-web.sh)
node scripts/prepare-web-lighting-qa.cjs
python3 -m http.server 8099 --bind 127.0.0.1 --directory out/web-lighting-qa
# Open http://127.0.0.1:8099/lighting-qa.html; all GPU checks must pass.

# Benchmarks; runtime benchmark requires the documented QA stack
python3 scripts/test-huffman-cache.py --benchmark
node downstream/gateway/benchmark.mjs --docker
```

The full build runs the static suite. Native code checks exercise real C paths,
including sanitizer-backed tests. A successful build alone does not establish
playable multiplayer; normal two-client browser checks cover combat, scopes,
reloads, room creation and the supported maps. Detailed measurements, deployment evidence
and remaining limitations are recorded in [RUNBOOK.md](RUNBOOK.md).

## Updates

```sh
python3 scripts/update-local.py          # Check without installing
python3 scripts/update-local.py --apply  # Fast-forward, build, and restart an empty standard host
```

Installation requires a clean `main` checkout and no active players. Players
see update notices after leaving a match and choose when to reload. Pi hosts
use the Pi deployment procedure instead of the standard rebuild command. See
[updates and asset delivery](docs/UPDATES.md) for cache/version behavior.

## Project and assets

This fork builds on [theodorecharles/cod2-wasm](https://github.com/theodorecharles/cod2-wasm)
and uses [wasm-game-framework](https://github.com/theodorecharles/wasm-game-framework)
for the browser shell. Maps, models, textures, sounds and original executables
remain the property of their respective rights holders. This repository supplies
code and tooling, not a copy or license of Call of Duty 2. Original game files
must be supplied privately by the owner and are excluded from Git.
