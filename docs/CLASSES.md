# Custom classes

Players build up to five named classes in the browser and pick one in any
match, on either team. A class sets the primary weapon, the secondary pistol,
the lethal grenade (frag) and the tactical grenade (smoke): CoD2's two offhand
slots. The screens follow the Create a Class flow of the Call of Duty: WWII
capture the owner supplied, drawn with CoD2's own images and the Barlow fonts
the menus already use, or with [pictures the owner keeps](#pictures).

## Using classes

- **Main menu → Create a Class** lists the five classes with the focused
  class's loadout at the right. Enter (or a click) opens **Modify** for that
  class; F1 renames it.
- **Modify** shows the four slots as tiles in two columns, primary over
  secondary and lethal over tactical. The arrows move in that grid; Enter opens
  the slot's list. **Equip Weapon** lists the slot's weapons from the left edge
  with a page counter, the weapon's picture, kind and country, its retail
  description where the game has one, and five rating bars. **Modify
  Equipment** shows the tactical and lethal rows; up and down switch rows.
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
four frags and four smokes. Every new gametag starts with these classes:

| Class | Primary | Secondary | Lethal | Tactical |
| --- | --- | --- | --- | --- |
| Assault | MP44 | Luger | German frag | German smoke |
| Submachine Gunner | Thompson | Colt .45 | American frag | American smoke |
| Rifleman | M1 Garand | Colt .45 | American frag | American smoke |
| Sniper | Scoped Kar98k | Luger | German frag | German smoke |
| Close Quarters | M1897 Trench Gun | Webley | British frag | British smoke |

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

## Server rules

Every room loads `browser_classes.gsc` before the level finishes initializing:

- It precaches the 33 class weapons and the class menu.
- It points both teams' weapon menus at the class menu.
- It wraps the gametype's weapon callback. A class response names one weapon
  for each slot from the lists, or it is refused and the menu reopens.
- The primary still goes through the gametype's own callback, so `scr_allow_*`
  restrictions, the respawn message and the S&D round rules behave as in
  retail.
- A bare weapon name, as the retail menus and bots send, keeps the gametype's
  loadout.

At each spawn the class gives:

- the primary and the pistol, both with full ammunition;
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
- rating bars at the bottom right;
- pill-shaped key prompts.

The repository holds no WWII logos, artwork, fonts, character or weapon
renders. Nor do the screens add things this game lacks: divisions, basic
training, attachments, weapon levels, unlocks, locks, the store or its currency.
The pictures are CoD2's weapon menu images for primaries and silhouettes of its
HUD icons for pistols and grenades, unless the owner keeps their own.

## Pictures

The owner may keep a PNG for any class weapon in `data/main/owner_class_pictures/`,
named after the weapon (`frag_grenade_american_mp.png`); the format and
packaging are in [docs/UPDATES.md](UPDATES.md#owner-class-pictures). Every class
screen then draws that weapon in the picture's own colors: the list's loadout,
the editor's tiles, the equipment rows and preview, and the weapon list. Each
picture is cropped to its visible texels and fit to the box. Weapons without
one keep the stock image, tinted as before. Nothing outside the class screens
changes: the HUD's offhand icons, the kill feed and the ammo counter stay
white.

The development host's pictures cover all 33 class weapons:

| Weapons | Picture |
| --- | --- |
| American frag (M2) | CoD WWII's MK 2, cut out of the owner's capture |
| All four smoke grenades | CoD WWII's smoke canister, cut out of the owner's capture; in CoD2 every smoke grenade is the American canister |
| British frag (MK1), RGD-33, Stielhandgranate | Rendered from CoD2's world model of each, lit like the captures |
| The 21 primaries and four pistols | Rendered from their world models the same way, muzzle to the left as the retail menu images have it |

The owner's other captures (the Signal Flare, Mk V gas grenade, No. 69,
throwing knife, satchel charge, No. 74 ST and S-Mine 44) show equipment this
game does not have. Their cut-outs wait in `data/wwii_equipment_pictures/`,
on canvases the generator accepts, for the day a class weapon matches one.

## Limits

- Gametags are shared, as described above.
- The server keeps a player's class while they stay connected. The menu
  highlights the last choice until the page reloads.
- The Grease Gun, PPS-42, SVT-40, G43 and Trench Gun have no retail menu
  description, so their lists show only the kind and the country.
- A script `notify("x", a, b)` reaches `waittill("x", p, q)` with the arguments
  reversed (`p` is `b`). Menu responses from real clients are engine notifies
  and arrive in order, so classes are unaffected. The retail `scr_testclients`
  helper in `_teams.gsc` sends menu responses with script notifies and is
  affected. The VM is not changed here; the runtime test calls the gametype's
  weapon callback directly, as the retail menu handler does.

## Tests

- `python3 scripts/test-class-data.py` compiles the client's class data with
  sanitizers. It covers the weapon table, the storage lines with per-field
  defaults, the name rules and renaming, the menu response and the ratings, and
  rejects 14 mutations. It also checks that `classes.json`, the C table and the
  GSC lists agree.
- `python3 scripts/test-owner-class-pictures.py` covers the pictures: the PNGs
  the generator accepts and refuses, the IWis it writes against a reference of
  every mip, the folder's names, and the checker. Under ASan/UBSan it loads the
  generator's files through the engine's loader against a recording backend,
  and runs the menus' choice between a picture, a retail image and a silhouette.
  It rejects 30 Python, 17 loader and 7 menu mutations.
- `node scripts/test-web-classes.cjs` runs the linked browser bridge. It covers
  the requests it sends and what it does with each answer: stored lines, 400 and
  503 refusals, a 200 without classes, an outage, oversized replies, ordered
  saves. The bodies it saves pass the gateway's own validation. It rejects 10
  mutations.
- `(cd downstream/gateway && npm test)` covers the store across reopening and
  the HTTP API: lookups, saves, cross-origin, invalid and oversized bodies,
  wrong methods, missing storage.
- `python3 scripts/test-room-maps.py` checks that every room receives the script.
- `python3 scripts/test-browser-classes-runtime.py --assets PREPARED_MAIN` runs
  the dedicated server with a bot and no network. It makes eight class choices:
  - each team carrying other countries' weapons;
  - an invalid class, refused;
  - bare weapon names keeping the gametype's loadout;
  - an alive change waiting for the spawn and applying at the respawn;
  - a restricted primary keeping the previous weapon.

  Removing the precache, the spawn hook, the validation, the restriction guard
  or the menu override makes it fail. Toujane and Carentan both passed.
  `--gametype sd` checks the class at an S&D spawn, then a class chosen before
  the match. That class applies whole, without the team's pistol and frags, and
  removing the in-place step fails it.
- In the browser, against an isolated stack:
  - Every screen was created, edited and renamed. The edits persisted across a
    reload, and the class list, the editor and back all worked with the mouse.
  - In a match, a British player spawned with a Kar98k and switched to a TT-30.
    The in-game menu showed "1. Choose Class", and choosing while alive gave the
    respawn message. A German player spawned with an M1 Garand after changing
    team.
