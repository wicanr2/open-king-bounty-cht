# Amiga 版 King's Bounty 素材格式逆向 (圖形已完全破解)

目標:從 Amiga 版 `.adf` 抽取美術導入 openkb 引擎(供 F8 主題切換)。
**圖形 codec 已完全破解並驗證**(cstl/town/tileset/peas/dragon/title 全部正確解出)。

## 來源

- ROM:`Kings_Bounty_Amiga_ROM_EN.zip` → `King's Bounty (1990)(New World Computing).adf`(880KB AmigaDOS OFS)。
- 開發者:New World Computing(與 Genesis 版同社,**圖形 codec 同源**:都是 Okumura LZSS)。
- 版權:`.adf` 與解出的素材一律 gitignore、不入公開 repo(`amiga-orig/`、`qa-amiga/`)。

## 解開磁碟 (已完成)

amitools(docker uv venv):`xdftool "...adf" unpack out_dir` → `KB/GAME/` 下 **72 個具名資源**。
另有頂層 `King's Bounty`(AmigaOS 執行檔,反組譯 oracle)。

## 圖形容器格式 (已完全破解)

big-endian:
```
u16  count                       // 子圖/幀數
count × { u16 a; u16 w; u16 h }  // 描述子;a: sprite=0x1f, tile/pic=0;w/h 像素尺寸
u16  a_global                    // 描述子後僅 1 個 word,palette 緊接其後
u16[32] palette                  // 固定 32 色 = 5 bitplanes,12-bit 0RGB;pal[0]=index0=黑(sprite 透明色)
u16  comp_size; u16 0x0000; u16 0x0000
u16  out_size                    // **解壓後 planar bitmap 大小** (= w/8 × h × 5 × frames)
... LZSS stream ...              // 緊接在 out_size 之後
```
> **關鍵踩雷 1(palette off-by-one,最終根因)**:palette 從描述子後**第一個** word 開始(count=1 時 offset 10),不是第二個(offset 12)。早期多跳一個 word → 每個 pixel index 顯示成 `palette[i-1]` 的色:天空 teal 取代 cyan、草地中綠取代亮綠、sprite index 0 變白而非透明。`amiga_color`(`<<4`)與 plane 解碼本來就對,純粹 palette 起點偏移一格。對齊 `kings-bounty_17.gif` 逐像素驗證:每像素均差 **213.8 → 4.8**。修正:`amiga-data.c`(palette 讀取改 `2+count*6+2`,並把那個 word 移到 palette 之後、out_size 之前,保持 stream 起點不變)。
>
> **關鍵踩雷 2**:stream 起點在 `out_size`(u16)**之後**,不是在 comp_size 之後。早期把 out_size 那 2 bytes 當成 stream 開頭 → 整張圖前一小段對、其餘全 drift 成雜訊。

## 12-bit palette 展開:`nibble << 4`,不是 `× 17`

每個 4-bit 通道展開成 8-bit 時,Amiga OCS/ECS 硬體做的是 **`n << 4`(= `n × 16`,低 nibble 補 0)**,
**不是 `n × 17`**。以真實 Amiga 截圖 `amiga_pic/kings-bounty_17.gif`(Plains 招募畫面,31 色)當 ground truth:
用 `<< 4` 解出的 32 色 palette 與截圖的 31 色**逐色精準命中(最近色距離 = 0)**;用 `× 17` 則每通道最多偏亮 15
(綠變螢光、紅岩偏紫紅、整體像過曝 / 反白)。這是唯一一個真正的解碼 bug,已修(`amiga-data.c` `amiga_color`、`amiga_decode.py` `palette_rgb`)。

> **plai 早期看起來「色彩不對」是真的解碼錯誤,根因為上述 palette off-by-one(踩雷 1),不是視覺誤判。**
> LZSS、plane 排列(plane-major、plane0=LSB)、尺寸、`<<4` 展開全部正確;唯一錯的是 palette 起點偏移一格,
> 讓全圖顏色整體位移(形狀正確但顏色錯)。location 圖的草地、石牆確實用**高頻 dithering 點繪**(綠/褐、灰/粉交錯)
> 在 32 色內表現質感,放大後密度與 `_17` 一致——但這點繪之上仍須正確 palette 才會對。
> 定位方法值得記住:**用「正確對齊時每個 index 應對應到參考圖單一顏色(色散最小)」反推**,
> 一眼就看出 `index i → palette[i-1]` 的乾淨位移規律,比盲試 plane permutation 快得多。

## 壓縮 codec:Okumura LZSS (與 Genesis 同源)

對應 `src/lib/md-rom.c` 的 `md_lzss_decompress`(Genesis 版,已驗證可用):
- 4096-byte ring dictionary(初值無所謂,正確解壓會覆寫;本實作填 0)。
- 寫指標 r 起始 0xFEE。
- control byte:8 個 flag bit,**LSB first**;每輪 `flags>>=1`,高位用 0xFF00 計數;flag bit=1 → literal(1 byte),=0 → match。
- match:讀 2 bytes b1,b2 → `offset = b1 | ((b2&0xF0)<<4)`(12-bit),**`length = (b2&0xF) + 3`**。複製 length bytes。
- 解到 out_size bytes 為止。
> **length 公式 = +3**(與 Genesis 一致)。Amiga 執行檔反組譯時看到 `(b2&0xF)+2` 然後迴圈 `d5=0..count`(含 count = count+1 次),等效 +3;Python 移植要算對邊界。

## 像素排列:sequential bitplane(plane-major)

- **每個 plane 整張存完才換下一個 plane**(NOT interleaved-per-row)。
- 每 plane:`rowbytes = w/8` bytes/列 × h 列。pixel value `v = Σ_p (bit_p << p)`,**plane0 = LSB**。
- bit 取法:byte 內 MSB first(`bit = 7-(x&7)`)。
- **多幀 sprite**(count>1,如 peas/drag 4 幀):各幀獨立,幀內 5 planes sequential;幀大小 = `w/8 × h × 5`,幀接幀。

## 驗證結果

**62 個圖形資源全部解碼,out_size 100% 精準命中**(`tools/amiga_batch.py` 批次轉 PNG 到 `qa-amiga/all/`)。
strip 尺寸與 free DOS 版同名 PNG **逐一吻合**(192×34 sprite、240×102 location、96×102、1728×34 tileset…),為強力交叉驗證。

目視確認(✅ = 已並排對照 free DOS 版,內容正確;Amiga 版美術較精細但同構):

| 資源 | count | 尺寸 | out_size | 目視 | 內容 |
|---|---|---|---|---|---|
| cstl | 1 | 240×102 | 15300 | ✅ | 城堡場景(藍天白雲、石牆、紅旗) |
| town | 1 | 240×102 | 15300 | ✅ | 城鎮(白屋紅瓦、市政廳、綠樹) |
| cave/dngn/frst/plai | 1 | 240×102 | 15300 | size✓ | 各 location 場景 |
| tileseta/tilesetb | 36 | 48×34 | 36720 | ✅ | 36 個地圖 tile(城堡/水/樹/橋…),與 free 同序 |
| tilesalt | 9 | 48×34 | — | size✓ | 替代地圖 tile |
| title | 1 | 320×200 | 40000 | ✅ | 標題全螢幕圖 |
| nwcp | 1 | 320×200 | — | size✓ | NWC logo(Amiga 全螢幕,free 裁成 320×82) |
| select | 3 | 288×184 | 35440 | size✓ | 選單背景(Amiga 獨有,無 free 對照) |
| endpic | 5 | 144×170 | — | size✓ | 結局圖(Amiga 獨有) |
| view | 14 | 48×34 | 14280 | size✓ | UI 元件 |
| comtiles | 15 | 48×34 | 15300 | size✓ | 戰鬥 tile(sprite,colorkey) |
| cursor | 28 | 48×34 | — | size✓ | 游標幀(Amiga 28 幀 > free 16 幀) |
| peas | 4 | 48×34 | 4080 | ✅ | 農民 sprite(4 幀持乾草叉) |
| drag | 4 | 48×34 | 4080 | ✅ | 藍龍 sprite(4 幀,frame3 噴火) |
| knig/barb/pala/sorc | 1 | 96×102 | 6120 | ✅(knig) | 英雄肖像 |
| spri/mili/ogre/skel/…(troop) | 4 | 48×34 | 4080 | size✓ | troop/monster sprite |

> **sprite 透明色**:a_global==0x1f 的 sprite,**index 0 = 透明**(批次轉 PNG 時設 colorkey alpha=0)。已對照 free 同 sprite 確認:Amiga sprite 背景確為 index 0(對應 free 版的 magenta 透明區)。

## 解碼器與工具

- **`tools/amiga_decode.py`** — 完整圖形解碼器:`parse()`(檔頭)、`lzss_decompress()`(Okumura +3)、`decode_frames()`(→ index 2D array + palette)、`palette_rgb()`。CLI:`python3 amiga_decode.py <resource>` 印 header/解壓資訊。
- **`tools/amiga_batch.py`** — 批次把全部 62 個圖形資源轉 PNG 到 `qa-amiga/all/`(多幀排成水平 strip,sprite index0 設 colorkey 透明),印驗證表(對照 free DOS 尺寸)。
- `tools/amiga_hunks.py` — hunk 執行檔解析(23 CODE + 2 DATA)。
- `tools/amiga_disasm_dump.py <seg> <start> <end>` — capstone M68K 反組譯。
- `tools/amiga_disasm_scan.py` — 解壓特徵掃描。
- `tools/amiga_relocs.py` / `find_callers.py` / `find_ptr.py` / `refto.py` / `find_strings.py` — reloc/呼叫/字串分析(reverse 期間用)。

## 執行檔內的第二個 LZSS(text/map,非圖形)

執行檔 SEG22@0x1ef40 另有一個 Okumura LZSS,**dict 填 0x20、length 同 +3**,但那是**文字/地圖資料**用(`LAND.ORG` 地圖檔未壓縮,開頭即大量 0x20 = 空 tile)。圖形 codec 與它同演算法、不同呼叫點,參數(dict fill)與資料用途不同。早期誤用它(dict 0x20 + 錯的 stream 起點)解圖形 → 雜訊,是另一條繞路。

## 導入 openkb 的下一步

1. 寫 `KBFAMILY_AMIGA` loader,或離線把 `amiga_decode.py` 批次轉 PNG 當 free 格式 theme 目錄。
2. sprite 的透明色:peas 等 a=0x1f sprite,index 0 應設 colorkey 透明(對照 free 版同 sprite 確認透明 index)。
3. palette:每張圖自帶 32 色 palette,導入時 per-image 套用。

## 工具與紀律

- amitools / capstone / pillow 一律 docker uv venv(`ghcr.io/astral-sh/uv:python3.12-bookworm-slim`)。
- ROM 衍生 PNG 放 `qa-amiga/`(gitignore),不入 repo。
- 相關:`~/.claude/skills/retro-game-remake/references/08-genesis-rom-graphics.md`(Genesis 對照,LZSS 同源)。
