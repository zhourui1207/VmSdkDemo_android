/*
 * RWLock.h
 *
 *  Created on: 2016年9月14日
 *      Author: zhourui
 *      读写锁，写优先，同时只允许一个线程写，允许多个线程同时读，在写的时候阻塞读
 */

#ifndef RWLOCK_H_
#define RWLOCK_H_

#include <condition_variable>
#include <mutex>

#include "Noncopyable.h"

namespace Dream {

    class RWLock : public Noncopyable {
    public:
        RWLock()
                : _reader(0), _writer(0), _reading(0), _writing(false) {

        }

        virtual ~RWLock() = default;

        void writeLock() {
            std::unique_lock<std::mutex> lock(_mutex);
            // 进入判断
            bool waiting = false;
            while (_writing || _reading > 0) {  // 如果有人在写或者正在读的数量>0，那么就先等待
                if (!waiting) {  // 从不等待进入等待状态
                    ++_writer;  // 等待写的数量+1
                    waiting = true;
                }
                _writeCondition.wait(lock);
            }
            // 可以进行写了，如果之前等待过，那么现在脱离等待
            if (waiting) {
				// 等待写的数量-1
				--_writer;
            }
            // 开始写
            _writing = true;
        }

        void writeUnlock() {
            std::unique_lock<std::mutex> lock(_mutex);
            // 写结束
            _writing = false;
            // 进入判断
            if (_writer > 0) {  // 如果正在等待写的数量>0，那么唤醒一个写
                _writeCondition.notify_one();
            } else if (_reader > 0) {  // 如果正在等待读的数量>0且没有等待写的人，那么唤起所有的读者
                _readCondition.notify_all();
            }
        }

        void readLock() {
            std::unique_lock<std::mutex> lock(_mutex);
            // 进入判断
            bool waiting = false;
            while (_writer > 0 || _writing) {  // 如果有人正在等待写，或者有人正在写
                if (!waiting) {  // 从不等待进入等待状态
                    ++_reader;  // 等待读的数量+1
                    waiting = true;
                }
                _readCondition.wait(lock);
            }

            if (waiting) {
                --_reader;  // 等待读的数量-1
            }

            ++_reading;
        }

        void readUnlock() {
            std::unique_lock<std::mutex> lock(_mutex);
            --_reading;
            if (_writer > 0 && _reading <= 0) {  // 如果有等待写，并且当前没有正在读的人，就唤醒一个写
                _writeCondition.notify_one();
            }
        }

    private:
        int _reader;  // 读者等待数量
        int _writer;  // 写者等待数量
        int _reading;  // 正在读的数量
        bool _writing;  // 是否正在写
        std::condition_variable _readCondition;
        std::condition_variable _writeCondition;
        std::mutex _mutex;
    };

} /* namespace Dream */

#endif /* RWLOCK_H_ */
