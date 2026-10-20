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
players see the game startup screen and then its native menu. F10 toggles the
browser display mode; neither the native menu nor the page has a fullscreen
button.

The game draws at 1920 × 1080 by default, its 3D view at 720 lines. On the
first start after this default arrived, a saved profile still at the old
1280 × 720 default moves to it, once; a resolution chosen afterwards stays.

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
owner's original IWDs, and the optional [hit sound](#owner-hit-sound),
[menu music](#owner-menu-music), [countdown music](#owner-countdown-music),
[end of match music](#owner-end-of-match-music),
[last minute music](#owner-last-minute-music), [sound pack](#owner-sound-pack) and
[class pictures](#owner-class-pictures), in
`~/cod2-original/main` (private, outside Git). It
validates package checksums, ARM64 image identities and the clean source/build
revision, regenerates private assets in staging, and checks their exact public
manifest hashes. A separate Docker project then exercises four maps, two bots
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
  A missing or different [owner hit sound](#owner-hit-sound),
  [menu music](#owner-menu-music) or [sound pack](#owner-sound-pack) causes
  the same status, because the published audio archive includes the build
  machine's; so do missing or different [class pictures](#owner-class-pictures),
  which the renderer archive includes.
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

The current Toujane/Carentan/Dawnville/Burgundy subset is approximately
**375 MB** with the owner's [sound pack](#owner-sound-pack), music and class
pictures; Burgundy accounts for 21 MB of it. Each browser
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

### Owner menu music

An optional `menumusic.mp3` beside the original IWDs replaces the main menu
music: the stream of `music_mainmenu` and `music_mainmenu_mp`, which
`ui_mp/main.menu` loops. Like the hit sound it belongs to the owner, is never
committed and needs a byte-identical copy on every host that prepares the
published build (`data/main/` and `~/cod2-original/main/`). The generator
stores it unchanged as `sound/browser/menumusic.mp3` in
`cod2_browser_audio.iwd`, and moves both aliases to
`soundaliases/browser_menumusic.csv`. The rows keep the original volume
(0.3), `menu` channel, looping and load spec. The original
`music/menu_GRTEMP.mp3` is then left out. `check-private-assets.py` verifies
the rows and the packaged bytes against the owner file. Without the file the
archive keeps the original music.

The file must be MPEG audio layer III throughout, as the engine reads its
frames before the browser decodes it, and last 1 to 240 seconds. The browser
decodes a stream whole, 384 KB a second at 48 kHz in stereo, while the menu
is open. Native builds play no streams, so they have no menu music either way.

### Owner countdown music

An optional `prematchmusic.mp3` beside the original IWDs plays through the
countdown before a match. The client starts it when a player spawns into the
countdown, from the beginning, keeps it at full volume until 2 seconds after
the match begins and fades it out over the next 3, so it is gone 5 seconds
after the start. Like the menu music it belongs to the owner, is never
committed and needs a byte-identical copy on every host that prepares the
published build (`data/main/` and `~/cod2-original/main/`).

The game has no countdown music to replace, so the music streams as an alias
of its own, `browser_prematch_music` in `soundaliases/browser_prematchmusic.csv`,
on a copy of the retail row of El Daba's opening music: volume 0.8, the
`music` channel, volume group and speaker map, streamed, now loaded in every
multiplayer map (`all_mp`) and without its half-second start delay. The
generator stores the first 65 seconds of the file's frames as
`sound/browser/prematchmusic.mp3` in `cod2_browser_audio.iwd`, the longest
countdown a room holds and the 5 seconds after it, and leaves out its ID3
tags and an encoder's Xing, Info or VBRI frame, which would count frames no
longer there. The browser then decodes about 25 MB in place of the whole
song. `check-private-assets.py` verifies the row and that the packaged bytes
are what the generator makes of the owner file. Without the file the
countdown is silent.

The file must be MPEG audio layer III throughout and last at least a second.
On October 19, 2026 the owner's file was 3:12 of 320 kbps stereo at 44.1 kHz
behind a 3.3 MB tag; the archive keeps 2.6 MB of it and grew from 61.5 to
64.0 MB.

### Owner end of match music

An optional `endgamemusic.mp3` beside the original IWDs plays at the end of a
match, from the moment the result is announced. It holds full volume for 21
seconds and fades out over the last 3 of the 24-second intermission, so it is
gone as the next map loads. Like the countdown music it belongs to the owner,
is never committed and needs a byte-identical copy on every host that
prepares the published build (`data/main/` and `~/cod2-original/main/`).

It streams the same way as an alias of its own, `browser_endgame_music` in
`soundaliases/browser_endgamemusic.csv`, on the same copy of El Daba's opening
music row. The generator keeps the first 28 seconds of the file's frames as
`sound/browser/endgamemusic.mp3`, without tags or an encoder's header frame,
and `check-private-assets.py` verifies the row and the bytes. Without the file
the end of a match is silent. On October 19, 2026 the owner's file was 11.9 MB;
the archive keeps 1.1 MB of it.

### Owner last minute music

When the match clock shows a minute left, music plays from 30 seconds into
the owner's track. It fades in over 4 seconds and stops as the match ends, giving
way at once to the end of match music, or fading out over 2 seconds without
it. The track is an optional `lastminutemusic.mp3` beside the original IWDs,
or else the countdown's `prematchmusic.mp3`. Like the other music it belongs
to the owner, is never committed and needs a byte-identical copy on every
host that prepares the published build (`data/main/` and
`~/cod2-original/main/`). Adding `lastminutemusic.mp3` on one host but not
the other stops the Pi's updates.

The client follows the stock clock, the one the score panel shows. It plays
the music once per clock, after the countdown, which holds the clock. A clock
that moves by more than a second belongs to another match: the music fades
out and waits for that match's last minute. Search and Destroy has none,
because its clock is a round's. If the countdown's music is still fading on
the music channel, the last minute's music starts once that channel is free.

The music streams as `browser_lastminute_music` in
`soundaliases/browser_lastminutemusic.csv`, on the same copy of El Daba's
opening music row. The generator stores 65 seconds of the track's frames,
from the first frame 30 seconds in, as `sound/browser/lastminutemusic.mp3`,
without tags or an encoder's header frame. It cuts at a frame boundary, so the
music enters within 26 ms of the 30-second mark, and the fade-in covers the
first frames, which reach back into frames left out. `check-private-assets.py`
verifies the row and the bytes. On October 20, 2026 the owner's countdown
track served for both. The audio archive grew from 64.8 to 67.4 MB.

### Owner sound pack

An optional `owner_sounds/` folder beside the original IWDs replaces the
sounds of any original aliases it lists. Like the hit sound and the music it
belongs to the owner: the WAVs, and the list of which original alias each one
replaces, are never committed, and every host that prepares the published build
needs a byte-identical copy (`data/main/owner_sounds/` and
`~/cod2-original/main/owner_sounds/`). Changing a file or the list changes the
asset version.

The folder holds 16-bit PCM WAVs, mono or stereo, 11025–48000 Hz and at most
20 seconds long, in lower-case subfolders, and `aliases.csv`: an `alias` and a
`file` column, then any other alias column, such as `pitch_min` or `vol_max`,
with an empty cell keeping the original value. Volumes stay within the alias
format's 0–1; the engine clamps a volume to 1 again after its volume mod group.

For each listed alias the generator drops its original rows and writes one row
per listed file to `soundaliases/browser_owner.csv`, taking the alias's own rows
in turn for every column the list does not set: channel, distances, falloff,
start delay, secondary alias and load spec. Several rows of an alias are
numbered 1, 2, 3 in `sequence`, because the engine drops rows of an alias that
share a sequence number; it then picks among them at random without repeating
the last. The files go to `sound/browser/owner/` in `cod2_browser_audio.iwd`,
each once, and original files that only the replaced rows played are left out.
The generator refuses an alias the four maps never play, a column the alias
table lacks, a missing or malformed file, and an alias the hit sound or menu
music already replaces. `check-private-assets.py` verifies the rows and that
the packaged files are the owner's bytes. Without the folder the archive keeps
the original sounds.

On October 18, 2026 the owner's pack of 213 files replaced 215 aliases: first-
and third-person fire, reloads and bolt cycles of every weapon, the mounted
MG42 and .30 cal loops and cooldowns, grenade pins, throws, bounces and
explosions, smoke, melee, weapon handling, mines and the radio and flak
explosions. The audio archive grew from 46.1 to 57.0 MB, and the hunk in use
after joining by 13.2 MB on Toujane and Carentan: on Carentan, the largest map,
to 97.8 MB of the 134.2 MB (128 MiB) hunk. In 60-second TDM rounds with ten bots
on both maps the console reported no sound alias or load errors, 39 and 30 of
the owner's files played, and the fire aliases heard alternated between their
variants.

### Owner class pictures

An optional `owner_class_pictures/` folder beside the original IWDs holds
pictures for the class screens: Create a Class, its editor and lists, and the
class choice in a match. A picture shows its weapon in its own colors, as
CoD WWII's menus show equipment, in place of the white silhouette of the HUD
icon (or a primary's retail menu image). The HUD, the kill feed and the ammo
counter keep their white icons. Like the sounds, the pictures belong to the
owner, are never committed, and every host that prepares the published build
needs a byte-identical copy (`data/main/owner_class_pictures/` and
`~/cod2-original/main/owner_class_pictures/`). Changing a picture changes the
asset version.

Each file is named after a class weapon in `downstream/gateway/classes.json`,
in any case: `frag_grenade_american_mp.png`, `TT30_mp.png`; attachments' files
are described below. It must be an 8-bit
RGBA PNG without interlacing, each side a power of two up to 1024 texels, with
something visible (alpha above 16). The screens crop it to its visible texels
and fit it to each tile, so the margins around the object do not matter.
Folders, other names and other files are refused; names starting with a dot,
such as `.DS_Store`, are skipped. Weapons without a picture keep the stock
image.

The generator stores each picture in `cod2_browser_renderer.iwd` as
`images/browser_class/<weapon>.iwi`, lower case: A8R8G8B8 texels with every mip
down to 1 × 1, each mip's colors averaged by their alpha so transparent texels
do not darken the edges. The client draws it through a copy of `$raw` with
linear filtering between the mips, so a picture stays smooth from the 25-unit
equipment tiles to the 130-unit preview. `check-private-assets.py` verifies
that the packaged pictures are exactly what the generator makes of the owner's
files, and that there are none without them.

A picture is read the first time a screen draws it and kept until the
renderer restarts; with its mips a 1024 × 256 picture takes 1.3 MB of texture
memory.

On October 18, 2026 the owner's folder held a picture for each of the 33 class
weapons. For the grenades, the MK 2 and the smoke canister were cut out of
CoD WWII's Modify Equipment screen, for the American frag and all four smoke
grenades (every smoke grenade is the same canister in this game), and the
Mills bomb, RGD-33 and Stielhandgranate were rendered from this game's own
world models. The 21 primaries (1024 × 256, 512 × 256 for the MP40) and the
four pistols (512 × 512) were rendered from their world models the same way:
muzzle to the left as the retail menu images have it, turned 15 degrees and
seen 12 degrees from above, under the grenades' lights. Together they added
8.19 MB to the renderer archive (270.96 to 279.15 MB), and about 20 seconds
to the generator on the build machine.

On October 19, with the S-Mine 44 a class weapon, the owner's cut-out of it
from the same captures (256 × 512) joined them as `mine_mp.png`, adding
0.16 MB. The C4 keeps its generated icon.

A gun's attachments take two more kinds of file, under the same rules:

- `<weapon>__<layer>.png`, for `extmag`, `suppressor`, `reflex`, `lens` or
  `optic4x`: the gun on its own picture's canvas with its longer magazine, or
  that attachment alone where it sits on the gun, so the screens lay it over
  the gun picture texel for texel. `scripts/render-class-pictures.py` makes
  them with the gun, from the private archives (see
  [docs/CLASSES.md](CLASSES.md#pictures)).
- `attachment__<attachment>.png`, for one of the attachments in
  `classes.json`: its icon in Modify Attachment and the editor's boxes.

Later the same day the 25 guns were rendered again from their first-person
guns with the attachment models, with 59 layers, and 12 icons were cut out of
CoD WWII's Modify Attachment captures, with High Caliber's rendered as a
cartridge: 106 pictures, 9.73 MB in the renderer archive.

Later still, with the attachments built after CoD WWII's (see
[docs/CLASSES.md](CLASSES.md#on-the-gun)), the guns and their layers were
rendered again, and the seven bolt-action rifles and the M1 Garand took an
`__extmag` layer each for their box magazine: 114 pictures, 12.48 MB in a
285.63 MB renderer archive. Hosts need the new folder before they take that
build.

## Version checks

`/build-info.json` identifies the source revision, dirty status and build-content
ID. `GET /version` reports the installed build, with repository notices disabled.
Host release checks run on the Pi, independently of player requests. Missing
build metadata reports `unknown` rather than claiming the browser is current.

Older deployments without this feature need one manual update before browser
notices become available.
