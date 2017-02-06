package com.jxll.vmsdk.util;

import android.util.Log;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.SocketException;
import java.nio.ByteBuffer;
import java.util.Enumeration;

/**
 * @author along
 */
public class StringUtil {
    private static final String TAG = "StringUtil";
    private static final String NEW_LINE = "\r";

    public static String getHexDump(byte[] buffer) {
        return getHexDump(buffer, 0);
    }

    public static String getHexDump(ByteBuffer byteBuffer) {
        byteBuffer.mark();
        int l_nLength = byteBuffer.limit();
        byte[] buffer = new byte[l_nLength];
        byteBuffer.get(buffer, 0, l_nLength);
        byteBuffer.reset();
        if (l_nLength == 0) {
            return "Dump Data Length IS ZERO.";
        }
        return getHexDump(buffer, 0);
    }

    public static String getHexDump(byte[] buffer, int len) {
        int dataLen = len;
        if (len <= 0 || len > buffer.length)
            dataLen = buffer.length;
        String dump = "";
        try {
            for (int i = 0; i < dataLen; i++) {
                if ((i % 8) == 0)
                    dump += " "; /* add gap every 8 chars */
                if ((i % 16) == 0)
                    dump += NEW_LINE;
                dump += Character.forDigit((buffer[i] >> 4) & 0x0f, 16);
                dump += Character.forDigit(buffer[i] & 0x0f, 16);
                dump += " ";
            }
        } catch (Throwable t) {
            // catch everything as this is for debug
            dump = "Throwable caught when dumping = " + t;
        }
        return dump;
    }

    public static String getStackTraceAsString(Throwable e) {
        if (e == null)
            return null;
        StringWriter stringWriter = new StringWriter();
        PrintWriter printWriter = new PrintWriter(stringWriter);
        e.printStackTrace(printWriter);
        StringBuffer error = stringWriter.getBuffer();
        return error.toString();
    }

    public static String getLocalIpAddress() {
        try {
            for (Enumeration<NetworkInterface> en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements(); ) {
                NetworkInterface intf = en.nextElement();
                for (Enumeration<InetAddress> enumIpAddr = intf.getInetAddresses(); enumIpAddr.hasMoreElements(); ) {
                    InetAddress inetAddress = enumIpAddr.nextElement();
                    if (!inetAddress.isLoopbackAddress() && !inetAddress.isLinkLocalAddress()) {
                        return inetAddress.getHostAddress().toString();
                    }
                }
            }
        } catch (SocketException ex) {
            Log.e("WifiPreference Ip", ex.toString());
        }
        return null;
    }

//	 public String getLocalIpAddressFromWifi() {
//		 WifiManager wifiManager = (WifiManager) getSystemService(WIFI_SERVICE);
//		 WifiInfo wifiInfo = wifiManager.getConnectionInfo();
//		 // 获取32位整型IP地址
//		 int ipAddress = wifiInfo.getIpAddress();
//
//		 //返回整型地址转换成“*.*.*.*”地址
//		 return String.format("%d.%d.%d.%d",
//				 (ipAddress & 0xff), (ipAddress >> 8 & 0xff),
//				 (ipAddress >> 16 & 0xff), (ipAddress >> 24 & 0xff));
//	 }

    public static void logDebug(String info) {
        Log.d(TAG, info);
    }

    public static void logError(String info) {
        Log.e(TAG, info);
    }

    public static void logError(Class<?> clsName, String info) {
        Log.e(TAG, info);
    }

    public static void logWarn(String info) {
        Log.w(TAG, info);
    }

    public static void logInfo(String info) {
        Log.i(TAG, info);
    }
}