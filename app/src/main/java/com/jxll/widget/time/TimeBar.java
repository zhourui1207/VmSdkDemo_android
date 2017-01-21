package com.jxll.widget.time;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.RectF;
import android.text.TextPaint;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.View;

import com.jxll.util.DeviceUtil;
import com.jxll.util.TimeUtil;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Created by zhourui on 16/12/9.
 * 自定义录像时间轴控件
 */

public class TimeBar extends View {
  private static final long oneSecond = 1000;
  private static final long oneMinute = oneSecond * 60;
  private static final long oneHour = oneMinute * 60;
  private static final long oneDay = oneHour * 24;

  /**
   * 总毫秒数
   */
  private long totalMilliseconds = oneDay;

  /**
   * 起始时间
   */
  private long beginTime = TimeUtil.date2TimeStamp("2016-12-13 00:00:00", "yyyy-MM-dd HH:mm:ss");

  /**
   * 时间刻度map
   */
  private Map<Integer, TimeScale> timeScaleMap = new HashMap<>();

  /**
   * 录像单元列表
   */
  private List<RecordTimeCell> recordTimeCellList;

  /**
   * 最小宽度
   */
  private int minWidth;

  /**
   * 最大宽度
   */
  private int maxWidth;

  /**
   * 原宽度
   */
  private int srcWidth;

  /**
   * 原左坐标
   */
  private int srcLeft;

  /**
   * 当前缩放级别
   */
  private int currentScaleLevel;

  /**
   * 当前时间
   */
  private long currentTime = beginTime;

  /**
   * 当前控件实际宽度
   */
  private int currentWidth;

  /**
   * 记录最后的x坐标位置
   */
  private int lastX = 0;

  /**
   * 缩放检测器
   */
  private ScaleGestureDetector scaleGestureDetector;

  /**
   * 用户对时间轴的操作模式
   */
  private static final int NONE = 0;
  private static final int DRAG = 1;
  private static final int ZOOM = 2;
  private int mode = NONE;

  /**
   * 当前时间变化侦听器
   */
  public interface CurrentTimeChangListener {
    void onCurrentTimeChanging(long currentTime);

    void onCurrentTimeChanged(long currentTime);
  }

  private CurrentTimeChangListener currentTimeChangListener;

  public void setCurrentTimeChangListener(CurrentTimeChangListener currentTimeChangListener) {
    this.currentTimeChangListener = currentTimeChangListener;
  }

  /**
   * 中心刻度
   */
  Paint centerScalePaint = new Paint();

  /**
   * 中心刻度上的三角形
   */
  Path centerScalePath = new Path();

  /**
   * 关键刻度
   */
  Paint keyScalePaint = new Paint();

  Paint minScalePaint = new Paint();

  TextPaint keyScaleTextPaint = new TextPaint();

  /**
   * 下面是关于界面的一些参数
   */

  /**
   * 颜色配置
   */
  private int centerScaleColor = Color.argb(255, 255, 0, 0);
  private int borderColor = Color.rgb(200, 200, 200);
  private int keyScaleColor = Color.rgb(200, 200, 200);
  private int minScaleColor = Color.rgb(200, 200, 200);
  private int backgroundColor = Color.WHITE;
  private int textColor = Color.GRAY;

  /**
   * 线的粗细设置
   */
  private int centerScaleWidth = 1;
  private int triangleWidth = 10;
  private int triangleHeight = 6;
  private int borderDpWidth = 1;
  private int keyScaleDpWidth = 1;
  private int keyScaleDpHeight = 10;
  private int minScaleDpWidth = 1;
  private int minScaleDpHeight = 6;

  /**
   * 文字到上边关键刻度的距离
   */
  private int textToTopkeyScaleDpDistance = 14;

  /**
   * 字体大小
   */
  private int textSpSize = 12;

  public TimeBar(Context context) {
    super(context);
    init();
  }

  public TimeBar(Context context, AttributeSet attrs) {
    super(context, attrs);
    init();
  }

  public TimeBar(Context context, AttributeSet attrs, int defStyleAttr) {
    super(context, attrs, defStyleAttr);
    init();
  }

  // 初始化
  private void init() {
//    initTimeScaleMap()
    ScaleGestureDetector.OnScaleGestureListener scaleGestureListener = new ScaleGestureDetector
        .OnScaleGestureListener() {
      @Override
      public boolean onScale(ScaleGestureDetector detector) {
        scaleTimebarByFactor(detector.getScaleFactor());
        return true;
      }

      @Override
      public boolean onScaleBegin(ScaleGestureDetector detector) {
        return true;
      }

      @Override
      public void onScaleEnd(ScaleGestureDetector detector) {

      }
    };
    scaleGestureDetector = new ScaleGestureDetector(getContext(), scaleGestureListener);
  }

  /**
   * 初始化时间刻度map
   */
  private void initTimeScaleMap() {
    if (timeScaleMap.size() > 0) {
      return;
    }

    TimeScale t1 = new TimeScale();
    t1.setTotalMillisecondsInOneScreen(30 * oneMinute);
    t1.setKeyScaleInterval(10 * oneMinute);
    t1.setMinScaleInterval(2 * oneMinute);
    t1.setDataFormat("HH:mm");
    t1.setDisplayTextInterval(0);
    t1.setViewLength((int) (getWidth() * (float) (totalMilliseconds / t1
        .getTotalMillisecondsInOneScreen()) + 0.5));
    timeScaleMap.put(1, t1);

    TimeScale t2 = new TimeScale();
    t2.setTotalMillisecondsInOneScreen(3 * oneHour);
    t2.setKeyScaleInterval(1 * oneHour);
    t2.setMinScaleInterval(10 * oneMinute);
    t2.setDataFormat("HH:mm");
    t2.setDisplayTextInterval(0);
    t2.setViewLength((int) (getWidth() * (float) (totalMilliseconds / t2
        .getTotalMillisecondsInOneScreen()) + 0.5));
    timeScaleMap.put(2, t2);

    TimeScale t3 = new TimeScale();
    t3.setTotalMillisecondsInOneScreen(6 * oneHour);
    t3.setKeyScaleInterval(1 * oneHour);
    t3.setMinScaleInterval(10 * oneMinute);
    t3.setDataFormat("HH:mm");
    t3.setDisplayTextInterval(1);
    t3.setViewLength((int) (getWidth() * (float) (totalMilliseconds / t3
        .getTotalMillisecondsInOneScreen()) + 0.5));
    timeScaleMap.put(3, t3);

    TimeScale t4 = new TimeScale();
    t4.setTotalMillisecondsInOneScreen(12 * oneHour);
    t4.setKeyScaleInterval(1 * oneHour);
    t4.setMinScaleInterval(0);
    t4.setDataFormat("HH:mm");
    t4.setDisplayTextInterval(1);
    t4.setViewLength((int) (getWidth() * (float) (totalMilliseconds / t4
        .getTotalMillisecondsInOneScreen()) + 0.5));
    timeScaleMap.put(4, t4);

    TimeScale t5 = new TimeScale();
    t5.setTotalMillisecondsInOneScreen(24 * oneHour);
    t5.setKeyScaleInterval(1 * oneHour);
    t5.setMinScaleInterval(0);
    t5.setDataFormat("HH:mm");
    t5.setDisplayTextInterval(3);
    t5.setViewLength((int) (getWidth() * (float) (totalMilliseconds / t5
        .getTotalMillisecondsInOneScreen()) + 0.5));
    timeScaleMap.put(5, t5);

    // 默认缩放级别是第二个
    currentScaleLevel = 4;

    changeWidth(timeScaleMap.get(currentScaleLevel).getViewLength());
  }

  public void setTotalMilliseconds(long totalMilliseconds) {
    this.totalMilliseconds = totalMilliseconds;
  }

  public long getTotalMilliseconds() {
    return totalMilliseconds;
  }

  public void setCurrentScaleLevel(int scaleLevel) {
    currentScaleLevel = scaleLevel;
  }

  public int getCurrentScaleLevel() {
    return currentScaleLevel;
  }

  public long getBeginTime() {
    return beginTime;
  }

  public void setBeginTime(long beginTime) {
    this.beginTime = beginTime;
  }

  public void setCurrentTime(long time) {
    long offsetTime = time - beginTime;
    if (offsetTime < 0 || offsetTime > totalMilliseconds) {
      return;
    }
    currentTime = time;
    int left = computeLeft(offsetTime, getWidth() - srcWidth);
    layout(left, getTop(), left + getWidth(), getBottom());
  }

  public long getCurrentTime() {
    return currentTime;
  }

  public void setRecordList(List<RecordTimeCell> recordList) {
    recordTimeCellList = recordList;
  }

  public void addRecord(RecordTimeCell record) {
    if (recordTimeCellList == null) {
      recordTimeCellList = new ArrayList<>();
    }
    recordTimeCellList.add(record);
  }

  public void clearRecord() {
    if (recordTimeCellList != null) {
      recordTimeCellList.clear();
      recordTimeCellList = null;
    }
  }

  /**
   * 计算当前时间
   * left: 左边界位置
   * currentWidth: 当前宽度
   * zeroWidth: 起点位置(指示时间点位置)
   */
  private long computeOffestTime(int left, int timeScaleWidth) {
    // 相对起止点左偏移的位置
    int offsetLeft = -left;
    // 得出偏移时间
    long offsetTime = (long) ((offsetLeft * 1.0f / timeScaleWidth) * totalMilliseconds);
    return offsetTime;
  }

  /**
   * 计算左坐标
   *
   * @param offsetTime
   * @return
   */
  private int computeLeft(long offsetTime, int timeScaleWidth) {
    // 相对起始点左偏移位置
    int offsetLeft = (int) (offsetTime * 1.0f / totalMilliseconds * timeScaleWidth);
    // 刻度位置向左偏移的位置
    int left = -offsetLeft;
    return left;
  }

  @Override
  protected void onDraw(Canvas canvas) {
    super.onDraw(canvas);

//    Log.e("onDraw", "currentTime=" + currentTime);

    // 画背景
    canvas.drawColor(backgroundColor);

    // 画外边框
    RectF topBorderRect = new RectF(0, 0, getWidth(), DeviceUtil.dip2px(borderDpWidth));
    Paint topBorderPaint = new Paint();
    topBorderPaint.setColor(borderColor);
    canvas.drawRect(topBorderRect, topBorderPaint);

    RectF bottomBorderRect = new RectF(0, getHeight() - DeviceUtil.dip2px(borderDpWidth),
        getWidth(), getHeight());
    Paint bottomBorderPaint = new Paint();
    bottomBorderPaint.setColor(borderColor);
    canvas.drawRect(bottomBorderRect, bottomBorderPaint);

    // 计算每毫秒多少长度
    float lenghtOfOneMillsecond = (getWidth() - srcWidth) * 1f / totalMilliseconds;

    // 画录像区域
    if (recordTimeCellList != null && recordTimeCellList.size() > 0) {
      for (RecordTimeCell cell : recordTimeCellList) {
        long begin = cell.getBeginTime();
        long end = cell.getEndTime();
        if (begin >= beginTime + totalMilliseconds || end <= beginTime) {  // 录像时间超出时间轴可显示时间
          continue;
        } else {
          float left = lenghtOfOneMillsecond * (begin - beginTime) + srcWidth / 2f;
          float right = lenghtOfOneMillsecond * (end - beginTime) + srcWidth / 2f;
          RectF recordRect = new RectF(left, DeviceUtil.dip2px(borderDpWidth), right,
              getHeight() - DeviceUtil.dip2px(borderDpWidth));
          Paint recordPaint = new Paint();
          recordPaint.setColor(cell.getColor());
          canvas.drawRect(recordRect, recordPaint);
        }
      }
    }

    // --------------------------- 画刻度开始 -------------------------------
    TimeScale timeScale = timeScaleMap.get(currentScaleLevel);

    if (timeScale != null) {
      // 获取可见范围
      long[] showRange = timeScaleShowRange(lenghtOfOneMillsecond);

      // 关键刻度
      if (timeScale.getKeyScaleInterval() > 0) {
        keyScalePaint.setColor(keyScaleColor);
        keyScaleTextPaint.setColor(textColor);
        keyScaleTextPaint.setAntiAlias(true);  // 消除锯齿
        keyScaleTextPaint.setTextSize(DeviceUtil.dip2px(textSpSize));

        int displayTextInterval = timeScale.getDisplayTextInterval() > 0 ? timeScale
            .getDisplayTextInterval() : 0;

        // 计算起始索引，优化速度
        int begin = (int) (showRange[0] / timeScale.getKeyScaleInterval() - 1);
        if (begin < 0) {
          begin = 0;
        }
        int currentDisplay = displayTextInterval > 0 ? begin % (displayTextInterval + 1) : 0;

        int lastIndex = (int) (totalMilliseconds / timeScale.getKeyScaleInterval());
        for (int i = begin; i < lastIndex + 1; ++i) {
          long tmpOffset = i * timeScale.getKeyScaleInterval();
          // 加减一个关键刻度是为了显示边缘的时间字
//          if (tmpOffset < showRange[0] - timeScale.getKeyScaleInterval()) {
//            // 间隔空白的关键刻度
//            if (++currentDisplay > displayTextInterval) {
//              currentDisplay = 0;
//            }
//            continue;
//          } else
          if (tmpOffset > showRange[1] + timeScale.getKeyScaleInterval()) {
            break;
          }
          if (true) {  // 判断是否在可见区域内
            // 上方刻度
            float left = lenghtOfOneMillsecond * tmpOffset + srcWidth / 2f - DeviceUtil.dip2px(keyScaleDpWidth) / 2f;

            RectF topKeyScaleRect = new RectF(left, DeviceUtil.dip2px
                (borderDpWidth), left + DeviceUtil.dip2px(keyScaleDpWidth),
                DeviceUtil.dip2px(borderDpWidth + keyScaleDpHeight));
            canvas.drawRect(topKeyScaleRect, keyScalePaint);

            // 下方刻度
            RectF bottomKeyScaleRect = new RectF(left, getHeight() - DeviceUtil.dip2px
                (borderDpWidth + keyScaleDpHeight), left + DeviceUtil.dip2px(keyScaleDpWidth),
                getHeight() - DeviceUtil.dip2px(borderDpWidth));
            canvas.drawRect(bottomKeyScaleRect, keyScalePaint);

            if (currentDisplay == 0) {
              // 文字
              String keytext = getTimeStringFromLong(timeScale.getDataFormat(), i * timeScale
                  .getKeyScaleInterval() + beginTime);
              if (i == lastIndex && keytext.equalsIgnoreCase("00:00")) {
                keytext = "24:00";
              }

              float keyTextWidth = keyScaleTextPaint.measureText(keytext);
              float keytextX = left - keyTextWidth / 2;

              canvas.drawText(keytext, keytextX, DeviceUtil.dip2px
                      (borderDpWidth + keyScaleDpHeight + textToTopkeyScaleDpDistance),
                  keyScaleTextPaint);

            }

            // 间隔空白的关键刻度
            if (++currentDisplay > displayTextInterval) {
              currentDisplay = 0;
            }
          }
        }
      }

      // 小刻度
      if (timeScale.getMinScaleInterval() > 0) {
        minScalePaint.setColor(minScaleColor);

        int begin = (int) (showRange[0] / timeScale.getMinScaleInterval());
        if (begin < 0) {
          begin = 0;
        }

        int lastIndex = (int) (totalMilliseconds / timeScale.getMinScaleInterval());
        for (int i = begin; i < lastIndex; ++i) {
          long tmpOffset = i * timeScale.getMinScaleInterval();
//          if (tmpOffset < showRange[0]) {
//            continue;
//          } else
          if (tmpOffset > showRange[1]) {
            break;
          }

          if (true && (tmpOffset % timeScale.getKeyScaleInterval() != 0)) {
            float left = lenghtOfOneMillsecond * tmpOffset + srcWidth / 2f - DeviceUtil.dip2px(minScaleDpWidth) / 2f;
            // 上方刻度
            RectF topMinScaleRect = new RectF(left, DeviceUtil.dip2px
                (borderDpWidth), left + DeviceUtil.dip2px(minScaleDpWidth),
                DeviceUtil.dip2px(borderDpWidth + minScaleDpHeight));
            canvas.drawRect(topMinScaleRect, minScalePaint);

            // 下方刻度
            RectF bottomMinScaleRect = new RectF(left, getHeight() - DeviceUtil.dip2px
                (borderDpWidth + minScaleDpHeight), left + DeviceUtil.dip2px(minScaleDpWidth),
                getHeight() - DeviceUtil.dip2px(borderDpWidth));
            canvas.drawRect(bottomMinScaleRect, minScalePaint);
          }
        }
      }
    }
    //---------------------- 画刻度结束 -------------------------

    //---------------------- 画中心刻度开始 ----------------------
    float center = lenghtOfOneMillsecond * (currentTime - beginTime) + srcWidth * 0.5f;
    float centerScaleleft = center - DeviceUtil.dip2px(centerScaleWidth) * 0.5f;
//    RectF centerScaleRect = new RectF(centerScaleleft, DeviceUtil.dip2px(borderDpWidth + triangleHeight),
//        centerScaleleft + DeviceUtil.dip2px(centerScaleWidth),
//        getHeight() - DeviceUtil.dip2px(borderDpWidth));
    centerScalePaint.setColor(centerScaleColor);
    centerScalePaint.setAntiAlias(true);  // 消除锯齿
//    canvas.drawRect(centerScaleRect, centerScalePaint);
    // 上方的三角形， 现在将三角形和下放矩形一起画出来，分开话的话，在连接处会出现类似错开的感觉，强迫症表示难受
    centerScalePath.reset();
    centerScalePath.moveTo(centerScaleleft, DeviceUtil.dip2px(borderDpWidth + triangleHeight));
    centerScalePath.lineTo(centerScaleleft, getHeight() - DeviceUtil.dip2px(borderDpWidth));
    centerScalePath.lineTo(centerScaleleft + DeviceUtil.dip2px(centerScaleWidth), getHeight() - DeviceUtil.dip2px(borderDpWidth));
    centerScalePath.lineTo(centerScaleleft + DeviceUtil.dip2px(centerScaleWidth), DeviceUtil.dip2px(borderDpWidth + triangleHeight));
    centerScalePath.lineTo(center + DeviceUtil.dip2px(triangleWidth) * 0.5f, DeviceUtil.dip2px(borderDpWidth));
    centerScalePath.lineTo(center - DeviceUtil.dip2px(triangleWidth) * 0.5f, DeviceUtil.dip2px(borderDpWidth));
    centerScalePath.close();
    canvas.drawPath(centerScalePath, centerScalePaint);
    //---------------------- 画中心刻度结束 ----------------------

//    Log.e("onDraw", "onDraw end!");
  }

  /**
   * 获取当前可见刻度区域
   */
  private long[] timeScaleShowRange(float lenghtOfOneMillsecond) {
    // 控件当前最多能显示的毫秒数
    long maxShowMillseconds = (long) (srcWidth * 1.0f / lenghtOfOneMillsecond);
    // 这是在控件中点的时间刻度
    long offsetTime = currentTime - beginTime;

    long leftTimeScale = offsetTime - maxShowMillseconds / 2;
    if (leftTimeScale < 0) {
      leftTimeScale = 0;
    }
    long rightTimeScale = offsetTime + maxShowMillseconds / 2;
    if (rightTimeScale > beginTime + totalMilliseconds) {
      rightTimeScale = beginTime + totalMilliseconds;
    }
    long[] retRange = new long[2];
    retRange[0] = leftTimeScale;
    retRange[1] = rightTimeScale;
    return retRange;
  }

  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
//    super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    setMeasuredDimension(measureWidth(widthMeasureSpec), getDefaultSize(getSuggestedMinimumHeight
        (), heightMeasureSpec));
  }

  private int measureWidth(int widthMeasureSpec) {
    int measureMode = MeasureSpec.getMode(widthMeasureSpec);
    int measureSize = MeasureSpec.getSize(widthMeasureSpec);

    int result = getSuggestedMinimumWidth();
    switch (measureMode) {
      case MeasureSpec.AT_MOST:
      case MeasureSpec.EXACTLY:
        result = measureSize;
        break;
      default:
        break;
    }

    if (currentWidth > 0) {
      return currentWidth;
    }
    return result;
  }

  public void scaleTimebarByFactor(float scaleFactor) {
    int newWidth = (int) ((getWidth() - srcWidth) * scaleFactor + 0.5);

    changeWidth(newWidth);
  }

  private void changeWidth(int width) {
    if (width < getMinWidth()) {
      width = getMinWidth();
      currentScaleLevel = 5;
    } else if (width >= getMinWidth() && width < betweenTwoScaleWidth(5, 4)) {
      currentScaleLevel = 5;
    } else if (width >= betweenTwoScaleWidth(5, 4) && width < betweenTwoScaleWidth(4, 3)) {
      currentScaleLevel = 4;
    } else if (width >= betweenTwoScaleWidth(4, 3) && width < betweenTwoScaleWidth(3, 2)) {
      currentScaleLevel = 3;
    } else if (width >= betweenTwoScaleWidth(3, 2) && width < betweenTwoScaleWidth(2, 1)) {
      currentScaleLevel = 2;
    } else if (width >= betweenTwoScaleWidth(2, 1) && width < getMaxWidth()) {
      currentScaleLevel = 1;
    } else if (width >= getMaxWidth()) {
      width = getMaxWidth();
      currentScaleLevel = 1;
    }

    currentWidth = width + srcWidth;

    int l = computeLeft(currentTime - beginTime, width) + srcLeft;
    int r = l + width + srcWidth;
    int t = getTop();
    int b = getBottom();
    layout(l, t, r, b);
//
//    ViewGroup.LayoutParams params = getLayoutParams();
//    params.width = width + srcWidth;
//    setLayoutParams(params);
  }

  private int betweenTwoScaleWidth(int scale1, int scale2) {
    if ((scale1 == 1 && scale2 == 2) || (scale2 == 1 && scale1 == 2)) {
      return (timeScaleMap.get(scale1).getViewLength() + timeScaleMap.get(scale2).getViewLength()
      ) / 4;
    }
    return (timeScaleMap.get(scale1).getViewLength() + timeScaleMap.get(scale2).getViewLength())
        / 2;
  }

  private int getMinWidth() {
    if (minWidth <= 0) {
      minWidth = timeScaleMap.get(5).getViewLength();
    }
    return minWidth;
  }

  private int getMaxWidth() {
    if (maxWidth <= 0) {
      maxWidth = timeScaleMap.get(1).getViewLength();
    }
    return maxWidth;
  }

  @Override
  protected void onSizeChanged(int w, int h, int oldw, int oldh) {
    super.onSizeChanged(w, h, oldw, oldh);

    if (w != 0 && h != 0) {
      initVariable();
      initTimeScaleMap();
    }
  }

  private void initVariable() {
    if (srcWidth <= 0) {
      srcWidth = getWidth();
    }
    if (srcLeft <= 0) {
      srcLeft = getLeft();
    }
  }

  @Override
  public boolean onTouchEvent(MotionEvent event) {
    scaleGestureDetector.onTouchEvent(event);

    if (scaleGestureDetector.isInProgress()) {
      return true;
    }

    switch (event.getAction() & MotionEvent.ACTION_MASK) {
      case MotionEvent.ACTION_DOWN:
        mode = DRAG;
        lastX = (int) event.getRawX();
        break;
      case MotionEvent.ACTION_POINTER_DOWN:
        mode = ZOOM;
        break;
      case MotionEvent.ACTION_MOVE:
        if (mode == DRAG) {
          int xMoved = (int) (event.getRawX() - lastX);

          int l = getLeft() + xMoved;
          int r = getRight() + xMoved;

          if (l > srcLeft) {  // 不可再向右滑动
            l = srcLeft;
            r = l + getWidth();
          } else if (r < srcLeft + srcWidth) {  // 不可再向左滑动
            r = srcLeft + srcWidth;
            l = r - getWidth();
          }

          int t = getTop();
          int b = getBottom();
          layout(l, t, r, b);
          currentTime = computeOffestTime(l - srcLeft, r - l - srcWidth) + beginTime;
          if (currentTimeChangListener != null) {
            currentTimeChangListener.onCurrentTimeChanging(currentTime);
          }
          invalidate();

          lastX = (int) event.getRawX();
        }
        break;
      case MotionEvent.ACTION_UP:
        if (mode == DRAG && currentTimeChangListener != null) {
          currentTimeChangListener.onCurrentTimeChanged(currentTime);
        }
        mode = NONE;
        break;
    }
    return true;
  }

  private String getTimeStringFromLong(String format, long value) {
    SimpleDateFormat timeFormat = new SimpleDateFormat(format);
    return timeFormat.format(value);
  }
}