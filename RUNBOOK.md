# Call of Duty 2 browser runbook

Status: **Toujane/Carentan browser port with five multiplayer modes under active development. Chrome and
the internal browser have verified movement, aiming, damage, death, respawn and
synchronized scores, including the normal room. This is two-client coverage,
not a 64-player load or long-duration stability result.**

## Carentan and match limits (2026-10-09)

The supported maps are now Toujane (`mp_toujane`) and Carentan
(`mp_carentan`). TDM rounds end at 100 points per team or 15 minutes, whichever
comes first. Startup has no rooms. Creating a room uses the map and game mode
selected in the native menu and starts rotation after that map, preserving
the selected mode. Both the gateway and room supervisor accept only these two map IDs.
The room list continues to discover the room when it rotates to Carentan.
The package now enables the five original modes: DM, TDM, CTF, HQ and SD.
See the game mode entry below for the additional selection and startup checks.

Faction overrides are empty so the original map scripts choose the teams:
British/German Africa on Toujane, American/German Normandy on Carentan.
The private closure includes both BSPs, map/FX scripts, loading artwork,
American menus/loadouts/models and France ambience/US quick messages.
The original owner archives remain unchanged. The generated set is
275,564,709 bytes, with 4,369 entries and 518 sound files. Public manifests
contain only file metadata/hashes, and caches use the new asset version.
The generator and resource audit accept `--main-dir` and `--manifest` for
staging assets before installation. The hash regression honors
`COD2_WEB_DATA_DIR`, matching the Compose data-root override.

Native UI creation and map/type-map voting now index `mapList` as an array;
the old 168-byte calculation was inconsistent with the 164-byte 32-bit
structure and selected an invalid pointer for the second map. The browser
arena allowlist also includes Carentan.

Verification: `scripts/check-private-assets.py` validates both BSP material
sets, faction models/loadouts/menus and original resource bytes/reviewed
overrides. `scripts/test-room-maps.py`, `scripts/test-web-room-create.cjs`,
`scripts/test-ui-map-selection.py` and the gateway suites cover selection,
discovery, input allowlists, next-map configuration and failure handling.
Four real server rotations alternated Carentan/Toujane successfully and
queried the configured 100/15 limits after each load; the isolated container
used 138.9 MiB at the end. The complete WASM build/static package checks and
gateway suites passed with the staged asset directory.

The in-app browser created a Carentan room through Start New Server, selected
American and spawned with the original Grease Gun. Movement changed the view
and four shots reduced the magazine from 32 to 28. The opening TDM screen
showed 100 points/15 minutes. A voted change to Toujane preserved the connection
and opened the British/German team menu, with no browser error logs. Evidence:
`out/carentan-map-menu.jpg`, `out/carentan-limits.jpg`,
`out/carentan-gameplay.jpg` and `out/carentan-to-toujane-teams.jpg`.
This is single-client map/selection/movement/fire/transition coverage;
two-client combat and long matches specifically on Carentan remain unverified.

## Fullscreen, damage blood and player animation (2026-10-09)

A trusted primary click on the canvas requests document fullscreen without
consuming game input. The in-app browser expanded from 1280×720 to the screen
size on the click and returned with Escape. The input regression covers
synthetic/right clicks, concurrent requests and clicks already in fullscreen.

The low-health overlay now follows `cg_blood`, whose default is enabled, rather
than the disabled health-bar setting. Its previously zero-initialized pulse
table now has descending strengths (1.0, 0.8, 0.6, 0.4). This uses the original
blood material and pulse timing with reconstructed strengths. Two real clients
on an isolated native Toujane server verified a Sten hit, the red blood overlay
and recovery to a clear image and 100 health. Local evidence:
`out/gameplay-blood-damage.png` and `out/gameplay-blood-recovered.png`.
`python3 scripts/test-low-health-overlay.py` exercises severe damage, preference
off/on, interpolation, recovery and another hit; restoring either former
invisible-overlay defect fails.

Aim/lean controllers now compose their rotation with the sampled animation
and preserve bone translation. They formerly erased the animation and reset
the controlled bone's position to zero. An already-applied controller is cached
within the skeleton frame, while root placement remains an absolute override.
`python3 scripts/test-animation-controllers.py` covers six controlled bones
over 120 moving poses, noncommuting aim rotations, cache/part masks and root
placement under ASan/UBSan; pose and translation overwrite mutants fail.
Two clients verified both original body models and the advancing
`pb_combatrun_forward_loop` (e.g. time 0.66→0.70), nonzero hip/spine offsets and
changing rotations. Evidence: `out/gameplay-running-pose.txt` and
`out/gameplay-running-2.png`. This establishes the rigid-pose repair; overall
animation fidelity still needs broader playtesting.

Recreating the isolated server while those clients were connected, then using
the native reconnect command, produced a client script compile error. Reloading
the browser and joining again worked. Deploy only with rooms empty; the browser
update notice reloads the page rather than invoking native reconnect.

## LAN access during testing

The current host address is `http://192.168.1.60:8088/` (DHCP may change it;
check `ipconfig getifaddr en0`). Clients on the same LAN open this address,
then use **Start New Server** to choose a map/mode, or **Join Game** to
select a room created by a player. There is no web account or login.
The former `10.14.9.235` address is no longer assigned to this host.

Each room now accepts 64 clients, including browsers still loading the map.
This is the engine's `sv_maxclients` upper bound (0x40); the user requested the
maximum on 2026-10-07. Earlier two-slot checks below describe the old limit.
Capacity is configured, but 64-player gameplay/load has not been verified. The deployed dedicated
server and gateway both report 64. Chrome Join Game shows `0 (64)` (evidence:
`out/server-64-slots.jpg`); Start New Server also shows 64 and its newly created
room reported 64. Both gateway suites pass with 64 isolated transports and
concurrent per-room reservations, rejecting the 65th. The actual dedicated
query through WebSocket/UDP passes. Idle main-room container measurement after
this change: server 11.45% CPU / 194.9 MiB, web 0% / 10.59 MiB, gateway 0.02% /
15.46 MiB. These are idle observations, not a 64-player benchmark.
Disconnect test clients before handing the main room to human players; use an
extra room for continued development. On 2026-10-07, both default-room slots
were occupied by agent tests when the user reported another machine could not
join. Both were disconnected, and `/servers` confirmed zero players and
`/gateway/health` zero connections. The user subsequently reported `exceeded maximum number of script variables`
from the other Mac; the server-side failure is documented below. Local HTTP
reachability alone does not prove successful remote gameplay.

Rejected WebSocket upgrades now become native game errors. The browser queries
the same-origin room list to explain a full or disappeared room; network failure
gets a connection error. Pending errors cannot be replaced by repeated connection
attempts, and intentional disconnects or stale callbacks do not raise errors.
`node scripts/test-web-net-errors.cjs` covers these cases using the actual EM_JS
callback code. The gateway room list includes connections reserved during loading.
The deployed change was verified in Chrome against a separate temporary room:
two transport connections filled that room, and the third browser attempt showed
the native **Notice / Server is full** screen immediately. Evidence:
`out/server-full-native-notice.jpg`. The default room remained empty throughout.

The two-player investigation also found a stale server client stride: snapshot
iteration used the 1.0 size `0x78f0c`, while this 1.3 build's `client_t` is
`0xb105c`. The second player's entity position arrived, but its client metadata
was omitted (`clients=1`, empty model, no DObj). Current and archived snapshot
iteration now use typed client array indexing, and client-number calculation
uses pointer subtraction. `scripts/test-snapshot-clients.py` runs in the native
builder against both actual layouts, 1–64 clients and mixed connection states;
restoring either old stride fails the 1.3 cases. The corrected native server was
deployed with both rooms empty. Both browsers then reported `clients=2`, a valid
original enemy model and a non-null DObj. A second defect left remote poses stale
between entity events; packet rendering now interpolates each entity every frame,
including viewlocked entities drawn later. Packet DObjs also advance their
animation time and client notetracks each frame, preserving the separate update
for the predicted local player. `python3 scripts/test-packet-entity-positions.py`
covers event-free frames and detects removal of the update. Browser combat
validation remains in progress.

Skeletal culling bounds now dereference the `XBoneInfo` pointer array correctly,
use each axis's bounds, and allocate all 64 `DSurface` entries. The former code
read pointer storage as floats and could overwrite its undersized surface array.
`python3 scripts/test-model-bounds.py` checks 40 rotated 61/128-bone cases with
ASan/UBSan, compares against independently transformed corners, and rejects all
three old behaviors. Visual multiplayer validation is still required.

Both test clients also disconnected at the round end with `Server Disconnected -
595`. `CL_GetServerCommand` had incorrect command letters: it interpreted the
server's `d 595 ...` configstring update as an expulsion. Dispatch now matches
the dedicated server: `d` updates configuration, `x/y/z` assemble large strings,
`B/n` reset per-round input/notifications, and only `w` disconnects. Score,
announcement and weapon commands pass through to cgame. The extracted-function
regression `python3 scripts/test-server-commands.py` covers all those paths.
The default room subsequently completed its 15-minute round with both clients
still connected and showed the tie scoreboard (`out/round-end-no-disconnect.jpg`).
A further complete round returned Chrome and the internal browser to the TDM
team menu without disconnecting either client.

The other-Mac disconnect was reproduced on the default server when selecting a
team. `ClearArray` used unconditional `RemoveVariable` for absent integer/string
keys. Its failed lookup returned index zero; unlinking that sentinel destroyed
the free-list head and caused `exceeded maximum number of script variables`
despite 64,807 free entries. Temporary reference/free validation caught the first
bad release at `ClearArray -> RemoveVariable -> FreeChildValue_core(id=0)`;
the intact free chain then contained 64,815 entries. Evidence is saved in
`out/script-pool-invalid-clear.log`. Both key types now use `SafeRemoveVariable`,
and the generic remover also protects the sentinel on absent keys. The temporary
backtrace/validation instrumentation was removed before deployment.
`python3 scripts/test-script-array-clear.py` executes the actual allocator and
clear/remove code under ASan/UBSan for 20,000 absent/present integer/string
cycles; it checks the full free chain, array size and string-name references.
Restoring the original clear path or unguarded generic remover fails. Native
and WASM builds with the fix are deployed. Chrome and the in-app browser joined
the default room, chose opposite teams and spawned with Sten/MP40 without the
error. The room reported three simultaneous clients during this check, including
a connection outside the two agent-controlled browsers. Evidence:
`out/script-array-clear-chrome.jpg` and `out/script-array-clear-iab.jpg`.
Two subsequent in-app-browser reconnects also passed team/weapon selection.
Full combat validation remains in progress.

The game canvas now cancels `contextmenu` so right-click does not open Save Image
or browser actions, including before pointer lock. Mouse button events remain
available to native aiming. `scripts/test-input-capture.cjs` covers captured and
uncaptured states; the adapter is deployed through the web service alone.

Combat checks exposed remote player extrapolation far below the map while that
player was stuck near a terrain edge. The mesh sweep used a cross product in
the cylinder intersection quadratic instead of the approach dot product, and
both sweep and stationary edge checks used a perpendicular axis for the finite
segment coordinate. BSP edges and the loader confirm axes 0/1 are perpendicular
and axis 2 is the edge direction divided by its length. Those calculations are
corrected in `cm_mesh.c`. `scripts/test-mesh-edge-trace.py` extracts the actual
sweep/position functions and checks 86 face/edge/vertex cases with ASan/UBSan,
including rotated geometry and mutations restoring each incorrect calculation.
Both corrected targets build; browser movement validation is pending.

## Requested scope

The client runs locally in each browser as WebAssembly and WebGL 2. Docker
serves the web package and will run the native dedicated server and a
WebSocket/UDP gateway. The supported maps are Toujane, Tunisia
(`mp_toujane`) and Carentan, France (`mp_carentan`), with Deathmatch,
Team Deathmatch, Capture the Flag, Headquarters and Search and Destroy,
and up to 64 human players per LAN server.
The minimum gameplay acceptance check still uses two independent browsers.
Singleplayer, bots, additional maps and modes outside these five are excluded. The previous
Wine/noVNC container is stopped and is not used by this port.

## Immutable inputs

`source-lock.json` pins wasm-game-framework 0.9.2 at
`53bc7e6eeef1ae35dcf3b25dea4e3ec0ab46726f` and the reconstructed-source
baseline at `f70e697476fceeb4f53de677e1c5d5fe12a00b36`. Builds create an
isolated framework worktree at that exact commit.

Do not restore or use inherited `src/web`, `build/web_gen`, another browser
port, or a compiled third-party browser artifact. Do not contact or submit
anything upstream. The reconstruction has no repository-level LICENSE or
COPYING file; keep images local until distribution terms are documented.

The audited GPL-2.0 alternative `xtnded/cod2` is pinned at
`8eccf06c80423f099fb01745529bee6bb43cc84a`. Its fresh native and Emscripten
builds fail in the entrypoint before compiling the reconstructed engine. It
has no shared commit ancestry with this baseline and is not selected. A GPL
restart must recreate the platform and data seams independently. See
[SOURCE_BASE_AUDIT.md](SOURCE_BASE_AUDIT.md).

## Current implementation

The selected source graph has 448 WebAssembly translation units, including
real multiplayer client, prediction, renderer, UI, scripting, and shared
server/game definitions. Native assembly and duplicate zlib are excluded.
The build uses Emscripten 3.1.64, SDL2, and zlib. The engine generation is
IW 2.0, not IW 3.0.

`scripts/classify-wasm-data-symbols.py` classifies generated relocation
symbols from the compiled objects before linking the actual `cod2_client`
target. There is no allow-undefined flag or signature-cast emulation.
`cod2.js` and `cod2.wasm` link with zero undefined symbols. Remaining function
signature warnings and platform placeholders still require repair.

Browser verification has reached real `Com_Init`, the original IWD
filesystem, configuration execution, and a Chromium WebGL 2 context. SDL
creates the context before initializing the D3D/OpenGL bridge. The browser
selects the reconstructed fixed-function DX7 path; the original DX7 texture
operations and pass options are parsed into real renderer state.

Material failures return NULL rather than partially relocated data. Original
core materials and the dynamic light definition now load. Renderer reset,
initial state, and common initialization complete in the browser. Native fonts and all selected multiplayer menus load after correcting the
font callbacks and the nested text-command call. The first rendered frame
previously trapped on a void/HRESULT mismatch; ten D3D calls now have their
actual return type. Frames subsequently ran until a GPU query leak exhausted
the fence pool. Synchronization-off now skips unnecessary queries.
The original menu now renders in Chromium/WebGL 2. Emscripten 3.1.64's
legacy-GL duplicate-texture-load regex truncated nested matrix expressions;
`scripts/patch-emscripten-gl.py` fixes the linked generator reproducibly in
CMake without modifying the SDK. Both observed shader programs compile/link,
and their first indexed draws report GL error 0. Some menu layout/texture
defects remain. Game audio and an actual match remain unverified.
Browser crashes must be fixed rather than hidden by the adapter.

The browser now completes map/media/menu loading, receives the server's TDM
join commands and reaches `CG_DrawActiveFrame`. One incomplete frame of real
Toujane geometry has rendered. This is not a playable scene. The subsequent
color-array trap was caused by the lighting routine writing a ninth light into
an eight-light buffer and corrupting draw arguments. It now reserves the final
slot for sunlight. A guard-byte test against the actual C implementation passes
4,608 cases on both native 32-bit and WebAssembly builds; restoring the previous
loop bound makes the test fail. Temporary draw-argument traces were removed and
repeated browser connections have since reached the original TDM/team menus.
A prior trap in static-model skinning was fixed by using the actual HRESULT
return type of vertex-buffer Unlock. DX7 world/cached vertices now provide their
texture coordinates, including world lightmap coordinates. Rendering still has
black frames and menu defects; a stable playable scene remains unverified.

The compiler's boolean literal nodes were reversed: `true` emitted zero and
`false` emitted one. Correcting the two mappings fixes the real TDM admission
script. Two browser instances have now connected through distinct gateway UDP
ports, chosen British/German teams and spawned with Sten/MP40. Keyboard movement
changed the camera; held mouse clicks reduced ammunition (32 to 17 for Sten,
32 to 25 for MP40). This does not yet prove damage, kills, synchronized scores
or a complete two-player match. The native stack-protector abort was reproduced
while moving against a wall: `PM_SlideMove` allowed six planes into arrays sized
for five. Its insertion guard now uses the actual capacity. The regression test
exercises 20 cases and confirms AddressSanitizer catches the old overflow.
The temporary stack-guard override has been removed; normal stack protection
remains enabled. Sustained browser movement must verify this repair in play.
The stepping routine also rejected world surfaces instead of players. Its
entity comparison is corrected and covered across all 1,024 entity numbers.
Use `?movementDebug=1` for bounded collision traces (`PTRACE` on native builds).
The step repair still needs the in-browser traversal check.

The voice HUD no longer draws six absent talkers or indexes player names at
minus one. It respects the voice settings and returns when the talker/player
lookup fails. Browser verification shows the spurious speaker icons are gone.
Game-message command prefixes now use stable literals instead of nested `va`
buffers; the old calls could alias snprintf's input and output when formatting
a player's name. Join messages no longer appear as unknown commands in the
latest client logs.

The dedicated server's timeout loop read a stale raw offset (`0x765f4`) instead
of `client_t.bIsTestClient` (`0xae704` in the current native 32-bit layout). It
now uses the typed field. The LAN configuration uses a 30-second lost-packet
timeout. A real closed browser was dropped with `EXE_TIMEDOUT` after 30.3 seconds
and its slot accepted a subsequent browser. Background tabs may be throttled;
final gameplay verification should use independent foreground browser windows
or devices, and must verify continued simultaneous traffic.

The linked Emscripten indexed-draw wrapper now computes the referenced vertex
range before copying separate client arrays. The original wrapper copied the
index count instead, leaving higher referenced vertices uninitialized. Tests
cover sparse indices, explicit ranges, vertex zero, empty draws and GPU buffers;
restoring the old code fails the sparse-index test (3 copied versus 256 needed).
The framework/SDK source is unchanged; the reproducible local linker patch owns
this correction. Already-RGBA interleaved colors no longer need a separate copy.

Additional renderer repairs preserve zero-valued D3DTA_DIFFUSE arguments and the
material's lightmap scale, select the traditional baked lightmap for DX7, and
convert BGRA surface uploads to RGBA for WebGL. The pixel conversion passes a
WASM test for channel order, alpha, bounds and in-place use. Browser verification
now shows textured Toujane walls and ground while moving. Sky rendering, HUD
artifacts, model lighting/color ordering and remaining assets still need work.
Static-model lighting now reads typed D3DLIGHT9 fields rather than incorrect
raw offsets, including reads before the light array.

The temporary per-opcode team-permission VM traces have been removed. Use
`?menuDebug=1` for bounded client menu traces, `?graphicsDebug=1` for renderer
traces, and ignored `out/server-debug.yaml` for server menu traces. The server
override is not enabled in the normal Compose deployment.
The page automatically loads the original main menu, without a launcher name
field or automatic connection. Player names are controlled by the native game
options. The browser title is Call of Duty 2 Multiplayer.

`web_client_state` reports gameplay only when the native client is active and
no menu/chat/console catcher is open. `web_capture_lost` delivers Escape into
the native event queue. Browser builds no longer call SDL window-grab or relative-mode setters; the framework alone requests/releases pointer lock. This latest input change still needs sustained in-match verification.
The frame limiter yields to requestAnimationFrame instead of spinning.

## Private assets

Original owner assets remain untouched in ignored `data/main/`: 28 IWDs,
3,685,129,248 bytes. `scripts/prepare-browser-bootstrap.py` produces a private
subset under `data/browser/main/` containing configuration, localized strings,
renderer/font/light dependencies, MP menus, the Toujane BSP and its models,
TDM scripts, British/Afrika Korps character assets, MP animations and loadouts.
It is currently 134,101,065 bytes across three archives (2,512 unique entries). Only one BSP is
included. Script comments are ignored when following dependencies; scripts
needed by the compiler are retained without models for unused factions.
The closure also includes engine-created mantle animations, client HUD,
impact effects, shellshock settings and the dynamically named British/German
team and weapon menus. Localized original IWDs contain some Toujane materials
and images, so they participate in dependency lookup as well. Sound
dependencies and complete gameplay still need verification.

The generated archives are deterministic and preserve original resource bytes.
Later original IWDs override earlier ones. The metadata policy records private
paths, sizes, ZIP signatures and SHA-256 hashes. The browser uses the canonical
container-to-IndexedDB client and mounts this bounded subset into MEMFS.
The remaining dependency closure and browser memory use require measurement.

No IWD belongs in Git, the public site, or a Docker image. Docker mounts
`data/browser` read-only at `/data`. The framework allowlisted data endpoint
serves these files; direct `/data` and `main/*.iwd` URLs remain inaccessible.

## Public package

The framework owns HTML, CSS, the service worker, manifest, launcher, setup
and viewport. The game package contains only:

```text
asset-sha256.js
cod2-diagnostic.svg
cod2.js
cod2.wasm
native-game-adapter.js
wasm-game-data.json
wasm-game-framework.json
wasm-game.json
```

The favicon is the original MP executable's icon, extracted into ignored
`data/browser/web/cod2.ico` and served by the gateway from a read-only mount.
The SVG remains an unused legacy package asset. The previous checksum probe and adapter are
excluded from the staged package. All generated binaries remain under ignored
`out/`. The adapter launches the real engine and never substitutes a demo or
claims gameplay after a build/menu/loading screen.

## Build and start

```bash
python3 scripts/prepare-browser-bootstrap.py
./scripts/build-docker.sh
docker compose up -d
```

`build-docker.sh` uses local Emscripten if available, otherwise the official
`emscripten/emsdk:3.1.64` image with the amd64 platform and a reusable SDL cache.
It builds the actual client and validates the framework package, then creates
local suite and multiplayer images. It also invokes `scripts/build-server.sh`
to compile the native 32-bit dedicated server and builds the server/gateway
images. `compose.yaml` exposes the gateway at http://localhost:8088. The web
container has a one-CPU/512-MB limit; the dedicated container has a two-CPU/1-GiB
ceiling shared by all rooms (64-slot snapshot buffers need more memory); the gateway
has a half-CPU/96-MB limit. UDP remains on an internal Docker network.
That URL opens the original main menu. Full gameplay remains unverified.
If the original favicon is absent, the build script extracts it from
`COD2_ORIGINAL_ZIP`, defaulting to the owner's ZIP in Downloads.

The native `cod2_lnxded` target compiles and has loaded Toujane/TDM, initialized
638 entities and run game frames for a 45-second observation. Missing sound
aliases remain. Repetitive per-frame diagnostic prints were removed, and the
native frame limiter now sleeps for the remaining interval instead of polling
with a zero timeout. This server observation does not prove client gameplay.

`downstream/wasm/web_net.c` replaces the browser's unavailable UDP socket with
binary WebSocket datagrams. The Node gateway assigns a distinct connected UDP
socket to each browser and fixes the remote endpoint to the dedicated server.
Queue sizes and client count are bounded, and compression is off. The isolated
transport test passes binary roundtrips for two clients, unique UDP source
ports, the two-client limit and origin checking. The real browser now completes
challenge/connect and receives gamestate over this path. Full two-player gameplay remains
unverified. The browser follows the engine's existing LAN authorization policy
when `net_lanauthorize=0`; no external authorization request or invented key is
used for this isolated local dedicated server.

Connecting initially exposed a server script-VM crash during disconnect.
Notify-owner tables now use pauseArrayId and timed waits use timeArrayId.
Resumed call frames and local-variable caches now index live frames 1..count,
preserving frame zero as the sentinel. The previous off-by-one restored stale
thread identities. A real browser connection/disconnection has since completed
without that crash, and the server kept running. Temporary verbose VM logs
were removed; the current bounded menu override is described above.

The server image includes only the public asset manifest, so changing the
private package causes Compose to recreate the server and refresh its cached
IWD checksums. Assets themselves remain mounted outside the image.

An idle snapshot after a browser disconnect measured server 8.25% CPU and
50.46 MiB RAM, web 0% and 12.74 MiB, gateway 0.28% and 45.56 MiB. These are
development-container observations, not two-player gameplay measurements.
A later snapshot with both browser connections admitted measured server 10.92%
CPU / 128.4 MiB, web 0% / 13.59 MiB and gateway 2.27% / 30.84 MiB. This was a
short development observation before the texture-upload repair, not a sustained
match benchmark; browser CPU/GPU/RAM and frame times still need measurement.

## Verification and remaining work

### Native menu, identity and additional servers (2026-10-07)

The adapter starts the actual engine automatically and stops at its main menu.
There is no HTML player-name form, Play button or injected connect command.
The native options control the player name. The full browser title and original
ICO favicon were checked in the internal browser; the icon's five original
sizes are extracted from `CoD2MP_s.exe` in the supplied ZIP and remain private.

Join Game defaults to Local. `/servers` lists live dedicated instances by
querying their real UDP `getinfo` response. Merely viewing the main menu or
server list does not open a gameplay WebSocket. The browser connects only after
the player joins. The list's binary insertion previously tried to insert its
first entry at index one; the corrected insertion interval includes index zero.
The JS discovery bridge also exports `_free` so releasing its temporary strings
does not abort Emscripten.

Start New Server uses the original menu action to request a separate native
dedicated process, then joins it after an actual ready response. The internal
Python supervisor exposes no Docker socket, arbitrary command or UDP target.
At this earlier stage only Toujane/TDM with up to 64 players per room was
allowed. The default process stayed available;
up to two additional rooms are allowed and empty ones expire after five minutes.
All instances share the existing server container's CPU/RAM limits and assets.
The gateway reserves up to 64 WebSocket slots per room. The native menu restricts the
map list to Toujane; the private package contains only the TDM game-type script.

The routing contract test covers room isolation, per-room capacity, origin and
fixed destination restrictions. An isolated actual server-container check
created a second dedicated process on UDP 28961 and received real Toujane/TDM
responses from both processes. Evidence: ignored `out/rooms-native-verification.json`.
This establishes room creation, not full in-game two-player verification.
The internal browser subsequently created room 1 through the original Start
button and reached its TDM information screen. A second browser client using
the LAN URL saw both rooms in Join Game, joined the created room by double click,
and the actual dedicated reported two connected players. Merely viewing the
list left the gateway at zero clients. Screenshot: ignored
`out/join-game-rooms.jpg`. Both gateway suites and 25,600 insertions against the
actual C server-list functions pass. The updated private package fidelity check
passes for all 2,510 entries.

This run later showed a client-side server timeout while switching between the
two clients; the server logged EXE_DISCONNECTED. It is not a sustained gameplay
pass. `CL_Frame` incorrectly measured timeout from `connectTime`; it now uses
`lastPacketTime`, which the native packet handler updates when receiving the
server's traffic. A test extracts the actual timeout branch and verifies that
live old connections stay active, missing traffic still times out after six
checks, and new packets reset the counter. Restoring the old field makes the
regression test fail. The next browser check created room 2 at approximately
22:14:26 UTC and remained in gameplay past 22:17:57 UTC (over 211 seconds),
with a Sten, movement and ammunition consumption observed. No client timeout
appeared past the old 200-second limit. This is a one-client check, not a
complete two-player stability pass.

A separate UI repair clears visible flags on all registered menus during
Menus_CloseAll, including menus initially visible in their source but absent
from the open stack. The original profile selector was otherwise drawn over
team/weapon menus. The corrected TDM, team and weapon screens were verified
without that overlay, and the player spawned with a Sten. At this earlier stage the creation screen
exposed only the name, TDM,
64-player capacity and Toujane; original menu files are not rewritten for this.

An idle container snapshot before the second dedicated was created measured
server 10.08% CPU / 143.5 MiB, web 0% / 16.29 MiB and gateway 0% / 34.92 MiB.
This does not measure a sustained two-player match or the browser's resources.
During the later one-client check with the default room also running, the
server measured 26.62% CPU / 257.2 MiB, web 0% / 15.11 MiB and gateway
0.92% / 21.14 MiB. Room 1 was automatically removed after remaining empty,
while the occupied room 2 stayed listed.

The Mac changed LANs during verification. Its current address is
`http://192.168.1.10:8088`; the old `10.14.9.235` address below is historical.

### LAN HTTP and terminal-input fixes (2026-10-07)

The LAN origin `http://10.14.9.235:8088` does not expose Web Crypto's
`crypto.subtle`. The adapter now supplies a mandatory custom SHA-256 validator
through the pinned framework's supported owner-file validation hook. A local
worker hashes 1-MiB slices, checks the exact original manifest digest, and exits.
Both downloads and cached files remain validated. HTTPS/localhost continue to
use the framework's Web Crypto implementation. No global crypto override,
browser security setting, or manifest digest was changed.

Tests cover standard hash vectors, padding and chunk boundaries, all three
private archives, corrupted bytes, worker cleanup, and the secure-origin path.
The internal browser loaded the actual LAN HTTP URL and reached the TDM server
information screen. The real worker logged 11 ms, 11 ms, and 678 ms for the
three files; a second cached run logged 9 ms, 7 ms, and 609 ms. This is asset
validation evidence, not proof of completed multiplayer gameplay.

The browser `Input:` modal came from Emscripten's default terminal stdin.
The module factory now supplies an EOF-only stdin callback, and the WASM
`Sys_ConsoleInput` path returns immediately without polling a terminal.
SDL keyboard/game input remains active. The adapter regression test checks
the EOF callback. A fresh LAN browser instance reached the TDM screen with
no active JavaScript dialog. Existing pages require a reload to pick up this fix.

The earlier map-rotation crash was traced to missing `codescripts/struct.gsc`
and `codescripts/delete.gsc`. The engine loads these directly, outside the
script dependency graph. `G_LoadStructs` was executing function handle zero,
entering the buffer sentinel and underflowing the VM stack while releasing
values. Both original files are now included (+400 compressed bytes), and
GScr_LoadScripts validates these required functions with its existing helper.
Temporary stack/entry diagnostics were removed after locating the cause.

Repeated rotation then exposed a second defect: the animation parser appended
another set of items on each of its two passes and every map load. The complete
script tables and their bounded pool count now reset for each parse. A map load
also resets animation metadata and reloads the source filename correctly.
`python3 scripts/test-map-rotation.py --rounds 12` passed twelve consecutive
real native rotations, with a fresh baseline each time. Its isolated server
measured 4.85% CPU / 109.4 MiB after the test and was removed. This is not a
two-player resource benchmark. Evidence: `out/map-rotation-check.log`.

Pointer lock was verified in Chrome on the HTTP LAN origin after activating
its native tab and clicking the game: `request-resolved` and
`pointerlockchange` both reported `locked=true`. Background browser automation
had returned WrongDocumentError; the internal browser returned UnknownError
even while visible. A captured mouse drag changed aim and ammunition. This
does not yet prove combat. Opening the console exposed a separate WASM callback
signature mismatch; R_DrawText and R_DrawConsoleText now use the renderer
interface's actual void return type.

A later LAN test connected but displayed `Connection interrupted`; the native
server logged repeated `bad command byte 4 for client 0`. Investigate this
alongside two-client reconnect behavior before claiming gameplay verification.

```bash
./scripts/test-static.sh
python3 scripts/test-server-list.py
python3 scripts/test-client-timeout.py
python3 scripts/test-map-rotation.py --rounds 12
python3 scripts/test-slide-bounds.py
python3 scripts/test-step-move.py
python3 scripts/test-voice-hud.py
python3 scripts/test-game-messages.py
./scripts/test-web.sh
# Inside emscripten/emsdk:3.1.64, from the repository root:
emcc -w -O2 -fno-strict-aliasing -I . -I src -I src/headers \
  scripts/test-light-bounds.c -s ENVIRONMENT=node -o out/test-light-bounds.js
node out/test-light-bounds.js
emcc scripts/test-web-pixels.c -s ENVIRONMENT=node -o out/test-web-pixels.js
node out/test-web-pixels.js
emcc -w -O2 -I . -I src -I src/headers scripts/test-font-callbacks.c \
  -s ENVIRONMENT=node -o out/test-font-callbacks.js
node out/test-font-callbacks.js
```

Static checks cover JS syntax, WASM signature, the pinned framework, native
adapter contract, exact public files, private metadata and tracked-asset
boundaries. They are not gameplay verification. Browser logs live in the
framework loading console and must be checked after every runtime repair.

Animated player poses now normalize the accumulated translation weights and
add the model's bind translation to each animated child bone. Original XAnim
tracks contain translation offsets; treating these as full translations
collapsed the hips/root onto one point. The null quaternion track also now
contributes a full identity quaternion at its blend weight. Browser evidence
after walking/jumping: `out/animation-pose-fixed.jpg`, remote player bounds
roughly 68–74 units high instead of the previous collapsed 30-unit pose.
`scripts/test-animation-pose-weight.py` passes 14 cases plus two deliberately
broken variants, with cached/ignored bones preserved. Native and WASM deployed.

The same browser test used right-click to aim down sights with no browser
context menu (`out/right-click-ads-no-menu.jpg`). Damage is still unverified.
The server's locational actor/model traces used a 3x3 matrix with an inverse
4x3 transform, reading past the matrix instead of subtracting the entity origin.
Both trace paths now supply a 4x3 matrix with the origin row. The regression
test `scripts/test-locational-transform.py` passes 96 translated/rotated actor,
model and visibility cases, and AddressSanitizer rejects the old matrix.
Native gameplay verification of this trace repair is pending.

Further combat diagnosis found three collision defects. `CM_TempBoxModel`
now sets its leaf's brush contents so dynamic entities actually enter the world
tree. World entity lists encode client indices as index+1, with zero reserved
for the terminator; both encoding and decoding now agree. The entity-linking
regression covers 64 clients, 32 relink cycles and three failing old variants.
Anti-lag now samples history at `sv_fps`, reports the actual cached sample age,
and interpolates with that age without division by zero. It previously moved
players temporarily to NaN positions during every shot. The archive also now
allocates typed player/entity scratch buffers, including the full 276-byte
archived entity. The history test covers 32,000 interpolations and old failures.
`CM_TraceBox` now performs a standard slab interval rejection, accepting a player
before a wall even when the world trace has shortened the ray. Its test includes
the captured browser shot and 600 translated bidirectional rays.

Native bullet logs now confirm real model hits on the other browser player.
The next gates were also broken: `ClientEndFrame` wrote `active` instead of
`takedamage`, and hit-location multipliers remained zero because their original
table was never initialized. The lifecycle assignment is repaired; the original
`info/mp_lochit_dmgtable` is included, loaded on each game initialization and
parsed with its correct ten-byte header. Bullet weapons use their own location
multipliers; other damage uses the global table. New regressions cover 6,400
player state cycles and all 19 locations across 12 table loads. The private
asset total is now 134,101,322 bytes. Death animation context dereferences the
`bgs` pointer correctly instead of treating its pointer storage as the structure.

Temporary investigation uses `out/bullet-debug.yaml` and a copied room supervisor
that starts room 1 with `devmap`; only this temporary room permits `setviewpos`
to reproduce shots promptly. Room 0 remains normal. BTRACE instrumentation is
temporary and must be removed, and ordinary `compose.yaml` restored before the
final gameplay verification. Damage/death/respawn/score are still pending.

The diagnostic room subsequently verified real combat in both directions:
Sten damage reduced client 0 from 100 to 65 health, sustained fire killed it,
both browsers displayed 1–0, and client 0 respawned with 100 health. MP40 fire
from client 0 then killed client 1, both displayed 1–1, and client 1 respawned
with 100 health. Evidence: `out/combat-bidirectional-server.log`, the matching
`-iab.log` / `-chrome.log`, and `out/combat-both-directions.jpg`. Only initial
positions were set by the diagnostic console; damage, deaths, scores and respawn
came from the original engine/scripts and actual browser input.

The first lethal shot exposed a native `XAnimCloneAnimTree` crash: corpse trees
were never allocated, and their empty entity sentinel was zero instead of -1.
Initialization now creates 64 live and eight corpse trees and resets corpse
slots on restart. Browser corpse snapshot/render paths also used an obsolete
`cgs` offset (49,684 instead of the actual 164,756), now replaced with typed
array access. Regressions cover 12 loads/96 restarts and 512 client corpse copies.
Twelve native map rotations passed again (6.30% CPU / 116.5 MiB for the isolated
empty server, not a gameplay benchmark). Temporary bullet diagnostics have
been removed; clean native/WASM builds and static checks passed. Ordinary
`compose.yaml` is deployed again: no BTRACE environment or diagnostic supervisor
mount remains. Normal-room verification is in progress.

Normal-room combat subsequently passed in both directions, without `devmap`
or position commands. The two clients walked from their original spawns. MP40
fire reduced Chrome from 100 to 55 health, killed it, and synchronized the
score to 1–0. After respawning, Chrome walked back and killed the internal
browser with the Sten. Both displayed 1–1 and both respawned at 100 health.
Evidence: `out/normal-combat-iab.log`, `out/normal-combat-chrome.log`,
`out/normal-combat-server.log`, `out/normal-combat-both-directions.jpg`,
`out/normal-combat-chrome-respawn.jpg`, and `out/normal-combat-iab-respawn.jpg`.
An earlier normal-room attempt hung one Chrome tab; the exact cause is still
unconfirmed. The successful fresh-client run does not prove sustained stability.

Two additional defects were repaired: `ClientEndFrame` now copies the session's
viewmodel index into `ps.viewmodelIndex` instead of overwriting `damageCount`;
and `player_die` establishes/restores the animation context, including deaths
from client commands outside `G_RunFrame`. The previous null-context write
crashed the dedicated server on `/kill`. Updated player-state coverage passes
6,400 cycles; `scripts/test-player-death-context.py` passes 192 deaths across
64 clients with null/server/other entry contexts under ASan/UBSan, and rejects
the original null write and missing restoration. Both browsers subsequently
survived `/kill`, normal weapon kills and respawns.

A Docker snapshot with two idle connected test players measured server CPU
36.00% and 137.3 MiB, web CPU 0.00% and 15.27 MiB, and gateway CPU 2.66% and
21.08 MiB. This is an idle snapshot, not a sustained gameplay benchmark.
Visual effects, sound, movement at some spawn ledges, and reconnecting without
a full page reload still need work.

The tracer investigation found that normal first-person shots explicitly skip
the local tracer in the current cgame code. Other clients do generate moving
polygons using the included original `gfx/misc/tracer` material. The BGR bitmap
loader incorrectly expanded them into ARGB bytes, while the surface uploader
expects BGRX. It now preserves BGR and appends opaque alpha. The actual loader
loop plus WebGL conversion passes all 341 original tracer mip pixels and 1,024
additional samples under ASan/UBSan (`scripts/test-bitmap-colors.py`).
Chrome firing while the internal browser spectated verified the yellow moving
tracer using default chance/width/speed in the normal room. Evidence:
`out/tracer-color-verified.jpg` and `out/tracer-color-verified.log`.
Temporary tracer diagnostics were removed after verification. This does not
claim that local first-person tracers or sound were added.

Repeated firing exposed a separate reload defect: `PM_DropTimers` and
`PM_UpdateWeaponTimers` both decremented weapon timers. The movement update could
consume the zero crossing before the weapon code transferred ammunition. Weapon,
kick and grenade-cook timers now have a single owner in `PM_Weapon`.
`python3 scripts/test-weapon-timers.py` covers 60,000 frame/delay combinations,
exactly one magazine transfer and grenade expiry under ASan/UBSan; restoring
the duplicate decrement fails. Native/WASM builds and static checks pass.
Chrome verified a partial reload from 19/192 to 32/179 and a subsequent empty
magazine reload to 32/147. Evidence: `out/reload-verified.jpg` and
`out/reload-empty-verified.jpg`.

The sky shader previously failed compilation because Emscripten emitted a
`textureCube` call with a `sampler2D` and two-coordinate argument. The local
linked-JS compatibility patch now emits matching cube samplers/3D coordinates;
the linked-generator tests cover both 2D and cube targets. Single-level cube
textures also set their allocated maximum mip to stay complete with trilinear
sampling. The shader now links and the sky image is present, but the initial
UV path stretched it. D3D camera-position texture coordinate generation was
added and passes 1,536 translated/rotated/interleaved input cases. The original
cloudy sky is visually verified in `out/sky-cubemap-verified.jpg`.

Browser audio is now implemented through a Miles-compatible Web Audio backend
(`downstream/wasm/web_audio.c` / `web_audio.js`). Playback starts with an ordinary
game click or keypress when the browser requires a gesture. No microphone or
extra input dialog is requested. PCM conversion preserves the original sample
rate and channels; the browser decodes the original MP3 streams. Sounds have
per-channel pitch, looping, pause/resume, distance attenuation and stereo pan.
The decoded-buffer LRU retains up to 32 MiB; active channels can also retain
buffers outside that cache. Environmental reverb DSP remains unimplemented.

The private package now includes 470 unchanged sound files (468 PCM WAV and
two MP3 files), plus the filtered original alias rows, curves and speaker maps.
Total private archives are 165,505,035 bytes / 3,000 entries. Alias traversal is
sorted for deterministic output. `check-private-assets.py` verifies each audio
payload against the owner's archive and each selected CSV row against its
original table. It also verifies the referenced sound files exist.

Audio activation exposed and repaired several latent engine defects: the
driver dereferenced the sound-state structure as though it were a pointer,
2D channel ids 45..52 indexed an eight-element array without subtracting 45,
and `SND_SetChannelInfo` was a no-op. Sound registrations and entity origins now
use typed fields. The unused legacy microphone initialization is excluded from
the WASM client. Spatial streams now use the real distance, correct listener
axis and global channel id.

`PM_FootstepEvent` was empty. It now emits the original surface-specific events,
and movement speed is calculated before the footstep update. The dedicated
server also cleared `eventSequence` each end-frame, discarding the player
events already exported by `ClientThink` before sending snapshots. The ring
now persists; the client handles its eight-bit wrap, including sequence zero.
This restored remote weapon and movement sound events.

Chrome and the internal browser both loaded and joined the ordinary room with
audio. Menu music/ambience, Sten/MP40 firing, reload and walking raised the
Web Audio output analyser above silence without decoder errors. Remote shots
passed in both directions: the idle receiving browser's start counter changed
26→41 (RMS 0.06634), and the reverse receiver changed 44→60 (RMS 0.01032).
The remote-audio check used two nearby teammates; the earlier opposing-team
damage/death/respawn checks remain separately documented above. Evidence:
`out/audio-remote-proof.json`, `out/audio-remote-shot-verified.jpg`,
`out/audio-mp40-verified.jpg`, `out/audio-sten-verified.jpg`,
`out/audio-reload-verified.jpg`, and `out/audio-footsteps-verified.jpg`.
The optional `?audioDebug=1` meter is absent from the ordinary game URL.

Passing regressions: actual WAV/MP3 parsers on all 470 payloads, truncated WAV
rejection, JS playback/cache/async decoder lifetime, all 53 sound channels,
2,184 positional stream cases, 393,216 footstep transitions, and 38,400 remote
event frames across 64 client ids. C checks use ASan/UBSan; restoring the old
end-frame sequence reset fails. Native/WASM builds, static checks, private asset
checks and twelve isolated native map rotations passed with the audio assets.

Twelve Docker samples during a two-client session (one moving, the other mostly
idle) averaged: server 33.15% CPU / 154.9 MiB at the final sample, web 0.12% /
16.69 MiB, gateway 2.62% / 25.89 MiB. This is approximately 197.5 MiB and 35.9%
of one CPU core, not a 64-player benchmark or browser CPU/GPU/RAM measurement.
Raw samples: `out/audio-gameplay-resources.jsonl`.

The internal browser also disconnected and reconnected to the ordinary server
without reloading the page. It completed team/weapon selection, spawned with
the MP40, fired five rounds (32→27), moved and resumed sound without decoder
errors. The earlier duplicate-animation reconnect error did not recur in this
check. Evidence: `out/reconnect-audio-gameplay.jpg` and
`out/reconnect-audio-gameplay.log`. This is one reconnect, not a long soak test.

## Grenades, ladders, static objects and local bullet effects — 2026-10-08

Grenade input now preserves pull-pin, hold and throw phases. The projectile is
released at the weapon's 100 ms release point, with one ammunition decrement;
the dedicated event matches the client event. The offhand regression passed
1,452 frag/smoke schedules. Browser captures: `out/grenade-held.jpg` and
`out/grenade-releasing.jpg`.

Ladder movement now detects the ladder surface, climbs, holds, descends and
jumps away. All six BSP ladder volumes were climbed in Chrome with matching
predicted/server positions. Evidence: `out/ladder-browser-proof.log` and
`out/ladder-rooftop-verified.jpg`. The regression passed 264 movement schedules
and 256 surface flag combinations; step movement and weapon timers also passed.

Static XModel callbacks now receive the model pointer using the correct ABI.
Vehicles, palms, crates and barrels render again. Static lighting indexes full
model records, and the skinning path receives the actual part-bit array.
The texture bridge accepts valid WASM allocations below 0x08000000; the old
native-address cutoff discarded valid model textures and reused stale bindings.
Indexed vertex conversion now covers only the vertices referenced by a draw,
rather than the entire 65,536-vertex static cache. Tests passed 8,190 texture
handles and 35,475 index ranges. `out/effects-two-client-combat.jpg` includes the
restored Kubelwagen and the synchronized 1–1 score.

The firing player's bullet events now produce tracers. Client initialization
loads the original weapon attachment tag identifiers so the muzzle position
comes from the weapon model. The regular defaults remain 4,500 units/second and
40% tracer probability. `out/local-tracer-normal-speed.jpg` used the regular
speed and 100% probability only to capture the short-lived visual reliably;
the probability was restored immediately afterward.

Impact decals receive a complete orientation basis and RGBA vertex colors.
FX parsing preserves three-channel RGB curves, endpoint values and scalar
ranges; the scheduler preserves world positions and valid primitive dispatch.
The scheduled-effects pass now actually calls the particle drawing pass after
rebuilding visibility from live particles. Sprite/line vertex colors preserve
RGBA, avoiding the yellow/green smoke caused by alpha-first packing. Original
wall marks and impact particles are visible in the normal LAN room:
`out/bullet-impact-lan-verified.jpg`. The FX regressions passed 7,138 original
curves, 8,256 drawing/lifetime cases, 514 world origins and 8,778 tracer schedules.
Grenade explosion and smoke-cloud presentation have not received equivalent
visual coverage; do not infer complete effect parity from the bullet checks.

The current private archives contain 3,016 entries / 165,587,676 bytes. Missing
nested FX dependencies were included from the owner's originals. Byte matching,
Toujane/TDM scope, native/WASM builds and `git diff --check` passed. Existing
link signature warnings remain; a successful build alone is not gameplay proof.

Two independent browsers again exchanged fatal fire, respawned and synchronized
the score at 1–1 in the isolated room. After publication, both used
`http://192.168.1.10:8088/`, entered through Join Game and spawned in the ordinary
room. The room API reported two players/two connections; Chrome fired against
a wall and displayed dust and bullet marks without a WASM runtime exception.

Six normal-room Docker samples averaged server 25.66% CPU, web 0.26%, gateway
2.67% (28.59% of one core combined). Final combined container memory was
209.33 MiB: 158 + 23.54 + 27.79 MiB. Samples are in
`out/effects-gameplay-resources.jsonl`. In Chrome's final 600-frame window,
with both browsers on this Mac, rendering at 640×480 measured 34.8 FPS,
32.8 ms frame p95, 28.5 ms main-thread time and 7.3 ms GPU time. The WASM heap
was 512 MiB and reported JS heap 174.8 MiB; these are not total browser RSS or
GPU memory. Earlier isolated-room views measured approximately 45–54 FPS.
The saved window is `out/effects-browser-performance.json`. Performance varies
by view and host, and 64-player load remains untested.

The three temporary movement/effects containers were removed. Only the normal
web, dedicated server and gateway remain. Test clients are disconnected before
handoff. Start the installed build with `docker compose up -d` from this repo;
to rebuild source, run `./scripts/build-server.sh`, `./scripts/build-docker.sh`,
then `docker compose up -d --build`.

Further coverage: sustained matches, additional hardware/browser configurations,
remaining model lighting/effect fidelity and loads beyond two browser players.

## 2026-10-08 — movement clock, smoke, model colors, recoil and map restart

The clock extrapolation test in `CL_SetCGameTime` was reversed. It reduced the
client clock offset while the client was behind the snapshot, causing drift and
large periodic resets. It now detects time at/ahead of the snapshot. Prediction's
`moved` flag is initialized before early exits. The extracted clock regression
runs 42 three-minute frame-rate/latency schedules and rejects the old condition.
The browser sample after a voted restart contains 559 snapshots over 28.054 s,
zero backward clock steps and zero `Prediction miss` corrections above 0.1 game
units (`out/fixes-clock-browser.log`, `out/fixes-clock-summary.json`). This is two
browsers on the host Mac; it does not measure the user's other physical Mac.

Camera recoil velocities now feed a damped spring and `CL_SetUserCmdAimValues`,
so kick reaches the actual command aim sent to the server. Original weapon kick
and spread parameters remain in use. Respawn clears the spring. The analytic
integrator is frame-rate independent; this is a reconstruction, not a claim of
bit-identical original recoil timing.

FX scheduling updates existing effects once per frame; only newly scheduled
particles get an additional initial update. `FX_AddLine` now writes material and
sort group through typed fields: the former Effect-pointer arithmetic wrote far
past its allocation. ASan/UBSan exercise that constructor. Dynamic world/effect
vertices retain RGBA, while UI command colors are converted ARGB→RGBA on upload.
This preserves smoke transparency and yellow voting text/shadows. Raw model
colors are RGBA; the lit static cache now reads the correct input channels and
retains its separate output layout. Browser shots showed finite gray smoke and
neutral vehicle/prop colors. Original FX assets were not edited for these fixes.

Restart releases model buffers through correctly typed D3D calls and retrieves
XModel surfaces with an `int **partBits` output. `loadingnewmap` moves an already
connected client back to CONNECTED so acknowledgements continue while loading.
Vote reset visits every client slot, YES parsing uses the first input character,
and the HUD uses the same voted bit as the server. Its two lines have adequate
spacing. Chrome and the in-app browser displayed the countdown and counts,
approved a restart from opposite teams, loaded Toujane again and resumed moving
and firing. Screenshot: `out/fixes-vote-visible.png`.

The sustained test also exposed archive corruption: `SV_ArchiveSnapshot` never
advanced `nextArchivedSnapshotBuffer`, so older records all referenced overwritten
bytes. Retrieval could spin until its entity counter overflowed because exhausted
bit reads set `overflowed` without advancing `readcount`. Archive writes now
reserve their byte ranges; reads validate sizes, frame age, prior-frame links,
client/entity limits and bit exhaustion. The regression exercises 1,600 writes,
1,200 reads after cache eviction, buffer wrap, truncation and cyclic references.
The corrected dedicated server remained responsive with both browsers over
multiple minutes before and after the full map restart; one two-client sample
used 22.73% of one CPU core and 146.5 MiB in the server container.

Passing focused regressions: `test-client-clock-recoil.py`,
`test-fx-line-allocation.py`, `test-snapshot-archive.py`, `test-model-colors.py`,
`test-antilag-history.py`, `test-fx-curves.py`, `test-fx-drawing.py`,
`test-server-commands.py`, `test-impact-fx.py`. Native and WASM builds succeeded;
pre-existing linker warnings remain. Coverage is two simultaneous browser
players, not a 64-player load test or an exhaustive check of every FX primitive.

Publication check: the rebuilt local Docker images were applied on
`http://192.168.1.10:8088/`. Chrome and the in-app browser joined through the
normal server list, chose opposing teams and spawned; the room API reported two
players/two connections, and the in-app client fired a six-shot burst. Evidence:
`out/fixes-lan-gameplay.png`. All three isolated test containers were removed.

## 2026-10-08 — original-style browser startup

The browser loading overlay now uses a plain black background, the original
`images/logo_cod2.iwi` logo, “Loading…” and a thin progress bar. It transitions
to the existing native main menu with no artificial delay. The old loading card
and map-specific Spanish loading labels are removed. Error details and the
optional debug console remain available.

`extract-startup-logo.py data/main/iw_09.iwd` uses the engine's wavelet decoder
to convert the original 512×128 logo into a private PNG in `data/browser/web/`.
The gateway serves only the explicit logo URL, as it already does for the icon;
the image is not copied into the distributable web package. `build-docker.sh`
prepares it when missing, and `build-web.sh` includes `startup.css`.

The web and gateway containers were rebuilt and updated. The native server was
not restarted. The in-app browser displayed the black loading screen at 50%
and then reached the native menu. Screenshot: `out/startup-black-loading.png`.

## 2026-10-08 — scoreboard rectangles and team banners

The Tab scoreboard allocated four border rectangles but wrote six, then drew
each rectangle using fields shifted by one float. It now allocates all six
and accesses their named x/y/w/h fields. Team banner names and labels are copied
out of temporary dvar/localization/va buffers; previously the renderer tried to
load materials named `British (1 player)` and `German (0 players)`, producing
fallback stripes. The score columns now start below the objective text.

The WASM client and web image were rebuilt and the web container updated. In-app
browser inspection confirmed original team flags, regular borders, no stray
rectangle and no objective/header overlap. Tab release returns to gameplay.
Screenshot: `out/scoreboard-fixed.png`. The test client was disconnected and its
tab closed; the dedicated server was not restarted.

## 2026-10-08 — fixed death camera and three-second respawn

Dead movement states (6/7) incorrectly entered `PM_NoclipMove`, allowing held
movement to move the player through the map during death. They now consume
commands with zero velocity, preserve their bounds, clear ladder/mantle movement
and reset the weapon. The animated corpse remains a separate entity. The client
retains the last living camera position, angles and FOV until respawn; respawn
clears that saved view.

The private derived TDM script now waits three seconds and sends its server-time
deadline through `cg_respawnDeadline`. The client displays “Reapareces en 3…”,
2, 1 with a progressive black overlay, and clears it when the new player state
arrives. This is the requested browser behavior: the retail script uses a
two-second death delay with optional killcam. The room defaults now select
automatic respawn and no killcam. Original source IWDs remain untouched; the
patch is reapplied by `prepare-browser-bootstrap.py` with updated asset hashes.

Native and WASM builds succeeded. The in-app browser ran the full sequence after
`/kill`; holding forward during the countdown did not move the camera, and the
player automatically respawned with the normal HUD restored. Evidence:
`out/death-countdown-3.png`, `out/death-countdown-2.png`,
`out/death-countdown-1.png`, `out/death-respawn-complete.png`.
The final images were deployed to the LAN service and the same full sequence
was repeated successfully there. The test client was disconnected and closed.

## 2026-10-08 — visible grenade explosions

Fragmentation grenades were spawned with the bounce flag `0x1000000`, but the
missile collision, ground adjustment and bounce damping paths checked the FX
lifetime flag `0x10000`. Their ballistic trajectory therefore continued through
the ground; a browser diagnostic observed a detonation at Z = -4235.8 with a
valid explosion effect. The original FX files and renderer were already working.

All three physics checks now use the named grenade bounce flag. Bounce events
also send an encoded collision normal, as the client expects, with the surface
type in its own field. Smoke grenades waiting to land schedule their next think
relative to the current server time instead of absolute time 50.

Native and WASM builds succeeded. In the in-app browser, real thrown grenades
now detonate at terrain height (including Z = -12.0, 38.2 and 48.8 on Toujane).
The captured sequence shows the grenade, flash, smoke and ground scorch mark:
`out/grenade-ground-0.png` through `out/grenade-ground-23.png`; the selected
proof is `out/grenade-explosion-verified.png`. Temporary diagnostic prints were
removed and the final web/server images rebuilt for the LAN service. The test
client's temporary look binding was removed, then it disconnected and closed.

## 2026-10-08 — mounted MG42 interaction

Mounted weapons (entity type 9) now enter the usable-entity list without the
script-trigger flag. Their HUD hint material uses the same weapon + 4 index as
the server and cursor-hint decoder. The original use/drop strings show the
configured activate key (F by default).

Both server world-tag helpers now store translation in an actual fourth matrix
row; the old separate stack vector was read out of bounds by the 4x3 transform.
That broke the line-of-sight check and mounted muzzle/view positions. The client
now places its mounted camera at the model's tag_player rather than player feet.
MG42 controllers use typed entity states and interpolate consecutive snapshots.

The private asset closure now retains radiant/keys.txt. Map entities spawn after
Radiant fields, constants, animation data and the script VM are initialized, so
the original TDM script can remove DM/HQ/SD copies of mounted weapons. Previously
script_gameobjectname was discarded at spawn and three or four MG42s occupied
each location. The turret pool initializes before those entities are created.

Native and WebAssembly builds succeeded. An isolated localhost Docker match was
used for browser verification: all three Toujane MG42s showed their use hint,
mounted with F and released with F; the held Sten returned on exit. Aiming was
checked on the third gun. The first and third gun's firing effects and wall
impacts were observed. Evidence:
`out/turret-use-hint-verified.png`, `out/turret-firing-verified.png`, and
`out/turret-third-firing-verified.png`. Temporary tracing was removed before the
final builds. Original retail IWDs remain untouched.

Final web/server images are deployed at http://192.168.1.10:8088/ with asset
version `bootstrap-a2fda434c0192172`. The final LAN client loaded, joined TDM and
spawned successfully (`out/turret-lan-deployed.png`). The temporary test project,
networks and duplicate assets were removed; the browser client disconnected
and closed. The web healthcheck passed and the room advertises 64 player slots.

## 2026-10-08: bounded bullet-impact dust trails

Reproduced the reported smoke streaks in the LAN browser client. Shooting a
plaster wall produced chains of dust puffs stretching across the scene
(`out/impact-wall-before-0.png` through `out/impact-wall-before-3.png`).

`Emitter_UpdateEmitFx` independently advanced its emission position using an
accumulated interval, then reset its sampling timestamp relative to birth on
every frame. That repeatedly integrated already elapsed time and emitted far
beyond the real particle trajectory. It now samples the world-space displacement
already computed by `Particle_UpdateOrigin`, preserves the original spacing and
carries the un-emitted distance into the next frame. Zero/nonfinite spacing and
non-progressing floating-point increments cannot enter an unbounded loop.
`FX_AddEmitter` initializes the trail position in world space for both free and
bolted emitters. Original effect definitions, opacity, lifetime and assets remain
unchanged.

The incremental WebAssembly build and Docker web-image build succeeded (existing
compiler/link signature warnings remain). Only the web container was recreated;
the dedicated server kept running. On the deployed LAN build, Sten bursts against
a wall showed localized impact dust, debris and bullet marks, followed by complete
smoke dissipation. Evidence: `out/impact-wall-after-0.png` through
`out/impact-wall-after-3.png` and `out/impact-wall-faded.png`. The browser console
reported no errors during that check. This verifies the observed wall-impact
regression; it does not establish visual parity for every effect in the game.

## 2026-10-08: Toujane vehicle-area brush collision

Located the center piece between the Panzer II and Opel Blitz at approximately
(2843, 1634, 45). In TDM this is the destroyed Flak88 model. Its original inline
BSP collider is model *20, comprising six brushes. Before this correction, a
normal forward walk from (3090, 1634) crossed the model to (2698, 1622).

`G_ParseEntityField` put inline BSP model numbers into the 8-bit XModel precache
field, leaving `s.index.brushmodel` unset. `SV_SetBrushModel` therefore selected
the wrong collision submodel. The parser now stores the full inline handle in
the correct entity-state field and clears the unrelated XModel index.
`CG_ScriptMover` and its loop-sound positioning now identify inline brush models
using the `solid == 0x00FFFFFF` sentinel, rather than `constantLight`. This also
prevents brush handles from being rendered as unrelated precached XModels.
The original map geometry and retail assets are unchanged.

Native and WebAssembly incremental builds and both Docker images succeeded.
In an isolated localhost match, normal walking stopped at (2916, 1621, 104)
from the east and (2617, 1634, 105) from the west. Continued forward input left
both positions unchanged. Cheats were used only to position the observer;
noclip was not enabled. Evidence: `out/vehicle-collision-east-position.png`,
`out/vehicle-collision-west-position.png`, and
`out/vehicle-collision-verified.png`. The browser reported no console errors.
The specific red graphic reported by the user was not independently reproduced;
the inspected center piece renders without it in the corrected build.

The web and dedicated-server containers were recreated on the LAN endpoint
http://192.168.1.10:8088/ after checking that the server had no players.

The deployed LAN client loaded Toujane, joined the British team and spawned
without browser console errors (`out/vehicle-lan-deployed.png`). The temporary
Docker project and its networks were removed, and the browser client was
disconnected and closed after verification.

## 2026-10-09: scope alignment, view-weapon recoil and dropped items

Scoped reticles now use the final weapon axis in world coordinates. The former
relative weapon angles were projected against the world camera, moving the
scope off screen as the player turned. View-weapon updates now preserve the
recoil spring speed/offset and movement/idle filter state returned by
`BG_CalculateWeaponAngles`. Angular recoil is no longer also applied as a world
position offset, and damage kick uses the absolute client clock expected by
its timestamp.

Dropped items now process ground contact while `r.inuse` is set. The former
`active` check tested the pickup/use flag, so ordinary dropped weapons skipped
the settling path. Item movement evaluates the angular trajectory, constructs
a normalized orientation along the ground plane, and stops both trajectories
on landing. Newly dropped items start with their assigned angles and no ground
entity. The `AxisToAngles` declaration matches its vector-axis implementation.

`scripts/test-weapon-view-ground.py` compiles production functions with address
and undefined-behavior sanitizers. It passes 333 scope directions, 11,000 recoil
frames, 168 slope/yaw landings, freed-entity checks and four regression mutants.
The animation-pose regression also passes. Native and WebAssembly builds and the
bundled static/input/assets/LAN/audio checks succeeded; existing compiler/link
signature warnings remain. Startup CSS fixtures were updated to match the
existing page's stylesheet.

In the browser match, the scoped Lee-Enfield remained centered while turning
and moving and visible through five reload cycles. The Sten remained visible
through 224 shots and eight reloads, then switched normally to the pistol after
running out of ammunition. Death/respawn and dropped weapons were exercised,
and no browser console errors were recorded. Settling on slopes is covered by
the production-function regression; visual parity for every ground surface
has not been established. The test client disconnected before updating the
web and dedicated-server containers.

The current LAN address is http://10.14.10.29:8088/. Local access and the room
advertisement work, but a second Wi-Fi client's connection times out before
loading the page. Client isolation is a suspected cause, not a confirmed router
setting. The installed `cloudflared` can provide temporary external access with
`cloudflared tunnel --url http://localhost:8088`; share its generated HTTPS URL
and keep the process running. Anyone with that URL can access the game. No
public tunnel was started during these checks.

The hip-fire crosshair also used ADS position as opacity. At the hip that value
is zero, causing `CG_DrawCrosshair` to return before drawing the four side marks.
It now uses the remaining visibility returned by `CG_DrawWeapReticle`, applies
that fade to both center and side marks, and draws the scoped overlay once.
`scripts/test-hip-crosshair.py` passes four-part visibility across 15 spread
levels, overlay/ADS fading, five HUD gates and two regression mutants under
address/undefined-behavior sanitizers. The web build and its bundled checks pass,
and the updated web image is deployed; the dedicated server kept running.
In the deployed Toujane match, all four white side marks were visible with the
Sten at the hip, disappeared in ADS, and returned when leaving ADS. Evidence:
`out/hip-crosshair-deployed.jpg`. The browser reported no console errors. The
temporary viewport override was cleared and the test client disconnected and
closed after verification.

## 2026-10-09: restore climbing onto marked ledges and roofs

The reconstructed `Pmove` never called `Mantle_Check` or `Mantle_Move`, so a
marked roof edge could only be attempted with the normal 39-unit jump. The
wall detector also rejected ordinary hits with `allsolid == 0` and
`startsolid == 0`, accepting the opposite state. It now rejects traces that
start inside solid geometry and continues to the existing surface, angle,
ledge-height and clearance checks. Trace/results locals use their typed
structures for correct alignment.

Live movement now runs the mantle detector and gives its original animation
root motion control until the climb completes. Ground sliding and ladder
movement run outside that phase; weapon updates continue. `mantleStarted` is
reset per movement invocation so the server cannot repeat a stale destination
blocker. The dedicated server now loads the same original mantle animations
as the client during full animation-tree initialization. Jump height, map
geometry and retail assets remain unchanged.

`scripts/test-mantle-movement.py` passes 168 ledge/root-motion paths, nine invalid
ledge cases, 219 movement schedules, original jump-height checks and three
regression mutants with address/undefined-behavior sanitizers. Ladder (264
schedules/256 surface flags), step (1,024 entities), slide bounds and player
lifecycle regressions pass. The ladder source fixture now accepts indentation
changes. Native/WebAssembly builds, bundled browser-package checks and both
Docker images succeeded; existing compiler/link warnings remain.

An isolated localhost match used `devmap` only to position the observer on the
lower roof. Normal walking approached the marked edges (brushes 4308 and 4364),
then Space climbed the 44-unit rise from roof height 171 to 215. The client and
server agreed at (810.9, 333.0, 215.1) and (810.9, 480.0, 215.1). Continued
walking on the upper roof worked and the weapon returned visibly after climbing.
Evidence: `out/mantle-roof-position.jpg`, `out/mantle-roof-second-position.jpg`,
`out/mantle-roof-verified.jpg` and `out/mantle-browser-proof.json`. No browser
console errors were recorded. This verifies two marked roof edges, not every
ledge on Toujane or the exact roof reported by the user.

The test client and Docker project were closed/removed. The normal LAN web
and dedicated-server containers were recreated after confirming zero players.

## 2026-10-09: keep Space printable in the native console

The browser shell can retain its gameplay state after the native console or
chat opens. Its captured-key guard prevents Space on keydown, which cancels
the keypress that SDL uses for text input. The adapter now reads the native
state in a window capture listener, before that document guard, and updates
the shell when they differ. It uses the current shell state rather than a
cached copy, preserving corrections made by other input listeners. Character
events still come from SDL, so this adds no duplicate spaces.

The input contract test exercises native gameplay/menu transitions against
the actual pinned framework key guard with and without pointer capture. The
LAN fixture supplies the browser window surface, and the static test passes
the pinned framework source explicitly. Browser-package and performance-meter
checks pass. Only the web image needed updating; the served adapter hash
matches the source and the web container is healthy.

At the deployed LAN URL, physical Space key events separated a connect command
and the arguments in `/set console_space_test "UNO DOS TRES"`. Querying that
temporary client variable returned exactly `UNO DOS TRES` during a Toujane
match. The shell correctly reported menu while the console was open and
gameplay after closing it and pressing Space. Evidence:
`out/console-space-verified.jpg`. No browser console errors were recorded and
the test client disconnected and closed. The automated browser did not acquire
pointer lock; the captured guard path is covered by the contract test rather
than an observed locked browser session.

## 2026-10-09: review all weapon stats and tune LAN automatics

Audited all 21 packaged weapon definitions against the owner's original retail
CoD2 IWDs, honoring later archive overrides. The five carried automatics had a
3x head/helmet multiplier, enough for a close single-bullet kill against 100
health. Bren and MP44 also retained 40 body damage at every range and started
with tighter standing hip spread than SMGs. Activision's WWII multiplayer
documentation supplies class roles, not numerical weapon stats; the numerical
baseline here is retail CoD2 and the adjustments are custom LAN tuning.

`downstream/weapon-balance.json` defines `lan-balanced-v1`. Head/helmet damage
is now 2x for Sten, Thompson, MP40, Bren and MP44. SMG full-damage range ends
earlier (Sten/MP40 700, Thompson 600 engine units). Bren/MP44 fall from 40 to
28 body damage between 1,200 and 2,500 units, with slightly wider standing
hip spread. Sten/Bren/MP44 ADS spread is 0.2 degrees. Close body damage,
cadence, recoil, magazine sizes and reloads retain their original values.
All other packaged definitions remain byte-identical to retail.

The preparation and private-asset audit scripts apply/check only the reviewed
fields and reject unexpected retail values. Original IWDs are never modified.
Client and dedicated server use the same generated private archives, asset
version `bootstrap-15e0c44e4f483ef2`. `test-weapon-balance.py` passes 75,525
retail/balanced cases through the actual server bullet/player-damage functions
under address/undefined-behavior sanitizers, covering all 19 hit locations,
distance curves and range boundaries. It audits all 21 definitions and writes
`out/weapon-balance-audit.json`. This verifies damage behavior, not competitive
win rates. Shotgun figures are per pellet; turret file stats do not assert
player TTK because this reconstruction selects hit multipliers from the held
activator weapon. Mounted MG42 tuning remains unchanged.

The two-client test exposed identical browser `qport` values derived from
near-zero relative startup clocks. Since the gateway gives clients one base
IP, the server alternated their translated ports and eventually rejected
reliable commands. Browser `NET_Init` now assigns an independent random 16-bit
channel ID before `CL_Init` copies it, using browser crypto with an HTTP-safe
random fallback. The native initializer remains unchanged. The qport contract
test covers initialization order, boundaries and both random paths. Random
IDs retain the protocol's small collision probability; they are not a global
uniqueness guarantee.

After the WebAssembly rebuild, two clients on opposite teams remained stable
for several minutes without translated-port switching or reliable-command
errors. A Sten shot consumed exactly one round (32 to 31) and changed the
stationary opponent's predicted and server health from 100 to 30. Both browser
error logs were empty. Evidence: `out/weapon-balance-aim-verified.jpg`,
`out/weapon-balance-shot-verified.jpg`,
`out/weapon-balance-headshot-verified.jpg` and
`out/weapon-balance-browser-proof.json`. This live test verifies the Sten
head/helmet change; the other weapon curves are covered by the damage tests.

The weapon timer regression passes 60,000 schedules after its test fixture
was updated for existing animation timers. Weapon view/ground, hit-location,
recoil and bundled static/input/LAN/network/audio checks pass. The WebAssembly
build and both Docker images succeeded. Served WASM SHA256:
`92525d3969afebd0c2b0a07276598b284f73319ff4c3359b6f4e8e340c314f48`.
The isolated loopback test clients and Docker project were closed/removed.
The normal LAN stack remains running at port 8088 with the balanced assets.

## 2026-10-09: gametag, kill feed, final scores and killcam

The browser requests a gametag before loading the engine when no valid saved
name exists. It accepts an existing framework nickname and saves new names in
localStorage for that origin. Names are limited to the native 31-byte payload;
console command separators and control characters are rejected. Quoted startup
arguments preserve spaces. Disabled storage still allows the current session.
The JS loader and WASM now share a build-specific URL, selected from uncached
build metadata, so page reloads cannot combine a new loader with an old binary.

`CL_InitCGame` now passes the snapshot sequence, command sequence and local
client number in the order expected by `CG_Init`. Obituaries use typed client
records and the actual names and teams. The upper-left game message window
is drawn below the team scores. Browser deaths show readable white
`attacker > victim` text; native builds retain weapon icons. Original console
icon control decoding is corrected, but those materials still render black in
the current web renderer, hence the browser text presentation.

Hidden scoreboards now return false instead of suppressing the subsequent HUD.
Intermission displays the summary automatically and requests fresh scores on
entry, even without Tab. Average team ping is placed in the Ping column.

The dedicated server enables the original `scr_killcam` flow. Archive deltas
use a recent full base and periodically create fresh bases, preventing the
history from becoming unreadable after ring eviction. The script's PS-offset
getter returns its own field. HUD horizontal and vertical anchors use the
correct argument order, centering the original KILLCAM title and skip prompt.
Original rules still omit replay for world deaths and a match-ending death;
shortly after joining, unavailable history can shorten a replay.

New checks cover nickname persistence/validation, coherent JS/WASM build URLs,
all 64 local identities, 4,096 killer/victim combinations, console icon decoding,
16 scoreboard states, HUD placement at three scales and 10,000 archive frames.
Sanitizers and deliberately broken variants reject the name/stride, alignment,
hidden-scoreboard and expired-base regressions. Existing snapshot, anti-lag,
player lifecycle and bundled browser package checks pass. Native and WASM
builds succeeded; original private archive hashes remain unchanged.

An isolated loopback match with two browser clients verified saved gametags,
live kill notifications, the original attacker-view killcam, automatic final
scores (3 points/0 deaths versus 0 points/3 deaths) and the next-round briefing.
Evidence: `out/gametag-prompt.png`, `out/kill-feed-verified.png`,
`out/killcam-verified.png` and `out/final-summary-verified.png`. Test-only devmap
and a score limit of three are confined to the isolated Compose override.

## 2026-10-09: muzzle flash attachment transforms

`FX_GetBoneOrientation` treated an `orientation_t` (origin followed by axes) as
a 4x3 matrix (axes followed by origin). World position therefore entered the
rotation calculation, and tag translation used the wrong rows. It now composes
the bone rotation with the parent's typed axes and explicitly builds the 4x3
matrix for the translated tag. The existing quaternion conversion helper
preserves nonunit quaternion weights. Invalid DObj handles are rejected.

The viewmodel orientation provider now reads `cg->viewModelOrigin`, matching
the rendered weapon, instead of stale byte offsets into the client globals.
World entities likewise use typed centity indexing. Original muzzle-flash
definitions, textures, sizes, randomness and weapon stats remain unchanged.
This corrects the shared transform used by muzzle flashes and other effects
attached to weapon bones.

`test-muzzle-fx-transform.py` passes 1,026 world/viewmodel transforms against
independent Rodrigues rotations, including translated cameras, pitch/yaw/roll,
nonunit quaternions, root attachments and missing bones. It checks 1,100 cached
orientation queries across changing frames and rejects origin/axis layout
regressions under address/undefined-behavior sanitizers. Existing FX drawing,
impact/tracer and weapon-view/ground checks pass, along with the full browser
package checks. Native and WebAssembly builds succeeded.

In an isolated Toujane browser match, Sten bursts showed flashes at the muzzle
from the hip and in ADS after a 45-degree turn. Bren bursts also showed their
flash at the barrel after a further turn. No browser console errors were
recorded. Evidence: `out/muzzle-fx-hip-0.png` through `out/muzzle-fx-hip-7.png`,
`out/muzzle-fx-ads-0.png` through `out/muzzle-fx-ads-5.png`, and
`out/muzzle-fx-bren-0.png` through `out/muzzle-fx-bren-5.png`. The selected
proof is `out/muzzle-fx-verified.png`. Other weapon transforms are covered by
the shared-path tests; they were not individually tested in the browser.

## 2026-10-09: regression audit and browser discovery recovery

Reviewed the running services and the existing 68-script regression suite.
Three stale fixtures were repaired: footstep checking now tolerates whitespace,
corpse initialization includes the mantle animation dependency, and the bot
foundation test no longer requires obsolete eight-player/12-slot prose. Their
runtime and sanitizer checks pass. Audio parsing of all 470 files and the
32-bit snapshot-layout test pass in the Linux native builder; the host lacks
ffprobe and does not provide the required 32-bit Linux headers. Other scripts
passed, including 12 real map rotations. Evidence: `out/audit-regressions.json`
and `out/audit-linux-tests.log`. The six gateway tests and three WebAssembly C
harnesses for light bounds, pixel uploads and font callbacks also passed.

Found a stale two-player limit in the WebSocket rejection diagnostic. It now
uses the room's advertised capacity, falling back to 64, so a connection error
with two occupants is not mislabeled as a full 64-slot server. Discovery now
retries empty/transient room replies within a 12-second deadline, with request
timeouts, stale-generation rejection and bounded allocations. New tests cover
startup delays, malformed/duplicate rooms, stale refreshes, unavailable hosts
and the total deadline. Command-list debug dumps now require DBGSPAM, preventing
partial diagnostic lines from leaking into ordinary browser logs.

In the isolated Toujane match, the scoped Lee-Enfield stayed centered after a
turn, fired five shots (10 to 5 rounds), and began its original five-round
reload without disappearing. Stopping the isolated dedicated server returned
the client to a normal connection-error dialog. No WebAssembly trap was seen.
The native and browser builds and canonical package checks passed before the
concurrent Carentan integration. Final compilation/deployment is coordinated
with that work to preserve its assets and shared-file changes.

This audit does not establish zero bugs. Existing missing-material/technique
warnings remain, and the opt-in meter varied roughly 20–60 FPS across menu,
scoped/outdoor views and background compilation on this Mac. Longer sessions,
more real players and other devices remain outside this verification.

## 2026-10-09: multiplayer rooms created only by users

Stopped all three running rooms and removed automatic room creation from the
supervisor's startup. Every start/restart now serves an empty room list until
a user chooses Start New Server. All three room IDs (0–2) are available for
explicit creation, and all rooms expire after five minutes empty. The browser
creation callback now accepts ID 0 rather than rejecting the first user room.

The room lifecycle/map checks cover empty startup, three user slots, full
capacity, reuse and idle cleanup of slot 0. Browser creation and connection
error checks and all six gateway tests pass. The WebAssembly build and canonical
browser package checks pass; rebuilt web/server images are deployed.

The deployed HTTP API started empty, created a Toujane room with ID 0 on request,
and returned its native getinfo reply through the WebSocket/UDP gateway. After
another server restart, `/servers` returned `{"rooms":[],"maxRooms":3}` and
the supervisor had no dedicated game children. Evidence:
`out/user-created-rooms-check.mjs`, `out/user-created-rooms-verified.json`,
`out/user-created-rooms-web-build.log` and
`out/user-created-rooms-images-build.log`.

## 2026-10-09: map selection keeps the creation menu visible

Clicking the map list from the settings panel dispatched an outside-panel
click and cleared the UI input catcher while sibling menus were still visible.
`Menus_HandleOOBClick` now releases the catcher and cinematics only after the
last menu closes. The list's feeder count also now receives its float ID
directly; interpreting those bits as an integer prevented selecting Toujane.

`scripts/test-ui-menu-clicks.py` exercises the actual C handlers under address
and undefined-behavior sanitizers: both map selections, keyboard navigation,
sibling/empty-space clicks and popup closure at three screen scales. Mutations
that restore either defect fail the test. The existing map-selection checks,
WebAssembly build and canonical browser package checks also pass.

The rebuilt web image is deployed and healthy. Browser verification selected
Toujane and then Carentan with the menu remaining visible. Evidence:
`out/map-menu-toujane-verified.png`, `out/map-menu-carentan-verified.png` and
`out/map-menu-black-screen-web-build.log`. This check verifies map selection;
it does not establish a new playable match. The dedicated server was left
running while the web client was updated.

## 2026-10-09: room ownership, smoke, combat feedback and rendering stalls

Join Game now offers **Eliminar mi server** for a room created by this browser.
Creation returns a private ownership token; discovery returns only a public
instance ID. The browser persists the token with that instance ID, and the
supervisor validates it before stopping the room. Reusing a room slot does not
transfer ownership. Successful deletion disconnects only that room's sockets.
The HTTP lifecycle, browser persistence/error paths and six gateway tests pass.
Two connected browser clients confirmed deletion through the native menu,
an empty room list and the other client's normal disconnect dialog. Evidence:
`out/delete-own-server-menu.png`, `out/delete-own-server-success.png`.

The smoke crash came from four dedicated-mode checks reading the address of an
import pointer instead of its dvar. This skipped `FX_InitSystem`, leaving an
effect-registration callback null when the grenade exploded. These branches
now use typed dvar reads. Dedicated model parsing also skips renderer-only
registration, and combined smoke entity flags expire through bitmask checks
without freeing turrets. Real smoke grenades exploded in Carentan and Toujane
while their dedicated processes and clients continued running. Evidence:
`out/smoke-registration-native-fixed.log`, `out/smoke-carentan-final.png`,
`out/smoke-toujane-final.png`, and the post-render-change repeat in
`out/smoke-toujane-final-render.png`. `test-smoke-effect-server.py` checks renderer-free
parsing, all four dedicated reads and expired effect cleanup with sanitizers.

Obituaries now have a reliable all-client server command alongside the retail
event. The browser validates and queues it, uses current typed client info and
avoids replaying the snapshot event during killcam. An expired center message
clears instead of drawing with a null fade color. Three real browser clients
confirmed that an uninvolved spectator sees Mati Test kill Gaston Test; the
killer's center message subsequently disappeared. Evidence:
`out/global-kill-spectator.png`, `out/local-kill-message.png`,
`out/local-kill-expired.png`. The kill-feed tests cover all 4,096 attacker/victim
pairs; the reliable queue test delivers ten kills to each of 64 simulated
viewers without duplicates. This is not a 64-player gameplay load test.

As explicitly requested, mounted MG42s now overheat after five seconds of
continuous fire and cool fully in 2.5 seconds. The gun retains its heat across
users and replicates it for the orange/red heat bar and **Enfriando...** label.
Actual mounted firing in Toujane reached the blocked state, cooled, then fired
again. This is a custom rule; the owner's original weapon definitions remain
untouched. Evidence: `out/mg42-overheated-live.png`, `out/mg42-cooled-live.png`,
and a repeat after the final render change in
`out/mg42-overheated-final-render.png`.

Visible teammates have projected green head labels without a crosshair hit;
active players no longer draw the old duplicate crosshair label. Incoming hits
use the original red arc material, rotate toward the attacker's position and
fade over the existing duration. The script supplies bullet travel direction,
so the display adds 180 degrees to locate the source. Real off-crosshair tags
and a front-right body hit were verified in Carentan. Evidence:
`out/friendly-tag-off-crosshair.png`, `out/directional-hit-live.png`.
`test-gameplay-feedback.py` exercises heat/cooling, independent guns, four hit
directions and rotated views, fading/scopes, slot-63 teammate projection and
expired kill text under sanitizers.

Rendering now caches unchanged static vertex buffers on the GPU and interleaves
converted colors with model/UI vertices. More significantly, Emscripten's
legacy GL path reused a single temporary index buffer per size while previous
draws still read it. Uploading the next draw's indices forced GPU synchronization.
The pinned SDK patch orphans that buffer's storage before each upload, retaining
its power-of-two capacity. Diagnostic-only native skeleton/trigger logs are also
behind `DBGSPAM`. The real linked GL flush test executes 600 draws against a
simulated busy GPU and verifies fresh storage and unchanged indices; static
vertex cache and color tests check invalidation, dynamic paths, tint/alpha and
interleaved offsets.

In this Mac's isolated browser, Carentan's courtyard at eye position
`1959 2299 36`, yaw `-135`, with a Thompson and a 640x480 render canvas went from
about 37.8 ms of main-thread work to 3.9 ms. Driver profiling located about
42.3 ms in index uploads before the final patch; that fell to 0.85 ms afterward.
The Toujane MG roof measured 2.2 ms of main-thread work and 0.33 ms of uploads.
The post-change reports contain 600 samples, one active browser renderer and no
concurrent compilation. Browser viewport dimensions changed, while the render
canvas and Carentan position stayed fixed. The meter's FPS counts callback
intervals, so its approximately 116–120 reading is not a promise of that many
distinct rendered game frames. These local measurements do not establish
performance on other hardware or in populated matches. Evidence:
`out/carentan-before-gameplay.png`, `out/carentan-index-stream-live.png`,
`out/carentan-index-stream-performance.json`,
`out/toujane-index-stream-live.png`,
`out/toujane-index-stream-performance.json`.

Native and WebAssembly builds passed. The final canonical browser package,
original/private asset boundaries and linked GL checks passed in
`out/gameplay-complete-web-build.log`; final image builds are recorded in
`out/gameplay-complete-images.log`. The deployment is recorded in
`out/gameplay-complete-deploy.log`. The deployed browser metadata matches the
verified package, the native binary matches its SHA-256, the web service is
healthy and the LAN URL returns HTTP 200. `/servers` contains zero rooms and
the supervisor has no dedicated game children. The isolated test stack was
removed; Cloudflare remains stopped. Evidence:
`out/gameplay-complete-deployment-verified.json`,
`out/gameplay-live-native-processes.log`,
`out/gameplay-complete-qa-cleanup.log`.

## 2026-10-09: original MG42 reticle, longer cooling and English interface

The reconstructed turret HUD used the solid white material as an 8x8 aim marker,
which appeared as a gray square. It now reads the mounted entity's weapon,
registers that weapon and draws its original center reticle at the original size.
The owner's MG42 definition specifies `gfx/reticle/mg42_cross.tga`, 32 pixels;
both its material and texture are present in the private archive. Missing or
invalid turret/reticle data does not draw a solid-square fallback.

Full MG42 cooling now takes eight seconds after the existing five seconds of
continuous fire. Integer heat units preserve that exact timing at different
server tick lengths. Gun ownership of heat and the existing replication remain
the same. The actual HUD/heat sanitizer checks verify the mounted weapon's
32-pixel material, boundary entity IDs, the exact eight-second firing block,
independent guns and long idle intervals.

All added interface text is in English: first-entry player-name prompt,
download/cache/preparation progress and accessibility labels, update notices,
**Delete my server**, **Cooling down...**, browser errors, supervisor API errors
and host-updater messages. Gateway validation and disconnect messages also use
English; all six gateway checks pass in `out/mg42-english-gateway-check.log`.
The loader uses English number formatting and the
owner's original English localization. Progress, creation/deletion, updater
notice and host-update checks pass with their English expectations. The private
asset check verifies the English server action and preserved original assets.

In a real isolated Toujane match, the mounted MG42 displayed its original
reticle, overheated after sustained fire, still showed **Cooling down...**
roughly 4.4 seconds after overheating, then resumed firing after the longer
pause. Captures were taken 5.79, 9.41 and 14.23 seconds after initial firing.
Evidence: `out/mg42-original-reticle-live.png`,
`out/mg42-english-cooling-start.png`, `out/mg42-english-still-cooling.png`,
`out/mg42-english-firing-resumed.png`.

The native and browser clients compiled successfully. Canonical package checks
passed on the host against the unchanged framework checkout at the pinned
`53bc7e6eeef1ae35dcf3b25dea4e3ec0ab46726f` after updating the fixtures that
expected Spanish error messages. Evidence:
`out/mg42-english-native-build.log`, `out/mg42-english-web-build.log`,
`out/mg42-english-package-check.log`, `out/mg42-english-private-assets.log`,
`out/mg42-english-images.log`, `out/mg42-english-deploy.log`.
The deployed metadata and native SHA-256 match the verified builds; the web
service is healthy, the LAN URL returns HTTP 200 and the invalid-name API error
is English. The host was left with zero rooms, and the isolated test stack and
browser tab were closed. Evidence:
`out/mg42-english-deployment-verified.json`, `out/mg42-english-qa-cleanup.log`.

## 2026-10-09: display-paced rendering and higher-quality browser graphics

The performance meter previously counted RAF callbacks, including callbacks
where `Com_Frame` returned without rendering. It now counts real backend draws
and reports p95/p99, maximum frame time and frames exceeding 33.34 ms. The
desktop `com_maxfps=85` cap skipped every other callback on this 120 Hz display:
the corrected baseline measured 60.6 rendered FPS. Disabling that desktop cap
uses the browser's existing display cadence and raised actual rendering to 120
FPS. The FPS regression test includes skipped callbacks.

The default canvas increases from 640×480 to 1280×720. The engine's mode table,
viewport, projection and render targets use the selected browser resolution.
Full texture resolution replaces the DX7 automatic texture reduction; 4×
anisotropic filtering is clamped to the supported extension limit, and the
browser requests anti-aliasing. The measured WebGL context confirms anti-aliasing
is enabled. Absolute mouse coordinates are scaled to the native 640×480 UI
coordinate space, with relative aiming unchanged; six resolutions, boundaries
and gameplay deltas pass under ASan/UBSan. Real HD menu map selection, server
creation, team/weapon selection and turret mounting were verified.

The browser caches repeated texture bindings, texture sampler settings and
unchanged 4×4 uniform matrices. Texture deletion, context restoration, shared
textures across units, matrix mutation and ranged upload fallback invalidate
the corresponding state. The focused state-cache tests execute the actual
adapter implementation. LAN client settings use a 25,000-byte/s rate and up to
60 input packets/s, replacing the modem-oriented defaults. Original server
snapshot cadence and gameplay definitions remain intact.

Tests ran on this Apple M4 Mac with 16 GiB RAM, one active browser renderer and
no concurrent compilation. The corrected Carentan courtyard baseline at eye
position `1959 2299 36`, yaw `-135`, was 60.6 FPS at 640×480, frame p95 18.74 ms.
At 1280×720, full textures, anti-aliasing and 4× filtering, the same view measured
119.99 FPS, p95 10.28 ms, maximum 15.10 ms, main-thread mean 5.54 ms. Three
overlapping smoke grenades, Thompson fire and a sweep across the surrounding
buildings measured 120.00 FPS, p95 10.34 ms, maximum 13.69 ms. Sustained mounted
MG42 fire in Toujane measured 120.01 FPS, p95 9.86 ms, maximum 10.26 ms; its
original reticle and eight-second cooldown remained visible. Each reported
window contains 600 actual rendered frames and no frame exceeded 33.34 ms.
These tests use one network player and do not establish performance for 64
simultaneous players. A temporary isolated GSC bot fixture did not establish a
populated test and was removed from the active QA configuration.

Evidence: `out/performance-carentan-baseline.json`,
`out/performance-carentan-hd-steady.json`,
`out/performance-carentan-hd-combat.json`,
`out/performance-toujane-hd-mg42.json`, and their screenshots. Web compilation,
canonical package checks, private asset boundaries, gameplay feedback, model
colors, static vertex caching, HD input and GPU state-cache tests passed;
see `out/performance-hd-input-link.log`,
`out/performance-hd-package-check.log`,
`out/performance-hd-feedback-check.log`,
`out/performance-hd-model-check.log`,
`out/performance-hd-vbo-check.log`,
`out/performance-hd-private-assets.log`.

Only the web image was rebuilt and recreated. The live build metadata, adapter
and WASM SHA-256 match the verified package; the web service is healthy and the
LAN URL returns HTTP 200. The deployed native binary still matches its existing
verified build. The production menu renders at 1280×720, the host currently has
zero rooms, the isolated QA stack was removed and Cloudflare remains stopped.
Evidence: `out/performance-hd-images.log`, `out/performance-hd-deploy.log`,
`out/performance-hd-deployment-verified.json`,
`out/performance-hd-production-menu.png`, `out/performance-hd-qa-cleanup.log`.


## Five selectable multiplayer modes (2026-10-09)

Start New Server now cycles between Capture the Flag, Deathmatch, Headquarters,
Search and Destroy and Team Deathmatch. TDM remains the initial selection.
The original Game Type item uses ownerdraw 245; its key handler incorrectly
used 244, so clicking it previously did nothing. The corrected handler and
creation action use the selected type's real ID, independent of file order.
Both maps remain selectable in every mode.

The browser POST, gateway allowlist, room supervisor, per-room config and
launch arguments carry the selected `gametype`. Omitting it still defaults to
TDM. Unsupported mode IDs and command strings are rejected before launch.
Each room's complete and current rotation retain the selected mode, and the
room list recognizes every supported type after a map change. Startup still
has no rooms; ownership, deletion, 64 slots and the three-room limit remain.

The derived private closure adds all five original GSC/TXT definitions,
settings and server-information menus, plus their objective models, shaders,
effects and sound dependencies. The two generated arena descriptors advertise
all five types. Owner archives remain untouched; the reviewed TDM respawn
change remains confined to TDM. The resulting four archives total
275,564,709 bytes, with 4,369 entries and 518 original sound files.

HQ and SD initially stalled while parsing `_objpoints.gsc`, whose last line
is a comment without a final newline. The reconstructed lexer kept refilling
the retained token at EOF. Returning the existing LAST_MATCH state once
finishes that token and then reaches EOF normally, without rewriting scripts.
The real lexer regression compares token sequences with/without a final newline
for every packaged GSC and five synthetic cases (96 cases), under ASan/UBSan.
The native menu regression exercises all five choices in both directions,
wrapping, map/type creation arguments and invalid indices under ASan/UBSan.
Both checks are part of `scripts/test-static.sh`.

Verification: isolated native rooms started in all ten map/mode combinations,
and each rotated to the other map while retaining its mode. Test rooms were
removed afterwards. Evidence: `out/game-modes-live-matrix.json` and
`out/game-modes-live-matrix.log`. The complete WASM/package checks, native
build, gateway suite, room supervisor checks and private resource audit passed.
The in-app browser cycled every mode while retaining Carentan, created an HQ
room on Carentan and spawned as American with a Thompson. It also created SD
on Toujane and spawned as British with a Sten. The original information screens
showed HQ's 600/30 settings and SD's 10-point limit. Browser error/warning lists
were empty. Evidence: `out/game-modes-menu-sd.png`,
`out/game-modes-hq-info.png`, `out/game-modes-hq-gameplay.png`,
`out/game-modes-sd-info.png` and `out/game-modes-sd-gameplay.png`.
CTF and DM were also created on Carentan through the native menu; both
completed character/weapon selection and spawned with a Thompson. Their
original information screens and HUD showed CTF 5/30 and DM 50/30 settings.
No browser errors or warnings were recorded. Evidence:
`out/game-modes-ctf-info.png`, `out/game-modes-ctf-gameplay.png`,
`out/game-modes-dm-info.png` and `out/game-modes-dm-gameplay.png`.
This establishes selection, creation, startup, player entry and rotation;
complete objective rounds with multiple players need separate coverage.


The tested web/server/gateway images and matching staged archives were installed
on the live LAN host at `http://192.168.1.60:8088`. Build ID:
`e195fe81e9ee2f55ceb022ee2d4999ada56b5548fae4daf4069452b687590806`.
The native image binary matches the compiled output; served metadata and all
four installed private archive hashes match the tested package. The coordinated
restart was announced because one player was connected, and it ended that
room. Startup was empty afterwards; clients must reload instead of issuing
native reconnect. A fresh LAN client opened Start New Server and changed TDM
to Search and Destroy, with both maps visible and no browser errors/warnings.
Evidence: `out/game-modes-deployed.json`,
`out/game-modes-deployed-menu.png`. The isolated QA containers and agent test
tabs were removed. The previous derived archives are retained locally for
recovery under the directory recorded in `out/game-modes-deployment-staging.json`.

## Sprint, aimed shots and movement recovery — 2026-10-09

The PC controls now use Shift for sprint/hold breath, V for melee and H for
quick messages. Q/E remain lean, with the other original controls preserved.
This follows the [Black Ops II PC manual](https://www.callofduty.com/content/dam/atvi/callofduty/blackops2/cod-bo2/manuals/Manual_PC_EN.pdf)
while retaining CoD2's leaning controls. The original CoD2 Shift melee binding
is replaced in both reviewed copies of `default_mp.cfg`. The Shoot menu shows
“Sprint / Hold Breath”; omitted `maxPaintChars` no longer hides binding labels,
and the native UI consumes pending remap keys before gameplay receives them.

Sprint applies a 1.45 speed multiplier to forward movement while standing,
grounded and ready to move. Aiming, firing, melee, reloading, using, grenades,
leaning, ladders, mantling and incompatible player states cancel sprint.
Client prediction and the native server share the eligibility calculation.
Existing body run clips follow movement speed; a blended lowered weapon carry
adds hand animation without changing player/user-command structure layouts.
Shift continues to hold breath when aiming. Sprint has no stamina limit.

ADS incorrectly enabled the snapshot's local player-state event path alongside
prediction. The gate now uses the actual follow/interpolation flag, preventing
duplicate local shot effects while keeping remote events intact. Browser room
addresses no longer bypass `cl_maxpackets` as LAN traffic: fractional pacing
honors 60 packets/s, includes unsent commands and avoids catch-up packet bursts.

Long browser/server stalls exposed a second movement issue: clock recovery
reset prediction to an older snapshot. Browser recovery now keeps time
monotonic and limits commands to the newest received snapshot plus 200 ms,
matching the native server's accepted command horizon. A prolonged update gap
can pause prediction at that horizon; subsequent recovery does not rewind it.
Native clock behavior is unchanged.

Verification passed the canonical WASM/package build, native build and private
asset audit: four derived archives, 4,369 entries, 518 original sounds and
275,564,718 bytes. Regression coverage includes 32 shot-path combinations,
27 sprint interruption cases, 66 replay schedules, 256 remap routes, packet
pacing at four frame intervals, and 42 normal plus 42 delayed clock schedules.
Shot, pacing and clock tests also reject the former behavior as mutants;
extracted C checks run under ASan/UBSan.

The final browser build verified physical Shift+W at approximately 274 units/s
versus 190 walking, with zero observed prediction error during those movement
checks. Three aimed shots consumed three rounds and produced three local fire
events. V played melee, aiming restored the normal sights, and sprint carry
remained visible. The Controls menu remapped Shift to Shift/J, cleared it and
restored Shift. Browser warning/error lists were empty. QA-only native pauses
of 124 and 662 ms recovered without backward command/time samples in the
retained 454-sample, 125.25-second trace. A 600-frame Carentan sample averaged
114.7 FPS, with 11.22 ms p95 frame time and no frames over 33 ms. These are
local observations, not guarantees for every device or multiplayer load.
Evidence: `out/gameplay-browser-diagnostics.txt`,
`out/gameplay-movement-verified.json`, `out/gameplay-controls.png` and
`out/gameplay-sprint.png`.

The user explicitly requested “Restart now” while two players were connected,
authorizing this exception to waiting for empty rooms. The live services were
restarted and their matches ended; clients must reload. Served build ID:
`2952a1c8e135703c356d5efa63d22579b400cdcce7d22a04b29bd4649812dfb6`.
Native SHA-256:
`ac4c70c7401fefd72c8df99f8fd76955916239ba77064e1893a96ae2661f0de4`.
The LAN endpoint remains `http://192.168.1.60:8088`. Served manifests, four
installed archive hashes, native binary hash and production mounts match the
tested package (`out/gameplay-deployed.json`). Recovery copies of the previous
site, native binary and derived archives are recorded in
`out/gameplay-deployment-staging.json`. QA containers and the agent test tab
were removed after verification. No Cloudflare tunnel was started.

## Weapon firing modes and wall corners — 2026-10-09

The weapon loader already populated the retail `semiAuto` setting, but the
reconstructed firing state machine never used it. Semi-automatic rifles and
pistols now consume one shot per trigger press. A trigger latch uses an unused
bit in the existing replicated player-state flags, so native authority and
browser prediction replay the same release/re-press history, including presses
during shot recovery and subdivided commands. A held shot stays in FIRING
until release; unrelated reload, offhand and rechamber timers continue normally.
Automatic fire can restart on the command that expires its original fire timer,
removing an extra READY command that made cadence depend on input frame rate.

Every one of the 29 packaged weapon definitions was audited against the last
retail IWD override and the existing reviewed balance profile. Original clip
sizes, fire delays, bolt timing and reload timing remain. The prior LAN damage
and spread adjustments for five automatics remain, as does the requested
mounted-MG cooling feature. This is not a claim that all stats are unmodified
retail values. Evidence: `out/weapon-fire-modes-audit.json`.

Angled brush planes previously expanded a square AABB while triangle meshes
swept an upright capsule, adding false shoulder overlap at clear wall corners.
Both now use capsule support. Finite mesh borders also had a swapped endpoint
approach product, a cylinder equation including vertical speed, flat face
normals at round endpoints, wrong endpoint heights and an unclamped negative
contact fraction. Endpoint hits now use the horizontal dot product, horizontal
speed, radial contact normal and actual endpoint height. Face fractions are
clamped and initial overlap is reported. Axis wall/floor bounds, point traces
and real blocking geometry remain covered by the regressions.

Verification passed the complete canonical WASM/package checks, native build,
existing weapon balance audit, weapon timers, offhand, step/slide and mesh-edge
regressions. New extracted-code checks exercise 20 guns over 2,640 firing and
reload schedules, 4,608 brush corner slides and 2,208 finite mesh border cases
under ASan/UBSan. Mutants restoring the ignored semi-auto flag, extra cadence
frame, square brush shoulders and erroneous border math fail these checks.
The new checks are included in `scripts/test-static.sh`.

The isolated browser used the final client and native images on loopback 8089.
On Carentan, holding the Garand trigger for 1.3 seconds consumed one round
from the hip and one while aiming (8 -> 7 -> 6), producing two local fire
events. A 950 ms Thompson hold consumed ten rounds and produced ten events.
On Toujane, a 1.8 second Lee-Enfield hold consumed one round (10 -> 9) and
played its bolt animation. Physical sprint, forward/strafe wall sliding and
lateral escape were sampled near Carentan courtyard wall pillars and a Toujane
building corner. Movement continued along the walls and away from contact.
These sampled locations do not certify every map corner. Background browser
frame rates are not a performance benchmark. Browser warning/error lists were
empty. Evidence: `out/weapon-corners-qa.json`,
`out/weapon-corners-garand-held.png`, `out/weapon-corners-garand-ads.png`,
`out/weapon-corners-carentan.png`, `out/weapon-corners-toujane.png` and the
two map diagnostics files listed in the QA record. The QA tab and containers
were removed before deployment; QA-only devmap/config/binary mounts did not
enter the production services.

Following the user's earlier explicit restart preference, a coordinated live
restart was announced and applied while two players were connected. Their
match ended and clients must reload. The tested candidate images were tagged
for production and recreated without another build. The previous live images
remain as `local/cod2-wasm:before-weapon-corners` and
`local/cod2-native-server:before-weapon-corners`, with their exact IDs recorded
in `out/weapon-corners-previous-images.json`.

The LAN endpoint remains `http://192.168.1.60:8088`. Served build ID:
`0a8eb87c29c24da1c26a0d5b14b14f279c58f427e18176421d2bdf407364d092`.
Native SHA-256:
`55209e8509cba7c05c1bc263f89725330da6a2169be579c3518a52d24b3a0e11`.
The served metadata matches the tested package, all four installed archive
hashes match, the native binary matches and production has only the expected
read-only private asset mount. All three services run and the web health check
passes. The startup room list was empty. Evidence:
`out/weapon-corners-deployed.json`. The four private archives remain unchanged
at 275,564,718 bytes, 4,369 entries and 518 original sounds. No Cloudflare
tunnel was started.


## Lossless client/server performance pass — 2026-10-09

No resolution, texture mip levels, geometry, drawing distance, filtering,
antialiasing, lighting or effect-count settings were reduced. The private
archives were not regenerated. New WebGL state caching skips identical valid
capability/depth/cull/color-mask submissions and vec4 uploads. Uniform aliases
share locations, scalar writes invalidate vectors, and relinking/context
restoration invalidate uniforms. Invalid enum submissions still reach WebGL.

Network compression now derives cached codes from the original seeded Huffman
encoder and packs those exact bits. `test-huffman-cache.py` checks 4,096 cases
against the original bytes, every symbol, tail padding, roundtrips and buffer
canaries under ASan/UBSan; the wrong-code mutant fails. The ARM host component
benchmark measured 6.28x for zeros, 14.81x for a snapshot-like distribution and
17.96x for random bytes. These are encoder speedups, not total game/FPS gains.
See `out/huffman-performance.json`.

Dedicated `Com_Frame` waits for the next tick using `NET_Sleep`/UDP `select`.
Packet events still dispatch immediately when the socket wakes. Fixed-time,
scaled-time and browser/client frame policies retain their former behavior.
`test-server-frame-wait.py` covers all 10..1000 Hz residual deadlines and an
actual UDP early wake. The original 20 Hz simulation is retained: changing it
blindly would alter item physics, turret turning and weapon/sound timers that
assume 50 ms steps. Local server fragments now use at most eight datagrams per
send call, including the initial packet. Public UDP still uses one at a time.
`test-server-fragment-pacing.py` compares 1,776 original-wire messages, checks
batch limits, zero-length terminal fragments and send failures.

The shared native UDP socket requests a 2 MiB receive buffer. This Docker host
clamps the effective size to 425,984 bytes, compared with its 212,992-byte
original default. The gateway's half-core quota produced CPU throttling under
synthetic bursts; production now permits two cores for gateway bursts and
three across up to three room processes, on this eight-core Docker host.
Memory limits remain unchanged.

Two QA rooms, Carentan and Toujane, with one active Carentan client and 300
internal UDP info requests at 20/s were sampled for 15 seconds per build.
Server-container CPU fell from 28.89% to 22.20% of one core (23.14% lower), with
no CFS throttling. Local info-response RTT p95 fell from 8.218 to 0.343 ms;
p99 fell from 9.198 to 0.771 ms. One candidate response reached 47.01 ms, so
this is not a zero-latency claim. These probes do not measure client-to-screen
latency or a crowded match. Evidence: `out/realtime-server-baseline.json` and
`out/realtime-server-candidate.json`.

A Carentan view at eye (1959,2299,36), yaw -135, with a Thompson retained the
same 252 draws, 1280x720 canvas and antialiasing. Mean client main-thread time
was 7.299 ms before and 6.789 ms after in 600-frame samples at approximately
30 FPS. The baseline browser was background paced; the candidate was temporarily
capped to match that sampling cadence, then restored to `com_maxfps 0`.
Their RAF scheduling differs, so do not compare frame-interval tails or claim
a 30-to-120 FPS optimization. An uncapped foreground Carentan window measured
119.998 FPS, frame p95 10.075 ms/p99 11.510 ms and no frames over 33 ms. That is
one test window, not a guarantee. Profiles are in `out/realtime-carentan-*.json`.

The synthetic gateway benchmark sends command/fragment-sized 128/1300-byte
packets through 64 WebSockets. With the gateway alone inside Docker (2-core
quota, 96 MiB limit) and clients/echo outside, all 38,400 packets arrived without
loss/corruption; RTT p95 was 8.476 ms, p99 59.06 ms, and no CFS throttling was
recorded. Tail delays remain. `out/realtime-gateway-isolated-benchmark-2cpu.json`
records the result. An earlier in-container test also hosted all synthetic
clients, the echo receiver and a second gateway under the same quota and used
oversized 8 KiB datagrams; it overflowed UDP buffers. It is not a gameplay
benchmark. The final isolated test separates those roles and uses actual game
packet sizes. No 64-active-player gameplay certification is implied.

The final canonical WASM/package checks, native build, gateway suite and new
sanitized regressions passed. QA uses loopback 8089 with its own devmap rooms;
those mounts/cheats do not enter production. Physical smoke input consumed the
smoke grenade and the native server stayed alive; both maps loaded, and fire,
sprint and map transition were exercised. A prolonged console-held fire run
also logged the engine's existing 2046-scene-entity capacity warning. No effect
limits were reduced; crowded/effect-heavy scene capacity remains a follow-up
rather than a claim that every worst-case scene has been certified.

The tested build is deployed at `http://192.168.1.60:8088/` using the previously
authorized immediate restart. Served build ID:
`8ddc49ae834b25c2c4502a6a5481ca1133eced4bebeaac1df019cb641c30becc`.
Native SHA-256:
`0e55863ef5e66e6f72ce12cbd4028f6c7265d234fc32e39ad316c5759f7ea93c`.
Both match the tested candidates. All three production services run, the web
health check passes and the gateway health endpoint works over the LAN. CPU
limits are verified at 1/3/2 cores for web/native/gateway respectively. All
four private archive hashes and sizes match; only the expected read-only asset
mounts are installed. Startup had no rooms; a user then created a room and
connected. A 1594 ms engine frame was logged during initial map loading; it
does not establish steady-state gameplay latency. The isolated QA stack has
been removed. Evidence: `out/realtime-deployed.json`. Rollback image tags are
`local/cod2-wasm:before-realtime` and
`local/cod2-native-server:before-realtime`, with exact IDs/config saved in
`out/realtime-previous-images.json` and `out/realtime-previous-compose.yaml`.
No Cloudflare tunnel was started.

## 2026-10-09: remote movement animations and stance controls

Remote lateral movement previously selected a forward animation because the
shared animation conditions did not populate the retail script's strafe
condition. It now selects the original left/right walk, run and crawl clips.
Movement direction is sent as signed degrees, matching the snapshot and client
leg-yaw consumers, with offsets relative to the selected forward/backward/strafe
clip. Diagonal input rotates the legs with travel; releasing movement retains
the last direction. Crouched run/turn classes now participate in stance detection.

The script's `initialLerp = -1` default could reach the skeleton as a negative
blend duration when compared to a future stance deadline. Defaults now resolve
to positive 120/170/250 ms blends, with at least 200 ms during a leg stance
transition; explicit script durations remain respected. Posture events fire
once at the actual view-height target boundary. Stand/prone changes pass through
crouch and play the original `pb_crouch2prone` / `pb_prone2crouch` clips.

The private default configuration binds C to `togglecrouch`. Space retains
`+gostand`: one press from crouch or prone stands up; a later press while standing
jumps. A signed spectator-mode range check had also set the jump bit for normal
players, even when the stand handler only requested a posture change. The range
check now excludes normal players. Both private archives containing
`default_mp.cfg` were regenerated; all other renderer archive entries are
unchanged from the previous deployed archive.

`test-player-stance-animations.py` extracts the production C functions and
replays stance inputs across eight movement modes, 100 held-key frames, staged
posture changes, a blocked ceiling, eight movement directions, analog strafe
conditions and skeleton blend durations under AddressSanitizer/UBSan. Mutations
restoring each of the jump gate, missing transition events, wrong direction and
negative default blend are rejected. Canonical WASM/package checks, native
compilation and the private asset audit passed. The new test is part of
`test-static.sh`.

Two browser clients exercised Carentan in isolated loopback QA. Physical C
changed camera height from 60 to 40 and back. Holding Space for 650 ms from
crouch and from prone raised the camera to 60, retaining the same grounded
origin and zero vertical velocity. The observer's live animation graph recorded
`pb_combatrun_left_loop`, `pb_combatrun_right_loop` and `pb_prone2crouch` at
normalized time 0.55 during a rise. Screenshots and traces are saved in
`out/stance-remote-*.png`, `out/stance-remote-{left,right,rise}.log` and
`out/stance-physical-inputs.log`. Initial prone attempts during a teleport/fall
were rejected by the existing ground check; the grounded retry completed. This
QA checks animation/controls, not crowded-match performance or every animation
and weapon combination. Both temporary tabs and the isolated QA stack were
removed.

The tested version is deployed at `http://192.168.1.60:8088/` using the previously
authorized restart. Browser build ID:
`2b6404f7c6017e4550553fe83f8046a7167be8404be36e6156720cfb8e3d8661`.
Native SHA-256:
`8c9b64547179fbbd02a9674bce5d0165a088e28e3f3ffa40a3cc24e861ef3b4e`.
All three running image IDs match the tested candidates, the web health check
and LAN gateway health pass, all four private archive hashes/sizes match the
served manifest, and only production read-only mounts are present. Startup had
zero rooms and zero connected clients. Evidence: `out/stance-build-proof.json`
and `out/stance-deployed.json`. Exact prior image IDs, Compose configuration,
served manifest and the two replaced archives are saved under
`out/stance-rollback/`; image tags use `:before-stance`. No Cloudflare tunnel was
started. Existing browsers need a reload to receive the new client and controls.

## 2026-10-09: frag grenade fuse, bounce and throwback

Damageable entities previously sent grenades into the direct-impact explosion
branch. Grenades now honor their existing bounce flag on players and objects,
retaining the original trajectory, surface bounce coefficients and fuse deadline.
The three packaged frag definitions have a 3500 ms fuse but disable held cooking.
The requested cooking behavior is enabled in shared movement code without
changing those assets. The countdown begins at pin removal, including the throw
delay; holding a frag until expiry generates one real explosion and consumes one
inventory grenade. Smoke retains its existing non-cooking behavior.

Nearby live frags expose an English throwback prompt using the player's actual
`+frag` binding. Holding G picks one up; releasing G throws it back. Pickup checks
a 64-unit reach, visibility and a live player with a ready weapon. A private
64-slot server cache associates the held missile with the player's spawn and the
entity's use count. The same missile and absolute server deadline survive pickup,
holding, rethrow, death and disconnect. Returning a grenade also works with zero
inventory frags and never consumes the receiver's ammunition. The held missile
is hidden from snapshots because the original grenade viewmodel draws it. The
server checks expiry even when no new user command arrives. Playerstate reuses
the last unused bit of the existing 27-bit flags field; no protocol expansion or
new assets are required.

Live QA caught an additional explosion crash: the occluded splash-damage fallback
passed the null `g_phys_world` handle as trace extents and used an undersized raw
trace buffer. It now uses zero extents and a complete `trace_t`. The death path
also passed the fuse as the grenade weapon index; those arguments are corrected.
Explosion callers now pass inner damage and radius in the order used by
`G_RadiusDamage`, respecting the packaged damage and range.

`test-grenade-lifecycle.py` executes the production missile, bounce, explosion,
pickup, offhand state and armed death-drop code under ASan/UBSan. It covers
player/object impacts, blocked/expired/racing pickup, zero-ammo returns, repeated
commands, preserved deadlines, death/disconnect/respawn, expiry without input,
and 400 held-cook schedules. Five mutations restoring contact detonation,
replacement missiles, a refreshed fuse, reversed death arguments or reversed
splash arguments are rejected. `test-explosion-radius.py` exercises the actual
splash fallback, occlusion and attenuation, rejecting null-extents and undersized
buffer mutations. Both tests are in the canonical static suite. Canonical WASM
compilation/package checks, native compilation and 1452 frag/smoke offhand
schedules passed.

Two browsers tested Carentan in isolated loopback QA. A physical five-second G
hold killed the player with a real explosion at the location that had crashed;
the corrected server remained running. Killing a player while holding an armed
frag also completed without a crash. The exact final browser/native images then
completed pickup and release between opposing players: the receiver displayed
`Release G or Middle Mouse to throw back grenade`, entered the throwback flags
state and retained both inventory frags after the returned grenade exploded.
Evidence: `out/grenade-throwback-held.png`, `out/grenade-return-after.png`,
`out/grenade-cookoff-death.png`, `out/grenade-*-client.log` and
`out/grenade-qa-fixed-server.log`. The original crash report is preserved in
`out/grenade-qa-first-crash.log`. This covers the requested lifecycle in Carentan;
physics/visibility boundary cases are covered by the C regressions rather than
every live map and connection condition. Temporary browser tabs and the QA stack
were removed after verification.

The tested images are deployed at `http://192.168.1.60:8088/` using the previously
authorized restart. Browser build ID:
`ea2f29aa12f2d96471bd67926239564ed43584922e9f017afb5653ed14678a97`.
Native SHA-256:
`83793fb28c09f8025f411b3d86cd14dba48548f6198d97cab4e0f1ee303c07f6`.
Running image IDs match the tested candidates, web and LAN gateway health pass,
all four private archive hashes/sizes are unchanged, and production mounts
exclude QA files. CPU/memory limits are unchanged. Startup has zero rooms.
Evidence: `out/grenade-build-proof.json` and `out/grenade-deployed.json`.
Previous image IDs, Compose configuration and served build information are saved
under `out/grenade-rollback/`; rollback tags use `:before-grenade`. No Cloudflare
tunnel was started. Existing browsers must reload to receive the new client.


## Unbuffered semi-automatic trigger recovery — 2026-10-09

The trigger latch now records every attack press, including input blocked by
firing recovery, raising or reloading. Previously it was set only when a shot
started: release/re-press during recovery could queue another shot for the
first ready command. Semi-automatic weapons now require a fresh press once
ready. Recovery expires normally while the trigger is held, allowing bolt
cycling and automatic empty-magazine reloads to complete. The firing helper
also rejects a new shot while either recovery timer is active. An existing
delayed shot retains its firing delay. Both native authority and browser
prediction use this shared code and the existing replicated latch bit.

Original fire/reload/cycle times and all private asset hashes remain unchanged.
The sampled baseline rapid-click run already respected the Garand's 135 ms
minimum; the reproduced defect was buffered input, rather than a demonstrated
sub-135 ms interval. Before correction, a 25 ms press, 25 ms release and early
400 ms held re-press produced two shots. The same physical input with the
corrected client/server produced one shot (8 -> 7); a fresh press after ready
produced one more (7 -> 6). Thirty rapid press/release cycles produced eight
shots with intervals of 141–201 ms. An 850 ms Thompson hold produced ten shots
(20 -> 10), retaining automatic firing. Browser warning/error lists were empty.
Evidence: `out/cadence-qa.json` and the referenced screenshots.

The shared-code ASan/UBSan regression exercises all 20 packaged handheld guns
at command steps of 1–66 ms, hip/ADS, 31,680 rapid-click schedules, recovery,
raising, reload, held triggers, magazine exhaustion and mandatory bolt cycles.
It rejects buffered-input, missing timer-gate, missing semi-auto and extra READY
frame mutants. The complete canonical WASM/package/static checks and native
build passed. No assets were regenerated.

Deployed browser build: `9a67156c38a203620439f263cc0d7ed00ca19ca85cc856c0b615e1f6918d295d`.
Native SHA-256: `b782dc487990bae27da5a24097d93a1020c630bbe969ec751389c7e12e6ffd64`.
Runtime image IDs and deployed hashes/mounts/health are recorded in
`out/cadence-build-proof.json` and `out/cadence-deployed.json`. The live server
was restarted under the prior restart authorization, ending the two-player
match. It started with no rooms and the gateway ready at
`http://192.168.1.60:8088/`. Reload the browser before creating a new match.
The isolated 8089 QA containers and agent tabs were removed. Cloudflare remains
disabled. Recovery images use `local/cod2-wasm:before-cadence`,
`local/cod2-native-server:before-cadence` and
`local/cod2-gateway:before-cadence`; previous site/native files and image IDs
are saved in `out/cadence-rollback/`.

## Aim transition timing — 2026-10-09

The weapon loader now derives ADS pose rates from the original entry/exit
durations rather than firing/rechamber timers. Movement advances that pose
once per command step, before weapon firing logic; the second update inside
`PM_Weapon` previously doubled its speed. These changes apply to both browser
prediction and native authority. The packaged handheld weapons enter ADS in
220–300 ms and exit in 333–600 ms; the Garand uses 300/600 ms. Weapon definitions,
firing cadence and private asset archives are unchanged.

The native build and full canonical browser build/package/static checks passed.
ASan/UBSan tests execute the production loader and ADS interpolation for 20 guns
and 1,320 schedules, including reversals, variable steps, interruptions and
fallbacks. The actual movement dispatch regression checks 219 schedules for a
single ADS integration before firing; incorrect timer-source and duplicate-ADS
mutants fail. Timing details are saved in `out/ads-transition-audit.json`.
In the isolated Carentan match, physical controls raised/lowered the Garand
through intermediate poses and restored hip position. One aimed and one hip
attack produced two firing events, with ammunition 8 -> 7 -> 6 and no browser
warnings/errors. Screenshots demonstrate the poses; exact durations come from
the shared-code tests, not screenshot capture timestamps. Evidence:
`out/ads-qa.json`, `out/ads-client.log` and the referenced screenshots.

The tested images are deployed at `http://192.168.1.60:8088/`.
Browser build ID:
`34baebbd0cc7af415a2d906c856960227860ff339108a17b03325230a2b52828`.
Native SHA-256:
`9ba78c69ec5670aa68605593ecc03e1d58ee7a4d0748afe08d1b8bdfe38f7fbe`.
Running image IDs, native hash, LAN readiness, web health, unchanged asset
hashes and production-only mounts match `out/ads-build-proof.json` and
`out/ads-deployed.json`. CPU/memory limits are unchanged. The restart occurred
with zero rooms and startup remains empty. Reload existing browsers before
creating a match. The temporary QA stack and agent browser tab were removed;
Cloudflare remains disabled. Previous runtime images use `:before-ads` tags,
with site/native/source backups and image IDs in `out/ads-rollback/`.

## Mounted machine-gun recoil — 2026-10-09

Mounted guns now push authoritative aim upward after every actual shot, with
small random lateral movement. Cold pitch kick is 0.28–0.40 degrees per shot;
heat adds up to 0.12 degrees, and yaw varies within ±0.18 degrees. The existing
view-angle setter updates replicated command offsets, so the kick persists
across movement commands and players can compensate with mouse input. The
turret pose follows the same angles, clamped to its original aiming arcs.
The first shot uses the player's current aim; following shots use the kicked
direction. Released triggers, firing recovery and overheating add no recoil.
The 50/100 ms firing intervals and five-second heat/eight-second cooling rule
are unchanged. This uses existing player snapshots with no new message type,
runtime allocation or asset regeneration.

Native and canonical browser build/package/static checks passed. The new
ASan/UBSan regression executes the production firing gates, recoil and
view-angle setter across all 32 turret slots and eight base orientations
(256 schedules). It checks command persistence, next-shot aim, compensation,
both yaw directions, aiming limits, release, sustained fire and cooling, and
rejects missing-recoil, camera-only, pre-shot and idle-kick mutants. Existing
turret cooling/HUD checks are included in the canonical static suite.

In isolated Toujane gameplay, a physical one-second trigger hold produced 20
MG42 firing events. Without moving the mouse, pitch changed from 0.0 to -6.9
degrees and yaw from -29.3 to -29.2; both predicted and server view-position
diagnostics reported the same angles. Screenshots show the aim climbing from
the wall toward its roof. Browser warnings/errors were empty. Evidence:
`out/mounted-recoil-qa.json`, `out/mounted-recoil-burst-client.log`,
`out/mounted-recoil-before.png` and `out/mounted-recoil-after-burst.png`.
This is a functional recoil check, not a new device/FPS benchmark.

The exact tested images are deployed at `http://192.168.1.60:8088/`.
Browser build ID:
`39f4d64dcbb8cee6ffa142783ff6ed412e206466c20a03670efee3c2f3e6e8e3`.
Native SHA-256:
`4c7689ac39fcc51c3b7db45773e965eb522033208bc396c27d8a32c81d981a78`.
Build/deployment records are `out/mounted-recoil-build-proof.json` and
`out/mounted-recoil-deployed.json`. The restart occurred with no active rooms.
Production retains its CPU/memory limits, read-only private asset mounts and
empty startup. Existing browsers must reload. Temporary QA containers and the
agent tab were removed; Cloudflare remains disabled. Recovery images use
`:before-mounted-recoil` tags, with previous sources/site/native binary and
image IDs under `out/mounted-recoil-rollback/`.

## Remote player movement smoothing — 2026-10-09

Remote live players now interpolate position using the actual timestamps of
their evaluated movement samples. Their `TR_LINEAR_STOP` endpoints often end
at `commandTime + 50`, before the containing snapshot's timestamp. Previously,
blending those endpoints against snapshot times converted uneven command
arrivals into uneven apparent speed. Monotone Hermite interpolation preserves
ordinary endpoint velocity without overshooting between samples. When the
render clock passes the latest endpoint, prediction bridges at most another
50 ms, then stops. It adds no fixed playback delay. Teleports, death/mount
transitions, local prediction and unsupported trajectories retain their
existing paths; the server simulation remains at 20 Hz.

The ASan/UBSan regression executes the production trajectory evaluator and
entity interpolator. Across 15,078 rendered movement frames, using integer
positions and alternating 35/65 ms command samples at five frame intervals,
step-error RMS decreased from 0.6349 to 0.0947 units (85.1%). Another 89,082
samples cover monotone turns/stops; missing packets, bounded prediction,
reset/local/mounted/dead/angle and malformed-state cases also pass. Four
mutants restoring snapshot timestamps, unlimited extrapolation, local smoothing
or teleport smoothing fail. These are replay measurements, not an FPS gain
or a promise of zero network lag. Per-frame packet pose/animation and existing
clock regressions are included in the canonical suite. Native and canonical
browser build/package/static checks passed.

Two physical browser clients joined an isolated Carentan TDM room. The driver
walked, strafed, reversed, sprinted, crouched and jumped while the observer
rendered the remote player. Saved diagnostic windows contain 1,350 moving
frames, with two held frames in the window following the jump; all saved
walk/strafe/reversal windows before that report zero held frames. The captured
view reports 119 FPS at 1280x720 with original assets. This is a functional
two-client observation on this Mac, not a sustained load benchmark. Both
browsers had no recorded warnings/errors. Evidence:
`out/remote-motion-qa.json`, `out/remote-motion-observer-client.log`,
`out/remote-motion-driver-client.log` and `out/remote-motion-carentan.png`.
`?movementDebug=1` now also prints one `[remote-motion]` summary per second
per remote player; ordinary clients return immediately without that tracing.

The exact tested web image is deployed at `http://192.168.1.60:8088/`:
`sha256:d15d51d1fe0122cf23eaff457590a7daea9ef74d2b32b3735e38bd2b47b25dce`.
Browser build ID:
`dc9cfc7f988690015ee1cbae0f2f0d3ca5cba8cb4e8861802ff573129b62ff5c`.
Only the web container was recreated, so the native server and gateway kept
their active match (two players at verification). Their production native
SHA-256 remains
`4c7689ac39fcc51c3b7db45773e965eb522033208bc396c27d8a32c81d981a78`.
The native candidate also built and passed QA with SHA-256
`9279055f2e028f77516b41831591ad757579b7df8de3f3456542aede76f7f70d`;
it is not required for this client-only fix. Build and deployment records are
`out/remote-motion-build-proof.json` and `out/remote-motion-deployed.json`.
Private asset hashes, resource limits and production mounts were verified;
all visual assets remain unchanged. Existing browsers must reload. The two
agent tabs and temporary QA stack were removed; Cloudflare remains disabled.
Recovery images use `:before-remote-motion` tags; previous sources/site/native
binary and image IDs are under `out/remote-motion-rollback/`.

## Multiplayer packet processing and gateway startup — 2026-10-09

Network fields now consume the remaining byte fragment at a time instead of
branching for every bit. The original independent byte/bit cursors, signed
fields, overflow behavior and packet bytes are preserved. A 4 KiB Huffman
prefix table derives its entries from the existing seeded tree; a bounded
16-bit reservoir handles ordinary packet data, with the original decoder
retained for tails, padding and unused prefixes. Both the native server and
browser use this path. Simulation timing, command pacing, rendering settings,
textures, models, effects and audio are unchanged.

Host C microbenchmarks, across four runs per case, measured:

| Work | Original | Optimized | Speedup |
| --- | ---: | ---: | ---: |
| Decode snapshot-like Huffman data | 46.601 ms | 17.448 ms | 2.67x |
| Decode random Huffman data | 67.152 ms | 21.076 ms | 3.19x |
| Write 10,240,000 mixed-width fields | 403.821 ms | 34.393 ms | 11.74x |
| Read 10,240,000 mixed-width fields | 111.879 ms | 37.446 ms | 2.99x |

These measure codec work on this Mac, not an equivalent whole-game FPS or
server CPU gain. Reproduce with `scripts/test-huffman-cache.py --benchmark`
and `scripts/test-network-bit-fields.py --benchmark`. Evidence is in
`out/online-performance-codec-benchmark.log`, `out/huffman-performance.json`
and `out/online-performance-bit-fields.json`.

The gateway now keeps up to 16 initial binary packets, bounded to 64 KiB,
until UDP connect/DNS completes, then sends them in their original order.
Previously a WebSocket could open first and discard the initial challenge,
forcing the engine to retry joining. Each peer also reuses one UDP completion
callback. Eight gateway tests pass, including delayed readiness, ordered
delivery, packet/byte limits, disconnect before readiness and send failure.
The existing destination, ownership, origin and backpressure checks remain.

A ten-second synthetic UDP echo load with 64 WebSocket peers at 60 packets/s
delivered all 38,400 datagrams without loss or corruption. The candidate run
reported mean RTT 4.54 ms and p95 8.81 ms. This measures the local Docker
transport, not 64 complete game clients or internet latency. The earlier run
and candidate ran under different background load; their latency difference
does not establish a causal improvement. Evidence:
`out/online-performance-gateway-before.log` and
`out/online-performance-gateway-after.log`. Use
`BENCH_GATEWAY_IMAGE=local/cod2-gateway:online-performance-candidate node
downstream/gateway/benchmark.mjs --docker` to test this image again.

The canonical native and browser builds/package/static checks passed. Codec
regressions compare 4,096 full encode/decode cases and 10,240 prefix/truncation
cases against the original implementation. Bit field checks execute 574,222
packet schedules with ASan/UBSan, every bit offset and valid width, interleaved
byte writes, signed values, truncation, cursor/overflow behavior and buffer
canaries. Deliberately incorrect encoders, decoders and cursors are rejected.

Two physical browser clients connected to the exact candidate images in an
isolated Carentan TDM room, walked, reversed, strafed, fired, reloaded and
detonated frag/smoke grenades. In the saved 600-frame movement windows, both
reported 120 FPS; p95 frame times were 9.07 and 10.14 ms, with no frames over
33 ms. During the smoke observation they reported 117.1 and 120.0 FPS, p95
10.03 and 9.19 ms, also with no frames over 33 ms. Both retained 1280x720,
antialiasing and original assets, with no recorded browser warnings/errors.
These are short local windows, not a guarantee for all devices or matches.
Direct native probes returned all 300 queries per window with no CPU quota
throttling. Two-client server CPU was 31.8% of one core in the movement window
and 29.2% in the effects window; the earlier baseline was 27.8%. These noisy
whole-container samples do not demonstrate a server CPU reduction. Browser
and native measurements are recorded in `out/online-performance-browser-*.json`,
`out/online-performance-server-*.json` and `out/online-performance-qa.json`;
screenshots and rendered client logs use the same prefix.

Tested images use `:online-performance-candidate`; exact IDs and checksums
are recorded in `out/online-performance-build-proof.json`. Recovery images
use `:before-online-performance`, and prior sources, site, the exact running
production native binary and image IDs are saved under
`out/online-performance-rollback/`. Deployment verification is recorded in
`out/online-performance-deployed.json`.

All three tested images are deployed at `http://192.168.1.60:8088/`.
Browser build ID:
`a1fc0620662fc9e19daf53ba189c19951222b5e28bacfb8b256bbaa956eb15f3`.
Native SHA-256:
`72966096a4ff56c0e7b02cc61e293f4e247feae2fa33327443b4650f80597820`.
The full restart closed the active player's match. The room manager then
reported no rooms, retaining user-created-only startup. Production image IDs,
native checksum, gateway health, CPU/memory limits, private read-only mounts
and all four asset hashes were verified. Existing browsers must reload and
create a room. Temporary QA clients/containers were removed; Cloudflare
remains disabled.

## Compact multiplayer chat and input capture — 2026-10-10

Chat now defaults to five visible rows and eight seconds of history. The
10-unit font has measured line spacing, a single subtle background and a
360-unit width limit. Word wrapping measures the same font used for drawing,
preserves player/text colors and bounds long words. The eight-row storage ring
stays independent of the visible-height setting; expired messages are removed
together when rendering resumes. The native public/team input fields fit the
same compact area.

Native chat has a separate state that the browser shell treats as gameplay,
so T/Y no longer release existing mouse capture. Space and slash are forwarded
once when the framework suppresses their browser text events under capture.
Enter sends; Escape cancels without injecting a second Escape when the browser
also releases the pointer. The next trusted game key can request capture again,
with a one-second retry bound for browsers that reject the request.

The two-player reconnection test also exposed a server broadcast error:
G_Say iterated the first N entity slots instead of the connected-client list.
It now uses level.sortedClients while retaining team, spectator, dead-chat and
targeted-message rules. This requires the updated native server as well as the
browser client.

Canonical native/browser builds and package/static checks passed. ASan/UBSan
chat checks cover bursts, measured wrapping/colors, bounds, height changes,
expiration and native state/character routing. Delivery checks exercise 4,032
sparse/reconnected slot pairs, all 64 players and team/target/spectator filters.
Mutants with overlapping rows, unstable ring indexing, chat treated as a menu
or delivery to the first N slots fail. The input contract uses the actual pinned
framework key guard and adapter to check capture lifecycle and printable keys.
Logs: out/chat-web-final-build.log, out/chat-native-build.log,
out/chat-native-tests.log, out/chat-delivery-tests.log and
out/chat-focus-tests.log.

Two browser clients on the exact candidate images exchanged eight alternating
messages, team messages, spaces/slashes and a colored 130-character word. Both
displayed the newest five rows without overlap; old messages expired. Cancelling
the input allowed subsequent W/S movement without a canvas click. After one
client reloaded and its old slot timed out, the active clients occupied slots
1 and 2 and both still received chat. Screenshots use out/chat-*.png; rendered
client/server logs and out/chat-browser-qa.json retain the observations.

Pointer-lock limitation: the in-app Chromium browser rejected requestPointerLock
with UnknownError in both the prior build and this candidate. Actual locked
mouse behavior could not be observed here; the captured-input path passed the
native and framework/adapter contract tests. A QA-only setviewpos attempt also
disconnected the first room with a mounted-view tag error. The final chat and
reconnection checks used normal spawns in a fresh room; no mounted-view code was
changed in this update.

The exact tested web/native images are deployed at http://192.168.1.60:8088/;
the gateway image is unchanged. There were no active production rooms or players
at restart. The deployed build ID is
be5a57f415093ae5be6fd359617bf9ae57b688dffcb954e57eb9c672edb08a22;
native SHA-256 is
b43642c71f786f8c033c166d1f3791b45e3dfce913c1e4ed29ecb55311c23c41.
Image IDs, live LAN endpoints, resources, private read-only mounts, all four
unchanged asset hashes and empty-room startup are verified in
out/chat-deployed.json. Reload existing browsers to use the new client.
Recovery images use :before-chat; previous sources/site and image identities
are under out/chat-rollback/. Temporary QA clients/containers were removed.
Cloudflare remains disabled.

## Prompt ground movement release — 2026-10-10

Walking friction retained noticeable horizontal velocity after all movement
keys were released. At the default 190-unit speed, the production friction
routine took 296 ms and moved another 24.33 units in an 8 ms flat-ground
schedule. At the 275.5-unit sprint speed it took 368 ms and 39.19 units.
These are isolated physics measurements, not network-latency measurements.

PM_WalkMove now plants the player's feet on the first neutral movement command
on ordinary ground. The same code runs in browser prediction and the native
server, avoiding a client-only stop followed by a server correction. Active
movement still uses the existing acceleration/friction rules. Airborne motion,
new jumps, slippery surfaces, ladders and timed landing/knockback momentum
retain their existing behavior. Ground stopping clears the full velocity so
slope motion cannot reintroduce horizontal drift. Rendering, assets, input
packet pacing and server tick frequency are unchanged.

The new scripts/test-movement-release.py runs the production walk/air,
friction, acceleration, sprint-state and ground-plane routines with a simple
displacement fixture. ASan/UBSan checks cover 19,008 release schedules over
1–66 ms command steps, 24 directions, standing/crouch/prone/sprint and slopes,
plus held inputs, air/jump/slick/landing/push exclusions and client/server
parity. Deliberate coasting, held-input braking and forced-momentum cancellation
mutants fail. The check is part of the canonical browser build/static suite.
Sprint, mantle/jump, corner collision and stance/animation checks also passed.
Both canonical native and browser builds completed successfully; logs use
out/stop-movement-*.log.

Two physical browser clients joined Carentan and Toujane on the exact candidate
images and exercised movement releases, Shift+W and crouching. A temporary F10
binding read viewpos while gameplay remained active. Integer camera positions
were stable by the second 100 ms-spaced sample and after another 700 ms of
other-client input. Some initial walking/strafe samples met spawn-area walls;
moving sprint displacement and subsequent stops were observed. These samples
do not measure exact input-to-stop latency or sub-unit camera changes. Browser
observations, logs and screenshots are saved in
out/stop-movement-browser-qa.json and out/stop-movement-carentan/toujane-*.
The integrated browser's existing pointer-lock limitation remains; keyboard
movement and the actual shared physics were verified.

The exact tested web/native images are deployed at http://192.168.1.60:8088/.
The gateway image is unchanged. Production had no active rooms or players at
restart and still starts with no rooms. Build ID:
701bfd8b86a52becf081eca07cb3dadd809bab3bea26bcc438883951e48d7ad4.
Native SHA-256:
26a76334d305aeaed2dc28058ab9db2a45d0f3415a1119f6d3c2aff06506267f.
out/stop-movement-deployed.json verifies image IDs, native checksum, live LAN
build/health, unchanged resources and all four asset hashes, private read-only
mounts and Cloudflare disabled. Existing browsers must reload to match the
updated server physics. Recovery images use :before-stop-movement; previous
sources, browser package and image identities are saved under
out/stop-movement-rollback/. Temporary QA clients and containers were removed.


## Persistent multiplayer bodies — 2026-10-10

Corpse movement tested the interaction flag `active`, which is zero for cloned
players, and returned before resolving ground contact. It now completes the
normal gravity/root-motion collision path. Ground alignment also uses a full
three-component angle buffer, rather than overwriting the two-component
animation rotation buffer.

The animation-end callback retains the final death pose and schedules entity
removal eight seconds after death. Respawn scripts and original artwork are
unchanged. The original eight clone slots bound the body count; rapid deaths
can recycle an older slot. Corpse movement and animation precede one think
dispatch per frame, and death-volume removal returns before touching a freed
animation tree.

`test-corpse-lifetime.py` executes production movement, animation-end and think
dispatch across 240 lifecycles with 20 Hz frames, verifies ground/root motion
and eight-second cleanup, and rejects five broken variants (missing cleanup,
interaction-flag gating, freed-body access, duplicate think dispatch and the
undersized angle buffer). ASan/UBSan pass. Static checks also include the
existing 72-tree initialization/restart and 512 client corpse-copy regressions.

Two background in-app browser clients fired real lethal bursts on Toujane and
Carentan using the final candidate images. Both maps retained the body after
five seconds while the owner was alive at a new spawn. The Carentan body was
gone at the 9.7-second observation, with the dropped weapon still present.
QA-only console positioning established the firing lines; damage, death,
body physics and respawn used the normal multiplayer code. Killcam was disabled
only in QA for the explicit respawn check. There were no browser warnings.
Evidence: `out/corpse-lifetime-qa.json`, map timing/screenshot files, client logs
and `out/corpse-lifetime-server.log`. Native and canonical browser builds,
including the complete static suite, passed.

The exact tested images were promoted to production (`:dev`), with gateway
unchanged. Production had no rooms or players at restart. LAN health, build ID,
native SHA, image IDs, resource limits and private read-only mounts are verified
in `out/corpse-lifetime-deployed.json`; all four private archives retain their
previous hashes. Rollback sources/binaries/images are in
`out/corpse-lifetime-rollback/`. Both agent-created tabs were closed and the
temporary QA project removed. Production remains user-created rooms only.

## 2026-10-10 — MG42 collision crash, ADS after reload and dropped-item expiry

A two-browser Toujane reproduction placed a second player against the rooftop
MG42 operator while firing. Both clients were disconnected by
`G_GetPlayerViewOrigin: couldn't find tag`; the exact failure is preserved in
`out/mg42-rooftop-crash-before-server.log`, the corresponding client log and PNG.
`StuckInClient` wrote its 300 ms collision-ejection bit into `ps.eFlags`, where
0x200 means a mounted turret. The ordinary player's view code therefore tried
to find `tag_player` on ENTITYNUM_NONE and shut down the room. Collision pushes
now set `ps.pm_flags` so the existing movement timer clears them. The collision
scan only visits client slots and checks live/connected clients before accessing
their state, avoiding non-player dereferences and scanning map entities.

Reload entry no longer emits RESET_ADS, which canceled toggled aiming on the
client. Existing ADS interpolation still lowers the weapon during the reload;
held or toggled aiming returns as the reload permits, and canceling aim during
reload leaves the weapon at hip level. Dropped weapons and ammunition keep their
one-second owner grace and pickup handler, then expire at an absolute twenty
seconds from creation. Slot reuse writes a new deadline; cleanup uses the
existing entity-free path and removes the dropped-item queue reference.

Three production-code ASan/UBSan regressions cover 1024 mounted/ordinary collision
cases including camera origin and push expiry, 2376 held/toggled/canceled reload
schedules, and 400 weapon/ammo expiry/reuse schedules with delayed frames.
Mutations restoring the wrong turret flag, null-client scan, RESET_ADS or missing
expiry are rejected. All three are part of the canonical static suite.
A diagnostic full-server build also caught a startup compatibility declaration
writing four bytes into the one-byte `sInWindowMode`; its external declaration
now matches the actual Boolean storage. This was separate from the reproduced
MG42 crash. The sanitizer server was used only in isolated QA; its packed script
bytecode reports expected unaligned reads, and global checks were disabled after
the compatibility error was identified. Normal optimized builds are deployed.

The optimized candidate repeated the exact fatal rooftop overlap with two real
browser clients. Both remained connected, and the mounted MG continued through
163 seconds of firing/cooling cycles before the test stopped it. Automatic
empty-magazine and manual MP40 reloads returned to the sights; canceling aim
during reload left the weapon at hip level. A normal lethal burst created
native `weapon_mp40_mp` and `weapon_frag_grenade_german_mp` item entities; the
entity-list snapshots show both before expiry and neither afterwards. The
production-code regression verifies the exact twenty-second boundary. Evidence
is in `out/mg42-crash-qa.json`, client/server logs, reload screenshots and
`out/dropped-weapon-entity-before/after.log`. The local QA run also recorded
native frame hitches and one transient Connection Interrupted display that
recovered without a room shutdown; this is gameplay regression validation,
not a zero-lag performance guarantee. No browser warnings were recorded.

The canonical browser build/full static suite and native build passed. The
exact tested candidate images were promoted to `:dev`; gateway, resource limits
and private read-only mounts are unchanged. Production had two empty rooms and
zero connected players at restart, then started with zero rooms. LAN build ID:
`48df04a08e98773f24d34efe7a5b0b39a1dc45ef5611495dbe4107ea58975874`.
Native SHA-256:
`a9a72cc6ec98e6578b3c86c8ea078399117ae8cf8901d0bde62c7916271db836`.
`out/mg42-crash-deployed.json` verifies live health, image identities, checksum,
unchanged private archives and QA removal. Both agent-created browser tabs and
the temporary QA project were closed. Recovery images use `:before-mg42-crash`,
with previous sources/browser package/native binary in
`out/mg42-crash-rollback/`. Reload existing browsers at
http://192.168.1.60:8088/ to receive the matching client.

## 2026-10-10 — Creation-menu focus and server-name editing

The live browser reproduced selecting Toujane and then being unable to click
the Game Type value or edit Server Name. The sibling-menu outside-click handler
restricted labeled controls to the label's painted text bounds. It now applies
text bounds only to text buttons; edit fields, ownerdraw choices and list boxes
use their full item rectangles. This preserves button bounds while allowing
clicks in the name and game-type value areas after selecting a map.

Leaving a text field previously consumed the mouse-down event. It now commits
the field and routes the same click to its target, so choosing a map/mode or
starting the room works with one click and preserves the typed name. Field
painting identifies the active edit item directly, draws its cursor and scrolls
only during editing. Blurred fields show the beginning of their value.

`test-ui-create-focus.py` executes the production menu dispatch, outside-click,
list-selection, text-editing and field-paint functions for sixty map/name/mode/
start sequences at three scales. It checks repeated changes, name persistence,
one-click transitions without Enter, long-name scrolling/blur, cursor ownership
and text-button bounds. ASan/UBSan pass, and mutations restoring label-only hits,
consumed clicks, stale scrolling or the missing cursor are rejected. The new
regression and the existing sibling-menu click regression run in the canonical
static suite. The game-mode and browser creation-argument regressions also
cover all five modes and both maps.

The final candidate passed the complete `build-web.sh` static/package suite.
In the normal isolated QA stack, the browser selected Toujane first, changed
Game Type to Capture the Flag, edited the name to `Toujane CTF QA` and clicked
Start while editing without pressing Enter. The room loaded the CTF information
screen and `/servers` confirmed the exact hostname, `mp_toujane` and `ctf`.
The same browser then selected Carentan, edited `Carentan HQ QA`, changed modes
directly from the editing field with one click, and created the Headquarters
room. Its information screen loaded and `/servers` confirmed the exact name,
`mp_carentan` and `hq`. Long-name editing/blur and the visible cursor were also
checked. Browser warning/error logs were empty. Evidence is in
`out/create-menu-focus-final-rooms.json`,
`out/create-menu-focus-final-client.log` and the
`out/create-menu-focus-*-final.png` screenshots.

Only the tested web image was promoted and recreated in the production stack:
`sha256:42174787b062b00566e0454e0240bd1c6881d7500d538b9749bfe883d6c4b8f0`.
The live LAN `/version` returns build ID
`02a154efb49fb41634e72b00e97302aaad8feddfbf71ccccb672c9e0830e9d2a`.
Native and gateway container IDs/images were preserved, with the previous
native checksum, private archive hashes, mounts and resource limits unchanged.
Live gateway health is ready; the production stack has no rooms or clients.
The temporary QA rooms/stack and browser tab were removed, and the temporary
browser viewport override was reset. `out/create-menu-focus-deployed.json`
records these assertions. Recovery uses
`local/cod2-wasm:before-create-menu-focus` and
`out/create-menu-focus-rollback/`. Reload http://192.168.1.60:8088/ to receive
the updated menu.

## 2026-10-10 — Explicit fullscreen control

Removed the adapter's automatic fullscreen request on primary canvas clicks.
An always-visible top-right button now switches between `Enter fullscreen` and
`Exit fullscreen`. F10 toggles the same action during loading, menus or a match,
including when pointer capture makes the mouse unavailable. Fullscreen changes
follow the browser's actual state through `fullscreenchange`; repeat keys and
overlapping requests cannot trigger duplicate transitions. Rejected requests
remain retryable with an English status message. Toolbar mouse/keyboard input
is isolated from SDL, and using the button restores keyboard focus to the game
or the previously focused name field. The control updates only on events.

`test-fullscreen-control.cjs` covers entry/exit, F10 with a captured pointer,
external exits, repeated/pending requests, synchronous and asynchronous failures,
focus restoration and toolbar input isolation. It is included in the canonical
static script. The focused fullscreen/input-capture, LAN asset-validation,
engine build-URL and package-contract checks passed; results are in
`out/fullscreen-control-tests.log`.

The normal isolated QA stack verified ordinary canvas/menu clicks remaining
windowed, button entry/exit and F10 entry/exit. A real Carentan TDM room was
created through the menu, and an American player spawned with a Garand. The
control remained visible through the information, team and weapon screens and
the live match; F10 entered fullscreen and the button exited it with the
magazine still at eight rounds. The final candidate was reloaded and its button
entry/F10 exit checked again. Browser warning/error logs were empty. Screenshots
and client/room evidence use the `out/fullscreen-control-*` prefix.

Only the adapter, startup CSS and build metadata changed in the packaged site.
The WASM checksum remains
`4d4acc4b57729834f3e79ea0f8f334fbfa8e5777edfb9258e647c022b3c240e2`.
The tested web image was promoted:
`sha256:4badb86a44a0c2d712b131ebe69d146552adb3531670ba229c9a114ade4016a4`.
The live LAN build ID is
`31817bfb790bdab9a434b0b4e47d4975e0593954405695dceb775b37e91fb94a`.
Native and gateway containers/images, native checksum, private assets, mounts
and resource limits were preserved. Live adapter/CSS bytes match the source and
gateway health is ready. The temporary browser and QA room/stack were removed.
`out/fullscreen-control-deployed.json` records these assertions. Recovery uses
`local/cod2-wasm:before-fullscreen-control` and
`out/fullscreen-control-rollback/`. Reload http://192.168.1.60:8088/ for the button.

## 2026-10-10 — Finite scoped breath hold

`PM_DropTimers` decremented `holdBreathTimer` before `PM_UpdateHoldBreath`
incremented it in each movement step. These changes canceled each other while
Shift was held, so the original exhaustion threshold was never reached. Breath
usage and recovery now have one owner, `PM_UpdateHoldBreath`, shared by browser
prediction and the native server. The original dvars remain 4.5 seconds of
holding and 5.5 seconds of exhausted recovery (hold time plus the 1-second gasp
penalty). Early release recovers the spent time; lowering the scope or changing
weapons retains that recovery. Scope sway and the existing breath sounds follow
the hold flag as before. No new network fields or per-frame work were added.

`scripts/test-hold-breath.py` executes the production movement timer, breath
update and smoothing functions under ASan/UBSan. It covers 66 command lengths,
20,000 variable-duration commands, continuous input across repeated exhaustion,
early release/re-press, scope/weapon interruptions, ineligible weapons, disabled
holding, sway recovery and identical client/server replay. Restoring the second
timer decrement, removing exhaustion or removing recovery makes the test fail.
The canonical static suite includes it; focused weapon-timer, sprint and ADS
checks also pass. Logs use the `out/hold-breath-*` prefix.

The candidate browser and native binaries ran in an isolated normal Carentan
TDM room named `Breath QA`. A Springfield was selected through the game menus.
A 16-second right-mouse/Shift hold crossed two exhausted breath cycles. A
temporary read-only sidecar sampled the native player's state 1,854 times over
40 seconds without changing the room process, config or state. Observed hold
durations were 4,493 and 4,506 ms, with 5,503 ms of recovery between them; ADS
remained fully engaged and sway increased during recovery. The browser had no
console warnings/errors. Evidence: `out/hold-breath-gameplay-proof.json`,
`out/hold-breath-native-samples.json` and `out/hold-breath-gameplay.png`.

After the full browser build/static suite passed, the matching client and native
server images were promoted with no live rooms or clients. The gateway kept its
container and image. Resource limits, read-only private asset mounts and archive
hashes match the previous deployment; the temporary QA stack and probe were
removed. `out/hold-breath-deployed.json` records the live build/binary identities
and assertions. Recovery uses the `before-hold-breath` web/server image tags and
`out/hold-breath-rollback/`. Reload http://192.168.1.60:8088/ for the corrected
client prediction.

## 2026-10-10 — Nearby grenade warning

The client now draws the existing original grenade icon and direction triangle
for live frag projectiles in the player's blast sphere. The direction follows
camera yaw, the arrow pulses, and scoped aiming keeps the warning visible. The
default range and height caps cover the retail frag radius of 256 units. Smoke,
exploded/hidden, removed and future projectiles do not warn. The warning is a
proximity cue; walls can still block actual splash damage. Friendly and own
frags also warn because their explosions can be dangerous.

Drawing scans only the bounded snapshot entity list and keeps the four closest
threats. It adds no server work, protocol fields, allocations or world traces,
and draws at most eight HUD quads using already-loaded private materials.

`scripts/test-grenade-indicators.py` runs the production selection/drawing
functions and native splash function under ASan/UBSan. Its 1,836 retail cases
cover 3D radius boundaries and rotating directions for all three frag weapons.
It also covers scope/HUD/death gates, lifecycle, smoke exclusion, pulse alpha,
nearest-four ordering and malformed snapshot bounds. Five regressions are
rejected: horizontal-only distance, persistent exploded warnings, smoke
warnings, reversed arrows and an unbounded warning count. Focused grenade
lifecycle, splash and gameplay-feedback checks and the full browser build/static
suite passed. Logs use the `out/grenade-warning-*` prefix.

The candidate ran in an isolated normal Carentan TDM room, `Grenade Warning QA`,
with two browser clients on opposite teams. Both clients displayed warnings for
their nearby thrown frags; one warning was visible through a scoped Kar98k.
Backing away from a close frag cleared the icon during the fuse. Neither client
reported browser warnings/errors. Screenshots and the scope/range observations
are recorded in `out/grenade-warning-gameplay-proof.json`. These checks used
normal player input and the unchanged production native server; no cheats or
test server config were installed.

Only the browser image was promoted. One production player and their Carentan
room remained connected, with the native and gateway containers/images
unchanged. The live build ID and served WASM hash match the verified candidate;
private archive hashes, resource limits and read-only mounts match the previous
deployment. The QA stack and temporary clients were removed.
`out/grenade-warning-deployed.json` records these assertions. Recovery uses
`local/cod2-wasm:before-grenade-warning` and `out/grenade-warning-rollback/`.
Reload http://192.168.1.60:8088/ to load the grenade HUD update.

## 2026-10-10 — Larger frag explosion visuals

Grenade explosion particles are 35% larger, including fire, smoke, dust and
nonuniform sprite/cloud dimensions. Impact registration creates one visual
variant per unique grenade surface effect and reuses it across surfaces.
Original curves and media are shared read-only; counts, lifetimes, physics,
lights, decals, camera shake and all non-grenade impact entries are retained.
Variants use map-lifetime hunk storage and are discarded with the map. No new
particles, textures, scheduling work, server behavior or network fields were
added. Larger sprites cover more pixels; particle/simulation counts are unchanged.

The full browser build/static suite and the existing FX curve, impact, drawing
and line-allocation checks passed. Live verification used normal input in an
isolated Carentan TDM room and then switched the same client to Toujane. Multiple
frags detonated without browser warnings/errors, and the map switch reloaded the
variants successfully. Visible FPS stayed around 112–120 during the recorded
Carentan sequences and around 120 in Toujane on this machine; these are local QA
observations, not a performance guarantee. The close frag caused normal splash
damage, while the other blasts could be watched safely from farther away.
Evidence is in `out/grenade-size-gameplay-proof.json` and the `out/grenade-size-*`
screenshots/build logs. `out/grenade-size-gameplay.png` shows the Toujane flash.

The browser-only deployment preserved the active production room, native server
and gateway containers/images, resource limits, environment and read-only asset
mounts. Served build metadata and WASM match the tested candidate, and private
archive hashes remain identical. QA containers, networks and the test tab were
removed. `out/grenade-size-deployed.json` records verification. Recovery uses
`local/cod2-wasm:before-grenade-size` and `out/grenade-size-rollback/`.
Reload http://192.168.1.60:8088/ to load the larger explosion visuals.

### Muzzle flash lighting and reload review — October 10, 2026

The browser's fixed-function DX7 path skipped the original FX point lights.
`web_point_lights.c` now packs up to four authored lights after FX submission
and combines them in the existing opaque world/model shader. World normals
come from eye-position derivatives; lit models retain their smooth vertex
normals. Positions use the active view matrix, radii fade with the authored FX,
and the shader applies illumination before fog without changing alpha. Shader
uniforms are cached per program and shooting creates no additional geometry
passes. The affected fixed-function programs use GLSL ES 3.00 on WebGL2.

Short-lived muzzle sprites are 40% larger, with RGB scale ×1.35 and alpha scale
×1.5. The registration-time variants share original curves and media and keep
particle counts, durations and smoke unchanged. Hidden first-person weapons
now retain a light-only variant, so scoped shots illuminate the environment
without adding muzzle smoke or sprites across the scope. Native weapon rules,
server binaries and private archives are unchanged.

The user explicitly chose to retain the original Garand reload rule: its
eight-round clip must be empty. The Lee-Enfield needs room for a five-round
charger. The actual shared state machine passes R reloads for every clip count
and reserve availability across all 20 handheld guns. Live QA confirmed
Garand 7|96 stays unchanged on R, then 8|96 -> eight shots -> 8|88; Thompson
20|180 -> 1|180 -> R -> 20|161; Scoped Kar98k 5|60 -> 4|60 -> R -> 5|59,
then a scoped shot and R -> 5|58 with the scope restored.

Both maps were tested through normal Create/Join Game, team and weapon menus.
The Carentan Thompson burst visibly pulses light over walls, floor and hands;
Toujane Kar98k hip/scoped fire shows the corresponding brief lighting and
expiry. The final clients reported no warnings or errors. Observed performance
was approximately 119–120 FPS at 1280×720 on this Mac; this does not establish
performance on other hardware or a full player load. Evidence:
`out/muzzle-light-gameplay-proof.json`, `out/muzzle-light-gameplay.png`,
`out/muzzle-light-scope-01.png`.

The complete browser/static build passed in `out/muzzle-light-final-build.log`.
The subsequent scope fix was rebuilt and its focused checks passed in
`out/muzzle-light-scope-build.log`. The final smooth-normal shader passes all
48 texture/normal combinations in `out/muzzle-light-shader-tests.log`.
ASan/UBSan cover light packing, 60 complete FX variants, hidden model/scoped
selection, and unchanged curve/media/count/lifetime data. Existing muzzle
transform and reload ADS tests also pass.

Deployment replaces only the web container, with rollback image
`local/cod2-wasm:before-muzzle-light`. The final hashes, resource limits,
mounts, native/gateway preservation and QA cleanup are recorded in
`out/muzzle-light-deployed.json`.

## Raspberry Pi runtime and documentation — October 10, 2026

- Reorganized the English README around supported maps/modes, intentional additions,
  restored original behavior, controls, hosting, private assets and measured
  performance. Sprint, MG42 heat/recoil, LAN weapon balance, larger FX and browser
  conveniences are documented separately from retail modes/features. Updated
  the English update guide and added `docs/RASPBERRY_PI.md`.
- Added `compose.pi.yaml`, an ARM64 supervisor image with only the i386 engine
  executed under Debian QEMU 10, and reproducible prepare/deploy helpers. Web,
  gateway and Python remain native ARM64. Private archives remain read-only
  external mounts; no privileged runtime or global binfmt installation is needed.
- Host tested: Raspberry Pi 5 Model B, 8 GB RAM, Debian 13.4, kernel
  `6.12.75+rpt-rpi-v8`, 4096-byte pages. The original 16 KiB-page kernel could not
  load the packaged i386 libraries. Selected the already installed official
  `kernel8.img`, preserved a boot-config backup on the host, rebooted, and verified
  the new page size and unrelated existing service restart. The deployment helper
  checks page size rather than editing boot settings. Memory cgroups are disabled
  on this host; Docker CPU quotas work, but requested memory limits are ignored.
- Fixed Linux file enumeration in `mac_common.c` and `MacWin32.c` to use the
  64-bit file ABI while keeping engine pointers 32-bit. Narrow `readdir`/`stat`
  conversion can stop on wide directory cookies/inodes before discovering IWDs.
  The actual i386 listing code passes wide-entry, extension/filter and empty
  directory cases; its former narrow ABI fails the same regression. The native
  server build runs this check. Also included the room supervisor in the standard
  native Docker build context, which previously omitted its source.
- Full existing static/browser regression suite, eight gateway tests, all 4,369
  private-entry checks and weapon-balance audit passed. Native Release build and
  i386 file-listing regression passed.
- Isolated Pi engine checks created and removed one TDM room on each original map:
  Toujane startup 3.687 s, 300 getinfo queries over 15 s, zero lost, mean 0.280 ms,
  p95 0.250 ms, max 23.614 ms, 1.65% of one CPU core; Carentan startup 4.227 s,
  300 queries over 15 s, zero lost, mean 0.320 ms, p95 0.292 ms, max 28.453 ms,
  1.90% of one core. Neither sample was CPU-throttled. These are empty-room
  response checks, not a populated-match load test or an end-to-end latency claim.
  The supervisor started and finished with zero rooms.
