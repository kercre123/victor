/**
 * File: NetEmulatorUDPSocket
 *
 * Author: Mark Wesley
 * Created: 02/15/15
 *
 * Description: Network Emulator UDP Socket, for testing and emulation/simulation of different conditions
 *              Pipes everything to another IUDPSocket interface (real or otherwise), and is only responsible
 *              for modifying delivery time (including re-ordering) and random packet loss
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __NetworkService_NetEmulatorUDPSocket_H__
#define __NetworkService_NetEmulatorUDPSocket_H__


#include "util/random/randomGenerator.h"
#include "util/transport/iUDPSocket.h"
#include "util/transport/packetByteArray.h"
#include <vector>
#include <cstdint>

namespace Anki {
namespace Util {


// Queued serialization of a msghdr (and contents), with scheduled delivery time
class QueuedUDPPacket
{
public:
  
  typedef uint32_t  DeliveryTime;
  
  QueuedUDPPacket();
  QueuedUDPPacket(ssize_t messageSize, const msghdr* messageHeader, QueuedUDPPacket::DeliveryTime deliveryTime);
  ~QueuedUDPPacket();
  
  QueuedUDPPacket(QueuedUDPPacket&& rhs) noexcept;
  QueuedUDPPacket& operator=(QueuedUDPPacket&& rhs) noexcept;
  QueuedUDPPacket(const QueuedUDPPacket& rhs);
  QueuedUDPPacket& operator=(const QueuedUDPPacket& rhs);
  
  void    Set(ssize_t messageSize, const msghdr* messageHeader, QueuedUDPPacket::DeliveryTime deliveryTime);
  ssize_t CopyOut(msghdr* messageHeader) const;
  
  DeliveryTime  GetDeliveryTime() const { return _deliveryTime; }

private:
  
  PacketByteArray   _msgName;
  PacketByteArray   _msgIo;
  PacketByteArray   _msgControl;
  uint32_t          _msgFlags;
  
  DeliveryTime      _deliveryTime;
};


class UDPSocketQueue
{
public:
  
  UDPSocketQueue();
  ~UDPSocketQueue();
  
  void ClearQueues();
  
  void Open(int socketId);
  void Close();
  
  void    QueueMessage(ssize_t messageSize, const msghdr* messageHeader, QueuedUDPPacket::DeliveryTime deliveryTime);
  ssize_t DequeueMessage(msghdr* messageHeader);
  
  bool      IsOpen()  const { return _isOpen; }
  int       GetSocketId() const { return _socketId; }

private:
  
  typedef std::vector<QueuedUDPPacket> PacketQueue;

  PacketQueue  _inMessageQueue;
  int          _socketId;
  bool         _isOpen;
};


class NetEmulatorUDPSocket : public IUDPSocket
{
public:
  
  // IUDPSocket Interface
  
  explicit NetEmulatorUDPSocket(uint32_t randSeed = 0, float inRandomPacketLossPercentage = -1.0f, uint32_t minLatencyInMS = 0, uint32_t maxLatencyInMS = 0);
  virtual ~NetEmulatorUDPSocket();
  
  virtual int OpenSocket(int protocolFamily, int socketType, int protocol) override;
  virtual int BindSocket(int socketId, const sockaddr* socketAddress, socklen_t socketAddressLength) override;
  virtual int GetSocketOpt(int socketId, int level, int optname, void* optVal, socklen_t* optlen) override;
  virtual int SetSocketOpt(int socketId, int level, int optname, const void* optVal, socklen_t optlen) override;
  virtual int CloseSocket(int socketId) override;
  virtual ssize_t SendTo(int socketId, const void* messageData, size_t messageDataSize, int flags, const sockaddr* destSockAddress, socklen_t destSockAddressLength) override;
  virtual ssize_t ReceiveMessage(int socketId, msghdr* messageHeader, int flags) override;
  virtual uint32_t GetLocalIpAddress() override;
  virtual struct in6_addr GetLocalIpv6LinkLocalAddress() override;
  virtual bool IsEmulator() const override { return true; }
  
  // FakeUDPSocket implementation specific

  void SetSocketImpl(IUDPSocket* udpSocketImpl)
  {
    _udpSocketImpl = udpSocketImpl;
  }
  
  // Controls for simulating / emulating different network conditions
  float GetRandomPacketLossPercentage() const;
  void  SetRandomPacketLossPercentage(float inRandomPacketLossPercentage);
  
  uint32_t  GetMinLatencyInMS() const;
  uint32_t  GetMaxLatencyInMS() const;
  void      SetLatencyRangeInMS(uint32_t minLatencyInMS, uint32_t maxLatencyInMS);
  
  uint32_t  GetNumPacketsDropped() const { return _numPacketsDropped; }
  
private:
  
  static const int kMaxSockets = 8;
  
  UDPSocketQueue* FindFreeSocket();
  UDPSocketQueue* FindSocketById(int socketId);
  
  // Member vars
  
  IUDPSocket*                 _udpSocketImpl;
  Anki::Util::RandomGenerator   _randomGenerator;
  UDPSocketQueue              _socketQueues[kMaxSockets];

  float                       _randomPacketLossPercentage; // <0 = no random packet loss, 100 = all packets lost
  uint32_t                    _minLatencyInMS;
  uint32_t                    _maxLatencyInMS;
  
  uint32_t                    _numPacketsDropped;
};


} // end namespace Util
} // end namespace Anki

#endif // __NetworkService_NetEmulatorUDPSocket_H__
