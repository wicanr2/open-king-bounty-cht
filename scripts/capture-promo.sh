#!/usr/bin/env bash
# 擷取 openkb 推廣影片素材(headless:Xvfb + ffmpeg x11grab 畫面 + SDL disk 音訊擷 FM-Towns 音樂)。
# 在 openkb-capture image 內跑(需 ffmpeg)。有界:固定秒數 SIGKILL,無 sentinel、無 GUI(rule 35)。
# 用完整資料(release/promodata:free+DOS+Genesis+Amiga+music)→ 可秀 F8 四主題 + 音樂。
# 產物:release/cap_show.mp4/.wav(showcase)、release/cap_combat.mp4/.wav(戰鬥修復)
set -u
cd "$(dirname "$0")/.."
BIN=./openkb
DATA=/src/release/promodata
[ -d release/promodata ] || { echo "缺 release/promodata"; exit 1; }
printf "[main]\ndatadir = %s\nautodiscover = 0\n[sdl]\nsound = 1\nfilter = normal2x\n[module]\nname = Free\ntype = free\npath = free/\n" "$DATA" > /tmp/promo.ini

cap() { # $1=name $2=secs $3=extra_env  (driver function 'drive' 已定義)
  local name="$1" secs="$2" env="$3"
  local DISP=:92 RAW=/tmp/$name.raw VID=release/$name.mp4 WAV=release/$name.wav
  rm -f "$RAW" "$VID" "$WAV"
  Xvfb $DISP -screen 0 1280x800x24 >/dev/null 2>&1 & local XV=$!; sleep 2
  env DISPLAY=$DISP SDL_AUDIODRIVER=disk SDL_DISKAUDIOFILE="$RAW" SDL_DISKAUDIODELAY=10 \
      KB_MUSIC="$DATA/music" KB_AMIGA_GAME="$DATA/amiga" $env \
      timeout -s KILL $((secs+8)) "$BIN" -c /tmp/promo.ini --rootdir "$DATA" >/tmp/$name.log 2>&1 & local GP=$!
  sleep 3
  export DISPLAY=$DISP
  WIN=$(xdotool search --name openkb 2>/dev/null | head -1)
  # 視窗幾何 → 擷取該區(去黑邊)
  eval $(xdotool getwindowgeometry --shell "$WIN" 2>/dev/null)
  local GEO="${WIDTH:-640}x${HEIGHT:-400}"; local OFF="+${X:-0},${Y:-0}"
  echo "[$name] window $WIN geo $GEO off $OFF"
  drive "$WIN" &   # 背景驅動(時間軸對齊 ffmpeg 0s)
  ffmpeg -y -loglevel error -f x11grab -video_size "$GEO" -framerate 25 -i "$DISP$OFF" \
    -t "$secs" -c:v libx264 -pix_fmt yuv420p -crf 18 "$VID"
  kill -KILL $GP 2>/dev/null; kill $XV 2>/dev/null; sleep 1
  ffmpeg -y -loglevel error -f s16le -ar 44100 -ac 2 -i "$RAW" "$WAV" 2>/dev/null
  echo "[$name] -> $VID ($(ffprobe -v error -show_entries format=duration -of csv=p=0 "$VID" 2>/dev/null)s) + $WAV"
}

K(){ xdotool key --window "$1" "$2" 2>/dev/null; }

# ---- Scene 0: 原版開場(停留 NWC 商標 → 原版 King's Bounty 標題)----
drive(){ local W="$1"
  sleep 3.2                  # New World Computing Presents 商標停留
  K "$W" Return; sleep 4.5   # → 原版 King's Bounty 標題停留
}
cap cap_intro 8 ""

# ---- Scene A: showcase(進地圖 → F8 循環四主題)----
drive(){ local W="$1"
  sleep 1; for n in 1 2 3 4; do K "$W" Return; sleep 0.7; done   # 快速跳過 商標/標題/製作群 → char select
  K "$W" a; sleep 0.8                                            # 騎士
  for c in y u f e n g; do K "$W" "$c"; sleep 0.2; done          # 名字 "yufeng"(隨意)
  K "$W" Return; sleep 1; K "$W" Return; sleep 2.5               # 難度→進地圖
  K "$W" Right; K "$W" Down; sleep 1.5                           # 動一下
  for t in 1 2 3 4; do K "$W" F8; sleep 2.6; done               # F8 循環 DOS/Genesis/Amiga/free
}
cap cap_show 26 ""

# ---- Scene B: 戰鬥修復(KB_DEBUG_COMBAT 直接切入)----
drive(){ local W="$1"; sleep 5; }   # 戰鬥畫面自動呈現,停留即可
cap cap_combat 9 "KB_DEBUG_COMBAT=1"
echo "=== 擷取完成 ==="
for f in cap_intro cap_show cap_combat; do
  echo "  $f: $(ffmpeg -hide_banner -i release/$f.wav -af volumedetect -f null /dev/null 2>&1 | grep mean_volume | grep -oE '\-?[0-9.]+ dB' | head -1) 音量"
done
