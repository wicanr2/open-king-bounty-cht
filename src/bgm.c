/*
 *  bgm.c -- 背景音樂子系統 (SDL2_mixer)
 *  見 bgm.h。音樂版本目錄結構:<music_root>/<version>/scenes.ini + *.ogg。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include <SDL_mixer.h>

#include "bgm.h"
#include "lib/kbstd.h"

#define MAX_VERSIONS 8
#define BGMPATH 1024

/* 場景 key (對應 scenes.ini 左側;index = enum 值) */
static const char *scene_key[BGM_SCENE_MAX] = {
	"title", "field1", "field2", "field3", "field4",
	"castle", "town", "combat", "siege", "win", "lose", "ending"
};

static int   bgm_ready = 0;
static char  music_root[BGMPATH] = {0};
static char  versions[MAX_VERSIONS][32];
static int   n_versions = 0;
static int   cur_version = 0;          /* 0 = 關閉;1..n = versions[idx-1] */
static char  track_file[BGM_SCENE_MAX][BGMPATH]; /* 目前版本各場景的音檔絕對路徑 ("" = 無) */
static int   cur_scene = BGM_NONE;
static Mix_Music *cur_music = NULL;

static int dir_has_music(const char *dir) {
	/* 視為音樂版本目錄:存在 scenes.ini 或任一 .ogg。用 scenes.ini 為主判斷。 */
	char p[BGMPATH];
	FILE *f;
	snprintf(p, sizeof(p), "%s/scenes.ini", dir);
	f = fopen(p, "r");
	if (f) { fclose(f); return 1; }
	return 0;
}

/* 掃 music_root 下的版本子目錄 (用已知名單,存在才加入) */
static void scan_versions(void) {
	static const char *known[] = { "fmtowns", "dos", "genesis", "amiga", "pc98", "apple2", NULL };
	int i;
	n_versions = 0;
	for (i = 0; known[i] && n_versions < MAX_VERSIONS; i++) {
		char d[BGMPATH];
		snprintf(d, sizeof(d), "%s/%s", music_root, known[i]);
		if (dir_has_music(d)) {
			KB_strncpy(versions[n_versions], known[i], sizeof(versions[0]));
			n_versions++;
		}
	}
}

/* 讀某版本的 scenes.ini → track_file[];格式: scene_key = filename.ogg */
static void load_mapping(const char *ver) {
	char inipath[BGMPATH], line[BGMPATH];
	FILE *f;
	int i;
	for (i = 0; i < BGM_SCENE_MAX; i++) track_file[i][0] = '\0';
	snprintf(inipath, sizeof(inipath), "%s/%s/scenes.ini", music_root, ver);
	f = fopen(inipath, "r");
	if (!f) return;
	while (fgets(line, sizeof(line), f)) {
		char key[64], val[256];
		if (line[0] == ';' || line[0] == '#' || line[0] == '[') continue;
		if (sscanf(line, " %63[^= ] = %255[^\r\n]", key, val) == 2) {
			for (i = 0; i < BGM_SCENE_MAX; i++)
				if (!KB_strcasecmp(key, scene_key[i])) {
					/* 去尾端空白 */
					int e = strlen(val); while (e > 0 && (val[e-1]==' '||val[e-1]=='\t')) val[--e]='\0';
					snprintf(track_file[i], BGMPATH, "%s/%s/%s", music_root, ver, val);
					break;
				}
		}
	}
	fclose(f);
}

int KB_bgm_init(const char *install_dir, const char *data_dir) {
	/* 候選 music root (依序試,找到含版本子目錄者即用)。
	 * roots[] 直接是 music root;另對 install/data dir 試其下 /music 與兄弟 ../music。 */
	char roots[8][BGMPATH];
	int nr = 0, i;
	const char *env = getenv("KB_MUSIC");
	if (env && env[0])        KB_strncpy(roots[nr++], env, BGMPATH);
	if (install_dir && install_dir[0]) snprintf(roots[nr++], BGMPATH, "%s/music", install_dir);
	if (data_dir && data_dir[0]) {
		snprintf(roots[nr++], BGMPATH, "%s/music", data_dir);     /* <data>/music */
		snprintf(roots[nr++], BGMPATH, "%s/../music", data_dir);  /* 與 data 同層 */
	}
	KB_strncpy(roots[nr++], "music", BGMPATH);   /* cwd/music */
	KB_strncpy(roots[nr++], ".", BGMPATH);       /* cwd 本身就是 music root */

	for (i = 0; i < nr; i++) {
		KB_strncpy(music_root, roots[i], sizeof(music_root));
		scan_versions();
		if (n_versions > 0) break;
	}
	if (n_versions == 0) {
		KB_stdlog("BGM: no music versions found (背景音樂關閉)\n");
		return 0;
	}
	KB_stdlog("BGM: found %d music version(s) at '%s' (first='%s')\n", n_versions, music_root, versions[0]);

	if (Mix_Init(MIX_INIT_OGG) == 0) {
		KB_errlog("BGM: Mix_Init failed: %s\n", Mix_GetError());
		return 0;
	}
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
		KB_errlog("BGM: Mix_OpenAudio failed: %s\n", Mix_GetError());
		return 0;
	}
	Mix_VolumeMusic(MIX_MAX_VOLUME * 3 / 4);
	bgm_ready = 1;
	cur_version = 1;               /* 預設開啟第一個版本 */
	load_mapping(versions[0]);
	KB_stdlog("BGM: %d music version(s) at '%s'; default '%s'\n", n_versions, music_root, versions[0]);
	return 1;
}

void KB_bgm_shutdown(void) {
	if (!bgm_ready) return;
	if (cur_music) { Mix_HaltMusic(); Mix_FreeMusic(cur_music); cur_music = NULL; }
	Mix_CloseAudio();
	Mix_Quit();
	bgm_ready = 0;
}

void KB_bgm_scene(int scene) {
	if (!bgm_ready || cur_version == 0) return;
	if (scene < 0 || scene >= BGM_SCENE_MAX) return;
	if (scene == cur_scene && cur_music && Mix_PlayingMusic()) return; /* 同場景持續播 */
	cur_scene = scene;
	if (track_file[scene][0] == '\0') { /* 此場景無對應音軌 → 停 */
		if (cur_music) { Mix_HaltMusic(); Mix_FreeMusic(cur_music); cur_music = NULL; }
		return;
	}
	if (cur_music) { Mix_HaltMusic(); Mix_FreeMusic(cur_music); cur_music = NULL; }
	cur_music = Mix_LoadMUS(track_file[scene]);
	if (!cur_music) { KB_errlog("BGM: load '%s' failed: %s\n", track_file[scene], Mix_GetError()); return; }
	Mix_PlayMusic(cur_music, -1); /* 循環 */
}

const char *KB_bgm_cycle_version(void) {
	if (!bgm_ready) return "OFF";
	cur_version = (cur_version + 1) % (n_versions + 1); /* 0=off, 1..n */
	if (cur_version == 0) {
		if (cur_music) { Mix_HaltMusic(); Mix_FreeMusic(cur_music); cur_music = NULL; }
	} else {
		load_mapping(versions[cur_version - 1]);
		{ int s = cur_scene; cur_scene = BGM_NONE; KB_bgm_scene(s); } /* 用新版本重播當前場景 */
	}
	return KB_bgm_version_name();
}

const char *KB_bgm_version_name(void) {
	if (!bgm_ready || cur_version == 0) return "OFF";
	return versions[cur_version - 1];
}
