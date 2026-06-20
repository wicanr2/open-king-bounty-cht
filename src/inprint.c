/*
 *  inprint.c -- openkb 內嵌點陣字渲染 (原 vendor/SDL_inprint + CJK 支援)
 *
 *  openkb 幾乎所有 UI 文字都經此繪製。原版逐字從 8x8 XBM inline font blit;
 *  本版在遇到 atlas 內的 3-byte UTF-8 CJK 碼點時,改記進 CJK draw-list,
 *  由 env-sdl 的合成層 (640x400) 以 24x24 + 黑外框畫出 (見 cjkfont.* / ADR 0001)。
 *  CJK 佔 2 個 ASCII 格寬。
 */
#include "sdlcompat.h"
#include "font.h"      /* inline_font_bits */
#include "cjkfont.h"

#define CHARACTERS_PER_ROW    16
#define CHARACTERS_PER_COLUMN 8

static SDL_Surface *inline_font = NULL;
static SDL_Surface *selected_font = NULL;
static Uint32 inprint_fg = 0xFFFFFF; /* 目前前景色,供 CJK glyph 上色 */

void prepare_inline_font(void)
{
	Uint8 *pix_ptr, tmp;
	int i, len, j;

	if (inline_font != NULL) { selected_font = inline_font; return; }

	inline_font = SDL_CreateRGBSurface(0, inline_font_width, inline_font_height, 8, 0, 0, 0, 0);

	pix_ptr = (Uint8 *)inline_font->pixels;
	len = inline_font->h * inline_font->w / 8;

	for (i = 0; i < len; i++)
	{
		tmp = (Uint8)inline_font_bits[i];
		for (j = 0; j < 8; j++)
		{
			Uint8 mask = (0x01 << j);
			pix_ptr[i * 8 + j] = (tmp & mask) >> j;
		}
	}

	selected_font = inline_font;
}
void kill_inline_font(void) { SDL_FreeSurface(inline_font); inline_font = NULL; }
void infont(SDL_Surface *font)
{
	selected_font = font;
	if (font == NULL) prepare_inline_font();
}
void incolor(Uint32 fore, Uint32 back) /* Colors must be in 0x00RRGGBB format ! */
{
	SDL_Color pal[2];
	pal[0].r = (Uint8)((back & 0x00FF0000) >> 16);
	pal[0].g = (Uint8)((back & 0x0000FF00) >> 8);
	pal[0].b = (Uint8)((back & 0x000000FF));
	pal[1].r = (Uint8)((fore & 0x00FF0000) >> 16);
	pal[1].g = (Uint8)((fore & 0x0000FF00) >> 8);
	pal[1].b = (Uint8)((fore & 0x000000FF));
	SDL_SetColors(selected_font, pal, 0, 2);
	inprint_fg = fore & 0x00FFFFFF;
}

/* 解 3-byte UTF-8 (0xE0-0xEF 前導);非該序列回 0。 */
static Uint32 inprint_utf8_3(const unsigned char *s)
{
	if (s[0] < 0xE0 || s[0] > 0xEF) return 0;
	if ((s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80) return 0;
	return ((Uint32)(s[0] & 0x0F) << 12) | ((Uint32)(s[1] & 0x3F) << 6) | (Uint32)(s[2] & 0x3F);
}

void inprint(SDL_Surface *dst, const char *str, Uint32 x, Uint32 y)
{
	SDL_Rect s_rect;
	SDL_Rect d_rect;

	d_rect.x = x;
	d_rect.y = y;
	s_rect.w = selected_font->w / CHARACTERS_PER_ROW;
	s_rect.h = selected_font->h / CHARACTERS_PER_COLUMN;
	d_rect.w = s_rect.w;
	d_rect.h = s_rect.h;

	while (*str)
	{
		int id = (unsigned char)*str;

		if (id == '\n')
		{
			d_rect.x = x;
			d_rect.y += s_rect.h;
			str++;
			continue;
		}

		/* CJK:3-byte UTF-8 且在 atlas 內 → 記進 draw-list (合成層畫),佔 2 格寬 */
		if (id >= 0xE0 && id <= 0xEF && str[1] && str[2])
		{
			Uint32 cp = inprint_utf8_3((const unsigned char *)str);
			if (cp && cjkfont_has(cp))
			{
				cjk_drawlist_add(d_rect.x, d_rect.y, cp, inprint_fg, (Uint8)s_rect.h);
				d_rect.x += s_rect.w * 2;
				str += 3;
				continue;
			}
		}

		{
			int row = id / CHARACTERS_PER_ROW;
			int col = id % CHARACTERS_PER_ROW;
			s_rect.x = col * s_rect.w;
			s_rect.y = row * s_rect.h;
		}
		SDL_BlitSurface(selected_font, &s_rect, dst, &d_rect);
		d_rect.x += s_rect.w;
		str++;
	}
}
SDL_Surface *get_inline_font(void) { return selected_font; }
