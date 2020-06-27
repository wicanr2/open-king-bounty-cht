/*
 *  free-data.c -- Free module
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

#include "kbconf.h"	/* KBmodule type */
#include "kbres.h"	/* GR_? defines */
#include "kbfile.h"	/* KB_File operations */
#include "kbauto.h"	/* KB_fopen_with */
#include "kbsound.h"/* KBsound */

#include <SDL.h>  	/* SDL data types */
#ifdef HAVE_LIBSDL_IMAGE
#include <SDL_image.h>	/* PNG support */
#else
#warning "SDL_Image is not used, PNG support disabled!"
#endif

#include "dos-snd.h"
#include "free-snd.h"

#define TILE_W 48
#define TILE_H 34

#define STRL_MAX 1024

byte* GNU_downto_byte(dword *src, int len, int freesrc) {
	int i;
	byte *b = malloc(sizeof(byte) * len);
	for (i = 0; i < len; i++) {
		b[i] = src[i];
	}
	if (freesrc) {
		free(src);
	}
	return b;
}
word* GNU_downto_word(dword *src, int len, int freesrc) {
	int i;
	word *b = malloc(sizeof(word) * len);
	for (i = 0; i < len; i++) {
		b[i] = src[i];
	}
	if (freesrc) {
		free(src);
	}
	return b;
}

byte* GNU_spell_downto_byte(char *types, int num, int freesrc) {
	int i, j;

	static const char *names[] = { /* Should map to spell action indexes defined as SPELL_* in bounty.h */
		"magic_damage", //0
		"clone",//1
		"teleport",//2
		"freeze",//3
		"resurrect",//4
		"holy_damage",//5
		"bridge",//6
		"time_stop",//7
		"find_villain",//8
		"castle_gate",//9
		"town_gate",//10
		"instant_army",//11
		"raise_control"//12
	};
	int num_names = 13;

	byte *actions = malloc(sizeof(byte) * num);
	if (actions == NULL) return NULL;

	for (i = 0; i < num; i++) {
		char *flag = KB_strlist_peek(types, i);
		int found = 0;
		for (j = 0; j < num_names; j++) {
			if (!KB_strcasecmp(names[j], flag)) {

				actions[i] = j;

				found = 1;
				break;
			}
		}
		if (!found) {
			KB_errlog("Failed to parse `%s` spell type\n", flag);
		}
	}
	if (freesrc) {
		free(types);
	}
	return actions;
}

byte* GNU_artifact_downto_byte(char *types, int num, int freesrc) {
	int i, j;

	static const char *names[] = { /* Map to artifact action indexes defined as POWER_* in bounty.h */
		"unknown",
		"quarter_protection",
		"double_leadership",
		"double_spell_power",
		"increase_commission",
		"cheaper_boat_rental",
		"double_max_spells",
		"increased_damage",
	};
	static const int powers[] = {
		0x01,/* POWER_UNKNOWN_XXX1 */
		0x02,/* POWER_QUARTER_PROTECTION */
		0x04,/* POWER_DOUBLE_LEADERSHIP */
		0x08,/* POWER_DOUBLE_SPELL_POWER */
		0x10,/* POWER_INCREASE_COMMISSION */
		0x20,/* POWER_CHEAPER_BOAT_RENTAL */
		0x40,/* POWER_DOUBLE_MAX_SPELLS */
		0x80,/* POWER_INCREASED_DAMAGE */
	};
	int num_names = 8;

	byte *actions = malloc(sizeof(byte) * num);
	if (actions == NULL) return NULL;

	for (i = 0; i < num; i++) {
		char *flag = KB_strlist_peek(types, i);
		int found = 0;
		for (j = 0; j < num_names; j++) {
			if (!KB_strcasecmp(names[j], flag)) {

				actions[i] = powers[j];

				found = 1;
				break;
			}
		}
		if (!found) {
			KB_errlog("Failed to parse `%s` artifact type\n", flag);
		}
	}
	if (freesrc) {
		free(types);
	}
	return actions;
}

int GNU_parse_troop(const char *line, byte *type, word *num, char *troop_names) {
	char name[256];
	int val = 0;
	int i;
	if (sscanf(line, "%d x %s", &val, name) == 2
	|| sscanf(line, "%dx%s", &val, name) == 2
	|| sscanf(line, "%d x%s", &val, name) == 2
	|| sscanf(line, "%dx %s", &val, name) == 2
	|| sscanf(line, "%d %s", &val, name) == 2) {

		for (i = 0; i < 25; i++) {
			char *troop_name = KB_strlist_peek(troop_names, i);
			if (!KB_strcasecmp(troop_name, name)) {

				*num = (word)val;
				*type = i;

				return 1;
			}
		}

	}
	return 0;
}

/* Parse "20 x Peasants" string, return TROOP TYPE */
byte* GNU_army_downto_byte(int first, int num, char **armies, int army_len, char *troop_names, int freesrc) {
	int i, j;
	int max = num - first;
	int len = max * army_len;
	byte *b = malloc(sizeof(byte) * len);
	for (j = 0; j < num; j++) {
		for (i = 0; i < army_len; i++) {
			char *line = KB_strlist_peek(armies[i], j);
			if (!line) break;
			byte type;
			word num;
			int ok = GNU_parse_troop(line, &type, &num, troop_names);
			if (ok) {
				b[j * army_len + i] = type;
			}
			else {
				KB_errlog("! Can't parse army `%s`\n", line);
				return NULL;
			}
		}
		for (; i < army_len; i++) {
			b[j * army_len + i] = 0;
		}
	}
	if (freesrc) {
		for (i = 0; i < army_len; i++) {
			free(armies[i]);
		}
		free(troop_names);
	}
	return b;
}
/* Parse "20 x Peasants" string, return TROOP NUMBER */
word* GNU_army_downto_word(int first, int num, char **armies, int army_len, char *troop_names, int freesrc) {
	int i, j;
	int max = num - first;
	int len = max * army_len;
	word *b = malloc(sizeof(word) * len);
	for (j = first; j < num; j++) {
		for (i = 0; i < army_len; i++) {
			char *line = KB_strlist_peek(armies[i], j);
			if (!line) break;
			byte type;
			word num;
			int ok = GNU_parse_troop(line, &type, &num, troop_names);
			if (ok) {
				b[j * army_len + i] = type;
			}
			else {
				KB_errlog("! Can't parse army `%s`\n", line);
				return NULL;
			}
		}
		for (;i < army_len; i++) {
			b[j * army_len + i] = 0;
		}
	}
	if (freesrc) {
		for (i = 0; i < army_len; i++) {
			free(armies[i]);
		}
		free(troop_names);
	}
	return b;
}

char* GNU_string_ini(KBmodule *mod, const char *inifile, const char *module, const char *name, int first, int num, char *dst) {

	int len = num * 256;

	KB_File *fd;
	char *filename;
	char line[256];
	char module_fmt[256];
	char module_test[256];
	int module_test_len, name_test_len;
	int i, section;

	filename = KB_fastpath(mod->slotA_name, "/", inifile);
	if (filename == NULL) return NULL; /* Out of memory */

	if (dst == NULL) {
		dst = malloc(sizeof(char) * len);
		if (dst == NULL) {
			free(filename);
			return NULL; /* Out of memory */
		}
		memset(dst, 0, sizeof(char) * len);
		KB_strlist_clear(dst);
	}

	KB_debuglog(0, "? FREE INI FILE: %s\n", filename);

	fd = KB_fopen(filename, "r");
	if (fd == NULL) {
		KB_debuglog(0, "> FAILED TO OPEN, %s\n", filename);
		free(filename);
		return NULL;
	}

	sprintf(module_fmt, "[%s]", module);
	section = -1;
	if (strpbrk(module, "%") != NULL) {
		module_test_len = sprintf(module_test, module_fmt, section + 1);
	} else {
		strcpy(module_test, module_fmt);
		module_test_len = strlen(module_test);
	}
	name_test_len = strlen(name);

	int filled = 0;
	while (KB_fgets(line, sizeof(line), fd)) {

		if (!strncasecmp(line, module_test, module_test_len)) {
			module_test_len = sprintf(module_test, module_fmt, (++section)+1);
			continue;
		}
		if (section < first) {
			continue;
		}
		if (section > num) {
			break;
		}

		if (!strncasecmp(line, name, name_test_len)) {
			char test[1024];
			int test_len;
			char val[256];
			test_len = sprintf(test, "%s = %%[^\t\n;]", name);
			if (sscanf(line, test, val) == 1) {
				KB_strlist_append(dst, val);
				filled++;
			}
			if (filled >= num - first) break;
		}

	}

	KB_fclose(fd);
	free(filename);
	return dst;
}

dword* GNU_extract_ini(KBmodule *mod, const char *inifile, const char *module, const char *name, int first, int num, dword *dst) {
	KB_File *fd;
	char *filename;
	char line[256];
	char module_fmt[256];
	char module_test[256];
	int module_test_len, name_test_len;
	int i, section;
	int section_first = first;
	int section_num = num;
	int section_slice = 1;

	filename = KB_fastpath(mod->slotA_name, "/", inifile);
	if (filename == NULL) return NULL; /* Out of memory */

	if (dst == NULL) {
		dst = malloc(sizeof(dword) * num);
		if (dst == NULL) {
			free(filename);
			return NULL; /* Out of memory */
		}
		memset(dst, 0, sizeof(dword) * num);
	}

	KB_debuglog(0, "? FREE INI FILE: %s\n", filename);

	fd = KB_fopen(filename, "r");
	if (fd == NULL) {
		KB_debuglog(0, "> FAILED TO OPEN, %s\n", filename);
		free(filename);
		return NULL;
	}

	sprintf(module_fmt, "[%s]", module);
	section = -1;
	if (strpbrk(module, "%") != NULL) {
		module_test_len = sprintf(module_test, module_fmt, section + 1);
		section_first = first;
		section_num = num;
	} else {
		strcpy(module_test, module_fmt);
		module_test_len = strlen(module_test);
		section_first = 0;
		section_num = 1;
	}
	if (strpbrk(name, "%") != NULL) {
		section_slice = 0;
	}
	name_test_len = strlen(name);

	int filled = 0;
	while (KB_fgets(line, sizeof(line), fd)) {

		if (!strncasecmp(line, module_test, module_test_len)) {
			module_test_len = sprintf(module_test, module_fmt, (++section)+1);
			continue;
		}
		if (section < section_first) {
			continue;
		}
		if (section > section_first + section_num) {
			break;
		}

		if (!section_slice) {
			char key[256];
			char value[256];
			if (sscanf(line, "%s = %s", key, value) == 2) {
				int idx;
				if (sscanf(key, name, &idx) == 1) {
					if (idx < first || idx > first + num) {
						/* Out of range */
					} else {
						dword val;
						if (sscanf(value, "%d", &val) == 1) {
							dst[idx] = val;
						}
					}
				}
			}
			continue;
		}

		if (!strncasecmp(line, name, name_test_len)) {
			char test[1024];
			int test_len;
			dword val;
			test_len = sprintf(test, "%s = %%d", name);
			if (sscanf(line, test, &val) == 1) {
				dst[section - first] = val;
				filled++;
			} else {
				char buf[16];
				test_len = sprintf(test, "%s = #%%6c", name);
				if (sscanf(line, test, &buf[0]) == 1) {
					val = hex2dec(buf);
					dst[section - first] = val;
					filled++;
				}
			}
			if (filled >= num - first) break;
		}

	}

	KB_fclose(fd);
	free(filename);
	return dst;
}

char* GNU_expand_newlines(char *src, int n, int freesrc)
{
	int i, j = 0;
	char *dst = malloc(sizeof(char) * n);
	if (dst == NULL) return NULL;
	for (i = 0; i < n; i++) {
		if (i + 1 < n && src[i] == '\\' && src[i + 1] == 'n') {
			dst[j++] = '\n';
			i++; /* Skip extra char */
			continue;
		}
		dst[j++] = src[i];
	}
	dst[j] = 0xFF;
	if (freesrc) free(src);
	return dst;
}

Uint32* GNU_ReadTextColors(KBmodule *mod, const char *inifile, const char *section) {
	dword values[1] = { 0 };

	const char *names[] = { /* Should map to color scheme for COL_TEXT enum from kbres.h */
		"background",   	/* 0 */
		"text1",        	/* 1 */
		"text2",        	/* 2 */
		"text3",        	/* 3 */
		"text4",        	/* 4 */
		"shadow1",      	/* 5 */
		"shadow2",      	/* 6 */
		"frame1",       	/* 7 */
		"frame2",       	/* 8 */
		"sel_background",	/* 9 */
		"sel_text1",    	/* 10 */
		"sel_text2",    	/* 11 */
		"sel_text3",    	/* 12 */
		"sel_text4",    	/* 13 */
		"sel_shadow1",  	/* 14 */
		"sel_shadow2",  	/* 15 */
		"sel_frame1",   	/* 16 */
		"sel_frame2",   	/* 17 */
		/* maintaining this index is hell, we should DRY it out */
	};
	int i;

	Uint32 *colors = malloc(sizeof(Uint32) * COLORS_MAX);
	if (colors == NULL) return NULL;

	for (i = 0; i < COLORS_MAX; i++) {
		colors[i] = 0;
		if (GNU_extract_ini(mod, inifile, section, names[i], 0, 1, &values[0])) {
			colors[i] = values[0];
		}
	}
	return colors;
}

Uint32* GNU_ReadMinimapColors(KBmodule *mod, const char *inifile) {
	dword values[1] = { 0 };

	const char *names[] = { /* Should map to COL_MINIMAP special enum */
		"shallow_water",	/* 0 */
		"deep_water",   	/* 1 */
		"grass",        	/* 2 */
		"desert",       	/* 3 */
		"rock",         	/* 4 */
		"tree",         	/* 5 */
		"castle",       	/* 6 */
		"object",       	/* 7 */
	};

	int i;

	Uint32 *colors = malloc(sizeof(Uint32) * 8);
	if (colors == NULL) return NULL;

	for (i = 0; i < 8; i++) {
		GNU_extract_ini(mod, inifile, "minimap", names[i], 0, 1, &values[0]);
		colors[i] = values[0];
	}

	return colors;
}

Uint32* GNU_ReadDwellingColors(KBmodule *mod, const char *inifile) {
	dword values[1] = { 0 };

	const char *names[] = { /* Should map to COL_MINIMAP special enum */
		"dwelling0",	/* 0 */
		"dwelling1",   	/* 1 */
		"dwelling2",   	/* 2 */
		"dwelling3",   	/* 3 */
		"dwelling4"    	/* 4 */
	};

	int i;

	Uint32 *colors = malloc(sizeof(Uint32) * 5);
	if (colors == NULL) return NULL;

	for (i = 0; i < 5; i++) {
		GNU_extract_ini(mod, inifile, "dwellings", names[i], 0, 1, &values[0]);
		colors[i] = values[0];
	}

	return colors;
}

SDL_Rect* GNU_ReadRect(KBmodule *mod, const char *inifile, int which) {
	dword values[4] = { 0 };

	const char *names[] = { /* This doesn't map directly into any of the RECT_* defines :/ */
		"top",  	/* 0 */
		"left", 	/* 1 */
		"right",	/* 2 */
		"bottom",	/* 3 */
		"bar",  	/* 4 */
		"map",  	/* 5 */
		"tile", 	/* 6 */
		"uitile",	/* 7 */
		/* Another horrible index to maintain and watch out for :/ */
	};

	/* Just to make it clear: the "which" argument is magic number :( */
	const char *section = names[which];

	SDL_Rect *rect = malloc(sizeof(SDL_Rect));
	if (rect == NULL) return NULL;

	GNU_extract_ini(mod, "ui.ini", section, "x", 0, 1, &values[0]);
	GNU_extract_ini(mod, "ui.ini", section, "y", 0, 1, &values[1]);
	GNU_extract_ini(mod, "ui.ini", section, "w", 0, 1, &values[2]);
	GNU_extract_ini(mod, "ui.ini", section, "h", 0, 1, &values[3]);

	rect->x = values[0];
	rect->y = values[1];
	rect->w = values[2];
	rect->h = values[3];

/*	KB_debuglog(0, "Read RECT '%s' [%d,%d] - [%d,%d]\n"); */

	return rect;
}

char* GNU_read_textfile(KBmodule *mod, const char *textfile, int split) {

	KB_File *fd;
	char *filename, *buf;
	int n, i;

	filename = KB_fastpath(mod->slotA_name, "/", textfile);
	if (filename == NULL) return NULL; /* Out of memory */

	KB_debuglog(0, "? FREE TXT FILE: %s\n", filename);

	fd = KB_fopen(filename, "r");
	if (fd == NULL) {
		KB_debuglog(0, "> FAILED TO OPEN, %s\n", filename);
		free(filename);
		return NULL;
	}

	buf = malloc(sizeof(char) * STRL_MAX);
	if (buf == NULL) {
		KB_fclose(fd);
		free(filename);
		return NULL; /* Out of memory */
	}

	n = KB_fread(buf, sizeof(char), STRL_MAX, fd);
	if (buf[n] != '\n') {
		KB_debuglog(0, "Warning: no newline at the end of file\n");
	}
	buf[n] = '\0';

	/* Convert multi-line file to a strlist */
	if (split) {
		for (i = 0; i < n; i++)
			if (buf[i] == '\n') buf[i] = '\0';
		buf[n] = (char)0xFF;
	}

	KB_fclose(fd);
	free(filename);

	return buf;
}

const dword note_hertz[12*9] = {
	/* Octave 0 */
	/* C0  */ 16, /* C#0 */ 17, /* D0  */ 18, /* D#0 */ 19,
	/* E0  */ 21, /* F0  */ 22, /* F#0 */ 23, /* G0  */ 25,
	/* G#0 */ 26, /* A0  */ 28, /* A#0 */ 29, /* B0  */ 31,
	/* Octave 1 */
	/* C1  */ 33, /* C1# */ 35, /* D1  */ 37, /* D#1 */ 39,
	/* E1  */ 41, /* F1  */ 44, /* F#1 */ 46, /* G1  */ 49,
	/* G#1 */ 52, /* A1  */ 55, /* A#1 */ 58, /* B1  */ 62,
	/* Octave 2 */
	/* C2  */  65, /* C#2 */  69, /* D2  */  73, /* D#2 */  78,
	/* E2  */  82, /* F2  */  87, /* F#2 */  92, /* G2  */  98,
	/* G#2 */ 104, /* A2  */ 110, /* A#2 */ 116, /* B2  */ 123,
	/* Octave 3 */
	/* C3  */ 131, /* C#3 */ 138, /* D3  */ 147, /* D#3 */ 156,
	/* E3  */ 165, /* F3  */ 175, /* F#3 */ 185, /* G3  */ 196,
	/* G#3 */ 208, /* A3  */ 220, /* A#3 */ 233, /* B3  */ 247,
	/* Octave 4 */
	/* C4  */ 262, /* C#4 */ 277, /* D4  */ 294, /* D#4 */ 311,
	/* E4  */ 330, /* F4  */ 349, /* F#4 */ 370, /* G4  */ 392,
	/* G#4 */ 415, /* A4  */ 440, /* A#4 */ 466, /* B4  */ 494,
	/* Octave 5 */
	/* C5  */ 523, /* C#5 */ 554, /* D5  */ 587, /* D#5 */ 622,
	/* E5  */ 659, /* F5  */ 698, /* F#5 */ 740, /* G5  */ 784,
	/* G#5 */ 831, /* A5  */ 880, /* A#5 */ 932, /* B5  */ 988,
	/* Octave 6 */
	/* C6  */ 1046, /* C#6 */ 1109, /* D6  */ 1175, /* D#6 */ 1245,
	/* E6  */ 1319, /* F6  */ 1397, /* F#6 */ 1480, /* G6  */ 1568,
	/* G#6 */ 1661, /* A6  */ 1760, /* A#6 */ 1865, /* B6  */ 1976,
	/* Octave 7 */
	/* C7  */ 2093, /* C#7 */ 2217, /* D7  */ 2349, /* D#7 */ 2489,
	/* E7  */ 2637, /* F7  */ 2794, /* F#7 */ 2960, /* G7  */ 3136,
	/* G#7 */ 3322, /* A7  */ 3520, /* A#7 */ 3729, /* B7  */ 3951,
	/* Octave 8 */
	/* C8  */ 4186, /* C#8 */ 4435, /* D8  */ 4699, /* D#8 */ 4978,
	/* E8  */ 5274, /* F8  */ 5588, /* F#8 */ 5920, /* G8  */ 6272,
	/* G#8 */ 6645, /* A8  */ 7040, /* A#8 */ 7459, /* B8  */ 7902,
};
word default_delays[16] = {
	1000, 750, 500, 250,
	 125, 100,  62,	 50,
	  31,  15,   7,	  4,
	   3,   2, 	 1,   1,
};

char letter_to_note[12] = {
	/* 'a' */ 9,
	/* 'b' */ 11,
	/* 'c' */ 0,
	/* 'd' */ 2,
	/* 'e' */ 4,
	/* 'f' */ 5,
	/* 'g' */ 7,
};

KBsound* GNU_load_tune_ini(KBmodule *mod, const char* tunefile) {
	KBsound *snd;
	struct tunFile *tun_file;

	dword palC[7] = { 0 }, palCs[7] = { 0 }, palD[7] = { 0 },
		  palDs[7] = { 0 }, palE[7] = { 0 }, palF[7] = { 0 },
		  palFs[7] = { 0 }, palG[7] = { 0 }, palGs[7] = { 0 },
		  palA[7] = { 0 }, palAs[7] = { 0 }, palB[7] = { 0 };
	dword palLengths[16] = { 0 };
	dword *pal_ptr[12] = {
		palC, palCs, palD, palDs, palE, palF,
		palFs, palG, palGs, palA, palAs, palB
	};
	char *commandstring;
	dword freq_pal[88];
	dword delay_pal[16];
	byte note_res[255];
	byte delay_res[255];
	int res_idx = 0;

	GNU_extract_ini(mod, tunefile, "palette", "C%d", 0, 6, palC);
	GNU_extract_ini(mod, tunefile, "palette", "C#%d", 0, 6, palCs);
	GNU_extract_ini(mod, tunefile, "palette", "D%d", 0, 6, palD);
	GNU_extract_ini(mod, tunefile, "palette", "D#%d", 0, 6, palDs);
	GNU_extract_ini(mod, tunefile, "palette", "E%d", 0, 6, palE);
	GNU_extract_ini(mod, tunefile, "palette", "F%d", 0, 6, palF);
	GNU_extract_ini(mod, tunefile, "palette", "F#%d", 0, 6, palFs);
	GNU_extract_ini(mod, tunefile, "palette", "G%d", 0, 6, palG);
	GNU_extract_ini(mod, tunefile, "palette", "G#%d", 0, 6, palGs);
	GNU_extract_ini(mod, tunefile, "palette", "A%d", 0, 6, palA);
	GNU_extract_ini(mod, tunefile, "palette", "A#%d", 0, 6, palAs);
	GNU_extract_ini(mod, tunefile, "palette", "B%d", 0, 6, palB);

	GNU_extract_ini(mod, tunefile, "palette", "L%d", 0, 16, palLengths);

	commandstring = GNU_string_ini(mod, tunefile, "tune", "play", 0, 1, NULL);
	if (commandstring == NULL) return NULL;

	int i, j;
	for (j = 0; j < 7; j++) { //for each octave
		for (i = 0; i < 12; i++) { //for each note
			dword hertz = pal_ptr[i][j];
			if (hertz)
				freq_pal[j*12+i] = hertz;
			else /* use default palette */
				freq_pal[j*12+i] = note_hertz[j*12+i];
		}
	}
	for (i = 0; i < MAX_TUN_DELAYS; i++) { //for each "lenght"
		if (palLengths[i])
			delay_pal[i] = palLengths[i];
		else
			delay_pal[i] = default_delays[i];
	}

	int current_octave = 3;
	int sharp_modifier = 0;
	int current_length = 0;
	int last_argument = 0;
	int last_command = 0;
	int last_note = -1;
	int last_octave = -1;

	char numbuf[16];
	int numlen = 0;

	int l = strlen(commandstring);
	for (i = 0; i < l + 1; i++) {
		char c;
		if (i < l) c = commandstring[i];
		else c = ' ';
		/* To lower */
		if (c >= 'A' && c <= 'Z') c = c - ('A' - 'a');
		/* Numeric input */
		if (c >= '0' && c <= '9') {
			numbuf[numlen++] = c;
			numbuf[numlen] = '\0';
			continue;
		} else {
			/* End numeric input */
			if (numlen) {
				last_argument = atoi(numbuf);
				numlen = 0;
				/* Flush it */
				if (last_command) {
					if (last_command == 'o')
					{
						current_octave = last_argument;
					}
					else if (last_command == 'l')
					{
						current_length = last_argument;
					}
					last_argument = 0;
					last_command = 0;
				} else if (last_note != -1){
					last_octave = last_argument;
					last_argument = 0;
				}
			}
			if (c == ' ') {
				if (last_note != -1) {
					if (last_octave == -1) last_octave = current_octave;

					/* Resolve final note */
					int final_note = last_octave * 12 + last_note + sharp_modifier;

					/* Add new "entry" */
					note_res[res_idx] = final_note;
					delay_res[res_idx] = current_length;
					res_idx++;

					// printf("Adding note %d -> #%d (%d hz), #%d (%d ms)\n", res_idx-1, 
					// 	note_res[res_idx-1],
					// 	freq_pal[note_res[res_idx-1]],
					// 	delay_res[res_idx-1],
					// 	delay_pal[delay_res[res_idx-1]]
					// );

					if (res_idx >= MAX_TUN_NOTES) break;

					/* Reset everything */
					sharp_modifier = 0;
					last_octave = -1;
					last_note = -1;
				}
			}
			else if (c == '>') { current_octave++; }
			else if (c == '<') { current_octave--; }
			else if (c == 'o') { last_command = 'o'; }
			else if (c == 'l') { last_command = 'l'; }
			else if (c >= 'a' && c <= 'g') { last_note = letter_to_note[c - 'a']; }
			else if (c == '#' || c == '+') { sharp_modifier = 1; }
			else if (c == '-') { sharp_modifier = -1; }
		}
	}
	free(commandstring);

	if (res_idx == 0) return NULL; /* Zero notes */

	/* Prepare tunFile */
	tun_file = malloc(sizeof(struct tunFile));
	if (tun_file == NULL) return NULL; /* Out of memory */
	for (i = 0; i < MAX_TUN_DELAYS; i++) {
		tun_file->palette.duration[i] = delay_pal[i];
	}
	for (i = 0; i < MAX_TUN_FREQS; i++) {
		tun_file->palette.freq[i] = freq_pal[i];
	}
	for (i = 0; i < res_idx; i++)
	{
		tun_file->delay[i] = delay_res[i];
		tun_file->notes[i] = note_res[i];
	}
	tun_file->num_notes = res_idx;

	/* Prepare KBsound */
	snd = malloc(sizeof(KBsound));
	if (snd == NULL) {
		free(tun_file);
		return NULL; /* Out of memory */
	}
	snd->type = KBSND_DOS;
	snd->data = tun_file;
	return snd;
}

KBsound* GNU_load_tune_wav(KBmodule *mod, const char* tunefile) {
	KBsound *snd;
	KB_File *f;

	f = KB_fopen_with(tunefile, "rb", mod);
	if (f == NULL) return NULL;

	SDL_AudioSpec audio_device_spec;
	struct wavFile *wav_file;

	if (KB_GetAudioSpec(&audio_device_spec))
	{
		KB_debuglog(0, "[free] Refusing to load '%s', audio device off.\n", tunefile);
		KB_fclose(f);
		return NULL;
	}

	wav_file = wavFile_load_FILE(f, &audio_device_spec);
	if (wav_file == NULL) {
		KB_fclose(f);
		return NULL;
	}

	/* Prepare KBsound */
	snd = malloc(sizeof(KBsound));
	if (snd == NULL) {
		free(wav_file);
		return NULL; /* Out of memory */
	}
	snd->type = KBSND_WAV;
	snd->data = wav_file;
	return snd;
}



void* GNU_Resolve(KBmodule *mod, int id, int sub_id) {

	char *image_name = NULL;
	char *image_suffix = NULL;
	char *image_subid = "";
	SDL_Rect image_cutout_rect;
	int image_cutout = 0;
	int is_transparent = 1;
#ifdef HAVE_LIBSDL_IMAGE
#define _EXTN ".png"
#else
#define _EXTN ".bmp"
#endif
	switch (id) {
		case GR_LOGO:
		{
			image_name = "nwcp";
			image_suffix = _EXTN;
			is_transparent = 0;
		}
		break;
		case GR_TITLE:
		{
			image_name = "title";
			image_suffix = _EXTN;
			is_transparent = 0;
		}
		break;		
		case GR_SELECT:
		{
			image_name = "select";
			image_subid = "-0";
			if (sub_id == 1) image_subid="-1";
			if (sub_id == 2) image_subid="-2";
			image_suffix = _EXTN;
			is_transparent = 0;
		}
		break;
		case GR_FONT:
		{
			image_name = "openkb8x8";
			image_suffix = ".bmp";
			is_transparent = 0;
		}
		break;
		case GR_VILLAIN:
		{
			image_name = DOS_villain_names[sub_id];
			image_suffix = _EXTN;
			is_transparent = 0;
		}
		break;
		case GR_TROOP:
		{
			image_name = DOS_troop_names[sub_id];
			image_suffix = _EXTN;
		}
		break;
		case GR_CURSOR:
		{
			image_name = "cursor";
			image_suffix = _EXTN;
		}
		break;
		case GR_UI:
		{
			image_name = "sidebar";
			image_suffix = _EXTN;
			is_transparent = 0;
		}
		break;
		case GR_PURSE:
		{
			SDL_Surface *ts = SDL_CreatePALSurface(TILE_W, TILE_H);
			return ts;
		}
		break;
		case GR_VIEW:
		{
			image_name = "view";
			image_suffix = _EXTN;
			is_transparent = 0;
		}
		break;
		case GR_PORTRAIT:	/* subId - class */
		{
			if (sub_id < 0 || sub_id > 3) sub_id = 0;
			image_name = DOS_class_names[sub_id];
			image_suffix = _EXTN;
			is_transparent = 0;
		}
		break;
		case GR_LOCATION:	/* subId - 0 home 1 town 2 - 6 dwelling */
		{
			if (sub_id < 0 || sub_id > 5) sub_id = 0;
			image_name = DOS_location_names[sub_id];
			image_suffix = _EXTN;
			is_transparent = 0;
		}
		break;
		case GR_COINS:
		{
			image_name = "coins";
			image_suffix = _EXTN;
		}
		break;
		case GR_PIECE:
		{
			image_name = "piece";
			image_suffix = _EXTN;
		}
		break;
		case GR_COMTILES:
		{
			image_name = "comtiles";
			image_suffix = _EXTN;
			is_transparent = 1;
		}
		break;
		case GR_TILE:	/* subId - tile index */
		{
			/* A tile */
			if (sub_id > 35) {
				sub_id -= 36;
				image_name = "tilesetb";
			} else {
				image_name = "tileseta";
			}
			image_suffix = _EXTN;
			is_transparent = 0;
			image_cutout = 1;
			image_cutout_rect.x = sub_id * TILE_W;
			image_cutout_rect.y = 0;
			image_cutout_rect.w = TILE_W;
			image_cutout_rect.h = TILE_H;
		}
		break;
		case GR_TILEROW:	/* subId - row index */
		{
			/* A tile */
			if (sub_id > 35) {
				sub_id -= 36;
				image_name = "tilesetb";
			} else {
				image_name = "tileseta";
			}
			image_suffix = _EXTN;
			is_transparent = 0;
		}
		break;
		case GR_TILESET:	/* subId - continent */
		{
			SDL_Rect tilesize = { 0, 0, TILE_W, TILE_H };
			if (sub_id) return KB_LoadTilesetSalted(sub_id, GNU_Resolve, mod);
			return KB_LoadTileset_ROWS(&tilesize, GNU_Resolve, mod);
		}
		break;
		case GR_TILESALT:	/* subId - 0=full; 1-3=continent */
		{
			/* A row of replacement tiles. */
			image_name = "tilesalt";
			image_suffix = _EXTN;
		}
		break;
		case GR_ENDING:
		{
			image_name = "endpic";
			image_suffix = _EXTN;
			image_subid = "-0";
			if (sub_id == 1) image_subid="-1";
		}
		break;
		case GR_ENDTILE:
		{
			image_name = "endpic";
			image_suffix = _EXTN;
			image_subid = "-2";
			if (sub_id == 1) image_subid="-3";
			if (sub_id == 2) image_subid="-4";
		}
		break;
		case STRL_ENDINGS:
		{
			if (sub_id)
				return GNU_read_textfile(mod, "endlose.txt", 1);
			else
				return GNU_read_textfile(mod, "endwin.txt", 1);
		}
		break;
		case STR_SIGN:
		{
			return KB_strlist_peek(GNU_Resolve(mod, STRL_SIGNS, 0), sub_id);
		}
		break;
		case STRL_SIGNS:
		{
			char *list = GNU_read_textfile(mod, "signs.txt", 1);
			/* Convert multi-line file to a strlist (aka "signs need some extra work") */
			int n = (list ? strlen(list) : 0);
			int i, j = 0;
			for (i = 0; i < n; i++) {
				if (list[i] == '\n') {
					if (j) list[i] = '\0';
					j = 1 - j;
				}
			}
			return list;
		}
		case STRL_CREDITS:	/* multiple lines of credits */
		{
			return GNU_read_textfile(mod, "credits.txt", 0);
		}
		break;
		case STRL_ADESCS:
		{
			char *desc = GNU_string_ini(mod, "artifacts.ini", "artifact%d", "description", 0, 8, NULL);
			desc = GNU_expand_newlines(
				desc,
				KB_strlist_len(desc),
				1
			);
			return KB_strdup(KB_strlist_peek(desc, sub_id));
		}
		case STRL_ANAMES:
		{
			return GNU_string_ini(mod, "artifacts.ini", "artifact%d", "name", 0, 8, NULL);
		}
		case STRL_VDESCS:	/* multiple lines of villain description */
		{
			char tmp[128];
			KB_strcpy(tmp, DOS_villain_names[sub_id]);
			KB_strcat(tmp, ".txt");
			return GNU_read_textfile(mod, tmp, 0);
		}
		break;
		case STRL_VNAMES:	/* villain names */
		{
			return GNU_string_ini(mod, "villains.ini", "villain%d", "name", 0, 17, NULL);
		}
		break;
		case STRL_TROOPS:	/* troop names */
		{
			return GNU_string_ini(mod, "troops.ini", "troop%d", "name", 0, 25, NULL);
		}
		break;
		case STRL_SPELLS:	/* spell names */
		{
			return GNU_string_ini(mod, "spells.ini", "spell%d", "name", 0, 14, NULL);
		}
		break;
		case STRL_CONTINENTS:	/* continent names */
		{
			return GNU_string_ini(mod, "land.ini", "continent%d", "name", 0, 4, NULL);
		}
		break;
		case STRL_CASTLES:	/* castle names */
		{
			return GNU_string_ini(mod, "castles.ini", "castle%d", "name", 0, 26, NULL);
		}
		break;
		case STRL_TOWNS:	/* town names */
		{
			return GNU_string_ini(mod, "towns.ini", "town%d", "name", 0, 26, NULL);
		}
		break;
		case STR_VNAME:
		{
			return KB_strlist_peek(GNU_Resolve(mod, STRL_VNAMES, 0), sub_id);
		}
		break;
		case STR_CASTLE:
		{
			return KB_strlist_peek(GNU_Resolve(mod, STRL_CASTLES, 0), sub_id);
		}
		break;
		case STR_TOWN:
		{
			return KB_strlist_peek(GNU_Resolve(mod, STRL_CASTLES, 0), sub_id);
		}
		break;
		case STR_TROOP:
		{
			return KB_strlist_peek(GNU_Resolve(mod, STRL_TROOPS, 0), sub_id);
		}
		break;
		case WDAT_VREWARD:
		{
			word *reward =
				GNU_downto_word(
					GNU_extract_ini(mod, "villains.ini", "villain%d", "reward", 0, 17, NULL)
					, 17, 1);
			return reward;
		}
		break;
		case WDAT_SCOST:
		{
			word *goldcost =
				GNU_downto_word(
					GNU_extract_ini(mod, "spells.ini", "spell%d", "gold", 0, 14, NULL)
					, 14, 1);
			return goldcost;
		}
		break;
		case DAT_SACTION:	/* [MAX_SPELLS] spell action for specific spell; subId - undefined */ \
		{
			byte *spelltypes =
				GNU_spell_downto_byte(
					GNU_string_ini(mod, "spells.ini", "spell%d", "type", 0, 14, NULL)
					, 14, 1);
			return spelltypes;
		}
		break;
		case WDAT_SDAMAGE: /* [MAX_SPELLS] spell power for specific spell; subId - undefined */ \
		{
			word *damages =
				GNU_downto_word(
					GNU_extract_ini(mod, "spells.ini", "spell%d", "damage", 0, 14, NULL)
					, 14, 1);
			return damages;
		}
		break;
		case DAT_APOWER:	/* [MAX_ARTIFACTS] artifcat power for specific artifcat; subId - undefined */ \
		{
			return GNU_artifact_downto_byte(
				GNU_string_ini(mod, "artifacts.ini", "artifact%d", "power", 0, 8, NULL)
				, 8, 1);
		}
		break;
		case DAT_SPECIALX:
		{
			byte *coord =
				GNU_downto_byte(
					GNU_extract_ini(mod, "land.ini", "special%d", "x", 0, 2, NULL)
					, 2, 1);
			return coord;
		}
		break;
		case DAT_SPECIALY:
		{
			byte *coord =
				GNU_downto_byte(
					GNU_extract_ini(mod, "land.ini", "special%d", "y", 0, 2, NULL)
					, 2, 1);
			return coord;
		}
		break;
		case DAT_SPECIALC:
		{
			byte *coord =
				GNU_downto_byte(
					GNU_extract_ini(mod, "land.ini", "special%d", "continent", 0, 2, NULL)
					, 2, 1);
			return coord;
		}
		break;
		case DAT_CASTLEX:
		{
			byte *coord =
				GNU_downto_byte(
					GNU_extract_ini(mod, "castles.ini", "castle%d", "x", 0, 26, NULL)
					, 26, 1);
			return coord;
		}
		break;
		case DAT_CASTLEY:
		{
			byte *coord =
				GNU_downto_byte(
					GNU_extract_ini(mod, "castles.ini", "castle%d", "y", 0, 26, NULL)
					, 26, 1);
			return coord;
		}
		break;
		case DAT_CASTLEC:
		{
			byte *coord =
				GNU_downto_byte(
					GNU_extract_ini(mod, "castles.ini", "castle%d", "continent", 0, 26, NULL)
					, 26, 1);
			return coord;
		}
		break;
		case DAT_TOWNC:
		{
			byte *coord =
				GNU_downto_byte(
					GNU_extract_ini(mod, "towns.ini", "town%d", "continent", 0, 26, NULL)
					, 26, 1);
			return coord;
		}
		break;
		case DAT_TOWNY:
		{
			byte *coord =
				GNU_downto_byte(
					GNU_extract_ini(mod, "towns.ini", "town%d", "y", 0, 26, NULL)
					, 26, 1);
			return coord;
		}
		break;
		case DAT_TOWNX:
		{
			byte *coord =
				GNU_downto_byte(
					GNU_extract_ini(mod, "towns.ini", "town%d", "x", 0, 26, NULL)
					, 26, 1);
			return coord;
		}
		break;
		case DAT_TOWNINV:
		{
			byte *inversion =
				GNU_downto_byte(
					GNU_extract_ini(mod, "towns.ini", "town%d", "invert_id", 0, 26, NULL)
					, 26, 1);
			return inversion;
		}
		break;
		case DAT_BOATY:
		{
			byte *coord =
				GNU_downto_byte(
					GNU_extract_ini(mod, "towns.ini", "town%d", "boat_y", 0, 26, NULL)
					, 26, 1);
			return coord;
		}
		break;
		case DAT_BOATX:
		{
			byte *coord =
				GNU_downto_byte(
					GNU_extract_ini(mod, "towns.ini", "town%d", "boat_x", 0, 26, NULL)
					, 26, 1);
			return coord;
		}
		break;
		case DAT_NAVY:
		{
			byte *coord =
				GNU_downto_byte(
					GNU_extract_ini(mod, "land.ini", "continent%d", "nav_y", 0, 26, NULL)
					, 26, 1);
			return coord;
		}
		break;
		case DAT_NAVX:
		{
			byte *coord =
				GNU_downto_byte(
					GNU_extract_ini(mod, "land.ini", "continent%d", "nav_x", 0, 26, NULL)
					, 26, 1);
			return coord;
		}
		break;
		case DAT_HPS:  	/* [MAX_TROOPS] hit points for specific troop; subId - undefined */ \
		{
			byte *hp =
				GNU_downto_byte(
					GNU_extract_ini(mod, "troops.ini", "troop%d", "hp", 0, 25, NULL)
					, 25, 1);
			return hp;
		}
		break;
		case DAT_VTROOP:	/* [MAX_VILLAINS * 5] villain troop type; subId - (villain index * 5) + army index */ \
		{
			char *troop_names = GNU_Resolve(mod, STRL_TROOPS, 0);
			char *army0 = GNU_string_ini(mod, "villains.ini", "villain%d", "army0", 0, 17, NULL);
			char *army1 = GNU_string_ini(mod, "villains.ini", "villain%d", "army1", 0, 17, NULL);
			char *army2 = GNU_string_ini(mod, "villains.ini", "villain%d", "army2", 0, 17, NULL);
			char *army3 = GNU_string_ini(mod, "villains.ini", "villain%d", "army3", 0, 17, NULL);
			char *army4 = GNU_string_ini(mod, "villains.ini", "villain%d", "army4", 0, 17, NULL);
			char *armies[5] = {
				army0,
				army1,
				army2,
				army3,
				army4,
			};
			return GNU_army_downto_byte(
				0, 17,
				armies, 5,
				troop_names,
				1
			);
		}
		case WDAT_VNUMBER:
		{
			char *troop_names = GNU_Resolve(mod, STRL_TROOPS, 0);
			char *army0 = GNU_string_ini(mod, "villains.ini", "villain%d", "army0", 0, 17, NULL);
			char *army1 = GNU_string_ini(mod, "villains.ini", "villain%d", "army1", 0, 17, NULL);
			char *army2 = GNU_string_ini(mod, "villains.ini", "villain%d", "army2", 0, 17, NULL);
			char *army3 = GNU_string_ini(mod, "villains.ini", "villain%d", "army3", 0, 17, NULL);
			char *army4 = GNU_string_ini(mod, "villains.ini", "villain%d", "army4", 0, 17, NULL);
			char *armies[5] = {
				army0,
				army1,
				army2,
				army3,
				army4,
			};
			return GNU_army_downto_word(
				0, 17,
				armies,	5,
				troop_names,
				1
			);
		}
		case DAT_WORLD:
		{
			KB_File *f;
			int n;
			byte *world;
			int len = 64 * 64 * 4;
			world = malloc(sizeof(byte) * len);
			if (!world) return NULL;
			f = KB_fopen_with("land.org", "rb", mod);
			if (f == NULL) return NULL;
			n = KB_fread(world, sizeof(byte), len, f);
			KB_fclose(f);
			return world;
		}
		break;
		case SN_TUNE:
		{
			KBsound* snd;
			char tunefile[256];
			sprintf(tunefile, "tune%02d.wav", sub_id);
			snd = GNU_load_tune_wav(mod, tunefile);
			if (snd) return snd;
			sprintf(tunefile, "tune%02d.ini", sub_id);
			snd = GNU_load_tune_ini(mod, tunefile);
			return snd;
		}
		case RECT_UI:
		{
			if (sub_id < 0 || sub_id > 4) return NULL;
			return GNU_ReadRect(mod, "ui.ini", sub_id);
		}
		case RECT_MAP:
		{
			return GNU_ReadRect(mod, "ui.ini", 5);
		}
		break;
		case RECT_TILE:
		{
			return GNU_ReadRect(mod, "ui.ini", 6);
		}
		break;
		case RECT_UITILE:
		{
			return GNU_ReadRect(mod, "ui.ini", 7);
		}
		break;
		case COL_TEXT:
		{
			const char *CS_names[] = { /* Index is one of CS_ defines from kbres.h */
				"generic",  /* CS_GENERIC  == 0 */
				"status1",  /* CS_STATUS_1 == 1 */
				"status2",  /* CS_STATUS_2 == 2 */
				"status3",  /* CS_STATUS_3 == 3 */
				"status4",  /* CS_STATUS_4 == 4 */
				"status5",  /* CS_STATUS_5 == 5 */
				"submenu",  /* CS_MINIMENU == 6 */
				"character",/* CS_VIEWCHAR == 7 */
				"army",     /* CS_VIEWARMY == 8 */
				"maplegend",/* CS_MINIMAP  == 9 */
				"chrome",   /* CS_CHROME   == 10 */
				"ending",   /* CS_ENDING   == 11 */
			};
			if (sub_id < 0 || sub_id > 11) return NULL;
			return GNU_ReadTextColors(mod, "colors.ini", CS_names[sub_id]);
		}
		break;
		case COL_DWELLING:
		{
			return GNU_ReadDwellingColors(mod, "colors.ini");
		}
		break;
		case COL_MINIMAP:
		{
			Uint32 *arch = GNU_ReadMinimapColors(mod, "colors.ini");
			enum {
				SHALLOW_WATER,
				DEEP_WATER,
				GRASS,
				DESERT,
				ROCK,
				TREE,
				CASTLE,
				MAP_OBJECT,
			} tile_type = DEEP_WATER;
			Uint32 *colors;
			int tile;

			colors = malloc(sizeof(Uint32) * 256);
			if (colors == NULL) return NULL;

			for (tile = 0; tile < 256; tile++) {

				if ( IS_GRASS(tile) ) tile_type = GRASS;
				else if ( IS_DEEP_WATER(tile) ) tile_type = DEEP_WATER;
				else if ( IS_WATER(tile) ) tile_type = SHALLOW_WATER;
				else if ( IS_DESERT(tile) ) tile_type = DESERT;
				else if ( IS_ROCK(tile) ) tile_type = ROCK;
				else if ( IS_TREE(tile) ) tile_type = TREE;
				else if ( IS_CASTLE(tile) ) tile_type = CASTLE;
				else if ( IS_MAPOBJECT(tile) || IS_INTERACTIVE(tile)) tile_type = MAP_OBJECT;

				colors[tile] = arch[tile_type];
			}
			free(arch);
			return colors;
		}
		break;
		default: break;
	}

	if (image_name) {

		char *realname;

		realname = KB_fastpath(mod->slotA_name, "/", image_name, image_subid, image_suffix); 

		KB_debuglog(0, "? FREE IMG FILE: %s\n", realname);

#ifdef HAVE_LIBSDL_IMAGE
		SDL_Surface *surf = IMG_Load(realname);
		if (surf == NULL) KB_debuglog(0, "> FAILED TO OPEN, %s\n", IMG_GetError());
#else
		SDL_Surface *surf = SDL_LoadBMP(realname);
		if (surf == NULL) KB_debuglog(0, "> FAILED TO OPEN, %s\n", SDL_GetError());
#endif
		if (image_cutout) {
			SDL_Surface *piece = SDL_CreatePALSurface(image_cutout_rect.w, image_cutout_rect.h);
			SDL_ClonePalette(piece, surf);
			SDL_BlitSurface(surf, &image_cutout_rect, piece, NULL);
			SDL_FreeSurface(surf);
			surf = piece;
		}

		if (surf && is_transparent)
			SDL_SetColorKey(surf, SDL_SRCCOLORKEY, 0xFF);

		free(realname);
		return surf;
	}

	return NULL;
}
