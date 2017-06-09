/*
 * TaskQueue.h
 *
 *  Created on: 2016年9月12日
 *      Author: zhourui
 *      任务队列类
 */

#ifndef TASKS_H_
#define TASKS_H_

#include <functional>
#include <future>
#include <mutex>
#include <queue>

#include "public/define.h"
#include "Noncopyable.h"

namespace Dream {

    using Task = std::function<void()>;

    class Tasks : public Noncopyable {
    public:
        Tasks(int maxSize = QUEUE_MAX_SIZE_DEFAULT, int warSize =
        QUEUE_WAR_SIZE_DEFAULT);

        virtual ~Tasks() = default;

        bool isEmpty();

        // 增加任务
        bool addTask(const Task &task);

        template<typename F, typename... Args>
        bool addTaskT(F &&f, Args &&... args) {
            return addTask(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        }

        bool removeTask(Task &task);

        void clearTask();

    private:
        std::queue<Task> _tasks;
        int _maxSize;
        int _warSize;
        int _size;
        std::mutex _mutex;
    };

} /* namespace Dream */

#endif /* TASKQUEUE_H_ */
