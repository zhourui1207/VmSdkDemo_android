/*
 * Task.cpp
 *
 *  Created on: 2016年9月12日
 *      Author: zhourui
 */
#include <stdio.h>

#include "Task.h"

namespace Dream {

    Task::~Task() {
        // TODO Auto-generated destructor stub
    }

    void Task::printTaskType() const {
        printf("%s\n", taskType().c_str());
    }

    void Task::pushTime(const time_point &time) {
        _timePoint = time;
    }

} /* namespace Dream */
