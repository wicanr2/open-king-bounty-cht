#include "../sdlcompat.h"

#include "kbfile.h"
#include "kbres.h"
#include "kbconf.h"
#include "kbauto.h"
#include "kbstd.h"

#define VILLAIN_OFFSET 0x04B04C
#define CURSOR_OFFSET (0x062D6C - (16 * 8))
#define TROOP_OFFSET 0x000668EC
#define MAP_OFFSET 0x1AA8E /* -64 // ? */
#define MD_PAL_OFFSET 0x025698 /* Genesis CRAM sprite palette (掃 ROM 找到,農民/villain 對色) */

/* Genesis 色字 = 0000 BBB0 GGG0 RRR0 (9-bit, 每通道 3-bit)。轉 RGB888 (×36)。
 * rompal 指向 ROM 內 32 bytes (16 色 big-endian word)。*/
static void md_build_palette(const byte *rompal, SDL_Color pal[16]) {
	int i;
	for (i = 0; i < 16; i++) {
		word w = (rompal[i*2] << 8) | rompal[i*2 + 1];
		pal[i].r = (Uint8)(((w >> 1) & 7) * 36);
		pal[i].g = (Uint8)(((w >> 5) & 7) * 36);
		pal[i].b = (Uint8)(((w >> 9) & 7) * 36);
	}
}

/* A tile is composed of several sub-tiles */
/* Each subtile is 8 x 8 */
#define SUBTILE_W	8
#define SUBTILE_H	8
/* Lay them in groups of 6 x 4 */
#define SUBGROUP_W	6
#define SUBGROUP_H	4
/* And get a tile (48 x 32) */
#define TILE_W	(SUBTILE_W * SUBGROUP_W)
#define TILE_H	(SUBTILE_H * SUBGROUP_H) 
/* In bytes, that takes */
#define SUBTILE_LEN ((SUBTILE_W * SUBTILE_H) / 2)
#define TILE_LEN (SUBTILE_LEN * (SUBGROUP_W * SUBGROUP_H))

SDL_Surface *MD_LoadIMGROW_BUF(const char *buf, int frames, const SDL_Color *pal) {

	SDL_Rect dstrect = { 0, 0, SUBTILE_W, SUBTILE_H };

	int col, row, frame;
	int pos = 0;

	SDL_Surface *surf = SDL_CreatePALSurface(TILE_W * frames, TILE_H);
	if (surf == NULL) return NULL;

	for (frame = 0; frame < frames; frame++)
	for (col = 0; col < SUBGROUP_W; col++)
	for (row = 0; row < SUBGROUP_H; row++) {

		dstrect.x = col * SUBTILE_W + frame * TILE_W;
		dstrect.y = row * SUBTILE_H;

		SDL_BlitXBPP(&buf[pos], surf, &dstrect, 4);

		pos += SUBTILE_LEN;
	}

	/* 套 Genesis 真調色盤 (取代 EGA → 顏色正確) */
	SDL_SetColors(surf, (SDL_Color *)pal, 0, 16);

	SDL_Rect f = { 0, 0, surf->w, surf->h };
	SDL_ReplaceIndex(surf, &f, 0, 0xFF);
	SDL_SetColorKey(surf, SDL_SRCCOLORKEY, 0xFF);

	return surf;
}

void* MD_Resolve(KBmodule *mod, int id, int sub_id) {

	enum {

		UNKNOWN,
		ROW,	
	
	} method = UNKNOWN;

	int frames = 0;
	int offset = 0;

	switch (id) {
		case GR_LOGO:
		{
		}
		break;
		case GR_TROOP:	/* A troop */
		{
			method = ROW;
			offset = TROOP_OFFSET;
			frames = 4;
			offset += (TILE_LEN * frames) * sub_id;
		}
		break;
		case GR_VILLAIN:	/* Villain animated face */
		{
			method = ROW;
			offset = VILLAIN_OFFSET;
			frames = 4;
			offset += (TILE_LEN * frames) * sub_id;
		}
		break;
		case DAT_WORLD:	/* World data for all 4 continents, sub_id - undefined */
		{
			KB_File *f;
			int n;
			byte *world;
			int len = 64 * 64 * 4;

			world = malloc(sizeof(byte) * len);
			if (!world) return NULL;

			f = KB_fopen(mod->slotA_name, "rb");
			if (f == NULL) return NULL;

			KB_fseek(f, MAP_OFFSET, 0);
			n = KB_fread(world, sizeof(byte), len, f);
			if (n != len) return NULL;

			KB_fclose(f);
			return world;
		}
		break;
		default: break;
	}

	switch (method) {
		case ROW:
		{
			char fullname[PATH_LEN];
			int span_len = TILE_LEN * frames;

			KB_File *f = KB_fopen(mod->slotA_name, "rb");

			/* 讀 Genesis 調色盤 (32 bytes = 16 色) */
			byte rompal[32];
			SDL_Color pal[16];
			KB_fseek(f, MD_PAL_OFFSET, 0);
			KB_fread(rompal, sizeof(byte), 32, f);
			md_build_palette(rompal, pal);

			KB_fseek(f, offset, 0);

			char buf[span_len];
			int n = KB_fread(buf, sizeof(char), span_len, f);

			KB_fclose(f);

			SDL_Surface *surf = MD_LoadIMGROW_BUF(&buf[0], frames, pal);
			return surf;
		}
		break;
		case UNKNOWN:
		default:
		break;
	}

	return NULL;
}
