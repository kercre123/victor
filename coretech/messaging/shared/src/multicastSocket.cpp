/*
 * multicastSocket.cpp
 *
 */

#include "anki/messaging/shared/multicastSocket.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

namespace
{
  struct sockaddr_in send_addr, recv_addr;
  int fd = -1;
  //int addrlen;
  struct ip_mreq mreq;
}

void MulticastSocket::set_nonblock(int socket) {
  int flags;
  flags = fcntl(socket,F_GETFL,0);
  assert(flags != -1);
  fcntl(socket, F_SETFL, flags | O_NONBLOCK);
}

MulticastSocket::MulticastSocket()
{
}

//MulticastSocket::~MulticastSocket()
//{
//  Disconnect();
//}

bool MulticastSocket::Connect(const char* mcast_addr, uint16_t port, bool enable_loop)
{
  // ---- Send setup ----
  memset(&send_addr,0,sizeof(send_addr));
  send_addr.sin_family=AF_INET;
  send_addr.sin_addr.s_addr=inet_addr(mcast_addr);
  send_addr.sin_port=htons(port);

  
  // ---- Recv socket setup ----
  // Create UDP socket
  if ((fd=socket(AF_INET,SOCK_DGRAM,0)) < 0) {
    perror("socket");
    return false;
  }
  
  // Allow port reuse by multiple sockets
  unsigned int yes=1;
  if (setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)) < 0) {
    perror("Reusing ADDR failed");
    return false;
  }
  
  // Enable loopback on same ip
  int loop = enable_loop ? 1 : 0;
  if (setsockopt(fd,IPPROTO_IP,IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0) {
    perror("Setting loop failed");
    return false;
  }
  
  memset(&recv_addr,0,sizeof(recv_addr));
  recv_addr.sin_family=AF_INET;
  recv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
  recv_addr.sin_port=htons(port);
  
  // Set socket to non-blocking
  set_nonblock(fd);
  
  if (bind(fd,(struct sockaddr *) &recv_addr,sizeof(recv_addr)) < 0) {
    perror("bind");
    return false;
  }
  
  /* use setsockopt() to request that the kernel join a multicast group */
  mreq.imr_multiaddr.s_addr=inet_addr(mcast_addr);
  mreq.imr_interface.s_addr=htonl(INADDR_ANY);
  if (setsockopt(fd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) < 0) {
    perror("setsockopt");
    return false;
  }

  return true;
}

void MulticastSocket::Disconnect()
{
  if (fd >= 0) {
    close(fd);
    fd = -1;
  }
}

bool MulticastSocket::IsConnected() const
{
  return fd >= 0;
}

ssize_t MulticastSocket::Send(const char* data, size_t size)
{
  return sendto(fd,data,size,0,(struct sockaddr *) &send_addr, sizeof(send_addr));
}

ssize_t MulticastSocket::Recv(char* data, size_t maxSize, uint32_t* ipAddr)
{
  socklen_t addrlen=sizeof(recv_addr);
  ssize_t bytesRecvd = recvfrom(fd,data,maxSize,0,(struct sockaddr *) &recv_addr,&addrlen);
  
  if (bytesRecvd <= 0) {
    if (errno == EWOULDBLOCK) {
      return 0;
    }
  }
  
  if (ipAddr) {
    *ipAddr = recv_addr.sin_addr.s_addr;
  }
  
//  char addbuf[128];
//  snprintf(addbuf, sizeof(addbuf), "Addr: %s", inet_ntoa(recv_addr.sin_addr));
//  puts(addbuf);
  
  return bytesRecvd;
}

