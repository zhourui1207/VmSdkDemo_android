//
// Created by zhou rui on 2017/7/24.
//

#ifndef DREAM_SENDLOSSLIST_H
#define DREAM_SENDLOSSLIST_H


#include <stdint.h>
#include <mutex>
#include <list>

namespace Dream {

    class SendGlideWindow;

    class SendLossList {

    public:
        // 移除并获取loss列表中第一个序列号  发送线程调用  返回值: true:移除成功 false:移除失败
        bool removeFirstLossPacket(int32_t &seqNumber);

        bool empty() const;

    private:
        // 接收线程调用
        void addLossList(int32_t seqNumber, int32_t left, int32_t right);

        friend class SendGlideWindow;

    private:
        std::mutex _mutex;
        std::list<int32_t> _lossList;  // 丢失列表  外部传入
    };
}


#endif //VMSDKDEMO_ANDROID_SENDLOSSLIST_H
