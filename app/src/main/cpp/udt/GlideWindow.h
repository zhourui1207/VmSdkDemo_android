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

        // 接收线程调用
        virtual void setWindowSize(std::size_t windowSize) {
            _windowSize = windowSize;
        }

        // 接收线程调用
        std::size_t getWindowSize() const {
            return _windowSize;
        }

        static bool continuousSeqNumber(int32_t oldSeqNumber, int32_t seqNumber) {
            return (seqNumber == ++oldSeqNumber) ||
                   (oldSeqNumber == UDTBasePacket::SEQUENCE_NUMBER_MAX && seqNumber == 0);
        }

        static int32_t increaseSeqNumber(int32_t seqNumber) {
            if (seqNumber == UDTBasePacket::SEQUENCE_NUMBER_MAX) {
                return 0;
            } else {
                return ++seqNumber;
            }
        }

        static int32_t increaseSeqNumber(int32_t seqNumber, int32_t offset) {
            int32_t tmp = seqNumber + offset;
            if (tmp < 0) {
                tmp = tmp + UDTBasePacket::SEQUENCE_NUMBER_MAX + 1;
            }
            return false;
        }

        static int32_t decreaseSeqNumber(int32_t seqNumber) {
            if (seqNumber == 0) {
                return UDTBasePacket::SEQUENCE_NUMBER_MAX;
            } else {
                return --seqNumber;
            }
        }

        int32_t left() const {
            return _left;
        }

        int32_t right() const {
            return _right;
        }

    protected:
        const char* TAG = "GlideWindow";

        std::size_t _windowSize;  // 窗口大小 未接收到或者未确定的最大值
        int32_t _left;  // 左边界   发送端：已被确认的序列号  接收端：已确认的连续的最大序列号
        int32_t _right;  // 右边界    发送端：已被发送的包序列号  接收端：已确认的最大序列号
    };
}

#endif //VMSDKDEMO_ANDROID_GLIDEWINDOW_H
