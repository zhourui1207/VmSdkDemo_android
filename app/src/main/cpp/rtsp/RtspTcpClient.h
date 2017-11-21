//
// Created by zhou rui on 17/6/1.
//

#ifndef DREAM_RTSPTCPCLIENT_H
#define DREAM_RTSPTCPCLIENT_H

#include "../plateform/VmType.h"
#include "../util/AsioTcpClient.h"

namespace Dream {

    class RtspTcpClient {

    private:
        const char* TAG = "RtspTcpClient";
        static const size_t RECEIVE_SIZE = 100 * 1024;

        enum RTSP_METHOD_STATUS {
            OPTIONS = 0,
            DESCRIBE,
            SETUP_VIDEO,
            SETUP_AUDIO,
            WAIT_PLAY,
            PLAY,
            WAIT_PAUSE,
            PAUSE,
            TEARDOWN
        };

        enum RET_CODE {
            Continue = 100,
            OK = 200,
            Unauthorized = 401
        };

    public:
        RtspTcpClient() = delete;

        RtspTcpClient(const std::string &rtspUrl, bool encrypt = false);

        virtual ~RtspTcpClient();

    public:
        // 打开
        bool startUp();

        // 关闭
        void shutdown();

        void setUser(void *pUser) {
            _pUser = pUser;
        }

        void setStreamCallback(std::function<void(const char *pData, std::size_t dataLen, void *pUser)> dataPacketCallback) {
            _dataPacketCallback = dataPacketCallback;
        }

        void setStreamCallback(fStreamCallBackV2 cbDataPacketCallback) {
            _cbDataPacketCallback = cbDataPacketCallback;
        }

        // 设置连接状态侦听器
        void setConnectStatusListener(std::function<void(bool isConnected, void *pUser)> connectStatusListener) {
            _connectStatusListener = connectStatusListener;
        }

        void setConnectStatusListener(fStreamConnectStatusCallBackV2  cbConnectStatusListener) {
            _cbConnectStatusListener = cbConnectStatusListener;
        }

        bool pause() {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_currentMethodStatus == PLAY) {
                _currentMethodStatus = WAIT_PAUSE;
                return sendPausePacket();
            }
            return false;
        }

        bool play(float speed = -1) {
            std::lock_guard<std::mutex> lock(_mutex);
            if (speed > 0) {
                _scale = speed;
            }
            if (_currentMethodStatus == PAUSE || _currentMethodStatus == PLAY) {
                _currentMethodStatus = WAIT_PLAY;
                return sendPlayPacket();
            }
            return false;
        }

    private:
        bool parseUrl(const std::string& url);

        void onStatus(AsioTcpClient::Status status);

        void receiveData(const char *pBuf, std::size_t dataLen);

        void handleRtspPacket(const std::string& rtspPacket);

        void handleDataPacket(const char* pData, std::size_t dataLen);

        std::string createCommonBuffer(const std::string& method, const std::string& uri, bool baseUrl);

        void addChar(char tmpChar);

        void resetChar();

        void updateStatus();

        bool sendRtspRequest(const char* rtspRequest, size_t len);

        bool sendOptionsPacket();

        bool sendDescribePacket();

        bool sendSetupVideoPacket();

        bool sendSetupAudioPacket();

        bool sendPlayPacket();

        bool sendPausePacket();

        bool sendTeardownPacket();

        bool sendTeardownPacketSync();

        std::string trim(const std::string& srcStr);

        std::string readValue(const std::string& str, std::size_t pos);

    private:
        const bool _encrypt;  // 是否加密
        std::mutex _mutex;
        RTSP_METHOD_STATUS _currentMethodStatus;

        std::string _serverAddress;  // 服务端地址
        unsigned short _serverPort;  // 服务端端口

        std::string _url;  // 去掉用户名密码后的url
        std::string _baseUrl;  // 去掉参数
        std::string _paramUrl;  // 参数部分

        bool _needAuthorized;  // 是否需要鉴权
        bool _tryAuthorized;  // 是否尝试鉴权
        std::string _userName;  // 用户名
        std::string _password;  // 密码
        std::string _realm;  // 需要鉴权时，由服务端发过来
        std::string _nonce;  // 需要鉴权时，由服务端发过来

        unsigned _cseq;
        std::string _session;

        AsioTcpClient::Status _currentStatus;  // tcp客户端状态

        std::unique_ptr<AsioTcpClient> _asioTcpClientPtr;

        std::function<void(bool isConnected, void *pUser)> _connectStatusListener;
        fStreamConnectStatusCallBackV2  _cbConnectStatusListener;

        std::function<void(const char *pData, std::size_t dataLen, void *pUser)> _dataPacketCallback; // 数据包回调
        fStreamCallBackV2 _cbDataPacketCallback;

        int _channel;
        char _receiveData[RECEIVE_SIZE];  // 接收到的临时数据，用来缓存数据
        std::size_t _receiveLen;  // 缓存数据长度
        std::size_t _dataPacketLen;  // 数据包长度

        char _tmpData[4];  // 查询字节"\r\n\r\n"时使用

        void *_pUser;
        float _scale;
    };

}


#endif //VMSDKDEMO_ANDROID_RTSPTCPCLIENT_H
