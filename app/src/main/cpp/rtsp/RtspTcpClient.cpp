//
// Created by zhou rui on 17/6/1.
//

#include "../util/public/platform.h"
#include "RtspTcpClient.h"
#include "authTools.h"
#include "../cipher/jw_cipher.h"

#ifdef _ANDROID

extern JavaVM *g_pJavaVM;  // 定义外部变量，该变量在VmNet-lib.cpp中被定义和赋值
#endif

namespace Dream {

    RtspTcpClient::RtspTcpClient(const std::string &rtspUrl, bool encrypt) :
            _encrypt(encrypt), _currentMethodStatus(TEARDOWN), _cbConnectStatusListener(nullptr),
            _cbDataPacketCallback(nullptr), _channel(-1), _serverPort(0), _needAuthorized(false),
            _tryAuthorized(false), _cseq(0), _dataPacketCallback(nullptr), _receiveLen(0),
            _dataPacketLen(0), _pUser(nullptr), _scale(1) {
        // 解析url
        if (!parseUrl(rtspUrl)) {
            LOGE(TAG, "url invalid： %s\n", rtspUrl.c_str());
        }

        if (_serverAddress.length() > 0 && _serverPort > 0) {
            _asioTcpClientPtr.reset(new AsioTcpClient(
                    std::bind(&RtspTcpClient::receiveData, this, std::placeholders::_1,
                              std::placeholders::_2), _serverAddress, _serverPort));
            // 设置状态侦听
            _asioTcpClientPtr->setStatusListener(
                    std::bind(&RtspTcpClient::onStatus, this, std::placeholders::_1));
        }

        memset(_tmpData, 0, 4);
    }

    RtspTcpClient::~RtspTcpClient() {

    }

    bool RtspTcpClient::parseUrl(const std::string &url) {
        // 如果不是以rtsp://开头
        if (url.find("rtsp://") != 0) {
            LOGE(TAG, "url must begin with rtsp://\n");
            return false;
        }

        // 去除url中的用户名，密码参数
        int authorizedInfoPos = url.find("@");
        if (authorizedInfoPos != std::string::npos && authorizedInfoPos > 6) {
            // 获取用户名和密码
            std::string userNameAndPassword = url.substr(7, (unsigned int) (authorizedInfoPos - 7));
            // 判断是否有密码，用户名和密码之间用:分隔
            int passwordPos = userNameAndPassword.find(":");
            if (passwordPos == std::string::npos) {
                _userName = userNameAndPassword;
                _password = "";
            } else {
                _userName = userNameAndPassword.substr(0, (unsigned int) passwordPos);
                _password = userNameAndPassword.substr((unsigned int) (passwordPos + 1),
                                                       std::string::npos);
            }

            std::string scale = "1.00";
            int scalePos = _paramUrl.find("scale=");
            if (scalePos != std::string::npos) {
                scale = readValue(_paramUrl, (std::size_t) (scalePos + 6));
            }
            _scale = (float) atof(scale.c_str());

            if (_encrypt) {
                char tmpPwd[23] = {0};
                size_t outlen = 22;
                if (jw_cipher_cloud_pass(_password.c_str(), _password.length(),
                                         tmpPwd, &outlen) == 1) {
                    _password = std::string(tmpPwd);
                }
            }
        }

        int addressAndPortPos = url.find("/", 7);
        if (addressAndPortPos != std::string::npos) {
            int addressBegin = authorizedInfoPos == std::string::npos ? 7 : authorizedInfoPos + 1;
            std::string addressAndPort = url.substr((unsigned int) addressBegin,
                                                    (unsigned int) (addressAndPortPos -
                                                                    addressBegin));
            int portPos = addressAndPort.find(":");
            std::string serverPort = "554";
            if (portPos != std::string::npos) {
                _serverAddress = addressAndPort.substr(0, (unsigned int) portPos);
                serverPort = addressAndPort.substr((unsigned int) (portPos + 1), std::string::npos);
                _serverPort = (unsigned short) atoi(serverPort.c_str());
            } else {
                _serverAddress = addressAndPort;
                _serverPort = 554;  // 没有端口的话，就是用默认的554端口
            }

            // 组成最终的url
            std::string other = url.substr((unsigned int) addressAndPortPos,
                                           std::string::npos);  // 包含了"/"
            _url = "rtsp://";
            _url += _serverAddress;
            _url += ":";
            _url += serverPort;
            _url += other;
            LOGW(TAG, "url:%s\n", _url.c_str());

            int baseUrlPos = _url.find("?");
            if (baseUrlPos != std::string::npos) {
                _baseUrl = _url.substr(0, (unsigned int) baseUrlPos);
                if (baseUrlPos + 1 < _url.size()) {
                    _paramUrl = _url.substr((unsigned int) (baseUrlPos + 1), std::string::npos);
                } else {
                    _paramUrl = "";
                }
            } else {
                _baseUrl = _url;
                _paramUrl = "";
            }
            char baseUrlEndChar = _baseUrl.c_str()[_baseUrl.length() - 1];
            if (baseUrlEndChar != '/') {
                _baseUrl += "/";
            }
            return true;
        }

        return false;
    }

    bool RtspTcpClient::startUp() {
        if (_asioTcpClientPtr.get() == nullptr) {
            return false;
        }

        return _asioTcpClientPtr->startUp();
    }

    void RtspTcpClient::shutdown() {
        if (_asioTcpClientPtr.get() == nullptr) {
            return;
        }

        sendTeardownPacketSync();
        _asioTcpClientPtr->shutDown(false);

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

    void RtspTcpClient::onStatus(AsioTcpClient::Status status) {
        // 未连接变成已连接
        if (_currentStatus != AsioTcpClient::CONNECTED
            && status == AsioTcpClient::CONNECTED) {
            if (_connectStatusListener) {
                _connectStatusListener(true, _pUser);
            }
            if (_cbConnectStatusListener) {
                _cbConnectStatusListener(true, _pUser);
            }
            std::lock_guard<std::mutex> lock(_mutex);

            _tryAuthorized = false;
            updateStatus();  // 更新状态
        } else if (_currentStatus == AsioTcpClient::CONNECTED
                   && status != AsioTcpClient::CONNECTED) {
            // 已连接变成未连接
            if (_connectStatusListener) {
                _connectStatusListener(false, _pUser);
            }
            if (_cbConnectStatusListener) {
                _cbConnectStatusListener(false, _pUser);
            }
        }
        _currentStatus = status;
    }

    void RtspTcpClient::addChar(char tmpChar) {
        _tmpData[0] = _tmpData[1];
        _tmpData[1] = _tmpData[2];
        _tmpData[2] = _tmpData[3];
        _tmpData[3] = tmpChar;
    }

    void RtspTcpClient::resetChar() {
        _tmpData[0] = (char) 0xff;
        _tmpData[1] = (char) 0xff;
        _tmpData[2] = (char) 0xff;
        _tmpData[3] = (char) 0xff;
    }

    void RtspTcpClient::updateStatus() {
        if (_currentMethodStatus == TEARDOWN) {  // 断开状态时发送
            sendOptionsPacket();
        } else if (_currentMethodStatus == OPTIONS) {
            sendDescribePacket();
        } else if (_currentMethodStatus == DESCRIBE) {
            sendSetupVideoPacket();
        } else if (_currentMethodStatus == SETUP_VIDEO) {
            sendSetupAudioPacket();
        } else if (_currentMethodStatus == SETUP_AUDIO) {
            sendPlayPacket();
        }
    }

    void RtspTcpClient::handleRtspPacket(const std::string &rtspPacket) {
        resetChar();

        std::lock_guard<std::mutex> lock(_mutex);

        LOGW(TAG, "handle rtsp packet begin---------------:\n%s\n", rtspPacket.c_str());

        // 查找cseq
        int cseqPos = rtspPacket.find("CSeq:");
        if (cseqPos != std::string::npos) {
            int cseqValuePos = rtspPacket.find("\r\n", (unsigned int) cseqPos);
            if (cseqValuePos != std::string::npos) {
                std::string cseqStr = rtspPacket.substr((unsigned int) (cseqPos + 5),
                                                        (unsigned int) (cseqValuePos - cseqPos -
                                                                        5));

                unsigned cseq = (unsigned int) atoi(cseqStr.c_str());
                if (_cseq == cseq) {
                    // 查找状态返回值
                    int statusPos = rtspPacket.find("RTSP/1.0 ");
                    if (statusPos != std::string::npos) {
                        std::string statusStr = rtspPacket.substr((unsigned int) (statusPos + 9),
                                                                  3);
                        int status = atoi(statusStr.c_str());

                        if (status == OK) {
                            bool needUpdte = true;

                            if (_currentMethodStatus == TEARDOWN) {  // 断开时，发送options返回成功
                                _currentMethodStatus = OPTIONS;
                            } else if (_currentMethodStatus == OPTIONS) {
                                _currentMethodStatus = DESCRIBE;
                            } else if (_currentMethodStatus == DESCRIBE) {
                                _currentMethodStatus = SETUP_VIDEO;
                                // 获取session
                                int sessionPos = rtspPacket.find("Session:");
                                if (sessionPos != std::string::npos) {
                                    int sessionValuePos = rtspPacket.find("\r\n",
                                                                          (unsigned int) (
                                                                                  sessionPos + 8));
                                    int sessionValueSpacePos = rtspPacket.find(";",
                                                                               (unsigned int) (
                                                                                       sessionPos +
                                                                                       8));
                                    if (sessionValuePos != std::string::npos) {
                                        _session = trim(rtspPacket.substr(
                                                (unsigned int) (sessionPos + 8),
                                                (unsigned int) (
                                                        (sessionValueSpacePos < sessionValuePos
                                                         ? sessionValueSpacePos : sessionValuePos) -
                                                        sessionPos - 8)));
                                    }
                                }

                            } else if (_currentMethodStatus == SETUP_VIDEO) {
                                _currentMethodStatus = SETUP_AUDIO;
                                // 获取session
                                int sessionPos = rtspPacket.find("Session:");
                                if (sessionPos != std::string::npos) {
                                    int sessionValuePos = rtspPacket.find("\r\n",
                                                                          (unsigned int) (
                                                                                  sessionPos + 8));
                                    int sessionValueSpacePos = rtspPacket.find(";",
                                                                               (unsigned int) (
                                                                                       sessionPos +
                                                                                       8));
                                    if (sessionValuePos != std::string::npos) {
                                        _session = trim(rtspPacket.substr(
                                                (unsigned int) (sessionPos + 8),
                                                (unsigned int) (
                                                        (sessionValueSpacePos < sessionValuePos
                                                         ? sessionValueSpacePos : sessionValuePos) -
                                                        sessionPos - 8)));
                                    }
                                }

                            } else if (_currentMethodStatus == SETUP_AUDIO) {
                                _currentMethodStatus = PLAY;
                            } else if (_currentMethodStatus == WAIT_PLAY) {
                                _currentMethodStatus = PLAY;
                                needUpdte = false;
                            } else if (_currentMethodStatus == WAIT_PAUSE) {
                                _currentMethodStatus = PAUSE;
                                needUpdte = false;
                            }

                            if (needUpdte) {
                                updateStatus();
                            }
                        } else if (status == Unauthorized && _currentMethodStatus == OPTIONS) {
                            if (_tryAuthorized) {  // 如果已经尝试过鉴权，那么就是用户名密码错误
                                _currentMethodStatus = TEARDOWN;
                                LOGE(TAG, "username or password error!\n");
                            } else {
                                // 获取鉴权信息
                                int realmPos = rtspPacket.find("realm=\"");
                                if (realmPos != std::string::npos) {
                                    int realmValuePos = rtspPacket.find("\"",
                                                                        (unsigned int) (realmPos +
                                                                                        7));
                                    if (realmValuePos != std::string::npos) {
                                        _realm = rtspPacket.substr((unsigned int) (realmPos + 7),
                                                                   (unsigned int) (realmValuePos -
                                                                                   realmPos - 7));
                                    }
                                }

                                int noncePos = rtspPacket.find("nonce=\"");
                                if (noncePos != std::string::npos) {
                                    int nonceValuePos = rtspPacket.find("\"",
                                                                        (unsigned int) (noncePos +
                                                                                        7));
                                    if (nonceValuePos != std::string::npos) {
                                        _nonce = rtspPacket.substr((unsigned int) (noncePos + 7),
                                                                   (unsigned int) (nonceValuePos -
                                                                                   noncePos - 7));
                                    }
                                }

                                _needAuthorized = true;
                                updateStatus();
                            }

                        }
                    } else {
                        LOGW(TAG, "can not find status\n");
                    }

                } else {
                    LOGE(TAG, "receive is cseq:%d, but send cseq is:%d\n", cseq, _cseq);
                }
            }
        }

        LOGW(TAG, "handle rtsp packet end------------------\n\n\n");
    }

    void RtspTcpClient::receiveData(const char *pBuf, std::size_t dataLen) {
        // 接收到数据，这边需要解析数据，大致的数据分为rtsp返回包，rtcp包，rtp包几类
        //                  "   \r   \n   \r   \n" "Magic:1 Channel:1 Length:2"
        // 循环检测4个连续字节 "0x0d 0x0a 0x0d 0x0a"  "0x24 0x?? 0x?? 0x??"
        int begin = 0;  // 未处理的数据起始位置

//        LOGW(TAG, "dataLen=%d _dataPacketLen=%d\n", dataLen, _dataPacketLen);

        if (_dataPacketLen > 0) {  // 如果上次解析出数据包，并且包还没有完整的话
            // 判断后面的数据长度能不能达到包长
            if (dataLen + _receiveLen < _dataPacketLen) {  // 剩余长度小于数据包长的话，就加入缓存中
                if (dataLen + _receiveLen > RECEIVE_SIZE) {  // 如果剩余长度大于接收缓存大小，那么可能是解析不对，那么不拷贝到缓存中
                    LOGE(TAG, "data size longer than receive max size!\n");
                    _receiveLen = 0;
                    _dataPacketLen = 0;
                    return;
                }
                memcpy(_receiveData + _receiveLen, pBuf + begin, dataLen);
                _receiveLen += dataLen;
                return;
            } else {  // 能达到包长的话，就直接回调上层
                char dataPacket[_dataPacketLen];
                memcpy(dataPacket, _receiveData, _receiveLen);
                std::size_t len = _dataPacketLen - _receiveLen;
                memcpy(dataPacket + _receiveLen, pBuf, len);

                handleDataPacket(dataPacket, _dataPacketLen);

                _receiveLen = 0;
                _dataPacketLen = 0;
                begin += len;
            }
        }

//        LOGW(TAG, "into for\n");

        for (int i = begin; i < dataLen; ++i) {
            addChar(*(pBuf + i));
            if (_tmpData[0] == '\r' && _tmpData[1] == '\n' && _tmpData[2] == '\r' &&
                _tmpData[3] == '\n') {  // rtsp的结束符，那么前面的数据都可以看做是rtsp数据
                // 将当前i位置和前面的缓存数据加在一起，组成rtsp数据
                std::size_t packet = _receiveLen + i - begin + 1;
                char rtspPacket[packet];
                if (_receiveLen > 0) {
                    memcpy(rtspPacket, _receiveData, _receiveLen);
                }
                memcpy(rtspPacket + _receiveLen, pBuf + begin, (std::size_t) (i - begin + 1));

                // 查询是否有content len


                // 分析rtsp包
                handleRtspPacket(rtspPacket);

                // 记录最新的位置
                begin = i + 1;

                // 重置缓存
                _receiveLen = 0;
            } else if (_tmpData[0] == '$') {  // 后面跟着的数据包
                // 计算出后面数据的长度
                _channel = _tmpData[1] & 0x00ff;

                _dataPacketLen = (std::size_t)((((_tmpData[2] & 0x00ff) << 8) & 0x00ff00) |
                                          (_tmpData[3] & 0x00ff));

//                LOGE(TAG, "_dataPacketLen=%d, i=%d, channel=%d\n", _dataPacketLen, i, _channel);
                // 假如这个时候临时数据还有值，那么也清空掉
                _receiveLen = 0;
                begin = i + 1;  // 前面的数据就全部丢掉

                // 判断后面的数据长度能不能达到包长
                std::size_t remainLen = dataLen - begin;
                if (remainLen <= 0) {  // 如果没有剩余了，那么就直接等待接受下一次数据
//                    LOGE(TAG, "a\n");
                    return;
                }

                if (remainLen < _dataPacketLen) {  // 剩余长度小于数据包长的话，就加入缓存中
                    if (remainLen > RECEIVE_SIZE) {  // 如果剩余长度大于接收缓存大小，那么可能是解析不对，那么不拷贝到缓存中
                        LOGE(TAG, "remain size longer than receive max size!\n");
                        _dataPacketLen = 0;
                        return;
                    }
                    memcpy(_receiveData, pBuf + begin, remainLen);
                    _receiveLen = remainLen;
//                    LOGE(TAG, "b\n");
                    return;
                } else {  // 能达到包长的话，就直接回调上层
                    char dataPacket[_dataPacketLen];
                    memcpy(dataPacket, pBuf + begin, _dataPacketLen);

                    handleDataPacket(dataPacket, _dataPacketLen);

                    begin += _dataPacketLen;
                    i += _dataPacketLen;
                    _dataPacketLen = 0;
                }
            }
        }

//        LOGW(TAG, "out for\n");

        if (begin < dataLen) {  // 还有未处理的数据，加入到缓存中
            if (dataLen - begin > RECEIVE_SIZE) {  // 如果这次数据就已经超出缓存最大大小，那么就不做处理了
                LOGE(TAG, "receive data size beyond receive buffer max size! do nothing.\n");
                return;
            }

            if (_receiveLen + dataLen - begin > RECEIVE_SIZE) {  // 如果超出缓存大小，那么就清空掉前面的缓存
                _receiveLen = 0;
                LOGE(TAG, "receive buffer size beyond max size! give up data.\n");
            }
            memcpy(_receiveData + _receiveLen, pBuf + begin, dataLen - begin);
            _receiveLen += dataLen - begin;
        }

//        LOGW(TAG, "end\n");
    }

    void RtspTcpClient::handleDataPacket(const char *pData, std::size_t dataLen) {
        resetChar();
//        LOGW(TAG, "handleDataPacket datalen:%d channel:%d\n", dataLen, _channel);
        // 回调给上层数据包
        if (_channel == 0 || _channel == 2) {  // 0：视频数据包 1：数据rtcp包 2：音频数据包 3：音频rtcp包
            if (_dataPacketCallback) {
                _dataPacketCallback(pData, dataLen, _pUser);
            }
            if (_cbDataPacketCallback) {
                _cbDataPacketCallback(pData, dataLen, _pUser);
            }
        }
//        else {
//            LOGW(TAG, "Unsupport channel[%d]\n", _channel);
//        }
    }

    std::string RtspTcpClient::createCommonBuffer(const std::string &method, const std::string &uri,
                                                  bool baseUrl) {
        std::string tmpBuffer;
        tmpBuffer += uri;
        tmpBuffer += " RTSP/1.0\r\n";

        ++_cseq;
        char cseq[256];
        sprintf(cseq, "CSeq: %u\r\n", _cseq);
        tmpBuffer += cseq;

        if (_needAuthorized && _userName.length() > 0) {
            char HRes[32 + 1];

            std::string tmpUrl = _url;
            if (baseUrl) {
                tmpUrl = _baseUrl;
            }

//            LOGE(TAG, "method[%s], username[%s], password[%s], realm[%s], nonce[%s], tmpUrl[%s]\n", method.c_str(), _userName.c_str(), _password.c_str(), _realm.c_str(), _nonce.c_str(), tmpUrl.c_str());
            ComputeResponse(HRes, method.c_str(), _userName.c_str(), _password.c_str(),
                            _realm.c_str(),
                            _nonce.c_str(), tmpUrl.c_str());
            char sAuthor[300];
            sprintf(sAuthor,
                    "Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\n",
                    _userName.c_str(), _realm.c_str(), _nonce.c_str(), tmpUrl.c_str(), HRes);
            tmpBuffer += sAuthor;

            _tryAuthorized = true;
        }

        if (_encrypt) {
            tmpBuffer += "User-Agent: joyware-cloud-client/1.0.0\r\n";
        } else {
            tmpBuffer += "User-Agent: LibVLC/2.2.4 (LIVE555 Streaming Media v2016.02.22)\r\n";
        }

        if (_session.size() > 0) {
            tmpBuffer += "Session: ";
            tmpBuffer += _session;
            tmpBuffer += "\r\n";
        }

        return tmpBuffer;
    }

    bool RtspTcpClient::sendRtspRequest(const char *rtspRequest, size_t len) {
        LOGW(TAG, "send rtsp request:\n%s\n", rtspRequest);
        return _asioTcpClientPtr->send(std::make_shared<PacketData>(len, rtspRequest));
    }

    bool RtspTcpClient::sendOptionsPacket() {
        if (_asioTcpClientPtr.get() == nullptr) {
            return false;
        }

        std::string sBuffer = "OPTIONS ";
        sBuffer += createCommonBuffer("OPTIONS", _url, false);
        sBuffer += "\r\n";
        return sendRtspRequest(sBuffer.c_str(), sBuffer.length());
    }

    bool RtspTcpClient::sendDescribePacket() {
        if (_asioTcpClientPtr.get() == nullptr) {
            return false;
        }

        std::string sBuffer = "DESCRIBE ";
        sBuffer += createCommonBuffer("DESCRIBE", _url, false);
        sBuffer += "Accept: application/sdp\r\n";
        sBuffer += "\r\n";
        return sendRtspRequest(sBuffer.c_str(), sBuffer.length());
    }

    bool RtspTcpClient::sendSetupVideoPacket() {
        if (_asioTcpClientPtr.get() == nullptr) {
            return false;
        }

        std::string sBuffer = "SETUP ";
        sBuffer += createCommonBuffer("SETUP", _baseUrl + "trackID=1", true);
        sBuffer += "Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n";
        sBuffer += "\r\n";
        return sendRtspRequest(sBuffer.c_str(), sBuffer.length());
    }

    bool RtspTcpClient::sendSetupAudioPacket() {
        if (_asioTcpClientPtr.get() == nullptr) {
            return false;
        }

        std::string sBuffer = "SETUP ";
        sBuffer += createCommonBuffer("SETUP", _baseUrl + "trackID=2", true);
        sBuffer += "Transport: RTP/AVP/TCP;unicast;interleaved=2-3\r\n";
        sBuffer += "\r\n";
        return sendRtspRequest(sBuffer.c_str(), sBuffer.length());
    }

    bool RtspTcpClient::sendPlayPacket() {
        if (_asioTcpClientPtr.get() == nullptr) {
            return false;
        }

        std::string sBuffer = "PLAY ";
        sBuffer += createCommonBuffer("PLAY", _url, true);
        // 查找starttime和endtime

        int startTimePos = _paramUrl.find("starttime=");
        int endTimePos = _paramUrl.find("endtime=");
        // 都有的话就是录像回放
        if (startTimePos != std::string::npos && endTimePos != std::string::npos) {
            if (_currentMethodStatus != PAUSE &&
                _currentMethodStatus != WAIT_PLAY) {  // 如果不是暂停状态, 需要解析starttime和endtime
                std::string startTime = readValue(_paramUrl, (std::size_t) (startTimePos + 10));
                std::string endTime = readValue(_paramUrl, (std::size_t) (endTimePos + 8));
                sBuffer += "Range:clock=";
                sBuffer += startTime;
                sBuffer += "-";
                sBuffer += endTime;
                sBuffer += "\r\n";
            }

            std::string scaleStr = "1.00";

            char scaleCh[20] = {0};
            sprintf(scaleCh, "%.3f", _scale);
            scaleStr = scaleCh;

            sBuffer += "Rate-Control:yes\r\n";
            sBuffer += "Scale:";
            sBuffer += scaleStr;
            sBuffer += "\r\n";
        } else {
            sBuffer += "Range: npt=0.000-\r\n";
        }

        sBuffer += "\r\n";
        return sendRtspRequest(sBuffer.c_str(), sBuffer.length());
    }

    bool RtspTcpClient::sendPausePacket() {
        if (_asioTcpClientPtr.get() == nullptr) {
            return false;
        }

        std::string sBuffer = "PAUSE ";
        sBuffer += createCommonBuffer("PAUSE", _url, false);
        sBuffer += "\r\n";
        return sendRtspRequest(sBuffer.c_str(), sBuffer.length());
    }

    bool RtspTcpClient::sendTeardownPacket() {
        if (_asioTcpClientPtr.get() == nullptr) {
            return false;
        }

        std::string sBuffer = "TEARDOWN ";
        sBuffer += createCommonBuffer("TEARDOWN", _url, true);
        sBuffer += "\r\n";
        return sendRtspRequest(sBuffer.c_str(), sBuffer.length());
    }

    bool RtspTcpClient::sendTeardownPacketSync() {
        if (_asioTcpClientPtr.get() == nullptr) {
            return false;
        }

        std::string sBuffer = "TEARDOWN ";
        sBuffer += createCommonBuffer("TEARDOWN", _url, true);
        sBuffer += "\r\n";

        LOGW(TAG, "send rtsp request:\n%s\n", sBuffer.c_str());
        return _asioTcpClientPtr->sendSync(
                std::make_shared<PacketData>(sBuffer.length(), sBuffer.c_str()));
    }

    /**
     * 去掉两侧空格
     * @param srcStr
     * @return
     **/
    std::string RtspTcpClient::trim(const std::string &srcStr) {
        std::string tmpStr = srcStr;
        int spacePos = 0;
        while (tmpStr.length() > 1 && (spacePos = tmpStr.find(" ")) != std::string::npos) {
            if (spacePos == 0) {  // 如果在头部
                tmpStr = tmpStr.substr((unsigned int) (spacePos + 1), std::string::npos);
            } else if (spacePos == tmpStr.length() - 1) {  // 尾部
                tmpStr = tmpStr.substr(0, tmpStr.length() - 1);
            } else {
                break;
            }
        }
        return tmpStr;
    }

    std::string RtspTcpClient::readValue(const std::string &str, std::size_t pos) {
        std::size_t strLen = str.length();
        std::string value = "";
        const char *tmpStr = str.c_str();
        for (std::size_t i = pos; i < strLen; ++i) {
            char tmpChar = tmpStr[i];
            if (tmpChar == ' ' || tmpChar == '&' || tmpChar == '\n' || tmpChar == '\r' ||
                tmpChar == ',' || tmpChar == ';') {
                break;
            } else {
                value += tmpChar;
            }
        }
        return value;
    }
}
