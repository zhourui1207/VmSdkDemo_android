/*
 * AsioUdpClient.h
 *
 *  Created on: 2016年11月30日
 *      Author: zhourui
 *      可以内网向外网通信
 */

#ifndef ASIOUDPCLIENT_H_
#define ASIOUDPCLIENT_H_

#include <boost/asio.hpp>
#include <memory>

namespace Dream {

class AsioUdpClient {
public:
  AsioUdpClient();
  virtual ~AsioUdpClient();

//  bool send(const std::shared_ptr<PacketData>& dataPtr);

private:
  std::string _address;
  unsigned _port;

  std::unique_ptr<boost::asio::io_service> _ioServicePtr;
  std::unique_ptr<boost::asio::ip::udp::socket> _socketPtr;  // 套接字智能指针
  boost::asio::ip::udp::resolver::iterator _endpointIterator;
};

} /* namespace Dream */

#endif /* ASIOUDPCLIENT_H_ */
