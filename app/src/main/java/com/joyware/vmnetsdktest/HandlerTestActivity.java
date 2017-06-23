package com.joyware.vmnetsdktest;

import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.widget.TextView;

import java.lang.ref.WeakReference;
import java.util.Timer;

/**
 * Created by zhourui on 17/4/27.
 * email: 332867864@qq.com
 * phone: 17006421542
 */

public class HandlerTestActivity extends AppCompatActivity {

    private MyHandlder mMyHandlder;

    private Timer mTimer = new Timer();

    private TextView mTextView;

    private int count = 0;

    private Thread mThread;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        init();
    }

    private void init() {
        setContentView(R.layout.activity_handler_test);

        mTextView = (TextView) findViewById(R.id.textView);

        mThread = new Thread(new Runnable() {
            @Override
            public void run() {
                Looper.prepare();
                mMyHandlder = new MyHandlder(new WeakReference<>(HandlerTestActivity.this));
                Looper.loop();
                Log.e("new Thread", "quit");
            }
        });

        mThread.start();

//        mTimer.schedule(new TimerTask() {
//            @Override
//            public void run() {
//               mMyHandlder.sendMessage(Message.obtain());
////                mTimeTextView.setText((++count) + "");
//            }
//        }, 1000, 1000);
    }

    private static class MyHandlder extends Handler {

        private final String TAG = "MyHandler";

        private int count = 0;

        public MyHandlder(WeakReference<HandlerTestActivity> handlerTestActivityWeakReference) {
            mHandlerTestActivityWeakReference = handlerTestActivityWeakReference;
        }

        WeakReference<HandlerTestActivity> mHandlerTestActivityWeakReference;



        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case 0:
                    if (mHandlerTestActivityWeakReference != null) {
                        HandlerTestActivity handlerTestActivity = mHandlerTestActivityWeakReference.get();
                        if (handlerTestActivity != null) {
                            Log.e(TAG, "count = " + count);
                            try {
                                Thread.sleep(4000);
                            } catch (InterruptedException e) {
                                Log.e(TAG, "sleep InterruptedException");
                                e.printStackTrace();
                            }
                            Log.e(TAG, "sleep end");
                        } else {
                            Log.e(TAG, "handlerTestActivity = null");
                        }
                    } else {
                        Log.e(TAG, "mHandlerTestActivityWeakReference = null");
                    }
                    break;
            }


        }
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onDestroy() {
//        mTimer.cancel();
        Log.e("activity", "onDestroy start");
        mMyHandlder.sendEmptyMessage(0);
        try {
            Thread.sleep(2000);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        mMyHandlder.getLooper().quit();
        mThread.interrupt();
        try {
            mThread.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        super.onDestroy();
        Log.e("activity", "onDestroy end");
    }
}
