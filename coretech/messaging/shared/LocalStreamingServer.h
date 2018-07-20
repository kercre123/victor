
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

#include <string>
#include <vector>
#include <sys/socket.h>
#include <sys/un.h>

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

private:

  // Listening socket descriptor
  int _socketfd;
  int _clientfd;

  // Socket names
  std::string _sockname;

  // sockaddr_un _client;

  // Returns true if client successfully added
  // bool AddClient(const struct sockaddr_un &saddr, socklen_t saddrlen);

};

#endif // ANKI_MESSAGING_LOCAL_STREAMING_SERVER_H
