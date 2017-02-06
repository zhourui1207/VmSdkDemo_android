/*
 * BaseTasks.h
 *
 *  Created on: 2016年9月12日
 *      Author: zhourui
 *      队列基础类，不可实例化
 */

#ifndef BASETASKS_H_
#define BASETASKS_H_

#include <memory>
#include <mutex>
#include <queue>

#include "public/define.h"
#include "Task.h"

namespace Dream {

    using TaskPtr = std::shared_ptr<Task>;

    class BaseTasks {
    public:
        BaseTasks(size_t maxSize, size_t warSize)
                : _maxSize(maxSize), _warSize(warSize) {
            if (_warSize > _maxSize) {
                _warSize = _maxSize;
            }
        }

        virtual ~BaseTasks() {

        }

        virtual bool isEmpty() const = 0;

        virtual void addTask(const TaskPtr &taskPtr) = 0;

        virtual TaskPtr removeTask() = 0;

    protected:
        std::mutex _mutex;
        size_t _maxSize;  // 队列最大长度
        size_t _warSize;  // 警告阀值
    };

} /* namespace Dream */

#endif /* BASETASKS_H_ */
