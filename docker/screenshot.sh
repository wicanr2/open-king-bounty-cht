#!/bin/sh
# headless 跑 openkb (free 模組) 並擷取畫面到 docs/baseline/sdl2/。
# 在 openkb-build-sdl2 image 內執行 (含 Xvfb/imagemagick/xdotool)。
# 直接擷取 openkb 視窗 (SDL2 視窗置中,不能用 root+crop)。
set +e
cd "$(dirname "$0")/.."
OUT=docs/baseline/sdl2
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
WIN=$(xdotool search --name openkb | head -1)

shoot() { import -window "$WIN" "$OUT/$1.png" 2>/dev/null && echo "  saved $OUT/$1.png ($(stat -c %s "$OUT/$1.png")b)"; }

xdotool key --window "$WIN" Return; sleep 2   # 選 Free 模組 → 片頭/製作名單
shoot credits
xdotool key --window "$WIN" Return; sleep 1
xdotool key --window "$WIN" Return; sleep 1
shoot character-select

kill $GPID $XVFB 2>/dev/null
echo "=== 擷取完成 ==="
ls -la "$OUT"/*.png 2>/dev/null
