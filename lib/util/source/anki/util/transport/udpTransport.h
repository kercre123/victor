/**
 * File: udpTransport
 *
 * Author: Mark Wesley
 * Created: 1/28/15
 *
 * Description: Describes an interface to UDP hardware to send and receive data
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __NetworkService_UDPTransport_H__
#define __NetworkService_UDPTransport_H__

#include "util/transport/iUnreliableTransport.h"
#include "util/transport/netTimeStamp.h"
#include "util/transport/transportStats.h"
#include <assert.h>
#include <sys/socket.h>

struct sockaddr;
struct sockaddr_in;
struct sockaddr_in6;


namespace Anki {
namespace Util {

class IIpRetriever;
class IUDPSocket;

class UDPTransport : public IUnreliableTransport {

public:
  static constexpr const char* const kDefaultIPv6MulticastAddress = "ff02::1";
  // only necessary to pass in IP retriever running on Android
  UDPTransport()
  : UDPTransport(AF_INET) { }
  UDPTransport(sa_family_t family);
  virtual ~UDPTransport();

  // from IUnreliableTransport
  virtual void SendData(const TransportAddress& destAddress, const SrcBufferSet& srcBuffers) override;
  virtual void StartHost() override;
  virtual void StopHost() override;
  virtual void StartClient() override;
  virtual void StopClient() override;
  virtual void FillAdvertisementBytes(AdvertisementBytes& bytes) override;
  virtual unsigned int FillAddressFromAdvertisement(TransportAddress& address, const uint8_t* buffer, unsigned int size) override;
  virtual uint32_t MaxTotalBytesPerMessage() const override;
  virtual void Update() override;
  virtual void Print() const override;

  void ResetSocket();

  void    SetSocketImpl(IUDPSocket* udpSocketImpl, bool ownsSocketImpl = false);

  bool    IsConnected() const { return (_socketId >= 0); }
  int     GetSocketId() const { return _socketId; }
  int     GetPort() const { return _port; }
  void    SetPort(int inPort) { assert(!IsConnected()); _port = inPort; }

  static uint32_t GetLocalIpAddress();
  static uint32_t GetBroadcastAddressFromIfAddr();

  const TransportStats& GetTransportStats() const { return _transportStats; }

  static void SetIpRetriever(IIpRetriever* ipRetriever);

  // ========== UDP Header settings - allows control per-app of things like prefix, whether to add a CRC, etc. ==========

  class HeaderPrefix
  {
  public:

    static const uint32_t   kMaxLength = 4;

    HeaderPrefix(const uint8_t* prefixBytes, uint32_t numBytes);

    void Set(const uint8_t* prefixBytes, uint32_t numBytes);

    const uint8_t*  GetBytes()  const { return _bytes;  }
    uint32_t        GetLength() const { return _length; }

  private:

    uint8_t               _bytes[kMaxLength];
    uint32_t              _length;
  };

  static void                SetHeaderPrefix(const uint8_t* prefixBytes, uint32_t numBytes);
  static const HeaderPrefix& GetHeaderPrefix();

  static void                SetDoesHeaderHaveCRC(bool hasCRC);
  static bool                DoesHeaderHaveCRC();

  static uint32_t            GetHeaderSize();

  static size_t GetMaxNetMessageSize();
  static void   SetMaxNetMessageSize(const size_t newMTU);

private:

  void    FreeSocketImpl();
  void    UpdateSocketImplForNetEmulation();
  
  ssize_t SendDataToSockAddress(const sockaddr_in& destSockAddress, uint32_t destSockAddressLength, const SrcBufferSet& srcBuffers)
  {
    return SendDataToSockAddress(reinterpret_cast<const sockaddr&>(destSockAddress), destSockAddressLength, srcBuffers);
  }
  ssize_t SendDataToSockAddress(const sockaddr_in6& destSockAddress, uint32_t destSockAddressLength, const SrcBufferSet& srcBuffers)
  {
    return SendDataToSockAddress(reinterpret_cast<const sockaddr&>(destSockAddress), destSockAddressLength, srcBuffers);
  }
  ssize_t SendDataToSockAddress(const sockaddr& destSockAddress, uint32_t destSockAddressLength, const SrcBufferSet& srcBuffers);

  bool    OpenSocket(int port);
  bool    CloseSocket();

  void    HandleReceivedMessage(const uint8_t* buffer, uint32_t bufferLength, const TransportAddress& sourceAddress, bool wasTruncated);
  bool    TryToReadMessage();

  unsigned int GetWifiInterfaceIndex() const;

  // ============================== Static Member Vars ==============================

  static HeaderPrefix sHeaderPrefix;
  static bool         sDoesHeaderHaveCRC;
  static size_t       sMaxNetMessageSize;

  // ============================== Member Vars ==============================

  TransportStats  _transportStats;
  IUDPSocket*     _udpSocketImpl;
  NetTimeStamp    _lastSendErrorPrinted;

  const sa_family_t _family;
  int             _socketId;
  int             _port;
  bool            _ownsSocketImpl;
  bool            _reset;
};

} // end namespace Util
} // end namespace Anki

#endif // __NetworkService_UDPTransport_H__
