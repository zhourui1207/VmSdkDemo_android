//
// Created by zhou rui on 17/6/9.
//

#ifndef DREAM_STREAMHEARTBEATSERVER_H
#define DREAM_STREAMHEARTBEATSERVER_H


#include <string>
#include "../util/AsioUdpServer.h"

namespace Dream {

    class StreamHeartbeatServer {
    private:
        static const unsigned DEFAULT_LOCAL_PORT = 12727;  // 默认端口

    public:
        StreamHeartbeatServer();

        ~StreamHeartbeatServer();

    public:
        bool start_up();

        void shut_down();

        bool
        send_packet(const std::string &remote_address, unsigned short port, unsigned heartbeatType,
                    const std::string &monitorId, const std::string &srcId);

    private:
        void
        receive(const std::string &remote_address, unsigned short remote_port, const char *pBuf,
                std::size_t len);

    private:
        std::unique_ptr<AsioUdpServer> _udp_server_ptr;
    };

}


#endif //VMSDKDEMO_ANDROID_STREAMHEARTBEATSERVER_H
