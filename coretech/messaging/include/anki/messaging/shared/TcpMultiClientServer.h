/**
 * File: TcpMultiClientServer.h
 *
 * Author: Kevin Yoon
 * Created: 1/6/2015
 *
 * Description: TCP server that supports multiple clients
 *              A thread is launched to handle client connections and read incoming data.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/


#ifndef TCP_MULTICLIENT_SERVER_H
#define TCP_MULTICLIENT_SERVER_H

#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <sys/socket.h> // Needed for the socket functions
#include <sys/select.h>

class TcpMultiClientServer {
public:
  TcpMultiClientServer();
  ~TcpMultiClientServer();

  // Starts server_thread() and begins listening for connections
  void Start(unsigned short listen_port);
  
  // Disconnects all clients and stop server_thread()
  void Stop();
  
  // Whether or not the server thread is running
  bool IsRunning() const;

  // Disconnects the client with the given id
  void DisconnectClient(unsigned int client_id);
  
  // Returns the number of connected clients
  unsigned int GetNumClients() const;
  
  // Fills the vector with the IDs of all connected clients
  int GetConnectedClientIDs(std::vector<int> &client_ids);

  // Sends data to the specified client
  int Send(unsigned int client_id, const char* data, int size);
  
  // Receives data from the specified client
  int Recv(unsigned int client_id, char* data, int maxSize);
  
  //int SendAll(const char* data, int size);
  
  
  // TODO: Define buffer to which you want received data to be dumped directly.
  //       This would make Recv() effectively useless for the specified client ID.
  // int SetRecvBuf(unsigned int client_id, char* data, int *data_len, std::mutex data_lock);

private:
  
  // The main server thread that handles incoming connections and reads data from connected clients
  std::thread server_thread;
  bool server_handler();
  
  // Flag to kill server thread
  bool kill_thread;
  
  // The port on which the server listens for client connections
  int listenPort;

  // The set of socket descriptors on which to select()
  fd_set allset;
  

  static const int MAX_RECV_BUF_SIZE = 1000000;
  typedef struct cli_info_t {
    cli_info_t() {sd = -1; recv_data_size = 0;}
    int sd;                  // socket descriptor
    std::mutex cli_mutex;    // client socket access mutex
    int recv_data_size;      // size of data in recv_data
    char recv_data[MAX_RECV_BUF_SIZE];  // buffer of received data
  } cli_info;
  
  std::recursive_mutex client_info_mutex;
  
  typedef std::map<int, cli_info>::iterator client_info_it;
  std::map<int, cli_info> client_info;
  
};

#endif
