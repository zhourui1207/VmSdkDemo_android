package com.jxll.vmnetsdktest;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.os.AsyncTask;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.TextView;

import com.jxll.util.TimeUtil;
import com.jxll.vmsdk.Channel;
import com.jxll.vmsdk.ChannelsHolder;
import com.jxll.vmsdk.DepTree;
import com.jxll.vmsdk.DepTreesHolder;
import com.jxll.vmsdk.Record;
import com.jxll.vmsdk.RecordsHolder;
import com.jxll.vmsdk.TalkAddressHolder;
import com.jxll.vmsdk.VmNet;
import com.jxll.vmsdk.VmPlayer;
import com.jxll.vmsdk.util.OpenGLESUtil;
import com.jxll.vmsdk.util.opengles.GLFrameRenderer;
import com.jxll.widget.time.RecordTimeCell;
import com.jxll.widget.time.TimeBar;

import java.util.ArrayList;
import java.util.List;

public class MainActivity extends AppCompatActivity implements VmNet.ServerConnectStatusCallback {

  private String TAG = "MainActivity";

  private static SurfaceView surfaceView;
  private SurfaceView surfaceView2;
  private SurfaceView surfaceView4;
  private SurfaceView surfaceView5;
  private SurfaceView surfaceView6;
  private SurfaceView surfaceView7;
  private SurfaceView surfaceView8;
  private SurfaceView surfaceView9;
  private SurfaceView surfaceView3;
  private TimeBar timeBar;
  GLFrameRenderer mGLFRenderer;
  private Button button;
  private Button record;
  private List<RecordTimeCell> recordTimeCellList = new ArrayList<>();
  TextView textView;

  private static int id = 0;

  public GLFrameRenderer getGLFrameRenderer() {
    return mGLFRenderer;
  }

  boolean audioOpen = false;

  boolean recording = false;

  VmPlayer player;

  public static Surface getNativeSurface() {
    Log.e("!!", "getNativeSurface");
    return surfaceView.getHolder().getSurface();
  }

  /**
   * This method is called by SDL using JNI.
   */
  public static int audioOpen(int sampleRate, boolean is16Bit, boolean isStereo, int desiredFrames) {
    Log.e("!!", "audioOpen");
    return 0;
  }

  public static void audioWriteShortBuffer(short[] buffer) {
    Log.e("!!", "audioWriteShortBuffer");
  }

  public static void audioWriteByteBuffer(byte[] buffer) {
    Log.e("!!", "audioWriteByteBuffer");
  }

  public static int captureOpen(int sampleRate, boolean is16Bit, boolean isStereo, int desiredFrames) {
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

  /** This method is called by SDL using JNI. */
  public static void audioClose() {
    Log.e("!!", "audioClose");
  }

  /** This method is called by SDL using JNI. */
  public static void captureClose() {
    Log.e("!!", "captureClose");
  }

  /**
   * This method is called by SDL using JNI.
   */
  public static void pollInputDevices() {
    Log.e("!!", "pollInputDevices");
  }

  class SDLMain implements Runnable {
    @Override
    public void run() {
      // Runs SDL_main()

      //Log.v("SDL", "SDL thread terminated");
    }
  }

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON, WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);  //保持屏幕常亮
    setContentView(R.layout.activity_main);

    surfaceView = (SurfaceView) findViewById(R.id.surfaceView);
    textView = (TextView) findViewById(R.id.tv);
    timeBar = (TimeBar) findViewById(R.id.tb);
    timeBar.setCurrentTimeChangListener(new TimeBar.CurrentTimeChangListener() {
      @Override
      public void onCurrentTimeChanging(long currentTime) {
        textView.setText(TimeUtil.timeStamp2Date(currentTime, null));
      }

      @Override
      public void onCurrentTimeChanged(long currentTime) {
        textView.setText(TimeUtil.timeStamp2Date(currentTime, null));
      }
    });


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
    });

    record = (Button) findViewById(R.id.record);
    record.setOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View view) {
        timeBar.invalidate();
      }
//        if (recording) {
//          player.stopRecord();
//          recording = false;
//          record.setText("开始录像");
//        } else {
//          recording = player.startRecord("/sdcard/MVSS/Localrecord/test1.flv");
//          if (recording) {
//            record.setText("停止录像");
//          }
//        }
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
//    final Thread sdlThread = new Thread(new SDLMain(), "SDLThread");
//    sdlThread.start();
  }

  @Override
  public void onServerConnectStatus(boolean isConnected) {
    Log.e(TAG, "服务器连接状态:" + isConnected);
  }

  class RealplayTask extends AsyncTask<Void, Void, Void> {

    @Override
    protected Void doInBackground(Void... voids) {
      VmNet.init(10);
      VmNet.connect("192.168.1.113", 5516, MainActivity.this);
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
            Log.e(TAG, "depName=" + depTree.getDepName() + ", depId=" + depTree.getDepId() + ", parentId=" + depTree.getParentId());
          }
        }

        ChannelsHolder channelsHolder = new ChannelsHolder();
        ret = VmNet.getChannels(1, 100, 0, channelsHolder);
        if (ret == 0 && channelsHolder.size() > 0) {
          List<Channel> channels = channelsHolder.list();
          for (Channel channel : channels) {
            Log.e(TAG, "channelName=" + channel.getChannelName() + ", fdId=" + channel.getFdId() + ", channelId=" + channel.getChannelId());
          }
        }

      } else {
        Log.e(TAG, "登录失败!");
      }

      VmNet.startReceiveRealAlarm(new VmNet.RealAlarmCallback() {
        @Override
        public void onRealAlarm(String fdId, int channelId, int alarmType, int param1, int param2) {
          Log.e(TAG, "onRealAlarm");
          NotificationManager manager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);

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

          notify3.flags |= Notification.FLAG_AUTO_CANCEL; // FLAG_AUTO_CANCEL表明当通知被用户点击时，通知将被清除。
          manager.notify(id++, notify3);// 步骤4：通过通知管理器来发起通知。如果id不同，则每click，在status哪里增加一个提
        }
      });

      TalkAddressHolder talkAddressHolder = new TalkAddressHolder();
      ret = VmNet.startTalk("201607201402389091", 1, talkAddressHolder);
      if (ret == 0) {
        Log.e(TAG, "onRealAlarm addr=" + talkAddressHolder.getTalkAddr() + ", port=" + talkAddressHolder.getTalkPort());
      } else {
        Log.e(TAG, "startTalk error");
      }

      RecordsHolder recordsHolder = new RecordsHolder();
      long currentTime = System.currentTimeMillis();
      Log.e("currentTime", "currentTime=" + currentTime);
      ret = VmNet.getRecords(0, 100, "201610111654538071", 1, (int)(currentTime / 1000) - 7200, (int)(currentTime / 1000), true, recordsHolder);

      Log.e("getRecords", "getRecords ret = " + ret + ", size=" + recordsHolder.size());
      if (ret == 0) {
        recordTimeCellList.clear();
        for (Record record : recordsHolder.list()) {
          RecordTimeCell recordTimeCell = new RecordTimeCell(record.getBeginTime()*1000L, record.getEndTime()*1000L, Color.rgb(150, 255, 150));
          recordTimeCellList.add(recordTimeCell);
        }
      }
      return null;
    }

    @Override
    protected void onPostExecute(Void aVoid) {
      timeBar.setRecordList(recordTimeCellList);
//      player = new VmPlayer();
////      player.startRealplay("201607201402389091", 1, true, 1, false, false, surfaceView.getHolder(), MainActivity.this);
//      player.startRealplay("201610111654538071", 1, true, 0, false, false, surfaceView.getHolder(), MainActivity.this);
//      player.startPlayback("201610111654538071", 1, true, 1481242200, 1481299200, 0, false, false, surfaceView.getHolder(), MainActivity.this);

    }
  }

}
