package com.joyware.vmsdk.util;

import android.os.Build;

/**
 * Created by zhourui on 17/4/14.
 */

public class DeviceUtil {

    public static String getBrand() {
        return Build.BRAND;
    }

    public static String getModel() {
        return Build.MODEL;
    }
}
