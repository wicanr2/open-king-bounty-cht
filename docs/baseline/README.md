# Baseline 截圖 (SDL 1.2,英文,free 模組)

中文化動工前的英文原貌,用 `docker/screenshot.sh` 在 headless Xvfb 內擷取 (640×400,filter=normal2x)。SDL2 化 (P1) 與 CJK 渲染 (P2) 後會再補對照圖。

| 檔案 | 畫面 | 之後的翻譯/改動對象 |
|---|---|---|
| `credits.png` | 製作名單 (King's Bounty Designed By…) | 美術 + 字型渲染正常的證明 |
| `character-select.png` | 角色選擇 (Select Char A-D…) | 寫死英文 UI 字串 + 8×8 字型 (將擴 CJK) |

重現方式:
```sh
docker build -t openkb-build -f docker/Dockerfile .
docker run --rm -v "$PWD":/src -w /src openkb-build \
  bash -c 'apt-get update -qq && apt-get install -y -qq xdotool >/dev/null 2>&1; sh docker/screenshot.sh'
```
