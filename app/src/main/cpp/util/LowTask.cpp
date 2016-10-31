/*
 * TestTask.cpp
 *
 *  Created on: 2016年9月12日
 *      Author: zhourui
 */
#include <stdio.h>

#include "LowTask.h"

namespace Dream {

LowTask::LowTask(const std::string& name)
: Task(LOW)
, _name(name) {
  printf("构造LowTask\n");
}

LowTask::LowTask(const LowTask&)
: Task(LOW) {
  printf("复制构造LowTask\n");
}

LowTask::~LowTask() {
  printf("析构LowTask\n");
}

int LowTask::doTask() {
  printf("LowTask::doTask\n");
  return 0;
}

const std::string LowTask::taskType() const {
  return _name;
}


} /* namespace Dream */
