# Raspberry Pi hosting

The Pi hosts the HTTP server, WebSocket gateway, room supervisor and game
simulation. Your computer runs the WebAssembly/WebGL client in its browser.
This deployment targets **64-bit Raspberry Pi OS / Debian on ARM64**, with Docker
and its Compose plugin already installed, and a **4 KiB-page kernel**. A Pi 5
with 8 GB RAM is the current validation target. Use a wired LAN connection where practical.

The reconstructed engine relies on 32-bit x86 data layouts. The Pi image runs
only `cod2_lnxded` through `qemu-i386`; Node.js, Python and the rest of the stack
run natively on ARM64. The emulator is taken from Debian's
[statically linked qemu-user package](https://packages.debian.org/trixie/qemu-user).
It does not emulate a whole x86 operating system and does
not require global binfmt registration or a privileged container.

## Kernel compatibility on Pi 5

Check `getconf PAGESIZE` on the Pi. This runtime requires `4096`. The Pi 5's
usual `kernel_2712.img` uses 16 KiB pages; the packaged i386 shared libraries
failed to load under that configuration during deployment testing.

Raspberry Pi's [official kernel documentation](https://www.raspberrypi.com/documentation/computers/linux_kernel.html)
confirms that `kernel8.img` also supports the Pi 5 and uses 4 KiB pages. If that
image, its matching modules and `initramfs8` are installed, back up
`/boot/firmware/config.txt`, set `kernel=kernel8.img` in its `[all]` section,
and reboot. Verify `getconf PAGESIZE` returns `4096` before deploying. This is a
host-wide kernel choice, so other services also restart during the reboot.
Restoring the saved configuration and rebooting restores the prior selection.
The deployment helper checks page size and does not modify boot configuration.

## Prepare on your build machine

Follow the [README build instructions](../README.md#host-locally), including
preparing the owner's private files. Compile the game on the build machine;
the Pi does not need Emscripten, a compiler toolchain or the original full IWD
collection. Keep source changes committed and pushed to `origin/master`.

The web and gateway images must be ARM64. Builds on an Apple Silicon Mac already
produce these ARM64 images. On another build host, build the framework/web and
gateway images for `linux/arm64` before running the helper. The dedicated engine
image remains the standard amd64 image.

For a clean source revision, refresh the compiled site's version metadata and
build the delivery images:

```sh
python3 scripts/write-build-info.py out/cod2-wasm-core/site
docker build -t local/cod2-wasm:dev .
docker build -t local/cod2-gateway:dev downstream/gateway
```

The helper validates the web image's revision/dirty status and archive hashes,
extracts the i386 engine with its exact shared libraries, then copies the small
runtime and private two-map package. It reuses ARM64 web/gateway image layers
and builds the Pi's supervisor/emulator image on the Pi.

```sh
./scripts/deploy-pi.sh USER@PI_LAN_IP
```

SSH uses your existing authentication and host verification. The helper reuses one SSH connection and stores no passwords. The destination defaults to `~/cod2-wasm`; pass a second directory
name to change it. It refuses an unrelated/dirty checkout, a non-ARM64 machine,
less than 2 GiB free space, or an existing game gateway with connected players.
Environment overrides:

- `COD2_PI_WEB_IMAGE` and `COD2_PI_GATEWAY_IMAGE`: alternative ARM64 image tags.
- `COD2_PI_ENGINE_IMAGE`: the already built amd64 engine image.
- `COD2_PI_SSH_CONTROL`: an existing SSH multiplexing socket.
- `COD2_PI_KNOWN_HOSTS`: a task-specific verified host-key file.

## Play and manage

Open **http://PI_LAN_IP:8088/** from your computer. The first visit downloads
approximately 275.6 MB of private game files into that browser's cache. A new
Pi origin has a separate cache from a previous Mac URL.

The host starts without rooms. Create a named room using **Start New Server**,
select its map and mode, and use **Join Game** from another browser. Only HTTP
and WebSocket port 8088 are published; game UDP and room management stay inside
the Docker network. Private archives are mounted read-only.

On the Pi:

```sh
cd ~/cod2-wasm

docker compose -p cod2-wasm-pi -f compose.yaml -f compose.pi.yaml ps
docker compose -p cod2-wasm-pi -f compose.yaml -f compose.pi.yaml logs --tail=100
docker compose -p cod2-wasm-pi -f compose.yaml -f compose.pi.yaml stop
# Start again without rebuilding:
docker compose -p cod2-wasm-pi -f compose.yaml -f compose.pi.yaml up -d --no-build
```

All three services restart automatically after a host reboot. Game rooms are
transient, so users create them again after a restart. The Compose project is
`cod2-wasm-pi`, separate from unrelated services on the same Pi.

## Updates and performance

Build and push the new committed version on the build machine, then run the
same deployment helper again while no players are connected. Use this procedure
instead of `update-local.py --apply`, which builds the standard server layout.

Client rendering quality and assets remain identical to the standard deployment.
The server retains the original 20 Hz simulation. Long sightlines use
segment/nearest-hit rejection and shared-brush deduplication
to avoid repeatedly solving collisions across the map. This also applies to
the browser client and keeps the original geometry and contact margins.
QEMU adds CPU cost; configured 64-player slots and three rooms do not establish
that the Pi can sustain that
load. Check `docker stats`, temperature and response/frame timings with your
actual player count. The browser `?perfDebug=1` overlay measures client rendering,
not the host's simulation cost. Some Pi kernels disable memory cgroups; Docker
then ignores `mem_limit` while keeping CPU quotas. Check `docker info` on the
actual host rather than assuming the requested memory cap is enforced.
