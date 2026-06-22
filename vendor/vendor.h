/*
 * vendor.h -- externs for bare third-party code 
 */

#include "../src/config.h"

#ifdef HAVE_LIBSDL
#include <SDL.h>

/* scale2x.c */
extern void scale2x(SDL_Surface *src, SDL_Surface *dst);

/* inprint.c */
extern void prepare_inline_font(void);
extern void kill_inline_font(void);
extern void infont(SDL_Surface *font);
extern void incolor(Uint32 fore, Uint32 back);
extern void inprint(SDL_Surface *dst, const char *str, Uint32 x, Uint32 y);
extern SDL_Surface* get_inline_font(void);

/* savepng.c */
#include "savepng.h"

#endif

/* mingw 的 C lib 有 strlcat/strlcpy (configure 測到 HAVE_STRLCAT=1) 但 <string.h>
 * 沒給原型;新版 gcc 預設 C23 把 implicit declaration 當錯誤 → 補宣告。 */
#if !defined(HAVE_STRLCAT) || defined(__MINGW32__)
/* strlcat.c */
extern size_t strlcat(char *dst, const char *src, size_t siz);
#endif

#if !defined(HAVE_STRLCPY) || defined(__MINGW32__)
/* strlcpy.c */
extern size_t strlcpy(char *dst, const char *src, size_t siz);
#endif
