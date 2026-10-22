# Browser memory, server load and gateway security — 2026-10-20

A player reported heavy lag and a browser tab that sometimes closed. Both fit a
tab that holds too much memory (the browser kills it, and swapping before that
stutters everything) and a host room that cannot keep up with its bots. This
change lowers the tab's memory, keeps a room playable when its host runs out of
CPU, bounds the queues that turned a stall into seconds of delay, and hardens
the public entry point.

## Browser memory

### What held the memory

| Cause | Size | When |
| --- | --- | --- |
| `cod2_browser_renderer.iwd` copied into the page (MEMFS) | 400 MB | Whole session |
| Every cached archive hashed again with Web Crypto, one buffer per archive | up to 400 MB more | Every start on HTTPS |
| A download accumulated chunk by chunk before becoming a Blob | up to 400 MB more | First visit |

`Runtime.getHeapUsage` on the old client at the main menu reported 478.7 MB of
ArrayBuffer storage: the 473.4 MB of archives plus a few MB.

### What changed

- **Archives of 128 MiB or more are read on demand.** The renderer IWD stays in
  IndexedDB and the Emscripten node reads 256 KiB chunks from the stored Blob,
  with a 32 MiB LRU cache. The engine reads its ZIP exactly as before
  (`fopen`/`fread`/`fseek`); only the bytes come from storage. Smaller archives
  (the 73 MB audio IWD, whose music the engine opens during matches) are still
  copied.
- **Reads go through a worker when the page is cross-origin isolated** (the
  framework already sends COOP/COEP). The worker reads with `FileReaderSync`
  into a `SharedArrayBuffer` while the page waits on it; elsewhere the page
  reads by synchronous XHR as x-user-defined text. Both prepend a zero byte so
  a chunk starting like a byte order mark is not decoded as Unicode. The
  worker allocates 128 KiB of JS heap per read: its thread never idles, so
  without that nothing started a collection and it kept every byte it had read
  (+230 MB per pass in an isolated experiment, released only by terminating it).
- **A failed probe falls back**: worker, then XHR, then the old copy.
  `?archives=memory` copies everything, `?archives=demand` reads every archive
  on demand, `?archives=xhr` skips the worker.
- **No whole-archive hash on every start.** Downloads are hashed in the
  existing bounded worker (`asset-sha256.js`) on HTTP and HTTPS. A cached copy
  was hashed when stored and the cache drops it when the manifest version
  changes, so a start checks its size and ZIP signature only.
- **Downloads are folded into 8 MiB Blob parts** as they arrive, and the client
  heap is created after the download.
- **Failures are visible.** A lost WebGL context, an out-of-memory abort, a
  WASM trap or unreadable stored files stop the engine and show a message
  ("Reload the page to play again", "Close other tabs…") instead of a frozen
  frame.

### Measurements

Development Mac, Chromium (Cursor's browser), the room below; every row is a
fresh renderer process and the same build, switching only `?archives=`.
Footprint is macOS `footprint` of the tab's renderer process after a forced
garbage collection; the tab was hidden, so the engine loop was driven by
`_web_background_frame` and render targets were not drawn.

| Client | Main menu | In a match (Carentan, 8 Hard bots) | `CL_InitCGame` |
| --- | ---: | ---: | ---: |
| Archives copied (previous behavior) | 734 MB | 1,149–1,204 MB | 1.58 s |
| On demand through the worker (new default) | **375 MB** | **815–836 MB** | 2.04 s |
| On demand through XHR (fallback) | — | ~1,025 MB (no forced GC) | 3.47–3.68 s |

The old build's cached start peaked at 1,178 MB or more in the tab while it
hashed and copied the archives; the new first visit, downloading all 473 MB,
peaked at 147 MB during the download and 410 MB when the menu appeared.

A map load reads about 190 MB of distinct data from the 400 MB renderer IWD
(90,238 engine reads, mostly through the 1 KiB stdio buffer; 877 chunk reads).
The worker sustained 660–1,050 MB/s in isolation; the XHR path about 250 MB/s
before decoding. Reading on demand therefore costs about half a second per map
load on this machine, for roughly 350 MB less memory all session.

Not measured: Firefox, Safari, Windows, low-end laptops, render targets of a
visible tab. A browser without synchronous Blob reads keeps the previous copy.

## Network queues

- **Client commands**: `WebNet_Send` drops a command when more than 8 KiB are
  already waiting (was 256 KiB, tens of seconds of input). Every command packet
  repeats the previous commands, as over UDP.
- **Gateway snapshots**: a player's backlog is capped at 64 KiB (was 256 KiB,
  about ten seconds at 25 KB/s); stale snapshots are dropped as UDP loss.
- **`net.ipv4.tcp_notsent_lowat=16384`** on the gateway container keeps at most
  16 KiB unsent in the kernel per socket, so a stalled player's backlog reaches
  the gateway's limit instead of the kernel's multi-megabyte buffers. A TLS
  proxy in front (the private Nginx overlay) owns the player-facing socket:
  give its service the same `sysctls:` entry.

TCP still holds every later packet behind a lost one (head-of-line blocking).
Removing that needs unreliable datagrams (WebRTC data channels or
WebTransport) and an open UDP port on the host's router.

## Server load

### Bot governor

Each room's engine publishes, once a second, `sv_cpuLoad` (the share of one
core the process used, QEMU included on the Pi) and `sv_lateFrames` (the share
of simulation frames run behind schedule), as read-only dvars outside
serverinfo. `browser_bots.gsc`'s governor, 20 seconds after a map starts:

- removes one bot after **3 consecutive seconds** with ≥ 10% late frames or
  ≥ 90% CPU, preferring an idle bot of the larger team, and tells the room
  "Server busy: a bot left to keep the match smooth.";
- brings one back after **20 calm seconds** (no late frames, ≤ 70% CPU), at the
  room's difficulty and never above its bot count, and never within a minute
  of a removal;
- records the count the host carried in `scr_bot_capacity`, which later maps of
  the rotation start with instead of overloading again.

`self botRemove()` queues `botkick <client>`, which drops the slot only if it
is still a test client, without `kick()`'s temporary ban; a bot whose kick is
queued is neither counted nor chosen again (a slow server can run several
script seconds before the command executes).

| Run (Pi image: i386 engine under QEMU 10.0.13, ARM64 container, Mac host) | Result |
| --- | --- |
| 32 Hard bots, Carentan TDM, no CPU limit, 100 s | 37.1% of one core, 30.3 simulation frames/s, no bot removed, no script error |
| 24 Hard bots, container limited to 0.3 CPU | `late=99%`, 3 s hitches; one distinct bot left per removal (24 → 12) as hitches fell to 0.8 s |

A Mac core is faster than a Pi 5 core; these runs validate the mechanism, not
the Pi's capacity. The earlier Pi measurements (eight clients and 16 Hard bots
already at 94–97% of a core) are where it is expected to act.

### Other server changes

- The dedicated server no longer prints a line for every connectionless packet
  (`[cnx] SV_ConnectionlessPacket`); `sv_packet_info 1` still logs them.
- Containers: `no-new-privileges` on all three services, `cap_drop: ALL` on the
  gateway and the server (which keeps `NET_RAW` for
  `benchmark-connected-server.py`), PID limits.
- Pi updates: the updater re-checks that nobody is connected before loading
  images, before rebuilding the private archives and before starting the
  candidate stack, and stops the candidate (which plays a real room with bots
  for minutes) as soon as a player connects, without rejecting the release. Host
  preparation runs at `nice 19` and idle I/O priority, candidate containers at
  the lowest CPU weight. `--install` also writes `Nice=19`, `CPUWeight=1` and
  `IOSchedulingClass=idle` into the systemd unit; existing installations keep
  their unit until reinstalled.

## Gateway security

- **Connectionless commands**: only `getchallenge`, `connect`, `getinfo` and
  `getstatus` reach an engine. `rcon`, voice, authorization, PunkBuster and any
  other command are dropped at the gateway.
- **Per-connection budgets**: 200 datagrams/s (burst 400), 128 KiB/s (burst
  256 KiB) and 10 connectionless commands/s (burst 20); a client sends about 60
  commands a second. Excess is dropped as UDP loss; more than 1,000 drops within
  ten seconds closes the connection (1008).
- **Datagram size**: at most 2,048 bytes (the client netchan sends at most 1,400
  and `connect` about 1 KiB); larger messages close the connection (1009).
- **Closed connections** (October 22): when a browser's WebSocket closes, the
  gateway sends `disconnect` from the UDP port it gave that browser, and the
  engine drops the player at that exact address at once instead of after
  `sv_timeout` (30 s). Browsers cannot send it: it is not on the list above.
- **HTTP**: 15 s to send request headers, 30 s for a whole request;
  `X-Frame-Options: DENY` and `Referrer-Policy: same-origin` on proxied pages
  (the framework already sends COOP, COEP and `nosniff`).

## Verification

| Check | Covers |
| --- | --- |
| `scripts/test-web-archives.cjs` | Bounded download parts, worker hashing on HTTP and HTTPS, on-demand node reads across chunks and byte-order-mark lookalikes, 32 MiB eviction, worker reads on real threads with shared memory, XHR and copy fallbacks, unreadable storage, GPU reset and memory messages |
| `scripts/test-lan-validation.cjs` | Downloads hashed in the worker on both origins; cached copies skip the whole-archive hash |
| `scripts/test-web-net-errors.cjs` | Commands dropped behind more than 8 KiB of backlog |
| `downstream/gateway/filter.test.mjs` | Command allowlist, size limit, budgets and disconnection, bounded snapshot backlog, headers and timeouts, `disconnect` from a closed browser's port |
| `scripts/test-closed-browser-disconnect.py` | The engine drops only the client at that exact address, with mutants |
| `scripts/test-server-load.py` | The real load meter compiled under ASan/UBSan, with mutants |
| `scripts/test-bot-governor.cjs` | The production governor second by second, queued kicks included |
| `scripts/test-pi-updater.py` | Idle re-checks, the candidate watchdog, priorities and CPU weight |

Every test in `scripts/test-static.sh` passes except `test-sprint.py`, which
fails identically on `main` before this change (`UI_JoinMenu_WantsKeys`
undeclared in its fixture). Inside the Emscripten build container,
`test-baked-shadow-filter.py` also fails on unchanged sources (a surviving
`sunVis < 0.0f` mutant with that container's compiler); it passes on the host.
