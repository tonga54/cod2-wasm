# Raspberry Pi tick rate and remote-player movement — 2026-10-12

Keep the default at **30 Hz**. The Pi sustained 50 Hz with two protocol clients,
but 50 Hz overloaded a single engine core with eight clients and 16 Hard bots.
Increasing the tick rate does not eliminate integer-position interpolation jitter.

## Method

Carentan TDM, Raspberry Pi 5, production i386 engine under QEMU, existing
ARM64 gateway, three-core server limit. Production remained running separately.
The QA gateway was published only on Pi localhost and reached through an SSH
LAN tunnel; no public port or authentication configuration changed.

Automated clients completed the actual game handshake, selected teams/weapons,
received and acknowledged snapshots, and moved/turned/fired with a 60-input/s
target (approximately 58/s achieved). They are **not human users or browser
renderers**. Counts were checked through room status. Each reported window
lasted 60 seconds after joining and a 15-second warmup; the largest 30 Hz case
ran for 90 seconds. Runs are samples, not randomized or long-duration capacity
certifications. Bot routes/combat vary between runs.

`benchmark-connected-server.py` measured container CPU and outbound packet
cadence using kernel receive timestamps. It excludes fragmented game messages;
the protocol clients independently reassembled snapshots and recorded arrival
gaps. CPU percentages refer to **one core**, not the entire Pi. Status RTT is
container-local. Client arrival times and ACK RTT include the gateway, SSH
transport and Mac scheduling; they cannot identify production WebSocket jitter.

## Baseline (commit 458aada)

| Connected protocol clients | Hard bots | Requested Hz | Server CPU / one core | Unfragmented game packets/s/client (mean) | Worst per-client gap p99 |
|---:|---:|---:|---:|---:|---:|
| 2 | 0 | 30 | 20.5% | 30.29 | 37.5 ms |
| 2 | 0 | 50 | 27.1% | 49.98 | 25.3 ms |
| 8 | 16 | 30 | 93.9% | 28.32 | 83.7 ms |
| 8 | 16 | 50 | 100.7% | 0.74 | 2,358 ms |
| 16 | 32 | 30 | 101.7% | Incomplete capture | Incomplete capture |
| 16 | 32 | 50 | Not measured | Join did not finish within 60 seconds | Not measured |

At 30 Hz with eight clients/16 bots, one outbound gap reached 181 ms; this
already failed the observer's realtime criterion despite maintaining its
roster. At 50 Hz the same roster stayed connected but status queries timed out
and the game sent updates in bursts. Increasing a global tick setting is not
safe for that load.

The 16-client/32-bot 30 Hz case had 991 **observer socket drops**; those are not
proof of player packet loss. Independent client snapshots nevertheless had
repeated 5–6.5-second gaps, with CPU at a full core and multi-second engine
hitch warnings. The 50 Hz attempt never completed handshakes, so it does not
supply a valid connected-player performance measurement.

All recorded thermal checks returned `throttled=0x0`; temperatures stayed at
or below 65°C. More container CPU allowance cannot make the single-threaded
engine use several cores for the same simulation loop. Catch-up frames are
processed before client snapshots are sent, which explains how simulation
counters can look active while remote movement freezes.

The client `snaps` setting and non-LAN snapshot interval still cap requests at
30. This test's gateway addresses take the server's LAN send-every-tick path,
which is why the 50 Hz light test really delivered about 50 messages/s. A future
general 50 Hz change must review all three settings, script timing, bandwidth
and load; changing `sv_fps` alone is not complete validation.

## Fractional-position correction

`G_PlayerStateToEntityStateExtrapolate` previously truncated each player origin
to integer units before sending it. At high rendering rates, a one-unit change
in rounding error is a large fraction of one visual movement step. The server
now preserves the fractional origin. The existing float field encoding already
supports it, so the browser and packet schema do not change. Angle snapping,
physics, hitboxes, command timestamps and bounded client extrapolation retain
their existing paths.

A deterministic replay of the production interpolator at 125 FPS, 190 units/s,
33 ms snapshots and alternating command sample ages of 65/80 ms measured:

| Position export | Per-frame step-error RMS | Maximum step error |
|---|---:|---:|
| Integer coordinates | 0.191644 units | 0.762715 units |
| Fractional coordinates | 0.000153 units | 0.000508 units |

The ideal step is 1.52 units, so the largest rounding error was about 50% of
that step. This is an isolated numerical result, **not a measured percentage
improvement in subjective gameplay or proof that all network stutter is gone**.

The regression now executes the production trajectory assignments and snapping
block, checking both directions at 20/30/50 Hz. Restoring integer-position
rounding fails. Existing teleport/death/local-player guards, missing-packet
bounds, turns/stops, packet pose updates, clock, event delivery, animation,
bit-field and snapshot tests pass. The i386 layout test runs in the native
builder, since macOS headers cannot build that Linux 32-bit fixture.

In the corrected two-client Pi run, CPU was 21.0%, cadence 30.31 messages/s and
p99 gap 38.6 ms. Average received traffic was approximately 2.30 KB/s/client
versus 2.18 KB/s before (whole-session estimates including startup). In the
corrected eight-client/16-bot sample, CPU was 97.4%, cadence 22.29 messages/s,
p99 gap 142 ms and maximum gap 243 ms. Both old and corrected versions fail
the realtime criterion at that load; preserving float origins has a bandwidth
cost and this crowded configuration should not be recommended as smooth.

The final native artifact was also tested with eight clients and eight Hard
bots at 30 Hz: **81.9%** of one core, **30.13** unfragmented messages/s/client,
worst p99 gap **63.2 ms**, maximum **92.3 ms**, no status-query failures or
observer drops. Both capture and realtime criteria passed for this 60-second
sample. An earlier run overlapping image export was excluded and repeated.

An unchanged production browser also joined the corrected isolated server
alongside one moving protocol client. Its log contained 47 remote-motion
windows, generally around 120 rendered frames/window, with no snapshot/script
errors. This verifies compatibility and continued per-frame processing; the
driver includes walls, turns and stops, so it is not a controlled visual A/B
comparison or proof of zero held frames.

## Evidence

Local raw reports, traffic traces, generated load clients, build logs and
rollback material are under `out/tickrate-pi/` (ignored; no credentials or
private game assets belong in Git). The clean two-client 30 Hz baseline was
repeated after browser asset downloading had completed. The public game and
its authentication were not used as the load-test target.
