/*
 * ThreadPool.cpp
 *
 *  Created on: 2016年9月11日
 *      Author: zhourui
 */

#include <stdio.h>

#include "ThreadPool.h"
#include "public/platform.h"

namespace Dream {

    ThreadPool::ThreadPool(std::size_t size, std::size_t maxSize, int idleTime) :
            _size{size < 1 ? 1 : size}, _maxSize(maxSize < _size ? _size : maxSize), _idleTime(
            idleTime < -1 ? -1 : idleTime), _running(false) {
        printf("_size=%zd, _maxSize=%zd, _idleTime=%d\n", _size, _maxSize, _idleTime);
        for (std::size_t i = 0; i < _size; ++i) {
            TaskThreadPtr threadPtr = std::make_shared<TaskThread>(_tasks);
            _pool.emplace_back(threadPtr);
            threadPtr->start();
        }
    }

    ThreadPool::~ThreadPool() {

    }

    bool ThreadPool::addTask(const Task &task) {
//        LOGW("ThreadPool", "addTask");
        std::unique_lock<std::mutex> lock{_threadMutex};
        if (_running.load()) {
            // 添加到任务队列
            if (_tasks.addTask(task)) {

                {
                    ReadLockGuard lock(_mutex);
                    // 给线程分配任务
                    for (std::size_t i = 0; i < _pool.size(); ++i) {
                        if (_pool[i]->isWaiting()) {  // 如果正在等待，那就唤醒线程工作
                            _pool[i]->weakUp();
                            return true;
                        }
                    }
                }

                {
                    WriteLockGuard lock(_mutex);
                    // 若没有可用线程，看是否要增加线程
                    if (_pool.size() < _maxSize) {
                        TaskThreadPtr threadPtr = std::make_shared<TaskThread>(_tasks,
                                                                               _idleTime,
                                                                               std::bind(
                                                                                       &ThreadPool::handleTimeout,
                                                                                       this,
                                                                                       std::placeholders::_1));
                        _pool.emplace_back(threadPtr);
                        threadPtr->start();
                    } else if (_maxSize > 1) {  // 如果不是单线程，就打印提醒信息，因为单线程一般都是故意排队的，所以不用提醒
                        // 如果线程已经到达上限，那么就什么都不做，放入队列中即可
                        printf("-----------线程数量已经到达上限，任务延迟处理----------\n");
                    }
                }

                return true;

            }
        }
        return false;
    }

    void ThreadPool::start() {
        std::unique_lock<std::mutex> lock{_threadMutex};
        if (!_running.load()) {
            _running.store(true);
        }
    }

    void ThreadPool::stop() {
        std::unique_lock<std::mutex> lock{_threadMutex};
        if (_running.load()) {
            _running.store(false);
        }
        _tasks.clearTask();
//        LOGW("ThreadPool", "stopTask start");
        stopTask();
//        LOGW("ThreadPool", "stopTask success");
    }

    void ThreadPool::handleTimeout(const TaskThreadPtr &threadPtr) {
        WriteLockGuard lock(_mutex);
        // printf("接收到线程的超时消息\n");
        for (auto it = _pool.begin() + _size; it != _pool.end(); ++it) {
            if ((*it) == threadPtr) {
                (*it)->stop();
                _pool.erase(it);
                return;
            }
        }
        printf("!!!!!!!!!!!!!!!找不到应该删除的线程!!!!!!!!!!!!!!\n");
    }

    void ThreadPool::stopTask() {
        for (std::size_t i = 0; i < _pool.size(); ++i) {
            _pool[i]->stop();
        }
//        LOGW("ThreadPool", "clear start");
        _pool.clear();
//        LOGW("ThreadPool", "clear end");
    }

} /* namespace Dream */
