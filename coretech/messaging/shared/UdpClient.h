#ifndef UDP_CLIENT_H
#define UDP_CLIENT_H

#include <string>
#include <sys/types.h> // ssize_t

class UdpClient {
public:
  UdpClient(const std::string& name = "");
  ~UdpClient();

  bool Connect(const char *host_address, const unsigned short port);
  bool Disconnect();
  bool IsConnected() const { return socketfd >= 0; }
  ssize_t Send(const char* data, int size);
  ssize_t Recv(char* data, int maxSize);
  
  int GetSocketFd() const { return socketfd; }

private:
  std::string _name;

  struct addrinfo *host_info_list; // Pointer to the to the linked list of host_info's.

  void set_nonblock(int socket);
  int socketfd; // The socket descriptor
};

#endif
