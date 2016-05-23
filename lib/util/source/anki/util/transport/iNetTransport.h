/**
 * File: iNetTransport
 *
 * Author: baustin
 * Created: 1/20/15
 *
 * Description: Interface for network communications to send and receive data
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __NetworkService_INetTransport_H__
#define __NetworkService_INetTransport_H__

#include <cstdint>
#include <vector>

namespace Anki {
namespace Util {

class INetTransportDataReceiver;
class TransportAddress;

class INetTransport {

public:
  using AdvertisementBytes = std::vector<uint8_t>;

  INetTransport(INetTransportDataReceiver* dataReceiver) : _dataReceiver(dataReceiver) {}
  virtual ~INetTransport() {}

  virtual void SendData(bool reliable, const TransportAddress& destAddress, const uint8_t* buffer, unsigned int size, bool flushPacket=false) = 0;
  virtual void StartHost() = 0;
  virtual void StopHost() = 0;
  virtual void StartClient() = 0;
  virtual void StopClient() = 0;
  virtual void Connect(const TransportAddress& destAddress) = 0;
  virtual void FinishConnection(const TransportAddress& destAddress) = 0; // confirm a remote connection request
  virtual void Disconnect(const TransportAddress& destAddress) = 0;
  virtual void FillAdvertisementBytes(AdvertisementBytes& bytes) = 0;
  virtual unsigned int FillAddressFromAdvertisement(TransportAddress& address, const uint8_t* buffer, unsigned int size) = 0;

protected:
  INetTransportDataReceiver* _dataReceiver;

};

} // end namespace Util
} // end namespace Anki


#endif // __NetworkService_INetTransport_H__
