# Custom classes

Players build up to five named classes in the browser and pick one in any
match, on either team. A class sets the primary weapon and up to three of its
attachments, the secondary pistol and one of its attachments, the lethal item (a
frag, the S-Mine 44 or C4) and the tactical grenade (smoke): CoD2's two offhand
slots. The screens follow the Create a Class flow of the Call of Duty: WWII
capture the owner supplied, drawn with CoD2's own images and the Barlow fonts
the menus already use, or with [pictures the owner keeps](#pictures).

## Using classes

- **Main menu → Create a Class** lists the five classes with the focused
  class's loadout at the right. Enter (or a click) opens **Modify** for that
  class; F1 renames it.
- **Modify** shows the four slots as tiles in two columns, primary over
  secondary and lethal over tactical. Under each gun a line names its
  camouflage, and under that are its attachment boxes: three for the primary,
  one for the secondary. The arrows move between the tiles and boxes as they
  lie; Enter opens the slot's list. **Equip Weapon** lists the slot's weapons
  from the left edge with a page counter, the weapon's picture, kind and
  country, its retail description where the game has one, and five rating
  bars. A different gun starts without attachments. **Modify Equipment** shows
  the tactical and lethal rows; up and down switch rows.
- Under the primary and secondary tiles, a line names each gun's
  camouflage. F2, a controller's Y (WWII's Customize) or a click on the line
  opens **Primary Camouflage** or **Secondary Camouflage**: eleven swatches
  four to a row (the gun's own finish, then the ten patterns below), the
  focused one's name, note and place in the list, and the gun's picture
  wearing it. The arrows move through the grid, Enter or a click equips and
  saves, Escape goes back. Grenades take no camouflage. The class list's
  panel, the Modify tiles and Select Class's Custom tab show the guns painted;
  Standard Issue keeps the game's own finish.
- An attachment box opens **Modify Attachment**: WWII's grid of the gun's
  attachments, three a row, with the focused one's description, a line for each
  value it changes on that gun (green better, red worse) and the rating bars
  with its change. Enter fits it in that box; Backspace, Delete or a
  controller's X takes the box's attachment off. An attachment already on the
  gun, or a second optic, is greyed with the reason in yellow.
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
secondaries are the Colt .45, Webley, TT-30 and Luger. The lethal items are the
four frags, the S-Mine 44 and C4; the tactical items are the four smokes (see
[placed explosives](#placed-explosives)). Every new gametag starts with these
classes; the primaries' attachments are the Prima guide's suggestions for their
WWII counterparts:

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
| M1 Garand, SVT-40, G43, MP44, BAR | Rifle | Reflex Sight, Lens Sight, 4x Optic, Quickdraw, Grip, Steady Aim, Rapid Fire, Extended Mag, FMJ, High Caliber, Advanced Rifling |
| M1A1 Carbine | Rifle | The rifle list without the Lens Sight, as WWII's carbine |
| Bren | LMG | Reflex Sight, 4x Optic, Quickdraw, Grip, Steady Aim, Rapid Fire, Extended Mag, FMJ |
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
| Advanced Rifling | Damage ranges ×2 on rifles, ×1.3 on SMGs, ×1.25 on the BAR and pistols, ×1.13 on the Trench Gun | WWII |
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

WWII's Extended Mag gives the Bren a 100-round pan; here it takes the +50% of
the others, as its longer box magazine shows.

Sources: [Prima's WWII guide](https://primagames.com/eguides/call-of-duty-wwii/multiplayer-introduction/firing-range/primary-weapons)
and the Call of Duty Wiki's [Bren](https://callofduty.fandom.com/wiki/Bren) and
[Advanced Rifling](https://callofduty.fandom.com/wiki/Advanced_Rifling) pages
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

Each class carries `primaryAttachments` and `secondaryAttachments`, arrays of
attachment names; a `PUT` without them saves none. The rows keep them in
`primary_attachments` and `secondary_attachments`, names joined by `+`. A store
created before attachments gains the two columns, empty, when the gateway
opens it, so its classes have none. Stored attachments a class's gun no longer
takes are dropped and the rest of the row kept.

## Server rules

Every room loads `browser_classes.gsc` before the level finishes initializing:

- It precaches the 35 class weapons and the class menu.
- It points both teams' weapon menus at the class menu, and the menu the
  connect callbacks open first for a player without a team, the server
  information, too.
- It wraps the gametype's weapon callback. A class response names one weapon
  for each slot from the lists and attachments its guns take, or it is refused
  and the menu reopens. After the tactical grenade it may name the two guns'
  camouflage from the same list, then their attachments, `none` or names joined
  by `+`
  (`class,primary,secondary,lethal,tactical,primaryCamo,secondaryCamo,primaryAttachments,secondaryAttachments`).
  The client sends them only when a gun is painted or has attachments, so a
  server from before both still takes a plain class; a camouflaged answer
  without the attachments, as a client from before them sends, is taken too.
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

- the primary and the pistol with their attachments, both with full
  ammunition, an Extended Mag's included;
- one smoke grenade, when smoke is allowed;
- the frag count retail gives for that primary (`getWeaponBasedGrenadeCount`),
  or two S-Mines or two C4 charges, when frags are allowed;
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

With attachments, the same values come from the attached definition, Fire Rate
scaled by the engine's whole-millisecond fire time. The bars stay continuous
before rounding, so a small change such as Rapid Fire's 6% still shows as a
sliver of green; the number changes color only when its rating does. Grip,
Steady Aim, Quickdraw and Ballistic Calibration change no bar: their lines say
what they change.

## Placed explosives

The S-Mine 44 and C4 are generated from the owner's files when the private
archives are prepared (`scripts/browser_explosives.py`,
`scripts/browser_smine.py`); the repository holds none of their data.

- **S-Mine 44**, as Call of Duty: WWII shows it. G lowers the gun; the right
  hand brings up a green canister with a yellow band and its igniter, the left
  hand draws the safety pin, the right hand stands the mine upright beside the
  view and sets it down at the player's feet, upright and facing where they
  look. It takes about two seconds with a rifle. The hand animations are the
  Mk 2 grenade's pull-pin, with the left arm reaching for the S-Mine's pin and
  a held pose and setting down added. The mine arms after 1.2 s; an enemy's
  feet within 40 units click it and it goes off 0.15 s later. Its owner takes
  it back by holding Use (F, or X on a controller) for half a second within
  52 units of it, with it in sight: "Press and hold F to pickup S-Mine 44"
  shows under the crosshair. A mine outlives its owner's life and is taken
  back the same way after a respawn; it returns to the kit when there is room
  for it, so a full kit stays at two.
- **C4**, CoD4's charge with CoD2's sticky bomb. G takes out the detonator,
  fire throws a charge that sticks where it lands, and aim sets off every
  charge the player placed, wherever they are.
- Both show the **squad emblem** above them, pulsing slowly between faint and
  nearly full every 1.8 s: the Allies' star in its ring, or the Axis' iron
  cross in the same ring. The owner and teammates see it through walls up to
  1600 units away; enemies never do. It is a 16-unit disc 17 units above the
  explosive, drawn between 22 and 96 virtual units across. `cg_hudExplosiveIcons
  0` hides it.
- Each player keeps two of each kind; a third replaces the oldest. Both go
  away when their owner leaves or changes sides.

## Download and runtime cost

The Soviet weapons now ship with every map. They add 3.87 MB to the renderer
archive and 1.60 MB to the audio archive, taking the four prepared archives
from 324.1 MB to 329.6 MB. The S-Mine 44 and C4 add 0.41 MB to the renderer
archive and 2.90 MB to the audio archive, nearly all of it the development
host's 22 owner sounds for them, for 332.9 MB. The development host's 34 class
pictures add 8.34 MB to the renderer archive, for 341.2 MB.

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
- the attachment grid, three tiles a row, and its yellow notes;
- rating bars at the bottom right, green and red for an attachment's change;
- pill-shaped key prompts.

The repository holds no WWII logos, artwork, fonts, character or weapon
renders: the attachment tiles show their names, not WWII's icons. Nor do the
screens add things this game lacks: divisions, basic training, weapon levels,
unlocks, locks, the store or its currency. Select Class leaves out the capture's
Division and Basic Training panels and its four attachment tiles, and shows
keyboard keycaps where the capture shows a controller's buttons. The pictures
are CoD2's weapon menu images for primaries and silhouettes of its HUD icons
for pistols and grenades, unless the owner keeps their own. The camouflage
screen takes the capture's swatch grid, page counter and painted preview; its
patterns are this port's own, and all ten are available from the start,
without WWII's challenges.

## Pictures

The owner may keep a PNG for any class weapon in `data/main/owner_class_pictures/`,
named after the weapon (`frag_grenade_american_mp.png`); the format and
packaging are in [docs/UPDATES.md](UPDATES.md#owner-class-pictures). Every class
screen then draws that weapon in the picture's own colors: the list's loadout,
the editor's tiles, the equipment rows and preview, and the weapon list. Each
picture is cropped to its visible texels and fit to the box, and a
camouflage paints it as it would the stock image. Weapons without one keep
the stock image, tinted as before. Nothing outside the class screens
changes: the HUD's offhand icons, the kill feed and the ammo counter stay
white.

The development host's pictures cover 34 of the 35 class weapons, all but the C4:

| Weapons | Picture |
| --- | --- |
| American frag (M2) | CoD WWII's MK 2, cut out of the owner's capture |
| All four smoke grenades | CoD WWII's smoke canister, cut out of the owner's capture; in CoD2 every smoke grenade is the American canister |
| British frag (MK1), RGD-33, Stielhandgranate | Rendered from CoD2's world model of each, lit like the captures |
| S-Mine 44 | CoD WWII's S-Mine 44, cut out of the owner's capture |
| The 21 primaries and four pistols | Rendered from their world models the same way, muzzle to the left as the retail menu images have it |

The owner's other captures (the Signal Flare, Mk V gas grenade, No. 69,
throwing knife, satchel charge and No. 74 ST) show equipment this game does
not have. Their cut-outs wait in `data/wwii_equipment_pictures/`,
on canvases the generator accepts, for the day a class weapon matches one.

## Limits

- Gametags are shared, as described above.
- The server keeps a player's class while they stay connected. The menus
  highlight the last choice until the page reloads.
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
  defaults and older lines without camouflage or attachments, the name rules
  and renaming, fitting and removing attachments, the menu response with
  camouflage and attachments only when set, the ratings and the effects the
  attachment menu lists, and rejects 34 mutations. It also checks that
  `classes.json`, the C tables and the GSC lists agree, camouflage and
  attachments included.
- `python3 scripts/test-owner-class-pictures.py` covers the pictures: the PNGs
  the generator accepts and refuses, the IWis it writes against a reference of
  every mip, the folder's names, and the checker. Under ASan/UBSan it loads the
  generator's files through the engine's loader against a recording backend,
  and runs the menus' choice between a picture, a retail image and a silhouette.
  It rejects 30 Python, 17 loader and 7 menu mutations.
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
  their camouflage and attachments (none from a host before them), 400 and 503
  refusals, a 200 without classes, an outage, oversized replies, ordered saves.
  The bodies it saves pass the gateway's own validation. It rejects 16
  mutations.
- `(cd downstream/gateway && npm test)` covers the store across reopening, the
  camouflage and attachment columns added to older databases, and the HTTP
  API: lookups, saves, invalid camouflage, attachments the rules refuse or
  drop, cross-origin, invalid and oversized bodies, wrong methods, missing
  storage.
- `python3 scripts/test-weapon-camo.py` compiles the camouflage catalog,
  renderer, client and server code with sanitizers. It covers the seamless
  deterministic patterns, the per-material masks (wood or the whole gun,
  specks cleared, every mip level), the per-frame entity tags, the painted
  menu pictures and their cache, the configstring read by weapon name and the
  `setWeaponCamos` method's checks, and rejects 31 mutations.
- `python3 scripts/test-class-camo.py` runs the production class menu on a
  recording UI: the camouflage lines, F2, Y and click opening the pickers, the
  grid's keys and clicks, saving only a change, the painted tiles and
  previews, the list's side panel with each slot's caption clear of the tile
  above it, Select Class's painted Custom classes, the camouflage lines and
  attachment boxes laid out together, Modify Attachment's painted gun, and the
  responses sent with both. It rejects 25 mutations.
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
  the dedicated server with a bot and no network. It makes 22 checks:
  - the class menu opening first for a player without a team;
  - each team carrying other countries' weapons;
  - attachments fitted at spawn with full magazines (a PPSh with an Extended
    Mag holds 106 rounds and 319 in reserve), and older answers carrying none;
  - an invalid class, refused, with or without a team, and invalid attachment
    sets refused;
  - the engine's own validation and fitting;
  - a class's camouflage kept with it, a class without any painting none, and
    an unknown camouflage refused;
  - bare weapon names keeping the gametype's loadout;
  - three players who have just joined choosing a class: each gets a team, the
    teams stay within one player of each other, and the class applies;
  - an alive change waiting for the spawn and applying at the respawn,
    camouflage and attachments included;
  - a weapon given again starting without attachments;
  - a restricted primary keeping the previous weapon.

  With the script before Select Class, the class menu first, the team with the
  class and the balance checks fail.

  Removing the precache, the spawn hook, the validation, the restriction guard,
  the menu override or the attachment fitting makes it fail. Toujane and
  Carentan both passed.
  `--gametype sd` checks the class menu first and a class chosen without a team
  at an S&D spawn, then a class chosen before the match. That class applies
  whole, with its Extended Mag and without the team's pistol and frags, and
  removing the in-place step fails it.
- `python3 scripts/test-placed-explosives-runtime.py --assets PREPARED_MAIN`
  plays a test client with the S-Mine through the dedicated server, with no
  network. It makes 9 checks: G sets one down, a held Use takes it back and a
  tap leaves it, both go down and come back, and a mine left from a life that
  ended is taken back with the full kit the next life starts with.
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
  - The class list showed each gun's attachment count and the editor its
    boxes. Modify Attachment fitted, replaced and removed attachments, listed
    each one's effects against the stats, and refused a second optic with its
    note. The gateway kept the choices.
  - In a match the MP44 with an Extended Mag showed 45 | 270, and its 4x Optic
    aimed through the German scope with the Hold Breath prompt. The suppressed
    shot and the Reflex and Lens reticles were not checked on screen; the tests
    above cover them.
