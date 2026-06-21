#!/bin/sh
# 打包 Linux AppImage:引擎 (SDL2) + free 自由資產 + CJK 字型。
# free 模組為 CC-BY-SA/GPL,可自由散布 → 玩家不需自備原版遊戲。
# 在 openkb-build-sdl2 image 內執行 (需先 build.sh + build-font.sh)。
set -e
cd "$(dirname "$0")/.."

[ -f openkb ]          || { echo "先 docker/build.sh 編譯 openkb"; exit 1; }
[ -f data/cjk24.bin ]  || { echo "先 docker/build-font.sh 烤 cjk24.bin"; exit 1; }

export DEBIAN_FRONTEND=noninteractive
apt-get update -qq >/dev/null 2>&1
apt-get install -y -qq --no-install-recommends wget file patchelf libfuse2 >/dev/null 2>&1 || \
  apt-get install -y -qq --no-install-recommends wget file patchelf >/dev/null 2>&1

AT=/tmp/appimagetool
if [ ! -x "$AT" ]; then
  wget -qO "$AT" "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage" || \
  wget -qO "$AT" "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"
  chmod +x "$AT"
fi

AD=/tmp/AppDir
SHARE=usr/share/open-king-bounty-cht
rm -rf "$AD"; mkdir -p "$AD/usr/bin" "$AD/usr/lib" "$AD/$SHARE"

cp openkb "$AD/usr/bin/"
cp -r data/free "$AD/$SHARE/free"
cp data/cjk24.bin "$AD/$SHARE/cjk24.bin"

# 個人版:設 KB_ORIGINAL_DOS=<含 256.CC 的目錄> 可把原版資料綁進包內,
# 啟動時自動偵測 → 原版 DOS 美術 + 中文。**含版權素材,僅供自己擁有正版者使用,
# 切勿散布此包。** 公開包不要設這個變數 (維持 free 美術)。
APPNAME="KingsBounty-CHT"
if [ -n "${KB_ORIGINAL_DOS:-}" ] && [ -f "$KB_ORIGINAL_DOS/256.CC" ]; then
  # KB.EXE 內含 troop 數值表 (DAT_RANGEMIN..DAT_GROWTH 等) 與 AdLib 曲;原版
  # KB.EXE 是 EXECOMP 壓縮,openkb 執行期不解壓 → 須離線先解壓,否則
  # 一堆 "Unable to resolve resource DAT_*" + tungrp 讀取失敗 (數值用預設值)。
  if [ -f "$KB_ORIGINAL_DOS/KB.EXE" ] && [ "$(wc -c < "$KB_ORIGINAL_DOS/KB.EXE")" -lt 100000 ]; then
    gcc -w "$(dirname "$0")/../src/tools/unexecomp.c" -o /tmp/unexecomp 2>/dev/null && \
    /tmp/unexecomp "$KB_ORIGINAL_DOS/KB.EXE" /tmp/KB.EXE.dec >/dev/null 2>&1 && \
    [ -s /tmp/KB.EXE.dec ] && cp /tmp/KB.EXE.dec "$KB_ORIGINAL_DOS/KB.EXE" && \
    echo "[build-appimage] KB.EXE 已解壓 (EXECOMP → 修復 DAT_* 數值表)"
  fi
  cp "$KB_ORIGINAL_DOS"/256.CC "$KB_ORIGINAL_DOS"/416.CC "$KB_ORIGINAL_DOS"/KB.EXE "$AD/$SHARE/" 2>/dev/null
  APPNAME="KingsBounty-CHT-original"
  echo "[build-appimage] 綁入原版 DOS 美術 (個人版,請勿散布)"
fi
# 個人版:綁入 Genesis ROM (kb.bin),F8 可循環到 Genesis 美術。版權素材,勿散布。
if [ -n "${KB_GENESIS_ROM:-}" ] && [ -f "$KB_GENESIS_ROM/kb.bin" ]; then
  cp "$KB_GENESIS_ROM/kb.bin" "$AD/$SHARE/kb.bin"
  APPNAME="KingsBounty-CHT-original"
  echo "[build-appimage] 綁入 Genesis ROM (個人版,請勿散布)"
fi
# 個人版:綁入 Amiga 美術 (已從 .adf unpack 的 GAME/ 散檔),F8 可循環到 Amiga 美術。版權素材,勿散布。
if [ -n "${KB_AMIGA_GAME:-}" ] && [ -f "$KB_AMIGA_GAME/tileseta" ]; then
  mkdir -p "$AD/$SHARE/amiga"
  cp "$KB_AMIGA_GAME"/* "$AD/$SHARE/amiga/" 2>/dev/null
  APPNAME="KingsBounty-CHT-original"
  echo "[build-appimage] 綁入 Amiga 美術 (個人版,請勿散布)"
fi
# 個人版:綁入各版本 BGM (music/<版本>/),啟動時 F9 可切換。版權素材,勿散布。
if [ -d music ] && ls music/*/scenes.ini >/dev/null 2>&1; then
  cp -r music "$AD/$SHARE/music"
  APPNAME="KingsBounty-CHT-original"
  echo "[build-appimage] 綁入背景音樂 (個人版,請勿散布)"
fi
# 引擎以 rootdir 找視窗 icon (icon_32x32.png);放進 bundled share 消除警告
[ -f data/icon_32x32.png ] && cp data/icon_32x32.png "$AD/$SHARE/icon_32x32.png"
[ -f data/free/icon_32x32.png ] && cp data/free/icon_32x32.png "$AD/openkb-cht.png" 2>/dev/null || \
  { [ -f data/icon_32x32.png ] && cp data/icon_32x32.png "$AD/openkb-cht.png"; }

# 蒐集非系統共用庫 (SDL2 等);GL/X11/libc 留給目標系統
for lib in $(ldd "$AD/usr/bin/openkb" | awk '/=>/{print $3}'); do
  case "$lib" in
    */libc.so*|*/libm.so*|*/libpthread*|*/libdl*|*/librt*|*/ld-linux*|*/libGL*|*/libGLX*|*/libX11*|*/libxcb*|*/libstdc++*|*/libgcc*) ;;
    *) cp -L "$lib" "$AD/usr/lib/" 2>/dev/null || true ;;
  esac
done
# SDL2_mixer 的 OGG 解碼庫是 runtime dlopen (ldd 抓不到) → 明確 bundle,
# 否則 Mix_Init(MIX_INIT_OGG) 失敗、背景音樂無聲。連 SDL2_mixer 本體一併確保。
for pat in libSDL2_mixer libvorbisfile libvorbisenc libvorbis libogg libFLAC libopus libopusfile libmpg123 libxmp libmodplug; do
  for f in /usr/lib/*/${pat}.so* /usr/lib/${pat}.so*; do
    [ -e "$f" ] && cp -L "$f" "$AD/usr/lib/" 2>/dev/null || true
  done
done

# AppRun:產生指向 bundled free 模組的設定,savedir 放使用者家目錄
cat > "$AD/AppRun" <<'EOF'
#!/bin/sh
HERE="$(dirname "$(readlink -f "$0")")"
DATA="$HERE/usr/share/open-king-bounty-cht"
CFGDIR="${XDG_CONFIG_HOME:-$HOME/.config}/open-king-bounty-cht"
SAVEDIR="${XDG_DATA_HOME:-$HOME/.local/share}/open-king-bounty-cht/saves"
mkdir -p "$CFGDIR" "$SAVEDIR"
CFG="$CFGDIR/openkb.ini"
cat > "$CFG" <<INI
[main]
datadir = $DATA
autodiscover = 0
[sdl]
sound = 1
fullscreen = 0
filter = normal2x
[module]
name = Free
type = free
path = $DATA/free/
INI
export LD_LIBRARY_PATH="$HERE/usr/lib:$LD_LIBRARY_PATH"
# Amiga 美術目錄 (若有綁入) → 引擎 auto-detect F8 可切 Amiga
[ -f "$DATA/amiga/tileseta" ] && export KB_AMIGA_GAME="$DATA/amiga"
# --rootdir 指向 bundled 資料,讓引擎找到 icon_32x32.png
exec "$HERE/usr/bin/openkb" -c "$CFG" --rootdir "$DATA" --savedir "$SAVEDIR" "$@"
EOF
chmod +x "$AD/AppRun"

cat > "$AD/open-king-bounty-cht.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=御封戰將 (King's Bounty CHT)
Exec=AppRun
Icon=openkb-cht
Categories=Game;
Terminal=false
EOF
[ -f "$AD/openkb-cht.png" ] || { printf 'P3\n16 16\n255\n' > /tmp/i.ppm; for y in $(seq 0 15); do for x in $(seq 0 15); do echo "200 90 20"; done; done >> /tmp/i.ppm; convert /tmp/i.ppm "$AD/openkb-cht.png" 2>/dev/null || cp /tmp/i.ppm "$AD/openkb-cht.png"; }

mkdir -p release
ARCH=x86_64 "$AT" --appimage-extract-and-run "$AD" "release/${APPNAME}-x86_64.AppImage" >/tmp/ai.log 2>&1 || { tail -20 /tmp/ai.log; exit 1; }
ls -la "release/${APPNAME}-x86_64.AppImage"
echo "[build-appimage] -> release/${APPNAME}-x86_64.AppImage"
