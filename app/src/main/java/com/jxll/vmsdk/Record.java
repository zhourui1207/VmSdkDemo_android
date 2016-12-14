package com.jxll.vmsdk;

/**
 * Created by zhourui on 16/10/27.
 * 录像
 */

public class Record {
  private int beginTime;
  private int endTime;
  private String playbackUrl;
  private String downloadUrl;

  public Record(int beginTime, int endTime, String playbackUrl, String downloadUrl) {
    this.beginTime = beginTime;
    this.endTime = endTime;
    this.playbackUrl = playbackUrl;
    this.downloadUrl = downloadUrl;
  }

  public int getBeginTime() {
    return beginTime;
  }

  public void setBeginTime(int beginTime) {
    this.beginTime = beginTime;
  }

  public int getEndTime() {
    return endTime;
  }

  public void setEndTime(int endTime) {
    this.endTime = endTime;
  }

  public String getPlaybackUrl() {
    return playbackUrl;
  }

  public void setPlaybackUrl(String playbackUrl) {
    this.playbackUrl = playbackUrl;
  }

  public String getDownloadUrl() {
    return downloadUrl;
  }

  public void setDownloadUrl(String downloadUrl) {
    this.downloadUrl = downloadUrl;
  }

  @Override
  public String toString() {
    return "Record{" +
        "beginTime=" + beginTime +
        ", endTime=" + endTime +
        ", playbackUrl='" + playbackUrl + '\'' +
        ", downloadUrl='" + downloadUrl + '\'' +
        '}';
  }
}
