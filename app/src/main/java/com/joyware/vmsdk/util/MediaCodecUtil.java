package com.joyware.vmsdk.util;

import android.annotation.SuppressLint;
import android.media.MediaCodecInfo;
import android.media.MediaCodecList;
import android.os.Build;

/**
 * Created by zhourui on 2017/7/6.
 * email: 332867864@qq.com
 * phone: 17006421542
 */

public class MediaCodecUtil {
    @SuppressLint("NewApi")
    public static boolean supportAvcCodec(String mediaType) {
        if (Build.VERSION.SDK_INT >= 18) {
            for (int j = MediaCodecList.getCodecCount() - 1; j >= 0; j--) {
                MediaCodecInfo codecInfo = MediaCodecList.getCodecInfoAt(j);

                String[] types = codecInfo.getSupportedTypes();
                for (String type : types) {
                    if (type.equalsIgnoreCase(mediaType)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }
}
