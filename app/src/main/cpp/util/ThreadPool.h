/*
 * ThreadPool.h
 *
 *  Created on: 2016年9月11日
 *      Author: zhourui
 */

#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <atomic>
#include <condition_variable>
#include <memory>
#include <vector>

#include "public/define.h"
#include "RWLock.h"
#include "RWLockGuard.h"
#include "Noncopyable.h"
#include "Tasks.h"
#include "TaskThread.h"

namespace Dream {

using TaskThreadPtr = std::shared_ptr<TaskThread>;

class ThreadPool : public Noncopyable {
public:
  ThreadPool() = delete;
  ThreadPool(std::size_t size = THREAD_POOL_SIZE_DEFAULT, std::size_t maxSize =
      THREAD_POOL_MAX_SIZE_DEFAULT, int idleTime =
      THREAD_POOL_IDLE_TIME_DEFAULT);

  virtual ~ThreadPool();

  bool addTask(const Task& task);

  void start();
  void stop();

private:
  void handleTimeout(const TaskThreadPtr& threadPtr);

private:
  std::size_t _size;
  std::size_t _maxSize;
  int _idleTime;
  std::atomic<bool> _running;  // 用来允许或禁止添加任务
  std::vector<TaskThreadPtr> _pool;
  Tasks _tasks;
  RWLock _mutex;
};

} /* namespace Dream */

#endif /* THREADPOOL_H_ */
