/*
 * multicastSocket.cpp
 */

#ifndef __MulticastSocket_H__
#define __MulticastSocket_H__

#include <stdint.h>
#include <unistd.h>

class MulticastSocket
{
public:
  MulticastSocket();
//  ~MulticastSocket();
  
  bool Connect(const char* mcast_addr, uint16_t port, bool enable_loop = true);
  void Disconnect();
  bool IsConnected() const;
  
  ssize_t Send(const char* data, size_t size);
  ssize_t Recv(char* data, size_t maxSize, uint32_t* ipAddr = NULL);
private:
  void set_nonblock(int socket);
};

#endif // __MulticastSocket_H__
