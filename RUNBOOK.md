# Call of Duty 2 downstream WASM runbook

## Scope and hard boundaries

This checkout is a **local-only feasibility lane** for a new browser integration
around OpenCoD2's reconstructed native core. It is not a playable browser port,
not a redistribution of Call of Duty 2, and not approved for publication.

The following rules are mandatory:

- Do not inspect, restore, copy, build, or derive from OpenCoD2's pre-existing
  `src/web` implementation or its generated `build/web_gen` support code.
- Do not invoke OpenCoD2's upstream Emscripten target. The only allowed web
  configuration is `downstream/wasm/CMakeLists.txt`.
- Use `wolfet-wasm` only as a product and architecture reference. Implement all
  Call of Duty 2 browser platform code in this repository.
- Never commit, package, upload, or publicly serve owner-supplied IWD files.
- Do not create a public repository, binary release, container image, or hosted
  runtime until the reconstructed source's license and provenance have received
  an explicit review and approval.
- Do not submit patches, issues, pull requests, or messages upstream.

The inherited web directories were removed from this branch by path without
opening their contents. The repository's root CMake entry point now fails closed
when configured by Emscripten, directing developers to `scripts/build-web.sh`.
The active downstream build names its native source files explicitly and never
configures the repository's root build system.

## Source and publication gate

Baseline commit `f70e697476fceeb4f53de677e1c5d5fe12a00b36` describes the
project as an early, incomplete source-level reconstruction. Its README says it
may crash or omit subsystems, should not be treated as hardened, and expects a
32-bit x86 multilib environment for the native Linux build. It provides a
notice but no explicit repository license grant.

The tree also contains reconstructed/generated blob and assembly material under
paths such as `src/blobs`. Its provenance, copyright status, pointer/layout
assumptions, and portability must be reviewed before it can become an input to
a distributable browser build. Until that gate passes, all work and artifacts
remain local.

The remote is deliberately fetch-only:

```text
upstream fetch: https://github.com/opencod2/opencod2.git
upstream push:  DISABLED
```

All downstream work is on local branch `devel`. Do not add a pushable downstream
remote until the publication gate is explicitly cleared.

## Honest implementation status (2026-08-14)

The first new Emscripten target builds and executes one real reconstructed
native module: `src/PC/qcommon/md4.c`. The MD4/checksum implementation is
compiled directly into WebAssembly and validated against deterministic block
and keyed checksums.

This is a **native-core compile probe**, below portfolio Milestone 1. It proves:

- the local Emscripten/CMake toolchain works;
- an allowed native translation unit can cross-compile without inherited web
  machinery;
- the generated JavaScript and WebAssembly load and execute under Node;
- the generated site can be delivered byte-for-byte by a local HTTP server.

It does **not** prove engine startup, asset loading, rendering, input, audio, a
cooperative main loop, single-player, multiplayer, or playability. The browser
page says this explicitly and has no fake Play control.

### Why the header seam exists

The first direct `emcc` compile of native `md4.c` failed before producing an
object because the reconstructed common header graph transitively requested the
now-removed inherited `web/webgl2_compat.h`. Restoring that file would violate
the clean-port rule.

`COD2_DOWNSTREAM_WASM_CORE_PROBE` therefore selects
`downstream/wasm/cod2_wasm_core_types.h`, which declares only the fixed-width
types and `MD4_CTX` layout that this module needs. Native builds retain their
original includes. This narrow seam is deliberately not presented as an engine
platform layer.

## Reproducible build and test

Known working tool versions:

```text
Emscripten 6.0.6
CMake 4.3.4
Node 24.x
```

Prerequisites are CMake, Node, and an activated Emscripten SDK. If `emcmake` is
not already on `PATH`, set `EMSDK` to the emsdk checkout; the script then sources
`$EMSDK/emsdk_env.sh`.

From the repository root:

```bash
./scripts/build-web.sh
./scripts/test-web.sh
```

The build output is ignored and appears under:

```text
out/cod2-wasm-core/site/index.html
out/cod2-wasm-core/site/cod2_core_probe.js
out/cod2-wasm-core/site/cod2_core_probe.wasm
```

The expected successful test output is:

```text
[cod2-wasm] downstream Emscripten core probe started
[cod2-wasm] native MD4 block checksum: 9028dc2c
[cod2-wasm] native keyed checksum: 4cdcd263
[cod2-wasm] probe complete; this is not a playable game build
```

The executable returns nonzero if either checksum changes.

A direct native-default compile was also probed to ensure the conditional include
did not conceal a host regression. A normal 64-bit compile reaches the baseline's
32-bit field/size assertions and rejects the incompatible layouts. A `-m32`
compile stops earlier because this workstation does not have the 32-bit libc
development headers (`bits/libc-header-start.h`). This matches the baseline's
documented multilib prerequisite; a full native regression build was therefore
not run and must not be reported as passing.

## Coordinator-only Chromium smoke handoff

Workers must not control Chrome. The coordinator may smoke-test only this probe,
serially, after stopping any previous test service:

```bash
cd out/cod2-wasm-core/site
python3 -m http.server 8014 --bind 127.0.0.1
```

Open `http://127.0.0.1:8014/` in a fresh Chromium tab. Expected result: the page
is visibly labeled “native-core WASM probe,” says it is not a game launcher, and
shows the four deterministic log lines above. Verify the console has the same
lines and no WASM load error. This test needs no retail assets and makes no game
runtime claim. Stop the HTTP server before testing another project.

Chromium has not been run for this checkpoint, by design.

## Owner Steam assets

The complete local Steam installation was detected at:

```text
/home/ted/.steam/debian-installation/steamapps/common/Call of Duty 2
```

Steam manifest `appmanifest_2630.acf` reports app ID 2630, `StateFlags=4`, and
equal downloaded/required byte counts. Its `main` directory contains 28 IWD
archives totaling 3,685,129,248 bytes (16 `iw_*.iwd` and 12 localized English
archives).

The inspection was metadata-only. No IWD was opened, copied, mounted, tracked,
embedded, or served. `git ls-files` contains no IWD, fastfile, map, executable,
shared library, PK3, PAK, or WASM retail artifact.

A future approved runtime should obtain assets through an explicit local file or
directory picker and retain them in origin-private browser storage such as
OPFS/IndexedDB. Do not add a public asset directory or unauthenticated upload
endpoint. Do not copy the Steam installation into this checkout.

## Single-player and multiplayer status

Report these independently:

| Mode | Current status | Evidence / blocker |
| --- | --- | --- |
| Single-player | Not built; not started; not playable | The baseline exposes some shared `game`, `cgame`, and `ui` code, but its documented native targets and the complete client/server directories are multiplayer-oriented. No separate complete SP target has been proven. |
| Multiplayer | Not built; not started; not playable | Native `_mp` client, server, game, cgame, and UI trees exist, but none are part of the downstream WASM target yet. Networking and security have not been adapted or audited. |

Do not infer single-player support merely from shared directory names. Do not
infer multiplayer support merely because reconstructed source files exist.

## Next engineering gates

Work in this order and stop claims at the last verified result:

1. Design a downstream platform/type boundary that can compile native qcommon
   modules without `imports.h` pulling in removed web/platform compatibility
   code. Keep every native default unchanged behind explicit downstream macros.
2. Audit all fixed-address, 32-bit pointer, x86 assembly, generated blob, and
   ABI assumptions before selecting additional translation units. Do not paper
   over layout mismatches with unsafe casts or broad linker stubs.
3. Compile a meaningful qcommon subset and supply only named, behaviorally
   documented platform functions. A link full of arbitrary no-op symbols is not
   a milestone.
4. Create a cooperative Emscripten main loop from the native `Com_Init` /
   `Com_Frame` boundary. The current Unix entry point funnels through a native
   `WinMain` path and has not been adapted.
5. Implement owner-directed filesystem selection and read-only IWD access
   without copying assets into the web root.
6. Port the renderer deliberately to WebGL 2. The reconstructed desktop renderer
   and SDL/GL path have not been compiled for WASM.
7. Add keyboard/mouse, audio, and networking only after initialization reaches
   each subsystem. Multiplayer must remain loopback/private until the old
   network and parser surfaces have a security review.
8. Prove single-player and multiplayer as separate launch modes. If the native
   reconstruction lacks a complete SP engine boundary, record that as a source
   limitation rather than fabricating a menu option.

The immediate compiler blocker is the broad reconstructed native header/import
graph. The larger feasibility blocker is whether enough independent,
license-reviewable, portable native core exists outside fixed-address and
generated reconstruction material to form a complete engine.

## Docker and deployment

There is intentionally no Dockerfile, Compose file, CI publishing workflow,
public asset server, or Docker image. Containerizing a probe would create a
misleading product artifact, and publishing reconstructed code is blocked by
the license/provenance gate. Revisit Docker only after a real engine runtime and
publication approval both exist.

## Checkpoint reporting template

Every handoff must include:

```text
Repository:
Branch and commit:
Milestone attempted:
Milestone result:
Files changed:
Commands run:
Tests passed:
Tests not run:
Retail assets touched:
Browser test requested:
Known regressions:
Next compiler/runtime blocker:
Upstream contacted: no
```
