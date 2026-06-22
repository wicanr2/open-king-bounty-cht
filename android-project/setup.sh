#!/bin/sh
# 組裝 openkb Android 專案:抓 SDL2/image/mixer 源 → 以 SDL2 android-project 為模板
# → 覆蓋 openkb overlay → 連結引擎源 → stage assets。之後 cd build && ./gradlew assembleDebug。
# 需求:curl、tar。實際編譯需 Android SDL/NDK (見 README)。
set -e
cd "$(dirname "$0")"
ROOT="$(cd .. && pwd)"          # repo 根
HERE="$(pwd)"                   # android-project/
BUILD="$HERE/build"
DL="$HERE/dl"

SDL_VER=2.30.9
IMG_VER=2.8.2
MIX_VER=2.8.0

mkdir -p "$DL"
fetch() { # url out
  [ -f "$2" ] || curl -fsSL "$1" -o "$2"
}
fetch "https://github.com/libsdl-org/SDL/releases/download/release-$SDL_VER/SDL2-$SDL_VER.tar.gz" "$DL/SDL2.tar.gz"
fetch "https://github.com/libsdl-org/SDL_image/releases/download/release-$IMG_VER/SDL2_image-$IMG_VER.tar.gz" "$DL/SDL2_image.tar.gz"
fetch "https://github.com/libsdl-org/SDL_mixer/releases/download/release-$MIX_VER/SDL2_mixer-$MIX_VER.tar.gz" "$DL/SDL2_mixer.tar.gz"

rm -rf "$BUILD"; mkdir -p "$BUILD"
tmp="$HERE/_src"; rm -rf "$tmp"; mkdir -p "$tmp"
tar xzf "$DL/SDL2.tar.gz" -C "$tmp"
tar xzf "$DL/SDL2_image.tar.gz" -C "$tmp"
tar xzf "$DL/SDL2_mixer.tar.gz" -C "$tmp"
SDL="$tmp/SDL2-$SDL_VER"

# 1. 以 SDL2 的 android-project 為模板 (gradle / wrapper / SDL java glue / jni 骨架)
cp -a "$SDL/android-project/." "$BUILD/"

# gradle 的 minSdkVersion 決定 ndkBuild 的 API level (會覆寫 Application.mk 的 APP_PLATFORM);
# seekdir/telldir 等 bionic 函式 API 23 才有 → 統一拉到 23。涵蓋多種寫法。
for g in "$BUILD/build.gradle" "$BUILD/app/build.gradle"; do
  [ -f "$g" ] && sed -i -E \
    -e 's/minSdkVersion[[:space:]]+[0-9]+/minSdkVersion 23/g' \
    -e 's/minSdk[[:space:]]+[0-9]+/minSdk 23/g' \
    -e 's/minSdk[[:space:]]*=[[:space:]]*[0-9]+/minSdk = 23/g' "$g"
done

# 2. 原生相依源放進 app/jni/ (各自帶 Android.mk)
JNI="$BUILD/app/jni"
rm -rf "$JNI/SDL"; cp -a "$SDL" "$JNI/SDL"
cp -a "$tmp/SDL2_image-$IMG_VER" "$JNI/SDL2_image"
cp -a "$tmp/SDL2_mixer-$MIX_VER" "$JNI/SDL2_mixer"

# SDL2_mixer:release tarball 未附 external codec 子專案 (wavpack/ogg/flac/mp3/mod/opus/mid),
# 其 Android.mk 會 include 不存在的路徑 → 關掉這些格式,只留 SDL 內建 WAV (無 external 相依,可編)。
# android v1 暫無 OGG 音樂;正式版再補 external codec。
[ -f "$JNI/SDL2_mixer/Android.mk" ] && \
  sed -i -E 's/^(SUPPORT_[A-Z0-9_]+[[:space:]]*)\??=[[:space:]]*true/\1:= false/' "$JNI/SDL2_mixer/Android.mk"
# SDL2_image:同理,只留 PNG (free 美術需要);若 PNG 也需 external 而缺,CI 會再報,屆時補 external/libpng。
[ -f "$JNI/SDL2_image/Android.mk" ] && \
  sed -i -E 's/^(SUPPORT_(JPG|WEBP|AVIF|TIF|JXL)[[:space:]]*)\??=[[:space:]]*true/\1:= false/' "$JNI/SDL2_image/Android.mk"

# 3. openkb overlay
APPMAIN="$BUILD/app/src/main"
mkdir -p "$APPMAIN/java/org/openkb/cht" "$APPMAIN/res/values" "$JNI/src/openkb"
cp "$HERE/overlay/AndroidManifest.xml" "$APPMAIN/AndroidManifest.xml"
cp "$HERE/overlay/OpenKBActivity.java" "$APPMAIN/java/org/openkb/cht/OpenKBActivity.java"
cp "$HERE/overlay/strings.xml"        "$APPMAIN/res/values/strings.xml"
cp "$HERE/overlay/Application.mk"     "$JNI/Application.mk"
cp "$HERE/overlay/Android.mk"         "$JNI/src/Android.mk"
cp "$HERE/overlay/cfg-android.h"      "$JNI/src/config.h"

# 4. 引擎源:先跑 fetch-vendor 產生 vendor/*.c (inprint/savepng/scale2x),
#    再「實體複製」src + vendor 進 jni/src/openkb (Android.mk 以 $(OKB)=openkb 參照)。
#    不用 symlink:gradle 把 jni 複製到 intermediates 時 symlink 會斷 → No rule to make target。
sh "$ROOT/docker/fetch-vendor.sh"
rm -rf "$JNI/src/openkb"; mkdir -p "$JNI/src/openkb/vendor"
cp -a "$ROOT/src" "$JNI/src/openkb/src"
cp "$ROOT/vendor/"*.c "$ROOT/vendor/"*.h "$JNI/src/openkb/vendor/" 2>/dev/null || true

# 5. stage assets:free 美術 + CJK atlas
ASSETS="$APPMAIN/assets/data"
mkdir -p "$ASSETS"
cp -a "$ROOT/data/free" "$ASSETS/free"
if [ -f "$ROOT/data/cjk24.bin" ]; then
  cp "$ROOT/data/cjk24.bin" "$ASSETS/cjk24.bin"
else
  echo "[setup] 警告:data/cjk24.bin 不存在,先跑 docker/build-font.sh 烤字型 atlas"
fi

rm -rf "$tmp"
echo "[setup] 完成 → $BUILD"
echo "[setup] 下一步:cd $BUILD && ./gradlew assembleDebug   (需 Android SDK + NDK)"
