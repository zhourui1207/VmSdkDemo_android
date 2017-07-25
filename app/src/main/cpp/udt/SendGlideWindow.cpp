//
// Created by zhou rui on 2017/7/21.
//

#include "SendGlideWindow.h"


// left: 最后已确认连续包的最大的序列号
// right: 已发送包的最大序列号
namespace Dream {

    SendGlideWindow::SendGlideWindow(std::size_t windowSize)
            : GlideWindow(windowSize) {

    }

    bool
    SendGlideWindow::trySendPacket(std::shared_ptr<UDTDataPacket> udtDataPacket, uint64_t setupTime) {
        std::unique_lock<std::mutex> lock(_mutex);

        // 第一次发送 || (连续的序列号 && 没到达窗口大小上限)
        if ((_right < 0) || (continuousSeqNumber(_right, udtDataPacket->seqNumber()) &&
                             ((_right - _left) < _windowSize))) {

            _packetList.push_back(udtDataPacket);  // 已发送列表
            uint64_t offsetTimeUs = getCurrentTimeStampMicro() - setupTime;
            udtDataPacket->setSendTimestamp(offsetTimeUs);
            udtDataPacket->setTimestamp((uint32_t) (offsetTimeUs / 1000000));

            // 滑动右边界
            _right = udtDataPacket->seqNumber();  // 发送的序列号
            if (_left < 0) {  // 第一次发送
                _left = decreaseSeqNumber(_right);  // 已接收的包为发送包的前一个序列号
            }
            return true;
        } else {
            return false;
        }
    }

    std::shared_ptr<UDTDataPacket> SendGlideWindow::getPacket(int32_t seqNumber) {
        std::unique_lock<std::mutex> lock(_mutex);

        std::shared_ptr<UDTDataPacket> packetPtr;
        if (!_packetList.empty()) {
            auto packetIt = _packetList.begin();
            bool normal = (_left < _right);
            for (; packetIt != _packetList.end(); ++packetIt) {
                if ((*packetIt)->seqNumber() == seqNumber) {
                    packetPtr = *packetIt;
                }
                // 如果元素大于seqnumber，那么后面就不可能再有相等的了
                if (normal && ((*packetIt)->seqNumber() > seqNumber)) {
                    break;
                }
                if (!normal) {
                    int32_t eleSeqNumber = (*packetIt)->seqNumber();
                    if (eleSeqNumber > _left) {
                        eleSeqNumber = ~eleSeqNumber + 1;  // 负数
                    }
                    int32_t tmpSeqNumber = seqNumber;
                    if (seqNumber > _left) {
                        seqNumber = ~seqNumber + 1;
                    }
                    if (tmpSeqNumber < eleSeqNumber) {
                        break;
                    }
                }
            }
        }
        return packetPtr;
    }

    bool SendGlideWindow::ackPacket(int32_t seqNumber) {
        std::unique_lock<std::mutex> lock(_mutex);

        // 移除包在滑动窗口范围内
        if ((_left >= 0 && _right >= 0) &&
            ((_right > _left) && (seqNumber > _left) && (seqNumber <= _right)) ||
            ((_right < _left) && (seqNumber > _left || seqNumber <= _right))) {

            // 从窗口中移除
            auto packetIt = _packetList.begin();
            for (; packetIt != _packetList.end(); ++packetIt) {
                if (seqNumber == (*packetIt)->seqNumber()) {
                    _packetList.erase(packetIt);
                    break;
                }
            }

            // 当前被移除的序列号等于左边界+1
            if (continuousSeqNumber(_left, seqNumber)) {
                // 如果窗口中还存在未确认包，那么左边界等于第一个未确认包序列号-1
                if (_packetList.size() > 0) {
                    _left = decreaseSeqNumber((*(_packetList.begin()))->seqNumber());
                } else {
                    _left = _right;  // 全部被确认
                }
            }  // 不连续于左边界的话，就不移动边界

            return true;
        } else {
            return false;
        }
    }

    void SendGlideWindow::nakPacket(SendLossList& sendLossList, int32_t seqNumber) {
        std::unique_lock<std::mutex> lock(_mutex);

        sendLossList.addLossList(seqNumber, _left, _right);
    }

    bool SendGlideWindow::checkExpAndLossNothing(SendLossList& sendLossList) {
        std::unique_lock<std::mutex> lock(_mutex);

        if (!_packetList.empty()) {
            // 倒序检查提高性能
            auto packetIt = --(_packetList.end());
            for (; packetIt != _packetList.begin(); --packetIt) {
                sendLossList.addLossList((*packetIt)->seqNumber(), _left, _right);
            }
            sendLossList.addLossList((*packetIt)->seqNumber(), _left, _right);
        }

        return sendLossList.empty();
    }
}