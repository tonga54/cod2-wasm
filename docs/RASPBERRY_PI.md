# Raspberry Pi hosting

The Pi hosts the HTTP server, WebSocket gateway, room supervisor and game
simulation. Your computer runs the WebAssembly/WebGL client in its browser.
This deployment targets **64-bit Raspberry Pi OS / Debian on ARM64** and a
**4 KiB-page kernel**. It has been tested on a **Pi 5 with 8 GB RAM**, running
Debian 13.4. Other Pi models and smaller memory configurations have not been
validated. Use a wired LAN connection where practical.

The reconstructed engine relies on 32-bit x86 data layouts. The Pi image runs
only `cod2_lnxded` through `qemu-i386`; Node.js, Python and the rest of the stack
run natively on ARM64. The emulator is taken from Debian's
[statically linked qemu-user package](https://packages.debian.org/trixie/qemu-user).
It does not emulate a whole x86 operating system and does
not require global binfmt registration or a privileged container.

## What is included

| File | Purpose |
| --- | --- |
| [`downstream/server/Dockerfile.pi`](../downstream/server/Dockerfile.pi) | ARM64 Python supervisor and QEMU runtime for the i386 game engine |
| [`downstream/server/pi-engine.sh`](../downstream/server/pi-engine.sh) | Launches only the engine through `qemu-i386` |
| [`compose.pi.yaml`](../compose.pi.yaml) | Pi service platforms, images and restart policy; combines with `compose.yaml` |
| [`scripts/prepare-pi-runtime.sh`](../scripts/prepare-pi-runtime.sh) | Extracts the compiled engine and its exact i386 library dependencies |
| [`scripts/deploy-pi.sh`](../scripts/deploy-pi.sh) | Checks the Pi, transfers images/private assets, builds the Pi image and starts the stack over SSH |

The web and gateway use the existing Dockerfiles, built for ARM64. No separate
Pi Dockerfile is needed for those services. The game executable, browser WASM,
original game files and generated `out/pi-runtime/` are not committed: a Git
clone alone is not a runnable game package.

## 1. Prepare the Pi

Install a 64-bit OS and enable SSH for your account. The deployment account must
be able to run Docker without an interactive `sudo` prompt. If Docker is not
installed, follow [Docker's Debian installation guide](https://docs.docker.com/engine/install/debian/),
including the Compose plugin and its linked Linux post-installation steps.
The helper uses `docker compose`, not the older `docker-compose` command.

On the Pi, install the transfer tools and enable Docker at boot:

```sh
sudo apt-get update
sudo apt-get install -y git python3 rsync
sudo systemctl enable --now docker

uname -m                    # Expected: aarch64
getconf PAGESIZE            # Required: 4096; see the kernel section below
docker compose version
docker info --format '{{.Architecture}}'
df -h /                    # Helper requires at least 2 GiB free
hostname -I                # Find the Pi's LAN address
```

Allow at least 4 GiB of free space for image layers, build cache and updates;
the 2 GiB deployment check is a minimum. The Pi needs Internet access for the
repository, base images and packages during installation. Keep port **8088**
available for the game. From your build computer, verify access before building:

```sh
ssh USER@PI_LAN_IP 'uname -m; getconf PAGESIZE; docker compose version'
```

Replace `USER@PI_LAN_IP` throughout this guide with your own SSH account and
address, for example `alice@raspberrypi.local`. Verify a first-time SSH host-key
fingerprint against your Pi. No password, private key or machine-specific IP is
stored in the repository.

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

If you need that kernel change, first check the installed boot files and modules:

```sh
ls -lh /boot/firmware/kernel8.img /boot/firmware/initramfs8
ls /lib/modules
sudo cp -a /boot/firmware/config.txt /boot/firmware/config.txt.cod2-backup
sudo nano /boot/firmware/config.txt
```

Set `kernel=kernel8.img` under `[all]`, save, and reboot with `sudo reboot`.
Reconnect over SSH and run `getconf PAGESIZE` again. Do not select an image
without its matching modules/initramfs. On a fresh installation, the backup
above provides this rollback if the old configuration is needed:

```sh
sudo cp -a /boot/firmware/config.txt.cod2-backup /boot/firmware/config.txt
sudo reboot
```

## 2. Build on your computer

Follow the [README build instructions](../README.md#host-locally), including
preparing the owner's private files. Compile the game on the build machine;
the Pi does not need Emscripten, a compiler toolchain or the original full IWD
collection.

The build computer needs Git, Python 3.9+, Node.js, Docker with Compose, `ssh`,
`rsync`, `curl`, and enough disk space for the original files and build toolchains.
Docker must support running **both `linux/amd64` and `linux/arm64` containers**.
Docker Desktop on Apple Silicon provides this; for a Linux build host, configure
the appropriate emulation using [Docker's multi-platform build instructions](https://docs.docker.com/build/building/multi-platform/).
Host-side build dependencies and the checks in `build-docker.sh` still apply.

For a first installation, run these commands on the build computer:

```sh
git clone https://github.com/tonga54/cod2-wasm.git
cd cod2-wasm
git clone https://github.com/theodorecharles/wasm-game-framework.git ../wasm-game-framework

# Put your original CoD2 1.3 English IWD files in data/main/ first.
python3 scripts/prepare-browser-bootstrap.py
python3 scripts/check-private-assets.py

# Set COD2_ORIGINAL_ZIP to your original installation ZIP if necessary.
# Build only; do not start the standard amd64 stack on the Pi.
DOCKER_DEFAULT_PLATFORM=linux/arm64 ./scripts/build-docker.sh
```

This builds the WebAssembly browser package and ARM64 framework/web/gateway images.
The native engine build explicitly uses `linux/amd64`, independently of the
default platform. The deployment helper extracts its i386 binary and libraries
for use inside the ARM64 Pi container. The default delivery image tags are
`local/cod2-wasm:dev`, `local/cod2-gateway:dev` and
`local/cod2-native-server:dev`.

Use a clean checkout of the published `origin/master`. If you change source,
commit and push those changes before building; the helper requires that exact
published revision and checks that the web image was built from it. Keep the
private game files out of Git. Check the delivery platforms:

```sh
docker image inspect local/cod2-wasm:dev --format '{{.Architecture}}'
docker image inspect local/cod2-gateway:dev --format '{{.Architecture}}'
# Both must report arm64.
```

## 3. Deploy from your computer

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

For example, if you built the web image under another tag:

```sh
COD2_PI_WEB_IMAGE=local/cod2-wasm:pi-build \
  ./scripts/deploy-pi.sh USER@PI_LAN_IP cod2-wasm
```

The helper creates or fast-forwards the Pi's checkout, loads the web/gateway
images, builds `Dockerfile.pi` using the transferred `out/pi-runtime/`, and runs
`docker compose ... up -d --no-build`. It does not install Docker, change your
kernel, modify unrelated containers, or delete unrelated files. The image build
needs access to Debian/Ubuntu package repositories.

## 4. Play and verify

Open **http://PI_LAN_IP:8088/** from your computer. The first visit downloads
approximately 275.6 MB of private game files into that browser's cache. A new
Pi origin has a separate cache from a previous Mac URL.

The host starts without rooms. Create a named room using **Start New Server**,
select its map and mode, and use **Join Game** from another browser. Only HTTP
and WebSocket port 8088 are published; game UDP and room management stay inside
the Docker network. Private archives are mounted read-only.

From your computer, check the running stack:

```sh
curl -fsS http://PI_LAN_IP:8088/gateway/health
# Expected: gateway ready, clients 0 before anyone joins.
curl -fsS http://PI_LAN_IP:8088/servers
# Expected on a fresh start: rooms [].
curl -fsS http://PI_LAN_IP:8088/build-info.json
# Identifies the source revision and build content.
```

Create a room, join it, select a team/weapon, then join from a second browser to
check movement and combat. A successful Docker start alone does not verify a
playable multiplayer session.

## Manage the installation

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
An explicit `stop` keeps the services stopped until you run `up -d` again.

The prepared game assets live in `~/cod2-wasm/data/browser/`; the source checkout
and generated i386 runtime live alongside them. Preserve the original IWDs on
the build computer. Rooms live in temporary container storage and are not saved
matches.

## Updates and performance

On the build computer, fetch the published version, rebuild it, and deploy
while no players are connected:

```sh
cd cod2-wasm
git pull --ff-only origin master
DOCKER_DEFAULT_PLATFORM=linux/arm64 ./scripts/build-docker.sh
./scripts/deploy-pi.sh USER@PI_LAN_IP
```

Commit and push any intended source changes before building. Use this procedure
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

## Troubleshooting

| Symptom | Check / action |
| --- | --- |
| Helper reports a 16 KiB-page kernel, or the engine cannot load i386 libraries | Run `getconf PAGESIZE`; use the compatible 4 KiB kernel procedure above. Rebuilding the image does not change the host page size. |
| Web/gateway image reports `amd64` | Rebuild on the computer with `DOCKER_DEFAULT_PLATFORM=linux/arm64`; ensure its Docker engine can run ARM64 containers. |
| `permission denied` accessing Docker over SSH | Configure Docker access for the deployment account using Docker's post-installation guide, reconnect, and verify `docker info` without `sudo`. |
| Missing `out/pi-runtime/` when building `Dockerfile.pi` manually | Run `scripts/prepare-pi-runtime.sh` after building the standard native engine image, or use the deployment helper, which prepares/transfers it. |
| Missing original files or an asset hash mismatch | Supply your own original English 1.3 IWDs and rerun the preparation/check commands on the build computer. Public Git and Docker images contain no IWDs. |
| Cannot open the game | Check the Pi's current LAN IP, port 8088, and Compose status/logs. An address from another machine's installation will not apply to yours. |
| Initial load is slow | The first browser visit downloads the private package; wait for the byte progress. Later visits reuse that origin's cache. |
| Deployment refuses an update | Leave active matches, commit/push intended source changes, and resolve a dirty or divergent Pi checkout before retrying. |
| Simulation slows with more players | Check per-container CPU use, temperature, power supply and throttling. Measure your actual load; the configured slot/room limit is not measured Pi capacity. |
