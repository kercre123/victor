#include "TcpClient.h"
#include <iostream>
#include <cstring>      // Needed for memset
#include <sys/socket.h> // Needed for the socket functions
#include <netdb.h>      // Needed for the socket functions
#include <assert.h>

TcpClient::TcpClient()
{
  socketfd = -1;
}

TcpClient::~TcpClient()
{
  if (socketfd >= -1) {
    Disconnect();
  }
}


bool TcpClient::Connect(const char *host_address, const char* port)
{
  int status;
  struct addrinfo host_info;       // The struct that getaddrinfo() fills up with data.

  // The MAN page of getaddrinfo() states "All  the other fields in the structure pointed
  // to by hints must contain either 0 or a null pointer, as appropriate." When a struct
  // is created in c++, it will be given a block of memory. This memory is not nessesary
  // empty. Therefor we use the memset function to make sure all fields are NULL.
  memset(&host_info, 0, sizeof host_info);

  host_info.ai_family = AF_UNSPEC;     // IP version not specified. Can be both.
  host_info.ai_socktype = SOCK_STREAM; // Use SOCK_STREAM for TCP or SOCK_DGRAM for UDP.

  status = getaddrinfo(host_address, port, &host_info, &host_info_list);  
  if (status != 0)  std::cout << "getaddrinfo error" << gai_strerror(status) ;

  std::cout << "TcpClient: Creating a socket on port " << port << "\n";
  socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                    host_info_list->ai_protocol);
  if (socketfd == -1)  std::cout << "socket error " ;


  std::cout << "TcpClient: Connecting to " << host_address << "\n";
  status = connect(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1)  std::cout << "connect error" ;

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
#if(DEBUG_TCP_CLIENT)
  std::cout << "TcpClient: sending " << size << " bytes: " << data << "\n";
#endif
  return send(socketfd, data, size, 0);
}

int TcpClient::Recv(char* data, int maxSize)
{
#if(DEBUG_TCP_CLIENT)
    std::cout << "TcpClient: Waiting to recieve data...\n";
#endif
    assert(data != NULL);
    ssize_t bytes_received;
    bytes_received = recv(socketfd, data, maxSize, 0);
    // If no data arrives, the program will just wait here until some data arrives.

#if(DEBUG_TCP_CLIENT)
    if (bytes_received == -1)std::cout << "TcpClient: receive error!\n";
    if (bytes_received > 0) {
      std::cout << "TcpClient: " << bytes_received << " bytes recieved : " << data << "\n";
    }
#endif

    return bytes_received;
}


