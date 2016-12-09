package com.jxll.vmsdk;

import java.util.LinkedList;
import java.util.List;

/**
 * Created by zhourui on 16/10/27.
 */

public class ChannelsHolder {
  private List<Channel> channels = new LinkedList<>();

  public void addItem(int depId, String fdId, int channelId, int channelType, String
      channelName, int isOnline, int videoState, int channelState, int recordState) {
    Channel channel = new Channel(depId, fdId, channelId, channelType, channelName, isOnline,
        videoState, channelState, recordState);
    channels.add(channel);
  }

  public int size() {
    return channels.size();
  }

  public List<Channel> list() {
    return channels;
  }
}
