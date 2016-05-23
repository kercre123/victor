/**
 * File: iNetTransportDataReceiver
 *
 * Author: baustin
 * Created: 1/20/15
 *
 * Description: Interface for callbacks that will be invoked when network messages are received
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __NetworkService_INetTransportDataReceiver_H__
#define __NetworkService_INetTransportDataReceiver_H__

#include <cstdint>

namespace Anki {
namespace Util {

class TransportAddress;

class INetTransportDataReceiver {

public:
  INetTransportDataReceiver() {}
  virtual ~INetTransportDataReceiver() {}

  virtual void ReceiveData(const uint8_t* buffer, unsigned int size, const TransportAddress& sourceAddress) = 0;

  // event markers
  static const uint8_t* const OnConnected;
  static const uint8_t* const OnConnectRequest;
  static const uint8_t* const OnDisconnected;
};

} // end namespace Util
} // end namespace Anki


#endif // __NetworkService_INetTransport_H__
