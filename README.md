# 御封戰將 — King's Bounty 繁體中文版

把 1991 年的策略冒險經典《King's Bounty》(臺灣 1990 年代官方譯名「**御封戰將**」) 帶回現代螢幕,並補上一份遲到三十年的繁體中文。本專案以開源重製引擎 [openkb](https://sourceforge.net/p/openkb/) 為基底,加上中文渲染、翻譯與跨平台打包。

> **目前狀態:規劃 / 開發初期。** 完整的開發計畫見 [`PLAN.md`](PLAN.md),術語對照見 [`CONTEXT.md`](CONTEXT.md)。本 README 在可遊玩版本完成後會改寫成正式的玩家向說明。

## 這是什麼

- **引擎**:openkb,一套用 C + SDL 重寫的 King's Bounty 開源實作 (原作者 Vitaly Driedfruit)。本 repo 是它的 fork。
- **中文化路線**:採用 openkb 的 `free` 自由資產模組 —— 玩家**不需要自備原版遊戲**即可遊玩,所有文字皆為可翻譯的純文字資料。
- **中文渲染**:沿用與《銀河霸主》(1oom) 中文化相同的作法 —— 邏輯畫布維持原始 320×200、合成層 2× 放大,漢字以 24×24 點陣疊圖呈現,字型烘烤自開源的 Noto Sans CJK TC。
- **目標平台**:Windows / Linux AppImage / macOS / Android。

## 規劃中的階段

| 階段 | 內容 |
|---|---|
| P0 | Docker build 環境 + 英文 baseline |
| P1 | 引擎 SDL 1.2 → SDL 2 移植 |
| P2 | CJK 文字渲染 (UTF-8 + 點陣 atlas) |
| P3 | 資料檔翻譯 (兵種 / 城鎮 / 法術 / 劇情) |
| P4 | 原始碼內嵌字串翻譯 (UI / 訊息) |
| P5 | 可玩性驗證 |
| P6 | 四平台打包 |
| P7 | 玩家向文件與攻略 |

詳見 [`PLAN.md`](PLAN.md)。

## 授權

- **引擎程式碼**:GNU GPL v3 或更新版本 (承襲 openkb)。
- **`free` 模組自由資產**:CC-BY-SA 4.0 與 GPL v3 雙重授權 (承襲 openkb)。
- **中文字型**:Noto Sans CJK TC,SIL Open Font License。
- 原版遊戲資料 (`*.CC`、`KB.EXE`) 與官方手冊 PDF 受版權保護,**不包含在本 repo**,玩家如需以原版資料遊玩請自備合法副本。

## 致謝

- **openkb** 原作者 Vitaly Driedfruit 與貢獻者。
- 1990 年代將《King's Bounty》以「御封戰將」之名引進臺灣、留下官方繁中手冊的代理與譯者。
- 原作 Jon Van Caneghem / New World Computing。
