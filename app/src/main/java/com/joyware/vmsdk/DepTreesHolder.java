package com.joyware.vmsdk;

import java.util.LinkedList;
import java.util.List;

/**
 * Created by zhourui on 16/10/26.
 */

public class DepTreesHolder {
    private List<DepTree> depTrees = new LinkedList<>();

    public void addItem(int depId, String depName, int parentId, int onlineChannelCounts, int offlineChannelCounts) {
        DepTree depTree = new DepTree(depId, depName, parentId, onlineChannelCounts, offlineChannelCounts);
        depTrees.add(depTree);
    }

    public int size() {
        return depTrees.size();
    }

    public List<DepTree> list() {
        return depTrees;
    }
}
