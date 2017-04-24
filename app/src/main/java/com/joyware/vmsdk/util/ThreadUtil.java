package com.joyware.vmsdk.util;

import android.os.Looper;

/**
 * Created by zhourui on 17/4/13.
 */

public class ThreadUtil {

    public static boolean isMainThread() {
        return Looper.getMainLooper().getThread() == Thread.currentThread();
    }
}
