/*
 * UserDatagram.h
 *
 *  Created on: 2017年2月13日
 *      Author: zhourui
 *      用户数据报文,upd协议的数据
 */

#ifndef USERDATAGRAM_H_
#define USERDATAGRAM_H_

namespace Dream {

class UserDatagram {
private:
	/**
	 * 为什么最大是65507?
	 * 因为udp包头有2个byte用于记录包体长度. 2个byte可表示最大值为: 2^16-1=64K-1=65535
	 * udp包头占8字节, ip包头占20字节, 65535-28 = 65507
	 */
	int USER_DATAGRAM_MAX_SIZE = 65507;
public:
	UserDatagram() = delete;

	UserDatagram(std::size_t length, const char* data = nullptr) {
		if (length > USER_DATAGRAM_MAX_SIZE) {
			_length = USER_DATAGRAM_MAX_SIZE;
			_next = new UserDatagram(length - _length, data + _length);
		}

	}

	virtual ~UserDatagram() {
		if (_next) {  // 如果有下一个分片，则析构下一个分片
			delete _next;
			_next = nullptr;
		}
	}

private:
	std::size_t _length;
	char _data[USER_DATAGRAM_MAX_SIZE];
	UserDatagram* _next;  // 指向下一个包的指针，用于超过最大长度时的分片
};

} /* namespace Dream */

#endif /* USERDATAGRAM_H_ */
