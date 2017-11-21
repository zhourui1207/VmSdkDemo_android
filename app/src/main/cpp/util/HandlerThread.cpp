//
// Created by zhou rui on 2017/10/13.
//

#include "public/platform.h"
#include "HandlerThread.h"

#ifdef _ANDROID
extern JavaVM *g_pJavaVM;  // 定义外部变量，该变量在VmNet-lib.cpp中被定义和赋值
#endif

namespace Dream {

    HandlerThread::HandlerThread()
    : _active(true) {
        
    }

    HandlerThread::~HandlerThread() {

    }

    void HandlerThread::start() {
        std::lock_guard<std::mutex> lock(_thisMutex);
        if (_threadPtr.get()) {
            LOGE(TAG, "Start() only called once time!\n");
        } else {
            _threadPtr.reset(new std::thread(&HandlerThread::run, this));
            _threadPtr->detach();
        }
    }

    LooperPtr HandlerThread::getLooper() {
        LooperPtr loopPtr;
        if (!_active.load()) {
            return loopPtr;
        }
        
        {
            std::unique_lock<std::mutex> lock(_thisMutex);
            while (_active.load() && !_looperPtr.get()) {
//                LOGE(TAG, "waiting...\n");
                _condition.wait(lock);
            }
//            LOGE(TAG, "wait ok...\n");
        }
        return _looperPtr;
    }

    bool HandlerThread::quit() {
        LooperPtr loopPtr = getLooper();
        if (loopPtr.get()) {
            loopPtr->quit();
            return true;
        }
        return false;
    }

    bool HandlerThread::quitSafely() {
        LooperPtr loopPtr = getLooper();
        if (loopPtr.get()) {
            loopPtr->quitSafely();
            return true;
        }
        return false;
    }

    std::thread::id HandlerThread::getThreadId() {
        return _threadPtr.get() ? _threadPtr->get_id() : std::thread::id();
    }

    void HandlerThread::onLooperPrepared() {

    }

    void HandlerThread::run() {
#ifdef _ANDROID
        // 绑定android线程
        JNIEnv *pJniEnv = nullptr;
        if (g_pJavaVM) {
            if (g_pJavaVM->AttachCurrentThread(&pJniEnv, nullptr) == JNI_OK) {
                LOGW(TAG, "Attach android thread success! pJniEnv[%zd]\n", pJniEnv);
            } else {
                LOGE(TAG, "Attach android thread failed！\n");
                return;
            }
        } else {
            LOGE("TaskThread", "pJavaVm is NULL！\n");
        }
#endif

        Looper::prepare();
        
        {
            std::lock_guard<std::mutex> lock(_thisMutex);
            _looperPtr = Looper::myLooper();
            _condition.notify_all();                     
        }
        
        onLooperPrepared();
        Looper::loop();
        _active.store(false);

#ifdef _ANDROID
        // 解绑android线程
        if (g_pJavaVM && pJniEnv) {
            LOGW(TAG, "Detach android thread [%zd]！\n", pJniEnv);
            g_pJavaVM->DetachCurrentThread();
        }
#endif
    }
}
