# Cover penetration and controller sprint

Implemented October 16, 2026 in the server bullet path and client input path.

## Ballistics

Penetration uses the collision surface flags already supplied by the map. It
does not guess from texture names. Wood, bark, glass, plaster, paper, cloth,
foliage, carpet and metal sheets have finite depth allowances. Rifle bullets can
also cross very thin brick/concrete; rock, terrain and unknown surfaces stop
them. Rifle/armor-piercing flags increase the allowance; shotgun pellets have
one quarter of their ordinary allowance.

Two short collision traces locate the back of eligible cover. The maximum
distance is reduced by the remaining bullet energy. The exit material can
further restrict penetration; thickness is measured along the shot, so oblique
hits lose more damage. Each successful layer also costs 12% of the current
energy. A shot stops below 10% energy or after three cover continuations.
The normal player hit trace resumes immediately after the exit. Range falloff
uses the original muzzle, including after cover or another player.

Shots starting inside solid geometry stop. A probe that cannot find a safe exit
stops as well, including very tight adjoining surfaces. Sky/noimpact surfaces
remain opaque. The original rifle flesh/team handling remains in place.
Damageable scripted entities keep their existing damage handling.

This is server-authoritative and needs the rebuilt dedicated server to take
effect. No asset repack, protocol change, physics projectile, additional exit
particle, shader, texture or render pass is required. Misses, direct player hits
and ineligible materials add no collision traces. Eligible cover costs up to two
short exit traces plus the continuation per layer. That is bounded extra server
CPU work at impact time, not a claim of zero CPU cost under all workloads.

## Controller

The sprint request now survives up to 250 ms of neutral stick input. This covers
clicking L3 immediately before pushing forward while prone. Backward movement,
combat actions, aiming, focus loss and menus cancel the request; an expired
request cannot start an unexpected later sprint. Existing remapped sprint
buttons keep the same behavior.

Standing to sprint clears the complete old crouch/prone key state, including
stored key identities, and cancels the old held-stance gesture. Releasing that
gesture cannot force prone again, and subsequent posture presses still work.
The shared movement code retains its headroom and sprint eligibility checks.

## Automated checks

- `scripts/test-bullet-penetration.py` compiles the production bullet functions
  together with the production brush collision solver under ASan/UBSan. It checks
  20,480 material/thickness cases plus angles, layers, hard back faces, very close
  blockers/targets, range falloff, glass, shotgun, armor piercing, inside-solid
  origins and explicit trace-count limits.
- `scripts/test-gamepad.py` combines actual controller polling and posture
  command generation in 128 repeated crouch/prone sequences, including delayed
  forward input, held buttons, release, sustained sprint and request expiration.
  Existing remap, focus, hotplug, menu and analog checks remain enabled.
- `scripts/test-player-stance-animations.py` verifies complete key-state cleanup
  and reuse, as well as existing posture/movement/animation regressions.
- `scripts/test-weapon-balance.py` passes 100,700 direct-hit damage comparisons
  against the reviewed weapon profile; `scripts/test-sprint.py` retains movement
  speed, interruption and prediction/server parity coverage.

The penetration and controller regression checks are included in
`scripts/test-static.sh`; the complete suite and package checks passed after
both builds. Build and runtime captures for this change are kept
locally under ignored `out/gameplay-qa/`.

The Emscripten client and Linux i386 dedicated server both compiled and linked.
A separate Carentan server was used for a Chrome smoke test with a synthetic
standard Gamepad API device, feeding the real SDL/controller path. Three repeats
of B hold → prone → L3 click/release → forward stick returned to standing with
the sprint weapon pose and movement after releasing L3. Screenshots and viewpos
logs confirm all three transitions. The test also fired the weapon and raised
no JavaScript exceptions. It is not a test with a physical controller, nor a
matched performance comparison. Existing running rooms were not restarted.
Chrome also logged one WebGL `drawElements` warning about an enabled attribute
without a bound buffer; its origin was not isolated in this gameplay check.
