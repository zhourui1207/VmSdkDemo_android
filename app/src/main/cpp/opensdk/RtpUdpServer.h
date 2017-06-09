//
// Created by zhou rui on 17/5/3.
//

#ifndef VMSDKDEMO_ANDROID_RTPUDPSERVER_H
#define VMSDKDEMO_ANDROID_RTPUDPSERVER_H

#include <string>
#include "../util/AsioUdpServer.h"
#include "../plateform/VmType.h"

namespace Dream {

    class RtpUdpServer {
    private:
        static const unsigned DEFAULT_LOCAL_PORT = 12726;  // 接收时的默认端口

    public:
        RtpUdpServer();

        ~RtpUdpServer();

    public:
        bool start_up();

        void shut_down();

        bool send_packet(const std::string &remote_address, unsigned short port, const char *data,
                         size_t data_len);

        void setStreamCallback(fStreamCallBackV3 cbDataPacketCallback) {
            _cbDataPacketCallback = cbDataPacketCallback;
        }

        void setUser(void *pUser) {
            _pUser = pUser;
        }

    private:
        void
        receive(const std::string &remote_address, unsigned short remote_port, const char *pBuf,
                std::size_t len);

    private:
        std::unique_ptr<AsioUdpServer> _udp_server_ptr;
        unsigned short _seq_number;

        fStreamCallBackV3 _cbDataPacketCallback;
        void* _pUser;
    };

}


#endif //VMSDKDEMO_ANDROID_TALKSERVER_H
