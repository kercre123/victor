#ifndef UDP_CLIENT_H
#define UDP_CLIENT_H

#include <sys/types.h> // ssize_t

#define DEBUG_UDP_CLIENT(__expr__)
//#define DEBUG_UDP_CLIENT(__expr__) (std::cout << __expr__ << std::endl)


class UdpClient {
public:
  UdpClient();
  ~UdpClient();

  bool Connect(const char *host_address, const unsigned short port);
  bool Disconnect();
  bool IsConnected() const { return socketfd >= 0; }
  ssize_t Send(const char* data, int size);
  ssize_t Recv(char* data, int maxSize);
  
  int GetSocketFd() const { return socketfd; }

private:
  struct addrinfo *host_info_list; // Pointer to the to the linked list of host_info's.

  void set_nonblock(int socket);
  int socketfd; // The socket descripter
};

#endif
