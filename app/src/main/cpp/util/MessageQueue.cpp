//
// Created by zhou rui on 2017/9/8.
//

#include <assert.h>

#include "Looper.h"
#include "MessageQueue.h"
#include "public/platform.h"

namespace Dream {

    MessageQueue::MessageQueue(bool quitAllowed)
    : _quitAllowed(quitAllowed), _pendingIdleHandlers(nullptr), _pendingIdleHandlersSize(0), _quitting(false), _blocked(false), _nextBarrierToken(0), _polling(false) {
        
    }
    
    MessageQueue::~MessageQueue() {
        dispose();
    }
    
    void MessageQueue::dispose() {
        nativeWake();
    
        if (_pendingIdleHandlers) {
            for (int i = 0; i < _pendingIdleHandlersSize; ++i) {
                _pendingIdleHandlers[i].reset();
            }
            delete [] _pendingIdleHandlers;
            _pendingIdleHandlers = nullptr;
            _pendingIdleHandlersSize = 0;
        }
    }
    
    bool MessageQueue::isIdle() {
        std::unique_lock<std::mutex> lock(_thisMutex);
        const int64_t now = getCurrentTimeStamp();
        return _messagesPtr.get() == nullptr || now < _messagesPtr->_when;
    }
    
    void MessageQueue::addIdleHandler(IdleHandlerPtr handlerPtr) {
        assert(handlerPtr.get());
        std::unique_lock<std::mutex> lock(_thisMutex);
        _idleHandlers.emplace_back(handlerPtr);
    }
    
    void MessageQueue::removeIdleHandler(IdleHandlerPtr handlerPtr) {
        std::unique_lock<std::mutex> lock(_thisMutex);
        auto it = _idleHandlers.begin();
        for (; it != _idleHandlers.end(); ++it) {
            if ((*it).get() == handlerPtr.get()) {
                _idleHandlers.erase(it);
                break;
            }
        }
    }
    
    bool MessageQueue::isPolling() {
        std::unique_lock<std::mutex> lock(_thisMutex);
        return isPollingLocked();
    }
    
    bool  MessageQueue::isPollingLocked() {
        return !_quitting && nativeIsPolling();
    }
    
    MessagePtr MessageQueue::next() {
        int pendingIdleHandlerCount = -1;
        int nextPollTimeoutMillis = 0;
        for (;;) {
       
            if (nextPollTimeoutMillis != 0) {
                nativePollOnce(nextPollTimeoutMillis);
            }


            {
                std::unique_lock<std::mutex> lock(_thisMutex);
                uint64_t now = getCurrentTimeStamp();
                MessagePtr prevMsgPtr;
                MessagePtr msgPtr = _messagesPtr;
                if (msgPtr.get() && !msgPtr->_targetPtr.get()) {
                    do {
                        prevMsgPtr = msgPtr;
                        msgPtr = msgPtr->_nextPtr;
                    } while (msgPtr.get() && !msgPtr->isAsynchronous());
                }
                if (msgPtr.get()) {
                    if (now < msgPtr->_when) {
                        nextPollTimeoutMillis = min(int32_t(msgPtr->_when - now), INT32_MAX);
                    } else {
                        _blocked = false;
                        if (prevMsgPtr.get()) {
                            prevMsgPtr->_nextPtr = msgPtr->_nextPtr;
                        } else {
                            _messagesPtr = msgPtr->_nextPtr;
                        }
                        msgPtr->_nextPtr.reset();
                        if (sDEBUG) {
                            LOGD(TAG, "Returning message: %s\n",msgPtr->toString().c_str());
                        }
                        msgPtr->markInUse();
                        return msgPtr;
                    }
                } else {
                    nextPollTimeoutMillis = -1;
                }
                
                if (_quitting) {
                    dispose();
                    MessagePtr tmpMsgPtr;
                    return tmpMsgPtr;
                }
                
                if (pendingIdleHandlerCount < 0 && (!_messagesPtr.get() || now < _messagesPtr->_when)) {
                    pendingIdleHandlerCount = (int)_idleHandlers.size();
                }
                if (pendingIdleHandlerCount <= 0) {
                    _blocked = true;
                    continue;
                }
                
                if (!_pendingIdleHandlers) {
                    _pendingIdleHandlersSize = max(pendingIdleHandlerCount, 4);
                    _pendingIdleHandlers = new IdleHandlerPtr[_pendingIdleHandlersSize];
                }
                toArray(_idleHandlers, _pendingIdleHandlers, _pendingIdleHandlersSize);
            }
            
            for (int i = 0; i < pendingIdleHandlerCount; ++i) {
                IdleHandlerPtr idlerPtr = _pendingIdleHandlers[i];
                _pendingIdleHandlers[i].reset();
                
                bool keep = false;
                keep = idlerPtr->queueIdle();
                
                if (!keep) {
                    std::unique_lock<std::mutex> lock(_thisMutex);
                    for (auto it = _idleHandlers.begin(); it != _idleHandlers.end(); ++it) {
                        if ((*it).get() == idlerPtr.get()) {
                            _idleHandlers.erase(it);
                        }
                    }
                }
                
                pendingIdleHandlerCount = 0;
                
                nextPollTimeoutMillis = 0;
            }
        }
    }
    
    void MessageQueue::quit(bool safe) {
        assert(_quitAllowed);
        
        std::unique_lock<std::mutex> lock(_thisMutex);
        if (_quitting) {
            return;
        }
        _quitting = true;
        
        if (safe) {
            removeAllFutureMessagesLocked();
        } else {
            removeAllMessagesLocked();
        }
        
        nativeWake();
    }
    
    int MessageQueue::postSyncBarrier() {
        return postSyncBarrier(getCurrentTimeStamp());
    }
    
    int MessageQueue::postSyncBarrier(int64_t when) {
        std::unique_lock<std::mutex> lock(_thisMutex);
        const int token = _nextBarrierToken++;
        const MessagePtr msgPtr = Message::obtain();
        msgPtr->markInUse();
        msgPtr->_when = when;
        msgPtr->_arg1 = token;
        
        MessagePtr prevPtr;
        MessagePtr pPtr = _messagesPtr;
        if (when != 0) {
            while (pPtr.get() != nullptr && pPtr->_when <= when) {
                prevPtr = pPtr;
                pPtr = pPtr->_nextPtr;
            }
        }
        if (prevPtr.get()) {
            msgPtr->_nextPtr = pPtr;
            prevPtr->_nextPtr = msgPtr;
        } else {
            msgPtr->_nextPtr = pPtr;
            _messagesPtr = msgPtr;
        }
        return token;
    }
    
    void MessageQueue::removeSyncBarrier(int token) {
        std::unique_lock<std::mutex> lock(_thisMutex);
        MessagePtr prevPtr;
        MessagePtr pPtr = _messagesPtr;
        while (pPtr.get() && (pPtr->_targetPtr || pPtr->_arg1 != token)) {
            prevPtr = pPtr;
            pPtr = pPtr->_nextPtr;
        }
        assert(pPtr.get());
        bool needWake = false;
        if (prevPtr.get()) {
            prevPtr->_nextPtr = pPtr->_nextPtr;
            needWake = false;
        } else {
            _messagesPtr = pPtr->_nextPtr;
            needWake = !_messagesPtr.get() || _messagesPtr->_targetPtr.get();
        }
        pPtr->recycleUnchecked();
        
        if (needWake && !_quitting) {
            nativeWake();
        }
    }
    
    bool MessageQueue::enqueueMessage(MessagePtr msgPtr, int64_t when) {
        assert(msgPtr->_targetPtr.get());
        assert(!msgPtr->isInUse());
        
        std::unique_lock<std::mutex> lock(_thisMutex);
        if (_quitting) {
            LOGE(TAG, "Target[%zd] sending message to a Handler on a dead thread\n", msgPtr->_targetPtr.get());
            msgPtr->recycle();
            return false;
        }
        
        msgPtr->markInUse();
        msgPtr->_when = when;
        MessagePtr pPtr = _messagesPtr;
        bool needWake;
        if (!pPtr.get() || when == 0 || when < pPtr->_when) {
            msgPtr->_nextPtr = pPtr;
            _messagesPtr = msgPtr;
            needWake = _blocked;
        } else {
            needWake = _blocked && !pPtr->_targetPtr && msgPtr->isAsynchronous();
            MessagePtr prevPtr;
            for (;;) {
                prevPtr = pPtr;
                pPtr = pPtr->_nextPtr;
                if (!pPtr.get() || when < pPtr->_when) {
                    break;
                }
                if (needWake && pPtr->isAsynchronous()) {
                    needWake = false;
                }
            }
            msgPtr->_nextPtr = pPtr;
            prevPtr->_nextPtr = msgPtr;
        }
        
        if (needWake) {
            nativeWake();
        }
        return true;
    }
    
    bool MessageQueue::hasMessages(HandlerPtr hPtr, int what, ObjectPtr objectPtr) {
        if (!hPtr.get()) {
            return false;
        }
        
        std::unique_lock<std::mutex> lock(_thisMutex);
        MessagePtr pPtr = _messagesPtr;
        while (pPtr.get()) {
            if (pPtr->_targetPtr.get() == hPtr.get() && pPtr->_what == what && (!objectPtr || pPtr->_objPtr.get() == objectPtr.get())) {
                return true;
            }
            pPtr = pPtr->_nextPtr;
        }
        return false;
    }
    
    bool MessageQueue::hasMessages(HandlerPtr hPtr, RunnablePtr rPtr, ObjectPtr objectPtr) {
        if (!hPtr.get()) {
            return false;
        }
        
        std::unique_lock<std::mutex> lock(_thisMutex);
        MessagePtr pPtr = _messagesPtr;
        while (pPtr.get()) {
            if (pPtr->_targetPtr.get() == hPtr.get() && pPtr->_callbackPtr.get() == rPtr.get() && (!objectPtr.get() || pPtr->_objPtr.get() == objectPtr.get())) {
                return true;
            }
            pPtr = pPtr->_nextPtr;
        }
        return false;
    }
    
    void MessageQueue::removeMessages(HandlerPtr hPtr, int what, ObjectPtr objectPtr) {
        if (!hPtr.get()) {
            return;
        }
        
        std::unique_lock<std::mutex> lock(_thisMutex);
        MessagePtr pPtr = _messagesPtr;
        
        while (pPtr.get() && pPtr->_targetPtr.get() == hPtr.get() && pPtr->_what == what && (!objectPtr.get() || pPtr->_objPtr.get() == objectPtr.get())) {
            MessagePtr nPtr = pPtr->_nextPtr;
            _messagesPtr = nPtr;
            pPtr->recycleUnchecked();
            pPtr = nPtr;
        }
        
        while (pPtr.get()) {
            MessagePtr nPtr = pPtr->_nextPtr;
            if (nPtr.get()) {
                if (nPtr->_targetPtr.get() == hPtr.get() && nPtr->_what == what && (!objectPtr.get() || nPtr->_objPtr.get() == objectPtr.get())) {
                    MessagePtr nnPtr = nPtr->_nextPtr;
                    nPtr->recycleUnchecked();
                    pPtr->_nextPtr = nnPtr;
                    continue;
                }
            }
            pPtr = nPtr;
        }
    }
    
    void MessageQueue::removeMessages(HandlerPtr hPtr, RunnablePtr rPtr, ObjectPtr objectPtr) {
        if (!hPtr.get() || !rPtr.get()) {
            return;
        }
        
        std::unique_lock<std::mutex> lock(_thisMutex);
        MessagePtr pPtr = _messagesPtr;
        
        while (pPtr.get() && pPtr->_targetPtr.get() == hPtr.get() && pPtr->_callbackPtr.get() == rPtr.get() && (!objectPtr.get() || pPtr->_objPtr.get() == objectPtr.get())) {
            MessagePtr nPtr = pPtr->_nextPtr;
            _messagesPtr = nPtr;
            pPtr->recycleUnchecked();
            pPtr = nPtr;
        }
        
        while (pPtr.get()) {
            MessagePtr nPtr = pPtr->_nextPtr;
            if (nPtr.get()) {
                if (nPtr->_targetPtr.get() == hPtr.get() && nPtr->_callbackPtr.get() == rPtr.get() && (!objectPtr.get() || nPtr->_objPtr.get() == objectPtr.get())) {
                    MessagePtr nnPtr = nPtr->_nextPtr;
                    nPtr->recycleUnchecked();
                    pPtr->_nextPtr = nnPtr;
                    continue;
                }
            }
            pPtr = nPtr;
        }
    }
    
    void MessageQueue::removeCallbacksAndMessages(HandlerPtr hPtr, ObjectPtr objectPtr) {
        if (!hPtr.get()) {
            return;
        }
        
        std::unique_lock<std::mutex> lock(_thisMutex);
        MessagePtr pPtr = _messagesPtr;
        
        while (pPtr.get() && pPtr->_targetPtr.get() == hPtr.get() && (!objectPtr.get() || pPtr->_objPtr.get() == objectPtr.get())) {
            MessagePtr nPtr = pPtr->_nextPtr;
            _messagesPtr = nPtr;
            pPtr->recycleUnchecked();
            pPtr = nPtr;
        }
        
        while (pPtr.get()) {
            MessagePtr nPtr = pPtr->_nextPtr;
            if (nPtr.get()) {
                if (nPtr->_targetPtr.get() == hPtr.get() && (!objectPtr.get() || nPtr->_objPtr.get() == objectPtr.get())) {
                    MessagePtr nnPtr = nPtr->_nextPtr;
                    nPtr->recycleUnchecked();
                    pPtr = nnPtr;
                    continue;
                }
            }
            pPtr = nPtr;
        }
    }

    
    void MessageQueue::removeAllMessagesLocked() {
        MessagePtr pPtr = _messagesPtr;
        while (pPtr.get()) {
            MessagePtr nPtr = pPtr->_nextPtr;
            pPtr->recycleUnchecked();
            pPtr = nPtr;
        }
        _messagesPtr.reset();
    }
    
    void MessageQueue::removeAllFutureMessagesLocked() {
        const int64_t now = getCurrentTimeStamp();
        MessagePtr pPtr = _messagesPtr;
        if (pPtr.get()) {
            if (pPtr->_when > now) {
                removeAllMessagesLocked();
            } else {
                MessagePtr nPtr;
                for (;;) {
                    nPtr = pPtr->_nextPtr;
                    if (!nPtr.get()) {
                        return;
                    }
                    if (nPtr->_when > now) {
                        break;
                    }
                    pPtr = nPtr;
                }
                pPtr->_nextPtr.reset();
                do {
                    pPtr = nPtr;
                    nPtr = pPtr->_nextPtr;
                    pPtr->recycleUnchecked();
                } while (nPtr.get());
            }
        }
    }
    
    void MessageQueue::nativePollOnce(int32_t timeoutMillis) {
        std::unique_lock<std::mutex> lock(_nativeMutex);
        _polling = true;
        _condition.wait_for(lock, std::chrono::milliseconds(timeoutMillis));
        _polling = false;
    }
    
    void MessageQueue::nativeWake() {
        std::unique_lock<std::mutex> lock(_nativeMutex);
        if (_polling) {
            _condition.notify_all();
        }
    }
    
    bool MessageQueue::nativeIsPolling() {
        std::unique_lock<std::mutex> lock(_nativeMutex);
        return _polling;
    }
    
}
