#!/bin/sh
# headless 劇本式試玩:驅動遊戲走過主要畫面並逐步截圖到 docs/test/shots/。
# 供 game-tester 判讀 CJK 顯示、破版、缺字、崩潰。
# 在 openkb-build-sdl2 image 內執行 (需 openkb + data/cjk24.bin)。
# 所有 import/xdotool 都帶 timeout,避免 ImageMagick/X 卡死整個容器 (rule 35)。
set +e
cd "$(dirname "$0")/.."
OUT=docs/test/shots
mkdir -p "$OUT"
rm -f "$OUT"/*.png

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
mkdir -p /tmp/kbsave
# 整體上限:openkb 最多跑 90s 自動結束,絕不掛機
timeout 90 ./openkb -c "$CFG" --savedir /tmp/kbsave >/tmp/playtest.log 2>&1 &
GPID=$!
sleep 3
WIN=$(timeout 5 xdotool search --name openkb | head -1)
timeout 5 xdotool windowactivate --sync "$WIN" 2>/dev/null
timeout 5 xdotool windowfocus --sync "$WIN" 2>/dev/null

step=0
shot() { step=$((step+1)); timeout 8 import -window "$WIN" "$OUT/$(printf %02d $step)-$1.png" 2>/dev/null || echo "  [warn] shot $1 timed out"; }
# 用 XTEST 真實輸入送到 focused 視窗 (char-select 等字母鍵需要;XSendEvent 合成事件 SDL2 可能忽略)
key()  { timeout 5 xdotool windowfocus "$WIN" 2>/dev/null; timeout 5 xdotool key --clearmodifiers "$1" 2>/dev/null; sleep "${2:-1}"; }

key Return 2;   shot module-to-title
key Return 2;   shot intro
key Return 2;   shot credits
key Return 2;   shot char-select
key a 2;        shot pick-knight
# 輸入名字 (XTEST 真實輸入) 後 Enter
key h 0.3; key e 0.3; key r 0.3; key o 0.3
key Return 1.5; shot named
key Return 1.5; shot difficulty
key Return 2.5; shot game-start
shot world-map
# 移動探索 (走向北方建築),嘗試觸發進城/戰鬥
key Up 0.6; key Up 0.6; key Up 0.6; shot move-up
key Up 0.6; key Up 0.6; shot move-up2
key Left 0.6; key Right 0.6; shot move-side
# 各種選單/互動鍵
key c 1;        shot key-c
key Escape 0.6; key v 1;        shot army-view
key Escape 0.6; key m 1;        shot view-army-menu
key Escape 0.6; key Return 1;   shot interact
key Escape 0.6; shot after-esc

kill $GPID $XVFB 2>/dev/null
wait 2>/dev/null
echo "=== playtest 截圖完成 ($step 張) ==="
ls "$OUT"/*.png 2>/dev/null | wc -l
echo "--- log 末尾 ---"; tail -5 /tmp/playtest.log
