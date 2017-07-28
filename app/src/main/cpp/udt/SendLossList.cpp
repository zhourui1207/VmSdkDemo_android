//
// Created by zhou rui on 2017/7/24.
//

#include "SendLossList.h"

namespace Dream {

    bool SendLossList::removeFirstLossPacket(int32_t &seqNumber) {
        std::unique_lock<std::mutex> lock(_mutex);

        if (!_lossList.empty()) {
            seqNumber = *(_lossList.begin());
            _lossList.pop_front();
            return true;
        }

        return false;
    }

    bool SendLossList::empty() const {
        return _lossList.empty();
    }

    void SendLossList::addLossList(int32_t seqNumber, int32_t left, int32_t right) {
        std::unique_lock<std::mutex> lock(_mutex);

        bool exist = false;
        auto insertPositionIt = _lossList.begin();
        bool normal = (left < right);  // true:正常情况 false:到最大值重置时
        for (; insertPositionIt != _lossList.end(); ++insertPositionIt) {
            int32_t eleSeqNumber = *insertPositionIt;
            if (seqNumber == eleSeqNumber) {  // 相等
                exist = true;
                break;
            }
            if (normal && (seqNumber < eleSeqNumber)) {  // 假如元素大于seqnumber，那么应插到元素的前面
                break;
            }
            if (!normal) {
                // 假如元素值大于左边界，那么就转成负数来比较
                if (eleSeqNumber > left) {
                    eleSeqNumber = ~eleSeqNumber + 1;  // 负数
                }
                int32_t tmpSeqNumber = seqNumber;
                if (tmpSeqNumber > left) {
                    tmpSeqNumber = ~tmpSeqNumber + 1;
                }
                if (tmpSeqNumber < eleSeqNumber) {
                    break;
                }
            }
        }

        if (!exist) {  // 如果不存在，那么插入列表
            _lossList.insert(insertPositionIt, seqNumber);
        }
    }
}