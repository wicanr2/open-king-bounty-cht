# 御封戰將中文化 — WORKLIST (持久待辦,防 context 遺失)

> 詳細計畫:`PLAN.md` (P0–P7 工程階段)、`docs/ASSETS-PLAN.md` (F8/F9 多版本)。
> 本檔是「做了什麼 / 還要做什麼」的單一真相清單。每完成一項就打勾並 commit。

## ✅ 已完成 (已 push 到 GitHub)

- P0 環境:Docker build (SDL2) + free 模組英文 baseline。
- P1:SDL 1.2 → SDL 2 移植 (env-sdl.c + sdlcompat.h;kbd_state scancode;RWops 簽名)。
- P2:CJK 渲染 (cjkfont atlas + inprint/KB_print UTF-8 + Noto Sans CJK TC)。
- P3:data/free 全文翻譯 (.ini/.txt,troop 引用一致;官方手冊譯名)。
- P4:src 寫死 UI 字串翻譯 (game.c/bounty.c/combat.c/ui.c)。
- P5:game-tester 報告 (docs/test/GAME-TEST-REPORT.md)。
- P6:AppImage 打包 (free 公開版 + -original 個人版)。
- P7:README (雜誌風) + CONTRIBUTORS。
- 實機 bug 修復:
  - 單一模組自動選取 (打包版啟動崩潰)。
  - SDL2 PAL surface 崩潰 (GR_PURSE NULL → 進遊戲即崩)。
  - CJK 文字消失 (draw-list 逐 cell 鏡像螢幕)。
  - 視窗放大內容不縮放 (KB_present + WINDOWEVENT 重呈現)。
  - 音訊噪音 (SDL_OpenAudio 強制 S16) + 城堡持續嘟嘟 (idle 填靜音)。
  - 像素過柔 → 銳利 (移除合成層、nearest 直繪)。
  - 英文改 Noto 與中文一致。
  - 混合模式:自動偵測原版 DOS 資料 → 原版美術 + 中文 (型別感知解析)。
  - ESC→F10 退出 + 自動存檔 + Y/N (cheat 改 F12);ESC 改 cancel。
  - 缺字「嗎」(重烤 atlas,1226 字)。

## 🔧 第十一輪 (2026-06-22):使用者回報 GitHub issue (#1 #2) — 啟動閃退

使用者(PowerSaka / shiun-git)在 macOS 上回報公開 free 版開不起來。

### ✅ 已修 + 驗證
- [x] **savedir 首次啟動閃退**(issue #2 完整、issue #1「需手動建 `~/.openkb`」):`save_dir` 預設 `$HOME/.openkb/saves`(兩層),`test_directory` 只單層 `mkdir` → 父層 `.openkb` 不存在即 ENOENT 失敗 → `Can't start without a proper savedir`。改 `src/lib/kbstd.c` `test_directory` 為遞迴建立(mkdir -p,中間層錯誤忽略 + 最後 stat 驗證),跨平台 + Android 通用。**已驗證**:全新無 `.openkb` 的 HOME 自動建 `~/.openkb/saves`、不再 fatal。commit `8bba5ec`(android-port)。

### 🔍 已釐清(紅鯡魚)
- `Unable to resolve resource DAT_RANGEMIN…` 等一堆 → **無害**。`bounty.c` 內建 canonical `troops[MAX_TROOPS]` 表,`refill_rules()` 對 DAT_* 是 null-guard 覆寫 → free 版缺這些只是保留內建預設值(move_rate 等不為 0)。ASan + xvfb 完整跑「新遊戲→騎士→命名→難度→進地圖→移動→開選單」**全程無崩潰**(Linux)。

### ✅ 已修(續)
- [x] **DAT_* 'Unable to resolve' 訊息**:改用既有 `KB_debuglog`(受 `KB_VERBOSE` 控制)→ **預設靜默**、`KB_VERBOSE=1` 才印。驗證:預設 0 條、verbose 44 條,皆正常進遊戲。commit `e37e34b`。
- [x] **併 master + 發 Release**:android-port → master 合併(`5d081d9`),tag `v0.0.3-cht.1` 觸發 CI(run 27957777230)重編五平台 free 版 + 建 GitHub Release。
- [ ] **(進行中)重打包 mac 完整版**:等 CI 新 mac 引擎(含修復)→ build-dist-all 注入完整資料 → dist-all。同步刷新 win/linux 完整版引擎。

### ✅ 真兇定位 + 修復 (Windows, 2026-06-22 晚)
使用者提供 Windows 截圖:選角後彈 `Critical Error: Unable to resolve resource DAT_RANGEMIN (from 4 modules)` 對話框。Wine 重現 + addr2line 定位根因鏈:
- Windows `KB_errlog` 會彈 `MessageBox("Critical Error")`(kbstd.c:108)→ 每個未解析資源都彈框。**已於 DAT 修復(e37e34b)消除**(改 debuglog)。
- 對話框消失後浮現**真‧NULL deref 崩潰**:`free-data.c:46 GNU_downto_byte` 對 NULL src deref。系統性根因 `KB_strlist_ind(NULL)` 在 `*list` deref NULL → ini 缺失時任何 `KB_strlist_peek` 都炸。
- 修正(commit `e8c0ee6`,tag `v0.0.3-cht.2`,全部 NULL-safe 退回 bounty.c/DOS):`KB_strlist_ind` NULL guard(最高槓桿)+ `GNU_downto_byte/word` + `GNU_spell/artifact_downto_byte` + `GNU_parse_troop` + army 警告改 debuglog。
- **這就是 issue #1「選完難度閃退」的真兇**(Windows;Mac 可能同源,待回報者驗)。
- 待 CI(run 見 /tmp/rid.txt)重編 → Wine 驗證崩潰消失 → 重打包 dist-all + 更新 Release/issue。
### ✅ 最底層真兇 + 全修復完成 (tag v0.0.3-cht.3)
KB_VERBOSE 揪出:`? FREE INI FILE: data\data/free/\troops.ini` → `FAILED TO OPEN`。**路徑雙 data!** 根因:`kbconf.c:231` 對相對模組 path 會前綴 `data_dir`,但 config 同時寫 `datadir=data` + `path=data/free/` → `data/data/free/` → Windows/macOS free 全檔(troops.ini/land.org…)開不到。AppImage 用絕對路徑(`buf2[0]=='/'`)跳過前綴故無症 → 這就是只有 Mac/Win 中招、Linux 沒事的原因。**Mac 與 Windows 同源**(非 arm64 問題)。
- 修正(`81fce7a`):build.yml 的 windows + macos `openkb.ini` 模組 path `data/free/`→`free/`(相對 datadir)。
- **Wine + KB_VERBOSE 三輪驗證**:修正後 `data\free/\troops.ini` 開檔成功、`FAILED TO OPEN=0`、4 模組載入、無 Critical Error 對話框、無 page fault。
- 三層修復全到位:① 路徑(cht.3)② NULL 防護(cht.2)③ 對話框/訊息靜默(cht.1)。
- **出貨**:Release `v0.0.3-cht.3`(free 版 mac/win/linux,五平台 CI 全綠)+ `dist-all/` 完整版三平台(含修復引擎 + DOS/Genesis/Amiga 美術 + FM-Towns 音樂,openkb.ini=path=free/ 驗證過)。
- GitHub issue #1 #2 已回報真根因 + 新版,請回報者重測。

### 🔧 回報者再回饋 (cht.4, 2026-06-25)
- [x] **issue #2 `couldn't open icon file`**:icon 沒打包 + `env-sdl.c:275` 用 `KB_errlog`(Windows 又彈框)。修:icon 載入失敗改 `KB_debuglog`(非致命)+ build.yml win/mac 複製 `data/icon_32x32.png` 進 rootdir。tag `v0.0.3-cht.4`,已驗證 icon 打包。
- [ ] **issue #1 macOS `failed loading sdl3 library`**:拆 cht.3 `.app` 驗過 → bundle 是 SDL2、無 SDL3、`@executable_path` 自包含 → 訊息非我們 binary 印。疑 Gatekeeper 拒載未簽 dylib / arm64-Intel 架構 / 下載問題。**已要對方提供晶片型號 + 完整錯誤截圖**。給了 `xattr -dr com.apple.quarantine` 解法。
- [ ] **macOS ad-hoc codesign 沒生效**:cht.4 加的 `codesign --force --deep -s -` 產物無 `_CodeSignature`(`|| true` 吞錯)。若確認是 Gatekeeper,要改**由下而上逐一簽**:先 `find Contents/libs -name '*.dylib' -exec codesign --force -s - {} \;` 再簽 MacOS/openkb 再簽 .app(去掉 deprecated `--deep`)。待 PowerSaka 回報晶片/錯誤再決定(可能其實是 Intel→需 universal build)。

### ✅ macOS "Failed loading SDL3 library" (issue #3, tag v0.0.3-cht.6)
回報者比對 `.app/Contents/libs` 證實是 libs 問題。根因:**Homebrew 把 `sdl2` 換成 sdl2-compat**(SDL2 API 架 SDL3,~0.5MB vs 真 SDL2 ~2MB),runtime dlopen libSDL3,dylibbundler 沒打包 SDL3 → 載入失敗。cht.4 是真 SDL2、cht.5 brew 更新後變 compat。
修(`docker/build-sdl2-from-source.sh` + build.yml macos):**源碼編 pinned 真 SDL2 2.30.9 + image 2.8.2(stb_image)+ mixer 2.8.0(stb_vorbis OGG)+ net 2.2.0,不連任何外部 codec**(libpng/vorbis/fluidsynth 全免)。Linux 先驗配方;cht.6 mac 產物驗證:libs 只剩 4 個、libSDL2=2MB 真 SDL2、SDL3/sdl2-compat/fluidsynth 字串=0。dist-all mac 完整版也重打包成真 SDL2(30MB)。
- [ ] **回報者驗 cht.6 mac**(#3 PowerSaka)。
- [ ] **issue #2 黑畫面**(shiun-git,完整版有音樂但黑畫面)→ 疑同 sdl2-compat 繪圖問題,已重打包真 SDL2 完整版待其驗;**需 wicanr2 重新分享 FB 連結(dist-all/KingsBounty-CHT-full-macOS.zip)**。仍黑畫面要晶片+log。
- [x] **Windows「not enough castles」已修(KB_fgets,tag v0.0.3-cht.7)**:`KB_fgets` 改逐字元讀(不再 fread+fseek 退讀)。**Wine 隔離測試實證**:舊版 filled=0(重現)、新版 filled=26(修好)。引擎重建 Linux 無回歸。dist-all 三平台 + free Release 皆已含。待 shiun-git 驗 Windows。

### ⏳ 待辦
- [ ] (已關閉) issue #1 選完難度閃退 (Mac):Linux 重現不出 → 疑 **macOS 特有**。需向回報者要 macOS crash report(Console.app)/ terminal 末尾 backtrace,或請其測新 build。**不可無 backtrace 盲修**。
- [ ] **出貨**:savedir 修復需進公開 free 版 → 重編 macOS/Windows 包(CI)+ 視情況併 `android-port`→`master` + 發新 Release。需使用者確認(對外/release)。
- [ ] 回覆 GitHub issue #1 #2(對外訊息,需使用者確認)。
- [ ] (選用)在 `free-data.c` 補實作 troop DAT_*(skill/moves/melee/ranged/ammo/gold/group/abilities/dwelling)以消除誤導性錯誤訊息 — 確認非崩潰主因,可延後。

## 🔧 第十輪 (2026-06-22):多平台 CI 打包 + Android 移植 (branch `android-port`)

### ✅ 已完成
- [x] **完整版三平台包 `dist-all/`**(`docker/build-dist-all.sh`,host 端組裝,gitignore):Linux AppImage / Windows x64 zip / macOS .app zip,**三者都含全套美術(free + DOS `256.CC`/`416.CC`/`KB.EXE` + Genesis `kb.bin` + Amiga `GAME`)+ FM-Towns 音樂(16 軌 ogg)**。做法:完整資料 payload 取自已驗證的完整版 AppImage 內解壓 `$SHARE`(單一真值),注入 CI 出的 win/mac 包(引擎與資料無關);win `play.bat` / mac `launch` 注入 `KB_AMIGA_GAME` + `KB_MUSIC` env。F8 切主題、F9 切音樂。**含版權素材,個人用勿散布**。AppImage headless smoke 確認 BGM=fmtowns + Amiga 偵測;F8 四主題 playtest 背景驗證中。
- [x] **GitHub Actions 多平台 CI**(`.github/workflows/build.yml`):font + linux(AppImage)/ windows(MSYS2+DLL)/ macos(.app)/ android(NDK APK)。**五個 job 全綠**。產物:AppImage 11MB / Windows zip 18MB / macOS .app 7MB / Android APK 5MB。建公開 free 版。
- [x] **桌面跨平台編譯修正**(clang/mingw C23 比 gcc 嚴):移除 libhfs/librsrc;`add_module_aux` 補宣告;`wavFile_read_FILE` 前向宣告;`KB_getpos` word 暫存;strlcat/strlcpy 原型移 kbstd.h;`KBRW_*` 宣告改 SDL2 簽名;windows `mkdir`→`_mkdir` + MSYS_TEST 認 MINGW64 + 不建 openkb.rc;build-appimage.sh apt 加 sudo。
- [x] **Android 骨架**:`android-project/`(setup.sh + overlay + src/android.c bootstrap);NDK build 通(SDL2 + WAV-only mixer + PNG image;seekdir/telldir rewinddir 後備;combat.c 不編)。
- [x] **Android phase 3 觸控**:`src/touch.c` D-pad + A(Enter)/B(ESC),FINGER→SDLK_* 餵回 KB_event。

### ⏳ 待辦(避免遺忘)
- [ ] **【週末】使用者用手機實測 APK**(GitHub Actions → 對應 run → Artifacts → `KingsBounty-CHT-android-apk`)→ 回報:能否啟動、看到標題、D-pad 走地圖、A/B 確認取消。據實機修。
- [ ] **Android phase 4 情境快捷列**:讀當前 keymap → 浮出字母按鈕(A–E)/ 直接點選單那行 → 城鎮/商店字母選單可用(目前只有方向鍵+Enter/ESC,字母選單不能操作)。
- [ ] Android phase 5:數字步進器(招募數量)、命名 IME(SDL_StartTextInput)、生命週期 pause/resume 存檔。
- [ ] Android 手感打磨:D-pad 按住連續移動(目前點一下一步)、控制透明度可調、左右手對調、SDL2_mixer 補 OGG(目前 WAV-only 無音樂)、☰ 系統選單(F8/F9/存退)。
- [ ] **合併決策**:android-port 待真機驗證 + phase 4 後再考慮併 main;桌面 CI 修正(libhfs 移除等)目前只在 android-port,可考慮先 cherry-pick / 併回 master。
- [ ] 觸發 CI 注意:`gh workflow run build.yml --ref android-port` 前要等 GitHub 更新 branch head(push 後 sleep ~18s + 確認 ls-remote),否則跑到舊 commit。
- 方法論 skill:`retro-keyboard-to-touch`(鍵盤老遊戲→觸控)。

## 🔧 第九輪 (2026-06-22):Amiga 配色根因 + 英雄透明 / Genesis tileset 仍破碎

- [x] **Amiga palette off-by-one(所有配色錯誤的唯一根因)**(commit 03ffd6f / 3cd74fd):
  - 真實 palette 從描述子後**第一個** word 開始(count=1 時 offset 10),引擎舊碼多跳一個 word → 每 pixel index 顯示成 `palette[i-1]` 的色(天空 teal 取代 cyan、草地中綠取代亮綠、sprite index0 變白而非透明)。`amiga_color`(`<<4`)與 plane 解碼本來就對。
  - 修正 `src/lib/amiga-data.c`(palette 讀取改 `2+count*6+2`,word 移到 palette 之後保持 stream 起點不變);`tools/amiga_decode.py` 同步;`docs/reverse-engineering/amiga-assets.md` 更正(舊「confetti 是視覺誤判」結論作廢)。
  - **驗證**:對齊 `kings-bounty_17.gif` 逐像素均差 **213.8 → 4.8**;引擎內 dump 世界地圖(亮綠草地/藍水/灰石城堡/黃沙/棕山)、location(城堡+雪山+綠谷)皆正確。
  - 定位法(可重用):色散最小對齊 → 反推每 index 真實色 → 一眼看出 off-by-one;比盲試 plane permutation 快。教訓:比 palette **色集**(index 重排下不變)測不到此 bug,要比**渲染後逐像素**。
- [x] **Amiga 地圖英雄黑框**(commit d9d9617):`GR_HERO = GR_CURSOR`(cursor=英雄 sprite sheet),`AMIGA_Resolve` 的 GR_CURSOR 漏設 `transparent=1` → index0(黑)未轉 colorkey。已加,引擎內驗證 sprite 疊綠底無黑框。
- [x] **Genesis tileset 錯位根因 = cell 編號偏移 +7(2026-06-22,使用者觀察 + Amiga 逐 cell 比對證實)**:
  - 真正根因:**Genesis cell template 比標準 (free/DOS/Amiga) tile 編號偏移 7**(前 7 個 template 是 Genesis 專屬水/動畫/結構格,不在標準 72-tile 模型)。引擎用「遊戲地圖資料(標準編號)+ Genesis 圖塊」渲染,未加偏移 → 整張地圖錯位 7 格(水變棕條紋等所有症狀)。
  - 修正 `src/lib/md-rom.c`:`MD_CELL_BASE 7`,canonical cell N → Genesis template N+7。逐 cell 比對 Amiga(已驗證正確)確認 Amiga[N]==Genesis[N+7] 全段(N=0..71,template 7..78)吻合;in-engine dump 與 Amiga grid 對齊。**移除先前的 tile-14 水 hack(治標),此為乾淨根因修正**;保留 vflip(bit12)支援。
  - 比較表(README `tileset-themes.png`)更新:free 改用真實 free 美術(直接讀 `data/free/tileseta.png`+`tilesetb.png`,先前 dump 因 free GR_TILESET fallback 到 DOS 而誤顯示成 DOS);四主題現各自正確獨立。
  - **(下方為過程紀錄,水/vflip 為中途發現,最終由 +7 偏移統一解釋)**
- [x] **(過程)Genesis tileset 水 cell + vflip(2026-06-22)**:
  - **第一性原理判定**:LZSS/tile/template/palette 解碼對(~9 成 cell 正確);唯一破口 = cell-type 0(最常見,河流/內陸水)= tile 14×30 全相同。反證:tile 14 在 line2 的 index3/4 是棕,而該棕被另 47 個地形 cell 正確當泥土/陰影 → 若 cell0 是普通 metatile 真機也會棕,但實機水是藍 → **cell0/tile14 是「水」特例,真機靠 palette cycling 動畫顯示藍**(MD 省 ROM:1 tile 重複 + palette 動畫)。
  - **修正**`src/lib/md-rom.c`:`MD_WATER_TILE 14` 強制 palette line 1(index3/4=藍,≈實機河流藍 `(4,38,132)/(36,110,196)`),不動其他 47 cell;index0 露底已是海藍 → 水 cell 整體呈藍。python+in-engine 雙驗證:cell0 (72,0,0)/(144,72,0) 棕 → (0,36,144)/(36,108,216) 藍;地圖 render 河流連貫(`qa-md/map_WATERFIX.png`)。
  - **補 vflip(bit 12)**:引擎原本忽略 nametable vflip(MD 用 h/v flip 重用 tile 拼圖案省空間);cell 16/27/29/31 有用,已在 `md_blit_subtile` 實作。
  - 河流藍 ≠ 海洋(cell 27-39)teal,與實機一致。
- [x] **Genesis troop/villain sprite 黑剪影修正(2026-06-22,模擬器 CRAM ground truth)**:`MD_PAL_OFFSET 0x25698` 整條全黑 → 黑剪影。ROM 內 sprite palette 為壓縮(raw 搜尋找不到)。改用 Genesis 模擬器(GPGX)CRAM dump(`qa-amiga/emu/cram_*.bin`)取得 ground-truth palette **硬編**:troop 用世界地圖 CRAM line0(灰/棕/黃/紅 單位通用色),villain 用角色畫面 CRAM line0(紫底+膚色)。troop/villain 各用一條共用 line(MD 戰鬥同屏多單位本就共用 sprite palette,非每 troop 一條)。in-engine 驗證:troop(灰狼/白骷髏/橙惡魔/紅龍/灰甲騎士)、villain(紫底臉)皆正確。**關鍵:GPGX CRAM 格式 = packed 9-bit `BBBGGGRRR` native-LE(非 raw Genesis BE word),先前誤用 BE 解出全藍**。
  - 殘留(次要):個別需要特殊色的單位(如某些綠色生物)用共用 palette 會略偏;若要逐單位精準需戰鬥畫面 sprite attribute 分析。
- [ ] **free 怪物美術不完整(非 bug,事實)**:`data/free/` 只有 `peas.png`/`knig.png` 兩個 troop sprite,其餘 troops.ini `file=` 指向的檔案缺 → fall back 到 DOS。故 free 主題的怪物多數顯示為 DOS 美術。tileset 則 free 有完整 `tileseta/b.png`。
- [~] **Genesis tileset(舊紀錄,已被上方取代)— 用實機圖反推定位到「水」cell**(2026-06-22,實機 06-54-21.png + `genesis_pic/`):
  - **72 cell 中約 9 成正確**:草地、森林、海洋(cell 27-39 teal)、沙漠、山、城堡(cell 8-14 灰)、物件(井/樹/招牌/寶箱)皆對。LZSS(ring 0x20、p=8、out_size=header[4..7] BE、pattern skip 2)、638 tiles、palette 0x3371A 4 line、nametable word(idx&0x7FF / hflip bit11 / line bit13-14)、vflip(bit12,引擎目前忽略)都驗證可用。
  - **破口收斂到 cell-type 0(map 最常見,第一洲 647、全圖 1531)= 內陸水域**:map 結構圖顯示 cell 0 形成蜿蜒水道+湖,與海洋連通(= 河/湖)。cell 0 的 template = **tile 14 重複 30 次**;**tile 14 只被 cell 0-6 使用 = 它就是「水 tile」**(bytes `43 00 00 00` = 細直線 + index0)。
  - **為何破**:tile 14 用 nibble 4,3,template 標 palette **line 2** → line2 col3/4 = 棕(144,72,0)/暗紅(72,0,0) → 水變「棕線 on 藍底」直條紋。但 line 2 對 grass/山/沙是**正確**的(它們也用 line 2,render 對)→ 所以**我們的 line 2 解碼沒錯,真機上 tile14+line2 也會是棕**。
  - **關鍵結論**:水**不是**走「template 0 + 畫 tile」這條路徑。cell-type 0 很可能是**「畫水 backdrop」的 sentinel**,或 map-value→cell 映射/水 palette 另有規則(palette 動畫?backdrop 色?)。實測把 cell 0 強制 line 1(藍)→ 河流變藍、整圖連貫(`qa-md/map_cell0_line1.png`),證明 cell 0=水,但 line 1 是**經驗 band-aid 非正解**(原理可能錯,未 ship)。
  - **下一步(正解,需 RE)**:反組譯 Genesis ROM 的地圖繪製常式,查 cell-type 0 / 水 tile 14 的真實渲染路徑(sentinel? 專用水 palette? palette cycling 動畫?)。沿用本輪「map 結構反推 + 逐 cell render 對照實機」法。先別 ship 猜測修正(正確性優先)。
  - 影響:Genesis 主題地圖水域顯示錯(棕藍直條);其餘地形正確。**不影響 free/DOS/Amiga 三主題**。
  - 診斷產物(本機 qa-md/,gitignore):`md_cells_labeled.png`(72 cell 編號圖)、`md_tiles_0_95.png`(pattern tile)、`map_struct.png`(cell0 結構)、`map_render_real.png`/`map_cell0_line1.png`(實圖渲染對照)。

## ✅ 第八輪完成 (2026-06-21):F8 四主題 — free / DOS / Genesis / Amiga

- [x] **Amiga F8 主題完整整合**(amiga-crack 逆向+loader / 主線接線):
  - Amiga 圖形 = **同源 Okumura LZSS**(length+3,與 Genesis 同公式);容器 count/desc/32色palette/comp_size/**out_size**/stream;sequential planar(plane0=LSB)。
  - `src/lib/amiga-data.c`:`AMIGA_Resolve`(GR_TROOP/VILLAIN/PORTRAIT/LOCATION/TITLE/SELECT/VIEW/CURSOR/COMTILES/TILE/TILESET),讀已 unpack 的 GAME 散檔。
  - 接線:`kbconf` KBFAMILY_AMIGA、`env-sdl` KB_Resolve+gfx_themes(F8 加 Amiga)、`game.c` auto-detect(tileseta 標記)、`Makefile`、`build-appimage` 綁 Amiga + AppRun KB_AMIGA_GAME。
  - **引擎內驗證**:GR_TILESET(乾淨 8 欄地圖 tile,48×34 對齊)、GR_LOCATION(城堡背景)、GR_TROOP 皆正確(diag BMP)。
  - 62 個 Amiga 資源批次轉 PNG 驗證(out_size 100% 命中、尺寸對 free)。
  - **F8 現循環 free / DOS / Genesis / Amiga 四主題**。push 完成(e66c29c)。
- [x] 兩個逆向 agent(genesis-re / amiga-crack)任務完成,已關閉。

## 🔧 第七輪 (2026-06-21):Genesis 世界地形完整破解+實作

- [x] **Genesis 世界地圖 terrain 完整破解+實作** (genesis-re agent 逆向 + 主線整合):
  - 自製 **Okumura LZSS** 解壓器 ROM 0x18B0C(**length=(b2&0xf)+3**,off-by-one 是卡關主因)。
  - terrain pattern = LZSS 塊 **0x30E82**(638 tiles);cell template **0x19666**(6×5 metatile);map **0x1AA8E**(&0x7F→0..71 對齊引擎 72-tile);palette **0x256B8**(4 line)。
  - md-rom.c 實作 GR_TILE/GR_TILESET(C 版 LZSS + 快取 + template 組)+ 兩個引擎適配:**48×40 降採樣→48×34** 對齊 RECT_TILE、**index0 填海藍(0,72,180)不透明**(openkb 單 plane + draw_map 紅底,colorkey 會露紅)。
  - Python 重現驗證 = 乾淨原版世界地圖(qa-md/VERIFY_fixed.png)。commit b070979 + 修正(本地,未 push)。
  - 完整 recipe 在 skill ref 08 §8 + qa-md/FINDINGS.txt。**待實機 F8→Genesis 驗證**。
  - 殘留(次要):起始城堡區物件/動畫水波 tile placeholder。
- [ ] **Amiga 圖形**(amiga-crack agent):容器檔頭全破,圖形 codec 試 LZSS+3(NWC 同源,genesis-re 證實圖形=資料同一 LZSS,別假設第二 codec)。

## 🔧 (第六輪) 多版本素材分工 — A 背景 agent / B 主線

- **A (背景 agent genesis-re)**:反組譯 Genesis ROM 找地圖 tileset(疑壓縮)。進度見 skill ref 08。
- **B (主線) Amiga 素材**:`amiga-orig/` .adf **完整解開**(amitools xdftool)→ `extracted/KB/GAME/` 72 個**具名**資源:
  - 地圖 tiles:`tileseta`(36 子圖)/`tilesetb`/`tilesalt`;location 圖:cstl/town/cave/dngn/frst/plai;UI:select/title/view/comtiles/cursor/nwcp;troop sprite:peas/spri/… ;音效樣本:Bash/Bump/Death/Horns/King/Vict/Treas/Walk/Tele(8-bit signed PCM)。
  - **格式**:`子圖數(u16)` + 描述子`(flag,w,h)` + 12-bit RGB palette + planar 點陣。**但 raw planar 解出是雜訊 → 點陣為壓縮**(sprite=TTComp;pic/tile 疑 NWC 自有 RLE)。→ 需破解壓縮才能轉 PNG。
  - openkb 無 Amiga loader;導入路徑:解碼→PNG→當 free 格式 theme,或寫 KBFAMILY_AMIGA。
- **B 音樂現實 (誠實)**:只有 FM Towns 是真 looping BGM(CDDA,已完成設預設)。Amiga=SFX/jingle 取樣(非 BGM)。Genesis/PC98/DOS BGM=晶片序列,需模擬器側錄。→ 「各平台 BGM」對非 FMTowns 平台都要晶片模擬,成本高。

## 🔧 (第五輪) DOS 資料修復 + Genesis palette + 多版本素材計畫

優先序 (使用者定):**1 Genesis 完整化 → 2 各平台音樂 → 3 Amiga 美術+音樂**;FM Towns sprite 不提取;FM Towns 音樂設預設 (已成立)。

- [x] **KB.EXE 解壓** (EXECOMP):修 `Unable to resolve DAT_*` (troop 數值表 range/melee/skills/moves/cost/growth…) + tungrp。build-appimage 自動解壓 (<100KB 判定)。
- [x] **#2 Genesis 調色盤修正**:讀 ROM 真 CRAM palette (0x25698) 取代 EGA;9-bit→RGB888。troop/villain 顏色應正確。**待使用者進城堡 F8→Genesis 驗證**。
- [ ] **Genesis 完整化剩餘** (大型 RE):MD_Resolve 只做 troop/villain/world → tile/UI/cursor/select/title 全缺 → Genesis 主題地圖/UI 仍 free。要補各資源 ROM offset + 對應 palette。
- [ ] **Genesis 音樂** (VGM):屬優先序 2;ROM 內 YM2612 序列,需模擬器側錄。
- [ ] 各平台音樂 (DOS AdLib / Amiga MOD / PC98 YM2608)。
- [ ] Amiga 美術 (.adf planar loader,優先序 3)。

## 🔧 (前一輪) 進行中 / 待確認 (2026-06-21 第四輪:F8 + DOS 噪音釐清)

- [x] **DOS 美術一直正常** (誤判更正):DOS_Resolve(GR_TILE/GR_SELECT)=OK,tileseta/select/MCGA 等資源**都在 256.CC**(雜湊查找)。`Missing real file / Can't open / 256.CCL not found` 是 KB_fopen_with 先試 slotA 散檔(必缺)、再試 slotB(.CC,成功) 的 **fallback 常態噪音**,非載入失敗。已降為 KB_debuglog + 加 KB_VERBOSE 開關靜默。
- [x] F8 三主題循環 (free/DOS/Genesis),雜訊清除。
- [ ] **Genesis 只做 troop/villain** (MD_Resolve 限制):地圖 tile/UI fallback free → 「看起來像 free」。戰鬥 troop sprite 才是 Genesis。完整 Genesis 需實作全部 ROM offset (大型逆向,低優先)。
- [ ] 招募數量輸入殘影 (其他輸入點待確認)。
- [ ] 場景→音軌對照按耳朵校正 (城堡已 kb05)。

## 🔧 (前一輪) 進行中 / 待確認 (2026-06-21 第三輪實機)

- [x] 城堡/城鎮 BGM 接線 (進城堡播 kb05);F9 BGM codec 已 bundle (vorbis 等)。
- [x] 游標 twirl 殘影 (text_input 清欄)、側欄數字殘影 (draw_sidebar 清區)。
- [ ] **原版 intro 畫面仍 free**:openkb 標題流程 (display_logo / GR_TITLE "title.256" / NWCP logo) 沒取 DOS 資源。`256.CCL/cstl.256 not found` 為**非致命警告** (命名查找用 KB_ccHash 直接算,gameplay 美術確為原版 DOS)。待查標題流程。
- [ ] 招募「數量」輸入框等其他輸入點殘影 (text_input 已修;個別 input loop 待逐一確認)。
- [ ] 場景→音軌對照按耳朵校正 (城堡已設 kb05)。

## 🔧 (舊) 進行中 / 待確認

- [ ] **credits→選角 殘影修復驗證**:blit hook 改用 src 尺寸判斷 (已改碼,需重編 + 實機確認)。code: `KB_BlitSurface_hook` in env-sdl.c。
- [ ] commit 上述 blit hook + 重打包 release。
- [ ] **命名輸入框打字殘影** (game-tester issue #1,中):打字過程游標/舊字形殘留;最終名字正確。需查 text_input/name 重繪是否先清底。

## 📋 待辦 — 使用者需求 (多版本素材 / 音樂,見 ASSETS-PLAN.md)

### F8 美術主題切換
- [x] **Tier 1**:free ↔ DOS ↔ Genesis(MD) 循環。✅ KB_Resolve 圖形 pref 改用 kb_gfx_family 全域;F8 在已載入家族間循環+清快取重載;文字恆走 GNU。Genesis ROM (kb.bin) 自動偵測 + 個人版 AppImage 綁入。驗證:log 確認三主題循環無崩潰;adventure_state 有 SYN 計時器→地圖即時生效;選角畫面確認為 DOS 美術。
  - 注意:純靜態入口畫面 (選角/標題,無 SYN 計時器) F8 不即時更新,下次重畫才套用 (架構限制,可接受)。
- [ ] **Tier 2 (大型逆向,逐項評估)**:Amiga(.adf) / Apple II(.nib/.dsk) / PC98(.d88) / 完整 FM Towns 圖形 loader。openkb 無 loader,需逆向各磁碟+圖形格式。

### F9 音樂版本切換
- [x] **BGM 子系統** (src/bgm.c,SDL2_mixer):依場景循環播 OGG,F9 全域切換。✅
- [x] **FM Towns 音樂導入**:該版 CD 映像 16 條 CDDA → OGG (music/fmtowns/,gitignore)。✅
- [x] OGG 解碼:SDL2_mixer;AppImage 已 bundle vorbis/ogg codec 庫。✅
- [x] F9 循環版本 + 個人版 AppImage 綁 music。✅ (驗證:build 容器偵測+Mix 成功;AppImage 含 codec)
- [ ] **場景→音軌對照校正**:music/fmtowns/scenes.ini 為依長度推測版,需使用者**按耳朵校正** (哪條是城堡/各大陸/戰鬥)。
- [ ] 其他版本音樂 (dos/genesis/amiga/pc98/apple2):各自抽取/錄音 + scenes.ini (genesis ROM 可試 VGM;Amiga/PC98/Apple2 待評估)。
- [ ] (選用) 部分 FM Towns 非 UI 圖形採納。

### 跨平台打包 (2026-06-22)
- [x] **Linux AppImage**:`docker/build-appimage.sh`(公開 free 版 / 個人版綁原版素材)。✅
- [~] **GitHub Actions 多平台 CI**(`.github/workflows/build.yml`,手動觸發或推 v* tag):
  - 共用 `font` job 烤 cjk24.bin → linux(AppImage)/ windows(MSYS2 MINGW64,bundle SDL2 等 DLL + openkb.ini + play.bat)/ macos(brew SDL2 + dylibbundler 內嵌 .app)三平台;推 tag 自動建 Release。
  - 建公開 **free 版**(不含版權素材)。`docker/fetch-vendor.sh` 的 hfsutils 來源改 Debian https 鏡像(原 ftp.mars.org/CI 不穩)。
  - **v1 未經實跑驗證**:多平台 CI 無法本地測,首跑可能要 1–2 輪修(heredoc 縮排 / configure 跨平台 / ldd 路徑 / dylibbundler)。執行後依失敗 log 修。
- [ ] **Android(if possible)= 需專門移植,非單純打包**:openkb 無 SDL2 android-project;要 NDK build 全 C 源 + Java SDLActivity + assets 打包 + **觸控輸入對應**(遊戲是鍵盤操作,觸控需 on-screen 控制)。屬獨立 port 工作,CI 無法一步到位。
- [ ] 片頭 intro 畫面也要能 F8 切換版本。
- [ ] 部隊畫面版面微調 (UX,目前可接受)。
- [ ] 個人版打包綁多版本素材的流程 (版權:不入公開 repo)。
- [ ] GitHub Release 發佈 free 版 + 原版啟用說明 (README 已寫)。

## 版權鐵則
原版各平台 ROM/美術/音樂/手冊一律 **不入公開 repo、不隨公開包散布**;`original_kb/`、`dos-orig/` 已 gitignore。個人版 (-original / 含其他版本) 僅供自有正版者本機用。

## 素材位置 (本機,gitignore)
- `original_kb/`:DOS/Amiga/Apple II/FM Towns/Genesis ROM zip + PC98 (kings-bounty-pc98/) + 官方手冊 PDF。
- `dos-orig/kings-bounty/`:解開的 DOS 256.CC/416.CC/KB.EXE (混合模式測試用)。
