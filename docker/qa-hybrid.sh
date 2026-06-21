set +e
cd /src
OUT=docs/test/qa-hybrid
mkdir -p "$OUT"; rm -f "$OUT"/*.png
CFG=/tmp/hy.ini
cat > "$CFG" <<INI
[main]
datadir = /src/data
autodiscover = 0
[sdl]
sound = 0
filter = normal2x
[module]
name = Free
type = free
path = /src/data/free/
INI
Xvfb :99 -screen 0 1024x768x24 >/dev/null 2>&1 &
XV=$!
sleep 2
export DISPLAY=:99 SDL_VIDEODRIVER=x11
export KB_ORIGINAL_DOS=/work/dos-orig/kings-bounty
rm -rf /tmp/s; mkdir -p /tmp/s
timeout 90 /src/openkb -c "$CFG" --savedir /tmp/s >/tmp/hy.log 2>&1 &
G=$!
sleep 3
WIN=$(timeout 5 xdotool search --name openkb | head -1)
echo "WIN=$WIN"
s=0
shot(){ s=$((s+1)); timeout 8 import -window "$WIN" "$OUT/$(printf %02d $s)-$1.png" 2>/dev/null||echo "warn $1"; }
k(){ timeout 4 xdotool windowfocus "$WIN" 2>/dev/null; timeout 4 xdotool key --clearmodifiers "$1" 2>/dev/null; sleep "${2:-1}"; }
shot 00-module
k Return 2; shot 01-title
k Return 2; shot 02-credits
k Return 2; shot 03-charselect
k a 1.5; shot 04-pick
k h 0.3; k e 0.3; k r 0.3; k o 0.3; shot 05-typed
k Return 1.5; shot 06-difficulty
k Return 2.5; shot 07-worldmap
k Down 0.6; k Down 0.6; shot 08-move
k Right 0.6; k Right 0.6; shot 09-move2
k v 1.2; shot 10-status
k Escape 1; k a 1.2; shot 11-army
k Escape 1; shot 12-back
echo "=== DOS art log lines ==="; grep -i "original\|DOS\|256.CC" /tmp/hy.log | head
echo "=== core ==="; ls core* 2>/dev/null || echo "no core"
kill $G $XV 2>/dev/null
echo "shots=$s"
