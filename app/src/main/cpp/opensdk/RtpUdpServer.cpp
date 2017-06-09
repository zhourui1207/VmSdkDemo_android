//
// Created by zhou rui on 17/5/3.
//

#include "RtpUdpServer.h"
#include "../rtp/RtpPacket.h"
#include "../util/public/platform.h"

#ifdef _ANDROID

extern JavaVM *g_pJavaVM;  // 定义外部变量，该变量在VmNet-lib.cpp中被定义和赋值
#endif

namespace Dream {

    RtpUdpServer::RtpUdpServer()
            : _seq_number(0), _cbDataPacketCallback(nullptr), _pUser(nullptr) {

    }

    RtpUdpServer::~RtpUdpServer() {
        shut_down();
    }

    bool RtpUdpServer::start_up() {
        if (_udp_server_ptr.get() != nullptr) {
            return true;
        }

        _udp_server_ptr.reset(
                new AsioUdpServer(std::bind(&RtpUdpServer::receive, this, std::placeholders::_1,
                                            std::placeholders::_2, std::placeholders::_3,
                                            std::placeholders::_4), "", DEFAULT_LOCAL_PORT));

        return _udp_server_ptr->start_up();
    }

    void RtpUdpServer::shut_down() {
        if (_udp_server_ptr.get() != nullptr) {
            _udp_server_ptr->shut_down();
        }

#ifdef _ANDROID
        if (g_pJavaVM && _pUser) {
            JNIEnv *jniEnv = nullptr;
            if (g_pJavaVM->GetEnv(reinterpret_cast<void **>(&jniEnv), JNI_VERSION_1_6) == JNI_OK) {
                if (jniEnv) {
                    try {
                        jniEnv->DeleteGlobalRef(static_cast<jobject>(_pUser));
                        _pUser = nullptr;
                    } catch (...) {

                    }
                }
            }
        }
#endif
    }

    bool
    RtpUdpServer::send_packet(const std::string &remote_address, unsigned short port,
                              const char *data,
                              size_t data_len) {
        if (_udp_server_ptr.get() == nullptr) {
            return false;
        }

        RtpPacket rtpPacket;
        rtpPacket.CreateRtpPacket(++_seq_number, (char *) data, data_len, 8,
                                  (unsigned int) (getCurrentTimeStamp() / 1000L), 1001, true);

        auto udpDataPtr = std::make_shared<UdpData>(remote_address, port,
                                                                        rtpPacket.GetBudderLen(), rtpPacket.GetBuffer());
        return _udp_server_ptr->send(udpDataPtr);
    }

    void RtpUdpServer::receive(const std::string &remote_address, unsigned short remote_port,
                               const char *pBuf, std::size_t len) {
        if (_cbDataPacketCallback) {
            _cbDataPacketCallback(remote_address.c_str(), remote_port, pBuf, len, _pUser);
        }
    }

}
