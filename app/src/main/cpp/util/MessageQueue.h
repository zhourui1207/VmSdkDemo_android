//
// Created by zhou rui on 2017/9/8.
//

#ifndef DREAM_MESSAGEQUEUE_H
#define DREAM_MESSAGEQUEUE_H

#include <memory>
#include <stdint.h>
#include <vector>
#include <condition_variable>

#include "Message.h"
#include "Handler.h"
#include "Runnable.h"

namespace Dream {

    class Looper;

    // 回调接口，用户发现某个线程是否空闲(正在阻塞等待更多的消息)
    class IdleHandler {
    public:
        IdleHandler() = default;
        virtual ~IdleHandler() = default;
        
        virtual bool queueIdle() = 0;
    };
    
    using IdleHandlerPtr = std::shared_ptr<IdleHandler>;

    class MessageQueue {
    
    public:
        virtual ~MessageQueue();
        
        bool isIdle();
        
        void addIdleHandler(IdleHandlerPtr handlerPtr);
        
        void removeIdleHandler(IdleHandlerPtr handlerPtr);
        
        bool isPolling();
        
        int postSyncBarrier();
        
        void removeSyncBarrier(int token);
    
    private:
        friend class Message;
        friend class Looper;
        friend class Handler;
        friend class std::shared_ptr<MessageQueue>;
        
        MessageQueue() = delete;
        MessageQueue(bool quitAllowed);
        
        void dispose();  // 处理消息队列
        
        bool isPollingLocked();
        
        MessagePtr next();
        
        void quit(bool safe);
        
        int postSyncBarrier(int64_t when);
        
        bool enqueueMessage(MessagePtr msgPtr, int64_t when);
        
        bool hasMessages(HandlerPtr hPtr, int what, ObjectPtr objectPtr);
        
        bool hasMessages(HandlerPtr hPtr, RunnablePtr rPtr, ObjectPtr objectPtr);
        
        void removeMessages(HandlerPtr hPtr, int what, ObjectPtr objectPtr);
        
        void removeMessages(HandlerPtr hPtr, RunnablePtr rPtr, ObjectPtr objectPtr);
        
        void removeCallbacksAndMessages(HandlerPtr hPtr, ObjectPtr objectPtr);
        
        void removeAllMessagesLocked();
        
        void removeAllFutureMessagesLocked();
        
        // 阻塞一定时间
        void nativePollOnce(int32_t timeoutMillis);
        
        // 唤醒
        void nativeWake();
        
        // 正在阻塞
        bool nativeIsPolling();
        
        std::mutex _nativeMutex;  // 用来控制阻塞的锁
        std::condition_variable _condition;
    
        std::mutex _thisMutex;
        const bool _quitAllowed;
        
        MessagePtr _messagesPtr;
        std::vector<IdleHandlerPtr> _idleHandlers;
        IdleHandlerPtr* _pendingIdleHandlers;
        std::size_t _pendingIdleHandlersSize;
        bool _quitting;
        bool _blocked;
        int _nextBarrierToken;
        bool _polling;
    
        const char* TAG = "MessageQueue";
        static const bool sDEBUG = false;
        
    };
}

#endif //VMSDKDEMO_ANDROID_MESSAGEQUEUE_H
