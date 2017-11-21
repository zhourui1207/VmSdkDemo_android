//
// Created by zhou rui on 2017/7/29.
//

#ifndef DREAM_ACKHISTORYWINDOW_H
#define DREAM_ACKHISTORYWINDOW_H

/**
 * ack历史窗口
 */
#include <cstdint>
#include <list>
#include <memory.h>
#include <memory>

namespace Dream {

    class AckInfo {
    public:
        AckInfo(int32_t ackNumber, int32_t seqNumber, uint64_t sendOutTime)
        : _ackNumber(ackNumber), _seqNumber(seqNumber), _sendOutTime(sendOutTime) {

        }

        virtual ~AckInfo() = default;

        int32_t ackNumber() const {
            return _ackNumber;
        }

        int32_t seqNumber() const {
            return _seqNumber;
        }

        uint64_t sendOutTime() const {
            return _sendOutTime;
        }

    private:
        int32_t _ackNumber;
        int32_t _seqNumber;
        uint64_t _sendOutTime;
    };

    class AckHistoryWindow {

    public:
        AckHistoryWindow(std::size_t maxSize);

        virtual ~AckHistoryWindow() = default;

        void addAck(int32_t ackNumber, int32_t seqNumber, uint64_t sendOutTime);

        std::shared_ptr<AckInfo> getLastAckInfo();

        std::shared_ptr<AckInfo> getAckInfoBySeqNumber(int32_t ackSeqNumber);

    private:
        std::size_t _maxSize;
        std::list<std::shared_ptr<AckInfo>> _ackList;
    };

}

#endif //VMSDKDEMO_ANDROID_ACKHISTORYWINDOW_H
