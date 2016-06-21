#include "anki/messaging/shared/UdpClient.h"

#include <iostream>
#include <cstring>      // Needed for memset
#include <sys/socket.h> // Needed for the socket functions
#include <netdb.h>      // Needed for the socket functions
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>


UdpClient::UdpClient()
: host_info_list(nullptr)
, socketfd(-1)
{
}

UdpClient::~UdpClient()
{
  Disconnect();
}

void UdpClient::set_nonblock(int socket) {
    int flags;
    flags = fcntl(socket,F_GETFL,0);
    assert(flags != -1);
    fcntl(socket, F_SETFL, flags | O_NONBLOCK);
}


bool UdpClient::Connect(const char *host_address, const unsigned short port)
{
  if (socketfd >= 0) {
    DEBUG_UDP_CLIENT("UdpClient: Already connected\n");
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
  host_info.ai_socktype = SOCK_DGRAM; // Use SOCK_STREAM for TCP or SOCK_DGRAM for UDP.

  char portStr[8];
  sprintf(portStr, "%d", port);
  status = getaddrinfo(host_address, portStr, &host_info, &host_info_list);
  if (status != 0) {
    std::cout << "getaddrinfo error: " << gai_strerror(status) << " (host " << host_address << ":" << port << ")\n";
    return false;
  }

  DEBUG_UDP_CLIENT("UdpClient: Creating a socket on port " << portStr << "\n");

  sockaddr_in socketAddress;
  memset( &socketAddress, 0, sizeof(socketAddress) );
  
  socketAddress.sin_family = AF_INET;                  // IPv4
  socketAddress.sin_addr.s_addr = htonl( INADDR_ANY ); // accept input from any address
  socketAddress.sin_port = port;
  socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                    host_info_list->ai_protocol);
  
  if (socketfd == -1) {
    std::cout << "socket error\n" ;
    return false;
  }

  set_nonblock(socketfd);

  // Bind socket (so we can send and receive on it)
  
  const int bindResult = bind(socketfd, (struct sockaddr *)&socketAddress, sizeof(socketAddress));
    
  if ( bindResult < 0 )
  {
    if (EADDRINUSE == errno)
    {
      printf("UdpClient::Connect: Warning: Unable to bind to in-use socket, continuing as this is OK in case of running multiple instances on one machine.\n");
    }
    else
    {
      printf("UdpClient::Connect: Error: Unable to bind socket (res = %d), errno = %d '%s'\n", bindResult, errno, strerror(errno));
    }
  }
  // Send connection packet (i.e. something so that the server adds us to the client list)
  const char zero = 0;
  Send(&zero, 1);
  
  return true;
}

bool UdpClient::Disconnect()
{
  if (socketfd > -1) {
    freeaddrinfo(host_info_list);
    host_info_list = nullptr;
    close(socketfd);
    socketfd = -1;
  }
  return true;
}

int UdpClient::Send(const char* data, int size)
{
  if (socketfd < 0) {
    DEBUG_UDP_CLIENT("UdpClient (WARN): Socket undefined. Skipping Send().\n");
    return 0;
  }
  
  DEBUG_UDP_CLIENT("UdpClient: sending " << size << " bytes: " << data << "\n");

  ssize_t bytes_sent = sendto(socketfd, data, size, 0,
                          (struct sockaddr *)(host_info_list->ai_addr),
                          sizeof(struct sockaddr_in));

  if (bytes_sent <= 0) {
    if (errno != EWOULDBLOCK) {
      DEBUG_UDP_CLIENT("UdpClient: Send error, disconnecting (" << errno << ").\n");
      Disconnect();
      return -1;
    }
  }
  
  if(bytes_sent > std::numeric_limits<int>::max()) {
    DEBUG_UDP_CLIENT("UdpClient: Send warning, num bytes sent > max integer.\n");
  }

  return static_cast<int>(bytes_sent);
}

int UdpClient::Recv(char* data, int maxSize)
{
    if (socketfd < 0) {
      DEBUG_UDP_CLIENT("UdpClient (WARN): Socket undefined. Skipping Recv().\n");
      return 0;
    }
  
    DEBUG_UDP_CLIENT( "UdpClient: Waiting to receive data...\n");

    assert(data != NULL);
    ssize_t bytes_received;
    bytes_received = recv(socketfd, data, maxSize, 0);
    // If no data arrives, the program will just wait here until some data arrives.
  
    if (bytes_received <= 0) {
      if (errno != EWOULDBLOCK) {
        DEBUG_UDP_CLIENT("UdpClient: Receive error, disconnecting.\n");
        Disconnect();
        return -1;
      } else {
        bytes_received = 0;
      }
    }
    else {
      DEBUG_UDP_CLIENT("UdpClient: " << bytes_received << " bytes recieved : " << data << "\n");
    }

  if(bytes_received > std::numeric_limits<int>::max()) {
    DEBUG_UDP_CLIENT("UdpClient: Receive warning, num bytes received > max integer.\n");
  }
  
  return static_cast<int>(bytes_received);
}


