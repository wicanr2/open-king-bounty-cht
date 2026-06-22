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

# 2. 原生相依源放進 app/jni/ (各自帶 Android.mk)
JNI="$BUILD/app/jni"
rm -rf "$JNI/SDL"; cp -a "$SDL" "$JNI/SDL"
cp -a "$tmp/SDL2_image-$IMG_VER" "$JNI/SDL2_image"
cp -a "$tmp/SDL2_mixer-$MIX_VER" "$JNI/SDL2_mixer"

# 3. openkb overlay
APPMAIN="$BUILD/app/src/main"
mkdir -p "$APPMAIN/java/org/openkb/cht" "$APPMAIN/res/values" "$JNI/src/openkb"
cp "$HERE/overlay/AndroidManifest.xml" "$APPMAIN/AndroidManifest.xml"
cp "$HERE/overlay/OpenKBActivity.java" "$APPMAIN/java/org/openkb/cht/OpenKBActivity.java"
cp "$HERE/overlay/strings.xml"        "$APPMAIN/res/values/strings.xml"
cp "$HERE/overlay/Application.mk"     "$JNI/Application.mk"
cp "$HERE/overlay/Android.mk"         "$JNI/src/Android.mk"
cp "$HERE/overlay/cfg-android.h"      "$JNI/src/config.h"

# 4. 連結引擎源 (jni/src/openkb -> repo 根),Android.mk 以 $(OKB)=openkb 參照 src/ vendor/
ln -sfn "$ROOT" "$JNI/src/openkb"

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
