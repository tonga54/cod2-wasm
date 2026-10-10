# Updating the game

The public repository is [tonga54/cod2-wasm](https://github.com/tonga54/cod2-wasm).
Update checks follow `master`. Pushing another branch does not announce a game
update.

## Browser players

The game checks the host version once a minute while the tab is visible. After
the host installs a new build, **Reload** appears in the main menu, once the
player has left the match. Opening pause or the console does not trigger it.
The page never reloads automatically; players can choose **Later**.

If GitHub has changes the host has not installed, a separate notice links to
this guide. Reloading a browser does not install server code.

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

The installer requires `origin` to point to this repository, branch `master`,
a clean working tree, and a fast-forward update. It does not reset, stash or
merge local history. It checks for active players before changing files and
before restarting. It preserves `data/main/`, regenerates the private subset,
builds the browser/native server/gateway, then runs `docker compose up -d`.

A failed build does not restart existing containers. Fix the failure and retry;
`--apply` can rebuild an already current checkout. Keep a backup of your original
files. Requirements and the pinned framework are listed in the [README](../README.md).

## Raspberry Pi hosts

Use the [Pi deployment procedure](RASPBERRY_PI.md) from the machine that builds
the game. It transfers the prepared ARM64 web/gateway images, the private subset
and the x86 engine runtime, then builds the small ARM64 supervisor/QEMU image.
The Pi uses `compose.pi.yaml` alongside the standard Compose file. The standard
`update-local.py --apply` procedure does not build this Pi runtime.

## Asset delivery and caching

Prepare the owner's files on the build machine:

```sh
python3 scripts/prepare-browser-bootstrap.py
```

The current Carentan/Toujane subset is approximately **275.6 MB**. Each browser
downloads it from the host over HTTP/HTTPS and stores it in IndexedDB. The
manifest verifies archive sizes and SHA-256 hashes; a new asset version
invalidates the old cache. Storage belongs to each browser and origin, so
moving from a Mac URL to a Pi URL requires a fresh initial download.

Players can join an already prepared host without individually importing a ZIP.
A person setting up another host must supply their original CoD2 IWD files in
`data/main/`. These files are not supplied by a Git clone, public image or Release.
Original assets remain private, read-only mounts; the repository contains their
public manifest hashes rather than the archives.

## Version checks

`/build-info.json` identifies the source revision, dirty status and build-content
ID. `GET /version` reports the installed build and the GitHub comparison. The
gateway shares results across players and checks GitHub at most once every five
minutes per revision, without requiring a token. Network/rate-limit failures
report `unknown`. Ahead/divergent histories are not announced as normal updates.

Older deployments without this feature need one manual update before browser
notices become available.
