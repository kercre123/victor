// Simple UDP server that "accepts" as clients anyone that sends a datagram to it.
// Send()s go to all clients.

#ifndef UDP_SERVER_H
#define UDP_SERVER_H

#include <netinet/in.h>
#include <vector>

#define DEBUG_UDP_SERVER(__expr__)
//#define DEBUG_UDP_SERVER(__expr__) (std::cout << __expr__ << std::endl)

class UdpServer {
public:
  UdpServer();
  ~UdpServer();

  bool StartListening(const unsigned short port);
  void StopListening();

  bool HasClient();
  void DisconnectClient();
  
  int GetNumClients();

  ssize_t Send(const char* data, int size);
  ssize_t Recv(char* data, int maxSize);
  //int GetNumBytesAvailable();
  
private:
  //struct addrinfo *host_info_list; // Pointer to the to the linked list of host_info's.
  struct sockaddr_in cliaddr;

  void set_nonblock(int socket);
  
  // Returns true if client successfully added
  bool AddClient(struct sockaddr_in &c);

  typedef std::vector<struct sockaddr_in>::iterator client_list_it;
  std::vector<struct sockaddr_in> client_list;

  int socketfd; // Listening socket descripter
};

#endif
