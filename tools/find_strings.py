import struct
path="/work/amiga-orig/extracted/KB/King's Bounty"
d=open(path,"rb").read()
for s in [b'cstl',b'title',b'GAME',b'%s',b'.pic',b'PACK',b'RLE']:
    i=0
    while True:
        j=d.find(s,i)
        if j<0:break
        ctx=d[max(0,j-4):j+24]
        print(f"{s!r} @file {j:#x}: {ctx}")
        i=j+1
