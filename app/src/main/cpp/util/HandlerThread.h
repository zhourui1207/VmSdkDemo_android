//
// Created by zhou rui on 2017/10/13.
//

#ifndef DREAM_HANDLERTHREAD_H
#define DREAM_HANDLERTHREAD_H

#include <atomic>
#include <memory.h>
#include <condition_variable>
#include "Noncopyable.h"
#include "Looper.h"

namespace Dream {

    class HandlerThread : private Noncopyable {
    
    public:
        HandlerThread();
        virtual ~HandlerThread();
        
        void start();
        
        LooperPtr getLooper();
        
        bool quit();
        
        bool quitSafely();
        
        std::thread::id getThreadId();
        
    protected:
        virtual void onLooperPrepared();
        
    private:
        void run();

        const char* TAG = "HandlerThread";

        std::atomic<bool> _active;
        std::mutex _thisMutex;
        std::condition_variable _condition;
        std::unique_ptr<std::thread> _threadPtr;
        LooperPtr _looperPtr;
    };
    
}

#endif //VMSDKDEMO_ANDROID_HANDLERTHREAD_H
