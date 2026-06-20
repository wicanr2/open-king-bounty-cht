/*
 *  cjkfont.h -- CJK 點陣字 atlas + draw-list (openkb 中文化 P2)
 *
 *  移植自 1oom (master-of-orion) 的同名模組。openkb 版差異:
 *    - draw-list 的字色改存 Uint32 RGB (openkb 無 1oom 的 palette 陣列)。
 *  atlas 由 tools/build_cjk_font.py 產生 (magic "OKBCJK\0\1")。
 *  渲染流程見 docs/adr/0001 與 rules/81-retro-cjk-hires-canvas。
 */
#ifndef OPENKB_CJKFONT_H
#define OPENKB_CJKFONT_H

#include <SDL.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 從 path 載入 atlas;成功回傳 1。重複呼叫安全 (先釋放舊的)。 */
int  cjkfont_init(const char *path);
void cjkfont_shutdown(void);

/* atlas 是否已載入且有任一字。 */
int  cjkfont_available(void);

/* 取得 cp 的 glyph alpha (gw*gh bytes,0..255);找不到回 NULL。 */
const uint8_t *cjkfont_get(uint32_t cp, int *w, int *h);
int  cjkfont_has(uint32_t cp);

int  cjkfont_glyph_w(void);
int  cjkfont_glyph_h(void);

/* ---- CJK draw-list ----
 * KB_print 在 320×200 邏輯座標遇到 CJK 時不畫進 screen surface,而是把
 * (x, y, codepoint, RGB字色, 字高) 記進 list;KB_flip 時在 640×400 合成層
 * 以 glyph + 黑外框畫出。double-buffer 綁 flip,避免重複 present 時閃爍。 */
typedef struct cjk_draw_s {
    int16_t  x, y;     /* 320×200 邏輯座標 (glyph 左、文字行頂) */
    uint32_t rgb;      /* 0x00RRGGBB 字色 */
    uint8_t  fonth;    /* 當下原版字高,供垂直對齊 */
    uint32_t cp;       /* Unicode 碼點 */
} cjk_draw_t;

void cjk_drawlist_add(int x, int y, uint32_t cp, uint32_t rgb, uint8_t fonth);
void cjk_drawlist_flip(void);
void cjk_drawlist_clear(void);
int  cjk_drawlist_count(void);
const cjk_draw_t *cjk_drawlist_get(int i);

#ifdef __cplusplus
}
#endif

#endif /* OPENKB_CJKFONT_H */
