#!/usr/bin/env bash
set -euo pipefail

export DISPLAY="${DISPLAY:-:1}"
export WINEPREFIX="${WINEPREFIX:-/data/wineprefix}"
export LIBGL_ALWAYS_SOFTWARE=1
export LP_NUM_THREADS="${LP_NUM_THREADS:-2}"

for required in /data/CoD2MP_s.exe /data/gfx_d3d_mp_x86_s.dll /data/mss32.dll; do
  if [[ ! -s "$required" ]]; then
    echo "Missing original multiplayer runtime file: $required" >&2
    echo "Run ./scripts/import-original-zip.sh first." >&2
    exit 1
  fi
done
if [[ ! -d /data/main ]] || ! compgen -G '/data/main/*.iwd' >/dev/null; then
  echo 'Missing Call of Duty 2 multiplayer IWD archives under /data/main.' >&2
  exit 1
fi

# Keep the host in a single small online TDM rotation: the stock Tunisia map.
cp /usr/local/share/cod2/server.cfg /data/main/server.cfg

mkdir -p /data/main/players/Player "$WINEPREFIX"
if [[ ! -s /data/main/players/active.txt ]]; then
  printf 'Player\n' > /data/main/players/active.txt
fi
if [[ ! -e /data/main/players/Player/config_mp.cfg ]]; then
  : > /data/main/players/Player/config_mp.cfg
fi
profile_config=/data/main/players/Player/config_mp.cfg
for setting in 'com_maxfps 30' 'r_fullscreen 0' 'r_swapInterval 1' 'r_aaSamples 1' 'r_anisotropy 1' 'r_picmip 1'; do
  name="${setting%% *}"
  value="${setting#* }"
  if grep -q "^seta ${name} " "$profile_config"; then
    sed -i "s/^seta ${name} .*/seta ${name} \"${value}\"/" "$profile_config"
  else
    printf 'seta %s "%s"\n' "$name" "$value" >> "$profile_config"
  fi
done
display_number="${DISPLAY#:}"
display_number="${display_number%%.*}"
rm -f "/tmp/.X${display_number}-lock" "/tmp/.X11-unix/X${display_number}"
Xvfb "$DISPLAY" -screen 0 1280x800x24 +extension GLX +render -noreset \
  >/data/xvfb.log 2>&1 &
XVFB_PID=$!
cleanup() {
  kill "${GAME_PID:-}" "${VNC_PID:-}" "$XVFB_PID" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

ready=0
for _ in $(seq 1 60); do
  if xdpyinfo -display "$DISPLAY" >/dev/null 2>&1; then ready=1; break; fi
  sleep 1
done
if [[ "$ready" != 1 ]]; then
  echo 'Xvfb did not become ready.' >&2
  exit 1
fi

if [[ ! -s "$WINEPREFIX/system.reg" ]]; then
  wineboot --init >/data/wineboot.log 2>&1
fi
x11vnc -display "$DISPLAY" -forever -shared -nopw -listen 127.0.0.1 \
  -rfbport 5900 >/data/x11vnc.log 2>&1 &
VNC_PID=$!

cd /data
wine ./CoD2MP_s.exe \
  +set fs_homepath /data \
  +set net_ip 0.0.0.0 \
  +set net_port 28960 \
  +set dedicated 0 \
  +set r_fullscreen 0 \
  +set r_mode 4 \
  +set com_hunkMegs 128 \
  +set com_introPlayed 1 \
  +set com_playerProfile Player \
  +set ui_playerProfileAlreadyChosen 1 \
  +set cl_punkbuster 0 \
  +exec server.cfg \
  +map mp_toujane \
  >/data/cod2.log 2>&1 &
GAME_PID=$!
echo "Call of Duty 2 Multiplayer started (pid $GAME_PID). Open http://localhost:8088."

websockify --web=/usr/share/novnc 6080 localhost:5900
