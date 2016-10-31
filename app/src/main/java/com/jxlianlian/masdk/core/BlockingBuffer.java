package com.jxlianlian.masdk.core;

import android.util.Log;

import java.util.concurrent.LinkedBlockingQueue;

/**
 * Created by zhourui on 16/10/28.
 * 阻塞缓存，若数据为空时，获取数据可以是阻塞的，可以有效减小线程的空循环从而提高性能
 */

public class BlockingBuffer {
  private String TAG = "BlockingBuffer";

  private int maxSize;  // 最大大小
  private int warningSize;  // 警告大小
  private LinkedBlockingQueue linkedBlockingQueue;  // 这是有序的阻塞队列
  private Object mutex = new Object();

  public BlockingBuffer() {
    init(1000, 800);
  }

  public BlockingBuffer(int maxSize) {
    maxSize = maxSize < 1 ? 1 : maxSize;
    init(maxSize, maxSize);
  }

  public BlockingBuffer(int maxSize, int warningSize) {
    maxSize = maxSize < 1 ? 1 : maxSize;
    warningSize = warningSize < 1 ? 1 : warningSize;
    init(maxSize, warningSize);
  }

  private void init(int maxSize, int warningSize) {
    this.maxSize = maxSize;
    this.warningSize = warningSize;
    linkedBlockingQueue = new LinkedBlockingQueue(maxSize);
  }

  public int size() {
    synchronized (mutex) {
      return linkedBlockingQueue.size();
    }
  }

  public boolean isEmpty() {
    synchronized (mutex) {
      return linkedBlockingQueue.isEmpty();
    }
  }

  /**
   * 增加对象，若队列已满，则添加失败，返回false，成功则返回true
   * @param object
   * @return
   */
  public boolean addObject(Object object) {
    if (object == null) {
      return false;
    }
    boolean ret = false;
    synchronized (mutex) {
      int currentSize = linkedBlockingQueue.size();

      // 若当前大小小于最大大小，则将数据保存
      if (currentSize < maxSize) {
        linkedBlockingQueue.add(object);
        ret = true;
        ++currentSize;
      }

      // 当前大小大于等于警告大小时，打印告警信息
      if (currentSize >= warningSize) {
        Log.w(TAG, "BlockingBuffer缓存大小已经到达告警阀值！当前大小为 [" + currentSize + "]!");
      }
    }
    return ret;
  }


  /**
   * 强制添加对象，永远都是成功的，若强制覆盖第一个则返回true，否则返回false
   * @param object
   * @return
   */
  public boolean addObjectForce(Object object) {
    if (object == null) {
      return false;
    }
    boolean ret = false;
    synchronized (mutex) {
      int currentSize = linkedBlockingQueue.size();

      // 若当前大小小于最大大小，则将数据保存
      if (currentSize < maxSize) {
        ++currentSize;
      } else {  // 否则就将第一个最久的数据顶掉
        linkedBlockingQueue.poll();
        ret = true;
      }
      linkedBlockingQueue.add(object);

      // 当前大小大于等于警告大小时，打印告警信息
      if (currentSize >= warningSize) {
        Log.w(TAG, "BlockingBuffer缓存大小已经到达告警阀值！当前大小为 [" + currentSize + "]!");
      }
    }
    return ret;
  }

  /**
   * 获取并移除对象，若缓存中无对象，则返回空
   * @return
   */
  public Object removeObject() {
    Object ret = null;
    synchronized (mutex) {
      if (linkedBlockingQueue.size() > 0) {
        ret = linkedBlockingQueue.poll();
      }
    }
    return ret;
  }

  /**
   * 阻塞方式获取并移除对象，若缓存中无对象，则阻塞
   * @return
   * @throws InterruptedException 被强制中断时，返回中断异常
   */
  public Object removeObjectBlocking() throws InterruptedException {
    Object ret = null;
    ret = linkedBlockingQueue.take();
    return ret;
  }

  /**
   * 清空缓存
   */
  public void clear() {
    synchronized (mutex) {
      linkedBlockingQueue.clear();
    }
  }
}
