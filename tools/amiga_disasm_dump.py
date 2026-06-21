import struct, sys
from capstone import Cs, CS_ARCH_M68K, CS_MODE_BIG_ENDIAN, CS_MODE_M68K_000

path="/work/amiga-orig/extracted/KB/King's Bounty"
d=open(path,"rb").read()
u32=lambda o:struct.unpack(">I",d[o:o+4])[0]

o=0
o+=4;o+=4;o+=4
first=u32(o);o+=4;last=u32(o);o+=4
nseg=last-first+1; o+=4*nseg
segs={}
si=0
while o<len(d):
    raw=u32(o);o+=4; ht=raw&0x0fffffff
    if ht in (0x3e9,0x3ea):
        nlw=u32(o)&0x0fffffff;o+=4;start=o;size=nlw*4
        segs[si]=('CODE' if ht==0x3e9 else 'DATA',start,size);o+=size;si+=1
    elif ht==0x3eb: u32(o);o+=4;si+=1
    elif ht==0x3ec:
        while True:
            n=u32(o);o+=4
            if n==0:break
            o+=4;o+=4*n
    elif ht==0x3f2: pass
    elif ht in (0x3f0,0x3f1): nlw=u32(o);o+=4;o+=nlw*4
    else: break

md=Cs(CS_ARCH_M68K, CS_MODE_BIG_ENDIAN|CS_MODE_M68K_000)
md.detail=False

# args: seg_idx start_fileoff end_fileoff
seg=int(sys.argv[1]); a=int(sys.argv[2],0); b=int(sys.argv[3],0)
kind,start,size=segs[seg]
# disasm from a-start within seg
rel_a=a-start; rel_b=b-start
for i in md.disasm(d[a:b],0):
    print(f"{start+rel_a+i.address:#08x}: {i.mnemonic:9} {i.op_str}")
