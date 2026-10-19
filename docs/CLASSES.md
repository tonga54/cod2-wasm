# Custom classes

Players build up to five named classes in the browser and pick one in any
match, on either team. A class sets the primary weapon, the secondary pistol,
the lethal grenade (frag) and the tactical grenade (smoke): CoD2's two offhand
slots. The screens follow the Create a Class flow of the Call of Duty: WWII
capture the owner supplied, drawn with CoD2's own images and the Barlow fonts
the menus already use.

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
- Under the primary and secondary tiles, a line names each gun's
  camouflage. F2, a controller's Y (WWII's Customize) or a click on the line
  opens **Primary Camouflage** or **Secondary Camouflage**: eleven swatches
  four to a row (the gun's own finish, then the ten patterns below), the
  focused one's name, note and place in the list, and the gun's picture
  wearing it. The arrows move through the grid, Enter or a click equips and
  saves, Escape goes back. Grenades take no camouflage. The class list's
  panel, the Modify tiles and Select Class's Custom tab show the guns painted;
  Standard Issue keeps the game's own finish.
- Names are 1 to 20 printable ASCII characters without `"`, `\`, `;`, `%` or
  `^`. Leading spaces cannot be typed, trailing spaces are removed.
- Escape or the right mouse button goes back one screen.
- **Joining a match** opens **Select Class** before anything else, in place of
  the server information and the team choice. Choosing a class puts the player
  on the team the mode's own auto-assign picks (the smaller team, or on a tie
  the one behind) and spawns them as the mode allows.
- **In a match**, Escape opens the pause menu on its **Class** tab, with the
  same two lists, with or without a team: there a choice without a team
  assigns one too. F1 edits a Custom class and Escape in the editor returns to
  the tab. The pause menu's **Change Team** tab moves between sides.
- **Select Class** has two tabs: **Custom**, the player's five classes, and
  **Standard Issue**, the five classes every gametag starts with, as the game
  gives them. Left and right (or Tab, or a click) switch tabs, up and down move
  through the list; Enter, a click or the keys 1 to 5 choose; F1 edits a Custom
  class. Reopened, it shows the class chosen last. A choice made while alive
  takes effect at the next spawn, with the retail "You will respawn with..."
  message.
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

The default classes keep their guns' own finish.

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
exactly five classes. A class has `name`, `primary`, `secondary`, `lethal`,
`tactical`, `primaryCamo` and `secondaryCamo`; a class sent without the two
camouflage fields keeps `none`. A store from before camouflage gains the
`primary_camo` and `secondary_camo` columns, set to `none`, when the gateway
opens it. It accepts only same-origin requests with bodies of at most
4 KiB. The gateway, the client and the server check the same lists, from
`downstream/gateway/classes.json`. The client sends one save at a time, in
order.

## Server rules

Every room loads `browser_classes.gsc` before the level finishes initializing:

- It precaches the 33 class weapons and the class menu.
- It points both teams' weapon menus at the class menu, and the menu the
  connect callbacks open first for a player without a team, the server
  information, too.
- It wraps the gametype's weapon callback. A class response names one weapon
  for each slot from the lists, or it is refused and the menu reopens. After
  the tactical grenade it may name the two guns' camouflage from the same
  list (`class,primary,secondary,lethal,tactical,primaryCamo,secondaryCamo`);
  the client sends them only when a gun is painted, so a server from before
  camouflage still takes an unpainted class.
- A class chosen without a team first goes through the gametype's own
  auto-assign, which reopens its team's weapon menu; that is closed again and
  the class applies. During the intermission such a choice is ignored.
- Without a team, the bindable Change Weapon key opens the class menu once
  `_menus.gsc` has closed the in-game menu.
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

## Camouflage

Ten patterns, generated in code for this port after the kinds the WWII
capture shows; none of WWII's artwork is used.

| Camouflage | Note |
| --- | --- |
| Frogskin | Olive and khaki spots |
| Brownspot | Sun-faded salmon spots |
| Splinter | Angular shards and rain streaks |
| Oak Leaf | Autumn foliage |
| Pea Pattern | Blotches peppered with dots |
| Plane Tree | Dark forest mottle |
| Desert | North African sand |
| Winter | Snow whitewash |
| Ambush | Spring green and rust |
| Ember | Black with glowing red |

The ids (`frogskin`, `brownspot`, `splinter`, `oakleaf`, `peapattern`,
`planetree`, `desert`, `winter`, `ambush`, `ember`, and `none`) are listed in
`classes.json`, in `bg_weapon_camo.c` and in `browser_classes.gsc`.

How a class's camouflage reaches the guns:

- At each spawn `browser_classes.gsc` calls the player method
  `setWeaponCamos("primary camo secondary camo")`. It sets configstring
  `0x7c0` plus the client number, which every client receives. The method takes
  only spaces, `_`, letters and digits, under 96 characters. The string is
  cleared when a player connects, and when the gametype keeps its own loadout
  (a restricted primary, a bare weapon).
- Each client looks a gun up by weapon name, so a different weapon index on
  another map cannot paint the wrong gun. It paints the first-person gun and
  the gun in every player's hands (not a turret), in killcams and while
  spectating too.
- Only the gun's own material is painted: hands, rounds, magazines, scopes
  and slings stay as they are. Rifles and submachine guns with wooden
  furniture (Kar98k, Gewehr 43, M1 Garand, M1 Carbine, Springfield, Lee-Enfield,
  Mosin-Nagant, SVT-40, Thompson, PPSh-41, Trench Gun and the scoped rifles)
  are painted on their wood, as WWII paints them; the others (MP40, MP44,
  BAR, Bren, Sten, Grease Gun, PPS-42) and the four pistols all over.
- The paint mask is made once per material from its color map, in blocks
  of 4 × 4 texels: warm brown texels for wood, any lit texel for a whole gun,
  cleared of specks. A seamless 256 × 256 pattern repeats 2.5 times over the
  gun's texture. The shader paints the texel before lighting, keeps its grain
  (each texel's brightness against the painted area's median) and halves the
  highlights where it paints.
- The pattern and mask bind on texture units 16 and 17, clear of the scene
  compositor's 8 to 14, which it unbinds every frame.
- The menu pictures are painted on the CPU from the same patterns: a
  primary's weapon menu image, or the pistol's HUD silhouette filled with the
  pattern. Sixteen are kept.

Each pattern in use is one 256 × 256 texture with mipmaps (about 340 KB), and
each painted gun material one mask a quarter of its color map's width and
height.

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

## Select Class

The screen follows the owner's capture of WWII's Select Division at
1024 × 576, 1.2 px a unit, measured with Barlow's metrics:

- the gold title, Bold capitals 11 tall;
- the two tabs, the chosen one filled cream with near-black Bold, the other
  framed;
- the tab's classes in Regular capitals 29.2 apart, the focused one framed in
  gold from the screen's left edge, in gold Bold 8 units further right;
- the focused class's primary, secondary, lethal and tactical, each slot's
  caption and weapon name over its framed tile, in the capture's tile sizes;
  the tiles stand where the capture's Division column began;
- the prompts lower and further in than the main menu's: Enter Select, F1 Edit
  on the Custom tab, Escape Back.

Behind it, as behind WWII's, the match is blurred and gray. While a class
screen of a match paints, it asks the browser's scene compositor for the next
frame's backdrop: the end of a match's blur and a mix of every pixel toward
its Rec. 709 luma, after the other effects. The gray costs no extra pass. A
lighter shade than the other class screens' and the main menu's wash darken
the left of the screen.

## What was not copied

From the WWII capture, the screens take the layout and flow:

- the gold spaced title;
- framed list rows and the side panel's `» LOADOUT` heading;
- captioned tiles;
- the left-edge weapon list with its page counter;
- rating bars at the bottom right;
- pill-shaped key prompts.

They do not use WWII logos, artwork, fonts, character or weapon renders. Nor do
they add things this game lacks: divisions, basic training, attachments, weapon
levels, unlocks, locks, the store or its currency. Select Class leaves out the
capture's Division and Basic Training panels and its four attachment tiles,
and shows keyboard keycaps where the capture shows a controller's buttons. The
pictures are CoD2's weapon menu images for primaries and silhouettes of its HUD
icons for pistols and grenades. The camouflage screen takes the capture's
swatch grid, page counter and painted preview; its patterns are this port's
own, and all ten are available from the start, without WWII's challenges.

## Limits

- Gametags are shared, as described above.
- The server keeps a player's class while they stay connected. The menus
  highlight the last choice until the page reloads.
- The Grease Gun, PPS-42, SVT-40, G43 and Trench Gun have no retail menu
  description, so their lists show only the kind and the country.
- A script `notify("x", a, b)` reaches `waittill("x", p, q)` with the arguments
  reversed (`p` is `b`). Menu responses from real clients are engine notifies
  and arrive in order, so classes are unaffected. The retail `scr_testclients`
  helper in `_teams.gsc` sends menu responses with script notifies and is
  affected. The VM is not changed here; the runtime test calls the gametype's
  weapon callback directly, as the retail menu handler does.
- Guns lying on the ground and mounted guns are not painted.
- The paint mask is read from the color map's colors: a warm-tinted metal
  part can take paint, and very dark wood can stay bare.
- Pistol silhouettes come from the small HUD icons and look blocky at the
  camouflage preview's size, painted or not.
- `browser_classes.gsc` calls `setWeaponCamos`, so it needs a server binary
  that has the method. The server image copies both; mounting the new script
  over an older image breaks the class script.

## Tests

- `python3 scripts/test-class-data.py` compiles the client's class data with
  sanitizers. It covers the weapon table, the storage lines with per-field
  defaults, the name rules and renaming, the menu response with camouflage
  only when a gun is painted and the ratings, and rejects 21 mutations. It also
  checks that `classes.json`, the C tables and the GSC lists agree, camouflage
  included.
- `node scripts/test-web-classes.cjs` runs the linked browser bridge. It covers
  the requests it sends and what it does with each answer: stored lines with
  their camouflage (`none` from a host before it), 400 and 503 refusals, a 200
  without classes, an outage, oversized replies, ordered saves. The bodies it
  saves pass the gateway's own validation. It rejects 13 mutations.
- `(cd downstream/gateway && npm test)` covers the store across reopening and
  the HTTP API: lookups, saves, cross-origin, invalid and oversized bodies,
  wrong methods, missing storage, invalid camouflage and the migration of a
  store from before camouflage.
- `python3 scripts/test-weapon-camo.py` compiles the camouflage catalog,
  renderer, client and server code with sanitizers. It covers the seamless
  deterministic patterns, the per-material masks (wood or the whole gun,
  specks cleared, every mip level), the per-frame entity tags, the painted
  menu pictures and their cache, the configstring read by weapon name and the
  `setWeaponCamos` method's checks, and rejects 31 mutations.
- `python3 scripts/test-class-camo.py` runs the production class menu on a
  recording UI: the camouflage lines, F2, Y and click opening the pickers, the
  grid's keys and clicks, saving only a change, the painted tiles and
  previews, Select Class's painted Custom classes, and the responses sent. It
  rejects 19 mutations.
- `node scripts/test-web-point-lights.cjs` checks the generated shaders'
  camouflage pass, that the pattern and mask bind on units clear of the scene
  compositor's, and their uploads; `python3 scripts/test-gamepad.py` that Y
  reaches the class screens only.
- `python3 scripts/test-class-select.py` runs the production Select Class on a
  recording UI with sanitizers: its layout, both tabs, Standard Issue keeping
  the defaults whatever the Custom classes hold, the response of each choice
  by key, number and click, reopening on the last choice, F1 editing only
  Custom classes and the backdrop request that Create a Class does not
  make. It checks the numbers against the capture's in Barlow's metrics and
  rejects 35 mutations.
- `python3 scripts/test-endgame.py` and `python3 scripts/test-web-scene-fx.py`
  check that a frame takes the backdrop the last one asked for, once, and that
  the compositor bounds and composes its gray; `node scripts/test-web-scene-fx.mjs`
  checks in WebGL2 that the gray leaves no color, keeps each pixel's luma and
  costs no pass.
- `python3 scripts/test-class-menu-pause.py` compiles the class screens with
  the pause menu's side of them: both lists, the store's note, a class chosen
  from either list or the class screen sent as the menu's response and
  remembered with its list, and the editor opened from the pause menu going
  back to it. It rejects 12 mutations.
- `python3 scripts/test-room-maps.py` checks that every room receives the script.
- `python3 scripts/test-browser-classes-runtime.py --assets PREPARED_MAIN` runs
  the dedicated server with a bot and no network. It makes 16 checks:
  - the class menu opening first for a player without a team;
  - each team carrying other countries' weapons;
  - an invalid class, refused, with or without a team;
  - a class's camouflage kept with it, a class without any painting none, and
    an unknown camouflage refused;
  - bare weapon names keeping the gametype's loadout;
  - three players who have just joined choosing a class: each gets a team, the
    teams stay within one player of each other, and the class applies;
  - an alive change waiting for the spawn and applying at the respawn;
  - a restricted primary keeping the previous weapon.

  With the script before Select Class, the class menu first, the team with the
  class and the balance checks fail.

  Removing the precache, the spawn hook, the validation, the restriction guard
  or the menu override makes it fail. Toujane and Carentan both passed.
  `--gametype sd` checks the class menu first and a class chosen without a team
  at an S&D spawn, then a class chosen before the match. That class applies
  whole, without the team's pistol and frags, and removing the in-place step
  fails it.
- In the browser, against an isolated stack:
  - Every screen was created, edited and renamed. The edits persisted across a
    reload, and the class list, the editor and back all worked with the mouse.
  - In a match, a British player spawned with a Kar98k and switched to a TT-30.
    The in-game menu showed "1. Choose Class", and choosing while alive gave the
    respawn message. A German player spawned with an M1 Garand after changing
    team.
  - Frogskin on Assault's MP44, Ember on its Luger and Winter on Rifleman's
    Garand were equipped by key; the pickers, the previews (the Garand's wood
    alone) and the Modify tiles showed them, and the gateway stored them. In a
    Carentan TDM match the server published `mp44_mp frogskin luger_mp ember`
    and the first-person MP44 and Luger were painted.
