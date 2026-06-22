# Android 移植(骨架)

御封戰將的 Android 版。設計見 [`../docs/android/ui-design.md`](../docs/android/ui-design.md)。
本目錄是**建置骨架**:用 SDL2 官方 android-project 模板 + NDK 編譯引擎 C 源 → `libmain.so`,
由 `SDLActivity` 載入;`data/free` + `cjk24.bin` 打包進 APK assets,首次啟動由 Java 複製到
內部儲存,引擎以內部儲存路徑用 stdio 讀寫(`src/android.c` 的 `android_bootstrap`)。

> 狀態:**骨架階段,尚未在 NDK 實際編譯通過**。`setup.sh` 把專案組裝出來,但需要
> Android SDK/NDK 跑一輪 build,預期要修(config.h 的 HAVE_* / SDL2_mixer·image 的
> android build / 各源相依)。建議用 GitHub Actions 的 ubuntu runner(預裝 SDK/NDK)或
> docker NDK image 驅動。觸控覆蓋層(D-pad/A·B/情境鍵)為下一階段,尚未做。

## 架構

```
APK
├─ libSDL2.so / libSDL2_mixer.so / libSDL2_image.so   (SDL 源碼編)
├─ libmain.so                                          (openkb 引擎 C 源)
├─ org.libsdl.app.SDLActivity (Java glue,SDL 提供)
│    └─ OpenKBActivity extends SDLActivity
│         · loadLibraries: SDL2, SDL2_mixer, SDL2_image, main
│         · onCreate: 首次把 assets/data → getFilesDir()/data
└─ assets/data/free/*, assets/data/cjk24.bin
```

- **輸入**:`SDLActivity` 已把觸控轉成 `SDL_FINGER*` 事件;觸控覆蓋層(下一階段)會把手指
  合成 `SDL_KEYDOWN(SDLK_*)` 塞回事件佇列,餵進引擎既有 `KB_event()`,主流程零改動。
- **存檔/資料**:走內部儲存(`SDL_AndroidGetInternalStoragePath()`),非唯讀 cwd。
- **設定**:Android 無命令列/config 檔 → `src/android.c` 程式化設 datadir/savedir + 掛 free 模組。

## 建置

需要:Android SDK(platform + build-tools)、NDK、JDK 17。

```sh
sh android-project/setup.sh          # 抓 SDL2/image/mixer 源 + 組裝專案 + stage assets
cd android-project/build
./gradlew assembleDebug              # → app/build/outputs/apk/debug/*.apk
```

`setup.sh` 會:
1. 下載 SDL2 / SDL2_image / SDL2_mixer 源碼(版本見腳本)到 `build/app/jni/`。
2. 以 SDL2 的 `android-project/` 為模板,覆蓋 openkb 的 `AndroidManifest.xml` /
   `OpenKBActivity.java` / `jni/src/Android.mk`(引擎源清單)/ `jni/Application.mk` / `config.h`。
3. 把 repo 的 `src/`、`vendor/` 連結進 `jni/src/`,把 `data/free` + `cjk24.bin` 放進 `assets/data/`。

## 待辦(本 branch 後續)

1. NDK build 跑通(config.h 的 HAVE_* 補齊、SDL2_mixer/image android 相依、libhfs/librsrc 納入)。
2. 觸控覆蓋層:D-pad + A/B + ☰(讓地圖能走、選單能用 Enter/ESC)。
3. 情境快捷列(讀 keymap)+ 直接點選單項目。
4. 數字步進器、命名 IME、生命週期(pause/resume 存檔)。
5. 真機實測 + 手感打磨。

方法論見 skill `retro-keyboard-to-touch`。
