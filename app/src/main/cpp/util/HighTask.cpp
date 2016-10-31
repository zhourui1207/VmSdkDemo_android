/*
 * TestTask.cpp
 *
 *  Created on: 2016年9月12日
 *      Author: zhourui
 */
#include <stdio.h>

#include "HighTask.h"

namespace Dream {

HighTask::HighTask(const std::string& name)
: Task(HIGH)
, _name(name) {
  printf("构造HighTask\n");
}

HighTask::HighTask(const HighTask&)
: Task(HIGH) {
  printf("复制构造HighTask\n");
}

HighTask::~HighTask() {
  printf("析构HighTask\n");
}

int HighTask::doTask() {
  printf("HighTask::doTask\n");
  return 0;
}

const std::string HighTask::taskType() const {
  return _name;
}

} /* namespace Dream */
