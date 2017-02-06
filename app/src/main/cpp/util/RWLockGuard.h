/*
 * RWLockGuard.h
 *
 *  Created on: 2016年9月14日
 *      Author: zhourui
 */

#ifndef RWLOCKGUARD_H_
#define RWLOCKGUARD_H_

#include "Noncopyable.h"
#include "RWLock.h"

namespace Dream {

    class ReadLockGuard : public Noncopyable {
    public:
        explicit ReadLockGuard(RWLock &mutex)
                : _mutex(mutex) {
            _mutex.readLock();
        }

        ReadLockGuard() = delete;

        virtual ~ReadLockGuard() {
            _mutex.readUnlock();
        }

    private:
        RWLock &_mutex;
    };

    class WriteLockGuard {
    public:
        explicit WriteLockGuard(RWLock &mutex)
                : _mutex(mutex) {
            _mutex.writeLock();
        }

        WriteLockGuard() = delete;

        WriteLockGuard(const WriteLockGuard &) = delete;

        WriteLockGuard &operator=(const WriteLockGuard &) = delete;

        virtual ~WriteLockGuard() {
            _mutex.writeUnlock();
        }

    private:
        RWLock &_mutex;
    };

} /* namespace Dream */

#endif /* RWLOCKGUARD_H_ */
