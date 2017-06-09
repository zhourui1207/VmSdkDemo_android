/*
 * TaskQueue.cpp
 *
 *  Created on: 2016年9月12日
 *      Author: zhourui
 */
#include <stdio.h>

#include "Tasks.h"

namespace Dream {

    Tasks::Tasks(int maxSize, int warSize)
            : _maxSize(maxSize), _warSize{(warSize > maxSize) ? maxSize : warSize}, _size(0) {
        printf("_maxSize=%d, _warSize=%d\n", _maxSize, _warSize);
    }

    bool Tasks::isEmpty() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _tasks.empty();
    }

    bool Tasks::addTask(const Task &task) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_size >= _maxSize) {
            printf("task missed\n");
            return false;
        }
        ++_size;
        if (_size >= _warSize) {
            printf("task warring size=%d!!\n", _size);
        }
        _tasks.emplace(task);
        return true;
//  _tasks.push(task);
    }

    bool Tasks::removeTask(Task &task) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_size <= 0) {
            return false;
        }
        --_size;
        task = _tasks.front();
        _tasks.pop();
        return true;
    }

    void Tasks::clearTask() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_size > 0) {
            _tasks.pop();
            --_size;
        }
    }

} /* namespace Dream */
