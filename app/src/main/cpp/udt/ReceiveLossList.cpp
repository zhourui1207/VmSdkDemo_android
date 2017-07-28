//
// Created by zhou rui on 2017/7/25.
//

#include "ReceiveLossList.h"

namespace Dream {

    void ReceiveLossList::addLossPacket(int32_t seqNumber, uint64_t feedbackTime, int32_t left, int32_t right) {
        bool exist = false;
        auto insertPositionIt = _lossList.begin();
        bool normal = (left < right);  // true:正常情况 false:到最大值重置时
        for (; insertPositionIt != _lossList.end(); ++insertPositionIt) {
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

        if (exist) {  // 存在的话，更新反馈时间，反馈次数+1
            (*insertPositionIt)->updateLatestFeedbackTime(feedbackTime);
            (*insertPositionIt)->setFeedbackNumber((*insertPositionIt)->feedbackNumber() + 1);
        } else { // 如果不存在，那么插入列表
            _lossList.insert(insertPositionIt, std::make_shared<LossInfo>(seqNumber, feedbackTime));
        }
    }

    bool ReceiveLossList::removeLossPacket(int32_t seqNumber) {
        auto insertPositionIt = _lossList.begin();
        for (; insertPositionIt != _lossList.end(); ++insertPositionIt) {
            if (seqNumber == (*insertPositionIt)->seqNumber()) {
                _lossList.erase(insertPositionIt);
                return true;
            }
        }
        return false;
    }
}