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
| 地圖資料 | LAND.ORG(16384 = 大陸地圖 tile index?) |

## 圖形格式 (部分破解)

檔頭結構(big-endian):
```
u16  count          // 子圖數 (tileseta=36, comtiles=15, view=14, tilesalt=9, title=1, cstl=1)
count × {           // 每子圖描述子 (3 words,待最終確認語意)
  u16 a             // tileseta 全 0;peas/comtiles 首項 0x001f → 疑為 flag/x
  u16 w             // 0x30 = 48
  u16 h             // 0x22 = 34
}
u16[]  palette      // 12-bit RGB:0RGB,R=(c>>8&0xF)*17, G=(c>>4&0xF)*17, B=(c&0xF)*17
                    // 色數 = 2^bitplanes (待定 16 或 32)
...    planar 點陣   // ← 此段為壓縮 (見下)
```
- location 圖範例 cstl:`count=1, desc=(0,240,102)`,palette 起 `0fff 0ccc 0aac…`。
- title:`count=1, desc=(0,320,200)`,palette 32 色(`0fff 0cce 0aac…`)→ 5 bitplanes。

## ⚠️ 卡點:點陣是壓縮的

- 把 palette 後的資料當 **raw planar**(interleaved 與 sequential、3/4/5 planes 都試)算繪 cstl → **全是雜訊**。
- 推論:點陣經壓縮。`file` 對 troop sprite(peas 等)報 **「TTComp archive data, 1K dictionary」**(TTComp 為已知 Amiga LZ 系壓縮);tile/location 圖(tileseta/cstl,`file` 報 "data")疑為 **NWC 自有 RLE**。
- size 不符 raw 佐證:tileseta 17424B,扣 header(2+36×6=218)+palette,餘量 < 36 個 48×34×4bpp(29376B)所需 → 壓縮。

## 下一步 (待續)

1. **破 TTComp**(已知演算法):decompress troop sprite → raw planar → 套 palette → PNG 驗證。
2. **逆 location/tile 的 NWC RLE**:對照 Amiga 執行檔 `King's Bounty`(68000 反組譯,capstone M68K)找載入/解壓常式;或差分已知 free 版同圖找 RLE 規則。
3. 確認 bitplane 數與描述子語意(a 欄位)。
4. 導入 openkb:解碼→PNG→當 free 格式 theme 目錄,或寫 `KBFAMILY_AMIGA` loader。

## 工具與紀律

- amitools / capstone / pillow 一律 docker uv venv(`ghcr.io/astral-sh/uv:python3.12-bookworm-slim`),不污染系統。
- ROM 衍生 PNG 放 `qa-*/`(gitignore),不入 repo。
- 相關:`~/.claude/skills/retro-game-remake/references/08-genesis-rom-graphics.md`(Genesis 對照)。
