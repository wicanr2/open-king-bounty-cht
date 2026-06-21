# Amiga 版 King's Bounty 素材格式逆向 (進行中)

目標:從 Amiga 版 `.adf` 抽取美術(地圖 tiles / location 圖 / UI / troop sprite)與音效,
導入 openkb 引擎(供 F8 主題切換)。本檔記錄逆向進度,未完成。

## 來源

- ROM:`Kings_Bounty_Amiga_ROM_EN.zip` → `King's Bounty (1990)(New World Computing).adf`(880KB AmigaDOS OFS)。
- 開發者:New World Computing(與 Might & Magic 同社,格式可能共血緣)。
- 版權:`.adf` 與解出的素材一律 gitignore、不入公開 repo(`amiga-orig/`)。

## 解開磁碟 (已完成)

用 amitools(docker uv venv,`pip install amitools`):
```
xdftool "King's Bounty ....adf" list           # 列檔
xdftool "King's Bounty ....adf" unpack out_dir  # 全解
```
→ `KB/GAME/` 下 **72 個具名資源**,檔名與 free/DOS 版對應(peas/spri/cstl/comtiles/tileseta…)。
另有頂層 `King's Bounty`(AmigaOS 執行檔 138KB,演算法 oracle)、fonts、c/devs/libs(AmigaDOS 系統檔,無關)。

### 資源分類
| 類別 | 檔名 |
|---|---|
| 地圖 tiles | `tileseta`(36 子圖)、`tilesetb`、`tilesalt` |
| location 圖 | cstl(城堡)、town、cave、dngn、frst、plai |
| UI / 全圖 | title、select、view、comtiles、cursor、nwcp、endpic、demo |
| troop / 怪物 sprite | peas、spri、mili、arch、… (約 40+,與 troop 名對應) |
| 音效樣本 | Bash、Bump、Death、Horns、King、Vict、Treas、Walk、Tele(8-bit signed PCM) |
| 地圖資料 | LAND.ORG(16384 = 大陸地圖 tile index,**未壓縮**,以 0x20 為空 tile) |

## 圖形容器檔頭 (已完全破解)

檔頭結構(big-endian),已用 12-bit palette 約束逐欄驗證:
```
u16  count                       // 子圖數 (tileseta=36, comtiles=15, view=14, tilesalt=9, title=1, cstl=1, peas=4)
count × {                        // 每子圖描述子 (3 words)
  u16 a                          // plane/旗標:peas=0x1f, tileseta/cstl/title=0x0000
  u16 w                          // 0x30=48 (tile), cstl=0xf0=240, title=0x140=320
  u16 h                          // 0x22=34 (tile), cstl=0x66=102, title=0xc8=200
}
u16 a_global                     // peas=0x1f, 其餘=0x0000
u16 0x0000
u16[32] palette                  // 固定 32 色 (5 bitplanes),12-bit 0RGB:R=(c>>8&0xF)*17 …
u16 comp_size                    // **壓縮後位元組數**(peas=0x854, cstl=0x204d, tileseta=0x42ec, title=0x2fcb)
u16 0x0000
... 壓縮點陣 ...                  // stream 緊接在 comp_size 之後 (peas=0x62, cstl=0x50)
```
- `comp_size` 已驗證:peas 0x854=2132 vs stream 實長 2130;cstl 0x204d=8269 vs 8271;tileseta 0x42ec=17132 vs 17134;title 0x2fcb=12235 vs 12237。一律 = 壓縮資料大小(±2 對齊)。
- palette 一律 **32 色 = 5 bitplanes**。各檔已逐一用「palette word 高 nibble 必為 0」確認剛好 32 entries。
- 全部資源都壓縮:raw planar 5-plane 大小 > 檔案大小(cstl 15300>8351、tileseta 36720>17424、peas 4080>2232)。

## 兩個解壓常式 (已從執行檔反組譯定位)

執行檔 hunk 結構:23 個 CODE 段 + 2 個 DATA 段(masked MEMF flags 後完整解析,見 `tools/_disasm_hunks.py`)。

### (1) Text/Map LZSS — SEG22 @ fileoff 0x1ef40 (已 100% 破解,但**非圖形用**)

教科書級 Haruhiko Okumura LZSS,反組譯逐指令確認(`tools/amiga_decode.py` 的 `lzss_decompress` 為忠實移植):
- 4096-byte ring dictionary,**初值填 `0x20`(space)**,寫指標 r0 = 0xFEE。
- control byte:8 個 flag bit,LSB first;**bit=1 → literal**(`move.b (a0)+`),**bit=0 → match**。
- match:2 bytes → 12-bit offset = `b1 | ((b2&0xF0)<<4)`,count = `(b2&0xF)+2`,複製 count+1 bytes。
- outlen 由 caller 從 struct+4 傳入,stream 在 struct+8。

**關鍵結論:此 LZSS 是給文字/地圖資料用的,不是圖形。**證據:
- 字典初值 0x20 = 文字最常見字元;`LAND.ORG`(地圖,未壓縮)開頭就是大量 `0x20`(= 空 tile 索引)。
- 用它解 peas/cstl 圖形 stream → 輸出被 0x20 灌爆,planar/chunky 各排列**全雜訊**;0x20=`00100000` 在 planar 下只會在 x%8==2 產生固定豎線(已數學證明不相容 planar)。
- 全段掃描:除此之外**找不到第二個含 sliding-window 索引讀取(`move.b (aX,dY.w)`)的解碼器**。

旁邊還有一個 word bit-reverse 小函式(0x1f014:對每個 16-bit word 做 `roxr/roxl` ×16 反轉位元序),疑為 planar 後處理,但單獨套用無法救回圖形。

### (2) 圖形解壓 — **未破解(卡點)**

圖形 stream 與 text-LZSS 明顯不同:
- peas@0x62:`0ff0 dd00 eefa 0c00 …`
- cstl@0x50:`3bc4 ffd5 c58a c15c 4418 00df 00ff f9d1 …`(注意 `00df 00ff 00fa` 的 0x00+byte 交錯)

已系統性排除的假設(全部產生雜訊或內容錯誤):
| 假設 | 結果 |
|---|---|
| text-LZSS(dict=0x20 或 0x00),seq/introw/chunky | 雜訊 |
| chunky 8-bit(多種寬度 48/96/120/192) | 雜訊 |
| PackBits / ByteRun1(多種 threshold、lit-first/rep-first) | **長度吻合(15347≈15300)但 uniformity 僅 5–7%**,無大塊 0xff/0x00 背景 → 內容錯 |
| word/byte bit-reverse 後再 planar | 雜訊 |

**最有希望的線索**:PackBits 系列長度幾乎完全吻合(cstl 解出 15347 vs 目標 15300),且 cstl PackBits 渲染出現**水平條帶**(row 結構浮現)→ 圖形 codec **很可能是 RLE 家族,且可能 per-row / per-plane 獨立打包**,只是控制位元組語意或 plane 排列尚未對上。`00xx` 交錯也支持「0x00 = run 控制」的方向。

## 下一步 (待續)

1. **鎖定圖形解壓常式**:text-LZSS 已排除。圖形 codec 無 sliding window → 應為 RLE。需在執行檔找「讀 comp_size → 逐 byte 解 RLE 到 planar buffer」的常式。建議從「載入資源後第一個寫入大 planar buffer 的迴圈」反查,或追 `select`/`title`(全螢幕圖,結構單純)的繪製路徑。
2. **RLE 變體窮舉**:per-row length-prefixed、per-plane、control=0x00 觸發 run、ESC-byte RLE。以 cstl(背景大塊單色,最易驗證)為 oracle,評分用「最大 plane-row 連續同值長度」而非整體 uniformity。
3. 確認 plane 排列(seq vs introw)— 等 codec 對了再定。
4. 導入 openkb:解碼→PNG→當 free 格式 theme 目錄,或寫 `KBFAMILY_AMIGA` loader。

## 已產出工具

- `tools/amiga_hunks.py` — 解析 AmigaOS hunk 執行檔(mask MEMF flags),列 23 CODE + 2 DATA 段 fileoffset。
- `tools/amiga_disasm_scan.py` — 掃描解壓特徵(shift-dense 區、magic 常數 0x30/0x22/0x1f、tight copy loop)。
- `tools/amiga_disasm_dump.py` — 指定 seg + fileoffset 範圍反組譯(capstone M68K):`python3 amiga_disasm_dump.py <seg> <start_fileoff> <end_fileoff>`。
- `tools/amiga_decode.py` — 容器檔頭解析(`decode_resource`)+ 忠實 text-LZSS(`lzss_decompress`)+ palette 轉換(`pal_to_rgb`)。**圖形解壓待補**。

## 工具與紀律

- amitools / capstone / pillow 一律 docker uv venv(`ghcr.io/astral-sh/uv:python3.12-bookworm-slim`),不污染系統。
- ROM 衍生 PNG 放 `qa-*/`(gitignore),不入 repo。
- 相關:`~/.claude/skills/retro-game-remake/references/08-genesis-rom-graphics.md`(Genesis 對照)。
