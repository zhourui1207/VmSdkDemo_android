//
// Created by zhou rui on 2017/7/29.
//

#include "AckHistoryWindow.h"

namespace Dream {

    AckHistoryWindow::AckHistoryWindow(std::size_t maxSize): _maxSize(maxSize) {

    }

    void AckHistoryWindow::addAck(int32_t ackNumber, int32_t seqNumber, uint64_t sendOutTime) {
        if (_ackList.size() >= _maxSize) {
            _ackList.pop_front();
        }

        auto ackPtr = std::make_shared<AckInfo>(ackNumber, seqNumber, sendOutTime);
        _ackList.push_back(ackPtr);
    }

    std::shared_ptr<AckInfo> AckHistoryWindow::getLastAckInfo() {
        std::shared_ptr<AckInfo> ackPtr;
        if (!_ackList.empty()) {
            return _ackList.back();
        }
        return ackPtr;
    }

    std::shared_ptr<AckInfo> AckHistoryWindow::getAckInfoBySeqNumber(int32_t ackSeqNumber) {
        std::shared_ptr<AckInfo> ackPtr;
        if (!_ackList.empty()) {
            auto ackInfoIt = _ackList.begin();
            for (; ackInfoIt != _ackList.end(); ++ackInfoIt) {
                if (ackSeqNumber == (*ackInfoIt)->ackNumber()) {
                    ackPtr = *ackInfoIt;
                }
            }
        }
        return ackPtr;
    }
}