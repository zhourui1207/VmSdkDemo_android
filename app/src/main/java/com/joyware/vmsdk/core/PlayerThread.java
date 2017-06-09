package com.joyware.vmsdk.core;

import android.os.HandlerThread;

/**
 * Created by zhourui on 17/5/3.
 * email: 332867864@qq.com
 * phone: 17006421542
 */

public class PlayerThread extends HandlerThread {

    public PlayerThread(String name) {
        super(name);
    }

    public PlayerThread(String name, int priority) {
        super(name, priority);
    }
}
