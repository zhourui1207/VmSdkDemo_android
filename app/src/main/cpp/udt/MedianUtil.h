//
// Created by zhou rui on 2017/7/30.
//

#ifndef DREAM_MEDIANUTIL_H
#define DREAM_MEDIANUTIL_H

/**
 * 中值计算工具
 */
#include <cstdint>
#include <list>

namespace Dream {
    class MedianUtil {

    public:
        MedianUtil() = default;

        virtual ~MedianUtil() = default;

        void reset() {
            _list.clear();
        }

        /**
         * 增加值
         * @param value
         */
        void add(int64_t value) {
            // 按从小到大顺序排列
            auto valueIt = _list.begin();
            for (; valueIt != _list.end(); ++valueIt) {
                if (*valueIt >= value ) {
                    break;
                }
            }
            _list.insert(valueIt, value);
        }

        /**
         * 获取中值
         * @return
         */
        int64_t getMedianValue() {
            if (_list.size() == 16) {  // 计算时内容大小必须为16，索引0～15计算中值，需要取出7、8
                auto valueIt = _list.begin();
                int position = 0;
                int64_t sum = 0;
                for (; valueIt != _list.end(); ++valueIt) {
                    if (position == 7) {
                        sum += *valueIt;
                    } else if (position == 8) {
                        sum += *valueIt;
                        break;
                    }
                    ++position;
                }
                return sum / 2;
            }
            return 0;
        }

        int32_t getPacketArrivalSpeed(int64_t ai) {
            if (_list.size() == 16 && ai > 0) {
                // 去掉小于AI/8的和大于AI*8的
                int64_t less = ai / 8;
                int64_t greater = ai * 8;
                auto valueIt = _list.begin();
                int64_t sum = 0;
                printf("getPacketArrivalSpeed: ai=%zd ", ai);
                for (; valueIt != _list.end();) {
                    int64_t value = *valueIt;
                    printf("%zd ", value);
                    if (value < less || value > greater) {
                        valueIt = _list.erase(valueIt);
                    } else {  // 计算到总和中
                        sum += value;
                        ++valueIt;
                    }
                }
                printf("\n");

                // 如果剩下的值大于8，计算它们的平均值
                if (_list.size() > 8) {
                    return (int32_t) ((1000000 * _list.size()) / sum);
                }
            }
            return 0;
        }

        int32_t getEstimatedLinkCapacity(int64_t pi) {
            if (pi > 0) {
                return (int32_t) (1000000 / pi);
            }
            return 0;
        }

    private:
        std::list<int64_t > _list;
    };
}

#endif //VMSDKDEMO_ANDROID_MEDIANUTIL_H
