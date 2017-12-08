/**
 * File: LocalUdpServer.cpp
 *
 * Description: Implementation of local-domain socket server class
 *
 * Copyright: Anki, inc. 2017
 *
 */

#include "anki/messaging/shared/LocalUdpServer.h"
#include "anki/messaging/shared/SocketUtils.h"

#include <iostream>

#include <assert.h>
#include <unistd.h>
//
// Socket buffer sizes
//
// Running under webots, the animation process (webotsCtrlAnim) may be delayed several seconds between ticks.
// We must allocate enough buffer space to hold all packets sent during this delay.
//
#ifdef SIMULATOR
#define UDP_SERVER_SNDBUFSZ (2048*1024)
#define UDP_SERVER_RCVBUFSZ (2048*1024)
#else
#define UDP_SERVER_SNDBUFSZ (256*1024)
#define UDP_SERVER_RCVBUFSZ (256*1024)
#endif

//
// This code is shared with robot so we don't have PRINT_NAMED macros available.
//
#define DEBUG_UDP_SERVER

#if defined(DEBUG_UDP_SERVER)
#define LOG_ERROR(__expr__) (std::cerr << "[Error] " << __expr__ << std::endl)
#define LOG_DEBUG(__expr__) (std::cout << "[Debug] " << __expr__ << std::endl)
#else
#define LOG_ERROR(__expr__) (std::cerr << "[Error] " << __expr__ << std::endl)
#define LOG_DEBUG(__expr__) {}
#endif

#if defined(DEBUG_UDP_SERVER)
static std::string string(const struct sockaddr_un & saddr, socklen_t saddrlen)
{
  return std::string(saddr.sun_path, saddrlen - (sizeof(saddr) - sizeof(saddr.sun_path)));
}
#endif

LocalUdpServer::LocalUdpServer()
{
  _socketfd = -1;
}

LocalUdpServer::~LocalUdpServer()
{
  if (_socketfd >= 0) {
    StopListening();
  }
}

bool LocalUdpServer::StartListening(const std::string & sockname)
{
  if (_socketfd >= 0) {
    LOG_ERROR("LocalUdpServer.StartListening: Server is already listening");
    return false;
  }

  _sockname = sockname;

  LOG_DEBUG("LocalUdpServer.StartListening: Creating a socket at " << _sockname);

  // Use PF_LOCAL for UNIX domain socket
  // Use SOCK_STREAM for TCP or SOCK_DGRAM for UDP
  const int sock_family = PF_LOCAL;
  const int sock_type = SOCK_DGRAM;

  _socketfd = socket(sock_family, sock_type, 0);
  if (_socketfd == -1) {
    LOG_ERROR("LocalUdpServer.StartListening: Unable to create socket at " << _sockname << "(" << strerror(errno) << ")");
    return false;
  }

  if (!Anki::Messaging::SetNonBlocking(_socketfd, 1)) {
    LOG_ERROR("LocalUdpServer.StartListening: Unable to set nonblocking (" << strerror(errno) << ")");
    return false;
  }

  if (!Anki::Messaging::SetReuseAddress(_socketfd, 1)) {
    LOG_ERROR("LocalUdpServer.StartListening: Unable to set reuseaddress (" << strerror(errno) << ")");
    return false;
  }

  if (!Anki::Messaging::SetSendBufferSize(_socketfd, UDP_SERVER_SNDBUFSZ)) {
    LOG_ERROR("LocalUdpServer.StartListening: Unable to set send buffer size (" << strerror(errno) << ")");
    return false;
  }

  if (!Anki::Messaging::SetRecvBufferSize(_socketfd, UDP_SERVER_RCVBUFSZ)) {
    LOG_ERROR("LocalUdpServer.StartListening: Unable to set recv buffer size (" << strerror(errno) << ")");
    return false;
  }

  // Remove any existing socket using this name
  unlink(_sockname.c_str());

  struct sockaddr_un saddr;
  memset(&saddr, 0, sizeof(saddr));
  saddr.sun_family = sock_family;
  strncpy(saddr.sun_path, _sockname.c_str(), sizeof(saddr.sun_path));
  const socklen_t socklen = (socklen_t) SUN_LEN(&saddr);

  const int status = bind(_socketfd, (const struct sockaddr*) &saddr, socklen);
  if (status == -1) {
    LOG_ERROR("LocalUdpServer.StartListening: Unable to bind at " << _sockname << "(" << strerror(errno) << ")");
    LOG_ERROR("LocalUdpServer.StartListening: You might have orphaned processes running");
    return false;
  }

  LOG_DEBUG("LocalUdpServer.StartListening: Socket is bound at " << _sockname);

  return true;

}

void LocalUdpServer::StopListening()
{
  if (_socketfd < 0) {
    LOG_DEBUG("LocalUdpServer.StopListening: Server already stopped");
    return;
  }

  LOG_DEBUG("LocalUdpServer.StopListening: Stopping server listening on socket " << _socketfd);

  if (HasClient()) {
    Disconnect();
  }

  if (close(_socketfd) < 0) {
    LOG_ERROR("LocalUdpServer.StopListening: Error closing socket (" << strerror(errno) << ")");
  }
  _socketfd = -1;
}


ssize_t LocalUdpServer::Send(const char* data, int size)
{
  if (size <= 0) {
    return 0;
  }

  if (!HasClient()) {
    LOG_DEBUG("LocalUdpServer.Send: no client");
    return -1;
  }

  //LOG_DEBUG("LocalUdpServer.Send: Sending " << size << " bytes to " << _peername);

  const ssize_t bytes_sent = send(_socketfd, data, size, 0);
  if (bytes_sent != size) {
    // If send fails, log it and report it to caller.  It is caller's responsibility to retry at
    // some appropriate interval.
    LOG_ERROR("LocalUdpServer.Send: sent " << bytes_sent << " bytes instead of " << size << " (" << strerror(errno) << ")");
  }

  //LOG_DEBUG("LocalUdpServer.Send: " << bytes_sent << " bytes sent");
  return bytes_sent;

}

ssize_t LocalUdpServer::Recv(char* data, int maxSize)
{
  struct sockaddr_un saddr;
  socklen_t saddrlen = sizeof(saddr);

  const ssize_t bytes_received = recvfrom(_socketfd, data, maxSize, 0, (struct sockaddr *)&saddr, &saddrlen);
  
  if (bytes_received <= 0) {
    if (errno == EWOULDBLOCK) {
      //LOG_DEBUG("LocalUdpServer.Recv: No data available");
      return 0;
    } else {
      LOG_ERROR("LocalUdpServer.Recv: Receive error " << strerror(errno));
      return -1;
    }
  }

  // LOG_DEBUG("LocalUdpServer.Recv: received " << bytes_received << " bytes from " << string(saddr, saddrlen));

  // Connect to new client?
  if (!HasClient()) {
    if (AddClient(saddr, saddrlen) && bytes_received == 1) {
      // If client was newly added, the first datagram (as long as it's only 1 byte long)
      // is assumed to be a "connection packet".
      return 0;
    }
  }

  return bytes_received;

}

bool LocalUdpServer::AddClient(const struct sockaddr_un &saddr, socklen_t saddrlen)
{
  std::string peername = string(saddr, saddrlen);

  LOG_DEBUG("LocalUdpServer.AddClient: Adding client " << peername);

  if (connect(_socketfd, (struct sockaddr *) &saddr, saddrlen) != 0) {
    LOG_ERROR("LocalUdpServer.AddClient: Unable to connect to " << peername << " (" << strerror(errno) << ")");
    return false;
  }
  _peername = peername;
  return true;
}



void LocalUdpServer::Disconnect()
{
  if (!HasClient()) {
    return;
  }

  LOG_DEBUG("LocalUdpServer.Disconnect: Disconnect from peer " << _peername);

  // Undo effects of connect() by resetting peer to an unspecified address
  struct sockaddr saddr;
  saddr.sa_family = AF_UNSPEC;
  if (connect(_socketfd, &saddr, sizeof(saddr)) != 0) {
    // MacOS returns ENOENT but operation has desired effect regardless.
    if (errno != ENOENT) {
      LOG_ERROR("LocalUdpServer.Disconnect: Failed to disconnect (" << strerror(errno) << ")");
    }
  }

  _peername.clear();
}

