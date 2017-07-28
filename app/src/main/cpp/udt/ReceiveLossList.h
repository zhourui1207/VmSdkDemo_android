//
// Created by zhou rui on 2017/7/25.
//

#ifndef DREAM_RECEIVELOSSLIST_H
#define DREAM_RECEIVELOSSLIST_H


#include <stdint.h>
#include <list>
#include <memory>

namespace Dream {

    class LossInfo {
    public:
        LossInfo(int32_t seqNumber, uint64_t feedbackTime)
                : _seqNumber(seqNumber), _latestFeedbackTime(feedbackTime),
                  _feedbackNumber(1) {

        }

        virtual ~LossInfo() = default;

        int32_t seqNumber() const {
            return _seqNumber;
        }

        void updateLatestFeedbackTime(uint64_t feedbackTime) {
            _latestFeedbackTime = feedbackTime;
        }

        uint64_t latestFeedbackTime() const {
            return _latestFeedbackTime;
        }

        void setFeedbackNumber(int number) {
            _feedbackNumber = number;
        }

        int feedbackNumber() const {
            return _feedbackNumber;
        }

    private:
        int32_t _seqNumber;  // 丢失数据包序列号
        uint64_t _latestFeedbackTime;  // 最后反馈时间
        int _feedbackNumber;  // 反馈次数
    };

    class ReceiveLossList {
    public:
        ReceiveLossList() = default;

        virtual ~ReceiveLossList() = default;

        void addLossPacket(int32_t seqNumber, uint64_t feedbackTime, int32_t left, int32_t right);

        bool removeLossPacket(int32_t seqNumber);

    private:
        std::list<std::shared_ptr<LossInfo>> _lossList;
    };

}

#endif //VMSDKDEMO_ANDROID_RECEIVELOSSLIST_H
