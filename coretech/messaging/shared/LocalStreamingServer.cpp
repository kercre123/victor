/**
 * File: LocalStreamingServer.cpp
 *
 * Description: Implementation of local-domain socket server class
 *
 * Copyright: Anki, inc. 2017
 *
 */

#include "coretech/messaging/shared/LocalStreamingServer.h"
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
#define LOG_CHANNEL                    "LocalStreamingServer"

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

// static std::string to_string(const struct sockaddr_un & saddr, socklen_t saddrlen)
// {
//   return std::string(saddr.sun_path, saddrlen - (sizeof(saddr) - sizeof(saddr.sun_path)));
// }

LocalStreamingServer::LocalStreamingServer()
{
  _socketfd = -1;
  _clientfd = -1;

  _messageCallback = nullptr;
  _messageBuffer = nullptr;
  _messageBufferSizeMax = 0;
  _messageBufferSizeCurrent = 0;
}

LocalStreamingServer::~LocalStreamingServer()
{
  if (_socketfd >= 0) {
    StopListening();
  }
}

bool LocalStreamingServer::StartListening(const std::string & sockname)
{
  if (_socketfd >= 0) {
    LOG_ERROR("LocalStreamingServer.StartListening", "Server is already listening");
    return false;
  }

  _sockname = sockname;

  LOG_DEBUG("LocalStreamingServer.StartListening", "Creating a socket at %s", _sockname.c_str());

  // Use AF_UNIX for UNIX domain socket
  // Use SOCK_STREAM streaming io
  const int sock_family = AF_UNIX;
  const int sock_type = SOCK_STREAM;

  _socketfd = socket(sock_family, sock_type, 0);
  if (_socketfd == -1) {
    LOG_ERROR("LocalStreamingServer.StartListening", "Unable to create socket at %s (%s)", _sockname.c_str(), strerror(errno));
    return false;
  }

  if (!Anki::Messaging::SetNonBlocking(_socketfd, 1)) {
    LOG_ERROR("LocalStreamingServer.StartListening", "Unable to set nonblocking (%s)", strerror(errno));
    close(_socketfd);
    _socketfd = -1;
    return false;
  }

  if (!Anki::Messaging::SetReuseAddress(_socketfd, 1)) {
    LOG_ERROR("LocalStreamingServer.StartListening", "Unable to set reuseaddress (%s)", strerror(errno));
    close(_socketfd);
    _socketfd = -1;
    return false;
  }

  if (!Anki::Messaging::SetSendBufferSize(_socketfd, UDP_SERVER_SNDBUFSZ)) {
    LOG_ERROR("LocalStreamingServer.StartListening", "Unable to set send buffer size (%s)", strerror(errno));
    close(_socketfd);
    _socketfd = -1;
    return false;
  }

  if (!Anki::Messaging::SetRecvBufferSize(_socketfd, UDP_SERVER_RCVBUFSZ)) {
    LOG_ERROR("LocalStreamingServer.StartListening", "Unable to set recv buffer size (%s)", strerror(errno));
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

  const int bindStatus = bind(_socketfd, (const struct sockaddr*) &saddr, socklen);
  if (bindStatus == -1) {
    LOG_ERROR("LocalStreamingServer.StartListening", "Unable to bind at %s (%s)", _sockname.c_str(), strerror(errno));
    LOG_ERROR("LocalStreamingServer.StartListening", "You might have orphaned processes running");
    close(_socketfd);
    _socketfd = -1;
    return false;
  }

  constexpr int numBacklogConnectionsAllowed = 1;
  const int listenStatus = listen(_socketfd, numBacklogConnectionsAllowed);
  if (listenStatus == -1) {
    LOG_ERROR("LocalStreamingServer.StartListening", "Unable to listen at %s (%s)", _sockname.c_str(), strerror(errno));
    close(_socketfd);
    _socketfd = -1;
    return false;
}

  LOG_DEBUG("LocalStreamingServer.StartListening", "Socket is bound at %s", _sockname.c_str());
  return true;
}


void LocalStreamingServer::StopListening()
{
  if (_socketfd < 0) {
    LOG_DEBUG("LocalStreamingServer.StopListening", "Server already stopped");
    return;
  }

  LOG_DEBUG("LocalStreamingServer.StopListening", "Stopping server listening on socket %d", _socketfd);

  if (HasClient()) {
    Disconnect();
  }

  if (close(_socketfd) < 0) {
    LOG_ERROR("LocalStreamingServer.StopListening", "Error closing socket (%s)", strerror(errno));
  }
  _socketfd = -1;
}


ssize_t LocalStreamingServer::Send(const char* data, int size)
{
  if (size <= 0) {
    return 0;
  }

  if (!HasClient()) {
    LOG_DEBUG("LocalStreamingServer.Send", "No client");
    return -1;
  }

  //LOG_DEBUG("LocalStreamingServer.Send", "Sending %d bytes to %s", size, _peername.c_str());
  ssize_t bytes_sent = send(_clientfd, data, size, 0);

  if (bytes_sent != size) {
    // If send fails, log it and report it to caller.  It is caller's responsibility to retry at
    // some appropriate interval.
    LOG_ERROR("LocalStreamingServer.Send", "Sent %zd bytes instead of %d (%s)", bytes_sent, size, strerror(errno));
  }

  return bytes_sent;
}


ssize_t LocalStreamingServer::Recv(char* data, int maxSize)
{
  // Connect to new client?
  if (!HasClient()) {
    int newClient = accept(_socketfd, NULL, NULL);
    if (newClient == -1) {
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        //LOG_DEBUG("LocalStreamingServer.Recv", "No data available");
        return 0;
      } else {
        LOG_ERROR("LocalStreamingServer.Recv", "Receive error (%s)", strerror(errno));
        return -1;
      }
    }
    LOG_INFO("LocalStreamingServer.Recv.NewClient", "%d", newClient);
    _clientfd = newClient;
  }

  if (nullptr != _messageCallback)
  {
    auto spaceAvailable = _messageBuffer.capacity() - _messageBuffer.size();
    constexpr int flags = 0;
    const ssize_t bytes_received = recv(_clientfd, data, maxSize, flags);
    if (bytes_received <= 0) {
      if (errno == EWOULDBLOCK || errno == EAGAIN) {
        //LOG_DEBUG("LocalStreamingServer.Recv", "No data available");
        return 0;
      } else {
        LOG_ERROR("LocalStreamingServer.Recv", "Receive error (%s)", strerror(errno));
        return -1;
      }
    }

    if (bytes_received > spaceAvailable)
    {
      LOG_ERROR("LocalStreamingServer.Recv",
                "Buffer size %zu too small for %zu bytes received",
                spaceAvailable,
                bytes_received);
      Disconnect();
      return -1;
    }

    // Returns -1 for error, 0 for not yet complete, or positive number for number of bytes in the next complete message.
    // Logically, this means the second param passed in (size of bytes to check) will always be greater than or equal to the return value.
    // typedef ssize_t (*CheckMessageCompleteCallback)(const char*, size_t);
    auto bytesComplete = _messageCallback()

  }
  _messageBufferSizeCurrent = 0;
  _messageCallback = callback;
  _messageBuffer = buffer;
  _messageBufferSizeMax = bufferSize;

  constexpr int flags = 0;
  const ssize_t bytes_received = recv(_clientfd, data, maxSize, flags);
  if (bytes_received <= 0) {
    if (errno == EWOULDBLOCK || errno == EAGAIN) {
      //LOG_DEBUG("LocalStreamingServer.Recv", "No data available");
      return 0;
    } else {
      LOG_ERROR("LocalStreamingServer.Recv", "Receive error (%s)", strerror(errno));
      return -1;
    }
  }

  //LOG_DEBUG("LocalStreamingServer.Recv", "Received %zd bytes from %s", bytes_received, to_string(saddr, saddrlen).c_str());
  return bytes_received;
}


void LocalStreamingServer::Disconnect()
{
  if (!HasClient()) {
    return;
  }

  LOG_DEBUG("LocalStreamingServer.Disconnect", "Disconnect from peer");

  if (close(_clientfd) != 0) {
    LOG_ERROR("LocalStreamingServer.Disconnect", "Disconnect error (%s)", strerror(errno));
  }

  _messageBuffer.clear();
  _clientfd = -1;
}

void LocalStreamingServer::SetupMessageCompletenessCheck(LocalStreamingServer::CheckMessageCompleteCallback callback)
{
  Disconnect();
  _messageCallback = callback;
}
