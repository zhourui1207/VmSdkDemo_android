/*
 * PriorityTaskQueue.cpp
 *
 *  Created on: 2016年9月12日
 *      Author: zhourui
 */

#include "PriorityTaskQueue.h"

namespace Dream {

    PriorityTaskQueue::PriorityTaskQueue(size_t maxSize, size_t warSize)
            : BaseTasks(maxSize, warSize) {

    }

    PriorityTaskQueue::~PriorityTaskQueue() {
    }

    bool PriorityTaskQueue::isEmpty() const {
        return _tasks.empty();
    }

    void PriorityTaskQueue::addTask(const TaskPtr &taskPtr) {
        std::lock_guard<std::mutex> lock(_mutex);
        time_point current = std::chrono::high_resolution_clock::now();
        taskPtr->pushTime(current);
        _tasks.push(taskPtr);
    }

    TaskPtr PriorityTaskQueue::removeTask() {
        std::lock_guard<std::mutex> lock(_mutex);
        TaskPtr taskPtr = _tasks.top();
        _tasks.pop();
        return taskPtr;
    }

} /* namespace Dream */
