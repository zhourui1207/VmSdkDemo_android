//
// Created by zhou rui on 2017/7/21.
//

#ifndef DREAM_GLIDEWINDOW_H
#define DREAM_GLIDEWINDOW_H

/**
 * 滑动窗口
 */
#include <stdint.h>
#include <mutex>
#include "packet/UDTBasePacket.h"

namespace Dream {

    class GlideWindow {

    public:
        GlideWindow(std::size_t windowSize = 1)
                : _windowSize(windowSize), _left(-1), _right(-1) {
            if (windowSize == 0) {
                LOGE(TAG, "Window size must be at least is 1\n");
                throw std::bad_alloc();
            }
        }

        virtual ~GlideWindow() = default;

    protected:
        const char* TAG = "GlideWindow";

        bool continuousSeqNumber(int32_t oldSeqNumber, int32_t seqNumber) {
            return (seqNumber == ++oldSeqNumber) ||
                   (oldSeqNumber == UDTBasePacket::SEQUENCE_NUMBER_MAX && seqNumber == 0);
        }

        int32_t increaseSeqNumber(int32_t seqNumber) {
            if (seqNumber == UDTBasePacket::SEQUENCE_NUMBER_MAX) {
                return 0;
            } else {
                return ++seqNumber;
            }
        }

        int32_t decreaseSeqNumber(int32_t seqNumber) {
            if (seqNumber == 0) {
                return UDTBasePacket::SEQUENCE_NUMBER_MAX;
            } else {
                return --seqNumber;
            }
        }

        std::size_t _windowSize;  // 窗口大小 未接收到或者未确定的最大值
        int32_t _left;  // 左边界   发送端：未被确认的序列号+1
        int32_t _right;  // 右边界    发送端：已被发送的包序列号+1
    };
}

#endif //VMSDKDEMO_ANDROID_GLIDEWINDOW_H
