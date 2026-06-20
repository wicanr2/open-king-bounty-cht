# 貢獻者與致謝

《御封戰將》(King's Bounty) 繁體中文版站在許多人與專案的肩膀上。本檔完整記錄各方貢獻與授權歸屬。

## 原作

- **King's Bounty**(1990),由 **Jon Van Caneghem** 設計,**New World Computing** 發行。
- 這款策略冒險經典是一切的源頭。本專案重現並中文化的,正是它的核心玩法:招募軍隊、追捕惡棍、尋找寶物、收復四大洲。

## 官方繁體中文譯本(譯名權威來源)

- 1990 年代於臺灣官方代理發行《御封戰將》,並留下官方繁體中文遊戲手冊(本專案參照的手冊掃描存檔編號 `DDSC-J-00055`;當年代理公司全名待考)。
- 本專案的譯名以這份官方手冊為**唯一權威來源**:法術、兵種、寶物、職業與遊戲標題,一律照手冊「中文(English)」並列的譯名沿用,不自創術語。
- 向當年的代理與譯者致敬。**譯名版權屬原代理**;本專案僅作對照引用,不散布手冊本身。

## openkb 開源引擎

- 作者 **Vitaly Driedfruit**(`driedfruit`)及貢獻者。
- 來源:SourceForge `p/openkb/code`。
- 授權:**GNU GPL v3 或更新版本**。
- 本專案為 openkb 的 fork,沿用其引擎架構與 `free` / `dos` / `fmtowns` 多模組設計。

## `free` 模組自由美術

- **missandei**、**Santiago Iborra**。
- 授權:**CC-BY-SA 4.0**(與 GPL v3 雙重授權)。
- 他們的自由美術讓玩家**無需自備原版遊戲**即可遊玩,是「免原版即可中文遊玩」這條路線得以成立的關鍵。

## 中文字型

- **Noto Sans CJK TC**(Google / Adobe,SIL Open Font License)。
- 烘烤為隨包綁定的點陣 atlas(`data/cjk24.bin`),不依賴使用者機器既有字型。

## 本中文化專案(2026)

- 繁體中文化工作內容:
  - **SDL 1.2 → SDL 2 移植**:視訊核心重寫、相容 shim、邏輯畫布固定 320×200 + renderer 縮放。
  - **CJK 點陣渲染管線**:640×400 合成層 + 24×24 漢字 atlas + 八方位黑外框,文字路徑加 UTF-8 解碼分流。
  - **全文翻譯**:`data/free/` 資料檔(兵種、城鎮、城堡、法術、寶物、惡棍故事、片頭與結局)+ 原始碼內嵌英文字串(UI 標籤、遊戲訊息)。
  - **跨平台打包**:Linux AppImage(可建置);Windows / macOS / Android 規劃中。
- 以 **Claude Code (Opus 4.8)** 協作完成。
- GitHub:[wicanr2/open-king-bounty-cht](https://github.com/wicanr2/open-king-bounty-cht)。

## 授權歸屬總表

| 元件 | 授權 | 散布於本 repo |
|---|---|---|
| 引擎程式碼(openkb + 本專案修改) | GNU GPL v3+ | 是 |
| `free` 模組自由資產 | CC-BY-SA 4.0 / GPL v3 | 是 |
| 中文字型 Noto Sans CJK TC | SIL OFL | 是(或烘烤後的 atlas) |
| 官方繁中手冊(譯名來源) | 版權屬原代理 | **否**(僅作對照) |
| 原版遊戲資料(`*.CC`、`KB.EXE`) | 版權所有 | **否**(玩家自備) |
