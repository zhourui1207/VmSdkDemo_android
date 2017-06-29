/*
 * Timer.h
 *
 *  Created on: 2016年9月15日
 *      Author: zhourui
 *      定时器类
 */

#ifndef DREAM_TIMER_H_
#define DREAM_TIMER_H_

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

#include "Noncopyable.h"

namespace Dream {

    class Timer : Noncopyable {
    public:
        static const uint64_t DELAY_DEFAULT = 0;
        static const uint64_t INTERVAL_DEFAULT = 60000;
    public:
        Timer() = delete;

        Timer(const std::function<void()> &task, uint64_t delay = DELAY_DEFAULT,
              bool isLoop = false, uint64_t interval = INTERVAL_DEFAULT);

        virtual ~Timer();

        void start();  // 开始启动定时器
        void cancel();  // 取消定时器

    private:
        void run();

    private:
        std::function<void()> _task;  // 需要执行的任务
        uint64_t _delay;  // 执行延时  单位毫秒
        bool _isLoop;  // 是否循环
        uint64_t _interval;  // 执行间隔  单位毫秒
        std::atomic<bool> _running;
        std::unique_ptr<std::thread> _threadPtr;
        std::condition_variable _condition;
        std::mutex _mutex;
    };

} /* namespace Dream */

#endif /* TIMER_H_ */
