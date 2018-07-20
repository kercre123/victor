#ifndef ANKI_MESSAGING_LOCAL_STREAMING_CLIENT_H
#define ANKI_MESSAGING_LOCAL_STREAMING_CLIENT_H

/**
 *
 * File: LocalStreamingClient.h
 *
 * Description: Declaration of local-domain socket client class
 *
 * Copyright: Anki, Inc. 2018
 *
 */
#include <string>

#include <sys/socket.h>
#include <sys/un.h>

class LocalStreamingClient {
public:
  LocalStreamingClient();
  ~LocalStreamingClient();

  bool Connect(const std::string& sockname);
  bool IsConnected() const { return _socketfd >= 0; }
  bool Disconnect();

  ssize_t Send(const char* data, int size);
  ssize_t Recv(char* data, int maxSize);
  
private:

  // Socket descriptor
  int _socketfd;

  // Socket names
  // std::string _sockname;
  // std::string _peername;

  // Socket addresses
  // struct sockaddr_un _sockaddr;
  // socklen_t _sockaddr_len;

  // struct sockaddr_un _peeraddr;
  // socklen_t _peeraddr_len;

};

#endif // ANKI_MESSAGING_LOCAL_STREAMING_CLIENT_H
