package com.jxll.vmsdk;

import java.util.LinkedList;
import java.util.List;

/**
 * Created by zhourui on 16/10/27.
 */

public class AlarmsHolder {
  private List<Alarm> alarms = new LinkedList<>();

  public void addItem(String alarmId, String fdId, String fdName, int channelId, String
      channelName, int channelBigType, String alarmTime, int alarmCode, String alarmName, String
                           alarmSubName, int alarmType, String photoUrl) {
    Alarm alarm = new Alarm(alarmId, fdId, fdName, channelId, channelName, channelBigType,
        alarmTime, alarmCode, alarmName, alarmSubName, alarmType, photoUrl);
    alarms.add(alarm);
  }

  public int size() {
    return alarms.size();
  }

  public List<Alarm> list() {
    return alarms;
  }
}
