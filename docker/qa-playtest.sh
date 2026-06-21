#!/bin/sh
# QA 實機驗證:正常玩家路徑逐步截圖。所有 import/xdotool 帶 timeout,openkb 用 timeout 包,絕不掛機。
set +e
cd "$(dirname "$0")/.."
OUT=docs/test/qa
mkdir -p "$OUT"
rm -f "$OUT"/*.png

MODE="${1:-free}"   # free 或 hybrid
CFG=/tmp/okb_qa.ini
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
SAVE=/tmp/s
rm -rf "$SAVE"; mkdir -p "$SAVE"

if [ "$MODE" = "hybrid" ]; then
  export KB_ORIGINAL_DOS=/work/dos-orig/kings-bounty
  echo "HYBRID mode: KB_ORIGINAL_DOS=$KB_ORIGINAL_DOS"
fi

# 整體上限 120s 自動結束
timeout 120 ./openkb -c "$CFG" --savedir "$SAVE" >/tmp/qa_playtest.log 2>&1 &
GPID=$!
sleep 3
WIN=$(timeout 5 xdotool search --name openkb | head -1)
echo "WIN=$WIN"
timeout 5 xdotool windowactivate --sync "$WIN" 2>/dev/null
timeout 5 xdotool windowfocus --sync "$WIN" 2>/dev/null

step=0
shot() { step=$((step+1)); timeout 8 import -window "$WIN" "$OUT/$(printf %02d $step)-$1.png" 2>/dev/null || echo "  [warn] shot $1 timed out"; }
key()  { timeout 5 xdotool windowfocus "$WIN" 2>/dev/null; timeout 5 xdotool key --clearmodifiers "$1" 2>/dev/null; sleep "${2:-1}"; }

shot 00-module-select
key Return 2;   shot 01-title-or-intro
key Return 2;   shot 02-credits
key Return 2;   shot 03-char-select
# 選武士 (a)
key a 1.5;      shot 04-after-pick-knight
# 打字輸入名字 hero -- 逐字截圖驗證打字中文是否殘影
key h 0.4;      shot 05-typing-h
key e 0.4;      shot 06-typing-he
key r 0.4;      shot 07-typing-her
key o 0.4;      shot 08-typing-hero
key Return 1.5; shot 09-difficulty
key Return 2.5; shot 10-world-map
shot 11-world-map-again
# 移動 (往南/東遠離城堡,避免卡在城堡選單)
key Down 0.6;  key Down 0.6; shot 12-move-down
key Right 0.6; key Right 0.6; shot 13-move-right
key Left 0.6;  key Left 0.6; shot 14-move-left
key Down 0.6;  key Down 0.6; shot 15-move-down2
shot 16-on-open-map
key Right 0.5; key Right 0.5;                   shot 17-explore-more
# v 角色狀態 (在地圖上)
key v 1.2;      shot 18-char-status
key Escape 1;   shot 19-after-esc-from-status
# a 部隊
key a 1.2;      shot 20-army-view
key Escape 1;   shot 21-after-esc-from-army
# c 操作說明
key c 1.2;      shot 22-controls
key Escape 1;   shot 23-after-esc-from-controls
# F10 離開確認 (應出現 自動存檔 + Y/N,不應是 Debug)
key F10 1.5;    shot 24-f10-quit-dialog
key n 1.5;      shot 25-after-N-should-continue
# 再次 F10 + Esc 應取消 (不退出)
key F10 1.5;    shot 26-f10-again
key Escape 1.5; shot 27-after-esc-on-quit-dialog
shot 28-final-state
# F12 才是 debug cheat
key F12 1.2;    shot 29-f12-cheat
key Escape 1;   shot 30-after-esc-cheat

kill $GPID 2>/dev/null
wait $GPID 2>/dev/null
EXITC=$?
kill $XVFB 2>/dev/null
echo "=== qa playtest ($MODE) 完成: $step 張 ==="
echo "openkb exit code: $EXITC"
echo "--- savedir 內容 ---"; ls -la "$SAVE" 2>/dev/null
echo "--- core dump 檢查 ---"; ls -la core* 2>/dev/null || echo "no core dump"
echo "--- log 末尾 ---"; tail -15 /tmp/qa_playtest.log
