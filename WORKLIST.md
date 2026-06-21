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

## 🔧 進行中 / 待確認 (2026-06-21 第三輪實機)

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

### 其他
- [ ] 片頭 intro 畫面也要能 F8 切換版本。
- [ ] 部隊畫面版面微調 (UX,目前可接受)。
- [ ] Windows / macOS / Android 打包 (CLAUDE.md 目標;Android = SDL2 NDK)。
- [ ] 個人版打包綁多版本素材的流程 (版權:不入公開 repo)。
- [ ] GitHub Release 發佈 free 版 + 原版啟用說明 (README 已寫)。

## 版權鐵則
原版各平台 ROM/美術/音樂/手冊一律 **不入公開 repo、不隨公開包散布**;`original_kb/`、`dos-orig/` 已 gitignore。個人版 (-original / 含其他版本) 僅供自有正版者本機用。

## 素材位置 (本機,gitignore)
- `original_kb/`:DOS/Amiga/Apple II/FM Towns/Genesis ROM zip + PC98 (kings-bounty-pc98/) + 官方手冊 PDF。
- `dos-orig/kings-bounty/`:解開的 DOS 256.CC/416.CC/KB.EXE (混合模式測試用)。
