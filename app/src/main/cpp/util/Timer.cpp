/*
 * Timer.cpp
 *
 *  Created on: 2016年9月15日
 *      Author: zhourui
 */

#include <chrono>

#include "Timer.h"

namespace Dream {

    Timer::Timer(const std::function<void()> &task, uint64_t delay, bool isLoop, uint64_t interval)
            : _task(task), _delay(delay), _isLoop(isLoop), _interval(interval < 1 ? 1 : interval),
              _running(false) {

    }

    Timer::~Timer() {
        cancel();  // 不加这个会出错
    }

    void Timer::start() {
        if (!_running.load()) {
            _running.store(true);
            _threadPtr.reset(new std::thread(&Timer::run, this));
        }
    }

    void Timer::cancel() {
        if (_running.load()) {  // 如果还在执行，那么就等待它停止

            std::unique_lock<std::mutex> lock(_mutex);
            _running.store(false);
            _condition.notify_one();

            lock.unlock();  // 在join前不能忘记解锁
        }
        if (_threadPtr->joinable()) {
            _threadPtr->join();
        }
    }

    void Timer::run() {
        std::unique_lock<std::mutex> lock(_mutex);
        bool first = true;
        while (_running.load() && (first || _isLoop)) {
            if (first) {  // 第一次执行
                first = false;
                _condition.wait_for(lock, std::chrono::milliseconds(_delay));
            } else {  // 循环执行
                _condition.wait_for(lock, std::chrono::milliseconds(_interval));
            }

            if (!_running.load()) {  // 如果定时器取消了，那么就退出
                return;
            }
            if (_task) {
                _task();  // 执行任务
            }
        }
        _running.store(false);
    }

} /* namespace Dream */
