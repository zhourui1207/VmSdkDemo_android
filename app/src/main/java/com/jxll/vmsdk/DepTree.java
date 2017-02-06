package com.jxll.vmsdk;

/**
 * Created by zhourui on 16/10/26.
 */

public class DepTree {
    private int depId;
    private String depName;
    private int parentId;
    private int onlineChannelCounts;
    private int offlineChannelCounts;

    public DepTree(int depId, String depName, int parentId, int onlineChannelCounts, int
            offlineChannelCounts) {
        this.depId = depId;
        this.depName = depName;
        this.parentId = parentId;
        this.onlineChannelCounts = onlineChannelCounts;
        this.offlineChannelCounts = offlineChannelCounts;
    }

    public int getDepId() {
        return depId;
    }

    public void setDepId(int depId) {
        this.depId = depId;
    }

    public String getDepName() {
        return depName;
    }

    public void setDepName(String depName) {
        this.depName = depName;
    }

    public int getParentId() {
        return parentId;
    }

    public void setParentId(int parentId) {
        this.parentId = parentId;
    }

    public int getOnlineChannelCounts() {
        return onlineChannelCounts;
    }

    public void setOnlineChannelCounts(int onlineChannelCounts) {
        this.onlineChannelCounts = onlineChannelCounts;
    }

    public int getOfflineChannelCounts() {
        return offlineChannelCounts;
    }

    public void setOfflineChannelCounts(int offlineChannelCounts) {
        this.offlineChannelCounts = offlineChannelCounts;
    }

    @Override
    public String toString() {
        return "DepTree{" +
                "depId=" + depId +
                ", depName='" + depName + '\'' +
                ", parentId=" + parentId +
                ", onlineChannelCounts=" + onlineChannelCounts +
                ", offlineChannelCounts=" + offlineChannelCounts +
                '}';
    }
}
