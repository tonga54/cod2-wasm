# Call of Duty 2 downstream WASM runbook

## Scope and non-negotiable boundaries

This checkout is a local-only feasibility lane for a new browser integration
around OpenCoD2's reconstructed native source. It is not currently a playable
browser port and must never be presented as one.

- Do not inspect, restore, copy, build, or derive from the removed inherited
  `src/web` implementation, removed `build/web_gen` output, or any other
  existing WebAssembly port.
- Do not invoke the repository's inherited/root Emscripten target. The only web
  build is `downstream/wasm/CMakeLists.txt`, entered by `scripts/build-web.sh`.
- `wolfet-wasm` is a product/architecture reference only. Call of Duty 2
  browser platform code is implemented independently in this checkout.
- Never commit, package, upload, or publicly serve owner IWDs.
- Do not publish this source, a binary, a container, or a hosted runtime until
  the source reconstruction's license and provenance have been reviewed.
- Do not submit patches, issues, pull requests, or messages upstream.

The remote is deliberately fetch-only:

```text
upstream fetch: https://github.com/opencod2/opencod2.git
upstream push:  DISABLED
```

All downstream work is local on branch `devel`.

## Source-completeness and license audit

Baseline `f70e697476fceeb4f53de677e1c5d5fe12a00b36` is a work-in-progress
source-level reconstruction. Its README says the native client can build and
boot but may crash or omit subsystems. That native claim was not re-verified in
this lane because the host lacks the documented 32-bit multilib environment.

The tree contains the complete-looking multiplayer families `client_mp`,
`server_mp`, `game_mp`, `cgame_mp`, and `ui_mp`. It contains **zero** files in
corresponding `*_sp` families. Shared directories named `game`, `cgame`, and
`ui` do not constitute a single-player engine. Therefore:

| Mode | Honest status | Evidence |
| --- | --- | --- |
| Single-player | Unavailable from this source base | No SP client/server/game/cgame/UI source families or SP target exist. |
| Multiplayer | Substantial source compiles; no executable links | All 395 selected native/generated translation units compile to wasm objects; the fixed-address data model does not link. |

No repository-wide `LICENSE` or `COPYING` file was found. The README has a
notice describing reconstruction and intended research/interoperability uses,
but it does not provide an explicit redistribution license grant. That is a
publication blocker independent of the technical blockers.

The reconstruction is strongly tied to a 32-bit native layout. The source has
over a thousand fixed-address-looking constants, hundreds of explicit
size/field assertions, generated ILP32 data images, BSS blobs, and symbol alias
tables. WebAssembly is ILP32, which is sufficient for compilation, but it does
not reproduce ELF's unified code/data symbol model.

## Verified implementation status (2026-08-14)

### Full client compile

The downstream CMake target now selects the real reconstructed multiplayer
client, renderer, UI, scripting, qcommon, input, networking, SDL/platform,
generated data, and compatibility sources. It excludes native assembly and the
duplicate bundled zlib implementation, uses Emscripten SDL2/zlib, and provides
a downstream OpenGL declaration/wrapper seam without restoring inherited web
code.

`cod2_client_objects` successfully compiles **395 WebAssembly object files**.
This is materially beyond the old MD4-only probe and establishes an honest
compile milestone. It does not establish engine startup.

The decompiler-faithful source still needs the same relaxed implicit-declaration,
pointer-conversion, aliasing, and overflow assumptions used by its native build.
Those warnings are not treated as proof of correctness; several emitted bounds
and size warnings require later runtime/security review.

The selected native entry path is also compiled. `web_main.c` enters the
reconstructed `WinMain`; `mac_main.c` already contains a guarded Emscripten
cooperative `emscripten_set_main_loop` frame path. Neither boundary has executed
because the engine does not link.

### Exact client-link blocker

The explicit `cod2_client` target reaches `wasm-ld` and fails. This is not a
list of ordinary missing platform calls. The generated reconstruction relies on
native linker behavior that WebAssembly intentionally cannot express:

- `build/native_gen/data32.c` and `literals32.c` encode a native 32-bit data
  image containing both data addresses and code pointers, often declaring the
  targets uniformly as byte arrays.
- `import_pointers_native.c` declares targets uniformly as functions, including
  symbols which are actually data.
- `src/stubs/link_stubs.c` contains data placeholders for symbols which are
  implemented as functions elsewhere.
- The native CMake path depends on `--defsym`, `--allow-multiple-definition`,
  and common-symbol merging to reconcile these representations.
- Wasm has separate linear-memory data and function-table references. The
  linker rejects a symbol used as both `WASM_SYMBOL_TYPE_FUNCTION` and
  `WASM_SYMBOL_TYPE_DATA`; broad undefined-symbol suppression cannot fix this.

Representative diagnostics include:

```text
CreateObjSpecifier: function in MacAppleEvents.c, data in link_stubs.c
MacDisplay_CreateScreenContext: function in macos_compat.c, data in data32.c
ZN11CColorArrayD1Ev: function in COpenGL.c, data in literals32.c
sv: data in bss.c, function in import_pointers_native.c
CSoundObject::sReadCallback: duplicate data in MacMSS_Object.c and data32.c
```

The remaining task is a wasm-aware reconstruction-data generator, not a linker
flag or a handful of no-op stubs. The explicit diagnostic can be reproduced by
setting `COD2_ATTEMPT_CLIENT_LINK=1`; failure is currently expected.

### Browser artifact

The generated site is deliberately labeled **not playable**. It contains:

- the old isolated native MD4/checksum diagnostic, clearly separated from the
  engine compile result;
- a loopback-only validator for the owner-mounted retail IWD inventory;
- no fake Play button, menu, level, renderer, input, or multiplayer claim.

The diagnostic remains useful as a minimal proof that generated JS/WASM loads
and executes in the browser. It is not counted as the client milestone.

## Owner-data boundary

The staged owner data used by the local portal is:

```text
/home/ted/Development/wasm/data/cod2/main
```

It contains the exact current Steam inventory: 16 `iw_00.iwd` through
`iw_15.iwd` archives plus 12 `localized_english_iw00.iwd` through
`localized_english_iw11.iwd` archives, 28 files totaling 3,685,129,248 bytes.
No archive is tracked by git or copied into the generated site.

`scripts/generate-owner-manifest.sh` accepts an explicit absolute owner `main`
directory. It rejects missing files, symlinks, and non-ZIP headers, then emits a
private manifest with the exact allowlisted path, byte size, SHA-256, and
`504b0304` header for each archive. The manifest is generated under ignored
build output only; it is not committed.

The portal mounts the owner data read-only at `/owner-data` and exposes it as
same-origin `/local-data/` from a service bound to `127.0.0.1`. The browser:

1. refuses asset validation on non-loopback hostnames;
2. validates the manifest schema and exact ordered 28-file allowlist;
3. sends `HEAD` for the expected size;
4. sends a four-byte range request and validates the ZIP/IWD header;
5. never fetches an entire IWD during validation.

The SHA-256 values prove what the local build-time generator inspected. The
browser does not re-hash 3.7 GB. The read-only mount, exact served sizes, and
headers form the runtime handoff.

All 28 IWDs cannot be copied into main-thread MEMFS: their 3.685 GB payload
nearly exhausts wasm32's 4 GB address space before the engine heap, stack,
renderer allocations, or decompression buffers exist. A playable runtime needs
an archive-aware lazy filesystem or a worker/pthread-backed synchronous bridge.
Pretending to preload the whole install would guarantee an out-of-memory
failure.

## Reproducible build

Known toolchain used here:

```text
Emscripten 6.0.6
CMake 4.3.4
Node 24.x
```

Activate Emscripten, or set `EMSDK` to an emsdk checkout. From the repository
root:

```bash
COD2_OWNER_DATA=/absolute/path/to/Call-of-Duty-2/main \
  ./scripts/build-web.sh
```

Without `COD2_OWNER_DATA`, the source still compiles and the site is produced,
but no owner manifest is emitted. The build never guesses a Steam path.

The default build compiles the full object graph plus the diagnostic site. To
reproduce the expected link failure:

```bash
COD2_ATTEMPT_CLIENT_LINK=1 ./scripts/build-web.sh
```

Output is under the ignored directory:

```text
out/cod2-wasm-core/site/index.html
out/cod2-wasm-core/site/asset-validator.js
out/cod2-wasm-core/site/cod2_core_probe.js
out/cod2-wasm-core/site/cod2_core_probe.wasm
out/cod2-wasm-core/site/owner-manifest.json  # only with COD2_OWNER_DATA
```

There is intentionally no `cod2.js`/`cod2.wasm` engine artifact yet.

## Automated and Chromium checks

Run:

```bash
COD2_OWNER_DATA=/absolute/path/to/main ./scripts/test-web.sh
```

It verifies:

- the 395-object full source graph still compiles;
- the native MD4/checksum diagnostic returns its expected deterministic values;
- `asset-validator.js` passes Node syntax checking;
- the downstream entrypoint object exists;
- `index.html`, JavaScript, and WASM are served successfully over loopback HTTP.

The local portal is already configured at:

```text
http://127.0.0.1:8014/
```

The serialized Chrome smoke on 2026-08-14 verified:

- the page title and visible “Not playable” status;
- private manifest discovery for all 28 files / 3,685,129,248 bytes;
- successful `[28/28]` size/header validation through the read-only mount;
- the two deterministic native checksum values;
- zero browser-console errors.

Chrome did not receive or retain the IWD bodies, and the smoke tab was closed.

## Next honest milestone

Do not begin renderer polish or claim a menu until the link model is repaired.

1. Replace the native data/import conversion with a downstream wasm-aware
   generator that classifies every relocation as function-table or linear-data.
2. Generate typed code-pointer fields and data-pointer fields for data/literal
   images, eliminating cross-kind symbol collisions instead of suppressing them.
3. Replace placeholder byte arrays with named behavioral functions only where
   the native implementation is truly absent; remove duplicate reconstructed
   statics deterministically.
4. Link an engine WASM and reach `Com_Init` with a minimal, lazy subset of owner
   archives. Record the exact archive dependency graph before adding more data.
5. Only after initialization runs, validate SDL canvas creation, the cooperative
   main loop, WebGL compatibility, keyboard/mouse input, and audio in that order.
6. Keep networking private/loopback until the old parser and protocol surfaces
   receive a security review.

Single-player requires a different, complete source base;
it cannot be created from this repository by adding a menu option.

## Handoff template

```text
Repository: /home/ted/Development/wasm/cod2-wasm
Branch: devel
Milestone: 395-object full multiplayer client compile; link blocked
Playable: no
Owner assets committed/copied: no
Browser URL: http://127.0.0.1:8014/
Upstream contacted: no
Published/pushed: no
```
