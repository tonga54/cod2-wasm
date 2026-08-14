# Call of Duty 2 WASM runbook

Status: **Still in development**

## Immutable inputs

`source-lock.json` pins wasm-game-framework 0.7.3 at
`be0b81301c5f12f09e445a3bc765b7709603265e` and records reconstructed-source
baseline `f70e697476fceeb4f53de677e1c5d5fe12a00b36`. Builds create an isolated
framework worktree at that exact commit.

Do not restore or use the removed inherited `src/web` implementation,
`build/web_gen` output, another Call of Duty 2 WebAssembly port, or a compiled
third-party browser artifact. Do not contact or submit anything upstream.

The pinned reconstruction baseline contains no repository-level `LICENSE` or
`COPYING` file. Keep the Docker images local until the repository maintainer
documents the distribution terms; this publication boundary is independent of
the native link blocker below.

## Source and mode audit

The reconstructed source contains multiplayer `client_mp`, `server_mp`,
`game_mp`, `cgame_mp`, and `ui_mp` families. It contains no corresponding SP
families or SP target. The framework therefore exposes only `cod2-mp`.

The selected reconstructed client, renderer, UI, scripting, qcommon, input,
networking, platform, generated-data, and compatibility sources compile to 395
WebAssembly object files. Native assembly and duplicate bundled zlib sources
are excluded; the browser target uses Emscripten SDL2 and zlib.

## Exact native blocker

The explicit `cod2_client` target reaches `wasm-ld` and fails because the
reconstruction's native generated data representation relies on symbol aliases
which WebAssembly cannot encode:

- `data32.c` and `literals32.c` contain a native 32-bit data image with both
  data addresses and code pointers;
- `import_pointers_native.c` represents targets uniformly as functions,
  including symbols which are data;
- compatibility placeholders collide with reconstructed functions and statics;
- the native build relies on `--defsym`, multiple definitions, and common-symbol
  merging;
- WebAssembly separates linear-memory data from function-table references and
  rejects cross-kind symbols.

Set `COD2_ATTEMPT_CLIENT_LINK=1` when intentionally reproducing that failure:

```bash
COD2_ATTEMPT_CLIENT_LINK=1 ./scripts/build-web.sh
```

Do not claim a native menu or gameplay until this generator model is repaired
and an engine executable reaches `Com_Init`.

## Framework package

The staged public directory contains only:

```text
cod2-diagnostic.svg
cod2_core_probe.js
cod2_core_probe.wasm
game-adapter.js
wasm-game-data.json
wasm-game-framework.json
wasm-game.json
```

The JS/WASM pair is built locally from `core_probe.c` and the reconstructed MD4
source. It is a diagnostic, not the engine. The adapter reports `launcher`,
then `loading`, then `crashed`; it never reports `menu` or `gameplay` and never
requests input capture, fullscreen, identity, or graphics controls.

The framework owns the document, CSS, service worker, PWA manifest, setup UI,
and viewport. The package checker is mandatory:

```bash
node ../wasm-game-framework/scripts/check-game-package.js \
  out/cod2-wasm-core/site
```

## Required data boundary

`site/wasm-game-data.json` pins 28 `main/*.iwd` paths, sizes, ZIP signatures,
and SHA-256 values totaling 3,685,129,248 bytes. Docker stores this tree under
`/data/main`. The framework setup endpoint is the only write path, and the
allowlisted `/game-data/files/:key` endpoint is the only read path. `/data`,
`/local-data`, and direct `main/*.iwd` URLs must remain inaccessible.

The adapter uses the canonical container-to-IndexedDB client for only the
706-byte `localized_english_iw11.iwd` diagnostic representative. Do not load
the complete archive set into MEMFS. A runnable engine needs an archive-aware
lazy filesystem or another bounded synchronous bridge.

No IWD enters Git, the public site, or a Docker image. Generated JS/WASM stays
under ignored `out/` output.

## Verification

```bash
./scripts/test-web.sh
./scripts/build-docker.sh
./scripts/test-http.sh
```

These checks cover:

- the full reconstructed object compile and native diagnostic output;
- framework v0.7.3 package and adapter validation;
- exact state transitions and safe repeat start;
- canonical PWA metadata and neutral ready-state copy;
- suite and `cod2-mp` locked images;
- COOP/COEP, WASM range requests, setup status, exact mounted-data validation,
  private cache headers, and inaccessible `/data` routes;
- absence of tracked/generated IWD, WASM, data, HTML, CSS, service-worker, and
  web-manifest artifacts.

Chromium testing is intentionally deferred until the shared serialized browser
slot is granted. At this milestone it can verify only launcher, setup/cache,
diagnostic output, PWA, and security behavior; it cannot verify gameplay.

## Next milestone

1. Build a wasm-aware generated-data/import tool that classifies each
   relocation as function-table or linear-memory data.
2. Eliminate every cross-kind and duplicate symbol deterministically.
3. Link `cod2.js`/`cod2.wasm` and reach `Com_Init` using a bounded lazy archive
   subset.
4. Add authoritative native state and capture-intent exports only after a real
   menu and controllable snapshot exist.
5. Then validate WebGL, resize/projection, menu pointer mapping, WASD/mouse,
   Escape, network parsing, audio, persistence, and recovery in that order.
