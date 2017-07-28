//
// Created by zhou rui on 2017/7/23.
//

#include "ReceiveGlideWindow.h"

namespace Dream {

    ReceiveGlideWindow::ReceiveGlideWindow(
            std::function<void(std::shared_ptr<UDTDataPacket>)> reliablePacketCallback,
            std::size_t windowSize)
            : GlideWindow(windowSize),
              _reliablePacketCallback(reliablePacketCallback) {

    }

    void ReceiveGlideWindow::setInitSeqNumber(int32_t seqNumber) {
        _left = seqNumber;
        _right = increaseSeqNumber(_left, _windowSize);
    }

    bool ReceiveGlideWindow::tryReceivePacket(std::shared_ptr<UDTDataPacket> udtDataPacket) {
        if (_left < 0 || _right < 0) {
            LOGE(TAG, "SeqNumber is't be init!\n");
            return false;
        }

        int32_t seqNumber = udtDataPacket->seqNumber();
        if (continuousSeqNumber(_left, seqNumber)) {  // 如果是连续的
            onReliablePacket(seqNumber, udtDataPacket);
            // 如果后面的包也是连续的，那么就将包回调给上层，并且从滑动窗口中移除
            if (!_packetList.empty()) {
                auto packetIt = _packetList.begin();
                while (packetIt != _packetList.end()) {
                    int32_t eleSeqNumber = (*packetIt)->seqNumber();
                    if (continuousSeqNumber(_left, eleSeqNumber)) {
                        onReliablePacket(seqNumber, udtDataPacket);
                        packetIt = _packetList.erase(packetIt);
                    } else {
                        break;
                    }
                }
            }
            return true;

        } else {  // 不连续的话，就加入滑动窗口，按序列号递增排列
            bool normal = (_left < _right);  // true:正常情况 false:到最大值重置时
            bool inWindow = false;
            if (normal && (seqNumber > _left) && (seqNumber <= _right)) {
                inWindow = true;
            } else if (!normal && ((seqNumber > _left) && (seqNumber <= _right))) {
                inWindow = true;
            }

            if (inWindow) {
                bool exist = false;
                auto insertPositionIt = _packetList.begin();
                for (; insertPositionIt != _packetList.end(); ++insertPositionIt) {
                    int32_t eleSeqNumber = (*insertPositionIt)->seqNumber();
                    if (seqNumber == eleSeqNumber) {  // 相等
                        exist = true;
                        break;
                    }
                    if (normal && (seqNumber < eleSeqNumber)) {  // 假如元素大于seqnumber，那么应插到元素的前面
                        break;
                    }
                    if (!normal) {
                        // 假如元素值大于左边界，那么就转成负数来比较
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

                if (!exist) {  // 如果不存在，那么插入列表
                    _packetList.insert(insertPositionIt, udtDataPacket);
                }
                return true;
            }
        }

        return false;
    }

    // 暂时只处理在队列最前方的
    void ReceiveGlideWindow::messageDropRequest(int32_t firstSeqNumber, int32_t lastSeqNumber) {
        int32_t wantSeqNumber = increaseSeqNumber(_left);
        bool normal = (firstSeqNumber <= lastSeqNumber);
        bool inWindow = false;
        if (normal && (wantSeqNumber >= firstSeqNumber && wantSeqNumber <= lastSeqNumber)) {
            inWindow = true;
        } else if (!normal && (wantSeqNumber >= firstSeqNumber || wantSeqNumber <= lastSeqNumber)) {
            inWindow = true;
        }
        if (inWindow) {
            glideTo(lastSeqNumber);
        }
    }

    void ReceiveGlideWindow::glideTo(int32_t seqNumber) {
        _left = seqNumber;
        _right = increaseSeqNumber(_left, _windowSize);
    }

    void ReceiveGlideWindow::onReliablePacket(int32_t seqNumber,
                                              std::shared_ptr<UDTDataPacket> udtDataPacket) {
        glideTo(seqNumber);
        if (_reliablePacketCallback && !udtDataPacket->isDrop()) {
            _reliablePacketCallback(udtDataPacket);
        }
    }

    void ReceiveGlideWindow::setWindowSize(std::size_t windowSize) {
        GlideWindow::setWindowSize(windowSize);
        _right = increaseSeqNumber(_left, windowSize);
    }
}