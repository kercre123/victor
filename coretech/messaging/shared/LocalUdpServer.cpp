/**
 * File: LocalUdpServer.cpp
 *
 * Description: Implementation of local-domain socket server class
 *
 * Copyright: Anki, inc. 2017
 *
 */

#include "coretech/messaging/shared/LocalUdpServer.h"
#include "coretech/messaging/shared/SocketUtils.h"
#include "coretech/common/shared/logging.h"

#include <iostream>

#include <assert.h>
#include <unistd.h>
//
// Socket buffer sizes
//
#define UDP_SERVER_SNDBUFSZ (256*1024)
#define UDP_SERVER_RCVBUFSZ (256*1024)

// Define this to enable logs
#define LOG_CHANNEL                    "LocalUdpServer"

#ifdef  LOG_CHANNEL
#define LOG_ERROR(name, format, ...)   CORETECH_LOG_ERROR(name, format, ##__VA_ARGS__)
#define LOG_WARNING(name, format, ...) CORETECH_LOG_WARNING(name, format, ##__VA_ARGS__)
#define LOG_INFO(name, format, ...)    CORETECH_LOG_INFO(LOG_CHANNEL, name, format, ##__VA_ARGS__)
#define LOG_DEBUG(name, format, ...)   CORETECH_LOG_DEBUG(LOG_CHANNEL, name, format, ##__VA_ARGS__)
#else
#define LOG_ERROR(name, format, ...)   {}
#define LOG_WARNING(name, format, ...) {}
#define LOG_INFO(name, format, ...)    {}
#define LOG_DEBUG(name, format, ...)   {}
#endif

static std::string to_string(const struct sockaddr_un & saddr, socklen_t saddrlen)
{
  return std::string(saddr.sun_path, saddrlen - (sizeof(saddr) - sizeof(saddr.sun_path)));
}

LocalUdpServer::LocalUdpServer()
{
  _bindClients = true;
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
    LOG_ERROR("LocalUdpServer.StartListening", "Server is already listening");
    return false;
  }

  _sockname = sockname;

  LOG_DEBUG("LocalUdpServer.StartListening", "Creating a socket at %s", _sockname.c_str());

  // Use PF_LOCAL for UNIX domain socket
  // Use SOCK_STREAM for TCP or SOCK_DGRAM for UDP
  const int sock_family = PF_LOCAL;
  const int sock_type = SOCK_DGRAM;

  _socketfd = socket(sock_family, sock_type, 0);
  if (_socketfd == -1) {
    LOG_ERROR("LocalUdpServer.StartListening", "Unable to create socket at %s (%s)", _sockname.c_str(), strerror(errno));
    return false;
  }

  if (!Anki::Messaging::SetNonBlocking(_socketfd, 1)) {
    LOG_ERROR("LocalUdpServer.StartListening", "Unable to set nonblocking (%s)", strerror(errno));
    close(_socketfd);
    _socketfd = -1;
    return false;
  }

  if (!Anki::Messaging::SetReuseAddress(_socketfd, 1)) {
    LOG_ERROR("LocalUdpServer.StartListening", "Unable to set reuseaddress (%s)", strerror(errno));
    close(_socketfd);
    _socketfd = -1;
    return false;
  }

  if (!Anki::Messaging::SetSendBufferSize(_socketfd, UDP_SERVER_SNDBUFSZ)) {
    LOG_ERROR("LocalUdpServer.StartListening", "Unable to set send buffer size (%s)", strerror(errno));
    close(_socketfd);
    _socketfd = -1;
    return false;
  }

  if (!Anki::Messaging::SetRecvBufferSize(_socketfd, UDP_SERVER_RCVBUFSZ)) {
    LOG_ERROR("LocalUdpServer.StartListening", "Unable to set recv buffer size (%s)", strerror(errno));
    close(_socketfd);
    _socketfd = -1;
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
    LOG_ERROR("LocalUdpServer.StartListening", "Unable to bind at %s (%s)", _sockname.c_str(), strerror(errno));
    LOG_ERROR("LocalUdpServer.StartListening", "You might have orphaned processes running");
    close(_socketfd);
    _socketfd = -1;
    return false;
  }

  LOG_DEBUG("LocalUdpServer.StartListening", "Socket is bound at %s", _sockname.c_str());

  return true;

}

void LocalUdpServer::StopListening()
{
  if (_socketfd < 0) {
    LOG_DEBUG("LocalUdpServer.StopListening", "Server already stopped");
    return;
  }

  LOG_DEBUG("LocalUdpServer.StopListening", "Stopping server listening on socket %d", _socketfd);

  if (HasClient()) {
    Disconnect();
  }

  if (close(_socketfd) < 0) {
    LOG_ERROR("LocalUdpServer.StopListening", "Error closing socket (%s)", strerror(errno));
  }
  _socketfd = -1;
}


ssize_t LocalUdpServer::Send(const char* data, int size)
{
  if (size <= 0) {
    return 0;
  }

  if (!HasClient()) {
    LOG_DEBUG("LocalUdpServer.Send", "No client");
    return -1;
  }

  //LOG_DEBUG("LocalUdpServer.Send", "Sending %d bytes to %s", size, _peername.c_str());
  ssize_t bytes_sent = 0;
  if (_bindClients) {
    bytes_sent = send(_socketfd, data, size, 0);
  }
  else {
    const socklen_t socklen = (socklen_t) SUN_LEN(&_client);
    bytes_sent = sendto(_socketfd, data, size, 0, (sockaddr*)&_client, socklen);
  }

  if (bytes_sent != size) {
    // If send fails, log it and report it to caller.  It is caller's responsibility to retry at
    // some appropriate interval.
    LOG_ERROR("LocalUdpServer.Send", "Sent %zd bytes instead of %d (%s)", bytes_sent, size, strerror(errno));
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
      //LOG_DEBUG("LocalUdpServer.Recv", "No data available");
      return 0;
    } else {
      LOG_ERROR("LocalUdpServer.Recv", "Receive error (%s)", strerror(errno));
      return -1;
    }
  }

  //LOG_DEBUG("LocalUdpServer.Recv", "Received %zd bytes from %s", bytes_received, to_string(saddr, saddrlen).c_str());

  // Connect to new client?
  if (!HasClient() || !_bindClients) {
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
  const std::string & peername = to_string(saddr, saddrlen);
  if (_bindClients) {
    LOG_DEBUG("LocalUdpServer.AddClient", "Adding client %s", peername.c_str());

    if (connect(_socketfd, (struct sockaddr *) &saddr, saddrlen) != 0) {
      LOG_ERROR("LocalUdpServer.AddClient", "Unable to connect to %s (%s)", peername.c_str(), strerror(errno));
      return false;
    }
  }
  else {
    if (memcmp(&_client, &saddr, saddrlen) == 0) {
      return false;
    }

    _client = saddr;
    _client.sun_path[saddrlen - (sizeof(saddr) - sizeof(saddr.sun_path))] = 0;
  }
  _peername = peername;
  return true;
}



void LocalUdpServer::Disconnect()
{
  if (!HasClient()) {
    return;
  }

  LOG_DEBUG("LocalUdpServer.Disconnect", "Disconnect from peer %s", _peername.c_str());

  if (_bindClients) {
    // Undo effects of connect() by resetting peer to an unspecified address
    struct sockaddr saddr;
    saddr.sa_family = AF_UNSPEC;
    if (connect(_socketfd, &saddr, sizeof(saddr)) != 0) {
      // MacOS returns ENOENT but operation has desired effect regardless.
      if (errno != ENOENT) {
        LOG_ERROR("LocalUdpServer.Disconnect", "Failed to disconnect (%s)", strerror(errno));
      }
    }
  }

  _peername.clear();
}

