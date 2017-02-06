/*
 * MemoryPool.h
 *
 *  Created on: 2016年10月16日
 *      Author: zhourui
 *      简单实现的一个内存池，使用定长元素，需要元素拥有默认构造器
 */

#ifndef MEMORYPOOL_H_
#define MEMORYPOOL_H_

#include <stdio.h>
#include <mutex>

#include "Singleton.h"

namespace Dream {

    template<typename Element>
    class FreeList {
    public:
        FreeList() : _next(nullptr) {
        }

        virtual ~FreeList() {
            if (_next != nullptr) {  // 如果后面还有内存未释放，则释放先释放后面的内存
                delete _next;
                _next = nullptr;
            }
        };

    public:
        Element _element;  // 保存元素
        FreeList *_next;  // 指向下一个对象，如果为空，则表示没有可用对象了
    };

    template<typename Element>
    class MemoryPool : public Singleton<MemoryPool<Element>> {
    public:
        MemoryPool() : _freeList(nullptr), _allocCount(0), _putoutCount(0) {
            printf("正在构造MemoryPool\n");
        }

        virtual ~MemoryPool() {
            printf("正在析构MemoryPool，总分配个数[%zd]，当前输出内存数量[%zd]\n", _allocCount, _putoutCount);
            if (_freeList != nullptr) {
                delete _freeList;
                _freeList = nullptr;
            }
        }

        void *get() {
            FreeList<Element> *tmp = nullptr;

            _mutex.lock();
            // 没有可用内存
            if (_freeList == nullptr) {
                tmp = new FreeList<Element>();
                tmp->_next = nullptr;
                ++_allocCount;
            } else {  // 有可用内存
                tmp = _freeList;
                _freeList = _freeList->_next;
            }
            ++_putoutCount;

            _mutex.unlock();
            return (void *) tmp;
        }

        void release(void *pElement) {
            _mutex.lock();

            FreeList<Element> *tmp = _freeList;
            _freeList = (FreeList<Element> *) pElement;
            _freeList->_next = tmp;
            --_putoutCount;

            _mutex.unlock();
        }

    private:
        FreeList<Element> *_freeList;  // 这是一个可用对象列表的头
        uint64_t _allocCount;  // 总共分配个数，统计使用，可以看出程序在运行期间，最多同时使用多少个对象
        int64_t _putoutCount;  // 输出的数量，如果在内存池析构时，此数量大于0的话，那么就表示程序没有将内存还给内存池，存在内存泄漏，小于0的话就证明程序释放多次内存
        std::mutex _mutex;  // 锁
    };

} /* namespace Dream */

#endif /* MEMORYPOOL_H_ */
