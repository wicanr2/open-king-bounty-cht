#!/usr/bin/env bash
# 組裝 openkb 推廣影片:繁中標題卡 + 實機片段(原版美術+中文 / 乾淨戰鬥)+ 字幕 + 鋪 FM-Towns 音樂。
# 依賴:先跑 scripts/capture-promo.sh(產 release/cap_show.* + cap_combat.*)。在 openkb-capture 內跑。
# 產物:dist-all/video/openkb-cht-promo.mp4
set -u
cd "$(dirname "$0")/.."
FONT=/usr/share/fonts/opentype/noto/NotoSansCJK-Bold.ttc
W=1280; H=720; FPS=25
BG='#0a0e1a'; GOLD='#f0c000'; WHITE='#eaeaea'; CAP='#f5d020'
SV=release; OUT=dist-all/video; mkdir -p "$OUT"
TMP="$(mktemp -d)"
[ -f "$SV/cap_show.mp4" ] || { echo "缺 $SV/cap_show.mp4,先跑 capture-promo.sh"; exit 1; }

# 全螢幕卡片
card(){ convert -size ${W}x${H} xc:"$BG" -font "$FONT" -gravity center \
  -fill "$GOLD" -pointsize 72 -annotate +0-60 "$2" \
  -fill "$WHITE" -pointsize 36 -annotate +0+70 "$3" "$1"; }
# 底部字幕條(透明 PNG,疊在片段上)
capstrip(){ convert -size ${W}x110 xc:'#000000B0' -font "$FONT" -gravity center \
  -fill "$CAP" -pointsize 38 -annotate +0+0 "$2" "$1"; }
# 卡片→片段(D 秒,淡入淡出)+ 一段音樂
cardseg(){ # $1 png $2 out $3 dur $4 wav $5 ss
  ffmpeg -y -loglevel error -loop 1 -t "$3" -i "$1" -ss "$5" -t "$3" -i "$4" \
    -vf "fps=$FPS,format=yuv420p,fade=t=in:st=0:d=0.5,fade=t=out:st=$(awk "BEGIN{print $3-0.5}"):d=0.5" \
    -af "afade=t=in:st=0:d=0.5,afade=t=out:st=$(awk "BEGIN{print $3-0.6}"):d=0.6,volume=0.85" \
    -c:v libx264 -pix_fmt yuv420p -c:a aac -ar 44100 -ac 2 -shortest "$2"; }
# 實機片段→標準化(縮放置中 letterbox + 底部字幕 + 該段音樂)
gameseg(){ # $1 mp4 $2 wav $3 capstrip.png $4 out
  ffmpeg -y -loglevel error -i "$1" -i "$2" -i "$3" \
    -filter_complex "[0:v]scale=${W}:${H}:force_original_aspect_ratio=decrease,pad=${W}:${H}:-1:-1:color=black,fps=$FPS[v0];[v0][2:v]overlay=0:H-h-30[v]" \
    -map "[v]" -map 1:a -c:v libx264 -pix_fmt yuv420p -c:a aac -ar 44100 -ac 2 -shortest "$4"; }

echo "== 卡片 =="
card "$TMP/t.png" '御封戰將' 'King'"'"'s Bounty (1990)　繁體中文化　Windows · macOS · Linux'
card "$TMP/e.png" '免費下載' 'github.com/wicanr2/open-king-bounty-cht'
capstrip "$TMP/cs_show.png"   '原版美術 ＋ 全程繁體中文化　·　F8 切換 DOS / Genesis / Amiga 美術'
capstrip "$TMP/cs_combat.png" '戰鬥畫面修復　·　介面乾淨無殘留'

echo "== 片段 =="
cardseg "$TMP/t.png" "$TMP/seg_t.mp4" 3.5 "$SV/cap_show.wav" 2
gameseg "$SV/cap_show.mp4"   "$SV/cap_show.wav"   "$TMP/cs_show.png"   "$TMP/seg_show.mp4"
gameseg "$SV/cap_combat.mp4" "$SV/cap_combat.wav" "$TMP/cs_combat.png" "$TMP/seg_combat.mp4"
cardseg "$TMP/e.png" "$TMP/seg_e.mp4" 3.5 "$SV/cap_show.wav" 14

echo "== concat =="
L="$TMP/list.txt"; : > "$L"
for s in seg_t seg_show seg_combat seg_e; do echo "file '$TMP/$s.mp4'" >> "$L"; done
ffmpeg -y -loglevel error -f concat -safe 0 -i "$L" -c:v libx264 -pix_fmt yuv420p -crf 20 -c:a aac -b:a 192k -movflags +faststart "$OUT/openkb-cht-promo.mp4"
rm -rf "$TMP"
echo "promo -> $OUT/openkb-cht-promo.mp4 ($(ffprobe -v error -show_entries format=duration -of csv=p=0 "$OUT/openkb-cht-promo.mp4" 2>/dev/null)s)"
