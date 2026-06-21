import struct,sys
path="/work/amiga-orig/extracted/KB/King's Bounty"
d=open(path,"rb").read()
u32=lambda o:struct.unpack(">I",d[o:o+4])[0]
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
def segof(fo):
    for i,(k,s,sz) in segs.items():
        if s<=fo<s+sz:return i,fo-s
    return None,None
# target fileoffset given on cmdline; find code that references it (reloc pointing to it)
target=int(sys.argv[1],0)
tseg,toff=segof(target)
print(f"target {target:#x} = seg{tseg}+{toff:#x}")
for cur,tgt,offs in relocs:
    if tgt==tseg:
        for x in offs:
            v=u32(segs[cur][1]+x)
            if abs(v-toff)<64:
                print(f"  ref from SEG{cur} @file {segs[cur][1]+x:#x} (val seg{tseg}+{v:#x}, file {segs[tseg][1]+v:#x})  [caller seg kind {segs[cur][0]}]")
