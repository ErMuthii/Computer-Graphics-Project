#!/usr/bin/env bash
# Real keyboard session for the desktop studio (manual check, not part of CTest).
# Owner: Andrew Karanja Gathirwa, 167144.
#
# Sends genuine X11 key events (xdotool) to a running kitenge_studio window,
# including held keys that rely on the OS auto-repeat, and saves the scene after
# each step with the application's own S export. Linux with Xvfb and xdotool:
#
#   sudo apt-get install xvfb xdotool
#   tests/real_key_session.sh build-desktop/kitenge_studio outputs/real-keys
#
# On a desktop session you can drop Xvfb by exporting DISPLAY first; the script
# starts its own virtual display only when DISPLAY is unset.
set -euo pipefail
BIN=${1:-build-desktop/kitenge_studio}
OUT=${2:-outputs/real-keys}
mkdir -p "$OUT"; rm -f "$OUT"/step*.ppm "$OUT"/live.ppm
XPID=""
if [ -z "${DISPLAY:-}" ]; then
    Xvfb :77 -screen 0 1400x900x24 >/dev/null 2>&1 & XPID=$!
    export DISPLAY=:77; sleep 1
fi
"$BIN" --export "$OUT/live.ppm" >"$OUT/app.log" 2>&1 & APP=$!
sleep 2
WID=$(xdotool search --name "Kitenge" | head -1)
xdotool windowfocus --sync "$WID"
n=0
snap() {   # export the current scene with S and keep a numbered copy
    sleep 0.4; xdotool key --window "$WID" s; sleep 0.5; n=$((n + 1))
    cp "$OUT/live.ppm" "$(printf '%s/step%02d_%s.ppm' "$OUT" "$n" "$1")"
}
hold() { xdotool keydown --window "$WID" "$1"; sleep "$2"; xdotool keyup --window "$WID" "$1"; }

snap start
xdotool key --window "$WID" m m m m m;            snap motif-bezier-arch
xdotool key --window "$WID" p;                    snap palette
xdotool key --window "$WID" q q;                  snap rotate-ccw-30
xdotool key --window "$WID" equal equal;          snap scale-1.50
xdotool key --window "$WID" x;                    snap mirror-x-axis
xdotool key --window "$WID" bracketright bracketright bracketright; snap pitch-wider
hold Right 1.2;                                   snap held-right-arrow
hold y 1.2;                                       snap held-y-toggles-once
xdotool key --window "$WID" a space; sleep 1.0;   snap playing-rotate
xdotool key --window "$WID" period period; sleep 0.6; snap faster
xdotool key --window "$WID" space;                snap paused
xdotool key --window "$WID" r;                    snap reset
xdotool key --window "$WID" Escape; sleep 0.8
if kill -0 "$APP" 2>/dev/null; then echo "FAIL: Esc did not close the window"; kill "$APP"; status=1
else echo "Esc closed the window"; status=0; fi
[ -n "$XPID" ] && kill "$XPID"
echo "Saved $n steps in $OUT"
exit $status
