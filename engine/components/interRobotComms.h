/**
 * File: interRobotComms.h
 *
 * Author: Kevin Yoon
 * Created: 2017-12-15
 *
 * Description: Manages communications with other robots on the same network
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Cozmo_Basestation_InterRobotComms_H__
#define __Cozmo_Basestation_InterRobotComms_H__

#include "anki/common/types.h"

#include "util/helpers/noncopyable.h"

#include <sys/types.h> // ssize_t

namespace webots {
  class Supervisor;
}

namespace Anki {
namespace Cozmo {


class InterRobotComms  // TODO: non-copyable?
{
public:

  InterRobotComms();
  ~InterRobotComms() = default;

#ifdef SIMULATOR
  static void SetSupervisor(webots::Supervisor* supervisor);
#endif

  void JoinChannel();

  ssize_t Send(const char* data, int size);

  ssize_t Recv(char* data, int maxSize, unsigned int* sending_ip = nullptr);

private:

 
};


} // namespace Cozmo
} // namespace Anki

#endif //__Cozmo_Basestation_InterRobotComms_H__
