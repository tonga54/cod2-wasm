# Client and server work reduction — October 16, 2026

Baseline: `428bcbc879e326f97242ac7a1152df7a2a52d7a7`. Private assets, map detail,
texture filtering, lighting, shadows, particles, output resolution, antialiasing,
audio, collision dimensions and simulation rate retain the same settings.

## Client

The browser CPU profile identified vertex copying, buffer submission and repeated
index traversal as major costs. The old ARGB/BGRA path copied whole interleaved
meshes just to reorder four color bytes. The vertex shader now selects RGBA,
BGRA or ARGB components before the original tint, lighting and clamp operations.
Static buffers retain original bytes on the GPU. Dynamic buffers retain their
original upload and mutation behavior, without the extra conversion buffer.

The format travels through the existing per-draw WASM/JS call. It is consumed
once by the renderer so unrelated GL draws still interpret RGBA. Uniform uploads
are cached per program. There are no additional draw calls, framebuffer passes,
textures, shader variants or fragment texture samples.

The browser's C draw path passes its already-validated index bounds to
`glDrawRangeElements`, avoiding the second index scan in JavaScript. Persistent
index buffers retain authored numbering; CPU arrays retain their rebased range.

Matrix loads compare all values, including signed zero, before invalidating
shader versions. In-place changes and matrix-stack replacements still propagate.
Fresh shader texture-matrix versions start invalid so the first identity is
uploaded even when its data version is zero. Unused normal uniforms no longer
trigger inverse/transpose allocations.

### Browser measurements

Chrome on the local Apple M4 rendered Carentan with one client, no bots,
Thompson, position `(0 1000 52)`, yaw zero and centered pitch. Output remained
1280×720 with the existing antialiasing and all visual settings. The isolated
server/map restarted for each capture. Measurements use the existing frame and
driver counters, without the CPU sampling profiler; each window contains 600
frames after 12 seconds in the pose. Browser refresh limits were disabled only
in the QA launch, not in the game or deployment configuration.

| Capture | Hip FPS | ADS FPS | Main-thread mean, hip / ADS (ms) | Frame p95, hip / ADS (ms) |
| --- | ---: | ---: | ---: | ---: |
| Before 1 | 178.1 | 158.9 | 5.47 / 6.17 | 6.59 / 6.75 |
| After 1 | 246.3 | 211.0 | 3.94 / 4.62 | 4.46 / 5.18 |
| After 2 | 223.9 | 171.6 | 4.30 / 5.64 | 5.14 / 7.74 |
| Before 2 | 131.0 | 124.0 | 7.44 / 7.87 | 13.38 / 13.95 |
| Before 3 | 138.6 | 125.9 | 7.03 / 7.75 | 10.67 / 10.86 |
| After 3 | 177.0 | 149.9 | 5.48 / 6.47 | 7.70 / 12.14 |

The second baseline was rerun after an interrupted attempt. The last pair ran
consecutively after validation; no builds or tests ran concurrently with these
timings. The final pair increased average FPS by about 28% hip / 19% ADS and
reduced mean main-thread time by about 22% / 16%. The vertex-attribute phase
dropped from 1.92 / 1.95 ms to 0.53 / 0.67 ms in that pair, consistent with
removing the whole-mesh copies.

These are measurements of one live scene on a shared workstation, not a fixed
replay or a universal speedup. Animated draws varied (253–348 hip, 345–406 ADS)
despite matching camera placement, and GPU query means ranged from 2.08 to
4.07 ms. Frame-time tails did not improve in every comparison, including the
last ADS pair. None of the twelve reported windows contained a frame over
33 ms. A display refresh cap can hide the FPS increase in normal use; reduced
CPU work still leaves additional frame budget. All captures are retained,
including the slower samples.

## Server and shared collision code

The existing eight-brush groups first check the coarse swept bounds. Surviving
groups now also check the actual segment, shortened to the nearest hit, using
the same conservative expansion as the individual brush solver. Off-axis or
already-occluded groups can be skipped before visiting their brushes. Brush
order, plane clipping, start-solid behavior, contents and materials are preserved.
Dynamic models keep their original solver. The change also benefits client
prediction through the shared collision implementation.

The deterministic synthetic benchmark compares both production selectors on
identical inputs. Its long-diagonal case visits 1,448,704 brushes instead of
6,015,808 across 32,768 queries, with identical hit results. Three alternating
samples measured roughly 22 ms versus 38 ms for that case on the local Apple M4.
Long axial and stationary cases were approximately unchanged. Short-movement
timings varied during warmup. These are synthetic collision timings, not whole
server CPU or FPS improvements.

A local Docker match with 16 Hard bots on Carentan TDM ran for 45 seconds per
build at the configured 30 Hz. Baseline/candidate measurements were 30.31/30.30
simulation Hz and 27.87/29.10 percent of one CPU core. The matches differed in
combat, pathfinding and respawns (7 versus 14 resets), so these samples do not
establish a whole-server CPU improvement. Maximum game-frame samples were
26/38 ms, including navigation warmup. Tickrate, bot intelligence, trace
precision, network updates and simulation rules were not reduced.

## Correctness and scope

The regression suite compares 100,000 accelerated/exhaustive brush sweeps,
40,000 cached/uncached traces, 4,000 grouped lists and shared BSP leaves. A new
off-axis fixture avoids all 1,024 individual brush visits with the same miss.
The benchmark additionally compares 16,384 paired queries across movement,
diagonal, axial and overlapping stationary cases.

The real WebGL2 color test passed 57 checks across all 256 alpha values, three
color layouts and unlit/0/1/4/8-light shader variants, including format changes.
Pixels match the previous CPU conversion exactly. Matrix tests cover 5,000
repeated loads plus mutable input, matrix replacement, signed zero and nonfinite
values. Existing tests cover static buffer writes/releases, sparse indices,
lighting/shadow cache invalidation and attribute limits.

The Linux dedicated server and Emscripten 3.1.64 client rebuilt successfully.
The complete `scripts/test-static.sh` suite passed against the final linked
client, including the sanitizer-backed collision and gameplay checks. The
separate combined respawn staging package passed its package contract and
4,745-entry private asset check. Reapplying the GL patch preserves the generated
JavaScript byte for byte. Existing compatibility-stub linker warnings remain.

The browser match captures had no JavaScript exceptions. Both baseline and
candidate emitted the existing startup `drawElements: no buffer is bound to
enabled attribute` warning; the isolated color tests emitted no WebGL errors.
Standing, ADS and weapon/world appearance were checked in Carentan. These
measurements do not cover every device, map or crowded combat scenario.

Measurements and complete logs are retained locally under ignored
`out/performance-2026-10-16/`, including baseline JS/WASM/server binaries. The
Raspberry Pi SSH connection timed out. Local Docker server measurements must
not be represented as measurements of the physical Pi.

The temporary browser QA stack was removed after validation. The existing
production containers were not restarted or replaced. Client/server artifacts
and the combined respawn package are built locally; source publication does
not deploy those artifacts.

Reproduce the shader and deterministic collision checks with:

```sh
CHROME_EXECUTABLE=/path/to/chrome node scripts/test-web-vertex-colors.mjs \
  /path/to/baseline/cod2.js out/cod2-wasm-core/site/cod2.js \
  /path/to/playwright/index.mjs out/vertex-color-qa
python3 scripts/benchmark-trace-groups.py /path/to/baseline/cm_trace.c
```
