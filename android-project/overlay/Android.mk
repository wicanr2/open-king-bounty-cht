# 遊戲原生 lib (libmain.so) — openkb 引擎全部 C 源。
# setup.sh 會把它放到 jni/src/Android.mk;並建 symlink jni/src/openkb -> repo root。
# 編出 libmain.so,由 SDLActivity 載入並呼叫 SDL_main(= openkb main)。

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL
OKB := openkb

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/$(SDL_PATH)/include \
    $(LOCAL_PATH)/$(OKB) \
    $(LOCAL_PATH)/$(OKB)/src \
    $(LOCAL_PATH)/$(OKB)/vendor

# config.h (setup.sh 放在 jni/src/config.h,即 $(LOCAL_PATH));#include "config.h" 經此找到

# 引擎源 = 桌面 Makefile.in 的 LIB_SOURCES + GAME_SOURCES + vendor + android.c。
# 不含 combat.c (那是 netkb 多人連線專用,需 SDL_net);不含 libhfs/librsrc (free 未引用)。
LOCAL_SRC_FILES := \
    $(OKB)/src/lib/kbstd.c $(OKB)/src/lib/kbconf.c \
    $(OKB)/src/lib/kbfile.c $(OKB)/src/lib/kbdir.c $(OKB)/src/lib/kbres.c \
    $(OKB)/src/lib/free-data.c $(OKB)/src/lib/free-snd.c \
    $(OKB)/src/lib/dos-data.c $(OKB)/src/lib/dos-cc.c $(OKB)/src/lib/dos-img.c \
    $(OKB)/src/lib/dos-snd.c $(OKB)/src/lib/dos-exe.c \
    $(OKB)/src/lib/md-rom.c \
    $(OKB)/src/lib/amiga-data.c \
    $(OKB)/src/lib/kbauto.c \
    $(OKB)/src/main.c $(OKB)/src/save.c $(OKB)/src/game.c $(OKB)/src/play.c \
    $(OKB)/src/bounty.c $(OKB)/src/env-sdl.c $(OKB)/src/ui.c $(OKB)/src/rogue.c \
    $(OKB)/src/cjkfont.c $(OKB)/src/inprint.c $(OKB)/src/bgm.c \
    $(OKB)/src/android.c $(OKB)/src/touch.c \
    $(OKB)/vendor/savepng.c $(OKB)/vendor/scale2x.c
    # 註:inprint 用 src/inprint.c (對齊桌面 LIB_SOURCES),非 vendor/inprint.c。
    #     vendor/strlcat.c/strlcpy.c 不編:bionic 已內建 (編了會 duplicate symbol)。

# 桌面靠 configure 產生這些;Android 無 configure → 用 setup.sh 放的 config.h + 這裡定義
LOCAL_CFLAGS := -DHAVE_CONFIG_H -DHAVE_LIBSDL -DHAVE_LIBSDL_IMAGE -Wno-format -Wno-implicit-function-declaration

LOCAL_SHARED_LIBRARIES := SDL2 SDL2_mixer SDL2_image

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -lOpenSLES -llog -landroid

include $(BUILD_SHARED_LIBRARY)
