# Technical and gameplay reference

Detailed gameplay rules, implementation notes and host operations for
[cod2-wasm](../README.md). For the project overview, screenshots and quick start,
start with the [README](../README.md).

## Maps and game modes

| Map | Allies | Axis |
| --- | --- | --- |
| Brecourt, France (`mp_brecourt`) | American | German Normandy |
| Burgundy, France (`mp_burgundy`) | American | German Normandy |
| Carentan, France (`mp_carentan`) | American | German Normandy |
| Dawnville, France (`mp_dawnville`) | American | German Normandy |
| Leningrad, Russia (`mp_leningrad`) | Soviet | German Winter |
| Matmata, Tunisia (`mp_matmata`) | British | Afrika Korps |
| Stalingrad, Russia (`mp_railyard`) | Soviet | German Winter |
| Toujane, Tunisia (`mp_toujane`) | British | Afrika Korps |

All eight maps support the original **Deathmatch, Team Deathmatch, Capture the Flag,
Headquarters, and Search and Destroy** modes. **Start New Server** is laid out
as CoD WWII's MATCH screen: **Mode** and **Map** open lists of the modes and
maps, each described at the right (a map with its loading-screen art),
**Game Rules** sets the time and score limits, **Bots** opens the bot setup,
and **Start Match** creates the room and joins it. Rooms rotate through
Toujane, Carentan, Dawnville, Burgundy, Stalingrad, Brecourt, Matmata and Leningrad while keeping their
selected mode and rules.
Unless the creator changes them, TDM uses 100 team points or 15 minutes; the
other modes use their original scripts and settings.

The host starts with **no rooms**. Users create their own servers, each named
after its creator's gametag (its letters, digits, spaces and `_|.-`, up to 32),
with up to three concurrent rooms and 64 configured player slots per room.
**Join Game** lists them by server name, map and players in the same style as
the other menus, refreshed every five seconds; Enter or a click joins. An empty
room closes after five minutes. **Join Game → DEL (Delete my server)** lets its owner
remove it immediately. Ownership is stored in that browser and checked with a
private token; changing browsers or clearing site storage loses that record.
The slot limit is configuration, not a claim that 64-player matches have been
load-tested.

## Match rules

In **Start New Server → Game Rules**, set **Time Limit** (Unlimited, 5, 10,
15, 20, 30, 45 or 60 minutes) and **Score Limit** the same way as the bot
settings. The score counts what each mode counts, with Unlimited also offered:

| Mode | Score Limit | Without changes |
|---|---|---|
| Deathmatch | 10–150 kills by one player | 30 minutes, 100 kills |
| Team Deathmatch | 25–300 team kills | 15 minutes, 100 kills |
| Capture the Flag | 1–20 captures | 30 minutes, no capture limit |
| Headquarters | 60–900 points (one a second holding the radio, 10 for destroying the enemy's) | 30 minutes, 300 points |
| Search and Destroy | 3–15 rounds won | no time limit, 10 rounds |

Choosing another mode returns to that mode's own rules; the match ends at
whichever limit comes first, on every map of the room's rotation. For a custom
room creator, POST `/servers` with `timeLimit` (0, 5, 10, 15, 20, 30, 45 or 60)
and `scoreLimit` (a value the mode offers, 0 for none); omitted fields keep the
mode's own.

## Bots

In **Start New Server → Bots**, set **Bot Count** (None, 1–16, 20, 24, 28 or 32)
and **Difficulty** (Easy, Normal or Hard) with ← and →, Enter or a click; a
click left of the value steps back. One count and difficulty cover the whole
room. The default is **None**. Settings
belong to that room and survive map rotation.

| Difficulty | Reaction delay | Aim and turning |
| --- | --- | --- |
| Easy | 700 ms | Slower turns and wider aiming error |
| Normal | 400 ms | Balanced turns and accuracy |
| Hard | 200 ms | Faster turns and smaller aiming error |

Bots are server-controlled players, marked **[BOT]** in their names. They join
teams through the original gametype callbacks, choose faction-appropriate
SMGs, rifles, automatic weapons and scoped sniper rifles, patrol navigable routes, acquire visible enemies,
shoot, reload and respawn under the mode's rules. Walls, other players and
smoke block target visibility. Health, ammunition, recoil and weapon cadence
use the same gameplay rules as human players.
DM/TDM bot scheduling is independent of the objective-roster refresh deadline,
so the 30 Hz server cannot enter an uninitialized objective list when a script
wait finishes early.

| Game Type | Bot objectives |
| --- | --- |
| Deathmatch | Patrol and fight other players, including other bots. |
| Team Deathmatch | Spread across patrol areas, share recent visible contacts, flank and engage at weapon-appropriate ranges. |
| Capture the Flag | Attack the enemy flag, return it home, recover dropped friendly flags, intercept enemy carriers, escort friendly carriers and guard the base. |
| Headquarters | Approach the announced radio, capture or neutralize it, then defend it under the original respawn rules. |
| Search and Destroy | Split between bomb sites, select a planter or defuser and have teammates cover them; defend sites and planted bombs. |

Planting and defusing use ordinary held-use input and the original timers;
bots do not award themselves objectives or bypass round rules. A bot already
interacting, or a human interacting nearby, keeps that role while teammates
cover. Difficulty also controls sight angle, bursts and how often objectives
are reconsidered. Normal and Hard bots seek nearby cover when wounded or
reloading. Snipers favor elevated, sheltered firing lanes, hold their position
instead of chasing distant targets, and relocate when compromised. Stalled
objective approaches can hand their role to another bot. Lost enemies are remembered only at their last visible position.

Navigation is built from map collision, spawn locations and announced
objectives, with bounded work per server frame and shared spatial lookups.
Cached A* routes, local doorway refinement, route-progress checks and temporary
blocked-edge exclusions improve progress. Navigation and search budgets are
shared across all bots; patrols remember visited areas and spread their destinations.
Route reservations favor less crowded lanes, and blocked teammates use a stable
priority to yield passage. Guards evaluate firing lanes and low cover within a
shared tactical trace budget. Original ladder materials connect accessible
rooftops. Directed, collision-checked drops and low-parapet jumps provide
exits from supported elevated areas; scoped bots seek lookout positions and wait to finish aiming.
This is first-party multiplayer AI, not the retail campaign AI. Complex routes
and tactical coordination can still need improvements.
See [Bot behavior and validation](BOT_AI.md) for the implementation and
the distinction between autonomous matches and controlled interaction tests.

A host that cannot keep up sheds bots: after three seconds with simulation
frames running late or nearly a whole core in use, one bot leaves ("Server busy:
a bot left to keep the match smooth."); after a minute and twenty calm seconds
they come back, never above the room's count, and later maps keep the count the
host carried. See [the governor](STABILITY_AND_SECURITY_2026-10-20.md#bot-governor).

Bots occupy ordinary player slots: eight bots leave 56 human slots, and 32 bots leave 32. They do not
keep a room alive after its last human leaves; the usual five-minute idle
cleanup still applies. A room's bots wait frozen where they spawned until the
first person spawns or spectates and the countdown before the match ends. No browser or rendering process is needed for each bot.
For a custom room creator, POST `/servers` with `botCount` (integer 0–32) and
`botDifficulty` (`easy`, `normal`, `hard`); omitted fields retain the defaults.

## Additions beyond the original game

These are intentional changes, rather than features claimed to be part of
retail CoD2:

- **Configurable multiplayer bots:** first-party server AI with per-room quantity
  and difficulty settings, objectives for all five modes, cover, normal weapon
  rules and collision-based navigation.
- **Custom classes:** **Create a Class** in the main menu keeps five named
  classes for your gametag, each with a primary from any faction, a pistol, a
  lethal item (a frag, the S-Mine 44 or C4) and a smoke grenade. Either gun can
  wear one of ten camouflage patterns, chosen from a WWII-style swatch grid (F2
  or a controller's Y on a gun's tile); the first-person gun and each player's
  gun in hand are painted with it. Joining a match opens **Select Class** first, laid
  out as CoD WWII's Select Division: **Custom** lists your classes, **Standard
  Issue** the game's five defaults, and choosing one puts you on the team the
  mode's auto-assign picks. The pause menu's **Class** tab (Escape) lists both
  again, with or without a team. The gateway stores the classes in SQLite;
  there are no accounts, so a gametag's classes belong to whoever uses it.
  Pictures of the host owner's can show a class weapon in color in these
  screens, as WWII's do, and its camouflage paints them; the HUD keeps its
  white icons. See [custom classes](CLASSES.md).
- **Weapon attachments:** a class's primary takes up to three of Call of Duty:
  WWII's attachments and its pistol one, from each gun's WWII list, one optic
  at a time: Extended Mag, Rapid Fire, Quickdraw, Grip, Steady Aim, Advanced
  Rifling, FMJ, High Caliber, Suppressor, Ballistic Calibration and the Reflex,
  Lens and 4x sights. They change that player's magazine, fire rate, aiming,
  spread, recoil, sway, zoom, damage range, penetration and head damage, in
  prediction and on the server alike, and a suppressed gun is quiet, without
  flash and off the enemy minimap. Each one also costs something: a slower
  aim, a harder kick or a shorter damage range, shown in red in Create a Class
  beside its gains. CoD2 has no attachment models; the sights are
  drawn reticles and the 4x Optic a sniper scope. Raised, the Reflex and Lens
  Sights frame the world in their ring, as WWII's do, with nothing of the gun's
  iron sights inside it and the reticle glowing on the glass. See
  [attachments](CLASSES.md#attachments).
- **Placed explosives:** CoD WWII's S-Mine 44, set down at your feet after
  the left hand draws its pin, which clicks when an enemy steps on or beside it
  and springs up to explode at chest height, and CoD4's C4, thrown to stick and
  set off with aim from anywhere. Your squad's emblem, a star or an iron cross,
  pulses over them for you and your teammates. Hold Use over your own S-Mine to
  take it back; enemies can shoot it to set it off, and its blast is theirs. See
  [placed explosives](CLASSES.md#placed-explosives).
- **Rejoining keeps your stats:** a person who leaves a match and joins the
  same room's match again under the same gametag gets back their match points,
  kills, assists, headshots, deaths and retail score, in any slot and on
  whichever team the mode assigns. The gametag matches as the class store's
  does: color codes, case and surrounding spaces aside; "Player", "Unknown
  Soldier" and the engine's "UnnamedPlayer" belong to nobody, and bots keep
  nothing. The server keeps up to 64 departed gametags (two sessions under one
  add up) until the next map, a restart or S&D starting its match over;
  `browser_rejoin.gsc` carries S&D's score and deaths through its rounds.
  Closing or reloading the tab counts as leaving: the gateway tells the engine
  at once instead of the 30-second `sv_timeout`.
- **WWII countdown before a match:** players who spawn before a match begins
  wait through one 15-second countdown, free to look around and aim but not to
  move, fire, throw, reload or use. It starts when the first person spawns or
  chooses to spectate; bots wait frozen until then, and the time limit starts
  when it ends. The
  view turns black and white. In place of the HUD it shows the mode and map
  over the mode's retail objective, the viewer's team and, at the right, the
  seconds left. The last five turn red, and color returns over the last two.
  With the owner's [countdown music](UPDATES.md#owner-countdown-music),
  it plays from the spawn and is gone, faded out, 5 seconds into the match.
  Whoever joins after the match has begun plays at once. S&D counts down
  through its first round's grace period instead. `cg_hudPrematch 0` hides it
  on a client; the room's `scr_prematch_time` (0 to 60 s, default 15) sets or
  disables it. See the
  [graphics notes](GRAPHICS_2026-10-16.md#wwii-countdown-before-a-match-october-18-2026).
- **End of a match:** the result shows for 8 seconds, both sides' scores or
  deathmatch's best three, then the final scoreboard until the next map loads,
  24 seconds in. With the owner's
  [end of match music](UPDATES.md#owner-end-of-match-music), it plays
  throughout and fades out over the last 3 seconds.
- **WWII pause menu:** Escape in a match blurs and darkens the live view and
  opens tabs laid out from Call of Duty: WWII's: **Class** (your five classes
  under Custom, the five defaults under Standard Issue, with the focused
  loadout), **Overview** (mode and map, both scores, your K/D, score, kills,
  assists, deaths and headshots, the time left and the whole map with your
  team), **Change Team** (free slots per side, Auto-Assign), **Vote**,
  **Leave Game** and **Settings** (Controls, Graphics, Audio, Game). It
  replaces the retail in-game menu, whose Mute Players entry is gone. See
  [the pause menu's measurements](GRAPHICS_2026-10-16.md#wwii-pause-menu-october-18-2026).
- **Votes in a match:** the pause menu's **Vote** tab calls the server's
  votes: Change Map (the eight arenas of Start New Server, in the mode played),
  Change Mode (the five modes, restarting the map), Restart Map, Next Map and
  Kick Player (the people in the match); the map or mode played is tagged NOW.
  While a vote runs, the HUD shows it with its keys (F1 yes, F2 no) and "Press
  ESC to vote", and Escape opens the menu on its ballot: what is voted on, the
  seconds left, both counts and Vote Yes / Vote No. A vote passes when more
  than half of the people playing vote yes, the caller's own yes included;
  spectators and bots do not count, so a player alone with bots passes one by
  calling it. In a match the browser leaves F1 and F2 to the game.
- **Sprint:** Shift + W increases movement speed, lowers and swings the weapon,
  and uses the running body animation. It also stands up from crouch/prone when
  headroom permits. Sprint currently has no stamina limit.
  Melee moves to V; scoped Hold Breath continues to use Shift.
- **More health:** players and bots have 105 health instead of retail's 100 in
  every mode. Bolt-action rifles without a scope kill with one shot to the
  head, neck or chest and need two to the stomach or a limb, as WWII's Kar98k
  with its Iron Sight does; scoped rifles kill with one shot to the body, and
  semi-automatic rifles and pistols but the TT-30 with one to the head up
  close. Health still regenerates to full, and falls hurt in proportion to it.
- **Melee damage:** butt strikes deal at most 50 of retail's 100 health, scaled
  to the players' 105 (53), without random bonuses or bullet hit-location
  multipliers. Two hits kill a player at full health; an injured player can
  still die from one. Weaker melee weapons keep the same share of a life. This
  server rule leaves the retail weapon assets intact.
- **Mounted MG42 overheating:** five seconds of continuous fire fills the heat
  bar, followed by eight seconds of cooling. Heat belongs to the gun and is
  synchronized with its user. Mounted aim also kicks upward and sideways, with
  stronger recoil during sustained bursts.
- **Custom LAN weapon balance:** `downstream/weapon-balance.json` keeps retail
  cadence, magazines, reloads and handling, and gives weapons alike in class,
  damage, rate of fire, zoom and bullet one damage model: the PPS-42 hurts as
  the Thompson does, the Grease Gun as the MP40, and the M1 Garand as the G43
  and SVT-40. No automatic weapon kills with one headshot, and two do. With the
  players' 105 health, the MP40, Grease Gun and pistols deal 40 instead of 45,
  while a pistol's headshot (×3.125) still kills up close. SMGs firing 700
  rounds a minute or more (Thompson, PPS-42, PPSh) keep full damage to 600
  units and slower ones to 700, all falling to 10; the automatic rifles (MP44,
  Bren, BAR) fall from 40 to 28 between 1,200 and 2,500 units, with wider hip
  fire and 0.2° of aimed spread. The PPSh deals 30 instead of 35 up close (four
  body shots) and kicks higher and wider when aimed. Bolt-action rifles
  without a scope deal 90 to the stomach and upper arms instead of retail's
  135; scoped rifles, the shotgun, grenades and the mounted MG42 keep their
  original definitions; the mounted heat/recoil rules are implemented
  separately.
- **Muzzle flashes:** every gun's view and world flash is rewritten in the
  renderer archive (`scripts/browser_muzzle_flashes.py`), after CoD WWII's:
  the original's lights and smoke, then a broad warm glow, a flame core
  drawn from eight generated shapes, one to three tongues of flame from
  eight side views fanning out of the barrel, a few sparks and a puff of
  smoke, each shot drawing its own shapes, size, proportion, strength and
  warmth, sized by weapon family. The flames are gone within 45 ms, the
  glow within 75; they replace CoD2's grenade-fireball puffs that swelled
  beside the gun for 100 ms. The textures are generated by correctly
  rounded operations only, so the Pi's rebuild matches the manifest. See
  [graphics validation and limits](GRAPHICS_2026-10-16.md#muzzle-flashes-after-cod-wwii-october-25-2026).
- **Grenade explosions:** each surface's `grenadeExp` effect (blacktop,
  concrete, dirt, mud, snow, wood) is rewritten in the renderer archive
  (`scripts/browser_grenade_explosions.py`), after CoD WWII's frag. It keeps
  its own debris, dust and crater, still 35% larger as the client loads it,
  and runs two generated effects in place of CoD2's fire, light and smoke: a
  white flash, four to six fireballs that roll out and rise while playing
  sixteen generated frames from white-hot to broken red, sparks, falling
  embers and a warm light of a sixth of a second; then a dark cloud lit
  orange as it forms, a plume, a ring of the surface's dust and a haze,
  climbing and thinning over three to six seconds. Grit waits 180 ms and
  craters 250 ms, so neither shows through or under the fire. Water keeps its
  splash; grenade damage is unchanged. The textures are generated by
  correctly rounded operations only. See
  [graphics validation and limits](GRAPHICS_2026-10-16.md#grenade-explosions-after-cod-wwii-october-25-2026).
- **Effect animation:** sprites, oriented particles and tails step through
  their atlas sequences, and sprites and oriented particles turn by their
  `rotationDelta` channel, as retail's `0x495f70` and `0x495ed0` do; the
  reconstruction had dropped both, so every atlas effect drew its first frame
  and nothing spun. Fire, smoke puffs, sparks, explosions and the generated
  muzzle flames now animate and vary as authored. Weather clouds are
  unchanged.
- **Smaller smoke screens:** a smoke grenade's cloud spreads 60% as far as
  CoD2's, about 15 m across at its fullest instead of 25. Each particle's spawn
  ring and offset, drift, gravity and size shrink alike when the effect loads
  (`FX_ShrinkSmokeGrenade`), so the cloud keeps its look and its density through
  the middle; its particle count and how long it lasts, up to 48 seconds, are
  unchanged. The client's smoke test, which hides names, the scope glint and
  aim assist, follows the smaller cloud.
- **Smoother baked shadows:** sun visibility uses all four source samples when
  preparing each lightmap texel at map load. Shadow edges have less aliasing,
  with the same uploaded texture size and no additional gameplay render passes.
- **Material lighting:** the browser applies the original normal, specular and
  detail maps that the DX7 renderer never sampled: relief, the sun's incidence
  on lightmapped surfaces (`r_sunShading`, 0 to 1, default 0.65), sun
  highlights and a reflection of the surroundings, brighter toward the sky and
  at grazing angles, on weapons, models and world surfaces with a specular map
  (`r_reflections`, archived, on); rough maps reflect little. Lightmapped layers
  blended over the world, such as terrain and decals, take the same lighting
  and shadows as the surface under them. `r_materialLighting 0` restores the
  previous look. Normal and specular maps load at half resolution
  (`r_picmip_bump`/`r_picmip_spec` 1), about 24 MiB more texture memory per map,
  and filter at up to 16× anisotropy. A 3D view rendered below the canvas's
  lines samples its color maps one mip level finer per halving of resolution
  (0.58 levels at 720 of 1080), so textures keep the output's detail.
  See [graphics validation and limits](GRAPHICS_2026-10-16.md).
- **Sun shadows of moving models:** players, bodies, grenades, dropped weapons
  and script models in view cast the sun's shadow on the world, from a
  2048 × 2048 depth map over 2,048 units around the view drawn once per frame
  (`r_sunShadows`, archived, on). Shadows only darken surfaces the map's bake
  lights with the sun, so they never fall indoors or into baked shade; static
  props keep their baked shadows. See [graphics validation and limits](GRAPHICS_2026-10-16.md).
- **Ambient occlusion:** corners, crevices and the ground under objects darken
  from the scene's depth, computed at half resolution over two rings, 22 and
  66 units wide (`r_ssao`, archived, on). The weapon and the sky stay
  unaffected, and it fades out past 1,400 units.
- **Relief folds and local contrast:** where a lightmapped surface's normal
  map folds into a joint or a groove the surface darkens, and over an edge it
  lightens, measured per world unit (`r_cavity`, 0 to 2, archived, 1). Each
  pixel's light moves from its surroundings' by up to 14%, less in the
  highlights, so stones, bricks and planks stand out and near things separate
  from far ones (`r_clarity`, 0 to 2, archived, 1). See
  [graphics validation and limits](GRAPHICS_2026-10-16.md).
- **Surface realism:** lightmapped surfaces vary in tone over several
  repeats of their color map, the ground by up to 22% and walls by 8%, its
  paler patches drier and warmer and its darker ones cooler about the map's
  own color, so fields, roads and sand no longer repeat one tile and each map
  keeps its palette; terrain layers blended by their vertices meet along their
  texture's own stones and tufts, at the same average opacity
  (`r_surfaceVariation`, 0 to 2, archived, 1). A detail map's grain tilts the
  lit normal, so the sun catches sand, stone and plaster up close
  (`r_detailRelief`, 0 to 2, archived, 1), and a magnified color map is
  sharpened against itself a mip and a half blurrier (`r_textureSharpen`,
  0 to 2, archived, 1); both fade out as the maps minify. About 0.1 ms a
  frame on an Apple M4. See
  [graphics validation and limits](GRAPHICS_2026-10-16.md#surface-realism-october-25-2026).
- **Sun shafts and shockwaves:** the sky about the sun throws shafts of light
  past whatever stands against it, wider and longer when the sun is low, from
  two quarter-resolution passes (`r_sunShafts`, 0 to 2, archived, 1). Grenade,
  S-Mine, C4 and rocket explosions bend the view about a spreading front for
  450 ms, never over the weapon (`r_shockwave`, archived, on). See
  [graphics validation and limits](GRAPHICS_2026-10-16.md).
- **WWII sniper scopes:** the scoped Kar98k, Springfield, Lee-Enfield and
  Mosin-Nagant, and the 4x Optic on the other rifles, the BAR and the Bren,
  stay on screen while raised, as in Call of Duty: WWII. The
  eyepiece's glass becomes a window onto the magnified world with the reticle
  on it, the world around the eyepiece blurs, and holding the breath (Shift)
  blacks out everything but the scope. The glass shows a dim reflection while
  the scope comes up or goes down, and working the bolt rolls the scope aside
  and back. `cg_wwiiScope 0` restores the stock full-screen overlay. See
  [graphics validation and limits](GRAPHICS_2026-10-16.md).
- **Map atmosphere and farther model detail:** the browser renders each map's
  original fog color/density, with separate world and HUD state and black fog
  for materials that request it. Model detail transitions and finite vegetation
  draw distances double by default. The archived `r_lodDistanceScale` setting
  ranges from 1 (original distances) to 4; profiles that saved the old 1.5
  default take 2 once. Authored lower-detail models, wall/frustum culling,
  texture resolution and mip filtering remain in use. When more static models
  pass culling than a scene's 2,046 entities hold, as on Brecourt, effects
  keep their slots and the nearest and largest models fill the rest (see
  [Effects and grass on Brecourt](GRAPHICS_2026-10-16.md#effects-and-grass-on-brecourt-october-21-2026)).
- **Browser hosting and controls:** create/delete rooms without a desktop game
  installation on each client, cached asset downloads with actual byte progress,
  English interface text, update notices, compact chat, and F10 for
  fullscreen. Change your gametag in **Options → Game Options**.
  The native profile saves the name, key bindings, mouse, sound and graphics
  settings in this browser, including across reloads and game updates. Browser
  storage must be enabled; clearing site data removes the saved profile.
- **Browser menus and video settings:** the main menu omits profile selection,
  Multiplayer Options, Quit and the online-play notice, and has no fullscreen
  option; settings omit voice chat.
  The menu version follows the deployed Git revision. Applying graphics
  settings releases renderer resources with the correct WebAssembly signature
  and resizes the canvas, including while connected to a match.
- **First-person weapon effects:** live players use the viewmodel muzzle for
  flashes, brass and tracers. BAR muzzle tails transform to world space once.
  Muzzle tails trail their own motion in the muzzle's axes, and the bolt-action
  rifles' starburst streak draws one whole atlas frame, as in retail (see
  [Kar98k muzzle flash](GRAPHICS_2026-10-16.md#kar98k-muzzle-flash-october-18-2026));
  ADS raise/lower poses follow the predicted aiming fraction without an extra
  half-second crossfade that can leave sights tilted during transitions.
- **Combat presentation within the existing render budget:** aiming preserves
  the weapon's original texture detail; material blur no longer lowers its mip
  resolution. Lighting reuses the same unmodified base texture sample.
  Tracers face the camera from their actual position as a tapered ribbon of
  eight segments, 100 units long, that bends a little and differently for every
  round (see [Shorter, bent tracers](GRAPHICS_2026-10-16.md#shorter-bent-tracers-october-21-2026)).
  Near-miss audio remains audible when
  visual tracers are disabled. The four default FX light slots prioritize
  camera proximity, radius and brightness. Shared lighting/shadow transforms
  and active texture attributes avoid redundant browser work; resolution,
  texture detail, particle counts and simulation rules are unchanged.
  See [graphics validation and limits](GRAPHICS_2026-10-16.md).
- **Rendering and collision efficiency:** the browser decodes vertex color
  order in the vertex shader instead of copying complete meshes, reuses
  validated index ranges and keeps unchanged matrix versions cached. Collision
  traces reject whole brush groups that the actual segment cannot reach before
  the nearest hit. Resolution, effects, geometry, collision results and server
  tickrate retain their original settings. See
  [performance measurements](PERFORMANCE_2026-10-16.md).
- **Remote player poses:** other players see vertical aim in the correct
  direction and Q/E as a lateral lean. Controllers rotate in model axes after
  skeletal composition, preserving movement animations. The respawn/teleport
  toggle no longer suppresses a living player's aim or lean.
- **Exact hit registration:** a shot meets the players near its aim in the
  pose its shooter's screen drew them in: origin, facing, view angles, lean
  and move direction, rebuilt bit for bit from the same snapshot samples, at
  any ping (see the anti-lag paragraph under performance). With the sight
  picture up, from 80% of the raise where the WWII scope's glass is whole, a
  shot leaves exactly along the sights, as in CoD WWII: CoD2 kept up to the
  hip spread through them after a run, a jump or the last shot, up to 6
  degrees for a scoped rifle (`g_exactAim`, archived, default 1; 0 keeps
  CoD2's spread). Hip fire and shotgun pellets keep their spread.
- **Pi server collision performance:** immutable map brushes use cached bounds
  for consecutive groups, preserving collision order, capsule dimensions and
  contact tolerances. A Pi 5 sample with eight protocol clients and 16 Hard bots
  sustained 30.2 updates/s/client at 86.8% of one core, versus 29.2 updates/s and
  93.9% before. This is a measured sample, not a maximum-player guarantee; see
  [the performance report](SERVER_PERFORMANCE_2026-10-13.md).
- **Bounded cleanup and respawn feedback:** bodies remain for eight seconds in
  an eight-body pool, dropped weapons/ammunition expire after twenty seconds,
  and TDM uses a three-second respawn countdown.
- **Safer respawns:** DM, TDM, CTF and HQ exclude occupied points and points
  within 1000 units of a living enemy when a free safe point exists. They prefer
  128 units of space from teammates and avoid points another player used in the
  last 750 ms. Up to eight visibility checks prefer cover from nearby enemies;
  team/objective weights rank the candidates. If every free point is threatened,
  the selector maximizes distance to the nearest enemy. See the
  [respawn rules and validation](RESPAWN_SAFETY.md) for crowded-map limits.
  Spectator and S&D round spawns retain their original rules.
- **Native update notice and Escape in fullscreen:** new installed client builds
  appear inside the main menu with an Update game action after disconnect.
  Repository pushes do not create player popups. Chrome/Edge keyboard capture
  lets a short Escape open the menu without exiting fullscreen; hold Escape
  or press F10 to exit. Unsupported/denied capture retains browser behavior.
- **Automatic Pi updates:** validated public `main` release packages are checked
  every five minutes, tested with real maps/bots in an isolated stack, and
  applied only when no players are connected. Failed activation restores the
  previous assets/images. Owner IWDs stay private on the Pi; see
  [update operations](UPDATES.md).
- **Movement conveniences:** C toggles crouch. Space first stands up from crouch
  or prone, including when the stance key is held; another press while standing
  jumps. Ordinary ground movement stops
  promptly when movement keys are released. Holding Space does not automatically
  jump again on landing. Steps require a clear path and a walkable landing;
  recovery from a small collision overlap does not inflict fall damage.
  Prone movement crawls at 15% of standing speed with the original forward
  and backward crawling animations. Mantling requires a fresh press, checks its
  path for obstacles, and stops safely if that path becomes blocked.
- **Owner sound pack:** an optional folder of the host owner's WAVs beside the
  original IWDs replaces the sounds of the original aliases it lists, keeping
  each alias's channel, distances and timing; see
  [docs/UPDATES.md](UPDATES.md#owner-sound-pack).
- **Owner class pictures:** an optional folder of the host owner's PNGs beside
  the original IWDs, one per class weapon, replaces the white HUD silhouettes
  and retail images of the class screens; see
  [docs/UPDATES.md](UPDATES.md#owner-class-pictures).
- **Fall damage and blood feedback:** falls above 256 game units progressively
  deal damage, reaching a lethal fall at 480 units. The thresholds are controlled
  by `bg_fallDamageMinHeight` and `bg_fallDamageMaxHeight`. Damage scales from
  maximum health, so the same fall remains dangerous when already wounded.
  Damaging falls and other hits briefly flash the original blood overlay, fading
  over 650 ms, with the existing low-health pulse retained.

## Original behavior restored in the port

- Semi-automatic weapons fire once per valid trigger press. Rapid clicks cannot
  bypass fire, bolt, raise or reload timers. Original magazines and weapon-specific
  ADS/reload timing are preserved. The M1 Garand reloads only after its eight-round
  clip is empty; the Lee-Enfield needs space for a complete five-round charger.
- Stale weapon selections after a respawn or loadout change recover to an owned
  gun, preferring the primary slots. Removed weapons, grenades and binoculars
  cannot leave a newly armed player waiting on an invalid weapon selection.
- Reloading returns to held/toggled aiming unless the player cancels aim.
  Switching weapons or striking cancels the current reload without adding
  ammunition; segmented reloads retain only completed insertions.
  Scoped Hold Breath lasts at most 4.5 seconds, with 5.5 seconds of recovery
  after exhaustion.
- Frag grenades have their original 3.5-second fuse, can be cooked in the hand,
  bounce off players, and can be picked up and returned with G while preserving
  the remaining fuse. Holding one too long kills its holder. Nearby live frags
  show the original icon and directional warning inside their blast radius,
  growing with proximity;
  cover can still block splash damage.
- Blast shellshock keeps the first-person weapon visible. Killcam samples use
  actual archive timestamps, including when the server tick rate changes.
  Script waits follow game time at the 30 Hz tick, so the killcam replays the
  seven seconds before the kill and the three after it.
- The player console is disabled, including its toggle command and saved bindings.
- Hits show the original red directional damage arc and low-health blood overlay.
  Teammate names remain visible without aiming at them. Kill notifications reach
  all players/spectators and expire normally.
- Original muzzle FX lights illuminate nearby floors, walls and opaque models,
  including during scoped shots. Their authored colors, radii and lifetimes are
  retained. Scopes stay clear of muzzle sprites and smoke.
- Other players use lateral/diagonal movement animations and blended stance
  transitions. Separate corpse bodies survive respawning, and dropped weapons
  remain available for pickup until their cleanup deadline. A dropped gun
  holds the rounds its player left in the clip and in reserve, and whoever
  picks it up gets those; the reconstructed pickup read the clip from a field
  nothing wrote, so it was lost and a full clip taken from the reserve.
- Effects that map and mode scripts play with `playFx` and `playFxOnTag` show
  their own effect: the retail minefields' blasts, the S&D bomb and the guns
  and vehicles it destroys at the sites, the HQ radio's explosion. The client
  looked them up where CoD2 1.0 keeps its effect table, inside 1.3's larger
  game state, and `playFx` one entry early besides, so each showed the red
  Missing FX placeholder.
- Wavelet-compressed textures keep their colors: Matmata's well, Toujane's
  vases, the sun flare of Brecourt, Burgundy, Carentan, Dawnville, Matmata and
  Stalingrad, the CoD2 logo of the main menu and the retail popups' dark
  backgrounds. The loader reversed each texel's bytes after decoding, a step
  of the Mac build, so RGB textures took their opaque alpha as blue and RGBA
  ones swapped alpha and blue.
- Original audio, tracers, wall impacts, bullet marks, map props, ladders and
  killcam are available. Smoke effects and sustained mounted-MG firing have
  dedicated crash regressions. Collision fixes cover angled walls and finite
  mesh corners in the supported maps.

## Controls

Defaults can be changed in **Options → Controls**.

Connect an Xbox, PlayStation or other SDL-compatible controller and press a
button to let the browser detect it. Open **Options → Controller**, choose
**Buttons 1** or **Buttons 2**, select an action with the mouse or A/Cross,
then press the desired controller button or
trigger. Controller assignments are saved in this browser independently of
keyboard/mouse bindings. Escape cancels an assignment; Backspace clears it.
The same page adjusts sensitivity, stick deadzone, inverted vertical look,
vibration and aim assist, and restores controller defaults. Left stick moves/navigates; right stick looks.
Within 15° of forward, back, left or right the left stick moves exactly that
way, so a run stays straight when the thumb drifts; the rest of each quarter
stretches to keep the diagonals.
By default, RT/R2 fires, LT/L2 aims, A/Cross jumps, B/Circle toggles crouch/stand
on a short press and goes prone when held for 300 ms (tap again to stand),
X/Square uses/reloads, Y/Triangle changes weapons, LB/L1 throws smoke,
RB/R1 throws a frag, L3 sprints/holds breath, R3 melees, and Start/Options pauses.
Click L3 once while moving forward to keep sprinting after releasing the click;
centering the stick, aiming, combat actions or changing stance cancels the run.
L3 plus forward also stands up from crouch/prone when there is headroom. A click
up to 250 ms before pushing forward is accepted, including after a long B hold.
While aiming, hold L3 to hold breath. Directional navigation highlights the
selected menu item, including team and weapon choices.
Use a browser that exposes the controller through the Gamepad API (HTTPS or
localhost).

While the controller is the device in use it vibrates with the original
console rumble: a kick for every round fired, held for 100 ms on single shots
so pistols and rifles are felt, and on automatic a kick per bullet over a
steady buzz that lasts the whole burst, until the gun stops firing or reloads;
nearby grenade, S-Mine, C4 and rocket blasts fading with distance, hits taken,
dying and melee impacts. **Options → Controller → Vibration** turns it off.
Chrome and Edge drive both motors; other browsers may not vibrate.

The controller also gets a minimal aim assist: over an enemy in sight the
right stick turns 30% slower, and while a stick moves the view follows 20% of
that enemy's motion. It never turns the view on its own or snaps onto
anyone, and the mouse never gets it. **Options → Controller → Aim assist**
turns it off.

Bullets can penetrate thin cover according to its collision material, thickness
and the weapon's power, losing damage with each layer. Wood, glass, plaster and
sheet metal offer less protection than thick masonry or rock. See
[ballistics and controller validation](BALLISTICS_AND_GAMEPAD.md).

| Action | Default |
| --- | --- |
| Move / lean | W A S D / Q E |
| Fire / aim | Left / right mouse button |
| Sprint / scoped Hold Breath | Shift; sprint while moving forward |
| Melee / interact / reload | V / F / R |
| Frag / smoke | G / 4 |
| Cook or return a frag | Hold G, then release to throw |
| Set down / take back an S-Mine | G / hold F over it |
| C4: take out, throw, set off | G, fire, aim |
| Toggle crouch / go prone | C / Ctrl |
| Stand up or jump | Space |
| Public / team chat | T / Y |
| Send / cancel chat | Enter / Escape |
| Quick messages | H |
| Class screens | Enter selects, F1 renames (edits a Custom class in a match), F2 or a controller's Y customizes a gun's camouflage, Escape goes back; in a match ← → or Tab switch Custom / Standard Issue and 1–5 choose |
| Pause menu (in a match) | Escape opens and closes; Q / E, Tab or LB / RB change tab; Enter or a click selects; arrows move; F1 edits a custom class; 1–5 choose a class; on Vote, → or Enter opens an action's choices and ← or Escape goes back |
| Vote yes / no | F1 / F2, or Escape and the Vote tab's ballot |
| Start New Server | ↑ ↓ or the pointer choose, Enter or a click opens and selects, ← → change a rule or bot setting, Escape or right-click goes back |
| Enter / exit fullscreen | F10 |

Chat keeps mouse capture while typing and displays five wrapped lines for eight
seconds by default. Clicking the canvas does not change fullscreen mode.

## Host locally

### Requirements

- Git, Python 3.9+ (3.12+ to build for the
  [Raspberry Pi](RASPBERRY_PI.md)), Node.js, and Docker with Compose.
- A local copy of the owner's original **CoD2 1.3 English IWD files**, including
  `localized_english_*.iwd`, placed in ignored `data/main/`.
- The original MP executable/installation ZIP for extracting the favicon, or an
  already prepared `data/browser/web/cod2.ico`. The startup logo is extracted
  from `data/main/iw_09.iwd`.
- The sibling [wasm-game-framework](https://github.com/theodorecharles/wasm-game-framework)
  checkout. Builds pin version **0.9.2**, commit
  `53bc7e6eeef1ae35dcf3b25dea4e3ec0ab46726f`.

The standard dedicated server uses a **32-bit x86 Linux binary** inside an amd64
container. Docker Desktop handles its execution on Apple Silicon. The browser
client itself runs on the player's machine, independent of the host CPU.

### Build and start

```sh
git clone https://github.com/tonga54/cod2-wasm.git
cd cod2-wasm
git clone https://github.com/theodorecharles/wasm-game-framework.git ../wasm-game-framework

# Import your original IWD files into data/main/ first.
python3 scripts/prepare-browser-bootstrap.py
python3 scripts/check-private-assets.py

# Set COD2_ORIGINAL_ZIP if your original installation ZIP is elsewhere.
./scripts/build-docker.sh
docker compose up -d
```

Emscripten **3.1.64** runs in Docker if it is not installed locally. Set
`COD2_BUILD_JOBS` to control build parallelism and `COD2_WASM_FRAMEWORK_DIR` to
use another framework checkout location.

Open **http://localhost:8088/** on the host, or **http://HOST_LAN_IP:8088/** from
another computer on the same network. Use **Start New Server** to create a room,
then **Join Game** on the other browsers. The server does not create matches at
startup.

### Services and private data

| Service | Responsibility |
| --- | --- |
| `cod2-web` | Framework shell, compiled browser client, private asset delivery |
| `cod2-gateway` | HTTP/WebSocket entry point on port 8088, room discovery/ownership, UDP forwarding, custom classes in the `cod2-classes` volume |
| `cod2-server` | Internal room supervisor and one authoritative engine process per room |

The game network is internal to Docker; raw UDP and the room supervisor are not
published to the LAN. The gateway forwards only the connectionless commands a
browser client sends (`getchallenge`, `connect`, `getinfo`, `getstatus`), caps
each connection's datagram size, rate and snapshot backlog, tells the engine
when a browser's connection closes, and containers run
without new privileges; see
[gateway security](STABILITY_AND_SECURITY_2026-10-20.md#gateway-security).
Private files are mounted read-only. Set `COD2_WEB_PORT`
to change the public port or `COD2_WEB_DATA_DIR` to use another private-data
location.

The page is an installable web app. Chrome installs one only with a PNG icon of
at least 144 px, and the executable's icon is 48 px, so the gateway draws the
install icons (`downstream/gateway/app-icons.mjs`) once at startup from the two
private originals it already serves, `cod2.ico` and the decoded `logo_cod2`: the
star over the lettering, on the loading screen's gradient. It serves them at
`/game-data/files/cod2-icon/app-192.png`, `app-512.png`,
`app-maskable-512.png` and `apple-touch-icon.png`; nothing derived from the
originals is committed or built into an image. The manifest lists them with the
original ICO at its real 16, 32 and 48 px sizes. `site/install-app.js` keeps
Chromium's install offer and shows **Install app** in the WWII menu's right
key-prompt place over the loading screen, then while the engine reports that
place free (`web_right_prompt_free`): the main menu itself, outside a match.
Every other WWII screen puts its Back there, and an installed window never
shows the label. The label takes no pointer events: the game keeps its own
cursor, and a click over the label opens the browser's install dialog without
reaching the menu. The manifest link sends credentials, so a host behind a login
(such as the Pi's Nginx overlay) remains installable.

The current six-map package is approximately **473.4 MB** (451.5 MiB) in
the checked-in manifest, including renderer and audio archives. Optional owner
sounds and class pictures affect the prepared package size; use
[`site/wasm-game-data.json`](../site/wasm-game-data.json) for exact sizes and hashes. Browsers download it once per site/cache version and keep it in
IndexedDB. A worker verifies each download's SHA-256; later starts check the
cached copy's size and signature. Archives of 128 MiB or more are read from
IndexedDB on demand instead of being copied into the page
(`?archives=memory` restores the copy). Original owner IWDs stay
untouched in `data/main/`; the prepared copies stay in `data/browser/`. Original
assets are excluded from Git and image layers. Generated WASM binaries are ignored by Git and shipped
in the web runtime image.

### Raspberry Pi

The [Raspberry Pi installation guide](RASPBERRY_PI.md) covers a fresh host,
the Pi 5 kernel requirement, building on your computer, deployment over SSH,
updates and troubleshooting. The repository includes a dedicated
[`Dockerfile.pi`](../downstream/server/Dockerfile.pi),
[`compose.pi.yaml`](../compose.pi.yaml), and deployment/runtime preparation scripts.
Deployments preserve an existing `~/cod2-edge/compose.security.yaml` overlay for
HTTPS, password protection and request limits. Use the configured HTTPS address
on secured installations; credentials and certificates stay outside the repository.
The game archives can also download from a free Cloudflare Worker through signed
links, so players do not use the home connection's upload; see
[Downloads from Cloudflare](RASPBERRY_PI.md#downloads-from-cloudflare).
The tested host is a Pi 5 with 8 GB RAM and 64-bit Debian; the required host page
size is 4 KiB. The web/gateway/supervisor run as ARM64 processes;
only the 32-bit x86 game executable uses QEMU. This preserves the engine's
32-bit layouts without claiming a native ARM engine port. Measure the host
under your expected player/room load before increasing it.

After preparing your own original assets, build on your computer and deploy:

```sh
DOCKER_DEFAULT_PLATFORM=linux/arm64 ./scripts/build-docker.sh
./scripts/deploy-pi.sh USER@PI_LAN_IP
```

Open **http://PI_LAN_IP:8088/** from another computer on the same LAN. The Pi
starts automatically after reboot, with users creating their own rooms. Follow
the full guide before running these commands on a new installation.

## Performance and verification

The default client draws at **1920×1080**, its 3D view at 720 lines
(`r_sceneHeight`), with full-resolution textures,
anti-aliasing and 16× anisotropic filtering where supported. Rendering follows
the display cadence, including 120 Hz. Performance changes cache static vertex
buffers, texture/sampler/render state, shader uniforms and animation work;
they do not reduce texture quality or remove map geometry/effects. Up to four
FX point lights are combined into existing opaque geometry passes. Their base
texture sample is shared with the material, and authored model normals prevent
flashes from illuminating the back side. Partial texture mip chains use their
existing final level instead of sampling black; no mip generation, additional
lights, geometry, resolution or post-processing passes are introduced. The browser
uses Emscripten's fixed-function-only GL path to avoid restoring an unused
programmable pipeline between draws; shader attributes, lighting and texture
filtering remain intact. Static model colors and index buffers also stay on the
GPU between draws: exact mesh ranges are cached, and authored indices retain
all of their vertices. Writes, layout changes and map teardown refresh or release
the cached data. Dynamic/animated geometry keeps its original streaming path;
its color conversion now needs one pass instead of two, preserving every channel
and alpha byte. Texture bindings and common uniform calls avoid temporary
strings/argument arrays. Four-light uniform updates preserve unrelated material
caches; all authored light values still reach the GPU.

The authoritative simulation is configured at **30 Hz** (33 ms simulation
steps), with clients requesting **30 snapshots/s**, up to **60 input packets/s**
and a 25 KB/s client rate. Actual snapshot delivery remains subject to bandwidth
limits and server load. The higher tick rate increases simulation work; validate
CPU and packet cadence with the intended player count before deployment. Other
players play back from their own movement samples, each snapshot's position at
that player's command time, with its velocity. Each plays behind its newest
sample by a delay that follows how late that player's samples arrive, at the
frame's real pace, so a player over the internet does not stop between
snapshots; past the newest sample, motion continues for at most 50 ms. Their
stride keeps the pace of the same command times. The server preserves fractional player
positions so integer rounding does not introduce small speed changes between
rendered frames; the existing snapshot format carries these coordinates.
A shot leaves along the view of the command that fired it. That command also
carries the camera's kick and a raised scope's idle sway, whose phase only the
client keeps, so the server aims where the scope was drawn; a bot's sway is the
server's. A command with attack held, or released within 300 ms, also
carries its view (`bg_shot_view.h`): for the four players nearest its aim in
the frame on screen, the two snapshot samples their playback drew them
between and how far along, or the newest and how far past it, quantized the
way the drawing itself used them. The view follows the packet's commands in
an op that older servers stop reading before. The server keeps every
player's snapshot samples for two seconds, angles as the snapshots carry
them, and traces those players in the pose the view names, their skeletons
rebuilt for the shot and restored after; a view may only name samples of the
player's current life, at most a second old. Anti-lag moves the other
playing clients, and every client when no view arrives, to their archived
positions at the shooter's command time whenever that time trails the
server, even by less than a frame. Before, that archive differed from the
drawn player by up to 12 units on a LAN and about 37 over a 300 ms or
stalling link (`test-shot-views.py`). `COD2_SHOTTRACE` set in the server's environment logs
each posed target and hit with that difference. At full aim a WWII scope's
rifle holds its resting pose, so the reticle on its glass stays on the line
the shot leaves along.
Command time stays monotonic after
stalls. Local shot effects are predicted once, avoiding duplicate sound/recoil. Local
prediction finds the first unacknowledged command in at most eight lookups
instead of scanning the full 128-command history each frame; it still replays
the same outstanding inputs and performs the same collision checks.

Server optimizations include UDP-readiness/deadline waits, cached byte-identical
Huffman encoding/decoding, byte-fragment bit fields, bounded local fragment
batches, larger receive buffers and gateway startup packet buffering. These
reduce work and avoid needless waits while preserving packet fields and
simulation rules. Snapshot writers search backward for the last changed field
and take an equality fast path for unchanged ammunition/objectives; regression
checks preserve the original packet bytes and overflow behavior. Collision
traces reject geometry outside the actual segment or beyond the nearest hit and avoid testing shared BSP brushes repeatedly;
the same collision geometry, contact margins and hit results are retained.
An exact-bit, bounded cache also reuses identical immutable-world sweeps in
the server and client. It resets on map changes, has independent native thread
storage, and excludes moving entities and temporary/transformed models. Replayed
prediction commands and repeated sightline checks can reuse their complete
results without rounding positions or skipping entity collisions.

The [30 versus 50 Hz Pi benchmark](TICKRATE_BENCHMARK_2026-10-12.md)
keeps **30 Hz** as the default. With two automated protocol clients and no bots,
both rates were sustained (about 21% versus 27% of one core). With eight
automated clients sending about 58 inputs/s each and 16 Hard bots, 50 Hz
saturated a core and delivered snapshots in bursts separated by seconds.
High client FPS and low average ping do not rule out interpolation jitter or
irregular snapshot delivery. These tests measure server/network work, not
browser FPS; the report includes the fractional-position correction and its
validation limits.

An earlier 60-second Carentan test on a Raspberry Pi 5 kept **eight real browser players**
connected throughout, with sprint, automatic fire, smoke and frags exercised.
Each player received 1,200 game packets (**20/s**); the server container used
**34.0% of one CPU core**, with no CPU throttling or container restarts. The
worst per-client packet-gap p95 was **54.7 ms**, and the maximum was **64.8 ms**.
The eight renderers shared one development Mac and sent approximately 10–27
input packets/s each, so this is not eight independent PCs at 60 inputs/s or
eight players continuously firing. See the [Pi load-test instructions](RASPBERRY_PI.md#repeat-a-connected-player-load-test)
for the measurement scope and a reproducible command.

A subsequent 60-second Carentan run sustained **32 Hard bots plus eight spawned
browser clients** at **20 game packets/s per client**, using **57.4% of one CPU
core**. Local status p95 was **18.1 ms**, server-reported ping p95 **45 ms**, and
the maximum server packet gap **112.6 ms**. Explicit realtime/capture checks
passed. This is a LAN load sample with eight renderers sharing one development
Mac; it does not guarantee minimum FPS or latency on every client/network.

Bot planning shares bounded work across the entire room. A* searches prioritize
routes toward objectives, retain usable routes and rotate pending requests
between clients. Nearby doors and short stair steps get collision-checked local
half cells; the rest of the map keeps its normal grid. Patrol destinations
penalize recently visited areas and teammates' destinations. Movement runs on
every original 50 ms simulation tick, including catch-up ticks. Bot-only HUD
name/crosshair queries are omitted because test clients have no renderer;
human labels, scripted look-at triggers, bullet hitboxes and graphics remain.
The native x86 build uses standard SSE2 float math to avoid expensive emulated
x87 arithmetic on the Pi, without fast-math. See [bot scaling and validation](BOT_AI.md)
for the work limits and measurement scope.

The optional **`?perfDebug=1`** overlay reports actual submitted frames and
frame-time percentiles. Building/smoke tests in Carentan and sustained MG fire
in Toujane measured about 120 FPS on the development Mac's 120 Hz display;
recent muzzle-light checks also held about 120 FPS. Results depend on the
client, host, network and player count. The project does not guarantee zero
latency or a fixed frame rate on every device.

The 2026-10-11 lighting/mipmap follow-up retained 120 FPS at the same Carentan
spectator camera and 1280×720 resolution. A warm reference repeat measured
4.37 ms main-thread / 3.15 ms GPU against 4.33 / 3.19 ms afterward; the initial
reference had 4.11 / 2.64 ms. These short desktop samples establish no reliable
GPU speedup and do not represent a low-end laptop. WebGL2 pixel checks verify
front/back model lighting, unchanged two-sided world lighting, matrix UVs and
partial-mip sampling. The updated client also completed a live Carentan
connection, weapon selection and firing check without shader errors.

```sh
# Browser package, shared gameplay, rendering, networking and UI regressions
./scripts/test-static.sh

# Gateway room/startup/ownership checks
(cd downstream/gateway && npm ci && npm test)

# Native i386 directory enumeration (also runs during the server build)
python3 scripts/test-native-file-listing.py

# Private assets and balance audit
python3 scripts/check-private-assets.py
python3 scripts/test-weapon-balance.py

# Custom classes through the dedicated server (Docker, prepared assets, no network)
python3 scripts/test-browser-classes-runtime.py --assets data/browser/main
python3 scripts/test-browser-classes-runtime.py --assets data/browser/main --gametype sd
python3 scripts/test-placed-explosives-runtime.py --assets data/browser/main

# Optional GPU fixture from the linked production shaders (after build-web.sh)
node scripts/prepare-web-lighting-qa.cjs
python3 -m http.server 8099 --bind 127.0.0.1 --directory out/web-lighting-qa
# Open http://127.0.0.1:8099/lighting-qa.html; all GPU checks must pass.

# Benchmarks; runtime benchmark requires the documented QA stack
python3 scripts/test-huffman-cache.py --benchmark
node downstream/gateway/benchmark.mjs --docker
```

The full build runs the static suite. Native code checks exercise real C paths,
including sanitizer-backed tests. A successful build alone does not establish
playable multiplayer; normal two-client browser checks cover combat, scopes,
reloads, room creation and the supported maps. Detailed measurements, deployment evidence
and remaining limitations are recorded in [RUNBOOK.md](../RUNBOOK.md).

## Updates

```sh
python3 scripts/update-local.py          # Check without installing
python3 scripts/update-local.py --apply  # Fast-forward, build, and restart an empty standard host
```

Installation requires a clean `main` checkout and no active players. Players
see update notices after leaving a match and choose when to reload. Pi hosts
use the Pi deployment procedure instead of the standard rebuild command. See
[updates and asset delivery](UPDATES.md) for cache/version behavior.

## Project and assets

This fork builds on [theodorecharles/cod2-wasm](https://github.com/theodorecharles/cod2-wasm)
and uses [wasm-game-framework](https://github.com/theodorecharles/wasm-game-framework)
for the browser shell. Maps, models, textures, sounds and original executables
remain the property of their respective rights holders. This repository supplies
code and tooling, not a copy or license of Call of Duty 2. Original game files
must be supplied privately by the owner and are excluded from Git.
