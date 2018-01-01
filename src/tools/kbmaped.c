#include "../bounty.h"
#include "../play.h"
#include "../save.h"
#include "../lib/kbconf.h"
#include "../lib/kbres.h"
#include "../lib/kbauto.h"
#include "../lib/kbstd.h"

#include "../env.h"
struct KBconfig KBconf;
struct KBenv *sys;
int filemode = 0;

KBgame* load_land(const char *filename) {

	KBgame *game;
	FILE *f;
	int map_size, n;

	game = malloc(sizeof(KBgame));
	if (game == NULL) return NULL;

	map_size = ( MAX_CONTINENTS * LEVEL_W * LEVEL_H );
	memset(game->map, 0, sizeof(char) * map_size);

	game->map[0][0][0] = 0xFF;
	game->map[1][0][0] = 0xFF;
	game->map[2][0][0] = 0xFF;
	game->map[3][0][0] = 0xFF;

	KB_strncpy(game->savefile, filename, 64);

	f = fopen(filename, "rb");
	if (f) {

		n = fread(game->map, sizeof(char), map_size, f);
		if (n != map_size) {
			printf("Read %d bytes of %d expected\n", n, map_size);
		}
		fclose(f);

	} else {
		printf("Unable to OPEN file %s\n", filename);
		free(game);
		return NULL;
	}

	return game;
}

KBmodule* preload_module(const char *datadir) {

	wipe_config(&KBconf);
	read_env_config(&KBconf);

	strcpy(KBconf.data_dir, datadir);
	KBconf.autodiscover = 1;

	KBconfig *conf = &KBconf; //shortcut

	/* Module auto-discovery */
	if (conf->autodiscover)
		discover_modules(conf->data_dir, conf);

	/* No module! (Unlikely...) */
	if (conf->num_modules == 0) {
		KB_errlog("No modules found.\n");
		return NULL;
	}

	/* Initialize module(s) */
	init_module(&conf->modules[1]);

	return &conf->modules[1];
}

void interactive_edit(KBgame *game) {

}

void interactive_main(KBgame *game) {

	/* Read default config file */
	wipe_config(&KBconf);
	read_env_config(&KBconf);
	//temp hack to always read local file
	KB_strcpy(KBconf.config_file, "../../openkb.ini");
	if ( read_file_config(&KBconf, KBconf.config_file) )
	{
		KB_errlog("[config] Unable to read config file '%s'\n", KBconf.config_file); 
		return;
	}

	/* Output final config to stdout */
	KB_stdlog("\n");
	report_config(&KBconf);
	KB_stdlog("\n");

	KBconfig *conf = &KBconf; //shortcut

	/* Hack? Pretend to use normal2x */
	conf->filter = 1;

	/* Start new environment (game window) */
	sys = KB_startENV(conf);

	/* That's clearly wrong: */
	conf->filter = 0;

	/* Must be successfull to continue */
	if (!sys) {
		return;
	}

	/* Module auto-discovery */
	if (conf->autodiscover)
		discover_modules(conf->data_dir, conf);

	/* --- ! ! ! --- */
	conf->module = 0;

	/* No module! (Unlikely...) */
	if (conf->num_modules == 0) {
		KB_errlog("No modules found.\n");
		return;
	}

	/* Initialize module(s) */
	init_modules(conf);

	/* Load and use module font */
	sys->font = KB_LoadIMG8(GR_FONT, 0);
	infont(sys->font);

	/* --- X X X --- */
	KB_stdlog("=====================================================\n");
	/* Run editor loop */
	interactive_edit(game);

	/* Kill modules */
	stop_modules(conf);
	/* --- @ @ @ --- */
	KB_stopENV(sys);
}

extern void* DOS_Resolve(KBmodule *mod, int id, int sub_id);

char *sign_split(char *src) {
	strtok(src, "\n");
	char *s = strtok(NULL,"\n");
	if (!s) return "";
	return s;
}

void stdout_game_tmx(KBgame *game, KBmodule *mod) {
	char *signs = mod ? DOS_Resolve(mod, STRL_SIGNS, 0) : NULL;

	int i;
	int x, y, continent = 0;
printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
printf("<map version=\"1.0\" orientation=\"orthogonal\" width=\"64\" height=\"64\" tilewidth=\"48\" tileheight=\"36\">\n");
printf(" <tileset firstgid=\"1\" name=\"tileseta\" tilewidth=\"48\" tileheight=\"34\">\n");
printf("  <image source=\"tileseta.png\" trans=\"ffffff\" width=\"1728\" height=\"34\"/>\n");
printf(" </tileset>\n");
printf(" <tileset firstgid=\"37\" name=\"tilesetb\" tilewidth=\"48\" tileheight=\"34\">\n");
printf("  <image source=\"tilesetb.png\" trans=\"ffffff\" width=\"1728\" height=\"34\"/>\n");
printf(" </tileset>\n");
	for (continent = 0; continent < MAX_CONTINENTS; continent++) {
printf(" <layer name=\"Continent %d\" width=\"64\" height=\"64\" visible=\"%d\">\n", continent, continent==0?1:0);
printf("  <properties>\n");
printf("   <property name=\"Name\" value=\"%s\"/>\n", continent_names[continent]);
printf("  </properties>\n");
printf("  <data>\n");
	for (y = 0; y < 64; y++) {
	for (x = 0; x < 64; x++) {
		byte tile = game->map[continent][63-y][x];
		if (tile == TILE_SIGNPOST && signs) tile = TILE_GRASS;
		tile &= 0x7F;
printf("   <tile gid=\"%d\"/>\n", tile + 1);
	}
	}
printf("  </data>\n");
printf(" </layer>\n");
	}
	int sign_id = -1;
	for (continent = 0; continent < MAX_CONTINENTS; continent++) {
//hardcoded GIDs?? TODO: figure this out
// Signposts
printf(" <objectgroup name=\"Signposts %d\" width=\"64\" height=\"64\" visible=\"%d\">\n", continent, continent==0?1:0);
printf("   <properties>\n");
printf("     <property name=\"continent\" value=\"%d\"/>\n", continent);
printf("   </properties>\n");
		if (signs) {
		for (y = 0; y < 64; y++) {
		for (x = 0; x < 64; x++) {
			byte tile = game->map[continent][y][x];
			int py = 64-y;
			if (tile == TILE_SIGNPOST) {
				sign_id++;
				char *sign = KB_strlist_peek(signs, sign_id);
				char *line1 = strdup(sign);
				char *line2 = sign_split(line1);
printf("   <object type=\"Signpost\" gid=\"17\" x=\"%d\" y=\"%d\">\n", x*48, py*36);
printf("     <properties>\n");
printf("       <property name=\"Text1\" value=\"%s\"/>\n", line1); // WHERE IS IT?!
printf("       <property name=\"Text2\" value=\"%s\"/>\n", line2);
printf("     </properties>\n");
printf("   </object>\n");
				free(line1);
			}
		}
		}
		}
printf(" </objectgroup>\n");
// Rest of them
printf(" <objectgroup name=\"Locations %d\" width=\"64\" height=\"64\" visible=\"%d\">\n", continent, continent==0?1:0);
printf("   <properties>\n");
printf("     <property name=\"continent\" value=\"%d\"/>\n", continent);
printf("   </properties>\n");
		x = continent_entry[continent][0];
		y = 64-continent_entry[continent][1];
printf("   <object type=\"Nav\" gid=\"33\" x=\"%d\" y=\"%d\"/>\n", x*48, y*36);
		if (special_coords[SP_ALCOVE][0] == continent) {
			x = special_coords[SP_ALCOVE][1];
			y = 64-special_coords[SP_ALCOVE][2];
printf("   <object type=\"Magic Alcove\" gid=\"14\" x=\"%d\" y=\"%d\">\n", x*48, y*36);
printf("     <properties>\n");
printf("       <property name=\"Alcove\" value=\"%s\"/>\n", "yes");
printf("     </properties>\n");
printf("   </object>\n");
		}
		if (special_coords[SP_HOME][0] == continent) {
			x = special_coords[SP_HOME][1];
			y = 64-special_coords[SP_HOME][2];
printf("   <object type=\"Castle\" name=\"Home\" gid=\"6\" x=\"%d\" y=\"%d\">\n", x*48, y*36);
printf("     <properties>\n");
printf("       <property name=\"Home\" value=\"%s\"/>\n", "yes");
printf("     </properties>\n");
printf("   </object>\n");
		}
	for (i = 0; i < MAX_CASTLES; i++) {
		if (castle_coords[i][0] != continent) continue;
		x = castle_coords[i][1];
		y = 64-castle_coords[i][2];
printf("   <object type=\"Castle\" name=\"%s\" gid=\"6\" x=\"%d\" y=\"%d\"/>\n", castle_names[i], x*48, y*36);
	}

	int boatoffx, boatoffy, gateoffx, gateoffy;
	int castle_id;
	for (castle_id = 0; castle_id < MAX_CASTLES; castle_id++) {
		i = town_inversion[castle_id];
		if (town_coords[i][0] != continent) continue;
		x = town_coords[i][1];
		y = 64-town_coords[i][2];
		boatoffx = boat_coords[i][1] - x;
		boatoffy = (64-boat_coords[i][2]) - y;
		gateoffx = towngate_coords[i][1] - x;
		gateoffy = (64-towngate_coords[i][2]) - y;
printf("   <object type=\"Town\" name=\"%s\" gid=\"11\" x=\"%d\" y=\"%d\">\n", town_names[i], x*48, y*36);
printf("     <properties>\n");
printf("       <property name=\"Castle\" value=\"%s\"/>\n", castle_names[castle_id]);
		if (boatoffx || boatoffy) {
printf("       <property name=\"BoatX\" value=\"%d\"/>\n", boatoffx);
printf("       <property name=\"BoatY\" value=\"%d\"/>\n", boatoffy);
		}
		if (gateoffx || gateoffy) {
printf("       <property name=\"GateX\" value=\"%d\"/>\n", gateoffx);
printf("       <property name=\"GateY\" value=\"%d\"/>\n", gateoffy);
		}
printf("     </properties>\n");
printf("   </object>\n");
	}
printf(" </objectgroup>\n");
}
printf("</map>\n");
}

void stdout_game(KBgame *game) {
	int cont, k, j, i;

	#define _TOTAL 4
	int num_hotspots[5] = { 0 };
	int num_foes[5] = { 0 };
	int num_grass[5] = { 0 };
	int num_castles[5] = { 0 };
	int num_towns[5] = { 0 };

	printf("Difficulty: %d\n", game->difficulty);
	printf("Scepter: %d, X=%d, Y=%d\n", game->scepter_continent, game->scepter_x, game->scepter_y);

	printf("Player: %s the %s\n", game->name, "Player");
	printf("Player: Level %d\n", game->rank);
	printf("Player: Mount - %d\n", game->mount);

	printf("Player: Location: Continent %d, X=%d, Y=%d\n", game->continent, game->x, game->y);
	printf("Player: Visited: Continent %d, X=%d, Y=%d\n", game->continent, game->last_x, game->last_y);

	printf("Boat: Exists: %s\n", game->boat != 0xFF ? "YES" : "NO");
	printf("Boat: Location: Continent %d, X=%d, Y=%d\n", game->boat, game->boat_x, game->boat_y);

	printf("Player: Siege Weapons: %s\n", game->siege_weapons ? "YES" : "NO");
	printf("Player: Knows Magic: %s\n", game->knows_magic ? "YES" : "NO");
	printf("Player: Owns Boat: %s\n", game->boat != 0xFF ? "YES" : "NO");

	printf("Artifcats: ");
	for (i = 0; i < MAX_ARTIFACTS; i++) {
		printf("[%c] ", game->artifact_found[i] ? 'X' : ' ');
	}
	printf("\n");
	
	printf("Villains: ");
	for (i = 0; i < MAX_VILLAINS; i++) {
		printf("[%c] ", game->villain_caught[i] ? 'X' : ' ');
	}
	printf("\n");

	printf("NavMaps: ");
	for (i = 0; i < MAX_CONTINENTS; i++) {
		printf("[%c] ", game->continent_found[i] ? 'X' : ' ');
	}
	for (i = 0; i < MAX_CONTINENTS; i++) {
		printf("%d,%d ", game->map_coords[i][0], game->map_coords[i][1]);
	}
	printf("\n");
	
	printf("MapOrbs: ");
	for (i = 0; i < MAX_CONTINENTS; i++) {
		printf("[%c] ", game->orb_found[i] ? 'X' : ' ');
	}
	for (i = 0; i < MAX_CONTINENTS; i++) {
		printf("%d,%d ", game->orb_coords[i][0], game->orb_coords[i][1]);
	}
	printf("\n");

	printf("Telecaves: ");
	for (i = 0; i < MAX_CONTINENTS; i++) {
		printf("[%d] ", i);
		printf("%d,%d->", game->teleport_coords[i][0][0], game->teleport_coords[i][0][1]);
		printf("%d,%d  ", game->teleport_coords[i][1][0], game->teleport_coords[i][1][1]);
	}
	printf("\n");

	for (cont = 0; cont < MAX_CONTINENTS; cont++) {
		printf("Continent %d: \n", cont);
		for (j = LEVEL_H - 1; j >= 0; j--) {
			printf("Map%d,%02d:", cont, j);
			for (i = 0; i < LEVEL_W; i++) {
				byte m = game->map[cont][j][i]; 
				char c = '?';

				/* Stats */
				if (m == 0x00) num_grass[cont]++;
				if (m == 0x85) num_castles[cont]++;
				if (m == 0x8A) num_towns[cont]++;
				if (m == 0x8B) num_hotspots[cont]++;
				if (m == 0x91) num_foes[cont]++;
				/* End stats */


				if (IS_WATER(m)) c = '~';
				if (IS_GRASS(m)) c = '.';
				if (IS_DESERT(m)) c = ',';
				if (IS_ROCK(m)) c = '#';
				if (IS_CASTLE(m)) c = '^';
				if (IS_TREE(m)) c = '*';
				if (IS_MAPOBJECT(m)) c = '!';
				if (IS_INTERACTIVE(m)) c = 'X';

				if (game->scepter_continent == cont && 
					i == game->scepter_x &&
					j == game->scepter_y) c = 'O';
				if (game->boat == cont && 
					i == game->boat_x &&
					j == game->boat_y) c = 'B';

				printf("%c", c);
			}
			printf("\n");
		}
	}

	for (cont = 0; cont < MAX_CONTINENTS+1; cont++) {
		printf("Stats,%s %d: ", cont == MAX_CONTINENTS ? "Total" : "Content", cont);
		printf("Castles: %d, Towns: %d, Hotspots: %d, Foes: %d, Grass: %d\n",
			num_castles[cont], num_towns[cont], num_hotspots[cont], num_foes[cont], num_grass[cont]);
		num_castles[_TOTAL] += num_castles[cont];
		num_towns[_TOTAL] += num_towns[cont];
		num_hotspots[_TOTAL] += num_hotspots[cont];
		num_foes[_TOTAL] += num_foes[cont];
		num_grass[_TOTAL] += num_grass[cont];
	}

	for (cont = 0; cont < MAX_CONTINENTS; cont++) {
		printf("Dwellings %d: ", cont);
		for (i = 0; i < MAX_DWELLINGS; i++) {
			printf("%02x ",
				game->dwelling_troop[cont][i] // type
			);
		}
		printf("| ");
		for (i = 0; i < MAX_DWELLINGS; i++) {
			printf("%d,%d ",
				game->dwelling_coords[cont][i][0],//x
				game->dwelling_coords[cont][i][1]//y
			);
		}
		printf("\n");
	}

//#define TEST_SCEPTER_TILES 0x58 //AX at 0788:0A8B
#ifdef TEST_SCEPTER_TILES
	for (cont = 0; cont < MAX_CONTINENTS; cont++) {
		printf("Puzzle%d: ", cont);
		int gtiles = TEST_SCEPTER_TILES;
		for (j = 0; j < LEVEL_H; j++) {
			for (i = 0; i < LEVEL_W; i++) {
				if (!(game->map[cont][j][i] & 0x7F)) {//no "tiling" bits sets
					gtiles--;
					if (gtiles == 0) {
						printf("X=%d, Y=%d\n", i, j);
						break;
					}
				}

			}
			if (gtiles == 0) break;
		}
		if (gtiles != 0) {
			printf("Not found\n");
		}
	}
#endif

	char *names[] = { 
		"Mury","Hack","Ammi","Baro","Drea","Cane","Mora","Barr",
		"Barg","Rina","Ragf","Mahk","Auri","Czar","Magu","Urth","Arec" };

	char *troops[] = {
		"Peasants","Sprites","Milita","Wolfs","Skeletons","Zombies","Gnomes","Orcs",
		"Archers","Elves","Pikemen","Nomads","Dwarves","Ghosts","Knights","Ogres", 
		"Barbarians","Trolls","Cavalery","Druids","Arcmages","Vampires","Giants",
		"Demons","Dragons" };

	for (j = 0; j < MAX_VILLAINS; j++) {
		printf("Villain%d: %s [%s]\n", j, names[j], game->villain_caught[j] ? "X" : " ");
		if (game->villain_caught[j]) continue;
		for (k = 0; k < MAX_CASTLES; k++) {
			if ((game->castle_owner[k] & 0x7F) == j) {
				/* Army */
				for (i = 0; i < 5; i++) {
					if (game->castle_numbers[k][i] == 0) break;
					printf("Villain%d: Castle%2d: Troop%d: %02d x %s\n", j, k, i, game->castle_numbers[k][i], troops[game->castle_troops[k][i]]);
				}
				break;
			}
		}
	}
	
	for (k = 0; k < MAX_CASTLES; k++) {
		//if ((game->castle_owner[k] & 0x7F) == j) {
			/* Army */
			for (i = 0; i < 5; i++) {
				if (game->castle_numbers[k][i] == 0) break;
				printf("Monster Castle%2d: Troop%d: %02d x %s\n", k, i, game->castle_numbers[k][i], troops[game->castle_troops[k][i]]);
			}
		//}
	}
	
}

int main(int argc, char* argv[]) {
	KB_logto_NULL();

	int be_interactive = 1;
	int file_is_land = 0;
	int dump_type = -1;
	int i;

	KBgame *game;
	char *filename;

	KBmodule *module = NULL;

	if (argc < 3) {
	
		printf("Usage: ./kbmaped [OPTION] SAVEFILE\n");
		printf("OPTIONS are:\n");
		printf("\t--dump\t Print textual representation of SAVEFILE and exit\n");
		printf("\t--tmx\t Print TMX representation of SAVEFILE and exit\n");
		//printf("\t--cheat\t Fill target SAVEFILE with dragons\n");
		printf("\t--land\t Treat SAVEFILE as land.org\n");
	
		return -1;
	}

	filename = argv[argc-1];

	for (i = 1; i < argc-1; i++) {

		if (!strcasecmp(argv[i], "--land")) {

			file_is_land = 1;

		}
		if (!strcasecmp(argv[i], "--dump")) {

			be_interactive = 0;
			dump_type = 0;

		}
	
		if (!strcasecmp(argv[i], "--tmx")) {

			be_interactive = 0;
			dump_type = 1;

		}

		if (!strcasecmp(argv[i], "--datadir")) {

			module = preload_module(argv[i+1]);

			if (!module) {
				return -1;
			}

		}

	}


	if (file_is_land) {

		game = load_land(filename);

	}
	else {

		game = KB_loadDAT(filename);

	}

	if (game == NULL) {

		printf("Unable to load game %s\n", argv[2]);

		return -2;

	}

	if (dump_type == 0) {

		stdout_game(game);

	}
	if (dump_type == 1) {

		stdout_game_tmx(game, module);

	}

	if (be_interactive) {

		interactive_main(game);

	}

	free(game);

	if (module) {

		stop_module(module);

	}

	return 0;
}
