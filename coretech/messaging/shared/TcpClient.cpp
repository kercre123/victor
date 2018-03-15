#include "coretech/messaging/shared/TcpClient.h"

#include <iostream>
#include <cstring>      // Needed for memset
#include <sys/socket.h> // Needed for the socket functions
#include <netdb.h>      // Needed for the socket functions
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

TcpClient::TcpClient()
{
  socketfd = -1;
}

TcpClient::~TcpClient()
{
  if (socketfd > -1) {
    Disconnect();
  }
}

void TcpClient::set_nonblock(int socket) {
    int flags;
    flags = fcntl(socket,F_GETFL,0);
    assert(flags != -1);
    fcntl(socket, F_SETFL, flags | O_NONBLOCK);
}


bool TcpClient::Connect(const char *host_address, const unsigned short port)
{
  if (socketfd >= 0) {
    DEBUG_TCP_CLIENT("TcpClient: Already connected\n");
    return false;
  }

  int status;
  struct addrinfo host_info;       // The struct that getaddrinfo() fills up with data.

  // The MAN page of getaddrinfo() states "All  the other fields in the structure pointed
  // to by hints must contain either 0 or a null pointer, as appropriate." When a struct
  // is created in c++, it will be given a block of memory. This memory is not nessesary
  // empty. Therefor we use the memset function to make sure all fields are NULL.
  memset(&host_info, 0, sizeof host_info);

  host_info.ai_family = AF_UNSPEC;     // IP version not specified. Can be both.
  host_info.ai_socktype = SOCK_STREAM; // Use SOCK_STREAM for TCP or SOCK_DGRAM for UDP.

  char portStr[8];
  sprintf(portStr, "%d", port);
  status = getaddrinfo(host_address, portStr, &host_info, &host_info_list);
  if (status != 0) {
    std::cout << "getaddrinfo error" << gai_strerror(status) ;
    return false;
  }

  DEBUG_TCP_CLIENT("TcpClient: Creating a socket on port " << portStr);
  socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                    host_info_list->ai_protocol);
  if (socketfd == -1) {
    std::cout << "socket error\n" ;
    return false;
  }


  DEBUG_TCP_CLIENT("TcpClient: Connecting to " << host_address << "\n");
  status = connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    std::cout << "connect error\n" ;
    close(socketfd);
    socketfd = -1;
    return false;
  }

  set_nonblock(socketfd);

  return true;
}

bool TcpClient::Disconnect()
{
  freeaddrinfo(host_info_list);
  close(socketfd);
  socketfd = -1;

  return true;
}

int TcpClient::Send(const char* data, int size)
{
  DEBUG_TCP_CLIENT("TcpClient: sending " << size << " bytes: " << data << "\n");

  ssize_t bytes_sent = send(socketfd, data, size, 0);
  if (bytes_sent < 0) {
    if (errno != EWOULDBLOCK) {
      DEBUG_TCP_CLIENT("TcpClient: Send error (" << errno << "), disconnecting. (bytes_sent = " << bytes_sent << ")\n");
      Disconnect();
      return -1;
    }
  }

  if(bytes_sent > std::numeric_limits<int>::max()) {
    DEBUG_TCP_CLIENT("TcpClient: Send warning, num bytes sent > max integer.\n");
  }
  
  return static_cast<int>(bytes_sent);
}

int TcpClient::Recv(char* data, int maxSize)
{
  DEBUG_TCP_CLIENT("TcpClient: Waiting to receive data...\n");
  
  assert(data != NULL);
  ssize_t bytes_received;
  bytes_received = recv(socketfd, data, maxSize, 0);
  // If no data arrives, the program will just wait here until some data arrives.
  
  if (bytes_received <= 0) {
    if (errno != EWOULDBLOCK) {
      DEBUG_TCP_CLIENT("TcpClient: Receive error (" << errno << "), disconnecting.\n");
      Disconnect();
      return -1;
    } else {
      bytes_received = 0;
    }
  }
  else {
    DEBUG_TCP_CLIENT("TcpClient: " << bytes_received << " bytes received : " << data << "\n");
  }
  
  if(bytes_received > std::numeric_limits<int>::max()) {
    DEBUG_TCP_CLIENT("TcpClient: Receive warning, num bytes received > max integer.\n");
  }
  
  return static_cast<int>(bytes_received);
}

bool TcpClient::IsConnected()
{
  return socketfd != -1;
}
