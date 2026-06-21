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

/* SDL_FillRect hook:遊戲對螢幕 surface 填色 (清背景/畫對話框) 時,連帶移除該
 * 區域的 CJK draw-list 項,避免上一畫面的中文殘留 (ghost)。實作在 env-sdl.c;
 * 該檔定義 KB_NO_FILLRECT_MACRO 以使用真正的 SDL_FillRect。 */
extern int KB_FillRect_hook(SDL_Surface *s, const SDL_Rect *r, Uint32 color);
#ifndef KB_NO_FILLRECT_MACRO
#define SDL_FillRect(s, r, c) KB_FillRect_hook((s), (r), (c))
#endif

/* SDL_BlitSurface hook:整屏/大面積背景圖 blit 到螢幕時 (dstrect=NULL 或涵蓋大半屏)
 * 連帶清該區 CJK,避免上一畫面中文殘留 (例:credits→選角 以背景圖覆蓋,非 FillRect)。
 * 小面積 blit (文字/sprite) 不觸發。 */
extern int KB_BlitSurface_hook(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);
#ifndef KB_NO_BLIT_MACRO
#define SDL_BlitSurface(src, sr, dst, dr) KB_BlitSurface_hook((src), (sr), (dst), (dr))
#endif

#endif /* OPENKB_SDLCOMPAT_H */
