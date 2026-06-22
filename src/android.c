/*
 *  android.c -- Android 啟動橋接 (僅 __ANDROID__ 編入)
 *
 *  Android 沒有命令列傳 -c config / --rootdir,APK 的 cwd 也是唯讀。
 *  資產 (data/free、cjk24.bin) 由 Java 端 (OpenKBActivity) 於首次啟動
 *  從 APK assets/ 複製到內部儲存 getFilesDir()。本檔在 main() 早期被呼叫,
 *  把 KBconfig 的 datadir/savedir 指向內部儲存,並程式化加入 free 模組,
 *  取代桌面版「讀 config 檔 + 命令列」那條路。引擎其餘流程不變。
 */
#ifdef __ANDROID__

#include "sdlcompat.h"      /* SDL */
#include "lib/kbconf.h"     /* KBconfig, KBFAMILY_GNU, C_data_dir... */
#include "lib/kbstd.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

extern void add_module_aux(KBconfig *conf, const char *name, int family, int bpp,
                           const char *path, const char *slotA, const char *slotB, const char *slotC);

/* 內部儲存根目錄 (SDL 提供;= getFilesDir())。回傳的字串由 SDL 管理。 */
static const char *android_base(void) {
	const char *p = SDL_AndroidGetInternalStoragePath();
	return p ? p : ".";
}

/* 設定 Android 上的 KBconfig:資料/存檔走內部儲存,直接掛 free 模組。
 * Java 端已把 APK assets/ 下的 data/ (含 free/ 與 cjk24.bin) 複製到 base。*/
void android_bootstrap(KBconfig *conf)
{
	const char *base = android_base();
	char datadir[PATH_LEN];
	char savedir[PATH_LEN];
	char freedir[PATH_LEN];

	snprintf(datadir, sizeof(datadir), "%s/data", base);
	snprintf(savedir, sizeof(savedir), "%s/saves", base);
	snprintf(freedir, sizeof(freedir), "%s/data/free", base);

	mkdir(savedir, 0777);   /* 存檔目錄;data/ 由 Java 端建立並填好 */

	KB_strcpy(conf->data_dir, datadir);
	conf->set[C_data_dir] = 1;
	KB_strcpy(conf->save_dir, savedir);
	conf->set[C_save_dir] = 1;
	KB_strcpy(conf->install_dir, datadir);   /* --rootdir 等價:讓引擎找到 icon 等 */
	conf->autodiscover = 0;
	conf->set[C_autodiscover] = 1;

	/* 公開 free 版美術 (CC-BY-SA);CJK 文字仍由 free 模組路徑下的 cjk24.bin 載入 */
	add_module_aux(conf, "Free", KBFAMILY_GNU, 0, freedir, "", NULL, NULL);

	KB_stdlog("[android] datadir=%s savedir=%s (free module)\n", datadir, savedir);
}

#endif /* __ANDROID__ */
