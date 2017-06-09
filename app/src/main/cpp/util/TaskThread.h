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
        TaskThread(Tasks &tasks, int idleTime = -1,
                   std::function<void(const std::shared_ptr<TaskThread> &)> callback =
                   [](const std::shared_ptr<TaskThread> &) -> void { });

        virtual ~TaskThread();

        void start();  // 启动
        void restart();  // 重启
        void stop();  // 停止
        void weakUp();

        bool isWaiting();

    private:
        void run();

        void dtachAndroidThread(void *point);

    protected:
        ThreadPtr _threadPtr;
        std::mutex _mutex;
        bool _running;
        bool _waiting;
        bool _exited = false;
        Tasks &_tasks;
        std::condition_variable _condition;
        int _idleTime;  // 空闲回收时间，单位是秒
        std::function<void(const std::shared_ptr<TaskThread> &)> _timeoutCallback; // 空闲超时的回调
    };

} /* namespace Dream */

#endif /* TASKTHREAD_H_ */
