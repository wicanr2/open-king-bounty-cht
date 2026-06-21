import struct
from capstone import Cs, CS_ARCH_M68K, CS_MODE_BIG_ENDIAN, CS_MODE_M68K_000

path="/work/amiga-orig/extracted/KB/King's Bounty"
d=open(path,"rb").read()
u32=lambda o:struct.unpack(">I",d[o:o+4])[0]
o=0;o+=12
first=u32(o);o+=4;last=u32(o);o+=4
nseg=last-first+1;o+=4*nseg
segs={};si=0
while o<len(d):
    raw=u32(o);o+=4;ht=raw&0x0fffffff
    if ht in (0x3e9,0x3ea):
        nlw=u32(o)&0x0fffffff;o+=4;start=o;size=nlw*4
        segs[si]=('CODE' if ht==0x3e9 else 'DATA',start,size);o+=size;si+=1
    elif ht==0x3eb:u32(o);o+=4;si+=1
    elif ht==0x3ec:
        while True:
            n=u32(o);o+=4
            if n==0:break
            o+=4;o+=4*n
    elif ht==0x3f2:pass
    elif ht in (0x3f0,0x3f1):nlw=u32(o);o+=4;o+=nlw*4
    else:break

md=Cs(CS_ARCH_M68K, CS_MODE_BIG_ENDIAN|CS_MODE_M68K_000)
md.detail=True

# Find tight loops: a backward branch (dbra/bne/bcc) whose target is within ~12 insns,
# and the loop body contains a postincrement source read (Ax)+ and a write.
for (idx,(kind,start,size)) in segs.items():
    if kind!='CODE': continue
    insns=list(md.disasm(d[start:start+size],0))
    addr2idx={ins.address:i for i,ins in enumerate(insns)}
    for i,ins in enumerate(insns):
        mn=ins.mnemonic.split('.')[0]
        if mn in ('dbra','dbf','bne','bcc','bcs','bra','dbne','dbeq') and ins.op_str.startswith('$'):
            try: tgt=int(ins.op_str.split(',')[-1].lstrip('$'),16)
            except: continue
            if tgt in addr2idx:
                ti=addr2idx[tgt]
                if 0 < i-ti <= 14:  # backward, tight
                    body=insns[ti:i+1]
                    txt=' '.join(b.op_str for b in body)
                    has_src = '(a' in txt and ')+' in txt
                    has_postinc_read = any('+' in b.op_str and b.mnemonic.startswith('move') for b in body)
                    # count shifts in body
                    sh=sum(1 for b in body if b.mnemonic.split('.')[0] in ('lsl','lsr','roxl','roxr','asl','asr','rol','ror'))
                    if has_postinc_read and (')+' in txt):
                        print(f"SEG{idx} loop {start+tgt:#x}..{start+ins.address:#x} ({i-ti+1} insns, shifts={sh}): {ins.mnemonic} {ins.op_str}")
