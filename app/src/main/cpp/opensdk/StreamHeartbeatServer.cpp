//
// Created by zhou rui on 17/6/9.
//

#include "StreamHeartbeatServer.h"
#include "../util/public/platform.h"
#include "../dp/HB.pb.h"
#include "../plateform/VmType.h"

namespace Dream {

    StreamHeartbeatServer::StreamHeartbeatServer() {

    }

    StreamHeartbeatServer::~StreamHeartbeatServer() {
        shut_down();
    }

    bool StreamHeartbeatServer::start_up() {
        if (_udp_server_ptr.get() != nullptr) {
            return true;
        }

        _udp_server_ptr.reset(
                new AsioUdpServer(std::bind(&StreamHeartbeatServer::receive, this, std::placeholders::_1,
                                            std::placeholders::_2, std::placeholders::_3,
                                            std::placeholders::_4), "", DEFAULT_LOCAL_PORT));

        return _udp_server_ptr->start_up();
    }

    void StreamHeartbeatServer::shut_down() {
        if (_udp_server_ptr.get() != nullptr) {
            _udp_server_ptr->shut_down();
        }
    }

    bool StreamHeartbeatServer::send_packet(const std::string &remote_address, unsigned short port,
                                            unsigned heartbeatType, const std::string &monitorId,
                                            const std::string &srcId) {
        if (_udp_server_ptr.get() == nullptr) {
            return false;
        }

        Command command;
        Heartbeat * hb = command.MutableExtension(Heartbeat::cmd);

        Heartbeat_TaskType taskType;
        switch (heartbeatType) {
            case VMNET_HEARTBEAT_TYPE_REALPLAY:
                taskType = Heartbeat_TaskType_REALPLAY;
                break;
            case VMNET_HEARTBEAT_TYPE_PLAYBACK:
                taskType = Heartbeat_TaskType_RECORDPLAY;
                break;
            case VMNET_HEARTBEAT_TYPE_TALK:
                taskType = Heartbeat_TaskType_TALK;
                break;
            default:
                LOGE("StreamHeartbeatServer", "heartbeat type [%d] unsupported!\n", heartbeatType);
                return false;
        }
        hb->set_taskid(monitorId);
        hb->set_tasktype(taskType);

        int version = 0;
        command.set_srcid(srcId);
        command.set_version(version);
        command.set_type(Command::HEARTBEAT);

        int bytesize = command.ByteSize();
        char sendBuf[bytesize];
        memset(sendBuf, 0, (size_t) bytesize);

        command.SerializeToArray(sendBuf, bytesize);

        auto udpDataPtr = std::make_shared<UdpData>(remote_address, port, bytesize, (char*) sendBuf);
        return _udp_server_ptr->send(udpDataPtr);
    }

    void
    StreamHeartbeatServer::receive(const std::string &remote_address, unsigned short remote_port,
                                   const char *pBuf, std::size_t len) {
        LOGW("RtpUdpServer", "receive[%s], len[%d], [%s], [%d]\n", pBuf, len, remote_address.c_str(), remote_port);
    }

}
