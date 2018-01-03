#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#define DEBUG_TCP_CLIENT(__expr__)
//#define DEBUG_TCP_CLIENT(__expr__) (std::cout << __expr__ << std::endl)



class TcpClient {
public:
  TcpClient();
  ~TcpClient();

  bool Connect(const char *host_address, const unsigned short port);
  bool Disconnect();
  int Send(const char* data, int size);
  int Recv(char* data, int maxSize);
  bool IsConnected();

private:
  struct addrinfo *host_info_list; // Pointer to the to the linked list of host_info's.

  void set_nonblock(int socket);
  int socketfd; // The socket descriptor
};

#endif
