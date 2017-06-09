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

namespace Dream {

    class StreamSession {
    public:
        StreamSession() = delete;

        StreamSession(unsigned streamId, const std::string &addr, unsigned port,
                      fStreamCallBack streamCallback,
                      fStreamConnectStatusCallBack streamConnectStatusCallback, void *user);

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
        fStreamConnectStatusCallBack _streamConnectStatusCallback;  // 码流连接状态回调
        void *_user;  // 用户传进来的指针，无需释放内存
        std::mutex _mutex;
    };

    class StreamSessionManager {
        using SessionMap = std::map<unsigned, std::shared_ptr<StreamSession>>;

    public:
        StreamSessionManager();

        virtual ~StreamSessionManager();

        // 增加码流客户端
        bool addStreamClient(const std::string &addr, unsigned port,
                             fStreamCallBack streamCallback,
                             fStreamConnectStatusCallBack streamConnectStatusCallback, void *pUser,
                             unsigned &streamId);

        // 移除码流客户端
        void removeStreamClient(unsigned streamId);

        // 清除所有码流连接
        void clearAll();

    private:
        // 获取码流id
        bool getStreamId(unsigned &streamId);

        // 回收码流id
        void recoveryStreamId(int streamId);

    private:
        SessionMap _sessionMap;
        unsigned _streamId;
        std::set<unsigned> _recoveryStreamIds;  // 这个用来保存回收的id
        std::mutex _mutex;
    };

} /* namespace Dream */

#endif /* STREAMSESSION_H_ */
