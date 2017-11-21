//
// Created by zhou rui on 2017/9/8.
//

#include <assert.h>

#include "Looper.h"

namespace Dream {

    std::map<std::thread::id, LooperPtr> Looper::sThreadLocal;
    std::mutex Looper::sThreadLocalMutex;

    void Looper::prepare() {
        prepare(true);
    }

    void Looper::prepare(bool quitAllowed) {
        std::thread::id threadId = std::this_thread::get_id();

        std::unique_lock<std::mutex> lock(sThreadLocalMutex);
        auto looperPtrIt = sThreadLocal.find(threadId);
        assert(looperPtrIt == sThreadLocal.end());  // Only one Looper may be created per thread
        sThreadLocal.insert(std::pair<std::thread::id, LooperPtr>(threadId, std::shared_ptr<Looper>(
                new Looper(quitAllowed))));
    }

    void Looper::loop() {
        LooperPtr mePtr = myLooper();
        assert(mePtr.get());

        MessageQueuePtr queuePtr = mePtr->_queuePtr;

        for (;;) {
            MessagePtr msgPtr = queuePtr->next();
            if (!msgPtr.get()) {
                break;
            }

            msgPtr->_targetPtr->dispatchMessage(msgPtr);

            msgPtr->recycleUnchecked();
        }

        std::unique_lock<std::mutex> lock(sThreadLocalMutex);
        auto looperPtrIt = sThreadLocal.find(std::this_thread::get_id());
        assert(looperPtrIt != sThreadLocal.end());
        sThreadLocal.erase(looperPtrIt);
    }

    LooperPtr Looper::myLooper() {
        LooperPtr looperPtr;
        std::unique_lock<std::mutex> lock(sThreadLocalMutex);

        auto looperPtrIt = sThreadLocal.find(std::this_thread::get_id());
        if (looperPtrIt != sThreadLocal.end()) {
            looperPtr = looperPtrIt->second;
        }
        return looperPtr;
    }

    MessageQueuePtr Looper::myQueue() {
        MessageQueuePtr messageQueuePtr;
        auto myLooperPtr = myLooper();
        if (myLooperPtr.get()) {
            messageQueuePtr = myLooperPtr->_queuePtr;
        }
        return messageQueuePtr;
    }

    Looper::Looper(bool quitAllowed) {
        _queuePtr = std::shared_ptr<MessageQueue>(new MessageQueue(quitAllowed));
        _threadId = std::this_thread::get_id();
    }

    Looper::~Looper() {
//        printf("~Looper()\n");
    }

    bool Looper::isCurrentThread() {
        return std::this_thread::get_id() == _threadId;
    }

    void Looper::quit() {
        _queuePtr->quit(false);
    }

    void Looper::quitSafely() {
        _queuePtr->quit(true);
    }
}
