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

namespace Anki {
namespace Cozmo {
 
namespace {

}
  
  
InterRobotComms::InterRobotComms()
{
  
}

void InterRobotComms::JoinChannel()
{

}

ssize_t InterRobotComms::Send(const char* data, int size) 
{
  return size;
}

ssize_t InterRobotComms::Recv(char* data, int maxSize)
{
  return 0;
}
  
} // namespace Cozmo
} // namespace Anki
