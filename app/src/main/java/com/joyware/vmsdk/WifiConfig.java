package com.joyware.vmsdk;

/**
 * Created by zhourui on 2017/11/9.
 * email: 332867864@qq.com
 * phone: 17130045945
 */

public class WifiConfig {

    static {
        System.loadLibrary("JWSmartConfig");
    }

    public static final int ENCRY_TYPE_NONE = 1;
    public static final int ENCRY_TYPE_WEP_OPEN = 2;
    public static final int ENCRY_TYPE_WEP_SHARED = 3;
    public static final int ENCRY_TYPE_WPA_TKIP = 4;
    public static final int ENCRY_TYPE_WPA_AES = 5;
    public static final int WEP_KEY_INDEX_0 = 0;
    public static final int WEP_KEY_INDEX_1 = 1;
    public static final int WEP_KEY_INDEX_2 = 2;
    public static final int WEP_KEY_INDEX_3 = 3;
    public static final int WEP_KEY_INDEX_MAX = 4;
    public static final int WEP_PWD_LEN_64 = 0;
    public static final int WEP_PWD_LEN_128 = 1;

    public static boolean config(String wifiSsid, String wifiPwd) {
        return config(wifiSsid, wifiPwd, 0, 1000);
    }

    public static boolean config(String wifiSsid, String wifiPwd, long packetIntervalMillis, long
            waitTimeMillis) {
        return config(wifiSsid, wifiPwd, packetIntervalMillis, waitTimeMillis,
                ENCRY_TYPE_WPA_TKIP, WEP_KEY_INDEX_0, WEP_PWD_LEN_64);
    }

    public native static boolean config(String wifiSsid, String wifiPwd, long
            packetIntervalMillis, long waitTimeMillis, int encryType, int wepKeyIndex, int
                                                wepPwdLen);

    public native static void stop();
}
