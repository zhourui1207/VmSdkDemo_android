/*
 * AsioUdpServer.h
 *
 *  Created on: 2016年9月27日
 *      Author: zhourui
 */

#ifndef ASIOUDPSERVER_H_
#define ASIOUDPSERVER_H_

namespace Dream {

    class AsioUdpServer {
    public:
        AsioUdpServer(const std::string address, );

        virtual ~AsioUdpServer();
    };

} /* namespace Dream */

#endif /* ASIOUDPSERVER_H_ */
