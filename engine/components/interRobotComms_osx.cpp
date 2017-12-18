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

#include "anki/cozmo/shared/cozmoEngineConfig.h"

#include "util/logging/logging.h"

#include <webots/Emitter.hpp>
#include <webots/Receiver.hpp>
#include <webots/Supervisor.hpp>

namespace Anki {
namespace Cozmo {
 
namespace {
  static const int kPartyLineChannel = 90;

  static webots::Supervisor* _sup = nullptr;
  static bool _supervisorIsSet = false;

  webots::Emitter*  _emitter = nullptr;
  webots::Receiver* _receiver = nullptr;
}
  

InterRobotComms::InterRobotComms()
{
  DEV_ASSERT(_supervisorIsSet, "InterRobotComms.Ctor.SupervisorNotSet");

  _emitter = _sup->getEmitter("multiRobotEmitter");
  _receiver = _sup->getReceiver("multiRobotReceiver");

  if (_emitter) {
    _emitter->setChannel(kPartyLineChannel);
  } else {
    PRINT_NAMED_WARNING("InterRobotComms.Ctor.EmitterNotFound", "");
  }

  if (_receiver) {
    _receiver->enable(BS_TIME_STEP);
    _receiver->setChannel(kPartyLineChannel);
  } else {
    PRINT_NAMED_WARNING("InterRobotComms.Ctor.ReceiverNotFound", "");
  }
}  

void InterRobotComms::SetSupervisor(webots::Supervisor* supervisor)
{
  _sup = supervisor;
  _supervisorIsSet = true;
}

void InterRobotComms::JoinChannel()
{
  //
}

ssize_t InterRobotComms::Send(const char* data, int size) 
{
  if (_emitter) {
    if (_emitter->send(data, size)) {
      return size;
    } else {
      PRINT_NAMED_WARNING("InterRobotComms.Recv.SendFailed", "");
    }
  } else {
    PRINT_NAMED_WARNING("InterRobotComms.Recv.NullEmitter", "");
  }
  return 0;
}

ssize_t InterRobotComms::Recv(char* data, int maxSize)
{
  if (_receiver) {
    if (_receiver->getQueueLength() > 0) {
      int dataSize = _receiver->getDataSize();
      u8* dataPtr = (u8*)_receiver->getData();
      
      dataSize = std::min(dataSize, maxSize);
      memcpy(data, dataPtr, dataSize);

      _receiver->nextPacket();
      return dataSize;
    }
  } else {
    PRINT_NAMED_WARNING("InterRobotComms.Recv.NullReceiver", "");
  }
  return 0;
}

  
  
} // namespace Cozmo
} // namespace Anki
