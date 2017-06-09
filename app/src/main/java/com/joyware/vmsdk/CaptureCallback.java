package com.joyware.vmsdk;

/**
 * Created by zhourui on 17/3/24.
 */

public interface CaptureCallback {

    void onSuccess(String filePath);

    void onFailed(String message);
}
