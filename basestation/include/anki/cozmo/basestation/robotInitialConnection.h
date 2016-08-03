/**
 * File: robotInitialConnection
 *
 * Author: baustin
 * Created: 8/1/2016
 *
 * Description: Monitors initial events after robot connection to determine result to report
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Anki_Cozmo_Basestation_RobotInitialConnection_H__
#define __Anki_Cozmo_Basestation_RobotInitialConnection_H__

#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/common/types.h"
#include "util/signals/signalHolder.h"

namespace Anki {
namespace Cozmo {

namespace RobotInterface {
class MessageHandler;
class RobotToEngine;
}

class IExternalInterface;

class RobotInitialConnection : private Util::SignalHolder
{
public:
  RobotInitialConnection(RobotID_t id, RobotInterface::MessageHandler* messageHandler,
    IExternalInterface* externalInterface, uint32_t fwVersion, uint32_t fwTime);

  // called when getting a disconnect message from robot
  // returns true if robot was in the process of connecting and we broadcasted a connection failed message
  bool HandleDisconnect();

private:
  void HandleFactoryFirmware(const AnkiEvent<RobotInterface::RobotToEngine>&);
  void HandleFirmwareVersion(const AnkiEvent<RobotInterface::RobotToEngine>&);
  void OnNotified();

  RobotID_t _id;
  bool _notified;
  IExternalInterface* _externalInterface;
  uint32_t _fwVersion;
  uint32_t _fwTime;
};

}
}

#endif
