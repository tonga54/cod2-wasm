# Call of Duty 2 in the browser

Work in progress: the reconstructed engine targets WebAssembly and WebGL 2.
Each browser executes its own client. The previous Wine/noVNC runtime is stopped.

The supported maps are Toujane, Tunisia (`mp_toujane`) and Carentan, France
(`mp_carentan`), with TDM and up to 64 human clients per server on a LAN.
Rounds end at 100 team points or 15 minutes. The main room rotates between
the two maps; Start New Server uses the map selected in the original menu.
Toujane uses British/Afrika Korps teams and Carentan uses American/German
Normandy teams. Other retail modes, including CTF, are not packaged/enabled.
Two real browser clients have connected, chosen opposing teams, moved, aimed,
fired, caused damage, killed each other and respawned with synchronized scores.
The complete combat check also passed in the normal room after walking from
the original spawns, with both sides scoring and respawning at 100 health. Bullet
tracers are visible from the firing client and other clients, with their original
texture colors restored. Wall impacts now draw particles and lasting bullet marks.
Static vehicles, barrels and crates render again; all six Toujane ladder volumes
have been climbed in Chrome. Grenades use the weapon's original release delay.
Browser audio now plays the original menu music, ambience, weapon fire, reloads
and footsteps. Shots from the other player were verified in both directions
between Chrome and the internal browser. The October 8 update fixes clock drift,
smoke alpha, purple model colors, command aim recoil and voted map reloads.
Two browsers completed the vote/reload/resume flow; archive ring recovery and
movement clock regressions pass. Longer matches, other hardware and remaining
visual fidelity still need broader coverage. See RUNBOOK.md for exact evidence.

The real engine has linked with zero undefined symbols. Function signature warnings
are still being repaired. A build does not establish that a match is playable.
The page loads directly into the original main menu without a launcher form or
automatic server connection. The tab title is **Call of Duty 2 Multiplayer**;
its favicon is extracted from the owner's original MP executable.
Startup shows the actual file download percentage and received/total MB,
counts cached files, and displays preparation progress as a separate stage.

A primary click on the game enters fullscreen when the browser supports it;
Escape exits fullscreen. Severe damage now draws the original blood overlay,
with fading pulses below 35% health, controlled by `cg_blood`. Aim and lean
controllers preserve the sampled walk/run pose and bone lengths instead of
replacing them. Regression checks are `scripts/test-animation-controllers.py`,
`scripts/test-low-health-overlay.py` and `scripts/test-input-capture.cjs`.

Join Game discovers the host's live dedicated servers. Start New Server creates
another dedicated TDM instance on the selected map, with 64 player slots. Up to three
instances are allowed; additional instances empty for five minutes are stopped.
The native menu creation/discovery/join flow has been checked with two browser
clients. Sustained gameplay stability and the remaining graphics work
still require verification.

Start with `./scripts/build-docker.sh` and `docker compose up -d`. The local URL
is `http://localhost:8088`; other LAN devices use the Mac's current IP and port
8088. On 2026-10-07 that address changed to `http://192.168.1.10:8088`.

Browser players receive update notices in the main menu after leaving a match.
Hosts can check with `python3 scripts/update-local.py` and explicitly install
with `python3 scripts/update-local.py --apply`. Local changes and active players
block installation. See [the update and asset delivery guide](docs/UPDATES.md).

Original owner assets stay untouched in ignored `data/main/`. The script
`scripts/prepare-browser-bootstrap.py` prepares a private Toujane/Carentan asset subset
(271.7 MB, including 496 original sound files). Archives stay outside the
public site and image. The canonical launcher pins wasm-game-framework 0.9.2
at `53bc7e6eeef1ae35dcf3b25dea4e3ec0ab46726f`.

The private subset applies `downstream/weapon-balance.json` (`lan-balanced-v1`)
to both browser clients and dedicated rooms. It retains retail close body damage,
cadence, recoil, magazines and reloads, reduces automatic headshots from ×3 to
×2, shortens SMG full-damage range, and adds ranged damage loss and hip-fire
dispersion to Bren/MP44. Rifles, scopes, pistols, shotgun, grenades and mounted
MG42 keep their original definitions. These are custom LAN adjustments based
on the owner's CoD2 files, using Activision's WWII class descriptions as design
context rather than importing unverified numeric stats from another game.
Regenerate with `python3 scripts/prepare-browser-bootstrap.py` and verify with
`python3 scripts/test-weapon-balance.py`; the audit is written to
`out/weapon-balance-audit.json`. Rebuild/recreate the web and server images so
the asset manifest and cached server archives stay synchronized.
