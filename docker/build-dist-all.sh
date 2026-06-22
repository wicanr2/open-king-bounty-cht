#!/bin/sh
# build-dist-all.sh — 組裝「完整版」三平台包到 dist-all/ (host 端執行,非容器)。
#
# 完整版 = 引擎 + 全套美術 (free + DOS 256.CC/416.CC/KB.EXE + Genesis kb.bin
#          + Amiga GAME) + FM-Towns 背景音樂。F8 切美術主題、F9 切/開關音樂。
# ★含版權素材 (原版遊戲資料),僅供自己擁有正版者使用,切勿散布;dist-all/ 已 gitignore。★
#
# 來源:
#   - 完整資料 payload:既有「完整版 AppImage」內已解壓好的 $SHARE (單一真值來源)
#   - 引擎 binary:GitHub Actions CI 產物 (release/ci/,free 版引擎,與資料無關)
# 引擎邏輯不因資料而異 → 把同一份 payload 注入 win/mac CI 包即得完整版。
set -e
cd "$(dirname "$0")/.."
ROOT="$(pwd)"

APPIMAGE="release/KingsBounty-CHT-original-x86_64.AppImage"   # 既有完整版 AppImage
CIDIR="release/ci"                                            # gh run download 下載處
OUT="dist-all"

[ -f "$APPIMAGE" ] || { echo "缺 $APPIMAGE (先用 KB_ORIGINAL_DOS/KB_GENESIS_ROM/KB_AMIGA_GAME + music/ 跑 build-appimage.sh)"; exit 1; }
[ -d "$CIDIR/KingsBounty-CHT-windows" ] || { echo "缺 $CIDIR/KingsBounty-CHT-windows (先 gh run download)"; exit 1; }
[ -f "$CIDIR/KingsBounty-CHT-macos/KingsBounty-CHT-macos.zip" ] || { echo "缺 macOS CI zip"; exit 1; }

rm -rf "$OUT"; mkdir -p "$OUT"
WORK="$(mktemp -d)"; trap 'rm -rf "$WORK"' EXIT

# 1) 從完整版 AppImage 取出已解壓的完整資料 payload ($SHARE)
echo "[1/4] 抽取完整資料 payload …"
( cd "$WORK" && "$ROOT/$APPIMAGE" --appimage-extract >/dev/null 2>&1 )
PAYLOAD="$WORK/squashfs-root/usr/share/open-king-bounty-cht"
[ -f "$PAYLOAD/256.CC" ] && [ -f "$PAYLOAD/kb.bin" ] && [ -f "$PAYLOAD/amiga/tileseta" ] && [ -d "$PAYLOAD/music" ] \
  || { echo "payload 不完整 (缺 DOS/Genesis/Amiga/music)"; exit 1; }
echo "      DOS: $(ls -1 "$PAYLOAD"/*.CC | wc -l) CC + KB.EXE | Genesis: kb.bin | Amiga: $(ls -1 "$PAYLOAD/amiga" | wc -l) | music: $(ls -1 "$PAYLOAD"/music/*/*.ogg 2>/dev/null | wc -l) ogg"

# 2) Linux AppImage — 既有完整版直接收進 dist-all
echo "[2/4] Linux AppImage …"
cp "$APPIMAGE" "$OUT/KingsBounty-CHT-full-linux-x86_64.AppImage"

# 3) Windows — CI 包 (含 exe + DLL) + 注入完整 data + 改 play.bat 帶 Amiga/音樂 env
echo "[3/4] Windows x64 …"
WIN="$WORK/win/KingsBounty-CHT-full-windows-x64"
mkdir -p "$WIN"
cp -r "$CIDIR/KingsBounty-CHT-windows/." "$WIN/"
rm -rf "$WIN/data"; mkdir -p "$WIN/data"
cp -r "$PAYLOAD/." "$WIN/data/"            # data/ = 完整 payload (free + 全美術 + music)
# rootdir=data;Amiga 走 KB_AMIGA_GAME、音樂走 KB_MUSIC (亦會自動找 data\music)
printf '@echo off\r\ncd /d "%%~dp0"\r\nset KB_AMIGA_GAME=%%~dp0data\\amiga\r\nset KB_MUSIC=%%~dp0data\\music\r\nopenkb.exe -c openkb.ini --rootdir data %%*\r\n' > "$WIN/play.bat"
( cd "$WORK/win" && zip -qry "$ROOT/$OUT/KingsBounty-CHT-full-windows-x64.zip" "KingsBounty-CHT-full-windows-x64" )

# 4) macOS — 解 CI .app zip + 注入完整 data + 改 launch 帶 Amiga/音樂 env
echo "[4/4] macOS .app …"
MAC="$WORK/mac"; mkdir -p "$MAC"
( cd "$MAC" && unzip -q "$ROOT/$CIDIR/KingsBounty-CHT-macos/KingsBounty-CHT-macos.zip" )
APP="$(find "$MAC" -maxdepth 1 -name '*.app' | head -1)"
[ -n "$APP" ] || { echo "macOS zip 內找不到 .app"; exit 1; }
RES="$APP/Contents/Resources"
rm -rf "$RES/data"; mkdir -p "$RES/data"
cp -r "$PAYLOAD/." "$RES/data/"
cat > "$APP/Contents/MacOS/launch" <<'SH'
#!/bin/sh
DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR/../Resources"
export KB_AMIGA_GAME="$PWD/data/amiga"
export KB_MUSIC="$PWD/data/music"
exec "$DIR/openkb" -c openkb.ini --rootdir data "$@"
SH
chmod +x "$APP/Contents/MacOS/launch"
( cd "$MAC" && zip -qry --symlinks "$ROOT/$OUT/KingsBounty-CHT-full-macOS.zip" "$(basename "$APP")" )

# 5) 說明檔 (隨包,提醒勿散布)
cat > "$OUT/README.txt" <<'TXT'
御封戰將 (King's Bounty) 繁體中文化 — 完整版

本資料夾三個包都含「全套美術 + 背景音樂」:
  - 美術主題 (遊戲中按 F8 循環): Free 自由美術 / DOS / Genesis / Amiga
  - 背景音樂 (按 F9 切換/開關): FM-Towns

平台:
  - KingsBounty-CHT-full-linux-x86_64.AppImage   Linux,chmod +x 後直接執行
  - KingsBounty-CHT-full-windows-x64.zip         Windows,解壓後執行 play.bat
  - KingsBounty-CHT-full-macOS.zip               macOS,解壓得 .app
        首次開啟若被 Gatekeeper 擋:右鍵→開啟,或
        xattr -dr com.apple.quarantine KingsBounty-CHT.app

★ 完整版含原版遊戲版權素材 (DOS/Genesis/Amiga 美術 + FM-Towns 音樂),
  僅供「擁有正版者」個人使用,請勿公開散布。
  公開散布請用 GitHub Release 的 free 版 (僅自由授權美術)。
TXT

echo
echo "===== dist-all/ ====="
ls -lh "$OUT"
