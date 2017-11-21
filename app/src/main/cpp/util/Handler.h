//
// Created by zhou rui on 2017/9/8.
//

#ifndef DREAM_HANDLER_H
#define DREAM_HANDLER_H

#include <stdint.h>
#include <memory>
#include <condition_variable>

#include "Message.h"
#include "MessageQueue.h"
#include "Object.h"
#include "Runnable.h"

namespace Dream {

    class Looper;
    using LooperPtr = std::shared_ptr<Looper>;
    
    class Handler;
    using HandlerPtr = std::shared_ptr<Handler>;
    
    using MessageQueuePtr = std::shared_ptr<MessageQueue>;
    
    class BlockingRunnable : public Runnable {
    public:
        BlockingRunnable() = delete;
    
        BlockingRunnable(RunnablePtr taskPtr);
        
        virtual ~BlockingRunnable() = default;
        
        void run() override;
        
        bool postAndWait(HandlerPtr handlerPtr, int64_t timeout);
    
    private:
        std::mutex _mutex;
        std::condition_variable _condition;
        RunnablePtr _taskPtr;
        bool _done;
    };
    
    class Callback {
    public:
        Callback() = default;
        virtual ~Callback() = default;
        
        virtual bool handleMessage(MessagePtr msgPtr) = 0;
    };
    
    using CallbackPtr = std::shared_ptr<Callback>;

    class Handler : public std::enable_shared_from_this<Handler> {
    public:
        Handler();
        
        Handler(CallbackPtr callbackPtr);
        
        Handler(LooperPtr looperPtr);
        
        Handler(LooperPtr looperPtr, CallbackPtr callbackPtr);
        
        Handler(bool async);
        
        Handler(CallbackPtr callbackPtr, bool async);
        
        Handler(LooperPtr looperPtr, CallbackPtr callbackPtr, bool async);
        
        inline void init(CallbackPtr callbackPtr, bool async);
        
        inline void init(LooperPtr looperPtr, CallbackPtr callbackPtr, bool async);
        
        virtual ~Handler();
    
        // 子类必须实现这个函数用来接收消息
        virtual void handleMessage(MessagePtr msgPtr) = 0;
        
        void dispatchMessage(MessagePtr msgPtr);
        
        MessagePtr obtainMessage();
        
        MessagePtr obtainMessage(int what);
        
        MessagePtr obtainMessage(int what, ObjectPtr objPtr);
        
        MessagePtr obtainMessage(int what, int arg1, int arg2);
        
        MessagePtr obtainMessage(int what, int arg1, int arg2, ObjectPtr objPtr);
        
        bool post(RunnablePtr rPtr);
        
        bool postAtTime(RunnablePtr rPtr, int64_t uptimeMillis);
        
        bool postAtTime(RunnablePtr rPtr, ObjectPtr tokenPtr, int64_t uptimeMillis);
        
        bool postDelayed(RunnablePtr rPtr, int64_t delayMillis);
        
        bool postAtFrontOfQueue(RunnablePtr rPtr);
        
        bool runWithScissors(RunnablePtr rPtr, int64_t timeout);
        
        void removeCallbacks(RunnablePtr rPtr);
        
        void removeCallbacks(RunnablePtr rPtr, ObjectPtr tokenPtr);
        
        bool sendMessage(MessagePtr msgPtr);
        
        bool sendEmptyMessage(int what);
        
        bool sendEmptyMessageDelayed(int what, int64_t delayMillis);
        
        bool sendEmptyMessageAtTime(int what, int64_t uptimeMillis);
        
        bool sendMessageDelayed(MessagePtr msgPtr, int64_t delayMillis);
        
        bool sendMessageAtTime(MessagePtr msgPtr, int64_t uptimeMillis);
        
        bool sendMessageAtFrontOfQueue(MessagePtr msgPtr);
        
        bool enqueueMessage(MessageQueuePtr queuePtr, MessagePtr msgPtr, int64_t uptimeMillis);
        
        void removeMessages(int what);
        
        void removeMessage(int what, ObjectPtr objectPtr);
        
        void removeCallbacksAndMessages(ObjectPtr tokenPtr);
        
        bool hasMessages(int what);
        
        bool hasMessages(int what, ObjectPtr objectPtr);
        
        bool hasCallbacks(RunnablePtr rPtr);
        
        LooperPtr getLooper();
        
        static MessagePtr getPostMessage(RunnablePtr rPtr);
        
        static MessagePtr getPostMessage(RunnablePtr rPtr, ObjectPtr tokenPtr);
        
        static void handleCallback(MessagePtr messagePtr);
        
    private:
        LooperPtr _looperPtr;
        MessageQueuePtr _queuePtr;
        CallbackPtr _callbackPtr;
        bool _asynchronous;
    };
}

#endif //VMSDKDEMO_ANDROID_HANDLER_H
