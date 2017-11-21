//
// Created by zhou rui on 2017/7/23.
//

#ifndef DREAM_RECEIVEGLIDEWINDOW_H
#define DREAM_RECEIVEGLIDEWINDOW_H


#include <memory>
#include <list>
#include "GlideWindow.h"
#include "packet/UDTDataPacket.h"

/**
 * 接收滑动窗口所有接口均为接收线程调用，无需加锁
 */
namespace Dream {

    class ReceiveGlideWindow : public GlideWindow {
    public:
        ReceiveGlideWindow(
                std::function<void(std::shared_ptr<UDTDataPacket>)> reliablePacketCallback,
                std::size_t windowSize = 1);

        virtual ~ReceiveGlideWindow() = default;

        void setInitSeqNumber(int32_t seqNumber);

        // 尝试接收包，如果返回true，则从缓存队列中移除并发送ack，否则不移除
        bool tryReceivePacket(std::shared_ptr<UDTDataPacket> udtDataPacket);

        // 消息丢弃请求，从firstSeqNumber到lastSeqNumber的包都当作已接收到
        void messageDropRequest(int32_t firstSeqNumber, int32_t lastSeqNumber);

        void glideTo(int32_t seqNumber);

        void onReliablePacket(int32_t seqNumber, std::shared_ptr<UDTDataPacket> udtDataPacket);

        // 接收线程调用
        void setWindowSize(std::size_t windowSize);

        std::size_t size() const;

    private:
        std::list<std::shared_ptr<UDTDataPacket>> _packetList;
        std::function<void(std::shared_ptr<UDTDataPacket>)> _reliablePacketCallback;  // 可靠包回调
    };

}

#endif //VMSDKDEMO_ANDROID_RECEIVEGILDEWINDOW_H
