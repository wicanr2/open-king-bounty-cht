#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
把繁中譯文用到的漢字子集,用系統 CJK TTF 烘成點陣 atlas,供 1oom 引擎 CJK 路徑渲染。
源自 u4-cht/tools/build_cjk_font.py,改 magic 與碼點來源。

輸出(自訂二進位,引擎 fread,codepoint 升序二分查找):
  magic[8]   "OKBCJK\0\1"
  uint16 glyphW
  uint16 glyphH
  uint32 count
  records[count]: uint32 codepoint(LE) + glyphW*glyphH bytes(alpha 0..255)

碼點來源(擇一/可疊加):
  --text FILE...   收集這些 UTF-8 文字檔內所有 CJK/全形碼點(≥0x3000)
  --chars "字串"   直接給一段字
用法範例:
  python3 tools/build_cjk_font.py \
      --font /usr/share/fonts/truetype/arphic/uming.ttc --size 24 \
      --chars "繼續讀取進度新遊戲離開選擇種族" \
      --out assets/fonts/cjk24.bin --preview build/cjk24_preview.png
"""
import argparse
import os
import struct
from PIL import Image, ImageFont, ImageDraw

MAGIC = b"OKBCJK\0\1"


def collect_codepoints(text_files, chars_literal):
    cps = set()

    def add(s):
        for ch in s:
            if ord(ch) >= 0x3000:   # CJK 統一表意 + 全形標點起點
                cps.add(ch)

    for path in text_files or []:
        with open(path, encoding="utf-8") as fh:
            add(fh.read())
    if chars_literal:
        add(chars_literal)
    # 一律納入可列印 ASCII (0x20–0x7E),讓英文/數字也以同一套 Noto 字形渲染,
    # 與中文風格一致 (box-drawing / 控制字元不納入,仍走點陣)。
    for cp in range(0x20, 0x7F):
        cps.add(chr(cp))
    return sorted(cps, key=ord)


def render_glyph(font, ch, W, H, mode, threshold=96):
    img = Image.new("L", (W, H), 0)
    d = ImageDraw.Draw(img)
    bbox = d.textbbox((0, 0), ch, font=font)
    gw, gh = bbox[2] - bbox[0], bbox[3] - bbox[1]
    x = (W - gw) // 2 - bbox[0]
    y = (H - gh) // 2 - bbox[1]
    d.text((x, y), ch, fill=255, font=font)
    if mode == "binary":
        return img.point(lambda p: 255 if p >= threshold else 0)
    return img


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--font", required=True)
    ap.add_argument("--index", type=int, default=0, help=".ttc face index")
    ap.add_argument("--size", type=int, default=24, help="字型 glyph 像素大小")
    ap.add_argument("--cell", type=int, default=0, help="atlas cell 邊長(預設=size)")
    ap.add_argument("--mode", choices=["gray", "binary"], default="gray")
    ap.add_argument("--text", nargs="*", default=[], help="UTF-8 文字檔(收集碼點)")
    ap.add_argument("--chars", default="", help="直接給一段字")
    ap.add_argument("--out", required=True)
    ap.add_argument("--preview", default="")
    args = ap.parse_args()

    chars = collect_codepoints(args.text, args.chars)
    if not chars:
        raise SystemExit("沒有收集到任何 CJK 碼點(給 --text 或 --chars)")
    font = ImageFont.truetype(args.font, args.size, index=args.index)
    W = H = args.cell if args.cell > 0 else args.size

    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    glyphs = [(ord(ch), render_glyph(font, ch, W, H, args.mode).tobytes()) for ch in chars]

    with open(args.out, "wb") as f:
        f.write(MAGIC)
        f.write(struct.pack("<HHI", W, H, len(glyphs)))
        for cp, data in glyphs:
            f.write(struct.pack("<I", cp))
            f.write(data)
    print(f"glyphs: {len(glyphs)}  cell {W}x{H}  -> {args.out} ({os.path.getsize(args.out)} bytes)")

    if args.preview:
        n = len(chars)
        cols = 16
        rows = (n + cols - 1) // cols
        prev = Image.new("L", (cols * W, rows * H), 0)
        for i, ch in enumerate(chars):
            prev.paste(render_glyph(font, ch, W, H, "gray"), ((i % cols) * W, (i // cols) * H))
        os.makedirs(os.path.dirname(args.preview) or ".", exist_ok=True)
        prev.save(args.preview)
        print(f"preview -> {args.preview}")


if __name__ == "__main__":
    main()
