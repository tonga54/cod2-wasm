# Custom classes

Players build up to five named classes in the browser and pick one in any
match, on either team. A class sets the primary weapon and up to three of its
attachments, the secondary pistol and one of its attachments, the lethal
grenade (frag) and the tactical grenade (smoke): CoD2's two offhand slots. The
screens follow the Create a Class flow of the Call of Duty: WWII capture the
owner supplied, drawn with CoD2's own images and the Barlow fonts the menus
already use.

## Using classes

- **Main menu → Create a Class** lists the five classes with the focused
  class's loadout at the right. Enter (or a click) opens **Modify** for that
  class; F1 renames it.
- **Modify** shows the four slots as tiles in two columns, primary over
  secondary and lethal over tactical, with three attachment boxes under the
  primary and one under the secondary. The arrows move between them as they lie;
  Enter opens the slot's list. **Equip Weapon** lists the slot's weapons from
  the left edge with a page counter, the weapon's picture, kind and country, its
  retail description where the game has one, and five rating bars. A different
  gun starts without attachments. **Modify Equipment** shows the tactical and
  lethal rows; up and down switch rows.
- An attachment box opens **Modify Attachment**: WWII's grid of the gun's
  attachments, three a row, with the focused one's description, a line for each
  value it changes on that gun (green better, red worse) and the rating bars
  with its change. Enter fits it in that box; Backspace, Delete or a
  controller's X takes the box's attachment off. An attachment already on the
  gun, or a second optic, is greyed with the reason in yellow.
- Names are 1 to 20 printable ASCII characters without `"`, `\`, `;`, `%` or
  `^`. Leading spaces cannot be typed, trailing spaces are removed.
- Escape or the right mouse button goes back one screen.
- **In a match**, the class list replaces the retail weapon menu after the team
  choice and behind **Esc → 1. Choose Class**. Enter, a click or the keys 1 to
  5 choose a class; F1 edits it there. A choice made while alive takes effect at
  the next spawn, with the retail "You will respawn with..." message.
- Any team may carry any country's weapons: a British player can spawn with a
  Kar98k and a TT-30, a German one with an M1 Garand.

The 21 primaries are those of the four factions' retail weapon menus. The
secondaries are the Colt .45, Webley, TT-30 and Luger, and the grenades are the
four frags and four smokes. Every new gametag starts with these classes; the
primaries' attachments are the Prima guide's suggestions for their WWII
counterparts:

| Class | Primary | Attachments | Secondary | Lethal | Tactical |
| --- | --- | --- | --- | --- | --- |
| Assault | MP44 | Grip, Extended Mag | Luger | German frag | German smoke |
| Submachine Gunner | Thompson | Steady Aim, Extended Mag | Colt .45 | American frag | American smoke |
| Rifleman | M1 Garand | Quickdraw, Extended Mag | Colt .45 | American frag | American smoke |
| Sniper | Scoped Kar98k | FMJ, Extended Mag | Luger | German frag | German smoke |
| Close Quarters | M1897 Trench Gun | Steady Aim, Rapid Fire | Webley | British frag | British smoke |

## Attachments

The rules are Call of Duty: WWII's since its April 2018 Divisions update: three
attachments on the primary and one on the pistol, one optic at a time, and each
gun the list of its WWII class:

| CoD2 weapons | WWII class | Attachments |
| --- | --- | --- |
| Thompson, Grease Gun, Sten, PPS-42, PPSh, MP40 | SMG | Reflex Sight, Lens Sight, Quickdraw, Grip, Steady Aim, Rapid Fire, Extended Mag, FMJ, Advanced Rifling, Suppressor |
| M1 Garand, M1A1 Carbine, SVT-40, G43, MP44, BAR | Rifle | Reflex Sight, Lens Sight, 4x Optic, Quickdraw, Grip, Steady Aim, Rapid Fire, Extended Mag, FMJ, High Caliber, Advanced Rifling |
| Bren | LMG | The rifle list without the Lens Sight |
| Springfield; scoped Lee-Enfield, Mosin-Nagant and Kar98k | Sniper | 4x Optic, Ballistic Calibration, Rapid Fire, Extended Mag, FMJ |
| Lee-Enfield, Mosin-Nagant, Kar98k | Sniper on its Iron Sight | The sniper list |
| M1897 Trench Gun | Shotgun | Reflex Sight, Quickdraw, Steady Aim, Rapid Fire, Extended Mag, Advanced Rifling |
| Colt .45, TT-30, Luger | Pistol | Quickdraw, Steady Aim, Extended Mag, FMJ, High Caliber, Advanced Rifling, Suppressor |
| Webley | Revolver | The pistol list without the Extended Mag |

WWII's Iron Sight is not offered: CoD2 already has each bolt-action rifle with
and without its scope. The Bayonet, Rifle Grenade, Incendiary Rounds, Rifle
Bullet and Tactical Knife need models or mechanics CoD2 lacks.

Each attachment changes the weapon file's values for its player only. The
numbers are WWII's where they are published, otherwise this port's:

| Attachment | Effect | Source |
| --- | --- | --- |
| Extended Mag | Magazine and reserve +50%, rounded down | WWII |
| Rapid Fire | Fire and rechamber times for +6.38% rate; +9.89% on the Bren, +5.4% on bolt-actions, +42% on the Trench Gun | WWII's RPM tables |
| Quickdraw | Aim-in time −60% on SMGs, −40% on pistols, −20% on the others | Community measurements; pistols ours |
| Steady Aim | Hip-fire spread −55%, −35% on the Thompson and PPSh | WWII (M1928, PPSh-41) |
| Grip | Aimed view and gun kick −35% | Ours |
| Advanced Rifling | Damage ranges ×2 on rifles, ×1.3 on SMGs, ×1.25 on the BAR, Bren and pistols, ×1.13 on the Trench Gun | WWII |
| Suppressor | Damage ranges −30% on SMGs, −15% on pistols; quiet, no flash, off the minimap | WWII |
| High Caliber | Helmet and head multipliers ×1.5 | WWII |
| FMJ | Penetrates as armor-piercing: 25% more depth, so less damage lost, through cover | Ours, on the existing penetration |
| Ballistic Calibration | Aimed idle sway and sway −50% | Ours |
| Reflex Sight | View −10% (more zoom), aimed spread −25%, a red dot over the sights | Ours |
| Lens Sight | View −20%, aimed spread −15%, an amber ring over the sights | Ours |
| 4x Optic | A 23.7° view, four times an 80° one; an iron-sighted gun takes its nation's sniper scope | Ours; on a sniper rifle, WWII's lower-zoom scope |

A suppressed shot has no muzzle flash and does not show on the enemy minimap.
CoD2 has no suppressed fire sounds, so it plays the weapon's own at 35% of its
volume, heard from a third of the distance, without the distant report. A 4x
Optic brings the sniper scope's rules: the gun hides at full zoom, the breath can
be held and shots follow the scope's view. Two attachments that change the same
value multiply: a Suppressor on Advanced Rifling leaves an SMG 9% short of its
range, as in WWII.

Sources: [Prima's WWII guide](https://primagames.com/eguides/call-of-duty-wwii/multiplayer-introduction/firing-range/primary-weapons)
for the lists and descriptions, the
[Divisions update](https://www.callofduty.com/blog/archives/the-call-of-duty-wwii-division-overhaul---six-ways-to-renew-your-loadout)
for the limits and suppressors, the Call of Duty Wiki's
[Rapid Fire](https://callofduty.fandom.com/wiki/Rapid_Fire),
[Extended Mags](https://callofduty.fandom.com/wiki/Extended_Mags) and
[Silencer](https://callofduty.fandom.com/wiki/Silencer) pages, the
[WWII wiki's attachment page](https://w.atwiki.jp/cod_wwiiwiki/pages/35.html)
for Advanced Rifling and High Caliber, and
[a hip-fire measurement](https://game-hashirigaki.site/fps/cod-ww2-steady/) for
Steady Aim.

### How they reach the game

- One table, `src/PC/bgame/bg_attachment_rules.c`, holds the lists, limits and
  effects for the menus and the game; `classes.json` repeats the lists and
  limits for the gateway.
- A class's response adds two fields: `class,primary,secondary,lethal,tactical,
  primary attachments,secondary attachments`, each `none` or names joined by
  `+`. A client from before attachments still sends five fields and gets none.
  `browser_classes.gsc` refuses a set with `validWeaponAttachments(weapon, list,
  slot)` and fits it at spawn with `setWeaponSlotAttachments(slot, list)`, before
  `giveMaxAmmo`, so an Extended Mag starts full.
- The set travels in the player state's `weaponslots[4..7]`, two bytes a slot
  that multiplayer never used and the snapshot already sent: no protocol change.
  Movement prediction and the server read the same set. A slot drops its
  attachments with its weapon, so a weapon picked up has none.
- `BG_GetPlayerWeaponDef` gives a copy of the weapon's definition with the
  player's attachments, kept in a cache of 128 that empties when the weapons
  reload. Reloads, fire timing, aiming, spread, recoil, sway, zoom, the scope,
  ammunition limits, damage range, penetration and head damage read it.
- Player movement marks a suppressed shot in its fire event's parameter, which
  every client already receives.

## Storage

The gateway keeps the classes in SQLite at `/data/classes.sqlite`, in the
`cod2-classes` Docker volume, with one row for each gametag and slot. The key is
the gametag without color codes, trimmed and lowercased.

There are no accounts: whoever uses a gametag reads and changes its classes.
Every new browser is called "Player", so "Player" and "Unknown Soldier" cannot
save. The screen asks for a gametag in Options, and changes made under those
names last until the player leaves. When storage is unreachable, the screen
says so and changes also last until the player leaves.

`GET /classes?player=NAME` returns `{classes, saved}`: the stored classes, or the
defaults with `saved: false`. `PUT /classes` takes `{player, classes}` with
exactly five classes. It accepts only same-origin requests with bodies of at most
4 KiB. The gateway, the client and the server check the same lists, from
`downstream/gateway/classes.json`. The client sends one save at a time, in
order.

Each class carries `primaryAttachments` and `secondaryAttachments`, arrays of
attachment names; a `PUT` without them saves none. The rows keep them in
`primary_attachments` and `secondary_attachments`, names joined by `+`. A store
created before attachments gains the two columns, empty, when the gateway
opens it, so its classes have none. Stored attachments a class's gun no longer
takes are dropped and the rest of the row kept.

## Server rules

Every room loads `browser_classes.gsc` before the level finishes initializing:

- It precaches the 33 class weapons and the class menu.
- It points both teams' weapon menus at the class menu.
- It wraps the gametype's weapon callback. A class response names one weapon
  for each slot from the lists and attachments its guns take, or it is refused
  and the menu reopens.
- The primary still goes through the gametype's own callback, so `scr_allow_*`
  restrictions, the respawn message and the S&D round rules behave as in
  retail.
- A bare weapon name, as the retail menus and bots send, keeps the gametype's
  loadout.

At each spawn the class gives:

- the primary and the pistol with their attachments, both with full
  ammunition, an Extended Mag's included;
- one smoke grenade, when smoke is allowed;
- the frag count retail gives for that primary (`getWeaponBasedGrenadeCount`),
  when frags are allowed;
- binoculars.

When S&D swaps weapons in place before a round, the whole class applies at
once.

## Ratings

The bars compare the 25 guns, 21 primaries and 4 pistols. Each value comes from
the weapon's file and is rated 1 to 10 between the lowest and highest:

| Bar | Weapon file value |
| --- | --- |
| Damage | `damage` |
| Range | `maxDamageRange` |
| Accuracy | `1 / (0.25 + adsSpread)`, times 1.2 with an `adsOverlayShader` scope |
| Fire Rate | `1 / fireTime` |
| Mobility | `moveSpeedScale` |
| Capacity | `clipSize` / `maxAmmo` (shown as numbers) |

These are comparisons, not retail statistics. The lethal note uses the frag's
`fuseTime`, 3.5 seconds: as elsewhere in this port, a frag cooks in the hand
from the moment the pin comes out. Smoke grenades do not cook.

With attachments, the same values come from the attached definition, Fire Rate
scaled by the engine's whole-millisecond fire time. The bars stay continuous
before rounding, so a small change such as Rapid Fire's 6% still shows as a
sliver of green; the number changes color only when its rating does. Grip,
Steady Aim, Quickdraw and Ballistic Calibration change no bar: their lines say
what they change.

## Download and runtime cost

The Soviet weapons now ship with every map. They add 3.87 MB to the renderer
archive and 1.60 MB to the audio archive. The four prepared archives total
329.6 MB, against 324.1 MB before.

Toujane TDM was measured in Chrome at 1280×800, two runs each with the same
build, against a room whose class script was empty:

| | Empty script | Classes |
| --- | --- | --- |
| Map load (room connect to the last script menu) | 2.31–2.32 s | 2.38–2.43 s |
| Texture bytes uploaded by the end of the load | 105.3 MB | 112.2 MB |
| Frame rate while playing | 125.8–126.4 FPS | 126.5–127.2 FPS |
| Server container memory | 239–240 MiB | 239–244 MiB |

WebAssembly memory stayed at 512 MB in both.

## What was not copied

From the WWII capture, the screens take the layout and flow:

- the gold spaced title;
- framed list rows and the side panel's `» LOADOUT` heading;
- captioned tiles;
- the left-edge weapon list with its page counter;
- the attachment grid, three tiles a row, and its yellow notes;
- rating bars at the bottom right, green and red for an attachment's change;
- pill-shaped key prompts.

They do not use WWII logos, artwork, fonts, character or weapon renders: the
attachment tiles show their names, not WWII's icons. Nor do they add things this
game lacks: divisions, basic training, weapon levels, unlocks, locks, the store
or its currency. The pictures are CoD2's weapon menu images for primaries and
silhouettes of its HUD icons for pistols and grenades.

## Limits

- Gametags are shared, as described above.
- The server keeps a player's class while they stay connected. The menu
  highlights the last choice until the page reloads.
- The Grease Gun, PPS-42, SVT-40, G43 and Trench Gun have no retail menu
  description, so their lists show only the kind and the country.
- CoD2 has no attachment models: a gun looks the same with any of them. The
  Reflex and Lens Sights are reticles drawn over the iron sights, and the 4x
  Optic borrows a sniper rifle's scope image.
- A suppressed shot is the gun's own sound made quieter, not a suppressed
  recording; the Suppressor cannot be taken off in a match as WWII's D-pad does.
- Attachments belong to the class's guns. A weapon picked up from the ground,
  even one a suppressed player dropped, has none. Bots use none.
- A script `notify("x", a, b)` reaches `waittill("x", p, q)` with the arguments
  reversed (`p` is `b`). Menu responses from real clients are engine notifies
  and arrive in order, so classes are unaffected. The retail `scr_testclients`
  helper in `_teams.gsc` sends menu responses with script notifies and is
  affected. The VM is not changed here; the runtime test calls the gametype's
  weapon callback directly, as the retail menu handler does.

## Tests

- `python3 scripts/test-class-data.py` compiles the client's class data with
  sanitizers. It covers the weapon table, the storage lines with per-field
  defaults and older lines without attachments, the name rules and renaming,
  fitting and removing attachments, the menu response, the ratings and the
  effects the attachment menu lists, and rejects 26 mutations. It also checks
  that `classes.json`, the C table and the GSC lists agree, attachments
  included.
- `python3 scripts/test-attachments.py` compiles the attachment rules, the
  attachment masks in the player state and the production weapon code with
  sanitizers. It covers every weapon's list, the limits and the one-optic rule,
  each attachment's numbers, the cached player definitions, and reload, spread,
  fire rate, aim time, recoil and the suppressed fire event. It rejects 24
  mutations.
- `python3 scripts/test-suppressed-shots.py` runs the client's `CG_FireWeapon`
  on 120 fire events. Only a player's shot marked suppressed loses its flash
  and minimap ping and plays the quiet copy of its sound, one copy per sound.
  It rejects 12 mutations.
- `python3 scripts/test-attachment-sight.py` draws the Reflex and Lens Sight
  reticles: when they show, their fade, placement, radii, feathering and
  meshes. It rejects 10 mutations.
- `node scripts/test-web-classes.cjs` runs the linked browser bridge. It covers
  the requests it sends and what it does with each answer: stored lines with
  their attachments, 400 and 503 refusals, a 200 without classes, an outage,
  oversized replies, ordered saves. The bodies it saves pass the gateway's own
  validation. It rejects 13 mutations.
- `(cd downstream/gateway && npm test)` covers the store across reopening, the
  attachment columns added to an older database, and the HTTP API: lookups,
  saves, attachments the rules refuse or drop, cross-origin, invalid and
  oversized bodies, wrong methods, missing storage.
- `python3 scripts/test-room-maps.py` checks that every room receives the script.
- `python3 scripts/test-browser-classes-runtime.py --assets PREPARED_MAIN` runs
  the dedicated server with a bot and no network. It makes 14 checks:
  - each team carrying other countries' weapons;
  - attachments fitted at spawn with full magazines (a PPSh with an Extended
    Mag holds 106 rounds and 319 in reserve), and older answers carrying none;
  - an invalid class and invalid attachment sets, refused;
  - the engine's own validation and fitting;
  - bare weapon names keeping the gametype's loadout;
  - an alive change waiting for the spawn and applying, attachments included,
    at the respawn;
  - a weapon given again starting without attachments;
  - a restricted primary keeping the previous weapon.

  Removing the precache, the spawn hook, the validation, the restriction guard,
  the menu override or the attachment fitting makes it fail. Toujane and
  Carentan both passed. `--gametype sd` checks the class at an S&D spawn, then a
  class chosen before the match. That class applies whole, with its Extended
  Mag and without the team's pistol and frags, and removing the in-place step
  fails it.
- In the browser, against an isolated stack:
  - Every screen was created, edited and renamed. The edits persisted across a
    reload, and the class list, the editor and back all worked with the mouse.
  - In a match, a British player spawned with a Kar98k and switched to a TT-30.
    The in-game menu showed "1. Choose Class", and choosing while alive gave the
    respawn message. A German player spawned with an M1 Garand after changing
    team.
  - The class list showed each gun's attachment count and the editor its
    boxes. Modify Attachment fitted, replaced and removed attachments, listed
    each one's effects against the stats, and refused a second optic with its
    note. The gateway kept the choices.
  - In a match the MP44 with an Extended Mag showed 45 | 270, and its 4x Optic
    aimed through the German scope with the Hold Breath prompt. The suppressed
    shot and the Reflex and Lens reticles were not checked on screen; the tests
    above cover them.
