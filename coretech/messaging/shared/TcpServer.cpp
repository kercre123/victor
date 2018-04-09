#include "coretech/messaging/shared/TcpServer.h"

#include <unistd.h>
#include <iostream>
#include <cstring>      // Needed for memset
#include <sys/socket.h> // Needed for the socket functions
#include <netdb.h>      // Needed for the socket functions
#include <netinet/tcp.h> // for tcp no delay
#include <fcntl.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <errno.h>

#ifdef __ANDROID__
#include <linux/in.h>
#endif

TcpServer::TcpServer()
{
  socketfd = -1;
  client_sd = -1;
}

TcpServer::~TcpServer()
{
  if (client_sd >= 0) {
    DisconnectClient();
  }

  if (socketfd >= 0) {
    StopListening();
  }
}

void TcpServer::set_nonblock(int socket) {
    int flags;
    flags = fcntl(socket,F_GETFL,0);
    assert(flags != -1);
    fcntl(socket, F_SETFL, flags | O_NONBLOCK);
}

bool TcpServer::StartListening(const unsigned short port)
{
  if (socketfd >= 0) {
    DEBUG_TCP_SERVER("WARNING (TcpServer): Already listening");
    return false;
  }

  int status;
  struct addrinfo host_info;       // The struct that getaddrinfo() fills up with data.
  struct addrinfo *host_info_list; // Pointer to the to the linked list of host_info's.

  // The MAN page of getaddrinfo() states "All  the other fields in the structure pointed
  // to by hints must contain either 0 or a null pointer, as appropriate." When a struct
  // is created in c++, it will be given a block of memory. This memory is not necessarily
  // empty. Therefore we use the memset function to make sure all fields are NULL.
  memset(&host_info, 0, sizeof host_info);

  host_info.ai_family = AF_UNSPEC;     // IP version not specified. Can be both.
  host_info.ai_socktype = SOCK_STREAM; // Use SOCK_STREAM for TCP or SOCK_DGRAM for UDP.
  host_info.ai_flags = AI_PASSIVE;     // IP Wildcard

  // Now fill up the linked list of host_info structs with google's address information.
  char portStr[8];
  sprintf(portStr, "%d", port);
  status = getaddrinfo(NULL, portStr, &host_info, &host_info_list);
  // getaddrinfo returns 0 on success, or some other value when an error occurred.
  // (translated into human readable text by the gai_strerror function).
  if (status != 0) {
    std::cerr << "getaddrinfo error" << gai_strerror(status) ;
    return false;
  }

  DEBUG_TCP_SERVER("TcpServer: Creating a socket on port " << portStr);

  socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                    host_info_list->ai_protocol);
  if (socketfd == -1) {
    std::cerr << "socket error\n" ;
    return false;
  }

  DEBUG_TCP_SERVER("TcpServer: Binding socket...");

  // we use to make the setsockopt() function to make sure the port is not in use
  // by a previous execution of our code. (see man page for more information)
  int yes = 1;

  status = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
  if (status != 0) {
    DEBUG_TCP_SERVER("TcpServer: Unable to set SO_REUSEADDR, errno=" << strerror(errno));
  }
  
  #if defined(LINUX) || defined(ANDROID) || defined(VICOS)
  // No SO_NOSIGPIPE available on these platforms - handled with MSG_NOSIGNAL flag on send instead
  #else
  // don't generate a SIGPIPE exception for writing to a closed socket
  status = setsockopt(socketfd, SOL_SOCKET, SO_NOSIGPIPE, &yes, sizeof(yes));
  if (status != 0) {
    DEBUG_TCP_SERVER("TcpServer: Unable to set SO_NOSIGPIPE, errno=" << strerror(errno));
  }
  #endif // defined(LINUX) || defined(ANDROID)
  
  status = setsockopt(socketfd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));
  if (status != 0) {
    DEBUG_TCP_SERVER("TcpServer: Unable to set TCP_NODELAY, errno=" << strerror(errno));
  }

  //status = setsockopt(socketfd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(int));  // For auto-detecting disconnected clients.
  //if (status != 0) {
  //  DEBUG_TCP_SERVER("TcpServer: Unable to set SO_KEEPALIVE, errno=" << strerror(errno));
  //}
  
  set_nonblock(socketfd);
  
  status = bind(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    std::cerr << "**** ERROR: bind error on port " << port << " (You might have orphaned processes running) ****\n";
    return false;
  }
  
  freeaddrinfo(host_info_list);

  DEBUG_TCP_SERVER("TcpServer: Listening for connections...");

  status =  listen(socketfd, 5);
  if (status == -1) {
    std::cerr << "listen error\n";
    return false;
  }

  return true;
}

void TcpServer::StopListening() 
{
  if (socketfd >= 0) {
    DEBUG_TCP_SERVER("TcpServer: Stopping server listening on socket " << socketfd);
    
    close(socketfd);
    socketfd = -1;
    return;
  }

  DEBUG_TCP_SERVER("TcpServer: Server already stopped");

}


bool TcpServer::Accept()
{
  if (socketfd < 0) {
    DEBUG_TCP_SERVER("WARNING (TcpServer): Listening socket not yet open");
    return false;
  }

  struct sockaddr_storage their_addr;
  socklen_t addr_size = sizeof(their_addr);
  client_sd = accept(socketfd, (struct sockaddr *)&their_addr, &addr_size);
  if (client_sd == -1)
  {
    if (errno != EWOULDBLOCK)
    {
       DEBUG_TCP_SERVER("accept error");
       StopListening();
    }
    return false;
  }
  else
  {
    DEBUG_TCP_SERVER("TcpServer: Connection accepted. Using new socketfd : "  <<  client_sd);
    set_nonblock(client_sd);
    return true;
  }

}


int TcpServer::Send(const char* data, int size)
{
  if (size <= 0) return 0;

  if (!HasClient()) {
    DEBUG_TCP_SERVER("TcpServer: Send() failed because no client connected");
    return -1;
  }

  DEBUG_TCP_SERVER_VERBOSE("TcpServer: sending " << size << " bytes " << data);
  
  #if defined(LINUX) || defined(ANDROID)
  // Prevent SIGPIPE if the socket has closed, not supported on all platforms
  const int sendFlags = MSG_NOSIGNAL;
  #else
  // No MSG_NOSIGNAL available on these platforms - handled with SO_NOSIGPIPE socketopt instead
  const int sendFlags = 0;
  #endif // defined(LINUX) || defined(ANDROID)
  
  const ssize_t bytes_sent = send(client_sd, data, size, sendFlags);
  
  if(bytes_sent > std::numeric_limits<int>::max()) {
    DEBUG_TCP_SERVER("TcpServer: Send warning, num bytes sent > max integer.\n");
  }
  
  return static_cast<int>(bytes_sent);
}

int TcpServer::Recv(char* data, int maxSize)
{
  if (!HasClient()) {
    DEBUG_TCP_SERVER("TcpServer: Recv() failed because no client connected");
    return -1;
  }

  //DEBUG_TCP_SERVER("TcpServer: Waiting to receive data...");

  ssize_t bytes_received;
  bytes_received = recv(client_sd, data, maxSize, 0);
  
  if (bytes_received <= 0) {
    if (errno != EWOULDBLOCK)
    {
      DEBUG_TCP_SERVER("receive error! " << errno);
      DisconnectClient();
    } else {
      bytes_received = 0;
    }
  } else {
    DEBUG_TCP_SERVER_VERBOSE("TcpServer: " << bytes_received << " bytes received : " << data);
  }

  if(bytes_received > std::numeric_limits<int>::max()) {
    DEBUG_TCP_SERVER("TcpServer: Receive warning, num bytes received > max integer.\n");
  }
  
  return static_cast<int>(bytes_received);
}


void TcpServer::DisconnectClient() 
{
    if (client_sd >= 0) {
      DEBUG_TCP_SERVER("TcpServer: Disconnecting client at socket " << client_sd);
      close(client_sd);
      client_sd = -1;
      return;
    }

    DEBUG_TCP_SERVER("TcpServer: No client connected");
}



bool TcpServer::HasClient() const
{
  return (client_sd != -1);
}


