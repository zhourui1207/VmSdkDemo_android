/*
 * Task.h
 *
 *  Created on: 2016年9月12日
 *      Author: zhourui
 *      任务类
 */

#ifndef TASK_H_
#define TASK_H_

#include <chrono>
#include <string>

namespace Dream {

    enum TaskPriority {
        LOW = -1,
        DEFAULT = 0,
        HIGH = 1
    };

    using time_point = std::chrono::high_resolution_clock::time_point;

    class Task {
    public:
        Task(TaskPriority priority)
                : _priority(priority) {

        }

        virtual ~Task();

        // 派生类重写该方法，任务线程会调用该方法做任务
        virtual int doTask() = 0;

        // 派生类重写任务类型，一般用于调试打印
        virtual const std::string taskType() const = 0;

        void printTaskType() const;

        void pushTime(const time_point &time);

        TaskPriority getPriority() { return _priority; }

        bool operator<(const Task &other) const {
            if (this == &other) {
                return false;
            }
            // 优先级别相同的话，就按push到队列的时间来排序
            if (_priority == other._priority) {
                return _timePoint > other._timePoint;
            }
            return _priority < other._priority;
        }

    private:
        TaskPriority _priority;
        time_point _timePoint;
    };

} /* namespace Dream */

#endif /* TASK_H_ */
