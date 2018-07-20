/**
 * File: LocalStreamingClient.cpp
 *
 * Description: Implementation of local-domain socket client class
 *
 * Copyright: Anki, Inc. 2018
 *
 */

#include "coretech/messaging/shared/LocalStreamingClient.h"
#include "coretech/messaging/shared/SocketUtils.h"
#include "coretech/common/shared/logging.h"

#include <iostream>

#include <assert.h>
#include <unistd.h>

// Socket buffer sizes
#define UDP_CLIENT_SNDBUFSZ (256*1024)
#define UDP_CLIENT_RCVBUFSZ (256*1024)

// Define this to enable logs
#define LOG_CHANNEL                    "LocalStreamingClient"

#ifdef VICOS
// todo: restore logging when vicos toolchain is available -PRA (approved by BRC)
#undef LOG_CHANNEL
#endif

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

LocalStreamingClient::LocalStreamingClient() :
  _socketfd(-1)
{
}

LocalStreamingClient::~LocalStreamingClient()
{
  Disconnect();
}

bool LocalStreamingClient::Connect(const std::string& sockname)
{
  //LOG_DEBUG("LocalStreamingClient.Connect", "Connect from %s to %s", sockname.c_str(), peername.c_str());

  if (_socketfd >= 0) {
    LOG_ERROR("LocalStreamingClient.Connect", "Already connected");
    return false;
  }

  const int ai_family = AF_UNIX;
  const int ai_socktype = SOCK_STREAM;
  const int ai_protocol = 0;

  _socketfd = socket(ai_family, ai_socktype, ai_protocol);
  if (_socketfd < 0) {
    LOG_ERROR("LocalStreamingClient.Connect", "Unable to create socket (%s)", strerror(errno));
    return false;
  }

  if (!Anki::Messaging::SetReuseAddress(_socketfd, 1)) {
    LOG_ERROR("LocalStreamingClient.Connect", "Unable to set reuseaddress (%s)", strerror(errno));
    close(_socketfd);
    _socketfd = -1;
    return false;
  }

  if (!Anki::Messaging::SetNonBlocking(_socketfd, 1)) {
    LOG_ERROR("LocalStreamingClient.Connect", "Unable to set nonblocking (%s)", strerror(errno));
    close(_socketfd);
    _socketfd = -1;
    return false;
  }

  if (!Anki::Messaging::SetSendBufferSize(_socketfd, UDP_CLIENT_SNDBUFSZ)) {
    LOG_ERROR("LocalStreamingClient.Connect", "Unable to set send buffer size (%s)", strerror(errno));
    close(_socketfd);
    _socketfd = -1;
    return false;
  }

  if (!Anki::Messaging::SetRecvBufferSize(_socketfd, UDP_CLIENT_RCVBUFSZ)) {
    LOG_ERROR("LocalStreamingClient.Connect", "Unable to set recv buffer size (%s)", strerror(errno));
    close(_socketfd);
    _socketfd = -1;
    return false;
  }

  // _sockname = sockname;
  // _peername = peername;

  // Remove any existing socket using this name
  // unlink(_sockname.c_str());

  struct sockaddr_un saddr;
  memset(&saddr, 0, sizeof(saddr));
  saddr.sun_family = ai_family;
  strncpy(saddr.sun_path, sockname.c_str(), sizeof(saddr.sun_path));
  const socklen_t socklen = (socklen_t) SUN_LEN(&saddr);
  // Bind to socket name
  // memset(&_sockaddr, 0, sizeof(_sockaddr));
  // _sockaddr.sun_family = ai_family;
  // strncpy(_sockaddr.sun_path, sockname.c_str(), sizeof(_sockaddr.sun_path));
  // _sockaddr_len = (socklen_t) SUN_LEN(&_sockaddr);

  // if (bind(_socketfd, (struct sockaddr *) &saddr, socklen) != 0) {
  //   LOG_ERROR("LocalStreamingClient.Connect", "Unable to bind socket (%s)", strerror(errno));
  //   close(_socketfd);
  //   _socketfd = -1;
  //   return false;
  // }

  // // Connect to peer name
  // memset(&_peeraddr, 0, sizeof(_peeraddr));
  // _peeraddr.sun_family = ai_family;
  // strncpy(_peeraddr.sun_path, _peername.c_str(), sizeof(_peeraddr.sun_path));
  // _peeraddr_len = (socklen_t) SUN_LEN(&_peeraddr);

  if (connect(_socketfd, (struct sockaddr *) &saddr, socklen) != 0) {
    LOG_ERROR("LocalStreamingClient.Connect", "Unable to connect to %s (%s)", sockname.c_str(),  strerror(errno));
    close(_socketfd);
    _socketfd = -1;
    return false;
  }

  // Send connection packet (i.e. something so that the server adds us to the client list)
  // const char zero = 0;
  // Send(&zero, 1);

  return true;
}

bool LocalStreamingClient::Disconnect()
{
  if (_socketfd > -1) {
    if (close(_socketfd) < 0) {
      LOG_ERROR("LocalStreamingClient.Disconnect", "Error closing socket (%s)", strerror(errno));
    };
    _socketfd = -1;
  }
  return true;
}

ssize_t LocalStreamingClient::Send(const char* data, int size)
{
  if (_socketfd < 0) {
    LOG_ERROR("LocalStreamingClient.Send", "Socket undefined, skipping send");
    return 0;
  }

  //LOG_DEBUG("LocalStreamingClient.Send", "Sending %d bytes", size);

  const ssize_t bytes_sent = send(_socketfd, data, size, 0);

  if (bytes_sent != size) {
    LOG_ERROR("LocalStreamingClient.Send", "Send error, disconnecting (%s)", strerror(errno));
    Disconnect();
    return -1;
  }

  return bytes_sent;
}

ssize_t LocalStreamingClient::Recv(char* data, int maxSize)
{
  assert(data != NULL);

  if (_socketfd < 0) {
    LOG_ERROR("LocalStreamingClient.Recv", "Socket undefined, skipping recv");
    return 0;
  }

  const ssize_t bytes_received = recv(_socketfd, data, maxSize, 0);

  if (bytes_received <= 0) {
    if (errno == EWOULDBLOCK || errno == EAGAIN) {
      //LOG_DEBUG("LocalStreamingClient.Recv", "No data available");
      return 0;
    } else {
      LOG_ERROR("LocalStreamingClient.Recv", "Receive error, dropping connection (%s)", strerror(errno));
      Disconnect();
      return -1;
    }
  }

  //LOG_DEBUG("LocalStreamingClient.Recv", "Received %d bytes", bytes_received);
  return bytes_received;
}
