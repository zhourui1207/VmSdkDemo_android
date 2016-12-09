package com.jxll.vmsdk;

/**
 * Created by zhourui on 16/10/29.
 */

public class VmType {
  // 码流类型
  public static final int STREAM_TYPE_FIX = 0;
  public static final int STREAM_TYPE_VIDEO = 1;
  public static final int STREAM_TYPE_AUDIO = 2;

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
  public static final int PLAY_STATUS_OPENSTREAMING = 1;  // 正在打开码流
  public static final int PLAY_STATUS_GETSTREAMING = 2;  // 正在获取码流
  public static final int PLAY_STATUS_DECODING = 3;  // 正在解码
  public static final int PLAY_STATUS_PLAYING = 4;  // 正在播放
}
