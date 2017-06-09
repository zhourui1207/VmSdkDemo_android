/*
 * PacketData.h
 *
 *  Created on: 2016年9月29日
 *      Author: zhourui
 *
 *      包数据
 */

#ifndef UDPDATA_H_
#define UDPDATA_H_

#include <cstring>
#include <string>

namespace Dream {

// todo 看情况需要改成使用内存池提高性能
    class UdpData {
    private:
        static const std::size_t UDP_DATA_DEFAULT_SIZE = 65507;

    public:
        UdpData() = delete;

        UdpData(const std::string &remote_address, unsigned short remote_port, std::size_t length,
                const char *data = nullptr)
                : _remote_address(remote_address), _remote_port(remote_port), _length(length) {
            // printf("PacketData 构造\n");
            if (length > UDP_DATA_DEFAULT_SIZE) {  // 超出最大值
                printf("传入长度[%zd]超出允许的最大值[%zd]\n", length, UDP_DATA_DEFAULT_SIZE);
                throw std::bad_alloc();
            }
//    memset(_data, 0, PACKET_DATA_DEFAULT_SIZE);
            if (data != nullptr) {
                memcpy(_data, data, _length);
            }
        }

        virtual ~UdpData() {
            // printf("PacketData 析构\n");
        }

        // 暂时不考虑使用内存池，测试出来，在线程池使用锁的情况下，性能比new、delete略差，不使用锁时，性能大概是new、delete的两倍，
        // 由于多线程情况下必须使用锁，所以如果不是碎片问题，暂不考虑使用内存池
//  void* operator new(size_t len) {
//    return _pool->get();
//  }
//
//  void operator delete(void* p) {
//    _pool->release(p);
//  }
        std::string address() {
            return _remote_address;
        }

        unsigned short port() {
            return _remote_port;
        }

        char *data() {
            return _data;
        }

        std::size_t length() {
            return _length;
        }


    private:
        std::string _remote_address;
        unsigned short _remote_port;
        std::size_t _length;
        char _data[UDP_DATA_DEFAULT_SIZE];
    };

} /* namespace Dream */

#endif /* PACKETDATA_H_ */
