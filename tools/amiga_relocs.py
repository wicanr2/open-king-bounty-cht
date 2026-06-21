import struct
path="/work/amiga-orig/extracted/KB/King's Bounty"
d=open(path,"rb").read()
u32=lambda o:struct.unpack(">I",d[o:o+4])[0]
o=0
o+=4;o+=4;o+=4
first=u32(o);o+=4;last=u32(o);o+=4
nseg=last-first+1; o+=4*nseg
segs={}; si=0; cur=None
while o<len(d):
    raw=u32(o);o+=4; ht=raw&0x0fffffff
    if ht in (0x3e9,0x3ea):
        nlw=u32(o)&0x0fffffff;o+=4;start=o;size=nlw*4
        segs[si]=('CODE' if ht==0x3e9 else 'DATA',start,size);cur=si;o+=size;si+=1
    elif ht==0x3eb:
        u32(o);o+=4
    elif ht==0x3ec:  # RELOC32
        print(f"--- RELOC32 in seg {cur} (start={segs[cur][1]:#x}) ---")
        while True:
            n=u32(o);o+=4
            if n==0:break
            tgt=u32(o);o+=4
            offs=[u32(o+4*k) for k in range(n)]
            o+=4*n
            print(f"  target seg {tgt}: {n} fixups -> "+", ".join(f"{x:#x}(@file {segs[cur][1]+x:#x})" for x in offs[:200]))
    elif ht==0x3f2: pass
    elif ht in (0x3f0,0x3f1): nlw=u32(o);o+=4;o+=nlw*4
    else: break
