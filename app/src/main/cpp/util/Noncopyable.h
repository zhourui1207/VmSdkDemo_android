/*
 * Noncopyable.h
 *
 *  Created on: 2016年9月16日
 *      Author: zhourui
 *      不可复制类
 */

#ifndef NONCOPYABLE_H_
#define NONCOPYABLE_H_

namespace Dream {

class Noncopyable {
protected:
  Noncopyable() = default;
  virtual ~Noncopyable() = default;
  Noncopyable(const Noncopyable&) = delete;
  const Noncopyable& operator=(const Noncopyable&) = delete;
};

} /* namespace Dream */

#endif /* NONCOPYABLE_H_ */
