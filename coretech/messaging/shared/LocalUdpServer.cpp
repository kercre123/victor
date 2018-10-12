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

constexpr const char LocalUdpServer::kConnectionPacket[];

static std::string to_string(const struct sockaddr_un & saddr, socklen_t saddrlen)
{
  return std::string(saddr.sun_path, saddrlen - (sizeof(saddr) - sizeof(saddr.sun_path)));
}

LocalUdpServer::LocalUdpServer(int sndbufsz, int rcvbufsz)
: _sndbufsz(sndbufsz)
, _rcvbufsz(rcvbufsz)
, _socket(-1)
, _bindClients(true)
{
}

LocalUdpServer::LocalUdpServer() : LocalUdpServer(UDP_SERVER_SNDBUFSZ, UDP_SERVER_RCVBUFSZ)
{
}

LocalUdpServer::~LocalUdpServer()
{
  if (_socket >= 0) {
    StopListening();
  }
}

bool LocalUdpServer::StartListening(const std::string & sockname)
{
  if (_socket >= 0) {
    LOG_ERROR("LocalUdpServer.StartListening", "Server is already listening");
    return false;
  }

  _sockname = sockname;

  LOG_DEBUG("LocalUdpServer.StartListening", "Creating a socket at %s", _sockname.c_str());

  // Use PF_LOCAL for UNIX domain socket
  // Use SOCK_STREAM for TCP or SOCK_DGRAM for UDP
  const int sock_family = PF_LOCAL;
  const int sock_type = SOCK_DGRAM;

  _socket = socket(sock_family, sock_type, 0);
  if (_socket == -1) {
    LOG_ERROR("LocalUdpServer.StartListening", "Unable to create socket at %s (%s)", _sockname.c_str(), strerror(errno));
    return false;
  }

  if (!Anki::Messaging::SetNonBlocking(_socket, 1)) {
    LOG_ERROR("LocalUdpServer.StartListening", "Unable to set nonblocking (%s)", strerror(errno));
    close(_socket);
    _socket = -1;
    return false;
  }

  if (!Anki::Messaging::SetReuseAddress(_socket, 1)) {
    LOG_ERROR("LocalUdpServer.StartListening", "Unable to set reuseaddress (%s)", strerror(errno));
    close(_socket);
    _socket = -1;
    return false;
  }

  if (!Anki::Messaging::SetSendBufferSize(_socket, _sndbufsz)) {
    LOG_ERROR("LocalUdpServer.StartListening", "Unable to set send buffer size (%s)", strerror(errno));
    close(_socket);
    _socket = -1;
    return false;
  }

  if (!Anki::Messaging::SetRecvBufferSize(_socket, _rcvbufsz)) {
    LOG_ERROR("LocalUdpServer.StartListening", "Unable to set recv buffer size (%s)", strerror(errno));
    close(_socket);
    _socket = -1;
    return false;
  }

  // Remove any existing socket using this name
  unlink(_sockname.c_str());

  struct sockaddr_un saddr;
  memset(&saddr, 0, sizeof(saddr));
  saddr.sun_family = sock_family;
  strncpy(saddr.sun_path, _sockname.c_str(), sizeof(saddr.sun_path));
  const socklen_t socklen = (socklen_t) SUN_LEN(&saddr);

  const int status = bind(_socket, (const struct sockaddr*) &saddr, socklen);
  if (status == -1) {
    LOG_ERROR("LocalUdpServer.StartListening", "Unable to bind at %s (%s)", _sockname.c_str(), strerror(errno));
    LOG_ERROR("LocalUdpServer.StartListening", "You might have orphaned processes running");
    close(_socket);
    _socket = -1;
    return false;
  }

  LOG_DEBUG("LocalUdpServer.StartListening", "Socket %d is bound at %s", _socket, _sockname.c_str());

  return true;

}

void LocalUdpServer::StopListening()
{
  if (_socket < 0) {
    LOG_DEBUG("LocalUdpServer.StopListening", "Server already stopped");
    return;
  }

  LOG_DEBUG("LocalUdpServer.StopListening", 
            "Stopping server listening on socket %s (%d)", 
            _sockname.c_str(), _socket);

  if (HasClient()) {
    Disconnect();
  }

  if (close(_socket) < 0) {
    LOG_ERROR("LocalUdpServer.StopListening.Fail", 
              "Error closing socket %s (sock: %d) (%s)", 
              _sockname.c_str(), _socket, strerror(errno));
  }
  _socket = -1;
}


ssize_t LocalUdpServer::Send(const char* data, size_t size)
{
  if (size <= 0) {
    return 0;
  }

  if (!HasClient()) {
    LOG_DEBUG("LocalUdpServer.Send", "No client");
    return -1;
  }

  //LOG_DEBUG("LocalUdpServer.Send", "Sending %zu bytes to %s", size, _peername.c_str());
  ssize_t bytes_sent = 0;
  if (_bindClients) {
    bytes_sent = send(_socket, data, size, 0);
  }
  else {
    const socklen_t socklen = (socklen_t) SUN_LEN(&_client);
    bytes_sent = sendto(_socket, data, size, 0, (sockaddr*)&_client, socklen);
  }

  if (bytes_sent != size) {
    // If send fails, log it and report it to caller.  It is caller's responsibility to retry at
    // some appropriate interval.
    LOG_ERROR("LocalUdpServer.Send.Fail", 
              "Sent %zd bytes instead of %zu on %s (sock: %d) (%s)", 
              bytes_sent, size, _sockname.c_str(), _socket, strerror(errno));
  }

  return bytes_sent;

}

ssize_t LocalUdpServer::Recv(char* data, size_t maxSize)
{
  struct sockaddr_un saddr;
  socklen_t saddrlen = sizeof(saddr);

  const ssize_t bytes_received = recvfrom(_socket, data, maxSize, 0, (struct sockaddr *)&saddr, &saddrlen);

  if (bytes_received <= 0) {
    if (errno == EWOULDBLOCK) {
      //LOG_DEBUG("LocalUdpServer.Recv", "No data available");
      return 0;
    } else {
      LOG_ERROR("LocalUdpServer.Recv.Fail", 
                "Receive error on %s (sock: %d) (%s)", 
                _sockname.c_str(), _socket, strerror(errno));
      return -1;
    }
  }

  //LOG_DEBUG("LocalUdpServer.Recv", "Received %zd bytes from %s", bytes_received, to_string(saddr, saddrlen).c_str());

  // Connect to new client?
  if (!HasClient() || !_bindClients) {
    if (AddClient(saddr, saddrlen)) {
      LOG_DEBUG("LocalUdpServer.Recv.NewClient", "");
    }
  }

  // Check if this is a connection packet

  if (bytes_received == sizeof(kConnectionPacket) &&
      strncmp(data, kConnectionPacket, sizeof(kConnectionPacket)) == 0)  {
    LOG_DEBUG("LocalUdpServer.Recv.ReceivedConnectionPacket", "");
    return 0;
  }

  return bytes_received;

}

bool LocalUdpServer::AddClient(const struct sockaddr_un &saddr, socklen_t saddrlen)
{
  const std::string & peername = to_string(saddr, saddrlen);
  if (_bindClients) {
    LOG_DEBUG("LocalUdpServer.AddClient", "Adding client %s", peername.c_str());

    if (connect(_socket, (struct sockaddr *) &saddr, saddrlen) != 0) {
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

  LOG_DEBUG("LocalUdpServer.Disconnect", "Disconnect %d from peer %s", _socket, _peername.c_str());

  if (_bindClients) {
    // Undo effects of connect() by resetting peer to an unspecified address
    struct sockaddr saddr;
    saddr.sa_family = AF_UNSPEC;
    if (connect(_socket, &saddr, sizeof(saddr)) != 0) {
      // MacOS returns ENOENT but operation has desired effect regardless.
      if (errno != ENOENT) {
        LOG_ERROR("LocalUdpServer.Disconnect", "Failed to disconnect (%s)", strerror(errno));
      }
    }
  }

  _peername.clear();
}

ssize_t LocalUdpServer::GetIncomingSize() const
{
  if (_socket >= 0) {
    return Anki::Messaging::GetIncomingSize(_socket);
  }
  return -1;
}

ssize_t LocalUdpServer::GetOutgoingSize() const
{
  if (_socket >= 0) {
    return Anki::Messaging::GetOutgoingSize(_socket);
  }
  return -1;
}
