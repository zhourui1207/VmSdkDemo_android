//
// Created by zhou rui on 2017/9/8.
//

#ifndef DREAM_MESSAGE_H
#define DREAM_MESSAGE_H

#include <stdint.h>
#include <memory>
#include <mutex>

#include "Object.h"
#include "Runnable.h"

//仿android
namespace Dream {

    class MessageQueue;
    //class Messenger;
    class Message;
    class Handler;
    class Looper;
    
    using MessagePtr = std::shared_ptr<Message>;
    //using MessengerPtr = std::shared_ptr<Messenger>;
    using HandlerPtr = std::shared_ptr<Handler>;

    class Message : public std::enable_shared_from_this<Message> {

    public:
        Message();
        
        virtual ~Message();
    
        static MessagePtr obtain(); // 从池子中获取消息类
        
        static MessagePtr obtain(MessagePtr origPtr);
        
        static MessagePtr obtain(HandlerPtr hPtr);
        
        static MessagePtr obtain(HandlerPtr hPtr, RunnablePtr callbackPtr);
        
        static MessagePtr obtain(HandlerPtr hPtr, int what);
        
        static MessagePtr obtain(HandlerPtr hPtr, int what, ObjectPtr objPtr);
        
        static MessagePtr obtain(HandlerPtr hPtr, int what, int arg1, int arg2);
        
        static MessagePtr obtain(HandlerPtr hPtr, int what, int arg1, int arg2, ObjectPtr objPtr);
        
        static void updateCheckRecycle(int targetSdkVersion);

        void recycle();  // 回收一个消息实体到全局池
        
        void recycleUnchecked();
        
        void copyFrom(MessagePtr oPtr);
        
        int64_t getWhen() const { return _when; }
        
        void setTarget(HandlerPtr targetPtr) { _targetPtr = targetPtr; }
        
        HandlerPtr getTarget() const { return _targetPtr; }
        
        RunnablePtr getCallback() const { return _callbackPtr; }
        
        void sendToTarget();
        
        bool isAsynchronous() const;
        
        void setAsynchronous(bool async);
        
        std::string toString();
    
    public:
        int32_t _what;  // 用户定义消息码用来描述这是什么消息，每个Handler拥有自己的消息码命名空间，所以你不需要担心和其他Handlers冲突
        int32_t _arg1;
        int32_t _arg2;
        ObjectPtr _objPtr;  // 对象
        //MessengerPtr _replayToPtr;
        int32_t _sendingUid;
        
    private:
        friend class MessageQueue;
        friend class Looper;
        friend class Handler;
    
        bool isInUse() const;
        
        void markInUse();
        
        std::string toString(uint64_t now);
    
        const char* TAG = "Dream::Message";
        static const int FLAG_IN_USE = 1 << 0;
        static const int FLAG_ASYNCHRONOUS = 1 << 1;
        static const int FLAGS_TO_CLEAR_ON_COPY_FROM = FLAG_IN_USE;
        static const int MAX_POOL_SIZE = 50;  // 对象上限，程序最多可以复用50个对象
        
        int _flags;
        int64_t _when;
        HandlerPtr _targetPtr;
        RunnablePtr _callbackPtr;
        MessagePtr _nextPtr;  // 指向下一个Message位置
        
        static std::mutex sPoolSync;  // 对象池锁
        static MessagePtr sPoolPtr;  // 对象池首个
        static int sPoolSize;  // 当前对象数量
        static bool gCheckRecycle;
    };
    
}

#endif //VMSDKDEMO_ANDROID_MESSAGE_H
