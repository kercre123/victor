/**
 * File: LocalUdpClient.cpp
 *
 * Description: Implementation of local-domain socket client class
 *
 * Copyright: Anki, inc. 2017
 *
 */

#include "anki/messaging/shared/LocalUdpClient.h"
#include "anki/messaging/shared/SocketUtils.h"

#include <iostream>

#include <assert.h>
#include <unistd.h>

// Socket buffer sizes
#define UDP_CLIENT_SNDBUFSZ (128*1024)
#define UDP_CLIENT_RCVBUFSZ (128*1024)

//
// This code is shared with robot so we don't have PRINT_NAMED macros available.
//
#define DEBUG_UDP_CLIENT

#ifdef DEBUG_UDP_CLIENT
#define LOG_ERROR(__expr__) (std::cerr << "[Error] " << __expr__ << std::endl)
#define LOG_INFO(__expr__)  (std::cout << "[Info] " << __expr__ << std::endl)
#define LOG_DEBUG(__expr__) (std::cout << "[Debug] " << __expr__ << std::endl)
#else
#define LOG_ERROR(__expr__) (std::cerr << "[Error] " << __expr__ << std::endl)
#define LOG_INFO(__expr__)  {}
#define LOG_DEBUG(__expr__) {}
#endif

LocalUdpClient::LocalUdpClient() :
  _socketfd(-1)
{
}

LocalUdpClient::~LocalUdpClient()
{
  Disconnect();
}

bool LocalUdpClient::Connect(const std::string& sockname, const std::string & peername)
{
  LOG_INFO("LocalUdpClient.Connect: Connect from " << sockname << " to " << peername);

  if (_socketfd >= 0) {
    LOG_ERROR("LocalUdpClient.Connect: Already connected");
    return false;
  }

  const int ai_family = AF_LOCAL;
  const int ai_socktype = SOCK_DGRAM;
  const int ai_protocol = 0;

  _socketfd = socket(ai_family, ai_socktype, ai_protocol);
  if (_socketfd < 0) {
    LOG_ERROR("LocalUdpClient.Connect: Unable to create socket (" << strerror(errno) << ")");
    return false;
  }

  if (!Anki::Messaging::SetReuseAddress(_socketfd, 1)) {
    LOG_ERROR("LocalUdpClient.Connect: Unable to set reuseaddress (" << strerror(errno) << ")");
    return false;
  }

  if (!Anki::Messaging::SetNonBlocking(_socketfd, 1)) {
    LOG_ERROR("LocalUdpClient.Connect: Unable to set nonblocking (" << strerror(errno) << ")");
    return false;
  }

  if (!Anki::Messaging::SetSendBufferSize(_socketfd, UDP_CLIENT_SNDBUFSZ)) {
    LOG_ERROR("LocalUdpClient.Connect: Unable to set send buffer size (" << strerror(errno) << ")");
    return false;
  }

  if (!Anki::Messaging::SetRecvBufferSize(_socketfd, UDP_CLIENT_RCVBUFSZ)) {
    LOG_ERROR("LocalUdpClient.Connect: Unable to set recv buffer size (" << strerror(errno) << ")");
    return false;
  }

  _sockname = sockname;
  _peername = peername;

  // Remove any existing socket using this name
  unlink(_sockname.c_str());
  
  // Bind to socket name
  memset(&_sockaddr, 0, sizeof(_sockaddr));
  _sockaddr.sun_family = ai_family;
  strncpy(_sockaddr.sun_path, _sockname.c_str(), sizeof(_sockaddr.sun_path));
  _sockaddr_len = (socklen_t) SUN_LEN(&_sockaddr);

  if (bind(_socketfd, (struct sockaddr *) &_sockaddr, _sockaddr_len)) {
    LOG_ERROR("LocalUdpClient.Connect: Unable to bind socket (" << strerror(errno) << ")");
    return false;
  }

  // Connect to peer name
  memset(&_peeraddr, 0, sizeof(_peeraddr));
  _peeraddr.sun_family = ai_family;
  strncpy(_peeraddr.sun_path, _peername.c_str(), sizeof(_peeraddr.sun_path));
  _peeraddr_len = (socklen_t) SUN_LEN(&_peeraddr);

  // Send connection packet (i.e. something so that the server adds us to the client list)
  const char zero = 0;
  Send(&zero, 1);
  
  return true;
}

bool LocalUdpClient::Disconnect()
{
  if (_socketfd > -1) {
    if (close(_socketfd) < 0) {
      LOG_ERROR("LocalUdpClient.Disconnect: Error closing socket (" << strerror(errno) << ")");
    };
    _socketfd = -1;
  }
  return true;
}

ssize_t LocalUdpClient::Send(const char* data, int size)
{
  if (_socketfd < 0) {
    LOG_ERROR("LocalUdpClient.Send: Socket undefined, skipping Send");
    return 0;
  }
  
  //LOG_DEBUG("LocalUdpClient.Send: sending " << size << " bytes");

  const ssize_t bytes_sent = sendto(_socketfd, data, size, 0, (const struct sockaddr *) &_peeraddr, _peeraddr_len);

  if (bytes_sent != size) {
    LOG_ERROR("LocalUdpClient.Send: Send error, disconnecting (" << strerror(errno) << ")");
    Disconnect();
    return -1;
  }

  return bytes_sent;
}

ssize_t LocalUdpClient::Recv(char* data, int maxSize)
{
  assert(data != NULL);

  if (_socketfd < 0) {
    LOG_ERROR("LocalUdpClient.Recv: Socket undefined, skipping recv");
    return 0;
  }

  const ssize_t bytes_received = recv(_socketfd, data, maxSize, 0);
  
  if (bytes_received <= 0) {
    if (errno == EWOULDBLOCK) {
      //LOG_DEBUG("LocalUdpClient.Recv: No data available");
      return 0;
    } else {
      LOG_ERROR("LocalUdpClient.Recv: Receive error, dropping connection (" << strerror(errno) << ")");
      Disconnect();
      return -1;
    }
  }
  
  //LOG_DEBUG("LocalUdpClient.Recv: received " << bytes_received << " bytes");
  return bytes_received;
}
