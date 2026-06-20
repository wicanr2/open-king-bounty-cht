#!/bin/sh
# 烘烤 CJK 點陣 atlas (cjk24.bin) → data/cjk24.bin。
# 在 docker uv 容器內跑 (Pillow + Noto Sans CJK TC);不污染系統 Python。
# 碼點來源:data/free 下所有譯文 .txt/.ini + docs/translation 譯名表。
# 用法 (在 repo 根):
#   docker run --rm -v "$PWD":/src -w /src ghcr.io/astral-sh/uv:python3.12-bookworm-slim \
#       bash docker/build-font.sh
set -e
cd "$(dirname "$0")/.."

export DEBIAN_FRONTEND=noninteractive
apt-get update -qq >/dev/null 2>&1
apt-get install -y -qq --no-install-recommends fonts-noto-cjk >/dev/null 2>&1

FONT=/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc
[ -f "$FONT" ] || FONT=/usr/share/fonts/opentype/noto/NotoSerifCJK-Regular.ttc

uv venv -q /tmp/fontvenv
. /tmp/fontvenv/bin/activate
uv pip install -q pillow

python3 tools/build_cjk_font.py \
    --font "$FONT" --index 0 --size 24 --mode gray \
    --text data/free/*.txt data/free/*.ini docs/translation/glossary-draft.md \
           src/game.c src/bounty.c src/combat.c src/ui.c src/inprint.c \
    --out data/cjk24.bin --preview build/cjk24_preview.png

echo "=== cjk24.bin 完成 ==="
ls -la data/cjk24.bin
