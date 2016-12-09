package com.jxll.vmsdk.util;

import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.ConfigurationInfo;

/**
 * Created by zhourui on 16/11/3.
 */

public class OpenGLESUtil {
  // 检查是否支持 OpenGLES2.0
  public static boolean detectOpenGLES20(Context context) {
    ActivityManager am = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
    ConfigurationInfo info = am.getDeviceConfigurationInfo();
    return (info.reqGlEsVersion >= 0x20000);
  }
}
