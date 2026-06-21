/*
 *  bgm.h -- 背景音樂子系統 (openkb 中文化,F9 多版本音樂)
 *
 *  openkb 原本只有 SFX 觸發,無場景背景音樂。本模組用 SDL2_mixer 播放各版本
 *  (FM Towns CDDA 等) 的 OGG 音軌,依「場景」循環播放,F9 切換音樂版本。
 *  音樂素材受版權保護,僅隨個人版打包,不入公開散布。
 */
#ifndef OPENKB_BGM_H
#define OPENKB_BGM_H

/* 場景 (對應 openkb 的主要狀態;index 用於 scene→track 對照表) */
enum {
	BGM_NONE = -1,
	BGM_TITLE = 0,   /* 標題 / 選角 / 製作名單 */
	BGM_FIELD0,      /* 大陸 1 Continentia */
	BGM_FIELD1,      /* 大陸 2 Forestria */
	BGM_FIELD2,      /* 大陸 3 Archipelia */
	BGM_FIELD3,      /* 大陸 4 Saharia */
	BGM_CASTLE,      /* 城堡 */
	BGM_TOWN,        /* 鄉鎮 */
	BGM_COMBAT,      /* 一般戰鬥 */
	BGM_SIEGE,       /* 攻城戰 */
	BGM_WIN,         /* 勝利 */
	BGM_LOSE,        /* 戰敗 */
	BGM_ENDING,      /* 結局 */
	BGM_SCENE_MAX
};

/* 初始化 (開啟 mixer + 偵測音樂版本目錄)。回傳 1 成功。 */
int  KB_bgm_init(const char *install_dir, const char *data_dir);
void KB_bgm_shutdown(void);

/* 播放某場景的 BGM (循環);同場景重複呼叫不會重啟。版本無此場景則靜音。 */
void KB_bgm_scene(int scene);

/* F9:循環切換到下一個可用音樂版本 (含「關閉」)。回傳目前版本名 (顯示用)。 */
const char *KB_bgm_cycle_version(void);
const char *KB_bgm_version_name(void);

#endif
