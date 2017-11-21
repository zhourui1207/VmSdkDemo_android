package com.joyware.vmsdk;

/**
 * Created by zhourui on 16/10/29.
 */

public class VmType {
    // 码流类型
    public static final int STREAM_TYPE_FIX = 0;
    public static final int STREAM_TYPE_VIDEO = 1;
    public static final int STREAM_TYPE_AUDIO = 2;
    public static final int STREAM_TYPE_AUDIO_G711A = 3;

    // 播放模式
    public static final int PLAY_MODE_NONE = -1;
    public static final int PLAY_MODE_REALPLAY = 0;
    public static final int PLAY_MODE_PLAYBACK = 1;

    // 解码类型
    public static final int DECODE_TYPE_INTELL = 0;  // 智能解码（优先采用硬件解码，若硬件解码不支持，则采用软件解码）
    public static final int DECODE_TYPE_HARDWARE = 1;  // 硬件解码
    public static final int DECODE_TYPE_SOFTWARE = 2;  // 软件解码

    // 播放状态
    public static final int PLAY_STATUS_NONE = 0;  // 无状态
    public static final int PLAY_STATUS_BUSY = 1;  // 正在忙
    public static final int PLAY_STATUS_OPENSTREAMING = 2;  // 正在打开码流
    public static final int PLAY_STATUS_CONNECTING = 3;  // 正在连接
    public static final int PLAY_STATUS_WAITSTREAMING = 4;  // 正在等待码流
    public static final int PLAY_STATUS_DECODING = 5;  // 正在解码
    public static final int PLAY_STATUS_PLAYING = 6;  // 正在播放

    // 控制指令
    public static final int CONTROL_TYPE_PTZ_STOP = 0;  // 停止云台。参数1、2保留
    public static final int CONTROL_TYPE_PTZ_UP = 1;  // 云台向上转动。参数1为速度值，速度值范围：0-10；参数2为自动停止时间，单位秒
    public static final int CONTROL_TYPE_PTZ_DOWN = 2; // 向下
    public static final int CONTROL_TYPE_PTZ_LEFT = 3;  // 向左
    public static final int CONTROL_TYPE_PTZ_RIGHT = 4;  // 向右
    public static final int CONTROL_TYPE_PTZ_LEFT_UP = 5;  // 向左上
    public static final int CONTROL_TYPE_PTZ_RIGHT_UP = 6;  // 向右上
    public static final int CONTROL_TYPE_PTZ_LEFT_DOWN = 7;  // 向左下
    public static final int CONTROL_TYPE_PTZ_RIGHT_DOWN = 8;  // 向右下
    public static final int CONTROL_TYPE_IRIS_OPEN = 20;  // 增加光圈。参数1为速度值，速度值范围：0-10；参数2为自动停止时间，单位秒
    public static final int CONTROL_TYPE_IRIS_CLOSE = 21;  // 减少光圈
    public static final int CONTROL_TYPE_ZOOM_WIDE = 30;  // 缩镜头。参数1为速度值，速度值范围：0-10；参数2为自动停止时间，单位秒
    public static final int CONTROL_TYPE_ZOOM_IN = 31;  // 伸镜头
    public static final int CONTROL_TYPE_FOCUS_FAR = 40;  // 增加焦距。参数1为速度值，速度值范围：0-10；参数2为自动停止时间，单位秒
    public static final int CONTROL_TYPE_FOCUS_NEAR = 41;  // 减少焦距
    public static final int CONTROL_TYPE_PRE_POINT_ADD = 50;  // 增加预置点。参数1为预置点号，从1开始编号。
    public static final int CONTROL_TYPE_PRE_POINT_DEL = 51;  // 删除预置点
    public static final int CONTROL_TYPE_PRE_POINT_GOTO = 52;  // 转到预置点
    public static final int CONTROL_TYPE_PRE_POINT_AUTO_SCAN_START = 82;  // 开始自动扫描
    public static final int CONTROL_TYPE_PRE_POINT_AUTO_SCAN_STOP = 83;  // 结束自动扫描
    public static final int CONTROL_TYPE_SYN_TIME = 0x2000;  // 同步时间
    public static final int CONTROL_TYPE_IMAGE_BRIGHTNESS = 0X1000;  // 设置图像亮度。参数1为亮度值，参数2保留
    public static final int CONTROL_TYPE_IMAGE_SATURATION = 0X1001;  // 设置图像饱和度。参数1为饱和度值，参数2保留
    public static final int CONTROL_TYPE_IMAGE_CONTRAST = 0X1002;  // 设置图像对比度。参数1为对比度值，参数2保留
    public static final int CONTROL_TYPE_IMAGE_HUE = 0X1003;  // 设置图像色度。参数1为色度值，参数2保留

    // 心跳类型
    public static final int HEARTBEAT_TYPE_REALPLAY = 0 ; // 实时流心跳
    public static final int HEARTBEAT_TYPE_PLAYBACK = 1;  // 录像回放心跳
    public static final int HEARTBEAT_TYPE_TALK = 2;  // 语音对讲心跳

    public static final int PLAY_TYPE_REALPLAY = 1;  // 实时
    public static final int PLAY_TYPE_PLAYBACK = 2;  // 回放

    public static final int CLIENT_TYPE_ANDROID = 100;
    public static final int CLIENT_TYPE_IOS = 200;
    public static final int CLIENT_TYPE_PC = 300;
}
