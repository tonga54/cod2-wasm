# README screenshots

Real captures of this project, composed for the main README. Most come from a
local build of October 21, 2026, at 1920 × 1080; screencast frames and clips are
1280 × 720. The "day one" Carentan shot is from October 9, and the scope glint
and renderer comparisons come from those features' own QA builds of October 20
and 21, also at 1280 × 720. Images are only cropped and scaled; no scene content
has been generated or retouched. The composites add captions in
the game's Barlow lettering, numbered markers and gold highlight rings, nothing
else. The host's optional weapon pictures appear in the class screens; other
installations can show the default art.

Sources are in the ignored `out/readme-qa/shots/` directory unless another
`out/` path is given. `out/readme-qa/build_images.py` rebuilds the files from
them.

| Image | Content | Source captures |
| --- | --- | --- |
| `hero-stalingrad.jpg` | Stalingrad TDM with the full HUD | `sv-0.png` |
| `then-and-now.jpg` | Carentan on October 9 and October 21 | `out/carentan-gameplay.jpg` (October 9), `act-0.png` |
| `hud-anatomy.jpg` | HUD with six numbered markers | `b-tank1.png` |
| `create-a-class.jpg` | Class list, editor, weapon tabs, Select Class | `02-cac-list-final.png`, `05-cac-edit-3att.png`, `08-weapons-tab.png`, `14-select-class.png` |
| `attachments.jpg` | Four attachment screens | `04-attach-suppressor.png`, `04-attach-reflex.png`, `04-attach-optic4x.png`, `04-attach-list.png` |
| `camouflage.jpg` | Eleven finishes and Ember in play | `c-camo-0.png` to `c-camo-10.png`, `b-tank1.png` |
| `optics.jpg` | Reflex, Lens, 4x Optic, sniper scope, scope glint | `ads-reflex-2.png`, `ads-lens-1.png`, `o4n-b.png`, `sc-1.png`, `out/scope-glint-fade-qa/m700c-new-5.png` (glint cropped at 2×, ringed) |
| `explosives.jpg` | S-Mine 44 and C4, three steps each | `sm-frame.png`, `sm-emb-a.png`, `sm-view1.png`, `c4-stuck.png`, `c4-boom-1.png`, `c4-boom-3.png` |
| `bots.jpg` | Bot setup and a bot match | `12-bot-setup.png`, `cl-view.png` |
| `match-flow.jpg` | Countdown, killcam, victory, scoreboard | `j1.png`, `kc-12.png`, `ew2-157.jpg`, `ew2-168.jpg` |
| `menus.jpg` | Nine menu screens | `01-main-menu.png`, `13-match-summary.png`, `10-map-select.png`, `11-game-rules.png`, `16-pause.png`, `17-overview.png`, `18-settings.png`, `19-graphics.png`, `21-controller.png` |
| `rendering-before-after.jpg` | Four renderer comparisons | `out/gfx-qa/final/shadow-a-off.png` and `shadow-a-on.png` (`r_sunShadows` 0 and 1); `out/visual-qa/shots/base-*.png` and `cand-*.png` for fire, leaves and relief (commit `887353d` and the change committed as `334f8d0`) |
| `combat-effects.jpg` | Motion blur, tracers, brass, muzzle flash | `f-1.png`, `bv-3.png`, `fx-6.png`, `mp44-132-*.jpg` |
| `maps.jpg` | The six maps in play | `p-1.png`, `roof-0.png`, `t-after-kill.png`, `sv-8.png`, `dx-2.png`, `bv-3.png` |
| `clip-mp44-burst.webp` | Animated MP44 burst, 2.5 s at 20 fps | `mp44-*.jpg` screencast, 250–2,700 ms |
| `clip-kar98k-scope.webp` | Animated Kar98k scope and shot, 3.4 s at 20 fps | `k98-*.jpg` screencast, 150–3,500 ms |
| `clip-smine-plant.webp` | Animated S-Mine 44 plant, 4.3 s at 18 fps | `smine-*.jpg` screencast, 300–4,500 ms |

The JPEGs and WebPs in this folder are documentation images only; original game
archives and standalone asset packs remain excluded from the repository. Game
imagery belongs to its respective rights holders.
