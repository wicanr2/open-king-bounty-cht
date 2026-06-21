#!/usr/bin/env python3
"""Batch-decode all Amiga King's Bounty graphics resources to PNG.

Output: qa-amiga/all/<name>.png  (gitignored). Multi-frame resources become a
horizontal strip. Sprites (a_global==0x1f) get index 0 as a transparent colorkey.
Prints a verification table comparing decoded dims against the free/DOS PNG.
"""
import os, sys, struct
sys.path.insert(0, "/work/tools")
from amiga_decode import parse, lzss_decompress, palette_rgb
from PIL import Image

GAME = "/work/amiga-orig/extracted/KB/GAME"
FREE = "/work/data/free"
OUT = "/work/qa-amiga/all"
os.makedirs(OUT, exist_ok=True)

# everything in GAME that is a graphics container (exclude PCM sfx + map data)
SKIP = {"Bash","Bump","Death","Horns","King","Vict","Treas","Walk","Tele","LAND.ORG",".info"}

def free_size(name):
    p = os.path.join(FREE, name + ".png")
    if os.path.exists(p):
        with Image.open(p) as im:
            return im.size
    return None

def decode_one(name):
    path = os.path.join(GAME, name)
    info = parse(path)
    raw = lzss_decompress(info["stream"], info["out_size"])
    rgb = palette_rgb(info["palette"])
    cnt = info["count"]
    a, w, h = info["descs"][0]
    planes = 5
    rb = (w + 7) // 8
    fsz = rb * h * planes
    nf = max(1, cnt)
    is_sprite = (info["a_global"] & 0x1f) == 0x1f
    mode = "RGBA" if is_sprite else "RGB"
    strip = Image.new(mode, (w * nf, h), (0, 0, 0, 0) if is_sprite else (0, 0, 0))
    sp = strip.load()
    for f in range(nf):
        base = f * fsz
        for y in range(h):
            for x in range(w):
                bit = 7 - (x & 7)
                v = 0
                for p in range(planes):
                    idx = base + p * rb * h + y * rb + x // 8
                    if idx < len(raw):
                        v |= ((raw[idx] >> bit) & 1) << p
                col = rgb[v]
                if is_sprite:
                    sp[f * w + x, y] = (*col, 0) if v == 0 else (*col, 255)
                else:
                    sp[f * w + x, y] = col
    strip.save(os.path.join(OUT, name + ".png"))
    ok = len(raw) == info["out_size"]
    return {
        "name": name, "count": cnt, "w": w, "h": h, "sprite": is_sprite,
        "out": info["out_size"], "got": len(raw), "ok": ok,
        "strip": (w * nf, h), "free": free_size(name),
    }

if __name__ == "__main__":
    names = sorted(n for n in os.listdir(GAME)
                   if n not in SKIP and os.path.isfile(os.path.join(GAME, n)))
    rows = []
    for n in names:
        try:
            rows.append(decode_one(n))
        except Exception as e:
            print("ERR %-10s %s" % (n, e))
    print("\n%-10s %5s %-9s %-7s %-6s %-9s %-9s" %
          ("name", "cnt", "dim", "sprite", "size", "strip", "free(DOS)"))
    nok = 0
    for r in rows:
        nok += r["ok"]
        free = "%dx%d" % r["free"] if r["free"] else "-"
        print("%-10s %5d %-9s %-7s %-6s %-9s %-9s" % (
            r["name"], r["count"], "%dx%d" % (r["w"], r["h"]),
            "yes" if r["sprite"] else "-",
            "OK" if r["ok"] else "SHORT(%d/%d)" % (r["got"], r["out"]),
            "%dx%d" % r["strip"], free))
    print("\n%d/%d decoded with exact out_size; PNGs in qa-amiga/all/" % (nok, len(rows)))
