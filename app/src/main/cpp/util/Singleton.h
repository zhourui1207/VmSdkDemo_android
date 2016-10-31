/*
 * Singleton.h
 *
 *  Created on: 2016年9月15日
 *      Author: zhourui
 *      单实例类
 *      继承该类使用getInstance方法获取实例，自动调用派生类的无参构造函数，注意的是为了安全，
 *      派生类要把构造函数隐藏成私有，使用者不需要管理生命周期，在程序结束退出时，会自动析构
 */

#ifndef SINGLETON_H_
#define SINGLETON_H_

#include <memory>
#include <mutex>

#include "Noncopyable.h"

namespace Dream {

template<typename T>
class Singleton: public Noncopyable {
public:
  static const std::shared_ptr<T>& getInstance() {
    static std::once_flag oc; //用于call_once的局部静态变量
    static std::shared_ptr<T> _instance;
    std::call_once(oc, [&]() {_instance.reset(new T());});
    return _instance;
  }

protected:
  Singleton() = default;  // 默认构造函数隐藏
  virtual ~Singleton() = default;

};

} /* namespace Dream */

#endif /* SINGLETON_H_ */
