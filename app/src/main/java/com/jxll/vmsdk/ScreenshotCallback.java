package com.jxll.vmsdk;

/**
 * Created by zhourui on 17/3/24.
 */

public interface ScreenshotCallback {

    void onSuccess(String filePath);

    void onFailed(String message);
}
