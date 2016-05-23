/**
 * File: iUnreliableTransport
 *
 * Author: baustin
 * Created: 1/21/15
 *
 * Description: Describes an interface to a network protocol that doesn't guarantee reliable data transmission
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __NetworkService_IUnreliableTransport_H__
#define __NetworkService_IUnreliableTransport_H__

#include "util/transport/iNetTransport.h"

namespace Anki {
namespace Util {

class INetTransportDataReceiver;
class SrcBufferSet;

class IUnreliableTransport {

public:
  using AdvertisementBytes = INetTransport::AdvertisementBytes;

  IUnreliableTransport() : _dataReceiver(nullptr) {}
  virtual ~IUnreliableTransport() {}
  virtual void SendData(const TransportAddress& destAddress, const SrcBufferSet& srcBuffers) = 0;
  virtual void StartHost() = 0;
  virtual void StopHost() = 0;
  virtual void StartClient() = 0;
  virtual void StopClient() = 0;
  virtual void FillAdvertisementBytes(AdvertisementBytes& bytes) = 0;
  virtual unsigned int FillAddressFromAdvertisement(TransportAddress& address, const uint8_t* buffer, unsigned int size) = 0;
  virtual uint32_t MaxTotalBytesPerMessage() const = 0;
  virtual void Update() = 0;
  virtual void Print() const = 0;

  void SetDataReceiver(INetTransportDataReceiver* dataReceiver) { _dataReceiver = dataReceiver; }

protected:
  INetTransportDataReceiver* _dataReceiver;
};

} // end namespace Util
} // end namespace Anki


#endif // __NetworkService_IUnreliableTransport_H__
