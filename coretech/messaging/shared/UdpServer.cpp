#include "coretech/messaging/shared/UdpServer.h"
#include "coretech/common/shared/logging.h"

#include <unistd.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h> // Needed for the socket functions
#include <netdb.h>      // Needed for the socket functions
#include <fcntl.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <errno.h>

// Define this to enable logs
#define LOG_CHANNEL                    "UdpServer"

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

constexpr const char UdpServer::kConnectionPacket[];

UdpServer::UdpServer(const std::string& name)
: _name(name)
, socketfd(-1)
{
}

UdpServer::~UdpServer()
{
  if (socketfd >= 0) {
    StopListening();
  }
}

void UdpServer::set_nonblock(int socket) {
    int flags;
    flags = fcntl(socket,F_GETFL,0);
    assert(flags != -1);
    fcntl(socket, F_SETFL, flags | O_NONBLOCK);
}

bool UdpServer::StartListening(const unsigned short port)
{
    bool res = false;
    if (socketfd >= 0) {
      LOG_WARNING("UdpServer.StartListening.AlreadyListening", "");
      return res;
    }

    int status;
    struct addrinfo host_info;       // The struct that getaddrinfo() fills up with data.
    struct addrinfo *host_info_list; // Pointer to the to the linked list of host_info's.

    // The MAN page of getaddrinfo() states "All  the other fields in the structure pointed
    // to by hints must contain either 0 or a null pointer, as appropriate." When a struct
    // is created in c++, it will be given a block of memory. This memory is not nessesary
    // empty. Therefor we use the memset function to make sure all fields are NULL.
    memset(&host_info, 0, sizeof host_info);

    host_info.ai_family = AF_INET;     // IP version not specified. Can be both.
    host_info.ai_socktype = SOCK_DGRAM; // Use SOCK_STREAM for TCP or SOCK_DGRAM for UDP.
    host_info.ai_flags = AI_PASSIVE;     // IP Wildcard

    // Now fill up the linked list of host_info structs
    char portStr[8];
    sprintf(portStr, "%d", port);
    status = getaddrinfo(NULL, portStr, &host_info, &host_info_list);
    // getaddrinfo returns 0 on succes, or some other value when an error occured.
    // (translated into human readable text by the gai_gai_strerror function).
    if (status != 0) {
     LOG_ERROR("UdpServer.StartListening.GetAddrInfoFailed", "%s", gai_strerror(status));
     freeaddrinfo(host_info_list);
     return res;
    }

    LOG_DEBUG("UdpServer.StartListening.CreatingSocketOnPort", "Port: %d", port);

    socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                      host_info_list->ai_protocol);
    if (socketfd == -1) {
      LOG_ERROR("UdpServer.StartListening.SocketError", "");
      freeaddrinfo(host_info_list);
      return res;
    }

    LOG_DEBUG("UdpServer.StartListening.BindingSocket", "");

    // we use to make the setsockopt() function to make sure the port is not in use
    // by a previous execution of our code. (see man page for more information)
    int yes = 1;
    status = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (status == -1) {
      LOG_WARNING("UdpServer.StartListening.SetSockOptFailed", "%d", status);
    }

    set_nonblock(socketfd);

    
    status = bind(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
      LOG_ERROR("UdpServer.StartListening.BindError", "Port: %d (You might have orphaned processes running)", port);
    } else {
      LOG_DEBUG("UdpServer.StartListening.PortOpen", "");
      res = true;
    }

    freeaddrinfo(host_info_list);
    return res;
}

void UdpServer::StopListening() 
{
  client_list.clear();
  if (socketfd >= 0) {
    LOG_DEBUG("UdpServer.StopListening.ClosingSocket", "");
    
    close(socketfd);
    socketfd = -1;
    return;
  }

  LOG_WARNING("UdpServer.StopListening.AlreadyStopped", "");
}


ssize_t UdpServer::Send(const char* data, size_t size)
{
  if (size <= 0) return 0;

  if (!HasClient()) {
    LOG_WARNING("UdpServer.Send.NoClient", "");
    return -1;
  }

  ssize_t bytes_sent = 0;
  for (client_list_it it = client_list.begin(); it != client_list.end(); it++ ) {
    //LOG_DEBUG("UdpServer.Send.Sending", "%d bytes", size);
    bytes_sent = sendto(socketfd, data, size, 0, (struct sockaddr *)&(*it), sizeof(*it));
    if(bytes_sent != size) {
      LOG_ERROR("UdpServer.Send.SendFailed", "%zd of %zu bytes sent", bytes_sent, size);
    }
  }

  return bytes_sent;
}

ssize_t UdpServer::Recv(char* data, size_t maxSize)
{
  socklen_t cliLen = sizeof(cliaddr);
  ssize_t bytes_received = recvfrom(socketfd, data, maxSize, 0, (struct sockaddr *)&cliaddr,&cliLen);
  
  if (bytes_received <= 0) {
    if (errno != EWOULDBLOCK)
    {
      LOG_ERROR("UdpServer.Recv.RecvError", "");
    } else {
      bytes_received = 0;
    }
  } else {
    //LOG_DEBUG("UdpServer.Recv", " %zd bytes received", bytes_received);

    // Add client to list
    if (AddClient(cliaddr)) {
      LOG_DEBUG("UdpServer.Recv.NewClient", "[%s]", _name.c_str());
    }

    // Check if this is a connection packet
    if (bytes_received == sizeof(kConnectionPacket) && 
        strncmp(data, kConnectionPacket, sizeof(kConnectionPacket)) == 0)  {
      LOG_DEBUG("UdpServer.Recv.ReceivedConnectionPacket", "[%s]", _name.c_str());
      return 0;
    }

  }
  
  return bytes_received;
}

bool UdpServer::AddClient(struct sockaddr_in &c)
{
  for (client_list_it it = client_list.begin(); it != client_list.end(); it++) {
    if ( memcmp(&(*it),&c, sizeof(c)) == 0) 
      return false;
  }

  LOG_DEBUG("UdpServer.AddClient.AddingClient", "");
  client_list.push_back(c);
  return true;
}


bool UdpServer::HasClient() 
{
  return !client_list.empty();
}

void UdpServer::DisconnectClient()
{
  client_list.clear();
}

int UdpServer::GetNumClients()
{
  const ssize_t numClients = client_list.size();
  
  if(numClients > std::numeric_limits<int>::max()) {
    LOG_WARNING("UdpServer.GetNumClients.TooManyClients", "More than MAX_INT clients");
  }
  
  return static_cast<int>(numClients);
}
