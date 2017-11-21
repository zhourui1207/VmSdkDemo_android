//
// Created by zhou rui on 2017/7/25.
//

#ifndef DREAM_RECEIVELOSSLIST_H
#define DREAM_RECEIVELOSSLIST_H


#include <stdint.h>
#include <vector>
#include <list>
#include <memory>

namespace Dream {

    class LossInfo {
    public:
        LossInfo(int32_t seqNumber, uint64_t feedbackTime)
                : _seqNumber(seqNumber), _latestFeedbackTime(feedbackTime),
                  _k(2) {

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

        void setK(int number) {
            _k = number;
        }

        int k() const {
            return _k;
        }

    private:
        int32_t _seqNumber;  // 丢失数据包序列号
        uint64_t _latestFeedbackTime;  // 最后反馈时间
        int _k;  // 反馈次数, 初始化为2
    };

    class ReceiveLossList {
    public:
        ReceiveLossList() = default;

        virtual ~ReceiveLossList() = default;

        bool serchNakTimeoutSeqNumber(std::vector<int32_t >& seqNumberList, uint64_t feedbackTime, int32_t ttsUs);

        void addLossPacket(int32_t seqNumber, uint64_t feedbackTime, int32_t left, int32_t right);

        bool removeLossPacket(int32_t seqNumber);

        int32_t getFirstSeqNumber() const;  // -1失败

        bool empty() const;

    private:
        std::list<std::shared_ptr<LossInfo>> _lossList;
    };

}

#endif //VMSDKDEMO_ANDROID_RECEIVELOSSLIST_H
