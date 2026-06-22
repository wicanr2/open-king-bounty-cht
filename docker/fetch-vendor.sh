#!/bin/sh
# 抓 openkb 的第三方 vendored 相依。
# upstream vendor/drop.sh 的 URL 已老舊 (git:// 失效、scale2x/sha2 來源死掉);
# 本腳本用可用的來源補齊,並略過未被任何原始碼引用的 sha2。
set -e
cd "$(dirname "$0")/../vendor"

# git:// → https:// (容器內已設;此處保險)
git config --global url."https://github.com/".insteadOf "git://github.com/" 2>/dev/null || true

# 1. SDL_inprint (內嵌點陣字渲染) — upstream 來源仍可用
if [ ! -f inprint.c ]; then
  rm -rf SDL_inprint
  git clone --depth 1 https://github.com/driedfruit/SDL_inprint.git >/dev/null 2>&1
  # 注入 SDL1.2→SDL2 shim,並把字型來源改指 openkb 的 font.h
  printf '#include "../src/sdlcompat.h"\n' > inprint.c
  sed 's/inline_font\.h/..\/src\/font.h/' SDL_inprint/inprint.c >> inprint.c
fi

# 2. SDL_SavePNG (截圖) — upstream 來源仍可用,包 HAVE_LIBPNG
if [ ! -f savepng.c ]; then
  rm -rf SDL_SavePNG
  git clone --depth 1 https://github.com/driedfruit/SDL_SavePNG.git >/dev/null 2>&1
  printf '#ifdef HAVE_LIBPNG\n' > savepng.c; cat SDL_SavePNG/savepng.c >> savepng.c; printf '#endif /* HAVE_LIBPNG */\n' >> savepng.c
  printf '#ifdef HAVE_LIBPNG\n' > savepng.h; cat SDL_SavePNG/savepng.h >> savepng.h; printf '#endif /* HAVE_LIBPNG */\n' >> savepng.h
fi

# 3. scale2x (2x 放大) — upstream sourceforge 死,改用 amadvance GitHub 鏡像
if [ ! -f scale2x.c ]; then
  rm -rf scale2x-src
  git clone --depth 1 https://github.com/amadvance/scale2x.git scale2x-src >/dev/null 2>&1
  cp scale2x-src/contrib/sdl/scale2x.c scale2x.c
fi

# 4. (已移除 hfsutils / libhfs / librsrc):free 等模組未引用 Mac 資源 fork,
#    Makefile 也不再連結;省掉 ftp 來源與古董 configure 在 mac/win 編不過的問題。

# sha2:upstream drop.sh 會抓,但沒有任何 .c 引用 → 略過。

echo "vendor 相依就緒: $(ls *.c 2>/dev/null | tr '\n' ' ')"
