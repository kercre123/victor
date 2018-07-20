
#ifndef ANKI_MESSAGING_LOCAL_STREAMING_SERVER_H
#define ANKI_MESSAGING_LOCAL_STREAMING_SERVER_H

/**
 *
 * File: LocalStreamingServer.h
 *
 * Description: Declaration of local-domain socket server class
 *
 * Current implementation is limited to ONE CLIENT PER SERVER for improved performance.
 * Profiling shows that connect()+send() to a single peer is much faster than using sendto()
 * to manage multiple peers.
 *
 * Copyright: Anki, Inc. 2018
 *
 */

#include "util/container/fixedCircularBuffer.h"

#include <string>
#include <vector>
#include <sys/socket.h>
#include <sys/un.h>

// Don't make capacity size 0. That's undefined behavior for the implementation of Util::FixedCircularBuffer
template <size_t kCapacity = 1>
class LocalStreamingServer {
public:
  LocalStreamingServer();
  ~LocalStreamingServer();

  // Socket lifetime
  bool StartListening(const std::string & sockname);
  void StopListening();

  // Client management
  bool HasClient() const { return _clientfd != -1; }
  void Disconnect();
  // void SetBindClients(bool value) { _bindClients = value; }

  // Client transport
  ssize_t Send(const char* data, int size);
  ssize_t Recv(char* data, int maxSize);

  // E.g. ssize_t CheckMessageCompleteCallback(const Util::FixedCircularBuffer<char, kCapacity>& buffer);
  // Returns -1 for error, 0 for not yet complete, or positive number for number of bytes in the next complete message.
  // Logically, this means the second param passed in (size of bytes to check) will always be greater than or equal to the return value.
  typedef ssize_t (*CheckMessageCompleteCallback)(const Util::FixedCircularBuffer<char, kCapacity>&);

  // If our callback decides on a message that's too big to fit into the buffer provided by the call to Recv(), we error and disconnect
  // If the callback keeps saying the message isn't complete and we end up filling the buffer we have internally, we error and disconnect

  // (optional) Set a place where incomplete messages can be stored
  void SetupMessageCompletenessCheck(CheckMessageCompleteCallback callback);

private:
  // Listening socket descriptor
  int _socketfd;
  int _clientfd;

  // Socket names
  std::string _sockname;

  CheckMessageCompleteCallback _messageCallback;
  Util::FixedCircularBuffer<char, kCapacity> _messageBuffer;

  // sockaddr_un _client;

  // Returns true if client successfully added
  // bool AddClient(const struct sockaddr_un &saddr, socklen_t saddrlen);

};

#endif // ANKI_MESSAGING_LOCAL_STREAMING_SERVER_H
