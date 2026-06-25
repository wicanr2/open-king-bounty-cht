#!/bin/sh
# build-sdl2-from-source.sh PREFIX
#
# 從源碼編譯 pinned 真 SDL2 + SDL2_image + SDL2_mixer 到 PREFIX。
# 用 stb 內建 codec(image=stb_image 處理 PNG/JPG、mixer=stb_vorbis 處理 OGG),
# 刻意不連任何外部 codec(libpng / libvorbis / fluidsynth …)→ 產物自包含、可重現。
#
# 為什麼:macOS CI 原本 `brew install sdl2` 會隨 Homebrew 更新而變
# (2026-06 brew 把 sdl2 換成 sdl2-compat = SDL2 API 架在 SDL3 上,執行時要 libSDL3
#  → 玩家看到 "Failed loading SDL3 library";見 GitHub issue)。源碼編真 SDL2 根治此問題。
#
# 用後設 PATH="$PREFIX/bin:$PATH",openkb 的 configure 即會用此 sdl2-config。
# Linux/macOS 通用(已於 Linux 驗證 build + 連結 + OGG mixer)。
set -e

PREFIX="${1:?用法: build-sdl2-from-source.sh PREFIX}"
SDL_VER=2.30.9
IMG_VER=2.8.2
MIX_VER=2.8.0
NET_VER=2.2.0
WORK="$(mktemp -d)"
mkdir -p "$PREFIX"

dl() { wget -q "$1" -O "$2" || curl -fsSL "$1" -o "$2"; }

cd "$WORK"
echo "[sdl-src] 下載 SDL2 $SDL_VER / image $IMG_VER / mixer $MIX_VER / net $NET_VER"
dl "https://github.com/libsdl-org/SDL/releases/download/release-$SDL_VER/SDL2-$SDL_VER.tar.gz" sdl2.tgz
dl "https://github.com/libsdl-org/SDL_image/releases/download/release-$IMG_VER/SDL2_image-$IMG_VER.tar.gz" img.tgz
dl "https://github.com/libsdl-org/SDL_mixer/releases/download/release-$MIX_VER/SDL2_mixer-$MIX_VER.tar.gz" mix.tgz
dl "https://github.com/libsdl-org/SDL_net/releases/download/release-$NET_VER/SDL2_net-$NET_VER.tar.gz" net.tgz

echo "[sdl-src] 1/3 SDL2"
tar xf sdl2.tgz && cd "SDL2-$SDL_VER"
./configure --prefix="$PREFIX" >/dev/null && make -j4 >/dev/null && make install >/dev/null
cd "$WORK"
export PATH="$PREFIX/bin:$PATH"

echo "[sdl-src] 2/3 SDL2_image (stb_image,無外部 codec)"
tar xf img.tgz && cd "SDL2_image-$IMG_VER"
./configure --prefix="$PREFIX" --with-sdl-prefix="$PREFIX" \
  --disable-png --disable-jpg --disable-tif --disable-webp --disable-avif --disable-jxl \
  --enable-stb-image >/dev/null && make -j4 >/dev/null && make install >/dev/null
cd "$WORK"

echo "[sdl-src] 3/3 SDL2_mixer (stb_vorbis OGG,無 fluidsynth/外部 codec)"
tar xf mix.tgz && cd "SDL2_mixer-$MIX_VER"
./configure --prefix="$PREFIX" --with-sdl-prefix="$PREFIX" \
  --enable-music-ogg-stb \
  --disable-music-ogg-vorbis --disable-music-ogg-tremor \
  --disable-music-flac-libflac --disable-music-mod-modplug --disable-music-mod-xmp \
  --disable-music-midi-fluidsynth --disable-music-mp3-mpg123 >/dev/null \
  && make -j4 >/dev/null && make install >/dev/null
cd "$WORK"

# SDL2_net:openkb base LDFLAGS 連了 -lSDL2_net(實際只 netkb/combat.c 用,
# 主程式不用但仍需連結成功)。很小、無外部相依。
echo "[sdl-src] +SDL2_net $NET_VER"
tar xf net.tgz && cd "SDL2_net-$NET_VER"
./configure --prefix="$PREFIX" --with-sdl-prefix="$PREFIX" >/dev/null \
  && make -j4 >/dev/null && make install >/dev/null
cd /
rm -rf "$WORK"

echo "[sdl-src] 完成 → $PREFIX (sdl2-config: $PREFIX/bin/sdl2-config)"
