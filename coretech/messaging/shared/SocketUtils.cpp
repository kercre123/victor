/**
 * File: SocketUtils.cpp
 *
 * Description: Implementation of coretech socket utilities
 *
 * Copyright: Anki, inc. 2017
 *
 */

#include "coretech/messaging/shared/SocketUtils.h"
#include "util/helpers/ankiDefines.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

namespace Anki {
namespace Messaging {

bool SetNonBlocking(int socket, int enable)
{
  int flags = fcntl(socket, F_GETFL, 0);
  if (flags == -1) {
    return false;
  }

  if (enable) {
    flags |= O_NONBLOCK;
  } else {
    flags &= ~O_NONBLOCK;
  }

  flags = fcntl(socket, F_SETFL, flags);
  if (flags == -1) {
    return false;
  }
  return true;
}

bool SetReuseAddress(int socket, int enable)
{
  const int status = setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
  return (status != -1);
}

bool SetSendBufferSize(int socket, int sndbufsz)
{
  const int status = setsockopt(socket, SOL_SOCKET, SO_SNDBUF, &sndbufsz, sizeof(sndbufsz));
  return (status != -1);
}

bool SetRecvBufferSize(int socket, int rcvbufsz)
{
  const int status = setsockopt(socket, SOL_SOCKET, SO_RCVBUF, &rcvbufsz, sizeof(rcvbufsz));
  return (status != -1);
}

int GetIncomingSize(int socket)
{
  #if defined(ANKI_PLATFORM_VICOS) || defined(ANKI_PLATFORM_LINUX)
  {
    int incoming = 0;
    if (ioctl(socket, FIONREAD, &incoming) != 0) {
      return -1;
    }
    return incoming;
  }
  #elif defined(ANKI_PLATFORM_OSX)
  {
    int incoming = 0;
    socklen_t incoming_len = sizeof(incoming);
    if (getsockopt(socket, SOL_SOCKET, SO_NREAD, &incoming, &incoming_len) != 0) {
      return -1;
    }
    return incoming;
  }
  #else
  {
    #warning "Unsupported platform"
    return -1;
  }
  #endif
}

int GetOutgoingSize(int socket)
{
  #if defined(ANKI_PLATFORM_VICOS) || defined(ANKI_PLATFORM_LINUX)
  {
    int outgoing = 0;
    if (ioctl(socket, TIOCOUTQ, &outgoing) != 0) {
      return -1;
    }
    return outgoing;
  }
  #elif defined(ANKI_PLATFORM_OSX)
  {
    int outgoing = 0;
    socklen_t outgoing_len = sizeof(outgoing);
    if (getsockopt(socket, SOL_SOCKET, SO_NWRITE, &outgoing, &outgoing_len) != 0) {
      return -1;
    }
    return outgoing;
  }
  #else
  {
    #warning "Unsupported platform"
    return -1;
  }
  #endif
}



} // end namespace Messaging
} // end namespace Anki
