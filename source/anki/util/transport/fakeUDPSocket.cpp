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


#include "util/transport/fakeUDPSocket.h"
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


// ============================== FakeUDPPacketData ==============================


FakeUDPPacketData::FakeUDPPacketData()
  : _message()
  , _socketAddressSrc()
  , _socketAddressDest()
{
}


FakeUDPPacketData::FakeUDPPacketData(const void* inBuffer, size_t inSize,
                                     const sockaddr* srcSockAddress,  socklen_t srcSockAddressLength,
                                     const sockaddr* destSockAddress, socklen_t destSockAddressLength)
  : _message()
  , _socketAddressSrc()
  , _socketAddressDest()
{
  Set(inBuffer, inSize, srcSockAddress, srcSockAddressLength, destSockAddress, destSockAddressLength);
}


FakeUDPPacketData::~FakeUDPPacketData()
{
}


void FakeUDPPacketData::Set(const void* inBuffer, size_t inSize,
                            const sockaddr* srcSockAddress,  socklen_t srcSockAddressLength,
                            const sockaddr* destSockAddress, socklen_t destSockAddressLength)
{
  _message.Set(inBuffer, Anki::Util::numeric_cast<uint32_t>(inSize));
  _socketAddressSrc.Set(srcSockAddress, Anki::Util::numeric_cast<uint32_t>(srcSockAddressLength));
  _socketAddressDest.Set(destSockAddress, Anki::Util::numeric_cast<uint32_t>(destSockAddressLength));
}


// ============================== FakeUDPSocketInfo ==============================
  

FakeUDPSocketInfo::FakeUDPSocketInfo()
  : _socketId(-1)
  , _protocolFamily(0)
  , _socketType(0)
  , _protocol(0)
  , _inAddress(0)
  , _port(0)
  , _isOpen(false)
  , _isBound(false)
{
}


FakeUDPSocketInfo::~FakeUDPSocketInfo()
{
  ClearQueues();
}
  
  
void FakeUDPSocketInfo::ClearQueues()
{
  _outMessageQueue.clear();
  _inMessageQueue.clear();
}
  

int FakeUDPSocketInfo::Open(int socketId, int protocolFamily, int socketType, int protocol)
{
  assert(!_isOpen);
  
  if (_isOpen)
  {
    errno = EADDRINUSE;
    return -1;
  }
  
  _socketId = socketId;
  _protocolFamily = protocolFamily;
  _socketType = socketType;
  _protocol = protocol;
  _inAddress = 0;
  _port = 0;
  _isOpen = (_socketId >= 0);
  _isBound = false;
  
  return _socketId;
}
  
  
int FakeUDPSocketInfo::Close()
{
  if (_isOpen)
  {
    ClearQueues();
    _isOpen = false;
    return 0;
  }
  
  errno = ENOTCONN; // The socket is not connected, and no target has been given.
  return -1;
}

  
int FakeUDPSocketInfo::Bind(const sockaddr* socketAddress, socklen_t socketAddressLength)
{
  if (_isOpen)
  {
    if (_isBound)
    {
      errno = EADDRINUSE;
      return -1;
    }
    else
    {
      assert(socketAddress->sa_family == AF_INET);
      assert(socketAddressLength == sizeof(sockaddr_in));
     
      const sockaddr_in* socketAddress_in = reinterpret_cast<const sockaddr_in*>(socketAddress);

      _inAddress = socketAddress_in->sin_addr.s_addr;
      _port      = socketAddress_in->sin_port;
      _isBound   = true;
    }

    return 0;
  }
  
  errno = ENOTCONN; // The socket is not connected, and no target has been given.
  return -1;
}


int FakeUDPSocketInfo::GetSocketOpt(int level, int optname, void* optVal, socklen_t* optlen)
{
  if (_isOpen)
  {
    // TODO, ensure that optVal at least always has the same value written to it
    memset(optVal, 0, *optlen);
    return 0;
  }
  
  errno = ENOTCONN; // The socket is not connected, and no target has been given.
  return -1;
}
  
  
int FakeUDPSocketInfo::SetSocketOpt(int level, int optname, const void* optVal, socklen_t optlen)
{
  if (_isOpen)
  {
    // TODO
    return 0;
  }
  
  errno = ENOTCONN; // The socket is not connected, and no target has been given.
  return -1;
}


ssize_t FakeUDPSocketInfo::SendTo(const void* messageData, size_t messageDataSize, int flags,
                                  const sockaddr* srcSockAddress, socklen_t srcSockAddressLength,
                                  const sockaddr* destSockAddress, socklen_t destSockAddressLength)
{
  if (_isOpen)
  {
    _outMessageQueue.emplace_back(messageData, messageDataSize,
                                  srcSockAddress, srcSockAddressLength,
                                  destSockAddress, destSockAddressLength);
    
    return messageDataSize;
  }
  
  errno = ENOTCONN; // The socket is not connected, and no target has been given.
  return -1;
}


ssize_t FakeUDPSocketInfo::ReceiveMessage(msghdr* messageHeader, int flags)
{
  if (_isOpen)
  {
    if (_inMessageQueue.size() > 0)
    {
      // There are messages in the queue, receive the first one
      
      FakeUDPPacketData& readMessage = _inMessageQueue[0];
      
      // Calculate max data to write out
      size_t availableBytes = 0;
      for (int i=0; i < messageHeader->msg_iovlen; ++i)
      {
        iovec& ioVec = messageHeader->msg_iov[i];
        availableBytes += ioVec.iov_len;
      }
      
      const uint32_t readMessageSize = readMessage.GetMessage().GetSize();

      // Write out data, clamped to max available size in messageHeader if applicable
      const bool isTruncated =  readMessageSize > availableBytes;
      size_t bytesToWrite = isTruncated ? availableBytes : readMessageSize;
      size_t bytesWritten = 0;

      {
        const uint8_t* srcMessageBuffer = readMessage.GetMessage().GetBuffer();
        size_t ioVecIndex = 0;
        
        while (bytesWritten < bytesToWrite)
        {
          assert(ioVecIndex < messageHeader->msg_iovlen);
          iovec& ioVec = messageHeader->msg_iov[ioVecIndex];
          const size_t bytesLeftToWrite = bytesToWrite - bytesWritten;
          const size_t bytesToWrite = (bytesLeftToWrite < ioVec.iov_len) ? bytesLeftToWrite : ioVec.iov_len;
          memcpy(ioVec.iov_base, &srcMessageBuffer[bytesWritten], bytesToWrite);
          bytesWritten += bytesToWrite;
        }
      }
      
      // Copy the SocketAddress out
      {
        const PacketByteArray& srcSocketAddress = readMessage.GetSocketAddressSrc();
        const uint32_t srcSocketAddressSize = srcSocketAddress.GetSize();
        const size_t bytesToWrite = (srcSocketAddressSize < messageHeader->msg_namelen) ? srcSocketAddressSize : messageHeader->msg_namelen;
        memcpy(messageHeader->msg_name, srcSocketAddress.GetBuffer(), bytesToWrite);
        messageHeader->msg_namelen = Anki::Util::numeric_cast<socklen_t>(bytesToWrite);
      }
      
      messageHeader->msg_flags = 0;
      if (isTruncated)
      {
        messageHeader->msg_flags |= MSG_TRUNC;
      }

      
      // Message succefully copied out, remove it
      _inMessageQueue.erase(_inMessageQueue.begin());
      
      return bytesWritten;
    }
    
    // if no message
    {
      errno = EWOULDBLOCK;
      return -1;
    }
  }
  
  errno = ENOTCONN; // The socket is not connected, and no target has been given.
  return -1;
}


void FakeUDPSocketInfo::PipeFrom(FakeUDPSocketInfo& inSocket, uint32_t localIpAddress)
{
  for(MessageQueue::iterator it = inSocket._outMessageQueue.begin(); it != inSocket._outMessageQueue.end(); )
  {
    FakeUDPPacketData& packetData = *it;
    bool pipeThisMessage = false;
    
    {
      const sockaddr* socketAddressSrc  = reinterpret_cast<const sockaddr*>(packetData.GetSocketAddressSrc().GetBuffer());
      const sockaddr* socketAddressDest = reinterpret_cast<const sockaddr*>(packetData.GetSocketAddressDest().GetBuffer());
      
      assert(socketAddressSrc->sa_family  == AF_INET);
      assert(socketAddressDest->sa_family == AF_INET);
      assert(packetData.GetSocketAddressSrc().GetSize()  == sizeof(sockaddr_in));
      assert(packetData.GetSocketAddressDest().GetSize() == sizeof(sockaddr_in));
      
      const sockaddr_in* socketAddressSrc_in  = reinterpret_cast<const sockaddr_in*>(socketAddressSrc);
      const sockaddr_in* socketAddressDest_in = reinterpret_cast<const sockaddr_in*>(socketAddressDest);
      
      const uint32_t srcIp  = socketAddressSrc_in->sin_addr.s_addr;
      const uint32_t destIp = socketAddressDest_in->sin_addr.s_addr;
      
      const bool isListeningToSrcAddress = ((_inAddress == htonl(INADDR_ANY)) || (_inAddress == srcIp));
      const bool isOnCorrectPort = (_port == socketAddressDest_in->sin_port);
      
      const bool dataIsForUs = isListeningToSrcAddress && isOnCorrectPort && (destIp == localIpAddress);
      pipeThisMessage = dataIsForUs;
    }
    
    if (pipeThisMessage)
    {
      _inMessageQueue.push_back( std::move(packetData) );
      it = inSocket._outMessageQueue.erase(it);
    }
    else
    {
      ++it;
    }
  }
}

  
// ============================== FakeUDPSocketManager ==============================
  

class FakeUDPSocketManager
{
public:
  
  FakeUDPSocketManager() { }
  ~FakeUDPSocketManager() { }
  
  
  void Add(FakeUDPSocket* inSocket)
  {
    std::lock_guard<std::mutex> lock(_mutex);
    
    const int existingIndex = FindSocketIndex(inSocket);
    assert(existingIndex < 0);
    if (existingIndex < 0)
    {
      _sockets.push_back(inSocket);
    }
  }
  
  
  void Remove(FakeUDPSocket* inSocket)
  {
    std::lock_guard<std::mutex> lock(_mutex);
    
    const int existingIndex = FindSocketIndex(inSocket);
    assert(existingIndex >= 0);
    if (existingIndex >= 0)
    {
      _sockets.erase( _sockets.begin() + existingIndex);
    }
  }
  
  
  void PipeTo(FakeUDPSocket* inSocket)
  {
    // manager mutex ensures that only 1 socket is attempting to PipeTo
    // at any given time, so there's not possibility of deadlocking
    // (also ensures that _sockets vector doesn't change during this function)
    
    std::lock_guard<std::mutex> lockManager(_mutex);
    {
      // lock inSocket for duration of loop rather than in every single call
      std::lock_guard<std::mutex> lockInSocket(inSocket->GetMutex());

      // See if there are any messages to pipe to inSocket's input queue
    
      for (FakeUDPSocket* socket : _sockets)
      {
        // Note: We deliberately pipe a socket to itself as well
        inSocket->PipeFrom(socket);
      }
    }
  }
  
  
private:
  
  int FindSocketIndex(const FakeUDPSocket* inSocket) const
  {
    int index = 0;
    for (const FakeUDPSocket* socket : _sockets)
    {
      if (socket == inSocket)
      {
        return index;
      }
      
      ++index;
    }
    
    return -1;
  }
  
  std::vector<FakeUDPSocket*> _sockets;
  std::mutex                  _mutex;
};
  

FakeUDPSocketManager g_FakeUDPSocketManager;

  
// ============================== FakeUDPSocket ==============================
  
  
FakeUDPSocket::FakeUDPSocket(uint32_t ipAddress)
  : IUDPSocket()
  , _localIpAddress(ipAddress)
{
  g_FakeUDPSocketManager.Add(this);
}


FakeUDPSocket::~FakeUDPSocket()
{
  g_FakeUDPSocketManager.Remove(this);
}
  
  
int FakeUDPSocket::OpenSocket(int protocolFamily, int socketType, int protocol)
{
  for (int i=0; i < k_MaxSockets; ++i)
  {
    if (!_socketInfo[i].IsOpen())
    {
      return _socketInfo[i].Open(IndexToSocketId(i), protocolFamily, socketType, protocol);
    }
  }
  
  errno = ENFILE; // Too many open files in system
  return -1;
}

  
int FakeUDPSocket::BindSocket(int socketId, const sockaddr* socketAddress, socklen_t socketAddressLength)
{
  FakeUDPSocketInfo* fakeSocket = GetSocketByIndex( SocketIdToIndex(socketId) );
  if (fakeSocket != nullptr)
  {
    return fakeSocket->Bind(socketAddress, socketAddressLength);
  }
  
  errno = ENOTSOCK; // 'Socket operation on non-socket'
  return -1;
}


int FakeUDPSocket::GetSocketOpt(int socketId, int level, int optname, void* optVal, socklen_t* optlen)
{
  FakeUDPSocketInfo* fakeSocket = GetSocketByIndex( SocketIdToIndex(socketId) );
  if (fakeSocket != nullptr)
  {
    return fakeSocket->GetSocketOpt(level, optname, optVal, optlen);
  }
  
  errno = ENOTSOCK; // 'Socket operation on non-socket'
  return -1;
}


int FakeUDPSocket::SetSocketOpt(int socketId, int level, int optname, const void* optVal, socklen_t optlen)
{
  FakeUDPSocketInfo* fakeSocket = GetSocketByIndex( SocketIdToIndex(socketId) );
  if (fakeSocket != nullptr)
  {
    return fakeSocket->SetSocketOpt(level, optname, optVal, optlen);
  }
  
  errno = ENOTSOCK; // 'Socket operation on non-socket'
  return -1;
}

  
int FakeUDPSocket::CloseSocket(int socketId)
{
  FakeUDPSocketInfo* fakeSocket = GetSocketByIndex( SocketIdToIndex(socketId) );
  if (fakeSocket != nullptr)
  {
    return fakeSocket->Close();
  }
  
  errno = ENOTSOCK; // 'Socket operation on non-socket'
  return -1;
}


ssize_t FakeUDPSocket::SendTo(int socketId, const void* messageData, size_t messageDataSize, int flags, const sockaddr* destSockAddress, socklen_t destSockAddressLength)
{
  std::lock_guard<std::mutex> lock(_mutex);
  
  FakeUDPSocketInfo* fakeSocket = GetSocketByIndex( SocketIdToIndex(socketId) );
  if (fakeSocket != nullptr)
  {
    // Build source sockaddr
    
    sockaddr_in srcSockAddress;
    const socklen_t srcSockAddressLength = Anki::Util::numeric_cast<socklen_t>(sizeof( srcSockAddress ));
    memset(&srcSockAddress, 0, sizeof(srcSockAddress));
    srcSockAddress.sin_family = AF_INET; // IPv4
    srcSockAddress.sin_addr.s_addr = _localIpAddress;
    srcSockAddress.sin_port = fakeSocket->GetPort();
    
    ssize_t sendResult = fakeSocket->SendTo(messageData, messageDataSize, flags,
                                            reinterpret_cast<const sockaddr*>(&srcSockAddress), srcSockAddressLength,
                                            destSockAddress, destSockAddressLength);
    
    return sendResult;
  }
  
  errno = ENOTSOCK; // 'Socket operation on non-socket'
  return -1;
}


ssize_t FakeUDPSocket::ReceiveMessage(int socketId, msghdr* messageHeader, int flags)
{
  // See if there's anything to pipe to us from other sockets
  // note: deliberately don't lock this mutex until after PipeTo call
  //       only 1 socket can be in PipeTo at a time and locking before could
  //       lead to a deadlock
  
  g_FakeUDPSocketManager.PipeTo(this);
  
  {
    std::lock_guard<std::mutex> lock(_mutex);
    
    FakeUDPSocketInfo* fakeSocket = GetSocketByIndex( SocketIdToIndex(socketId) );
    if (fakeSocket != nullptr)
    {
      return fakeSocket->ReceiveMessage(messageHeader, flags);
    }
    
    errno = ENOTSOCK; // 'Socket operation on non-socket'
    return -1;
  }
}


class lock_guard_optional
{
public:
  
  lock_guard_optional(std::mutex& inMutex, bool doLock)
    : _mutex(inMutex)
    , _doLock(doLock)
  {
    if (_doLock)
    {
      _mutex.lock();
    }
  }
  
  ~lock_guard_optional()
  {
    if (_doLock)
    {
      _mutex.unlock();
    }
  }
  
private:
  
  std::mutex& _mutex;
  bool        _doLock;
};


void FakeUDPSocket::PipeFrom(FakeUDPSocket* inSocket)
{
  // For anything in inSocket's outgoing queue that's for us, copy it over
  
  // Only lock inSocket if it's not this socket (this socket already locked via PipeTo)
  lock_guard_optional lockInSocket(inSocket->_mutex, (this != inSocket));

  for (int i=0; i < k_MaxSockets; ++i)
  {
    FakeUDPSocketInfo& thisSocketInfo = _socketInfo[i];
    if (thisSocketInfo.IsOpen())
    {
      for (int j=0; j < k_MaxSockets; ++j)
      {
        FakeUDPSocketInfo& incomingSocketInfo = inSocket->_socketInfo[j];
        thisSocketInfo.PipeFrom(incomingSocketInfo, _localIpAddress);
      }
    }
  }
}
  
  
uint32_t FakeUDPSocket::GetLocalIpAddress()
{
  return _localIpAddress;
}

struct in6_addr FakeUDPSocket::GetLocalIpv6LinkLocalAddress()
{
  return {};
}


} // end namespace Util
} // end namespace Anki

