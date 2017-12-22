/**
 * File: interRobotComms.cpp
 *
 * Author: Kevin Yoon
 * Created: 2017-12-15
 *
 * Description: Manages communications with other robots on the same network
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "engine/components/interRobotComms.h"

// #include "anki/messaging/shared/UdpClient.h"
// #include "anki/messaging/shared/TcpClient.h"
// #include "anki/messaging/shared/TcpServer.h"

#include "anki/messaging/shared/multicastSocket.h"

#define MCAST_PORT 12349
#define MCAST_GROUP "225.0.0.37"

namespace Anki {
namespace Cozmo {
 
namespace {
  MulticastSocket _sock;
}
  
InterRobotComms::InterRobotComms()
{
  _sock.Connect(MCAST_GROUP, MCAST_PORT, false);
}

void InterRobotComms::JoinChannel()
{

}

ssize_t InterRobotComms::Send(const char* data, int size) 
{
  return _sock.Send(data, size);
}

ssize_t InterRobotComms::Recv(char* data, int maxSize, unsigned int* sending_ip)
{
  return _sock.Recv(data, maxSize, sending_ip);
}
  
} // namespace Cozmo
} // namespace Anki
