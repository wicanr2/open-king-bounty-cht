/*
 *  env-sdl.c -- resource adapter for SDL
 *  Copyright (C) 2011 Vitaly Driedfruit
 *
 *  This file is part of openkb.
 *
 *  openkb is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  openkb is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with openkb.  If not, see <http://www.gnu.org/licenses/>.
 */
#define KB_NO_FILLRECT_MACRO /* 本檔用真正的 SDL_FillRect (實作 hook) */
#include "sdlcompat.h"

#include "lib/kbconf.h"
#include "lib/kbres.h"
#include "lib/kbsound.h"

#include "../vendor/vendor.h"
#include "env.h"
#include "cjkfont.h"

#ifdef HAVE_LIBSDL_IMAGE
#include <SDL_image.h>
#endif

/* Forward-declare audio callback */
void KBenv_audio_callback(void *userdata, Uint8 *stream, int len);

/*
 * Default config.
 * Yeah, a local global variable :/
 * NULL by default, to ensure clean segfaults if no default was provided.
 */
KBconfig *conf = NULL;

/*
 * SDL2 視訊管線 (file-static,如同上方的 conf)。
 * 邏輯畫布固定 320x200;視窗放大與縮放交給 renderer (對齊 1oom CJK 模型)。
 */
#define KB_LOGICAL_W 320
#define KB_LOGICAL_H 200
static SDL_Window   *g_window   = NULL;
static SDL_Renderer *g_renderer = NULL;
static SDL_Texture  *g_texture  = NULL;

/* 遊戲螢幕 surface 指標,供 SDL_FillRect hook 判斷是否要連帶清 CJK 區域 */
SDL_Surface *KB_screen = NULL;
int KB_FillRect_hook(SDL_Surface *s, const SDL_Rect *r, Uint32 color) {
	if (s == KB_screen) {
		if (r) cjk_drawlist_remove_region(r->x, r->y, r->w, r->h);
		else   cjk_drawlist_clear();
	}
	return SDL_FillRect(s, r, color); /* 此檔 macro 已停用 → 真正的 SDL_FillRect */
}

/* 供 game.c 改視窗標題 (取代 SDL_WM_SetCaption) */
void KB_setcaption(const char *title) {
	if (g_window) SDL_SetWindowTitle(g_window, title);
}

/* ===== CJK 渲染 (移植自 1oom;見 docs/adr/0001、rules/81) =====
 * 邏輯畫布 320x200,合成層 2x (640x400)。CJK 不畫進 screen surface,而是經
 * draw-list 在合成層以 atlas glyph + 黑外框疊上,確保彩色背景上可讀。 */
#define CJK_SCALE   2
#define CJK_CACHE_N 2048

static struct {
	SDL_Texture *composite;   /* 640x400 render target */
	int cw, ch;
	int gw, gh;               /* 目前 glyph cache 對應的 atlas 尺寸 */
	struct { uint32_t cp; SDL_Texture *tex; } cache[CJK_CACHE_N];
} cjkv = { 0 };

/* 取得 (或建立並快取) 某碼點的白色 glyph texture (alpha 來自 atlas) */
static SDL_Texture *cjk_glyph_tex(uint32_t cp) {
	int gw = cjkfont_glyph_w(), gh = cjkfont_glyph_h();
	unsigned h0, i;
	if (gw <= 0 || gh <= 0) return NULL;
	if (cjkv.gw != gw || cjkv.gh != gh) { /* atlas 尺寸變了,清快取 */
		int k;
		for (k = 0; k < CJK_CACHE_N; k++)
			if (cjkv.cache[k].tex) SDL_DestroyTexture(cjkv.cache[k].tex);
		memset(cjkv.cache, 0, sizeof(cjkv.cache));
		cjkv.gw = gw; cjkv.gh = gh;
	}
	h0 = (cp * 2654435761u) & (CJK_CACHE_N - 1);
	for (i = 0; i < CJK_CACHE_N; i++) {
		unsigned idx = (h0 + i) & (CJK_CACHE_N - 1);
		if (cjkv.cache[idx].tex) {
			if (cjkv.cache[idx].cp == cp) return cjkv.cache[idx].tex;
			continue;
		} else {
			const uint8_t *a = cjkfont_get(cp, NULL, NULL);
			SDL_Texture *t;
			Uint32 *px;
			int p;
			if (!a) return NULL;
			t = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_ARGB8888,
				SDL_TEXTUREACCESS_STATIC, gw, gh);
			if (!t) return NULL;
			px = malloc(gw * gh * 4);
			if (!px) { SDL_DestroyTexture(t); return NULL; }
			for (p = 0; p < gw * gh; p++)
				px[p] = ((Uint32)a[p] << 24) | 0x00FFFFFFu; /* 白 + atlas alpha */
			SDL_UpdateTexture(t, NULL, px, gw * 4);
			free(px);
			SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
			SDL_SetTextureScaleMode(t, SDL_ScaleModeLinear); /* 24→16 平滑縮小 */
			cjkv.cache[idx].cp = cp;
			cjkv.cache[idx].tex = t;
			return t;
		}
	}
	return NULL; /* cache 滿 (2048 槽,實務上夠) */
}

/* 直接在 logical (320x200) 座標把 drawlist 的 CJK 畫到目前 render target。
 * 由 renderer 的 logical size 一次縮放到視窗 (不經 640 合成層再降回,較銳利),
 * 且滑鼠座標仍維持在 320 邏輯空間。黑外框確保任何背景上可讀。 */
static void cjk_draw_overlay(void) {
	static const int ox[4] = { -1, 1, 0, 0 };
	static const int oy[4] = {  0, 0,-1, 1 };
	int n = cjk_drawlist_count();
	int i, k;
	for (i = 0; i < n; i++) {
		const cjk_draw_t *d = cjk_drawlist_get(i);
		SDL_Texture *t = cjk_glyph_tex(d->cp);
		SDL_Rect cell;
		Uint8 r, g, b;
		if (!t) continue;

		cell.x = d->x; cell.y = d->y; cell.w = d->cw; cell.h = d->ch;

		/* 4 方位黑外框 (logical ±1) */
		SDL_SetTextureColorMod(t, 0, 0, 0);
		for (k = 0; k < 4; k++) {
			SDL_Rect o = cell; o.x += ox[k]; o.y += oy[k];
			SDL_RenderCopy(g_renderer, t, NULL, &o);
		}
		/* 字色 (太暗自動提亮) */
		r = (d->fg >> 16) & 0xFF; g = (d->fg >> 8) & 0xFF; b = d->fg & 0xFF;
		if ((r * 30 + g * 59 + b * 11) / 100 < 60) { r = g = b = 235; }
		SDL_SetTextureColorMod(t, r, g, b);
		SDL_RenderCopy(g_renderer, t, NULL, &cell);
	}
}

/*
 * Start/Stop the "environment"
 */
KBenv *KB_startENV(KBconfig *conf) {

	Uint32 iflags, winflags;
	int win_w, win_h;

	SDL_AudioSpec desired;

	char *iconfile;

	KBenv *nsys = malloc(sizeof(KBenv));

	if (!nsys) {
		KB_errlog("Out of memory!\n");
		return NULL;
	}


	iflags = SDL_INIT_VIDEO;

	if (conf->sound) {
		iflags |= SDL_INIT_AUDIO;
	}

	if ( SDL_Init( iflags ) == -1 ) {
		KB_errlog("Couldn't initialize SDL: %s\n", SDL_GetError());
		free(nsys);
		return NULL;
	}

	/* 邏輯畫布永遠 320x200,zoom=1:所有繪圖在原生解析度進行,放大由 renderer 處理。
	 * 底圖一律用 nearest 保持像素銳利 (pixel art 不糊)。 */
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
	conf->filter = 0;

	nsys->pan = 0;
	nsys->zoom = 1;

	/* 預設視窗為邏輯畫布的 2x (640x400);renderer 的 logical size 負責等比縮放 */
	win_w = KB_LOGICAL_W * 2;
	win_h = KB_LOGICAL_H * 2;
	winflags = SDL_WINDOW_RESIZABLE;
	if (conf->fullscreen) winflags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

	g_window = SDL_CreateWindow("openkb " PACKAGE_VERSION,
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		win_w, win_h, winflags);
	if (g_window == NULL) {
		KB_errlog("Couldn't create window: %s\n", SDL_GetError());
		free(nsys);
		return NULL;
	}

	g_renderer = SDL_CreateRenderer(g_window, -1,
		SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (g_renderer == NULL) /* 退回軟體 renderer */
		g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_SOFTWARE);
	if (g_renderer == NULL) {
		KB_errlog("Couldn't create renderer: %s\n", SDL_GetError());
		free(nsys);
		return NULL;
	}

	/* 等比縮放 + 黑邊 letterbox (取代舊的 fullscreen pan hack) */
	SDL_RenderSetLogicalSize(g_renderer, KB_LOGICAL_W, KB_LOGICAL_H);

	/* 串流 texture:每 frame 由 nsys->screen (軟體 surface) 上傳 */
	g_texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING, KB_LOGICAL_W, KB_LOGICAL_H);
	if (g_texture == NULL) {
		KB_errlog("Couldn't create texture: %s\n", SDL_GetError());
		free(nsys);
		return NULL;
	}

	/* 遊戲全程繪製目標:32-bit ARGB8888 軟體 surface,與 texture 同格式 */
	nsys->screen = SDL_CreateRGBSurface(0, KB_LOGICAL_W, KB_LOGICAL_H, 32,
		0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	if (nsys->screen == NULL) {
		KB_errlog("Couldn't create screen surface: %s\n", SDL_GetError());
		free(nsys);
		return NULL;
	}
	KB_screen = nsys->screen; /* SDL_FillRect hook 據此清 CJK 區域 */

	/* Set window icon */
	iconfile = KB_fastpath(conf->install_dir, "/", "icon_32x32"
#ifdef HAVE_LIBSDL_IMAGE
	".png"
#else
	".bmp"
#endif
	); 
	nsys->icon = 
#ifdef HAVE_LIBSDL_IMAGE
	IMG_Load(iconfile);
#else
	SDL_LoadBMP(iconfile);
#endif
	if (!nsys->icon) {
		KB_errlog("Couldn't open icon file: %s\n", iconfile);
	} else {
		SDL_SetWindowIcon(g_window, nsys->icon);
	}
	free(iconfile);

	if (conf->sound) {
		/* Open audio device */
		desired.format = AUDIO_FORMAT;
		desired.freq = 11025;	/* Common values are 11025, 22050 and 44100 hz. We don't need much (yet)! */
		desired.channels = 2;  	// TODO: allow user selection
		desired.samples = 512;	/* Large audio buffer reduces risk of dropouts but increases response time */

		desired.callback = KBenv_audio_callback;
		desired.userdata = nsys;

		/* 傳 NULL 為 obtained:強制 device 採用 desired 格式 (S16/11025/2ch),
		 * 由 SDL 內部轉換到實際硬體。否則 SDL2 在現代 Linux (PulseAudio/PipeWire)
		 * 常回傳 AUDIO_F32,而 tune synth 產生的是 S16 → 被當 float 解讀成噪音。
		 * mixer 直接採用 desired,讓 synth 以正確 freq/format/channels 取樣。 */
		if (SDL_OpenAudio(&desired, NULL) < 0) {
			KB_errlog("Couldn't open audio device: %s\n", SDL_GetError());

			conf->sound = 0; /* Turn sound off */
		} else {
			nsys->mixer = desired;
			KB_stdlog("Opened audio device: %d channels, %d frequency, format 0x%04x\n",
				nsys->mixer.channels, nsys->mixer.freq, nsys->mixer.format);

			SDL_PauseAudio(0); /* Start playing */

			KB_SetAudioSpec(&nsys->mixer); /* Store for later */
		}
	}

	nsys->sound = NULL;


    nsys->conf = conf;

	nsys->font = NULL;
	
	RESOURCE_DefaultConfig(conf);

	prepare_inline_font();	// <-- inline font
	nsys->font_size.w = 8;
	nsys->font_size.h = 8;
	KB_setfont(nsys, get_inline_font());

	nsys->text_rgb = 0xFFFFFF; /* 預設前景白 (CJK 字色) */

	/* 載入 CJK atlas (cjk24.bin):先試 install_dir,再試 data_dir;找不到不致命 */
	{
		char *cjkpath = KB_fastpath(conf->install_dir, "/", "cjk24.bin");
		if (!cjkfont_init(cjkpath)) {
			free(cjkpath);
			cjkpath = KB_fastpath(conf->data_dir, "/", "cjk24.bin");
			if (!cjkfont_init(cjkpath))
				KB_stdlog("CJK: no cjk24.bin (英文模式);中文需先 build-font\n");
		}
		free(cjkpath);
	}

	return nsys;
}

void KB_stopENV(KBenv *env) {

	SDL_CloseAudio();

	if (env->font) SDL_FreeSurface(env->font);
	if (env->icon) SDL_FreeSurface(env->icon);

	SDL_FreeCachedSurfaces();

	cjkfont_shutdown();
	cjk_drawlist_clear();
	{
		int k;
		for (k = 0; k < CJK_CACHE_N; k++)
			if (cjkv.cache[k].tex) { SDL_DestroyTexture(cjkv.cache[k].tex); cjkv.cache[k].tex = NULL; }
	}
	if (cjkv.composite) { SDL_DestroyTexture(cjkv.composite); cjkv.composite = NULL; }

	if (env->screen) SDL_FreeSurface(env->screen);
	if (g_texture)  { SDL_DestroyTexture(g_texture);   g_texture = NULL; }
	if (g_renderer) { SDL_DestroyRenderer(g_renderer); g_renderer = NULL; }
	if (g_window)   { SDL_DestroyWindow(g_window);     g_window = NULL; }

	free(env);

	SDL_Quit();
}

/* 把目前畫面 (screen surface + 持續的 CJK 清單) 呈現到視窗。
 * 底圖上傳 texture → 依 logical size 縮放至視窗 (nearest 銳利);CJK glyph 直接
 * 在 logical 座標疊畫。抽成獨立函式,讓視窗 resize/expose 時可重新呈現。 */
void KB_present(KBenv *env) {
	SDL_UpdateTexture(g_texture, NULL, env->screen->pixels, env->screen->pitch);
	SDL_RenderClear(g_renderer);
	SDL_RenderCopy(g_renderer, g_texture, NULL, NULL); /* 320x200 → 視窗 */
	if (cjk_drawlist_count() > 0) cjk_draw_overlay();  /* 在其上疊中文 */
	SDL_RenderPresent(g_renderer);
}

void KB_flip(KBenv *env) {
	KB_present(env);
	/* 本 frame 結束:CJK 清單保留 (供 resize 重呈現與動畫畫面持續顯示),
	 * 下一個被印出的中文會自動清舊換新。 */
	cjk_drawlist_flip();
}


void KB_loc(KBenv *env, word base_x, word base_y) {
	env->base_x = base_x;
	env->base_y = base_y;
	env->cursor_x = 0;
	env->cursor_y = 0;
	env->line_height = env->font_size.h;
}

void KB_curs(KBenv *env, word cursor_x, word cursor_y) {
	env->cursor_x = cursor_x;
	env->cursor_y = cursor_y;
}

void KB_getpos(KBenv *env, word *x, word *y) {
	*x = env->base_x + env->cursor_x * env->font_size.w;
	*y = env->base_y + env->cursor_y * env->line_height;
}

void KB_lh(KBenv *env, byte h) {
	env->line_height = (h == 0 ? env->font_size.h : h);
}

void KB_setfont(KBenv *env, SDL_Surface *surf) {
	infont(surf);
	env->font = surf;
	env->font_size.w = surf->w / 16;
	env->font_size.h = surf->h / 8;
	env->line_height = env->font_size.h;
}

void KB_setcolor(KBenv *env, Uint32* colors) /* Colors must be in 0x00RRGGBB format ! */
{
	SDL_Color pal[4];
	int i;
	for (i = 0; i < 4; i++) {
		pal[i].r = (Uint8)((colors[i] & 0x00FF0000) >> 16); 
		pal[i].g = (Uint8)((colors[i] & 0x0000FF00) >> 8);
		pal[i].b = (Uint8)((colors[i] & 0x000000FF));
	}
	SDL_SetColors(env->font, pal, 0, 4);

	cjk_text_fg = colors[0] & 0x00FFFFFF;
	cjk_text_bg = colors[1] & 0x00FFFFFF;
}

/* 解 3-byte UTF-8 (0xE0-0xEF 前導,涵蓋 U+0800..U+FFFF,含 CJK)。
 * 回傳碼點;非 3-byte 序列回 0。 */
static uint32_t kb_utf8_decode3(const unsigned char *s) {
	if (s[0] < 0xE0 || s[0] > 0xEF) return 0;
	if ((s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80) return 0;
	return ((uint32_t)(s[0] & 0x0F) << 12)
	     | ((uint32_t)(s[1] & 0x3F) << 6)
	     |  (uint32_t)(s[2] & 0x3F);
}

void KB_print(KBenv *env, const char *str) {
	SDL_Rect dest = { 0 }, letter = { 0 };
	int i, len;

	dest.w = letter.w = env->font_size.w;
	dest.h = letter.h = env->font_size.h;

	len = strlen(str);
	for (i = 0; i < len; i++) {
		int id = (unsigned char)str[i];
		int row, col;
		uint32_t cp = 0;
		int nbytes = 1;

		if (str[i] == '\n') {
			env->cursor_x = 0;
			env->cursor_y++;
			continue;
		}

		/* 解碼點:可列印 ASCII 或 3-byte UTF-8 CJK */
		if (id >= 0x20 && id < 0x7F) { cp = id; nbytes = 1; }
		else if (id >= 0xE0 && id <= 0xEF && i + 2 < len) {
			cp = kb_utf8_decode3((const unsigned char *)&str[i]); nbytes = 3;
		}

		/* 在 atlas 內 (ASCII 或 CJK) → 走 Noto 合成層 (風格一致);
		 * 否則走點陣 (box-drawing/控制字元)。皆前進 1 格。 */
		if (cp && cjkfont_has(cp)) {
			word px, py;
			KB_getpos(env, &px, &py);
			cjk_drawlist_add(px, py, cp, cjk_text_fg, cjk_text_bg,
				env->font_size.w, env->font_size.h);
			env->cursor_x += 1;
			i += (nbytes - 1); /* 迴圈尾 i++ 吃最後一個位元組 */
			continue;
		}

		row = id / 16;
		col = id - (row * 16);
		letter.x = col * letter.w;
		letter.y = row * letter.h;
		KB_getpos(env, &dest.x, &dest.y);
		cjk_drawlist_remove(dest.x, dest.y); /* 此格改畫點陣字 → 清掉舊中文 */
		SDL_BlitSurface(env->font, &letter, env->screen, &dest);
		env->cursor_x++;
	}
}


void KB_printf(KBenv *env, const char *fmt, ...) { 
	char buf[1024];
	va_list argptr;
	va_start(argptr, fmt);
	vsnprintf(buf, 1024, fmt, argptr);
	KB_print(env, buf);
	va_end(argptr);
}

void KB_vprintf(KBenv *env, const char *fmt, va_list argptr) { 
	char buf[1024];
	vsnprintf(buf, 1024, fmt, argptr);
	KB_print(env, buf);
}

/*
 * SDL Operations.
 * Any SDL resource handler should have those or similar! :)
 * Warning: might not work for non-paletted surfaces, use with caution.
 */
#define SDL_CloneSurfaceX(SURFACE, SIZE) SDL_CreateRGBSurface(SURFACE->flags, SURFACE->w * SIZE, SURFACE->h * SIZE, SURFACE->format->BitsPerPixel, \
		SURFACE->format->Rmask, SURFACE->format->Gmask,	SURFACE->format->Bmask,	SURFACE->format->Amask)
#define SDL_CloneSurfaceHW(SURFACE, H, W) SDL_CreateRGBSurface(SURFACE->flags, W, H, SURFACE->format->BitsPerPixel, \
		SURFACE->format->Rmask, SURFACE->format->Gmask,	SURFACE->format->Bmask,	SURFACE->format->Amask)

void SDL_SizeX(SDL_Surface *surface, SDL_Surface *new_surface, Uint8 size) {
	SDL_Rect sr = { 0, 0, surface->w, surface->h };
	SDL_Rect dr = { 0, 0, sr.w * size, sr.h * size };
	SDL_SoftStretch(surface,
                    &sr,
                    new_surface,
                    &dr);
	return;
}

void SDL_BlitSurfaceFLIP(SDL_Surface *surface, SDL_Rect *src, SDL_Surface *new_surface, SDL_Rect *dst) {
	Uint16 sx, sy, dx, dy;
	Uint8 bpp = surface->format->BytesPerPixel, p;
	Uint8 *source, *dest;

	source = (Uint8*)(surface->pixels);
	dest = (Uint8*)(new_surface->pixels);

	for(sy = src->y, dy = dst->y; sy < src->y + src->h; sy++, dy++)
	for(sx = src->x, dx = dst->x + dst->w - 1; sx < src->x + src->w; sx++, dx--)
		for (p = 0; p < bpp; p++)	
		{
			dest[ dy * (new_surface->w * bpp) + (dx * bpp) + p ] = 
			source[ sy * (surface->w * bpp) + (sx * bpp) + p ];
		}
}

/* 
 * Prepare matching surface and perform AdvancedMAME's scale2x implementation
 */
SDL_Surface* SDL_Scale2X_Surface(SDL_Surface *surface) 
{
	SDL_Surface* new_surface = NULL;

	new_surface = SDL_CloneSurfaceX(surface, 2);

	scale2x(surface, new_surface);

	return new_surface;
}

SDL_Surface* SDL_SizeX_Surface(SDL_Surface* surface, Uint8 size)
{
	SDL_Surface* new_surface = NULL;

	new_surface = SDL_CloneSurfaceX(surface, size);

	SDL_SizeX(surface, new_surface, size);

	return new_surface;
}

SDL_Surface *SDL_Flipped_Surface(SDL_Surface *surface) 
{
	SDL_Surface* new_surface = NULL;
	SDL_Rect src = { 0 };
	SDL_Rect dst = { 0 };

	new_surface = SDL_CloneSurfaceX(surface, 1);

	src.w = dst.w = new_surface->w;
	src.h = dst.h = new_surface->h;	

	SDL_BlitSurfaceFLIP(surface, &src, new_surface, &dst);

	return new_surface;
}


void tell_surface(SDL_Surface *surf) {

	printf("Surface <%p>\n", surf);
	printf("W: %d, H: %d, BPP: %d\n", surf->w, surf->h, surf->format->BitsPerPixel);
	int i = 0;
	for (i = 0; i < 16; i++) {
	SDL_Color *col = &surf->format->palette->colors[i];
	printf("COLOR 0: %02x %02x %02x\n", col->r, col->g, col->b);
	}
}

void SDL_SBlitTile(SDL_Surface *src, int id, SDL_Surface *dst, SDL_Rect *dstrect) {

}

void SDL_BlitTile(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect) {

}

void SDL_SBlitSurface(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect) {

	int zoom = (conf->filter ? 2 : 1);

	SDL_Rect nsrcrect;
	nsrcrect.x = srcrect->x * zoom;
	nsrcrect.y = srcrect->y * zoom;
	nsrcrect.w = srcrect->w * zoom;
	nsrcrect.h = srcrect->h * zoom;

	SDL_Rect ndstrect;
	ndstrect.x = dstrect->x * zoom;
	ndstrect.y = dstrect->y * zoom;
	ndstrect.w = dstrect->w * zoom;
	ndstrect.h = dstrect->h * zoom;

	SDL_BlitSurface(src, &nsrcrect, dst, &ndstrect);
}


/* 
 * Set the the default config.
 * Must be set before any calls to *RESOURCE functions.
 * This is done to avoid passing the same config over and over
 * again in tight loops.
 */ 
void RESOURCE_DefaultConfig(KBconfig* _conf) { conf = _conf; }

/*
 * Load a graphical KB resource using config 'conf' (must be provided
 *  via the 'RESOURCE_DefaultConfig' function beforehand)
 *
 * Take conf->filter into account and zoom accordingly.
 *
 * If 'flip' is set, make the resulting surface twice as large, and
 *  blit a horizontally-flipped copy into resulting space.
 * If 'flip' is 1, do that horizontally, vertically if 2.
 */
SDL_Surface *SDL_LoadRESOURCE(int id, int sub_id, int flip) {

	int zoom = (conf->filter ? 2 : 1);

	enum {
		PASS,
	} strategy;

	Uint32 w, h;

	word flip_range = 48;

	SDL_Surface *ret = NULL;

	SDL_Surface *surf = KB_LoadIMG8(id, sub_id);

	if (!surf) return NULL;

	w = surf->w * zoom;
	h = surf->h * zoom;

	if (flip == 2) w *= 2;
	if (flip == 1) h *= 2;

	ret = surf;

	if (w != surf->w || h != surf->h) {

		SDL_Surface *bigsurf = SDL_CloneSurfaceHW(surf, h, w);
		if (surf->format->palette) SDL_ClonePalette(bigsurf, surf);
		else {
			KB_stdlog("Warning: A %d-bpp image\n", surf->format->BitsPerPixel);
		}

		if (SDL_HasColorKey(surf)) { /* Use same colorkey */
			SDL_SetColorKey(bigsurf, SDL_SRCCOLORKEY, 0xFF);
		}

		if (zoom > 1) { /* Zoom-copy */

			if (conf->filter == 1)	SDL_SizeX(surf, bigsurf, 2);
			if (conf->filter == 2)	scale2x(surf, bigsurf);

		} else { /* Flipping only, just copy over */

			SDL_SetColorKey(surf, 0, 0);
			SDL_FillRect(bigsurf, NULL, 0xFF00000);
			SDL_BlitSurface(surf, NULL, bigsurf, NULL); 

		}

		if (flip == 1) {
			SDL_Rect base_rect = { 0, 0 };
			SDL_Rect flip_rect = { 0, 0 };

			flip_rect.w = base_rect.w = surf->w * zoom;
			flip_rect.h = base_rect.h = surf->h * zoom;

			if (flip == 2) flip_rect.x += flip_rect.w;
			if (flip == 1) flip_rect.y += flip_rect.h;
			
			int i;
			flip_range *= zoom;
			SDL_Rect mbase;
			SDL_Rect mflip;
			mbase.h = mflip.h = base_rect.h;
			mbase.w = mflip.w = flip_range;
			mbase.y = base_rect.y;
			mbase.x = base_rect.x;// + base_rect.w - flip_range;
			mflip.y = flip_rect.y;
			mflip.x = flip_rect.x;
			for (i = 0; i < flip_rect.w/flip_range; i++) {
				SDL_BlitSurfaceFLIP(bigsurf, &mbase, bigsurf, &mflip);
				mbase.x += flip_range;
				mflip.x += flip_range;				
			}

			//SDL_BlitSurfaceFLIP(bigsurf, &base_rect, bigsurf, &flip_rect); 			
		} 	


		ret = bigsurf;
		SDL_FreeSurface(surf);
	}

	return ret;
}

/*
 * Same as SDL_LoadRESOURCE, but with a cache layer. 
 */
SDL_Surface *surf_cache[64][64] = { NULL };
SDL_Surface *SDL_TakeSurface(int id, int sub_id, int flip) {
	SDL_Surface *ret = NULL;
	if (id < 64 && sub_id < 64) {
		if (surf_cache[id][sub_id] != NULL)
		{ 
			return surf_cache[id][sub_id];
		} 
	}
	ret = SDL_LoadRESOURCE(id, sub_id, flip);
	if (id < 64 && sub_id < 64) {
		surf_cache[id][sub_id] = ret;
	}
	return ret;
}
void SDL_ReleaseSurface(int id, int sub_id) {
	if (id < 64 && sub_id < 64) {
		if (surf_cache[id][sub_id] != NULL)
		{ 
			SDL_FreeSurface( surf_cache[id][sub_id] );
			surf_cache[id][sub_id] = NULL;
		} 
	}
}
void SDL_FreeCachedSurfaces() {
	int id, sub_id;
	for (id = 0; id < 64; id++)
	for (sub_id = 0; sub_id < 64; sub_id++) {
		if (surf_cache[id][sub_id] != NULL) 
			SDL_FreeSurface(surf_cache[id][sub_id]);
		surf_cache[id][sub_id] = NULL; 
	}
}
/* Convert strlist into an array of strings (accessible by index). Expensive. */
char **STRL_LoadArray(int id, int sub_id) {

	char **arr;
	char *list;
	int i, max;

	list = (char*)KB_Resolve(id, sub_id);
	if (list == NULL) return NULL;

	max = KB_strlist_max(list);
	arr = malloc(sizeof(char*) * (max+1));
	if (arr == NULL) return NULL;

	for (i = 0; i < max; i++) {

		char *item = KB_strlist_ind(list, i);
		int len = strlen(item);

		arr[i] = malloc(sizeof(char) * len);
		if (arr[i] == NULL) { /* Out of memory */
			STRL_FreeArray(arr);
			free(list);
			return NULL;
		}
		memcpy(arr[i], item, len);
		arr[i][len] = '\0';
	}
	arr[i] = NULL;

	free(list);
	return arr;
}
void STRL_FreeArray(char **arr) {
	int i;
	for (i = 0; arr[i]; i++) {
		free(arr[i]);
	}
	free(arr);
}
#if 0
/* Same as TakeSurface/FreeCachedSurfaces but for strings */
char **strl_cache[64][64] = { NULL };
char **STRL_TakeArray(int id, int sub_id) {
	char **ret = NULL;
	int cid = FIRST_STRL - id;
	if (cid < 64 && sub_id < 64) {
		if (strl_cache[cid][sub_id] != NULL)
		{ 
			return strl_cache[cid][sub_id];
		} 
	}
	ret = STRL_LoadArray(id, sub_id);
	if (cid < 64 && sub_id < 64) {
		strl_cache[cid][sub_id] = ret;
	}
	return ret;
}
void STRL_FreeCachedArrays() {
	int id, sub_id;
	for (id = 0; id < 64; id++)
	for (sub_id = 0; sub_id < 64; sub_id++) {
		if (strl_cache[id][sub_id] != NULL) 
			STRL_FreeArray(strl_cache[id][sub_id]);
		strl_cache[id][sub_id] = NULL; 
	}
}
#endif
#if 1
char *STRL_LoadRESOURCE(int id, int sub_id) {
	return (char*)KB_Resolve(id, sub_id);
}

char *STR_LoadRESOURCE(int id, int sub_id, int line) {
	char *list = KB_Resolve(id, sub_id);
	char *match = KB_strlist_peek(list, line);
	int len = strlen(match) + 1;
	char *item = malloc(sizeof(char) * len);
	item[0] = '\0';
	KB_strncpy(item, match, len);
	/* FREE! */
	free(list);
	return item;
}
#endif

SDL_Rect* RECT_LoadRESOURCE(int id, int sub_id) {
	int zoom = (conf->filter ? 2 : 1);
	SDL_Rect *src = (SDL_Rect *)KB_Resolve(id, sub_id);
	SDL_Rect *dst = malloc(sizeof(SDL_Rect));
	dst->x = src->x*zoom;//probably cheaper then memcpy 
	dst->y = src->y*zoom;
	dst->w = src->w*zoom;
	dst->h = src->h*zoom;
	return dst;
}

SDL_Surface* KB_LoadIMG8(int id, int sub_id) {
	SDL_Surface *surf = (SDL_Surface *)KB_Resolve(id, sub_id);
	return surf;
}

void* GNU_Resolve(KBmodule *mod, int id, int sub_id);
void* DOS_Resolve(KBmodule *mod, int id, int sub_id);
void* MD_Resolve(KBmodule *mod, int id, int sub_id);

void* KB_Resolve(int id, int sub_id) {
	/* By contract, things resolved by this function MUST be freed.
	 * But, alas, sometimes this function returns things that CAN NOT be freed,
	 * in particular, strings obtained via KB_strlist_peek().
	 *
	 * TODO: remove KB_strlist_peek calls from resolvers, either rethink this contract.
	 */
	int i, pass;
	void *ret = NULL;
	/* 型別感知解析 (混合模式):圖形 (GR_*) 優先用 DOS 原版美術,其餘 (字串/資料/
	 * 音效) 優先用 GNU free (我們的中文翻譯)。每類先試偏好家族,再 fallback 另一個。
	 * 如此:玩家自備原版資料 → 看到原版美術 + 中文;只有 free → 全 free + 中文。 */
	int is_graphic = (id >= GR_TROOP && id <= GR_ENDTILES);
	int pref = is_graphic ? KBFAMILY_DOS : KBFAMILY_GNU;

	for (pass = 0; pass < 2 && ret == NULL; pass++) {
		for (i = 0; i < conf->num_modules; i++) {
			KBmodule *mod = &conf->modules[i];
			if (pass == 0 && mod->kb_family != pref) continue; /* 先試偏好家族 */
			if (pass == 1 && mod->kb_family == pref) continue; /* 再試其餘 */
			switch (mod->kb_family) {
				case KBFAMILY_GNU: ret = GNU_Resolve(mod, id, sub_id); break;
				case KBFAMILY_DOS: ret = DOS_Resolve(mod, id, sub_id); break;
				case KBFAMILY_MD:  ret = MD_Resolve(mod, id, sub_id); break;
				default: break;
			}
			if (ret != NULL) break;
		}
	}
	if (ret == NULL)
		KB_errlog("Unable to resolve resource %s::%d (from %d modules)\n", KBresid_names[id], sub_id, conf->num_modules);
	return ret;
}

/*
 * SDL_SOUND internal mixer.
 */
void KB_play(KBenv *sys, KBsound *snd) {

	int n;

	if (snd == NULL) {
		KB_debuglog(0, "(Non)Playing an empty sound\n");
		return;
	}

	sys->sound = NULL;

	switch (snd->type) {	//TODO: make this a callback, for speed
		case KBSND_DOS:

			n = tunFile_reset(snd->data, sys->mixer.format);

		break;
		case KBSND_WAV:

			n = wavFile_reset(snd->data, sys->mixer.format);

		break;
		default:
		break;
	}

	sys->sound = snd;
}
void KBenv_audio_callback(void *userdata, Uint8 *stream, int len) {

	KBenv *sys = (KBenv *) userdata;

	/* We need to figure out and write "len" samples,
	 * so this condition for outer loop is a reasonable assumption. */
	while (len) {

		if (sys->sound == NULL) { /* 無音效 → 填靜音 (S16=0) 後結束。
			* 原本只 break 不填,SDL 會播放緩衝區殘留 → 持續嘟嘟噪音 (城堡內等)。 */
			SDL_memset(stream, 0, len);
			break;
		}

		int n = 0;

		KBsound *snd = sys->sound;

		switch (snd->type) {	//TODO: make this a callback, for speed
			case KBSND_DOS:

				n = tunFile_play(snd->data, stream, len, sys->mixer.freq * sys->mixer.channels);

			break;
			case KBSND_WAV:

				n = wavFile_play(snd->data, stream, len, sys->mixer.freq * sys->mixer.channels);

			break;
			default:
			break;
		}

		if (n == 0) {	/* Sample ended */
			sys->sound = NULL;
		}

		if (n < 0) {
			KB_errlog("Audio buffer underrun: need %d more bytes of samples\n", len);
			break;
		}
		
		len -= n;
	}

}
