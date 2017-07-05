//
// Created by zhou rui on 17/6/29.
//

#include "P2PKeepAlive.h"

namespace Dream {

    P2PKeepAlive::P2PKeepAlive(const std::string &client_id, const std::string &client_pwd,
                               const std::string &remote_addr, unsigned short remote_port,
                               const std::string &local_addr, unsigned short local_port) :
		    _first(true), _client_id(client_id), _client_pwd(client_pwd), _remote_addr(remote_addr),
            _remote_port(remote_port) {
        _udp_server_ptr.reset(new AsioUdpServer(
                std::bind(&P2PKeepAlive::receive, this, std::placeholders::_1,
                          std::placeholders::_2, std::placeholders::_3, std::placeholders::_4),
                local_addr, local_port));
    }

    P2PKeepAlive::~P2PKeepAlive() {
        shut_down();
    }

    bool P2PKeepAlive::start_up() {
        if (_udp_server_ptr.get() != nullptr) {
            if (_udp_server_ptr->start_up()) {
                // 如果是客户端，那么需要开始保活
                if (is_client()) {
                    start_keep_alive();
                }
                return true;
            }
        }
        return false;
    }

    void P2PKeepAlive::shut_down() {
        stop_keep_alive();
        if (_udp_server_ptr.get() != nullptr) {
            _udp_server_ptr->shut_down();
            _udp_server_ptr.reset();
        }
    }

    bool P2PKeepAlive::is_client() {
        return (!_remote_addr.empty() && _remote_port > 0);
    }

    void P2PKeepAlive::receive(const std::string &remote_address, unsigned short remote_port,
                               const char *pBuf, std::size_t len) {
        if (is_client()) {  // 如果是客户端
            // 首先要判断是否是服务端发来的包
            if (remote_address == _remote_addr && remote_port == _remote_port) {
                std::string msg = std::string(pBuf, len);
                int msgTypeEndPos = msg.find(";");
                if (msgTypeEndPos != std::string::npos) {
                    std::string msgType = msg.substr(0, (unsigned int) msgTypeEndPos);
                    if (msgType == "keep-alive-ok") {
                        int natAddrEndPos = msg.find(";", (unsigned int) (msgTypeEndPos + 1));
                        if (natAddrEndPos != std::string::npos) {
                            std::string natAddr = msg.substr(
                                    (unsigned int) (msgTypeEndPos + 1),
                                    (unsigned int) (natAddrEndPos - msgTypeEndPos - 1));
                            if (!natAddr.empty()) {
                                std::string natPort = msg.substr(
                                        (unsigned int) (natAddrEndPos + 1),
                                        std::string::npos);
                                if (!natPort.empty()) {
                                    unsigned short port = (unsigned short) atoi(
                                            natPort.c_str());

									if (_first) {
										_first = false;
										LOGW(TAG, "Receive data from server[%s:%d]\n", natAddr.c_str(), port);
									}
                                }
                            }
                        }
                    }
                }
            }
			else {
				LOGW(TAG, "Receive data from server[%s:%d]\n", remote_address.c_str(), remote_port);
			}
        } else {  // 服务端
            // 添加到客户ip地址表中，并应答消息
            std::string msg = std::string(pBuf, len);
            int msgTypeEndPos = msg.find(";");
            if (msgTypeEndPos != std::string::npos) {
                std::string msgType = msg.substr(0, (unsigned int) msgTypeEndPos);
                if (msgType == "keep-alive") {
                    int clientIdEndPos = msg.find(";", (unsigned int) (msgTypeEndPos + 1));
                    if (clientIdEndPos != std::string::npos) {
                        std::string clientId = msg.substr((unsigned int) (msgTypeEndPos + 1),
                                                          (unsigned int) (clientIdEndPos -
                                                                          msgTypeEndPos - 1));
                        if (!clientId.empty()) {
                            int clientPwdEndPos = msg.find(";",
                                                           (unsigned int) (clientIdEndPos + 1));
                            if (clientPwdEndPos != std::string::npos) {
                                std::string clientPwd = msg.substr(
                                        (unsigned int) (clientIdEndPos + 1),
                                        (unsigned int) (clientPwdEndPos -
                                                        clientIdEndPos - 1));
                                /*if (!clientPwd.empty()) {*/
                                    int internalAddrEndPos = msg.find(";", (unsigned int) (
                                            clientPwdEndPos + 1));
                                    if (internalAddrEndPos != std::string::npos) {
                                        std::string internalAddr = msg.substr(
                                                (unsigned int) (clientPwdEndPos + 1),
                                                (unsigned int) (internalAddrEndPos -
                                                                clientPwdEndPos - 1));
                                        if (!internalAddr.empty()) {
                                            std::string internalPort = msg.substr(
                                                    (unsigned int) (internalAddrEndPos + 1),
                                                    std::string::npos);
                                            if (!internalPort.empty()) {
                                                unsigned short port = (unsigned short) atoi(
                                                        internalPort.c_str());

                                                client_keep_alive(clientId, clientPwd,
                                                                  remote_address, remote_port,
                                                                  internalAddr, port);
                                                response_keep_alive(remote_address, remote_port);
                                            }
                                        }
                                    }
                                //}
                            }
                        }
                    }
                }
            }
        }
    }

    void P2PKeepAlive::start_keep_alive() {
        if (_time_ptr.get() == nullptr) {
            _time_ptr.reset(new Timer(std::bind(&P2PKeepAlive::keep_alive, this), 0, true,
                                      KEEP_ALIVE_INTERVAL_MS));
            _time_ptr->start();
        }
    }

    void P2PKeepAlive::stop_keep_alive() {
        if (_time_ptr != nullptr) {
            _time_ptr->cancel();
            _time_ptr.reset();
        }
    }

    void P2PKeepAlive::keep_alive() {
        char port[20];
        memset(port, 0, 20);
        sprintf(port, "%d", local_port());
        std::string request =
                "keep-alive;" + _client_id + ";" + _client_pwd + ";" + local_address() + ";" + port;
        send(_remote_addr, _remote_port, request.c_str(), request.length());
    }

    void
    P2PKeepAlive::client_keep_alive(const std::string &client_id, const std::string &client_pwd,
                                    const std::string &nat_addr, unsigned short nat_port,
                                    const std::string &internalAddr, unsigned short internalPort) {
        auto it = _client_session_map.find(client_id);
        std::shared_ptr<P2PClientSession> sessionPtr;
        if (it == _client_session_map.end()) {  // 插入
            sessionPtr = std::make_shared<P2PClientSession>(client_id, client_pwd);
        } else {
            sessionPtr = it->second;
        }

        if (sessionPtr.get() != nullptr) {
            sessionPtr->update_nat_addr(nat_addr);
            sessionPtr->update_nat_port(nat_port);
            sessionPtr->update_internal_addr(internalAddr);
            sessionPtr->update_internal_port(internalPort);
        }
    }

    void P2PKeepAlive::response_keep_alive(const std::string &nat_addr, unsigned short nat_port) {
        char port[20];
        memset(port, 0, 20);
        sprintf(port, "%d", nat_port);
        std::string request = "keep-alive-ok;" + nat_addr + ";" + port;
        send(nat_addr, nat_port, request.c_str(), request.length());
    }
}