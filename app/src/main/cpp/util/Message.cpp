//
// Created by zhou rui on 2017/9/8.
//

#include "Message.h"
#include "MessageQueue.h"
#include "Looper.h"
#include "Handler.h"
#include "public/platform.h"

namespace Dream {

    std::mutex Message::sPoolSync;  // 对象池锁
    MessagePtr Message::sPoolPtr;  // 对象池首个
    int Message::sPoolSize = 0;
    bool Message::gCheckRecycle = true;

    Message::Message()
    :_what(0), _arg1(0), _arg2(0), _sendingUid(-1), _flags(0), _when(0) {
        
    }
    
    Message::~Message() {
       
    }

    MessagePtr Message::obtain() {
        {
            std::unique_lock<std::mutex> lock(sPoolSync);
            if (sPoolPtr.get()) {
                MessagePtr mPtr = sPoolPtr;
                sPoolPtr = mPtr->_nextPtr;
                mPtr->_nextPtr.reset();
                mPtr->_flags = 0;
                sPoolSize--;
                return mPtr;
            }
//            LOGD(TAG, "obtain sPoolSize=%d\n", sPoolSize);
        }
        return std::make_shared<Message>();
    }
    
    MessagePtr Message::obtain(MessagePtr origPtr) {
        MessagePtr mPtr = obtain();
        mPtr->_what = origPtr->_what;
        mPtr->_arg1 = origPtr->_arg1;
        mPtr->_arg2 = origPtr->_arg2;
        mPtr->_objPtr = origPtr->_objPtr;
        //mPtr->_replayToPtr = origPtr->_replayToPtr;
        mPtr->_sendingUid = origPtr->_sendingUid;
        mPtr->_targetPtr = origPtr->_targetPtr;
        mPtr->_callbackPtr = origPtr->_callbackPtr;
        
        return mPtr;
    }
    
    MessagePtr Message::obtain(HandlerPtr hPtr) {
        MessagePtr mPtr = obtain();
        mPtr->_targetPtr = hPtr;
        
        return mPtr;
    }
    
    MessagePtr Message::obtain(HandlerPtr hPtr, RunnablePtr callbackPtr) {
        MessagePtr mPtr = obtain();
        mPtr->_targetPtr = hPtr;
        mPtr->_callbackPtr = callbackPtr;
        
        return mPtr;
    }
    
    MessagePtr Message::obtain(HandlerPtr hPtr, int what) {
        MessagePtr mPtr = obtain();
        mPtr->_targetPtr = hPtr;
        mPtr->_what = what;
        
        return mPtr;

    }
    
    MessagePtr Message::obtain(HandlerPtr hPtr, int what, ObjectPtr objPtr) {
        MessagePtr mPtr = obtain();
        mPtr->_targetPtr = hPtr;
        mPtr->_what = what;
        mPtr->_objPtr = objPtr;
        
        return mPtr;
    }
    
    MessagePtr Message::obtain(HandlerPtr hPtr, int what, int arg1, int arg2) {
        MessagePtr mPtr = obtain();
        mPtr->_targetPtr = hPtr;
        mPtr->_what = what;
        mPtr->_arg1 = arg1;
        mPtr->_arg2 = arg2;
        
        return mPtr;
    }
    
    MessagePtr Message::obtain(HandlerPtr hPtr, int what, int arg1, int arg2, ObjectPtr objPtr) {
        MessagePtr mPtr = obtain();
        mPtr->_targetPtr = hPtr;
        mPtr->_what = what;
        mPtr->_arg1 = arg1;
        mPtr->_arg2 = arg2;
        mPtr->_objPtr = objPtr;
        
        return mPtr;
    }
    
    void Message::updateCheckRecycle(int targetSdkVersion) {
        gCheckRecycle = true;
    }
    
    void Message::recycle() {  // 回收一个消息实体到全局池
        if (isInUse()) {
            if (gCheckRecycle) {
                LOGE(TAG, "This message cannot be recycled because it is still in use.\n");
            }
            return;
        }
        recycleUnchecked();
    }
    
    void Message::recycleUnchecked() {
        _flags = FLAG_IN_USE;
        _what = 0;
        _arg1 = 0;
        _arg2 = 0;
        _objPtr.reset();
        //_replayToPtr.reset();
        _sendingUid = -1;
        _when = 0;
        _targetPtr.reset();
        _callbackPtr.reset();
        
        std::unique_lock<std::mutex> lock(sPoolSync);
        if (sPoolSize < MAX_POOL_SIZE) {
            _nextPtr = sPoolPtr;
            sPoolPtr = shared_from_this();
            ++sPoolSize;
        }
//        LOGD(TAG, "RecycleUnchecked sPoolSize=%d\n", sPoolSize);
    }
    
    void Message::copyFrom(MessagePtr oPtr) {
        _flags = oPtr->_flags & ~FLAGS_TO_CLEAR_ON_COPY_FROM;
        _what = oPtr->_what;
        _arg1 = oPtr->_arg1;
        _arg2 = oPtr->_arg2;
        _objPtr = oPtr->_objPtr;
        //_replayToPtr = oPtr->_replayToPtr;
        _sendingUid = oPtr->_sendingUid;
    }
    
    void Message::sendToTarget() {
        _targetPtr->sendMessage(shared_from_this());
    }
    
    bool Message::isAsynchronous() const {
        return (_flags & FLAG_ASYNCHRONOUS) != 0;
    }
    
    void Message::setAsynchronous(bool async) {
        if (async) {
            _flags |= FLAG_ASYNCHRONOUS;
        } else {
            _flags &= ~FLAG_ASYNCHRONOUS;
        }
    }

    bool Message::isInUse() const {
        return ((_flags & FLAG_IN_USE) == FLAG_IN_USE);
    }
    
    void Message::markInUse() {
        _flags |= FLAG_IN_USE;
    }
    
    std::string Message::toString() {
        return toString(getCurrentTimeStamp());
    }
    
    std::string Message::toString(uint64_t now) {
        std::string b = "{ when=";
        char timeStr[100] = {0};
        sprintf(timeStr, "%lld", (_when - now));
        char arg1Str[100] = {0};
        sprintf(arg1Str, "%d", _arg1);
        
        b.append(timeStr);
        
        if (_targetPtr.get()) {
            if (_callbackPtr.get()) {
                b.append(" callback=");
                char callbackStr[100] = {0};
                sprintf(callbackStr, "%zd", _callbackPtr.get());
                b.append(callbackStr);
            } else {
                b.append(" what=");
                char whatStr[100] = {0};
                sprintf(whatStr, "%d", _what);
                b.append(whatStr);
            }
            
            if (_arg1 != 0) {
                b.append(" arg1=");
                b.append(arg1Str);
            }
            
            if (_arg2 != 0) {
                b.append(" arg2=");
                char arg2Str[100] = {0};
                sprintf(arg2Str, "%d", _arg2);
                b.append(arg2Str);
            }
            
            if (_objPtr.get()) {
                b.append(" obj=");
                char objStr[100] = {0};
                sprintf(objStr, "%zd", _objPtr.get());
                b.append(objStr);
            }
            
            b.append(" target=");
            char targetStr[100] = {0};
            sprintf(targetStr, "%zd", _targetPtr.get());
            b.append(targetStr);

        } else {
            b.append(" barrier=");
            b.append(arg1Str);
        }
        
        b.append(" }");
        return b;
    }
}
