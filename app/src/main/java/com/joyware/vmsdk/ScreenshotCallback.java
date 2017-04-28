package com.joyware.vmsdk;

import android.graphics.Bitmap;

/**
 * Created by zhourui on 17/3/24.
 */

public interface ScreenshotCallback {

    void onSuccess(String filePath, Bitmap bitmap);

    void onFailed(String message);
}
