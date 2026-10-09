# Call of Duty 2 browser runbook

Status: **Playable Toujane/TDM browser port under active development. Chrome and
the internal browser have verified movement, aiming, damage, death, respawn and
synchronized scores, including the normal room. This is two-client coverage,
not a 64-player load or long-duration stability result.**

## LAN access during testing

The current host address is `http://192.168.1.10:8088/` (DHCP may change it;
check `ipconfig getifaddr en0`). Clients on the same LAN open this address,
then choose **Join Game → Toujane | TDM**. There is no web account or login.
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
WebSocket/UDP gateway. The only intended match is Toujane, Tunisia
(`mp_toujane`), Team Deathmatch, with up to 64 human players per LAN server.
The minimum gameplay acceptance check still uses two independent browsers.
Singleplayer, bots, other maps, and other modes are excluded. The previous
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
Only Toujane/TDM with up to 64 players per room is allowed. The default process stays available;
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
without that overlay, and the player spawned with a Sten. The browser creation screen exposes only the name, TDM,
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
