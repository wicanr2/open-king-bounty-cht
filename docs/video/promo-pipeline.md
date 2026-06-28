# 推廣影片 Pipeline 與設計

御封戰將(openkb 繁中)推廣影片的設計理念、分鏡,與可重現的 headless 製作流程。
產物:`dist-all/video/openkb-cht-promo.mp4`(約 50 秒,含 FM-Towns 音樂)。
腳本:[`scripts/capture-promo.sh`](../../scripts/capture-promo.sh)(擷取)+ [`scripts/make-promo.sh`](../../scripts/make-promo.sh)(組裝)。
製作環境:[`docker/Dockerfile.capture`](../../docker/Dockerfile.capture)(build 環境 + ffmpeg + Noto CJK = `openkb-capture` image)。

## 設計理念(這支片要說什麼)

一支 50 秒的片要在沒有旁白的情況下,讓人 10 秒內看懂「這是什麼、做了什麼、能不能用」。三個訊息,各用畫面自己證明,不用嘴說:

1. **這是經典老遊戲的正版重現** → 用「原版開場」(New World Computing 商標 + 原版 King's Bounty 標題)開場,一眼喚起情懷與正統感。
2. **完整繁體中文化、跨平台** → 實機 showcase:原版美術配中文 UI,標題卡點明 Windows / macOS / Linux。
3. **品質可信(我們真的在修)** → 直接秀剛修好的戰鬥畫面(乾淨無殘留),用「眼見為憑」取代「我們修了 bug」這種空話。

收尾給下載入口。整支不放浮誇形容詞,讓畫面與真實遊戲音樂自己說話。

## 分鏡(storyboard)

| # | 段落 | 長度 | 畫面 | 字幕 | 音 |
|---|---|---|---|---|---|
| 1 | 標題卡 | 3.5s | 深底 +「御封戰將」金字 | King's Bounty (1990)　繁體中文化　Win·macOS·Linux | 音樂淡入 |
| 2 | **原版開場** | ~8s | NWC 商標(黑底)→ 原版 King's Bounty 標題 | 原版開場 · New World Computing／King's Bounty 標題 | 實機音樂 |
| 3 | 實機 showcase | ~26s | 進地圖、移動、F8 切美術主題 | 原版美術 ＋ 全程繁體中文化 · F8 切 DOS/Genesis/Amiga | 實機音樂 |
| 4 | **戰鬥(修正證明)** | ~9s | 乾淨戰鬥畫面(無殘留) | 戰鬥畫面修復 · 介面乾淨無殘留 | 實機音樂 |
| 5 | 結尾卡 | 3.5s | 深底 +「免費下載」+ GitHub 連結 | — | 音樂淡出 |

> 順序刻意是「情懷 → 賣點 → 信任 → 行動」。第 4 段是這次製作的核心目的(證明修正),用 `KB_DEBUG_COMBAT` 才能穩定錄到。

## 技術 Pipeline

### 1) 擷取(capture-promo.sh)

headless 擷取「畫面 + 真實遊戲音樂」,三個關鍵技巧:

- **Xvfb 虛擬螢幕 + ffmpeg `x11grab`**:無實體顯示器也能錄。先 `xdotool getwindowgeometry` 抓遊戲視窗實際區域 → ffmpeg 只擷該區(去黑邊)。
- **`SDL_AUDIODRIVER=disk` 擷取音樂**:headless 沒有音效裝置,但 SDL 的 disk 音訊驅動會把 SDL2_mixer 混好的輸出(FM-Towns OGG)寫成 raw PCM 檔(`SDL_DISKAUDIOFILE`),再 `ffmpeg -f s16le … → wav`。這是 headless 拿得到聲音的核心。
- **時間軸對齊的 `xdotool` 驅動**:背景送鍵的子行程與 ffmpeg 同時起跑(t=0 對齊),用 `sleep` 排出每個動作的時間點。全程有界(固定秒數 `timeout -s KILL`),不開 GUI viewer、無 sentinel 迴圈(遵守背景存活性紀律)。
- **`KB_DEBUG_COMBAT` 開發者鉤子**:設此環境變數即跳過選單、spawn 預設遊戲並直接切入一場戰鬥 → 穩定錄到戰鬥畫面(否則得在地圖盲走找敵人,headless 極不可靠)。見 `src/game.c` 的 `run_game`。
- 用**完整版資料**(`release/promodata`:free + DOS + Genesis + Amiga + music)→ 才有原版開場、F8 四主題、FM-Towns 音樂。

產物:`release/cap_intro.{mp4,wav}`、`cap_show.{mp4,wav}`、`cap_combat.{mp4,wav}`。

### 2) 組裝(make-promo.sh)

- **卡片**:ImageMagick `convert` 用 Noto CJK 烤全螢幕標題/結尾卡(避免 ffmpeg drawtext 對 `.ttc` 字型集的麻煩)。
- **字幕條**:半透明底 + 金字 PNG,`overlay` 疊在實機片段底部。
- **實機片段標準化**:`scale=…force_original_aspect_ratio=decrease` + `pad` → 統一 1280×720、置中 letterbox;各段配自己擷到的音樂。
- **鋪音樂**:卡片段取一小段擷到的遊戲音樂當底(afade 淡入淡出);實機段用該段同步音樂。
- **串接**:全部標準化成同參數 mp4 後用 concat demuxer 接起 → `-movflags +faststart`(利於網路串流)。

## 重現步驟

```bash
cd openkb-code
# 0) 一次性:建擷取用 image(build 環境 + ffmpeg + Noto CJK)
docker build --network host -t openkb-capture -f docker/Dockerfile.capture .
# 1) 備完整資料(從完整版 AppImage 取 $SHARE,含 4 主題 + 音樂)
mkdir -p release/promodata
./dist-all/KingsBounty-CHT-full-linux-x86_64.AppImage --appimage-extract  # 取 squashfs-root/.../open-king-bounty-cht/* → release/promodata/
# 2) 擷取 + 組裝(容器內)
docker run --rm --network host -v "$PWD":/src -w /src openkb-capture sh -c \
  'make && sh scripts/capture-promo.sh && sh scripts/make-promo.sh'
# 產物:dist-all/video/openkb-cht-promo.mp4
```

## 可調整處(knobs)

- **片長/節奏**:各 `cap <name> <secs>`(擷取秒數)+ 卡片 `cardseg … <dur>`。
- **分鏡**:在 `make-promo.sh` 的 concat 清單增刪段落(可加城鎮/招兵/F9 換樂段)。
- **字幕/卡片文案**:`capstrip` / `card` 的字串。
- **解析度**:`make-promo.sh` 的 `W`/`H`(預設 1280×720)。
- **游標**:目前開場 logo/標題會有極小的遊戲游標「✕」;若要乾淨,可在 `display_logo`/`display_title` 期間隱藏游標再錄。

## 版權注意

完整版資料(原版 DOS/Genesis/Amiga 美術、FM-Towns 音樂)為原作版權素材。本片用於粉絲漢化推廣;公開散布與否由專案維護者斟酌。若需「純自由素材」安全版,可改用 free 資料(無原版開場、無背景音樂或換自由音樂)重錄。

## 來源

製作方法移植自 `indiana-jones-and-last-crusade-cht`(ScummVM FM-Towns 介紹片:Xvfb + x11grab + SDL disk 音訊 + 繁中卡片組裝)。openkb 版加上 `KB_DEBUG_COMBAT` 直接切入戰鬥、F8 多主題 showcase、原版開場段。
