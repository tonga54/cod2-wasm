# Updating the game

The public repository is [tonga54/cod2-wasm](https://github.com/tonga54/cod2-wasm).
Automatic host releases follow `main`. Other branches are never installed.

## Browser players

The game checks the installed host version once a minute while the tab is
visible. After a new client build is installed, the native main menu displays
**New client version available** and **Update game**. The player must leave
the match before the button appears. The game never reloads automatically.
There is no HTML popup, guide link or repository-push notice. The gateway also
suppresses the obsolete host-update notice for clients running older scripts.

The generic launcher is hidden in the document head before configuration loads;
players see the game startup screen and then its native menu. The native main
menu's **Fullscreen: Off/On** text toggles the browser display mode. There is no
floating HTML fullscreen button.

In Chrome/Edge, fullscreen entry requests capture of Escape only. A short
press opens the native menu; holding Escape retains the browser's fullscreen
exit gesture. F10 also exits. Keyboard capture requires browser permission and
is unavailable in some browsers; denial falls back to browser behavior. See
[Chrome's Keyboard Lock documentation](https://developer.chrome.com/blog/better-full-screen-mode).

## Standard Docker hosts

From the repository directory:

```sh
python3 scripts/update-local.py
```

This fetches Git references and checks for changes without installing or
restarting Docker. Exit codes are `0` for current, `1` for an available update,
and `2` when the check failed or the history needs review.

To install and rebuild an empty host:

```sh
python3 scripts/update-local.py --apply
```

The installer requires `origin` to point to this repository, branch `main`,
a clean working tree, and a fast-forward update. It does not reset, stash or
merge local history. It checks for active players before changing files and
before restarting. It preserves `data/main/`, regenerates the private subset,
builds the browser/native server/gateway, then runs `docker compose up -d`.

A failed build does not restart existing containers. Fix the failure and retry;
`--apply` can rebuild an already current checkout. Keep a backup of your original
files. Requirements and the pinned framework are listed in the [README](../README.md).

## Raspberry Pi hosts

The GitHub **Validated Raspberry Pi release** workflow builds the browser,
ARM64 web/gateway and ARM64 supervisor/QEMU images for code pushes to `main`.
Compilation and source/JS regression tests must pass before it publishes an
immutable `pi-<full commit SHA>` prerelease. Packages contain public code only;
private IWDs and credentials never enter GitHub Actions or releases.

`scripts/update-pi.py` checks every five minutes through a user systemd timer.
It requires a clean `main` checkout, a fast-forward source update, and the
owner's original IWDs, and the optional [hit sound](#owner-hit-sound), in
`~/cod2-original/main` (private, outside Git). It
validates package checksums, ARM64 image identities and the clean source/build
revision, regenerates private assets in staging, and checks their exact public
manifest hashes. A separate Docker project then exercises three maps, two bots
per map, browser metadata and WebSocket-to-native challenges.

The updater waits while players or pending joins are present. During the brief
idle switch, a host maintenance flag blocks new joins. It replaces the three
game services, checks the new private manager/native UDP path, then advances the
Pi checkout. Failed activation restores the previous assets, images and Compose
configuration; unfinished transactions are recovered on the next timer run.
The last good package/images remain available. A rejected code commit is not
retried automatically; a new main commit or explicit `--retry` is required.
Network failures and unfinished CI builds are retried without replacing the
running game. These checks catch build/startup regressions; they cannot prove
that every future change is bug-free.

Install on a prepared Pi with Docker Compose 2.24.4+ and user lingering enabled:

```sh
python3 scripts/update-pi.py --install
systemctl --user list-timers cod2-auto-update.timer
journalctl --user -u cod2-auto-update.service -n 60
cat ~/.local/state/cod2-updater/status.json
```

To check immediately or retry an explicitly reviewed rejected commit:

```sh
systemctl --user start cod2-auto-update.service
python3 ~/.local/lib/cod2/update-pi.py --retry
```

Pause the timer and let any active update finish before a manual deployment:

```sh
systemctl --user stop cod2-auto-update.timer
systemctl --user status cod2-auto-update.service
# After the manual deployment:
systemctl --user start cod2-auto-update.timer
```

### Stalled updates

Two statuses keep the Pi on its installed build until someone acts:

- `Prepared owner assets differ from the published build`: the commit changed
  an input of the private archives without regenerating
  `site/wasm-game-data.json`. Inputs include the generator scripts, scanned
  engine sources and `downstream/server/*.gsc`; for example
  `browser_spawn_safety.gsc` becomes `maps/mp/gametypes/_spawnlogic.gsc`. Run
  `python3 scripts/prepare-browser-bootstrap.py` on the build machine and
  commit the manifest. On October 16, 2026 the Pi rejected `428bcbc` and
  `fd9057b` this way: its renderer archive was 848 bytes larger than the
  published one, and the build machine produced the same archive once rerun.
  A missing or different [owner hit sound](#owner-hit-sound) causes the same
  status, because the published audio archive includes the build machine's.
- `At least 1.5 GiB free space is needed for a staged update`: every run
  downloads only `manifest.json` and waits. Check `df -h /` and
  `docker system df`. Build cache from manual Pi builds can be removed with
  `docker builder prune -af` without touching containers or rollback images;
  this freed 4.9 GB on the configured Pi on October 17, 2026. Rejected
  releases keep their staging directories (about 440 MB each) until the next
  successful update.

The [manual Pi deployment procedure](RASPBERRY_PI.md) remains available for
first installation and recovery. The standard `update-local.py --apply`
procedure does not build the Pi runtime. HTTPS credentials, certificates and
`~/cod2-edge/compose.security.yaml` are preserved outside the update package.

## Asset delivery and caching

Prepare the owner's files on the build machine:

```sh
python3 scripts/prepare-browser-bootstrap.py
```

The current Toujane/Carentan/Dawnville subset is approximately **307 MB**. Each browser
downloads it from the host over HTTP/HTTPS and stores it in IndexedDB. The
manifest verifies archive sizes and SHA-256 hashes; a new asset version
invalidates the old cache. Storage belongs to each browser and origin, so
moving from a Mac URL to a Pi URL requires a fresh initial download.

Players can join an already prepared host without individually importing a ZIP.
A person setting up another host must supply their original CoD2 IWD files in
`data/main/`. These files are not supplied by a Git clone, public image or Release.
Original assets remain private, read-only mounts; the repository contains their
public manifest hashes rather than the archives.

### Owner hit sound

An optional `hitmarker.wav` beside the original IWDs replaces the sound of
`MP_hit_alert`, which the original scripts play to the attacker for each hit.
Like the IWDs it belongs to the owner and is never committed. The generator
stores it as `sound/browser/hitmarker.wav` in `cod2_browser_audio.iwd`: a
stereo file unchanged, a mono one as stereo with both channels the same,
because the engine plays a mono 2D sound at half level on each side. It moves
the alias to `soundaliases/browser_hitmarker.csv`. The new row keeps the
original volume, pitch, `auto2d` channel and `all_mp` load spec, but drops the
original 35 ms start delay. `check-private-assets.py` verifies the alias, and
that the packaged bytes are what the generator makes of the owner file.
Without the file the archive keeps the original sound.

The file must be a 16-bit PCM WAV, mono or stereo, 11025–48000 Hz, at most
two seconds long, reaching a tenth of its peak within 50 ms; the generator
rejects anything else. Every host that prepares the published build needs a
byte-identical copy: `data/main/hitmarker.wav` on the build machine and
`~/cod2-original/main/hitmarker.wav` on the Pi. Changing the file changes the
asset version, so each browser downloads the archives again once.

## Version checks

`/build-info.json` identifies the source revision, dirty status and build-content
ID. `GET /version` reports the installed build, with repository notices disabled.
Host release checks run on the Pi, independently of player requests. Missing
build metadata reports `unknown` rather than claiming the browser is current.

Older deployments without this feature need one manual update before browser
notices become available.
