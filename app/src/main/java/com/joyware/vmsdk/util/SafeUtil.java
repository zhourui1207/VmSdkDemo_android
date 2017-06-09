package com.joyware.vmsdk.util;

/**
 * Created by zhourui on 17/1/9.
 */

public class SafeUtil {
    private static long clickTime = 0;

    public static boolean clickIsSafe() {
        long currentTime = System.currentTimeMillis();
        boolean isSafe = false;
        if (currentTime - clickTime > 500) {  //0.5秒内点击两次是不安全的
            isSafe = true;
        }
        clickTime = currentTime;
        return isSafe;
    }

    public static <T> T checkNotNull(T object, String errorMessage) throws NullPointerException {
        if (object == null) {
            throw new NullPointerException(errorMessage);
        } else {
            return object;
        }
    }

    public static <T> T checkNotNull(T object) throws NullPointerException {
        if (object == null) {
            throw new NullPointerException();
        } else {
            return object;
        }
    }
}
