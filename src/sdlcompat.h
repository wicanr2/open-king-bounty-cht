/*
 *  sdlcompat.h -- SDL 1.2 → SDL 2 相容層 (openkb 中文化 P1)
 *
 *  把散落在 codebase 的少量 SDL 1.2 專有名稱,以 SDL 2 等價物收斂於此,
 *  讓非視訊核心的呼叫點 (kbres.c / dos-data.c / free-data.c / combat.c …)
 *  不必逐處改寫。真正不同的視訊初始化 / flip / 事件迴圈在 env-sdl.c 與 ui.c 處理。
 */
#ifndef OPENKB_SDLCOMPAT_H
#define OPENKB_SDLCOMPAT_H

#include <SDL.h>

/* keysym 結構改名 */
typedef SDL_Keysym SDL_keysym;

/* SDL 1.2 的 Super 鍵在 SDL 2 改名為 GUI 鍵 */
#ifndef SDLK_LSUPER
#define SDLK_LSUPER SDLK_LGUI
#endif
#ifndef SDLK_RSUPER
#define SDLK_RSUPER SDLK_RGUI
#endif

/* SDL 1.2 surface flags 在 SDL 2 多已無意義,給定中性值以利編譯 */
#ifndef SDL_SWSURFACE
#define SDL_SWSURFACE 0
#endif
#ifndef SDL_HWSURFACE
#define SDL_HWSURFACE 0
#endif
#ifndef SDL_FULLSCREEN
#define SDL_FULLSCREEN 0
#endif

/* 調色盤:SDL 1.2 SDL_SetColors(surface, ...) → SDL 2 SDL_SetPaletteColors(palette, ...) */
#define SDL_SetColors(surf, cols, first, n) \
    SDL_SetPaletteColors((surf)->format->palette, (cols), (first), (n))

/* SDL 1.2 SDL_SetPalette(surface, flags, ...) → SDL 2 SDL_SetPaletteColors;
 * LOGPAL/PHYSPAL 旗標在 SDL 2 無意義,給 0。 */
#ifndef SDL_LOGPAL
#define SDL_LOGPAL 0
#define SDL_PHYSPAL 0
#endif
#define SDL_SetPalette(surf, flags, cols, first, n) \
    SDL_SetPaletteColors((surf)->format->palette, (cols), (first), (n))

/* colorkey:SDL 1.2 以 SDL_SRCCOLORKEY 旗標啟用,SDL 2 改用 SDL_TRUE。
 * 注意:對 "surface->flags & SDL_SRCCOLORKEY" 這種旗標測試,改用 SDL_HasColorKey(),
 * 不要套這個巨集 (本檔只覆蓋 SDL_SetColorKey 的啟用旗標用法)。 */
#ifndef SDL_SRCCOLORKEY
#define SDL_SRCCOLORKEY SDL_TRUE
#endif

#endif /* OPENKB_SDLCOMPAT_H */
