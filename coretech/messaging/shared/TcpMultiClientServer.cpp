#include "coretech/messaging/shared/TcpMultiClientServer.h"

#include <unistd.h>
#include <iostream>
#include <cstring>      // Needed for memset
#include <sys/socket.h> // Needed for the socket functions
#include <netdb.h>      // Needed for the socket functions
#include <fcntl.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <errno.h>

#ifdef __ANDROID__
#include <linux/in.h>
#endif

// TODO: Should be possible to do this with HANDLE_RECV_ON_SELECT == 0.
//       This way we can skip a memcpy().
#define HANDLE_RECV_ON_SELECT 1

//
// Create a debug messaging macro to avoid a zillion #if/#endif directives.
// Uncomment the first one to disable debug messages.  Uncomment the second
// one to enable them.  Newline (std::endl) automatically appended.
//  Example usage: DEBUG_TCP_MULTICLIENT_SERVER("Number of clients " << N);
//
#define DEBUG_TCP_SERVER(__expr__)
//#define DEBUG_TCP_SERVER(__expr__) (std::cout << __expr__ << std::endl)

TcpMultiClientServer::TcpMultiClientServer()
{}

TcpMultiClientServer::~TcpMultiClientServer()
{
  Stop();
}

void set_nonblock(int socket) {
    int flags;
    flags = fcntl(socket,F_GETFL,0);
    assert(flags != -1);
    fcntl(socket, F_SETFL, flags | O_NONBLOCK);
}

bool TcpMultiClientServer::server_handler()
{

  int listenfd; // Listening socket descriptor
  int status;
  struct addrinfo host_info;       // The struct that getaddrinfo() fills up with data.
  struct addrinfo *host_info_list; // Pointer to the to the linked list of host_info's.
  
  int maxClientId = -1;
  int maxfd;
  fd_set rset;


  // The MAN page of getaddrinfo() states "All  the other fields in the structure pointed
  // to by hints must contain either 0 or a null pointer, as appropriate." When a struct
  // is created in c++, it will be given a block of memory. This memory is not nessesary
  // empty. Therefor we use the memset function to make sure all fields are NULL.
  memset(&host_info, 0, sizeof host_info);

  host_info.ai_family = AF_UNSPEC;     // IP version not specified. Can be both.
  host_info.ai_socktype = SOCK_STREAM; // Use SOCK_STREAM for TCP or SOCK_DGRAM for UDP.
  host_info.ai_flags = AI_PASSIVE;     // IP Wildcard

  // Now fill up the linked list of host_info structs with google's address information.
  char portStr[8];
  sprintf(portStr, "%d", listenPort);
  status = getaddrinfo(NULL, portStr, &host_info, &host_info_list);
  // getaddrinfo returns 0 on succes, or some other value when an error occured.
  // (translated into human readable text by the gai_gai_strerror function).
  if (status != 0)  std::cerr << "getaddrinfo error" << gai_strerror(status) ;

  DEBUG_TCP_SERVER("TcpMultiClientServer: Creating a socket on port " << portStr);

  listenfd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                    host_info_list->ai_protocol);
  if (listenfd == -1) {
    std::cerr << "socket error\n" ;
    return false;
  }

  DEBUG_TCP_SERVER("TcpMultiClientServer: Binding socket...");

  // we use to make the setsockopt() function to make sure the port is not in use
  // by a previous execution of our code. (see man page for more information)
  int yes = 1;
  status = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));


  //status = setsockopt(socketfd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(int));  // For auto-detecting disconnected clients.
  set_nonblock(listenfd);
  if (status == -1) {
    DEBUG_TCP_SERVER("TcpMultiClientServer: Failed to set socket options (status=" << status << ")");
  }


  status = bind(listenfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    std::cerr << "bind error\n";
    return false;
  }
  freeaddrinfo(host_info_list);

  DEBUG_TCP_SERVER("TcpMultiClientServer: Listening for connections...");

  status =  listen(listenfd, 10);
  if (status == -1) {
    std::cerr << "listen error\n";
    return false;
  }

  
  // Initialize all socket descriptor set for select()
  maxfd = listenfd;
  FD_ZERO(&allset);
  FD_SET(listenfd, &allset);
  
  // Set timeout for select()
  struct timeval select_timeout;
  select_timeout.tv_sec = 0;
  select_timeout.tv_usec = 100000; // Max amount of time it takes to kill the thread after Stop() is called
  
  while (1) {

    rset = allset;
    int nready = select(maxfd+1, &rset, NULL, NULL, &select_timeout);

    if (FD_ISSET(listenfd, &rset)) {
      // New client connection
      struct sockaddr_in cli_addr;
      socklen_t addr_size = sizeof(cli_addr);
      int connfd = accept(listenfd, (struct sockaddr *)&cli_addr, &addr_size);
      if (connfd == -1)
      {
        DEBUG_TCP_SERVER("accept error");
        continue;
      }
      else
      {
        DEBUG_TCP_SERVER("TcpMultiClientServer: Connection accepted. Using new socketfd : "  <<  connfd);
        
        // Set the socket descriptor to the first available one
        ++maxClientId;
        
        set_nonblock(connfd);
        client_info_mutex.lock();
        client_info[maxClientId].cli_mutex.lock();
        client_info[maxClientId].sd = connfd;
        client_info[maxClientId].cli_mutex.unlock();
        client_info_mutex.unlock();
        
#if(HANDLE_RECV_ON_SELECT)
        // Add descriptor to set
        FD_SET(connfd, &allset);
        if(connfd > maxfd) {
          maxfd = connfd;
        }
#endif
        
        // If there are no more readable descriptors
        // there is nothing left to do so go back to select
        if (--nready <= 0) {
          continue;
        }
      }
    }
    
    
#if(HANDLE_RECV_ON_SELECT)
    // Check all clients for data
    client_info_mutex.lock();
    for (client_info_it it=client_info.begin(); it != client_info.end(); ++it) {
      
      if (FD_ISSET(it->second.sd, &rset)) {
        
        it->second.cli_mutex.lock();
        
        ssize_t bytes_received;
        bytes_received = recv(it->second.sd, it->second.recv_data + it->second.recv_data_size, MAX_RECV_BUF_SIZE - it->second.recv_data_size, 0);
        it->second.recv_data_size += bytes_received;
        it->second.cli_mutex.unlock();
        
        if (bytes_received <= 0) {
          DisconnectClient(it->first);
        } else {
          DEBUG_TCP_SERVER("TcpMultiClientServer: " << bytes_received << " bytes received");
        }
      
        // Check if there are any more clients to read from
        if (--nready <= 0) {
          break;
        }
        
      }
    }
    client_info_mutex.unlock();
#endif
    
    if (kill_thread) {
      DEBUG_TCP_SERVER("TcpMultiClientServer: Killing server_thread");
      
      for (client_info_it it=client_info.begin(); it != client_info.end(); ++it) {
        DisconnectClient(it->first);
      }
      return true;
    }
  
  }
  

  return true;
}

void TcpMultiClientServer::Start(unsigned short listen_port)
{
  if (!server_thread.joinable()) {
    kill_thread = false;
    listenPort = listen_port;
    server_thread = std::thread(&TcpMultiClientServer::server_handler, this);
  }
}

void TcpMultiClientServer::Stop()
{
  if (server_thread.joinable()) {
    DEBUG_TCP_SERVER("TcpMultiClientServer: Stopping...");
    kill_thread = true;
    server_thread.join();
  }
}

bool TcpMultiClientServer::IsRunning() const
{
  return server_thread.joinable();
}


int TcpMultiClientServer::Send(unsigned int client_id, const char* data, int size)
{
  if (size <= 0) return 0;

  client_info_it it = client_info.find(client_id);
  if (it == client_info.end()) {
    DEBUG_TCP_SERVER("TcpMultiClientServer: Send() failed because no client connected");
    return -1;
  }

  client_info_mutex.lock();
  
  // Check if iterator is valid still
  it = client_info.find(client_id);
  if (it == client_info.end()) {
    client_info_mutex.unlock();
    DEBUG_TCP_SERVER("TcpMultiClientServer: Send() failed because client disconnected while waiting for lock");
    return -1;
  }
  
  DEBUG_TCP_SERVER("TcpMultiClientServer: sending   " << data);

  // Lock access to client data
  it->second.cli_mutex.lock();
  
  const ssize_t bytes_sent = send(it->second.sd, data, size, 0);
  
  if (bytes_sent < size) {
    DisconnectClient(client_id);
  }
  
  // Unlock access to client data
  it->second.cli_mutex.unlock();

  client_info_mutex.unlock();
  
  return static_cast<int>(bytes_sent);
}

int TcpMultiClientServer::Recv(unsigned int client_id, char* data, int maxSize)
{
  
  client_info_it it = client_info.find(client_id);
  if (it == client_info.end()) {
    DEBUG_TCP_SERVER("TcpMultiClientServer: Recv() failed because no client connected");
    return -1;
  }

#if(HANDLE_RECV_ON_SELECT)
  if (it->second.recv_data_size == 0) {
    return 0;
  }

  client_info_mutex.lock();
  
  // Check if iterator is valid still
  it = client_info.find(client_id);
  if (it == client_info.end()) {
    client_info_mutex.unlock();
    DEBUG_TCP_SERVER("TcpMultiClientServer: Recv() failed because client disconnected while waiting for lock");
    return -1;
  }
  
  // Lock access to client data
  it->second.cli_mutex.lock();
  
  int bytesReadyToRead = it->second.recv_data_size;
  
  // Calculate number of bytes that will be read out
  int dataSize = std::min(bytesReadyToRead, maxSize);
  
  // Copy bytes to be readout to data
  memcpy(data, it->second.recv_data, dataSize);
  
  // Shift remaining bytes to front of recv buffer
  it->second.recv_data_size -= dataSize;
  memmove(it->second.recv_data,
          it->second.recv_data + dataSize,
          it->second.recv_data_size);

  // Unlock access to client data
  it->second.cli_mutex.unlock();
  
  client_info_mutex.unlock();
  
  return dataSize;
  
#else
  
  client_info_mutex.lock();
  
  // Check if iterator is valid still
  it = client_info.find(client_id);
  if (it == client_info.end()) {
    client_info_mutex.unlock();
    DEBUG_TCP_SERVER("TcpMultiClientServer: Recv() failed because client disconnected while waiting for lock");
    return -1;
  }
  
  // Lock access to client data
  it->second.cli_mutex.lock();
  
  ssize_t bytes_received;
  bytes_received = recv(it->second.sd, data, maxSize, 0);
  
  // Unlock access to client data
  it->second.cli_mutex.unlock();
  
  client_info_mutex.unlock();
  
  
  if (bytes_received <= 0) {
    DisconnectClient(it->first);
    return -1;
  }
  DEBUG_TCP_SERVER("TcpMultiClientServer: " << bytes_received << " bytes received");
  
  return bytes_received;

#endif
  

}


void TcpMultiClientServer::DisconnectClient(unsigned int client_id)
{
  client_info_mutex.lock();
  client_info_it it = client_info.find(client_id);
  if (it != client_info.end()) {

    it->second.cli_mutex.lock();
    DEBUG_TCP_SERVER("TcpMultiClientServer: Disconnecting client with id " << client_id);
    close(it->second.sd);
    FD_CLR(it->second.sd, &allset);
    it->second.cli_mutex.unlock();
    client_info.erase(it);
    DEBUG_TCP_SERVER("TcpMultiClientServer: Total num clients = " << client_info.size());
  }
  client_info_mutex.unlock();
}

int TcpMultiClientServer::GetConnectedClientIDs(std::vector<int> &client_ids)
{
  client_ids.clear();
  client_info_mutex.lock();
  for (client_info_it it=client_info.begin(); it != client_info.end(); ++it) {
    client_ids.push_back(it->first);
  }
  client_info_mutex.unlock();
  return static_cast<int>(client_ids.size());
}

unsigned int TcpMultiClientServer::GetNumClients() const
{
  return static_cast<int>(client_info.size());
}


