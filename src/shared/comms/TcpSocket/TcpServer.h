// Simple TCP server that only accepts one client.

#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#define DEBUG_TCP_SERVER 1

class TcpServer {
public:
  TcpServer();
  ~TcpServer();

  bool StartListening(const char* port);
  void StopListening();

  bool Accept();
  void DisconnectClient();
  bool HasClient();
  //bool SetBlocking()

  int Send(const char* data, int size);
  int Recv(char* data, int maxSize);

private:
  struct addrinfo *host_info_list; // Pointer to the to the linked list of host_info's.

  int socketfd; // Listening socket descripter
  int client_sd; // Client socket descripter
};

#endif
