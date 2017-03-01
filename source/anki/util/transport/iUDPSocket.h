/**
 * File: iUDPSocket
 *
 * Author: Mark Wesley
 * Created: 02/03/15
 *
 * Description: Interface for a Posix/BSD style socket implementation
 *              This allows a lightweight wrapper around Posix sockets to be swapped out for virtual/test impl
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __NetworkService_iUDPSocket_H__
#define __NetworkService_iUDPSocket_H__


#include <sys/socket.h> // for socklen_t
#include <netinet/in.h>
#include <stdint.h>


struct msghdr;
struct sockaddr;
struct sockaddr_in;


namespace Anki {
namespace Util {


  class IUDPSocket
  {
  public:
    
    IUDPSocket() {}
    virtual ~IUDPSocket() {}
    
    virtual int OpenSocket(int protocolFamily, int socketType, int protocol) = 0;
    virtual int BindSocket(int socketId, const sockaddr* socketAddress, socklen_t socketAddressLength) = 0;
    virtual int GetSocketOpt(int socketId, int level, int optname, void* optVal, socklen_t* optlen) = 0;
    virtual int SetSocketOpt(int socketId, int level, int optname, const void* optVal, socklen_t optlen) = 0;
    virtual int CloseSocket(int socketId) = 0;
    virtual ssize_t SendTo(int socketId, const void* messageData, size_t messageDataSize, int flags, const sockaddr* destSockAddress, socklen_t destSockAddressLength) = 0;
    virtual ssize_t ReceiveMessage(int socketId, msghdr* messageHeader, int flags) = 0;
    virtual uint32_t GetLocalIpAddress() = 0;
    virtual struct in6_addr GetLocalIpv6LinkLocalAddress() = 0;
    
    virtual bool IsEmulator() const { return false; }
  };


} // end namespace Util
} // end namespace Anki

#endif // __NetworkService_iUDPSocket_H__
