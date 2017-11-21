/*
 * TaskThread.cpp
 *
 *  Created on: 2016年9月12日
 *      Author: zhourui
 */

#include <chrono>

#include "public/platform.h"
#include "TaskThread.h"

#ifdef _ANDROID

extern JavaVM *g_pJavaVM;  // 定义外部变量，该变量在VmNet-lib.cpp中被定义和赋值
#endif

namespace Dream {

    TaskThread::TaskThread(Tasks &tasks, int idleTime,
                           std::function<void(const std::shared_ptr<TaskThread> &)> callback) :
            _running(false), _waiting(false), _exited(false), _tasks(tasks), _idleTime(idleTime), _timeoutCallback(
            callback) {
    }

    TaskThread::~TaskThread() {

    }

    void TaskThread::start() {
        std::unique_lock<std::mutex> lock{_mutex};
        if (!_running) {
            _running = true;
            _threadPtr.reset(new std::thread(&TaskThread::run, this));
        }
    }

    void TaskThread::restart() {
        stop();
        start();
    }

    void TaskThread::stop() {
        std::unique_lock<std::mutex> lock{_mutex};
        if (_running) {
            _running = false;
            _condition.notify_all();
//            LOGW("TaskThread", "thread stop wait\n");
            _condition.wait(lock, [&]()-> bool { return _exited;});
//            if (_waiting.load()) {  // 如果正在等待
//                _sendThreadPtr->join();  // 等待线程执行完毕
//            } else {  // 如果正在执行，就不等待了，可能会死锁
//                _sendThreadPtr->detach();
//            }
        }
        if (_threadPtr.get() != nullptr) {
            _threadPtr->join();
            _threadPtr.reset();
        }

//        LOGW("TaskThread", "thread stop success\n");
    }

    bool TaskThread::isWaiting() {
        std::unique_lock<std::mutex> lock{_mutex};
        return _waiting;
    }

    void TaskThread::weakUp() {
        // 这里必须加上锁，不然会出现唤醒操作在线程等待前执行，那么那样线程会一直等待着，而线程池一直以为线程已经醒了，就无法给线程发放任务了
        std::unique_lock<std::mutex> lock{_mutex};
        _waiting = false;
        _condition.notify_all();
    }

    void TaskThread::run() {
#ifdef _ANDROID
        // 绑定android线程
        JNIEnv *pJniEnv = nullptr;
        if (g_pJavaVM) {
            if (g_pJavaVM->AttachCurrentThread(&pJniEnv, nullptr) == JNI_OK) {
                LOGW("TaskThread", "Attach android thread success! pJniEnv[%zd]\n", pJniEnv);
            } else {
                LOGE("TaskThread", "Attach android thread failed！\n");
                return;
            }
        } else {
            LOGE("TaskThread", "pJavaVm is NULL！\n");
        }
#endif
        while (_running) {
//            LOGW("TaskThread", "lock start\n");
            std::unique_lock<std::mutex> lock{_mutex};
//            LOGW("TaskThread", "lock end\n");
            Task task;  // 这个必须要写在里面，因为如果写在外面的话，task()执行完后，task所占用的内存不会被释放
            // 如果是没有任务，则进入等待
            while (_running && !_tasks.removeTask(task)) {
                _waiting = true;
                if (_idleTime < 0) {  // 空闲时不会销毁
//                    LOGE("TaskThread", "wait start\n");
                    _condition.wait(lock/*, [&]()->bool { return !_tasks.isEmpty(); }*/);
//                    LOGE("TaskThread", "wait end\n");
                } else {
                    if (_condition.wait_for(lock, std::chrono::seconds(_idleTime))
                        == std::cv_status::timeout) {
                        _waiting = false;
                        _timeoutCallback(shared_from_this());
#ifdef _ANDROID
                        dtachAndroidThread(pJniEnv);
#endif
                        _running = false;
                        if (_threadPtr.get() != nullptr) {
                            _threadPtr->detach();
                            _threadPtr.reset();
                        }
//                        LOGW("TaskThread", "thread stop\n");
                        return;
                    }
                }

                // 如果线程停止，则该线程退出
                if (!_running) {
#ifdef _ANDROID
                    dtachAndroidThread(pJniEnv);
#endif
                    _exited = true;
                    _condition.notify_all();
//                    LOGW("TaskThread", "thread stop, skip task\n");
                    return;
                }
            }
            // 出来只有一种可能，1.获取到task

            // -------------执行任务-------------------
            if (_running && task) {
//                LOGW("TaskThread", "task start\n");
                lock.unlock();
                task();
//                LOGW("TaskThread", "task end\n");
            }
            // -------------执行结束-------------------
        }
#ifdef _ANDROID
        dtachAndroidThread(pJniEnv);
#endif

        std::unique_lock<std::mutex> lock{_mutex};
        _exited = true;
        _condition.notify_all();
        LOGW("TaskThread", "thread exited\n");
    }

    void TaskThread::dtachAndroidThread(void *point) {
#ifdef _ANDROID
        JNIEnv *pJniEnv = reinterpret_cast<JNIEnv *>(point);
        // 解绑android线程
        if (g_pJavaVM && pJniEnv) {
            LOGW("TaskThread", "Detach android thread [%zd]！\n", pJniEnv);
            g_pJavaVM->DetachCurrentThread();
        }
#endif
    }

} /* namespace Dream */
