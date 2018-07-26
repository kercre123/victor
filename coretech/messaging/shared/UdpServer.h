// Simple UDP server that "accepts" as clients anyone that sends a datagram to it.
// Send()s go to all clients.

#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <netinet/in.h>
#include <string>
#include <vector>

class UdpServer {
public:
  static constexpr const char kConnectionPacket[] = {'A', 'N', 'K', 'I', 'C', 'O', 'N', 'N'};

  UdpServer(const std::string& name = "");
  ~UdpServer();

  bool StartListening(const unsigned short port);
  void StopListening();

  bool HasClient();
  void DisconnectClient();
  
  int GetNumClients();

  ssize_t Send(const char* data, size_t size);
  ssize_t Recv(char* data, size_t maxSize);
  
private:

  std::string _name;

  //struct addrinfo *host_info_list; // Pointer to the to the linked list of host_info's.
  struct sockaddr_in cliaddr;

  void set_nonblock(int socket);
  
  // Returns true if client successfully added
  bool AddClient(struct sockaddr_in &c);

  typedef std::vector<struct sockaddr_in>::iterator client_list_it;
  std::vector<struct sockaddr_in> client_list;

  int socketfd; // Listening socket descriptor
};

#endif
