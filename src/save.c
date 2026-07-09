/*
 *  save.c -- routines to load/save KB games
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
#include "bounty.h"
#include "lib/kbstd.h" /* for malloc */

#define DAT_SIZE 20421

/* issue #9 遊戲調整設定:存檔本體(DAT_SIZE)之後追加的一小塊區塊。
 * 不改 DAT_SIZE、不動 buf 版面,向後相容舊存檔(讀不到/magic 不符 -> 全部設定預設 0)。 */
#define KB_OPT_MAGIC 0xE9
#define KB_OPT_COUNT 6

#define RPOS(STR) printf("%s Pos: %06x\n", STR, p - &buf[0])

KBgame* KB_loadDAT_amiga(const char* filename);
KBgame* KB_loadDAT(const char* filename) {
	char buf[DAT_SIZE];
	KBgame *game;
	FILE *f;
	int n, k, map_size;
	int x, y;
	byte opt_block[1 + KB_OPT_COUNT];
	int opt_block_ok = 0;

	f = fopen(filename, "rb");
	if (f == NULL) return NULL;

	n = fread(buf, sizeof(char), DAT_SIZE, f);
	if (n != DAT_SIZE) { fclose(f); return NULL; }

	/* issue #9:嘗試讀存檔尾端追加的遊戲調整設定區塊。
	 * 舊存檔在 DAT_SIZE 處就是 EOF,下面這個 fread 讀不滿 -> opt_block_ok 維持 0,設定全部預設關。
	 * 必須在 fclose(f) 之前讀,檔案關掉就沒機會了。 */
	if (fread(opt_block, sizeof(char), 1, f) == 1 && opt_block[0] == KB_OPT_MAGIC) {
		if (fread(&opt_block[1], sizeof(char), KB_OPT_COUNT, f) == (size_t)KB_OPT_COUNT)
			opt_block_ok = 1;
	}

	fclose(f);

	game = malloc(sizeof(KBgame));
	if (game == NULL) return NULL;

	/* Read out the save from "buf" */
	char *p = &buf[0];

	/* Player name */
	memcpy(game->name, p, 10);
	for (n = 9; n > 0; n--) {
		if (game->name[n] != ' ') break;
		game->name[n] = '\0';
	}
	p += 11;

	/* Main stats */
	game->class = READ_BYTE(p);
	game->rank = READ_BYTE(p);
	game->spell_power = READ_BYTE(p);
	game->max_spells = READ_BYTE(p);

	/* Quest progress */
	for (n = 0; n < MAX_VILLAINS; n++)
		game->villain_caught[n] = READ_BYTE(p);

	for (n = 0; n < MAX_ARTIFACTS; n++)
		game->artifact_found[n] = READ_BYTE(p);

	for (n = 0; n < MAX_CONTINENTS; n++)
		game->continent_found[n] = READ_BYTE(p);

	for (n = 0; n < MAX_CONTINENTS; n++)
		game->orb_found[n] = READ_BYTE(p);

	/* Spell book */
	for (n = 0; n < MAX_SPELLS; n++)
		game->spells[n] = READ_BYTE(p);

	/* More stats */
	game->knows_magic = READ_BYTE(p);
	game->siege_weapons = READ_BYTE(p);
	game->contract = READ_BYTE(p);

	for (n = 0; n < 5; n++)
		game->player_troops[n] = READ_BYTE(p);

	/* Options and difficulty setting */
	game->options[0] = READ_BYTE(p);//delay

	game->difficulty = READ_BYTE(p);

 	game->options[1] = READ_BYTE(p);//sounds
 	game->options[2] = READ_BYTE(p);//walk beep
 	game->options[3] = READ_BYTE(p);//animation
 	game->options[4] = READ_BYTE(p);//show army size

	/* Player location */
	game->continent = READ_BYTE(p);
	game->x = READ_BYTE(p);
	game->y = READ_BYTE(p);
	game->last_x = READ_BYTE(p);
	game->last_y = READ_BYTE(p);
	game->boat_x = READ_BYTE(p);
	game->boat_y = READ_BYTE(p);
	game->boat = READ_BYTE(p);
	game->mount = READ_BYTE(p);

	game->options[5] = READ_BYTE(p);//CGA palette

	/* Spells sold in towns */
	for (n = 0; n < MAX_TOWNS; n++)
		game->town_spell[n] = READ_BYTE(p);

	/* Town/Contract */
	for (n = 0; n < 5; n++)
		game->contract_cycle[n] = READ_BYTE(p);
	game->last_contract =  READ_BYTE(p);
	game->max_contract = READ_BYTE(p);

	/* Steps left till end of day */
	game->steps_left = READ_BYTE(p);

	/* Skip 1 byte */
	game->unknown3 = READ_BYTE(p);

	/* Castle ownership */
	for (n = 0; n < MAX_CASTLES; n++)
		game->castle_owner[n] = READ_BYTE(p);

	/* Visited castles and towns */
	for (n = 0; n < MAX_CASTLES; n++)
		game->castle_visited[n] = READ_BYTE(p);

	for (n = 0; n < MAX_TOWNS; n++)
		game->town_visited[n] = READ_BYTE(p);

	/* Scepter location (encrypted) */
	game->scepter_continent = READ_BYTE(p);
	game->scepter_x = READ_BYTE(p);
	game->scepter_y = READ_BYTE(p);

	/* Fog of war (convert from 1-bit-per-tile to 1-byte-per-tile format) */
	map_size = ( MAX_CONTINENTS * LEVEL_W * LEVEL_H );
	for (n = 0; n < MAX_CONTINENTS; n++) {
		for (y = 0; y < LEVEL_H; y++)
		for (x = 0; x < LEVEL_W; x += 8) {
			char test_bits = READ_BYTE(p);
			for (k = 0; k < 8; k++) {
				char one_bit = ((test_bits & (0x01 << k)) >> k) & 0x01;
				game->fog[n][y][x + 7 - k] = one_bit;
			}
		}
	}

	/* Castle garrison troops */
	for (n = 0; n < MAX_CASTLES; n++)
		for (k = 0; k < 5; k++)
			game->castle_troops[n][k] = READ_BYTE(p);

	/* Read friendly foes' coords */
	for (n = 0; n < MAX_CONTINENTS; n++) {
		for (k = 0; k < FRIENDLY_FOES; k++) {
			game->foe_coords[n][k][0] = READ_BYTE(p);//X
			game->foe_coords[n][k][1] = READ_BYTE(p);//Y
		}	
	}

	/* Map chests */
	for (n = 0; n < MAX_CONTINENTS - 1; n++) {
		game->map_coords[n][0] = READ_BYTE(p);//X
		game->map_coords[n][1] = READ_BYTE(p);//Y
	}

	/* Orb chests */
	for (n = 0; n < MAX_CONTINENTS; n++) {
		game->orb_coords[n][0] = READ_BYTE(p);//X
		game->orb_coords[n][1] = READ_BYTE(p);//Y
	}

	/* Teleporting caves */
	for (n = 0; n < MAX_CONTINENTS; n++) {
		for (k = 0; k < MAX_TELECAVES; k++) {
			game->teleport_coords[n][k][0] = READ_BYTE(p);//X
			game->teleport_coords[n][k][1] = READ_BYTE(p);//Y
		}
	}

	/* Dwellings locations */
	for (n = 0; n < MAX_CONTINENTS; n++) {
		for (k = 0; k < MAX_DWELLINGS; k++) {
			game->dwelling_coords[n][k][0] = READ_BYTE(p);//X
			game->dwelling_coords[n][k][1] = READ_BYTE(p);//Y
		}
	}

	/* Read hostile foes' coords */
	for (n = 0; n < MAX_CONTINENTS; n++) {
		for (k = FRIENDLY_FOES; k < MAX_FOES; k++) {
			game->foe_coords[n][k][0] = READ_BYTE(p);//X
			game->foe_coords[n][k][1] = READ_BYTE(p);//Y	
		}
	}

	/* Read hostile foes' troops */
	for (n = 0; n < MAX_CONTINENTS; n++) {
		for (k = FRIENDLY_FOES; k < MAX_FOES; k++) {
			game->foe_troops[n][k][0] = READ_BYTE(p);
			game->foe_troops[n][k][1] = READ_BYTE(p);
			game->foe_troops[n][k][2] = READ_BYTE(p);
		}
	}

	/* Read foe numbers */
	for (n = 0; n < MAX_CONTINENTS; n++) {
		for (k = FRIENDLY_FOES; k < MAX_FOES; k++) {
			game->foe_numbers[n][k][0] = READ_BYTE(p);
			game->foe_numbers[n][k][1] = READ_BYTE(p);
			game->foe_numbers[n][k][2] = READ_BYTE(p);
		}
	}

	/* Read dwelling troop type and population count */
	for (n = 0; n < MAX_CONTINENTS; n++)
		for (k = 0; k < MAX_DWELLINGS; k++)
			game->dwelling_troop[n][k] = READ_BYTE(p);	

	for (n = 0; n < MAX_CONTINENTS; n++)
		for (k = 0; k < MAX_DWELLINGS; k++)
			game->dwelling_population[n][k] = READ_BYTE(p);

	/* Read scepter key and un-encrypt the coordinates */
	game->scepter_key = READ_BYTE(p);
	game->scepter_continent ^= game->scepter_key;
	game->scepter_x ^= game->scepter_key;
	game->scepter_y ^= game->scepter_key;

	/* More stats */
	game->base_leadership = READ_WORD(p);
	game->leadership = READ_WORD(p);
	game->commission = READ_WORD(p);
	game->followers_killed = READ_WORD(p);

	/* Player army numbers */
	for (n = 0; n < 5; n++)
		game->player_numbers[n] = READ_WORD(p);

	/* Castle garrison numbers */
	for (n = 0; n < MAX_CASTLES; n++)
		for (k = 0; k < 5; k++)
			game->castle_numbers[n][k] = READ_WORD(p);

	/* More stats */
	game->time_stop = READ_WORD(p);
	game->days_left = READ_WORD(p);

	/* Score (unused) */
	game->score = READ_WORD(p);

	/* Skip 2 bytes */
	game->unknown1 = READ_BYTE(p);
	game->unknown2 = READ_BYTE(p);

	/* Player's gold (32 bit, can get quite rich!) */
	game->gold = READ_DWORD(p);

	/* Map dump */
	n = map_size; 
	memcpy(game->map, p, n);
	p += n;

	if (p - &buf[0] != DAT_SIZE)
	fprintf(stdout, "Save file: %d bytes (needed %d)\n", p - &buf[0], DAT_SIZE);

	/* issue #9 遊戲調整設定:opt_block_ok 才套用讀到的值,否則明確預設 0(關閉),
	 * 別讓 malloc() 未初始化的記憶體亂入(這個 struct 不是 calloc 出來的)。 */
	if (opt_block_ok) {
		game->opt_no_wages     = opt_block[1];
		game->opt_ai_mode      = opt_block[2];
		game->opt_days_x2      = opt_block[3];
		game->opt_foe_freq     = opt_block[4];
		game->opt_foe_strength = opt_block[5];
		game->opt_recruit_caps = opt_block[6];
	} else {
		game->opt_no_wages = 0;
		game->opt_ai_mode = 0;
		game->opt_days_x2 = 0;
		game->opt_foe_freq = 0;
		game->opt_foe_strength = 0;
		game->opt_recruit_caps = 0;
	}

	return game;
}


KBgame* KB_loadDAT_amiga(const char* filename) {

	char buf[DAT_SIZE];
	KBgame *game;
	FILE *f;
	int n, k, map_size;
	int x, y;

	f = fopen(filename, "rb");
	if (f == NULL) return NULL;

	n = fread(buf, sizeof(char), DAT_SIZE, f);
	fclose(f);
	if (n != DAT_SIZE) return NULL;

	game = malloc(sizeof(KBgame));
	if (game == NULL) return NULL;

	/* Read out the save from "buf" */
	char *p = &buf[0];

	/* Player name */
	memcpy(game->name, p, 10);
	for (n = 9; n > 0; n--) {
		if (game->name[n] != ' ') break;
		game->name[n] = '\0';
	}
	p += 11;

	/* Main stats */
	game->class = READ_BYTE(p);
	game->rank = READ_BYTE(p);
	game->spell_power = READ_BYTE(p);
	game->max_spells = READ_BYTE(p);

	/* Quest progress */
	for (n = 0; n < MAX_VILLAINS; n++)
		game->villain_caught[n] = READ_BYTE(p);

	for (n = 0; n < MAX_ARTIFACTS; n++)
		game->artifact_found[n] = READ_BYTE(p);

	for (n = 0; n < MAX_CONTINENTS; n++)
		game->continent_found[n] = READ_BYTE(p);

	for (n = 0; n < MAX_CONTINENTS; n++)
		game->orb_found[n] = READ_BYTE(p);

	/* Spell book */
	for (n = 0; n < MAX_SPELLS; n++)
		game->spells[n] = READ_BYTE(p);

	/* More stats */
	game->knows_magic = READ_BYTE(p);
	game->siege_weapons = READ_BYTE(p);
	game->contract = READ_BYTE(p);

	for (n = 0; n < 5; n++)
		game->player_troops[n] = READ_BYTE(p);

	/* Options and difficulty setting */
	game->options[0] = READ_BYTE(p);//delay

	game->difficulty = READ_BYTE(p);

	game->options[1] = READ_BYTE(p);//sounds
	game->options[2] = READ_BYTE(p);//walk beep
	game->options[3] = READ_BYTE(p);//animation
	game->options[4] = READ_BYTE(p);//show army size

	/* Player location */
	game->continent = READ_BYTE(p);
	game->x = READ_BYTE(p);
	game->y = READ_BYTE(p);
	game->last_x = READ_BYTE(p);
	game->last_y = READ_BYTE(p);
	game->boat_x = READ_BYTE(p);
	game->boat_y = READ_BYTE(p);
	game->boat = READ_BYTE(p);
	game->mount = READ_BYTE(p);

	game->options[5] = READ_BYTE(p);//CGA palette

	/* Spells sold in towns */
	for (n = 0; n < MAX_TOWNS; n++)
		game->town_spell[n] = READ_BYTE(p);

	/* Town/Contract */
	for (n = 0; n < 5; n++)
		game->contract_cycle[n] = READ_BYTE(p);
	game->last_contract =  READ_BYTE(p);
	game->max_contract = READ_BYTE(p);

	/* Steps left till end of day */
	game->steps_left = READ_BYTE(p);

	/* Skip 1 byte */
	game->unknown3 = READ_BYTE(p);

	/* Castle ownership */
	for (n = 0; n < MAX_CASTLES; n++)
		game->castle_owner[n] = READ_BYTE(p);

	/* Visited castles and towns */
	for (n = 0; n < MAX_CASTLES; n++)
		game->castle_visited[n] = READ_BYTE(p);

	for (n = 0; n < MAX_TOWNS; n++)
		game->town_visited[n] = READ_BYTE(p);

	/* Scepter location (encrypted) */
	game->scepter_continent = READ_BYTE(p);
	game->scepter_x = READ_BYTE(p);
	game->scepter_y = READ_BYTE(p);

	/* Fog of war (convert from 1-bit-per-tile to 1-byte-per-tile format) */
	map_size = ( MAX_CONTINENTS * LEVEL_W * LEVEL_H );
	for (n = 0; n < MAX_CONTINENTS; n++) {
		for (y = 0; y < LEVEL_H; y++)
		for (x = 0; x < LEVEL_W; x += 8) {
			char test_bits = READ_BYTE(p);
			for (k = 0; k < 8; k++) {
				char one_bit = ((test_bits & (0x01 << k)) >> k) & 0x01;
				game->fog[n][y][x + 7 - k] = one_bit;
			}
		}
	}

	/* Waste AMIGA byte?! */
	READ_BYTE(p);

	/* Castle garrison troops */
	for (n = 0; n < MAX_CASTLES; n++)
		for (k = 0; k < 5; k++)
			game->castle_troops[n][k] = READ_BYTE(p);

	/* Read friendly foes' coords */
	for (n = 0; n < MAX_CONTINENTS; n++) {
		for (k = 0; k < FRIENDLY_FOES; k++) {
			game->foe_coords[n][k][0] = READ_BYTE(p);//X
			game->foe_coords[n][k][1] = READ_BYTE(p);//Y
		}
	}

	/* Map chests */
	for (n = 0; n < MAX_CONTINENTS - 1; n++) {
		game->map_coords[n][0] = READ_BYTE(p);//X
		game->map_coords[n][1] = READ_BYTE(p);//Y
	}

	/* Orb chests */
	for (n = 0; n < MAX_CONTINENTS; n++) {
		game->orb_coords[n][0] = READ_BYTE(p);//X
		game->orb_coords[n][1] = READ_BYTE(p);//Y
	}

	/* Waste another AMIGA byte?! */
	READ_BYTE(p);

	/* Teleporting caves */
	for (n = 0; n < MAX_CONTINENTS; n++) {
		for (k = 0; k < MAX_TELECAVES; k++) {
			game->teleport_coords[n][k][0] = READ_BYTE(p);//X
			game->teleport_coords[n][k][1] = READ_BYTE(p);//Y
		}
	}

	/* Dwellings locations */
	for (n = 0; n < MAX_CONTINENTS; n++) {
		for (k = 0; k < MAX_DWELLINGS; k++) {
			game->dwelling_coords[n][k][0] = READ_BYTE(p);//X
			game->dwelling_coords[n][k][1] = READ_BYTE(p);//Y
		}
	}

	/* Read hostile foes' coords */
	for (n = 0; n < MAX_CONTINENTS; n++) {
		for (k = FRIENDLY_FOES; k < MAX_FOES; k++) {
			game->foe_coords[n][k][0] = READ_BYTE(p);//X
			game->foe_coords[n][k][1] = READ_BYTE(p);//Y
		}
	}

	/* Read hostile foes' troops */
	for (n = 0; n < MAX_CONTINENTS; n++) {
		for (k = FRIENDLY_FOES; k < MAX_FOES; k++) {
			game->foe_troops[n][k][0] = READ_BYTE(p);
			game->foe_troops[n][k][1] = READ_BYTE(p);
			game->foe_troops[n][k][2] = READ_BYTE(p);
		}
	}

	/* Read foe numbers */
	for (n = 0; n < MAX_CONTINENTS; n++) {
		for (k = FRIENDLY_FOES; k < MAX_FOES; k++) {
			game->foe_numbers[n][k][0] = READ_BYTE(p);
			game->foe_numbers[n][k][1] = READ_BYTE(p);
			game->foe_numbers[n][k][2] = READ_BYTE(p);
		}
	}

	//READ_BYTE(p); //waste byte
	//READ_BYTE(p); //waste byte

	/* Read dwelling troop type and population count */
	for (n = 0; n < MAX_CONTINENTS; n++)
		for (k = 0; k < MAX_DWELLINGS; k++)
			game->dwelling_troop[n][k] = READ_BYTE(p);

	for (n = 0; n < MAX_CONTINENTS; n++)
		for (k = 0; k < MAX_DWELLINGS; k++)
			game->dwelling_population[n][k] = READ_BYTE(p);

	/* Read scepter key and un-encrypt the coordinates */
	//game->scepter_key = READ_BYTE(p);
	//game->scepter_continent ^= game->scepter_key;
	//game->scepter_x ^= game->scepter_key;
	//game->scepter_y ^= game->scepter_key;

	/* More stats */
	game->base_leadership = READ_WORD_BE(p);
	game->leadership = READ_WORD_BE(p);
	game->commission = READ_WORD_BE(p);
	game->followers_killed = READ_WORD_BE(p);

	/* Player army numbers */
	for (n = 0; n < 5; n++)
		game->player_numbers[n] = READ_WORD_BE(p);

	/* Castle garrison numbers */
	for (n = 0; n < MAX_CASTLES; n++)
		for (k = 0; k < 5; k++)
			game->castle_numbers[n][k] = READ_WORD_BE(p);

	/* More stats */
	game->time_stop = READ_WORD_BE(p);
	game->days_left = READ_WORD_BE(p);

	/* Score (unused) */
	game->score = READ_WORD_BE(p);

	/* Skip 2 bytes */
	game->unknown1 = READ_BYTE(p);
	game->unknown2 = READ_BYTE(p);

	/* Player's gold (32 bit, can get quite rich!) */
	game->gold = READ_DWORD_BE(p);

	/* Map dump */
	n = map_size;
	memcpy(game->map, p, n);
	p += n;

	if (p - &buf[0] != DAT_SIZE)
	fprintf(stdout, "Save file: %d bytes (needed %d)\n", p - &buf[0], DAT_SIZE);

	return game;
}


int KB_saveDAT(const char* filename, KBgame *game) {

	char buf[DAT_SIZE];
	FILE *f;
	int n, k, map_size;
	int x, y;

	f = fopen(filename, "wb");
	if (f == NULL) return 1;

	/* Layout the save in "buf" */
	char *p = &buf[0];

	/* Player name */
	memcpy(p, game->name, 10);
	for (n = strlen(game->name); n < 10; n++) {
		buf[n] = ' ';
	}
	p += 11;

	/* Main stats */
	WRITE_BYTE(p, game->class);
	WRITE_BYTE(p, game->rank);
	WRITE_BYTE(p, game->spell_power);
	WRITE_BYTE(p, game->max_spells);

	/* Quest progress */
	for (n = 0; n < MAX_VILLAINS; n++)
		WRITE_BYTE(p, game->villain_caught[n]);

	for (n = 0; n < MAX_ARTIFACTS; n++)
		WRITE_BYTE(p, game->artifact_found[n]);

	for (n = 0; n < MAX_CONTINENTS; n++)
		WRITE_BYTE(p, game->continent_found[n]);

	for (n = 0; n < MAX_CONTINENTS; n++)
		WRITE_BYTE(p, game->orb_found[n]);

	/* Spell book */
	for (n = 0; n < MAX_SPELLS; n++)
		WRITE_BYTE(p, game->spells[n]);

	/* More stats */
	WRITE_BYTE(p, game->knows_magic);
	WRITE_BYTE(p, game->siege_weapons);
	WRITE_BYTE(p, game->contract);

	for (n = 0; n < 5; n++)
		WRITE_BYTE(p, game->player_troops[n]);

	/* Options and difficulty setting */
	WRITE_BYTE(p, game->options[0]);//delay

	WRITE_BYTE(p, game->difficulty);

 	WRITE_BYTE(p, game->options[1]);//sounds
 	WRITE_BYTE(p, game->options[2]);//walk beep
 	WRITE_BYTE(p, game->options[3]);//animation
 	WRITE_BYTE(p, game->options[4]);//show army size

	/* Player location */
	WRITE_BYTE(p, game->continent);
	WRITE_BYTE(p, game->x);
	WRITE_BYTE(p, game->y);
	WRITE_BYTE(p, game->last_x);
	WRITE_BYTE(p, game->last_y);
	WRITE_BYTE(p, game->boat_x);
	WRITE_BYTE(p, game->boat_y);
	WRITE_BYTE(p, game->boat);
	WRITE_BYTE(p, game->mount);

	WRITE_BYTE(p, game->options[5]);//CGA palette

	/* Spells sold in towns */
	for (n = 0; n < MAX_TOWNS; n++)
		WRITE_BYTE(p, game->town_spell[n]);

	/* Town/Contract */
	for (n = 0; n < 5; n++)
		WRITE_BYTE(p, game->contract_cycle[n]);
	WRITE_BYTE(p, game->last_contract);
	WRITE_BYTE(p, game->max_contract);

	/* Steps left till end of day */
	WRITE_BYTE(p, game->steps_left);

	/* Skip 1 byte */
	WRITE_BYTE(p, game->unknown3);

	/* Castle ownership */
	for (n = 0; n < MAX_CASTLES; n++)
		WRITE_BYTE(p, game->castle_owner[n]);

	/* Visited castles and towns */
	for (n = 0; n < MAX_CASTLES; n++)
		WRITE_BYTE(p, game->castle_visited[n]);

	for (n = 0; n < MAX_TOWNS; n++)
		WRITE_BYTE(p, game->town_visited[n]);

	/* Scepter location (encrypted) */
	WRITE_BYTE(p, game->scepter_continent ^ game->scepter_key);
	WRITE_BYTE(p, game->scepter_x ^ game->scepter_key);
	WRITE_BYTE(p, game->scepter_y ^ game->scepter_key);

	/* Fog of war (convert from 1-byte-per-tile to 1-bit-per-tile format) */
	map_size = ( MAX_CONTINENTS * LEVEL_W * LEVEL_H );
	for (n = 0; n < MAX_CONTINENTS; n++) {
		for (y = 0; y < LEVEL_H; y++)
		for (x = 0; x < LEVEL_W; x += 8) {
			char one_byte = 0;
			for (k = 0; k < 8; k++) {
				char one_bit = (game->fog[n][y][x + 7 - k] & 0x01) << k;
				one_byte |= one_bit;
			}
			WRITE_BYTE(p, one_byte);
		}
	}

	/* Castle garrison troops */
	for (n = 0; n < MAX_CASTLES; n++)
		for (k = 0; k < 5; k++)
			WRITE_BYTE(p, game->castle_troops[n][k]);

	/* Read friendly foes' coords */
	for (n = 0; n < MAX_CONTINENTS; n++) {
		for (k = 0; k < FRIENDLY_FOES; k++) {
			WRITE_BYTE(p, game->foe_coords[n][k][0]);//X
			WRITE_BYTE(p, game->foe_coords[n][k][1]);//Y
		}	
	}

	/* Map chests */
	for (n = 0; n < MAX_CONTINENTS - 1; n++) {
		WRITE_BYTE(p, game->map_coords[n][0]);//X
		WRITE_BYTE(p, game->map_coords[n][1]);//Y
	}

	/* Orb chests */
	for (n = 0; n < MAX_CONTINENTS; n++) {
		WRITE_BYTE(p, game->orb_coords[n][0]);//X
		WRITE_BYTE(p, game->orb_coords[n][1]);//Y
	}

	/* Teleporting caves */
	for (n = 0; n < MAX_CONTINENTS; n++) {
		for (k = 0; k < MAX_TELECAVES; k++) {
			WRITE_BYTE(p, game->teleport_coords[n][k][0]);//X
			WRITE_BYTE(p, game->teleport_coords[n][k][1]);//Y
		}
	}

	/* Dwellings locations */
	for (n = 0; n < MAX_CONTINENTS; n++) {
		for (k = 0; k < MAX_DWELLINGS; k++) {
			WRITE_BYTE(p, game->dwelling_coords[n][k][0]);//X
			WRITE_BYTE(p, game->dwelling_coords[n][k][1]);//Y
		}
	}

	/* Hostile foes' coords */
	for (n = 0; n < MAX_CONTINENTS; n++) {
		for (k = FRIENDLY_FOES; k < MAX_FOES; k++) {
			WRITE_BYTE(p, game->foe_coords[n][k][0]);//X
			WRITE_BYTE(p, game->foe_coords[n][k][1]);//Y
		}
	}

	/* Hostile foes' troops */
	for (n = 0; n < MAX_CONTINENTS; n++) {
		for (k = FRIENDLY_FOES; k < MAX_FOES; k++) {
			WRITE_BYTE(p, game->foe_troops[n][k][0]);
			WRITE_BYTE(p, game->foe_troops[n][k][1]);
			WRITE_BYTE(p, game->foe_troops[n][k][2]);
		}
	}

	/* Foes' army counts */
	for (n = 0; n < MAX_CONTINENTS; n++) {
		for (k = FRIENDLY_FOES; k < MAX_FOES; k++) {
			WRITE_BYTE(p, game->foe_numbers[n][k][0]);
			WRITE_BYTE(p, game->foe_numbers[n][k][1]);
			WRITE_BYTE(p, game->foe_numbers[n][k][2]);
		}
	}

	/* Dwelling troop type and population count */
	for (n = 0; n < MAX_CONTINENTS; n++)
		for (k = 0; k < MAX_DWELLINGS; k++)
			WRITE_BYTE(p, game->dwelling_troop[n][k]);

	for (n = 0; n < MAX_CONTINENTS; n++)
		for (k = 0; k < MAX_DWELLINGS; k++)
			WRITE_BYTE(p, game->dwelling_population[n][k]);

	/* Write scepter key */
	WRITE_BYTE(p, game->scepter_key);

	/* More stats */
	WRITE_WORD(p, game->base_leadership);
	WRITE_WORD(p, game->leadership);
	WRITE_WORD(p, game->commission);
	WRITE_WORD(p, game->followers_killed);

	/* Player army numbers */
	for (n = 0; n < 5; n++)
		WRITE_WORD(p, game->player_numbers[n]);

	/* Castle garrison numbers */
	for (n = 0; n < MAX_CASTLES; n++)
		for (k = 0; k < 5; k++)
			WRITE_WORD(p, game->castle_numbers[n][k]);

	/* More stats */
	WRITE_WORD(p, game->time_stop);
	WRITE_WORD(p, game->days_left);

	/* Score (unused) */
	WRITE_WORD(p, game->score);

	/* Skip 2 bytes */
	WRITE_BYTE(p, game->unknown1);
	WRITE_BYTE(p, game->unknown2);

	/* Player's gold (32 bit, can get quite rich!) */
	WRITE_DWORD(p, game->gold);

	/* Map dump */
	n = map_size; 
	memcpy(p, game->map, n);
	p += n;

	/** DONE **/
	n = fwrite(buf, sizeof(char), DAT_SIZE, f);

	/* issue #9 遊戲調整設定:寫在 DAT_SIZE 本體之後的追加區塊(magic byte + 6 個設定 byte)。
	 * 不佔用/不改 buf 版面,DAT_SIZE 與原版存檔格式不變;舊版 openkb / 原版 DOS 讀我們存的檔
	 * 一樣只會讀到前 DAT_SIZE bytes,不受影響。 */
	if (n == DAT_SIZE) {
		byte opt_block[1 + KB_OPT_COUNT];
		opt_block[0] = KB_OPT_MAGIC;
		opt_block[1] = game->opt_no_wages;
		opt_block[2] = game->opt_ai_mode;
		opt_block[3] = game->opt_days_x2;
		opt_block[4] = game->opt_foe_freq;
		opt_block[5] = game->opt_foe_strength;
		opt_block[6] = game->opt_recruit_caps;
		fwrite(opt_block, sizeof(char), 1 + KB_OPT_COUNT, f);
	}

	fclose(f);

	/* Wrong filesize */
	if (n != DAT_SIZE) return 2;

	/* Success */
	return 0;
}
