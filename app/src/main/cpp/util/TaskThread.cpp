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
            _running(false), _waiting(false), _tasks(tasks), _idleTime(idleTime), _timeoutCallback(
            callback) {
    }

    TaskThread::~TaskThread() {
        stop();
    }

    void TaskThread::start() {
        if (!_running.load()) {
            _running.store(true);
            _threadPtr.reset(new std::thread(&TaskThread::run, this));
            // _threadPtr->detach();
        }
    }

    void TaskThread::restart() {
        stop();
        start();
    }

    void TaskThread::stop() {
        if (_running.load()) {
            _running.store(false);
            if (_waiting.load()) {  // 如果正在等待
                _condition.notify_one();
                _threadPtr->join();  // 等待线程执行完毕
            } else {  // 如果正在执行，就不等待了，可能会死锁
                _threadPtr->detach();
            }
        }
    }

    void TaskThread::run() {
        std::unique_lock<std::mutex> lock{_mutex};
#ifdef _ANDROID
        // 绑定android线程
        JNIEnv *pJniEnv = nullptr;
        if (g_pJavaVM) {
            if (g_pJavaVM->AttachCurrentThread(&pJniEnv, nullptr) == JNI_OK) {
                __android_log_print(ANDROID_LOG_WARN, "TaskThread", "绑定android线程成功pJniEnv[%zd]！",
                                    pJniEnv);
            } else {
                __android_log_print(ANDROID_LOG_ERROR, "TaskThread", "绑定android线程失败！");
                return;
            }
        } else {
            __android_log_print(ANDROID_LOG_ERROR, "TaskThread", "pJavaVm 为空！");
        }
#endif
        while (_running.load()) {
            Task task;  // 这个必须要写在里面，因为如果写在外面的话，task()执行完后，task所占用的内存不会被释放
            // 如果是没有任务，则进入等待
            while (!_tasks.removeTask(task)) {
                _waiting.store(true);
                if (_idleTime < 0) {  // 空闲时不会销毁
                    _condition.wait(lock/*, [&]()->bool { return !_tasks.isEmpty(); }*/);
                } else {
                    if (_condition.wait_for(lock, std::chrono::seconds(_idleTime))
                        == std::cv_status::timeout) {
                        _waiting.store(false);
                        _timeoutCallback(shared_from_this());
                        dtachAndroidThread(pJniEnv);
                        LOGW("TaskThread", "thread stop\n");
                        return;
                    }
                }

                // 如果线程停止，则该线程退出
                if (!_running.load()) {
                    dtachAndroidThread(pJniEnv);
                    LOGW("TaskThread", "thread stop\n");
                    return;
                }
            }
            // 出来只有一种可能，1.获取到task

            // -------------执行任务-------------------
            if (task) {
                task();
            }
            // -------------执行结束-------------------
        }

        dtachAndroidThread(pJniEnv);
        LOGW("TaskThread", "thread stop\n");
    }

    void TaskThread::dtachAndroidThread(void *point) {
#ifdef _ANDROID
        JNIEnv *pJniEnv = reinterpret_cast<JNIEnv *>(point);
        // 解绑android线程
        if (g_pJavaVM && pJniEnv) {
            LOGW("TaskThread", "解绑android线程[%zd]！\n", pJniEnv);
            g_pJavaVM->DetachCurrentThread();
        }
#endif
    }

} /* namespace Dream */
