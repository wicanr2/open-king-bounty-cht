/*
 *  touch.h -- Android 觸控覆蓋層 (僅 __ANDROID__)
 *
 *  手指事件 → 合成 SDL_KEYDOWN/KEYUP(SDLK_*)→ 餵回既有 KB_event 事件迴圈;
 *  引擎主流程零改動。並在 RenderPresent 前把 D-pad / A / B 畫上去。
 */
#ifndef OPENKB_TOUCH_H
#define OPENKB_TOUCH_H

#ifdef __ANDROID__
#include "sdlcompat.h"   /* SDL */

/* 把一個 SDL_FINGER* 事件「就地改寫」成對應的 KEYDOWN/KEYUP。
 * 回傳 1 = 已改寫 (呼叫端續用既有按鍵處理);0 = 非控制區,呼叫端應略過此事件。 */
int  touch_translate(SDL_Event *e);

/* 在遊戲畫面 RenderCopy 之後、RenderPresent 之前畫觸控控制 (真實視窗 px)。 */
void touch_render(SDL_Renderer *r, int win_w, int win_h);

#endif /* __ANDROID__ */
#endif
