APP_ABI := arm64-v8a armeabi-v7a
APP_PLATFORM := android-23
APP_STL := c++_shared
APP_CPPFLAGS := -frtti -fexceptions
# 暫允許缺少的 shared 相依 (SDL2_mixer/image 的 codec 子專案尚未備齊);
# 先讓引擎 C 在 NDK 編出 libmain.so、露出下一層問題。正式版需補齊 codec 相依。
APP_ALLOW_MISSING_DEPS := true
