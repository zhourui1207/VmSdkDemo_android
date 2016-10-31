/*
 * PriorityTaskQueue.h
 *
 *  Created on: 2016年9月12日
 *      Author: zhourui
 *
 *      优先级任务队列
 */

#ifndef PRIORITYTASKQUEUE_H_
#define PRIORITYTASKQUEUE_H_

#include "BaseTasks.h"

namespace Dream {

// 比较函数
struct cmp {
  bool operator()(const TaskPtr& a, const TaskPtr& b) const {
    return *(a) < *(b);
  }
};

using PriorityQueue = std::priority_queue<TaskPtr, std::vector<TaskPtr>, cmp>;

class PriorityTaskQueue: public BaseTasks {
public:
  PriorityTaskQueue(size_t maxSize = QUEUE_MAX_SIZE_DEFAULT, size_t warSize =
      QUEUE_WAR_SIZE_DEFAULT);
  virtual ~PriorityTaskQueue();

  bool isEmpty() const override;
  void addTask(const TaskPtr& taskPtr) override;
  TaskPtr removeTask() override;
private:
  PriorityQueue _tasks;
};

} /* namespace Dream */

#endif /* PRIORITYTASKQUEUE_H_ */
