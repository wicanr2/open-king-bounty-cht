import struct
path="/work/amiga-orig/extracted/KB/King's Bounty"
d=open(path,"rb").read()
u32=lambda o:struct.unpack(">I",d[o:o+4])[0]
def parse():
    o=12
    first=u32(o);o+=4;last=u32(o);o+=4
    nseg=last-first+1;o+=4*nseg
    segs={};si=0;cur=None;relocs=[]
    while o<len(d):
        raw=u32(o);o+=4;ht=raw&0x0fffffff
        if ht in (0x3e9,0x3ea):
            nlw=u32(o)&0x0fffffff;o+=4;start=o;size=nlw*4
            segs[si]=('CODE' if ht==0x3e9 else 'DATA',start,size);cur=si;o+=size;si+=1
        elif ht==0x3eb:u32(o);o+=4
        elif ht==0x3ec:
            while True:
                n=u32(o);o+=4
                if n==0:break
                tgt=u32(o);o+=4
                offs=[u32(o+4*k) for k in range(n)];o+=4*n
                relocs.append((cur,tgt,offs))
        elif ht==0x3f2:pass
        elif ht in (0x3f0,0x3f1):nlw=u32(o);o+=4;o+=nlw*4
        else:break
    return segs,relocs
segs,relocs=parse()
for cur,tgt,offs in relocs:
    if tgt==22:
        for x in offs:
            val=u32(segs[cur][1]+x)
            print(f"SEG{cur}@{segs[cur][1]+x:#x} -> seg22off {val:#x}")
