package com.joyware.vmsdk;

/**
 * Created by zhourui on 2017/11/10.
 * email: 332867864@qq.com
 * phone: 17130045945
 */

public class DeviceDiscovery {

    static {
        System.loadLibrary("JWDeviceDiscovery");
    }

    public static native boolean start(DeviceFindCallBack deviceFindCallBack);

    public static native void stop();

    public static native void clearup();

    public static native void setAutoRequestInterval(int intervalSec);
}
