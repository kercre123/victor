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


#include "util/transport/netEmulatorUDPSocket.h"
#include "util/console/consoleInterface.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include <assert.h>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>


namespace Anki {
namespace Util {
  
  
CONSOLE_VAR_RANGED(float,    gUDPRandomPacketLossPercentage, "Network.Emulator", -1.0f, -1.0f, 101.0f);
CONSOLE_VAR_RANGED(uint32_t, gUDPMinLatency,                 "Network.Emulator",  0, 0, 5000);
CONSOLE_VAR_RANGED(uint32_t, gUDPMaxLatency,                 "Network.Emulator",  0, 0, 5000);
  
  
// ============================== Helper functions ==============================


static bool RandPercentChanceOccured(float randPercentSuccess, float randValueIn0To100Range)
{
  // randPercentSuccess <=   0 -> always false
  // randPercentSuccess >= 100 -> always true

  if (randPercentSuccess <= 0.0f)
  {
    return false;
  }
  
  // Add a small epsilon to ensuret that 100% is always true
  const float k_Epsilon = 0.00000001f;
  return (randValueIn0To100Range < (randPercentSuccess + k_Epsilon));
}
 

QueuedUDPPacket::DeliveryTime GetCurrentDeliveryTime() // Milliseconds since first call to this function
{
  using namespace std::chrono;
  
  static high_resolution_clock::time_point firstEverTime = high_resolution_clock::now();
  high_resolution_clock::time_point currentTimeChrono = high_resolution_clock::now();
  
  const double numMilliSecondsSinceStart = duration_cast<milliseconds>(currentTimeChrono - firstEverTime).count();
  const QueuedUDPPacket::DeliveryTime currentDeliveryTime = Anki::Util::numeric_cast<QueuedUDPPacket::DeliveryTime>(numMilliSecondsSinceStart);
  
  return currentDeliveryTime;
}


// ============================== QueuedUDPPacket ==============================

  
QueuedUDPPacket::QueuedUDPPacket()
  : _msgFlags()
  , _deliveryTime(0)
{
}


QueuedUDPPacket::QueuedUDPPacket(ssize_t messageSize, const msghdr* messageHeader, QueuedUDPPacket::DeliveryTime deliveryTime)
  : _msgFlags()
  , _deliveryTime(0)
{
  Set(messageSize, messageHeader, deliveryTime);
}


QueuedUDPPacket::~QueuedUDPPacket()
{
}
  
  
QueuedUDPPacket::QueuedUDPPacket(QueuedUDPPacket&& rhs) noexcept
  : _msgName( std::move(rhs._msgName) )
  , _msgIo( std::move(rhs._msgIo) )
  , _msgControl( std::move(rhs._msgControl) )
  , _msgFlags(rhs._msgFlags)
  , _deliveryTime(rhs._deliveryTime)
{
}


QueuedUDPPacket& QueuedUDPPacket::operator=(QueuedUDPPacket&& rhs) noexcept
{
  if (this != &rhs)
  {
    _msgName      = std::move(rhs._msgName);
    _msgIo        = std::move(rhs._msgIo);
    _msgControl   = std::move(rhs._msgControl);
    _msgFlags     = rhs._msgFlags;
    _deliveryTime = rhs._deliveryTime;
  }
  return *this;
}

  
QueuedUDPPacket::QueuedUDPPacket(const QueuedUDPPacket& rhs)
  : _msgName(rhs._msgName)
  , _msgIo(rhs._msgIo)
  , _msgControl(rhs._msgControl)
  , _msgFlags(rhs._msgFlags)
  , _deliveryTime(rhs._deliveryTime)
{
}


QueuedUDPPacket& QueuedUDPPacket::operator=(const QueuedUDPPacket& rhs)
{
  if (this != &rhs)
  {
    _msgName      = rhs._msgName;
    _msgIo        = rhs._msgIo;
    _msgControl   = rhs._msgControl;
    _msgFlags     = rhs._msgFlags;
    _deliveryTime = rhs._deliveryTime;
  }
  return *this;
}


void QueuedUDPPacket::Set(ssize_t messageSize, const msghdr* messageHeader, QueuedUDPPacket::DeliveryTime deliveryTime)
{
  _msgName.Set(messageHeader->msg_name, messageHeader->msg_namelen);

  {
    // only 1 contiguous buffer support currently - if required add support for appending them into stored buffer
    assert(messageHeader->msg_iovlen == 1);
    {
      iovec& ioVec = messageHeader->msg_iov[0];
      const uint32_t clampedSize = std::min(Anki::Util::numeric_cast<uint32_t>(messageSize), Anki::Util::numeric_cast<uint32_t>(ioVec.iov_len));
      _msgIo.Set(ioVec.iov_base, clampedSize);
    }
  }

  _msgControl.Set(messageHeader->msg_control, (uint32_t)messageHeader->msg_controllen);
  _msgFlags = messageHeader->msg_flags;
  
  _deliveryTime = deliveryTime;
}

  
ssize_t QueuedUDPPacket::CopyOut(msghdr* messageHeader) const
{
  
  _msgName.CopyTo(messageHeader->msg_name, messageHeader->msg_namelen);
  
  {
    // only 1 contiguous buffer support currently - if required add support for appending them into stored buffer
    assert(messageHeader->msg_iovlen == 1);
    {
      iovec& ioVec = messageHeader->msg_iov[0];
      uint32_t inOutLen =  Anki::Util::numeric_cast<uint32_t>(ioVec.iov_len);
      _msgIo.CopyTo(ioVec.iov_base, inOutLen);
      ioVec.iov_len = Anki::Util::numeric_cast<size_t>(inOutLen);
    }
  }
  
  _msgControl.CopyTo(messageHeader->msg_control, messageHeader->msg_controllen);
  messageHeader->msg_flags = _msgFlags;
  
  return _msgIo.GetSize();
}


// ============================== UDPSocketQueue ==============================
  

UDPSocketQueue::UDPSocketQueue()
  : _socketId(-1)
  , _isOpen(false)
{
}


UDPSocketQueue::~UDPSocketQueue()
{
  ClearQueues();
}
  
  
void UDPSocketQueue::ClearQueues()
{
  _inMessageQueue.clear();
}
  

void UDPSocketQueue::Open(int socketId)
{
  if (_isOpen)
  {
    assert(0); // shouldn't happen?
    Close();
  }
  
  _isOpen = true;
  _socketId = socketId;
}
  
  
void UDPSocketQueue::Close()
{
  ClearQueues();
  _isOpen = false;
  _socketId = -1;
}


void UDPSocketQueue::QueueMessage(ssize_t messageSize, const msghdr* messageHeader, QueuedUDPPacket::DeliveryTime deliveryTime)
{
  if (_isOpen)
  {
    _inMessageQueue.emplace_back(messageSize, messageHeader, deliveryTime);
  }
  else
  {
    assert(0); // just for testing, remove for checkin
  }
}


ssize_t UDPSocketQueue::DequeueMessage(msghdr* messageHeader)
{
  if (_isOpen)
  {
    // find the first deliverable message
    
    for(PacketQueue::iterator it = _inMessageQueue.begin(); it != _inMessageQueue.end(); )
    {
      QueuedUDPPacket& queuedMessage = *it;
      const bool isReadyToDeliver = (GetCurrentDeliveryTime() >= queuedMessage.GetDeliveryTime());
      
      if (isReadyToDeliver)
      {
        // copy out the queued message, then delete and remove it from the queue
        ssize_t retVal = queuedMessage.CopyOut(messageHeader);
        it = _inMessageQueue.erase(it);
        
        return retVal;
      }
      else
      {
        ++it;
      }
    }
  }
    
  return -1;
}
  
 
NetEmulatorUDPSocket::NetEmulatorUDPSocket(uint32_t randSeed, float inRandomPacketLossPercentage, uint32_t minLatencyInMS, uint32_t maxLatencyInMS)
  : IUDPSocket()
  , _udpSocketImpl(nullptr)
  , _randomGenerator(randSeed)
  , _randomPacketLossPercentage(-1.0f)
  , _minLatencyInMS(0)
  , _maxLatencyInMS(0)
  , _numPacketsDropped(0)
{
  SetRandomPacketLossPercentage(inRandomPacketLossPercentage);
  SetLatencyRangeInMS(minLatencyInMS, maxLatencyInMS);
}


NetEmulatorUDPSocket::~NetEmulatorUDPSocket()
{
}
  

UDPSocketQueue* NetEmulatorUDPSocket::FindFreeSocket()
{
  for (int i=0; i < kMaxSockets; ++i)
  {
    UDPSocketQueue& socketQueue = _socketQueues[i];
    if (!socketQueue.IsOpen())
    {
      return &socketQueue;
    }
  }
  
  return nullptr;
}
  
  
UDPSocketQueue* NetEmulatorUDPSocket::FindSocketById(int socketId)
{
  for (int i=0; i < kMaxSockets; ++i)
  {
    UDPSocketQueue& socketQueue = _socketQueues[i];
    if (socketQueue.IsOpen() && (socketQueue.GetSocketId() == socketId))
    {
      return &socketQueue;
    }
  }
  
  return nullptr;
}
  
  
int NetEmulatorUDPSocket::OpenSocket(int protocolFamily, int socketType, int protocol)
{
  UDPSocketQueue* socketQueue = FindFreeSocket();
  
  if (socketQueue != nullptr)
  {
    const int socketId = _udpSocketImpl->OpenSocket(protocolFamily, socketType, protocol);
    socketQueue->Open(socketId);
    return socketId;
  }
  else
  {
    PRINT_NAMED_ERROR("NetEmulatorUDPSocket", "Error: No free emulator socket queues - all %d are in use!", kMaxSockets);
    errno = ENOBUFS;
    return -1;
  }
}

  
int NetEmulatorUDPSocket::BindSocket(int socketId, const sockaddr* socketAddress, socklen_t socketAddressLength)
{
  return _udpSocketImpl->BindSocket(socketId, socketAddress, socketAddressLength);
}


int NetEmulatorUDPSocket::GetSocketOpt(int socketId, int level, int optname, void* optVal, socklen_t* optlen)
{
  return _udpSocketImpl->GetSocketOpt(socketId, level, optname, optVal, optlen);
}


int NetEmulatorUDPSocket::SetSocketOpt(int socketId, int level, int optname, const void* optVal, socklen_t optlen)
{
  return _udpSocketImpl->SetSocketOpt(socketId, level, optname, optVal, optlen);
}

  
int NetEmulatorUDPSocket::CloseSocket(int socketId)
{
  UDPSocketQueue* socketQueue = FindSocketById(socketId);
  if (socketQueue != nullptr)
  {
    socketQueue->Close();
  }

  return _udpSocketImpl->CloseSocket(socketId);
}


ssize_t NetEmulatorUDPSocket::SendTo(int socketId, const void* messageData, size_t messageDataSize, int flags, const sockaddr* destSockAddress, socklen_t destSockAddressLength)
{
  return _udpSocketImpl->SendTo(socketId, messageData, messageDataSize, flags, destSockAddress, destSockAddressLength);
}


ssize_t NetEmulatorUDPSocket::ReceiveMessage(int socketId, msghdr* messageHeader, int flags)
{
  const msghdr inMessageHeader = *messageHeader;
  
  UDPSocketQueue* socketQueue = FindSocketById(socketId);
  
  // Pump the wrapped socket - we deliberately empty it here (rather than 1 per call) to ensure correct time stamps
  {
    ssize_t wrappedRecv = 0;
    do
    {
      wrappedRecv = _udpSocketImpl->ReceiveMessage(socketId, messageHeader, flags);
      if (wrappedRecv >= 0)
      {
        assert(socketQueue != nullptr);
        
        if (RandPercentChanceOccured(GetRandomPacketLossPercentage(), _randomGenerator.RandDblInRange(0.0f, 100.0f)))
        {
          // throw away the packet
          ++_numPacketsDropped;
        }
        else
        {
          // Queue packet to be delivered in the future
          QueuedUDPPacket::DeliveryTime deliveryTime = GetCurrentDeliveryTime() + _randomGenerator.RandIntInRange(_minLatencyInMS, _maxLatencyInMS);

          socketQueue->QueueMessage(wrappedRecv, messageHeader, deliveryTime);
        }
      }
      else
      {
        if (errno != EWOULDBLOCK)
        {
          // pass any real errors on directly
          return wrappedRecv;
        }
      }

      *messageHeader = inMessageHeader; // restore buffer sizes etc. for next read attempt
      
    } while (wrappedRecv >= 0);
  }
  
  if (socketQueue != nullptr)
  {
    // see if there's anything we can dequeue currently
    const ssize_t messageSize = socketQueue->DequeueMessage(messageHeader);

    if (messageSize < 0)
    {
      // no message, just report that there's nothing there yet
      errno = EWOULDBLOCK;
    }
    
    return messageSize;
  }
  else
  {
    errno = ENOTSOCK; // 'Socket operation on non-socket'
    return -1;
  }
}

  
uint32_t NetEmulatorUDPSocket::GetLocalIpAddress()
{
  return _udpSocketImpl->GetLocalIpAddress();
}

struct in6_addr NetEmulatorUDPSocket::GetLocalIpv6LinkLocalAddress()
{
  return _udpSocketImpl->GetLocalIpv6LinkLocalAddress();
}
  
float NetEmulatorUDPSocket::GetRandomPacketLossPercentage() const
{
  return (gUDPRandomPacketLossPercentage > _randomPacketLossPercentage) ? gUDPRandomPacketLossPercentage : _randomPacketLossPercentage;
}
  
  
void NetEmulatorUDPSocket::SetRandomPacketLossPercentage(float inRandomPacketLossPercentage)
{
  _randomPacketLossPercentage = inRandomPacketLossPercentage;
}


uint32_t NetEmulatorUDPSocket::GetMinLatencyInMS() const
{
  return (gUDPMaxLatency > gUDPMinLatency) ? gUDPMinLatency : _minLatencyInMS;
}


uint32_t NetEmulatorUDPSocket::GetMaxLatencyInMS() const
{
  return (gUDPMaxLatency > gUDPMinLatency) ? gUDPMinLatency : _maxLatencyInMS;
}

  
void  NetEmulatorUDPSocket::SetLatencyRangeInMS(uint32_t minLatencyInMS, uint32_t maxLatencyInMS)
{
  assert(maxLatencyInMS >= minLatencyInMS);
  _minLatencyInMS = minLatencyInMS;
  _maxLatencyInMS = maxLatencyInMS;
}


} // end namespace Util
} // end namespace Anki

