package com.joyware.widget.time;

/**
 * Created by zhourui on 16/12/12.
 * 显示时间刻度
 */

public class TimeScale {
    /**
     * 在屏幕上能看到的总毫秒
     */
    private long totalMillisecondsInOneScreen;

    /**
     * 关键刻度的间隔
     */
    private long KeyScaleInterval;

    /**
     * 关键刻度之间的有小刻度间隔
     */
    private long minScaleInterval;

    /**
     * 显示的日期或时间格式
     */
    private String dataFormat = "HH:mm";

    /**
     * 关键刻度之间跳过多少刻度之后显示时间文字
     */
    private int displayTextInterval = 0;

    /**
     * 有时间刻度部分的视图长度
     */
    private int viewLength;

    public long getTotalMillisecondsInOneScreen() {
        return totalMillisecondsInOneScreen;
    }

    public void setTotalMillisecondsInOneScreen(long totalMillisecondsInOneScreen) {
        this.totalMillisecondsInOneScreen = totalMillisecondsInOneScreen;
    }

    public long getKeyScaleInterval() {
        return KeyScaleInterval;
    }

    public void setKeyScaleInterval(long keyScaleInterval) {
        KeyScaleInterval = keyScaleInterval;
    }

    public long getMinScaleInterval() {
        return minScaleInterval;
    }

    public void setMinScaleInterval(long minScaleInterval) {
        this.minScaleInterval = minScaleInterval;
    }

    public String getDataFormat() {
        return dataFormat;
    }

    public void setDataFormat(String dataFormat) {
        this.dataFormat = dataFormat;
    }

    public int getDisplayTextInterval() {
        return displayTextInterval;
    }

    public void setDisplayTextInterval(int displayTextInterval) {
        this.displayTextInterval = displayTextInterval;
    }

    public int getViewLength() {
        return viewLength;
    }

    public void setViewLength(int viewLength) {
        this.viewLength = viewLength;
    }
}
