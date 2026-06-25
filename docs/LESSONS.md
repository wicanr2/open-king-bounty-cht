# 工程經驗整理 (Lessons Learned)

把御封戰將繁中化 + 跨平台移植過程中,對「下一個做類似老遊戲漢化/移植的人」有用的教訓集中於此。細節分散在各專門文件,本檔是索引 + 跨檔的共通教訓。

## 1. 反向工程 / 多版本資產破解

把不同平台版本(DOS / Mega Drive / Amiga / FM-Towns)的原版資料當「美術來源」抽出來,引擎執行時依 F8 切換主題。

- **同血緣常用同壓縮容器**,但參數要逐版實測,不能照抄。
- **Amiga**:容器(count + descriptors + 32 色盤 + LZSS)+ 連續 planar(5 bitplanes,plane0=LSB);12-bit 0RGB 色盤用 nibble<<4 展開(**不是 ×17**);色盤起點在 descriptors 後一個 word(off-by-one 會讓整盤偏移)。細節見 [`reverse-engineering/amiga-assets.md`](reverse-engineering/amiga-assets.md)。
- **Mega Drive**:Okumura LZSS;cell template 與「正規編號」差 **+7**(前 7 個忽略才與 DOS/Amiga 對齊);**GPGX 模擬器的 CRAM 是 packed 9-bit BBBGGGRRR(native-LE),不是 Genesis 匯流排 BE word** —— 色盤解錯多半是這個。
- 教訓:**靜態臆測屢錯時,改全程式模擬解碼**;設上限、逆不出就誠實暫停,別無限挖。

## 2. CJK 在固定低解析引擎

老遊戲 320×200 之類固定解析做中文時,**不要把中文字縮小去塞原本小字位**(糊掉)。正解:拉高引擎內部畫布、底圖用 nearest-neighbor 放大保持銳利,中文用正常點陣尺寸(24×24)畫在放大後的畫布。引擎選型與 CJK 雙層渲染決策見 [`adr/0001-engine-and-cjk-approach.md`](adr/0001-engine-and-cjk-approach.md)。

## 3. 跨平台打包與驗證(最貴的一課)

**dev 機(Linux)全綠 ≠ 目標平台能跑。** 完整事故(Windows/macOS 開不起來 + 選角閃退,Linux 正常)見 [`POSTMORTEM-cross-platform-startup.md`](POSTMORTEM-cross-platform-startup.md)。萃取出的通則:

- **跨平台 bug 先在目標平台自己重現**(Windows → Wine;無 Mac → CI macos runner),別第一時間向使用者要 backtrace。「dev 機重現不出」≠「不可重現」。
- **驗實際打包產物**(解開 AppImage/zip/.app 在它自己的 cwd 跑),不是只驗 `build/` binary。
- **同段 log 在不同 OS 是不同嚴重度**:`KB_errlog` 在 Linux 印 stderr(噪音),在 Windows 彈 `MessageBox`(阻斷)。別用單一平台判斷某訊息「無害」。
- **「能跑的那個變體」會遮 bug**:一個打包正常(絕對路徑)、其他不正常(相對路徑)時,先比對路徑/設定構造差異,別假設同份 config 到處能用。
- **相對路徑 + 引擎自動前綴 base = 雙重前綴陷阱**(`datadir=data` + `path=data/free/` → `data/data/free/`)。
- **唯讀 cwd 存檔**:AppImage/.app/Android cwd 唯讀 → 存使用者可寫目錄;建目錄用 `mkdir -p`(遞迴),別假設父層存在。
- **編譯器嚴格度分歧**:clang/mingw 把 implicit-declaration、incompatible-pointer 當錯誤(C23),gcc 只警告 → 本機先用 `-Werror=implicit-function-declaration -Werror=incompatible-pointer-types -k` 一次抓出。
- **缺資料一路 NULL-safe 回退**,葉節點函式 NULL-check(打最底層共用函式,一處保護所有呼叫端)。
- 工具:**verbose flag 出貨靜默、診斷開**(印字面路徑才看得到雙 data);**`-g` + `addr2line`** 把 Wine 的 page-fault VMA 直接對到 `file:line`。

## 4. Android:鍵盤遊戲 → 觸控

不重寫輸入,而是**讀引擎每個畫面的 keymap 動態渲染觸控控制**,把手指事件合成 `SDL_KEYDOWN(SDLK_*)` 餵回原事件迴圈(引擎邏輯零改)。界面設計與分期見 [`android/ui-design.md`](android/ui-design.md)。

## 5. 共通 meta 教訓

- **CI 綠 ≠ 可玩**:headless smoke 測不到「預設視角錯/唯讀 cwd 不存檔/視窗縮放偏移/平台分歧」。要走玩家真的會走的路徑、在真平台看真畫面。實機驗證報告見 [`test/GAME-TEST-REPORT.md`](test/GAME-TEST-REPORT.md)。
- **引擎與版權資料分離**:原版 binary/ROM/美術/音樂一律不入公開 repo;公開包只給引擎 + 自由授權資產,玩家自備正版副本。
- **全程 Docker build**,不污染系統。
