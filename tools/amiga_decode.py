#!/usr/bin/env python3
"""Amiga King's Bounty (New World Computing) asset decoder.

Reverse-engineered from the AmigaOS executable (hunk SEG22 @0x1ef40):
the bitmap payload is LZSS-compressed (Haruhiko Okumura style):
  - 4096-byte ring dictionary, pre-filled with 0x20 (space)
  - write pointer starts at 0xFEE
  - control byte: 8 flag bits, LSB first; 1 = literal, 0 = match
  - match: 2 bytes -> 12-bit offset + 4-bit (len-3); copy (len-3)+3 bytes
Output length is taken from a header field inside the resource (see below).

Container header (big-endian):
  u16 count
  count * { u16 a; u16 w; u16 h }      # a: plane/flag, w/h pixel size
  u16 a_global; u16 0000
  u16[2^planes] palette                # 12-bit 0RGB
  u16 comp_size; u16 0000              # size of LZSS stream (bytes)
  ... LZSS stream ...
"""
import struct, sys, os

def lzss_decompress(src, out_len):
    """Okumura LZSS. src = compressed bytes, out_len = expected output length."""
    N = 0x1000
    F_THRESH = 2  # min encoded length offset: count = (b&0xf)+2, then copy count+1
    dic = bytearray(b'\x20' * N)
    r = 0xFEE
    out = bytearray()
    si = 0
    flags = 0  # we emulate the d6 16-bit shifter: high byte counts bits
    # Replicate exact asm logic: d6 starts 0; each iter d6 >>=1 (logical, word),
    # then test bit8 (0x100). If set -> still have bits. else reload.
    # Simpler faithful emulation:
    bit_buf = 0
    bits_left = 0
    def next_ctrl_bit():
        nonlocal bit_buf, bits_left, si
        if bits_left == 0:
            if si >= len(src):
                return None
            bit_buf = src[si]; si += 1
            bits_left = 8
        b = bit_buf & 1
        bit_buf >>= 1
        bits_left -= 1
        return b

    while len(out) < out_len:
        bit = next_ctrl_bit()
        if bit is None:
            break
        if bit == 1:
            # literal
            if si >= len(src): break
            c = src[si]; si += 1
            out.append(c)
            dic[r] = c; r = (r + 1) & 0xFFF
        else:
            # match: two bytes
            if si + 1 >= len(src): break
            lo = src[si]; si += 1
            hi = src[si]; si += 1
            # d3 = lo ; d3 |= (hi & 0xf0)<<4  -> 12-bit offset
            offset = lo | ((hi & 0xF0) << 4)
            count = (hi & 0x0F) + 2           # d4 = (hi&0xf)+2
            # loop d5 = 0..count inclusive  -> copies count+1 bytes
            d5 = 0
            while True:
                c = dic[(offset + d5) & 0xFFF]
                out.append(c)
                dic[r] = c; r = (r + 1) & 0xFFF
                if len(out) >= out_len:
                    break
                d5 += 1
                if d5 > count:
                    break
    return bytes(out[:out_len])


def parse_header(d):
    u16 = lambda o: struct.unpack(">H", d[o:o+2])[0]
    count = u16(0)
    o = 2
    descs = []
    for _ in range(count):
        a = u16(o); w = u16(o+2); h = u16(o+4); o += 6
        descs.append((a, w, h))
    a_global = u16(o); o += 2
    o += 2  # 0000
    # plane count: a_global (0x1f -> 5 planes). palette size = 2^planes
    planes = bin(a_global & 0x1f).count('1') if a_global else 0
    # In practice a_global is 0x1f (=5 planes, 32 colors) for sprites,
    # 0 for tilesets. Tilesets still have a 32-entry palette in samples.
    # Determine palette length empirically: read until comp_size makes sense.
    return count, descs, a_global, o


def decode_resource(path):
    d = open(path, "rb").read()
    u16 = lambda o: struct.unpack(">H", d[o:o+2])[0]
    count = u16(0)
    o = 2
    descs = []
    for _ in range(count):
        descs.append((u16(o), u16(o+2), u16(o+4))); o += 6
    a_global = u16(o); o += 2
    o += 2
    # palette: 32 entries (5 bitplanes) in all observed samples
    NPAL = 32
    pal_off = o
    pal = [u16(pal_off + 2*i) for i in range(NPAL)]
    o = pal_off + 2*NPAL
    comp_size = u16(o); o += 2
    o += 2  # 0000
    data = d[o:o + comp_size] if comp_size else d[o:]
    return {
        'count': count, 'descs': descs, 'a_global': a_global,
        'palette': pal, 'comp_size': comp_size, 'data_off': o,
        'data': data, 'raw': d,
    }


def pal_to_rgb(pal):
    rgb = []
    for c in pal:
        r = ((c >> 8) & 0xF) * 17
        g = ((c >> 4) & 0xF) * 17
        b = (c & 0xF) * 17
        rgb.append((r, g, b))
    return rgb


if __name__ == "__main__":
    name = sys.argv[1]
    info = decode_resource(name)
    print("file:", name)
    print("count:", info['count'])
    print("descs:", info['descs'][:6], "..." if info['count'] > 6 else "")
    print("a_global: %#x  comp_size: %#x (%d)  data_off: %#x  data_len: %d" % (
        info['a_global'], info['comp_size'], info['comp_size'], info['data_off'], len(info['data'])))
    # try decompress with a generous out_len, then trim
    # estimate planar size: sum of w*h per subimage * planes /8
    a = info['a_global']
    planes = 5 if (a & 0x1f) == 0x1f else 4
    total_px = sum(w*h for (_, w, h) in info['descs'])
    est = total_px * planes // 8
    print("planes(est):", planes, "total_px:", total_px, "est_planar_bytes:", est)
    out = lzss_decompress(info['data'], est + 4096)
    print("decompressed:", len(out), "bytes")
    print("first 32:", out[:32].hex())
