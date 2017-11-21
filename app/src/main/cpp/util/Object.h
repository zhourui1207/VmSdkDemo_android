//
// Created by zhou rui on 2017/9/8.
//

#ifndef DREAM_OBJECT_H
#define DREAM_OBJECT_H

#include <memory>

namespace Dream {

    class Object {
    
    public:
        Object() = default;
        virtual ~Object() = default;
    };
    
    using ObjectPtr = std::shared_ptr<Object>;
}

#endif //VMSDKDEMO_ANDROID_OBJECT_H
