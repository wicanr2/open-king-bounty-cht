/*
 *  cjkfont.c -- CJK 點陣字 atlas + draw-list (openkb 中文化 P2)
 *  移植自 1oom 的同名模組;draw-list 字色改存 Uint32 RGB。
 *
 *  atlas 檔格式 (little endian):
 *     magic[8] "OKBCJK\0\1"
 *     u16 glyphW, u16 glyphH, u32 count
 *     count * { u32 codepoint ; glyphW*glyphH bytes alpha }
 *  codepoint 升序排列 -> 二分查找。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cjkfont.h"
#include "lib/kbstd.h"

static const char CJK_MAGIC[8] = { 'O','K','B','C','J','K','\0','\1' };

uint32_t cjk_text_fg = 0xFFFFFF; /* 預設白字 */
uint32_t cjk_text_bg = 0x000000; /* 預設黑底 */

static struct {
    uint8_t *raw;
    int w, h;
    uint32_t count;
    int recsz;            /* 4 + w*h */
    const uint8_t *recs;
} cjk = { 0 };

static uint32_t rec_cp(int i)
{
    const uint8_t *p = cjk.recs + (size_t)i * cjk.recsz;
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int cjkfont_init(const char *path)
{
    FILE *f;
    long len;
    uint8_t *raw;
    int w, h, recsz;
    uint32_t count;

    cjkfont_shutdown();

    if (!path || !path[0]) return 0;

    f = fopen(path, "rb");
    if (!f) {
        KB_errlog("CJK: cannot open font '%s'\n", path);
        return 0;
    }
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (len < 16) { fclose(f); return 0; }

    raw = malloc(len);
    if (!raw) { fclose(f); return 0; }
    if (fread(raw, 1, len, f) != (size_t)len) {
        fclose(f); free(raw); return 0;
    }
    fclose(f);

    if (memcmp(raw, CJK_MAGIC, 8) != 0) {
        KB_errlog("CJK: bad magic in '%s'\n", path);
        free(raw);
        return 0;
    }
    w = raw[8] | (raw[9] << 8);
    h = raw[10] | (raw[11] << 8);
    count = (uint32_t)raw[12] | ((uint32_t)raw[13] << 8)
          | ((uint32_t)raw[14] << 16) | ((uint32_t)raw[15] << 24);
    recsz = 4 + w * h;
    if (w <= 0 || h <= 0 || (long)(16 + (long)count * recsz) > len) {
        KB_errlog("CJK: corrupt header in '%s'\n", path);
        free(raw);
        return 0;
    }
    cjk.raw = raw;
    cjk.w = w;
    cjk.h = h;
    cjk.count = count;
    cjk.recsz = recsz;
    cjk.recs = raw + 16;
    KB_stdlog("CJK: loaded %u glyphs %dx%d from '%s'\n", count, w, h, path);
    return 1;
}

void cjkfont_shutdown(void)
{
    if (cjk.raw) free(cjk.raw);
    memset(&cjk, 0, sizeof(cjk));
}

int cjkfont_available(void)
{
    return (cjk.raw && cjk.count) ? 1 : 0;
}

const uint8_t *cjkfont_get(uint32_t cp, int *w, int *h)
{
    int lo, hi;
    if (!cjk.raw || cjk.count == 0) return NULL;
    lo = 0; hi = (int)cjk.count - 1;
    while (lo <= hi) {
        int mid = (lo + hi) >> 1;
        uint32_t c = rec_cp(mid);
        if (c == cp) {
            if (w) *w = cjk.w;
            if (h) *h = cjk.h;
            return cjk.recs + (size_t)mid * cjk.recsz + 4;
        } else if (c < cp) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return NULL;
}

int cjkfont_has(uint32_t cp) { return cjkfont_get(cp, NULL, NULL) != NULL; }
int cjkfont_glyph_w(void)    { return cjk.w; }
int cjkfont_glyph_h(void)    { return cjk.h; }

/* ---- CJK draw-list (逐 cell 鏡像螢幕) ----
 * 清單鏡像「螢幕上目前有哪些中文格」。draw-list 不隨 flip 整清,而是:
 *   - 在某格畫中文 → 取代該格舊項 (cjk_drawlist_add)。
 *   - 在某格畫 ASCII/點陣字 → 移除該格中文項 (cjk_drawlist_remove,由 inprint/
 *     KB_print 的 ASCII 路徑呼叫)。
 *   - 整塊清除 (FillRect/背景) → 移除該區中文項 (cjk_drawlist_remove_region)。
 * 如此「部分重畫只重印部分文字」時,未被覆寫的中文格保留,不會整片消失。 */

#define CJK_DRAWLIST_MAX 4096
static cjk_draw_t cjk_list[CJK_DRAWLIST_MAX];
static int cjk_n = 0;

static int cell_overlap(const cjk_draw_t *d, int x, int y) {
    return (x < d->x + d->cw && x + 1 > d->x && y < d->y + d->ch && y + 1 > d->y);
}

void cjk_drawlist_remove(int x, int y) {
    int i;
    for (i = 0; i < cjk_n; i++) {
        if (cell_overlap(&cjk_list[i], x, y)) {
            cjk_list[i] = cjk_list[--cjk_n]; /* swap with last */
            i--;
        }
    }
}

void cjk_drawlist_remove_region(int x, int y, int w, int h) {
    int i;
    for (i = 0; i < cjk_n; i++) {
        cjk_draw_t *d = &cjk_list[i];
        if (x < d->x + d->cw && x + w > d->x && y < d->y + d->ch && y + h > d->y) {
            cjk_list[i] = cjk_list[--cjk_n];
            i--;
        }
    }
}

void cjk_drawlist_add(int x, int y, uint32_t cp, uint32_t fg, uint32_t bg, uint8_t cw, uint8_t ch)
{
    cjk_draw_t *d;
    cjk_drawlist_remove(x, y);            /* 取代同格舊項,避免重複堆積 */
    if (cjk_n >= CJK_DRAWLIST_MAX) return;
    d = &cjk_list[cjk_n++];
    d->x = (int16_t)x;
    d->y = (int16_t)y;
    d->cp = cp;
    d->fg = fg;
    d->bg = bg;
    d->cw = cw;
    d->ch = ch;
}

void cjk_drawlist_flip(void) { /* 不再整清:清單鏡像螢幕,逐格增刪 */ }

void cjk_drawlist_clear(void) { cjk_n = 0; }
int  cjk_drawlist_count(void) { return cjk_n; }

const cjk_draw_t *cjk_drawlist_get(int i)
{
    return ((i >= 0) && (i < cjk_n)) ? &cjk_list[i] : NULL;
}
