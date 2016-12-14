package com.jxll.util;

import java.text.SimpleDateFormat;
import java.util.Date;

/**
 * Created by zhourui on 16/12/12.
 */

public class TimeUtil {
  /**
   * 时间戳转换成日期格式字符串
   * @param milliseconds 精确到毫秒的字符串
   * @param format
   * @return
   */
  public static String timeStamp2Date(long milliseconds, String format) {
    if(format == null || format.isEmpty()) format = "yyyy-MM-dd HH:mm:ss";
    SimpleDateFormat sdf = new SimpleDateFormat(format);
    return sdf.format(new Date(milliseconds));
  }

  /**
   * 日期格式字符串转换成时间戳
   * @param date 字符串日期
   * @param format 如：yyyy-MM-dd HH:mm:ss
   * @return
   */
  public static long date2TimeStamp(String date, String format) {
    try {
      SimpleDateFormat sdf = new SimpleDateFormat(format);
      return sdf.parse(date).getTime();
    } catch (Exception e) {
      e.printStackTrace();
    }
    return 0;
  }

  /**
   * 取得当前时间戳（精确到秒）
   * @return
   */
  public static String timeStamp(){
    long time = System.currentTimeMillis();
    String t = String.valueOf(time/1000);
    return t;
  }

  //  输出结果：
  //  timeStamp=1417792627
  //  date=2014-12-05 23:17:07
  //  1417792627
  public static void main(String[] args) {
    String timeStamp = timeStamp();
    System.out.println("timeStamp="+timeStamp);
  }
}
