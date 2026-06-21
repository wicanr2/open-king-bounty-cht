# 多版本素材 + 音樂切換 (F8 / F9) — 可行性分析與計畫

需求:F8 切換**美術主題**、F9 切換**音樂版本**,涵蓋 free / DOS / Amiga / Apple II / FM Towns / PC98 / Genesis。原版資料在 `original_kb/`。

## 現況:openkb 實際能載入的格式

| 版本 | 檔案 | openkb 圖形 loader | 音樂 |
|---|---|---|---|
| free | data/free/*.png+ini | ✅ GNU 模組 | tune .ini (簡易方波) |
| DOS | 256.CC/416.CC/KB.EXE | ✅ DOS 模組 (已驗證) | DOS tune (PC 喇叭式方波) |
| Genesis | .bin (512KB Sega ROM) | ✅ MD 模組 (md-rom.c) | YM2612 (ROM 內,需另解) |
| FM Towns | ROM_JP zip | △ 部分 (data/fmtowns 存在) | EUP/OPN2 (你提供場景表) |
| Amiga | .adf 磁碟映像 | ❌ 無 loader | MOD/自訂 |
| Apple II | .nib/.dsk 磁碟映像 | ❌ 無 loader | — |
| PC98 | .d88/3.5" 磁碟 | ❌ 無 loader | FM (YM2608) |

**關鍵結論:** openkb 圖形只支援 **free / DOS / Genesis**(及部分 FM Towns)。Amiga / Apple II / PC98 的圖形要從磁碟映像逆向(各自磁碟格式 + 壓縮 + 圖形格式),屬重大逆向工程,每個數天起跳。

## F8 美術主題切換 — 計畫

**Tier 1 (近期可行):** free ↔ DOS ↔ Genesis 三者循環。openkb 都能載,只需:
- 同時註冊多個圖形模組,F8 切換「目前使用哪個圖形模組」(改一個全域 + KB_Resolve 圖形偏好用它)。
- 文字一律維持 free 中文 (型別感知解析已具備)。

**Tier 2 (大工程,逐版評估):** Amiga / Apple II / PC98 / 完整 FM Towns 圖形。
- 需為各磁碟格式寫 loader + 圖形解碼 (參 retro-game-remake skill 的 format-cracking)。
- 風險高、工時長;建議逐版獨立評估,非本階段。

## F9 音樂版本切換 — 計畫 (建議走「錄音檔」路徑)

各版本音樂格式 (EUP/YM2612/MOD/FM) 各需專用播放器,逆向成本高。**務實作法:用各版本的錄音 (OGG/WAV) 依場景播放**,F9 切換音樂組。openkb 已能播 WAV (free-snd wavFile + SDL 轉換),OGG 需加 SDL_mixer 或解碼 (評估)。

**場景 → 音軌對應 (採用使用者提供的 FM Towns OPN2 真機錄音表):**

| openkb 場景 / 觸發 | FM Towns 曲目 |
|---|---|
| 片頭 logo | StarCraft Logo Jingle |
| 開場 | Opening |
| 標題 | Title |
| 城堡 (進 castle) | Castle |
| 大陸 1 Continentia | Field 1 |
| 大陸 2 Forestria | Field 2 |
| 大陸 3 Archipelia | Field 3 |
| 大陸 4 Saharia | Field 4 |
| 一般戰鬥 | Battle 1: Normal Combat |
| 攻城戰 | Battle 2: Castle Siege |
| 戰敗 | Battle Lose / Bad Ending |
| 勝利 | Battle Win v2 |
| 結局 | Ending Theme |

> 注意:openkb 目前**只有 SFX 觸發 (走路/撞擊/寶箱/勝敗)**,沒有「場景背景音樂」框架。要播放上述 BGM 需**新增 BGM 子系統**:依「目前場景」(地圖大陸/城堡/戰鬥) 播放並循環對應音軌,場景切換時換曲。這是 F9 的主要工程。

**F9 實作步驟 (建議):**
1. 取得各版本音樂為**分軌音檔** (使用者提供,或從錄音依時間戳切軌;版權同原版美術,不入公開 repo)。
2. 加 BGM 子系統:`bgm_play(scene)` 依場景循環播放;openkb 各場景進入點呼叫。
3. F9 在已備妥的音樂組間循環 (DOS/FMTowns/… 各一組分軌)。
4. 音訊解碼:WAV 直接可;OGG 需 SDL_mixer (評估加入)。

## 版權

各平台原版美術 / ROM / 音樂受版權保護,**一律不入公開 repo、不隨公開包散布**;比照 DOS 原版美術 (自動偵測 + 個人版打包),僅供已擁有正版者本機使用。

## 建議優先序

1. **先把核心中文化 + 既有修復穩定發版** (free 公開版 + DOS 個人版)。
2. **F8 Tier 1** (free/DOS/Genesis 美術循環) — 中等工程,openkb 原生支援。
3. **F9 BGM 子系統 + FM Towns 錄音** — 中大工程,需音檔素材。
4. Amiga/Apple II/PC98 圖形 loader、其他版本音樂 — 大型逆向,逐項評估。
