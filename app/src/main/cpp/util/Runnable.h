//
// Created by zhou rui on 2017/9/11.
//

#ifndef DREAM_RUNNABLE_H
#define DREAM_RUNNABLE_H

#include <memory>

namespace Dream {

    //class Runnable;

    class Runnable : public std::enable_shared_from_this<Runnable> {
    public:
        Runnable() = default;
        
        virtual ~Runnable() = default;
        
        virtual void run() = 0;
    };
    
    using RunnablePtr = std::shared_ptr<Runnable>;
}

#endif //VMSDKDEMO_ANDROID_RUNNABLE_H
