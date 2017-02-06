/*
 * TestTask.h
 *
 *  Created on: 2016年9月12日
 *      Author: zhourui
 */

#ifndef HIGHTASK_H_
#define HIGHTASK_H_

#include "Task.h"

namespace Dream {

    class HighTask : public Task {
    public:
        HighTask(const std::string &name);

        HighTask(const HighTask &);

        virtual ~HighTask();

        int doTask() override;

        const std::string taskType() const override;

    private:
        std::string _name;
    };

} /* namespace Dream */

#endif /* TESTTASK_H_ */
