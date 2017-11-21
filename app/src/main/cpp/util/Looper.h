//
// Created by zhou rui on 2017/9/8.
//

#ifndef DREAM_LOOPER_H
#define DREAM_LOOPER_H


#include <map>
#include <thread>
#include "MessageQueue.h"


namespace Dream {

    class Looper;
    class Handler;

    using LooperPtr = std::shared_ptr<Looper>;

    class Looper {
    
        using ThreadPtr = std::shared_ptr<std::thread>;
        
    public:
        static void prepare();
        
        static void prepare(bool quitAllowed);
        
        static void loop();
        
        static LooperPtr myLooper();
        
        static MessageQueuePtr myQueue();
        
        virtual ~Looper();
        
        bool isCurrentThread();
        
        void quit();
        
        void quitSafely();
        
        std::thread::id getThread() {
            return _threadId;
        }
        
        MessageQueuePtr getQueue() {
            return _queuePtr;
        }
        
        
    private:
        friend class Handler;
        friend class std::shared_ptr<Looper>;
    
        Looper() = default;
        
        Looper(bool quitAllowed);
        
        const char* TAG = "Dream::Looper";
        
        MessageQueuePtr _queuePtr;
        
        std::thread::id _threadId;
        
        static std::map<std::thread::id, LooperPtr> sThreadLocal;
        static std::mutex sThreadLocalMutex;
    };
}

#endif //VMSDKDEMO_ANDROID_LOOPER_H
