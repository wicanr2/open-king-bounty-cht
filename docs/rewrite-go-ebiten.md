# 重寫計畫:Go + Ebiten,手機優先

把《御封戰將》(openkb 繁中)用 **Go + [Ebiten](https://ebitengine.org)** 乾淨重寫,**手機為第一目標**(Android / iOS),桌面(Win/mac/Linux)與 Web(WASM)一併順帶。C 版 openkb-cht 作為**行為真值 oracle**,不移植其 runtime。

> 前提:本次允許派 **subagent 調用便宜模型執行 coding**。分工策略見 §6(依 `rulebook/45`)。
> 架構紀律見 §2(依 `rulebook/70` 深模組 / 垂直切片)。方法論同 `retro-game-remake`(反編/現有實作當 oracle,不照抄)。

## 0. 為什麼是 Go + Ebiten(且值得重寫)

- **一份程式碼、全平台**:Ebiten 是純 Go 2D 引擎,同一份碼可出 **Android / iOS / Windows / macOS / Linux / Web(WASM)**。手機用 `gomobile bind` / `ebitenmobile`,取代目前 C 版「SDL2 桌面 + 另一套 Android NDK + 觸控 overlay hack」的雙軌維護。
- **記憶體安全 = 這半年一堆 bug 直接消失**:C 版修過的 `NULL deref`、`KB_fgets` 的 `fread/fseek` 文字模式錯位、手動 `mkdir` 路徑雙 data、指標截斷…在 Go 裡結構上不會發生。Ebiten 每幀全畫,戰鬥殘影類問題(overlay 沒清)也自然消失。
- **好測**:遊戲邏輯與渲染分離 → 邏輯層可 headless 跑,對 C oracle 做 **parity 測試**。
- **最難的部分已完成**:所有資料格式都破解、遊戲公式都摸透(見 §3),重寫是「翻譯已知邏輯」而非「逆向未知」。

## 1. 我們已經有的資產(重寫的地基,別重挖)

| 資產 | 現況 | 重寫怎麼用 |
|---|---|---|
| **資料格式** | 全破解:free `*.ini/*.txt` 結構、DOS `256.CC/416.CC`、Genesis ROM、Amiga GAME、`cjk24.bin` 點陣字 atlas、`land.org` 地圖 | 把解碼器**移植成 Go**(格式已知,照抄結構即可) |
| **遊戲邏輯/公式** | 全摸透:兵種數值表(`bounty.c`)、領導力(base+寶箱,end_week 重設)、`army_leadership = leadership - Σ(hp×num)`、寶箱骰法、惡棍佈置、晉升、法術、戰鬥 grid/AI | 當**規格**在 Go 重寫;C 檔逐一當該功能的 oracle |
| **繁體翻譯** | `data/free/*.ini/*.txt`(兵種/法術/城鎮/劇情)+ `cjk24.bin` 字型 | **直接沿用**(是資料,不是碼) |
| **已知 bug 清單** | issue #1–#5 + 半年修的十幾個(savedir/路徑/NULL/SDL2/KB_fgets/戰鬥殘影/寶箱領導力) | **變成回歸測試**:Go 版一開始就正確,並寫測試釘住 |
| **手機觸控設計** | `docs/android/ui-design.md`(keymap 驅動觸控)+ `retro-keyboard-to-touch` 方法論 | 觸控 UI 直接照此設計,不再是 overlay hack |
| **C oracle** | openkb 可 headless(`KB_DEBUG_COMBAT`、`SDL_VIDEODRIVER=dummy`)、固定 seed | 產生「同輸入→期望輸出」黃金樣本供 parity |

## 2. 架構(深模組 · 垂直切片)

新 repo(建議 `open-king-bounty-go`),`internal/` 下**按功能切**,不按抽象層攤平:

```
cmd/openkb/            桌面進入點 (main;熱點,只旗艦動)
mobile/               gomobile/ebitenmobile bind (Android AAR / iOS framework)
internal/
  kbdata/             ★資料層:讀解所有格式 → 乾淨 Go 型別 (Assets)。窄介面:Load(dir) (*Assets, error)
  gamestate/          ★純遊戲狀態與規則 (troop/army/leadership/chest/villain/spell/world)。無渲染、可 headless、吃固定 RNG
  combat/             ★戰鬥狀態機 + AI (grid/move/attack/morale)。無渲染
  render/             Ebiten 繪製:worldmap / combat / ui / cjktext (吃 gamestate 的唯讀快照)
  input/              action 事件:桌面鍵盤 + 手機觸控 → 同一套 Action(keymap 驅動)
  screen/             畫面流程 (title/charselect/town/shop/combat) 狀態機
  save/               存讀檔 (跨平台路徑 os.UserConfigDir;不重蹈 C 版 mkdir 雷)
  audio/              BGM (Ebiten audio + vorbis OGG,FM-Towns 完整版用)
docs/ assets/(free 公開) 
```

- **窄介面**:`kbdata.Load()` 回一包唯讀 `Assets`;`gamestate` 對外只暴露「動作 + 唯讀快照」;`render` 只讀不寫狀態。
- **每模組自己的語言**,對外用 ubiquitous language(沿用 C 版 `CONTEXT.md`)。
- **RNG 可注入**(interface),parity 測試才能對 seed。
- 拒絕:提早抽象、pass-through、把邏輯漏進 render。

## 3. 對 C oracle 的 parity 策略(正確性的錨)

- C openkb 加最小 hook,固定 seed 印出關鍵狀態(建角後隊伍、寶箱骰值、戰鬥每回合、領導力數學)→ 存成黃金樣本 JSON。
- Go `gamestate`/`combat` 用**同一套公式 + 同一 RNG 演算法**,對同輸入比對輸出。不符即修 Go(C 是真值)。
- **回歸測試釘住已修 bug**:寶箱領導力過週仍在、savedir 首啟自動建、路徑不雙 data、戰鬥移動無殘影…每條一個 test。

## 4. 分期(每期可交付、可驗)

| 期 | 內容 | 驗收 |
|---|---|---|
| **P0 骨架** | Go module + Ebiten 空視窗 + CI(桌面交叉編 + `ebitenmobile` Android/iOS smoke) | CI 綠、視窗開得起來 |
| **P1 資料層** | `kbdata` 移植解碼器:free ini、`cjk24` 字、land 地圖、tileset。 | 載入並畫出一格 tile + 一個中文字 |
| **P2 世界地圖** | 地圖渲染 + 移動 + 迷霧 + sidebar + 狀態列(全中文) | 走地圖、切迷霧、UI 正確 |
| **P3 遊戲邏輯核** | spawn/建角、army、leadership、寶箱、城鎮/招兵、晉升 | **parity vs oracle** + 寶箱領導力回歸測試 |
| **P4 戰鬥** | grid/單位/移動/攻擊/AI/士氣 | parity(同 seed 同結果)+ 移動無殘影 |
| **P5 目標系統** | 惡棍佈置/追捕/勝利、法術、寶物、每週結算 | 可破關鏈 headless 驗 |
| **P6 畫面打磨** | 所有畫面 + CJK 銳利 + 原版開場(完整版)+ 四主題 F8(選) | 截圖逐張目視 |
| **P7 手機** | 觸控 UI(keymap 驅動:D-pad+A/B+情境字母列)+ gomobile Android/iOS | 真機:啟動/走圖/戰鬥/選單可玩 |
| **P8 出貨** | 桌面 + APK/IPA + WASM;parity 測試套件;playtest | 全平台包 + 綠測 |

## 5. 手機(第一目標)

- Ebiten 原生吃觸控;**不重寫輸入,`input` 層把觸控與鍵盤都映成同一套 Action**(照 `retro-keyboard-to-touch`)。
- 觸控配置照 `docs/android/ui-design.md`:左下虛擬 D-pad、右下 A/B、**依當前 keymap 動態浮出情境字母列**(城鎮/商店選單)、命名用系統 IME、系統選單收 ☰。
- 打包:`ebitenmobile bind` 產 Android `.aar` / iOS `.framework` → 薄殼 Activity/ViewController;或直接 `gomobile build` 出 APK。存檔走 app 內部儲存。
- 音樂:Ebiten `audio` + `vorbis` 解 OGG(FM-Towns,完整版)。

## 6. Subagent 分工(便宜模型執行 coding · 依 `rulebook/45`)

**旗艦(主迴圈,我)自己做**:架構與模組介面、`gamestate`/`combat` 的公式與狀態機、AI、parity harness 設計、**每件交付的抽驗把關**、熱點檔(`cmd/openkb/main`、狀態核心)。

**派 `sonnet`**(有明確規格 + 單測 + 檔案邊界的套件級實作):
- 單一解碼器(如 free ini parser、cjk atlas loader)——附「C oracle 檔 + 格式說明 + 期望輸出」。
- 單一畫面/UI 元件(如 sidebar、招兵框)——附版面規格 + 目視驗收點。
- 有既定方法論的 RE 移植(給錨點:哪個 C 檔/函式是真值)。

**派 `haiku`**(機械、不吃判斷):
- 移植資料表(`bounty.c` 的 troop/class 表 → Go struct 常量)。
- boilerplate、資產盤點、跑既有工具批次匯出、格式轉換。

**規則**(否則便宜模型產出品質崩):
- prompt 要素:工作目錄 + 工具呼叫範例、**已知錨點**(該功能對應的 C 檔/函式「先讀」清單)、明確產出物與格式、**「只動哪些檔」邊界**(絕不與主迴圈改同檔;`main`/狀態核心只留旗艦)、「不 commit/push,回報 X」收尾。
- **併發 ≤3**、檔案邊界互不重疊。
- **把關不可省**:每件交付先跑 `go test` + `go build` +(畫面類)截圖,再由旗艦 commit。
- 規格寫「預設值 + 查證優先」:公式以 C oracle 為準,agent 若發現 C 與手冊不符先回報。

## 7. 風險與對策

- **gomobile/iOS 工具鏈**:iOS 需 macOS + Xcode(用 CI macOS runner);先確保 Android 通,iOS 視資源。P0 就驗 `ebitenmobile` smoke,別留到最後。
- **CJK 字**:直接沿用 `cjk24.bin` 點陣 atlas(最省事)或 `ebiten/text` + TTF;先用點陣保證與 C 版一致。
- **RNG parity**:必須在 Go 重現 C 的 `KB_rand` 演算法(不是用 Go `math/rand`),否則對不上 seed。P3 前先把這顆釘死。
- **版權**:同現行政策——free 自由美術/資料進公開 repo;原版美術/FM-Towns 音樂只給正版擁有者的完整版,不散布。
- **範圍**:先 Android + 桌面 + WASM 打通可玩;iOS 與四主題 F8 列為 P6/P7 加值,不擋主線。

## 8. 立即可起手(P0,待核可)

1. 建 repo `open-king-bounty-go`,`go mod init` + Ebiten 依賴 + 空視窗 + GitHub Actions(桌面交叉編 + Android `ebitenmobile` smoke)。
2. 把本檔 §3 的 C oracle hook 先加到 openkb(固定 seed 印關鍵狀態),產第一批黃金樣本。
3. P1 派工:`haiku` 移植 `bounty.c` 資料表、`sonnet` 寫 free ini + cjk atlas loader;旗艦定 `kbdata.Assets` 介面 + 把關。

> 待決策(§9):新 repo 名稱?iOS 要不要納入 P0 目標(需 Mac/Xcode)?四主題 F8 是否列 P1 必要(或先只做 free/DOS)?
