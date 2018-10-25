
#ifndef ANKI_MESSAGING_LOCAL_UDP_SERVER_H
#define ANKI_MESSAGING_LOCAL_UDP_SERVER_H

/**
 *
 * File: LocalUdpServer.h
 *
 * Description: Declaration of local-domain socket server class
 *
 * Current implementation is limited to ONE CLIENT PER SERVER for improved performance.
 * Profiling shows that connect()+send() to a single peer is much faster than using sendto()
 * to manage multiple peers.
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
  static constexpr const char kConnectionPacket[] = {'A', 'N', 'K', 'I', 'C', 'O', 'N', 'N'};

  LocalUdpServer(int sndbufsz, int rcvbufsz);
  LocalUdpServer();

  ~LocalUdpServer();

  // Socket lifetime
  bool StartListening(const std::string & sockname);
  void StopListening();

  // Client management
  bool HasClient() const { return !_peername.empty(); }
  void Disconnect();
  void SetBindClients(bool value) { _bindClients = value; }

  // Client transport
  ssize_t Send(const char* data, size_t size);
  ssize_t Recv(char* data, size_t maxSize);

  int GetSocket() const { return _socket; }

  // Return count of bytes queued for read or -1 on error
  ssize_t GetIncomingSize() const;

  // Return count of bytes queued for write or -1 on error
  ssize_t GetOutgoingSize() const;

private:
  // Socket parameters
  int _sndbufsz;
  int _rcvbufsz;

  // Listening socket descriptor
  int _socket;

  // Socket names
  std::string _sockname;
  std::string _peername;

  bool _bindClients;
  sockaddr_un _client;

  // Returns true if client successfully added
  bool AddClient(const struct sockaddr_un &saddr, socklen_t saddrlen);

};

#endif
