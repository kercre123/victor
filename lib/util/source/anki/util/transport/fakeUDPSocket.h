/**
 * File: FakeUDPSocket
 *
 * Author: Mark Wesley
 * Created: 02/09/15
 *
 * Description: Fake UDP Socket implementation
 *              Fulfills same interface for a Posix/BSD style socket implementation, but is faked internally
 *              to allow for easier testing of different network situations.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __NetworkService_FakeUDPSocket_H__
#define __NetworkService_FakeUDPSocket_H__


#include "util/transport/iUDPSocket.h"
#include "util/transport/packetByteArray.h"
#include <mutex>
#include <vector>


namespace Anki {
namespace Util {


// ============================== FakeUDPPacketData ==============================


class FakeUDPPacketData
{
public:
  
  FakeUDPPacketData();
  FakeUDPPacketData(const void* inBuffer, size_t inSize,
                    const sockaddr* srcSockAddress,  socklen_t srcSockAddressLength,
                    const sockaddr* destSockAddress, socklen_t destSockAddressLength);
  ~FakeUDPPacketData();
  
  FakeUDPPacketData(FakeUDPPacketData&& rhs) noexcept = default;
  FakeUDPPacketData& operator=(FakeUDPPacketData&& rhs) noexcept = default;
  FakeUDPPacketData(const FakeUDPPacketData& rhs) = default;
  FakeUDPPacketData& operator=(const FakeUDPPacketData& rhs) = default;

  void Set(const void* inBuffer, size_t inSize,
           const sockaddr* srcSockAddress,  socklen_t srcSockAddressLength,
           const sockaddr* destSockAddress, socklen_t destSockAddressLength);
  
  PacketByteArray&        GetMessage()       { return _message; }
  const PacketByteArray&  GetMessage() const { return _message; }
  
  PacketByteArray&        GetSocketAddressSrc()       { return _socketAddressSrc; }
  const PacketByteArray&  GetSocketAddressSrc() const { return _socketAddressSrc; }

  PacketByteArray&        GetSocketAddressDest()       { return _socketAddressDest; }
  const PacketByteArray&  GetSocketAddressDest() const { return _socketAddressDest; }

private:
  
  PacketByteArray   _message;
  PacketByteArray   _socketAddressSrc;
  PacketByteArray   _socketAddressDest;
};
  
  
// ============================== FakeUDPSocketInfo ==============================

  
class FakeUDPSocketInfo
{
public:
  
  FakeUDPSocketInfo();
  ~FakeUDPSocketInfo();
  
  void ClearQueues();
  
  int Open(int socketId, int protocolFamily, int socketType, int protocol);
  int Close();
  int Bind(const sockaddr* socketAddress, socklen_t socketAddressLength);
  
  int GetSocketOpt(int level, int optname,       void* optVal, socklen_t* optlen);
  int SetSocketOpt(int level, int optname, const void* optVal, socklen_t  optlen);
  
  ssize_t SendTo(const void* messageData, size_t messageDataSize, int flags,
                 const sockaddr* srcSockAddress, socklen_t srcSockAddressLength,
                 const sockaddr* destSockAddress, socklen_t destSockAddressLength);
  ssize_t ReceiveMessage(msghdr* messageHeader, int flags);
  
  void  PipeFrom(FakeUDPSocketInfo& inSocket, uint32_t localIpAddress);
  
  uint16_t  GetPort() const { return _port; }
  bool      IsOpen()  const { return _isOpen; }

private:
  
  typedef std::vector<FakeUDPPacketData> MessageQueue;

  MessageQueue  _outMessageQueue;
  MessageQueue  _inMessageQueue;
  
  int  _socketId;
  int  _protocolFamily;
  int  _socketType;
  int  _protocol;
  
  uint32_t _inAddress;  // in network order (i.e. same as socket.sin_addr.s_addr)
  uint16_t _port;       // in network order (i.e. same as socket.sin_port)
  
  bool _isOpen;
  bool _isBound;
};

  
// ============================== FakeUDPSocket ==============================


class FakeUDPSocket : public IUDPSocket
{
public:
  
  // IUDPSocket Interface
  
  static const uint32_t k_DefaultLocalIpAddress = (192) | (168 << 8) | (0 << 16) | (1 << 24);
  
  explicit FakeUDPSocket(uint32_t ipAddress = k_DefaultLocalIpAddress);
  virtual ~FakeUDPSocket();
  
  virtual int OpenSocket(int protocolFamily, int socketType, int protocol) override;
  virtual int BindSocket(int socketId, const sockaddr* socketAddress, socklen_t socketAddressLength) override;
  virtual int GetSocketOpt(int socketId, int level, int optname, void* optVal, socklen_t* optlen) override;
  virtual int SetSocketOpt(int socketId, int level, int optname, const void* optVal, socklen_t optlen) override;
  virtual int CloseSocket(int socketId) override;
  virtual ssize_t SendTo(int socketId, const void* messageData, size_t messageDataSize, int flags, const sockaddr* destSockAddress, socklen_t destSockAddressLength) override;
  virtual ssize_t ReceiveMessage(int socketId, msghdr* messageHeader, int flags) override;
  virtual uint32_t GetLocalIpAddress() override;
  virtual struct in6_addr GetLocalIpv6LinkLocalAddress() override;;
  
  // FakeUDPSocket implementation specific
  void  PipeFrom(FakeUDPSocket* outSocket);
  
  std::mutex& GetMutex() { return _mutex; }
  
private:
  
  static const int k_IndexToSocketIdOffset = 1;
  static const int k_MaxSockets = 4;
  
  FakeUDPSocketInfo* GetSocketByIndex(int index)
  {
    return ((index >= 0) && (index < k_MaxSockets)) ? &_socketInfo[index] : nullptr;
  }

  int SocketIdToIndex(int socketId) const
  {
    const int index = (socketId - k_IndexToSocketIdOffset);
    return index;
  }
  
  int IndexToSocketId(int index) const
  {
    const int socketId = (index + k_IndexToSocketIdOffset);
    return socketId;
  }
  
  FakeUDPSocketInfo _socketInfo[k_MaxSockets];
  uint32_t          _localIpAddress;
  std::mutex        _mutex;
};


} // end namespace Util
} // end namespace Anki

#endif // __NetworkService_FakeUDPSocket_H__
