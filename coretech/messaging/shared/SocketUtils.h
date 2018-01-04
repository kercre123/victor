
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

bool SetNonBlocking(int socket, int enable);

bool SetReuseAddress(int socket, int enable);

bool SetSendBufferSize(int socket, int sndbufsz);

bool SetRecvBufferSize(int socket, int rcvbufsz);

} // end namespace Messaging
} // end namespace Anki

#endif
