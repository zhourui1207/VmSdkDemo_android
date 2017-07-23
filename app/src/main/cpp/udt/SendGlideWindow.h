//
// Created by zhou rui on 2017/7/21.
//

#ifndef DREAM_SENDGLIDEWINDOW_H
#define DREAM_SENDGLIDEWINDOW_H

#include <list>
#include "GlideWindow.h"
#include "UDTMultiplexer.h"
#include "packet/UDTDataPacket.h"

/**
 * 发送滑动窗口
 */
namespace Dream {

    class SendGlideWindow : public GlideWindow {

    public:
        SendGlideWindow(std::size_t windowSize = 1);

        virtual ~SendGlideWindow() = default;

        // 能加入滑动窗口的包才能被发送  发送线程调用
        bool trySendPacket(std::shared_ptr<UDTDataPacket> udtDataPacket, uint64_t setupTime);

        // 移除并获取loss列表中第一个序列号  发送线程调用  返回值: true:移除成功 false:移除失败
        bool removeFirstLossPacket(int32_t &seqNumber);

        // 获取包 发送线程调用 用来获取包，然后重传
        std::shared_ptr<UDTDataPacket> getPacket(int32_t seqNumber);


        // 移除已响应包  接收线程调用
        bool ackPacket(int32_t seqNumber);

        // 重发请求 加入losslist 接收线程调用
        bool nakPacket(int32_t seqNumber);

        // 检查超时并且没有丢失包 超时的包加入losslist 接收线程调用  返回: true: loss list空
        bool checkExpAndLossNothing();

        // 接收线程调用
        void setWindowSize(std::size_t windowSize);

        // 接收线程调用
        std::size_t getWindowSize();

    private:
        void addLossList(int32_t seqNumber);

    private:
        std::mutex _mutex;
        std::list<std::shared_ptr<UDTDataPacket>> _packetList;  // 包列表  // 包的序列号是从小到大的，但是并不保证连续
        std::list<int32_t> _lossList;  // 丢失列表  外部传入
    };

}

#endif //VMSDKDEMO_ANDROID_SENDGLIDEWINDOW_H
