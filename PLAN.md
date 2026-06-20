# 御封戰將 (King's Bounty, 1991) 繁體中文化 — 開發計畫

> 引擎:openkb (SourceForge 開源重製版,C + SDL)。目標:繁中化 + 跨平台打包 (Windows / AppImage / macOS / Android)。
> 本檔為工程計畫,定案後逐步落地;重大設計決策另記 `docs/adr/`。

## 1. 現況盤點 (已完成的資料收集)

### 1.1 引擎技術底層

| 項目 | 現況 | 對中文化的意義 |
|---|---|---|
| 語言 / 框架 | C + **SDL 1.2** (`SDL_SetVideoMode`、`SDL_Flip`),選用 SDL_image | SDL 1.2 是關鍵限制,見下方決策 1 |
| Build | GNU Autotools (`autogen.sh` → `configure` → `make`) | 需建 Docker build 環境 |
| 內部畫布 | 320×200,可選 2× filter 放大到 640×400 顯示 (`zoom`) | 邏輯座標仍是 320×200,8px 字格 |
| 文字渲染 | `KB_print` @ `src/env-sdl.c:231-254`。**純 ASCII**:`id=str[i]`,`row=id/16`,`col=id%16`,從 8×8 字格 atlas blit。無任何多位元組處理 | 要支援中文,渲染層必須改 (核心改動點) |
| 字型來源 | ① 內嵌 `src/font.h` (XBM 8×8) ② DOS `KB.CH` (128 glyph 8×8) ③ free `openkb8x8.bmp` | 都是 8×8 點陣,中文需另開 TTF 路徑 |
| 平台打包 | `dist/win`、`dist/osx`、`dist/freedesktop` 已有腳本;**無 Android** | win/mac/appimage 有基礎;Android 要從零 |

### 1.2 資料模組 (data module) — 選 `free`

引擎支援多模組:`free` (純自由資產)、`dos` (讀原版 KB.EXE + 256.CC/416.CC)、`fmtowns`。

**選用 `free` 模組做中文化**,理由:
- 玩家不需自備原版遊戲,所有文字在人類可讀的 `.ini` / `.txt`。
- `free-data.c` 的 `GNU_string_ini()` 從 `.ini` 讀名稱 (troops/towns/spells…),**改檔即生效,不需反編 binary**。
- `bounty.c` 裡的寫死英文陣列 (`troops[]`、`town_names[]`…) 是 **DOS 模組的 fallback**,free 模組會用 `.ini` 覆蓋。

### 1.3 可翻譯文字盤點 (~550–700 條)

| 來源 | 位置 | 量 | 改法 |
|---|---|---|---|
| **資料檔 (70%)** | `data/free/` 12 個 `.ini` + 21 個 `.txt` | ~400 行 | 直接編輯,只翻 `name =` / `description =`,保留 `file=`/`abilities=` 等鍵 |
| — 單位/兵種 | `troops.ini` | 25 | |
| — 敵方 boss | `villains.ini` | 17 | |
| — 法術 | `spells.ini` | 14 | |
| — 工藝品 + 描述 | `artifacts.ini` | 8 + 8 | |
| — 城鎮 / 城堡 / 大陸 | `towns.ini`/`castles.ini`/`land.ini` | 26/26/4 | |
| — 地標 / 結局 / 製作 / 17 個 villain 故事 | `signs.txt`、`endwin/endlose.txt`、`credits.txt`、`*.txt` | ~350 行 | |
| **原始碼 (30%)** | `src/game.c`、`src/ui.c`、`src/bounty.c` | ~150–200 | 寫死英文,需改原始碼後重編 |
| — UI 狀態標籤 | `game.c:1733-1758` ("Leadership"、"Gold"…) | 12 | |
| — 遊戲訊息 | `game.c` (~54 處 `KB_MessageBox`/`KB_BottomBox`) | ~50–100 | |
| — 按鍵提示 | `ui.c` ("ESC - quit game"…) | 數個 | |

### 1.4 中文化素材 (`original_kb/`) — 含官方譯名權威

| 檔案 | 用途 |
|---|---|
| **`DDSC-J-00055-01遊戲手冊：御封戰將.pdf`** | **1990 年代官方繁中手冊**。譯名 (御封戰將、兵種、城鎮、法術) 的權威「考古」來源 |
| `DDSC-J-00055-02-遊戲附件：御封戰將密碼.pdf` | 官方繁中密碼/防拷附件 |
| `Kings-Bounty_Manual_DOS_EN_Web-Manual.pdf` | 英文手冊 (對照原文語意) |
| `Kings-Bounty_DOS_EN.zip` | 原版 DOS (256.CC/416.CC/KB.EXE),供對照與 dos 模組測試 |

> 有官方繁中譯本 → 術語不自創,以手冊為準 (符合 retro CHT「譯名考古」原則)。

---

## 2. 已定案決策

| # | 決策 | 結論 |
|---|---|---|
| 1 | SDL 版本 | **升 SDL 2**。一次解鎖 4 平台 (含 Android);SDL 呼叫集中在 `env-sdl.c` 單檔,adapter 隔離,改動可控 |
| 2 | CJK 畫法 | **照搬今天完成的 1oom (MOO1) 中文化模式** (見 §2.1):邏輯畫布維持 320×200、合成層 2× 放大、24×24 漢字疊圖 |
| 3 | 字型來源 | **隨包綁定 Noto Sans CJK TC** (OFL),烤成點陣 atlas。「系統中文字型」採向量字型烘烤,不依賴使用者機器已裝字型 |
| 4 | push GitHub | **先不 push**,本地討論定案後再一起 push |

### 2.1 CJK 渲染架構 (學自 1oom `/home/anr2/master-of-orion/1oom`,今天剛完成)

**核心洞察:邏輯座標不動,只在合成層放大。** 這同時滿足 rule 81 (高解析畫布) 又避免「重映射所有 UI 座標」的大工程 —— 因為 1oom 證明了邏輯層維持 320×200、視覺層才放大的作法可行。

```
8-bit 320×200 buffer ──► 32-bit ARGB 320×200 texture ──► [有 CJK?]
                                                           ├─是─► 640×400 composite target:
                                                           │       ① 底圖 nearest 放大 320×200→640×400
                                                           │       ② 疊 24×24 漢字 glyph (8 方位黑外框)
                                                           │       └─► RenderCopy 到視窗
                                                           └─否─► 標準 upscale 到視窗
```

具體要點 (對應 openkb 落點):
- **邏輯座標全程 320×200**:UI widget、滑鼠 hit-test 都不動 → **不需 `mapX/mapY` 座標重映射** (省掉最大工程量)。
- **CJK 前進寬度 = glyph/2 (邏輯座標)**:24×24 漢字在邏輯座標佔 12px (約 1.5 個 8px ASCII 格);實際渲染在 640×400 合成層才是 24px。換行/字串寬度/置中改用此前進寬度逐字累加。
- **預烤 atlas**:`cjkfont.c` 載入 `data/fonts/cjk24.bin` (magic + glyphW/H + count + 升序 codepoint + alpha bitmap),二分查找;runtime 把 glyph 上傳成 SDL_Texture 快取 (hash + linear probing)。
- **UTF-8 分流**:`KB_print` 解 3-byte UTF-8 (0xE0–0xEF 前導);`cjkfont_has(cp)` 命中走 CJK draw-list,否則走原 ASCII 8×8 路徑。openkb 控制碼與 UTF-8 前導不衝突。
- **draw-list 雙緩衝**:`KB_flip` 時 `cjk_drawlist_flip()`,避免 present 期間閃爍。
- **黑外框 + 自動提亮**:漢字 8 方位黑框 + 亮度 <110 自動提亮,確保彩色背景上可讀。

可直接複製的 1oom 檔案:`src/cjkfont.h`、`src/cjkfont.c` (幾乎無改)、`scripts/build-font.sh`、`scripts/build-appimage.sh` (改路徑/應用名);`src/lbxfont.c` 的 UTF-8 分流邏輯移植進 `KB_print`。

**待確認的小決策**:畫布合成尺寸 **640×400 (乾淨 2×,1oom 採用,無非整數縮放瑕疵)** vs **640×480 (4:3 方正但 200→240 需非整數映射)**。你先前提到 640×480;但 1oom 用 640×400 避開「非整數縮放像素不均」的雷。建議跟 1oom 一致用 **640×400**,除非你要 4:3 觀感。

---

## 3. 階段規劃

| 階段 | 內容 | 產出 | 風險 |
|---|---|---|---|
| **P0 環境 ✅** | Docker build 環境 (SDL1.2 + autotools);free 模組跑出英文原版 | ✅ `docker/` 腳本 + `docs/baseline/` 截圖 | 已解 (見 §4.7) |
| **P1 SDL2 ✅** | `env-sdl.c` 移植 SDL1.2→SDL2 + compat shim;邏輯 320×200 + renderer 縮放 | ✅ 編譯通過、渲染像素正確 (`docs/baseline/sdl2/`) | 已解 (見 §4.8) |
| **P2 CJK 渲染 ✅** | `cjkfont.h/c` atlas + draw-list;**`inprint()`** 加 UTF-8 解碼 (主文字路徑);`env-sdl` 合成層 640×400 疊 24×24 漢字 + 黑外框;`build-font.sh` 烤 Noto CJK TC | ✅ 中文已在遊戲內渲染 (`docs/baseline/cjk-credits.png`) | 見 §4.9 (cache 座標待 P5 全面驗證) |
| **P3 資料翻譯** | 翻 `data/free/*.ini` + `*.txt`;譯名對照官方手冊;troop 引用一致性處理 | 全資料中文化 | troop 名引用需一致 |
| **P4 原始碼字串** | 翻 `game.c`/`ui.c`/`bounty.c` 寫死英文;UI 標籤對齊新字寬 | 全 UI 中文化 | 標籤對齊欄寬 |
| **P5 驗證** | headless 確定性回歸 + 正常玩家路徑可玩性 (rule:debug hook 會遮真 bug) | 可破關驗證 | soft-lock |
| **P6 打包** | Win (mingw) / AppImage / macOS (CI) / Android (SDL2 NDK + 觸控) | 4 平台安裝檔 | Android 觸控 UI |
| **P7 文件** | 玩家向 README (雜誌風) + 工程文件 + 繁中攻略 | repo 文件 | |

---

## 4. 已知技術細節 / 待處理坑

1. **troop 名引用一致性**:`GNU_parse_troop` 用 `name` 字串比對軍隊組成 (如 "5 x Peasants")。若把 name 翻成中文,所有引用處要一致;**較佳解**:改引擎以穩定 id (`file=` 欄) 比對,而非顯示名。
2. **只翻 `name=`/`description=`**:`.ini` 內 `file=`/`abilities=`/`group=` 是程式鍵,不可翻。
3. **debug hook 遮 bug** (rule):回歸測試另驗「無 debug 正常玩家路徑」,用 flood-fill 檢查落點/城鎮連通性。
4. **版權分離**:`original_kb/` 的原版資料、官方手冊 PDF 一律 gitignore,不入公開 repo;公開包只給引擎 + free 自由資產 + 開源字型。
5. **全程 Docker build**,Python 用 docker uv.venv。
6. **1oom 踩雷** (見原 §4.6,移此後仍適用):邏輯/視覺座標分離、滑鼠 hit-test 反演、乾淨 2× 避免像素不均、glyph cache 容量、paletted→ARGB 用 LockTexture。

### 4.7 P0 實作紀錄 (已驗證)

- **vendor 相依** upstream `drop.sh` 的 URL 多已失效:`git://` GitHub 已停用 (改 https);scale2x sourceforge 死 (改 amadvance GitHub 鏡像);sha2 來源死但**無任何 .c 引用,可略過**。已收斂進 `docker/fetch-vendor.sh`。
- **free 模組不能靠 autodiscover** (kbauto.c 對 free DIR 模組的標記偵測未實作)。必須用**顯式 config**:`[module] / type=free / path=<abs>/data/free/`。
- **模組選單一律互動** (`select_module` 連單模組也要按 Enter,game.c:805)。headless 截圖用 xdotool 送 Return 推進。
- 內部視窗 `filter=normal2x` → 640×400。已截到片頭 (Royal Reward)、製作名單、角色選擇,美術 + 8×8 字型渲染正常。
- 一鍵重現:`docker build -t openkb-build -f docker/Dockerfile .` → `docker run ... openkb-build sh docker/build.sh`。

### 4.8 P1 實作紀錄 (已驗證)

- **SDL2 環境**:`docker/Dockerfile.sdl2` (libsdl2-dev / SDL2_image / SDL2_net)。`configure.ac` 改 sdl2-config + SDL2 系列;`Makefile.in` 改 -lSDL2_net。
- **compat shim** `src/sdlcompat.h` 收斂散落的 SDL1.2 名稱:`SDL_keysym`→`SDL_Keysym`、`SDL_SetColors`→`SDL_SetPaletteColors`、`SDL_SetPalette`/`SDL_LOGPAL`、`SDLK_LSUPER`→`SDLK_LGUI`、`SDL_SWSURFACE`/`SDL_SRCCOLORKEY` 等旗標。各檔 `#include <SDL.h>` 改指 shim。
- **env-sdl.c 視訊核心重寫**:`SDL_SetVideoMode`→`CreateWindow`+`CreateRenderer`+串流 `Texture`;`SDL_Flip`→`UpdateTexture`+`RenderCopy`+`RenderPresent`;`WM_SetCaption/SetIcon`→`SetWindowTitle/Icon` (+ `KB_setcaption()`)。
- **邏輯畫布固定 320×200,zoom=1**:`conf->filter` 轉成 render scale quality 後歸零,讓資源層的軟體預放大全失效,改由 renderer `RenderSetLogicalSize` 等比縮放 (對齊 1oom 模型,P2 合成層的前置)。
- **kbd_state 改 scancode 索引** (`ui.c`):避免 SDL2 特殊鍵 keysym `0x40000000|scancode` 巨值溢位 `kbd_state[512]` —— 本 codebase SDL2 化的頭號雷。
- **kbfile.c 自訂 SDL_RWops** 簽名改 SDL2 (`Sint64`/`size_t`);**combat.c** 兩處 `SDL_Flip(screen)`→`KB_flip(sys)`。
- 驗證:headless 跑出製作名單畫面,色彩/文字/縮放正確,與 SDL1.2 baseline 一致。

### 4.9 P2 實作紀錄 (已驗證)

- **真正的文字路徑是 `inprint()`** (原 vendor/SDL_inprint),非 `KB_print`。openkb UI 文字 188+ 處走 `KB_iprintf`→`inprint()`。已把 inprint 收為 `src/inprint.c` (openkb 自有) 並加 CJK:遇 atlas 內 3-byte UTF-8 → 記進 draw-list、佔 2 格寬;`incolor()` 擷取前景色供上色。
- **合成層** (`env-sdl.c`):`KB_flip` 先 `cjk_drawlist_flip` (pending→front,讓「畫一次就等待」的選單當次就顯示),有 CJK 時渲染到 640×400 composite (底圖 2× nearest + 24×24 glyph + 8 方位黑外框 + 亮度保底),再縮放呈現。
- **字型**:`tools/build_cjk_font.py` + `docker/build-font.sh` 用 Noto Sans CJK TC 烤 `data/cjk24.bin` (magic OKBCJK,24×24,碼點取自 data/free 譯文 + 譯名表)。glyph texture hash cache (2048)。
- **驗證**:`御封戰將 設計者：` 在製作名單畫面正確渲染 (白字黑框,與英文並存)。
- **待 P5 驗證**:文字若繪到 offscreen cache 而非 `sys->screen`,draw-list 的邏輯座標可能偏移;製作名單 (KB_MessageBox) 走 screen 正常,其他畫面待 game-test 全面確認。
6. **1oom 踩雷 (照搬模式時一併避開)**:① 邏輯座標 vs 視覺座標別混 (前進寬度用 glyph/2,別用 24px);② 滑鼠 hit-test 座標要從合成層反演回邏輯座標;③ 非整數縮放會出現像素不均 → 優先乾淨 2× (640×400);④ glyph cache 滿會 silent drop,需監控/擴容;⑤ paletted→ARGB 轉色用 SDL2 `LockTexture` 而非手寫迴圈。

---

## 5. 下一步 (待你拍板)

1. 確認 **決策 1–4** (SDL2 升級 / CJK 畫法 / 字型綁定 / 是否 push)。
2. 我接著:建 Docker build 環境 (P0),跑出英文 baseline 截圖,確認可重現後再進 P1。
3. 補充素材需求:目前素材已足夠啟動;官方繁中手冊待我做 OCR/抽文以建立譯名對照表。
