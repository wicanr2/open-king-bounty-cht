# CONTEXT — 御封戰將中文化 術語表

跨人/agent 共用詞彙。譯名以 `original_kb/` 官方 1990s 繁中手冊為權威來源,待 OCR 後逐條回填。

## 專案核心詞

- **openkb** — King's Bounty 的開源 C/SDL 重製引擎 (SourceForge `p/openkb/code`)。本專案的引擎基底。
- **御封戰將** — King's Bounty 的官方 1990 年代繁中譯名 (出自官方手冊)。對外標題用此。
- **free 模組** — openkb 的純自由資產資料集 (`data/free/`),文字在 `.ini`/`.txt`,不需原版遊戲。本專案翻譯標的。_Avoid_: 把 free 模組說成「免費版遊戲」。
- **dos 模組** — 讀原版 KB.EXE + `256.CC`/`416.CC` 的資料集。僅供對照,不發佈。
- **資料字串** — 存在 `data/free/*.ini`/`*.txt` 的可翻譯文字 (約 70%)。
- **寫死字串 (hardcoded)** — 嵌在 `src/*.c` 的英文 (UI 標籤/訊息,約 30%),需改源碼重編。
- **KB_print** — 引擎核心文字渲染函式 (`src/env-sdl.c`)。CJK 改動的主要落點。
- **CJK 渲染路徑** — 為漢字新增的 UTF-8 解碼 + TTF glyph 渲染流程。

## 官方譯名 (摘自手冊 OCR,完整見 `docs/translation/glossary-draft.md`)

高信心 (官方手冊已列,可直接採用):
- **御封戰將** — King's Bounty (遊戲標題)。
- 職業:**武士** (Knight)、**遊俠** (Paladin)、**女巫師** (Sorceress)、**蠻俠** (Barbarian)。
- 14 法術、25 兵種、8 工藝品譯名已配出 (手冊以「中文(English)」並列,信心高);細節見草表。

待確認 (手冊未逐一給譯名,需另定或暫留英文):17 名 Boss、26 城鎮、26 城堡、大陸名 (Flandria…)、職業專有人名 (Sir Crimsaun…)。

## 待釐清 (Flagged ambiguities)

- 「系統中文字型」= 向量字型渲染 (TTF) vs 使用者機器已安裝字型 → 傾向前者 + 綁定 Noto Sans CJK TC,待確認。
- 邏輯畫布是否拉高 (320×200 維持 vs rule 81 升解析) → 待 P2 排版實測。
