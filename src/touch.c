/*
 *  touch.c -- Android 觸控覆蓋層 (僅 __ANDROID__)
 *
 *  設計見 docs/android/ui-design.md。本階段 (phase 3) 做最小可玩控制:
 *  左下 D-pad(方向)、右下 A(Enter 確認)/ B(ESC 取消)。
 *  手指 → 合成 SDLK_*(在 ui.c 的 KB_event 迴圈就地改寫 FINGER 事件)→ 引擎照常處理。
 *  情境快捷列(字母選項)、☰ 系統選單為後續 phase。
 */
#ifdef __ANDROID__

#include "touch.h"

/* 控制元件版面(由視窗 px 計算;單位 u ≈ 螢幕高 1/6,拇指好按)。 */
static void layout(int ww, int wh, SDL_Rect *dpad, SDL_Rect *btnA, SDL_Rect *btnB)
{
	int u = wh / 6;          /* 按鍵單位 */
	int m = wh / 14;         /* 邊距 */
	dpad->w = dpad->h = 2 * u;          /* 左下 D-pad (邊長 2u) */
	dpad->x = m;
	dpad->y = wh - dpad->h - m;
	btnA->w = btnA->h = u;              /* A:右下,略內上 */
	btnA->x = ww - 2 * u - m;
	btnA->y = wh - u - m - (u / 2);
	btnB->w = btnB->h = u;              /* B:右下角 */
	btnB->x = ww - u - m;
	btnB->y = wh - u - m;
}

static int in_rect(int x, int y, const SDL_Rect *r)
{
	return x >= r->x && x < r->x + r->w && y >= r->y && y < r->y + r->h;
}

/* 視窗 px 座標 → 對應的 SDLK_*(0 = 非控制區)。 */
static int hit_key(int px, int py, int ww, int wh)
{
	SDL_Rect dpad, btnA, btnB;
	layout(ww, wh, &dpad, &btnA, &btnB);
	if (in_rect(px, py, &btnA)) return SDLK_RETURN;
	if (in_rect(px, py, &btnB)) return SDLK_ESCAPE;
	if (in_rect(px, py, &dpad)) {
		int cx = dpad.x + dpad.w / 2;
		int cy = dpad.y + dpad.h / 2;
		int dx = px - cx, dy = py - cy;
		int dead = dpad.w / 8;
		if (dx > -dead && dx < dead && dy > -dead && dy < dead) return 0; /* 中心死區 */
		if (abs(dx) > abs(dy)) return dx < 0 ? SDLK_LEFT : SDLK_RIGHT;
		return dy < 0 ? SDLK_UP : SDLK_DOWN;
	}
	return 0;
}

/* 多指追蹤:記住每根手指按下時對應的鍵,放開時送對應 KEYUP。 */
#define MAXF 10
static struct { SDL_FingerID id; int used; int key; } g_fmap[MAXF];
static int g_ww = 1, g_wh = 1;   /* 由 touch_render 每幀更新 */

static void rewrite_key(SDL_Event *e, Uint32 type, int sym)
{
	SDL_memset(&e->key, 0, sizeof(e->key));
	e->type = type;
	e->key.type = type;
	e->key.state = (type == SDL_KEYDOWN) ? SDL_PRESSED : SDL_RELEASED;
	e->key.keysym.sym = sym;
	e->key.keysym.scancode = SDL_GetScancodeFromKey(sym);
	e->key.keysym.mod = KMOD_NONE;
}

int touch_translate(SDL_Event *e)
{
	int px, py, i, free_slot, sym;
	switch (e->type) {
	case SDL_FINGERDOWN:
		px = (int)(e->tfinger.x * g_ww);
		py = (int)(e->tfinger.y * g_wh);
		sym = hit_key(px, py, g_ww, g_wh);
		if (!sym) return 0;                 /* 非控制區 → 略過 */
		free_slot = -1;
		for (i = 0; i < MAXF; i++) {
			if (g_fmap[i].used && g_fmap[i].id == e->tfinger.fingerId) { free_slot = i; break; }
			if (free_slot < 0 && !g_fmap[i].used) free_slot = i;
		}
		if (free_slot >= 0) { g_fmap[free_slot].id = e->tfinger.fingerId; g_fmap[free_slot].used = 1; g_fmap[free_slot].key = sym; }
		rewrite_key(e, SDL_KEYDOWN, sym);
		return 1;
	case SDL_FINGERUP:
		for (i = 0; i < MAXF; i++) {
			if (g_fmap[i].used && g_fmap[i].id == e->tfinger.fingerId) {
				sym = g_fmap[i].key;
				g_fmap[i].used = 0;
				rewrite_key(e, SDL_KEYUP, sym);
				return 1;
			}
		}
		return 0;
	case SDL_FINGERMOTION:
	default:
		return 0;   /* phase 3 不處理滑動;放開才送 KEYUP */
	}
}

/* 半透明填色矩形小工具。 */
static void fill(SDL_Renderer *r, int x, int y, int w, int h, Uint8 cr, Uint8 cg, Uint8 cb, Uint8 ca)
{
	SDL_Rect rc = { x, y, w, h };
	SDL_SetRenderDrawColor(r, cr, cg, cb, ca);
	SDL_RenderFillRect(r, &rc);
}

void touch_render(SDL_Renderer *r, int win_w, int win_h)
{
	SDL_Rect dpad, btnA, btnB;
	int arm, t, cx, cy;
	g_ww = win_w; g_wh = win_h;
	layout(win_w, win_h, &dpad, &btnA, &btnB);

	SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

	/* D-pad:暗底 + 白色十字 */
	fill(r, dpad.x, dpad.y, dpad.w, dpad.h, 0, 0, 0, 90);
	cx = dpad.x + dpad.w / 2; cy = dpad.y + dpad.h / 2;
	arm = dpad.w / 2; t = dpad.w / 6;
	fill(r, cx - t / 2, cy - arm, t, 2 * arm, 255, 255, 255, 150);   /* 直 */
	fill(r, cx - arm, cy - t / 2, 2 * arm, t, 255, 255, 255, 150);   /* 橫 */

	/* A(確認,綠)、B(取消,紅);圓角省略,以方塊呈現 */
	fill(r, btnA.x, btnA.y, btnA.w, btnA.h, 40, 200, 80, 170);
	fill(r, btnB.x, btnB.y, btnB.w, btnB.h, 210, 60, 60, 170);

	SDL_SetRenderDrawColor(r, 0, 0, 0, 255);   /* 還原 */
}

#endif /* __ANDROID__ */
