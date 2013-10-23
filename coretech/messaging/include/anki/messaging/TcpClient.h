#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

// To turn on cout debugging
#define DEBUG_TCP_CLIENT 1

class TcpClient {
public:
  TcpClient();
  ~TcpClient();

  bool Connect(const char *host_address, const char* port);
  bool Disconnect();
  int Send(const char* data, int size);
  int Recv(char* data, int maxSize);

private:
  struct addrinfo *host_info_list; // Pointer to the to the linked list of host_info's.

  int socketfd; // The socket descripter
};

#endif
