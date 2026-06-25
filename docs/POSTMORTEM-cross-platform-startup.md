# Postmortem:Windows / macOS 開不起來 + 選角後閃退

對應 GitHub issue [#1](https://github.com/wicanr2/open-king-bounty-cht/issues/1)、[#2](https://github.com/wicanr2/open-king-bounty-cht/issues/2),修正版本 `v0.0.3-cht.3`。

使用者在 macOS / Windows 上回報:啟動就閃退(需手動建 `~/.openkb` 才能跑),選完難度後又跳一連串 `Unable to resolve resource DAT_*` 並當機。Linux(AppImage,native build)完全正常。最後查出是三層獨立問題疊在一起,而且只有在目標平台才會發作。

## 三層根因

### 1. 存檔目錄未遞迴建立(啟動就死)

預設存檔目錄是 `$HOME/.openkb/saves`(兩層),但 `test_directory()` 只做單層 `mkdir`。首次啟動時父層 `~/.openkb` 還不存在 → `mkdir` 回 `ENOENT` → 觸發 `Can't start without a proper savedir`。AppImage 因 `AppRun` 自己做了 `mkdir -p` 而倖免,所以只有 Windows / macOS 包中招。

**修正**:`test_directory()` 改為逐層遞迴建立(`mkdir -p` 語意),中間層錯誤忽略、最後 `stat` 驗證。

### 2. 缺資料時 NULL 解參考(選角後崩潰)

`refill_rules()` 對每個資料表都有 null 防護,但底層轉換 / 字串清單函式沒有:`KB_strlist_ind(NULL)` 會直接對 `*list` 解參考,`GNU_downto_byte(NULL)` 在 `b[i]=src[i]` 解參考。任何資源解析回 NULL 都會炸。

雪上加霜:在 Windows 上 `KB_errlog()` 會彈出 `MessageBox("Critical Error")`。所以每個未解析資源都跳一個阻斷式對話框 —— 在 Linux 只是 stderr 噪音的訊息,在 Windows 變成擋住玩家的牆。

**修正**:
- `KB_strlist_ind`、`GNU_downto_byte/word`、`GNU_spell/artifact_downto_byte`、`GNU_parse_troop` 全面 NULL-safe,缺資料時回退引擎內建預設(`bounty.c` 的 `troops[]` / `classes[]` 表)或上游模組。
- `KB_Resolve` 解析失敗的訊息與解析警告改用 `KB_debuglog`(受 `KB_VERBOSE` 控制),預設不顯示、不彈框。

### 3. 模組路徑雙重前綴(最底層真兇 — 檔案全讀不到)

`kbconf.c` 在讀設定的 `[module] path` 時,若路徑是相對的(不以 `/` 開頭),會自動前綴 `datadir`。但 Windows / macOS 的 `openkb.ini` 同時寫了:

```ini
datadir = data
[module]
path = data/free/      ; ← 多了 data/
```

於是模組路徑被組成 `data/data/free/`(雙 `data`),free 模組的所有資料檔(`troops.ini`、地圖 `land.org`、`colors.ini` …)在 Windows / macOS 上**全部讀不到**。讀不到 → 解析回 NULL → 觸發第 2 層的崩潰、第 1 層之外又一個 `Unable to load map!` 對話框。

AppImage 的 `AppRun` 產生的設定用**絕對路徑**(`path = <abs>/free/`),`kbconf.c` 對絕對路徑會跳過前綴 → 不受影響。這就是為什麼只有 Windows / macOS 中招、Linux 沒事,也代表 **macOS 與 Windows 是同源問題**(與 CPU 架構無關)。

**修正**:`.github/workflows/build.yml` 產生的 Windows / macOS `openkb.ini`,模組路徑由 `data/free/` 改為 `free/`(相對 `datadir`,引擎會自行前綴)。

## 為什麼很難發現

- **dev 機(Linux)完全正常**:native build 與 AppImage 走的路徑分支都不觸發以上任一層。
- **「能跑的變體」遮住 bug**:AppImage 用絕對路徑,讓人以為設定沒問題。
- **同一段 log 嚴重度因平台而異**:`Unable to resolve DAT_*` 在 Linux 是無害噪音,一度被當成紅鯡魚;在 Windows 它是阻斷對話框。

## 怎麼查出來的

1. 在 **Wine** 下跑 Windows 包重現崩潰(`WINEPREFIX` 放家目錄、獨立 Xvfb display、`timeout -s KILL` 有界)。
2. `addr2line -e openkb.exe 0x14002A051`(mingw build 帶 `-g` DWARF)→ 直接定位到 `free-data.c:46`(`GNU_downto_byte` 對 NULL 解參考)。
3. 開 `KB_VERBOSE=1` 跑,log 印出字面路徑 `? FREE INI FILE: data\data/free/\troops.ini` → `FAILED TO OPEN` → 看到雙 `data`,鎖定第 3 層真兇。
4. 修正後重驗:`FAILED TO OPEN` 歸零、4 模組載入、無對話框、無 page fault。

> Wine 下 `xdotool` 合成的鍵常送不進 SDL app(事件轉譯),所以判定改看「引擎層證據」(log 進度、開檔失敗數、模組數、page fault 數),不硬驅動 UI。

## 教訓

- 跨平台打包 bug,**先在目標平台(哪怕用 Wine / CI runner)自己重現**,再談向使用者要 backtrace。「dev 機重現不出」≠「不可重現」。
- 驗**實際打包產物**在它自己的執行環境,不是只驗 `build/` binary。
- 缺資料一路 **NULL-safe** 回退到內建預設,不可解參考;防護打在最底層共用函式。
- 診斷用 verbose flag 把噪音改 debug 級:**出貨靜默、需要時開**,同一旗標兩面。
- 相對路徑 + 自動前綴 base dir = 雙重前綴經典陷阱;一個變體能跑、其他不行時,先比對路徑構造差異。
