# ADR 0001 — 引擎選擇與 CJK 渲染策略

- 狀態:已接受 (2026-06-20)

## 背景

要把 King's Bounty (1991) 做成繁體中文、跨四平台 (Windows/AppImage/macOS/Android) 的可遊玩版本。以 openkb 開源重製引擎為基底 (C + SDL 1.2,內部畫布 320×200,文字渲染為純 ASCII 8×8 字格)。

## 決策

1. **資料模組選 `free`**:玩家不需自備原版;文字在可讀的 `.ini`/`.txt`,改檔即生效。`bounty.c` 寫死陣列是 DOS 模組 fallback。
2. **引擎升 SDL 1.2 → SDL 2**:Android 實質要求 SDL2;SDL 呼叫集中在 `env-sdl.c` 單一 adapter,改動可控。
3. **CJK 渲染照搬 1oom (MOO1) 模式**:邏輯座標維持 320×200 (UI 與滑鼠 hit-test 不重映射),只在合成層做 2× 放大 (640×400),24×24 漢字以點陣 atlas 疊圖,前進寬度 = glyph/2。直接複用 1oom 的 `cjkfont.h/c` 與打包腳本。
4. **字型綁定 Noto Sans CJK TC (OFL)**,烘烤成 `cjk24.bin` atlas,不依賴使用者機器字型。

## 理由

1oom 證明「邏輯層維持低解析、視覺層才放大」可行,同時滿足高解析畫布 (rule 81) 又免去重映射所有 UI 座標的大工程。1oom 與 King's Bounty 同為 320×200 DOS 策略遊戲,模式高度可移植。

## 後果

- 主要工作量落在 `env-sdl.c` 的 SDL2 移植,其餘 CJK 基礎建設可大量複用。
- 合成尺寸 640×400 (乾淨 2×) vs 640×480 (4:3) 待定;傾向 640×400 避免非整數縮放瑕疵。
- 譯名以 `original_kb/` 官方 1990s 繁中手冊為權威來源 (待 OCR)。
