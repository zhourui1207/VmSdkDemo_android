//
// Created by zhou rui on 2017/9/8.
//

#include <assert.h>

#include "Looper.h"
#include "Handler.h"
#include "public/platform.h"

namespace Dream {
    
    BlockingRunnable::BlockingRunnable(RunnablePtr taskPtr)
    : _taskPtr(taskPtr), _done(false) {
        
    }
    
    void BlockingRunnable::run() {
        _taskPtr->run();
        
        std::unique_lock<std::mutex> lock(_mutex);
        _done = true;
        _condition.notify_all();
    }
    
    bool BlockingRunnable::postAndWait(HandlerPtr handlerPtr, int64_t timeout) {
        if (!handlerPtr->post(shared_from_this())) {
            return false;
        }
        
        std::unique_lock<std::mutex> lock(_mutex);
        if (timeout > 0) {
            int64_t expirationTime = getCurrentTimeStamp() + timeout;
            while (!_done) {
                int64_t delay = expirationTime - getCurrentTimeStamp();
                if (delay <= 0) {
                    return false;
                }
                _condition.wait_for(lock, std::chrono::milliseconds(delay));
            }
        } else {
            while (!_done) {
                _condition.wait(lock);
            }
        }
        return true;
    }
    
    void Handler::dispatchMessage(MessagePtr msgPtr) {
        if (msgPtr->_callbackPtr.get()) {
            handleCallback(msgPtr);
        } else {
            if (_callbackPtr.get()) {
                if (_callbackPtr->handleMessage(msgPtr)) {
                    return;
                }
            }
            handleMessage(msgPtr);
        }
    }
    
    Handler::Handler() {
        CallbackPtr callbackPtr;
        init(callbackPtr, false);
    }
    
    Handler::Handler(CallbackPtr callbackPtr) {
        init(callbackPtr, false);
    }
    
    Handler::Handler(LooperPtr looperPtr) {
        CallbackPtr callbackPtr;
        init(looperPtr, callbackPtr, false);
    }
    
    Handler::Handler(LooperPtr looperPtr, CallbackPtr callbackPtr) {
        init(looperPtr, callbackPtr, false);
    }
    
    Handler::Handler(bool async) {
        CallbackPtr callbackPtr;
        init(callbackPtr, async);
    }
    
    Handler::Handler(CallbackPtr callbackPtr, bool async) {
        init(callbackPtr, async);
    }
    
    Handler::Handler(LooperPtr looperPtr, CallbackPtr callbackPtr, bool async) {
        init(looperPtr, callbackPtr, async);
    }

    Handler::~Handler() {

    }
    
    void Handler::init(CallbackPtr callbackPtr, bool async) {
        _looperPtr = Looper::myLooper();
        assert(_looperPtr.get());
        _queuePtr = _looperPtr->_queuePtr;
        _callbackPtr = callbackPtr;
        _asynchronous = async;
    }
    
    void Handler::init(LooperPtr looperPtr, CallbackPtr callbackPtr, bool async) {
        _looperPtr = looperPtr;
        _queuePtr = looperPtr->_queuePtr;
        _callbackPtr = callbackPtr;
        _asynchronous = async;
    }
    
    MessagePtr Handler::obtainMessage() {
        return Message::obtain(shared_from_this());
    }
    
    MessagePtr Handler::obtainMessage(int what) {
        return Message::obtain(shared_from_this(), what);
    }
    
    MessagePtr Handler::obtainMessage(int what, ObjectPtr objPtr) {
        return Message::obtain(shared_from_this(), what, objPtr);
    }
    
    MessagePtr Handler::obtainMessage(int what, int arg1, int arg2) {
        return Message::obtain(shared_from_this(), what, arg1, arg2);
    }
    
    MessagePtr Handler::obtainMessage(int what, int arg1, int arg2, ObjectPtr objPtr) {
        return Message::obtain(shared_from_this(), what, arg1, arg2, objPtr);
    }
    
    bool Handler::post(RunnablePtr rPtr) {
        return sendMessageDelayed(getPostMessage(rPtr), 0);
    }
    
    bool Handler::postAtTime(RunnablePtr rPtr, int64_t uptimeMillis) {
        return sendMessageAtTime(getPostMessage(rPtr), uptimeMillis);
    }
    
    bool Handler::postAtTime(RunnablePtr rPtr, ObjectPtr tokenPtr, int64_t uptimeMillis) {
        return sendMessageAtTime(getPostMessage(rPtr, tokenPtr), uptimeMillis);
    }
    
    bool Handler::postDelayed(RunnablePtr rPtr, int64_t delayMillis) {
        return sendMessageDelayed(getPostMessage(rPtr), delayMillis);
    }
    
    bool Handler::postAtFrontOfQueue(RunnablePtr rPtr) {
        return sendMessageAtFrontOfQueue(getPostMessage(rPtr));
    }
    
    bool Handler::runWithScissors(RunnablePtr rPtr, int64_t timeout) {
        assert(rPtr.get());
        assert(timeout >= 0);
        
        if (Looper::myLooper().get() == _looperPtr.get()) {
            rPtr->run();
            return true;
        }
        
        std::shared_ptr<BlockingRunnable> brPtr = std::shared_ptr<BlockingRunnable>(new BlockingRunnable(rPtr));
        return brPtr->postAndWait(shared_from_this(), timeout);
    }
    
    void Handler::removeCallbacks(RunnablePtr rPtr) {
        ObjectPtr objPtr;
        _queuePtr->removeMessages(shared_from_this(), rPtr, objPtr);
    }
    
    void Handler::removeCallbacks(RunnablePtr rPtr, ObjectPtr tokenPtr) {
        _queuePtr->removeMessages(shared_from_this(), rPtr, tokenPtr);
    }
    
    bool Handler::sendMessage(MessagePtr msgPtr) {
        return sendMessageDelayed(msgPtr, 0);
    }
    
    bool Handler::sendEmptyMessage(int what) {
        return sendEmptyMessageDelayed(what, 0);
    }
    
    bool Handler::sendEmptyMessageDelayed(int what, int64_t delayMillis) {
        MessagePtr msgPtr = Message::obtain();
        msgPtr->_what = what;
        return sendMessageDelayed(msgPtr, delayMillis);
    }
    
    bool Handler::sendEmptyMessageAtTime(int what, int64_t uptimeMillis) {
        MessagePtr msgPtr = Message::obtain();
        msgPtr->_what = what;
        return sendMessageAtTime(msgPtr, uptimeMillis);
    }
    
    bool Handler::sendMessageDelayed(MessagePtr msgPtr, int64_t delayMillis) {
        if (delayMillis < 0) {
            delayMillis = 0;
        }
        return sendMessageAtTime(msgPtr, getCurrentTimeStamp() + delayMillis);
    }
    
    bool Handler::sendMessageAtTime(MessagePtr msgPtr, int64_t uptimeMillis) {
        MessageQueuePtr queuePtr = _queuePtr;
        if (!queuePtr.get()) {
            LOGW("Looper", "SendMessageAtTime() called with no mQueue\n");
            return false;
        }
        return enqueueMessage(queuePtr, msgPtr, uptimeMillis);
    }
    
    bool Handler::sendMessageAtFrontOfQueue(MessagePtr msgPtr) {
        MessageQueuePtr queuePtr = _queuePtr;
        if (!queuePtr.get()) {
            LOGW("Looper", "SendMessageAtTime() called with no mQueue\n");
            return false;
        }
        return enqueueMessage(queuePtr, msgPtr, 0);
    }
    
    bool Handler::enqueueMessage(MessageQueuePtr queuePtr, MessagePtr msgPtr, int64_t uptimeMillis) {
        msgPtr->_targetPtr = shared_from_this();
        if (_asynchronous) {
            msgPtr->setAsynchronous(true);
        }
        return queuePtr->enqueueMessage(msgPtr, uptimeMillis);
    }
    
    void Handler::removeMessages(int what) {
        ObjectPtr objPtr;
        _queuePtr->removeMessages(shared_from_this(), what, objPtr);
    }
    
    void Handler::removeMessage(int what, ObjectPtr objectPtr) {
        _queuePtr->removeMessages(shared_from_this(), what, objectPtr);
    }
    
    void Handler::removeCallbacksAndMessages(ObjectPtr tokenPtr) {
        _queuePtr->removeCallbacksAndMessages(shared_from_this(), tokenPtr);
    }
    
    bool Handler::hasMessages(int what) {
        ObjectPtr objPtr;
        return _queuePtr->hasMessages(shared_from_this(), what, objPtr);
    }
    
    bool Handler::hasMessages(int what, ObjectPtr objectPtr) {
        return _queuePtr->hasMessages(shared_from_this(), what, objectPtr);

    }
    
    bool Handler::hasCallbacks(RunnablePtr rPtr) {
        ObjectPtr objPtr;
        return _queuePtr->hasMessages(shared_from_this(), rPtr, objPtr);
    }
    
    LooperPtr Handler::getLooper() {
        return _looperPtr;
    }
    
    MessagePtr Handler::getPostMessage(RunnablePtr rPtr) {
        MessagePtr mPtr = Message::obtain();
        mPtr->_callbackPtr = rPtr;
        return mPtr;
    }
    
    MessagePtr Handler::getPostMessage(RunnablePtr rPtr, ObjectPtr tokenPtr) {
        MessagePtr mPtr = Message::obtain();
        mPtr->_objPtr = tokenPtr;
        mPtr->_callbackPtr = rPtr;
        return mPtr;
    }
    
    void Handler::handleCallback(MessagePtr messagePtr) {
        messagePtr->_callbackPtr->run();
    }

}
