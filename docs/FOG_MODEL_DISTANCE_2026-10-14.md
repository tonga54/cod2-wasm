# Browser fog and model distance validation — October 14, 2026

The WebGL client now honors authored map fog through the existing fixed-function
shader path. The D3D bridge retains table/vertex modes, start/end and density,
applies them to world draws and disables fog for HUD/menu draws. DX7 materials
select standard map fog or black fog according to their `fogToBlack` flag; the
previous inverted selector made ordinary materials fade toward black.

The browser-only archived `r_lodDistanceScale` defaults to 1.5, with a supported
range of 1–2. It divides the model distance scale before the existing FOV
adjustment. Both static-model cull paths and model LOD selection use that same
scale. Existing graphics profiles receive the new setting without resetting
saved resolution, filtering, quality, controls or names. Finite grass draw
limits extend by 50% at the default zero LOD bias; ordinary lower LODs and
frustum/occlusion culling remain active. Texture resolution and mipmaps are
unchanged. Higher draw distance can increase rendering work in denser views.

## Validation

- Browser client compiled with Emscripten 3.1.64.
- New ASan/UBSan fog checks cover float-bit state transport, table/vertex modes,
  linear/exponential fog, world/HUD/off transitions, exact map color/density,
  material selection, additive black and cached backend color updates.
- New ASan/UBSan distance checks cover model LOD boundaries and finite grass
  disappearance at 101 distance scales, user quality/bias, FOV, locked PVS and
  the shared distance expression in both static-model cull paths.
- Linked shader/point-light tests pass. The archive regression now extracts
  `VariableStackBuffer` from the production header, checks four-byte VM values,
  and exercises encoded caller positions on 64-bit hosts and native pointers
  on i386. The corrected fixture retains 1,024 simultaneous waits, cancellation,
  relocated notification stacks, recycled IDs and the highest valid ID.
  Host/build checks use ASan/UBSan; the i386 QEMU check uses UBSan with alignment
  checks disabled for the original packed five-byte stack records. No production
  script/gameplay code was changed.
- Real Chrome/WebGL2 sessions connected to Carentan, Toujane and Dawnville in
  disposable local devmap rooms. GPU uniforms confirm original fog densities
  0.0001, 0.00015 and 0.00025 and each map's authored color. HUD programs do not
  have fog uniforms. Menus, team/weapon selection, movement, firing and reload
  were exercised; no JavaScript page errors or shader link failures occurred.
- An existing Emscripten attribute-reset issue still produces GL_INVALID_VALUE
  for constant attributes 16–18 on this GPU. It also occurs with original
  distances and fog disabled; it is recorded separately, not counted as a
  successful zero-error GPU check.
- Same-camera screenshots of the Carentan patio show grass retained beyond its
  former cutoff and the fence/tree keeping more detail. Screenshot files and
  raw measurements are under `out/fog-distance-qa/` (ignored/private).

## Fixed-camera performance sample

One visible Chrome client, 1280×720, antialiasing enabled, 120 Hz development
Mac. Four consecutive warmed 600-frame windows used baseline/candidate/
candidate/baseline settings at the same Carentan patio camera. Baseline means
fog disabled and `r_lodDistanceScale 1`; candidate means fog enabled and 1.5.

| Measurement | Baseline | Candidate |
| --- | ---: | ---: |
| Main-thread mean | 4.972 ms | 5.334 ms |
| GPU mean | 2.139 ms | 2.131 ms |
| FPS | 119.99 | 119.99 |
| Frame p95 | 8.858 ms | 8.845 ms |
| Frames above 33 ms | 0 | 0 |
| WASM memory | 512 MiB | 512 MiB |

Main-thread time rose 7.29%. The GPU difference is within
measurement noise. This single desktop scene is not a low-end hardware or
many-player benchmark. No extra fullscreen rendering passes, higher resolution
textures or server simulation changes are part of this change.

The release targets the ARM64 browser image on the Pi. The native server,
gateway and private game archives do not need a rebuild for these changes.
Manual deployment retains the previous browser image for rollback, requires
an idle gateway, and verifies the installed revision and WASM hash. Release
logs and HTTPS verification evidence are saved under the ignored
`out/fog-distance-release/` directory.
