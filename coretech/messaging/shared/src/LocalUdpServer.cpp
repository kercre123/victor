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

// Socket buffer sizes
#define UDP_SERVER_SNDBUFSZ (128*1024)
#define UDP_SERVER_RCVBUFSZ (128*1024)

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
  _clients.clear();

  if (_socketfd < 0) {
    LOG_DEBUG("LocalUdpServer.StopListening: Server already stopped");
    return;
  }

  LOG_DEBUG("LocalUdpServer.StopListening: Stopping server listening on socket " << _socketfd);

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

  //LOG_DEBUG("LocalUdpServer.Send: Sending " << size << " bytes to " << _clients.size() << " clients");

  ssize_t bytes_sent = 0;

  for (ClientListIter it = _clients.begin(); it != _clients.end(); it++ ) {
    const struct sockaddr_un * saddr = &(*it);
    const socklen_t saddrlen = (socklen_t) SUN_LEN(saddr);

    bytes_sent = sendto(_socketfd, data, size, 0, (struct sockaddr *) saddr, saddrlen);
    if (bytes_sent != size) {
      // If send fails, log it and report it to caller.  It is caller's responsibility to retry at
      // some appropriate interval.
      LOG_ERROR("LocalUdpServer.Send: sent " << bytes_sent << " bytes instead of " << size << " (" << strerror(errno) << ")");
    }
  }

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

  // Add client to list
  if (AddClient(saddr, saddrlen) && bytes_received == 1) {
    // If client was newly added, the first datagram (as long as it's only 1 byte long)
    // is assumed to be a "connection packet".
    return 0;
  }

  return bytes_received;

}

bool LocalUdpServer::AddClient(const struct sockaddr_un &saddr, socklen_t saddrlen)
{
  for (ClientListIter it = _clients.begin(); it != _clients.end(); it++) {
    if (memcmp(&(*it),&saddr, saddrlen) == 0)
      return false;
  }

  LOG_DEBUG("LocalUdpServer.AddClient: Adding client " << string(saddr, saddrlen));
  
  _clients.push_back(saddr);
  _clients.back().sun_path[saddrlen - (sizeof(saddr) - sizeof(saddr.sun_path))] = 0;

  return true;
}

bool LocalUdpServer::HasClient() const
{
  return !_clients.empty();
}

int LocalUdpServer::GetNumClients() const
{
  const size_t numClients = _clients.size();

  assert(numClients < std::numeric_limits<int>::max());

  return static_cast<int>(numClients);
}

void LocalUdpServer::Disconnect()
{
  _clients.clear();
}

