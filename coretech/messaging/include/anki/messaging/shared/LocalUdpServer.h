
#ifndef ANKI_MESSAGING_LOCAL_UDP_SERVER_H
#define ANKI_MESSAGING_LOCAL_UDP_SERVER_H

/**
 *
 * File: LocalUdpServer.h
 *
 * Description: Declaration of local-domain socket server class
 *
 * Copyright: Anki, inc. 2017
 *
 */

#include <string>
#include <vector>
#include <sys/socket.h>
#include <sys/un.h>

class LocalUdpServer {
public:
  LocalUdpServer();
  ~LocalUdpServer();

  // Socket lifetime
  bool StartListening(const std::string & sockname);
  void StopListening();

  // Client management
  bool HasClient() const;
  int GetNumClients() const;
  void Disconnect();

  // Client transport
  ssize_t Send(const char* data, int size);
  ssize_t Recv(char* data, int maxSize);
  
private:
  typedef std::vector<struct sockaddr_un> ClientList;
  typedef ClientList::iterator ClientListIter;

  // Listening socket descriptor
  int _socketfd;

  // Socket name
  std::string _sockname;

  // List of known clients
  ClientList _clients;

  // Returns true if client successfully added
  bool AddClient(const struct sockaddr_un &saddr, socklen_t saddrlen);

};

#endif
