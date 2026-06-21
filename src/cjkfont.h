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
    int16_t  x, y;     /* 320×200 邏輯座標 (cell 左上) */
    uint32_t fg;       /* 0x00RRGGBB 字色 */
    uint32_t bg;       /* 0x00RRGGBB 底色 (反白/填底用;0xFFFFFFFF=不填底) */
    uint8_t  cw, ch;   /* cell 寬高 (邏輯像素),供填底與字框定位 */
    uint32_t cp;       /* Unicode 碼點 (含 ASCII) */
} cjk_draw_t;

#define CJK_BG_NONE 0xFFFFFFFFu /* 不填底 */

/* 目前文字前景/底色 (0x00RRGGBB),由 incolor()/KB_setcolor() 設定,
 * inprint 與 KB_print 共用 (兩者其實共用同一個 font surface)。 */
extern uint32_t cjk_text_fg;
extern uint32_t cjk_text_bg;

void cjk_drawlist_add(int x, int y, uint32_t cp, uint32_t fg, uint32_t bg, uint8_t cw, uint8_t ch);
void cjk_drawlist_remove(int x, int y);                 /* 移除涵蓋該點的中文格 (ASCII 覆寫時) */
void cjk_drawlist_remove_region(int x, int y, int w, int h); /* 移除該矩形內中文格 (FillRect/背景) */
void cjk_drawlist_flip(void);
void cjk_drawlist_clear(void);
int  cjk_drawlist_count(void);
const cjk_draw_t *cjk_drawlist_get(int i);

#ifdef __cplusplus
}
#endif

#endif /* OPENKB_CJKFONT_H */
