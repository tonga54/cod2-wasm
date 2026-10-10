# Call of Duty 2 Multiplayer — in your browser

A WebAssembly / WebGL 2 port of the reconstructed CoD2 IW 2.0 engine, with a
native dedicated server and a WebSocket-to-UDP gateway. Each browser runs its
own game client; the host runs the authoritative multiplayer simulation.

Play on a LAN using the original maps, models, textures, animations, sounds and
menus. The project is under active development. Two-browser combat, respawn,
room creation and map rotation have been exercised on the supported maps;
large populated matches and other hardware still need broader testing.

## Maps and game modes

| Map | Allies | Axis |
| --- | --- | --- |
| Carentan, France (`mp_carentan`) | American | German Normandy |
| Toujane, Tunisia (`mp_toujane`) | British | Afrika Korps |

Both maps support the original **Deathmatch, Team Deathmatch, Capture the Flag,
Headquarters, and Search and Destroy** modes. Select a map and Game Type in
**Start New Server**. Click Game Type to cycle forward; right-click to go back.
Rooms rotate between the two maps while keeping their selected mode. TDM uses
100 team points or 15 minutes; the other modes use their original scripts and
settings.

The host starts with **no rooms**. Users create their own named servers, with
up to three concurrent rooms and 64 configured player slots per room. An empty
room closes after five minutes. **Join Game → Delete my server** lets its owner
remove it immediately. Ownership is stored in that browser and checked with a
private token; changing browsers or clearing site storage loses that record.
The slot limit is configuration, not a claim that 64-player matches have been
load-tested.

## Additions beyond the original game

These are intentional changes, rather than features claimed to be part of
retail CoD2:

- **Sprint:** Shift + W increases movement speed, lowers and swings the weapon,
  and uses the running body animation. Sprint currently has no stamina limit.
  Melee moves to V; scoped Hold Breath continues to use Shift.
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
- **Browser hosting and controls:** create/delete rooms without a desktop game
  installation on each client, cached asset downloads with actual byte progress,
  saved gametags, English interface text, update notices, compact chat, and an
  explicit fullscreen button/F10.
- **Bounded cleanup and respawn feedback:** bodies remain for eight seconds in
  an eight-body pool, dropped weapons/ammunition expire after twenty seconds,
  and TDM uses a three-second respawn countdown.
- **Movement conveniences:** C toggles crouch. Space first stands up from crouch
  or prone; another press while standing jumps. Ordinary ground movement stops
  promptly when movement keys are released.

## Original behavior restored in the port

- Semi-automatic weapons fire once per valid trigger press. Rapid clicks cannot
  bypass fire, bolt, raise or reload timers. Original magazines and weapon-specific
  ADS/reload timing are preserved. The M1 Garand reloads only after its eight-round
  clip is empty; the Lee-Enfield needs space for a complete five-round charger.
- Reloading returns to held/toggled aiming unless the player cancels aim.
  Scoped Hold Breath lasts at most 4.5 seconds, with 5.5 seconds of recovery
  after exhaustion.
- Frag grenades have their original 3.5-second fuse, can be cooked in the hand,
  bounce off players, and can be picked up and returned with G while preserving
  the remaining fuse. Holding one too long kills its holder. Nearby live frags
  show the original icon and directional warning inside their blast radius;
  cover can still block splash damage.
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
FX point lights are combined into existing opaque geometry passes.

The authoritative simulation runs at the original **20 Hz**, with up to **60
input packets/s** and a 25 KB/s client rate. Remote motion interpolation uses
sample timestamps/velocities, smooths sample boundaries and allows bounded
50 ms extrapolation across short gaps. Command time stays monotonic after
stalls. Local shot effects are predicted once, avoiding duplicate sound/recoil.

Server optimizations include UDP-readiness/deadline waits, cached byte-identical
Huffman encoding/decoding, byte-fragment bit fields, bounded local fragment
batches, larger receive buffers and gateway startup packet buffering. These
reduce work and avoid needless waits while preserving packet fields and
simulation rules. Collision traces reject geometry outside the actual segment
or beyond the nearest hit and avoid testing shared BSP brushes repeatedly;
the same collision geometry, contact margins and hit results are retained.

The optional **`?perfDebug=1`** overlay reports actual submitted frames and
frame-time percentiles. Building/smoke tests in Carentan and sustained MG fire
in Toujane measured about 120 FPS on the development Mac's 120 Hz display;
recent muzzle-light checks also held about 120 FPS. Results depend on the
client, host, network and player count. The project does not guarantee zero
latency or a fixed frame rate on every device.

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

# Benchmarks; runtime benchmark requires the documented QA stack
python3 scripts/test-huffman-cache.py --benchmark
node downstream/gateway/benchmark.mjs --docker
```

The full build runs the static suite. Native code checks exercise real C paths,
including sanitizer-backed tests. A successful build alone does not establish
playable multiplayer; normal two-client browser checks cover combat, scopes,
reloads, room creation and both maps. Detailed measurements, deployment evidence
and remaining limitations are recorded in [RUNBOOK.md](RUNBOOK.md).

## Updates

```sh
python3 scripts/update-local.py          # Check without installing
python3 scripts/update-local.py --apply  # Fast-forward, build, and restart an empty standard host
```

Installation requires a clean `master` checkout and no active players. Players
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
