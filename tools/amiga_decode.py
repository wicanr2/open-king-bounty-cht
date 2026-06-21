#!/usr/bin/env python3
"""Amiga King's Bounty (New World Computing) graphics decoder -- CRACKED.

Container format (big-endian):
  u16 count
  count * { u16 a; u16 w; u16 h }      # a: plane/flag (0x1f sprites, 0 tiles/pics)
  u16 a_global; u16 0x0000
  u16[32] palette                       # 32 colors = 5 bitplanes, 12-bit 0RGB
  u16 comp_size; u16 0x0000             # size of the compressed LZSS stream
  u16 out_size                          # size of the decompressed planar bitmap
  ... LZSS stream ...                   # starts right after out_size

Codec: Okumura LZSS (same family as Genesis KB, see src/lib/md-rom.c):
  - 4096-byte ring dictionary (fill value irrelevant; 0 used here)
  - write pointer starts at 0xFEE
  - control byte: 8 flag bits LSB-first; 1 = literal, 0 = match
  - match: 2 bytes -> 12-bit offset = b1 | ((b2&0xF0)<<4), length = (b2&0xF) + 3
  - decompress until out_size bytes produced

Pixel layout: SEQUENTIAL bitplanes (plane0 full image, then plane1, ...).
  Each plane is (w/8) bytes/row * h rows. Pixel value = sum_p (bit_p << p),
  plane0 = LSB. For multi-frame sprites (count>1), the planes are stored
  per-frame: frame0 [p0..p4], frame1 [p0..p4], ...  Each frame is w*h*5/8 bytes.

12-bit palette: R = (c>>8 & 0xF)*17, G = (c>>4 & 0xF)*17, B = (c & 0xF)*17.

Verified: cstl (castle scene 240x102), peas (4-frame peasant 48x34).
"""
import struct, sys


def lzss_decompress(src, out_size, fill=0):
    """Okumura LZSS, length = (b2&0xF)+3. src starts at the stream (after out_size word)."""
    N = 0x1000
    dic = bytearray(bytes([fill]) * N)
    out = bytearray()
    r = 0xFEE
    flags = 0
    p = 0
    while len(out) < out_size and p < len(src):
        flags >>= 1
        if (flags & 0x100) == 0:
            if p >= len(src):
                break
            flags = src[p] | 0xFF00
            p += 1
        if flags & 1:
            if p >= len(src):
                break
            c = src[p]; p += 1
            out.append(c); dic[r] = c; r = (r + 1) & 0xFFF
        else:
            if p + 1 >= len(src):
                break
            b1 = src[p]; b2 = src[p + 1]; p += 2
            off = b1 | ((b2 & 0xF0) << 4)
            ln = (b2 & 0xF) + 3
            for k in range(ln):
                c = dic[(off + k) & 0xFFF]
                out.append(c); dic[r] = c; r = (r + 1) & 0xFFF
                if len(out) >= out_size:
                    break
    return bytes(out)


def parse(path):
    d = open(path, "rb").read()
    u16 = lambda o: struct.unpack(">H", d[o:o + 2])[0]
    count = u16(0)
    o = 2
    descs = []
    for _ in range(count):
        descs.append((u16(o), u16(o + 2), u16(o + 4))); o += 6
    a_global = u16(o); o += 2
    o += 2  # 0x0000
    palette = [u16(o + 2 * i) for i in range(32)]
    o += 64
    comp_size = u16(o); o += 2
    o += 2  # 0x0000
    out_size = u16(o); o += 2
    stream = d[o:]
    return {
        "count": count, "descs": descs, "a_global": a_global,
        "palette": palette, "comp_size": comp_size, "out_size": out_size,
        "stream": stream, "stream_off": o,
    }


def palette_rgb(palette):
    return [(((c >> 8) & 0xF) * 17, ((c >> 4) & 0xF) * 17, (c & 0xF) * 17) for c in palette]


def decode_frames(path):
    """Return (frames, palette_rgb). frames = list of 2D index arrays [h][w]."""
    info = parse(path)
    raw = lzss_decompress(info["stream"], info["out_size"])
    count = info["count"]
    a, w, h = info["descs"][0]  # all observed descs share w/h within a file
    planes = 5
    rb = (w + 7) // 8
    fsz = rb * h * planes
    frames = []
    for f in range(max(1, count)):
        base = f * fsz
        img = [[0] * w for _ in range(h)]
        for y in range(h):
            for x in range(w):
                bit = 7 - (x & 7)
                v = 0
                for p in range(planes):
                    idx = base + p * rb * h + y * rb + x // 8
                    if idx < len(raw):
                        v |= ((raw[idx] >> bit) & 1) << p
                img[y][x] = v
        frames.append(img)
    return frames, palette_rgb(info["palette"])


if __name__ == "__main__":
    info = parse(sys.argv[1])
    print("count", info["count"], "descs", info["descs"][:4])
    print("a_global %#x comp_size %#x out_size %d stream_off %#x" % (
        info["a_global"], info["comp_size"], info["out_size"], info["stream_off"]))
    raw = lzss_decompress(info["stream"], info["out_size"])
    print("decompressed", len(raw), "/", info["out_size"])
