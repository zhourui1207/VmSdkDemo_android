package com.joyware.vmsdk.core;

/**
 * Created by zhourui on 17/4/29.
 * email: 332867864@qq.com
 * phone: 17006421542
 */

public interface GLRenderer {

    void onSizeChanged(long renderHandle, int w, int h);

    int onDrawFrame(long renderHandle);
}
