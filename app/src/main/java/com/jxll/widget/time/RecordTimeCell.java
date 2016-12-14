package com.jxll.widget.time;

/**
 * Created by zhourui on 16/12/12.
 * 这是录像时间单元
 */

public class RecordTimeCell {
  /**
   * 录像起始时间
   */
  private long beginTime;

  /**
   * 录像截止时间
   */
  private long endTime;

  /**
   * 有录像区域显示颜色
   */
  private int color;

  public RecordTimeCell(long beginTime, long endTime, int color) {
    this.beginTime = beginTime;
    this.endTime = endTime;
    this.color = color;
  }

  public long getBeginTime() {
    return beginTime;
  }

  public void setBeginTime(long beginTime) {
    this.beginTime = beginTime;
  }

  public long getEndTime() {
    return endTime;
  }

  public void setEndTime(long endTime) {
    this.endTime = endTime;
  }

  public int getColor() {
    return color;
  }

  public void setColor(int color) {
    this.color = color;
  }

  @Override
  public String toString() {
    return "RecordTimeCell{" +
        "beginTime=" + beginTime +
        ", endTime=" + endTime +
        ", color=" + color +
        '}';
  }
}
