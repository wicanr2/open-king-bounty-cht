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
u16  a_global; u16 0x0000
u16[32] palette                  // 固定 32 色 = 5 bitplanes,12-bit 0RGB
u16  comp_size; u16 0x0000       // LZSS 壓縮流大小
u16  out_size                    // **解壓後 planar bitmap 大小** (= w/8 × h × 5 × frames)
... LZSS stream ...              // 緊接在 out_size 之後
```
> **關鍵踩雷**:stream 起點在 `out_size`(u16)**之後**,不是在 comp_size 之後。早期把 out_size 那 2 bytes 當成 stream 開頭 → 整張圖前一小段對、其餘全 drift 成雜訊,卡關最久就是這 2-byte off-by-two。

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

## 驗證結果(全部 out_size 精準命中、視覺正確)

| 資源 | count | 尺寸 | out_size | 內容 |
|---|---|---|---|---|
| cstl | 1 | 240×102 | 15300 | 城堡場景(藍天白雲、石牆、紅旗) |
| town | 1 | 240×102 | 15300 | 城鎮(白屋紅瓦、市政廳、綠樹) |
| cave/dngn/frst/plai | 1 | 240×102 | 15300 | 各 location 場景 |
| tileseta | 36 | 48×34 | 36720 | 36 個地圖 tile(城堡/水/樹/橋…) |
| title | 1 | 320×200 | 40000 | 標題全螢幕圖 |
| select | 3 | 288×184 | 35440 | 選單背景 |
| view | 14 | 48×34 | 14280 | UI 元件 |
| comtiles | 15 | 48×34 | 15300 | 戰鬥 tile |
| peas | 4 | 48×34 | 4080 | 農民 sprite(4 幀持乾草叉) |
| spri/drag/knig… | 4/4/1 | 48×34 / 96×102 | — | troop/monster sprite |

## 解碼器與工具

- **`tools/amiga_decode.py`** — 完整圖形解碼器:`parse()`(檔頭)、`lzss_decompress()`(Okumura +3)、`decode_frames()`(→ index 2D array + palette)、`palette_rgb()`。CLI:`python3 amiga_decode.py <resource>` 印 header/解壓資訊。
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
