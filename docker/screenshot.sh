#!/bin/sh
# headless 跑 openkb (free 模組) 並擷取數張畫面到 docs/baseline/。
# 用於 SDL1.2 baseline 紀錄;需在 openkb-build image 內執行 (含 Xvfb/imagemagick/xdotool)。
set +e
cd "$(dirname "$0")/.."
OUT=docs/baseline
mkdir -p "$OUT"

CFG=/tmp/okb.ini
cat > "$CFG" <<EOF
[main]
datadir = $PWD/data
autodiscover = 0
[sdl]
sound = 0
filter = normal2x
[module]
name = Free
type = free
path = $PWD/data/free/
EOF

Xvfb :99 -screen 0 1024x768x24 >/dev/null 2>&1 &
XVFB=$!
sleep 2
export DISPLAY=:99 SDL_VIDEODRIVER=x11

./openkb -c "$CFG" --savedir /tmp > /tmp/openkb-run.log 2>&1 &
GPID=$!
sleep 3

shoot() { import -window root -crop 640x400+0+0 +repage "$OUT/$1.png" 2>/dev/null && echo "  saved $OUT/$1.png ($(stat -c %s "$OUT/$1.png") bytes)"; }

# 模組選單 → 按 Enter 選 Free
xdotool key Return 2>/dev/null; sleep 2
shoot 01-title           # Royal Reward 片頭
xdotool key Return 2>/dev/null; sleep 1
shoot 02-after-title
xdotool key Return 2>/dev/null; sleep 1
shoot 03-menu
xdotool key space 2>/dev/null; sleep 1
shoot 04-next

kill $GPID $XVFB 2>/dev/null
echo "=== baseline 擷取完成 ==="
ls -la "$OUT"/*.png 2>/dev/null
