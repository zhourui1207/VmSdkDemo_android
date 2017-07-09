package com.joyware.vmnetsdktest;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.graphics.Matrix;
import android.graphics.PointF;
import android.opengl.GLSurfaceView;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.joyware.util.TimeUtil;
import com.joyware.vmsdk.Channel;
import com.joyware.vmsdk.ChannelsHolder;
import com.joyware.vmsdk.DepTree;
import com.joyware.vmsdk.DepTreesHolder;
import com.joyware.vmsdk.RecordDownloader;
import com.joyware.vmsdk.VmNet;
import com.joyware.vmsdk.VmPlayer;
import com.joyware.vmsdk.core.AudioCollector;
import com.joyware.vmsdk.core.BlockingBuffer;
import com.joyware.vmsdk.core.CheckAudioPermission;
import com.joyware.vmsdk.core.JWAsyncTask;
import com.joyware.vmsdk.core.PriorityData;
import com.joyware.vmsdk.core.RTPSortFilter;
import com.joyware.vmsdk.core.RecordThread;
import com.joyware.vmsdk.util.OpenGLESUtil;
import com.joyware.vmsdk.util.opengles.GLFrameRenderer;
import com.joyware.widget.time.RecordTimeCell;
import com.joyware.widget.time.TimeBar;

import java.io.File;
import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.List;

public class MainActivity extends AppCompatActivity implements VmNet.ServerConnectStatusCallback {

    private static SurfaceView surfaceView;
    private static int id = 0;
    GLFrameRenderer mGLFRenderer;
    GLSurfaceView mGLSurfaceView;
    TextView textView;
    TextView mScaleText;
    boolean audioOpen = false;
    boolean recording = false;
    VmPlayer player;
    private String TAG = "MainActivity";
    private SurfaceView surfaceView2;
    private SurfaceView surfaceView4;
    private SurfaceView surfaceView5;
    private SurfaceView surfaceView6;
    private SurfaceView surfaceView7;
    private SurfaceView surfaceView8;
    private SurfaceView surfaceView9;
    private SurfaceView surfaceView3;
    private TimeBar timeBar;
    private Button button;
    private Button record;
    private List<RecordTimeCell> recordTimeCellList = new ArrayList<>();

    private AudioCollector mAudioCollector = new AudioCollector();

    JWDefaultScaleFilter mJWDefaultScaleFilter;

    TextView mProgressTextView;

    ProgressBar mProgressBar;

    long rtspStreamId;

    TextView mTimeTextView;

    @NonNull
    private final RecordDownloader mRecordDownloader = new RecordDownloader();

    public static Surface getNativeSurface() {
        Log.e("!!", "getNativeSurface");
        return surfaceView.getHolder().getSurface();
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static int audioOpen(int sampleRate, boolean is16Bit, boolean isStereo, int
            desiredFrames) {
        Log.e("!!", "audioOpen");
        return 0;
    }

    public static void audioWriteShortBuffer(short[] buffer) {
        Log.e("!!", "audioWriteShortBuffer");
    }

    public static void audioWriteByteBuffer(byte[] buffer) {
        Log.e("!!", "audioWriteByteBuffer");
    }

    public static int captureOpen(int sampleRate, boolean is16Bit, boolean isStereo, int
            desiredFrames) {
        Log.e("!!", "captureOpen");
        return 0;
    }

    public static int captureReadShortBuffer(short[] buffer, boolean blocking) {
        Log.e("!!", "captureReadShortBuffer");
        return 0;
    }

    public static int captureReadByteBuffer(byte[] buffer, boolean blocking) {
        Log.e("!!", "captureReadByteBuffer");
        return 0;
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static void audioClose() {
        Log.e("!!", "audioClose");
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static void captureClose() {
        Log.e("!!", "captureClose");
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static void pollInputDevices() {
        Log.e("!!", "pollInputDevices");
    }

    public GLFrameRenderer getGLFrameRenderer() {
        return mGLFRenderer;
    }

    private float ma;
    private float mScaleCenterX;
    private float mScaleCenterY;
    private float mScalesRrcLeft;
    private float mScalesRrcTop;
    private float mScaleFactor = 1f;
    private float mLeft = 0f;
    private float mTop = 0f;
    private float mSrcLeft;
    private float mSrcTop;
    private boolean mDrag = false;
    private boolean mZoom = false;

    DecimalFormat decimalFormat = new DecimalFormat(".0");//构造方法的字符格式这里如果小数不足2位,会以0补足.

    /**
     * 一个坐标点，以某个点为缩放中心，缩放指定倍数，求这个坐标点在缩放后的新坐标值。
     *
     * @param targetPointX 坐标点的X
     * @param targetPointY 坐标点的Y
     * @param scaleCenterX 缩放中心的X
     * @param scaleCenterY 缩放中心的Y
     * @param scale        缩放倍数
     * @return 坐标点的新坐标
     */
    protected PointF scaleByPoint(float targetPointX, float targetPointY, float scaleCenterX,
                                  float scaleCenterY, float scale) {
        Matrix matrix = new Matrix();
        // 将Matrix移到到当前圆所在的位置，
        // 然后再以某个点为中心进行缩放
        matrix.preTranslate(targetPointX, targetPointY);
        matrix.postScale(scale, scale, scaleCenterX, scaleCenterY);
        float[] values = new float[9];
        matrix.getValues(values);
        return new PointF(values[Matrix.MTRANS_X], values[Matrix.MTRANS_Y]);
    }

    // 点过滤器，过滤掉不符合规范的坐标
    private void defaultPointFilter() {
        // 范围，放大倍速不超过4.0，缩小倍速不能小于1.0，不能留出黑边
        String p = decimalFormat.format(mScaleFactor);
        mScaleText.setText(p);

        // 黑边过滤
        if (mLeft > 0f) {
            mLeft = 0f;
        }
        if (mLeft + mScaleFactor < 1f) {
            mLeft = 1f - mScaleFactor;
        }
        if (mTop > 0f) {
            mTop = 0f;
        }
        if (mTop + mScaleFactor < 1f) {
            mTop = 1f - mScaleFactor;
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON, WindowManager
                .LayoutParams.FLAG_KEEP_SCREEN_ON);  //保持屏幕常亮
        setContentView(R.layout.activity_main);

        mScaleText = (TextView) findViewById(R.id.sample_text);

        mTimeTextView = (TextView) findViewById(R.id.textView_time);

        mProgressTextView = (TextView) findViewById(R.id.textView_progress);
        mProgressBar = (ProgressBar) findViewById(R.id.pb);

        findViewById(R.id.btn_show_timebar).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                timeBar.setVisibility(View.VISIBLE);
            }
        });

        findViewById(R.id.btn_hide_timebar).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                timeBar.setVisibility(View.GONE);
            }
        });


        mRecordDownloader.setOnRecordDownloadStatusListener(new RecordDownloader
                .OnRecordDownloadStatusListener() {


            @Override
            public void onRecordDownloadStart(RecordDownloader recordDownloader) {
                mProgressTextView.post(new Runnable() {
                    @Override
                    public void run() {
                        mProgressTextView.setText("开始录像");
                        mProgressBar.setProgress(0);
                    }
                });
            }

            @Override
            public void onRecordDownloadProgress(RecordDownloader recordDownloader, final float
                    progress) {
//                Log.e(TAG, "onRecordDownloadProgress progress = " + progress);
                mProgressTextView.post(new Runnable() {
                    @Override
                    public void run() {
                        mProgressTextView.setText("录像进度:" + (progress * 100) + "%");
                        mProgressBar.setProgress((int) (progress * 100));
                    }
                });
            }

            @Override
            public void onRecordDownloadSuccess(RecordDownloader recordDownloader) {
                mProgressTextView.post(new Runnable() {
                    @Override
                    public void run() {
                        mProgressTextView.setText("录像下载成功");
                        mProgressBar.setProgress(100);
                    }
                });
            }

            @Override
            public void onRecordDownloadFailed(RecordDownloader recordDownloader, int errorCode,
                                               final String message) {
                mProgressTextView.post(new Runnable() {
                    @Override
                    public void run() {
                        mProgressTextView.setText("录像下载失败:" + message);
                    }
                });
            }
        });


        findViewById(R.id.btn_start_rtsp).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                rtspStreamId = VmNet.startStreamByRtsp
                        ("rtsp://admin:admin12345@192.168.3.38:554/Streaming/Channels/101" +
                                "?transportmode=unicast&profile=Profile_1", new VmNet
                                .StreamCallbackV2() {
                    @Override
                    public void onStreamConnectStatus(boolean isConnected) {
                        Log.e(TAG, "onStreamConnectStatus isConnected = " + isConnected);
                    }

                    @Override
                    public void onReceiveStream(byte[] streamData, int streamLen) {
                        Log.e(TAG, "onReceiveStream streamLen = " + streamLen);
                    }
                });

                Log.e(TAG, "startStreamByRtsp rtspStreamId=" + rtspStreamId);
            }
        });

        findViewById(R.id.btn_stop_rtsp).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (rtspStreamId != 0) {
                    VmNet.stopStreamByRtsp(rtspStreamId);
                    rtspStreamId = 0;
                }
            }
        });

        findViewById(R.id.btn_start_download_record).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Log.e(TAG, "startRecordTask");
                int startTime = 1498452620;
                mRecordDownloader.startRecordTask(RecordThread.RECORD_FILE_TYPE_MP4,
                        "/sdcard/MVSS/test.mp4", true, "201706151925230392", 1, startTime,
                        startTime + 60 * 2);
            }
        });

        findViewById(R.id.btn_cancel_download_record).setOnClickListener(new View.OnClickListener
                () {
            @Override
            public void onClick(View v) {
                Log.e(TAG, "cancelRecordTask");
                mRecordDownloader.cancelRecordTask();
            }
        });

        findViewById(R.id.btn_start_talk).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (CheckAudioPermission.isHasPermission(MainActivity.this)) {
                    mAudioCollector.setOnAudioDataListener(new AudioCollector
                            .OnG711AudioDataListener() {

                        @Override
                        public void onG711AudioData(byte[] audioData, int audioLen) {
                            Log.e(TAG, "onG711AudioData audiolen=" + audioLen);
                        }
                    });
                    mAudioCollector.startRecord();
                }
            }
        });

        findViewById(R.id.btn_stop_talk).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mAudioCollector.stopRecord();
            }
        });

        findViewById(R.id.btn_start_talk_play).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mAudioCollector.setPlay(true);
            }
        });

        findViewById(R.id.btn_stop_talk_play).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mAudioCollector.setPlay(false);
            }
        });

//        mJWDefaultScaleFilter = new JWDefaultScaleFilter(this, new Runnable() {
//            @Override
//            public void run() {
//                player.scaleTo(true, mJWDefaultScaleFilter.getLeftPercent(),
//                        mJWDefaultScaleFilter.getTopPercent(), mJWDefaultScaleFilter
//                                .getScaleFactor(), mJWDefaultScaleFilter.getScaleFactor());
//            }
//        });
//        surfaceView = (SurfaceView) findViewById(R.id.surfaceView);
//        mJWDefaultScaleFilter.setView(surfaceView);
//
        final JWAsyncTask myAsyncTask = new MyAsyncTask().execute(2);

        BlockingBuffer blockingBuffer = new BlockingBuffer(BlockingBuffer.BlockingBufferType
                .PRIORITY);

        int i = 0;
        blockingBuffer.addObject(new PriorityData("1", 5, ++i));
        blockingBuffer.addObject(new PriorityData("2", 4, ++i));
        blockingBuffer.addObject(new PriorityData("3", 4, ++i));
        blockingBuffer.addObject(new PriorityData("4", 4, ++i));
        blockingBuffer.addObject(new PriorityData("5", 4, ++i));
        blockingBuffer.addObject(new PriorityData("6", 4, ++i));
        blockingBuffer.addObject(new PriorityData("7", 4, ++i));
        blockingBuffer.addObject(new PriorityData("8", 1, ++i));

        for (int j = 0; j < 8; ++j) {
            try {
                PriorityData priorityData = (PriorityData) blockingBuffer.removeObjectBlocking();
                Log.e(TAG, priorityData.toString());
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }

        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    Thread.sleep(5000);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                myAsyncTask.cancel(true);
            }
        }).start();


//        scaleGestureDetector = new ScaleGestureDetector(this, new ScaleGestureDetector
//                .OnScaleGestureListener() {
//
//
//            @Override
//            public boolean onScale(ScaleGestureDetector detector) {
//                if (mZoom) {
//                    float sacleFactor = detector.getScaleFactor();
//                    mScaleFactor *= sacleFactor;
//                    if (mScaleFactor > 4) {
//                        mScaleFactor = 4;
//                    } else if (mScaleFactor < 1) {
//                        mScaleFactor = 1;
//                    }
//                    float left = mScalesRrcLeft;
//                    float top = mScalesRrcTop;
//
//                    PointF pointF = scaleByPoint(left, top, mScaleCenterX,
//                            mScaleCenterY, mScaleFactor / ma);
//
//                    mLeft = pointF.x / surfaceView.getWidth();
//                    mTop = pointF.y / surfaceView.getHeight();
//
//                    defaultPointFilter();
//                    Log.e(TAG, "mLeft=" + mLeft + ", mTop=" + mTop);
//
//                }
//                return true;
//            }
//
//            @Override
//            public boolean onScaleBegin(ScaleGestureDetector detector) {
//                Log.e(TAG, "onScaleBegin");
//                if (!mZoom) {
//                    mDrag = false;
//                    mZoom = true;
//
//                    // 左上角当前实际坐标 像素
//                    mScalesRrcLeft = mLeft * surfaceView.getWidth();
//                    mScalesRrcTop = mTop * surfaceView.getHeight();
//                    ma = mScaleFactor;
//
//                    // 获取两指中心点坐标相对于0.5f坐标 像素
//                    mScaleCenterX = detector.getFocusX();
//                    mScaleCenterY = detector.getFocusY();
////                    // 将当前两指中点平移到屏幕中心点
//                    Log.e(TAG, "mScaleCenterX=" + mScaleCenterX + ", mScaleCenterY=" +
//                            mScaleCenterY);
//                }
//
//                // 中点坐标
//                return true;
//            }
//
//            @Override
//            public void onScaleEnd(ScaleGestureDetector detector) {
////                Log.e(TAG, "onScaleEnd");
////                mZoom = false;
//            }
//        });

//        surfaceView.setOnTouchListener(new View.OnTouchListener() {
//            @Override
//            public boolean onTouch(View v, MotionEvent event) {
//                mJWDefaultScaleFilter.isChanged(event);
//                return true;
////                if (event.getPointerCount() > 1) {
////                    scaleGestureDetector.onTouchEvent(event);
////                    mDrag = false;
////                    if (event.getAction() == MotionEvent.ACTION_CANCEL || event.getAction() ==
////                            MotionEvent.ACTION_UP || event.getAction() == MotionEvent
////                            .ACTION_POINTER_UP) {
////                        mZoom = false;
////                    }
////                    return true;
////                }
////                switch (event.getAction()) {
////                    case MotionEvent.ACTION_DOWN:
////                        mDrag = true;
////                        mSrcLeft = event.getRawX();
////                        mSrcTop = event.getRawY();
////                        break;
////                    case MotionEvent.ACTION_MOVE:
////                        if (mDrag) {
////                            float currentX = event.getRawX();
////                            float currentY = event.getRawY();
////                            float offsetX = (currentX - mSrcLeft) / v.getWidth();
////                            float offsetY = (currentY - mSrcTop) / v.getHeight();
////                            mSrcLeft = currentX;
////                            mSrcTop = currentY;
////                            mLeft += offsetX;
////                            mTop += offsetY;
////                            defaultPointFilter();
////                            player.scaleTo(true, mLeft, mTop, mScaleFactor, mScaleFactor);
////                            Log.e(TAG, "scaleTo");
////                        }
////                        break;
////                    case MotionEvent.ACTION_CANCEL:
////                    case MotionEvent.ACTION_UP:
////                        mDrag = false;
////                        mZoom = false;
////                        break;
////                }
////                return true;
//            }
//        });
        textView = (TextView) findViewById(R.id.tv);
        timeBar = (TimeBar) findViewById(R.id.tb);
        timeBar.setOnCurrentTimeChangListener(new TimeBar.OnCurrentTimeChangListener() {
            @Override
            public void onCurrentTimeChanging(long currentTime) {
                mTimeTextView.setText(TimeUtil.timeStamp2Date(currentTime, "HH:mm:ss"));
            }

            @Override
            public void onCurrentTimeChanged(long currentTime) {
                mTimeTextView.setText(TimeUtil.timeStamp2Date(currentTime, "HH:mm:ss"));
            }
        });

        String currentStr = TimeUtil.timeStamp2Date(System.currentTimeMillis(), null);
        String[] sub = currentStr.split(" ");
        String dayBegin = sub[0] + " 00:00:00";
        long beginTime = TimeUtil.date2TimeStamp(dayBegin, null);
        timeBar.setBeginTime(beginTime);
        timeBar.setCurrentTime(System.currentTimeMillis());
        timeBar.setRecordList(recordTimeCellList);
//        timeBar.setCurrentTimeChangListener(new TimeBar.CurrentTimeChangListener() {
//            @Override
//            public void onCurrentTimeChanging(long currentTime) {
//                textView.setText(TimeUtil.timeStamp2Date(currentTime, null));
//            }
//
//            @Override
//            public void onCurrentTimeChanged(long currentTime) {
//                textView.setText(TimeUtil.timeStamp2Date(currentTime, null));
//                player.stopPlay();
//                player.startPlayback("201610111654538071", 1, true, (int) (currentTime / 1000L),
//                        (int) (System.currentTimeMillis() / 1000L), 0, false);
//            }
//        });


//    surfaceView2 = (SurfaceView) findViewById(R.id.surfaceView2);
//    surfaceView3 = (SurfaceView) findViewById(R.id.surfaceView3);
//    surfaceView4 = (SurfaceView) findViewById(R.id.surfaceView4);
//    surfaceView5 = (SurfaceView) findViewById(R.id.surfaceView5);
//    surfaceView6 = (SurfaceView) findViewById(R.id.surfaceView6);
//    surfaceView7 = (SurfaceView) findViewById(R.id.surfaceView7);
//    surfaceView8 = (SurfaceView) findViewById(R.id.surfaceView8);
//    surfaceView9 = (SurfaceView) findViewById(R.id.surfaceView9);


        button = (Button) findViewById(R.id.button);
//    glSurfaceView = (GLSurfaceView) findViewById(R.id.glSurfaceView);

        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (player == null) {
                    return;
                }
                if (audioOpen) {
                    player.closeAudio();
                    audioOpen = false;
                    button.setText("打开声音");
                } else {
                    audioOpen = player.openAudio();
                    if (audioOpen) {
                        button.setText("关闭声音");
                    }
                }
            }
//                Log.e("!!!", "stop start");
//                player.stopPlay();
//                Log.e("!!!", "stop end");
//                try {
//                    Thread.sleep(1000);
//                } catch (InterruptedException e) {
//                    e.printStackTrace();
//                }
//                Log.e("!!!", "startRealplay start");
//                player.startRealplay("201610111654538071", 1, false, 2, false, false,
// surfaceView.getHolder(), MainActivity.this);
//                Log.e("!!!", "startRealplay end");
//
//            }
        });

        record = (Button) findViewById(R.id.record);
        record.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                String saveDir = "/sdcard/MVSS/Localrecord";
                File dir = new File(saveDir);
                if (!dir.exists()) {
                    dir.mkdir();
                }


                if (recording) {
                    player.stopRecord();
                    recording = false;
                    record.setText("开始录像");
                } else {
                    recording = player.startRecord("/sdcard/MVSS/Localrecord/test1.mp4");
                    if (recording) {
                        record.setText("停止录像");
                    }
                }
            }
//      }
        });

        // Example of a call to a native method
        TextView tv = (TextView) findViewById(R.id.sample_text);

        boolean supportOpenGLES = OpenGLESUtil.detectOpenGLES20(this);
        if (supportOpenGLES) {
            tv.setText("支持openGLES2.0");
        } else {
            tv.setText("不支持openGLES2.0");
        }
//    glSurfaceView.setEGLContextClientVersion(2);
//    mGLFRenderer = new GLFrameRenderer(glSurfaceView);
//    glSurfaceView.setRenderer(mGLFRenderer);


        new RealplayTask().executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);

        RTPSortFilter rtpSortFilter = new RTPSortFilter(5);
        rtpSortFilter.setOnSortedCallback(new RTPSortFilter.OnSortedCallback() {
            @Override
            public void onSorted(int payloadType, byte[] buffer, int start, int len, int
                    timeStamp, int seqNumber, boolean mark) {
                Log.e(TAG, "onSorted seqNumber=" + seqNumber);
            }
        });

        rtpSortFilter.setOnMissedCallback(new RTPSortFilter.OnMissedCallback() {
            @Override
            public void onMissed(int missedStartSeqNumber, int missedNumber) {
                Log.e(TAG, "onMissed missedStartSeqNumber=" + missedStartSeqNumber + " , " +
                        "missedNumbe=" + missedNumber);
            }
        });

        rtpSortFilter.receive(1,null, 0, 0, 0, 1, false);
        rtpSortFilter.receive(1,null, 0, 0, 0, 2, false);
        rtpSortFilter.receive(1,null, 0, 0, 0, 3, false);
        rtpSortFilter.receive(1,null, 0, 0, 0, 5, false);
        rtpSortFilter.receive(1,null, 0, 0, 0, 6, false);
        rtpSortFilter.receive(1,null, 0, 0, 0, 4, false);
        rtpSortFilter.receive(1,null, 0, 0, 0, 8, false);
        rtpSortFilter.receive(1,null, 0, 0, 0, 7, false);
        rtpSortFilter.receive(1,null, 0, 0, 0, 9, false);
//    final Thread sdlThread = new Thread(new SDLMain(), "SDLThread");
//    sdlThread.start();
    }

    @Override
    public void onServerConnectStatus(boolean isConnected) {
        Log.e(TAG, "服务器连接状态:" + isConnected);
    }

    class SDLMain implements Runnable {
        @Override
        public void run() {
            // Runs SDL_main()

            //Log.v("SDL", "SDL thread terminated");
        }
    }

    class RealplayTask extends AsyncTask<Void, Void, Void> {

        @Override
        protected Void doInBackground(Void... voids) {
            VmNet.init(10);

//            Log.e(TAG, "connect start");
//            VmNet.connect("10.234.11.3", 5516, MainActivity.this);
//            Log.e(TAG, "connect end");
//            try {
//                Thread.sleep(5000);
//            } catch (InterruptedException e) {
//                e.printStackTrace();
//            }
//            Log.e(TAG, "disconnect start");
//            VmNet.disconnect();
//            Log.e(TAG, "disconnect end");


            VmNet.connect("10.100.23.142", 5516, MainActivity.this);
//            VmNet.connect("118.178.132.146", 5516, MainActivity.this);
//            VmNet.connect("10.151.0.252", 5516, MainActivity.this);


//            VmNet.connect("118.178.132.146", 5516, MainActivity.this);
//      VmNet.connect("118.178.132.146", 5516, MainActivity.this);
            try {
                Thread.sleep(2000);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            int ret = VmNet.login("admin", "123456");
            if (ret == 0) {
                Log.e(TAG, "登录成功!");
                DepTreesHolder depTreesHolder = new DepTreesHolder();
                ret = VmNet.getDepTrees(1, 100, depTreesHolder);
                if (ret == 0 && depTreesHolder.size() > 0) {
                    List<DepTree> depTrees = depTreesHolder.list();
                    for (DepTree depTree : depTrees) {
                        Log.e(TAG, "depName=" + depTree.getDepName() + ", depId=" + depTree
                                .getDepId() + ", parentId=" + depTree.getParentId());
                    }
                }

                ChannelsHolder channelsHolder = new ChannelsHolder();
                ret = VmNet.getChannels(1, 100, 0, channelsHolder);
                if (ret == 0 && channelsHolder.size() > 0) {
                    List<Channel> channels = channelsHolder.list();
                    for (Channel channel : channels) {
                        Log.e(TAG, "channelName=" + channel.getChannelName() + ", fdId=" +
                                channel.getFdId() + ", channelId=" + channel.getChannelId());
                    }
                }

            } else {
                Log.e(TAG, "登录失败!");
            }

            VmNet.startReceiveRealAlarm(new VmNet.RealAlarmCallback() {
                @Override
                public void onRealAlarm(String fdId, int channelId, int alarmType, int param1,
                                        int param2) {
                    Log.e(TAG, "onRealAlarm");
                    NotificationManager manager = (NotificationManager) getSystemService(Context
                            .NOTIFICATION_SERVICE);

                    PendingIntent pendingIntent3 = PendingIntent.getActivity(MainActivity.this, 0,
                            new Intent(MainActivity.this, null), 0);
                    // 通过Notification.Builder来创建通知，注意API Level
                    // API16之后才支持
                    Notification notify3 = new Notification.Builder(MainActivity.this)
                            .setSmallIcon(R.mipmap.ic_launcher)
                            .setTicker("fdId:" + fdId + ", channelId:" + channelId + "发生报警")
                            .setContentTitle("告警消息通知")
                            .setContentText("外部告警")
                            .setContentIntent(pendingIntent3).build();

                    notify3.flags |= Notification.FLAG_AUTO_CANCEL; //
                    // FLAG_AUTO_CANCEL表明当通知被用户点击时，通知将被清除。
                    manager.notify(id++, notify3);// 步骤4：通过通知管理器来发起通知。如果id不同，则每click，在status哪里增加一个提
                }
            });

//            TalkAddressHolder talkAddressHolder = new TalkAddressHolder();
//            ret = VmNet.startTalk("201607201402389091", 1, talkAddressHolder);
//            if (ret == 0) {
//                Log.e(TAG, "onRealAlarm addr=" + talkAddressHolder.getTalkAddr() + ", port=" +
// talkAddressHolder.getTalkPort());
//            } else {
//                Log.e(TAG, "startTalk error");
//            }
//
//            RecordsHolder recordsHolder = new RecordsHolder();
//            long currentTime = System.currentTimeMillis();
//            Log.e("currentTime", "currentTime=" + currentTime);
//            ret = VmNet.getRecords(0, 100, "201610111654538071", 1, (int) (currentTime / 1000)
// - 7200, (int) (currentTime / 1000), true, recordsHolder);
//
//            Log.e("getRecords", "getRecords ret = " + ret + ", size=" + recordsHolder.size());
//            if (ret == 0) {
//                recordTimeCellList.clear();
//                for (Record record : recordsHolder.list()) {
//                    RecordTimeCell recordTimeCell = new RecordTimeCell(record.getBeginTime() *
// 1000L, record.getEndTime() * 1000L, Color.rgb(150, 255, 150));
//                    recordTimeCellList.add(recordTimeCell);
//                }
//            }

            try {
                Thread.sleep(1000);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            return null;
        }

        @Override
        protected void onPostExecute(Void aVoid) {

//            timeBar.invalidate();
//            player = new VmPlayer();
//      player.startRealplay("201612022115042811", 1, true, 1, false, false, surfaceView
// .getHolder(), MainActivity.this);
            // gb122
//            player.setSurfaceHolder(surfaceView.getHolder());
//            player.startRealplay("201701161314340662", 1, false, 0, false);
            // gb_hk119
//            player.startRealplay("201702131054154181", 1, true, 0, false, false, surfaceView
// .getHolder(), MainActivity.this);
//            player.startRealplay("201701161355294261", 1, true, 0, false, false, surfaceView
// .getHolder(), MainActivity.this);
//            player.startRealplay("201701171014494814", 1, true, 0, false, false, surfaceView
// .getHolder(), MainActivity.this);
            // 119
//            player.setSurfaceHolder(surfaceView.getHolder());
//            player.startRealplay("201701131326271432", 1, false, 0, false);
            // 23.142
//            player.startRealplay("201701161355294261", 1, true, 0, false, false, surfaceView
// .getHolder(), MainActivity.this);
//            player.setAutoTryReconnectCount(3);
//            player.setTimeoutSeconds(10);

//      player.startPlayback("201610111654538071", 1, true, 1481242200, 1481299200, 0, false,
// false, surfaceView.getHolder(), MainActivity.this);
            Log.e(TAG, " 0x000001BA=" + 0x000001BA + "0xFFFFFFFF=" + 0xFFFFFFFF);
        }
    }

    private class MyAsyncTask extends JWAsyncTask<Integer, Integer, Integer> {

        private static final String TAG = "MyAsyncTask";

        @Override
        protected void onPreExecute() {
            Log.e(TAG, "onPreExecute, thread = " + Thread.currentThread().getId());
        }

        @Override
        protected void onProgressUpdate(Integer... values) {
            Log.e(TAG, "onProgressUpdate, values = " + values[0]);
        }

        @Override
        protected void onCancelled() {
            Log.e(TAG, "onCancelled");
        }

        @Override
        protected void onPostExecute(Integer integer) {
            Log.e(TAG, "onPostExecute result = " + integer);
        }

        @Override
        protected Integer doInBackground(Integer... params) throws InterruptedException {
            Log.e(TAG, "doInBackground start, thread = " + Thread.currentThread().getId() + ", " +
                    "params = " + params[0]);
            for (int i = 0; i < 5; ++i) {
                publishProgress(i);
                Thread.sleep(3000);
            }
            Log.e(TAG, "doInBackground end");
            return 10;
        }
    }

}
