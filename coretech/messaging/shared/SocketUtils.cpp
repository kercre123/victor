/**
 * File: SocketUtils.cpp
 *
 * Description: Implementation of coretech socket utilities
 *
 * Copyright: Anki, inc. 2017
 *
 */

#include "coretech/messaging/shared/SocketUtils.h"

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
  int incoming = 0;
  if (ioctl(socket, FIONREAD, &incoming) != 0) {
    return -1;
  }
  return incoming;
}

int GetOutgoingSize(int socket)
{
  int outgoing = 0;
  if (ioctl(socket, TIOCOUTQ, &outgoing) != 0) {
    return -1;
  }
  return outgoing;
}

} // end namespace Messaging
} // end namespace Anki
