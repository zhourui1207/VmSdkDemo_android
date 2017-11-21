/*
 * StreamSessionManager.h
 *
 *  Created on: 2016年10月19日
 *      Author: zhourui
 *      码流session管理类
 */

#ifndef STREAMSESSIONMANAGER_H_
#define STREAMSESSIONMANAGER_H_

#include <map>
#include <mutex>
#include <set>

#include "../stream/StreamClient.h"
#include "VmType.h"
#include "../rtsp/RtspTcpClient.h"

namespace Dream {

    class StreamSession {
    public:
        StreamSession() = delete;

        StreamSession(unsigned streamId, const std::string &addr, unsigned short port,
                      fStreamCallBack streamCallback,
                      fStreamConnectStatusCallBack streamConnectStatusCallback, void *user,
                      const std::string &monitorId, const std::string &deviceId,
                      int playType, int clientType, bool rtp = true);

        StreamSession(unsigned streamId, const std::string &addr, unsigned short port,
                      fStreamCallBackExt streamCallback,
                      fStreamConnectStatusCallBack streamConnectStatusCallback, void *user,
                      const std::string &monitorId, const std::string &deviceId,
                      int playType, int clientType, bool rtp = true);

        virtual ~StreamSession();

        bool startUp();

        void shutDown();

    private:
        // 码流回调
        void onStream(const std::shared_ptr<StreamData> &streamDataPtr);

        // 码流连接状态回调
        void onConnectStatus(bool isConnected);

    private:
        unsigned _streamId;
        StreamClient _streamClient;  // 码流客户端
        fStreamCallBack _streamCallback;  // 码流回调
        fStreamCallBackExt _streamCallbackExt;
        fStreamConnectStatusCallBack _streamConnectStatusCallback;  // 码流连接状态回调
        void *_user;  // 用户传进来的指针，无需释放内存
        bool _rtp;
        std::mutex _mutex;
    };

    class StreamSessionManager {
        using SessionMap = std::map<unsigned, std::shared_ptr<StreamSession>>;
        using RTSPSessionMap = std::map<unsigned, std::shared_ptr<RtspTcpClient>>;

    public:
        StreamSessionManager();

        virtual ~StreamSessionManager();

        // 增加码流客户端
        bool addStreamClient(const std::string &addr, unsigned short port,
                             fStreamCallBack streamCallback,
                             fStreamConnectStatusCallBack streamConnectStatusCallback, void *pUser,
                             unsigned &streamId, const std::string &monitorId, const std::string &deviceId,
                             int playType, int clientType, bool rtp = true);

        bool addStreamClient(const std::string &addr, unsigned short port,
                             fStreamCallBackExt streamCallback,
                             fStreamConnectStatusCallBack streamConnectStatusCallback, void *pUser,
                             unsigned &streamId, const std::string &monitorId, const std::string &deviceId,
                             int playType, int clientType, bool rtp = true);

        // 移除码流客户端
        void removeStreamClient(unsigned streamId);

        bool addRTSPStreamClient(const std::string &url, fStreamCallBackV2 streamCallback,
                                 fStreamConnectStatusCallBackV2 streamConnectStatusCallback,
                                 void *pUser, unsigned &streamId, bool encrypt = false);

        void removeRTSPStreamClient(unsigned streamId);

        bool pauseRTSPStream(unsigned streamId);

        bool playRTSPStream(unsigned streamId, float scale = -1);

        // 清除所有码流连接
        void clearAll();

        bool streamIsValid(unsigned int streamId);

    private:
        // 获取码流id
        bool getStreamId(unsigned &streamId);

        // 回收码流id
        void recoveryStreamId(int streamId);

    private:
        SessionMap _sessionMap;
        RTSPSessionMap _rtspSessionMap;
        unsigned _streamId;
        std::set<unsigned> _recoveryStreamIds;  // 这个用来保存回收的id
        std::mutex _mutex;
    };

} /* namespace Dream */

#endif /* STREAMSESSION_H_ */
