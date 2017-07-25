//
// Created by zhou rui on 2017/7/25.
//

#ifndef DREAM_RECEIVELOSSLIST_H
#define DREAM_RECEIVELOSSLIST_H


#include <stdint.h>
#include <list>
#include <memory>

namespace Dream {

    class ReceiveLossList {

    private:
        std::list<std::shared_ptr<LossInfo>> _lossList;
    };

    class LossInfo {
    public:
        LossInfo(int32_t seqNumber)
        : _seqNumber(seqNumber) {

        }

        virtual ~LossInfo() = default;

        void updateLatestFeedbackTime(uint64_t time) {
            _latestFeedbackTime = time;
        }

        uint64_t latestFeedbackTime() {
            return _latestFeedbackTime;
        }

        void setFeedbackNumber(int number) {
            _feedbackNumber = number;
        }

        int feedbackNumber() {
            return _feedbackNumber;
        }

    private:
        int32_t _seqNumber;  // 丢失数据包序列号
        uint64_t _latestFeedbackTime;  // 最后反馈时间
        int _feedbackNumber;  // 反馈次数
    };

}

#endif //VMSDKDEMO_ANDROID_RECEIVELOSSLIST_H
