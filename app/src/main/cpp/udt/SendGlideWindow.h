//
// Created by zhou rui on 2017/7/21.
//

#ifndef DREAM_SENDGLIDEWINDOW_H
#define DREAM_SENDGLIDEWINDOW_H

#include <list>
#include <memory>
#include "GlideWindow.h"
#include "packet/UDTDataPacket.h"
#include "SendLossList.h"

/**
 * 发送滑动窗口
 */
namespace Dream {

    class SendGlideWindow : public GlideWindow {

    public:
        SendGlideWindow(std::size_t windowSize = 1);

        virtual ~SendGlideWindow() = default;

        bool empty();

        // 能加入滑动窗口的包才能被发送  发送线程调用
        bool trySendPacket(std::shared_ptr<UDTDataPacket> udtDataPacket, uint64_t setupTime);

        // 获取包 发送线程调用 用来获取包，然后重传
        std::shared_ptr<UDTDataPacket> getPacket(int32_t seqNumber);


        // 移除已响应包  接收线程调用
        bool ackPacket(int32_t seqNumber);

        // 确认丢失
        void nakPacket(SendLossList& sendLossList, int32_t seqNumber);

        // 检查超时并且没有丢失包 超时的包加入losslist 接收线程调用  返回: true: loss list空
        bool checkExpAndLossNothing(SendLossList& sendLossList);


    private:
        std::mutex _mutex;
        std::list<std::shared_ptr<UDTDataPacket>> _packetList;  // 包列表  // 包的序列号是从小到大的，但是并不保证连续
    };

}

#endif //VMSDKDEMO_ANDROID_SENDGLIDEWINDOW_H
