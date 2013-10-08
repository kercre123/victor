#include "TcpServer.h"
#include <unistd.h>
#include <iostream>
#include <cstring>      // Needed for memset
#include <sys/socket.h> // Needed for the socket functions
#include <netdb.h>      // Needed for the socket functions
#include <fcntl.h>
#include <assert.h>
#include <errno.h>

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

void set_nonblock(int socket) {
    int flags;
    flags = fcntl(socket,F_GETFL,0);
    assert(flags != -1);
    fcntl(socket, F_SETFL, flags | O_NONBLOCK);
}

bool TcpServer::StartListening(const char* port)
{
    if (socketfd >= 0) {
#if(DEBUG_TCP_SERVER)
      std::cout << "WARNING (TcpServer): Already listening\n";
#endif
      return false;
    }

    int status;
    struct addrinfo host_info;       // The struct that getaddrinfo() fills up with data.
    struct addrinfo *host_info_list; // Pointer to the to the linked list of host_info's.

    // The MAN page of getaddrinfo() states "All  the other fields in the structure pointed
    // to by hints must contain either 0 or a null pointer, as appropriate." When a struct
    // is created in c++, it will be given a block of memory. This memory is not nessesary
    // empty. Therefor we use the memset function to make sure all fields are NULL.
    memset(&host_info, 0, sizeof host_info);

    host_info.ai_family = AF_UNSPEC;     // IP version not specified. Can be both.
    host_info.ai_socktype = SOCK_STREAM; // Use SOCK_STREAM for TCP or SOCK_DGRAM for UDP.
    host_info.ai_flags = AI_PASSIVE;     // IP Wildcard

    // Now fill up the linked list of host_info structs with google's address information.
    status = getaddrinfo(NULL, port, &host_info, &host_info_list);
    // getaddrinfo returns 0 on succes, or some other value when an error occured.
    // (translated into human readable text by the gai_gai_strerror function).
    if (status != 0)  std::cout << "getaddrinfo error" << gai_strerror(status) ;

#if(DEBUG_TCP_SERVER)
    std::cout << "TcpServer: Creating a socket on port " << port  << "\n";
#endif
    socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                      host_info_list->ai_protocol);
    if (socketfd == -1) {
      std::cout << "socket error\n" ;
      return false;
    }

#if(DEBUG_TCP_SERVER)
    std::cout << "TcpServer: Binding socket...\n";
#endif
    // we use to make the setsockopt() function to make sure the port is not in use
    // by a previous execution of our code. (see man page for more information)
    int yes = 1;
    status = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));


    //status = setsockopt(socketfd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(int));  // For auto-detecting disconnected clients.
    set_nonblock(socketfd);

    
    status = bind(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
      std::cout << "bind error\n";
      return false;
    }

#if(DEBUG_TCP_SERVER)
    std::cout << "TcpServer: Listening for connections...\n";
#endif
    status =  listen(socketfd, 5);
    if (status == -1) {
      std::cout << "listen error\n";
      return false;
    }

    return true;
}

void TcpServer::StopListening() 
{
  if (socketfd >= 0) {
#if(DEBUG_TCP_SERVER)
    std::cout << "TcpServer: Stopping server listening on socket " << socketfd << "\n";
#endif
    freeaddrinfo(host_info_list);
    close(socketfd);
    socketfd = -1;
    return;
  }

#if(DEBUG_TCP_SERVER)
    std::cout << "TcpServer: Server already stopped\n";
#endif
}


bool TcpServer::Accept()
{
  if (socketfd < 0) {
#if(DEBUG_TCP_SERVER)
    std::cout << "WARNING (TcpServer): Listening socket not yet open\n";
#endif
    return false;
  }

  struct sockaddr_storage their_addr;
  socklen_t addr_size = sizeof(their_addr);
  client_sd = accept(socketfd, (struct sockaddr *)&their_addr, &addr_size);
  if (client_sd == -1)
  {
    if (errno != EWOULDBLOCK)
    {
#if(DEBUG_TCP_SERVER)
       std::cout << "accept error\n";
#endif
       StopListening();
    }
    return false;
  }
  else
  {
#if(DEBUG_TCP_SERVER)
    std::cout << "TcpServer: Connection accepted. Using new socketfd : "  <<  client_sd << "\n";
#endif
    return true;
  }

}


int TcpServer::Send(const char* data, int size)
{
  if (size <= 0) return 0;

  if (!HasClient()) {
#if(DEBUG_TCP_SERVER)
    std::cout << "TcpServer: Send() failed because no client connected\n";
#endif
    return -1;
  }


#if(DEBUG_TCP_SERVER)
  std::cout << "TcpServer: sending   " << data << "\n";
#endif
  return send(client_sd, data, size, 0);
}

int TcpServer::Recv(char* data, int maxSize)
{
  if (!HasClient()) {
#if(DEBUG_TCP_SERVER)
    std::cout << "TcpServer: Recv() failed because no client connected\n";
#endif
    return -1;
  }


#if(DEBUG_TCP_SERVER)
  //std::cout << "TcpServer: Waiting to receive data...\n";
#endif
  ssize_t bytes_received;
  bytes_received = recv(client_sd, data, maxSize, 0);
  // If no data arrives, the program will just wait here until some data arrives.


  
  if (bytes_received <= 0) {
    if (errno != EWOULDBLOCK)
    {
#if(DEBUG_TCP_SERVER)
      std::cout << "receive error!\n";
#endif
      DisconnectClient();
    }
  } else {
#if(DEBUG_TCP_SERVER)
    std::cout << "TcpServer: " << bytes_received << " bytes received : " << data << "\n";
#endif
  }

  return bytes_received;
}


void TcpServer::DisconnectClient() 
{
    if (client_sd >= 0) {
#if(DEBUG_TCP_SERVER)
      std::cout << "TcpServer: Disconnecting client at socket " << client_sd << "\n";
#endif
      close(client_sd);
      client_sd = -1;
      return;
    }

#if(DEBUG_TCP_SERVER)
    std::cout << "TcpServer: No client connected\n";
#endif
}



bool TcpServer::HasClient() 
{
  return (client_sd != -1);
}


