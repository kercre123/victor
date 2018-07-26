#include "coretech/messaging/shared/UdpClient.h"
#include "coretech/messaging/shared/UdpServer.h"  // for kConnectionPacket
#include "coretech/common/shared/logging.h"

#include <iostream>
#include <sys/socket.h> // Needed for the socket functions
#include <netdb.h>      // Needed for the socket functions
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>

// Define this to enable logs
#define LOG_CHANNEL                    "UdpClient"

#ifdef  LOG_CHANNEL
#define LOG_ERROR(name, format, ...)   CORETECH_LOG_ERROR(name, format, ##__VA_ARGS__)
#define LOG_WARNING(name, format, ...) CORETECH_LOG_WARNING(name, format, ##__VA_ARGS__)
#define LOG_INFO(name, format, ...)    CORETECH_LOG_INFO(LOG_CHANNEL, name, format, ##__VA_ARGS__)
#define LOG_DEBUG(name, format, ...)   CORETECH_LOG_DEBUG(LOG_CHANNEL, name, format, ##__VA_ARGS__)
#else
#define LOG_ERROR(name, format, ...)   {}
#define LOG_WARNING(name, format, ...) {}
#define LOG_INFO(name, format, ...)    {}
#define LOG_DEBUG(name, format, ...)   {}
#endif

UdpClient::UdpClient(const std::string& name)
: _name(name)
, host_info_list(nullptr)
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
    LOG_WARNING("UdpClient.Connect.AlreadyConnected", "");
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
    LOG_ERROR("UdpClient.Connect.GetAddrInfoError", 
              "%s (host: %s, port %hu", 
              gai_strerror(status), 
              host_address,
              port);
    return false;
  }

  LOG_INFO("UdpClient.Connect.CreatingSocket", "Port: %hu (%s)", port, _name.c_str());

  sockaddr_in socketAddress;
  memset( &socketAddress, 0, sizeof(socketAddress) );
  
  socketAddress.sin_family = AF_INET;                  // IPv4
  socketAddress.sin_addr.s_addr = htonl( INADDR_ANY ); // accept input from any address
  socketAddress.sin_port = port;
  socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                    host_info_list->ai_protocol);
  
  if (socketfd == -1) {
    LOG_ERROR("UdpClient.Connect.SocketError", "");
    return false;
  }

  set_nonblock(socketfd);

  // Bind socket (so we can send and receive on it)
  
  const int bindResult = bind(socketfd, (struct sockaddr *)&socketAddress, sizeof(socketAddress));
    
  if ( bindResult < 0 )
  {
    if (EADDRINUSE == errno)
    {
      // Unable to bind to in-use socket, continuing as this is OK in case of running multiple instances on one machine.
      LOG_WARNING("UdpClient.Connect.AddrInUse", "");
    }
    else
    {
      LOG_ERROR("UdpClient.Connect.BindFailed", "res = %d, errno = %d '%s'", bindResult, errno, strerror(errno));
    }
  }
  // Send connection packet (i.e. something so that the server adds us to the client list)
  LOG_INFO("UdpClient.Connect.SendConnectionPacket","%s (%zu)", _name.c_str(), sizeof(UdpServer::kConnectionPacket));
  Send(UdpServer::kConnectionPacket, sizeof(UdpServer::kConnectionPacket));
  
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

ssize_t UdpClient::Send(const char* data, size_t size)
{
  if (socketfd < 0) {
    LOG_WARNING("UdpClient.Send.NoSocket", "");
    return 0;
  }
  
  //LOG_DEBUG("UdpClient.Send.Sending", "%d bytes", size);

  ssize_t bytes_sent = sendto(socketfd, data, size, 0,
                          (struct sockaddr *)(host_info_list->ai_addr),
                          sizeof(struct sockaddr_in));

  if (bytes_sent <= 0) {
    if (errno != EWOULDBLOCK) {
      LOG_ERROR("UdpClient.Send.SendFailed", "%d", errno);
      Disconnect();
      return -1;
    }
  }

  return bytes_sent;
}

ssize_t UdpClient::Recv(char* data, size_t maxSize)
{
  if (socketfd < 0) {
    LOG_WARNING("UdpClient.Recv.NoSocket", "");
    return 0;
  }

  assert(data != NULL);
  ssize_t bytes_received = recv(socketfd, data, maxSize, 0);

  if (bytes_received <= 0) {
    if (errno != EWOULDBLOCK) {
      LOG_ERROR("UdpClient.Recv.RecvError", "Disconnecting");
      Disconnect();
      return -1;
    } else {
      bytes_received = 0;
    }
  }
  else {
    //LOG_DEBUG("UdpClient.Recv", "%zd bytes received", bytes_received);
  }

  return bytes_received;
}


