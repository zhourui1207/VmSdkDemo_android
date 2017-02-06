/*
 * TestTask.h
 *
 *  Created on: 2016年9月12日
 *      Author: zhourui
 */

#ifndef LOWTASK_H_
#define LOWTASK_H_

#include "Task.h"

namespace Dream {

    class LowTask : public Task {
    public:
        LowTask(const std::string &name);

        LowTask(const LowTask &);

        virtual ~LowTask();

        int doTask() override;

        const std::string taskType() const override;

    private:
        std::string _name;
    };

} /* namespace Dream */

#endif /* TESTTASK_H_ */
