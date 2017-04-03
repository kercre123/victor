#include "anki/messaging/shared/UdpServer.h"

#include <unistd.h>
#include <iostream>
#include <sys/types.h>
#include <cstring>      // Needed for memset
#include <sys/socket.h> // Needed for the socket functions
#include <netdb.h>      // Needed for the socket functions
#include <fcntl.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <errno.h>

UdpServer::UdpServer()
{
  socketfd = -1;
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
      DEBUG_UDP_SERVER("WARNING (UdpServer): Already listening");
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
     std::cerr << "getaddrinfo error" << gai_strerror(status) ;
     freeaddrinfo(host_info_list);
     return res;
    }

    DEBUG_UDP_SERVER("UdpServer: Creating a socket on port " << portStr);

    socketfd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                      host_info_list->ai_protocol);
    if (socketfd == -1) {
      std::cerr << "socket error\n" ;
      freeaddrinfo(host_info_list);
      return res;
    }

    DEBUG_UDP_SERVER("UdpServer: Binding socket...");

    // we use to make the setsockopt() function to make sure the port is not in use
    // by a previous execution of our code. (see man page for more information)
    int yes = 1;
    status = setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (status == -1) {
      DEBUG_UDP_SERVER("UdpServer: Failed to set socket options (status=" << status << ")");
    }

    set_nonblock(socketfd);

    
    status = bind(socketfd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
      std::cerr << "**** ERROR: bind error (You might have orphaned processes running) ****\n";
    } else {
      DEBUG_UDP_SERVER("UdpServer: Port is open");
      res = true;
    }

    freeaddrinfo(host_info_list);
    return res;
}

void UdpServer::StopListening() 
{
  client_list.clear();
  if (socketfd >= 0) {
    DEBUG_UDP_SERVER("UdpServer: Stopping server listening on socket " << socketfd);
    

    close(socketfd);
    socketfd = -1;
    return;
  }

  DEBUG_UDP_SERVER("UdpServer: Server already stopped");

}


int UdpServer::Send(const char* data, int size)
{
  if (size <= 0) return 0;

  if (!HasClient()) {
    //DEBUG_UDP_SERVER("UdpServer: Send() failed because no client connected");
    return -1;
  }

  DEBUG_UDP_SERVER("UdpServer: sending   " << data);

  ssize_t bytes_sent = 0;
  for (client_list_it it = client_list.begin(); it != client_list.end(); it++ ) {
    DEBUG_UDP_SERVER("Sending to client ");
    bytes_sent = sendto(socketfd, data, size, 0, (struct sockaddr *)&(*it), sizeof(*it));
    if(bytes_sent != size) {
      std::cerr << "*** ERROR: UdpServer reported sending " << bytes_sent << " bytes instead of " << size << "! ***\n";
    }
  }

  if(bytes_sent > std::numeric_limits<int>::max()) {
    DEBUG_UDP_SERVER("UdpServer warning: bytes sent larger than max int.\n");
  }
  
  return static_cast<int>(bytes_sent);
}

int UdpServer::Recv(char* data, int maxSize)
{
  socklen_t cliLen = sizeof(cliaddr);
  ssize_t bytes_received;
  bytes_received = recvfrom(socketfd, data, maxSize, 0, (struct sockaddr *)&cliaddr,&cliLen);
  
  if (bytes_received <= 0) {
    if (errno != EWOULDBLOCK)
    {
      DEBUG_UDP_SERVER("receive error! " << errno);
    } else {
      bytes_received = 0;
    }
  } else {
    DEBUG_UDP_SERVER("UdpServer: " << bytes_received << " bytes received : " << data);

    // Add client to list
    if (AddClient(cliaddr) && bytes_received == 1) {
      // If client was newly added, the first datagram (as long as it's only 1 byte long)
      // is assumed to be a "connection packet".
      return 0;
    }
  }

  if(bytes_received > std::numeric_limits<int>::max()) {
    DEBUG_UDP_SERVER("UdpServer warning: bytes received larger than max int.\n");
  }
  
  return static_cast<int>(bytes_received);
}
/*
int UdpServer::GetNumBytesAvailable()
{
  int bytes_avail = 0;
  ioctl(socketfd, FIONREAD, &bytes_avail);
  return bytes_avail;
}
*/


bool UdpServer::AddClient(struct sockaddr_in &c)
{
  for (client_list_it it = client_list.begin(); it != client_list.end(); it++) {
    if ( memcmp(&(*it),&c, sizeof(c)) == 0) 
      return false;
  }

  DEBUG_UDP_SERVER("UdpServer: Adding client\n");
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
    DEBUG_UDP_SERVER("UdpServer warning: number of clients larger than max int.\n");
  }
  
  return static_cast<int>(numClients);
}
