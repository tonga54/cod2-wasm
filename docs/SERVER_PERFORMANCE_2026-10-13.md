# Raspberry Pi server performance — 2026-10-13

The selected optimization keeps the production **30 Hz** simulation, original
collision capsules, contact epsilon, hit ordering, movement, damage, animation,
bot difficulty and network format. Release remains `-O2` with standard SSE2
floating-point arithmetic and existing hardening. No approximate math is enabled.

## What changed

The collision loader emits flat brush lists. Most visited brushes fail a cheap
bounds test, but entering the complete plane solver still costs register/call
setup, particularly with the i386 engine emulated on ARM. The cheap rejection
now inlines before the plane solver. Axial brush faces use their exact coordinate
axis instead of dot products with two zero terms; sloped faces retain the general
solver and both paths share the original clipping logic.

For immutable world lists, each eight consecutive brushes also has a cached union
bound and contents mask. A missed group skips those eight entries. Surviving
groups retain the original order, overlap behavior and full solvers. Lists below
16 or above 1,024 entries fall back to individual checks. Map changes invalidate
the cache; keys include the node, brush-list pointer and count. Native caches are
per thread; temporary and moving models use their original path. Cache storage
is bounded at roughly 1.8 MiB per engine thread and does not change legacy structs.

## Measurement on the actual Pi

Raspberry Pi 5, four Cortex-A76 cores at up to 2.4 GHz, 8 GB RAM, existing ARM64
gateway and QEMU 10.0.13/i386 runtime. One isolated Carentan TDM room, eight
automated protocol clients and 16 Hard bots; production remained separate.
Clients completed the real handshake, chose teams/weapons, moved/turned/fired
with a 60-input/s target, and received/acknowledged snapshots. They do not render
the game or represent eight independent human/browser sessions.

Each row is one 60-second window after joining and a 15-second warmup. Bot routes
and combat vary between runs, so these samples do not establish a universal
percentage speedup. CPU is expressed as a percentage of **one core**. The observer
uses kernel timestamps at the server interface and omits fragmented messages.

| Build | CPU / one core | Game packets/s/client | Worst client gap p99 | Largest gap |
|---|---:|---:|---:|---:|
| Baseline `9796539` | 93.9% | 29.22 | 66.8 ms | 100.4 ms |
| Axial face optimization alone | 95.9% | 27.43 | 97.0 ms | 201.0 ms |
| Axial faces plus inlined rejection | 96.4% | 26.95 | 92.5 ms | 142.6 ms |
| **Selected: above plus brush groups** | **86.8%** | **30.20** | **55.2 ms** | **121.3 ms** |
| Brush groups with experimental `-O3` | 91.3% | 29.88 | 63.0 ms | 130.7 ms |

The selected sample used 7.1 fewer CPU percentage points (about 7.6% relative)
and sustained the requested cadence. The largest observed gap increased versus
the baseline sample, so this is not evidence of eliminating every hitch. `-O3`
did not improve the selected sample and was not adopted. Every listed run kept
the expected roster and had zero status-query failures and capture drops.
Thermal samples remained below 64°C with `throttled=0x0`.

Machine-readable measurements are in
[benchmarks/pi-server-2026-10-13.json](benchmarks/pi-server-2026-10-13.json).
Ignored raw reports, load clients, build logs and the measured engine are under
`out/server-perf/`. The diagnostic `-pg` runs changed scheduling substantially
and are excluded from the capacity comparison.

## Correctness and operating limits

The collision regression compares 100,000 brush sweeps and 40,000 cached/uncached
results, with exact hit fractions, normals, contents and materials. Another
4,000 cases exercise brush groups, overlaps, partial/max/fallback lists,
contents masks, duplicate/invalid indices and map reloads. A separated-group
fixture reduces visited brushes from 1,024 to eight with the same impact.
Existing checks cover shared BSP leaves, recursive nodes, independent thread
caches, wall slides, prone crawling, car/sill gravity, slopes and the former
square-shoulder collision failure. Bot navigation/combat and collision-loading
regressions also pass.

Each room's simulation still runs on one engine thread. Additional CPU quota
does not parallelize that room. Separate rooms can use different Pi cores, but
aggregate room load must also be measured. The configured 64 player slots and
32-bot limit are not a claim that the Pi can sustain either maximum at full
quality. See the earlier [30/50 Hz test](TICKRATE_BENCHMARK_2026-10-12.md) for
overload cases. A new higher-roster and long-duration capacity test remains
pending because LAN SSH access became unavailable; the selected 24-participant
sample is the validated case from this change.

The implementation and release artifacts can be prepared locally. Applying
them to the Pi requires its SSH connection; preserve the existing HTTPS/security
Compose overlay when updating. The prepared release manifest records the
committed revision and exact image/runtime hashes. Production deployment is
pending until network access is restored.
