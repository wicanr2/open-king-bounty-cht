import struct, sys
path="/work/amiga-orig/extracted/KB/King's Bounty"
d=open(path,"rb").read()
u32=lambda o:struct.unpack(">I",d[o:o+4])[0]
o=0
magic=u32(o); o+=4
assert magic==0x000003f3, hex(magic)
# library names
nlib=u32(o); o+=4
assert nlib==0, nlib
tablesize=u32(o); o+=4
first=u32(o); o+=4
last=u32(o); o+=4
nseg=last-first+1
print(f"hunk exe: tablesize={tablesize} first={first} last={last} nseg={nseg}")
seg_sizes=[]
for i in range(nseg):
    seg_sizes.append(u32(o)); o+=4
print("seg sizes (longwords):", [hex(s) for s in seg_sizes])
# now hunks
HUNK={0x3e9:'CODE',0x3ea:'DATA',0x3eb:'BSS',0x3ec:'RELOC32',0x3f0:'SYMBOL',0x3f1:'DEBUG',0x3f2:'END',0x3e8:'UNIT',0x3e7:'NAME'}
segs=[]
si=0
while o < len(d):
    raw=u32(o); o+=4
    ht=raw & 0x0fffffff  # mask MEMF flags
    name=HUNK.get(ht, f'?{ht:#x}')
    if ht in (0x3e9,0x3ea):
        nlw=u32(o)&0x0fffffff; o+=4
        start=o
        size=nlw*4
        segs.append((name,si,start,size))
        print(f"hunk {name} #{si} fileoff={start:#x} size={size:#x} ({size}B)")
        o+=size
        si+=1
    elif ht==0x3eb: # BSS
        nlw=u32(o); o+=4
        print(f"hunk BSS #{si} size={nlw*4:#x}")
        si+=1
    elif ht==0x3ec: # RELOC32
        while True:
            n=u32(o); o+=4
            if n==0: break
            hn=u32(o); o+=4
            o+=4*n
    elif ht==0x3f2: # END
        pass
    elif ht in (0x3f0,0x3f1): # SYMBOL/DEBUG -> has size
        nlw=u32(o); o+=4
        o+=nlw*4
    else:
        print(f"unknown hunk {ht:#x} at {o-4:#x}, stop")
        break
print("\nCODE segments:")
for s in segs:
    if s[0]=='CODE': print(" ", s)
