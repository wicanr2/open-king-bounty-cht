/*
 *  amiga-data.c -- resource loader for the Amiga version of King's Bounty
 *  (New World Computing, 1990).  Part of openkb.
 *
 *  The Amiga release ships its graphics as individually-named, LZSS-compressed
 *  files inside the disk image's GAME/ directory (peas, cstl, tileseta, ...).
 *  We read those already-unpacked files directly (the .adf is unpacked at build
 *  time); we do NOT parse the AmigaDOS filesystem here.
 *
 *  Container format (big-endian), reverse-engineered -- see
 *  docs/reverse-engineering/amiga-assets.md :
 *
 *    u16 count
 *    count * { u16 a; u16 w; u16 h }   // a: 0x1f sprite, 0 tile/picture
 *    u16 a_global; u16 0x0000
 *    u16[32] palette                   // 32 colors = 5 bitplanes, 12-bit 0RGB
 *    u16 comp_size; u16 0x0000         // size of the LZSS stream
 *    u16 out_size                      // size of the decompressed planar bitmap
 *    ... LZSS stream ...               // starts right after out_size
 *
 *  Codec: Okumura LZSS (same family as the Genesis port, see md-rom.c):
 *    4096-byte ring buffer, write pointer starts at 0xFEE, control byte = 8
 *    flag bits LSB-first (1 = literal, 0 = match), match = 2 bytes:
 *    offset = b1 | ((b2 & 0xF0) << 4) (12-bit), length = (b2 & 0xF) + 3.
 *
 *  Pixel layout: SEQUENTIAL bitplanes (plane0 whole image, then plane1, ...),
 *  plane0 = LSB, MSB-first within each byte.  Multi-frame sprites store the
 *  planes per frame: frame0 [p0..p4], frame1 [p0..p4], ...  Each frame is
 *  (w/8 * h * 5) bytes.  Decoded frames are laid out as a horizontal strip,
 *  matching the engine's IMGROW convention.
 */
#include "../sdlcompat.h"

#include <string.h> /* memset */

#include "kbfile.h"
#include "kbres.h"
#include "kbconf.h"
#include "kbstd.h"

#define AMIGA_PLANES   5      /* all resources are 5 bitplanes (32 colors) */
#define AMIGA_COLORKEY 0xFF   /* engine colorkey index for transparent sprites */

#define TILE_W 48
#define TILE_H 34

/* === Okumura LZSS (length = (b2 & 0xF) + 3; see md-rom.c md_lzss_decompress) ===
 * Decompresses exactly out_size bytes from src. Returns a newly malloc'd buffer
 * (out_size bytes); caller frees. Returns NULL on allocation failure. */
static byte *amiga_lzss(const byte *src, int src_len, int out_size) {
	byte ring[4096];
	int r = 0xFEE;
	int p = 0, op = 0;
	unsigned int flags = 0;
	byte *out;

	if (out_size <= 0) return NULL;
	out = malloc(out_size);
	if (!out) return NULL;

	memset(ring, 0, sizeof(ring));

	while (op < out_size) {
		flags >>= 1;
		if ((flags & 0x100) == 0) {
			if (p >= src_len) break;
			flags = src[p++] | 0xFF00; /* high byte counts remaining flag bits */
		}
		if (flags & 1) {
			/* literal */
			if (p >= src_len) break;
			out[op++] = src[p];
			ring[r] = src[p];
			r = (r + 1) & 0xFFF;
			p++;
		} else {
			/* match */
			int off, len, k, b1, b2;
			if (p + 1 >= src_len) break;
			b1 = src[p]; b2 = src[p + 1]; p += 2;
			off = b1 | ((b2 & 0xF0) << 4);
			len = (b2 & 0x0F) + 3;
			for (k = 0; k < len && op < out_size; k++) {
				byte c = ring[(off + k) & 0xFFF];
				out[op++] = c;
				ring[r] = c;
				r = (r + 1) & 0xFFF;
			}
		}
	}
	return out;
}

/* Convert one 12-bit 0RGB Amiga color word to an SDL_Color (4-bit channels *17). */
static void amiga_color(word c, SDL_Color *out) {
	out->r = (Uint8)(((c >> 8) & 0xF) * 17);
	out->g = (Uint8)(((c >> 4) & 0xF) * 17);
	out->b = (Uint8)(((c) & 0xF) * 17);
}

/* Read u16 big-endian from buf at offset o. */
static word be16(const byte *buf, int o) {
	return (word)((buf[o] << 8) | buf[o + 1]);
}

/*
 * Load an Amiga graphics resource into a single 8-bit PAL surface.
 * Multi-frame resources become a horizontal strip (w*count wide).
 * If transparent != 0, palette index 0 is remapped to AMIGA_COLORKEY and set as
 * the surface colorkey (Amiga sprites use index 0 as the transparent color).
 * Returns NULL on any failure.
 */
static SDL_Surface *amiga_load_surface(const char *path, int transparent) {
	KB_File *f;
	long flen;
	byte *file = NULL, *raw = NULL;
	SDL_Color pal[32];
	SDL_Surface *surf = NULL;
	int count, w, h, planes, rb, frame_sz, nframes, i;
	int hdr, comp_size, out_size, stream_off;

	f = KB_fopen(path, "rb");
	if (!f) return NULL;
	KB_fseek(f, 0, SEEK_END);
	flen = KB_ftell(f);
	KB_fseek(f, 0, SEEK_SET);
	if (flen < 8) { KB_fclose(f); return NULL; }
	file = malloc(flen);
	if (!file) { KB_fclose(f); return NULL; }
	if (KB_fread(file, 1, flen, f) != (int)flen) { free(file); KB_fclose(f); return NULL; }
	KB_fclose(f);

	/* --- parse header --- */
	count = be16(file, 0);
	if (count < 1 || count > 256) { free(file); return NULL; }
	hdr = 2 + count * 6;          /* count + descriptors */
	w = be16(file, 4);            /* desc[0].w (all descriptors share w/h) */
	h = be16(file, 6);            /* desc[0].h */
	hdr += 2 + 2;                 /* a_global + 0x0000 */
	/* palette: 32 words */
	for (i = 0; i < 32; i++) amiga_color(be16(file, hdr + i * 2), &pal[i]);
	hdr += 32 * 2;
	comp_size = be16(file, hdr); hdr += 2;
	hdr += 2;                     /* 0x0000 */
	out_size = be16(file, hdr); hdr += 2;
	stream_off = hdr;
	(void)comp_size;

	/* 除錯證據:每次載入印出檔名 + 解析出的 header (下次崩潰時 log 末行即兇手檔)。
	 * 走 KB_debuglog (預設靜默,KB_VERBOSE=1 開啟);異常值另用 KB_errlog 必印。 */
	KB_debuglog(0, "[amiga] %s: count=%d w=%d h=%d out_size=%d flen=%ld\n",
		path, count, w, h, out_size, flen);

	if (w <= 0 || h <= 0 || w > 4096 || h > 4096) {
		KB_errlog("[amiga] %s: bad dims w=%d h=%d (count=%d) -- skipped\n", path, w, h, count);
		free(file); return NULL;
	}
	if (out_size <= 0 || out_size > 0x20000) {
		KB_errlog("[amiga] %s: bad out_size=%d -- skipped\n", path, out_size);
		free(file); return NULL;
	}

	planes = AMIGA_PLANES;
	rb = (w + 7) / 8;             /* bytes per plane row */
	frame_sz = rb * h * planes;   /* bytes per frame (all planes) */
	nframes = count;
	if (frame_sz * nframes > out_size) {
		/* container claims fewer/more frames than out_size supports; clamp */
		nframes = out_size / frame_sz;
		if (nframes < 1) { free(file); return NULL; }
	}

	raw = amiga_lzss(file + stream_off, (int)flen - stream_off, out_size);
	free(file);
	if (!raw) return NULL;

	surf = SDL_CreatePALSurface(w * nframes, h);
	if (!surf) { free(raw); return NULL; }

	/* sequential planar -> chunky index, written straight into the surface */
	for (i = 0; i < nframes; i++) {
		int base = i * frame_sz;
		int x, y, p;
		for (y = 0; y < h; y++) {
			Uint8 *dst = (Uint8 *)surf->pixels + y * surf->pitch + i * w;
			for (x = 0; x < w; x++) {
				int bit = 7 - (x & 7);
				int v = 0;
				for (p = 0; p < planes; p++) {
					int idx = base + p * rb * h + y * rb + x / 8;
					if (idx < out_size)
						v |= ((raw[idx] >> bit) & 1) << p;
				}
				dst[x] = (Uint8)v;
			}
		}
	}
	free(raw);

	if (transparent) {
		/* Amiga sprites use index 0 as transparent. Remap it to the engine's
		 * colorkey index (0xFF) so it never collides with a real color, then
		 * set the colorkey. Palette slot 0xFF gets index-0's color (unused). */
		SDL_Rect whole = { 0, 0, surf->w, surf->h };
		pal[AMIGA_COLORKEY & 0x1f].r = pal[0].r; /* keep table sane (unused) */
		SDL_SetColors(surf, pal, 0, 32);
		SDL_ReplaceIndex(surf, &whole, 0, AMIGA_COLORKEY);
		SDL_SetColorKey(surf, SDL_SRCCOLORKEY, AMIGA_COLORKEY);
	} else {
		SDL_SetColors(surf, pal, 0, 32);
	}

	return surf;
}

/* Cut a w*h piece out of a wider strip surface (mirrors free-data GR_TILE). */
static SDL_Surface *amiga_cutout(SDL_Surface *strip, int x, int w, int h) {
	SDL_Surface *piece;
	SDL_Rect src = { x, 0, w, h };
	if (!strip) return NULL;
	piece = SDL_CreatePALSurface(w, h);
	if (!piece) { SDL_FreeSurface(strip); return NULL; }
	SDL_ClonePalette(piece, strip);
	SDL_BlitSurface(strip, &src, piece, NULL);
	SDL_FreeSurface(strip);
	return piece;
}

void *AMIGA_Resolve(KBmodule *mod, int id, int sub_id) {

	const char *name = NULL;   /* GAME/ filename for this resource */
	int transparent = 0;       /* sprite (index 0 colorkey) */
	int cutout = 0;            /* cut a single TILE_W tile out of a strip */
	int cutout_x = 0;

	switch (id) {
		case GR_TROOP:        /* sub_id = troop index 0..24 */
			if (sub_id < 0 || sub_id > 24) sub_id = 0;
			name = DOS_troop_names[sub_id];
			transparent = 1;
			break;
		case GR_VILLAIN:      /* sub_id = villain index 0..16 */
			if (sub_id < 0 || sub_id > 16) sub_id = 0;
			name = DOS_villain_names[sub_id];
			break;
		case GR_PORTRAIT:     /* sub_id = class 0..3 */
			if (sub_id < 0 || sub_id > 3) sub_id = 0;
			name = DOS_class_names[sub_id];
			break;
		case GR_LOCATION:     /* sub_id = 0 home, 1 town, 2..5 dwelling */
			if (sub_id < 0 || sub_id > 5) sub_id = 0;
			name = DOS_location_names[sub_id];
			break;
		case GR_TITLE:
			name = "title";
			break;
		case GR_LOGO:
			name = "nwcp";
			break;
		case GR_SELECT:
			name = "select";
			break;
		case GR_VIEW:
			name = "view";
			break;
		case GR_CURSOR:
			name = "cursor";
			break;
		case GR_COMTILES:
			name = "comtiles";
			transparent = 1;
			break;
		case GR_TILE:         /* sub_id = tile index 0..71 -> cut from tileseta/b */
			if (sub_id > 35) { cutout_x = (sub_id - 36) * TILE_W; name = "tilesetb"; }
			else             { cutout_x = sub_id * TILE_W;        name = "tileseta"; }
			cutout = 1;
			break;
		case GR_TILESALT:
			name = "tilesalt";
			break;
		case GR_TILESET:      /* sub_id = continent flavor; build via tile factory */
		{
			SDL_Rect tilesize = { 0, 0, TILE_W, TILE_H };
			if (sub_id) return KB_LoadTilesetSalted(sub_id, AMIGA_Resolve, mod);
			return KB_LoadTileset_TILES(&tilesize, AMIGA_Resolve, mod);
		}
		break;
		case RECT_TILE:
		{
			SDL_Rect *r = malloc(sizeof(SDL_Rect));
			if (!r) return NULL;
			r->x = 0; r->y = 0; r->w = TILE_W; r->h = TILE_H;
			return r;
		}
		break;
		default:
			break;
	}

	if (name) {
		char *realname = KB_fastpath(mod->slotA_name, "/", name);
		SDL_Surface *surf = amiga_load_surface(realname, transparent);
		free(realname);
		if (surf && cutout)
			surf = amiga_cutout(surf, cutout_x, TILE_W, TILE_H);
		return surf;
	}

	return NULL;
}
