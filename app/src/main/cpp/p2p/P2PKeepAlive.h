//
// Created by zhou rui on 17/6/29.
//

#ifndef DREAM_P2PKEEPALIVE_H
#define DREAM_P2PKEEPALIVE_H


#include <memory>
#include <map>
#include "../util/AsioUdpServer.h"
#include "../util/Timer.h"
#include "P2PClientSession.h"

namespace Dream {

    class P2PKeepAlive {

    private:
        const static uint64_t KEEP_ALIVE_INTERVAL_MS = 2000;
        const char *TAG = "P2PKeepAlive";

    public:
        P2PKeepAlive(const std::string &client_id, const std::string &client_pwd,
                     const std::string &remote_addr, unsigned short remote_port,
                     const std::string &local_addr, unsigned short local_port);

        ~P2PKeepAlive();

        bool start_up();

        void shut_down();

        bool is_client();

        const std::string local_address() const {
            if (_udp_server_ptr.get() == nullptr) {
                return "";
            }
            return _udp_server_ptr->local_address();
        }

        unsigned short local_port() const {
            if (_udp_server_ptr.get() == nullptr) {
                return 0;
            }
            return _udp_server_ptr->local_port();
        }

		bool send(const std::string &remoteAddr, unsigned short remotePort, const char *buf,
			std::size_t len) {
			if (_udp_server_ptr.get() != nullptr) {
				return _udp_server_ptr->send(remoteAddr, remotePort, buf, len);
			}
			return false;
		}

    private:
        void
        receive(const std::string &remote_address, unsigned short remote_port, const char *pBuf,
                std::size_t len);

        void start_keep_alive();

        void stop_keep_alive();

        void keep_alive();

        void client_keep_alive(const std::string &client_id, const std::string &client_pwd,
                               const std::string &nat_addr, unsigned short nat_port,
                               const std::string &internalAddr, unsigned short internalPort);

        void response_keep_alive(const std::string &nat_addr, unsigned short nat_port);

    private:
		bool _first;
        std::string _client_id;
        std::string _client_pwd;

        std::string _remote_addr;
        unsigned short _remote_port;

        std::unique_ptr<AsioUdpServer> _udp_server_ptr;

        std::unique_ptr<Timer> _time_ptr;
        std::map<std::string, std::shared_ptr<P2PClientSession>> _client_session_map;
    };
}

#endif //VMSDKDEMO_ANDROID_P2PKEEPALIVE_H
