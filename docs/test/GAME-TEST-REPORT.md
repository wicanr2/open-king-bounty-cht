# 御封戰將中文化 — 遊戲測試報告

測試版本:commit `c10b419` 之後(含 CJK 排版修正)。引擎 openkb (SDL2 移植版),`free` 模組,繁體中文。

## 結論

中文化後的遊戲**可正常啟動、建立角色、進入世界地圖並操作**,中文在已測畫面**全部正確渲染**(清晰、無亂碼、無垂直重疊)。測試期間發現並修正 3 個問題(其中 1 個會導致一開始遊戲就崩潰)。戰鬥、城鎮內部、商店等需精確走位的畫面因 headless 自動化限制未能觸發,建議補一輪人工實機測試。

## 測試方法

- **環境**:Docker (`openkb-build-sdl2`,Debian + SDL2) + Xvfb 虛擬顯示,`docker/playtest.sh` 劇本式驅動 + ImageMagick 逐畫面截圖。
- **防掛機**:所有 `import`/`xdotool` 均加 `timeout`,`openkb` 以 `timeout 90` 包覆(早期一次 `import` 卡死導致容器空轉,已用 timeout 根治)。
- **輸入**:headless 無視窗管理員,字母鍵需 `windowfocus` + XTEST 真實輸入(`xdotool --window` 的 XSendEvent 合成事件 SDL2 對選角等不生效)。
- 截圖見 `docs/test/shots/`。

## 各畫面 CJK 驗證

| 畫面 | 截圖 | 中文內容 | 結果 |
|---|---|---|---|
| 模組選擇 / 片頭 | 01–02 | (ASCII) | ✅ 正常 |
| 製作名單 | 03 | 御封戰將 設計者：/ openkb 程式開發：/ free 美術設計： | ✅ 清晰 |
| 選角畫面 | 04 | 選角 A-D 或 L-讀取存檔 | ✅ 清晰 |
| 難度 / 命名 | 06–07 | (流程) | ✅ 可操作 |
| 世界地圖頂列 | 09, 16 | 選項 / 操作說明 / 剩餘天數:600 | ✅ 緊湊清晰 |
| 角色 / 狀態面板 | 14 | 領導力 / 每週佣金 / 金幣 / 法術威力 / 法術上限 / 捕獲惡棍 / 尋得寶物 / 駐守城堡 / 陣亡部眾 / 目前分數 | ✅ 對齊、無重疊 |
| 地圖移動 | 10–12, 16 | (走位,英雄移至城堡入口) | ✅ 正常 |

## 發現並修正的問題

1. **[嚴重,已修] 開始遊戲即崩潰**:`SDL_CreatePALSurface` 在 SDL2 以非零 mask 建立 8-bit surface 會回傳 NULL,導致 `GR_PURSE`(金袋圖)等資源無法解析、進遊戲時 core dump。屬 SDL1.2→SDL2 移植遺留,與中文無關。修正:8-bit surface 的 mask 改為 0(`src/lib/kbres.c`)。修後遊戲可正常進入並操作。
2. **[中等,已修] 密集面板中文垂直重疊**:CJK glyph 原以 24px 繪製(=12px 邏輯高),超過狀態面板 8px 邏輯行距 → 多行標籤上下相疊不可讀。修正:依原字高選 16px(=行高),並以 linear 平滑縮小(`src/env-sdl.c`)。
3. **[輕微,已修] 中文字距過寬**:CJK 原前進 2 格,16px glyph 在 32px 槽位顯得鬆散。修正:前進改 1 格,漢字相鄰緊湊(`src/env-sdl.c`、`src/inprint.c`)。

## 未涵蓋 / 待人工驗證

- **戰鬥畫面**(兵種中文名、戰鬥訊息)、**城鎮內部 / 城堡 / 商店 / 法術書**:需走位至特定地點或進入戰鬥才會出現,headless 自動走位難以穩定觸發,本輪未截到。建議人工實機各進一次確認中文與排版。
- **長敘事文字框**(artifact 描述、villain 通緝令):原文 `\n` 斷行沿用,中文每行字數較密,實機若超出文字框寬度可能需重新斷行。
- **角色命名輸入**:可輸入,但中文輸入法非目標(玩家以英數命名)。

## 已知設計取捨(非缺陷)

- **villain(17)/ 城鎮(26)/ 城堡(26)/ 大陸名為音譯**:官方手冊未提供這些專名的譯名,採一致風格自行音譯;法術 / 兵種 / 寶物 / 職業 / 標題則用官方手冊譯名。
- **C1/C2/C3 大陸佔位字串**暫譯為序數洲名;**debug 作弊選單**保留英文(開發者用)。

## 重現方式

```sh
docker build -t openkb-build-sdl2 -f docker/Dockerfile.sdl2 .
docker run --rm -v "$PWD":/src -w /src ghcr.io/astral-sh/uv:python3.12-bookworm-slim bash docker/build-font.sh
docker run --rm -v "$PWD":/src -w /src openkb-build-sdl2 bash -c 'sh docker/build.sh && bash docker/playtest.sh'
```
