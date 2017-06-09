/*
 * PrintTimer.h
 *
 *  Created on: 2016年10月16日
 *      Author: zhourui
 *      打印从构造都析构的时间差，这是一个简单的计时器
 */

#ifndef PRINTTIMER_H_
#define PRINTTIMER_H_

#include "public/platform.h"
#include <stdio.h>

namespace Dream {

    class PrintTimer {
    public:
        PrintTimer(const std::string &name = "") : _name(name) {
            _millSeconds = getCurrentTimeStamp();
        }

        virtual ~PrintTimer() {
            LOGW("PrintTimer", "计时器[%s]耗时[%zd]\n", _name.c_str(),
                 getCurrentTimeStamp() - _millSeconds);
        }

    private:
        std::string _name;
        std::time_t _millSeconds;  // 构造时的毫秒数
    };

} /* namespace Dream */

#endif /* PRINTTIMER_H_ */
