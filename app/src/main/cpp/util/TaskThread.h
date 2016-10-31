/*
 * TaskThread.h
 *
 *  Created on: 2016年9月12日
 *      Author: zhourui
 */

#ifndef TASKTHREAD_H_
#define TASKTHREAD_H_

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

#include "Noncopyable.h"
#include "Tasks.h"

namespace Dream {

using ThreadPtr = std::unique_ptr<std::thread>;

class TaskThread : public Noncopyable, public std::enable_shared_from_this<TaskThread> {
public:
  TaskThread(Tasks& tasks, int idleTime = -1,
      std::function<void(const std::shared_ptr<TaskThread>&)> callback =
          [](const std::shared_ptr<TaskThread>&)->void {});

  virtual ~TaskThread();

  void start();  // 启动
  void restart();  // 重启
  void stop();  // 停止
  void weakUp() {  // 唤醒
    // 这里必须加上锁，不然会出现唤醒操作在线程等待前执行，那么那样线程会一直等待着，而线程池一直以为线程已经醒了，就无法给线程发放任务了
    std::unique_lock<std::mutex> lock { _mutex };
    _waiting.store(false);
    _condition.notify_one();
  }
  bool isWaiting() {
    return _waiting.load();
  }

private:
  void run();

protected:
  ThreadPtr _threadPtr;
  std::mutex _mutex;
  std::atomic<bool> _running;
  std::atomic<bool> _waiting;
  Tasks& _tasks;
  std::condition_variable _condition;
  int _idleTime;  // 空闲回收时间，单位是秒
  std::function<void(const std::shared_ptr<TaskThread>&)> _timeoutCallback; // 空闲超时的回调
};

} /* namespace Dream */

#endif /* TASKTHREAD_H_ */
