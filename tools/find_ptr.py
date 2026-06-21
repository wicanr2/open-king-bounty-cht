import struct
path="/work/amiga-orig/extracted/KB/King's Bounty"
d=open(path,"rb").read()
u32=lambda o:struct.unpack(">I",d[o:o+4])[0]
def parse():
    o=12;first=u32(o);o+=4;last=u32(o);o+=4;o+=4*(last-first+1)
    segs={};si=0;cur=None;relocs=[]
    while o<len(d):
        raw=u32(o);o+=4;ht=raw&0x0fffffff
        if ht in(0x3e9,0x3ea):
            nlw=u32(o)&0x0fffffff;o+=4;segs[si]=('CODE'if ht==0x3e9 else'DATA',o,nlw*4);cur=si;o+=nlw*4;si+=1
        elif ht==0x3eb:u32(o);o+=4
        elif ht==0x3ec:
            while True:
                n=u32(o);o+=4
                if n==0:break
                tgt=u32(o);o+=4;offs=[u32(o+4*k)for k in range(n)];o+=4*n
                relocs.append((cur,tgt,offs))
        elif ht==0x3f2:pass
        elif ht in(0x3f0,0x3f1):nlw=u32(o);o+=4;o+=nlw*4
        else:break
    return segs,relocs
segs,relocs=parse()
# which seg index holds fileoffset X
def segof(fo):
    for i,(k,s,sz) in segs.items():
        if s<=fo<s+sz:return i,fo-s
    return None,None
for fo in (0x19b48,0x1ade6,0x1ae3c):
    print(fo, "->", segof(fo))
# resource name table @0x1ade6: SEG21 data. The code that reads names: find relocs pointing into seg21 data range near the name table
nt_seg,_=segof(0x1ade6)
print("name-table seg:",nt_seg)
for cur,tgt,offs in relocs:
    if tgt==nt_seg:
        for x in offs:
            v=u32(segs[cur][1]+x)
            tfo=segs[nt_seg][1]+v
            if 0x1ad00<=tfo<=0x1aef0:
                print(f"  ref from SEG{cur}@{segs[cur][1]+x:#x} -> seg{nt_seg}+{v:#x} (file {tfo:#x})")
