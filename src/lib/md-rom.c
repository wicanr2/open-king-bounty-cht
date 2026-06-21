#include "../sdlcompat.h"

#include <string.h> /* memset */

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

/* === 世界地圖地形 (反組譯成果) ===
 * 地形 tile 圖形是「自製 Okumura LZSS」壓縮的 Genesis 4bpp tile,解壓器在 ROM 0x18B0C。
 *  - terrain pattern  : LZSS 塊 @ 0x30E82 → 解出 638 個 8x8 tile (skip 2-byte header)
 *  - cell template    : 0x19666,每 cell-type 60 bytes = 6x5=30 個 VDP nametable word
 *                       (bit0-10 tile index、bit11 hflip、bit13-14 palette line)
 *  - map cell data    : 0x1AA8E (見 MAP_OFFSET),cell-type 經引擎 &0x7F → 0..71
 *  - terrain palette  : 0x256B8 起,4 條 line x 16 色
 * 一個 cell = 6x5 個 8x8 tile = 48x40 px 的 metatile。GR_TILE sub_id = cell-type。*/
#define MD_TERRAIN_LZSS 0x30E82  /* LZSS 壓縮的地形 tile pattern */
#define MD_CELLTPL_OFFSET 0x19666 /* cell-type → 6x5 nametable word 模板表 */
#define MD_TERRAIN_PAL 0x0256B8  /* 地形 palette (4 line x 16 色) */
#define MD_CELL_COLS 6           /* 一個 cell 寬幾個 8x8 tile */
#define MD_CELL_ROWS 5           /* 一個 cell 高幾個 8x8 tile */
#define MD_CELL_W (MD_CELL_COLS * 8) /* 48 */
#define MD_CELL_H (MD_CELL_ROWS * 8) /* 40 (Genesis metatile 原高) */
#define MD_TILE_H 34                 /* 引擎 RECT_TILE 高 (free ui.ini,全主題共用,F8 不重載) → 降採樣對齊 */
#define MD_NUM_CELLS 72          /* 引擎 tileset 模型用 72 個 tile (map &0x7F) */

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

/* === Okumura LZSS 解壓 (對應 ROM 0x18B0C) ===
 * ring buffer 4096 bytes、初值填 0x20、起始寫入位 0xFEE;
 * src header 8 bytes,out_size = header offset+4 的 big-endian long,壓縮流從 src+8 起;
 * flag byte 由 LSB 取出 (右移),bit=1 → literal (1 byte);bit=0 → match:
 *   讀 2 bytes b1,b2,offset = b1 | ((b2 & 0xF0) << 4) (12-bit),length = (b2 & 0xF) + 3。
 * 回傳新配置的 buffer (out_size bytes),呼叫端負責 free;失敗回 NULL。*/
static byte *md_lzss_decompress(const byte *comp, int comp_len, int *out_size) {
	byte ring[4096];
	int r = 0xFEE;
	int osz, p, op;
	unsigned int flags;
	byte *out;

	if (comp_len < 8) return NULL;
	osz = (comp[4] << 24) | (comp[5] << 16) | (comp[6] << 8) | comp[7];
	if (osz <= 0 || osz > 0x40000) return NULL;

	out = malloc(osz);
	if (!out) return NULL;

	memset(ring, 0x20, sizeof(ring));
	p = 8;        /* 壓縮流位置 (相對 comp) */
	op = 0;       /* 輸出位置 */
	flags = 0;

	while (op < osz) {
		flags >>= 1;
		if ((flags & 0x100) == 0) {
			if (p >= comp_len) break;
			flags = comp[p++] | 0xFF00; /* 高位塞 1 當「還有幾 bit」計數 */
		}
		if (flags & 1) {
			/* literal */
			if (p >= comp_len) break;
			out[op++] = comp[p];
			ring[r] = comp[p];
			r = (r + 1) & 0xFFF;
			p++;
		} else {
			/* match */
			int off, len, k, b1, b2;
			if (p + 1 >= comp_len) break;
			b1 = comp[p]; b2 = comp[p + 1]; p += 2;
			off = b1 | ((b2 & 0xF0) << 4);
			len = (b2 & 0x0F) + 3;
			for (k = 0; k < len && op < osz; k++) {
				byte c = ring[(off + k) & 0xFFF];
				out[op++] = c;
				ring[r] = c;
				r = (r + 1) & 0xFFF;
			}
		}
	}

	if (out_size) *out_size = op;
	return out;
}

/* 解壓並快取地形 tile pattern (整個 ROM 生命週期只解一次)。
 * 回傳指向 pattern bytes (已 skip 2-byte header) 的指標 + tile 數;失敗回 NULL。*/
static const byte *md_terrain_pattern(const char *romname, int *out_ntiles) {
	static byte *cache = NULL;     /* 解壓後完整 buffer (含 2-byte header) */
	static int cache_len = 0;
	if (cache == NULL) {
		KB_File *f = KB_fopen(romname, "rb");
		byte *comp;
		long flen;
		int n, dsz;
		if (!f) return NULL;
		KB_fseek(f, 0, SEEK_END);
		flen = KB_ftell(f);
		KB_fseek(f, MD_TERRAIN_LZSS, 0);
		if (flen <= MD_TERRAIN_LZSS) { KB_fclose(f); return NULL; }
		n = (int)(flen - MD_TERRAIN_LZSS);
		comp = malloc(n);
		if (!comp) { KB_fclose(f); return NULL; }
		KB_fread(comp, 1, n, f);
		KB_fclose(f);
		cache = md_lzss_decompress(comp, n, &dsz);
		free(comp);
		if (!cache) return NULL;
		cache_len = dsz;
	}
	if (cache_len <= 2) return NULL;
	if (out_ntiles) *out_ntiles = (cache_len - 2) / 32; /* skip 2-byte header */
	return cache + 2;
}

/* 把一個 8x8 4bpp tile (32 bytes,高 nibble=左像素) 畫進 8-bit PAL surface。
 * 支援水平翻轉 (hflip)。pal_base = 該 sub-tile 的 palette line 在 64 色表的起點 (line*16)。
 * 像素 0 一律映到全域 index 0 (各 line 共用的透明色,設成 colorkey);像素 1..15 映到
 * pal_base + 像素 → per-tile palette line 正確上色。*/
static void md_blit_subtile(const byte *tile, SDL_Surface *surf, int ox, int oy,
                            int hflip, int pal_base) {
	int y, x;
	for (y = 0; y < 8; y++) {
		Uint8 *row = (Uint8 *)surf->pixels + (oy + y) * surf->pitch + ox;
		for (x = 0; x < 8; x += 2) {
			byte b = tile[y * 4 + x / 2];
			int hi = b >> 4, lo = b & 0xF;
			Uint8 ph = hi ? (Uint8)(pal_base + hi) : 0;
			Uint8 pl = lo ? (Uint8)(pal_base + lo) : 0;
			if (hflip) { row[7 - x] = ph; row[7 - (x + 1)] = pl; }
			else       { row[x] = ph;     row[x + 1] = pl; }
		}
	}
}

/* 用 cell template (0x19666) + terrain pattern 組一個 cell-type 的 48x40 metatile surface。
 * palette 取 4 條 line (per-tile bit13-14 選);index 0 設為 colorkey 透明。失敗回 NULL。*/
static SDL_Surface *md_build_cell(const char *romname, int cell_type) {
	const byte *pat;
	int ntiles = 0;
	byte tplbuf[60];          /* 60 bytes = 30 nametable word */
	byte rompal[4 * 32];      /* 4 line x 32 bytes */
	SDL_Color pal64[64];      /* 4 line x 16 色 (per-tile palette;index = line*16 + pixel) */
	SDL_Surface *surf;
	KB_File *f;
	int r, c, line;

	pat = md_terrain_pattern(romname, &ntiles);
	if (!pat) {
		KB_errlog("[md] terrain pattern decompress failed (cell %d) -- NULL\n", cell_type);
		return NULL;
	}
	KB_debuglog(0, "[md] build cell %d (ntiles=%d)\n", cell_type, ntiles);

	f = KB_fopen(romname, "rb");
	if (!f) return NULL;
	KB_fseek(f, MD_CELLTPL_OFFSET + cell_type * 60, 0);
	if (KB_fread(tplbuf, 1, 60, f) != 60) { KB_fclose(f); return NULL; }
	KB_fseek(f, MD_TERRAIN_PAL, 0);
	KB_fread(rompal, 1, sizeof(rompal), f);
	KB_fclose(f);

	/* 組 64 色 palette:4 條 Genesis line 各 16 色,展平成 line*16+pixel。
	 * metatile 內每個 sub-tile 依其 template 的 palette line 取對應 16 色區段。*/
	for (line = 0; line < 4; line++)
		md_build_palette(rompal + line * 32, &pal64[line * 16]);

	surf = SDL_CreatePALSurface(MD_CELL_W, MD_CELL_H);
	if (!surf) return NULL;
	{ SDL_Rect clr = { 0, 0, MD_CELL_W, MD_CELL_H }; SDL_FillRect(surf, &clr, 0); }

	for (r = 0; r < MD_CELL_ROWS; r++)
	for (c = 0; c < MD_CELL_COLS; c++) {
		int wi = r * MD_CELL_COLS + c;
		word e = (tplbuf[wi * 2] << 8) | tplbuf[wi * 2 + 1];
		int idx = e & 0x7FF;
		int hflip = (e >> 11) & 1;
		line = (e >> 13) & 3;
		if (idx < ntiles)
			md_blit_subtile(pat + idx * 32, surf, c * 8, r * 8, hflip, line * 16);
		/* idx 越界 (動畫 tile placeholder) → 留 index 0 (透明,露底層) */
	}

	/* index 0 = Genesis 透明色,真機露出 backdrop 海色。openkb 單 plane 無 plane B、
	 * 且 draw_map 畫地圖前 FillRect 紅底 (0xFF0000) → 不能 colorkey (會露紅)。
	 * 改把共用透明色 (pal64[0]) 設成不透明海藍,讓海洋/露底處顯示海色。 */
	pal64[0].r = 0; pal64[0].g = 72; pal64[0].b = 180;
	SDL_SetColors(surf, pal64, 0, 64);

	/* 垂直降採樣 48x40 → 48x34,對齊引擎 RECT_TILE 高 (否則地圖整體垂直錯位)。
	 * 索引式取樣 (不混色) 保留 palette。 */
	{
		SDL_Surface *out = SDL_CreatePALSurface(MD_CELL_W, MD_TILE_H);
		if (out) {
			int yy;
			SDL_SetColors(out, pal64, 0, 64);
			for (yy = 0; yy < MD_TILE_H; yy++) {
				int srcy = yy * MD_CELL_H / MD_TILE_H;
				memcpy((Uint8 *)out->pixels + yy * out->pitch,
				       (Uint8 *)surf->pixels + srcy * surf->pitch, MD_CELL_W);
			}
			SDL_FreeSurface(surf);
			return out;
		}
	}

	return surf;
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
		case GR_TILE:	/* 單一地形 metatile;sub_id = cell-type (0..71) */
		{
			return md_build_cell(mod->slotA_name, sub_id);
		}
		break;
		case GR_TILESET:	/* 整套地形 tileset;sub_id = continent (目前共用一套) */
		{
			SDL_Rect tilesize = { 0, 0, MD_CELL_W, MD_TILE_H };
			return KB_LoadTileset_TILES(&tilesize, MD_Resolve, mod);
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
