
#ifndef ANKI_MESSAGING_SOCKETUTILS_H
#define ANKI_MESSAGING_SOCKETUTILS_H

/**
 * File: SocketUtils.h
 *
 * Description: Declaration of socket utility fns
 *
 * Copyright: Anki, inc. 2017
 *
 */

#include <sys/un.h>

namespace Anki {
namespace Messaging {

//
// Returns length of sockaddr_un that has been initialized with null-terminated path.
// Compatible with SUN_LEN defined in <sys/un.h> on some (but not all) platforms.
//
#if !defined(SUN_LEN)
#define SUN_LEN(su) (sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path))
#endif

// Set non-blocking I/O flag, aka O_NONBLOCK
// Returns true on success, false on error
bool SetNonBlocking(int socket, int enable);

// Set reuse address flag, aka SO_REUSEADDR
// Returns true on success, false on error
bool SetReuseAddress(int socket, int enable);

// Set outgoing buffer size, aka SO_SNDBUF
// Returns true on success, false on error
bool SetSendBufferSize(int socket, int sndbufsz);

// Set incoming buffer size, aka SO_RCVBUF
// Returns true on success, false on error
bool SetRecvBufferSize(int socket, int rcvbufsz);

// Returns number of bytes queued for read on socket or -1 on error
int GetIncomingSize(int socket);

// Returns number of bytes queued for write on socket or -1 on error
int GetOutgoingSize(int socket);

} // end namespace Messaging
} // end namespace Anki

#endif
