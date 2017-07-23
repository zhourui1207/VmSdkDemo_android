/*
 * Timer.cpp
 *
 *  Created on: 2016年9月15日
 *      Author: zhourui
 */

#include <chrono>

#include "Timer.h"

namespace Dream {

    Timer::Timer(const std::function<void()> &task, uint64_t delay, bool isLoop, uint64_t interval,
                 DURATION duration)
            : _duration(duration), _task(task), _delay(delay), _isLoop(isLoop),
              _interval(interval < 1 ? 1 : interval), _running(false) {

    }

    Timer::~Timer() {
        cancel();
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

            // 在join前不能忘记解锁
            lock.unlock();
        }
        if (_threadPtr->joinable()) {
            _threadPtr->join();
        }
    }

    void Timer::run() {
        std::unique_lock<std::mutex> lock(_mutex);
        bool first = true;
        while (_running.load() && (first || _isLoop)) {

            uint64_t waiteTime = _interval;

            if (first) {  // 第一次执行
                waiteTime = _delay;
                first = false;
            }

            switch (_duration) {
                case SEC:
                    _condition.wait_for(lock, std::chrono::seconds(waiteTime));
                    break;
                case MILLI:
                    _condition.wait_for(lock, std::chrono::milliseconds(waiteTime));
                    break;
                case MICRO:
                    _condition.wait_for(lock, std::chrono::microseconds(waiteTime));
                    break;
                case NANO:
                    _condition.wait_for(lock, std::chrono::nanoseconds(waiteTime));
                    break;
                default:
                    _condition.wait_for(lock, std::chrono::milliseconds(waiteTime));
                    break;
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
