package com.jxll.vmsdk;

/**
 * Created by zhourui on 16/10/27.
 * 报警
 */

public class Alarm {
  private String alarmId;
  private String fdId;
  private String fdName;
  private int channelId;
  private String channelName;
  private int channelBigType;
  private String alarmTime;
  private int alarmCode;
  private String alarmName;
  private String alarmSubName;
  private int alarmType;
  private String photoUrl;

  public Alarm(String alarmId, String fdId, String fdName, int channelId, String channelName, int
      channelBigType, String alarmTime, int alarmCode, String alarmName, String alarmSubName, int
      alarmType, String photoUrl) {
    this.alarmId = alarmId;
    this.fdId = fdId;
    this.fdName = fdName;
    this.channelId = channelId;
    this.channelName = channelName;
    this.channelBigType = channelBigType;
    this.alarmTime = alarmTime;
    this.alarmCode = alarmCode;
    this.alarmName = alarmName;
    this.alarmSubName = alarmSubName;
    this.alarmType = alarmType;
    this.photoUrl = photoUrl;
  }

  public String getAlarmId() {
    return alarmId;
  }

  public void setAlarmId(String alarmId) {
    this.alarmId = alarmId;
  }

  public String getFdId() {
    return fdId;
  }

  public void setFdId(String fdId) {
    this.fdId = fdId;
  }

  public String getFdName() {
    return fdName;
  }

  public void setFdName(String fdName) {
    this.fdName = fdName;
  }

  public int getChannelId() {
    return channelId;
  }

  public void setChannelId(int channelId) {
    this.channelId = channelId;
  }

  public String getChannelName() {
    return channelName;
  }

  public void setChannelName(String channelName) {
    this.channelName = channelName;
  }

  public int getChannelBigType() {
    return channelBigType;
  }

  public void setChannelBigType(int channelBigType) {
    this.channelBigType = channelBigType;
  }

  public String getAlarmTime() {
    return alarmTime;
  }

  public void setAlarmTime(String alarmTime) {
    this.alarmTime = alarmTime;
  }

  public int getAlarmCode() {
    return alarmCode;
  }

  public void setAlarmCode(int alarmCode) {
    this.alarmCode = alarmCode;
  }

  public String getAlarmName() {
    return alarmName;
  }

  public void setAlarmName(String alarmName) {
    this.alarmName = alarmName;
  }

  public String getAlarmSubName() {
    return alarmSubName;
  }

  public void setAlarmSubName(String alarmSubName) {
    this.alarmSubName = alarmSubName;
  }

  public int getAlarmType() {
    return alarmType;
  }

  public void setAlarmType(int alarmType) {
    this.alarmType = alarmType;
  }

  public String getPhotoUrl() {
    return photoUrl;
  }

  public void setPhotoUrl(String photoUrl) {
    this.photoUrl = photoUrl;
  }

  @Override
  public String toString() {
    return "Alarm{" +
        "alarmId='" + alarmId + '\'' +
        ", fdId='" + fdId + '\'' +
        ", fdName='" + fdName + '\'' +
        ", channelId=" + channelId +
        ", channelName='" + channelName + '\'' +
        ", channelBigType=" + channelBigType +
        ", alarmTime='" + alarmTime + '\'' +
        ", alarmCode=" + alarmCode +
        ", alarmName='" + alarmName + '\'' +
        ", alarmSubName='" + alarmSubName + '\'' +
        ", alarmType=" + alarmType +
        ", photoUrl='" + photoUrl + '\'' +
        '}';
  }
}
