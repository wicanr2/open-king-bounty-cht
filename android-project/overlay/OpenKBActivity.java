package org.openkb.cht;

import android.os.Bundle;
import android.content.res.AssetManager;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import org.libsdl.app.SDLActivity;

/**
 * 御封戰將 Android 入口。繼承 SDL 的 SDLActivity:
 *  - getLibraries():宣告要載入的原生 lib (SDL2 / mixer / image / main=引擎)。
 *  - onCreate():首次啟動把 APK assets/data 複製到 getFilesDir()/data,
 *    引擎 (src/android.c) 之後以 SDL_AndroidGetInternalStoragePath() = getFilesDir()
 *    的路徑用 stdio 讀寫。APK assets 本身是唯讀且不在檔案系統,故須先攤平。
 */
public class OpenKBActivity extends SDLActivity {

    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", "SDL2_mixer", "SDL2_image", "main" };
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        try {
            File data = new File(getFilesDir(), "data");
            // 用標記檔判斷是否已攤平 (版本變更可刪此檔強制重攤)
            File stamp = new File(data, ".unpacked-v1");
            if (!stamp.exists()) {
                copyAssetDir("data", getFilesDir());
                stamp.getParentFile().mkdirs();
                new FileOutputStream(stamp).close();
            }
        } catch (Exception e) {
            android.util.Log.e("openkb", "asset unpack failed", e);
        }
        super.onCreate(savedInstanceState);
    }

    /** 把 assets/<path> 遞迴複製到 dstParent/<path>。 */
    private void copyAssetDir(String path, File dstParent) throws Exception {
        AssetManager am = getAssets();
        String[] items = am.list(path);
        File dst = new File(dstParent, path);
        if (items == null || items.length == 0) {   // 是檔案
            dst.getParentFile().mkdirs();
            try (InputStream in = am.open(path); OutputStream out = new FileOutputStream(dst)) {
                byte[] buf = new byte[8192]; int n;
                while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
            }
            return;
        }
        dst.mkdirs();                                 // 是目錄
        for (String it : items) copyAssetDir(path + "/" + it, dstParent);
    }
}
